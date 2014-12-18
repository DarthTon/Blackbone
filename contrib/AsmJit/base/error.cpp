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

#if !defined(ASMJIT_DISABLE_NAMES)
static const char errorMessages[] = {
  "Ok\0"
  "No heap memory\0"
  "No virtual memory\0"
  "Invalid argument\0"
  "Invalid state\0"
  "No code generated\0"
  "Code too large\0"
  "Label already bound\0"
  "Unknown instruction\0"
  "Illegal instruction\0"
  "Illegal addressing\0"
  "Illegal displacement\0"
  "Overlapped arguments\0"
  "Unknown error\0"
};

static const char* findPackedString(const char* p, uint32_t id, uint32_t maxId) {
  uint32_t i = 0;

  if (id > maxId)
    id = maxId;

  while (i < id) {
    while (p[0])
      p++;

    p++;
    i++;
  }

  return p;
}

const char* ErrorUtil::asString(Error e) {
  return findPackedString(errorMessages, e, kErrorCount);
}
#endif // ASMJIT_DISABLE_NAMES

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
