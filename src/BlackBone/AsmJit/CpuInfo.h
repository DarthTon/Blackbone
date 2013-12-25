// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_CPUINFO_H
#define _ASMJIT_CPUINFO_H

// [Dependencies]
#include "Build.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

#pragma warning(disable : 4201)

//! @addtogroup AsmJit_CpuInfo
//! @{

// ============================================================================
// [AsmJit::CpuId]
// ============================================================================

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
//! @brief Structure (union) used by cpuid() function.
union CpuId
{
  //! @brief cpuid results array(eax, ebx, ecx and edx registers).
  uint32_t i[4];

  struct
  {
    //! @brief cpuid result in eax register.
    uint32_t eax;
    //! @brief cpuid result in ebx register.
    uint32_t ebx;
    //! @brief cpuid result in ecx register.
    uint32_t ecx;
    //! @brief cpuid result in edx register.
    uint32_t edx;
  };
};

//! @brief Calls CPUID instruction with eax == @a in and returns result to @a out.
//!
//! @c cpuid() function has one input parameter that is passed to cpuid through 
//! eax register and results in four output values representing result of cpuid 
//! instruction (eax, ebx, ecx and edx registers).
ASMJIT_API void cpuid(uint32_t in, CpuId* out) ASMJIT_NOTHROW;
#endif // ASMJIT_X86 || ASMJIT_X64

// ============================================================================
// [AsmJit::CPU_VENDOR]
// ============================================================================

//! @brief Cpu vendor IDs.
//!
//! Cpu vendor IDs are specific for AsmJit library. Vendor ID is not directly
//! read from cpuid result, instead it's based on CPU vendor string.
enum CPU_VENDOR
{
  //! @brief Unknown CPU vendor.
  CPU_VENDOR_UNKNOWN = 0,

  //! @brief Intel CPU vendor.
  CPU_VENDOR_INTEL = 1,
  //! @brief AMD CPU vendor.
  CPU_VENDOR_AMD = 2,
  //! @brief National Semiconductor CPU vendor (applies also to Cyrix processors).
  CPU_VENDOR_NSM = 3,
  //! @brief Transmeta CPU vendor.
  CPU_VENDOR_TRANSMETA = 4,
  //! @brief VIA CPU vendor.
  CPU_VENDOR_VIA = 5
};

// ============================================================================
// [AsmJit::CPU_FEATURE]
// ============================================================================

//! @brief X86/X64 CPU features.
enum CPU_FEATURE
{
  //! @brief Cpu has RDTSC instruction.
  CPU_FEATURE_RDTSC = 1U << 0,
  //! @brief Cpu has RDTSCP instruction.
  CPU_FEATURE_RDTSCP = 1U << 1,
  //! @brief Cpu has CMOV instruction (conditional move)
  CPU_FEATURE_CMOV = 1U << 2,
  //! @brief Cpu has CMPXCHG8B instruction
  CPU_FEATURE_CMPXCHG8B = 1U << 3,
  //! @brief Cpu has CMPXCHG16B instruction (64-bit processors)
  CPU_FEATURE_CMPXCHG16B = 1U << 4,
  //! @brief Cpu has CLFUSH instruction
  CPU_FEATURE_CLFLUSH = 1U << 5,
  //! @brief Cpu has PREFETCH instruction
  CPU_FEATURE_PREFETCH = 1U << 6,
  //! @brief Cpu supports LAHF and SAHF instrictions.
  CPU_FEATURE_LAHF_SAHF = 1U << 7,
  //! @brief Cpu supports FXSAVE and FXRSTOR instructions.
  CPU_FEATURE_FXSR = 1U << 8,
  //! @brief Cpu supports FXSAVE and FXRSTOR instruction optimizations (FFXSR).
  CPU_FEATURE_FFXSR = 1U << 9,
  //! @brief Cpu has MMX.
  CPU_FEATURE_MMX = 1U << 10,
  //! @brief Cpu has extended MMX.
  CPU_FEATURE_MMX_EXT = 1U << 11,
  //! @brief Cpu has 3dNow!
  CPU_FEATURE_3DNOW = 1U << 12,
  //! @brief Cpu has enchanced 3dNow!
  CPU_FEATURE_3DNOW_EXT = 1U << 13,
  //! @brief Cpu has SSE.
  CPU_FEATURE_SSE = 1U << 14,
  //! @brief Cpu has SSE2.
  CPU_FEATURE_SSE2 = 1U << 15,
  //! @brief Cpu has SSE3.
  CPU_FEATURE_SSE3 = 1U << 16,
  //! @brief Cpu has Supplemental SSE3 (SSSE3).
  CPU_FEATURE_SSSE3 = 1U << 17,
  //! @brief Cpu has SSE4.A.
  CPU_FEATURE_SSE4_A = 1U << 18,
  //! @brief Cpu has SSE4.1.
  CPU_FEATURE_SSE4_1 = 1U << 19,
  //! @brief Cpu has SSE4.2.
  CPU_FEATURE_SSE4_2 = 1U << 20,
  //! @brief Cpu has AVX.
  CPU_FEATURE_AVX = 1U << 22,
  //! @brief Cpu has Misaligned SSE (MSSE).
  CPU_FEATURE_MSSE = 1U << 23,
  //! @brief Cpu supports MONITOR and MWAIT instructions.
  CPU_FEATURE_MONITOR_MWAIT = 1U << 24,
  //! @brief Cpu supports MOVBE instruction.
  CPU_FEATURE_MOVBE = 1U << 25,
  //! @brief Cpu supports POPCNT instruction.
  CPU_FEATURE_POPCNT = 1U << 26,
  //! @brief Cpu supports LZCNT instruction.
  CPU_FEATURE_LZCNT = 1U << 27,
  //! @brief Cpu supports PCLMULDQ set of instructions.
  CPU_FEATURE_PCLMULDQ  = 1U << 28,
  //! @brief Cpu supports multithreading.
  CPU_FEATURE_MULTI_THREADING = 1U << 29,
  //! @brief Cpu supports execute disable bit (execute protection).
  CPU_FEATURE_EXECUTE_DISABLE_BIT = 1U << 30,
  //! @brief 64-bit CPU.
  CPU_FEATURE_64_BIT = 1U << 31
};

