// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_OPERAND_H
#define _ASMJIT_OPERAND_H

// [Dependencies]
#include "Build.h"

namespace AsmJit {

//! @addtogroup AsmJit_Core
//! @{

// There is currently no platform independent code.

//! @}

} // AsmJit namespace

// ============================================================================
// [Platform Specific]
// ============================================================================

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
#include "OperandX86X64.h"
#endif // ASMJIT_X86 || ASMJIT_X64

// [Guard]
#endif // _ASMJIT_OPERAND_H
