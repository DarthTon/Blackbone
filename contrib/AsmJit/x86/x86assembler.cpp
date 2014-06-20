// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64)

// [Dependencies - AsmJit]
#include "../base/intutil.h"
#include "../base/logger.h"
#include "../base/runtime.h"
#include "../base/string.h"
#include "../base/vmem.h"
#include "../x86/x86assembler.h"
#include "../x86/x86cpuinfo.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [Constants]
// ============================================================================

enum { kRexShift = 6 };
enum { kRexForbidden = 0x80 };
enum { kMaxCommentLength = 80 };

// 2-byte VEX prefix.
//   [0] kVex2Byte.
//   [1] RvvvvLpp.
enum { kVex2Byte = 0xC5 };

// 3-byte VEX prefix.
//   [0] kVex3Byte.
//   [1] RXBmmmmm.
//   [2] WvvvvLpp.
enum { kVex3Byte = 0xC4 };

// 3-byte XOP prefix.
//   [0] kXopByte
//   [1] RXBmmmmm
//   [2] WvvvvLpp
enum { kXopByte = 0x8F };

// AsmJit specific (used to encode VVVV field in XOP/VEX).
enum kVexVVVV {
  kVexVVVVShift = 12,
  kVexVVVVMask = 0xF << kVexVVVVShift
};

//! \internal
//!
//! Instruction 2-byte/3-byte opcode prefix definition.
struct OpCodeMM {
  uint8_t len;
  uint8_t data[3];
};

//! \internal
//!
//! Mandatory prefixes encoded in 'asmjit' opcode [66, F3, F2] and asmjit
//! extensions
static const uint8_t x86OpCodePP[8] = {
  0x00,
  0x66,
  0xF3,
  0xF2,
  0x00,
  0x00,
  0x00,
  0x9B
};

