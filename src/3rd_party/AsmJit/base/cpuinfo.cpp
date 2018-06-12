// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/cpuinfo.h"
#include "../base/utils.h"

#if ASMJIT_OS_POSIX
# include <errno.h>
# include <sys/utsname.h>
# include <unistd.h>
#endif // ASMJIT_OS_POSIX

#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
# if ASMJIT_CC_MSC_GE(14, 0, 0)
 # include <intrin.h>         // Required by `__cpuid()` and `_xgetbv()`.
# endif // _MSC_VER >= 1400
#endif

#if ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64
# if ASMJIT_OS_LINUX
#  include <sys/auxv.h>       // Required by `getauxval()`.
# endif
#endif

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CpuInfo - Detect ARM]
// ============================================================================

// ARM information has to be retrieved by the OS (this is how ARM was designed).
#if ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64

#if ASMJIT_ARCH_ARM32
static ASMJIT_INLINE void armPopulateBaselineA32Features(CpuInfo* cpuInfo) noexcept {
  cpuInfo->_archInfo.init(ArchInfo::kTypeA32);
}
#endif // ASMJIT_ARCH_ARM32

#if ASMJIT_ARCH_ARM64
static ASMJIT_INLINE void armPopulateBaselineA64Features(CpuInfo* cpuInfo) noexcept {
  cpuInfo->_archInfo.init(ArchInfo::kTypeA64);

  // Thumb (including all variations) is supported on A64 (but not accessible from A64).
  cpuInfo->addFeature(CpuInfo::kArmFeatureTHUMB);
  cpuInfo->addFeature(CpuInfo::kArmFeatureTHUMB2);

  // A64 is based on ARMv8 and newer.
  cpuInfo->addFeature(CpuInfo::kArmFeatureV6);
  cpuInfo->addFeature(CpuInfo::kArmFeatureV7);
  cpuInfo->addFeature(CpuInfo::kArmFeatureV8);

  // A64 comes with these features by default.
  cpuInfo->addFeature(CpuInfo::kArmFeatureVFPv2);
  cpuInfo->addFeature(CpuInfo::kArmFeatureVFPv3);
  cpuInfo->addFeature(CpuInfo::kArmFeatureVFPv4);
  cpuInfo->addFeature(CpuInfo::kArmFeatureEDSP);
  cpuInfo->addFeature(CpuInfo::kArmFeatureASIMD);
  cpuInfo->addFeature(CpuInfo::kArmFeatureIDIVA);
  cpuInfo->addFeature(CpuInfo::kArmFeatureIDIVT);
}
#endif // ASMJIT_ARCH_ARM64

#if ASMJIT_OS_WINDOWS
//! \internal
//!
//! Detect ARM CPU features on Windows.
//!
//! The detection is based on `IsProcessorFeaturePresent()` API call.
static ASMJIT_INLINE void armDetectCpuInfoOnWindows(CpuInfo* cpuInfo) noexcept {
#if ASMJIT_ARCH_ARM32
  armPopulateBaselineA32Features(cpuInfo);

  // Windows for ARM requires at least ARMv7 with DSP extensions.
  cpuInfo->addFeature(CpuInfo::kArmFeatureV6);
  cpuInfo->addFeature(CpuInfo::kArmFeatureV7);
  cpuInfo->addFeature(CpuInfo::kArmFeatureEDSP);

  // Windows for ARM requires VFPv3.
  cpuInfo->addFeature(CpuInfo::kArmFeatureVFPv2);
  cpuInfo->addFeature(CpuInfo::kArmFeatureVFPv3);

  // Windows for ARM requires and uses THUMB2.
  cpuInfo->addFeature(CpuInfo::kArmFeatureTHUMB);
  cpuInfo->addFeature(CpuInfo::kArmFeatureTHUMB2);
#else
  armPopulateBaselineA64Features(cpuInfo);
#endif

  // Windows for ARM requires ASIMD.
  cpuInfo->addFeature(CpuInfo::kArmFeatureASIMD);

  // Detect additional CPU features by calling `IsProcessorFeaturePresent()`.
  struct WinPFPMapping {
    uint32_t pfpId;
    uint32_t featureId;
  };

  static const WinPFPMapping mapping[] = {
    { PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE , CpuInfo::kArmFeatureVFPv4     },
    { PF_ARM_VFP_32_REGISTERS_AVAILABLE  , CpuInfo::kArmFeatureVFP_D32   },
    { PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE, CpuInfo::kArmFeatureIDIVT     },
    { PF_ARM_64BIT_LOADSTORE_ATOMIC      , CpuInfo::kArmFeatureAtomics64 }
  };

  for (uint32_t i = 0; i < ASMJIT_ARRAY_SIZE(mapping); i++)
    if (::IsProcessorFeaturePresent(mapping[i].pfpId))
      cpuInfo->addFeature(mapping[i].featureId);
}
#endif // ASMJIT_OS_WINDOWS

