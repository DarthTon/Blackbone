#pragma once

#include "Winheaders.h"
#include "NativeStructures.h"
#include "Wow64Subsystem.h"
#include "x86Subsystem.h"

#include <memory>
#include <stdint.h>

namespace blackbone
{

class ProcessCore
{
    friend class Process;

    typedef std::unique_ptr<Native> ptrNative;

public:

    /// <summary>
    /// Check if target process is running in WOW64 mode
    /// </summary>
    /// <returns>true if process is WOW64</returns>
    inline bool isWow64() const { return _native->GetWow64Barrier().targetWow64; }

    /// <summary>
    /// Get process handle
    /// </summary>
    /// <returns>Process handle</returns>
    inline HANDLE handle() const { return _hProcess; }

    /// <summary>
    /// Get process ID
    /// </summary>
    /// <returns>Process ID</returns>
    inline DWORD pid() const { return _pid; }

    /// <summary>
    /// Get process data execution prevention state
    /// </summary>
    /// <returns>true if DEP is enabled for process</returns>
    inline bool DEP() const { return _dep; };
  
    /// <summary>
    /// Get system routines
    /// </summary>
    /// <returns></returns>
    inline Native* native() { return _native.get(); }

    /// <summary>
    /// Get WOW64 PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB</param>
    /// <returns>PEB pointer</returns>
    inline ptr_t peb( _PEB32* ppeb ) { return _native->getPEB( ppeb ); }

    /// <summary>
    /// Get native PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB</param>
    /// <returns>PEB pointer</returns>
    inline ptr_t peb( _PEB64* ppeb ) { return _native->getPEB( ppeb ); }

    /// <summary>
    /// Get PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB</param>
    /// <returns>PEB pointer</returns>
    inline ptr_t peb() { return peb( (PEB_T*)nullptr ); }

private:
     ProcessCore();
     ProcessCore( const ProcessCore& ) = delete;
    ~ProcessCore();

    /// <summary>
    /// Attach to existing process
    /// </summary>
    /// <param name="pid">Process ID</param>
    /// <param name="access">Access mask</param>
    /// <returns>Status</returns>
    NTSTATUS Open( DWORD pid, DWORD access );

    /// <summary>
    /// Close current process handle
    /// </summary>
    void Close();

private:
    HANDLE    _hProcess = NULL; // Process handle
    DWORD     _pid = 0;         // Process ID
    ptrNative _native;          // Api wrapper
    bool      _dep = false;     // DEP state for process
};

}