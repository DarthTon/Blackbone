// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86REGS_H
#define _ASMJIT_X86_X86REGS_H

// [Dependencies - AsmJit]
#include "../base/operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

struct GpReg;
struct FpReg;
struct MmReg;
struct XmmReg;
struct YmmReg;
struct SegReg;

#if defined(ASMJIT_REGS_INIT)
// Remap all classes to POD structs that can be statically initialized.
struct GpReg  { Operand::InitRegOp data; };
struct FpReg  { Operand::InitRegOp data; };
struct MmReg  { Operand::InitRegOp data; };
struct XmmReg { Operand::InitRegOp data; };
struct YmmReg { Operand::InitRegOp data; };
struct SegReg { Operand::InitRegOp data; };
#endif // ASMJIT_REGS_INIT

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x86x64::kRegClass]
// ============================================================================

//! X86/X64 variable class.
ASMJIT_ENUM(kRegClass) {
  // kRegClassGp defined earlier.

  //! X86/X64 Fp register class.
  kRegClassFp = 1,
  //! X86/X64 Mm register class.
  kRegClassMm = 2,
  //! X86/X64 Xmm/Ymm/Zmm register class.
  kRegClassXyz = 3,

  //! Count of X86/X64 register classes.
  kRegClassCount = 4
};

// ============================================================================
// [asmjit::x86x64::kRegCount]
// ============================================================================

//! X86/X64 registers count.
ASMJIT_ENUM(kRegCount) {
  //! Count of Fp registers (8).
  kRegCountFp = 8,
  //! Count of Mm registers (8).
  kRegCountMm = 8,
  //! Count of segment registers (6).
  kRegCountSeg = 6
};

// ============================================================================
// [asmjit::x86x64::kRegType]
// ============================================================================

//! X86/X64 register type.
ASMJIT_ENUM(kRegType) {
  //! Gpb-lo register (AL, BL, CL, DL, ...).
  kRegTypeGpbLo = 0x01,
  //! Gpb-hi register (AH, BH, CH, DH only).
  kRegTypeGpbHi = 0x02,

  //! \internal
  //!
  //! Gpb-hi register patched to native index (4-7).
  kRegTypePatchedGpbHi = kRegTypeGpbLo | kRegTypeGpbHi,

  //! Gpw register.
  kRegTypeGpw = 0x10,
  //! Gpd register.
  kRegTypeGpd = 0x20,
  //! Gpq register.
  kRegTypeGpq = 0x30,

  //! Fp register.
  kRegTypeFp = 0x50,
  //! Mm register.
  kRegTypeMm = 0x60,

  //! Xmm register.
  kRegTypeXmm = 0x70,
  //! Ymm register.
  kRegTypeYmm = 0x80,
  //! Zmm register.
  kRegTypeZmm = 0x90,

  //! Segment register.
  kRegTypeSeg = 0xF0
};

// ============================================================================
// [asmjit::x86x64::kRegIndex]
// ============================================================================

