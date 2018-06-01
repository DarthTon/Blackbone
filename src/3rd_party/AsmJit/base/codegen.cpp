// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/codegen.h"
#include "../base/intutil.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CodeGen - Construction / Destruction]
// ============================================================================

CodeGen::CodeGen(Runtime* runtime) :
  _runtime(runtime),
  _logger(NULL),
  _errorHandler(NULL),
  _baseAddress(runtime->getBaseAddress()),
  _arch(kArchNone),
  _regSize(0),
  _reserved(0),
  _features(IntUtil::mask(kCodeGenOptimizedAlign)),
  _instOptions(0),
  _error(kErrorOk),
  _baseZone(16384 - kZoneOverhead) {}

CodeGen::~CodeGen() {
  if (_errorHandler != NULL)
    _errorHandler->release();
}

// ============================================================================
// [asmjit::CodeGen - Logging]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGER)
Error CodeGen::setLogger(Logger* logger) {
  _logger = logger;
  return kErrorOk;
}
#endif // !ASMJIT_DISABLE_LOGGER

// ============================================================================
// [asmjit::CodeGen - Error]
// ============================================================================

Error CodeGen::setError(Error error, const char* message) {
  if (error == kErrorOk)  {
    _error = kErrorOk;
    return kErrorOk;
  }

  if (message == NULL) {
#if !defined(ASMJIT_DISABLE_NAMES)
    message = ErrorUtil::asString(error);
#else
    static const char noMessage[] = "";
    message = noMessage;
#endif // ASMJIT_DISABLE_NAMES
  }

  // Error handler is called before logger so logging can be skipped if error
  // has been handled.
  ErrorHandler* handler = _errorHandler;
  ASMJIT_TLOG("[ERROR] %s %s\n", message, !handler ? "(Possibly unhandled?)" : "");

  if (handler != NULL && handler->handleError(error, message))
    return error;

#if !defined(ASMJIT_DISABLE_LOGGER)
  Logger* logger = _logger;
  if (logger != NULL) {
    logger->logFormat(kLoggerStyleComment,
      "*** ERROR: %s (%u).\n", message, static_cast<unsigned int>(error));
  }
#endif // !ASMJIT_DISABLE_LOGGER

  // The handler->handleError() function may throw an exception or longjmp()
  // to terminate the execution of setError(). This is the reason why we have
  // delayed changing the _error member until now.
  _error = error;

  return error;
}

Error CodeGen::setErrorHandler(ErrorHandler* handler) {
  ErrorHandler* oldHandler = _errorHandler;

  if (oldHandler != NULL)
    oldHandler->release();

  if (handler != NULL)
    handler = handler->addRef();

  _errorHandler = handler;
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