//! \internal
//!
//! Instruction 2-byte/3-byte opcode prefix data.
static const OpCodeMM x86OpCodeMM[] = {
  { 0, { 0x00, 0x00, 0 } },
  { 1, { 0x0F, 0x00, 0 } },
  { 2, { 0x0F, 0x38, 0 } },
  { 2, { 0x0F, 0x3A, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 0, { 0x00, 0x00, 0 } },
  { 2, { 0x0F, 0x01, 0 } }
};

static const uint8_t x86SegmentPrefix[8] = { 0x00, 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65 };
static const uint8_t x86OpCodePushSeg[8] = { 0x00, 0x06, 0x0E, 0x16, 0x1E, 0xA0, 0xA8 };
static const uint8_t x86OpCodePopSeg[8]  = { 0x00, 0x07, 0x00, 0x17, 0x1F, 0xA1, 0xA9 };

//! Encode MODR/M.
static ASMJIT_INLINE uint32_t x86EncodeMod(uint32_t m, uint32_t o, uint32_t rm) {
  return (m << 6) + (o << 3) + rm;
}

//! Encode SIB.
static ASMJIT_INLINE uint32_t x86EncodeSib(uint32_t s, uint32_t i, uint32_t b) {
  return (s << 6) + (i << 3) + b;
}

// ============================================================================
// [asmjit::x86x64::Emit]
// ============================================================================

#define ENC_OPS(_Op0_, _Op1_, _Op2_) \
  ((kOperandType##_Op0_) + ((kOperandType##_Op1_) << 3) + ((kOperandType##_Op2_) << 6))

#define ADD_66H_P(_Exp_) \
  do { \
    opCode |= (static_cast<uint32_t>(_Exp_) << kInstOpCode_PP_Shift); \
  } while (0)

#define ADD_REX_W(_Exp_) \
  do { \
    if (Arch == kArchX64) \
      opX |= static_cast<uint32_t>(_Exp_) << 3; \
  } while (0)
#define ADD_REX_B(_Reg_) \
  do { \
    if (Arch == kArchX64) \
      opX |= static_cast<uint32_t>(_Reg_) >> 3; \
  } while (0)

#define ADD_VEX_W(_Exp_) \
  do { \
    opX |= static_cast<uint32_t>(_Exp_) << 3; \
  } while (0)

#define ADD_VEX_L(_Exp_) \
  do { \
    opCode |= static_cast<uint32_t>(_Exp_) << kInstOpCode_L_Shift; \
  } while (0)

#define EMIT_BYTE(_Val_) \
  do { \
    cursor[0] = static_cast<uint8_t>(_Val_); \
    cursor += 1; \
  } while (0)

#define EMIT_WORD(_Val_) \
  do { \
    reinterpret_cast<uint16_t*>(cursor)[0] = static_cast<uint16_t>(_Val_); \
    cursor += 2; \
  } while (0)

#define EMIT_DWORD(_Val_) \
  do { \
    reinterpret_cast<uint32_t*>(cursor)[0] = static_cast<uint32_t>(_Val_); \
    cursor += 4; \
  } while (0)

#define EMIT_QWORD(_Val_) \
  do { \
    reinterpret_cast<uint64_t*>(cursor)[0] = static_cast<uint64_t>(_Val_); \
    cursor += 8; \
  } while (0)

#define EMIT_OP(_Val_) \
  do { \
    EMIT_BYTE((_Val_) & 0xFF); \
  } while (0)

#define EMIT_PP(_Val_) \
  do { \
    uint32_t ppIndex = ((_Val_) >> kInstOpCode_PP_Shift) & (kInstOpCode_PP_Mask >> kInstOpCode_PP_Shift); \
    uint8_t ppCode = x86OpCodePP[ppIndex]; \
    \
    if (!ppIndex) \
      break; \
    \
    cursor[0] = ppCode; \
    cursor++; \
  } while (0)

#define EMIT_MM(_Val_) \
  do { \
    uint32_t mmIndex = ((_Val_) >> kInstOpCode_MM_Shift) & (kInstOpCode_MM_Mask >> kInstOpCode_MM_Shift); \
    const OpCodeMM& mmCode = x86OpCodeMM[mmIndex]; \
    \
    if (!mmIndex) \
      break; \
    \
    cursor[0] = mmCode.data[0]; \
    cursor[1] = mmCode.data[1]; \
    cursor += mmCode.len; \
  } while (0)

// ============================================================================
// [asmjit::x86x64::Assembler - Construction / Destruction]
// ============================================================================

X86X64Assembler::X86X64Assembler(Runtime* runtime) : BaseAssembler(runtime) {}
X86X64Assembler::~X86X64Assembler() {}

// ============================================================================
// [asmjit::x86x64::Assembler - Label]
// ============================================================================

void X86X64Assembler::_bind(const Label& label) {
  // Get label data based on label id.
  uint32_t index = label.getId();
  LabelData* data = getLabelDataById(index);

  // Label can be bound only once.
  ASMJIT_ASSERT(data->offset == -1);

#if !defined(ASMJIT_DISABLE_LOGGER)
  if (_logger)
    _logger->logFormat(kLoggerStyleLabel, "L%u:\n", index);
#endif // !ASMJIT_DISABLE_LOGGER

  size_t pos = getOffset();

  LabelLink* link = data->links;
  LabelLink* prev = NULL;

  while (link) {
    intptr_t offset = link->offset;

    if (link->relocId != -1) {
      // If linked label points to RelocData then instead of writing relative
      // displacement to assembler stream, we will write it to RelocData->
      _relocData[link->relocId].data += static_cast<Ptr>(pos);
    }
    else {
      // Not using relocId, this means that we overwriting real displacement
      // in assembler stream.
      int32_t patchedValue = static_cast<int32_t>(
        static_cast<intptr_t>(pos) - offset + link->displacement);

      // Size of the value we are going to patch. Only BYTE/DWORD is allowed.
      uint32_t size = getByteAt(offset);
      ASMJIT_ASSERT(size == 1 || size == 4);

      if (size == 4) {
        setInt32At(offset, patchedValue);
      }
      else { // if (size) == 1
        if (IntUtil::isInt8(patchedValue))
          setByteAt(offset, static_cast<uint8_t>(patchedValue & 0xFF));
        else
          setError(kErrorIllegalDisplacement);
      }
    }

    prev = link->prev;
    link = prev;
  }

  // Chain unused links.
  link = data->links;
  if (link) {
    if (prev == NULL)
      prev = link;

    prev->prev = _unusedLinks;
    _unusedLinks = link;
  }

  // Unlink label if it was linked.
  data->offset = pos;
  data->links = NULL;
}

// ============================================================================
// [asmjit::x86x64::Assembler - Embed]
// ============================================================================

Error X86X64Assembler::embedLabel(const Label& op) {
  ASMJIT_ASSERT(op.getId() != kInvalidValue);
  uint32_t regSize = _regSize;

  if (getRemainingSpace() < regSize)
    ASMJIT_PROPAGATE_ERROR(_grow(regSize));

  uint8_t* cursor = getCursor();

  LabelData* label = getLabelDataById(op.getId());
  RelocData reloc;

#if !defined(ASMJIT_DISABLE_LOGGER)
  if (_logger)
    _logger->logFormat(kLoggerStyleData, regSize == 4 ? ".dd L%u\n" : ".dq L%u\n", op.getId());
#endif // !ASMJIT_DISABLE_LOGGER

  reloc.type = kRelocRelToAbs;
  reloc.size = regSize;
  reloc.from = static_cast<Ptr>(getOffset());
  reloc.data = 0;

  if (label->offset != -1) {
    // Bound label.
    reloc.data = static_cast<Ptr>(static_cast<SignedPtr>(label->offset));
  }
  else {
    // Non-bound label. Need to chain.
    LabelLink* link = _newLabelLink();

    link->prev = (LabelLink*)label->links;
    link->offset = getOffset();
    link->displacement = 0;
    link->relocId = _relocData.getLength();

    label->links = link;
  }

  if (_relocData.append(reloc) != kErrorOk)
    return setError(kErrorNoHeapMemory);

  // Emit dummy intptr_t (4 or 8 bytes; depends on the address size).
  if (regSize == 4)
    EMIT_DWORD(0);
  else
    EMIT_QWORD(0);

  setCursor(cursor);
  return kErrorOk;
}

// ============================================================================
// [asmjit::x86x64::Assembler - Align]
// ============================================================================

Error X86X64Assembler::_align(uint32_t mode, uint32_t offset) {
#if !defined(ASMJIT_DISABLE_LOGGER)
  if (_logger)
    _logger->logFormat(kLoggerStyleDirective,
      "%s.align %u\n", _logger->getIndentation(), static_cast<unsigned int>(offset));
#endif // !ASMJIT_DISABLE_LOGGER

  if (offset <= 1 || !IntUtil::isPowerOf2(offset) || offset > 64)
    return setError(kErrorInvalidArgument);

  uint32_t i = static_cast<uint32_t>(IntUtil::deltaTo<size_t>(getOffset(), offset));
  if (i == 0)
    return kErrorOk;

  if (getRemainingSpace() < i)
    ASMJIT_PROPAGATE_ERROR(_grow(i));

  uint8_t* cursor = getCursor();
  uint8_t alignPattern = 0xCC;

  if (mode == kAlignCode) {
    alignPattern = 0x90;

    if (IntUtil::hasBit(_features, kCodeGenOptimizedAlign)) {
      const CpuInfo* cpuInfo = static_cast<const CpuInfo*>(getRuntime()->getCpuInfo());

      // NOPs optimized for Intel:
      //   Intel 64 and IA-32 Architectures Software Developer's Manual
      //   - Volume 2B
      //   - Instruction Set Reference N-Z
      //     - NOP

      // NOPs optimized for AMD:
      //   Software Optimization Guide for AMD Family 10h Processors (Quad-Core)
      //   - 4.13 - Code Padding with Operand-Size Override and Multibyte NOP

      // Intel and AMD.
      static const uint8_t nop1[] = { 0x90 };
      static const uint8_t nop2[] = { 0x66, 0x90 };
      static const uint8_t nop3[] = { 0x0F, 0x1F, 0x00 };
      static const uint8_t nop4[] = { 0x0F, 0x1F, 0x40, 0x00 };
      static const uint8_t nop5[] = { 0x0F, 0x1F, 0x44, 0x00, 0x00 };
      static const uint8_t nop6[] = { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 };
      static const uint8_t nop7[] = { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 };
      static const uint8_t nop8[] = { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };
      static const uint8_t nop9[] = { 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

      // AMD.
      static const uint8_t nop10[] = { 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };
      static const uint8_t nop11[] = { 0x66, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

      const uint8_t* p;
      uint32_t n;

      if (cpuInfo->getVendorId() == kCpuVendorIntel && (
          (cpuInfo->getFamily() & 0x0F) == 0x06 ||
          (cpuInfo->getFamily() & 0x0F) == 0x0F)) {
        do {
          switch (i) {
            case  1: p = nop1; n = 1; break;
            case  2: p = nop2; n = 2; break;
            case  3: p = nop3; n = 3; break;
            case  4: p = nop4; n = 4; break;
            case  5: p = nop5; n = 5; break;
            case  6: p = nop6; n = 6; break;
            case  7: p = nop7; n = 7; break;
            case  8: p = nop8; n = 8; break;
            default: p = nop9; n = 9; break;
          }

          i -= n;
          do {
            EMIT_BYTE(*p++);
          } while (--n);
        } while (i);
      }
      else if (cpuInfo->getVendorId() == kCpuVendorAmd && cpuInfo->getFamily() >= 0x0F) {
        do {
          switch (i) {
            case  1: p = nop1 ; n =  1; break;
            case  2: p = nop2 ; n =  2; break;
            case  3: p = nop3 ; n =  3; break;
            case  4: p = nop4 ; n =  4; break;
            case  5: p = nop5 ; n =  5; break;
            case  6: p = nop6 ; n =  6; break;
            case  7: p = nop7 ; n =  7; break;
            case  8: p = nop8 ; n =  8; break;
            case  9: p = nop9 ; n =  9; break;
            case 10: p = nop10; n = 10; break;
            default: p = nop11; n = 11; break;
          }

          i -= n;
          do {
            EMIT_BYTE(*p++);
          } while (--n);
        } while (i);
      }
    }
  }

  while (i) {
    EMIT_BYTE(alignPattern);
    i--;
  }

  setCursor(cursor);
  return kErrorOk;
}

// ============================================================================
// [asmjit::x86x64::Assembler - Reloc]
// ============================================================================

template<int Arch>
static ASMJIT_INLINE size_t X86X64Assembler_relocCode(const X86X64Assembler* self, void* _dst, Ptr base) {
  uint8_t* dst = static_cast<uint8_t*>(_dst);

  size_t codeOffset = self->getOffset();
  size_t codeSize = self->getCodeSize();

  // We will copy the exact size of the generated code. Extra code for trampolines
  // is generated on-the-fly by the relocator (this code doesn't exist at the moment).
  ::memcpy(dst, self->_buffer, codeOffset);

  // Trampoline pointer.
  uint8_t* tramp;

  if (Arch == kArchX64)
    tramp = dst + codeOffset;

  // Relocate all recorded locations.
  size_t i, len = self->_relocData.getLength();

  for (i = 0; i < len; i++) {
    const RelocData& r = self->_relocData[i];
    Ptr ptr = 0;

    // Whether to use trampoline, can be only used if relocation type is kRelocAbsToRel.
    bool useTrampoline = false;

    // Be sure that reloc data structure is correct.
    size_t offset = static_cast<size_t>(r.from);
    ASMJIT_ASSERT(offset + r.size <= static_cast<Ptr>(codeSize));

    switch (r.type) {
      case kRelocAbsToAbs:
        ptr = r.data;
        break;

      case kRelocRelToAbs:
        ptr = r.data + base;
        break;

      case kRelocAbsToRel:
      case kRelocTrampoline:
        ptr = r.data - (base + r.from + 4);

        if (Arch == kArchX64 && r.type == kRelocTrampoline && !IntUtil::isInt32(ptr)) {
          ptr = (Ptr)tramp - (base + r.from + 4);
          useTrampoline = true;
        }
        break;

      default:
        ASMJIT_ASSERT(!"Reached");
    }

    switch (r.size) {
      case 4:
        *reinterpret_cast<int32_t*>(dst + offset) = static_cast<int32_t>(ptr);
        break;

      case 8:
        *reinterpret_cast<int64_t*>(dst + offset) = static_cast<int64_t>(ptr);
        break;

      default:
        ASMJIT_ASSERT(!"Reached");
    }

    // Patch `jmp/call` to use trampoline.
    if (Arch == kArchX64 && useTrampoline) {
      uint32_t byte0 = 0xFF;
      uint32_t byte1 = dst[offset - 1];

      if (byte1 == 0xE8) {
        // Call, path to FF/2 (-> 0x15).
        byte1 = x86EncodeMod(0, 2, 5);
      }
      else if (byte1 == 0xE9) {
        // Jmp, path to FF/4 (-> 0x25).
        byte1 = x86EncodeMod(0, 4, 5);
      }

      // Patch `jmp/call` instruction.
      ASMJIT_ASSERT(offset >= 2);
      dst[offset - 2] = byte0;
      dst[offset - 1] = byte1;

      // Absolute address.
      ((uint64_t*)tramp)[0] = static_cast<uint64_t>(r.data);

      // Advance trampoline pointer.
      tramp += 8;

#if !defined(ASMJIT_DISABLE_LOGGER)
      if (self->_logger)
        self->_logger->logFormat(kLoggerStyleComment, "; Trampoline %llX\n", r.data);
#endif // !ASMJIT_DISABLE_LOGGER
    }
  }

  if (Arch == kArchX64)
    return (size_t)(tramp - dst);
  else
    return (size_t)(codeOffset);
}

// ============================================================================
// [asmjit::x86x64::Assembler - Logging]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGER)
// Logging helpers.
static const char* AssemblerX86_operandSize[] = {
  "",
  "byte ptr ",
  "word ptr ",
  NULL,
  "dword ptr ",
  NULL,
  NULL,
  NULL,
  "qword ptr ",
  NULL,
  "tword ptr ",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "oword ptr "
};

static const char X86Assembler_segName[] =
  "\0\0\0\0"
  "es:\0"
  "cs:\0"
  "ss:\0"
  "ds:\0"
  "fs:\0"
  "gs:\0"
  "\0\0\0\0";

static void X86Assembler_dumpRegister(StringBuilder& sb, uint32_t type, uint32_t index) {
  // -- (Not-Encodable).
  static const char reg8l[] = "al\0\0" "cl\0\0" "dl\0\0" "bl\0\0" "spl\0"  "bpl\0"  "sil\0"  "dil\0" ;
  static const char reg8h[] = "ah\0\0" "ch\0\0" "dh\0\0" "bh\0\0" "--\0\0" "--\0\0" "--\0\0" "--\0\0";
  static const char reg16[] = "ax\0\0" "cx\0\0" "dx\0\0" "bx\0\0" "sp\0\0" "bp\0\0" "si\0\0" "di\0\0";

  char suffix = '\0';

  switch (type) {
    case kRegTypeGpbLo:
      if (index >= 8) {
        sb._appendChar('r');
        suffix = 'b';
        goto _EmitID;
      }

      sb._appendString(&reg8l[index * 4]);
      return;

    case kRegTypePatchedGpbHi:
      if (index < 4)
        goto _EmitNE;

      index -= 4;
      // ... Fall through ...

    case kRegTypeGpbHi:
      if (index >= 4)
        goto _EmitNE;

      sb._appendString(&reg8h[index * 4]);
      return;

_EmitNE:
      sb._appendString("--", 2);
      return;

    case kRegTypeGpw:
      if (index >= 8) {
        sb._appendChar('r');
        suffix = 'w';
        goto _EmitID;
      }

      sb._appendString(&reg16[index * 4]);
      return;

    case kRegTypeGpd:
      if (index >= 8) {
        sb._appendChar('r');
        suffix = 'd';
        goto _EmitID;
      }

      sb._appendChar('e');
      sb._appendString(&reg16[index * 4]);
      return;

    case kRegTypeGpq:
      sb._appendChar('r');
      if (index >= 8)
        goto _EmitID;

      sb._appendString(&reg16[index * 4]);
      return;

    case kRegTypeFp:
      sb._appendString("fp", 2);
      goto _EmitID;

    case kRegTypeMm:
      sb._appendString("mm", 2);
      goto _EmitID;

    case kRegTypeXmm:
      sb._appendString("xmm", 3);
      goto _EmitID;

    case kRegTypeYmm:
      sb._appendString("ymm", 3);
      goto _EmitID;

    case kRegTypeSeg:
      if (index >= kRegCountSeg)
        goto _EmitNE;

      sb._appendString(&X86Assembler_segName[index * 4], 2);
      return;

    default:
      return;
  }

_EmitID:
  sb._appendUInt32(index);

  if (suffix)
    sb._appendChar(suffix);
}

static void X86Assembler_dumpOperand(StringBuilder& sb, uint32_t arch, const Operand* op, uint32_t loggerOptions) {
  if (op->isReg()) {
    X86Assembler_dumpRegister(sb,
      static_cast<const X86Reg*>(op)->getRegType(),
      static_cast<const X86Reg*>(op)->getRegIndex());
  }
  else if (op->isMem()) {
    const Mem* m = static_cast<const Mem*>(op);

    uint32_t type = kRegTypeGpd;
    uint32_t seg = m->getSegment();
    bool isAbsolute = false;

    if (arch == kArchX86) {
      if (!m->hasGpdBase())
        type = kRegTypeGpw;
    }
    else {
      if (!m->hasGpdBase())
        type = kRegTypeGpq;
    }

    if (op->getSize() <= 16)
      sb._appendString(AssemblerX86_operandSize[op->getSize()]);

    if (seg < kRegCountSeg)
      sb._appendString(&X86Assembler_segName[seg * 4]);

    sb._appendChar('[');
    switch (m->getMemType()) {
      case kMemTypeBaseIndex:
      case kMemTypeStackIndex:
        // [base + index << shift + displacement]
        X86Assembler_dumpRegister(sb, type, m->getBase());
        break;

      case kMemTypeLabel:
        // [label + index << shift + displacement]
        sb.appendFormat("L%u", m->getBase());
        break;

      case kMemTypeAbsolute:
        // [absolute]
        isAbsolute = true;
        sb.appendUInt(static_cast<uint32_t>(m->getDisplacement()), 16);
        break;
    }

    if (m->hasIndex()) {
      switch (m->getVSib()) {
        case kMemVSibXmm: type = kRegTypeXmm; break;
        case kMemVSibYmm: type = kRegTypeYmm; break;
      }

      sb._appendChar('+');
      X86Assembler_dumpRegister(sb, type, m->getIndex());

      if (m->getShift()) {
        sb._appendChar('*');
        sb._appendChar("1248"[m->getShift() & 3]);
      }
    }

    if (m->getDisplacement() && !isAbsolute) {
      uint32_t base = 10;
      int32_t dispOffset = m->getDisplacement();

      char prefix = '+';
      if (dispOffset < 0) {
        dispOffset = -dispOffset;
        prefix = '-';
      }

      sb._appendChar(prefix);
      if ((loggerOptions & (1 << kLoggerOptionHexDisplacement)) != 0 && dispOffset > 9) {
        sb._appendString("0x", 2);
        base = 16;
      }
      sb.appendUInt(static_cast<uint32_t>(dispOffset), base);
    }

    sb._appendChar(']');
  }
  else if (op->isImm()) {
    const Imm* i = static_cast<const Imm*>(op);
    int64_t val = i->getInt64();

    if ((loggerOptions & (1 << kLoggerOptionHexImmediate)) && static_cast<uint64_t>(val) > 9)
      sb.appendUInt(static_cast<uint64_t>(val), 16);
    else
      sb.appendInt(val, 10);
  }
  else if (op->isLabel()) {
    sb.appendFormat("L%u", op->getId());
  }
  else {
    sb._appendString("None", 4);
  }
}

static bool X86Assembler_dumpInstruction(StringBuilder& sb,
  uint32_t arch,
  uint32_t code, uint32_t options,
  const Operand* o0,
  const Operand* o1,
  const Operand* o2,
  const Operand* o3,
  uint32_t loggerOptions) {

  if (!sb.reserve(sb.getLength() + 128))
    return false;

  // Rex, lock and short prefix.
  if (options & kInstOptionRex)
    sb._appendString("rex ", 4);

  if (options & kInstOptionLock)
    sb._appendString("lock ", 5);

  if (options & kInstOptionShortForm)
    sb._appendString("short ", 6);

  // Dump instruction name.
  sb._appendString(_instInfo[code].getName());

  // Dump operands.
  if (!o0->isNone()) {
    sb._appendChar(' ');
    X86Assembler_dumpOperand(sb, arch, o0, loggerOptions);
  }

  if (!o1->isNone()) {
    sb._appendString(", ", 2);
    X86Assembler_dumpOperand(sb, arch, o1, loggerOptions);
  }

  if (!o2->isNone()) {
    sb._appendString(", ", 2);
    X86Assembler_dumpOperand(sb, arch, o2, loggerOptions);
  }

  if (!o3->isNone()) {
    sb._appendString(", ", 3);
    X86Assembler_dumpOperand(sb, arch, o3, loggerOptions);
  }

  return true;
}

static bool X86Assembler_dumpComment(StringBuilder& sb, size_t len, const uint8_t* binData, size_t binLength, size_t dispSize, const char* comment) {
  size_t currentLength = len;
  size_t commentLength = comment ? StringUtil::nlen(comment, kMaxCommentLength) : 0;

  ASMJIT_ASSERT(binLength >= dispSize);

  if (binLength || commentLength) {
    size_t align = 36;
    char sep = ';';

    for (size_t i = (binLength == 0); i < 2; i++) {
      size_t begin = sb.getLength();

      // Append align.
      if (currentLength < align) {
        if (!sb.appendChars(' ', align - currentLength))
          return false;
      }

      // Append separator.
      if (sep) {
        if (!(sb.appendChar(sep) & sb.appendChar(' ')))
          return false;
      }

      // Append binary data or comment.
      if (i == 0) {
        if (!sb.appendHex(binData, binLength - dispSize))
          return false;
        if (!sb.appendChars('.', dispSize * 2))
          return false;
        if (commentLength == 0)
          break;
      }
      else {
        if (!sb.appendString(comment, commentLength))
          return false;
      }

      currentLength += sb.getLength() - begin;
      align += 22;
      sep = '|';
    }
  }

  return sb.appendChar('\n');
}
#endif // !ASMJIT_DISABLE_LOGGER

// ============================================================================
// [asmjit::x86x64::Assembler - Emit]
// ============================================================================

//! \internal
static const Operand::VRegOp x86PatchedHiRegs[4] = {
  // --------------+---+--------------------------------+--------------+------+
  // Operand       | S | Register Code                  | OperandId    |Unused|
  // --------------+---+--------------------------------+--------------+------+
  { kOperandTypeReg, 1 , (kRegTypePatchedGpbHi << 8) | 4, kInvalidValue, 0, 0 },
  { kOperandTypeReg, 1 , (kRegTypePatchedGpbHi << 8) | 5, kInvalidValue, 0, 0 },
  { kOperandTypeReg, 1 , (kRegTypePatchedGpbHi << 8) | 6, kInvalidValue, 0, 0 },
  { kOperandTypeReg, 1 , (kRegTypePatchedGpbHi << 8) | 7, kInvalidValue, 0, 0 }
};

template<int Arch>
static ASMJIT_INLINE Error X86X64Assembler_emit(X86X64Assembler* self, uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2, const Operand* o3) {
  uint8_t* cursor = self->getCursor();

  uint32_t encoded = o0->getOp() + (o1->getOp() << 3) + (o2->getOp() << 6);
  uint32_t options = self->getOptionsAndClear();

  // Instruction opcode.
  uint32_t opCode;
  // MODR/R opcode or register code.
  uint32_t opReg;

  // REX or VEX prefix data.
  //
  // REX:
  //   0x0008 - REX.W.
  //   0x0040 - Always emit REX prefix.
  //
  // AVX:
  //   0x0008 - AVX.W.
  //   0xF000 - VVVV, zeros by default, see `kVexVVVV`.
  //
  uint32_t opX;

  // MOD/RM, both rmReg and rmMem should refer to the same variable since they
  // are never used together - either rmReg or rmMem.
  union {
    // MODR/M - register code.
    uintptr_t rmReg;
    // MODR/M - Memory operand.
    const Mem* rmMem;
  };

  // Immediate value.
  int64_t imVal = 0;
  // Immediate length.
  uint32_t imLen = 0;

  // Memory operand base register index.
  uint32_t mBase;
  // Memory operand index register index.
  uint32_t mIndex;

  // Label.
  LabelData* label;
  // Displacement offset
  int32_t dispOffset;
  // Displacement size.
  uint32_t dispSize = 0;
  // Displacement relocation id.
  intptr_t relocId;

#if defined(ASMJIT_DEBUG)
  bool assertIllegal = false;
#endif // ASMJIT_DEBUG

  // Invalid instruction.
  const InstInfo* info = &_instInfo[code];
  if (code >= _kInstCount)
    goto _UnknownInst;

  // Grow request happens rarely. C++ compiler generates better code if it is
  // handled at the end of the function.
  if ((size_t)(self->_end - cursor) < 16)
    goto _GrowBuffer;

  // --------------------------------------------------------------------------
  // [Prepare]
  // --------------------------------------------------------------------------

_Prepare:
  opCode = info->_opCode[0];
  opReg  = opCode >> kInstOpCode_O_Shift;
  opX    = info->getFlags() >> (15 - 3);

  if (Arch == kArchX86) {
    // AVX.W prefix.
    opX &= 0x08;

    // Check if one or more register operand is one of AH, BH, CH, or DH and
    // patch them to ensure that the binary code with correct byte-index (4-7)
    // is generated.
    if (o0->isRegType(kRegTypeGpbHi))
      o0 = (const Operand*)(&x86PatchedHiRegs[static_cast<const X86Reg*>(o0)->getRegIndex()]);

    if (o1->isRegType(kRegTypeGpbHi))
      o1 = (const Operand*)(&x86PatchedHiRegs[static_cast<const X86Reg*>(o1)->getRegIndex()]);
  }
  else {
    ASMJIT_ASSERT(kInstOptionRex == 0x40);

    // AVX.W prefix and REX prefix.
    opX |= options;
    opX &= 0x48;

    // Check if one or more register operand is one of BPL, SPL, SIL, DIL and
    // force a REX prefix in such case.
    if (X86Util::isGpbRegOp(*o0)) {
      uint32_t index = static_cast<const X86Reg*>(o0)->getRegIndex();
      if (static_cast<const X86Reg*>(o0)->isGpbLo()) {
        opX |= (index >= 4) << kRexShift;
      }
      else {
        opX |= kRexForbidden;
        o0 = reinterpret_cast<const Operand*>(&x86PatchedHiRegs[index]);
      }
    }

    if (X86Util::isGpbRegOp(*o1)) {
      uint32_t index = static_cast<const X86Reg*>(o1)->getRegIndex();
      if (static_cast<const X86Reg*>(o1)->isGpbLo()) {
        opX |= (index >= 4) << kRexShift;
      }
      else {
        opX |= kRexForbidden;
        o1 = reinterpret_cast<const Operand*>(&x86PatchedHiRegs[index]);
      }
    }
  }

  // --------------------------------------------------------------------------
  // [Lock-Prefix]
  // --------------------------------------------------------------------------

  if (options & kInstOptionLock) {
    if (!info->isLockable())
      goto _IllegalInst;
    EMIT_BYTE(0xF0);
  }

  // --------------------------------------------------------------------------
  // [Group]
  // --------------------------------------------------------------------------

  switch (info->getGroup()) {

    // ------------------------------------------------------------------------
    // [None]
    // ------------------------------------------------------------------------

    case kInstGroupNone:
      goto _EmitDone;

    // ------------------------------------------------------------------------
    // [X86]
    // ------------------------------------------------------------------------

    case kInstGroupX86Op:
      goto _EmitX86Op;

    case kInstGroupX86Rm_B:
      opCode += o0->getSize() != 1;
      // ... Fall through ...

    case kInstGroupX86Rm:
      ADD_66H_P(o0->getSize() == 2);
      ADD_REX_W(o0->getSize() == 8);

      if (encoded == ENC_OPS(Reg, None, None)) {
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86RmReg:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opCode += o1->getSize() != 1;
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86RegRm:
      ADD_66H_P(o0->getSize() == 2);
      ADD_REX_W(o0->getSize() == 8);

      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86M:
      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Arith:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opCode +=(o0->getSize() != 1) + 2;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opCode +=(o0->getSize() != 1) + 2;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opCode += o1->getSize() != 1;
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The remaining instructions use 0x80 opcode.
      opCode = 0x80;

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = IntUtil::isInt8(imVal) ? static_cast<uint32_t>(1) : IntUtil::iMin<uint32_t>(o0->getSize(), 4);
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();

        // Alternate Form - AL, AX, EAX, RAX.
        if (rmReg == 0 && (o0->getSize() == 1 || imLen != 1)) {
          opCode = ((opReg << 3) | (0x04 + (o0->getSize() != 1)));
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);

          imLen = IntUtil::iMin<uint32_t>(o0->getSize(), 4);
          goto _EmitX86OpI;
        }

        opCode += o0->getSize() != 1 ? (imLen != 1 ? 1 : 3) : 0;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = IntUtil::isInt8(imVal) ? static_cast<uint32_t>(1) : IntUtil::iMin<uint32_t>(o0->getSize(), 4);

        opCode += o0->getSize() != 1 ? (imLen != 1 ? 1 : 3) : 0;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86BSwap:
      if (encoded == ENC_OPS(Reg, None, None)) {
        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        opCode += opReg & 0x7;

        ADD_REX_W(o0->getSize() == 8);
        ADD_REX_B(opReg);
        goto _EmitX86Op;
      }
      break;

    case kInstGroupX86BTest:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The remaining instructions use the secondary opcode/r.
      imVal = static_cast<const Imm*>(o1)->getInt64();
      imLen = 1;

      opCode = info->_opCode[1];
      opReg = opCode >> kInstOpCode_O_Shift;

      ADD_66H_P(o0->getSize() == 2);
      ADD_REX_W(o0->getSize() == 8);

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Imm, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Call:
      if (encoded == ENC_OPS(Reg, None, None)) {
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The following instructions use the secondary opcode.
      opCode = info->_opCode[1];

      if (encoded == ENC_OPS(Imm, None, None)) {
        imVal = static_cast<const Imm*>(o0)->getInt64();
        goto _EmitJmpOrCallImm;
      }

      if (encoded == ENC_OPS(Label, None, None)) {
        label = self->getLabelDataById(static_cast<const Label*>(o0)->getId());
        if (label->offset != -1) {
          // Bound label.
          static const intptr_t kRel32Size = 5;
          intptr_t offs = label->offset - (intptr_t)(cursor - self->_buffer);

          ASMJIT_ASSERT(offs <= 0);
          EMIT_OP(opCode);
          EMIT_DWORD(static_cast<int32_t>(offs - kRel32Size));
        }
        else {
          // Non-bound label.
          EMIT_OP(opCode);
          dispOffset = -4;
          dispSize = 4;
          relocId = -1;
          goto _EmitDisplacement;
        }
        goto _EmitDone;
      }
      break;

    case kInstGroupX86Enter:
      if (encoded == ENC_OPS(Imm, Imm, None)) {
        EMIT_BYTE(0xC8);
        EMIT_WORD(static_cast<const Imm*>(o1)->getUInt16());
        EMIT_BYTE(static_cast<const Imm*>(o0)->getUInt8());
        goto _EmitDone;
      }
      break;

    case kInstGroupX86Imul:
      if (encoded == ENC_OPS(Reg, None, None)) {
        opCode = 0xF6 + (o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = 5;
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        opCode = 0xF6 + (o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = 5;
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The following instructions use 0x0FAF opcode.
      opCode = kInstOpCode_MM_0F | 0xAF;

      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      // The following instructions use 0x69/0x6B opcode.
      opCode = 0x6B;

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = 1;

        if (!IntUtil::isInt8(imVal)) {
          opCode -= 2;
          imLen = o0->getSize() == 2 ? 2 : 4;
        }

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = opReg;
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        imVal = static_cast<const Imm*>(o2)->getInt64();
        imLen = 1;

        if (!IntUtil::isInt8(imVal)) {
          opCode -= 2;
          imLen = o0->getSize() == 2 ? 2 : 4;
        }

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        ASMJIT_ASSERT(o0->getSize() != 1);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        imVal = static_cast<const Imm*>(o2)->getInt64();
        imLen = 1;

        if (!IntUtil::isInt8(imVal)) {
          opCode -= 2;
          imLen = o0->getSize() == 2 ? 2 : 4;
        }

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86IncDec:
      if (encoded == ENC_OPS(Reg, None, None)) {
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();

        // INC r16|r32 is not encodable in 64-bit mode.
        if (Arch == kArchX86 && (o0->getSize() == 2 || o0->getSize() == 4)) {
          opCode = info->_opCode[1] + (static_cast<uint32_t>(rmReg) & 0x7);
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86Op;
        }
        else {
          opCode += o0->getSize() != 1;
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86R;
        }
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        opCode += o0->getSize() != 1;
        rmMem = static_cast<const Mem*>(o0);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Int:
      if (encoded == ENC_OPS(Imm, None, None)) {
        imVal = static_cast<const Imm*>(o0)->getInt64();
        uint8_t imm8 = static_cast<uint8_t>(imVal & 0xFF);

        if (imm8 == 0x03) {
          EMIT_OP(opCode);
        }
        else {
          EMIT_OP(opCode + 1);
          EMIT_BYTE(imm8);
        }
        goto _EmitDone;
      }
      break;

    case kInstGroupX86Jcc:
      if (encoded == ENC_OPS(Label, None, None)) {
        label = self->getLabelDataById(static_cast<const Label*>(o0)->getId());

        if (IntUtil::hasBit(self->_features, kCodeGenPredictedJumps)) {
          if (options & kInstOptionTaken)
            EMIT_BYTE(0x3E);
          if (options & kInstOptionNotTaken)
            EMIT_BYTE(0x2E);
        }

        if (label->offset != -1) {
          // Bound label.
          static const intptr_t kRel8Size = 2;
          static const intptr_t kRel32Size = 6;

          intptr_t offs = label->offset - (intptr_t)(cursor - self->_buffer);
          ASMJIT_ASSERT(offs <= 0);

          if ((options & kInstOptionLongForm) == 0 && IntUtil::isInt8(offs - kRel8Size)) {
            EMIT_OP(opCode);
            EMIT_BYTE(offs - kRel8Size);

            options |= kInstOptionShortForm;
            goto _EmitDone;
          }
          else {
            EMIT_BYTE(0x0F);
            EMIT_OP(opCode + 0x10);
            EMIT_DWORD(static_cast<int32_t>(offs - kRel32Size));

            options &= ~kInstOptionShortForm;
            goto _EmitDone;
          }
        }
        else {
          // Non-bound label.
          if (options & kInstOptionShortForm) {
            EMIT_OP(opCode);
            dispOffset = -1;
            dispSize = 1;
            relocId = -1;
            goto _EmitDisplacement;
          }
          else {
            EMIT_BYTE(0x0F);
            EMIT_OP(opCode + 0x10);
            dispOffset = -4;
            dispSize = 4;
            relocId = -1;
            goto _EmitDisplacement;
          }
        }
      }
      break;

    case kInstGroupX86Jecxz:
      if (encoded == ENC_OPS(Reg, Label, None)) {
        ASMJIT_ASSERT(static_cast<const X86Reg*>(o0)->getRegIndex() == kRegIndexCx);

        if ((Arch == kArchX86 && o0->getSize() == 2) ||
            (Arch == kArchX64 && o0->getSize() == 4)) {
          EMIT_BYTE(0x67);
        }

        EMIT_BYTE(0xE3);
        label = self->getLabelDataById(static_cast<const Label*>(o1)->getId());

        if (label->offset != -1) {
          // Bound label.
          intptr_t offs = label->offset - (intptr_t)(cursor - self->_buffer) - 1;
          if (!IntUtil::isInt8(offs))
            goto _IllegalInst;

          EMIT_BYTE(offs);
          goto _EmitDone;
        }
        else {
          // Non-bound label.
          dispOffset = -1;
          dispSize = 1;
          relocId = -1;
          goto _EmitDisplacement;
        }
      }
      break;

    case kInstGroupX86Jmp:
      if (encoded == ENC_OPS(Reg, None, None)) {
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The following instructions use the secondary opcode (0xE9).
      opCode = 0xE9;

      if (encoded == ENC_OPS(Imm, None, None)) {
        imVal = static_cast<const Imm*>(o0)->getInt64();
        goto _EmitJmpOrCallImm;
      }

      if (encoded == ENC_OPS(Label, None, None)) {
        label = self->getLabelDataById(static_cast<const Label*>(o0)->getId());
        if (label->offset != -1) {
          // Bound label.
          const intptr_t kRel8Size = 2;
          const intptr_t kRel32Size = 5;

          intptr_t offs = label->offset - (intptr_t)(cursor - self->_buffer);

          if ((options & kInstOptionLongForm) == 0 && IntUtil::isInt8(offs - kRel8Size)) {
            options |= kInstOptionShortForm;

            EMIT_BYTE(0xEB);
            EMIT_BYTE(offs - kRel8Size);
            goto _EmitDone;
          }
          else {
            options &= ~kInstOptionShortForm;

            EMIT_BYTE(0xE9);
            EMIT_DWORD(static_cast<int32_t>(offs - kRel32Size));
            goto _EmitDone;
          }
        }
        else {
          // Non-bound label.
          if ((options & kInstOptionShortForm) != 0) {
            EMIT_BYTE(0xEB);
            dispOffset = -1;
            dispSize = 1;
            relocId = -1;
            goto _EmitDisplacement;
          }
          else {
            EMIT_BYTE(0xE9);
            dispOffset = -4;
            dispSize = 4;
            relocId = -1;
            goto _EmitDisplacement;
          }
        }
      }
      break;

    case kInstGroupX86Lea:
      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Mov:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();

        // Sreg <- Reg
        if (static_cast<const X86Reg*>(o0)->isSeg()) {
          ASMJIT_ASSERT(static_cast<const X86Reg*>(o1)->isGpw() ||
                        static_cast<const X86Reg*>(o1)->isGpd() ||
                        static_cast<const X86Reg*>(o1)->isGpq() );
          opCode = 0x8E;
          ADD_66H_P(o1->getSize() == 2);
          ADD_REX_W(o1->getSize() == 8);
          goto _EmitX86R;
        }

        // Reg <- Sreg
        if (static_cast<const X86Reg*>(o1)->isSeg()) {
          ASMJIT_ASSERT(static_cast<const X86Reg*>(o0)->isGpw() ||
                        static_cast<const X86Reg*>(o0)->isGpd() ||
                        static_cast<const X86Reg*>(o0)->isGpq() );
          opCode = 0x8C;
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86R;
        }
        // Reg <- Reg
        else {
          ASMJIT_ASSERT(static_cast<const X86Reg*>(o0)->isGpb() ||
                        static_cast<const X86Reg*>(o0)->isGpw() ||
                        static_cast<const X86Reg*>(o0)->isGpd() ||
                        static_cast<const X86Reg*>(o0)->isGpq() );
          opCode = 0x8A + (o0->getSize() != 1);
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86R;
        }
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);

        // Sreg <- Mem
        if (static_cast<const X86Reg*>(o0)->isRegType(kRegTypeSeg)) {
          opCode = 0x8E;
          opReg--;
          ADD_66H_P(o1->getSize() == 2);
          ADD_REX_W(o1->getSize() == 8);
          goto _EmitX86M;
        }
        // Reg <- Mem
        else {
          ASMJIT_ASSERT(static_cast<const X86Reg*>(o0)->isGpb() ||
                        static_cast<const X86Reg*>(o0)->isGpw() ||
                        static_cast<const X86Reg*>(o0)->isGpd() ||
                        static_cast<const X86Reg*>(o0)->isGpq() );
          opCode = 0x8A + (o0->getSize() != 1);
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86M;
        }
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);

        // Mem <- Sreg
        if (static_cast<const X86Reg*>(o1)->isSeg()) {
          opCode = 0x8C;
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86M;
        }
        // Mem <- Reg
        else {
          ASMJIT_ASSERT(static_cast<const X86Reg*>(o1)->isGpb() ||
                        static_cast<const X86Reg*>(o1)->isGpw() ||
                        static_cast<const X86Reg*>(o1)->isGpd() ||
                        static_cast<const X86Reg*>(o1)->isGpq() );
          opCode = 0x88 + (o1->getSize() != 1);
          ADD_66H_P(o1->getSize() == 2);
          ADD_REX_W(o1->getSize() == 8);
          goto _EmitX86M;
        }
      }

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        // 64-bit immediate in 64-bit mode is allowed.
        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = o0->getSize();

        opReg = 0;
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();

        // Optimize instruction size by using 32-bit immediate if possible.
        if (Arch == kArchX64 && imLen == 8 && IntUtil::isInt32(imVal)) {
          opCode = 0xC7;
          ADD_REX_W(1);
          imLen = 4;
          goto _EmitX86R;
        }
        else {
          opCode = 0xB0 + (static_cast<uint32_t>(o0->getSize() != 1) << 3) + (static_cast<uint32_t>(rmReg) & 0x7);
          ADD_REX_W(imLen == 8);
          ADD_REX_B(rmReg);
          goto _EmitX86OpI;
        }
      }

      if (encoded == ENC_OPS(Mem, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = IntUtil::iMin<uint32_t>(o0->getSize(), 4);

        opCode = 0xC6 + (o0->getSize() != 1);
        opReg = 0;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86MovSxZx:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opCode += o1->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opCode += o1->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86MovSxd:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ADD_REX_W(true);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ADD_REX_W(true);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86MovPtr:
      if (encoded == ENC_OPS(Reg, Imm, None)) {
        ASMJIT_ASSERT(static_cast<const GpReg*>(o0)->getRegIndex() == 0);

        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = self->_regSize;
        goto _EmitX86OpI;
      }

      // The following instruction uses the secondary opcode.
      opCode = info->_opCode[1];

      if (encoded == ENC_OPS(Imm, Reg, None)) {
        ASMJIT_ASSERT(static_cast<const GpReg*>(o1)->getRegIndex() == 0);

        opCode += o1->getSize() != 1;
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        imVal = static_cast<const Imm*>(o0)->getInt64();
        imLen = self->_regSize;
        goto _EmitX86OpI;
      }
      break;

    case kInstGroupX86Push:
      if (encoded == ENC_OPS(Reg, None, None)) {
        if (o0->isRegType(kRegTypeSeg)) {
          uint32_t segment = static_cast<const SegReg*>(o0)->getRegIndex();
          ASMJIT_ASSERT(segment < kRegCountSeg);

          if (segment >= kSegFs)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(x86OpCodePushSeg[segment]);
          goto _EmitDone;
        }
        else {
          goto _GroupPop_Gp;
        }
      }

      if (encoded == ENC_OPS(Imm, None, None)) {
        imVal = static_cast<const Imm*>(o0)->getInt64();
        imLen = IntUtil::isInt8(imVal) ? 1 : 4;

        EMIT_BYTE(imLen == 1 ? 0x6A : 0x68);
        goto _EmitImm;
      }
      // ... Fall through ...

    case kInstGroupX86Pop:
      if (encoded == ENC_OPS(Reg, None, None)) {
        if (o0->isRegType(kRegTypeSeg)) {
          uint32_t segment = static_cast<const SegReg*>(o0)->getRegIndex();
          ASMJIT_ASSERT(segment < kRegCountSeg);

          if (segment >= kSegFs)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(x86OpCodePopSeg[segment]);
          goto _EmitDone;
        }
        else {
_GroupPop_Gp:
          ASMJIT_ASSERT(static_cast<const X86Reg*>(o0)->getSize() == 2 ||
                        static_cast<const X86Reg*>(o0)->getSize() == self->_regSize);

          opReg = static_cast<const GpReg*>(o0)->getRegIndex();
          opCode = info->_opCode[1] + (opReg & 7);

          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_B(opReg);

          goto _EmitX86Op;
        }
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        ADD_66H_P(o0->getSize() == 2);

        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Rep:
      // Emit REP 0xF2 or 0xF3 prefix first.
      EMIT_BYTE(0xF2 + opReg);
      goto _EmitX86Op;

    case kInstGroupX86Ret:
      if (encoded == ENC_OPS(None, None, None)) {
        EMIT_BYTE(0xC3);
        goto _EmitDone;
      }

      if (encoded == ENC_OPS(Imm, None, None)) {
        imVal = static_cast<const Imm*>(o0)->getInt64();
        if (imVal == 0) {
          EMIT_BYTE(0xC3);
          goto _EmitDone;
        }
        else {
          EMIT_BYTE(0xC2);
          imLen = 2;
          goto _EmitImm;
        }
      }
      break;

    case kInstGroupX86Rot:
      opCode += o0->getSize() != 1;
      ADD_66H_P(o0->getSize() == 2);
      ADD_REX_W(o0->getSize() == 8);

      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ASMJIT_ASSERT(static_cast<const X86Reg*>(o1)->isRegCode(kRegTypeGpbLo, kRegIndexCx));
        opCode += 2;
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        ASMJIT_ASSERT(static_cast<const X86Reg*>(o1)->isRegCode(kRegTypeGpbLo, kRegIndexCx));
        opCode += 2;
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64() & 0xFF;
        imLen = imVal != 1;
        if (imLen)
          opCode -= 16;
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64() & 0xFF;
        imLen = imVal != 1;
        if (imLen)
          opCode -= 0x10;
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Set:
      if (encoded == ENC_OPS(Reg, None, None)) {
        ASMJIT_ASSERT(o0->getSize() == 1);

        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        ASMJIT_ASSERT(o0->getSize() <= 1);

        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Shlrd:
      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        ASMJIT_ASSERT(o0->getSize() == o1->getSize());

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        imVal = static_cast<const Imm*>(o2)->getInt64();
        imLen = 1;

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, Imm)) {
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        imVal = static_cast<const Imm*>(o2)->getInt64();
        imLen = 1;

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The following instructions use opCode + 1.
      opCode++;

      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        ASMJIT_ASSERT(static_cast<const X86Reg*>(o2)->isRegCode(kRegTypeGpbLo, kRegIndexCx));
        ASMJIT_ASSERT(o0->getSize() == o1->getSize());

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, Reg)) {
        ASMJIT_ASSERT(static_cast<const X86Reg*>(o2)->isRegCode(kRegTypeGpbLo, kRegIndexCx));

        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Test:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ASMJIT_ASSERT(o0->getSize() == o1->getSize());

        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opCode += o1->getSize() != 1;
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }

      // The following instructions use the secondary opcode.
      opCode = info->_opCode[1] + (o0->getSize() != 1);
      opReg = opCode >> kInstOpCode_O_Shift;

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = IntUtil::iMin<uint32_t>(o0->getSize(), 4);

        // Alternate Form - AL, AX, EAX, RAX.
        if (static_cast<const GpReg*>(o0)->getRegIndex() == 0) {
          opCode = 0xA8 + (o0->getSize() != 1);
          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86OpI;
        }

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Imm, None)) {
        ASMJIT_ASSERT(o0->getSize() != 0);

        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = IntUtil::iMin<uint32_t>(o0->getSize(), 4);

        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupX86Xchg:
      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      // ... fall through ...

    case kInstGroupX86Xadd:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o0)->getRegIndex();

        // Special opcode for 'xchg ?ax, reg'.
        if (code == kInstXchg && o0->getSize() > 1 && (opReg == 0 || rmReg == 0)) {
          // One of them is zero, it doesn't matter if the instruction's form is
          // 'xchg ?ax, reg' or 'xchg reg, ?ax'.
          opReg += rmReg;

          // Rex.B (0x01).
          if (Arch == kArchX64) {
            opX += opReg >> 3;
            opReg &= 0x7;
          }

          opCode = 0x90 + opReg;

          ADD_66H_P(o0->getSize() == 2);
          ADD_REX_W(o0->getSize() == 8);
          goto _EmitX86Op;
        }

        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opCode += o1->getSize() != 1;
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Fpu]
    // ------------------------------------------------------------------------

    case kInstGroupFpuOp:
      goto _EmitFpuOp;

    case kInstGroupFpuArith:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const FpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const FpReg*>(o1)->getRegIndex();
        rmReg += opReg;

        // We switch to the alternative opcode if the first operand is zero.
        if (opReg == 0) {
_EmitFpArith_Reg:
          opCode = 0xD800 + ((opCode >> 8) & 0xFF) + static_cast<uint32_t>(rmReg);
          goto _EmitFpuOp;
        }
        else {
          opCode = 0xDC00 + ((opCode >> 0) & 0xFF) + static_cast<uint32_t>(rmReg);
          goto _EmitFpuOp;
        }
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        // 0xD8/0xDC, depends on the size of the memory operand; opReg has been
        // set already.
_EmitFpArith_Mem:
        opCode = (o0->getSize() == 4) ? 0xD8 : 0xDC;
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupFpuCom:
      if (encoded == ENC_OPS(None, None, None)) {
        rmReg = 1;
        goto _EmitFpArith_Reg;
      }

      if (encoded == ENC_OPS(Reg, None, None)) {
        rmReg = static_cast<const FpReg*>(o0)->getRegIndex();
        goto _EmitFpArith_Reg;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        goto _EmitFpArith_Mem;
      }
      break;

    case kInstGroupFpuFldFst:
      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);

        if (o0->getSize() == 4 && (info->_flags & kInstFlagMem4)) {
          goto _EmitX86M;
        }

        if (o0->getSize() == 8 && (info->_flags & kInstFlagMem8)) {
          opCode += 4;
          goto _EmitX86M;
        }

        if (o0->getSize() == 10 && (info->_flags & kInstFlagMem10)) {
          opCode = info->_opCode[1];
          opReg = opCode >> kInstOpCode_O_Shift;
          goto _EmitX86M;
        }
      }

      if (encoded == ENC_OPS(Reg, None, None)) {
        if (code == kInstFld) {
          opCode = 0xD9C0 + static_cast<const FpReg*>(o0)->getRegIndex();
          goto _EmitFpuOp;
        }

        if (code == kInstFst) {
          opCode = 0xDDD0 + static_cast<const FpReg*>(o0)->getRegIndex();
          goto _EmitFpuOp;
        }

        if (code == kInstFstp) {
          opCode = 0xDDD8 + static_cast<const FpReg*>(o0)->getRegIndex();
          goto _EmitFpuOp;
        }
      }
      break;


    case kInstGroupFpuM:
      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);

        if (o0->getSize() == 2 && (info->_flags & kInstFlagMem2)) {
          opCode += 4;
          goto _EmitX86M;
        }

        if (o0->getSize() == 4 && (info->_flags & kInstFlagMem4)) {
          goto _EmitX86M;
        }

        if (o0->getSize() == 8 && (info->_flags & kInstFlagMem8)) {
          opCode = info->_opCode[1];
          opReg = opCode >> kInstOpCode_O_Shift;
          goto _EmitX86M;
        }
      }
      break;

    case kInstGroupFpuRDef:
      if (encoded == ENC_OPS(None, None, None)) {
        opCode += 1;
        goto _EmitFpuOp;
      }
      // ... Fall through ...

    case kInstGroupFpuR:
      if (encoded == ENC_OPS(Reg, None, None)) {
        opCode += static_cast<const FpReg*>(o0)->getRegIndex();
        goto _EmitFpuOp;
      }
      break;

    case kInstGroupFpuStsw:
      if (encoded == ENC_OPS(Reg, None, None)) {
        if (static_cast<const GpReg*>(o0)->getRegIndex() != 0)
          goto _IllegalInst;

        opCode = info->_opCode[1];
        goto _EmitX86Op;
      }

      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Ext]
    // ------------------------------------------------------------------------

    case kInstGroupExtCrc:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ASMJIT_ASSERT(static_cast<const BaseReg*>(o0)->getRegType() == kRegTypeGpd ||
                      static_cast<const BaseReg*>(o0)->getRegType() == kRegTypeGpq);

        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmReg = static_cast<const GpReg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ASMJIT_ASSERT(static_cast<const BaseReg*>(o0)->getRegType() == kRegTypeGpd ||
                      static_cast<const BaseReg*>(o0)->getRegType() == kRegTypeGpq);

        opCode += o0->getSize() != 1;
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtExtract:
      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        ADD_66H_P(static_cast<const X86Reg*>(o1)->isXmm());

        imVal = static_cast<const Imm*>(o2)->getInt64();
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Mem, Reg, Imm)) {
        // Secondary opcode for 'pextrw' instruction (SSE2).
        opCode = info->_opCode[1];
        ADD_66H_P(static_cast<const X86Reg*>(o1)->isXmm());

        imVal = static_cast<const Imm*>(o2)->getInt64();
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtFence:
      if (Arch == kArchX64 && opX) {
        EMIT_BYTE(0x40 | opX);
      }

      EMIT_BYTE(0x0F);
      EMIT_OP(opCode);
      EMIT_BYTE(0xC0 | (opReg << 3));
      goto _EmitDone;

    case kInstGroupExtMov:
    case kInstGroupExtMovNoRexW:
      ASMJIT_ASSERT(info->_opFlags[0] != 0);
      ASMJIT_ASSERT(info->_opFlags[1] != 0);

      // Check parameters Gpd|Gpq|Mm|Xmm <- Gpd|Gpq|Mm|Xmm|Mem|Imm.
      ASMJIT_ASSERT(!((o0->isMem()                && (info->_opFlags[0] & kInstOpMem) == 0) ||
                      (o0->isRegType(kRegTypeMm ) && (info->_opFlags[0] & kInstOpMm ) == 0) ||
                      (o0->isRegType(kRegTypeXmm) && (info->_opFlags[0] & kInstOpXmm) == 0) ||
                      (o0->isRegType(kRegTypeGpd) && (info->_opFlags[0] & kInstOpGd ) == 0) ||
                      (o0->isRegType(kRegTypeGpq) && (info->_opFlags[0] & kInstOpGq ) == 0) ||
                      (o1->isMem()                && (info->_opFlags[1] & kInstOpMem) == 0) ||
                      (o1->isRegType(kRegTypeMm ) && (info->_opFlags[1] & kInstOpMm ) == 0) ||
                      (o1->isRegType(kRegTypeXmm) && (info->_opFlags[1] & kInstOpXmm) == 0) ||
                      (o1->isRegType(kRegTypeGpd) && (info->_opFlags[1] & kInstOpGd ) == 0) ||
                      (o1->isRegType(kRegTypeGpq) && (info->_opFlags[1] & kInstOpGq ) == 0) ));

      // Gp|Mm|Xmm <- Gp|Mm|Xmm
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ADD_REX_W(static_cast<const X86Reg*>(o0)->isGpq() && (info->getGroup() != kInstGroupExtMovNoRexW));
        ADD_REX_W(static_cast<const X86Reg*>(o1)->isGpq() && (info->getGroup() != kInstGroupExtMovNoRexW));

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      // Gp|Mm|Xmm <- Mem
      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ADD_REX_W(static_cast<const X86Reg*>(o0)->isGpq() && (info->getGroup() != kInstGroupExtMovNoRexW));

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      // The following instruction uses opCode[1].
      opCode = info->_opCode[1];

      // Mem <- Gp|Mm|Xmm
      if (encoded == ENC_OPS(Mem, Reg, None)) {
        ADD_REX_W(static_cast<const X86Reg*>(o1)->isGpq() && (info->getGroup() != kInstGroupExtMovNoRexW));

        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtMovBe:
      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ADD_66H_P(o0->getSize() == 2);
        ADD_REX_W(o0->getSize() == 8);

        opReg = static_cast<const GpReg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = info->_opCode[1];

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        ADD_66H_P(o1->getSize() == 2);
        ADD_REX_W(o1->getSize() == 8);

        opReg = static_cast<const GpReg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtMovD:
_EmitMmMovD:
      opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
      ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm());

      // Mm/Xmm <- Gp
      if (encoded == ENC_OPS(Reg, Reg, None) && static_cast<const X86Reg*>(o1)->isGp()) {
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      // Mm/Xmm <- Mem
      if (encoded == ENC_OPS(Reg, Mem, None)) {
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      // The following instructions use the secondary opcode.
      opCode = info->_opCode[1];
      opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
      ADD_66H_P(static_cast<const X86Reg*>(o1)->isXmm());

      // Gp <- Mm/Xmm
      if (encoded == ENC_OPS(Reg, Reg, None) && static_cast<const X86Reg*>(o0)->isGp()) {
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }

      // Mem <- Mm/Xmm
      if (encoded == ENC_OPS(Mem, Reg, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtMovQ:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();

        // Mm <- Mm
        if (static_cast<const X86Reg*>(o0)->isMm() && static_cast<const X86Reg*>(o1)->isMm()) {
          opCode = kInstOpCode_PP_00 | kInstOpCode_MM_0F | 0x6F;
          goto _EmitX86R;
        }

        // Xmm <- Xmm
        if (static_cast<const X86Reg*>(o0)->isXmm() && static_cast<const X86Reg*>(o1)->isXmm()) {
          opCode = kInstOpCode_PP_F3 | kInstOpCode_MM_0F | 0x7E;
          goto _EmitX86R;
        }

        // Mm <- Xmm (Movdq2q)
        if (static_cast<const X86Reg*>(o0)->isMm() && static_cast<const X86Reg*>(o1)->isXmm()) {
          opCode = kInstOpCode_PP_F2 | kInstOpCode_MM_0F | 0xD6;
          goto _EmitX86R;
        }

        // Xmm <- Mm (Movq2dq)
        if (static_cast<const X86Reg*>(o0)->isXmm() && static_cast<const X86Reg*>(o1)->isMm()) {
          opCode = kInstOpCode_PP_F3 | kInstOpCode_MM_0F | 0xD6;
          goto _EmitX86R;
        }
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);

        // Mm <- Mem
        if (static_cast<const X86Reg*>(o0)->isMm()) {
          opCode = kInstOpCode_PP_00 | kInstOpCode_MM_0F | 0x6F;
          goto _EmitX86M;
        }

        // Xmm <- Mem
        if (static_cast<const X86Reg*>(o0)->isXmm()) {
          opCode = kInstOpCode_PP_F3 | kInstOpCode_MM_0F | 0x7E;
          goto _EmitX86M;
        }
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);

        // Mem <- Mm
        if (static_cast<const X86Reg*>(o1)->isMm()) {
          opCode = kInstOpCode_PP_00 | kInstOpCode_MM_0F | 0x7F;
          goto _EmitX86M;
        }

        // Mem <- Xmm
        if (static_cast<const X86Reg*>(o1)->isXmm()) {
          opCode = kInstOpCode_PP_66 | kInstOpCode_MM_0F | 0xD6;
          goto _EmitX86M;
        }
      }

      if (Arch == kArchX64) {
        // Movq in other case is simply promoted Movd instruction by REX prefix.
        ADD_REX_W(true);

        opCode = kInstOpCode_PP_00 | kInstOpCode_MM_0F | 0x6E;
        goto _EmitMmMovD;
      }
      break;

    case kInstGroupExtPrefetch:
      if (encoded == ENC_OPS(Mem, Imm, None)) {
        opReg = static_cast<const Imm*>(o1)->getUInt32() & 0x3;
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtRm_PQ:
      ADD_66H_P(o0->isRegType(kRegTypeXmm) || o1->isRegType(kRegTypeXmm));
      // ... Fall through ...

    case kInstGroupExtRm_Q:
      ADD_REX_W(o0->isRegType(kRegTypeGpq) || o1->isRegType(kRegTypeGpq) || (o1->isMem() && o1->getSize() == 8));
      // ... Fall through ...

    case kInstGroupExtRm:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtRm_P:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm() || static_cast<const X86Reg*>(o1)->isXmm());

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm());

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtRmRi:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = info->_opCode[1];
      opReg = opCode >> kInstOpCode_O_Shift;

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = 1;

        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }
      break;

    case kInstGroupExtRmRi_P:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm() || static_cast<const X86Reg*>(o1)->isXmm());

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm());

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = info->_opCode[1];
      opReg = opCode >> kInstOpCode_O_Shift;

      if (encoded == ENC_OPS(Reg, Imm, None)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm());

        imVal = static_cast<const Imm*>(o1)->getInt64();
        imLen = 1;

        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitX86R;
      }
      break;

    case kInstGroupExtRmi:
      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    case kInstGroupExtRmi_P:
      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm() || static_cast<const X86Reg*>(o1)->isXmm());

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        ADD_66H_P(static_cast<const X86Reg*>(o0)->isXmm());

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Group - 3dNow]
    // ------------------------------------------------------------------------

    case kInstGroup3dNow:
      // Every 3dNow instruction starts with 0x0F0F and the actual opcode is
      // stored as 8-bit immediate.
      imVal = opCode & 0xFF;
      imLen = 1;

      opCode = kInstOpCode_MM_0F | 0x0F;
      opReg = static_cast<const X86Reg*>(o0)->getRegIndex();

      if (encoded == ENC_OPS(Reg, Reg, None)) {
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitX86R;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Avx]
    // ------------------------------------------------------------------------

    case kInstGroupAvxOp:
      goto _EmitAvxOp;

    case kInstGroupAvxM:
      if (encoded == ENC_OPS(Mem, None, None)) {
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxMr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxMr:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxMri_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxMri:
      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Mem, Reg, Imm)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRm_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRm:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRmi_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRmi:
      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvm_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvm:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
