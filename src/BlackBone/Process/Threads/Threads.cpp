#include "Threads.h"
#include "../ProcessCore.h"
#include "../../DriverControl/DriverControl.h"

#include <memory>
#include <random>
#include <algorithm>

namespace blackbone
{
    
ProcessThreads::ProcessThreads( ProcessCore& core )
    : _core( core )
{
}

ProcessThreads::~ProcessThreads()
{
}

/// <summary>
/// Create the thread.
/// </summary>
/// <param name="threadProc">Thread enty point</param>
/// <param name="arg">Thread argument.</param>
/// <param name="flags">Thread creation flags</param>
/// <returns>New thread object</returns>
call_result_t<ThreadPtr> ProcessThreads::CreateNew( ptr_t threadProc, ptr_t arg, enum CreateThreadFlags flags /*= NoThreadFlags*/ )
{
    HANDLE hThd = NULL;
    auto status = _core.native()->CreateRemoteThreadT( hThd, threadProc, arg, flags, THREAD_ALL_ACCESS );
    if (!NT_SUCCESS( status ))
    {
        // Ensure full thread access
        status = _core.native()->CreateRemoteThreadT( hThd, threadProc, arg, flags, THREAD_QUERY_LIMITED_INFORMATION );
        if (NT_SUCCESS( status ))
        {
            if (Driver().loaded())
                status = Driver().PromoteHandle( GetCurrentProcessId(), hThd, THREAD_ALL_ACCESS );
        }
    }

    if (!NT_SUCCESS( status ))
        return status;

    CSLock lg( _lock );
    _threads.emplace_back( std::make_shared<Thread>( hThd, &_core ) );
    return _threads.back();
}

/// <summary>
/// Gets all process threads
/// </summary>
/// <param name="dontUpdate">Return already existing thread list</param>
/// <returns>Threads collection</returns>
std::vector<ThreadPtr>& ProcessThreads::getAll( bool dontUpdate /*= false*/ )
{
    if (!_threads.empty() && dontUpdate)
        return _threads;

    auto hThreadSnapshot = SnapHandle( CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ) );
    if (!hThreadSnapshot)
        return _threads;
   

    THREADENTRY32 tEntry = { 0 };
    tEntry.dwSize = sizeof( THREADENTRY32 );

    _threads.clear();

    // Iterate threads
    for (BOOL success = Thread32First( hThreadSnapshot, &tEntry );
        success != FALSE;
        success = Thread32Next( hThreadSnapshot, &tEntry ))
    {
        if (tEntry.th32OwnerProcessID != _core.pid())
            continue;

        CSLock lg( _lock );
        _threads.emplace_back( std::make_shared<Thread>( tEntry.th32ThreadID, &_core ) );
    }

    return _threads;
}

/// <summary>
/// Get main process thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getMain()
{
    uint64_t mintime = MAXULONG64_2;
    ThreadPtr pMain;

    for (auto& thread : getAll())
    {
        uint64_t time = thread->startTime();

        if (time < mintime)
        {
            mintime = time;
            pMain = thread;
        }
    }

    return pMain ? pMain : (!_threads.empty() ? _threads.front() : nullptr);
}

/// <summary>
/// Get least executed thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getLeastExecuted()
{
    uint64_t mintime = MAXULONG64_2;
    ThreadPtr pThread;

    for (auto& thread : getAll())
    {
        uint64_t time = thread->execTime();

        if (time < mintime)
        {
            mintime = time;
            pThread = thread;
        }
    }

    return pThread;
}

/// <summary>
/// Get most executed thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getMostExecuted()
{
    uint64_t maxtime = 0;
    ThreadPtr result;

    for (auto& thread : getAll( true ))
    {
        if (thread->id() == GetCurrentThreadId())
            continue;

        uint64_t time = thread->execTime();
        if (thread->id() != GetCurrentThreadId() /*&& !thread->Suspended()*/ && time > maxtime)
        {
            maxtime = time;
            result = thread;
        }
    }

    return result;
}

/// <summary>
/// Get random thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getRandom()
{
    if (getAll().empty())
        return nullptr;

    static std::random_device rd;
    std::uniform_int_distribution<size_t> dist( 0, _threads.size() - 1 );

    return _threads[dist(rd)];
}

/// <summary>
/// Get thread by ID
/// </summary>
/// <param name="id">Thread ID</param>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::get( DWORD id )
{
    getAll();
    auto iter = std::find_if( _threads.begin(), _threads.end(), [id]( const ThreadPtr& thread ) { return thread->id() == id; } );
    if (iter != _threads.end())
        return *iter;

    return nullptr;
}

}