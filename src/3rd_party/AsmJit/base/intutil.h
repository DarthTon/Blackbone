// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_INTUTIL_H
#define _ASMJIT_BASE_INTUTIL_H

// [Dependencies - AsmJit]
#include "../base/globals.h"

#if defined(_MSC_VER)
#pragma intrinsic(_BitScanForward)
#endif // ASMJIT_OS_WINDOWS

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_util
//! \{

// ============================================================================
// [asmjit::IntTraits]
// ============================================================================

//! \internal
template<typename T>
struct IntTraits {
  enum {
    kIsSigned = static_cast<T>(~static_cast<T>(0)) < static_cast<T>(0),
    kIsUnsigned = !kIsSigned,

    kIs8Bit = sizeof(T) == 1,
    kIs16Bit = sizeof(T) == 2,
    kIs32Bit = sizeof(T) == 4,
    kIs64Bit = sizeof(T) == 8,

    kIsIntPtr = sizeof(T) == sizeof(intptr_t)
  };
};

// ============================================================================
// [asmjit::IntUtil]
// ============================================================================

//! Integer utilities.
struct IntUtil {
  // --------------------------------------------------------------------------
  // [Float <-> Int]
  // --------------------------------------------------------------------------

  //! \internal
  union Float {
    int32_t i;
    float f;
  };

  //! \internal
  union Double {
    int64_t i;
    double d;
  };

  //! Bit-cast `float` to 32-bit integer.
  static ASMJIT_INLINE int32_t floatAsInt(float f) { Float m; m.f = f; return m.i; }
  //! Bit-cast 32-bit integer to `float`.
  static ASMJIT_INLINE float intAsFloat(int32_t i) { Float m; m.i = i; return m.f; }

  //! Bit-cast `double` to 64-bit integer.
  static ASMJIT_INLINE int64_t doubleAsInt(double d) { Double m; m.d = d; return m.i; }
  //! Bit-cast 64-bit integer to `double`.
  static ASMJIT_INLINE double intAsDouble(int64_t i) { Double m; m.i = i; return m.d; }

  // --------------------------------------------------------------------------
  // [AsmJit - Pack / Unpack]
  // --------------------------------------------------------------------------

  //! Pack two 8-bit integer and one 16-bit integer into a 32-bit integer as it
  //! is an array of `{u0,u1,w2}`.
  static ASMJIT_INLINE uint32_t pack32_2x8_1x16(uint32_t u0, uint32_t u1, uint32_t w2) {
#if defined(ASMJIT_HOST_LE)
    return u0 + (u1 << 8) + (w2 << 16);
#else
    return (u0 << 24) + (u1 << 16) + (w2);
#endif // ASMJIT_HOST
  }

  //! Pack four 8-bit integer into a 32-bit integer as it is an array of `{u0,u1,u2,u3}`.
  static ASMJIT_INLINE uint32_t pack32_4x8(uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3) {
#if defined(ASMJIT_HOST_LE)
    return u0 + (u1 << 8) + (u2 << 16) + (u3 << 24);
#else
    return (u0 << 24) + (u1 << 16) + (u2 << 8) + u3;
#endif // ASMJIT_HOST
  }

  //! Pack two 32-bit integer into a 64-bit integer as it is an array of `{u0,u1}`.
  static ASMJIT_INLINE uint64_t pack64_2x32(uint32_t u0, uint32_t u1) {
#if defined(ASMJIT_HOST_LE)
    return (static_cast<uint64_t>(u1) << 32) + u0;
#else
    return (static_cast<uint64_t>(u0) << 32) + u1;
#endif // ASMJIT_HOST
  }

  // --------------------------------------------------------------------------
  // [AsmJit - Min/Max]
  // --------------------------------------------------------------------------

  // NOTE: Because some environments declare min() and max() as macros, it has
  // been decided to use different name so we never collide with them.

