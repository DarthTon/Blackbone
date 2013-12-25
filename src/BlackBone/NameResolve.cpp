#include "NameResolve.h"
#include "NativeStructures.h"
#include "Macro.h"
#include "Utils.h"
#include "DynImport.h"

#include <stdint.h>
#include <algorithm>
#include <VersionHelpers.h>

namespace blackbone
{

NameResolve::NameResolve()
{
    DynImport::load( "RtlDosApplyFileIsolationRedirection_Ustr", L"ntdll.dll" );
    DynImport::load( "RtlInitUnicodeString", L"ntdll.dll" );
    DynImport::load( "RtlFreeUnicodeString", L"ntdll.dll" );
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
    if (IsWindows8Point1OrGreater())
        return InitializeP< PAPI_SET_NAMESPACE_ARRAY, 
                            PAPI_SET_NAMESPACE_ENTRY,
                            PAPI_SET_VALUE_ARRAY,
                            PAPI_SET_VALUE_ENTRY >();
    else
        return InitializeP< PAPI_SET_NAMESPACE_ARRAY_V2, 
                            PAPI_SET_NAMESPACE_ENTRY_V2,
                            PAPI_SET_VALUE_ARRAY_V2,
                            PAPI_SET_VALUE_ENTRY_V2 >();
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

    PEB_T *ppeb = reinterpret_cast<PEB_T*>(NtCurrentTeb()->ProcessEnvironmentBlock);
    T1 pSetMap = reinterpret_cast<T1>(ppeb->ApiSetMap);

    for (DWORD i = 0; i < pSetMap->Count; i++)
    {
        T2 pDescriptor = pSetMap->Array + i;

        std::vector<std::wstring> vhosts;

        wchar_t apiName[MAX_PATH] = { 0 };
        wchar_t dllName[MAX_PATH] = { 0 };

        memcpy( apiName, (uint8_t*)pSetMap + pDescriptor->NameOffset, pDescriptor->NameLength );
        swprintf_s( dllName, MAX_PATH, L"API-%s.dll", apiName );
        std::transform( dllName, dllName + MAX_PATH, dllName, ::tolower );

        T3 pHostData = reinterpret_cast<T3>(reinterpret_cast<uint8_t*>(pSetMap)+pDescriptor->DataOffset);

        for (DWORD j = 0; j < pHostData->Count; j++)
        {
            T4 pHost = pHostData->Array + j;
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
NTSTATUS NameResolve::ResolvePath( std::wstring& path, 
                                   const std::wstring& baseName,
                                   const std::wstring& searchDir,
                                   eResolveFlag flags, 
                                   DWORD procID, 
                                   HANDLE actx /*= INVALID_HANDLE_VALUE*/ )
{
    wchar_t tmpPath[4096] = { 0 };
    std::wstring completePath;

    std::transform( path.begin(), path.end(), path.begin(), ::tolower );

    // Leave only file name
    std::wstring filename = Utils::StripPath( path );

    // 'ext-ms-' are resolved the same way 'api-ms-' are
    if (filename.find( L"ext-ms-" ) == 0)
        filename.replace( 0, 3, L"api" );

    //
    // ApiSchema redirection
    //
    auto iter = _apiSchema.find( filename );

    if (iter != _apiSchema.end())
    {
        // Select appropriate api host
        if (iter->second.front() != baseName)
            path.assign( iter->second.front().begin(), iter->second.front().end() );
        else
            path.assign( iter->second.back().begin(), iter->second.back().end() );

        if (ProbeSxSRedirect( path, actx ) == STATUS_SUCCESS)
            return STATUS_SUCCESS;
        else if (flags & EnsureFullPath)
            path = L"C:\\windows\\system32\\" + path;
        
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
    res = RegOpenKey( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs", &hKey );

    if (res == 0)
    {
        for (int i = 0; i < 0x1000 && res == ERROR_SUCCESS; i++)
        {
            wchar_t value_name[255] = { 0 };
            wchar_t value_data[255] = { 0 };

            DWORD dwSize = 255;
            DWORD dwType = 0;

            res = RegEnumValue( hKey, i, value_name, &dwSize, NULL, &dwType, reinterpret_cast<LPBYTE>(value_data), &dwSize );

            if (_wcsicmp( value_data, filename.c_str() ) == 0)
            {
                wchar_t sys_path[255] = { 0 };
                dwSize = 255;

                res = RegGetValueW( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs",
                                    L"DllDirectory", RRF_RT_ANY, &dwType, sys_path, &dwSize );

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
    // 2. Prent directory of image being resolved
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
    GetSystemDirectory( tmpPath, ARRAYSIZE( tmpPath ) );

    completePath = std::wstring( tmpPath ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 5. The Windows directory
    //
    GetWindowsDirectory( tmpPath, ARRAYSIZE( tmpPath ) );

    completePath = std::wstring( tmpPath ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 6. The current directory
    //
    GetCurrentDirectory( ARRAYSIZE( tmpPath ), tmpPath );

    completePath = std::wstring( tmpPath ) + L"\\" + filename;

    if (Utils::FileExists( completePath ))
    {
        path = completePath;
        return STATUS_SUCCESS;
    }

    //
    // 7. Directories listed in PATH environment variable
    //
    GetEnvironmentVariable( L"PATH", tmpPath, ARRAYSIZE( tmpPath ) );
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
    UNICODE_STRING OriginalName;
    UNICODE_STRING Extension;
    UNICODE_STRING DllName1;
    UNICODE_STRING DllName2;
    PUNICODE_STRING pPath = nullptr;
    ULONG_PTR cookie = 0;
    wchar_t wBuf[255];

    // No underlying function
    if (GET_IMPORT( RtlDosApplyFileIsolationRedirection_Ustr ) == nullptr)
        return STATUS_ORDINAL_NOT_FOUND;

    if (path.rfind( L".dll" ) != std::wstring::npos)
        path.erase( path.rfind( L".dll" ) );

    GET_IMPORT( RtlInitUnicodeString )( &Extension, L".dll" );
    GET_IMPORT( RtlInitUnicodeString )( &OriginalName, path.c_str()) ;
    GET_IMPORT( RtlInitUnicodeString )( &DllName2, L"" );

    DllName1.Buffer = wBuf;
    DllName1.Length = NULL;
    DllName1.MaximumLength = ARRAYSIZE( wBuf );

    // Use activation context
    if (actx != INVALID_HANDLE_VALUE)
        ActivateActCtx( actx, &cookie );

    // SxS resolve
    NTSTATUS status = GET_IMPORT( RtlDosApplyFileIsolationRedirection_Ustr )( 1, &OriginalName, &Extension,
                                                                              &DllName1, &DllName2, &pPath,
                                                                              NULL, NULL, NULL );

    if (cookie != 0 && actx != INVALID_HANDLE_VALUE)
        DeactivateActCtx( 0, cookie );

    if (status == STATUS_SUCCESS)
    {
        path = pPath->Buffer;
    }
    else
    {
        GET_IMPORT( RtlFreeUnicodeString )( &DllName2 );
        path.append( L".dll" );
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
    MODULEENTRY32 mod = { sizeof(MODULEENTRY32), 0 };
    std::wstring path = L"";

    if ((snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid )) == INVALID_HANDLE_VALUE)
        return L"";

    if (Module32First( snapshot, &mod ) == TRUE)
    {
        path = mod.szExePath;
        path = path.substr( 0, path.rfind( L"\\" ) );
    }

    CloseHandle( snapshot );

    return path;
}

}