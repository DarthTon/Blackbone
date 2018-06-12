// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODEBUILDER_H
#define _ASMJIT_BASE_CODEBUILDER_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_BUILDER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codeholder.h"
#include "../base/constpool.h"
#include "../base/inst.h"
#include "../base/operand.h"
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

class CBNode;
class CBPass;

class CBAlign;
class CBComment;
class CBConstPool;
class CBData;
class CBInst;
class CBJump;
class CBLabel;
class CBLabelData;
class CBSentinel;

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::CodeBuilder]
// ============================================================================

class ASMJIT_VIRTAPI CodeBuilder : public CodeEmitter {
public:
  ASMJIT_NONCOPYABLE(CodeBuilder)
  typedef CodeEmitter Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CodeBuilder` instance.
  ASMJIT_API CodeBuilder() noexcept;
  //! Destroy the `CodeBuilder` instance.
  ASMJIT_API virtual ~CodeBuilder() noexcept;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API virtual Error onDetach(CodeHolder* code) noexcept override;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get a vector of CBPass objects that will be executed by `process()`.
  ASMJIT_INLINE const ZoneVector<CBPass*>& getPasses() const noexcept { return _cbPasses; }

  //! Get a vector of CBLabel nodes.
  //!
  //! NOTE: If a label of some index is not associated with `CodeBuilder` it
  //! would be null, so always check for nulls if you iterate over the vector.
  ASMJIT_INLINE const ZoneVector<CBLabel*>& getLabels() const noexcept { return _cbLabels; }

  //! Get the first node.
  ASMJIT_INLINE CBNode* getFirstNode() const noexcept { return _firstNode; }
  //! Get the last node.
  ASMJIT_INLINE CBNode* getLastNode() const noexcept { return _lastNode; }

  // --------------------------------------------------------------------------
  // [Node-Management]
  // --------------------------------------------------------------------------

  //! \internal
  template<typename T>
  ASMJIT_INLINE T* newNodeT() noexcept { return new(_cbHeap.alloc(sizeof(T))) T(this); }

  //! \internal
  template<typename T, typename P0>
  ASMJIT_INLINE T* newNodeT(P0 p0) noexcept { return new(_cbHeap.alloc(sizeof(T))) T(this, p0); }

  //! \internal
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE T* newNodeT(P0 p0, P1 p1) noexcept { return new(_cbHeap.alloc(sizeof(T))) T(this, p0, p1); }

  //! \internal
  template<typename T, typename P0, typename P1, typename P2>
  ASMJIT_INLINE T* newNodeT(P0 p0, P1 p1, P2 p2) noexcept { return new(_cbHeap.alloc(sizeof(T))) T(this, p0, p1, p2); }

  ASMJIT_API Error registerLabelNode(CBLabel* node) noexcept;
  //! Get `CBLabel` by `id`.
  ASMJIT_API Error getCBLabel(CBLabel** pOut, uint32_t id) noexcept;
  //! Get `CBLabel` by `label`.
  ASMJIT_INLINE Error getCBLabel(CBLabel** pOut, const Label& label) noexcept { return getCBLabel(pOut, label.getId()); }

  //! Create a new \ref CBLabel node.
  ASMJIT_API CBLabel* newLabelNode() noexcept;
  //! Create a new \ref CBAlign node.
  ASMJIT_API CBAlign* newAlignNode(uint32_t mode, uint32_t alignment) noexcept;
  //! Create a new \ref CBData node.
  ASMJIT_API CBData* newDataNode(const void* data, uint32_t size) noexcept;
  //! Create a new \ref CBConstPool node.
  ASMJIT_API CBConstPool* newConstPool() noexcept;
  //! Create a new \ref CBComment node.
  ASMJIT_API CBComment* newCommentNode(const char* s, size_t len) noexcept;

  // --------------------------------------------------------------------------
  // [Code-Emitter]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Label newLabel() override;
  ASMJIT_API virtual Label newNamedLabel(const char* name, size_t nameLength = Globals::kInvalidIndex, uint32_t type = Label::kTypeGlobal, uint32_t parentId = kInvalidValue) override;
  ASMJIT_API virtual Error bind(const Label& label) override;
  ASMJIT_API virtual Error align(uint32_t mode, uint32_t alignment) override;
  ASMJIT_API virtual Error embed(const void* data, uint32_t size) override;
  ASMJIT_API virtual Error embedLabel(const Label& label) override;
  ASMJIT_API virtual Error embedConstPool(const Label& label, const ConstPool& pool) override;
  ASMJIT_API virtual Error comment(const char* s, size_t len = Globals::kInvalidIndex) override;

  // --------------------------------------------------------------------------
  // [Node-Management]
  // --------------------------------------------------------------------------

  //! Add `node` after the current and set current to `node`.
  ASMJIT_API CBNode* addNode(CBNode* node) noexcept;
  //! Insert `node` after `ref`.
  ASMJIT_API CBNode* addAfter(CBNode* node, CBNode* ref) noexcept;
  //! Insert `node` before `ref`.
  ASMJIT_API CBNode* addBefore(CBNode* node, CBNode* ref) noexcept;
  //! Remove `node`.
  ASMJIT_API CBNode* removeNode(CBNode* node) noexcept;
  //! Remove multiple nodes.
  ASMJIT_API void removeNodes(CBNode* first, CBNode* last) noexcept;

  //! Get current node.
  //!
  //! \note If this method returns null it means that nothing has been
  //! emitted yet.
  ASMJIT_INLINE CBNode* getCursor() const noexcept { return _cursor; }
  //! Set the current node without returning the previous node.
  ASMJIT_INLINE void _setCursor(CBNode* node) noexcept { _cursor = node; }
  //! Set the current node to `node` and return the previous one.
  ASMJIT_API CBNode* setCursor(CBNode* node) noexcept;

  // --------------------------------------------------------------------------
  // [Passes]
  // --------------------------------------------------------------------------

  template<typename T>
  ASMJIT_INLINE T* newPassT() noexcept { return new(_cbBaseZone.alloc(sizeof(T))) T(); }
  template<typename T, typename P0>
  ASMJIT_INLINE T* newPassT(P0 p0) noexcept { return new(_cbBaseZone.alloc(sizeof(T))) T(p0); }
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE T* newPassT(P0 p0, P1 p1) noexcept { return new(_cbBaseZone.alloc(sizeof(T))) T(p0, p1); }

  template<typename T>
  ASMJIT_INLINE Error addPassT() noexcept { return addPass(newPassT<T>()); }
  template<typename T, typename P0>
  ASMJIT_INLINE Error addPassT(P0 p0) noexcept { return addPass(newPassT<P0>(p0)); }
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE Error addPassT(P0 p0, P1 p1) noexcept { return addPass(newPassT<P0, P1>(p0, p1)); }

  //! Get a `CBPass` by name.
  ASMJIT_API CBPass* getPassByName(const char* name) const noexcept;
  //! Add `pass` to the list of passes.
  ASMJIT_API Error addPass(CBPass* pass) noexcept;
  //! Remove `pass` from the list of passes and delete it.
  ASMJIT_API Error deletePass(CBPass* pass) noexcept;

  // --------------------------------------------------------------------------
  // [Serialization]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error serialize(CodeEmitter* dst);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone _cbBaseZone;                      //!< Base zone used to allocate nodes and `CBPass`.
  Zone _cbDataZone;                      //!< Data zone used to allocate data and names.
  Zone _cbPassZone;                      //!< Zone passed to `CBPass::process()`.
  ZoneHeap _cbHeap;                      //!< ZoneHeap that uses `_cbBaseZone`.

  ZoneVector<CBPass*> _cbPasses;         //!< Array of `CBPass` objects.
  ZoneVector<CBLabel*> _cbLabels;        //!< Maps label indexes to `CBLabel` nodes.

  CBNode* _firstNode;                    //!< First node of the current section.
  CBNode* _lastNode;                     //!< Last node of the current section.
  CBNode* _cursor;                       //!< Current node (cursor).

  uint32_t _position;                    //!< Flow-id assigned to each new node.
  uint32_t _nodeFlags;                   //!< Flags assigned to each new node.
};

// ============================================================================
// [asmjit::CBPass]
// ============================================================================

//! `CodeBuilder` pass used to  code transformations, analysis, and lowering.
class ASMJIT_VIRTAPI CBPass {
public:
  ASMJIT_NONCOPYABLE(CBPass);

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API CBPass(const char* name) noexcept;
  ASMJIT_API virtual ~CBPass() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Process the code stored in CodeBuffer `cb`.
  //!
  //! This is the only function that is called by the `CodeBuilder` to process
  //! the code. It passes the CodeBuilder itself (`cb`) and also a zone memory
  //! allocator `zone`, which will be reset after the `process()` returns. The
  //! allocator should be used for all allocations as it's fast and everything
  //! it allocates will be released at once when `process()` returns.
  virtual Error process(Zone* zone) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE const CodeBuilder* cb() const noexcept { return _cb; }
  ASMJIT_INLINE const char* getName() const noexcept { return _name; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CodeBuilder* _cb;                      //!< CodeBuilder this pass is assigned to.
  const char* _name;                     //!< Name of the pass.
};

// ============================================================================
// [asmjit::CBNode]
// ============================================================================

//! Node (CodeBuilder).
//!
//! Every node represents a building-block used by \ref CodeBuilder. It can be
//! instruction, data, label, comment, directive, or any other high-level
//! representation that can be transformed to the building blocks mentioned.
//! Every class that inherits \ref CodeBuilder can define its own nodes that it
//! can lower to basic nodes.
class CBNode {
public:
  ASMJIT_NONCOPYABLE(CBNode)

  // --------------------------------------------------------------------------
  // [Type]
  // --------------------------------------------------------------------------

  //! Type of \ref CBNode.
  ASMJIT_ENUM(NodeType) {
    kNodeNone       = 0,                 //!< Invalid node (internal, don't use).

    // [CodeBuilder]
    kNodeInst       = 1,                 //!< Node is \ref CBInst or \ref CBJump.
    kNodeData       = 2,                 //!< Node is \ref CBData.
    kNodeAlign      = 3,                 //!< Node is \ref CBAlign.
    kNodeLabel      = 4,                 //!< Node is \ref CBLabel.
    kNodeLabelData  = 5,                 //!< Node is \ref CBLabelData.
    kNodeConstPool  = 6,                 //!< Node is \ref CBConstPool.
    kNodeComment    = 7,                 //!< Node is \ref CBComment.
    kNodeSentinel   = 8,                 //!< Node is \ref CBSentinel.

    // [CodeCompiler]
    kNodeFunc       = 16,                //!< Node is \ref CCFunc (considered as \ref CBLabel by \ref CodeBuilder).
    kNodeFuncExit   = 17,                //!< Node is \ref CCFuncRet.
    kNodeFuncCall   = 18,                //!< Node is \ref CCFuncCall.
    kNodePushArg    = 19,                //!< Node is \ref CCPushArg.
    kNodeHint       = 20,                //!< Node is \ref CCHint.

    // [UserDefined]
    kNodeUser       = 32                 //!< First id of a user-defined node.
  };

  // --------------------------------------------------------------------------
  // [Flags]
  // --------------------------------------------------------------------------

  ASMJIT_ENUM(Flags) {
    //! The node has been translated by the CodeCompiler.
    kFlagIsTranslated = 0x0001,
    //! If the node can be safely removed (has no effect).
    kFlagIsRemovable = 0x0004,
    //! If the node is informative only and can be safely removed.
    kFlagIsInformative = 0x0008,

    //! If the `CBInst` is a jump.
    kFlagIsJmp = 0x0010,
    //! If the `CBInst` is a conditional jump.
    kFlagIsJcc = 0x0020,

    //! If the `CBInst` is an unconditional jump or conditional jump that is
    //! likely to be taken.
    kFlagIsTaken = 0x0040,

    //! If the `CBNode` will return from a function.
    //!
    //! This flag is used by both `CBSentinel` and `CCFuncRet`.
    kFlagIsRet = 0x0080,

    //! Whether the instruction is special.
    kFlagIsSpecial = 0x0100,

    //! Whether the instruction is an FPU instruction.
    kFlagIsFp = 0x0200
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new \ref CBNode - always use \ref CodeBuilder to allocate nodes.
  ASMJIT_INLINE CBNode(CodeBuilder* cb, uint32_t type) noexcept {
    _prev = nullptr;
    _next = nullptr;
    _type = static_cast<uint8_t>(type);
    _opCount = 0;
    _flags = static_cast<uint16_t>(cb->_nodeFlags);
    _position = cb->_position;
    _inlineComment = nullptr;
    _passData = nullptr;
  }
  //! Destroy the `CBNode` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBNode() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  template<typename T>
  ASMJIT_INLINE T* as() noexcept { return static_cast<T*>(this); }
  template<typename T>
  ASMJIT_INLINE const T* as() const noexcept { return static_cast<const T*>(this); }

  //! Get previous node in the compiler stream.
  ASMJIT_INLINE CBNode* getPrev() const noexcept { return _prev; }
  //! Get next node in the compiler stream.
  ASMJIT_INLINE CBNode* getNext() const noexcept { return _next; }

  //! Get the node type, see \ref Type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _type; }
  //! Get the node flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _flags; }

  //! Get whether the instruction has flag `flag`.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (static_cast<uint32_t>(_flags) & flag) != 0; }
  //! Set node flags to `flags`.
  ASMJIT_INLINE void setFlags(uint32_t flags) noexcept { _flags = static_cast<uint16_t>(flags); }
  //! Add instruction `flags`.
  ASMJIT_INLINE void orFlags(uint32_t flags) noexcept { _flags |= static_cast<uint16_t>(flags); }
  //! And instruction `flags`.
  ASMJIT_INLINE void andFlags(uint32_t flags) noexcept { _flags &= static_cast<uint16_t>(flags); }
  //! Clear instruction `flags`.
  ASMJIT_INLINE void andNotFlags(uint32_t flags) noexcept { _flags &= ~static_cast<uint16_t>(flags); }

  //! Get whether the node has been translated.
  ASMJIT_INLINE bool isTranslated() const noexcept { return hasFlag(kFlagIsTranslated); }

  //! Get whether the node is removable if it's in unreachable code block.
  ASMJIT_INLINE bool isRemovable() const noexcept { return hasFlag(kFlagIsRemovable); }
  //! Get whether the node is informative only (comment, hint).
  ASMJIT_INLINE bool isInformative() const noexcept { return hasFlag(kFlagIsInformative); }

  //! Whether the node is `CBLabel`.
  ASMJIT_INLINE bool isLabel() const noexcept { return _type == kNodeLabel; }
  //! Whether the `CBInst` node is an unconditional jump.
  ASMJIT_INLINE bool isJmp() const noexcept { return hasFlag(kFlagIsJmp); }
  //! Whether the `CBInst` node is a conditional jump.
  ASMJIT_INLINE bool isJcc() const noexcept { return hasFlag(kFlagIsJcc); }
  //! Whether the `CBInst` node is a conditional/unconditional jump.
  ASMJIT_INLINE bool isJmpOrJcc() const noexcept { return hasFlag(kFlagIsJmp | kFlagIsJcc); }
  //! Whether the `CBInst` node is a return.
  ASMJIT_INLINE bool isRet() const noexcept { return hasFlag(kFlagIsRet); }

  //! Get whether the node is `CBInst` and the instruction is special.
  ASMJIT_INLINE bool isSpecial() const noexcept { return hasFlag(kFlagIsSpecial); }
  //! Get whether the node is `CBInst` and the instruction uses x87-FPU.
  ASMJIT_INLINE bool isFp() const noexcept { return hasFlag(kFlagIsFp); }

  ASMJIT_INLINE bool hasPosition() const noexcept { return _position != 0; }
  //! Get flow index.
  ASMJIT_INLINE uint32_t getPosition() const noexcept { return _position; }
  //! Set flow index.
  ASMJIT_INLINE void setPosition(uint32_t position) noexcept { _position = position; }

  //! Get if the node has an inline comment.
  ASMJIT_INLINE bool hasInlineComment() const noexcept { return _inlineComment != nullptr; }
  //! Get an inline comment string.
  ASMJIT_INLINE const char* getInlineComment() const noexcept { return _inlineComment; }
  //! Set an inline comment string to `s`.
  ASMJIT_INLINE void setInlineComment(const char* s) noexcept { _inlineComment = s; }
  //! Set an inline comment string to null.
  ASMJIT_INLINE void resetInlineComment() noexcept { _inlineComment = nullptr; }

  //! Get if the node has associated work-data.
  ASMJIT_INLINE bool hasPassData() const noexcept { return _passData != nullptr; }
  //! Get work-data - data used during processing & transformations.
  template<typename T>
  ASMJIT_INLINE T* getPassData() const noexcept { return (T*)_passData; }
  //! Set work-data to `data`.
  template<typename T>
  ASMJIT_INLINE void setPassData(T* data) noexcept { _passData = (void*)data; }
  //! Reset work-data to null.
  ASMJIT_INLINE void resetPassData() noexcept { _passData = nullptr; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CBNode* _prev;                         //!< Previous node.
  CBNode* _next;                         //!< Next node.

  uint8_t _type;                         //!< Node type, see \ref NodeType.
  uint8_t _opCount;                      //!< Count of operands or zero.
  uint16_t _flags;                       //!< Flags, different meaning for every type of the node.
  uint32_t _position;                    //!< Flow index.

  const char* _inlineComment;            //!< Inline comment or null if not used.
  void* _passData;                       //!< Data used exclusively by the current `CBPass`.
};

// ============================================================================
// [asmjit::CBInst]
// ============================================================================

//! Instruction (CodeBuilder).
//!
//! Wraps an instruction with its options and operands.
class CBInst : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBInst)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBInst` instance.
  ASMJIT_INLINE CBInst(CodeBuilder* cb, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : CBNode(cb, kNodeInst) {

    orFlags(kFlagIsRemovable);
    _instDetail.instId = static_cast<uint16_t>(instId);
    _instDetail.options = options;

    _opCount = static_cast<uint8_t>(opCount);
    _opArray = opArray;

    _updateMemOp();
  }

  //! Destroy the `CBInst` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBInst() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Inst::Detail& getInstDetail() noexcept { return _instDetail; }
  ASMJIT_INLINE const Inst::Detail& getInstDetail() const noexcept { return _instDetail; }

  //! Get the instruction id, see \ref Inst::Id.
  ASMJIT_INLINE uint32_t getInstId() const noexcept { return _instDetail.instId; }
  //! Set the instruction id to `instId`, see \ref Inst::Id.
  ASMJIT_INLINE void setInstId(uint32_t instId) noexcept { _instDetail.instId = instId; }

  //! Whether the instruction is either a jump or a conditional jump likely to be taken.
  ASMJIT_INLINE bool isTaken() const noexcept { return hasFlag(kFlagIsTaken); }

  //! Get emit options.
  ASMJIT_INLINE uint32_t getOptions() const noexcept { return _instDetail.options; }
  //! Set emit options.
  ASMJIT_INLINE void setOptions(uint32_t options) noexcept { _instDetail.options = options; }
  //! Add emit options.
  ASMJIT_INLINE void addOptions(uint32_t options) noexcept { _instDetail.options |= options; }
  //! Mask emit options.
  ASMJIT_INLINE void andOptions(uint32_t options) noexcept { _instDetail.options &= options; }
  //! Clear emit options.
  ASMJIT_INLINE void delOptions(uint32_t options) noexcept { _instDetail.options &= ~options; }

  //! Get if the node has an extra register operand.
  ASMJIT_INLINE bool hasExtraReg() const noexcept { return _instDetail.hasExtraReg(); }
  //! Get extra register operand.
  ASMJIT_INLINE RegOnly& getExtraReg() noexcept { return _instDetail.extraReg; }
  //! \overload
  ASMJIT_INLINE const RegOnly& getExtraReg() const noexcept { return _instDetail.extraReg; }
  //! Set extra register operand to `reg`.
  ASMJIT_INLINE void setExtraReg(const Reg& reg) noexcept { _instDetail.extraReg.init(reg); }
  //! Set extra register operand to `reg`.
  ASMJIT_INLINE void setExtraReg(const RegOnly& reg) noexcept { _instDetail.extraReg.init(reg); }
  //! Reset extra register operand.
  ASMJIT_INLINE void resetExtraReg() noexcept { _instDetail.extraReg.reset(); }

  //! Get operands count.
  ASMJIT_INLINE uint32_t getOpCount() const noexcept { return _opCount; }
  //! Get operands list.
  ASMJIT_INLINE Operand* getOpArray() noexcept { return _opArray; }
  //! \overload
  ASMJIT_INLINE const Operand* getOpArray() const noexcept { return _opArray; }

  //! Get whether the instruction contains a memory operand.
  ASMJIT_INLINE bool hasMemOp() const noexcept { return _memOpIndex != 0xFF; }
  //! Get memory operand.
  //!
  //! NOTE: Can only be called if the instruction has such operand,
  //! see `hasMemOp()`.
  ASMJIT_INLINE Mem* getMemOp() const noexcept {
    ASMJIT_ASSERT(hasMemOp());
    return static_cast<Mem*>(&_opArray[_memOpIndex]);
  }
  //! \overload
  template<typename T>
  ASMJIT_INLINE T* getMemOp() const noexcept {
    ASMJIT_ASSERT(hasMemOp());
    return static_cast<T*>(&_opArray[_memOpIndex]);
  }

  //! Set memory operand index, `0xFF` means no memory operand.
  ASMJIT_INLINE void setMemOpIndex(uint32_t index) noexcept { _memOpIndex = static_cast<uint8_t>(index); }
  //! Reset memory operand index to `0xFF` (no operand).
  ASMJIT_INLINE void resetMemOpIndex() noexcept { _memOpIndex = 0xFF; }

  // --------------------------------------------------------------------------
  // [Utils]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void _updateMemOp() noexcept {
    Operand* opArray = getOpArray();
    uint32_t opCount = getOpCount();

    uint32_t i;
    for (i = 0; i < opCount; i++)
      if (opArray[i].isMem())
        goto Update;
    i = 0xFF;

Update:
    setMemOpIndex(i);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Inst::Detail _instDetail;              //!< Instruction id, options, and extra register.
  uint8_t _memOpIndex;                   //!< \internal
  uint8_t _reserved[7];                  //!< \internal
  Operand* _opArray;                     //!< Instruction operands.
};

// ============================================================================
// [asmjit::CBInstEx]
// ============================================================================

struct CBInstEx : public CBInst {
  Operand _op4;
  Operand _op5;
};

// ============================================================================
// [asmjit::CBJump]
// ============================================================================

//! Asm jump (conditional or direct).
//!
//! Extension of `CBInst` node, which stores more information about the jump.
class CBJump : public CBInst {
public:
  ASMJIT_NONCOPYABLE(CBJump)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CBJump(CodeBuilder* cb, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : CBInst(cb, instId, options, opArray, opCount),
      _target(nullptr),
      _jumpNext(nullptr) {}
  ASMJIT_INLINE ~CBJump() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CBLabel* getTarget() const noexcept { return _target; }
  ASMJIT_INLINE CBJump* getJumpNext() const noexcept { return _jumpNext; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CBLabel* _target;                     //!< Target node.
  CBJump* _jumpNext;                    //!< Next jump to the same target in a single linked-list.
};

// ============================================================================
// [asmjit::CBData]
// ============================================================================

//! Asm data (CodeBuilder).
//!
//! Wraps `.data` directive. The node contains data that will be placed at the
//! node's position in the assembler stream. The data is considered to be RAW;
//! no analysis nor byte-order conversion is performed on RAW data.
class CBData : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBData)
  enum { kInlineBufferSize = static_cast<int>(64 - sizeof(CBNode) - 4) };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBData` instance.
  ASMJIT_INLINE CBData(CodeBuilder* cb, void* data, uint32_t size) noexcept : CBNode(cb, kNodeData) {
    if (size <= kInlineBufferSize) {
      if (data) ::memcpy(_buf, data, size);
    }
    else {
      _externalPtr = static_cast<uint8_t*>(data);
    }
    _size = size;
  }

  //! Destroy the `CBData` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBData() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get size of the data.
  uint32_t getSize() const noexcept { return _size; }
  //! Get pointer to the data.
  uint8_t* getData() const noexcept { return _size <= kInlineBufferSize ? const_cast<uint8_t*>(_buf) : _externalPtr; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint8_t _buf[kInlineBufferSize];   //!< Embedded data buffer.
      uint32_t _size;                    //!< Size of the data.
    };
    struct {
      uint8_t* _externalPtr;             //!< Pointer to external data.
    };
  };
};

// ============================================================================
// [asmjit::CBAlign]
// ============================================================================

//! Align directive (CodeBuilder).
//!
//! Wraps `.align` directive.
class CBAlign : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBAlign)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBAlign` instance.
  ASMJIT_INLINE CBAlign(CodeBuilder* cb, uint32_t mode, uint32_t alignment) noexcept
    : CBNode(cb, kNodeAlign),
      _mode(mode),
      _alignment(alignment) {}
  //! Destroy the `CBAlign` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBAlign() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get align mode.
  ASMJIT_INLINE uint32_t getMode() const noexcept { return _mode; }
  //! Set align mode.
  ASMJIT_INLINE void setMode(uint32_t mode) noexcept { _mode = mode; }

