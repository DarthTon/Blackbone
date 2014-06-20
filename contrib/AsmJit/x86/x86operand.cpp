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
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [asmjit::x86x64::Variables]
// ============================================================================

#define C(_Class_) kRegClass##_Class_
#define D(_Desc_) kVarDesc##_Desc_

const VarInfo _varInfo[] = {
  /* 00: kVarTypeInt8    */ { kRegTypeGpbLo, 1 , C(Gp) , 0                , "gpb" },
  /* 01: kVarTypeUInt8   */ { kRegTypeGpbLo, 1 , C(Gp) , 0                , "gpb" },
  /* 02: kVarTypeInt16   */ { kRegTypeGpw  , 2 , C(Gp) , 0                , "gpw" },
  /* 03: kVarTypeUInt16  */ { kRegTypeGpw  , 2 , C(Gp) , 0                , "gpw" },
  /* 04: kVarTypeInt32   */ { kRegTypeGpd  , 4 , C(Gp) , 0                , "gpd" },
  /* 05: kVarTypeUInt32  */ { kRegTypeGpd  , 4 , C(Gp) , 0                , "gpd" },
  /* 06: kVarTypeInt64   */ { kRegTypeGpq  , 8 , C(Gp) , 0                , "gpq" },
  /* 07: kVarTypeUInt64  */ { kRegTypeGpq  , 8 , C(Gp) , 0                , "gpq" },
  /* 08: kVarTypeIntPtr  */ { 0            , 0 , C(Gp) , 0                , ""    }, // Remapped.
  /* 09: kVarTypeUIntPtr */ { 0            , 0 , C(Gp) , 0                , ""    }, // Remapped.
  /* 10: kVarTypeFp32    */ { kRegTypeFp   , 4 , C(Fp) , D(Sp)            , "fp"  },
  /* 11: kVarTypeFp64    */ { kRegTypeFp   , 8 , C(Fp) , D(Dp)            , "fp"  },
  /* 12: kVarTypeMm      */ { kRegTypeMm   , 8 , C(Mm) , 0                , "mm"  },
  /* 13: kVarTypeXmm     */ { kRegTypeXmm  , 16, C(Xyz), 0                , "xmm" },
  /* 14: kVarTypeXmmSs   */ { kRegTypeXmm  , 4 , C(Xyz), D(Sp)            , "xmm" },
  /* 15: kVarTypeXmmPs   */ { kRegTypeXmm  , 16, C(Xyz), D(Sp) | D(Packed), "xmm" },
  /* 16: kVarTypeXmmSd   */ { kRegTypeXmm  , 8 , C(Xyz), D(Dp)            , "xmm" },
  /* 17: kVarTypeXmmPd   */ { kRegTypeXmm  , 16, C(Xyz), D(Dp) | D(Packed), "xmm" },
  /* 18: kVarTypeYmm     */ { kRegTypeYmm  , 32, C(Xyz), 0                , "ymm" },
  /* 19: kVarTypeYmmPs   */ { kRegTypeYmm  , 32, C(Xyz), D(Sp) | D(Packed), "ymm" },
  /* 20: kVarTypeYmmPd   */ { kRegTypeYmm  , 32, C(Xyz), D(Dp) | D(Packed), "ymm" }
};

#undef D
#undef C

// ============================================================================
// [asmjit::host::Mem - abs[]]
// ============================================================================

Mem ptr_abs(Ptr pAbs, int32_t disp, uint32_t size) {
  Mem m(NoInit);

  m._init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeAbsolute, 0, kInvalidValue);
  m._vmem.index = kInvalidValue;
  m._vmem.displacement = static_cast<int32_t>((intptr_t)(pAbs + disp));

  return m;
}

Mem ptr_abs(Ptr pAbs, const X86Reg& index, uint32_t shift, int32_t disp, uint32_t size) {
  Mem m(NoInit);
  uint32_t flags = shift << kMemShiftIndex;

  if (index.isGp()) flags |= Mem::_getGpdFlags(reinterpret_cast<const BaseVar&>(index));
  if (index.isXmm()) flags |= kMemVSibXmm << kMemVSibIndex;
  if (index.isYmm()) flags |= kMemVSibYmm << kMemVSibIndex;

  m._init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeAbsolute, flags, kInvalidValue);
  m._vmem.index = index.getRegIndex();
  m._vmem.displacement = static_cast<int32_t>((intptr_t)(pAbs + disp));

  return m;
}

