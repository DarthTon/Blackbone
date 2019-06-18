#pragma once

#include <cstdio>
#include <cstdlib>

#pragma warning(push)
#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(pop)

namespace blackbone
{
#ifndef BLACKBONE_NO_TRACE

inline void DoTraceV( const char* fmt, va_list va_args )
{
    constexpr size_t buf_size = 1024 * 1024;
    static auto buf = static_cast<char*>(VirtualAlloc( nullptr, buf_size, MEM_COMMIT, PAGE_READWRITE ));
    static auto userbuf = static_cast<char*>(VirtualAlloc( nullptr, buf_size, MEM_COMMIT, PAGE_READWRITE ));

    vsprintf_s( userbuf, buf_size, fmt, va_args );
    sprintf_s( buf, buf_size, "BlackBone: %s\r\n", userbuf );
    OutputDebugStringA( buf );

#ifdef CONSOLE_TRACE
    printf_s( buf );
#endif
}

inline void DoTraceV( const wchar_t* fmt, va_list va_args )
{
    constexpr size_t buf_size = 1024 * 1024;
    static auto buf = static_cast<wchar_t*>(VirtualAlloc( nullptr, buf_size, MEM_COMMIT, PAGE_READWRITE ));
    static auto userbuf = static_cast<wchar_t*>(VirtualAlloc( nullptr, buf_size, MEM_COMMIT, PAGE_READWRITE ));

    vswprintf_s( userbuf, buf_size / sizeof( wchar_t ), fmt, va_args );
    swprintf_s( buf, buf_size / sizeof( wchar_t ), L"BlackBone: %ls\r\n", userbuf );
    OutputDebugStringW( buf );

#ifdef CONSOLE_TRACE
    wprintf_s( buf );
#endif
}

template<typename Ch>
inline void DoTrace( const Ch* fmt, ... )
{
    va_list va_args;
    va_start( va_args, fmt );
    DoTraceV( fmt, va_args );
    va_end( va_args );
}

#define BLACKBONE_TRACE(fmt, ...) DoTrace(fmt, ##__VA_ARGS__)

#else
#define BLACKBONE_TRACE(...)
#endif

}