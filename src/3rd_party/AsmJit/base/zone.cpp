// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! Zero size block used by `Zone` that doesn't have any memory allocated.
static const Zone::Block Zone_zeroBlock = { nullptr, nullptr, 0, { 0 } };

static ASMJIT_INLINE uint32_t Zone_getAlignmentOffsetFromAlignment(uint32_t x) noexcept {
  switch (x) {
    default: return 0;
    case 0 : return 0;
    case 1 : return 0;
    case 2 : return 1;
    case 4 : return 2;
    case 8 : return 3;
    case 16: return 4;
    case 32: return 5;
    case 64: return 6;
  }
}

// ============================================================================
// [asmjit::Zone - Construction / Destruction]
// ============================================================================

Zone::Zone(uint32_t blockSize, uint32_t blockAlignment) noexcept
  : _ptr(nullptr),
    _end(nullptr),
    _block(const_cast<Zone::Block*>(&Zone_zeroBlock)),
    _blockSize(blockSize),
    _blockAlignmentShift(Zone_getAlignmentOffsetFromAlignment(blockAlignment)) {}

Zone::~Zone() noexcept {
  reset(true);
}

// ============================================================================
// [asmjit::Zone - Reset]
// ============================================================================

void Zone::reset(bool releaseMemory) noexcept {
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
      Internal::releaseMemory(cur);
      cur = prev;
    } while (cur);

    cur = next;
    while (cur) {
      next = cur->next;
      Internal::releaseMemory(cur);
      cur = next;
    }

    _ptr = nullptr;
    _end = nullptr;
    _block = const_cast<Zone::Block*>(&Zone_zeroBlock);
  }
  else {
    while (cur->prev)
      cur = cur->prev;

    _ptr = cur->data;
    _end = _ptr + cur->size;
    _block = cur;
  }
}

// ============================================================================
// [asmjit::Zone - Alloc]
// ============================================================================

void* Zone::_alloc(size_t size) noexcept {
  Block* curBlock = _block;
  uint8_t* p;

  size_t blockSize = std::max<size_t>(_blockSize, size);
  size_t blockAlignment = getBlockAlignment();

  // The `_alloc()` method can only be called if there is not enough space
  // in the current block, see `alloc()` implementation for more details.
  ASMJIT_ASSERT(curBlock == &Zone_zeroBlock || getRemainingSize() < size);

  // If the `Zone` has been cleared the current block doesn't have to be the
  // last one. Check if there is a block that can be used instead of allocating
  // a new one. If there is a `next` block it's completely unused, we don't have
  // to check for remaining bytes.
  Block* next = curBlock->next;
  if (next && next->size >= size) {
    p = Utils::alignTo(next->data, blockAlignment);

    _block = next;
    _ptr = p + size;
    _end = next->data + next->size;

    return static_cast<void*>(p);
  }

  // Prevent arithmetic overflow.
  if (ASMJIT_UNLIKELY(blockSize > (~static_cast<size_t>(0) - sizeof(Block) - blockAlignment)))
    return nullptr;

  blockSize += blockAlignment;
  Block* newBlock = static_cast<Block*>(Internal::allocMemory(sizeof(Block) + blockSize));

  if (ASMJIT_UNLIKELY(!newBlock))
    return nullptr;

  // Align the pointer to `blockAlignment` and adjust the size of this block
  // accordingly. It's the same as using `blockAlignment - Utils::alignDiff()`,
  // just written differently.
  p = Utils::alignTo(newBlock->data, blockAlignment);
  newBlock->prev = nullptr;
  newBlock->next = nullptr;
  newBlock->size = blockSize;

  if (curBlock != &Zone_zeroBlock) {
    newBlock->prev = curBlock;
    curBlock->next = newBlock;

    // Does only happen if there is a next block, but the requested memory
    // can't fit into it. In this case a new buffer is allocated and inserted
    // between the current block and the next one.
    if (next) {
      newBlock->next = next;
      next->prev = newBlock;
    }
  }

  _block = newBlock;
  _ptr = p + size;
  _end = newBlock->data + blockSize;

  return static_cast<void*>(p);
}

