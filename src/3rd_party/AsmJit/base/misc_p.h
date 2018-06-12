// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_MISC_P_H
#define _ASMJIT_BASE_MISC_P_H

// [Dependencies]
#include "../asmjit_build.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

//! \internal
//!
//! Macro used to populate a table with 16 elements starting at `I`.
#define ASMJIT_TABLE_16(DEF, I) DEF(I +  0), DEF(I +  1), DEF(I +  2), DEF(I +  3), \
                                DEF(I +  4), DEF(I +  5), DEF(I +  6), DEF(I +  7), \
                                DEF(I +  8), DEF(I +  9), DEF(I + 10), DEF(I + 11), \
                                DEF(I + 12), DEF(I + 13), DEF(I + 14), DEF(I + 15)

#define ASMJIT_TABLE_T_8(TABLE, VALUE, I) \
  TABLE< I + 0 >::VALUE, TABLE< I + 1 >::VALUE, \
  TABLE< I + 2 >::VALUE, TABLE< I + 3 >::VALUE, \
  TABLE< I + 4 >::VALUE, TABLE< I + 5 >::VALUE, \
  TABLE< I + 6 >::VALUE, TABLE< I + 7 >::VALUE

#define ASMJIT_TABLE_T_16(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_8(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_8(TABLE, VALUE, I + 8)

#define ASMJIT_TABLE_T_32(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_16(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_16(TABLE, VALUE, I + 16)

#define ASMJIT_TABLE_T_64(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_32(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_32(TABLE, VALUE, I + 32)

#define ASMJIT_TABLE_T_128(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_64(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_64(TABLE, VALUE, I + 64)

#define ASMJIT_TABLE_T_256(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_128(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_128(TABLE, VALUE, I + 128)

#define ASMJIT_TABLE_T_512(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_256(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_256(TABLE, VALUE, I + 256)

#define ASMJIT_TABLE_T_1024(TABLE, VALUE, I) \
  ASMJIT_TABLE_T_512(TABLE, VALUE, I), \
  ASMJIT_TABLE_T_512(TABLE, VALUE, I + 512)

//! \}

} // asmjit namespace

//! \}

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_MISC_P_H
