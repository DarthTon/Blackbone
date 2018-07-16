// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/containers.h"
#include "../base/intutil.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::PodVectorBase - NullData]
// ============================================================================

const PodVectorData PodVectorBase::_nullData = { 0, 0 };

// ============================================================================
// [asmjit::PodVectorBase - Reset]
// ============================================================================

//! Clear vector data and free internal buffer.
void PodVectorBase::reset(bool releaseMemory) {
  PodVectorData* d = _d;

  if (d == &_nullData)
    return;

  if (releaseMemory) {
    ASMJIT_FREE(d);
    _d = const_cast<PodVectorData*>(&_nullData);
    return;
  }

  d->length = 0;
}

// ============================================================================
// [asmjit::PodVectorBase - Helpers]
// ============================================================================

Error PodVectorBase::_grow(size_t n, size_t sizeOfT) {
  PodVectorData* d = _d;

  size_t threshold = kMemAllocGrowMax / sizeOfT;
  size_t capacity = d->capacity;
  size_t after = d->length;

  if (IntUtil::maxUInt<size_t>() - n < after)
    return kErrorNoHeapMemory;

  after += n;

  if (capacity >= after)
    return kErrorOk;

  // PodVector is used as a linear array for some data structures used by
  // AsmJit code generation. The purpose of this agressive growing schema
  // is to minimize memory reallocations, because AsmJit code generation
  // classes live short life and will be freed or reused soon.
  if (capacity < 32)
    capacity = 32;
  else if (capacity < 128)
    capacity = 128;
  else if (capacity < 512)
    capacity = 512;

  while (capacity < after) {
    if (capacity < threshold)
      capacity *= 2;
    else
      capacity += threshold;
  }

  return _reserve(capacity, sizeOfT);
}

Error PodVectorBase::_reserve(size_t n, size_t sizeOfT) {
  PodVectorData* d = _d;

  if (d->capacity >= n)
    return kErrorOk;

  size_t nBytes = sizeof(PodVectorData) + n * sizeOfT;
  if (nBytes < n)
    return kErrorNoHeapMemory;

  if (d == &_nullData) {
    d = static_cast<PodVectorData*>(ASMJIT_ALLOC(nBytes));
    if (d == NULL)
      return kErrorNoHeapMemory;
    d->length = 0;
  }
  else {
    d = static_cast<PodVectorData*>(ASMJIT_REALLOC(d, nBytes));
    if (d == NULL)
      return kErrorNoHeapMemory;
  }

  d->capacity = n;
  _d = d;

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
