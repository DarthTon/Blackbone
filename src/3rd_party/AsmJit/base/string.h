// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_STRING_H
#define _ASMJIT_BASE_STRING_H

// [Dependencies]
#include "../base/globals.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::SmallString]
// ============================================================================

//! Small string is a template that helps to create strings that can be either
//! statically allocated if they are small, or externally allocated in case
//! their length exceed the limit. The `WholeSize` represents the size of the
//! whole `SmallString` structure, based on that size the maximum size of the
//! internal buffer is determined.
template<size_t WholeSize>
class SmallString {
public:
  enum { kMaxEmbeddedLength = WholeSize - 5 };

  ASMJIT_INLINE SmallString() noexcept { reset(); }
  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  ASMJIT_INLINE bool isEmpty() const noexcept { return _length == 0; }
  ASMJIT_INLINE bool isEmbedded() const noexcept { return _length <= kMaxEmbeddedLength; }
  ASMJIT_INLINE bool mustEmbed(size_t len) const noexcept { return len <= kMaxEmbeddedLength; }

  ASMJIT_INLINE uint32_t getLength() const noexcept { return _length; }
  ASMJIT_INLINE char* getData() const noexcept {
    return _length <= kMaxEmbeddedLength ? const_cast<char*>(_embedded) : _external[1];
  }

  ASMJIT_INLINE void setEmbedded(const char* data, size_t len) noexcept {
    ASMJIT_ASSERT(len <= kMaxEmbeddedLength);

    _length = static_cast<uint32_t>(len);
    ::memcpy(_embedded, data, len);
    _embedded[len] = '\0';
  }

  ASMJIT_INLINE void setExternal(const char* data, size_t len) noexcept {
    ASMJIT_ASSERT(len > kMaxEmbeddedLength);
    ASMJIT_ASSERT(len <= ~static_cast<uint32_t>(0));

    _length = static_cast<uint32_t>(len);
    _external[1] = const_cast<char*>(data);
  }

  union {
    struct {
      uint32_t _length;
      char _embedded[WholeSize - 4];
    };
    char* _external[2];
  };
};

// ============================================================================
// [asmjit::StringBuilder]
// ============================================================================

//! String builder.
//!
//! String builder was designed to be able to build a string using append like
//! operation to append numbers, other strings, or signle characters. It can
//! allocate it's own buffer or use a buffer created on the stack.
//!
//! String builder contains method specific to AsmJit functionality, used for
//! logging or HTML output.
class StringBuilder {
public:
  ASMJIT_NONCOPYABLE(StringBuilder)

  //! \internal
  //!
  //! String operation.
  ASMJIT_ENUM(OpType) {
    kStringOpSet = 0,                    //!< Replace the current string by a given content.
    kStringOpAppend = 1                  //!< Append a given content to the current string.
  };

  //! \internal
  //!
  //! String format flags.
  ASMJIT_ENUM(StringFormatFlags) {
    kStringFormatShowSign  = 0x00000001,
    kStringFormatShowSpace = 0x00000002,
    kStringFormatAlternate = 0x00000004,
    kStringFormatSigned    = 0x80000000
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API StringBuilder() noexcept;
  ASMJIT_API ~StringBuilder() noexcept;

  ASMJIT_INLINE StringBuilder(const _NoInit&) noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get string builder capacity.
  ASMJIT_INLINE size_t getCapacity() const noexcept { return _capacity; }
  //! Get length.
  ASMJIT_INLINE size_t getLength() const noexcept { return _length; }

  //! Get null-terminated string data.
  ASMJIT_INLINE char* getData() noexcept { return _data; }
  //! Get null-terminated string data (const).
  ASMJIT_INLINE const char* getData() const noexcept { return _data; }

  // --------------------------------------------------------------------------
  // [Prepare / Reserve]
  // --------------------------------------------------------------------------

  //! Prepare to set/append.
  ASMJIT_API char* prepare(uint32_t op, size_t len) noexcept;

  //! Reserve `to` bytes in string builder.
  ASMJIT_API Error reserve(size_t to) noexcept;

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

  //! Clear the content in String builder.
  ASMJIT_API void clear() noexcept;

  // --------------------------------------------------------------------------
  // [Op]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _opString(uint32_t op, const char* str, size_t len = Globals::kInvalidIndex) noexcept;
  ASMJIT_API Error _opVFormat(uint32_t op, const char* fmt, va_list ap) noexcept;
  ASMJIT_API Error _opChar(uint32_t op, char c) noexcept;
  ASMJIT_API Error _opChars(uint32_t op, char c, size_t n) noexcept;
  ASMJIT_API Error _opNumber(uint32_t op, uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept;
  ASMJIT_API Error _opHex(uint32_t op, const void* data, size_t len) noexcept;

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  //! Replace the current string with `str` having `len` characters (or `kInvalidIndex` if it's null terminated).
  ASMJIT_INLINE Error setString(const char* str, size_t len = Globals::kInvalidIndex) noexcept { return _opString(kStringOpSet, str, len); }
  //! Replace the current content by a formatted string `fmt`.
  ASMJIT_API Error setFormat(const char* fmt, ...) noexcept;
  //! Replace the current content by a formatted string `fmt` (va_list version).
  ASMJIT_INLINE Error setFormatVA(const char* fmt, va_list ap) noexcept { return _opVFormat(kStringOpSet, fmt, ap); }

  //! Replace the current content by a single `c` character.
  ASMJIT_INLINE Error setChar(char c) noexcept { return _opChar(kStringOpSet, c); }
  //! Replace the current content by `c` character `n` times.
  ASMJIT_INLINE Error setChars(char c, size_t n) noexcept { return _opChars(kStringOpSet, c, n); }

  //! Replace the current content by a formatted integer `i` (signed).
  ASMJIT_INLINE Error setInt(uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpSet, i, base, width, flags | kStringFormatSigned);
  }

  //! Replace the current content by a formatted integer `i` (unsigned).
  ASMJIT_INLINE Error setUInt(uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpSet, i, base, width, flags);
  }

