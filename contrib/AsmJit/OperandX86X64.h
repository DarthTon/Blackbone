// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_OPERANDX86X64_H
#define _ASMJIT_OPERANDX86X64_H

#if !defined(_ASMJIT_OPERAND_H)
#warning "AsmJit/OperandX86X64.h can be only included by AsmJit/Operand.h"
#endif // _ASMJIT_OPERAND_H

// [Dependencies]
#include "Build.h"
#include "Defs.h"

namespace AsmJit {

#pragma warning(disable : 4245 4127 4244)


// ============================================================================
// [AsmJit::Forward Declarations]
// ============================================================================

struct BaseReg;
struct BaseVar;
struct Compiler;
struct GPReg;
struct GPVar;
struct Imm;
struct Label;
struct Mem;
struct MMReg;
struct MMVar;
struct Operand;
struct X87Reg;
struct X87Var;
struct XMMReg;
struct XMMVar;

//! @addtogroup AsmJit_Core
//! @{

// ============================================================================
// [AsmJit::Operand]
// ============================================================================

//! @brief Operand, abstract class for register, memory location and immediate
//! value operands.
struct ASMJIT_HIDDEN Operand
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create an uninitialized operand.
  inline Operand() ASMJIT_NOTHROW
  {
    memset(this, 0, sizeof(Operand));
    _base.id = INVALID_VALUE;
  }

  //! @brief Create a reference to @a other operand.
  inline Operand(const Operand& other) ASMJIT_NOTHROW
  {
    _init(other);
  }

#if !defined(ASMJIT_NODOC)
  inline Operand(const _DontInitialize&) ASMJIT_NOTHROW
  {
  }
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Init & Copy]
  // --------------------------------------------------------------------------

  //! @internal
  //!
  //! @brief Initialize operand to @a other (used by constructors).
  inline void _init(const Operand& other) ASMJIT_NOTHROW
  {
    memcpy(this, &other, sizeof(Operand));
  }

  //! @internal
  //!
  //! @brief Initialize operand to @a other (used by assign operators).
  inline void _copy(const Operand& other) ASMJIT_NOTHROW
  {
    memcpy(this, &other, sizeof(Operand));
  }

  // --------------------------------------------------------------------------
  // [Identification]
  // --------------------------------------------------------------------------

  //! @brief Get type of operand, see @c OPERAND_TYPE.
  inline uint32_t getType() const ASMJIT_NOTHROW
  { return _base.op; }

  //! @brief Get whether operand is none (@c OPERAND_NONE).
  inline bool isNone() const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_NONE); }

  //! @brief Get whether operand is any (general purpose, mmx or sse) register (@c OPERAND_REG).
  inline bool isReg() const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_REG); }

  //! @brief Get whether operand is memory address (@c OPERAND_MEM).
  inline bool isMem() const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_MEM); }

  //! @brief Get whether operand is immediate (@c OPERAND_IMM).
  inline bool isImm() const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_IMM); }

  //! @brief Get whether operand is label (@c OPERAND_LABEL).
  inline bool isLabel() const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_LABEL); }

  //! @brief Get whether operand is variable (@c OPERAND_VAR).
  inline bool isVar() const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_VAR); }

  //! @brief Get whether operand is variable or memory.
  inline bool isVarMem() const ASMJIT_NOTHROW
  { return ((_base.op & (OPERAND_VAR | OPERAND_MEM)) != 0); }

  //! @brief Get whether operand is register and type of register is @a regType.
  inline bool isRegType(uint32_t regType) const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_REG) & ((_reg.code & REG_TYPE_MASK) == regType); }

  //! @brief Get whether operand is register and code of register is @a regCode.
  inline bool isRegCode(uint32_t regCode) const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_REG) & (_reg.code == regCode); }

  //! @brief Get whether operand is register and index of register is @a regIndex.
  inline bool isRegIndex(uint32_t regIndex) const ASMJIT_NOTHROW
  { return (_base.op == OPERAND_REG) & ((_reg.code & REG_INDEX_MASK) == (regIndex & REG_INDEX_MASK)); }

  //! @brief Get whether operand is any register or memory.
  inline bool isRegMem() const ASMJIT_NOTHROW
  { return ((_base.op & (OPERAND_REG | OPERAND_MEM)) != 0); }

  //! @brief Get whether operand is register of @a regType type or memory.
  inline bool isRegTypeMem(uint32_t regType) const ASMJIT_NOTHROW
  { return ((_base.op == OPERAND_REG) & ((_reg.code & REG_TYPE_MASK) == regType)) | (_base.op == OPERAND_MEM); }

  // --------------------------------------------------------------------------
  // [Operand Size]
  // --------------------------------------------------------------------------

  //! @brief Return size of operand in bytes.
  inline uint32_t getSize() const ASMJIT_NOTHROW
  { return _base.size; }

  // --------------------------------------------------------------------------
  // [Operand Id]
  // --------------------------------------------------------------------------

  //! @brief Return operand Id (Operand Id's are used internally by 
  //! @c Assembler and @c Compiler classes).
  //!
  //! @note There is no way how to change or remove operand id. If you don't
  //! need the operand just assign different operand to this one.
  inline uint32_t getId() const ASMJIT_NOTHROW
  { return _base.id; }

  // --------------------------------------------------------------------------
  // [Extensions]
  // --------------------------------------------------------------------------

  //! @brief Get whether the extended register (additional eight registers
  //! introduced by 64-bit mode) is used.
  inline bool isExtendedRegisterUsed() const ASMJIT_NOTHROW
  {
    // Hacky, but correct.
    // - If operand type is register then extended register is register with
    //   index 8 and greater (8 to 15 inclusive).
    // - If operand type is memory operand then we need to take care about
    //   label (in _mem.base) and INVALID_VALUE, we just decrement the value
    //   by 8 and check if it's at interval 0 to 7 inclusive (if it's there
    //   then it's extended register.
    return (isReg() && (_reg.code & REG_INDEX_MASK)  >= 8) ||
           (isMem() && ((((uint32_t)_mem.base  - 8U) < 8U) ||
                        (((uint32_t)_mem.index - 8U) < 8U) ));
  }

  // --------------------------------------------------------------------------
  // [Data Structures]
  // --------------------------------------------------------------------------

  //! @internal
  //! 
  //! @brief Base operand data shared between all operands.
  struct BaseData
  {
    //! @brief Type of operand, see @c OPERAND_TYPE.
    uint8_t op;
    //! @brief Size of operand (register, address, immediate or variable).
    uint8_t size;

    //! @brief Not used.
    uint8_t reserved[2];

    //! @brief Operand id (private variable for @c Assembler and @c Compiler classes).
    //!
    //! @note Uninitialized operands has id equal to zero.
    uint32_t id;
  };

  //! @internal
  //! 
  //! @brief Register data.
  struct RegData
  {
    //! @brief Type of operand, see @c OPERAND_TYPE (in this case @c OPERAND_REG).
    uint8_t op;
    //! @brief Size of register.
    uint8_t size;

    //! @brief Not used.
    uint8_t reserved[2];

    //! @brief Operand id.
    uint32_t id;

    //! @brief Register code or variable, see @c REG and @c INVALID_VALUE.
    uint32_t code;
  };

  //! @internal
  //! 
  //! @brief Memory address data.
  struct MemData
  {
    //! @brief Type of operand, see @c OPERAND_TYPE (in this case @c OPERAND_MEM).
    uint8_t op;
    //! @brief Size of pointer.
    uint8_t size;

    //! @brief Memory operand type, see @c OPERAND_MEM_TYPE.
    uint8_t type;
    //! @brief Segment override prefix, see @c SEGMENT_PREFIX.
    uint8_t segmentPrefix : 4;
    //! @brief Emit MOV/LEA instruction using 16-bit form of base/index registers.
    uint8_t sizePrefix : 1;
    //! @brief Index register shift/scale (0 to 3 inclusive, see @c SCALE).
    uint8_t shift : 3;

    //! @brief Operand ID.
    uint32_t id;

    //! @brief Base register index, variable or label id.
    uint32_t base;
    //! @brief Index register index or variable id.
    uint32_t index;

    //! @brief Target (for 32-bit, absolute address).
    void* target;

    //! @brief Displacement.
    sysint_t displacement;
  };

  //! @internal
  //! 
  //! @brief Immediate value data.
  struct ImmData
  {
    //! @brief Type of operand, see @c OPERAND_TYPE (in this case @c OPERAND_IMM)..
    uint8_t op;
    //! @brief Size of immediate (or 0 to autodetect).
    uint8_t size;

    //! @brief @c true if immediate is unsigned.
    uint8_t isUnsigned;
    //! @brief Not used.
    uint8_t reserved;

    //! @brief Operand ID.
    uint32_t id;

    //! @brief Immediate value.
    sysint_t value;
  };

  //! @internal
  //! 
  //! @brief Label data.
  struct LblData
  {
    //! @brief Type of operand, see @c OPERAND_TYPE (in this case @c OPERAND_LABEL).
    uint8_t op;
    //! @brief Size of label, currently not used.
    uint8_t size;

    //! @brief Not used.
    uint8_t reserved[2];

    //! @brief Operand ID.
    uint32_t id;
  };

  //! @internal
  //! 
  //! @brief Variable data.
  struct VarData
  {
    //! @brief Type of operand, see @c OPERAND_TYPE (in this case @c OPERAND_VAR).
    uint8_t op;
    //! @brief Size of variable (0 if don't known).
    uint8_t size;

    //! @brief Not used.
    uint8_t reserved[2];

    //! @brief Operand ID.
    uint32_t id;

    //! @brief Type (and later also code) of register, see @c REG_TYPE, @c REG_CODE.
    //!
    //! @note Register code and variable code are two different things. In most
    //! cases registerCode is very related to variableType, but general purpose
    //! registers are divided to 64-bit, 32-bit, 16-bit and 8-bit entities so
    //! the registerCode can be used to access these, variableType remains
    //! unchanged from the initialization state. Variable type describes mainly
    //! variable type and home memory size.
    uint32_t registerCode;

    //! @brief Type of variable. See @c VARIABLE_TYPE enum.
    uint32_t variableType;
  };
  
  //! @brief Binary data.
  struct BinData
  {
    //! @brief First four 32-bit integers.
    uint32_t u32[4];
    //! @brief Second two 32 or 64-bit integers.
    sysuint_t s[2];
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union
  {
    //! @brief Generic operand data.
    BaseData _base;
    //! @brief Register operand data.
    RegData _reg;
    //! @brief Memory operand data.
    MemData _mem;
    //! @brief Immediate operand data.
    ImmData _imm;
    //! @brief Label data.
    LblData _lbl;
    //! @brief Variable data.
    VarData _var;
    //! @brief 
    BinData _bin;
  };
};

