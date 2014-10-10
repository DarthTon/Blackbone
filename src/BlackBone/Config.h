#pragma once

#if defined(_MSC_VER)

    #define COMPILER_MSVC

    #if defined(BLACKBONE_IMPORTS)
        #define BLACKBONE_API __declspec(dllimport)
    #elif defined(BLACKBONE_EXPORTS)
        #define BLACKBONE_API __declspec(dllexport)
    #else
        #define BLACKBONE_API
    #endif

#elif defined(__GNUC__)
    #define COMPILER_GCC
    #define BLACKBONE_API
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

