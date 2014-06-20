// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86OPERAND_H
#define _ASMJIT_X86_X86OPERAND_H

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../base/compiler.h"
#include "../base/globals.h"
#include "../base/intutil.h"
#include "../base/operand.h"
#include "../base/vectypes.h"
#include "../x86/x86regs.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct VarInfo;

struct X86Reg;
struct X86Var;

struct GpReg;
struct GpVar;

struct MmReg;
struct MmVar;

struct XmmReg;
struct XmmVar;

struct YmmReg;
struct YmmVar;

#define _OP_ID(_Op_) reinterpret_cast<const Operand&>(_Op_).getId()

// ============================================================================
// [asmjit::x86x64::Typedefs]
// ============================================================================

//! \addtogroup asmjit_x86x64_general
//! \{

typedef Vec64Data MmData;
typedef Vec128Data XmmData;
typedef Vec256Data YmmData;

// ============================================================================
// [asmjit::x86x64::Variables]
// ============================================================================

//! \internal
ASMJIT_VAR const VarInfo _varInfo[];

// ============================================================================
// [asmjit::x86x64::kMemVSib]
// ============================================================================

//! X86/X64 index register legacy and AVX2 (VSIB) support.
ASMJIT_ENUM(kMemVSib) {
  //! Memory operand uses Gp or no index register.
  kMemVSibGpz = 0,
  //! Memory operand uses Xmm or no index register.
  kMemVSibXmm = 1,
  //! Memory operand uses Ymm or no index register.
  kMemVSibYmm = 2
};

// ============================================================================
// [asmjit::x86x64::kMemFlags]
// ============================================================================

//! \internal
//!
//! X86/X64 specific memory flags.
ASMJIT_ENUM(kMemFlags) {
  kMemSegBits    = 0x7,
  kMemSegIndex   = 0,
  kMemSegMask    = kMemSegBits << kMemSegIndex,

  kMemGpdBits    = 0x1,
  kMemGpdIndex   = 3,
  kMemGpdMask    = kMemGpdBits << kMemGpdIndex,

  kMemVSibBits   = 0x3,
  kMemVSibIndex  = 4,
  kMemVSibMask   = kMemVSibBits << kMemVSibIndex,

  kMemShiftBits  = 0x3,
  kMemShiftIndex = 6,
  kMemShiftMask  = kMemShiftBits << kMemShiftIndex
};

// ============================================================================
// [asmjit::x86x64::kVarType]
// ============================================================================

//! X86/X64 variable type.
ASMJIT_ENUM(kVarType) {
  //! Variable is Mm (MMX).
  kVarTypeMm = 12,

  //! Variable is Xmm (SSE+).
  kVarTypeXmm,
  //! Variable is scalar Xmm SP-FP number.
  kVarTypeXmmSs,
  //! Variable is packed Xmm SP-FP number (4 floats).
  kVarTypeXmmPs,
  //! Variable is scalar Xmm DP-FP number.
  kVarTypeXmmSd,
  //! Variable is packed Xmm DP-FP number (2 doubles).
  kVarTypeXmmPd,

  //! Variable is Ymm (AVX+).
  kVarTypeYmm,
  //! Variable is packed Ymm SP-FP number (8 floats).
  kVarTypeYmmPs,
  //! Variable is packed Ymm DP-FP number (4 doubles).
  kVarTypeYmmPd,

  //! Count of variable types.
  kVarTypeCount,

  //! \internal
  //! \{
  _kVarTypeMmStart = kVarTypeMm,
  _kVarTypeMmEnd = kVarTypeMm,

  _kVarTypeXmmStart = kVarTypeXmm,
  _kVarTypeXmmEnd = kVarTypeXmmPd,

  _kVarTypeYmmStart = kVarTypeYmm,
  _kVarTypeYmmEnd = kVarTypeYmmPd
  //! \}
};

// ============================================================================
// [asmjit::x86x64::kVarDesc]
// ============================================================================

//! \internal
//!
//! X86/X64 variable description.
ASMJIT_ENUM(kVarDesc) {
  //! Variable contains single-precision floating-point(s).
  kVarDescSp = 0x10,
  //! Variable contains double-precision floating-point(s).
  kVarDescDp = 0x20,
  //! Variable is packed (for example float4x, double2x, ...).
  kVarDescPacked = 0x40
};

//! \}

// ============================================================================
// [asmjit::x86x64::VarInfo]
// ============================================================================

//! \addtogroup asmjit_x86x64_util
//! \{

