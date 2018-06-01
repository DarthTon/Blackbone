// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER) && (defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64))

// [Dependencies - AsmJit]
#include "../base/intutil.h"
#include "../base/string.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86context_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Debug]
// ============================================================================

#if !defined(ASMJIT_DEBUG)
#define ASMJIT_ASSERT_UNINITIALIZED(op) \
  do {} while(0)
#else
#define ASMJIT_ASSERT_UNINITIALIZED(op) \
  do { \
    if (op.isVar() || op.isLabel()) { \
      ASMJIT_ASSERT(op.getId() != kInvalidValue); \
    } \
  } while(0)
#endif

// ============================================================================
// [asmjit::X86VarInfo]
// ============================================================================

#define C(_Class_) kX86RegClass##_Class_
#define D(_Desc_) kVarFlag##_Desc_

const X86VarInfo _x86VarInfo[] = {
  /* 00: kVarTypeInt8     */ { kX86RegTypeGpbLo, 1 , C(Gp) , 0                , "gpb" },
  /* 01: kVarTypeUInt8    */ { kX86RegTypeGpbLo, 1 , C(Gp) , 0                , "gpb" },
  /* 02: kVarTypeInt16    */ { kX86RegTypeGpw  , 2 , C(Gp) , 0                , "gpw" },
  /* 03: kVarTypeUInt16   */ { kX86RegTypeGpw  , 2 , C(Gp) , 0                , "gpw" },
  /* 04: kVarTypeInt32    */ { kX86RegTypeGpd  , 4 , C(Gp) , 0                , "gpd" },
  /* 05: kVarTypeUInt32   */ { kX86RegTypeGpd  , 4 , C(Gp) , 0                , "gpd" },
  /* 06: kVarTypeInt64    */ { kX86RegTypeGpq  , 8 , C(Gp) , 0                , "gpq" },
  /* 07: kVarTypeUInt64   */ { kX86RegTypeGpq  , 8 , C(Gp) , 0                , "gpq" },
  /* 08: kVarTypeIntPtr   */ { 0               , 0 , C(Gp) , 0                , ""    }, // Remapped.
  /* 09: kVarTypeUIntPtr  */ { 0               , 0 , C(Gp) , 0                , ""    }, // Remapped.
  /* 10: kVarTypeFp32     */ { kX86RegTypeFp   , 4 , C(Fp) , D(Sp)            , "fp"  },
  /* 11: kVarTypeFp64     */ { kX86RegTypeFp   , 8 , C(Fp) , D(Dp)            , "fp"  },
  /* 12: kX86VarTypeMm    */ { kX86RegTypeMm   , 8 , C(Mm) , 0                , "mm"  },
  /* 13: kX86VarTypeK     */ { kX86RegTypeK    , 8 , C(K)  , 0                , "k"   },
  /* 14: kX86VarTypeXmm   */ { kX86RegTypeXmm  , 16, C(Xyz), 0                , "xmm" },
  /* 15: kX86VarTypeXmmSs */ { kX86RegTypeXmm  , 4 , C(Xyz), D(Sp)            , "xmm" },
  /* 16: kX86VarTypeXmmPs */ { kX86RegTypeXmm  , 16, C(Xyz), D(Sp) | D(Packed), "xmm" },
  /* 17: kX86VarTypeXmmSd */ { kX86RegTypeXmm  , 8 , C(Xyz), D(Dp)            , "xmm" },
  /* 18: kX86VarTypeXmmPd */ { kX86RegTypeXmm  , 16, C(Xyz), D(Dp) | D(Packed), "xmm" },
  /* 19: kX86VarTypeYmm   */ { kX86RegTypeYmm  , 32, C(Xyz), 0                , "ymm" },
  /* 20: kX86VarTypeYmmPs */ { kX86RegTypeYmm  , 32, C(Xyz), D(Sp) | D(Packed), "ymm" },
  /* 21: kX86VarTypeYmmPd */ { kX86RegTypeYmm  , 32, C(Xyz), D(Dp) | D(Packed), "ymm" },
  /* 22: kX86VarTypeZmm   */ { kX86RegTypeZmm  , 64, C(Xyz), 0                , "zmm" },
  /* 23: kX86VarTypeZmmPs */ { kX86RegTypeZmm  , 64, C(Xyz), D(Sp) | D(Packed), "zmm" },
  /* 24: kX86VarTypeZmmPd */ { kX86RegTypeZmm  , 64, C(Xyz), D(Dp) | D(Packed), "zmm" }
};

#undef D
#undef C

