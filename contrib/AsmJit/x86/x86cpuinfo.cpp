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

// ============================================================================
// [asmjit::X86CpuVendor]
// ============================================================================

struct X86CpuVendor {
  uint32_t id;
  char text[12];
};

static const X86CpuVendor x86CpuVendorList[] = {
  { kCpuVendorIntel    , { 'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l' } },
  { kCpuVendorAmd      , { 'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D' } },
  { kCpuVendorVia      , { 'V', 'I', 'A',  0 , 'V', 'I', 'A',  0 , 'V', 'I', 'A',  0  } },
  { kCpuVendorVia      , { 'C', 'e', 'n', 't', 'a', 'u', 'r', 'H', 'a', 'u', 'l', 's' } }
};

static ASMJIT_INLINE bool x86CpuVendorEq(const X86CpuVendor& info, const char* vendorString) {
  const uint32_t* a = reinterpret_cast<const uint32_t*>(info.text);
  const uint32_t* b = reinterpret_cast<const uint32_t*>(vendorString);

  return (a[0] == b[0]) & (a[1] == b[1]) & (a[2] == b[2]);
}

static ASMJIT_INLINE void x86SimplifyBrandString(char* s) {
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
// [asmjit::X86CpuUtil]
// ============================================================================

// This is messy, I know. Cpuid is implemented as intrinsic in VS2005, but
// we should support other compilers as well. Main problem is that MS compilers
// in 64-bit mode not allows to use inline assembler, so we need intrinsic and
// we need also asm version.

union X86XCR {
  uint64_t value;

  struct {
    uint32_t eax;
    uint32_t edx;
  };
};

// callCpuId() and detectCpuInfo() for x86 and x64 platforms begins here.
#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
void X86CpuUtil::callCpuId(uint32_t inEax, uint32_t inEcx, X86CpuId* result) {

#if defined(_MSC_VER)
// 2009-02-05: Thanks to Mike Tajmajer for supporting VC7.1 compiler.
// ASMJIT_HOST_X64 is here only for readibility, only VS2005 can compile 64-bit code.
# if _MSC_VER >= 1400 || defined(ASMJIT_HOST_X64)
  // Done by intrinsics.
  __cpuidex(reinterpret_cast<int*>(result->i), inEax, inEcx);
# else // _MSC_VER < 1400
  uint32_t cpuid_eax = inEax;
  uint32_t cpuid_ecx = inCax;
  uint32_t* cpuid_out = result->i;

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
  __asm__ __volatile__( \
    "mov %%ebx, %%edi\n"  \
    "cpuid\n"             \
    "xchg %%edi, %%ebx\n" \
      : "=a" (outEax), "=D" (outEbx), "=c" (outEcx), "=d" (outEdx) \
      : "a" (inEax), "c" (inEcx))
# else
#  define __myCpuId(inEax, inEcx, outEax, outEbx, outEcx, outEdx) \
  __asm__ __volatile__( \
    "mov %%rbx, %%rdi\n"  \
    "cpuid\n"             \
    "xchg %%rdi, %%rbx\n" \
      : "=a" (outEax), "=D" (outEbx), "=c" (outEcx), "=d" (outEdx) \
      : "a" (inEax), "c" (inEcx))
# endif
  __myCpuId(inEax, inEcx, result->eax, result->ebx, result->ecx, result->edx);
#endif // COMPILER
}

static void callXGetBV(uint32_t inEcx, X86XCR* result) {

#if defined(_MSC_VER)

# if (_MSC_FULL_VER >= 160040219) // 2010SP1+
  result->value = _xgetbv(inEcx);
# else
  result->value = 0;
# endif

#elif defined(__GNUC__)

  unsigned int eax, edx;
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
  __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(inEcx));
# else
  __asm__ __volatile__(".byte 0x0F, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(inEcx));
# endif
  result->eax = eax;
  result->edx = edx;

#else

  result->value = 0;

#endif // COMPILER
}

void X86CpuUtil::detect(X86CpuInfo* cpuInfo) {
  X86CpuId regs;

  uint32_t i;
  uint32_t maxBaseId;

  bool maybeMPX = false;
  X86XCR xcr0;
  xcr0.value = 0;

  // Clear everything except the '_size' member.
  ::memset(reinterpret_cast<uint8_t*>(cpuInfo) + sizeof(uint32_t),
    0, sizeof(CpuInfo) - sizeof(uint32_t));

  // Fill safe defaults.
  cpuInfo->_hwThreadsCount = CpuInfo::detectHwThreadsCount();

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x0]
  // --------------------------------------------------------------------------

  // Get vendor string/id.
  callCpuId(0x0, 0x0, &regs);

  maxBaseId = regs.eax;
  ::memcpy(cpuInfo->_vendorString, &regs.ebx, 4);
  ::memcpy(cpuInfo->_vendorString + 4, &regs.edx, 4);
  ::memcpy(cpuInfo->_vendorString + 8, &regs.ecx, 4);

  for (i = 0; i < ASMJIT_ARRAY_SIZE(x86CpuVendorList); i++) {
    if (x86CpuVendorEq(x86CpuVendorList[i], cpuInfo->_vendorString)) {
      cpuInfo->_vendorId = x86CpuVendorList[i].id;
      break;
    }
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x1]
  // --------------------------------------------------------------------------

  if (maxBaseId >= 0x1) {
    // Get feature flags in ECX/EDX and family/model in EAX.
    callCpuId(0x1, 0x0, &regs);

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

    if (regs.ecx & 0x00000001U) cpuInfo->addFeature(kX86CpuFeatureSSE3);
    if (regs.ecx & 0x00000002U) cpuInfo->addFeature(kX86CpuFeaturePCLMULQDQ);
    if (regs.ecx & 0x00000008U) cpuInfo->addFeature(kX86CpuFeatureMONITOR);
    if (regs.ecx & 0x00000200U) cpuInfo->addFeature(kX86CpuFeatureSSSE3);
    if (regs.ecx & 0x00002000U) cpuInfo->addFeature(kX86CpuFeatureCMPXCHG16B);
    if (regs.ecx & 0x00080000U) cpuInfo->addFeature(kX86CpuFeatureSSE4_1);
    if (regs.ecx & 0x00100000U) cpuInfo->addFeature(kX86CpuFeatureSSE4_2);
    if (regs.ecx & 0x00400000U) cpuInfo->addFeature(kX86CpuFeatureMOVBE);
    if (regs.ecx & 0x00800000U) cpuInfo->addFeature(kX86CpuFeaturePOPCNT);
    if (regs.ecx & 0x02000000U) cpuInfo->addFeature(kX86CpuFeatureAESNI);
    if (regs.ecx & 0x04000000U) cpuInfo->addFeature(kX86CpuFeatureXSave);
    if (regs.ecx & 0x08000000U) cpuInfo->addFeature(kX86CpuFeatureXSaveOS);
    if (regs.ecx & 0x40000000U) cpuInfo->addFeature(kX86CpuFeatureRDRAND);

    if (regs.edx & 0x00000010U) cpuInfo->addFeature(kX86CpuFeatureRDTSC);
    if (regs.edx & 0x00000100U) cpuInfo->addFeature(kX86CpuFeatureCMPXCHG8B);
    if (regs.edx & 0x00008000U) cpuInfo->addFeature(kX86CpuFeatureCMOV);
    if (regs.edx & 0x00080000U) cpuInfo->addFeature(kX86CpuFeatureCLFLUSH);
    if (regs.edx & 0x00800000U) cpuInfo->addFeature(kX86CpuFeatureMMX);
    if (regs.edx & 0x01000000U) cpuInfo->addFeature(kX86CpuFeatureFXSR);
    if (regs.edx & 0x02000000U) cpuInfo->addFeature(kX86CpuFeatureSSE).addFeature(kX86CpuFeatureMMX2);
    if (regs.edx & 0x04000000U) cpuInfo->addFeature(kX86CpuFeatureSSE).addFeature(kX86CpuFeatureSSE2);
    if (regs.edx & 0x10000000U) cpuInfo->addFeature(kX86CpuFeatureMT);

    // AMD sets Multithreading to ON if it has two or more cores.
    if (cpuInfo->_hwThreadsCount == 1 && cpuInfo->_vendorId == kCpuVendorAmd && (regs.edx & 0x10000000U)) {
      cpuInfo->_hwThreadsCount = 2;
    }

    // Get the content of XCR0 if supported by CPU and enabled by OS.
    if ((regs.ecx & 0x0C000000U) == 0x0C000000U) {
      callXGetBV(0, &xcr0);
    }

    // Detect AVX+.
    if (regs.ecx & 0x10000000U) {
      // - XCR0[2:1] == 11b
      //   XMM & YMM states are enabled by OS.
      if ((xcr0.eax & 0x00000006U) == 0x00000006U) {
        cpuInfo->addFeature(kX86CpuFeatureAVX);

        if (regs.ecx & 0x00000800U) cpuInfo->addFeature(kX86CpuFeatureXOP);
        if (regs.ecx & 0x00004000U) cpuInfo->addFeature(kX86CpuFeatureFMA3);
        if (regs.ecx & 0x00010000U) cpuInfo->addFeature(kX86CpuFeatureFMA4);
        if (regs.ecx & 0x20000000U) cpuInfo->addFeature(kX86CpuFeatureF16C);
      }
    }
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x7 ECX=0x0]
  // --------------------------------------------------------------------------

  // Detect new features if the processor supports CPUID-07.
  if (maxBaseId >= 0x7) {
    callCpuId(0x7, 0x0, &regs);

    if (regs.ebx & 0x00000001U) cpuInfo->addFeature(kX86CpuFeatureFSGSBase);
    if (regs.ebx & 0x00000008U) cpuInfo->addFeature(kX86CpuFeatureBMI);
    if (regs.ebx & 0x00000010U) cpuInfo->addFeature(kX86CpuFeatureHLE);
    if (regs.ebx & 0x00000100U) cpuInfo->addFeature(kX86CpuFeatureBMI2);
    if (regs.ebx & 0x00000200U) cpuInfo->addFeature(kX86CpuFeatureMOVSBSTOSBOpt);
    if (regs.ebx & 0x00000800U) cpuInfo->addFeature(kX86CpuFeatureRTM);
    if (regs.ebx & 0x00004000U) maybeMPX = true;
    if (regs.ebx & 0x00040000U) cpuInfo->addFeature(kX86CpuFeatureRDSEED);
    if (regs.ebx & 0x00080000U) cpuInfo->addFeature(kX86CpuFeatureADX);
    if (regs.ebx & 0x00800000U) cpuInfo->addFeature(kX86CpuFeatureCLFLUSHOpt);
    if (regs.ebx & 0x20000000U) cpuInfo->addFeature(kX86CpuFeatureSHA);

    if (regs.ecx & 0x00000001U) cpuInfo->addFeature(kX86CpuFeaturePREFETCHWT1);

    // Detect AVX2.
    if (cpuInfo->hasFeature(kX86CpuFeatureAVX)) {
      if (regs.ebx & 0x00000020U) cpuInfo->addFeature(kX86CpuFeatureAVX2);
    }

    // Detect AVX-512+.
    if (regs.ebx & 0x00010000U) {
      // - XCR0[2:1] == 11b
      //   XMM & YMM states are enabled by OS.
      // - XCR0[7:5] == 111b
      //   Upper 256-bit of ZMM0-XMM15 and ZMM16-ZMM31 state are enabled by OS.
      if ((xcr0.eax & 0x00000076U) == 0x00000076U) {
        cpuInfo->addFeature(kX86CpuFeatureAVX512F);

        if (regs.ebx & 0x00020000U) cpuInfo->addFeature(kX86CpuFeatureAVX512DQ);
        if (regs.ebx & 0x04000000U) cpuInfo->addFeature(kX86CpuFeatureAVX512PF);
        if (regs.ebx & 0x08000000U) cpuInfo->addFeature(kX86CpuFeatureAVX512ER);
        if (regs.ebx & 0x10000000U) cpuInfo->addFeature(kX86CpuFeatureAVX512CD);
        if (regs.ebx & 0x40000000U) cpuInfo->addFeature(kX86CpuFeatureAVX512BW);
        if (regs.ebx & 0x80000000U) cpuInfo->addFeature(kX86CpuFeatureAVX512VL);
      }
    }
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0xD, ECX=0x0]
  // --------------------------------------------------------------------------

  if (maxBaseId >= 0xD && maybeMPX) {
    callCpuId(0xD, 0x0, &regs);

    // Both CPUID result and XCR0 has to be enabled to have support for MPX.
    if (((regs.eax & xcr0.eax) & 0x00000018U) == 0x00000018U) {
      cpuInfo->addFeature(kX86CpuFeatureMPX);
    }
  }

  // --------------------------------------------------------------------------
  // [CPUID EAX=0x80000000]
  // --------------------------------------------------------------------------

  // Calling cpuid with 0x80000000 as the in argument gets the number of valid
  // extended IDs.
  callCpuId(0x80000000, 0x0, &regs);

  uint32_t maxExtId = IntUtil::iMin<uint32_t>(regs.eax, 0x80000004);
  uint32_t* brand = reinterpret_cast<uint32_t*>(cpuInfo->_brandString);

  for (i = 0x80000001; i <= maxExtId; i++) {
    callCpuId(i, 0x0, &regs);

    switch (i) {
      case 0x80000001:
        if (regs.ecx & 0x00000001U) cpuInfo->addFeature(kX86CpuFeatureLahfSahf);
        if (regs.ecx & 0x00000020U) cpuInfo->addFeature(kX86CpuFeatureLZCNT);
        if (regs.ecx & 0x00000040U) cpuInfo->addFeature(kX86CpuFeatureSSE4A);
        if (regs.ecx & 0x00000080U) cpuInfo->addFeature(kX86CpuFeatureMSSE);
        if (regs.ecx & 0x00000100U) cpuInfo->addFeature(kX86CpuFeaturePREFETCH);

        if (regs.edx & 0x00100000U) cpuInfo->addFeature(kX86CpuFeatureNX);
        if (regs.edx & 0x00200000U) cpuInfo->addFeature(kX86CpuFeatureFXSROpt);
        if (regs.edx & 0x00400000U) cpuInfo->addFeature(kX86CpuFeatureMMX2);
        if (regs.edx & 0x08000000U) cpuInfo->addFeature(kX86CpuFeatureRDTSCP);
        if (regs.edx & 0x40000000U) cpuInfo->addFeature(kX86CpuFeature3DNOW2).addFeature(kX86CpuFeatureMMX2);
        if (regs.edx & 0x80000000U) cpuInfo->addFeature(kX86CpuFeature3DNOW);
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
  x86SimplifyBrandString(cpuInfo->_brandString);
}
#endif

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