_EmitAvxRvm:
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvmr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvmr:
      if (!o3->isReg())
        goto _IllegalInst;

      imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvmi_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvmi:
      if (!o3->isImm())
        goto _IllegalInst;

      imVal = static_cast<const Imm*>(o3)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRmv:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRmvi:
      if (!o3->isImm())
        goto _IllegalInst;

      imVal = static_cast<const Imm*>(o3)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRmMr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRmMr:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }

      // The following instruction uses the secondary opcode.
      opCode = info->_opCode[1];

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvmRmi_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvmRmi:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }

      // The following instructions use the secondary opcode.
      opCode &= kInstOpCode_L_Mask;
      opCode |= info->_opCode[1];

      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvmMr:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }

      // The following instructions use the secondary opcode.
      opCode = info->_opCode[1];

      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvmMvr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvmMvr:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= kInstOpCode_L_Mask;
      opCode |= info->_opCode[1];

      if (encoded == ENC_OPS(Mem, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvmVmi_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvmVmi:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitAvxM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= kInstOpCode_L_Mask;
      opCode |= info->_opCode[1];
      opReg = opCode >> kInstOpCode_O_Shift;

      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxVm:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxVmi_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxVmi:
      imVal = static_cast<const Imm*>(o3)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxRvrmRvmr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupAvxRvrmRvmr:
      if (encoded == ENC_OPS(Reg, Reg, Reg) && o3->isReg()) {
        imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();

        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Reg) && o3->isMem()) {
        imVal = static_cast<const X86Reg*>(o2)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o3);

        ADD_VEX_W(true);
        goto _EmitAvxM;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem) && o3->isReg()) {
        imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);

        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxMovSsSd:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        goto _EmitAvxRvm;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opX  |= static_cast<const X86Reg*>(o0)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitAvxM;
      }

      if (encoded == ENC_OPS(Mem, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        rmMem = static_cast<const Mem*>(o0);
        goto _EmitAvxM;
      }
      break;

    case kInstGroupAvxGatherEx:
      if (encoded == ENC_OPS(Reg, Mem, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);

        uint32_t vSib = rmMem->getVSib();
        if (vSib == kMemVSibGpz)
          goto _IllegalInst;

        ADD_VEX_L(vSib == kMemVSibYmm);
        goto _EmitAvxV;
      }
      break;

    case kInstGroupAvxGather:
      if (encoded == ENC_OPS(Reg, Mem, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);

        uint32_t vSib = rmMem->getVSib();
        if (vSib == kMemVSibGpz)
          goto _IllegalInst;

        ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o2)->isYmm());
        goto _EmitAvxV;
      }
      break;

    // ------------------------------------------------------------------------
    // [FMA4]
    // ------------------------------------------------------------------------

    case kInstGroupFma4_P:
      // It's fine to just check the first operand, second is just for sanity.
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupFma4:
      if (encoded == ENC_OPS(Reg, Reg, Reg) && o3->isReg()) {
        imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();

        goto _EmitAvxR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Reg) && o3->isMem()) {
        imVal = static_cast<const X86Reg*>(o2)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o3);

        ADD_VEX_W(true);
        goto _EmitAvxM;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem) && o3->isReg()) {
        imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);

        goto _EmitAvxM;
      }
      break;

    // ------------------------------------------------------------------------
    // [XOP]
    // ------------------------------------------------------------------------

    case kInstGroupXopRm_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupXopRm:
      if (encoded == ENC_OPS(Reg, Reg, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Mem, None)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitXopM;
      }
      break;

    case kInstGroupXopRvmRmv:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();

        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);

        goto _EmitXopM;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);

        ADD_VEX_W(true);
        goto _EmitXopM;
      }

      break;

    case kInstGroupXopRvmRmi:
      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o2)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o1);

        goto _EmitXopM;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);

        ADD_VEX_W(true);
        goto _EmitXopM;
      }

      // The following instructions use the secondary opcode.
      opCode = info->_opCode[1];

      imVal = static_cast<const Imm*>(o2)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o1)->getRegIndex();
        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Mem, Imm)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmMem = static_cast<const Mem*>(o1);
        goto _EmitXopM;
      }
      break;

    case kInstGroupXopRvmr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupXopRvmr:
      if (!o3->isReg())
        goto _IllegalInst;

      imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitXopM;
      }
      break;

    case kInstGroupXopRvmi_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupXopRvmi:
      if (!o3->isImm())
        goto _IllegalInst;

      imVal = static_cast<const Imm*>(o3)->getInt64();
      imLen = 1;

      if (encoded == ENC_OPS(Reg, Reg, Reg)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem)) {
        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);
        goto _EmitXopM;
      }
      break;

    case kInstGroupXopRvrmRvmr_P:
      ADD_VEX_L(static_cast<const X86Reg*>(o0)->isYmm() || static_cast<const X86Reg*>(o1)->isYmm());
      // ... Fall through ...

    case kInstGroupXopRvrmRvmr:
      if (encoded == ENC_OPS(Reg, Reg, Reg) && o3->isReg()) {
        imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmReg = static_cast<const X86Reg*>(o2)->getRegIndex();

        goto _EmitXopR;
      }

      if (encoded == ENC_OPS(Reg, Reg, Reg) && o3->isMem()) {
        imVal = static_cast<const X86Reg*>(o2)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o3);

        ADD_VEX_W(true);
        goto _EmitXopM;
      }

      if (encoded == ENC_OPS(Reg, Reg, Mem) && o3->isReg()) {
        imVal = static_cast<const X86Reg*>(o3)->getRegIndex() << 4;
        imLen = 1;

        opReg = static_cast<const X86Reg*>(o0)->getRegIndex();
        opX  |= static_cast<const X86Reg*>(o1)->getRegIndex() << kVexVVVVShift;
        rmMem = static_cast<const Mem*>(o2);

        goto _EmitXopM;
      }
      break;
  }

  // --------------------------------------------------------------------------
  // [Illegal]
  // --------------------------------------------------------------------------

