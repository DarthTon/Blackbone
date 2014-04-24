// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_LOGGER_H
#define _ASMJIT_LOGGER_H

// [Dependencies]
#include "Defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_Logging
//! @{

//! @brief Abstract logging class.
//!
//! This class can be inherited and reimplemented to fit into your logging
//! subsystem. When reimplementing use @c AsmJit::Logger::log() method to
//! log into your stream.
//!
//! This class also contain @c _enabled member that can be used to enable
//! or disable logging.
struct ASMJIT_API Logger
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create logger.
  Logger() ASMJIT_NOTHROW;
  //! @brief Destroy logger.
  virtual ~Logger() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  //! @brief Abstract method to log output.
  //!
  //! Default implementation that is in @c AsmJit::Logger is to do nothing.
  //! It's virtual to fit to your logging system.
  virtual void logString(const char* buf, sysuint_t len = (sysuint_t)-1) ASMJIT_NOTHROW = 0;

  //! @brief Log formatter message (like sprintf) sending output to @c logString() method.
  virtual void logFormat(const char* fmt, ...) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Enabled]
  // --------------------------------------------------------------------------

  //! @brief Return @c true if logging is enabled.
  inline bool isEnabled() const ASMJIT_NOTHROW { return _enabled; }

  //! @brief Set logging to enabled or disabled.
  virtual void setEnabled(bool enabled) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Used]
  // --------------------------------------------------------------------------

  //! @brief Get whether the logger should be used.
  inline bool isUsed() const ASMJIT_NOTHROW { return _used; }

  // --------------------------------------------------------------------------
  // [LogBinary]
  // --------------------------------------------------------------------------

  //! @brief Get whether logging binary output.
  inline bool getLogBinary() const { return _logBinary; }
  //! @brief Get whether to log binary output.
  inline void setLogBinary(bool val) { _logBinary = val; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:

  //! @brief Whether logger is enabled or disabled.
  //!
  //! Default @c true.
  bool _enabled;

  //! @brief Whether logger is enabled and can be used.
  //!
  //! This value can be set by inherited classes to inform @c Logger that
  //! assigned stream (or something that can log output) is invalid. If
  //! @c _used is false it means that there is no logging output and AsmJit
  //! shouldn't use this logger (because all messages will be lost).
  //!
  //! This is designed only to optimize cases that logger exists, but its
  //! configured not to output messages. The API inside Logging and AsmJit
  //! should only check this value when needed. The API outside AsmJit should
  //! check only whether logging is @c _enabled.
  //!
  //! Default @c true.
  bool _used;

  //! @brief Whether to log instruction in binary form.
  bool _logBinary;

private:
  ASMJIT_DISABLE_COPY(Logger)
};

//! @brief Logger that can log to standard C @c FILE* stream.
struct ASMJIT_API FileLogger : public Logger
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new @c FileLogger.
  //! @param stream FILE stream where logging will be sent (can be @c NULL
  //! to disable logging).
  FileLogger(FILE* stream = NULL) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  virtual void logString(const char* buf, sysuint_t len = (sysuint_t)-1) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Enabled]
  // --------------------------------------------------------------------------

  virtual void setEnabled(bool enabled) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Stream]
  // --------------------------------------------------------------------------

  //! @brief Get @c FILE* stream.
  //!
  //! @note Return value can be @c NULL.
  inline FILE* getStream() const ASMJIT_NOTHROW { return _stream; }

  //! @brief Set @c FILE* stream.
  //!
  //! @param stream @c FILE stream where to log output (can be @c NULL to
  //! disable logging).
  void setStream(FILE* stream) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief C file stream.
  FILE* _stream;

  ASMJIT_DISABLE_COPY(FileLogger)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_LOGGER_H
