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
#include "../base/cpuinfo.h"
#include "../base/logging.h"
#include "../base/misc_p.h"
#include "../base/utils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86logging_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [FastUInt8]
// ============================================================================

#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
typedef unsigned char FastUInt8;
#else
typedef unsigned int FastUInt8;
#endif

// ============================================================================
// [Constants]
// ============================================================================

//! \internal
//!
//! X86/X64 bytes used to encode important prefixes.
enum X86Byte {
  //! 1-byte REX prefix mask.
  kX86ByteRex = 0x40,

  //! 1-byte REX.W component.
  kX86ByteRexW = 0x08,

  //! 2-byte VEX prefix:
  //!   - `[0]` - `0xC5`.
  //!   - `[1]` - `RvvvvLpp`.
  kX86ByteVex2 = 0xC5,

  //! 3-byte VEX prefix.
  //!   - `[0]` - `0xC4`.
  //!   - `[1]` - `RXBmmmmm`.
  //!   - `[2]` - `WvvvvLpp`.
  kX86ByteVex3 = 0xC4,

  //! 3-byte XOP prefix.
  //!   - `[0]` - `0x8F`.
  //!   - `[1]` - `RXBmmmmm`.
  //!   - `[2]` - `WvvvvLpp`.
  kX86ByteXop3 = 0x8F,

  //! 4-byte EVEX prefix.
  //!   - `[0]` - `0x62`.
  //!   - `[1]` - Payload0 or `P[ 7: 0]` - `[R  X  B  R' 0  0  m  m]`.
  //!   - `[2]` - Payload1 or `P[15: 8]` - `[W  v  v  v  v  1  p  p]`.
  //!   - `[3]` - Payload2 or `P[23:16]` - `[z  L' L  b  V' a  a  a]`.
  //!
  //! Groups:
  //!   - `P[ 1: 0]` - OPCODE: EVEX.mmmmm, only lowest 2 bits [1:0] used.
  //!   - `P[ 3: 2]` - ______: Must be 0.
  //!   - `P[    4]` - REG-ID: EVEX.R' - 5th bit of 'RRRRR'.
  //!   - `P[    5]` - REG-ID: EVEX.B  - 4th bit of 'BBBBB'.
  //!   - `P[    6]` - REG-ID: EVEX.X  - 5th bit of 'BBBBB' or 4th bit of 'XXXX' (with SIB).
  //!   - `P[    7]` - REG-ID: EVEX.R  - 4th bit of 'RRRRR'.
  //!   - `P[ 9: 8]` - OPCODE: EVEX.pp.
  //!   - `P[   10]` - ______: Must be 1.
  //!   - `P[14:11]` - REG-ID: 4 bits of 'VVVV'.
  //!   - `P[   15]` - OPCODE: EVEX.W.
  //!   - `P[18:16]` - REG-ID: K register k0...k7 (Merging/Zeroing Vector Ops).
  //!   - `P[   19]` - REG-ID: 5th bit of 'VVVVV'.
  //!   - `P[   20]` - OPCODE: Broadcast/Rounding Control/SAE bit.
  //!   - `P[22.21]` - OPCODE: Vector Length (L' and  L) / Rounding Control.
  //!   - `P[   23]` - OPCODE: Zeroing/Merging.
  kX86ByteEvex = 0x62
};

// AsmJit specific (used to encode VVVVV field in XOP/VEX/EVEX).
enum VexVVVVV {
  kVexVVVVVShift = 7,
  kVexVVVVVMask = 0x1F << kVexVVVVVShift
};

//! \internal
//!
//! Instruction 2-byte/3-byte opcode prefix definition.
struct X86OpCodeMM {
  uint8_t len;
  uint8_t data[3];
};

//! \internal
//!
//! Mandatory prefixes used to encode legacy [66, F3, F2] or [9B] byte.
static const uint8_t x86OpCodePP[8] = { 0x00, 0x66, 0xF3, 0xF2, 0x00, 0x00, 0x00, 0x9B };

//! \internal
//!
//! Instruction 2-byte/3-byte opcode prefix data.
static const X86OpCodeMM x86OpCodeMM[] = {
  { 0, { 0x00, 0x00, 0 } }, // #00 (0b0000).
  { 1, { 0x0F, 0x00, 0 } }, // #01 (0b0001).
  { 2, { 0x0F, 0x38, 0 } }, // #02 (0b0010).
  { 2, { 0x0F, 0x3A, 0 } }, // #03 (0b0011).
  { 2, { 0x0F, 0x01, 0 } }, // #04 (0b0100).
  { 0, { 0x00, 0x00, 0 } }, // #05 (0b0101).
  { 0, { 0x00, 0x00, 0 } }, // #06 (0b0110).
  { 0, { 0x00, 0x00, 0 } }, // #07 (0b0111).
  { 0, { 0x00, 0x00, 0 } }, // #08 (0b1000).
  { 0, { 0x00, 0x00, 0 } }, // #09 (0b1001).
  { 0, { 0x00, 0x00, 0 } }, // #0A (0b1010).
  { 0, { 0x00, 0x00, 0 } }, // #0B (0b1011).
  { 0, { 0x00, 0x00, 0 } }, // #0C (0b1100).
  { 0, { 0x00, 0x00, 0 } }, // #0D (0b1101).
  { 0, { 0x00, 0x00, 0 } }, // #0E (0b1110).
  { 0, { 0x00, 0x00, 0 } }  // #0F (0b1111).
};

static const uint8_t x86SegmentPrefix[8] = { 0x00, 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x00 };
static const uint8_t x86OpCodePushSeg[8] = { 0x00, 0x06, 0x0E, 0x16, 0x1E, 0xA0, 0xA8, 0x00 };
static const uint8_t x86OpCodePopSeg[8]  = { 0x00, 0x07, 0x00, 0x17, 0x1F, 0xA1, 0xA9, 0x00 };

// ============================================================================
// [asmjit::X86MemInfo | X86VEXPrefix | X86LLByRegType | X86CDisp8Table]
// ============================================================================

//! \internal
//!
//! Memory operand's info bits.
//!
//! A lookup table that contains various information based on the BASE and INDEX
//! information of a memory operand. This is much better and safer than playing
//! with IFs in the code and can check for errors must faster and better.
enum X86MemInfo_Enum {
  kX86MemInfo_0         = 0x00,

  kX86MemInfo_BaseGp    = 0x01, //!< Has BASE reg, REX.B can be 1, compatible with REX.B byte.
  kX86MemInfo_Index     = 0x02, //!< Has INDEX reg, REX.X can be 1, compatible with REX.X byte.

  kX86MemInfo_BaseLabel = 0x10, //!< Base is Label.
  kX86MemInfo_BaseRip   = 0x20, //!< Base is RIP.

  kX86MemInfo_67H_X86   = 0x40, //!< Address-size override in 32-bit mode.
  kX86MemInfo_67H_X64   = 0x80, //!< Address-size override in 64-bit mode.
  kX86MemInfo_67H_Mask  = 0xC0  //!< Contains all address-size override bits.
};

template<uint32_t X>
struct X86MemInfo_T {
  enum {
    B = (X     ) & 0x1F,
    I = (X >> 5) & 0x1F,

    kBase  = ((B >= X86Reg::kRegGpw  && B <= X86Reg::kRegGpq ) ? kX86MemInfo_BaseGp    :
              (B == X86Reg::kRegRip                          ) ? kX86MemInfo_BaseRip   :
              (B == Label::kLabelTag                         ) ? kX86MemInfo_BaseLabel : 0),

    kIndex = ((I >= X86Reg::kRegGpw  && I <= X86Reg::kRegGpq ) ? kX86MemInfo_Index     :
              (I >= X86Reg::kRegXmm  && I <= X86Reg::kRegZmm ) ? kX86MemInfo_Index     : 0),

    k67H   = ((B == X86Reg::kRegGpw  && I == X86Reg::kRegNone) ? kX86MemInfo_67H_X86   :
              (B == X86Reg::kRegGpd  && I == X86Reg::kRegNone) ? kX86MemInfo_67H_X64   :
              (B == X86Reg::kRegNone && I == X86Reg::kRegGpw ) ? kX86MemInfo_67H_X86   :
              (B == X86Reg::kRegNone && I == X86Reg::kRegGpd ) ? kX86MemInfo_67H_X64   :
              (B == X86Reg::kRegGpw  && I == X86Reg::kRegGpw ) ? kX86MemInfo_67H_X86   :
              (B == X86Reg::kRegGpd  && I == X86Reg::kRegGpd ) ? kX86MemInfo_67H_X64   :
              (B == X86Reg::kRegGpw  && I == X86Reg::kRegXmm ) ? kX86MemInfo_67H_X86   :
              (B == X86Reg::kRegGpd  && I == X86Reg::kRegXmm ) ? kX86MemInfo_67H_X64   :
              (B == X86Reg::kRegGpw  && I == X86Reg::kRegYmm ) ? kX86MemInfo_67H_X86   :
              (B == X86Reg::kRegGpd  && I == X86Reg::kRegYmm ) ? kX86MemInfo_67H_X64   :
              (B == X86Reg::kRegGpw  && I == X86Reg::kRegZmm ) ? kX86MemInfo_67H_X86   :
              (B == X86Reg::kRegGpd  && I == X86Reg::kRegZmm ) ? kX86MemInfo_67H_X64   :
              (B == Label::kLabelTag && I == X86Reg::kRegGpw ) ? kX86MemInfo_67H_X86   :
              (B == Label::kLabelTag && I == X86Reg::kRegGpd ) ? kX86MemInfo_67H_X64   : 0),

    kValue = kBase | kIndex | k67H | 0x04 | 0x08
  };
};

// The result stored in the LUT is a combination of
//   - 67H - Address override prefix - depends on BASE+INDEX register types and
//           the target architecture.
//   - REX - A possible combination of REX.[B|X|R|W] bits in REX prefix where
//           REX.B and REX.X are possibly masked out, but REX.R and REX.W are
//           kept as is.
static const uint8_t x86MemInfo[] = { ASMJIT_TABLE_T_1024(X86MemInfo_T, kValue, 0) };

// VEX3 or XOP xor bits applied to the opcode before emitted. The index to this
// table is 'mmmmm' value, which contains all we need. This is only used by a
// 3 BYTE VEX and XOP prefixes, 2 BYTE VEX prefix is handled differently. The
// idea is to minimize the difference between VEX3 vs XOP when encoding VEX
// or XOP instruction. This should minimize the code required to emit such
// instructions and should also make it faster as we don't need any branch to
// decide between VEX3 vs XOP.
//            ____    ___
// [_OPCODE_|WvvvvLpp|RXBmmmmm|VEX3_XOP]
template<uint32_t X>
struct X86VEXPrefix_T {
  enum { kValue = ((X & 0x08) ? kX86ByteXop3 : kX86ByteVex3) | (0xF << 19) | (0x7 << 13) };
};
static const uint32_t x86VEXPrefix[] = { ASMJIT_TABLE_T_16(X86VEXPrefix_T, kValue, 0) };

// Table that contains LL opcode field addressed by a register size / 16. It's
// used to propagate L.256 or L.512 when YMM or ZMM registers are used,
// respectively.
template<uint32_t X>
struct X86LLBySizeDiv16_T {
  enum {
    kValue = (X & (64 >> 4)) ? X86Inst::kOpCode_LL_512 :
             (X & (32 >> 4)) ? X86Inst::kOpCode_LL_256 : 0
  };
};
static const uint32_t x86LLBySizeDiv16[] = { ASMJIT_TABLE_T_16(X86LLBySizeDiv16_T, kValue, 0) };

// Table that contains LL opcode field addressed by a register size / 16. It's
// used to propagate L.256 or L.512 when YMM or ZMM registers are used,
// respectively.
template<uint32_t X>
struct X86LLByRegType_T {
  enum {
    kValue = X == X86Reg::kRegZmm ? X86Inst::kOpCode_LL_512 :
             X == X86Reg::kRegYmm ? X86Inst::kOpCode_LL_256 : 0
  };
};
static const uint32_t x86LLByRegType[] = { ASMJIT_TABLE_T_16(X86LLByRegType_T, kValue, 0) };

// Table that contains a scale (shift left) based on 'TTWLL' field and
// the instruction's tuple-type (TT) field. The scale is then applied to
// the BASE-N stored in each opcode to calculate the final compressed
// displacement used by all EVEX encoded instructions.
template<uint32_t X>
struct X86CDisp8SHL_T {
  enum {
    TT = (((X) >> 3) << X86Inst::kOpCode_CDTT_Shift),
    LL = (((X) >> 0) & 0x3),
    W  = (((X) >> 2) & 0x1),

    kValue = (TT == X86Inst::kOpCode_CDTT_None ? ((LL==0) ? 0 : (LL==1) ? 0   : 0  ) :
              TT == X86Inst::kOpCode_CDTT_ByLL ? ((LL==0) ? 0 : (LL==1) ? 1   : 2  ) :
              TT == X86Inst::kOpCode_CDTT_T1W  ? ((LL==0) ? W : (LL==1) ? 1+W : 2+W) :
              TT == X86Inst::kOpCode_CDTT_DUP  ? ((LL==0) ? 0 : (LL==1) ? 2   : 3  ) : 0 ) << X86Inst::kOpCode_CDSHL_Shift
  };
};
static const uint32_t x86CDisp8SHL[] = { ASMJIT_TABLE_T_32(X86CDisp8SHL_T, kValue, 0) };

// Table that contains MOD byte of a 16-bit [BASE + disp] address.
//   0xFF == Invalid.
static const uint8_t x86Mod16BaseTable[8] = {
  0xFF, // AX -> N/A.
  0xFF, // CX -> N/A.
  0xFF, // DX -> N/A.
  0x07, // BX -> 111.
  0xFF, // SP -> N/A.
  0x06, // BP -> 110.
  0x04, // SI -> 100.
  0x05  // DI -> 101.
};

// Table that contains MOD byte of a 16-bit [BASE + INDEX + disp] combination.
//   0xFF == Invalid.
template<uint32_t X>
struct X86Mod16BaseIndexTable_T {
  enum {
    B = X >> 3,
    I = X & 0x7,

    kValue = ((B == X86Gp::kIdBx && I == X86Gp::kIdSi) || (B == X86Gp::kIdSi && I == X86Gp::kIdBx)) ? 0x00 :
             ((B == X86Gp::kIdBx && I == X86Gp::kIdDi) || (B == X86Gp::kIdDi && I == X86Gp::kIdBx)) ? 0x01 :
             ((B == X86Gp::kIdBp && I == X86Gp::kIdSi) || (B == X86Gp::kIdSi && I == X86Gp::kIdBp)) ? 0x02 :
             ((B == X86Gp::kIdBp && I == X86Gp::kIdDi) || (B == X86Gp::kIdDi && I == X86Gp::kIdBp)) ? 0x03 : 0xFF
  };
};
static const uint8_t x86Mod16BaseIndexTable[] = { ASMJIT_TABLE_T_64(X86Mod16BaseIndexTable_T, kValue, 0) };

// ============================================================================
// [asmjit::X86Assembler - Helpers]
// ============================================================================

static ASMJIT_INLINE bool x86IsJmpOrCall(uint32_t instId) noexcept {
  return instId == X86Inst::kIdJmp ||
         instId == X86Inst::kIdCall;
}

static ASMJIT_INLINE bool x86IsImplicitMem(const Operand_& op, uint32_t base) noexcept {
  return op.isMem() && op.as<X86Mem>().getBaseId() == base;
}

