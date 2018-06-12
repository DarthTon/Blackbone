// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_VMEM_H
#define _ASMJIT_BASE_VMEM_H

// [Dependencies]
#include "../base/globals.h"
#include "../base/osutils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::VMemMgr]
// ============================================================================

//! Reference implementation of memory manager that uses `VMemUtil` to allocate
//! chunks of virtual memory and bit arrays to manage it.
class VMemMgr {
public:
  //! Type of virtual memory allocation, see `VMemMgr::alloc()`.
  ASMJIT_ENUM(AllocType) {
    //! Normal memory allocation, has to be freed by `VMemMgr::release()`.
    kAllocFreeable = 0,
    //! Allocate permanent memory, can't be freed.
    kAllocPermanent = 1
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

#if !ASMJIT_OS_WINDOWS
  //! Create a `VMemMgr` instance.
  ASMJIT_API VMemMgr() noexcept;
#else
  //! Create a `VMemMgr` instance.
  //!
  //! NOTE: When running on Windows it's possible to specify a `hProcess` to
  //! be used for memory allocation. Using `hProcess` allows to allocate memory
  //! of a remote process.
  ASMJIT_API VMemMgr(HANDLE hProcess = static_cast<HANDLE>(0)) noexcept;
#endif // ASMJIT_OS_WINDOWS

  //! Destroy the `VMemMgr` instance and free all blocks.
  ASMJIT_API ~VMemMgr() noexcept;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Free all allocated memory.
  ASMJIT_API void reset() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

#if ASMJIT_OS_WINDOWS
  //! Get the handle of the process memory manager is bound to.
  ASMJIT_INLINE HANDLE getProcessHandle() const noexcept { return _hProcess; }
#endif // ASMJIT_OS_WINDOWS

  //! Get how many bytes are currently allocated.
  ASMJIT_INLINE size_t getAllocatedBytes() const noexcept { return _allocatedBytes; }
  //! Get how many bytes are currently used.
  ASMJIT_INLINE size_t getUsedBytes() const noexcept { return _usedBytes; }

  //! Get whether to keep allocated memory after the `VMemMgr` is destroyed.
  //!
  //! \sa \ref setKeepVirtualMemory.
  ASMJIT_INLINE bool getKeepVirtualMemory() const noexcept { return _keepVirtualMemory; }
  //! Set whether to keep allocated memory after the memory manager is destroyed.
  //!
  //! This method is usable when patching code of remote process. You need to
  //! allocate process memory, store generated assembler into it and patch the
  //! method you want to redirect (into your code). This method affects only
  //! VMemMgr destructor. After destruction all internal
  //! structures are freed, only the process virtual memory remains.
  //!
  //! NOTE: Memory allocated with kAllocPermanent is always kept.
  //!
  //! \sa \ref getKeepVirtualMemory.
  ASMJIT_INLINE void setKeepVirtualMemory(bool val) noexcept { _keepVirtualMemory = val; }

  // --------------------------------------------------------------------------
  // [Alloc / Release]
  // --------------------------------------------------------------------------

  //! Allocate a `size` bytes of virtual memory.
  //!
  //! Note that if you are implementing your own virtual memory manager then you
  //! can quitly ignore type of allocation. This is mainly for AsmJit to memory
  //! manager that allocated memory will be never freed.
  ASMJIT_API void* alloc(size_t size, uint32_t type = kAllocFreeable) noexcept;
  //! Free previously allocated memory at a given `address`.
  ASMJIT_API Error release(void* p) noexcept;
  //! Free extra memory allocated with `p`.
  ASMJIT_API Error shrink(void* p, size_t used) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

#if ASMJIT_OS_WINDOWS
  HANDLE _hProcess;                      //!< Process passed to `VirtualAllocEx` and `VirtualFree`.
#endif // ASMJIT_OS_WINDOWS
  Lock _lock;                            //!< Lock to enable thread-safe functionality.

  size_t _blockSize;                     //!< Default block size.
  size_t _blockDensity;                  //!< Default block density.
  bool _keepVirtualMemory;               //!< Keep virtual memory after destroyed.

  size_t _allocatedBytes;                //!< How many bytes are currently allocated.
  size_t _usedBytes;                     //!< How many bytes are currently used.

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
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_VMEM_H
