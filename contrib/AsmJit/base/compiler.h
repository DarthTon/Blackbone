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
#include "../base/containers.h"
#include "../base/error.h"
#include "../base/intutil.h"
#include "../base/operand.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Compiler;

struct VarAttr;
struct VarData;
struct VarMap;
struct VarState;

struct Node;
struct EndNode;
struct InstNode;
struct JumpNode;

// ============================================================================
// [asmjit::kConstScope]
// ============================================================================

//! \addtogroup asmjit_base_compiler
//! \{

//! Scope of the constant.
ASMJIT_ENUM(kConstScope) {
  //! Local constant, always embedded right after the current function.
  kConstScopeLocal = 0,
  //! Global constant, embedded at the end of the currently compiled code.
  kConstScopeGlobal = 1
};

// ============================================================================
// [asmjit::kVarType]
// ============================================================================

ASMJIT_ENUM(kVarType) {
  //! Variable is 8-bit signed integer.
  kVarTypeInt8 = 0,
  //! Variable is 8-bit unsigned integer.
  kVarTypeUInt8 = 1,
  //! Variable is 16-bit signed integer.
  kVarTypeInt16 = 2,
  //! Variable is 16-bit unsigned integer.
  kVarTypeUInt16 = 3,
  //! Variable is 32-bit signed integer.
  kVarTypeInt32 = 4,
  //! Variable is 32-bit unsigned integer.
  kVarTypeUInt32 = 5,
  //! Variable is 64-bit signed integer.
  kVarTypeInt64 = 6,
  //! Variable is 64-bit unsigned integer.
  kVarTypeUInt64 = 7,

  //! Variable is target `intptr_t`, not compatible with host `intptr_t`.
  kVarTypeIntPtr = 8,
  //! Variable is target `uintptr_t`, not compatible with host `uintptr_t`.
  kVarTypeUIntPtr = 9,

  //! Variable is 32-bit floating point (single precision).
  kVarTypeFp32 = 10,
  //! Variable is 64-bit floating point (double precision).
  kVarTypeFp64 = 11,

  //! \internal
  _kVarTypeIntStart = kVarTypeInt8,
  //! \internal
  _kVarTypeIntEnd = kVarTypeUIntPtr,

  //! \internal
  _kVarTypeFpStart = kVarTypeFp32,
  //! \internal
  _kVarTypeFpEnd = kVarTypeFp64
};

// ============================================================================
// [asmjit::kVarFlags]
// ============================================================================

//! \internal
//!
//! X86/X64 variable flags.
ASMJIT_ENUM(kVarFlags) {
  //! Variable contains single-precision floating-point(s).
  kVarFlagSp = 0x10,
  //! Variable contains double-precision floating-point(s).
  kVarFlagDp = 0x20,
  //! Variable is packed, i.e. packed floats, doubles, ...
  kVarFlagPacked = 0x40
};

// ============================================================================
// [asmjit::kVarAttrFlags]
// ============================================================================

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
  //! Variable should be spilled.
  kVarAttrSpill = 0x00000800,
  //! Variable should be unused at the end of the instruction/node.
  kVarAttrUnuse = 0x00001000,

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

//! Variable hint (used by `Compiler)`.
//!
//! \sa Compiler.
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
// [asmjit::kFuncConv]
// ============================================================================

//! Function calling convention.
//!
//! For a platform specific calling conventions, see:
//!   - `kX86FuncConv` - X86/X64 calling conventions.
ASMJIT_ENUM(kFuncConv) {
  //! Calling convention is invalid (can't be used).
  kFuncConvNone = 0,

#if defined(ASMJIT_DOCGEN)
  //! Default calling convention for current platform / operating system.
  kFuncConvHost = DependsOnHost,

  //! Default C calling convention based on current compiler's settings.
  kFuncConvHostCDecl = DependsOnHost,

  //! Compatibility for `__stdcall` calling convention.
  //!
  //! \note This enumeration is always set to a value which is compatible with
  //! current compilers __stdcall calling convention. In 64-bit mode the value
  //! is compatible with `kX86FuncConvW64` or `kX86FuncConvU64`.
  kFuncConvHostStdCall = DependsOnHost,

  //! Compatibility for `__fastcall` calling convention.
  //!
  //! \note This enumeration is always set to a value which is compatible with
  //! current compilers `__fastcall` calling convention. In 64-bit mode the value
  //! is compatible with `kX86FuncConvW64` or `kX86FuncConvU64`.
  kFuncConvHostFastCall = DependsOnHost
#endif // ASMJIT_DOCGEN
};

// ============================================================================
// [asmjit::kFuncHint]
// ============================================================================

//! Function hints.
//!
//! For a platform specific calling conventions, see:
//!   - `kX86FuncHint` - X86/X64 function hints.
ASMJIT_ENUM(kFuncHint) {
  //! Make a naked function (default true).
  //!
  //! Naked function is function without using standard prolog/epilog sequence).
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Standard prolog sequence is:
  //!
  //! ~~~
  //! push zbp
  //! mov zsp, zbp
  //! sub zsp, StackAdjustment
  //! ~~~
  //!
  //! which is an equivalent to:
  //!
  //! ~~~
  //! enter StackAdjustment, 0
  //! ~~~
  //!
  //! Standard epilog sequence is:
  //!
  //! ~~~
  //! mov zsp, zbp
  //! pop zbp
  //! ~~~
  //!
  //! which is an equavalent to:
  //!
  //! ~~~
  //! leave
  //! ~~~
  //!
  //! Naked functions can omit the prolog/epilog sequence. The advantage of
  //! doing such modification is that EBP/RBP register can be used by the
  //! register allocator which can result in less spills/allocs.
  kFuncHintNaked = 0,

  //! Generate compact function prolog/epilog if possible.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Use shorter, but possible slower prolog/epilog sequence to save/restore
  //! registers.
  kFuncHintCompact = 1
};

// ============================================================================
// [asmjit::kFuncFlags]
// ============================================================================

