// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_UTIL_H
#define _ASMJIT_UTIL_H

// [Dependencies]
#include "Build.h"

#include <stdlib.h>
#include <string.h>

namespace AsmJit {

//! @addtogroup AsmJit_Util
//! @{

// ============================================================================
// [AsmJit::Macros]
// ============================================================================

// Skip documenting this.
#if !defined(ASMJIT_NODOC)
struct ASMJIT_HIDDEN _DontInitialize {};
struct ASMJIT_HIDDEN _Initialize {};
#endif // !ASMJIT_NODOC

// ============================================================================
// [AsmJit::Util]
// ============================================================================

namespace Util {

#pragma warning(disable : 4127 4100)


// ============================================================================
// [AsmJit::Util::isInt?]
// ============================================================================

//! @brief Returns @c true if a given integer @a x is signed 8-bit integer
static inline bool isInt8(sysint_t x) ASMJIT_NOTHROW { return x >= -128 && x <= 127; }
//! @brief Returns @c true if a given integer @a x is unsigned 8-bit integer
static inline bool isUInt8(sysint_t x) ASMJIT_NOTHROW { return x >= 0 && x <= 255; }

//! @brief Returns @c true if a given integer @a x is signed 16-bit integer
static inline bool isInt16(sysint_t x) ASMJIT_NOTHROW { return x >= -32768 && x <= 32767; }
//! @brief Returns @c true if a given integer @a x is unsigned 16-bit integer
static inline bool isUInt16(sysint_t x) ASMJIT_NOTHROW { return x >= 0 && x <= 65535; }

//! @brief Returns @c true if a given integer @a x is signed 16-bit integer
static inline bool isInt32(sysint_t x) ASMJIT_NOTHROW
{
#if defined(ASMJIT_X86)
  return true;
#else
  return x >= ASMJIT_INT64_C(-2147483648) && x <= ASMJIT_INT64_C(2147483647);
#endif
}
//! @brief Returns @c true if a given integer @a x is unsigned 16-bit integer
static inline bool isUInt32(sysint_t x) ASMJIT_NOTHROW
{
#if defined(ASMJIT_X86)
  return x >= 0;
#else
  return x >= 0 && x <= ASMJIT_INT64_C(4294967295);
#endif
}

// ============================================================================
// [Bit Utils]
// ============================================================================

static inline uint32_t maskFromIndex(uint32_t x)
{
  return (1U << x);
}

static inline uint32_t maskUpToIndex(uint32_t x)
{
  if (x >= 32)
    return 0xFFFFFFFF;
  else
    return (1U << x) - 1;
}

// From http://graphics.stanford.edu/~seander/bithacks.html .
static inline uint32_t bitCount(uint32_t x)
{
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  return ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

static inline uint32_t findFirstBit(uint32_t mask) ASMJIT_NOTHROW
{
  for (uint32_t i = 0, bit = 1; i < sizeof(uint32_t) * 8; i++, bit <<= 1)
  {
    if (mask & bit) return i;
  }

  // INVALID_VALUE.
  return 0xFFFFFFFF;
}

// ============================================================================
// [Alignment]
// ============================================================================

// Align variable @a x to 16-bytes.
template<typename T>
static inline T alignTo16(const T& x)
{
  return (x + (T)15) & (T)~15;
}

// Return the size needed to align variable @a x to 16-bytes.
template<typename T>
static inline T deltaTo16(const T& x)
{
  T aligned = alignTo16(x);
  return aligned - x;
}

} // Util namespace

// ============================================================================
// [AsmJit::function_cast<>]
// ============================================================================

//! @brief Cast used to cast pointer to function. It's like reinterpret_cast<>, 
//! but uses internally C style cast to work with MinGW.
//!
//! If you are using single compiler and @c reinterpret_cast<> works for you,
//! there is no reason to use @c AsmJit::function_cast<>. If you are writing
//! cross-platform software with various compiler support, consider using
//! @c AsmJit::function_cast<> instead of @c reinterpret_cast<>.
template<typename T, typename Z>
static inline T function_cast(Z* p) ASMJIT_NOTHROW { return (T)p; }

// ============================================================================
// [AsmJit::(X)MMData]
// ============================================================================

//! @brief Structure used for MMX specific data (64-bit).
//!
//! This structure can be used to load / store data from / to MMX register.
union ASMJIT_HIDDEN MMData
{
  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Set all eight signed 8-bit integers.
  inline void setSB(
    int8_t x0, int8_t x1, int8_t x2, int8_t x3, int8_t x4, int8_t x5, int8_t x6, int8_t x7) ASMJIT_NOTHROW
  {
    sb[0] = x0; sb[1] = x1; sb[2] = x2; sb[3] = x3; sb[4] = x4; sb[5] = x5; sb[6] = x6; sb[7] = x7;
  }

