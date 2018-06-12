// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BUILD_H
#define _ASMJIT_BUILD_H

// ============================================================================
// [asmjit::Build - Configuration]
// ============================================================================

// AsmJit is by default compiled only for a host processor for the purpose of
// JIT code generation. Both Assembler and CodeCompiler emitters are compiled
// by default. Preprocessor macros can be used to change the default behavior.

// External Config File
// --------------------
//
// Define in case your configuration is generated in an external file to be
// included.

#if defined(ASMJIT_CONFIG_FILE)
# include ASMJIT_CONFIG_FILE
#endif // ASMJIT_CONFIG_FILE

// AsmJit Static Builds and Embedding
// ----------------------------------
//
// These definitions can be used to enable static library build. Embed is used
// when AsmJit's source code is embedded directly in another project, implies
// static build as well.
//
#define ASMJIT_EMBED              // Asmjit is embedded (implies ASMJIT_STATIC).
// #define ASMJIT_STATIC             // Define to enable static-library build.

// AsmJit Build Modes
// ------------------
//
// These definitions control the build mode and tracing support. The build mode
// should be auto-detected at compile time, but it's possible to override it in
// case that the auto-detection fails.
//
// Tracing is a feature that is never compiled by default and it's only used to
// debug AsmJit itself.
//
// #define ASMJIT_DEBUG              // Define to enable debug-mode.
// #define ASMJIT_RELEASE            // Define to enable release-mode.

// AsmJit Build Backends
// ---------------------
//
// These definitions control which backends to compile. If none of these is
// defined AsmJit will use host architecture by default (for JIT code generation).
//
#define ASMJIT_BUILD_X86          // Define to enable X86 and X64 code-generation.
// #define ASMJIT_BUILD_ARM          // Define to enable ARM32 and ARM64 code-generation.
// #define ASMJIT_BUILD_HOST         // Define to enable host instruction set.

// AsmJit Build Features
// ---------------------
//
// Flags can be defined to disable standard features. These are handy especially
// when building AsmJit statically and some features are not needed or unwanted
// (like CodeCompiler).
//
// AsmJit features are enabled by default.
 #define ASMJIT_DISABLE_COMPILER   // Disable CodeCompiler (completely).
 #define ASMJIT_DISABLE_LOGGING    // Disable logging and formatting (completely).
// #define ASMJIT_DISABLE_TEXT       // Disable everything that contains text
//                                   // representation (instructions, errors, ...).
// #define ASMJIT_DISABLE_VALIDATION // Disable Validation (completely).

// Prevent compile-time errors caused by misconfiguration.
#if defined(ASMJIT_DISABLE_TEXT) && !defined(ASMJIT_DISABLE_LOGGING)
# error "[asmjit] ASMJIT_DISABLE_TEXT requires ASMJIT_DISABLE_LOGGING to be defined."
#endif // ASMJIT_DISABLE_TEXT && !ASMJIT_DISABLE_LOGGING

// Detect ASMJIT_DEBUG and ASMJIT_RELEASE if not forced from outside.
#if !defined(ASMJIT_DEBUG) && !defined(ASMJIT_RELEASE)
# if !defined(NDEBUG)
#  define ASMJIT_DEBUG
# else
#  define ASMJIT_RELEASE
# endif
#endif

// ASMJIT_EMBED implies ASMJIT_STATIC.
#if defined(ASMJIT_EMBED) && !defined(ASMJIT_STATIC)
# define ASMJIT_STATIC
#endif

// ============================================================================
// [asmjit::Build - VERSION]
// ============================================================================

// [@VERSION{@]
#define ASMJIT_VERSION_MAJOR 1
#define ASMJIT_VERSION_MINOR 0
#define ASMJIT_VERSION_PATCH 0
#define ASMJIT_VERSION_STRING "1.0.0"
// [@VERSION}@]

// ============================================================================
// [asmjit::Build - WIN32]
// ============================================================================

// [@WIN32_CRT_NO_DEPRECATE{@]
#if defined(_MSC_VER) && defined(ASMJIT_EXPORTS)
# if !defined(_CRT_SECURE_NO_DEPRECATE)
#  define _CRT_SECURE_NO_DEPRECATE
# endif
# if !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS
# endif
#endif
// [@WIN32_CRT_NO_DEPRECATE}@]

// [@WIN32_LEAN_AND_MEAN{@]
#if (defined(_WIN32) || defined(_WINDOWS)) && !defined(_WINDOWS_)
# if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#  define ASMJIT_UNDEF_WIN32_LEAN_AND_MEAN
# endif
# if !defined(NOMINMAX)
#  define NOMINMAX
#  define ASMJIT_UNDEF_NOMINMAX
# endif
# include <windows.h>
# if defined(ASMJIT_UNDEF_NOMINMAX)
#  undef NOMINMAX
#  undef ASMJIT_UNDEF_NOMINMAX
# endif
# if defined(ASMJIT_UNDEF_WIN32_LEAN_AND_MEAN)
#  undef WIN32_LEAN_AND_MEAN
#  undef ASMJIT_UNDEF_WIN32_LEAN_AND_MEAN
# endif
#endif
// [@WIN32_LEAN_AND_MEAN}@]

// ============================================================================
// [asmjit::Build - OS]
// ============================================================================

// [@OS{@]
#if defined(_WIN32) || defined(_WINDOWS)
#define ASMJIT_OS_WINDOWS       (1)
#else
#define ASMJIT_OS_WINDOWS       (0)
#endif

#if defined(__APPLE__)
# include <TargetConditionals.h>
# define ASMJIT_OS_MAC          (TARGET_OS_MAC)
# define ASMJIT_OS_IOS          (TARGET_OS_IPHONE)
#else
# define ASMJIT_OS_MAC          (0)
# define ASMJIT_OS_IOS          (0)
#endif

#if defined(__ANDROID__)
# define ASMJIT_OS_ANDROID      (1)
#else
# define ASMJIT_OS_ANDROID      (0)
#endif

