#include "AsmHelper32.h"

#include <set>
#include <assert.h>

namespace blackbone
{

AsmHelper32::AsmHelper32( AsmJit::Assembler& _a )
    : AsmHelperBase( _a )
{
}

AsmHelper32::~AsmHelper32( void )
{
}

/// <summary>
/// Generate function prologue code
/// </summary>
/// <param name="switchMode">Unused</param>
void AsmHelper32::GenPrologue( bool switchMode /*= false*/ )
{
    if(switchMode == true)
    {
        AsmHelperBase::SwitchTo64();
    }
    else
    {
        a.push( AsmJit::ebp );
        a.mov( AsmJit::ebp, AsmJit::esp );
    }
}

/// <summary>
///  Generate function epilogue code
/// </summary>
/// <param name="switchMode">Unused</param>
/// <param name="retSize">Stack change value</param>
void AsmHelper32::GenEpilogue( bool switchMode /*= false*/ , int retSize /*= WordSize */ )
{
    if (retSize == -1)
        retSize = WordSize;

    if (switchMode == true)
    {
        AsmHelperBase::SwitchTo86();
    }
    else
    {
        a.mov( AsmJit::esp, AsmJit::ebp );
        a.pop( AsmJit::ebp );
    }

    a.ret( retSize );
}

/// <summary>
/// Generate function call
/// </summary>
/// <param name="pFN">Function pointer</param>
/// <param name="args">Function arguments</param>
/// <param name="cc">Calling convention</param>
void AsmHelper32::GenCall( const AsmVariant& pFN, const std::vector<AsmVariant>& args, eCalligConvention cc /*= CC_stdcall*/ )
{
    std::set<int> passedIdx;

    // Pass arguments in registers
    // 64bit arguments are never passed in registers
    if ((cc == cc_thiscall || cc == cc_fastcall) && !args.empty())
        for (int i = 0, j = 0; i < static_cast<int>(args.size()) && (j < ((cc == cc_fastcall) ? 2 : 1)); i++)
            if (args[i].reg86Compatible())
            {
                PushArg( args[i], (eArgType)j );
                passedIdx.emplace( i );
                j++;
            }      

    // Push args on stack
    for (int i = (int)args.size() - 1; i >= 0; i--)
    {
        // Skip arguments passed in registers
        if (passedIdx.count( i ) == 0)
            PushArg( args[i] );
    }
 
    // Direct pointer
    if(pFN.type == AsmVariant::imm)
    {
        a.mov( AsmJit::eax, pFN.imm_val );
        a.call( AsmJit::eax );
    }
    // Already in register
    else if (pFN.type == AsmVariant::reg)
    {
        a.call( pFN.reg_val );
    }
    else
    {
        assert( "Invalid function pointer type" && false );
    }

    // Adjust stack ptr
    if (cc == cc_cdecl)
    {
        size_t argsize = 0;

        for (auto& arg : args)
        {
            if (arg.type != AsmVariant::dataPtr)
                argsize += arg.size;
            else
                argsize += sizeof(void*);
        }

        a.add( AsmJit::esp, argsize );
    }
      
}

/// <summary>
/// Save eax value and terminate current thread
/// </summary>
/// <param name="pExitThread">NtTerminateThread address</param>
/// <param name="resultPtr">Memry where eax value will be saved</param>
void AsmHelper32::ExitThreadWithStatus( size_t pExitThread, size_t resultPtr = 0 )
{
    if (resultPtr != 0)
    {
        a.mov( AsmJit::edx, resultPtr );
        a.mov( AsmJit::dword_ptr( AsmJit::edx ), AsmJit::eax );
    }

    // NtTerminateThread( NULL, eax );
    a.push( AsmJit::eax );
    a.push( 0 );
    a.mov( AsmJit::eax, pExitThread );
    a.call( AsmJit::eax );
    
    GenEpilogue();
}

/// <summary>
/// Save return value and signal thread return event
/// </summary>
/// <param name="pSetEvent">NtSetEvent address</param>
/// <param name="ResultPtr">Result value memory location</param>
/// <param name="EventPtr">Event memory location</param>
/// <param name="errPtr">Error code memory location</param>
/// <param name="rtype">Return type</param>
void AsmHelper32::SaveRetValAndSignalEvent( size_t pSetEvent,
                                            size_t ResultPtr, 
                                            size_t EventPtr, 
                                            size_t errPtr, 
                                            eReturnType rtype /*= rt_int32*/ )
{
    a.mov( AsmJit::ecx, ResultPtr );

    // Return 64bit value
    if (rtype == rt_int64)
    {
        a.mov( AsmJit::dword_ptr( AsmJit::ecx, 4 ), AsmJit::eax );
        a.mov( AsmJit::dword_ptr( AsmJit::ecx ), AsmJit::edx );
    }
    else if (rtype == rt_int32)
        a.mov( AsmJit::dword_ptr( AsmJit::ecx ), AsmJit::eax );

    // Save last NT status
    SetTebPtr();

    // Save status
    a.add( AsmJit::edx, LAST_STATUS_OFS );
    a.mov( AsmJit::edx, AsmJit::dword_ptr( AsmJit::edx ) );
    a.mov( AsmJit::eax, errPtr );
    a.mov( AsmJit::dword_ptr( AsmJit::eax ), AsmJit::edx );

    // SetEvent(hEvent)
    // NtSetEvent(hEvent, NULL)
    a.push( 0 );
    a.mov( AsmJit::eax, EventPtr );
    a.mov( AsmJit::eax, AsmJit::dword_ptr( AsmJit::eax ) );
    a.push( AsmJit::eax );
    a.mov( AsmJit::eax, pSetEvent );
    a.call( AsmJit::eax );
}

/// <summary>
/// Push function argument
/// </summary>
/// <param name="arg">Argument.</param>
/// <param name="regidx">Push type(register or stack)</param>
void AsmHelper32::PushArg( const AsmVariant& arg, eArgType regidx /*= AT_stack*/ )
{
    switch (arg.type)
    {

    case AsmVariant::imm:
    case AsmVariant::structRet:
        PushArgp( arg.imm_val, regidx );
        break;

    case AsmVariant::dataPtr:
        PushArgp( arg.new_imm_val, regidx );
        break;

        // Copy argument onto stack   
    case AsmVariant::dataStruct:
        {
            // Ensure stack remain aligned on word size
            size_t realSize = (arg.size < WordSize) ? WordSize : arg.size;
            a.sub( AsmJit::esp, realSize );
            a.mov( AsmJit::esi, arg.new_imm_val );
            a.mov( AsmJit::edi, AsmJit::esp);

            // Preserve ecx value, may be __thiscall or __fastcall function
            a.mov( AsmJit::eax, AsmJit::ecx );

            a.mov( AsmJit::ecx, arg.size );
            a.rep_movsb();

            // Restore registers
            a.mov( AsmJit::ecx, AsmJit::eax );
        }
        break;

    case AsmVariant::imm_double:        
        {
            ULARGE_INTEGER li;
            li.QuadPart = arg.getImm_double();

            PushArgp( li.HighPart, regidx );
            PushArgp( li.LowPart, regidx );
        }
        break;

    case AsmVariant::imm_float:
        PushArgp( arg.getImm_float(), regidx );
        break;

    case AsmVariant::mem_ptr:        
        a.lea( AsmJit::eax, arg.mem_val );
        PushArgp( AsmJit::eax, regidx );
        break;

    case AsmVariant::mem:
        PushArgp( arg.mem_val, regidx );
        break;

    case AsmVariant::reg:
        PushArgp( arg.reg_val, regidx );
        break;

    default:
        assert( "Invalid argument type" && false );
        break;
    }
}

/// <summary>
/// Push argument into function
/// </summary>
/// <param name="arg">Argument</param>
/// <param name="index">Argument location</param>
template<typename _Type>
void AsmHelper32::PushArgp( _Type arg, eArgType index )
{
    static const AsmJit::GPReg regs[] = { AsmJit::ecx, AsmJit::edx };

    // for __fastcall and __thiscall
    if (index < at_stack)
        a.mov( regs[index], arg );
    else
        a.push( arg );
}

}