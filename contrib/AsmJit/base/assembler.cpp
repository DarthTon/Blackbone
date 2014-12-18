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
// [asmjit::Assembler - Construction / Destruction]
// ============================================================================

Assembler::Assembler(Runtime* runtime) :
  CodeGen(runtime),
  _buffer(NULL),
  _end(NULL),
  _cursor(NULL),
  _trampolineSize(0),
  _comment(NULL),
  _unusedLinks(NULL) {}

Assembler::~Assembler() {
  reset(true);
}

// ============================================================================
// [asmjit::Assembler - Clear / Reset]
// ============================================================================

void Assembler::reset(bool releaseMemory) {
  // CodeGen members.
  _baseAddress = kNoBaseAddress;
  _instOptions = 0;
  _error = kErrorOk;

  _baseZone.reset(releaseMemory);

  // Assembler members.
  if (releaseMemory && _buffer != NULL) {
    ASMJIT_FREE(_buffer);
    _buffer = NULL;
    _end = NULL;
  }

  _cursor = _buffer;
  _trampolineSize = 0;

  _comment = NULL;
  _unusedLinks = NULL;

  _labelList.reset(releaseMemory);
  _relocList.reset(releaseMemory);
}

// ============================================================================
// [asmjit::Assembler - Buffer]
// ============================================================================

Error Assembler::_grow(size_t n) {
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

Error Assembler::_reserve(size_t n) {
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
// [asmjit::Assembler - Label]
// ============================================================================

Error Assembler::_registerIndexedLabels(size_t index) {
  size_t i = _labelList.getLength();
  if (index < i)
    return kErrorOk;

  if (_labelList._grow(index - i) != kErrorOk)
    return setError(kErrorNoHeapMemory);

  LabelData data;
  data.offset = -1;
  data.links = NULL;

  do {
    _labelList.append(data);
  } while (++i < index);

  return kErrorOk;
}

Error Assembler::_newLabel(Label* dst) {
  dst->_label.op = kOperandTypeLabel;
  dst->_label.size = 0;
  dst->_label.id = OperandUtil::makeLabelId(static_cast<uint32_t>(_labelList.getLength()));

  LabelData data;
  data.offset = -1;
  data.links = NULL;

  if (_labelList.append(data) != kErrorOk)
    goto _NoMemory;
  return kErrorOk;

_NoMemory:
  dst->_label.id = kInvalidValue;
  return setError(kErrorNoHeapMemory);
}

LabelLink* Assembler::_newLabelLink() {
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

Error Assembler::bind(const Label& label) {
  // Get label data based on label id.
  uint32_t index = label.getId();
  LabelData* data = getLabelData(index);

  // Label can be bound only once.
  if (data->offset != -1)
    return setError(kErrorLabelAlreadyBound);

#if !defined(ASMJIT_DISABLE_LOGGER)
  if (_logger)
    _logger->logFormat(kLoggerStyleLabel, "L%u:\n", index);
#endif // !ASMJIT_DISABLE_LOGGER

  Error error = kErrorOk;
  size_t pos = getOffset();

  LabelLink* link = data->links;
  LabelLink* prev = NULL;

  while (link) {
    intptr_t offset = link->offset;

    if (link->relocId != -1) {
      // Handle RelocData - We have to update RelocData information instead of
      // patching the displacement in LabelData.
      _relocList[link->relocId].data += static_cast<Ptr>(pos);
    }
    else {
      // Not using relocId, this means that we are overwriting a real
      // displacement in the binary stream.
      int32_t patchedValue = static_cast<int32_t>(
        static_cast<intptr_t>(pos) - offset + link->displacement);

      // Size of the value we are going to patch. Only BYTE/DWORD is allowed.
      uint32_t size = getByteAt(offset);
      ASMJIT_ASSERT(size == 1 || size == 4);

      if (size == 4) {
        setInt32At(offset, patchedValue);
      }
      else {
        ASMJIT_ASSERT(size == 1);
        if (IntUtil::isInt8(patchedValue))
          setByteAt(offset, static_cast<uint8_t>(patchedValue & 0xFF));
        else
          error = kErrorIllegalDisplacement;
      }
    }

    prev = link->prev;
    link = prev;
  }

  // Chain unused links.
  link = data->links;
  if (link) {
    if (prev == NULL)
      prev = link;

    prev->prev = _unusedLinks;
    _unusedLinks = link;
  }

  // Set as bound (offset is zero or greater and no links).
  data->offset = pos;
  data->links = NULL;

  if (error != kErrorOk)
    return setError(error);

  return error;
}

// ============================================================================
// [asmjit::Assembler - Embed]
// ============================================================================

Error Assembler::embed(const void* data, uint32_t size) {
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
// [asmjit::Assembler - Reloc]
// ============================================================================

size_t Assembler::relocCode(void* dst, Ptr baseAddress) const {
  if (baseAddress == kNoBaseAddress)
    baseAddress = hasBaseAddress() ? getBaseAddress() : static_cast<Ptr>((uintptr_t)dst);
  else if (getBaseAddress() != baseAddress)
    return 0;

  return _relocCode(dst, baseAddress);
}

// ============================================================================
// [asmjit::Assembler - Make]
// ============================================================================

void* Assembler::make() {
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
// [asmjit::Assembler - Emit (Helpers)]
// ============================================================================

#define NA noOperand

Error Assembler::emit(uint32_t code) {
  return _emit(code, NA, NA, NA, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0) {
  return _emit(code, o0, NA, NA, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, const Operand& o1) {
  return _emit(code, o0, o1, NA, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) {
  return _emit(code, o0, o1, o2, NA);
}

Error Assembler::emit(uint32_t code, int o0) {
  Imm imm(o0);
  return _emit(code, imm, NA, NA, NA);
}

Error Assembler::emit(uint32_t code, uint64_t o0) {
  Imm imm(o0);
  return _emit(code, imm, NA, NA, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, int o1) {
  Imm imm(o1);
  return _emit(code, o0, imm, NA, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, uint64_t o1) {
  Imm imm(o1);
  return _emit(code, o0, imm, NA, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, const Operand& o1, int o2) {
  Imm imm(o2);
  return _emit(code, o0, o1, imm, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, const Operand& o1, uint64_t o2) {
  Imm imm(o2);
  return _emit(code, o0, o1, imm, NA);
}

Error Assembler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, int o3) {
  Imm imm(o3);
  return _emit(code, o0, o1, o2, imm);
}

Error Assembler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, uint64_t o3) {
  Imm imm(o3);
  return _emit(code, o0, o1, o2, imm);
}

#undef NA

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
