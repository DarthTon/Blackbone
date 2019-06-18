#pragma once

#include "ProcessCore.h"
#include "ProcessMemory.h"
#include "ProcessModules.h"
#include "Threads/Threads.h"
#include "RPC/RemoteExec.h"
#include "RPC/RemoteHook.h"
#include "RPC/RemoteLocalHook.h"
#include "../ManualMap/Native/NtLoader.h"
#include "../ManualMap/MMap.h"

#include "../Include/NativeStructures.h"
#include "../Misc/InitOnce.h"

#include <string>
#include <list>

namespace blackbone
{

/// <summary>
/// Process thread information
/// </summary>
struct ThreadInfo
{
    uint32_t tid = 0;
    uintptr_t startAddress = 0;
    bool mainThread = false;
};

/// <summary>
/// Process information
/// </summary>
struct ProcessInfo
{
    uint32_t pid = 0;
    std::wstring imageName;
    std::vector<ThreadInfo> threads;

    bool operator < ( const ProcessInfo& other )
    {
        return this->pid < other.pid;
    }
};

/// <summary>
/// Section object information
/// </summary>
struct SectionInfo
{
    ptr_t size = 0;
    uint32_t attrib = 0;
};

/// <summary>
/// Process handle information
/// </summary>
struct HandleInfo
{
    HANDLE handle = nullptr;
    uint32_t access = 0;
    uint32_t flags = 0;
    ptr_t pObject = 0;

    std::wstring typeName;
    std::wstring name;

    // Object-specific info
    SectionInfo section;
};

#define DEFAULT_ACCESS_P  PROCESS_QUERY_INFORMATION | \
                          PROCESS_VM_READ           | \
                          PROCESS_VM_WRITE          | \
                          PROCESS_VM_OPERATION      | \
                          PROCESS_CREATE_THREAD     | \
                          PROCESS_SET_QUOTA         | \
                          PROCESS_TERMINATE         | \
                          PROCESS_SUSPEND_RESUME    | \
                          PROCESS_DUP_HANDLE

class Process
{
public:
    BLACKBONE_API Process();
    BLACKBONE_API explicit Process( DWORD pid, DWORD access = DEFAULT_ACCESS_P );
    BLACKBONE_API explicit Process( const wchar_t* name, DWORD access = DEFAULT_ACCESS_P );
    BLACKBONE_API explicit Process( HANDLE hProc );
    BLACKBONE_API Process(
        const std::wstring& path,
        bool suspended,
        bool forceInit,
        const std::wstring& cmdLine,
        const wchar_t* currentDir,
        STARTUPINFOW* pStartup
    );

    Process( const Process& ) = delete;
    Process& operator =( const Process& ) = delete;

    Process( Process&& ) = default;
    Process& operator =( Process&& ) = default;

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="pid">Process ID</param>
    /// <param name="access">Access mask</param>
    BLACKBONE_API void Attach( DWORD pid, DWORD access = DEFAULT_ACCESS_P );

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="name">Process name</param>
    /// <param name="access">Access mask</param>
    BLACKBONE_API void Attach( const wchar_t* name, DWORD access = DEFAULT_ACCESS_P );

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="pid">Process handle</param>
    BLACKBONE_API void Attach( HANDLE hProc );

    /// <summary>
    /// Create new process and attach to it
    /// </summary>
    /// <param name="path">Executable path</param>
    /// <param name="suspended">Leave process in suspended state. To resume process one should resume its main thread</param>
    /// <param name="forceInit">If 'suspended' is true, this flag will enforce process initialization via second thread</param>
    /// <param name="cmdLine">Process command line</param>
    /// <param name="currentDir">Startup directory</param>
    /// <param name="pStartup">Additional startup params</param>
    BLACKBONE_API void CreateAndAttach(
        const std::wstring& path,
        bool suspended = false,
        bool forceInit = true,
        const std::wstring& cmdLine = L"",
        const wchar_t* currentDir = nullptr,
        STARTUPINFOW* pStartup = nullptr
    );