// ============================================================================
// [AsmJit::BaseReg]
// ============================================================================

//! @brief Base class for all registers.
struct ASMJIT_HIDDEN BaseReg : public Operand
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new base register.
  inline BaseReg(uint32_t code, uint32_t size) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _reg.op = OPERAND_REG;
    _reg.size = (uint8_t)size;
    _reg.id = INVALID_VALUE;
    _reg.code = code;
  }

  //! @brief Create a new reference to @a other.
  inline BaseReg(const BaseReg& other) ASMJIT_NOTHROW :
    Operand(other)
  {}

#if !defined(ASMJIT_NODOC)
  inline BaseReg(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    Operand(dontInitialize)
  {}
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [BaseReg Specific]
  // --------------------------------------------------------------------------

  //! @brief Get register code, see @c REG.
  inline uint32_t getRegCode() const ASMJIT_NOTHROW
  { return (uint32_t)(_reg.code); }

  //! @brief Get register type, see @c REG.
  inline uint32_t getRegType() const ASMJIT_NOTHROW
  { return (uint32_t)(_reg.code & REG_TYPE_MASK); }

  //! @brief Get register index (value from 0 to 7/15).
  inline uint32_t getRegIndex() const ASMJIT_NOTHROW
  { return (uint32_t)(_reg.code & REG_INDEX_MASK); }

  //! @brief Get whether register code is equal to @a code.
  inline bool isRegCode(uint32_t code) const ASMJIT_NOTHROW
  { return _reg.code == code; }

  //! @brief Get whether register code is equal to @a type.
  inline bool isRegType(uint32_t type) const ASMJIT_NOTHROW
  { return (uint32_t)(_reg.code & REG_TYPE_MASK) == type; }

  //! @brief Get whether register index is equal to @a index.
  inline bool isRegIndex(uint32_t index) const ASMJIT_NOTHROW
  { return (uint32_t)(_reg.code & REG_INDEX_MASK) == index; }

  //! @brief Set register code to @a code.
  inline void setCode(uint32_t code) ASMJIT_NOTHROW
  { _reg.code = code; }

  //! @brief Set register size to @a size.
  inline void setSize(uint32_t size) ASMJIT_NOTHROW
  { _reg.size = (uint8_t)size; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  inline BaseReg& operator=(const BaseReg& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }

  inline bool operator==(const BaseReg& other) const ASMJIT_NOTHROW
  { return getRegCode() == other.getRegCode(); }

  inline bool operator!=(const BaseReg& other) const ASMJIT_NOTHROW
  { return getRegCode() != other.getRegCode(); }
};

// ============================================================================
// [AsmJit::Reg]
// ============================================================================

//! @brief General purpose register.
//!
//! This class is for all general purpose registers (64, 32, 16 and 8-bit).
struct ASMJIT_HIDDEN GPReg : public BaseReg
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create non-initialized general purpose register.
  inline GPReg() ASMJIT_NOTHROW :
    BaseReg(INVALID_VALUE, 0) {}

  //! @brief Create a reference to @a other general purpose register.
  inline GPReg(const GPReg& other) ASMJIT_NOTHROW :
    BaseReg(other) {}

#if !defined(ASMJIT_NODOC)
  inline GPReg(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseReg(dontInitialize) {}

  inline GPReg(const _Initialize&, uint32_t code) ASMJIT_NOTHROW :
    BaseReg(code, static_cast<uint32_t>(1U << ((code & REG_TYPE_MASK) >> 12))) {}
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [GPReg Specific]
  // --------------------------------------------------------------------------

  //! @brief Get whether the general purpose register is BYTE (8-bit) type.
  inline bool isGPB() const ASMJIT_NOTHROW { return (_reg.code & REG_TYPE_MASK) <= REG_TYPE_GPB_HI; }
  //! @brief Get whether the general purpose register is LO-BYTE (8-bit) type.
  inline bool isGPBLo() const ASMJIT_NOTHROW { return (_reg.code & REG_TYPE_MASK) == REG_TYPE_GPB_LO; }
  //! @brief Get whether the general purpose register is HI-BYTE (8-bit) type.
  inline bool isGPBHi() const ASMJIT_NOTHROW { return (_reg.code & REG_TYPE_MASK) == REG_TYPE_GPB_HI; }

  //! @brief Get whether the general purpose register is WORD (16-bit) type.
  inline bool isGPW() const ASMJIT_NOTHROW { return (_reg.code & REG_TYPE_MASK) == REG_TYPE_GPW; }
  //! @brief Get whether the general purpose register is DWORD (32-bit) type.
  //!
  //! This is default type for 32-bit platforms.
  inline bool isGPD() const ASMJIT_NOTHROW { return (_reg.code & REG_TYPE_MASK) == REG_TYPE_GPD; }
  //! @brief Get whether the general purpose register is QWORD (64-bit) type.
  //!
  //! This is default type for 64-bit platforms.
  inline bool isGPQ() const ASMJIT_NOTHROW { return (_reg.code & REG_TYPE_MASK) == REG_TYPE_GPQ; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline GPReg& operator=(const GPReg& other) ASMJIT_NOTHROW { _copy(other); return *this; }
  inline bool operator==(const GPReg& other) const ASMJIT_NOTHROW { return getRegCode() == other.getRegCode(); }
  inline bool operator!=(const GPReg& other) const ASMJIT_NOTHROW { return getRegCode() != other.getRegCode(); }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::X87Register]
// ============================================================================

//! @brief 80-bit x87 floating point register.
//!
//! To create instance of x87 register, use @c st() function.
struct ASMJIT_HIDDEN X87Reg : public BaseReg
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create non-initialized x87 register.
  inline X87Reg() ASMJIT_NOTHROW :
    BaseReg(INVALID_VALUE, 10) {}

  //! @brief Create a reference to @a other x87 register.
  inline X87Reg(const X87Reg& other) ASMJIT_NOTHROW :
    BaseReg(other) {}

#if !defined(ASMJIT_NODOC)
  inline X87Reg(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseReg(dontInitialize) {}

  inline X87Reg(const _Initialize&, uint32_t code) ASMJIT_NOTHROW :
    BaseReg(code | REG_TYPE_X87, 10) {}
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline X87Reg& operator=(const X87Reg& other) ASMJIT_NOTHROW { _copy(other); return *this; }
  inline bool operator==(const X87Reg& other) const ASMJIT_NOTHROW { return getRegCode() == other.getRegCode(); }
  inline bool operator!=(const X87Reg& other) const ASMJIT_NOTHROW { return getRegCode() != other.getRegCode(); }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::MMReg]
// ============================================================================

//! @brief 64-bit MMX register.
struct ASMJIT_HIDDEN MMReg : public BaseReg
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create non-initialized MM register.
  inline MMReg() ASMJIT_NOTHROW :
    BaseReg(INVALID_VALUE, 8) {}

  //! @brief Create a reference to @a other MM register.
  inline MMReg(const MMReg& other) ASMJIT_NOTHROW :
    BaseReg(other) {}

#if !defined(ASMJIT_NODOC)
  inline MMReg(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseReg(dontInitialize) {}

  inline MMReg(const _Initialize&, uint32_t code) ASMJIT_NOTHROW :
    BaseReg(code, 8) {}
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline MMReg& operator=(const MMReg& other) ASMJIT_NOTHROW { _copy(other); return *this; }
  inline bool operator==(const MMReg& other) const ASMJIT_NOTHROW { return getRegCode() == other.getRegCode(); }
  inline bool operator!=(const MMReg& other) const ASMJIT_NOTHROW { return getRegCode() != other.getRegCode(); }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::XMMReg]
// ============================================================================

//! @brief 128-bit SSE register.
struct ASMJIT_HIDDEN XMMReg : public BaseReg
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create non-initialized XMM register.
  inline XMMReg() ASMJIT_NOTHROW :
    BaseReg(INVALID_VALUE, 16) {}

  //! @brief Create a reference to @a other XMM register.
  inline XMMReg(const _Initialize&, uint32_t code) ASMJIT_NOTHROW :
    BaseReg(code, 16) {}

#if !defined(ASMJIT_NODOC)
  inline XMMReg(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseReg(dontInitialize) {}

  inline XMMReg(const XMMReg& other) ASMJIT_NOTHROW :
    BaseReg(other) {}
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline XMMReg& operator=(const XMMReg& other) ASMJIT_NOTHROW { _copy(other); return *this; }
  inline bool operator==(const XMMReg& other) const ASMJIT_NOTHROW { return getRegCode() == other.getRegCode(); }
  inline bool operator!=(const XMMReg& other) const ASMJIT_NOTHROW { return getRegCode() != other.getRegCode(); }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::SegmentReg]
// ============================================================================

//! @brief Segment register.
struct ASMJIT_HIDDEN SegmentReg : public BaseReg
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create non-initialized segment register.
  inline SegmentReg() ASMJIT_NOTHROW :
    BaseReg(INVALID_VALUE, 2) {}

  //! @brief Create a reference to @a other segment register.
  inline SegmentReg(const _Initialize&, uint32_t code) ASMJIT_NOTHROW :
    BaseReg(code, 2) {}

#if !defined(ASMJIT_NODOC)
  inline SegmentReg(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseReg(dontInitialize) {}

  inline SegmentReg(const SegmentReg& other) ASMJIT_NOTHROW :
    BaseReg(other) {}
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline SegmentReg& operator=(const SegmentReg& other) ASMJIT_NOTHROW { _copy(other); return *this; }
  inline bool operator==(const SegmentReg& other) const ASMJIT_NOTHROW { return getRegCode() == other.getRegCode(); }
  inline bool operator!=(const SegmentReg& other) const ASMJIT_NOTHROW { return getRegCode() != other.getRegCode(); }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::Registers - no_reg]
// ============================================================================

//! @brief No register, can be used only in @c Mem operand.
ASMJIT_VAR const GPReg no_reg;

// ============================================================================
// [AsmJit::Registers - 8-bit]
// ============================================================================

//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg al;
//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg cl;
//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg dl;
//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg bl;

#if defined(ASMJIT_X64)
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg spl;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg bpl;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg sil;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg dil;

//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r8b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r9b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r10b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r11b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r12b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r13b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r14b;
//! @brief 8-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r15b;
#endif // ASMJIT_X64

//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg ah;
//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg ch;
//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg dh;
//! @brief 8-bit General purpose register.
ASMJIT_VAR const GPReg bh;

// ============================================================================
// [AsmJit::Registers - 16-bit]
// ============================================================================

//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg ax;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg cx;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg dx;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg bx;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg sp;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg bp;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg si;
//! @brief 16-bit General purpose register.
ASMJIT_VAR const GPReg di;

#if defined(ASMJIT_X64)
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r8w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r9w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r10w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r11w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r12w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r13w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r14w;
//! @brief 16-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r15w;
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - 32-bit]
// ============================================================================

//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg eax;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg ecx;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg edx;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg ebx;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg esp;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg ebp;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg esi;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg edi;

#if defined(ASMJIT_X64)
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r8d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r9d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r10d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r11d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r12d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r13d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r14d;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg r15d;
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - 64-bit]
// ============================================================================