void* Zone::allocZeroed(size_t size) noexcept {
  void* p = alloc(size);
  if (ASMJIT_UNLIKELY(!p)) return p;
  return ::memset(p, 0, size);
}

void* Zone::dup(const void* data, size_t size, bool nullTerminate) noexcept {
  if (ASMJIT_UNLIKELY(!data || !size)) return nullptr;

  ASMJIT_ASSERT(size != IntTraits<size_t>::maxValue());
  uint8_t* m = allocT<uint8_t>(size + nullTerminate);
  if (ASMJIT_UNLIKELY(!m)) return nullptr;

  ::memcpy(m, data, size);
  if (nullTerminate) m[size] = '\0';

  return static_cast<void*>(m);
}

char* Zone::sformat(const char* fmt, ...) noexcept {
  if (ASMJIT_UNLIKELY(!fmt)) return nullptr;

  char buf[512];
  size_t len;

  va_list ap;
  va_start(ap, fmt);

  len = vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf) - 1, fmt, ap);
  buf[len++] = 0;

  va_end(ap);
  return static_cast<char*>(dup(buf, len));
}

// ============================================================================
// [asmjit::ZoneHeap - Helpers]
// ============================================================================

static bool ZoneHeap_hasDynamicBlock(ZoneHeap* self, ZoneHeap::DynamicBlock* block) noexcept {
  ZoneHeap::DynamicBlock* cur = self->_dynamicBlocks;
  while (cur) {
    if (cur == block)
      return true;
    cur = cur->next;
  }
  return false;
}

// ============================================================================
// [asmjit::ZoneHeap - Init / Reset]
// ============================================================================

void ZoneHeap::reset(Zone* zone) noexcept {
  // Free dynamic blocks.
  DynamicBlock* block = _dynamicBlocks;
  while (block) {
    DynamicBlock* next = block->next;
    Internal::releaseMemory(block);
    block = next;
  }

  // Zero the entire class and initialize to the given `zone`.
  ::memset(this, 0, sizeof(*this));
  _zone = zone;
}

// ============================================================================
// [asmjit::ZoneHeap - Alloc / Release]
// ============================================================================