//! Function flags.
//!
//! For a platform specific calling conventions, see:
//!   - `kX86FuncFlags` - X86/X64 function flags.
ASMJIT_ENUM(kFuncFlags) {
  //! Whether the function is using naked (minimal) prolog / epilog.
  kFuncFlagIsNaked = 0x00000001,

  //! Whether an another function is called from this function.
  kFuncFlagIsCaller = 0x00000002,

  //! Whether the stack is not aligned to the required stack alignment,
  //! thus it has to be aligned manually.
  kFuncFlagIsStackMisaligned = 0x00000004,

  //! Whether the stack pointer is adjusted by the stack size needed
  //! to save registers and function variables.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Stack pointer (ESP/RSP) is adjusted by 'sub' instruction in prolog and by
  //! 'add' instruction in epilog (only if function is not naked). If function
  //! needs to perform manual stack alignment more instructions are used to
  //! adjust the stack (like "and zsp, -Alignment").
  kFuncFlagIsStackAdjusted = 0x00000008,

  //! Whether the function is finished using `Compiler::endFunc()`.
  kFuncFlagIsFinished = 0x80000000
};

// ============================================================================
// [asmjit::kFuncDir]
// ============================================================================

//! Function arguments direction.
ASMJIT_ENUM(kFuncDir) {
  //! Arguments are passed left to right.
  //!
  //! This arguments direction is unusual in C, however it's used in Pascal.
  kFuncDirLtr = 0,

  //! Arguments are passed right ro left
  //!
  //! This is the default argument direction in C.
  kFuncDirRtl = 1
};

// ============================================================================
// [asmjit::kFuncArg]
// ============================================================================

//! Function argument (lo/hi) specification.
ASMJIT_ENUM(kFuncArg) {
  //! Maxumum number of function arguments supported by AsmJit.
  kFuncArgCount = 16,
  //! Extended maximum number of arguments (used internally).
  kFuncArgCountLoHi = kFuncArgCount * 2,

  //! Index to the LO part of function argument (default).
  //!
  //! This value is typically omitted and added only if there is HI argument
  //! accessed.
  kFuncArgLo = 0,
  //! Index to the HI part of function argument.
  //!
  //! HI part of function argument depends on target architecture. On x86 it's
  //! typically used to transfer 64-bit integers (they form a pair of 32-bit
  //! integers).
  kFuncArgHi = kFuncArgCount
};

// ============================================================================
// [asmjit::kFuncRet]
// ============================================================================

//! Function return value (lo/hi) specification.
ASMJIT_ENUM(kFuncRet) {
  //! Index to the LO part of function return value.
  kFuncRetLo = 0,
  //! Index to the HI part of function return value.
  kFuncRetHi = 1
};

// ============================================================================
// [asmjit::kFuncStackInvalid]
// ============================================================================

enum kFuncMisc {
  //! Invalid stack offset in function or function parameter.
  kFuncStackInvalid = -1
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
// [asmjit::kNodeFlags]
// ============================================================================

ASMJIT_ENUM(kNodeFlags) {
  //! Whether the node has been translated, thus contains only registers.
  kNodeFlagIsTranslated = 0x0001,

  //! Whether the node was scheduled - possibly reordered, but basically this
  //! is a mark that is set by scheduler after the node has been visited.
  kNodeFlagIsScheduled = 0x0002,

  //! Whether the node is informative only and can be safely removed.
  kNodeFlagIsInformative = 0x0004,

  //! Whether the `InstNode` is a jump.
  kNodeFlagIsJmp = 0x0008,
  //! Whether the `InstNode` is a conditional jump.
  kNodeFlagIsJcc = 0x0010,

  //! Whether the `InstNode` is an unconditinal jump or conditional
  //! jump that is likely to be taken.
  kNodeFlagIsTaken = 0x0020,

  //! Whether the `Node` will return from a function.
  //!
  //! This flag is used by both `EndNode` and `RetNode`.
  kNodeFlagIsRet = 0x0040,

  //! Whether the instruction is special.
  kNodeFlagIsSpecial = 0x0080,

  //! Whether the instruction is an FPU instruction.
  kNodeFlagIsFp = 0x0100
};

// ============================================================================
// [asmjit::MemCell]
// ============================================================================

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
// [asmjit::Var]
// ============================================================================

//! Base class for all variables.
struct Var : public Operand {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Var() : Operand(NoInit) {
    _init_packed_op_sz_b0_b1_id(kOperandTypeVar, 0, 0, 0, kInvalidValue);
    _init_packed_d2_d3(kInvalidValue, kInvalidValue);
  }

  ASMJIT_INLINE Var(const Var& other) : Operand(other) {}

  explicit ASMJIT_INLINE Var(const _NoInit&) : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Var Specific]
  // --------------------------------------------------------------------------

  //! Clone `Var` operand.
  ASMJIT_INLINE Var clone() const {
    return Var(*this);
  }

  //! Reset Var operand.
  ASMJIT_INLINE void reset() {
    _init_packed_op_sz_b0_b1_id(kOperandTypeVar, 0, kInvalidReg, kInvalidReg, kInvalidValue);
    _init_packed_d2_d3(kInvalidValue, kInvalidValue);
  }

  //! Get whether the variable has been initialized by `Compiler`.
  ASMJIT_INLINE bool isInitialized() const {
    return _vreg.id != kInvalidValue;
  }

