#pragma once

#include "../../Config.h"
#include "RemoteContext.hpp"

#include "../../Include/Winheaders.h"
#include "../../Include/Macro.h"
#include "../../Misc/Utils.h"
#include "../Threads/Threads.h"

#include <map>
#include <set>
#include <stdint.h>

namespace blackbone
{

class RemoteHook
{
public:
    
    // Hook type
    enum eHookType
    {
        int3 = 0,           // Default int 3 breakpoint
        hwbp                // Hardware breakpoint
    };

    enum eHookFlags
    {
        none = 0,           // No flags
        returnHook = 1,     // Hook function return
    };

    // Hook callback prototype
    using fnCallback = void( *)(RemoteContext& context);
    using fnClassCallback = void( __thiscall* )(const void* __this, RemoteContext& context);

    /// <summary>
    /// Hook descriptor
    /// </summary>
    struct HookData
    {
        // Callback pointer
        union callback
        {
            struct _classFn
            {
                fnClassCallback ptr;    // Class member function pointer
                const void* classPtr;   // Class instance
            };

            fnCallback freeFn;          // Free function pointer
            _classFn   classFn;         // Class member pointer
        };

        callback   onExecute;           // Callback called upon address breakpoint
        callback   onReturn;            // Callback called upon function return
        eHookType  type;                // int 3 or HWBP
        eHookFlags flags;               // Some hooking flags
        uint8_t    oldByte;             // Original byte in case of int 3 hook
        DWORD      threadID;            // Thread id for HWBP (0 means global hook for all threads)
        int        hwbp_idx;            // Index of HWBP if applied to one thread only
        _CONTEXT64 entryCtx;            // Thread context on function entry (used in function return hook)
    };

    using mapHook = std::map<ptr_t, HookData>;
    using mapAddress = std::map<ptr_t, ptr_t>;
    using setAddresses = std::map<ptr_t, bool>;

public:
    BLACKBONE_API RemoteHook( class ProcessMemory& memory );
    BLACKBONE_API ~RemoteHook();

    /// <summary>
    /// Hook specified address
    /// </summary>
    /// <param name="type">Hook type</param>
    /// <param name="ptr">Address</param>
    /// <param name="newFn">Callback</param>
    /// <param name="pThread">Thread to hook. Valid only for HWBP</param>
    /// <returns>true on success</returns>
    BLACKBONE_API inline NTSTATUS Apply( eHookType type, uint64_t ptr, fnCallback newFn, ThreadPtr pThread = nullptr )
    {
        return ApplyP( type, ptr, newFn, nullptr, pThread );
    }

    /// <summary>
    /// Hook function return
    /// This hook will only work if function is hooked normally
    /// </summary>
    /// <param name="ptr">Hooked function address</param>
    /// <param name="newFn">Callback</param>
    /// <returns>true on success</returns>
    BLACKBONE_API inline NTSTATUS AddReturnHook( uint64_t ptr, fnCallback newFn )
    {
        return AddReturnHookP( ptr, newFn, nullptr );
    }

    // FIXME: Alternative for MinGW
#ifdef COMPILER_MSVC
    /// <summary>
    /// Hook specified address
    /// </summary>
    /// </summary>
    /// <param name="type">Hook type</param>
    /// <param name="ptr">Address</param>
    /// <param name="newFn">Callback</param>
    /// <param name="classRef">Class reference.</param>
    /// <param name="pThread">Thread to hook. Valid only for HWBP</param>
    /// <returns>true on success</returns>
    template<typename C>
    inline NTSTATUS Apply( eHookType type, uint64_t ptr, void(C::* newFn)(RemoteContext& ctx), const C& classRef, ThreadPtr pThread = nullptr )
    {
        return ApplyP( type, ptr, brutal_cast<fnCallback>(newFn), &classRef, pThread );
    }

    /// <summary>
    /// Hook function return
    /// This hook will only work if function is hooked normally
    /// </summary>
    /// <param name="ptr">Hooked function address</param>
    /// <param name="newFn">Callback</param>
    /// <param name="classRef">Class reference.</param>
    /// <returns>true on success</returns>
    template<typename C>
    inline NTSTATUS AddReturnHook( uint64_t ptr, void(C::* newFn)(RemoteContext& ctx), const C& classRef )
    {
        return AddReturnHookP( ptr, brutal_cast<fnCallback>(newFn), &classRef );
    }
#endif // COMPILER_MSVC

