// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_MEMORYMANAGER_H
#define _ASMJIT_MEMORYMANAGER_H

// [Dependencies]
#include "Build.h"
#include "Defs.h"

// [Api-Begin]
#include "ApiBegin.h"

// [Debug]
// #define ASMJIT_MEMORY_MANAGER_DUMP

namespace AsmJit {

//! @addtogroup AsmJit_MemoryManagement
//! @{

// ============================================================================
// [AsmJit::MemoryManager]
// ============================================================================

//! @brief Virtual memory manager interface.
//!
//! This class is pure virtual. You can get default virtual memory manager using
//! @c getGlobal() method. If you want to create more memory managers with same
//! functionality as global memory manager use @c VirtualMemoryManager class.
struct ASMJIT_API MemoryManager
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create memory manager instance.
  MemoryManager() ASMJIT_NOTHROW;
  //! @brief Destroy memory manager instance, this means also to free all memory
  //! blocks.
  virtual ~MemoryManager() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! @brief Allocate a @a size bytes of virtual memory.
  //!
  //! Note that if you are implementing your own virtual memory manager then you
  //! can quitly ignore type of allocation. This is mainly for AsmJit to memory
  //! manager that allocated memory will be never freed.
  virtual void* alloc(sysuint_t size, uint32_t type = MEMORY_ALLOC_FREEABLE) ASMJIT_NOTHROW = 0;
  //! @brief Free previously allocated memory at a given @a address.
  virtual bool free(void* address) ASMJIT_NOTHROW = 0;
  //! @brief Free some tail memory.
  virtual bool shrink(void* address, sysuint_t used) ASMJIT_NOTHROW = 0;
  //! @brief Free all allocated memory.
  virtual void freeAll() ASMJIT_NOTHROW = 0;

  //! @brief Get how many bytes are currently used.
  virtual sysuint_t getUsedBytes() ASMJIT_NOTHROW = 0;
  //! @brief Get how many bytes are currently allocated.
  virtual sysuint_t getAllocatedBytes() ASMJIT_NOTHROW = 0;

  //! @brief Get global memory manager instance.
  //!
  //! Global instance is instance of @c VirtualMemoryManager class. Global memory
  //! manager is used by default by @ref Assembler::make() and @ref Compiler::make()
  //! methods.
  static MemoryManager* getGlobal() ASMJIT_NOTHROW;
};

//! @brief Reference implementation of memory manager that uses
//! @ref AsmJit::VirtualMemory class to allocate chunks of virtual memory
//! and bit arrays to manage it.
struct ASMJIT_API VirtualMemoryManager : public MemoryManager
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a @c VirtualMemoryManager instance.
  VirtualMemoryManager() ASMJIT_NOTHROW;

#if defined(ASMJIT_WINDOWS)
  //! @brief Create a @c VirtualMemoryManager instance for process @a hProcess.
  //!
  //! This is specialized version of constructor available only for windows and
  //! usable to alloc/free memory of different process.
  VirtualMemoryManager(HANDLE hProcess) ASMJIT_NOTHROW;
#endif // ASMJIT_WINDOWS

  //! @brief Destroy the @c VirtualMemoryManager instance, this means also to
  //! free all blocks.
  virtual ~VirtualMemoryManager() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual void* alloc(sysuint_t size, uint32_t type = MEMORY_ALLOC_FREEABLE) ASMJIT_NOTHROW;
  virtual bool free(void* address) ASMJIT_NOTHROW;
  virtual bool shrink(void* address, sysuint_t used) ASMJIT_NOTHROW;
  virtual void freeAll() ASMJIT_NOTHROW;

  virtual sysuint_t getUsedBytes() ASMJIT_NOTHROW;
  virtual sysuint_t getAllocatedBytes() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Virtual Memory Manager Specific]
  // --------------------------------------------------------------------------

  //! @brief Get whether to keep allocated memory after memory manager is
  //! destroyed.
  //!
  //! @sa @c setKeepVirtualMemory().
  bool getKeepVirtualMemory() const ASMJIT_NOTHROW;

  //! @brief Set whether to keep allocated memory after memory manager is
  //! destroyed.
  //!
  //! This method is usable when patching code of remote process. You need to
  //! allocate process memory, store generated assembler into it and patch the
  //! method you want to redirect (into your code). This method affects only
  //! VirtualMemoryManager destructor. After destruction all internal 
  //! structures are freed, only the process virtual memory remains.
  //! 
  //! @note Memory allocated with MEMORY_ALLOC_PERMANENT is always kept.
  //!
  //! @sa @c getKeepVirtualMemory().
  void setKeepVirtualMemory(bool keepVirtualMemory) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Debug]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_MEMORY_MANAGER_DUMP)
  //! @brief Dump memory manager tree into file.
  //!
  //! Generated output is using DOT language (from graphviz package).
  void dump(const char* fileName);
#endif // ASMJIT_MEMORY_MANAGER_DUMP

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Pointer to private data hidden from the public API.
  void* _d;
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_MEMORYMANAGER_H