  //! Get align offset in bytes.
  ASMJIT_INLINE uint32_t getAlignment() const noexcept { return _alignment; }
  //! Set align offset in bytes to `offset`.
  ASMJIT_INLINE void setAlignment(uint32_t alignment) noexcept { _alignment = alignment; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _mode;                        //!< Align mode, see \ref AlignMode.
  uint32_t _alignment;                   //!< Alignment (in bytes).
};

// ============================================================================
// [asmjit::CBLabel]
// ============================================================================

//! Label (CodeBuilder).
class CBLabel : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBLabel)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBLabel` instance.
  ASMJIT_INLINE CBLabel(CodeBuilder* cb, uint32_t id = kInvalidValue) noexcept
    : CBNode(cb, kNodeLabel),
      _id(id),
      _numRefs(0),
      _from(nullptr) {}
  //! Destroy the `CBLabel` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBLabel() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the label id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Get the label as `Label` operand.
  ASMJIT_INLINE Label getLabel() const noexcept { return Label(_id); }

  //! Get first jmp instruction.
  ASMJIT_INLINE CBJump* getFrom() const noexcept { return _from; }

  //! Get number of jumps to this target.
  ASMJIT_INLINE uint32_t getNumRefs() const noexcept { return _numRefs; }
  //! Set number of jumps to this target.
  ASMJIT_INLINE void setNumRefs(uint32_t i) noexcept { _numRefs = i; }

  //! Add number of jumps to this target.
  ASMJIT_INLINE void addNumRefs(uint32_t i = 1) noexcept { _numRefs += i; }
  //! Subtract number of jumps to this target.
  ASMJIT_INLINE void subNumRefs(uint32_t i = 1) noexcept { _numRefs -= i; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;                          //!< Label id.
  uint32_t _numRefs;                     //!< Count of jumps here.
  CBJump* _from;                         //!< Linked-list of nodes that can jump here.
};

// ============================================================================
// [asmjit::CBLabelData]
// ============================================================================

class CBLabelData : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBLabelData)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBLabelData` instance.
  ASMJIT_INLINE CBLabelData(CodeBuilder* cb, uint32_t id = kInvalidValue) noexcept
    : CBNode(cb, kNodeLabelData),
      _id(id) {}

