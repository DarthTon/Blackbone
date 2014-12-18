// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_OPERAND_H
#define _ASMJIT_BASE_OPERAND_H

// [Dependencies - AsmJit]
#include "../base/intutil.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Assembler;
struct Compiler;

//! \addtogroup asmjit_base_general
//! \{

// ============================================================================
// [asmjit::kOperandType]
// ============================================================================

//! Operand types that can be encoded in `Operand`.
ASMJIT_ENUM(kOperandType) {
  //! Invalid operand, used only internally (not initialized Operand).
  kOperandTypeNone = 0,
  //! Operand is a register.
  kOperandTypeReg = 1,
  //! Operand is a variable.
  kOperandTypeVar = 2,
  //! Operand is a memory.
  kOperandTypeMem = 3,
  //! Operand is an immediate value.
  kOperandTypeImm = 4,
  //! Operand is a label.
  kOperandTypeLabel = 5
};

// ============================================================================
// [asmjit::kOperandId]
// ============================================================================

//! Operand id masks used to determine the operand type.
ASMJIT_ENUM(kOperandId) {
  //! Operand id refers to `Var`.
  kOperandIdVar = 0x80000000U,
  //! Operand id to real index mask.
  kOperandIdNum = 0x7FFFFFFFU
};

// ============================================================================
// [asmjit::kRegClass]
// ============================================================================

//! Register class.
ASMJIT_ENUM(kRegClass) {
  //! Gp register class, compatible with all architectures.
  kRegClassGp = 0
};

// ============================================================================
// [asmjit::kSize]
// ============================================================================

//! Common size of registers and pointers.
ASMJIT_ENUM(kSize) {
  //! 1 byte size (BYTE).
  kSizeByte = 1,
  //! 2 bytes size (WORD).
  kSizeWord = 2,
  //! 4 bytes size (DWORD).
  kSizeDWord = 4,
  //! 8 bytes size (QWORD).
  kSizeQWord = 8,
  //! 10 bytes size (TWORD).
  kSizeTWord = 10,
  //! 16 bytes size (OWORD / DQWORD).
  kSizeOWord = 16,
  //! 32 bytes size (YWORD / QQWORD).
  kSizeYWord = 32
};

// ============================================================================
// [asmjit::kMemType]
// ============================================================================

//! Type of memory operand.
ASMJIT_ENUM(kMemType) {
  //! Memory operand is a combination of base register and optional index register
  //! and displacement.
  //!
  //! The `Assembler` interprets `kMemTypeBaseIndex` and `kMemTypeStackIndex`
  //! types the same way, but `Compiler` interprets  `kMemTypeBaseIndex` as
  //! `[base + index]` and `kMemTypeStackIndex` as `[stack(base) + index]`.
  kMemTypeBaseIndex = 0,

  //! Memory operand is a combination of variable's memory location,
  //! optional index register and displacement.
  //!
  //! The `Assembler` interprets `kMemTypeBaseIndex` and  `kMemTypeStackIndex`
  //! types in the same way, but `Compiler` interprets `kMemTypeBaseIndex` as
  //! `[base + index]` and `kMemTypeStackIndex` as `[stack(base) + index]`.
  kMemTypeStackIndex = 1,

  //! Memory operand refers to the memory location specified by a label.
  kMemTypeLabel = 2,
  //! Memory operand is an absolute memory location.
  //!
  //! Supported mostly by x86, truncated to a 32-bit value when running in
  //! 64-bit mode (x64).
  kMemTypeAbsolute = 3
};

// ============================================================================
// [asmjit::Operand]
// ============================================================================

//! Operand can contain register, memory location, immediate, or label.
struct Operand {
  // --------------------------------------------------------------------------
  // [Structs]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Base operand data.
  struct BaseOp {
    //! Type of operand, see `kOperandType`.
    uint8_t op;
    //! Size of operand (register, address, immediate, or variable).
    uint8_t size;
    //! \internal
    uint8_t reserved_2_1;
    //! \internal
    uint8_t reserved_3_1;

    //! Operand id, identifier used by `Assembler` and `Compiler`.
    //!
    //! \note Uninitialized operand has always set id to `kInvalidValue`.
    uint32_t id;

    //! \internal
    uint32_t reserved_8_4;
    //! \internal
    uint32_t reserved_12_4;
  };

