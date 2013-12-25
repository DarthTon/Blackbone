// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_UTIL_P_H
#define _ASMJIT_UTIL_P_H

// [Dependencies]
#include "Util.h"

#include <stdlib.h>
#include <string.h>

namespace AsmJit {

//! @addtogroup AsmJit_Util
//! @{

// ============================================================================
// [AsmJit::Util]
// ============================================================================

namespace Util
{
  // --------------------------------------------------------------------------
  // [AsmJit::floatAsInt32, int32AsFloat]
  // --------------------------------------------------------------------------

  //! @internal
  //!
  //! @brief used to cast from float to 32-bit integer and vica versa.
  union I32FPUnion
  {
    //! @brief 32-bit signed integer value.
    int32_t i;
    //! @brief 32-bit SP-FP value.
    float f;
  };

  //! @internal
  //!
  //! @brief used to cast from double to 64-bit integer and vica versa.
  union I64FPUnion
  {
    //! @brief 64-bit signed integer value.
    int64_t i;
    //! @brief 64-bit DP-FP value.
    double f;
  };

  //! @brief Binary cast from 32-bit integer to SP-FP value (@c float).
  static inline float int32AsFloat(int32_t i) ASMJIT_NOTHROW
  {
    I32FPUnion u;
    u.i = i;
    return u.f;
  }

  //! @brief Binary cast SP-FP value (@c float) to 32-bit integer.
  static inline int32_t floatAsInt32(float f) ASMJIT_NOTHROW
  {
    I32FPUnion u;
    u.f = f;
    return u.i;
  }

  //! @brief Binary cast from 64-bit integer to DP-FP value (@c double).
  static inline double int64AsDouble(int64_t i) ASMJIT_NOTHROW
  {
    I64FPUnion u;
    u.i = i;
    return u.f;
  }

  //! @brief Binary cast from DP-FP value (@c double) to 64-bit integer.
  static inline int64_t doubleAsInt64(double f) ASMJIT_NOTHROW
  {
    I64FPUnion u;
    u.f = f;
    return u.i;
  }

  // --------------------------------------------------------------------------
  // [Str Utils]
  // --------------------------------------------------------------------------

  ASMJIT_HIDDEN char* mycpy(char* dst, const char* src, sysuint_t len = (sysuint_t)-1) ASMJIT_NOTHROW;
  ASMJIT_HIDDEN char* myfill(char* dst, const int c, sysuint_t len) ASMJIT_NOTHROW;
  ASMJIT_HIDDEN char* myhex(char* dst, const uint8_t* src, sysuint_t len) ASMJIT_NOTHROW;
  ASMJIT_HIDDEN char* myutoa(char* dst, sysuint_t i, sysuint_t base = 10) ASMJIT_NOTHROW;
  ASMJIT_HIDDEN char* myitoa(char* dst, sysint_t i, sysuint_t base = 10) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Mem Utils]
  // --------------------------------------------------------------------------

  static inline void memset32(uint32_t* p, uint32_t c, sysuint_t len) ASMJIT_NOTHROW
  {
    sysuint_t i;
    for (i = 0; i < len; i++) p[i] = c;
  }
} // Util namespace

//! @}

} // AsmJit namespace

#endif // _ASMJIT_UTIL_P_H
