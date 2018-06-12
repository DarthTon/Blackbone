// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODECOMPILER_H
#define _ASMJIT_BASE_CODECOMPILER_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codebuilder.h"
#include "../base/constpool.h"
#include "../base/func.h"
#include "../base/operand.h"
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct VirtReg;
struct TiedReg;
struct RAState;
struct RACell;

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::ConstScope]
// ============================================================================

//! Scope of the constant.
ASMJIT_ENUM(ConstScope) {
  //! Local constant, always embedded right after the current function.
  kConstScopeLocal = 0,
  //! Global constant, embedded at the end of the currently compiled code.
  kConstScopeGlobal = 1
};

// ============================================================================
// [asmjit::VirtReg]
// ============================================================================

//! Virtual register data (CodeCompiler).
struct VirtReg {
  //! A state of a virtual register (used during register allocation).
  ASMJIT_ENUM(State) {
    kStateNone = 0,                      //!< Not allocated, not used.
    kStateReg = 1,                       //!< Allocated in register.
    kStateMem = 2                        //!< Allocated in memory or spilled.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the virtual-register id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Get virtual-register's name.
  ASMJIT_INLINE const char* getName() const noexcept { return _name; }

  //! Get a physical register type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _regInfo.getType(); }
  //! Get a physical register kind.
  ASMJIT_INLINE uint32_t getKind() const noexcept { return _regInfo.getKind(); }
  //! Get a physical register size.
  ASMJIT_INLINE uint32_t getRegSize() const noexcept { return _regInfo.getSize(); }
  //! Get a register signature of this virtual register.
  ASMJIT_INLINE uint32_t getSignature() const noexcept { return _regInfo.getSignature(); }

  //! Get a register's type-id, see \ref TypeId.
  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _typeId; }

  //! Get virtual-register's size.
  ASMJIT_INLINE uint32_t getSize() const noexcept { return _size; }
  //! Get virtual-register's alignment.
  ASMJIT_INLINE uint32_t getAlignment() const noexcept { return _alignment; }

  //! Get the virtual-register  priority, used by compiler to decide which variable to spill.
  ASMJIT_INLINE uint32_t getPriority() const noexcept { return _priority; }
  //! Set the virtual-register  priority.
  ASMJIT_INLINE void setPriority(uint32_t priority) noexcept {
    ASMJIT_ASSERT(priority <= 0xFF);
    _priority = static_cast<uint8_t>(priority);
  }

  //! Get variable state, only used by `RAPass`.
  ASMJIT_INLINE uint32_t getState() const noexcept { return _state; }
  //! Set variable state, only used by `RAPass`.
  ASMJIT_INLINE void setState(uint32_t state) {
    ASMJIT_ASSERT(state <= 0xFF);
    _state = static_cast<uint8_t>(state);
  }

  //! Get register index.
  ASMJIT_INLINE uint32_t getPhysId() const noexcept { return _physId; }
  //! Set register index.
  ASMJIT_INLINE void setPhysId(uint32_t physId) {
    ASMJIT_ASSERT(physId <= Globals::kInvalidRegId);
    _physId = static_cast<uint8_t>(physId);
  }
  //! Reset register index.
  ASMJIT_INLINE void resetPhysId() {
    _physId = static_cast<uint8_t>(Globals::kInvalidRegId);
  }

  //! Get home registers mask.
  ASMJIT_INLINE uint32_t getHomeMask() const { return _homeMask; }
  //! Add a home register index to the home registers mask.
  ASMJIT_INLINE void addHomeId(uint32_t physId) { _homeMask |= Utils::mask(physId); }

  ASMJIT_INLINE bool isFixed() const noexcept { return static_cast<bool>(_isFixed); }

  //! Get whether the VirtReg is only memory allocated on the stack.
  ASMJIT_INLINE bool isStack() const noexcept { return static_cast<bool>(_isStack); }

  //! Get whether to save variable when it's unused (spill).
  ASMJIT_INLINE bool saveOnUnuse() const noexcept { return static_cast<bool>(_saveOnUnuse); }

  //! Get whether the variable was changed.
  ASMJIT_INLINE bool isModified() const noexcept { return static_cast<bool>(_modified); }
  //! Set whether the variable was changed.
  ASMJIT_INLINE void setModified(bool modified) noexcept { _modified = modified; }