  //! \internal
  //!
  //! Register or Variable operand data.
  struct VRegOp {
    //! Type of operand, `kOperandTypeReg`.
    uint8_t op;
    //! Size of register or variable.
    uint8_t size;

    union {
      //! Register code = (type << 8) | index.
      uint16_t code;

      //! Register type and index access.
      struct {
#if defined(ASMJIT_HOST_LE)
        //! Register index.
        uint8_t index;
        //! Register type.
        uint8_t type;
#else
        //! Register type.
        uint8_t type;
        //! Register index.
        uint8_t index;
#endif // ASMJIT_HOST
      };
    };

    //! Variable id, used by `Compiler` to identify variables.
    uint32_t id;

    union {
      struct {
        //! Variable type.
        uint32_t vType;
        //! \internal
        uint32_t reserved_12_4;
      };

      //! \internal
      //!
      //! This is not needed or used, it's just to force compiler to always
      //! align this struct to 8-bytes (so the struct is compatible to others
      //! when it comes to alignment). It should fix VS linker warning as well.
      uint64_t reserved8_8;
    };
  };

  //! \internal
  //!
  //! Memory or Variable operand data.
  struct VMemOp {
    //! Type of operand, `kOperandTypeMem`.
    uint8_t op;
    //! Size of the pointer in bytes.
    uint8_t size;
    //! Type of the memory operand, see `kMemType`.
    uint8_t type;
    //! X86/X64 layout:
    //!   - segment  [3 bits], see `kX86Seg`.
    //!   - shift    [2 bits], index register shift (0 to 3).
    uint8_t flags;

    //! Base register, variable or label id.
    uint32_t base;
    //! Index register or variable.
    uint32_t index;
    //! 32-bit displacement or absolute address.
    int32_t displacement;
  };

  //! \internal
  //!
  //! Immediate operand data.
  struct ImmOp {
    //! Type of operand, `kOperandTypeImm`.
    uint8_t op;
    //! Size of immediate (or 0 to autodetect).
    uint8_t size;
    //! \internal
    uint8_t reserved_2_1;
    //! \internal
    uint8_t reserved_3_1;

    //! Operand id, always set to `kInvalidValue` (immediates don't have IDs).
    uint32_t id;

    union {
      //! 8x8-bit signed immediate values.
      int8_t _i8[8];
      //! 8x8-bit unsigned immediate values.
      uint8_t _u8[8];

      //! 4x16-bit signed immediate values.
      int16_t _i16[4];
      //! 4x16-bit unsigned immediate values.
      uint16_t _u16[4];

      //! 2x32-bit signed immediate values.
      int32_t _i32[2];
      //! 2x32-bit unsigned immediate values.
      uint32_t _u32[2];

      //! 1x64-bit signed immediate value.
      int64_t _i64[1];
      //! 1x64-bit unsigned immediate value.
      uint64_t _u64[1];

      //! 2x SP-FP values.
      float _f32[2];
      //! 1x DP-FP value.
      double _f64[1];
    } value;
  };

  //! \internal
  //!
  //! Label operand data.
  struct LabelOp {
    //! Type of operand, `kOperandTypeLabel`.
    uint8_t op;
    //! Always zero, labels don't have size.
    uint8_t size;
    //! \internal
    uint8_t reserved_2_1;
    //! \internal
    uint8_t reserved_3_1;

    //! Operand id (`kInvalidValue` if the label is not initialized by code
    //! generator).
    uint32_t id;

    //! \internal
    uint32_t reserved_8_4;
    //! \internal
    uint32_t reserved_12_4;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create an uninitialized operand.
  ASMJIT_INLINE Operand() {
    _init_packed_op_sz_b0_b1_id(kOperandTypeNone, 0, 0, 0, kInvalidValue);
    _init_packed_d2_d3(0, 0);
  }

  //! Create a reference to `other` operand.
  ASMJIT_INLINE Operand(const Operand& other) {
    _init(other);
  }

  explicit ASMJIT_INLINE Operand(const _NoInit&) {}

  // --------------------------------------------------------------------------
  // [Operand]
  // --------------------------------------------------------------------------

  //! Clone `Operand`.
  ASMJIT_INLINE Operand clone() const {
    return Operand(*this);
  }

