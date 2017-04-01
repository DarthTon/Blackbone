#pragma once

#include "../../Config.h"
#include "../../Asm/AsmFactory.h"
#include "../../Include/Types.h"
#include "../MemBlock.h"


namespace blackbone
{

/// <summary>
/// Hook data, sizeof = 0x50 bytes
/// </summary>
#pragma pack(push, 1)
struct HookCtx32
{
    uint32_t codeSize;          // Size of saved code
    uint32_t jmp_size;          // Size of jump from thunk to original
    uint8_t original_code[29];  // Original function code

    union
    {
        uint8_t jmp_code[5];    // Jump instruction
        struct  
        {
            uint8_t opcode; 
            int32_t ptr;
        } jmp;
    };
};

struct HookCtx64
{
    uint64_t dst_ptr;           // Target address
    uint32_t codeSize;          // Size of saved code
    uint8_t original_code[32];  // Original function code
    uint8_t far_jmp[6];         // Far jump code
};
#pragma pack(pop)

union HookCtx
{
    HookCtx32 hook32;
    HookCtx64 hook64;
};


/// <summary>
/// In-process remote hook
/// </summary>
class RemoteLocalHook
{
public:
    RemoteLocalHook( class Process& process );
    ~RemoteLocalHook();

    NTSTATUS SetHook( ptr_t address, asmjit::Assembler& hook );
    NTSTATUS Restore();

private:
    RemoteLocalHook( const RemoteLocalHook& ) = delete;
    RemoteLocalHook& operator = (const RemoteLocalHook&) = delete;

    NTSTATUS AllocateMem( ptr_t address, size_t codeSize );

    NTSTATUS SetHook32( ptr_t address, asmjit::Assembler& hook );

    NTSTATUS SetHook64( ptr_t address, asmjit::Assembler& hook );

    bool CopyOldCode( ptr_t address, bool x64 );

private:
    class Process& _process;
    HookCtx _ctx;
    MemBlock _hookData;
    ptr_t _pHookCode = 0;
    ptr_t _pThunkCode = 0;
    ptr_t _address = 0;

    bool _hooked = false;
    bool _hook64 = false;
};

}