  //! Destroy the `CBLabelData` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBLabelData() noexcept {}

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Get the label id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Get the label as `Label` operand.
  ASMJIT_INLINE Label getLabel() const noexcept { return Label(_id); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;
};

// ============================================================================
// [asmjit::CBConstPool]
// ============================================================================

class CBConstPool : public CBLabel {
public:
  ASMJIT_NONCOPYABLE(CBConstPool)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBConstPool` instance.
  ASMJIT_INLINE CBConstPool(CodeBuilder* cb, uint32_t id = kInvalidValue) noexcept
    : CBLabel(cb, id),
      _constPool(&cb->_cbBaseZone) { _type = kNodeConstPool; }

  //! Destroy the `CBConstPool` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBConstPool() noexcept {}

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ConstPool& getConstPool() noexcept { return _constPool; }
  ASMJIT_INLINE const ConstPool& getConstPool() const noexcept { return _constPool; }

  //! Get whether the constant-pool is empty.
  ASMJIT_INLINE bool isEmpty() const noexcept { return _constPool.isEmpty(); }
  //! Get the size of the constant-pool in bytes.
  ASMJIT_INLINE size_t getSize() const noexcept { return _constPool.getSize(); }
  //! Get minimum alignment.
  ASMJIT_INLINE size_t getAlignment() const noexcept { return _constPool.getAlignment(); }

  //! See \ref ConstPool::add().
  ASMJIT_INLINE Error add(const void* data, size_t size, size_t& dstOffset) noexcept {
    return _constPool.add(data, size, dstOffset);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ConstPool _constPool;
};

// ============================================================================
// [asmjit::CBComment]
// ============================================================================

//! Comment (CodeBuilder).
class CBComment : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBComment)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBComment` instance.
  ASMJIT_INLINE CBComment(CodeBuilder* cb, const char* comment) noexcept : CBNode(cb, kNodeComment) {
    orFlags(kFlagIsRemovable | kFlagIsInformative);
    _inlineComment = comment;
  }

  //! Destroy the `CBComment` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBComment() noexcept {}
};

// ============================================================================
// [asmjit::CBSentinel]
// ============================================================================

//! Sentinel (CodeBuilder).
//!
//! Sentinel is a marker that is completely ignored by the code builder. It's
//! used to remember a position in a code as it never gets removed by any pass.
class CBSentinel : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBSentinel)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBSentinel` instance.
  ASMJIT_INLINE CBSentinel(CodeBuilder* cb) noexcept : CBNode(cb, kNodeSentinel) {}
  //! Destroy the `CBSentinel` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBSentinel() noexcept {}
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_BUILDER
#endif // _ASMJIT_BASE_CODEBUILDER_H
