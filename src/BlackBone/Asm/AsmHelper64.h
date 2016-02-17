#pragma once

#include "AsmHelperBase.h"
#include "../Include/Macro.h"

namespace blackbone
{

class AsmHelper64 : public AsmHelperBase
{
public:
    BLACKBONE_API AsmHelper64();
    BLACKBONE_API ~AsmHelper64( void );

    /// <summary>
    /// Generate function prologue code
    /// </summary>
    /// <param name="switchMode">true if execution must be swithed to x64 mode</param>
    virtual void GenPrologue( bool switchMode = false );

    /// <summary>
    /// Generate function epilogue code
    /// </summary>
    /// <param name="switchMode">true if execution must be swithed to x86 mode</param>
    /// <param name="retSize">Stack change value</param>
    virtual void GenEpilogue( bool switchMode = false, int retSize = 0 );

    /// <summary>
    /// Generate function call
    /// </summary>
    /// <param name="pFN">Function pointer</param>
    /// <param name="args">Function arguments</param>
    /// <param name="cc">Ignored</param>
    virtual void GenCall( const AsmVariant& pFN, const std::vector<AsmVariant>& args, eCalligConvention cc = cc_stdcall );

    /// <summary>
    /// Save rax value and terminate current thread
    /// </summary>
    /// <param name="pExitThread">NtTerminateThread address</param>
    /// <param name="resultPtr">Memry where rax value will be saved</param>
    virtual void ExitThreadWithStatus( uintptr_t pExitThread, uintptr_t resultPtr );

    /// <summary>
    /// Save return value and signal thread return event
    /// </summary>
    /// <param name="pSetEvent">NtSetEvent address</param>
    /// <param name="ResultPtr">Result value memory location</param>
    /// <param name="EventPtr">Event memory location</param>
    /// <param name="errPtr">Error code memory location</param>
    /// <param name="rtype">Return type</param>
    virtual void SaveRetValAndSignalEvent( 
        uintptr_t pSetEvent,
        uintptr_t ResultPtr,
        uintptr_t EventPtr,
        uintptr_t lastStatusPtr,
        eReturnType rtype = rt_int32
        );

    /// <summary>
    /// Set stack reservation policy on call generation
    /// </summary>
    /// <param name="state">
    /// If true - stack space will be reserved during each call generation
    /// If false - no automatic stack reservation, user must allocate stack by hand
    /// </param>
    virtual void EnableX64CallStack( bool state );

    /// <summary>
    /// Move TEB pointer into rdx
    /// </summary>
    virtual void SetTebPtr()
    {
        // mov rdx, gs:[0x30]   
        _assembler.mov( asmjit::host::rdx, asmjit::host::dword_ptr_abs( 0x30 ).setSegment( asmjit::host::gs ) );
    }

private:
    AsmHelper64( const AsmHelper64& ) = delete;
    AsmHelper64& operator = (const AsmHelper64&) = delete;

    /// <summary>
    /// Push function argument
    /// </summary>
    /// <param name="arg">Argument.</param>
    /// <param name="regidx">Push type(register or stack)</param>
    void PushArg( const AsmVariant& arg, size_t index );

    /// <summary>
    /// Push function argument
    /// </summary>
    /// <param name="arg">Argument</param>
    /// <param name="index">Argument index</param>
    /// <param name="fpu">true if argument is a floating point value</param>
    template<typename _Type>
    void PushArgp( const _Type& arg, size_t index, bool fpu = false );

private:
    bool _stackEnabled;     // if true - GenCall will allocate shadow stack space
};

}