//! \internal
//!
//! X86 variable information.
struct VarInfo {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get register type, see `kRegType`.
  ASMJIT_INLINE uint32_t getReg() const { return _reg; }
  //! Get register size in bytes.
  ASMJIT_INLINE uint32_t getSize() const { return _size; }
  //! Get variable class, see `kRegClass`.
  ASMJIT_INLINE uint32_t getClass() const { return _class; }
  //! Get variable description, see `kVarDesc`.
  ASMJIT_INLINE uint32_t getDesc() const { return _desc; }
  //! Get variable type name.
  ASMJIT_INLINE const char* getName() const { return _name; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Register type, see `kRegType`.
  uint8_t _reg;
  //! Register size in bytes.
  uint8_t _size;
  //! Register class, see `kRegClass`.
  uint8_t _class;
  //! Variable flags, see `kVarDesc`.
  uint8_t _desc;
  //! Variable type name.
  char _name[4];
};

// ============================================================================
// [asmjit::x86x64::X86Reg]
// ============================================================================

//! X86/X64 register.
struct X86Reg : public BaseReg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy X86 register.
  ASMJIT_INLINE X86Reg() : BaseReg() {}
  //! Create a custom X86 register.
  ASMJIT_INLINE X86Reg(uint32_t type, uint32_t index, uint32_t size) : BaseReg(type, index, size) {}
  //! Create a reference to `other` X86 register.
  ASMJIT_INLINE X86Reg(const X86Reg& other) : BaseReg(other) {}
  //! Create non-initialized X86 register.
  explicit ASMJIT_INLINE X86Reg(const _NoInit&) : BaseReg(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86Reg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(X86Reg)

  //! Get whether the register is Gp register.
  ASMJIT_INLINE bool isGp() const { return _vreg.type <= kRegTypeGpq; }
  //! Get whether the register is Gp byte (8-bit) register.
  ASMJIT_INLINE bool isGpb() const { return _vreg.type <= kRegTypeGpbHi; }
  //! Get whether the register is Gp lo-byte (8-bit) register.
  ASMJIT_INLINE bool isGpbLo() const { return _vreg.type == kRegTypeGpbLo; }
  //! Get whether the register is Gp hi-byte (8-bit) register.
  ASMJIT_INLINE bool isGpbHi() const { return _vreg.type == kRegTypeGpbHi; }
  //! Get whether the register is Gp word (16-bit) register.
  ASMJIT_INLINE bool isGpw() const { return _vreg.type == kRegTypeGpw; }
  //! Get whether the register is Gp dword (32-bit) register.
  ASMJIT_INLINE bool isGpd() const { return _vreg.type == kRegTypeGpd; }
  //! Get whether the register is Gp qword (64-bit) register.
  ASMJIT_INLINE bool isGpq() const { return _vreg.type == kRegTypeGpq; }

  //! Get whether the register is Fp register.
  ASMJIT_INLINE bool isFp() const { return _vreg.type == kRegTypeFp; }
  //! Get whether the register is Mm (64-bit) register.
  ASMJIT_INLINE bool isMm() const { return _vreg.type == kRegTypeMm; }
  //! Get whether the register is Xmm (128-bit) register.
  ASMJIT_INLINE bool isXmm() const { return _vreg.type == kRegTypeXmm; }
  //! Get whether the register is Ymm (256-bit) register.
  ASMJIT_INLINE bool isYmm() const { return _vreg.type == kRegTypeYmm; }

  //! Get whether the register is a segment.
  ASMJIT_INLINE bool isSeg() const { return _vreg.type == kRegTypeSeg; }
};

// ============================================================================
// [asmjit::x86x64::GpReg]
// ============================================================================

//! X86/X64 Gpb/Gpw/Gpd/Gpq register.
struct GpReg : public X86Reg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy Gp register.
  ASMJIT_INLINE GpReg() : X86Reg() {}
  //! Create a reference to `other` Gp register.
  ASMJIT_INLINE GpReg(const GpReg& other) : X86Reg(other) {}
  //! Create a custom Gp register.
  ASMJIT_INLINE GpReg(uint32_t type, uint32_t index, uint32_t size) : X86Reg(type, index, size) {}
  //! Create non-initialized Gp register.
  explicit ASMJIT_INLINE GpReg(const _NoInit&) : X86Reg(NoInit) {}

  // --------------------------------------------------------------------------
  // [GpReg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(GpReg)
};

// ============================================================================
// [asmjit::x86x64::FpReg]
// ============================================================================

//! X86/X64 80-bit Fp register.
struct FpReg : public X86Reg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy Fp register.
  ASMJIT_INLINE FpReg() : X86Reg() {}
  //! Create a reference to `other` FPU register.
  ASMJIT_INLINE FpReg(const FpReg& other) : X86Reg(other) {}
  //! Create a custom Fp register.
  ASMJIT_INLINE FpReg(uint32_t type, uint32_t index, uint32_t size) : X86Reg(type, index, size) {}
  //! Create non-initialized Fp register.
  explicit ASMJIT_INLINE FpReg(const _NoInit&) : X86Reg(NoInit) {}

  // --------------------------------------------------------------------------
  // [FpReg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(FpReg)
};

// ============================================================================
// [asmjit::x86x64::MmReg]
// ============================================================================

//! X86/X64 64-bit Mm register.
struct MmReg : public X86Reg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy Mm register.
  ASMJIT_INLINE MmReg() : X86Reg() {}
  //! Create a reference to `other` Mm register.
  ASMJIT_INLINE MmReg(const MmReg& other) : X86Reg(other) {}
  //! Create a custom Mm register.
  ASMJIT_INLINE MmReg(uint32_t type, uint32_t index, uint32_t size) : X86Reg(type, index, size) {}
  //! Create non-initialized Mm register.
  explicit ASMJIT_INLINE MmReg(const _NoInit&) : X86Reg(NoInit) {}

  // --------------------------------------------------------------------------
  // [MmReg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(MmReg)
};

// ============================================================================
// [asmjit::x86x64::XmmReg]
// ============================================================================

//! X86/X64 128-bit Xmm register.
struct XmmReg : public X86Reg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy Xmm register.
  ASMJIT_INLINE XmmReg() : X86Reg() {}
  //! Create a reference to `other` Xmm register.
  ASMJIT_INLINE XmmReg(const XmmReg& other) : X86Reg(other) {}
  //! Create a custom Xmm register.
  ASMJIT_INLINE XmmReg(uint32_t type, uint32_t index, uint32_t size) : X86Reg(type, index, size) {}
  //! Create non-initialized Xmm register.
  explicit ASMJIT_INLINE XmmReg(const _NoInit&) : X86Reg(NoInit) {}

  // --------------------------------------------------------------------------
  // [XmmReg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(XmmReg)
};

// ============================================================================
// [asmjit::x86x64::YmmReg]
// ============================================================================

//! X86/X64 256-bit Ymm register.
struct YmmReg : public X86Reg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy Ymm register.
  ASMJIT_INLINE YmmReg() : X86Reg() {}
  //! Create a reference to `other` Xmm register.
  ASMJIT_INLINE YmmReg(const YmmReg& other) : X86Reg(other) {}
  //! Create a custom Ymm register.
  ASMJIT_INLINE YmmReg(uint32_t type, uint32_t index, uint32_t size) : X86Reg(type, index, size) {}
  //! Create non-initialized Ymm register.
  explicit ASMJIT_INLINE YmmReg(const _NoInit&) : X86Reg(NoInit) {}

  // --------------------------------------------------------------------------
  // [YmmReg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(YmmReg)
};

// ============================================================================
// [asmjit::x86x64::SegReg]
// ============================================================================

