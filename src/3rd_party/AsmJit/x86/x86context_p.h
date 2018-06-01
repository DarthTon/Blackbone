// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86CONTEXT_P_H
#define _ASMJIT_X86_X86CONTEXT_P_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies - AsmJit]
#include "../base/compiler.h"
#include "../base/context_p.h"
#include "../base/intutil.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86_compiler
//! \{

// ============================================================================
// [asmjit::X86Context]
// ============================================================================

#if defined(ASMJIT_DEBUG)
# define ASMJIT_X86_CHECK_STATE _checkState();
#else
# define ASMJIT_X86_CHECK_STATE
#endif // ASMJIT_DEBUG

//! \internal
//!
//! Compiler context is used by `X86Compiler`.
//!
//! Compiler context is used during compilation and normally developer doesn't
//! need access to it. The context is user per function (it's reset after each
//! function is generated).
struct X86Context : public Context {
  ASMJIT_NO_COPY(X86Context)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `X86Context` instance.
  X86Context(X86Compiler* compiler);
  //! Destroy the `X86Context` instance.
  virtual ~X86Context();

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  virtual void reset(bool releaseMemory = false);

  // --------------------------------------------------------------------------
  // [Arch]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isX64() const {
    return _zsp.getSize() == 16;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get compiler as `X86Compiler`.
  ASMJIT_INLINE X86Compiler* getCompiler() const {
    return static_cast<X86Compiler*>(_compiler);
  }

  //! Get function as `X86FuncNode`.
  ASMJIT_INLINE X86FuncNode* getFunc() const {
    return reinterpret_cast<X86FuncNode*>(_func);
  }

  //! Get clobbered registers (global).
  ASMJIT_INLINE uint32_t getClobberedRegs(uint32_t c) {
    return _clobberedRegs.get(c);
  }

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86VarMap* newVarMap(uint32_t vaCount) {
    return static_cast<X86VarMap*>(
      _baseZone.alloc(sizeof(X86VarMap) + vaCount * sizeof(VarAttr)));
  }

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  void emitLoad(VarData* vd, uint32_t regIndex, const char* reason);
  void emitSave(VarData* vd, uint32_t regIndex, const char* reason);
  void emitMove(VarData* vd, uint32_t toRegIndex, uint32_t fromRegIndex, const char* reason);
  void emitSwapGp(VarData* aVd, VarData* bVd, uint32_t aIndex, uint32_t bIndex, const char* reason);

  void emitPushSequence(uint32_t regs);
  void emitPopSequence(uint32_t regs);

  void emitConvertVarToVar(uint32_t dstType, uint32_t dstIndex, uint32_t srcType, uint32_t srcIndex);
  void emitMoveVarOnStack(uint32_t dstType, const X86Mem* dst, uint32_t srcType, uint32_t srcIndex);
  void emitMoveImmOnStack(uint32_t dstType, const X86Mem* dst, const Imm* src);

  void emitMoveImmToReg(uint32_t dstType, uint32_t dstIndex, const Imm* src);

  // --------------------------------------------------------------------------
  // [Register Management]
  // --------------------------------------------------------------------------

  void _checkState();

  ASMJIT_INLINE uint32_t getRegSize() const {
    return _zsp.getSize();
  }

  // --------------------------------------------------------------------------
  // [Attach / Detach]
  // --------------------------------------------------------------------------

  //! Attach.
  //!
  //! Attach a register to the 'VarData', changing 'VarData' members to show
  //! that the variable is currently alive and linking variable with the
  //! current 'X86VarState'.
  template<int C>
  ASMJIT_INLINE void attach(VarData* vd, uint32_t regIndex, bool modified) {
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(regIndex != kInvalidReg);

    // Prevent Esp allocation if C==Gp.
    ASMJIT_ASSERT(C != kX86RegClassGp || regIndex != kX86RegIndexSp);

    uint32_t regMask = IntUtil::mask(regIndex);

    vd->setState(kVarStateReg);
    vd->setRegIndex(regIndex);
    vd->addHomeIndex(regIndex);
    vd->setModified(modified);

    _x86State.getListByClass(C)[regIndex] = vd;
    _x86State._occupied.or_(C, regMask);
    _x86State._modified.or_(C, static_cast<uint32_t>(modified) << regIndex);

    ASMJIT_X86_CHECK_STATE
  }

  //! Detach.
  //!
  //! The opposite of 'Attach'. Detach resets the members in 'VarData'
  //! (regIndex, state and changed flags) and unlinks the variable with the
  //! current 'X86VarState'.
  template<int C>
  ASMJIT_INLINE void detach(VarData* vd, uint32_t regIndex, uint32_t vState) {
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(vd->getRegIndex() == regIndex);
    ASMJIT_ASSERT(vState != kVarStateReg);

    uint32_t regMask = IntUtil::mask(regIndex);

    vd->setState(vState);
    vd->resetRegIndex();
    vd->setModified(false);

    _x86State.getListByClass(C)[regIndex] = NULL;
    _x86State._occupied.andNot(C, regMask);
    _x86State._modified.andNot(C, regMask);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Rebase]
  // --------------------------------------------------------------------------

