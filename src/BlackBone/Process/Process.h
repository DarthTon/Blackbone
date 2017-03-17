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
#include "../Include/CallResult.h"
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

    bool operator < (const ProcessInfo& other)
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
    std::shared_ptr<SectionInfo> section;
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
    BLACKBONE_API ~Process(void);

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="pid">Process ID</param>
    /// <param name="access">Access mask</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Attach( DWORD pid, DWORD access = DEFAULT_ACCESS_P );

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="name">Process name</param>
    /// <param name="access">Access mask</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Attach( const wchar_t* name, DWORD access = DEFAULT_ACCESS_P );

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="pid">Process handle</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Attach( HANDLE hProc );

    /// <summary>
    /// Create new process and attach to it
    /// </summary>
    /// <param name="path">Executable path</param>
    /// <param name="suspended">Leave process in suspended state. To resume process one should resume its main thread</param>
    /// <param name="forceInit">If 'suspended' is true, this flag will enforce process initialization via second thread</param>
    /// <param name="cmdLine">Process command line</param>
    /// <param name="currentDir">Startup directory</param>
    /// <param name="pStartup">Additional startup params</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS CreateAndAttach(
        const std::wstring& path,
        bool suspended = false,
        bool forceInit = true,
        const std::wstring& cmdLine = L"",
        const wchar_t* currentDir = nullptr,
        STARTUPINFOW* pStartup = nullptr
        );

    /// <summary>
    /// Detach form current process, if any
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Detach();

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
    BLACKBONE_API inline DWORD pid() const { return _core.pid(); }

    /// <summary>
    /// Checks if process still exists
    /// </summary>
    /// <returns>true if process is valid and exists</returns>
    BLACKBONE_API bool valid();

    /// <summary>
    /// Terminate process
    /// </summary>
    /// <param name="code">Exit code</param>
    /// <returns>Stratus code</returns>
    BLACKBONE_API NTSTATUS Terminate( uint32_t code = 0 );

    /// <summary>
    /// Enumerate all open handles
    /// </summary>
    /// <returns>Found handles or status code</returns>
    BLACKBONE_API call_result_t<std::vector<HandleInfo>> EnumHandles();

    /// <summary>
    /// Search for process by executable name
    /// </summary>
    /// <param name="name">Process name. If empty - function will retrieve all existing processes</param>
    /// <param name="found">Found processses</param>
    BLACKBONE_API static std::vector<DWORD> EnumByName( const std::wstring& name );

    /// <summary>
    /// Search for process by executable name or by process ID
    /// </summary>
    /// <param name="pid">Target process ID. rocess name. If empty - function will retrieve all existing processes</param>
    /// <param name="name">Process executable name. If empty - function will retrieve all existing processes</param>
    /// <param name="found">Found processses</param>
    /// <param name="includeThreads">If set to true, function will retrieve info ablout process threads</param>
    /// <returns>Status code</returns>
    BLACKBONE_API static call_result_t<std::vector<ProcessInfo>> EnumByNameOrPID(
        uint32_t pid,
        const std::wstring& name, 
        bool includeThreads = false
        );

    //
    // Subroutines
    //
    BLACKBONE_API inline ProcessCore&     core()       { return _core;       }  // Core routines and native 
    BLACKBONE_API inline ProcessMemory&   memory()     { return _memory;     }  // Memory manipulations
    BLACKBONE_API inline ProcessModules&  modules()    { return _modules;    }  // Module management
    BLACKBONE_API inline ProcessThreads&  threads()    { return _threads;    }  // Threads
    BLACKBONE_API inline RemoteHook&      hooks()      { return _hooks;      }  // Hooking code remotely
    BLACKBONE_API inline RemoteLocalHook& localHooks() { return _localHooks; }  // Hooking code remotely
    BLACKBONE_API inline RemoteExec&      remote()     { return _remote;     }  // Remote code execution
    BLACKBONE_API inline MMap&            mmap()       { return _mmap;       }  // Manual module mapping
    BLACKBONE_API inline NtLdr&           nativeLdr()  { return _nativeLdr;  }  // Native loader routines

    // Sugar
    BLACKBONE_API inline const Wow64Barrier& barrier() const { return _core._native->GetWow64Barrier(); }

private:
    Process(const Process&) = delete;
    Process& operator =(const Process&) = delete;

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
