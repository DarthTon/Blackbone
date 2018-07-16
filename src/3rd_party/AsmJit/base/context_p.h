// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CONTEXT_P_H
#define _ASMJIT_BASE_CONTEXT_P_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies - AsmJit]
#include "../base/compiler.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_compiler
//! \{

// ============================================================================
// [asmjit::Context]
// ============================================================================

//! \internal
//!
//! Code generation context is the logic behind `Compiler`. The context is
//! used to compile the code stored in `Compiler`.
struct Context {
  ASMJIT_NO_COPY(Context)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  Context(Compiler* compiler);
  virtual ~Context();

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the whole context.
  virtual void reset(bool releaseMemory = false);

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get compiler.
  ASMJIT_INLINE Compiler* getCompiler() const { return _compiler; }

  //! Get function.
  ASMJIT_INLINE FuncNode* getFunc() const { return _func; }
  //! Get stop node.
  ASMJIT_INLINE Node* getStop() const { return _stop; }

  //! Get start of the current scope.
  ASMJIT_INLINE Node* getStart() const { return _start; }
  //! Get end of the current scope.
  ASMJIT_INLINE Node* getEnd() const { return _end; }

  //! Get extra block.
  ASMJIT_INLINE Node* getExtraBlock() const { return _extraBlock; }
  //! Set extra block.
  ASMJIT_INLINE void setExtraBlock(Node* node) { _extraBlock = node; }

  // --------------------------------------------------------------------------
  // [Error]
  // --------------------------------------------------------------------------

  //! Get the last error code.
  ASMJIT_INLINE Error getError() const {
    return getCompiler()->getError();
  }

  //! Set the last error code and propagate it through the error handler.
  ASMJIT_INLINE Error setError(Error error, const char* message = NULL) {
    return getCompiler()->setError(error, message);
  }

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Get current state.
  ASMJIT_INLINE VarState* getState() const {
    return _state;
  }

  //! Load current state from `target` state.
  virtual void loadState(VarState* src) = 0;

  //! Save current state, returning new `VarState` instance.
  virtual VarState* saveState() = 0;

  //! Change the current state to `target` state.
  virtual void switchState(VarState* src) = 0;

  //! Change the current state to the intersection of two states `a` and `b`.
  virtual void intersectStates(VarState* a, VarState* b) = 0;

  // --------------------------------------------------------------------------
  // [Context]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error _registerContextVar(VarData* vd) {
    if (vd->hasContextId())
      return kErrorOk;

    uint32_t cid = static_cast<uint32_t>(_contextVd.getLength());
    ASMJIT_PROPAGATE_ERROR(_contextVd.append(vd));

    vd->setContextId(cid);
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Mem]
  // --------------------------------------------------------------------------

  MemCell* _newVarCell(VarData* vd);
  MemCell* _newStackCell(uint32_t size, uint32_t alignment);

  ASMJIT_INLINE MemCell* getVarCell(VarData* vd) {
    MemCell* cell = vd->getMemCell();
    return cell ? cell : _newVarCell(vd);
  }

  virtual Error resolveCellOffsets();

  // --------------------------------------------------------------------------
  // [Bits]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE VarBits* newBits(uint32_t len) {
    return static_cast<VarBits*>(
      _baseZone.allocZeroed(static_cast<size_t>(len) * VarBits::kEntitySize));
  }

  ASMJIT_INLINE VarBits* copyBits(const VarBits* src, uint32_t len) {
    return static_cast<VarBits*>(
      _baseZone.dup(src, static_cast<size_t>(len) * VarBits::kEntitySize));
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
  // [RemoveUnreachableCode]
  // --------------------------------------------------------------------------

  //! Remove unreachable code.
  virtual Error removeUnreachableCode();

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

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  //! Translate code by allocating registers and handling state changes.
  virtual Error translate() = 0;

  // --------------------------------------------------------------------------
  // [Schedule]
  // --------------------------------------------------------------------------

  virtual Error schedule();

  // --------------------------------------------------------------------------
  // [Cleanup]
  // --------------------------------------------------------------------------

  virtual void cleanup();

  // --------------------------------------------------------------------------
  // [Compile]
  // --------------------------------------------------------------------------

  virtual Error compile(FuncNode* func);

  // --------------------------------------------------------------------------
  // [Serialize]
  // --------------------------------------------------------------------------

  virtual Error serialize(Assembler* assembler, Node* start, Node* stop) = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Compiler.
  Compiler* _compiler;
  //! Function.
  FuncNode* _func;

  //! Zone allocator.
  Zone _baseZone;

  //! \internal
  //!
  //! Offset (how many bytes to add) to `VarMap` to get `VarAttr` array. Used
  //! by liveness analysis shared across all backends. This is needed because
  //! `VarMap` is a base class for a specialized version that liveness analysis
  //! doesn't use, it just needs `VarAttr` array.
  uint32_t _varMapToVaListOffset;

  //! Start of the current active scope.
  Node* _start;
  //! End of the current active scope.
  Node* _end;

  //! Node that is used to insert extra code after the function body.
  Node* _extraBlock;
  //! Stop node.
  Node* _stop;

  //! Unreachable nodes.
  PodList<Node*> _unreachableList;
  //! Jump nodes.
  PodList<Node*> _jccList;

  //! All variables used by the current function.
  PodVector<VarData*> _contextVd;

  //! Memory used to spill variables.
  MemCell* _memVarCells;
  //! Memory used to alloc memory on the stack.
  MemCell* _memStackCells;

  //! Count of 1-byte cells.
  uint32_t _mem1ByteVarsUsed;
  //! Count of 2-byte cells.
  uint32_t _mem2ByteVarsUsed;
  //! Count of 4-byte cells.
  uint32_t _mem4ByteVarsUsed;
  //! Count of 8-byte cells.
  uint32_t _mem8ByteVarsUsed;
  //! Count of 16-byte cells.
  uint32_t _mem16ByteVarsUsed;
  //! Count of 32-byte cells.
  uint32_t _mem32ByteVarsUsed;
  //! Count of 64-byte cells.
  uint32_t _mem64ByteVarsUsed;
  //! Count of stack memory cells.
  uint32_t _memStackCellsUsed;

  //! Maximum memory alignment used by the function.
  uint32_t _memMaxAlign;
  //! Count of bytes used by variables.
  uint32_t _memVarTotal;
  //! Count of bytes used by stack.
  uint32_t _memStackTotal;
  //! Count of bytes used by variables and stack after alignment.
  uint32_t _memAllTotal;

  //! Default lenght of annotated instruction.
  uint32_t _annotationLength;

  //! Current state (used by register allocator).
  VarState* _state;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_CONTEXT_P_H
