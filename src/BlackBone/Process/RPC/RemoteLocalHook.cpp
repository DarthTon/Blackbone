#include "RemoteLocalHook.h"

#include "../Process.h"
#include "../../Asm/LDasm.h"

#include <cstdio>



// Must be enough to hold the displaced original code (instr_align(sizeof(1*jmp64))) plus the return jmp (sizeof(1*jmp64))
#define THUNK_MAX_SIZE 50



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

NTSTATUS RemoteLocalHook::AllocateMem( ptr_t address, size_t hookCodeSize )
{
	auto pagesize = _process.core().native()->pageSize();
    auto size = Align( hookCodeSize + THUNK_MAX_SIZE, pagesize );

    auto allocation = _process.memory().AllocateClosest( size, PAGE_EXECUTE_READWRITE, address );
    if (!allocation)
        return allocation.status;

    _hookData   = std::move( allocation.result() );
    return allocation.status;
}

void RemoteLocalHook::SetJumpStrategy(eJumpStrategy strategy)
{
	_jumpStrategy = strategy;
}

void RemoteLocalHook::SetJumpRegister(const asmjit::X86GpReg& reg)
{
	_jumpRegister = &reg;
}

size_t RemoteLocalHook::GetDisplacedOriginalCode( uint8_t* code )
{
	if (!_prepared)
		return 0;
	if (code)
		memcpy(code, _ctx.origCode, _ctx.origCodeSize);
	return _ctx.origCodeSize;
}

NTSTATUS RemoteLocalHook::PrepareHook( ptr_t address, size_t maxHookSize )
{
	_ctx.address = address;
    NTSTATUS status = AllocateMem( address, maxHookSize );
    if (!NT_SUCCESS( status ))
        return status;

    _ctx.thunkAddr = _hookData.ptr() + maxHookSize;


    bool x64 = !_process.core().isWow64();

    _ctx.hookJumpCodeSize = GenerateJump( _ctx.hookJumpCode, _hookData.ptr(), address, x64 );

    status = CopyOldCode( x64 );
    if (!NT_SUCCESS( status )) {
    	_hookData.Free();
    	_hookData = MemBlock();
		return status;
    }

    _prepared = true;

    return STATUS_SUCCESS;
}

NTSTATUS RemoteLocalHook::SetHook( ptr_t address, asmjit::Assembler& hook )
{
	bool x64 = !_process.core().isWow64();
    auto& mem = _process.memory();

    NTSTATUS status = STATUS_SUCCESS;

    if (!_prepared) {
    	status = PrepareHook( address, hook.getCodeSize() );
    	if (!NT_SUCCESS( status )) {
    		return status;
    	}
    }

    uint8_t hookCode[256];
    uint8_t* heapHookCode = nullptr; // Only used if hook.getCodeSize() > sizeof(hookCode)

    if (hook.getCodeSize() > sizeof(hookCode)) {
    	heapHookCode = new uint8_t[hook.getCodeSize()];
    }

    hook.setBaseAddress( _hookData.ptr() );
	hook.relocCode( heapHookCode ? heapHookCode : hookCode );

	uint8_t jmpBackCode[sizeof(_ctx.hookJumpCode)];
	uint8_t jmpBackCodeSize;

	jmpBackCodeSize = GenerateJump(jmpBackCode, address + _ctx.origCodeSize, _hookData.ptr() + hook.getCodeSize() + _ctx.origCodeSize, x64);

	if (hook.getCodeSize() > (_ctx.thunkAddr - _hookData.ptr())) {
    	// Can happen if PrepareHook() was called manually with maxCodeSize < hook.getCodeSize().
		delete[] heapHookCode;
    	return STATUS_NO_MEMORY;
    }

    mem.Write( _hookData.ptr(), hook.getCodeSize(), heapHookCode ? heapHookCode : hookCode );
	mem.Write( _ctx.thunkAddr, _ctx.origCodeSize, _ctx.patchedOrigCode );
	mem.Write( _ctx.thunkAddr + _ctx.origCodeSize, jmpBackCodeSize, jmpBackCode );

	// Fill region between end of hook and start of thunk with nop. This region is normally empty, but can be non-empty
	// if PrepareHook() was called manually with maxCodeSize > hook.getCodeSize().
	for (ptr_t addr = _hookData.ptr() + hook.getCodeSize() ; addr < _ctx.thunkAddr ; addr++)
	{
		uint8_t nop = 0x90;
		mem.Write(addr, nop);
	}

	delete[] heapHookCode;

	DWORD flOld = 0;
	mem.Protect( address, _ctx.hookJumpCodeSize, PAGE_EXECUTE_READWRITE, &flOld );
	status = mem.Write( address, _ctx.hookJumpCodeSize, _ctx.hookJumpCode );
	mem.Protect( address, _ctx.hookJumpCodeSize, flOld );

	if (NT_SUCCESS( status ))
		_hooked = true;

	return status;
}