  //! @brief Set all eight unsigned 8-bit integers.
  inline void setUB(
    uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4, uint8_t x5, uint8_t x6, uint8_t x7) ASMJIT_NOTHROW
  {
    ub[0] = x0; ub[1] = x1; ub[2] = x2; ub[3] = x3; ub[4] = x4; ub[5] = x5; ub[6] = x6; ub[7] = x7;
  }

  //! @brief Set all four signed 16-bit integers.
  inline void setSW(
    int16_t x0, int16_t x1, int16_t x2, int16_t x3) ASMJIT_NOTHROW
  {
    sw[0] = x0; sw[1] = x1; sw[2] = x2; sw[3] = x3;
  }

  //! @brief Set all four unsigned 16-bit integers.
  inline void setUW(
    uint16_t x0, uint16_t x1, uint16_t x2, uint16_t x3) ASMJIT_NOTHROW
  {
    uw[0] = x0; uw[1] = x1; uw[2] = x2; uw[3] = x3;
  }

  //! @brief Set all two signed 32-bit integers.
  inline void setSD(
    int32_t x0, int32_t x1) ASMJIT_NOTHROW
  {
    sd[0] = x0; sd[1] = x1;
  }

  //! @brief Set all two unsigned 32-bit integers.
  inline void setUD(
    uint32_t x0, uint32_t x1) ASMJIT_NOTHROW
  {
    ud[0] = x0; ud[1] = x1;
  }

  //! @brief Set signed 64-bit integer.
  inline void setSQ(
    int64_t x0) ASMJIT_NOTHROW
  {
    sq[0] = x0;
  }

  //! @brief Set unsigned 64-bit integer.
  inline void setUQ(
    uint64_t x0) ASMJIT_NOTHROW
  {
    uq[0] = x0;
  }

  //! @brief Set all two SP-FP values.
  inline void setSF(
    float x0, float x1) ASMJIT_NOTHROW
  {
    sf[0] = x0; sf[1] = x1;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Array of eight signed 8-bit integers.
  int8_t sb[8];
  //! @brief Array of eight unsigned 8-bit integers.
  uint8_t ub[8];
  //! @brief Array of four signed 16-bit integers.
  int16_t sw[4];
  //! @brief Array of four unsigned 16-bit integers.
  uint16_t uw[4];
  //! @brief Array of two signed 32-bit integers.
  int32_t sd[2];
  //! @brief Array of two unsigned 32-bit integers.
  uint32_t ud[2];
  //! @brief Array of one signed 64-bit integer.
  int64_t sq[1];
  //! @brief Array of one unsigned 64-bit integer.
  uint64_t uq[1];

  //! @brief Array of two SP-FP values.
  float sf[2];
};

//! @brief Structure used for SSE specific data (128-bit).
//!
//! This structure can be used to load / store data from / to SSE register.
//!
//! @note Always align SSE data to 16-bytes.
union ASMJIT_HIDDEN XMMData
{
  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Set all sixteen signed 8-bit integers.
  inline void setSB(
    int8_t x0, int8_t x1, int8_t x2 , int8_t x3 , int8_t x4 , int8_t x5 , int8_t x6 , int8_t x7 ,
    int8_t x8, int8_t x9, int8_t x10, int8_t x11, int8_t x12, int8_t x13, int8_t x14, int8_t x15) ASMJIT_NOTHROW
  {
    sb[0] = x0; sb[1] = x1; sb[ 2] = x2 ; sb[3 ] = x3 ; sb[4 ] = x4 ; sb[5 ] = x5 ; sb[6 ] = x6 ; sb[7 ] = x7 ;
    sb[8] = x8; sb[9] = x9; sb[10] = x10; sb[11] = x11; sb[12] = x12; sb[13] = x13; sb[14] = x14; sb[15] = x15; 
  }