#if ASMJIT_OS_LINUX
struct LinuxHWCapMapping {
  uint32_t hwcapMask;
  uint32_t featureId;
};

static void armDetectHWCaps(CpuInfo* cpuInfo, unsigned long type, const LinuxHWCapMapping* mapping, size_t length) noexcept {
  unsigned long mask = getauxval(type);

  for (size_t i = 0; i < length; i++)
    if ((mask & mapping[i].hwcapMask) == mapping[i].hwcapMask)
      cpuInfo->addFeature(mapping[i].featureId);
}

//! \internal
//!
//! Detect ARM CPU features on Linux.
//!
//! The detection is based on `getauxval()`.
ASMJIT_FAVOR_SIZE static void armDetectCpuInfoOnLinux(CpuInfo* cpuInfo) noexcept {
#if ASMJIT_ARCH_ARM32
  armPopulateBaselineA32Features(cpuInfo);

  // `AT_HWCAP` provides ARMv7 (and less) related flags.
  static const LinuxHWCapMapping hwCapMapping[] = {
    { /* HWCAP_VFP     */ (1 <<  6), CpuInfo::kArmFeatureVFPv2     },
    { /* HWCAP_EDSP    */ (1 <<  7), CpuInfo::kArmFeatureEDSP      },
    { /* HWCAP_NEON    */ (1 << 12), CpuInfo::kArmFeatureASIMD     },
    { /* HWCAP_VFPv3   */ (1 << 13), CpuInfo::kArmFeatureVFPv3     },
    { /* HWCAP_VFPv4   */ (1 << 16), CpuInfo::kArmFeatureVFPv4     },
    { /* HWCAP_IDIVA   */ (1 << 17), CpuInfo::kArmFeatureIDIVA     },
    { /* HWCAP_IDIVT   */ (1 << 18), CpuInfo::kArmFeatureIDIVT     },
    { /* HWCAP_VFPD32  */ (1 << 19), CpuInfo::kArmFeatureVFP_D32   }
  };
  armDetectHWCaps(cpuInfo, AT_HWCAP, hwCapMapping, ASMJIT_ARRAY_SIZE(hwCapMapping));

  // VFPv3 implies VFPv2.
  if (cpuInfo->hasFeature(CpuInfo::kArmFeatureVFPv3)) {
    cpuInfo->addFeature(CpuInfo::kArmFeatureVFPv2);
  }

  // VFPv2 implies ARMv6.
  if (cpuInfo->hasFeature(CpuInfo::kArmFeatureVFPv2)) {
    cpuInfo->addFeature(CpuInfo::kArmFeatureV6);
  }

  // VFPv3 or ASIMD implies ARMv7.
  if (cpuInfo->hasFeature(CpuInfo::kArmFeatureVFPv3) ||
      cpuInfo->hasFeature(CpuInfo::kArmFeatureASIMD)) {
    cpuInfo->addFeature(CpuInfo::kArmFeatureV7);
  }

  // `AT_HWCAP2` provides ARMv8+ related flags.
  static const LinuxHWCapMapping hwCap2Mapping[] = {
    { /* HWCAP2_AES    */ (1 <<  0), CpuInfo::kArmFeatureAES       },
    { /* HWCAP2_PMULL  */ (1 <<  1), CpuInfo::kArmFeaturePMULL     },
    { /* HWCAP2_SHA1   */ (1 <<  2), CpuInfo::kArmFeatureSHA1      },
    { /* HWCAP2_SHA2   */ (1 <<  3), CpuInfo::kArmFeatureSHA256    },
    { /* HWCAP2_CRC32  */ (1 <<  4), CpuInfo::kArmFeatureCRC32     }
  };
  armDetectHWCaps(cpuInfo, AT_HWCAP2, hwCap2Mapping, ASMJIT_ARRAY_SIZE(hwCap2Mapping));

  if (cpuInfo->hasFeature(CpuInfo::kArmFeatureAES   ) ||
      cpuInfo->hasFeature(CpuInfo::kArmFeatureCRC32 ) ||
      cpuInfo->hasFeature(CpuInfo::kArmFeaturePMULL ) ||
      cpuInfo->hasFeature(CpuInfo::kArmFeatureSHA1  ) ||
      cpuInfo->hasFeature(CpuInfo::kArmFeatureSHA256)) {
    cpuInfo->addFeature(CpuInfo::kArmFeatureV8);
  }
#else
  armPopulateBaselineA64Features(cpuInfo);

  // `AT_HWCAP` provides ARMv8+ related flags.
  static const LinuxHWCapMapping hwCapMapping[] = {
    { /* HWCAP_ASIMD   */ (1 <<  1), CpuInfo::kArmFeatureASIMD     },
    { /* HWCAP_AES     */ (1 <<  3), CpuInfo::kArmFeatureAES       },
    { /* HWCAP_CRC32   */ (1 <<  7), CpuInfo::kArmFeatureCRC32     },
    { /* HWCAP_PMULL   */ (1 <<  4), CpuInfo::kArmFeaturePMULL     },
    { /* HWCAP_SHA1    */ (1 <<  5), CpuInfo::kArmFeatureSHA1      },
    { /* HWCAP_SHA2    */ (1 <<  6), CpuInfo::kArmFeatureSHA256    },
    { /* HWCAP_ATOMICS */ (1 <<  8), CpuInfo::kArmFeatureAtomics64 }
  };
  armDetectHWCaps(cpuInfo, AT_HWCAP, hwCapMapping, ASMJIT_ARRAY_SIZE(hwCapMapping));

  // `AT_HWCAP2` is not used at the moment.
#endif
}
#endif // ASMJIT_OS_LINUX