void* ZoneHeap::_alloc(size_t size, size_t& allocatedSize) noexcept {
  ASMJIT_ASSERT(isInitialized());

  // We use our memory pool only if the requested block is of a reasonable size.
  uint32_t slot;
  if (_getSlotIndex(size, slot, allocatedSize)) {
    // Slot reuse.
    uint8_t* p = reinterpret_cast<uint8_t*>(_slots[slot]);
    size = allocatedSize;

    if (p) {
      _slots[slot] = reinterpret_cast<Slot*>(p)->next;
      //printf("ALLOCATED %p of size %d (SLOT %d)\n", p, int(size), slot);
      return p;
    }

    // So use Zone to allocate a new chunk for us. But before we use it, we
    // check if there is enough room for the new chunk in zone, and if not,
    // we redistribute the remaining memory in Zone's current block into slots.
    Zone* zone = _zone;
    p = Utils::alignTo(zone->getCursor(), kBlockAlignment);
    size_t remain = (p <= zone->getEnd()) ? (size_t)(zone->getEnd() - p) : size_t(0);

    if (ASMJIT_LIKELY(remain >= size)) {
      zone->setCursor(p + size);
      //printf("ALLOCATED %p of size %d (SLOT %d)\n", p, int(size), slot);
      return p;
    }
    else {
      // Distribute the remaining memory to suitable slots.
      if (remain >= kLoGranularity) {
        do {
          size_t distSize = std::min<size_t>(remain, kLoMaxSize);
          uint32_t distSlot = static_cast<uint32_t>((distSize - kLoGranularity) / kLoGranularity);
          ASMJIT_ASSERT(distSlot < kLoCount);

          reinterpret_cast<Slot*>(p)->next = _slots[distSlot];
          _slots[distSlot] = reinterpret_cast<Slot*>(p);

          p += distSize;
          remain -= distSize;
        } while (remain >= kLoGranularity);
        zone->setCursor(p);
      }

      p = static_cast<uint8_t*>(zone->_alloc(size));
      if (ASMJIT_UNLIKELY(!p)) {
        allocatedSize = 0;
        return nullptr;
      }

      //printf("ALLOCATED %p of size %d (SLOT %d)\n", p, int(size), slot);
      return p;
    }
  }
  else {
    // Allocate a dynamic block.
    size_t overhead = sizeof(DynamicBlock) + sizeof(DynamicBlock*) + kBlockAlignment;

    // Handle a possible overflow.
    if (ASMJIT_UNLIKELY(overhead >= ~static_cast<size_t>(0) - size))
      return nullptr;

    void* p = Internal::allocMemory(size + overhead);
    if (ASMJIT_UNLIKELY(!p)) {
      allocatedSize = 0;
      return nullptr;
    }

    // Link as first in `_dynamicBlocks` double-linked list.
    DynamicBlock* block = static_cast<DynamicBlock*>(p);
    DynamicBlock* next = _dynamicBlocks;

    if (next)
      next->prev = block;

    block->prev = nullptr;
    block->next = next;
    _dynamicBlocks = block;

    // Align the pointer to the guaranteed alignment and store `DynamicBlock`
    // at the end of the memory block, so `_releaseDynamic()` can find it.
    p = Utils::alignTo(static_cast<uint8_t*>(p) + sizeof(DynamicBlock) + sizeof(DynamicBlock*), kBlockAlignment);
    reinterpret_cast<DynamicBlock**>(p)[-1] = block;

    allocatedSize = size;
    //printf("ALLOCATED DYNAMIC %p of size %d\n", p, int(size));
    return p;
  }
}

void* ZoneHeap::_allocZeroed(size_t size, size_t& allocatedSize) noexcept {
  ASMJIT_ASSERT(isInitialized());

  void* p = _alloc(size, allocatedSize);
  if (ASMJIT_UNLIKELY(!p)) return p;
  return ::memset(p, 0, allocatedSize);
}

void ZoneHeap::_releaseDynamic(void* p, size_t size) noexcept {
  ASMJIT_ASSERT(isInitialized());
  //printf("RELEASING DYNAMIC %p of size %d\n", p, int(size));

  // Pointer to `DynamicBlock` is stored at [-1].
  DynamicBlock* block = reinterpret_cast<DynamicBlock**>(p)[-1];
  ASMJIT_ASSERT(ZoneHeap_hasDynamicBlock(this, block));

  // Unlink and free.
  DynamicBlock* prev = block->prev;
  DynamicBlock* next = block->next;

  if (prev)
    prev->next = next;
  else
    _dynamicBlocks = next;

  if (next)
    next->prev = prev;

  Internal::releaseMemory(block);
}

// ============================================================================
// [asmjit::ZoneVectorBase - Helpers]
// ============================================================================

Error ZoneVectorBase::_grow(ZoneHeap* heap, size_t sizeOfT, size_t n) noexcept {
  size_t threshold = Globals::kAllocThreshold / sizeOfT;
  size_t capacity = _capacity;
  size_t after = _length;

  if (ASMJIT_UNLIKELY(IntTraits<size_t>::maxValue() - n < after))
    return DebugUtils::errored(kErrorNoHeapMemory);

  after += n;
  if (capacity >= after)
    return kErrorOk;

  // ZoneVector is used as an array to hold short-lived data structures used
  // during code generation. The growing strategy is simple - use small capacity
  // at the beginning (very good for ZoneHeap) and then grow quicker to prevent
  // successive reallocations.
  if (capacity < 4)
    capacity = 4;
  else if (capacity < 8)
    capacity = 8;
  else if (capacity < 16)
    capacity = 16;
  else if (capacity < 64)
    capacity = 64;
  else if (capacity < 256)
    capacity = 256;

  while (capacity < after) {
    if (capacity < threshold)
      capacity *= 2;
    else
      capacity += threshold;
  }

  return _reserve(heap, sizeOfT, capacity);
}

