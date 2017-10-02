#pragma once

#include "../../Config.h"
#include "../../Include/Winheaders.h"
#include "../../Include/NativeStructures.h"
#include "../../Include/CallResult.h"
#include "../../Include/HandleGuard.h"
#include "../../Include/Types.h"
#include "../../Misc/Utils.h"

#include <memory>

namespace blackbone
{

#define DEFAULT_ACCESS_T  THREAD_SUSPEND_RESUME    | \
                          THREAD_GET_CONTEXT       | \
                          THREAD_SET_CONTEXT       | \
                          THREAD_QUERY_INFORMATION | \
                          THREAD_TERMINATE         | \
                          SYNCHRONIZE

#define MAXULONG64_2 ((uint64_t)~((uint64_t)0))
#define MAXULONG32_2 ((uint32_t)~((uint32_t)0))


// Breakpoint type
enum HWBPType
{
    hwbp_access  = 3,   // Read or write
    hwbp_write   = 1,   // Write only
    hwbp_execute = 0,   // Execute only
};

enum HWBPLength
{
    hwbp_1 = 0,         // 1 byte
    hwbp_2 = 1,         // 2 bytes
    hwbp_4 = 3,         // 4 bytes
    hwbp_8 = 2,         // 8 bytes
};


/// <summary>
/// DR7 register bitfield
/// </summary>
struct regDR7
{
    //
    // Local/global enabled index
    //
    uint32_t l0 : 1;
    uint32_t g0 : 1;
    uint32_t l1 : 1;
    uint32_t g1 : 1;
    uint32_t l2 : 1;
    uint32_t g2 : 1;
    uint32_t l3 : 1;
    uint32_t g3 : 1;

    uint32_t l_enable : 1;       // Local breakpoints, obsolete for P6+
    uint32_t g_enable : 1;       // Global breakpoints, obsolete for P6+
    uint32_t one : 1;            // Reserved
    uint32_t rtm : 1;            // Restricted transactional memory
    uint32_t ice : 1;            // ICE debugger
    uint32_t gd  : 1;            // General detect table
    uint32_t tr1 : 1;            // Trace 1
    uint32_t tr2 : 1;            // Trace2

    //
    // Breakpoint type/length
    //
    uint32_t rw0  : 2;
    uint32_t len0 : 2;
    uint32_t rw1  : 2;
    uint32_t len1 : 2;
    uint32_t rw2  : 2;
    uint32_t len2 : 2;
    uint32_t rw3  : 2;
    uint32_t len3 : 2;

    inline void setLocal( int idx, int val ) { idx == 0 ? l0   = val : (idx == 1 ? l1   = val : (idx == 2 ? l2   = val : l3   = val)); }
    inline void setRW   ( int idx, int val ) { idx == 0 ? rw0  = val : (idx == 1 ? rw1  = val : (idx == 2 ? rw2  = val : rw3  = val)); }
    inline void setLen  ( int idx, int val ) { idx == 0 ? len0 = val : (idx == 1 ? len1 = val : (idx == 2 ? len2 = val : len3 = val)); }

    inline bool empty() const { return (l0 | l1 | l2 | l3) ? false : true; }
    inline int  getFreeIndex() const { return !l0 ? 0 : (!l1 ? 1 : (!l2 ? 2 : (!l3 ? 3 : -1))); }
};


/// <summary>
/// Thread management
/// </summary>
class Thread
{
public:
    BLACKBONE_API Thread( DWORD id, class ProcessCore* hProcess, DWORD access = DEFAULT_ACCESS_T );
    BLACKBONE_API Thread( HANDLE handle, class ProcessCore* hProcess );
    BLACKBONE_API ~Thread();

    BLACKBONE_API Thread( const Thread& ) = delete;
    BLACKBONE_API Thread& operator =( const Thread& ) = delete;

    BLACKBONE_API Thread( Thread&& ) = default;
    BLACKBONE_API Thread& operator =( Thread&& ) = default;

    /// <summary>
    /// Get thread ID
    /// </summary>
    /// <returns>Thread ID</returns>
    BLACKBONE_API inline DWORD id() const { return _id; }

    /// <summary>
    /// Get thread handle
    /// </summary>
    /// <returns>Thread hande</returns>
    BLACKBONE_API inline HANDLE handle() const { return _handle; }

