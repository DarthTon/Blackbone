#include "Process.h"
#include "../Include/Exception.h"
#include "../Misc/NameResolve.h"
#include "../Misc/DynImport.h"

#include <memory>

namespace blackbone
{

#define SystemHandleInformation (SYSTEM_INFORMATION_CLASS)16
#define ObjectNameInformation   (OBJECT_INFORMATION_CLASS)1

Process::Process()
    : _core()
    , _modules( this )
    , _memory( this )
    , _threads( &_core )
    , _hooks( &_memory )
    , _localHooks( this )
    , _remote( this )
    , _mmap( this )
    , _nativeLdr( this )
{
    // Ensure InitOnce is called
    InitializeOnce();
}

Process::Process( DWORD pid, DWORD access /*= DEFAULT_ACCESS_P */ )
    : Process()
{
    Attach( pid, access );
}

Process::Process( const wchar_t* name, DWORD access /*= DEFAULT_ACCESS_P */ )
    : Process()
{
    Attach( name, access );
}

Process::Process( HANDLE hProc )
    : Process()
{
    Attach( hProc );
}

Process::Process(
    const std::wstring& path,
    bool suspended,
    bool forceInit,
    const std::wstring& cmdLine,
    const wchar_t* currentDir,
    STARTUPINFOW* pStartup
)
    : Process()
{
    CreateAndAttach( path, suspended, forceInit, cmdLine, currentDir, pStartup );
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="pid">Process ID</param>
/// <param name="access">Access mask</param>
void Process::Attach( DWORD pid, DWORD access /*= DEFAULT_ACCESS_P*/ )
{
    Detach();
    _core.Open( pid, access );
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="pid">Process handle</param>
void Process::Attach( HANDLE hProc )
{
    Detach();
    _core.Open( hProc );
}

/// <summary>
/// Attach to existing process
/// </summary>
/// <param name="name">Process name</param>
/// <param name="access">Access mask</param>
void Process::Attach( const wchar_t* name, DWORD access /*= DEFAULT_ACCESS_P*/ )
{
    auto pids = EnumByName( name );
    if (pids.empty())
        THROW_AND_LOG( "process %ls does not exist", name );

    Attach( pids.front(), access );
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
void Process::CreateAndAttach(
    const std::wstring& path,
    bool suspended /*= false*/,
    bool forceInit /*= true*/,
    const std::wstring& cmdLine /*= L""*/,
    const wchar_t* currentDir /*= nullptr*/,
    STARTUPINFOW* pStartup /*= nullptr*/
)
{
    Detach();

    STARTUPINFOW si = { };
    PROCESS_INFORMATION pi = { };
    if (!pStartup)
        pStartup = &si;

    if (!CreateProcessW(
        path.c_str(), const_cast<LPWSTR>(cmdLine.c_str()),
        nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr,
        currentDir, pStartup, &pi
    ))
    {
        THROW_WITH_STATUS_AND_LOG( LastNtStatus(), "failed to create process '%ls'", path.c_str() );
    }

    // Get handle ownership
    _core.Open( pi.hProcess );

    // Check if process must be left in suspended mode
    if (suspended)
    {
        // Create new thread to make sure LdrInitializeProcess gets called
        if (forceInit)
            EnsureInit();
    }
    else
        ResumeThread( pi.hThread );

    // Close unneeded handles
    CloseHandle( pi.hThread );
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
/// <returns>Process object</returns>
Process Process::CreateNew(
    const std::wstring& path,
    bool suspended /*= false*/,
    bool forceInit /*= true*/,
    const std::wstring& cmdLine /*= L""*/,
    const wchar_t* currentDir /*= nullptr*/,
    STARTUPINFOW* pStartup /*= nullptr */
)
{
    return Process( path, suspended, forceInit, cmdLine, currentDir, pStartup );
}

/// <summary>
/// Attach to current process
/// </summary>
Process Process::CurrentProcess()
{
    return Process( GetCurrentProcessId() );
}

/// <summary>
/// Detach form current process, if any
/// </summary>
void Process::Detach()
{
    // Reset data
    _memory.reset();
    _modules.reset();
    _remote.reset();
    _mmap.reset();
    _hooks.reset();
    _core.Close();
}

/// <summary>
/// Ensure LdrInitializeProcess gets called
/// </summary>
/// <returns>Status code</returns>
NTSTATUS Process::EnsureInit()
{
    auto pProc = _modules.GetNtdllExport( "NtYieldExecution", mt_default, Sections );
    return _remote.ExecDirect( pProc.procAddress, 0 );
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
/// <returns>true if process is valid and exists</returns>
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
/// <returns>Status code</returns>
NTSTATUS Process::Terminate( uint32_t code /*= 0*/ )
{
    TerminateProcess( _core.handle(), code );
    return LastNtStatus();
}

/// <summary>
/// Enumerate all open handles
/// </summary>
/// <returns>Found handles</returns>
std::vector<HandleInfo> Process::EnumHandles()
{
    ULONG bufSize = 0x10000;
    ULONG returnLength = 0;
    std::vector<HandleInfo> handles;
    auto buffer = make_raw_ptr( bufSize );

    // Query handle list
    NTSTATUS status = SAFE_NATIVE_CALL( NtQuerySystemInformation, SystemHandleInformation, buffer.get(), bufSize, &returnLength );
    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        bufSize *= 2;
        buffer = make_raw_ptr( bufSize );
        status = SAFE_NATIVE_CALL( NtQuerySystemInformation, SystemHandleInformation, buffer.get(), bufSize, &returnLength );
    }

    if (!NT_SUCCESS( status ))
        THROW_WITH_STATUS_AND_LOG( status, "NtQuerySystemInformation failed" );

    auto handleInfo = static_cast<const SYSTEM_HANDLE_INFORMATION_T*>(buffer.get());
    for (ULONG i = 0; i < handleInfo->HandleCount; i++)
    {
        HandleInfo info;
        ProcessHandle hLocal;
        std::unique_ptr<OBJECT_TYPE_INFORMATION_T, decltype(&free)> typeInfo( nullptr, &free );
        std::unique_ptr<UNICODE_STRING, decltype(&free)> nameInfo( nullptr, &free );

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
        typeInfo.reset( static_cast<OBJECT_TYPE_INFORMATION_T*>(malloc( 0x1000 )) );
        status = SAFE_NATIVE_CALL( NtQueryObject, hLocal, ObjectTypeInformation, typeInfo.get(), 0x1000, nullptr );
        if (!NT_SUCCESS( status ))
            continue;

        //
        // Obtain object name
        //
        nameInfo.reset( static_cast<UNICODE_STRING*>(malloc( 0x1000 )) );
        status = SAFE_NATIVE_CALL( NtQueryObject, hLocal, ObjectNameInformation, nameInfo.get(), 0x1000, &returnLength );
        if (!NT_SUCCESS( status ))
        {
            auto old = nameInfo.release();
            nameInfo.reset( static_cast<UNICODE_STRING*>(realloc( old, returnLength )) );
            status = SAFE_NATIVE_CALL( NtQueryObject, hLocal, ObjectNameInformation, nameInfo.get(), returnLength, nullptr );
            if (!NT_SUCCESS( status ))
                continue;
        }

        //
        // Fill handle info structure
        //
        info.handle = reinterpret_cast<HANDLE>(handleInfo->Handles[i].Handle);
        info.access = handleInfo->Handles[i].GrantedAccess;
        info.flags = handleInfo->Handles[i].Flags;
        info.pObject = handleInfo->Handles[i].Object;

        if (typeInfo->Name.Length)
            info.typeName = reinterpret_cast<wchar_t*>(typeInfo->Name.Buffer);

        if (nameInfo->Buffer)
            info.name = nameInfo->Buffer;

        //
        // Type-specific info
        //
        if (_wcsicmp( info.typeName.c_str(), L"Section" ) == 0)
        {
            SECTION_BASIC_INFORMATION_T secInfo = { };

            status = SAFE_NATIVE_CALL( NtQuerySection, hLocal, SectionBasicInformation, &secInfo, (ULONG)sizeof( secInfo ), nullptr );
            if (NT_SUCCESS( status ))
            {
                info.section.size = secInfo.Size.QuadPart;
                info.section.attrib = secInfo.Attributes;
            }
        }

        handles.emplace_back( info );
    }

    return handles;
}

/// <summary>
/// Search for process by executable name
/// </summary>
/// <param name="name">Process name. If empty - function will retrieve all existing processes</param>
/// <returns">Found processes</returns>
std::vector<DWORD> Process::EnumByName( const std::wstring& name )
{
    std::vector<DWORD> found;
    auto hProcSnap = Handle( CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ) );
    if (!hProcSnap)
        return found;

    PROCESSENTRY32W tEntry = { };
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
/// <param name="pid">Target process ID. If empty - function will retrieve all existing processes</param>
/// <param name="name">Process executable name. If empty - function will retrieve all existing processes</param>
/// <param name="includeThreads">If set to true, function will retrieve info about process threads</param>
/// <returns">Found processes</returns>
std::vector<ProcessInfo> Process::EnumByNameOrPID( uint32_t pid, const std::wstring& name, bool includeThreads /*= false*/ )
{
    ULONG bufSize = 0x100;
    uint8_t tmpbuf[0x100];
    auto buffer = make_raw_ptr( tmpbuf );
    ULONG returnLength = 0;
    std::vector<ProcessInfo> found;

    // Query process info
    NTSTATUS status = SAFE_NATIVE_CALL( NtQuerySystemInformation, SYSTEM_INFORMATION_CLASS( 57 ), buffer.get(), bufSize, &returnLength );
    if (!NT_SUCCESS( status ))
    {
        bufSize = returnLength;
        buffer.reset( VirtualAlloc( nullptr, bufSize, MEM_COMMIT, PAGE_READWRITE ) );
        status = SAFE_NATIVE_CALL( NtQuerySystemInformation, SYSTEM_INFORMATION_CLASS( 57 ), buffer.get(), bufSize, &returnLength );
        if (!NT_SUCCESS( status ))
            THROW_WITH_STATUS_AND_LOG( status, "NtQuerySystemInformation failed" );
    }

    using info_t = _SYSTEM_PROCESS_INFORMATION_T<DWORD_PTR>;

    // Parse info
    for (auto pInfo = static_cast<info_t*>(buffer.get());;)
    {
        //  Skip idle process, compare name or compare pid
        if (pInfo->UniqueProcessId != 0 && ((name.empty() && pid == 0) ||
            _wcsicmp( name.c_str(), (wchar_t*)pInfo->ImageName.Buffer ) == 0 ||
            pid == pInfo->UniqueProcessId))
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
            pInfo = reinterpret_cast<info_t*>(reinterpret_cast<uint8_t*>(pInfo) + pInfo->NextEntryOffset);
        else
            break;
    }

    // Sort results
    std::sort( found.begin(), found.end() );
    return found;
}

}
