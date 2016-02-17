#pragma once

#include "AsmHelperBase.h"

namespace blackbone
{

/// <summary>
/// 32 bit assembler helper
/// </summary>
class AsmHelper32 : public AsmHelperBase
{
public:
    BLACKBONE_API AsmHelper32( );
    BLACKBONE_API ~AsmHelper32( void );

    /// <summary>
    /// Generate function prologue code
    /// </summary>
    /// <param name="switchMode">Unused</param>
    virtual void GenPrologue( bool switchMode = false );

    /// <summary>
    ///  Generate function epilogue code
    /// </summary>
    /// <param name="switchMode">Unused</param>
    /// <param name="retSize">Stack change value</param>
    virtual void GenEpilogue( bool switchMode = false, int retSize = 4 );

    /// <summary>
    /// Generate function call
    /// </summary>
    /// <param name="pFN">Function pointer</param>
    /// <param name="args">Function arguments</param>
    /// <param name="cc">Calling convention</param>
    virtual void GenCall( const AsmVariant& pFN, const std::vector<AsmVariant>& args, eCalligConvention cc = cc_stdcall );

    /// <summary>
    /// Save eax value and terminate current thread
    /// </summary>
    /// <param name="pExitThread">NtTerminateThread address</param>
    /// <param name="resultPtr">Memry where eax value will be saved</param>
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
        uintptr_t errPtr,
        eReturnType rtype = rt_int32
        );

    /// <summary>
    /// Does nothing under x86
    /// </summary>
    /// <param name="">Unused</param>
    virtual void EnableX64CallStack( bool ) { }

    /// <summary>
    /// Move TEB pointer into edx
    /// </summary>
    virtual void SetTebPtr()
    {
        // mov edx, fs:[0x18]   
        _assembler.mov( asmjit::host::edx, asmjit::host::dword_ptr_abs( 0x18 ).setSegment( asmjit::host::fs ) );
    }

private:
    AsmHelper32( const AsmHelper32& ) = delete;
    AsmHelper32& operator = (const AsmHelper32&) = delete;

    /// <summary>
    /// Push function argument
    /// </summary>
    /// <param name="arg">Argument.</param>
    /// <param name="regidx">Push type(register or stack)</param>
    void PushArg( const AsmVariant& arg, eArgType regidx = at_stack );

    /// <summary>
    /// Push argument into function
    /// </summary>
    /// <param name="arg">Argument</param>
    /// <param name="index">Argument location</param>
    template<typename _Type>
    void PushArgp( _Type arg, eArgType index );
};

}

