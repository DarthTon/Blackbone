// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/osutils.h"
#include "../base/utils.h"

#if ASMJIT_OS_POSIX
# include <sys/types.h>
# include <sys/mman.h>
# include <time.h>
# include <unistd.h>
#endif // ASMJIT_OS_POSIX

#if ASMJIT_OS_MAC
# include <mach/mach_time.h>
#endif // ASMJIT_OS_MAC

#if ASMJIT_OS_WINDOWS
# if defined(_MSC_VER) && _MSC_VER >= 1400
#  include <intrin.h>
# else
#  define _InterlockedCompareExchange InterlockedCompareExchange
# endif // _MSC_VER
#endif // ASMJIT_OS_WINDOWS

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::OSUtils - Virtual Memory]
// ============================================================================

// Windows specific implementation using `VirtualAllocEx` and `VirtualFree`.
#if ASMJIT_OS_WINDOWS
static ASMJIT_NOINLINE const VMemInfo& OSUtils_GetVMemInfo() noexcept {
  static VMemInfo vmi;

  if (ASMJIT_UNLIKELY(!vmi.hCurrentProcess)) {
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);

    vmi.pageSize = Utils::alignToPowerOf2<uint32_t>(info.dwPageSize);
    vmi.pageGranularity = info.dwAllocationGranularity;
    vmi.hCurrentProcess = ::GetCurrentProcess();
  }

  return vmi;
};

VMemInfo OSUtils::getVirtualMemoryInfo() noexcept { return OSUtils_GetVMemInfo(); }

void* OSUtils::allocVirtualMemory(size_t size, size_t* allocated, uint32_t flags) noexcept {
  return allocProcessMemory(static_cast<HANDLE>(0), size, allocated, flags);
}

Error OSUtils::releaseVirtualMemory(void* p, size_t size) noexcept {
  return releaseProcessMemory(static_cast<HANDLE>(0), p, size);
}

void* OSUtils::allocProcessMemory(HANDLE hProcess, size_t size, size_t* allocated, uint32_t flags) noexcept {
  if (size == 0)
    return nullptr;

  const VMemInfo& vmi = OSUtils_GetVMemInfo();
  if (!hProcess) hProcess = vmi.hCurrentProcess;

  // VirtualAllocEx rounds the allocated size to a page size automatically,
  // but we need the `alignedSize` so we can store the real allocated size
  // into `allocated` output.
  size_t alignedSize = Utils::alignTo(size, vmi.pageSize);

  // Windows XP SP2 / Vista+ allow data-execution-prevention (DEP).
  DWORD protectFlags = 0;

  if (flags & kVMExecutable)
    protectFlags |= (flags & kVMWritable) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
  else
    protectFlags |= (flags & kVMWritable) ? PAGE_READWRITE : PAGE_READONLY;

  LPVOID mBase = ::VirtualAllocEx(hProcess, nullptr, alignedSize, MEM_COMMIT | MEM_RESERVE, protectFlags);
  if (ASMJIT_UNLIKELY(!mBase)) return nullptr;

  ASMJIT_ASSERT(Utils::isAligned<size_t>(reinterpret_cast<size_t>(mBase), vmi.pageSize));
  if (allocated) *allocated = alignedSize;
  return mBase;
}

Error OSUtils::releaseProcessMemory(HANDLE hProcess, void* p, size_t size) noexcept {
  const VMemInfo& vmi = OSUtils_GetVMemInfo();
  if (!hProcess) hProcess = vmi.hCurrentProcess;

  if (ASMJIT_UNLIKELY(!::VirtualFreeEx(hProcess, p, 0, MEM_RELEASE)))
    return DebugUtils::errored(kErrorInvalidState);

  return kErrorOk;
}
#endif // ASMJIT_OS_WINDOWS

// Posix specific implementation using `mmap()` and `munmap()`.
#if ASMJIT_OS_POSIX

// Mac uses MAP_ANON instead of MAP_ANONYMOUS.
#if !defined(MAP_ANONYMOUS)
# define MAP_ANONYMOUS MAP_ANON
#endif // MAP_ANONYMOUS

static const VMemInfo& OSUtils_GetVMemInfo() noexcept {
  static VMemInfo vmi;
  if (ASMJIT_UNLIKELY(!vmi.pageSize)) {
    size_t pageSize = ::getpagesize();
    vmi.pageSize = pageSize;
    vmi.pageGranularity = std::max<size_t>(pageSize, 65536);
  }
  return vmi;
};

