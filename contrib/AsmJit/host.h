// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_HOST_H
#define _ASMJIT_HOST_H

// [Dependencies - Core]
#include "base.h"

// [Host - Helpers]
#define ASMJIT_USE_HOST(_Arch_) \
  namespace asmjit { \
    namespace host { \
      using namespace ::asmjit::_Arch_; \
    } \
  }

// [Host - X86]
#if defined(ASMJIT_HOST_X86)
#include "x86.h"
ASMJIT_USE_HOST(x86)
#endif // ASMJIT_HOST_X86

// [Host - X64]
#if defined(ASMJIT_HOST_X64)
#include "x86.h"
ASMJIT_USE_HOST(x64)
#endif // ASMJIT_HOST_X64

// [Host - Cleanup]
#undef ASMJIT_USE_HOST

// [Guard]
#endif // _ASMJIT_HOST_H
