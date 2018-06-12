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
#include "../base/cpuinfo.h"
#include "../base/utils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86internal_p.h"
#include "../x86/x86regalloc_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

enum { kCompilerDefaultLookAhead = 64 };

static Error X86RAPass_translateOperands(X86RAPass* self, Operand_* opArray, uint32_t opCount);

// ============================================================================
// [asmjit::X86RAPass - SpecialInst]
// ============================================================================

struct X86SpecialInst {
  uint8_t inReg;
  uint8_t outReg;
  uint16_t flags;
};

static ASMJIT_INLINE const X86SpecialInst* X86SpecialInst_get(uint32_t instId, const Operand* opArray, uint32_t opCount) noexcept {
  enum { kAny = Globals::kInvalidRegId };

#define R(ri) { uint8_t(ri)  , uint8_t(kAny), uint16_t(TiedReg::kRReg) }
#define W(ri) { uint8_t(kAny), uint8_t(ri)  , uint16_t(TiedReg::kWReg) }
#define X(ri) { uint8_t(ri)  , uint8_t(ri)  , uint16_t(TiedReg::kXReg) }
#define NONE() { uint8_t(kAny), uint8_t(kAny), 0 }
  static const X86SpecialInst instCpuid[]        = { X(X86Gp::kIdAx), W(X86Gp::kIdBx), X(X86Gp::kIdCx), W(X86Gp::kIdDx) };
  static const X86SpecialInst instCbwCdqeCwde[]  = { X(X86Gp::kIdAx) };
  static const X86SpecialInst instCdqCwdCqo[]    = { W(X86Gp::kIdDx), R(X86Gp::kIdAx) };
  static const X86SpecialInst instCmpxchg[]      = { X(kAny), R(kAny), X(X86Gp::kIdAx) };
  static const X86SpecialInst instCmpxchg8b16b[] = { NONE(), X(X86Gp::kIdDx), X(X86Gp::kIdAx), R(X86Gp::kIdCx), R(X86Gp::kIdBx) };
  static const X86SpecialInst instDaaDas[]       = { X(X86Gp::kIdAx) };
  static const X86SpecialInst instDiv2[]         = { X(X86Gp::kIdAx), R(kAny) };
  static const X86SpecialInst instDiv3[]         = { X(X86Gp::kIdDx), X(X86Gp::kIdAx), R(kAny) };
  static const X86SpecialInst instJecxz[]        = { R(X86Gp::kIdCx) };
  static const X86SpecialInst instMul2[]         = { X(X86Gp::kIdAx), R(kAny) };
  static const X86SpecialInst instMul3[]         = { W(X86Gp::kIdDx), X(X86Gp::kIdAx), R(kAny) };
  static const X86SpecialInst instMulx[]         = { W(kAny), W(kAny), R(kAny), R(X86Gp::kIdDx) };
  static const X86SpecialInst instLahf[]         = { W(X86Gp::kIdAx) };
  static const X86SpecialInst instSahf[]         = { R(X86Gp::kIdAx) };
  static const X86SpecialInst instMaskmovq[]     = { R(kAny), R(kAny), R(X86Gp::kIdDi) };
  static const X86SpecialInst instRdtscRdtscp[]  = { W(X86Gp::kIdDx), W(X86Gp::kIdAx), W(X86Gp::kIdCx) };
  static const X86SpecialInst instRot[]          = { X(kAny), R(X86Gp::kIdCx) };
  static const X86SpecialInst instShldShrd[]     = { X(kAny), R(kAny), R(X86Gp::kIdCx) };
  static const X86SpecialInst instThirdXMM0[]    = { W(kAny), R(kAny), R(0) };
  static const X86SpecialInst instPcmpestri[]    = { R(kAny), R(kAny), NONE(), W(X86Gp::kIdCx) };
  static const X86SpecialInst instPcmpestrm[]    = { R(kAny), R(kAny), NONE(), W(0) };
  static const X86SpecialInst instPcmpistri[]    = { R(kAny), R(kAny), NONE(), W(X86Gp::kIdCx), R(X86Gp::kIdAx), R(X86Gp::kIdDx) };
  static const X86SpecialInst instPcmpistrm[]    = { R(kAny), R(kAny), NONE(), W(0)           , R(X86Gp::kIdAx), R(X86Gp::kIdDx) };
  static const X86SpecialInst instXsaveXrstor[]  = { W(kAny), R(X86Gp::kIdDx), R(X86Gp::kIdAx) };
  static const X86SpecialInst instReadMR[]       = { W(X86Gp::kIdDx), W(X86Gp::kIdAx), R(X86Gp::kIdCx) };
  static const X86SpecialInst instWriteMR[]      = { R(X86Gp::kIdDx), R(X86Gp::kIdAx), R(X86Gp::kIdCx) };

  static const X86SpecialInst instCmps[]         = { X(X86Gp::kIdSi), X(X86Gp::kIdDi) };
  static const X86SpecialInst instLods[]         = { W(X86Gp::kIdAx), X(X86Gp::kIdSi) };
  static const X86SpecialInst instMovs[]         = { X(X86Gp::kIdDi), X(X86Gp::kIdSi) };
  static const X86SpecialInst instScas[]         = { X(X86Gp::kIdDi), R(X86Gp::kIdAx) };
  static const X86SpecialInst instStos[]         = { X(X86Gp::kIdDi), R(X86Gp::kIdAx) };
#undef NONE
#undef X
#undef W
#undef R

  switch (instId) {
    case X86Inst::kIdCpuid      : return instCpuid;
    case X86Inst::kIdCbw        :
    case X86Inst::kIdCdqe       :
    case X86Inst::kIdCwde       : return instCbwCdqeCwde;
    case X86Inst::kIdCdq        :
    case X86Inst::kIdCwd        :
    case X86Inst::kIdCqo        : return instCdqCwdCqo;
    case X86Inst::kIdCmps       : return instCmps;
    case X86Inst::kIdCmpxchg    : return instCmpxchg;
    case X86Inst::kIdCmpxchg8b  :
    case X86Inst::kIdCmpxchg16b : return instCmpxchg8b16b;
    case X86Inst::kIdDaa        :
    case X86Inst::kIdDas        : return instDaaDas;
    case X86Inst::kIdDiv        : return (opCount == 2) ? instDiv2 : instDiv3;
    case X86Inst::kIdIdiv       : return (opCount == 2) ? instDiv2 : instDiv3;
    case X86Inst::kIdImul       : if (opCount == 2) return nullptr;
                                  if (opCount == 3 && !(opArray[0].isReg() && opArray[1].isReg() && opArray[2].isRegOrMem())) return nullptr;
                                  ASMJIT_FALLTHROUGH;
    case X86Inst::kIdMul        : return (opCount == 2) ? instMul2 : instMul3;
    case X86Inst::kIdMulx       : return instMulx;
    case X86Inst::kIdJecxz      : return instJecxz;
    case X86Inst::kIdLods       : return instLods;
    case X86Inst::kIdMovs       : return instMovs;
    case X86Inst::kIdLahf       : return instLahf;
    case X86Inst::kIdSahf       : return instSahf;
    case X86Inst::kIdMaskmovq   :
    case X86Inst::kIdMaskmovdqu :
    case X86Inst::kIdVmaskmovdqu: return instMaskmovq;
    case X86Inst::kIdEnter      : return nullptr; // Not supported.
    case X86Inst::kIdLeave      : return nullptr; // Not supported.
    case X86Inst::kIdRet        : return nullptr; // Not supported.
    case X86Inst::kIdMonitor    : return nullptr; // TODO: [COMPILER] Monitor/MWait.
    case X86Inst::kIdMwait      : return nullptr; // TODO: [COMPILER] Monitor/MWait.
    case X86Inst::kIdPop        : return nullptr; // TODO: [COMPILER] Pop/Push.
    case X86Inst::kIdPush       : return nullptr; // TODO: [COMPILER] Pop/Push.
    case X86Inst::kIdPopa       : return nullptr; // Not supported.
    case X86Inst::kIdPopf       : return nullptr; // Not supported.
    case X86Inst::kIdPusha      : return nullptr; // Not supported.
    case X86Inst::kIdPushf      : return nullptr; // Not supported.
    case X86Inst::kIdRcl        :
    case X86Inst::kIdRcr        :
    case X86Inst::kIdRol        :
    case X86Inst::kIdRor        :
    case X86Inst::kIdSal        :
    case X86Inst::kIdSar        :
    case X86Inst::kIdShl        : // Rot instruction is special only if the last operand is a variable.
    case X86Inst::kIdShr        : if (!opArray[1].isReg()) return nullptr;
                                  return instRot;
    case X86Inst::kIdShld       : // Shld/Shrd instruction is special only if the last operand is a variable.
    case X86Inst::kIdShrd       : if (!opArray[2].isReg()) return nullptr;
                                  return instShldShrd;
    case X86Inst::kIdRdtsc      :
    case X86Inst::kIdRdtscp     : return instRdtscRdtscp;
    case X86Inst::kIdScas       : return instScas;
    case X86Inst::kIdStos       : return instStos;
    case X86Inst::kIdBlendvpd   :
    case X86Inst::kIdBlendvps   :
    case X86Inst::kIdPblendvb   :
    case X86Inst::kIdSha256rnds2: return instThirdXMM0;
    case X86Inst::kIdPcmpestri  :
    case X86Inst::kIdVpcmpestri : return instPcmpestri;
    case X86Inst::kIdPcmpistri  :
    case X86Inst::kIdVpcmpistri : return instPcmpistri;
    case X86Inst::kIdPcmpestrm  :
    case X86Inst::kIdVpcmpestrm : return instPcmpestrm;
    case X86Inst::kIdPcmpistrm  :
    case X86Inst::kIdVpcmpistrm : return instPcmpistrm;
    case X86Inst::kIdXrstor     :
    case X86Inst::kIdXrstor64   :
    case X86Inst::kIdXsave      :
    case X86Inst::kIdXsave64    :
    case X86Inst::kIdXsaveopt   :
    case X86Inst::kIdXsaveopt64 : return instXsaveXrstor;
    case X86Inst::kIdRdmsr      :
    case X86Inst::kIdRdpmc      :
    case X86Inst::kIdXgetbv     : return instReadMR;
    case X86Inst::kIdWrmsr      :
    case X86Inst::kIdXsetbv     : return instWriteMR;
    default                     : return nullptr;
  }
}

// ============================================================================
// [asmjit::X86RAPass - Construction / Destruction]
// ============================================================================

X86RAPass::X86RAPass() noexcept : RAPass() {
  _state = &_x86State;
  _varMapToVaListOffset = ASMJIT_OFFSET_OF(X86RAData, tiedArray);
}
X86RAPass::~X86RAPass() noexcept {}

// ============================================================================
// [asmjit::X86RAPass - Interface]
// ============================================================================

Error X86RAPass::process(Zone* zone) noexcept {
  return Base::process(zone);
}

Error X86RAPass::prepare(CCFunc* func) noexcept {
  ASMJIT_PROPAGATE(Base::prepare(func));

  uint32_t archType = cc()->getArchType();
  _regCount._gp  = archType == ArchInfo::kTypeX86 ? 8 : 16;
  _regCount._mm  = 8;
  _regCount._k   = 8;
  _regCount._vec = archType == ArchInfo::kTypeX86 ? 8 : 16;
  _zsp = cc()->zsp();
  _zbp = cc()->zbp();

  _gaRegs[X86Reg::kKindGp ] = Utils::bits(_regCount.getGp()) & ~Utils::mask(X86Gp::kIdSp);
  _gaRegs[X86Reg::kKindMm ] = Utils::bits(_regCount.getMm());
  _gaRegs[X86Reg::kKindK  ] = Utils::bits(_regCount.getK());
  _gaRegs[X86Reg::kKindVec] = Utils::bits(_regCount.getVec());

  _x86State.reset(0);
  _clobberedRegs.reset();

  _avxEnabled = false;

  _varBaseRegId = Globals::kInvalidRegId; // Used by patcher.
  _varBaseOffset = 0;                     // Used by patcher.

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - Emit]
// ============================================================================

Error X86RAPass::emitMove(VirtReg* vReg, uint32_t dstId, uint32_t srcId, const char* reason) {
  const char* comment = nullptr;
  if (_emitComments) {
    _stringBuilder.setFormat("[%s] %s", reason, vReg->getName());
    comment = _stringBuilder.getData();
  }

  X86Reg dst(X86Reg::fromSignature(vReg->getSignature(), dstId));
  X86Reg src(X86Reg::fromSignature(vReg->getSignature(), srcId));
  return X86Internal::emitRegMove(reinterpret_cast<X86Emitter*>(cc()), dst, src, vReg->getTypeId(), _avxEnabled, comment);
}

Error X86RAPass::emitLoad(VirtReg* vReg, uint32_t id, const char* reason) {
  const char* comment = nullptr;
  if (_emitComments) {
    _stringBuilder.setFormat("[%s] %s", reason, vReg->getName());
    comment = _stringBuilder.getData();
  }

  X86Reg dst(X86Reg::fromSignature(vReg->getSignature(), id));
  X86Mem src(getVarMem(vReg));
  return X86Internal::emitRegMove(reinterpret_cast<X86Emitter*>(cc()), dst, src, vReg->getTypeId(), _avxEnabled, comment);
}

Error X86RAPass::emitSave(VirtReg* vReg, uint32_t id, const char* reason) {
  const char* comment = nullptr;
  if (_emitComments) {
    _stringBuilder.setFormat("[%s] %s", reason, vReg->getName());
    comment = _stringBuilder.getData();
  }

  X86Mem dst(getVarMem(vReg));
  X86Reg src(X86Reg::fromSignature(vReg->getSignature(), id));
  return X86Internal::emitRegMove(reinterpret_cast<X86Emitter*>(cc()), dst, src, vReg->getTypeId(), _avxEnabled, comment);
}

Error X86RAPass::emitSwapGp(VirtReg* dstReg, VirtReg* srcReg, uint32_t dstPhysId, uint32_t srcPhysId, const char* reason) noexcept {
  ASMJIT_ASSERT(dstPhysId != Globals::kInvalidRegId);
  ASMJIT_ASSERT(srcPhysId != Globals::kInvalidRegId);

  uint32_t is64 = std::max(dstReg->getTypeId(), srcReg->getTypeId()) >= TypeId::kI64;
  uint32_t sign = is64 ? uint32_t(X86RegTraits<X86Reg::kRegGpq>::kSignature)
                       : uint32_t(X86RegTraits<X86Reg::kRegGpd>::kSignature);

  X86Reg a = X86Reg::fromSignature(sign, dstPhysId);
  X86Reg b = X86Reg::fromSignature(sign, srcPhysId);

  ASMJIT_PROPAGATE(cc()->emit(X86Inst::kIdXchg, a, b));
  if (_emitComments)
    cc()->getCursor()->setInlineComment(cc()->_cbDataZone.sformat("[%s] %s, %s", reason, dstReg->getName(), srcReg->getName()));
  return kErrorOk;
}

Error X86RAPass::emitImmToReg(uint32_t dstTypeId, uint32_t dstPhysId, const Imm* src) noexcept {
  ASMJIT_ASSERT(dstPhysId != Globals::kInvalidRegId);

  X86Reg r0;
  Imm imm(*src);

  switch (dstTypeId) {
    case TypeId::kI8:
    case TypeId::kU8:
      imm.truncateTo8Bits();
      ASMJIT_FALLTHROUGH;

    case TypeId::kI16:
    case TypeId::kU16:
      imm.truncateTo16Bits();
      ASMJIT_FALLTHROUGH;

    case TypeId::kI32:
    case TypeId::kU32:
Mov32Truncate:
      imm.truncateTo32Bits();
      r0.setX86RegT<X86Reg::kRegGpd>(dstPhysId);
      cc()->emit(X86Inst::kIdMov, r0, imm);
      break;

    case TypeId::kI64:
    case TypeId::kU64:
      // Move to GPD register will also clear the high DWORD of GPQ
      // register in 64-bit mode.
      if (imm.isUInt32())
        goto Mov32Truncate;

      r0.setX86RegT<X86Reg::kRegGpq>(dstPhysId);
      cc()->emit(X86Inst::kIdMov, r0, imm);
      break;

    case TypeId::kF32:
    case TypeId::kF64:
      // Compiler doesn't manage FPU stack.
      ASMJIT_NOT_REACHED();
      break;

    case TypeId::kMmx32:
    case TypeId::kMmx64:
      // TODO: [COMPILER] EmitMoveImmToReg.
      break;

    default:
      // TODO: [COMPILER] EmitMoveImmToReg.
      break;
  }

  return kErrorOk;
}