#if defined(__linux__) || defined(__ANDROID__)
# define ASMJIT_OS_LINUX        (1)
#else
# define ASMJIT_OS_LINUX        (0)
#endif

#if defined(__DragonFly__)
# define ASMJIT_OS_DRAGONFLYBSD (1)
#else
# define ASMJIT_OS_DRAGONFLYBSD (0)
#endif

#if defined(__FreeBSD__)
# define ASMJIT_OS_FREEBSD      (1)
#else
# define ASMJIT_OS_FREEBSD      (0)
#endif

#if defined(__NetBSD__)
# define ASMJIT_OS_NETBSD       (1)
#else
# define ASMJIT_OS_NETBSD       (0)
#endif

#if defined(__OpenBSD__)
# define ASMJIT_OS_OPENBSD      (1)
#else
# define ASMJIT_OS_OPENBSD      (0)
#endif

#if defined(__QNXNTO__)
# define ASMJIT_OS_QNX          (1)
#else
# define ASMJIT_OS_QNX          (0)
#endif

#if defined(__sun)
# define ASMJIT_OS_SOLARIS      (1)
#else
# define ASMJIT_OS_SOLARIS      (0)
#endif

#if defined(__CYGWIN__)
# define ASMJIT_OS_CYGWIN       (1)
#else
# define ASMJIT_OS_CYGWIN       (0)
#endif

#define ASMJIT_OS_BSD ( \
        ASMJIT_OS_FREEBSD       || \
        ASMJIT_OS_DRAGONFLYBSD  || \
        ASMJIT_OS_NETBSD        || \
        ASMJIT_OS_OPENBSD       || \
        ASMJIT_OS_MAC)
#define ASMJIT_OS_POSIX         (!ASMJIT_OS_WINDOWS)
// [@OS}@]

// ============================================================================
// [asmjit::Build - ARCH]
// ============================================================================

// [@ARCH{@]
// \def ASMJIT_ARCH_ARM32
// True if the target architecture is a 32-bit ARM.
//
// \def ASMJIT_ARCH_ARM64
// True if the target architecture is a 64-bit ARM.
//
// \def ASMJIT_ARCH_X86
// True if the target architecture is a 32-bit X86/IA32
//
// \def ASMJIT_ARCH_X64
// True if the target architecture is a 64-bit X64/AMD64
//
// \def ASMJIT_ARCH_LE
// True if the target architecture is little endian.
//
// \def ASMJIT_ARCH_BE
// True if the target architecture is big endian.
//
// \def ASMJIT_ARCH_64BIT
// True if the target architecture is 64-bit.

#if (defined(_M_X64  ) || defined(__x86_64) || defined(__x86_64__) || \
     defined(_M_AMD64) || defined(__amd64 ) || defined(__amd64__ ))
# define ASMJIT_ARCH_X64 1
#else
# define ASMJIT_ARCH_X64 0
#endif

#if (defined(_M_IX86 ) || defined(__X86__ ) || defined(__i386  ) || \
     defined(__IA32__) || defined(__I86__ ) || defined(__i386__) || \
     defined(__i486__) || defined(__i586__) || defined(__i686__))
# define ASMJIT_ARCH_X86 (!ASMJIT_ARCH_X64)
#else
# define ASMJIT_ARCH_X86 0
#endif

#if defined(__aarch64__)
# define ASMJIT_ARCH_ARM64 1
#else
# define ASMJIT_ARCH_ARM64 0
#endif

#if (defined(_M_ARM  ) || defined(__arm    ) || defined(__thumb__ ) || \
     defined(_M_ARMT ) || defined(__arm__  ) || defined(__thumb2__))
# define ASMJIT_ARCH_ARM32 (!ASMJIT_ARCH_ARM64)
#else
# define ASMJIT_ARCH_ARM32 0
#endif

#define ASMJIT_ARCH_LE    (  \
        ASMJIT_ARCH_X86   || \
        ASMJIT_ARCH_X64   || \
        ASMJIT_ARCH_ARM32 || \
        ASMJIT_ARCH_ARM64 )
#define ASMJIT_ARCH_BE (!(ASMJIT_ARCH_LE))
#define ASMJIT_ARCH_64BIT (ASMJIT_ARCH_X64 || ASMJIT_ARCH_ARM64)
// [@ARCH}@]

// [@ARCH_UNALIGNED_RW{@]
// \def ASMJIT_ARCH_UNALIGNED_16
// True if the target architecture allows unaligned 16-bit reads and writes.
//
// \def ASMJIT_ARCH_UNALIGNED_32
// True if the target architecture allows unaligned 32-bit reads and writes.
//
// \def ASMJIT_ARCH_UNALIGNED_64
// True if the target architecture allows unaligned 64-bit reads and writes.

#define ASMJIT_ARCH_UNALIGNED_16 (ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64)
#define ASMJIT_ARCH_UNALIGNED_32 (ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64)
#define ASMJIT_ARCH_UNALIGNED_64 (ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64)
// [@ARCH_UNALIGNED_RW}@]

// ============================================================================
// [asmjit::Build - CC]
// ============================================================================

// [@CC{@]
// \def ASMJIT_CC_CLANG
// Non-zero if the detected C++ compiler is CLANG (contains normalized CLANG version).
//
// \def ASMJIT_CC_CODEGEAR
// Non-zero if the detected C++ compiler is CODEGEAR or BORLAND (version not normalized).
//
// \def ASMJIT_CC_INTEL
// Non-zero if the detected C++ compiler is INTEL (version not normalized).
//
// \def ASMJIT_CC_GCC
// Non-zero if the detected C++ compiler is GCC (contains normalized GCC version).
//
// \def ASMJIT_CC_MSC
// Non-zero if the detected C++ compiler is MSC (contains normalized MSC version).
//
// \def ASMJIT_CC_MINGW
// Non-zero if the detected C++ compiler is MINGW32 (set to 32) or MINGW64 (set to 64).

