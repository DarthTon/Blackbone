#include "Process.h"
#include "NameResolve.h"

#include <memory>

namespace blackbone
{

Process::Process()
    : _core()
    , _memory( _core )
    , _threads( _core )
    , _hooks( _memory )
    , _modules( *this )
    , _remote( *this )
    , _nativeLdr( *this )
    , _mmap( *this )
{
    GrantPriviledge( SE_DEBUG_NAME );
    GrantPriviledge( SE_LOAD_DRIVER_NAME );
    GrantPriviledge( SE_LOCK_MEMORY_NAME );

    NameResolve::Instance().Initialize();
}

Process::~Process(void)
{
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="pid">Process ID</param>
/// <param name="access">Access mask</param>
/// <returns>Status</returns>
NTSTATUS Process::Attach( DWORD pid, DWORD access /*= DEFAULT_ACCESS_P*/ )
{
    // Reset data
    _modules.reset();
    _remote.reset();
    _mmap.reset();
    _hooks.reset();

    auto res = _core.Open( pid, access );

    if (res == STATUS_SUCCESS)
    {
        _nativeLdr.Init();
        _remote.CreateRPCEnvironment( true );
    }

    return res;
}

/// <summary>
/// Checks if process still exists
/// </summary>
/// <returns></returns>
bool Process::valid()
{
    DWORD dwExitCode = 0;

    if (!GetExitCodeProcess( _core.handle(), &dwExitCode ))
        return false;

    return (dwExitCode == STILL_ACTIVE);
}

/// <summary>
/// Grant current process arbitrary privilege
/// </summary>
/// <param name="name">Privilege name</param>
/// <returns>Status</returns>
NTSTATUS Process::GrantPriviledge( const std::wstring& name )
{
    TOKEN_PRIVILEGES Priv, PrivOld;
    DWORD cbPriv = sizeof(PrivOld);
    HANDLE hToken;

    if (!OpenThreadToken( GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, FALSE, &hToken ))
    {
        if (GetLastError() != ERROR_NO_TOKEN)
            return LastNtStatus();

        if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken ))
            return LastNtStatus();
    }

    Priv.PrivilegeCount = 1;
    Priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    LookupPrivilegeValue( NULL, name.c_str(), &Priv.Privileges[0].Luid );

    if (!AdjustTokenPrivileges( hToken, FALSE, &Priv, sizeof(Priv), &PrivOld, &cbPriv ))
    {
        CloseHandle( hToken );
        return LastNtStatus();
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        CloseHandle( hToken );
        return LastNtStatus();
    }
    
    return STATUS_SUCCESS;
}

/// <summary>
/// Search for process by executable name
/// </summary>
/// <param name="name">Process name. If empty - function will retrieve all existing processes</param>
/// <param name="found">Found processses</param>
void Process::EnumByName( const std::wstring& name, std::vector<DWORD>& found )
{
    HANDLE hProcSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

    if (hProcSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W tEntry = { 0 };
        tEntry.dwSize = sizeof(PROCESSENTRY32W);

        // Iterate threads
        for (BOOL success = Process32FirstW( hProcSnap, &tEntry );
              success == TRUE; 
              success = Process32NextW( hProcSnap, &tEntry ))
        {
            if (name.empty() || _wcsicmp( tEntry.szExeFile, name.c_str() ) == 0)
                found.emplace_back( tEntry.th32ProcessID );
        }

        CloseHandle( hProcSnap );
    }
}


}