Error X86RAPass::emitImmToStack(uint32_t dstTypeId, const X86Mem* dst, const Imm* src) noexcept {
  X86Mem mem(*dst);
  Imm imm(*src);

  // One stack entry has the same size as the native register size. That means
  // that if we want to move a 32-bit integer on the stack in 64-bit mode, we
  // need to extend it to a 64-bit integer first. In 32-bit mode, pushing a
  // 64-bit on stack is done in two steps by pushing low and high parts
  // separately.
  uint32_t gpSize = cc()->getGpSize();

  switch (dstTypeId) {
    case TypeId::kI8:
    case TypeId::kU8:
      imm.truncateTo8Bits();
      ASMJIT_FALLTHROUGH;

    case TypeId::kI16:
    case TypeId::kU16:
      imm.truncateTo16Bits();
      ASMJIT_FALLTHROUGH;

    case TypeId::kI32:
    case TypeId::kU32:
    case TypeId::kF32:
      mem.setSize(4);
      imm.truncateTo32Bits();
      cc()->emit(X86Inst::kIdMov, mem, imm);
      break;

    case TypeId::kI64:
    case TypeId::kU64:
    case TypeId::kF64:
    case TypeId::kMmx32:
    case TypeId::kMmx64:
      if (gpSize == 4) {
        uint32_t hi = imm.getUInt32Hi();

        // Lo-Part.
        mem.setSize(4);
        imm.truncateTo32Bits();

        cc()->emit(X86Inst::kIdMov, mem, imm);
        mem.addOffsetLo32(gpSize);

        // Hi-Part.
        imm.setUInt32(hi);
        cc()->emit(X86Inst::kIdMov, mem, imm);
      }
      else {
        mem.setSize(8);
        cc()->emit(X86Inst::kIdMov, mem, imm);
      }
      break;

    default:
      return DebugUtils::errored(kErrorInvalidState);
  }

  return kErrorOk;
}