#define ASMJIT_CC_CLANG    0
#define ASMJIT_CC_CODEGEAR 0
#define ASMJIT_CC_GCC      0
#define ASMJIT_CC_INTEL    0
#define ASMJIT_CC_MSC      0

// Intel masquerades as GCC, so check for it first.
#if defined(__INTEL_COMPILER)
# undef  ASMJIT_CC_INTEL
# define ASMJIT_CC_INTEL __INTEL_COMPILER
#elif defined(__CODEGEARC__)
# undef  ASMJIT_CC_CODEGEAR
# define ASMJIT_CC_CODEGEAR (__CODEGEARC__)
#elif defined(__BORLANDC__)
# undef  ASMJIT_CC_CODEGEAR
# define ASMJIT_CC_CODEGEAR (__BORLANDC__)
#elif defined(__clang__) && defined(__clang_minor__)
# undef  ASMJIT_CC_CLANG
# define ASMJIT_CC_CLANG (__clang_major__ * 10000000 + __clang_minor__ * 100000 + __clang_patchlevel__)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
# undef  ASMJIT_CC_GCC
# define ASMJIT_CC_GCC (__GNUC__ * 10000000 + __GNUC_MINOR__ * 100000 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER) && defined(_MSC_FULL_VER)
# undef  ASMJIT_CC_MSC
# if _MSC_VER == _MSC_FULL_VER / 10000
#  define ASMJIT_CC_MSC (_MSC_VER * 100000 + (_MSC_FULL_VER % 10000))
# else
#  define ASMJIT_CC_MSC (_MSC_VER * 100000 + (_MSC_FULL_VER % 100000))
# endif
#else
# error "[asmjit] Unable to detect the C/C++ compiler."
#endif

#if ASMJIT_CC_INTEL && (defined(__GNUC__) || defined(__clang__))
# define ASMJIT_CC_INTEL_COMPAT_MODE 1
# else
# define ASMJIT_CC_INTEL_COMPAT_MODE 0
#endif

#define ASMJIT_CC_CODEGEAR_EQ(x, y) (ASMJIT_CC_CODEGEAR == (((x) << 8) + (y)))
#define ASMJIT_CC_CODEGEAR_GE(x, y) (ASMJIT_CC_CODEGEAR >= (((x) << 8) + (y)))

#define ASMJIT_CC_CLANG_EQ(x, y, z) (ASMJIT_CC_CLANG == ((x) * 10000000 + (y) * 100000 + (z)))
#define ASMJIT_CC_CLANG_GE(x, y, z) (ASMJIT_CC_CLANG >= ((x) * 10000000 + (y) * 100000 + (z)))

#define ASMJIT_CC_GCC_EQ(x, y, z) (ASMJIT_CC_GCC == ((x) * 10000000 + (y) * 100000 + (z)))
#define ASMJIT_CC_GCC_GE(x, y, z) (ASMJIT_CC_GCC >= ((x) * 10000000 + (y) * 100000 + (z)))

#define ASMJIT_CC_INTEL_EQ(x, y) (ASMJIT_CC_INTEL == (((x) * 100) + (y)))
#define ASMJIT_CC_INTEL_GE(x, y) (ASMJIT_CC_INTEL >= (((x) * 100) + (y)))

#define ASMJIT_CC_MSC_EQ(x, y, z) (ASMJIT_CC_MSC == ((x) * 10000000 + (y) * 100000 + (z)))
#define ASMJIT_CC_MSC_GE(x, y, z) (ASMJIT_CC_MSC >= ((x) * 10000000 + (y) * 100000 + (z)))

#if defined(__MINGW64__)
# define ASMJIT_CC_MINGW 64
#elif defined(__MINGW32__)
# define ASMJIT_CC_MINGW 32
#else
# define ASMJIT_CC_MINGW 0
#endif

#if defined(__cplusplus)
# if __cplusplus >= 201103L
#  define ASMJIT_CC_CXX_VERSION __cplusplus
# elif defined(__GXX_EXPERIMENTAL_CXX0X__) || ASMJIT_CC_MSC_GE(18, 0, 0) || ASMJIT_CC_INTEL_GE(14, 0)
#  define ASMJIT_CC_CXX_VERSION 201103L
# else
#  define ASMJIT_CC_CXX_VERSION 199711L
# endif
#endif

#if !defined(ASMJIT_CC_CXX_VERSION)
# define ASMJIT_CC_CXX_VERSION 0
#endif
// [@CC}@]