ASMJIT_FAVOR_SIZE static void armDetectCpuInfo(CpuInfo* cpuInfo) noexcept {
#if ASMJIT_OS_WINDOWS
  armDetectCpuInfoOnWindows(cpuInfo);
#elif ASMJIT_OS_LINUX
  armDetectCpuInfoOnLinux(cpuInfo);
#else
# error "[asmjit] armDetectCpuInfo() - Unsupported OS."
#endif
}
#endif // ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64

// ============================================================================
// [asmjit::CpuInfo - Detect X86]
// ============================================================================

#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64

//! \internal
//!
//! X86 CPUID result.
struct CpuIdResult {
  uint32_t eax, ebx, ecx, edx;
};

//! \internal
//!
//! Content of XCR register, result of XGETBV instruction.
struct XGetBVResult {
  uint32_t eax, edx;
};

#if ASMJIT_CC_MSC && !ASMJIT_CC_MSC_GE(15, 0, 30729) && ASMJIT_ARCH_X64
//! \internal
//!
//! HACK: VS2008 or less, 64-bit mode - `__cpuidex` doesn't exist! However,
//! 64-bit calling convention specifies the first parameter to be passed by
//! ECX, so we may be lucky if compiler doesn't move the register, otherwise
//! the result would be wrong.
static void ASMJIT_NOINLINE void x86CallCpuIdWorkaround(uint32_t inEcx, uint32_t inEax, CpuIdResult* result) noexcept {
  __cpuid(reinterpret_cast<int*>(result), inEax);
}
#endif