#if defined(ASMJIT_BUILD_X86)
const uint8_t _x86VarMapping[kX86VarTypeCount] = {
  /* 00: kVarTypeInt8     */ kVarTypeInt8,
  /* 01: kVarTypeUInt8    */ kVarTypeUInt8,
  /* 02: kVarTypeInt16    */ kVarTypeInt16,
  /* 03: kVarTypeUInt16   */ kVarTypeUInt16,
  /* 04: kVarTypeInt32    */ kVarTypeInt32,
  /* 05: kVarTypeUInt32   */ kVarTypeUInt32,
  /* 06: kVarTypeInt64    */ kInvalidVar,     // Invalid in 32-bit mode.
  /* 07: kVarTypeUInt64   */ kInvalidVar,     // Invalid in 32-bit mode.
  /* 08: kVarTypeIntPtr   */ kVarTypeInt32,   // Remapped to Int32.
  /* 09: kVarTypeUIntPtr  */ kVarTypeUInt32,  // Remapped to UInt32.
  /* 10: kVarTypeFp32     */ kVarTypeFp32,
  /* 11: kVarTypeFp64     */ kVarTypeFp64,
  /* 12: kX86VarTypeMm    */ kX86VarTypeMm,
  /* 13: kX86VarTypeK     */ kX86VarTypeK,
  /* 14: kX86VarTypeXmm   */ kX86VarTypeXmm,
  /* 15: kX86VarTypeXmmSs */ kX86VarTypeXmmSs,
  /* 16: kX86VarTypeXmmPs */ kX86VarTypeXmmPs,
  /* 17: kX86VarTypeXmmSd */ kX86VarTypeXmmSd,
  /* 18: kX86VarTypeXmmPd */ kX86VarTypeXmmPd,
  /* 19: kX86VarTypeYmm   */ kX86VarTypeYmm,
  /* 20: kX86VarTypeYmmPs */ kX86VarTypeYmmPs,
  /* 21: kX86VarTypeYmmPd */ kX86VarTypeYmmPd,
  /* 22: kX86VarTypeZmm   */ kX86VarTypeZmm,
  /* 23: kX86VarTypeZmmPs */ kX86VarTypeZmmPs,
  /* 24: kX86VarTypeZmmPd */ kX86VarTypeZmmPd
};
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
const uint8_t _x64VarMapping[kX86VarTypeCount] = {
  /* 00: kVarTypeInt8     */ kVarTypeInt8,
  /* 01: kVarTypeUInt8    */ kVarTypeUInt8,
  /* 02: kVarTypeInt16    */ kVarTypeInt16,
  /* 03: kVarTypeUInt16   */ kVarTypeUInt16,
  /* 04: kVarTypeInt32    */ kVarTypeInt32,
  /* 05: kVarTypeUInt32   */ kVarTypeUInt32,
  /* 06: kVarTypeInt64    */ kVarTypeInt64,
  /* 07: kVarTypeUInt64   */ kVarTypeUInt64,
  /* 08: kVarTypeIntPtr   */ kVarTypeInt64,   // Remapped to Int64.
  /* 09: kVarTypeUIntPtr  */ kVarTypeUInt64,  // Remapped to UInt64.
  /* 10: kVarTypeFp32     */ kVarTypeFp32,
  /* 11: kVarTypeFp64     */ kVarTypeFp64,
  /* 12: kX86VarTypeMm    */ kX86VarTypeMm,
  /* 13: kX86VarTypeK     */ kX86VarTypeK,
  /* 14: kX86VarTypeXmm   */ kX86VarTypeXmm,
  /* 15: kX86VarTypeXmmSs */ kX86VarTypeXmmSs,
  /* 16: kX86VarTypeXmmPs */ kX86VarTypeXmmPs,
  /* 17: kX86VarTypeXmmSd */ kX86VarTypeXmmSd,
  /* 18: kX86VarTypeXmmPd */ kX86VarTypeXmmPd,
  /* 19: kX86VarTypeYmm   */ kX86VarTypeYmm,
  /* 20: kX86VarTypeYmmPs */ kX86VarTypeYmmPs,
  /* 21: kX86VarTypeYmmPd */ kX86VarTypeYmmPd,
  /* 22: kX86VarTypeZmm   */ kX86VarTypeZmm,
  /* 23: kX86VarTypeZmmPs */ kX86VarTypeZmmPs,
  /* 24: kX86VarTypeZmmPd */ kX86VarTypeZmmPd
};
#endif // ASMJIT_BUILD_X64

// ============================================================================
// [asmjit::X86FuncDecl - Helpers]
// ============================================================================

static ASMJIT_INLINE bool x86ArgIsInt(uint32_t aType) {
  ASMJIT_ASSERT(aType < kX86VarTypeCount);
  return IntUtil::inInterval<uint32_t>(aType, _kVarTypeIntStart, _kVarTypeIntEnd);
}

static ASMJIT_INLINE bool x86ArgIsFp(uint32_t aType) {
  ASMJIT_ASSERT(aType < kX86VarTypeCount);
  return IntUtil::inInterval<uint32_t>(aType, _kVarTypeFpStart, _kVarTypeFpEnd);
}

static ASMJIT_INLINE uint32_t x86ArgTypeToXmmType(uint32_t aType) {
  if (aType == kVarTypeFp32)
    return kX86VarTypeXmmSs;
  if (aType == kVarTypeFp64)
    return kX86VarTypeXmmSd;
  return aType;
}

//! Get an architecture from calling convention.
//!
//! Returns `kArchX86` or `kArchX64` depending on `conv`.
static ASMJIT_INLINE uint32_t x86GetArchFromCConv(uint32_t conv) {
  return IntUtil::inInterval<uint32_t>(conv, kX86FuncConvW64, kX86FuncConvU64) ? kArchX64 : kArchX86;
}

// ============================================================================
// [asmjit::X86FuncDecl - SetPrototype]
// ============================================================================