//! X86/X64 segment register.
struct SegReg : public X86Reg {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy segment register.
  ASMJIT_INLINE SegReg() : X86Reg() {}
  //! Create a reference to `other` segment register.
  ASMJIT_INLINE SegReg(const SegReg& other) : X86Reg(other) {}
  //! Create a custom segment register.
  ASMJIT_INLINE SegReg(uint32_t type, uint32_t index, uint32_t size) : X86Reg(type, index, size) {}
  //! Create non-initialized segment register.
  explicit ASMJIT_INLINE SegReg(const _NoInit&) : X86Reg(NoInit) {}

  // --------------------------------------------------------------------------
  // [SegReg Specific]
  // --------------------------------------------------------------------------

  ASMJIT_REG_OP(SegReg)
};

// ============================================================================
// [asmjit::x86x64::Mem]
// ============================================================================

//! X86 memory operand.
struct Mem : public BaseMem {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Mem() : BaseMem(NoInit) {
    reset();
  }

  ASMJIT_INLINE Mem(const Label& label, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeLabel, 0, label._base.id);
    _init_packed_d2_d3(kInvalidValue, disp);
  }

  ASMJIT_INLINE Mem(const Label& label, const GpReg& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeLabel,
      (kMemVSibGpz << kMemVSibIndex)
        + (shift << kMemShiftIndex),
      label.getId());
    _vmem.index = index.getRegIndex();
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const Label& label, const GpVar& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeLabel,
      (kMemVSibGpz << kMemVSibIndex)
        + (shift << kMemShiftIndex),
      label.getId());
    _vmem.index = _OP_ID(index);
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const GpReg& base, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(base)
        + (kMemVSibGpz << kMemVSibIndex),
      base.getRegIndex());
    _init_packed_d2_d3(kInvalidValue, disp);
  }

  ASMJIT_INLINE Mem(const GpReg& base, const GpReg& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(base) + (shift << kMemShiftIndex),
      base.getRegIndex());
    _vmem.index = index.getRegIndex();
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const GpReg& base, const XmmReg& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(base)
        + (kMemVSibXmm << kMemVSibIndex)
        + (shift << kMemShiftIndex),
      base.getRegIndex());
    _vmem.index = index.getRegIndex();
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const GpReg& base, const YmmReg& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(base)
        + (kMemVSibYmm << kMemVSibIndex)
        + (shift << kMemShiftIndex),
      base.getRegIndex());
    _vmem.index = index.getRegIndex();
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const GpVar& base, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(reinterpret_cast<const BaseVar&>(base))
        + (kMemVSibGpz << kMemVSibIndex),
      _OP_ID(base));
    _init_packed_d2_d3(kInvalidValue, disp);
  }


  ASMJIT_INLINE Mem(const GpVar& base, const GpVar& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(reinterpret_cast<const BaseVar&>(base))
        + (shift << kMemShiftIndex),
      _OP_ID(base));
    _vmem.index = _OP_ID(index);
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const GpVar& base, const XmmVar& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(reinterpret_cast<const BaseVar&>(base))
        + (kMemVSibXmm << kMemVSibIndex)
        + (shift << kMemShiftIndex),
      _OP_ID(base));
    _vmem.index = _OP_ID(index);
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const GpVar& base, const YmmVar& index, uint32_t shift, int32_t disp, uint32_t size = 0) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeBaseIndex,
      _getGpdFlags(reinterpret_cast<const BaseVar&>(base))
        + (kMemVSibYmm << kMemVSibIndex)
        + (shift << kMemShiftIndex),
      _OP_ID(base));
    _vmem.index = _OP_ID(index);
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const _Init&, uint32_t memType, const X86Var& base, int32_t disp, uint32_t size) : BaseMem(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, memType, 0, _OP_ID(base));
    _vmem.index = kInvalidValue;
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const _Init&, uint32_t memType, const X86Var& base, const GpVar& index, uint32_t shift, int32_t disp, uint32_t size) : BaseMem(NoInit) {
    ASMJIT_ASSERT(shift <= 3);

    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, memType, shift << kMemShiftIndex, _OP_ID(base));
    _vmem.index = _OP_ID(index);
    _vmem.displacement = disp;
  }

  ASMJIT_INLINE Mem(const Mem& other) : BaseMem(other) {}
  explicit ASMJIT_INLINE Mem(const _NoInit&) : BaseMem(NoInit) {}

  // --------------------------------------------------------------------------
  // [Mem Specific]
  // --------------------------------------------------------------------------

  //! Clone Mem operand.
  ASMJIT_INLINE Mem clone() const {
    return Mem(*this);
  }

  //! Reset Mem operand.
  ASMJIT_INLINE void reset() {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, 0, kMemTypeBaseIndex, 0, kInvalidValue);
    _init_packed_d2_d3(kInvalidValue, 0);
  }

  //! \internal
  ASMJIT_INLINE void _init(uint32_t memType, uint32_t base, int32_t disp, uint32_t size) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, memType, 0, base);
    _vmem.index = kInvalidValue;
    _vmem.displacement = disp;
  }

  // --------------------------------------------------------------------------
  // [Segment]
  // --------------------------------------------------------------------------

  //! Get whether the memory operand has segment override prefix.
  ASMJIT_INLINE bool hasSegment() const {
    return (_vmem.flags & kMemSegMask) != (kSegDefault << kMemSegIndex);
  }

  //! Get memory operand segment, see `kSeg`.
  ASMJIT_INLINE uint32_t getSegment() const {
    return (static_cast<uint32_t>(_vmem.flags) >> kMemSegIndex) & kMemSegBits;
  }

  //! Set memory operand segment, see `kSeg`.
  ASMJIT_INLINE Mem& setSegment(uint32_t segIndex) {
    _vmem.flags = static_cast<uint8_t>(
      (static_cast<uint32_t>(_vmem.flags) & kMemSegMask) + (segIndex << kMemSegIndex));
    return *this;
  }

  //! Set memory operand segment, see `kSeg`.
  ASMJIT_INLINE Mem& setSegment(const SegReg& seg) {
    return setSegment(seg.getRegIndex());
  }

  // --------------------------------------------------------------------------
  // [Gpd]
  // --------------------------------------------------------------------------

  //! Get whether the memory operand has 32-bit GP base.
  ASMJIT_INLINE bool hasGpdBase() const {
    return (_packed[0].u32[0] & IntUtil::pack32_4x8(0x00, 0x00, 0x00, kMemGpdMask)) != 0;
  }

  //! Set whether the memory operand has 32-bit GP base.
  ASMJIT_INLINE Mem& setGpdBase() {
    _packed[0].u32[0] |= IntUtil::pack32_4x8(0x00, 0x00, 0x00, kMemGpdMask);
    return *this;
  }

  //! Set whether the memory operand has 32-bit GP base to `b`.
  ASMJIT_INLINE Mem& setGpdBase(uint32_t b) {
    _packed[0].u32[0] &=~IntUtil::pack32_4x8(0x00, 0x00, 0x00, kMemGpdMask);
    _packed[0].u32[0] |= IntUtil::pack32_4x8(0x00, 0x00, 0x00, b << kMemGpdIndex);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [VSib]
  // --------------------------------------------------------------------------

  //! Get SIB type.
  ASMJIT_INLINE uint32_t getVSib() const {
    return (static_cast<uint32_t>(_vmem.flags) >> kMemVSibIndex) & kMemVSibBits;
  }

  //! Set SIB type.
  ASMJIT_INLINE Mem& _setVSib(uint32_t vsib) {
    _packed[0].u32[0] &=~IntUtil::pack32_4x8(0x00, 0x00, 0x00, kMemVSibMask);
    _packed[0].u32[0] |= IntUtil::pack32_4x8(0x00, 0x00, 0x00, vsib << kMemVSibIndex);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Size]
  // --------------------------------------------------------------------------

  //! Set memory operand size.
  ASMJIT_INLINE Mem& setSize(uint32_t size) {
    _vmem.size = static_cast<uint8_t>(size);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Base]
  // --------------------------------------------------------------------------

  //! Get whether the memory operand has base register.
  ASMJIT_INLINE bool hasBase() const {
    return _vmem.base != kInvalidValue;
  }

  //! Get memory operand base register code, variable id, or `kInvalidValue`.
  ASMJIT_INLINE uint32_t getBase() const {
    return _vmem.base;
  }

  //! Set memory operand base register code, variable id, or `kInvalidValue`.
  ASMJIT_INLINE Mem& setBase(uint32_t base) {
    _vmem.base = base;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Index]
  // --------------------------------------------------------------------------

  //! Get whether the memory operand has index.
  ASMJIT_INLINE bool hasIndex() const {
    return _vmem.index != kInvalidValue;
  }

  //! Get memory operand index register code, variable id, or `kInvalidValue`.
  ASMJIT_INLINE uint32_t getIndex() const {
    return _vmem.index;
  }

  //! Set memory operand index register code, variable id, or `kInvalidValue`.
  ASMJIT_INLINE Mem& setIndex(uint32_t index) {
    _vmem.index = index;
    return *this;
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const GpReg& index) {
    _vmem.index = index.getRegIndex();
    return _setVSib(kMemVSibGpz);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const GpReg& index, uint32_t shift) {
    _vmem.index = index.getRegIndex();
    return _setVSib(kMemVSibGpz).setShift(shift);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const GpVar& index) {
    _vmem.index = reinterpret_cast<const BaseVar&>(index).getId();
    return _setVSib(kMemVSibGpz);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const GpVar& index, uint32_t shift) {
    _vmem.index = reinterpret_cast<const BaseVar&>(index).getId();
    return _setVSib(kMemVSibGpz).setShift(shift);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const XmmReg& index) {
    _vmem.index = index.getRegIndex();
    return _setVSib(kMemVSibXmm);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const XmmReg& index, uint32_t shift) {
    _vmem.index = index.getRegIndex();
    return _setVSib(kMemVSibXmm).setShift(shift);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const XmmVar& index) {
    _vmem.index = reinterpret_cast<const BaseVar&>(index).getId();
    return _setVSib(kMemVSibXmm);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const XmmVar& index, uint32_t shift) {
    _vmem.index = reinterpret_cast<const BaseVar&>(index).getId();
    return _setVSib(kMemVSibXmm).setShift(shift);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const YmmReg& index) {
    _vmem.index = index.getRegIndex();
    return _setVSib(kMemVSibYmm);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const YmmReg& index, uint32_t shift) {
    _vmem.index = index.getRegIndex();
    return _setVSib(kMemVSibYmm).setShift(shift);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const YmmVar& index) {
    _vmem.index = reinterpret_cast<const BaseVar&>(index).getId();
    return _setVSib(kMemVSibYmm);
  }

  //! Set memory index.
  ASMJIT_INLINE Mem& setIndex(const YmmVar& index, uint32_t shift) {
    _vmem.index = reinterpret_cast<const BaseVar&>(index).getId();
    return _setVSib(kMemVSibYmm).setShift(shift);
  }

  //! Reset memory index.
  ASMJIT_INLINE Mem& resetIndex() {
    _vmem.index = kInvalidValue;
    return _setVSib(kMemVSibGpz);
  }

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  //! Get whether the memory operand has base and index register.
  ASMJIT_INLINE bool hasBaseOrIndex() const {
    return _vmem.base != kInvalidValue || _vmem.index != kInvalidValue;
  }

  //! Get whether the memory operand has base and index register.
  ASMJIT_INLINE bool hasBaseAndIndex() const {
    return _vmem.base != kInvalidValue && _vmem.index != kInvalidValue;
  }

  // --------------------------------------------------------------------------
  // [Shift]
  // --------------------------------------------------------------------------

  //! Get whether the memory operand has shift used.
  ASMJIT_INLINE bool hasShift() const {
    return (_vmem.flags & kMemShiftMask) != 0;
  }

  //! Get memory operand index scale (0, 1, 2 or 3).
  ASMJIT_INLINE uint32_t getShift() const {
    return _vmem.flags >> kMemShiftIndex;
  }

  //! Set memory operand index scale (0, 1, 2 or 3).
  ASMJIT_INLINE Mem& setShift(uint32_t shift) {
    _packed[0].u32[0] &=~IntUtil::pack32_4x8(0x00, 0x00, 0x00, kMemShiftMask);
    _packed[0].u32[0] |= IntUtil::pack32_4x8(0x00, 0x00, 0x00, shift << kMemShiftIndex);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Displacement]
  // --------------------------------------------------------------------------

  //! Get memory operand relative displacement.
  ASMJIT_INLINE int32_t getDisplacement() const {
    return _vmem.displacement;
  }

  //! Set memory operand relative displacement.
  ASMJIT_INLINE Mem& setDisplacement(int32_t disp) {
    _vmem.displacement = disp;
    return *this;
  }

  //! Reset memory operand relative displacement.
  ASMJIT_INLINE Mem& resetDisplacement(int32_t /*disp*/) {
    _vmem.displacement = 0;
    return *this;
  }

  //! Adjust memory operand relative displacement by `disp`.
  ASMJIT_INLINE Mem& adjust(int32_t disp) {
    _vmem.displacement += disp;
    return *this;
  }

  //! Get new memory operand adjusted by `disp`.
  ASMJIT_INLINE Mem adjusted(int32_t disp) const {
    Mem result(*this);
    result.adjust(disp);
    return result;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Mem& operator=(const Mem& other) {
    _copy(other);
    return *this;
  }

  ASMJIT_INLINE bool operator==(const Mem& other) const {
    return (_packed[0] == other._packed[0]) & (_packed[1] == other._packed[1]) ;
  }

  ASMJIT_INLINE bool operator!=(const Mem& other) const {
    return !(*this == other);
  }

  // --------------------------------------------------------------------------
  // [Static]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE uint32_t _getGpdFlags(const Operand& base) {
    return (base._vreg.size & 0x4) << (kMemGpdIndex - 2);
  }
};

