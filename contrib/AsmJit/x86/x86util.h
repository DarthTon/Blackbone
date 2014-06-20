// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86UTIL_H
#define _ASMJIT_X86_X86UTIL_H

// [Dependencies - AsmJit]
#include "../base/func.h"
#include "../x86/x86inst.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

//! \addtogroup asmjit_x86x64_util
//! \{

// ============================================================================
// [asmjit::x86x64::Internal]
// ============================================================================

//! \internal
//!
//! X86/X64 condition codes to reversed condition codes map.
ASMJIT_VAR const uint32_t _reverseCond[20];

//! \internal
//!
//! X86X64 condition codes to "cmovcc" group map.
ASMJIT_VAR const uint32_t _condToCmovcc[20];

//! \internal
//!
//! X86X64 condition codes to "jcc" group map.
ASMJIT_VAR const uint32_t _condToJcc[20];

//! \internal
//!
//! X86X64 condition codes to "setcc" group map.
ASMJIT_VAR const uint32_t _condToSetcc[20];

// ============================================================================
// [asmjit::x86x64::RegCount]
// ============================================================================

//! \internal
//!
//! X86/X64 registers count (Gp, Fp, Mm, Xmm).
struct RegCount {
  // --------------------------------------------------------------------------
  // [Zero]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() {
    _packed = 0;
  }

  // --------------------------------------------------------------------------
  // [Get]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t get(uint32_t c) const {
    ASMJIT_ASSERT(c < kRegClassCount);
    return _regs[c];
  }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void set(uint32_t c, uint32_t n) {
    ASMJIT_ASSERT(c < kRegClassCount);
    ASMJIT_ASSERT(n < 0x100);

    _regs[c] = static_cast<uint8_t>(n);
  }

  // --------------------------------------------------------------------------
  // [Add]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void add(uint32_t c, uint32_t n = 1) {
    ASMJIT_ASSERT(c < kRegClassCount);
    ASMJIT_ASSERT(n < 0x100);

    _regs[c] += static_cast<uint8_t>(n);
  }

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void makeIndex(const RegCount& count) {
    uint8_t a = count._regs[0];
    uint8_t b = count._regs[1];
    uint8_t c = count._regs[2];

    _regs[0] = 0;
    _regs[1] = a;
    _regs[2] = a + b;
    _regs[3] = a + b + c;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint8_t _gp;
      uint8_t _fp;
      uint8_t _mm;
      uint8_t _xy;
    };

    uint8_t _regs[4];
    uint32_t _packed;
  };
};

// ============================================================================
// [asmjit::x86x64::RegMask]
// ============================================================================

//! \internal
//!
//! X86/X64 registers mask (Gp, Fp, Mm, Xmm/Ymm/Zmm).
struct RegMask {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() {
    _packed.reset();
  }

