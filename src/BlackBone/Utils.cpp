#include "Utils.h"
#include "DynImport.h"

#include <codecvt>
#include <locale>
#include <filesystem>
#include <algorithm>

namespace blackbone
{

/// <summary>
/// Convert UTF-8 string to wide char one
/// </summary>
/// <param name="str">UTF-8 string</param>
/// <returns>wide char string</returns>
std::wstring Utils::UTF8ToWstring( const std::string& str )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
    return conv.from_bytes( str );
}

/// <summary>
/// Convert wide string to UTF-8
/// </summary>
/// <param name="str">Wide char string</param>
/// <returns>UTF-8 string</returns>
std::string Utils::WstringToUTF8( const std::wstring& str )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
    return conv.to_bytes( str );
}

/// <summary>
/// Convert ANSI string to wide char one
/// </summary>
/// <param name="input">ANSI string.</param>
/// <param name="locale">String locale</param>
/// <returns>wide char string</returns>
std::wstring Utils::AnsiToWstring( const std::string& input, DWORD locale /*= CP_ACP*/ )
{
    wchar_t buf[8192] = { 0 };
    MultiByteToWideChar( locale, 0, input.c_str(), (int)input.length(), buf, ARRAYSIZE( buf ) );
    return buf;
}

/// <summary>
/// Get filename from full-qualified path
/// </summary>
/// <param name="path">File path</param>
/// <returns>Filename</returns>
std::wstring Utils::StripPath( const std::wstring& path )
{
    if (path.empty())
        return path;

    auto idx = path.rfind( L'\\' );
    if(idx == path.npos)
        idx = path.rfind( L'/' );

    if (idx != path.npos)
        return path.substr( idx + 1 );
    else
        return path;
}

/// <summary>
/// Get parent directory
/// </summary>
/// <param name="path">File path</param>
/// <returns>Parent directory</returns>
std::wstring Utils::GetParent( const std::wstring& path )
{
    if (path.empty())
        return path;

    auto idx = path.rfind( L'\\' );
    if (idx == path.npos)
        idx = path.rfind( L'/' );

    if (idx != path.npos)
        return path.substr( 0, idx );
    else
        return path;
}

/// <summary>
/// Get current process exe file directory
/// </summary>
/// <returns>Exe directory</returns>
std::wstring Utils::GetExeDirectory()
{
    wchar_t imgName[MAX_PATH] = { 0 };
    DWORD len = ARRAYSIZE(imgName);

    QueryFullProcessImageNameW( GetCurrentProcess(), 0, imgName, &len );

    return GetParent( imgName );
}

/// <summary>
/// Cast string characters to lower case
/// </summary>
/// <param name="str">Source string.</param>
/// <returns>Result string</returns>
std::wstring Utils::ToLower( const std::wstring& str )
{
    std::wstring str2( str );
    std::transform( str2.begin(), str2.end(), str2.begin(), ::tolower );

    return str2;
}

/// <summary>
/// Get system error description
/// </summary>
/// <param name="code">The code.</param>
/// <returns>Error message</returns>
std::wstring Utils::GetErrorDescription( NTSTATUS code )
{
    LPWSTR lpMsgBuf = nullptr;

    if (FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_FROM_HMODULE,
        GetModuleHandleW( L"ntdll.dll" ),
        code, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPWSTR)&lpMsgBuf, 0, NULL ) != 0)
    {
        std::wstring ret( lpMsgBuf );

        LocalFree( lpMsgBuf );
        return ret;
    }

    return L"";
}

/// <summary>
/// Check if file exists
/// </summary>
/// <param name="path">Full-qualified file path</param>
/// <returns>true if exists</returns>
bool Utils::FileExists( const std::wstring& path )
{
    return (GetFileAttributesW( path.c_str() ) != 0xFFFFFFFF );
}

/// <summary>
/// Load arbitrary driver
/// </summary>
/// <param name="svcName">Driver service name</param>
/// <param name="path">Driver file path</param>
/// <returns>Status</returns>
NTSTATUS Utils::LoadDriver( const std::wstring& svcName, const std::wstring& path )
{
    HKEY key1, key2;
    BYTE dwType = 1;
    UNICODE_STRING Ustr;
    LSTATUS status = 0;
    WCHAR wszLocalPath[MAX_PATH] = { 0 };

    swprintf_s( wszLocalPath, ARRAYSIZE( wszLocalPath ), L"\\??\\%s", path.c_str() );

    status = RegOpenKey( HKEY_LOCAL_MACHINE, L"system\\CurrentControlSet\\Services", &key1 );

    if (status)
        return status;

    status = RegCreateKeyW( key1, svcName.c_str(), &key2 );

    if (status)
    {
        RegCloseKey( key1 );
        return status;
    }

    status = RegSetValueEx( key2, L"ImagePath", 0, REG_SZ, reinterpret_cast<const BYTE*>(wszLocalPath), 
                            static_cast<DWORD>(sizeof(WCHAR)* (wcslen( wszLocalPath ) + 1)) );

    if (status)
    {
        RegCloseKey( key2 );
        RegCloseKey( key1 );
        return status;
    }

    status = RegSetValueEx( key2, L"Type", 0, REG_DWORD, &dwType, sizeof(DWORD) );

    if (status)
    {
        RegCloseKey( key2 );
        RegCloseKey( key1 );
        return status;
    }

    RegCloseKey( key2 );
    RegCloseKey( key1 );

    std::wstring regPath = L"\\registry\\machine\\SYSTEM\\CurrentControlSet\\Services\\" + svcName;
    GET_IMPORT( RtlInitUnicodeString )(&Ustr, regPath.c_str());

    DynImport::load( "NtUnloadDriver", GetModuleHandleW( L"ntdll.dll" ) );
    DynImport::load( "NtLoadDriver", GetModuleHandleW( L"ntdll.dll" ) );

    // Remove previously loaded instance
    GET_IMPORT( NtUnloadDriver )(&Ustr);

    return GET_IMPORT( NtLoadDriver )(&Ustr);
}

}