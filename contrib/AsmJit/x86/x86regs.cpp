// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS
#define ASMJIT_REGS_INIT

// [Guard]
#include "../build.h"
#if defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64)

// [Dependencies - AsmJit]
#include "../x86/x86regs.h"

// [Api-Begin]
#include "../apibegin.h"

#define DEFINE_REG(_Class_, _Name_, _Type_, _Index_, _Size_) \
  const _Class_ _Name_ = { { kOperandTypeReg, _Size_, ((_Type_) << 8) + _Index_, kInvalidValue, kVarTypeInvalid, 0 } }

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [asmjit::x86x64::Registers]
// ============================================================================

DEFINE_REG(GpReg, noGpReg, kInvalidReg, kInvalidReg, 0);

DEFINE_REG(GpReg, al, kRegTypeGpbLo, kRegIndexAx, 1);
DEFINE_REG(GpReg, cl, kRegTypeGpbLo, kRegIndexCx, 1);
DEFINE_REG(GpReg, dl, kRegTypeGpbLo, kRegIndexDx, 1);
DEFINE_REG(GpReg, bl, kRegTypeGpbLo, kRegIndexBx, 1);
DEFINE_REG(GpReg, spl, kRegTypeGpbLo, kRegIndexSp, 1);
DEFINE_REG(GpReg, bpl, kRegTypeGpbLo, kRegIndexBp, 1);
DEFINE_REG(GpReg, sil, kRegTypeGpbLo, kRegIndexSi, 1);
DEFINE_REG(GpReg, dil, kRegTypeGpbLo, kRegIndexDi, 1);
DEFINE_REG(GpReg, r8b, kRegTypeGpbLo, 8, 1);
DEFINE_REG(GpReg, r9b, kRegTypeGpbLo, 9, 1);
DEFINE_REG(GpReg, r10b, kRegTypeGpbLo, 10, 1);
DEFINE_REG(GpReg, r11b, kRegTypeGpbLo, 11, 1);
DEFINE_REG(GpReg, r12b, kRegTypeGpbLo, 12, 1);
DEFINE_REG(GpReg, r13b, kRegTypeGpbLo, 13, 1);
DEFINE_REG(GpReg, r14b, kRegTypeGpbLo, 14, 1);
DEFINE_REG(GpReg, r15b, kRegTypeGpbLo, 15, 1);

DEFINE_REG(GpReg, ah, kRegTypeGpbHi, kRegIndexAx, 1);
DEFINE_REG(GpReg, ch, kRegTypeGpbHi, kRegIndexCx, 1);
DEFINE_REG(GpReg, dh, kRegTypeGpbHi, kRegIndexDx, 1);
DEFINE_REG(GpReg, bh, kRegTypeGpbHi, kRegIndexBx, 1);

DEFINE_REG(GpReg, ax, kRegTypeGpw, kRegIndexAx, 2);
DEFINE_REG(GpReg, cx, kRegTypeGpw, kRegIndexCx, 2);
DEFINE_REG(GpReg, dx, kRegTypeGpw, kRegIndexDx, 2);
DEFINE_REG(GpReg, bx, kRegTypeGpw, kRegIndexBx, 2);
DEFINE_REG(GpReg, sp, kRegTypeGpw, kRegIndexSp, 2);
DEFINE_REG(GpReg, bp, kRegTypeGpw, kRegIndexBp, 2);
DEFINE_REG(GpReg, si, kRegTypeGpw, kRegIndexSi, 2);
DEFINE_REG(GpReg, di, kRegTypeGpw, kRegIndexDi, 2);
DEFINE_REG(GpReg, r8w, kRegTypeGpw, 8, 2);
DEFINE_REG(GpReg, r9w, kRegTypeGpw, 9, 2);
DEFINE_REG(GpReg, r10w, kRegTypeGpw, 10, 2);
DEFINE_REG(GpReg, r11w, kRegTypeGpw, 11, 2);
DEFINE_REG(GpReg, r12w, kRegTypeGpw, 12, 2);
DEFINE_REG(GpReg, r13w, kRegTypeGpw, 13, 2);
DEFINE_REG(GpReg, r14w, kRegTypeGpw, 14, 2);
DEFINE_REG(GpReg, r15w, kRegTypeGpw, 15, 2);