  //! Get minimum value of `a` and `b`.
  template<typename T>
  static ASMJIT_INLINE T iMin(const T& a, const T& b) { return a < b ? a : b; }

  //! Get maximum value of `a` and `b`.
  template<typename T>
  static ASMJIT_INLINE T iMax(const T& a, const T& b) { return a > b ? a : b; }

  // --------------------------------------------------------------------------
  // [AsmJit - MaxUInt]
  // --------------------------------------------------------------------------

  //! Get maximum unsigned value of `T`.
  template<typename T>
  static ASMJIT_INLINE T maxUInt() { return ~T(0); }

  // --------------------------------------------------------------------------
  // [AsmJit - InInterval]
  // --------------------------------------------------------------------------

  //! Get whether `x` is greater or equal than `start` and less or equal than `end`.
  template<typename T>
  static ASMJIT_INLINE bool inInterval(const T& x, const T& start, const T& end) {
    return x >= start && x <= end;
  }

  // --------------------------------------------------------------------------
  // [AsmJit - IsInt/IsUInt]
  // --------------------------------------------------------------------------

  //! Get whether the given integer `x` can be casted to 8-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt8(T x) {
    if (IntTraits<T>::kIsSigned)
      return sizeof(T) <= sizeof(int8_t) ? true : x >= T(-128) && x <= T(127);
    else
      return x <= T(127);
  }

  //! Get whether the given integer `x` can be casted to 8-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt8(T x) {
    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= sizeof(uint8_t) ? true : x <= T(255));
    else
      return sizeof(T) <= sizeof(uint8_t) ? true : x <= T(255);
  }

  //! Get whether the given integer `x` can be casted to 16-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt16(T x) {
    if (IntTraits<T>::kIsSigned)
      return sizeof(T) <= sizeof(int16_t) ? true : x >= T(-32768) && x <= T(32767);
    else
      return x >= T(0) && (sizeof(T) <= sizeof(int16_t) ? true : x <= T(32767));
  }

  //! Get whether the given integer `x` can be casted to 16-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt16(T x) {
    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= sizeof(uint16_t) ? true : x <= T(65535));
    else
      return sizeof(T) <= sizeof(uint16_t) ? true : x <= T(65535);
  }

  //! Get whether the given integer `x` can be casted to 32-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt32(T x) {
    if (IntTraits<T>::kIsSigned)
      return sizeof(T) <= sizeof(int32_t) ? true : x >= T(-2147483647) - 1 && x <= T(2147483647);
    else
      return x >= T(0) && (sizeof(T) <= sizeof(int32_t) ? true : x <= T(2147483647));
  }

  //! Get whether the given integer `x` can be casted to 32-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt32(T x) {
    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= sizeof(uint32_t) ? true : x <= T(4294967295U));
    else
      return sizeof(T) <= sizeof(uint32_t) ? true : x <= T(4294967295U);
  }

  // --------------------------------------------------------------------------
  // [AsmJit - IsPowerOf2]
  // --------------------------------------------------------------------------

  //! Get whether the `n` value is a power of two (only one bit is set).
  template<typename T>
  static ASMJIT_INLINE bool isPowerOf2(T n) {
    return n != 0 && (n & (n - 1)) == 0;
  }

  // --------------------------------------------------------------------------
  // [AsmJit - Mask]
  // --------------------------------------------------------------------------

  //! Generate a bit-mask that has `x` bit set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x) {
    ASMJIT_ASSERT(x < 32);
    return (1U << x);
  }

  //! Generate a bit-mask that has `x0` and `x1` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1) {
    return mask(x0) | mask(x1);
  }

  //! Generate a bit-mask that has `x0`, `x1` and `x2` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2) {
    return mask(x0) | mask(x1) | mask(x2);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2` and `x3` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3` and `x4` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3) |
           mask(x4) ;
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4` and `x5` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3) |
           mask(x4) | mask(x5) ;
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5` and `x6` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3) |
           mask(x4) | mask(x5) | mask(x6) ;
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5`, `x6` and `x7` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6, uint32_t x7) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3) |
           mask(x4) | mask(x5) | mask(x6) | mask(x7) ;
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5`, `x6`, `x7` and `x8` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6, uint32_t x7, uint32_t x8) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3) |
           mask(x4) | mask(x5) | mask(x6) | mask(x7) |
           mask(x8) ;
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5`, `x6`, `x7`, `x8` and `x9` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6, uint32_t x7, uint32_t x8, uint32_t x9) {
    return mask(x0) | mask(x1) | mask(x2) | mask(x3) |
           mask(x4) | mask(x5) | mask(x6) | mask(x7) |
           mask(x8) | mask(x9) ;
  }

  // --------------------------------------------------------------------------
  // [AsmJit - Bits]
  // --------------------------------------------------------------------------

  //! Generate a bit-mask that has `x` most significant bits set.
  static ASMJIT_INLINE uint32_t bits(uint32_t x) {
    // Shifting more bits that the type has has undefined behavior. Everything
    // we need is that application shouldn't crash because of that, but the
    // content of register after shift is not defined. So in case that the
    // requested shift is too large for the type we correct this undefined
    // behavior by setting all bits to ones (this is why we generate an overflow
    // mask).
    uint32_t overflow = static_cast<uint32_t>(
      -static_cast<int32_t>(x >= sizeof(uint32_t) * 8));

    return ((static_cast<uint32_t>(1) << x) - 1U) | overflow;
  }

  // --------------------------------------------------------------------------
  // [AsmJit - HasBit]
  // --------------------------------------------------------------------------

  //! Get whether `x` has bit `n` set.
  static ASMJIT_INLINE bool hasBit(uint32_t x, uint32_t n) {
    return static_cast<bool>((x >> n) & 0x1);
  }

  // --------------------------------------------------------------------------
  // [AsmJit - BitCount]
  // --------------------------------------------------------------------------

  //! Get count of bits in `x`.
  //!
  //! Taken from http://graphics.stanford.edu/~seander/bithacks.html .
  static ASMJIT_INLINE uint32_t bitCount(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555U);
    x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
    return (((x + (x >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
  }

  // --------------------------------------------------------------------------
  // [AsmJit - FindFirstBit]
  // --------------------------------------------------------------------------

  //! \internal
  static ASMJIT_INLINE uint32_t findFirstBitSlow(uint32_t mask) {
    // This is a reference (slow) implementation of findFirstBit(), used when
    // we don't have compiler support for this task. The implementation speed
    // has been improved to check for 2 bits per iteration.
    uint32_t i = 1;

    while (mask != 0) {
      uint32_t two = mask & 0x3;
      if (two != 0x0)
        return i - (two & 0x1);

      i += 2;
      mask >>= 2;
    }

    return 0xFFFFFFFFU;
  }

  //! Find a first bit in `mask`.
  static ASMJIT_INLINE uint32_t findFirstBit(uint32_t mask) {
#if defined(_MSC_VER)
    DWORD i;
    if (_BitScanForward(&i, mask)) {
      ASMJIT_ASSERT(findFirstBitSlow(mask) == i);
      return static_cast<uint32_t>(i);
    }
    return 0xFFFFFFFFU;
#else
    return findFirstBitSlow(mask);
#endif
  }

  // --------------------------------------------------------------------------
  // [AsmJit - Misc]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE uint32_t keepNOnesFromRight(uint32_t mask, uint32_t nBits) {
    uint32_t m = 0x1;

    do {
      nBits -= (mask & m) == 0;
      m <<= 1;
      if (nBits == 0) {
        m -= 1;
        mask &= m;
        break;
      }
    } while (m);

    return mask;
  }

  static ASMJIT_INLINE uint32_t indexNOnesFromRight(uint8_t* dst, uint32_t mask, uint32_t nBits) {
    uint32_t totalBits = nBits;
    uint8_t i = 0;
    uint32_t m = 0x1;

    do {
      if (mask & m) {
        *dst++ = i;
        if (--nBits == 0)
          break;
      }

      m <<= 1;
      i++;
    } while (m);

    return totalBits - nBits;
  }

  // --------------------------------------------------------------------------
  // [AsmJit - Alignment]
  // --------------------------------------------------------------------------

  template<typename T>
  static ASMJIT_INLINE bool isAligned(T base, T alignment) {
    return (base % alignment) == 0;
  }

  //! Align `base` to `alignment`.
  template<typename T>
  static ASMJIT_INLINE T alignTo(T base, T alignment) {
    return (base + (alignment - 1)) & ~(alignment - 1);
  }

  template<typename T>
  static ASMJIT_INLINE T alignToPowerOf2(T base) {
    // Implementation is from "Hacker's Delight" by Henry S. Warren, Jr.
    base -= 1;

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4293)
#endif // _MSC_VER

    base = base | (base >> 1);
    base = base | (base >> 2);
    base = base | (base >> 4);

    // 8/16/32 constants are multiplied by the condition to prevent a compiler
    // complaining about the 'shift count >= type width' (GCC).
    if (sizeof(T) >= 2) base = base | (base >> ( 8 * (sizeof(T) >= 2))); // Base >>  8.
    if (sizeof(T) >= 4) base = base | (base >> (16 * (sizeof(T) >= 4))); // Base >> 16.
    if (sizeof(T) >= 8) base = base | (base >> (32 * (sizeof(T) >= 8))); // Base >> 32.

#if defined(_MSC_VER)
# pragma warning(pop)
#endif // _MSC_VER

    return base + 1;
  }

  //! Get delta required to align `base` to `alignment`.
  template<typename T>
  static ASMJIT_INLINE T deltaTo(T base, T alignment) {
    return alignTo(base, alignment) - base;
  }
};

// ============================================================================
// [asmjit::UInt64]
// ============================================================================

union UInt64 {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64 fromUInt64(uint64_t val) {
    UInt64 data;
    data.setUInt64(val);
    return data;
  }

  ASMJIT_INLINE UInt64 fromUInt64(const UInt64& val) {
    UInt64 data;
    data.setUInt64(val);
    return data;
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() {
    if (kArchHost64Bit) {
      u64 = 0;
    }
    else {
      u32[0] = 0;
      u32[1] = 0;
    }
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint64_t getUInt64() const {
    return u64;
  }

  ASMJIT_INLINE UInt64& setUInt64(uint64_t val) {
    u64 = val;
    return *this;
  }

  ASMJIT_INLINE UInt64& setUInt64(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 = val.u64;
    }
    else {
      u32[0] = val.u32[0];
      u32[1] = val.u32[1];
    }
    return *this;
  }

  ASMJIT_INLINE UInt64& setPacked_2x32(uint32_t u0, uint32_t u1) {
    if (kArchHost64Bit) {
      u64 = IntUtil::pack64_2x32(u0, u1);
    }
    else {
      u32[0] = u0;
      u32[1] = u1;
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Add]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& add(uint64_t val) {
    u64 += val;
    return *this;
  }

  ASMJIT_INLINE UInt64& add(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 += val.u64;
    }
    else {
      u32[0] += val.u32[0];
      u32[1] += val.u32[1];
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Sub]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& sub(uint64_t val) {
    u64 -= val;
    return *this;
  }

  ASMJIT_INLINE UInt64& sub(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 -= val.u64;
    }
    else {
      u32[0] -= val.u32[0];
      u32[1] -= val.u32[1];
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [And]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& and_(uint64_t val) {
    u64 &= val;
    return *this;
  }

  ASMJIT_INLINE UInt64& and_(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 &= val.u64;
    }
    else {
      u32[0] &= val.u32[0];
      u32[1] &= val.u32[1];
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [AndNot]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& andNot(uint64_t val) {
    u64 &= ~val;
    return *this;
  }

  ASMJIT_INLINE UInt64& andNot(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 &= ~val.u64;
    }
    else {
      u32[0] &= ~val.u32[0];
      u32[1] &= ~val.u32[1];
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Or]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& or_(uint64_t val) {
    u64 |= val;
    return *this;
  }

  ASMJIT_INLINE UInt64& or_(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 |= val.u64;
    }
    else {
      u32[0] |= val.u32[0];
      u32[1] |= val.u32[1];
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Xor]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& xor_(uint64_t val) {
    u64 ^= val;
    return *this;
  }

  ASMJIT_INLINE UInt64& xor_(const UInt64& val) {
    if (kArchHost64Bit) {
      u64 ^= val.u64;
    }
    else {
      u32[0] ^= val.u32[0];
      u32[1] ^= val.u32[1];
    }
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isZero() const {
    return kArchHost64Bit ? u64 == 0 : (u32[0] | u32[1]) == 0;
  }

  ASMJIT_INLINE bool isNonZero() const {
    return kArchHost64Bit ? u64 != 0 : (u32[0] | u32[1]) != 0;
  }

  ASMJIT_INLINE bool eq(uint64_t val) const {
    return u64 == val;
  }

  ASMJIT_INLINE bool eq(const UInt64& val) const {
    return kArchHost64Bit ? u64 == val.u64 : (u32[0] == val.u32[0]) & (u32[1] == val.u32[1]);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE UInt64& operator+=(uint64_t val) { return add(val); }
  ASMJIT_INLINE UInt64& operator+=(const UInt64& val) { return add(val); }

  ASMJIT_INLINE UInt64& operator-=(uint64_t val) { return sub(val); }
  ASMJIT_INLINE UInt64& operator-=(const UInt64& val) { return sub(val); }

  ASMJIT_INLINE UInt64& operator&=(uint64_t val) { return and_(val); }
  ASMJIT_INLINE UInt64& operator&=(const UInt64& val) { return and_(val); }

  ASMJIT_INLINE UInt64& operator|=(uint64_t val) { return or_(val); }
  ASMJIT_INLINE UInt64& operator|=(const UInt64& val) { return or_(val); }

  ASMJIT_INLINE UInt64& operator^=(uint64_t val) { return xor_(val); }
  ASMJIT_INLINE UInt64& operator^=(const UInt64& val) { return xor_(val); }

  ASMJIT_INLINE bool operator==(uint64_t val) const { return eq(val); }
  ASMJIT_INLINE bool operator==(const UInt64& val) const { return eq(val); }

  ASMJIT_INLINE bool operator!=(uint64_t val) const { return !eq(val); }
  ASMJIT_INLINE bool operator!=(const UInt64& val) const { return !eq(val); }

  ASMJIT_INLINE bool operator<(uint64_t val) const { return u64 < val; }
  ASMJIT_INLINE bool operator<(const UInt64& val) const { return u64 < val.u64; }

  ASMJIT_INLINE bool operator<=(uint64_t val) const { return u64 <= val; }
  ASMJIT_INLINE bool operator<=(const UInt64& val) const { return u64 <= val.u64; }

  ASMJIT_INLINE bool operator>(uint64_t val) const { return u64 > val; }
  ASMJIT_INLINE bool operator>(const UInt64& val) const { return u64 > val.u64; }

  ASMJIT_INLINE bool operator>=(uint64_t val) const { return u64 >= val; }
  ASMJIT_INLINE bool operator>=(const UInt64& val) const { return u64 >= val.u64; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint64_t u64;

  uint32_t u32[2];
  uint16_t u16[4];
  uint8_t u8[8];

  struct {
#if defined(ASMJIT_HOST_LE)
    uint32_t lo, hi;
#else
    uint32_t hi, lo;
#endif // ASMJIT_HOST_LE
  };
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_INTUTIL_H