//! \internal
//!
//! Wrapper to call `cpuid` instruction.
static void ASMJIT_INLINE x86CallCpuId(CpuIdResult* result, uint32_t inEax, uint32_t inEcx = 0) noexcept {
#if ASMJIT_CC_MSC && ASMJIT_CC_MSC_GE(15, 0, 30729)
  __cpuidex(reinterpret_cast<int*>(result), inEax, inEcx);
#elif ASMJIT_CC_MSC && ASMJIT_ARCH_X64
  x86CallCpuIdWorkaround(inEcx, inEax, result);
#elif ASMJIT_CC_MSC && ASMJIT_ARCH_X86
  uint32_t paramEax = inEax;
  uint32_t paramEcx = inEcx;
  uint32_t* out = reinterpret_cast<uint32_t*>(result);

  __asm {
    mov     eax, paramEax
    mov     ecx, paramEcx
    mov     edi, out
    cpuid
    mov     dword ptr[edi +  0], eax
    mov     dword ptr[edi +  4], ebx
    mov     dword ptr[edi +  8], ecx
    mov     dword ptr[edi + 12], edx
  }
#elif (ASMJIT_CC_GCC || ASMJIT_CC_CLANG) && ASMJIT_ARCH_X86
  __asm__ __volatile__(
    "mov %%ebx, %%edi\n"
    "cpuid\n"
    "xchg %%edi, %%ebx\n"
      : "=a"(result->eax),
        "=D"(result->ebx),
        "=c"(result->ecx),
        "=d"(result->edx)
      : "a"(inEax),
        "c"(inEcx));
#elif (ASMJIT_CC_GCC || ASMJIT_CC_CLANG || ASMJIT_CC_INTEL) && ASMJIT_ARCH_X64
  __asm__ __volatile__(
    "mov %%rbx, %%rdi\n"
    "cpuid\n"
    "xchg %%rdi, %%rbx\n"
      : "=a"(result->eax),
        "=D"(result->ebx),
        "=c"(result->ecx),
        "=d"(result->edx)
      : "a"(inEax),
        "c"(inEcx));
#else
# error "[asmjit] x86CallCpuid() - Unsupported compiler."
#endif
}

//! \internal
//!
//! Wrapper to call `xgetbv` instruction.
static ASMJIT_INLINE void x86CallXGetBV(XGetBVResult* result, uint32_t inEcx) noexcept {
#if ASMJIT_CC_MSC_GE(16, 0, 40219) // 2010SP1+
  uint64_t value = _xgetbv(inEcx);
  result->eax = static_cast<uint32_t>(value & 0xFFFFFFFFU);
  result->edx = static_cast<uint32_t>(value >> 32);
#elif ASMJIT_CC_GCC || ASMJIT_CC_CLANG
  uint32_t outEax;
  uint32_t outEdx;

  // Replaced, because the world is not perfect:
  //   __asm__ __volatile__("xgetbv" : "=a"(outEax), "=d"(outEdx) : "c"(inEcx));
  __asm__ __volatile__(".byte 0x0F, 0x01, 0xd0" : "=a"(outEax), "=d"(outEdx) : "c"(inEcx));

  result->eax = outEax;
  result->edx = outEdx;
#else
  result->eax = 0;
  result->edx = 0;
#endif
}

