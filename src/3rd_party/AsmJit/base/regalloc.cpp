// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/regalloc_p.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::RAPass - Construction / Destruction]
// ============================================================================

RAPass::RAPass() noexcept :
  CBPass("RA"),
  _varMapToVaListOffset(0) {}
RAPass::~RAPass() noexcept {}

// ============================================================================
// [asmjit::RAPass - Interface]
// ============================================================================

Error RAPass::process(Zone* zone) noexcept {
  _zone = zone;
  _heap.reset(zone);
  _emitComments = (cb()->getGlobalOptions() & CodeEmitter::kOptionLoggingEnabled) != 0;

  Error err = kErrorOk;
  CBNode* node = cc()->getFirstNode();
  if (!node) return err;

  do {
    if (node->getType() == CBNode::kNodeFunc) {
      CCFunc* func = static_cast<CCFunc*>(node);
      node = func->getEnd();

      err = compile(func);
      if (err) break;
    }

    // Find a function by skipping all nodes that are not `kNodeFunc`.
    do {
      node = node->getNext();
    } while (node && node->getType() != CBNode::kNodeFunc);
  } while (node);

  _heap.reset(nullptr);
  _zone = nullptr;
  return err;
}

Error RAPass::compile(CCFunc* func) noexcept {
  ASMJIT_PROPAGATE(prepare(func));

  Error err;
  do {
    err = fetch();
    if (err) break;

    err = removeUnreachableCode();
    if (err) break;

    err = livenessAnalysis();
    if (err) break;

#if !defined(ASMJIT_DISABLE_LOGGING)
    if (cc()->getGlobalOptions() & CodeEmitter::kOptionLoggingEnabled) {
      err = annotate();
      if (err) break;
    }
#endif // !ASMJIT_DISABLE_LOGGING

    err = translate();
  } while (false);

  cleanup();

  // We alter the compiler cursor, because it doesn't make sense to reference
  // it after compilation - some nodes may disappear and it's forbidden to add
  // new code after the compilation is done.
  cc()->_setCursor(nullptr);
  return err;
}

Error RAPass::prepare(CCFunc* func) noexcept {
  CBNode* end = func->getEnd();

  _func = func;
  _stop = end->getNext();

  _unreachableList.reset();
  _returningList.reset();
  _jccList.reset();
  _contextVd.reset();

  _memVarCells = nullptr;
  _memStackCells = nullptr;

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

  return kErrorOk;
}

void RAPass::cleanup() noexcept {
  VirtReg** virtArray = _contextVd.getData();
  size_t virtCount = _contextVd.getLength();

  for (size_t i = 0; i < virtCount; i++) {
    VirtReg* vreg = virtArray[i];
    vreg->_raId = kInvalidValue;
    vreg->resetPhysId();
  }

  _contextVd.reset();
}

// ============================================================================
// [asmjit::RAPass - Mem]
// ============================================================================