  //! @brief Set all sixteen unsigned 8-bit integers.
  inline void setUB(
    uint8_t x0, uint8_t x1, uint8_t x2 , uint8_t x3 , uint8_t x4 , uint8_t x5 , uint8_t x6 , uint8_t x7 ,
    uint8_t x8, uint8_t x9, uint8_t x10, uint8_t x11, uint8_t x12, uint8_t x13, uint8_t x14, uint8_t x15) ASMJIT_NOTHROW
  {
    ub[0] = x0; ub[1] = x1; ub[ 2] = x2 ; ub[3 ] = x3 ; ub[4 ] = x4 ; ub[5 ] = x5 ; ub[6 ] = x6 ; ub[7 ] = x7 ;
    ub[8] = x8; ub[9] = x9; ub[10] = x10; ub[11] = x11; ub[12] = x12; ub[13] = x13; ub[14] = x14; ub[15] = x15; 
  }

  //! @brief Set all eight signed 16-bit integers.
  inline void setSW(
    int16_t x0, int16_t x1, int16_t x2, int16_t x3, int16_t x4, int16_t x5, int16_t x6, int16_t x7) ASMJIT_NOTHROW
  {
    sw[0] = x0; sw[1] = x1; sw[2] = x2; sw[3] = x3; sw[4] = x4; sw[5] = x5; sw[6] = x6; sw[7] = x7;
  }

  //! @brief Set all eight unsigned 16-bit integers.
  inline void setUW(
    uint16_t x0, uint16_t x1, uint16_t x2, uint16_t x3, uint16_t x4, uint16_t x5, uint16_t x6, uint16_t x7) ASMJIT_NOTHROW
  {
    uw[0] = x0; uw[1] = x1; uw[2] = x2; uw[3] = x3; uw[4] = x4; uw[5] = x5; uw[6] = x6; uw[7] = x7;
  }

  //! @brief Set all four signed 32-bit integers.
  inline void setSD(
    int32_t x0, int32_t x1, int32_t x2, int32_t x3) ASMJIT_NOTHROW
  {
    sd[0] = x0; sd[1] = x1; sd[2] = x2; sd[3] = x3;
  }

  //! @brief Set all four unsigned 32-bit integers.
  inline void setUD(
    uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3) ASMJIT_NOTHROW
  {
    ud[0] = x0; ud[1] = x1; ud[2] = x2; ud[3] = x3;
  }

  //! @brief Set all two signed 64-bit integers.
  inline void setSQ(
    int64_t x0, int64_t x1) ASMJIT_NOTHROW
  {
    sq[0] = x0; sq[1] = x1;
  }

  //! @brief Set all two unsigned 64-bit integers.
  inline void setUQ(
    uint64_t x0, uint64_t x1) ASMJIT_NOTHROW
  {
    uq[0] = x0; uq[1] = x1;
  }

  //! @brief Set all four SP-FP floats.
  inline void setSF(
    float x0, float x1, float x2, float x3) ASMJIT_NOTHROW
  {
    sf[0] = x0; sf[1] = x1; sf[2] = x2; sf[3] = x3;
  }

