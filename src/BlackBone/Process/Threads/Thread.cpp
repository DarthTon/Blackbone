#include "Thread.h"
#include "../ProcessCore.h"
#include "../../Misc/DynImport.h"
#include "../../Include/Macro.h"

namespace blackbone
{

Thread::Thread( DWORD id, ProcessCore* core, DWORD access /*= DEFAULT_ACCESS*/ )
    : _core( core )
    , _id( id )
    , _handle( OpenThread( access , FALSE, id ) )
{  
}

Thread::Thread( HANDLE handle, ProcessCore* core )
    : _core( core )
    , _handle( handle )
{
    _id = (handle != NULL ? GetThreadIdT( handle ) : 0);
}

Thread::Thread( const Thread& other )
    : _core( other._core )
    , _id( other._id )
    , _handle( other._handle )
{
    other._owner = false;
}

Thread::~Thread()
{
    Close();
}

/// <summary>
/// Get WOW64 TEB
/// </summary>
/// <param name="pteb">Process TEB</param>
/// <returns>TEB pointer</returns>
blackbone::ptr_t Thread::teb( _TEB32* pteb ) const
{
    return _core->native()->getTEB( _handle, pteb );
}

/// <summary>
/// Get Native TEB
/// </summary>
/// <param name="pteb">Process TEB</param>
/// <returns>TEB pointer</returns>
blackbone::ptr_t Thread::teb( _TEB64* pteb ) const
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
    
