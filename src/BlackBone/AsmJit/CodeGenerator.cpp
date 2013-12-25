// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies]
#include "Assembler.h"
#include "CodeGenerator.h"
#include "Defs.h"
#include "MemoryManager.h"
#include "MemoryMarker.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::CodeGenerator - Construction / Destruction]
// ============================================================================

CodeGenerator::CodeGenerator()
{
}

CodeGenerator::~CodeGenerator()
{
}

// ============================================================================
// [AsmJit::CodeGenerator - GetGlobal]
// ============================================================================

JitCodeGenerator* CodeGenerator::getGlobal()
{
  static JitCodeGenerator global;
  return &global;
}

// ============================================================================
// [AsmJit::JitCodeGenerator - Construction / Destruction]
// ============================================================================

JitCodeGenerator::JitCodeGenerator() :
  _memoryManager(NULL),
  _memoryMarker(NULL),
  _allocType(MEMORY_ALLOC_FREEABLE)
{
}

JitCodeGenerator::~JitCodeGenerator()
{
}

// ============================================================================
// [AsmJit::JitCodeGenerator - Generate]
// ============================================================================

uint32_t JitCodeGenerator::generate(void** dest, Assembler* assembler)
{
  // Disallow empty code generation.
  sysuint_t codeSize = assembler->getCodeSize();
  if (codeSize == 0)
  {
    *dest = NULL;
    return AsmJit::ERROR_NO_FUNCTION;
  }

  // Switch to global memory manager if not provided.
  MemoryManager* memmgr = getMemoryManager();

  if (memmgr == NULL)
  {
    memmgr = MemoryManager::getGlobal();
  }

  void* p = memmgr->alloc(codeSize, getAllocType());
  if (p == NULL)
  {
    *dest = NULL;
    return ERROR_NO_VIRTUAL_MEMORY;
  }

  // Relocate the code.
  sysuint_t relocatedSize = assembler->relocCode(p);

  // Return unused memory to MemoryManager.
  if (relocatedSize < codeSize)
  {
    memmgr->shrink(p, relocatedSize);
  }

  // Mark memory if MemoryMarker provided.
  if (_memoryMarker)
  {
    _memoryMarker->mark(p, relocatedSize);
  }

  // Return the code.
  *dest = p;
  return ERROR_NONE;
}

} // AsmJit namespace