  //! @brief Set all two DP-FP floats.
  inline void setDF(
    double x0, double x1) ASMJIT_NOTHROW
  {
    df[0] = x0; df[1] = x1;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Array of sixteen signed 8-bit integers.
  int8_t sb[16];
  //! @brief Array of sixteen unsigned 8-bit integers.
  uint8_t ub[16];
  //! @brief Array of eight signed 16-bit integers.
  int16_t sw[8];
  //! @brief Array of eight unsigned 16-bit integers.
  uint16_t uw[8];
  //! @brief Array of four signed 32-bit integers.
  int32_t sd[4];
  //! @brief Array of four unsigned 32-bit integers.
  uint32_t ud[4];
  //! @brief Array of two signed 64-bit integers.
  int64_t sq[2];
  //! @brief Array of two unsigned 64-bit integers.
  uint64_t uq[2];

  //! @brief Array of four 32-bit single precision floating points.
  float sf[4];
  //! @brief Array of two 64-bit double precision floating points.
  double df[2];
};

// ============================================================================
// [AsmJit::Buffer]
// ============================================================================

//! @brief Buffer used to store instruction stream in AsmJit.
//! 
//! This class can be dangerous, if you don't know how it works. Assembler
//! instruction stream is usually constructed by multiple calls of emit
//! functions that emits bytes, words, dwords or qwords. But to decrease
//! AsmJit library size and improve performance, we are not checking for
//! buffer overflow for each emit operation, but only once in highler level
//! emit instruction.
//!
//! So, if you want to use this class, you need to do buffer checking yourself
//! by using @c ensureSpace() method. It's designed to grow buffer if needed.
//! Threshold for growing is named @c growThreshold() and it means count of
//! bytes for emitting single operation. Default size is set to 16 bytes,
//! because x86 and x64 instruction can't be larger (so it's space to hold 1
//! instruction).
//!
//! Example using Buffer:
//!
//! @code
//! // Buffer instance, growThreshold == 16
//! // (no memory allocated in constructor).
//! AsmJit::Buffer buf(16);
//!
//! // Begin of emit stream, ensure space can fail on out of memory error.
//! if (buf.ensureSpace()) 
//! {
//!   // here, you can emit up to 16 (growThreshold) bytes
//!   buf.emitByte(0x00);
//!   buf.emitByte(0x01);
//!   buf.emitByte(0x02);
//!   buf.emitByte(0x03);
//!   ...
//! }
//! @endcode
struct ASMJIT_API Buffer
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline Buffer(sysint_t growThreshold = 16) ASMJIT_NOTHROW :
    _data(NULL),
    _cur(NULL),
    _max(NULL),
    _capacity(0),
    _growThreshold(growThreshold)
  {
  }

  inline ~Buffer() ASMJIT_NOTHROW
  {
    if (_data) ASMJIT_FREE(_data);
  }

  //! @brief Get start of buffer.
  inline uint8_t* getData() const ASMJIT_NOTHROW { return _data; }

  //! @brief Get current pointer in code buffer.
  inline uint8_t* getCur() const ASMJIT_NOTHROW { return _cur; }

  //! @brief Get maximum pointer in code buffer for growing.
  inline uint8_t* getMax() const ASMJIT_NOTHROW { return _max; }

  //! @brief Get current offset in buffer.
  inline sysint_t getOffset() const ASMJIT_NOTHROW { return (sysint_t)(_cur - _data); }

  //! @brief Get capacity of buffer.
  inline sysint_t getCapacity() const ASMJIT_NOTHROW { return _capacity; }

  //! @brief Get grow threshold.
  inline sysint_t getGrowThreshold() const ASMJIT_NOTHROW { return _growThreshold; }

  //! @brief Ensure space for next instruction
  inline bool ensureSpace() ASMJIT_NOTHROW { return (_cur >= _max) ? grow() : true; }

  //! @brief Sets offset to @a o and returns previous offset.
  //!
  //! This method can be used to truncate buffer or it's used to
  //! overwrite specific position in buffer by Assembler.
  inline sysint_t toOffset(sysint_t o) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(o < _capacity);

    sysint_t prev = (sysint_t)(_cur - _data);
    _cur = _data + o;
    return prev;
  }

  //! @brief Reallocate buffer.
  //!
  //! It's only used for growing, buffer is never reallocated to smaller 
  //! number than current capacity() is.
  bool realloc(sysint_t to) ASMJIT_NOTHROW;

  //! @brief Used to grow the buffer.
  //!
  //! It will typically realloc to twice size of capacity(), but if capacity()
  //! is large, it will use smaller steps.
  bool grow() ASMJIT_NOTHROW;

  //! @brief Clear everything, but not deallocate buffer.
  void clear() ASMJIT_NOTHROW;

  //! @brief Free buffer and NULL all pointers.
  void free() ASMJIT_NOTHROW;