//! \internal
//!
//! Map a 12-byte vendor string returned by `cpuid` into a `CpuInfo::Vendor` ID.
static ASMJIT_INLINE uint32_t x86GetCpuVendorID(const char* vendorString) noexcept {
  struct VendorData {
    uint32_t id;
    char text[12];
  };

  static const VendorData vendorList[] = {
    { CpuInfo::kVendorIntel , { 'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l' } },
    { CpuInfo::kVendorAMD   , { 'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D' } },
    { CpuInfo::kVendorVIA   , { 'V', 'I', 'A',  0 , 'V', 'I', 'A',  0 , 'V', 'I', 'A',  0  } },
    { CpuInfo::kVendorVIA   , { 'C', 'e', 'n', 't', 'a', 'u', 'r', 'H', 'a', 'u', 'l', 's' } }
  };

  uint32_t dw0 = reinterpret_cast<const uint32_t*>(vendorString)[0];
  uint32_t dw1 = reinterpret_cast<const uint32_t*>(vendorString)[1];
  uint32_t dw2 = reinterpret_cast<const uint32_t*>(vendorString)[2];

  for (uint32_t i = 0; i < ASMJIT_ARRAY_SIZE(vendorList); i++) {
    if (dw0 == reinterpret_cast<const uint32_t*>(vendorList[i].text)[0] &&
        dw1 == reinterpret_cast<const uint32_t*>(vendorList[i].text)[1] &&
        dw2 == reinterpret_cast<const uint32_t*>(vendorList[i].text)[2])
      return vendorList[i].id;
  }

  return CpuInfo::kVendorNone;
}

static ASMJIT_INLINE void x86SimplifyBrandString(char* s) noexcept {
  // Used to always clear the current character to ensure that the result
  // doesn't contain garbage after the new zero terminator.
  char* d = s;

  char prev = 0;
  char curr = s[0];
  s[0] = '\0';

  for (;;) {
    if (curr == 0)
      break;

    if (curr == ' ') {
      if (prev == '@' || s[1] == ' ' || s[1] == '@')
        goto L_Skip;
    }

    d[0] = curr;
    d++;
    prev = curr;

L_Skip:
    curr = *++s;
    s[0] = '\0';
  }

  d[0] = '\0';
}