#define R(_Index_) kX86RegIndex##_Index_
static uint32_t X86FuncDecl_initConv(X86FuncDecl* self, uint32_t arch, uint32_t conv) {
  // Setup defaults.
  self->_argStackSize = 0;
  self->_redZoneSize = 0;
  self->_spillZoneSize = 0;

  self->_convention = static_cast<uint8_t>(conv);
  self->_calleePopsStack = false;
  self->_direction = kFuncDirRtl;

  self->_passed.reset();
  self->_preserved.reset();

  ::memset(self->_passedOrderGp, kInvalidReg, ASMJIT_ARRAY_SIZE(self->_passedOrderGp));
  ::memset(self->_passedOrderXmm, kInvalidReg, ASMJIT_ARRAY_SIZE(self->_passedOrderXmm));

  // --------------------------------------------------------------------------
  // [X86 Support]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_BUILD_X86)
  if (arch == kArchX86) {
    self->_preserved.set(kX86RegClassGp, IntUtil::mask(R(Bx), R(Sp), R(Bp), R(Si), R(Di)));

    switch (conv) {
      case kX86FuncConvCDecl:
        break;

      case kX86FuncConvStdCall:
        self->_calleePopsStack = true;
        break;

      case kX86FuncConvMsThisCall:
        self->_calleePopsStack = true;
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Cx)));
        self->_passedOrderGp[0] = R(Cx);
        break;

      case kX86FuncConvMsFastCall:
        self->_calleePopsStack = true;
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Cx), R(Cx)));
        self->_passedOrderGp[0] = R(Cx);
        self->_passedOrderGp[1] = R(Dx);
        break;

      case kX86FuncConvBorlandFastCall:
        self->_calleePopsStack = true;
        self->_direction = kFuncDirLtr;
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Ax), R(Dx), R(Cx)));
        self->_passedOrderGp[0] = R(Ax);
        self->_passedOrderGp[1] = R(Dx);
        self->_passedOrderGp[2] = R(Cx);
        break;

      case kX86FuncConvGccFastCall:
        self->_calleePopsStack = true;
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Cx), R(Dx)));
        self->_passedOrderGp[0] = R(Cx);
        self->_passedOrderGp[1] = R(Dx);
        break;

      case kX86FuncConvGccRegParm1:
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Ax)));
        self->_passedOrderGp[0] = R(Ax);
        break;

      case kX86FuncConvGccRegParm2:
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Ax), R(Dx)));
        self->_passedOrderGp[0] = R(Ax);
        self->_passedOrderGp[1] = R(Dx);
        break;

      case kX86FuncConvGccRegParm3:
        self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Ax), R(Dx), R(Cx)));
        self->_passedOrderGp[0] = R(Ax);
        self->_passedOrderGp[1] = R(Dx);
        self->_passedOrderGp[2] = R(Cx);
        break;

      default:
        ASMJIT_ASSERT(!"Reached");
    }

    return kErrorOk;
  }
#endif // ASMJIT_BUILD_X86

  // --------------------------------------------------------------------------
  // [X64 Support]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_BUILD_X64)
  switch (conv) {
    case kX86FuncConvW64:
      self->_spillZoneSize = 32;

      self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Cx), R(Dx), 8, 9));
      self->_passedOrderGp[0] = R(Cx);
      self->_passedOrderGp[1] = R(Dx);
      self->_passedOrderGp[2] = 8;
      self->_passedOrderGp[3] = 9;

      self->_passed.set(kX86RegClassXyz, IntUtil::mask(0, 1, 2, 3));
      self->_passedOrderXmm[0] = 0;
      self->_passedOrderXmm[1] = 1;
      self->_passedOrderXmm[2] = 2;
      self->_passedOrderXmm[3] = 3;

      self->_preserved.set(kX86RegClassGp , IntUtil::mask(R(Bx), R(Sp), R(Bp), R(Si), R(Di), 12, 13, 14, 15));
      self->_preserved.set(kX86RegClassXyz, IntUtil::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
      break;

    case kX86FuncConvU64:
      self->_redZoneSize = 128;

      self->_passed.set(kX86RegClassGp, IntUtil::mask(R(Di), R(Si), R(Dx), R(Cx), 8, 9));
      self->_passedOrderGp[0] = R(Di);
      self->_passedOrderGp[1] = R(Si);
      self->_passedOrderGp[2] = R(Dx);
      self->_passedOrderGp[3] = R(Cx);
      self->_passedOrderGp[4] = 8;
      self->_passedOrderGp[5] = 9;

      self->_passed.set(kX86RegClassXyz, IntUtil::mask(0, 1, 2, 3, 4, 5, 6, 7));
      self->_passedOrderXmm[0] = 0;
      self->_passedOrderXmm[1] = 1;
      self->_passedOrderXmm[2] = 2;
      self->_passedOrderXmm[3] = 3;
      self->_passedOrderXmm[4] = 4;
      self->_passedOrderXmm[5] = 5;
      self->_passedOrderXmm[6] = 6;
      self->_passedOrderXmm[7] = 7;

      self->_preserved.set(kX86RegClassGp, IntUtil::mask(R(Bx), R(Sp), R(Bp), 12, 13, 14, 15));
      break;

    default:
      ASMJIT_ASSERT(!"Reached");
  }
#endif // ASMJIT_BUILD_X64

  return kErrorOk;
}
#undef R

