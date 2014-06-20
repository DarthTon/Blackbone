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
#include "../base/globals.h"
#include "../base/intutil.h"
#include "../base/string.h"
#include "../x86/x86func.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [asmjit::X86X64FuncDecl - Helpers]
// ============================================================================

static ASMJIT_INLINE bool x86ArgIsInt(uint32_t aType) {
  ASMJIT_ASSERT(aType < kVarTypeCount);
  return IntUtil::inInterval<uint32_t>(aType, _kVarTypeIntStart, _kVarTypeIntEnd);
}

static ASMJIT_INLINE bool x86ArgIsFp(uint32_t aType) {
  ASMJIT_ASSERT(aType < kVarTypeCount);
  return IntUtil::inInterval<uint32_t>(aType, _kVarTypeFpStart, _kVarTypeFpEnd);
}

static ASMJIT_INLINE uint32_t x86ArgTypeToXmmType(uint32_t aType) {
  if (aType == kVarTypeFp32)
    return kVarTypeXmmSs;
  if (aType == kVarTypeFp64)
    return kVarTypeXmmSd;
  return aType;
}

//! Get an architecture from calling convention.
//!
//! Returns `kArchX86` or `kArchX64` depending on `conv`.
static ASMJIT_INLINE uint32_t x86GetArchFromCConv(uint32_t conv) {
  return IntUtil::inInterval<uint32_t>(conv, kFuncConvX64W, kFuncConvX64U) ? kArchX64 : kArchX86;
}

// ============================================================================
// [asmjit::X86X64FuncDecl - SetPrototype]
// ============================================================================

#define R(_Index_) kRegIndex##_Index_
static uint32_t X86X64FuncDecl_initConv(X86X64FuncDecl* self, uint32_t arch, uint32_t conv) {
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
    self->_preserved.set(kRegClassGp, IntUtil::mask(R(Bx), R(Sp), R(Bp), R(Si), R(Di)));

    switch (conv) {
      case kFuncConvCDecl:
        break;

      case kFuncConvStdCall:
        self->_calleePopsStack = true;
        break;

      case kFuncConvMsThisCall:
        self->_calleePopsStack = true;
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Cx)));
        self->_passedOrderGp[0] = R(Cx);
        break;

      case kFuncConvMsFastCall:
        self->_calleePopsStack = true;
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Cx), R(Cx)));
        self->_passedOrderGp[0] = R(Cx);
        self->_passedOrderGp[1] = R(Dx);
        break;

      case kFuncConvBorlandFastCall:
        self->_calleePopsStack = true;
        self->_direction = kFuncDirLtr;
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Ax), R(Dx), R(Cx)));
        self->_passedOrderGp[0] = R(Ax);
        self->_passedOrderGp[1] = R(Dx);
        self->_passedOrderGp[2] = R(Cx);
        break;

      case kFuncConvGccFastCall:
        self->_calleePopsStack = true;
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Cx), R(Dx)));
        self->_passedOrderGp[0] = R(Cx);
        self->_passedOrderGp[1] = R(Dx);
        break;

      case kFuncConvGccRegParm1:
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Ax)));
        self->_passedOrderGp[0] = R(Ax);
        break;

      case kFuncConvGccRegParm2:
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Ax), R(Dx)));
        self->_passedOrderGp[0] = R(Ax);
        self->_passedOrderGp[1] = R(Dx);
        break;

      case kFuncConvGccRegParm3:
        self->_passed.set(kRegClassGp, IntUtil::mask(R(Ax), R(Dx), R(Cx)));
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
    case kFuncConvX64W:
      self->_spillZoneSize = 32;

      self->_passed.set(kRegClassGp, IntUtil::mask(R(Cx), R(Dx), 8, 9));
      self->_passedOrderGp[0] = R(Cx);
      self->_passedOrderGp[1] = R(Dx);
      self->_passedOrderGp[2] = 8;
      self->_passedOrderGp[3] = 9;

      self->_passed.set(kRegClassXyz, IntUtil::mask(0, 1, 2, 3));
      self->_passedOrderXmm[0] = 0;
      self->_passedOrderXmm[1] = 1;
      self->_passedOrderXmm[2] = 2;
      self->_passedOrderXmm[3] = 3;

      self->_preserved.set(kRegClassGp , IntUtil::mask(R(Bx), R(Sp), R(Bp), R(Si), R(Di), 12, 13, 14, 15));
      self->_preserved.set(kRegClassXyz, IntUtil::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
      break;

    case kFuncConvX64U:
      self->_redZoneSize = 128;

      self->_passed.set(kRegClassGp, IntUtil::mask(R(Di), R(Si), R(Dx), R(Cx), 8, 9));
      self->_passedOrderGp[0] = R(Di);
      self->_passedOrderGp[1] = R(Si);
      self->_passedOrderGp[2] = R(Dx);
      self->_passedOrderGp[3] = R(Cx);
      self->_passedOrderGp[4] = 8;
      self->_passedOrderGp[5] = 9;

      self->_passed.set(kRegClassXyz, IntUtil::mask(0, 1, 2, 3, 4, 5, 6, 7));
      self->_passedOrderXmm[0] = 0;
      self->_passedOrderXmm[1] = 1;
      self->_passedOrderXmm[2] = 2;
      self->_passedOrderXmm[3] = 3;
      self->_passedOrderXmm[4] = 4;
      self->_passedOrderXmm[5] = 5;
      self->_passedOrderXmm[6] = 6;
      self->_passedOrderXmm[7] = 7;

      self->_preserved.set(kRegClassGp, IntUtil::mask(R(Bx), R(Sp), R(Bp), 12, 13, 14, 15));
      break;

    default:
      ASMJIT_ASSERT(!"Reached");
  }
