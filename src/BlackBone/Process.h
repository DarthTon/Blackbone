#pragma once

#include "ProcessCore.h"
#include "ProcessMemory.h"
#include "ProcessModules.h"
#include "Threads.h"
#include "RemoteExec.h"
#include "RemoteHook.h"
#include "NtLoader.h"
#include "MMap.h"

#include "NativeStructures.h"

#include <string>
#include <list>

namespace blackbone
{

#define DEFAULT_ACCESS_P  PROCESS_QUERY_INFORMATION | \
                          PROCESS_VM_READ           | \
                          PROCESS_VM_WRITE          | \
                          PROCESS_VM_OPERATION      | \
                          PROCESS_CREATE_THREAD     | \
                          PROCESS_SET_QUOTA         | \
                          PROCESS_TERMINATE         | \
                          PROCESS_DUP_HANDLE
class Process
{
public:
    Process();
    ~Process(void);

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="pid">Process ID</param>
    /// <param name="access">Access mask</param>
    /// <returns>Status</returns>
    NTSTATUS Attach( DWORD pid, DWORD access = DEFAULT_ACCESS_P );

    /// <summary>
    /// Get process ID
    /// </summary>
    /// <returns>Process ID</returns>
    inline DWORD pid() const { return _core.pid(); }

    /// <summary>
    /// Checks if process still exists
    /// </summary>
    /// <returns></returns>
    bool valid();

    /// <summary>
    /// Search for process by executable name
    /// </summary>
    /// <param name="name">Process name. If empty - function will retrieve all existing processes</param>
    /// <param name="found">Found processses</param>
    static void EnumByName( const std::wstring& name, std::vector<DWORD>& found );

    //
    // Subroutines
    //
    ProcessCore&    core()      { return _core; }       // Core routines and native 
    ProcessMemory&  memory()    { return _memory; }     // Memory manipulations
    ProcessThreads& threads()   { return _threads; }    // Threads
    RemoteHook&     hooks()     { return _hooks; }      // Hooking code remotely
    RemoteExec&     remote()    { return _remote; }     // Remote code execution
    ProcessModules& modules()   { return _modules; }    // Module management
    MMap&           mmap()      { return _mmap; }       // Manual module mapping
    NtLdr&          nativeLdr() { return _nativeLdr; }  // Native loader routines

private:

    NTSTATUS GrantPriviledge( const std::wstring& name );

    Process(const Process&) = delete;
    Process& operator =(const Process&) = delete;

private:
    ProcessCore    _core;       // Core routines and native subsystem
    ProcessMemory  _memory;     // Memory manipulations
    ProcessThreads _threads;    // Threads
    RemoteHook     _hooks;      // Hooking code remotely
    RemoteExec     _remote;     // Remote code execution
    ProcessModules _modules;    // Module management
    MMap           _mmap;       // Manual module mapping
    NtLdr          _nativeLdr;  // Native loader routines
};

}