  //! Get home memory offset.
  ASMJIT_INLINE int32_t getMemOffset() const noexcept { return _memOffset; }
  //! Set home memory offset.
  ASMJIT_INLINE void setMemOffset(int32_t offset) noexcept { _memOffset = offset; }

  //! Get home memory cell.
  ASMJIT_INLINE RACell* getMemCell() const noexcept { return _memCell; }
  //! Set home memory cell.
  ASMJIT_INLINE void setMemCell(RACell* cell) noexcept { _memCell = cell; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;                          //!< Virtual register id.
  RegInfo _regInfo;                      //!< Physical register info & signature.
  const char* _name;                     //!< Virtual name (user provided).
  uint32_t _size;                        //!< Virtual size (can be smaller than `regInfo._size`).
  uint8_t _typeId;                       //!< Type-id.
  uint8_t _alignment;                    //!< Register's natural alignment (for spilling).
  uint8_t _priority;                     //!< Allocation priority (hint for RAPass that can be ignored).
  uint8_t _isFixed : 1;                  //!< True if this is a fixed register, never reallocated.
  uint8_t _isStack : 1;                  //!< True if the virtual register is only used as a stack.
  uint8_t _isMaterialized : 1;           //!< Register is constant that is easily created by a single instruction.
  uint8_t _saveOnUnuse : 1;              //!< Save on unuse (at end of the variable scope).

  // -------------------------------------------------------------------------
  // The following members are used exclusively by RAPass. They are initialized
  // when the VirtReg is created and then changed during RAPass.
  // -------------------------------------------------------------------------

  uint32_t _raId;                        //!< Register allocator work-id (used by RAPass).
  int32_t _memOffset;                    //!< Home memory offset.
  uint32_t _homeMask;                    //!< Mask of all registers variable has been allocated to.

  uint8_t _state;                        //!< Variable state (connected with actual `RAState)`.
  uint8_t _physId;                       //!< Actual register index (only used by `RAPass)`, during translate.
  uint8_t _modified;                     //!< Whether variable was changed (connected with actual `RAState)`.

  RACell* _memCell;                      //!< Home memory cell, used by `RAPass` (initially nullptr).

  //! Temporary link to TiedReg* used by the `RAPass` used in
  //! various phases, but always set back to nullptr when finished.
  //!
  //! This temporary data is designed to be used by algorithms that need to
  //! store some data into variables themselves during compilation. But it's
  //! expected that after variable is compiled & translated the data is set
  //! back to zero/null. Initial value is nullptr.
  TiedReg* _tied;
};

// ============================================================================
// [asmjit::CCHint]
// ============================================================================

//! Hint for register allocator (CodeCompiler).
class CCHint : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CCHint)

  //! Hint type.
  ASMJIT_ENUM(Hint) {
    //! Alloc to physical reg.
    kHintAlloc = 0,
    //! Spill to memory.
    kHintSpill = 1,
    //! Save if modified.
    kHintSave = 2,
    //! Save if modified and mark it as unused.
    kHintSaveAndUnuse = 3,
    //! Mark as unused.
    kHintUnuse = 4
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCHint` instance.
  ASMJIT_INLINE CCHint(CodeBuilder* cb, VirtReg* vreg, uint32_t hint, uint32_t value) noexcept : CBNode(cb, kNodeHint) {
    orFlags(kFlagIsRemovable | kFlagIsInformative);
    _vreg = vreg;
    _hint = hint;
    _value = value;
  }

  //! Destroy the `CCHint` instance (NEVER CALLED).
  ASMJIT_INLINE ~CCHint() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get variable.
  ASMJIT_INLINE VirtReg* getVReg() const noexcept { return _vreg; }

  //! Get hint it, see \ref Hint.
  ASMJIT_INLINE uint32_t getHint() const noexcept { return _hint; }
  //! Set hint it, see \ref Hint.
  ASMJIT_INLINE void setHint(uint32_t hint) noexcept { _hint = hint; }

  //! Get hint value.
  ASMJIT_INLINE uint32_t getValue() const noexcept { return _value; }
  //! Set hint value.
  ASMJIT_INLINE void setValue(uint32_t value) noexcept { _value = value; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Variable.
  VirtReg* _vreg;
  //! Hint id.
  uint32_t _hint;
  //! Value.
  uint32_t _value;
};

// ============================================================================
// [asmjit::CCFunc]
// ============================================================================

//! Function entry (CodeCompiler).
class CCFunc : public CBLabel {
public:
  ASMJIT_NONCOPYABLE(CCFunc)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCFunc` instance.
  //!
  //! Always use `CodeCompiler::addFunc()` to create \ref CCFunc.
  ASMJIT_INLINE CCFunc(CodeBuilder* cb) noexcept
    : CBLabel(cb),
      _funcDetail(),
      _frameInfo(),
      _exitNode(nullptr),
      _end(nullptr),
      _args(nullptr),
      _isFinished(false) {

    _type = kNodeFunc;
  }

  //! Destroy the `CCFunc` instance (NEVER CALLED).
  ASMJIT_INLINE ~CCFunc() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function exit `CBLabel`.
  ASMJIT_INLINE CBLabel* getExitNode() const noexcept { return _exitNode; }
  //! Get function exit label.
  ASMJIT_INLINE Label getExitLabel() const noexcept { return _exitNode->getLabel(); }

  //! Get "End of Func" sentinel.
  ASMJIT_INLINE CBSentinel* getEnd() const noexcept { return _end; }

  //! Get function declaration.
  ASMJIT_INLINE FuncDetail& getDetail() noexcept { return _funcDetail; }
  //! Get function declaration.
  ASMJIT_INLINE const FuncDetail& getDetail() const noexcept { return _funcDetail; }

  //! Get function declaration.
  ASMJIT_INLINE FuncFrameInfo& getFrameInfo() noexcept { return _frameInfo; }
  //! Get function declaration.
  ASMJIT_INLINE const FuncFrameInfo& getFrameInfo() const noexcept { return _frameInfo; }

  //! Get arguments count.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _funcDetail.getArgCount(); }
  //! Get returns count.
  ASMJIT_INLINE uint32_t getRetCount() const noexcept { return _funcDetail.getRetCount(); }

  //! Get arguments list.
  ASMJIT_INLINE VirtReg** getArgs() const noexcept { return _args; }

  //! Get argument at `i`.
  ASMJIT_INLINE VirtReg* getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < getArgCount());
    return _args[i];
  }

  //! Set argument at `i`.
  ASMJIT_INLINE void setArg(uint32_t i, VirtReg* vreg) noexcept {
    ASMJIT_ASSERT(i < getArgCount());
    _args[i] = vreg;
  }

  //! Reset argument at `i`.
  ASMJIT_INLINE void resetArg(uint32_t i) noexcept {
    ASMJIT_ASSERT(i < getArgCount());
    _args[i] = nullptr;
  }

  ASMJIT_INLINE uint32_t getAttributes() const noexcept { return _frameInfo.getAttributes(); }
  ASMJIT_INLINE void addAttributes(uint32_t attrs) noexcept { _frameInfo.addAttributes(attrs); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FuncDetail _funcDetail;                //!< Function detail.
  FuncFrameInfo _frameInfo;              //!< Function frame information.

  CBLabel* _exitNode;                    //!< Function exit.
  CBSentinel* _end;                      //!< Function end.

  VirtReg** _args;                       //!< Arguments array as `VirtReg`.

  //! Function was finished by `Compiler::endFunc()`.
  uint8_t _isFinished;
};

