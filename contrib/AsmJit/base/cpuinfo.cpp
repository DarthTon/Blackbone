// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/cpuinfo.h"

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
#include "../x86/x86cpuinfo.h"
#else
// ?
#endif // ASMJIT_HOST || ASMJIT_HOST_X64

// [Dependencies - Posix]
#if defined(ASMJIT_OS_POSIX)
# include <errno.h>
# include <sys/statvfs.h>
# include <sys/utsname.h>
# include <unistd.h>
#endif // ASMJIT_OS_POSIX

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::BaseCpuInfo - DetectNumberOfCores]
// ============================================================================

uint32_t BaseCpuInfo::detectNumberOfCores() {
#if defined(ASMJIT_OS_WINDOWS)
  SYSTEM_INFO info;
  ::GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
#elif defined(ASMJIT_OS_POSIX) && defined(_SC_NPROCESSORS_ONLN)
  // It seems that sysconf returns the number of "logical" processors on both
  // mac and linux.  So we get the number of "online logical" processors.
  long res = ::sysconf(_SC_NPROCESSORS_ONLN);
  if (res == -1) return 1;

  return static_cast<uint32_t>(res);
#else
  return 1;
#endif
}

// ============================================================================
// [asmjit::BaseCpuInfo - GetHost]
// ============================================================================

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
struct HostCpuInfo : public x86x64::CpuInfo {
  ASMJIT_INLINE HostCpuInfo() : CpuInfo() {
    x86x64::CpuUtil::detect(this);
  }
};
#else
#error "AsmJit - Unsupported CPU."
#endif // ASMJIT_HOST || ASMJIT_HOST_X64

const BaseCpuInfo* BaseCpuInfo::getHost() {
#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
  static HostCpuInfo cpuInfo;
#else
#error "AsmJit - Unsupported CPU."
#endif // ASMJIT_HOST || ASMJIT_HOST_X64
  return &cpuInfo;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