// [@CC_FEATURES{@]
#if ASMJIT_CC_CLANG
# define ASMJIT_CC_HAS_ATTRIBUTE               (1)
# define ASMJIT_CC_HAS_ATTRIBUTE_ALIGNED       (__has_attribute(__aligned__))
# define ASMJIT_CC_HAS_ATTRIBUTE_ALWAYS_INLINE (__has_attribute(__always_inline__))
# define ASMJIT_CC_HAS_ATTRIBUTE_NOINLINE      (__has_attribute(__noinline__))
# define ASMJIT_CC_HAS_ATTRIBUTE_NORETURN      (__has_attribute(__noreturn__))
# define ASMJIT_CC_HAS_ATTRIBUTE_OPTIMIZE      (__has_attribute(__optimize__))
# define ASMJIT_CC_HAS_BUILTIN_ASSUME          (__has_builtin(__builtin_assume))
# define ASMJIT_CC_HAS_BUILTIN_ASSUME_ALIGNED  (__has_builtin(__builtin_assume_aligned))
# define ASMJIT_CC_HAS_BUILTIN_EXPECT          (__has_builtin(__builtin_expect))
# define ASMJIT_CC_HAS_BUILTIN_UNREACHABLE     (__has_builtin(__builtin_unreachable))
# define ASMJIT_CC_HAS_ALIGNAS                 (__has_extension(__cxx_alignas__))
# define ASMJIT_CC_HAS_ALIGNOF                 (__has_extension(__cxx_alignof__))
# define ASMJIT_CC_HAS_CONSTEXPR               (__has_extension(__cxx_constexpr__))
# define ASMJIT_CC_HAS_DECLTYPE                (__has_extension(__cxx_decltype__))
# define ASMJIT_CC_HAS_DEFAULT_FUNCTION        (__has_extension(__cxx_defaulted_functions__))
# define ASMJIT_CC_HAS_DELETE_FUNCTION         (__has_extension(__cxx_deleted_functions__))
# define ASMJIT_CC_HAS_FINAL                   (__has_extension(__cxx_override_control__))
# define ASMJIT_CC_HAS_INITIALIZER_LIST        (__has_extension(__cxx_generalized_initializers__))
# define ASMJIT_CC_HAS_LAMBDA                  (__has_extension(__cxx_lambdas__))
# define ASMJIT_CC_HAS_NATIVE_CHAR             (1)
# define ASMJIT_CC_HAS_NATIVE_WCHAR_T          (1)
# define ASMJIT_CC_HAS_NATIVE_CHAR16_T         (__has_extension(__cxx_unicode_literals__))
# define ASMJIT_CC_HAS_NATIVE_CHAR32_T         (__has_extension(__cxx_unicode_literals__))
# define ASMJIT_CC_HAS_NOEXCEPT                (__has_extension(__cxx_noexcept__))
# define ASMJIT_CC_HAS_NULLPTR                 (__has_extension(__cxx_nullptr__))
# define ASMJIT_CC_HAS_OVERRIDE                (__has_extension(__cxx_override_control__))
# define ASMJIT_CC_HAS_RVALUE                  (__has_extension(__cxx_rvalue_references__))
# define ASMJIT_CC_HAS_STATIC_ASSERT           (__has_extension(__cxx_static_assert__))
# define ASMJIT_CC_HAS_VARIADIC_TEMPLATES      (__has_extension(__cxx_variadic_templates__))
#endif

#if ASMJIT_CC_CODEGEAR
# define ASMJIT_CC_HAS_DECLSPEC_ALIGN          (ASMJIT_CC_CODEGEAR >= 0x0610)
# define ASMJIT_CC_HAS_DECLSPEC_FORCEINLINE    (0)
# define ASMJIT_CC_HAS_DECLSPEC_NOINLINE       (0)
# define ASMJIT_CC_HAS_DECLSPEC_NORETURN       (ASMJIT_CC_CODEGEAR >= 0x0610)
# define ASMJIT_CC_HAS_ALIGNAS                 (0)
# define ASMJIT_CC_HAS_ALIGNOF                 (0)
# define ASMJIT_CC_HAS_CONSTEXPR               (0)
# define ASMJIT_CC_HAS_DECLTYPE                (ASMJIT_CC_CODEGEAR >= 0x0610)
# define ASMJIT_CC_HAS_DEFAULT_FUNCTION        (0)
# define ASMJIT_CC_HAS_DELETE_FUNCTION         (0)
# define ASMJIT_CC_HAS_FINAL                   (0)
# define ASMJIT_CC_HAS_INITIALIZER_LIST        (0)
# define ASMJIT_CC_HAS_LAMBDA                  (0)
# define ASMJIT_CC_HAS_NATIVE_CHAR             (1)
# define ASMJIT_CC_HAS_NATIVE_WCHAR_T          (1)
# define ASMJIT_CC_HAS_NATIVE_CHAR16_T         (0)
# define ASMJIT_CC_HAS_NATIVE_CHAR32_T         (0)
# define ASMJIT_CC_HAS_NOEXCEPT                (0)
# define ASMJIT_CC_HAS_NULLPTR                 (0)
# define ASMJIT_CC_HAS_OVERRIDE                (0)
# define ASMJIT_CC_HAS_RVALUE                  (ASMJIT_CC_CODEGEAR >= 0x0610)
# define ASMJIT_CC_HAS_STATIC_ASSERT           (ASMJIT_CC_CODEGEAR >= 0x0610)
# define ASMJIT_CC_HAS_VARIADIC_TEMPLATES      (0)
#endif

#if ASMJIT_CC_GCC
# define ASMJIT_CC_HAS_ATTRIBUTE               (1)
# define ASMJIT_CC_HAS_ATTRIBUTE_ALIGNED       (ASMJIT_CC_GCC_GE(2, 7, 0))
# define ASMJIT_CC_HAS_ATTRIBUTE_ALWAYS_INLINE (ASMJIT_CC_GCC_GE(4, 4, 0) && !ASMJIT_CC_MINGW)
# define ASMJIT_CC_HAS_ATTRIBUTE_NOINLINE      (ASMJIT_CC_GCC_GE(3, 4, 0) && !ASMJIT_CC_MINGW)
# define ASMJIT_CC_HAS_ATTRIBUTE_NORETURN      (ASMJIT_CC_GCC_GE(2, 5, 0))
# define ASMJIT_CC_HAS_ATTRIBUTE_OPTIMIZE      (ASMJIT_CC_GCC_GE(4, 4, 0))
# define ASMJIT_CC_HAS_BUILTIN_ASSUME          (0)
# define ASMJIT_CC_HAS_BUILTIN_ASSUME_ALIGNED  (ASMJIT_CC_GCC_GE(4, 7, 0))
# define ASMJIT_CC_HAS_BUILTIN_EXPECT          (1)
# define ASMJIT_CC_HAS_BUILTIN_UNREACHABLE     (ASMJIT_CC_GCC_GE(4, 5, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_ALIGNAS                 (ASMJIT_CC_GCC_GE(4, 8, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_ALIGNOF                 (ASMJIT_CC_GCC_GE(4, 8, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_CONSTEXPR               (ASMJIT_CC_GCC_GE(4, 6, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_DECLTYPE                (ASMJIT_CC_GCC_GE(4, 3, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_DEFAULT_FUNCTION        (ASMJIT_CC_GCC_GE(4, 4, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_DELETE_FUNCTION         (ASMJIT_CC_GCC_GE(4, 4, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_FINAL                   (ASMJIT_CC_GCC_GE(4, 7, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_INITIALIZER_LIST        (ASMJIT_CC_GCC_GE(4, 4, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_LAMBDA                  (ASMJIT_CC_GCC_GE(4, 5, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_NATIVE_CHAR             (1)
# define ASMJIT_CC_HAS_NATIVE_WCHAR_T          (1)
# define ASMJIT_CC_HAS_NATIVE_CHAR16_T         (ASMJIT_CC_GCC_GE(4, 5, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_NATIVE_CHAR32_T         (ASMJIT_CC_GCC_GE(4, 5, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_NOEXCEPT                (ASMJIT_CC_GCC_GE(4, 6, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_NULLPTR                 (ASMJIT_CC_GCC_GE(4, 6, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_OVERRIDE                (ASMJIT_CC_GCC_GE(4, 7, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_RVALUE                  (ASMJIT_CC_GCC_GE(4, 3, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_STATIC_ASSERT           (ASMJIT_CC_GCC_GE(4, 3, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
# define ASMJIT_CC_HAS_VARIADIC_TEMPLATES      (ASMJIT_CC_GCC_GE(4, 3, 0) && ASMJIT_CC_CXX_VERSION >= 201103L)
#endif