Mem ptr_abs(Ptr pAbs, const X86Var& index, uint32_t shift, int32_t disp, uint32_t size) {
  Mem m(NoInit);
  uint32_t flags = shift << kMemShiftIndex;

  if (index.isGp()) flags |= Mem::_getGpdFlags(reinterpret_cast<const BaseVar&>(index));
  if (index.isXmm()) flags |= kMemVSibXmm << kMemVSibIndex;
  if (index.isYmm()) flags |= kMemVSibYmm << kMemVSibIndex;

  m._init_packed_op_sz_b0_b1_id(kOperandTypeMem, size, kMemTypeAbsolute, flags, kInvalidValue);
  m._vmem.index = index.getId();
  m._vmem.displacement = static_cast<int32_t>((intptr_t)(pAbs + disp));

  return m;
}

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

const uint8_t _varMapping[kVarTypeCount] = {
  /* 00: kVarTypeInt8    */ kVarTypeInt8,
  /* 01: kVarTypeUInt8   */ kVarTypeUInt8,
  /* 02: kVarTypeInt16   */ kVarTypeInt16,
  /* 03: kVarTypeUInt16  */ kVarTypeUInt16,
  /* 04: kVarTypeInt32   */ kVarTypeInt32,
  /* 05: kVarTypeUInt32  */ kVarTypeUInt32,
  /* 06: kVarTypeInt64   */ kVarTypeInvalid, // Invalid in 32-bit mode.
  /* 07: kVarTypeUInt64  */ kVarTypeInvalid, // Invalid in 32-bit mode.
  /* 08: kVarTypeIntPtr  */ kVarTypeInt32,   // Remapped.
  /* 09: kVarTypeUIntPtr */ kVarTypeUInt32,  // Remapped.
  /* 10: kVarTypeFp32    */ kVarTypeFp32,
  /* 11: kVarTypeFp64    */ kVarTypeFp64,
  /* 12: kVarTypeMm      */ kVarTypeMm,
  /* 13: kVarTypeXmm     */ kVarTypeXmm,
  /* 14: kVarTypeXmmSs   */ kVarTypeXmmSs,
  /* 15: kVarTypeXmmPs   */ kVarTypeXmmPs,
  /* 16: kVarTypeXmmSd   */ kVarTypeXmmSd,
  /* 17: kVarTypeXmmPd   */ kVarTypeXmmPd,
  /* 18: kVarTypeYmm     */ kVarTypeYmm,
  /* 19: kVarTypeYmmPs   */ kVarTypeYmmPs,
  /* 20: kVarTypeYmmPd   */ kVarTypeYmmPd
};

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)
namespace asmjit {
namespace x64 {

const uint8_t _varMapping[kVarTypeCount] = {
  /* 00: kVarTypeInt8    */ kVarTypeInt8,
  /* 01: kVarTypeUInt8   */ kVarTypeUInt8,
  /* 02: kVarTypeInt16   */ kVarTypeInt16,
  /* 03: kVarTypeUInt16  */ kVarTypeUInt16,
  /* 04: kVarTypeInt32   */ kVarTypeInt32,
  /* 05: kVarTypeUInt32  */ kVarTypeUInt32,
  /* 06: kVarTypeInt64   */ kVarTypeInt64,
  /* 07: kVarTypeUInt64  */ kVarTypeUInt64,
  /* 08: kVarTypeIntPtr  */ kVarTypeInt64,   // Remapped.
  /* 09: kVarTypeUIntPtr */ kVarTypeUInt64,  // Remapped.
  /* 10: kVarTypeFp32    */ kVarTypeFp32,
  /* 11: kVarTypeFp64    */ kVarTypeFp64,
  /* 12: kVarTypeMm      */ kVarTypeMm,
  /* 13: kVarTypeXmm     */ kVarTypeXmm,
  /* 14: kVarTypeXmmSs   */ kVarTypeXmmSs,
  /* 15: kVarTypeXmmPs   */ kVarTypeXmmPs,
  /* 16: kVarTypeXmmSd   */ kVarTypeXmmSd,
  /* 17: kVarTypeXmmPd   */ kVarTypeXmmPd,
  /* 18: kVarTypeYmm     */ kVarTypeYmm,
  /* 19: kVarTypeYmmPs   */ kVarTypeYmmPs,
  /* 20: kVarTypeYmmPd   */ kVarTypeYmmPd
};

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
