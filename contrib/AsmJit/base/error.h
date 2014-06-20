// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_ERROR_H
#define _ASMJIT_BASE_ERROR_H

// [Api-Begin]
#include "../apibegin.h"

// [Dependencies - AsmJit]
#include "../base/globals.h"

namespace asmjit {

//! \addtogroup asmjit_base_general
//! \{

// ============================================================================
// [asmjit::kError]
// ============================================================================

//! AsmJit error codes.
ASMJIT_ENUM(kError) {
  //! No error (success).
  //!
  //! This is default state and state you want.
  kErrorOk = 0,

  //! Heap memory allocation failed.
  kErrorNoHeapMemory = 1,

  //! Virtual memory allocation failed.
  kErrorNoVirtualMemory = 2,

  //! Invalid argument.
  kErrorInvalidArgument = 3,

  //! Invalid state.
  kErrorInvalidState = 4,

  //! Unknown instruction. This happens only if instruction code is
  //! out of bounds. Shouldn't happen.
  kErrorUnknownInst = 5,

  //! Illegal instruction (Assembler).
  //!
  //! This status code can also be returned in X64 mode if AH, BH, CH or DH
  //! registers have been used together with a REX prefix. The instruction
  //! is not encodable in such case.
  //!
  //! Example of raising `kErrorIllegalInst` error.
  //!
  //! ~~~
  //! // Invalid address size.
  //! a->mov(dword_ptr(eax), al);
  //!
  //! // Undecodable instruction - AH used with R10, however R10 can only be
  //! // encoded by using REX prefix, which conflicts with AH.
  //! a->mov(byte_ptr(r10), ah);
  //! ~~~
  //!
  //! \note In debug mode assertion is raised instead of returning an error.
  kErrorIllegalInst = 6,

  //! Illegal (unencodable) addressing used (Assembler).
  kErrorIllegalAddresing = 7,

  //! Illegal (unencodable) displacement used (Assembler).
  //!
  //! X86/X64
  //! -------
  //!
  //! Short form of jump instruction has been used, but the displacement is out
  //! of bounds.
  kErrorIllegalDisplacement = 8,

  //! Invalid function (Compiler).
  //!
  //! Returned if no function is defined, but `make()` has been called.
  kErrorInvalidFunction = 9,

  //! A variable has been assigned more than once to a function argument (Compiler).
  kErrorOverlappedArgs = 10,

  //! Count of AsmJit status codes. Can grow in future.
  kErrorCount = 11
};

// ============================================================================
// [asmjit::Error]
// ============================================================================

//! AsmJit error type (unsigned integer).
typedef uint32_t Error;

// ============================================================================
// [asmjit::ErrorHandler]
// ============================================================================

//! Error handler.
//!
//! Error handler can be used to override the default behavior of `CodeGen`
//! error handling and propagation. See `handleError` on how to override it.
//!
//! Please note that `addRef` and `release` functions are used, but there is
//! no reference counting implemented by default, reimplement to change the
//! default behavior.
struct ErrorHandler {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `ErrorHandler` instance.
  ASMJIT_API ErrorHandler();
  //! Destroy the `ErrorHandler` instance.
  ASMJIT_API virtual ~ErrorHandler();

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Reference this error handler.
  //!
  //! \note This member function is provided for convenience. The default
  //! implementation does nothing. If you are working in environment where
  //! multiple `ErrorHandler` instances are used by a different code generators
  //! you may provide your own functionality for reference counting. In that
  //! case `addRef()` and `release()` functions should be overridden.
  ASMJIT_API virtual ErrorHandler* addRef() const;

  //! Release this error handler.
  //!
  //! \note This member function is provided for convenience. See `addRef()`
  //! for more detailed information related to reference counting.
  ASMJIT_API virtual void release();

  //! Error handler (pure).
  //!
  //! Error handler is called when an error happened. An error can happen in
  //! many places, but error handler is mostly used by `BaseAssembler` and
  //! `BaseCompiler` classes to report anything that may cause incorrect code
  //! generation. There are multiple ways how the error handler can be used
  //! and each has it's pros/cons.
  //!
  //! AsmJit library doesn't use exceptions and can be compiled with or without
  //! exception handling support. Even if the AsmJit library is compiled without
  //! exceptions it is exception-safe and handleError() can report an incoming
  //! error by throwing an exception of any type. It's guaranteed that the
  //! exception won't be catched by AsmJit and will be propagated to the code
  //! calling AsmJit `BaseAssembler` or `BaseCompiler` methods. Alternative to
  //! throwing an exception is using `setjmp()` and `longjmp()` pair available
  //! in the standard C library.
  //!
  //! If the exception or setjmp() / longjmp() mechanism is used, the state of
  //! the `BaseAssember` or `BaseCompiler` is unchanged and if it's possible the
  //! execution (instruction serialization) can continue. However if the error
  //! happened during any phase that translates or modifies the stored code
  //! (for example relocation done by `BaseAssembler` or analysis/translation
  //! done by `BaseCompiler`) the execution can't continue and the error will
  //! be also stored in `BaseAssembler` or `BaseCompiler`.
  //!
  //! Finally, if no exceptions nor setjmp() / longjmp() mechanisms were used,
  //! you can still implement a compatible handling by returning from your
  //! error handler. Returning `true` means that error was reported and AsmJit
  //! should continue execution, but `false` sets the rror immediately to the
  //! `BaseAssembler` or `BaseCompiler` and execution shouldn't continue (this
  //! is the default behavior in case no error handler is used).
  virtual bool handleError(Error code, const char* message) = 0;
};

// ============================================================================
// [asmjit::ErrorUtil]
// ============================================================================

//! Error utilities.
struct ErrorUtil {
  //! Get printable version of AsmJit `kError` code.
  static ASMJIT_API const char* asString(Error code);
};

//! \}

// ============================================================================
// [ASMJIT_PROPAGATE_ERROR]
// ============================================================================

//! \internal
//!
//! Used by AsmJit to return the `_Exp_` result if it's an error.
#define ASMJIT_PROPAGATE_ERROR(_Exp_) \
  do { \
    ::asmjit::Error errval_ = (_Exp_); \
    if (errval_ != ::asmjit::kErrorOk) \
      return errval_; \
  } while (0)

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_ERROR_H
