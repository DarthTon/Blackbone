// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../base/intutil.h"
#include "../base/vmem.h"

// [Dependenceis - C]
#include <stdarg.h>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::BaseAssembler - Construction / Destruction]
// ============================================================================

BaseAssembler::BaseAssembler(Runtime* runtime) :
  CodeGen(runtime),
  _buffer(NULL),
  _end(NULL),
  _cursor(NULL),
  _trampolineSize(0),
  _comment(NULL),
  _unusedLinks(NULL) {}

BaseAssembler::~BaseAssembler() {
  if (_buffer != NULL)
    ASMJIT_FREE(_buffer);
}

// ============================================================================
// [asmjit::BaseAssembler - Clear / Reset]
// ============================================================================

void BaseAssembler::clear() {
  _purge();
}

void BaseAssembler::reset() {
  _purge();
  _baseZone.reset();

  if (_buffer != NULL) {
    ASMJIT_FREE(_buffer);

    _buffer = NULL;
    _end = NULL;
    _cursor = NULL;
  }

  _labels.reset();
  _relocData.reset();
}

void BaseAssembler::_purge() {
  _baseZone.clear();
  _cursor = _buffer;

  _options = 0;
  _trampolineSize = 0;

  _comment = NULL;
  _unusedLinks = NULL;

  _labels.clear();
  _relocData.clear();

  clearError();
}

// ============================================================================
// [asmjit::BaseAssembler - Buffer]
// ============================================================================

Error BaseAssembler::_grow(size_t n) {
  size_t capacity = getCapacity();
  size_t after = getOffset() + n;

  // Overflow.
  if (n > IntUtil::maxUInt<uintptr_t>() - capacity)
    return setError(kErrorNoHeapMemory);

  // Grow is called when allocation is needed, so it shouldn't happen, but on
  // the other hand it is simple to catch and it's not an error.
  if (after <= capacity)
    return kErrorOk;

  if (capacity < kMemAllocOverhead)
    capacity = kMemAllocOverhead;
  else
    capacity += kMemAllocOverhead;

  do {
    size_t oldCapacity = capacity;

    if (capacity < kMemAllocGrowMax)
      capacity *= 2;
    else
      capacity += kMemAllocGrowMax;

    // Overflow.
    if (oldCapacity > capacity)
      return setError(kErrorNoHeapMemory);
  } while (capacity - kMemAllocOverhead < after);

  capacity -= kMemAllocOverhead;
  return _reserve(capacity);
}

Error BaseAssembler::_reserve(size_t n) {
  size_t capacity = getCapacity();
  if (n <= capacity)
    return kErrorOk;

  uint8_t* newBuffer;
  if (_buffer == NULL)
    newBuffer = static_cast<uint8_t*>(ASMJIT_ALLOC(n));
  else
    newBuffer = static_cast<uint8_t*>(ASMJIT_REALLOC(_buffer, n));

  if (newBuffer == NULL)
    return setError(kErrorNoHeapMemory);

  size_t offset = getOffset();

  _buffer = newBuffer;
  _end = _buffer + n;
  _cursor = newBuffer + offset;

  return kErrorOk;
}

// ============================================================================
// [asmjit::BaseAssembler - Label]
// ============================================================================

Error BaseAssembler::_registerIndexedLabels(size_t index) {
  size_t i = _labels.getLength();
  if (index < i)
    return kErrorOk;

  if (_labels._grow(index - i) != kErrorOk)
    return setError(kErrorNoHeapMemory);

  LabelData data;
  data.offset = -1;
  data.links = NULL;

  do {
    _labels.append(data);
  } while (++i < index);

  return kErrorOk;
}

Error BaseAssembler::_newLabel(Label* dst) {
  dst->_label.op = kOperandTypeLabel;
  dst->_label.size = 0;
  dst->_label.id = OperandUtil::makeLabelId(static_cast<uint32_t>(_labels.getLength()));

  LabelData data;
  data.offset = -1;
  data.links = NULL;

  if (_labels.append(data) != kErrorOk)
    goto _NoMemory;
  return kErrorOk;

_NoMemory:
  dst->_label.id = kInvalidValue;
  return setError(kErrorNoHeapMemory);
}

LabelLink* BaseAssembler::_newLabelLink() {
  LabelLink* link = _unusedLinks;

  if (link) {
    _unusedLinks = link->prev;
  }
  else {
    link = _baseZone.allocT<LabelLink>();
    if (link == NULL)
      return NULL;
  }

  link->prev = NULL;
  link->offset = 0;
  link->displacement = 0;
  link->relocId = -1;

  return link;
}

// ============================================================================
// [asmjit::BaseAssembler - Embed]
// ============================================================================

Error BaseAssembler::embed(const void* data, uint32_t size) {
  if (getRemainingSpace() < size) {
    Error error = _grow(size);
    if (error != kErrorOk)
      return setError(error);
  }

  uint8_t* cursor = getCursor();
  ::memcpy(cursor, data, size);
  setCursor(cursor + size);

#if !defined(ASMJIT_DISABLE_LOGGER)
  if (_logger)
    _logger->logBinary(kLoggerStyleData, data, size);
#endif // !ASMJIT_DISABLE_LOGGER

  return kErrorOk;
}

// ============================================================================
// [asmjit::BaseAssembler - Make]
// ============================================================================

void* BaseAssembler::make() {
  // Do nothing on error condition or if no instruction has been emitted.
  if (_error != kErrorOk || getCodeSize() == 0)
    return NULL;

  void* p;
  Error error = _runtime->add(&p, this);

  if (error != kErrorOk)
    setError(error);

  return p;
}

// ============================================================================
// [asmjit::BaseAssembler - Emit (Helpers)]
// ============================================================================

#define no noOperand

Error BaseAssembler::emit(uint32_t code) {
  return _emit(code, no, no, no, no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0) {
  return _emit(code, o0, no, no, no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, const Operand& o1) {
  return _emit(code, o0, o1, no, no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) {
  return _emit(code, o0, o1, o2, no);
}

Error BaseAssembler::emit(uint32_t code, int o0_) {
  return _emit(code, Imm(o0_), no, no, no);
}

Error BaseAssembler::emit(uint32_t code, uint64_t o0_) {
  return _emit(code, Imm(o0_), no, no, no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, int o1_) {
  return _emit(code, o0, Imm(o1_), no, no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, uint64_t o1_) {
  return _emit(code, o0, Imm(o1_), no, no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, const Operand& o1, int o2_) {
  return _emit(code, o0, o1, Imm(o2_), no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, const Operand& o1, uint64_t o2_) {
  return _emit(code, o0, o1, Imm(o2_), no);
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, int o3_) {
  return _emit(code, o0, o1, o2, Imm(o3_));
}

Error BaseAssembler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, uint64_t o3_) {
  return _emit(code, o0, o1, o2, Imm(o3_));
}

#undef no

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