Error X86RAPass::emitRegToStack(uint32_t dstTypeId, const X86Mem* dst, uint32_t srcTypeId, uint32_t srcPhysId) noexcept {
  ASMJIT_ASSERT(srcPhysId != Globals::kInvalidRegId);

  X86Mem m0(*dst);
  X86Reg r0, r1;

  uint32_t gpSize = cc()->getGpSize();
  uint32_t instId = 0;

  switch (dstTypeId) {
    case TypeId::kI64:
    case TypeId::kU64:
      // Extend BYTE->QWORD (GP).
      if (TypeId::isGpb(srcTypeId)) {
        r1.setX86RegT<X86Reg::kRegGpbLo>(srcPhysId);

        instId = (dstTypeId == TypeId::kI64 && srcTypeId == TypeId::kI8) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend WORD->QWORD (GP).
      if (TypeId::isGpw(srcTypeId)) {
        r1.setX86RegT<X86Reg::kRegGpw>(srcPhysId);

        instId = (dstTypeId == TypeId::kI64 && srcTypeId == TypeId::kI16) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend DWORD->QWORD (GP).
      if (TypeId::isGpd(srcTypeId)) {
        r1.setX86RegT<X86Reg::kRegGpd>(srcPhysId);

        instId = X86Inst::kIdMovsxd;
        if (dstTypeId == TypeId::kI64 && srcTypeId == TypeId::kI32)
          goto _ExtendMovGpXQ;
        else
          goto _ZeroExtendGpDQ;
      }

      // Move QWORD (GP).
      if (TypeId::isGpq(srcTypeId)) goto MovGpQ;
      if (TypeId::isMmx(srcTypeId)) goto MovMmQ;
      if (TypeId::isVec(srcTypeId)) goto MovXmmQ;
      break;

    case TypeId::kI32:
    case TypeId::kU32:
    case TypeId::kI16:
    case TypeId::kU16:
      // DWORD <- WORD (Zero|Sign Extend).
      if (TypeId::isGpw(srcTypeId)) {
        bool isDstSigned = dstTypeId == TypeId::kI16 || dstTypeId == TypeId::kI32;
        bool isSrcSigned = srcTypeId == TypeId::kI8  || srcTypeId == TypeId::kI16;

        r1.setX86RegT<X86Reg::kRegGpw>(srcPhysId);
        instId = isDstSigned && isSrcSigned ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpD;
      }

      // DWORD <- BYTE (Zero|Sign Extend).
      if (TypeId::isGpb(srcTypeId)) {
        bool isDstSigned = dstTypeId == TypeId::kI16 || dstTypeId == TypeId::kI32;
        bool isSrcSigned = srcTypeId == TypeId::kI8  || srcTypeId == TypeId::kI16;

        r1.setX86RegT<X86Reg::kRegGpbLo>(srcPhysId);
        instId = isDstSigned && isSrcSigned ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpD;
      }
      ASMJIT_FALLTHROUGH;

    case TypeId::kI8:
    case TypeId::kU8:
      if (TypeId::isInt(srcTypeId)) goto MovGpD;
      if (TypeId::isMmx(srcTypeId)) goto MovMmD;
      if (TypeId::isVec(srcTypeId)) goto MovXmmD;
      break;

    case TypeId::kMmx32:
    case TypeId::kMmx64:
      // Extend BYTE->QWORD (GP).
      if (TypeId::isGpb(srcTypeId)) {
        r1.setX86RegT<X86Reg::kRegGpbLo>(srcPhysId);

        instId = X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend WORD->QWORD (GP).
      if (TypeId::isGpw(srcTypeId)) {
        r1.setX86RegT<X86Reg::kRegGpw>(srcPhysId);

        instId = X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      if (TypeId::isGpd(srcTypeId)) goto _ExtendMovGpDQ;
      if (TypeId::isGpq(srcTypeId)) goto MovGpQ;
      if (TypeId::isMmx(srcTypeId)) goto MovMmQ;
      if (TypeId::isVec(srcTypeId)) goto MovXmmQ;
      break;

    case TypeId::kF32:
    case TypeId::kF32x1:
      if (TypeId::isVec(srcTypeId)) goto MovXmmD;
      break;

    case TypeId::kF64:
    case TypeId::kF64x1:
      if (TypeId::isVec(srcTypeId)) goto MovXmmQ;
      break;

    default:
      // TODO: Vector types by stack.
      break;
  }
  return DebugUtils::errored(kErrorInvalidState);

  // Extend+Move Gp.
_ExtendMovGpD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegGpd>(srcPhysId);

  cc()->emit(instId, r0, r1);
  cc()->emit(X86Inst::kIdMov, m0, r0);
  return kErrorOk;

_ExtendMovGpXQ:
  if (gpSize == 8) {
    m0.setSize(8);
    r0.setX86RegT<X86Reg::kRegGpq>(srcPhysId);

    cc()->emit(instId, r0, r1);
    cc()->emit(X86Inst::kIdMov, m0, r0);
  }
  else {
    m0.setSize(4);
    r0.setX86RegT<X86Reg::kRegGpd>(srcPhysId);

    cc()->emit(instId, r0, r1);

_ExtendMovGpDQ:
    cc()->emit(X86Inst::kIdMov, m0, r0);
    m0.addOffsetLo32(4);
    cc()->emit(X86Inst::kIdAnd, m0, 0);
  }
  return kErrorOk;

_ZeroExtendGpDQ:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegGpd>(srcPhysId);
  goto _ExtendMovGpDQ;

  // Move Gp.
MovGpD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegGpd>(srcPhysId);
  return cc()->emit(X86Inst::kIdMov, m0, r0);

MovGpQ:
  m0.setSize(8);
  r0.setX86RegT<X86Reg::kRegGpq>(srcPhysId);
  return cc()->emit(X86Inst::kIdMov, m0, r0);

  // Move Mm.
MovMmD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegMm>(srcPhysId);
  return cc()->emit(X86Inst::kIdMovd, m0, r0);

MovMmQ:
  m0.setSize(8);
  r0.setX86RegT<X86Reg::kRegMm>(srcPhysId);
  return cc()->emit(X86Inst::kIdMovq, m0, r0);

  // Move XMM.
MovXmmD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegXmm>(srcPhysId);
  return cc()->emit(X86Inst::kIdMovss, m0, r0);

MovXmmQ:
  m0.setSize(8);
  r0.setX86RegT<X86Reg::kRegXmm>(srcPhysId);
  return cc()->emit(X86Inst::kIdMovlps, m0, r0);
}

// ============================================================================
// [asmjit::X86RAPass - Register Management]
// ============================================================================

#if defined(ASMJIT_DEBUG)
template<int C>
static ASMJIT_INLINE void X86RAPass_checkStateVars(X86RAPass* self) {
  X86RAState* state = self->getState();
  VirtReg** sVars = state->getListByKind(C);

  uint32_t physId;
  uint32_t regMask;
  uint32_t regCount = self->_regCount.get(C);

  uint32_t occupied = state->_occupied.get(C);
  uint32_t modified = state->_modified.get(C);

  for (physId = 0, regMask = 1; physId < regCount; physId++, regMask <<= 1) {
    VirtReg* vreg = sVars[physId];

    if (!vreg) {
      ASMJIT_ASSERT((occupied & regMask) == 0);
      ASMJIT_ASSERT((modified & regMask) == 0);
    }
    else {
      ASMJIT_ASSERT((occupied & regMask) != 0);
      ASMJIT_ASSERT((modified & regMask) == (static_cast<uint32_t>(vreg->isModified()) << physId));

      ASMJIT_ASSERT(vreg->getKind() == C);
      ASMJIT_ASSERT(vreg->getState() == VirtReg::kStateReg);
      ASMJIT_ASSERT(vreg->getPhysId() == physId);
    }
  }
}

void X86RAPass::_checkState() {
  X86RAPass_checkStateVars<X86Reg::kKindGp >(this);
  X86RAPass_checkStateVars<X86Reg::kKindMm >(this);
  X86RAPass_checkStateVars<X86Reg::kKindVec>(this);
}
#else
void X86RAPass::_checkState() {}
#endif // ASMJIT_DEBUG

// ============================================================================
// [asmjit::X86RAPass - State - Load]
// ============================================================================

template<int C>
static ASMJIT_INLINE void X86RAPass_loadStateVars(X86RAPass* self, X86RAState* src) {
  X86RAState* cur = self->getState();

  VirtReg** cVars = cur->getListByKind(C);
  VirtReg** sVars = src->getListByKind(C);

  uint32_t physId;
  uint32_t modified = src->_modified.get(C);
  uint32_t regCount = self->_regCount.get(C);

  for (physId = 0; physId < regCount; physId++, modified >>= 1) {
    VirtReg* vreg = sVars[physId];
    cVars[physId] = vreg;
    if (!vreg) continue;

    vreg->setState(VirtReg::kStateReg);
    vreg->setPhysId(physId);
    vreg->setModified(modified & 0x1);
  }
}

void X86RAPass::loadState(RAState* src_) {
  X86RAState* cur = getState();
  X86RAState* src = static_cast<X86RAState*>(src_);

  VirtReg** vregs = _contextVd.getData();
  uint32_t count = static_cast<uint32_t>(_contextVd.getLength());

  // Load allocated variables.
  X86RAPass_loadStateVars<X86Reg::kKindGp >(this, src);
  X86RAPass_loadStateVars<X86Reg::kKindMm >(this, src);
  X86RAPass_loadStateVars<X86Reg::kKindVec>(this, src);

  // Load masks.
  cur->_occupied = src->_occupied;
  cur->_modified = src->_modified;

  // Load states of other variables and clear their 'Modified' flags.
  for (uint32_t i = 0; i < count; i++) {
    uint32_t vState = src->_cells[i].getState();

    if (vState == VirtReg::kStateReg)
      continue;

    vregs[i]->setState(vState);
    vregs[i]->setPhysId(Globals::kInvalidRegId);
    vregs[i]->setModified(false);
  }

  ASMJIT_X86_CHECK_STATE
}

// ============================================================================
// [asmjit::X86RAPass - State - Save]
// ============================================================================

RAState* X86RAPass::saveState() {
  VirtReg** vregs = _contextVd.getData();
  uint32_t count = static_cast<uint32_t>(_contextVd.getLength());

  size_t size = Utils::alignTo<size_t>(
    sizeof(X86RAState) + count * sizeof(X86StateCell), sizeof(void*));

  X86RAState* cur = getState();
  X86RAState* dst = _zone->allocT<X86RAState>(size);
  if (!dst) return nullptr;

  // Store links.
  ::memcpy(dst->_list, cur->_list, X86RAState::kAllCount * sizeof(VirtReg*));

  // Store masks.
  dst->_occupied = cur->_occupied;
  dst->_modified = cur->_modified;

  // Store cells.
  for (uint32_t i = 0; i < count; i++) {
    VirtReg* vreg = static_cast<VirtReg*>(vregs[i]);
    X86StateCell& cell = dst->_cells[i];

    cell.reset();
    cell.setState(vreg->getState());
  }

  return dst;
}

// ============================================================================
// [asmjit::X86RAPass - State - Switch]
// ============================================================================

template<int C>
static ASMJIT_INLINE void X86RAPass_switchStateVars(X86RAPass* self, X86RAState* src) {
  X86RAState* dst = self->getState();

  VirtReg** dVars = dst->getListByKind(C);
  VirtReg** sVars = src->getListByKind(C);

  X86StateCell* cells = src->_cells;
  uint32_t regCount = self->_regCount.get(C);
  bool didWork;

  do {
    didWork = false;

    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* dVReg = dVars[physId];
      VirtReg* sVd = sVars[physId];
      if (dVReg == sVd) continue;

      if (dVReg) {
        const X86StateCell& cell = cells[dVReg->_raId];

        if (cell.getState() != VirtReg::kStateReg) {
          if (cell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          dVReg = nullptr;
          didWork = true;
          if (!sVd) continue;
        }
      }

      if (!dVReg && sVd) {
_MoveOrLoad:
        if (sVd->getPhysId() != Globals::kInvalidRegId)
          self->move<C>(sVd, physId);
        else
          self->load<C>(sVd, physId);

        didWork = true;
        continue;
      }

      if (dVReg) {
        const X86StateCell& cell = cells[dVReg->_raId];
        if (!sVd) {
          if (cell.getState() == VirtReg::kStateReg)
            continue;

          if (cell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          didWork = true;
          continue;
        }
        else {
          if (cell.getState() == VirtReg::kStateReg) {
            if (dVReg->getPhysId() != Globals::kInvalidRegId && sVd->getPhysId() != Globals::kInvalidRegId) {
              if (C == X86Reg::kKindGp) {
                self->swapGp(dVReg, sVd);
              }
              else {
                self->spill<C>(dVReg);
                self->move<C>(sVd, physId);
              }

              didWork = true;
              continue;
            }
            else {
              didWork = true;
              continue;
            }
          }

          if (cell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);
          goto _MoveOrLoad;
        }
      }
    }
  } while (didWork);

  uint32_t dModified = dst->_modified.get(C);
  uint32_t sModified = src->_modified.get(C);

  if (dModified != sModified) {
    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* vreg = dVars[physId];
      if (!vreg) continue;

      if ((dModified & regMask) && !(sModified & regMask)) {
        self->save<C>(vreg);
        continue;
      }

      if (!(dModified & regMask) && (sModified & regMask)) {
        self->modify<C>(vreg);
        continue;
      }
    }
  }
}

void X86RAPass::switchState(RAState* src_) {
  ASMJIT_ASSERT(src_ != nullptr);

  X86RAState* cur = getState();
  X86RAState* src = static_cast<X86RAState*>(src_);

  // Ignore if both states are equal.
  if (cur == src)
    return;

  // Switch variables.
  X86RAPass_switchStateVars<X86Reg::kKindGp >(this, src);
  X86RAPass_switchStateVars<X86Reg::kKindMm >(this, src);
  X86RAPass_switchStateVars<X86Reg::kKindVec>(this, src);

  // Calculate changed state.
  VirtReg** vregs = _contextVd.getData();
  uint32_t count = static_cast<uint32_t>(_contextVd.getLength());

  X86StateCell* cells = src->_cells;
  for (uint32_t i = 0; i < count; i++) {
    VirtReg* vreg = static_cast<VirtReg*>(vregs[i]);
    const X86StateCell& cell = cells[i];
    uint32_t vState = cell.getState();

    if (vState != VirtReg::kStateReg) {
      vreg->setState(vState);
      vreg->setModified(false);
    }
  }

  ASMJIT_X86_CHECK_STATE
}

// ============================================================================
// [asmjit::X86RAPass - State - Intersect]
// ============================================================================

// The algorithm is actually not so smart, but tries to find an intersection od
// `a` and `b` and tries to move/alloc a variable into that location if it's
// possible. It also finds out which variables will be spilled/unused  by `a`
// and `b` and performs that action here. It may improve the switch state code
// in certain cases, but doesn't necessarily do the best job possible.
template<int C>
static ASMJIT_INLINE void X86RAPass_intersectStateVars(X86RAPass* self, X86RAState* a, X86RAState* b) {
  X86RAState* dst = self->getState();

  VirtReg** dVars = dst->getListByKind(C);
  VirtReg** aVars = a->getListByKind(C);

  X86StateCell* aCells = a->_cells;
  X86StateCell* bCells = b->_cells;

  uint32_t regCount = self->_regCount.get(C);
  bool didWork;

  // Similar to `switchStateVars()`, we iterate over and over until there is
  // no work to be done.
  do {
    didWork = false;

    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* dVReg = dVars[physId]; // Destination reg.
      VirtReg* aVReg = aVars[physId]; // State-a reg.

      if (dVReg == aVReg) continue;

      if (dVReg) {
        const X86StateCell& aCell = aCells[dVReg->_raId];
        const X86StateCell& bCell = bCells[dVReg->_raId];

        if (aCell.getState() != VirtReg::kStateReg && bCell.getState() != VirtReg::kStateReg) {
          if (aCell.getState() == VirtReg::kStateMem || bCell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          dVReg = nullptr;
          didWork = true;
          if (!aVReg) continue;
        }
      }

      if (!dVReg && aVReg) {
        if (aVReg->getPhysId() != Globals::kInvalidRegId)
          self->move<C>(aVReg, physId);
        else
          self->load<C>(aVReg, physId);

        didWork = true;
        continue;
      }

      if (dVReg) {
        const X86StateCell& aCell = aCells[dVReg->_raId];
        const X86StateCell& bCell = bCells[dVReg->_raId];

        if (!aVReg) {
          if (aCell.getState() == VirtReg::kStateReg || bCell.getState() == VirtReg::kStateReg)
            continue;

          if (aCell.getState() == VirtReg::kStateMem || bCell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          didWork = true;
          continue;
        }
        else if (C == X86Reg::kKindGp) {
          if (aCell.getState() == VirtReg::kStateReg) {
            if (dVReg->getPhysId() != Globals::kInvalidRegId && aVReg->getPhysId() != Globals::kInvalidRegId) {
              self->swapGp(dVReg, aVReg);

              didWork = true;
              continue;
            }
          }
        }
      }
    }
  } while (didWork);

  uint32_t dModified = dst->_modified.get(C);
  uint32_t aModified = a->_modified.get(C);

  if (dModified != aModified) {
    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* vreg = dVars[physId];
      if (!vreg) continue;

      const X86StateCell& aCell = aCells[vreg->_raId];
      if ((dModified & regMask) && !(aModified & regMask) && aCell.getState() == VirtReg::kStateReg)
        self->save<C>(vreg);
    }
  }
}

void X86RAPass::intersectStates(RAState* a_, RAState* b_) {
  X86RAState* a = static_cast<X86RAState*>(a_);
  X86RAState* b = static_cast<X86RAState*>(b_);

  ASMJIT_ASSERT(a != nullptr);
  ASMJIT_ASSERT(b != nullptr);

  X86RAPass_intersectStateVars<X86Reg::kKindGp >(this, a, b);
  X86RAPass_intersectStateVars<X86Reg::kKindMm >(this, a, b);
  X86RAPass_intersectStateVars<X86Reg::kKindVec>(this, a, b);

  ASMJIT_X86_CHECK_STATE
}

// ============================================================================
// [asmjit::X86RAPass - GetJccFlow / GetOppositeJccFlow]
// ============================================================================

//! \internal
static ASMJIT_INLINE CBNode* X86RAPass_getJccFlow(CBJump* jNode) {
  if (jNode->isTaken())
    return jNode->getTarget();
  else
    return jNode->getNext();
}

//! \internal
static ASMJIT_INLINE CBNode* X86RAPass_getOppositeJccFlow(CBJump* jNode) {
  if (jNode->isTaken())
    return jNode->getNext();
  else
    return jNode->getTarget();
}

// ============================================================================
// [asmjit::X86RAPass - SingleVarInst]
// ============================================================================

//! \internal
static void X86RAPass_prepareSingleVarInst(uint32_t instId, TiedReg* tr) {
  switch (instId) {
    // - andn     reg, reg ; Set all bits in reg to 0.
    // - xor/pxor reg, reg ; Set all bits in reg to 0.
    // - sub/psub reg, reg ; Set all bits in reg to 0.
    // - pcmpgt   reg, reg ; Set all bits in reg to 0.
    // - pcmpeq   reg, reg ; Set all bits in reg to 1.
    case X86Inst::kIdPandn     :
    case X86Inst::kIdXor       : case X86Inst::kIdXorpd     : case X86Inst::kIdXorps     : case X86Inst::kIdPxor      :
    case X86Inst::kIdSub:
    case X86Inst::kIdPsubb     : case X86Inst::kIdPsubw     : case X86Inst::kIdPsubd     : case X86Inst::kIdPsubq     :
    case X86Inst::kIdPsubsb    : case X86Inst::kIdPsubsw    : case X86Inst::kIdPsubusb   : case X86Inst::kIdPsubusw   :
    case X86Inst::kIdPcmpeqb   : case X86Inst::kIdPcmpeqw   : case X86Inst::kIdPcmpeqd   : case X86Inst::kIdPcmpeqq   :
    case X86Inst::kIdPcmpgtb   : case X86Inst::kIdPcmpgtw   : case X86Inst::kIdPcmpgtd   : case X86Inst::kIdPcmpgtq   :
      tr->flags &= ~TiedReg::kRReg;
      break;

    // - and      reg, reg ; Nop.
    // - or       reg, reg ; Nop.
    // - xchg     reg, reg ; Nop.
    case X86Inst::kIdAnd       : case X86Inst::kIdAndpd     : case X86Inst::kIdAndps     : case X86Inst::kIdPand      :
    case X86Inst::kIdOr        : case X86Inst::kIdOrpd      : case X86Inst::kIdOrps      : case X86Inst::kIdPor       :
    case X86Inst::kIdXchg      :
      tr->flags &= ~TiedReg::kWReg;
      break;
  }
}

// ============================================================================
// [asmjit::X86RAPass - Helpers]
// ============================================================================

static void X86RAPass_assignStackArgsRegId(X86RAPass* self, CCFunc* func) {
  const FuncDetail& fd = func->getDetail();
  FuncFrameInfo& ffi = func->getFrameInfo();

  // Select some register which will contain the base address of function
  // arguments and return address. The algorithm tries to select registers
  // which are saved or not preserved by default, if not successful it picks
  // any other register and adds it to `_savedRegs`.
  uint32_t stackArgsRegId;
  if (ffi.hasPreservedFP()) {
    stackArgsRegId = X86Gp::kIdBp;
  }
  else {
    // Passed registers as defined by the calling convention.
    uint32_t passed = fd.getPassedRegs(X86Reg::kKindGp);

    // Registers actually used to pass function arguments (related to this
    // function signature) with ESP|RSP included as this register can't be
    // used in general to hold anything bug stack pointer.
    uint32_t used = fd.getUsedRegs(X86Reg::kKindGp) | Utils::mask(X86Gp::kIdSp);

    // First try register that is defined to pass a function argument by the
    // calling convention, but is not used by this function. This will most
    // likely fail in 32-bit mode, but there is a high chance that it will
    // pass in 64-bit mode if the function doesn't use so many arguments.
    uint32_t regs = passed & ~used;

    // Pick any other register if that didn't work out.
    if (!regs) regs = ~passed & ~used;

    stackArgsRegId = Utils::findFirstBit(regs);
    ASMJIT_ASSERT(stackArgsRegId < self->cc()->getGpCount());
  }

  ffi.setStackArgsRegId(stackArgsRegId);
}

// ============================================================================
// [asmjit::X86RAPass - SArg Insertion]
// ============================================================================

struct SArgData {
  VirtReg* sVd;
  VirtReg* cVd;
  CCPushArg* sArg;
  uint32_t aType;
};

static ASMJIT_INLINE bool X86RAPass_mustConvertSArg(X86RAPass* self, uint32_t dstTypeId, uint32_t srcTypeId) noexcept{
  bool dstFloatSize = dstTypeId == TypeId::kF32   ? 4 :
                      dstTypeId == TypeId::kF64   ? 8 : 0;

  bool srcFloatSize = srcTypeId == TypeId::kF32   ? 4 :
                      srcTypeId == TypeId::kF32x1 ? 4 :
                      srcTypeId == TypeId::kF64   ? 8 :
                      srcTypeId == TypeId::kF64x1 ? 8 : 0;

  if (dstFloatSize && srcFloatSize)
    return dstFloatSize != srcFloatSize;
  else
    return false;
}

static ASMJIT_INLINE uint32_t X86RAPass_typeOfConvertedSArg(X86RAPass* self, uint32_t dstTypeId, uint32_t srcTypeId) noexcept {
  ASMJIT_ASSERT(X86RAPass_mustConvertSArg(self, dstTypeId, srcTypeId));
  return dstTypeId == TypeId::kF32 ? TypeId::kF32x1 : TypeId::kF64x1;
}

static ASMJIT_INLINE Error X86RAPass_insertPushArg(
  X86RAPass* self, CCFuncCall* call,
  VirtReg* sReg, const uint32_t* gaRegs,
  const FuncDetail::Value& arg, uint32_t argIndex,
  SArgData* sArgList, uint32_t& sArgCount) {

  X86Compiler* cc = self->cc();
  uint32_t i;
  uint32_t dstTypeId = arg.getTypeId();
  uint32_t srcTypeId = sReg->getTypeId();

  // First locate or create sArgBase.
  for (i = 0; i < sArgCount; i++)
    if (sArgList[i].sVd == sReg && !sArgList[i].cVd)
      break;

  SArgData* sArgData = &sArgList[i];
  if (i == sArgCount) {
    sArgData->sVd = sReg;
    sArgData->cVd = nullptr;
    sArgData->sArg = nullptr;
    sArgData->aType = 0xFF;
    sArgCount++;
  }

  uint32_t srcRegKind = sReg->getKind();

  // Only handles float<->double conversion.
  if (X86RAPass_mustConvertSArg(self, dstTypeId, srcTypeId)) {
    uint32_t cvtTypeId = X86RAPass_typeOfConvertedSArg(self, dstTypeId, srcTypeId);
    uint32_t cvtRegKind = X86Reg::kKindVec;

    while (++i < sArgCount) {
      sArgData = &sArgList[i];
      if (sArgData->sVd != sReg)
        break;

      if (sArgData->cVd->getTypeId() != cvtTypeId || sArgData->aType != dstTypeId)
        continue;

      sArgData->sArg->_args |= Utils::mask(argIndex);
      return kErrorOk;
    }

    VirtReg* cReg = cc->newVirtReg(dstTypeId, x86OpData.archRegs.regInfo[X86Reg::kRegXmm].getSignature(), nullptr);
    if (!cReg) return DebugUtils::errored(kErrorNoHeapMemory);

    CCPushArg* sArg = cc->newNodeT<CCPushArg>(call, sReg, cReg);
    if (!sArg) return DebugUtils::errored(kErrorNoHeapMemory);

    X86RAData* raData = self->newRAData(2);
    if (!raData) return DebugUtils::errored(kErrorNoHeapMemory);

    ASMJIT_PROPAGATE(self->assignRAId(cReg));
    ASMJIT_PROPAGATE(self->assignRAId(sReg));

    raData->tiedTotal = 2;
    raData->tiedCount.reset();
    raData->tiedCount.add(srcRegKind);
    raData->tiedCount.add(cvtRegKind);

    raData->tiedIndex.reset();
    raData->inRegs.reset();
    raData->outRegs.reset();
    raData->clobberedRegs.reset();

    if (srcRegKind <= cvtRegKind) {
      raData->tiedArray[0].init(sReg, TiedReg::kRReg, 0, gaRegs[srcRegKind]);
      raData->tiedArray[1].init(cReg, TiedReg::kWReg, 0, gaRegs[cvtRegKind]);
      raData->tiedIndex.set(cvtRegKind, srcRegKind != cvtRegKind);
    }
    else {
      raData->tiedArray[0].init(cReg, TiedReg::kWReg, 0, gaRegs[cvtRegKind]);
      raData->tiedArray[1].init(sReg, TiedReg::kRReg, 0, gaRegs[srcRegKind]);
      raData->tiedIndex.set(srcRegKind, 1);
    }

    sArg->setPassData(raData);
    sArg->_args |= Utils::mask(argIndex);

    cc->addBefore(sArg, call);
    ::memmove(sArgData + 1, sArgData, (sArgCount - i) * sizeof(SArgData));

    sArgData->sVd = sReg;
    sArgData->cVd = cReg;
    sArgData->sArg = sArg;
    sArgData->aType = dstTypeId;

    sArgCount++;
    return kErrorOk;
  }
  else {
    CCPushArg* sArg = sArgData->sArg;
    ASMJIT_PROPAGATE(self->assignRAId(sReg));

    if (!sArg) {
      sArg = cc->newNodeT<CCPushArg>(call, sReg, (VirtReg*)nullptr);
      if (!sArg) return DebugUtils::errored(kErrorNoHeapMemory);

      X86RAData* raData = self->newRAData(1);
      if (!raData) return DebugUtils::errored(kErrorNoHeapMemory);

      raData->tiedTotal = 1;
      raData->tiedIndex.reset();
      raData->tiedCount.reset();
      raData->tiedCount.add(srcRegKind);
      raData->inRegs.reset();
      raData->outRegs.reset();
      raData->clobberedRegs.reset();
      raData->tiedArray[0].init(sReg, TiedReg::kRReg, 0, gaRegs[srcRegKind]);

      sArg->setPassData(raData);
      sArgData->sArg = sArg;

      cc->addBefore(sArg, call);
    }

    sArg->_args |= Utils::mask(argIndex);
    return kErrorOk;
  }
}

// ============================================================================
// [asmjit::X86RAPass - Fetch]
// ============================================================================

//! \internal
//!
//! Prepare the given function `func`.
//!
//! For each node:
//! - Create and assign groupId and position.
//! - Collect all variables and merge them to vaList.
Error X86RAPass::fetch() {
  uint32_t archType = cc()->getArchType();
  CCFunc* func = getFunc();

  CBNode* node_ = func;
  CBNode* next = nullptr;
  CBNode* stop = getStop();

  TiedReg agTmp[80];
  SArgData sArgList[80];

  uint32_t position = 0;
  ZoneList<CBNode*>::Link* jLink = nullptr;

  // Global allocable registers.
  uint32_t* gaRegs = _gaRegs;

  if (func->getFrameInfo().hasPreservedFP())
    gaRegs[X86Reg::kKindGp] &= ~Utils::mask(X86Gp::kIdBp);

  // Allowed index registers (GP/XMM/YMM).
  const uint32_t indexMask = Utils::bits(_regCount.getGp()) & ~(Utils::mask(4));

  // --------------------------------------------------------------------------
  // [VI Macros]
  // --------------------------------------------------------------------------

#define RA_POPULATE(NODE) \
  do { \
    X86RAData* raData = newRAData(0); \
    if (!raData) goto NoMem; \
    NODE->setPassData(raData); \
  } while (0)

#define RA_DECLARE() \
  do { \
    X86RegCount tiedCount; \
    X86RegCount tiedIndex; \
    uint32_t tiedTotal = 0; \
    \
    X86RegMask inRegs; \
    X86RegMask outRegs; \
    X86RegMask clobberedRegs; \
    \
    tiedCount.reset(); \
    inRegs.reset(); \
    outRegs.reset(); \
    clobberedRegs.reset()

#define RA_FINALIZE(NODE) \
    { \
      X86RAData* raData = newRAData(tiedTotal); \
      if (!raData) goto NoMem; \
      \
      tiedIndex.indexFromRegCount(tiedCount); \
      raData->tiedCount = tiedCount; \
      raData->tiedIndex = tiedIndex; \
      \
      raData->inRegs = inRegs; \
      raData->outRegs = outRegs; \
      raData->clobberedRegs = clobberedRegs; \
      \
      TiedReg* tied = agTmp; \
      while (tiedTotal) { \
        VirtReg* vreg = tied->vreg; \
        \
        uint32_t _kind  = vreg->getKind(); \
        uint32_t _index = tiedIndex.get(_kind); \
        \
        tiedIndex.add(_kind); \
        if (tied->inRegs) \
          tied->allocableRegs = tied->inRegs; \
        else if (tied->outPhysId != Globals::kInvalidRegId) \
          tied->allocableRegs = Utils::mask(tied->outPhysId); \
        else \
          tied->allocableRegs &= ~inRegs.get(_kind); \
        \
        vreg->_tied = nullptr; \
        raData->setTiedAt(_index, *tied); \
        \
        tied++; \
        tiedTotal--; \
      } \
      NODE->setPassData(raData); \
     } \
  } while (0)

#define RA_INSERT(REG, TIED, FLAGS, NEW_ALLOCABLE) \
  do { \
    ASMJIT_ASSERT(REG->_tied == nullptr); \
    TIED = &agTmp[tiedTotal++]; \
    TIED->init(REG, FLAGS, 0, NEW_ALLOCABLE); \
    TIED->refCount++; \
    REG->_tied = TIED; \
    \
    if (assignRAId(REG) != kErrorOk) goto NoMem; \
    tiedCount.add(REG->getKind()); \
  } while (0)

#define RA_MERGE(REG, TIED, FLAGS, NEW_ALLOCABLE) \
  do { \
    TIED = REG->_tied; \
    \
    if (!TIED) { \
      TIED = &agTmp[tiedTotal++]; \
      TIED->init(REG, 0, 0, NEW_ALLOCABLE); \
      REG->_tied = TIED; \
      \
      if (assignRAId(REG) != kErrorOk) goto NoMem; \
      tiedCount.add(REG->getKind()); \
    } \
    \
    TIED->flags |= FLAGS; \
    TIED->refCount++; \
  } while (0)

  // --------------------------------------------------------------------------
  // [Loop]
  // --------------------------------------------------------------------------

  do {
_Do:
    while (node_->hasPassData()) {
_NextGroup:
      if (!jLink)
        jLink = _jccList.getFirst();
      else
        jLink = jLink->getNext();

      if (!jLink) goto _Done;
      node_ = X86RAPass_getOppositeJccFlow(static_cast<CBJump*>(jLink->getValue()));
    }

    position++;

    next = node_->getNext();
    node_->setPosition(position);

    switch (node_->getType()) {
      // ----------------------------------------------------------------------
      // [Align/Embed]
      // ----------------------------------------------------------------------

      case CBNode::kNodeAlign:
      case CBNode::kNodeData:
      default:
        RA_POPULATE(node_);
        break;

      // ----------------------------------------------------------------------
      // [Hint]
      // ----------------------------------------------------------------------

      case CBNode::kNodeHint: {
        CCHint* node = static_cast<CCHint*>(node_);
        RA_DECLARE();

        if (node->getHint() == CCHint::kHintAlloc) {
          uint32_t remain[Globals::kMaxVRegKinds];
          CCHint* cur = node;

          remain[X86Reg::kKindGp ] = _regCount.getGp() - 1 - func->getFrameInfo().hasPreservedFP();
          remain[X86Reg::kKindMm ] = _regCount.getMm();
          remain[X86Reg::kKindK  ] = _regCount.getK();
          remain[X86Reg::kKindVec] = _regCount.getVec();

          // Merge as many alloc-hints as possible.
          for (;;) {
            VirtReg* vreg = static_cast<VirtReg*>(cur->getVReg());
            TiedReg* tied = vreg->_tied;

            uint32_t kind = vreg->getKind();
            uint32_t physId = cur->getValue();
            uint32_t regMask = 0;

            // We handle both kInvalidReg and kInvalidValue.
            if (physId < Globals::kInvalidRegId)
              regMask = Utils::mask(physId);

            if (!tied) {
              if (inRegs.has(kind, regMask) || remain[kind] == 0)
                break;
              RA_INSERT(vreg, tied, TiedReg::kRReg, gaRegs[kind]);

              if (regMask != 0) {
                inRegs.xor_(kind, regMask);
                tied->inRegs = regMask;
                tied->setInPhysId(physId);
              }
              remain[kind]--;
            }
            else if (regMask != 0) {
              if (inRegs.has(kind, regMask) && tied->inRegs != regMask)
                break;

              inRegs.xor_(kind, tied->inRegs | regMask);
              tied->inRegs = regMask;
              tied->setInPhysId(physId);
            }

            if (cur != node)
              cc()->removeNode(cur);

            cur = static_cast<CCHint*>(node->getNext());
            if (!cur || cur->getType() != CBNode::kNodeHint || cur->getHint() != CCHint::kHintAlloc)
              break;
          }

          next = node->getNext();
        }
        else  {
          VirtReg* vreg = static_cast<VirtReg*>(node->getVReg());
          TiedReg* tied;

          uint32_t flags = 0;
          switch (node->getHint()) {
            case CCHint::kHintSpill       : flags = TiedReg::kRMem | TiedReg::kSpill; break;
            case CCHint::kHintSave        : flags = TiedReg::kRMem                  ; break;
            case CCHint::kHintSaveAndUnuse: flags = TiedReg::kRMem | TiedReg::kUnuse; break;
            case CCHint::kHintUnuse       : flags = TiedReg::kUnuse                 ; break;
          }
          RA_INSERT(vreg, tied, flags, 0);
        }

        RA_FINALIZE(node_);
        break;
      }

      // ----------------------------------------------------------------------
      // [Label]
      // ----------------------------------------------------------------------

      case CBNode::kNodeLabel: {
        RA_POPULATE(node_);
        if (node_ == func->getExitNode()) {
          ASMJIT_PROPAGATE(addReturningNode(node_));
          goto _NextGroup;
        }
        break;
      }

      // ----------------------------------------------------------------------
      // [Inst]
      // ----------------------------------------------------------------------

      case CBNode::kNodeInst: {
        CBInst* node = static_cast<CBInst*>(node_);

        uint32_t instId = node->getInstId();
        uint32_t flags = node->getFlags();
        uint32_t options = node->getOptions();
        uint32_t gpAllowedMask = 0xFFFFFFFF;

        Operand* opArray = node->getOpArray();
        uint32_t opCount = node->getOpCount();

        RA_DECLARE();
        if (opCount) {
          const X86Inst& inst = X86Inst::getInst(instId);
          const X86Inst::CommonData& commonData = inst.getCommonData();
          const X86SpecialInst* special = nullptr;

          // Collect instruction flags and merge all 'TiedReg's.
          if (commonData.isFpu())
            flags |= CBNode::kFlagIsFp;

          if (commonData.hasFixedRM() && (special = X86SpecialInst_get(instId, opArray, opCount)) != nullptr)
            flags |= CBNode::kFlagIsSpecial;

          for (uint32_t i = 0; i < opCount; i++) {
            Operand* op = &opArray[i];
            VirtReg* vreg;
            TiedReg* tied;

            if (op->isVirtReg()) {
              vreg = cc()->getVirtRegById(op->getId());
              if (vreg->isFixed()) continue;

              RA_MERGE(vreg, tied, 0, gaRegs[vreg->getKind()] & gpAllowedMask);
              if (static_cast<X86Reg*>(op)->isGpb()) {
                tied->flags |= static_cast<X86Gp*>(op)->isGpbLo() ? TiedReg::kX86GpbLo : TiedReg::kX86GpbHi;
                if (archType == ArchInfo::kTypeX86) {
                  // If a byte register is accessed in 32-bit mode we have to limit
                  // all allocable registers for that variable to eax/ebx/ecx/edx.
                  // Other variables are not affected.
                  tied->allocableRegs &= 0x0F;
                }
                else {
                  // It's fine if lo-byte register is accessed in 64-bit mode;
                  // however, hi-byte has to be checked and if it's used all
                  // registers (GP/XMM) could be only allocated in the lower eight
                  // half. To do that, we patch 'allocableRegs' of all variables
                  // we collected until now and change the allocable restriction
                  // for variables that come after.
                  if (static_cast<X86Gp*>(op)->isGpbHi()) {
                    tied->allocableRegs &= 0x0F;
                    if (gpAllowedMask != 0xFF) {
                      for (uint32_t j = 0; j < i; j++)
                        agTmp[j].allocableRegs &= (agTmp[j].flags & TiedReg::kX86GpbHi) ? 0x0F : 0xFF;
                      gpAllowedMask = 0xFF;
                    }
                  }
                }
              }

              if (special) {
                uint32_t inReg = special[i].inReg;
                uint32_t outReg = special[i].outReg;
                uint32_t c;

                if (static_cast<const X86Reg*>(op)->isGp())
                  c = X86Reg::kKindGp;
                else
                  c = X86Reg::kKindVec;

                if (inReg != Globals::kInvalidRegId) {
                  uint32_t mask = Utils::mask(inReg);
                  inRegs.or_(c, mask);
                  tied->inRegs |= mask;
                }

                if (outReg != Globals::kInvalidRegId) {
                  uint32_t mask = Utils::mask(outReg);
                  outRegs.or_(c, mask);
                  tied->setOutPhysId(outReg);
                }

                tied->flags |= special[i].flags;
              }
              else {
                uint32_t inFlags = TiedReg::kRReg;
                uint32_t outFlags = TiedReg::kWReg;
                uint32_t combinedFlags;

                if (i == 0) {
                  // Read/Write is usually the combination of the first operand.
                  combinedFlags = inFlags | outFlags;

                  if (node->getOptions() & CodeEmitter::kOptionOverwrite) {
                    // Manually forcing write-only.
                    combinedFlags = outFlags;
                  }
                  else if (commonData.isUseW()) {
                    // Write-only instruction.
                    uint32_t movSize = commonData.getWriteSize();
                    uint32_t regSize = vreg->getSize();

                    // Exception - If the source operand is a memory location
                    // promote move size into 16 bytes.
                    if (opArray[1].isMem() && inst.getOperationData().isMovSsSd())
                      movSize = 16;

                    if (static_cast<const X86Reg*>(op)->isGp()) {
                      uint32_t opSize = static_cast<const X86Reg*>(op)->getSize();

                      // Move size is zero in case that it should be determined
                      // from the destination register.
                      if (movSize == 0)
                        movSize = opSize;

                      // Handle the case that a 32-bit operation in 64-bit mode
                      // always clears the rest of the destination register and
                      // the case that move size is actually greater than or
                      // equal to the size of the variable.
                      if (movSize >= 4 || movSize >= regSize)
                        combinedFlags = outFlags;
                    }
                    else if (movSize == 0 || movSize >= regSize) {
                      // If move size is greater than or equal to the size of
                      // the variable there is nothing to do, because the move
                      // will overwrite the variable in all cases.
                      combinedFlags = outFlags;
                    }
                  }
                  else if (commonData.isUseR()) {
                    // Comparison/Test instructions don't modify any operand.
                    combinedFlags = inFlags;
                  }
                  else if (instId == X86Inst::kIdImul && opCount == 3) {
                    // Imul.
                    combinedFlags = outFlags;
                  }
                }
                else {
                  // Read-Only is usualy the combination of the second/third/fourth operands.
                  combinedFlags = inFlags;

                  // Idiv is a special instruction, never handled here.
                  ASMJIT_ASSERT(instId != X86Inst::kIdIdiv);

                  // Xchg/Xadd/Imul.
                  if (commonData.isUseXX() || (instId == X86Inst::kIdImul && opCount == 3 && i == 1))
                    combinedFlags = inFlags | outFlags;
                }
                tied->flags |= combinedFlags;
              }
            }
            else if (op->isMem()) {
              X86Mem* m = static_cast<X86Mem*>(op);
              node->setMemOpIndex(i);

              uint32_t specBase = special ? uint32_t(special[i].inReg) : uint32_t(Globals::kInvalidRegId);

              if (m->hasBaseReg()) {
                uint32_t id = m->getBaseId();
                if (cc()->isVirtRegValid(id)) {
                  vreg = cc()->getVirtRegById(id);
                  if (!vreg->isStack() && !vreg->isFixed()) {
                    RA_MERGE(vreg, tied, 0, gaRegs[vreg->getKind()] & gpAllowedMask);
                    if (m->isRegHome()) {
                      uint32_t inFlags = TiedReg::kRMem;
                      uint32_t outFlags = TiedReg::kWMem;
                      uint32_t combinedFlags;

                      if (i == 0) {
                        // Default for the first operand.
                        combinedFlags = inFlags | outFlags;

                        if (commonData.isUseW()) {
                          // Move to memory - setting the right flags is important
                          // as if it's just move to the register. It's just a bit
                          // simpler as there are no special cases.
                          uint32_t movSize = std::max<uint32_t>(commonData.getWriteSize(), m->getSize());
                          uint32_t regSize = vreg->getSize();

                          if (movSize >= regSize)
                            combinedFlags = outFlags;
                        }
                        else if (commonData.isUseR()) {
                          // Comparison/Test instructions don't modify any operand.
                          combinedFlags = inFlags;
                        }
                      }
                      else {
                        // Default for the second operand.
                        combinedFlags = inFlags;

                        // Handle Xchg instruction (modifies both operands).
                        if (commonData.isUseXX())
                          combinedFlags = inFlags | outFlags;
                      }

                      tied->flags |= combinedFlags;
                    }
                    else {
                      if (specBase != Globals::kInvalidRegId) {
                        uint32_t mask = Utils::mask(specBase);
                        inRegs.or_(vreg->getKind(), mask);
                        outRegs.or_(vreg->getKind(), mask);
                        tied->inRegs |= mask;
                        tied->setOutPhysId(specBase);
                        tied->flags |= special[i].flags;
                      }
                      else {
                        tied->flags |= TiedReg::kRReg;
                      }
                    }
                  }
                }
              }

              if (m->hasIndexReg()) {
                uint32_t id = m->getIndexId();
                if (cc()->isVirtRegValid(id)) {
                  // Restrict allocation to all registers except ESP|RSP.
                  vreg = cc()->getVirtRegById(m->getIndexId());
                  if (!vreg->isFixed()) {
                    // TODO: AVX vector operands support.
                    RA_MERGE(vreg, tied, 0, gaRegs[X86Reg::kKindGp] & gpAllowedMask);
                    tied->allocableRegs &= indexMask;
                    tied->flags |= TiedReg::kRReg;
                  }
                }
              }
            }
          }

          node->setFlags(flags);
          if (tiedTotal) {
            // Handle instructions which result in zeros/ones or nop if used with the
            // same destination and source operand.
            if (tiedTotal == 1 && opCount >= 2 && opArray[0].isVirtReg() && opArray[1].isVirtReg() && !node->hasMemOp())
              X86RAPass_prepareSingleVarInst(instId, &agTmp[0]);
          }

          // Turn on AVX if the instruction operates on XMM|YMM|ZMM registers and uses VEX|EVEX prefix.
          if (tiedCount.getVec() && commonData.hasFlag(X86Inst::kFlagVex | X86Inst::kFlagEvex))
            _avxEnabled = true;
        }

        const RegOnly& extraReg = node->getExtraReg();
        if (extraReg.isValid()) {
          uint32_t id = extraReg.getId();
          if (cc()->isVirtRegValid(id)) {
            VirtReg* vreg = cc()->getVirtRegById(id);
            TiedReg* tied;
            RA_MERGE(vreg, tied, 0, gaRegs[vreg->getKind()] & gpAllowedMask);

            if (options & (X86Inst::kOptionRep | X86Inst::kOptionRepnz)) {
              tied->allocableRegs = Utils::mask(X86Gp::kIdCx);
              tied->flags |= TiedReg::kXReg;
            }
            else {
              tied->flags |= TiedReg::kRReg;
            }
          }
        }

        RA_FINALIZE(node_);

        // Handle conditional/unconditional jump.
        if (node->isJmpOrJcc()) {
          CBJump* jNode = static_cast<CBJump*>(node);
          CBLabel* jTarget = jNode->getTarget();

          // If this jump is unconditional we put next node to unreachable node
          // list so we can eliminate possible dead code. We have to do this in
          // all cases since we are unable to translate without fetch() step.
          //
          // We also advance our node pointer to the target node to simulate
          // natural flow of the function.
          if (jNode->isJmp()) {
            if (next && !next->hasPassData())
              ASMJIT_PROPAGATE(addUnreachableNode(next));

            // Jump not followed.
            if (!jTarget) {
              ASMJIT_PROPAGATE(addReturningNode(jNode));
              goto _NextGroup;
            }

            node_ = jTarget;
            goto _Do;
          }
          else {
            // Jump not followed.
            if (!jTarget) break;

            if (jTarget->hasPassData()) {
              uint32_t jTargetPosition = jTarget->getPosition();

              // Update CBNode::kFlagIsTaken to true if this is a conditional
              // backward jump. This behavior can be overridden by using
              // `X86Inst::kOptionTaken` when the instruction is created.
              if (!jNode->isTaken() && opCount == 1 && jTargetPosition <= position) {
                jNode->_flags |= CBNode::kFlagIsTaken;
              }
            }
            else if (next->hasPassData()) {
              node_ = jTarget;
              goto _Do;
            }
            else {
              ASMJIT_PROPAGATE(addJccNode(jNode));
              node_ = X86RAPass_getJccFlow(jNode);
              goto _Do;
            }
          }
        }
        break;
      }

      // ----------------------------------------------------------------------
      // [Func-Entry]
      // ----------------------------------------------------------------------

      case CBNode::kNodeFunc: {
        ASMJIT_ASSERT(node_ == func);
        X86RAPass_assignStackArgsRegId(this, func);

        FuncDetail& fd = func->getDetail();
        TiedReg* tied;

        RA_DECLARE();
        cc()->setCursor(node_);

        X86Gp saReg;
        uint32_t argCount = fd.getArgCount();

        for (uint32_t i = 0; i < argCount; i++) {
          const FuncDetail::Value& arg = fd.getArg(i);

          VirtReg* vReg = func->getArg(i);
          if (!vReg) continue;

          // Overlapped function arguments.
          if (vReg->_tied)
            return DebugUtils::errored(kErrorOverlappedRegs);

          uint32_t aKind = X86Reg::kindOf(arg.getRegType());
          uint32_t vKind = vReg->getKind();

          if (arg.byReg()) {
            if (aKind == vKind) {
              RA_INSERT(vReg, tied, TiedReg::kWReg, 0);
              tied->setOutPhysId(arg.getRegId());
            }
            else {
              X86Reg rTmp = cc()->newReg(arg.getTypeId(), "arg%u", i);
              VirtReg* vTmp = cc()->getVirtReg(rTmp);

              RA_INSERT(vTmp, tied, TiedReg::kWReg, 0);
              tied->setOutPhysId(arg.getRegId());

              X86Reg dstReg(X86Reg::fromSignature(vReg->getSignature(), vReg->getId()));
              X86Reg srcReg(X86Reg::fromSignature(vTmp->getSignature(), vTmp->getId()));

              // Emit conversion after the prolog.
              return X86Internal::emitArgMove(reinterpret_cast<X86Emitter*>(cc()),
                dstReg, vReg->getTypeId(),
                srcReg, vTmp->getTypeId(), _avxEnabled);
            }
          }
          else {
            // Instead of complicating the prolog allocation we create a virtual
            // register that holds the base address to all arguments passed by
            // stack and then insert nodes that copy these arguments to registers.
            if (!saReg.isValid()) {
              saReg = cc()->newGpz("__args");
              if (!saReg.isValid()) goto NoMem;

              VirtReg* saBase = cc()->getVirtReg(saReg);
              RA_INSERT(saBase, tied, TiedReg::kWReg, 0);

              if (func->getFrameInfo().hasPreservedFP())
                saBase->_isFixed = true;
              tied->setOutPhysId(func->getFrameInfo().getStackArgsRegId());
            }

            // Argument passed by stack is handled after the prolog.
            X86Gp aReg = X86Gp::fromSignature(vReg->getSignature(), vReg->getId());
            X86Mem aMem = x86::ptr(saReg, arg.getStackOffset());
            aMem.setArgHome();

            ASMJIT_PROPAGATE(
              X86Internal::emitArgMove(reinterpret_cast<X86Emitter*>(cc()),
                aReg, vReg->getTypeId(), aMem, arg.getTypeId(), _avxEnabled));
          }
        }

        // If saReg is not needed, clear it also from FuncFrameInfo.
        if (!saReg.isValid())
          func->getFrameInfo().setStackArgsRegId(Globals::kInvalidRegId);

        RA_FINALIZE(node_);
        next = node_->getNext();
        break;
      }

      // ----------------------------------------------------------------------
      // [End]
      // ----------------------------------------------------------------------

      case CBNode::kNodeSentinel: {
        RA_POPULATE(node_);
        ASMJIT_PROPAGATE(addReturningNode(node_));
        goto _NextGroup;
      }

      // ----------------------------------------------------------------------
      // [Func-Exit]
      // ----------------------------------------------------------------------

      case CBNode::kNodeFuncExit: {
        CCFuncRet* node = static_cast<CCFuncRet*>(node_);
        ASMJIT_PROPAGATE(addReturningNode(node));

        FuncDetail& fd = func->getDetail();
        RA_DECLARE();

        if (fd.hasRet()) {
          const FuncDetail::Value& ret = fd.getRet(0);
          uint32_t retKind = X86Reg::kindOf(ret.getRegType());

          for (uint32_t i = 0; i < 2; i++) {
            Operand_* op = &node->_ret[i];
            if (op->isVirtReg()) {
              VirtReg* vreg = cc()->getVirtRegById(op->getId());
              TiedReg* tied;
              RA_MERGE(vreg, tied, 0, 0);

              if (retKind == vreg->getKind()) {
                tied->flags |= TiedReg::kRReg;
                tied->inRegs = Utils::mask(ret.getRegId());
                inRegs.or_(retKind, tied->inRegs);
              }
              else if (retKind == X86Reg::kKindFp) {
                uint32_t fldFlag = ret.getTypeId() == TypeId::kF32 ? TiedReg::kX86Fld4 : TiedReg::kX86Fld8;
                tied->flags |= TiedReg::kRMem | fldFlag;
              }
              else {
                // TODO: Fix possible other return type conversions.
                ASMJIT_NOT_REACHED();
              }
            }
          }
        }
        RA_FINALIZE(node_);

        if (!next->hasPassData())
          ASMJIT_PROPAGATE(addUnreachableNode(next));
        goto _NextGroup;
      }

      // ----------------------------------------------------------------------
      // [Func-Call]
      // ----------------------------------------------------------------------

      case CBNode::kNodeFuncCall: {
        CCFuncCall* node = static_cast<CCFuncCall*>(node_);
        FuncDetail& fd = node->getDetail();

        Operand_* target = node->_opArray;
        Operand_* args = node->_args;
        Operand_* rets = node->_ret;

        func->getFrameInfo().enableCalls();
        func->getFrameInfo().mergeCallFrameSize(fd.getArgStackSize());
        // TODO: Each function frame should also define its stack arguments' alignment.
        // func->getFrameInfo().mergeCallFrameAlignment();

        uint32_t i;
        uint32_t argCount = fd.getArgCount();
        uint32_t sArgCount = 0;
        uint32_t gpAllocableMask = gaRegs[X86Reg::kKindGp] & ~node->getDetail().getUsedRegs(X86Reg::kKindGp);

        VirtReg* vreg;
        TiedReg* tied;

        RA_DECLARE();

        // Function-call operand.
        if (target->isVirtReg()) {
          vreg = cc()->getVirtRegById(target->getId());
          RA_MERGE(vreg, tied, 0, 0);

          tied->flags |= TiedReg::kRReg | TiedReg::kRCall;
          if (tied->inRegs == 0)
            tied->allocableRegs |= gpAllocableMask;
        }
        else if (target->isMem()) {
          X86Mem* m = static_cast<X86Mem*>(target);

          if (m->hasBaseReg() &&  Operand::isPackedId(m->getBaseId())) {
            vreg = cc()->getVirtRegById(m->getBaseId());
            if (!vreg->isStack()) {
              RA_MERGE(vreg, tied, 0, 0);
              if (m->isRegHome()) {
                tied->flags |= TiedReg::kRMem | TiedReg::kRCall;
              }
              else {
                tied->flags |= TiedReg::kRReg | TiedReg::kRCall;
                if (tied->inRegs == 0)
                  tied->allocableRegs |= gpAllocableMask;
              }
            }
          }

          if (m->hasIndexReg() && Operand::isPackedId(m->getIndexId())) {
            // Restrict allocation to all registers except ESP/RSP.
            vreg = cc()->getVirtRegById(m->getIndexId());
            RA_MERGE(vreg, tied, 0, 0);

            tied->flags |= TiedReg::kRReg | TiedReg::kRCall;
            if ((tied->inRegs & ~indexMask) == 0)
              tied->allocableRegs &= gpAllocableMask & indexMask;
          }
        }

        // Function-call arguments.
        for (i = 0; i < argCount; i++) {
          Operand_* op = &args[i];
          if (!op->isVirtReg()) continue;

          vreg = cc()->getVirtRegById(op->getId());
          const FuncDetail::Value& arg = fd.getArg(i);

          if (arg.byReg()) {
            RA_MERGE(vreg, tied, 0, 0);

            uint32_t argClass = X86Reg::kindOf(arg.getRegType());

            if (vreg->getKind() == argClass) {
              tied->inRegs |= Utils::mask(arg.getRegId());
              tied->flags |= TiedReg::kRReg | TiedReg::kRFunc;
            }
            else {
              // TODO: Function-call argument conversion.
            }
          }
          // If this is a stack-based argument we insert CCPushArg instead of
          // using TiedReg. It improves the code, because the argument can be
          // moved onto stack as soon as it is ready and the register used by
          // the variable can be reused for something else. It is also much
          // easier to handle argument conversions, because there will be at
          // most only one node per conversion.
          else {
            if (X86RAPass_insertPushArg(this, node, vreg, gaRegs, arg, i, sArgList, sArgCount) != kErrorOk)
              goto NoMem;
          }
        }

        // Function-call returns.
        for (i = 0; i < 2; i++) {
          Operand_* op = &rets[i];
          if (!op->isVirtReg()) continue;

          const FuncDetail::Value& ret = fd.getRet(i);
          if (ret.byReg()) {
            uint32_t retKind = X86Reg::kindOf(ret.getRegType());

            vreg = cc()->getVirtRegById(op->getId());
            RA_MERGE(vreg, tied, 0, 0);

            if (vreg->getKind() == retKind) {
              tied->setOutPhysId(ret.getRegId());
              tied->flags |= TiedReg::kWReg | TiedReg::kWFunc;
            }
            else {
              // TODO: Function-call return value conversion.
            }
          }
        }

        // Init clobbered.
        clobberedRegs.set(X86Reg::kKindGp , Utils::bits(_regCount.getGp())  & (fd.getPassedRegs(X86Reg::kKindGp ) | ~fd.getPreservedRegs(X86Reg::kKindGp )));
        clobberedRegs.set(X86Reg::kKindMm , Utils::bits(_regCount.getMm())  & (fd.getPassedRegs(X86Reg::kKindMm ) | ~fd.getPreservedRegs(X86Reg::kKindMm )));
        clobberedRegs.set(X86Reg::kKindK  , Utils::bits(_regCount.getK())   & (fd.getPassedRegs(X86Reg::kKindK  ) | ~fd.getPreservedRegs(X86Reg::kKindK  )));
        clobberedRegs.set(X86Reg::kKindVec, Utils::bits(_regCount.getVec()) & (fd.getPassedRegs(X86Reg::kKindVec) | ~fd.getPreservedRegs(X86Reg::kKindVec)));

        RA_FINALIZE(node_);
        break;
      }
    }

    node_ = next;
  } while (node_ != stop);

_Done:
  // Mark exit label and end node as fetched, otherwise they can be removed by
  // `removeUnreachableCode()`, which could lead to a crash in some later step.
  node_ = func->getEnd();
  if (!node_->hasPassData()) {
    CBLabel* fExit = func->getExitNode();
    RA_POPULATE(fExit);
    fExit->setPosition(++position);

    RA_POPULATE(node_);
    node_->setPosition(++position);
  }
  return kErrorOk;

  // --------------------------------------------------------------------------
  // [Failure]
  // --------------------------------------------------------------------------

NoMem:
  return DebugUtils::errored(kErrorNoHeapMemory);
}

// ============================================================================
// [asmjit::X86RAPass - Annotate]
// ============================================================================

Error X86RAPass::annotate() {
#if !defined(ASMJIT_DISABLE_LOGGING)
  CCFunc* func = getFunc();

  CBNode* node_ = func;
  CBNode* end = func->getEnd();

  Zone& dataZone = cc()->_cbDataZone;
  StringBuilderTmp<256> sb;

  uint32_t maxLen = 0;
  while (node_ && node_ != end) {
    if (!node_->hasInlineComment()) {
      if (node_->getType() == CBNode::kNodeInst) {
        CBInst* node = static_cast<CBInst*>(node_);
        Logging::formatInstruction(
          sb,
          0,
          cc(),
          cc()->getArchType(),
          node->getInstDetail(), node->getOpArray(), node->getOpCount());

        node_->setInlineComment(
          static_cast<char*>(dataZone.dup(sb.getData(), sb.getLength(), true)));
        maxLen = std::max<uint32_t>(maxLen, static_cast<uint32_t>(sb.getLength()));

        sb.clear();
      }
    }

    node_ = node_->getNext();
  }
  _annotationLength = maxLen + 1;
#endif // !ASMJIT_DISABLE_LOGGING

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86BaseAlloc]
// ============================================================================

struct X86BaseAlloc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86BaseAlloc(X86RAPass* context) {
    _context = context;
    _cc = context->cc();
  }
  ASMJIT_INLINE ~X86BaseAlloc() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the context.
  ASMJIT_INLINE X86RAPass* getContext() const { return _context; }
  //! Get the current state (always the same instance as X86RAPass::_x86State).
  ASMJIT_INLINE X86RAState* getState() const { return _context->getState(); }

  //! Get the node.
  ASMJIT_INLINE CBNode* getNode() const { return _node; }

  //! Get TiedReg list (all).
  ASMJIT_INLINE TiedReg* getTiedArray() const { return _tiedArray[0]; }
  //! Get TiedReg list (per class).
  ASMJIT_INLINE TiedReg* getTiedArrayByKind(uint32_t kind) const { return _tiedArray[kind]; }

  //! Get TiedReg count (all).
  ASMJIT_INLINE uint32_t getTiedCount() const { return _tiedTotal; }
  //! Get TiedReg count (per class).
  ASMJIT_INLINE uint32_t getTiedCountByKind(uint32_t kind) const { return _tiedCount.get(kind); }

  //! Get if all variables of the given register `kind` are done.
  ASMJIT_INLINE bool isTiedDone(uint32_t kind) const { return _tiedDone.get(kind) == _tiedCount.get(kind); }

  //! Get how many variables have been allocated.
  ASMJIT_INLINE uint32_t getTiedDone(uint32_t kind) const { return _tiedDone.get(kind); }
  //! Add to the count of variables allocated.
  ASMJIT_INLINE void addTiedDone(uint32_t kind, uint32_t n = 1) { _tiedDone.add(kind, n); }

  //! Get number of allocable registers per class.
  ASMJIT_INLINE uint32_t getGaRegs(uint32_t kind) const {
    return _context->_gaRegs[kind];
  }

  // --------------------------------------------------------------------------
  // [Init / Cleanup]
  // --------------------------------------------------------------------------

protected:
  // Just to prevent calling these methods by X86RAPass::translate().
  ASMJIT_INLINE void init(CBNode* node, X86RAData* map);
  ASMJIT_INLINE void cleanup();

  // --------------------------------------------------------------------------
  // [Unuse]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void unuseBefore();

  template<int C>
  ASMJIT_INLINE void unuseAfter();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! RA context.
  X86RAPass* _context;
  //! Compiler.
  X86Compiler* _cc;

  //! Node.
  CBNode* _node;

  //! Register allocator (RA) data.
  X86RAData* _raData;
  //! TiedReg list (per register kind).
  TiedReg* _tiedArray[Globals::kMaxVRegKinds];

  //! Count of all TiedReg's.
  uint32_t _tiedTotal;

  //! TiedReg's total counter.
  X86RegCount _tiedCount;
  //! TiedReg's done counter.
  X86RegCount _tiedDone;
};

// ============================================================================
// [asmjit::X86BaseAlloc - Init / Cleanup]
// ============================================================================

ASMJIT_INLINE void X86BaseAlloc::init(CBNode* node, X86RAData* raData) {
  _node = node;
  _raData = raData;

  // We have to set the correct cursor in case any instruction is emitted
  // during the allocation phase; it has to be emitted before the current
  // instruction.
  _cc->_setCursor(node->getPrev());

  // Setup the lists of variables.
  {
    TiedReg* tied = raData->getTiedArray();
    _tiedArray[X86Reg::kKindGp ] = tied;
    _tiedArray[X86Reg::kKindMm ] = tied + raData->getTiedStart(X86Reg::kKindMm );
    _tiedArray[X86Reg::kKindK  ] = tied + raData->getTiedStart(X86Reg::kKindK  );
    _tiedArray[X86Reg::kKindVec] = tied + raData->getTiedStart(X86Reg::kKindVec);
  }

  // Setup counters.
  _tiedTotal = raData->tiedTotal;
  _tiedCount = raData->tiedCount;
  _tiedDone.reset();

  // Connect VREG->TIED.
  for (uint32_t i = 0; i < _tiedTotal; i++) {
    TiedReg* tied = &_tiedArray[0][i];
    VirtReg* vreg = tied->vreg;
    vreg->_tied = tied;
  }
}

ASMJIT_INLINE void X86BaseAlloc::cleanup() {
  // Disconnect VREG->TIED.
  for (uint32_t i = 0; i < _tiedTotal; i++) {
    TiedReg* tied = &_tiedArray[0][i];
    VirtReg* vreg = tied->vreg;
    vreg->_tied = nullptr;
  }
}

// ============================================================================
// [asmjit::X86BaseAlloc - Unuse]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86BaseAlloc::unuseBefore() {
  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  const uint32_t checkFlags = TiedReg::kXReg  |
                              TiedReg::kRMem  |
                              TiedReg::kRFunc |
                              TiedReg::kRCall ;

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    if ((tied->flags & checkFlags) == TiedReg::kWReg)
      _context->unuse<C>(tied->vreg);
  }
}

template<int C>
ASMJIT_INLINE void X86BaseAlloc::unuseAfter() {
  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    if (tied->flags & TiedReg::kUnuse)
      _context->unuse<C>(tied->vreg);
  }
}