static ASMJIT_INLINE int64_t x86SignExtend32To64(int64_t imm) noexcept {
  return static_cast<int64_t>(static_cast<int32_t>(imm & 0xFFFFFFFF));
}

//! Get `O` field of `opCode`.
static ASMJIT_INLINE uint32_t x86ExtractO(uint32_t opCode) noexcept {
  return (opCode >> X86Inst::kOpCode_O_Shift) & 0x07;
}

static ASMJIT_INLINE uint32_t x86ExtractREX(uint32_t opCode, uint32_t options) noexcept {
  // kOpCode_REX was designed in a way that when shifted there will be no bytes
  // set except REX.[B|X|R|W]. The returned value forms a real REX prefix byte.
  // This case is tested by `X86Inst.cpp`.
  return (opCode | options) >> X86Inst::kOpCode_REX_Shift;
}

//! Combine `regId` and `vvvvvId` into a single value (used by AVX and AVX-512).
static ASMJIT_INLINE uint32_t x86PackRegAndVvvvv(uint32_t regId, uint32_t vvvvvId) noexcept {
  return regId + (vvvvvId << kVexVVVVVShift);
}

static ASMJIT_INLINE uint32_t x86OpCodeLByVMem(const Operand_& op) noexcept {
  return x86LLByRegType[op.as<X86Mem>().getIndexType()];
}

static ASMJIT_INLINE uint32_t x86OpCodeLBySize(uint32_t size) noexcept {
  return x86LLBySizeDiv16[size / 16];
}

static ASMJIT_INLINE uint32_t x86ExtractLLMM(uint32_t opCode, uint32_t options) noexcept {
  uint32_t x = opCode & (X86Inst::kOpCode_LL_Mask | X86Inst::kOpCode_MM_Mask);
  uint32_t y = options & X86Inst::kOptionVex3;
  return (x | y) >> X86Inst::kOpCode_MM_Shift;
}

//! Encode MOD byte.
static ASMJIT_INLINE uint32_t x86EncodeMod(uint32_t m, uint32_t o, uint32_t rm) noexcept {
  ASMJIT_ASSERT(m <= 3);
  ASMJIT_ASSERT(o <= 7);
  ASMJIT_ASSERT(rm <= 7);
  return (m << 6) + (o << 3) + rm;
}

//! Encode SIB byte.
static ASMJIT_INLINE uint32_t x86EncodeSib(uint32_t s, uint32_t i, uint32_t b) noexcept {
  ASMJIT_ASSERT(s <= 3);
  ASMJIT_ASSERT(i <= 7);
  ASMJIT_ASSERT(b <= 7);
  return (s << 6) + (i << 3) + b;
}

// ============================================================================
// [asmjit::X86Assembler - Construction / Destruction]
// ============================================================================

X86Assembler::X86Assembler(CodeHolder* code) noexcept : Assembler() {
  if (code)
    code->attach(this);
}
X86Assembler::~X86Assembler() noexcept {}

// ============================================================================
// [asmjit::X86Assembler - Events]
// ============================================================================

Error X86Assembler::onAttach(CodeHolder* code) noexcept {
  uint32_t archType = code->getArchType();
  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  ASMJIT_PROPAGATE(Base::onAttach(code));

  if (archType == ArchInfo::kTypeX86) {
    // 32 bit architecture - X86.
    _setAddressOverrideMask(kX86MemInfo_67H_X86);
    _globalOptions |= X86Inst::_kOptionInvalidRex;
    _nativeGpArray = x86OpData.gpd;
  }
  else {
    // 64 bit architecture - X64 or X32.
    _setAddressOverrideMask(kX86MemInfo_67H_X64);
    _nativeGpArray = x86OpData.gpq;
  }

  _nativeGpReg = _nativeGpArray[0];
  return kErrorOk;
}

Error X86Assembler::onDetach(CodeHolder* code) noexcept {
  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::X86Assembler - Helpers]
// ============================================================================

#define EMIT_BYTE(VAL)                               \
  do {                                               \
    cursor[0] = static_cast<uint8_t>((VAL) & 0xFFU); \
    cursor += 1;                                     \
  } while (0)

#define EMIT_16(VAL)                                 \
  do {                                               \
    Utils::writeU16uLE(cursor,                       \
      static_cast<uint32_t>((VAL) & 0xFFFFU));       \
    cursor += 2;                                     \
  } while (0)

#define EMIT_32(VAL)                                 \
  do {                                               \
    Utils::writeU32uLE(cursor,                       \
      static_cast<uint32_t>((VAL) & 0xFFFFFFFFU));   \
    cursor += 4;                                     \
  } while (0)

#define ADD_66H_P(EXP)                                                   \
  do {                                                                   \
    opCode |= (static_cast<uint32_t>(EXP) << X86Inst::kOpCode_PP_Shift); \
  } while (0)

#define ADD_66H_P_BY_SIZE(SIZE)                                          \
  do {                                                                   \
    opCode |= (static_cast<uint32_t>((SIZE) & 0x02))                     \
           << (X86Inst::kOpCode_PP_Shift - 1);                           \
  } while (0)

#define ADD_REX_W(EXP)                                                   \
  do {                                                                   \
    if (EXP)                                                             \
      opCode |= X86Inst::kOpCode_W;                                      \
  } while (0)

#define ADD_REX_W_BY_SIZE(SIZE)                                          \
  do {                                                                   \
    if ((SIZE) == 8)                                                     \
      opCode |= X86Inst::kOpCode_W;                                      \
  } while (0)

#define ADD_PREFIX_BY_SIZE(SIZE)                                         \
  do {                                                                   \
    ADD_66H_P_BY_SIZE(SIZE);                                             \
    ADD_REX_W_BY_SIZE(SIZE);                                             \
  } while (0)

#define ADD_VEX_W(EXP)                                                   \
  do {                                                                   \
    opCode |= static_cast<uint32_t>(EXP) << X86Inst::kOpCode_W_Shift;    \
  } while (0)

#define EMIT_PP(OPCODE)                                                  \
  do {                                                                   \
    uint32_t ppIndex =                                                   \
      ((OPCODE                   ) >> X86Inst::kOpCode_PP_Shift) &       \
      (X86Inst::kOpCode_PP_FPUMask >> X86Inst::kOpCode_PP_Shift) ;       \
    uint8_t ppCode = x86OpCodePP[ppIndex];                               \
                                                                         \
    cursor[0] = ppCode;                                                  \
    cursor   += ppIndex != 0;                                            \
  } while (0)

#define EMIT_MM_OP(OPCODE)                                               \
  do {                                                                   \
    uint32_t op = OPCODE & (0x00FF | X86Inst::kOpCode_MM_Mask);          \
                                                                         \
    uint32_t mmIndex = op >> X86Inst::kOpCode_MM_Shift;                  \
    const X86OpCodeMM& mmCode = x86OpCodeMM[mmIndex];                    \
                                                                         \
    if (mmIndex) {                                                       \
      cursor[0] = mmCode.data[0];                                        \
      cursor[1] = mmCode.data[1];                                        \
      cursor   += mmCode.len;                                            \
    }                                                                    \
                                                                         \
    EMIT_BYTE(op);                                                       \
  } while (0)

// If the operand is BPL|SPL|SIL|DIL|R8B-15B
//   - Force REX prefix
// If the operand is AH|BH|CH|DH
//   - patch its index from 0..3 to 4..7 as encoded by X86.
//   - Disallow REX prefix.
#define FIXUP_GPB(REG_OP, REG_ID, ...)                                   \
  do {                                                                   \
    if (!static_cast<const X86Gp&>(REG_OP).isGpbHi()) {                  \
      options |= (REG_ID >= 4) ? X86Inst::kOptionRex : 0;                \
    }                                                                    \
    else {                                                               \
      options |= X86Inst::_kOptionInvalidRex;                            \
      REG_ID += 4;                                                       \
    }                                                                    \
  } while (0)

#define ENC_OPS1(OP0)                     ((Operand::kOp##OP0))
#define ENC_OPS2(OP0, OP1)                ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3))
#define ENC_OPS3(OP0, OP1, OP2)           ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3) + ((Operand::kOp##OP2) << 6))
#define ENC_OPS4(OP0, OP1, OP2, OP3)      ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3) + ((Operand::kOp##OP2) << 6) + ((Operand::kOp##OP3) << 9))

// ============================================================================
// [asmjit::X86Assembler - Emit]
// ============================================================================

