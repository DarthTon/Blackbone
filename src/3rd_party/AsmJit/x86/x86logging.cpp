// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_LOGGING)

// [Dependencies]
#include "../base/misc_p.h"
#include "../x86/x86inst.h"
#include "../x86/x86logging_p.h"
#include "../x86/x86operand.h"

#if !defined(ASMJIT_DISABLE_COMPILER)
#include "../base/codecompiler.h"
#endif // !ASMJIT_DISABLE_COMPILER

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Logging - Constants]
// ============================================================================

struct X86RegFormatInfo {
  uint8_t count;
  uint8_t formatIndex;
  uint8_t specialIndex;
  uint8_t specialCount;
};

static const char x86RegFormatStrings[] =
  "r%ub"  "\0" // #0
  "r%uh"  "\0" // #5
  "r%uw"  "\0" // #10
  "r%ud"  "\0" // #15
  "r%u"   "\0" // #20
  "xmm%u" "\0" // #24
  "ymm%u" "\0" // #30
  "zmm%u" "\0" // #36
  "rip%u" "\0" // #42
  "seg%u" "\0" // #48
  "fp%u"  "\0" // #54
  "mm%u"  "\0" // #59
  "k%u"   "\0" // #64
  "bnd%u" "\0" // #68
  "cr%u"  "\0" // #74
  "dr%u"  "\0" // #79

  "rip\0"      // #84
  "\0\0\0\0"   // #88
  "\0\0\0\0"   // #92

  "al\0\0" "cl\0\0" "dl\0\0" "bl\0\0" "spl\0"  "bpl\0"  "sil\0"  "dil\0"  // #96
  "ah\0\0" "ch\0\0" "dh\0\0" "bh\0\0" "n/a\0"  "n/a\0"  "n/a\0"  "n/a\0"  // #128
  "eax\0"  "ecx\0"  "edx\0"  "ebx\0"  "esp\0"  "ebp\0"  "esi\0"  "edi\0"  // #160
  "rax\0"  "rcx\0"  "rdx\0"  "rbx\0"  "rsp\0"  "rbp\0"  "rsi\0"  "rdi\0"  // #192
  "n/a\0"  "es\0\0" "cs\0\0" "ss\0\0" "ds\0\0" "fs\0\0" "gs\0\0" "n/a\0"; // #224

template<uint32_t X>
struct X86RegFormatInfo_T {
  enum {
    kFormatIndex  = X == X86Reg::kRegGpbLo ? 0   :
                    X == X86Reg::kRegGpbHi ? 5   :
                    X == X86Reg::kRegGpw   ? 10  :
                    X == X86Reg::kRegGpd   ? 15  :
                    X == X86Reg::kRegGpq   ? 20  :
                    X == X86Reg::kRegXmm   ? 24  :
                    X == X86Reg::kRegYmm   ? 30  :
                    X == X86Reg::kRegZmm   ? 36  :
                    X == X86Reg::kRegRip   ? 42  :
                    X == X86Reg::kRegSeg   ? 48  :
                    X == X86Reg::kRegFp    ? 54  :
                    X == X86Reg::kRegMm    ? 59  :
                    X == X86Reg::kRegK     ? 64  :
                    X == X86Reg::kRegBnd   ? 68  :
                    X == X86Reg::kRegCr    ? 74  :
                    X == X86Reg::kRegDr    ? 79  : 0,

    kSpecialIndex = X == X86Reg::kRegGpbLo ? 96  :
                    X == X86Reg::kRegGpbHi ? 128 :
                    X == X86Reg::kRegGpw   ? 161 :
                    X == X86Reg::kRegGpd   ? 160 :
                    X == X86Reg::kRegGpq   ? 192 :
                    X == X86Reg::kRegRip   ? 84  :
                    X == X86Reg::kRegSeg   ? 224 : 0,

    kSpecialCount = X == X86Reg::kRegGpbLo ? 8   :
                    X == X86Reg::kRegGpbHi ? 4   :
                    X == X86Reg::kRegGpw   ? 8   :
                    X == X86Reg::kRegGpd   ? 8   :
                    X == X86Reg::kRegGpq   ? 8   :
                    X == X86Reg::kRegRip   ? 1   :
                    X == X86Reg::kRegSeg   ? 7   : 0
  };
};

