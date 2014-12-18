// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BUILD_H
#define _ASMJIT_BUILD_H

// [Include]
#if defined(ASMJIT_CONFIG_FILE)
# include ASMJIT_CONFIG_FILE
#else
# include "./config.h"
#endif // ASMJIT_CONFIG_FILE

// Turn off deprecation warnings when compiling AsmJit.
#if defined(ASMJIT_EXPORTS) && defined(_MSC_VER)
# if !defined(_CRT_SECURE_NO_DEPRECATE)
#  define _CRT_SECURE_NO_DEPRECATE
# endif // !_CRT_SECURE_NO_DEPRECATE
# if !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS
# endif // !_CRT_SECURE_NO_WARNINGS
#endif // ASMJIT_EXPORTS

// [Dependencies - C]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// [Dependencies - C++]
#include <new>

// ============================================================================
// [asmjit::build - Sanity]
// ============================================================================

#if defined(ASMJIT_DISABLE_NAMES) && !defined(ASMJIT_DISABLE_LOGGER)
# error "ASMJIT_DISABLE_NAMES requires ASMJIT_DISABLE_LOGGER to be defined."
#endif // ASMJIT_DISABLE_NAMES && !ASMJIT_DISABLE_LOGGER

// ============================================================================
// [asmjit::build - OS]
// ============================================================================

#if defined(_WINDOWS) || defined(__WINDOWS__) || defined(_WIN32) || defined(_WIN64)
# define ASMJIT_OS_WINDOWS
#elif defined(__linux) || defined(__linux__)
# define ASMJIT_OS_POSIX
# define ASMJIT_OS_LINUX
#elif defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define ASMJIT_OS_POSIX
# define ASMJIT_OS_BSD
#elif defined(__APPLE__)
# define ASMJIT_OS_POSIX
# define ASMJIT_OS_MAC
#else
# warning "AsmJit - Unable to detect host operating system, using ASMJIT_OS_POSIX"
# define ASMJIT_OS_POSIX
#endif

// ============================================================================
// [asmjit::build - Arch]
// ============================================================================

#if defined(_M_X64    ) || \
    defined(_M_AMD64  ) || \
    defined(_WIN64    ) || \
    defined(__amd64__ ) || \
    defined(__LP64    ) || \
    defined(__x86_64__)
# define ASMJIT_HOST_X64
# define ASMJIT_HOST_LE
# define ASMJIT_HOST_UNALIGNED_16
# define ASMJIT_HOST_UNALIGNED_32
# define ASMJIT_HOST_UNALIGNED_64
#elif \
    defined(_M_IX86  ) || \
    defined(__INTEL__) || \
    defined(__i386__ )
# define ASMJIT_HOST_X86
# define ASMJIT_HOST_LE
# define ASMJIT_HOST_UNALIGNED_16
# define ASMJIT_HOST_UNALIGNED_32
# define ASMJIT_HOST_UNALIGNED_64
#elif \
    defined(_ARM               ) || \
    defined(_M_ARM_FP          ) || \
    defined(__ARM_NEON__       ) || \
    defined(__arm              ) || \
    defined(__arm__            ) || \
    defined(__TARGET_ARCH_ARM  ) || \
    defined(__TARGET_ARCH_THUMB) || \
    defined(__thumb__          )
# define ASMJIT_HOST_ARM
# define ASMJIT_HOST_LE
#else
# warning "AsmJit - Unable to detect host architecture"
#endif

// ============================================================================
// [asmjit::build - Build]
// ============================================================================

// Build host architecture if no architecture is selected.
#if !defined(ASMJIT_BUILD_HOST) && \
    !defined(ASMJIT_BUILD_X86) && \
    !defined(ASMJIT_BUILD_X64)
# define ASMJIT_BUILD_HOST
#endif

// Autodetect host architecture if enabled.
#if defined(ASMJIT_BUILD_HOST)
# if defined(ASMJIT_HOST_X86) && !defined(ASMJIT_BUILD_X86)
#  define ASMJIT_BUILD_X86
# endif // ASMJIT_HOST_X86 && !ASMJIT_BUILD_X86
# if defined(ASMJIT_HOST_X64) && !defined(ASMJIT_BUILD_X64)
#  define ASMJIT_BUILD_X64
# endif // ASMJIT_HOST_X64 && !ASMJIT_BUILD_X64
#endif // ASMJIT_BUILD_HOST

// ============================================================================
// [asmjit::build - Decorators]
// ============================================================================

#if defined(ASMJIT_EMBED) && !defined(ASMJIT_STATIC)
# define ASMJIT_STATIC
#endif // ASMJIT_EMBED && !ASMJIT_STATIC