Error X86Assembler::_emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {
  Error err;

  const Operand_* rmRel;         // Memory operand or operand that holds Label|Imm.
  uint32_t rmInfo;               // Memory operand's info based on x86MemInfo.
  uint32_t rbReg;                // Memory base or modRM register.
  uint32_t rxReg;                // Memory index register.
  uint32_t opReg;                // ModR/M opcode or register id.
  uint32_t opCode;               // Instruction opcode.

  LabelEntry* label;             // Label entry.
  RelocEntry* re = nullptr;      // Relocation entry.
  int32_t relOffset;             // Relative offset
  FastUInt8 relSize = 0;         // Relative size.

  int64_t imVal = 0;             // Immediate value (must be 64-bit).
  FastUInt8 imLen = 0;           // Immediate length.

  const uint32_t kSHR_W_PP = X86Inst::kOpCode_PP_Shift - 16;
  const uint32_t kSHR_W_EW = X86Inst::kOpCode_EW_Shift - 23;

  uint8_t* cursor = _bufferPtr;
  uint32_t options = static_cast<uint32_t>(instId >= X86Inst::_kIdCount)       |
                     static_cast<uint32_t>((size_t)(_bufferEnd - cursor) < 16) |
                     getGlobalOptions() | getOptions();

  const X86Inst* instData = X86InstDB::instData + instId;
  const X86Inst::CommonData* commonData;

  // Handle failure and rare cases first.
  const uint32_t kErrorsAndSpecialCases =
    CodeEmitter::kOptionMaybeFailureCase | // Error and buffer check.
    CodeEmitter::kOptionStrictValidation | // Strict validation.
    X86Inst::kOptionRep                  | // REP/REPZ prefix.
    X86Inst::kOptionRepnz                | // REPNZ prefix.
    X86Inst::kOptionLock                 | // LOCK prefix.
    X86Inst::kOptionXAcquire             | // XACQUIRE prefix.
    X86Inst::kOptionXRelease             ; // XRELEASE prefix.

  // Signature of the first 3 operands.
  uint32_t isign3 = o0.getOp() + (o1.getOp() << 3) + (o2.getOp() << 6);

  if (ASMJIT_UNLIKELY(options & kErrorsAndSpecialCases)) {
    // Don't do anything if we are in error state.
    if (_lastError) return _lastError;

    if (options & CodeEmitter::kOptionMaybeFailureCase) {
      // Unknown instruction.
      if (ASMJIT_UNLIKELY(instId >= X86Inst::_kIdCount))
        goto InvalidArgument;

      // Grow request, happens rarely.
      if ((size_t)(_bufferEnd - cursor) < 16) {
        err = _code->growBuffer(&_section->_buffer, 16);
        if (ASMJIT_UNLIKELY(err)) goto Failed;

        cursor = _bufferPtr;
        options &= ~1;
      }
    }

    // Strict validation.
#if !defined(ASMJIT_DISABLE_VALIDATION)
    if (options & CodeEmitter::kOptionStrictValidation) {
      Operand_ opArray[6];

      opArray[0].copyFrom(o0);
      opArray[1].copyFrom(o1);
      opArray[2].copyFrom(o2);
      opArray[3].copyFrom(o3);

      if (options & kOptionOp4Op5Used) {
        opArray[4].copyFrom(_op4);
        opArray[5].copyFrom(_op5);
      }
      else {
        opArray[4].reset();
        opArray[5].reset();
      }

      err = Inst::validate(getArchType(), Inst::Detail(instId, options, _extraReg), opArray, 6);
      if (ASMJIT_UNLIKELY(err)) goto Failed;
    }
#endif // !ASMJIT_DISABLE_VALIDATION

    uint32_t iFlags = instData->getFlags();

    // LOCK, XACQUIRE, and XRELEASE prefixes.
    if (options & X86Inst::kOptionLock) {
      bool xAcqRel = (options & (X86Inst::kOptionXAcquire | X86Inst::kOptionXRelease)) != 0;

      if (ASMJIT_UNLIKELY(!(iFlags & (X86Inst::kFlagLock)) && !xAcqRel))
        goto InvalidLockPrefix;

      if (xAcqRel) {
        if (ASMJIT_UNLIKELY((options & X86Inst::kOptionXAcquire) && !(iFlags & X86Inst::kFlagXAcquire)))
          goto InvalidXAcquirePrefix;

        if (ASMJIT_UNLIKELY((options & X86Inst::kOptionXRelease) && !(iFlags & X86Inst::kFlagXRelease)))
          goto InvalidXReleasePrefix;

        EMIT_BYTE((options & X86Inst::kOptionXAcquire) ? 0xF2 : 0xF3);
      }

      EMIT_BYTE(0xF0);
    }

    // REP and REPNZ prefixes.
    if (options & (X86Inst::kOptionRep | X86Inst::kOptionRepnz)) {
      if (ASMJIT_UNLIKELY(!(iFlags & (X86Inst::kFlagRep | X86Inst::kFlagRepnz))))
        goto InvalidRepPrefix;

      if (_extraReg.isValid() && ASMJIT_UNLIKELY(_extraReg.getKind() != X86Reg::kKindGp || _extraReg.getId() != X86Gp::kIdCx))
        goto InvalidRepPrefix;

      EMIT_BYTE((options & X86Inst::kOptionRepnz) ? 0xF2 : 0xF3);
    }
  }

  // --------------------------------------------------------------------------
  // [Encoding Scope]
  // --------------------------------------------------------------------------

  opCode = instData->getMainOpCode();
  opReg = x86ExtractO(opCode);
  commonData = &instData->getCommonData();

  switch (instData->getEncodingType()) {
    case X86Inst::kEncodingNone:
      goto EmitDone;

    // ------------------------------------------------------------------------
    // [X86]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingX86Op:
      goto EmitX86Op;

    case X86Inst::kEncodingX86Op_O_I8:
      if (ASMJIT_UNLIKELY(isign3 != ENC_OPS1(Imm)))
        goto InvalidInstruction;

      imVal = o0.as<Imm>().getUInt8();
      imLen = 1;
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Op_O:
      rbReg = 0;
      goto EmitX86R;

    case X86Inst::kEncodingX86Op_xAX:
      if (isign3 == 0)
        goto EmitX86Op;

      if (isign3 == ENC_OPS1(Reg) && o0.getId() == X86Gp::kIdAx)
        goto EmitX86Op;
      break;

    case X86Inst::kEncodingX86Op_xDX_xAX:
      if (isign3 == 0)
        goto EmitX86Op;

      if (isign3 == ENC_OPS2(Reg, Reg) && o0.getId() == X86Gp::kIdDx &&
                                          o1.getId() == X86Gp::kIdAx)
        goto EmitX86Op;
      break;

    case X86Inst::kEncodingX86Op_ZAX:
      if (isign3 == 0)
        goto EmitX86Op;

      rmRel = &o0;
      if (isign3 == ENC_OPS1(Mem) && x86IsImplicitMem(o0, X86Gp::kIdAx))
        goto EmitX86OpImplicitMem;

      break;

    case X86Inst::kEncodingX86I_xAX:
      // Implicit form.
      if (isign3 == ENC_OPS1(Imm)) {
        imVal = o0.as<Imm>().getUInt8();
        imLen = 1;
        goto EmitX86Op;
      }

      // Explicit form.
      if (isign3 == ENC_OPS2(Reg, Imm) && o0.getId() == X86Gp::kIdAx) {
        imVal = o1.as<Imm>().getUInt8();
        imLen = 1;
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86M:
      rbReg = o0.getId();
      ADD_PREFIX_BY_SIZE(o0.getSize());

      if (isign3 == ENC_OPS1(Reg))
        goto EmitX86R;

      rmRel = &o0;
      if (isign3 == ENC_OPS1(Mem))
        goto EmitX86M;
      break;

    case X86Inst::kEncodingX86M_GPB_MulDiv:
CaseX86M_GPB_MulDiv:
      // Explicit form?
      if (isign3 > 0x7) {
        // [AX] <- [AX] div|mul r8.
        if (isign3 == ENC_OPS2(Reg, Reg)) {
          if (ASMJIT_UNLIKELY(!X86Reg::isGpw(o0, X86Gp::kIdAx) || !X86Reg::isGpb(o1)))
            goto InvalidInstruction;

          rbReg = o1.getId();
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }

        // [AX] <- [AX] div|mul m8.
        if (isign3 == ENC_OPS2(Reg, Mem)) {
          if (ASMJIT_UNLIKELY(!X86Reg::isGpw(o0, X86Gp::kIdAx)))
            goto InvalidInstruction;

          rmRel = &o1;
          goto EmitX86M;
        }

        // [?DX:?AX] <- [?DX:?AX] div|mul r16|r32|r64
        if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
          if (ASMJIT_UNLIKELY(o0.getSize() != o1.getSize()))
            goto InvalidInstruction;
          rbReg = o2.getId();

          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }

        // [?DX:?AX] <- [?DX:?AX] div|mul m16|m32|m64
        if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
          if (ASMJIT_UNLIKELY(o0.getSize() != o1.getSize()))
            goto InvalidInstruction;
          rmRel = &o2;

          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }

        goto InvalidInstruction;
      }

      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86M_GPB:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto AmbiguousOperandSize;
        rmRel = &o0;

        opCode += o0.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86M_Only:
      if (isign3 == ENC_OPS1(Mem)) {
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Rm:
      ADD_PREFIX_BY_SIZE(o0.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Rm_NoRexW:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Rm_Raw66H:
      // We normally emit either [66|F2|F3], this instruction requires 66+[F2|F3].
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        if (o0.getSize() == 2)
          EMIT_BYTE(0x66);
        else
          ADD_REX_W_BY_SIZE(o0.getSize());
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;

        if (o0.getSize() == 2)
          EMIT_BYTE(0x66);
        else
          ADD_REX_W_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Mr:
      ADD_PREFIX_BY_SIZE(o0.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Mr_NoSize:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o0.getId();
        opReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        rmRel = &o0;
        opReg = o1.getId();
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Arith:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (o0.getSize() != o1.getSize())
          goto OperandSizeMismatch;

        opReg = o0.getId();
        rbReg = o1.getId();

        if (o0.getSize() == 1) {
          opCode += 2;
          FIXUP_GPB(o0, opReg);
          FIXUP_GPB(o1, rbReg);

          if (!(options & X86Inst::kOptionModMR))
            goto EmitX86R;

          opCode -= 2;
          Utils::swap(opReg, rbReg);
          goto EmitX86R;
        }
        else {
          opCode += 3;
          ADD_PREFIX_BY_SIZE(o0.getSize());

          if (!(options & X86Inst::kOptionModMR))
            goto EmitX86R;

          opCode -= 2;
          Utils::swap(opReg, rbReg);
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, opReg);
          opCode += 2;
          goto EmitX86M;
        }
        else {
          opCode += 3;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, opReg);
          goto EmitX86M;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o1.getSize());
          goto EmitX86M;
        }
      }

      // The remaining instructions use 0x80 opcode.
      opCode = 0x80;

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        uint32_t size = o0.getSize();

        rbReg = o0.getId();
        imVal = static_cast<const Imm&>(o1).getInt64();

        if (size == 1) {
          FIXUP_GPB(o0, rbReg);
          imLen = 1;
        }
        else {
          if (size == 2) {
            ADD_66H_P(1);
          }
          else if (size == 4) {
            // Sign extend so isInt8 returns the right result.
            imVal = x86SignExtend32To64(imVal);
          }
          else if (size == 8) {
            // In 64-bit mode it's not possible to use 64-bit immediate.
            if (Utils::isUInt32(imVal)) {
              // Zero-extend `and` by using a 32-bit GPD destination instead of a 64-bit GPQ.
              if (instId == X86Inst::kIdAnd)
                size = 4;
              else if (!Utils::isInt32(imVal))
                goto InvalidImmediate;
            }
            ADD_REX_W_BY_SIZE(size);
          }

          imLen = std::min<uint32_t>(size, 4);
          if (Utils::isInt8(imVal) && !(options & X86Inst::kOptionLongForm))
            imLen = 1;
        }

        // Alternate Form - AL, AX, EAX, RAX.
        if (rbReg == 0 && (size == 1 || imLen != 1) && !(options & X86Inst::kOptionLongForm)) {
          opCode &= X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_W;
          opCode |= ((opReg << 3) | (0x04 + (size != 1)));
          imLen = std::min<uint32_t>(size, 4);
          goto EmitX86Op;
        }

        opCode += size != 1 ? (imLen != 1 ? 1 : 3) : 0;
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        uint32_t memSize = o0.getSize();

        if (ASMJIT_UNLIKELY(memSize == 0))
          goto AmbiguousOperandSize;

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = std::min<uint32_t>(memSize, 4);

        // Sign extend so isInt8 returns the right result.
        if (memSize == 4)
          imVal = x86SignExtend32To64(imVal);

        if (Utils::isInt8(imVal) && !(options & X86Inst::kOptionLongForm))
          imLen = 1;

        opCode += memSize != 1 ? (imLen != 1 ? 1 : 3) : 0;
        ADD_PREFIX_BY_SIZE(memSize);

        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Bswap:
      if (isign3 == ENC_OPS1(Reg)) {
        if (ASMJIT_UNLIKELY(o0.getSize() < 4))
          goto InvalidInstruction;

        opReg = o0.getId();
        ADD_REX_W_BY_SIZE(o0.getSize());
        goto EmitX86OpReg;
      }
      break;

    case X86Inst::kEncodingX86Bt:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }

      // The remaining instructions use the secondary opcode/r.
      imVal = static_cast<const Imm&>(o1).getInt64();
      imLen = 1;

      opCode = commonData->getAltOpCode();
      opReg = x86ExtractO(opCode);
      ADD_PREFIX_BY_SIZE(o0.getSize());

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto AmbiguousOperandSize;

        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Call:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      rmRel = &o0;
      if (isign3 == ENC_OPS1(Mem))
        goto EmitX86M;

      // Call with 32-bit displacement use 0xE8 opcode. Call with 8-bit
      // displacement is not encodable so the alternative opcode field
      // in X86DB must be zero.
      opCode = 0xE8;
      opReg = 0;
      goto EmitJmpCall;

    case X86Inst::kEncodingX86Cmpxchg: {
      // Convert explicit to implicit.
      if (isign3 & (0x7 << 6)) {
        if (!X86Reg::isGp(o2) || o2.getId() != X86Gp::kIdAx)
          goto InvalidInstruction;
        isign3 &= 0x3F;
      }

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (o0.getSize() != o1.getSize())
          goto OperandSizeMismatch;

        rbReg = o0.getId();
        opReg = o1.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          FIXUP_GPB(o1, opReg);
          goto EmitX86R;
        }
        else {
          ADD_PREFIX_BY_SIZE(o0.getSize());
          opCode++;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;

        if (o1.getSize() == 1) {
          FIXUP_GPB(o0, opReg);
          goto EmitX86M;
        }
        else {
          ADD_PREFIX_BY_SIZE(o1.getSize());
          opCode++;
          goto EmitX86M;
        }
      }
      break;
    }

    case X86Inst::kEncodingX86Crc:
      opReg = o0.getId();
      ADD_REX_W_BY_SIZE(o0.getSize());

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o1.getId();

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }
        else {
          // This seems to be the only exception of encoding 66F2 PP prefix.
          if (o1.getSize() == 2) EMIT_BYTE(0x66);

          opCode++;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmRel = &o1;
        if (o1.getSize() == 0)
          goto AmbiguousOperandSize;

        // This seems to be the only exception of encoding 66F2 PP prefix.
        if (o1.getSize() == 2) EMIT_BYTE(0x66);

        opCode += o1.getSize() != 1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Enter:
      if (isign3 == ENC_OPS2(Imm, Imm)) {
        uint32_t iw = static_cast<const Imm&>(o0).getUInt16();
        uint32_t ib = static_cast<const Imm&>(o1).getUInt8();

        imVal = iw | (ib << 16);
        imLen = 3;
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86Imul:
      // First process all forms distinct of `kEncodingX86M_OptB_MulDiv`.
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opCode = 0x6B;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        if (!Utils::isInt8(imVal) || (options & X86Inst::kOptionLongForm)) {
          opCode -= 2;
          imLen = o0.getSize() == 2 ? 2 : 4;
        }

        opReg = o0.getId();
        rbReg = o1.getId();

        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opCode = 0x6B;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        // Sign extend so isInt8 returns the right result.
        if (o0.getSize() == 4)
          imVal = x86SignExtend32To64(imVal);

        if (!Utils::isInt8(imVal) || (options & X86Inst::kOptionLongForm)) {
          opCode -= 2;
          imLen = o0.getSize() == 2 ? 2 : 4;
        }

        opReg = o0.getId();
        rmRel = &o1;

        goto EmitX86M;
      }

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        // Must be explicit 'ax, r8' form.
        if (o1.getSize() == 1)
          goto CaseX86M_GPB_MulDiv;

        if (o0.getSize() != o1.getSize())
          goto OperandSizeMismatch;

        opReg = o0.getId();
        rbReg = o1.getId();

        opCode = X86Inst::kOpCode_MM_0F | 0xAF;
        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        // Must be explicit 'ax, m8' form.
        if (o1.getSize() == 1)
          goto CaseX86M_GPB_MulDiv;

        opReg = o0.getId();
        rmRel = &o1;

        opCode = X86Inst::kOpCode_MM_0F | 0xAF;
        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }

      // Shorthand to imul 'reg, reg, imm'.
      if (isign3 == ENC_OPS2(Reg, Imm)) {
        opCode = 0x6B;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = 1;

        // Sign extend so isInt8 returns the right result.
        if (o0.getSize() == 4)
          imVal = x86SignExtend32To64(imVal);

        if (!Utils::isInt8(imVal) || (options & X86Inst::kOptionLongForm)) {
          opCode -= 2;
          imLen = o0.getSize() == 2 ? 2 : 4;
        }

        opReg = rbReg = o0.getId();
        goto EmitX86R;
      }

      // Try implicit form.
      goto CaseX86M_GPB_MulDiv;

    case X86Inst::kEncodingX86In:
      if (isign3 == ENC_OPS2(Reg, Imm)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdAx))
          goto InvalidInstruction;

        imVal = o1.as<Imm>().getUInt8();
        imLen = 1;

        opCode = commonData->getAltOpCode() + (o0.getSize() != 1);
        ADD_66H_P_BY_SIZE(o0.getSize());
        goto EmitX86Op;
      }

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdAx || o1.getId() != X86Gp::kIdDx))
          goto InvalidInstruction;

        opCode += o0.getSize() != 1;
        ADD_66H_P_BY_SIZE(o0.getSize());
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86Ins:
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        if (ASMJIT_UNLIKELY(!x86IsImplicitMem(o0, X86Gp::kIdDi) || o1.getId() != X86Gp::kIdDx))
          goto InvalidInstruction;

        uint32_t size = o0.getSize();
        if (ASMJIT_UNLIKELY(size == 0))
          goto AmbiguousOperandSize;

        rmRel = &o0;
        opCode += (size != 1);

        ADD_66H_P_BY_SIZE(size);
        goto EmitX86OpImplicitMem;
      }
      break;

    case X86Inst::kEncodingX86IncDec:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          goto EmitX86R;
        }

        if (is32Bit()) {
          // INC r16|r32 is only encodable in 32-bit mode (collides with REX).
          opCode = commonData->getAltOpCode() + (rbReg & 0x07);
          ADD_66H_P_BY_SIZE(o0.getSize());
          goto EmitX86Op;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
        rmRel = &o0;
        opCode += o0.getSize() != 1;

        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Int:
      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        imLen = 1;
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86Jcc:
      if (_globalHints & CodeEmitter::kHintPredictedJumps) {
        if (options & X86Inst::kOptionTaken)
          EMIT_BYTE(0x3E);
        if (options & X86Inst::kOptionNotTaken)
          EMIT_BYTE(0x2E);
      }

      rmRel = &o0;
      opReg = 0;
      goto EmitJmpCall;

    case X86Inst::kEncodingX86JecxzLoop:
      rmRel = &o0;
      // Explicit jecxz|loop [r|e]cx, dst
      if (o0.isReg()) {
        if (ASMJIT_UNLIKELY(!X86Reg::isGp(o0, X86Gp::kIdCx)))
          goto InvalidInstruction;

        if ((is32Bit() && o0.getSize() == 2) || (is64Bit() && o0.getSize() == 4))
          EMIT_BYTE(0x67);

        rmRel = &o1;
      }

      opReg = 0;
      goto EmitJmpCall;

    case X86Inst::kEncodingX86Jmp:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      rmRel = &o0;
      if (isign3 == ENC_OPS1(Mem))
        goto EmitX86M;

      // Jump encoded with 32-bit displacement use 0xE9 opcode. Jump encoded
      // with 8-bit displacement's opcode is stored as an alternative opcode.
      opCode = 0xE9;
      opReg = 0;
      goto EmitJmpCall;

    case X86Inst::kEncodingX86JmpRel:
      rmRel = &o0;
      goto EmitJmpCall;

    case X86Inst::kEncodingX86Lea:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        ADD_PREFIX_BY_SIZE(o0.getSize());
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Mov:
      // Reg <- Reg
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // Asmjit uses segment registers indexed from 1 to 6, leaving zero as
        // "no segment register used". We have to fix this (decrement the index
        // of the register) when emitting MOV instructions which move to/from
        // a segment register. The segment register is always `opReg`, because
        // the MOV instruction uses either RM or MR encoding.

        // GP <- ??
        if (X86Reg::isGp(o0)) {
          // GP <- GP
          if (X86Reg::isGp(o1)) {
            uint32_t size0 = o0.getSize();
            uint32_t size1 = o1.getSize();

            if (size0 != size1) {
              // We allow 'mov r64, r32' as it's basically zero-extend.
              if (size0 == 8 && size1 == 4)
                size0 = 4; // Zero extend, don't promote to 64-bit.
              else
                goto InvalidInstruction;
            }

            if (size0 == 1) {
              FIXUP_GPB(o0, opReg);
              FIXUP_GPB(o1, rbReg);
              opCode = 0x8A;

              if (!(options & X86Inst::kOptionModMR))
                goto EmitX86R;

              opCode -= 2;
              Utils::swap(opReg, rbReg);
              goto EmitX86R;
            }
            else {
              opCode = 0x8B;
              ADD_PREFIX_BY_SIZE(size0);

              if (!(options & X86Inst::kOptionModMR))
                goto EmitX86R;

              opCode -= 2;
              Utils::swap(opReg, rbReg);
              goto EmitX86R;
            }
          }

          opReg = rbReg;
          rbReg = o0.getId();

          // GP <- SEG
          if (X86Reg::isSeg(o1)) {
            opCode = 0x8C;
            opReg--;
            ADD_PREFIX_BY_SIZE(o0.getSize());
            goto EmitX86R;
          }

          // GP <- CR
          if (X86Reg::isCr(o1)) {
            opCode = 0x20 | X86Inst::kOpCode_MM_0F;

            // Use `LOCK MOV` in 32-bit mode if CR8+ register is accessed (AMD extension).
            if ((opReg & 0x8) && is32Bit()) {
              EMIT_BYTE(0xF0);
              opReg &= 0x7;
            }
            goto EmitX86R;
          }

          // GP <- DR
          if (X86Reg::isDr(o1)) {
            opCode = 0x21 | X86Inst::kOpCode_MM_0F;
            goto EmitX86R;
          }
        }
        else {
          // ?? <- GP
          if (!X86Reg::isGp(o1))
            goto InvalidInstruction;

          // SEG <- GP
          if (X86Reg::isSeg(o0)) {
            opCode = 0x8E;
            opReg--;
            ADD_PREFIX_BY_SIZE(o1.getSize());
            goto EmitX86R;
          }

          // CR <- GP
          if (X86Reg::isCr(o0)) {
            opCode = 0x22 | X86Inst::kOpCode_MM_0F;

            // Use `LOCK MOV` in 32-bit mode if CR8+ register is accessed (AMD extension).
            if ((opReg & 0x8) && is32Bit()) {
              EMIT_BYTE(0xF0);
              opReg &= 0x7;
            }
            goto EmitX86R;
          }

          // DR <- GP
          if (X86Reg::isDr(o0)) {
            opCode = 0x23 | X86Inst::kOpCode_MM_0F;
            goto EmitX86R;
          }
        }

        goto InvalidInstruction;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;

        // SEG <- Mem
        if (X86Reg::isSeg(o0)) {
          opCode = 0x8E;
          opReg--;
          ADD_PREFIX_BY_SIZE(o1.getSize());
          goto EmitX86M;
        }
        // Reg <- Mem
        else {
          if (o0.getSize() == 1) {
            opCode = 0;
            FIXUP_GPB(o0, opReg);
          }
          else {
            opCode = 1;
            ADD_PREFIX_BY_SIZE(o0.getSize());
          }

          // Handle a special form `mov al|ax|eax|rax, [ptr64]` that doesn't use MOD.
          if (o0.getId() == X86Gp::kIdAx && !rmRel->as<X86Mem>().hasBaseOrIndex()) {
            imVal = rmRel->as<X86Mem>().getOffset();
            if (!is64Bit() || (is64Bit() && ((options & X86Inst::kOptionLongForm) || !Utils::isInt32(imVal)))) {
              opCode += 0xA0;
              goto EmitX86OpMovAbs;
            }
          }

          opCode += 0x8A;
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;

        // Mem <- SEG
        if (X86Reg::isSeg(o1)) {
          opCode = 0x8C;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }
        // Mem <- Reg
        else {
          if (o1.getSize() == 1) {
            opCode = 0;
            FIXUP_GPB(o1, opReg);
          }
          else {
            opCode = 1;
            ADD_PREFIX_BY_SIZE(o1.getSize());
          }

          // Handle a special form `mov [ptr64], al|ax|eax|rax` that doesn't use MOD.
          if (o1.getId() == X86Gp::kIdAx && !rmRel->as<X86Mem>().hasBaseOrIndex()) {
            imVal = rmRel->as<X86Mem>().getOffset();
            if (!is64Bit() || (is64Bit() && ((options & X86Inst::kOptionLongForm) || !Utils::isInt32(imVal)))) {
              opCode += 0xA2;
              goto EmitX86OpMovAbs;
            }
          }

          opCode += 0x88;
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        opReg = o0.getId();
        imLen = o0.getSize();

        if (imLen == 1) {
          FIXUP_GPB(o0, opReg);

          imVal = static_cast<const Imm&>(o1).getUInt8();
          opCode = 0xB0;
          goto EmitX86OpReg;
        }
        else {
          // 64-bit immediate in 64-bit mode is allowed.
          imVal = static_cast<const Imm&>(o1).getInt64();

          // Optimize the instruction size by using a 32-bit immediate if possible.
          if (imLen == 8 && !(options & X86Inst::kOptionLongForm)) {
            if (Utils::isUInt32(imVal)) {
              // Zero-extend by using a 32-bit GPD destination instead of a 64-bit GPQ.
              imLen = 4;
            }
            else if (Utils::isInt32(imVal)) {
              // Sign-extend, uses 'C7 /0' opcode.
              rbReg = opReg;

              opCode = 0xC7 | X86Inst::kOpCode_W;
              opReg = 0;

              imLen = 4;
              goto EmitX86R;
            }
          }

          opCode = 0xB8;
          ADD_PREFIX_BY_SIZE(imLen);
          goto EmitX86OpReg;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        uint32_t memSize = o0.getSize();

        if (ASMJIT_UNLIKELY(memSize == 0))
          goto AmbiguousOperandSize;

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = std::min<uint32_t>(memSize, 4);

        opCode = 0xC6 + (memSize != 1);
        opReg = 0;
        ADD_PREFIX_BY_SIZE(memSize);

        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86MovsxMovzx:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        ADD_PREFIX_BY_SIZE(o0.getSize());

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opCode += o1.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Out:
      if (isign3 == ENC_OPS2(Imm, Reg)) {
        if (ASMJIT_UNLIKELY(o1.getId() != X86Gp::kIdAx))
          goto InvalidInstruction;

        imVal = o0.as<Imm>().getUInt8();
        imLen = 1;

        opCode = commonData->getAltOpCode() + (o1.getSize() != 1);
        ADD_66H_P_BY_SIZE(o1.getSize());
        goto EmitX86Op;
      }

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdDx || o1.getId() != X86Gp::kIdAx))
          goto InvalidInstruction;

        opCode += o1.getSize() != 1;
        ADD_66H_P_BY_SIZE(o1.getSize());
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86Outs:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdDx || !x86IsImplicitMem(o1, X86Gp::kIdSi)))
          goto InvalidInstruction;

        uint32_t size = o1.getSize();
        if (ASMJIT_UNLIKELY(size == 0))
          goto AmbiguousOperandSize;

        rmRel = &o1;
        opCode += (size != 1);

        ADD_66H_P_BY_SIZE(size);
        goto EmitX86OpImplicitMem;
      }
      break;

    case X86Inst::kEncodingX86Push:
      if (isign3 == ENC_OPS1(Reg)) {
        if (X86Reg::isSeg(o0)) {
          uint32_t segment = o0.getId();
          if (ASMJIT_UNLIKELY(segment >= X86Seg::kIdCount))
            goto InvalidSegment;

          if (segment >= X86Seg::kIdFs)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(x86OpCodePushSeg[segment]);
          goto EmitDone;
        }
        else {
          goto CaseX86Pop_Gp;
        }
      }

      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        imLen = 4;

        if (Utils::isInt8(imVal) && !(options & X86Inst::kOptionLongForm))
          imLen = 1;

        opCode = imLen == 1 ? 0x6A : 0x68;
        goto EmitX86Op;
      }
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Pop:
      if (isign3 == ENC_OPS1(Reg)) {
        if (X86Reg::isSeg(o0)) {
          uint32_t segment = o0.getId();
          if (ASMJIT_UNLIKELY(segment == X86Seg::kIdCs || segment >= X86Seg::kIdCount))
            goto InvalidSegment;

          if (segment >= X86Seg::kIdFs)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(x86OpCodePopSeg[segment]);
          goto EmitDone;
        }
        else {
CaseX86Pop_Gp:
          // We allow 2 byte, 4 byte, and 8 byte register sizes, although PUSH
          // and POP only allow 2 bytes or native size. On 64-bit we simply
          // PUSH/POP 64-bit register even if 32-bit register was given.
          if (ASMJIT_UNLIKELY(o0.getSize() < 2))
            goto InvalidInstruction;

          opCode = commonData->getAltOpCode();
          opReg = o0.getId();

          ADD_66H_P_BY_SIZE(o0.getSize());
          goto EmitX86OpReg;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto AmbiguousOperandSize;

        if (ASMJIT_UNLIKELY(o0.getSize() != 2 && o0.getSize() != getGpSize()))
          goto InvalidInstruction;

        ADD_66H_P_BY_SIZE(o0.getSize());
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Ret:
      if (isign3 == 0) {
        // 'ret' without immediate, change C2 to C3.
        opCode++;
        goto EmitX86Op;
      }

      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        if (imVal == 0 && !(options & X86Inst::kOptionLongForm)) {
          // 'ret' without immediate, change C2 to C3.
          opCode++;
          goto EmitX86Op;
        }
        else {
          imLen = 2;
          goto EmitX86Op;
        }
      }
      break;

    case X86Inst::kEncodingX86Rot:
      if (o0.isReg()) {
        rbReg = o0.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
        }

        if (isign3 == ENC_OPS2(Reg, Reg)) {
          if (ASMJIT_UNLIKELY(o1.getId() != X86Gp::kIdCx))
            goto InvalidInstruction;

          opCode += 2;
          goto EmitX86R;
        }

        if (isign3 == ENC_OPS2(Reg, Imm)) {
          imVal = static_cast<const Imm&>(o1).getInt64() & 0xFF;
          imLen = 0;

          if (imVal == 1 && !(options & X86Inst::kOptionLongForm))
            goto EmitX86R;

          imLen = 1;
          opCode -= 0x10;
          goto EmitX86R;
        }
      }
      else {
        opCode += o0.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        if (isign3 == ENC_OPS2(Mem, Reg)) {
          if (ASMJIT_UNLIKELY(o1.getId() != X86Gp::kIdCx))
            goto InvalidInstruction;

          opCode += 2;
          rmRel = &o0;
          goto EmitX86M;
        }

        if (isign3 == ENC_OPS2(Mem, Imm)) {
          if (ASMJIT_UNLIKELY(o0.getSize() == 0))
            goto AmbiguousOperandSize;

          imVal = static_cast<const Imm&>(o1).getInt64() & 0xFF;
          imLen = 0;
          rmRel = &o0;

          if (imVal == 1 && !(options & X86Inst::kOptionLongForm))
            goto EmitX86M;

          imLen = 1;
          opCode -= 0x10;
          goto EmitX86M;
        }
      }
      break;

    case X86Inst::kEncodingX86Set:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        FIXUP_GPB(o0, rbReg);
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86ShldShrd:
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_PREFIX_BY_SIZE(o0.getSize());
        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        ADD_PREFIX_BY_SIZE(o1.getSize());
        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }

      // The following instructions use opCode + 1.
      opCode++;

      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        if (ASMJIT_UNLIKELY(o2.getId() != X86Gp::kIdCx))
          goto InvalidInstruction;

        ADD_PREFIX_BY_SIZE(o0.getSize());
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Reg)) {
        if (ASMJIT_UNLIKELY(o2.getId() != X86Gp::kIdCx))
          goto InvalidInstruction;

        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86StrRm:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmRel = &o1;
        if (ASMJIT_UNLIKELY(rmRel->as<X86Mem>().getOffsetLo32() || !X86Reg::isGp(o0.as<X86Reg>(), X86Gp::kIdAx)))
          goto InvalidInstruction;

        uint32_t size = o0.getSize();
        if (o1.hasSize() && ASMJIT_UNLIKELY(o1.getSize() != size))
          goto OperandSizeMismatch;

        ADD_PREFIX_BY_SIZE(size);
        opCode += static_cast<uint32_t>(size != 1);

        goto EmitX86OpImplicitMem;
      }
      break;

    case X86Inst::kEncodingX86StrMr:
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        rmRel = &o0;
        if (ASMJIT_UNLIKELY(rmRel->as<X86Mem>().getOffsetLo32() || !X86Reg::isGp(o1.as<X86Reg>(), X86Gp::kIdAx)))
          goto InvalidInstruction;

        uint32_t size = o1.getSize();
        if (o0.hasSize() && ASMJIT_UNLIKELY(o0.getSize() != size))
          goto OperandSizeMismatch;

        ADD_PREFIX_BY_SIZE(size);
        opCode += static_cast<uint32_t>(size != 1);

        goto EmitX86OpImplicitMem;
      }
      break;

    case X86Inst::kEncodingX86StrMm:
      if (isign3 == ENC_OPS2(Mem, Mem)) {
        if (ASMJIT_UNLIKELY(o0.as<X86Mem>().getBaseIndexType() !=
                            o1.as<X86Mem>().getBaseIndexType()))
          goto InvalidInstruction;

        rmRel = &o1;
        if (ASMJIT_UNLIKELY(o0.as<X86Mem>().hasOffset()))
          goto InvalidInstruction;

        uint32_t size = o1.getSize();
        if (ASMJIT_UNLIKELY(size == 0))
          goto AmbiguousOperandSize;

        if (ASMJIT_UNLIKELY(o0.getSize() != size))
          goto OperandSizeMismatch;

        ADD_PREFIX_BY_SIZE(size);
        opCode += static_cast<uint32_t>(size != 1);

        goto EmitX86OpImplicitMem;
      }
      break;

    case X86Inst::kEncodingX86Test:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (o0.getSize() != o1.getSize())
          goto OperandSizeMismatch;

        rbReg = o0.getId();
        opReg = o1.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          FIXUP_GPB(o1, opReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, opReg);
          goto EmitX86M;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o1.getSize());
          goto EmitX86M;
        }
      }

      // The following instructions use the secondary opcode.
      opCode = commonData->getAltOpCode();
      opReg = x86ExtractO(opCode);

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        rbReg = o0.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);

          imVal = static_cast<const Imm&>(o1).getUInt8();
          imLen = 1;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());

          imVal = static_cast<const Imm&>(o1).getInt64();
          imLen = std::min<uint32_t>(o0.getSize(), 4);
        }

        // Alternate Form - AL, AX, EAX, RAX.
        if (o0.getId() == 0 && !(options & X86Inst::kOptionLongForm)) {
          opCode &= X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_W;
          opCode |= 0xA8 + (o0.getSize() != 1);
          goto EmitX86Op;
        }

        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto AmbiguousOperandSize;

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = std::min<uint32_t>(o0.getSize(), 4);

        opCode += (o0.getSize() != 1);
        ADD_PREFIX_BY_SIZE(o0.getSize());

        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Xchg:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, opReg);
          goto EmitX86M;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }
      }
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Xadd:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o0.getId();
        opReg = o1.getId();

        if (o0.getSize() != o1.getSize())
          goto OperandSizeMismatch;

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          FIXUP_GPB(o1, opReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());

          // Special opcode for 'xchg ?ax, reg'.
          if (instId == X86Inst::kIdXchg && (opReg == 0 || rbReg == 0)) {
            opCode &= X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_W;
            opCode |= 0x90;
            // One of `xchg a, b` or `xchg b, a` is AX/EAX/RAX.
            opReg += rbReg;
            goto EmitX86OpReg;
          }
          else {
            goto EmitX86R;
          }
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode += o1.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o1.getSize());

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Fence:
      rbReg = 0;
      goto EmitX86R;

    case X86Inst::kEncodingX86Bndmov:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // ModRM encoding:
        if (!(options & X86Inst::kOptionModMR))
          goto EmitX86R;

        // ModMR encoding:
        opCode = commonData->getAltOpCode();
        std::swap(opReg, rbReg);
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode = commonData->getAltOpCode();

        rmRel = &o0;
        opReg = o1.getId();
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [FPU]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingFpuOp:
      goto EmitFpuOp;

    case X86Inst::kEncodingFpuArith:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // We switch to the alternative opcode if the first operand is zero.
        if (opReg == 0) {
CaseFpuArith_Reg:
          opCode = ((0xD8   << X86Inst::kOpCode_FPU_2B_Shift)       ) +
                   ((opCode >> X86Inst::kOpCode_FPU_2B_Shift) & 0xFF) + rbReg;
          goto EmitFpuOp;
        }
        else if (rbReg == 0) {
          rbReg = opReg;
          opCode = ((0xDC   << X86Inst::kOpCode_FPU_2B_Shift)       ) +
                   ((opCode                                 ) & 0xFF) + rbReg;
          goto EmitFpuOp;
        }
        else {
          goto InvalidInstruction;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
CaseFpuArith_Mem:
        // 0xD8/0xDC, depends on the size of the memory operand; opReg is valid.
        opCode = (o0.getSize() == 4) ? 0xD8 : 0xDC;
        // Clear compressed displacement before going to EmitX86M.
        opCode &= ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);

        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingFpuCom:
      if (isign3 == 0) {
        rbReg = 1;
        goto CaseFpuArith_Reg;
      }

      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto CaseFpuArith_Reg;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        goto CaseFpuArith_Mem;
      }
      break;

    case X86Inst::kEncodingFpuFldFst:
      if (isign3 == ENC_OPS1(Mem)) {
        rmRel = &o0;

        if (o0.getSize() == 4 && commonData->hasFlag(X86Inst::kFlagFpuM32)) {
          goto EmitX86M;
        }

        if (o0.getSize() == 8 && commonData->hasFlag(X86Inst::kFlagFpuM64)) {
          opCode += 4;
          goto EmitX86M;
        }

        if (o0.getSize() == 10 && commonData->hasFlag(X86Inst::kFlagFpuM80)) {
          opCode = commonData->getAltOpCode();
          opReg  = x86ExtractO(opCode);
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS1(Reg)) {
        if (instId == X86Inst::kIdFld ) { opCode = (0xD9 << X86Inst::kOpCode_FPU_2B_Shift) + 0xC0 + o0.getId(); goto EmitFpuOp; }
        if (instId == X86Inst::kIdFst ) { opCode = (0xDD << X86Inst::kOpCode_FPU_2B_Shift) + 0xD0 + o0.getId(); goto EmitFpuOp; }
        if (instId == X86Inst::kIdFstp) { opCode = (0xDD << X86Inst::kOpCode_FPU_2B_Shift) + 0xD8 + o0.getId(); goto EmitFpuOp; }
      }
      break;

    case X86Inst::kEncodingFpuM:
      if (isign3 == ENC_OPS1(Mem)) {
        // Clear compressed displacement before going to EmitX86M.
        opCode &= ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);

        rmRel = &o0;
        if (o0.getSize() == 2 && commonData->hasFlag(X86Inst::kFlagFpuM16)) {
          opCode += 4;
          goto EmitX86M;
        }

        if (o0.getSize() == 4 && commonData->hasFlag(X86Inst::kFlagFpuM32)) {
          goto EmitX86M;
        }

        if (o0.getSize() == 8 && commonData->hasFlag(X86Inst::kFlagFpuM64)) {
          opCode = commonData->getAltOpCode() & ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);
          opReg  = x86ExtractO(opCode);
          goto EmitX86M;
        }
      }
      break;

    case X86Inst::kEncodingFpuRDef:
      if (isign3 == 0) {
        opCode += 1;
        goto EmitFpuOp;
      }
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingFpuR:
      if (isign3 == ENC_OPS1(Reg)) {
        opCode += o0.getId();
        goto EmitFpuOp;
      }
      break;

    case X86Inst::kEncodingFpuStsw:
      if (isign3 == ENC_OPS1(Reg)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdAx))
          goto InvalidInstruction;

        opCode = commonData->getAltOpCode();
        goto EmitFpuOp;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        // Clear compressed displacement before going to EmitX86M.
        opCode &= ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);

        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Ext]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingExtPextrw:
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_66H_P(X86Reg::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        // Secondary opcode of 'pextrw' instruction (SSE4.1).
        opCode = commonData->getAltOpCode();
        ADD_66H_P(X86Reg::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtExtract:
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_66H_P(X86Reg::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        ADD_66H_P(X86Reg::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMov:
      // GP|MMX|XMM <- GP|MMX|XMM
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        if (!(options & X86Inst::kOptionModMR) || !commonData->hasAltOpCode())
          goto EmitX86R;

        opCode = commonData->getAltOpCode();
        Utils::swap(opReg, rbReg);
        goto EmitX86R;
      }

      // GP|MMX|XMM <- Mem
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }

      // The following instruction uses opCode[1].
      opCode = commonData->getAltOpCode();

      // Mem <- GP|MMX|XMM
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovnti:
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        ADD_REX_W(X86Reg::isGpq(o1));

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovbe:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        if (o0.getSize() == 1)
          goto InvalidInstruction;

        ADD_PREFIX_BY_SIZE(o0.getSize());
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = commonData->getAltOpCode();

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        if (o1.getSize() == 1)
          goto InvalidInstruction;

        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovd:
CaseExtMovd:
      opReg = o0.getId();
      ADD_66H_P(X86Reg::isXmm(o0));

      // MMX/XMM <- Gp
      if (isign3 == ENC_OPS2(Reg, Reg) && X86Reg::isGp(o1)) {
        rbReg = o1.getId();
        goto EmitX86R;
      }

      // MMX/XMM <- Mem
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmRel = &o1;
        goto EmitX86M;
      }

      // The following instructions use the secondary opcode.
      opCode &= X86Inst::kOpCode_W;
      opCode |= commonData->getAltOpCode();
      opReg = o1.getId();
      ADD_66H_P(X86Reg::isXmm(o1));

      // GP <- MMX/XMM
      if (isign3 == ENC_OPS2(Reg, Reg) && X86Reg::isGp(o0)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      // Mem <- MMX/XMM
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        rmRel = &o0;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovq:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // MMX <- MMX
        if (X86Reg::isMm(o0) && X86Reg::isMm(o1)) {
          opCode = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F | 0x6F;

          if (!(options & X86Inst::kOptionModMR))
            goto EmitX86R;

          opCode += 0x10;
          Utils::swap(opReg, rbReg);
          goto EmitX86R;
        }

        // XMM <- XMM
        if (X86Reg::isXmm(o0) && X86Reg::isXmm(o1)) {
          opCode = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F | 0x7E;

          if (!(options & X86Inst::kOptionModMR))
            goto EmitX86R;

          opCode = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F | 0xD6;
          Utils::swap(opReg, rbReg);
          goto EmitX86R;
        }

        // MMX <- XMM (MOVDQ2Q)
        if (X86Reg::isMm(o0) && X86Reg::isXmm(o1)) {
          opCode = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F | 0xD6;
          goto EmitX86R;
        }

        // XMM <- MMX (MOVQ2DQ)
        if (X86Reg::isXmm(o0) && X86Reg::isMm(o1)) {
          opCode = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F | 0xD6;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;

        // MMX <- Mem
        if (X86Reg::isMm(o0)) {
          opCode = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F | 0x6F;
          goto EmitX86M;
        }

        // XMM <- Mem
        if (X86Reg::isXmm(o0)) {
          opCode = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F | 0x7E;
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;

        // Mem <- MMX
        if (X86Reg::isMm(o1)) {
          opCode = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F | 0x7F;
          goto EmitX86M;
        }

        // Mem <- XMM
        if (X86Reg::isXmm(o1)) {
          opCode = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F | 0xD6;
          goto EmitX86M;
        }
      }

      // MOVQ in other case is simply a MOVD instruction promoted to 64-bit.
      opCode |= X86Inst::kOpCode_W;
      goto CaseExtMovd;

    case X86Inst::kEncodingExtRm_XMM0:
      if (ASMJIT_UNLIKELY(!o2.isNone() && !X86Reg::isXmm(o2, 0)))
        goto InvalidInstruction;

      isign3 &= 0x3F;
      goto CaseExtRm;

    case X86Inst::kEncodingExtRm_ZDI:
      if (ASMJIT_UNLIKELY(!o2.isNone() && !x86IsImplicitMem(o2, X86Gp::kIdDi)))
        goto InvalidInstruction;

      isign3 &= 0x3F;
      goto CaseExtRm;

    case X86Inst::kEncodingExtRm_Wx:
      ADD_REX_W(X86Reg::isGpq(o0) || o1.getSize() == 8);
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingExtRm:
CaseExtRm:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtRm_P:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        ADD_66H_P(X86Reg::isXmm(o0) | X86Reg::isXmm(o1));

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        ADD_66H_P(X86Reg::isXmm(o0));

        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtRmRi:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = commonData->getAltOpCode();
      opReg  = x86ExtractO(opCode);

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = 1;

        rbReg = o0.getId();
        goto EmitX86R;
      }
      break;

    case X86Inst::kEncodingExtRmRi_P:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        ADD_66H_P(X86Reg::isXmm(o0) | X86Reg::isXmm(o1));

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        ADD_66H_P(X86Reg::isXmm(o0));

        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = commonData->getAltOpCode();
      opReg  = x86ExtractO(opCode);

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        ADD_66H_P(X86Reg::isXmm(o0));

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = 1;

        rbReg = o0.getId();
        goto EmitX86R;
      }
      break;

    case X86Inst::kEncodingExtRmi:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtRmi_P:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_66H_P(X86Reg::isXmm(o0) | X86Reg::isXmm(o1));

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        ADD_66H_P(X86Reg::isXmm(o0));

        opReg = o0.getId();
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Extrq / Insertq (SSE4A)]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingExtExtrq:
      opReg = o0.getId();
      rbReg = o1.getId();

      if (isign3 == ENC_OPS2(Reg, Reg))
        goto EmitX86R;

      // The following instruction uses the secondary opcode.
      opCode = commonData->getAltOpCode();

      if (isign3 == ENC_OPS3(Reg, Imm, Imm)) {
        imVal = (static_cast<const Imm&>(o1).getUInt32()     ) +
                (static_cast<const Imm&>(o2).getUInt32() << 8) ;
        imLen = 2;

        rbReg = x86ExtractO(opCode);
        goto EmitX86R;
      }
      break;

    case X86Inst::kEncodingExtInsertq: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      opReg = o0.getId();
      rbReg = o1.getId();

      if (isign4 == ENC_OPS2(Reg, Reg))
        goto EmitX86R;

      // The following instruction uses the secondary opcode.
      opCode = commonData->getAltOpCode();

      if (isign4 == ENC_OPS4(Reg, Reg, Imm, Imm)) {
        imVal = (static_cast<const Imm&>(o2).getUInt32()     ) +
                (static_cast<const Imm&>(o3).getUInt32() << 8) ;
        imLen = 2;
        goto EmitX86R;
      }
      break;
    }

    // ------------------------------------------------------------------------
    // [3dNow]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingExt3dNow:
      // Every 3dNow instruction starts with 0x0F0F and the actual opcode is
      // stored as 8-bit immediate.
      imVal = opCode & 0xFF;
      imLen = 1;

      opCode = X86Inst::kOpCode_MM_0F | 0x0F;
      opReg = o0.getId();

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmRel = &o1;
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [VEX/EVEX]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingVexOp:
      goto EmitVexEvexOp;

    case X86Inst::kEncodingVexKmov:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // Form 'k, reg'.
        if (X86Reg::isGp(o1)) {
          opCode = commonData->getAltOpCode();
          goto EmitVexEvexR;
        }

        // Form 'reg, k'.
        if (X86Reg::isGp(o0)) {
          opCode = commonData->getAltOpCode() + 1;
          goto EmitVexEvexR;
        }

        // Form 'k, k'.
        if (!(options & X86Inst::kOptionModMR))
          goto EmitVexEvexR;

        opCode++;
        Utils::swap(opReg, rbReg);
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;

        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;

        opCode++;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexM:
      if (isign3 == ENC_OPS1(Mem)) {
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexM_VM:
      if (isign3 == ENC_OPS1(Mem)) {
        opCode |= x86OpCodeLByVMem(o0);
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexMr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexMr_VM:
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode |= std::max(x86OpCodeLByVMem(o0), x86OpCodeLBySize(o1.getSize()));

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexMri_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexMri:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRm_ZDI:
      if (ASMJIT_UNLIKELY(!o2.isNone() && !x86IsImplicitMem(o2, X86Gp::kIdDi)))
        goto InvalidInstruction;

      isign3 &= 0x3F;
      goto CaseVexRm;

    case X86Inst::kEncodingVexRm_Wx:
      ADD_REX_W(X86Reg::isGpq(o0) | X86Reg::isGpq(o1));
      goto CaseVexRm;

    case X86Inst::kEncodingVexRm_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRm:
CaseVexRm:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRm_VM:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opCode |= std::max(x86OpCodeLByVMem(o1), x86OpCodeLBySize(o0.getSize()));
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRm_T1_4X: {
      if (!(options & kOptionOp4Op5Used))
        goto InvalidInstruction;

      if (X86Reg::isZmm(o0  ) && X86Reg::isZmm(o1) &&
          X86Reg::isZmm(o2  ) && X86Reg::isZmm(o3) &&
          X86Reg::isZmm(_op4) && _op5.isMem()) {

        // Registers [o1, o2, o3, _op4] must start aligned and must be consecutive.
        uint32_t i1 = o1.getId();
        uint32_t i2 = o2.getId();
        uint32_t i3 = o3.getId();
        uint32_t i4 = _op4.getId();

        if (ASMJIT_UNLIKELY((i1 & 0x3) != 0 || i2 != i1 + 1 || i3 != i1 + 2 || i4 != i1 + 3))
          goto NotConsecutiveRegs;

        opReg = o0.getId();
        rmRel = &_op5;
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRmi_Wx:
      ADD_REX_W(X86Reg::isGpq(o0) | X86Reg::isGpq(o1));
      goto CaseVexRmi;

    case X86Inst::kEncodingVexRmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmi:
CaseVexRmi:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvm:
CaseVexRvm:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
CaseVexRvm_R:
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvm_ZDX_Wx:
      if (ASMJIT_UNLIKELY(!o3.isNone() && !X86Reg::isGp(o3, X86Gp::kIdDx)))
        goto InvalidInstruction;
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvm_Wx:
      ADD_REX_W(X86Reg::isGpq(o0) | (o2.getSize() == 8));
      goto CaseVexRvm;

    case X86Inst::kEncodingVexRvm_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      goto CaseVexRvm;

    case X86Inst::kEncodingVexRvmr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmr: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      imVal = o3.getId() << 4;
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRvmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmi: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      imVal = static_cast<const Imm&>(o3).getInt64();
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRmv_Wx:
      ADD_REX_W(X86Reg::isGpq(o0) | X86Reg::isGpq(o2));
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmv:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRmvRm_VM:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opCode  = commonData->getAltOpCode();
        opCode |= std::max(x86OpCodeLByVMem(o1), x86OpCodeLBySize(o0.getSize()));

        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }

      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmv_VM:
      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opCode |= std::max(x86OpCodeLByVMem(o1), x86OpCodeLBySize(o0.getSize() | o2.getSize()));

        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;


    case X86Inst::kEncodingVexRmvi: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      imVal = static_cast<const Imm&>(o3).getInt64();
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Mem, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexMovdMovq:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (X86Reg::isGp(o0)) {
          opCode = commonData->getAltOpCode();
          ADD_REX_W_BY_SIZE(o0.getSize());
          opReg = o1.getId();
          rbReg = o0.getId();
          goto EmitVexEvexR;
        }

        if (X86Reg::isGp(o1)) {
          ADD_REX_W_BY_SIZE(o1.getSize());
          opReg = o0.getId();
          rbReg = o1.getId();
          goto EmitVexEvexR;
        }

        // If this is a 'W' version (movq) then allow also vmovq 'xmm|xmm' form.
        if (opCode & X86Inst::kOpCode_EW) {
          opCode &= ~(X86Inst::kOpCode_PP_VEXMask | X86Inst::kOpCode_MM_Mask | 0xFF);
          opCode |=  (X86Inst::kOpCode_PP_F3      | X86Inst::kOpCode_MM_0F   | 0x7E);

          opReg = o0.getId();
          rbReg = o1.getId();
          goto EmitVexEvexR;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        if (opCode & X86Inst::kOpCode_EW) {
          opCode &= ~(X86Inst::kOpCode_PP_VEXMask | X86Inst::kOpCode_MM_Mask | 0xFF);
          opCode |=  (X86Inst::kOpCode_PP_F3      | X86Inst::kOpCode_MM_0F   | 0x7E);
        }

        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode = commonData->getAltOpCode();

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        if (opCode & X86Inst::kOpCode_EW) {
          opCode &= ~(X86Inst::kOpCode_PP_VEXMask | X86Inst::kOpCode_MM_Mask | 0xFF);
          opCode |=  (X86Inst::kOpCode_PP_66      | X86Inst::kOpCode_MM_0F   | 0xD6);
        }

        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRmMr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmMr:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= X86Inst::kOpCode_LL_Mask;
      opCode |= commonData->getAltOpCode();

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmRmv:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();

        if (!(options & X86Inst::kOptionModMR))
          goto EmitVexEvexR;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        ADD_VEX_W(true);
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmRmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmRmi:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }

      // The following instructions use the secondary opcode.
      opCode &= X86Inst::kOpCode_LL_Mask;
      opCode |= commonData->getAltOpCode();

      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmRmvRmi:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();

        if (!(options & X86Inst::kOptionModMR))
          goto EmitVexEvexR;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        ADD_VEX_W(true);
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmRel = &o1;

        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      // The following instructions use the secondary opcode.
      opCode = commonData->getAltOpCode();

      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmMr:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }

      // The following instructions use the secondary opcode.
      opCode = commonData->getAltOpCode();

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmMvr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmMvr:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= X86Inst::kOpCode_LL_Mask;
      opCode |= commonData->getAltOpCode();

      if (isign3 == ENC_OPS3(Mem, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o2.getId(), o1.getId());
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmVmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmVmi:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= X86Inst::kOpCode_LL_Mask;
      opCode |= commonData->getAltOpCode();
      opReg = x86ExtractO(opCode);

      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexVm_Wx:
      ADD_REX_W(X86Reg::isGpq(o0) | X86Reg::isGpq(o1));
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexVm:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexEvexVmi_Lx:
      if (isign3 == ENC_OPS3(Reg, Mem, Imm))
        opCode |= X86Inst::kOpCode_MM_ForceEvex;
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexVmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexVmi:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rmRel = &o1;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvrmRvmr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvrmRvmr: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Mem)) {
        imVal = o2.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o3;

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;

        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRvrmiRvmri_Lx: {
      if (!(options & CodeEmitter::kOptionOp4Op5Used) || !_op4.isImm())
        goto InvalidInstruction;

      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize() | o2.getSize() | o3.getSize());

      imVal = static_cast<const Imm&>(_op4).getUInt8() & 0x0F;
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        imVal |= o3.getId() << 4;
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Mem)) {
        imVal |= o2.getId() << 4;
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o3;

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        imVal |= o3.getId() << 4;
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;

        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexMovssMovsd:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        goto CaseVexRvm_R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmRel = &o1;
        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode = commonData->getAltOpCode();
        opReg = o1.getId();
        rmRel = &o0;
        goto EmitVexEvexM;
      }
      break;

    // ------------------------------------------------------------------------
    // [FMA4]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingFma4_Lx:
      // It's fine to just check the first operand, second is just for sanity.
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingFma4: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Mem)) {
        imVal = o2.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o3;

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmRel = &o2;

        goto EmitVexEvexM;
      }
      break;
    }
  }
  goto InvalidInstruction;

  // --------------------------------------------------------------------------
  // [Emit - X86]
  // --------------------------------------------------------------------------

