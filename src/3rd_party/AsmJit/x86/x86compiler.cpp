// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86) && !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/utils.h"
#include "../x86/x86compiler.h"
#include "../x86/x86regalloc_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Compiler - Construction / Destruction]
// ============================================================================

X86Compiler::X86Compiler(CodeHolder* code) noexcept : CodeCompiler() {
  if (code)
    code->attach(this);
}
X86Compiler::~X86Compiler() noexcept {}

// ============================================================================
// [asmjit::X86Compiler - Events]
// ============================================================================

Error X86Compiler::onAttach(CodeHolder* code) noexcept {
  uint32_t archType = code->getArchType();
  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  ASMJIT_PROPAGATE(_cbPasses.willGrow(&_cbHeap, 1));
  ASMJIT_PROPAGATE(Base::onAttach(code));

  if (archType == ArchInfo::kTypeX86)
    _nativeGpArray = x86OpData.gpd;
  else
    _nativeGpArray = x86OpData.gpq;
  _nativeGpReg = _nativeGpArray[0];

  return addPassT<X86RAPass>();
}

// ============================================================================
// [asmjit::X86Compiler - Finalize]
// ============================================================================

Error X86Compiler::finalize() {
  if (_lastError) return _lastError;

  // Flush the global constant pool.
  if (_globalConstPool) {
    addNode(_globalConstPool);
    _globalConstPool = nullptr;
  }

  Error err = kErrorOk;
  ZoneVector<CBPass*>& passes = _cbPasses;

  for (size_t i = 0, len = passes.getLength(); i < len; i++) {
    CBPass* pass = passes[i];
    err = pass->process(&_cbPassZone);
    _cbPassZone.reset();
    if (err) break;
  }

  _cbPassZone.reset();
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  // TODO: There must be possibility to attach more assemblers, this is not so nice.
  if (_code->_cgAsm) {
    return serialize(_code->_cgAsm);
  }
  else {
    X86Assembler a(_code);
    return serialize(&a);
  }
}

// ============================================================================
// [asmjit::X86Compiler - Inst]
// ============================================================================

static ASMJIT_INLINE bool isJumpInst(uint32_t instId) noexcept {
  return (instId >= X86Inst::kIdJa   && instId <= X86Inst::kIdJz    ) ||
         (instId >= X86Inst::kIdLoop && instId <= X86Inst::kIdLoopne) ;
}