  // --------------------------------------------------------------------------
  // [Init & Copy]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Initialize operand to `other` (used by constructors).
  ASMJIT_INLINE void _init(const Operand& other) {
    ::memcpy(this, &other, sizeof(Operand));
  }

  ASMJIT_INLINE void _init_packed_op_sz_b0_b1_id(uint32_t op, uint32_t sz, uint32_t r0, uint32_t r1, uint32_t id) {
    // This hack is not for performance, but to decrease the size of the binary
    // generated when constructing AsmJit operands (mostly for third parties).
    // Some compilers are not able to join four BYTE writes to a single DWORD
    // write. Because the 'a', 'b', 'c' and 'd' variables are usually compile
    // time constants the compiler can do a really nice job if they are joined
    // by using bitwise operations.
    _packed[0].setPacked_2x32(IntUtil::pack32_4x8(op, sz, r0, r1), id);
  }

  ASMJIT_INLINE void _init_packed_op_sz_w0_id(uint32_t op, uint32_t sz, uint32_t w0, uint32_t id) {
    _packed[0].setPacked_2x32(IntUtil::pack32_2x8_1x16(op, sz, w0), id);
  }

  ASMJIT_INLINE void _init_packed_d0_d1(uint32_t u0, uint32_t u1) {
    _packed[0].setPacked_2x32(u0, u1);
  }

  ASMJIT_INLINE void _init_packed_d2_d3(uint32_t u2, uint32_t u3) {
    _packed[1].setPacked_2x32(u2, u3);
  }

  //! \internal
  //!
  //! Initialize operand to `other` (used by assign operators).
  ASMJIT_INLINE void _copy(const Operand& other) {
    ::memcpy(this, &other, sizeof(Operand));
  }

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  template<typename T>
  ASMJIT_INLINE T& getData() {
    return reinterpret_cast<T&>(_base);
  }

  template<typename T>
  ASMJIT_INLINE const T& getData() const {
    return reinterpret_cast<const T&>(_base);
  }

  // --------------------------------------------------------------------------
  // [Type]
  // --------------------------------------------------------------------------

  //! Get type of the operand, see `kOperandType`.
  ASMJIT_INLINE uint32_t getOp() const { return _base.op; }

  //! Get whether the operand is none - `kOperandTypeNone`.
  ASMJIT_INLINE bool isNone() const { return (_base.op == kOperandTypeNone); }
  //! Get whether the operand is a register - `kOperandTypeReg`.
  ASMJIT_INLINE bool isReg() const { return (_base.op == kOperandTypeReg); }
  //! Get whether the operand is a variable - `kOperandTypeVar`.
  ASMJIT_INLINE bool isVar() const { return (_base.op == kOperandTypeVar); }
  //! Get whether the operand is a memory address - `kOperandTypeMem`.
  ASMJIT_INLINE bool isMem() const { return (_base.op == kOperandTypeMem); }
  //! Get whether the operand is an immediate value - `kOperandTypeImm`.
  ASMJIT_INLINE bool isImm() const { return (_base.op == kOperandTypeImm); }
  //! Get whether the operand is a label - `kOperandTypeLabel`.
  ASMJIT_INLINE bool isLabel() const { return (_base.op == kOperandTypeLabel); }

  // --------------------------------------------------------------------------
  // [Type - Combined]
  // --------------------------------------------------------------------------

  //! Get register type.
  ASMJIT_INLINE uint32_t getRegType() const {
    return _vreg.type;
  }

  //! Get register index.
  ASMJIT_INLINE uint32_t getRegIndex() const {
    return _vreg.index;
  }

  //! Get whether the operand is register of `type`.
  ASMJIT_INLINE bool isRegType(uint32_t type) const {
    return (_packed[0].u32[0] & IntUtil::pack32_2x8_1x16(0xFF, 0, 0xFF00)) == IntUtil::pack32_2x8_1x16(kOperandTypeReg, 0, (type << 8));
  }

  //! Get whether the operand is register and of `type` and `index`.
  ASMJIT_INLINE bool isRegCode(uint32_t type, uint32_t index) const {
    return (_packed[0].u32[0] & IntUtil::pack32_2x8_1x16(0xFF, 0, 0xFFFF)) == IntUtil::pack32_2x8_1x16(kOperandTypeReg, 0, (type << 8) + index);
  }