#define ASMJIT_X86_REG_FORMAT(TYPE) {      \
  X86RegTraits<TYPE>::kCount,              \
  X86RegFormatInfo_T<TYPE>::kFormatIndex,  \
  X86RegFormatInfo_T<TYPE>::kSpecialIndex, \
  X86RegFormatInfo_T<TYPE>::kSpecialCount  \
}

static const X86RegFormatInfo x86RegFormatInfo[] = {
  ASMJIT_TABLE_16(ASMJIT_X86_REG_FORMAT, 0 ),
  ASMJIT_TABLE_16(ASMJIT_X86_REG_FORMAT, 16)
};

static const char* x86GetAddressSizeString(uint32_t size) noexcept {
  switch (size) {
    case 1 : return "byte ";
    case 2 : return "word ";
    case 4 : return "dword ";
    case 6 : return "fword ";
    case 8 : return "qword ";
    case 10: return "tword ";
    case 16: return "oword ";
    case 32: return "yword ";
    case 64: return "zword ";
    default: return "";
  }
}

// ============================================================================
// [asmjit::X86Logging - Format Operand]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Logging::formatOperand(
  StringBuilder& sb,
  uint32_t logOptions,
  const CodeEmitter* emitter,
  uint32_t archType,
  const Operand_& op) noexcept {

  if (op.isReg())
    return formatRegister(sb, logOptions, emitter, archType, op.as<Reg>().getType(), op.as<Reg>().getId());

  if (op.isMem()) {
    const X86Mem& m = op.as<X86Mem>();
    ASMJIT_PROPAGATE(sb.appendString(x86GetAddressSizeString(m.getSize())));

    // Segment override prefix.
    uint32_t seg = m.getSegmentId();
    if (seg != X86Seg::kIdNone && seg < X86Seg::kIdCount)
      ASMJIT_PROPAGATE(sb.appendFormat("%s:", x86RegFormatStrings + 224 + seg * 4));

    ASMJIT_PROPAGATE(sb.appendChar('['));
    if (m.isAbs())
      ASMJIT_PROPAGATE(sb.appendString("abs "));

    if (m.hasBase()) {
      if (m.hasBaseLabel()) {
        ASMJIT_PROPAGATE(Logging::formatLabel(sb, logOptions, emitter, m.getBaseId()));
      }
      else {
        if (m.isArgHome()) ASMJIT_PROPAGATE(sb.appendString("$"));
        if (m.isRegHome()) ASMJIT_PROPAGATE(sb.appendString("&"));
        ASMJIT_PROPAGATE(formatRegister(sb, logOptions, emitter, archType, m.getBaseType(), m.getBaseId()));
      }
    }

    if (m.hasIndex()) {
      ASMJIT_PROPAGATE(sb.appendChar('+'));
      ASMJIT_PROPAGATE(formatRegister(sb, logOptions, emitter, archType, m.getIndexType(), m.getIndexId()));
      if (m.hasShift())
        ASMJIT_PROPAGATE(sb.appendFormat("*%u", 1 << m.getShift()));
    }

    uint64_t off = static_cast<uint64_t>(m.getOffset());
    if (off) {
      uint32_t base = 10;
      char prefix = '+';

      if (static_cast<int64_t>(off) < 0) {
        off = ~off + 1;
        prefix = '-';
      }

      ASMJIT_PROPAGATE(sb.appendChar(prefix));
      if ((logOptions & Logger::kOptionHexDisplacement) != 0 && off > 9) {
        ASMJIT_PROPAGATE(sb.appendString("0x", 2));
        base = 16;
      }
      ASMJIT_PROPAGATE(sb.appendUInt(off, base));
    }

    return sb.appendChar(']');
  }

  if (op.isImm()) {
    const Imm& i = op.as<Imm>();
    int64_t val = i.getInt64();

    if ((logOptions & Logger::kOptionHexImmediate) != 0 && static_cast<uint64_t>(val) > 9)
      return sb.appendUInt(static_cast<uint64_t>(val), 16);
    else
      return sb.appendInt(val, 10);
  }

  if (op.isLabel()) {
    return Logging::formatLabel(sb, logOptions, emitter, op.getId());
  }

  return sb.appendString("<None>");
}

// ============================================================================
// [asmjit::X86Logging - Format Immediate (Extension)]
// ============================================================================

struct ImmBits {
  enum Mode {
    kModeLookup = 0x0,
    kModeFormat = 0x1
  };