// ============================================================================
// [asmjit::x86x64::X86Var]
// ============================================================================

//! Base class for all variables.
struct X86Var : public BaseVar {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86Var() : BaseVar(NoInit) {
    reset();
  }

  ASMJIT_INLINE X86Var(const X86Var& other) : BaseVar(other) {}

  explicit ASMJIT_INLINE X86Var(const _NoInit&) : BaseVar(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86Var Specific]
  // --------------------------------------------------------------------------

  //! Clone X86Var operand.
  ASMJIT_INLINE X86Var clone() const {
    return X86Var(*this);
  }

  //! Reset X86Var operand.
  ASMJIT_INLINE void reset() {
    _init_packed_op_sz_b0_b1_id(kOperandTypeVar, 0, kInvalidReg, kInvalidReg, kInvalidValue);
    _init_packed_d2_d3(kInvalidValue, kInvalidValue);
  }

  // --------------------------------------------------------------------------
  // [Type]
  // --------------------------------------------------------------------------

  //! Get register type.
  ASMJIT_INLINE uint32_t getRegType() const { return _vreg.type; }
  //! Get variable type.
  ASMJIT_INLINE uint32_t getVarType() const { return _vreg.vType; }

  //! Get whether the variable is Gpb register.
  ASMJIT_INLINE bool isGp() const { return _vreg.type <= kRegTypeGpq; }
  //! Get whether the variable is Gpb register.
  ASMJIT_INLINE bool isGpb() const { return _vreg.type <= kRegTypeGpbHi; }
  //! Get whether the variable is Gpb-lo register.
  ASMJIT_INLINE bool isGpbLo() const { return _vreg.type == kRegTypeGpbLo; }
  //! Get whether the variable is Gpb-hi register.
  ASMJIT_INLINE bool isGpbHi() const { return _vreg.type == kRegTypeGpbHi; }
  //! Get whether the variable is Gpw register.
  ASMJIT_INLINE bool isGpw() const { return _vreg.type == kRegTypeGpw; }
  //! Get whether the variable is Gpd register.
  ASMJIT_INLINE bool isGpd() const { return _vreg.type == kRegTypeGpd; }
  //! Get whether the variable is Gpq register.
  ASMJIT_INLINE bool isGpq() const { return _vreg.type == kRegTypeGpq; }