  //! Get whether the operand is a register or memory.
  ASMJIT_INLINE bool isRegOrMem() const {
    ASMJIT_ASSERT(kOperandTypeReg == 1);
    ASMJIT_ASSERT(kOperandTypeMem == 3);
    return (static_cast<uint32_t>(_base.op) | 0x2U) == 0x3U;
  }

  //! Get whether the operand is variable or memory.
  ASMJIT_INLINE bool isVarOrMem() const {
    ASMJIT_ASSERT(kOperandTypeVar == 2);
    ASMJIT_ASSERT(kOperandTypeMem == 3);
    return (static_cast<uint32_t>(_base.op) - 2U) <= 1;
  }

  // --------------------------------------------------------------------------
  // [Size]
  // --------------------------------------------------------------------------

  //! Get size of the operand in bytes.
  ASMJIT_INLINE uint32_t getSize() const {
    return _base.size;
  }

  // --------------------------------------------------------------------------
  // [Id]
  // --------------------------------------------------------------------------

  //! Get operand id.
  //!
  //! Operand id's are used internally by `Assembler` and `Compiler`.
  //!
  //! There is no way to change or remove operand id. Unneeded operands can be
  //! simply reassigned by `operator=`.
  ASMJIT_INLINE uint32_t getId() const {
    return _base.id;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    //! Base data.
    BaseOp _base;
    //! Register or variable data.
    VRegOp _vreg;
    //! Memory data.
    VMemOp _vmem;
    //! Immediate data.
    ImmOp _imm;
    //! Label data.
    LabelOp _label;

    //! Packed operand as two 64-bit integers.
    UInt64 _packed[2];
  };
};

// ============================================================================
// [asmjit::OperandUtil]
// ============================================================================

//! Operand utilities.
struct OperandUtil {
  //! Make variable id.
  static ASMJIT_INLINE uint32_t makeVarId(uint32_t id) {
    return id | kOperandIdVar;
  }

  //! Make label id.
  static ASMJIT_INLINE uint32_t makeLabelId(uint32_t id) {
    return id;
  }

  //! Strip variable id bit so it becomes a pure index to `VarData[]` array.
  static ASMJIT_INLINE uint32_t stripVarId(uint32_t id) {
    return id & 0x7FFFFFFFU;
  }

  //! Get whether the id refers to `Var`.
  //!
  //! \note The function will never return `true` if the id is `kInvalidValue`.
  //! The trick is to compare a given id to -1 (kInvalidValue) so we check both
  //! using only one comparison.
  static ASMJIT_INLINE bool isVarId(uint32_t id) {
    return static_cast<int32_t>(id) < -1;
  }

  //! Get whether the id refers to `Label`.
  //!
  //! \note The function will never return `true` if the id is `kInvalidValue`.
  static ASMJIT_INLINE bool isLabelId(uint32_t id) {
    return static_cast<int32_t>(id) >= 0;
  }
};

// ============================================================================
// [asmjit::Reg]
// ============================================================================

//! Base class for all register operands.
struct Reg : public Operand {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy base register.
  ASMJIT_INLINE Reg() : Operand(NoInit) {
    _init_packed_op_sz_w0_id(kOperandTypeReg, 0, (kInvalidReg << 8) + kInvalidReg, kInvalidValue);
    _init_packed_d2_d3(kInvalidVar, 0);
  }

  //! Create a new base register.
  ASMJIT_INLINE Reg(uint32_t type, uint32_t index, uint32_t size) : Operand(NoInit) {
    _init_packed_op_sz_w0_id(kOperandTypeReg, size, (type << 8) + index, kInvalidValue);
    _init_packed_d2_d3(kInvalidVar, 0);
  }

  //! Create a new reference to `other`.
  ASMJIT_INLINE Reg(const Reg& other) : Operand(other) {}

  //! Create a new reference to `other` and change the index to `index`.
  ASMJIT_INLINE Reg(const Reg& other, uint32_t index) : Operand(other) {
    _vreg.index = static_cast<uint8_t>(index);
  }

  explicit ASMJIT_INLINE Reg(const _NoInit&) : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Reg Specific]
  // --------------------------------------------------------------------------

  //! Clone `Reg` operand.
  ASMJIT_INLINE Reg clone() const {
    return Reg(*this);
  }