_IllegalInst:
  self->setError(kErrorIllegalInst);
#if defined(ASMJIT_DEBUG)
  assertIllegal = true;
#endif // ASMJIT_DEBUG
  goto _EmitDone;

_IllegalAddr:
  self->setError(kErrorIllegalAddresing);
#if defined(ASMJIT_DEBUG)
  assertIllegal = true;
#endif // ASMJIT_DEBUG
  goto _EmitDone;

_IllegalDisp:
  self->setError(kErrorIllegalDisplacement);
#if defined(ASMJIT_DEBUG)
  assertIllegal = true;
#endif // ASMJIT_DEBUG
  goto _EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - X86]
  // --------------------------------------------------------------------------

_EmitX86Op:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Rex prefix (64-bit only).
  if (Arch == kArchX64 && opX) {
    opX |= 0x40;
    EMIT_BYTE(opX);
    if (opX >= kRexForbidden)
      goto _IllegalInst;
  }

  // Instruction opcodes.
  EMIT_MM(opCode);
  EMIT_OP(opCode);
  goto _EmitDone;

_EmitX86OpI:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Rex prefix (64-bit only).
  if (Arch == kArchX64 && opX) {
    opX |= 0x40;
    EMIT_BYTE(opX);
    if (opX >= kRexForbidden)
      goto _IllegalInst;
  }

  // Instruction opcodes.
  EMIT_MM(opCode);
  EMIT_OP(opCode);
  goto _EmitImm;