  uint8_t mask;
  uint8_t shift;
  uint8_t mode;
  char text[45];
};

ASMJIT_FAVOR_SIZE static Error X86Logging_formatImmShuf(StringBuilder& sb, uint32_t u8, uint32_t bits, uint32_t count) noexcept {
  ASMJIT_PROPAGATE(sb.appendChar('<'));
  uint32_t mask = (1 << bits) - 1;

  for (uint32_t i = 0; i < count; i++, u8 >>= bits) {
    uint32_t value = u8 & mask;
    if (i != 0)
      ASMJIT_PROPAGATE(sb.appendChar('|'));
    ASMJIT_PROPAGATE(sb.appendUInt(value));
  }

  return sb.appendChar('>');
}

ASMJIT_FAVOR_SIZE static Error X86Logging_formatImmBits(StringBuilder& sb, uint32_t u8, const ImmBits* bits, uint32_t count) noexcept {
  uint32_t n = 0;
  char buf[64];

  for (uint32_t i = 0; i < count; i++) {
    const ImmBits& spec = bits[i];

    uint32_t value = (u8 & static_cast<uint32_t>(spec.mask)) >> spec.shift;
    const char* str = nullptr;

    switch (spec.mode) {
      case ImmBits::kModeLookup:
        str = Utils::findPackedString(spec.text, value);
        break;

      case ImmBits::kModeFormat:
        snprintf(buf, sizeof(buf), spec.text, static_cast<unsigned int>(value));
        str = buf;
        break;

      default:
        return DebugUtils::errored(kErrorInvalidState);
    }

    if (!str[0])
      continue;

    ASMJIT_PROPAGATE(sb.appendChar(++n == 1 ? '<' : '|'));
    ASMJIT_PROPAGATE(sb.appendString(str));
  }

  return n ? sb.appendChar('>') : static_cast<Error>(kErrorOk);
}

ASMJIT_FAVOR_SIZE static Error X86Logging_formatImmText(StringBuilder& sb, uint32_t u8, uint32_t bits, uint32_t advance, const char* text, uint32_t count = 1) noexcept {
  ASMJIT_PROPAGATE(sb.appendChar('<'));

  uint32_t mask = (1 << bits) - 1;
  uint32_t pos = 0;

  for (uint32_t i = 0; i < count; i++, u8 >>= bits, pos += advance) {
    uint32_t value = (u8 & mask) + pos;
    if (i != 0)
      ASMJIT_PROPAGATE(sb.appendChar('|'));
    ASMJIT_PROPAGATE(sb.appendString(Utils::findPackedString(text, value)));
  }

  return sb.appendChar('>');
}

