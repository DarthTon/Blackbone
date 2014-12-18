// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_LOGGER_H
#define _ASMJIT_BASE_LOGGER_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_LOGGER)

// [Dependencies - AsmJit]
#include "../base/string.h"

// [Dependencies - C]
#include <stdarg.h>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_util
//! \{

// ============================================================================
// [asmjit::kLoggerOption]
// ============================================================================

//! Logger options.
ASMJIT_ENUM(kLoggerOption) {
  //! Whether to output instructions also in binary form.
  kLoggerOptionBinaryForm = 0,

  //! Whether to output immediates as hexadecimal numbers.
  kLoggerOptionHexImmediate = 1,
  //! Whether to output displacements as hexadecimal numbers.
  kLoggerOptionHexDisplacement = 2,

  //! Count of logger options.
  kLoggerOptionCount = 3
};

// ============================================================================
// [asmjit::kLoggerStyle]
// ============================================================================

//! Logger style.
ASMJIT_ENUM(kLoggerStyle) {
  kLoggerStyleDefault = 0,
  kLoggerStyleDirective = 1,
  kLoggerStyleLabel = 2,
  kLoggerStyleData = 3,
  kLoggerStyleComment = 4,

  kLoggerStyleCount = 5
};

// ============================================================================
// [asmjit::Logger]
// ============================================================================

//! Abstract logging class.
//!
//! This class can be inherited and reimplemented to fit into your logging
//! subsystem. When reimplementing use `Logger::log()` method to log into
//! a custom stream.
//!
//! This class also contain `_enabled` member that can be used to enable
//! or disable logging.
struct ASMJIT_VCLASS Logger {
  ASMJIT_NO_COPY(Logger)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `Logger` instance.
  ASMJIT_API Logger();
  //! Destroy the `Logger` instance.
  ASMJIT_API virtual ~Logger();

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  //! Log output.
  virtual void logString(uint32_t style, const char* buf, size_t len = kInvalidIndex) = 0;

  //! Log formatter message (like sprintf) sending output to `logString()` method.
  ASMJIT_API void logFormat(uint32_t style, const char* fmt, ...);
  //! Log binary data.
  ASMJIT_API void logBinary(uint32_t style, const void* data, size_t size);

  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

  //! Get all logger options as a single integer.
  ASMJIT_INLINE uint32_t getOptions() const {
    return _options;
  }

  //! Get the given logger option.
  ASMJIT_INLINE bool getOption(uint32_t id) const {
    ASMJIT_ASSERT(id < kLoggerOptionCount);
    return static_cast<bool>((_options >> id) & 0x1);
  }

  //! Set the given logger option.
  ASMJIT_API void setOption(uint32_t id, bool value);

  // --------------------------------------------------------------------------
  // [Indentation]
  // --------------------------------------------------------------------------

  //! Get indentation.
  ASMJIT_INLINE const char* getIndentation() const {
    return _indentation;
  }

  //! Set indentation.
  ASMJIT_API void setIndentation(const char* indentation);

  //! Reset indentation.
  ASMJIT_INLINE void resetIndentation() {
    setIndentation(NULL);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Options, see `kLoggerOption`.
  uint32_t _options;

  //! Indentation.
  char _indentation[12];
};

// ============================================================================
// [asmjit::FileLogger]
// ============================================================================

//! Logger that can log to standard C `FILE*` stream.
struct ASMJIT_VCLASS FileLogger : public Logger {
  ASMJIT_NO_COPY(FileLogger)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `FileLogger` that logs to a `FILE` stream.
  ASMJIT_API FileLogger(FILE* stream = NULL);

  //! Destroy the `FileLogger`.
  ASMJIT_API virtual ~FileLogger();

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get `FILE*` stream.
  //!
  //! \note Return value can be `NULL`.
  ASMJIT_INLINE FILE* getStream() const {
    return _stream;
  }

  //! Set `FILE*` stream, can be set to `NULL` to disable logging, although
  //! the `CodeGen` will still call `logString` even if there is no stream.
  ASMJIT_INLINE void setStream(FILE* stream) {
    _stream = stream;
  }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void logString(uint32_t style, const char* buf, size_t len = kInvalidIndex);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! C file stream.
  FILE* _stream;
};

// ============================================================================
// [asmjit::StringLogger]
// ============================================================================

//! String logger.
struct ASMJIT_VCLASS StringLogger : public Logger {
  ASMJIT_NO_COPY(StringLogger)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create new `StringLogger`.
  ASMJIT_API StringLogger();

  //! Destroy the `StringLogger`.
  ASMJIT_API virtual ~StringLogger();

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get `char*` pointer which represents the resulting string.
  //!
  //! The pointer is owned by `StringLogger`, it can't be modified or freed.
  ASMJIT_INLINE const char* getString() const {
    return _stringBuilder.getData();
  }

  //! Clear the resulting string.
  ASMJIT_INLINE void clearString() {
    _stringBuilder.clear();
  }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void logString(uint32_t style, const char* buf, size_t len = kInvalidIndex);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Output.
  StringBuilder _stringBuilder;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_LOGGER
#endif // _ASMJIT_BASE_LOGGER_H