_EmitX86R:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Rex prefix (64-bit only).
  if (Arch == kArchX64) {
    opX += static_cast<uint32_t>(opReg & 0x08) >> 1; // Rex.R (0x04).
    opX += static_cast<uint32_t>(rmReg) >> 3;        // Rex.B (0x01).

    if (opX) {
      opX |= 0x40;
      EMIT_BYTE(opX);

      if (opX >= kRexForbidden)
        goto _IllegalInst;

      opReg &= 0x7;
      rmReg &= 0x7;
    }
  }

  // Instruction opcodes.
  EMIT_MM(opCode);
  EMIT_OP(opCode);

  // ModR.
  EMIT_BYTE(x86EncodeMod(3, opReg, static_cast<uint32_t>(rmReg)));

  if (imLen != 0)
    goto _EmitImm;
  else
    goto _EmitDone;

_EmitX86M:
  ASMJIT_ASSERT(rmMem != NULL);
  ASMJIT_ASSERT(rmMem->getOp() == kOperandTypeMem);

  mBase = rmMem->getBase();
  mIndex = rmMem->getIndex();

  // Size override prefix.
  if (rmMem->hasBaseOrIndex()) {
    if (Arch == kArchX86) {
      if (!rmMem->hasGpdBase())
        EMIT_BYTE(0x67);
    }
    else {
      if (rmMem->hasGpdBase())
        EMIT_BYTE(0x67);
    }
  }

  // Segment override prefix.
  if (rmMem->hasSegment()) {
    EMIT_BYTE(x86SegmentPrefix[rmMem->getSegment()]);
  }

  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Rex prefix (64-bit only).
  if (Arch == kArchX64) {
    opX += static_cast<uint32_t>(opReg      & 8) >> 1; // Rex.R (0x04).
    opX += static_cast<uint32_t>(mIndex - 8 < 8) << 1; // Rex.X (0x02).
    opX += static_cast<uint32_t>(mBase  - 8 < 8);      // Rex.B (0x01).

    if (opX) {
      opX |= 0x40;
      EMIT_BYTE(opX);

      if (opX >= kRexForbidden)
        goto _IllegalInst;

      opReg &= 0x7;
    }

    mBase &= 0x7;
  }

  // Instruction opcodes.
  EMIT_MM(opCode);
  EMIT_OP(opCode);

