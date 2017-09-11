#include "Process.h"
#include "../Misc/NameResolve.h"
#include "../Misc/DynImport.h"

#include <memory>

namespace blackbone
{
#define SystemHandleInformation (SYSTEM_INFORMATION_CLASS)16
#define ObjectNameInformation   (OBJECT_INFORMATION_CLASS)1

Process::Process()
    : _core()
    , _modules( *this )
    , _memory( this )
    , _threads( _core )
    , _hooks( _memory )
    , _localHooks( *this )
    , _remote( *this )
    , _mmap( *this )
    , _nativeLdr( *this )
{
    // Ensure InitOnce is called
    InitializeOnce();
}

Process::~Process(void)
{
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="pid">Process ID</param>
/// <param name="access">Access mask</param>
/// <returns>Status code</returns>
NTSTATUS Process::Attach( DWORD pid, DWORD access /*= DEFAULT_ACCESS_P*/ )
{
    Detach();
    return _core.Open( pid, access );
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="pid">Process handle</param>
/// <returns>Status code</returns>
NTSTATUS Process::Attach( HANDLE hProc )
{
    Detach();
    return _core.Open( hProc );
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="name">Process name</param>
/// <param name="access">Access mask</param>
/// <returns>Status code</returns>
NTSTATUS Process::Attach( const wchar_t* name, DWORD access /*= DEFAULT_ACCESS_P*/ )
{
    auto pids = EnumByName( name );
    return pids.empty() ? STATUS_NOT_FOUND : Attach( pids.front(), access );
}

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
NTSTATUS Process::CreateAndAttach( 
    const std::wstring& path, 
    bool suspended /*= false*/,
    bool forceInit /*= true*/,
    const std::wstring& cmdLine /*= L""*/,
    const wchar_t* currentDir /*= nullptr*/,
    STARTUPINFOW* pStartup /*= nullptr*/
    )
{
    Detach();

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    if (!pStartup)
        pStartup = &si;

    if (!CreateProcessW(
        path.c_str(), const_cast<LPWSTR>(cmdLine.c_str()), 
        NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, 
        currentDir, pStartup, &pi
        ))
    {
        return LastNtStatus();
    } 

    // Get handle ownership
    auto status = _core.Open( pi.hProcess );
    if (NT_SUCCESS( status ))
    {
        // Check if process must be left in suspended mode
        if (suspended)
        {
            // Create new thread to make sure LdrInitializeProcess gets called
            if (forceInit)
                EnsureInit();
        }
        else
            ResumeThread( pi.hThread );
    }

    // Close unneeded handles
    CloseHandle( pi.hThread );

    return status;
}

/// <summary>
/// Detach form current process, if any
/// </summary>
/// <returns>Status code</returns>
NTSTATUS Process::Detach()
{
    // Reset data
    _memory.reset();
    _modules.reset();
    _remote.reset();
    _mmap.reset();
    _threads.reset();
    _hooks.reset();
    _core.Close();

    return STATUS_SUCCESS;
}

/// <summary>
/// Ensure LdrInitializeProcess gets called
/// </summary>
/// <returns>Status code</returns>
NTSTATUS Process::EnsureInit()
{
    auto pProc = _modules.GetNtdllExport( "NtYieldExecution", mt_default, Sections );
    if (pProc)
        return _remote.ExecDirect( pProc->procAddress, 0 );

    return STATUS_NOT_FOUND;
}

/// <summary>
/// Suspend process
/// </summary>
/// <returns>Status code</returns>
NTSTATUS Process::Suspend()
{
    return SAFE_NATIVE_CALL( NtSuspendProcess, _core._hProcess );
}

/// <summary>
/// Resume process
/// </summary>
/// <returns>Status code</returns>
NTSTATUS Process::Resume()
{
    return SAFE_NATIVE_CALL( NtResumeProcess, _core._hProcess );
}

/// <summary>
/// Checks if process still exists
/// </summary>
/// <returns></returns>
bool Process::valid()
{
    DWORD dwExitCode = 0;

    if (!_core.handle() || !GetExitCodeProcess( _core.handle(), &dwExitCode ))
        return false;

    return (dwExitCode == STILL_ACTIVE);
}

/// <summary>
/// Terminate process
/// </summary>
/// <param name="code">Exit code</param>
/// <returns>Stratus code</returns>
NTSTATUS Process::Terminate( uint32_t code /*= 0*/ )
{
    TerminateProcess( _core.handle(), code );
    return LastNtStatus();
}

/// <summary>
/// Enumerate all open handles
/// </summary>
/// <returns>Found handles or status code</returns>
call_result_t<std::vector<HandleInfo>> Process::EnumHandles()
{
    ULONG bufSize = 0x10000;
    uint8_t* buffer = (uint8_t*)VirtualAlloc( NULL, bufSize, MEM_COMMIT, PAGE_READWRITE );
    ULONG returnLength = 0;
    std::vector<HandleInfo> handles;

    // Query handle list
    NTSTATUS status = SAFE_NATIVE_CALL( NtQuerySystemInformation, SystemHandleInformation, buffer, bufSize, &returnLength );
    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        bufSize *= 2;
        VirtualFree( buffer, 0, MEM_RELEASE );
        buffer = (uint8_t*)VirtualAlloc( NULL, bufSize, MEM_COMMIT, PAGE_READWRITE );

        status = SAFE_NATIVE_CALL( NtQuerySystemInformation, SystemHandleInformation, buffer, bufSize, &returnLength );
    }

    if (!NT_SUCCESS( status ))
    {
        VirtualFree( buffer, 0, MEM_RELEASE );
        return status;
    }

    SYSTEM_HANDLE_INFORMATION_T* handleInfo = (SYSTEM_HANDLE_INFORMATION_T*)buffer;
    for (ULONG i = 0; i < handleInfo->HandleCount; i++)
    {
        HandleInfo info;
        Handle hLocal;
        OBJECT_TYPE_INFORMATION_T* pTypeInfo = nullptr;
        PVOID pNameInfo = nullptr;
        UNICODE_STRING objectName = { 0 };

        // Filter process
        if (handleInfo->Handles[i].ProcessId != _core._pid)
            continue;

        // Get local handle copy
        status = SAFE_NATIVE_CALL(
            NtDuplicateObject,
            _core._hProcess,
            reinterpret_cast<HANDLE>(handleInfo->Handles[i].Handle),
            GetCurrentProcess(),
            &hLocal, 0, 0, DUPLICATE_SAME_ACCESS
            );

        if (!NT_SUCCESS( status ))
            continue;

        // 
        // Get type information
        //
        pTypeInfo = (OBJECT_TYPE_INFORMATION_T*)malloc( 0x1000 );
        status = SAFE_NATIVE_CALL( NtQueryObject, hLocal, ObjectTypeInformation, pTypeInfo, 0x1000, nullptr );
        if (!NT_SUCCESS( status ))
        {
            continue;
        }

        //
        // Obtain object name
        //
        pNameInfo = malloc( 0x1000 );
        status = SAFE_NATIVE_CALL( NtQueryObject, hLocal, ObjectNameInformation, pNameInfo, 0x1000, &returnLength);
        if (!NT_SUCCESS( status ))
        {
            pNameInfo = realloc( pNameInfo, returnLength );
            status = SAFE_NATIVE_CALL( NtQueryObject, hLocal, ObjectNameInformation, pNameInfo, returnLength, nullptr );
            if (!NT_SUCCESS( status ))
            {
                free( pTypeInfo );
                free( pNameInfo );
                continue;
            }
        }

        objectName = *(PUNICODE_STRING)pNameInfo;

        //
        // Fill handle info structure
        //
        info.handle   = reinterpret_cast<HANDLE>(handleInfo->Handles[i].Handle);
        info.access   = handleInfo->Handles[i].GrantedAccess;
        info.flags    = handleInfo->Handles[i].Flags;
        info.pObject  = handleInfo->Handles[i].Object;

        if (pTypeInfo->Name.Length)
            info.typeName = (wchar_t*)pTypeInfo->Name.Buffer;

        if (objectName.Length)
            info.name = objectName.Buffer;

        //
        // Type-specific info
        //
        if (_wcsicmp( info.typeName.c_str(), L"Section" ) == 0)
        {
            SECTION_BASIC_INFORMATION_T secInfo = { 0 };

            status = SAFE_NATIVE_CALL( NtQuerySection, hLocal, SectionBasicInformation, &secInfo, (ULONG)sizeof( secInfo ), nullptr );
            if (NT_SUCCESS( status ))
            {
                info.section.reset( new SectionInfo() );

                info.section->size = secInfo.Size.QuadPart;
                info.section->attrib = secInfo.Attributes;
            }
        }

        handles.emplace_back( info );

        free( pTypeInfo );
        free( pNameInfo );
    }

    VirtualFree( buffer, 0, MEM_RELEASE );
    return call_result_t<std::vector<HandleInfo>>( handles, status );
}

/// <summary>
/// Search for process by executable name
/// </summary>
/// <param name="name">Process name. If empty - function will retrieve all existing processes</param>
/// <param name="found">Found processses</param>
std::vector<DWORD> Process::EnumByName( const std::wstring& name )
{
    std::vector<DWORD> found;
    auto hProcSnap = SnapHandle( CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ) );
    if (!hProcSnap)
        return found;

    PROCESSENTRY32W tEntry = { 0 };
    tEntry.dwSize = sizeof( PROCESSENTRY32W );

    // Iterate threads
    for (BOOL success = Process32FirstW( hProcSnap, &tEntry );
        success != FALSE;
        success = Process32NextW( hProcSnap, &tEntry ))
    {
        if (name.empty() || _wcsicmp( tEntry.szExeFile, name.c_str() ) == 0)
            found.emplace_back( tEntry.th32ProcessID );
    }

    return found;
}

/// <summary>
/// Search for process by executable name or by process ID
/// </summary>
/// <param name="pid">Target process ID. rocess name. If empty - function will retrieve all existing processes</param>
/// <param name="name">Process executable name. If empty - function will retrieve all existing processes</param>
/// <param name="found">Found processses</param>
/// <param name="includeThreads">If set to true, function will retrieve info ablout process threads</param>
/// <returns>Status code</returns>
call_result_t<std::vector<ProcessInfo>> Process::EnumByNameOrPID(
    uint32_t pid,
    const std::wstring& name,
    bool includeThreads /*= false*/
    )
{
    ULONG bufSize = 0x100;
    uint8_t tmpbuf[0x100];
    uint8_t* buffer = tmpbuf;
    ULONG returnLength = 0;
    std::vector<ProcessInfo> found;

    // Query process info
    NTSTATUS status = SAFE_NATIVE_CALL( NtQuerySystemInformation, (SYSTEM_INFORMATION_CLASS)57, buffer, bufSize, &returnLength );
    if (!NT_SUCCESS( status ))
    {
        bufSize = returnLength;
        buffer = (uint8_t*)VirtualAlloc( NULL, bufSize, MEM_COMMIT, PAGE_READWRITE );
        status = SAFE_NATIVE_CALL( NtQuerySystemInformation, (SYSTEM_INFORMATION_CLASS)57, buffer, bufSize, &returnLength );
        if (!NT_SUCCESS( status ))
        {
            VirtualFree( buffer, 0, MEM_RELEASE );
            return status;
        }
    }

    // Parse info
    for (auto pInfo = reinterpret_cast<_SYSTEM_PROCESS_INFORMATION_T<DWORD_PTR>*>(buffer);;)
    {
        //  Skip idle process, compare name or compare pid
        if (pInfo->UniqueProcessId != 0 && ((name.empty() && pid == 0) || _wcsicmp( name.c_str(), (wchar_t*)pInfo->ImageName.Buffer ) == 0 ||  pid == pInfo->UniqueProcessId))
        {
            ProcessInfo info;

            info.pid = static_cast<uint32_t>(pInfo->UniqueProcessId);

            if (pInfo->ImageName.Buffer)
                info.imageName = reinterpret_cast<wchar_t*>(pInfo->ImageName.Buffer);

            // Get threads info
            if (includeThreads)
            {
                int64_t minTime = 0xFFFFFFFFFFFFFFFF;
                ULONG mainIdx = 0;

                for (ULONG i = 0; i < pInfo->NumberOfThreads; i++)
                {
                    ThreadInfo tinfo;
                    auto& thd = pInfo->Threads[i];

                    tinfo.tid = static_cast<uint32_t>(thd.ThreadInfo.ClientId.UniqueThread);
                    tinfo.startAddress = static_cast<uintptr_t>(thd.ThreadInfo.StartAddress);

                    // Check for main thread
                    if (thd.ThreadInfo.CreateTime.QuadPart < minTime)
                    {
                        minTime = thd.ThreadInfo.CreateTime.QuadPart;
                        mainIdx = i;
                    }

                    info.threads.emplace_back( tinfo );
                    info.threads[mainIdx].mainThread = true;
                }
            }

            found.emplace_back( info );
        }

        if (pInfo->NextEntryOffset)
            pInfo = reinterpret_cast<_SYSTEM_PROCESS_INFORMATION_T<DWORD_PTR>*>((uint8_t*)pInfo + pInfo->NextEntryOffset);
        else
            break;        
    }

    // Sort results
    std::sort( found.begin(), found.end() );

    VirtualFree( buffer, 0, MEM_RELEASE );
    return call_result_t<std::vector<ProcessInfo>>( found, status );
}


}