static Error X86FuncDecl_initFunc(X86FuncDecl* self, uint32_t arch,
  uint32_t ret, const uint32_t* argList, uint32_t argCount) {

  ASMJIT_ASSERT(argCount <= kFuncArgCount);

  uint32_t conv = self->_convention;
  uint32_t regSize = (arch == kArchX86) ? 4 : 8;

  int32_t i = 0;
  int32_t gpPos = 0;
  int32_t xmmPos = 0;
  int32_t stackOffset = 0;

  const uint8_t* varMapping;

#if defined(ASMJIT_BUILD_X86)
  if (arch == kArchX86)
    varMapping = _x86VarMapping;
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
  if (arch == kArchX64)
    varMapping = _x64VarMapping;
#endif // ASMJIT_BUILD_X64

  self->_argCount = static_cast<uint8_t>(argCount);
  self->_retCount = 0;

  for (i = 0; i < static_cast<int32_t>(argCount); i++) {
    FuncInOut& arg = self->getArg(i);
    arg._varType = static_cast<uint8_t>(varMapping[argList[i]]);
    arg._regIndex = kInvalidReg;
    arg._stackOffset = kFuncStackInvalid;
  }

  for (; i < kFuncArgCount; i++) {
    self->_argList[i].reset();
  }

  self->_retList[0].reset();
  self->_retList[1].reset();
  self->_argStackSize = 0;
  self->_used.reset();

  if (ret != kInvalidVar) {
    ret = varMapping[ret];
    switch (ret) {
      case kVarTypeInt64:
      case kVarTypeUInt64:
        // 64-bit value is returned in EDX:EAX on x86.
#if defined(ASMJIT_BUILD_X86)
        if (arch == kArchX86) {
          self->_retCount = 2;
          self->_retList[0]._varType = kVarTypeUInt32;
          self->_retList[0]._regIndex = kX86RegIndexAx;
          self->_retList[1]._varType = static_cast<uint8_t>(ret - 2);
          self->_retList[1]._regIndex = kX86RegIndexDx;
        }
#endif // ASMJIT_BUILD_X86
        // ... Fall through ...

      case kVarTypeInt8:
      case kVarTypeUInt8:
      case kVarTypeInt16:
      case kVarTypeUInt16:
      case kVarTypeInt32:
      case kVarTypeUInt32:
        self->_retCount = 1;
        self->_retList[0]._varType = static_cast<uint8_t>(ret);
        self->_retList[0]._regIndex = kX86RegIndexAx;
        break;

      case kX86VarTypeMm:
        self->_retCount = 1;
        self->_retList[0]._varType = static_cast<uint8_t>(ret);
        self->_retList[0]._regIndex = 0;
        break;

      case kVarTypeFp32:
        self->_retCount = 1;
        if (arch == kArchX86) {
          self->_retList[0]._varType = kVarTypeFp32;
          self->_retList[0]._regIndex = 0;
        }
        else {
          self->_retList[0]._varType = kX86VarTypeXmmSs;
          self->_retList[0]._regIndex = 0;
        }
        break;

      case kVarTypeFp64:
        self->_retCount = 1;
        if (arch == kArchX86) {
          self->_retList[0]._varType = kVarTypeFp64;
          self->_retList[0]._regIndex = 0;
        }
        else {
          self->_retList[0]._varType = kX86VarTypeXmmSd;
          self->_retList[0]._regIndex = 0;
          break;
        }
        break;

      case kX86VarTypeXmm:
      case kX86VarTypeXmmSs:
      case kX86VarTypeXmmSd:
      case kX86VarTypeXmmPs:
      case kX86VarTypeXmmPd:
        self->_retCount = 1;
        self->_retList[0]._varType = static_cast<uint8_t>(ret);
        self->_retList[0]._regIndex = 0;
        break;
    }
  }

  if (self->_argCount == 0)
    return kErrorOk;

#if defined(ASMJIT_BUILD_X86)
  if (arch == kArchX86) {
    // Register arguments (Integer), always left-to-right.
    for (i = 0; i != static_cast<int32_t>(argCount); i++) {
      FuncInOut& arg = self->getArg(i);
      uint32_t varType = varMapping[arg.getVarType()];

      if (!x86ArgIsInt(varType) || gpPos >= ASMJIT_ARRAY_SIZE(self->_passedOrderGp))
        continue;

      if (self->_passedOrderGp[gpPos] == kInvalidReg)
        continue;

      arg._regIndex = self->_passedOrderGp[gpPos++];
      self->_used.or_(kX86RegClassGp, IntUtil::mask(arg.getRegIndex()));
    }

    // Stack arguments.
    int32_t iStart = static_cast<int32_t>(argCount - 1);
    int32_t iEnd   = -1;
    int32_t iStep  = -1;

    if (self->_direction == kFuncDirLtr) {
      iStart = 0;
      iEnd   = static_cast<int32_t>(argCount);
      iStep  = 1;
    }

    for (i = iStart; i != iEnd; i += iStep) {
      FuncInOut& arg = self->getArg(i);
      uint32_t varType = varMapping[arg.getVarType()];

      if (arg.hasRegIndex())
        continue;

      if (x86ArgIsInt(varType)) {
        stackOffset -= 4;
        arg._stackOffset = static_cast<int16_t>(stackOffset);
      }
      else if (x86ArgIsFp(varType)) {
        int32_t size = static_cast<int32_t>(_x86VarInfo[varType].getSize());
        stackOffset -= size;
        arg._stackOffset = static_cast<int16_t>(stackOffset);
      }
    }
  }
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
  if (arch == kArchX64) {
    if (conv == kX86FuncConvW64) {
      int32_t argMax = IntUtil::iMin<int32_t>(argCount, 4);

      // Register arguments (Gp/Xmm), always left-to-right.
      for (i = 0; i != argMax; i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (x86ArgIsInt(varType) && i < ASMJIT_ARRAY_SIZE(self->_passedOrderGp)) {
          arg._regIndex = self->_passedOrderGp[i];
          self->_used.or_(kX86RegClassGp, IntUtil::mask(arg.getRegIndex()));
          continue;
        }

        if (x86ArgIsFp(varType) && i < ASMJIT_ARRAY_SIZE(self->_passedOrderXmm)) {
          arg._varType = static_cast<uint8_t>(x86ArgTypeToXmmType(varType));
          arg._regIndex = self->_passedOrderXmm[i];
          self->_used.or_(kX86RegClassXyz, IntUtil::mask(arg.getRegIndex()));
        }
      }

      // Stack arguments (always right-to-left).
      for (i = argCount - 1; i != -1; i--) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (arg.hasRegIndex())
          continue;

        if (x86ArgIsInt(varType)) {
          stackOffset -= 8; // Always 8 bytes.
          arg._stackOffset = stackOffset;
        }
        else if (x86ArgIsFp(varType)) {
          stackOffset -= 8; // Always 8 bytes (float/double).
          arg._stackOffset = stackOffset;
        }
      }

      // 32 bytes shadow space (X64W calling convention specific).
      stackOffset -= 4 * 8;
    }
    else {
      // Register arguments (Gp), always left-to-right.
      for (i = 0; i != static_cast<int32_t>(argCount); i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (!x86ArgIsInt(varType) || gpPos >= ASMJIT_ARRAY_SIZE(self->_passedOrderGp))
          continue;

        if (self->_passedOrderGp[gpPos] == kInvalidReg)
          continue;

        arg._regIndex = self->_passedOrderGp[gpPos++];
        self->_used.or_(kX86RegClassGp, IntUtil::mask(arg.getRegIndex()));
      }

      // Register arguments (Xmm), always left-to-right.
      for (i = 0; i != static_cast<int32_t>(argCount); i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (x86ArgIsFp(varType)) {
          arg._varType = static_cast<uint8_t>(x86ArgTypeToXmmType(varType));
          arg._regIndex = self->_passedOrderXmm[xmmPos++];
          self->_used.or_(kX86RegClassXyz, IntUtil::mask(arg.getRegIndex()));
        }
      }

      // Stack arguments.
      for (i = argCount - 1; i != -1; i--) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (arg.hasRegIndex())
          continue;

        if (x86ArgIsInt(varType)) {
          stackOffset -= 8;
          arg._stackOffset = static_cast<int16_t>(stackOffset);
        }
        else if (x86ArgIsFp(varType)) {
          int32_t size = static_cast<int32_t>(_x86VarInfo[varType].getSize());

          stackOffset -= size;
          arg._stackOffset = static_cast<int16_t>(stackOffset);
        }
      }
    }
  }
