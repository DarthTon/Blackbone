// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86LOGGING_P_H
#define _ASMJIT_X86_X86LOGGING_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_LOGGING)

// [Dependencies]
#include "../base/logging.h"
#include "../x86/x86globals.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::X86Logging]
// ============================================================================

struct X86Logging {
  static Error formatRegister(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t archType,
    uint32_t regType,
    uint32_t regId) noexcept;

  static Error formatOperand(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t archType,
    const Operand_& op) noexcept;

  static Error formatInstruction(
    StringBuilder& sb,
    uint32_t logOptions,
    const CodeEmitter* emitter,
    uint32_t archType,
    const Inst::Detail& detail, const Operand_* opArray, uint32_t opCount) noexcept;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_LOGGING
#endif // _ASMJIT_X86_X86LOGGING_P_H