// ============================================================================
// [asmjit::X86VarAlloc]
// ============================================================================

//! \internal
//!
//! Register allocator context (asm instructions).
struct X86VarAlloc : public X86BaseAlloc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86VarAlloc(X86RAPass* context) : X86BaseAlloc(context) {}
  ASMJIT_INLINE ~X86VarAlloc() {}

  // --------------------------------------------------------------------------
  // [Run]
  // --------------------------------------------------------------------------

  Error run(CBNode* node);

  // --------------------------------------------------------------------------
  // [Init / Cleanup]
  // --------------------------------------------------------------------------

protected:
  // Just to prevent calling these methods by X86RAPass::translate().
  ASMJIT_INLINE void init(CBNode* node, X86RAData* map);
  ASMJIT_INLINE void cleanup();

  // --------------------------------------------------------------------------
  // [Plan / Spill / Alloc]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void plan();

  template<int C>
  ASMJIT_INLINE void spill();

  template<int C>
  ASMJIT_INLINE void alloc();

  // --------------------------------------------------------------------------
  // [GuessAlloc / GuessSpill]
  // --------------------------------------------------------------------------

  //! Guess which register is the best candidate for `vreg` from `allocableRegs`.
  //!
  //! The guess is based on looking ahead and inspecting register allocator
  //! instructions. The main reason is to prevent allocation to a register
  //! which is needed by next instruction(s). The guess look tries to go as far
  //! as possible, after the remaining registers are zero, the mask of previous
  //! registers (called 'safeRegs') is returned.
  template<int C>
  ASMJIT_INLINE uint32_t guessAlloc(VirtReg* vreg, uint32_t allocableRegs);

  //! Guess whether to move the given `vreg` instead of spill.
  template<int C>
  ASMJIT_INLINE uint32_t guessSpill(VirtReg* vreg, uint32_t allocableRegs);

  // --------------------------------------------------------------------------
  // [Modified]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void modified();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Will alloc to these registers.
  X86RegMask _willAlloc;
  //! Will spill these registers.
  X86RegMask _willSpill;
};