_EmitSib:
  dispOffset = rmMem->getDisplacement();
  if (rmMem->isBaseIndexType()) {
    if (mIndex >= kInvalidReg) {
      if (mBase == kRegIndexSp) {
        if (dispOffset == 0) {
          // [Esp/Rsp/R12].
          EMIT_BYTE(x86EncodeMod(0, opReg, 4));
          EMIT_BYTE(x86EncodeSib(0, 4, 4));
        }
        else if (IntUtil::isInt8(dispOffset)) {
          // [Esp/Rsp/R12 + Disp8].
          EMIT_BYTE(x86EncodeMod(1, opReg, 4));
          EMIT_BYTE(x86EncodeSib(0, 4, 4));
          EMIT_BYTE(static_cast<int8_t>(dispOffset));
        }
        else {
          // [Esp/Rsp/R12 + Disp32].
          EMIT_BYTE(x86EncodeMod(2, opReg, 4));
          EMIT_BYTE(x86EncodeSib(0, 4, 4));
          EMIT_DWORD(static_cast<int32_t>(dispOffset));
        }
      }
      else if (mBase != kRegIndexBp && dispOffset == 0) {
        // [Base].
        EMIT_BYTE(x86EncodeMod(0, opReg, mBase));
      }
      else if (IntUtil::isInt8(dispOffset)) {
        // [Base + Disp8].
        EMIT_BYTE(x86EncodeMod(1, opReg, mBase));
        EMIT_BYTE(static_cast<int8_t>(dispOffset));
      }
      else {
        // [Base + Disp32].
        EMIT_BYTE(x86EncodeMod(2, opReg, mBase));
        EMIT_DWORD(static_cast<int32_t>(dispOffset));
      }
    }
    else {
      uint32_t shift = rmMem->getShift();

      // Esp/Rsp/R12 register can't be used as an index.
      if (Arch == kArchX64)
        mIndex &= 0x7;
      ASMJIT_ASSERT(mIndex != kRegIndexSp);

      if (mBase != kRegIndexBp && dispOffset == 0) {
        // [Base + Index * Scale].
        EMIT_BYTE(x86EncodeMod(0, opReg, 4));
        EMIT_BYTE(x86EncodeSib(shift, mIndex, mBase));
      }
      else if (IntUtil::isInt8(dispOffset)) {
        // [Base + Index * Scale + Disp8].
        EMIT_BYTE(x86EncodeMod(1, opReg, 4));
        EMIT_BYTE(x86EncodeSib(shift, mIndex, mBase));
        EMIT_BYTE(static_cast<int8_t>(dispOffset));
      }
      else {
        // [Base + Index * Scale + Disp32].
        EMIT_BYTE(x86EncodeMod(2, opReg, 4));
        EMIT_BYTE(x86EncodeSib(shift, mIndex, mBase));
        EMIT_DWORD(static_cast<int32_t>(dispOffset));
      }
    }
  }
  else if (Arch == kArchX86) {
    if (mIndex >= kInvalidReg) {
      // [Disp32].
      EMIT_BYTE(x86EncodeMod(0, opReg, 5));
    }
    else {
      // [Index * Scale + Disp32].
      uint32_t shift = rmMem->getShift();
      ASMJIT_ASSERT(mIndex != kRegIndexSp);

      EMIT_BYTE(x86EncodeMod(0, opReg, 4));
      EMIT_BYTE(x86EncodeSib(shift, mIndex, 5));
    }

    if (rmMem->getMemType() == kMemTypeLabel) {
      // Relative->Absolute [x86 mode].
      label = self->getLabelDataById(rmMem->_vmem.base);
      relocId = self->_relocData.getLength();

      RelocData reloc;
      reloc.type = kRelocRelToAbs;
      reloc.size = 4;
      reloc.from = static_cast<Ptr>((uintptr_t)(cursor - self->_buffer));
      reloc.data = static_cast<SignedPtr>(dispOffset);

      if (self->_relocData.append(reloc) != kErrorOk)
        return self->setError(kErrorNoHeapMemory);

      if (label->offset != -1) {
        // Bound label.
        reloc.data += static_cast<SignedPtr>(label->offset);
        EMIT_DWORD(0);
      }
      else {
        // Non-bound label.
        dispOffset = -4 - imLen;
        dispSize = 4;
        goto _EmitDisplacement;
      }
    }
    else {
      // [Disp32].
      EMIT_DWORD(static_cast<int32_t>(dispOffset));
    }
  }
  else {
    if (rmMem->getMemType() == kMemTypeLabel) {
      // [RIP + Disp32].
      label = self->getLabelDataById(rmMem->_vmem.base);

      // Indexing is invalid.
      if (mIndex < kInvalidReg)
        goto _IllegalDisp;

      EMIT_BYTE(x86EncodeMod(0, opReg, 5));
      dispOffset -= (4 + imLen);

      if (label->offset != -1) {
        // Bound label.
        dispOffset += label->offset - static_cast<int32_t>((intptr_t)(cursor - self->_buffer));
        EMIT_DWORD(static_cast<int32_t>(dispOffset));
      }
      else {
        // Non-bound label.
        dispSize = 4;
        relocId = -1;
        goto _EmitDisplacement;
      }
    }
    else {
      EMIT_BYTE(x86EncodeMod(0, opReg, 4));
      if (mIndex >= kInvalidReg) {
        // [Disp32].
        EMIT_BYTE(x86EncodeSib(0, 4, 5));
      }
      else {
        // [Disp32 + Index * Scale].
        mIndex &= 0x7;
        ASMJIT_ASSERT(mIndex != kRegIndexSp);

        uint32_t shift = rmMem->getShift();
        EMIT_BYTE(x86EncodeSib(shift, mIndex, 5));
      }

      EMIT_DWORD(static_cast<int32_t>(dispOffset));
    }
  }

  if (imLen == 0)
    goto _EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Imm]
  // --------------------------------------------------------------------------

