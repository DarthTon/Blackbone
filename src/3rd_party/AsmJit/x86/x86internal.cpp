// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../x86/x86internal_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Internal - Helpers]
// ============================================================================

static ASMJIT_INLINE uint32_t x86GetXmmMovInst(const FuncFrameLayout& layout) {
  bool avx = layout.isAvxEnabled();
  bool aligned = layout.hasAlignedVecSR();

  return aligned ? (avx ? X86Inst::kIdVmovaps : X86Inst::kIdMovaps)
                 : (avx ? X86Inst::kIdVmovups : X86Inst::kIdMovups);
}

static ASMJIT_INLINE uint32_t x86VecTypeIdToRegType(uint32_t typeId) noexcept {
  return typeId <= TypeId::_kVec128End ? X86Reg::kRegXmm :
         typeId <= TypeId::_kVec256End ? X86Reg::kRegYmm :
                                         X86Reg::kRegZmm ;
}

// ============================================================================
// [asmjit::X86FuncArgsContext]
// ============================================================================

// Used by both, `Utils::argsToFrameInfo()` and `Utils::allocArgs()`.
class X86FuncArgsContext {
public:
  typedef FuncDetail::Value SrcArg;
  typedef FuncArgsMapper::Value DstArg;

  enum { kMaxVRegKinds = Globals::kMaxVRegKinds };

  struct WorkData {
    uint32_t archRegs;                   //!< Architecture provided and allocable regs.
    uint32_t workRegs;                   //!< Registers that can be used by shuffler.
    uint32_t usedRegs;                   //!< Only registers used to pass arguments.
    uint32_t srcRegs;                    //!< Source registers that need shuffling.
    uint32_t dstRegs;                    //!< Destination registers that need shuffling.
    uint8_t numOps;                      //!< Number of operations to finish.
    uint8_t numSwaps;                    //!< Number of register swaps.
    uint8_t numStackArgs;                //!< Number of stack loads.
    uint8_t reserved[9];                 //!< Reserved (only used as padding).
    uint8_t argIndex[32];                //!< Only valid if a corresponding bit in `userRegs` is true.
  };

  X86FuncArgsContext() noexcept;
  Error initWorkData(const FuncArgsMapper& args, const uint32_t* dirtyRegs, bool preservedFP) noexcept;

  Error markRegsForSwaps(FuncFrameInfo& ffi) noexcept;
  Error markDstRegsDirty(FuncFrameInfo& ffi) noexcept;
  Error markStackArgsReg(FuncFrameInfo& ffi) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  WorkData _workData[kMaxVRegKinds];
  bool _hasStackArgs;
  bool _hasRegSwaps;
};