//! X86/X64 register indexes.
//!
//! \note Register indexes have been reduced to only support general purpose
//! registers. There is no need to have enumerations with number suffix that
//! expands to the exactly same value as the suffix value itself.
ASMJIT_ENUM(kRegIndex) {
  //! Index of Al/Ah/Ax/Eax/Rax registers.
  kRegIndexAx = 0,
  //! Index of Cl/Ch/Cx/Ecx/Rcx registers.
  kRegIndexCx = 1,
  //! Index of Dl/Dh/Dx/Edx/Rdx registers.
  kRegIndexDx = 2,
  //! Index of Bl/Bh/Bx/Ebx/Rbx registers.
  kRegIndexBx = 3,
  //! Index of Spl/Sp/Esp/Rsp registers.
  kRegIndexSp = 4,
  //! Index of Bpl/Bp/Ebp/Rbp registers.
  kRegIndexBp = 5,
  //! Index of Sil/Si/Esi/Rsi registers.
  kRegIndexSi = 6,
  //! Index of Dil/Di/Edi/Rdi registers.
  kRegIndexDi = 7,
  //! Index of R8b/R8w/R8d/R8 registers (64-bit only).
  kRegIndexR8 = 8,
  //! Index of R9B/R9w/R9d/R9 registers (64-bit only).
  kRegIndexR9 = 9,
  //! Index of R10B/R10w/R10D/R10 registers (64-bit only).
  kRegIndexR10 = 10,
  //! Index of R11B/R11w/R11d/R11 registers (64-bit only).
  kRegIndexR11 = 11,
  //! Index of R12B/R12w/R12d/R12 registers (64-bit only).
  kRegIndexR12 = 12,
  //! Index of R13B/R13w/R13d/R13 registers (64-bit only).
  kRegIndexR13 = 13,
  //! Index of R14B/R14w/R14d/R14 registers (64-bit only).
  kRegIndexR14 = 14,
  //! Index of R15B/R15w/R15d/R15 registers (64-bit only).
  kRegIndexR15 = 15
};

// ============================================================================
// [asmjit::x86x64::kSeg]
// ============================================================================

//! X86/X64 segment codes.
ASMJIT_ENUM(kSeg) {
  //! No/Default segment.
  kSegDefault = 0,
  //! Es segment.
  kSegEs = 1,
  //! Cs segment.
  kSegCs = 2,
  //! Ss segment.
  kSegSs = 3,
  //! Ds segment.
  kSegDs = 4,
  //! Fs segment.
  kSegFs = 5,
  //! Gs segment.
  kSegGs = 6
};

// ============================================================================
// [asmjit::x86x64::Registers]
// ============================================================================

//! No Gp register, can be used only within `Mem` operand.
ASMJIT_VAR const GpReg noGpReg;

ASMJIT_VAR const GpReg al;     //!< 8-bit Gpb-lo register.
ASMJIT_VAR const GpReg cl;     //!< 8-bit Gpb-lo register.
ASMJIT_VAR const GpReg dl;     //!< 8-bit Gpb-lo register.
ASMJIT_VAR const GpReg bl;     //!< 8-bit Gpb-lo register.
ASMJIT_VAR const GpReg spl;    //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg bpl;    //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg sil;    //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg dil;    //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r8b;    //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r9b;    //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r10b;   //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r11b;   //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r12b;   //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r13b;   //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r14b;   //!< 8-bit Gpb-lo register (X64).
ASMJIT_VAR const GpReg r15b;   //!< 8-bit Gpb-lo register (X64).

ASMJIT_VAR const GpReg ah;     //!< 8-bit Gpb-hi register.
ASMJIT_VAR const GpReg ch;     //!< 8-bit Gpb-hi register.
ASMJIT_VAR const GpReg dh;     //!< 8-bit Gpb-hi register.
ASMJIT_VAR const GpReg bh;     //!< 8-bit Gpb-hi register.

ASMJIT_VAR const GpReg ax;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg cx;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg dx;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg bx;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg sp;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg bp;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg si;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg di;     //!< 16-bit Gpw register.
ASMJIT_VAR const GpReg r8w;    //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r9w;    //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r10w;   //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r11w;   //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r12w;   //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r13w;   //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r14w;   //!< 16-bit Gpw register (X64).
ASMJIT_VAR const GpReg r15w;   //!< 16-bit Gpw register (X64).

ASMJIT_VAR const GpReg eax;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg ecx;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg edx;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg ebx;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg esp;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg ebp;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg esi;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg edi;    //!< 32-bit Gpd register.
ASMJIT_VAR const GpReg r8d;    //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r9d;    //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r10d;   //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r11d;   //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r12d;   //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r13d;   //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r14d;   //!< 32-bit Gpd register (X64).
ASMJIT_VAR const GpReg r15d;   //!< 32-bit Gpd register (X64).

