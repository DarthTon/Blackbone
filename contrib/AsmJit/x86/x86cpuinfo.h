// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86CPUINFO_H
#define _ASMJIT_X86_X86CPUINFO_H

// [Dependencies - AsmJit]
#include "../base/cpuinfo.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct CpuInfo;

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x86x64::kCpuFeature]
// ============================================================================

//! X86 CPU features.
ASMJIT_ENUM(kCpuFeature) {
  //! Cpu has multithreading.
  kCpuFeatureMultithreading = 1,
  //! Cpu has execute disable bit.
  kCpuFeatureExecuteDisableBit,
  //! Cpu has RDTSC.
  kCpuFeatureRdtsc,
  //! Cpu has RDTSCP.
  kCpuFeatureRdtscp,
  //! Cpu has CMOV.
  kCpuFeatureCmov,
  //! Cpu has CMPXCHG8B.
  kCpuFeatureCmpXchg8B,
  //! Cpu has CMPXCHG16B (x64).
  kCpuFeatureCmpXchg16B,
  //! Cpu has CLFUSH.
  kCpuFeatureClflush,
  //! Cpu has PREFETCH.
  kCpuFeaturePrefetch,
  //! Cpu has LAHF/SAHF.
  kCpuFeatureLahfSahf,
  //! Cpu has FXSAVE/FXRSTOR.
  kCpuFeatureFxsr,
  //! Cpu has FXSAVE/FXRSTOR optimizations.
  kCpuFeatureFfxsr,
  //! Cpu has MMX.
  kCpuFeatureMmx,
  //! Cpu has extended MMX.
  kCpuFeatureMmxExt,
  //! Cpu has 3dNow!
  kCpuFeature3dNow,
  //! Cpu has enchanced 3dNow!
  kCpuFeature3dNowExt,
  //! Cpu has SSE.
  kCpuFeatureSse,
  //! Cpu has SSE2.
  kCpuFeatureSse2,
  //! Cpu has SSE3.
  kCpuFeatureSse3,
  //! Cpu has Supplemental SSE3 (SSSE3).
  kCpuFeatureSsse3,
  //! Cpu has SSE4.A.
  kCpuFeatureSse4A,
  //! Cpu has SSE4.1.
  kCpuFeatureSse41,
  //! Cpu has SSE4.2.
  kCpuFeatureSse42,
  //! Cpu has Misaligned SSE (MSSE).
  kCpuFeatureMsse,
  //! Cpu has MONITOR and MWAIT.
  kCpuFeatureMonitorMWait,
  //! Cpu has MOVBE.
  kCpuFeatureMovbe,
  //! Cpu has POPCNT.
  kCpuFeaturePopcnt,
  //! Cpu has LZCNT.
  kCpuFeatureLzcnt,
  //! Cpu has AESNI.
  kCpuFeatureAesni,
  //! Cpu has PCLMULQDQ.
  kCpuFeaturePclmulqdq,
  //! Cpu has RDRAND.
  kCpuFeatureRdrand,
  //! Cpu has AVX.
  kCpuFeatureAvx,
  //! Cpu has AVX2.
  kCpuFeatureAvx2,
  //! Cpu has F16C.
  kCpuFeatureF16C,
  //! Cpu has FMA3.
  kCpuFeatureFma3,
  //! Cpu has FMA4.
  kCpuFeatureFma4,
  //! Cpu has XOP.
  kCpuFeatureXop,
  //! Cpu has BMI.
  kCpuFeatureBmi,
  //! Cpu has BMI2.
  kCpuFeatureBmi2,
  //! Cpu has HLE.
  kCpuFeatureHle,
  //! Cpu has RTM.
  kCpuFeatureRtm,
  //! Cpu has FSGSBASE.
  kCpuFeatureFsGsBase,
  //! Cpu has enhanced REP MOVSB/STOSB.
  kCpuFeatureRepMovsbStosbExt,

  //! Count of X86/X64 Cpu features.
  kCpuFeatureCount
};

// ============================================================================
// [asmjit::x86x64::CpuId]
// ============================================================================

//! X86/X64 CPUID output.
union CpuId {
  //! EAX/EBX/ECX/EDX output.
  uint32_t i[4];

  struct {
    //! EAX output.
    uint32_t eax;
    //! EBX output.
    uint32_t ebx;
    //! ECX output.
    uint32_t ecx;
    //! EDX output.
    uint32_t edx;
  };
};

// ============================================================================
// [asmjit::x86x64::CpuUtil]
// ============================================================================

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
//! CPU utilities available only if the host processor is X86/X64.
struct CpuUtil {
  //! Get the result of calling CPUID instruction to `out`.
  ASMJIT_API static void callCpuId(uint32_t inEax, uint32_t inEcx, CpuId* out);

  //! Detect the Host CPU.
  ASMJIT_API static void detect(CpuInfo* cpuInfo);
};
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64

// ============================================================================
// [asmjit::x86x64::CpuInfo]
// ============================================================================

struct CpuInfo : public BaseCpuInfo {
  ASMJIT_NO_COPY(CpuInfo)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CpuInfo(uint32_t size = sizeof(CpuInfo)) :
    BaseCpuInfo(size) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get processor type.
  ASMJIT_INLINE uint32_t getProcessorType() const {
    return _processorType;
  }

  //! Get brand index.
  ASMJIT_INLINE uint32_t getBrandIndex() const {
    return _brandIndex;
  }

  //! Get flush cache line size.
  ASMJIT_INLINE uint32_t getFlushCacheLineSize() const {
    return _flushCacheLineSize;
  }

  //! Get maximum logical processors count.
  ASMJIT_INLINE uint32_t getMaxLogicalProcessors() const {
    return _maxLogicalProcessors;
  }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  //! Get global instance of `x86x64::CpuInfo`.
  static ASMJIT_INLINE const CpuInfo* getHost() {
    return static_cast<const CpuInfo*>(BaseCpuInfo::getHost());
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Processor type.
  uint32_t _processorType;
  //! Brand index.
  uint32_t _brandIndex;
  //! Flush cache line size in bytes.
  uint32_t _flushCacheLineSize;
  //! Maximum number of addressable IDs for logical processors.
  uint32_t _maxLogicalProcessors;
};

//! \}

} // x86x64 namespace
} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86CPUINFO_H
