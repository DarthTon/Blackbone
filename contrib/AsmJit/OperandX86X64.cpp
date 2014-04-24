// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies]
#include "Defs.h"
#include "Operand.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::Registers - no_reg]
// ============================================================================

const GPReg no_reg(_Initialize(), INVALID_VALUE);

// ============================================================================
// [AsmJit::Registers - 8-bit]
// ============================================================================

const GPReg al(_Initialize(), REG_AL);
const GPReg cl(_Initialize(), REG_CL);
const GPReg dl(_Initialize(), REG_DL);
const GPReg bl(_Initialize(), REG_BL);

#if defined(ASMJIT_X64)
const GPReg spl(_Initialize(), REG_SPL);
const GPReg bpl(_Initialize(), REG_BPL);
const GPReg sil(_Initialize(), REG_SIL);
const GPReg dil(_Initialize(), REG_DIL);

const GPReg r8b(_Initialize(), REG_R8B);
const GPReg r9b(_Initialize(), REG_R9B);
const GPReg r10b(_Initialize(), REG_R10B);
const GPReg r11b(_Initialize(), REG_R11B);
const GPReg r12b(_Initialize(), REG_R12B);
const GPReg r13b(_Initialize(), REG_R13B);
const GPReg r14b(_Initialize(), REG_R14B);
const GPReg r15b(_Initialize(), REG_R15B);
#endif // ASMJIT_X64

const GPReg ah(_Initialize(), REG_AH);
const GPReg ch(_Initialize(), REG_CH);
const GPReg dh(_Initialize(), REG_DH);
const GPReg bh(_Initialize(), REG_BH);

// ============================================================================
// [AsmJit::Registers - 16-bit]
// ============================================================================

const GPReg ax(_Initialize(), REG_AX);
const GPReg cx(_Initialize(), REG_CX);
const GPReg dx(_Initialize(), REG_DX);
const GPReg bx(_Initialize(), REG_BX);
const GPReg sp(_Initialize(), REG_SP);
const GPReg bp(_Initialize(), REG_BP);
const GPReg si(_Initialize(), REG_SI);
const GPReg di(_Initialize(), REG_DI);

#if defined(ASMJIT_X64)
const GPReg r8w(_Initialize(), REG_R8W);
const GPReg r9w(_Initialize(), REG_R9W);
const GPReg r10w(_Initialize(), REG_R10W);
const GPReg r11w(_Initialize(), REG_R11W);
const GPReg r12w(_Initialize(), REG_R12W);
const GPReg r13w(_Initialize(), REG_R13W);
const GPReg r14w(_Initialize(), REG_R14W);
const GPReg r15w(_Initialize(), REG_R15W);
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - 32-bit]
// ============================================================================

const GPReg eax(_Initialize(), REG_EAX);
const GPReg ecx(_Initialize(), REG_ECX);
const GPReg edx(_Initialize(), REG_EDX);
const GPReg ebx(_Initialize(), REG_EBX);
const GPReg esp(_Initialize(), REG_ESP);
const GPReg ebp(_Initialize(), REG_EBP);
const GPReg esi(_Initialize(), REG_ESI);
const GPReg edi(_Initialize(), REG_EDI);

#if defined(ASMJIT_X64)
const GPReg r8d(_Initialize(), REG_R8D);
const GPReg r9d(_Initialize(), REG_R9D);
const GPReg r10d(_Initialize(), REG_R10D);
const GPReg r11d(_Initialize(), REG_R11D);
const GPReg r12d(_Initialize(), REG_R12D);
const GPReg r13d(_Initialize(), REG_R13D);
const GPReg r14d(_Initialize(), REG_R14D);
const GPReg r15d(_Initialize(), REG_R15D);
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - 64-bit]
// ============================================================================

#if defined(ASMJIT_X64)
const GPReg rax(_Initialize(), REG_RAX);
const GPReg rcx(_Initialize(), REG_RCX);
const GPReg rdx(_Initialize(), REG_RDX);
const GPReg rbx(_Initialize(), REG_RBX);
const GPReg rsp(_Initialize(), REG_RSP);
const GPReg rbp(_Initialize(), REG_RBP);
const GPReg rsi(_Initialize(), REG_RSI);
const GPReg rdi(_Initialize(), REG_RDI);

const GPReg r8(_Initialize(), REG_R8);
const GPReg r9(_Initialize(), REG_R9);
const GPReg r10(_Initialize(), REG_R10);
const GPReg r11(_Initialize(), REG_R11);
const GPReg r12(_Initialize(), REG_R12);
const GPReg r13(_Initialize(), REG_R13);
const GPReg r14(_Initialize(), REG_R14);
const GPReg r15(_Initialize(), REG_R15);
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - Native (AsmJit extension)]
// ============================================================================

