// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CPUINFO_H
#define _ASMJIT_BASE_CPUINFO_H

// [Dependencies - AsmJit]
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_general
//! \{

// ============================================================================
// [asmjit::kCpuVendor]
// ============================================================================

//! Cpu vendor ID.
//!
//! Vendor IDs are specific to AsmJit library. During the library initialization
//! AsmJit checks host CPU and tries to identify the vendor based on the CPUID
//! calls. Some manufacturers changed their vendor strings and AsmJit is aware
//! of that - it checks multiple combinations and decides which vendor ID should
//! be used.
ASMJIT_ENUM(kCpuVendor) {
  //! No/Unknown vendor.
  kCpuVendorNone = 0,

  //! Intel vendor.
  kCpuVendorIntel = 1,
  //! AMD vendor.
  kCpuVendorAmd = 2,
  //! VIA vendor.
  kCpuVendorVia = 3
};

// ============================================================================
// [asmjit::CpuInfo]
// ============================================================================

//! Base cpu information.
struct CpuInfo {
  ASMJIT_NO_COPY(CpuInfo)

  //! \internal
  enum {
    kFeaturesPerUInt32 = static_cast<int>(sizeof(uint32_t)) * 8
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CpuInfo(uint32_t size = sizeof(CpuInfo)) : _size(size) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get CPU vendor string.
  ASMJIT_INLINE const char* getVendorString() const { return _vendorString; }
  //! Get CPU brand string.
  ASMJIT_INLINE const char* getBrandString() const { return _brandString; }

  //! Get CPU vendor ID.
  ASMJIT_INLINE uint32_t getVendorId() const { return _vendorId; }
  //! Get CPU family ID.
  ASMJIT_INLINE uint32_t getFamily() const { return _family; }
  //! Get CPU model ID.
  ASMJIT_INLINE uint32_t getModel() const { return _model; }
  //! Get CPU stepping.
  ASMJIT_INLINE uint32_t getStepping() const { return _stepping; }

  //! Get number of hardware threads available.
  ASMJIT_INLINE uint32_t getHwThreadsCount() const { return _hwThreadsCount; }

  //! Get whether CPU has a `feature`.
  ASMJIT_INLINE bool hasFeature(uint32_t feature) const {
    ASMJIT_ASSERT(feature < sizeof(_features) * 8);

    return static_cast<bool>(
      (_features[feature / kFeaturesPerUInt32] >> (feature % kFeaturesPerUInt32)) & 0x1);
  }

  //! Add a CPU `feature`.
  ASMJIT_INLINE CpuInfo& addFeature(uint32_t feature) {
    ASMJIT_ASSERT(feature < sizeof(_features) * 8);

    _features[feature / kFeaturesPerUInt32] |= (1U << (feature % kFeaturesPerUInt32));
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

  //! Detect the number of hardware threads.
  static ASMJIT_API uint32_t detectHwThreadsCount();

  //! Get host cpu.
  static ASMJIT_API const CpuInfo* getHost();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Size of the structure in bytes.
  uint32_t _size;

  //! Cpu short vendor string.
  char _vendorString[16];
  //! Cpu long vendor string (brand).
  char _brandString[64];

  //! Cpu vendor id, see `asmjit::kCpuVendor`.
  uint32_t _vendorId;
  //! Cpu family ID.
  uint32_t _family;
  //! Cpu model ID.
  uint32_t _model;
  //! Cpu stepping.
  uint32_t _stepping;

  //! Number of hardware threads.
  uint32_t _hwThreadsCount;

  //! Cpu features bitfield.
  uint32_t _features[4];
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_CPUINFO_H
