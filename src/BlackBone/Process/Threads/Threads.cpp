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
Thread ProcessThreads::CreateNew( ptr_t threadProc, ptr_t arg, enum CreateThreadFlags flags /*= NoThreadFlags*/ )
{
    HANDLE hThd = NULL;
    if (!NT_SUCCESS( _core.native()->CreateRemoteThreadT( hThd, threadProc, arg, flags, THREAD_ALL_ACCESS ) ))
    {
        // Ensure full thread access
        if (NT_SUCCESS( _core.native()->CreateRemoteThreadT( hThd, threadProc, arg, flags, THREAD_QUERY_LIMITED_INFORMATION ) ))
        {
            if (Driver().loaded())
                Driver().PromoteHandle( GetCurrentProcessId(), hThd, THREAD_ALL_ACCESS );
        }
    }

    return Thread( hThd, &_core );
}

/// <summary>
/// Gets all process threads
/// </summary>
/// <param name="dontUpdate">Return already existing thread list</param>
/// <returns>Threads collection</returns>
std::vector<Thread>& ProcessThreads::getAll( bool dontUpdate /*= false*/ )
{
    if (!_threads.empty() && dontUpdate)
        return _threads;

    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );

    _threads.clear();

    if (hThreadSnapshot != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 tEntry = { 0 };
        tEntry.dwSize = sizeof(THREADENTRY32);

        // Iterate threads
        for (BOOL success = Thread32First( hThreadSnapshot, &tEntry ); 
              success == TRUE;
              success = Thread32Next( hThreadSnapshot, &tEntry ))
        {
            if (tEntry.th32OwnerProcessID != _core.pid())
                continue;

            _threads.emplace_back( Thread( tEntry.th32ThreadID, &_core ) );
        }

        CloseHandle( hThreadSnapshot );
    }

    return _threads;
}

/// <summary>
/// Get main process thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
Thread* ProcessThreads::getMain()
{
    uint64_t mintime = MAXULONG64_2;
    Thread* pMain = nullptr;

    for (auto& thread : getAll())
    {
        uint64_t time = thread.startTime();

        if (time < mintime)
        {
            mintime = time;
            pMain = &thread;
        }
    }

    return pMain;
}

/// <summary>
/// Get least executed thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
Thread* ProcessThreads::getLeastExecuted()
{
    uint64_t mintime = MAXULONG64_2;
    Thread* pThread = nullptr;

    for (auto& thread : getAll())
    {
        uint64_t time = thread.execTime();

        if (time < mintime)
        {
            mintime = time;
            pThread = &thread;
        }
    }

    return pThread;
}

/// <summary>
/// Get most executed thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
Thread* ProcessThreads::getMostExecuted()
{
    uint64_t maxtime = 0;
    Thread* pThread = getMain();
    if (pThread->Suspended())
        pThread = nullptr;

    for (auto& thread : getAll( true ))
    {
        uint64_t time = thread.execTime();
        if (!thread.Suspended() && time > maxtime)
        {
            maxtime = time;
            pThread = &thread;
        }
    }

    return pThread;
}

/// <summary>
/// Get random thread
/// </summary>
/// <returns>Pointer to thread object, nullptr if failed</returns>
Thread* ProcessThreads::getRandom()
{
    getAll();

    if (_threads.empty())
        return nullptr;

    static std::random_device rd;
    std::uniform_int_distribution<size_t> dist( 0, _threads.size() - 1 );

    return &_threads[dist(rd)];
}

/// <summary>
/// Get thread by ID
/// </summary>
/// <param name="id">Thread ID</param>
/// <returns>Pointer to thread object, nullptr if failed</returns>
Thread* ProcessThreads::get( DWORD id )
{
    getAll();
    auto iter = std::find_if( _threads.begin(), _threads.end(), [id]( const Thread& item ) { return item.id() == id; } );
    if (iter != _threads.end())
        return &*iter;

    return nullptr;
}

}