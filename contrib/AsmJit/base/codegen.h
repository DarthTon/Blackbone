// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODEGEN_H
#define _ASMJIT_BASE_CODEGEN_H

// [Dependencies - AsmJit]
#include "../base/error.h"
#include "../base/logger.h"
#include "../base/runtime.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_general
//! \{

// ============================================================================
// [asmjit::kCodeGen]
// ============================================================================

//! Features of \ref CodeGen.
ASMJIT_ENUM(kCodeGen) {
  //! Emit optimized code-alignment sequences (true by default).
  //!
  //! X86/X64
  //! -------
  //!
  //! Default align sequence used by X86/X64 architecture is one-byte 0x90
  //! opcode that is mostly shown by disassemblers as nop. However there are
  //! more optimized align sequences for 2-11 bytes that may execute faster.
  //! If this feature is enabled asmjit will generate specialized sequences
  //! for alignment between 1 to 11 bytes. Also when `x86x64::Compiler` is
  //! used, it may add rex prefixes into the code to make some instructions
  //! greater so no alignment sequences are needed.
  kCodeGenOptimizedAlign = 0,

  //! Emit jump-prediction hints (false by default).
  //!
  //! X86/X64
  //! -------
  //!
  //! Jump prediction is usually based on the direction of the jump. If the
  //! jump is backward it is usually predicted as taken; and if the jump is
  //! forward it is usually predicted as not-taken. The reason is that loops
  //! generally use backward jumps and conditions usually use forward jumps.
  //! However this behavior can be overridden by using instruction prefixes.
  //! If this option is enabled these hints will be emitted.
  //!
  //! This feature is disabled by default, because the only processor that
  //! used to take into consideration prediction hints was P4 that is not used
  //! anymore.
  kCodeGenPredictedJumps = 1
};

// ============================================================================
// [asmjit::kAlignMode]
// ============================================================================

//! Code aligning mode.
ASMJIT_ENUM(kAlignMode) {
  kAlignCode = 0,
  kAlignData = 1
};

// ============================================================================
// [asmjit::kRelocMode]
// ============================================================================

//! Relocation mode.
ASMJIT_ENUM(kRelocMode) {
  kRelocAbsToAbs = 0,
  kRelocRelToAbs = 1,
  kRelocAbsToRel = 2,
  kRelocTrampoline = 3
};

// ============================================================================
// [asmjit::CodeGen]
// ============================================================================

//! Abstract class defining basics of \ref Assembler and \ref BaseCompiler.
struct CodeGen {
  ASMJIT_NO_COPY(CodeGen)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CodeGen` instance.
  ASMJIT_API CodeGen(Runtime* runtime);
  //! Destroy the `CodeGen` instance.
  ASMJIT_API virtual ~CodeGen();

  // --------------------------------------------------------------------------
  // [Runtime]
  // --------------------------------------------------------------------------

  //! Get runtime.
  ASMJIT_INLINE Runtime* getRuntime() const {
    return _runtime;
  }

  // --------------------------------------------------------------------------
  // [Logger]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_DISABLE_LOGGER)
  //! Get whether the code generator has a logger.
  ASMJIT_INLINE bool hasLogger() const {
    return _logger != NULL;
  }

  //! Get logger.
  ASMJIT_INLINE Logger* getLogger() const {
    return _logger;
  }

  //! Set logger to `logger`.
  ASMJIT_API Error setLogger(Logger* logger);
#endif // !ASMJIT_DISABLE_LOGGER

  // --------------------------------------------------------------------------
  // [Arch]
  // --------------------------------------------------------------------------

  //! Get target architecture.
  ASMJIT_INLINE uint32_t getArch() const {
    return _arch;
  }

  //! Get default register size (4 or 8 bytes).
  ASMJIT_INLINE uint32_t getRegSize() const {
    return _regSize;
  }

  // --------------------------------------------------------------------------
  // [Error]
  // --------------------------------------------------------------------------

  //! Get last error code.
  ASMJIT_INLINE Error getError() const {
    return _error;
  }

  //! Set last error code and propagate it through the error handler.
  ASMJIT_API Error setError(Error error, const char* message = NULL);

  //! Clear the last error code.
  ASMJIT_INLINE void clearError() {
    _error = kErrorOk;
  }

  //! Get error handler.
  ASMJIT_INLINE ErrorHandler* getErrorHandler() const {
    return _errorHandler;
  }

  //! Set error handler.
  ASMJIT_API Error setErrorHandler(ErrorHandler* handler);

  //! Clear error handler.
  ASMJIT_INLINE Error clearErrorHandler() {
    return setErrorHandler(NULL);
  }

  // --------------------------------------------------------------------------
  // [Features]
  // --------------------------------------------------------------------------

  //! Get code-generator `feature`.
  ASMJIT_API bool hasFeature(uint32_t feature) const;
  //! Set code-generator `feature` to `value`.
  ASMJIT_API Error setFeature(uint32_t feature, bool value);

  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

  //! Get options of the next instruction.
  ASMJIT_INLINE uint32_t getOptions() const {
    return _options;
  }

  //! Set options of the next instruction.
  ASMJIT_INLINE void setOptions(uint32_t options) {
    _options = options;
  }

  //! Get options of the next instruction and clear them.
  ASMJIT_INLINE uint32_t getOptionsAndClear() {
    uint32_t options = _options;
    _options = 0;
    return options;
  };

  // --------------------------------------------------------------------------
  // [Purge]
  // --------------------------------------------------------------------------

  //! Called by \ref clear() and \ref reset() to clear all data used by the
  //! code generator.
  virtual void _purge() = 0;

  // --------------------------------------------------------------------------
  // [Make]
  // --------------------------------------------------------------------------

  //! Make is a convenience method to make and relocate the current code and
  //! add it to the associated `Runtime`.
  //!
  //! What is needed is only to cast the returned pointer to your function type
  //! and then use it. If there was an error during `make()` `NULL` is returned
  //! and the last error code can be obtained by calling `getError()`.
  virtual void* make() = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Runtime.
  Runtime* _runtime;

#if !defined(ASMJIT_DISABLE_LOGGER)
  //! Logger.
  Logger* _logger;
#else
  // Makes the libraries built with/without logging support binary compatible.
  void* _logger;
#endif // ASMJIT_DISABLE_LOGGER

  //! Error handler, called by \ref setError().
  ErrorHandler* _errorHandler;

  //! Target architecture.
  uint8_t _arch;
  //! Target general-purpose register size (4 or 8 bytes).
  uint8_t _regSize;
  //! Target features.
  uint8_t _features;
  //! Last error code.
  uint8_t _error;

  //! Options for the next generated instruction (only 8-bits used).
  uint32_t _options;

  //! Base zone.
  Zone _baseZone;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_CODEGEN_H
