// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_COMPILER_H
#define _ASMJIT_BASE_COMPILER_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../base/codegen.h"
#include "../base/constpool.h"
#include "../base/error.h"
#include "../base/func.h"
#include "../base/intutil.h"
#include "../base/operand.h"
#include "../base/podlist.h"
#include "../base/podvector.h"
#include "../base/runtime.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct BaseCompiler;

struct VarAttr;
struct VarData;
struct BaseVarInst;
struct BaseVarState;

struct Node;
struct EndNode;
struct InstNode;
struct JumpNode;

// ============================================================================
// [asmjit::kConstScope]
// ============================================================================

//! \addtogroup asmjit_base_general
//! \{

//! Scope of the constant.
ASMJIT_ENUM(kConstScope) {
  //! Local constant, always embedded right after the current function.
  kConstScopeLocal = 0,
  //! Global constant, embedded at the end of the currently compiled code.
  kConstScopeGlobal = 1
};

//! \}

// ============================================================================
// [asmjit::kVarAttrFlags]
// ============================================================================

//! \addtogroup asmjit_base_tree
//! \{

//! Variable attribute flags.
ASMJIT_ENUM(kVarAttrFlags) {
  //! Variable is accessed through register on input.
  kVarAttrInReg = 0x00000001,
  //! Variable is accessed through register on output.
  kVarAttrOutReg = 0x00000002,
  //! Variable is accessed through register on input & output.
  kVarAttrInOutReg = 0x00000003,

  //! Variable is accessed through memory on input.
  kVarAttrInMem = 0x00000004,
  //! Variable is accessed through memory on output.
  kVarAttrOutMem = 0x00000008,
  //! Variable is accessed through memory on input & output.
  kVarAttrInOutMem = 0x0000000C,

  //! Register allocator can decide if input will be in register or memory.
  kVarAttrInDecide = 0x00000010,
  //! Register allocator can decide if output will be in register or memory.
  kVarAttrOutDecide = 0x00000020,
  //! Register allocator can decide if in/out will be in register or memory.
  kVarAttrInOutDecide = 0x00000030,

  //! Variable is converted to other type/class on the input.
  kVarAttrInConv = 0x00000040,
  //! Variable is converted from other type/class on the output.
  kVarAttrOutConv = 0x00000080,
  //! Combination of `kVarAttrInConv` and `kVarAttrOutConv`.
  kVarAttrInOutConv = 0x000000C0,

  //! Variable is a function call operand.
  kVarAttrInCall = 0x00000100,
  //! Variable is a function argument passed in register.
  kVarAttrInArg = 0x00000200,

  //! Variable is a function return value passed in register.
  kVarAttrOutRet = 0x00000400,
  //! Variable should be unused at the end of the instruction/node.
  kVarAttrUnuse = 0x00000800,

  //! \internal
  //!
  //! All in-flags.
  kVarAttrInAll =
    kVarAttrInReg     |
    kVarAttrInMem     |
    kVarAttrInDecide  |
    kVarAttrInCall    |
    kVarAttrInArg,

  //! \internal
  //!
  //! All out-flags.
  kVarAttrOutAll =
    kVarAttrOutReg    |
    kVarAttrOutMem    |
    kVarAttrOutDecide |
    kVarAttrOutRet,

  //! Variable is already allocated on the input.
  kVarAttrAllocInDone = 0x00400000,
  //! Variable is already allocated on the output.
  kVarAttrAllocOutDone = 0x00800000
};

// ============================================================================
// [asmjit::kVarHint]
// ============================================================================

//! Variable hint (used by `BaseCompiler)`.
//!
//! @sa `BaseCompiler`.
ASMJIT_ENUM(kVarHint) {
  //! Alloc variable.
  kVarHintAlloc = 0,
  //! Spill variable.
  kVarHintSpill = 1,
  //! Save variable if modified.
  kVarHintSave = 2,
  //! Save variable if modified and mark it as unused.
  kVarHintSaveAndUnuse = 3,
  //! Mark variable as unused.
  kVarHintUnuse = 4
};

// ============================================================================
// [asmjit::kVarState]
// ============================================================================

//! State of variable.
//!
//! \note State of variable is used only during make process and it's not
//! visible to the developer.
ASMJIT_ENUM(kVarState) {
  //! Variable is currently not used.
  kVarStateUnused = 0,

  //! Variable is in register.
  //!
  //! Variable is currently allocated in register.
  kVarStateReg = 1,

  //! Variable is in memory location or spilled.
  //!
  //! Variable was spilled from register to memory or variable is used for
  //! memory only storage.
  kVarStateMem = 2
};

// ============================================================================
// [asmjit::kNodeType]
// ============================================================================

//! Type of node, see \ref Node.
ASMJIT_ENUM(kNodeType) {
  //! Invalid node (internal, can't be used).
  kNodeTypeNone = 0,
  //! Node is an .align directive, see \ref AlignNode.
  kNodeTypeAlign,
  //! Node is an embedded data, see \ref EmbedNode.
  kNodeTypeEmbed,
  //! Node is a comment, see \ref CommentNode.
  kNodeTypeComment,
  //! Node is a variable hint (alloc, spill, use, unuse), see \ref HintNode.
  kNodeTypeHint,
  //! Node is a label, see \ref TargetNode.
  kNodeTypeTarget,
  //! Node is an instruction, see \ref InstNode.
  kNodeTypeInst,
  //! Node is a function declaration, see \ref FuncNode.
  kNodeTypeFunc,
  //! Node is an end of the function, see \ref EndNode.
  kNodeTypeEnd,
  //! Node is a return, see \ref RetNode.
  kNodeTypeRet,
  //! Node is a function call, see \ref CallNode.
  kNodeTypeCall,
  //! Node is a function call argument moved on stack, see \ref SArgNode.
  kNodeTypeSArg
};

// ============================================================================
// [asmjit::kNodeFlag]
// ============================================================================

ASMJIT_ENUM(kNodeFlag) {
  //! Whether the node was translated by `BaseContext`.
  kNodeFlagIsTranslated = 0x0001,

  //! Whether the `InstNode` is a jump.
  kNodeFlagIsJmp = 0x0002,
  //! Whether the `InstNode` is a conditional jump.
  kNodeFlagIsJcc = 0x0004,

  //! Whether the `InstNode` is an unconditinal jump or conditional
  //! jump that is likely to be taken.
  kNodeFlagIsTaken = 0x0008,

  //! Whether the `Node` will return from a function.
  //!
  //! This flag is used by both `EndNode` and `RetNode`.
  kNodeFlagIsRet = 0x0010,

  //! Whether the instruction is special.
  kNodeFlagIsSpecial = 0x0020,

  //! Whether the instruction is an FPU instruction.
  kNodeFlagIsFp = 0x0040
};

//! \}

// ============================================================================
// [asmjit::host::MemCell]
// ============================================================================

//! \addtogroup asmjit_base_tree
//! \{

struct MemCell {
  ASMJIT_NO_COPY(MemCell)

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get cell offset.
  ASMJIT_INLINE int32_t getOffset() const { return _offset; }
  //! Set cell offset.
  ASMJIT_INLINE void setOffset(int32_t offset) { _offset = offset; }

  //! Get cell size.
  ASMJIT_INLINE uint32_t getSize() const { return _size; }
  //! Set cell size.
  ASMJIT_INLINE void setSize(uint32_t size) { _size = size; }

  //! Get cell alignment.
  ASMJIT_INLINE uint32_t getAlignment() const { return _alignment; }
  //! Set cell alignment.
  ASMJIT_INLINE void setAlignment(uint32_t alignment) { _alignment = alignment; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Next active cell.
  MemCell* _next;

  //! Offset, relative to base-offset.
  int32_t _offset;
  //! Size.
  uint32_t _size;
  //! Alignment.
  uint32_t _alignment;
};

// ============================================================================
// [asmjit::VarBits]
// ============================================================================

//! Bit-array used by variable-liveness analysis.
struct VarBits {
  // --------------------------------------------------------------------------
  // [Enums]
  // --------------------------------------------------------------------------