  //! Get whether register code is equal to `type`.
  ASMJIT_INLINE bool isRegType(uint32_t type) const {
    return _vreg.type == type;
  }

  //! Get whether register code is equal to `type`.
  ASMJIT_INLINE bool isRegCode(uint32_t code) const {
    return _vreg.code == code;
  }

  //! Get whether register code is equal to `type`.
  ASMJIT_INLINE bool isRegCode(uint32_t type, uint32_t index) const {
    return _vreg.code == (type << 8) + index;
  }

  //! Get register code that equals to '(type << 8) + index'.
  ASMJIT_INLINE uint32_t getRegCode() const {
    return _vreg.code;
  }

  //! Get register type.
  ASMJIT_INLINE uint32_t getRegType() const {
    return _vreg.type;
  }

  //! Get register index.
  ASMJIT_INLINE uint32_t getRegIndex() const {
    return _vreg.index;
  }

#define ASMJIT_REG_OP(_Type_) \
  ASMJIT_INLINE _Type_ clone() const { \
    return _Type_(*this); \
  } \
  \
  /*! Set register `size`. */ \
  ASMJIT_INLINE _Type_& setSize(uint32_t size) { \
    _vreg.size = static_cast<uint8_t>(size); \
    return *this; \
  } \
  \
  /*! Set register `code`. */ \
  ASMJIT_INLINE _Type_& setCode(uint32_t code) { \
    _vreg.code = static_cast<uint16_t>(code); \
    return *this; \
  } \
  \
  /*! Set register `type` and `index`. */ \
  ASMJIT_INLINE _Type_& setCode(uint32_t type, uint32_t index) { \
    _vreg.type = static_cast<uint8_t>(type); \
    _vreg.index = static_cast<uint8_t>(index); \
    return *this; \
  } \
  \
  /*! Set register `type`. */ \
  ASMJIT_INLINE _Type_& setType(uint32_t type) { \
    _vreg.type = static_cast<uint8_t>(type); \
    return *this; \
  } \
  \
  /*! Set register `index`. */ \
  ASMJIT_INLINE _Type_& setIndex(uint32_t index) { \
    _vreg.index = static_cast<uint8_t>(index); \
    return *this; \
  } \
  \
  ASMJIT_INLINE _Type_& operator=(const _Type_& other) { _copy(other); return *this; } \
  \
  ASMJIT_INLINE bool operator==(const _Type_& other) const { return _packed[0].u32[0] == other._packed[0].u32[0]; } \
  ASMJIT_INLINE bool operator!=(const _Type_& other) const { return !operator==(other); }
};

// ============================================================================
// [asmjit::BaseMem]
// ============================================================================

//! Base class for all memory operands.
struct BaseMem : public Operand {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE BaseMem() : Operand(NoInit) {
    reset();
  }

  ASMJIT_INLINE BaseMem(const BaseMem& other) : Operand(other) {}
  explicit ASMJIT_INLINE BaseMem(const _NoInit&) : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [BaseMem Specific]
  // --------------------------------------------------------------------------

  //! Clone `BaseMem` operand.
  ASMJIT_INLINE BaseMem clone() const {
    return BaseMem(*this);
  }

  //! Reset `BaseMem` operand.
  ASMJIT_INLINE void reset() {
    _init_packed_op_sz_b0_b1_id(kOperandTypeMem, 0, kMemTypeBaseIndex, 0, kInvalidValue);
    _init_packed_d2_d3(kInvalidValue, 0);
  }

  //! Get the type of the memory operand, see `kMemType`.
  ASMJIT_INLINE uint32_t getMemType() const {
    return _vmem.type;
  }

  //! Get whether the type of the memory operand is either `kMemTypeBaseIndex`
  //! or `kMemTypeStackIndex`.
  ASMJIT_INLINE bool isBaseIndexType() const {
    return _vmem.type <= kMemTypeStackIndex;
  }

  //! Get whether the memory operand has base register.
  ASMJIT_INLINE bool hasBase() const {
    return _vmem.base != kInvalidValue;
  }

  //! Get memory operand base id, or `kInvalidValue`.
  ASMJIT_INLINE uint32_t getBase() const {
    return _vmem.base;
  }