Error ZoneVectorBase::_reserve(ZoneHeap* heap, size_t sizeOfT, size_t n) noexcept {
  size_t oldCapacity = _capacity;
  if (oldCapacity >= n) return kErrorOk;

  size_t nBytes = n * sizeOfT;
  if (ASMJIT_UNLIKELY(nBytes < n))
    return DebugUtils::errored(kErrorNoHeapMemory);

  size_t allocatedBytes;
  uint8_t* newData = static_cast<uint8_t*>(heap->alloc(nBytes, allocatedBytes));

  if (ASMJIT_UNLIKELY(!newData))
    return DebugUtils::errored(kErrorNoHeapMemory);

  void* oldData = _data;
  if (_length)
    ::memcpy(newData, oldData, _length * sizeOfT);

  if (oldData)
    heap->release(oldData, oldCapacity * sizeOfT);

  _capacity = allocatedBytes / sizeOfT;
  ASMJIT_ASSERT(_capacity >= n);

  _data = newData;
  return kErrorOk;
}

Error ZoneVectorBase::_resize(ZoneHeap* heap, size_t sizeOfT, size_t n) noexcept {
  size_t length = _length;
  if (_capacity < n) {
    ASMJIT_PROPAGATE(_grow(heap, sizeOfT, n - length));
    ASMJIT_ASSERT(_capacity >= n);
  }

  if (length < n)
    ::memset(static_cast<uint8_t*>(_data) + length * sizeOfT, 0, (n - length) * sizeOfT);

  _length = n;
  return kErrorOk;
}

// ============================================================================
// [asmjit::ZoneBitVector - Ops]
// ============================================================================