  //! @brief Take ownership of the buffer data and purge @c Buffer instance.
  uint8_t* take() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  //! @brief Emit Byte.
  inline void emitByte(uint8_t x) ASMJIT_NOTHROW
  {
    *_cur++ = x;
  }

  //! @brief Emit Word (2 bytes).
  inline void emitWord(uint16_t x) ASMJIT_NOTHROW
  {
    *(uint16_t *)_cur = x;
    _cur += 2;
  }

  //! @brief Emit DWord (4 bytes).
  inline void emitDWord(uint32_t x) ASMJIT_NOTHROW
  {
    *(uint32_t *)_cur = x;
    _cur += 4;
  }

  //! @brief Emit QWord (8 bytes).
  inline void emitQWord(uint64_t x) ASMJIT_NOTHROW
  {
    *(uint64_t *)_cur = x;
    _cur += 8;
  }

  //! @brief Emit system signed integer (4 or 8 bytes).
  inline void emitSysInt(sysint_t x) ASMJIT_NOTHROW
  {
    *(sysint_t *)_cur = x;
    _cur += sizeof(sysint_t);
  }

  //! @brief Emit system unsigned integer (4 or 8 bytes).
  inline void emitSysUInt(sysuint_t x) ASMJIT_NOTHROW
  {
    *(sysuint_t *)_cur = x;
    _cur += sizeof(sysuint_t);
  }

  //! @brief Emit custom data. 
  void emitData(const void* ptr, sysuint_t len) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Get / Set]
  // --------------------------------------------------------------------------

  //! @brief Set byte at position @a pos.
  inline uint8_t getByteAt(sysint_t pos) const ASMJIT_NOTHROW
  {
    return *reinterpret_cast<const uint8_t*>(_data + pos);
  }

  //! @brief Set word at position @a pos.
  inline uint16_t getWordAt(sysint_t pos) const ASMJIT_NOTHROW
  {
    return *reinterpret_cast<const uint16_t*>(_data + pos);
  }

  //! @brief Set word at position @a pos.
  inline uint32_t getDWordAt(sysint_t pos) const ASMJIT_NOTHROW
  {
    return *reinterpret_cast<const uint32_t*>(_data + pos);
  }

  //! @brief Set word at position @a pos.
  inline uint64_t getQWordAt(sysint_t pos) const ASMJIT_NOTHROW
  {
    return *reinterpret_cast<const uint64_t*>(_data + pos);
  }

  //! @brief Set byte at position @a pos.
  inline void setByteAt(sysint_t pos, uint8_t x) ASMJIT_NOTHROW
  {
    *reinterpret_cast<uint8_t*>(_data + pos) = x;
  }

  //! @brief Set word at position @a pos.
  inline void setWordAt(sysint_t pos, uint16_t x) ASMJIT_NOTHROW
  {
    *reinterpret_cast<uint16_t*>(_data + pos) = x;
  }

  //! @brief Set word at position @a pos.
  inline void setDWordAt(sysint_t pos, uint32_t x) ASMJIT_NOTHROW
  {
    *reinterpret_cast<uint32_t*>(_data + pos) = x;
  }

  //! @brief Set word at position @a pos.
  inline void setQWordAt(sysint_t pos, uint64_t x) ASMJIT_NOTHROW
  {
    *reinterpret_cast<uint64_t*>(_data + pos) = x;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  // All members are public, because they can be accessed and modified by 
  // Assembler/Compiler directly.

  //! @brief Beginning position of buffer.
  uint8_t* _data;
  //! @brief Current position in buffer.
  uint8_t* _cur;
  //! @brief Maximum position in buffer for realloc.
  uint8_t* _max;

  //! @brief Buffer capacity (in bytes).
  sysint_t _capacity;

  //! @brief Grow threshold
  sysint_t _growThreshold;
};

// ============================================================================
// [AsmJit::PodVector<>]
// ============================================================================

//! @brief Template used to store and manage array of POD data.
//!
//! This template has these adventages over other vector<> templates:
//! - Non-copyable (designed to be non-copyable, we want it)
//! - No copy-on-write (some implementations of stl can use it)
//! - Optimized for working only with POD types
//! - Uses ASMJIT_... memory management macros
template <typename T>
struct PodVector
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new instance of PodVector template. Data will not
  //! be allocated (will be NULL).
  inline PodVector() ASMJIT_NOTHROW : _data(NULL), _length(0), _capacity(0)
  {
  }
  
