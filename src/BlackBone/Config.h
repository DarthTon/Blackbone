#pragma once

#if defined(_MSC_VER)
#define COMPILER_MSVC
#elif defined(__GNUC__)
#define COMPILER_GCC
#else
#error "Unknown or unsupported compiler"
#endif

// No IA64 support
#if defined (_M_AMD64) || defined (__x86_64__)
#define USE64
#elif defined (_M_IX86) || defined (__i386__)
#define USE32
#else
#error "Unknown or unsupported platform"
#endif

