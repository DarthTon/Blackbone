// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_BUILD_H
#define _ASMJIT_BUILD_H

// [Include]
#include "Config.h"

// Here should be optional include files that's needed fo successfuly
// use macros defined here. Remember, AsmJit uses only AsmJit namespace
// and all macros are used within it.
#include <stdio.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------
// [AsmJit - OS]
// ----------------------------------------------------------------------------

#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WIN32) || defined(_WIN64)
# define ASMJIT_WINDOWS
#elif defined(__linux__)     || defined(__unix__)    || \
      defined(__OpenBSD__)   || defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__DragonFly__) || defined(__BSD__)     || defined(__FREEBSD__) || \
      defined(__APPLE__)
# define ASMJIT_POSIX
#else
# warning "AsmJit - Can't match operating system, using ASMJIT_POSIX"
# define ASMJIT_POSIX
#endif

// ----------------------------------------------------------------------------
// [AsmJit - Architecture]
// ----------------------------------------------------------------------------

// define it only if it's not defined. In some systems we can
// use -D command in compiler to bypass this autodetection.
#if !defined(ASMJIT_X86) && !defined(ASMJIT_X64)
# if defined(__x86_64__) || defined(__LP64) || defined(__IA64__) || \
     defined(_M_X64)     || defined(_WIN64) 
#  define ASMJIT_X64 // x86-64
# else
// _M_IX86, __INTEL__, __i386__
#  define ASMJIT_X86
# endif
#endif

// ----------------------------------------------------------------------------
// [AsmJit - API]
// ----------------------------------------------------------------------------

// Hide AsmJit symbols that we don't want to export (AssemblerIntrinsics class for example).
#if !defined(ASMJIT_HIDDEN)
# if defined(__GNUC__) && __GNUC__ >= 4
#  define ASMJIT_HIDDEN __attribute__((visibility("hidden")))
# endif // __GNUC__ && __GNUC__ >= 4
#endif // ASMJIT_HIDDEN

// Make AsmJit as shared library by default.
#if !defined(ASMJIT_API)
# if defined(ASMJIT_WINDOWS)
#  if defined(__GNUC__)
#   if defined(AsmJit_EXPORTS)
#    define ASMJIT_API __attribute__((dllexport))
#   else
#    define ASMJIT_API __attribute__((dllimport))
#   endif // AsmJit_EXPORTS
#  else
#   if defined(AsmJit_EXPORTS)
#    define ASMJIT_API __declspec(dllexport)
#   else
#    define ASMJIT_API __declspec(dllimport)
#   endif // AsmJit_EXPORTS
#  endif // __GNUC__
# else
#  if defined(__GNUC__)
#   if __GNUC__ >= 4
#    define ASMJIT_API __attribute__((visibility("default")))
#    define ASMJIT_VAR extern ASMJIT_API
#   endif // __GNUC__ >= 4
#  endif // __GNUC__
# endif
#endif // ASMJIT_API

#if defined(ASMJIT_API)
# define ASMJIT_VAR extern ASMJIT_API
#else
# define ASMJIT_API
# define ASMJIT_VAR
#endif // ASMJIT_API

// If not detected, fallback to nothing.
#if !defined(ASMJIT_HIDDEN)
# define ASMJIT_HIDDEN
#endif // ASMJIT_HIDDEN

#if !defined(ASMJIT_NOTHROW)
#define ASMJIT_NOTHROW throw()
#endif // ASMJIT_NOTHROW

// [AsmJit - Memory Management]
#if !defined(ASMJIT_MALLOC)
# define ASMJIT_MALLOC ::malloc
#endif // ASMJIT_MALLOC

#if !defined(ASMJIT_REALLOC)
# define ASMJIT_REALLOC ::realloc
#endif // ASMJIT_REALLOC

#if !defined(ASMJIT_FREE)
# define ASMJIT_FREE ::free
#endif // ASMJIT_FREE

// ----------------------------------------------------------------------------
// [AsmJit - Calling Conventions]
// ----------------------------------------------------------------------------

#if defined(ASMJIT_X86)
# if defined(__GNUC__)
#  define ASMJIT_REGPARM_1 __attribute__((regparm(1)))
#  define ASMJIT_REGPARM_2 __attribute__((regparm(2)))
#  define ASMJIT_REGPARM_3 __attribute__((regparm(3)))
#  define ASMJIT_FASTCALL  __attribute__((fastcall))
#  define ASMJIT_STDCALL   __attribute__((stdcall))
#  define ASMJIT_CDECL     __attribute__((cdecl))
# else
#  define ASMJIT_FASTCALL   __fastcall
#  define ASMJIT_STDCALL    __stdcall
#  define ASMJIT_CDECL      __cdecl
# endif
#else
# define ASMJIT_FASTCALL
# define ASMJIT_STDCALL
# define ASMJIT_CDECL
#endif // ASMJIT_X86