  //! Get whether the variable is Fp register.
  ASMJIT_INLINE bool isFp() const { return _vreg.type == kRegTypeFp; }
  //! Get whether the variable is Mm type.
  ASMJIT_INLINE bool isMm() const { return _vreg.type == kRegTypeMm; }
  //! Get whether the variable is Xmm type.
  ASMJIT_INLINE bool isXmm() const { return _vreg.type == kRegTypeXmm; }
  //! Get whether the variable is Ymm type.
  ASMJIT_INLINE bool isYmm() const { return _vreg.type == kRegTypeYmm; }

  // --------------------------------------------------------------------------
  // [Memory Cast]
  // --------------------------------------------------------------------------

  //! Cast this variable to a memory operand.
  //!
  //! \note Size of operand depends on native variable type, you can use other
  //! variants if you want specific one.
  ASMJIT_INLINE Mem m(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, getSize());
  }

  //! \overload
  ASMJIT_INLINE Mem m(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, getSize());
  }

  //! Cast this variable to 8-bit memory operand.
  ASMJIT_INLINE Mem m8(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 1);
  }

  //! \overload
  ASMJIT_INLINE Mem m8(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 1);
  }

  //! Cast this variable to 16-bit memory operand.
  ASMJIT_INLINE Mem m16(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 2);
  }

  //! \overload
  ASMJIT_INLINE Mem m16(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 2);
  }

  //! Cast this variable to 32-bit memory operand.
  ASMJIT_INLINE Mem m32(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 4);
  }

  //! \overload
  ASMJIT_INLINE Mem m32(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 4);
  }

  //! Cast this variable to 64-bit memory operand.
  ASMJIT_INLINE Mem m64(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 8);
  }

  //! \overload
  ASMJIT_INLINE Mem m64(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 8);
  }

  //! Cast this variable to 80-bit memory operand (long double).
  ASMJIT_INLINE Mem m80(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 10);
  }

  //! \overload
  ASMJIT_INLINE Mem m80(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 10);
  }

  //! Cast this variable to 128-bit memory operand.
  ASMJIT_INLINE Mem m128(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 16);
  }

  //! \overload
  ASMJIT_INLINE Mem m128(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 16);
  }

  //! Cast this variable to 256-bit memory operand.
  ASMJIT_INLINE Mem m256(int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, disp, 32);
  }

  //! \overload
  ASMJIT_INLINE Mem m256(const GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 32);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86Var& operator=(const X86Var& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const X86Var& other) const { return _packed[0] == other._packed[0]; }
  ASMJIT_INLINE bool operator!=(const X86Var& other) const { return !operator==(other); }

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

