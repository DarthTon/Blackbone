// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// This file is designed to be modifyable. Platform specific changes should
// be applied to this file so it's guaranteed that never versions of AsmJit
// library will never overwrite generated config files.
//
// So modify this file by your build system or by hand.

// [Guard]
#ifndef _ASMJIT_CONFIG_H
#define _ASMJIT_CONFIG_H

// ============================================================================
// [AsmJit - OS]
// ============================================================================

// Provides definitions about your operating system. It's detected by default,
// so override it if you have problems with automatic detection.
//
// #define ASMJIT_WINDOWS 1
// #define ASMJIT_POSIX 2

// ============================================================================
// [AsmJit - Architecture]
// ============================================================================

// Provides definitions about your cpu architecture. It's detected by default,
// so override it if you have problems with automatic detection.

// #define ASMJIT_X86
// #define ASMJIT_X64

// ============================================================================
// [AsmJit - API]
// ============================================================================

// If you are embedding AsmJit library into your project (statically), undef
// ASMJIT_API macro. ASMJIT_HIDDEN macro can contain visibility (used by GCC)
// to hide some AsmJit symbols that shouldn't be never exported.
//
// If you have problems with throw() in compilation time, undef ASMJIT_NOTHROW
// to disable this feature. ASMJIT_NOTHROW marks functions that never throws
// an exception.

// #define ASMJIT_HIDDEN
#define ASMJIT_API
// #define ASMJIT_NOTHROW


// ============================================================================
// [AsmJit - Memory Management]
// ============================================================================

// #define ASMJIT_MALLOC ::malloc
// #define ASMJIT_REALLOC ::realloc
// #define ASMJIT_FREE ::free

// ============================================================================
// [AsmJit - Debug]
// ============================================================================

// Turn debug on/off (to bypass autodetection)
// #define ASMJIT_DEBUG
// #define ASMJIT_NO_DEBUG

// Setup custom assertion code.
// #define ASMJIT_ASSERT(exp) do { if (!(exp)) ::AsmJit::assertionFailure(__FILE__, __LINE__, #exp); } while(0)

// [Guard]
#endif // _ASMJIT_CONFIG_H
