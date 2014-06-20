// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies - AsmJit]
#include "../base/context_p.h"
#include "../base/intutil.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::BaseContext - Construction / Destruction]
// ============================================================================

BaseContext::BaseContext(BaseCompiler* compiler) :
  _compiler(compiler),
  _baseZone(8192 - kZoneOverhead) {

  BaseContext::reset();
}

BaseContext::~BaseContext() {}

// ============================================================================
// [asmjit::BaseContext - Reset]
// ============================================================================

void BaseContext::reset() {
  _baseZone.clear();

  _func = NULL;
  _start = NULL;
  _end = NULL;
  _extraBlock = NULL;
  _stop = NULL;

  _unreachableList.reset();
  _jccList.reset();
  _contextVd.clear();

  _memVarCells = NULL;
  _memStackCells = NULL;

  _mem1ByteVarsUsed = 0;
  _mem2ByteVarsUsed = 0;
  _mem4ByteVarsUsed = 0;
  _mem8ByteVarsUsed = 0;
  _mem16ByteVarsUsed = 0;
  _mem32ByteVarsUsed = 0;
  _mem64ByteVarsUsed = 0;
  _memStackCellsUsed = 0;

  _memMaxAlign = 0;
  _memVarTotal = 0;
  _memStackTotal = 0;
  _memAllTotal = 0;
  _annotationLength = 12;

  _state = NULL;
}

// ============================================================================
// [asmjit::BaseContext - Mem]
// ============================================================================

static ASMJIT_INLINE uint32_t BaseContext_getDefaultAlignment(uint32_t size) {
  if (size > 32)
    return 64;
  else if (size > 16)
    return 32;
  else if (size > 8)
    return 16;
  else if (size > 4)
    return 8;
  else if (size > 2)
    return 4;
  else if (size > 1)
    return 2;
  else
    return 1;
}

MemCell* BaseContext::_newVarCell(VarData* vd) {
  ASMJIT_ASSERT(vd->_memCell == NULL);

  MemCell* cell;
  uint32_t size = vd->getSize();

  if (vd->isStack()) {
    cell = _newStackCell(size, vd->getAlignment());

    if (cell == NULL)
      return NULL;
  }
  else {
    cell = static_cast<MemCell*>(_baseZone.alloc(sizeof(MemCell)));
    if (cell == NULL)
      goto _NoMemory;

    cell->_next = _memVarCells;
    _memVarCells = cell;

    cell->_offset = 0;
    cell->_size = size;
    cell->_alignment = size;

    _memMaxAlign = IntUtil::iMax<uint32_t>(_memMaxAlign, size);
    _memVarTotal += size;

    switch (size) {
      case  1: _mem1ByteVarsUsed++ ; break;
      case  2: _mem2ByteVarsUsed++ ; break;
      case  4: _mem4ByteVarsUsed++ ; break;
      case  8: _mem8ByteVarsUsed++ ; break;
      case 16: _mem16ByteVarsUsed++; break;
      case 32: _mem32ByteVarsUsed++; break;
      case 64: _mem64ByteVarsUsed++; break;
      default: ASMJIT_ASSERT(!"Reached");
    }
  }

  vd->_memCell = cell;
  return cell;

_NoMemory:
  _compiler->setError(kErrorNoHeapMemory);
  return NULL;
}

MemCell* BaseContext::_newStackCell(uint32_t size, uint32_t alignment) {
  MemCell* cell = static_cast<MemCell*>(_baseZone.alloc(sizeof(MemCell)));
  if (cell == NULL)
    goto _NoMemory;

  if (alignment == 0)
    alignment = BaseContext_getDefaultAlignment(size);

  if (alignment > 64)
    alignment = 64;

  ASMJIT_ASSERT(IntUtil::isPowerOf2(alignment));
  size = IntUtil::alignTo<uint32_t>(size, alignment);

  // Insert it sorted according to the alignment and size.
  {
    MemCell** pPrev = &_memStackCells;
    MemCell* cur = *pPrev;

    for (cur = *pPrev; cur != NULL; cur = cur->_next) {
      if (cur->getAlignment() > alignment)
        continue;
      if (cur->getAlignment() == alignment && cur->getSize() > size)
        continue;
      break;
    }

    cell->_next = cur;
    cell->_offset = 0;
    cell->_size = size;
    cell->_alignment = alignment;

    *pPrev = cell;
    _memStackCellsUsed++;

    _memMaxAlign = IntUtil::iMax<uint32_t>(_memMaxAlign, alignment);
    _memStackTotal += size;
  }

  return cell;

_NoMemory:
  _compiler->setError(kErrorNoHeapMemory);
  return NULL;
}

