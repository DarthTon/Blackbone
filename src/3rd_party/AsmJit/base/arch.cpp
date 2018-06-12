// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/arch.h"

#if defined(ASMJIT_BUILD_X86)
#include "../x86/x86operand.h"
#endif // ASMJIT_BUILD_X86

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::ArchInfo]
// ============================================================================

static const uint32_t archInfoTable[] = {
  // <-------------+---------------------+-----------------------+-------+
  //               | Type                | SubType               | GPInfo|
  // <-------------+---------------------+-----------------------+-------+
  ASMJIT_PACK32_4x8(ArchInfo::kTypeNone  , ArchInfo::kSubTypeNone, 0,  0),
  ASMJIT_PACK32_4x8(ArchInfo::kTypeX86   , ArchInfo::kSubTypeNone, 4,  8),
  ASMJIT_PACK32_4x8(ArchInfo::kTypeX64   , ArchInfo::kSubTypeNone, 8, 16),
  ASMJIT_PACK32_4x8(ArchInfo::kTypeX32   , ArchInfo::kSubTypeNone, 8, 16),
  ASMJIT_PACK32_4x8(ArchInfo::kTypeA32   , ArchInfo::kSubTypeNone, 4, 16),
  ASMJIT_PACK32_4x8(ArchInfo::kTypeA64   , ArchInfo::kSubTypeNone, 8, 32)
};

ASMJIT_FAVOR_SIZE void ArchInfo::init(uint32_t type, uint32_t subType) noexcept {
  uint32_t index = type < ASMJIT_ARRAY_SIZE(archInfoTable) ? type : uint32_t(0);

  // Make sure the `archInfoTable` array is correctly indexed.
  _signature = archInfoTable[index];
  ASMJIT_ASSERT(_type == index);

  // Even if the architecture is not known we setup its type and sub-type,
  // however, such architecture is not really useful.
  _type = type;
  _subType = subType;
}

// ============================================================================
// [asmjit::ArchUtils]
// ============================================================================

ASMJIT_FAVOR_SIZE Error ArchUtils::typeIdToRegInfo(uint32_t archType, uint32_t& typeIdInOut, RegInfo& regInfo) noexcept {
  uint32_t typeId = typeIdInOut;

  // Zero the signature so it's clear in case that typeId is not invalid.
  regInfo._signature = 0;

#if defined(ASMJIT_BUILD_X86)
  if (ArchInfo::isX86Family(archType)) {
    // Passed RegType instead of TypeId?
    if (typeId <= Reg::kRegMax)
      typeId = x86OpData.archRegs.regTypeToTypeId[typeId];

    if (ASMJIT_UNLIKELY(!TypeId::isValid(typeId)))
      return DebugUtils::errored(kErrorInvalidTypeId);

    // First normalize architecture dependent types.
    if (TypeId::isAbstract(typeId)) {
      if (typeId == TypeId::kIntPtr)
        typeId = (archType == ArchInfo::kTypeX86) ? TypeId::kI32 : TypeId::kI64;
      else
        typeId = (archType == ArchInfo::kTypeX86) ? TypeId::kU32 : TypeId::kU64;
    }

    // Type size helps to construct all kinds of registers. If the size is zero
    // then the TypeId is invalid.
    uint32_t size = TypeId::sizeOf(typeId);
    if (ASMJIT_UNLIKELY(!size))
      return DebugUtils::errored(kErrorInvalidTypeId);

    if (ASMJIT_UNLIKELY(typeId == TypeId::kF80))
      return DebugUtils::errored(kErrorInvalidUseOfF80);

    uint32_t regType = 0;

    switch (typeId) {
      case TypeId::kI8:
      case TypeId::kU8:
        regType = X86Reg::kRegGpbLo;
        break;

      case TypeId::kI16:
      case TypeId::kU16:
        regType = X86Reg::kRegGpw;
        break;

      case TypeId::kI32:
      case TypeId::kU32:
        regType = X86Reg::kRegGpd;
        break;

      case TypeId::kI64:
      case TypeId::kU64:
        if (archType == ArchInfo::kTypeX86)
          return DebugUtils::errored(kErrorInvalidUseOfGpq);

        regType = X86Reg::kRegGpq;
        break;

      // F32 and F64 are always promoted to use vector registers.
      case TypeId::kF32:
        typeId = TypeId::kF32x1;
        regType = X86Reg::kRegXmm;
        break;

      case TypeId::kF64:
        typeId = TypeId::kF64x1;
        regType = X86Reg::kRegXmm;
        break;

      // Mask registers {k}.
      case TypeId::kMask8:
      case TypeId::kMask16:
      case TypeId::kMask32:
      case TypeId::kMask64:
        regType = X86Reg::kRegK;
        break;

      // MMX registers.
      case TypeId::kMmx32:
      case TypeId::kMmx64:
        regType = X86Reg::kRegMm;
        break;

      // XMM|YMM|ZMM registers.
      default:
        if (size <= 16)
          regType = X86Reg::kRegXmm;
        else if (size == 32)
          regType = X86Reg::kRegYmm;
        else
          regType = X86Reg::kRegZmm;
        break;
    }

    typeIdInOut = typeId;
    regInfo._signature = x86OpData.archRegs.regInfo[regType].getSignature();
    return kErrorOk;
  }
#endif // ASMJIT_BUILD_X86

  return DebugUtils::errored(kErrorInvalidArch);
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