// ============================================================================
// [asmjit::X86VarAlloc - Run]
// ============================================================================

Error X86VarAlloc::run(CBNode* node_) {
  // Initialize.
  X86RAData* raData = node_->getPassData<X86RAData>();
  // Initialize the allocator; connect Vd->Va.
  init(node_, raData);

  if (raData->tiedTotal != 0) {
    // Unuse overwritten variables.
    unuseBefore<X86Reg::kKindGp>();
    unuseBefore<X86Reg::kKindMm>();
    unuseBefore<X86Reg::kKindVec>();

    // Plan the allocation. Planner assigns input/output registers for each
    // variable and decides whether to allocate it in register or stack.
    plan<X86Reg::kKindGp>();
    plan<X86Reg::kKindMm>();
    plan<X86Reg::kKindVec>();

    // Spill all variables marked by plan().
    spill<X86Reg::kKindGp>();
    spill<X86Reg::kKindMm>();
    spill<X86Reg::kKindVec>();

    // Alloc all variables marked by plan().
    alloc<X86Reg::kKindGp>();
    alloc<X86Reg::kKindMm>();
    alloc<X86Reg::kKindVec>();

    // Translate node operands.
    if (node_->getType() == CBNode::kNodeInst) {
      CBInst* node = static_cast<CBInst*>(node_);
      if (node->hasExtraReg()) {
        Reg reg = node->getExtraReg().toReg<Reg>();
        ASMJIT_PROPAGATE(X86RAPass_translateOperands(_context, &reg, 1));
        node->setExtraReg(reg);
      }
      ASMJIT_PROPAGATE(X86RAPass_translateOperands(_context, node->getOpArray(), node->getOpCount()));
    }
    else if (node_->getType() == CBNode::kNodePushArg) {
      CCPushArg* node = static_cast<CCPushArg*>(node_);

      CCFuncCall* call = static_cast<CCFuncCall*>(node->getCall());
      FuncDetail& fd = call->getDetail();

      uint32_t argIndex = 0;
      uint32_t argMask = node->_args;

      VirtReg* cvtReg = node->getCvtReg();
      VirtReg* srcReg = node->getSrcReg();

      // Convert first.
      ASMJIT_ASSERT(srcReg->getPhysId() != Globals::kInvalidRegId);

      if (cvtReg) {
        ASMJIT_ASSERT(cvtReg->getPhysId() != Globals::kInvalidRegId);

        X86Reg dstOp(X86Reg::fromSignature(cvtReg->getSignature(), cvtReg->getId()));
        X86Reg srcOp(X86Reg::fromSignature(srcReg->getSignature(), srcReg->getId()));

        // Emit conversion after the prolog.
        X86Internal::emitArgMove(reinterpret_cast<X86Emitter*>(_context->cc()),
          dstOp, cvtReg->getTypeId(),
          srcOp, srcReg->getTypeId(), _context->_avxEnabled);
        srcReg = cvtReg;
      }

      while (argMask != 0) {
        if (argMask & 0x1) {
          FuncDetail::Value& arg = fd.getArg(argIndex);
          ASMJIT_ASSERT(arg.byStack());

          X86Mem dst = x86::ptr(_context->_zsp, -static_cast<int>(_context->getGpSize()) + arg.getStackOffset());
          _context->emitRegToStack(arg.getTypeId(), &dst, srcReg->getTypeId(), srcReg->getPhysId());
        }

        argIndex++;
        argMask >>= 1;
      }
    }

    // Mark variables as modified.
    modified<X86Reg::kKindGp>();
    modified<X86Reg::kKindMm>();
    modified<X86Reg::kKindVec>();

    // Cleanup; disconnect Vd->Va.
    cleanup();

    // Update clobbered mask.
    _context->_clobberedRegs.or_(_willAlloc);
  }

  // Update clobbered mask.
  _context->_clobberedRegs.or_(raData->clobberedRegs);

  // Unuse.
  if (raData->tiedTotal != 0) {
    unuseAfter<X86Reg::kKindGp>();
    unuseAfter<X86Reg::kKindMm>();
    unuseAfter<X86Reg::kKindVec>();
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86VarAlloc - Init / Cleanup]
// ============================================================================

ASMJIT_INLINE void X86VarAlloc::init(CBNode* node, X86RAData* raData) {
  X86BaseAlloc::init(node, raData);

  // These will block planner from assigning them during planning. Planner will
  // add more registers when assigning registers to variables that don't need
  // any specific register.
  _willAlloc = raData->inRegs;
  _willAlloc.or_(raData->outRegs);
  _willSpill.reset();
}

ASMJIT_INLINE void X86VarAlloc::cleanup() {
  X86BaseAlloc::cleanup();
}

// ============================================================================
// [asmjit::X86VarAlloc - Plan / Spill / Alloc]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86VarAlloc::plan() {
  if (isTiedDone(C)) return;

  uint32_t i;
  uint32_t willAlloc = _willAlloc.get(C);
  uint32_t willFree = 0;

  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);
  X86RAState* state = getState();

  // Calculate 'willAlloc' and 'willFree' masks based on mandatory masks.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    uint32_t vaFlags = tied->flags;
    uint32_t physId = vreg->getPhysId();
    uint32_t regMask = (physId != Globals::kInvalidRegId) ? Utils::mask(physId) : 0;

    if ((vaFlags & TiedReg::kXReg) != 0) {
      // Planning register allocation. First check whether the variable is
      // already allocated in register and if it can stay allocated there.
      //
      // The following conditions may happen:
      //
      // a) Allocated register is one of the mandatoryRegs.
      // b) Allocated register is one of the allocableRegs.
      uint32_t mandatoryRegs = tied->inRegs;
      uint32_t allocableRegs = tied->allocableRegs;

      if (regMask != 0) {
        // Special path for planning output-only registers.
        if ((vaFlags & TiedReg::kXReg) == TiedReg::kWReg) {
          uint32_t outPhysId = tied->outPhysId;
          mandatoryRegs = (outPhysId != Globals::kInvalidRegId) ? Utils::mask(outPhysId) : 0;

          if ((mandatoryRegs | allocableRegs) & regMask) {
            tied->setOutPhysId(physId);
            tied->flags |= TiedReg::kWDone;

            if (mandatoryRegs & regMask) {
              // Case 'a' - 'willAlloc' contains initially all inRegs from all TiedReg's.
              ASMJIT_ASSERT((willAlloc & regMask) != 0);
            }
            else {
              // Case 'b'.
              tied->setOutPhysId(physId);
              willAlloc |= regMask;
            }

            addTiedDone(C);
            continue;
          }
        }
        else {
          if ((mandatoryRegs | allocableRegs) & regMask) {
            tied->setInPhysId(physId);
            tied->flags |= TiedReg::kRDone;

            if (mandatoryRegs & regMask) {
              // Case 'a' - 'willAlloc' contains initially all inRegs from all TiedReg's.
              ASMJIT_ASSERT((willAlloc & regMask) != 0);
            }
            else {
              // Case 'b'.
              tied->inRegs |= regMask;
              willAlloc |= regMask;
            }

            addTiedDone(C);
            continue;
          }
        }
      }

      // Variable is not allocated or allocated in register that doesn't
      // match inRegs or allocableRegs. The next step is to pick the best
      // register for this variable. If `inRegs` contains any register the
      // decision is simple - we have to follow, in other case will use
      // the advantage of `guessAlloc()` to find a register (or registers)
      // by looking ahead. But the best way to find a good register is not
      // here since now we have no information about the registers that
      // will be freed. So instead of finding register here, we just mark
      // the current register (if variable is allocated) as `willFree` so
      // the planner can use this information in the second step to plan the
      // allocation as a whole.
      willFree |= regMask;
      continue;
    }
    else {
      if (regMask != 0) {
        willFree |= regMask;
        continue;
      }
      else {
        tied->flags |= TiedReg::kRDone;
        addTiedDone(C);
        continue;
      }
    }
  }

  // Occupied registers without 'willFree' registers; contains basically
  // all the registers we can use to allocate variables without inRegs
  // specified.
  uint32_t occupied = state->_occupied.get(C) & ~willFree;
  uint32_t willSpill = 0;

  // Find the best registers for variables that are not allocated yet.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;
    uint32_t vaFlags = tied->flags;

    if ((vaFlags & TiedReg::kXReg) != 0) {
      if ((vaFlags & TiedReg::kXReg) == TiedReg::kWReg) {
        if (vaFlags & TiedReg::kWDone)
          continue;

        // Skip all registers that have assigned outPhysId. Spill if occupied.
        if (tied->hasOutPhysId()) {
          uint32_t outRegs = Utils::mask(tied->outPhysId);
          willSpill |= occupied & outRegs;
          continue;
        }
      }
      else {
        if (vaFlags & TiedReg::kRDone)
          continue;

        // We skip all registers that have assigned inPhysId, indicates that
        // the register to allocate in is known.
        if (tied->hasInPhysId()) {
          uint32_t inRegs = tied->inRegs;
          willSpill |= occupied & inRegs;
          continue;
        }
      }

      uint32_t m = tied->inRegs;
      if (tied->hasOutPhysId())
        m |= Utils::mask(tied->outPhysId);

      m = tied->allocableRegs & ~(willAlloc ^ m);
      m = guessAlloc<C>(vreg, m);
      ASMJIT_ASSERT(m != 0);

      uint32_t candidateRegs = m & ~occupied;
      uint32_t homeMask = vreg->getHomeMask();

      uint32_t physId;
      uint32_t regMask;

      if (candidateRegs == 0) {
        candidateRegs = m & occupied & ~state->_modified.get(C);
        if (candidateRegs == 0)
          candidateRegs = m;
      }
      if (candidateRegs & homeMask) candidateRegs &= homeMask;

      physId = Utils::findFirstBit(candidateRegs);
      regMask = Utils::mask(physId);

      if ((vaFlags & TiedReg::kXReg) == TiedReg::kWReg) {
        tied->setOutPhysId(physId);
      }
      else {
        tied->setInPhysId(physId);
        tied->inRegs = regMask;
      }

      willAlloc |= regMask;
      willSpill |= regMask & occupied;
      willFree  &=~regMask;
      occupied  |= regMask;

      continue;
    }
    else if ((vaFlags & TiedReg::kXMem) != 0) {
      uint32_t physId = vreg->getPhysId();
      if (physId != Globals::kInvalidRegId && (vaFlags & TiedReg::kXMem) != TiedReg::kWMem) {
        willSpill |= Utils::mask(physId);
      }
    }
  }

  // Set calculated masks back to the allocator; needed by spill() and alloc().
  _willSpill.set(C, willSpill);
  _willAlloc.set(C, willAlloc);
}