#if ASMJIT_CC_INTEL
# define ASMJIT_CC_HAS_ATTRIBUTE               (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_ATTRIBUTE_ALIGNED       (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_ATTRIBUTE_ALWAYS_INLINE (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_ATTRIBUTE_NOINLINE      (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_ATTRIBUTE_NORETURN      (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_ATTRIBUTE_OPTIMIZE      (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_BUILTIN_EXPECT          (ASMJIT_CC_INTEL_COMPAT_MODE)
# define ASMJIT_CC_HAS_DECLSPEC_ALIGN          (ASMJIT_CC_INTEL_COMPAT_MODE == 0)
# define ASMJIT_CC_HAS_DECLSPEC_FORCEINLINE    (ASMJIT_CC_INTEL_COMPAT_MODE == 0)
# define ASMJIT_CC_HAS_DECLSPEC_NOINLINE       (ASMJIT_CC_INTEL_COMPAT_MODE == 0)
# define ASMJIT_CC_HAS_DECLSPEC_NORETURN       (ASMJIT_CC_INTEL_COMPAT_MODE == 0)
# define ASMJIT_CC_HAS_ASSUME                  (1)
# define ASMJIT_CC_HAS_ASSUME_ALIGNED          (1)
# define ASMJIT_CC_HAS_ALIGNAS                 (ASMJIT_CC_INTEL >= 1500)
# define ASMJIT_CC_HAS_ALIGNOF                 (ASMJIT_CC_INTEL >= 1500)
# define ASMJIT_CC_HAS_CONSTEXPR               (ASMJIT_CC_INTEL >= 1400)
# define ASMJIT_CC_HAS_DECLTYPE                (ASMJIT_CC_INTEL >= 1200)
# define ASMJIT_CC_HAS_DEFAULT_FUNCTION        (ASMJIT_CC_INTEL >= 1200)
# define ASMJIT_CC_HAS_DELETE_FUNCTION         (ASMJIT_CC_INTEL >= 1200)
# define ASMJIT_CC_HAS_FINAL                   (ASMJIT_CC_INTEL >= 1400)
# define ASMJIT_CC_HAS_INITIALIZER_LIST        (ASMJIT_CC_INTEL >= 1400)
# define ASMJIT_CC_HAS_LAMBDA                  (ASMJIT_CC_INTEL >= 1200)
# define ASMJIT_CC_HAS_NATIVE_CHAR             (1)
# define ASMJIT_CC_HAS_NATIVE_WCHAR_T          (1)
# define ASMJIT_CC_HAS_NATIVE_CHAR16_T         (ASMJIT_CC_INTEL >= 1400 || (ASMJIT_CC_INTEL_COMPAT_MODE > 0 && ASMJIT_CC_INTEL >= 1206))
# define ASMJIT_CC_HAS_NATIVE_CHAR32_T         (ASMJIT_CC_INTEL >= 1400 || (ASMJIT_CC_INTEL_COMPAT_MODE > 0 && ASMJIT_CC_INTEL >= 1206))
# define ASMJIT_CC_HAS_NOEXCEPT                (ASMJIT_CC_INTEL >= 1400)
# define ASMJIT_CC_HAS_NULLPTR                 (ASMJIT_CC_INTEL >= 1206)
# define ASMJIT_CC_HAS_OVERRIDE                (ASMJIT_CC_INTEL >= 1400)
# define ASMJIT_CC_HAS_RVALUE                  (ASMJIT_CC_INTEL >= 1110)
# define ASMJIT_CC_HAS_STATIC_ASSERT           (ASMJIT_CC_INTEL >= 1110)
# define ASMJIT_CC_HAS_VARIADIC_TEMPLATES      (ASMJIT_CC_INTEL >= 1206)
#endif