  //! Set memory operand size.
  ASMJIT_INLINE BaseMem& setSize(uint32_t size) {
    _vmem.size = static_cast<uint8_t>(size);
    return *this;
  }

  //! Get memory operand relative displacement.
  ASMJIT_INLINE int32_t getDisplacement() const {
    return _vmem.displacement;
  }

  //! Set memory operand relative displacement.
  ASMJIT_INLINE BaseMem& setDisplacement(int32_t disp) {
    _vmem.displacement = disp;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE BaseMem& operator=(const BaseMem& other) {
    _copy(other);
    return *this;
  }

  ASMJIT_INLINE bool operator==(const BaseMem& other) const {
    return (_packed[0] == other._packed[0]) & (_packed[1] == other._packed[1]);
  }

  ASMJIT_INLINE bool operator!=(const BaseMem& other) const {
    return !(*this == other);
  }
};

// ============================================================================
// [asmjit::Imm]
// ============================================================================

//! Immediate operand.
//!
//! Immediate operand is usually part of instruction itself. It's inlined after
//! or before the instruction opcode. Immediates can be only signed or unsigned
//! integers.
//!
//! To create immediate operand use `imm()` or `imm_u()` non-members or `Imm`
//! constructors.
struct Imm : public Operand {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new immediate value (initial value is 0).
  Imm() : Operand(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeImm, 0, 0, 0, kInvalidValue);
    _imm.value._i64[0] = 0;
  }

