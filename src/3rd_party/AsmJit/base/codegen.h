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
  //! Emit optimized code-alignment sequences (`Assembler` and `Compiler`).
  //!
  //! Default `true`.
  //!
  //! X86/X64
  //! -------
  //!
  //! Default align sequence used by X86/X64 architecture is one-byte 0x90
  //! opcode that is mostly shown by disassemblers as nop. However there are
  //! more optimized align sequences for 2-11 bytes that may execute faster.
  //! If this feature is enabled asmjit will generate specialized sequences
  //! for alignment between 1 to 11 bytes. Also when `X86Compiler` is used,
  //! it can add REX prefixes into the code to make some instructions greater
  //! so no alignment sequence is needed.
  kCodeGenOptimizedAlign = 0,

  //! Emit jump-prediction hints (`Assembler` and `Compiler`).
  //!
  //! Default `false`.
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
  //! used to take into consideration prediction hints was P4. Newer processors
  //! implement heuristics for branch prediction that ignores any static hints.
  kCodeGenPredictedJumps = 1,

  //! Schedule instructions so they can be executed faster (`Compiler` only).
  //!
  //! Default `false` - has to be explicitly enabled as the scheduler needs
  //! some time to run.
  //!
  //! X86/X64
  //! -------
  //!
  //! If scheduling is enabled AsmJit will try to reorder instructions to
  //! minimize dependency chain. Scheduler always runs after the registers are
  //! allocated so it doesn't change count of register allocs/spills.
  //!
  //! This feature is highly experimental and untested.
  kCodeGenEnableScheduler = 2
};

// ============================================================================
// [asmjit::kAlignMode]
// ============================================================================

//! Code aligning mode.
ASMJIT_ENUM(kAlignMode) {
  //! Align by emitting a sequence that can be executed (code).
  kAlignCode = 0,
  //! Align by emitting sequence that shouldn't be executed (data).
  kAlignData = 1
};

// ============================================================================
// [asmjit::kRelocMode]
// ============================================================================

//! Relocation mode.
ASMJIT_ENUM(kRelocMode) {
  //! Relocate an absolute address to an absolute address.
  kRelocAbsToAbs = 0,
  //! Relocate a relative address to an absolute address.
  kRelocRelToAbs = 1,
  //! Relocate an absolute address to a relative address.
  kRelocAbsToRel = 2,
  //! Relocate an absolute address to a relative address or use trampoline.
  kRelocTrampoline = 3
};

// ============================================================================
// [asmjit::CodeGen]
// ============================================================================

//! Abstract class defining basics of \ref Assembler and \ref Compiler.
struct ASMJIT_VCLASS CodeGen {
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
  // [BaseAddress]
  // --------------------------------------------------------------------------

  //! Get whether the code-generator has a base address.
  //!
  //! \sa \ref getBaseAddress()
  ASMJIT_INLINE bool hasBaseAddress() const {
    return _baseAddress != kNoBaseAddress;
  }

  //! Get the base address.
  ASMJIT_INLINE Ptr getBaseAddress() const {
    return _baseAddress;
  }

  //! Set the base address to `baseAddress`.
  ASMJIT_INLINE void setBaseAddress(Ptr baseAddress) {
    _baseAddress = baseAddress;
  }

  //! Reset the base address.
  ASMJIT_INLINE void resetBaseAddress() {
    setBaseAddress(kNoBaseAddress);
  }

  // --------------------------------------------------------------------------
  // [LastError / ErrorHandler]
  // --------------------------------------------------------------------------

  //! Get last error code.
  ASMJIT_INLINE Error getError() const {
    return _error;
  }

  //! Set last error code and propagate it through the error handler.
  ASMJIT_API Error setError(Error error, const char* message = NULL);

  //! Clear the last error code.
  ASMJIT_INLINE void resetError() {
    _error = kErrorOk;
  }

  //! Get error handler.
  ASMJIT_INLINE ErrorHandler* getErrorHandler() const {
    return _errorHandler;
  }

  //! Set error handler.
  ASMJIT_API Error setErrorHandler(ErrorHandler* handler);

  //! Clear error handler.
  ASMJIT_INLINE Error resetErrorHandler() {
    return setErrorHandler(NULL);
  }

  // --------------------------------------------------------------------------
  // [Code-Generation Features]
  // --------------------------------------------------------------------------

  //! Get code-generator `feature`.
  ASMJIT_INLINE bool hasFeature(uint32_t feature) const {
    ASMJIT_ASSERT(feature < 32);

    return (_features & (1 << feature)) != 0;
  }

  //! Set code-generator `feature` to `value`.
  ASMJIT_INLINE void setFeature(uint32_t feature, bool value) {
    ASMJIT_ASSERT(feature < 32);

    feature = static_cast<uint32_t>(value) << feature;
    _features = (_features & ~feature) | feature;
  }

  //! Get code-generator features.
  ASMJIT_INLINE uint32_t getFeatures() const {
    return _features;
  }

  //! Set code-generator features.
  ASMJIT_INLINE void setFeatures(uint32_t features) {
    _features = features;
  }

  // --------------------------------------------------------------------------
  // [Instruction Options]
  // --------------------------------------------------------------------------

  //! Get options of the next instruction.
  ASMJIT_INLINE uint32_t getInstOptions() const {
    return _instOptions;
  }

  //! Get options of the next instruction and reset them.
  ASMJIT_INLINE uint32_t getInstOptionsAndReset() {
    uint32_t instOptions = _instOptions;
    _instOptions = 0;
    return instOptions;
  };

  //! Set options of the next instruction.
  ASMJIT_INLINE void setInstOptions(uint32_t instOptions) {
    _instOptions = instOptions;
  }

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

  //! Target runtime.
  Runtime* _runtime;

#if !defined(ASMJIT_DISABLE_LOGGER)
  //! Logger.
  Logger* _logger;
#else
  //! \internal
  //!
  //! Makes libraries built with or without logging support binary compatible.
  void* _logger;
#endif // ASMJIT_DISABLE_LOGGER

  //! Error handler, called by \ref setError().
  ErrorHandler* _errorHandler;

  //! Base address (-1 if unknown/not used).
  Ptr _baseAddress;

  //! Target architecture ID.
  uint8_t _arch;
  //! Target architecture GP register size in bytes (4 or 8).
  uint8_t _regSize;
  //! \internal
  uint16_t _reserved;

  //! Code-Generation features, used by \ref hasFeature() and \ref setFeature().
  uint32_t _features;
  //! Options affecting the next instruction.
  uint32_t _instOptions;

  //! Last error code.
  uint32_t _error;

  //! Base zone.
  Zone _baseZone;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_CODEGEN_H
