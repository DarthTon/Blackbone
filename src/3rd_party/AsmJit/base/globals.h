// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_GLOBALS_H
#define _ASMJIT_BASE_GLOBALS_H

// [Dependencies - AsmJit]
#include "../build.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_general
//! \{

// ============================================================================
// [asmjit::Ptr / SignedPtr]
// ============================================================================

//! 64-bit unsigned pointer, compatible with JIT and non-JIT generators.
//!
//! This is the preferred pointer type to use with AsmJit library. It has a
//! capability to hold any pointer for any architecture making it an ideal
//! candidate for cross-platform code generation.
typedef uint64_t Ptr;

//! 64-bit signed pointer, like \ref Ptr, but made signed.
typedef int64_t SignedPtr;

// ============================================================================
// [asmjit::kGlobals]
// ============================================================================

//! Invalid index
//!
//! Invalid index is the last possible index that is never used in practice. In
//! AsmJit it is used exclusively with strings to indicate the the length of the
//! string is not known and has to be determined.
static const size_t kInvalidIndex = ~static_cast<size_t>(0);

//! Invalid base address.
static const Ptr kNoBaseAddress = static_cast<Ptr>(static_cast<SignedPtr>(-1));

//! Global constants.
ASMJIT_ENUM(kGlobals) {
  //! Invalid value or operand id.
  kInvalidValue = 0xFFFFFFFF,

  //! Invalid register index.
  kInvalidReg = 0xFF,
  //! Invalid variable type.
  kInvalidVar = 0xFF,

  //! Host memory allocator overhead.
  //!
  //! The overhead is decremented from all zone allocators so the operating
  //! system doesn't have allocate extra virtual page to keep tract of the
  //! requested memory block.
  //!
  //! The number is actually a guess.
  kMemAllocOverhead = sizeof(intptr_t) * 4,

  //! Memory grow threshold.
  //!
  //! After the grow threshold is reached the capacity won't be doubled
  //! anymore.
  kMemAllocGrowMax = 8192 * 1024
};

// ============================================================================
// [asmjit::kArch]
// ============================================================================

//! Architecture.
ASMJIT_ENUM(kArch) {
  //! No/Unknown architecture.
  kArchNone = 0,

  //! X86 architecture.
  kArchX86 = 1,
  //! X64 architecture, also called AMD64.
  kArchX64 = 2,

  //! Arm architecture.
  kArchArm = 4,

#if defined(ASMJIT_HOST_X86)
  kArchHost = kArchX86,
#endif // ASMJIT_HOST_X86

#if defined(ASMJIT_HOST_X64)
  kArchHost = kArchX64,
#endif // ASMJIT_HOST_X64

#if defined(ASMJIT_HOST_ARM)
  kArchHost = kArchArm,
#endif // ASMJIT_HOST_ARM

  //! Whether the host is 64-bit.
  kArchHost64Bit = sizeof(intptr_t) >= 8
};

//! \}

// ============================================================================
// [asmjit::Init / NoInit]
// ============================================================================

#if !defined(ASMJIT_DOCGEN)
struct _Init {};
static const _Init Init = {};

struct _NoInit {};
static const _NoInit NoInit = {};
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::Assert]
// ============================================================================

//! \addtogroup asmjit_base_general
//! \{

//! Called in debug build on assertion failure.
//!
//! \param exp Expression that failed.
//! \param file Source file name where it happened.
//! \param line Line in the source file.
//!
//! If you have problems with assertions put a breakpoint at assertionFailed()
//! function (asmjit/base/globals.cpp) and check the call stack to locate the
//! failing code.
ASMJIT_API void assertionFailed(const char* exp, const char* file, int line);

#if defined(ASMJIT_DEBUG)
#define ASMJIT_ASSERT(_Exp_) \
  do { \
    if (!(_Exp_)) ::asmjit::assertionFailed(#_Exp_, __FILE__, __LINE__); \
  } while (0)
#else
#define ASMJIT_ASSERT(_Exp_) ASMJIT_NOP()
#endif // DEBUG

//! \}

} // asmjit namespace

// ============================================================================
// [asmjit_cast<>]
// ============================================================================

//! \addtogroup asmjit_base_util
//! \{

//! Cast used to cast pointer to function. It's like reinterpret_cast<>,
//! but uses internally C style cast to work with MinGW.
//!
//! If you are using single compiler and `reinterpret_cast<>` works for you,
//! there is no reason to use `asmjit_cast<>`. If you are writing
//! cross-platform software with various compiler support, consider using
//! `asmjit_cast<>` instead of `reinterpret_cast<>`.
template<typename T, typename Z>
static ASMJIT_INLINE T asmjit_cast(Z* p) { return (T)p; }

//! \}

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_GLOBALS_H