  enum {
    kEntitySize = static_cast<int>(sizeof(uintptr_t)),
    kEntityBits = kEntitySize * 8
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uintptr_t getBit(uint32_t index) const {
    return (data[index / kEntityBits] >> (index % kEntityBits)) & 1;
  }

  ASMJIT_INLINE void setBit(uint32_t index) {
    data[index / kEntityBits] |= static_cast<uintptr_t>(1) << (index % kEntityBits);
  }

  ASMJIT_INLINE void delBit(uint32_t index) {
    data[index / kEntityBits] &= ~(static_cast<uintptr_t>(1) << (index % kEntityBits));
  }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool copyBits(const VarBits* s0, uint32_t len) {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool addBits(const VarBits* s0, uint32_t len) {
    return addBits(this, s0, len);
  }

  ASMJIT_INLINE bool addBits(const VarBits* s0, const VarBits* s1, uint32_t len) {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] | s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool andBits(const VarBits* s1, uint32_t len) {
    return andBits(this, s1, len);
  }

  ASMJIT_INLINE bool andBits(const VarBits* s0, const VarBits* s1, uint32_t len) {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] & s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool delBits(const VarBits* s1, uint32_t len) {
    return delBits(this, s1, len);
  }

  ASMJIT_INLINE bool delBits(const VarBits* s0, const VarBits* s1, uint32_t len) {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] & ~s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool _addBitsDelSource(VarBits* s1, uint32_t len) {
    return _addBitsDelSource(this, s1, len);
  }

  ASMJIT_INLINE bool _addBitsDelSource(const VarBits* s0, VarBits* s1, uint32_t len) {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t a = s0->data[i];
      uintptr_t b = s1->data[i];

      this->data[i] = a | b;
      b &= ~a;

      s1->data[i] = b;
      r |= b;
    }
    return r != 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uintptr_t data[1];
};

// ============================================================================
// [asmjit::VarData]
// ============================================================================

//! Base variable data.
struct VarData {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get variable name.
  ASMJIT_INLINE const char* getName() const { return _name; }
  //! Get variable id.
  ASMJIT_INLINE uint32_t getId() const { return _id; }

  //! Get whether the variable has context id.
  ASMJIT_INLINE bool hasContextId() const { return _contextId != kInvalidValue; }
  //! Get context variable id (used only by `Context)`.
  ASMJIT_INLINE uint32_t getContextId() const { return _contextId; }
  //! Set context variable id (used only by `Context)`.
  ASMJIT_INLINE void setContextId(uint32_t contextId) { _contextId = contextId; }
  //! Reset context variable id (used only by `Context)`.
  ASMJIT_INLINE void resetContextId() { _contextId = kInvalidValue; }

  //! Get variable type.
  ASMJIT_INLINE uint32_t getType() const { return _type; }
  //! Get variable class.
  ASMJIT_INLINE uint32_t getClass() const { return _class; }
  //! Get variable flags.
  ASMJIT_INLINE uint32_t getFlags() const { return _flags; }

  //! Get variable priority.
  ASMJIT_INLINE uint32_t getPriority() const { return _priority; }

  //! Get variable state (only used by `Context)`.
  ASMJIT_INLINE uint32_t getState() const { return _state; }
  //! Set variable state (only used by `Context)`.
  ASMJIT_INLINE void setState(uint32_t state) { _state = static_cast<uint8_t>(state); }

  //! Get register index.
  ASMJIT_INLINE uint32_t getRegIndex() const { return _regIndex; }
  //! Set register index.
  ASMJIT_INLINE void setRegIndex(uint32_t regIndex) { _regIndex = static_cast<uint8_t>(regIndex); }
  //! Reset register index.
  ASMJIT_INLINE void resetRegIndex() { _regIndex = static_cast<uint8_t>(kInvalidReg); }

  //! Get whether the VarData is only memory allocated on the stack.
  ASMJIT_INLINE bool isStack() const { return static_cast<bool>(_isStack); }

  //! Get whether the variable is a function argument passed through memory.
  ASMJIT_INLINE bool isMemArg() const { return static_cast<bool>(_isMemArg); }

  //! Get variable content can be calculated by a simple instruction.
  ASMJIT_INLINE bool isCalculated() const { return static_cast<bool>(_isCalculated); }
  //! Get whether to save variable when it's unused (spill).
  ASMJIT_INLINE bool saveOnUnuse() const { return static_cast<bool>(_saveOnUnuse); }

  //! Get whether the variable was changed.
  ASMJIT_INLINE bool isModified() const { return static_cast<bool>(_modified); }
  //! Set whether the variable was changed.
  ASMJIT_INLINE void setModified(bool modified) { _modified = modified; }

  //! Get variable alignment.
  ASMJIT_INLINE uint32_t getAlignment() const { return _alignment; }
  //! Get variable size.
  ASMJIT_INLINE uint32_t getSize() const { return _size; }

  //! Get home memory offset.
  ASMJIT_INLINE int32_t getMemOffset() const { return _memOffset; }
  //! Set home memory offset.
  ASMJIT_INLINE void setMemOffset(int32_t offset) { _memOffset = offset; }

  //! Get home memory cell.
  ASMJIT_INLINE MemCell* getMemCell() const { return _memCell; }
  //! Set home memory cell.
  ASMJIT_INLINE void setMemCell(MemCell* cell) { _memCell = cell; }

  // --------------------------------------------------------------------------
  // [Accessors - Temporary Usage]
  // --------------------------------------------------------------------------

