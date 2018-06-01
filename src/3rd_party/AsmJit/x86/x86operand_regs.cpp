// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS
#define ASMJIT_EXPORTS_X86OPERAND_REGS

// [Guard]
#include "../build.h"
#if defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64)

// [Dependencies - AsmJit]
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

#define REG(_Type_, _Index_, _Size_) {{ \
  kOperandTypeReg, _Size_, { ((_Type_) << 8) + _Index_ }, kInvalidValue, {{ kInvalidVar, 0 }} \
}}

const X86RegData x86RegData = {
  // RIP.
  REG(kX86RegTypeRip, 0, 0),
  // NpGp.
  REG(kInvalidReg, kInvalidReg, 0),

  // Segments.
  {
    REG(kX86RegTypeSeg, 0, 2), // Default.
    REG(kX86RegTypeSeg, 1, 2), // ES.
    REG(kX86RegTypeSeg, 2, 2), // CS.
    REG(kX86RegTypeSeg, 3, 2), // SS.
    REG(kX86RegTypeSeg, 4, 2), // DS.
    REG(kX86RegTypeSeg, 5, 2), // FS.
    REG(kX86RegTypeSeg, 6, 2)  // GS.
  },

  // GpbLo.
  {
    REG(kX86RegTypeGpbLo, 0, 1),
    REG(kX86RegTypeGpbLo, 1, 1),
    REG(kX86RegTypeGpbLo, 2, 1),
    REG(kX86RegTypeGpbLo, 3, 1),
    REG(kX86RegTypeGpbLo, 4, 1),
    REG(kX86RegTypeGpbLo, 5, 1),
    REG(kX86RegTypeGpbLo, 6, 1),
    REG(kX86RegTypeGpbLo, 7, 1),
    REG(kX86RegTypeGpbLo, 8, 1),
    REG(kX86RegTypeGpbLo, 9, 1),
    REG(kX86RegTypeGpbLo, 10, 1),
    REG(kX86RegTypeGpbLo, 11, 1),
    REG(kX86RegTypeGpbLo, 12, 1),
    REG(kX86RegTypeGpbLo, 13, 1),
    REG(kX86RegTypeGpbLo, 14, 1),
    REG(kX86RegTypeGpbLo, 15, 1)
  },

  // GpbHi.
  {
    REG(kX86RegTypeGpbHi, 0, 1),
    REG(kX86RegTypeGpbHi, 1, 1),
    REG(kX86RegTypeGpbHi, 2, 1),
    REG(kX86RegTypeGpbHi, 3, 1)
  },

  // Gpw.
  {
    REG(kX86RegTypeGpw, 0, 2),
    REG(kX86RegTypeGpw, 1, 2),
    REG(kX86RegTypeGpw, 2, 2),
    REG(kX86RegTypeGpw, 3, 2),
    REG(kX86RegTypeGpw, 4, 2),
    REG(kX86RegTypeGpw, 5, 2),
    REG(kX86RegTypeGpw, 6, 2),
    REG(kX86RegTypeGpw, 7, 2),
    REG(kX86RegTypeGpw, 8, 2),
    REG(kX86RegTypeGpw, 9, 2),
    REG(kX86RegTypeGpw, 10, 2),
    REG(kX86RegTypeGpw, 11, 2),
    REG(kX86RegTypeGpw, 12, 2),
    REG(kX86RegTypeGpw, 13, 2),
    REG(kX86RegTypeGpw, 14, 2),
    REG(kX86RegTypeGpw, 15, 2)
  },

  // Gpd.
  {
    REG(kX86RegTypeGpd, 0, 4),
    REG(kX86RegTypeGpd, 1, 4),
    REG(kX86RegTypeGpd, 2, 4),
    REG(kX86RegTypeGpd, 3, 4),
    REG(kX86RegTypeGpd, 4, 4),
    REG(kX86RegTypeGpd, 5, 4),
    REG(kX86RegTypeGpd, 6, 4),
    REG(kX86RegTypeGpd, 7, 4),
    REG(kX86RegTypeGpd, 8, 4),
    REG(kX86RegTypeGpd, 9, 4),
    REG(kX86RegTypeGpd, 10, 4),
    REG(kX86RegTypeGpd, 11, 4),
    REG(kX86RegTypeGpd, 12, 4),
    REG(kX86RegTypeGpd, 13, 4),
    REG(kX86RegTypeGpd, 14, 4),
    REG(kX86RegTypeGpd, 15, 4)
  },

  // Gpq.
  {
    REG(kX86RegTypeGpq, 0, 8),
    REG(kX86RegTypeGpq, 1, 8),
    REG(kX86RegTypeGpq, 2, 8),
    REG(kX86RegTypeGpq, 3, 8),
    REG(kX86RegTypeGpq, 4, 8),
    REG(kX86RegTypeGpq, 5, 8),
    REG(kX86RegTypeGpq, 6, 8),
    REG(kX86RegTypeGpq, 7, 8),
    REG(kX86RegTypeGpq, 8, 8),
    REG(kX86RegTypeGpq, 9, 8),
    REG(kX86RegTypeGpq, 10, 8),
    REG(kX86RegTypeGpq, 11, 8),
    REG(kX86RegTypeGpq, 12, 8),
    REG(kX86RegTypeGpq, 13, 8),
    REG(kX86RegTypeGpq, 14, 8),
    REG(kX86RegTypeGpq, 15, 8)
  },

  // Fp.
  {
    REG(kX86RegTypeFp, 0, 10),
    REG(kX86RegTypeFp, 1, 10),
    REG(kX86RegTypeFp, 2, 10),
    REG(kX86RegTypeFp, 3, 10),
    REG(kX86RegTypeFp, 4, 10),
    REG(kX86RegTypeFp, 5, 10),
    REG(kX86RegTypeFp, 6, 10),
    REG(kX86RegTypeFp, 7, 10)
  },

  // Mm.
  {
    REG(kX86RegTypeMm, 0, 8),
    REG(kX86RegTypeMm, 1, 8),
    REG(kX86RegTypeMm, 2, 8),
    REG(kX86RegTypeMm, 3, 8),
    REG(kX86RegTypeMm, 4, 8),
    REG(kX86RegTypeMm, 5, 8),
    REG(kX86RegTypeMm, 6, 8),
    REG(kX86RegTypeMm, 7, 8)
  },

  // K.
  {
    REG(kX86RegTypeK, 0, 8),
    REG(kX86RegTypeK, 1, 8),
    REG(kX86RegTypeK, 2, 8),
    REG(kX86RegTypeK, 3, 8),
    REG(kX86RegTypeK, 4, 8),
    REG(kX86RegTypeK, 5, 8),
    REG(kX86RegTypeK, 6, 8),
    REG(kX86RegTypeK, 7, 8)
  },

  // Xmm.
  {
    REG(kX86RegTypeXmm, 0, 16),
    REG(kX86RegTypeXmm, 1, 16),
    REG(kX86RegTypeXmm, 2, 16),
    REG(kX86RegTypeXmm, 3, 16),
    REG(kX86RegTypeXmm, 4, 16),
    REG(kX86RegTypeXmm, 5, 16),
    REG(kX86RegTypeXmm, 6, 16),
    REG(kX86RegTypeXmm, 7, 16),
    REG(kX86RegTypeXmm, 8, 16),
    REG(kX86RegTypeXmm, 9, 16),
    REG(kX86RegTypeXmm, 10, 16),
    REG(kX86RegTypeXmm, 11, 16),
    REG(kX86RegTypeXmm, 12, 16),
    REG(kX86RegTypeXmm, 13, 16),
    REG(kX86RegTypeXmm, 14, 16),
    REG(kX86RegTypeXmm, 15, 16),
    REG(kX86RegTypeXmm, 16, 16),
    REG(kX86RegTypeXmm, 17, 16),
    REG(kX86RegTypeXmm, 18, 16),
    REG(kX86RegTypeXmm, 19, 16),
    REG(kX86RegTypeXmm, 20, 16),
    REG(kX86RegTypeXmm, 21, 16),
    REG(kX86RegTypeXmm, 22, 16),
    REG(kX86RegTypeXmm, 23, 16),
    REG(kX86RegTypeXmm, 24, 16),
    REG(kX86RegTypeXmm, 25, 16),
    REG(kX86RegTypeXmm, 26, 16),
    REG(kX86RegTypeXmm, 27, 16),
    REG(kX86RegTypeXmm, 28, 16),
    REG(kX86RegTypeXmm, 29, 16),
    REG(kX86RegTypeXmm, 30, 16),
    REG(kX86RegTypeXmm, 31, 16)
  },

  // Ymm.
  {
    REG(kX86RegTypeYmm, 0, 32),
    REG(kX86RegTypeYmm, 1, 32),
    REG(kX86RegTypeYmm, 2, 32),
    REG(kX86RegTypeYmm, 3, 32),
    REG(kX86RegTypeYmm, 4, 32),
    REG(kX86RegTypeYmm, 5, 32),
    REG(kX86RegTypeYmm, 6, 32),
    REG(kX86RegTypeYmm, 7, 32),
    REG(kX86RegTypeYmm, 8, 32),
    REG(kX86RegTypeYmm, 9, 32),
    REG(kX86RegTypeYmm, 10, 32),
    REG(kX86RegTypeYmm, 11, 32),
    REG(kX86RegTypeYmm, 12, 32),
    REG(kX86RegTypeYmm, 13, 32),
    REG(kX86RegTypeYmm, 14, 32),
    REG(kX86RegTypeYmm, 15, 32),
    REG(kX86RegTypeYmm, 16, 32),
    REG(kX86RegTypeYmm, 17, 32),
    REG(kX86RegTypeYmm, 18, 32),
    REG(kX86RegTypeYmm, 19, 32),
    REG(kX86RegTypeYmm, 20, 32),
    REG(kX86RegTypeYmm, 21, 32),
    REG(kX86RegTypeYmm, 22, 32),
    REG(kX86RegTypeYmm, 23, 32),
    REG(kX86RegTypeYmm, 24, 32),
    REG(kX86RegTypeYmm, 25, 32),
    REG(kX86RegTypeYmm, 26, 32),
    REG(kX86RegTypeYmm, 27, 32),
    REG(kX86RegTypeYmm, 28, 32),
    REG(kX86RegTypeYmm, 29, 32),
    REG(kX86RegTypeYmm, 30, 32),
    REG(kX86RegTypeYmm, 31, 32)
  },

  // Zmm.
  {
    REG(kX86RegTypeZmm, 0, 64),
    REG(kX86RegTypeZmm, 1, 64),
    REG(kX86RegTypeZmm, 2, 64),
    REG(kX86RegTypeZmm, 3, 64),
    REG(kX86RegTypeZmm, 4, 64),
    REG(kX86RegTypeZmm, 5, 64),
    REG(kX86RegTypeZmm, 6, 64),
    REG(kX86RegTypeZmm, 7, 64),
    REG(kX86RegTypeZmm, 8, 64),
    REG(kX86RegTypeZmm, 9, 64),
    REG(kX86RegTypeZmm, 10, 64),
    REG(kX86RegTypeZmm, 11, 64),
    REG(kX86RegTypeZmm, 12, 64),
    REG(kX86RegTypeZmm, 13, 64),
    REG(kX86RegTypeZmm, 14, 64),
    REG(kX86RegTypeZmm, 15, 64),
    REG(kX86RegTypeZmm, 16, 64),
    REG(kX86RegTypeZmm, 17, 64),
    REG(kX86RegTypeZmm, 18, 64),
    REG(kX86RegTypeZmm, 19, 64),
    REG(kX86RegTypeZmm, 20, 64),
    REG(kX86RegTypeZmm, 21, 64),
    REG(kX86RegTypeZmm, 22, 64),
    REG(kX86RegTypeZmm, 23, 64),
    REG(kX86RegTypeZmm, 24, 64),
    REG(kX86RegTypeZmm, 25, 64),
    REG(kX86RegTypeZmm, 26, 64),
    REG(kX86RegTypeZmm, 27, 64),
    REG(kX86RegTypeZmm, 28, 64),
    REG(kX86RegTypeZmm, 29, 64),
    REG(kX86RegTypeZmm, 30, 64),
    REG(kX86RegTypeZmm, 31, 64)
  }
};

#undef REG

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