ASMJIT_FAVOR_SIZE static Error X86Logging_formatImmExtended(
  StringBuilder& sb,
  uint32_t logOptions,
  uint32_t instId,
  uint32_t vecSize,
  const Imm& imm) noexcept {

  static const char vcmpx[] =
    "eq_oq\0" "lt_os\0"  "le_os\0"  "unord_q\0"  "neq_uq\0" "nlt_us\0" "nle_us\0" "ord_q\0"
    "eq_uq\0" "nge_us\0" "ngt_us\0" "false_oq\0" "neq_oq\0" "ge_os\0"  "gt_os\0"  "true_uq\0"
    "eq_os\0" "lt_oq\0"  "le_oq\0"  "unord_s\0"  "neq_us\0" "nlt_uq\0" "nle_uq\0" "ord_s\0"
    "eq_us\0" "nge_uq\0" "ngt_uq\0" "false_os\0" "neq_os\0" "ge_oq\0"  "gt_oq\0"  "true_us\0";

  // Try to find 7 differences...
  static const char vpcmpx[] = "eq\0" "lt\0" "le\0" "false\0" "neq\0" "ge\0"  "gt\0"    "true\0";
  static const char vpcomx[] = "lt\0" "le\0" "gt\0" "ge\0"    "eq\0"  "neq\0" "false\0" "true\0";

  static const char vshufpd[] = "a0\0a1\0b0\0b1\0a2\0a3\0b2\0b3\0a4\0a5\0b4\0b5\0a6\0a7\0b6\0b7\0";
  static const char vshufps[] = "a0\0a1\0a2\0a3\0a0\0a1\0a2\0a3\0b0\0b1\0b2\0b3\0b0\0b1\0b2\0b3\0";

  static const ImmBits vfpclassxx[] = {
    { 0x07, 0, ImmBits::kModeLookup, "qnan\0" "+0\0" "-0\0" "+inf\0" "-inf\0" "denormal\0" "-finite\0" "snan\0" }
  };

  static const ImmBits vgetmantxx[] = {
    { 0x03, 0, ImmBits::kModeLookup, "[1, 2)\0" "[1/2, 2)\0" "1/2, 1)\0" "[3/4, 3/2)\0" },
    { 0x04, 2, ImmBits::kModeLookup, "\0" "no-sign\0" },
    { 0x08, 3, ImmBits::kModeLookup, "\0" "qnan-if-sign\0" }
  };

  static const ImmBits vmpsadbw[] = {
    { 0x04, 2, ImmBits::kModeLookup, "blk1[0]\0" "blk1[1]\0" },
    { 0x03, 0, ImmBits::kModeLookup, "blk2[0]\0" "blk2[1]\0" "blk2[2]\0" "blk2[3]\0" },
    { 0x40, 6, ImmBits::kModeLookup, "blk1[4]\0" "blk1[5]\0" },
    { 0x30, 4, ImmBits::kModeLookup, "blk2[4]\0" "blk2[5]\0" "blk2[6]\0" "blk2[7]\0" }
  };

  static const ImmBits vpclmulqdq[] = {
    { 0x01, 0, ImmBits::kModeLookup, "lq\0" "hq\0" },
    { 0x10, 4, ImmBits::kModeLookup, "lq\0" "hq\0" }
  };

  static const ImmBits vperm2x128[] = {
    { 0x0B, 0, ImmBits::kModeLookup, "a0\0" "a1\0" "b0\0" "b1\0" "\0" "\0" "\0" "\0" "0\0" "0\0" "0\0" "0\0" },
    { 0xB0, 4, ImmBits::kModeLookup, "a0\0" "a1\0" "b0\0" "b1\0" "\0" "\0" "\0" "\0" "0\0" "0\0" "0\0" "0\0" }
  };

  static const ImmBits vrangexx[] = {
    { 0x03, 0, ImmBits::kModeLookup, "min\0" "max\0" "min-abs\0" "max-abs\0" },
    { 0x0C, 2, ImmBits::kModeLookup, "sign=src1\0" "sign=src2\0" "sign=0\0" "sign=1\0" }
  };

  static const ImmBits vreducexx_vrndscalexx[] = {
    { 0x07, 0, ImmBits::kModeLookup, "\0" "\0" "\0" "\0" "round\0" "floor\0" "ceil\0" "truncate\0" },
    { 0x08, 3, ImmBits::kModeLookup, "\0" "suppress\0" },
    { 0xF0, 4, ImmBits::kModeFormat, "len=%d" }
  };

  static const ImmBits vroundxx[] = {
    { 0x07, 0, ImmBits::kModeLookup, "round\0" "floor\0" "ceil\0" "truncate\0" "\0" "\0" "\0" "\0" },
    { 0x08, 3, ImmBits::kModeLookup, "\0" "inexact\0" }
  };

  uint32_t u8 = imm.getUInt8();
  switch (instId) {
    case X86Inst::kIdVblendpd:
    case X86Inst::kIdBlendpd:
      return X86Logging_formatImmShuf(sb, u8, 1, vecSize / 8);

    case X86Inst::kIdVblendps:
    case X86Inst::kIdBlendps:
      return X86Logging_formatImmShuf(sb, u8, 1, vecSize / 4);

    case X86Inst::kIdVcmppd:
    case X86Inst::kIdVcmpps:
    case X86Inst::kIdVcmpsd:
    case X86Inst::kIdVcmpss:
      return X86Logging_formatImmText(sb, u8, 5, 0, vcmpx);

    case X86Inst::kIdCmppd:
    case X86Inst::kIdCmpps:
    case X86Inst::kIdCmpsd:
    case X86Inst::kIdCmpss:
      return X86Logging_formatImmText(sb, u8, 3, 0, vcmpx);

    case X86Inst::kIdVdbpsadbw:
      return X86Logging_formatImmShuf(sb, u8, 2, 4);

    case X86Inst::kIdVdppd:
    case X86Inst::kIdVdpps:
    case X86Inst::kIdDppd:
    case X86Inst::kIdDpps:
      return X86Logging_formatImmShuf(sb, u8, 1, 8);

    case X86Inst::kIdVmpsadbw:
    case X86Inst::kIdMpsadbw:
      return X86Logging_formatImmBits(sb, u8, vmpsadbw, std::min<uint32_t>(vecSize / 8, 4));

    case X86Inst::kIdVpblendw:
    case X86Inst::kIdPblendw:
      return X86Logging_formatImmShuf(sb, u8, 1, 8);

    case X86Inst::kIdVpblendd:
      return X86Logging_formatImmShuf(sb, u8, 1, std::min<uint32_t>(vecSize / 4, 8));

    case X86Inst::kIdVpclmulqdq:
    case X86Inst::kIdPclmulqdq:
      return X86Logging_formatImmBits(sb, u8, vpclmulqdq, ASMJIT_ARRAY_SIZE(vpclmulqdq));

    case X86Inst::kIdVroundpd:
    case X86Inst::kIdVroundps:
    case X86Inst::kIdVroundsd:
    case X86Inst::kIdVroundss:
    case X86Inst::kIdRoundpd:
    case X86Inst::kIdRoundps:
    case X86Inst::kIdRoundsd:
    case X86Inst::kIdRoundss:
      return X86Logging_formatImmBits(sb, u8, vroundxx, ASMJIT_ARRAY_SIZE(vroundxx));

    case X86Inst::kIdVshufpd:
    case X86Inst::kIdShufpd:
      return X86Logging_formatImmText(sb, u8, 1, 2, vshufpd, std::min<uint32_t>(vecSize / 8, 8));

    case X86Inst::kIdVshufps:
    case X86Inst::kIdShufps:
      return X86Logging_formatImmText(sb, u8, 2, 4, vshufps, 4);

    case X86Inst::kIdVcvtps2ph:
      return X86Logging_formatImmBits(sb, u8, vroundxx, 1);

    case X86Inst::kIdVperm2f128:
    case X86Inst::kIdVperm2i128:
      return X86Logging_formatImmBits(sb, u8, vperm2x128, ASMJIT_ARRAY_SIZE(vperm2x128));

    case X86Inst::kIdVpermilpd:
      return X86Logging_formatImmShuf(sb, u8, 1, vecSize / 8);

    case X86Inst::kIdVpermilps:
      return X86Logging_formatImmShuf(sb, u8, 2, 4);

    case X86Inst::kIdVpshufd:
    case X86Inst::kIdPshufd:
      return X86Logging_formatImmShuf(sb, u8, 2, 4);

    case X86Inst::kIdVpshufhw:
    case X86Inst::kIdVpshuflw:
    case X86Inst::kIdPshufhw:
    case X86Inst::kIdPshuflw:
    case X86Inst::kIdPshufw:
      return X86Logging_formatImmShuf(sb, u8, 2, 4);

    // TODO: Maybe?
    case X86Inst::kIdVfixupimmpd:
    case X86Inst::kIdVfixupimmps:
    case X86Inst::kIdVfixupimmsd:
    case X86Inst::kIdVfixupimmss:
      return kErrorOk;

    case X86Inst::kIdVfpclasspd:
    case X86Inst::kIdVfpclassps:
    case X86Inst::kIdVfpclasssd:
    case X86Inst::kIdVfpclassss:
      return X86Logging_formatImmBits(sb, u8, vfpclassxx, ASMJIT_ARRAY_SIZE(vfpclassxx));

    case X86Inst::kIdVgetmantpd:
    case X86Inst::kIdVgetmantps:
    case X86Inst::kIdVgetmantsd:
    case X86Inst::kIdVgetmantss:
      return X86Logging_formatImmBits(sb, u8, vgetmantxx, ASMJIT_ARRAY_SIZE(vgetmantxx));

    case X86Inst::kIdVpcmpb:
    case X86Inst::kIdVpcmpd:
    case X86Inst::kIdVpcmpq:
    case X86Inst::kIdVpcmpw:
    case X86Inst::kIdVpcmpub:
    case X86Inst::kIdVpcmpud:
    case X86Inst::kIdVpcmpuq:
    case X86Inst::kIdVpcmpuw:
      return X86Logging_formatImmText(sb, u8, 2, 4, vpcmpx, 4);

    case X86Inst::kIdVpcomb:
    case X86Inst::kIdVpcomd:
    case X86Inst::kIdVpcomq:
    case X86Inst::kIdVpcomw:
    case X86Inst::kIdVpcomub:
    case X86Inst::kIdVpcomud:
    case X86Inst::kIdVpcomuq:
    case X86Inst::kIdVpcomuw:
      return X86Logging_formatImmText(sb, u8, 2, 4, vpcomx, 4);

    case X86Inst::kIdVpermq:
    case X86Inst::kIdVpermpd:
      return X86Logging_formatImmShuf(sb, u8, 2, 4);

    case X86Inst::kIdVpternlogd:
    case X86Inst::kIdVpternlogq:
      return X86Logging_formatImmShuf(sb, u8, 1, 8);

    case X86Inst::kIdVrangepd:
    case X86Inst::kIdVrangeps:
    case X86Inst::kIdVrangesd:
    case X86Inst::kIdVrangess:
      return X86Logging_formatImmBits(sb, u8, vrangexx, ASMJIT_ARRAY_SIZE(vrangexx));

    case X86Inst::kIdVreducepd:
    case X86Inst::kIdVreduceps:
    case X86Inst::kIdVreducesd:
    case X86Inst::kIdVreducess:
    case X86Inst::kIdVrndscalepd:
    case X86Inst::kIdVrndscaleps:
    case X86Inst::kIdVrndscalesd:
    case X86Inst::kIdVrndscaless:
      return X86Logging_formatImmBits(sb, u8, vreducexx_vrndscalexx, ASMJIT_ARRAY_SIZE(vreducexx_vrndscalexx));

    case X86Inst::kIdVshuff32x4:
    case X86Inst::kIdVshuff64x2:
    case X86Inst::kIdVshufi32x4:
    case X86Inst::kIdVshufi64x2: {
      uint32_t count = std::max<uint32_t>(vecSize / 16, 2);
      uint32_t bits = count <= 2 ? 1 : 2;
      return X86Logging_formatImmShuf(sb, u8, bits, count);
    }

    default:
      return kErrorOk;
  }
}

