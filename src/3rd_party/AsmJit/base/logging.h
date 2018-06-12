// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_LOGGING_H
#define _ASMJIT_BASE_LOGGING_H

// [Dependencies]
#include "../base/inst.h"
#include "../base/string.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

#if !defined(ASMJIT_DISABLE_LOGGING)

// ============================================================================
// [Forward Declarations]
// ============================================================================

class CodeEmitter;
class Reg;
struct Operand_;

#if !defined(ASMJIT_DISABLE_BUILDER)
class CodeBuilder;
class CBNode;
#endif // !ASMJIT_DISABLE_BUILDER

// ============================================================================
// [asmjit::Logger]
// ============================================================================

//! Abstract logging interface and helpers.
//!
//! This class can be inherited and reimplemented to fit into your logging
//! subsystem. When reimplementing use `Logger::_log()` method to log into
//! a custom stream.
//!
//! There are two \ref Logger implementations offered by AsmJit:
//!   - \ref FileLogger - allows to log into a `FILE*` stream.
//!   - \ref StringLogger - logs into a \ref StringBuilder.
class ASMJIT_VIRTAPI Logger {
public:
  ASMJIT_NONCOPYABLE(Logger)

  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

  //! Logger options.
  ASMJIT_ENUM(Options) {
    kOptionBinaryForm      = 0x00000001, //! Output instructions also in binary form.
    kOptionImmExtended     = 0x00000002, //! Output a meaning of some immediates.
    kOptionHexImmediate    = 0x00000004, //! Output constants in hexadecimal form.
    kOptionHexDisplacement = 0x00000008  //! Output displacements in hexadecimal form.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `Logger` instance.
  ASMJIT_API Logger() noexcept;
  //! Destroy the `Logger` instance.
  ASMJIT_API virtual ~Logger() noexcept;

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  //! Log `str` - must be reimplemented.
  virtual Error _log(const char* str, size_t len) noexcept = 0;

  //! Log a string `str`, which is either null terminated or having `len` length.
  ASMJIT_INLINE Error log(const char* str, size_t len = Globals::kInvalidIndex) noexcept { return _log(str, len); }
  //! Log a content of a `StringBuilder` `str`.
  ASMJIT_INLINE Error log(const StringBuilder& str) noexcept { return _log(str.getData(), str.getLength()); }

  //! Format the message by using `sprintf()` and then send to `log()`.
  ASMJIT_API Error logf(const char* fmt, ...) noexcept;
  //! Format the message by using `vsprintf()` and then send to `log()`.
  ASMJIT_API Error logv(const char* fmt, va_list ap) noexcept;
  //! Log binary data.
  ASMJIT_API Error logBinary(const void* data, size_t size) noexcept;

  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

  //! Get all logger options as a single integer.
  ASMJIT_INLINE uint32_t getOptions() const noexcept { return _options; }
  //! Get the given logger option.
  ASMJIT_INLINE bool hasOption(uint32_t option) const noexcept { return (_options & option) != 0; }
  ASMJIT_INLINE void addOptions(uint32_t options) noexcept { _options |= options; }
  ASMJIT_INLINE void clearOptions(uint32_t options) noexcept { _options &= ~options; }

  // --------------------------------------------------------------------------
  // [Indentation]
  // --------------------------------------------------------------------------

  //! Get indentation.
  ASMJIT_INLINE const char* getIndentation() const noexcept { return _indentation; }
  //! Set indentation.
  ASMJIT_API void setIndentation(const char* indentation) noexcept;
  //! Reset indentation.
  ASMJIT_INLINE void resetIndentation() noexcept { setIndentation(nullptr); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Options, see \ref LoggerOption.
  uint32_t _options;

  //! Indentation.
  char _indentation[12];
};

// ============================================================================
// [asmjit::FileLogger]
// ============================================================================

//! Logger that can log to a `FILE*` stream.
class ASMJIT_VIRTAPI FileLogger : public Logger {
public:
  ASMJIT_NONCOPYABLE(FileLogger)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `FileLogger` that logs to a `FILE` stream.
  ASMJIT_API FileLogger(FILE* stream = nullptr) noexcept;
  //! Destroy the `FileLogger`.
  ASMJIT_API virtual ~FileLogger() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the logging out put stream or null.
  ASMJIT_INLINE FILE* getStream() const noexcept { return _stream; }

  //! Set the logging output stream to `stream` or null.
  //!
  //! NOTE: If the `stream` is null it will disable logging, but it won't
  //! stop calling `log()` unless the logger is detached from the
  //! \ref Assembler.
  ASMJIT_INLINE void setStream(FILE* stream) noexcept { _stream = stream; }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _log(const char* buf, size_t len = Globals::kInvalidIndex) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! C file stream.
  FILE* _stream;
};

// ============================================================================
// [asmjit::StringLogger]
// ============================================================================

//! Logger that stores everything in an internal string buffer.
class ASMJIT_VIRTAPI StringLogger : public Logger {
public:
  ASMJIT_NONCOPYABLE(StringLogger)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create new `StringLogger`.
  ASMJIT_API StringLogger() noexcept;
  //! Destroy the `StringLogger`.
  ASMJIT_API virtual ~StringLogger() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get `char*` pointer which represents the resulting string.
  //!
  //! The pointer is owned by `StringLogger`, it can't be modified or freed.
  ASMJIT_INLINE const char* getString() const noexcept { return _stringBuilder.getData(); }
  //! Clear the resulting string.
  ASMJIT_INLINE void clearString() noexcept { _stringBuilder.clear(); }

  //! Get the length of the string returned by `getString()`.
  ASMJIT_INLINE size_t getLength() const noexcept { return _stringBuilder.getLength(); }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _log(const char* buf, size_t len = Globals::kInvalidIndex) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Output string.
  StringBuilder _stringBuilder;
};

// ============================================================================
// [asmjit::Logging]
// ============================================================================

struct Logging {
  ASMJIT_API static Error formatRegister(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t archType,
    uint32_t regType,
    uint32_t regId) noexcept;

  ASMJIT_API static Error formatLabel(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t labelId) noexcept;

  ASMJIT_API static Error formatOperand(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t archType,
    const Operand_& op) noexcept;

  ASMJIT_API static Error formatInstruction(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t archType,
    const Inst::Detail& detail, const Operand_* opArray, uint32_t opCount) noexcept;

#if !defined(ASMJIT_DISABLE_BUILDER)
  ASMJIT_API static Error formatNode(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeBuilder* cb,
    const CBNode* node_) noexcept;
#endif // !ASMJIT_DISABLE_BUILDER

// Only used by AsmJit internals, not available to users.
#if defined(ASMJIT_EXPORTS)
  enum {
    // Has to be big to be able to hold all metadata compiler can assign to a
    // single instruction.
    kMaxCommentLength = 512,
    kMaxInstLength = 40,
    kMaxBinaryLength = 26
  };

  static Error formatLine(
    StringBuilder& sb,
    const uint8_t* binData, size_t binLen, size_t dispLen, size_t imLen, const char* comment) noexcept;
#endif // ASMJIT_EXPORTS
};
#else
class Logger;
#endif // !ASMJIT_DISABLE_LOGGING

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_LOGGER_H