#if ASMJIT_CC_MSC
# define ASMJIT_CC_HAS_DECLSPEC_ALIGN          (1)
# define ASMJIT_CC_HAS_DECLSPEC_FORCEINLINE    (1)
# define ASMJIT_CC_HAS_DECLSPEC_NOINLINE       (1)
# define ASMJIT_CC_HAS_DECLSPEC_NORETURN       (1)
# define ASMJIT_CC_HAS_ASSUME                  (1)
# define ASMJIT_CC_HAS_ASSUME_ALIGNED          (0)
# define ASMJIT_CC_HAS_ALIGNAS                 (ASMJIT_CC_MSC_GE(19, 0, 0))
# define ASMJIT_CC_HAS_ALIGNOF                 (ASMJIT_CC_MSC_GE(19, 0, 0))
# define ASMJIT_CC_HAS_CONSTEXPR               (ASMJIT_CC_MSC_GE(19, 0, 0))
# define ASMJIT_CC_HAS_DECLTYPE                (ASMJIT_CC_MSC_GE(16, 0, 0))
# define ASMJIT_CC_HAS_DEFAULT_FUNCTION        (ASMJIT_CC_MSC_GE(18, 0, 0))
# define ASMJIT_CC_HAS_DELETE_FUNCTION         (ASMJIT_CC_MSC_GE(18, 0, 0))
# define ASMJIT_CC_HAS_FINAL                   (ASMJIT_CC_MSC_GE(14, 0, 0))
# define ASMJIT_CC_HAS_INITIALIZER_LIST        (ASMJIT_CC_MSC_GE(18, 0, 0))
# define ASMJIT_CC_HAS_LAMBDA                  (ASMJIT_CC_MSC_GE(16, 0, 0))
# define ASMJIT_CC_HAS_NATIVE_CHAR             (1)
# if defined(_NATIVE_WCHAR_T_DEFINED)
#  define ASMJIT_CC_HAS_NATIVE_WCHAR_T         (1)
# else
#  define ASMJIT_CC_HAS_NATIVE_WCHAR_T         (0)
# endif
# define ASMJIT_CC_HAS_NATIVE_CHAR16_T         (ASMJIT_CC_MSC_GE(19, 0, 0))
# define ASMJIT_CC_HAS_NATIVE_CHAR32_T         (ASMJIT_CC_MSC_GE(19, 0, 0))
# define ASMJIT_CC_HAS_NOEXCEPT                (ASMJIT_CC_MSC_GE(19, 0, 0))
# define ASMJIT_CC_HAS_NULLPTR                 (ASMJIT_CC_MSC_GE(16, 0, 0))
# define ASMJIT_CC_HAS_OVERRIDE                (ASMJIT_CC_MSC_GE(14, 0, 0))
# define ASMJIT_CC_HAS_RVALUE                  (ASMJIT_CC_MSC_GE(16, 0, 0))
# define ASMJIT_CC_HAS_STATIC_ASSERT           (ASMJIT_CC_MSC_GE(16, 0, 0))
# define ASMJIT_CC_HAS_VARIADIC_TEMPLATES      (ASMJIT_CC_MSC_GE(18, 0, 0))
#endif

// Fixup some vendor specific keywords.
#if !defined(ASMJIT_CC_HAS_ASSUME)
# define ASMJIT_CC_HAS_ASSUME                  (0)
#endif
#if !defined(ASMJIT_CC_HAS_ASSUME_ALIGNED)
# define ASMJIT_CC_HAS_ASSUME_ALIGNED          (0)
#endif

// Fixup compilers that don't support '__attribute__'.
#if !defined(ASMJIT_CC_HAS_ATTRIBUTE)
# define ASMJIT_CC_HAS_ATTRIBUTE               (0)
#endif
#if !defined(ASMJIT_CC_HAS_ATTRIBUTE_ALIGNED)
# define ASMJIT_CC_HAS_ATTRIBUTE_ALIGNED       (0)
#endif
#if !defined(ASMJIT_CC_HAS_ATTRIBUTE_ALWAYS_INLINE)
# define ASMJIT_CC_HAS_ATTRIBUTE_ALWAYS_INLINE (0)
#endif
#if !defined(ASMJIT_CC_HAS_ATTRIBUTE_NOINLINE)
# define ASMJIT_CC_HAS_ATTRIBUTE_NOINLINE      (0)
#endif
#if !defined(ASMJIT_CC_HAS_ATTRIBUTE_NORETURN)
# define ASMJIT_CC_HAS_ATTRIBUTE_NORETURN      (0)
#endif
#if !defined(ASMJIT_CC_HAS_ATTRIBUTE_OPTIMIZE)
# define ASMJIT_CC_HAS_ATTRIBUTE_OPTIMIZE      (0)
#endif

// Fixup compilers that don't support '__builtin?'.
#if !defined(ASMJIT_CC_HAS_BUILTIN_ASSUME)
# define ASMJIT_CC_HAS_BUILTIN_ASSUME          (0)
#endif
#if !defined(ASMJIT_CC_HAS_BUILTIN_ASSUME_ALIGNED)
# define ASMJIT_CC_HAS_BUILTIN_ASSUME_ALIGNED  (0)
#endif
#if !defined(ASMJIT_CC_HAS_BUILTIN_EXPECT)
# define ASMJIT_CC_HAS_BUILTIN_EXPECT          (0)
#endif
#if !defined(ASMJIT_CC_HAS_BUILTIN_UNREACHABLE)
# define ASMJIT_CC_HAS_BUILTIN_UNREACHABLE     (0)
#endif

// Fixup compilers that don't support 'declspec'.
#if !defined(ASMJIT_CC_HAS_DECLSPEC_ALIGN)
# define ASMJIT_CC_HAS_DECLSPEC_ALIGN          (0)
#endif
#if !defined(ASMJIT_CC_HAS_DECLSPEC_FORCEINLINE)
# define ASMJIT_CC_HAS_DECLSPEC_FORCEINLINE    (0)
#endif
#if !defined(ASMJIT_CC_HAS_DECLSPEC_NOINLINE)
# define ASMJIT_CC_HAS_DECLSPEC_NOINLINE       (0)
#endif
#if !defined(ASMJIT_CC_HAS_DECLSPEC_NORETURN)
# define ASMJIT_CC_HAS_DECLSPEC_NORETURN       (0)
#endif
// [@CC_FEATURES}@]