#if defined(ASMJIT_STATIC)
# define ASMJIT_API
#elif defined(ASMJIT_OS_WINDOWS)
# if (defined(__GNUC__) || defined(__clang__)) && !defined(__MINGW32__)
#  if defined(ASMJIT_EXPORTS)
#   define ASMJIT_API __attribute__((dllexport))
#  else
#   define ASMJIT_API __attribute__((dllimport))
#  endif // ASMJIT_EXPORTS
# else
#  if defined(ASMJIT_EXPORTS)
#   define ASMJIT_API __declspec(dllexport)
#  else
#   define ASMJIT_API __declspec(dllimport)
#  endif
# endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
# define ASMJIT_API __attribute__((visibility("default")))
#endif

#if !defined(ASMJIT_API)
# define ASMJIT_API
#endif // ASMJIT_API

// This is basically a workaround. When using MSVC and marking class as DLL
// export everything is exported, which is unwanted since there are many
// inlines which mimic instructions. MSVC automatically exports typeinfo and
// vtable if at least one symbol of that class is exported. However, GCC has
// some strange behavior that even if one or more symbol is exported it doesn't
// export `typeinfo` unless the class itself is marked as "visibility(default)".
#if !defined(ASMJIT_OS_WINDOWS) && (defined(__GNUC__) || defined (__clang__))
# define ASMJIT_VCLASS ASMJIT_API
#else
# define ASMJIT_VCLASS
#endif

#if !defined(ASMJIT_VAR)
# define ASMJIT_VAR extern ASMJIT_API
#endif // !ASMJIT_VAR

#if defined(_MSC_VER)
# define ASMJIT_INLINE __forceinline
#elif defined(__clang__)
# define ASMJIT_INLINE inline __attribute__((always_inline)) __attribute__((visibility("hidden")))
#elif defined(__GNUC__)
# define ASMJIT_INLINE inline __attribute__((always_inline))
#else
# define ASMJIT_INLINE inline
#endif

#if defined(ASMJIT_HOST_X86)
# if defined(__GNUC__) || defined(__clang__)
#  define ASMJIT_REGPARM_1 __attribute__((regparm(1)))
#  define ASMJIT_REGPARM_2 __attribute__((regparm(2)))
#  define ASMJIT_REGPARM_3 __attribute__((regparm(3)))
#  define ASMJIT_FASTCALL  __attribute__((fastcall))
#  define ASMJIT_STDCALL   __attribute__((stdcall))
#  define ASMJIT_CDECL     __attribute__((cdecl))
# else
#  define ASMJIT_FASTCALL  __fastcall
#  define ASMJIT_STDCALL   __stdcall
#  define ASMJIT_CDECL     __cdecl
# endif
#else
# define ASMJIT_FASTCALL
# define ASMJIT_STDCALL
# define ASMJIT_CDECL
#endif // ASMJIT_HOST_X86

// ============================================================================
// [asmjit::build - Enum]
// ============================================================================

#if defined(_MSC_VER)
# define ASMJIT_ENUM(_Name_) enum _Name_ : uint32_t
#else
# define ASMJIT_ENUM(_Name_) enum _Name_
#endif

// ============================================================================
// [asmjit::build - Memory Management]
// ============================================================================

#if !defined(ASMJIT_ALLOC) && !defined(ASMJIT_REALLOC) && !defined(ASMJIT_FREE)
# define ASMJIT_ALLOC(_Size_) ::malloc(_Size_)
# define ASMJIT_REALLOC(_Ptr_, _Size_) ::realloc(_Ptr_, _Size_)
# define ASMJIT_FREE(_Ptr_) ::free(_Ptr_)
#else
# if !defined(ASMJIT_ALLOC) || !defined(ASMJIT_REALLOC) || !defined(ASMJIT_FREE)
#  error "AsmJit - You must redefine ASMJIT_ALLOC, ASMJIT_REALLOC and ASMJIT_FREE."
# endif
#endif // !ASMJIT_ALLOC && !ASMJIT_REALLOC && !ASMJIT_FREE

// ============================================================================
// [asmjit::build - _ASMJIT_HOST_INDEX]
// ============================================================================

#if defined(ASMJIT_HOST_LE)
# define _ASMJIT_HOST_INDEX(_Total_, _Index_) (_Index_)
#else
# define _ASMJIT_HOST_INDEX(_Total_, _Index_) ((_Total_) - 1 - (_Index_))
#endif

// ============================================================================
// [asmjit::build - BLEND_OFFSET_OF]
// ============================================================================

//! Cross-platform solution to get offset of `_Field_` in `_Struct_`.
#define ASMJIT_OFFSET_OF(_Struct_, _Field_) \
  static_cast<int>((intptr_t) ((const uint8_t*) &((const _Struct_*)0x1)->_Field_) - 1)

