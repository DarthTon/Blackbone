#pragma once

#include "../../Config.h"
#include "../../Asm/AsmHelper.h"
#include "../../Include/Types.h"

#include <unordered_map>

namespace blackbone
{

/// <summary>
/// Hook data, sizeof = 0x50 bytes
/// </summary>
struct HookCtx32
{
    uint32_t codeSize;          // Size of saved code
    uint8_t original_code[29];  // Original function code
    uint8_t jmp_code[5];        // Jump instruction
};

struct HookCtx64
{
    uint64_t dst_ptr;           // Target address
    uint32_t codeSize;          // Size of saved code
    uint8_t original_code[32];  // Original function code
    uint8_t far_jmp[6];         // Far jump code
};

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
    typedef std::unordered_map<ptr_t, HookCtx> mapCtx;

public:
    RemoteLocalHook( class Process& process );
    ~RemoteLocalHook();

    NTSTATUS SetHook( ptr_t address, asmjit::Assembler& hook );

    NTSTATUS Restore( ptr_t address );

private:
    RemoteLocalHook( const RemoteLocalHook& ) = delete;
    RemoteLocalHook& operator = (const RemoteLocalHook&) = delete;

private:
    class Process& _process;
    mapCtx _hooks;
};

}