#if defined(ASMJIT_X64)
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rax;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rcx;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rdx;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rbx;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rsp;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rbp;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rsi;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg rdi;

//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r8;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r9;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r10;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r11;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r12;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r13;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r14;
//! @brief 64-bit General purpose register (64-bit mode only).
ASMJIT_VAR const GPReg r15;
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - Native (AsmJit extension)]
// ============================================================================

//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg nax;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg ncx;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg ndx;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg nbx;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg nsp;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg nbp;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg nsi;
//! @brief 32-bit General purpose register.
ASMJIT_VAR const GPReg ndi;

// ============================================================================
// [AsmJit::Registers - MM]
// ============================================================================

//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm0;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm1;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm2;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm3;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm4;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm5;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm6;
//! @brief 64-bit MM register.
ASMJIT_VAR const MMReg mm7;

// ============================================================================
// [AsmJit::Registers - XMM]
// ============================================================================

//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm0;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm1;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm2;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm3;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm4;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm5;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm6;
//! @brief 128-bit XMM register.
ASMJIT_VAR const XMMReg xmm7;

#if defined(ASMJIT_X64)
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm8;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm9;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm10;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm11;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm12;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm13;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm14;
//! @brief 128-bit XMM register (64-bit mode only).
ASMJIT_VAR const XMMReg xmm15;
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - Segment]
// ============================================================================