    // Target process is x86 and not running on x86 OS
    if (_core->isWow64() && !_core->native()->GetWow64Barrier().x86OS)
        return (SAFE_CALL(Wow64SuspendThread, _handle ) != -1);
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
/// Check if thread is suspended
/// </summary>
/// <returns>true if suspended</returns>
bool Thread::Suspended()
{
    auto count = (_core->isWow64() && !_core->native()->GetWow64Barrier().x86OS)
        ? SAFE_CALL( Wow64SuspendThread, _handle )
        : SuspendThread( _handle );

    ResumeThread( _handle );
    return count > 0;
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
        result = NT_SUCCESS( _core->native()->SetThreadContextT( _handle, ctx ) );
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
        result = NT_SUCCESS( _core->native()->SetThreadContextT( _handle, ctx ) );
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
    _CONTEXT64 context64 = { 0 };
    _CONTEXT32 context32 = { 0 };
    bool use64 = !_core->native()->GetWow64Barrier().x86OS;

    // CONTEXT_DEBUG_REGISTERS can be operated without thread suspension
    bool res = use64 ? GetContext( context64, CONTEXT64_DEBUG_REGISTERS, true ) : GetContext( context32, CONTEXT_DEBUG_REGISTERS, true );
    auto pDR7 = use64 ? reinterpret_cast<regDR7*>(&context64.Dr7) : reinterpret_cast<regDR7*>(&context32.Dr7);
    if (!res)
        return -1;

    // Get free DR
    int freeIdx = pDR7->getFreeIndex();

    // If all 4 registers are occupied - error
    if (freeIdx < 0)
    {
        LastNtStatus( STATUS_NO_MORE_ENTRIES );
        return -1;
    }

    // Enable corresponding HWBP and local BP flag

    pDR7->l_enable = 1;
    pDR7->setLocal( freeIdx, 1 );
    pDR7->setRW( freeIdx, static_cast<char>(type) );
    pDR7->setLen( freeIdx, static_cast<char>(length) );

    use64 ? *(&context64.Dr0 + freeIdx) = addr : *(&context32.Dr0 + freeIdx) = static_cast<DWORD>(addr);

    // Write values to registers
    res = use64 ? SetContext( context64, true ) : SetContext( context32, true );
    return res ? freeIdx : -1;
}

/// <summary>
/// Remove existing hardware breakpoint
/// </summary>
/// <param name="idx">Breakpoint index</param>
/// <returns>true on success</returns>
bool Thread::RemoveHWBP( int idx )
{
    if (idx < 0 || idx > 4)
        return false;
   
    _CONTEXT64 context64 = { 0 };
    _CONTEXT32 context32 = { 0 };
    bool use64 = !_core->native()->GetWow64Barrier().x86OS;
    bool res = use64 ? GetContext( context64, CONTEXT64_DEBUG_REGISTERS, true ) : GetContext( context32, CONTEXT_DEBUG_REGISTERS, true );
    auto pDR7 = use64 ? reinterpret_cast<regDR7*>(&context64.Dr7) : reinterpret_cast<regDR7*>(&context32.Dr7);
    if (!res)
        return false;
   
    pDR7->setLocal( idx, 0 );
    pDR7->setLen( idx, 0 );
    pDR7->setRW( idx, 0 );
    if (pDR7->empty())
        pDR7->l_enable = 0;

    return use64 ? SetContext( context64 ) : SetContext( context32 );
}

/// <summary>
/// Remove existing hardware breakpoint
/// </summary>
/// <param name="ptr">Breakpoint address</param>
/// <returns>true on success</returns>
bool Thread::RemoveHWBP( ptr_t ptr )
{
    _CONTEXT64 context64 = { 0 };
    _CONTEXT32 context32 = { 0 };
    bool use64 = !_core->native()->GetWow64Barrier().x86OS;
    bool res = use64 ? GetContext( context64, CONTEXT64_DEBUG_REGISTERS, true ) : GetContext( context32, CONTEXT_DEBUG_REGISTERS, true );
    auto pDR7 = use64 ? reinterpret_cast<regDR7*>(&context64.Dr7) : reinterpret_cast<regDR7*>(&context32.Dr7);

    if (!res)
        return false;

    // Search for breakpoint
    for (int i = 0; i < 4; i++)
    {
        if ((&context64.Dr0)[i] == ptr || (&context32.Dr0)[i] == static_cast<DWORD>(ptr))
        {
            use64 ? *(&context64.Dr0 + i) = 0 : *(&context32.Dr0 + i) = 0;

            pDR7->setLocal( i, 0 );
            pDR7->setLen( i, 0 );
            pDR7->setRW( i, 0 );
            if (pDR7->empty())
                pDR7->l_enable = 0;

            return use64 ? SetContext( context64 ) : SetContext( context32 );
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
    DWORD code = MAXULONG32_2;
    GetExitCodeThread( _handle, &code );

    return code;
}

/// <summary>
/// Get thread creation time
/// </summary>
/// <returns>Thread creation time</returns>
uint64_t Thread::startTime()
{
    FILETIME times[4] = { { 0 } };

    if (GetThreadTimes( _handle, &times[0], &times[1], &times[2], &times[3] ))
        return (static_cast<uint64_t>(times[0].dwHighDateTime) << 32) | times[0].dwLowDateTime;

    return MAXULONG64_2;
}

/// <summary>
/// Get total execution time(user mode and kernel mode)
/// </summary>
/// <returns>Total execution time</returns>
uint64_t Thread::execTime()
{
    FILETIME times[4] = { { 0 } };

    if (GetThreadTimes( _handle, &times[0], &times[1], &times[2], &times[3] ))
        return ((static_cast<uint64_t>(times[2].dwHighDateTime) << 32) | times[2].dwLowDateTime) 
             + ((static_cast<uint64_t>(times[3].dwHighDateTime) << 32) | times[3].dwLowDateTime);

    return MAXULONG64_2;
}

/// <summary>
/// Close handle
/// </summary>
void Thread::Close()
{
    if (_owner && _handle)
    {
        CloseHandle( _handle );
        _handle = NULL;
        _id = 0;
    }
}

/// <summary>
/// GetThreadId support for XP
/// </summary>
/// <param name="hThread">Thread handle</param>
/// <returns>Thread ID</returns>
DWORD Thread::GetThreadIdT( HANDLE hThread )
{
    static auto pGetThreadId = (decltype(&GetThreadId))GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "GetThreadId" );
    if (pGetThreadId != nullptr)
    {
        return pGetThreadId( hThread );
    }
    // XP version
    else
    {
        _THREAD_BASIC_INFORMATION_T<DWORD> tbi = { 0 };
        ULONG bytes = 0;
        
        if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationThread, hThread, (THREADINFOCLASS)0, &tbi, (ULONG)sizeof( tbi ), &bytes ) ))
            return tbi.ClientID.UniqueThread;

        return 0;
    }
}


}