#if !defined(ASMJIT_UNUSED)
# define ASMJIT_UNUSED(var) ((void)var)
#endif // ASMJIT_UNUSED

#if !defined(ASMJIT_NOP)
# define ASMJIT_NOP() ((void)0)
#endif // ASMJIT_NOP

// [AsmJit - C++ Compiler Support]
#define ASMJIT_TYPE_TO_TYPE(type) type 
#define ASMJIT_HAS_STANDARD_DEFINE_OPTIONS
#define ASMJIT_HAS_PARTIAL_TEMPLATE_SPECIALIZATION

// Support for VC6
#if defined(_MSC_VER) && (_MSC_VER < 1400)
#undef ASMJIT_TYPE_TO_TYPE
namespace AsmJit {
  template<typename T>
  struct _Type2Type { typedef T Type; };
}
#define ASMJIT_TYPE_TO_TYPE(T) _Type2Type<T>::Type

#undef ASMJIT_HAS_STANDARD_DEFINE_OPTIONS
#undef ASMJIT_HAS_PARTIAL_TEMPLATE_SPECIALIZATION

#endif

// ----------------------------------------------------------------------------
// [AsmJit - Types]
// ----------------------------------------------------------------------------

#if defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER >= 1600)

// Use <stdint.h>
#include <stdint.h>

#else

// Use typedefs.
#if defined(_MSC_VER)
#if (_MSC_VER < 1300)
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif
#endif // _MSC_VER
#endif // STDINT.H

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#if defined(ASMJIT_X86)
typedef int32_t sysint_t;
typedef uint32_t sysuint_t;
#else
typedef int64_t sysint_t;
typedef uint64_t sysuint_t;
#endif

#if defined(_MSC_VER)
# define ASMJIT_INT64_C(num) num##i64
# define ASMJIT_UINT64_C(num) num##ui64
#else
# define ASMJIT_INT64_C(num) num##LL
# define ASMJIT_UINT64_C(num) num##ULL
#endif

// ----------------------------------------------------------------------------
// [AsmJit - C++ Macros]
// ----------------------------------------------------------------------------

#define ASMJIT_ARRAY_SIZE(A) (sizeof(A) / sizeof(*A))

#define ASMJIT_DISABLE_COPY(__type__) \
private: \
  inline __type__(const __type__& other); \
  inline __type__& operator=(const __type__& other);

// ----------------------------------------------------------------------------
// [AsmJit - Debug]
// ----------------------------------------------------------------------------

// If ASMJIT_DEBUG and ASMJIT_NO_DEBUG is not defined then ASMJIT_DEBUG will be
// detected using the compiler specific macros. This enables to set the build 
// type using IDE.
#if !defined(ASMJIT_DEBUG) && !defined(ASMJIT_NO_DEBUG)

#if defined(_DEBUG)
#define ASMJIT_DEBUG
#endif // _DEBUG

#endif // !ASMJIT_DEBUG && !ASMJIT_NO_DEBUG
  
// ----------------------------------------------------------------------------
// [AsmJit - Assert]
// ----------------------------------------------------------------------------

namespace AsmJit {
ASMJIT_API void assertionFailure(const char* file, int line, const char* exp);
} // AsmJit namespace

#if defined(ASMJIT_DEBUG)
# if !defined(ASMJIT_ASSERT)
#  define ASMJIT_ASSERT(exp) do { if (!(exp)) ::AsmJit::assertionFailure(__FILE__, __LINE__, #exp); } while(0)
# endif
#else
# if !defined(ASMJIT_ASSERT)
#  define ASMJIT_ASSERT(exp) ASMJIT_NOP()
# endif
#endif // DEBUG

// GCC warnings fix: I can't understand why GCC has no interface to push/pop
// specific warnings.
// #if defined(__GNUC__)
// # if (__GNUC__ * 10000  + __GNUC_MINOR__ * 100  + __GNUC_PATCHLEVEL__) >= 402001
// #  pragma GCC diagnostic ignored "-w"
// # endif
// #endif // __GNUC__

// ----------------------------------------------------------------------------
// [AsmJit - OS Support]
// ----------------------------------------------------------------------------

#if defined(ASMJIT_WINDOWS)
#include <Windows.h>
#endif // ASMJIT_WINDOWS

// [Guard]
#endif // _ASMJIT_BUILD_H
