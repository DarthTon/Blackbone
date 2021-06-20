#pragma once

#include "../../Include/Winheaders.h"
#include "../../Asm/AsmFactory.h"
#include "../Threads/Threads.h"
#include "../MemBlock.h"

// User data offsets
#define INTRET_OFFSET   0x00
#define RET_OFFSET      0x08
#define ERR_OFFSET      0x10
#define EVENT_OFFSET    0x18
#define ARGS_OFFSET     0x20


namespace blackbone
{

enum WorkerThreadMode
{
    Worker_None,            // No worker thread
    Worker_CreateNew,       // Create dedicated worker thread
    Worker_UseExisting,     // Hijack existing thread
};

class RemoteExec
{
    using vecArgs = std::vector<AsmVariant>;

public:
    BLACKBONE_API RemoteExec( class Process& proc );
    BLACKBONE_API ~RemoteExec();

    /// <summary>
    /// Create environment for future remote procedure calls
    ///
    /// _userData layout (x86/x64):
    /// --------------------------------------------------------------------------------------------------------------------------
    /// | Internal return value | Return value |  Last Status code  |  Event handle   |  Space for copied arguments and strings  |
    /// -------------------------------------------------------------------------------------------------------------------------
    /// |       8/8 bytes       |   8/8 bytes  |      8/8 bytes     |    8/8 bytes    |                                          |
    /// --------------------------------------------------------------------------------------------------------------------------
    /// </summary>
    /// <param name="mode">Worket thread mode</param>
    /// <param name="bEvent">Create sync event for worker thread</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS CreateRPCEnvironment( WorkerThreadMode mode = Worker_None, bool bEvent = false );

    /// <summary>
    /// Create new thread and execute code in it. Wait until execution ends
    /// </summary>
    /// <param name="pCode">Code to execute</param>
    /// <param name="size">Code size</param>
    /// <param name="callResult">Code return value</param>
    /// <param name="modeSwitch">Switch wow64 thread to long mode upon creation</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS ExecInNewThread(
        PVOID pCode, size_t size, 
        uint64_t& callResult, 
        eThreadModeSwitch modeSwitch = AutoSwitch 
        );

    /// <summary>
    /// Execute code in context of our worker thread
    /// </summary>
    /// <param name="pCode">Cde to execute</param>
    /// <param name="size">Code size.</param>
    /// <param name="callResult">Execution result</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS ExecInWorkerThread( PVOID pCode, size_t size, uint64_t& callResult );

    /// <summary>
    /// Execute code in context of any existing thread
    /// </summary>
    /// <param name="pCode">Cde to execute</param>
    /// <param name="size">Code size.</param>
    /// <param name="callResult">Execution result</param>
    /// <param name="thd">Target thread</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS ExecInAnyThread( PVOID pCode, size_t size, uint64_t& callResult, ThreadPtr& thread );

    /// <summary>
    /// Create new thread with specified entry point and argument
    /// </summary>
    /// <param name="pCode">Entry point</param>
    /// <param name="arg">Thread function argument</param>
    /// <returns>Thread exit code</returns>
    BLACKBONE_API DWORD ExecDirect( ptr_t pCode, ptr_t arg );