// ============================================================================
// [asmjit::CCFuncRet]
// ============================================================================

//! Function return (CodeCompiler).
class CCFuncRet : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CCFuncRet)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCFuncRet` instance.
  ASMJIT_INLINE CCFuncRet(CodeBuilder* cb, const Operand_& o0, const Operand_& o1) noexcept : CBNode(cb, kNodeFuncExit) {
    orFlags(kFlagIsRet);
    _ret[0].copyFrom(o0);
    _ret[1].copyFrom(o1);
  }

  //! Destroy the `CCFuncRet` instance (NEVER CALLED).
  ASMJIT_INLINE ~CCFuncRet() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the first return operand.
  ASMJIT_INLINE Operand& getFirst() noexcept { return static_cast<Operand&>(_ret[0]); }
  //! \overload
  ASMJIT_INLINE const Operand& getFirst() const noexcept { return static_cast<const Operand&>(_ret[0]); }

  //! Get the second return operand.
  ASMJIT_INLINE Operand& getSecond() noexcept { return static_cast<Operand&>(_ret[1]); }
   //! \overload
  ASMJIT_INLINE const Operand& getSecond() const noexcept { return static_cast<const Operand&>(_ret[1]); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Return operands.
  Operand_ _ret[2];
};

// ============================================================================
// [asmjit::CCFuncCall]
// ============================================================================

//! Function call (CodeCompiler).
class CCFuncCall : public CBInst {
public:
  ASMJIT_NONCOPYABLE(CCFuncCall)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCFuncCall` instance.
  ASMJIT_INLINE CCFuncCall(CodeBuilder* cb, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : CBInst(cb, instId, options, opArray, opCount),
      _funcDetail(),
      _args(nullptr) {

    _type = kNodeFuncCall;
    _ret[0].reset();
    _ret[1].reset();
    orFlags(kFlagIsRemovable);
  }

  //! Destroy the `CCFuncCall` instance (NEVER CALLED).
  ASMJIT_INLINE ~CCFuncCall() noexcept {}

  // --------------------------------------------------------------------------
  // [Signature]
  // --------------------------------------------------------------------------

  //! Set function signature.
  ASMJIT_INLINE Error setSignature(const FuncSignature& sign) noexcept {
    return _funcDetail.init(sign);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function declaration.
  ASMJIT_INLINE FuncDetail& getDetail() noexcept { return _funcDetail; }
  //! Get function declaration.
  ASMJIT_INLINE const FuncDetail& getDetail() const noexcept { return _funcDetail; }

  //! Get target operand.
  ASMJIT_INLINE Operand& getTarget() noexcept { return static_cast<Operand&>(_opArray[0]); }
  //! \overload
  ASMJIT_INLINE const Operand& getTarget() const noexcept { return static_cast<const Operand&>(_opArray[0]); }

  //! Get return at `i`.
  ASMJIT_INLINE Operand& getRet(uint32_t i = 0) noexcept {
    ASMJIT_ASSERT(i < 2);
    return static_cast<Operand&>(_ret[i]);
  }
  //! \overload
  ASMJIT_INLINE const Operand& getRet(uint32_t i = 0) const noexcept {
    ASMJIT_ASSERT(i < 2);
    return static_cast<const Operand&>(_ret[i]);
  }

  //! Get argument at `i`.
  ASMJIT_INLINE Operand& getArg(uint32_t i) noexcept {
    ASMJIT_ASSERT(i < kFuncArgCountLoHi);
    return static_cast<Operand&>(_args[i]);
  }
  //! \overload
  ASMJIT_INLINE const Operand& getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < kFuncArgCountLoHi);
    return static_cast<const Operand&>(_args[i]);
  }

  //! Set argument at `i` to `op`.
  ASMJIT_API bool _setArg(uint32_t i, const Operand_& op) noexcept;
  //! Set return at `i` to `op`.
  ASMJIT_API bool _setRet(uint32_t i, const Operand_& op) noexcept;

  //! Set argument at `i` to `reg`.
  ASMJIT_INLINE bool setArg(uint32_t i, const Reg& reg) noexcept { return _setArg(i, reg); }
  //! Set argument at `i` to `imm`.
  ASMJIT_INLINE bool setArg(uint32_t i, const Imm& imm) noexcept { return _setArg(i, imm); }

  //! Set return at `i` to `var`.
  ASMJIT_INLINE bool setRet(uint32_t i, const Reg& reg) noexcept { return _setRet(i, reg); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FuncDetail _funcDetail;                //!< Function detail.
  Operand_ _ret[2];                      //!< Return.
  Operand_* _args;                       //!< Arguments.
};