template<int C>
ASMJIT_INLINE void X86VarAlloc::spill() {
  uint32_t m = _willSpill.get(C);
  uint32_t i = static_cast<uint32_t>(0) - 1;
  if (m == 0) return;

  X86RAState* state = getState();
  VirtReg** vregs = state->getListByKind(C);

  // Available registers for decision if move has any benefit over spill.
  uint32_t availableRegs = getGaRegs(C) & ~(state->_occupied.get(C) | m | _willAlloc.get(C));

  do {
    // We always advance one more to destroy the bit that we have found.
    uint32_t bitIndex = Utils::findFirstBit(m) + 1;

    i += bitIndex;
    m >>= bitIndex;

    VirtReg* vreg = vregs[i];
    ASMJIT_ASSERT(vreg);

    TiedReg* tied = vreg->_tied;
    ASMJIT_ASSERT(!tied || (tied->flags & TiedReg::kXReg) == 0);

    if (vreg->isModified() && availableRegs) {
      // Don't check for alternatives if the variable has to be spilled.
      if (!tied || (tied->flags & TiedReg::kSpill) == 0) {
        uint32_t altRegs = guessSpill<C>(vreg, availableRegs);

        if (altRegs != 0) {
          uint32_t physId = Utils::findFirstBit(altRegs);
          uint32_t regMask = Utils::mask(physId);

          _context->move<C>(vreg, physId);
          availableRegs ^= regMask;
          continue;
        }
      }
    }

    _context->spill<C>(vreg);
  } while (m != 0);
}

template<int C>
ASMJIT_INLINE void X86VarAlloc::alloc() {
  if (isTiedDone(C)) return;

  uint32_t i;
  bool didWork;

  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  // Alloc `in` regs.
  do {
    didWork = false;
    for (i = 0; i < tiedCount; i++) {
      TiedReg* aTied = &tiedArray[i];
      VirtReg* aVReg = aTied->vreg;

      if ((aTied->flags & (TiedReg::kRReg | TiedReg::kRDone)) != TiedReg::kRReg)
        continue;

      uint32_t aPhysId = aVReg->getPhysId();
      uint32_t bPhysId = aTied->inPhysId;

      // Shouldn't be the same.
      ASMJIT_ASSERT(aPhysId != bPhysId);

      VirtReg* bVReg = getState()->getListByKind(C)[bPhysId];
      if (bVReg) {
        // Gp registers only - Swap two registers if we can solve two
        // allocation tasks by a single 'xchg' instruction, swapping
        // two registers required by the instruction/node or one register
        // required with another non-required.
        if (C == X86Reg::kKindGp && aPhysId != Globals::kInvalidRegId) {
          TiedReg* bTied = bVReg->_tied;
          _context->swapGp(aVReg, bVReg);

          aTied->flags |= TiedReg::kRDone;
          addTiedDone(C);

          // Double-hit, two registers allocated by a single xchg.
          if (bTied && bTied->inPhysId == aPhysId) {
            bTied->flags |= TiedReg::kRDone;
            addTiedDone(C);
          }

          didWork = true;
          continue;
        }
      }
      else if (aPhysId != Globals::kInvalidRegId) {
        _context->move<C>(aVReg, bPhysId);

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
      else {
        _context->alloc<C>(aVReg, bPhysId);

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
    }
  } while (didWork);

  // Alloc 'out' regs.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    if ((tied->flags & (TiedReg::kXReg | TiedReg::kWDone)) != TiedReg::kWReg)
      continue;

    uint32_t physId = tied->outPhysId;
    ASMJIT_ASSERT(physId != Globals::kInvalidRegId);

    if (vreg->getPhysId() != physId) {
      ASMJIT_ASSERT(getState()->getListByKind(C)[physId] == nullptr);
      _context->attach<C>(vreg, physId, false);
    }

    tied->flags |= TiedReg::kWDone;
    addTiedDone(C);
  }
}

// ============================================================================
// [asmjit::X86VarAlloc - GuessAlloc / GuessSpill]
// ============================================================================

template<int C>
ASMJIT_INLINE uint32_t X86VarAlloc::guessAlloc(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  // Stop now if there is only one bit (register) set in `allocableRegs` mask.
  if (Utils::isPowerOf2(allocableRegs)) return allocableRegs;

  uint32_t raId = vreg->_raId;
  uint32_t safeRegs = allocableRegs;

  uint32_t i;
  uint32_t maxLookAhead = kCompilerDefaultLookAhead;

  // Look ahead and calculate mask of special registers on both - input/output.
  CBNode* node = _node;
  for (i = 0; i < maxLookAhead; i++) {
    X86RAData* raData = node->getPassData<X86RAData>();
    RABits* liveness = raData ? raData->liveness : static_cast<RABits*>(nullptr);

    // If the variable becomes dead it doesn't make sense to continue.
    if (liveness && !liveness->getBit(raId)) break;

    // Stop on `CBSentinel` and `CCFuncRet`.
    if (node->hasFlag(CBNode::kFlagIsRet)) break;

    // Stop on conditional jump, we don't follow them.
    if (node->hasFlag(CBNode::kFlagIsJcc)) break;

    // Advance on non-conditional jump.
    if (node->hasFlag(CBNode::kFlagIsJmp)) {
      node = static_cast<CBJump*>(node)->getTarget();
      // Stop on jump that is not followed.
      if (!node) break;
    }

    node = node->getNext();
    ASMJIT_ASSERT(node != nullptr);

    raData = node->getPassData<X86RAData>();
    if (raData) {
      TiedReg* tied = raData->findTiedByKind(C, vreg);
      uint32_t mask;

      if (tied) {
        // If the variable is overwritten it doesn't make sense to continue.
        if ((tied->flags & TiedReg::kRAll) == 0)
          break;

        mask = tied->allocableRegs;
        if (mask != 0) {
          allocableRegs &= mask;
          if (allocableRegs == 0) break;
          safeRegs = allocableRegs;
        }

        mask = tied->inRegs;
        if (mask != 0) {
          allocableRegs &= mask;
          if (allocableRegs == 0) break;
          safeRegs = allocableRegs;
          break;
        }

        allocableRegs &= ~(raData->outRegs.get(C) | raData->clobberedRegs.get(C));
        if (allocableRegs == 0) break;
      }
      else {
        allocableRegs &= ~(raData->inRegs.get(C) | raData->outRegs.get(C) | raData->clobberedRegs.get(C));
        if (allocableRegs == 0) break;
      }

      safeRegs = allocableRegs;
    }
  }

  return safeRegs;
}

template<int C>
ASMJIT_INLINE uint32_t X86VarAlloc::guessSpill(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  return 0;
}

// ============================================================================
// [asmjit::X86VarAlloc - Modified]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86VarAlloc::modified() {
  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];

    if (tied->flags & TiedReg::kWReg) {
      VirtReg* vreg = tied->vreg;

      uint32_t physId = vreg->getPhysId();
      uint32_t regMask = Utils::mask(physId);

      vreg->setModified(true);
      _context->_x86State._modified.or_(C, regMask);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc]
