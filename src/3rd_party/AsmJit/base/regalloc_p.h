// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_REGALLOC_P_H
#define _ASMJIT_BASE_REGALLOC_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/codecompiler.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::TiedReg]
// ============================================================================

//! Tied register (CodeCompiler)
//!
//! Tied register is used to describe one ore more register operands that share
//! the same virtual register. Tied register contains all the data that is
//! essential for register allocation.
struct TiedReg {
  //! Flags.
  ASMJIT_ENUM(Flags) {
    kRReg        = 0x00000001U,          //!< Register read.
    kWReg        = 0x00000002U,          //!< Register write.
    kXReg        = 0x00000003U,          //!< Register read-write.

    kRMem        = 0x00000004U,          //!< Memory read.
    kWMem        = 0x00000008U,          //!< Memory write.
    kXMem        = 0x0000000CU,          //!< Memory read-write.

    kRDecide     = 0x00000010U,          //!< RA can decide between reg/mem read.
    kWDecide     = 0x00000020U,          //!< RA can decide between reg/mem write.
    kXDecide     = 0x00000030U,          //!< RA can decide between reg/mem read-write.

    kRFunc       = 0x00000100U,          //!< Function argument passed in register.
    kWFunc       = 0x00000200U,          //!< Function return value passed into register.
    kXFunc       = 0x00000300U,          //!< Function argument and return value.
    kRCall       = 0x00000400U,          //!< Function call operand.

    kSpill       = 0x00000800U,          //!< Variable should be spilled.
    kUnuse       = 0x00001000U,          //!< Variable should be unused at the end of the instruction/node.

    kRAll        = kRReg | kRMem | kRDecide | kRFunc | kRCall, //!< All in-flags.
    kWAll        = kWReg | kWMem | kWDecide | kWFunc,          //!< All out-flags.

    kRDone       = 0x00400000U,          //!< Already allocated on the input.
    kWDone       = 0x00800000U,          //!< Already allocated on the output.

    kX86GpbLo    = 0x10000000U,
    kX86GpbHi    = 0x20000000U,
    kX86Fld4     = 0x40000000U,
    kX86Fld8     = 0x80000000U
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void init(VirtReg* vreg, uint32_t flags = 0, uint32_t inRegs = 0, uint32_t allocableRegs = 0) noexcept {
    this->vreg = vreg;
    this->flags = flags;
    this->refCount = 0;
    this->inPhysId = Globals::kInvalidRegId;
    this->outPhysId = Globals::kInvalidRegId;
    this->reserved = 0;
    this->inRegs = inRegs;
    this->allocableRegs = allocableRegs;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get whether the variable has to be allocated in a specific input register.
  ASMJIT_INLINE uint32_t hasInPhysId() const { return inPhysId != Globals::kInvalidRegId; }
  //! Get whether the variable has to be allocated in a specific output register.
  ASMJIT_INLINE uint32_t hasOutPhysId() const { return outPhysId != Globals::kInvalidRegId; }

  //! Set the input register index.
  ASMJIT_INLINE void setInPhysId(uint32_t index) { inPhysId = static_cast<uint8_t>(index); }
  //! Set the output register index.
  ASMJIT_INLINE void setOutPhysId(uint32_t index) { outPhysId = static_cast<uint8_t>(index); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE TiedReg& operator=(const TiedReg& other) {
    ::memcpy(this, &other, sizeof(TiedReg));
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Pointer to the associated \ref VirtReg.
  VirtReg* vreg;
  //! Tied flags.
  uint32_t flags;

  union {
    struct {
      //! How many times the variable is used by the instruction/node.
      uint8_t refCount;
      //! Input register index or `kInvalidReg` if it's not given.
      //!
      //! Even if the input register index is not given (i.e. it may by any
      //! register), register allocator should assign an index that will be
      //! used to persist a variable into this specific index. It's helpful
      //! in situations where one variable has to be allocated in multiple
      //! registers to determine the register which will be persistent.
      uint8_t inPhysId;
      //! Output register index or `kInvalidReg` if it's not given.
      //!
      //! Typically `kInvalidReg` if variable is only used on input.
      uint8_t outPhysId;
      //! \internal
      uint8_t reserved;
    };

    //! \internal
    //!
    //! Packed data #0.
    uint32_t packed;
  };

  //! Mandatory input registers.
  //!
  //! Mandatory input registers are required by the instruction even if
  //! there are duplicates. This schema allows us to allocate one variable
  //! in one or more register when needed. Required mostly by instructions
  //! that have implicit register operands (imul, cpuid, ...) and function
  //! call.
  uint32_t inRegs;

  //! Allocable input registers.
  //!
  //! Optional input registers is a mask of all allocable registers for a given
  //! variable where we have to pick one of them. This mask is usually not used
  //! when _inRegs is set. If both masks are used then the register
  //! allocator tries first to find an intersection between these and allocates
  //! an extra slot if not found.
  uint32_t allocableRegs;
};

// ============================================================================
// [asmjit::RABits]
// ============================================================================

//! Fixed size bit-array.
//!
//! Used by variable liveness analysis.
struct RABits {
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

  ASMJIT_INLINE uintptr_t getBit(uint32_t index) const noexcept {
    return (data[index / kEntityBits] >> (index % kEntityBits)) & 1;
  }

  ASMJIT_INLINE void setBit(uint32_t index) noexcept {
    data[index / kEntityBits] |= static_cast<uintptr_t>(1) << (index % kEntityBits);
  }

  ASMJIT_INLINE void delBit(uint32_t index) noexcept {
    data[index / kEntityBits] &= ~(static_cast<uintptr_t>(1) << (index % kEntityBits));
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Copy bits from `s0`, returns `true` if at least one bit is set in `s0`.
  ASMJIT_INLINE bool copyBits(const RABits* s0, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool addBits(const RABits* s0, uint32_t len) noexcept {
    return addBits(this, s0, len);
  }

  ASMJIT_INLINE bool addBits(const RABits* s0, const RABits* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] | s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool andBits(const RABits* s1, uint32_t len) noexcept {
    return andBits(this, s1, len);
  }

  ASMJIT_INLINE bool andBits(const RABits* s0, const RABits* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] & s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool delBits(const RABits* s1, uint32_t len) noexcept {
    return delBits(this, s1, len);
  }

  ASMJIT_INLINE bool delBits(const RABits* s0, const RABits* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] & ~s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool _addBitsDelSource(RABits* s1, uint32_t len) noexcept {
    return _addBitsDelSource(this, s1, len);
  }

  ASMJIT_INLINE bool _addBitsDelSource(const RABits* s0, RABits* s1, uint32_t len) noexcept {
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
// [asmjit::RACell]
// ============================================================================

//! Register allocator's (RA) memory cell.
struct RACell {
  RACell* next;                          //!< Next active cell.
  int32_t offset;                        //!< Cell offset, relative to base-offset.
  uint32_t size;                         //!< Cell size.
  uint32_t alignment;                    //!< Cell alignment.
};

// ============================================================================
// [asmjit::RAData]
// ============================================================================

//! Register allocator's (RA) data associated with each \ref CBNode.
struct RAData {
  ASMJIT_INLINE RAData(uint32_t tiedTotal) noexcept
    : liveness(nullptr),
      state(nullptr),
      tiedTotal(tiedTotal) {}

  RABits* liveness;                      //!< Liveness bits (populated by liveness-analysis).
  RAState* state;                        //!< Optional saved \ref RAState.
  uint32_t tiedTotal;                    //!< Total count of \ref TiedReg regs.
};

// ============================================================================
// [asmjit::RAState]
// ============================================================================

//! Variables' state.
struct RAState {};

// ============================================================================
// [asmjit::RAPass]
// ============================================================================

//! \internal
//!
//! Register allocator pipeline used by \ref CodeCompiler.
struct RAPass : public CBPass {
public:
  ASMJIT_NONCOPYABLE(RAPass)

  typedef void (ASMJIT_CDECL* TraceNodeFunc)(RAPass* self, CBNode* node_, const char* prefix);

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  RAPass() noexcept;
  virtual ~RAPass() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual Error process(Zone* zone) noexcept override;

  //! Run the register allocator for a given function `func`.
  virtual Error compile(CCFunc* func) noexcept;

  //! Called by `compile()` to prepare the register allocator to process the
  //! given function. It should reset and set-up everything (i.e. no states
  //! from a previous compilation should prevail).
  virtual Error prepare(CCFunc* func) noexcept;

  //! Called after `compile()` to clean everything up, no matter if it
  //! succeeded or failed.
  virtual void cleanup() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the associated `CodeCompiler`.
  ASMJIT_INLINE CodeCompiler* cc() const noexcept { return static_cast<CodeCompiler*>(_cb); }

  //! Get function.
  ASMJIT_INLINE CCFunc* getFunc() const noexcept { return _func; }
  //! Get stop node.
  ASMJIT_INLINE CBNode* getStop() const noexcept { return _stop; }

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Get current state.
  ASMJIT_INLINE RAState* getState() const { return _state; }

  //! Load current state from `target` state.
  virtual void loadState(RAState* src) = 0;

  //! Save current state, returning new `RAState` instance.
  virtual RAState* saveState() = 0;

  //! Change the current state to `target` state.
  virtual void switchState(RAState* src) = 0;

  //! Change the current state to the intersection of two states `a` and `b`.
  virtual void intersectStates(RAState* a, RAState* b) = 0;

  // --------------------------------------------------------------------------
  // [Context]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error assignRAId(VirtReg* vreg) noexcept {
    // Likely as a single virtual register would be mostly used more than once,
    // this means that each virtual register will hit one bad case (doesn't
    // have id) and then all likely cases.
    if (ASMJIT_LIKELY(vreg->_raId != kInvalidValue)) return kErrorOk;

    uint32_t raId = static_cast<uint32_t>(_contextVd.getLength());
    ASMJIT_PROPAGATE(_contextVd.append(&_heap, vreg));

    vreg->_raId = raId;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Mem]
  // --------------------------------------------------------------------------

  RACell* _newVarCell(VirtReg* vreg);
  RACell* _newStackCell(uint32_t size, uint32_t alignment);

  ASMJIT_INLINE RACell* getVarCell(VirtReg* vreg) {
    RACell* cell = vreg->getMemCell();
    return cell ? cell : _newVarCell(vreg);
  }

  virtual Error resolveCellOffsets();

  // --------------------------------------------------------------------------
  // [Bits]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RABits* newBits(uint32_t len) {
    return static_cast<RABits*>(
      _zone->allocZeroed(static_cast<size_t>(len) * RABits::kEntitySize));
  }

  ASMJIT_INLINE RABits* copyBits(const RABits* src, uint32_t len) {
    return static_cast<RABits*>(
      _zone->dup(src, static_cast<size_t>(len) * RABits::kEntitySize));
  }

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  //! Fetch.
  //!
  //! Fetch iterates over all nodes and gathers information about all variables
  //! used. The process generates information required by register allocator,
  //! variable liveness analysis and translator.
  virtual Error fetch() = 0;

  // --------------------------------------------------------------------------
  // [Unreachable Code]
  // --------------------------------------------------------------------------

  //! Add unreachable-flow data to the unreachable flow list.
  ASMJIT_INLINE Error addUnreachableNode(CBNode* node) {
    ZoneList<CBNode*>::Link* link = _zone->allocT<ZoneList<CBNode*>::Link>();
    if (!link) return DebugUtils::errored(kErrorNoHeapMemory);

    link->setValue(node);
    _unreachableList.append(link);

    return kErrorOk;
  }

  //! Remove unreachable code.
  virtual Error removeUnreachableCode();

  // --------------------------------------------------------------------------
  // [Code-Flow]
  // --------------------------------------------------------------------------

  //! Add returning node (i.e. node that returns and where liveness analysis
  //! should start).
  ASMJIT_INLINE Error addReturningNode(CBNode* node) {
    ZoneList<CBNode*>::Link* link = _zone->allocT<ZoneList<CBNode*>::Link>();
    if (!link) return DebugUtils::errored(kErrorNoHeapMemory);

    link->setValue(node);
    _returningList.append(link);

    return kErrorOk;
  }

  //! Add jump-flow data to the jcc flow list.
  ASMJIT_INLINE Error addJccNode(CBNode* node) {
    ZoneList<CBNode*>::Link* link = _zone->allocT<ZoneList<CBNode*>::Link>();
    if (!link) return DebugUtils::errored(kErrorNoHeapMemory);

    link->setValue(node);
    _jccList.append(link);

    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Analyze]
  // --------------------------------------------------------------------------

  //! Perform variable liveness analysis.
  //!
  //! Analysis phase iterates over nodes in reverse order and generates a bit
  //! array describing variables that are alive at every node in the function.
  //! When the analysis start all variables are assumed dead. When a read or
  //! read/write operations of a variable is detected the variable becomes
  //! alive; when only write operation is detected the variable becomes dead.
  //!
  //! When a label is found all jumps to that label are followed and analysis
  //! repeats until all variables are resolved.
  virtual Error livenessAnalysis();

  // --------------------------------------------------------------------------
  // [Annotate]
  // --------------------------------------------------------------------------

  virtual Error annotate() = 0;
  virtual Error formatInlineComment(StringBuilder& dst, CBNode* node);

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  //! Translate code by allocating registers and handling state changes.
  virtual Error translate() = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone* _zone;                           //!< Zone passed to `process()`.
  ZoneHeap _heap;                        //!< ZoneHeap that uses `_zone`.

  CCFunc* _func;                         //!< Function being processed.
  CBNode* _stop;                         //!< Stop node.

  //! \internal
  //!
  //! Offset (how many bytes to add) to `VarMap` to get `TiedReg` array. Used
  //! by liveness analysis shared across all backends. This is needed because
  //! `VarMap` is a base class for a specialized version that liveness analysis
  //! doesn't use, it just needs `TiedReg` array.
  uint32_t _varMapToVaListOffset;

  uint8_t _emitComments;                 //!< Whether to emit comments.

  ZoneList<CBNode*> _unreachableList;     //!< Unreachable nodes.
  ZoneList<CBNode*> _returningList;       //!< Returning nodes.
  ZoneList<CBNode*> _jccList;             //!< Jump nodes.

  ZoneVector<VirtReg*> _contextVd;       //!< All variables used by the current function.
  RACell* _memVarCells;                  //!< Memory used to spill variables.
  RACell* _memStackCells;                //!< Memory used to allocate memory on the stack.

  uint32_t _mem1ByteVarsUsed;            //!< Count of 1-byte cells.
  uint32_t _mem2ByteVarsUsed;            //!< Count of 2-byte cells.
  uint32_t _mem4ByteVarsUsed;            //!< Count of 4-byte cells.
  uint32_t _mem8ByteVarsUsed;            //!< Count of 8-byte cells.
  uint32_t _mem16ByteVarsUsed;           //!< Count of 16-byte cells.
  uint32_t _mem32ByteVarsUsed;           //!< Count of 32-byte cells.
  uint32_t _mem64ByteVarsUsed;           //!< Count of 64-byte cells.
  uint32_t _memStackCellsUsed;           //!< Count of stack memory cells.

  uint32_t _memMaxAlign;                 //!< Maximum memory alignment used by the function.
  uint32_t _memVarTotal;                 //!< Count of bytes used by variables.
  uint32_t _memStackTotal;               //!< Count of bytes used by stack.
  uint32_t _memAllTotal;                 //!< Count of bytes used by variables and stack after alignment.

  uint32_t _annotationLength;            //!< Default length of an annotated instruction.
  RAState* _state;                       //!< Current RA state.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_REGALLOC_P_H
