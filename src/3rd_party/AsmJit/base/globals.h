// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_GLOBALS_H
#define _ASMJIT_BASE_GLOBALS_H

// [Dependencies]
#include "../asmjit_build.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::Globals]
// ============================================================================

enum { kInvalidValue = 0xFFFFFFFFU };

//! AsmJit globals.
namespace Globals {

//! Invalid index
//!
//! Invalid index is the last possible index that is never used in practice. In
//! AsmJit it is used exclusively with strings to indicate the the length of the
//! string is not known and has to be determined.
static const size_t kInvalidIndex = ~static_cast<size_t>(0);

//! Invalid base address.
static const uint64_t kNoBaseAddress = ~static_cast<uint64_t>(0);

//! Global definitions.
ASMJIT_ENUM(Defs) {
  //! Invalid register id.
  kInvalidRegId = 0xFF,

  //! Host memory allocator overhead.
  kAllocOverhead = static_cast<int>(sizeof(intptr_t) * 4),
  //! Aggressive growing strategy threshold.
  kAllocThreshold = 8192 * 1024
};

ASMJIT_ENUM(Limits) {
  //! Count of register kinds that are important to Function API and CodeCompiler.
  //! The target architecture can define more register kinds for special registers,
  //! but these will never map to virtual registers and will never be used to pass
  //! and return function arguments and function return values, respectively.
  kMaxVRegKinds = 4,

  //! Maximum number of physical registers of all kinds of all supported
  //! architectures. This is only important for \ref CodeCompiler and its
  //! \ref RAPass (register allocator pass).
  //!
  //! NOTE: The distribution of these registers is architecture specific.
  kMaxPhysRegs = 64,

  //! Maximum alignment.
  kMaxAlignment = 64,

  //! Maximum label or symbol length in bytes (take into consideration that a
  //! single UTF-8 character can take more than single byte to encode it).
  kMaxLabelLength = 2048
};

} // Globals namespace

// ============================================================================
// [asmjit::Error]
// ============================================================================

//! AsmJit error type (uint32_t).
typedef uint32_t Error;

