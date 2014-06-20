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
  _arch(kArchNone),
  _regSize(0),
  _features(static_cast<uint8_t>(IntUtil::mask(kCodeGenOptimizedAlign))),
  _error(kErrorOk),
  _options(0),
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

  if (message == NULL)
    message = ErrorUtil::asString(error);

  // Error handler is called before logger so logging can be skipped if error
  // has been handled.
  ErrorHandler* handler = _errorHandler;
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

// ============================================================================
// [asmjit::CodeGen - Features]
// ============================================================================

bool CodeGen::hasFeature(uint32_t feature) const {
  if (feature >= sizeof(_features) * 8)
    return false;

  feature = 1 << feature;
  return (_features & feature) != 0;
}

Error CodeGen::setFeature(uint32_t feature, bool value) {
  if (feature >= sizeof(_features) * 8)
    return setError(kErrorInvalidArgument);

  feature = static_cast<uint32_t>(value) << feature;
  _features = static_cast<uint8_t>((static_cast<uint32_t>(_features) & ~feature) | feature);

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