Error ZoneBitVector::_resize(ZoneHeap* heap, size_t newLength, size_t idealCapacity, bool newBitsValue) noexcept {
  ASMJIT_ASSERT(idealCapacity >= newLength);

  if (newLength <= _length) {
    // The size after the resize is lesser than or equal to the current length.
    size_t idx = newLength / kBitsPerWord;
    size_t bit = newLength % kBitsPerWord;

    // Just set all bits outside of the new length in the last word to zero.
    // There is a case that there are not bits to set if `bit` is zero. This
    // happens when `newLength` is a multiply of `kBitsPerWord` like 64, 128,
    // and so on. In that case don't change anything as that would mean settings
    // bits outside of the `_length`.
    if (bit)
      _data[idx] &= (static_cast<uintptr_t>(1) << bit) - 1U;

    _length = newLength;
    return kErrorOk;
  }

  size_t oldLength = _length;
  BitWord* data = _data;

  if (newLength > _capacity) {
    // Realloc needed... Calculate the minimum capacity (in bytes) requied.
    size_t minimumCapacityInBits = Utils::alignTo<size_t>(idealCapacity, kBitsPerWord);
    size_t allocatedCapacity;

    if (ASMJIT_UNLIKELY(minimumCapacityInBits < newLength))
      return DebugUtils::errored(kErrorNoHeapMemory);

    // Normalize to bytes.
    size_t minimumCapacity = minimumCapacityInBits / 8;
    BitWord* newData = static_cast<BitWord*>(heap->alloc(minimumCapacity, allocatedCapacity));

    if (ASMJIT_UNLIKELY(!newData))
      return DebugUtils::errored(kErrorNoHeapMemory);

    // `allocatedCapacity` now contains number in bytes, we need bits.
    size_t allocatedCapacityInBits = allocatedCapacity * 8;

    // Arithmetic overflow should normally not happen. If it happens we just
    // change the `allocatedCapacityInBits` to the `minimumCapacityInBits` as
    // this value is still safe to be used to call `_heap->release(...)`.
    if (ASMJIT_UNLIKELY(allocatedCapacityInBits < allocatedCapacity))
      allocatedCapacityInBits = minimumCapacityInBits;

    if (oldLength)
      ::memcpy(newData, data, _wordsPerBits(oldLength));

    if (data)
      heap->release(data, _capacity / 8);
    data = newData;

    _data = data;
    _capacity = allocatedCapacityInBits;
  }

  // Start (of the old length) and end (of the new length) bits
  size_t idx = oldLength / kBitsPerWord;
  size_t startBit = oldLength % kBitsPerWord;
  size_t endBit = newLength % kBitsPerWord;

  // Set new bits to either 0 or 1. The `pattern` is used to set multiple
  // bits per bit-word and contains either all zeros or all ones.
  BitWord pattern = _patternFromBit(newBitsValue);

  // First initialize the last bit-word of the old length.
  if (startBit) {
    size_t nBits = 0;

    if (idx == (newLength / kBitsPerWord)) {
      // The number of bit-words is the same after the resize. In that case
      // we need to set only bits necessary in the current last bit-word.
      ASMJIT_ASSERT(startBit < endBit);
      nBits = endBit - startBit;
    }
    else {
      // There is be more bit-words after the resize. In that case we don't
      // have to be extra careful about the last bit-word of the old length.
      nBits = kBitsPerWord - startBit;
    }

    data[idx++] |= pattern << nBits;
  }

  // Initialize all bit-words after the last bit-word of the old length.
  size_t endIdx = _wordsPerBits(newLength);
  endIdx -= static_cast<size_t>(endIdx * kBitsPerWord == newLength);

  while (idx <= endIdx)
    data[idx++] = pattern;

  // Clear unused bits of the last bit-word.
  if (endBit)
    data[endIdx] &= (static_cast<BitWord>(1) << endBit) - 1;

  _length = newLength;
  return kErrorOk;
}

Error ZoneBitVector::_append(ZoneHeap* heap, bool value) noexcept {
  size_t kThreshold = Globals::kAllocThreshold * 8;
  size_t newLength = _length + 1;
  size_t idealCapacity = _capacity;

  if (idealCapacity < 128)
    idealCapacity = 128;
  else if (idealCapacity <= kThreshold)
    idealCapacity *= 2;
  else
    idealCapacity += kThreshold;

  if (ASMJIT_UNLIKELY(idealCapacity < _capacity)) {
    // It's technically impossible that `_length + 1` overflows.
    idealCapacity = newLength;
    ASMJIT_ASSERT(idealCapacity > _capacity);
  }

  return _resize(heap, newLength, idealCapacity, value);
}