  //! Create a new signed immediate value, assigning the value to `val`.
  explicit Imm(int64_t val) : Operand(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeImm, 0, 0, 0, kInvalidValue);
    _imm.value._i64[0] = val;
  }

  //! Create a new immediate value from `other`.
  ASMJIT_INLINE Imm(const Imm& other) : Operand(other) {}

  explicit ASMJIT_INLINE Imm(const _NoInit&) : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Immediate Specific]
  // --------------------------------------------------------------------------

  //! Clone `Imm` operand.
  ASMJIT_INLINE Imm clone() const {
    return Imm(*this);
  }

  //! Get whether the immediate can be casted to 8-bit signed integer.
  ASMJIT_INLINE bool isInt8() const { return IntUtil::isInt8(_imm.value._i64[0]); }
  //! Get whether the immediate can be casted to 8-bit unsigned integer.
  ASMJIT_INLINE bool isUInt8() const { return IntUtil::isUInt8(_imm.value._i64[0]); }

  //! Get whether the immediate can be casted to 16-bit signed integer.
  ASMJIT_INLINE bool isInt16() const { return IntUtil::isInt16(_imm.value._i64[0]); }
  //! Get whether the immediate can be casted to 16-bit unsigned integer.
  ASMJIT_INLINE bool isUInt16() const { return IntUtil::isUInt16(_imm.value._i64[0]); }

  //! Get whether the immediate can be casted to 32-bit signed integer.
  ASMJIT_INLINE bool isInt32() const { return IntUtil::isInt32(_imm.value._i64[0]); }
  //! Get whether the immediate can be casted to 32-bit unsigned integer.
  ASMJIT_INLINE bool isUInt32() const { return IntUtil::isUInt32(_imm.value._i64[0]); }

  //! Get immediate value as 8-bit signed integer.
  ASMJIT_INLINE int8_t getInt8() const { return _imm.value._i8[_ASMJIT_HOST_INDEX(8, 0)]; }
  //! Get immediate value as 8-bit unsigned integer.
  ASMJIT_INLINE uint8_t getUInt8() const { return _imm.value._u8[_ASMJIT_HOST_INDEX(8, 0)]; }
  //! Get immediate value as 16-bit signed integer.
  ASMJIT_INLINE int16_t getInt16() const { return _imm.value._i16[_ASMJIT_HOST_INDEX(4, 0)]; }
  //! Get immediate value as 16-bit unsigned integer.
  ASMJIT_INLINE uint16_t getUInt16() const { return _imm.value._u16[_ASMJIT_HOST_INDEX(4, 0)]; }
  //! Get immediate value as 32-bit signed integer.
  ASMJIT_INLINE int32_t getInt32() const { return _imm.value._i32[_ASMJIT_HOST_INDEX(2, 0)]; }
  //! Get immediate value as 32-bit unsigned integer.
  ASMJIT_INLINE uint32_t getUInt32() const { return _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)]; }
  //! Get immediate value as 64-bit signed integer.
  ASMJIT_INLINE int64_t getInt64() const { return _imm.value._i64[0]; }
  //! Get immediate value as 64-bit unsigned integer.
  ASMJIT_INLINE uint64_t getUInt64() const { return _imm.value._u64[0]; }

  //! Get immediate value as `intptr_t`.
  ASMJIT_INLINE intptr_t getIntPtr() const {
    if (sizeof(intptr_t) == sizeof(int64_t))
      return static_cast<intptr_t>(getInt64());
    else
      return static_cast<intptr_t>(getInt32());
  }

  //! Get immediate value as `uintptr_t`.
  ASMJIT_INLINE uintptr_t getUIntPtr() const {
    if (sizeof(uintptr_t) == sizeof(uint64_t))
      return static_cast<uintptr_t>(getUInt64());
    else
      return static_cast<uintptr_t>(getUInt32());
  }

  //! Get low 32-bit signed integer.
  ASMJIT_INLINE int32_t getInt32Lo() const { return _imm.value._i32[_ASMJIT_HOST_INDEX(2, 0)]; }
  //! Get low 32-bit signed integer.
  ASMJIT_INLINE uint32_t getUInt32Lo() const { return _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)]; }
  //! Get high 32-bit signed integer.
  ASMJIT_INLINE int32_t getInt32Hi() const { return _imm.value._i32[_ASMJIT_HOST_INDEX(2, 1)]; }
  //! Get high 32-bit signed integer.
  ASMJIT_INLINE uint32_t getUInt32Hi() const { return _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)]; }

  //! Set immediate value to 8-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt8(int8_t val) {
    if (kArchHost64Bit) {
      _imm.value._i64[0] = static_cast<int64_t>(val);
    }
    else {
      int32_t val32 = static_cast<int32_t>(val);
      _imm.value._i32[_ASMJIT_HOST_INDEX(2, 0)] = val32;
      _imm.value._i32[_ASMJIT_HOST_INDEX(2, 1)] = val32 >> 31;
    }
    return *this;
  }

  //! Set immediate value to 8-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt8(uint8_t val) {
    if (kArchHost64Bit) {
      _imm.value._u64[0] = static_cast<uint64_t>(val);
    }
    else {
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)] = static_cast<uint32_t>(val);
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  //! Set immediate value to 16-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt16(int16_t val) {
    if (kArchHost64Bit) {
      _imm.value._i64[0] = static_cast<int64_t>(val);
    }
    else {
      int32_t val32 = static_cast<int32_t>(val);
      _imm.value._i32[_ASMJIT_HOST_INDEX(2, 0)] = val32;
      _imm.value._i32[_ASMJIT_HOST_INDEX(2, 1)] = val32 >> 31;
    }
    return *this;
  }

  //! Set immediate value to 16-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt16(uint16_t val) {
    if (kArchHost64Bit) {
      _imm.value._u64[0] = static_cast<uint64_t>(val);
    }
    else {
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)] = static_cast<uint32_t>(val);
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  //! Set immediate value to 32-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt32(int32_t val) {
    if (kArchHost64Bit) {
      _imm.value._i64[0] = static_cast<int64_t>(val);
    }
    else {
      _imm.value._i32[_ASMJIT_HOST_INDEX(2, 0)] = val;
      _imm.value._i32[_ASMJIT_HOST_INDEX(2, 1)] = val >> 31;
    }
    return *this;
  }

  //! Set immediate value to 32-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt32(uint32_t val) {
    if (kArchHost64Bit) {
      _imm.value._u64[0] = static_cast<uint64_t>(val);
    }
    else {
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)] = val;
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  //! Set immediate value to 64-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt64(int64_t val) {
    _imm.value._i64[0] = val;
    return *this;
  }

  //! Set immediate value to 64-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt64(uint64_t val) {
    _imm.value._u64[0] = val;
    return *this;
  }

  //! Set immediate value to intptr_t `val`.
  ASMJIT_INLINE Imm& setIntPtr(intptr_t val) {
    _imm.value._i64[0] = static_cast<int64_t>(val);
    return *this;
  }

  //! Set immediate value to uintptr_t `val`.
  ASMJIT_INLINE Imm& setUIntPtr(uintptr_t val) {
    _imm.value._u64[0] = static_cast<uint64_t>(val);
    return *this;
  }

  //! Set immediate value as unsigned type to `val`.
  ASMJIT_INLINE Imm& setPtr(void* p) { return setIntPtr((intptr_t)p); }

  // --------------------------------------------------------------------------
  // [Float]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Imm& setFloat(float f) {
    _imm.value._f32[_ASMJIT_HOST_INDEX(2, 0)] = f;
    _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    return *this;
  }

  ASMJIT_INLINE Imm& setDouble(double d) {
    _imm.value._f64[0] = d;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Truncate]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Imm& truncateTo8Bits() {
    if (kArchHost64Bit) {
      _imm.value._u64[0] &= static_cast<uint64_t>(0x000000FFU);
    }
    else {
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)] &= 0x000000FFU;
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  ASMJIT_INLINE Imm& truncateTo16Bits() {
    if (kArchHost64Bit) {
      _imm.value._u64[0] &= static_cast<uint64_t>(0x0000FFFFU);
    }
    else {
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 0)] &= 0x0000FFFFU;
      _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  ASMJIT_INLINE Imm& truncateTo32Bits() {
    _imm.value._u32[_ASMJIT_HOST_INDEX(2, 1)] = 0;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  //! Assign `other` to the immediate operand.
  ASMJIT_INLINE Imm& operator=(const Imm& other) {
    _copy(other);
    return *this;
  }
};