//! AsmJit error codes.
ASMJIT_ENUM(ErrorCode) {
  //! No error (success).
  //!
  //! This is default state and state you want.
  kErrorOk = 0,

  //! Heap memory allocation failed.
  kErrorNoHeapMemory,

  //! Virtual memory allocation failed.
  kErrorNoVirtualMemory,

  //! Invalid argument.
  kErrorInvalidArgument,

  //! Invalid state.
  //!
  //! If this error is returned it means that either you are doing something
  //! wrong or AsmJit caught itself by doing something wrong. This error should
  //! not be underestimated.
  kErrorInvalidState,

  //! Invalid or incompatible architecture.
  kErrorInvalidArch,

  //! The object is not initialized.
  kErrorNotInitialized,
  //! The object is already initialized.
  kErrorAlreadyInitialized,

  //! Built-in feature was disabled at compile time and it's not available.
  kErrorFeatureNotEnabled,

  //! CodeHolder can't have attached more than one \ref Assembler at a time.
  kErrorSlotOccupied,

  //! No code generated.
  //!
  //! Returned by runtime if the \ref CodeHolder contains no code.
  kErrorNoCodeGenerated,
  //! Code generated is larger than allowed.
  kErrorCodeTooLarge,

  //! Attempt to use uninitialized label.
  kErrorInvalidLabel,
  //! Label index overflow - a single `Assembler` instance can hold more than
  //! 2 billion labels (2147483391 to be exact). If there is an attempt to
  //! create more labels this error is returned.
  kErrorLabelIndexOverflow,
  //! Label is already bound.
  kErrorLabelAlreadyBound,
  //! Label is already defined (named labels).
  kErrorLabelAlreadyDefined,
  //! Label name is too long.
  kErrorLabelNameTooLong,
  //! Label must always be local if it's anonymous (without a name).
  kErrorInvalidLabelName,
  //! Parent id passed to `CodeHolder::newNamedLabelId()` was invalid.
  kErrorInvalidParentLabel,
  //! Parent id specified for a non-local (global) label.
  kErrorNonLocalLabelCantHaveParent,

  //! Relocation index overflow.
  kErrorRelocIndexOverflow,
  //! Invalid relocation entry.
  kErrorInvalidRelocEntry,

  //! Invalid instruction.
  kErrorInvalidInstruction,
  //! Invalid register type.
  kErrorInvalidRegType,
  //! Invalid register kind.
  kErrorInvalidRegKind,
  //! Invalid register's physical id.
  kErrorInvalidPhysId,
  //! Invalid register's virtual id.
  kErrorInvalidVirtId,
  //! Invalid prefix combination.
  kErrorInvalidPrefixCombination,
  //! Invalid LOCK prefix.
  kErrorInvalidLockPrefix,
  //! Invalid XACQUIRE prefix.
  kErrorInvalidXAcquirePrefix,
  //! Invalid XACQUIRE prefix.
  kErrorInvalidXReleasePrefix,
  //! Invalid REP prefix.
  kErrorInvalidRepPrefix,
  //! Invalid REX prefix.
  kErrorInvalidRexPrefix,
  //! Invalid mask register (not 'k').
  kErrorInvalidKMaskReg,
  //! Invalid {k} use (not supported by the instruction).
  kErrorInvalidKMaskUse,
  //! Invalid {k}{z} use (not supported by the instruction).
  kErrorInvalidKZeroUse,
  //! Invalid broadcast - Currently only related to invalid use of AVX-512 {1tox}.
  kErrorInvalidBroadcast,
  //! Invalid 'embedded-rounding' {er} or 'suppress-all-exceptions' {sae} (AVX-512).
  kErrorInvalidEROrSAE,
  //! Invalid address used (not encodable).
  kErrorInvalidAddress,
  //! Invalid index register used in memory address (not encodable).
  kErrorInvalidAddressIndex,
  //! Invalid address scale (not encodable).
  kErrorInvalidAddressScale,
  //! Invalid use of 64-bit address.
  kErrorInvalidAddress64Bit,
  //! Invalid displacement (not encodable).
  kErrorInvalidDisplacement,
  //! Invalid segment (X86).
  kErrorInvalidSegment,

  //! Invalid immediate (out of bounds on X86 and invalid pattern on ARM).
  kErrorInvalidImmediate,

  //! Invalid operand size.
  kErrorInvalidOperandSize,
  //! Ambiguous operand size (memory has zero size while it's required to determine the operation type.
  kErrorAmbiguousOperandSize,
  //! Mismatching operand size (size of multiple operands doesn't match the operation size).
  kErrorOperandSizeMismatch,

  //! Invalid TypeId.
  kErrorInvalidTypeId,
  //! Invalid use of a 8-bit GPB-HIGH register.
  kErrorInvalidUseOfGpbHi,
  //! Invalid use of a 64-bit GPQ register in 32-bit mode.
  kErrorInvalidUseOfGpq,
  //! Invalid use of an 80-bit float (TypeId::kF80).
  kErrorInvalidUseOfF80,
  //! Some registers in the instruction muse be consecutive (some ARM and AVX512 neural-net instructions).
  kErrorNotConsecutiveRegs,

  //! AsmJit requires a physical register, but no one is available.
  kErrorNoMorePhysRegs,
  //! A variable has been assigned more than once to a function argument (CodeCompiler).
  kErrorOverlappedRegs,
  //! Invalid register to hold stack arguments offset.
  kErrorOverlappingStackRegWithRegArg,

  //! Count of AsmJit error codes.
  kErrorCount
};

// ============================================================================
// [asmjit::Internal]
// ============================================================================