// ============================================================================
// [asmjit::X86Logging - Format Register]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Logging::formatRegister(
  StringBuilder& sb,
  uint32_t logOptions,
  const CodeEmitter* emitter,
  uint32_t archType,
  uint32_t rType,
  uint32_t rId) noexcept {

  ASMJIT_UNUSED(logOptions);
  ASMJIT_UNUSED(archType);

  if (Operand::isPackedId(rId)) {
#if !defined(ASMJIT_DISABLE_COMPILER)
    if (emitter && emitter->getType() == CodeEmitter::kTypeCompiler) {
      const CodeCompiler* cc = static_cast<const CodeCompiler*>(emitter);

      if (cc->isVirtRegValid(rId)) {
        VirtReg* vReg = cc->getVirtRegById(rId);
        ASMJIT_ASSERT(vReg != nullptr);

        const char* name = vReg->getName();
        if (name && name[0] != '\0')
          return sb.appendString(name);
        else
          return sb.appendFormat("v%u", static_cast<unsigned int>(Operand::unpackId(rId)));
      }
    }
#endif // !ASMJIT_DISABLE_COMPILER

    return sb.appendFormat("VirtReg<Type=%u Id=%u>", rType, rId);
  }
  else {
    if (rType < ASMJIT_ARRAY_SIZE(x86RegFormatInfo)) {
      const X86RegFormatInfo& rfi = x86RegFormatInfo[rType];

      if (rId < rfi.specialCount)
        return sb.appendString(x86RegFormatStrings + rfi.specialIndex + rId * 4);

      if (rId < rfi.count)
        return sb.appendFormat(x86RegFormatStrings + rfi.formatIndex, static_cast<unsigned int>(rId));
    }

    return sb.appendFormat("PhysReg<Type=%u Id=%u>", rType, rId);
  }
}

