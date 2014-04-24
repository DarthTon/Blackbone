// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies]
#include "Defs.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

const char* getErrorString(uint32_t error) ASMJIT_NOTHROW
{
  static const char* errorMessage[] = {
    "No error",

    "No heap memory",
    "No virtual memory",

    "Unknown instruction",
    "Illegal instruction",
    "Illegal addressing",
    "Illegal short jump",

    "No function defined",
    "Incomplete function",

    "Not enough registers",
    "Registers overlap",

    "Incompatible argument",
    "Incompatible return value",

    "Unknown error"
  };

  // Saturate error code to be able to use errorMessage[].
  if (error > _ERROR_COUNT) error = _ERROR_COUNT;

  return errorMessage[error];
}

} // AsmJit

// [Api-End]
#include "ApiEnd.h"
