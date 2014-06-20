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
#include "../base/intutil.h"
#include "../x86/x86cpuinfo.h"

// 2009-02-05: Thanks to Mike Tajmajer for VC7.1 compiler support. It shouldn't
// affect x64 compilation, because x64 compiler starts with VS2005 (VC8.0).
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#include <intrin.h>
#endif // _MSC_VER >= 1400

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [asmjit::x86x64::CpuVendor]
// ============================================================================

struct CpuVendor {
  uint32_t id;
  char text[12];
};

static const CpuVendor cpuVendorTable[] = {
  { kCpuVendorIntel    , { 'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l' } },
  { kCpuVendorAmd      , { 'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D' } },
  { kCpuVendorAmd      , { 'A', 'M', 'D', 'i', 's', 'b', 'e', 't', 't', 'e', 'r', '!' } },
  { kCpuVendorVia      , { 'V', 'I', 'A',  0 , 'V', 'I', 'A',  0 , 'V', 'I', 'A',  0  } },
  { kCpuVendorVia      , { 'C', 'e', 'n', 't', 'a', 'u', 'r', 'H', 'a', 'u', 'l', 's' } }
};

static ASMJIT_INLINE bool cpuVendorEq(const CpuVendor& info, const char* vendorString) {
  const uint32_t* a = reinterpret_cast<const uint32_t*>(info.text);
  const uint32_t* b = reinterpret_cast<const uint32_t*>(vendorString);

  return (a[0] == b[0]) & (a[1] == b[1]) & (a[2] == b[2]);
}

static ASMJIT_INLINE void simplifyBrandString(char* s) {
  // Always clear the current character in the buffer. It ensures that there
  // is no garbage after the string NULL terminator.
  char* d = s;

  char prev = 0;
  char curr = s[0];
  s[0] = '\0';

  for (;;) {
    if (curr == 0)
      break;

    if (curr == ' ') {
      if (prev == '@' || s[1] == ' ' || s[1] == '@')
        goto _Skip;
    }

    d[0] = curr;
    d++;
    prev = curr;

_Skip:
    curr = *++s;
    s[0] = '\0';
  }

  d[0] = '\0';
}

// ============================================================================
// [asmjit::x86x64::CpuUtil]
// ============================================================================

// This is messy, I know. Cpuid is implemented as intrinsic in VS2005, but
// we should support other compilers as well. Main problem is that MS compilers
// in 64-bit mode not allows to use inline assembler, so we need intrinsic and
// we need also asm version.

// callCpuId() and detectCpuInfo() for x86 and x64 platforms begins here.
#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
void CpuUtil::callCpuId(uint32_t inEax, uint32_t inEcx, CpuId* outResult) {

#if defined(_MSC_VER)
// 2009-02-05: Thanks to Mike Tajmajer for supporting VC7.1 compiler.
// ASMJIT_HOST_X64 is here only for readibility, only VS2005 can compile 64-bit code.
# if _MSC_VER >= 1400 || defined(ASMJIT_HOST_X64)
  // Done by intrinsics.
  __cpuidex(reinterpret_cast<int*>(outResult->i), inEax, inEcx);
# else // _MSC_VER < 1400
  uint32_t cpuid_eax = inEax;
  uint32_t cpuid_ecx = inCax;
  uint32_t* cpuid_out = outResult->i;

  __asm {
    mov     eax, cpuid_eax
    mov     ecx, cpuid_ecx
    mov     edi, cpuid_out
    cpuid
    mov     dword ptr[edi +  0], eax
    mov     dword ptr[edi +  4], ebx
    mov     dword ptr[edi +  8], ecx
    mov     dword ptr[edi + 12], edx
  }
# endif // _MSC_VER < 1400

#elif defined(__GNUC__)
// Note, patched to preserve ebx/rbx register which is used by GCC.
# if defined(ASMJIT_HOST_X86)
#  define __myCpuId(inEax, inEcx, outEax, outEbx, outEcx, outEdx) \
  asm ("mov %%ebx, %%edi\n"  \
       "cpuid\n"             \
       "xchg %%edi, %%ebx\n" \
       : "=a" (outEax), "=D" (outEbx), "=c" (outEcx), "=d" (outEdx) : "a" (inEax), "c" (inEcx))
# else
#  define __myCpuId(inEax, inEcx, outEax, outEbx, outEcx, outEdx) \
  asm ("mov %%rbx, %%rdi\n"  \
       "cpuid\n"             \
       "xchg %%rdi, %%rbx\n" \
       : "=a" (outEax), "=D" (outEbx), "=c" (outEcx), "=d" (outEdx) : "a" (inEax), "c" (inEcx))
# endif
  __myCpuId(inEax, inEcx, outResult->eax, outResult->ebx, outResult->ecx, outResult->edx);
#endif // COMPILER
}

