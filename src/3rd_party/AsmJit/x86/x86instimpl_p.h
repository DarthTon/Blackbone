// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86INSTIMPL_P_H
#define _ASMJIT_X86_X86INSTIMPL_P_H

// [Dependencies]
#include "../x86/x86inst.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

//! \internal
//!
//! Contains X86/X64 specific implementation of APIs provided by `asmjit::Inst`.
//!
//! The purpose of `X86InstImpl` is to move most of the logic out of `X86Inst`.
struct X86InstImpl {
  #if !defined(ASMJIT_DISABLE_VALIDATION)
  static Error validate(uint32_t archType, const Inst::Detail& detail, const Operand_* operands, uint32_t count) noexcept;
  #endif

  #if !defined(ASMJIT_DISABLE_EXTENSIONS)
  static Error checkFeatures(uint32_t archType, const Inst::Detail& detail, const Operand_* operands, uint32_t count, CpuFeatures& out) noexcept;
  #endif
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86INSTIMPL_P_H