const GPReg nax(_Initialize(), REG_NAX);
const GPReg ncx(_Initialize(), REG_NCX);
const GPReg ndx(_Initialize(), REG_NDX);
const GPReg nbx(_Initialize(), REG_NBX);
const GPReg nsp(_Initialize(), REG_NSP);
const GPReg nbp(_Initialize(), REG_NBP);
const GPReg nsi(_Initialize(), REG_NSI);
const GPReg ndi(_Initialize(), REG_NDI);

// ============================================================================
// [AsmJit::Registers - MM]
// ============================================================================

const MMReg mm0(_Initialize(), REG_MM0);
const MMReg mm1(_Initialize(), REG_MM1);
const MMReg mm2(_Initialize(), REG_MM2);
const MMReg mm3(_Initialize(), REG_MM3);
const MMReg mm4(_Initialize(), REG_MM4);
const MMReg mm5(_Initialize(), REG_MM5);
const MMReg mm6(_Initialize(), REG_MM6);
const MMReg mm7(_Initialize(), REG_MM7);

// ============================================================================
// [AsmJit::Registers - XMM]
// ============================================================================

const XMMReg xmm0(_Initialize(), REG_XMM0);
const XMMReg xmm1(_Initialize(), REG_XMM1);
const XMMReg xmm2(_Initialize(), REG_XMM2);
const XMMReg xmm3(_Initialize(), REG_XMM3);
const XMMReg xmm4(_Initialize(), REG_XMM4);
const XMMReg xmm5(_Initialize(), REG_XMM5);
const XMMReg xmm6(_Initialize(), REG_XMM6);
const XMMReg xmm7(_Initialize(), REG_XMM7);

#if defined(ASMJIT_X64)
const XMMReg xmm8(_Initialize(), REG_XMM8);
const XMMReg xmm9(_Initialize(), REG_XMM9);
const XMMReg xmm10(_Initialize(), REG_XMM10);
const XMMReg xmm11(_Initialize(), REG_XMM11);
const XMMReg xmm12(_Initialize(), REG_XMM12);
const XMMReg xmm13(_Initialize(), REG_XMM13);
const XMMReg xmm14(_Initialize(), REG_XMM14);
const XMMReg xmm15(_Initialize(), REG_XMM15);
#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::Registers - Segment]
// ============================================================================

const SegmentReg cs(_Initialize(), REG_CS);
const SegmentReg ss(_Initialize(), REG_SS);
const SegmentReg ds(_Initialize(), REG_DS);
const SegmentReg es(_Initialize(), REG_ES);
const SegmentReg fs(_Initialize(), REG_FS);
const SegmentReg gs(_Initialize(), REG_GS);

// ============================================================================
// [AsmJit::Immediate]
// ============================================================================

//! @brief Create signed immediate value operand.
Imm imm(sysint_t i) ASMJIT_NOTHROW
{ 
  return Imm(i, false);
}

//! @brief Create unsigned immediate value operand.
Imm uimm(sysuint_t i) ASMJIT_NOTHROW
{
  return Imm((sysint_t)i, true);
}

// ============================================================================
// [AsmJit::BaseVar]
// ============================================================================

Mem _BaseVarMem(const BaseVar& var, uint32_t ptrSize) ASMJIT_NOTHROW
{
  Mem m; //(_DontInitialize());

  m._mem.op = OPERAND_MEM;
  m._mem.size = (ptrSize == INVALID_VALUE) ? var.getSize() : (uint8_t)ptrSize;
  m._mem.type = OPERAND_MEM_NATIVE;
  m._mem.segmentPrefix = SEGMENT_NONE;
  m._mem.sizePrefix = 0;
  m._mem.shift = 0;

  m._mem.id = var.getId();
  m._mem.base = INVALID_VALUE;
  m._mem.index = INVALID_VALUE;

  m._mem.target = NULL;
  m._mem.displacement = 0;

  return m;
}


Mem _BaseVarMem(const BaseVar& var, uint32_t ptrSize, sysint_t disp) ASMJIT_NOTHROW
{
  Mem m; //(_DontInitialize());

  m._mem.op = OPERAND_MEM;
  m._mem.size = (ptrSize == INVALID_VALUE) ? var.getSize() : (uint8_t)ptrSize;
  m._mem.type = OPERAND_MEM_NATIVE;
  m._mem.segmentPrefix = SEGMENT_NONE;
  m._mem.sizePrefix = 0;
  m._mem.shift = 0;

  m._mem.id = var.getId();

  m._mem.base = INVALID_VALUE;
  m._mem.index = INVALID_VALUE;

  m._mem.target = NULL;
  m._mem.displacement = disp;

  return m;
}