  //! Rebase.
  //!
  //! Change the register of the 'VarData' changing also the current 'X86VarState'.
  //! Rebase is nearly identical to 'Detach' and 'Attach' sequence, but doesn't
  // change the 'VarData' modified flag.
  template<int C>
  ASMJIT_INLINE void rebase(VarData* vd, uint32_t newRegIndex, uint32_t oldRegIndex) {
    ASMJIT_ASSERT(vd->getClass() == C);

    uint32_t newRegMask = IntUtil::mask(newRegIndex);
    uint32_t oldRegMask = IntUtil::mask(oldRegIndex);
    uint32_t bothRegMask = newRegMask ^ oldRegMask;

    vd->setRegIndex(newRegIndex);

    _x86State.getListByClass(C)[oldRegIndex] = NULL;
    _x86State.getListByClass(C)[newRegIndex] = vd;

    _x86State._occupied.xor_(C, bothRegMask);
    _x86State._modified.xor_(C, bothRegMask & -static_cast<int32_t>(vd->isModified()));

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Load / Save]
  // --------------------------------------------------------------------------

  //! Load.
  //!
  //! Load variable from its memory slot to a register, emitting 'Load'
  //! instruction and changing the variable state to allocated.
  template<int C>
  ASMJIT_INLINE void load(VarData* vd, uint32_t regIndex) {
    // Can be only called if variable is not allocated.
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(vd->getState() != kVarStateReg);
    ASMJIT_ASSERT(vd->getRegIndex() == kInvalidReg);

    emitLoad(vd, regIndex, "Load");
    attach<C>(vd, regIndex, false);

    ASMJIT_X86_CHECK_STATE
  }

  //! Save.
  //!
  //! Save the variable into its home location, but keep it as allocated.
  template<int C>
  ASMJIT_INLINE void save(VarData* vd) {
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(vd->getState() == kVarStateReg);
    ASMJIT_ASSERT(vd->getRegIndex() != kInvalidReg);

    uint32_t regIndex = vd->getRegIndex();
    uint32_t regMask = IntUtil::mask(regIndex);

    emitSave(vd, regIndex, "Save");

    vd->setModified(false);
    _x86State._modified.andNot(C, regMask);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Move / Swap]
  // --------------------------------------------------------------------------

  //! Move a register.
  //!
  //! Move register from one index to another, emitting 'Move' if needed. This
  //! function does nothing if register is already at the given index.
  template<int C>
  ASMJIT_INLINE void move(VarData* vd, uint32_t regIndex) {
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(vd->getState() == kVarStateReg);
    ASMJIT_ASSERT(vd->getRegIndex() != kInvalidReg);

    uint32_t oldIndex = vd->getRegIndex();
    if (regIndex != oldIndex) {
      emitMove(vd, regIndex, oldIndex, "Move");
      rebase<C>(vd, regIndex, oldIndex);
    }

    ASMJIT_X86_CHECK_STATE
  }

  //! Swap two registers
  //!
  //! It's only possible to swap Gp registers.
  ASMJIT_INLINE void swapGp(VarData* aVd, VarData* bVd) {
    ASMJIT_ASSERT(aVd != bVd);

    ASMJIT_ASSERT(aVd->getClass() == kX86RegClassGp);
    ASMJIT_ASSERT(aVd->getState() == kVarStateReg);
    ASMJIT_ASSERT(aVd->getRegIndex() != kInvalidReg);

    ASMJIT_ASSERT(bVd->getClass() == kX86RegClassGp);
    ASMJIT_ASSERT(bVd->getState() == kVarStateReg);
    ASMJIT_ASSERT(bVd->getRegIndex() != kInvalidReg);

    uint32_t aIndex = aVd->getRegIndex();
    uint32_t bIndex = bVd->getRegIndex();

    emitSwapGp(aVd, bVd, aIndex, bIndex, "Swap");

    aVd->setRegIndex(bIndex);
    bVd->setRegIndex(aIndex);

    _x86State.getListByClass(kX86RegClassGp)[aIndex] = bVd;
    _x86State.getListByClass(kX86RegClassGp)[bIndex] = aVd;

    uint32_t m = aVd->isModified() ^ bVd->isModified();
    _x86State._modified.xor_(kX86RegClassGp, (m << aIndex) | (m << bIndex));

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Alloc / Spill]
  // --------------------------------------------------------------------------

