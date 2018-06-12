// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86ASSEMBLER_H
#define _ASMJIT_X86_X86ASSEMBLER_H

// [Dependencies]
#include "../base/assembler.h"
#include "../base/utils.h"
#include "../x86/x86emitter.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86Assembler]
// ============================================================================

//! X86/X64 assembler.
//!
//! X86/X64 assembler emits machine-code into buffers managed by \ref CodeHolder.
class ASMJIT_VIRTAPI X86Assembler
  : public Assembler,
    public X86EmitterImplicitT<X86Assembler> {

public:
  typedef Assembler Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API X86Assembler(CodeHolder* code = nullptr) noexcept;
  ASMJIT_API virtual ~X86Assembler() noexcept;

  // --------------------------------------------------------------------------
  // [Compatibility]
  // --------------------------------------------------------------------------

  //! Explicit cast to `X86Emitter`.
  ASMJIT_INLINE X86Emitter* asEmitter() noexcept { return reinterpret_cast<X86Emitter*>(this); }
  //! Explicit cast to `X86Emitter` (const).
  ASMJIT_INLINE const X86Emitter* asEmitter() const noexcept { return reinterpret_cast<const X86Emitter*>(this); }

  //! Implicit cast to `X86Emitter`.
  ASMJIT_INLINE operator X86Emitter&() noexcept { return *asEmitter(); }
  //! Implicit cast to `X86Emitter` (const).
  ASMJIT_INLINE operator const X86Emitter&() const noexcept { return *asEmitter(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  // NOTE: X86Assembler uses _privateData to store 'address-override' bit that
  // is used to decide whether to emit address-override (67H) prefix based on
  // the memory BASE+INDEX registers. It's either `kX86MemInfo_67H_X86` or
  // `kX86MemInfo_67H_X64`.
  ASMJIT_INLINE uint32_t _getAddressOverrideMask() const noexcept { return _privateData; }
  ASMJIT_INLINE void _setAddressOverrideMask(uint32_t m) noexcept { _privateData = m; }

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API Error onDetach(CodeHolder* code) noexcept override;

  // --------------------------------------------------------------------------
  // [Code-Generation]
  // --------------------------------------------------------------------------

  using CodeEmitter::_emit;

  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) override;
  ASMJIT_API Error align(uint32_t mode, uint32_t alignment) override;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86ASSEMBLER_H
