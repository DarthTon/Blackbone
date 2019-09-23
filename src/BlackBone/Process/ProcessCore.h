#pragma once

#include "../Include/NativeStructures.h"
#include "../Include/HandleGuard.h"
#include "../Subsystem/Wow64Subsystem.h"
#include "../Subsystem/x86Subsystem.h"

#include <memory>
#include <stdint.h>

namespace blackbone
{

class ProcessCore
{
public:

    /// <summary>
    /// Check if target process is running in WOW64 mode
    /// </summary>
    /// <returns>true if process is WOW64</returns>
    BLACKBONE_API inline bool isWow64() const { return _native->GetWow64Barrier().targetWow64; }

    /// <summary>
    /// Get process handle
    /// </summary>
    /// <returns>Process handle</returns>
    BLACKBONE_API inline HANDLE handle() const { return _hProcess; }

    /// <summary>
    /// Get process ID
    /// </summary>
    /// <returns>Process ID</returns>
    BLACKBONE_API inline DWORD pid() const { return _pid; }

    /// <summary>
    /// Get process data execution prevention state
    /// </summary>
    /// <returns>true if DEP is enabled for process</returns>
    BLACKBONE_API inline bool DEP() const { return _dep; };
  
    /// <summary>
    /// Get system routines
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline Native* native() { return _native.get(); }

    /// <summary>
    /// Get WOW64 PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB32</param>
    /// <returns>PEB pointer</returns>
    BLACKBONE_API inline ptr_t peb32( _PEB32* ppeb = nullptr ) { return _native->getPEB( ppeb ); }

    /// <summary>
    /// Get native PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB64</param>
    /// <returns>PEB pointer</returns>
    BLACKBONE_API inline ptr_t peb64( _PEB64* ppeb = nullptr ) { return _native->getPEB( ppeb ); }

    /// <summary>
    /// Get PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB</param>
    /// <returns>PEB pointer</returns>
    template<typename T = uintptr_t>
    BLACKBONE_API inline ptr_t peb( _PEB_T<T>* ppeb = nullptr ) { return _native->getPEB( ppeb ); }

    /// <summary>
    /// Check if process is a protected process
    /// </summary>
    /// <returns>true if protected</returns>
    BLACKBONE_API bool isProtected();

private:
    friend class Process;
    using ptrNative = std::unique_ptr<Native>;

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
    /// Attach to existing process by handle
    /// </summary>
    /// <param name="pid">Process handle</param>
    /// <returns>Status</returns>
    NTSTATUS Open( HANDLE handle );

    /// <summary>
    /// Initialize some internal data
    /// </summary>
    /// <returns>Status code</returns>
    NTSTATUS Init();

    /// <summary>
    /// Close current process handle
    /// </summary>
    void Close();

private:
    ProcessHandle _hProcess;        // Process handle
    DWORD         _pid = 0;         // Process ID
    ptrNative     _native;          // Api wrapper
    bool          _dep = true;      // DEP state for process
};

}