// ============================================================================

//! \internal
//!
//! Register allocator context (function call).
struct X86CallAlloc : public X86BaseAlloc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86CallAlloc(X86RAPass* context) : X86BaseAlloc(context) {}
  ASMJIT_INLINE ~X86CallAlloc() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the node.
  ASMJIT_INLINE CCFuncCall* getNode() const { return static_cast<CCFuncCall*>(_node); }

  // --------------------------------------------------------------------------
  // [Run]
  // --------------------------------------------------------------------------

  Error run(CCFuncCall* node);

  // --------------------------------------------------------------------------
  // [Init / Cleanup]
  // --------------------------------------------------------------------------

protected:
  // Just to prevent calling these methods from X86RAPass::translate().
  ASMJIT_INLINE void init(CCFuncCall* node, X86RAData* raData);
  ASMJIT_INLINE void cleanup();

  // --------------------------------------------------------------------------
  // [Plan / Alloc / Spill / Move]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void plan();

  template<int C>
  ASMJIT_INLINE void spill();

  template<int C>
  ASMJIT_INLINE void alloc();

  // --------------------------------------------------------------------------
  // [AllocImmsOnStack]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void allocImmsOnStack();

  // --------------------------------------------------------------------------
  // [Duplicate]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void duplicate();

  // --------------------------------------------------------------------------
  // [GuessAlloc / GuessSpill]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE uint32_t guessAlloc(VirtReg* vreg, uint32_t allocableRegs);

  template<int C>
  ASMJIT_INLINE uint32_t guessSpill(VirtReg* vreg, uint32_t allocableRegs);

  // --------------------------------------------------------------------------
  // [Save]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void save();

  // --------------------------------------------------------------------------
  // [Clobber]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void clobber();

  // --------------------------------------------------------------------------
  // [Ret]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void ret();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Will alloc to these registers.
  X86RegMask _willAlloc;
  //! Will spill these registers.
  X86RegMask _willSpill;
};

// ============================================================================
// [asmjit::X86CallAlloc - Run]
// ============================================================================

Error X86CallAlloc::run(CCFuncCall* node) {
  // Initialize the allocator; prepare basics and connect Vd->Va.
  X86RAData* raData = node->getPassData<X86RAData>();
  init(node, raData);

  // Plan register allocation. Planner is only able to assign one register per
  // variable. If any variable is used multiple times it will be handled later.
  plan<X86Reg::kKindGp >();
  plan<X86Reg::kKindMm >();
  plan<X86Reg::kKindVec>();

  // Spill.
  spill<X86Reg::kKindGp >();
  spill<X86Reg::kKindMm >();
  spill<X86Reg::kKindVec>();

  // Alloc.
  alloc<X86Reg::kKindGp >();
  alloc<X86Reg::kKindMm >();
  alloc<X86Reg::kKindVec>();

  // Unuse clobbered registers that are not used to pass function arguments and
  // save variables used to pass function arguments that will be reused later on.
  save<X86Reg::kKindGp >();
  save<X86Reg::kKindMm >();
  save<X86Reg::kKindVec>();

  // Allocate immediates in registers and on the stack.
  allocImmsOnStack();

  // Duplicate.
  duplicate<X86Reg::kKindGp >();
  duplicate<X86Reg::kKindMm >();
  duplicate<X86Reg::kKindVec>();

  // Translate call operand.
  ASMJIT_PROPAGATE(X86RAPass_translateOperands(_context, node->getOpArray(), node->getOpCount()));

  // To emit instructions after call.
  _cc->_setCursor(node);

  // If the callee pops stack it has to be manually adjusted back.
  FuncDetail& fd = node->getDetail();
  if (fd.hasFlag(CallConv::kFlagCalleePopsStack) && fd.getArgStackSize() != 0)
    _cc->emit(X86Inst::kIdSub, _context->_zsp, static_cast<int>(fd.getArgStackSize()));

  // Clobber.
  clobber<X86Reg::kKindGp >();
  clobber<X86Reg::kKindMm >();
  clobber<X86Reg::kKindVec>();

  // Return.
  ret();

  // Unuse.
  unuseAfter<X86Reg::kKindGp >();
  unuseAfter<X86Reg::kKindMm >();
  unuseAfter<X86Reg::kKindVec>();

  // Cleanup; disconnect Vd->Va.
  cleanup();

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86CallAlloc - Init / Cleanup]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::init(CCFuncCall* node, X86RAData* raData) {
  X86BaseAlloc::init(node, raData);

  // Create mask of all registers that will be used to pass function arguments.
  _willAlloc.reset();
  _willAlloc.set(X86Reg::kKindGp , node->getDetail().getUsedRegs(X86Reg::kKindGp ));
  _willAlloc.set(X86Reg::kKindMm , node->getDetail().getUsedRegs(X86Reg::kKindMm ));
  _willAlloc.set(X86Reg::kKindK  , node->getDetail().getUsedRegs(X86Reg::kKindK  ));
  _willAlloc.set(X86Reg::kKindVec, node->getDetail().getUsedRegs(X86Reg::kKindVec));
  _willSpill.reset();
}

ASMJIT_INLINE void X86CallAlloc::cleanup() {
  X86BaseAlloc::cleanup();
}

// ============================================================================
// [asmjit::X86CallAlloc - Plan / Spill / Alloc]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::plan() {
  uint32_t i;
  uint32_t clobbered = _raData->clobberedRegs.get(C);

  uint32_t willAlloc = _willAlloc.get(C);
  uint32_t willFree = clobbered & ~willAlloc;

  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  X86RAState* state = getState();

  // Calculate 'willAlloc' and 'willFree' masks based on mandatory masks.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    uint32_t vaFlags = tied->flags;
    uint32_t physId = vreg->getPhysId();
    uint32_t regMask = (physId != Globals::kInvalidRegId) ? Utils::mask(physId) : 0;

    if ((vaFlags & TiedReg::kRReg) != 0) {
      // Planning register allocation. First check whether the variable is
      // already allocated in register and if it can stay there. Function
      // arguments are passed either in a specific register or in stack so
      // we care mostly of mandatory registers.
      uint32_t inRegs = tied->inRegs;

      if (inRegs == 0) {
        inRegs = tied->allocableRegs;
      }

      // Optimize situation where the variable has to be allocated in a
      // mandatory register, but it's already allocated in register that
      // is not clobbered (i.e. it will survive function call).
      if ((regMask & inRegs) != 0 || ((regMask & ~clobbered) != 0 && (vaFlags & TiedReg::kUnuse) == 0)) {
        tied->setInPhysId(physId);
        tied->flags |= TiedReg::kRDone;
        addTiedDone(C);
      }
      else {
        willFree |= regMask;
      }
    }
    else {
      // Memory access - if variable is allocated it has to be freed.
      if (regMask != 0) {
        willFree |= regMask;
      }
      else {
        tied->flags |= TiedReg::kRDone;
        addTiedDone(C);
      }
    }
  }

  // Occupied registers without 'willFree' registers; contains basically
  // all the registers we can use to allocate variables without inRegs
  // speficied.
  uint32_t occupied = state->_occupied.get(C) & ~willFree;
  uint32_t willSpill = 0;

  // Find the best registers for variables that are not allocated yet. Only
  // useful for Gp registers used as call operand.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    uint32_t vaFlags = tied->flags;
    if ((vaFlags & TiedReg::kRDone) != 0 || (vaFlags & TiedReg::kRReg) == 0)
      continue;

    // All registers except Gp used by call itself must have inPhysId.
    uint32_t m = tied->inRegs;
    if (C != X86Reg::kKindGp || m) {
      ASMJIT_ASSERT(m != 0);
      tied->setInPhysId(Utils::findFirstBit(m));
      willSpill |= occupied & m;
      continue;
    }

    m = tied->allocableRegs & ~(willAlloc ^ m);
    m = guessAlloc<C>(vreg, m);
    ASMJIT_ASSERT(m != 0);

    uint32_t candidateRegs = m & ~occupied;
    if (candidateRegs == 0) {
      candidateRegs = m & occupied & ~state->_modified.get(C);
      if (candidateRegs == 0)
        candidateRegs = m;
    }

    if (!(vaFlags & (TiedReg::kWReg | TiedReg::kUnuse)) && (candidateRegs & ~clobbered))
      candidateRegs &= ~clobbered;

    uint32_t physId = Utils::findFirstBit(candidateRegs);
    uint32_t regMask = Utils::mask(physId);

    tied->setInPhysId(physId);
    tied->inRegs = regMask;

    willAlloc |= regMask;
    willSpill |= regMask & occupied;
    willFree &= ~regMask;

    occupied |= regMask;
    continue;
  }

  // Set calculated masks back to the allocator; needed by spill() and alloc().
  _willSpill.set(C, willSpill);
  _willAlloc.set(C, willAlloc);
}

template<int C>
ASMJIT_INLINE void X86CallAlloc::spill() {
  uint32_t m = _willSpill.get(C);
  uint32_t i = static_cast<uint32_t>(0) - 1;

  if (m == 0)
    return;

  X86RAState* state = getState();
  VirtReg** sVars = state->getListByKind(C);

  // Available registers for decision if move has any benefit over spill.
  uint32_t availableRegs = getGaRegs(C) & ~(state->_occupied.get(C) | m | _willAlloc.get(C));

  do {
    // We always advance one more to destroy the bit that we have found.
    uint32_t bitIndex = Utils::findFirstBit(m) + 1;

    i += bitIndex;
    m >>= bitIndex;

    VirtReg* vreg = sVars[i];
    ASMJIT_ASSERT(vreg && !vreg->_tied);

    if (vreg->isModified() && availableRegs) {
      uint32_t available = guessSpill<C>(vreg, availableRegs);
      if (available != 0) {
        uint32_t physId = Utils::findFirstBit(available);
        uint32_t regMask = Utils::mask(physId);

        _context->move<C>(vreg, physId);
        availableRegs ^= regMask;
        continue;
      }
    }

    _context->spill<C>(vreg);
  } while (m != 0);
}