// ============================================================================
// [asmjit::build - ASMJIT_ARRAY_SIZE]
// ============================================================================

#define ASMJIT_ARRAY_SIZE(_Array_) \
  (sizeof(_Array_) / sizeof(*_Array_))

// ============================================================================
// [asmjit::build - ASMJIT_DEBUG / ASMJIT_TRACE]
// ============================================================================

// If ASMJIT_DEBUG and ASMJIT_RELEASE is not defined ASMJIT_DEBUG will be
// detected using the compiler specific macros. This enables to set the build
// type using IDE.
#if !defined(ASMJIT_DEBUG) && !defined(ASMJIT_RELEASE)
# if defined(_DEBUG)
#  define ASMJIT_DEBUG
# endif // _DEBUG
#endif // !ASMJIT_DEBUG && !ASMJIT_RELEASE

// ASMJIT_TRACE is only used by sources and private headers. It's safe to make
// it unavailable outside of AsmJit.
#if defined(ASMJIT_EXPORTS)
namespace asmjit { static inline int disabledTrace(...) { return 0; } }
# if defined(ASMJIT_TRACE)
#  define ASMJIT_TSEC(_Section_) _Section_
#  define ASMJIT_TLOG ::printf(__VA_ARGS__)
# else
#  define ASMJIT_TSEC(_Section_) do {} while(0)
#  define ASMJIT_TLOG 0 && ::asmjit::disabledTrace
# endif // ASMJIT_TRACE
#endif // ASMJIT_EXPORTS

// ============================================================================
// [asmjit::build - ASMJIT_UNUSED]
// ============================================================================

#if !defined(ASMJIT_UNUSED)
# define ASMJIT_UNUSED(_Var_) ((void)_Var_)
#endif // ASMJIT_UNUSED

// ============================================================================
// [asmjit::build - ASMJIT_NOP]
// ============================================================================

#if !defined(ASMJIT_NOP)
# define ASMJIT_NOP() ((void)0)
#endif // ASMJIT_NOP

// ============================================================================
// [asmjit::build - ASMJIT_NO_COPY]
// ============================================================================

#define ASMJIT_NO_COPY(_Type_) \
private: \
  ASMJIT_INLINE _Type_(const _Type_& other); \
  ASMJIT_INLINE _Type_& operator=(const _Type_& other); \
public:

// ============================================================================
// [asmjit::build - StdInt]
// ============================================================================

#if defined(__MINGW32__)
# include <sys/types.h>
#endif // __MINGW32__

#if defined(_MSC_VER) && (_MSC_VER < 1600)
# if !defined(ASMJIT_SUPRESS_STD_TYPES)
#  if (_MSC_VER < 1300)
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed __int64 int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#  else
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#  endif // _MSC_VER
# endif // ASMJIT_SUPRESS_STD_TYPES
#else
# include <stdint.h>
# include <limits.h>
#endif

#if defined(_MSC_VER)
# define ASMJIT_INT64_C(_Num_) _Num_##i64
# define ASMJIT_UINT64_C(_Num_) _Num_##ui64
#else
# define ASMJIT_INT64_C(_Num_) _Num_##LL
# define ASMJIT_UINT64_C(_Num_) _Num_##ULL
#endif

// ============================================================================
// [asmjit::build - Windows]
// ============================================================================

#if defined(ASMJIT_OS_WINDOWS) && !defined(ASMJIT_SUPRESS_WINDOWS_H)

# if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#  define ASMJIT_UNDEF_WIN32_LEAN_AND_MEAN
# endif // !WIN32_LEAN_AND_MEAN

# if !defined(NOMINMAX)
#  define NOMINMAX
#  define ASMJIT_UNDEF_NOMINMAX
# endif // !NOMINMAX

# include <windows.h>

# if defined(ASMJIT_UNDEF_NOMINMAX)
#  undef NOMINMAX
#  undef ASMJIT_UNDEF_NOMINMAX
# endif

# if defined(ASMJIT_UNDEF_WIN32_LEAN_AND_MEAN)
#  undef WIN32_LEAN_AND_MEAN
#  undef ASMJIT_UNDEF_WIN32_LEAN_AND_MEAN
# endif

#endif // ASMJIT_OS_WINDOWS  && !ASMJIT_SUPRESS_WINDOWS_H

// ============================================================================
// [asmjit::build - Test]
// ============================================================================

// Include a unit testing package if this is a `asmjit_test` build.
#if defined(ASMJIT_TEST)
#include "./test/broken.h"
#endif // ASMJIT_TEST

// [Guard]
#endif // _ASMJIT_BUILD_H