EmitX86OpMovAbs:
  imLen = getGpSize();

  // Segment-override prefix.
  if (rmRel->as<X86Mem>().hasSegment())
    EMIT_BYTE(x86SegmentPrefix[rmRel->as<X86Mem>().getSegmentId()]);

EmitX86Op:
  // Emit mandatory instruction prefix.
  EMIT_PP(opCode);

  // Emit REX prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options);
    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto InvalidRexPrefix;
      EMIT_BYTE(rex | kX86ByteRex);
    }
  }

  // Emit instruction opcodes.
  EMIT_MM_OP(opCode);

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86OpReg:
  // Emit mandatory instruction prefix.
  EMIT_PP(opCode);

  // Emit REX prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options) |
                   (opReg >> 3); // Rex.B (0x01).
    if (rex) {
      EMIT_BYTE(rex | kX86ByteRex);
      if (options & X86Inst::_kOptionInvalidRex)
        goto InvalidRexPrefix;
      opReg &= 0x7;
    }
  }

  // Emit instruction opcodes.
  opCode += opReg;
  EMIT_MM_OP(opCode);

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86OpImplicitMem:
  // NOTE: Don't change the emit order here, it's compatible with KeyStone/LLVM.
  rmInfo = x86MemInfo[rmRel->as<X86Mem>().getBaseIndexType()];
  if (ASMJIT_UNLIKELY(rmRel->as<X86Mem>().hasOffset() || (rmInfo & kX86MemInfo_Index)))
    goto InvalidInstruction;

  // Emit mandatory instruction prefix.
  EMIT_PP(opCode);

  // Emit REX prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options);
    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto InvalidRexPrefix;
      EMIT_BYTE(rex | kX86ByteRex);
    }
  }

  // Segment-override prefix.
  if (rmRel->as<X86Mem>().hasSegment())
    EMIT_BYTE(x86SegmentPrefix[rmRel->as<X86Mem>().getSegmentId()]);

  // Address-override prefix.
  if (rmInfo & _getAddressOverrideMask())
    EMIT_BYTE(0x67);

  // Emit instruction opcodes.
  EMIT_MM_OP(opCode);

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86R:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Rex prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options) |
                   ((opReg & 0x08) >> 1) | // REX.R (0x04).
                   ((rbReg       ) >> 3) ; // REX.B (0x01).
    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto InvalidRexPrefix;
      EMIT_BYTE(rex | kX86ByteRex);
      opReg &= 0x07;
      rbReg &= 0x07;
    }
  }

  // Instruction opcodes.
  EMIT_MM_OP(opCode);
  // ModR.
  EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86M:
  ASMJIT_ASSERT(rmRel != nullptr);
  ASMJIT_ASSERT(rmRel->getOp() == Operand::kOpMem);
  rmInfo = x86MemInfo[rmRel->as<X86Mem>().getBaseIndexType()];

  // GP instructions have never compressed displacement specified.
  ASMJIT_ASSERT((opCode & X86Inst::kOpCode_CDSHL_Mask) == 0);

  // Segment-override prefix.
  if (rmRel->as<X86Mem>().hasSegment())
    EMIT_BYTE(x86SegmentPrefix[rmRel->as<X86Mem>().getSegmentId()]);

  // Address-override prefix.
  if (rmInfo & _getAddressOverrideMask())
    EMIT_BYTE(0x67);

  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  rbReg = rmRel->as<X86Mem>().getBaseId();
  rxReg = rmRel->as<X86Mem>().getIndexId();

  // REX prefix (64-bit only).
  {
    uint32_t rex;

    rex  = (rbReg >> 3) & 0x01; // REX.B (0x01).
    rex |= (rxReg >> 2) & 0x02; // REX.X (0x02).
    rex |= (opReg >> 1) & 0x04; // REX.R (0x04).

    rex &= rmInfo;
    rex |= x86ExtractREX(opCode, options);

    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto InvalidRexPrefix;
      EMIT_BYTE(rex | kX86ByteRex);
      opReg &= 0x07;
    }
  }

  // Instruction opcodes.
  EMIT_MM_OP(opCode);
  // ... Fall through ...

  // --------------------------------------------------------------------------
  // [Emit - MOD/SIB]
  // --------------------------------------------------------------------------