DEFINE_REG(GpReg, eax, kRegTypeGpd, kRegIndexAx, 4);
DEFINE_REG(GpReg, ecx, kRegTypeGpd, kRegIndexCx, 4);
DEFINE_REG(GpReg, edx, kRegTypeGpd, kRegIndexDx, 4);
DEFINE_REG(GpReg, ebx, kRegTypeGpd, kRegIndexBx, 4);
DEFINE_REG(GpReg, esp, kRegTypeGpd, kRegIndexSp, 4);
DEFINE_REG(GpReg, ebp, kRegTypeGpd, kRegIndexBp, 4);
DEFINE_REG(GpReg, esi, kRegTypeGpd, kRegIndexSi, 4);
DEFINE_REG(GpReg, edi, kRegTypeGpd, kRegIndexDi, 4);
DEFINE_REG(GpReg, r8d, kRegTypeGpd, 8, 4);
DEFINE_REG(GpReg, r9d, kRegTypeGpd, 9, 4);
DEFINE_REG(GpReg, r10d, kRegTypeGpd, 10, 4);
DEFINE_REG(GpReg, r11d, kRegTypeGpd, 11, 4);
DEFINE_REG(GpReg, r12d, kRegTypeGpd, 12, 4);
DEFINE_REG(GpReg, r13d, kRegTypeGpd, 13, 4);
DEFINE_REG(GpReg, r14d, kRegTypeGpd, 14, 4);
DEFINE_REG(GpReg, r15d, kRegTypeGpd, 15, 4);

DEFINE_REG(GpReg, rax, kRegTypeGpq, kRegIndexAx, 8);
DEFINE_REG(GpReg, rcx, kRegTypeGpq, kRegIndexCx, 8);
DEFINE_REG(GpReg, rdx, kRegTypeGpq, kRegIndexDx, 8);
DEFINE_REG(GpReg, rbx, kRegTypeGpq, kRegIndexBx, 8);
DEFINE_REG(GpReg, rsp, kRegTypeGpq, kRegIndexSp, 8);
DEFINE_REG(GpReg, rbp, kRegTypeGpq, kRegIndexBp, 8);
DEFINE_REG(GpReg, rsi, kRegTypeGpq, kRegIndexSi, 8);
DEFINE_REG(GpReg, rdi, kRegTypeGpq, kRegIndexDi, 8);
DEFINE_REG(GpReg, r8, kRegTypeGpq, 8, 8);
DEFINE_REG(GpReg, r9, kRegTypeGpq, 9, 8);
DEFINE_REG(GpReg, r10, kRegTypeGpq, 10, 8);
DEFINE_REG(GpReg, r11, kRegTypeGpq, 11, 8);
DEFINE_REG(GpReg, r12, kRegTypeGpq, 12, 8);
DEFINE_REG(GpReg, r13, kRegTypeGpq, 13, 8);
DEFINE_REG(GpReg, r14, kRegTypeGpq, 14, 8);
DEFINE_REG(GpReg, r15, kRegTypeGpq, 15, 8);

DEFINE_REG(FpReg, fp0, kRegTypeFp, 0, 10);
DEFINE_REG(FpReg, fp1, kRegTypeFp, 1, 10);
DEFINE_REG(FpReg, fp2, kRegTypeFp, 2, 10);
DEFINE_REG(FpReg, fp3, kRegTypeFp, 3, 10);
DEFINE_REG(FpReg, fp4, kRegTypeFp, 4, 10);
DEFINE_REG(FpReg, fp5, kRegTypeFp, 5, 10);
DEFINE_REG(FpReg, fp6, kRegTypeFp, 6, 10);
DEFINE_REG(FpReg, fp7, kRegTypeFp, 7, 10);

DEFINE_REG(MmReg, mm0, kRegTypeMm, 0, 8);
DEFINE_REG(MmReg, mm1, kRegTypeMm, 1, 8);
DEFINE_REG(MmReg, mm2, kRegTypeMm, 2, 8);
DEFINE_REG(MmReg, mm3, kRegTypeMm, 3, 8);
DEFINE_REG(MmReg, mm4, kRegTypeMm, 4, 8);
DEFINE_REG(MmReg, mm5, kRegTypeMm, 5, 8);
DEFINE_REG(MmReg, mm6, kRegTypeMm, 6, 8);
DEFINE_REG(MmReg, mm7, kRegTypeMm, 7, 8);