#endif // ASMJIT_BUILD_X64

  return kErrorOk;
}
#undef R

static Error X86X64FuncDecl_initFunc(X86X64FuncDecl* self, uint32_t arch,
  uint32_t ret, const uint32_t* argList, uint32_t argCount) {

  ASMJIT_ASSERT(argCount <= kFuncArgCount);

  uint32_t conv = self->_convention;
  uint32_t regSize = (arch == kArchX86) ? 4 : 8;

  int32_t i = 0;
  int32_t gpPos = 0;
  int32_t xmmPos = 0;
  int32_t stackOffset = 0;

  const uint8_t* varMapping = nullptr;

#if defined(ASMJIT_BUILD_X86)
  if (arch == kArchX86)
    varMapping = x86::_varMapping;
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
  if (arch == kArchX64)
    varMapping = x64::_varMapping;
#endif // ASMJIT_BUILD_X64

  self->_argCount = static_cast<uint8_t>(argCount);
  self->_retCount = 0;

  for (i = 0; i < static_cast<int32_t>(argCount); i++) {
    FuncInOut& arg = self->getArg(i);
    arg._varType = static_cast<uint8_t>(argList[i]);
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

  if (ret != kVarTypeInvalid) {
    ret = varMapping[ret];
    switch (ret) {
      case kVarTypeInt64:
      case kVarTypeUInt64:
        // 64-bit value is returned in EDX:EAX on x86.
#if defined(ASMJIT_BUILD_X86)
        if (arch == kArchX86) {
          self->_retCount = 2;
          self->_retList[0]._varType = kVarTypeUInt32;
          self->_retList[0]._regIndex = kRegIndexAx;
          self->_retList[1]._varType = static_cast<uint8_t>(ret - 2);
          self->_retList[1]._regIndex = kRegIndexDx;
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
        self->_retList[0]._regIndex = kRegIndexAx;
        break;

      case kVarTypeMm:
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
          self->_retList[0]._varType = kVarTypeXmmSs;
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
          self->_retList[0]._varType = kVarTypeXmmSd;
          self->_retList[0]._regIndex = 0;
          break;
        }
        break;

      case kVarTypeXmm:
      case kVarTypeXmmSs:
      case kVarTypeXmmSd:
      case kVarTypeXmmPs:
      case kVarTypeXmmPd:
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
      self->_used.add(kRegClassGp, IntUtil::mask(arg.getRegIndex()));
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
        int32_t size = static_cast<int32_t>(_varInfo[varType].getSize());
        stackOffset -= size;
        arg._stackOffset = static_cast<int16_t>(stackOffset);
      }
    }
  }
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
  if (arch == kArchX64) {
    if (conv == kFuncConvX64W) {
      int32_t argMax = IntUtil::iMin<int32_t>(argCount, 4);

      // Register arguments (Gp/Xmm), always left-to-right.
      for (i = 0; i != argMax; i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (x86ArgIsInt(varType) && i < ASMJIT_ARRAY_SIZE(self->_passedOrderGp)) {
          arg._regIndex = self->_passedOrderGp[i];
          self->_used.add(kRegClassGp, IntUtil::mask(arg.getRegIndex()));
          continue;
        }

        if (x86ArgIsFp(varType) && i < ASMJIT_ARRAY_SIZE(self->_passedOrderXmm)) {
          arg._varType = static_cast<uint8_t>(x86ArgTypeToXmmType(varType));
          arg._regIndex = self->_passedOrderXmm[i];
          self->_used.add(kRegClassXyz, IntUtil::mask(arg.getRegIndex()));
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
        self->_used.add(kRegClassGp, IntUtil::mask(arg.getRegIndex()));
      }

      // Register arguments (Xmm), always left-to-right.
      for (i = 0; i != static_cast<int32_t>(argCount); i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = varMapping[arg.getVarType()];

        if (x86ArgIsFp(varType)) {
          arg._varType = static_cast<uint8_t>(x86ArgTypeToXmmType(varType));
          arg._regIndex = self->_passedOrderXmm[xmmPos++];
          self->_used.add(kRegClassXyz, IntUtil::mask(arg.getRegIndex()));
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
          int32_t size = static_cast<int32_t>(_varInfo[varType].getSize());

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

Error X86X64FuncDecl::setPrototype(uint32_t conv, const FuncPrototype& p) {
  if (conv == kFuncConvNone || conv >= _kFuncConvCount)
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

  ASMJIT_PROPAGATE_ERROR(X86X64FuncDecl_initConv(this, arch, conv));
  ASMJIT_PROPAGATE_ERROR(X86X64FuncDecl_initFunc(this, arch, p.getRet(), p.getArgList(), p.getArgCount()));

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86X64FuncDecl - Reset]
// ============================================================================

void X86X64FuncDecl::reset() {
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

} // x86x64 namespace
} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && (ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64)