ASMJIT_VAR const SegmentReg cs;
ASMJIT_VAR const SegmentReg ss;
ASMJIT_VAR const SegmentReg ds;
ASMJIT_VAR const SegmentReg es;
ASMJIT_VAR const SegmentReg fs;
ASMJIT_VAR const SegmentReg gs;

// ============================================================================
// [AsmJit::Registers - Register From Index]
// ============================================================================

//! @brief Get general purpose register of byte size.
static inline GPReg gpb_lo(uint32_t index) ASMJIT_NOTHROW
{ return GPReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_GPB_LO)); }

//! @brief Get general purpose register of byte size.
static inline GPReg gpb_hi(uint32_t index) ASMJIT_NOTHROW
{ return GPReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_GPB_HI)); }

//! @brief Get general purpose register of word size.
static inline GPReg gpw(uint32_t index) ASMJIT_NOTHROW
{ return GPReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_GPW)); }

//! @brief Get general purpose register of dword size.
static inline GPReg gpd(uint32_t index) ASMJIT_NOTHROW
{ return GPReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_GPD)); }

#if defined(ASMJIT_X64)
//! @brief Get general purpose register of qword size (64-bit only).
static inline GPReg gpq(uint32_t index) ASMJIT_NOTHROW
{ return GPReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_GPQ)); }
#endif

//! @brief Get general purpose dword/qword register (depending to architecture).
static inline GPReg gpn(uint32_t index) ASMJIT_NOTHROW
{ return GPReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_GPN)); }

//! @brief Get MMX (MM) register .
static inline MMReg mm(uint32_t index) ASMJIT_NOTHROW
{ return MMReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_MM)); }

//! @brief Get SSE (XMM) register.
static inline XMMReg xmm(uint32_t index) ASMJIT_NOTHROW
{ return XMMReg(_Initialize(), static_cast<uint32_t>(index | REG_TYPE_XMM)); }

//! @brief Get x87 register with index @a i.
static inline X87Reg st(uint32_t i) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(i < 8);
  return X87Reg(_Initialize(), static_cast<uint32_t>(i));
}

// ============================================================================
// [AsmJit::Imm]
// ============================================================================

//! @brief Immediate operand.
//!
//! Immediate operand is part of instruction (it's inlined after it).
//!
//! To create immediate operand, use @c imm() and @c uimm() constructors
//! or constructors provided by @c Immediate class itself.
struct ASMJIT_HIDDEN Imm : public Operand
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new immediate value (initial value is 0).
  Imm() ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _imm.op = OPERAND_IMM;
    _imm.size = 0;
    _imm.isUnsigned = false;
    _imm.reserved = 0;

    _imm.id = INVALID_VALUE;
    _imm.value = 0;
  }

  //! @brief Create a new signed immediate value, assigning the value to @a i.
  Imm(sysint_t i) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _imm.op = OPERAND_IMM;
    _imm.size = 0;
    _imm.isUnsigned = false;
    _imm.reserved = 0;

    _imm.id = INVALID_VALUE;
    _imm.value = i;
  }

  //! @brief Create a new signed or unsigned immediate value, assigning the value to @a i.
  Imm(sysint_t i, bool isUnsigned) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _imm.op = OPERAND_IMM;
    _imm.size = 0;
    _imm.isUnsigned = isUnsigned;
    _imm.reserved = 0;

    _imm.id = INVALID_VALUE;
    _imm.value = i;
  }

  //! @brief Create a new immediate value from @a other.
  inline Imm(const Imm& other) ASMJIT_NOTHROW :
    Operand(other) {}

  // --------------------------------------------------------------------------
  // [Immediate Specific]
  // --------------------------------------------------------------------------

  //! @brief Get whether an immediate is unsigned value.
  inline bool isUnsigned() const ASMJIT_NOTHROW { return _imm.isUnsigned != 0; }

  //! @brief Get signed immediate value.
  inline sysint_t getValue() const ASMJIT_NOTHROW { return _imm.value; }

  //! @brief Get unsigned immediate value.
  inline sysuint_t getUValue() const ASMJIT_NOTHROW { return (sysuint_t)_imm.value; }

  //! @brief Set immediate value as signed type to @a val.
  inline void setValue(sysint_t val, bool isUnsigned = false) ASMJIT_NOTHROW
  {
    _imm.value = val;
    _imm.isUnsigned = isUnsigned;
  }

  //! @brief Set immediate value as unsigned type to @a val.
  inline void setUValue(sysuint_t val) ASMJIT_NOTHROW
  {
    _imm.value = (sysint_t)val;
    _imm.isUnsigned = true;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  //! @brief Assign a signed value @a val to the immediate operand.
  inline Imm& operator=(sysint_t val) ASMJIT_NOTHROW
  { setValue(val); return *this; }

  //! @brief Assign @a other to the immediate operand.
  inline Imm& operator=(const Imm& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }
};

//! @brief Create signed immediate value operand.
ASMJIT_API Imm imm(sysint_t i) ASMJIT_NOTHROW;

//! @brief Create unsigned immediate value operand.
ASMJIT_API Imm uimm(sysuint_t i) ASMJIT_NOTHROW;

// ============================================================================
// [AsmJit::Label]
// ============================================================================

