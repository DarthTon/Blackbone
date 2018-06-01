// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_VMEM_H
#define _ASMJIT_BASE_VMEM_H

// [Dependencies]
#include "../base/error.h"
#include "../base/lock.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_util
//! \{

// ============================================================================
// [asmjit::kVMemAlloc]
// ============================================================================

//! Type of virtual memory allocation, see `VMemMgr::alloc()`.
ASMJIT_ENUM(kVMemAlloc) {
  //! Normal memory allocation, has to be freed by `VMemMgr::release()`.
  kVMemAllocFreeable = 0,
  //! Allocate permanent memory, can't be freed.
  kVMemAllocPermanent = 1
};

// ============================================================================
// [asmjit::kVMemFlags]
// ============================================================================

//! Type of virtual memory allocation, see `VMemMgr::alloc()`.
ASMJIT_ENUM(kVMemFlags) {
  //! Memory is writable.
  kVMemFlagWritable = 0x00000001,
  //! Memory is executable.
  kVMemFlagExecutable = 0x00000002
};

// ============================================================================
// [asmjit::VMemUtil]
// ============================================================================

//! Virtual memory utilities.
//!
//! Defines functions that provide facility to allocate and free memory that is
//! executable in a platform independent manner. If both the processor and host
//! operating system support data-execution-prevention then the only way how to
//! run machine code is to allocate it to a memory that has marked as executable.
//! VMemUtil is just unified interface to platform dependent APIs.
//!
//! `VirtualAlloc()` function is used on Windows operating system and `mmap()`
//! on POSIX. `VirtualAlloc()` and `mmap()` documentation provide a detailed
//! overview on how to use a platform specific APIs.
struct VMemUtil {
  //! Get a size/alignment of a single virtual memory page.
  static ASMJIT_API size_t getPageSize();

  //! Get a recommended granularity for a single `alloc` call.
  static ASMJIT_API size_t getPageGranularity();

  //! Allocate virtual memory.
  //!
  //! Pages are readable/writeable, but they are not guaranteed to be
  //! executable unless 'canExecute' is true. Returns the address of
  //! allocated memory, or NULL on failure.
  static ASMJIT_API void* alloc(size_t length, size_t* allocated, uint32_t flags);

#if defined(ASMJIT_OS_WINDOWS)
  //! Allocate virtual memory of `hProcess`.
  //!
  //! \note This function is Windows specific.
  static ASMJIT_API void* allocProcessMemory(HANDLE hProcess, size_t length, size_t* allocated, uint32_t flags);
#endif // ASMJIT_OS_WINDOWS

  //! Free memory allocated by `alloc()`.
  static ASMJIT_API Error release(void* addr, size_t length);

#if defined(ASMJIT_OS_WINDOWS)
  //! Release virtual memory of `hProcess`.
  //!
  //! \note This function is Windows specific.
  static ASMJIT_API Error releaseProcessMemory(HANDLE hProcess, void* addr, size_t length);
#endif // ASMJIT_OS_WINDOWS
};

// ============================================================================
// [asmjit::VMemMgr]
// ============================================================================

//! Reference implementation of memory manager that uses `VMemUtil` to allocate
//! chunks of virtual memory and bit arrays to manage it.
struct VMemMgr {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_OS_WINDOWS)
  //! Create a `VMemMgr` instance.
  ASMJIT_API VMemMgr();
#else
  //! Create a `VMemMgr` instance.
  //!
  //! \note When running on Windows it's possible to specify a `hProcess` to
  //! be used for memory allocation. This allows to allocate memory of remote
  //! process.
  ASMJIT_API VMemMgr(HANDLE hProcess = static_cast<HANDLE>(0));
#endif // ASMJIT_OS_WINDOWS

  //! Destroy the `VMemMgr` instance and free all blocks.
  ASMJIT_API ~VMemMgr();

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Free all allocated memory.
  ASMJIT_API void reset();

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_OS_WINDOWS)
  //! Get the handle of the process memory manager is bound to.
  ASMJIT_INLINE HANDLE getProcessHandle() const {
    return _hProcess;
  }
#endif // ASMJIT_OS_WINDOWS

  //! Get how many bytes are currently allocated.
  ASMJIT_INLINE size_t getAllocatedBytes() const {
    return _allocatedBytes;
  }

  //! Get how many bytes are currently used.
  ASMJIT_INLINE size_t getUsedBytes() const {
    return _usedBytes;
  }

  //! Get whether to keep allocated memory after the `VMemMgr` is destroyed.
  //!
  //! \sa \ref setKeepVirtualMemory.
  ASMJIT_INLINE bool getKeepVirtualMemory() const {
    return _keepVirtualMemory;
  }

  //! Set whether to keep allocated memory after memory manager is
  //! destroyed.
  //!
  //! This method is usable when patching code of remote process. You need to
  //! allocate process memory, store generated assembler into it and patch the
  //! method you want to redirect (into your code). This method affects only
  //! VMemMgr destructor. After destruction all internal
  //! structures are freed, only the process virtual memory remains.
  //!
  //! \note Memory allocated with kVMemAllocPermanent is always kept.
  //!
  //! \sa \ref getKeepVirtualMemory.
  ASMJIT_INLINE void setKeepVirtualMemory(bool keepVirtualMemory) {
    _keepVirtualMemory = keepVirtualMemory;
  }

  // --------------------------------------------------------------------------
  // [Alloc / Release]
  // --------------------------------------------------------------------------

  //! Allocate a `size` bytes of virtual memory.
  //!
  //! Note that if you are implementing your own virtual memory manager then you
  //! can quitly ignore type of allocation. This is mainly for AsmJit to memory
  //! manager that allocated memory will be never freed.
  ASMJIT_API void* alloc(size_t size, uint32_t type = kVMemAllocFreeable);

  //! Free previously allocated memory at a given `address`.
  ASMJIT_API Error release(void* p);

  //! Free extra memory allocated with `p`.
  ASMJIT_API Error shrink(void* p, size_t used);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_OS_WINDOWS)
  //! Process passed to `VirtualAllocEx` and `VirtualFree`.
  HANDLE _hProcess;
#endif // ASMJIT_OS_WINDOWS

  //! Lock to enable thread-safe functionality.
  Lock _lock;

  //! Default block size.
  size_t _blockSize;
  //! Default block density.
  size_t _blockDensity;

  // Whether to keep virtual memory after destroy.
  bool _keepVirtualMemory;

  //! How many bytes are currently allocated.
  size_t _allocatedBytes;
  //! How many bytes are currently used.
  size_t _usedBytes;

  //! \internal
  //! \{

  struct RbNode;
  struct MemNode;
  struct PermanentNode;

  // Memory nodes root.
  MemNode* _root;
  // Memory nodes list.
  MemNode* _first;
  MemNode* _last;
  MemNode* _optimal;
  // Permanent memory.
  PermanentNode* _permanent;

  //! \}
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_VMEM_H
