#include "InitOnce.h"
#include "../Include/Winheaders.h"
#include "../../../contrib/VersionHelpers.h"
#include "../Include/Macro.h"
#include "DynImport.h"
#include "PatternLoader.h"
#include "NameResolve.h"

#include <string>
#include <cassert>

namespace blackbone
{

class InitOnce
{
public:
    static bool Exec()
    {
        if(!_done)
        {
            InitVersion();

            GrantPriviledge( L"SeDebugPrivilege" );
            GrantPriviledge( L"SeLoadDriverPrivilege" );
            LoadFuncs();

            g_PatternLoader.reset( new PatternLoader );
            g_PatternLoader->DoSearch();

            NameResolve::Instance().Initialize();

            _done = true;
        }

        return _done;
    }

private:
    InitOnce() = delete;
    InitOnce( const InitOnce& ) = delete;
    InitOnce& operator=( const InitOnce& ) = delete;

    /// <summary>
    /// Grant current process arbitrary privilege
    /// </summary>
    /// <param name="name">Privilege name</param>
    /// <returns>Status</returns>
    static NTSTATUS GrantPriviledge( const std::wstring& name )
    {
        TOKEN_PRIVILEGES Priv, PrivOld;
        DWORD cbPriv = sizeof( PrivOld );
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
        LookupPrivilegeValueW( NULL, name.c_str(), &Priv.Privileges[0].Luid );

        if (!AdjustTokenPrivileges( hToken, FALSE, &Priv, sizeof( Priv ), &PrivOld, &cbPriv ))
        {
            CloseHandle( hToken );
            return LastNtStatus();
        }

        if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
        {
            CloseHandle( hToken );
            return LastNtStatus();
        }

        CloseHandle( hToken );
        return STATUS_SUCCESS;
    }

    static void LoadFuncs()
    {
        HMODULE hNtdll = GetModuleHandleW( L"ntdll.dll" );
        HMODULE hKernel32 = GetModuleHandleW( L"kernel32.dll" );

        LOAD_IMPORT( "NtQuerySystemInformation",                 hNtdll );
        LOAD_IMPORT( "RtlDosApplyFileIsolationRedirection_Ustr", hNtdll );
        LOAD_IMPORT( "RtlInitUnicodeString",                     hNtdll );
        LOAD_IMPORT( "RtlFreeUnicodeString",                     hNtdll );
        LOAD_IMPORT( "RtlHashUnicodeString",                     hNtdll );
        LOAD_IMPORT( "RtlUpcaseUnicodeChar",                     hNtdll );
        LOAD_IMPORT( "NtQueryInformationProcess",                hNtdll );
        LOAD_IMPORT( "NtSetInformationProcess",                  hNtdll );
        LOAD_IMPORT( "NtQueryInformationThread",                 hNtdll );
        LOAD_IMPORT( "NtDuplicateObject",                        hNtdll );
        LOAD_IMPORT( "NtQueryObject",                            hNtdll );
        LOAD_IMPORT( "NtQuerySection",                           hNtdll );
        LOAD_IMPORT( "RtlCreateActivationContext",               hNtdll );
        LOAD_IMPORT( "NtQueryVirtualMemory",                     hNtdll );
        LOAD_IMPORT( "NtCreateThreadEx",                         hNtdll );
        LOAD_IMPORT( "NtLockVirtualMemory",                      hNtdll );
        LOAD_IMPORT( "NtSuspendProcess",                         hNtdll );
        LOAD_IMPORT( "NtResumeProcess",                          hNtdll );
        LOAD_IMPORT( "RtlImageNtHeader",                         hNtdll );
        LOAD_IMPORT( "NtLoadDriver",                             hNtdll );
        LOAD_IMPORT( "NtUnloadDriver",                           hNtdll );
        LOAD_IMPORT( "RtlDosPathNameToNtPathName_U",             hNtdll );
        LOAD_IMPORT( "NtOpenEvent",                              hNtdll );
        LOAD_IMPORT( "NtCreateEvent",                            hNtdll );
        LOAD_IMPORT( "NtQueueApcThread",                         hNtdll );
        LOAD_IMPORT( "RtlEncodeSystemPointer",                   hNtdll );
        LOAD_IMPORT( "RtlQueueApcWow64Thread",                   hNtdll ); 
        LOAD_IMPORT( "NtWow64QueryInformationProcess64",         hNtdll );
        LOAD_IMPORT( "NtWow64ReadVirtualMemory64",               hNtdll );
        LOAD_IMPORT( "NtWow64WriteVirtualMemory64",              hNtdll );
        LOAD_IMPORT( "Wow64GetThreadContext",                    hKernel32 );
        LOAD_IMPORT( "Wow64SetThreadContext",                    hKernel32 );
        LOAD_IMPORT( "Wow64SuspendThread",                       hKernel32 );
        LOAD_IMPORT( "GetProcessDEPPolicy",                      hKernel32 );
        LOAD_IMPORT( "QueryFullProcessImageNameW",               hKernel32 );
    }

private:
    static bool _done;
};

std::unique_ptr<PatternLoader> g_PatternLoader;

bool InitOnce::_done = false;
const bool g_Initialized = InitOnce::Exec();
}