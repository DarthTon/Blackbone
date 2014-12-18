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

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct X86CpuInfo;

//! \addtogroup asmjit_x86_general
//! \{

// ============================================================================
// [asmjit::kX86CpuFeature]
// ============================================================================

//! X86 CPU features.
ASMJIT_ENUM(kX86CpuFeature) {
  //! Cpu has Not-Execute-Bit.
  kX86CpuFeatureNX = 0,
  //! Cpu has multithreading.
  kX86CpuFeatureMT,
  //! Cpu has RDTSC.
  kX86CpuFeatureRDTSC,
  //! Cpu has RDTSCP.
  kX86CpuFeatureRDTSCP,
  //! Cpu has CMOV.
  kX86CpuFeatureCMOV,
  //! Cpu has CMPXCHG8B.
  kX86CpuFeatureCMPXCHG8B,
  //! Cpu has CMPXCHG16B (X64).
  kX86CpuFeatureCMPXCHG16B,
  //! Cpu has CLFUSH.
  kX86CpuFeatureCLFLUSH,
  //! Cpu has CLFUSH (Optimized).
  kX86CpuFeatureCLFLUSHOpt,
  //! Cpu has PREFETCH.
  kX86CpuFeaturePREFETCH,
  //! Cpu has PREFETCHWT1.
  kX86CpuFeaturePREFETCHWT1,
  //! Cpu has LAHF/SAHF.
  kX86CpuFeatureLahfSahf,
  //! Cpu has FXSAVE/FXRSTOR.
  kX86CpuFeatureFXSR,
  //! Cpu has FXSAVE/FXRSTOR (Optimized).
  kX86CpuFeatureFXSROpt,
  //! Cpu has MMX.
  kX86CpuFeatureMMX,
  //! Cpu has extended MMX.
  kX86CpuFeatureMMX2,
  //! Cpu has 3dNow!
  kX86CpuFeature3DNOW,
  //! Cpu has enchanced 3dNow!
  kX86CpuFeature3DNOW2,
  //! Cpu has SSE.
  kX86CpuFeatureSSE,
  //! Cpu has SSE2.
  kX86CpuFeatureSSE2,
  //! Cpu has SSE3.
  kX86CpuFeatureSSE3,
  //! Cpu has SSSE3.
  kX86CpuFeatureSSSE3,
  //! Cpu has SSE4.A.
  kX86CpuFeatureSSE4A,
  //! Cpu has SSE4.1.
  kX86CpuFeatureSSE4_1,
  //! Cpu has SSE4.2.
  kX86CpuFeatureSSE4_2,
  //! Cpu has Misaligned SSE (MSSE).
  kX86CpuFeatureMSSE,
  //! Cpu has MONITOR and MWAIT.
  kX86CpuFeatureMONITOR,
  //! Cpu has MOVBE.
  kX86CpuFeatureMOVBE,
  //! Cpu has POPCNT.
  kX86CpuFeaturePOPCNT,
  //! Cpu has LZCNT.
  kX86CpuFeatureLZCNT,
  //! Cpu has AESNI.
  kX86CpuFeatureAESNI,
  //! Cpu has PCLMULQDQ.
  kX86CpuFeaturePCLMULQDQ,
  //! Cpu has RDRAND.
  kX86CpuFeatureRDRAND,
  //! Cpu has RDSEED.
  kX86CpuFeatureRDSEED,
  //! Cpu has SHA-1 and SHA-256.
  kX86CpuFeatureSHA,
  //! Cpu has XSAVE support - XSAVE/XRSTOR, XSETBV/XGETBV, and XCR0.
  kX86CpuFeatureXSave,
  //! OS has enabled XSAVE, you can call XGETBV to get value of XCR0.
  kX86CpuFeatureXSaveOS,
  //! Cpu has AVX.
  kX86CpuFeatureAVX,
  //! Cpu has AVX2.
  kX86CpuFeatureAVX2,
  //! Cpu has F16C.
  kX86CpuFeatureF16C,
  //! Cpu has FMA3.
  kX86CpuFeatureFMA3,
  //! Cpu has FMA4.
  kX86CpuFeatureFMA4,
  //! Cpu has XOP.
  kX86CpuFeatureXOP,
  //! Cpu has BMI.
  kX86CpuFeatureBMI,
  //! Cpu has BMI2.
  kX86CpuFeatureBMI2,
  //! Cpu has HLE.
  kX86CpuFeatureHLE,
  //! Cpu has RTM.
  kX86CpuFeatureRTM,
  //! Cpu has ADX.
  kX86CpuFeatureADX,
  //! Cpu has MPX (Memory Protection Extensions).
  kX86CpuFeatureMPX,
  //! Cpu has FSGSBASE.
  kX86CpuFeatureFSGSBase,
  //! Cpu has optimized REP MOVSB/STOSB.
  kX86CpuFeatureMOVSBSTOSBOpt,

  //! Cpu has AVX-512F (Foundation).
  kX86CpuFeatureAVX512F,
  //! Cpu has AVX-512CD (Conflict Detection).
  kX86CpuFeatureAVX512CD,
  //! Cpu has AVX-512PF (Prefetch Instructions).
  kX86CpuFeatureAVX512PF,
  //! Cpu has AVX-512ER (Exponential and Reciprocal Instructions).
  kX86CpuFeatureAVX512ER,
  //! Cpu has AVX-512DQ (DWord/QWord).
  kX86CpuFeatureAVX512DQ,
  //! Cpu has AVX-512BW (Byte/Word).
  kX86CpuFeatureAVX512BW,
  //! Cpu has AVX VL (Vector Length Excensions).
  kX86CpuFeatureAVX512VL,

  //! Count of X86/X64 Cpu features.
  kX86CpuFeatureCount
};

// ============================================================================
// [asmjit::X86CpuId]
// ============================================================================

//! X86/X64 CPUID output.
union X86CpuId {
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
// [asmjit::X86CpuUtil]
// ============================================================================

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
//! CPU utilities available only if the host processor is X86/X64.
struct X86CpuUtil {
  //! Get the result of calling CPUID instruction to `out`.
  ASMJIT_API static void callCpuId(uint32_t inEax, uint32_t inEcx, X86CpuId* out);

  //! Detect the Host CPU.
  ASMJIT_API static void detect(X86CpuInfo* cpuInfo);
};
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64

// ============================================================================
// [asmjit::X86CpuInfo]
// ============================================================================

struct X86CpuInfo : public CpuInfo {
  ASMJIT_NO_COPY(X86CpuInfo)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86CpuInfo(uint32_t size = sizeof(X86CpuInfo)) :
    CpuInfo(size) {}

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

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
  //! Get global instance of `X86CpuInfo`.
  static ASMJIT_INLINE const X86CpuInfo* getHost() {
    return static_cast<const X86CpuInfo*>(CpuInfo::getHost());
  }
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64

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

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86CPUINFO_H
