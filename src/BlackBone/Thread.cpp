#include "Thread.h"
#include "ProcessCore.h"
#include "Macro.h"

namespace blackbone
{
    
Thread::Thread( DWORD id, ProcessCore* core, DWORD access /*= DEFAULT_ACCESS*/ )
    : _id( id )
    , _core( core )
    , _handle( OpenThread( access , FALSE, id ) )
{  
}

Thread::Thread( HANDLE handle, ProcessCore* core )
    : _handle( handle )
    , _core( core )
    , _id( handle != NULL ? GetThreadId( handle ) : 0 )
{
}

Thread::Thread( const Thread& other )
    : _handle( other._handle )
    , _id( other._id )
    , _core( other._core )
{
    other.ReleaseHandle();
}

Thread::~Thread()
{
    if (_handle != NULL)
        CloseHandle( _handle );
}

/// <summary>
/// Get WOW64 TEB
/// </summary>
/// <param name="pteb">Process TEB</param>
/// <returns>TEB pointer</returns>
blackbone::ptr_t Thread::teb( _TEB32* pteb /*= nullptr */ ) const
{
    return _core->native()->getTEB( _handle, pteb );
}

/// <summary>
/// Get Native TEB
/// </summary>
/// <param name="pteb">Process TEB</param>
/// <returns>TEB pointer</returns>
blackbone::ptr_t Thread::teb( _TEB64* pteb /*= nullptr */ ) const
{
    return _core->native()->getTEB( _handle, pteb );
}

/// <summary>
/// Suspend thread
/// </summary>
/// <returns>true on success</returns>
bool Thread::Suspend()
{
    // Prevent deadlock
    if (_id == GetCurrentThreadId())
        return true;

    if (_core->isWow64())
        return (Wow64SuspendThread( _handle ) != -1);
    else
        return (SuspendThread( _handle ) != -1);
}

/// <summary>
/// Resumes thread.
/// </summary>
/// <returns>true on success</returns>
bool Thread::Resume()
{
    if (_id == GetCurrentThreadId())
        return true;

    return (ResumeThread( _handle ) != -1);
}

/// <summary>
/// Get WOW64 thread context
/// </summary>
/// <param name="ctx">Returned context</param>
/// <param name="flags">Context flags.</param>
/// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
/// <returns>true on success</returns>
bool Thread::GetContext( _CONTEXT32& ctx, DWORD flags /*= CONTEXT_ALL*/, bool dontSuspend /*= false*/ )
{
    bool result = false;

    memset( &ctx, 0x00, sizeof(ctx) );
    ctx.ContextFlags = flags;

    if (dontSuspend || Suspend())
    {
        result = (_core->native()->GetThreadContextT( _handle, ctx ) == STATUS_SUCCESS);

        if (!dontSuspend)
            Resume();
    }
        
    return result;
}

/// <summary>
/// Get native thread context
/// </summary>
/// <param name="ctx">Returned context</param>
/// <param name="flags">Context flags.</param>
/// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
/// <returns>true on success</returns>
bool Thread::GetContext( _CONTEXT64& ctx, DWORD flags /*= CONTEXT64_ALL*/, bool dontSuspend /*= false*/ )
{
    bool result = false;

    memset( &ctx, 0x00, sizeof(ctx) );
    ctx.ContextFlags = flags;

    if (dontSuspend || Suspend())
    {
        result = (_core->native()->GetThreadContextT( _handle, ctx ) == STATUS_SUCCESS);

        if (!dontSuspend)
            Resume();
    }

    return result;
}

/// <summary>
/// Set WOW64 thread context
/// </summary>
/// <param name="ctx">Context to set</param>
/// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
/// <returns>true on success</returns>
bool Thread::SetContext( _CONTEXT32& ctx, bool dontSuspend /*= false */ )
{
    bool result = false;

    if (dontSuspend || Suspend())
    {
        result = (_core->native()->SetThreadContextT( _handle, ctx ) == STATUS_SUCCESS);

        if (!dontSuspend)
            Resume();
    }

    return result;
}

/// <summary>
/// Set native thread context
/// </summary>
/// <param name="ctx">Context to set</param>
/// <param name="dontSuspend">true if thread shouldn't be suspended before retrieving context</param>
/// <returns>true on success</returns>
bool Thread::SetContext( _CONTEXT64& ctx, bool dontSuspend /*= false*/ )
{
    bool result = false;

    if(dontSuspend || Suspend())
    {
        result = (_core->native()->SetThreadContextT( _handle, ctx ) == STATUS_SUCCESS);

        if(!dontSuspend)
            Resume();
    }

    return result;
}

/// <summary>
/// Terminate thread
/// </summary>
/// <param name="code">Exit code</param>
/// <returns>true on success</returns>
bool Thread::Terminate( DWORD code /*= 0*/ )
{
    return (TerminateThread( _handle, code ) == TRUE);
}

/// <summary>
/// Join thread
/// </summary>
/// <param name="timeout">Join timeout</param>
/// <returns>true on success</returns>
bool Thread::Join( int timeout /*= INFINITE*/ )
{
    return (WaitForSingleObject( _handle, timeout ) == WAIT_OBJECT_0);
}

/// <summary>
/// Add hardware breakpoint to thread
/// </summary>
/// <param name="addr">Breakpoint address</param>
/// <param name="type">Breakpoint type(read/write/execute)</param>
/// <param name="length">Number of bytes to include into breakpoint</param>
/// <returns>Index of used breakpoint; -1 if failed</returns>
int Thread::AddHWBP( ptr_t addr, HWBPType type, HWBPLength length )
{
    _CONTEXT64 context;

    // CONTEXT_DEBUG_REGISTERS can be operated without thread suspension
    if (!GetContext( context, CONTEXT64_DEBUG_REGISTERS, true ))
        return -1;

    auto getFree = []( ptr_t regval )
    {
        if (!(regval & 1))
            return 0;
        else if (!(regval & 4))
            return 1;
        else if (!(regval & 16))
            return 2;
        else if (!(regval & 64))
            return 3;

        return -1;
    };

    // Get free DR
    int freeIdx = getFree( context.Dr7 );

    // If all 4 registers are occupied - error
    if (freeIdx < 0)
        return -1;

    // Enable corresponding HWBP and local BP flag
    context.Dr7 |= (1 << (2 * freeIdx)) | ((int)type << (16 + 4 * freeIdx)) | ((int)length << (18 + 4 * freeIdx)) | 0x100;

    *(&context.Dr0 + freeIdx) = addr;

    // Write values to registers
    if (!SetContext( context, true ))
        return -1;

    return freeIdx;
}

/// <summary>
/// Remove existing hardware breakpoint
/// </summary>
/// <param name="idx">Breakpoint index</param>
/// <returns>true on success</returns>
bool Thread::RemoveHWBP( int idx )
{
    if (idx > 0 && idx < 4)
    {
        _CONTEXT64 context;

        if (GetContext( context, CONTEXT64_DEBUG_REGISTERS ))
        {

            *(&context.Dr0 + idx) = 0;
            RESET_BIT( context.Dr7, ( 2 * idx) );
            return SetContext( context );
        }
    }

    return false;
}

/// <summary>
/// Remove existing hardware breakpoint
/// </summary>
/// <param name="ptr">Breakpoint address</param>
/// <returns>true on success</returns>
bool Thread::RemoveHWBP( ptr_t ptr )
{
    _CONTEXT64 context;
    
    if (GetContext( context, CONTEXT64_DEBUG_REGISTERS ))
    {
        // Search breakpoint
        for (int i = 0; i < 4; i++)
        {
            if ((&context.Dr0)[i] == ptr)
            {
                *(&context.Dr0 + i) = 0;
                RESET_BIT( context.Dr7, ( 2 * i ) );
                return SetContext( context );
            }
        }
    }

    return false;
}

/// <summary>
/// Get thread exit code
/// </summary>
/// <returns>Thread exit code</returns>
DWORD Thread::ExitCode() const
{
    DWORD code = MAXULONG32;
    GetExitCodeThread( _handle, &code );

    return code;
}

/// <summary>
/// Get thread creation time
/// </summary>
/// <returns>Thread creation time</returns>
uint64_t Thread::startTime()
{
    FILETIME times[4] = { 0 };

    if (GetThreadTimes( _handle, &times[0], &times[1], &times[2], &times[3] ))
        return (static_cast<uint64_t>(times[0].dwHighDateTime) << 32) | times[0].dwLowDateTime;

    return MAXULONG64;
}

/// <summary>
/// Get total execution time(user mode and kernel mode)
/// </summary>
/// <returns>Total execution time</returns>
uint64_t Thread::execTime()
{
    FILETIME times[4] = { 0 };

    if (GetThreadTimes( _handle, &times[0], &times[1], &times[2], &times[3] ))
        return ((static_cast<uint64_t>(times[2].dwHighDateTime) << 32) | times[2].dwLowDateTime) 
             + ((static_cast<uint64_t>(times[3].dwHighDateTime) << 32) | times[3].dwLowDateTime);

    return MAXULONG64;
}


}