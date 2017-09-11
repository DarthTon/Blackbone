#include "NameResolve.h"
#include "../Include/NativeStructures.h"
#include "../Include/Macro.h"
#include "../Misc/Utils.h"
#include "../Misc/DynImport.h"
#include "../Process/Process.h"

#include <stdint.h>
#include <algorithm>
#include <VersionHelpers.h>

namespace blackbone
{

NameResolve& NameResolve::Instance()
{
    static NameResolve instance;
    return instance;
}

/// <summary>
/// Initialize api set map
/// </summary>
/// <returns></returns>
bool NameResolve::Initialize()
{
    if (IsWindows10OrGreater())
        return InitializeP< PAPI_SET_NAMESPACE_ARRAY_10, 
                            PAPI_SET_NAMESPACE_ENTRY_10,
                            PAPI_SET_VALUE_ARRAY_10,
                            PAPI_SET_VALUE_ENTRY_10 >();
    else if (IsWindows8Point1OrGreater())
        return InitializeP< PAPI_SET_NAMESPACE_ARRAY, 
                            PAPI_SET_NAMESPACE_ENTRY,
                            PAPI_SET_VALUE_ARRAY,
                            PAPI_SET_VALUE_ENTRY >();
    else if (IsWindows7OrGreater())
        return InitializeP< PAPI_SET_NAMESPACE_ARRAY_V2, 
                            PAPI_SET_NAMESPACE_ENTRY_V2,
                            PAPI_SET_VALUE_ARRAY_V2,
                            PAPI_SET_VALUE_ENTRY_V2 >();
    else
        return true;
}

/// <summary>
/// OS dependent api set initialization
/// </summary>
/// <returns>true on success</returns>
template<typename PApiSetMap, typename PApiSetEntry, typename PHostArray, typename PHostEntry>
bool blackbone::NameResolve::InitializeP()
{
    if (!_apiSchema.empty())
        return true;

    PEB_T *ppeb = reinterpret_cast<PEB_T*>(reinterpret_cast<TEB_T*>(NtCurrentTeb())->ProcessEnvironmentBlock);
    PApiSetMap pSetMap = reinterpret_cast<PApiSetMap>(ppeb->ApiSetMap);

    for (DWORD i = 0; i < pSetMap->Count; i++)
    {
        PApiSetEntry pDescriptor = pSetMap->entry(i);

        std::vector<std::wstring> vhosts;
        wchar_t dllName[MAX_PATH] = { 0 };

        auto nameSize = pSetMap->apiName( pDescriptor, dllName );
        std::transform( dllName, dllName + nameSize / sizeof( wchar_t ), dllName, ::towlower );

        PHostArray pHostData = pSetMap->valArray( pDescriptor );

        for (DWORD j = 0; j < pHostData->Count; j++)
        { 
            PHostEntry pHost = pHostData->entry( pSetMap, j );
            std::wstring hostName( 
                reinterpret_cast<wchar_t*>(reinterpret_cast<uint8_t*>(pSetMap) + pHost->ValueOffset), 
                pHost->ValueLength / sizeof( wchar_t ) 
            );

            if (!hostName.empty())
                vhosts.emplace_back( std::move( hostName ) );
        }

        _apiSchema.emplace( dllName, std::move( vhosts ) );
    }

    return true;
}

/// <summary>
/// Resolve image path.
/// </summary>
/// <param name="path">Image to resolve</param>
/// <param name="baseName">Name of parent image. Used only when resolving import images</param>
/// <param name="searchDir">Directory where source image is located</param>
/// <param name="flags">Resolve flags</param>
/// <param name="procID">Process ID. Used to search process executable directory</param>
/// <param name="actx">Activation context</param>
/// <returns>Status</returns>
NTSTATUS NameResolve::ResolvePath( 
    std::wstring& path, 
    const std::wstring& baseName,
    const std::wstring& searchDir,
    eResolveFlag flags, 
    Process& proc,
    HANDLE actx /*= INVALID_HANDLE_VALUE*/ 
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar_t tmpPath[4096] = { 0 };
    std::wstring completePath;

    path = Utils::ToLower( std::move( path ) );

    // Leave only file name
    std::wstring filename = Utils::StripPath( path );

    // 'ext-ms-' are resolved the same way 'api-ms-' are
    if (!IsWindows10OrGreater() && filename.find( L"ext-ms-" ) == 0)
        filename.erase( 0, 4 );

    //
    // ApiSchema redirection
    //
    auto iter = std::find_if( _apiSchema.begin(), _apiSchema.end(), [&filename]( const auto& val ) { 
        return filename.find( val.first.c_str() ) != filename.npos; } );

    if (iter != _apiSchema.end())
    {
        // Select appropriate api host
        if (!iter->second.empty())
            path = iter->second.front() != baseName ? iter->second.front() : iter->second.back();
        else
            path = baseName;

        status = ProbeSxSRedirect( path, proc, actx );
        if (NT_SUCCESS( status ) || status == STATUS_SXS_IDENTITIES_DIFFERENT)
        {
            return status;
        }
        else if (flags & EnsureFullPath)
        {
            wchar_t sys_path[255] = { 0 };
            GetSystemDirectoryW( sys_path, 255 );

            path = std::wstring( sys_path ) + L"\\" + path;
        }
        
        return STATUS_SUCCESS;
    }

    if (flags & ApiSchemaOnly)
        return STATUS_NOT_FOUND;

    // SxS redirection
    status = ProbeSxSRedirect( path, proc, actx );
    if (NT_SUCCESS( status ) || status == STATUS_SXS_IDENTITIES_DIFFERENT)
        return status;

    if (flags & NoSearch)
        return STATUS_NOT_FOUND;

    // Already a full-qualified name
    if (Utils::FileExists( path ))
        return STATUS_SUCCESS;

    //
    // Perform search accordingly to Windows Image loader search order 
    // 1. KnownDlls
    //
    HKEY hKey = NULL;
    LRESULT res = 0;
    res = RegOpenKeyW( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs", &hKey );

    if (res == 0)
    {
        for (int i = 0; i < 0x1000 && res == ERROR_SUCCESS; i++)
        {
            wchar_t value_name[255] = { 0 };
            wchar_t value_data[255] = { 0 };

            DWORD dwSize = 255;
            DWORD dwType = 0;

            res = RegEnumValueW( hKey, i, value_name, &dwSize, NULL, &dwType, reinterpret_cast<LPBYTE>(value_data), &dwSize );

            if (_wcsicmp( value_data, filename.c_str() ) == 0)
            {
                wchar_t sys_path[255] = { 0 };
                dwSize = 255;

                // In Win10 DllDirectory value got screwed, so less reliable method is used
                if (flags & Wow64)
                    GetSystemWow64DirectoryW( sys_path, dwSize );
                else
                    GetSystemDirectoryW( sys_path, dwSize );

                if (res == ERROR_SUCCESS)
                {
                    path = std::wstring( sys_path ) + L"\\" + value_data;

                    RegCloseKey( hKey );
                    return STATUS_SUCCESS;
                }
            }
        }

        RegCloseKey( hKey );
    }


    //
    // 2. Parent directory of the image being resolved
    //
    if (!searchDir.empty())
    {
        completePath = searchDir + L"\\" + filename;
        if (Utils::FileExists( completePath ))
        {
            path = completePath;
            return STATUS_SUCCESS;
        }
    }

    //
    // 3. The directory from which the application was started.
    //
    completePath = GetProcessDirectory( proc.core().pid() ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 4. The system directory
    //
    if (flags & Wow64)
        GetSystemWow64DirectoryW( tmpPath, ARRAYSIZE( tmpPath ) );
    else
        GetSystemDirectoryW( tmpPath, ARRAYSIZE( tmpPath ) );

    completePath = std::wstring( tmpPath ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 5. The Windows directory
    //
    GetWindowsDirectoryW( tmpPath, ARRAYSIZE( tmpPath ) );

    completePath = std::wstring( tmpPath ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 6. The current directory
    //
    GetCurrentDirectoryW( ARRAYSIZE( tmpPath ), tmpPath );

    completePath = std::wstring( tmpPath ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 7. Directories listed in PATH environment variable
    //
    GetEnvironmentVariableW( L"PATH", tmpPath, ARRAYSIZE( tmpPath ) );
    wchar_t *pContext;

    for (wchar_t *pDir = wcstok_s( tmpPath, L";", &pContext ); pDir; pDir = wcstok_s( pContext, L";", &pContext ))
    {
        completePath = std::wstring( pDir ) + L"\\" + filename;

        if (Utils::FileExists( completePath ))
        {
            path = completePath;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}


/// <summary>
/// Try SxS redirection
/// </summary>
/// <param name="path">Image path.</param>
/// <param name="proc">Process. Used to search process executable directory</param>
/// <param name="actx">Activation context</param>
/// <returns></returns>
NTSTATUS NameResolve::ProbeSxSRedirect( std::wstring& path, Process& proc, HANDLE actx /*= INVALID_HANDLE_VALUE*/ )
{
    UNICODE_STRING OriginalName = { 0 };
    UNICODE_STRING DllName1 = { 0 };
    UNICODE_STRING DllName2 = { 0 };
    PUNICODE_STRING pPath = nullptr;
    ULONG_PTR cookie = 0;
    wchar_t wBuf[255];

    // No underlying function
    if (GET_IMPORT( RtlDosApplyFileIsolationRedirection_Ustr ) == nullptr)
        return STATUS_ORDINAL_NOT_FOUND;

    SAFE_CALL( RtlInitUnicodeString, &OriginalName, path.c_str() );

    DllName1.Buffer = wBuf;
    DllName1.Length = NULL;
    DllName1.MaximumLength = sizeof( wBuf );

    // Use activation context
    if (actx != INVALID_HANDLE_VALUE)
        ActivateActCtx( actx, &cookie );

    // SxS resolve
    NTSTATUS status = SAFE_NATIVE_CALL(
        RtlDosApplyFileIsolationRedirection_Ustr, TRUE, &OriginalName, (PUNICODE_STRING)NULL,
        &DllName1, &DllName2, &pPath,
        nullptr, nullptr, nullptr
        );

    if (cookie != 0 && actx != INVALID_HANDLE_VALUE)
        DeactivateActCtx( 0, cookie );

    if (status == STATUS_SUCCESS)
    {
        // Arch mismatch, local SxS redirection is incorrect
        if (proc.barrier().mismatch)
            return STATUS_SXS_IDENTITIES_DIFFERENT;
        else
            path = pPath->Buffer;
    }
    else
    {
        if (DllName2.Buffer)
            SAFE_CALL( RtlFreeUnicodeString, &DllName2);
    }

    return status;
}

/// <summary>
/// Gets the process executable directory
/// </summary>
/// <param name="pid">Process ID</param>
/// <returns>Process executable directory</returns>
std::wstring NameResolve::GetProcessDirectory( DWORD pid )
{
    SnapHandle snapshot;
    MODULEENTRY32W mod = { sizeof(MODULEENTRY32W), 0 };
    std::wstring path = L"";

    if ((snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid )) && 
        Module32FirstW( snapshot, &mod ) != FALSE
        )
    {
        path = mod.szExePath;
        path = path.substr( 0, path.rfind( L"\\" ) );
    }

    return path;
}

}