  //! Get temporary VarAttr.
  ASMJIT_INLINE VarAttr* getVa() const { return _va; }
  //! Set temporary VarAttr.
  ASMJIT_INLINE void setVa(VarAttr* va) { _va = va; }
  //! Reset temporary VarAttr.
  ASMJIT_INLINE void resetVa() { _va = NULL; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Variable name.
  const char* _name;

  //! Variable id.
  uint32_t _id;
  //! Context variable id, used by `Context` only, initially `kInvalidValue`.
  uint32_t _contextId;

  //! Variable type.
  uint8_t _type;
  //! Variable class.
  uint8_t _class;
  //! Variable flags.
  uint8_t _flags;
  //! Variable priority.
  uint8_t _priority;

  //! Variable state (connected with actual `BaseVarState)`.
  uint8_t _state;
  //! Actual register index (only used by `Context)`, during translate.
  uint8_t _regIndex;

  //! Whether the variable is only used as memory allocated on the stack.
  uint8_t _isStack : 1;
  //! Whether the variable is a function argument passed through memory.
  uint8_t _isMemArg : 1;
  //! Whether variable content can be calculated by a simple instruction.
  //!
  //! This is used mainly by MMX and SSE2 code. This flag indicates that
  //! register allocator should never reserve memory for this variable, because
  //! the content can be generated by a single instruction (for example PXOR).
  uint8_t _isCalculated : 1;
  //! Save on unuse (at end of the variable scope).
  uint8_t _saveOnUnuse : 1;
  //! Whether variable was changed (connected with actual `BaseVarState)`.
  uint8_t _modified : 1;
  //! \internal
  uint8_t _reserved0 : 3;
  //! Variable natural alignment.
  uint8_t _alignment;

  //! Variable size.
  uint32_t _size;

  //! Home memory offset.
  int32_t _memOffset;
  //! Home memory cell, used by `BaseContext` (initially NULL).
  MemCell* _memCell;

  //! Register read access statistics.
  uint32_t rReadCount;
  //! Register write access statistics.
  uint32_t rWriteCount;

  //! Memory read statistics.
  uint32_t mReadCount;
  //! Memory write statistics.
  uint32_t mWriteCount;

  // --------------------------------------------------------------------------
  // [Members - Temporary Usage]
  // --------------------------------------------------------------------------

  // These variables are only used during register allocation. They are
  // initialized by init() phase and cleared by cleanup() phase.

  union {
    //! Temporary link to VarAttr* used by the `Context` used in
    //! various phases, but always set back to NULL when finished.
    //!
    //! This temporary data is designed to be used by algorithms that need to
    //! store some data into variables themselves during compilation. But it's
    //! expected that after variable is compiled & translated the data is set
    //! back to zero/null. Initial value is NULL.
    VarAttr* _va;

    //! \internal
    //!
    //! Same as `_va` just provided as `uintptr_t`.
    uintptr_t _vaUInt;
  };
};

// ============================================================================
// [asmjit::VarAttr]
// ============================================================================

struct VarAttr {
  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void setup(VarData* vd, uint32_t flags = 0, uint32_t inRegs = 0, uint32_t allocableRegs = 0) {
    _vd = vd;
    _flags = flags;
    _varCount = 0;
    _inRegIndex = kInvalidReg;
    _outRegIndex = kInvalidReg;
    _reserved = 0;
    _inRegs = inRegs;
    _allocableRegs = allocableRegs;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get VarData->
  ASMJIT_INLINE VarData* getVd() const { return _vd; }
  //! Set VarData->
  ASMJIT_INLINE void setVd(VarData* vd) { _vd = vd; }

  //! Get flags.
  ASMJIT_INLINE uint32_t getFlags() const { return _flags; }
  //! Set flags.
  ASMJIT_INLINE void setFlags(uint32_t flags) { _flags = flags; }

  //! Get whether `flag` is on.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) { return (_flags & flag) != 0; }
  //! Add `flags`.
  ASMJIT_INLINE void addFlags(uint32_t flags) { _flags |= flags; }
  //! Mask `flags`.
  ASMJIT_INLINE void andFlags(uint32_t flags) { _flags &= flags; }
  //! Clear `flags`.
  ASMJIT_INLINE void delFlags(uint32_t flags) { _flags &= ~flags; }

  //! Get how many times the variable is used by the instruction/node.
  ASMJIT_INLINE uint32_t getVarCount() const { return _varCount; }
  //! Set how many times the variable is used by the instruction/node.
  ASMJIT_INLINE void setVarCount(uint32_t count) { _varCount = static_cast<uint8_t>(count); }
  //! Add how many times the variable is used by the instruction/node.
  ASMJIT_INLINE void addVarCount(uint32_t count = 1) { _varCount += static_cast<uint8_t>(count); }

  //! Get whether the variable has to be allocated in a specific input register.
  ASMJIT_INLINE uint32_t hasInRegIndex() const { return _inRegIndex != kInvalidReg; }
  //! Get the input register index or `kInvalidReg`.
  ASMJIT_INLINE uint32_t getInRegIndex() const { return _inRegIndex; }
  //! Set the input register index.
  ASMJIT_INLINE void setInRegIndex(uint32_t index) { _inRegIndex = static_cast<uint8_t>(index); }
  //! Reset the input register index.
  ASMJIT_INLINE void resetInRegIndex() { _inRegIndex = kInvalidReg; }

  //! Get whether the variable has to be allocated in a specific output register.
  ASMJIT_INLINE uint32_t hasOutRegIndex() const { return _outRegIndex != kInvalidReg; }
  //! Get the output register index or `kInvalidReg`.
  ASMJIT_INLINE uint32_t getOutRegIndex() const { return _outRegIndex; }
  //! Set the output register index.
  ASMJIT_INLINE void setOutRegIndex(uint32_t index) { _outRegIndex = static_cast<uint8_t>(index); }
  //! Reset the output register index.
  ASMJIT_INLINE void resetOutRegIndex() { _outRegIndex = kInvalidReg; }

  //! Get whether the mandatory input registers are in used.
  ASMJIT_INLINE bool hasInRegs() const { return _inRegs != 0; }
  //! Get mandatory input registers (mask).
  ASMJIT_INLINE uint32_t getInRegs() const { return _inRegs; }
  //! Set mandatory input registers (mask).
  ASMJIT_INLINE void setInRegs(uint32_t mask) { _inRegs = mask; }
  //! Add mandatory input registers (mask).
  ASMJIT_INLINE void addInRegs(uint32_t mask) { _inRegs |= mask; }
  //! And mandatory input registers (mask).
  ASMJIT_INLINE void andInRegs(uint32_t mask) { _inRegs &= mask; }
  //! Clear mandatory input registers (mask).
  ASMJIT_INLINE void delInRegs(uint32_t mask) { _inRegs &= ~mask; }

  //! Get allocable input registers (mask).
  ASMJIT_INLINE uint32_t getAllocableRegs() const { return _allocableRegs; }
  //! Set allocable input registers (mask).
  ASMJIT_INLINE void setAllocableRegs(uint32_t mask) { _allocableRegs = mask; }
  //! Add allocable input registers (mask).
  ASMJIT_INLINE void addAllocableRegs(uint32_t mask) { _allocableRegs |= mask; }
  //! And allocable input registers (mask).
  ASMJIT_INLINE void andAllocableRegs(uint32_t mask) { _allocableRegs &= mask; }
  //! Clear allocable input registers (mask).
  ASMJIT_INLINE void delAllocableRegs(uint32_t mask) { _allocableRegs &= ~mask; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE VarAttr& operator=(const VarAttr& other) {
    ::memcpy(this, &other, sizeof(VarAttr));
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  VarData* _vd;
  //! Flags.
  uint32_t _flags;

  union {
    struct {
      //! How many times the variable is used by the instruction/node.
      uint8_t _varCount;
      //! Input register index or `kInvalidReg` if it's not given.
      //!
      //! Even if the input register index is not given (i.e. it may by any
      //! register), register allocator should assign an index that will be
      //! used to persist a variable into this specific index. It's helpful
      //! in situations where one variable has to be allocated in multiple
      //! registers to determine the register which will be persistent.
      uint8_t _inRegIndex;
      //! Output register index or `kInvalidReg` if it's not given.
      //!
      //! Typically `kInvalidReg` if variable is only used on input.
      uint8_t _outRegIndex;
      //! \internal
      uint8_t _reserved;
    };

    //! \internal
    //!
    //! Packed data #0.
    uint32_t _packed;
  };

  //! Mandatory input registers.
  //!
  //! Mandatory input registers are required by the instruction even if
  //! there are duplicates. This schema allows us to allocate one variable
  //! in one or more register when needed. Required mostly by instructions
  //! that have implicit register operands (imul, cpuid, ...) and function
  //! call.
  uint32_t _inRegs;

  //! Allocable input registers.
  //!
  //! Optional input registers is a mask of all allocable registers for a given
  //! variable where we have to pick one of them. This mask is usually not used
  //! when _inRegs is set. If both masks are used then the register
  //! allocator tries first to find an intersection between these and allocates
  //! an extra slot if not found.
  uint32_t _allocableRegs;
};

// ============================================================================
// [asmjit::BaseVarInst]
// ============================================================================

//! Variable allocation instructions.
struct BaseVarInst {};

// ============================================================================
// [asmjit::BaseVarState]
// ============================================================================

//! Variable(s) state.
struct BaseVarState {};

// ============================================================================
// [asmjit::Node]
// ============================================================================

//! Base node.
//!
//! `Every` node represents an abstract instruction, directive, label, or
//! macro-instruction generated by compiler.
struct Node {
  ASMJIT_NO_COPY(Node)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create new `Node`.
  //!
  //! \note Always use compiler to create nodes.
  ASMJIT_INLINE Node(BaseCompiler* compiler, uint32_t type); // Defined-Later.

  //! Destroy `Node`.
  ASMJIT_INLINE ~Node() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get previous node in the compiler stream.
  ASMJIT_INLINE Node* getPrev() const { return _prev; }
  //! Get next node in the compiler stream.
  ASMJIT_INLINE Node* getNext() const { return _next; }

  //! Get comment string.
  ASMJIT_INLINE const char* getComment() const { return _comment; }
  //! Set comment string to `str`.
  ASMJIT_INLINE void setComment(const char* comment) { _comment = comment; }

  //! Get type of node, see `kNodeType`.
  ASMJIT_INLINE uint32_t getType() const { return _type; }

  //! Get node flags.
  ASMJIT_INLINE uint32_t getFlags() const { return _flags; }
  //! Set node flags to `flags`.
  ASMJIT_INLINE void setFlags(uint32_t flags) { _flags = static_cast<uint16_t>(flags); }

  //! Get whether the instruction has flag `flag`.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const { return (static_cast<uint32_t>(_flags) & flag) != 0; }
  //! Add instruction `flags`.
  ASMJIT_INLINE void addFlags(uint32_t flags) { _flags |= static_cast<uint16_t>(flags); }
  //! Clear instruction `flags`.
  ASMJIT_INLINE void delFlags(uint32_t flags) { _flags &= static_cast<uint16_t>(~flags); }

  //! Get whether the node has beed fetched.
  ASMJIT_INLINE bool isFetched() const { return _flowId != 0; }
  //! Get whether the node has been translated.
  ASMJIT_INLINE bool isTranslated() const { return hasFlag(kNodeFlagIsTranslated); }

  //! Whether the instruction is an unconditional jump.
  ASMJIT_INLINE bool isJmp() const { return hasFlag(kNodeFlagIsJmp); }
  //! Whether the instruction is a conditional jump.
  ASMJIT_INLINE bool isJcc() const { return hasFlag(kNodeFlagIsJcc); }
  //! Whether the instruction is an unconditional or conditional jump.
  ASMJIT_INLINE bool isJmpOrJcc() const { return hasFlag(kNodeFlagIsJmp | kNodeFlagIsJcc); }
  //! Whether the instruction is a return.
  ASMJIT_INLINE bool isRet() const { return hasFlag(kNodeFlagIsRet); }

  //! Get whether the instruction is special.
  ASMJIT_INLINE bool isSpecial() const { return hasFlag(kNodeFlagIsSpecial); }
  //! Get whether the instruction accesses FPU.
  ASMJIT_INLINE bool isFp() const { return hasFlag(kNodeFlagIsFp); }

  //! Get flow index.
  ASMJIT_INLINE uint32_t getFlowId() const { return _flowId; }
  //! Set flow index.
  ASMJIT_INLINE void setFlowId(uint32_t flowId) { _flowId = flowId; }

  //! Get whether node contains variable allocation instructions.
  ASMJIT_INLINE bool hasVarInst() const { return _varInst != NULL; }

  //! Get variable allocation instructions.
  ASMJIT_INLINE BaseVarInst* getVarInst() const { return _varInst; }
  //! Get variable allocation instructions casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* getVarInst() const { return static_cast<T*>(_varInst); }
  //! Set variable allocation instructions.
  ASMJIT_INLINE void setVarInst(BaseVarInst* vi) { _varInst = vi; }

  //! Get node state.
  ASMJIT_INLINE BaseVarState* getState() const { return _state; }
  //! Get node state casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* getState() const { return static_cast<BaseVarState*>(_state); }
  //! Set node state.
  ASMJIT_INLINE void setState(BaseVarState* state) { _state = state; }

  //! Get whether the node has variable liveness bits.
  ASMJIT_INLINE bool hasLiveness() const { return _liveness != NULL; }
  //! Get variable liveness bits.
  ASMJIT_INLINE VarBits* getLiveness() const { return _liveness; }
  //! Set variable liveness bits.
  ASMJIT_INLINE void setLiveness(VarBits* liveness) { _liveness = liveness; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Previous node.
  Node* _prev;
  //! Next node.
  Node* _next;

  //! Node type, see `kNodeType`.
  uint8_t _type;
  //! Operands count (if the node has operands, otherwise zero).
  uint8_t _opCount;
  //! Node flags, different meaning for every node type.
  uint16_t _flags;

  //! Flow index.
  uint32_t _flowId;

  //! Inline comment string, initially set to NULL.
  const char* _comment;

  //! Variable allocation instructions (initially NULL, filled by prepare
  //! phase).
  BaseVarInst* _varInst;

  //! Variable liveness bits (initially NULL, filled by analysis phase).
  VarBits* _liveness;

  //! Saved state.
  //!
  //! Initially NULL, not all nodes have saved state, only branch/flow control
  //! nodes.
  BaseVarState* _state;
};

// ============================================================================
// [asmjit::AlignNode]
// ============================================================================

//! Align node.
struct AlignNode : public Node {
  ASMJIT_NO_COPY(AlignNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AlignNode` instance.
  ASMJIT_INLINE AlignNode(BaseCompiler* compiler, uint32_t mode, uint32_t offset) :
    Node(compiler, kNodeTypeAlign) {

    _mode = mode;
    _offset = offset;
  }

  //! Destroy the `AlignNode` instance.
  ASMJIT_INLINE ~AlignNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get alignment mode.
  ASMJIT_INLINE uint32_t getMode() const {
    return _mode;
  }

  //! Set alignment mode.
  ASMJIT_INLINE void setMode(uint32_t mode) {
    _mode = mode;
  }

  //! Get align offset in bytes.
  ASMJIT_INLINE uint32_t getOffset() const {
    return _offset;
  }

  //! Set align offset in bytes to `offset`.
  ASMJIT_INLINE void setOffset(uint32_t offset) {
    _offset = offset;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Alignment mode, see \ref kAlignMode.
  uint32_t _mode;
  //! Alignment offset in bytes.
  uint32_t _offset;
};

// ============================================================================
// [asmjit::EmbedNode]
// ============================================================================

//! Embed node.
//!
//! Embed node is used to embed data into final assembler stream. The data is
//! considered to be RAW; No analysis is performed on RAW data.
struct EmbedNode : public Node {
  ASMJIT_NO_COPY(EmbedNode)

  // --------------------------------------------------------------------------
  // [Enums]
  // --------------------------------------------------------------------------

  enum { kInlineBufferSize = 8 };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `EmbedNode` instance.
  ASMJIT_INLINE EmbedNode(BaseCompiler* compiler, void* data, uint32_t size) : Node(compiler, kNodeTypeEmbed) {
    _size = size;
    if (size <= kInlineBufferSize) {
      if (data != NULL)
        ::memcpy(_data.buf, data, size);
    }
    else {
      _data.ptr = static_cast<uint8_t*>(data);
    }
  }

  //! Destroy the `EmbedNode` instance.
  ASMJIT_INLINE ~EmbedNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get pointer to data.
  uint8_t* getData() { return getSize() <= kInlineBufferSize ? const_cast<uint8_t*>(_data.buf) : _data.ptr; }
  //! Get size of data.
  uint32_t getSize() const { return _size; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Size of the embedded data.
  uint32_t _size;

  union {
    //! data buffer.
    uint8_t buf[kInlineBufferSize];
    //! Data buffer.
    uint8_t* ptr;
  } _data;
};

// ============================================================================
// [asmjit::CommentNode]
// ============================================================================

//! Comment node.
//!
//! Comments allows to comment your assembler stream for better debugging
//! and visualization. Comments are usually ignored in release builds unless
//! the logger is present.
struct CommentNode : public Node {
  ASMJIT_NO_COPY(CommentNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CommentNode` instance.
  ASMJIT_INLINE CommentNode(BaseCompiler* compiler, const char* comment) : Node(compiler, kNodeTypeComment) {
    _comment = comment;
  }

  //! Destroy the `CommentNode` instance.
  ASMJIT_INLINE ~CommentNode() {}
};

// ============================================================================
// [asmjit::HintNode]
// ============================================================================

//! Hint node.
struct HintNode : public Node {
  ASMJIT_NO_COPY(HintNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `HintNode` instance.
  ASMJIT_INLINE HintNode(BaseCompiler* compiler, VarData* vd, uint32_t hint, uint32_t value) : Node(compiler, kNodeTypeHint) {
    _vd = vd;
    _hint = hint;
    _value = value;
  }

  //! Destroy the `HintNode` instance.
  ASMJIT_INLINE ~HintNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get variable.
  ASMJIT_INLINE VarData* getVd() const { return _vd; }

  //! Get hint it (see `kVarHint)`.
  ASMJIT_INLINE uint32_t getHint() const{ return _hint; }
  //! Set hint it (see `kVarHint)`.
  ASMJIT_INLINE void setHint(uint32_t hint) { _hint = hint; }

  //! Get hint value.
  ASMJIT_INLINE uint32_t getValue() const { return _value; }
  //! Set hint value.
  ASMJIT_INLINE void setValue(uint32_t value) { _value = value; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Variable.
  VarData* _vd;
  //! Hint id.
  uint32_t _hint;
  //! Value.
  uint32_t _value;
};

// ============================================================================
// [asmjit::TargetNode]
// ============================================================================

//! label node.
struct TargetNode : public Node {
  ASMJIT_NO_COPY(TargetNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `TargetNode` instance.
  ASMJIT_INLINE TargetNode(BaseCompiler* compiler, uint32_t labelId) : Node(compiler, kNodeTypeTarget) {
    _id = labelId;
    _numRefs = 0;
    _from = NULL;
  }

  //! Destroy the `TargetNode` instance.
  ASMJIT_INLINE ~TargetNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get target label.
  ASMJIT_INLINE Label getLabel() const { return Label(_id); }
  //! Get target label id.
  ASMJIT_INLINE uint32_t getLabelId() const { return _id; }

  //! Get first jmp instruction.
  ASMJIT_INLINE JumpNode* getFrom() const { return _from; }

  //! Get whether the node has assigned state.
  ASMJIT_INLINE bool hasState() const { return _state != NULL; }
  //! Get state for this target.
  ASMJIT_INLINE BaseVarState* getState() const { return _state; }
  //! Set state for this target.
  ASMJIT_INLINE void setState(BaseVarState* state) { _state = state; }

  //! Get number of jumps to this target.
  ASMJIT_INLINE uint32_t getNumRefs() const { return _numRefs; }
  //! Set number of jumps to this target.
  ASMJIT_INLINE void setNumRefs(uint32_t i) { _numRefs = i; }

  //! Add number of jumps to this target.
  ASMJIT_INLINE void addNumRefs(uint32_t i = 1) { _numRefs += i; }
  //! Subtract number of jumps to this target.
  ASMJIT_INLINE void subNumRefs(uint32_t i = 1) { _numRefs -= i; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Label id.
  uint32_t _id;
  //! Count of jumps here.
  uint32_t _numRefs;

  //! First jump instruction that points to this target (label).
  JumpNode* _from;
};

// ============================================================================
// [asmjit::InstNode]
// ============================================================================

//! Instruction node.
struct InstNode : public Node {
  ASMJIT_NO_COPY(InstNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `InstNode` instance.
  ASMJIT_INLINE InstNode(BaseCompiler* compiler, uint32_t code, uint32_t options, Operand* opList, uint32_t opCount) : Node(compiler, kNodeTypeInst) {
    _code = static_cast<uint16_t>(code);
    _options = static_cast<uint8_t>(options);

    _opCount = static_cast<uint8_t>(opCount);
    _opList = opList;

    _updateMemOp();
  }

  //! Destroy the `InstNode` instance.
  ASMJIT_INLINE ~InstNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get instruction code, see `kInstCode`.
  ASMJIT_INLINE uint32_t getCode() const {
    return _code;
  }

  //! Set instruction code to `code`.
  //!
  //! Please do not modify instruction code if you are not know what you are
  //! doing. Incorrect instruction code or operands can raise assertion() at
  //! runtime.
  ASMJIT_INLINE void setCode(uint32_t code) {
    _code = static_cast<uint16_t>(code);
  }

  //! Whether the instruction is an unconditional jump or whether the
  //! instruction is a conditional jump which is likely to be taken.
  ASMJIT_INLINE bool isTaken() const {
    return hasFlag(kNodeFlagIsTaken);
  }

  //! Get emit options.
  ASMJIT_INLINE uint32_t getOptions() const {
    return _options;
  }
  //! Set emit options.
  ASMJIT_INLINE void setOptions(uint32_t options) {
    _options = static_cast<uint8_t>(options);
  }
  //! Add emit options.
  ASMJIT_INLINE void addOptions(uint32_t options) {
    _options |= static_cast<uint8_t>(options);
  }
  //! Mask emit options.
  ASMJIT_INLINE void andOptions(uint32_t options) {
    _options &= static_cast<uint8_t>(options);
  }
  //! Clear emit options.
  ASMJIT_INLINE void delOptions(uint32_t options) {
    _options &= static_cast<uint8_t>(~options);
  }

  //! Get operands list.
  ASMJIT_INLINE Operand* getOpList() {
    return _opList;
  }
  //! \overload
  ASMJIT_INLINE const Operand* getOpList() const {
    return _opList;
  }

  //! Get operands count.
  ASMJIT_INLINE uint32_t getOpCount() const {
    return _opCount;
  }

  //! Get whether the instruction contains a memory operand.
  ASMJIT_INLINE bool hasMemOp() const {
    return _memOpIndex != 0xFF;
  }

  //! Set memory operand index (in opList), 0xFF means that instruction
  //! doesn't have a memory operand.
  ASMJIT_INLINE void setMemOpIndex(uint32_t index) {
    _memOpIndex = static_cast<uint8_t>(index);
  }
  //! Reset memory operand index, setting it to 0xFF.
  ASMJIT_INLINE void resetMemOpIndex() {
    _memOpIndex = 0xFF;
  }

  //! Get memory operand.
  //!
  //! Can only be called if the instruction has such operand, see `hasMemOp()`.
  ASMJIT_INLINE BaseMem* getMemOp() const {
    ASMJIT_ASSERT(hasMemOp());
    return static_cast<BaseMem*>(&_opList[_memOpIndex]);
  }

  //! \overload
  template<typename T>
  ASMJIT_INLINE T* getMemOp() const {
    ASMJIT_ASSERT(hasMemOp());
    return static_cast<T*>(&_opList[_memOpIndex]);
  }

  // --------------------------------------------------------------------------
  // [Utils]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void _updateMemOp() {
    Operand* opList = getOpList();
    uint32_t opCount = getOpCount();

    uint32_t i;
    for (i = 0; i < opCount; i++)
      if (opList[i].isMem())
        goto _Update;
    i = 0xFF;

_Update:
    setMemOpIndex(i);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Instruction code, see `kInstCode`.
  uint16_t _code;
  //! Instruction options, see `kInstOptions`.
  uint8_t _options;
  //! \internal
  uint8_t _memOpIndex;

  //! Operands list.
  Operand* _opList;
};

// ============================================================================
// [asmjit::JumpNode]
// ============================================================================

//! Jump node.
struct JumpNode : public InstNode {
  ASMJIT_NO_COPY(JumpNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE JumpNode(BaseCompiler* compiler, uint32_t code, uint32_t options, Operand* opList, uint32_t opCount) :
    InstNode(compiler, code, options, opList, opCount) {}
  ASMJIT_INLINE ~JumpNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE TargetNode* getTarget() const { return _target; }
  ASMJIT_INLINE JumpNode* getJumpNext() const { return _jumpNext; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Target node.
  TargetNode* _target;
  //! Next jump to the same target in a single linked-list.
  JumpNode *_jumpNext;
};

// ============================================================================
// [asmjit::FuncNode]
// ============================================================================

//! Function declaration node.
//!
//! Functions are base blocks for generating assembler output. Each generated
//! assembler stream needs standard entry and leave sequences which are compatible
//! with the operating system ABI.
//!
//! `FuncNode` can be used to generate function prolog and epilog which are
//! compatible with a given function calling convention and to allocate and
//! manage variables that can be allocated/spilled during compilation phase.
struct FuncNode : public Node {
  ASMJIT_NO_COPY(FuncNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `FuncNode` instance.
  //!
  //! Always use `BaseCompiler::addFunc()` to create a `FuncNode` instance.
  ASMJIT_INLINE FuncNode(BaseCompiler* compiler) :
    Node(compiler, kNodeTypeFunc),
    _entryNode(NULL),
    _exitNode(NULL),
    _decl(NULL),
    _end(NULL),
    _argList(NULL),
    _funcHints(IntUtil::mask(kFuncHintNaked)),
    _funcFlags(0),
    _expectedStackAlignment(0),
    _requiredStackAlignment(0),
    _redZoneSize(0),
    _spillZoneSize(0),
    _argStackSize(0),
    _memStackSize(0),
    _callStackSize(0) {}

  //! Destroy the `FuncNode` instance.
  ASMJIT_INLINE ~FuncNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function entry `TargetNode`.
  ASMJIT_INLINE TargetNode* getEntryNode() const { return _entryNode; }
  //! Get function exit `TargetNode`.
  ASMJIT_INLINE TargetNode* getExitNode() const { return _exitNode; }

  //! Get function entry label.
  ASMJIT_INLINE Label getEntryLabel() const { return _entryNode->getLabel(); }
  //! Get function exit label.
  ASMJIT_INLINE Label getExitLabel() const { return _exitNode->getLabel(); }

  //! Get function `EndNode`.
  ASMJIT_INLINE EndNode* getEnd() const { return _end; }
  //! Get function declaration.
  ASMJIT_INLINE FuncDecl* getDecl() const { return _decl; }

  //! Get arguments list.
  ASMJIT_INLINE VarData** getArgList() const { return _argList; }
  //! Get arguments count.
  ASMJIT_INLINE uint32_t getArgCount() const { return _decl->getArgCount(); }

  //! Get argument at `i`.
  ASMJIT_INLINE VarData* getArg(uint32_t i) const {
    ASMJIT_ASSERT(i < getArgCount());
    return _argList[i];
  }

  //! Set argument at `i`.
  ASMJIT_INLINE void setArg(uint32_t i, VarData* vd) {
    ASMJIT_ASSERT(i < getArgCount());
    _argList[i] = vd;
  }

  //! Reset argument at `i`.
  ASMJIT_INLINE void resetArg(uint32_t i) {
    ASMJIT_ASSERT(i < getArgCount());
    _argList[i] = NULL;
  }

  //! Get function hints.
  ASMJIT_INLINE uint32_t getFuncHints() const { return _funcHints; }
  //! Get function flags.
  ASMJIT_INLINE uint32_t getFuncFlags() const { return _funcFlags; }

  //! Get whether the _funcFlags has `flag`
  ASMJIT_INLINE bool hasFuncFlag(uint32_t flag) const { return (_funcFlags & flag) != 0; }
  //! Set function `flag`.
  ASMJIT_INLINE void addFuncFlags(uint32_t flags) { _funcFlags |= flags; }
  //! Clear function `flag`.
  ASMJIT_INLINE void clearFuncFlags(uint32_t flags) { _funcFlags &= ~flags; }

  //! Get whether the function is naked.
  ASMJIT_INLINE bool isNaked() const { return hasFuncFlag(kFuncFlagIsNaked); }
  //! Get whether the function is also a caller.
  ASMJIT_INLINE bool isCaller() const { return hasFuncFlag(kFuncFlagIsCaller); }
  //! Get whether the required stack alignment is lower than expected one,
  //! thus it has to be aligned manually.
  ASMJIT_INLINE bool isStackMisaligned() const { return hasFuncFlag(kFuncFlagIsStackMisaligned); }
  //! Get whether the stack pointer is adjusted inside function prolog/epilog.
  ASMJIT_INLINE bool isStackAdjusted() const { return hasFuncFlag(kFuncFlagIsStackAdjusted); }

  //! Get whether the function is finished.
  ASMJIT_INLINE bool isFinished() const { return hasFuncFlag(kFuncFlagIsFinished); }

  //! Get expected stack alignment.
  ASMJIT_INLINE uint32_t getExpectedStackAlignment() const { return _expectedStackAlignment; }
  //! Set expected stack alignment.
  ASMJIT_INLINE void setExpectedStackAlignment(uint32_t alignment) { _expectedStackAlignment = alignment; }

  //! Get required stack alignment.
  ASMJIT_INLINE uint32_t getRequiredStackAlignment() const { return _requiredStackAlignment; }
  //! Set required stack alignment.
  ASMJIT_INLINE void setRequiredStackAlignment(uint32_t alignment) { _requiredStackAlignment = alignment; }

  //! Update required stack alignment so it's not lower than expected
  //! stack alignment.
  ASMJIT_INLINE void updateRequiredStackAlignment() {
    if (_requiredStackAlignment <= _expectedStackAlignment) {
      _requiredStackAlignment = _expectedStackAlignment;
      clearFuncFlags(kFuncFlagIsStackMisaligned);
    }
    else {
      addFuncFlags(kFuncFlagIsStackMisaligned);
    }
  }

  //! Set stack "Red Zone" size.
  ASMJIT_INLINE uint32_t getRedZoneSize() const { return _redZoneSize; }
  //! Get stack "Red Zone" size.
  ASMJIT_INLINE void setRedZoneSize(uint32_t s) { _redZoneSize = static_cast<uint16_t>(s); }

  //! Set stack "Spill Zone" size.
  ASMJIT_INLINE uint32_t getSpillZoneSize() const { return _spillZoneSize; }
  //! Get stack "Spill Zone" size.
  ASMJIT_INLINE void setSpillZoneSize(uint32_t s) { _spillZoneSize = static_cast<uint16_t>(s); }

  //! Get stack size used by function arguments.
  ASMJIT_INLINE uint32_t getArgStackSize() const { return _argStackSize; }

  //! Get stack size used by variables and memory allocated on the stack.
  ASMJIT_INLINE uint32_t getMemStackSize() const { return _memStackSize; }

  //! Get stack size used by function calls.
  ASMJIT_INLINE uint32_t getCallStackSize() const { return _callStackSize; }
  //! Merge stack size used by function call with `s`.
  ASMJIT_INLINE void mergeCallStackSize(uint32_t s) { if (_callStackSize < s) _callStackSize = s; }

  // --------------------------------------------------------------------------
  // [Hints]
  // --------------------------------------------------------------------------

  //! Set function hint.
  ASMJIT_INLINE void setHint(uint32_t hint, uint32_t value) {
    ASMJIT_ASSERT(hint <= 31);
    ASMJIT_ASSERT(value <= 1);

    _funcHints &= ~(1     << hint);
    _funcHints |=  (value << hint);
  }

  //! Get function hint.
  ASMJIT_INLINE uint32_t getHint(uint32_t hint) const {
    ASMJIT_ASSERT(hint <= 31);
    return (_funcHints >> hint) & 0x1;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Function entry.
  TargetNode* _entryNode;
  //! Function exit.
  TargetNode* _exitNode;

  //! Function declaration.
  FuncDecl* _decl;
  //! Function end.
  EndNode* _end;

  //! Arguments list as `VarData`.
  VarData** _argList;

  //! Function hints;
  uint32_t _funcHints;
  //! Function flags.
  uint32_t _funcFlags;

  //! Expected stack alignment (we depend on this value).
  //!
  //! \note It can be global alignment given by the OS or described by an
  //! target platform ABI.
  uint32_t _expectedStackAlignment;
  //! Required stack alignment (usually for multimedia instructions).
  uint32_t _requiredStackAlignment;

  //! The "Red Zone" suze - count of bytes which might be accessed
  //! without adjusting the stack pointer.
  uint16_t _redZoneSize;
  //! Spill zone size (zone used by WIN64ABI).
  uint16_t _spillZoneSize;

  //! Stack size needed for function arguments.
  uint32_t _argStackSize;
  //! Stack size needed for all variables and memory allocated on
  //! the stack.
  uint32_t _memStackSize;
  //! Stack size needed to call other functions.
  uint32_t _callStackSize;
};

// ============================================================================
// [asmjit::EndNode]
// ============================================================================

//! End of function/block node.
struct EndNode : public Node {
  ASMJIT_NO_COPY(EndNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `EndNode` instance.
  ASMJIT_INLINE EndNode(BaseCompiler* compiler) : Node(compiler, kNodeTypeEnd) {
    _flags |= kNodeFlagIsRet;
  }

  //! Destroy the `EndNode` instance.
  ASMJIT_INLINE ~EndNode() {}
};

// ============================================================================
// [asmjit::RetNode]
// ============================================================================

//! Function return node.
struct RetNode : public Node {
  ASMJIT_NO_COPY(RetNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `RetNode` instance.
  ASMJIT_INLINE RetNode(BaseCompiler* compiler, const Operand& o0, const Operand& o1) : Node(compiler, kNodeTypeRet) {
    _flags |= kNodeFlagIsRet;
    _ret[0] = o0;
    _ret[1] = o1;
  }

  //! Destroy the `RetNode` instance.
  ASMJIT_INLINE ~RetNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the first return operand.
  ASMJIT_INLINE Operand& getFirst() { return _ret[0]; }
  //! \overload
  ASMJIT_INLINE const Operand& getFirst() const { return _ret[0]; }

  //! Get the second return operand.
  ASMJIT_INLINE Operand& getSecond() { return _ret[1]; }
   //! \overload
  ASMJIT_INLINE const Operand& getSecond() const { return _ret[1]; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Ret operand(s).
  Operand _ret[2];
};

// ============================================================================
// [asmjit::CallNode]
// ============================================================================

//! Function-call node.
struct CallNode : public Node {
  ASMJIT_NO_COPY(CallNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CallNode` instance.
  ASMJIT_INLINE CallNode(BaseCompiler* compiler, const Operand& target) :
    Node(compiler, kNodeTypeCall),
    _decl(NULL),
    _target(target),
    _args(NULL) {}

  //! Destroy the `CallNode` instance.
  ASMJIT_INLINE ~CallNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function declaration.
  ASMJIT_INLINE FuncDecl* getDecl() const { return _decl; }

  //! Get target operand.
  ASMJIT_INLINE Operand& getTarget() { return _target; }
  //! \overload
  ASMJIT_INLINE const Operand& getTarget() const  { return _target; }

  //! Get return at `i`.
  ASMJIT_INLINE Operand& getRet(uint32_t i = 0) {
    ASMJIT_ASSERT(i < 2);
    return _ret[i];
  }
  //! \overload
  ASMJIT_INLINE const Operand& getRet(uint32_t i = 0) const  {
    ASMJIT_ASSERT(i < 2);
    return _ret[i];
  }

  //! Get argument at `i`.
  ASMJIT_INLINE Operand& getArg(uint32_t i) {
    ASMJIT_ASSERT(i < kFuncArgCountLoHi);
    return _args[i];
  }
  //! \overload
  ASMJIT_INLINE const Operand& getArg(uint32_t i) const  {
    ASMJIT_ASSERT(i < kFuncArgCountLoHi);
    return _args[i];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Function declaration.
  FuncDecl* _decl;

  //! Target (address of function, register, label, ...).
  Operand _target;
  //! Return.
  Operand _ret[2];
  //! Arguments.
  Operand* _args;
};

// ============================================================================
// [asmjit::SArgNode]
// ============================================================================

//! Function-call 'argument on the stack' node.
struct SArgNode : public Node {
  ASMJIT_NO_COPY(SArgNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `SArgNode` instance.
  ASMJIT_INLINE SArgNode(BaseCompiler* compiler, CallNode* call, VarData* sVd, VarData* cVd) :
    Node(compiler, kNodeTypeSArg),
    _call(call),
    _sVd(sVd),
    _cVd(cVd),
    _args(0) {}

  //! Destroy the `SArgNode` instance.
  ASMJIT_INLINE ~SArgNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the associated function-call.
  ASMJIT_INLINE CallNode* getCall() const { return _call; }
  //! Get source variable.
  ASMJIT_INLINE VarData* getSVd() const { return _sVd; }
  //! Get conversion variable.
  ASMJIT_INLINE VarData* getCVd() const { return _cVd; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Associated `CallNode`.
  CallNode* _call;
  //! Source variable.
  VarData* _sVd;
  //! Temporary variable used for conversion (or NULL).
  VarData* _cVd;

  //! Affected arguments bit-array.
  uint32_t _args;
};

//! \}

// ============================================================================
// [asmjit::BaseCompiler]
// ============================================================================

//! \addtogroup asmjit_base_general
//! \{

//! Base compiler.
//!
//! @sa BaseAssembler.
struct BaseCompiler : public CodeGen {
  ASMJIT_NO_COPY(BaseCompiler)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `BaseCompiler` instance.
  ASMJIT_API BaseCompiler(Runtime* runtime);
  //! Destroy the `BaseCompiler` instance.
  ASMJIT_API virtual ~BaseCompiler();

  // --------------------------------------------------------------------------
  // [LookAhead]
  // --------------------------------------------------------------------------

  //! Get maximum look ahead.
  ASMJIT_INLINE uint32_t getMaxLookAhead() const { return _maxLookAhead; }
  //! Set maximum look ahead to `val`.
  ASMJIT_INLINE void setMaxLookAhead(uint32_t val) { _maxLookAhead = val; }

  // --------------------------------------------------------------------------
  // [Clear / Reset]
  // --------------------------------------------------------------------------

  //! Clear everything, but keep buffers allocated.
  //!
  //! \note This method will destroy your code.
  ASMJIT_API void clear();
  //! Clear everything and reset all buffers.
  //!
  //! \note This method will destroy your code.
  ASMJIT_API void reset();
  //! Called by clear() and reset() to clear all data related to derived
  //! class implementation.
  ASMJIT_API virtual void _purge();

  // --------------------------------------------------------------------------
  // [Nodes]
  // --------------------------------------------------------------------------

  template<typename T>
  ASMJIT_INLINE T* newNode() {
    void* p = _baseZone.alloc(sizeof(T));
    return new(p) T(this);
  }

  template<typename T, typename P0>
  ASMJIT_INLINE T* newNode(P0 p0) {
    void* p = _baseZone.alloc(sizeof(T));
    return new(p) T(this, p0);
  }

  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE T* newNode(P0 p0, P1 p1) {
    void* p = _baseZone.alloc(sizeof(T));
    return new(p) T(this, p0, p1);
  }

  template<typename T, typename P0, typename P1, typename P2>
  ASMJIT_INLINE T* newNode(P0 p0, P1 p1, P2 p2) {
    void* p = _baseZone.alloc(sizeof(T));
    return new(p) T(this, p0, p1, p2);
  }

  //! Get first node.
  ASMJIT_INLINE Node* getFirstNode() const { return _firstNode; }
  //! Get last node.
  ASMJIT_INLINE Node* getLastNode() const { return _lastNode; }

  //! Get current node.
  //!
  //! \note If this method returns `NULL` it means that nothing has been emitted
  //! yet.
  ASMJIT_INLINE Node* getCursor() const { return _cursor; }
  //! Set the current node without returning the previous node (private).
  ASMJIT_INLINE void _setCursor(Node* node) { _cursor = node; }
  //! Set the current node to `node` and return the previous one.
  ASMJIT_API Node* setCursor(Node* node);

  //! Add node `node` after current and set current to `node`.
  ASMJIT_API Node* addNode(Node* node);
  //! Add node before `ref`.
  ASMJIT_API Node* addNodeBefore(Node* node, Node* ref);
  //! Add node after `ref`.
  ASMJIT_API Node* addNodeAfter(Node* node, Node* ref);
  //! Remove node `node`.
  ASMJIT_API Node* removeNode(Node* node);
  //! Remove multiple nodes.
  ASMJIT_API void removeNodes(Node* first, Node* last);

  // --------------------------------------------------------------------------
  // [Func]
  // --------------------------------------------------------------------------

  //! Get current function.
  ASMJIT_INLINE FuncNode* getFunc() const { return _func; }

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  //! Create a new `AlignNode`.
  ASMJIT_API AlignNode* newAlign(uint32_t mode, uint32_t offset);
  //! Add a new `AlignNode`.
  ASMJIT_API AlignNode* addAlign(uint32_t mode, uint32_t offset);

  //! Align target buffer to `m` bytes.
  //!
  //! Typical usage of this is to align labels at start of the inner loops.
  //!
  //! Inserts `nop()` instructions or CPU optimized NOPs.
  ASMJIT_INLINE AlignNode* align(uint32_t mode, uint32_t offset) {
    return addAlign(mode, offset);
  }

  // --------------------------------------------------------------------------
  // [Target]
  // --------------------------------------------------------------------------

  //! Create a new `TargetNode`.
  ASMJIT_API TargetNode* newTarget();
  //! Add a new `TargetNode`.
  ASMJIT_API TargetNode* addTarget();

  //! Get `TargetNode` by `id`.
  ASMJIT_INLINE TargetNode* getTargetById(uint32_t id) {
    ASMJIT_ASSERT(OperandUtil::isLabelId(id));
    ASMJIT_ASSERT(id < _targets.getLength());

    return _targets[id];
  }

  //! Get `TargetNode` by `label`.
  ASMJIT_INLINE TargetNode* getTarget(const Label& label) {
    return getTargetById(label.getId());
  }

  // --------------------------------------------------------------------------
  // [Label]
  // --------------------------------------------------------------------------

  //! Get count of created labels.
  ASMJIT_INLINE size_t getLabelsCount() const {
    return _targets.getLength();
  }

  //! Get whether `label` is created.
  ASMJIT_INLINE bool isLabelCreated(const Label& label) const {
    return static_cast<size_t>(label.getId()) < _targets.getLength();
  }

  //! \internal
  //!
  //! Create and initialize a new `Label`.
  ASMJIT_API Error _newLabel(Label* dst);

  //! Create and return a new `Label`.
  ASMJIT_INLINE Label newLabel() {
    Label result(NoInit);
    _newLabel(&result);
    return result;
  }

  //! Bind label to the current offset.
  //!
  //! \note Label can be bound only once!
  ASMJIT_API void bind(const Label& label);

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! Create a new `EmbedNode`.
  ASMJIT_API EmbedNode* newEmbed(const void* data, uint32_t size);
  //! Add a new `EmbedNode`.
  ASMJIT_API EmbedNode* addEmbed(const void* data, uint32_t size);

  //! Embed data.
  ASMJIT_INLINE EmbedNode* embed(const void* data, uint32_t size) {
    return addEmbed(data, size);
  }

  // --------------------------------------------------------------------------
  // [Comment]
  // --------------------------------------------------------------------------

  //! Create a new `CommentNode`.
  ASMJIT_API CommentNode* newComment(const char* str);
  //! Add a new `CommentNode`.
  ASMJIT_API CommentNode* addComment(const char* str);

  //! Emit a single comment line.
  ASMJIT_API CommentNode* comment(const char* fmt, ...);

  // --------------------------------------------------------------------------
  // [Hint]
  // --------------------------------------------------------------------------

  //! Create a new `HintNode`.
  ASMJIT_API HintNode* newHint(BaseVar& var, uint32_t hint, uint32_t value);
  //! Add a new `HintNode`.
  ASMJIT_API HintNode* addHint(BaseVar& var, uint32_t hint, uint32_t value);

  // --------------------------------------------------------------------------
  // [Vars]
  // --------------------------------------------------------------------------

  //! Get whether variable `var` is created.
  ASMJIT_INLINE bool isVarCreated(const BaseVar& var) const {
    return static_cast<size_t>(var.getId() & kOperandIdNum) < _vars.getLength();
  }

  //! \internal
  //!
  //! Get `VarData` by `var`.
  ASMJIT_INLINE VarData* getVd(const BaseVar& var) const {
    return getVdById(var.getId());
  }

  //! \internal
  //!
  //! Get `VarData` by `id`.
  ASMJIT_INLINE VarData* getVdById(uint32_t id) const {
    ASMJIT_ASSERT(id != kInvalidValue);
    ASMJIT_ASSERT(static_cast<size_t>(id & kOperandIdNum) < _vars.getLength());

    return _vars[id & kOperandIdNum];
  }

  //! \internal
  //!
  //! Get an array of 'VarData*'.
  ASMJIT_INLINE VarData** _getVdArray() const {
    return const_cast<VarData**>(_vars.getData());
  }

  //! \internal
  //!
  //! Create a new `VarData`.
  ASMJIT_API VarData* _newVd(uint32_t type, uint32_t size, uint32_t c, const char* name);

  //! Create a new `BaseVar`.
  virtual Error _newVar(BaseVar* var, uint32_t type, const char* name) = 0;

  //! Alloc variable `var`.
  ASMJIT_API void alloc(BaseVar& var);
  //! Alloc variable `var` using `regIndex` as a register index.
  ASMJIT_API void alloc(BaseVar& var, uint32_t regIndex);
  //! Alloc variable `var` using `reg` as a demanded register.
  ASMJIT_API void alloc(BaseVar& var, const BaseReg& reg);
  //! Spill variable `var`.
  ASMJIT_API void spill(BaseVar& var);
  //! Save variable `var` if modified.
  ASMJIT_API void save(BaseVar& var);
  //! Unuse variable `var`.
  ASMJIT_API void unuse(BaseVar& var);

  //! Get priority of variable `var`.
  ASMJIT_API uint32_t getPriority(BaseVar& var) const;
  //! Set priority of variable `var` to `priority`.
  ASMJIT_API void setPriority(BaseVar& var, uint32_t priority);

  //! Get save-on-unuse `var` property.
  ASMJIT_API bool getSaveOnUnuse(BaseVar& var) const;
  //! Set save-on-unuse `var` property to `value`.
  ASMJIT_API void setSaveOnUnuse(BaseVar& var, bool value);

  //! Rename variable `var` to `name`.
  //!
  //! \note Only new name will appear in the logger.
  ASMJIT_API void rename(BaseVar& var, const char* name);

  // --------------------------------------------------------------------------
  // [Stack]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Create a new memory chunk allocated on the current function's stack.
  virtual Error _newStack(BaseMem* mem, uint32_t size, uint32_t alignment, const char* name) = 0;

  // --------------------------------------------------------------------------
  // [Const]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Put data to a constant-pool and get a memory reference to it.
  virtual Error _newConst(BaseMem* mem, uint32_t scope, const void* data, size_t size) = 0;

  // --------------------------------------------------------------------------
  // [Serialize]
  // --------------------------------------------------------------------------

  //! Send assembled code to `assembler`.
  virtual Error serialize(BaseAssembler& assembler) = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Flow id added to each node created (used only by `Context)`.
  uint32_t _nodeFlowId;
  //! Flags added to each node created (used only by `Context)`.
  uint32_t _nodeFlags;
  //! Maximum count of nodes to look ahead when allocating/spilling
  //! registers.
  uint32_t _maxLookAhead;

  //! Variable mapping (translates incoming kVarType into target).
  const uint8_t* _targetVarMapping;

  //! First node.
  Node* _firstNode;
  //! Last node.
  Node* _lastNode;

  //! Current node.
  Node* _cursor;
  //! Current function.
  FuncNode* _func;

  //! Variable zone.
  Zone _varZone;
  //! String/data zone.
  Zone _stringZone;
  //! Local constant pool zone.
  Zone _localConstZone;

  //! Targets.
  PodVector<TargetNode*> _targets;
  //! Variables.
  PodVector<VarData*> _vars;

  //! Local constant pool, flushed at the end of each function.
  ConstPool _localConstPool;
  //! Global constant pool, flushed at the end of the compilation.
  ConstPool _globalConstPool;

  //! Label to start of the local constant pool.
  Label _localConstPoolLabel;
  //! Label to start of the global constant pool.
  Label _globalConstPoolLabel;
};

//! \}

// ============================================================================
// [Defined-Later]
// ============================================================================

ASMJIT_INLINE Label::Label(BaseCompiler& c) : Operand(NoInit) {
  c._newLabel(this);
}

ASMJIT_INLINE Node::Node(BaseCompiler* compiler, uint32_t type) {
  _prev = NULL;
  _next = NULL;
  _type = static_cast<uint8_t>(type);
  _opCount = 0;
  _flags = static_cast<uint16_t>(compiler->_nodeFlags);
  _flowId = compiler->_nodeFlowId;
  _comment = NULL;
  _varInst = NULL;
  _liveness = NULL;
  _state = NULL;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_COMPILER_H
