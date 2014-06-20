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
  //! Get the alignment guaranteed by alloc().
  static ASMJIT_API size_t getAlignment();

  //! Get size of the single page.
  static ASMJIT_API size_t getPageSize();

  //! Allocate virtual memory.
  //!
  //! Pages are readable/writeable, but they are not guaranteed to be
  //! executable unless 'canExecute' is true. Returns the address of
  //! allocated memory, or NULL on failure.
  static ASMJIT_API void* alloc(size_t length, size_t* allocated, uint32_t flags);

  //! Free memory allocated by `alloc()`.
  static ASMJIT_API void release(void* addr, size_t length);

#if defined(ASMJIT_OS_WINDOWS)
  //! Allocate virtual memory of `hProcess`.
  //!
  //! \note This function is Windows specific.
  static ASMJIT_API void* allocProcessMemory(HANDLE hProcess, size_t length, size_t* allocated, uint32_t flags);

  //! Free virtual memory of `hProcess`.
  //!
  //! \note This function is Windows specific.
  static ASMJIT_API void releaseProcessMemory(HANDLE hProcess, void* addr, size_t length);
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

  //! Create a `VMemMgr` instance.
  ASMJIT_API VMemMgr();

#if defined(ASMJIT_OS_WINDOWS)
  //! Create a `VMemMgr` instance for `hProcess`.
  //!
  //! This is a specialized version of constructor available only for windows
  //! and usable to alloc/free memory of a different process.
  explicit ASMJIT_API VMemMgr(HANDLE hProcess);
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
  ASMJIT_API HANDLE getProcessHandle() const;
#endif // ASMJIT_OS_WINDOWS

  //! Get how many bytes are currently used.
  ASMJIT_API size_t getUsedBytes() const;
  //! Get how many bytes are currently allocated.
  ASMJIT_API size_t getAllocatedBytes() const;

  //! Get whether to keep allocated memory after the `VMemMgr` is destroyed.
  //!
  //! \sa \ref setKeepVirtualMemory.
  ASMJIT_API bool getKeepVirtualMemory() const;

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
  ASMJIT_API void setKeepVirtualMemory(bool keepVirtualMemory);

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
  ASMJIT_API Error release(void* address);

  //! Free some tail memory.
  ASMJIT_API Error shrink(void* address, size_t used);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Pointer to a private data hidden from the public API.
  void* _d;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_VMEM_H
