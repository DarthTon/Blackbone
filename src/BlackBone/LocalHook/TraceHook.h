#pragma once

#include "../Include/WinHeaders.h"

#include <stdint.h>
#include <vector>
#include <map>
#include <unordered_map>

namespace blackbone
{

enum TraceState
{
    TS_Start,       // Initial state. Internal use only
    TS_Step,        // Do single-step
    TS_StepOut,     // Break on function return
    TS_StepInto,    // Step into specific function
    TS_WaitReturn,  // Wait for break-on-return
};

struct PathNode
{
    TraceState action;
    uintptr_t arg;

    PathNode( TraceState  _action, uintptr_t _arg = 0 )
        : action( _action )
        , arg( _arg ) { }
};


/// <summary>
/// Hook-related data
/// </summary>
struct HookContext
{
    using mapHooks = std::unordered_map<uintptr_t, std::pair<uintptr_t, bool>>;
    using vecState = std::vector<PathNode>;

    uintptr_t lastIP = 0;       // Previous EIP/RIP value
    uintptr_t lastSP = 0;       // Previous ESP/RSP value
    uintptr_t targetPtr = 0;    // Address causing exception
    uintptr_t origPtrVal = 0;   // Original pointer value
    uintptr_t checkIP = 0;      // Address of instruction that checks target pointer
    uintptr_t breakValue = 0;   // Value used to generate exception
    uintptr_t stateIdx = 0;     // Current state index in state vector

    TraceState state = TS_Start;    // Current tracing state
    vecState tracePath;             // Function trace path
    mapHooks hooks;                 // List of hooks associated with current pointer


    /// <summary>
    /// Reset tracing state
    /// </summary>
    void reset()
    {
        state = TS_Start;
        lastIP = lastSP = 0;
        stateIdx = 0;

        // Mark hooks as non-called
        for (auto& item : hooks)
            item.second.second = false;
    }
};

class TraceHook
{
public:
    using mapContext = std::map<uintptr_t, HookContext>;
    using vecStackFrames = std::vector <std::pair<uintptr_t, uintptr_t>>;

public:
    ~TraceHook();
    BLACKBONE_API static TraceHook& Instance();

    /// <summary>
    /// Setup hook
    /// </summary>
    /// <param name="targetFunc">Target function to be hooked</param>
    /// <param name="hookFunc">New function</param>
    /// <param name="ptrAddress">Address of pointer to destroy</param>
    /// <param name="tracePath">Function tracing path</param>
    /// <param name="checkIP">Optional. Address of instruction that checks target pointer</param>
    /// <returns>true on success, false if already hooked</returns>
    BLACKBONE_API bool ApplyHook( void* targetFunc,
                                  void* hookFunc,
                                  void* ptrAddress,
                                  const HookContext::vecState& tracePath = HookContext::vecState(),
                                  void* checkIP = 0 );

    /// <summary>
    /// Remove existing hook
    /// </summary>
    /// <param name="targetFunc">Target function ptr</param>
    /// <returns>true on success, false if not found</returns>
    BLACKBONE_API bool RemoveHook( void* targetFunc );

private:
    // 
    // Singleton
    //
    TraceHook();
    TraceHook( const TraceHook& ) = delete;
    TraceHook& operator =( const TraceHook& ) = delete;

    //
    // Exception handlers
    //
    static LONG __stdcall VecHandler( PEXCEPTION_POINTERS ExceptionInfo );
    LONG VecHandlerP( PEXCEPTION_POINTERS ExceptionInfo );

    /// <summary>
    /// Capture stack frames
    /// </summary>
    /// <param name="ip">Current instruction pointer</param>
    /// <param name="sp">Current stack pointer</param>
    /// <param name="results">Found frames.</param>
    /// <param name="depth">Frame depth limit</param>
    /// <returns>Number of found frames</returns>
    size_t StackBacktrace( uintptr_t ip, uintptr_t sp, vecStackFrames& results, uintptr_t depth = 10 );

    /// <summary>
    /// Setup exception upon function return
    /// </summary>
    /// <param name="ExceptionInfo">The exception information</param>
    inline void BreakOnReturn( uintptr_t sp );

    /// <summary>
    /// Check if last instruction caused branching
    /// </summary>
    /// <param name="ctx">Current hook info</param>
    /// <param name="ip">Instruction pointer</param>
    /// <param name="sp">Stack pointer</param>
    /// <returns>True if branching has occurred</returns>
    bool CheckBranching( const HookContext& ctx, uintptr_t ip, uintptr_t sp );

    /// <summary>
    /// Handle branching
    /// </summary>
    /// <param name="ctx">Current hook context</param>
    /// <param name="exptContex">Thread context</param>
    void HandleBranch( HookContext& ctx, PCONTEXT exptContex );

    /// <summary>
    /// Restore original pointer value
    /// </summary>
    /// <param name="ctx">The CTX.</param>
    /// <param name="ExceptionInfo">The exception information</param>
    /// <returns>true on success, false if no invalid register was found</returns>
    bool RestorePtr( const HookContext& ctx, PEXCEPTION_POINTERS ExceptionInfo );

private:
    PVOID       _pExptHandler = nullptr;        // Exception handler
    mapContext  _contexts;                      // Hook contexts
    uintptr_t   _breakPtr = 0x2000;             // Exception pointer generator
};

}