#endif // ASMJIT_BUILD_X64

  // Modify the stack offset, thus in result all parameters would have positive
  // non-zero stack offset.
  for (i = 0; i < static_cast<int32_t>(argCount); i++) {
    FuncInOut& arg = self->getArg(i);
    if (!arg.hasRegIndex()) {
      arg._stackOffset += static_cast<uint16_t>(static_cast<int32_t>(regSize) - stackOffset);
    }
  }

  self->_argStackSize = static_cast<uint32_t>(-stackOffset);
  return kErrorOk;
}

Error X86FuncDecl::setPrototype(uint32_t conv, const FuncPrototype& p) {
  if (conv == kFuncConvNone || conv >= _kX86FuncConvCount)
    return kErrorInvalidArgument;

  if (p.getArgCount() > kFuncArgCount)
    return kErrorInvalidArgument;

  // Validate that the required convention is supported by the current asmjit
  // configuration, if only one target is compiled.
  uint32_t arch = x86GetArchFromCConv(conv);
#if defined(ASMJIT_BUILD_X86) && !defined(ASMJIT_BUILD_X64)
  if (arch == kArchX64)
    return kErrorInvalidState;
#endif // ASMJIT_BUILD_X86 && !ASMJIT_BUILD_X64

#if !defined(ASMJIT_BUILD_X86) && defined(ASMJIT_BUILD_X64)
  if (arch == kArchX86)
    return kErrorInvalidState;
#endif // !ASMJIT_BUILD_X86 && ASMJIT_BUILD_X64

  ASMJIT_PROPAGATE_ERROR(X86FuncDecl_initConv(this, arch, conv));
  ASMJIT_PROPAGATE_ERROR(X86FuncDecl_initFunc(this, arch, p.getRet(), p.getArgList(), p.getArgCount()));

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86FuncDecl - Reset]
// ============================================================================

void X86FuncDecl::reset() {
  uint32_t i;

  _convention = kFuncConvNone;
  _calleePopsStack = false;
  _direction = kFuncDirRtl;
  _reserved0 = 0;

  _argCount = 0;
  _retCount = 0;

  _argStackSize = 0;
  _redZoneSize = 0;
  _spillZoneSize = 0;

  for (i = 0; i < ASMJIT_ARRAY_SIZE(_argList); i++) {
    _argList[i].reset();
  }

  _retList[0].reset();
  _retList[1].reset();

  _used.reset();
  _passed.reset();
  _preserved.reset();

  ::memset(_passedOrderGp, kInvalidReg, ASMJIT_ARRAY_SIZE(_passedOrderGp));
  ::memset(_passedOrderXmm, kInvalidReg, ASMJIT_ARRAY_SIZE(_passedOrderXmm));
}

// ============================================================================
// [asmjit::X86CallNode - Prototype]
// ============================================================================

Error X86CallNode::setPrototype(uint32_t conv, const FuncPrototype& p) {
  return _x86Decl.setPrototype(conv, p);
}

// ============================================================================
// [asmjit::X86CallNode - Arg / Ret]
// ============================================================================

bool X86CallNode::_setArg(uint32_t i, const Operand& op) {
  if ((i & ~kFuncArgHi) >= _x86Decl.getArgCount())
    return false;

  _args[i] = op;
  return true;
}

bool X86CallNode::_setRet(uint32_t i, const Operand& op) {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::X86Compiler - Helpers (Private)]
// ============================================================================

