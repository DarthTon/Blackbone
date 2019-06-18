#pragma once

#include "../../Config.h"
#include "../../Asm/AsmFactory.h"
#include "../../Include/Types.h"
#include "../MemBlock.h"

namespace blackbone
{

/// <summary>
/// In-process remote hook
/// </summary>
class RemoteLocalHook
{
public:
    // Must be large enough to hold ANY jump this code uses
    static const size_t MaxHookJumpCodeLen = 14;

    // The displaced code should be at most 1 hook length + 1 instruction - 1 byte (if the hook jump overlaps with only the
    // first byte of the following instruction), and x86_64 instructions can be at most 15 bytes.
    static const size_t MaxOriginalCodeLen = MaxHookJumpCodeLen + 14;

    static const size_t MaxPatchedOriginalCodeLen = MaxOriginalCodeLen;

    enum eJumpStrategy
    {
        JumpPushMovRet,
        JumpMovRegRet
    };

private:
    /// <summary>
    /// Hook data
    /// </summary>
    #pragma pack(push, 1)
    struct HookCtx
    {
        ptr_t address; // Hooked address in original code
        ptr_t thunkAddr;
        uint8_t origCodeSize; // Size of displaced original code
        uint8_t origCode[MaxOriginalCodeLen]; // Copy of displaced original code
        uint8_t patchedOrigCode[MaxPatchedOriginalCodeLen];
        uint8_t hookJumpCode[MaxHookJumpCodeLen];
        uint8_t hookJumpCodeSize;
    };
    #pragma pack(pop)

public:
    BLACKBONE_API RemoteLocalHook( class Process* process );
    RemoteLocalHook( const RemoteLocalHook& ) = delete;

    BLACKBONE_API ~RemoteLocalHook();

    void SetJumpStrategy(eJumpStrategy strategy);
    void SetJumpRegister(const asmjit::X86GpReg& reg);

    BLACKBONE_API void SetHook( ptr_t address, asmjit::Assembler& hook );
    BLACKBONE_API void Restore();

    NTSTATUS PrepareHook( ptr_t address, size_t maxHookSize );

    size_t GetDisplacedOriginalCode( uint8_t* code = nullptr );

    bool isHooked() const { return _hooked; }

private:
    void AllocateMem( ptr_t address, size_t hookCodeSize );

    void CopyOldCode( bool x64 );

    uint8_t GenerateJump( uint8_t* code, ptr_t toAddr, ptr_t fromAddr, bool x64 ) const;

private:
    class Process* _process;
    HookCtx _ctx;
    MemBlock _hookData;
    eJumpStrategy _jumpStrategy = JumpPushMovRet;
    const asmjit::X86GpReg* _jumpRegister = &asmjit::host::rax;

    bool _prepared = false;
    bool _hooked = false;
};

}