  //! Get variable type.
  ASMJIT_INLINE uint32_t getVarType() const {
    return _vreg.vType;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Var& operator=(const Var& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const Var& other) const { return _packed[0] == other._packed[0]; }
  ASMJIT_INLINE bool operator!=(const Var& other) const { return !operator==(other); }
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
  // [Accessors - Base]
  // --------------------------------------------------------------------------

  //! Get variable name.
  ASMJIT_INLINE const char* getName() const {
    return _name;
  }

  //! Get variable id.
  ASMJIT_INLINE uint32_t getId() const {
    return _id;
  }

  //! Get variable type.
  ASMJIT_INLINE uint32_t getType() const {
    return _type;
  }

  //! Get variable class.
  ASMJIT_INLINE uint32_t getClass() const {
    return _class;
  }

  // --------------------------------------------------------------------------
  // [Accessors - ContextId]
  // --------------------------------------------------------------------------

  //! Get whether the variable has context id.
  ASMJIT_INLINE bool hasContextId() const {
    return _contextId != kInvalidValue;
  }

  //! Get context variable id (used only by `Context)`.
  ASMJIT_INLINE uint32_t getContextId() const {
    return _contextId;
  }

  //! Set context variable id (used only by `Context)`.
  ASMJIT_INLINE void setContextId(uint32_t contextId) {
    _contextId = contextId;
  }

  //! Reset context variable id (used only by `Context)`.
  ASMJIT_INLINE void resetContextId() {
    _contextId = kInvalidValue;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Priority]
  // --------------------------------------------------------------------------

  //! Get variable priority, used by compiler to decide which variable to spill.
  ASMJIT_INLINE uint32_t getPriority() const {
    return _priority;
  }

  //! Set variable priority.
  ASMJIT_INLINE void setPriority(uint32_t priority) {
    ASMJIT_ASSERT(priority <= 0xFF);
    _priority = static_cast<uint8_t>(priority);
  }

  // --------------------------------------------------------------------------
  // [Accessors - State]
  // --------------------------------------------------------------------------

  //! Get variable state, only used by `Context`.
  ASMJIT_INLINE uint32_t getState() const {
    return _state;
  }

  //! Set variable state, only used by `Context`.
  ASMJIT_INLINE void setState(uint32_t state) {
    ASMJIT_ASSERT(state <= 0xFF);
    _state = static_cast<uint8_t>(state);
  }

  // --------------------------------------------------------------------------
  // [Accessors - RegIndex]
  // --------------------------------------------------------------------------

  //! Get register index.
  ASMJIT_INLINE uint32_t getRegIndex() const {
    return _regIndex;
  }

  //! Set register index.
  ASMJIT_INLINE void setRegIndex(uint32_t regIndex) {
    ASMJIT_ASSERT(regIndex <= 0xFF);
    _regIndex = static_cast<uint8_t>(regIndex);
  }

  //! Reset register index.
  ASMJIT_INLINE void resetRegIndex() {
    _regIndex = static_cast<uint8_t>(kInvalidReg);
  }

  // --------------------------------------------------------------------------
  // [Accessors - HomeIndex/Mask]
  // --------------------------------------------------------------------------

  //! Get home registers mask.
  ASMJIT_INLINE uint32_t getHomeMask() const {
    return _homeMask;
  }

  //! Add a home register index to the home registers mask.
  ASMJIT_INLINE void addHomeIndex(uint32_t regIndex) {
    _homeMask |= IntUtil::mask(regIndex);
  }

  // --------------------------------------------------------------------------
  // [Accessors - Flags]
  // --------------------------------------------------------------------------

  //! Get variable flags.
  ASMJIT_INLINE uint32_t getFlags() const {
    return _flags;
  }

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

  //! Variable state (connected with actual `VarState)`.
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
  //! Whether variable was changed (connected with actual `VarState)`.
  uint8_t _modified : 1;
  //! \internal
  uint8_t _reserved0 : 3;
  //! Variable natural alignment.
  uint8_t _alignment;

  //! Variable size.
  uint32_t _size;

  //! Mask of all registers variable has been allocated to.
  uint32_t _homeMask;

  //! Home memory offset.
  int32_t _memOffset;
  //! Home memory cell, used by `Context` (initially NULL).
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
  // initialized by init() phase and reset by cleanup() phase.

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

  //! Get VarData.
  ASMJIT_INLINE VarData* getVd() const { return _vd; }
  //! Set VarData.
  ASMJIT_INLINE void setVd(VarData* vd) { _vd = vd; }

  //! Get flags.
  ASMJIT_INLINE uint32_t getFlags() const { return _flags; }
  //! Set flags.
  ASMJIT_INLINE void setFlags(uint32_t flags) { _flags = flags; }

  //! Get whether `flag` is on.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) { return (_flags & flag) != 0; }
  //! Add `flags`.
  ASMJIT_INLINE void orFlags(uint32_t flags) { _flags |= flags; }
  //! Mask `flags`.
  ASMJIT_INLINE void andFlags(uint32_t flags) { _flags &= flags; }
  //! Clear `flags`.
  ASMJIT_INLINE void andNotFlags(uint32_t flags) { _flags &= ~flags; }

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
// [asmjit::VarMap]
// ============================================================================

//! Variables' map related to a single node (instruction / other node).
struct VarMap {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get count of variables (all).
  ASMJIT_INLINE uint32_t getVaCount() const {
    return _vaCount;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Variables count.
  uint32_t _vaCount;
};

// ============================================================================
// [asmjit::VarState]
// ============================================================================

//! Variables' state.
struct VarState {};

// ============================================================================
// [asmjit::TypeId / VarMapping]
// ============================================================================

//! Function builder 'void' type.
struct Void {};

//! Function builder 'int8_t' type.
struct Int8Type {};
//! Function builder 'uint8_t' type.
struct UInt8Type {};

//! Function builder 'int16_t' type.
struct Int16Type {};
//! Function builder 'uint16_t' type.
struct UInt16Type {};

//! Function builder 'int32_t' type.
struct Int32Type {};
//! Function builder 'uint32_t' type.
struct UInt32Type {};

//! Function builder 'int64_t' type.
struct Int64Type {};
//! Function builder 'uint64_t' type.
struct UInt64Type {};

//! Function builder 'intptr_t' type.
struct IntPtrType {};
//! Function builder 'uintptr_t' type.
struct UIntPtrType {};

//! Function builder 'float' type.
struct FloatType {};
//! Function builder 'double' type.
struct DoubleType {};

#if !defined(ASMJIT_DOCGEN)
template<typename T>
struct TypeId {
  enum { kId = static_cast<int>(::asmjit::kInvalidVar) };
};

template<typename T>
struct TypeId<T*> {
  enum { kId = kVarTypeIntPtr };
};

#define ASMJIT_TYPE_ID(_T_, _Id_) \
  template<> \
  struct TypeId<_T_> { enum { kId = _Id_ }; }

ASMJIT_TYPE_ID(void, kInvalidVar);
ASMJIT_TYPE_ID(Void, kInvalidVar);

ASMJIT_TYPE_ID(int8_t, kVarTypeInt8);
ASMJIT_TYPE_ID(Int8Type, kVarTypeInt8);

ASMJIT_TYPE_ID(uint8_t, kVarTypeUInt8);
ASMJIT_TYPE_ID(UInt8Type, kVarTypeUInt8);

ASMJIT_TYPE_ID(int16_t, kVarTypeInt16);
ASMJIT_TYPE_ID(Int16Type, kVarTypeInt16);

ASMJIT_TYPE_ID(uint16_t, kVarTypeUInt8);
ASMJIT_TYPE_ID(UInt16Type, kVarTypeUInt8);

ASMJIT_TYPE_ID(int32_t, kVarTypeInt32);
ASMJIT_TYPE_ID(Int32Type, kVarTypeUInt8);

ASMJIT_TYPE_ID(uint32_t, kVarTypeUInt32);
ASMJIT_TYPE_ID(UInt32Type, kVarTypeUInt8);

ASMJIT_TYPE_ID(int64_t, kVarTypeInt64);
ASMJIT_TYPE_ID(Int64Type, kVarTypeUInt8);

ASMJIT_TYPE_ID(uint64_t, kVarTypeUInt64);
ASMJIT_TYPE_ID(UInt64Type, kVarTypeUInt8);

ASMJIT_TYPE_ID(float, kVarTypeFp32);
ASMJIT_TYPE_ID(FloatType, kVarTypeFp32);

ASMJIT_TYPE_ID(double, kVarTypeFp64);
ASMJIT_TYPE_ID(DoubleType, kVarTypeFp64);
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::FuncInOut]
// ============================================================================

//! Function in/out - argument or return value translated from `FuncPrototype`.
struct FuncInOut {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getVarType() const { return _varType; }

  ASMJIT_INLINE bool hasRegIndex() const { return _regIndex != kInvalidReg; }
  ASMJIT_INLINE uint32_t getRegIndex() const { return _regIndex; }

  ASMJIT_INLINE bool hasStackOffset() const { return _stackOffset != kFuncStackInvalid; }
  ASMJIT_INLINE int32_t getStackOffset() const { return static_cast<int32_t>(_stackOffset); }

  //! Get whether the argument / return value is assigned.
  ASMJIT_INLINE bool isSet() const {
    return (_regIndex != kInvalidReg) | (_stackOffset != kFuncStackInvalid);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the function argument to "unassigned state".
  ASMJIT_INLINE void reset() { _packed = 0xFFFFFFFF; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! Variable type, see `kVarType`.
      uint8_t _varType;
      //! Register index if argument / return value is a register.
      uint8_t _regIndex;
      //! Stack offset if argument / return value is on the stack.
      int16_t _stackOffset;
    };

    //! All members packed into single 32-bit integer.
    uint32_t _packed;
  };
};

// ============================================================================
// [asmjit::FuncPrototype]
// ============================================================================

//! Function prototype.
//!
//! Function prototype contains information about function return type, count
//! of arguments and their types. Function prototype is a low level structure
//! which doesn't contain platform specific or calling convention specific
//! information. Function prototype is used to create a `FuncDecl`.
struct FuncPrototype {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function return value.
  ASMJIT_INLINE uint32_t getRet() const { return _ret; }

  //! Get function arguments' IDs.
  ASMJIT_INLINE const uint32_t* getArgList() const { return _argList; }
  //! Get count of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const { return _argCount; }

  //! Get argument at index `id`.
  ASMJIT_INLINE uint32_t getArg(uint32_t id) const {
    ASMJIT_ASSERT(id < _argCount);
    return _argList[id];
  }

  //! Set function definition - return type and arguments.
  ASMJIT_INLINE void _setPrototype(uint32_t ret, const uint32_t* argList, uint32_t argCount) {
    _ret = ret;
    _argList = argList;
    _argCount = argCount;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _ret;
  uint32_t _argCount;
  const uint32_t* _argList;
};

// ============================================================================
// [asmjit::FuncBuilderX]
// ============================================================================

//! Custom function builder for up to 32 function arguments.
struct FuncBuilderX : public FuncPrototype {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncBuilderX() {
    _setPrototype(kInvalidVar, _builderArgList, 0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Set return type to `retType`.
  ASMJIT_INLINE void setRet(uint32_t retType) {
    _ret = retType;
  }

  ASMJIT_INLINE void setArg(uint32_t id, uint32_t type) {
    ASMJIT_ASSERT(id < _argCount);
    _builderArgList[id] = type;
  }

  ASMJIT_INLINE void addArg(uint32_t type) {
    ASMJIT_ASSERT(_argCount < kFuncArgCount);
    _builderArgList[_argCount++] = type;
  }

  template<typename T>
  ASMJIT_INLINE void setRetT() {
    setRet(TypeId<T>::kId);
  }

  template<typename T>
  ASMJIT_INLINE void setArgT(uint32_t id) {
    setArg(id, TypeId<T>::kId);
  }

  template<typename T>
  ASMJIT_INLINE void addArgT() {
    addArg(TypeId<T>::kId);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _builderArgList[kFuncArgCount];
};

//! \internal
#define T(_Type_) TypeId<_Type_>::kId

//! Function prototype (no args).
template<typename RET>
struct FuncBuilder0 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder0() {
    _setPrototype(T(RET), NULL, 0);
  }
};

//! Function prototype (1 argument).
template<typename RET, typename P0>
struct FuncBuilder1 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder1() {
    static const uint32_t args[] = { T(P0) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (2 arguments).
template<typename RET, typename P0, typename P1>
struct FuncBuilder2 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder2() {
    static const uint32_t args[] = { T(P0), T(P1) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (3 arguments).
template<typename RET, typename P0, typename P1, typename P2>
struct FuncBuilder3 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder3() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (4 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3>
struct FuncBuilder4 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder4() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (5 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4>
struct FuncBuilder5 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder5() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (6 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
struct FuncBuilder6 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder6() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (7 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct FuncBuilder7 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder7() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (8 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct FuncBuilder8 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder8() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6), T(P7) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (9 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct FuncBuilder9 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder9() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6), T(P7), T(P8) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (10 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct FuncBuilder10 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder10() {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6), T(P7), T(P8), T(P9) };
    _setPrototype(T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

#undef T

// ============================================================================
// [asmjit::FuncDecl]
// ============================================================================

//! Function declaration.
struct FuncDecl {
  // --------------------------------------------------------------------------
  // [Accessors - Calling Convention]
  // --------------------------------------------------------------------------

  //! Get function calling convention, see `kFuncConv`.
  ASMJIT_INLINE uint32_t getConvention() const { return _convention; }

  //! Get whether the callee pops the stack.
  ASMJIT_INLINE uint32_t getCalleePopsStack() const { return _calleePopsStack; }

  //! Get direction of arguments passed on the stack.
  //!
  //! Direction should be always `kFuncDirRtl`.
  //!
  //! \note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  ASMJIT_INLINE uint32_t getDirection() const { return _direction; }

  //! Get stack size needed for function arguments passed on the stack.
  ASMJIT_INLINE uint32_t getArgStackSize() const { return _argStackSize; }
  //! Get size of "Red Zone".
  ASMJIT_INLINE uint32_t getRedZoneSize() const { return _redZoneSize; }
  //! Get size of "Spill Zone".
  ASMJIT_INLINE uint32_t getSpillZoneSize() const { return _spillZoneSize; }

  // --------------------------------------------------------------------------
  // [Accessors - Arguments and Return]
  // --------------------------------------------------------------------------

  //! Get whether the function has a return value.
  ASMJIT_INLINE bool hasRet() const { return _retCount != 0; }
  //! Get count of function return values.
  ASMJIT_INLINE uint32_t getRetCount() const { return _retCount; }

  //! Get function return value.
  ASMJIT_INLINE FuncInOut& getRet(uint32_t index = kFuncRetLo) { return _retList[index]; }
  //! Get function return value.
  ASMJIT_INLINE const FuncInOut& getRet(uint32_t index = kFuncRetLo) const { return _retList[index]; }

  //! Get count of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const { return _argCount; }

  //! Get function arguments array.
  ASMJIT_INLINE FuncInOut* getArgList() { return _argList; }
  //! Get function arguments array (const).
  ASMJIT_INLINE const FuncInOut* getArgList() const { return _argList; }

  //! Get function argument at index `index`.
  ASMJIT_INLINE FuncInOut& getArg(size_t index) {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _argList[index];
  }

  //! Get function argument at index `index`.
  ASMJIT_INLINE const FuncInOut& getArg(size_t index) const {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _argList[index];
  }

  ASMJIT_INLINE void resetArg(size_t index) {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    _argList[index].reset();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Calling convention.
  uint8_t _convention;
  //! Whether a callee pops stack.
  uint8_t _calleePopsStack : 1;
  //! Direction for arguments passed on the stack, see `kFuncDir`.
  uint8_t _direction : 1;
  //! Reserved #0 (alignment).
  uint8_t _reserved0 : 6;

  //! Count of arguments in `_argList`.
  uint8_t _argCount;
  //! Count of return value(s).
  uint8_t _retCount;

  //! Count of bytes consumed by arguments on the stack (aligned).
  uint32_t _argStackSize;

  //! Size of "Red Zone".
  //!
  //! \note Used by AMD64-ABI (128 bytes).
  uint16_t _redZoneSize;

  //! Size of "Spill Zone".
  //!
  //! \note Used by WIN64-ABI (32 bytes).
  uint16_t _spillZoneSize;

  //! Function arguments (including HI arguments) mapped to physical
  //! registers and stack offset.
  FuncInOut _argList[kFuncArgCountLoHi];

  //! Function return value(s).
  FuncInOut _retList[2];
};

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
  ASMJIT_INLINE Node(Compiler* compiler, uint32_t type); // Defined-Later.

  //! Destroy `Node`.
  ASMJIT_INLINE ~Node() {}

  // --------------------------------------------------------------------------
  // [Accessors - List]
  // --------------------------------------------------------------------------

  //! Get previous node in the compiler stream.
  ASMJIT_INLINE Node* getPrev() const {
    return _prev;
  }

  //! Get next node in the compiler stream.
  ASMJIT_INLINE Node* getNext() const {
    return _next;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Comment]
  // --------------------------------------------------------------------------

  //! Get comment string.
  ASMJIT_INLINE const char* getComment() const {
    return _comment;
  }

  //! Set comment string to `str`.
  ASMJIT_INLINE void setComment(const char* comment) {
    _comment = comment;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Type and Flags]
  // --------------------------------------------------------------------------

  //! Get node type, see `kNodeType`.
  ASMJIT_INLINE uint32_t getType() const {
    return _type;
  }

  //! Get node flags.
  ASMJIT_INLINE uint32_t getFlags() const {
    return _flags;
  }

  //! Get whether the instruction has flag `flag`.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const {
    return (static_cast<uint32_t>(_flags) & flag) != 0;
  }

  //! Set node flags to `flags`.
  ASMJIT_INLINE void setFlags(uint32_t flags) {
    _flags = static_cast<uint16_t>(flags);
  }

  //! Add instruction `flags`.
  ASMJIT_INLINE void orFlags(uint32_t flags) {
    _flags |= static_cast<uint16_t>(flags);
  }

  //! And instruction `flags`.
  ASMJIT_INLINE void andFlags(uint32_t flags) {
    _flags &= static_cast<uint16_t>(flags);
  }

  //! Clear instruction `flags`.
  ASMJIT_INLINE void andNotFlags(uint32_t flags) {
    _flags &= ~static_cast<uint16_t>(flags);
  }

  //! Get whether the node has beed fetched.
  ASMJIT_INLINE bool isFetched() const {
    return _flowId != 0;
  }

  //! Get whether the node has been translated.
  ASMJIT_INLINE bool isTranslated() const {
    return hasFlag(kNodeFlagIsTranslated);
  }

  //! Get whether the node has been translated.
  ASMJIT_INLINE bool isScheduled() const {
    return hasFlag(kNodeFlagIsScheduled);
  }

  //! Get whether the node is informative only and can be safely removed after
  //! translation.
  //!
  //! Informative nodes are comments and hints.
  ASMJIT_INLINE bool isInformative() const {
    return hasFlag(kNodeFlagIsInformative);
  }

  //! Whether the node is `InstNode` and unconditional jump.
  ASMJIT_INLINE bool isJmp() const { return hasFlag(kNodeFlagIsJmp); }
  //! Whether the node is `InstNode` and conditional jump.
  ASMJIT_INLINE bool isJcc() const { return hasFlag(kNodeFlagIsJcc); }
  //! Whether the node is `InstNode` and conditional/unconditional jump.
  ASMJIT_INLINE bool isJmpOrJcc() const { return hasFlag(kNodeFlagIsJmp | kNodeFlagIsJcc); }
  //! Whether the node is `InstNode` and return.
  ASMJIT_INLINE bool isRet() const { return hasFlag(kNodeFlagIsRet); }

  //! Get whether the node is `InstNode` and the instruction is special.
  ASMJIT_INLINE bool isSpecial() const { return hasFlag(kNodeFlagIsSpecial); }
  //! Get whether the node is `InstNode` and the instruction uses x87-FPU.
  ASMJIT_INLINE bool isFp() const { return hasFlag(kNodeFlagIsFp); }

  // --------------------------------------------------------------------------
  // [Accessors - FlowId]
  // --------------------------------------------------------------------------

  //! Get flow index.
  ASMJIT_INLINE uint32_t getFlowId() const { return _flowId; }
  //! Set flow index.
  ASMJIT_INLINE void setFlowId(uint32_t flowId) { _flowId = flowId; }

  // --------------------------------------------------------------------------
  // [Accessors - VarMap]
  // --------------------------------------------------------------------------

  //! Get whether node contains variable allocation instructions.
  ASMJIT_INLINE bool hasMap() const {
    return _map != NULL;
  }

  //! Get variable allocation instructions.
  ASMJIT_INLINE VarMap* getMap() const {
    return _map;
  }

  //! Get variable allocation instructions casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* getMap() const {
    return static_cast<T*>(_map);
  }

  //! Set variable allocation instructions.
  ASMJIT_INLINE void setMap(VarMap* map) {
    _map = map;
  }

  // --------------------------------------------------------------------------
  // [Accessors - VarState]
  // --------------------------------------------------------------------------

  //! Get node state.
  ASMJIT_INLINE VarState* getState() const {
    return _state;
  }

  //! Get node state casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* getState() const {
    return static_cast<VarState*>(_state);
  }

  //! Set node state.
  ASMJIT_INLINE void setState(VarState* state) {
    _state = state;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Liveness]
  // --------------------------------------------------------------------------

  //! Get whether the node has variable liveness bits.
  ASMJIT_INLINE bool hasLiveness() const {
    return _liveness != NULL;
  }

  //! Get variable liveness bits.
  ASMJIT_INLINE VarBits* getLiveness() const {
    return _liveness;
  }

  //! Set variable liveness bits.
  ASMJIT_INLINE void setLiveness(VarBits* liveness) {
    _liveness = liveness;
  }

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

  //! Variable mapping (VarAttr to VarData), initially NULL, filled during
  //! fetch phase.
  VarMap* _map;

  //! Variable liveness bits (initially NULL, filled by analysis phase).
  VarBits* _liveness;

  //! Saved state.
  //!
  //! Initially NULL, not all nodes have saved state, only branch/flow control
  //! nodes.
  VarState* _state;
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
  ASMJIT_INLINE AlignNode(Compiler* compiler, uint32_t mode, uint32_t offset) :
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
  ASMJIT_INLINE EmbedNode(Compiler* compiler, void* data, uint32_t size) :
    Node(compiler, kNodeTypeEmbed) {

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
  ASMJIT_INLINE CommentNode(Compiler* compiler, const char* comment) : Node(compiler, kNodeTypeComment) {
    orFlags(kNodeFlagIsInformative);
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
  ASMJIT_INLINE HintNode(Compiler* compiler, VarData* vd, uint32_t hint, uint32_t value) :
    Node(compiler, kNodeTypeHint) {

    orFlags(kNodeFlagIsInformative);
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
  ASMJIT_INLINE TargetNode(Compiler* compiler, uint32_t labelId) : Node(compiler, kNodeTypeTarget) {
    _id = labelId;
    _numRefs = 0;
    _offset = -1;
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
  ASMJIT_INLINE VarState* getState() const { return _state; }
  //! Set state for this target.
  ASMJIT_INLINE void setState(VarState* state) { _state = state; }

  //! Get number of jumps to this target.
  ASMJIT_INLINE uint32_t getNumRefs() const { return _numRefs; }
  //! Set number of jumps to this target.
  ASMJIT_INLINE void setNumRefs(uint32_t i) { _numRefs = i; }

  //! Add number of jumps to this target.
  ASMJIT_INLINE void addNumRefs(uint32_t i = 1) { _numRefs += i; }
  //! Subtract number of jumps to this target.
  ASMJIT_INLINE void subNumRefs(uint32_t i = 1) { _numRefs -= i; }

  //! Get the label offset.
  //!
  //! \note Only valid after the content has been serialized to the `Assembler`.
  ASMJIT_INLINE intptr_t getOffset() const { return _offset; }

  //! Set the label offset.
  ASMJIT_INLINE void setOffset(intptr_t offset) { _offset = offset; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Label id.
  uint32_t _id;
  //! Count of jumps here.
  uint32_t _numRefs;

  //! Label offset, after serialization.
  intptr_t _offset;
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
  ASMJIT_INLINE InstNode(Compiler* compiler, uint32_t instId, uint32_t instOptions, Operand* opList, uint32_t opCount) :
    Node(compiler, kNodeTypeInst) {

    _instId = static_cast<uint16_t>(instId);
    _reserved = 0;
    _instOptions = instOptions;

    _opCount = static_cast<uint8_t>(opCount);
    _opList = opList;

    _updateMemOp();
  }

  //! Destroy the `InstNode` instance.
  ASMJIT_INLINE ~InstNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get instruction ID, see `kX86InstId`.
  ASMJIT_INLINE uint32_t getInstId() const {
    return _instId;
  }

  //! Set instruction ID to `instId`.
  //!
  //! Please do not modify instruction code if you don't know what are you
  //! doing. Incorrect instruction code or operands can cause assertion failure.
  ASMJIT_INLINE void setInstId(uint32_t instId) {
    _instId = static_cast<uint16_t>(instId);
  }

  //! Whether the instruction is an unconditional jump or whether the
  //! instruction is a conditional jump which is likely to be taken.
  ASMJIT_INLINE bool isTaken() const {
    return hasFlag(kNodeFlagIsTaken);
  }

  //! Get emit options.
  ASMJIT_INLINE uint32_t getOptions() const {
    return _instOptions;
  }
  //! Set emit options.
  ASMJIT_INLINE void setOptions(uint32_t options) {
    _instOptions = options;
  }
  //! Add emit options.
  ASMJIT_INLINE void addOptions(uint32_t options) {
    _instOptions |= options;
  }
  //! Mask emit options.
  ASMJIT_INLINE void andOptions(uint32_t options) {
    _instOptions &= options;
  }
  //! Clear emit options.
  ASMJIT_INLINE void delOptions(uint32_t options) {
    _instOptions &= ~options;
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

  //! Instruction ID, see `kInstId`.
  uint16_t _instId;
  //! \internal
  uint8_t _memOpIndex;
  //! \internal
  uint8_t _reserved;
  //! Instruction options, see `kInstOptions`.
  uint32_t _instOptions;

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

  ASMJIT_INLINE JumpNode(Compiler* compiler, uint32_t code, uint32_t options, Operand* opList, uint32_t opCount) :
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
  //! Always use `Compiler::addFunc()` to create a `FuncNode` instance.
  ASMJIT_INLINE FuncNode(Compiler* compiler) :
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

  //! The "Red Zone" size - count of bytes which might be accessed without
  //! adjusting the stack pointer.
  uint16_t _redZoneSize;
  //! Spill zone size (used by WIN64 ABI).
  uint16_t _spillZoneSize;

  //! Stack size needed for function arguments.
  uint32_t _argStackSize;
  //! Stack size needed for all variables and memory allocated on the stack.
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
  ASMJIT_INLINE EndNode(Compiler* compiler) : Node(compiler, kNodeTypeEnd) {
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
  ASMJIT_INLINE RetNode(Compiler* compiler, const Operand& o0, const Operand& o1) : Node(compiler, kNodeTypeRet) {
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
  ASMJIT_INLINE CallNode(Compiler* compiler, const Operand& target) :
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
  ASMJIT_INLINE SArgNode(Compiler* compiler, CallNode* call, VarData* sVd, VarData* cVd) :
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
// [asmjit::Compiler]
// ============================================================================

//! \addtogroup asmjit_base_general
//! \{

//! Base compiler.
//!
//! \sa Assembler.
struct ASMJIT_VCLASS Compiler : public CodeGen {
  ASMJIT_NO_COPY(Compiler)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `Compiler` instance.
  ASMJIT_API Compiler(Runtime* runtime);
  //! Destroy the `Compiler` instance.
  ASMJIT_API virtual ~Compiler();

  // --------------------------------------------------------------------------
  // [LookAhead]
  // --------------------------------------------------------------------------

  //! Get maximum look ahead.
  ASMJIT_INLINE uint32_t getMaxLookAhead() const {
    return _maxLookAhead;
  }

  //! Set maximum look ahead to `val`.
  ASMJIT_INLINE void setMaxLookAhead(uint32_t val) {
    _maxLookAhead = val;
  }

  // --------------------------------------------------------------------------
  // [Clear / Reset]
  // --------------------------------------------------------------------------

  //! Reset the compiler.
  //!
  //! If `releaseMemory` is true all buffers will be released to the system.
  ASMJIT_API void reset(bool releaseMemory = false);

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
    ASMJIT_ASSERT(id < _targetList.getLength());

    return _targetList[id];
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
    return _targetList.getLength();
  }

  //! Get whether `label` is created.
  ASMJIT_INLINE bool isLabelValid(const Label& label) const {
    return isLabelValid(label.getId());
  }

  //! \overload
  ASMJIT_INLINE bool isLabelValid(uint32_t id) const {
    return static_cast<size_t>(id) < _targetList.getLength();
  }

  //! Get `TargetNode` by `label`.
  ASMJIT_INLINE TargetNode* getTargetByLabel(const Label& label) {
    return getTargetByLabel(label.getId());
  }

  //! \overload
  ASMJIT_INLINE TargetNode* getTargetByLabel(uint32_t id) {
    ASMJIT_ASSERT(isLabelValid(id));
    return _targetList[id];
  }

  //! Get `label` offset or -1 if the label is not bound.
  //!
  //! This method can be only called after the code has been serialized to the
  //! `Assembler`, otherwise the offset returned will be -1 (even if the label
  //! has been bound).
  ASMJIT_INLINE intptr_t getLabelOffset(const Label& label) const {
    return getLabelOffset(label.getId());
  }

  //! \overload
  ASMJIT_INLINE intptr_t getLabelOffset(uint32_t id) const {
    ASMJIT_ASSERT(isLabelValid(id));
    return _targetList[id]->getOffset();
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
  ASMJIT_API Error bind(const Label& label);

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
  ASMJIT_API HintNode* newHint(Var& var, uint32_t hint, uint32_t value);
  //! Add a new `HintNode`.
  ASMJIT_API HintNode* addHint(Var& var, uint32_t hint, uint32_t value);

  // --------------------------------------------------------------------------
  // [Vars]
  // --------------------------------------------------------------------------

  //! Get whether variable `var` is created.
  ASMJIT_INLINE bool isVarValid(const Var& var) const {
    return static_cast<size_t>(var.getId() & kOperandIdNum) < _varList.getLength();
  }

  //! \internal
  //!
  //! Get `VarData` by `var`.
  ASMJIT_INLINE VarData* getVd(const Var& var) const {
    return getVdById(var.getId());
  }

  //! \internal
  //!
  //! Get `VarData` by `id`.
  ASMJIT_INLINE VarData* getVdById(uint32_t id) const {
    ASMJIT_ASSERT(id != kInvalidValue);
    ASMJIT_ASSERT(static_cast<size_t>(id & kOperandIdNum) < _varList.getLength());

    return _varList[id & kOperandIdNum];
  }

  //! \internal
  //!
  //! Get an array of 'VarData*'.
  ASMJIT_INLINE VarData** _getVdArray() const {
    return const_cast<VarData**>(_varList.getData());
  }

  //! \internal
  //!
  //! Create a new `VarData`.
  ASMJIT_API VarData* _newVd(uint32_t type, uint32_t size, uint32_t c, const char* name);

  //! Create a new `Var`.
  virtual Error _newVar(Var* var, uint32_t type, const char* name) = 0;

  //! Alloc variable `var`.
  ASMJIT_API void alloc(Var& var);
  //! Alloc variable `var` using `regIndex` as a register index.
  ASMJIT_API void alloc(Var& var, uint32_t regIndex);
  //! Alloc variable `var` using `reg` as a register operand.
  ASMJIT_API void alloc(Var& var, const Reg& reg);
  //! Spill variable `var`.
  ASMJIT_API void spill(Var& var);
  //! Save variable `var` if the status is `modified` at this point.
  ASMJIT_API void save(Var& var);
  //! Unuse variable `var`.
  ASMJIT_API void unuse(Var& var);

  //! Alloc variable `var` (if initialized), but only if it's initialized.
  ASMJIT_INLINE void allocUnsafe(Var& var) {
    if (var.isInitialized())
      alloc(var);
  }

  //! Alloc variable `var` (if initialized) using `regIndex` as a register index
  ASMJIT_INLINE void allocUnsafe(Var& var, uint32_t regIndex) {
    if (var.isInitialized())
      alloc(var, regIndex);
  }

  //! Alloc variable `var` (if initialized) using `reg` as a register operand.
  ASMJIT_INLINE void allocUnsafe(Var& var, const Reg& reg) {
    if (var.isInitialized())
      alloc(var, reg);
  }

  //! Spill variable `var` (if initialized).
  ASMJIT_INLINE void spillUnsafe(Var& var) {
    if (var.isInitialized())
      spill(var);
  }

  //! Save variable `var` (if initialized) if the status is `modified` at this point.
  ASMJIT_INLINE void saveUnsafe(Var& var) {
    if (var.isInitialized())
      save(var);
  }

  //! Unuse variable `var` (if initialized).
  ASMJIT_INLINE void unuseUnsafe(Var& var) {
    if (var.isInitialized())
      unuse(var);
  }

  //! Get priority of variable `var`.
  ASMJIT_API uint32_t getPriority(Var& var) const;
  //! Set priority of variable `var` to `priority`.
  ASMJIT_API void setPriority(Var& var, uint32_t priority);

  //! Get save-on-unuse `var` property.
  ASMJIT_API bool getSaveOnUnuse(Var& var) const;
  //! Set save-on-unuse `var` property to `value`.
  ASMJIT_API void setSaveOnUnuse(Var& var, bool value);

  //! Rename variable `var` to `name`.
  //!
  //! \note Only new name will appear in the logger.
  ASMJIT_API void rename(Var& var, const char* name);

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
  // [Assembler]
  // --------------------------------------------------------------------------

  //! Get an assembler instance that is associated with the compiler.
  //!
  //! \note One instance of `Assembler` is shared and has lifetime same as the
  //! compiler, however, each call to `getAssembler()` resets the assembler so
  //! new code can be serialized into it.
  ASMJIT_API Assembler* getAssembler();

  //! \internal
  //!
  //! Create a new `Assembler` instance associated with the compiler.
  virtual Assembler* _newAssembler() = 0;

  // --------------------------------------------------------------------------
  // [Serialize]
  // --------------------------------------------------------------------------

  //! Serialize a compiled code to `assembler`.
  virtual Error serialize(Assembler* assembler) = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Internal assembler.
  Assembler* _assembler;

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

  //! TargetNode list.
  PodVector<TargetNode*> _targetList;
  //! VarData list.
  PodVector<VarData*> _varList;

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

ASMJIT_INLINE Label::Label(Compiler& c) : Operand(NoInit) {
  c._newLabel(this);
}

ASMJIT_INLINE Node::Node(Compiler* compiler, uint32_t type) {
  _prev = NULL;
  _next = NULL;
  _type = static_cast<uint8_t>(type);
  _opCount = 0;
  _flags = static_cast<uint16_t>(compiler->_nodeFlags);
  _flowId = compiler->_nodeFlowId;
  _comment = NULL;
  _map = NULL;
  _liveness = NULL;
  _state = NULL;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_COMPILER_H