EmitModSib:
  if (!(rmInfo & (kX86MemInfo_Index | kX86MemInfo_67H_X86))) {
    // ==========|> [BASE + DISP8|DISP32].
    if (rmInfo & kX86MemInfo_BaseGp) {
      rbReg &= 0x7;
      relOffset = rmRel->as<X86Mem>().getOffsetLo32();

      uint32_t mod = x86EncodeMod(0, opReg, rbReg);
      if (rbReg == X86Gp::kIdSp) {
        // [XSP|R12].
        if (relOffset == 0) {
          EMIT_BYTE(mod);
          EMIT_BYTE(x86EncodeSib(0, 4, 4));
        }
        // [XSP|R12 + DISP8|DISP32].
        else {
          uint32_t cdShift = (opCode & X86Inst::kOpCode_CDSHL_Mask) >> X86Inst::kOpCode_CDSHL_Shift;
          int32_t cdOffset = relOffset >> cdShift;

          if (Utils::isInt8(cdOffset) && relOffset == (cdOffset << cdShift)) {
            EMIT_BYTE(mod + 0x40); // <- MOD(1, opReg, rbReg).
            EMIT_BYTE(x86EncodeSib(0, 4, 4));
            EMIT_BYTE(cdOffset & 0xFF);
          }
          else {
            EMIT_BYTE(mod + 0x80); // <- MOD(2, opReg, rbReg).
            EMIT_BYTE(x86EncodeSib(0, 4, 4));
            EMIT_32(relOffset);
          }
        }
      }
      else if (rbReg != X86Gp::kIdBp && relOffset == 0) {
        // [BASE].
        EMIT_BYTE(mod);
      }
      else {
        // [BASE + DISP8|DISP32].
        uint32_t cdShift = (opCode & X86Inst::kOpCode_CDSHL_Mask) >> X86Inst::kOpCode_CDSHL_Shift;
        int32_t cdOffset = relOffset >> cdShift;

        if (Utils::isInt8(cdOffset) && relOffset == (cdOffset << cdShift)) {
          EMIT_BYTE(mod + 0x40);
          EMIT_BYTE(cdOffset & 0xFF);
        }
        else {
          EMIT_BYTE(mod + 0x80);
          EMIT_32(relOffset);
        }
      }
    }
    // ==========|> [ABSOLUTE | DISP32].
    else if (!(rmInfo & (kX86MemInfo_BaseLabel | kX86MemInfo_BaseRip))) {
      if (is32Bit()) {
        relOffset = rmRel->as<X86Mem>().getOffsetLo32();
        EMIT_BYTE(x86EncodeMod(0, opReg, 5));
        EMIT_32(relOffset);
      }
      else {
        uint64_t baseAddress = getCodeInfo().getBaseAddress();
        relOffset = rmRel->as<X86Mem>().getOffsetLo32();

        // Prefer absolute addressing mode if FS|GS segment override is present.
        bool absoluteValid = rmRel->as<X86Mem>().getOffsetHi32() == (relOffset >> 31);
        bool preferAbsolute = (rmRel->as<X86Mem>().getSegmentId() >= X86Seg::kIdFs) || rmRel->as<X86Mem>().isAbs();

        // If we know the base address and the memory operand points to an
        // absolute address it's possible to calculate REL32 that can be
        // be used as [RIP+REL32] in 64-bit mode.
        if (baseAddress != Globals::kNoBaseAddress && !preferAbsolute) {
          const uint32_t kModRel32Size = 5;
          uint64_t rip64 = baseAddress +
            static_cast<uint64_t>((uintptr_t)(cursor - _bufferData)) + imLen + kModRel32Size;

          uint64_t rel64 = static_cast<uint64_t>(rmRel->as<X86Mem>().getOffset()) - rip64;
          if (Utils::isInt32(static_cast<int64_t>(rel64))) {
            EMIT_BYTE(x86EncodeMod(0, opReg, 5));
            EMIT_32(static_cast<uint32_t>(rel64 & 0xFFFFFFFFU));
            if (imLen != 0)
              goto EmitImm;
            else
              goto EmitDone;
          }
        }

        if (ASMJIT_UNLIKELY(!absoluteValid))
          goto InvalidAddress64Bit;

        EMIT_BYTE(x86EncodeMod(0, opReg, 4));
        EMIT_BYTE(x86EncodeSib(0, 4, 5));
        EMIT_32(relOffset);
      }
    }
    // ==========|> [LABEL|RIP + DISP32]
    else {
      EMIT_BYTE(x86EncodeMod(0, opReg, 5));

      if (is32Bit()) {
EmitModSib_LabelRip_X86:
        if (ASMJIT_UNLIKELY(_code->_relocations.willGrow(&_code->_baseHeap) != kErrorOk))
          goto NoHeapMemory;

        relOffset = rmRel->as<X86Mem>().getOffsetLo32();
        if (rmInfo & kX86MemInfo_BaseLabel) {
          // [LABEL->ABS].
          label = _code->getLabelEntry(rmRel->as<X86Mem>().getBaseId());
          if (!label) goto InvalidLabel;

          err = _code->newRelocEntry(&re, RelocEntry::kTypeRelToAbs, 4);
          if (ASMJIT_UNLIKELY(err)) goto Failed;

          re->_sourceSectionId = _section->getId();
          re->_sourceOffset = static_cast<uint64_t>((uintptr_t)(cursor - _bufferData));
          re->_data = static_cast<int64_t>(relOffset);

          if (label->isBound()) {
            // Bound label.
            re->_data += static_cast<uint64_t>(label->getOffset());
            EMIT_32(0);
          }
          else {
            // Non-bound label.
            relOffset = -4 - imLen;
            relSize = 4;
            goto EmitRel;
          }
        }
        else {
          // [RIP->ABS].
          err = _code->newRelocEntry(&re, RelocEntry::kTypeRelToAbs, 4);
          if (ASMJIT_UNLIKELY(err)) goto Failed;

          re->_sourceSectionId = _section->getId();
          re->_sourceOffset = static_cast<uint64_t>((uintptr_t)(cursor - _bufferData));
          re->_data = re->_sourceOffset + static_cast<uint64_t>(static_cast<int64_t>(relOffset));
          EMIT_32(0);
        }
      }
      else {
        relOffset = rmRel->as<X86Mem>().getOffsetLo32();
        if (rmInfo & kX86MemInfo_BaseLabel) {
          // [RIP].
          label = _code->getLabelEntry(rmRel->as<X86Mem>().getBaseId());
          if (!label) goto InvalidLabel;

          relOffset -= (4 + imLen);
          if (label->isBound()) {
            // Bound label.
            relOffset += label->getOffset() - static_cast<int32_t>((intptr_t)(cursor - _bufferData));
            EMIT_32(static_cast<int32_t>(relOffset));
          }
          else {
            // Non-bound label.
            relSize = 4;
            goto EmitRel;
          }
        }
        else {
          // [RIP].
          EMIT_32(static_cast<int32_t>(relOffset));
        }
      }
    }
  }
  else if (!(rmInfo & kX86MemInfo_67H_X86)) {
    // ESP|RSP can't be used as INDEX in pure SIB mode, however, VSIB mode
    // allows XMM4|YMM4|ZMM4 (that's why the check is before the label).
    if (ASMJIT_UNLIKELY(rxReg == X86Gp::kIdSp)) goto InvalidAddressIndex;

EmitModVSib:
    rxReg &= 0x7;

    // ==========|> [BASE + INDEX + DISP8|DISP32].
    if (rmInfo & kX86MemInfo_BaseGp) {
      rbReg &= 0x7;
      relOffset = rmRel->as<X86Mem>().getOffsetLo32();

      uint32_t mod = x86EncodeMod(0, opReg, 4);
      uint32_t sib = x86EncodeSib(rmRel->as<X86Mem>().getShift(), rxReg, rbReg);

      if (relOffset == 0 && rbReg != X86Gp::kIdBp) {
        // [BASE + INDEX << SHIFT].
        EMIT_BYTE(mod);
        EMIT_BYTE(sib);
      }
      else {
        uint32_t cdShift = (opCode & X86Inst::kOpCode_CDSHL_Mask) >> X86Inst::kOpCode_CDSHL_Shift;
        int32_t cdOffset = relOffset >> cdShift;

        if (Utils::isInt8(cdOffset) && relOffset == (cdOffset << cdShift)) {
          // [BASE + INDEX << SHIFT + DISP8].
          EMIT_BYTE(mod + 0x40); // <- MOD(1, opReg, 4).
          EMIT_BYTE(sib);
          EMIT_BYTE(cdOffset);
        }
        else {
          // [BASE + INDEX << SHIFT + DISP32].
          EMIT_BYTE(mod + 0x80); // <- MOD(2, opReg, 4).
          EMIT_BYTE(sib);
          EMIT_32(relOffset);
        }
      }
    }
    // ==========|> [INDEX + DISP32].
    else if (!(rmInfo & (kX86MemInfo_BaseLabel | kX86MemInfo_BaseRip))) {
      // [INDEX << SHIFT + DISP32].
      EMIT_BYTE(x86EncodeMod(0, opReg, 4));
      EMIT_BYTE(x86EncodeSib(rmRel->as<X86Mem>().getShift(), rxReg, 5));

      relOffset = rmRel->as<X86Mem>().getOffsetLo32();
      EMIT_32(relOffset);
    }
    // ==========|> [LABEL|RIP + INDEX + DISP32].
    else {
      if (is32Bit()) {
        EMIT_BYTE(x86EncodeMod(0, opReg, 4));
        EMIT_BYTE(x86EncodeSib(rmRel->as<X86Mem>().getShift(), rxReg, 5));
        goto EmitModSib_LabelRip_X86;
      }
      else {
        // NOTE: This also handles VSIB+RIP, which is not allowed in 64-bit mode.
        goto InvalidAddress;
      }
    }
  }
  else {
    // 16-bit address mode (32-bit mode with 67 override prefix).
    relOffset = (static_cast<int32_t>(rmRel->as<X86Mem>().getOffsetLo32()) << 16) >> 16;

    // NOTE: 16-bit addresses don't use SIB byte and their encoding differs. We
    // use a table-based approach to calculate the proper MOD byte as it's easier.
    // Also, not all BASE [+ INDEX] combinations are supported in 16-bit mode, so
    // this may fail.
    const uint32_t kBaseGpIdx = (kX86MemInfo_BaseGp | kX86MemInfo_Index);

    if (rmInfo & kBaseGpIdx) {
      // ==========|> [BASE + INDEX + DISP16].
      uint32_t mod;

      rbReg &= 0x7;
      rxReg &= 0x7;

      if ((rmInfo & kBaseGpIdx) == kBaseGpIdx) {
        uint32_t shf = rmRel->as<X86Mem>().getShift();
        if (ASMJIT_UNLIKELY(shf != 0))
          goto InvalidAddress;
        mod = x86Mod16BaseIndexTable[(rbReg << 3) + rxReg];
      }
      else {
        if (rmInfo & kX86MemInfo_Index)
          rbReg = rxReg;
        mod = x86Mod16BaseTable[rbReg];
      }

      if (ASMJIT_UNLIKELY(mod == 0xFF))
        goto InvalidAddress;

      mod += opReg << 3;
      if (relOffset == 0 && mod != 0x06) {
        EMIT_BYTE(mod);
      }
      else if (Utils::isInt8(relOffset)) {
        EMIT_BYTE(mod + 0x40);
        EMIT_BYTE(relOffset);
      }
      else {
        EMIT_BYTE(mod + 0x80);
        EMIT_16(relOffset);
      }
    }
    else {
      // Not supported in 16-bit addresses.
      if (rmInfo & (kX86MemInfo_BaseRip | kX86MemInfo_BaseLabel))
        goto InvalidAddress;

      // ==========|> [DISP16].
      EMIT_BYTE(opReg | 0x06);
      EMIT_16(relOffset);
    }
  }

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - FPU]
  // --------------------------------------------------------------------------