protected:
  ASMJIT_INLINE X86Var(const X86Var& other, uint32_t reg, uint32_t size) : BaseVar(NoInit) {
    _init_packed_op_sz_w0_id(kOperandTypeVar, size, (reg << 8) + other._vreg.index, other._base.id);
    _vreg.vType = other._vreg.vType;
  }
};

// ============================================================================
// [asmjit::x86x64::GpVar]
// ============================================================================

//! Gp variable.
struct GpVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `GpVar` instance.
  ASMJIT_INLINE GpVar() : X86Var() {}

  //! Create a new initialized `GpVar` instance.
  ASMJIT_INLINE GpVar(BaseCompiler& c, uint32_t type = kVarTypeIntPtr, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE GpVar(const GpVar& other) : X86Var(other) {}

  //! Create a new uninitialized `GpVar` instance (internal).
  explicit ASMJIT_INLINE GpVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [GpVar Specific]
  // --------------------------------------------------------------------------

  //! Clone GpVar operand.
  ASMJIT_INLINE GpVar clone() const {
    return GpVar(*this);
  }

  //! Reset GpVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [GpVar Cast]
  // --------------------------------------------------------------------------

  //! Cast this variable to 8-bit (LO) part of variable
  ASMJIT_INLINE GpVar r8() const { return GpVar(*this, kRegTypeGpbLo, 1); }
  //! Cast this variable to 8-bit (LO) part of variable
  ASMJIT_INLINE GpVar r8Lo() const { return GpVar(*this, kRegTypeGpbLo, 1); }
  //! Cast this variable to 8-bit (HI) part of variable
  ASMJIT_INLINE GpVar r8Hi() const { return GpVar(*this, kRegTypeGpbHi, 1); }

  //! Cast this variable to 16-bit part of variable
  ASMJIT_INLINE GpVar r16() const { return GpVar(*this, kRegTypeGpw, 2); }
  //! Cast this variable to 32-bit part of variable
  ASMJIT_INLINE GpVar r32() const { return GpVar(*this, kRegTypeGpd, 4); }
  //! Cast this variable to 64-bit part of variable
  ASMJIT_INLINE GpVar r64() const { return GpVar(*this, kRegTypeGpq, 8); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE GpVar& operator=(const GpVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const GpVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const GpVar& other) const { return X86Var::operator!=(other); }

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

protected:
  ASMJIT_INLINE GpVar(const GpVar& other, uint32_t reg, uint32_t size) : X86Var(other, reg, size) {}
};

// ============================================================================
// [asmjit::x86x64::FpVar]
// ============================================================================

//! Fpu variable.
struct FpVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `FpVar` instance.
  ASMJIT_INLINE FpVar() : X86Var() {}
  //! Create a new variable that links to `other`.
  ASMJIT_INLINE FpVar(const FpVar& other) : X86Var(other) {}

  //! Create a new uninitialized `FpVar` instance (internal).
  explicit ASMJIT_INLINE FpVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [FpVar Specific]
  // --------------------------------------------------------------------------

  //! Clone FpVar operand.
  ASMJIT_INLINE FpVar clone() const {
    return FpVar(*this);
  }

  //! Reset FpVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FpVar& operator=(const FpVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const FpVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const FpVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::x86x64::MmVar]
// ============================================================================

//! Mm variable.
struct MmVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `MmVar` instance.
  ASMJIT_INLINE MmVar() : X86Var() {}
  //! Create a new initialized `MmVar` instance.
  ASMJIT_INLINE MmVar(BaseCompiler& c, uint32_t type = kVarTypeMm, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE MmVar(const MmVar& other) : X86Var(other) {}

  //! Create a new uninitialized `MmVar` instance (internal).
  explicit ASMJIT_INLINE MmVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [MmVar Specific]
  // --------------------------------------------------------------------------

  //! Clone MmVar operand.
  ASMJIT_INLINE MmVar clone() const {
    return MmVar(*this);
  }

  //! Reset MmVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE MmVar& operator=(const MmVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const MmVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const MmVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::x86x64::XmmVar]
// ============================================================================

//! Xmm variable.
struct XmmVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `XmmVar` instance.
  ASMJIT_INLINE XmmVar() : X86Var() {}
  //! Create a new initialized `XmmVar` instance.
  ASMJIT_INLINE XmmVar(BaseCompiler& c, uint32_t type = kVarTypeXmm, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE XmmVar(const XmmVar& other) : X86Var(other) {}

  //! Create a new uninitialized `XmmVar` instance (internal).
  explicit ASMJIT_INLINE XmmVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [XmmVar Specific]
  // --------------------------------------------------------------------------

  //! Clone XmmVar operand.
  ASMJIT_INLINE XmmVar clone() const {
    return XmmVar(*this);
  }

  //! Reset XmmVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE XmmVar& operator=(const XmmVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const XmmVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const XmmVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::x86x64::YmmVar]
// ============================================================================

//! Ymm variable.
struct YmmVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `YmmVar` instance.
  ASMJIT_INLINE YmmVar() : X86Var() {}
  //! Create a new initialized `YmmVar` instance.
  ASMJIT_INLINE YmmVar(BaseCompiler& c, uint32_t type = kVarTypeYmm, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE YmmVar(const YmmVar& other) : X86Var(other) {}

  //! Create a new uninitialized `YmmVar` instance (internal).
  explicit ASMJIT_INLINE YmmVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [YmmVar Specific]
  // --------------------------------------------------------------------------

  //! Clone YmmVar operand.
  ASMJIT_INLINE YmmVar clone() const {
    return YmmVar(*this);
  }

  //! Reset YmmVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE YmmVar& operator=(const YmmVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const YmmVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const YmmVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::x86x64::Registers]
// ============================================================================

//! Make 8-bit Gpb-lo register operand.
static ASMJIT_INLINE GpReg gpb_lo(uint32_t index) { return GpReg(kRegTypeGpbLo, index, 1); }
//! Make 8-bit Gpb-hi register operand.
static ASMJIT_INLINE GpReg gpb_hi(uint32_t index) { return GpReg(kRegTypeGpbHi, index, 1); }
//! Make 16-bit Gpw register operand.
static ASMJIT_INLINE GpReg gpw(uint32_t index) { return GpReg(kRegTypeGpw, index, 2); }
//! Make 32-bit Gpd register operand.
static ASMJIT_INLINE GpReg gpd(uint32_t index) { return GpReg(kRegTypeGpd, index, 4); }
//! Make 64-bit Gpq register operand (X64).
static ASMJIT_INLINE GpReg gpq(uint32_t index) { return GpReg(kRegTypeGpq, index, 8); }
//! Make 80-bit Fp register operand.
static ASMJIT_INLINE FpReg fp(uint32_t index) { return FpReg(kRegTypeFp, index, 10); }
//! Make 64-bit Mm register operand.
static ASMJIT_INLINE MmReg mm(uint32_t index) { return MmReg(kRegTypeMm, index, 8); }
//! Make 128-bit Xmm register operand.
static ASMJIT_INLINE XmmReg xmm(uint32_t index) { return XmmReg(kRegTypeXmm, index, 16); }
//! Make 256-bit Ymm register operand.
static ASMJIT_INLINE YmmReg ymm(uint32_t index) { return YmmReg(kRegTypeYmm, index, 32); }

// ============================================================================
// [asmjit::x86x64::Memory]
// ============================================================================

//! Make `[base.reg + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpReg& base, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, disp, size);
}
//! Make `[base.var + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpVar& base, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, disp, size);
}

//! Make `[base.reg + (index.reg << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpReg& base, const GpReg& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, index, shift, disp, size);
}
//! Make `[base.var + (index.var << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpVar& base, const GpVar& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, index, shift, disp, size);
}

