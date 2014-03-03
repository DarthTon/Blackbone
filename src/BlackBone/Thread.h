#pragma once

#include "Winheaders.h"
#include "NativeStructures.h"
#include "Types.h"

#include <memory>

namespace blackbone
{

#define DEFAULT_ACCESS_T  THREAD_SUSPEND_RESUME    | \
                          THREAD_GET_CONTEXT       | \
                          THREAD_SET_CONTEXT       | \
                          THREAD_QUERY_INFORMATION | \
                          THREAD_TERMINATE         | \
                          SYNCHRONIZE

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
/// Thread management
/// </summary>
class Thread
{
public:
    Thread( DWORD id, class ProcessCore* hProcess, DWORD access = DEFAULT_ACCESS_T );
    Thread( HANDLE handle, class ProcessCore* hProcess );
    Thread( const Thread& other );
    ~Thread();

    /// <summary>
    /// Get thread ID
    /// </summary>
    /// <returns>Thread ID</returns>
    inline DWORD  id()     const { return _id; }

    /// <summary>
    /// Get thread handle
    /// </summary>
    /// <returns>Thread hande</returns>
    inline HANDLE handle() const { return _handle; }

    /// <summary>
    /// Check if thread exists
    /// </summary>
    /// <returns>true if thread exists</returns>
    inline bool valid() const { return (_handle != NULL && ExitCode() == STILL_ACTIVE); }

    /// <summary>
    /// Get WOW64 TEB
    /// </summary>
    /// <param name="pteb">Process TEB</param>
    /// <returns>TEB pointer</returns>
    ptr_t teb( _TEB32* pteb = nullptr ) const;
   
    /// <summary>
    /// Get Native TEB
    /// </summary>
    /// <param name="pteb">Process TEB</param>
    /// <returns>TEB pointer</returns>
    ptr_t teb( _TEB64* pteb = nullptr ) const;

    /// <summary>
    /// Get TEB
    /// </summary>
    /// <returns>TEB pointer</returns>
    inline ptr_t teb() const { return teb( (TEB_T*)nullptr ); }

    /// <summary>
    /// Get thread creation time
    /// </summary>
    /// <returns>Thread creation time</returns>
    uint64_t startTime();

    /// <summary>
    /// Get total execution time(user mode and kernel mode)
    /// </summary>
    /// <returns>Total execution time</returns>
    uint64_t execTime();

    /// <summary>
    /// Suspend thread
    /// </summary>
    /// <returns>true on success</returns>
    bool Suspend();

    /// <summary>
    /// Resumes thread.
    /// </summary>
    /// <returns>true on success</returns>
    bool Resume();

    /// <summary>
    /// Get WOW64 thread context
    /// </summary>
    /// <param name="ctx">Returned context</param>
    /// <param name="flags">Context flags.</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>true on success</returns>
    bool GetContext( _CONTEXT32& ctx, DWORD flags = CONTEXT_ALL, bool dontSuspend = false );

    /// <summary>
    /// Get native thread context
    /// </summary>
    /// <param name="ctx">Returned context</param>
    /// <param name="flags">Context flags.</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>true on success</returns>
    bool GetContext( _CONTEXT64& ctx, DWORD flags = CONTEXT64_ALL, bool dontSuspend = false );

    /// <summary>
    /// Set WOW64 thread context
    /// </summary>
    /// <param name="ctx">Context to set</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>true on success</returns>
    bool SetContext( _CONTEXT32& ctx, bool dontSuspend = false );

    /// <summary>
    /// Set native thread context
    /// </summary>
    /// <param name="ctx">Context to set</param>
    /// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
    /// <returns>true on success</returns>
    bool SetContext( _CONTEXT64& ctx, bool dontSuspend = false );

    /// <summary>
    /// Terminate thread
    /// </summary>
    /// <param name="code">Exit code</param>
    /// <returns>true on success</returns>
    bool Terminate( DWORD code = 0 );

    /// <summary>
    /// Join thread
    /// </summary>
    /// <param name="timeout">Join timeout</param>
    /// <returns>true on success</returns>
    bool Join( int timeout = INFINITE );

    /// <summary>
    /// Get thread exit code
    /// </summary>
    /// <returns>Thread exit code</returns>
    DWORD ExitCode() const;

    /// <summary>
    /// Add hardware breakpoint to thread
    /// </summary>
    /// <param name="addr">Breakpoint address</param>
    /// <param name="type">Breakpoint type(read/write/execute)</param>
    /// <param name="length">Number of bytes to include into breakpoint</param>
    /// <returns>Index of used breakpoint; -1 if failed</returns>
    int AddHWBP( ptr_t addr, HWBPType type, HWBPLength length );

    /// <summary>
    /// Remove existing hardware breakpoint
    /// </summary>
    /// <param name="idx">Breakpoint index</param>
    /// <returns>true on success</returns>
    bool RemoveHWBP( int idx );

    /// <summary>
    /// Remove existing hardware breakpoint
    /// </summary>
    /// <param name="ptr">Breakpoint address</param>
    /// <returns>true on success</returns>
    bool RemoveHWBP( ptr_t ptr );

    inline bool operator ==(const Thread& other) { return (_id == other._id); }

    Thread& operator =(const Thread& other)
    {
        _id = other._id;
        _core = other._core;
        _handle = other._handle;

        // Transfer handle ownership
        other.ReleaseHandle();

        return *this;
    }

private:

    /// <summary>
    /// Release thread handle
    /// </summary>
    inline void ReleaseHandle() const { _handle = NULL; }

private:
    class ProcessCore* _core;           // Core routines

    DWORD _id = 0;                      // Thread ID
    mutable HANDLE _handle = NULL;      // Thread handle
};

}