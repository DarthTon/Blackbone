#pragma once

#include "../../Include/Winheaders.h"
#include "../../Asm/AsmHelper.h"
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

class RemoteExec
{
    template<typename Fn>
    friend class RemoteFuncBase;

    typedef std::vector<AsmVariant> vecArgs;

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
    /// |       8/8 bytes       |   8/8 bytes  |      8/8 bytes     |   16/16 bytes   |                                          |
    /// --------------------------------------------------------------------------------------------------------------------------
    /// </summary>
    /// <param name="bThread">Create worker thread</param>
    /// <param name="bEvent">Create sync event for worker thread</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS CreateRPCEnvironment( bool bThread = true, bool bEvent = true );

    /// <summary>
    /// Create new thread and execute code in it. Wait until execution ends
    /// </summary>
    /// <param name="pCode">Code to execute</param>
    /// <param name="size">Code size</param>
    /// <param name="callResult">Code return value</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS ExecInNewThread( PVOID pCode, size_t size, uint64_t& callResult );

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
    BLACKBONE_API NTSTATUS ExecInAnyThread( PVOID pCode, size_t size, uint64_t& callResult, Thread& thread );

    /// <summary>
    /// Create new thread with specified entry point and argument
    /// </summary>
    /// <param name="pCode">Entry point</param>
    /// <param name="arg">Thread function argument</param>
    /// <returns>Thread exit code</returns>
    BLACKBONE_API DWORD ExecDirect( ptr_t pCode, ptr_t arg );

    /// <summary>
    /// Generate return from function with event synchronization
    /// </summary>
    /// <param name="a">Target assembly helper</param>
    /// <param name="mt">32/64bit loader</param>
    /// <param name="retType">Function return type</param>
    /// <param name="retOffset">Return value offset</param>
    BLACKBONE_API void AddReturnWithEvent(
        AsmHelperBase& a,
        eModType mt = mt_default, 
        eReturnType retType = rt_int32,
        uint32_t retOffset = RET_OFFSET 
        );

    /// <summary>
    /// Retrieve last NTSTATUS code
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline NTSTATUS GetLastStatus()
    {
        return _userData.Read<NTSTATUS>( ERR_OFFSET, STATUS_NOT_FOUND );
    }

    /// <summary>
    /// Terminate existing worker thread
    /// </summary>
    BLACKBONE_API void TerminateWorker();

    /// <summary>
    /// Get worker thread
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline Thread* getWorker() { return &_hWorkThd; }

    /// <summary>
    /// Ge memory routines
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline class ProcessMemory& memory() { return _memory; }

    /// <summary>
    /// Reset instance
    /// </summary>
    BLACKBONE_API void reset();

private:

    /// <summary>
    /// Create worker RPC thread
    /// </summary>
    /// <returns>Thread ID</returns>
    DWORD CreateWorkerThread();

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

    /// <summary>
    /// Generate assembly code for remote call.
    /// </summary>
    /// <param name="a">Underlying assembler object</param>
    /// <param name="pfn">Remote function pointer</param>
    /// <param name="args">Function arguments</param>
    /// <param name="cc">Calling convention</param>
    /// <param name="retType">Return type</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool PrepareCallAssembly(
        AsmHelperBase& a, 
        const void* pfn,
        std::vector<blackbone::AsmVariant>& args,
        eCalligConvention cc,
        eReturnType retType
        );

#pragma warning(disable : 4127)

    /// <summary>
    /// Retrieve call result
    /// </summary>
    /// <param name="result">Retrieved result</param>
    /// <returns>true on success</returns>
    template<typename T>
    inline bool GetCallResult( T& result )
    { 
        if (sizeof(T) > sizeof(uint64_t))
        {
            if (std::is_reference<T>::value)
                return _userData.Read( _userData.Read<uintptr_t>( RET_OFFSET, 0 ), sizeof(T), (PVOID)&result ) == STATUS_SUCCESS;
            else
                return _userData.Read( ARGS_OFFSET, sizeof(T), (PVOID)&result ) == STATUS_SUCCESS;
        }
        else
            return _userData.Read( RET_OFFSET, sizeof(T), (PVOID)&result ) == STATUS_SUCCESS;
    }
#pragma warning(default : 4127)

    RemoteExec( const RemoteExec& ) = delete;
    RemoteExec& operator =(const RemoteExec&) = delete;

private:    
    // Process routines
    class Process&        _proc;
    class ProcessModules& _mods;
    class ProcessMemory&  _memory;
    class ProcessThreads& _threads;

    Thread   _hWorkThd;         // Worker thread handle
    HANDLE   _hWaitEvent;       // APC sync event handle
    MemBlock _workerCode;       // Worker thread address space
    MemBlock _userCode;         // Codecave for code execution
    MemBlock _userData;         // Region to store copied structures and strings
    bool     _apcPatched;       // KiUserApcDispatcher was patched
};


}