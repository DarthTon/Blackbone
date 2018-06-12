// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86MISC_H
#define _ASMJIT_X86_X86MISC_H

// [Dependencies]
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86RegCount]
// ============================================================================

//! \internal
//!
//! X86/X64 registers count.
//!
//! Since the number of registers changed across CPU generations `X86RegCount`
//! class is used by `X86Assembler` and `X86Compiler` to provide a way to get
//! number of available registers dynamically. 32-bit mode offers always only
//! 8 registers of all classes, however, 64-bit mode offers 16 GP registers and
//! 16 XMM/YMM/ZMM registers. AVX512 instruction set doubles the number of SIMD
//! registers (XMM/YMM/ZMM) to 32, this mode has to be explicitly enabled to
//! take effect as it changes some assumptions.
//!
//! `X86RegCount` is also used extensively by X86Compiler's register allocator
//! and data structures. FP registers were omitted as they are never mapped to
//! variables, thus, not needed to be managed.
//!
//! NOTE: At the moment `X86RegCount` can fit into 32-bits, having 8-bits for
//! each register kind except FP. This can change in the future after a new
//! instruction set, which adds more registers, is introduced.
struct X86RegCount {
  // --------------------------------------------------------------------------
  // [Zero]
  // --------------------------------------------------------------------------

  //! Reset all counters to zero.
  ASMJIT_INLINE void reset() noexcept { _packed = 0; }

  // --------------------------------------------------------------------------
  // [Get]
  // --------------------------------------------------------------------------

  //! Get register count by a register `kind`.
  ASMJIT_INLINE uint32_t get(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    uint32_t shift = Utils::byteShiftOfDWordStruct(kind);
    return (_packed >> shift) & static_cast<uint32_t>(0xFF);
  }

  //! Get Gp count.
  ASMJIT_INLINE uint32_t getGp() const noexcept { return get(X86Reg::kKindGp); }
  //! Get Mm count.
  ASMJIT_INLINE uint32_t getMm() const noexcept { return get(X86Reg::kKindMm); }
  //! Get K count.
  ASMJIT_INLINE uint32_t getK() const noexcept { return get(X86Reg::kKindK); }
  //! Get XMM/YMM/ZMM count.
  ASMJIT_INLINE uint32_t getVec() const noexcept { return get(X86Reg::kKindVec); }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  //! Set register count by a register `kind`.
  ASMJIT_INLINE void set(uint32_t kind, uint32_t n) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);
    ASMJIT_ASSERT(n <= 0xFF);

    uint32_t shift = Utils::byteShiftOfDWordStruct(kind);
    _packed = (_packed & ~static_cast<uint32_t>(0xFF << shift)) + (n << shift);
  }

  //! Set Gp count.
  ASMJIT_INLINE void setGp(uint32_t n) noexcept { set(X86Reg::kKindGp, n); }
  //! Set Mm count.
  ASMJIT_INLINE void setMm(uint32_t n) noexcept { set(X86Reg::kKindMm, n); }
  //! Set K count.
  ASMJIT_INLINE void setK(uint32_t n) noexcept { set(X86Reg::kKindK, n); }
  //! Set XMM/YMM/ZMM count.
  ASMJIT_INLINE void setVec(uint32_t n) noexcept { set(X86Reg::kKindVec, n); }

  // --------------------------------------------------------------------------
  // [Add]
  // --------------------------------------------------------------------------

  //! Add register count by a register `kind`.
  ASMJIT_INLINE void add(uint32_t kind, uint32_t n = 1) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);
    ASMJIT_ASSERT(0xFF - static_cast<uint32_t>(_regs[kind]) >= n);

    uint32_t shift = Utils::byteShiftOfDWordStruct(kind);
    _packed += n << shift;
  }

  //! Add GP count.
  ASMJIT_INLINE void addGp(uint32_t n) noexcept { add(X86Reg::kKindGp, n); }
  //! Add MMX count.
  ASMJIT_INLINE void addMm(uint32_t n) noexcept { add(X86Reg::kKindMm, n); }
  //! Add K count.
  ASMJIT_INLINE void addK(uint32_t n) noexcept { add(X86Reg::kKindK, n); }
  //! Add XMM/YMM/ZMM count.
  ASMJIT_INLINE void addVec(uint32_t n) noexcept { add(X86Reg::kKindVec, n); }

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  //! Build register indexes based on the given `count` of registers.
  ASMJIT_INLINE void indexFromRegCount(const X86RegCount& count) noexcept {
    uint32_t x = static_cast<uint32_t>(count._regs[0]);
    uint32_t y = static_cast<uint32_t>(count._regs[1]) + x;
    uint32_t z = static_cast<uint32_t>(count._regs[2]) + y;

    ASMJIT_ASSERT(y <= 0xFF);
    ASMJIT_ASSERT(z <= 0xFF);
    _packed = Utils::pack32_4x8(0, x, y, z);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! Count of GP registers.
      uint8_t _gp;
      //! Count of XMM|YMM|ZMM registers.
      uint8_t _vec;
      //! Count of MMX registers.
      uint8_t _mm;
      //! Count of K registers.
      uint8_t _k;
    };

    uint8_t _regs[4];
    uint32_t _packed;
  };
};

