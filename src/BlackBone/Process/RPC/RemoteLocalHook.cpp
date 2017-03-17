#include "RemoteLocalHook.h"

#include "../Process.h"
#include "../../Asm/LDasm.h"

namespace blackbone
{

RemoteLocalHook::RemoteLocalHook( class Process& process )
    : _process( process )
{
}

RemoteLocalHook::~RemoteLocalHook()
{
    Restore();
}

NTSTATUS RemoteLocalHook::AllocateMem( ptr_t /*address*/, size_t codeSize )
{
    auto pagesize = _process.core().native()->pageSize();
    auto size = Align( codeSize, pagesize ) + Align( sizeof( _ctx ), pagesize );

    auto allocation = _process.memory().Allocate( size, PAGE_EXECUTE_READWRITE );
    if (!allocation)
        return allocation.status;

    _hookData   = std::move( allocation.result() );
    _pHookCode  = _hookData.ptr();
    _pThunkCode = _pHookCode + _hookData.size() - pagesize;

    return allocation.status;
}

NTSTATUS RemoteLocalHook::SetHook( ptr_t address, asmjit::Assembler& hook )
{
    _address = address;
    NTSTATUS status = AllocateMem( address, hook.getCodeSize() );
    if (!NT_SUCCESS( status ))
        return status;

    return _process.core().isWow64() ? SetHook32( address, hook ) : SetHook64( address, hook );
}

NTSTATUS RemoteLocalHook::SetHook32( ptr_t address, asmjit::Assembler& hook )
{
    NTSTATUS status = STATUS_SUCCESS;
    auto& mem = _process.memory();

    if (!CopyOldCode( address, false ))
        return STATUS_UNSUCCESSFUL;

    _ctx.hook32.codeSize = 5;
    _ctx.hook32.jmp.opcode = 0xE9;
    _ctx.hook32.jmp.ptr = static_cast<int32_t>(_pThunkCode - address - 5);

    uint8_t buf[0x1000] = { 0 };
    hook.setBaseAddress( _hookData.ptr<int32_t>() );
    hook.relocCode( buf );

    mem.Write( _pHookCode, buf );
    mem.Write( _pThunkCode, _ctx.hook32.codeSize + _ctx.hook32.jmp_size, _ctx.hook32.original_code );

    DWORD flOld = 0;
    mem.Protect( address, sizeof( _ctx.hook32.jmp_code ), PAGE_EXECUTE_READWRITE, &flOld );
    status = mem.Write( address, _ctx.hook32.jmp_code );
    mem.Protect( address, sizeof( _ctx.hook32.jmp_code ), flOld );

    if (NT_SUCCESS( status ))
        _hooked = true;

    return status;
}

NTSTATUS RemoteLocalHook::SetHook64( ptr_t /*address*/, asmjit::Assembler& /*hook*/ )
{
    _hook64 = true;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS RemoteLocalHook::Restore()
{
    if (!_hooked)
        return STATUS_SUCCESS;

    DWORD flOld = 0;
    _process.memory().Protect( _address, sizeof( _ctx.hook32.jmp_code ), PAGE_EXECUTE_READWRITE, &flOld );
    NTSTATUS status = _process.memory().Write( _address, _ctx.hook32.codeSize, _ctx.hook32.original_code );
    _process.memory().Protect( _address, sizeof( _ctx.hook32.jmp_code ), flOld );
    
    if (NT_SUCCESS( status ))
    {
        _hookData.Free();
        _pHookCode = _pThunkCode = 0;
        _address = 0;
        _hooked = false;
    }

    return status;
}

bool RemoteLocalHook::CopyOldCode( ptr_t address, bool x64 )
{
    _process.memory().Read( address, sizeof( _ctx.hook32.original_code ), _ctx.hook32.original_code );

    // Store original bytes
    uint8_t* src = _ctx.hook32.original_code;
    uint8_t* old = (uint8_t*)(_hookData.ptr() + _hookData.size() - _process.core().native()->pageSize());
    uint32_t all_len = 0;
    ldasm_data ld = { 0 };

    do
    {
        uint32_t len = ldasm( src, &ld, x64 );

        // Determine code end
        if (ld.flags & F_INVALID
            || (len == 1 && (src[ld.opcd_offset] == 0xCC || src[ld.opcd_offset] == 0xC3))
            || (len == 3 && src[ld.opcd_offset] == 0xC2)
            || len + all_len > 128)
        {
            break;
        }

        // if instruction has relative offset, calculate new offset 
        if (ld.flags & F_RELATIVE)
        {
            int32_t diff = 0;
            const uintptr_t ofst = (ld.disp_offset != 0 ? ld.disp_offset : ld.imm_offset);
            const uintptr_t sz = ld.disp_size != 0 ? ld.disp_size : ld.imm_size;

            memcpy( &diff, src + ofst, sz );

            if(x64)
            {
                // exit if jump is greater then 2GB
                /*if (_abs64( src + len + diff - old ) > INT_MAX)
                {
                    break;
                }
                else
                {
                    diff += static_cast<int32_t>(src - old);
                    memcpy( old + ofst, &diff, sz );
                }*/
            }
            else
            {
                //diff += src - old;
                memcpy( src + ofst, &diff, sz );
            }
        }

        src += len;
        old += len;
        all_len += len;

    } while (all_len < sizeof( _ctx.hook32.jmp_code ));

    // Failed to copy old code, use backup plan
    if (all_len >= sizeof( _ctx.hook32.jmp_code ))
    {
        _ctx.hook32.jmp_size = 5;
        _ctx.hook32.codeSize = all_len;
        *src = 0xE9;
        //*(int32_t*)(src + 1) = (int32_t)address + all_len - (int32_t)old - 5;
        return true;
    }

    return false;
}

}