namespace Internal {

#if defined(ASMJIT_CUSTOM_ALLOC)   && \
    defined(ASMJIT_CUSTOM_REALLOC) && \
    defined(ASMJIT_CUSTOM_FREE)
static ASMJIT_INLINE void* allocMemory(size_t size) noexcept { return ASMJIT_CUSTOM_ALLOC(size); }
static ASMJIT_INLINE void* reallocMemory(void* p, size_t size) noexcept { return ASMJIT_CUSTOM_REALLOC(p, size); }
static ASMJIT_INLINE void releaseMemory(void* p) noexcept { ASMJIT_CUSTOM_FREE(p); }
#elif !defined(ASMJIT_CUSTOM_ALLOC)   && \
      !defined(ASMJIT_CUSTOM_REALLOC) && \
      !defined(ASMJIT_CUSTOM_FREE)
static ASMJIT_INLINE void* allocMemory(size_t size) noexcept { return ::malloc(size); }
static ASMJIT_INLINE void* reallocMemory(void* p, size_t size) noexcept { return ::realloc(p, size); }
static ASMJIT_INLINE void releaseMemory(void* p) noexcept { ::free(p); }
#else
# error "[asmjit] You must provide either none or all of ASMJIT_CUSTOM_[ALLOC|REALLOC|FREE]"
#endif

//! Cast designed to cast between function and void* pointers.
template<typename Dst, typename Src>
static ASMJIT_INLINE Dst ptr_cast(Src p) noexcept { return (Dst)p; }

} // Internal namespace

template<typename Func>
static ASMJIT_INLINE Func ptr_as_func(void* func) noexcept { return Internal::ptr_cast<Func, void*>(func); }

template<typename Func>
static ASMJIT_INLINE void* func_as_ptr(Func func) noexcept { return Internal::ptr_cast<void*, Func>(func); }

// ============================================================================
// [asmjit::DebugUtils]
// ============================================================================

namespace DebugUtils {

//! Returns the error `err` passed.
//!
//! Provided for debugging purposes. Putting a breakpoint inside `errored` can
//! help with tracing the origin of any error reported / returned by AsmJit.
static ASMJIT_INLINE Error errored(Error err) noexcept { return err; }

//! Get a printable version of `asmjit::Error` code.
ASMJIT_API const char* errorAsString(Error err) noexcept;

//! Called to output debugging message(s).
ASMJIT_API void debugOutput(const char* str) noexcept;

//! Called on assertion failure.
//!
//! \param file Source file name where it happened.
//! \param line Line in the source file.
//! \param msg Message to display.
//!
//! If you have problems with assertions put a breakpoint at assertionFailed()
//! function (asmjit/base/globals.cpp) and check the call stack to locate the
//! failing code.
ASMJIT_API void ASMJIT_NORETURN assertionFailed(const char* file, int line, const char* msg) noexcept;

#if defined(ASMJIT_DEBUG)
# define ASMJIT_ASSERT(exp)                                          \
  do {                                                               \
    if (ASMJIT_LIKELY(exp))                                          \
      break;                                                         \
    ::asmjit::DebugUtils::assertionFailed(__FILE__, __LINE__, #exp); \
  } while (0)
# define ASMJIT_NOT_REACHED()                                        \
  do {                                                               \
    ::asmjit::DebugUtils::assertionFailed(__FILE__, __LINE__,        \
      "ASMJIT_NOT_REACHED has been reached");                        \
    ASMJIT_ASSUME(0);                                                \
  } while (0)
#else
# define ASMJIT_ASSERT(exp) ASMJIT_NOP
# define ASMJIT_NOT_REACHED() ASMJIT_ASSUME(0)
#endif // DEBUG

//! \internal
//!
//! Used by AsmJit to propagate a possible `Error` produced by `...` to the caller.
#define ASMJIT_PROPAGATE(...)               \
  do {                                      \
    ::asmjit::Error _err = __VA_ARGS__;     \
    if (ASMJIT_UNLIKELY(_err))              \
      return _err;                          \
  } while (0)

} // DebugUtils namespace

// ============================================================================
// [asmjit::Init / NoInit]
// ============================================================================

#if !defined(ASMJIT_DOCGEN)
struct _Init {};
static const _Init Init = {};

struct _NoInit {};
static const _NoInit NoInit = {};
#endif // !ASMJIT_DOCGEN

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_GLOBALS_H
