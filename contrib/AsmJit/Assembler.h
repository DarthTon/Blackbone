// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_ASSEMBLER_H
#define _ASMJIT_ASSEMBLER_H

// [Dependencies]
#include "Build.h"

namespace AsmJit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Logger;
struct MemoryManager;
struct EInstruction;

} // AsmJit namespace

// ============================================================================
// [Platform Specific]
// ============================================================================

// [X86 / X64]
#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
#include "AssemblerX86X64.h"
#endif // ASMJIT_X86 || ASMJIT_X64

// [Guard]
#endif // _ASMJIT_ASSEMBLER_H