Error X86Compiler::_emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {
  uint32_t options = getOptions() | getGlobalOptions();
  const char* inlineComment = getInlineComment();

  uint32_t opCount = static_cast<uint32_t>(!o0.isNone()) +
                     static_cast<uint32_t>(!o1.isNone()) +
                     static_cast<uint32_t>(!o2.isNone()) +
                     static_cast<uint32_t>(!o3.isNone()) ;

  // Handle failure and rare cases first.
  const uint32_t kErrorsAndSpecialCases = kOptionMaybeFailureCase | // CodeEmitter is in error state.
                                          kOptionStrictValidation ; // Strict validation.

  if (ASMJIT_UNLIKELY(options & kErrorsAndSpecialCases)) {
    // Don't do anything if we are in error state.
    if (_lastError) return _lastError;

#if !defined(ASMJIT_DISABLE_VALIDATION)
    // Strict validation.
    if (options & kOptionStrictValidation) {
      Operand opArray[] = {
        Operand(o0),
        Operand(o1),
        Operand(o2),
        Operand(o3)
      };

      Inst::Detail instDetail(instId, options, _extraReg);
      Error err = Inst::validate(getArchType(), instDetail, opArray, opCount);

      if (err) {
#if !defined(ASMJIT_DISABLE_LOGGING)
        StringBuilderTmp<256> sb;
        sb.appendString(DebugUtils::errorAsString(err));
        sb.appendString(": ");
        Logging::formatInstruction(sb, 0, this, getArchType(), instDetail, opArray, opCount);
        return setLastError(err, sb.getData());
#else
        return setLastError(err);
#endif
      }

      // Clear it as it must be enabled explicitly on assembler side.
      options &= ~kOptionStrictValidation;
    }
#endif // ASMJIT_DISABLE_VALIDATION
  }

  resetOptions();
  resetInlineComment();

  // decide between `CBInst` and `CBJump`.
  if (isJumpInst(instId)) {
    CBJump* node = _cbHeap.allocT<CBJump>(sizeof(CBJump) + opCount * sizeof(Operand));
    Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CBJump));

    if (ASMJIT_UNLIKELY(!node))
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    if (opCount > 0) opArray[0].copyFrom(o0);
    if (opCount > 1) opArray[1].copyFrom(o1);
    if (opCount > 2) opArray[2].copyFrom(o2);
    if (opCount > 3) opArray[3].copyFrom(o3);

    new(node) CBJump(this, instId, options, opArray, opCount);
    node->_instDetail.extraReg = _extraReg;
    _extraReg.reset();

    CBLabel* jTarget = nullptr;
    if (!(options & kOptionUnfollow)) {
      if (opArray[0].isLabel()) {
        Error err = getCBLabel(&jTarget, static_cast<Label&>(opArray[0]));
        if (err) return setLastError(err);
      }
      else {
        options |= kOptionUnfollow;
      }
    }
    node->setOptions(options);

    node->orFlags(instId == X86Inst::kIdJmp ? CBNode::kFlagIsJmp | CBNode::kFlagIsTaken : CBNode::kFlagIsJcc);
    node->_target = jTarget;
    node->_jumpNext = nullptr;

    if (jTarget) {
      node->_jumpNext = static_cast<CBJump*>(jTarget->_from);
      jTarget->_from = node;
      jTarget->addNumRefs();
    }

    // The 'jmp' is always taken, conditional jump can contain hint, we detect it.
    if (instId == X86Inst::kIdJmp)
      node->orFlags(CBNode::kFlagIsTaken);
    else if (options & X86Inst::kOptionTaken)
      node->orFlags(CBNode::kFlagIsTaken);

    if (inlineComment) {
      inlineComment = static_cast<char*>(_cbDataZone.dup(inlineComment, ::strlen(inlineComment), true));
      node->setInlineComment(inlineComment);
    }

    addNode(node);
    return kErrorOk;
  }
  else {
    CBInst* node = _cbHeap.allocT<CBInst>(sizeof(CBInst) + opCount * sizeof(Operand));
    Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CBInst));

    if (ASMJIT_UNLIKELY(!node))
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    if (opCount > 0) opArray[0].copyFrom(o0);
    if (opCount > 1) opArray[1].copyFrom(o1);
    if (opCount > 2) opArray[2].copyFrom(o2);
    if (opCount > 3) opArray[3].copyFrom(o3);

    node = new(node) CBInst(this, instId, options, opArray, opCount);
    node->_instDetail.extraReg = _extraReg;
    _extraReg.reset();

    if (inlineComment) {
      inlineComment = static_cast<char*>(_cbDataZone.dup(inlineComment, ::strlen(inlineComment), true));
      node->setInlineComment(inlineComment);
    }

    addNode(node);
    return kErrorOk;
  }
}