DEFINE_REG(XmmReg, xmm0, kRegTypeXmm, 0, 16);
DEFINE_REG(XmmReg, xmm1, kRegTypeXmm, 1, 16);
DEFINE_REG(XmmReg, xmm2, kRegTypeXmm, 2, 16);
DEFINE_REG(XmmReg, xmm3, kRegTypeXmm, 3, 16);
DEFINE_REG(XmmReg, xmm4, kRegTypeXmm, 4, 16);
DEFINE_REG(XmmReg, xmm5, kRegTypeXmm, 5, 16);
DEFINE_REG(XmmReg, xmm6, kRegTypeXmm, 6, 16);
DEFINE_REG(XmmReg, xmm7, kRegTypeXmm, 7, 16);
DEFINE_REG(XmmReg, xmm8, kRegTypeXmm, 8, 16);
DEFINE_REG(XmmReg, xmm9, kRegTypeXmm, 9, 16);
DEFINE_REG(XmmReg, xmm10, kRegTypeXmm, 10, 16);
DEFINE_REG(XmmReg, xmm11, kRegTypeXmm, 11, 16);
DEFINE_REG(XmmReg, xmm12, kRegTypeXmm, 12, 16);
DEFINE_REG(XmmReg, xmm13, kRegTypeXmm, 13, 16);
DEFINE_REG(XmmReg, xmm14, kRegTypeXmm, 14, 16);
DEFINE_REG(XmmReg, xmm15, kRegTypeXmm, 15, 16);

DEFINE_REG(YmmReg, ymm0, kRegTypeYmm, 0, 32);
DEFINE_REG(YmmReg, ymm1, kRegTypeYmm, 1, 32);
DEFINE_REG(YmmReg, ymm2, kRegTypeYmm, 2, 32);
DEFINE_REG(YmmReg, ymm3, kRegTypeYmm, 3, 32);
DEFINE_REG(YmmReg, ymm4, kRegTypeYmm, 4, 32);
DEFINE_REG(YmmReg, ymm5, kRegTypeYmm, 5, 32);
DEFINE_REG(YmmReg, ymm6, kRegTypeYmm, 6, 32);
DEFINE_REG(YmmReg, ymm7, kRegTypeYmm, 7, 32);
DEFINE_REG(YmmReg, ymm8, kRegTypeYmm, 8, 32);
DEFINE_REG(YmmReg, ymm9, kRegTypeYmm, 9, 32);
DEFINE_REG(YmmReg, ymm10, kRegTypeYmm, 10, 32);
DEFINE_REG(YmmReg, ymm11, kRegTypeYmm, 11, 32);
DEFINE_REG(YmmReg, ymm12, kRegTypeYmm, 12, 32);
DEFINE_REG(YmmReg, ymm13, kRegTypeYmm, 13, 32);
DEFINE_REG(YmmReg, ymm14, kRegTypeYmm, 14, 32);
DEFINE_REG(YmmReg, ymm15, kRegTypeYmm, 15, 32);

DEFINE_REG(SegReg, cs, kRegTypeSeg, kSegCs, 2);
DEFINE_REG(SegReg, ss, kRegTypeSeg, kSegSs, 2);
DEFINE_REG(SegReg, ds, kRegTypeSeg, kSegDs, 2);
DEFINE_REG(SegReg, es, kRegTypeSeg, kSegEs, 2);
DEFINE_REG(SegReg, fs, kRegTypeSeg, kSegFs, 2);
DEFINE_REG(SegReg, gs, kRegTypeSeg, kSegGs, 2);

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

DEFINE_REG(GpReg, zax, kRegTypeGpd, kRegIndexAx, 4);
DEFINE_REG(GpReg, zcx, kRegTypeGpd, kRegIndexCx, 4);
DEFINE_REG(GpReg, zdx, kRegTypeGpd, kRegIndexDx, 4);
DEFINE_REG(GpReg, zbx, kRegTypeGpd, kRegIndexBx, 4);
DEFINE_REG(GpReg, zsp, kRegTypeGpd, kRegIndexSp, 4);
DEFINE_REG(GpReg, zbp, kRegTypeGpd, kRegIndexBp, 4);
DEFINE_REG(GpReg, zsi, kRegTypeGpd, kRegIndexSi, 4);
DEFINE_REG(GpReg, zdi, kRegTypeGpd, kRegIndexDi, 4);

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)
namespace asmjit {
namespace x64 {

DEFINE_REG(GpReg, zax, kRegTypeGpq, kRegIndexAx, 8);
DEFINE_REG(GpReg, zcx, kRegTypeGpq, kRegIndexCx, 8);
DEFINE_REG(GpReg, zdx, kRegTypeGpq, kRegIndexDx, 8);
DEFINE_REG(GpReg, zbx, kRegTypeGpq, kRegIndexBx, 8);
DEFINE_REG(GpReg, zsp, kRegTypeGpq, kRegIndexSp, 8);
DEFINE_REG(GpReg, zbp, kRegTypeGpq, kRegIndexBp, 8);
DEFINE_REG(GpReg, zsi, kRegTypeGpq, kRegIndexSi, 8);
DEFINE_REG(GpReg, zdi, kRegTypeGpq, kRegIndexDi, 8);

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
