// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../base/cpuinfo.h"
#include "../base/error.h"
#include "../base/runtime.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::Runtime - Construction / Destruction]
// ============================================================================

Runtime::Runtime() {
  _sizeLimit = 0;

  _runtimeType = kRuntimeTypeNone;
  _allocType = kVMemAllocFreeable;
  ::memset(_reserved, 0, sizeof(_reserved));

  _baseAddress = kNoBaseAddress;
}

Runtime::~Runtime() {}

// ============================================================================
// [asmjit::HostRuntime - Construction / Destruction]
// ============================================================================

HostRuntime::HostRuntime() {
  _runtimeType = kRuntimeTypeJit;
}

HostRuntime::~HostRuntime() {}

// ============================================================================
// [asmjit::HostRuntime - Interface]
// ============================================================================

const CpuInfo* HostRuntime::getCpuInfo() {
  return CpuInfo::getHost();
}

uint32_t HostRuntime::getStackAlignment() {
  uint32_t alignment = sizeof(intptr_t);

#if defined(ASMJIT_HOST_X86)
  // Modern Linux, APPLE and UNIX guarantees 16-byte stack alignment, but I'm
  // not sure about all other UNIX operating systems, because 16-byte alignment
  // is addition to an older specification.
# if (defined(__linux__)   || \
      defined(__linux)     || \
      defined(__unix__)    || \
      defined(__FreeBSD__) || \
      defined(__NetBSD__)  || \
      defined(__OpenBSD__) || \
      defined(__DARWIN__)  || \
      defined(__APPLE__)   )
  alignment = 16;
# endif
#elif defined(ASMJIT_HOST_X64)
  alignment = 16;
#endif

  return alignment;
}

void HostRuntime::flush(void* p, size_t size) {
  // Only useful on non-x86 architectures.
#if !defined(ASMJIT_HOST_X86) && !defined(ASMJIT_HOST_X64)

  // Windows has built-in support in kernel32.dll.
#if defined(ASMJIT_OS_WINDOWS)
  ::FlushInstructionCache(_memMgr.getProcessHandle(), p, size);
#endif // ASMJIT_OS_WINDOWS

#endif // !ASMJIT_HOST_X86 && !ASMJIT_HOST_X64
}

// ============================================================================
// [asmjit::StaticRuntime - Construction / Destruction]
// ============================================================================

StaticRuntime::StaticRuntime(void* baseAddress, size_t sizeLimit) {
  _sizeLimit = sizeLimit;
  _baseAddress = static_cast<Ptr>((uintptr_t)baseAddress);
}

StaticRuntime::~StaticRuntime() {}

// ============================================================================
// [asmjit::StaticRuntime - Interface]
// ============================================================================

Error StaticRuntime::add(void** dst, Assembler* assembler) {
  size_t codeSize = assembler->getCodeSize();
  size_t sizeLimit = _sizeLimit;

  if (codeSize == 0) {
    *dst = NULL;
    return kErrorNoCodeGenerated;
  }

  if (sizeLimit != 0 && sizeLimit < codeSize) {
    *dst = NULL;
    return kErrorCodeTooLarge;
  }

  Ptr baseAddress = _baseAddress;
  uint8_t* p = static_cast<uint8_t*>((void*)static_cast<uintptr_t>(baseAddress));

  // Since the base address is known the `relocSize` returned should be equal
  // to `codeSize`. It's better to fail if they don't match instead of passsing
  // silently.
  size_t relocSize = assembler->relocCode(p, baseAddress);
  if (relocSize == 0 || codeSize != relocSize) {
    *dst = NULL;
    return kErrorInvalidState;
  }

  _baseAddress += codeSize;
  if (sizeLimit)
    sizeLimit -= codeSize;

  flush(p, codeSize);
  *dst = p;

  return kErrorOk;
}

Error StaticRuntime::release(void* p) {
  // There is nothing to release as `StaticRuntime` doesn't manage any memory.
  ASMJIT_UNUSED(p);
  return kErrorOk;
}

// ============================================================================
// [asmjit::JitRuntime - Construction / Destruction]
// ============================================================================

JitRuntime::JitRuntime() {}
JitRuntime::~JitRuntime() {}

// ============================================================================
// [asmjit::JitRuntime - Interface]
// ============================================================================

Error JitRuntime::add(void** dst, Assembler* assembler) {
  size_t codeSize = assembler->getCodeSize();
  if (codeSize == 0) {
    *dst = NULL;
    return kErrorNoCodeGenerated;
  }

  void* p = _memMgr.alloc(codeSize, getAllocType());
  if (p == NULL) {
    *dst = NULL;
    return kErrorNoVirtualMemory;
  }

  // Relocate the code and release the unused memory back to `VMemMgr`.
  size_t relocSize = assembler->relocCode(p);
  if (relocSize < codeSize) {
    _memMgr.shrink(p, relocSize);
  }

  flush(p, relocSize);
  *dst = p;

  return kErrorOk;
}

Error JitRuntime::release(void* p) {
  return _memMgr.release(p);
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