  //! @brief Destroy PodVector and free all data.
  inline ~PodVector() ASMJIT_NOTHROW
  {
    if (_data) ASMJIT_FREE(_data);
  }

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  //! @brief Get vector data.
  inline T* getData() ASMJIT_NOTHROW { return _data; }
  //! @overload
  inline const T* getData() const ASMJIT_NOTHROW { return _data; }
  //! @brief Get vector length.
  inline sysuint_t getLength() const ASMJIT_NOTHROW { return _length; }
  //! @brief get vector capacity (allocation capacity).
  inline sysuint_t getCapacity() const ASMJIT_NOTHROW { return _capacity; }

  // --------------------------------------------------------------------------
  // [Manipulation]
  // --------------------------------------------------------------------------

  //! @brief Clear vector data, but not free internal buffer.
  void clear() ASMJIT_NOTHROW
  {
    _length = 0;
  }

  //! @brief Clear vector data and free internal buffer.
  void free() ASMJIT_NOTHROW
  {
    if (_data) 
    {
      ASMJIT_FREE(_data);
      _data = 0;
      _length = 0;
      _capacity = 0;
    }
  }

  //! @brief Prepend @a item to vector.
  bool prepend(const T& item) ASMJIT_NOTHROW
  {
    if (_length == _capacity && !_grow()) return false;

    memmove(_data + 1, _data, sizeof(T) * _length);
    memcpy(_data, &item, sizeof(T));

    _length++;
    return true;
  }

  //! @brief Insert an @a item at the @a index.
  bool insert(sysuint_t index, const T& item) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(index <= _length);
    if (_length == _capacity && !_grow()) return false;

    T* dst = _data + index;
    memmove(dst + 1, dst, _length - index);
    memcpy(dst, &item, sizeof(T));

    _length++;
    return true;
  }

  //! @brief Append @a item to vector.
  bool append(const T& item) ASMJIT_NOTHROW
  {
    if (_length == _capacity && !_grow()) return false;

    memcpy(_data + _length, &item, sizeof(T));

    _length++;
    return true;
  }

  //! @brief Get index of @a val or (sysuint_t)-1 if not found.
  sysuint_t indexOf(const T& val) const ASMJIT_NOTHROW
  {
    sysuint_t i = 0, len = _length;
    for (i = 0; i < len; i++) { if (_data[i] == val) return i; }
    return (sysuint_t)-1;
  }

  //! @brief Remove element at index @a i.
  void removeAt(sysuint_t i) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(i < _length);

    T* dst = _data + i;
    _length--;
    memmove(dst, dst + 1, _length - i);
  }

  //! @brief Swap this pod-vector with @a other.
  void swap(PodVector<T>& other) ASMJIT_NOTHROW
  {
    T* _tmp_data = _data;
    sysuint_t _tmp_length = _length;
    sysuint_t _tmp_capacity = _capacity;

    _data = other._data;
    _length = other._length;
    _capacity = other._capacity;

    other._data = _tmp_data;
    other._length = _tmp_length;
    other._capacity = _tmp_capacity;
  }

  //! @brief Get item at position @a i.
  inline T& operator[](sysuint_t i) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(i < _length);
    return _data[i];
  }
  //! @brief Get item at position @a i.
  inline const T& operator[](sysuint_t i) const ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(i < _length);
    return _data[i];
  }

  //! @brief Append the item and return address so it can be initialized.
  T* newItem() ASMJIT_NOTHROW
  {
    if (_length == _capacity && !_grow()) return NULL;
    return _data + (_length++);
  }

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

private:
  //! @brief Called to grow internal array.
  bool _grow() ASMJIT_NOTHROW
  {
    return _realloc(_capacity < 16 ? 16 : _capacity * 2);
  }

  //! @brief Realloc internal array to fit @a to items.
  bool _realloc(sysuint_t to) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(to >= _length);

    T* p = reinterpret_cast<T*>(_data 
      ? ASMJIT_REALLOC(_data, to * sizeof(T)) 
      : ASMJIT_MALLOC(to * sizeof(T)));
    if (!p) return false;

    _data = p;
    _capacity = to;
    return true;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Items data.
  T* _data;
  //! @brief Length of buffer (count of items in array).
  sysuint_t _length;
  //! @brief Capacity of buffer (maximum items that can fit to current array).
  sysuint_t _capacity;

