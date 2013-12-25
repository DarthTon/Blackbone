#include "ProcessCore.h"
#include "Macro.h"

namespace blackbone
{

ProcessCore::ProcessCore()
    : _native( nullptr )
{
}

ProcessCore::~ProcessCore()
{
    Close();
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="pid">Process ID</param>
/// <param name="access">Access mask</param>
/// <returns>Status</returns>
NTSTATUS ProcessCore::Open( DWORD pid, DWORD access )
{
    // Prevent handle leak
    Close();

    // Handle current process differently
    _hProcess = (pid == GetCurrentProcessId()) ? GetCurrentProcess() : OpenProcess( access, false, pid );

    if (_hProcess != NULL)
    {
        _pid = pid;

        // Detect x86 OS
        SYSTEM_INFO info = { 0 };
        GetNativeSystemInfo( &info );

        if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        {
            _native.reset( new x86Native( _hProcess ) );
        }
        else
        {
            // Detect wow64 barrier
            BOOL wowSrc = FALSE;
            IsWow64Process( GetCurrentProcess(), &wowSrc );

            if (wowSrc == TRUE)
                _native.reset( new NativeWow64( _hProcess ) );
            else
                _native.reset( new Native( _hProcess ) );
        }

        // Get DEP info
        // For native x64 processes DEP is always enabled
        if (_native->GetWow64Barrier().targetWow64 == false)
        {
            _dep = true;
        }
        else
        {
            DWORD flags = 0;
            BOOL perm = 0;

            if (GetProcessDEPPolicy( _hProcess, &flags, &perm ))
                _dep = static_cast<bool>(flags & PROCESS_DEP_ENABLE);
        }

        return STATUS_SUCCESS;
    }

    return LastNtStatus();
}

/// <summary>
/// Close current process handle
/// </summary>
void ProcessCore::Close()
{
    if (_hProcess)
    {
        CloseHandle( _hProcess );

        _hProcess = NULL;
        _pid = 0;
        _native.reset( nullptr );
    }
}

}