//! @brief Label (jump target or data location).
//!
//! Label represents locations typically used as jump targets, but may be also
//! used as position where are stored constants or static variables. If you 
//! want to use @c Label you need first to associate it with @c Assembler or
//! @c Compiler instance. To create new label use @c Assembler::newLabel() or
//! @c Compiler::newLabel().
//!
//! Example of using labels:
//!
//! @code
//! // Create Assembler or Compiler instance.
//! Assembler a;
//! 
//! // Create Label instance.
//! Label L_1(a);
//!
//! // ... your code ...
//!
//! // Using label, see @c AsmJit::Assembler or @c AsmJit::Compiler.
//! a.jump(L_1);
//!
//! // ... your code ...
//!
//! // Bind label to current position, see @c AsmJit::Assembler::bind() or
//! // @c AsmJit::Compiler::bind().
//! a.bind(L_1);
//! @endcode
struct ASMJIT_HIDDEN Label : public Operand
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new, unassociated label.
  inline Label() ASMJIT_NOTHROW : 
    Operand(_DontInitialize())
  {
    _lbl.op = OPERAND_LABEL;
    _lbl.size = 0;
    _lbl.id = INVALID_VALUE;
  }

  //! @brief Create reference to another label.
  inline Label(const Label& other) ASMJIT_NOTHROW :
    Operand(other)
  {
  }

  //! @brief Destroy the label.
  inline ~Label() ASMJIT_NOTHROW
  {
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline Label& operator=(const Label& other)
  { _copy(other); return *this; }

  inline bool operator==(const Label& other) const ASMJIT_NOTHROW { return _base.id == other._base.id; }
  inline bool operator!=(const Label& other) const ASMJIT_NOTHROW { return _base.id != other._base.id; }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::Mem]
// ============================================================================

//! @brief Memory operand.
struct ASMJIT_HIDDEN Mem : public Operand
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Mem() ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _mem.op = OPERAND_MEM;
    _mem.size = 0;
    _mem.type = OPERAND_MEM_NATIVE;
    _mem.segmentPrefix = SEGMENT_NONE;
    _mem.sizePrefix = 0;
    _mem.shift = 0;

    _mem.id = INVALID_VALUE;
    _mem.base = INVALID_VALUE;
    _mem.index = INVALID_VALUE;

    _mem.target = NULL;
    _mem.displacement = 0;
  }

  inline Mem(const Label& label, sysint_t displacement, uint32_t size = 0) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _mem.op = OPERAND_MEM;
    _mem.size = (uint8_t)size;
    _mem.type = OPERAND_MEM_LABEL;
    _mem.segmentPrefix = SEGMENT_NONE;
    _mem.sizePrefix = 0;
    _mem.shift = 0;

    _mem.id = INVALID_VALUE;
    _mem.base = reinterpret_cast<const Operand&>(label)._base.id;
    _mem.index = INVALID_VALUE;

    _mem.target = NULL;
    _mem.displacement = displacement;
  }

  inline Mem(const GPReg& base, sysint_t displacement, uint32_t size = 0) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _mem.op = OPERAND_MEM;
    _mem.size = (uint8_t)size;
    _mem.type = OPERAND_MEM_NATIVE;
    _mem.segmentPrefix = SEGMENT_NONE;

#if defined(ASMJIT_X86)
    _mem.sizePrefix = base.getSize() != 4;
#else
    _mem.sizePrefix = base.getSize() != 8;
#endif

    _mem.shift = 0;

    _mem.id = INVALID_VALUE;
    _mem.base = base.getRegCode() & REG_INDEX_MASK;
    _mem.index = INVALID_VALUE;

    _mem.target = NULL;
    _mem.displacement = displacement;
  }

  inline Mem(const GPVar& base, sysint_t displacement, uint32_t size = 0) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _mem.op = OPERAND_MEM;
    _mem.size = (uint8_t)size;
    _mem.type = OPERAND_MEM_NATIVE;
    _mem.segmentPrefix = SEGMENT_NONE;

#if defined(ASMJIT_X86)
    _mem.sizePrefix = (reinterpret_cast<const Operand&>(base)._var.size) != 4;
#else
    _mem.sizePrefix = (reinterpret_cast<const Operand&>(base)._var.size) != 8;
#endif

    _mem.shift = 0;

    _mem.id = INVALID_VALUE;
    _mem.base = reinterpret_cast<const Operand&>(base).getId();
    _mem.index = INVALID_VALUE;

    _mem.target = NULL;
    _mem.displacement = displacement;
  }

  inline Mem(const GPReg& base, const GPReg& index, uint32_t shift, sysint_t displacement, uint32_t size = 0) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    ASMJIT_ASSERT(shift <= 3);

    _mem.op = OPERAND_MEM;
    _mem.size = (uint8_t)size;
    _mem.type = OPERAND_MEM_NATIVE;
    _mem.segmentPrefix = SEGMENT_NONE;

#if defined(ASMJIT_X86)
    _mem.sizePrefix = (base.getSize() | index.getSize()) != 4;
#else
    _mem.sizePrefix = (base.getSize() | index.getSize()) != 8;
#endif

    _mem.shift = (uint8_t)shift;

    _mem.id = INVALID_VALUE;
    _mem.base = base.getRegIndex();
    _mem.index = index.getRegIndex();

    _mem.target = NULL;
    _mem.displacement = displacement;
  }

  inline Mem(const GPVar& base, const GPVar& index, uint32_t shift, sysint_t displacement, uint32_t size = 0) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    ASMJIT_ASSERT(shift <= 3);

    _mem.op = OPERAND_MEM;
    _mem.size = (uint8_t)size;
    _mem.type = OPERAND_MEM_NATIVE;
    _mem.segmentPrefix = SEGMENT_NONE;

#if defined(ASMJIT_X86)
    _mem.sizePrefix = (reinterpret_cast<const Operand&>(base )._var.size | 
                       reinterpret_cast<const Operand&>(index)._var.size ) != 4;
#else
    _mem.sizePrefix = (reinterpret_cast<const Operand&>(base )._var.size | 
                       reinterpret_cast<const Operand&>(index)._var.size ) != 8;
