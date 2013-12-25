// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_MEMORYMARKER_H
#define _ASMJIT_MEMORYMARKER_H

// [Dependencies]
#include "Build.h"
#include "Defs.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_MemoryManagement
//! @{

// ============================================================================
// [AsmJit::MemoryMarker]
// ============================================================================

//! @brief Virtual memory marker interface.
struct ASMJIT_API MemoryMarker
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  MemoryMarker() ASMJIT_NOTHROW;
  virtual ~MemoryMarker() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual void mark(const void* ptr, sysuint_t size) ASMJIT_NOTHROW = 0;

private:
  ASMJIT_DISABLE_COPY(MemoryMarker)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_MEMORYMARKER_H
