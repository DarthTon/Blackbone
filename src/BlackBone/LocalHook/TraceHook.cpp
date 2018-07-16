#include "../Config.h"
#include "TraceHook.h"
#include "../Include/Macro.h"

#define BEA_USE_STDCALL
//#define BEA_ENGINE_STATIC
#include <3rd_party/BeaEngine/headers/BeaEngine.h>

#include <algorithm>

#define HIGHEST_BIT_SET     (1LL << (sizeof(void*) * 8 - 1))
#define HIGHEST_BIT_UNSET  ~HIGHEST_BIT_SET
#define SingleStep          0x100

// Architecture-specific
#ifdef USE64
#define ADDR_MASK 0xFFFFFFFFFFFFF000
#else
#define ADDR_MASK 0xFFFFF000
#endif

namespace blackbone
{

TraceHook::TraceHook()
{
}

TraceHook::~TraceHook()
{
    if (_pExptHandler != nullptr)
        RemoveVectoredExceptionHandler( _pExptHandler );
}

TraceHook& TraceHook::Instance()
{
    static TraceHook instance;  
    return instance;
}

/// <summary>
/// Setup hook
/// </summary>
/// <param name="targetFunc">Target function to be hooked</param>
/// <param name="hookFunc">New function</param>
/// <param name="ptrAddress">Address of pointer to destroy</param>
/// <param name="tracePath">Function tracing path</param>
/// <param name="checkIP">Optional. Address of instruction that checks target pointer</param>
/// <returns>true on success, false if already hooked</returns>
bool TraceHook::ApplyHook( void* targetFunc, 
                           void* hookFunc, 
                           void* ptrAddress, 
                           const HookContext::vecState& tracePath /*= HookContext::vecState()*/,
                           void* chekcIP /*= 0*/)
{
    // Pointer already present
    if (_contexts.count( (uintptr_t)ptrAddress ))
    {
        HookContext& ctx = _contexts[(uintptr_t)ptrAddress];

        // Already hooked
        if (ctx.hooks.count( (uintptr_t)targetFunc ))
            return false;
        else
            ctx.hooks.emplace( (uintptr_t)targetFunc, std::make_pair( (uintptr_t)hookFunc, false ) );
    }
    // Create new context
    else
    {
        HookContext ctx;

        // Setup context
        ctx.targetPtr = (uintptr_t)ptrAddress;
        ctx.checkIP = (uintptr_t)chekcIP;
        ctx.origPtrVal = *(uintptr_t*)ptrAddress;
        ctx.breakValue = _breakPtr;
        ctx.tracePath = tracePath;

        ctx.hooks.emplace( (uintptr_t)targetFunc, std::make_pair( (uintptr_t)hookFunc, false ) );
        _contexts.emplace( (uintptr_t)ptrAddress, std::move( ctx ) );

        if (_pExptHandler == nullptr)
            _pExptHandler = AddVectoredExceptionHandler( 0, &TraceHook::VecHandler );

        // Setup exception
        *(uintptr_t*)ptrAddress = ctx.breakValue;
        _breakPtr += 0x10;
    }

    return true;
}

/// <summary>
/// Remove existing hook
/// </summary>
/// <param name="targetFunc">Target function ptr</param>
/// <returns>true on success, false if not found</returns>
bool TraceHook::RemoveHook( void* targetFunc )
{
    auto findfn = [targetFunc]( const mapContext::value_type& val ) { 
        return val.second.hooks.count( (uintptr_t)targetFunc );
    };

    // Get context containing target function
    auto iter = std::find_if( _contexts.begin(), _contexts.end(), findfn );
                              
    if (iter != _contexts.end())
    {
        auto& ctx = iter->second;

        // Remove function from list
        ctx.hooks.erase( (uintptr_t)targetFunc );

        if (ctx.hooks.empty())
        {
            // Remove hook itself
            *(uintptr_t*)ctx.targetPtr = ctx.origPtrVal;
            _contexts.erase( iter );
        }

        // Before handler is removed we need to make sure no thread will 
        // generate exception again
        Sleep( 10 );

        // Remove exception handler
        if (_contexts.empty() && _pExptHandler != nullptr)
        {
            RemoveVectoredExceptionHandler( _pExptHandler );
            _pExptHandler = nullptr;
        }

        return true;
    }

    return false;
}


/// <summary>
/// Exception handler
/// </summary>
/// <param name="ExceptionInfo">The exception information</param>
/// <returns>Handling status</returns>
LONG __stdcall TraceHook::VecHandler( PEXCEPTION_POINTERS ExceptionInfo )
{
    return Instance().VecHandlerP( ExceptionInfo );
}

/// <summary>
/// Exception handler
/// </summary>
/// <param name="ExceptionInfo">The exception information</param>
/// <returns>Handling status</returns>
LONG TraceHook::VecHandlerP( PEXCEPTION_POINTERS ExceptionInfo )
{
    auto exptContex = ExceptionInfo->ContextRecord;
    auto exptRecord = ExceptionInfo->ExceptionRecord;
    auto exptCode   = exptRecord->ExceptionCode;

    // TODO: Somehow determine current hook context
    HookContext* ctx = &_contexts.begin()->second;

    // Check if exception should be handled
    if (exptCode != EXCEPTION_SINGLE_STEP && exptCode != EXCEPTION_ACCESS_VIOLATION)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else if (exptCode == EXCEPTION_ACCESS_VIOLATION && (ctx->state == TS_Step || ctx->state == TS_StepInto))
    {
        if ((exptRecord->ExceptionInformation[1] & ADDR_MASK) != (ctx->breakValue & ADDR_MASK))
            return EXCEPTION_CONTINUE_SEARCH;

        // Pointer accessed by non-target address 
        if (ctx->checkIP != 0 && exptContex->NIP != ctx->checkIP)
        {
            exptContex->EFlags |= SingleStep;

            RestorePtr( *ctx, ExceptionInfo );
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    
    switch (ctx->state)
    {
        // Start of tracing
        case TS_Start:
            {
                ctx->state = ctx->tracePath[ctx->stateIdx].action;

                RestorePtr( *ctx, ExceptionInfo );
                return VecHandlerP( ExceptionInfo );
            }
            break;

        // Single step
        case TS_Step:
            {
                // Function call occurred. Queue break on return.
                if (CheckBranching( *ctx, exptContex->NIP, exptContex->NSP ))
                {
                    // Target function reached
                    if (ctx->hooks.count( exptContex->NIP ))
                    {
                        HandleBranch( *ctx, exptContex );
                        return EXCEPTION_CONTINUE_EXECUTION;
                    }
                    else
                    {
                        ctx->state = TS_WaitReturn;
                        BreakOnReturn( exptContex->NSP );
                    }
                }
                // Step further
                else
                    exptContex->EFlags |= SingleStep;
            }
            break;

        // Step out from function
        case TS_StepOut:
            {
                // Get current stack frame
                vecStackFrames frames;
                StackBacktrace( exptContex->NIP, exptContex->NSP, frames, 1 );

                if (frames.size() > 1)
                {
                    ctx->stateIdx++;
                    ctx->state = TS_WaitReturn;
                    BreakOnReturn( frames.back().first );
                }
            }
            break;

        // Step into specific function
        case TS_StepInto:
            {
                // Check if step into path function has occurred
                if (CheckBranching( *ctx, exptContex->NIP, exptContex->NSP ))
                {
                    if (exptContex->NIP == ctx->tracePath[ctx->stateIdx].arg)
                    {
                        ctx->stateIdx++;
                        ctx->state = ctx->tracePath[ctx->stateIdx].action;
                    }
                }

                exptContex->EFlags |= SingleStep;
            }
            break;

        // Break on 'ret' instruction
        case TS_WaitReturn:
            {
                exptContex->NIP &= HIGHEST_BIT_UNSET;

                // Restore stack for x64
            #ifdef USE64
                *(uintptr_t*)exptContex->NSP &= HIGHEST_BIT_UNSET;
            #endif // USE64

                // Continue stepping
                exptContex->EFlags |= SingleStep;
                ctx->state = ctx->tracePath[ctx->stateIdx].action;
            }
            break;

        default:
            break;
    }

    ctx->lastIP = exptContex->NIP;
    ctx->lastSP = exptContex->NSP;
    
    return EXCEPTION_CONTINUE_EXECUTION;
}


/// <summary>
/// Check if last instruction caused branching
/// </summary>
/// <param name="ctx">Current hook info</param>
/// <param name="ip">Instruction pointer</param>
/// <param name="sp">Stack pointer</param>
/// <returns>True if branching has occurred</returns>
bool TraceHook::CheckBranching( const HookContext& ctx, uintptr_t ip, uintptr_t sp )
{
    // Not yet initialized
    if (ctx.lastIP == 0 || ctx.lastSP == 0)
        return false;

    // Difference in instruction pointer more than possible 'call' length
    // Stack pointer changed
    if (ip - ctx.lastIP >= 8 && sp != ctx.lastSP)
    {
        DISASM info = { 0 };
        info.EIP = ctx.lastIP;

    #ifdef USE64
        info.Archi = 64;
    #endif  

        // FIXME: Alternateve for MinGW
#ifdef COMPILER_MSVC
        // Double-check call instruction using disasm
        if (Disasm( &info ) > 0 && info.Instruction.BranchType == CallType)
            return true;
#endif //COMPILER_MSVC
    }

    return false;
}

/// <summary>
/// Handle branching
/// </summary>
/// <param name="ctx">Current hook context</param>
/// <param name="exptContex">Thread context</param>
void TraceHook::HandleBranch( HookContext& ctx, PCONTEXT exptContex )
{
    // Mark this hook as called
    ctx.hooks[exptContex->NIP].second = true;

    // Reset tracing state if all hooks were called
    auto iter = std::find_if( ctx.hooks.begin(), ctx.hooks.end(),
                              []( const decltype(ctx.hooks)::value_type& val ){ return (val.second.second == false); } );

    // Break after hook execution
    if (iter != ctx.hooks.end())
    {
        ctx.state = TS_WaitReturn;
        BreakOnReturn( exptContex->NSP );
    }
    // Reset state
    else
        ctx.reset();

    // Reroute to hook function
    exptContex->NIP = ctx.hooks[exptContex->NIP].first;
}


/// <summary>
/// Setup exception upon function return
/// </summary>
/// <param name="ExceptionInfo">The exception information</param>
inline void TraceHook::BreakOnReturn( uintptr_t sp )
{
    *(DWORD_PTR*)sp |= HIGHEST_BIT_SET;
}

/// <summary>
/// Restore original pointer value
/// </summary>
/// <param name="ctx">The CTX.</param>
/// <param name="ExceptionInfo">The exception information</param>
/// <returns>true on success, false if no invalid register was found</returns>
bool TraceHook::RestorePtr( const HookContext& ctx, PEXCEPTION_POINTERS ExceptionInfo )
{
    bool found = false;
    auto expCtx = ExceptionInfo->ContextRecord;

    // Exception on execute
    if (ExceptionInfo->ExceptionRecord->ExceptionInformation[0] == 8)
    {
        expCtx->NIP = ctx.origPtrVal;
        return true;
    }

    // Exception on read/write
#ifdef USE64
    for (DWORD_PTR* pRegVal = &expCtx->Rax; pRegVal <= &expCtx->R15; pRegVal++)
#else
    for (DWORD_PTR* pRegVal = &expCtx->NDI; pRegVal <= &expCtx->NAX; pRegVal++)  
#endif
    {
        // Compare high address parts
        if ((*pRegVal & ADDR_MASK) == (ExceptionInfo->ExceptionRecord->ExceptionInformation[1] & ADDR_MASK))
        {
            *pRegVal = ctx.origPtrVal;
            found = true;
        }
    }

    return found;
}

/// <summary>
/// Capture stack frames
/// </summary>
/// <param name="ip">Current instruction pointer</param>
/// <param name="sp">Current stack pointer</param>
/// <param name="results">Found frames.</param>
/// <param name="depth">Frame depth limit</param>
/// <returns>Number of found frames</returns>
size_t TraceHook::StackBacktrace( uintptr_t ip, uintptr_t sp, vecStackFrames& results, uintptr_t depth /*= 10 */ )
{
    SYSTEM_INFO sysinfo = {};
    uintptr_t stack_base = (uintptr_t)((PNT_TIB)NtCurrentTeb())->StackBase;

    GetNativeSystemInfo( &sysinfo );

    // Store exception address
    results.emplace_back( 0, ip );

    // Walk stack
    for (uintptr_t stackPtr = sp; stackPtr < stack_base && results.size() <= depth; stackPtr += sizeof(void*))
    {
        uintptr_t stack_val = *(uintptr_t*)stackPtr;
        MEMORY_BASIC_INFORMATION meminfo = { 0 };

        // Decode value
        uintptr_t original = stack_val & HIGHEST_BIT_UNSET;

        // Invalid value
        if ( original < (uintptr_t)sysinfo.lpMinimumApplicationAddress ||
             original > (uintptr_t)sysinfo.lpMaximumApplicationAddress)
        {
            continue;
        }

        // Check if memory is executable
        if (VirtualQuery( (LPVOID)original, &meminfo, sizeof(meminfo) ) != sizeof(meminfo))
            continue;

        if ( meminfo.Protect != PAGE_EXECUTE_READ &&
             meminfo.Protect != PAGE_EXECUTE_WRITECOPY &&
             meminfo.Protect != PAGE_EXECUTE_READWRITE)
        {
            continue;
        }

        // Detect 'call' instruction
        for (uintptr_t j = 1; j < 8; j++)
        {
            DISASM info = { 0 };
            info.EIP = original - j;

        #ifdef USE64
            info.Archi = 64;
        #endif  

            // FIXME: Alternative for MinGW
#ifdef COMPILER_MSVC
            if (Disasm( &info ) > 0 && info.Instruction.BranchType == CallType)
            {
                results.emplace_back( stackPtr, stack_val );
                break;
            }
#endif // COMPILER_MSVC
        }

    }

    return results.size();
}

}