// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/misc_p.h"
#include "../base/utils.h"
#include "../x86/x86instimpl_p.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86InstImpl - Validate]
// ============================================================================

#if !defined(ASMJIT_DISABLE_VALIDATION)
template<uint32_t RegType>
struct X86OpTypeFromRegTypeT {
  enum {
    kValue = (RegType == X86Reg::kRegGpbLo) ? X86Inst::kOpGpbLo :
             (RegType == X86Reg::kRegGpbHi) ? X86Inst::kOpGpbHi :
             (RegType == X86Reg::kRegGpw  ) ? X86Inst::kOpGpw   :
             (RegType == X86Reg::kRegGpd  ) ? X86Inst::kOpGpd   :
             (RegType == X86Reg::kRegGpq  ) ? X86Inst::kOpGpq   :
             (RegType == X86Reg::kRegXmm  ) ? X86Inst::kOpXmm   :
             (RegType == X86Reg::kRegYmm  ) ? X86Inst::kOpYmm   :
             (RegType == X86Reg::kRegZmm  ) ? X86Inst::kOpZmm   :
             (RegType == X86Reg::kRegRip  ) ? X86Inst::kOpNone  :
             (RegType == X86Reg::kRegSeg  ) ? X86Inst::kOpSeg   :
             (RegType == X86Reg::kRegFp   ) ? X86Inst::kOpFp    :
             (RegType == X86Reg::kRegMm   ) ? X86Inst::kOpMm    :
             (RegType == X86Reg::kRegK    ) ? X86Inst::kOpK     :
             (RegType == X86Reg::kRegBnd  ) ? X86Inst::kOpBnd   :
             (RegType == X86Reg::kRegCr   ) ? X86Inst::kOpCr    :
             (RegType == X86Reg::kRegDr   ) ? X86Inst::kOpDr    : X86Inst::kOpNone
  };
};

template<uint32_t RegType>
struct X86RegMaskFromRegTypeT {
  enum {
    kMask = (RegType == X86Reg::kRegGpbLo) ? 0x0000000FU :
            (RegType == X86Reg::kRegGpbHi) ? 0x0000000FU :
            (RegType == X86Reg::kRegGpw  ) ? 0x000000FFU :
            (RegType == X86Reg::kRegGpd  ) ? 0x000000FFU :
            (RegType == X86Reg::kRegGpq  ) ? 0x000000FFU :
            (RegType == X86Reg::kRegXmm  ) ? 0x000000FFU :
            (RegType == X86Reg::kRegYmm  ) ? 0x000000FFU :
            (RegType == X86Reg::kRegZmm  ) ? 0x000000FFU :
            (RegType == X86Reg::kRegRip  ) ? 0x00000001U :
            (RegType == X86Reg::kRegSeg  ) ? 0x0000007EU : // [ES|CS|SS|DS|FS|GS]
            (RegType == X86Reg::kRegFp   ) ? 0x000000FFU :
            (RegType == X86Reg::kRegMm   ) ? 0x000000FFU :
            (RegType == X86Reg::kRegK    ) ? 0x000000FFU :
            (RegType == X86Reg::kRegBnd  ) ? 0x0000000FU :
            (RegType == X86Reg::kRegCr   ) ? 0x0000FFFFU :
            (RegType == X86Reg::kRegDr   ) ? 0x000000FFU : X86Inst::kOpNone
  };
};

template<uint32_t RegType>
struct X64RegMaskFromRegTypeT {
  enum {
    kMask = (RegType == X86Reg::kRegGpbLo) ? 0x0000FFFFU :
            (RegType == X86Reg::kRegGpbHi) ? 0x0000000FU :
            (RegType == X86Reg::kRegGpw  ) ? 0x0000FFFFU :
            (RegType == X86Reg::kRegGpd  ) ? 0x0000FFFFU :
            (RegType == X86Reg::kRegGpq  ) ? 0x0000FFFFU :
            (RegType == X86Reg::kRegXmm  ) ? 0xFFFFFFFFU :
            (RegType == X86Reg::kRegYmm  ) ? 0xFFFFFFFFU :
            (RegType == X86Reg::kRegZmm  ) ? 0xFFFFFFFFU :
            (RegType == X86Reg::kRegRip  ) ? 0x00000001U :
            (RegType == X86Reg::kRegSeg  ) ? 0x0000007EU : // [ES|CS|SS|DS|FS|GS]
            (RegType == X86Reg::kRegFp   ) ? 0x000000FFU :
            (RegType == X86Reg::kRegMm   ) ? 0x000000FFU :
            (RegType == X86Reg::kRegK    ) ? 0x000000FFU :
            (RegType == X86Reg::kRegBnd  ) ? 0x0000000FU :
            (RegType == X86Reg::kRegCr   ) ? 0x0000FFFFU :
            (RegType == X86Reg::kRegDr   ) ? 0x0000FFFFU : X86Inst::kOpNone
  };
};

