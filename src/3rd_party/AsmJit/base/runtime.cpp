// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/assembler.h"
#include "../base/cpuinfo.h"
#include "../base/runtime.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

static ASMJIT_INLINE void hostFlushInstructionCache(const void* p, size_t size) noexcept {
  // Only useful on non-x86 architectures.
#if !ASMJIT_ARCH_X86 && !ASMJIT_ARCH_X64
# if ASMJIT_OS_WINDOWS
  // Windows has a built-in support in kernel32.dll.
  ::FlushInstructionCache(_memMgr.getProcessHandle(), p, size);
# endif // ASMJIT_OS_WINDOWS
#else
  ASMJIT_UNUSED(p);
  ASMJIT_UNUSED(size);
#endif // !ASMJIT_ARCH_X86 && !ASMJIT_ARCH_X64
}

static ASMJIT_INLINE uint32_t hostDetectNaturalStackAlignment() noexcept {
  // Alignment is assumed to match the pointer-size by default.
  uint32_t alignment = sizeof(intptr_t);

  // X86 & X64
  // ---------
  //
  //   - 32-bit X86 requires stack to be aligned to 4 bytes. Modern Linux, Mac
  //     and UNIX guarantees 16-byte stack alignment even on 32-bit. I'm not
  //     sure about all other UNIX operating systems, because 16-byte alignment
  //!    is addition to an older specification.
  //   - 64-bit X86 requires stack to be aligned to at least 16 bytes.
#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
  int kIsModernOS = ASMJIT_OS_LINUX  || // Linux & ANDROID.
                    ASMJIT_OS_MAC    || // OSX and iOS.
                    ASMJIT_OS_BSD    ;  // BSD variants.
  alignment = ASMJIT_ARCH_X64 || kIsModernOS ? 16 : 4;
#endif

  // ARM32 & ARM64
  // -------------
  //
  //   - 32-bit ARM requires stack to be aligned to 8 bytes.
  //   - 64-bit ARM requires stack to be aligned to 16 bytes.
#if ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64
  alignment = ASMJIT_ARCH_ARM32 ? 8 : 16;
#endif

  return alignment;
}


// ============================================================================
// [asmjit::Runtime - Construction / Destruction]
// ============================================================================

Runtime::Runtime() noexcept
  : _codeInfo(),
    _runtimeType(kRuntimeNone),
    _allocType(VMemMgr::kAllocFreeable) {}
Runtime::~Runtime() noexcept {}

// ============================================================================
// [asmjit::HostRuntime - Construction / Destruction]
// ============================================================================

HostRuntime::HostRuntime() noexcept {
  _runtimeType = kRuntimeJit;

  // Setup the CodeInfo of this Runtime.
  _codeInfo._archInfo       = CpuInfo::getHost().getArchInfo();
  _codeInfo._stackAlignment = static_cast<uint8_t>(hostDetectNaturalStackAlignment());
  _codeInfo._cdeclCallConv  = CallConv::kIdHostCDecl;
  _codeInfo._stdCallConv    = CallConv::kIdHostStdCall;
  _codeInfo._fastCallConv   = CallConv::kIdHostFastCall;
}
HostRuntime::~HostRuntime() noexcept {}

// ============================================================================
// [asmjit::HostRuntime - Interface]
// ============================================================================

void HostRuntime::flush(const void* p, size_t size) noexcept {
  hostFlushInstructionCache(p, size);
}

// ============================================================================
// [asmjit::JitRuntime - Construction / Destruction]
// ============================================================================

JitRuntime::JitRuntime() noexcept {}
JitRuntime::~JitRuntime() noexcept {}

// ============================================================================
// [asmjit::JitRuntime - Interface]
// ============================================================================

Error JitRuntime::_add(void** dst, CodeHolder* code) noexcept {
  size_t codeSize = code->getCodeSize();
  if (ASMJIT_UNLIKELY(codeSize == 0)) {
    *dst = nullptr;
    return DebugUtils::errored(kErrorNoCodeGenerated);
  }

  void* p = _memMgr.alloc(codeSize, getAllocType());
  if (ASMJIT_UNLIKELY(!p)) {
    *dst = nullptr;
    return DebugUtils::errored(kErrorNoVirtualMemory);
  }

  // Relocate the code and release the unused memory back to `VMemMgr`.
  size_t relocSize = code->relocate(p);
  if (ASMJIT_UNLIKELY(relocSize == 0)) {
    *dst = nullptr;
    _memMgr.release(p);
    return DebugUtils::errored(kErrorInvalidState);
  }

  if (relocSize < codeSize)
    _memMgr.shrink(p, relocSize);

  flush(p, relocSize);
  *dst = p;

  return kErrorOk;
}

Error JitRuntime::_release(void* p) noexcept {
  return _memMgr.release(p);
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
