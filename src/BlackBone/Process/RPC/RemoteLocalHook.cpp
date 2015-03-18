#include "RemoteLocalHook.h"

#include "../Process.h"
#include "../../Asm/LDasm.h"

#ifdef USE64
#define HOOK_CTX_T(v, ctx) HookCtx64& v = ctx.hook64
#else
#define HOOK_CTX_T(v, ctx) HookCtx32& v = ctx.hook32
#endif

namespace blackbone
{

RemoteLocalHook::RemoteLocalHook( class Process& process )
    : _process( process )
{
}


RemoteLocalHook::~RemoteLocalHook()
{
}

NTSTATUS RemoteLocalHook::SetHook( ptr_t address, asmjit::Assembler& /*hook*/ )
{
    HookCtx ctx = { { 0 } };
    HOOK_CTX_T( ctx_t, ctx );
    UNREFERENCED_PARAMETER( ctx_t );

    // Already hooked
    if (_hooks.count( address ) != 0)
        return STATUS_ADDRESS_ALREADY_EXISTS;

    auto memBlock = _process.memory().Allocate( 0x1000 );
    if (!memBlock.valid())
        return LastNtStatus();

    auto status = _process.memory().Read( address, sizeof( ctx_t.original_code ), ctx_t.original_code );
    if (!NT_SUCCESS( status ))
        return status;

    // Copy original
    uint8_t* codePtr = ctx_t.original_code;
    ptr_t old = 0;
    uint32_t all_len = 0;
    ldasm_data ld = { 0 };

    do
    {
        uint32_t len = ldasm( codePtr, &ld, is_x64 );

        // Determine code end
        if (ld.flags & F_INVALID
             || (len == 1 && (codePtr[ld.opcd_offset] == 0xCC || codePtr[ld.opcd_offset] == 0xC3))
             || (len == 3 && codePtr[ld.opcd_offset] == 0xC2)
             || len + all_len > 128)
        {
            break;
        }

        // if instruction has relative offset, calculate new offset 
        if (ld.flags & F_RELATIVE)
        {
#ifdef USE64
            // exit if jump is greater then 2GB
            if (_abs64( (uintptr_t)(codePtr + *((int*)(old + ld.opcd_size))) - (uintptr_t)old ) > INT_MAX)
                break;
            else
                *(uint32_t*)(old + ld.opcd_size) += (uint32_t)(codePtr - old);
#else
            *(uintptr_t*)(codePtr + ld.opcd_size) += reinterpret_cast<uintptr_t>(codePtr) - static_cast<uintptr_t>(old);
#endif
        }

        codePtr += len;
        old += len;
        all_len += len;

    } while (all_len < 5);


#ifdef USE64
#else 
    *codePtr = ctx_t.jmp_code[0] = 0xE9;
    *(int32_t*)(codePtr + 1) = memBlock.ptr<int32_t>() - static_cast<int32_t>(address)- 5;
    *(int32_t*)(ctx_t.jmp_code + 1) = memBlock.ptr<int32_t>() + FIELD_OFFSET( HookCtx32, original_code ) - static_cast<int32_t>(address) - 5;
#endif

    ctx_t.codeSize = all_len;
    memset( ctx_t.original_code + ctx_t.codeSize, 0x00, sizeof( ctx_t.original_code ) - ctx_t.codeSize );

    memBlock.Write( 0, ctx_t );

    /*DWORD flOld = 0;
    _process.memory().Protect( address, all_len, PAGE_EXECUTE_READWRITE, &flOld );
    _process.memory().Write( address, ctx_t.jmp_code );
    _process.memory().Protect( address, all_len, flOld );*/

    _hooks.emplace( std::make_pair( address, ctx ) );
    memBlock.Release();

    return STATUS_SUCCESS;
}

NTSTATUS RemoteLocalHook::Restore( ptr_t address )
{
    // No such hook
    if (_hooks.count( address ) == 0)
        return STATUS_NOT_FOUND;

    HOOK_CTX_T( ctx_t, _hooks[address] );
    UNREFERENCED_PARAMETER( ctx_t );

    _hooks.erase( address );

    return STATUS_SUCCESS;
}

}