// ============================================================================
// [AsmJit::CPU_BUG]
// ============================================================================

//! @brief X86/X64 CPU bugs.
enum CPU_BUG
{
  //! @brief Whether the processor contains bug seen in some 
  //! AMD-Opteron processors.
  CPU_BUG_AMD_LOCK_MB = 1U << 0
};

// ============================================================================
// [AsmJit::CpuInfo]
// ============================================================================

//! @brief Informations about host cpu.
struct ASMJIT_HIDDEN CpuInfo
{
  //! @brief Cpu short vendor string.
  char vendor[16];
  //! @brief Cpu long vendor string (brand).
  char brand[64];
  //! @brief Cpu vendor id (see @c AsmJit::CpuInfo::VendorId enum).
  uint32_t vendorId;
  //! @brief Cpu family ID.
  uint32_t family;
  //! @brief Cpu model ID.
  uint32_t model;
  //! @brief Cpu stepping.
  uint32_t stepping;
  //! @brief Number of processors or cores.
  uint32_t numberOfProcessors;
  //! @brief Cpu features bitfield, see @c AsmJit::CpuInfo::Feature enum).
  uint32_t features;
  //! @brief Cpu bugs bitfield, see @c AsmJit::CpuInfo::Bug enum).
  uint32_t bugs;

#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
  //! @brief Extended information for x86/x64 compatible processors.
  struct X86ExtendedInfo
  {
    //! @brief Processor type.
    uint32_t processorType;
    //! @brief Brand index.
    uint32_t brandIndex;
    //! @brief Flush cache line size in bytes.
    uint32_t flushCacheLineSize;
    //! @brief Maximum number of addressable IDs for logical processors.
    uint32_t maxLogicalProcessors;
    //! @brief Initial APIC ID.
    uint32_t apicPhysicalId;
  };
  //! @brief Extended information for x86/x64 compatible processors.
  X86ExtendedInfo x86ExtendedInfo;
#endif // ASMJIT_X86 || ASMJIT_X64
};

//! @brief Detect CPU features to CpuInfo structure @a i.
//!
//! @sa @c CpuInfo.
ASMJIT_API void detectCpuInfo(CpuInfo* i) ASMJIT_NOTHROW;

//! @brief Return CpuInfo (detection is done only once).
//!
//! @sa @c CpuInfo.
ASMJIT_API CpuInfo* getCpuInfo() ASMJIT_NOTHROW;

//! @}

#pragma warning(default : 4201)

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_CPUINFO_H