//! Make `[base.reg + (xmm.reg << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpReg& base, const XmmReg& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, index, shift, disp, size);
}
//! Make `[base.var + (xmm.var << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpVar& base, const XmmVar& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, index, shift, disp, size);
}
//! Make `[base.reg + (ymm.reg << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpReg& base, const YmmReg& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, index, shift, disp, size);
}
//! Make `[base.var + (ymm.var << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const GpVar& base, const YmmVar& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0) {
  return Mem(base, index, shift, disp, size);
}

//! Make `[label + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const Label& label, int32_t disp = 0, uint32_t size = 0) {
  return Mem(label, disp, size);
}
//! Make `[label + (index.reg << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const Label& label, const GpReg& index, uint32_t shift, int32_t disp = 0, uint32_t size = 0) { \
  return Mem(label, index, shift, disp, size); \
}
//! Make `[label + (index.var << shift) + disp]` memory operand with no/custom size information.
static ASMJIT_INLINE Mem ptr(const Label& label, const GpVar& index, uint32_t shift, int32_t disp = 0, uint32_t size = 0) { \
  return Mem(label, index, shift, disp, size); \
}

//! Make `[pAbs + disp]` absolute memory operand with no/custom size information.
ASMJIT_API Mem ptr_abs(Ptr pAbs, int32_t disp = 0, uint32_t size = 0);
//! Make `[pAbs + (index.reg << shift) + disp]` absolute memory operand with no/custom size information.
ASMJIT_API Mem ptr_abs(Ptr pAbs, const X86Reg& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0);
//! Make `[pAbs + (index.var << shift) + disp]` absolute memory operand with no/custom size information.
ASMJIT_API Mem ptr_abs(Ptr pAbs, const X86Var& index, uint32_t shift = 0, int32_t disp = 0, uint32_t size = 0);

//! \internal
#define ASMJIT_X86_DEFINE_PTR(_Prefix_, _Size_) \
  /*! Make `[base.reg + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpReg& base, int32_t disp = 0) { \
    return Mem(base, disp, _Size_); \
  } \
  /*! Make `[base.var + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpVar& base, int32_t disp = 0) { \
    return Mem(base, disp, _Size_); \
  } \
  /*! Make `[base.reg + (index.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpReg& base, const GpReg& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr(base, index, shift, disp, _Size_); \
  } \
  /*! Make `[base.var + (index.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpVar& base, const GpVar& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr(base, index, shift, disp, _Size_); \
  } \
  /*! Make `[base.reg + (xmm.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpReg& base, const XmmReg& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr(base, index, shift, disp, _Size_); \
  } \
  /*! Make `[base.var + (xmm.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpVar& base, const XmmVar& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr(base, index, shift, disp, _Size_); \
  } \
  /*! Make `[base.reg + (ymm.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpReg& base, const YmmReg& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr(base, index, shift, disp, _Size_); \
  } \
  /*! Make `[base.var + (ymm.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const GpVar& base, const YmmVar& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr(base, index, shift, disp, _Size_); \
  } \
  /*! Make `[label + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const Label& label, int32_t disp = 0) { \
    return ptr(label, disp, _Size_); \
  } \
  /*! Make `[label + (index.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const Label& label, const GpReg& index, uint32_t shift, int32_t disp = 0) { \
    return ptr(label, index, shift, disp, _Size_); \
  } \
  /*! Make `[label + (index.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr(const Label& label, const GpVar& index, uint32_t shift, int32_t disp = 0) { \
    return ptr(label, index, shift, disp, _Size_); \
  } \
  /*! Make `[pAbs + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, int32_t disp = 0) { \
    return ptr_abs(pAbs, disp, _Size_); \
  } \
  /*! Make `[pAbs + (index.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, const GpReg& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr_abs(pAbs, index, shift, disp, _Size_); \
  } \
  /*! Make `[pAbs + (index.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, const GpVar& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr_abs(pAbs, index, shift, disp, _Size_); \
  } \
  /*! Make `[pAbs + (xmm.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, const XmmReg& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr_abs(pAbs, index, shift, disp, _Size_); \
  } \
  /*! Make `[pAbs + (xmm.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, const XmmVar& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr_abs(pAbs, index, shift, disp, _Size_); \
  } \
  /*! Make `[pAbs + (ymm.reg << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, const YmmReg& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr_abs(pAbs, index, shift, disp, _Size_); \
  } \
  /*! Make `[pAbs + (ymm.var << shift) + disp]` memory operand. */ \
  static ASMJIT_INLINE Mem _Prefix_##_ptr##_abs(Ptr pAbs, const YmmVar& index, uint32_t shift = 0, int32_t disp = 0) { \
    return ptr_abs(pAbs, index, shift, disp, _Size_); \
  }

