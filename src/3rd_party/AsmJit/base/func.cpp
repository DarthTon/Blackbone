// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/arch.h"
#include "../base/func.h"

#if defined(ASMJIT_BUILD_X86)
#include "../x86/x86internal_p.h"
#include "../x86/x86operand.h"
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
#include "../arm/arminternal_p.h"
#include "../arm/armoperand.h"
#endif // ASMJIT_BUILD_ARM

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CallConv - Init / Reset]
// ============================================================================

ASMJIT_FAVOR_SIZE Error CallConv::init(uint32_t ccId) noexcept {
  reset();

#if defined(ASMJIT_BUILD_X86)
  if (CallConv::isX86Family(ccId))
    return X86Internal::initCallConv(*this, ccId);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv::isArmFamily(ccId))
    return ArmInternal::initCallConv(*this, ccId);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

// ============================================================================
// [asmjit::FuncDetail - Init / Reset]
// ============================================================================

ASMJIT_FAVOR_SIZE Error FuncDetail::init(const FuncSignature& sign) {
  uint32_t ccId = sign.getCallConv();
  CallConv& cc = _callConv;

  uint32_t argCount = sign.getArgCount();
  if (ASMJIT_UNLIKELY(argCount > kFuncArgCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  ASMJIT_PROPAGATE(cc.init(ccId));

  uint32_t gpSize = (cc.getArchType() == ArchInfo::kTypeX86) ? 4 : 8;
  uint32_t deabstractDelta = TypeId::deabstractDeltaOfSize(gpSize);

  const uint8_t* args = sign.getArgs();
  for (uint32_t i = 0; i < argCount; i++) {
    Value& arg = _args[i];
    arg.initTypeId(TypeId::deabstract(args[i], deabstractDelta));
  }
  _argCount = static_cast<uint8_t>(argCount);

  uint32_t ret = sign.getRet();
  if (ret != TypeId::kVoid) {
    _rets[0].initTypeId(TypeId::deabstract(ret, deabstractDelta));
    _retCount = 1;
  }

#if defined(ASMJIT_BUILD_X86)
  if (CallConv::isX86Family(ccId))
    return X86Internal::initFuncDetail(*this, sign, gpSize);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv::isArmFamily(ccId))
    return ArmInternal::initFuncDetail(*this, sign, gpSize);
#endif // ASMJIT_BUILD_ARM

  // We should never bubble here as if `cc.init()` succeeded then there has to
  // be an implementation for the current architecture. However, stay safe.
  return DebugUtils::errored(kErrorInvalidArgument);
}

// ============================================================================
// [asmjit::FuncFrameLayout - Init / Reset]
// ============================================================================

ASMJIT_FAVOR_SIZE Error FuncFrameLayout::init(const FuncDetail& func, const FuncFrameInfo& ffi) noexcept {
  uint32_t ccId = func.getCallConv().getId();

#if defined(ASMJIT_BUILD_X86)
  if (CallConv::isX86Family(ccId))
    return X86Internal::initFrameLayout(*this, func, ffi);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv::isArmFamily(ccId))
    return ArmInternal::initFrameLayout(*this, func, ffi);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

// ============================================================================
// [asmjit::FuncArgsMapper]
// ============================================================================

ASMJIT_FAVOR_SIZE Error FuncArgsMapper::updateFrameInfo(FuncFrameInfo& ffi) const noexcept {
  const FuncDetail* func = getFuncDetail();
  if (!func) return DebugUtils::errored(kErrorInvalidState);

  uint32_t ccId = func->getCallConv().getId();

#if defined(ASMJIT_BUILD_X86)
  if (CallConv::isX86Family(ccId))
    return X86Internal::argsToFrameInfo(*this, ffi);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv::isArmFamily(ccId))
    return ArmInternal::argsToFrameInfo(*this, ffi);
#endif // ASMJIT_BUILD_X86

  return DebugUtils::errored(kErrorInvalidArch);
}

// ============================================================================
// [asmjit::FuncUtils]
// ============================================================================

ASMJIT_FAVOR_SIZE Error FuncUtils::emitProlog(CodeEmitter* emitter, const FuncFrameLayout& layout) {
#if defined(ASMJIT_BUILD_X86)
  if (emitter->getArchInfo().isX86Family())
    return X86Internal::emitProlog(static_cast<X86Emitter*>(emitter), layout);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (emitter->getArchInfo().isArmFamily())
    return ArmInternal::emitProlog(static_cast<ArmEmitter*>(emitter), layout);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArch);
}

ASMJIT_FAVOR_SIZE Error FuncUtils::emitEpilog(CodeEmitter* emitter, const FuncFrameLayout& layout) {
#if defined(ASMJIT_BUILD_X86)
  if (emitter->getArchInfo().isX86Family())
    return X86Internal::emitEpilog(static_cast<X86Emitter*>(emitter), layout);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (emitter->getArchInfo().isArmFamily())
    return ArmInternal::emitEpilog(static_cast<ArmEmitter*>(emitter), layout);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArch);
}

ASMJIT_FAVOR_SIZE Error FuncUtils::allocArgs(CodeEmitter* emitter, const FuncFrameLayout& layout, const FuncArgsMapper& args) {
#if defined(ASMJIT_BUILD_X86)
  if (emitter->getArchInfo().isX86Family())
    return X86Internal::allocArgs(static_cast<X86Emitter*>(emitter), layout, args);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (emitter->getArchInfo().isArmFamily())
    return ArmInternal::allocArgs(static_cast<ArmEmitter*>(emitter), layout, args);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArch);
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