ASMJIT_FAVOR_SIZE static void x86DetectCpuInfo(CpuInfo* cpuInfo) noexcept {
  uint32_t i, maxId;

  CpuIdResult regs;
  XGetBVResult xcr0 = { 0, 0 };

  cpuInfo->_archInfo.init(ArchInfo::kTypeHost);
  cpuInfo->addFeature(CpuInfo::kX86FeatureI486);

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x0]
  // --------------------------------------------------------------------------

  // Get vendor string/id.
  x86CallCpuId(&regs, 0x0);

  maxId = regs.eax;
  ::memcpy(cpuInfo->_vendorString + 0, &regs.ebx, 4);
  ::memcpy(cpuInfo->_vendorString + 4, &regs.edx, 4);
  ::memcpy(cpuInfo->_vendorString + 8, &regs.ecx, 4);
  cpuInfo->_vendorId = x86GetCpuVendorID(cpuInfo->_vendorString);

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x1]
  // --------------------------------------------------------------------------

  if (maxId >= 0x1) {
    // Get feature flags in ECX/EDX and family/model in EAX.
    x86CallCpuId(&regs, 0x1);

    // Fill family and model fields.
    cpuInfo->_family   = (regs.eax >> 8) & 0x0F;
    cpuInfo->_model    = (regs.eax >> 4) & 0x0F;
    cpuInfo->_stepping = (regs.eax     ) & 0x0F;

    // Use extended family and model fields.
    if (cpuInfo->_family == 0x0F) {
      cpuInfo->_family += ((regs.eax >> 20) & 0xFF);
      cpuInfo->_model  += ((regs.eax >> 16) & 0x0F) << 4;
    }

    cpuInfo->_x86Data._processorType        = ((regs.eax >> 12) & 0x03);
    cpuInfo->_x86Data._brandIndex           = ((regs.ebx      ) & 0xFF);
    cpuInfo->_x86Data._flushCacheLineSize   = ((regs.ebx >>  8) & 0xFF) * 8;
    cpuInfo->_x86Data._maxLogicalProcessors = ((regs.ebx >> 16) & 0xFF);

    if (regs.ecx & 0x00000001U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSE3);
    if (regs.ecx & 0x00000002U) cpuInfo->addFeature(CpuInfo::kX86FeaturePCLMULQDQ);
    if (regs.ecx & 0x00000008U) cpuInfo->addFeature(CpuInfo::kX86FeatureMONITOR);
    if (regs.ecx & 0x00000200U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSSE3);
    if (regs.ecx & 0x00002000U) cpuInfo->addFeature(CpuInfo::kX86FeatureCMPXCHG16B);
    if (regs.ecx & 0x00080000U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSE4_1);
    if (regs.ecx & 0x00100000U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSE4_2);
    if (regs.ecx & 0x00400000U) cpuInfo->addFeature(CpuInfo::kX86FeatureMOVBE);
    if (regs.ecx & 0x00800000U) cpuInfo->addFeature(CpuInfo::kX86FeaturePOPCNT);
    if (regs.ecx & 0x02000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAESNI);
    if (regs.ecx & 0x04000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureXSAVE);
    if (regs.ecx & 0x08000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureOSXSAVE);
    if (regs.ecx & 0x40000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureRDRAND);
    if (regs.edx & 0x00000010U) cpuInfo->addFeature(CpuInfo::kX86FeatureRDTSC);
    if (regs.edx & 0x00000020U) cpuInfo->addFeature(CpuInfo::kX86FeatureMSR);
    if (regs.edx & 0x00000100U) cpuInfo->addFeature(CpuInfo::kX86FeatureCMPXCHG8B);
    if (regs.edx & 0x00008000U) cpuInfo->addFeature(CpuInfo::kX86FeatureCMOV);
    if (regs.edx & 0x00080000U) cpuInfo->addFeature(CpuInfo::kX86FeatureCLFLUSH);
    if (regs.edx & 0x00800000U) cpuInfo->addFeature(CpuInfo::kX86FeatureMMX);
    if (regs.edx & 0x01000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureFXSR);
    if (regs.edx & 0x02000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSE)
                                        .addFeature(CpuInfo::kX86FeatureMMX2);
    if (regs.edx & 0x04000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSE)
                                        .addFeature(CpuInfo::kX86FeatureSSE2);
    if (regs.edx & 0x10000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureMT);

    // Get the content of XCR0 if supported by CPU and enabled by OS.
    if ((regs.ecx & 0x0C000000U) == 0x0C000000U) {
      x86CallXGetBV(&xcr0, 0);
    }

    // Detect AVX+.
    if (regs.ecx & 0x10000000U) {
      // - XCR0[2:1] == 11b
      //   XMM & YMM states need to be enabled by OS.
      if ((xcr0.eax & 0x00000006U) == 0x00000006U) {
        cpuInfo->addFeature(CpuInfo::kX86FeatureAVX);

        if (regs.ecx & 0x00001000U) cpuInfo->addFeature(CpuInfo::kX86FeatureFMA);
        if (regs.ecx & 0x20000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureF16C);
      }
    }
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x7]
  // --------------------------------------------------------------------------

  // Detect new features if the processor supports CPUID-07.
  bool maybeMPX = false;

  if (maxId >= 0x7) {
    x86CallCpuId(&regs, 0x7);

    if (regs.ebx & 0x00000001U) cpuInfo->addFeature(CpuInfo::kX86FeatureFSGSBASE);
    if (regs.ebx & 0x00000008U) cpuInfo->addFeature(CpuInfo::kX86FeatureBMI);
    if (regs.ebx & 0x00000010U) cpuInfo->addFeature(CpuInfo::kX86FeatureHLE);
    if (regs.ebx & 0x00000080U) cpuInfo->addFeature(CpuInfo::kX86FeatureSMEP);
    if (regs.ebx & 0x00000100U) cpuInfo->addFeature(CpuInfo::kX86FeatureBMI2);
    if (regs.ebx & 0x00000200U) cpuInfo->addFeature(CpuInfo::kX86FeatureERMS);
    if (regs.ebx & 0x00000800U) cpuInfo->addFeature(CpuInfo::kX86FeatureRTM);
    if (regs.ebx & 0x00004000U) maybeMPX = true;
    if (regs.ebx & 0x00040000U) cpuInfo->addFeature(CpuInfo::kX86FeatureRDSEED);
    if (regs.ebx & 0x00080000U) cpuInfo->addFeature(CpuInfo::kX86FeatureADX);
    if (regs.ebx & 0x00100000U) cpuInfo->addFeature(CpuInfo::kX86FeatureSMAP);
    if (regs.ebx & 0x00400000U) cpuInfo->addFeature(CpuInfo::kX86FeaturePCOMMIT);
    if (regs.ebx & 0x00800000U) cpuInfo->addFeature(CpuInfo::kX86FeatureCLFLUSHOPT);
    if (regs.ebx & 0x01000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureCLWB);
    if (regs.ebx & 0x20000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureSHA);
    if (regs.ecx & 0x00000001U) cpuInfo->addFeature(CpuInfo::kX86FeaturePREFETCHWT1);

    // TSX is supported if at least one of `HLE` and `RTM` is supported.
    if (regs.ebx & 0x00000810U) cpuInfo->addFeature(CpuInfo::kX86FeatureTSX);

    // Detect AVX2.
    if (cpuInfo->hasFeature(CpuInfo::kX86FeatureAVX)) {
      if (regs.ebx & 0x00000020U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX2);
    }

    // Detect AVX-512+.
    if (regs.ebx & 0x00010000U) {
      // - XCR0[2:1] == 11b
      //   XMM/YMM states need to be enabled by OS.
      // - XCR0[7:5] == 111b
      //   Upper 256-bit of ZMM0-XMM15 and ZMM16-ZMM31 need to be enabled by the OS.
      if ((xcr0.eax & 0x000000E6U) == 0x000000E6U) {
        cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_F);

        if (regs.ebx & 0x00020000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_DQ);
        if (regs.ebx & 0x00200000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_IFMA);
        if (regs.ebx & 0x04000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_PFI);
        if (regs.ebx & 0x08000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_ERI);
        if (regs.ebx & 0x10000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_CDI);
        if (regs.ebx & 0x40000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_BW);
        if (regs.ebx & 0x80000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_VL);
        if (regs.ecx & 0x00000002U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_VBMI);
        if (regs.ecx & 0x00004000U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_VPOPCNTDQ);
        if (regs.edx & 0x00000004U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_4VNNIW);
        if (regs.edx & 0x00000008U) cpuInfo->addFeature(CpuInfo::kX86FeatureAVX512_4FMAPS);
      }
    }
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0xD]
  // --------------------------------------------------------------------------

  if (maxId >= 0xD) {
    x86CallCpuId(&regs, 0xD, 0);

    // Both CPUID result and XCR0 has to be enabled to have support for MPX.
    if (((regs.eax & xcr0.eax) & 0x00000018U) == 0x00000018U && maybeMPX)
      cpuInfo->addFeature(CpuInfo::kX86FeatureMPX);

    x86CallCpuId(&regs, 0xD, 1);
    if (regs.eax & 0x00000001U) cpuInfo->addFeature(CpuInfo::kX86FeatureXSAVEOPT);
    if (regs.eax & 0x00000002U) cpuInfo->addFeature(CpuInfo::kX86FeatureXSAVEC);
    if (regs.eax & 0x00000008U) cpuInfo->addFeature(CpuInfo::kX86FeatureXSAVES);
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x80000000...maxId]
  // --------------------------------------------------------------------------

  // The highest EAX that we understand.
  uint32_t kHighestProcessedEAX = 0x80000008U;

  // Several CPUID calls are required to get the whole branc string. It's easy
  // to copy one DWORD at a time instead of performing a byte copy.
  uint32_t* brand = reinterpret_cast<uint32_t*>(cpuInfo->_brandString);

  i = maxId = 0x80000000U;
  do {
    x86CallCpuId(&regs, i);
    switch (i) {
      case 0x80000000U:
        maxId = std::min<uint32_t>(regs.eax, kHighestProcessedEAX);
        break;

      case 0x80000001U:
        if (regs.ecx & 0x00000001U) cpuInfo->addFeature(CpuInfo::kX86FeatureLAHFSAHF);
        if (regs.ecx & 0x00000020U) cpuInfo->addFeature(CpuInfo::kX86FeatureLZCNT);
        if (regs.ecx & 0x00000040U) cpuInfo->addFeature(CpuInfo::kX86FeatureSSE4A);
        if (regs.ecx & 0x00000080U) cpuInfo->addFeature(CpuInfo::kX86FeatureMSSE);
        if (regs.ecx & 0x00000100U) cpuInfo->addFeature(CpuInfo::kX86FeaturePREFETCHW);
        if (regs.ecx & 0x00200000U) cpuInfo->addFeature(CpuInfo::kX86FeatureTBM);
        if (regs.edx & 0x00100000U) cpuInfo->addFeature(CpuInfo::kX86FeatureNX);
        if (regs.edx & 0x00200000U) cpuInfo->addFeature(CpuInfo::kX86FeatureFXSROPT);
        if (regs.edx & 0x00400000U) cpuInfo->addFeature(CpuInfo::kX86FeatureMMX2);
        if (regs.edx & 0x08000000U) cpuInfo->addFeature(CpuInfo::kX86FeatureRDTSCP);
        if (regs.edx & 0x40000000U) cpuInfo->addFeature(CpuInfo::kX86Feature3DNOW2)
                                            .addFeature(CpuInfo::kX86FeatureMMX2);
        if (regs.edx & 0x80000000U) cpuInfo->addFeature(CpuInfo::kX86Feature3DNOW);

        if (cpuInfo->hasFeature(CpuInfo::kX86FeatureAVX)) {
          if (regs.ecx & 0x00000800U) cpuInfo->addFeature(CpuInfo::kX86FeatureXOP);
          if (regs.ecx & 0x00010000U) cpuInfo->addFeature(CpuInfo::kX86FeatureFMA4);
        }

        // These seem to be only supported by AMD.
        if (cpuInfo->getVendorId() == CpuInfo::kVendorAMD) {
          if (regs.ecx & 0x00000010U) cpuInfo->addFeature(CpuInfo::kX86FeatureALTMOVCR8);
        }
        break;

      case 0x80000002U:
      case 0x80000003U:
      case 0x80000004U:
        *brand++ = regs.eax;
        *brand++ = regs.ebx;
        *brand++ = regs.ecx;
        *brand++ = regs.edx;

        // Go directly to the last one.
        if (i == 0x80000004U) i = 0x80000008U - 1;
        break;

      case 0x80000008U:
        if (regs.ebx & 0x00000001U) cpuInfo->addFeature(CpuInfo::kX86FeatureCLZERO);
        break;
    }
  } while (++i <= maxId);

  // Simplify CPU brand string by removing unnecessary spaces.
  x86SimplifyBrandString(cpuInfo->_brandString);
}
#endif // ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64