// ============================================================================
// [asmjit::CCPushArg]
// ============================================================================

//! Push argument before a function call (CodeCompiler).
class CCPushArg : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CCPushArg)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCPushArg` instance.
  ASMJIT_INLINE CCPushArg(CodeBuilder* cb, CCFuncCall* call, VirtReg* src, VirtReg* cvt) noexcept
    : CBNode(cb, kNodePushArg),
      _call(call),
      _src(src),
      _cvt(cvt),
      _args(0) {
    orFlags(kFlagIsRemovable);
  }

  //! Destroy the `CCPushArg` instance.
  ASMJIT_INLINE ~CCPushArg() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the associated function-call.
  ASMJIT_INLINE CCFuncCall* getCall() const noexcept { return _call; }
  //! Get source variable.
  ASMJIT_INLINE VirtReg* getSrcReg() const noexcept { return _src; }
  //! Get conversion variable.
  ASMJIT_INLINE VirtReg* getCvtReg() const noexcept { return _cvt; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CCFuncCall* _call;                     //!< Associated `CCFuncCall`.
  VirtReg* _src;                         //!< Source variable.
  VirtReg* _cvt;                         //!< Temporary variable used for conversion (or null).
  uint32_t _args;                        //!< Affected arguments bit-array.
};

// ============================================================================
// [asmjit::CodeCompiler]
// ============================================================================

//! Code emitter that uses virtual registers and performs register allocation.
//!
//! Compiler is a high-level code-generation tool that provides register
//! allocation and automatic handling of function calling conventions. It was
//! primarily designed for merging multiple parts of code into a function
//! without worrying about registers and function calling conventions.
//!
//! CodeCompiler can be used, with a minimum effort, to handle 32-bit and 64-bit
//! code at the same time.
//!
//! CodeCompiler is based on CodeBuilder and contains all the features it
//! provides. It means that the code it stores can be modified (removed, added,
//! injected) and analyzed. When the code is finalized the compiler can emit
//! the code into an Assembler to translate the abstract representation into a
//! machine code.
class ASMJIT_VIRTAPI CodeCompiler : public CodeBuilder {
public:
  ASMJIT_NONCOPYABLE(CodeCompiler)
  typedef CodeBuilder Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CodeCompiler` instance.
  ASMJIT_API CodeCompiler() noexcept;
  //! Destroy the `CodeCompiler` instance.
  ASMJIT_API virtual ~CodeCompiler() noexcept;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API virtual Error onDetach(CodeHolder* code) noexcept override;

  // --------------------------------------------------------------------------
  // [Node-Factory]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Create a new `CCHint`.
  ASMJIT_API CCHint* newHintNode(Reg& reg, uint32_t hint, uint32_t value) noexcept;

  // --------------------------------------------------------------------------
  // [Func]
  // --------------------------------------------------------------------------

  //! Get the current function.
  ASMJIT_INLINE CCFunc* getFunc() const noexcept { return _func; }

  //! Create a new `CCFunc`.
  ASMJIT_API CCFunc* newFunc(const FuncSignature& sign) noexcept;
  //! Add a function `node` to the stream.
  ASMJIT_API CCFunc* addFunc(CCFunc* func);
  //! Add a new function.
  ASMJIT_API CCFunc* addFunc(const FuncSignature& sign);
  //! Emit a sentinel that marks the end of the current function.
  ASMJIT_API CBSentinel* endFunc();

  // --------------------------------------------------------------------------
  // [Ret]
  // --------------------------------------------------------------------------

  //! Create a new `CCFuncRet`.
  ASMJIT_API CCFuncRet* newRet(const Operand_& o0, const Operand_& o1) noexcept;
  //! Add a new `CCFuncRet`.
  ASMJIT_API CCFuncRet* addRet(const Operand_& o0, const Operand_& o1) noexcept;

  // --------------------------------------------------------------------------
  // [Call]
  // --------------------------------------------------------------------------

  //! Create a new `CCFuncCall`.
  ASMJIT_API CCFuncCall* newCall(uint32_t instId, const Operand_& o0, const FuncSignature& sign) noexcept;
  //! Add a new `CCFuncCall`.
  ASMJIT_API CCFuncCall* addCall(uint32_t instId, const Operand_& o0, const FuncSignature& sign) noexcept;

  // --------------------------------------------------------------------------
  // [Args]
  // --------------------------------------------------------------------------

  //! Set a function argument at `argIndex` to `reg`.
  ASMJIT_API Error setArg(uint32_t argIndex, const Reg& reg);

  // --------------------------------------------------------------------------
  // [Hint]
  // --------------------------------------------------------------------------

  //! Emit a new hint (purely informational node).
  ASMJIT_API Error _hint(Reg& reg, uint32_t hint, uint32_t value);

  // --------------------------------------------------------------------------
  // [VirtReg / Stack]
  // --------------------------------------------------------------------------

  //! Create a new virtual register representing the given `vti` and `signature`.
  //!
  //! This function accepts either register type representing a machine-specific
  //! register, like `X86Reg`, or RegTag representation, which represents
  //! machine independent register, and from the machine-specific register
  //! is deduced.
  ASMJIT_API VirtReg* newVirtReg(uint32_t typeId, uint32_t signature, const char* name) noexcept;

  ASMJIT_API Error _newReg(Reg& out, uint32_t typeId, const char* name);
  ASMJIT_API Error _newReg(Reg& out, uint32_t typeId, const char* nameFmt, va_list ap);

  ASMJIT_API Error _newReg(Reg& out, const Reg& ref, const char* name);
  ASMJIT_API Error _newReg(Reg& out, const Reg& ref, const char* nameFmt, va_list ap);

  ASMJIT_API Error _newStack(Mem& out, uint32_t size, uint32_t alignment, const char* name);
  ASMJIT_API Error _newConst(Mem& out, uint32_t scope, const void* data, size_t size);

  // --------------------------------------------------------------------------
  // [VirtReg]
  // --------------------------------------------------------------------------

  //! Get whether the virtual register `r` is valid.
  ASMJIT_INLINE bool isVirtRegValid(const Reg& reg) const noexcept {
    return isVirtRegValid(reg.getId());
  }
  //! \overload
  ASMJIT_INLINE bool isVirtRegValid(uint32_t id) const noexcept {
    size_t index = Operand::unpackId(id);
    return index < _vRegArray.getLength();
  }

  //! Get \ref VirtReg associated with the given `r`.
  ASMJIT_INLINE VirtReg* getVirtReg(const Reg& reg) const noexcept {
    return getVirtRegById(reg.getId());
  }
  //! Get \ref VirtReg associated with the given `id`.
  ASMJIT_INLINE VirtReg* getVirtRegById(uint32_t id) const noexcept {
    ASMJIT_ASSERT(id != kInvalidValue);
    size_t index = Operand::unpackId(id);

    ASMJIT_ASSERT(index < _vRegArray.getLength());
    return _vRegArray[index];
  }

  //! Get an array of all virtual registers managed by CodeCompiler.
  ASMJIT_INLINE const ZoneVector<VirtReg*>& getVirtRegArray() const noexcept { return _vRegArray; }

  //! Alloc a virtual register `reg`.
  ASMJIT_API Error alloc(Reg& reg);
  //! Alloc a virtual register `reg` using `physId` as a register id.
  ASMJIT_API Error alloc(Reg& reg, uint32_t physId);
  //! Alloc a virtual register `reg` using `ref` as a register operand.
  ASMJIT_API Error alloc(Reg& reg, const Reg& ref);
  //! Spill a virtual register `reg`.
  ASMJIT_API Error spill(Reg& reg);
  //! Save a virtual register `reg` if the status is `modified` at this point.
  ASMJIT_API Error save(Reg& reg);
  //! Unuse a virtual register `reg`.
  ASMJIT_API Error unuse(Reg& reg);

  //! Get priority of a virtual register `reg`.
  ASMJIT_API uint32_t getPriority(Reg& reg) const;
  //! Set priority of variable `reg` to `priority`.
  ASMJIT_API void setPriority(Reg& reg, uint32_t priority);

  //! Get save-on-unuse `reg` property.
  ASMJIT_API bool getSaveOnUnuse(Reg& reg) const;
  //! Set save-on-unuse `reg` property to `value`.
  ASMJIT_API void setSaveOnUnuse(Reg& reg, bool value);

  //! Rename variable `reg` to `name`.
  //!
  //! NOTE: Only new name will appear in the logger.
  ASMJIT_API void rename(Reg& reg, const char* fmt, ...);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CCFunc* _func;                         //!< Current function.

  Zone _vRegZone;                        //!< Allocates \ref VirtReg objects.
  ZoneVector<VirtReg*> _vRegArray;       //!< Stores array of \ref VirtReg pointers.

  CBConstPool* _localConstPool;          //!< Local constant pool, flushed at the end of each function.
  CBConstPool* _globalConstPool;         //!< Global constant pool, flushed at the end of the compilation.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_CODECOMPILER_H