// [@CC_API{@]
// \def ASMJIT_API
// The decorated function is asmjit API and should be exported.
#if !defined(ASMJIT_API)
# if defined(ASMJIT_STATIC)
#  define ASMJIT_API
# elif ASMJIT_OS_WINDOWS
#  if (ASMJIT_CC_GCC || ASMJIT_CC_CLANG) && !ASMJIT_CC_MINGW
#   if defined(ASMJIT_EXPORTS)
#    define ASMJIT_API __attribute__((__dllexport__))
#   else
#    define ASMJIT_API __attribute__((__dllimport__))
#   endif
#  else
#   if defined(ASMJIT_EXPORTS)
#    define ASMJIT_API __declspec(dllexport)
#   else
#    define ASMJIT_API __declspec(dllimport)
#   endif
#  endif
# else
#  if ASMJIT_CC_CLANG || ASMJIT_CC_GCC_GE(4, 0, 0) || ASMJIT_CC_INTEL
#   define ASMJIT_API __attribute__((__visibility__("default")))
#  endif
# endif
#endif
// [@CC_API}@]

// [@CC_VARAPI{@]
// \def ASMJIT_VARAPI
// The decorated variable is part of asmjit API and is exported.
#if !defined(ASMJIT_VARAPI)
# define ASMJIT_VARAPI extern ASMJIT_API
#endif
// [@CC_VARAPI}@]

// [@CC_VIRTAPI{@]
// \def ASMJIT_VIRTAPI
// The decorated class has a virtual table and is part of asmjit API.
//
// This is basically a workaround. When using MSVC and marking class as DLL
// export everything gets exported, which is unwanted in most projects. MSVC
// automatically exports typeinfo and vtable if at least one symbol of the
// class is exported. However, GCC has some strange behavior that even if
// one or more symbol is exported it doesn't export typeinfo unless the
// class itself is decorated with "visibility(default)" (i.e. asmjit_API).
#if (ASMJIT_CC_GCC || ASMJIT_CC_CLANG) && !ASMJIT_OS_WINDOWS
# define ASMJIT_VIRTAPI ASMJIT_API
#else
# define ASMJIT_VIRTAPI
#endif
// [@CC_VIRTAPI}@]

// [@CC_INLINE{@]
// \def ASMJIT_INLINE
// Always inline the decorated function.
#if ASMJIT_CC_HAS_ATTRIBUTE_ALWAYS_INLINE
# define ASMJIT_INLINE inline __attribute__((__always_inline__))
#elif ASMJIT_CC_HAS_DECLSPEC_FORCEINLINE
# define ASMJIT_INLINE __forceinline
#else
# define ASMJIT_INLINE inline
#endif
// [@CC_INLINE}@]

// [@CC_NOINLINE{@]
// \def ASMJIT_NOINLINE
// Never inline the decorated function.
#if ASMJIT_CC_HAS_ATTRIBUTE_NOINLINE
# define ASMJIT_NOINLINE __attribute__((__noinline__))
#elif ASMJIT_CC_HAS_DECLSPEC_NOINLINE
# define ASMJIT_NOINLINE __declspec(noinline)
#else
# define ASMJIT_NOINLINE
#endif
// [@CC_NOINLINE}@]

// [@CC_NORETURN{@]
// \def ASMJIT_NORETURN
// The decorated function never returns (exit, assertion failure, etc...).
#if ASMJIT_CC_HAS_ATTRIBUTE_NORETURN
# define ASMJIT_NORETURN __attribute__((__noreturn__))
#elif ASMJIT_CC_HAS_DECLSPEC_NORETURN
# define ASMJIT_NORETURN __declspec(noreturn)
#else
# define ASMJIT_NORETURN
#endif
// [@CC_NORETURN}@]

// [@CC_CDECL{@]
// \def ASMJIT_CDECL
// Standard C function calling convention decorator (__cdecl).
#if ASMJIT_ARCH_X86
# if ASMJIT_CC_HAS_ATTRIBUTE
#  define ASMJIT_CDECL __attribute__((__cdecl__))
# else
#  define ASMJIT_CDECL __cdecl
# endif
#else
# define ASMJIT_CDECL
#endif
// [@CC_CDECL}@]

// [@CC_STDCALL{@]
// \def ASMJIT_STDCALL
// StdCall function calling convention decorator (__stdcall).
#if ASMJIT_ARCH_X86
# if ASMJIT_CC_HAS_ATTRIBUTE
#  define ASMJIT_STDCALL __attribute__((__stdcall__))
# else
#  define ASMJIT_STDCALL __stdcall
# endif
#else
# define ASMJIT_STDCALL
#endif
// [@CC_STDCALL}@]

// [@CC_FASTCALL{@]
// \def ASMJIT_FASTCALL
// FastCall function calling convention decorator (__fastcall).
#if ASMJIT_ARCH_X86
# if ASMJIT_CC_HAS_ATTRIBUTE
#  define ASMJIT_FASTCALL __attribute__((__fastcall__))
# else
#  define ASMJIT_FASTCALL __fastcall
# endif
#else
# define ASMJIT_FASTCALL
#endif
// [@CC_FASTCALL}@]

// [@CC_REGPARM{@]
// \def ASMJIT_REGPARM(n)
// A custom calling convention which passes n arguments in registers.
#if ASMJIT_ARCH_X86 && ASMJIT_CC_HAS_ATTRIBUTE
# define ASMJIT_REGPARM(n) __attribute__((__regparm__(n)))
#else
# define ASMJIT_REGPARM(n)
#endif
// [@CC_REGPARM}@]

// [@CC_NOEXCEPT{@]
// \def ASMJIT_NOEXCEPT
// The decorated function never throws an exception (noexcept).
#if ASMJIT_CC_HAS_NOEXCEPT
# define ASMJIT_NOEXCEPT noexcept
#else
# define ASMJIT_NOEXCEPT
#endif
// [@CC_NOEXCEPT}@]

// [@CC_NOP{@]
// \def ASMJIT_NOP
// No operation.
#if !defined(ASMJIT_NOP)
# define ASMJIT_NOP ((void)0)
#endif
// [@CC_NOP}@]