static Error X86Compiler_emitConstPool(X86Compiler* self,
  Label& label, ConstPool& pool) {

  if (label.getId() == kInvalidValue)
    return kErrorOk;

  self->align(kAlignData, static_cast<uint32_t>(pool.getAlignment()));
  self->bind(label);

  EmbedNode* embedNode = self->embed(NULL, static_cast<uint32_t>(pool.getSize()));
  if (embedNode == NULL)
    return kErrorNoHeapMemory;

  pool.fill(embedNode->getData());
  pool.reset();
  label.reset();

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Compiler - Construction / Destruction]
// ============================================================================

X86Compiler::X86Compiler(Runtime* runtime, uint32_t arch) :
  Compiler(runtime),
  zax(NoInit),
  zcx(NoInit),
  zdx(NoInit),
  zbx(NoInit),
  zsp(NoInit),
  zbp(NoInit),
  zsi(NoInit),
  zdi(NoInit) {

  setArch(arch);
}

X86Compiler::~X86Compiler() {}

// ============================================================================
// [asmjit::X86Compiler - Arch]
// ============================================================================

Error X86Compiler::setArch(uint32_t arch) {
#if defined(ASMJIT_BUILD_X86)
  if (arch == kArchX86) {
    _arch = kArchX86;
    _regSize = 4;

    _regCount.reset();
    _regCount._gp  = 8;
    _regCount._mm  = 8;
    _regCount._k   = 8;
    _regCount._xyz = 8;

    zax = x86::eax;
    zcx = x86::ecx;
    zdx = x86::edx;
    zbx = x86::ebx;
    zsp = x86::esp;
    zbp = x86::ebp;
    zsi = x86::esi;
    zdi = x86::edi;

    _targetVarMapping = _x86VarMapping;
    return kErrorOk;
  }
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
  if (arch == kArchX64) {
    _arch = kArchX64;
    _regSize = 8;

    _regCount.reset();
    _regCount._gp  = 16;
    _regCount._mm  = 8;
    _regCount._k   = 8;
    _regCount._xyz = 16;

    zax = x86::rax;
    zcx = x86::rcx;
    zdx = x86::rdx;
    zbx = x86::rbx;
    zsp = x86::rsp;
    zbp = x86::rbp;
    zsi = x86::rsi;
    zdi = x86::rdi;

    _targetVarMapping = _x64VarMapping;
    return kErrorOk;
  }
#endif // ASMJIT_BUILD_X64

  ASMJIT_ASSERT(!"Reached");
  return kErrorInvalidArgument;
}

// ============================================================================
// [asmjit::X86Compiler - Inst]
// ============================================================================

//! Get compiler instruction item size without operands assigned.
static ASMJIT_INLINE size_t X86Compiler_getInstSize(uint32_t code) {
  return IntUtil::inInterval<uint32_t>(code, _kX86InstIdJbegin, _kX86InstIdJend) ? sizeof(JumpNode) : sizeof(InstNode);
}

static InstNode* X86Compiler_newInst(X86Compiler* self, void* p, uint32_t code, uint32_t options, Operand* opList, uint32_t opCount) {
  if (IntUtil::inInterval<uint32_t>(code, _kX86InstIdJbegin, _kX86InstIdJend)) {
    JumpNode* node = new(p) JumpNode(self, code, options, opList, opCount);
    TargetNode* jTarget = self->getTargetById(opList[0].getId());

    node->orFlags(code == kX86InstIdJmp ? kNodeFlagIsJmp | kNodeFlagIsTaken : kNodeFlagIsJcc);
    node->_target = jTarget;
    node->_jumpNext = static_cast<JumpNode*>(jTarget->_from);

    jTarget->_from = node;
    jTarget->addNumRefs();

    // The 'jmp' is always taken, conditional jump can contain hint, we detect it.
    if (code == kX86InstIdJmp)
      node->orFlags(kNodeFlagIsTaken);
    else if (options & kInstOptionTaken)
      node->orFlags(kNodeFlagIsTaken);

    node->addOptions(options);
    return node;
  }
  else {
    InstNode* node = new(p) InstNode(self, code, options, opList, opCount);
    node->addOptions(options);
    return node;
  }
}