Error X86Compiler::_emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, const Operand_& o5) {
  uint32_t options = getOptions() | getGlobalOptions();
  const char* inlineComment = getInlineComment();

  uint32_t opCount = static_cast<uint32_t>(!o0.isNone()) +
                     static_cast<uint32_t>(!o1.isNone()) +
                     static_cast<uint32_t>(!o2.isNone()) +
                     static_cast<uint32_t>(!o3.isNone()) ;

  // Count 5th and 6th operands.
  if (!o4.isNone()) opCount = 5;
  if (!o5.isNone()) opCount = 6;

  // Handle failure and rare cases first.
  const uint32_t kErrorsAndSpecialCases = kOptionMaybeFailureCase | // CodeEmitter in error state.
                                          kOptionStrictValidation ; // Strict validation.

  if (ASMJIT_UNLIKELY(options & kErrorsAndSpecialCases)) {
    // Don't do anything if we are in error state.
    if (_lastError) return _lastError;

#if !defined(ASMJIT_DISABLE_VALIDATION)
    // Strict validation.
    if (options & kOptionStrictValidation) {
      Operand opArray[] = {
        Operand(o0),
        Operand(o1),
        Operand(o2),
        Operand(o3),
        Operand(o4),
        Operand(o5)
      };

      Inst::Detail instDetail(instId, options, _extraReg);
      Error err = Inst::validate(getArchType(), instDetail, opArray, opCount);

      if (err) {
#if !defined(ASMJIT_DISABLE_LOGGING)
        StringBuilderTmp<256> sb;
        sb.appendString(DebugUtils::errorAsString(err));
        sb.appendString(": ");
        Logging::formatInstruction(sb, 0, this, getArchType(), instDetail, opArray, opCount);
        return setLastError(err, sb.getData());
#else
        return setLastError(err);
#endif
      }

      // Clear it as it must be enabled explicitly on assembler side.
      options &= ~kOptionStrictValidation;
    }
#endif // ASMJIT_DISABLE_VALIDATION
  }

  resetOptions();
  resetInlineComment();

  // decide between `CBInst` and `CBJump`.
  if (isJumpInst(instId)) {
    CBJump* node = _cbHeap.allocT<CBJump>(sizeof(CBJump) + opCount * sizeof(Operand));
    Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CBJump));

    if (ASMJIT_UNLIKELY(!node))
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    if (opCount > 0) opArray[0].copyFrom(o0);
    if (opCount > 1) opArray[1].copyFrom(o1);
    if (opCount > 2) opArray[2].copyFrom(o2);
    if (opCount > 3) opArray[3].copyFrom(o3);
    if (opCount > 4) opArray[4].copyFrom(o4);
    if (opCount > 5) opArray[5].copyFrom(o5);

    new(node) CBJump(this, instId, options, opArray, opCount);
    node->_instDetail.extraReg = _extraReg;
    _extraReg.reset();

    CBLabel* jTarget = nullptr;
    if (!(options & kOptionUnfollow)) {
      if (opArray[0].isLabel()) {
        Error err = getCBLabel(&jTarget, static_cast<Label&>(opArray[0]));
        if (err) return setLastError(err);
      }
      else {
        options |= kOptionUnfollow;
      }
    }
    node->setOptions(options);

    node->orFlags(instId == X86Inst::kIdJmp ? CBNode::kFlagIsJmp | CBNode::kFlagIsTaken : CBNode::kFlagIsJcc);
    node->_target = jTarget;
    node->_jumpNext = nullptr;

    if (jTarget) {
      node->_jumpNext = static_cast<CBJump*>(jTarget->_from);
      jTarget->_from = node;
      jTarget->addNumRefs();
    }

    // The 'jmp' is always taken, conditional jump can contain hint, we detect it.
    if (instId == X86Inst::kIdJmp)
      node->orFlags(CBNode::kFlagIsTaken);
    else if (options & X86Inst::kOptionTaken)
      node->orFlags(CBNode::kFlagIsTaken);

    if (inlineComment) {
      inlineComment = static_cast<char*>(_cbDataZone.dup(inlineComment, ::strlen(inlineComment), true));
      node->setInlineComment(inlineComment);
    }

    addNode(node);
    return kErrorOk;
  }
  else {
    CBInst* node = _cbHeap.allocT<CBInst>(sizeof(CBInst) + opCount * sizeof(Operand));
    Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CBInst));

    if (ASMJIT_UNLIKELY(!node))
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    if (opCount > 0) opArray[0].copyFrom(o0);
    if (opCount > 1) opArray[1].copyFrom(o1);
    if (opCount > 2) opArray[2].copyFrom(o2);
    if (opCount > 3) opArray[3].copyFrom(o3);
    if (opCount > 4) opArray[4].copyFrom(o4);
    if (opCount > 5) opArray[5].copyFrom(o5);

    node = new(node) CBInst(this, instId, options, opArray, opCount);
    node->_instDetail.extraReg = _extraReg;
    _extraReg.reset();

    if (inlineComment) {
      inlineComment = static_cast<char*>(_cbDataZone.dup(inlineComment, ::strlen(inlineComment), true));
      node->setInlineComment(inlineComment);
    }

    addNode(node);
    return kErrorOk;
  }
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 && !ASMJIT_DISABLE_COMPILER
