// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/string.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::StringBuilder - Construction / Destruction]
// ============================================================================

// Should be placed in read-only memory.
static const char StringBuilder_empty[4] = { 0 };

StringBuilder::StringBuilder() noexcept
  : _data(const_cast<char*>(StringBuilder_empty)),
    _length(0),
    _capacity(0),
    _canFree(false) {}

StringBuilder::~StringBuilder() noexcept {
  if (_canFree)
    Internal::releaseMemory(_data);
}

// ============================================================================
// [asmjit::StringBuilder - Prepare / Reserve]
// ============================================================================

ASMJIT_FAVOR_SIZE char* StringBuilder::prepare(uint32_t op, size_t len) noexcept {
  if (op == kStringOpSet) {
    // We don't care here, but we can't return a null pointer since it indicates
    // failure in memory allocation.
    if (len == 0) {
      if (_data != StringBuilder_empty)
        _data[0] = 0;

      _length = 0;
      return _data;
    }

    if (_capacity < len) {
      if (len >= IntTraits<size_t>::maxValue() - sizeof(intptr_t) * 2)
        return nullptr;

      size_t to = Utils::alignTo<size_t>(len, sizeof(intptr_t));
      if (to < 256 - sizeof(intptr_t))
        to = 256 - sizeof(intptr_t);

      char* newData = static_cast<char*>(Internal::allocMemory(to + sizeof(intptr_t)));
      if (!newData) {
        clear();
        return nullptr;
      }

      if (_canFree)
        Internal::releaseMemory(_data);

      _data = newData;
      _capacity = to + sizeof(intptr_t) - 1;
      _canFree = true;
    }

    _data[len] = 0;
    _length = len;

    ASMJIT_ASSERT(_length <= _capacity);
    return _data;
  }
  else {
    // We don't care here, but we can't return a null pointer since it indicates
    // failure of memory allocation.
    if (len == 0)
      return _data + _length;

    // Overflow.
    if (IntTraits<size_t>::maxValue() - sizeof(intptr_t) * 2 - _length < len)
      return nullptr;

    size_t after = _length + len;
    if (_capacity < after) {
      size_t to = _capacity;

      if (to < 256)
        to = 256;

      while (to < 1024 * 1024 && to < after)
        to *= 2;

      if (to < after) {
        to = after;
        if (to < (IntTraits<size_t>::maxValue() - 1024 * 32))
          to = Utils::alignTo<size_t>(to, 1024 * 32);
      }

      to = Utils::alignTo<size_t>(to, sizeof(intptr_t));
      char* newData = static_cast<char*>(Internal::allocMemory(to + sizeof(intptr_t)));
      if (!newData) return nullptr;

      ::memcpy(newData, _data, _length);
      if (_canFree)
        Internal::releaseMemory(_data);

      _data = newData;
      _capacity = to + sizeof(intptr_t) - 1;
      _canFree = true;
    }

    char* ret = _data + _length;
    _data[after] = 0;
    _length = after;

    ASMJIT_ASSERT(_length <= _capacity);
    return ret;
  }
}

ASMJIT_FAVOR_SIZE Error StringBuilder::reserve(size_t to) noexcept {
  if (_capacity >= to)
    return kErrorOk;

  if (to >= IntTraits<size_t>::maxValue() - sizeof(intptr_t) * 2)
    return DebugUtils::errored(kErrorNoHeapMemory);

  to = Utils::alignTo<size_t>(to, sizeof(intptr_t));
  char* newData = static_cast<char*>(Internal::allocMemory(to + sizeof(intptr_t)));

  if (!newData)
    return DebugUtils::errored(kErrorNoHeapMemory);

  ::memcpy(newData, _data, _length + 1);
  if (_canFree)
    Internal::releaseMemory(_data);

  _data = newData;
  _capacity = to + sizeof(intptr_t) - 1;
  _canFree = true;
  return kErrorOk;
}

// ============================================================================
// [asmjit::StringBuilder - Clear]
// ============================================================================

void StringBuilder::clear() noexcept {
  if (_data != StringBuilder_empty)
    _data[0] = 0;
  _length = 0;
}

// ============================================================================
// [asmjit::StringBuilder - Methods]
// ============================================================================

Error StringBuilder::_opString(uint32_t op, const char* str, size_t len) noexcept {
  if (len == Globals::kInvalidIndex)
    len = str ? ::strlen(str) : static_cast<size_t>(0);

  char* p = prepare(op, len);
  if (!p) return DebugUtils::errored(kErrorNoHeapMemory);

  ::memcpy(p, str, len);
  return kErrorOk;
}

Error StringBuilder::_opChar(uint32_t op, char c) noexcept {
  char* p = prepare(op, 1);
  if (!p) return DebugUtils::errored(kErrorNoHeapMemory);

  *p = c;
  return kErrorOk;
}