#endif

    _mem.shift = (uint8_t)shift;

    _mem.id = INVALID_VALUE;
    _mem.base = reinterpret_cast<const Operand&>(base).getId();
    _mem.index = reinterpret_cast<const Operand&>(index).getId();

    _mem.target = NULL;
    _mem.displacement = displacement;
  }

  inline Mem(const Mem& other) ASMJIT_NOTHROW :
    Operand(other)
  {
  }

  inline Mem(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    Operand(dontInitialize)
  {
  }

  // --------------------------------------------------------------------------
  // [Mem Specific]
  // --------------------------------------------------------------------------

  //! @brief Get type of memory operand, see @c OPERAND_MEM_TYPE enum.
  inline uint32_t getMemType() const ASMJIT_NOTHROW
  { return _mem.type; }

  //! @brief Get memory operand segment prefix, see @c SEGMENT_PREFIX enum.
  inline uint32_t getSegmentPrefix() const ASMJIT_NOTHROW
  { return _mem.segmentPrefix; }

  //! @brief Get whether the memory operand has base register.
  inline bool hasBase() const ASMJIT_NOTHROW
  { return _mem.base != INVALID_VALUE; }

  //! @brief Get whether the memory operand has index.
  inline bool hasIndex() const ASMJIT_NOTHROW
  { return _mem.index != INVALID_VALUE; }

  //! @brief Get whether the memory operand has shift used.
  inline bool hasShift() const ASMJIT_NOTHROW
  { return _mem.shift != 0; }

  //! @brief Get memory operand base register or @c INVALID_VALUE.
  inline uint32_t getBase() const ASMJIT_NOTHROW
  { return _mem.base; }

  //! @brief Get memory operand index register or @c INVALID_VALUE.
  inline uint32_t getIndex() const ASMJIT_NOTHROW
  { return _mem.index; }

  //! @brief Get memory operand index scale (0, 1, 2 or 3).
  inline uint32_t getShift() const ASMJIT_NOTHROW
  { return _mem.shift; }

  //! @brief Get whether to use size-override prefix.
  //!
  //! @note This is useful only for MOV and LEA type of instructions.
  inline bool getSizePrefix() const ASMJIT_NOTHROW
  { return _mem.sizePrefix; }
  
  //! @brief Set whether to use size-override prefix.
  inline void setSizePrefix(bool b) ASMJIT_NOTHROW
  { _mem.sizePrefix = b; }

  //! @brief Get absolute target address.
  //!
  //! @note You should always check if operand contains address by @c getMemType().
  inline void* getTarget() const ASMJIT_NOTHROW
  { return _mem.target; }

  //! @brief Set memory operand size.
  inline void setSize(uint32_t size) ASMJIT_NOTHROW
  { _mem.size = size; }

  //! @brief Set absolute target address.
  inline void setTarget(void* target) ASMJIT_NOTHROW
  { _mem.target = target; }

  //! @brief Get memory operand relative displacement.
  inline sysint_t getDisplacement() const ASMJIT_NOTHROW
  { return _mem.displacement; }

  //! @brief Set memory operand relative displacement.
  inline void setDisplacement(sysint_t displacement) ASMJIT_NOTHROW
  { _mem.displacement = displacement; }

  //! @brief Adjust memory operand relative displacement by @a displacement.
  inline void adjust(sysint_t displacement) ASMJIT_NOTHROW
  {
    _mem.displacement += displacement;
  }

  //! @brief Get new memory operand adjusted by @a displacement.
  inline Mem adjusted(sysint_t displacement) const ASMJIT_NOTHROW
  {
    Mem result(*this);
    result.adjust(displacement);
    return result;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline Mem& operator=(const Mem& other) ASMJIT_NOTHROW
  {
    _copy(other);
    return *this;
  }

  inline bool operator==(const Mem& other) const ASMJIT_NOTHROW
  {
    return _bin.u32[0] == other._bin.u32[0] &&
           _bin.u32[1] == other._bin.u32[1] &&
           _bin.u32[2] == other._bin.u32[2] &&
           _bin.u32[3] == other._bin.u32[3] &&
           _bin.s[0] == other._bin.s[0] &&
           _bin.s[1] == other._bin.s[1];
  }

  inline bool operator!=(const Mem& other) const ASMJIT_NOTHROW
  {
    return !(*this == other);
  }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::BaseVar]
// ============================================================================

ASMJIT_API Mem _BaseVarMem(const BaseVar& var, uint32_t ptrSize) ASMJIT_NOTHROW;
ASMJIT_API Mem _BaseVarMem(const BaseVar& var, uint32_t ptrSize, sysint_t disp) ASMJIT_NOTHROW;
ASMJIT_API Mem _BaseVarMem(const BaseVar& var, uint32_t ptrSize, const GPVar& index, uint32_t shift, sysint_t disp) ASMJIT_NOTHROW;

//! @brief Base class for all variables.
struct ASMJIT_HIDDEN BaseVar : public Operand
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline BaseVar(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    Operand(dontInitialize)
  {
  }
#endif // ASMJIT_NODOC

  inline BaseVar() ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _var.op = OPERAND_VAR;
    _var.size = 0;
    _var.registerCode = INVALID_VALUE;
    _var.variableType = INVALID_VALUE;
    _var.id = INVALID_VALUE;
  }

  inline BaseVar(const BaseVar& other) ASMJIT_NOTHROW :
    Operand(other)
  {
  }

  // --------------------------------------------------------------------------
  // [Type]
  // --------------------------------------------------------------------------

  inline uint32_t getVariableType() const ASMJIT_NOTHROW
  { return _var.variableType; }

  inline bool isGPVar() const ASMJIT_NOTHROW
  { return _var.variableType <= VARIABLE_TYPE_GPQ; }

  inline bool isX87Var() const ASMJIT_NOTHROW
  { return _var.variableType >= VARIABLE_TYPE_X87 && _var.variableType <= VARIABLE_TYPE_X87_1D; }

  inline bool isMMVar() const ASMJIT_NOTHROW
  { return _var.variableType == VARIABLE_TYPE_MM; }

  inline bool isXMMVar() const ASMJIT_NOTHROW
  { return _var.variableType >= VARIABLE_TYPE_XMM && _var.variableType <= VARIABLE_TYPE_XMM_2D; }

  // --------------------------------------------------------------------------
  // [Memory Cast]
  // --------------------------------------------------------------------------

  //! @brief Cast this variable to memory operand.
  //!
  //! @note Size of operand depends on native variable type, you can use other
  //! variants if you want specific one.
  inline Mem m() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, INVALID_VALUE); }

  //! @overload.
  inline Mem m(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, INVALID_VALUE, disp); }

  //! @overload.
  inline Mem m(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, INVALID_VALUE, index, shift, disp); }

  //! @brief Cast this variable to 8-bit memory operand.
  inline Mem m8() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 1); }

  //! @overload.
  inline Mem m8(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 1, disp); }

  //! @overload.
  inline Mem m8(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 1, index, shift, disp); }

  //! @brief Cast this variable to 16-bit memory operand.
  inline Mem m16() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 2); }

  //! @overload.
  inline Mem m16(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 2, disp); }

  //! @overload.
  inline Mem m16(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 2, index, shift, disp); }

  //! @brief Cast this variable to 32-bit memory operand.
  inline Mem m32() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 4); }

  //! @overload.
  inline Mem m32(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 4, disp); }

  //! @overload.
  inline Mem m32(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 4, index, shift, disp); }

  //! @brief Cast this variable to 64-bit memory operand.
  inline Mem m64() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 8); }

  //! @overload.
  inline Mem m64(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 8, disp); }

  //! @overload.
  inline Mem m64(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 8, index, shift, disp); }

  //! @brief Cast this variable to 80-bit memory operand (long double).
  inline Mem m80() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 10); }

  //! @overload.
  inline Mem m80(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 10, disp); }

  //! @overload.
  inline Mem m80(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 10, index, shift, disp); }

  //! @brief Cast this variable to 128-bit memory operand.
  inline Mem m128() const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 16); }

  //! @overload.
  inline Mem m128(sysint_t disp) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 16, disp); }

  //! @overload.
  inline Mem m128(const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) const ASMJIT_NOTHROW
  { return _BaseVarMem(*this, 16, index, shift, disp); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline BaseVar& operator=(const BaseVar& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }

  inline bool operator==(const BaseVar& other) const ASMJIT_NOTHROW { return _base.id == other._base.id && _var.registerCode == other._var.registerCode; }
  inline bool operator!=(const BaseVar& other) const ASMJIT_NOTHROW { return _base.id != other._base.id || _var.registerCode != other._var.registerCode; }
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

protected:
  inline BaseVar(const BaseVar& other, uint32_t registerCode, uint32_t size) ASMJIT_NOTHROW :
    Operand(_DontInitialize())
  {
    _var.op = OPERAND_VAR;
    _var.size = (uint8_t)size;
    _var.id = other._base.id;
    _var.registerCode = registerCode;
    _var.variableType = other._var.variableType;
  }
};