EmitFpuOp:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // FPU instructions consist of two opcodes.
  EMIT_BYTE(opCode >> X86Inst::kOpCode_FPU_2B_Shift);
  EMIT_BYTE(opCode);
  goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - VEX / EVEX]
  // --------------------------------------------------------------------------

EmitVexEvexOp:
  {
    // These don't use immediate.
    ASMJIT_ASSERT(imLen == 0);

    // Only 'vzeroall' and 'vzeroupper' instructions use this encoding, they
    // don't define 'W' to be '1' so we can just check the 'mmmmm' field. Both
    // functions can encode by using VEV2 prefix so VEV3 is basically only used
    // when forced from outside.
    ASMJIT_ASSERT((opCode & X86Inst::kOpCode_W) == 0);

    uint32_t x = ((opCode & X86Inst::kOpCode_MM_Mask   ) >> (X86Inst::kOpCode_MM_Shift     )) |
                 ((opCode & X86Inst::kOpCode_LL_Mask   ) >> (X86Inst::kOpCode_LL_Shift - 10)) |
                 ((opCode & X86Inst::kOpCode_PP_VEXMask) >> (X86Inst::kOpCode_PP_Shift -  8)) |
                 ((options & X86Inst::kOptionVex3      ) >> (X86Inst::kOpCode_MM_Shift     )) ;
    if (x & 0x04U) {
      x  = (x & (0x4 ^ 0xFFFF)) << 8;                    // [00000000|00000Lpp|0000m0mm|00000000].
      x ^= (kX86ByteVex3) |                              // [........|00000Lpp|0000m0mm|__VEX3__].
           (0x07U  << 13) |                              // [........|00000Lpp|1110m0mm|__VEX3__].
           (0x0FU  << 19) |                              // [........|01111Lpp|1110m0mm|__VEX3__].
           (opCode << 24) ;                              // [_OPCODE_|01111Lpp|1110m0mm|__VEX3__].

      EMIT_32(x);
      goto EmitDone;
    }
    else {
      x = ((x >> 8) ^ x) ^ 0xF9;
      EMIT_BYTE(kX86ByteVex2);
      EMIT_BYTE(x);
      EMIT_BYTE(opCode);
      goto EmitDone;
    }
  }

