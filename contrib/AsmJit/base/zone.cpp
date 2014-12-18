// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/intutil.h"
#include "../base/zone.h"

// [Dependencies - C]
#include <stdarg.h>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! Zero size block used by `Zone` that doesn't have any memory allocated.
static const Zone::Block Zone_zeroBlock = {
  NULL, NULL, NULL, NULL, { 0 }
};

// ============================================================================
// [asmjit::Zone - Construction / Destruction]
// ============================================================================

Zone::Zone(size_t blockSize) {
  _block = const_cast<Zone::Block*>(&Zone_zeroBlock);
  _blockSize = blockSize;
}

Zone::~Zone() {
  reset(true);
}

// ============================================================================
// [asmjit::Zone - Reset]
// ============================================================================

void Zone::reset(bool releaseMemory) {
  Block* cur = _block;

  // Can't be altered.
  if (cur == &Zone_zeroBlock)
    return;

  if (releaseMemory) {
    // Since cur can be in the middle of the double-linked list, we have to
    // traverse to both directions `prev` and `next` separately.
    Block* next = cur->next;
    do {
      Block* prev = cur->prev;
      ASMJIT_FREE(cur);
      cur = prev;
    } while (cur != NULL);

    cur = next;
    while (cur != NULL) {
      next = cur->next;
      ASMJIT_FREE(cur);
      cur = next;
    }

    _block = const_cast<Zone::Block*>(&Zone_zeroBlock);
  }
  else {
    while (cur->prev != NULL)
      cur = cur->prev;

    cur->pos = cur->data;
    _block = cur;
  }
}

// ============================================================================
// [asmjit::Zone - Alloc]
// ============================================================================

void* Zone::_alloc(size_t size) {
  Block* curBlock = _block;
  size_t blockSize = IntUtil::iMax<size_t>(_blockSize, size);

  // The `_alloc()` method can only be called if there is not enough space
  // in the current block, see `alloc()` implementation for more details.
  ASMJIT_ASSERT(curBlock == &Zone_zeroBlock || curBlock->getRemainingSize() < size);

  // If the `Zone` has been reset the current block doesn't have to be the
  // last one. Check if there is a block that can be used instead of allocating
  // a new one. If there is a `next` block it's completely unused, we don't have
  // to check for remaining bytes.
  Block* next = curBlock->next;
  if (next != NULL && next->getBlockSize() >= size) {
    next->pos = next->data + size;
    _block = next;
    return static_cast<void*>(next->data);
  }

  // Prevent arithmetic overflow.
  if (blockSize > ~static_cast<size_t>(0) - sizeof(Block))
    return NULL;

  Block* newBlock = static_cast<Block*>(ASMJIT_ALLOC(sizeof(Block) - sizeof(void*) + blockSize));
  if (newBlock == NULL)
    return NULL;

  newBlock->pos = newBlock->data + size;
  newBlock->end = newBlock->data + blockSize;
  newBlock->prev = NULL;
  newBlock->next = NULL;

  if (curBlock != &Zone_zeroBlock) {
    newBlock->prev = curBlock;
    curBlock->next = newBlock;

    // Does only happen if there is a next block, but the requested memory
    // can't fit into it. In this case a new buffer is allocated and inserted
    // between the current block and the next one.
    if (next != NULL) {
      newBlock->next = next;
      next->prev = newBlock;
    }
  }

  _block = newBlock;
  return static_cast<void*>(newBlock->data);
}

void* Zone::allocZeroed(size_t size) {
  void* p = alloc(size);
  if (p != NULL)
    ::memset(p, 0, size);
  return p;
}

void* Zone::dup(const void* data, size_t size) {
  if (data == NULL)
    return NULL;

  if (size == 0)
    return NULL;

  void* m = alloc(size);
  if (m == NULL)
    return NULL;

  ::memcpy(m, data, size);
  return m;
}

char* Zone::sdup(const char* str) {
  if (str == NULL)
    return NULL;

  size_t len = ::strlen(str);
  if (len == 0)
    return NULL;

  // Include NULL terminator and limit string length.
  if (++len > 256)
    len = 256;

  char* m = static_cast<char*>(alloc(len));
  if (m == NULL)
    return NULL;

  ::memcpy(m, str, len);
  m[len - 1] = '\0';
  return m;
}

char* Zone::sformat(const char* fmt, ...) {
  if (fmt == NULL)
    return NULL;

  char buf[512];
  size_t len;

  va_list ap;
  va_start(ap, fmt);

  len = vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf) - 1, fmt, ap);
  buf[len++] = 0;

  va_end(ap);
  return static_cast<char*>(dup(buf, len));
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