// ============================================================================
// [asmjit::Label]
// ============================================================================

//! Label (jump target or data location).
//!
//! Label represents a location in code typically used as jump targets, but may
//! be also reference data or static variables. Label has to be explicitly
//! created by a code-generator by calling `CodeGen::newLabel()` where `CodeGen`
//! is your code generator, which derives from `Assembler` or `Compiler`.
//!
//! Example of using labels:
//!
//! ~~~
//! // Create Assembler/Compiler.
//! host::Assembler a;
//!
//! // Create Label instance.
//! Label L_1(a);
//!
//! // ... your code ...
//!
//! // Using label.
//! a.jump(L_1);
//!
//! // ... your code ...
//!
//! // Bind label to the current position, see `CodeGen::bind()`.
//! a.bind(L_1);
//! ~~~
struct Label : public Operand {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create new, unassociated label.
  ASMJIT_INLINE Label() : Operand(NoInit) {
    reset();
  }

  explicit ASMJIT_INLINE Label(uint32_t id) : Operand(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeLabel, 0, 0, 0, id);
    _init_packed_d2_d3(0, 0);
  }

  //! Create new initialized label.
  explicit ASMJIT_INLINE Label(Assembler& a);
  //! Create new initialized label.
  explicit ASMJIT_INLINE Label(Compiler& c);

  //! Create reference to another label.
  ASMJIT_INLINE Label(const Label& other) : Operand(other) {}

  explicit ASMJIT_INLINE Label(const _NoInit&) : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() {
    _init_packed_op_sz_b0_b1_id(kOperandTypeLabel, 0, 0, 0, kInvalidValue);
    _init_packed_d2_d3(0, 0);
  }

  // --------------------------------------------------------------------------
  // [Label Specific]
  // --------------------------------------------------------------------------

  //! Get whether the label has been initialized by `Assembler` or `Compiler`.
  ASMJIT_INLINE bool isInitialized() const {
    return _label.id != kInvalidValue;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Label& operator=(const Label& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const Label& other) const { return _base.id == other._base.id; }
  ASMJIT_INLINE bool operator!=(const Label& other) const { return _base.id != other._base.id; }
};

// ============================================================================
// [asmjit::Operand - Globals]
// ============================================================================

//! No operand, can be used to reset an operand by assignment or to refer to an
//! operand that doesn't exist.
ASMJIT_VAR const Operand noOperand;

//! Create signed immediate value operand.
static ASMJIT_INLINE Imm imm(int64_t val) {
  return Imm(val);
}

//! Create unsigned immediate value operand.
static ASMJIT_INLINE Imm imm_u(uint64_t val) {
  return Imm(static_cast<int64_t>(val));
}

//! Create void* pointer immediate value operand.
static ASMJIT_INLINE Imm imm_ptr(void* p) {
  return Imm(static_cast<int64_t>((intptr_t)p));
}

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_OPERAND_H