  //! Replace the current content by the given `data` converted to a HEX string.
  ASMJIT_INLINE Error setHex(const void* data, size_t len) noexcept {
    return _opHex(kStringOpSet, data, len);
  }

  // --------------------------------------------------------------------------
  // [Append]
  // --------------------------------------------------------------------------

  //! Append string `str` having `len` characters (or `kInvalidIndex` if it's null terminated).
  ASMJIT_INLINE Error appendString(const char* str, size_t len = Globals::kInvalidIndex) noexcept { return _opString(kStringOpAppend, str, len); }
  //! Append a formatted string `fmt`.
  ASMJIT_API Error appendFormat(const char* fmt, ...) noexcept;
  //! Append a formatted string `fmt` (va_list version).
  ASMJIT_INLINE Error appendFormatVA(const char* fmt, va_list ap) noexcept { return _opVFormat(kStringOpAppend, fmt, ap); }

  //! Append a single `c` character.
  ASMJIT_INLINE Error appendChar(char c) noexcept { return _opChar(kStringOpAppend, c); }
  //! Append `c` character `n` times.
  ASMJIT_INLINE Error appendChars(char c, size_t n) noexcept { return _opChars(kStringOpAppend, c, n); }

  //! Append `i`.
  ASMJIT_INLINE Error appendInt(int64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpAppend, static_cast<uint64_t>(i), base, width, flags | kStringFormatSigned);
  }

  //! Append `i`.
  ASMJIT_INLINE Error appendUInt(uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpAppend, i, base, width, flags);
  }

  //! Append the given `data` converted to a HEX string.
  ASMJIT_INLINE Error appendHex(const void* data, size_t len) noexcept {
    return _opHex(kStringOpAppend, data, len);
  }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  //! Check for equality with other `str` of length `len`.
  ASMJIT_API bool eq(const char* str, size_t len = Globals::kInvalidIndex) const noexcept;
  //! Check for equality with `other`.
  ASMJIT_INLINE bool eq(const StringBuilder& other) const noexcept { return eq(other._data, other._length); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool operator==(const StringBuilder& other) const noexcept { return  eq(other); }
  ASMJIT_INLINE bool operator!=(const StringBuilder& other) const noexcept { return !eq(other); }

  ASMJIT_INLINE bool operator==(const char* str) const noexcept { return  eq(str); }
  ASMJIT_INLINE bool operator!=(const char* str) const noexcept { return !eq(str); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  char* _data;                           //!< String data.
  size_t _length;                        //!< String length.
  size_t _capacity;                      //!< String capacity.
  size_t _canFree;                       //!< If the string data can be freed.
};

// ============================================================================
// [asmjit::StringBuilderTmp]
// ============================================================================

//! Temporary string builder, has statically allocated `N` bytes.
template<size_t N>
class StringBuilderTmp : public StringBuilder {
public:
  ASMJIT_NONCOPYABLE(StringBuilderTmp<N>)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE StringBuilderTmp() noexcept : StringBuilder(NoInit) {
    _data = _embeddedData;
    _data[0] = 0;

    _length = 0;
    _capacity = N;
    _canFree = false;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Embedded data.
  char _embeddedData[static_cast<size_t>(
    N + 1 + sizeof(intptr_t)) & ~static_cast<size_t>(sizeof(intptr_t) - 1)];
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_STRING_H
