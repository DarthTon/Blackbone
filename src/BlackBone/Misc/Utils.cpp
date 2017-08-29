#include "../Config.h"
#include "Utils.h"
#include "DynImport.h"

#include <algorithm>
#include <random>

namespace blackbone
{

/// <summary>
/// Convert UTF-8 string to wide char one
/// </summary>
/// <param name="str">UTF-8 string</param>
/// <returns>wide char string</returns>
std::wstring Utils::UTF8ToWstring( const std::string& str )
{
    return AnsiToWstring( str, CP_UTF8 );
}

/// <summary>
/// Convert wide string to UTF-8
/// </summary>
/// <param name="str">Wide char string</param>
/// <returns>UTF-8 string</returns>
std::string Utils::WstringToUTF8( const std::wstring& str )
{
    return WstringToAnsi( str, CP_UTF8 );
}

/// <summary>
/// Convert ANSI string to wide char one
/// </summary>
/// <param name="input">ANSI string.</param>
/// <param name="locale">String locale</param>
/// <returns>wide char string</returns>
std::wstring Utils::AnsiToWstring( const std::string& input, DWORD locale /*= CP_ACP*/ )
{
    wchar_t buf[2048] = { 0 };
    MultiByteToWideChar( locale, 0, input.c_str(), (int)input.length(), buf, ARRAYSIZE( buf ) );
    return buf;
}

/// <summary>
/// Convert wide char string to ANSI one
/// </summary>
/// <param name="input">wide char string.</param>
/// <param name="locale">String locale</param>
/// <returns>ANSI string</returns>
std::string Utils::WstringToAnsi( const std::wstring& input, DWORD locale /*= CP_ACP*/ )
{
    char buf[2048] = { 0 };
    WideCharToMultiByte( locale, 0, input.c_str(), (int)input.length(), buf, ARRAYSIZE( buf ), nullptr, nullptr );
    return buf;
}

/// <summary>
/// Format string
/// </summary>
/// <param name="fmt">Format specifier</param>
/// <param name="">Arguments</param>
/// <returns>Formatted string</returns>
std::wstring Utils::FormatString( const wchar_t* fmt, ... )
{
    wchar_t buf[4096] = { 0 };

    va_list vl;
    va_start( vl, fmt );
    vswprintf_s( buf, fmt, vl );
    va_end( vl );

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

    auto pFunc = GET_IMPORT( QueryFullProcessImageNameW );
    if (pFunc != nullptr)
        pFunc( GetCurrentProcess(), 0, imgName, &len );
    else
        GetModuleFileNameW( NULL, imgName, len );

    return GetParent( imgName );
}

/// <summary>
/// Generate random alpha-numeric string
/// </summary>
/// <param name="length">Desired length. 0 - random length from 5 to 15</param>
/// <returns>Generated string</returns>
std::wstring Utils::RandomANString( int length /*= 0*/ )
{
    static const wchar_t alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZbcdefghijklmnopqrstuvwxyz1234567890";
    static std::random_device rd;
    static std::uniform_int_distribution<> dist( 0, ARRAYSIZE(alphabet) - 2 );
    static std::uniform_int_distribution<> dist_len( 5, 15 );
    std::wstring result;

    // Get random string length
    if (length == 0)
        length = dist_len( rd );

    for (int i = 0; i < length; i++)
        result.push_back( alphabet[dist( rd )] );

    return result;
}

/// <summary>
/// Cast string characters to lower case
/// </summary>
/// <param name="str">Source string.</param>
/// <returns>Result string</returns>
std::wstring Utils::ToLower( std::wstring str )
{
    std::transform( str.begin(), str.end(), str.begin(), ::towlower );
    return str;
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
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS,
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


}