// [@CC_ASSUME{@]
// \def ASMJIT_ASSUME(exp)
// Assume that the expression exp is always true.
#if ASMJIT_CC_HAS_ASSUME
# define ASMJIT_ASSUME(exp) __assume(exp)
#elif ASMJIT_CC_HAS_BUILTIN_ASSUME
# define ASMJIT_ASSUME(exp) __builtin_assume(exp)
#elif ASMJIT_CC_HAS_BUILTIN_UNREACHABLE
# define ASMJIT_ASSUME(exp) do { if (!(exp)) __builtin_unreachable(); } while (0)
#else
# define ASMJIT_ASSUME(exp) ((void)0)
#endif
// [@CC_ASSUME}@]

// [@CC_ASSUME_ALIGNED{@]
// \def ASMJIT_ASSUME_ALIGNED(p, alignment)
// Assume that the pointer 'p' is aligned to at least 'alignment' bytes.
#if ASMJIT_CC_HAS_ASSUME_ALIGNED
# define ASMJIT_ASSUME_ALIGNED(p, alignment) __assume_aligned(p, alignment)
#elif ASMJIT_CC_HAS_BUILTIN_ASSUME_ALIGNED
# define ASMJIT_ASSUME_ALIGNED(p, alignment) p = __builtin_assume_aligned(p, alignment)
#else
# define ASMJIT_ASSUME_ALIGNED(p, alignment) ((void)0)
#endif
// [@CC_ASSUME_ALIGNED}@]

// [@CC_EXPECT{@]
// \def ASMJIT_LIKELY(exp)
// Expression exp is likely to be true.
//
// \def ASMJIT_UNLIKELY(exp)
// Expression exp is likely to be false.
#if ASMJIT_CC_HAS_BUILTIN_EXPECT
# define ASMJIT_LIKELY(exp) __builtin_expect(!!(exp), 1)
# define ASMJIT_UNLIKELY(exp) __builtin_expect(!!(exp), 0)
#else
# define ASMJIT_LIKELY(exp) (exp)
# define ASMJIT_UNLIKELY(exp) (exp)
#endif
// [@CC_EXPECT}@]

// [@CC_FALLTHROUGH{@]
// \def ASMJIT_FALLTHROUGH
// The code falls through annotation (switch / case).
#if ASMJIT_CC_CLANG && __cplusplus >= 201103L
# define ASMJIT_FALLTHROUGH [[clang::fallthrough]]
#else
# define ASMJIT_FALLTHROUGH (void)0
#endif
// [@CC_FALLTHROUGH}@]

// [@CC_UNUSED{@]
// \def ASMJIT_UNUSED(x)
// Mark a variable x as unused.
#define ASMJIT_UNUSED(x) (void)(x)
// [@CC_UNUSED}@]

// [@CC_OFFSET_OF{@]
// \def ASMJIT_OFFSET_OF(x, y).
// Get the offset of a member y of a struct x at compile-time.
#define ASMJIT_OFFSET_OF(x, y) ((int)(intptr_t)((const char*)&((const x*)0x1)->y) - 1)
// [@CC_OFFSET_OF}@]

// [@CC_ARRAY_SIZE{@]
// \def ASMJIT_ARRAY_SIZE(x)
// Get the array size of x at compile-time.
#define ASMJIT_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
// [@CC_ARRAY_SIZE}@]

// ============================================================================
// [asmjit::Build - STDTYPES]
// ============================================================================

// [@STDTYPES{@]
#if defined(__MINGW32__) || defined(__MINGW64__)
# include <sys/types.h>
#endif
#if defined(_MSC_VER) && (_MSC_VER < 1600)
# include <limits.h>
# if !defined(ASMJIT_SUPPRESS_STD_TYPES)
#  if (_MSC_VER < 1300)
typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed __int64   int64_t;
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned __int64 uint64_t;
#  else
typedef __int8           int8_t;
typedef __int16          int16_t;
typedef __int32          int32_t;
typedef __int64          int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#  endif
# endif
#else
# include <stdint.h>
# include <limits.h>
#endif
// [@STDTYPES}@]

// ============================================================================
// [asmjit::Build - Dependencies]
// ============================================================================

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#if ASMJIT_OS_POSIX
# include <pthread.h>
#endif // ASMJIT_OS_POSIX

// ============================================================================
// [asmjit::Build - Additional]
// ============================================================================

// Build host architecture if no architecture is selected.
#if !defined(ASMJIT_BUILD_HOST) && \
    !defined(ASMJIT_BUILD_X86)  && \
    !defined(ASMJIT_BUILD_ARM)
# define ASMJIT_BUILD_HOST
#endif

// Detect host architecture if building only for host.
#if defined(ASMJIT_BUILD_HOST)
# if (ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64) && !defined(ASMJIT_BUILD_X86)
#  define ASMJIT_BUILD_X86
# endif // ASMJIT_ARCH_X86
#endif // ASMJIT_BUILD_HOST

#if ASMJIT_CC_MSC
# define ASMJIT_UINT64_C(x) x##ui64
#else
# define ASMJIT_UINT64_C(x) x##ull
#endif

#if ASMJIT_ARCH_LE
# define ASMJIT_PACK32_4x8(A, B, C, D) ((A) + ((B) << 8) + ((C) << 16) + ((D) << 24))
#else
# define ASMJIT_PACK32_4x8(A, B, C, D) ((D) + ((C) << 8) + ((B) << 16) + ((A) << 24))
#endif

// Internal macros that are only used when building AsmJit itself.
#if defined(ASMJIT_EXPORTS)
# if !defined(ASMJIT_DEBUG) && ASMJIT_CC_HAS_ATTRIBUTE_OPTIMIZE
#  define ASMJIT_FAVOR_SIZE __attribute__((__optimize__("Os")))
# else
#  define ASMJIT_FAVOR_SIZE
# endif
#endif // ASMJIT_EXPORTS

// ============================================================================
// [asmjit::Build - Test]
// ============================================================================

// Include a unit testing package if this is a `asmjit_test` build.
#if defined(ASMJIT_TEST)
# include "../../test/broken.h"
#endif // ASMJIT_TEST

// [Guard]
#endif // _ASMJIT_BUILD_H