    /// <summary>
    /// Remove existing hook
    /// </summary>
    /// <param name="ptr">Hooked address</param>
    BLACKBONE_API void Remove( uint64_t ptr );

    /// <summary>
    /// Stop debug and remove all hooks
    /// </summary>
    BLACKBONE_API void reset();

private:

    /// <summary>
    /// Hook specified address
    /// </summary>
    /// <param name="type">Hook type</param>
    /// <param name="ptr">Address</param>
    /// <param name="newFn">Callback</param>
    /// <param name="pClass">Class reference.</param>
    /// <param name="pThread">Thread to hook. Valid only for HWBP</param>
    /// <returns>true on success</returns>
    BLACKBONE_API NTSTATUS ApplyP( eHookType type, uint64_t ptr, fnCallback newFn, const void* pClass = nullptr, ThreadPtr pThread = nullptr );

    /// <summary>
    /// Hook function return
    /// This hook will only work if function is hooked normally
    /// </summary>
    /// <param name="ptr">Hooked function address</param>
    /// <param name="newFn">Callback</param>
    /// <param name="pClass">Class reference.</param>
    /// <returns>true on success</returns>
    BLACKBONE_API NTSTATUS AddReturnHookP( uint64_t ptr, fnCallback newFn, const void* pClass = nullptr );

    /// <summary>
    /// Restore hooked function
    /// </summary>
    /// <param name="hook">Hook data</param>
    /// <param name="ptr">Hooked address</param>
    BLACKBONE_API void Restore( const HookData &hook, uint64_t ptr );

    /// <summary>
    /// Debug selected process
    /// </summary>
    /// <returns>true on success</returns>
    NTSTATUS EnsureDebug();

    /// <summary>
    /// Stop process debug
    /// </summary>
    void EndDebug();

    /// <summary>
    /// Wrapper for debug event thread
    /// </summary>
    /// <param name="lpParam">RemoteHook pointer</param>
    /// <returns>Error code</returns>
    static DWORD __stdcall EventThreadWrap( LPVOID lpParam );

    /// <summary>
    /// Debug thread
    /// </summary>
    /// <returns>Error code</returns>
    DWORD EventThread();

    /// <summary>
    /// Debug event handler
    /// </summary>
    /// <param name="DebugEv">Debug event data</param>
    /// <returns>Status</returns>
    DWORD OnDebugEvent( const DEBUG_EVENT& DebugEv );

    /// <summary>
    /// Int 3 handler
    /// </summary>
    /// <param name="DebugEv">Debug event data</param>
    /// <returns>Status</returns>
    DWORD OnBreakpoint( const DEBUG_EVENT& DebugEv );

    /// <summary>
    /// Trace and hardware breakpoints handler
    /// </summary>
    /// <param name="DebugEv">Debug event data</param>
    /// <returns>Status</returns>
    DWORD OnSinglestep( const DEBUG_EVENT& DebugEv );

    /// <summary>
    /// Access violation handler
    /// Used when hooking function return
    /// </summary>
    /// <param name="DebugEv">Debug event data</param>
    /// <returns>Status</returns>
    DWORD OnAccessViolation( const DEBUG_EVENT& DebugEv );

    /// <summary>
    /// Walk stack frames
    /// </summary>
    /// <param name="ip">Thread instruction pointer</param>
    /// <param name="sp">>Thread stack pointer</param>
    /// <param name="thd">Stack owner</param>
    /// <param name="results">Stack frames</param>
    /// <param name="depth">Max frame count</param>
    /// <returns>Frame count</returns>
    DWORD StackBacktrace( ptr_t ip, ptr_t sp, Thread& thd, std::vector<std::pair<ptr_t, ptr_t>>& results, int depth = 100 );

    RemoteHook( const RemoteHook& ) = delete;
    RemoteHook& operator =( const RemoteHook& ) = delete;

private:
    class ProcessMemory& _memory;
    class ProcessCore&   _core;
    CriticalSection _lock;              // Hook lock

    DWORD        _debugPID = 0;         // PID of process being debugged
    HANDLE       _hEventThd = NULL;     // Debug Event thread
    BOOL         _x64Target = FALSE;    // Target is x64 process
    int          _wordSize = 4;         // 4 or 8 bytes
    bool         _active = false;       // Event thread activity flag
    mapHook      _hooks;                // Hooked callbacks
    setAddresses _repatch;              // Pending repatch addresses
    mapAddress   _retHooks;             // Hooked return addresses
};

ENUM_OPS( RemoteHook::eHookFlags )

}