private:
  ASMJIT_DISABLE_COPY(PodVector<T>)
};

// ============================================================================
// [AsmJit::Zone]
// ============================================================================

//! @brief Memory allocator designed to fast alloc memory that will be freed
//! in one step.
//!
//! @note This is hackery for performance. Concept is that objects created
//! by @c Zone are freed all at once. This means that lifetime of 
//! these objects are same as zone object itselt.
//!
//! All emittables, variables, labels and states allocated by @c Compiler are
//! allocated through @c Zone object.
struct ASMJIT_API Zone
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new instance of @c Zone.
  //! @param chunkSize Default size for one zone chunk.
  Zone(sysuint_t chunkSize) ASMJIT_NOTHROW;

  //! @brief Destroy zone instance.
  ~Zone() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Allocate @c size bytes of memory and return pointer to it.
  //!
  //! Pointer allocated by this way will be valid until @c Zone object is
  //! destroyed. To create class by this way use placement @c new and 
  //! @c delete operators:
  //!
  //! @code
  //! // Example of allocating simple class
  //!
  //! // Your class
  //! class Object
  //! {
  //!   // members...
  //! };
  //!
  //! // Your function
  //! void f()
  //! {
  //!   // We are using AsmJit namespace
  //!   using namespace AsmJit
  //!
  //!   // Create zone object with chunk size of 65536 bytes.
  //!   Zone zone(65536);
  //!
  //!   // Create your objects using zone object allocating, for example:
  //!   Object* obj = new(zone.alloc(sizeof(YourClass))) Object();
  //! 
  //!   // ... lifetime of your objects ...
  //! 
  //!   // Destroy your objects:
  //!   obj->~Object();
  //!
  //!   // Zone destructor will free all memory allocated through it, 
  //!   // alternative is to call @c zone.freeAll().
  //! }
  //! @endcode
  void* zalloc(sysuint_t size) ASMJIT_NOTHROW;

  //! @brief Helper to duplicate string.
  char* zstrdup(const char* str) ASMJIT_NOTHROW;

  //! @brief Free all allocated memory except first block that remains for reuse.
  //!
  //! Note that this method will invalidate all instances using this memory
  //! allocated by this zone instance.
  void clear() ASMJIT_NOTHROW;

  //! @brief Free all allocated memory at once.
  //!
  //! Note that this method will invalidate all instances using this memory
  //! allocated by this zone instance.
  void freeAll() ASMJIT_NOTHROW;

  //! @brief Get total size of allocated objects - by @c alloc().
  inline sysuint_t getTotal() const ASMJIT_NOTHROW { return _total; }
  //! @brief Get (default) chunk size.
  inline sysuint_t getChunkSize() const ASMJIT_NOTHROW { return _chunkSize; }

  // --------------------------------------------------------------------------
  // [Chunk]
  // --------------------------------------------------------------------------

  //! @internal
  //!
  //! @brief One allocated chunk of memory.
  struct ASMJIT_HIDDEN Chunk
  {
    //! @brief Link to previous chunk.
    Chunk* prev;
    //! @brief Position in this chunk.
    sysuint_t pos;
    //! @brief Size of this chunk (in bytes).
    sysuint_t size;

    //! @brief Data.
    uint8_t data[sizeof(void*)];

    //! @brief Get count of remaining (unused) bytes in chunk.
    inline sysuint_t getRemainingBytes() const ASMJIT_NOTHROW { return size - pos; }
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

private:
  //! @brief Last allocated chunk of memory.
  Chunk* _chunks;
  //! @brief Total size of allocated objects - by @c alloc() method.
  sysuint_t _total;
  //! @brief One chunk size.
  sysuint_t _chunkSize;
};

//! @}

#pragma warning(default : 4127 4100)

} // AsmJit namespace

#endif // _ASMJIT_UTIL_H