void CpuUtil::detect(CpuInfo* cpuInfo) {
  CpuId regs;

  uint32_t i;
  uint32_t maxId;

  // Clear everything except the '_size' member.
  ::memset(reinterpret_cast<uint8_t*>(cpuInfo) + sizeof(uint32_t),
    0, sizeof(BaseCpuInfo) - sizeof(uint32_t));

  // Fill safe defaults.
  ::memcpy(cpuInfo->_vendorString, "Unknown", 8);
  cpuInfo->_coresCount = BaseCpuInfo::detectNumberOfCores();

  // Get vendor string/id.
  callCpuId(0, 0, &regs);

  maxId = regs.eax;
  ::memcpy(cpuInfo->_vendorString, &regs.ebx, 4);
  ::memcpy(cpuInfo->_vendorString + 4, &regs.edx, 4);
  ::memcpy(cpuInfo->_vendorString + 8, &regs.ecx, 4);

  for (i = 0; i < ASMJIT_ARRAY_SIZE(cpuVendorTable); i++) {
    if (cpuVendorEq(cpuVendorTable[i], cpuInfo->_vendorString)) {
      cpuInfo->_vendorId = cpuVendorTable[i].id;
      break;
    }
  }

  // Get feature flags in ecx/edx and family/model in eax.
  callCpuId(1, 0, &regs);

  // Fill family and model fields.
  cpuInfo->_family   = (regs.eax >> 8) & 0x0F;
  cpuInfo->_model    = (regs.eax >> 4) & 0x0F;
  cpuInfo->_stepping = (regs.eax     ) & 0x0F;

  // Use extended family and model fields.
  if (cpuInfo->_family == 0x0F) {
    cpuInfo->_family += ((regs.eax >> 20) & 0xFF);
    cpuInfo->_model  += ((regs.eax >> 16) & 0x0F) << 4;
  }

  cpuInfo->_processorType        = ((regs.eax >> 12) & 0x03);
  cpuInfo->_brandIndex           = ((regs.ebx      ) & 0xFF);
  cpuInfo->_flushCacheLineSize   = ((regs.ebx >>  8) & 0xFF) * 8;
  cpuInfo->_maxLogicalProcessors = ((regs.ebx >> 16) & 0xFF);

  if (regs.ecx & 0x00000001U) cpuInfo->addFeature(kCpuFeatureSse3);
  if (regs.ecx & 0x00000002U) cpuInfo->addFeature(kCpuFeaturePclmulqdq);
  if (regs.ecx & 0x00000008U) cpuInfo->addFeature(kCpuFeatureMonitorMWait);
  if (regs.ecx & 0x00000200U) cpuInfo->addFeature(kCpuFeatureSsse3);
  if (regs.ecx & 0x00002000U) cpuInfo->addFeature(kCpuFeatureCmpXchg16B);
  if (regs.ecx & 0x00080000U) cpuInfo->addFeature(kCpuFeatureSse41);
  if (regs.ecx & 0x00100000U) cpuInfo->addFeature(kCpuFeatureSse42);
  if (regs.ecx & 0x00400000U) cpuInfo->addFeature(kCpuFeatureMovbe);
  if (regs.ecx & 0x00800000U) cpuInfo->addFeature(kCpuFeaturePopcnt);
  if (regs.ecx & 0x02000000U) cpuInfo->addFeature(kCpuFeatureAesni);
  if (regs.ecx & 0x40000000U) cpuInfo->addFeature(kCpuFeatureRdrand);

  if (regs.edx & 0x00000010U) cpuInfo->addFeature(kCpuFeatureRdtsc);
  if (regs.edx & 0x00000100U) cpuInfo->addFeature(kCpuFeatureCmpXchg8B);
  if (regs.edx & 0x00008000U) cpuInfo->addFeature(kCpuFeatureCmov);
  if (regs.edx & 0x00800000U) cpuInfo->addFeature(kCpuFeatureMmx);
  if (regs.edx & 0x01000000U) cpuInfo->addFeature(kCpuFeatureFxsr);
  if (regs.edx & 0x02000000U) cpuInfo->addFeature(kCpuFeatureSse).addFeature(kCpuFeatureMmxExt);
  if (regs.edx & 0x04000000U) cpuInfo->addFeature(kCpuFeatureSse).addFeature(kCpuFeatureSse2);
  if (regs.edx & 0x10000000U) cpuInfo->addFeature(kCpuFeatureMultithreading);

  if (cpuInfo->_vendorId == kCpuVendorAmd && (regs.edx & 0x10000000U)) {
    // AMD sets Multithreading to ON if it has more cores.
    if (cpuInfo->_coresCount == 1)
      cpuInfo->_coresCount = 2;
  }

  // Detect AVX.
  if (regs.ecx & 0x10000000U) {
    cpuInfo->addFeature(kCpuFeatureAvx);

    if (regs.ecx & 0x00000800U) cpuInfo->addFeature(kCpuFeatureXop);
    if (regs.ecx & 0x00004000U) cpuInfo->addFeature(kCpuFeatureFma3);
    if (regs.ecx & 0x00010000U) cpuInfo->addFeature(kCpuFeatureFma4);
    if (regs.ecx & 0x20000000U) cpuInfo->addFeature(kCpuFeatureF16C);
  }

  // Detect new features if the processor supports CPUID-07.
  if (maxId >= 7) {
    callCpuId(7, 0, &regs);

    if (regs.ebx & 0x00000001) cpuInfo->addFeature(kCpuFeatureFsGsBase);
    if (regs.ebx & 0x00000008) cpuInfo->addFeature(kCpuFeatureBmi);
    if (regs.ebx & 0x00000010) cpuInfo->addFeature(kCpuFeatureHle);
    if (regs.ebx & 0x00000100) cpuInfo->addFeature(kCpuFeatureBmi2);
    if (regs.ebx & 0x00000200) cpuInfo->addFeature(kCpuFeatureRepMovsbStosbExt);
    if (regs.ebx & 0x00000800) cpuInfo->addFeature(kCpuFeatureRtm);

    // AVX2 depends on AVX.
    if (cpuInfo->hasFeature(kCpuFeatureAvx)) {
      if (regs.ebx & 0x00000020) cpuInfo->addFeature(kCpuFeatureAvx2);
    }
  }

  // Calling cpuid with 0x80000000 as the in argument gets the number of valid
  // extended IDs.
  callCpuId(0x80000000, 0, &regs);

  uint32_t maxExtId = IntUtil::iMin<uint32_t>(regs.eax, 0x80000004);
  uint32_t* brand = reinterpret_cast<uint32_t*>(cpuInfo->_brandString);

  for (i = 0x80000001; i <= maxExtId; i++) {
    callCpuId(i, 0, &regs);

    switch (i) {
      case 0x80000001:
        if (regs.ecx & 0x00000001U) cpuInfo->addFeature(kCpuFeatureLahfSahf);
        if (regs.ecx & 0x00000020U) cpuInfo->addFeature(kCpuFeatureLzcnt);
        if (regs.ecx & 0x00000040U) cpuInfo->addFeature(kCpuFeatureSse4A);
        if (regs.ecx & 0x00000080U) cpuInfo->addFeature(kCpuFeatureMsse);
        if (regs.ecx & 0x00000100U) cpuInfo->addFeature(kCpuFeaturePrefetch);

        if (regs.edx & 0x00100000U) cpuInfo->addFeature(kCpuFeatureExecuteDisableBit);
        if (regs.edx & 0x00200000U) cpuInfo->addFeature(kCpuFeatureFfxsr);
        if (regs.edx & 0x00400000U) cpuInfo->addFeature(kCpuFeatureMmxExt);
        if (regs.edx & 0x08000000U) cpuInfo->addFeature(kCpuFeatureRdtscp);
        if (regs.edx & 0x40000000U) cpuInfo->addFeature(kCpuFeature3dNowExt).addFeature(kCpuFeatureMmxExt);
        if (regs.edx & 0x80000000U) cpuInfo->addFeature(kCpuFeature3dNow);
        break;

      case 0x80000002:
      case 0x80000003:
      case 0x80000004:
        *brand++ = regs.eax;
        *brand++ = regs.ebx;
        *brand++ = regs.ecx;
        *brand++ = regs.edx;
        break;

      default:
        // Additional features can be detected in the future.
        break;
    }
  }

  // Simplify the brand string (remove unnecessary spaces to make printing nicer).
  simplifyBrandString(cpuInfo->_brandString);
}
#endif

} // x86x64 namespace
} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