// ============================================================================
// [AsmJit::X87Var]
// ============================================================================

//! @brief X87 Variable operand.
struct ASMJIT_HIDDEN X87Var : public BaseVar
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline X87Var(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseVar(dontInitialize)
  {
  }

  inline X87Var() ASMJIT_NOTHROW :
    BaseVar(_DontInitialize())
  {
    _var.op = OPERAND_VAR;
    _var.size = 12;
    _var.id = INVALID_VALUE;

    _var.registerCode = REG_TYPE_X87;
    _var.variableType = VARIABLE_TYPE_X87;
  }

  inline X87Var(const X87Var& other) ASMJIT_NOTHROW :
    BaseVar(other) {}

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline X87Var& operator=(const X87Var& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }

  inline bool operator==(const X87Var& other) const ASMJIT_NOTHROW { return _base.id == other._base.id; }
  inline bool operator!=(const X87Var& other) const ASMJIT_NOTHROW { return _base.id != other._base.id; }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::GPVar]
// ============================================================================

//! @brief GP variable operand.
struct ASMJIT_HIDDEN GPVar : public BaseVar
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new uninitialized @c GPVar instance (internal constructor).
  inline GPVar(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseVar(dontInitialize)
  {
  }

  //! @brief Create new uninitialized @c GPVar instance.
  inline GPVar() ASMJIT_NOTHROW :
    BaseVar(_DontInitialize())
  {
    _var.op = OPERAND_VAR;
    _var.size = sizeof(sysint_t);
    _var.id = INVALID_VALUE;

    _var.registerCode = REG_TYPE_GPN;
    _var.variableType = VARIABLE_TYPE_GPN;
  }

  //! @brief Create new @c GPVar instance using @a other.
  //!
  //! Note this will not create a different variable, use @c Compiler::newGP()
  //! if you want to do so. This is only copy-constructor that allows to store
  //! the same variable in different places.
  inline GPVar(const GPVar& other) ASMJIT_NOTHROW :
    BaseVar(other) {}

  // --------------------------------------------------------------------------
  // [GPVar Specific]
  // --------------------------------------------------------------------------

  //! @brief Get whether this variable is general purpose BYTE register.
  inline bool isGPB() const ASMJIT_NOTHROW { return (_var.registerCode & REG_TYPE_MASK) <= REG_TYPE_GPB_HI; }
  //! @brief Get whether this variable is general purpose BYTE.LO register.
  inline bool isGPBLo() const ASMJIT_NOTHROW { return (_var.registerCode & REG_TYPE_MASK) == REG_TYPE_GPB_LO; }
  //! @brief Get whether this variable is general purpose BYTE.HI register.
  inline bool isGPBHi() const ASMJIT_NOTHROW { return (_var.registerCode & REG_TYPE_MASK) == REG_TYPE_GPB_HI; }

  //! @brief Get whether this variable is general purpose WORD register.
  inline bool isGPW() const ASMJIT_NOTHROW { return (_var.registerCode & REG_TYPE_MASK) == REG_TYPE_GPW; }
  //! @brief Get whether this variable is general purpose DWORD register.
  inline bool isGPD() const ASMJIT_NOTHROW { return (_var.registerCode & REG_TYPE_MASK) == REG_TYPE_GPD; }
  //! @brief Get whether this variable is general purpose QWORD (only 64-bit) register.
  inline bool isGPQ() const ASMJIT_NOTHROW { return (_var.registerCode & REG_TYPE_MASK) == REG_TYPE_GPQ; }

  // --------------------------------------------------------------------------
  // [GPVar Cast]
  // --------------------------------------------------------------------------

  //! @brief Cast this variable to 8-bit (LO) part of variable
  inline GPVar r8() const ASMJIT_NOTHROW { return GPVar(*this, REG_TYPE_GPB_LO, 1); }
  //! @brief Cast this variable to 8-bit (LO) part of variable
  inline GPVar r8Lo() const ASMJIT_NOTHROW { return GPVar(*this, REG_TYPE_GPB_LO, 1); }
  //! @brief Cast this variable to 8-bit (HI) part of variable
  inline GPVar r8Hi() const ASMJIT_NOTHROW { return GPVar(*this, REG_TYPE_GPB_HI, 1); }

  //! @brief Cast this variable to 16-bit part of variable
  inline GPVar r16() const ASMJIT_NOTHROW { return GPVar(*this, REG_TYPE_GPW, 2); }
  //! @brief Cast this variable to 32-bit part of variable
  inline GPVar r32() const ASMJIT_NOTHROW { return GPVar(*this, REG_TYPE_GPD, 4); }
#if defined(ASMJIT_X64)
  //! @brief Cast this variable to 64-bit part of variable
  inline GPVar r64() const ASMJIT_NOTHROW { return GPVar(*this, REG_TYPE_GPQ, 8); }
#endif // ASMJIT_X64

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline GPVar& operator=(const GPVar& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }

  inline bool operator==(const GPVar& other) const ASMJIT_NOTHROW { return _base.id == other._base.id && _var.registerCode == other._var.registerCode; }
  inline bool operator!=(const GPVar& other) const ASMJIT_NOTHROW { return _base.id != other._base.id || _var.registerCode != other._var.registerCode; }
#endif // ASMJIT_NODOC

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

protected:
  inline GPVar(const GPVar& other, uint32_t registerCode, uint32_t size) ASMJIT_NOTHROW :
    BaseVar(other, registerCode, size)
  {
  }
};

// ============================================================================
// [AsmJit::MMVar]
// ============================================================================

//! @brief MM variable operand.
struct ASMJIT_HIDDEN MMVar : public BaseVar
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new uninitialized @c MMVar instance (internal constructor).
  inline MMVar(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseVar(dontInitialize)
  {
  }

  //! @brief Create new uninitialized @c MMVar instance.
  inline MMVar() ASMJIT_NOTHROW :
    BaseVar(_DontInitialize())
  {
    _var.op = OPERAND_VAR;
    _var.size = 8;
    _var.id = INVALID_VALUE;

    _var.registerCode = REG_TYPE_MM;
    _var.variableType = VARIABLE_TYPE_MM;
  }

  //! @brief Create new @c MMVar instance using @a other.
  //!
  //! Note this will not create a different variable, use @c Compiler::newMM()
  //! if you want to do so. This is only copy-constructor that allows to store
  //! the same variable in different places.
  inline MMVar(const MMVar& other) ASMJIT_NOTHROW :
    BaseVar(other) {}

  // --------------------------------------------------------------------------
  // [MMVar Cast]
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline MMVar& operator=(const MMVar& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }

  inline bool operator==(const MMVar& other) const ASMJIT_NOTHROW { return _base.id == other._base.id; }
  inline bool operator!=(const MMVar& other) const ASMJIT_NOTHROW { return _base.id != other._base.id; }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::XMMVar]
