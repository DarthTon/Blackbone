// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/cputicks.h"

// [Dependencies - Posix]
#if defined(ASMJIT_OS_POSIX)
# include <time.h>
# include <unistd.h>
#endif // ASMJIT_OS_POSIX

// [Dependencies - Mac]
#if defined(ASMJIT_OS_MAC)
# include <mach/mach_time.h>
#endif // ASMJIT_OS_MAC

// [Dependencies - Windows]
#if defined(ASMJIT_OS_WINDOWS)
// `_InterlockedCompareExchange` is only available as intrinsic (MS Compiler).
# if defined(_MSC_VER)
#  include <intrin.h>
#  pragma intrinsic(_InterlockedCompareExchange)
# else
#  define _InterlockedCompareExchange InterlockedCompareExchange
# endif // _MSC_VER
#endif // ASMJIT_OS_WINDOWS

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CpuTicks - Windows]
// ============================================================================

#if defined(ASMJIT_OS_WINDOWS)
static volatile uint32_t CpuTicks_hiResOk;
static volatile double CpuTicks_hiResFreq;

uint32_t CpuTicks::now() {
  do {
    uint32_t hiResOk = CpuTicks_hiResOk;

    if (hiResOk == 1) {
      LARGE_INTEGER now;
      if (!::QueryPerformanceCounter(&now))
        break;
      return (int64_t)(double(now.QuadPart) / CpuTicks_hiResFreq);
    }

    if (hiResOk == 0) {
      LARGE_INTEGER qpf;
      if (!::QueryPerformanceFrequency(&qpf)) {
        _InterlockedCompareExchange((LONG*)&CpuTicks_hiResOk, 0xFFFFFFFF, 0);
        break;
      }

      LARGE_INTEGER now;
      if (!::QueryPerformanceCounter(&now)) {
        _InterlockedCompareExchange((LONG*)&CpuTicks_hiResOk, 0xFFFFFFFF, 0);
        break;
      }

      double freqDouble = double(qpf.QuadPart) / 1000.0;

      CpuTicks_hiResFreq = freqDouble;
      _InterlockedCompareExchange((LONG*)&CpuTicks_hiResOk, 1, 0);

      return static_cast<uint32_t>(
        static_cast<int64_t>(double(now.QuadPart) / freqDouble) & 0xFFFFFFFF);
    }
  } while (0);

  // Bail to a less precise GetTickCount().
  return ::GetTickCount();
}

// ============================================================================
// [asmjit::CpuTicks - Mac]
// ============================================================================

#elif defined(ASMJIT_OS_MAC)
static mach_timebase_info_data_t CpuTicks_machTime;

uint32_t CpuTicks::now() {
  // Initialize the first time CpuTicks::now() is called (See Apple's QA1398).
  if (CpuTicks_machTime.denom == 0) {
    if (mach_timebase_info(&CpuTicks_machTime) != KERN_SUCCESS);
      return 0;
  }

  // mach_absolute_time() returns nanoseconds, we need just milliseconds.
  uint64_t t = mach_absolute_time() / 1000000;

  t = t * CpuTicks_machTime.numer / CpuTicks_machTime.denom;
  return static_cast<uint32_t>(t & 0xFFFFFFFFU);
}

// ============================================================================
// [asmjit::CpuTicks - Posix]
// ============================================================================

#else
uint32_t CpuTicks::now() {
#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return 0;

  uint64_t t = (uint64_t(ts.tv_sec ) * 1000) + (uint64_t(ts.tv_nsec) / 1000000);
  return static_cast<uint32_t>(t & 0xFFFFFFFFU);
#else  // _POSIX_MONOTONIC_CLOCK
#error "AsmJit - Unsupported OS."
  return 0;
#endif  // _POSIX_MONOTONIC_CLOCK
}
#endif // ASMJIT_OS

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