  //! Alloc.
  template<int C>
  ASMJIT_INLINE void alloc(VarData* vd, uint32_t regIndex) {
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(regIndex != kInvalidReg);

    uint32_t oldRegIndex = vd->getRegIndex();
    uint32_t oldState = vd->getState();
    uint32_t regMask = IntUtil::mask(regIndex);

    ASMJIT_ASSERT(_x86State.getListByClass(C)[regIndex] == NULL || regIndex == oldRegIndex);

    if (oldState != kVarStateReg) {
      if (oldState == kVarStateMem)
        emitLoad(vd, regIndex, "Alloc");
      vd->setModified(false);
    }
    else if (oldRegIndex != regIndex) {
      emitMove(vd, regIndex, oldRegIndex, "Alloc");

      _x86State.getListByClass(C)[oldRegIndex] = NULL;
      regMask ^= IntUtil::mask(oldRegIndex);
    }
    else {
      ASMJIT_X86_CHECK_STATE
      return;
    }

    vd->setState(kVarStateReg);
    vd->setRegIndex(regIndex);

    _x86State.getListByClass(C)[regIndex] = vd;
    _x86State._occupied.xor_(C, regMask);
    _x86State._modified.xor_(C, regMask & -static_cast<int32_t>(vd->isModified()));

    ASMJIT_X86_CHECK_STATE
  }

  //! Spill.
  //!
  //! Spill variable/register, saves the content to the memory-home if modified.
  template<int C>
  ASMJIT_INLINE void spill(VarData* vd) {
    ASMJIT_ASSERT(vd->getClass() == C);

    if (vd->getState() != kVarStateReg) {
      ASMJIT_X86_CHECK_STATE
      return;
    }

    uint32_t regIndex = vd->getRegIndex();

    ASMJIT_ASSERT(regIndex != kInvalidReg);
    ASMJIT_ASSERT(_x86State.getListByClass(C)[regIndex] == vd);

    if (vd->isModified())
      emitSave(vd, regIndex, "Spill");
    detach<C>(vd, regIndex, kVarStateMem);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Modify]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void modify(VarData* vd) {
    ASMJIT_ASSERT(vd->getClass() == C);

    uint32_t regIndex = vd->getRegIndex();
    uint32_t regMask = IntUtil::mask(regIndex);

    vd->setModified(true);
    _x86State._modified.or_(C, regMask);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Unuse]
  // --------------------------------------------------------------------------

  //! Unuse.
  //!
  //! Unuse variable, it will be detached it if it's allocated then its state
  //! will be changed to kVarStateUnused.
  template<int C>
  ASMJIT_INLINE void unuse(VarData* vd, uint32_t vState = kVarStateUnused) {
    ASMJIT_ASSERT(vd->getClass() == C);
    ASMJIT_ASSERT(vState != kVarStateReg);

    uint32_t regIndex = vd->getRegIndex();
    if (regIndex != kInvalidReg)
      detach<C>(vd, regIndex, vState);
    else
      vd->setState(vState);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Get state as `X86VarState`.
  ASMJIT_INLINE X86VarState* getState() const {
    return const_cast<X86VarState*>(&_x86State);
  }

  virtual void loadState(VarState* src);
  virtual VarState* saveState();

  virtual void switchState(VarState* src);
  virtual void intersectStates(VarState* a, VarState* b);

  // --------------------------------------------------------------------------
  // [Memory]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86Mem getVarMem(VarData* vd) {
    (void)getVarCell(vd);

    X86Mem mem(_memSlot);
    mem.setBase(vd->getId());
    return mem;
  }

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  virtual Error fetch();

  // --------------------------------------------------------------------------
  // [Annotate]
  // --------------------------------------------------------------------------

  virtual Error annotate();

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  virtual Error translate();

  // --------------------------------------------------------------------------
  // [Schedule]
  // --------------------------------------------------------------------------

  virtual Error schedule();

  // --------------------------------------------------------------------------
  // [Serialize]
  // --------------------------------------------------------------------------

  virtual Error serialize(Assembler* assembler, Node* start, Node* stop);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Count of X86/X64 registers.
  X86RegCount _regCount;
  //! X86/X64 stack-pointer (esp or rsp).
  X86GpReg _zsp;
  //! X86/X64 frame-pointer (ebp or rbp).
  X86GpReg _zbp;
  //! Temporary memory operand.
  X86Mem _memSlot;

  //! X86/X64 specific compiler state, linked to `_state`.
  X86VarState _x86State;
  //! Clobbered registers (for the whole function).
  X86RegMask _clobberedRegs;

  //! Memory cell where is stored address used to restore manually
  //! aligned stack.
  MemCell* _stackFrameCell;

  //! Global allocable registers mask.
  uint32_t _gaRegs[kX86RegClassCount];

  //! Function arguments base pointer (register).
  uint8_t _argBaseReg;
  //! Function variables base pointer (register).
  uint8_t _varBaseReg;
  //! Whether to emit comments.
  uint8_t _emitComments;

  //! Function arguments base offset.
  int32_t _argBaseOffset;
  //! Function variables base offset.
  int32_t _varBaseOffset;

  //! Function arguments displacement.
  int32_t _argActualDisp;
  //! Function variables displacement.
  int32_t _varActualDisp;

  //! Temporary string builder used for logging.
  StringBuilderT<256> _stringBuilder;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_X86_X86CONTEXT_P_H