// ============================================================================

//! @brief XMM Variable operand.
struct ASMJIT_HIDDEN XMMVar : public BaseVar
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline XMMVar(const _DontInitialize& dontInitialize) ASMJIT_NOTHROW :
    BaseVar(dontInitialize)
  {
  }

  inline XMMVar() ASMJIT_NOTHROW :
    BaseVar(_DontInitialize())
  {
    _var.op = OPERAND_VAR;
    _var.size = 16;
    _var.id = INVALID_VALUE;

    _var.registerCode = REG_TYPE_XMM;
    _var.variableType = VARIABLE_TYPE_XMM;
  }

  inline XMMVar(const XMMVar& other) ASMJIT_NOTHROW :
    BaseVar(other) {}

  // --------------------------------------------------------------------------
  // [XMMVar Access]
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_NODOC)
  inline XMMVar& operator=(const XMMVar& other) ASMJIT_NOTHROW
  { _copy(other); return *this; }

  inline bool operator==(const XMMVar& other) const ASMJIT_NOTHROW { return _base.id == other._base.id; }
  inline bool operator!=(const XMMVar& other) const ASMJIT_NOTHROW { return _base.id != other._base.id; }
#endif // ASMJIT_NODOC
};

// ============================================================================
// [AsmJit::Mem - ptr[displacement]]
// ============================================================================

//! @internal
ASMJIT_API Mem _MemPtrBuild(const Label& label, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;

//! @internal
ASMJIT_API Mem _MemPtrBuild(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;

//! @internal
ASMJIT_API Mem _MemPtrBuild(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const Label& label, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, disp, sizeof(sysint_t)); }



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const Label& label, const GPReg& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, sizeof(sysint_t)); }



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const Label& label, const GPVar& index, uint32_t shift, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(label, index, shift, disp, sizeof(sysint_t)); }

// ============================================================================
// [AsmJit::Mem - Absolute Addressing]
// ============================================================================

//! @internal
ASMJIT_API Mem _MemPtrAbs(
  void* target,
  sysint_t disp,
  uint32_t segmentPrefix, uint32_t ptrSize) ASMJIT_NOTHROW;

//! @internal
ASMJIT_API Mem _MemPtrAbs(
  void* target,
  const GPReg& index, uint32_t shift, sysint_t disp,
  uint32_t segmentPrefix, uint32_t ptrSize) ASMJIT_NOTHROW;

//! @internal
ASMJIT_API Mem _MemPtrAbs(
  void* target,
  const GPVar& index, uint32_t shift, sysint_t disp,
  uint32_t segmentPrefix, uint32_t ptrSize) ASMJIT_NOTHROW;



//! @brief Create pointer operand with not specified size.
static inline Mem ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr_abs(void* target, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, disp, segmentPrefix, sizeof(sysint_t)); }



//! @brief Create pointer operand with not specified size.
static inline Mem ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr_abs(void* target, const GPReg& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, sizeof(sysint_t)); }



//! @brief Create pointer operand with not specified size.
static inline Mem ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr_abs(void* target, const GPVar& index, uint32_t shift, sysint_t disp = 0, uint32_t segmentPrefix = SEGMENT_NONE) ASMJIT_NOTHROW
{ return _MemPtrAbs(target, index, shift, disp, segmentPrefix, sizeof(sysint_t)); }

// ============================================================================
// [AsmJit::Mem - ptr[base + displacement]]
// ============================================================================

//! @internal
ASMJIT_API Mem _MemPtrBuild(const GPReg& base, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;

//! @internal
ASMJIT_API Mem _MemPtrBuild(const GPVar& base, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const GPReg& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, sizeof(sysint_t)); }



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 bytes) pointer operand
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const GPVar& base, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, disp, sizeof(sysint_t)); }

// ============================================================================
// [AsmJit::Mem - ptr[base + (index << shift) + displacement]]
// ============================================================================

//! @internal
ASMJIT_API Mem _MemPtrBuild(const GPReg& base, const GPReg& index, uint32_t shift, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;

//! @internal
ASMJIT_API Mem _MemPtrBuild(const GPVar& base, const GPVar& index, uint32_t shift, sysint_t disp, uint32_t ptrSize) ASMJIT_NOTHROW;



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 Bytes) pointer operand).
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 Bytes) pointer operand.
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const GPReg& base, const GPReg& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, sizeof(sysint_t)); }



//! @brief Create pointer operand with not specified size.
static inline Mem ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, 0); }

//! @brief Create byte pointer operand.
static inline Mem byte_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_BYTE); }

//! @brief Create word (2 Bytes) pointer operand.
static inline Mem word_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_WORD); }

//! @brief Create dword (4 Bytes) pointer operand.
static inline Mem dword_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_DWORD); }

//! @brief Create qword (8 Bytes) pointer operand.
static inline Mem qword_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_QWORD); }

//! @brief Create tword (10 Bytes) pointer operand (used for 80-bit floating points).
static inline Mem tword_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_TWORD); }

//! @brief Create dqword (16 Bytes) pointer operand.
static inline Mem dqword_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_DQWORD); }

//! @brief Create mmword (8 Bytes) pointer operand).
//!
//! @note This constructor is provided only for convenience for mmx programming.
static inline Mem mmword_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_QWORD); }

//! @brief Create xmmword (16 Bytes) pointer operand.
//!
//! @note This constructor is provided only for convenience for sse programming.
static inline Mem xmmword_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, SIZE_DQWORD); }

//! @brief Create system dependent pointer operand (32-bit or 64-bit).
static inline Mem sysint_ptr(const GPVar& base, const GPVar& index, uint32_t shift = 0, sysint_t disp = 0) ASMJIT_NOTHROW
{ return _MemPtrBuild(base, index, shift, disp, sizeof(sysint_t)); }

// ============================================================================
// [AsmJit::Macros]
// ============================================================================

//! @brief Create Shuffle Constant for MMX/SSE shuffle instrutions.
//! @param z First component position, number at interval [0, 3] inclusive.
//! @param x Second component position, number at interval [0, 3] inclusive.
//! @param y Third component position, number at interval [0, 3] inclusive.
//! @param w Fourth component position, number at interval [0, 3] inclusive.
//!
//! Shuffle constants can be used to make immediate value for these intrinsics:
//! - @c AsmJit::AssemblerIntrinsics::pshufw()
//! - @c AsmJit::AssemblerIntrinsics::pshufd()
//! - @c AsmJit::AssemblerIntrinsics::pshufhw()
//! - @c AsmJit::AssemblerIntrinsics::pshuflw()
//! - @c AsmJit::AssemblerIntrinsics::shufps()
static inline uint8_t mm_shuffle(uint8_t z, uint8_t y, uint8_t x, uint8_t w) ASMJIT_NOTHROW
{ return (z << 6) | (y << 4) | (x << 2) | w; }

//! @}

} // AsmJit namespace

// [Guard]
#endif // _ASMJIT_OPERANDX86X64_H
