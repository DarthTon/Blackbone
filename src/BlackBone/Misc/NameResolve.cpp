#include "NameResolve.h"
#include "../Include/NativeStructures.h"
#include "../Include/Macro.h"
#include "../Misc/Utils.h"
#include "../Misc/DynImport.h"

#include <stdint.h>
#include <algorithm>
#include <VersionHelpers.h>

namespace blackbone
{

NameResolve::NameResolve()
{
    HMODULE ntdll = GetModuleHandleW( L"ntdll.dll" );

    DynImport::load( "NtQuerySystemInformation", ntdll );
    DynImport::load( "RtlDosApplyFileIsolationRedirection_Ustr", ntdll );
    DynImport::load( "RtlInitUnicodeString", ntdll );
    DynImport::load( "RtlFreeUnicodeString", ntdll );
}

NameResolve::~NameResolve()
{
}

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
template<typename T1, typename T2, typename T3, typename T4>
bool blackbone::NameResolve::InitializeP()
{
    if (!_apiSchema.empty())
        return true;

    PEB_T *ppeb = reinterpret_cast<PEB_T*>(reinterpret_cast<TEB_T*>(NtCurrentTeb())->ProcessEnvironmentBlock);
    T1 pSetMap = reinterpret_cast<T1>(ppeb->ApiSetMap);

    for (DWORD i = 0; i < pSetMap->Count; i++)
    {
        T2 pDescriptor = pSetMap->entry(i);

        std::vector<std::wstring> vhosts;
        wchar_t dllName[MAX_PATH] = { 0 };

        pSetMap->apiName( pDescriptor, dllName );
        std::transform( dllName, dllName + MAX_PATH, dllName, ::tolower );

        T3 pHostData = pSetMap->valArray( pDescriptor );

        for (DWORD j = 0; j < pHostData->Count; j++)
        { 
            T4 pHost = pHostData->entry( pSetMap, j );
            std::wstring hostName( reinterpret_cast<wchar_t*>(reinterpret_cast<uint8_t*>(pSetMap)+pHost->ValueOffset),
                                   pHost->ValueLength / sizeof(wchar_t) );

            if (!hostName.empty())
                vhosts.push_back( hostName );
        }

        _apiSchema.insert( std::make_pair( dllName, vhosts ) );
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
    DWORD procID, 
    HANDLE actx /*= INVALID_HANDLE_VALUE*/ 
    )
{
    wchar_t tmpPath[4096] = { 0 };
    std::wstring completePath;

    std::transform( path.begin(), path.end(), path.begin(), ::tolower );

    // Leave only file name
    std::wstring filename = Utils::StripPath( path );

    // 'ext-ms-' are resolved the same way 'api-ms-' are
    if (filename.find( L"ext-ms-" ) == 0)
        filename.erase( 0, 4 );

    //
    // ApiSchema redirection
    //
    auto iter = std::find_if( _apiSchema.begin(), _apiSchema.end(), [&filename]( const mapApiSchema::value_type& val ) { 
        return filename.find( val.first.c_str() ) != filename.npos; } );

    if (iter != _apiSchema.end())
    {
        // Select appropriate api host
        path = iter->second.front() != baseName ? iter->second.front() : iter->second.back();

        if (ProbeSxSRedirect( path, actx ) == STATUS_SUCCESS)
        {
            return STATUS_SUCCESS;
        }
        else if (flags & EnsureFullPath)
        {
            wchar_t sys_path[255] = { 0 };
            GetSystemDirectoryW( sys_path, 255 );

            path = sys_path + path;
        }
        
        return LastNtStatus( STATUS_SUCCESS );
    }

    if (flags & ApiSchemaOnly)
        return LastNtStatus( STATUS_NOT_FOUND );

    // SxS redirection
    if (ProbeSxSRedirect( path, actx ) == STATUS_SUCCESS)
        return LastNtStatus( STATUS_SUCCESS );

    if (flags & NoSearch)
        return LastNtStatus( STATUS_NOT_FOUND );

    // Already a full-qualified name
    if (Utils::FileExists( path ))
        return LastNtStatus( STATUS_SUCCESS );

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
    completePath = GetProcessDirectory( procID ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 4. The system directory
    //
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

    return LastNtStatus( STATUS_NOT_FOUND );
}


/// <summary>
/// Try SxS redirection
/// </summary>
/// <param name="path">Image path.</param>
/// <param name="actx">Activation context</param>
/// <returns></returns>
NTSTATUS NameResolve::ProbeSxSRedirect( std::wstring& path, HANDLE actx /*= INVALID_HANDLE_VALUE*/ )
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
        path = pPath->Buffer;
    }
    else
    {
        if (DllName2.Buffer)
            SAFE_CALL( RtlFreeUnicodeString, &DllName2);
    }

    return LastNtStatus( status );
}

/// <summary>
/// Gets the process executable directory
/// </summary>
/// <param name="pid">Process ID</param>
/// <returns>Process executable directory</returns>
std::wstring NameResolve::GetProcessDirectory( DWORD pid )
{
    HANDLE snapshot = 0;
    MODULEENTRY32W mod = { sizeof(MODULEENTRY32W), 0 };
    std::wstring path = L"";

    if ((snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid )) == INVALID_HANDLE_VALUE)
        return L"";

    if (Module32FirstW( snapshot, &mod ) == TRUE)
    {
        path = mod.szExePath;
        path = path.substr( 0, path.rfind( L"\\" ) );
    }

    CloseHandle( snapshot );

    return path;
}

}