// ============================================================================
// [asmjit::CpuInfo - Detect - HWThreadsCount]
// ============================================================================

static ASMJIT_INLINE uint32_t cpuDetectHWThreadsCount() noexcept {
#if ASMJIT_OS_WINDOWS
  SYSTEM_INFO info;
  ::GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
#elif ASMJIT_OS_POSIX && defined(_SC_NPROCESSORS_ONLN)
  long res = ::sysconf(_SC_NPROCESSORS_ONLN);
  if (res <= 0) return 1;
  return static_cast<uint32_t>(res);
#else
  return 1;
#endif
}

// ============================================================================
// [asmjit::CpuInfo - Detect]
// ============================================================================

ASMJIT_FAVOR_SIZE void CpuInfo::detect() noexcept {
  reset();

#if ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64
  armDetectCpuInfo(this);
#endif // ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64

#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
  x86DetectCpuInfo(this);
#endif // ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64

  _hwThreadsCount = cpuDetectHWThreadsCount();
}

// ============================================================================
// [asmjit::CpuInfo - GetHost]
// ============================================================================

struct HostCpuInfo : public CpuInfo {
  ASMJIT_INLINE HostCpuInfo() noexcept : CpuInfo() { detect(); }
};

const CpuInfo& CpuInfo::getHost() noexcept {
  static HostCpuInfo host;
  return host;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