EmitVexEvexR:
  {
    // VEX instructions use only 0-1 BYTE immediate.
    ASMJIT_ASSERT(imLen <= 1);

    // Construct `x` - a complete EVEX|VEX prefix.
    uint32_t x = ((opReg << 4) & 0xF980U) |              // [........|........|Vvvvv..R|R.......].
                 ((rbReg << 2) & 0x0060U) |              // [........|........|........|.BB.....].
                 (x86ExtractLLMM(opCode, options)) |     // [........|.LL.....|Vvvvv..R|RBBmmmmm].
                 (_extraReg.getId() << 16);              // [........|.LL..aaa|Vvvvv..R|RBBmmmmm].
    opReg &= 0x7;

    // Mark invalid VEX (force EVEX) case:               // [@.......|.LL..aaa|Vvvvv..R|RBBmmmmm].
    x |= (~commonData->getFlags() & X86Inst::kFlagVex) << (31 - Utils::firstBitOfT<X86Inst::kFlagVex>());

    // Handle AVX512 options by a single branch.
    const uint32_t kAvx512Options = X86Inst::kOptionZMask   |
                                    X86Inst::kOption1ToX    |
                                    X86Inst::kOptionSAE     |
                                    X86Inst::kOptionER      ;
    if (options & kAvx512Options) {
      // Memory broadcast without a memory operand is invalid.
      if (ASMJIT_UNLIKELY(options & X86Inst::kOption1ToX))
        goto InvalidBroadcast;

      // TODO: {sae} and {er}
      x |= options & X86Inst::kOptionZMask;              // [@.......|zLL..aaa|Vvvvv..R|RBBmmmmm].
    }

    // Check if EVEX is required by checking bits in `x` :  [@.......|xx...xxx|x......x|.x.x....].
    if (x & 0x80C78150U) {
      uint32_t y = ((x << 4) & 0x00080000U) |            // [@.......|....V...|........|........].
                   ((x >> 4) & 0x00000010U) ;            // [@.......|....V...|........|...R....].
      x  = (x & 0x00FF78E3U) | y;                        // [........|zLL.Vaaa|0vvvv000|RBBR00mm].
      x  = (x << 8) |                                    // [zLL.Vaaa|0vvvv000|RBBR00mm|00000000].
           ((opCode >> kSHR_W_PP) & 0x00830000U) |       // [zLL.Vaaa|Wvvvv0pp|RBBR00mm|00000000].
           ((opCode >> kSHR_W_EW) & 0x00800000U) ;       // [zLL.Vaaa|Wvvvv0pp|RBBR00mm|00000000] (added EVEX.W).
                                                         //      _     ____    ____
      x ^= 0x087CF000U | kX86ByteEvex;                   // [zLL.Vaaa|Wvvvv1pp|RBBR00mm|01100010].

      EMIT_32(x);
      EMIT_BYTE(opCode);

      rbReg &= 0x7;
      EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

      if (imLen == 0) goto EmitDone;
      EMIT_BYTE(imVal & 0xFF);
      goto EmitDone;
    }

    // Not EVEX, prepare `x` for VEX2 or VEX3:          x = [........|00L00000|0vvvv000|R0B0mmmm].
    x |= ((opCode >> (kSHR_W_PP + 8)) & 0x8300U) |       // [00000000|00L00000|Wvvvv0pp|R0B0mmmm].
         ((x      >> 11             ) & 0x0400U) ;       // [00000000|00L00000|WvvvvLpp|R0B0mmmm].

    // Check if VEX3 is required / forced:                  [........|........|x.......|..x..x..].
    if (x & 0x0008024U) {
      uint32_t xorMsk = x86VEXPrefix[x & 0xF] | (opCode << 24);

      // Clear 'FORCE-VEX3' bit and all high bits.
      x  = (x & (0x4 ^ 0xFFFF)) << 8;                    // [00000000|WvvvvLpp|R0B0m0mm|00000000].
                                                         //            ____    _ _
      x ^= xorMsk;                                       // [_OPCODE_|WvvvvLpp|R1Bmmmmm|VEX3|XOP].
      EMIT_32(x);

      rbReg &= 0x7;
      EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

      if (imLen == 0) goto EmitDone;
      EMIT_BYTE(imVal & 0xFF);
      goto EmitDone;
    }
    else {
      // 'mmmmm' must be '00001'.
      ASMJIT_ASSERT((x & 0x1F) == 0x01);

      x = ((x >> 8) ^ x) ^ 0xF9;
      EMIT_BYTE(kX86ByteVex2);
      EMIT_BYTE(x);
      EMIT_BYTE(opCode);

      rbReg &= 0x7;
      EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

      if (imLen == 0) goto EmitDone;
      EMIT_BYTE(imVal & 0xFF);
      goto EmitDone;
    }
  }

EmitVexEvexM:
  ASMJIT_ASSERT(rmRel != nullptr);
  ASMJIT_ASSERT(rmRel->getOp() == Operand::kOpMem);
  rmInfo = x86MemInfo[rmRel->as<X86Mem>().getBaseIndexType()];

  // Segment-override prefix.
  if (rmRel->as<X86Mem>().hasSegment())
    EMIT_BYTE(x86SegmentPrefix[rmRel->as<X86Mem>().getSegmentId()]);

  // Address-override prefix.
  if (rmInfo & _getAddressOverrideMask())
    EMIT_BYTE(0x67);

  rbReg = rmRel->as<X86Mem>().hasBaseReg()  ? rmRel->as<X86Mem>().getBaseId()  : uint32_t(0);
  rxReg = rmRel->as<X86Mem>().hasIndexReg() ? rmRel->as<X86Mem>().getIndexId() : uint32_t(0);

  {
    // VEX instructions use only 0-1 BYTE immediate.
    ASMJIT_ASSERT(imLen <= 1);

    // Construct `x` - a complete EVEX|VEX prefix.
    uint32_t x = ((opReg << 4 ) & 0x0000F980U) |         // [........|........|Vvvvv..R|R.......].
                 ((rxReg << 3 ) & 0x00000040U) |         // [........|........|........|.X......].
                 ((rxReg << 15) & 0x00080000U) |         // [........|....X...|........|........].
                 ((rbReg << 2 ) & 0x00000020U) |         // [........|........|........|..B.....].
                 (x86ExtractLLMM(opCode, options)) |     // [........|.LL.X...|Vvvvv..R|RXBmmmmm].
                 (_extraReg.getId() << 16)         ;     // [........|.LL.Xaaa|Vvvvv..R|RXBmmmmm].
    opReg &= 0x07U;

    // Mark invalid VEX (force EVEX) case:               // [@.......|.LL.Xaaa|Vvvvv..R|RXBmmmmm].
    x |= (~commonData->getFlags() & X86Inst::kFlagVex) << (31 - Utils::firstBitOfT<X86Inst::kFlagVex>());

    // Handle AVX512 options by a single branch.
    const uint32_t kAvx512Options = X86Inst::kOption1ToX    |
                                    X86Inst::kOptionZMask   |
                                    X86Inst::kOptionSAE     |
                                    X86Inst::kOptionER      ;
    if (options & kAvx512Options) {
      // {er} and {sae} are both invalid if memory operand is used.
      if (ASMJIT_UNLIKELY(options & (X86Inst::kOptionSAE | X86Inst::kOptionER)))
        goto InvalidEROrSAE;

      x |= options & (X86Inst::kOption1ToX |             // [@.......|.LLbXaaa|Vvvvv..R|RXBmmmmm].
                      X86Inst::kOptionZMask);            // [@.......|zLLbXaaa|Vvvvv..R|RXBmmmmm].
    }

    // Check if EVEX is required by checking bits in `x` :  [@.......|xx.xxxxx|x......x|...x....].
    if (x & 0x80DF8110U) {
      uint32_t y = ((x << 4) & 0x00080000U) |            // [@.......|....V...|........|........].
                   ((x >> 4) & 0x00000010U) ;            // [@.......|....V...|........|...R....].
      x  = (x & 0x00FF78E3U) | y;                        // [........|zLLbVaaa|0vvvv000|RXBR00mm].
      x  = (x << 8) |                                    // [zLLbVaaa|0vvvv000|RBBR00mm|00000000].
           ((opCode >> kSHR_W_PP) & 0x00830000U) |       // [zLLbVaaa|Wvvvv0pp|RBBR00mm|00000000].
           ((opCode >> kSHR_W_EW) & 0x00800000U) ;       // [zLLbVaaa|Wvvvv0pp|RBBR00mm|00000000] (added EVEX.W).
                                                         //      _     ____    ____
      x ^= 0x087CF000U | kX86ByteEvex;                   // [zLLbVaaa|Wvvvv1pp|RBBR00mm|01100010].

      EMIT_32(x);
      EMIT_BYTE(opCode);

      if (opCode & 0x10000000U) {
        // Broadcast, change the compressed displacement scale to either x4 (SHL 2) or x8 (SHL 3)
        // depending on instruction's W. If 'W' is 1 'SHL' must be 3, otherwise it must be 2.
        opCode &=~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);
        opCode |= ((x & 0x00800000U) ? 3 : 2) << X86Inst::kOpCode_CDSHL_Shift;
      }
      else {
        // Add the compressed displacement 'SHF' to the opcode based on 'TTWLL'.
        uint32_t TTWLL = ((opCode >> (X86Inst::kOpCode_CDTT_Shift - 3)) & 0x18) +
                         ((opCode >> (X86Inst::kOpCode_W_Shift    - 2)) & 0x04) +
                         ((x >> 29) & 0x3);
        opCode += x86CDisp8SHL[TTWLL];
      }
    }
    else {
      // Not EVEX, prepare `x` for VEX2 or VEX3:        x = [........|00L00000|0vvvv000|RXB0mmmm].
      x |= ((opCode >> (kSHR_W_PP + 8)) & 0x8300U) |     // [00000000|00L00000|Wvvvv0pp|RXB0mmmm].
           ((x      >> 11             ) & 0x0400U) ;     // [00000000|00L00000|WvvvvLpp|RXB0mmmm].

      // Clear a possible CDisp specified by EVEX.
      opCode &= ~X86Inst::kOpCode_CDSHL_Mask;

      // Check if VEX3 is required / forced:                [........|........|x.......|.xx..x..].
      if (x & 0x0008064U) {
        uint32_t xorMsk = x86VEXPrefix[x & 0xF] | (opCode << 24);

        // Clear 'FORCE-VEX3' bit and all high bits.
        x  = (x & (0x4 ^ 0xFFFF)) << 8;                  // [00000000|WvvvvLpp|RXB0m0mm|00000000].
                                                         //            ____    ___
        x ^= xorMsk;                                     // [_OPCODE_|WvvvvLpp|RXBmmmmm|VEX3_XOP].
        EMIT_32(x);
      }
      else {
        // 'mmmmm' must be '00001'.
        ASMJIT_ASSERT((x & 0x1F) == 0x01);

        x = ((x >> 8) ^ x) ^ 0xF9;
        EMIT_BYTE(kX86ByteVex2);
        EMIT_BYTE(x);
        EMIT_BYTE(opCode);
      }
    }
  }

  // MOD|SIB address.
  if (!commonData->hasFlag(X86Inst::kFlagVsib))
    goto EmitModSib;

  // MOD|VSIB address without INDEX is invalid.
  if (rmInfo & kX86MemInfo_Index)
    goto EmitModVSib;
  goto InvalidInstruction;

  // --------------------------------------------------------------------------
  // [Emit - Jmp/Jcc/Call]
  // --------------------------------------------------------------------------

  // TODO: Should be adjusted after the support for multiple sections feature is added.