ASMJIT_X86_DEFINE_PTR(byte, 1)
ASMJIT_X86_DEFINE_PTR(word, 2)
ASMJIT_X86_DEFINE_PTR(dword, 4)
ASMJIT_X86_DEFINE_PTR(qword, 8)
ASMJIT_X86_DEFINE_PTR(tword, 10)
ASMJIT_X86_DEFINE_PTR(oword, 16)
ASMJIT_X86_DEFINE_PTR(yword, 32)

#undef ASMJIT_X86_DEFINE_PTR

// ============================================================================
// [asmjit::x86x64::x86VarTypeToClass]
// ============================================================================

static ASMJIT_INLINE uint32_t x86VarTypeToClass(uint32_t vType) {
  // Getting varClass is the only safe operation when dealing with denormalized
  // varType. Any other property would require to map vType to the architecture
  // specific one.
  ASMJIT_ASSERT(vType < kVarTypeCount);
  return _varInfo[vType].getClass();
}

//! \}

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x86::kRegCount]
// ============================================================================

//! X86 registers count per class.
ASMJIT_ENUM(kRegCount) {
  //! Base count of registers (8).
  kRegCountBase = 8,
  //! Count of Gp registers (8).
  kRegCountGp = kRegCountBase,
  //! Count of Xmm registers (8).
  kRegCountXmm = kRegCountBase,
  //! Count of Ymm registers (8).
  kRegCountYmm = kRegCountBase
};

// ============================================================================
// [asmjit::x86::Variables]
// ============================================================================

//! \internal
//!
//! Mapping of x86 variables into their real IDs.
//!
//! This mapping translates the following:
//! - `kVarTypeInt64` to `kVarTypeInvalid`.
//! - `kVarTypeUInt64` to `kVarTypeInvalid`.
//! - `kVarTypeIntPtr` to `kVarTypeInt32`.
//! - `kVarTypeUIntPtr` to `kVarTypeUInt32`.
ASMJIT_VAR const uint8_t _varMapping[kVarTypeCount];

// ============================================================================
// [asmjit::x86::Registers]
// ============================================================================

//! Get Gp qword register.
static ASMJIT_INLINE GpReg gpz(uint32_t index) { return GpReg(kRegTypeGpd, index, 4); }

// ============================================================================
// [asmjit::x86::Mem]
// ============================================================================

//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpReg& base, int32_t disp = 0) { return ptr(base, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpVar& base, int32_t disp = 0) { return ptr(base, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpReg& base, const GpReg& index, uint32_t shift = 0, int32_t disp = 0) { return ptr(base, index, shift, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpVar& base, const GpVar& index, uint32_t shift = 0, int32_t disp = 0) { return ptr(base, index, shift, disp, 4); }

//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const Label& label, int32_t disp = 0) { return ptr(label, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const Label& label, const GpReg& index, uint32_t shift, int32_t disp = 0) { return ptr(label, index, shift, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const Label& label, const GpVar& index, uint32_t shift, int32_t disp = 0) { return ptr(label, index, shift, disp, 4); }

//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr_abs(Ptr pAbs, int32_t disp = 0) { return ptr_abs(pAbs, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr_abs(Ptr pAbs, const GpReg& index, uint32_t shift, int32_t disp = 0) { return ptr_abs(pAbs, index, shift, disp, 4); }
//! Create an intptr_t 32-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr_abs(Ptr pAbs, const GpVar& index, uint32_t shift, int32_t disp = 0) { return ptr_abs(pAbs, index, shift, disp, 4); }

//! \}

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)

namespace asmjit {
namespace x64 {

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x64::kRegCount]
// ============================================================================

//! X86 registers count per class.
ASMJIT_ENUM(kRegCount) {
  //! Base count of registers (16).
  kRegCountBase = 16,
  //! Count of Gp registers (16).
  kRegCountGp = kRegCountBase,
  //! Count of Xmm registers (16).
  kRegCountXmm = kRegCountBase,
  //! Count of Ymm registers (16).
  kRegCountYmm = kRegCountBase
};

// ============================================================================
// [asmjit::x64::Variables]
// ============================================================================

//! \internal
//!
//! Mapping of x64 variables into their real IDs.
//!
//! This mapping translates the following:
//! - `kVarTypeIntPtr` to `kVarTypeInt64`.
//! - `kVarTypeUIntPtr` to `kVarTypeUInt64`.
ASMJIT_VAR const uint8_t _varMapping[kVarTypeCount];

// ============================================================================
// [asmjit::x64::Registers]
// ============================================================================

//! Get Gpq register.
static ASMJIT_INLINE GpReg gpz(uint32_t index) { return GpReg(kRegTypeGpq, index, 8); }

// ============================================================================
// [asmjit::x64::Mem]
// ============================================================================

//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const Label& label, int32_t disp = 0) { return ptr(label, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const Label& label, const GpReg& index, uint32_t shift, int32_t disp = 0) { return ptr(label, index, shift, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const Label& label, const GpVar& index, uint32_t shift, int32_t disp = 0) { return ptr(label, index, shift, disp, 8); }

//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr_abs(Ptr pAbs, int32_t disp = 0) { return ptr_abs(pAbs, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr_abs(Ptr pAbs, const GpReg& index, uint32_t shift, int32_t disp = 0) { return ptr_abs(pAbs, index, shift, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr_abs(Ptr pAbs, const GpVar& index, uint32_t shift, int32_t disp = 0) { return ptr_abs(pAbs, index, shift, disp, 8); }

//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpReg& base, int32_t disp = 0) { return ptr(base, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpVar& base, int32_t disp = 0) { return ptr(base, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpReg& base, const GpReg& index, uint32_t shift = 0, int32_t disp = 0) { return ptr(base, index, shift, disp, 8); }
//! Create an intptr_t 64-bit pointer operand.
static ASMJIT_INLINE Mem intptr_ptr(const GpVar& base, const GpVar& index, uint32_t shift = 0, int32_t disp = 0) { return ptr(base, index, shift, disp, 8); }

//! \}

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

#undef _OP_ID

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86OPERAND_H