    /// <summary>
    /// Generate assembly code for remote call.
    /// </summary>
    /// <param name="a">Underlying assembler object</param>
    /// <param name="pfn">Remote function pointer</param>
    /// <param name="args">Function arguments</param>
    /// <param name="cc">Calling convention</param>
    /// <param name="retType">Return type</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS PrepareCallAssembly(
        IAsmHelper& a,
        ptr_t pfn,
        std::vector<blackbone::AsmVariant>& args,
        eCalligConvention cc,
        eReturnType retType
    );

    /// <summary>
    /// Generate return from function with event synchronization
    /// </summary>
    /// <param name="a">Target assembly helper</param>
    /// <param name="mt">32/64bit loader</param>
    /// <param name="retType">Function return type</param>
    /// <param name="retOffset">Return value offset</param>
    BLACKBONE_API void AddReturnWithEvent(
        IAsmHelper& a,
        eModType mt = mt_default, 
        eReturnType retType = rt_int32,
        uint32_t retOffset = RET_OFFSET 
        );

    /// <summary>
    /// Save value in rax to user buffer
    /// </summary>
    /// <param name="a">Target assembly helper</param>
    BLACKBONE_API void SaveCallResult( IAsmHelper& a, uint32_t retOffset = RET_OFFSET )
    {
        a->mov( a->zdx, _userData[_currentBufferIdx].ptr() + retOffset );
        a->mov( asmjit::host::dword_ptr( a->zdx ), a->zax );
    }

    /// <summary>
    /// Retrieve call result
    /// </summary>
    /// <param name="result">Retrieved result</param>
    /// <returns>true on success</returns>
    template<typename T>
    NTSTATUS GetCallResult( T& result )
    {
        // This method is called after an RPC call, so the ping pong buffers have already been switched, so
        // we want to access the OTHER buffer here.
        if constexpr (sizeof( T ) > sizeof( uint64_t ))
        {
            if constexpr (std::is_reference_v<T>)
                return _userData[1 - _currentBufferIdx].Read( _userData[1 - _currentBufferIdx].Read<uintptr_t>( RET_OFFSET, 0 ), sizeof( T ), reinterpret_cast<PVOID>(&result) );
            else
                return _userData[1 - _currentBufferIdx].Read( ARGS_OFFSET, sizeof( T ), reinterpret_cast<PVOID>(&result) );
        }
        else
            return _userData[1 - _currentBufferIdx].Read( RET_OFFSET, sizeof( T ), reinterpret_cast<PVOID>(&result) );
    }

    /// <summary>
    /// Retrieve last NTSTATUS code
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API NTSTATUS GetLastStatus()
    {
        return _userData[_currentBufferIdx].Read<NTSTATUS>( ERR_OFFSET, STATUS_NOT_FOUND );
    }

    /// <summary>
    /// Terminate existing worker thread
    /// </summary>
    BLACKBONE_API void TerminateWorker();

    /// <summary>
    /// Get worker thread
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API ThreadPtr getWorker() { return _workerThread; }

    /// <summary>
    /// Get execution thread
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API ThreadPtr getExecThread() { return _hijackThread ? _hijackThread : _workerThread; }

    /// <summary>
    /// Ge memory routines
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API class ProcessMemory& memory() { return _memory; }

    /// <summary>
    /// Reset instance
    /// </summary>
    BLACKBONE_API void reset();

private:

    /// <summary>
    /// Create worker RPC thread
    /// </summary>
    /// <returns>Thread ID</returns>
    call_result_t<DWORD> CreateWorkerThread();

    /// <summary>
    /// Create event to synchronize APC procedures
    /// </summary>
    /// <param name="threadID">The thread identifier.</param>
    /// <returns>Status code</returns>
    NTSTATUS CreateAPCEvent( DWORD threadID );

    /// <summary>
    /// Copy executable code into remote codecave for future execution
    /// </summary>
    /// <param name="pCode">Code to copy</param>
    /// <param name="size">Code size</param>
    /// <returns>Status</returns>
    NTSTATUS CopyCode( PVOID pCode, size_t size );

    void SwitchActiveBuffer()
    {
        // The ExecIn*() methods might return while the remote RPC code is still executing (it still has to
        // return after signaling the event), making it possible to start another RPC call which would then
        // overwrite the _userData/_userCode blocks that are still in use by the previous call. If subsequent
        // RPC calls use the same buffers, this creates a race condition, very rarely resulting in crashes in
        // the remote process, especially in KiUserApcDispatcher().
        //
        // For this reason, we allocate two separate instances of _userCode and _userData, and switch between
        // them for subsequent RPC calls (buffer ping-ponging). This should prevent the race condition without
        // having to Sleep() for an arbitrary amount of time after each RPC call.
        _currentBufferIdx = 1 - _currentBufferIdx;
    }

    RemoteExec( const RemoteExec& ) = delete;
    RemoteExec& operator =(const RemoteExec&) = delete;

private:    
    // Process routines
    class Process&        _process;
    class ProcessModules& _mods;
    class ProcessMemory&  _memory;
    class ProcessThreads& _threads;

    ThreadPtr _workerThread;    // Worker thread handle
    ThreadPtr _hijackThread;    // Thread to use for hijacking  
    HANDLE    _hWaitEvent;      // APC sync event handle
    MemBlock  _workerCode;      // Worker thread address space
    MemBlock  _userCode[2];     // Codecave for code execution
    MemBlock  _userData[2];     // Region to store copied structures and strings
    bool      _apcPatched;      // KiUserApcDispatcher was patched
    int       _currentBufferIdx;// Index of the currently used _userCode/_userData block. See SwitchActiveBuffer().
};


}
