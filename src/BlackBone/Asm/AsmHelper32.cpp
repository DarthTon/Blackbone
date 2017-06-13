#include "AsmHelper32.h"

#include <set>
#include <assert.h>

namespace blackbone
{

AsmHelper32::AsmHelper32( )
    : IAsmHelper( asmjit::kArchX86 )
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
        IAsmHelper::SwitchTo64();
    }
    else
    {
        _assembler.push( asmjit::host::ebp );
        _assembler.mov( asmjit::host::ebp, asmjit::host::esp );
    }
}

/// <summary>
///  Generate function epilogue code
/// </summary>
/// <param name="switchMode">Unused</param>
/// <param name="retSize">Stack change value</param>
void AsmHelper32::GenEpilogue( bool switchMode /*= false*/ , int retSize /*= -1 */ )
{
    if (switchMode == true)
    {
        IAsmHelper::SwitchTo86();
    }
    else
    {
        _assembler.mov( asmjit::host::esp, asmjit::host::ebp );
        _assembler.pop( asmjit::host::ebp );
    }

    if (retSize == -1)
        _assembler.ret();
    else
        _assembler.ret( retSize );
}

/// <summary>
/// Generate function call
/// </summary>
/// <param name="pFN">Function pointer</param>
/// <param name="args">Function arguments</param>
/// <param name="cc">Calling convention</param>
void AsmHelper32::GenCall( const AsmFunctionPtr& pFN, const std::vector<AsmVariant>& args, eCalligConvention cc /*= CC_stdcall*/ )
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
        assert( pFN.imm_val64 <= std::numeric_limits<uint32_t>::max() );
        _assembler.mov( asmjit::host::eax, pFN.imm_val );
        _assembler.call( asmjit::host::eax );
    }
    // Already in register
    else if (pFN.type == AsmVariant::reg)
    {
        _assembler.call( pFN.reg_val );
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
                argsize += sizeof( uint32_t );
        }

        _assembler.add( asmjit::host::esp, argsize );
    }
      
}

/// <summary>
/// Save eax value and terminate current thread
/// </summary>
/// <param name="pExitThread">NtTerminateThread address</param>
/// <param name="resultPtr">Memry where eax value will be saved</param>
void AsmHelper32::ExitThreadWithStatus( uint64_t pExitThread, uint64_t resultPtr )
{
    if (resultPtr != 0)
    {
        _assembler.mov( asmjit::host::edx, resultPtr );
        _assembler.mov( asmjit::host::dword_ptr( asmjit::host::edx ), asmjit::host::eax );
    }

    // NtTerminateThread( NULL, eax );
    _assembler.push( asmjit::host::eax );
    _assembler.push( 0 );
    _assembler.mov( asmjit::host::eax, pExitThread );
    _assembler.call( asmjit::host::eax );
    
    _assembler.ret();
}

/// <summary>
/// Save return value and signal thread return event
/// </summary>
/// <param name="pSetEvent">NtSetEvent address</param>
/// <param name="ResultPtr">Result value memory location</param>
/// <param name="EventPtr">Event memory location</param>
/// <param name="errPtr">Error code memory location</param>
/// <param name="rtype">Return type</param>
void AsmHelper32::SaveRetValAndSignalEvent( 
    uint64_t pSetEvent,
    uint64_t ResultPtr,
    uint64_t EventPtr,
    uint64_t errPtr,
    eReturnType rtype /*= rt_int32*/ 
    )
{
    _assembler.mov( asmjit::host::ecx, ResultPtr );

    // Return 64bit value
    if (rtype == rt_int64)
    {
        _assembler.mov( asmjit::host::dword_ptr( asmjit::host::ecx, 4 ), asmjit::host::eax );
        _assembler.mov( asmjit::host::dword_ptr( asmjit::host::ecx ), asmjit::host::edx );
    }
    else if (rtype == rt_int32)
        _assembler.mov( asmjit::host::dword_ptr( asmjit::host::ecx ), asmjit::host::eax );

    // Save last NT status
    _assembler.mov( asmjit::host::edx, asmjit::host::dword_ptr_abs( 0x18 ).setSegment( asmjit::host::fs ) );
    _assembler.add( asmjit::host::edx, 0x598 + 0x197 * sizeof( uint32_t ) );
    _assembler.mov( asmjit::host::edx, asmjit::host::dword_ptr( asmjit::host::edx ) );
    _assembler.mov( asmjit::host::eax, errPtr );
    _assembler.mov( asmjit::host::dword_ptr( asmjit::host::eax ), asmjit::host::edx );

    // SetEvent(hEvent)
    // NtSetEvent(hEvent, NULL)
    _assembler.push( 0 );
    _assembler.mov( asmjit::host::eax, EventPtr );
    _assembler.mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::eax ) );
    _assembler.push( asmjit::host::eax );
    _assembler.mov( asmjit::host::eax, pSetEvent );
    _assembler.call( asmjit::host::eax );
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
        // TODO: resolve 64bit imm values instead of ignoring high bits
        PushArgp( arg.imm_val, regidx );
        break;

    case AsmVariant::dataPtr:
        PushArgp( arg.new_imm_val, regidx );
        break;

        // Copy argument onto stack   
    case AsmVariant::dataStruct:
        {
            // Ensure stack remain aligned on word size
            size_t realSize = Align( arg.size, sizeof( uint32_t ) );
            _assembler.sub( asmjit::host::esp, realSize );
            _assembler.mov( asmjit::host::esi, arg.new_imm_val );
            _assembler.mov( asmjit::host::edi, asmjit::host::esp);

            // Preserve ecx value, may be __thiscall or __fastcall function
            _assembler.mov( asmjit::host::eax, asmjit::host::ecx );

            _assembler.mov( asmjit::host::ecx, arg.size );
            _assembler.rep_movsb();

            // Restore registers
            _assembler.mov( asmjit::host::ecx, asmjit::host::eax );
        }
        break;

    case AsmVariant::imm_double:        
        {
            ULARGE_INTEGER li;
            li.QuadPart = arg.getImm_double();

            PushArgp( static_cast<uint32_t>(li.HighPart), regidx );
            PushArgp( static_cast<uint32_t>(li.LowPart), regidx );
        }
        break;

    case AsmVariant::imm_float:
        PushArgp( arg.getImm_float(), regidx );
        break;

    case AsmVariant::mem_ptr:        
        _assembler.lea( asmjit::host::eax, arg.mem_val );
        PushArgp( asmjit::host::eax, regidx );
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
    static const asmjit::GpReg regs[] = { asmjit::host::ecx, asmjit::host::edx };

    // for __fastcall and __thiscall
    if (index < at_stack)
        _assembler.mov( regs[index], arg );
    else
        _assembler.push( arg );
}

}