NTSTATUS RemoteLocalHook::Restore()
{
    NTSTATUS status = STATUS_SUCCESS;

	if (_hooked) {
		DWORD flOld = 0;
		_process.memory().Protect( _ctx.address, _ctx.hookJumpCodeSize, PAGE_EXECUTE_READWRITE, &flOld );
		status = _process.memory().Write( _ctx.address, _ctx.origCodeSize, _ctx.origCode );
		_process.memory().Protect( _ctx.address, _ctx.hookJumpCodeSize, flOld );

		if (!NT_SUCCESS( status )) {
			return status;
		}
	}
	if (_hookData.valid()) {
		_hookData.Free();
		_hookData = MemBlock();
	}

	_prepared = false;
	_hooked = false;

    return status;
}

NTSTATUS RemoteLocalHook::CopyOldCode( bool x64 )
{
	NTSTATUS status = STATUS_SUCCESS;

	_process.memory().Read( _ctx.address, sizeof( _ctx.origCode ), _ctx.origCode );
	memcpy(_ctx.patchedOrigCode, _ctx.origCode, sizeof(_ctx.patchedOrigCode));

    // Store original bytes
	uint8_t* src = _ctx.origCode;
	ptr_t newAddr = _ctx.thunkAddr;
    uint32_t thunkSize = 0;
    ldasm_data ld = { 0 };

    const int64_t diffMinVals[] = {0ll, -128ll, -32768ll, -8388608ll, -2147483648ll, -549755813888ll, -140737488355328ll, -36028797018963968ll, -9223372036854775807ll};
    const int64_t diffMaxVals[] = {0ll, 127ll, 32767ll, 8388607ll, 2147483647ll, 549755813887ll, 140737488355327ll, 36028797018963967ll, 9223372036854775807ll};

    do
    {
        uint32_t len = ldasm( src, &ld, x64 );

        // Determine code end
        if (ld.flags & F_INVALID
            || (len == 1 && (src[ld.opcd_offset] == 0xCC || src[ld.opcd_offset] == 0xC3))
            || (len == 3 && src[ld.opcd_offset] == 0xC2)
            || len + thunkSize > 128)
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

            // An attempted (partial) solution to https://github.com/DarthTon/Blackbone/issues/418
            // TODO: Do NOT adjust the offset if it points to WITHIN the code that's being moved!

            int64_t newDiff = ((int64_t) diff) + (((ptr_t) (_ctx.address+thunkSize))-newAddr);

            if (newDiff < diffMinVals[sz]  ||  newDiff > diffMaxVals[sz]) {
            	status = STATUS_NOT_IMPLEMENTED;
            	break;
            }

            memcpy(_ctx.patchedOrigCode + thunkSize + ofst, &newDiff, sz);
        }

        src += len;
        newAddr += len;
        thunkSize += len;
    } while (thunkSize < _ctx.hookJumpCodeSize);

    assert(thunkSize <= MaxOriginalCodeLen);

    if (thunkSize < _ctx.hookJumpCodeSize)
    {
    	// TODO: Anything else we can do now?
    }
    else
    {
		_ctx.origCodeSize = static_cast<uint8_t>(thunkSize);
    }

    return status;
}

uint8_t RemoteLocalHook::GenerateJump( uint8_t* code, ptr_t toAddr, ptr_t fromAddr, bool x64 ) const
{
	size_t size = 0;

	auto asmp = AsmFactory::GetAssembler();
	auto& a = *asmp;

	int64_t relJmp = toAddr >= fromAddr ? (int64_t) (toAddr-fromAddr) : -(int64_t)(fromAddr-toAddr);

	if (x64  &&  _abs64( relJmp ) > INT32_MAX)
	{
		switch (_jumpStrategy)
		{
		case JumpPushMovRet:
			// A relatively non-intrusive way to jmp far on x86_64, leaving all registers intact.
			// As described on Nikolay Igotti's blog:
			//		https://web.archive.org/web/20090504135800/http://blogs.sun.com/nike/entry/long_absolute_jumps_on_amd64
			// See also Gil Dabah's blog post, where it's #3:
			//		https://www.ragestorm.net/blogs/?p=107

			// push toAddr[0:31]
			*code = 0x68;
			*((uint32_t*) (code+1)) = (uint32_t) (toAddr & 0xFFFFFFFF);

			if ((toAddr >> 32) != 0)
			{
				// mov [rsp+4], toAddr[32:63]
				*((uint32_t*) (code+5)) = 0x042444C7;
				*((uint32_t*) (code+9)) = (uint32_t) (toAddr >> 32);

				// ret
				*(code+13) = 0xC3;

				size = 14;
			}
			else
			{
				// ret
				*(code+5) = 0xC3;
				size = 6;
			}
			break;
		case JumpMovRegRet:
			// Alternative method that overwrites a register, but keeps the stack untouched. See #2:
			//		https://www.ragestorm.net/blogs/?p=107
			a->mov(*_jumpRegister, (uint64_t) toAddr);
			a->jmp(*_jumpRegister);
			size = a->relocCode(code);
			break;
		}
	}
	else
	{
		// jmp rel toAddr
		*code = 0xE9;
		*((int32_t*) (code+1)) = (int32_t) (relJmp - 5);

		size = 5;
	}

	assert(size <= sizeof(_ctx.hookJumpCode));
	return static_cast<uint8_t>(size);
}

}