_EmitImm:
  switch (imLen) {
    case 1: EMIT_BYTE (imVal & 0x000000FF); break;
    case 2: EMIT_WORD (imVal & 0x0000FFFF); break;
    case 4: EMIT_DWORD(imVal & 0xFFFFFFFF); break;
    case 8: EMIT_QWORD(imVal             ); break;

    default:
      ASMJIT_ASSERT(!"Reached");
  }
  goto _EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Fpu]
  // --------------------------------------------------------------------------

_EmitFpuOp:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Instruction opcodes.
  EMIT_OP(opCode >> 8);
  EMIT_OP(opCode);
  goto _EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Avx]
  // --------------------------------------------------------------------------

#define EMIT_AVX_M \
  ASMJIT_ASSERT(rmMem != NULL); \
  ASMJIT_ASSERT(rmMem->getOp() == kOperandTypeMem); \
  \
  if (rmMem->hasSegment()) { \
    EMIT_BYTE(x86SegmentPrefix[rmMem->getSegment()]); \
  } \
  \
  mBase = rmMem->getBase(); \
  mIndex = rmMem->getIndex(); \
  \
  { \
    uint32_t vex_XvvvvLpp; \
    uint32_t vex_rxbmmmmm; \
    \
    vex_XvvvvLpp  = (opCode >> (kInstOpCode_L_Shift - 2)) & 0x04; \
    vex_XvvvvLpp += (opCode >> (kInstOpCode_PP_Shift)) & 0x03; \
    vex_XvvvvLpp += (opX >> (kVexVVVVShift - 3)); \
    vex_XvvvvLpp += (opX << 4) & 0x80; \
    \
    vex_rxbmmmmm  = (opCode >> kInstOpCode_MM_Shift) & 0x1F; \
    vex_rxbmmmmm += static_cast<uint32_t>(mBase  - 8 < 8) << 5; \
    vex_rxbmmmmm += static_cast<uint32_t>(mIndex - 8 < 8) << 6; \
    \
    if (vex_rxbmmmmm != 0x01 || vex_XvvvvLpp >= 0x80 || (options & kInstOptionVex3) != 0) { \
      vex_rxbmmmmm |= static_cast<uint32_t>(opReg << 4) & 0x80; \
      vex_rxbmmmmm ^= 0xE0; \
      vex_XvvvvLpp ^= 0x78; \
      \
      EMIT_BYTE(kVex3Byte); \
      EMIT_BYTE(vex_rxbmmmmm); \
      EMIT_BYTE(vex_XvvvvLpp); \
      EMIT_OP(opCode); \
    } \
    else { \
      vex_XvvvvLpp |= static_cast<uint32_t>(opReg << 4) & 0x80; \
      vex_XvvvvLpp ^= 0xF8; \
      \
      EMIT_BYTE(kVex2Byte); \
      EMIT_BYTE(vex_XvvvvLpp); \
      EMIT_OP(opCode); \
    } \
  } \
  \
  mBase &= 0x7; \
  opReg &= 0x7;

_EmitAvxOp:
  {
    uint32_t vex_XvvvvLpp;

    vex_XvvvvLpp  = (opCode >> (kInstOpCode_L_Shift - 2)) & 0x04;
    vex_XvvvvLpp |= (opCode >> (kInstOpCode_PP_Shift));
    vex_XvvvvLpp |= 0xF8;

    // Encode 3-byte VEX prefix only if specified in options.
    if ((options & kInstOptionVex3) != 0) {
      uint32_t vex_rxbmmmmm = (opCode >> kInstOpCode_MM_Shift) | 0xE0;

      EMIT_BYTE(kVex3Byte);
      EMIT_OP(vex_rxbmmmmm);
      EMIT_OP(vex_XvvvvLpp);
      EMIT_OP(opCode);
    }
    else {
      EMIT_BYTE(kVex2Byte);
      EMIT_OP(vex_XvvvvLpp);
      EMIT_OP(opCode);
    }
  }
  goto _EmitDone;

