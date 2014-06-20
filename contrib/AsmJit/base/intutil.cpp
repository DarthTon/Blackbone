// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/intutil.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

#if defined(ASMJIT_TEST)
UNIT(base_intutil) {
  uint32_t i;

  INFO("IntTraits<>.");
  EXPECT(IntTraits<signed char>::kIsSigned,
    "IntTraits<signed char> should report signed.");
  EXPECT(IntTraits<unsigned char>::kIsUnsigned,
    "IntTraits<unsigned char> should report unsigned.");

  EXPECT(IntTraits<signed short>::kIsSigned,
    "IntTraits<signed short> should report signed.");
  EXPECT(IntTraits<unsigned short>::kIsUnsigned,
    "IntTraits<unsigned short> should report unsigned.");

  EXPECT(IntTraits<int>::kIsSigned,
    "IntTraits<int> should report signed.");
  EXPECT(IntTraits<unsigned int>::kIsUnsigned,
    "IntTraits<unsigned int> should report unsigned.");

  EXPECT(IntTraits<long>::kIsSigned,
    "IntTraits<long> should report signed.");
  EXPECT(IntTraits<unsigned long>::kIsUnsigned,
    "IntTraits<unsigned long> should report unsigned.");

  EXPECT(IntTraits<intptr_t>::kIsSigned,
    "IntTraits<intptr_t> should report signed.");
  EXPECT(IntTraits<uintptr_t>::kIsUnsigned,
    "IntTraits<uintptr_t> should report unsigned.");

  EXPECT(IntTraits<intptr_t>::kIsIntPtr,
    "IntTraits<intptr_t> should report intptr_t type.");
  EXPECT(IntTraits<uintptr_t>::kIsIntPtr,
    "IntTraits<uintptr_t> should report intptr_t type.");

  INFO("IntUtil::iMin()/iMax().");
  EXPECT(IntUtil::iMin<int>(0, -1) == -1,
    "IntUtil::iMin<int> should return a minimum value.");
  EXPECT(IntUtil::iMin<int>(-1, -2) == -2,
    "IntUtil::iMin<int> should return a minimum value.");
  EXPECT(IntUtil::iMin<int>(1, 2) == 1,
    "IntUtil::iMin<int> should return a minimum value.");

  EXPECT(IntUtil::iMax<int>(0, -1) == 0,
    "IntUtil::iMax<int> should return a maximum value.");
  EXPECT(IntUtil::iMax<int>(-1, -2) == -1,
    "IntUtil::iMax<int> should return a maximum value.");
  EXPECT(IntUtil::iMax<int>(1, 2) == 2,
    "IntUtil::iMax<int> should return a maximum value.");

  INFO("IntUtil::inInterval().");
  EXPECT(IntUtil::inInterval<int>(11, 10, 20) == true,
    "IntUtil::inInterval<int> should return true if inside.");
  EXPECT(IntUtil::inInterval<int>(101, 10, 20) == false,
    "IntUtil::inInterval<int> should return false if outside.");

  INFO("IntUtil::isInt8().");
  EXPECT(IntUtil::isInt8(-128) == true,
    "IntUtil::isInt8<> should return true if inside.");
  EXPECT(IntUtil::isInt8(127) == true,
    "IntUtil::isInt8<> should return true if inside.");
  EXPECT(IntUtil::isInt8(-129) == false,
    "IntUtil::isInt8<> should return false if outside.");
  EXPECT(IntUtil::isInt8(128) == false,
    "IntUtil::isInt8<> should return false if outside.");

  INFO("IntUtil::isUInt8().");
  EXPECT(IntUtil::isUInt8(255) == true,
    "IntUtil::isUInt8<> should return true if inside.");
  EXPECT(IntUtil::isUInt8(256) == false,
    "IntUtil::isUInt8<> should return false if outside.");
  EXPECT(IntUtil::isUInt8(-1) == false,
    "IntUtil::isUInt8<> should return false if negative.");

  INFO("IntUtil::isInt16().");
  EXPECT(IntUtil::isInt16(-32768) == true,
    "IntUtil::isInt16<> should return true if inside.");
  EXPECT(IntUtil::isInt16(32767) == true,
    "IntUtil::isInt16<> should return true if inside.");
  EXPECT(IntUtil::isInt16(-32769) == false,
    "IntUtil::isInt16<> should return false if outside.");
  EXPECT(IntUtil::isInt16(32768) == false,
    "IntUtil::isInt16<> should return false if outside.");

  INFO("IntUtil::isUInt16().");
  EXPECT(IntUtil::isUInt16(65535) == true,
    "IntUtil::isUInt16<> should return true if inside.");
  EXPECT(IntUtil::isUInt16(65536) == false,
    "IntUtil::isUInt16<> should return false if outside.");
  EXPECT(IntUtil::isUInt16(-1) == false,
    "IntUtil::isUInt16<> should return false if negative.");

  INFO("IntUtil::isInt32().");
  EXPECT(IntUtil::isInt32(2147483647) == true,
    "IntUtil::isInt32<int> should return true if inside.");
  EXPECT(IntUtil::isInt32(-2147483647 - 1) == true,
    "IntUtil::isInt32<int> should return true if inside.");
  EXPECT(IntUtil::isInt32(ASMJIT_UINT64_C(2147483648)) == false,
    "IntUtil::isInt32<int> should return false if outside.");
  EXPECT(IntUtil::isInt32(ASMJIT_UINT64_C(0xFFFFFFFF)) == false,
    "IntUtil::isInt32<int> should return false if outside.");
  EXPECT(IntUtil::isInt32(ASMJIT_UINT64_C(0xFFFFFFFF) + 1) == false,
    "IntUtil::isInt32<int> should return false if outside.");

  INFO("IntUtil::isUInt32().");
  EXPECT(IntUtil::isUInt32(ASMJIT_UINT64_C(0xFFFFFFFF)) == true,
    "IntUtil::isUInt32<int> should return true if inside.");
  EXPECT(IntUtil::isUInt32(ASMJIT_UINT64_C(0xFFFFFFFF) + 1) == false,
    "IntUtil::isUInt32<int> should return false if outside.");
  EXPECT(IntUtil::isUInt32(-1) == false,
    "IntUtil::isUInt32<int> should return false if negative.");

  INFO("IntUtil::isPower2().");
  for (i = 0; i < 64; i++) {
    EXPECT(IntUtil::isPowerOf2(static_cast<uint64_t>(1) << i) == true,
      "IntUtil::isPower2() didn't report power of 2.");
    EXPECT(IntUtil::isPowerOf2((static_cast<uint64_t>(1) << i) ^ 0x001101) == false,
      "IntUtil::isPower2() didn't report not power of 2.");
  }

  INFO("IntUtil::mask().");
  for (i = 0; i < 32; i++) {
    EXPECT(IntUtil::mask(i) == (1 << i),
      "IntUtil::mask(%u) should return %X.", i, (1 << i));
  }

  INFO("IntUtil::bits().");
  for (i = 0; i < 32; i++) {
    uint32_t expectedBits = 0;

    for (uint32_t b = 0; b < i; b++)
      expectedBits |= static_cast<uint32_t>(1) << b;

    EXPECT(IntUtil::bits(i) == expectedBits,
      "IntUtil::bits(%u) should return %X.", i, expectedBits);
  }

  INFO("IntUtil::hasBit().");
  for (i = 0; i < 32; i++) {
    EXPECT(IntUtil::hasBit((1 << i), i) == true,
      "IntUtil::hasBit(%X, %u) should return true.", (1 << i), i);
  }

  INFO("IntUtil::bitCount().");
  for (i = 0; i < 32; i++) {
    EXPECT(IntUtil::bitCount((1 << i)) == 1,
      "IntUtil::bitCount(%X) should return true.", (1 << i));
  }
  EXPECT(IntUtil::bitCount(0x000000F0) ==  4, "");
  EXPECT(IntUtil::bitCount(0x10101010) ==  4, "");
  EXPECT(IntUtil::bitCount(0xFF000000) ==  8, "");
  EXPECT(IntUtil::bitCount(0xFFFFFFF7) == 31, "");
  EXPECT(IntUtil::bitCount(0x7FFFFFFF) == 31, "");

  INFO("IntUtil::findFirstBit().");
  for (i = 0; i < 32; i++) {
    EXPECT(IntUtil::findFirstBit((1 << i)) == i,
      "IntUtil::findFirstBit(%X) should return %u.", (1 << i), i);
  }

  INFO("IntUtil::isAligned().");
  EXPECT(IntUtil::isAligned<size_t>(0xFFFF,  4) == false, "");
  EXPECT(IntUtil::isAligned<size_t>(0xFFF4,  4) == true , "");
  EXPECT(IntUtil::isAligned<size_t>(0xFFF8,  8) == true , "");
  EXPECT(IntUtil::isAligned<size_t>(0xFFF0, 16) == true , "");

  INFO("IntUtil::alignTo().");
  EXPECT(IntUtil::alignTo<size_t>(0xFFFF,  4) == 0x10000, "");
  EXPECT(IntUtil::alignTo<size_t>(0xFFF4,  4) == 0x0FFF4, "");
  EXPECT(IntUtil::alignTo<size_t>(0xFFF8,  8) == 0x0FFF8, "");
  EXPECT(IntUtil::alignTo<size_t>(0xFFF0, 16) == 0x0FFF0, "");
  EXPECT(IntUtil::alignTo<size_t>(0xFFF0, 32) == 0x10000, "");

  INFO("IntUtil::alignToPowerOf2().");
  EXPECT(IntUtil::alignToPowerOf2<size_t>(0xFFFF) == 0x10000, "");
  EXPECT(IntUtil::alignToPowerOf2<size_t>(0xF123) == 0x10000, "");
  EXPECT(IntUtil::alignToPowerOf2<size_t>(0x0F00) == 0x01000, "");
  EXPECT(IntUtil::alignToPowerOf2<size_t>(0x0100) == 0x00100, "");
  EXPECT(IntUtil::alignToPowerOf2<size_t>(0x1001) == 0x02000, "");

  INFO("IntUtil::deltaTo().");
  EXPECT(IntUtil::deltaTo<size_t>(0xFFFF,  4) ==  1, "");
  EXPECT(IntUtil::deltaTo<size_t>(0xFFF4,  4) ==  0, "");
  EXPECT(IntUtil::deltaTo<size_t>(0xFFF8,  8) ==  0, "");
  EXPECT(IntUtil::deltaTo<size_t>(0xFFF0, 16) ==  0, "");
  EXPECT(IntUtil::deltaTo<size_t>(0xFFF0, 32) == 16, "");
}
#endif // ASMJIT_TEST

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