Error BaseContext::resolveCellOffsets() {
  MemCell* varCell = _memVarCells;
  MemCell* stackCell = _memStackCells;

  uint32_t stackAlignment = 0;
  if (stackCell != NULL)
    stackAlignment = stackCell->getAlignment();

  uint32_t pos64 = 0;
  uint32_t pos32 = pos64 + _mem64ByteVarsUsed * 64;
  uint32_t pos16 = pos32 + _mem32ByteVarsUsed * 32;
  uint32_t pos8  = pos16 + _mem16ByteVarsUsed * 16;
  uint32_t pos4  = pos8  + _mem8ByteVarsUsed  * 8 ;
  uint32_t pos2  = pos4  + _mem4ByteVarsUsed  * 4 ;
  uint32_t pos1  = pos2  + _mem2ByteVarsUsed  * 2 ;

  uint32_t stackPos = pos1 + _mem1ByteVarsUsed;

  uint32_t gapAlignment = stackAlignment;
  uint32_t gapSize = 0;

  if (gapAlignment)
    IntUtil::deltaTo(stackPos, gapAlignment);
  stackPos += gapSize;

  uint32_t gapPos = stackPos;
  uint32_t allTotal = stackPos;

  // Vars - Allocated according to alignment/width.
  while (varCell != NULL) {
    uint32_t size = varCell->getSize();
    uint32_t offset = 0;

    switch (size) {
      case  1: offset = pos1 ; pos1  += 1 ; break;
      case  2: offset = pos2 ; pos2  += 2 ; break;
      case  4: offset = pos4 ; pos4  += 4 ; break;
      case  8: offset = pos8 ; pos8  += 8 ; break;
      case 16: offset = pos16; pos16 += 16; break;
      case 32: offset = pos32; pos32 += 32; break;
      case 64: offset = pos64; pos64 += 64; break;
      default: ASMJIT_ASSERT(!"Reached");
    }

    varCell->setOffset(static_cast<int32_t>(offset));
    varCell = varCell->_next;
  }

  // Stack - Allocated according to alignment and width.
  while (stackCell != NULL) {
    uint32_t size = stackCell->getSize();
    uint32_t alignment = stackCell->getAlignment();
    uint32_t offset;

    // Try to fill the gap between variables / stack first.
    if (size <= gapSize && alignment <= gapAlignment) {
      offset = gapPos;

      gapSize -= size;
      gapPos -= size;

      if (alignment < gapAlignment)
        gapAlignment = alignment;
    }
    else {
      offset = stackPos;

      stackPos += size;
      allTotal += size;
    }

    stackCell->setOffset(offset);
    stackCell = stackCell->_next;
  }

  _memAllTotal = allTotal;
  return kErrorOk;
}

// ============================================================================
// [asmjit::BaseContext - RemoveUnreachableCode]
// ============================================================================

Error BaseContext::removeUnreachableCode() {
  PodList<Node*>::Link* link = _unreachableList.getFirst();
  Node* stop = getStop();

  while (link != NULL) {
    Node* node = link->getValue();
    if (node != NULL && node->getPrev() != NULL) {
      // Locate all unreachable nodes.
      Node* first = node;
      do {
        if (node->isFetched())
          break;
        node = node->getNext();
      } while (node != stop);

      // Remove.
      if (node != first) {
        Node* last = (node != NULL) ? node->getPrev() : getCompiler()->getLastNode();
        getCompiler()->removeNodes(first, last);
      }
    }

    link = link->getNext();
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::BaseContext - Cleanup]
// ============================================================================

//! \internal
//!
//! Translate the given function `func`.
void BaseContext::cleanup() {
  VarData** array = _contextVd.getData();
  size_t length = _contextVd.getLength();

  for (size_t i = 0; i < length; i++) {
    VarData* vd = array[i];
    vd->resetContextId();
    vd->resetRegIndex();
  }

  _contextVd.clear();
  _extraBlock = NULL;
}

// ============================================================================
// [asmjit::BaseContext - CompileFunc]
// ============================================================================

Error BaseContext::compile(FuncNode* func) {
  Node* end = func->getEnd();
  Node* stop = end->getNext();

  _func = func;
  _stop = stop;
  _extraBlock = end;

  ASMJIT_PROPAGATE_ERROR(fetch());
  ASMJIT_PROPAGATE_ERROR(removeUnreachableCode());
  ASMJIT_PROPAGATE_ERROR(analyze());

#if !defined(ASMJIT_DISABLE_LOGGER)
  if (_compiler->hasLogger())
    ASMJIT_PROPAGATE_ERROR(annotate());
#endif // !ASMJIT_DISABLE_LOGGER

  ASMJIT_PROPAGATE_ERROR(translate());

  // We alter the compiler cursor, because it doesn't make sense to reference
  // it after compilation - some nodes may disappear and it's forbidden to add
  // new code after the compilation is done.
  _compiler->_setCursor(NULL);

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