template<int C>
ASMJIT_INLINE void X86CallAlloc::alloc() {
  if (isTiedDone(C)) return;

  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  uint32_t i;
  bool didWork;

  do {
    didWork = false;
    for (i = 0; i < tiedCount; i++) {
      TiedReg* aTied = &tiedArray[i];
      VirtReg* aVReg = aTied->vreg;
      if ((aTied->flags & (TiedReg::kRReg | TiedReg::kRDone)) != TiedReg::kRReg) continue;

      uint32_t sPhysId = aVReg->getPhysId();
      uint32_t bPhysId = aTied->inPhysId;

      // Shouldn't be the same.
      ASMJIT_ASSERT(sPhysId != bPhysId);

      VirtReg* bVReg = getState()->getListByKind(C)[bPhysId];
      if (bVReg) {
        TiedReg* bTied = bVReg->_tied;

        // GP registers only - Swap two registers if we can solve two
        // allocation tasks by a single 'xchg' instruction, swapping
        // two registers required by the instruction/node or one register
        // required with another non-required.
        if (C == X86Reg::kKindGp) {
          _context->swapGp(aVReg, bVReg);

          aTied->flags |= TiedReg::kRDone;
          addTiedDone(C);

          // Double-hit, two registers allocated by a single swap.
          if (bTied && bTied->inPhysId == sPhysId) {
            bTied->flags |= TiedReg::kRDone;
            addTiedDone(C);
          }

          didWork = true;
          continue;
        }
      }
      else if (sPhysId != Globals::kInvalidRegId) {
        _context->move<C>(aVReg, bPhysId);
        _context->_clobberedRegs.or_(C, Utils::mask(bPhysId));

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
      else {
        _context->alloc<C>(aVReg, bPhysId);
        _context->_clobberedRegs.or_(C, Utils::mask(bPhysId));

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
    }
  } while (didWork);
}

// ============================================================================
// [asmjit::X86CallAlloc - AllocImmsOnStack]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::allocImmsOnStack() {
  CCFuncCall* node = getNode();
  FuncDetail& fd = node->getDetail();

  uint32_t argCount = fd.getArgCount();
  Operand_* args = node->_args;

  for (uint32_t i = 0; i < argCount; i++) {
    Operand_& op = args[i];
    if (!op.isImm()) continue;

    const Imm& imm = static_cast<const Imm&>(op);
    const FuncDetail::Value& arg = fd.getArg(i);
    uint32_t varType = arg.getTypeId();

    if (arg.byReg()) {
      _context->emitImmToReg(varType, arg.getRegId(), &imm);
    }
    else {
      X86Mem dst = x86::ptr(_context->_zsp, -static_cast<int>(_context->getGpSize()) + arg.getStackOffset());
      _context->emitImmToStack(varType, &dst, &imm);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Duplicate]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::duplicate() {
  TiedReg* tiedArray = getTiedArrayByKind(C);
  uint32_t tiedCount = getTiedCountByKind(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    if ((tied->flags & TiedReg::kRReg) == 0) continue;

    uint32_t inRegs = tied->inRegs;
    if (!inRegs) continue;

    VirtReg* vreg = tied->vreg;
    uint32_t physId = vreg->getPhysId();

    ASMJIT_ASSERT(physId != Globals::kInvalidRegId);

    inRegs &= ~Utils::mask(physId);
    if (!inRegs) continue;

    for (uint32_t dupIndex = 0; inRegs != 0; dupIndex++, inRegs >>= 1) {
      if (inRegs & 0x1) {
        _context->emitMove(vreg, dupIndex, physId, "Duplicate");
        _context->_clobberedRegs.or_(C, Utils::mask(dupIndex));
      }
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - GuessAlloc / GuessSpill]
// ============================================================================

template<int C>
ASMJIT_INLINE uint32_t X86CallAlloc::guessAlloc(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  // Stop now if there is only one bit (register) set in 'allocableRegs' mask.
  if (Utils::isPowerOf2(allocableRegs))
    return allocableRegs;

  uint32_t i;
  uint32_t safeRegs = allocableRegs;
  uint32_t maxLookAhead = kCompilerDefaultLookAhead;

  // Look ahead and calculate mask of special registers on both - input/output.
  CBNode* node = _node;
  for (i = 0; i < maxLookAhead; i++) {
    // Stop on `CCFuncRet` and `CBSentinel`.
    if (node->hasFlag(CBNode::kFlagIsRet))
      break;

    // Stop on conditional jump, we don't follow them.
    if (node->hasFlag(CBNode::kFlagIsJcc))
      break;

    // Advance on non-conditional jump.
    if (node->hasFlag(CBNode::kFlagIsJmp)) {
      node = static_cast<CBJump*>(node)->getTarget();
      // Stop on jump that is not followed.
      if (!node) break;
    }

    node = node->getNext();
    ASMJIT_ASSERT(node != nullptr);

    X86RAData* raData = node->getPassData<X86RAData>();
    if (raData) {
      TiedReg* tied = raData->findTiedByKind(C, vreg);
      if (tied) {
        uint32_t inRegs = tied->inRegs;
        if (inRegs != 0) {
          safeRegs = allocableRegs;
          allocableRegs &= inRegs;

          if (allocableRegs == 0)
            goto _UseSafeRegs;
          else
            return allocableRegs;
        }
      }

      safeRegs = allocableRegs;
      allocableRegs &= ~(raData->inRegs.get(C) | raData->outRegs.get(C) | raData->clobberedRegs.get(C));

      if (allocableRegs == 0)
        break;
    }
  }

_UseSafeRegs:
  return safeRegs;
}

template<int C>
ASMJIT_INLINE uint32_t X86CallAlloc::guessSpill(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);
  return 0;
}

// ============================================================================
// [asmjit::X86CallAlloc - Save]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::save() {
  X86RAState* state = getState();
  VirtReg** sVars = state->getListByKind(C);

  uint32_t i;
  uint32_t affected = _raData->clobberedRegs.get(C) & state->_occupied.get(C) & state->_modified.get(C);

  for (i = 0; affected != 0; i++, affected >>= 1) {
    if (affected & 0x1) {
      VirtReg* vreg = sVars[i];
      ASMJIT_ASSERT(vreg != nullptr);
      ASMJIT_ASSERT(vreg->isModified());

      TiedReg* tied = vreg->_tied;
      if (!tied || (tied->flags & (TiedReg::kWReg | TiedReg::kUnuse)) == 0)
        _context->save<C>(vreg);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Clobber]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::clobber() {
  X86RAState* state = getState();
  VirtReg** sVars = state->getListByKind(C);

  uint32_t i;
  uint32_t affected = _raData->clobberedRegs.get(C) & state->_occupied.get(C);

  for (i = 0; affected != 0; i++, affected >>= 1) {
    if (affected & 0x1) {
      VirtReg* vreg = sVars[i];
      ASMJIT_ASSERT(vreg != nullptr);

      TiedReg* tied = vreg->_tied;
      uint32_t vdState = VirtReg::kStateNone;

      if (!vreg->isModified() || (tied && (tied->flags & (TiedReg::kWAll | TiedReg::kUnuse)) != 0))
        vdState = VirtReg::kStateMem;
      _context->unuse<C>(vreg, vdState);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Ret]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::ret() {
  CCFuncCall* node = getNode();
  FuncDetail& fd = node->getDetail();
  Operand_* rets = node->_ret;

  for (uint32_t i = 0; i < 2; i++) {
    const FuncDetail::Value& ret = fd.getRet(i);
    Operand_* op = &rets[i];

    if (!ret.byReg() || !op->isVirtReg())
      continue;

    VirtReg* vreg = _cc->getVirtRegById(op->getId());
    uint32_t regId = ret.getRegId();

    switch (vreg->getKind()) {
      case X86Reg::kKindGp:
        _context->unuse<X86Reg::kKindGp>(vreg);
        _context->attach<X86Reg::kKindGp>(vreg, regId, true);
        break;

      case X86Reg::kKindMm:
        _context->unuse<X86Reg::kKindMm>(vreg);
        _context->attach<X86Reg::kKindMm>(vreg, regId, true);
        break;

      case X86Reg::kKindVec:
        if (X86Reg::kindOf(ret.getRegType()) == X86Reg::kKindVec) {
          _context->unuse<X86Reg::kKindVec>(vreg);
          _context->attach<X86Reg::kKindVec>(vreg, regId, true);
        }
        else {
          uint32_t elementId = TypeId::elementOf(vreg->getTypeId());
          uint32_t size = (elementId == TypeId::kF32) ? 4 : 8;

          X86Mem m = _context->getVarMem(vreg);
          m.setSize(size);

          _context->unuse<X86Reg::kKindVec>(vreg, VirtReg::kStateMem);
          _cc->fstp(m);
        }
        break;
    }
  }
}

// ============================================================================
// [asmjit::X86RAPass - TranslateOperands]
// ============================================================================

//! \internal
static Error X86RAPass_translateOperands(X86RAPass* self, Operand_* opArray, uint32_t opCount) {
  X86Compiler* cc = self->cc();

  // Translate variables into registers.
  for (uint32_t i = 0; i < opCount; i++) {
    Operand_* op = &opArray[i];
    if (op->isVirtReg()) {
      VirtReg* vreg = cc->getVirtRegById(op->getId());
      ASMJIT_ASSERT(vreg != nullptr);
      ASMJIT_ASSERT(vreg->getPhysId() != Globals::kInvalidRegId);
      op->_reg.id = vreg->getPhysId();
    }
    else if (op->isMem()) {
      X86Mem* m = static_cast<X86Mem*>(op);

      if (m->hasBaseReg() && cc->isVirtRegValid(m->getBaseId())) {
        VirtReg* vreg = cc->getVirtRegById(m->getBaseId());

        if (m->isRegHome()) {
          self->getVarCell(vreg);
        }
        else {
          ASMJIT_ASSERT(vreg->getPhysId() != Globals::kInvalidRegId);
          op->_mem.base = vreg->getPhysId();
        }
      }

      if (m->hasIndexReg() && cc->isVirtRegValid(m->getIndexId())) {
        VirtReg* vreg = cc->getVirtRegById(m->getIndexId());
        op->_mem.index = vreg->getPhysId();
      }
    }
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - TranslatePrologEpilog]
// ============================================================================

//! \internal
static Error X86RAPass_prepareFuncFrame(X86RAPass* self, CCFunc* func) {
  FuncFrameInfo& ffi = func->getFrameInfo();

  X86RegMask& clobberedRegs = self->_clobberedRegs;

  // Initialize dirty registers.
  ffi.setDirtyRegs(X86Reg::kKindGp , clobberedRegs.get(X86Reg::kKindGp ));
  ffi.setDirtyRegs(X86Reg::kKindMm , clobberedRegs.get(X86Reg::kKindMm ));
  ffi.setDirtyRegs(X86Reg::kKindK  , clobberedRegs.get(X86Reg::kKindK  ));
  ffi.setDirtyRegs(X86Reg::kKindVec, clobberedRegs.get(X86Reg::kKindVec));

  // Initialize stack size & alignment.
  ffi.setStackFrameSize(self->_memAllTotal);
  ffi.setStackFrameAlignment(self->_memMaxAlign);

  return kErrorOk;
}

//! \internal
static Error X86RAPass_patchFuncMem(X86RAPass* self, CCFunc* func, CBNode* stop, FuncFrameLayout& layout) {
  X86Compiler* cc = self->cc();
  CBNode* node = func;

  do {
    if (node->getType() == CBNode::kNodeInst) {
      CBInst* iNode = static_cast<CBInst*>(node);

      if (iNode->hasMemOp()) {
        X86Mem* m = iNode->getMemOp<X86Mem>();

        if (m->isArgHome()) {
          m->addOffsetLo32(layout.getStackArgsOffset());
          m->clearArgHome();
        }

        if (m->isRegHome() && Operand::isPackedId(m->getBaseId())) {
          VirtReg* vreg = cc->getVirtRegById(m->getBaseId());
          ASMJIT_ASSERT(vreg != nullptr);

          RACell* cell = vreg->getMemCell();
          ASMJIT_ASSERT(cell != nullptr);

          m->_setBase(cc->_nativeGpReg.getType(), self->_varBaseRegId);
          m->addOffsetLo32(self->_varBaseOffset + cell->offset);
          m->clearRegHome();
        }
      }
    }

    node = node->getNext();
  } while (node != stop);

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Jump]
// ============================================================================

//! \internal
static void X86RAPass_translateJump(X86RAPass* self, CBJump* jNode, CBLabel* jTarget) {
  X86Compiler* cc = self->cc();

  CBNode* injectRef = self->getFunc()->getEnd()->getPrev();
  CBNode* prevCursor = cc->setCursor(injectRef);

  self->switchState(jTarget->getPassData<RAData>()->state);

  // Any code necessary to `switchState()` will be added at the end of the function.
  if (cc->getCursor() != injectRef) {
    // TODO: Can fail.
    CBLabel* injectLabel = cc->newLabelNode();

    // Add the jump to the target.
    cc->jmp(jTarget->getLabel());

    // Inject the label.
    cc->_setCursor(injectRef);
    cc->addNode(injectLabel);

    // Finally, patch `jNode` target.
    ASMJIT_ASSERT(jNode->getOpCount() > 0);
    jNode->_opArray[jNode->getOpCount() - 1] = injectLabel->getLabel();
    jNode->_target = injectLabel;
    // If we injected any code it may not satisfy short form anymore.
    jNode->delOptions(X86Inst::kOptionShortForm);
  }

  cc->_setCursor(prevCursor);
  self->loadState(jNode->getPassData<RAData>()->state);
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Ret]
// ============================================================================

static Error X86RAPass_translateRet(X86RAPass* self, CCFuncRet* rNode, CBLabel* exitTarget) {
  X86Compiler* cc = self->cc();
  CBNode* node = rNode->getNext();

  // 32-bit mode requires to push floating point return value(s), handle it
  // here as it's a special case.
  X86RAData* raData = rNode->getPassData<X86RAData>();
  if (raData) {
    TiedReg* tiedArray = raData->tiedArray;
    uint32_t tiedTotal = raData->tiedTotal;

    for (uint32_t i = 0; i < tiedTotal; i++) {
      TiedReg* tied = &tiedArray[i];
      if (tied->flags & (TiedReg::kX86Fld4 | TiedReg::kX86Fld8)) {
        VirtReg* vreg = tied->vreg;
        X86Mem m(self->getVarMem(vreg));

        uint32_t elementId = TypeId::elementOf(vreg->getTypeId());
        m.setSize(elementId == TypeId::kF32 ? 4 :
                  elementId == TypeId::kF64 ? 8 :
                  (tied->flags & TiedReg::kX86Fld4) ? 4 : 8);

        cc->fld(m);
      }
    }
  }

  // Decide whether to `jmp` or not in case we are next to the return label.
  while (node) {
    switch (node->getType()) {
      // If we have found an exit label we just return, there is no need to
      // emit jump to that.
      case CBNode::kNodeLabel:
        if (static_cast<CBLabel*>(node) == exitTarget)
          return kErrorOk;
        goto _EmitRet;

      case CBNode::kNodeData:
      case CBNode::kNodeInst:
      case CBNode::kNodeFuncCall:
      case CBNode::kNodeFuncExit:
        goto _EmitRet;

      // Continue iterating.
      case CBNode::kNodeComment:
      case CBNode::kNodeAlign:
      case CBNode::kNodeHint:
        break;

      // Invalid node to be here.
      case CBNode::kNodeFunc:
        return DebugUtils::errored(kErrorInvalidState);

      // We can't go forward from here.
      case CBNode::kNodeSentinel:
        return kErrorOk;
    }

    node = node->getNext();
  }

_EmitRet:
  {
    cc->_setCursor(rNode);
    cc->jmp(exitTarget->getLabel());
  }
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Func]
// ============================================================================

Error X86RAPass::translate() {
  X86Compiler* cc = this->cc();
  CCFunc* func = getFunc();

  // Register allocator contexts.
  X86VarAlloc vAlloc(this);
  X86CallAlloc cAlloc(this);

  // Flow.
  CBNode* node_ = func;
  CBNode* next = nullptr;
  CBNode* stop = getStop();

  ZoneList<CBNode*>::Link* jLink = _jccList.getFirst();

  for (;;) {
    while (node_->isTranslated()) {
      // Switch state if we went to a node that is already translated.
      if (node_->getType() == CBNode::kNodeLabel) {
        CBLabel* node = static_cast<CBLabel*>(node_);
        cc->_setCursor(node->getPrev());
        switchState(node->getPassData<RAData>()->state);
      }

_NextGroup:
      if (!jLink) {
        goto _Done;
      }
      else {
        node_ = jLink->getValue();
        jLink = jLink->getNext();

        CBNode* jFlow = X86RAPass_getOppositeJccFlow(static_cast<CBJump*>(node_));
        loadState(node_->getPassData<RAData>()->state);

        if (jFlow->hasPassData() && jFlow->getPassData<RAData>()->state) {
          X86RAPass_translateJump(this, static_cast<CBJump*>(node_), static_cast<CBLabel*>(jFlow));

          node_ = jFlow;
          if (node_->isTranslated())
            goto _NextGroup;
        }
        else {
          node_ = jFlow;
        }

        break;
      }
    }

    next = node_->getNext();
    node_->_flags |= CBNode::kFlagIsTranslated;

    if (node_->hasPassData()) {
      switch (node_->getType()) {
        // --------------------------------------------------------------------
        // [Align / Embed]
        // --------------------------------------------------------------------

        case CBNode::kNodeAlign:
        case CBNode::kNodeData:
          break;

        // --------------------------------------------------------------------
        // [Label]
        // --------------------------------------------------------------------

        case CBNode::kNodeLabel: {
          CBLabel* node = static_cast<CBLabel*>(node_);
          ASMJIT_ASSERT(node->getPassData<RAData>()->state == nullptr);
          node->getPassData<RAData>()->state = saveState();

          if (node == func->getExitNode())
            goto _NextGroup;
          break;
        }

        // --------------------------------------------------------------------
        // [Inst/Call/SArg/Ret]
        // --------------------------------------------------------------------

        case CBNode::kNodeInst:
        case CBNode::kNodeFunc:
        case CBNode::kNodeFuncCall:
        case CBNode::kNodePushArg:
          // Update TiedReg's unuse flags based on liveness of the next node.
          if (!node_->isJcc()) {
            X86RAData* raData = node_->getPassData<X86RAData>();
            RABits* liveness;

            if (raData && next && next->hasPassData() && (liveness = next->getPassData<RAData>()->liveness)) {
              TiedReg* tiedArray = raData->tiedArray;
              uint32_t tiedTotal = raData->tiedTotal;

              for (uint32_t i = 0; i < tiedTotal; i++) {
                TiedReg* tied = &tiedArray[i];
                VirtReg* vreg = tied->vreg;

                if (!liveness->getBit(vreg->_raId) && !vreg->isFixed())
                  tied->flags |= TiedReg::kUnuse;
              }
            }
          }

          if (node_->getType() == CBNode::kNodeFuncCall) {
            ASMJIT_PROPAGATE(cAlloc.run(static_cast<CCFuncCall*>(node_)));
            break;
          }
          ASMJIT_FALLTHROUGH;

        case CBNode::kNodeHint:
        case CBNode::kNodeFuncExit: {
          ASMJIT_PROPAGATE(vAlloc.run(node_));

          // Handle conditional/unconditional jump.
          if (node_->isJmpOrJcc()) {
            CBJump* node = static_cast<CBJump*>(node_);
            CBLabel* jTarget = node->getTarget();

            // Target not followed.
            if (!jTarget) {
              if (node->isJmp())
                goto _NextGroup;
              else
                break;
            }

            if (node->isJmp()) {
              if (jTarget->hasPassData() && jTarget->getPassData<RAData>()->state) {
                cc->_setCursor(node->getPrev());
                switchState(jTarget->getPassData<RAData>()->state);

                goto _NextGroup;
              }
              else {
                next = jTarget;
              }
            }
            else {
              CBNode* jNext = node->getNext();

              if (jTarget->isTranslated()) {
                if (jNext->isTranslated()) {
                  ASMJIT_ASSERT(jNext->getType() == CBNode::kNodeLabel);
                  cc->_setCursor(node->getPrev());
                  intersectStates(
                    jTarget->getPassData<RAData>()->state,
                    jNext->getPassData<RAData>()->state);
                }

                RAState* savedState = saveState();
                node->getPassData<RAData>()->state = savedState;

                X86RAPass_translateJump(this, node, jTarget);
                next = jNext;
              }
              else if (jNext->isTranslated()) {
                ASMJIT_ASSERT(jNext->getType() == CBNode::kNodeLabel);

                RAState* savedState = saveState();
                node->getPassData<RAData>()->state = savedState;

                cc->_setCursor(node);
                switchState(jNext->getPassData<RAData>()->state);
                next = jTarget;
              }
              else {
                node->getPassData<RAData>()->state = saveState();
                next = X86RAPass_getJccFlow(node);
              }
            }
          }
          else if (node_->isRet()) {
            ASMJIT_PROPAGATE(
              X86RAPass_translateRet(this, static_cast<CCFuncRet*>(node_), func->getExitNode()));
            goto _NextGroup;
          }
          break;
        }

        // --------------------------------------------------------------------
        // [End]
        // --------------------------------------------------------------------

        case CBNode::kNodeSentinel: {
          goto _NextGroup;
        }

        default:
          break;
      }
    }

    if (next == stop)
      goto _NextGroup;
    node_ = next;
  }

_Done:
  {
    ASMJIT_PROPAGATE(resolveCellOffsets());
    ASMJIT_PROPAGATE(X86RAPass_prepareFuncFrame(this, func));

    FuncFrameLayout layout;
    ASMJIT_PROPAGATE(layout.init(func->getDetail(), func->getFrameInfo()));

    _varBaseRegId = layout._stackBaseRegId;
    _varBaseOffset = layout._stackBaseOffset;

    ASMJIT_PROPAGATE(X86RAPass_patchFuncMem(this, func, stop, layout));

    cc->_setCursor(func);
    ASMJIT_PROPAGATE(FuncUtils::emitProlog(this->cc(), layout));

    cc->_setCursor(func->getExitNode());
    ASMJIT_PROPAGATE(FuncUtils::emitEpilog(this->cc(), layout));
  }

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 && !ASMJIT_DISABLE_COMPILER