// ============================================================================
// [asmjit::X86Logging - Format Instruction]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Logging::formatInstruction(
  StringBuilder& sb,
  uint32_t logOptions,
  const CodeEmitter* emitter,
  uint32_t archType,
  const Inst::Detail& detail, const Operand_* opArray, uint32_t opCount) noexcept {

  uint32_t instId = detail.instId;
  uint32_t options = detail.options;

  // Format instruction options and instruction mnemonic.
  if (instId < X86Inst::_kIdCount) {
    const X86Inst& instInfo = X86Inst::getInst(instId);

    // SHORT|LONG options.
    if (options & X86Inst::kOptionShortForm) ASMJIT_PROPAGATE(sb.appendString("short "));
    if (options & X86Inst::kOptionLongForm) ASMJIT_PROPAGATE(sb.appendString("long "));

    // LOCK|XACQUIRE|XRELEASE options.
    if (options & X86Inst::kOptionXAcquire) ASMJIT_PROPAGATE(sb.appendString("xacquire "));
    if (options & X86Inst::kOptionXRelease) ASMJIT_PROPAGATE(sb.appendString("xrelease "));
    if (options & X86Inst::kOptionLock) ASMJIT_PROPAGATE(sb.appendString("lock "));

    // REP|REPNZ options.
    if (options & (X86Inst::kOptionRep | X86Inst::kOptionRepnz)) {
      sb.appendString((options & X86Inst::kOptionRep) ? "rep " : "repnz ");
      if (detail.hasExtraReg()) {
        ASMJIT_PROPAGATE(sb.appendChar('{'));
        ASMJIT_PROPAGATE(formatOperand(sb, logOptions, emitter, archType, detail.extraReg.toReg<Reg>()));
        ASMJIT_PROPAGATE(sb.appendString("} "));
      }
    }

    // REX options.
    if (options & X86Inst::kOptionRex) {
      const uint32_t kRXBWMask = X86Inst::kOptionOpCodeR |
                                 X86Inst::kOptionOpCodeX |
                                 X86Inst::kOptionOpCodeB |
                                 X86Inst::kOptionOpCodeW ;
      if (options & kRXBWMask) {
        sb.appendString("rex.");
        if (options & X86Inst::kOptionOpCodeR) sb.appendChar('r');
        if (options & X86Inst::kOptionOpCodeX) sb.appendChar('x');
        if (options & X86Inst::kOptionOpCodeB) sb.appendChar('b');
        if (options & X86Inst::kOptionOpCodeW) sb.appendChar('w');
        sb.appendChar(' ');
      }
      else {
        ASMJIT_PROPAGATE(sb.appendString("rex "));
      }
    }

    // VEX|EVEX options.
    if (options & X86Inst::kOptionVex3) ASMJIT_PROPAGATE(sb.appendString("vex3 "));
    if (options & X86Inst::kOptionEvex) ASMJIT_PROPAGATE(sb.appendString("evex "));

    ASMJIT_PROPAGATE(sb.appendString(instInfo.getName()));
  }
  else {
    ASMJIT_PROPAGATE(sb.appendFormat("<unknown id=#%u>", static_cast<unsigned int>(instId)));
  }

  for (uint32_t i = 0; i < opCount; i++) {
    const Operand_& op = opArray[i];
    if (op.isNone()) break;

    ASMJIT_PROPAGATE(sb.appendString(i == 0 ? " " : ", "));
    ASMJIT_PROPAGATE(formatOperand(sb, logOptions, emitter, archType, op));

    if (op.isImm() && (logOptions & Logger::kOptionImmExtended)) {
      uint32_t vecSize = 16;
      for (uint32_t j = 0; j < opCount; j++)
        if (opArray[j].isReg())
          vecSize = std::max<uint32_t>(vecSize, opArray[j].getSize());
      ASMJIT_PROPAGATE(X86Logging_formatImmExtended(sb, logOptions, instId, vecSize, op.as<Imm>()));
    }

    // Support AVX-512 {k}{z}.
    if (i == 0) {
      if (detail.extraReg.getKind() == X86Reg::kKindK) {
        ASMJIT_PROPAGATE(sb.appendString(" {"));
        ASMJIT_PROPAGATE(formatOperand(sb, logOptions, emitter, archType, detail.extraReg.toReg<Reg>()));
        ASMJIT_PROPAGATE(sb.appendChar('}'));

        if (options & X86Inst::kOptionZMask)
          ASMJIT_PROPAGATE(sb.appendString("{z}"));
      }
      else if (options & X86Inst::kOptionZMask) {
        ASMJIT_PROPAGATE(sb.appendString(" {z}"));
      }
    }

    // Support AVX-512 {1tox}.
    if (op.isMem() && (options & X86Inst::kOption1ToX))
      ASMJIT_PROPAGATE(sb.appendString(" {1tox}"));
  }

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_LOGGING