_EmitAvxR:
  {
    uint32_t vex_XvvvvLpp;
    uint32_t vex_rxbmmmmm;

    vex_XvvvvLpp  = (opCode >> (kInstOpCode_L_Shift - 2)) & 0x04;
    vex_XvvvvLpp |= (opCode >> (kInstOpCode_PP_Shift));
    vex_XvvvvLpp |= (opX    >> (kVexVVVVShift - 3));
    vex_XvvvvLpp |= (opX << 4) & 0x80;

    vex_rxbmmmmm  = (opCode >> kInstOpCode_MM_Shift) & 0x1F;
    vex_rxbmmmmm |= (rmReg << 2) & 0x20;

    if (vex_rxbmmmmm != 0x01 || vex_XvvvvLpp >= 0x80 || (options & kInstOptionVex3) != 0) {
      vex_rxbmmmmm |= static_cast<uint32_t>(opReg & 0x08) << 4;
      vex_rxbmmmmm ^= 0xE0;
      vex_XvvvvLpp ^= 0x78;

      EMIT_BYTE(kVex3Byte);
      EMIT_OP(vex_rxbmmmmm);
      EMIT_OP(vex_XvvvvLpp);
      EMIT_OP(opCode);

      rmReg &= 0x07;
    }
    else {
      vex_XvvvvLpp += static_cast<uint32_t>(opReg & 0x08) << 4;
      vex_XvvvvLpp ^= 0xF8;

      EMIT_BYTE(kVex2Byte);
      EMIT_OP(vex_XvvvvLpp);
      EMIT_OP(opCode);
    }
  }

  EMIT_BYTE(x86EncodeMod(3, opReg, static_cast<uint32_t>(rmReg)));

  if (imLen == 0)
    goto _EmitDone;

  EMIT_BYTE(imVal & 0xFF);
  goto _EmitDone;

_EmitAvxM:
  EMIT_AVX_M
  goto _EmitSib;

_EmitAvxV:
  EMIT_AVX_M

  if (mIndex >= kInvalidReg)
    goto _IllegalInst;

  if (Arch == kArchX64)
    mIndex &= 0x7;

  dispOffset = rmMem->getDisplacement();
  if (rmMem->isBaseIndexType()) {
    uint32_t shift = rmMem->getShift();

    if (mBase != kRegIndexBp && dispOffset == 0) {
      // [Base + Index * Scale].
      EMIT_BYTE(x86EncodeMod(0, opReg, 4));
      EMIT_BYTE(x86EncodeSib(shift, mIndex, mBase));
    }
    else if (IntUtil::isInt8(dispOffset)) {
      // [Base + Index * Scale + Disp8].
      EMIT_BYTE(x86EncodeMod(1, opReg, 4));
      EMIT_BYTE(x86EncodeSib(shift, mIndex, mBase));
      EMIT_BYTE(static_cast<int8_t>(dispOffset));
    }
    else {
      // [Base + Index * Scale + Disp32].
      EMIT_BYTE(x86EncodeMod(2, opReg, 4));
      EMIT_BYTE(x86EncodeSib(shift, mIndex, mBase));
      EMIT_DWORD(static_cast<int32_t>(dispOffset));
    }
  }
  else {
    // [Index * Scale + Disp32].
    uint32_t shift = rmMem->getShift();

    EMIT_BYTE(x86EncodeMod(0, opReg, 4));
    EMIT_BYTE(x86EncodeSib(shift, mIndex, 5));

    if (rmMem->getMemType() == kMemTypeLabel) {
      if (Arch == kArchX64)
        goto _IllegalAddr;

      // Relative->Absolute [x86 mode].
      label = self->getLabelDataById(rmMem->_vmem.base);
      relocId = self->_relocData.getLength();

      RelocData reloc;
      reloc.type = kRelocRelToAbs;
      reloc.size = 4;
      reloc.from = static_cast<Ptr>((uintptr_t)(cursor - self->_buffer));
      reloc.data = static_cast<SignedPtr>(dispOffset);

      if (self->_relocData.append(reloc) != kErrorOk)
        return self->setError(kErrorNoHeapMemory);

      if (label->offset != -1) {
        // Bound label.
        reloc.data += static_cast<SignedPtr>(label->offset);
        EMIT_DWORD(0);
      }
      else {
        // Non-bound label.
        dispOffset = -4 - imLen;
        dispSize = 4;
        goto _EmitDisplacement;
      }
    }
    else {
      // [Disp32].
      EMIT_DWORD(static_cast<int32_t>(dispOffset));
    }
  }
  goto _EmitDone;

  // --------------------------------------------------------------------------
  // [Xop]
  // --------------------------------------------------------------------------

#define EMIT_XOP_M \
  ASMJIT_ASSERT(rmMem != NULL); \
  ASMJIT_ASSERT(rmMem->getOp() == kOperandTypeMem); \
  \
  if (rmMem->hasSegment()) { \
    EMIT_BYTE(x86SegmentPrefix[rmMem->getSegment()]); \
  } \
  \
  mBase = rmMem->getBase(); \
  mIndex = rmMem->getIndex(); \
  \
  { \
    uint32_t vex_XvvvvLpp; \
    uint32_t vex_rxbmmmmm; \
    \
    vex_XvvvvLpp  = (opCode >> (kInstOpCode_L_Shift - 2)) & 0x04; \
    vex_XvvvvLpp += (opCode >> (kInstOpCode_PP_Shift)) & 0x03; \
    vex_XvvvvLpp += (opX >> (kVexVVVVShift - 3)); \
    vex_XvvvvLpp += (opX << 4) & 0x80; \
    \
    vex_rxbmmmmm  = (opCode >> kInstOpCode_MM_Shift) & 0x1F; \
    vex_rxbmmmmm += static_cast<uint32_t>(mBase  - 8 < 8) << 5; \
    vex_rxbmmmmm += static_cast<uint32_t>(mIndex - 8 < 8) << 6; \
    \
    vex_rxbmmmmm |= static_cast<uint32_t>(opReg << 4) & 0x80; \
    vex_rxbmmmmm ^= 0xE0; \
    vex_XvvvvLpp ^= 0x78; \
    \
    EMIT_BYTE(kXopByte); \
    EMIT_BYTE(vex_rxbmmmmm); \
    EMIT_BYTE(vex_XvvvvLpp); \
    EMIT_OP(opCode); \
  } \
  \
  mBase &= 0x7; \
  opReg &= 0x7;

_EmitXopR:
  {
    uint32_t xop_XvvvvLpp;
    uint32_t xop_rxbmmmmm;

    xop_XvvvvLpp  = (opCode >> (kInstOpCode_L_Shift - 2)) & 0x04;
    xop_XvvvvLpp |= (opCode >> (kInstOpCode_PP_Shift));
    xop_XvvvvLpp |= (opX    >> (kVexVVVVShift - 3));
    xop_XvvvvLpp |= (opX << 4) & 0x80;

    xop_rxbmmmmm  = (opCode >> kInstOpCode_MM_Shift) & 0x1F;
    xop_rxbmmmmm |= (rmReg << 2) & 0x20;

    xop_rxbmmmmm |= static_cast<uint32_t>(opReg & 0x08) << 4;
    xop_rxbmmmmm ^= 0xE0;
    xop_XvvvvLpp ^= 0x78;

    EMIT_BYTE(kXopByte);
    EMIT_OP(xop_rxbmmmmm);
    EMIT_OP(xop_XvvvvLpp);
    EMIT_OP(opCode);

    rmReg &= 0x07;
  }

  EMIT_BYTE(x86EncodeMod(3, opReg, static_cast<uint32_t>(rmReg)));

  if (imLen == 0)
    goto _EmitDone;

  EMIT_BYTE(imVal & 0xFF);
  goto _EmitDone;

_EmitXopM:
  EMIT_XOP_M
  goto _EmitSib;

  // --------------------------------------------------------------------------
  // [Emit - Jump/Call to an Immediate]
  // --------------------------------------------------------------------------

  // 64-bit mode requires a trampoline if a relative displacement doesn't fit
  // into 32-bit integer. Old version of AsmJit used to emit jump to a section
  // which contained another jump followed by an address (it worked well for
  // both `jmp` and `call`), but it required to reserve 14-bytes for a possible
  // trampoline.
  //
  // Instead of using 5-byte `jmp/call` and reserving 14 bytes required by the
  // trampoline, it's better to use 6-byte `jmp/call` (prefixing it with REX
  // prefix) and to patch the `jmp/call` instruction itself.
_EmitJmpOrCallImm:
  {
    // Emit REX prefix (64-bit).
    //
    // Does nothing, but allows to path the instruction in case a trampoline is
    // needed.
    if (Arch == kArchX64) {
      EMIT_OP(0x40);
    }

    // Both `jmp` and `call` instructions have a single-byte opcode.
    EMIT_OP(opCode);

    RelocData rd;
    rd.type = kRelocTrampoline;
    rd.size = 4;
    rd.from = (intptr_t)(cursor - self->_buffer);
    rd.data = static_cast<SignedPtr>(imVal);

    if (self->_relocData.append(rd) != kErrorOk)
      return self->setError(kErrorNoHeapMemory);

    // Emit dummy 32-bit integer; will be overwritten by `relocCode()`.
    EMIT_DWORD(0);

    // Trampoline has to be reserved, even if it's not used.
    if (Arch == kArchX64) {
      self->_trampolineSize += 8;
    }
  }
  goto _EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Displacement]
  // --------------------------------------------------------------------------

_EmitDisplacement:
  {
    ASMJIT_ASSERT(label->offset == -1);
    ASMJIT_ASSERT(dispSize == 1 || dispSize == 4);

    // Chain with label.
    LabelLink* link = self->_newLabelLink();
    link->prev = label->links;
    link->offset = (intptr_t)(cursor - self->_buffer);
    link->displacement = dispOffset;
    link->relocId = relocId;
    label->links = link;

    // Emit label size as dummy data.
    if (dispSize == 1)
      EMIT_BYTE(0x01);
    else // if (dispSize == 4)
      EMIT_DWORD(0x04040404);
  }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

_EmitDone:
#if !defined(ASMJIT_DISABLE_LOGGER)
# if defined(ASMJIT_DEBUG)
  if (self->_logger || assertIllegal) {
# else
  if (self->_logger) {
# endif // ASMJIT_DEBUG
    StringBuilderT<512> sb;
    uint32_t loggerOptions = 0;

    if (self->_logger) {
      sb.appendString(self->_logger->getIndentation());
      loggerOptions = self->_logger->getOptions();
    }

    X86Assembler_dumpInstruction(sb, Arch, code, options, o0, o1, o2, o3, loggerOptions);

    if ((loggerOptions & (1 << kLoggerOptionBinaryForm)) != 0)
      X86Assembler_dumpComment(sb, sb.getLength(), self->_cursor, (intptr_t)(cursor - self->_cursor), dispSize, self->_comment);
    else
      X86Assembler_dumpComment(sb, sb.getLength(), NULL, 0, 0, self->_comment);

# if defined(ASMJIT_DEBUG)
    if (self->_logger)
# endif // ASMJIT_DEBUG
      self->_logger->logString(kLoggerStyleDefault, sb.getData(), sb.getLength());

# if defined(ASMJIT_DEBUG)
    // Raise an assertion failure, because this situation shouldn't happen.
    if (assertIllegal)
      assertionFailed(sb.getData(), __FILE__, __LINE__);
# endif // ASMJIT_DEBUG
  }
#else
# if defined(ASMJIT_DEBUG)
  ASMJIT_ASSERT(!assertIllegal);
# endif // ASMJIT_DEBUG
#endif // !ASMJIT_DISABLE_LOGGER

  self->_comment = NULL;
  self->setCursor(cursor);

  return kErrorOk;

_UnknownInst:
  self->_comment = NULL;
  return self->setError(kErrorUnknownInst);

_GrowBuffer:
  ASMJIT_PROPAGATE_ERROR(self->_grow(16));

  cursor = self->getCursor();
  goto _Prepare;
}

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

Assembler::Assembler(Runtime* runtime) : X86X64Assembler(runtime) {
  _arch = kArchX86;
  _regSize = 4;
}

Assembler::~Assembler() {}

size_t Assembler::_relocCode(void* dst, Ptr base) const {
  return X86X64Assembler_relocCode<kArchX86>(this, dst, base);
}

Error Assembler::_emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3) {
  return X86X64Assembler_emit<kArchX86>(this, code, &o0, &o1, &o2, &o3);
}

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)

namespace asmjit {
namespace x64 {

Assembler::Assembler(Runtime* runtime) : X86X64Assembler(runtime) {
  _arch = kArchX64;
  _regSize = 8;
}

Assembler::~Assembler() {}

size_t Assembler::_relocCode(void* dst, Ptr base) const {
  return X86X64Assembler_relocCode<kArchX64>(this, dst, base);
}

Error Assembler::_emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3) {
  return X86X64Assembler_emit<kArchX64>(this, code, &o0, &o1, &o2, &o3);
}

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
