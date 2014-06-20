// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/error.h"
#include "../base/intutil.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::ErrorHandler - Construction / Destruction]
// ============================================================================

ErrorHandler::ErrorHandler() {}
ErrorHandler::~ErrorHandler() {}

// ============================================================================
// [asmjit::ErrorHandler - Interface]
// ============================================================================

ErrorHandler* ErrorHandler::addRef() const {
  return const_cast<ErrorHandler*>(this);
}

void ErrorHandler::release() {}

// ============================================================================
// [asmjit::ErrorUtil - AsString]
// ============================================================================

static const char* errorMessages[] = {
  "Ok",

  "No heap memory",
  "No virtual memory",

  "Invalid argument",
  "Invalid state",

  "Unknown instruction",
  "Illegal instruction",
  "Illegal addressing",
  "Illegal displacement",

  "Invalid function",
  "Overlapped arguments",
  "Unknown error"
};

const char* ErrorUtil::asString(Error e) {
  return errorMessages[IntUtil::iMin<Error>(e, kErrorCount)];
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