struct X86ValidationData {
  //! Allowed registers by reg-type (X86::kReg...).
  uint32_t allowedRegMask[X86Reg::kRegMax + 1];
  uint32_t allowedMemBaseRegs;
  uint32_t allowedMemIndexRegs;
};

static const uint32_t _x86OpFlagFromRegType[X86Reg::kRegMax + 1] = {
  ASMJIT_TABLE_T_32(X86OpTypeFromRegTypeT, kValue, 0)
};

static const X86ValidationData _x86ValidationData = {
  { ASMJIT_TABLE_T_32(X86RegMaskFromRegTypeT, kMask, 0) },
  (1U << X86Reg::kRegGpw) | (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegRip) | (1U << Label::kLabelTag),
  (1U << X86Reg::kRegGpw) | (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegXmm) | (1U << X86Reg::kRegYmm) | (1U << X86Reg::kRegZmm)
};

static const X86ValidationData _x64ValidationData = {
  { ASMJIT_TABLE_T_32(X64RegMaskFromRegTypeT, kMask, 0) },
  (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegGpq) | (1U << X86Reg::kRegRip) | (1U << Label::kLabelTag),
  (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegGpq) | (1U << X86Reg::kRegXmm) | (1U << X86Reg::kRegYmm) | (1U << X86Reg::kRegZmm)
};

static ASMJIT_INLINE bool x86CheckOSig(const X86Inst::OSignature& op, const X86Inst::OSignature& ref, bool& immOutOfRange) noexcept {
  // Fail if operand types are incompatible.
  uint32_t opFlags = op.flags;
  if ((opFlags & ref.flags) == 0) {
    // Mark temporarily `immOutOfRange` so we can return a more descriptive error.
    if ((opFlags & X86Inst::kOpAllImm) && (ref.flags & X86Inst::kOpAllImm)) {
      immOutOfRange = true;
      return true;
    }

    return false;
  }

  // Fail if memory specific flags and sizes are incompatibles.
  uint32_t opMemFlags = op.memFlags;
  if (opMemFlags != 0) {
    uint32_t refMemFlags = ref.memFlags;
    if ((refMemFlags & opMemFlags) == 0)
      return false;

    if ((refMemFlags & X86Inst::kMemOpBaseOnly) && !(opMemFlags && X86Inst::kMemOpBaseOnly))
      return false;
  }

  // Specific register index.
  if (opFlags & X86Inst::kOpAllRegs) {
    uint32_t refRegMask = ref.regMask;
    if (refRegMask && !(op.regMask & refRegMask))
      return false;
  }

  return true;
}

