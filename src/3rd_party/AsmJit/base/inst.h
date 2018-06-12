// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_INST_H
#define _ASMJIT_BASE_INST_H

// [Dependencies]
#include "../base/cpuinfo.h"
#include "../base/operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::Inst]
// ============================================================================

//! Definitions and utilities related to instructions used by all architectures.
struct Inst {
  ASMJIT_ENUM(Id) {
    kIdNone = 0                          //!< Invalid or uninitialized instruction id.
  };

  //! Describes an instruction's jump type, if any.
  ASMJIT_ENUM(JumpType) {
    kJumpTypeNone        = 0,            //!< Instruction doesn't jump (regular instruction).
    kJumpTypeDirect      = 1,            //!< Instruction is a unconditional (direct) jump.
    kJumpTypeConditional = 2,            //!< Instruction is a conditional jump.
    kJumpTypeCall        = 3,            //!< Instruction is a function call.
    kJumpTypeReturn      = 4             //!< Instruction is a function return.
  };

  // --------------------------------------------------------------------------
  // [Detail]
  // --------------------------------------------------------------------------

  //! Instruction id, options, and extraReg packed in a single structure. This
  //! structure exists to simplify analysis and validation API that requires a
  //! lot of information about the instruction to be processed.
  class Detail {
  public:
    ASMJIT_INLINE Detail() noexcept
      : instId(0),
        options(0),
        extraReg() {}

    explicit ASMJIT_INLINE Detail(uint32_t instId, uint32_t options = 0) noexcept
      : instId(instId),
        options(options),
        extraReg() {}

    ASMJIT_INLINE Detail(uint32_t instId, uint32_t options, const RegOnly& reg) noexcept
      : instId(instId),
        options(options),
        extraReg(reg) {}

    ASMJIT_INLINE Detail(uint32_t instId, uint32_t options, const Reg& reg) noexcept
      : instId(instId),
        options(options) { extraReg.init(reg); }

    // ------------------------------------------------------------------------
    // [Accessors]
    // ------------------------------------------------------------------------

    ASMJIT_INLINE bool hasExtraReg() const noexcept { return extraReg.isValid(); }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    uint32_t instId;
    uint32_t options;
    RegOnly extraReg;
  };

  // --------------------------------------------------------------------------
  // [API]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_DISABLE_VALIDATION)
  //! Validate the given instruction.
  ASMJIT_API static Error validate(uint32_t archType, const Detail& detail, const Operand_* operands, uint32_t count) noexcept;
#endif // !ASMJIT_DISABLE_VALIDATION

#if !defined(ASMJIT_DISABLE_EXTENSIONS)
  //! Check CPU features required to execute the given instruction.
  ASMJIT_API static Error checkFeatures(uint32_t archType, const Detail& detail, const Operand_* operands, uint32_t count, CpuFeatures& out) noexcept;
#endif // !defined(ASMJIT_DISABLE_EXTENSIONS)
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_INST_H