X86FuncArgsContext::X86FuncArgsContext() noexcept {
  ::memset(_workData, 0, sizeof(_workData));
  _hasStackArgs = false;
  _hasRegSwaps = false;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::initWorkData(const FuncArgsMapper& args, const uint32_t* dirtyRegs, bool preservedFP) noexcept {
  // This code has to be updated if this changes.
  ASMJIT_ASSERT(kMaxVRegKinds == 4);

  uint32_t i;
  const FuncDetail& func = *args.getFuncDetail();

  uint32_t archType = func.getCallConv().getArchType();
  uint32_t count = (archType == ArchInfo::kTypeX86) ? 8 : 16;

  // Initialize WorkData::archRegs.
  _workData[X86Reg::kKindGp ].archRegs = Utils::bits(count) & ~Utils::mask(X86Gp::kIdSp);
  _workData[X86Reg::kKindMm ].archRegs = Utils::bits(8);
  _workData[X86Reg::kKindK  ].archRegs = Utils::bits(8);
  _workData[X86Reg::kKindVec].archRegs = Utils::bits(count);

  if (preservedFP)
    _workData[X86Reg::kKindGp].archRegs &= ~Utils::mask(X86Gp::kIdBp);

  // Initialize WorkData::workRegs.
  for (i = 0; i < kMaxVRegKinds; i++)
    _workData[i].workRegs = _workData[i].archRegs & (dirtyRegs[i] | ~func.getCallConv().getPreservedRegs(i));

  // Build WorkData.
  for (i = 0; i < kFuncArgCountLoHi; i++) {
    const DstArg& dstArg = args.getArg(i);
    if (!dstArg.isAssigned()) continue;

    const SrcArg& srcArg = func.getArg(i);
    if (ASMJIT_UNLIKELY(!srcArg.isAssigned()))
      return DebugUtils::errored(kErrorInvalidState);

    uint32_t dstRegType = dstArg.getRegType();
    if (ASMJIT_UNLIKELY(dstRegType >= X86Reg::kRegCount))
      return DebugUtils::errored(kErrorInvalidRegType);

    uint32_t dstRegKind = X86Reg::kindOf(dstRegType);
    if (ASMJIT_UNLIKELY(dstRegKind >= kMaxVRegKinds))
      return DebugUtils::errored(kErrorInvalidState);

    WorkData& dstData = _workData[dstRegKind];
    uint32_t dstRegId = dstArg.getRegId();
    if (ASMJIT_UNLIKELY(dstRegId >= 32 || !(dstData.archRegs & Utils::mask(dstRegId))))
      return DebugUtils::errored(kErrorInvalidPhysId);

    uint32_t dstRegMask = Utils::mask(dstRegId);
    if (ASMJIT_UNLIKELY(dstData.usedRegs & dstRegMask))
      return DebugUtils::errored(kErrorOverlappedRegs);

    dstData.usedRegs |= dstRegMask;
    dstData.argIndex[dstRegId] = static_cast<uint8_t>(i);

    if (srcArg.byReg()) {
      uint32_t srcRegKind = X86Reg::kindOf(srcArg.getRegType());
      uint32_t srcRegId = srcArg.getRegId();
      uint32_t srcRegMask = Utils::mask(srcRegId);

      if (dstRegKind == srcRegKind) {
        // The best case, register is allocated where it is expected to be.
        if (dstRegId == srcRegId) continue;

        // Detect a register swap.
        if (dstData.usedRegs & srcRegMask) {
          const SrcArg& ref = func.getArg(dstData.argIndex[srcRegId]);
          if (ref.byReg() && X86Reg::kindOf(ref.getRegType()) == dstRegKind && ref.getRegId() == dstRegId) {
            dstData.numSwaps++;
            _hasRegSwaps = true;
          }
        }
        dstData.srcRegs |= srcRegMask;
      }
      else {
        if (ASMJIT_UNLIKELY(srcRegKind >= kMaxVRegKinds))
          return DebugUtils::errored(kErrorInvalidState);

        WorkData& srcData = _workData[srcRegKind];
        srcData.srcRegs |= srcRegMask;
      }
    }
    else {
      dstData.numStackArgs++;
      _hasStackArgs = true;
    }

    dstData.numOps++;
    dstData.dstRegs |= dstRegMask;
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::markDstRegsDirty(FuncFrameInfo& ffi) noexcept {
  for (uint32_t i = 0; i < kMaxVRegKinds; i++) {
    WorkData& wd = _workData[i];
    uint32_t regs = wd.usedRegs | wd.dstRegs;

    wd.workRegs |= regs;
    ffi.addDirtyRegs(i, regs);
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::markRegsForSwaps(FuncFrameInfo& ffi) noexcept {
  if (!_hasRegSwaps)
    return kErrorOk;

  // If some registers require swapping then select one dirty register that
  // can be used as a temporary. We can do it also without it (by using xors),
  // but using temporary is always safer and also faster approach.
  for (uint32_t i = 0; i < kMaxVRegKinds; i++) {
    // Skip all register kinds where swapping is natively supported (GP regs).
    if (i == X86Reg::kKindGp) continue;

    // Skip all register kinds that don't require swapping.
    WorkData& wd = _workData[i];
    if (!wd.numSwaps) continue;

    // Initially, pick some clobbered or dirty register.
    uint32_t workRegs = wd.workRegs;
    uint32_t regs = workRegs & ~(wd.usedRegs | wd.dstRegs);

    // If that didn't work out pick some register which is not in 'used'.
    if (!regs) regs = workRegs & ~wd.usedRegs;

    // If that didn't work out pick any other register that is allocable.
    // This last resort case will, however, result in marking one more
    // register dirty.
    if (!regs) regs = wd.archRegs & ~workRegs;

    // If that didn't work out we will have to use xors instead of moves.
    if (!regs) continue;

    uint32_t regMask = Utils::mask(Utils::findFirstBit(regs));
    wd.workRegs |= regMask;
    ffi.addDirtyRegs(i, regMask);
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::markStackArgsReg(FuncFrameInfo& ffi) noexcept {
  if (!_hasStackArgs)
    return kErrorOk;

  // Decide which register to use to hold the stack base address.
  if (!ffi.hasPreservedFP()) {
    WorkData& wd = _workData[X86Reg::kKindGp];
    uint32_t saRegId = ffi.getStackArgsRegId();
    uint32_t usedRegs = wd.usedRegs;

    if (saRegId != Globals::kInvalidRegId) {
      // Check if the user chosen SA register doesn't overlap with others.
      // However, it's fine if it overlaps with some 'dstMove' register.
      if (usedRegs & Utils::mask(saRegId))
        return DebugUtils::errored(kErrorOverlappingStackRegWithRegArg);
    }
    else {
      // Initially, pick some clobbered or dirty register that is neither
      // in 'used' and neither in 'dstMove'. That's the safest bet as the
      // register won't collide with anything right now.
      uint32_t regs = wd.workRegs & ~(usedRegs | wd.dstRegs);

      // If that didn't work out pick some register which is not in 'used'.
      if (!regs) regs = wd.workRegs & ~usedRegs;

      // If that didn't work out then we have to make one more register dirty.
      if (!regs) regs = wd.archRegs & ~wd.workRegs;

      // If that didn't work out we can't continue.
      if (ASMJIT_UNLIKELY(!regs))
        return DebugUtils::errored(kErrorNoMorePhysRegs);

      saRegId = Utils::findFirstBit(regs);
      ffi.setStackArgsRegId(saRegId);
    }
  }
  else {
    ffi.setStackArgsRegId(X86Gp::kIdBp);
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - CallConv]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::initCallConv(CallConv& cc, uint32_t ccId) noexcept {
  const uint32_t kKindGp  = X86Reg::kKindGp;
  const uint32_t kKindVec = X86Reg::kKindVec;
  const uint32_t kKindMm  = X86Reg::kKindMm;
  const uint32_t kKindK   = X86Reg::kKindK;

  const uint32_t kZax = X86Gp::kIdAx;
  const uint32_t kZbx = X86Gp::kIdBx;
  const uint32_t kZcx = X86Gp::kIdCx;
  const uint32_t kZdx = X86Gp::kIdDx;
  const uint32_t kZsp = X86Gp::kIdSp;
  const uint32_t kZbp = X86Gp::kIdBp;
  const uint32_t kZsi = X86Gp::kIdSi;
  const uint32_t kZdi = X86Gp::kIdDi;

  switch (ccId) {
    case CallConv::kIdX86StdCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      goto X86CallConv;

    case CallConv::kIdX86MsThisCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      cc.setPassedOrder(kKindGp, kZcx);
      goto X86CallConv;

    case CallConv::kIdX86MsFastCall:
    case CallConv::kIdX86GccFastCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      cc.setPassedOrder(kKindGp, kZcx, kZdx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm1:
      cc.setPassedOrder(kKindGp, kZax);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm2:
      cc.setPassedOrder(kKindGp, kZax, kZdx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm3:
      cc.setPassedOrder(kKindGp, kZax, kZdx, kZcx);
      goto X86CallConv;

    case CallConv::kIdX86CDecl:
X86CallConv:
      cc.setNaturalStackAlignment(4);
      cc.setArchType(ArchInfo::kTypeX86);
      cc.setPreservedRegs(kKindGp, Utils::mask(kZbx, kZsp, kZbp, kZsi, kZdi));
      break;

    case CallConv::kIdX86Win64:
      cc.setArchType(ArchInfo::kTypeX64);
      cc.setAlgorithm(CallConv::kAlgorithmWin64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec | CallConv::kFlagIndirectVecArgs);
      cc.setNaturalStackAlignment(16);
      cc.setSpillZoneSize(32);
      cc.setPassedOrder(kKindGp, kZcx, kZdx, 8, 9);
      cc.setPassedOrder(kKindVec, 0, 1, 2, 3);
      cc.setPreservedRegs(kKindGp, Utils::mask(kZbx, kZsp, kZbp, kZsi, kZdi, 12, 13, 14, 15));
      cc.setPreservedRegs(kKindVec, Utils::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
      break;

    case CallConv::kIdX86SysV64:
      cc.setArchType(ArchInfo::kTypeX64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setNaturalStackAlignment(16);
      cc.setRedZoneSize(128);
      cc.setPassedOrder(kKindGp, kZdi, kZsi, kZdx, kZcx, 8, 9);
      cc.setPassedOrder(kKindVec, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPreservedRegs(kKindGp, Utils::mask(kZbx, kZsp, kZbp, 12, 13, 14, 15));
      break;

    case CallConv::kIdX86FastEval2:
    case CallConv::kIdX86FastEval3:
    case CallConv::kIdX86FastEval4: {
      uint32_t n = ccId - CallConv::kIdX86FastEval2;

      cc.setArchType(ArchInfo::kTypeX86);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setNaturalStackAlignment(16);
      cc.setPassedOrder(kKindGp, kZax, kZdx, kZcx, kZsi, kZdi);
      cc.setPassedOrder(kKindMm, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPassedOrder(kKindVec, 0, 1, 2, 3, 4, 5, 6, 7);

      cc.setPreservedRegs(kKindGp , Utils::bits(8));
      cc.setPreservedRegs(kKindVec, Utils::bits(8) & ~Utils::bits(n));
      cc.setPreservedRegs(kKindMm , Utils::bits(8));
      cc.setPreservedRegs(kKindK  , Utils::bits(8));
      break;
    }

    case CallConv::kIdX64FastEval2:
    case CallConv::kIdX64FastEval3:
    case CallConv::kIdX64FastEval4: {
      uint32_t n = ccId - CallConv::kIdX64FastEval2;

      cc.setArchType(ArchInfo::kTypeX64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setNaturalStackAlignment(16);
      cc.setPassedOrder(kKindGp, kZax, kZdx, kZcx, kZsi, kZdi);
      cc.setPassedOrder(kKindMm, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPassedOrder(kKindVec, 0, 1, 2, 3, 4, 5, 6, 7);

      cc.setPreservedRegs(kKindGp , Utils::bits(16));
      cc.setPreservedRegs(kKindVec,~Utils::bits(n));
      cc.setPreservedRegs(kKindMm , Utils::bits(8));
      cc.setPreservedRegs(kKindK  , Utils::bits(8));
      break;
    }

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  cc.setId(ccId);
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - FuncDetail]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::initFuncDetail(FuncDetail& func, const FuncSignature& sign, uint32_t gpSize) noexcept {
  const CallConv& cc = func.getCallConv();
  uint32_t archType = cc.getArchType();

  uint32_t i;
  uint32_t argCount = func.getArgCount();

  if (func.getRetCount() != 0) {
    uint32_t typeId = func._rets[0].getTypeId();
    switch (typeId) {
      case TypeId::kI64:
      case TypeId::kU64: {
        if (archType == ArchInfo::kTypeX86) {
          // Convert a 64-bit return to two 32-bit returns.
          func._retCount = 2;
          typeId -= 2;

          // 64-bit value is returned in EDX:EAX on X86.
          func._rets[0].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdAx);
          func._rets[1].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdDx);
          break;
        }
        else {
          func._rets[0].initReg(typeId, X86Gp::kRegGpq, X86Gp::kIdAx);
        }
        break;
      }

      case TypeId::kI8:
      case TypeId::kU8:
      case TypeId::kI16:
      case TypeId::kU16:
      case TypeId::kI32:
      case TypeId::kU32: {
        func._rets[0].assignToReg(X86Gp::kRegGpd, X86Gp::kIdAx);
        break;
      }

      case TypeId::kF32:
      case TypeId::kF64: {
        uint32_t regType = (archType == ArchInfo::kTypeX86) ? X86Reg::kRegFp : X86Reg::kRegXmm;
        func._rets[0].assignToReg(regType, 0);
        break;
      }

      case TypeId::kF80: {
        // 80-bit floats are always returned by FP0.
        func._rets[0].assignToReg(X86Reg::kRegFp, 0);
        break;
      }

      case TypeId::kMmx32:
      case TypeId::kMmx64: {
        // On X64 MM register(s) are returned through XMM or GPQ (Win64).
        uint32_t regType = X86Reg::kRegMm;
        if (archType != ArchInfo::kTypeX86)
          regType = cc.getAlgorithm() == CallConv::kAlgorithmDefault ? X86Reg::kRegXmm : X86Reg::kRegGpq;

        func._rets[0].assignToReg(regType, 0);
        break;
      }

      default: {
        func._rets[0].assignToReg(x86VecTypeIdToRegType(typeId), 0);
        break;
      }
    }
  }

  uint32_t stackBase = gpSize;
  uint32_t stackOffset = stackBase + cc._spillZoneSize;

  if (cc.getAlgorithm() == CallConv::kAlgorithmDefault) {
    uint32_t gpzPos = 0;
    uint32_t vecPos = 0;

    for (i = 0; i < argCount; i++) {
      FuncDetail::Value& arg = func._args[i];
      uint32_t typeId = arg.getTypeId();

      if (TypeId::isInt(typeId)) {
        uint32_t regId = gpzPos < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindGp].id[gpzPos] : Globals::kInvalidRegId;
        if (regId != Globals::kInvalidRegId) {
          uint32_t regType = (typeId <= TypeId::kU32)
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;
          arg.assignToReg(regType, regId);
          func.addUsedRegs(X86Reg::kKindGp, Utils::mask(regId));
          gpzPos++;
        }
        else {
          uint32_t size = std::max<uint32_t>(TypeId::sizeOf(typeId), gpSize);
          arg.assignToStack(stackOffset);
          stackOffset += size;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = vecPos < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindVec].id[vecPos] : Globals::kInvalidRegId;

        // If this is a float, but `floatByVec` is false, we have to pass by stack.
        if (TypeId::isFloat(typeId) && !cc.hasFlag(CallConv::kFlagPassFloatsByVec))
          regId = Globals::kInvalidRegId;

        if (regId != Globals::kInvalidRegId) {
          arg.initReg(typeId, x86VecTypeIdToRegType(typeId), regId);
          func.addUsedRegs(X86Reg::kKindVec, Utils::mask(regId));
          vecPos++;
        }
        else {
          int32_t size = TypeId::sizeOf(typeId);
          arg.assignToStack(stackOffset);
          stackOffset += size;
        }
        continue;
      }
    }
  }

  if (cc.getAlgorithm() == CallConv::kAlgorithmWin64) {
    for (i = 0; i < argCount; i++) {
      FuncDetail::Value& arg = func._args[i];

      uint32_t typeId = arg.getTypeId();
      uint32_t size = TypeId::sizeOf(typeId);

      if (TypeId::isInt(typeId) || TypeId::isMmx(typeId)) {
        uint32_t regId = i < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindGp].id[i] : Globals::kInvalidRegId;
        if (regId != Globals::kInvalidRegId) {
          uint32_t regType = (size <= 4 && !TypeId::isMmx(typeId))
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;

          arg.assignToReg(regType, regId);
          func.addUsedRegs(X86Reg::kKindGp, Utils::mask(regId));
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += gpSize;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = i < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindVec].id[i] : Globals::kInvalidRegId;
        if (regId != Globals::kInvalidRegId && (TypeId::isFloat(typeId) || cc.hasFlag(CallConv::kFlagVectorCall))) {
          uint32_t regType = x86VecTypeIdToRegType(typeId);
          uint32_t regId = cc._passedOrder[X86Reg::kKindVec].id[i];

          arg.assignToReg(regType, regId);
          func.addUsedRegs(X86Reg::kKindVec, Utils::mask(regId));
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += 8; // Always 8 bytes (float/double).
        }
        continue;
      }
    }
  }

  func._argStackSize = stackOffset - stackBase;
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - FrameLayout]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::initFrameLayout(FuncFrameLayout& layout, const FuncDetail& func, const FuncFrameInfo& ffi) noexcept {
  layout.reset();

  uint32_t kind;
  uint32_t gpSize = (func.getCallConv().getArchType() == ArchInfo::kTypeX86) ? 4 : 8;

  // Calculate a bit-mask of all registers that must be saved & restored.
  for (kind = 0; kind < Globals::kMaxVRegKinds; kind++)
    layout._savedRegs[kind] = (ffi.getDirtyRegs(kind) & ~func.getPassedRegs(kind)) & func.getPreservedRegs(kind);

  // Include EBP|RBP if the function preserves the frame-pointer.
  if (ffi.hasPreservedFP()) {
    layout._preservedFP = true;
    layout._savedRegs[X86Reg::kKindGp] |= Utils::mask(X86Gp::kIdBp);
  }

  // Exclude ESP/RSP - this register is never included in saved-regs.
  layout._savedRegs[X86Reg::kKindGp] &= ~Utils::mask(X86Gp::kIdSp);

  // Calculate the final stack alignment.
  uint32_t stackAlignment =
    std::max<uint32_t>(
      std::max<uint32_t>(
        ffi.getStackFrameAlignment(),
        ffi.getCallFrameAlignment()),
      func.getCallConv().getNaturalStackAlignment());
  layout._stackAlignment = static_cast<uint8_t>(stackAlignment);

  // Calculate if dynamic stack alignment is required. If true the function has
  // to align stack dynamically to match `_stackAlignment` and would require to
  // access its stack-based arguments through `_stackArgsRegId`.
  bool dsa = stackAlignment > func.getCallConv().getNaturalStackAlignment() && stackAlignment >= 16;
  layout._dynamicAlignment = dsa;

  // This flag describes if the prolog inserter must store the previous ESP|RSP
  // to stack so the epilog inserter can load the stack from it before returning.
  bool dsaSlotUsed = dsa && !ffi.hasPreservedFP();
  layout._dsaSlotUsed = dsaSlotUsed;

  // These two are identical if the function doesn't align its stack dynamically.
  uint32_t stackArgsRegId = ffi.getStackArgsRegId();
  if (stackArgsRegId == Globals::kInvalidRegId)
    stackArgsRegId = X86Gp::kIdSp;

  // Fix stack arguments base-register from ESP|RSP to EBP|RBP in case it was
  // not picked before and the function performs dynamic stack alignment.
  if (dsa && stackArgsRegId == X86Gp::kIdSp)
    stackArgsRegId = X86Gp::kIdBp;

  if (stackArgsRegId != X86Gp::kIdSp)
    layout._savedRegs[X86Reg::kKindGp] |= Utils::mask(stackArgsRegId) & func.getPreservedRegs(X86Gp::kKindGp);

  layout._stackBaseRegId = X86Gp::kIdSp;
  layout._stackArgsRegId = static_cast<uint8_t>(stackArgsRegId);

  // Setup stack size used to save preserved registers.
  layout._gpStackSize  = Utils::bitCount(layout.getSavedRegs(X86Reg::kKindGp )) * gpSize;
  layout._vecStackSize = Utils::bitCount(layout.getSavedRegs(X86Reg::kKindVec)) * 16 +
                         Utils::bitCount(layout.getSavedRegs(X86Reg::kKindMm )) *  8 ;

  uint32_t v = 0;                        // The beginning of the stack frame, aligned to CallFrame alignment.
  v += ffi._callFrameSize;               // Count '_callFrameSize'  <- This is used to call functions.
  v  = Utils::alignTo(v, stackAlignment);// Align to function's SA

  layout._stackBaseOffset = v;           // Store '_stackBaseOffset'<- Function's own stack starts here..
  v += ffi._stackFrameSize;              // Count '_stackFrameSize' <- Function's own stack ends here.

  // If the function is aligned, calculate the alignment necessary to store
  // vector registers, and set `FuncFrameInfo::kX86FlagAlignedVecSR` to inform
  // PrologEpilog inserter that it can use instructions to perform aligned
  // stores/loads to save/restore VEC registers.
  if (stackAlignment >= 16 && layout._vecStackSize) {
    v = Utils::alignTo(v, 16);           // Align '_vecStackOffset'.
    layout._alignedVecSR = true;
  }

  layout._vecStackOffset = v;            // Store '_vecStackOffset' <- Functions VEC Save|Restore starts here.
  v += layout._vecStackSize;             // Count '_vecStackSize'   <- Functions VEC Save|Restore ends here.

  if (dsaSlotUsed) {
    layout._dsaSlot = v;                 // Store '_dsaSlot'        <- Old stack pointer is stored here.
    v += gpSize;
  }

  // The return address should be stored after GP save/restore regs. It has
  // the same size as `gpSize` (basically the native register/pointer size).
  // We don't adjust it now as `v` now contains the exact size that the
  // function requires to adjust (call frame + stack frame, vec stack size).
  // The stack (if we consider this size) is misaligned now, as it's always
  // aligned before the function call - when `call()` is executed it pushes
  // the current EIP|RIP onto the stack, and misaligns it by 12 or 8 bytes
  // (depending on the architecture). So count number of bytes needed to align
  // it up to the function's CallFrame (the beginning).
  if (v || ffi.hasCalls())
    v += Utils::alignDiff(v + layout._gpStackSize + gpSize, stackAlignment);

  layout._stackAdjustment = v;           // Store '_stackAdjustment'<- SA used by 'add zsp, SA' and 'sub zsp, SA'.
  layout._gpStackOffset = v;             // Store '_gpStackOffset'  <- Functions GP Save|Restore starts here.
  v += layout._gpStackSize;              // Count '_gpStackSize'    <- Functions GP Save|Restore ends here.

  v += gpSize;                           // Count 'ReturnAddress'.
  v += func.getSpillZoneSize();          // Count 'SpillZoneSize'.

  // Calculate where function arguments start, relative to the stackArgsRegId.
  // If the register that will be used to access arguments passed by stack is
  // ESP|RSP then it's exactly where we are now, otherwise we must calculate
  // how many 'push regs' we did and adjust it based on that.
  uint32_t stackArgsOffset = v;
  if (stackArgsRegId != X86Gp::kIdSp) {
    if (ffi.hasPreservedFP())
      stackArgsOffset = gpSize;
    else
      stackArgsOffset = layout._gpStackSize;
  }
  layout._stackArgsOffset = stackArgsOffset;

  // If the function does dynamic stack adjustment then the stack-adjustment
  // must be aligned.
  if (dsa)
    layout._stackAdjustment = Utils::alignTo(layout._stackAdjustment, stackAlignment);

  // Initialize variables based on CallConv flags.
  if (func.hasFlag(CallConv::kFlagCalleePopsStack))
    layout._calleeStackCleanup = static_cast<uint16_t>(func.getArgStackSize());

  // Initialize variables based on FFI flags.
  layout._mmxCleanup = ffi.hasMmxCleanup();
  layout._avxEnabled = ffi.isAvxEnabled();
  layout._avxCleanup = ffi.hasAvxCleanup();

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - ArgsToFrameInfo]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::argsToFrameInfo(const FuncArgsMapper& args, FuncFrameInfo& ffi) noexcept {
  X86FuncArgsContext ctx;
  ASMJIT_PROPAGATE(ctx.initWorkData(args, ffi._dirtyRegs, ffi.hasPreservedFP()));

  ASMJIT_PROPAGATE(ctx.markDstRegsDirty(ffi));
  ASMJIT_PROPAGATE(ctx.markRegsForSwaps(ffi));
  ASMJIT_PROPAGATE(ctx.markStackArgsReg(ffi));
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - Emit Helpers]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::emitRegMove(X86Emitter* emitter,
  const Operand_& dst_,
  const Operand_& src_, uint32_t typeId, bool avxEnabled, const char* comment) {

  // Invalid or abstract TypeIds are not allowed.
  ASMJIT_ASSERT(TypeId::isValid(typeId) && !TypeId::isAbstract(typeId));

  Operand dst(dst_);
  Operand src(src_);

  uint32_t instId = Inst::kIdNone;
  uint32_t memFlags = 0;

  enum MemFlags {
    kDstMem = 0x1,
    kSrcMem = 0x2
  };

  // Detect memory operands and patch them to have the same size as the register.
  // CodeCompiler always sets memory size of allocs and spills, so it shouldn't
  // be really necessary, however, after this function was separated from Compiler
  // it's better to make sure that the size is always specified, as we can use
  // 'movzx' and 'movsx' that rely on it.
  if (dst.isMem()) { memFlags |= kDstMem; dst.as<X86Mem>().setSize(src.getSize()); }
  if (src.isMem()) { memFlags |= kSrcMem; src.as<X86Mem>().setSize(dst.getSize()); }

  switch (typeId) {
    case TypeId::kI8:
    case TypeId::kU8:
    case TypeId::kI16:
    case TypeId::kU16:
      // Special case - 'movzx' load.
      if (memFlags & kSrcMem) {
        instId = X86Inst::kIdMovzx;
        dst.setSignature(X86RegTraits<X86Reg::kRegGpd>::kSignature);
      }
      else if (!memFlags) {
        // Change both destination and source registers to GPD (safer, no dependencies).
        dst.setSignature(X86RegTraits<X86Reg::kRegGpd>::kSignature);
        src.setSignature(X86RegTraits<X86Reg::kRegGpd>::kSignature);
      }
      ASMJIT_FALLTHROUGH;

    case TypeId::kI32:
    case TypeId::kU32:
    case TypeId::kI64:
    case TypeId::kU64:
      instId = X86Inst::kIdMov;
      break;

    case TypeId::kMmx32:
      instId = X86Inst::kIdMovd;
      if (memFlags) break;
      ASMJIT_FALLTHROUGH;
    case TypeId::kMmx64 : instId = X86Inst::kIdMovq ; break;
    case TypeId::kMask8 : instId = X86Inst::kIdKmovb; break;
    case TypeId::kMask16: instId = X86Inst::kIdKmovw; break;
    case TypeId::kMask32: instId = X86Inst::kIdKmovd; break;
    case TypeId::kMask64: instId = X86Inst::kIdKmovq; break;

    default: {
      uint32_t elementTypeId = TypeId::elementOf(typeId);
      if (TypeId::isVec32(typeId) && memFlags) {
        if (elementTypeId == TypeId::kF32)
          instId = avxEnabled ? X86Inst::kIdVmovss : X86Inst::kIdMovss;
        else
          instId = avxEnabled ? X86Inst::kIdVmovd : X86Inst::kIdMovd;
        break;
      }

      if (TypeId::isVec64(typeId) && memFlags) {
        if (elementTypeId == TypeId::kF64)
          instId = avxEnabled ? X86Inst::kIdVmovsd : X86Inst::kIdMovsd;
        else
          instId = avxEnabled ? X86Inst::kIdVmovq : X86Inst::kIdMovq;
        break;
      }

      if (elementTypeId == TypeId::kF32)
        instId = avxEnabled ? X86Inst::kIdVmovaps : X86Inst::kIdMovaps;
      else if (elementTypeId == TypeId::kF64)
        instId = avxEnabled ? X86Inst::kIdVmovapd : X86Inst::kIdMovapd;
      else if (typeId <= TypeId::_kVec256End)
        instId = avxEnabled ? X86Inst::kIdVmovdqa : X86Inst::kIdMovdqa;
      else if (elementTypeId <= TypeId::kU32)
        instId = X86Inst::kIdVmovdqa32;
      else
        instId = X86Inst::kIdVmovdqa64;
      break;
    }
  }

  if (!instId)
    return DebugUtils::errored(kErrorInvalidState);

  emitter->setInlineComment(comment);
  return emitter->emit(instId, dst, src);
}

ASMJIT_FAVOR_SIZE Error X86Internal::emitArgMove(X86Emitter* emitter,
  const X86Reg& dst_, uint32_t dstTypeId,
  const Operand_& src_, uint32_t srcTypeId, bool avxEnabled, const char* comment) {

  // Deduce optional `dstTypeId`, which may be `TypeId::kVoid` in some cases.
  if (!dstTypeId) dstTypeId = x86OpData.archRegs.regTypeToTypeId[dst_.getType()];

  // Invalid or abstract TypeIds are not allowed.
  ASMJIT_ASSERT(TypeId::isValid(dstTypeId) && !TypeId::isAbstract(dstTypeId));
  ASMJIT_ASSERT(TypeId::isValid(srcTypeId) && !TypeId::isAbstract(srcTypeId));

  X86Reg dst(dst_);
  Operand src(src_);

  uint32_t dstSize = TypeId::sizeOf(dstTypeId);
  uint32_t srcSize = TypeId::sizeOf(srcTypeId);

  int32_t instId = Inst::kIdNone;

  // Not a real loop, just 'break' is nicer than 'goto'.
  for (;;) {
    if (TypeId::isInt(dstTypeId)) {
      if (TypeId::isInt(srcTypeId)) {
        instId = X86Inst::kIdMovsx;
        uint32_t typeOp = (dstTypeId << 8) | srcTypeId;

        // Sign extend by using 'movsx'.
        if (typeOp == ((TypeId::kI16 << 8) | TypeId::kI8 ) ||
            typeOp == ((TypeId::kI32 << 8) | TypeId::kI8 ) ||
            typeOp == ((TypeId::kI32 << 8) | TypeId::kI16) ||
            typeOp == ((TypeId::kI64 << 8) | TypeId::kI8 ) ||
            typeOp == ((TypeId::kI64 << 8) | TypeId::kI16)) break;

        // Sign extend by using 'movsxd'.
        instId = X86Inst::kIdMovsxd;
        if (typeOp == ((TypeId::kI64 << 8) | TypeId::kI32)) break;
      }

      if (TypeId::isInt(srcTypeId) || src_.isMem()) {
        // Zero extend by using 'movzx' or 'mov'.
        if (dstSize <= 4 && srcSize < 4) {
          instId = X86Inst::kIdMovzx;
          dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        }
        else {
          // We should have caught all possibilities where `srcSize` is less
          // than 4, so we don't have to worry about 'movzx' anymore. Minimum
          // size is enough to determine if we want 32-bit or 64-bit move.
          instId = X86Inst::kIdMov;
          srcSize = std::min(srcSize, dstSize);

          dst.setSignature(srcSize == 4 ? X86Reg::signatureOfT<X86Reg::kRegGpd>()
                                        : X86Reg::signatureOfT<X86Reg::kRegGpq>());
          if (src.isReg()) src.setSignature(dst.getSignature());
        }
        break;
      }

      // NOTE: The previous branch caught all memory sources, from here it's
      // always register to register conversion, so catch the remaining cases.
      srcSize = std::min(srcSize, dstSize);

      if (TypeId::isMmx(srcTypeId)) {
        // 64-bit move.
        instId = X86Inst::kIdMovq;
        if (srcSize == 8) break;

        // 32-bit move.
        instId = X86Inst::kIdMovd;
        dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }

      if (TypeId::isMask(srcTypeId)) {
        instId = X86Inst::kmovIdFromSize(srcSize);
        dst.setSignature(srcSize <= 4 ? X86Reg::signatureOfT<X86Reg::kRegGpd>()
                                      : X86Reg::signatureOfT<X86Reg::kRegGpq>());
        break;
      }

      if (TypeId::isVec(srcTypeId)) {
        // 64-bit move.
        instId = avxEnabled ? X86Inst::kIdVmovq : X86Inst::kIdMovq;
        if (srcSize == 8) break;

        // 32-bit move.
        instId = avxEnabled ? X86Inst::kIdVmovd : X86Inst::kIdMovd;
        dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }
    }

    if (TypeId::isMmx(dstTypeId)) {
      instId = X86Inst::kIdMovq;
      srcSize = std::min(srcSize, dstSize);

      if (TypeId::isInt(srcTypeId) || src.isMem()) {
        // 64-bit move.
        if (srcSize == 8) break;

        // 32-bit move.
        instId = X86Inst::kIdMovd;
        if (src.isReg()) src.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }

      if (TypeId::isMmx(srcTypeId)) break;

      // NOTE: This will hurt if `avxEnabled`.
      instId = X86Inst::kIdMovdq2q;
      if (TypeId::isVec(srcTypeId)) break;
    }

    if (TypeId::isMask(dstTypeId)) {
      srcSize = std::min(srcSize, dstSize);

      if (TypeId::isInt(srcTypeId) || TypeId::isMask(srcTypeId) || src.isMem()) {
        instId = X86Inst::kmovIdFromSize(srcSize);
        if (X86Reg::isGp(src) && srcSize <= 4) src.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }
    }

    if (TypeId::isVec(dstTypeId)) {
      // By default set destination to XMM, will be set to YMM|ZMM if needed.
      dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegXmm>());

      // NOTE: This will hurt if `avxEnabled`.
      if (X86Reg::isMm(src)) {
        // 64-bit move.
        instId = X86Inst::kIdMovq2dq;
        break;
      }

      // Argument conversion.
      uint32_t dstElement = TypeId::elementOf(dstTypeId);
      uint32_t srcElement = TypeId::elementOf(srcTypeId);

      if (dstElement == TypeId::kF32 && srcElement == TypeId::kF64) {
        srcSize = std::min(dstSize * 2, srcSize);
        dstSize = srcSize / 2;

        if (srcSize <= 8)
          instId = avxEnabled ? X86Inst::kIdVcvtss2sd : X86Inst::kIdCvtss2sd;
        else
          instId = avxEnabled ? X86Inst::kIdVcvtps2pd : X86Inst::kIdCvtps2pd;

        if (dstSize == 32)
          dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegYmm>());
        if (src.isReg())
          src.setSignature(X86Reg::signatureOfVecBySize(srcSize));
        break;
      }

      if (dstElement == TypeId::kF64 && srcElement == TypeId::kF32) {
        srcSize = std::min(dstSize, srcSize * 2) / 2;
        dstSize = srcSize * 2;

        if (srcSize <= 4)
          instId = avxEnabled ? X86Inst::kIdVcvtsd2ss : X86Inst::kIdCvtsd2ss;
        else
          instId = avxEnabled ? X86Inst::kIdVcvtpd2ps : X86Inst::kIdCvtpd2ps;

        dst.setSignature(X86Reg::signatureOfVecBySize(dstSize));
        if (src.isReg() && srcSize >= 32)
          src.setSignature(X86Reg::signatureOfT<X86Reg::kRegYmm>());
        break;
      }

      srcSize = std::min(srcSize, dstSize);
      if (X86Reg::isGp(src) || src.isMem()) {
        // 32-bit move.
        if (srcSize <= 4) {
          instId = avxEnabled ? X86Inst::kIdVmovd : X86Inst::kIdMovd;
          if (src.isReg()) src.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
          break;
        }

        // 64-bit move.
        if (srcSize == 8) {
          instId = avxEnabled ? X86Inst::kIdVmovq : X86Inst::kIdMovq;
          break;
        }
      }

      if (X86Reg::isVec(src) || src.isMem()) {
        instId = avxEnabled ? X86Inst::kIdVmovaps : X86Inst::kIdMovaps;
        uint32_t sign = X86Reg::signatureOfVecBySize(srcSize);

        dst.setSignature(sign);
        if (src.isReg()) src.setSignature(sign);
        break;
      }
    }

    return DebugUtils::errored(kErrorInvalidState);
  }

  if (src.isMem())
    src.as<X86Mem>().setSize(srcSize);

  emitter->setInlineComment(comment);
  return emitter->emit(instId, dst, src);
}

// ============================================================================
// [asmjit::X86Internal - Emit Prolog & Epilog]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::emitProlog(X86Emitter* emitter, const FuncFrameLayout& layout) {
  uint32_t gpSaved = layout.getSavedRegs(X86Reg::kKindGp);

  X86Gp zsp = emitter->zsp();   // ESP|RSP register.
  X86Gp zbp = emitter->zsp();   // EBP|RBP register.
  zbp.setId(X86Gp::kIdBp);

  X86Gp gpReg = emitter->zsp(); // General purpose register (temporary).
  X86Gp saReg = emitter->zsp(); // Stack-arguments base register.

  // Emit: 'push zbp'
  //       'mov  zbp, zsp'.
  if (layout.hasPreservedFP()) {
    gpSaved &= ~Utils::mask(X86Gp::kIdBp);
    ASMJIT_PROPAGATE(emitter->push(zbp));
    ASMJIT_PROPAGATE(emitter->mov(zbp, zsp));
  }

  // Emit: 'push gp' sequence.
  if (gpSaved) {
    for (uint32_t i = gpSaved, regId = 0; i; i >>= 1, regId++) {
      if (!(i & 0x1)) continue;
      gpReg.setId(regId);
      ASMJIT_PROPAGATE(emitter->push(gpReg));
    }
  }

  // Emit: 'mov saReg, zsp'.
  uint32_t stackArgsRegId = layout.getStackArgsRegId();
  if (stackArgsRegId != Globals::kInvalidRegId && stackArgsRegId != X86Gp::kIdSp) {
    saReg.setId(stackArgsRegId);
    if (!(layout.hasPreservedFP() && stackArgsRegId == X86Gp::kIdBp))
      ASMJIT_PROPAGATE(emitter->mov(saReg, zsp));
  }

  // Emit: 'and zsp, StackAlignment'.
  if (layout.hasDynamicAlignment())
    ASMJIT_PROPAGATE(emitter->and_(zsp, -static_cast<int32_t>(layout.getStackAlignment())));

  // Emit: 'sub zsp, StackAdjustment'.
  if (layout.hasStackAdjustment())
    ASMJIT_PROPAGATE(emitter->sub(zsp, layout.getStackAdjustment()));

  // Emit: 'mov [zsp + dsaSlot], saReg'.
  if (layout.hasDynamicAlignment() && layout.hasDsaSlotUsed()) {
    X86Mem saMem = x86::ptr(zsp, layout._dsaSlot);
    ASMJIT_PROPAGATE(emitter->mov(saMem, saReg));
  }

  // Emit 'movaps|movups [zsp + X], xmm0..15'.
  uint32_t xmmSaved = layout.getSavedRegs(X86Reg::kKindVec);
  if (xmmSaved) {
    X86Mem vecBase = x86::ptr(zsp, layout.getVecStackOffset());
    X86Reg vecReg = x86::xmm(0);

    uint32_t vecInst = x86GetXmmMovInst(layout);
    uint32_t vecSize = 16;

    for (uint32_t i = xmmSaved, regId = 0; i; i >>= 1, regId++) {
      if (!(i & 0x1)) continue;
      vecReg.setId(regId);
      ASMJIT_PROPAGATE(emitter->emit(vecInst, vecBase, vecReg));
      vecBase.addOffsetLo32(static_cast<int32_t>(vecSize));
    }
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86Internal::emitEpilog(X86Emitter* emitter, const FuncFrameLayout& layout) {
  uint32_t i;
  uint32_t regId;

  uint32_t gpSize = emitter->getGpSize();
  uint32_t gpSaved = layout.getSavedRegs(X86Reg::kKindGp);

  X86Gp zsp = emitter->zsp();   // ESP|RSP register.
  X86Gp zbp = emitter->zsp();   // EBP|RBP register.
  zbp.setId(X86Gp::kIdBp);

  X86Gp gpReg = emitter->zsp(); // General purpose register (temporary).

  // Don't emit 'pop zbp' in the pop sequence, this case is handled separately.
  if (layout.hasPreservedFP()) gpSaved &= ~Utils::mask(X86Gp::kIdBp);

  // Emit 'movaps|movups xmm0..15, [zsp + X]'.
  uint32_t xmmSaved = layout.getSavedRegs(X86Reg::kKindVec);
  if (xmmSaved) {
    X86Mem vecBase = x86::ptr(zsp, layout.getVecStackOffset());
    X86Reg vecReg = x86::xmm(0);

    uint32_t vecInst = x86GetXmmMovInst(layout);
    uint32_t vecSize = 16;

    for (i = xmmSaved, regId = 0; i; i >>= 1, regId++) {
      if (!(i & 0x1)) continue;
      vecReg.setId(regId);
      ASMJIT_PROPAGATE(emitter->emit(vecInst, vecReg, vecBase));
      vecBase.addOffsetLo32(static_cast<int32_t>(vecSize));
    }
  }

  // Emit 'emms' and 'vzeroupper'.
  if (layout.hasMmxCleanup()) ASMJIT_PROPAGATE(emitter->emms());
  if (layout.hasAvxCleanup()) ASMJIT_PROPAGATE(emitter->vzeroupper());

  if (layout.hasPreservedFP()) {
    // Emit 'mov zsp, zbp' or 'lea zsp, [zbp - x]'
    int32_t count = static_cast<int32_t>(layout.getGpStackSize() - gpSize);
    if (!count)
      ASMJIT_PROPAGATE(emitter->mov(zsp, zbp));
    else
      ASMJIT_PROPAGATE(emitter->lea(zsp, x86::ptr(zbp, -count)));
  }
  else {
    if (layout.hasDynamicAlignment() && layout.hasDsaSlotUsed()) {
      // Emit 'mov zsp, [zsp + DsaSlot]'.
      X86Mem saMem = x86::ptr(zsp, layout._dsaSlot);
      ASMJIT_PROPAGATE(emitter->mov(zsp, saMem));
    }
    else if (layout.hasStackAdjustment()) {
      // Emit 'add zsp, StackAdjustment'.
      ASMJIT_PROPAGATE(emitter->add(zsp, static_cast<int32_t>(layout.getStackAdjustment())));
    }
  }

  // Emit 'pop gp' sequence.
  if (gpSaved) {
    i = gpSaved;
    regId = 16;

    do {
      regId--;
      if (i & 0x8000) {
        gpReg.setId(regId);
        ASMJIT_PROPAGATE(emitter->pop(gpReg));
      }
      i <<= 1;
    } while (regId != 0);
  }

  // Emit 'pop zbp'.
  if (layout.hasPreservedFP()) ASMJIT_PROPAGATE(emitter->pop(zbp));

  // Emit 'ret' or 'ret x'.
  if (layout.hasCalleeStackCleanup())
    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdRet, static_cast<int>(layout.getCalleeStackCleanup())));
  else
    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdRet));

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - AllocArgs]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::allocArgs(X86Emitter* emitter, const FuncFrameLayout& layout, const FuncArgsMapper& args) {
  typedef X86FuncArgsContext::SrcArg SrcArg;
  typedef X86FuncArgsContext::DstArg DstArg;
  typedef X86FuncArgsContext::WorkData WorkData;
  enum { kMaxVRegKinds = Globals::kMaxVRegKinds };

  uint32_t i;
  const FuncDetail& func = *args.getFuncDetail();

  X86FuncArgsContext ctx;
  ASMJIT_PROPAGATE(ctx.initWorkData(args, layout._savedRegs, layout.hasPreservedFP()));

  // We must honor AVX if it's enabled.
  bool avxEnabled = layout.isAvxEnabled();

  // Free registers that can be used as temporaries and during shuffling.
  // We initialize them to match all workRegs (registers that can be used
  // by the function) except source regs, which are used to pass arguments.
  // Free registers are changed during shuffling - when an argument is moved
  // to the final register then the register itself is removed from freeRegs
  // (it can't be altered anymore during shuffling).
  uint32_t freeRegs[kMaxVRegKinds];
  for (i = 0; i < kMaxVRegKinds; i++)
    freeRegs[i] = ctx._workData[i].workRegs & ~ctx._workData[i].srcRegs;

  // This is an iterative process that runs until there is a work to do. When
  // one register is moved it can create space for another move. Such moves can
  // depend on each other so the algorithm may run multiple times before all
  // arguments are in place. This part does only register-to-register work,
  // arguments moved from stack-to-register area handled later.
  for (;;) {
    bool hasWork = false; // Do we have a work to do?
    bool didWork = false; // If we did something...

    uint32_t dstRegKind = kMaxVRegKinds;
    do {
      WorkData& wd = ctx._workData[--dstRegKind];
      if (wd.numOps > wd.numStackArgs) {
        hasWork = true;

        // Iterate over all destination regs and check if we can do something.
        // We always go from destination to source, never the opposite.
        uint32_t regsToDo = wd.dstRegs;
        do {
          // If there is a work to do there has to be at least one dstReg.
          ASMJIT_ASSERT(regsToDo != 0);
          uint32_t dstRegId = Utils::findFirstBit(regsToDo);
          uint32_t dstRegMask = Utils::mask(dstRegId);

          uint32_t argIndex = wd.argIndex[dstRegId];
          const DstArg& dstArg = args.getArg(argIndex);
          const SrcArg& srcArg = func.getArg(argIndex);

          if (srcArg.byReg()) {
            uint32_t srcRegType = srcArg.getRegType();
            uint32_t srcRegKind = X86Reg::kindOf(srcRegType);

            if (freeRegs[dstRegKind] & dstRegMask) {
              X86Reg dstReg(X86Reg::fromTypeAndId(dstArg.getRegType(), dstRegId));
              X86Reg srcReg(X86Reg::fromTypeAndId(srcRegType, srcArg.getRegId()));

              ASMJIT_PROPAGATE(
                emitArgMove(emitter,
                  dstReg, dstArg.getTypeId(),
                  srcReg, srcArg.getTypeId(), avxEnabled));
              freeRegs[dstRegKind] ^= dstRegMask;                     // Make the DST reg occupied.
              freeRegs[srcRegKind] |= Utils::mask(srcArg.getRegId()); // Make the SRC reg free.

              ASMJIT_ASSERT(wd.numOps >= 1);
              wd.numOps--;
              didWork = true;
            }
            else {
              // Check if this is a swap operation.
              if (dstRegKind == srcRegKind) {
                uint32_t srcRegId = srcArg.getRegId();

                uint32_t otherIndex = wd.argIndex[srcRegId];
                const DstArg& otherArg = args.getArg(otherIndex);

                if (otherArg.getRegId() == srcRegId && X86Reg::kindOf(otherArg.getRegType()) == dstRegKind) {
                  // If this is GP reg it can be handled by 'xchg'.
                  if (dstRegKind == X86Reg::kKindGp) {
                    uint32_t highestType = std::max(dstArg.getRegType(), srcRegType);

                    X86Reg dstReg = x86::gpd(dstRegId);
                    X86Reg srcReg = x86::gpd(srcRegId);

                    if (highestType == X86Reg::kRegGpq) {
                      dstReg.setSignature(X86RegTraits<X86Reg::kRegGpq>::kSignature);
                      srcReg.setSignature(X86RegTraits<X86Reg::kRegGpq>::kSignature);
                    }
                    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdXchg, dstReg, srcReg));
                    regsToDo &= ~Utils::mask(srcRegId);
                    freeRegs[dstRegKind] &= ~(Utils::mask(srcRegId) | dstRegMask);

                    ASMJIT_ASSERT(wd.numOps >= 2);
                    ASMJIT_ASSERT(wd.numSwaps >= 1);
                    wd.numOps-=2;
                    wd.numSwaps--;
                    didWork = true;
                  }
                }
              }
            }
          }

          // Clear the reg in `regsToDo` and continue if there are more.
          regsToDo ^= dstRegMask;
        } while (regsToDo);
      }
    } while (dstRegKind);

    if (!hasWork)
      break;

    if (!didWork)
      return DebugUtils::errored(kErrorInvalidState);
  }

  // Load arguments passed by stack into registers. This is pretty simple and
  // it never requires multiple iterations like the previous phase.
  if (ctx._hasStackArgs) {
    // Base address of all arguments passed by stack.
    X86Mem saBase = x86::ptr(emitter->gpz(layout.getStackArgsRegId()), layout.getStackArgsOffset());

    uint32_t dstRegKind = kMaxVRegKinds;
    do {
      WorkData& wd = ctx._workData[--dstRegKind];
      if (wd.numStackArgs) {
        // Iterate over all destination regs and check if we can do something.
        // We always go from destination to source, never the opposite.
        uint32_t regsToDo = wd.dstRegs;
        do {
          // If there is a work to do there has to be at least one dstReg.
          ASMJIT_ASSERT(regsToDo != 0);
          ASMJIT_ASSERT(wd.numOps > 0);

          uint32_t dstRegId = Utils::findFirstBit(regsToDo);
          uint32_t dstRegMask = Utils::mask(dstRegId);

          uint32_t argIndex = wd.argIndex[dstRegId];
          const DstArg& dstArg = args.getArg(argIndex);
          const SrcArg& srcArg = func.getArg(argIndex);

          // Only arguments passed by stack should remain, also the destination
          // registers must be free now (otherwise the first part of the algorithm
          // failed). Ideally this should be assert, but it's much safer to enforce
          // this in release as well.
          if (!srcArg.byStack() || !(freeRegs[dstRegKind] & dstRegMask))
            return DebugUtils::errored(kErrorInvalidState);

          X86Reg dstReg = X86Reg::fromTypeAndId(dstArg.getRegType(), dstRegId);
          X86Mem srcMem = saBase.adjusted(srcArg.getStackOffset());

          ASMJIT_PROPAGATE(
            emitArgMove(emitter,
              dstReg, dstArg.getTypeId(),
              srcMem, srcArg.getTypeId(), avxEnabled));

          freeRegs[dstRegKind] ^= dstRegMask;
          regsToDo ^= dstRegMask;
          wd.numOps--;
        } while (regsToDo);
      }
    } while (dstRegKind);
  }

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
