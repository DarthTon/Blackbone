// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86INTERNAL_P_H
#define _ASMJIT_X86_X86INTERNAL_P_H

#include "../asmjit_build.h"

// [Dependencies]
#include "../base/func.h"
#include "../x86/x86emitter.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::X86Internal]
// ============================================================================

//! \internal
//!
//! X86 utilities used at multiple places, not part of public API, not exported.
struct X86Internal {
  //! Initialize `CallConv` to X86/X64 specific calling convention.
  static Error initCallConv(CallConv& cc, uint32_t ccId) noexcept;

  //! Initialize `FuncDetail` to X86/X64 specific function signature.
  static Error initFuncDetail(FuncDetail& func, const FuncSignature& sign, uint32_t gpSize) noexcept;

  //! Initialize `FuncFrameLayout` from X86/X64 specific function detail and frame information.
  static Error initFrameLayout(FuncFrameLayout& layout, const FuncDetail& func, const FuncFrameInfo& ffi) noexcept;

  static Error argsToFrameInfo(const FuncArgsMapper& args, FuncFrameInfo& ffi) noexcept;

  //! Emit function prolog.
  static Error emitProlog(X86Emitter* emitter, const FuncFrameLayout& layout);

  //! Emit function epilog.
  static Error emitEpilog(X86Emitter* emitter, const FuncFrameLayout& layout);

  //! Emit a pure move operation between two registers or the same type or
  //! between a register and its home slot. This function does not handle
  //! register conversion.
  static Error emitRegMove(X86Emitter* emitter,
    const Operand_& dst_,
    const Operand_& src_, uint32_t typeId, bool avxEnabled, const char* comment = nullptr);

  //! Emit move from a function argument (either register or stack) to a register.
  //!
  //! This function can handle the necessary conversion from one argument to
  //! another, and from one register type to another, if it's possible. Any
  //! attempt of conversion that requires third register of a different kind
  //! (for example conversion from K to MMX) will fail.
  static Error emitArgMove(X86Emitter* emitter,
    const X86Reg& dst_, uint32_t dstTypeId,
    const Operand_& src_, uint32_t srcTypeId, bool avxEnabled, const char* comment = nullptr);

  static Error allocArgs(X86Emitter* emitter, const FuncFrameLayout& layout, const FuncArgsMapper& args);
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86INTERNAL_P_H