static ASMJIT_INLINE uint32_t RAGetDefaultAlignment(uint32_t size) {
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

RACell* RAPass::_newVarCell(VirtReg* vreg) {
  ASMJIT_ASSERT(vreg->_memCell == nullptr);

  RACell* cell;
  uint32_t size = vreg->getSize();

  if (vreg->isStack()) {
    cell = _newStackCell(size, vreg->getAlignment());
    if (ASMJIT_UNLIKELY(!cell)) return nullptr;
  }
  else {
    cell = static_cast<RACell*>(_zone->alloc(sizeof(RACell)));
    if (!cell) goto _NoMemory;

    cell->next = _memVarCells;
    cell->offset = 0;
    cell->size = size;
    cell->alignment = size;

    _memVarCells = cell;
    _memMaxAlign = std::max<uint32_t>(_memMaxAlign, size);
    _memVarTotal += size;

    switch (size) {
      case  1: _mem1ByteVarsUsed++ ; break;
      case  2: _mem2ByteVarsUsed++ ; break;
      case  4: _mem4ByteVarsUsed++ ; break;
      case  8: _mem8ByteVarsUsed++ ; break;
      case 16: _mem16ByteVarsUsed++; break;
      case 32: _mem32ByteVarsUsed++; break;
      case 64: _mem64ByteVarsUsed++; break;

      default:
        ASMJIT_NOT_REACHED();
    }
  }

  vreg->_memCell = cell;
  return cell;

_NoMemory:
  cc()->setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

RACell* RAPass::_newStackCell(uint32_t size, uint32_t alignment) {
  RACell* cell = static_cast<RACell*>(_zone->alloc(sizeof(RACell)));
  if (ASMJIT_UNLIKELY(!cell)) return nullptr;

  if (alignment == 0)
    alignment = RAGetDefaultAlignment(size);

  if (alignment > 64)
    alignment = 64;

  ASMJIT_ASSERT(Utils::isPowerOf2(alignment));
  size = Utils::alignTo<uint32_t>(size, alignment);

  // Insert it sorted according to the alignment and size.
  {
    RACell** pPrev = &_memStackCells;
    RACell* cur = *pPrev;

    while (cur && ((cur->alignment > alignment) || (cur->alignment == alignment && cur->size > size))) {
      pPrev = &cur->next;
      cur = *pPrev;
    }

    cell->next = cur;
    cell->offset = 0;
    cell->size = size;
    cell->alignment = alignment;

    *pPrev = cell;
    _memStackCellsUsed++;

    _memMaxAlign = std::max<uint32_t>(_memMaxAlign, alignment);
    _memStackTotal += size;
  }

  return cell;
}

Error RAPass::resolveCellOffsets() {
  RACell* varCell = _memVarCells;
  RACell* stackCell = _memStackCells;

  uint32_t pos64 = 0;
  uint32_t pos32 = pos64 + _mem64ByteVarsUsed * 64;
  uint32_t pos16 = pos32 + _mem32ByteVarsUsed * 32;
  uint32_t pos8  = pos16 + _mem16ByteVarsUsed * 16;
  uint32_t pos4  = pos8  + _mem8ByteVarsUsed  * 8 ;
  uint32_t pos2  = pos4  + _mem4ByteVarsUsed  * 4 ;
  uint32_t pos1  = pos2  + _mem2ByteVarsUsed  * 2 ;

  // Assign home slots.
  while (varCell) {
    uint32_t size = varCell->size;
    uint32_t offset = 0;

    switch (size) {
      case  1: offset = pos1 ; pos1  += 1 ; break;
      case  2: offset = pos2 ; pos2  += 2 ; break;
      case  4: offset = pos4 ; pos4  += 4 ; break;
      case  8: offset = pos8 ; pos8  += 8 ; break;
      case 16: offset = pos16; pos16 += 16; break;
      case 32: offset = pos32; pos32 += 32; break;
      case 64: offset = pos64; pos64 += 64; break;

      default:
        ASMJIT_NOT_REACHED();
    }

    varCell->offset = static_cast<int32_t>(offset);
    varCell = varCell->next;
  }

  // Assign stack slots.
  uint32_t stackPos = pos1 + _mem1ByteVarsUsed;
  while (stackCell) {
    uint32_t size = stackCell->size;
    uint32_t alignment = stackCell->alignment;
    ASMJIT_ASSERT(alignment != 0 && Utils::isPowerOf2(alignment));

    stackPos = Utils::alignTo(stackPos, alignment);
    stackCell->offset = stackPos;
    stackCell = stackCell->next;

    stackPos += size;
  }

  _memAllTotal = stackPos;
  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - RemoveUnreachableCode]
// ============================================================================

Error RAPass::removeUnreachableCode() {
  ZoneList<CBNode*>::Link* link = _unreachableList.getFirst();
  CBNode* stop = getStop();

  while (link) {
    CBNode* node = link->getValue();
    if (node && node->getPrev() && node != stop) {
      // Locate all unreachable nodes.
      CBNode* first = node;
      do {
        if (node->hasPassData()) break;
        node = node->getNext();
      } while (node != stop);

      // Remove unreachable nodes that are neither informative nor directives.
      if (node != first) {
        CBNode* end = node;
        node = first;

        // NOTE: The strategy is as follows:
        // 1. The algorithm removes everything until it finds a first label.
        // 2. After the first label is found it removes only removable nodes.
        bool removeEverything = true;
        do {
          CBNode* next = node->getNext();
          bool remove = node->isRemovable();

          if (!remove) {
            if (node->isLabel())
              removeEverything = false;
            remove = removeEverything;
          }

          if (remove)
            cc()->removeNode(node);

          node = next;
        } while (node != end);
      }
    }

    link = link->getNext();
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Liveness Analysis]
// ============================================================================

//! \internal
struct LivenessTarget {
  LivenessTarget* prev;  //!< Previous target.
  CBLabel* node;         //!< Target node.
  CBJump* from;          //!< Jumped from.
};

Error RAPass::livenessAnalysis() {
  uint32_t bLen = static_cast<uint32_t>(
    ((_contextVd.getLength() + RABits::kEntityBits - 1) / RABits::kEntityBits));

  // No variables.
  if (bLen == 0)
    return kErrorOk;

  CCFunc* func = getFunc();
  CBJump* from = nullptr;

  LivenessTarget* ltCur = nullptr;
  LivenessTarget* ltUnused = nullptr;

  ZoneList<CBNode*>::Link* retPtr = _returningList.getFirst();
  ASMJIT_ASSERT(retPtr != nullptr);

  CBNode* node = retPtr->getValue();
  RAData* wd;

  size_t varMapToVaListOffset = _varMapToVaListOffset;
  RABits* bCur = newBits(bLen);
  if (ASMJIT_UNLIKELY(!bCur)) goto NoMem;

  // Allocate bits for code visited first time.
Visit:
  for (;;) {
    wd = node->getPassData<RAData>();
    if (wd->liveness) {
      if (bCur->_addBitsDelSource(wd->liveness, bCur, bLen))
        goto Patch;
      else
        goto Done;
    }

    RABits* bTmp = copyBits(bCur, bLen);
    if (!bTmp) goto NoMem;

    wd = node->getPassData<RAData>();
    wd->liveness = bTmp;

    uint32_t tiedTotal = wd->tiedTotal;
    TiedReg* tiedArray = reinterpret_cast<TiedReg*>(((uint8_t*)wd) + varMapToVaListOffset);

    for (uint32_t i = 0; i < tiedTotal; i++) {
      TiedReg* tied = &tiedArray[i];
      VirtReg* vreg = tied->vreg;

      uint32_t flags = tied->flags;
      uint32_t raId = vreg->_raId;

      if ((flags & TiedReg::kWAll) && !(flags & TiedReg::kRAll)) {
        // Write-Only.
        bTmp->setBit(raId);
        bCur->delBit(raId);
      }
      else {
        // Read-Only or Read/Write.
        bTmp->setBit(raId);
        bCur->setBit(raId);
      }
    }

    if (node->getType() == CBNode::kNodeLabel)
      goto Target;

    if (node == func)
      goto Done;

    ASMJIT_ASSERT(node->getPrev());
    node = node->getPrev();
  }

  // Patch already generated liveness bits.
Patch:
  for (;;) {
    ASMJIT_ASSERT(node->hasPassData());
    ASMJIT_ASSERT(node->getPassData<RAData>()->liveness != nullptr);

    RABits* bNode = node->getPassData<RAData>()->liveness;
    if (!bNode->_addBitsDelSource(bCur, bLen)) goto Done;
    if (node->getType() == CBNode::kNodeLabel) goto Target;

    if (node == func) goto Done;
    node = node->getPrev();
  }

Target:
  if (static_cast<CBLabel*>(node)->getNumRefs() != 0) {
    // Push a new LivenessTarget onto the stack if needed.
    if (!ltCur || ltCur->node != node) {
      // Allocate a new LivenessTarget object (from pool or zone).
      LivenessTarget* ltTmp = ltUnused;

      if (ltTmp) {
        ltUnused = ltUnused->prev;
      }
      else {
        ltTmp = _zone->allocT<LivenessTarget>(
          sizeof(LivenessTarget) - sizeof(RABits) + bLen * sizeof(uintptr_t));
        if (!ltTmp) goto NoMem;
      }

      // Initialize and make current - ltTmp->from will be set later on.
      ltTmp->prev = ltCur;
      ltTmp->node = static_cast<CBLabel*>(node);
      ltCur = ltTmp;

      from = static_cast<CBLabel*>(node)->getFrom();
      ASMJIT_ASSERT(from != nullptr);
    }
    else {
      from = ltCur->from;
      goto JumpNext;
    }

    // Visit/Patch.
    do {
      ltCur->from = from;
      bCur->copyBits(node->getPassData<RAData>()->liveness, bLen);

      if (!from->getPassData<RAData>()->liveness) {
        node = from;
        goto Visit;
      }

      // Issue #25: Moved 'JumpNext' here since it's important to patch
      // code again if there are more live variables than before.
JumpNext:
      if (bCur->delBits(from->getPassData<RAData>()->liveness, bLen)) {
        node = from;
        goto Patch;
      }

      from = from->getJumpNext();
    } while (from);

    // Pop the current LivenessTarget from the stack.
    {
      LivenessTarget* ltTmp = ltCur;
      ltCur = ltCur->prev;
      ltTmp->prev = ltUnused;
      ltUnused = ltTmp;
    }
  }

  bCur->copyBits(node->getPassData<RAData>()->liveness, bLen);
  node = node->getPrev();
  if (node->isJmp() || !node->hasPassData()) goto Done;

  wd = node->getPassData<RAData>();
  if (!wd->liveness) goto Visit;
  if (bCur->delBits(wd->liveness, bLen)) goto Patch;

Done:
  if (ltCur) {
    node = ltCur->node;
    from = ltCur->from;

    goto JumpNext;
  }

  retPtr = retPtr->getNext();
  if (retPtr) {
    node = retPtr->getValue();
    goto Visit;
  }

  return kErrorOk;

NoMem:
  return DebugUtils::errored(kErrorNoHeapMemory);
}

// ============================================================================
// [asmjit::RAPass - Annotate]
// ============================================================================

Error RAPass::formatInlineComment(StringBuilder& dst, CBNode* node) {
#if !defined(ASMJIT_DISABLE_LOGGING)
  RAData* wd = node->getPassData<RAData>();

  if (node->hasInlineComment())
    dst.appendString(node->getInlineComment());

  if (wd && wd->liveness) {
    if (dst.getLength() < _annotationLength)
      dst.appendChars(' ', _annotationLength - dst.getLength());

    uint32_t vdCount = static_cast<uint32_t>(_contextVd.getLength());
    size_t offset = dst.getLength() + 1;

    dst.appendChar('[');
    dst.appendChars(' ', vdCount);
    dst.appendChar(']');
    RABits* liveness = wd->liveness;

    uint32_t i;
    for (i = 0; i < vdCount; i++) {
      if (liveness->getBit(i))
        dst.getData()[offset + i] = '.';
    }

    uint32_t tiedTotal = wd->tiedTotal;
    TiedReg* tiedArray = reinterpret_cast<TiedReg*>(((uint8_t*)wd) + _varMapToVaListOffset);

    for (i = 0; i < tiedTotal; i++) {
      TiedReg* tied = &tiedArray[i];
      VirtReg* vreg = tied->vreg;
      uint32_t flags = tied->flags;

      char c = 'u';
      if ( (flags & TiedReg::kRAll) && !(flags & TiedReg::kWAll)) c = 'r';
      if (!(flags & TiedReg::kRAll) &&  (flags & TiedReg::kWAll)) c = 'w';
      if ( (flags & TiedReg::kRAll) &&  (flags & TiedReg::kWAll)) c = 'x';
      // Uppercase if unused.
      if ( (flags & TiedReg::kUnuse)) c -= 'a' - 'A';

      ASMJIT_ASSERT(offset + vreg->_raId < dst.getLength());
      dst._data[offset + vreg->_raId] = c;
    }
  }
#endif // !ASMJIT_DISABLE_LOGGING

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