    // Syntax sugar for CreateAndAttach
    BLACKBONE_API static Process CreateNew(
        const std::wstring& path,
        bool suspended = false,
        bool forceInit = true,
        const std::wstring& cmdLine = L"",
        const wchar_t* currentDir = nullptr,
        STARTUPINFOW* pStartup = nullptr
    );

    /// <summary>
    /// Attach to current process
    /// </summary>
    BLACKBONE_API static Process CurrentProcess();

    /// <summary>
    /// Detach form current process, if any
    /// </summary>
    BLACKBONE_API void Detach();

    /// <summary>
    /// Ensure LdrInitializeProcess gets called
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS EnsureInit();

    /// <summary>
    /// Suspend process
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Suspend();

    /// <summary>
    /// Resume process
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Resume();

    /// <summary>
    /// Get process ID
    /// </summary>
    /// <returns>Process ID</returns>
    BLACKBONE_API DWORD pid() const { return _core.pid(); }

    /// <summary>
    /// Checks if process still exists
    /// </summary>
    /// <returns>true if process is valid and exists</returns>
    BLACKBONE_API bool valid();

    /// <summary>
    /// Terminate process
    /// </summary>
    /// <param name="code">Exit code</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Terminate( uint32_t code = 0 );

    /// <summary>
    /// Enumerate all open handles
    /// </summary>
    /// <returns>Found handles</returns>
    BLACKBONE_API std::vector<HandleInfo> EnumHandles();

    /// <summary>
    /// Search for process by executable name
    /// </summary>
    /// <param name="name">Process name. If empty - function will retrieve all existing processes</param>
    /// <returns">Found processes</returns>
    BLACKBONE_API static std::vector<DWORD> EnumByName( const std::wstring& name );

    /// <summary>
    /// Search for process by executable name or by process ID
    /// </summary>
    /// <param name="pid">Target process ID. If empty - function will retrieve all existing processes</param>
    /// <param name="name">Process executable name. If empty - function will retrieve all existing processes</param>
    /// <param name="includeThreads">If set to true, function will retrieve info about process threads</param>
    /// <returns">Found processes</returns>
    BLACKBONE_API static std::vector<ProcessInfo> EnumByNameOrPID(
        uint32_t pid,
        const std::wstring& name,
        bool includeThreads = false
    );

    //
    // Subroutines
    //
    BLACKBONE_API ProcessCore&     core()       { return _core;       }  // Core routines and native 
    BLACKBONE_API ProcessMemory&   memory()     { return _memory;     }  // Memory manipulations
    BLACKBONE_API ProcessModules&  modules()    { return _modules;    }  // Module management
    BLACKBONE_API ProcessThreads&  threads()    { return _threads;    }  // Threads
    BLACKBONE_API RemoteHook&      hooks()      { return _hooks;      }  // Hooking code remotely
    BLACKBONE_API RemoteLocalHook& localHooks() { return _localHooks; }  // Hooking code locally
    BLACKBONE_API RemoteExec&      remote()     { return _remote;     }  // Remote code execution
    BLACKBONE_API MMap&            mmap()       { return _mmap;       }  // Manual module mapping
    BLACKBONE_API NtLdr&           nativeLdr()  { return _nativeLdr;  }  // Native loader routines

    // Sugar
    BLACKBONE_API const Wow64Barrier& barrier() const { return _core._native->GetWow64Barrier(); }

private:
    ProcessCore     _core;          // Core routines and native subsystem
    ProcessModules  _modules;       // Module management
    ProcessMemory   _memory;        // Memory manipulations
    ProcessThreads  _threads;       // Threads
    RemoteHook      _hooks;         // Hooking code remotely
    RemoteLocalHook _localHooks;    // In-process remote hooks
    RemoteExec      _remote;        // Remote code execution
    MMap            _mmap;          // Manual module mapping
    NtLdr           _nativeLdr;     // Native loader routines
};

}
