#pragma once

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <DbgHelp.h>

namespace blackbone
{
#ifndef BLACBONE_NO_TRACE

template<typename Ch>
inline void DoTrace( const Ch* fmt, ... )
{
    va_list va_args;
    va_start( va_args, fmt );
    DoTraceV( fmt, va_args );
    va_end( va_args );
}

template<typename Ch>
void DoTraceV( const Ch* fmt, va_list va_args );

template<>
void DoTraceV<char>( const char* fmt, va_list va_args )
{
    char buf[2048], userbuf[1024];
    vsprintf_s( userbuf, fmt, va_args );
    sprintf_s( buf, "BlackBone: %ls\r\n", userbuf );
    OutputDebugStringA( buf );
}

template<>
void DoTraceV<wchar_t>( const wchar_t* fmt, va_list va_args )
{
    wchar_t buf[2048], userbuf[1024];
    vswprintf_s( userbuf, fmt, va_args );
    swprintf_s( buf, L"BlackBone: %ls\r\n", userbuf );
    OutputDebugStringW( buf );
}

#define BLACBONE_TRACE(fmt, ...) DoTrace(fmt, __VA_ARGS__)

#else
#define BLACBONE_TRACE(...)
#endif

}