#include "AsmHelper64.h"

#include <assert.h>

namespace blackbone
{

AsmHelper64::AsmHelper64( AsmJit::Assembler& _a )
    : AsmHelperBase( _a )
    , _stackEnabled( true )
{
}

AsmHelper64::~AsmHelper64( void )
{
}

/// <summary>
/// Generate function prologue code
/// </summary>
/// <param name="switchMode">true if execution must be swithed to x64 mode</param>
void AsmHelper64::GenPrologue( bool switchMode /*= false*/ )
{
    // If switch is required, function was called from x86 mode,
    // so arguments can't be saved in x64 way, because there is no shadow space on the stack
    if (switchMode)
    {
        SwitchTo64();

        // Align stack
        a.and_( AsmJit::nsp, 0xFFFFFFFFFFFFFFF0 );
    }
    else
    {
        a.mov( AsmJit::qword_ptr( AsmJit::rsp, 1 * WordSize ), AsmJit::rcx );
        a.mov( AsmJit::qword_ptr( AsmJit::rsp, 2 * WordSize ), AsmJit::rdx );
        a.mov( AsmJit::qword_ptr( AsmJit::rsp, 3 * WordSize ), AsmJit::r8 );
        a.mov( AsmJit::qword_ptr( AsmJit::rsp, 4 * WordSize ), AsmJit::r9 );
    }
}

/// <summary>
/// Generate function epilogue code
/// </summary>
/// <param name="switchMode">true if execution must be swithed to x86 mode</param>
/// <param name="retSize">Stack change value</param>
void AsmHelper64::GenEpilogue( bool switchMode /*= false*/, int retSize /*= 0*/ )
{
    UNREFERENCED_PARAMETER( retSize );

    if (switchMode)
    {
        SwitchTo86();
    }
    else
    {
        a.mov( AsmJit::rcx, AsmJit::qword_ptr( AsmJit::rsp, 1 * WordSize ) );
        a.mov( AsmJit::rdx, AsmJit::qword_ptr( AsmJit::rsp, 2 * WordSize ) );
        a.mov( AsmJit::r8, AsmJit::qword_ptr( AsmJit::rsp, 3 * WordSize ) );
        a.mov( AsmJit::r9, AsmJit::qword_ptr( AsmJit::rsp, 4 * WordSize ) );
    }

    a.ret();
}

/// <summary>
/// Generate function call
/// </summary>
/// <param name="pFN">Function pointer</param>
/// <param name="args">Function arguments</param>
/// <param name="cc">Ignored</param>
void AsmHelper64::GenCall( const AsmVariant& pFN, const std::vector<AsmVariant>& args, eCalligConvention /*cc = CC_stdcall*/ )
{
    //
    // reserve stack size (0x28 - minimal size for 4 registers and return address)
    // after call, stack must be aligned on 16 bytes boundary
    //
    size_t rsp_dif = (args.size() > 4) ? args.size() * WordSize : 0x28;

    // align on (16 bytes - sizeof(return address))
    rsp_dif = Align( rsp_dif, 0x10 );

    if (_stackEnabled)
        a.sub( AsmJit::rsp, rsp_dif + 8 );

    // Set args
    for (size_t i = 0; i < args.size(); i++)
        PushArg( args[i], i );

    if (pFN.type == AsmVariant::imm)
    {
        a.mov( AsmJit::r13, pFN.imm_val );
        a.call( AsmJit::r13 );
    }
    else if (pFN.type == AsmVariant::reg)
    {
        a.call( pFN.reg_val );
    }
    else
        assert("Invalid function pointer type" && false );

    if (_stackEnabled)
        a.add( AsmJit::rsp, rsp_dif + 8 );
}

/// <summary>
/// Save rax value and terminate current thread
/// </summary>
/// <param name="pExitThread">NtTerminateThread address</param>
/// <param name="resultPtr">Memry where rax value will be saved</param>
void AsmHelper64::ExitThreadWithStatus( uint64_t pExitThread, size_t resultPtr )
{
    if (resultPtr != 0)
    {
        a.mov( AsmJit::rdx, resultPtr );
        a.mov( AsmJit::dword_ptr( AsmJit::rdx ), AsmJit::rax );
    }

    a.mov( AsmJit::rdx, AsmJit::rax );
    a.mov( AsmJit::rcx, 0 );
    a.mov( AsmJit::r13, pExitThread );
    a.call( AsmJit::r13 );
}

/// <summary>
/// Save return value and signal thread return event
/// </summary>
/// <param name="pSetEvent">NtSetEvent address</param>
/// <param name="ResultPtr">Result value memory location</param>
/// <param name="EventPtr">Event memory location</param>
/// <param name="errPtr">Error code memory location</param>
/// <param name="rtype">Return type</param>
void AsmHelper64::SaveRetValAndSignalEvent( size_t pSetEvent,
                                            size_t ResultPtr,
                                            size_t EventPtr, 
                                            size_t lastStatusPtr, 
                                            eReturnType rtype /*= rt_int32*/ )
{
    a.mov( AsmJit::rcx, ResultPtr );

    // FPU value has been already saved
    if (rtype == rt_int64 || rtype == rt_int32)
        a.mov( AsmJit::dword_ptr( AsmJit::rcx ), AsmJit::rax );

    // Save last NT status
    SetTebPtr();
    a.add( AsmJit::rdx, LAST_STATUS_OFS );
    a.mov( AsmJit::rdx, AsmJit::dword_ptr( AsmJit::rdx ) );
    a.mov( AsmJit::rax, lastStatusPtr );
    a.mov( AsmJit::dword_ptr( AsmJit::rax ), AsmJit::rdx );

    // NtSetEvent(hEvent, NULL)
    a.mov( AsmJit::rax, EventPtr );
    a.mov( AsmJit::rcx, AsmJit::dword_ptr( AsmJit::rax ) );
    a.mov( AsmJit::rdx, 0 );
    a.mov( AsmJit::r13, pSetEvent );
    a.call( AsmJit::r13 );
}


/// <summary>
/// Set stack reservation policy on call generation
/// </summary>
/// <param name="state">
/// If true - stack space will be reserved during each call generation
/// If false - no automatic stack reservation, user must allocate stack by hand
/// </param>
void AsmHelper64::EnableX64CallStack( bool state )
{
    _stackEnabled = state;
}

/// <summary>
/// Push function argument
/// </summary>
/// <param name="arg">Argument.</param>
/// <param name="regidx">Push type(register or stack)</param>
void AsmHelper64::PushArg( const AsmVariant& arg, size_t index )
{
    switch (arg.type)
    {

    case AsmVariant::imm:
    case AsmVariant::structRet:
        PushArgp( arg.imm_val, index );
        break;

    case AsmVariant::dataPtr:
    case AsmVariant::dataStruct:
        PushArgp( arg.new_imm_val, index );
        break;

    case AsmVariant::imm_double:        
        PushArgp( arg.getImm_double(), index, true );
        break;

    case AsmVariant::imm_float:
        PushArgp( arg.getImm_float(), index, true );
        break;

    case AsmVariant::mem_ptr:
        a.lea( AsmJit::rax, arg.mem_val );
        PushArgp( AsmJit::rax, index );
        break;

    case AsmVariant::mem:
        PushArgp( arg.mem_val, index );
        break;

    case AsmVariant::reg:
        PushArgp( arg.reg_val, index );
        break;

    default:
        assert( "Invalid argument type" && false );
        break;
    }
}

/// <summary>
/// Push function argument
/// </summary>
/// <param name="arg">Argument</param>
/// <param name="index">Argument index</param>
/// <param name="fpu">true if argument is a floating point value</param>
template<typename _Type>
void AsmHelper64::PushArgp( const _Type& arg, size_t index, bool fpu /*= false*/ )
{
    static const AsmJit::GPReg regs[] = { AsmJit::rcx, AsmJit::rdx, AsmJit::r8, AsmJit::r9 };
    static const AsmJit::XMMReg xregs[] = { AsmJit::xmm0, AsmJit::xmm1, AsmJit::xmm2, AsmJit::xmm3 };

    // Pass via register
    if (index < 4)
    {
        // Use XMM register
        if (fpu)
        {
            a.mov( AsmJit::rax, arg );
            a.movq( xregs[index], AsmJit::rax );
        }
        else
            a.mov( regs[index], arg );
    }
    // Pass on stack
    else
    {
        a.mov( AsmJit::r15, arg );
        a.mov( AsmJit::qword_ptr( AsmJit::rsp, index * WordSize ), AsmJit::r15 );
    }
}

}