InstNode* X86Compiler::newInst(uint32_t code) {
  size_t size = X86Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size));

  if (inst == NULL)
    goto _NoMemory;

  return X86Compiler_newInst(this, inst, code, getInstOptionsAndReset(), NULL, 0);

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86Compiler::newInst(uint32_t code, const Operand& o0) {
  size_t size = X86Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 1 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    ASMJIT_ASSERT_UNINITIALIZED(o0);
    return X86Compiler_newInst(this, inst, code, getInstOptionsAndReset(), opList, 1);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1) {
  size_t size = X86Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 2 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    ASMJIT_ASSERT_UNINITIALIZED(o0);
    ASMJIT_ASSERT_UNINITIALIZED(o1);
    return X86Compiler_newInst(this, inst, code, getInstOptionsAndReset(), opList, 2);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) {
  size_t size = X86Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 3 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    opList[2] = o2;
    ASMJIT_ASSERT_UNINITIALIZED(o0);
    ASMJIT_ASSERT_UNINITIALIZED(o1);
    ASMJIT_ASSERT_UNINITIALIZED(o2);
    return X86Compiler_newInst(this, inst, code, getInstOptionsAndReset(), opList, 3);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3) {
  size_t size = X86Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 4 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    opList[2] = o2;
    opList[3] = o3;
    ASMJIT_ASSERT_UNINITIALIZED(o0);
    ASMJIT_ASSERT_UNINITIALIZED(o1);
    ASMJIT_ASSERT_UNINITIALIZED(o2);
    ASMJIT_ASSERT_UNINITIALIZED(o3);
    return X86Compiler_newInst(this, inst, code, getInstOptionsAndReset(), opList, 4);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3, const Operand& o4) {
  size_t size = X86Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 5 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    opList[2] = o2;
    opList[3] = o3;
    opList[4] = o4;
    ASMJIT_ASSERT_UNINITIALIZED(o0);
    ASMJIT_ASSERT_UNINITIALIZED(o1);
    ASMJIT_ASSERT_UNINITIALIZED(o2);
    ASMJIT_ASSERT_UNINITIALIZED(o3);
    ASMJIT_ASSERT_UNINITIALIZED(o4);
    return X86Compiler_newInst(this, inst, code, getInstOptionsAndReset(), opList, 5);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86Compiler::emit(uint32_t code) {
  InstNode* node = newInst(code);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0) {
  InstNode* node = newInst(code, o0);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1){
  InstNode* node = newInst(code, o0, o1);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) {
  InstNode* node = newInst(code, o0, o1, o2);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3){
  InstNode* node = newInst(code, o0, o1, o2, o3);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3, const Operand& o4) {
  InstNode* node = newInst(code, o0, o1, o2, o3, o4);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, int o0_) {
  Imm o0(o0_);
  InstNode* node = newInst(code, o0);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, uint64_t o0_) {
  Imm o0(o0_);
  InstNode* node = newInst(code, o0);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, int o1_) {
  Imm o1(o1_);
  InstNode* node = newInst(code, o0, o1);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, uint64_t o1_) {
  Imm o1(o1_);
  InstNode* node = newInst(code, o0, o1);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, int o2_) {
  Imm o2(o2_);
  InstNode* node = newInst(code, o0, o1, o2);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, uint64_t o2_) {
  Imm o2(o2_);
  InstNode* node = newInst(code, o0, o1, o2);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, int o3_) {
  Imm o3(o3_);
  InstNode* node = newInst(code, o0, o1, o2, o3);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, uint64_t o3_) {
  Imm o3(o3_);
  InstNode* node = newInst(code, o0, o1, o2, o3);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

// ============================================================================
// [asmjit::X86Compiler - Func]
// ============================================================================

X86FuncNode* X86Compiler::newFunc(uint32_t conv, const FuncPrototype& p) {
  X86FuncNode* func = newNode<X86FuncNode>();
  Error error;

  if (func == NULL)
    goto _NoMemory;

  // Create helper nodes.
  func->_entryNode = newTarget();
  func->_exitNode = newTarget();
  func->_end = newNode<EndNode>();

  if (func->_entryNode == NULL || func->_exitNode == NULL || func->_end == NULL)
    goto _NoMemory;

  // Emit push/pop sequence by default.
  func->_funcHints |= IntUtil::mask(kX86FuncHintPushPop);

  // Function prototype.
  if ((error = func->_x86Decl.setPrototype(conv, p)) != kErrorOk) {
    setError(error);
    return NULL;
  }

  // Function arguments stack size. Since function requires _argStackSize to be
  // set, we have to copy it from X86FuncDecl.
  func->_argStackSize = func->_x86Decl.getArgStackSize();
  func->_redZoneSize = static_cast<uint16_t>(func->_x86Decl.getRedZoneSize());
  func->_spillZoneSize = static_cast<uint16_t>(func->_x86Decl.getSpillZoneSize());

  // Expected/Required stack alignment.
  func->_expectedStackAlignment = getRuntime()->getStackAlignment();
  func->_requiredStackAlignment = 0;

  // Allocate space for function arguments.
  func->_argList = NULL;
  if (func->getArgCount() != 0) {
    func->_argList = _baseZone.allocT<VarData*>(func->getArgCount() * sizeof(VarData*));
    if (func->_argList == NULL)
      goto _NoMemory;
    ::memset(func->_argList, 0, func->getArgCount() * sizeof(VarData*));
  }

  return func;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

X86FuncNode* X86Compiler::addFunc(uint32_t conv, const FuncPrototype& p) {
  X86FuncNode* func = newFunc(conv, p);

  if (func == NULL) {
    setError(kErrorNoHeapMemory);
    return NULL;
  }

  ASMJIT_ASSERT(_func == NULL);
  _func = func;

  addNode(func);
  addNode(func->getEntryNode());

  return func;
}

EndNode* X86Compiler::endFunc() {
  X86FuncNode* func = getFunc();
  ASMJIT_ASSERT(func != NULL);

  // App function exit / epilog marker.
  addNode(func->getExitNode());

  // Add local constant pool at the end of the function (if exist).
  X86Compiler_emitConstPool(this, _localConstPoolLabel, _localConstPool);

  // Add function end marker.
  addNode(func->getEnd());

  // Finalize...
  func->addFuncFlags(kFuncFlagIsFinished);
  _func = NULL;

  return func->getEnd();
}

// ============================================================================
// [asmjit::X86Compiler - Ret]
// ============================================================================

RetNode* X86Compiler::newRet(const Operand& o0, const Operand& o1) {
  RetNode* node = newNode<RetNode>(o0, o1);
  if (node == NULL)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

RetNode* X86Compiler::addRet(const Operand& o0, const Operand& o1) {
  RetNode* node = newRet(o0, o1);
  if (node == NULL)
    return node;
  return static_cast<RetNode*>(addNode(node));
}

// ============================================================================
// [asmjit::X86Compiler - Call]
// ============================================================================

X86CallNode* X86Compiler::newCall(const Operand& o0, uint32_t conv, const FuncPrototype& p) {
  X86CallNode* node = newNode<X86CallNode>(o0);
  Error error;
  uint32_t nArgs;

  if (node == NULL)
    goto _NoMemory;

  if ((error = node->_x86Decl.setPrototype(conv, p)) != kErrorOk) {
    setError(error);
    return NULL;
  }

  // If there are no arguments skip the allocation.
  if ((nArgs = p.getArgCount()) == 0)
    return node;

  node->_args = static_cast<Operand*>(_baseZone.alloc(nArgs * sizeof(Operand)));
  if (node->_args == NULL)
    goto _NoMemory;

  ::memset(node->_args, 0, nArgs * sizeof(Operand));
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

X86CallNode* X86Compiler::addCall(const Operand& o0, uint32_t conv, const FuncPrototype& p) {
  X86CallNode* node = newCall(o0, conv, p);
  if (node == NULL)
    return NULL;
  return static_cast<X86CallNode*>(addNode(node));
}

// ============================================================================
// [asmjit::X86Compiler - Vars]
// ============================================================================

Error X86Compiler::setArg(uint32_t argIndex, Var& var) {
  X86FuncNode* func = getFunc();

  if (func == NULL)
    return kErrorInvalidArgument;

  if (!isVarValid(var))
    return kErrorInvalidState;

  VarData* vd = getVd(var);
  func->setArg(argIndex, vd);

  return kErrorOk;
}

Error X86Compiler::_newVar(Var* var, uint32_t vType, const char* name) {
  ASMJIT_ASSERT(vType < kX86VarTypeCount);

  vType = _targetVarMapping[vType];
  ASMJIT_ASSERT(vType != kInvalidVar);

  // There is not ASSERT in release mode and this should be checked.
  if (vType == kInvalidVar) {
    static_cast<X86Var*>(var)->reset();
    return kErrorInvalidArgument;
  }

  const X86VarInfo& vInfo = _x86VarInfo[vType];
  VarData* vd = _newVd(vType, vInfo.getSize(), vInfo.getClass(), name);

  if (vd == NULL) {
    static_cast<X86Var*>(var)->reset();
    return getError();
  }

  var->_init_packed_op_sz_w0_id(kOperandTypeVar, vd->getSize(), vInfo.getReg() << 8, vd->getId());
  var->_vreg.vType = vType;
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Compiler - Stack]
// ============================================================================

Error X86Compiler::_newStack(BaseMem* mem, uint32_t size, uint32_t alignment, const char* name) {
  if (size == 0)
    return kErrorInvalidArgument;

  if (alignment > 64)
    alignment = 64;

  VarData* vd = _newVd(kInvalidVar, size, kInvalidReg, name);
  if (vd == NULL) {
    static_cast<X86Mem*>(mem)->reset();
    return getError();
  }

  vd->_isStack = true;
  vd->_alignment = static_cast<uint8_t>(alignment);

  static_cast<X86Mem*>(mem)->_init(kMemTypeStackIndex, vd->getId(), 0, 0);
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Compiler - Const]
// ============================================================================

Error X86Compiler::_newConst(BaseMem* mem, uint32_t scope, const void* data, size_t size) {
  Error error = kErrorOk;
  size_t offset;

  Label* dstLabel;
  ConstPool* dstPool;

  if (scope == kConstScopeLocal) {
    dstLabel = &_localConstPoolLabel;
    dstPool = &_localConstPool;
  }
  else if (scope == kConstScopeGlobal) {
    dstLabel = &_globalConstPoolLabel;
    dstPool = &_globalConstPool;
  }
  else {
    error = kErrorInvalidArgument;
    goto _OnError;
  }

  error = dstPool->add(data, size, offset);
  if (error != kErrorOk)
    goto _OnError;

  if (dstLabel->getId() == kInvalidValue) {
    error = _newLabel(dstLabel);
    if (error != kErrorOk)
      goto _OnError;
  }

  *static_cast<X86Mem*>(mem) = x86::ptr(*dstLabel, static_cast<int32_t>(offset), static_cast<uint32_t>(size));
  return kErrorOk;

_OnError:
  return error;
}

// ============================================================================
// [asmjit::X86Compiler - Make]
// ============================================================================

void* X86Compiler::make() {
  Assembler* assembler = getAssembler();
  if (assembler == NULL) {
    setError(kErrorNoHeapMemory);
    return NULL;
  }

  Error error = serialize(assembler);
  if (error != kErrorOk) {
    setError(error);
    return NULL;
  }

  void* result = assembler->make();
  return result;
}

// ============================================================================
// [asmjit::X86Compiler - Assembler]
// ============================================================================

Assembler* X86Compiler::_newAssembler() {
  return new(std::nothrow) X86Assembler(_runtime, _arch);
}

// ============================================================================
// [asmjit::X86Compiler - Serialize]
// ============================================================================

Error X86Compiler::serialize(Assembler* assembler) {
  // Flush the global constant pool.
  X86Compiler_emitConstPool(this, _globalConstPoolLabel, _globalConstPool);

  if (_firstNode == NULL)
    return kErrorOk;

  X86Context context(this);
  Error error = kErrorOk;

  Node* node = _firstNode;
  Node* start;

  // Find function and use the context to translate/emit.
  do {
    start = node;

    if (node->getType() == kNodeTypeFunc) {
      node = static_cast<X86FuncNode*>(start)->getEnd();
      error = context.compile(static_cast<X86FuncNode*>(start));

      if (error != kErrorOk)
        goto _Error;
    }

    do {
      node = node->getNext();
    } while (node != NULL && node->getType() != kNodeTypeFunc);

    error = context.serialize(assembler, start, node);
    if (error != kErrorOk)
      goto _Error;
    context.cleanup();
  } while (node != NULL);
  return kErrorOk;

_Error:
  context.cleanup();
  return error;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && (ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64)
