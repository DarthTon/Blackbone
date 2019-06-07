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

    return std::make_shared<Thread>( hThd, &_core );
}

/// <summary>
/// Gets all process threads
/// </summary>
/// <returns>Threads collection</returns>
std::vector<ThreadPtr> ProcessThreads::getAll() const
{
    std::vector<ThreadPtr> result;
    auto hThreadSnapshot = Handle( CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ) );
    if (!hThreadSnapshot)
        return result;
   
    THREADENTRY32 tEntry = { 0 };
    tEntry.dwSize = sizeof( THREADENTRY32 );

    // Iterate threads
    for (BOOL success = Thread32First( hThreadSnapshot, &tEntry );
        success != FALSE;
        success = Thread32Next( hThreadSnapshot, &tEntry ))
    {
        if (tEntry.th32OwnerProcessID != _core.pid())
            continue;

        result.emplace_back( std::make_shared<Thread>( tEntry.th32ThreadID, &_core ) );
    }

    return result;
}

/// <summary>
/// Get main process thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getMain() const
{
    uint64_t mintime = MAXULONG64_2;
    auto threads = getAll();
    ThreadPtr result = !threads.empty() ? threads.front() : nullptr;

    for (const auto& thread : threads)
    {
        uint64_t time = thread->startTime();
        if (time < mintime)
        {
            mintime = time;
            result = thread;
        }
    }

    return result;
}

/// <summary>
/// Get least executed thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getLeastExecuted() const
{
    uint64_t mintime = MAXULONG64_2;
    auto threads = getAll();
    ThreadPtr result = !threads.empty() ? threads.front() : nullptr;

    for (const auto& thread : threads)
    {
        uint64_t time = thread->execTime();
        if (time < mintime)
        {
            mintime = time;
            result = thread;
        }
    }

    return result;
}

/// <summary>
/// Get most executed thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::getMostExecuted() const
{
    uint64_t maxtime = 0;
    auto threads = getAll();
    ThreadPtr result = !threads.empty() ? threads.front() : nullptr;

    for (const auto& thread : threads)
    {
        if (thread->id() == GetCurrentThreadId())
            continue;

        uint64_t time = thread->execTime();
        if (thread->id() != GetCurrentThreadId() && time >= maxtime)
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
ThreadPtr ProcessThreads::getRandom() const
{
    auto threads = getAll();
    if (threads.empty())
        return nullptr;

    static std::random_device rd;
    std::uniform_int_distribution<size_t> dist( 0, threads.size() - 1 );

    return threads[dist(rd)];
}

/// <summary>
/// Get thread by ID
/// </summary>
/// <param name="id">Thread ID</param>
/// <returns>Pointer to thread object, nullptr if failed</returns>
ThreadPtr ProcessThreads::get( DWORD id ) const
{
    auto threads = getAll();
    auto iter = std::find_if( threads.begin(), threads.end(), [id]( const auto& thread ) { return thread->id() == id; } );

    return iter != threads.end() ? *iter : nullptr;
}

}