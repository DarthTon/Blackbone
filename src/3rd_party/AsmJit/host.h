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

// ============================================================================
// [asmjit::host - X86 / X64]
// ============================================================================

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
#include "x86.h"

namespace asmjit {

// Define `asmjit::host` namespace wrapping `asmjit::x86`.
namespace host { using namespace ::asmjit::x86; }

// Define host assembler.
typedef X86Assembler HostAssembler;

// Define host operands.
typedef X86GpReg GpReg;
typedef X86FpReg FpReg;
typedef X86MmReg MmReg;
typedef X86XmmReg XmmReg;
typedef X86YmmReg YmmReg;
typedef X86SegReg SegReg;
typedef X86Mem Mem;

// Define host utilities.
typedef X86CpuInfo HostCpuInfo;

// Define host compiler and related.
#if !defined(ASMJIT_DISABLE_COMPILER)
typedef X86Compiler HostCompiler;
typedef X86CallNode HostCallNode;
typedef X86FuncDecl HostFuncDecl;
typedef X86FuncNode HostFuncNode;

typedef X86GpVar GpVar;
typedef X86MmVar MmVar;
typedef X86XmmVar XmmVar;
typedef X86YmmVar YmmVar;
#endif // !ASMJIT_DISABLE_COMPILER

} // asmjit namespace

#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64

// [Guard]
#endif // _ASMJIT_HOST_H
