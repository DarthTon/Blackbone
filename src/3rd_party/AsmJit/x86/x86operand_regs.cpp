// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS
#define ASMJIT_EXPORTS_X86_OPERAND

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/misc_p.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86OpData]
// ============================================================================

// Register Operand {
//   uint32_t signature;
//   uint32_t id;
//   uint32_t reserved8_4;
//   uint32_t reserved12_4;
// }
#define ASMJIT_X86_REG_01(TYPE, ID)         \
{{{                                         \
  uint32_t(X86RegTraits<TYPE>::kSignature), \
  uint32_t(ID),                             \
  uint32_t(0),                              \
  uint32_t(0)                               \
}}}

#define ASMJIT_X86_REG_04(TYPE, ID) \
  ASMJIT_X86_REG_01(TYPE, ID + 0 ), \
  ASMJIT_X86_REG_01(TYPE, ID + 1 ), \
  ASMJIT_X86_REG_01(TYPE, ID + 2 ), \
  ASMJIT_X86_REG_01(TYPE, ID + 3 )

#define ASMJIT_X86_REG_07(TYPE, ID) \
  ASMJIT_X86_REG_04(TYPE, ID + 0 ), \
  ASMJIT_X86_REG_01(TYPE, ID + 4 ), \
  ASMJIT_X86_REG_01(TYPE, ID + 5 ), \
  ASMJIT_X86_REG_01(TYPE, ID + 6 )

#define ASMJIT_X86_REG_08(TYPE, ID) \
  ASMJIT_X86_REG_04(TYPE, ID + 0 ), \
  ASMJIT_X86_REG_04(TYPE, ID + 4 )

#define ASMJIT_X86_REG_16(TYPE, ID) \
  ASMJIT_X86_REG_08(TYPE, ID + 0 ), \
  ASMJIT_X86_REG_08(TYPE, ID + 8 )

#define ASMJIT_X86_REG_32(TYPE, ID) \
  ASMJIT_X86_REG_16(TYPE, ID + 0 ), \
  ASMJIT_X86_REG_16(TYPE, ID + 16)

const X86OpData x86OpData = {
  // --------------------------------------------------------------------------
  // [ArchRegs]
  // --------------------------------------------------------------------------

  {
    {
#define ASMJIT_X86_REG_SIGNATURE(TYPE) { X86RegTraits<TYPE>::kSignature }
      ASMJIT_TABLE_16(ASMJIT_X86_REG_SIGNATURE,  0),
      ASMJIT_TABLE_16(ASMJIT_X86_REG_SIGNATURE, 16)
#undef ASMJIT_X86_REG_SIGNATURE
    },

    // RegCount[]
    { ASMJIT_TABLE_T_32(X86RegTraits, kCount, 0) },

    // RegTypeToTypeId[]
    { ASMJIT_TABLE_T_32(X86RegTraits, kTypeId, 0) }
  },

  // --------------------------------------------------------------------------
  // [Registers]
  // --------------------------------------------------------------------------

  { ASMJIT_X86_REG_01(X86Reg::kRegRip  , 0) },
  { ASMJIT_X86_REG_07(X86Reg::kRegSeg  , 0) },
  { ASMJIT_X86_REG_16(X86Reg::kRegGpbLo, 0) },
  { ASMJIT_X86_REG_04(X86Reg::kRegGpbHi, 0) },
  { ASMJIT_X86_REG_16(X86Reg::kRegGpw  , 0) },
  { ASMJIT_X86_REG_16(X86Reg::kRegGpd  , 0) },
  { ASMJIT_X86_REG_16(X86Reg::kRegGpq  , 0) },
  { ASMJIT_X86_REG_08(X86Reg::kRegFp   , 0) },
  { ASMJIT_X86_REG_08(X86Reg::kRegMm   , 0) },
  { ASMJIT_X86_REG_08(X86Reg::kRegK    , 0) },
  { ASMJIT_X86_REG_32(X86Reg::kRegXmm  , 0) },
  { ASMJIT_X86_REG_32(X86Reg::kRegYmm  , 0) },
  { ASMJIT_X86_REG_32(X86Reg::kRegZmm  , 0) },
  { ASMJIT_X86_REG_04(X86Reg::kRegBnd  , 0) },
  { ASMJIT_X86_REG_16(X86Reg::kRegCr   , 0) },
  { ASMJIT_X86_REG_16(X86Reg::kRegDr   , 0) }
};

#undef ASMJIT_X86_REG_32
#undef ASMJIT_X86_REG_16
#undef ASMJIT_X86_REG_08
#undef ASMJIT_X86_REG_04
#undef ASMJIT_X86_REG_01

#undef ASMJIT_X86_REG_SIGNATURE

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