Mem _BaseVarMem(const BaseVar& var, uint32_t ptrSize, const GPVar& index, uint32_t shift, sysint_t disp) ASMJIT_NOTHROW
{
  Mem m; //(_DontInitialize());

  m._mem.op = OPERAND_MEM;
  m._mem.size = (ptrSize == INVALID_VALUE) ? var.getSize() : (uint8_t)ptrSize;
  m._mem.type = OPERAND_MEM_NATIVE;
  m._mem.segmentPrefix = SEGMENT_NONE;
  m._mem.sizePrefix = 0;
  m._mem.shift = shift;

  m._mem.id = var.getId();

  m._mem.base = INVALID_VALUE;
  m._mem.index = index.getId();

  m._mem.target = NULL;
  m._mem.displacement = disp;

  return m;
}

// ============================================================================
// [AsmJit::Mem - ptr[]]
// ============================================================================

Mem _MemPtrBuild(
  const Label& label, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  return Mem(label, disp, ptrSize);
}

Mem _MemPtrBuild(
  const Label& label,
  const GPReg& index, uint32_t shift, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  Mem m(label, disp, ptrSize);

  m._mem.index = index.getRegIndex();
  m._mem.shift = shift;

  return m;
}

Mem _MemPtrBuild(
  const Label& label,
  const GPVar& index, uint32_t shift, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  Mem m(label, disp, ptrSize);

  m._mem.index = index.getId();
  m._mem.shift = shift;

  return m;
}

// ============================================================================
// [AsmJit::Mem - ptr[] - Absolute Addressing]
// ============================================================================

ASMJIT_API Mem _MemPtrAbs(
  void* target, sysint_t disp,
  uint32_t segmentPrefix, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  Mem m;

  m._mem.size = ptrSize;
  m._mem.type = OPERAND_MEM_ABSOLUTE;
  m._mem.segmentPrefix = segmentPrefix;

  m._mem.target = target;
  m._mem.displacement = disp;

  return m;
}

ASMJIT_API Mem _MemPtrAbs(
  void* target,
  const GPReg& index, uint32_t shift, sysint_t disp,
  uint32_t segmentPrefix, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  Mem m;// (_DontInitialize());

  m._mem.op = OPERAND_MEM;
  m._mem.size = ptrSize;
  m._mem.type = OPERAND_MEM_ABSOLUTE;
  m._mem.segmentPrefix = (uint8_t)segmentPrefix;

#if defined(ASMJIT_X86)
  m._mem.sizePrefix = index.getSize() != 4;
#else
  m._mem.sizePrefix = index.getSize() != 8;
#endif

  m._mem.shift = shift;

  m._mem.id = INVALID_VALUE;
  m._mem.base = INVALID_VALUE;
  m._mem.index = index.getRegIndex();

  m._mem.target = target;
  m._mem.displacement = disp;

  return m;
}

ASMJIT_API Mem _MemPtrAbs(
  void* target,
  const GPVar& index, uint32_t shift, sysint_t disp,
  uint32_t segmentPrefix, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  Mem m;// (_DontInitialize());

  m._mem.op = OPERAND_MEM;
  m._mem.size = ptrSize;
  m._mem.type = OPERAND_MEM_ABSOLUTE;
  m._mem.segmentPrefix = (uint8_t)segmentPrefix;

#if defined(ASMJIT_X86)
  m._mem.sizePrefix = index.getSize() != 4;
#else
  m._mem.sizePrefix = index.getSize() != 8;
#endif

  m._mem.shift = shift;

  m._mem.id = INVALID_VALUE;
  m._mem.base = INVALID_VALUE;
  m._mem.index = index.getId();

  m._mem.target = target;
  m._mem.displacement = disp;

  return m;
}

// ============================================================================
// [AsmJit::Mem - ptr[base + displacement]]
// ============================================================================

Mem _MemPtrBuild(
  const GPReg& base, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  return Mem(base, disp, ptrSize);
}

Mem _MemPtrBuild(
  const GPReg& base,
  const GPReg& index, uint32_t shift, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  return Mem(base, index, shift, disp, ptrSize);
}

Mem _MemPtrBuild(
  const GPVar& base, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  return Mem(base, disp, ptrSize);
}

Mem _MemPtrBuild(
  const GPVar& base,
  const GPVar& index, uint32_t shift, sysint_t disp, uint32_t ptrSize)
  ASMJIT_NOTHROW
{
  return Mem(base, index, shift, disp, ptrSize);
}

} // AsmJit namespace