Error StringBuilder::_opChars(uint32_t op, char c, size_t n) noexcept {
  char* p = prepare(op, n);
  if (!p) return DebugUtils::errored(kErrorNoHeapMemory);

  ::memset(p, c, n);
  return kErrorOk;
}

static const char StringBuilder_numbers[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

Error StringBuilder::_opNumber(uint32_t op, uint64_t i, uint32_t base, size_t width, uint32_t flags) noexcept {
  if (base < 2 || base > 36)
    base = 10;

  char buf[128];
  char* p = buf + ASMJIT_ARRAY_SIZE(buf);

  uint64_t orig = i;
  char sign = '\0';

  // --------------------------------------------------------------------------
  // [Sign]
  // --------------------------------------------------------------------------

  if ((flags & kStringFormatSigned) != 0 && static_cast<int64_t>(i) < 0) {
    i = static_cast<uint64_t>(-static_cast<int64_t>(i));
    sign = '-';
  }
  else if ((flags & kStringFormatShowSign) != 0) {
    sign = '+';
  }
  else if ((flags & kStringFormatShowSpace) != 0) {
    sign = ' ';
  }

  // --------------------------------------------------------------------------
  // [Number]
  // --------------------------------------------------------------------------

  do {
    uint64_t d = i / base;
    uint64_t r = i % base;

    *--p = StringBuilder_numbers[r];
    i = d;
  } while (i);

  size_t numberLength = (size_t)(buf + ASMJIT_ARRAY_SIZE(buf) - p);

  // --------------------------------------------------------------------------
  // [Alternate Form]
  // --------------------------------------------------------------------------

  if ((flags & kStringFormatAlternate) != 0) {
    if (base == 8) {
      if (orig != 0)
        *--p = '0';
    }
    if (base == 16) {
      *--p = 'x';
      *--p = '0';
    }
  }

  // --------------------------------------------------------------------------
  // [Width]
  // --------------------------------------------------------------------------

  if (sign != 0)
    *--p = sign;

  if (width > 256)
    width = 256;

  if (width <= numberLength)
    width = 0;
  else
    width -= numberLength;

  // --------------------------------------------------------------------------
  // Write]
  // --------------------------------------------------------------------------

  size_t prefixLength = (size_t)(buf + ASMJIT_ARRAY_SIZE(buf) - p) - numberLength;
  char* data = prepare(op, prefixLength + width + numberLength);

  if (!data)
    return DebugUtils::errored(kErrorNoHeapMemory);

  ::memcpy(data, p, prefixLength);
  data += prefixLength;

  ::memset(data, '0', width);
  data += width;

  ::memcpy(data, p + prefixLength, numberLength);
  return kErrorOk;
}

Error StringBuilder::_opHex(uint32_t op, const void* data, size_t len) noexcept {
  char* dst;

  if (len >= IntTraits<size_t>::maxValue() / 2 || !(dst = prepare(op, len * 2)))
    return DebugUtils::errored(kErrorNoHeapMemory);;

  const char* src = static_cast<const char*>(data);
  for (size_t i = 0; i < len; i++, dst += 2, src++) {
    dst[0] = StringBuilder_numbers[(src[0] >> 4) & 0xF];
    dst[1] = StringBuilder_numbers[(src[0]     ) & 0xF];
  }
  return kErrorOk;
}

Error StringBuilder::_opVFormat(uint32_t op, const char* fmt, va_list ap) noexcept {
  char buf[1024];

  vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf), fmt, ap);
  buf[ASMJIT_ARRAY_SIZE(buf) - 1] = '\0';

  return _opString(op, buf);
}

Error StringBuilder::setFormat(const char* fmt, ...) noexcept {
  bool result;

  va_list ap;
  va_start(ap, fmt);
  result = _opVFormat(kStringOpSet, fmt, ap);
  va_end(ap);

  return result;
}

Error StringBuilder::appendFormat(const char* fmt, ...) noexcept {
  bool result;

  va_list ap;
  va_start(ap, fmt);
  result = _opVFormat(kStringOpAppend, fmt, ap);
  va_end(ap);

  return result;
}

bool StringBuilder::eq(const char* str, size_t len) const noexcept {
  const char* aData = _data;
  const char* bData = str;

  size_t aLength = _length;
  size_t bLength = len;

  if (bLength == Globals::kInvalidIndex) {
    size_t i;
    for (i = 0; i < aLength; i++)
      if (aData[i] != bData[i] || bData[i] == 0)
        return false;
    return bData[i] == 0;
  }
  else {
    if (aLength != bLength)
      return false;
    return ::memcmp(aData, bData, aLength) == 0;
  }
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
