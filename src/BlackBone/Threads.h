#pragma once

#include "Winheaders.h"
#include "Thread.h"

#include <vector>

namespace blackbone
{

class ProcessThreads
{
public:
    ProcessThreads( class ProcessCore& core );
    ~ProcessThreads();

    /// <summary>
    /// Create the thread.
    /// </summary>
    /// <param name="threadProc">Thread enty point</param>
    /// <param name="arg">Thread argument.</param>
    /// <param name="flags">Thread creation flags</param>
    /// <returns>New thread object</returns>
    Thread CreateNew( ptr_t threadProc, ptr_t arg, DWORD flags = 0 );

    /// <summary>
    /// Gets all process threads
    /// </summary>
    /// <param name="dontUpdate">Return already existing thread list</param>
    /// <returns>Threads collection</returns>
    std::vector<Thread>& getAll( bool dontUpdate = false );

    /// <summary>
    /// Get main process thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    Thread* getMain();

    /// <summary>
    /// Get least executed thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    Thread* getLeastExecuted();

    /// <summary>
    /// Get least executed thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    Thread* getRandom();

    /// <summary>
    /// Get thread by ID
    /// </summary>
    /// <param name="id">Thread ID</param>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    Thread* get( DWORD id );

private:
    ProcessThreads( const ProcessThreads& ) = delete;
    ProcessThreads& operator =(const ProcessThreads&) = delete;

private:
    class ProcessCore& _core;       // Core process functions
    std::vector<Thread> _threads;   // Process thread snapshot
};

}