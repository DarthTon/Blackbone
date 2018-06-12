// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CPUINFO_H
#define _ASMJIT_BASE_CPUINFO_H

// [Dependencies]
#include "../base/arch.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::CpuFeatures]
// ============================================================================

class CpuFeatures {
public:
  typedef uintptr_t BitWord;

  enum {
    kMaxFeatures = 128,
    kBitWordSize = static_cast<int>(sizeof(BitWord)) * 8,
    kNumBitWords = kMaxFeatures / kBitWordSize
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CpuFeatures() noexcept { reset(); }
  ASMJIT_INLINE CpuFeatures(const CpuFeatures& other) noexcept { init(other); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void init(const CpuFeatures& other) noexcept { ::memcpy(this, &other, sizeof(*this)); }
  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  //! Get all features as `BitWord` array.
  ASMJIT_INLINE BitWord* getBits() noexcept { return _bits; }
  //! Get all features as `BitWord` array (const).
  ASMJIT_INLINE const BitWord* getBits() const noexcept { return _bits; }

  //! Get if feature `feature` is present.
  ASMJIT_INLINE bool has(uint32_t feature) const noexcept {
    ASMJIT_ASSERT(feature < kMaxFeatures);

    uint32_t idx = feature / kBitWordSize;
    uint32_t bit = feature % kBitWordSize;

    return static_cast<bool>((_bits[idx] >> bit) & 0x1);
  }

  //! Get if all features as defined by `other` are  present.
  ASMJIT_INLINE bool hasAll(const CpuFeatures& other) const noexcept {
    for (uint32_t i = 0; i < kNumBitWords; i++)
      if ((_bits[i] & other._bits[i]) != other._bits[i])
        return false;
    return true;
  }

  //! Add a CPU `feature`.
  ASMJIT_INLINE CpuFeatures& add(uint32_t feature) noexcept {
    ASMJIT_ASSERT(feature < kMaxFeatures);

    uint32_t idx = feature / kBitWordSize;
    uint32_t bit = feature % kBitWordSize;

    _bits[idx] |= static_cast<BitWord>(1) << bit;
    return *this;
  }

  //! Remove a CPU `feature`.
  ASMJIT_INLINE CpuFeatures& remove(uint32_t feature) noexcept {
    ASMJIT_ASSERT(feature < kMaxFeatures);

    uint32_t idx = feature / kBitWordSize;
    uint32_t bit = feature % kBitWordSize;

    _bits[idx] &= ~(static_cast<BitWord>(1) << bit);
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  BitWord _bits[kNumBitWords];
};

// ============================================================================
// [asmjit::CpuInfo]
// ============================================================================

//! CPU information.
class CpuInfo {
public:
  //! CPU vendor ID.
  ASMJIT_ENUM(Vendor) {
    kVendorNone  = 0,                    //!< Generic or unknown.
    kVendorIntel = 1,                    //!< Intel vendor.
    kVendorAMD   = 2,                    //!< AMD vendor.
    kVendorVIA   = 3                     //!< VIA vendor.
  };

  //! ARM/ARM64 CPU features.
  ASMJIT_ENUM(ArmFeatures) {
    kArmFeatureV6 = 1,                   //!< ARMv6 instruction set.
    kArmFeatureV7,                       //!< ARMv7 instruction set.
    kArmFeatureV8,                       //!< ARMv8 instruction set.
    kArmFeatureTHUMB,                    //!< CPU provides THUMB v1 instruction set (THUMB mode).
    kArmFeatureTHUMB2,                   //!< CPU provides THUMB v2 instruction set (THUMB mode).
    kArmFeatureVFPv2,                    //!< CPU provides VFPv2 instruction set.
    kArmFeatureVFPv3,                    //!< CPU provides VFPv3 instruction set.
    kArmFeatureVFPv4,                    //!< CPU provides VFPv4 instruction set.
    kArmFeatureVFP_D32,                  //!< CPU provides 32 VFP-D (64-bit) registers.
    kArmFeatureEDSP,                     //!< CPU provides EDSP extensions.
    kArmFeatureASIMD,                    //!< CPU provides 'Advanced SIMD'.
    kArmFeatureIDIVA,                    //!< CPU provides hardware SDIV and UDIV (ARM mode).
    kArmFeatureIDIVT,                    //!< CPU provides hardware SDIV and UDIV (THUMB mode).
    kArmFeatureAES,                      //!< CPU provides AES instructions (ARM64 only).
    kArmFeatureCRC32,                    //!< CPU provides CRC32 instructions.
    kArmFeaturePMULL,                    //!< CPU provides PMULL instructions (ARM64 only).
    kArmFeatureSHA1,                     //!< CPU provides SHA1 instructions.
    kArmFeatureSHA256,                   //!< CPU provides SHA256 instructions.
    kArmFeatureAtomics64,                //!< CPU provides 64-bit load/store atomics (ARM64 only).

    kArmFeaturesCount                    //!< Count of ARM/ARM64 CPU features.
  };

  //! X86/X64 CPU features.
  ASMJIT_ENUM(X86Features) {
    kX86FeatureI486 = 1,                 //!< CPU is at least I486.
    kX86FeatureNX,                       //!< CPU has Not-Execute-Bit.
    kX86FeatureMT,                       //!< CPU has multi-threading.
    kX86FeatureALTMOVCR8,                //!< CPU supports `LOCK MOV CR8` (AMD CPUs).
    kX86FeatureCMOV,                     //!< CPU has CMOV.
    kX86FeatureCMPXCHG8B,                //!< CPU has CMPXCHG8B.
    kX86FeatureCMPXCHG16B,               //!< CPU has CMPXCHG16B (x64).
    kX86FeatureMSR,                      //!< CPU has RDMSR/WRMSR.
    kX86FeatureRDTSC,                    //!< CPU has RDTSC.
    kX86FeatureRDTSCP,                   //!< CPU has RDTSCP.
    kX86FeatureCLFLUSH,                  //!< CPU has CLFUSH.
    kX86FeatureCLFLUSHOPT,               //!< CPU has CLFUSHOPT.
    kX86FeatureCLWB,                     //!< CPU has CLWB.
    kX86FeatureCLZERO,                   //!< CPU has CLZERO.
    kX86FeaturePCOMMIT,                  //!< CPU has PCOMMIT.
    kX86FeaturePREFETCHW,                //!< CPU has PREFETCHW.
    kX86FeaturePREFETCHWT1,              //!< CPU has PREFETCHWT1.
    kX86FeatureLAHFSAHF,                 //!< CPU has LAHF/SAHF.
    kX86FeatureFXSR,                     //!< CPU has FXSAVE/FXRSTOR.
    kX86FeatureFXSROPT,                  //!< CPU has FXSAVE/FXRSTOR (optimized).
    kX86FeatureMMX,                      //!< CPU has MMX.
    kX86FeatureMMX2,                     //!< CPU has extended MMX.
    kX86Feature3DNOW,                    //!< CPU has 3DNOW.
    kX86Feature3DNOW2,                   //!< CPU has 3DNOW2 (enhanced).
    kX86FeatureGEODE,                    //!< CPU has GEODE extensions (few additions to 3DNOW).
    kX86FeatureSSE,                      //!< CPU has SSE.
    kX86FeatureSSE2,                     //!< CPU has SSE2.
    kX86FeatureSSE3,                     //!< CPU has SSE3.
    kX86FeatureSSSE3,                    //!< CPU has SSSE3.
    kX86FeatureSSE4A,                    //!< CPU has SSE4.A.
    kX86FeatureSSE4_1,                   //!< CPU has SSE4.1.
    kX86FeatureSSE4_2,                   //!< CPU has SSE4.2.
    kX86FeatureMSSE,                     //!< CPU has Misaligned SSE (MSSE).
    kX86FeatureMONITOR,                  //!< CPU has MONITOR and MWAIT.
    kX86FeatureMOVBE,                    //!< CPU has MOVBE.
    kX86FeaturePOPCNT,                   //!< CPU has POPCNT.
    kX86FeatureLZCNT,                    //!< CPU has LZCNT.
    kX86FeatureAESNI,                    //!< CPU has AESNI.
    kX86FeaturePCLMULQDQ,                //!< CPU has PCLMULQDQ.
    kX86FeatureRDRAND,                   //!< CPU has RDRAND.
    kX86FeatureRDSEED,                   //!< CPU has RDSEED.
    kX86FeatureSMAP,                     //!< CPU has SMAP (supervisor-mode access prevention).
    kX86FeatureSMEP,                     //!< CPU has SMEP (supervisor-mode execution prevention).
    kX86FeatureSHA,                      //!< CPU has SHA-1 and SHA-256.
    kX86FeatureXSAVE,                    //!< CPU has XSAVE support (XSAVE/XRSTOR, XSETBV/XGETBV, and XCR).
    kX86FeatureXSAVEC,                   //!< CPU has XSAVEC support (XSAVEC).
    kX86FeatureXSAVES,                   //!< CPU has XSAVES support (XSAVES/XRSTORS).
    kX86FeatureXSAVEOPT,                 //!< CPU has XSAVEOPT support (XSAVEOPT/XSAVEOPT64).
    kX86FeatureOSXSAVE,                  //!< CPU has XSAVE enabled by OS.
    kX86FeatureAVX,                      //!< CPU has AVX.
    kX86FeatureAVX2,                     //!< CPU has AVX2.
    kX86FeatureF16C,                     //!< CPU has F16C.
    kX86FeatureFMA,                      //!< CPU has FMA.
    kX86FeatureFMA4,                     //!< CPU has FMA4.
    kX86FeatureXOP,                      //!< CPU has XOP.
    kX86FeatureBMI,                      //!< CPU has BMI (bit manipulation instructions #1).
    kX86FeatureBMI2,                     //!< CPU has BMI2 (bit manipulation instructions #2).
    kX86FeatureADX,                      //!< CPU has ADX (multi-precision add-carry instruction extensions).
    kX86FeatureTBM,                      //!< CPU has TBM (trailing bit manipulation).
    kX86FeatureMPX,                      //!< CPU has MPX (memory protection extensions).
    kX86FeatureHLE,                      //!< CPU has HLE.
    kX86FeatureRTM,                      //!< CPU has RTM.
    kX86FeatureTSX,                      //!< CPU has TSX.
    kX86FeatureERMS,                     //!< CPU has ERMS (enhanced REP MOVSB/STOSB).
    kX86FeatureFSGSBASE,                 //!< CPU has FSGSBASE.
    kX86FeatureAVX512_F,                 //!< CPU has AVX512-F (foundation).
    kX86FeatureAVX512_CDI,               //!< CPU has AVX512-CDI (conflict detection).
    kX86FeatureAVX512_PFI,               //!< CPU has AVX512-PFI (prefetch instructions).
    kX86FeatureAVX512_ERI,               //!< CPU has AVX512-ERI (exponential and reciprocal).
    kX86FeatureAVX512_DQ,                //!< CPU has AVX512-DQ (DWORD/QWORD).
    kX86FeatureAVX512_BW,                //!< CPU has AVX512-BW (BYTE/WORD).
    kX86FeatureAVX512_VL,                //!< CPU has AVX512-VL (vector length extensions).
    kX86FeatureAVX512_IFMA,              //!< CPU has AVX512-IFMA (integer fused-multiply-add using 52-bit precision).
    kX86FeatureAVX512_VBMI,              //!< CPU has AVX512-VBMI (vector byte manipulation).
    kX86FeatureAVX512_VPOPCNTDQ,         //!< CPU has AVX512-VPOPCNTDQ (VPOPCNT[D|Q] instructions).
    kX86FeatureAVX512_4VNNIW,            //!< CPU has AVX512-VNNIW (vector NN instructions word variable precision).
    kX86FeatureAVX512_4FMAPS,            //!< CPU has AVX512-FMAPS (FMA packed single).

    kX86FeaturesCount                    //!< Count of X86/X64 CPU features.
  };

  // --------------------------------------------------------------------------
  // [ArmInfo]
  // --------------------------------------------------------------------------

  struct ArmData {
  };

  // --------------------------------------------------------------------------
  // [X86Info]
  // --------------------------------------------------------------------------

  struct X86Data {
    uint32_t _processorType;             //!< Processor type.
    uint32_t _brandIndex;                //!< Brand index.
    uint32_t _flushCacheLineSize;        //!< Flush cache line size (in bytes).
    uint32_t _maxLogicalProcessors;      //!< Maximum number of addressable IDs for logical processors.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CpuInfo() noexcept { reset(); }
  ASMJIT_INLINE CpuInfo(const CpuInfo& other) noexcept { init(other); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize CpuInfo to the given architecture, see \ArchInfo.
  ASMJIT_INLINE void initArch(uint32_t archType, uint32_t archMode = 0) noexcept {
    _archInfo.init(archType, archMode);
  }

  ASMJIT_INLINE void init(const CpuInfo& other) noexcept { ::memcpy(this, &other, sizeof(*this)); }
  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Detect]
  // --------------------------------------------------------------------------

  ASMJIT_API void detect() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get generic architecture information.
  ASMJIT_INLINE const ArchInfo& getArchInfo() const noexcept { return _archInfo; }
  //! Get CPU architecture type, see \ArchInfo::Type.
  ASMJIT_INLINE uint32_t getArchType() const noexcept { return _archInfo.getType(); }
  //! Get CPU architecture sub-type, see \ArchInfo::SubType.
  ASMJIT_INLINE uint32_t getArchSubType() const noexcept { return _archInfo.getSubType(); }

    //! Get CPU vendor ID.
  ASMJIT_INLINE uint32_t getVendorId() const noexcept { return _vendorId; }
  //! Get CPU family ID.
  ASMJIT_INLINE uint32_t getFamily() const noexcept { return _family; }
  //! Get CPU model ID.
  ASMJIT_INLINE uint32_t getModel() const noexcept { return _model; }
  //! Get CPU stepping.
  ASMJIT_INLINE uint32_t getStepping() const noexcept { return _stepping; }

  //! Get number of hardware threads available.
  ASMJIT_INLINE uint32_t getHwThreadsCount() const noexcept {
    return _hwThreadsCount;
  }

  //! Get all CPU features.
  ASMJIT_INLINE const CpuFeatures& getFeatures() const noexcept { return _features; }
  //! Get whether CPU has a `feature`.
  ASMJIT_INLINE bool hasFeature(uint32_t feature) const noexcept { return _features.has(feature); }
  //! Add a CPU `feature`.
  ASMJIT_INLINE CpuInfo& addFeature(uint32_t feature) noexcept { _features.add(feature); return *this; }

  //! Get CPU vendor string.
  ASMJIT_INLINE const char* getVendorString() const noexcept { return _vendorString; }
  //! Get CPU brand string.
  ASMJIT_INLINE const char* getBrandString() const noexcept { return _brandString; }

  // --------------------------------------------------------------------------
  // [Accessors - ARM]
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // [Accessors - X86]
  // --------------------------------------------------------------------------

  //! Get processor type.
  ASMJIT_INLINE uint32_t getX86ProcessorType() const noexcept {
    return _x86Data._processorType;
  }

  //! Get brand index.
  ASMJIT_INLINE uint32_t getX86BrandIndex() const noexcept {
    return _x86Data._brandIndex;
  }

  //! Get flush cache line size.
  ASMJIT_INLINE uint32_t getX86FlushCacheLineSize() const noexcept {
    return _x86Data._flushCacheLineSize;
  }

  //! Get maximum logical processors count.
  ASMJIT_INLINE uint32_t getX86MaxLogicalProcessors() const noexcept {
    return _x86Data._maxLogicalProcessors;
  }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  //! Get the host CPU information.
  ASMJIT_API static const CpuInfo& getHost() noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ArchInfo _archInfo;                    //!< CPU architecture information.
  uint32_t _vendorId;                    //!< CPU vendor id, see \ref Vendor.
  uint32_t _family;                      //!< CPU family ID.
  uint32_t _model;                       //!< CPU model ID.
  uint32_t _stepping;                    //!< CPU stepping.
  uint32_t _hwThreadsCount;              //!< Number of hardware threads.
  CpuFeatures _features;                 //!< CPU features.
  char _vendorString[16];                //!< CPU vendor string.
  char _brandString[64];                 //!< CPU brand string.

  // Architecture specific data.
  union {
    ArmData _armData;
    X86Data _x86Data;
  };
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_CPUINFO_H