VMemInfo OSUtils::getVirtualMemoryInfo() noexcept { return OSUtils_GetVMemInfo(); }

void* OSUtils::allocVirtualMemory(size_t size, size_t* allocated, uint32_t flags) noexcept {
  const VMemInfo& vmi = OSUtils_GetVMemInfo();

  size_t alignedSize = Utils::alignTo<size_t>(size, vmi.pageSize);
  int protection = PROT_READ;

  if (flags & kVMWritable  ) protection |= PROT_WRITE;
  if (flags & kVMExecutable) protection |= PROT_EXEC;

  void* mbase = ::mmap(nullptr, alignedSize, protection, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ASMJIT_UNLIKELY(mbase == MAP_FAILED)) return nullptr;

  if (allocated) *allocated = alignedSize;
  return mbase;
}

Error OSUtils::releaseVirtualMemory(void* p, size_t size) noexcept {
  if (ASMJIT_UNLIKELY(::munmap(p, size) != 0))
    return DebugUtils::errored(kErrorInvalidState);

  return kErrorOk;
}
#endif // ASMJIT_OS_POSIX

// ============================================================================
// [asmjit::OSUtils - GetTickCount]
// ============================================================================

#if ASMJIT_OS_WINDOWS
static ASMJIT_INLINE uint32_t OSUtils_calcHiRes(const LARGE_INTEGER& now, double freq) noexcept {
  return static_cast<uint32_t>(
    (int64_t)(double(now.QuadPart) / freq) & 0xFFFFFFFF);
}

uint32_t OSUtils::getTickCount() noexcept {
  static volatile uint32_t _hiResTicks;
  static volatile double _hiResFreq;

  do {
    uint32_t hiResOk = _hiResTicks;
    LARGE_INTEGER qpf, now;

    // If for whatever reason this fails, bail to `GetTickCount()`.
    if (!::QueryPerformanceCounter(&now)) break;

    // Expected - if we ran through this at least once `hiResTicks` will be
    // either 1 or 0xFFFFFFFF. If it's '1' then the Hi-Res counter is available
    // and `QueryPerformanceCounter()` can be used.
    if (hiResOk == 1) return OSUtils_calcHiRes(now, _hiResFreq);

    // Hi-Res counter is not available, bail to `GetTickCount()`.
    if (hiResOk != 0) break;

    // Detect availability of Hi-Res counter, if not available, bail to `GetTickCount()`.
    if (!::QueryPerformanceFrequency(&qpf)) {
      _InterlockedCompareExchange((LONG*)&_hiResTicks, 0xFFFFFFFF, 0);
      break;
    }

    double freq = double(qpf.QuadPart) / 1000.0;
    _hiResFreq = freq;

    _InterlockedCompareExchange((LONG*)&_hiResTicks, 1, 0);
    return OSUtils_calcHiRes(now, freq);
  } while (0);

  return ::GetTickCount();
}
#elif ASMJIT_OS_MAC
uint32_t OSUtils::getTickCount() noexcept {
  static mach_timebase_info_data_t _machTime;

  // See Apple's QA1398.
  if (ASMJIT_UNLIKELY(_machTime.denom == 0) || mach_timebase_info(&_machTime) != KERN_SUCCESS)
    return 0;

  // `mach_absolute_time()` returns nanoseconds, we want milliseconds.
  uint64_t t = mach_absolute_time() / 1000000;

  t = t * _machTime.numer / _machTime.denom;
  return static_cast<uint32_t>(t & 0xFFFFFFFFU);
}
#elif defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
uint32_t OSUtils::getTickCount() noexcept {
  struct timespec ts;

  if (ASMJIT_UNLIKELY(clock_gettime(CLOCK_MONOTONIC, &ts) != 0))
    return 0;

  uint64_t t = (uint64_t(ts.tv_sec ) * 1000) + (uint64_t(ts.tv_nsec) / 1000000);
  return static_cast<uint32_t>(t & 0xFFFFFFFFU);
}
#else
#error "[asmjit] OSUtils::getTickCount() is not implemented for your target OS."
uint32_t OSUtils::getTickCount() noexcept { return 0; }
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