Error ZoneBitVector::fill(size_t from, size_t to, bool value) noexcept {
  if (ASMJIT_UNLIKELY(from >= to)) {
    if (from > to)
      return DebugUtils::errored(kErrorInvalidArgument);
    else
      return kErrorOk;
  }

  ASMJIT_ASSERT(from <= _length);
  ASMJIT_ASSERT(to <= _length);

  // This is very similar to `ZoneBitVector::_fill()`, however, since we
  // actually set bits that are already part of the container we need to
  // special case filiing to zeros and ones.
  size_t idx = from / kBitsPerWord;
  size_t startBit = from % kBitsPerWord;

  size_t endIdx = to / kBitsPerWord;
  size_t endBit = to % kBitsPerWord;

  BitWord* data = _data;
  ASMJIT_ASSERT(data != nullptr);

  // Special case for non-zero `startBit`.
  if (startBit) {
    if (idx == endIdx) {
      ASMJIT_ASSERT(startBit < endBit);

      size_t nBits = endBit - startBit;
      BitWord mask = ((static_cast<BitWord>(1) << nBits) - 1) << startBit;

      if (value)
        data[idx] |= mask;
      else
        data[idx] &= ~mask;
      return kErrorOk;
    }
    else {
      BitWord mask = (static_cast<BitWord>(0) - 1) << startBit;

      if (value)
        data[idx++] |= mask;
      else
        data[idx++] &= ~mask;
    }
  }

  // Fill all bits in case there is a gap between the current `idx` and `endIdx`.
  if (idx < endIdx) {
    BitWord pattern = _patternFromBit(value);
    do {
      data[idx++] = pattern;
    } while (idx < endIdx);
  }

  // Special case for non-zero `endBit`.
  if (endBit) {
    BitWord mask = ((static_cast<BitWord>(1) << endBit) - 1);
    if (value)
      data[endIdx] |= mask;
    else
      data[endIdx] &= ~mask;
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::ZoneStackBase - Init / Reset]
// ============================================================================

Error ZoneStackBase::_init(ZoneHeap* heap, size_t middleIndex) noexcept {
  ZoneHeap* oldHeap = _heap;

  if (oldHeap) {
    Block* block = _block[kSideLeft];
    while (block) {
      Block* next = block->getNext();
      oldHeap->release(block, kBlockSize);
      block = next;
    }

    _heap = nullptr;
    _block[kSideLeft] = nullptr;
    _block[kSideRight] = nullptr;
  }


  if (heap) {
    Block* block = static_cast<Block*>(heap->alloc(kBlockSize));
    if (ASMJIT_UNLIKELY(!block))
      return DebugUtils::errored(kErrorNoHeapMemory);

    block->_link[kSideLeft] = nullptr;
    block->_link[kSideRight] = nullptr;
    block->_start = (uint8_t*)block + middleIndex;
    block->_end = (uint8_t*)block + middleIndex;

    _heap = heap;
    _block[kSideLeft] = block;
    _block[kSideRight] = block;
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::ZoneStackBase - Ops]
// ============================================================================

Error ZoneStackBase::_prepareBlock(uint32_t side, size_t initialIndex) noexcept {
  ASMJIT_ASSERT(isInitialized());

  Block* prev = _block[side];
  ASMJIT_ASSERT(!prev->isEmpty());

  Block* block = _heap->allocT<Block>(kBlockSize);
  if (ASMJIT_UNLIKELY(!block))
    return DebugUtils::errored(kErrorNoHeapMemory);

  block->_link[ side] = nullptr;
  block->_link[!side] = prev;
  block->_start = (uint8_t*)block + initialIndex;
  block->_end = (uint8_t*)block + initialIndex;

  prev->_link[side] = block;
  _block[side] = block;

  return kErrorOk;
}

void ZoneStackBase::_cleanupBlock(uint32_t side, size_t middleIndex) noexcept {
  Block* block = _block[side];
  ASMJIT_ASSERT(block->isEmpty());

  Block* prev = block->_link[!side];
  if (prev) {
    ASMJIT_ASSERT(prev->_link[side] == block);
    _heap->release(block, kBlockSize);

    prev->_link[side] = nullptr;
    _block[side] = prev;
  }
  else if (_block[!side] == prev && prev->isEmpty()) {
    // If the container becomes empty center both pointers in the remaining block.
    prev->_start = (uint8_t*)prev + middleIndex;
    prev->_end = (uint8_t*)prev + middleIndex;
  }
}

// ============================================================================
// [asmjit::ZoneHashBase - Utilities]
// ============================================================================

static uint32_t ZoneHash_getClosestPrime(uint32_t x) noexcept {
  static const uint32_t primeTable[] = {
    23, 53, 193, 389, 769, 1543, 3079, 6151, 12289, 24593
  };

  size_t i = 0;
  uint32_t p;

  do {
    if ((p = primeTable[i]) > x)
      break;
  } while (++i < ASMJIT_ARRAY_SIZE(primeTable));

  return p;
}

// ============================================================================
// [asmjit::ZoneHashBase - Reset]
// ============================================================================

void ZoneHashBase::reset(ZoneHeap* heap) noexcept {
  ZoneHashNode** oldData = _data;
  if (oldData != _embedded)
    _heap->release(oldData, _bucketsCount * sizeof(ZoneHashNode*));

  _heap = heap;
  _size = 0;
  _bucketsCount = 1;
  _bucketsGrow = 1;
  _data = _embedded;
  _embedded[0] = nullptr;
}

// ============================================================================
// [asmjit::ZoneHashBase - Rehash]
// ============================================================================

void ZoneHashBase::_rehash(uint32_t newCount) noexcept {
  ASMJIT_ASSERT(isInitialized());

  ZoneHashNode** oldData = _data;
  ZoneHashNode** newData = reinterpret_cast<ZoneHashNode**>(
    _heap->allocZeroed(static_cast<size_t>(newCount) * sizeof(ZoneHashNode*)));

  // We can still store nodes into the table, but it will degrade.
  if (ASMJIT_UNLIKELY(newData == nullptr))
    return;

  uint32_t i;
  uint32_t oldCount = _bucketsCount;

  for (i = 0; i < oldCount; i++) {
    ZoneHashNode* node = oldData[i];
    while (node) {
      ZoneHashNode* next = node->_hashNext;
      uint32_t hMod = node->_hVal % newCount;

      node->_hashNext = newData[hMod];
      newData[hMod] = node;

      node = next;
    }
  }

  // 90% is the maximum occupancy, can't overflow since the maximum capacity
  // is limited to the last prime number stored in the prime table.
  if (oldData != _embedded)
    _heap->release(oldData, _bucketsCount * sizeof(ZoneHashNode*));

  _bucketsCount = newCount;
  _bucketsGrow = newCount * 9 / 10;

  _data = newData;
}

// ============================================================================
// [asmjit::ZoneHashBase - Ops]
// ============================================================================

ZoneHashNode* ZoneHashBase::_put(ZoneHashNode* node) noexcept {
  uint32_t hMod = node->_hVal % _bucketsCount;
  ZoneHashNode* next = _data[hMod];

  node->_hashNext = next;
  _data[hMod] = node;

  if (++_size >= _bucketsGrow && next) {
    uint32_t newCapacity = ZoneHash_getClosestPrime(_bucketsCount);
    if (newCapacity != _bucketsCount)
      _rehash(newCapacity);
  }

  return node;
}

ZoneHashNode* ZoneHashBase::_del(ZoneHashNode* node) noexcept {
  uint32_t hMod = node->_hVal % _bucketsCount;

  ZoneHashNode** pPrev = &_data[hMod];
  ZoneHashNode* p = *pPrev;

  while (p) {
    if (p == node) {
      *pPrev = p->_hashNext;
      return node;
    }

    pPrev = &p->_hashNext;
    p = *pPrev;
  }

  return nullptr;
}

// ============================================================================
// [asmjit::Zone - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(base_zonevector) {
  Zone zone(8096 - Zone::kZoneOverhead);
  ZoneHeap heap(&zone);

  int i;
  int kMax = 100000;

  ZoneVector<int> vec;

  INFO("ZoneVector<int> basic tests");
  EXPECT(vec.append(&heap, 0) == kErrorOk);
  EXPECT(vec.isEmpty() == false);
  EXPECT(vec.getLength() == 1);
  EXPECT(vec.getCapacity() >= 1);
  EXPECT(vec.indexOf(0) == 0);
  EXPECT(vec.indexOf(-11) == Globals::kInvalidIndex);

  vec.clear();
  EXPECT(vec.isEmpty());
  EXPECT(vec.getLength() == 0);
  EXPECT(vec.indexOf(0) == Globals::kInvalidIndex);

  for (i = 0; i < kMax; i++) {
    EXPECT(vec.append(&heap, i) == kErrorOk);
  }
  EXPECT(vec.isEmpty() == false);
  EXPECT(vec.getLength() == static_cast<size_t>(kMax));
  EXPECT(vec.indexOf(kMax - 1) == static_cast<size_t>(kMax - 1));
}

UNIT(base_ZoneBitVector) {
  Zone zone(8096 - Zone::kZoneOverhead);
  ZoneHeap heap(&zone);

  size_t i, count;
  size_t kMaxCount = 100;

  ZoneBitVector vec;
  EXPECT(vec.isEmpty());
  EXPECT(vec.getLength() == 0);

  INFO("ZoneBitVector::resize()");
  for (count = 1; count < kMaxCount; count++) {
    vec.clear();
    EXPECT(vec.resize(&heap, count, false) == kErrorOk);
    EXPECT(vec.getLength() == count);

    for (i = 0; i < count; i++)
      EXPECT(vec.getAt(i) == false);

    vec.clear();
    EXPECT(vec.resize(&heap, count, true) == kErrorOk);
    EXPECT(vec.getLength() == count);

    for (i = 0; i < count; i++)
      EXPECT(vec.getAt(i) == true);
  }

  INFO("ZoneBitVector::fill()");
  for (count = 1; count < kMaxCount; count += 2) {
    vec.clear();
    EXPECT(vec.resize(&heap, count) == kErrorOk);
    EXPECT(vec.getLength() == count);

    for (i = 0; i < (count + 1) / 2; i++) {
      bool value = static_cast<bool>(i & 1);
      EXPECT(vec.fill(i, count - i, value) == kErrorOk);
    }

    for (i = 0; i < count; i++) {
      EXPECT(vec.getAt(i) == static_cast<bool>(i & 1));
    }
  }
}

UNIT(base_zonestack) {
  Zone zone(8096 - Zone::kZoneOverhead);
  ZoneHeap heap(&zone);
  ZoneStack<int> stack;

  INFO("ZoneStack<int> contains %d elements per one Block", ZoneStack<int>::kNumBlockItems);

  EXPECT(stack.init(&heap) == kErrorOk);
  EXPECT(stack.isEmpty(), "Stack must be empty after `init()`");

  EXPECT(stack.append(42) == kErrorOk);
  EXPECT(!stack.isEmpty() , "Stack must not be empty after an item has been appended");
  EXPECT(stack.pop() == 42, "Stack.pop() must return the item that has been appended last");
  EXPECT(stack.isEmpty()  , "Stack must be empty after the last element has been removed");

  EXPECT(stack.prepend(43) == kErrorOk);
  EXPECT(!stack.isEmpty()      , "Stack must not be empty after an item has been prepended");
  EXPECT(stack.popFirst() == 43, "Stack.popFirst() must return the item that has been prepended last");
  EXPECT(stack.isEmpty()       , "Stack must be empty after the last element has been removed");

  int i;
  int iMin =-100;
  int iMax = 100000;

  INFO("Adding items from %d to %d to the stack", iMin, iMax);
  for (i = 1; i <= iMax; i++) stack.append(i);
  for (i = 0; i >= iMin; i--) stack.prepend(i);

  INFO("Validating popFirst()");
  for (i = iMin; i <= iMax; i++) {
    int item = stack.popFirst();
    EXPECT(i == item, "Item '%d' didn't match the item '%d' popped", i, item);
  }
  EXPECT(stack.isEmpty());

  INFO("Adding items from %d to %d to the stack", iMin, iMax);
  for (i = 0; i >= iMin; i--) stack.prepend(i);
  for (i = 1; i <= iMax; i++) stack.append(i);

  INFO("Validating pop()");
  for (i = iMax; i >= iMin; i--) {
    int item = stack.pop();
    EXPECT(i == item, "Item '%d' didn't match the item '%d' popped", i, item);
  }
  EXPECT(stack.isEmpty());
}
#endif // ASMJIT_TEST

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