// ============================================================================
// [asmjit::X86RegMask]
// ============================================================================

//! \internal
//!
//! X86/X64 registers mask.
struct X86RegMask {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset all register masks to zero.
  ASMJIT_INLINE void reset() noexcept {
    _packed.reset();
  }

  // --------------------------------------------------------------------------
  // [IsEmpty / Has]
  // --------------------------------------------------------------------------

  //! Get whether all register masks are zero (empty).
  ASMJIT_INLINE bool isEmpty() const noexcept {
    return _packed.isZero();
  }

  ASMJIT_INLINE bool has(uint32_t kind, uint32_t mask = 0xFFFFFFFFU) const noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : return (static_cast<uint32_t>(_gp ) & mask) != 0;
      case X86Reg::kKindVec: return (static_cast<uint32_t>(_vec) & mask) != 0;
      case X86Reg::kKindMm : return (static_cast<uint32_t>(_mm ) & mask) != 0;
      case X86Reg::kKindK  : return (static_cast<uint32_t>(_k  ) & mask) != 0;
    }

    return false;
  }

  ASMJIT_INLINE bool hasGp(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kKindGp, mask); }
  ASMJIT_INLINE bool hasVec(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kKindVec, mask); }
  ASMJIT_INLINE bool hasMm(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kKindMm, mask); }
  ASMJIT_INLINE bool hasK(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kKindK, mask); }

  // --------------------------------------------------------------------------
  // [Get]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t get(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : return _gp;
      case X86Reg::kKindVec: return _vec;
      case X86Reg::kKindMm : return _mm;
      case X86Reg::kKindK  : return _k;
    }

    return 0;
  }

  ASMJIT_INLINE uint32_t getGp() const noexcept { return get(X86Reg::kKindGp); }
  ASMJIT_INLINE uint32_t getVec() const noexcept { return get(X86Reg::kKindVec); }
  ASMJIT_INLINE uint32_t getMm() const noexcept { return get(X86Reg::kKindMm); }
  ASMJIT_INLINE uint32_t getK() const noexcept { return get(X86Reg::kKindK); }

  // --------------------------------------------------------------------------
  // [Zero]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void zero(uint32_t kind) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : _gp  = 0; break;
      case X86Reg::kKindVec: _vec = 0; break;
      case X86Reg::kKindMm : _mm  = 0; break;
      case X86Reg::kKindK  : _k   = 0; break;
    }
  }

  ASMJIT_INLINE void zeroGp() noexcept { zero(X86Reg::kKindGp); }
  ASMJIT_INLINE void zeroVec() noexcept { zero(X86Reg::kKindVec); }
  ASMJIT_INLINE void zeroMm() noexcept { zero(X86Reg::kKindMm); }
  ASMJIT_INLINE void zeroK() noexcept { zero(X86Reg::kKindK); }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void set(const X86RegMask& other) noexcept {
    _packed = other._packed;
  }

  ASMJIT_INLINE void set(uint32_t kind, uint32_t mask) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : _gp  = static_cast<uint16_t>(mask); break;
      case X86Reg::kKindMm : _mm  = static_cast<uint8_t >(mask); break;
      case X86Reg::kKindK  : _k   = static_cast<uint8_t >(mask); break;
      case X86Reg::kKindVec: _vec = static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void setGp(uint32_t mask) noexcept { return set(X86Reg::kKindGp, mask); }
  ASMJIT_INLINE void setVec(uint32_t mask) noexcept { return set(X86Reg::kKindVec, mask); }
  ASMJIT_INLINE void setMm(uint32_t mask) noexcept { return set(X86Reg::kKindMm, mask); }
  ASMJIT_INLINE void setK(uint32_t mask) noexcept { return set(X86Reg::kKindK, mask); }

  // --------------------------------------------------------------------------
  // [And]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void and_(const X86RegMask& other) noexcept {
    _packed.and_(other._packed);
  }

  ASMJIT_INLINE void and_(uint32_t kind, uint32_t mask) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : _gp  &= static_cast<uint16_t>(mask); break;
      case X86Reg::kKindMm : _mm  &= static_cast<uint8_t >(mask); break;
      case X86Reg::kKindK  : _k   &= static_cast<uint8_t >(mask); break;
      case X86Reg::kKindVec: _vec &= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void andGp(uint32_t mask) noexcept { and_(X86Reg::kKindGp, mask); }
  ASMJIT_INLINE void andVec(uint32_t mask) noexcept { and_(X86Reg::kKindVec, mask); }
  ASMJIT_INLINE void andMm(uint32_t mask) noexcept { and_(X86Reg::kKindMm, mask); }
  ASMJIT_INLINE void andK(uint32_t mask) noexcept { and_(X86Reg::kKindK, mask); }

  // --------------------------------------------------------------------------
  // [AndNot]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void andNot(const X86RegMask& other) noexcept {
    _packed.andNot(other._packed);
  }

  ASMJIT_INLINE void andNot(uint32_t kind, uint32_t mask) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : _gp  &= ~static_cast<uint16_t>(mask); break;
      case X86Reg::kKindMm : _mm  &= ~static_cast<uint8_t >(mask); break;
      case X86Reg::kKindK  : _k   &= ~static_cast<uint8_t >(mask); break;
      case X86Reg::kKindVec: _vec &= ~static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void andNotGp(uint32_t mask) noexcept { andNot(X86Reg::kKindGp, mask); }
  ASMJIT_INLINE void andNotVec(uint32_t mask) noexcept { andNot(X86Reg::kKindVec, mask); }
  ASMJIT_INLINE void andNotMm(uint32_t mask) noexcept { andNot(X86Reg::kKindMm, mask); }
  ASMJIT_INLINE void andNotK(uint32_t mask) noexcept { andNot(X86Reg::kKindK, mask); }

  // --------------------------------------------------------------------------
  // [Or]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void or_(const X86RegMask& other) noexcept {
    _packed.or_(other._packed);
  }

  ASMJIT_INLINE void or_(uint32_t kind, uint32_t mask) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);
    switch (kind) {
      case X86Reg::kKindGp : _gp  |= static_cast<uint16_t>(mask); break;
      case X86Reg::kKindMm : _mm  |= static_cast<uint8_t >(mask); break;
      case X86Reg::kKindK  : _k   |= static_cast<uint8_t >(mask); break;
      case X86Reg::kKindVec: _vec |= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void orGp(uint32_t mask) noexcept { return or_(X86Reg::kKindGp, mask); }
  ASMJIT_INLINE void orVec(uint32_t mask) noexcept { return or_(X86Reg::kKindVec, mask); }
  ASMJIT_INLINE void orMm(uint32_t mask) noexcept { return or_(X86Reg::kKindMm, mask); }
  ASMJIT_INLINE void orK(uint32_t mask) noexcept { return or_(X86Reg::kKindK, mask); }

  // --------------------------------------------------------------------------
  // [Xor]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void xor_(const X86RegMask& other) noexcept {
    _packed.xor_(other._packed);
  }

  ASMJIT_INLINE void xor_(uint32_t kind, uint32_t mask) noexcept {
    ASMJIT_ASSERT(kind < Globals::kMaxVRegKinds);

    switch (kind) {
      case X86Reg::kKindGp : _gp  ^= static_cast<uint16_t>(mask); break;
      case X86Reg::kKindMm : _mm  ^= static_cast<uint8_t >(mask); break;
      case X86Reg::kKindK  : _k   ^= static_cast<uint8_t >(mask); break;
      case X86Reg::kKindVec: _vec ^= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void xorGp(uint32_t mask) noexcept { xor_(X86Reg::kKindGp, mask); }
  ASMJIT_INLINE void xorVec(uint32_t mask) noexcept { xor_(X86Reg::kKindVec, mask); }
  ASMJIT_INLINE void xorMm(uint32_t mask) noexcept { xor_(X86Reg::kKindMm, mask); }
  ASMJIT_INLINE void xorK(uint32_t mask) noexcept { xor_(X86Reg::kKindK, mask); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! GP registers mask (16 bits).
      uint16_t _gp;
      //! MMX registers mask (8 bits).
      uint8_t _mm;
      //! K registers mask (8 bits).
      uint8_t _k;
      //! XMM|YMM|ZMM registers mask (32 bits).
      uint32_t _vec;
    };

    //! Packed masks.
    UInt64 _packed;
  };
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86MISC_H