  // --------------------------------------------------------------------------
  // [IsEmpty / Has]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isEmpty() const {
    return _packed.isZero();
  }

  ASMJIT_INLINE bool has(uint32_t c, uint32_t mask = 0xFFFFFFFF) const {
    switch (c) {
      case kRegClassGp : return (static_cast<uint32_t>(_gp ) & mask) != 0;
      case kRegClassFp : return (static_cast<uint32_t>(_fp ) & mask) != 0;
      case kRegClassMm : return (static_cast<uint32_t>(_mm ) & mask) != 0;
      case kRegClassXyz: return (static_cast<uint32_t>(_xyz) & mask) != 0;
    }

    ASMJIT_ASSERT(!"Reached");
    return false;
  }

  // --------------------------------------------------------------------------
  // [Zero]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void zero(uint32_t c) {
    switch (c) {
      case kRegClassGp : _gp  = 0; break;
      case kRegClassFp : _fp  = 0; break;
      case kRegClassMm : _mm  = 0; break;
      case kRegClassXyz: _xyz = 0; break;
    }
  }

  // --------------------------------------------------------------------------
  // [Get]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t get(uint32_t c) const {
    switch (c) {
      case kRegClassGp : return _gp;
      case kRegClassFp : return _fp;
      case kRegClassMm : return _mm;
      case kRegClassXyz: return _xyz;
    }

    ASMJIT_ASSERT(!"Reached");
    return 0;
  }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void set(uint32_t c, uint32_t mask) {
    switch (c) {
      case kRegClassGp : _gp  = static_cast<uint16_t>(mask); break;
      case kRegClassFp : _fp  = static_cast<uint8_t >(mask); break;
      case kRegClassMm : _mm  = static_cast<uint8_t >(mask); break;
      case kRegClassXyz: _xyz = static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void set(const RegMask& other) {
    _packed.setUInt64(other._packed);
  }

  // --------------------------------------------------------------------------
  // [Add]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void add(uint32_t c, uint32_t mask) {
    switch (c) {
      case kRegClassGp : _gp  |= static_cast<uint16_t>(mask); break;
      case kRegClassFp : _fp  |= static_cast<uint8_t >(mask); break;
      case kRegClassMm : _mm  |= static_cast<uint8_t >(mask); break;
      case kRegClassXyz: _xyz |= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void add(const RegMask& other) {
    _packed.or_(other._packed);
  }

  // --------------------------------------------------------------------------
  // [Del]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void del(uint32_t c, uint32_t mask) {
    switch (c) {
      case kRegClassGp : _gp  &= ~static_cast<uint16_t>(mask); break;
      case kRegClassFp : _fp  &= ~static_cast<uint8_t >(mask); break;
      case kRegClassMm : _mm  &= ~static_cast<uint8_t >(mask); break;
      case kRegClassXyz: _xyz &= ~static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void del(const RegMask& other) {
    _packed.del(other._packed);
  }

  // --------------------------------------------------------------------------
  // [And]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void and_(uint32_t c, uint32_t mask) {
    switch (c) {
      case kRegClassGp : _gp  &= static_cast<uint16_t>(mask); break;
      case kRegClassFp : _fp  &= static_cast<uint8_t >(mask); break;
      case kRegClassMm : _mm  &= static_cast<uint8_t >(mask); break;
      case kRegClassXyz: _xyz &= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void and_(const RegMask& other) {
    _packed.and_(other._packed);
  }

  // --------------------------------------------------------------------------
  // [Xor]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void xor_(uint32_t c, uint32_t mask) {
    switch (c) {
      case kRegClassGp : _gp  ^= static_cast<uint16_t>(mask); break;
      case kRegClassFp : _fp  ^= static_cast<uint8_t >(mask); break;
      case kRegClassMm : _mm  ^= static_cast<uint8_t >(mask); break;
      case kRegClassXyz: _xyz ^= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void xor_(const RegMask& other) {
    _packed.xor_(other._packed);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! Gp mask (16-bit).
      uint16_t _gp;
      //! Fp mask (8-bit).
      uint8_t _fp;
      //! Mm mask (8-bit).
      uint8_t _mm;
      //! Xmm/Ymm/Zmm mask (32-bit).
      uint32_t _xyz;
    };

    //! All masks as 64-bit integer.
    UInt64 _packed;
  };
};

// ============================================================================
// [asmjit::x86x64::X86Util]
// ============================================================================

//! X86/X64 utilities.
struct X86Util {
  // --------------------------------------------------------------------------
  // [Condition Codes]
  // --------------------------------------------------------------------------

  //! Corresponds to trazsposing the operands of a comparison.
  static ASMJIT_INLINE uint32_t reverseCond(uint32_t cond) {
    ASMJIT_ASSERT(cond < ASMJIT_ARRAY_SIZE(_reverseCond));
    return _reverseCond[cond];
  }

  //! Get the equivalent of negated condition code.
  static ASMJIT_INLINE uint32_t negateCond(uint32_t cond) {
    ASMJIT_ASSERT(cond < ASMJIT_ARRAY_SIZE(_reverseCond));
    return static_cast<kCond>(cond ^ static_cast<uint32_t>(cond < kCondNone));
  }

  //! Translate condition code `cc` to `cmovcc` instruction code.
  //! \sa \ref kInstCode, \ref _kInstCmovcc.
  static ASMJIT_INLINE uint32_t condToCmovcc(uint32_t cond) {
    ASMJIT_ASSERT(static_cast<uint32_t>(cond) < ASMJIT_ARRAY_SIZE(_condToCmovcc));
    return _condToCmovcc[cond];
  }

  //! Translate condition code `cc` to `jcc` instruction code.
  //! \sa \ref kInstCode, \ref _kInstJcc.
  static ASMJIT_INLINE uint32_t condToJcc(uint32_t cond) {
    ASMJIT_ASSERT(static_cast<uint32_t>(cond) < ASMJIT_ARRAY_SIZE(_condToJcc));
    return _condToJcc[cond];
  }

  //! Translate condition code `cc` to `setcc` instruction code.
  //! \sa \ref kInstCode, \ref _kInstSetcc.
  static ASMJIT_INLINE uint32_t condToSetcc(uint32_t cond) {
    ASMJIT_ASSERT(static_cast<uint32_t>(cond) < ASMJIT_ARRAY_SIZE(_condToSetcc));
    return _condToSetcc[cond];
  }

  // --------------------------------------------------------------------------
  // [Registers]
  // --------------------------------------------------------------------------

  //! Get whether the `op` operand is Gpb-Lo or Gpb-Hi register.
  static ASMJIT_INLINE bool isGpbRegOp(const Operand& op) {
    const uint32_t mask = IntUtil::pack32_2x8_1x16(0xFF, 0xFF, ~(kRegTypePatchedGpbHi << 8) & 0xFF00);
    return (op._packed[0].u32[0] & mask) == IntUtil::pack32_2x8_1x16(kOperandTypeReg, 1, 0x0000);
  }

  // --------------------------------------------------------------------------
  // [Immediates]
  // --------------------------------------------------------------------------

  //! Pack a shuffle constant to be used with multimedia instrutions (2 values).
  //!
  //! \param x First component position, number at interval [0, 1] inclusive.
  //! \param y Second component position, number at interval [0, 1] inclusive.
  //!
  //! Shuffle constants can be used to make immediate value for these intrinsics:
  //! - `X86X64Assembler::shufpd()` and `X86X64Compiler::shufpd()`
  static ASMJIT_INLINE int mmShuffle(uint32_t x, uint32_t y) {
    return static_cast<int>((x << 1) | y);
  }

  //! Pack a shuffle constant to be used with multimedia instrutions (4 values).
  //!
  //! \param z First component position, number at interval [0, 3] inclusive.
  //! \param x Second component position, number at interval [0, 3] inclusive.
  //! \param y Third component position, number at interval [0, 3] inclusive.
  //! \param w Fourth component position, number at interval [0, 3] inclusive.
  //!
  //! Shuffle constants can be used to make immediate value for these intrinsics:
  //! - `X86X64Assembler::pshufw()` and `X86X64Compiler::pshufw()`
  //! - `X86X64Assembler::pshufd()` and `X86X64Compiler::pshufd()`
  //! - `X86X64Assembler::pshufhw()` and `X86X64Compiler::pshufhw()`
  //! - `X86X64Assembler::pshuflw()` and `X86X64Compiler::pshuflw()`
  //! - `X86X64Assembler::shufps()` and `X86X64Compiler::shufps()`
  static ASMJIT_INLINE int mmShuffle(uint32_t z, uint32_t y, uint32_t x, uint32_t w) {
    return static_cast<int>((z << 6) | (y << 4) | (x << 2) | w);
  }
};

//! \}

} // x86x64 namespace
} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86UTIL_H