ASMJIT_FAVOR_SIZE Error X86InstImpl::validate(uint32_t archType, const Inst::Detail& detail, const Operand_* operands, uint32_t count) noexcept {
  uint32_t i;
  uint32_t archMask;
  const X86ValidationData* vd;

  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  if (archType == ArchInfo::kTypeX86) {
    vd = &_x86ValidationData;
    archMask = X86Inst::kArchMaskX86;
  }
  else {
    vd = &_x64ValidationData;
    archMask = X86Inst::kArchMaskX64;
  }

  // Get the instruction data.
  uint32_t instId = detail.instId;
  uint32_t options = detail.options;

  if (ASMJIT_UNLIKELY(instId >= X86Inst::_kIdCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  const X86Inst* iData = &X86InstDB::instData[instId];
  uint32_t iFlags = iData->getFlags();

  // Validate LOCK, XACQUIRE, and XRELEASE prefixes.
  const uint32_t kLockXAcqRel = X86Inst::kOptionXAcquire | X86Inst::kOptionXRelease;
  if (options & (X86Inst::kOptionLock | kLockXAcqRel)) {
    if (options & X86Inst::kOptionLock) {
      if (ASMJIT_UNLIKELY(!(iFlags & X86Inst::kFlagLock) && !(options & kLockXAcqRel)))
        return DebugUtils::errored(kErrorInvalidLockPrefix);

      if (ASMJIT_UNLIKELY(count < 1 || !operands[0].isMem()))
        return DebugUtils::errored(kErrorInvalidLockPrefix);
    }

    if (options & kLockXAcqRel) {
      if (ASMJIT_UNLIKELY(!(options & X86Inst::kOptionLock) || (options & kLockXAcqRel) == kLockXAcqRel))
        return DebugUtils::errored(kErrorInvalidPrefixCombination);

      if (ASMJIT_UNLIKELY((options & X86Inst::kOptionXAcquire) && !(iFlags & X86Inst::kFlagXAcquire)))
        return DebugUtils::errored(kErrorInvalidXAcquirePrefix);

      if (ASMJIT_UNLIKELY((options & X86Inst::kOptionXRelease) && !(iFlags & X86Inst::kFlagXRelease)))
        return DebugUtils::errored(kErrorInvalidXReleasePrefix);
    }
  }

  // Validate REP and REPNZ prefixes.
  const uint32_t kRepRepRepnz = X86Inst::kOptionRep | X86Inst::kOptionRepnz;
  if (options & kRepRepRepnz) {
    if (ASMJIT_UNLIKELY((options & kRepRepRepnz) == kRepRepRepnz))
      return DebugUtils::errored(kErrorInvalidPrefixCombination);

    if (ASMJIT_UNLIKELY((options & X86Inst::kOptionRep) && !(iFlags & X86Inst::kFlagRep)))
      return DebugUtils::errored(kErrorInvalidRepPrefix);

    if (ASMJIT_UNLIKELY((options & X86Inst::kOptionRepnz) && !(iFlags & X86Inst::kFlagRepnz)))
      return DebugUtils::errored(kErrorInvalidRepPrefix);

    // TODO: Validate extraReg {cx|ecx|rcx}.
  }

  // Translate the given operands to `X86Inst::OSignature`.
  X86Inst::OSignature oSigTranslated[6];
  uint32_t combinedOpFlags = 0;
  uint32_t combinedRegMask = 0;

  const X86Mem* memOp = nullptr;

  for (i = 0; i < count; i++) {
    const Operand_& op = operands[i];
    if (op.getOp() == Operand::kOpNone) break;

    uint32_t opFlags = 0;
    uint32_t memFlags = 0;
    uint32_t regMask = 0;

    switch (op.getOp()) {
      case Operand::kOpReg: {
        uint32_t regType = op.as<Reg>().getType();
        if (ASMJIT_UNLIKELY(regType >= X86Reg::kRegCount))
          return DebugUtils::errored(kErrorInvalidRegType);

        opFlags = _x86OpFlagFromRegType[regType];
        if (ASMJIT_UNLIKELY(opFlags == 0))
          return DebugUtils::errored(kErrorInvalidRegType);

        // If `regId` is equal or greater than Operand::kPackedIdMin it means
        // that the register is virtual and its index will be assigned later
        // by the register allocator. We must pass unless asked to disallow
        // virtual registers.
        // TODO: We need an option to refuse virtual regs here.
        uint32_t regId = op.getId();
        if (regId < Operand::kPackedIdMin) {
          if (ASMJIT_UNLIKELY(regId >= 32))
            return DebugUtils::errored(kErrorInvalidPhysId);

          regMask = Utils::mask(regId);
          if (ASMJIT_UNLIKELY((vd->allowedRegMask[regType] & regMask) == 0))
            return DebugUtils::errored(kErrorInvalidPhysId);

          combinedRegMask |= regMask;
        }
        else {
          regMask = 0xFFFFFFFFU;
        }
        break;
      }

      // TODO: Validate base and index and combine with `combinedRegMask`.
      case Operand::kOpMem: {
        const X86Mem& m = op.as<X86Mem>();

        uint32_t baseType = m.getBaseType();
        uint32_t indexType = m.getIndexType();

        memOp = &m;

        if (m.getSegmentId() > 6)
          return DebugUtils::errored(kErrorInvalidSegment);

        if (baseType) {
          uint32_t baseId = m.getBaseId();

          if (m.isRegHome()) {
            // Home address of virtual register. In such case we don't want to
            // validate the type of the base register as it will always be patched
            // to ESP|RSP.
          }
          else {
            if (ASMJIT_UNLIKELY((vd->allowedMemBaseRegs & (1U << baseType)) == 0))
              return DebugUtils::errored(kErrorInvalidAddress);
          }

          // Create information that will be validated only if this is an implicit
          // memory operand. Basically only usable for string instructions and other
          // instructions where memory operand is implicit and has 'seg:[reg]' form.
          if (baseId < Operand::kPackedIdMin) {
            // Physical base id.
            regMask = Utils::mask(baseId);
            combinedRegMask |= regMask;
          }
          else {
            // Virtual base id - will the whole mask for implicit mem validation.
            // The register is not assigned yet, so we cannot predict the phys id.
            regMask = 0xFFFFFFFFU;
          }

          if (!indexType && !m.getOffsetLo32())
            memFlags |= X86Inst::kMemOpBaseOnly;
        }
        else {
          // Base is an address, make sure that the address doesn't overflow 32-bit
          // integer (either int32_t or uint32_t) in 32-bit targets.
          int64_t offset = m.getOffset();
          if (archMask == X86Inst::kArchMaskX86 && !Utils::isInt32(offset) && !Utils::isUInt32(offset))
            return DebugUtils::errored(kErrorInvalidAddress);
        }

        if (indexType) {
          if (ASMJIT_UNLIKELY((vd->allowedMemIndexRegs & (1U << indexType)) == 0))
            return DebugUtils::errored(kErrorInvalidAddress);

          if (indexType == X86Reg::kRegXmm) {
            opFlags |= X86Inst::kOpVm;
            memFlags |= X86Inst::kMemOpVm32x | X86Inst::kMemOpVm64x;
          }
          else if (indexType == X86Reg::kRegYmm) {
            opFlags |= X86Inst::kOpVm;
            memFlags |= X86Inst::kMemOpVm32y | X86Inst::kMemOpVm64y;
          }
          else if (indexType == X86Reg::kRegZmm) {
            opFlags |= X86Inst::kOpVm;
            memFlags |= X86Inst::kMemOpVm32z | X86Inst::kMemOpVm64z;
          }
          else {
            opFlags |= X86Inst::kOpMem;
            if (baseType)
              memFlags |= X86Inst::kMemOpMib;
          }

          // [RIP + {XMM|YMM|ZMM}] is not allowed.
          if (baseType == X86Reg::kRegRip && (opFlags & X86Inst::kOpVm))
            return DebugUtils::errored(kErrorInvalidAddress);

          uint32_t indexId = m.getIndexId();
          if (indexId < Operand::kPackedIdMin)
            combinedRegMask |= Utils::mask(indexId);

          // Only used for implicit memory operands having 'seg:[reg]' form, so clear it.
          regMask = 0;
        }
        else {
          opFlags |= X86Inst::kOpMem;
        }

        switch (m.getSize()) {
          case  0: memFlags |= X86Inst::kMemOpAny ; break;
          case  1: memFlags |= X86Inst::kMemOpM8  ; break;
          case  2: memFlags |= X86Inst::kMemOpM16 ; break;
          case  4: memFlags |= X86Inst::kMemOpM32 ; break;
          case  6: memFlags |= X86Inst::kMemOpM48 ; break;
          case  8: memFlags |= X86Inst::kMemOpM64 ; break;
          case 10: memFlags |= X86Inst::kMemOpM80 ; break;
          case 16: memFlags |= X86Inst::kMemOpM128; break;
          case 32: memFlags |= X86Inst::kMemOpM256; break;
          case 64: memFlags |= X86Inst::kMemOpM512; break;
          default:
            return DebugUtils::errored(kErrorInvalidOperandSize);
        }

        break;
      }

      case Operand::kOpImm: {
        uint64_t immValue = op.as<Imm>().getUInt64();
        uint32_t immFlags = 0;

        if (static_cast<int64_t>(immValue) >= 0) {
          const uint32_t k32AndMore = X86Inst::kOpI32 | X86Inst::kOpU32 |
                                      X86Inst::kOpI64 | X86Inst::kOpU64 ;

          if (immValue <= 0xFU)
            immFlags = X86Inst::kOpU4 | X86Inst::kOpI8 | X86Inst::kOpU8 | X86Inst::kOpI16 | X86Inst::kOpU16 | k32AndMore;
          else if (immValue <= 0x7FU)
            immFlags = X86Inst::kOpI8 | X86Inst::kOpU8 | X86Inst::kOpI16 | X86Inst::kOpU16 | k32AndMore;
          else if (immValue <= 0xFFU)
            immFlags = X86Inst::kOpU8 | X86Inst::kOpI16 | X86Inst::kOpU16 | k32AndMore;
          else if (immValue <= 0x7FFFU)
            immFlags = X86Inst::kOpI16 | X86Inst::kOpU16 | k32AndMore;
          else if (immValue <= 0xFFFFU)
            immFlags = X86Inst::kOpU16 | k32AndMore;
          else if (immValue <= 0x7FFFFFFFU)
            immFlags = k32AndMore;
          else if (immValue <= 0xFFFFFFFFU)
            immFlags = X86Inst::kOpU32 | X86Inst::kOpI64 | X86Inst::kOpU64;
          else if (immValue <= ASMJIT_UINT64_C(0x7FFFFFFFFFFFFFFF))
            immFlags = X86Inst::kOpI64 | X86Inst::kOpU64;
          else
            immFlags = X86Inst::kOpU64;
        }
        else {
          // 2s complement negation, as our number is unsigned...
          immValue = (~immValue + 1);

          if (immValue <= 0x80U)
            immFlags = X86Inst::kOpI8 | X86Inst::kOpI16 | X86Inst::kOpI32 | X86Inst::kOpI64;
          else if (immValue <= 0x8000U)
            immFlags = X86Inst::kOpI16 | X86Inst::kOpI32 | X86Inst::kOpI64;
          else if (immValue <= 0x80000000U)
            immFlags = X86Inst::kOpI32 | X86Inst::kOpI64;
          else
            immFlags = X86Inst::kOpI64;
        }
        opFlags |= immFlags;
        break;
      }

      case Operand::kOpLabel: {
        opFlags |= X86Inst::kOpRel8 | X86Inst::kOpRel32;
        break;
      }

      default:
        return DebugUtils::errored(kErrorInvalidState);
    }

    X86Inst::OSignature& tod = oSigTranslated[i];
    tod.flags = opFlags;
    tod.memFlags = static_cast<uint16_t>(memFlags);
    tod.regMask = static_cast<uint8_t>(regMask & 0xFFU);
    combinedOpFlags |= opFlags;
  }

  // Decrease the number of operands of those that are none. This is important
  // as Assembler and CodeCompiler may just pass more operands where some of
  // them are none (it means that no operand is given at that index). However,
  // validate that there are no gaps (like [reg, none, reg] or [none, reg]).
  if (i < count) {
    while (--count > i)
      if (ASMJIT_UNLIKELY(!operands[count].isNone()))
        return DebugUtils::errored(kErrorInvalidState);
  }

  // Validate X86 and X64 specific cases.
  if (archMask == X86Inst::kArchMaskX86) {
    // Illegal use of 64-bit register in 32-bit mode.
    if (ASMJIT_UNLIKELY((combinedOpFlags & X86Inst::kOpGpq) != 0))
      return DebugUtils::errored(kErrorInvalidUseOfGpq);
  }
  else {
    // Illegal use of a high 8-bit register with REX prefix.
    if (ASMJIT_UNLIKELY((combinedOpFlags & X86Inst::kOpGpbHi) != 0 && (combinedRegMask & 0xFFFFFF00U) != 0))
      return DebugUtils::errored(kErrorInvalidUseOfGpbHi);
  }

  // Validate instruction operands.
  const X86Inst::CommonData* commonData = &iData->getCommonData();
  const X86Inst::ISignature* iSig = X86InstDB::iSignatureData + commonData->_iSignatureIndex;
  const X86Inst::ISignature* iEnd = iSig                      + commonData->_iSignatureCount;

  if (iSig != iEnd) {
    const X86Inst::OSignature* oSigData = X86InstDB::oSignatureData;

    // If set it means that we matched a signature where only immediate value
    // was out of bounds. We can return a more descriptive error if we know this.
    bool globalImmOutOfRange = false;

    do {
      // Check if the architecture is compatible.
      if ((iSig->archMask & archMask) == 0) continue;

      // Compare the operands table with reference operands.
      uint32_t j = 0;
      uint32_t iSigCount = iSig->opCount;
      bool localImmOutOfRange = false;

      if (iSigCount == count) {
        for (j = 0; j < count; j++)
          if (!x86CheckOSig(oSigTranslated[j], oSigData[iSig->operands[j]], localImmOutOfRange))
            break;
      }
      else if (iSigCount - iSig->implicit == count) {
        uint32_t r = 0;
        for (j = 0; j < count && r < iSigCount; j++, r++) {
          const X86Inst::OSignature* oChk = oSigTranslated + j;
          const X86Inst::OSignature* oRef;
Next:
          oRef = oSigData + iSig->operands[r];
          // Skip implicit.
          if ((oRef->flags & X86Inst::kOpImplicit) != 0) {
            if (++r >= iSigCount)
              break;
            else
              goto Next;
          }

          if (!x86CheckOSig(*oChk, *oRef, localImmOutOfRange))
            break;
        }
      }

      if (j == count) {
        if (!localImmOutOfRange) {
          // Match, must clear possible `globalImmOutOfRange`.
          globalImmOutOfRange = false;
          break;
        }
        globalImmOutOfRange = localImmOutOfRange;
      }
    } while (++iSig != iEnd);

    if (iSig == iEnd) {
      if (globalImmOutOfRange)
        return DebugUtils::errored(kErrorInvalidImmediate);
      else
        return DebugUtils::errored(kErrorInvalidInstruction);
    }
  }

  // Validate AVX-512 options:
  const RegOnly& extraReg = detail.extraReg;
  const uint32_t kAvx512Options = X86Inst::kOptionZMask   |
                                  X86Inst::kOption1ToX    |
                                  X86Inst::kOptionER      |
                                  X86Inst::kOptionSAE     ;

  if (!extraReg.isNone() || (options & kAvx512Options)) {
    if (commonData->hasFlag(X86Inst::kFlagEvex)) {
      // Validate AVX-512 {k} and {k}{z}.
      if (!extraReg.isNone()) {
        // Mask can only be specified by a 'k' register.
        if (ASMJIT_UNLIKELY(extraReg.getType() != X86Reg::kRegK))
          return DebugUtils::errored(kErrorInvalidKMaskReg);

        if (ASMJIT_UNLIKELY(!commonData->hasAvx512K()))
          return DebugUtils::errored(kErrorInvalidKMaskUse);
      }

      if ((options & X86Inst::kOptionZMask)) {
        if (ASMJIT_UNLIKELY((options & X86Inst::kOptionZMask) != 0 && !commonData->hasAvx512Z()))
          return DebugUtils::errored(kErrorInvalidKZeroUse);
      }

      // Validate AVX-512 broadcast {1tox}.
      if (options & X86Inst::kOption1ToX) {
        if (ASMJIT_UNLIKELY(!memOp))
          return DebugUtils::errored(kErrorInvalidBroadcast);

        uint32_t size = memOp->getSize();
        if (size != 0) {
          // The the size is specified it has to match the broadcast size.
          if (ASMJIT_UNLIKELY(commonData->hasAvx512B32() && size != 4))
            return DebugUtils::errored(kErrorInvalidBroadcast);

          if (ASMJIT_UNLIKELY(commonData->hasAvx512B64() && size != 8))
            return DebugUtils::errored(kErrorInvalidBroadcast);
        }
      }

      // Validate AVX-512 {sae} and {er}.
      if (options & (X86Inst::kOptionSAE | X86Inst::kOptionER)) {
        // Rounding control is impossible if the instruction is not reg-to-reg.
        if (ASMJIT_UNLIKELY(memOp))
          return DebugUtils::errored(kErrorInvalidEROrSAE);

        // Check if {sae} or {er} is supported by the instruction.
        if (options & X86Inst::kOptionER) {
          // NOTE: if both {sae} and {er} are set, we don't care, as {sae} is implied.
          if (ASMJIT_UNLIKELY(!commonData->hasAvx512ER()))
            return DebugUtils::errored(kErrorInvalidEROrSAE);

          // {er} is defined for scalar ops or vector ops using zmm (LL = 10). We
          // don't need any more bits in the instruction database to be able to
          // validate this, as each AVX512 instruction that has broadcast is vector
          // instruction (in this case we require zmm registers), otherwise it's a
          // scalar instruction, which is valid.
          if (commonData->hasAvx512B()) {
            // Supports broadcast, thus we require LL to be '10', which means there
            // have to be zmm registers used. We don't calculate LL here, but we know
            // that it would be '10' if there is at least one ZMM register used.

            // There is no 'ER' enabled instruction with less than two operands.
            ASMJIT_ASSERT(count >= 2);
            if (ASMJIT_UNLIKELY(!X86Reg::isZmm(operands[0]) && !X86Reg::isZmm(operands[1])))
              return DebugUtils::errored(kErrorInvalidEROrSAE);
          }
        }
        else {
          // {sae} doesn't have the same limitations as {er}, this is enough.
          if (ASMJIT_UNLIKELY(!commonData->hasAvx512SAE()))
            return DebugUtils::errored(kErrorInvalidEROrSAE);
        }
      }
    }
    else {
      // Not AVX512 instruction - maybe OpExtra is xCX register used by REP/REPNZ prefix. Otherwise the instruction is invalid.
      if ((options & kAvx512Options) || (options & (X86Inst::kOptionRep | X86Inst::kOptionRepnz)) == 0)
        return DebugUtils::errored(kErrorInvalidInstruction);
    }
  }

  return kErrorOk;
}
#endif

// ============================================================================
// [asmjit::X86InstImpl - CheckFeatures]
// ============================================================================

#if !defined(ASMJIT_DISABLE_EXTENSIONS)
ASMJIT_FAVOR_SIZE static uint32_t x86GetRegTypesMask(const Operand_* operands, uint32_t count) noexcept {
  uint32_t mask = 0;
  for (uint32_t i = 0; i < count; i++) {
    const Operand_& op = operands[i];
    if (op.isReg()) {
      const Reg& reg = op.as<Reg>();
      mask |= Utils::mask(reg.getType());
    }
    else if (op.isMem()) {
      const Mem& mem = op.as<Mem>();
      if (mem.hasBaseReg()) mask |= Utils::mask(mem.getBaseType());
      if (mem.hasIndexReg()) mask |= Utils::mask(mem.getIndexType());
    }
  }
  return mask;
}

ASMJIT_FAVOR_SIZE Error X86InstImpl::checkFeatures(uint32_t archType, const Inst::Detail& detail, const Operand_* operands, uint32_t count, CpuFeatures& out) noexcept {
  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  // Get the instruction data.
  uint32_t instId = detail.instId;
  if (ASMJIT_UNLIKELY(instId >= X86Inst::_kIdCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  const X86Inst* iData = &X86InstDB::instData[instId];
  const X86Inst::OperationData& od = iData->getOperationData();

  const uint8_t* fData = od.getFeaturesData();
  const uint8_t* fEnd = od.getFeaturesEnd();

  // Copy all features to `out`.
  out.reset();
  do {
    uint32_t feature = fData[0];
    if (!feature)
      break;
    out.add(feature);
  } while (++fData != fEnd);

  // Since AsmJit merges all instructions that share the same name we have to
  // deal with some special cases and also with MMX/SSE and AVX/AVX2 overlaps.

  // Only proceed if there were some CPU flags set.
  if (fData != od.getFeaturesData()) {
    uint32_t mask = x86GetRegTypesMask(operands, count);

    // Check for MMX vs SSE overlap.
    if (out.has(CpuInfo::kX86FeatureMMX) || out.has(CpuInfo::kX86FeatureMMX2)) {
      // Only instructions defined by SSE and SSE2 overlap. Instructions introduced
      // by newer instruction sets like SSE3+ don't state MMX as they require SSE3+.
      if (out.has(CpuInfo::kX86FeatureSSE) || out.has(CpuInfo::kX86FeatureSSE2)) {
        if (!(mask & Utils::mask(X86Reg::kRegXmm))) {
          // The instruction doesn't use XMM register(s), thus it's MMX/MMX2 only.
          out.remove(CpuInfo::kX86FeatureSSE);
          out.remove(CpuInfo::kX86FeatureSSE2);
        }
        else {
          out.remove(CpuInfo::kX86FeatureMMX);
          out.remove(CpuInfo::kX86FeatureMMX2);
        }

        // Special case: PEXTRW instruction is MMX/SSE2 instruction. However, this
        // instruction couldn't access memory (only register to register extract) so
        // when SSE4.1 introduced the whole family of PEXTR/PINSR instructions they
        // also introduced PEXTRW with a new opcode 0x15 that can extract directly to
        // memory. This instruction is, of course, not compatible with MMX/SSE2 one.
        if (instId == X86Inst::kIdPextrw && count > 0 && !operands[0].isMem()) {
          out.remove(CpuInfo::kX86FeatureSSE4_1);
        }
      }
    }

    // Check for AVX vs AVX2 overlap.
    if (out.has(CpuInfo::kX86FeatureAVX) && out.has(CpuInfo::kX86FeatureAVX2)) {
      bool isAVX2 = true;
      // Special case: VBROADCASTSS and VBROADCASTSD were introduced in AVX, but
      // only version that uses memory as a source operand. AVX2 then added support
      // for register source operand.
      if (instId == X86Inst::kIdVbroadcastss || instId == X86Inst::kIdVbroadcastsd) {
        if (count > 1 && operands[0].isMem())
          isAVX2 = false;
      }
      else {
        // AVX instruction set doesn't support integer operations on YMM registers
        // as these were later introcuced by AVX2. In our case we have to check if
        // YMM register(s) are in use and if that is the case this is an AVX2 instruction.
        if (!(mask & Utils::mask(X86Reg::kRegYmm, X86Reg::kRegZmm)))
          isAVX2 = false;
      }

      if (isAVX2)
        out.remove(CpuInfo::kX86FeatureAVX);
      else
        out.remove(CpuInfo::kX86FeatureAVX2);
    }

    // Check for AVX|AVX2|FMA|F16C vs AVX512 overlap.
    if (out.has(CpuInfo::kX86FeatureAVX) || out.has(CpuInfo::kX86FeatureAVX2) || out.has(CpuInfo::kX86FeatureFMA) || out.has(CpuInfo::kX86FeatureF16C)) {
      // Only AVX512-F|BW|DQ allow to encode AVX/AVX2 instructions
      if (out.has(CpuInfo::kX86FeatureAVX512_F) || out.has(CpuInfo::kX86FeatureAVX512_BW) || out.has(CpuInfo::kX86FeatureAVX512_DQ)) {
        uint32_t options = detail.options;
        uint32_t kAvx512Options = X86Inst::kOptionEvex | X86Inst::_kOptionAvx512Mask;

        if (!(mask & Utils::mask(X86Reg::kRegZmm, X86Reg::kRegK)) && !(options & (kAvx512Options)) && detail.extraReg.getType() != X86Reg::kRegK) {
          out.remove(CpuInfo::kX86FeatureAVX512_F)
             .remove(CpuInfo::kX86FeatureAVX512_BW)
             .remove(CpuInfo::kX86FeatureAVX512_DQ)
             .remove(CpuInfo::kX86FeatureAVX512_VL);
        }
      }
    }

    // Remove or keep AVX512_VL feature.
    if (out.has(CpuInfo::kX86FeatureAVX512_VL)) {
      if (!(mask & Utils::mask(X86Reg::kRegZmm)))
        out.remove(CpuInfo::kX86FeatureAVX512_VL);
    }
  }

  return kErrorOk;
}
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
