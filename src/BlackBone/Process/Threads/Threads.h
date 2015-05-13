#pragma once

#include "../../Include/Winheaders.h"
#include "Thread.h"

#include <vector>

namespace blackbone
{

class ProcessThreads
{
public:
    BLACKBONE_API ProcessThreads( class ProcessCore& core );
    BLACKBONE_API ~ProcessThreads();

    /// <summary>
    /// Create the thread.
    /// </summary>
    /// <param name="threadProc">Thread enty point</param>
    /// <param name="arg">Thread argument.</param>
    /// <param name="flags">Thread creation flags</param>
    /// <returns>New thread object</returns>
    BLACKBONE_API Thread CreateNew( ptr_t threadProc, ptr_t arg, enum CreateThreadFlags flags = (CreateThreadFlags)0 );

    /// <summary>
    /// Gets all process threads
    /// </summary>
    /// <param name="dontUpdate">Return already existing thread list</param>
    /// <returns>Threads collection</returns>
    BLACKBONE_API std::vector<Thread>& getAll( bool dontUpdate = false );

    /// <summary>
    /// Get main process thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    BLACKBONE_API Thread* getMain();

    /// <summary>
    /// Get least executed thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    BLACKBONE_API Thread* getLeastExecuted();

    /// <summary>
    /// Get most executed thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    BLACKBONE_API Thread* getMostExecuted();

    /// <summary>
    /// Get random thread
    /// </summary>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    BLACKBONE_API Thread* getRandom();

    /// <summary>
    /// Get thread by ID
    /// </summary>
    /// <param name="id">Thread ID</param>
    /// <returns>Pointer to thread object, nullptr if failed</returns>
    BLACKBONE_API Thread* get( DWORD id );

    /// <summary>
    /// Reset data
    /// </summary>
    BLACKBONE_API inline void reset() { _threads.clear(); }

private:
    ProcessThreads( const ProcessThreads& ) = delete;
    ProcessThreads& operator =(const ProcessThreads&) = delete;
private:
    class ProcessCore& _core;       // Core process functions
    std::vector<Thread> _threads;   // Process thread snapshot
};

}