EmitJmpCall:
  {
    // Emit REX prefix if asked for (64-bit only).
    uint32_t rex = x86ExtractREX(opCode, options);
    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto InvalidRexPrefix;
      EMIT_BYTE(rex | kX86ByteRex);
    }

    uint64_t ip = static_cast<uint64_t>((intptr_t)(cursor - _bufferData));
    uint32_t rel32 = 0;
    uint32_t opCode8 = commonData->getAltOpCode();

    uint32_t inst8Size  = 1 + 1; //          OPCODE + REL8 .
    uint32_t inst32Size = 1 + 4; // [PREFIX] OPCODE + REL32.

    // Jcc instructions with 32-bit displacement use 0x0F prefix,
    // other instructions don't. No other prefixes are used by X86.
    ASMJIT_ASSERT((opCode8 & X86Inst::kOpCode_MM_Mask) == 0);
    ASMJIT_ASSERT((opCode  & X86Inst::kOpCode_MM_Mask) == 0 ||
                  (opCode  & X86Inst::kOpCode_MM_Mask) == X86Inst::kOpCode_MM_0F);

    // Only one of these should be used at the same time.
    inst32Size += static_cast<uint32_t>(opReg != 0);
    inst32Size += static_cast<uint32_t>((opCode & X86Inst::kOpCode_MM_Mask) == X86Inst::kOpCode_MM_0F);

    if (rmRel->isLabel()) {
      label = _code->getLabelEntry(rmRel->as<Label>());
      if (!label) goto InvalidLabel;

      if (label->isBound()) {
        // Bound label.
        rel32 = static_cast<uint32_t>((static_cast<uint64_t>(label->getOffset()) - ip - inst32Size) & 0xFFFFFFFFU);
        goto EmitJmpCallRel;
      }
      else {
        // Non-bound label.
        if (opCode8 && (!opCode || (options & X86Inst::kOptionShortForm))) {
          EMIT_BYTE(opCode8);
          relOffset = -1;
          relSize = 1;
          goto EmitRel;
        }
        else {
          // Refuse also 'short' prefix, if specified.
          if (ASMJIT_UNLIKELY(!opCode || (options & X86Inst::kOptionShortForm) != 0))
            goto InvalidDisplacement;

          // Emit [PREFIX] OPCODE [/X] <DISP32>.
          if (opCode & X86Inst::kOpCode_MM_Mask)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(opCode);
          if (opReg)
            EMIT_BYTE(x86EncodeMod(3, opReg, 0));

          relOffset = -4;
          relSize = 4;
          goto EmitRel;
        }
      }
    }

    if (rmRel->isImm()) {
      uint64_t baseAddress = getCodeInfo().getBaseAddress();
      uint64_t jumpAddress = rmRel->as<Imm>().getUInt64();

      // If the base-address is known calculate a relative displacement and
      // check if it fits in 32 bits (which is always true in 32-bit mode).
      // Emit relative displacement as it was a bound label if all checks ok.
      if (baseAddress != Globals::kNoBaseAddress) {
        uint64_t rel64 = jumpAddress - (ip + baseAddress) - inst32Size;
        if (getArchType() == ArchInfo::kTypeX86 || Utils::isInt32(static_cast<int64_t>(rel64))) {
          rel32 = static_cast<uint32_t>(rel64 & 0xFFFFFFFFU);
          goto EmitJmpCallRel;
        }
        else {
          // Relative displacement exceeds 32-bits - relocator can only
          // insert trampoline for jmp/call, but not for jcc/jecxz.
          if (ASMJIT_UNLIKELY(!x86IsJmpOrCall(instId)))
            goto InvalidDisplacement;
        }
      }

      if (ASMJIT_UNLIKELY(_code->_relocations.willGrow(&_code->_baseHeap) != kErrorOk))
        goto NoHeapMemory;

      err = _code->newRelocEntry(&re, RelocEntry::kTypeAbsToRel, 0);
      if (ASMJIT_UNLIKELY(err)) goto Failed;

      re->_sourceSectionId = _section->getId();
      re->_data = static_cast<int64_t>(jumpAddress);

      if (ASMJIT_LIKELY(opCode)) {
        // 64-bit: Emit REX prefix so the instruction can be patched later.
        // REX prefix does nothing if not patched, but allows to patch the
        // instruction to use MOD/M and to point to a memory where the final
        // 64-bit address is stored.
        re->_size = 4;
        re->_sourceOffset = ip + inst32Size - 4;

        if (getArchType() != ArchInfo::kTypeX86 && x86IsJmpOrCall(instId)) {
          if (!rex) {
            re->_sourceOffset++;
            EMIT_BYTE(kX86ByteRex);
          }

          re->_type = RelocEntry::kTypeTrampoline;
          _code->_trampolinesSize += 8;
        }

        // Emit [PREFIX] OPCODE [/X] DISP32.
        if (opCode & X86Inst::kOpCode_MM_Mask)
          EMIT_BYTE(0x0F);

        EMIT_BYTE(opCode);
        if (opReg)
          EMIT_BYTE(x86EncodeMod(3, opReg, 0));

        EMIT_32(0);
      }
      else {
        re->_size = 1;
        re->_sourceOffset = ip + inst8Size - 1;

        // Emit OPCODE + DISP8.
        EMIT_BYTE(opCode8);
        EMIT_BYTE(0);
      }
      goto EmitDone;
    }

    // Not Label|Imm -> Invalid.
    goto InvalidInstruction;

    // Emit jmp/call with relative displacement known at assembly-time. Decide
    // between 8-bit and 32-bit displacement encoding. Some instructions only
    // allow either 8-bit or 32-bit encoding, others allow both encodings.
EmitJmpCallRel:
    if (Utils::isInt8(static_cast<int32_t>(rel32 + inst32Size - inst8Size)) && opCode8 && !(options & X86Inst::kOptionLongForm)) {
      options |= X86Inst::kOptionShortForm;
      EMIT_BYTE(opCode8);
      EMIT_BYTE(rel32 + inst32Size - inst8Size);
      goto EmitDone;
    }
    else {
      if (ASMJIT_UNLIKELY(!opCode || (options & X86Inst::kOptionShortForm) != 0))
        goto InvalidDisplacement;

      options &= ~X86Inst::kOptionShortForm;
      if (opCode & X86Inst::kOpCode_MM_Mask)
        EMIT_BYTE(0x0F);

      EMIT_BYTE(opCode);
      if (opReg)
        EMIT_BYTE(x86EncodeMod(3, opReg, 0));

      EMIT_32(rel32);
      goto EmitDone;
    }
  }

  // --------------------------------------------------------------------------
  // [Emit - Relative]
  // --------------------------------------------------------------------------

EmitRel:
  {
    ASMJIT_ASSERT(!label->isBound());
    ASMJIT_ASSERT(relSize == 1 || relSize == 4);

    // Chain with label.
    size_t offset = (size_t)(cursor - _bufferData);
    LabelLink* link = _code->newLabelLink(label, _section->getId(), offset, relOffset);

    if (ASMJIT_UNLIKELY(!link))
      goto NoHeapMemory;

    if (re)
      link->relocId = re->getId();

    // Emit label size as dummy data.
    if (relSize == 1)
      EMIT_BYTE(0x01);
    else // if (relSize == 4)
      EMIT_32(0x04040404);
  }

  if (imLen == 0)
    goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Immediate]
  // --------------------------------------------------------------------------

EmitImm:
  {
#if ASMJIT_ARCH_64BIT
    uint32_t i = imLen;
    uint64_t imm = static_cast<uint64_t>(imVal);
#else
    uint32_t i = imLen;
    uint32_t imm = static_cast<uint32_t>(imVal & 0xFFFFFFFFU);
#endif

    // Many instructions just use a single byte immediate, so make it fast.
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;
    imm >>= 8;
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;
    imm >>= 8;
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;
    imm >>= 8;
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;

    // Can be 1-4 or 8 bytes, this handles the remaining high DWORD of an 8-byte immediate.
    ASMJIT_ASSERT(i == 4);

#if ASMJIT_ARCH_64BIT
    imm >>= 8;
    EMIT_32(static_cast<uint32_t>(imm));
#else
    EMIT_32(static_cast<uint32_t>((static_cast<uint64_t>(imVal) >> 32) & 0xFFFFFFFFU));
#endif
  }

  // --------------------------------------------------------------------------
  // [Done]
  // --------------------------------------------------------------------------

EmitDone:
#if !defined(ASMJIT_DISABLE_LOGGING)
  // Logging is a performance hit anyway, so make it the unlikely case.
  if (ASMJIT_UNLIKELY(options & CodeEmitter::kOptionLoggingEnabled))
    _emitLog(instId, options, o0, o1, o2, o3, relSize, imLen, cursor);
#endif // !ASMJIT_DISABLE_LOGGING

  resetOptions();
  resetExtraReg();
  resetInlineComment();

  _bufferPtr = cursor;
  return kErrorOk;

  // --------------------------------------------------------------------------
  // [Error Cases]
  // --------------------------------------------------------------------------

#define ERROR_HANDLER(ERROR)                \
ERROR:                                      \
  err = DebugUtils::errored(kError##ERROR); \
  goto Failed;

ERROR_HANDLER(NoHeapMemory)
ERROR_HANDLER(InvalidArgument)
ERROR_HANDLER(InvalidLabel)
ERROR_HANDLER(InvalidInstruction)
ERROR_HANDLER(InvalidLockPrefix)
ERROR_HANDLER(InvalidXAcquirePrefix)
ERROR_HANDLER(InvalidXReleasePrefix)
ERROR_HANDLER(InvalidRepPrefix)
ERROR_HANDLER(InvalidRexPrefix)
ERROR_HANDLER(InvalidBroadcast)
ERROR_HANDLER(InvalidEROrSAE)
ERROR_HANDLER(InvalidAddress)
ERROR_HANDLER(InvalidAddressIndex)
ERROR_HANDLER(InvalidAddress64Bit)
ERROR_HANDLER(InvalidDisplacement)
ERROR_HANDLER(InvalidSegment)
ERROR_HANDLER(InvalidImmediate)
ERROR_HANDLER(OperandSizeMismatch)
ERROR_HANDLER(AmbiguousOperandSize)
ERROR_HANDLER(NotConsecutiveRegs)

Failed:
  return _emitFailed(err, instId, options, o0, o1, o2, o3);
}

// ============================================================================
// [asmjit::X86Assembler - Align]
// ============================================================================

Error X86Assembler::align(uint32_t mode, uint32_t alignment) {
#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_globalOptions & kOptionLoggingEnabled)
    _code->_logger->logf("%s.align %u\n", _code->_logger->getIndentation(), alignment);
#endif // !ASMJIT_DISABLE_LOGGING

  if (mode >= kAlignCount)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment <= 1)
    return kErrorOk;

  if (!Utils::isPowerOf2(alignment) || alignment > Globals::kMaxAlignment)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  uint32_t i = static_cast<uint32_t>(Utils::alignDiff<size_t>(getOffset(), alignment));
  if (i == 0)
    return kErrorOk;

  if (getRemainingSpace() < i) {
    Error err = _code->growBuffer(&_section->_buffer, i);
    if (ASMJIT_UNLIKELY(err)) return setLastError(err);
  }

  uint8_t* cursor = _bufferPtr;
  uint8_t pattern = 0x00;

  switch (mode) {
    case kAlignCode: {
      if (_globalHints & kHintOptimizedAlign) {
        // Intel 64 and IA-32 Architectures Software Developer's Manual - Volume 2B (NOP).
        enum { kMaxNopSize = 9 };

        static const uint8_t nopData[kMaxNopSize][kMaxNopSize] = {
          { 0x90 },
          { 0x66, 0x90 },
          { 0x0F, 0x1F, 0x00 },
          { 0x0F, 0x1F, 0x40, 0x00 },
          { 0x0F, 0x1F, 0x44, 0x00, 0x00 },
          { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 }
        };

        do {
          uint32_t n = std::min<uint32_t>(i, kMaxNopSize);
          const uint8_t* src = nopData[n - 1];

          i -= n;
          do {
            EMIT_BYTE(*src++);
          } while (--n);
        } while (i);
      }

      pattern = 0x90;
      break;
    }

    case kAlignData:
      pattern = 0xCC;
      break;

    case kAlignZero:
      // Pattern already set to zero.
      break;
  }

  while (i) {
    EMIT_BYTE(pattern);
    i--;
  }

  _bufferPtr = cursor;
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