    /// <summary>
    /// Check if thread exists
    /// </summary>
    /// <returns>true if thread exists</returns>
    BLACKBONE_API inline bool valid() const { return (_handle && ExitCode() == STILL_ACTIVE); }

    /// <summary>
    /// Get WOW64 TEB
    /// </summary>
    /// <param name="pteb">Process TEB</param>
    /// <returns>TEB pointer</returns>
    BLACKBONE_API ptr_t teb( _TEB32* pteb ) const;
   
    /// <summary>
    /// Get Native TEB
    /// </summary>
    /// <param name="pteb">Process TEB</param>
    /// <returns>TEB pointer</returns>
    BLACKBONE_API ptr_t teb( _TEB64* pteb ) const;

    /// <summary>
    /// Get TEB
    /// </summary>
    /// <returns>TEB pointer</returns>
    BLACKBONE_API inline ptr_t teb() const { return teb( (TEB_T*)nullptr ); }

    /// <summary>
    /// Get thread creation time
    /// </summary>
    /// <returns>Thread creation time</returns>
    BLACKBONE_API uint64_t startTime();

    /// <summary>
    /// Get total execution time(user mode and kernel mode)
    /// </summary>
    /// <returns>Total execution time</returns>
    BLACKBONE_API uint64_t execTime();

    /// <summary>
    /// Suspend thread
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Suspend();

    /// <summary>
    /// Resumes thread.
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Resume();

    /// <summary>
    /// Check if thread is suspended
    /// </summary>
    /// <returns>true if suspended</returns>
    BLACKBONE_API bool Suspended();

    /// <summary>
    /// Get WOW64 thread context
    /// </summary>
    /// <param name="ctx">Returned context</param>
    /// <param name="flags">Context flags.</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS GetContext( _CONTEXT32& ctx, DWORD flags = CONTEXT_ALL, bool dontSuspend = false );

    /// <summary>
    /// Get native thread context
    /// </summary>
    /// <param name="ctx">Returned context</param>
    /// <param name="flags">Context flags.</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS GetContext( _CONTEXT64& ctx, DWORD flags = CONTEXT64_ALL, bool dontSuspend = false );

    /// <summary>
    /// Set WOW64 thread context
    /// </summary>
    /// <param name="ctx">Context to set</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS SetContext( _CONTEXT32& ctx, bool dontSuspend = false );

    /// <summary>
    /// Set native thread context
    /// </summary>
    /// <param name="ctx">Context to set</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS SetContext( _CONTEXT64& ctx, bool dontSuspend = false );

    /// <summary>
    /// Terminate thread
    /// </summary>
    /// <param name="code">Exit code</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Terminate( DWORD code = 0 );

    /// <summary>
    /// Join thread
    /// </summary>
    /// <param name="timeout">Join timeout</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Join( int timeout = INFINITE );

    /// <summary>
    /// Get thread exit code
    /// </summary>
    /// <returns>Thread exit code</returns>
    BLACKBONE_API DWORD ExitCode() const;

    /// <summary>
    /// Add hardware breakpoint to thread
    /// </summary>
    /// <param name="addr">Breakpoint address</param>
    /// <param name="type">Breakpoint type(read/write/execute)</param>
    /// <param name="length">Number of bytes to include into breakpoint</param>
    /// <returns>Index of used breakpoint; -1 if failed</returns>
    BLACKBONE_API call_result_t<int> AddHWBP( ptr_t addr, HWBPType type, HWBPLength length );

    /// <summary>
    /// Remove existing hardware breakpoint
    /// </summary>
    /// <param name="idx">Breakpoint index</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS RemoveHWBP( int idx );

    /// <summary>
    /// Remove existing hardware breakpoint
    /// </summary>
    /// <param name="ptr">Breakpoint address</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS RemoveHWBP( ptr_t ptr );

    /// <summary>
    /// Close handle
    /// </summary>
    BLACKBONE_API void Close();

    BLACKBONE_API inline bool operator ==( const Thread& other ) { return (_id == other._id); }

private:
    /// <summary>
    /// GetThreadId support for XP
    /// </summary>
    /// <param name="hThread">Thread handle</param>
    /// <returns>Thread ID</returns>
    DWORD GetThreadIdT( HANDLE hThread );

private:
    class ProcessCore* _core;       // Core routines

    DWORD _id = 0;                  // Thread ID
    Handle _handle;                 // Thread handle
};

using ThreadPtr = std::shared_ptr<Thread>;

}