ASMJIT_VAR const GpReg rax;    //!< 64-bit Gpq register (X64).
ASMJIT_VAR const GpReg rcx;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg rdx;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg rbx;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg rsp;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg rbp;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg rsi;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg rdi;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r8;     //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r9;     //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r10;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r11;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r12;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r13;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r14;    //!< 64-bit Gpq register (X64)
ASMJIT_VAR const GpReg r15;    //!< 64-bit Gpq register (X64)

ASMJIT_VAR const FpReg fp0;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp1;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp2;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp3;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp4;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp5;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp6;    //!< 80-bit Fp register.
ASMJIT_VAR const FpReg fp7;    //!< 80-bit Fp register.

ASMJIT_VAR const MmReg mm0;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm1;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm2;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm3;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm4;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm5;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm6;    //!< 64-bit Mm register.
ASMJIT_VAR const MmReg mm7;    //!< 64-bit Mm register.

ASMJIT_VAR const XmmReg xmm0;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm1;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm2;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm3;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm4;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm5;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm6;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm7;  //!< 128-bit Xmm register.
ASMJIT_VAR const XmmReg xmm8;  //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm9;  //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm10; //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm11; //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm12; //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm13; //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm14; //!< 128-bit Xmm register (X64).
ASMJIT_VAR const XmmReg xmm15; //!< 128-bit Xmm register (X64).

ASMJIT_VAR const YmmReg ymm0;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm1;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm2;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm3;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm4;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm5;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm6;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm7;  //!< 256-bit Ymm register.
ASMJIT_VAR const YmmReg ymm8;  //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm9;  //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm10; //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm11; //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm12; //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm13; //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm14; //!< 256-bit Ymm register (X64).
ASMJIT_VAR const YmmReg ymm15; //!< 256-bit Ymm register (X64).

ASMJIT_VAR const SegReg cs;    //!< Cs segment register.
ASMJIT_VAR const SegReg ss;    //!< Ss segment register.
ASMJIT_VAR const SegReg ds;    //!< Ds segment register.
ASMJIT_VAR const SegReg es;    //!< Es segment register.
ASMJIT_VAR const SegReg fs;    //!< Fs segment register.
ASMJIT_VAR const SegReg gs;    //!< Gs segment register.

//! \}

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

// This is the only place where the x86x64 namespace is included into x86.
using namespace ::asmjit::x86x64;

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x86::Registers]
// ============================================================================

//! Gpd register.
ASMJIT_VAR const GpReg zax;
//! Gpd register.
ASMJIT_VAR const GpReg zcx;
//! Gpd register.
ASMJIT_VAR const GpReg zdx;
//! Gpd register.
ASMJIT_VAR const GpReg zbx;
//! Gpd register.
ASMJIT_VAR const GpReg zsp;
//! Gpd register.
ASMJIT_VAR const GpReg zbp;
//! Gpd register.
ASMJIT_VAR const GpReg zsi;
//! Gpd register.
ASMJIT_VAR const GpReg zdi;

//! \}

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)

namespace asmjit {
namespace x64 {

// This is the only place where the x86x64 namespace is included into x64.
using namespace ::asmjit::x86x64;

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x64::Registers]
// ============================================================================

//! Gpq register.
ASMJIT_VAR const GpReg zax;
//! Gpq register.
ASMJIT_VAR const GpReg zcx;
//! Gpq register.
ASMJIT_VAR const GpReg zdx;
//! Gpq register.
ASMJIT_VAR const GpReg zbx;
//! Gpq register.
ASMJIT_VAR const GpReg zsp;
//! Gpq register.
ASMJIT_VAR const GpReg zbp;
//! Gpq register.
ASMJIT_VAR const GpReg zsi;
//! Gpq register.
ASMJIT_VAR const GpReg zdi;

//! \}

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86REGS_H
