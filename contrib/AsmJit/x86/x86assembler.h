// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86ASSEMBLER_H
#define _ASMJIT_X86_X86ASSEMBLER_H

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../x86/x86inst.h"
#include "../x86/x86operand.h"
#include "../x86/x86regs.h"
#include "../x86/x86util.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [CodeGen-Begin]
// ============================================================================

#define INST_0x(_Inst_, _Code_) \
  ASMJIT_INLINE Error _Inst_() { \
    return emit(_Code_); \
  }

#define INST_1x(_Inst_, _Code_, _Op0_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0) { \
    return emit(_Code_, o0); \
  }

#define INST_1x_(_Inst_, _Code_, _Op0_, _Cond_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0); \
  }

#define INST_1i(_Inst_, _Code_, _Op0_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0) { \
    return emit(_Code_, o0); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(int o0) { \
    return emit(_Code_, o0); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(unsigned int o0) { \
    return emit(_Code_, static_cast<uint64_t>(o0)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(int64_t o0) { \
    return emit(_Code_, static_cast<uint64_t>(o0)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(uint64_t o0) { \
    return emit(_Code_, o0); \
  }

#define INST_1cc(_Inst_, _Code_, _Translate_, _Op0_) \
  ASMJIT_INLINE Error _Inst_(uint32_t cc, const _Op0_& o0) { \
    return emit(_Translate_(cc), o0); \
  } \
  \
  ASMJIT_INLINE Error _Inst_##a(const _Op0_& o0) { return emit(_Code_##a, o0); } \
  ASMJIT_INLINE Error _Inst_##ae(const _Op0_& o0) { return emit(_Code_##ae, o0); } \
  ASMJIT_INLINE Error _Inst_##b(const _Op0_& o0) { return emit(_Code_##b, o0); } \
  ASMJIT_INLINE Error _Inst_##be(const _Op0_& o0) { return emit(_Code_##be, o0); } \
  ASMJIT_INLINE Error _Inst_##c(const _Op0_& o0) { return emit(_Code_##c, o0); } \
  ASMJIT_INLINE Error _Inst_##e(const _Op0_& o0) { return emit(_Code_##e, o0); } \
  ASMJIT_INLINE Error _Inst_##g(const _Op0_& o0) { return emit(_Code_##g, o0); } \
  ASMJIT_INLINE Error _Inst_##ge(const _Op0_& o0) { return emit(_Code_##ge, o0); } \
  ASMJIT_INLINE Error _Inst_##l(const _Op0_& o0) { return emit(_Code_##l, o0); } \
  ASMJIT_INLINE Error _Inst_##le(const _Op0_& o0) { return emit(_Code_##le, o0); } \
  ASMJIT_INLINE Error _Inst_##na(const _Op0_& o0) { return emit(_Code_##na, o0); } \
  ASMJIT_INLINE Error _Inst_##nae(const _Op0_& o0) { return emit(_Code_##nae, o0); } \
  ASMJIT_INLINE Error _Inst_##nb(const _Op0_& o0) { return emit(_Code_##nb, o0); } \
  ASMJIT_INLINE Error _Inst_##nbe(const _Op0_& o0) { return emit(_Code_##nbe, o0); } \
  ASMJIT_INLINE Error _Inst_##nc(const _Op0_& o0) { return emit(_Code_##nc, o0); } \
  ASMJIT_INLINE Error _Inst_##ne(const _Op0_& o0) { return emit(_Code_##ne, o0); } \
  ASMJIT_INLINE Error _Inst_##ng(const _Op0_& o0) { return emit(_Code_##ng, o0); } \
  ASMJIT_INLINE Error _Inst_##nge(const _Op0_& o0) { return emit(_Code_##nge, o0); } \
  ASMJIT_INLINE Error _Inst_##nl(const _Op0_& o0) { return emit(_Code_##nl, o0); } \
  ASMJIT_INLINE Error _Inst_##nle(const _Op0_& o0) { return emit(_Code_##nle, o0); } \
  ASMJIT_INLINE Error _Inst_##no(const _Op0_& o0) { return emit(_Code_##no, o0); } \
  ASMJIT_INLINE Error _Inst_##np(const _Op0_& o0) { return emit(_Code_##np, o0); } \
  ASMJIT_INLINE Error _Inst_##ns(const _Op0_& o0) { return emit(_Code_##ns, o0); } \
  ASMJIT_INLINE Error _Inst_##nz(const _Op0_& o0) { return emit(_Code_##nz, o0); } \
  ASMJIT_INLINE Error _Inst_##o(const _Op0_& o0) { return emit(_Code_##o, o0); } \
  ASMJIT_INLINE Error _Inst_##p(const _Op0_& o0) { return emit(_Code_##p, o0); } \
  ASMJIT_INLINE Error _Inst_##pe(const _Op0_& o0) { return emit(_Code_##pe, o0); } \
  ASMJIT_INLINE Error _Inst_##po(const _Op0_& o0) { return emit(_Code_##po, o0); } \
  ASMJIT_INLINE Error _Inst_##s(const _Op0_& o0) { return emit(_Code_##s, o0); } \
  ASMJIT_INLINE Error _Inst_##z(const _Op0_& o0) { return emit(_Code_##z, o0); }

#define INST_2x(_Inst_, _Code_, _Op0_, _Op1_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1) { \
    return emit(_Code_, o0, o1); \
  }

#define INST_2x_(_Inst_, _Code_, _Op0_, _Op1_, _Cond_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0, o1); \
  }

#define INST_2i(_Inst_, _Code_, _Op0_, _Op1_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1) { \
    return emit(_Code_, o0, o1); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, int o1) { \
    return emit(_Code_, o0, o1); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, unsigned int o1) { \
    return emit(_Code_, o0, static_cast<uint64_t>(o1)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, int64_t o1) { \
    return emit(_Code_, o0, static_cast<uint64_t>(o1)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, uint64_t o1) { \
    return emit(_Code_, o0, o1); \
  }

#define INST_2cc(_Inst_, _Code_, _Translate_, _Op0_, _Op1_) \
  ASMJIT_INLINE Error _Inst_(uint32_t cc, const _Op0_& o0, const _Op1_& o1) { \
    return emit(_Translate_(cc), o0, o1); \
  } \
  \
  ASMJIT_INLINE Error _Inst_##a(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##a, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##ae(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ae, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##b(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##b, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##be(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##be, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##c(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##c, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##e(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##e, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##g(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##g, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##ge(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ge, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##l(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##l, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##le(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##le, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##na(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##na, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nae(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nae, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nb(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nb, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nbe(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nbe, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nc(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nc, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##ne(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ne, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##ng(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ng, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nge(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nge, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nl(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nl, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nle(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nle, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##no(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##no, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##np(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##np, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##ns(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ns, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##nz(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nz, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##o(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##o, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##p(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##p, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##pe(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##pe, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##po(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##po, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##s(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##s, o0, o1); } \
  ASMJIT_INLINE Error _Inst_##z(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##z, o0, o1); }

#define INST_3x(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    return emit(_Code_, o0, o1, o2); \
  }

#define INST_3x_(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Cond_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0, o1, o2); \
  }

#define INST_3i(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    return emit(_Code_, o0, o1, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, int o2) { \
    return emit(_Code_, o0, o1, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, unsigned int o2) { \
    return emit(_Code_, o0, o1, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, int64_t o2) { \
    return emit(_Code_, o0, o1, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, uint64_t o2) { \
    return emit(_Code_, o0, o1, o2); \
  }

#define INST_4x(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Op3_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  }

#define INST_4x_(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Op3_, _Cond_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0, o1, o2, o3); \
  }

#define INST_4i(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Op3_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, int o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, unsigned int o3) { \
    return emit(_Code_, o0, o1, o2, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, int64_t o3) { \
    return emit(_Code_, o0, o1, o2, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, uint64_t o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  }

#define ASMJIT_X86X64_EMIT_OPTIONS(_Class_) \
  /*! Force short form of jmp/jcc instruction. */ \
  ASMJIT_INLINE _Class_& short_() { \
    _options |= kInstOptionShortForm; \
    return *this; \
  } \
  \
  /*! Force long form of jmp/jcc instruction. */ \
  ASMJIT_INLINE _Class_& long_() { \
    _options |= kInstOptionLongForm; \
    return *this; \
  } \
  \
  /*! Condition is likely to be taken (has only benefit on P4). */ \
  ASMJIT_INLINE _Class_& taken() { \
    _options |= kInstOptionTaken; \
    return *this; \
  } \
  \
  /*! Condition is unlikely to be taken (has only benefit on P4). */ \
  ASMJIT_INLINE _Class_& notTaken() { \
    _options |= kInstOptionNotTaken; \
    return *this; \
  } \
  \
  /*! Use LOCK prefix. */ \
  ASMJIT_INLINE _Class_& lock() { \
    _options |= kInstOptionLock; \
    return *this; \
  } \
  \
  /*! Force REX prefix. */ \
  ASMJIT_INLINE _Class_& rex() { \
    _options |= kInstOptionRex; \
    return *this; \
  } \
  \
  /*! Force 3-byte VEX prefix. */ \
  ASMJIT_INLINE _Class_& vex3() { \
    _options |= kInstOptionVex3; \
    return *this; \
  }

//! \addtogroup asmjit_x86x64_general
//! \{

// ============================================================================
// [asmjit::x86x64::X86X64Assembler]
// ============================================================================

//! X86/X64 assembler.
//!
//! Assembler is the main class in AsmJit that can encode instructions and their
//! operands to a binary stream runnable by CPU. It creates internal buffer
//! where the encodes instructions are stored and it contains intrinsics that
//! can be used to emit the code in a convenent way. Code generation is in
//! general safe, because the intrinsics uses method overloading so even the
//! code is emitted it can be checked by a C++ compiler. It's nearly impossible
//! to create invalid instruction, for example `mov [eax], [eax]`, because such
//! overload doesn't exist.
//!
//! Each call to an assembler intrinsic function emits instruction directly
//! to the binary stream. There are also runtime checks that prevent invalid
//! code to be emitted. It will assert in debug mode and put the `BaseAssembler`
//! instance to an  error state in production mode.
//!
//! Code Generation
//! ---------------
//!
//! To generate code is only needed to create instance of `BaseAssembler`
//! and to use intrinsics. See example how to do that:
//!
//! ~~~
//! // Use asmjit namespace.
//! using namespace asmjit;
//! using namespace asmjit::host;
//!
//! // Create Assembler instance.
//! Assembler a;
//!
//! // Prolog.
//! a->push(ebp);
//! a->mov(ebp, esp);
//!
//! // Mov 1024 to EAX, EAX is also return value.
//! a->mov(eax, 1024);
//!
//! // Epilog.
//! a->mov(esp, ebp);
//! a->pop(ebp);
//!
//! // Return.
//! a->ret();
//! ~~~
//!
//! You can see that syntax is very close to Intel one. Only difference is that
//! you are calling functions that emits the binary code for you. All registers
//! are in `asmjit` namespace, so it's very comfortable to use it (look at
//! first line). There is also used method `imm()` to create an immediate value.
//! Use `imm_u()` to create unsigned immediate value.
//!
//! There is also possibility to use memory addresses and immediates. Use
//! `ptr()`, `byte_ptr()`, `word_ptr()`, `dword_ptr()` and similar functions to
//! build a memory address operand. In most cases `ptr()` is enough, because an
//! information related to the operand size is needed only in rare cases, that
//! is an instruction without having any register operands, such as `inc [mem]`.
//!
//! for example, `a` is `x86::BaseAssembler` instance:
//!
//! ~~~
//! a->mov(ptr(eax), 0);             // mov ptr [eax], 0
//! a->mov(ptr(eax), edx);           // mov ptr [eax], edx
//! ~~~
//!
//! But it's also possible to create complex addresses:
//!
//! ~~~
//! // eax + ecx*x addresses
//! a->mov(ptr(eax, ecx, 0), 0);     // mov ptr [eax + ecx], 0
//! a->mov(ptr(eax, ecx, 1), 0);     // mov ptr [eax + ecx * 2], 0
//! a->mov(ptr(eax, ecx, 2), 0);     // mov ptr [eax + ecx * 4], 0
//! a->mov(ptr(eax, ecx, 3), 0);     // mov ptr [eax + ecx * 8], 0
//! // eax + ecx*x + disp addresses
//! a->mov(ptr(eax, ecx, 0,  4), 0); // mov ptr [eax + ecx     +  4], 0
//! a->mov(ptr(eax, ecx, 1,  8), 0); // mov ptr [eax + ecx * 2 +  8], 0
//! a->mov(ptr(eax, ecx, 2, 12), 0); // mov ptr [eax + ecx * 4 + 12], 0
//! a->mov(ptr(eax, ecx, 3, 16), 0); // mov ptr [eax + ecx * 8 + 16], 0
//! ~~~
//!
//! All addresses shown are using `ptr()` to make memory operand. Some assembler
//! instructions (single operand ones) needs to have specified memory operand
//! size. For example `a->inc(ptr(eax))` can't be called, because the meaning is
//! ambiguous, see the code below.
//!
//! ~~~
//! // [byte] address.
//! a->inc(byte_ptr(eax));           // Inc byte ptr [eax].
//! a->dec(byte_ptr(eax));           // Dec byte ptr [eax].
//! // [word] address.
//! a->inc(word_ptr(eax));           // Inc word ptr [eax].
//! a->dec(word_ptr(eax));           // Dec word ptr [eax].
//! // [dword] address.
//! a->inc(dword_ptr(eax));          // Inc dword ptr [eax].
//! a->dec(dword_ptr(eax));          // Dec dword ptr [eax].
//! ~~~
//!
//! Calling JIT Code
//! ----------------
//!
//! While you are over from emitting instructions, you can make your function
//! by using `BaseAssembler::make()` method. This method will use memory
//! manager to allocate virtual memory and relocates generated code to it. For
//! memory allocation is used global memory manager by default and memory is
//! freeable, but of course this default behavior can be overridden specifying
//! your memory manager and allocation type. If you want to do with code
//! something else you can always override make() method and do what you want.
//!
//! You can get size of generated code by `getCodeSize()` or `getOffset()`
//! methods. These methods returns you code size or more precisely the current
//! code offset in bytes. The `takeCode()` function can be used to take the
//! internal buffer and clear the code generator, but the buffer taken has to
//! be freed manually.
//!
//! Machine code can be executed only in memory that is marked executable. This
//! mark is usually not set for memory returned by a C/C++ `malloc` function.
//! The `VMem::alloc()` function can be used allocate a memory where the code can
//! be executed or more preferably `VMemMgr` which has interface
//! similar to `malloc/free` and can allocate chunks of various sizes.
//!
//! The next example shows how to allocate memory where the code can be executed:
//!
//! ~~~
//! using namespace asmjit;
//!
//! JitRuntime runtime;
//! Assembler a(&runtime);
//!
//! // ... Your code generation ...
//!
//! // The function prototype
//! typedef void (*MyFunc)();
//!
//! // make your function
//! MyFunc func = asmjit_cast<MyFunc>(a->make());
//!
//! // call your function
//! func();
//!
//! // If you don't need your function again, free it.
//! runtime.release(func);
//! ~~~
//!
//! This was a very primitive showing how the generated code can be executed.
//! In production noone will probably generate a function that is only called
//! once and nobody will probably free the function right after it was executed.
//! The code just shows the proper way of code generation and cleanup.
//!
//! Labels
//! ------
//!
//! While generating assembler code, you will usually need to create complex
//! code with labels. Labels are fully supported and you can call `jmp` or
//! `je` (and similar) instructions to initialized or yet uninitialized label.
//! Each label expects to be bound into offset. To bind label to specific
//! offset, use `CodeGen::bind()` method.
//!
//! See next example that contains complete code that creates simple memory
//! copy function (in DWord entities).
//!
//! ~~~
//! // Example: Usage of Label (32-bit code).
//! //
//! // Create simple DWord memory copy function:
//! // ASMJIT_STDCALL void copy32(uint32_t* dst, const uint32_t* src, size_t count);
//! using namespace asmjit;
//!
//! // Assembler instance.
//! JitRuntime runtime;
//! Assembler a(&runtime);
//!
//! // Constants.
//! const int arg_offset = 8; // Arguments offset (STDCALL EBP).
//! const int arg_size = 12;  // Arguments size.
//!
//! // Labels.
//! Label L_Loop(a);
//!
//! // Prolog.
//! a->push(ebp);
//! a->mov(ebp, esp);
//! a->push(esi);
//! a->push(edi);
//!
//! // Fetch arguments
//! a->mov(esi, dword_ptr(ebp, arg_offset + 0)); // Get dst.
//! a->mov(edi, dword_ptr(ebp, arg_offset + 4)); // Get src.
//! a->mov(ecx, dword_ptr(ebp, arg_offset + 8)); // Get count.
//!
//! // Bind L_Loop label to here.
//! a->bind(L_Loop);
//!
//! Copy 4 bytes.
//! a->mov(eax, dword_ptr(esi));
//! a->mov(dword_ptr(edi), eax);
//!
//! // Increment pointers.
//! a->add(esi, 4);
//! a->add(edi, 4);
//!
//! // Repeat loop until (--ecx != 0).
//! a->dec(ecx);
//! a->jz(L_Loop);
//!
//! // Epilog.
//! a->pop(edi);
//! a->pop(esi);
//! a->mov(esp, ebp);
//! a->pop(ebp);
//!
//! // Return: STDCALL convention is to pop stack in called function.
//! a->ret(arg_size);
//! ~~~
//!
//! If you need more abstraction for generating assembler code and you want
//! to hide calling conventions between 32-bit and 64-bit operating systems,
//! look at @c Compiler class that is designed for higher level code
//! generation.
//!
//! Advanced Code Generation
//! ------------------------
//!
//! This section describes some advanced generation features of @c Assembler
//! class which can be simply overlooked. The first thing that is very likely
//! needed is generic register support. In previous example the named registers
//! were used. AsmJit contains functions which can convert register index into
//! operand and back.
//!
//! Let's define function which can be used to generate some abstract code:
//!
//! ~~~
//! // Simple function that generates dword copy.
//! void genCopyDWord(BaseAssembler& a, const GpReg& dst, const GpReg& src, const GpReg& tmp) {
//!   a->mov(tmp, dword_ptr(src));
//!   a->mov(dword_ptr(dst), tmp);
//! }
//! ~~~
//!
//! This function can be called like <code>genCopyDWord(a, edi, esi, ebx)</code>
//! or by using existing `GpReg` instances. This abstraction allows to join
//! more code sections together without rewriting each to use specific registers.
//! You need to take care only about implicit registers which may be used by
//! several instructions (like mul, imul, div, idiv, shifting, etc...).
//!
//! Next, more advanced, but often needed technique is that you can build your
//! own registers allocator. X86 architecture contains 8 general purpose registers,
//! 8 Mm registers and 8 Xmm registers. The X64 (AMD64) architecture extends count
//! of Gp registers and Xmm registers to 16. Use the \ref kRegCountBase constant to
//! get count of Gp or Xmm registers or \ref kRegCountGp, \ref kRegCountMm and \ref
//! kRegCountXmm constants individually.
//!
//! To build register from index (value from 0 inclusive to `kRegNum...` exclusive)
//! use `gpd()`, `gpq()` or `gpz()` functions. To create a 8 or 16-bit register use
//! `gpw()`, `gpb_lo()` or `gpb_hi()`. To create other registers there are similar
//! methods like `mm()`, `xmm()`, `ymm()` and `fp()`.
//!
//! So our function call to `genCopyDWord` can be also used like this:
//!
//! ~~~
//! genCopyDWord(a, gpd(kRegIndexDi), gpd(kRegIndexSi), gpd(kRegIndexBx));
//! ~~~
//!
//! `kRegIndex...` are constants defined by `kRegIndex` enum. You can use your
//! own register allocator (or register slot manager) to alloc / free registers
//! so `kRegIndex...` values can be replaced by your variables (`0` to
//! `kRegNum...-1`).
//!
//! @sa `X86X64Compiler`.
struct X86X64Assembler : public BaseAssembler {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API X86X64Assembler(Runtime* runtime);
  ASMJIT_API virtual ~X86X64Assembler();

  // --------------------------------------------------------------------------
  // [Label]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void _bind(const Label& label);

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! Add 8-bit integer data to the instuction stream.
  ASMJIT_INLINE void db(uint8_t x) { embed(&x, 1); }
  //! Add 16-bit integer data to the instuction stream.
  ASMJIT_INLINE void dw(uint16_t x) { embed(&x, 2); }
  //! Add 32-bit integer data to the instuction stream.
  ASMJIT_INLINE void dd(uint32_t x) { embed(&x, 4); }
  //! Add 64-bit integer data to the instuction stream.
  ASMJIT_INLINE void dq(uint64_t x) { embed(&x, 8); }

  //! Add 8-bit integer data to the instuction stream.
  ASMJIT_INLINE void dint8(int8_t x) { embed(&x, sizeof(int8_t)); }
  //! Add 8-bit integer data to the instuction stream.
  ASMJIT_INLINE void duint8(uint8_t x) { embed(&x, sizeof(uint8_t)); }

  //! Add 16-bit integer data to the instuction stream.
  ASMJIT_INLINE void dint16(int16_t x) { embed(&x, sizeof(int16_t)); }
  //! Add 16-bit integer data to the instuction stream.
  ASMJIT_INLINE void duint16(uint16_t x) { embed(&x, sizeof(uint16_t)); }

  //! Add 32-bit integer data to the instuction stream.
  ASMJIT_INLINE void dint32(int32_t x) { embed(&x, sizeof(int32_t)); }
  //! Add 32-bit integer data to the instuction stream.
  ASMJIT_INLINE void duint32(uint32_t x) { embed(&x, sizeof(uint32_t)); }

  //! Add 64-bit integer data to the instuction stream.
  ASMJIT_INLINE void dint64(int64_t x) { embed(&x, sizeof(int64_t)); }
  //! Add 64-bit integer data to the instuction stream.
  ASMJIT_INLINE void duint64(uint64_t x) { embed(&x, sizeof(uint64_t)); }

  //! Add float data to the instuction stream.
  ASMJIT_INLINE void dfloat(float x) { embed(&x, sizeof(float)); }
  //! Add double data to the instuction stream.
  ASMJIT_INLINE void ddouble(double x) { embed(&x, sizeof(double)); }

  //! Add pointer data to the instuction stream.
  ASMJIT_INLINE void dptr(void* x) { embed(&x, sizeof(void*)); }

  //! Add Mm data to the instuction stream.
  ASMJIT_INLINE void dmm(const MmData& x) { embed(&x, sizeof(MmData)); }
  //! Add Xmm data to the instuction stream.
  ASMJIT_INLINE void dxmm(const XmmData& x) { embed(&x, sizeof(XmmData)); }
  //! Add Ymm data to the instuction stream.
  ASMJIT_INLINE void dymm(const YmmData& x) { embed(&x, sizeof(YmmData)); }

  //! Add data in a given structure instance to the instuction stream.
  template<typename T>
  ASMJIT_INLINE void dstruct(const T& x) { embed(&x, static_cast<uint32_t>(sizeof(T))); }

  //! Embed absolute label pointer (4 or 8 bytes).
  ASMJIT_API Error embedLabel(const Label& op);

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _align(uint32_t mode, uint32_t offset);

  // -------------------------------------------------------------------------
  // [Options]
  // -------------------------------------------------------------------------

  ASMJIT_X86X64_EMIT_OPTIONS(X86X64Assembler)

  // --------------------------------------------------------------------------
  // [Base Instructions]
  // --------------------------------------------------------------------------

  //! Add with Carry.
  INST_2x(adc, kInstAdc, GpReg, GpReg)
  //! \overload
  INST_2x(adc, kInstAdc, GpReg, Mem)
  //! \overload
  INST_2i(adc, kInstAdc, GpReg, Imm)
  //! \overload
  INST_2x(adc, kInstAdc, Mem, GpReg)
  //! \overload
  INST_2i(adc, kInstAdc, Mem, Imm)

  //! Add.
  INST_2x(add, kInstAdd, GpReg, GpReg)
  //! \overload
  INST_2x(add, kInstAdd, GpReg, Mem)
  //! \overload
  INST_2i(add, kInstAdd, GpReg, Imm)
  //! \overload
  INST_2x(add, kInstAdd, Mem, GpReg)
  //! \overload
  INST_2i(add, kInstAdd, Mem, Imm)

  //! And.
  INST_2x(and_, kInstAnd, GpReg, GpReg)
  //! \overload
  INST_2x(and_, kInstAnd, GpReg, Mem)
  //! \overload
  INST_2i(and_, kInstAnd, GpReg, Imm)
  //! \overload
  INST_2x(and_, kInstAnd, Mem, GpReg)
  //! \overload
  INST_2i(and_, kInstAnd, Mem, Imm)

  //! Bit scan forward.
  INST_2x_(bsf, kInstBsf, GpReg, GpReg, !o0.isGpb())
  //! \overload
  INST_2x_(bsf, kInstBsf, GpReg, Mem, !o0.isGpb())

  //! Bit scan reverse.
  INST_2x_(bsr, kInstBsr, GpReg, GpReg, !o0.isGpb())
  //! \overload
  INST_2x_(bsr, kInstBsr, GpReg, Mem, !o0.isGpb())

  //! Byte swap (32-bit or 64-bit registers only) (i486).
  INST_1x_(bswap, kInstBswap, GpReg, o0.getSize() >= 4)

  //! Bit test.
  INST_2x(bt, kInstBt, GpReg, GpReg)
  //! \overload
  INST_2i(bt, kInstBt, GpReg, Imm)
  //! \overload
  INST_2x(bt, kInstBt, Mem, GpReg)
  //! \overload
  INST_2i(bt, kInstBt, Mem, Imm)

  //! Bit test and complement.
  INST_2x(btc, kInstBtc, GpReg, GpReg)
  //! \overload
  INST_2i(btc, kInstBtc, GpReg, Imm)
  //! \overload
  INST_2x(btc, kInstBtc, Mem, GpReg)
  //! \overload
  INST_2i(btc, kInstBtc, Mem, Imm)

  //! Bit test and reset.
  INST_2x(btr, kInstBtr, GpReg, GpReg)
  //! \overload
  INST_2i(btr, kInstBtr, GpReg, Imm)
  //! \overload
  INST_2x(btr, kInstBtr, Mem, GpReg)
  //! \overload
  INST_2i(btr, kInstBtr, Mem, Imm)

  //! Bit test and set.
  INST_2x(bts, kInstBts, GpReg, GpReg)
  //! \overload
  INST_2i(bts, kInstBts, GpReg, Imm)
  //! \overload
  INST_2x(bts, kInstBts, Mem, GpReg)
  //! \overload
  INST_2i(bts, kInstBts, Mem, Imm)

  //! Call.
  INST_1x(call, kInstCall, GpReg)
  //! \overload
  INST_1x(call, kInstCall, Mem)
  //! \overload
  INST_1x(call, kInstCall, Label)
  //! \overload
  INST_1x(call, kInstCall, Imm)
  //! \overload
  ASMJIT_INLINE Error call(void* dst) { return call(Imm((intptr_t)dst)); }

  //! Clear carry flag.
  INST_0x(clc, kInstClc)
  //! Clear direction flag.
  INST_0x(cld, kInstCld)
  //! Complement carry flag.
  INST_0x(cmc, kInstCmc)

  //! Convert BYTE to WORD (AX <- Sign Extend AL).
  INST_0x(cbw, kInstCbw)
  //! Convert WORD to DWORD (DX:AX <- Sign Extend AX).
  INST_0x(cwd, kInstCwd)
  //! Convert WORD to DWORD (EAX <- Sign Extend AX).
  INST_0x(cwde, kInstCwde)
  //! Convert DWORD to QWORD (EDX:EAX <- Sign Extend EAX).
  INST_0x(cdq, kInstCdq)

  //! Conditional move.
  INST_2cc(cmov, kInstCmov, X86Util::condToCmovcc, GpReg, GpReg)
  //! Conditional move.
  INST_2cc(cmov, kInstCmov, X86Util::condToCmovcc, GpReg, Mem)

  //! Compare two operands.
  INST_2x(cmp, kInstCmp, GpReg, GpReg)
  //! \overload
  INST_2x(cmp, kInstCmp, GpReg, Mem)
  //! \overload
  INST_2i(cmp, kInstCmp, GpReg, Imm)
  //! \overload
  INST_2x(cmp, kInstCmp, Mem, GpReg)
  //! \overload
  INST_2i(cmp, kInstCmp, Mem, Imm)

  //! Compare and exchange (i486).
  INST_2x(cmpxchg, kInstCmpxchg, GpReg, GpReg)
  //! \overload
  INST_2x(cmpxchg, kInstCmpxchg, Mem, GpReg)

  //! Compares the 64-bit value in EDX:EAX with the memory operand (Pentium).
  INST_1x(cmpxchg8b, kInstCmpxchg8b, Mem)

  //! CPU identification (i486).
  INST_0x(cpuid, kInstCpuid)

  //! Accumulate crc32 value (polynomial 0x11EDC6F41) (SSE4.2).
  INST_2x_(crc32, kInstCrc32, GpReg, GpReg, o0.isRegType(kRegTypeGpd) || o0.isRegType(kRegTypeGpq))
  //! \overload
  INST_2x_(crc32, kInstCrc32, GpReg, Mem, o0.isRegType(kRegTypeGpd) || o0.isRegType(kRegTypeGpq))

  //! Decrement by 1.
  INST_1x(dec, kInstDec, GpReg)
  //! \overload
  INST_1x(dec, kInstDec, Mem)

  //! Unsigned divide (xDX:xAX <- xDX:xAX / o0).
  INST_1x(div, kInstDiv, GpReg)
  //! \overload
  INST_1x(div, kInstDiv, Mem)

  //! Make stack frame for procedure parameters.
  INST_2x(enter, kInstEnter, Imm, Imm)

  //! Signed divide (xDX:xAX <- xDX:xAX / op).
  INST_1x(idiv, kInstIdiv, GpReg)
  //! \overload
  INST_1x(idiv, kInstIdiv, Mem)

  //! Signed multiply (xDX:xAX <- xAX * o0).
  INST_1x(imul, kInstImul, GpReg)
  //! \overload
  INST_1x(imul, kInstImul, Mem)

  //! Signed multiply.
  INST_2x(imul, kInstImul, GpReg, GpReg)
  //! \overload
  INST_2x(imul, kInstImul, GpReg, Mem)
  //! \overload
  INST_2i(imul, kInstImul, GpReg, Imm)

  //! Signed multiply.
  INST_3i(imul, kInstImul, GpReg, GpReg, Imm)
  //! \overload
  INST_3i(imul, kInstImul, GpReg, Mem, Imm)

  //! Increment by 1.
  INST_1x(inc, kInstInc, GpReg)
  //! \overload
  INST_1x(inc, kInstInc, Mem)

  //! Interrupt.
  INST_1i(int_, kInstInt, Imm)
  //! Interrupt 3 - trap to debugger.
  ASMJIT_INLINE Error int3() { return int_(3); }

  //! Jump to `label` if condition `cc` is met.
  INST_1cc(j, kInstJ, X86Util::condToJcc, Label)

  //! Short jump if CX/ECX/RCX is zero.
  INST_2x_(jecxz, kInstJecxz, GpReg, Label, o0.getRegIndex() == kRegIndexCx)

  //! Jump.
  INST_1x(jmp, kInstJmp, GpReg)
  //! \overload
  INST_1x(jmp, kInstJmp, Mem)
  //! \overload
  INST_1x(jmp, kInstJmp, Label)
  //! \overload
  INST_1x(jmp, kInstJmp, Imm)
  //! \overload
  ASMJIT_INLINE Error jmp(void* dst) { return jmp(Imm((intptr_t)dst)); }

  //! Load AH from flags.
  INST_0x(lahf, kInstLahf)

  //! Load effective address
  INST_2x(lea, kInstLea, GpReg, Mem)

  //! High level procedure exit.
  INST_0x(leave, kInstLeave)

  //! Move.
  INST_2x(mov, kInstMov, GpReg, GpReg)
  //! \overload
  INST_2x(mov, kInstMov, GpReg, Mem)
  //! \overload
  INST_2i(mov, kInstMov, GpReg, Imm)
  //! \overload
  INST_2x(mov, kInstMov, Mem, GpReg)
  //! \overload
  INST_2i(mov, kInstMov, Mem, Imm)

  //! Move from segment register.
  INST_2x(mov, kInstMov, GpReg, SegReg)
  //! \overload
  INST_2x(mov, kInstMov, Mem, SegReg)
  //! Move to segment register.
  INST_2x(mov, kInstMov, SegReg, GpReg)
  //! \overload
  INST_2x(mov, kInstMov, SegReg, Mem)

  //! Move (AL|AX|EAX|RAX <- absolute address in immediate).
  ASMJIT_INLINE Error mov_ptr(const GpReg& dst, void* src) {
    ASMJIT_ASSERT(dst.getRegIndex() == 0);

    Imm imm(static_cast<int64_t>((intptr_t)src));
    return emit(kInstMovPtr, dst, imm);
  }

  //! Move (absolute address in immediate <- AL|AX|EAX|RAX).
  ASMJIT_INLINE Error mov_ptr(void* dst, const GpReg& src) {
    ASMJIT_ASSERT(src.getRegIndex() == 0);

    Imm imm(static_cast<int64_t>((intptr_t)dst));
    return emit(kInstMovPtr, imm, src);
  }

  //! Move data after dwapping bytes (SSE3 - Atom).
  INST_2x_(movbe, kInstMovbe, GpReg, Mem, !o0.isGpb());
  //! \overload
  INST_2x_(movbe, kInstMovbe, Mem, GpReg, !o1.isGpb());

  //! Move with sign-extension.
  INST_2x(movsx, kInstMovsx, GpReg, GpReg)
  //! \overload
  INST_2x(movsx, kInstMovsx, GpReg, Mem)

  //! Move with zero-extension.
  INST_2x(movzx, kInstMovzx, GpReg, GpReg)
  //! \overload
  INST_2x(movzx, kInstMovzx, GpReg, Mem)

  //! Unsigned multiply (xDX:xAX <- xAX * o0).
  INST_1x(mul, kInstMul, GpReg)
  //! \overload
  INST_1x(mul, kInstMul, Mem)

  //! Two's complement negation.
  INST_1x(neg, kInstNeg, GpReg)
  //! \overload
  INST_1x(neg, kInstNeg, Mem)

  //! No operation.
  INST_0x(nop, kInstNop)

  //! One's complement negation.
  INST_1x(not_, kInstNot, GpReg)
  //! \overload
  INST_1x(not_, kInstNot, Mem)

  //! Or.
  INST_2x(or_, kInstOr, GpReg, GpReg)
  //! \overload
  INST_2x(or_, kInstOr, GpReg, Mem)
  //! \overload
  INST_2i(or_, kInstOr, GpReg, Imm)
  //! \overload
  INST_2x(or_, kInstOr, Mem, GpReg)
  //! \overload
  INST_2i(or_, kInstOr, Mem, Imm)

  //! Pop a value from the stack.
  INST_1x_(pop, kInstPop, GpReg, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! \overload
  INST_1x_(pop, kInstPop, Mem, o0.getSize() == 2 || o0.getSize() == _regSize)

  //! Pop a segment register from the stack.
  //!
  //! \note There is no instruction to pop a cs segment register.
  INST_1x_(pop, kInstPop, SegReg, o0.getRegIndex() != kSegCs);

  //! Pop stack into EFLAGS register (32-bit or 64-bit).
  INST_0x(popf, kInstPopf)

  //! Return the count of number of bits set to 1 (SSE4.2).
  INST_2x_(popcnt, kInstPopcnt, GpReg, GpReg, !o0.isGpb() && o0.getRegType() == o1.getRegType())
  //! \overload
  INST_2x_(popcnt, kInstPopcnt, GpReg, Mem, !o0.isGpb())

  //! Push WORD or DWORD/QWORD on the stack.
  INST_1x_(push, kInstPush, GpReg, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! Push WORD or DWORD/QWORD on the stack.
  INST_1x_(push, kInstPush, Mem, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! Push segment register on the stack.
  INST_1x(push, kInstPush, SegReg)
  //! Push WORD or DWORD/QWORD on the stack.
  INST_1i(push, kInstPush, Imm)

  //! Push EFLAGS register (32-bit or 64-bit) on the stack.
  INST_0x(pushf, kInstPushf)

  //! Rotate bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(rcl, kInstRcl, GpReg, GpReg)
  //! \overload
  INST_2x(rcl, kInstRcl, Mem, GpReg)
  //! Rotate bits left.
  INST_2i(rcl, kInstRcl, GpReg, Imm)
  //! \overload
  INST_2i(rcl, kInstRcl, Mem, Imm)

  //! Rotate bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(rcr, kInstRcr, GpReg, GpReg)
  //! \overload
  INST_2x(rcr, kInstRcr, Mem, GpReg)
  //! Rotate bits right.
  INST_2i(rcr, kInstRcr, GpReg, Imm)
  //! \overload
  INST_2i(rcr, kInstRcr, Mem, Imm)

  //! Read time-stamp counter (Pentium).
  INST_0x(rdtsc, kInstRdtsc)
  //! Read time-stamp counter and processor id (Pentium).
  INST_0x(rdtscp, kInstRdtscp)

  //! Load ECX/RCX bytes from DS:[ESI/RSI] to AL.
  INST_0x(rep_lodsb, kInstRepLodsb)
  //! Load ECX/RCX dwords from DS:[ESI/RSI] to EAX.
  INST_0x(rep_lodsd, kInstRepLodsd)
  //! Load ECX/RCX Words from DS:[ESI/RSI] to AX.
  INST_0x(rep_lodsw, kInstRepLodsw)

  //! Move ECX/RCX bytes from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(rep_movsb, kInstRepMovsb)
  //! Move ECX/RCX dwords from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(rep_movsd, kInstRepMovsd)
  //! Move ECX/RCX words from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(rep_movsw, kInstRepMovsw)

  //! Fill ECX/RCX bytes at ES:[EDI/RDI] with AL.
  INST_0x(rep_stosb, kInstRepStosb)
  //! Fill ECX/RCX dwords at ES:[EDI/RDI] with EAX.
  INST_0x(rep_stosd, kInstRepStosd)
  //! Fill ECX/RCX words at ES:[EDI/RDI] with AX.
  INST_0x(rep_stosw, kInstRepStosw)

  //! Repeated find nonmatching bytes in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repe_cmpsb, kInstRepeCmpsb)
  //! Repeated find nonmatching dwords in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repe_cmpsd, kInstRepeCmpsd)
  //! Repeated find nonmatching words in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repe_cmpsw, kInstRepeCmpsw)

  //! Find non-AL byte starting at ES:[EDI/RDI].
  INST_0x(repe_scasb, kInstRepeScasb)
  //! Find non-EAX dword starting at ES:[EDI/RDI].
  INST_0x(repe_scasd, kInstRepeScasd)
  //! Find non-AX word starting at ES:[EDI/RDI].
  INST_0x(repe_scasw, kInstRepeScasw)

  //! Repeated find nonmatching bytes in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repne_cmpsb, kInstRepneCmpsb)
  //! Repeated find nonmatching dwords in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repne_cmpsd, kInstRepneCmpsd)
  //! Repeated find nonmatching words in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repne_cmpsw, kInstRepneCmpsw)

  //! Find AL, starting at ES:[EDI/RDI].
  INST_0x(repne_scasb, kInstRepneScasb)
  //! Find EAX, starting at ES:[EDI/RDI].
  INST_0x(repne_scasd, kInstRepneScasd)
  //! Find AX, starting at ES:[EDI/RDI].
  INST_0x(repne_scasw, kInstRepneScasw)

  //! Return.
  INST_0x(ret, kInstRet)
  //! \overload
  INST_1i(ret, kInstRet, Imm)

  //! Rotate bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(rol, kInstRol, GpReg, GpReg)
  //! \overload
  INST_2x(rol, kInstRol, Mem, GpReg)
  //! Rotate bits left.
  INST_2i(rol, kInstRol, GpReg, Imm)
  //! \overload
  INST_2i(rol, kInstRol, Mem, Imm)

  //! Rotate bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(ror, kInstRor, GpReg, GpReg)
  //! \overload
  INST_2x(ror, kInstRor, Mem, GpReg)
  //! Rotate bits right.
  INST_2i(ror, kInstRor, GpReg, Imm)
  //! \overload
  INST_2i(ror, kInstRor, Mem, Imm)

  //! Store AH into flags.
  INST_0x(sahf, kInstSahf)

  //! Integer subtraction with borrow.
  INST_2x(sbb, kInstSbb, GpReg, GpReg)
  //! \overload
  INST_2x(sbb, kInstSbb, GpReg, Mem)
  //! \overload
  INST_2i(sbb, kInstSbb, GpReg, Imm)
  //! \overload
  INST_2x(sbb, kInstSbb, Mem, GpReg)
  //! \overload
  INST_2i(sbb, kInstSbb, Mem, Imm)

  //! Shift bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(sal, kInstSal, GpReg, GpReg)
  //! \overload
  INST_2x(sal, kInstSal, Mem, GpReg)
  //! Shift bits left.
  INST_2i(sal, kInstSal, GpReg, Imm)
  //! \overload
  INST_2i(sal, kInstSal, Mem, Imm)

  //! Shift bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(sar, kInstSar, GpReg, GpReg)
  //! \overload
  INST_2x(sar, kInstSar, Mem, GpReg)
  //! Shift bits right.
  INST_2i(sar, kInstSar, GpReg, Imm)
  //! \overload
  INST_2i(sar, kInstSar, Mem, Imm)

  //! Set byte on condition.
  INST_1cc(set, kInstSet, X86Util::condToSetcc, GpReg)
  //! Set byte on condition.
  INST_1cc(set, kInstSet, X86Util::condToSetcc, Mem)

  //! Shift bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(shl, kInstShl, GpReg, GpReg)
  //! \overload
  INST_2x(shl, kInstShl, Mem, GpReg)
  //! Shift bits left.
  INST_2i(shl, kInstShl, GpReg, Imm)
  //! \overload
  INST_2i(shl, kInstShl, Mem, Imm)

  //! Shift bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(shr, kInstShr, GpReg, GpReg)
  //! \overload
  INST_2x(shr, kInstShr, Mem, GpReg)
  //! Shift bits right.
  INST_2i(shr, kInstShr, GpReg, Imm)
  //! \overload
  INST_2i(shr, kInstShr, Mem, Imm)

  //! Double precision shift left.
  //!
  //! \note `o2` register can be only `cl` register.
  INST_3x(shld, kInstShld, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(shld, kInstShld, Mem, GpReg, GpReg)
  //! Double precision shift left.
  INST_3i(shld, kInstShld, GpReg, GpReg, Imm)
  //! \overload
  INST_3i(shld, kInstShld, Mem, GpReg, Imm)

  //! Double precision shift right.
  //!
  //! \note `o2` register can be only `cl` register.
  INST_3x(shrd, kInstShrd, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(shrd, kInstShrd, Mem, GpReg, GpReg)
  //! Double precision shift right.
  INST_3i(shrd, kInstShrd, GpReg, GpReg, Imm)
  //! \overload
  INST_3i(shrd, kInstShrd, Mem, GpReg, Imm)

  //! Set carry flag to 1.
  INST_0x(stc, kInstStc)
  //! Set direction flag to 1.
  INST_0x(std, kInstStd)

  //! Subtract.
  INST_2x(sub, kInstSub, GpReg, GpReg)
  //! \overload
  INST_2x(sub, kInstSub, GpReg, Mem)
  //! \overload
  INST_2i(sub, kInstSub, GpReg, Imm)
  //! \overload
  INST_2x(sub, kInstSub, Mem, GpReg)
  //! \overload
  INST_2i(sub, kInstSub, Mem, Imm)

  //! Logical compare.
  INST_2x(test, kInstTest, GpReg, GpReg)
  //! \overload
  INST_2i(test, kInstTest, GpReg, Imm)
  //! \overload
  INST_2x(test, kInstTest, Mem, GpReg)
  //! \overload
  INST_2i(test, kInstTest, Mem, Imm)

  //! Undefined instruction - Raise #UD exception.
  INST_0x(ud2, kInstUd2)

  //! Exchange and Add.
  INST_2x(xadd, kInstXadd, GpReg, GpReg)
  //! \overload
  INST_2x(xadd, kInstXadd, Mem, GpReg)

  //! Exchange register/memory with register.
  INST_2x(xchg, kInstXchg, GpReg, GpReg)
  //! \overload
  INST_2x(xchg, kInstXchg, Mem, GpReg)
  //! \overload
  INST_2x(xchg, kInstXchg, GpReg, Mem)

  //! Xor.
  INST_2x(xor_, kInstXor, GpReg, GpReg)
  //! \overload
  INST_2x(xor_, kInstXor, GpReg, Mem)
  //! \overload
  INST_2i(xor_, kInstXor, GpReg, Imm)
  //! \overload
  INST_2x(xor_, kInstXor, Mem, GpReg)
  //! \overload
  INST_2i(xor_, kInstXor, Mem, Imm)

  // --------------------------------------------------------------------------
  // [Fpu]
  // --------------------------------------------------------------------------

  //! Compute 2^x - 1 (FPU).
  INST_0x(f2xm1, kInstF2xm1)

  //! Absolute value of fp0 (FPU).
  INST_0x(fabs, kInstFabs)

  //! Add `o1` to `o0` and store result in `o0` (FPU).
  //!
  //! \note One of dst or src must be fp0.
  INST_2x_(fadd, kInstFadd, FpReg, FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Add 4-byte or 8-byte FP `o0` to fp0 and store result in fp0 (FPU).
  INST_1x(fadd, kInstFadd, Mem)
  //! Add fp0 to `o0` and POP register stack (FPU).
  INST_1x(faddp, kInstFaddp, FpReg)
  //! \overload
  INST_0x(faddp, kInstFaddp)

  //! Load binary coded decimal (FPU).
  INST_1x(fbld, kInstFbld, Mem)
  //! Store BCD integer and Pop (FPU).
  INST_1x(fbstp, kInstFbstp, Mem)
  //! Change fp0 sign (FPU).
  INST_0x(fchs, kInstFchs)

  //! Clear exceptions (FPU).
  INST_0x(fclex, kInstFclex)

  //! FP Conditional move (FPU).
  INST_1x(fcmovb, kInstFcmovb, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmovbe, kInstFcmovbe, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmove, kInstFcmove, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmovnb, kInstFcmovnb, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmovnbe, kInstFcmovnbe, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmovne, kInstFcmovne, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmovnu, kInstFcmovnu, FpReg)
  //! FP Conditional move (FPU).
  INST_1x(fcmovu, kInstFcmovu, FpReg)

  //! Compare fp0 with `o0` (FPU).
  INST_1x(fcom, kInstFcom, FpReg)
  //! Compare fp0 with fp1 (FPU).
  INST_0x(fcom, kInstFcom)
  //! Compare fp0 with 4-byte or 8-byte FP at `src` (FPU).
  INST_1x(fcom, kInstFcom, Mem)
  //! Compare fp0 with `o0` and pop the stack (FPU).
  INST_1x(fcomp, kInstFcomp, FpReg)
  //! Compare fp0 with fp1 and pop the stack (FPU).
  INST_0x(fcomp, kInstFcomp)
  //! Compare fp0 with 4-byte or 8-byte FP at `adr` and pop the stack (FPU).
  INST_1x(fcomp, kInstFcomp, Mem)
    //! Compare fp0 with fp1 and pop register stack twice (FPU).
  INST_0x(fcompp, kInstFcompp)
  //! Compare fp0 and `o0` and Set EFLAGS (FPU).
  INST_1x(fcomi, kInstFcomi, FpReg)
  //! Compare fp0 and `o0` and Set EFLAGS and pop the stack (FPU).
  INST_1x(fcomip, kInstFcomip, FpReg)

  //! Calculate cosine of fp0 and store result in fp0 (FPU).
  INST_0x(fcos, kInstFcos)

  //! Decrement stack-top pointer (FPU).
  INST_0x(fdecstp, kInstFdecstp)

  //! Divide `o0` by `o1` (FPU).
  //!
  //! \note One of `o0` or `o1` register must be fp0.
  INST_2x_(fdiv, kInstFdiv, FpReg, FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Divide fp0 by 32-bit or 64-bit FP value (FPU).
  INST_1x(fdiv, kInstFdiv, Mem)
  //! Divide `o0` by fp0 (FPU).
  INST_1x(fdivp, kInstFdivp, FpReg)
  //! \overload
  INST_0x(fdivp, kInstFdivp)

  //! Reverse divide `o0` by `o1` (FPU).
  //!
  //! \note One of `o0` or `src` register must be fp0.
  INST_2x_(fdivr, kInstFdivr, FpReg, FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Reverse divide fp0 by 32-bit or 64-bit FP value (FPU).
  INST_1x(fdivr, kInstFdivr, Mem)
  //! Reverse divide `o0` by fp0 (FPU).
  INST_1x(fdivrp, kInstFdivrp, FpReg)
  //! \overload
  INST_0x(fdivrp, kInstFdivrp)

  //! Free FP register (FPU).
  //!
  //! Sets the tag in the FPU tag register associated with register `o0`
  //! to empty (11B). The contents of `o0` and the FPU stack-top pointer
  //! (TOP) are not affected.
  INST_1x(ffree, kInstFfree, FpReg)

  //! Add 16-bit or 32-bit integer to fp0 (FPU).
  INST_1x_(fiadd, kInstFiadd, Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Compare fp0 with 16-bit or 32-bit Integer (FPU).
  INST_1x_(ficom, kInstFicom, Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Compare fp0 with 16-bit or 32-bit Integer and pop the stack (FPU).
  INST_1x_(ficomp, kInstFicomp, Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Divide fp0 by 32-bit or 16-bit integer (`src`) (FPU).
  INST_1x_(fidiv, kInstFidiv, Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Reverse divide fp0 by 32-bit or 16-bit integer (`src`) (FPU).
  INST_1x_(fidivr, kInstFidivr, Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Load 16-bit, 32-bit or 64-bit Integer and push it to the stack (FPU).
  INST_1x_(fild, kInstFild, Mem, o0.getSize() == 2 || o0.getSize() == 4 || o0.getSize() == 8)

  //! Multiply fp0 by 16-bit or 32-bit integer and store it to fp0 (FPU).
  INST_1x_(fimul, kInstFimul, Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Increment stack-top pointer (FPU).
  INST_0x(fincstp, kInstFincstp)

  //! Initialize FPU (FPU).
  INST_0x(finit, kInstFinit)

  //! Subtract 16-bit or 32-bit integer from fp0 and store result to fp0 (FPU).
  INST_1x_(fisub, kInstFisub, Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Reverse subtract 16-bit or 32-bit integer from fp0 and store result to fp0 (FPU).
  INST_1x_(fisubr, kInstFisubr, Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Initialize FPU without checking for pending unmasked exceptions (FPU).
  INST_0x(fninit, kInstFninit)

  //! Store fp0 as 16-bit or 32-bit Integer to `o0` (FPU).
  INST_1x_(fist, kInstFist, Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Store fp0 as 16-bit, 32-bit or 64-bit Integer to `o0` and pop stack (FPU).
  INST_1x_(fistp, kInstFistp, Mem, o0.getSize() == 2 || o0.getSize() == 4 || o0.getSize() == 8)

  //! Push 32-bit, 64-bit or 80-bit floating point value on the FPU register stack (FPU).
  INST_1x_(fld, kInstFld, Mem, o0.getSize() == 4 || o0.getSize() == 8 || o0.getSize() == 10)
  //! Push `o0` on the FPU register stack (FPU).
  INST_1x(fld, kInstFld, FpReg)

  //! Push +1.0 on the FPU register stack (FPU).
  INST_0x(fld1, kInstFld1)
  //! Push log2(10) on the FPU register stack (FPU).
  INST_0x(fldl2t, kInstFldl2t)
  //! Push log2(e) on the FPU register stack (FPU).
  INST_0x(fldl2e, kInstFldl2e)
  //! Push pi on the FPU register stack (FPU).
  INST_0x(fldpi, kInstFldpi)
  //! Push log10(2) on the FPU register stack (FPU).
  INST_0x(fldlg2, kInstFldlg2)
  //! Push ln(2) on the FPU register stack (FPU).
  INST_0x(fldln2, kInstFldln2)
  //! Push +0.0 on the FPU register stack (FPU).
  INST_0x(fldz, kInstFldz)

  //! Load x87 FPU control word (2 bytes) (FPU).
  INST_1x(fldcw, kInstFldcw, Mem)
  //! Load x87 FPU environment (14 or 28 bytes) (FPU).
  INST_1x(fldenv, kInstFldenv, Mem)

  //! Multiply `o0` by `o1` and store result in `o0` (FPU).
  //!
  //! \note One of dst or src must be fp0.
  INST_2x_(fmul, kInstFmul, FpReg, FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Multiply fp0 by 32-bit or 64-bit `o0` and store result in fp0 (FPU).
  INST_1x(fmul, kInstFmul, Mem)

  //! Multiply fp0 by `o0` and POP register stack (FPU).
  INST_1x(fmulp, kInstFmulp, FpReg)
  //! \overload
  INST_0x(fmulp, kInstFmulp)

  //! Clear exceptions (FPU).
  INST_0x(fnclex, kInstFnclex)

  //! No operation (FPU).
  INST_0x(fnop, kInstFnop)

  //! Save FPU state (FPU).
  INST_1x(fnsave, kInstFnsave, Mem)

  //! Store x87 FPU environment (FPU).
  INST_1x(fnstenv, kInstFnstenv, Mem)

  //! Store x87 FPU control word (FPU).
  INST_1x(fnstcw, kInstFnstcw, Mem)

  //! Store x87 FPU status word to `o0` (AX) (FPU).
  INST_1x_(fnstsw, kInstFnstsw, GpReg, o0.isRegCode(kRegTypeGpw, kRegIndexAx))
  //! Store x87 FPU status word to `o0` (2 bytes) (FPU).
  INST_1x(fnstsw, kInstFnstsw, Mem)

  //! Calculate arctan(fp1 / fp0) and pop the register stack (FPU).
  INST_0x(fpatan, kInstFpatan)

  //! Calculate fprem(fp0, fp1) and pop the register stack (FPU).
  INST_0x(fprem, kInstFprem)
  //! Calculate IEEE fprem(fp0, fp1) and pop the register stack (FPU).
  INST_0x(fprem1, kInstFprem1)

  //! Calculate arctan(fp0) and pop the register stack (FPU).
  INST_0x(fptan, kInstFptan)
  //! Round fp0 to Integer (FPU).
  INST_0x(frndint, kInstFrndint)

  //! Restore FPU state from `o0` (94 or 108 bytes) (FPU).
  INST_1x(frstor, kInstFrstor, Mem)

  //! Save FPU state to `o0` (FPU).
  //!
  //! Store FPU state to 94 or 108-bytes after checking for
  //! pending unmasked FP exceptions. Then reinitialize
  //! the FPU.
  INST_1x(fsave, kInstFsave, Mem)

  //! Scale (FPU).
  //!
  //! Scale fp0 by fp1.
  INST_0x(fscale, kInstFscale)

  //! Calculate sine of fp0 and store result in fp0 (FPU).
  INST_0x(fsin, kInstFsin)

  //! Sine and cosine (FPU).
  //!
  //! Compute the sine and cosine of fp0; replace fp0 with the sine
  //! and push the cosine on the register stack.
  INST_0x(fsincos, kInstFsincos)

  //! Square root (FPU).
  //!
  //! Calculates square root of fp0 and stores the result in fp0.
  INST_0x(fsqrt, kInstFsqrt)

  //! Store floating point value (FPU).
  //!
  //! Store fp0 as 32-bit or 64-bit floating point value to `o0`.
  INST_1x_(fst, kInstFst, Mem, o0.getSize() == 4 || o0.getSize() == 8)

  //! Store floating point value to `o0` (FPU).
  INST_1x(fst, kInstFst, FpReg)

  //! Store floating point value and pop register stack (FPU).
  //!
  //! Store fp0 as 32-bit or 64-bit floating point value to `o0`
  //! and pop register stack.
  INST_1x_(fstp, kInstFstp, Mem, o0.getSize() == 4 || o0.getSize() == 8 || o0.getSize() == 10)

  //! Store floating point value and pop register stack (FPU).
  //!
  //! Store fp0 to `o0` and pop register stack.
  INST_1x(fstp, kInstFstp, FpReg)

  //! Store x87 FPU control word (FPU).
  //!
  //! Store FPU control word to `o0` (2 bytes) after checking for pending
  //! unmasked floating-point exceptions.
  INST_1x(fstcw, kInstFstcw, Mem)

  //! Store x87 FPU environment (FPU).
  //!
  //! Store FPU environment to `o0` (14 or 28 bytes) after checking for
  //! pending unmasked floating-point exceptions. Then mask all floating
  //! point exceptions.
  INST_1x(fstenv, kInstFstenv, Mem)

  //! Store x87 FPU status word (AX) (FPU).
  INST_1x_(fstsw, kInstFstsw, GpReg, o0.isRegCode(kRegTypeGpw, kRegIndexAx))
  //! Store x87 FPU status sord (2 bytes) (FPU).
  INST_1x(fstsw, kInstFstsw, Mem)

  //! Subtract `o0` from `o0` and store result in `o0` (FPU).
  //!
  //! \note One of dst or src must be fp0.
  INST_2x_(fsub, kInstFsub, FpReg, FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Subtract 32-bit or 64-bit `o0` from fp0 and store result in fp0 (FPU).
  INST_1x_(fsub, kInstFsub, Mem, o0.getSize() == 4 || o0.getSize() == 8)
  //! Subtract fp0 from `o0` and POP register stack (FPU).
  INST_1x(fsubp, kInstFsubp, FpReg)
  //! \overload
  INST_0x(fsubp, kInstFsubp)

  //! Reverse subtract `o1` from `o0` and store result in `o0` (FPU).
  //!
  //! \note One of dst or src must be fp0.
  INST_2x_(fsubr, kInstFsubr, FpReg, FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Reverse subtract 32-bit or 64-bit `o0` from fp0 and store result in fp0 (FPU).
  INST_1x_(fsubr, kInstFsubr, Mem, o0.getSize() == 4 || o0.getSize() == 8)
  //! Reverse subtract fp0 from `o0` and POP register stack (FPU).
  INST_1x(fsubrp, kInstFsubrp, FpReg)
  //! \overload
  INST_0x(fsubrp, kInstFsubrp)

  //! Floating point test - Compare fp0 with 0.0. (FPU).
  INST_0x(ftst, kInstFtst)

  //! Unordered compare fp0 with `o0` (FPU).
  INST_1x(fucom, kInstFucom, FpReg)
  //! Unordered compare fp0 with fp1 (FPU).
  INST_0x(fucom, kInstFucom)
  //! Unordered compare fp0 and `o0`, check for ordered values and Set EFLAGS (FPU).
  INST_1x(fucomi, kInstFucomi, FpReg)
  //! Unordered compare fp0 and `o0`, Check for ordered values and Set EFLAGS and pop the stack (FPU).
  INST_1x(fucomip, kInstFucomip, FpReg)
  //! Unordered compare fp0 with `o0` and pop register stack (FPU).
  INST_1x(fucomp, kInstFucomp, FpReg)
  //! Unordered compare fp0 with fp1 and pop register stack (FPU).
  INST_0x(fucomp, kInstFucomp)
  //! Unordered compare fp0 with fp1 and pop register stack twice (FPU).
  INST_0x(fucompp, kInstFucompp)

  INST_0x(fwait, kInstFwait)

  //! Examine fp0 (FPU).
  INST_0x(fxam, kInstFxam)

  //! Exchange register contents (FPU).
  //!
  //! Exchange content of fp0 with `o0`.
  INST_1x(fxch, kInstFxch, FpReg)

  //! Restore FP, MMX and streaming SIMD extension states (FPU, MMX, SSE).
  //!
  //! Load FP and MMX technology and Streaming SIMD Extension state from
  //! src (512 bytes).
  INST_1x(fxrstor, kInstFxrstor, Mem)

  //! Store FP, MMX and streaming SIMD extension states (FPU, MMX, SSE).
  //!
  //! Store FP and MMX technology state and Streaming SIMD Extension state
  //! to dst (512 bytes).
  INST_1x(fxsave, kInstFxsave, Mem)

  //! Extract exponent and significand (FPU).
  //!
  //! Separate value in fp0 into exponent and significand, store exponent
  //! in fp0 and push the significand on the register stack.
  INST_0x(fxtract, kInstFxtract)

  //! Compute y * log2(x).
  //!
  //! Replace fp1 with (fp1 * log2(fp0)) and pop the register stack.
  INST_0x(fyl2x, kInstFyl2x)
  //! Compute y * log_2(x+1).
  //!
  //! Replace fp1 with (fp1 * (log2(fp0)+1)) and pop the register stack.
  INST_0x(fyl2xp1, kInstFyl2xp1)

  // --------------------------------------------------------------------------
  // [MMX]
  // --------------------------------------------------------------------------

  //! Move DWORD (MMX).
  INST_2x(movd, kInstMovd, Mem, MmReg)
  //! \overload
  INST_2x(movd, kInstMovd, GpReg, MmReg)
  //! \overload
  INST_2x(movd, kInstMovd, MmReg, Mem)
  //! \overload
  INST_2x(movd, kInstMovd, MmReg, GpReg)

  //! Move QWORD (MMX).
  INST_2x(movq, kInstMovq, MmReg, MmReg)
  //! \overload
  INST_2x(movq, kInstMovq, Mem, MmReg)
  //! \overload
  INST_2x(movq, kInstMovq, MmReg, Mem)

  //! Pack DWORDs to WORDs with signed saturation (MMX).
  INST_2x(packssdw, kInstPackssdw, MmReg, MmReg)
  //! \overload
  INST_2x(packssdw, kInstPackssdw, MmReg, Mem)

  //! Pack WORDs to BYTEs with signed saturation (MMX).
  INST_2x(packsswb, kInstPacksswb, MmReg, MmReg)
  //! \overload
  INST_2x(packsswb, kInstPacksswb, MmReg, Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (MMX).
  INST_2x(packuswb, kInstPackuswb, MmReg, MmReg)
  //! \overload
  INST_2x(packuswb, kInstPackuswb, MmReg, Mem)

  //! Packed BYTE add (MMX).
  INST_2x(paddb, kInstPaddb, MmReg, MmReg)
  //! \overload
  INST_2x(paddb, kInstPaddb, MmReg, Mem)

  //! Packed DWORD add (MMX).
  INST_2x(paddd, kInstPaddd, MmReg, MmReg)
  //! \overload
  INST_2x(paddd, kInstPaddd, MmReg, Mem)

  //! Packed BYTE add with saturation (MMX).
  INST_2x(paddsb, kInstPaddsb, MmReg, MmReg)
  //! \overload
  INST_2x(paddsb, kInstPaddsb, MmReg, Mem)

  //! Packed WORD add with saturation (MMX).
  INST_2x(paddsw, kInstPaddsw, MmReg, MmReg)
  //! \overload
  INST_2x(paddsw, kInstPaddsw, MmReg, Mem)

  //! Packed BYTE add with unsigned saturation (MMX).
  INST_2x(paddusb, kInstPaddusb, MmReg, MmReg)
  //! \overload
  INST_2x(paddusb, kInstPaddusb, MmReg, Mem)

  //! Packed WORD add with unsigned saturation (MMX).
  INST_2x(paddusw, kInstPaddusw, MmReg, MmReg)
  //! \overload
  INST_2x(paddusw, kInstPaddusw, MmReg, Mem)

  //! Packed WORD add (MMX).
  INST_2x(paddw, kInstPaddw, MmReg, MmReg)
  //! \overload
  INST_2x(paddw, kInstPaddw, MmReg, Mem)

  //! Packed bitwise and (MMX).
  INST_2x(pand, kInstPand, MmReg, MmReg)
  //! \overload
  INST_2x(pand, kInstPand, MmReg, Mem)

  //! Packed bitwise and-not (MMX).
  INST_2x(pandn, kInstPandn, MmReg, MmReg)
  //! \overload
  INST_2x(pandn, kInstPandn, MmReg, Mem)

  //! Packed BYTEs compare for equality (MMX).
  INST_2x(pcmpeqb, kInstPcmpeqb, MmReg, MmReg)
  //! \overload
  INST_2x(pcmpeqb, kInstPcmpeqb, MmReg, Mem)

  //! Packed DWORDs compare for equality (MMX).
  INST_2x(pcmpeqd, kInstPcmpeqd, MmReg, MmReg)
  //! \overload
  INST_2x(pcmpeqd, kInstPcmpeqd, MmReg, Mem)

  //! Packed WORDs compare for equality (MMX).
  INST_2x(pcmpeqw, kInstPcmpeqw, MmReg, MmReg)
  //! \overload
  INST_2x(pcmpeqw, kInstPcmpeqw, MmReg, Mem)

  //! Packed BYTEs compare if greater than (MMX).
  INST_2x(pcmpgtb, kInstPcmpgtb, MmReg, MmReg)
  //! \overload
  INST_2x(pcmpgtb, kInstPcmpgtb, MmReg, Mem)

  //! Packed DWORDs compare if greater than (MMX).
  INST_2x(pcmpgtd, kInstPcmpgtd, MmReg, MmReg)
  //! \overload
  INST_2x(pcmpgtd, kInstPcmpgtd, MmReg, Mem)

  //! Packed WORDs compare if greater than (MMX).
  INST_2x(pcmpgtw, kInstPcmpgtw, MmReg, MmReg)
  //! \overload
  INST_2x(pcmpgtw, kInstPcmpgtw, MmReg, Mem)

  //! Packed WORDs multiply high (MMX).
  INST_2x(pmulhw, kInstPmulhw, MmReg, MmReg)
  //! \overload
  INST_2x(pmulhw, kInstPmulhw, MmReg, Mem)

  //! Packed WORDs multiply low (MMX).
  INST_2x(pmullw, kInstPmullw, MmReg, MmReg)
  //! \overload
  INST_2x(pmullw, kInstPmullw, MmReg, Mem)

  //! Pakced bitwise or (MMX).
  INST_2x(por, kInstPor, MmReg, MmReg)
  //! \overload
  INST_2x(por, kInstPor, MmReg, Mem)

  //! Packed WORD multiply and add to packed DWORD (MMX).
  INST_2x(pmaddwd, kInstPmaddwd, MmReg, MmReg)
  //! \overload
  INST_2x(pmaddwd, kInstPmaddwd, MmReg, Mem)

  //! Packed DWORD shift left logical (MMX).
  INST_2x(pslld, kInstPslld, MmReg, MmReg)
  //! \overload
  INST_2x(pslld, kInstPslld, MmReg, Mem)
  //! \overload
  INST_2i(pslld, kInstPslld, MmReg, Imm)

  //! Packed QWORD shift left logical (MMX).
  INST_2x(psllq, kInstPsllq, MmReg, MmReg)
  //! \overload
  INST_2x(psllq, kInstPsllq, MmReg, Mem)
  //! \overload
  INST_2i(psllq, kInstPsllq, MmReg, Imm)

  //! Packed WORD shift left logical (MMX).
  INST_2x(psllw, kInstPsllw, MmReg, MmReg)
  //! \overload
  INST_2x(psllw, kInstPsllw, MmReg, Mem)
  //! \overload
  INST_2i(psllw, kInstPsllw, MmReg, Imm)

  //! Packed DWORD shift right arithmetic (MMX).
  INST_2x(psrad, kInstPsrad, MmReg, MmReg)
  //! \overload
  INST_2x(psrad, kInstPsrad, MmReg, Mem)
  //! \overload
  INST_2i(psrad, kInstPsrad, MmReg, Imm)

  //! Packed WORD shift right arithmetic (MMX).
  INST_2x(psraw, kInstPsraw, MmReg, MmReg)
  //! \overload
  INST_2x(psraw, kInstPsraw, MmReg, Mem)
  //! \overload
  INST_2i(psraw, kInstPsraw, MmReg, Imm)

  //! Packed DWORD shift right logical (MMX).
  INST_2x(psrld, kInstPsrld, MmReg, MmReg)
  //! \overload
  INST_2x(psrld, kInstPsrld, MmReg, Mem)
  //! \overload
  INST_2i(psrld, kInstPsrld, MmReg, Imm)

  //! Packed QWORD shift right logical (MMX).
  INST_2x(psrlq, kInstPsrlq, MmReg, MmReg)
  //! \overload
  INST_2x(psrlq, kInstPsrlq, MmReg, Mem)
  //! \overload
  INST_2i(psrlq, kInstPsrlq, MmReg, Imm)

  //! Packed WORD shift right logical (MMX).
  INST_2x(psrlw, kInstPsrlw, MmReg, MmReg)
  //! \overload
  INST_2x(psrlw, kInstPsrlw, MmReg, Mem)
  //! \overload
  INST_2i(psrlw, kInstPsrlw, MmReg, Imm)

  //! Packed BYTE subtract (MMX).
  INST_2x(psubb, kInstPsubb, MmReg, MmReg)
  //! \overload
  INST_2x(psubb, kInstPsubb, MmReg, Mem)

  //! Packed DWORD subtract (MMX).
  INST_2x(psubd, kInstPsubd, MmReg, MmReg)
  //! \overload
  INST_2x(psubd, kInstPsubd, MmReg, Mem)

  //! Packed BYTE subtract with saturation (MMX).
  INST_2x(psubsb, kInstPsubsb, MmReg, MmReg)
  //! \overload
  INST_2x(psubsb, kInstPsubsb, MmReg, Mem)

  //! Packed WORD subtract with saturation (MMX).
  INST_2x(psubsw, kInstPsubsw, MmReg, MmReg)
  //! \overload
  INST_2x(psubsw, kInstPsubsw, MmReg, Mem)

  //! Packed BYTE subtract with unsigned saturation (MMX).
  INST_2x(psubusb, kInstPsubusb, MmReg, MmReg)
  //! \overload
  INST_2x(psubusb, kInstPsubusb, MmReg, Mem)

  //! Packed WORD subtract with unsigned saturation (MMX).
  INST_2x(psubusw, kInstPsubusw, MmReg, MmReg)
  //! \overload
  INST_2x(psubusw, kInstPsubusw, MmReg, Mem)

  //! Packed WORD subtract (MMX).
  INST_2x(psubw, kInstPsubw, MmReg, MmReg)
  //! \overload
  INST_2x(psubw, kInstPsubw, MmReg, Mem)

  //! Unpack high packed BYTEs to WORDs (MMX).
  INST_2x(punpckhbw, kInstPunpckhbw, MmReg, MmReg)
  //! \overload
  INST_2x(punpckhbw, kInstPunpckhbw, MmReg, Mem)

  //! Unpack high packed DWORDs to QWORDs (MMX).
  INST_2x(punpckhdq, kInstPunpckhdq, MmReg, MmReg)
  //! \overload
  INST_2x(punpckhdq, kInstPunpckhdq, MmReg, Mem)

  //! Unpack high packed WORDs to DWORDs (MMX).
  INST_2x(punpckhwd, kInstPunpckhwd, MmReg, MmReg)
  //! \overload
  INST_2x(punpckhwd, kInstPunpckhwd, MmReg, Mem)

  //! Unpack low packed BYTEs to WORDs (MMX).
  INST_2x(punpcklbw, kInstPunpcklbw, MmReg, MmReg)
  //! \overload
  INST_2x(punpcklbw, kInstPunpcklbw, MmReg, Mem)

  //! Unpack low packed DWORDs to QWORDs (MMX).
  INST_2x(punpckldq, kInstPunpckldq, MmReg, MmReg)
  //! \overload
  INST_2x(punpckldq, kInstPunpckldq, MmReg, Mem)

  //! Unpack low packed WORDs to DWORDs (MMX).
  INST_2x(punpcklwd, kInstPunpcklwd, MmReg, MmReg)
  //! \overload
  INST_2x(punpcklwd, kInstPunpcklwd, MmReg, Mem)

  //! Packed bitwise xor (MMX).
  INST_2x(pxor, kInstPxor, MmReg, MmReg)
  //! \overload
  INST_2x(pxor, kInstPxor, MmReg, Mem)

  //! Empty MMX state.
  INST_0x(emms, kInstEmms)

  // -------------------------------------------------------------------------
  // [3dNow]
  // -------------------------------------------------------------------------

  //! Packed SP-FP to DWORD convert (3dNow!).
  INST_2x(pf2id, kInstPf2id, MmReg, MmReg)
  //! \overload
  INST_2x(pf2id, kInstPf2id, MmReg, Mem)

  //!  Packed SP-FP to WORD convert (3dNow!).
  INST_2x(pf2iw, kInstPf2iw, MmReg, MmReg)
  //! \overload
  INST_2x(pf2iw, kInstPf2iw, MmReg, Mem)

  //! Packed SP-FP accumulate (3dNow!).
  INST_2x(pfacc, kInstPfacc, MmReg, MmReg)
  //! \overload
  INST_2x(pfacc, kInstPfacc, MmReg, Mem)

  //! Packed SP-FP addition (3dNow!).
  INST_2x(pfadd, kInstPfadd, MmReg, MmReg)
  //! \overload
  INST_2x(pfadd, kInstPfadd, MmReg, Mem)

  //! Packed SP-FP compare - dst == src (3dNow!).
  INST_2x(pfcmpeq, kInstPfcmpeq, MmReg, MmReg)
  //! \overload
  INST_2x(pfcmpeq, kInstPfcmpeq, MmReg, Mem)

  //! Packed SP-FP compare - dst >= src (3dNow!).
  INST_2x(pfcmpge, kInstPfcmpge, MmReg, MmReg)
  //! \overload
  INST_2x(pfcmpge, kInstPfcmpge, MmReg, Mem)

  //! Packed SP-FP compare - dst > src (3dNow!).
  INST_2x(pfcmpgt, kInstPfcmpgt, MmReg, MmReg)
  //! \overload
  INST_2x(pfcmpgt, kInstPfcmpgt, MmReg, Mem)

  //! Packed SP-FP maximum (3dNow!).
  INST_2x(pfmax, kInstPfmax, MmReg, MmReg)
  //! \overload
  INST_2x(pfmax, kInstPfmax, MmReg, Mem)

  //! Packed SP-FP minimum (3dNow!).
  INST_2x(pfmin, kInstPfmin, MmReg, MmReg)
  //! \overload
  INST_2x(pfmin, kInstPfmin, MmReg, Mem)

  //! Packed SP-FP multiply (3dNow!).
  INST_2x(pfmul, kInstPfmul, MmReg, MmReg)
  //! \overload
  INST_2x(pfmul, kInstPfmul, MmReg, Mem)

  //! Packed SP-FP negative accumulate (3dNow!).
  INST_2x(pfnacc, kInstPfnacc, MmReg, MmReg)
  //! \overload
  INST_2x(pfnacc, kInstPfnacc, MmReg, Mem)

  //! Packed SP-FP mixed accumulate (3dNow!).
  INST_2x(pfpnacc, kInstPfpnacc, MmReg, MmReg)
  //! \overload
  INST_2x(pfpnacc, kInstPfpnacc, MmReg, Mem)

  //! Packed SP-FP reciprocal Approximation (3dNow!).
  INST_2x(pfrcp, kInstPfrcp, MmReg, MmReg)
  //! \overload
  INST_2x(pfrcp, kInstPfrcp, MmReg, Mem)

  //! Packed SP-FP reciprocal, first iteration step (3dNow!).
  INST_2x(pfrcpit1, kInstPfrcpit1, MmReg, MmReg)
  //! \overload
  INST_2x(pfrcpit1, kInstPfrcpit1, MmReg, Mem)

  //! Packed SP-FP reciprocal, second iteration step (3dNow!).
  INST_2x(pfrcpit2, kInstPfrcpit2, MmReg, MmReg)
  //! \overload
  INST_2x(pfrcpit2, kInstPfrcpit2, MmReg, Mem)

  //! Packed SP-FP reciprocal square root, first iteration step (3dNow!).
  INST_2x(pfrsqit1, kInstPfrsqit1, MmReg, MmReg)
  //! \overload
  INST_2x(pfrsqit1, kInstPfrsqit1, MmReg, Mem)

  //! Packed SP-FP reciprocal square root approximation (3dNow!).
  INST_2x(pfrsqrt, kInstPfrsqrt, MmReg, MmReg)
  //! \overload
  INST_2x(pfrsqrt, kInstPfrsqrt, MmReg, Mem)

  //! Packed SP-FP subtract (3dNow!).
  INST_2x(pfsub, kInstPfsub, MmReg, MmReg)
  //! \overload
  INST_2x(pfsub, kInstPfsub, MmReg, Mem)

  //! Packed SP-FP reverse subtract (3dNow!).
  INST_2x(pfsubr, kInstPfsubr, MmReg, MmReg)
  //! \overload
  INST_2x(pfsubr, kInstPfsubr, MmReg, Mem)

  //! Packed DWORDs to SP-FP (3dNow!).
  INST_2x(pi2fd, kInstPi2fd, MmReg, MmReg)
  //! \overload
  INST_2x(pi2fd, kInstPi2fd, MmReg, Mem)

  //! Packed WORDs to SP-FP (3dNow!).
  INST_2x(pi2fw, kInstPi2fw, MmReg, MmReg)
  //! \overload
  INST_2x(pi2fw, kInstPi2fw, MmReg, Mem)

  //! Packed swap DWORDs (3dNow!)
  INST_2x(pswapd, kInstPswapd, MmReg, MmReg)
  //! \overload
  INST_2x(pswapd, kInstPswapd, MmReg, Mem)

  //! Prefetch (3dNow!).
  INST_1x(prefetch3dnow, kInstPrefetch3dNow, Mem)

  //! Prefetch and set cache to modified (3dNow!).
  INST_1x(prefetchw3dnow, kInstPrefetchw3dNow, Mem)

  //! Faster EMMS (3dNow!).
  INST_0x(femms, kInstFemms)

  // --------------------------------------------------------------------------
  // [SSE]
  // --------------------------------------------------------------------------

  //! Packed SP-FP add (SSE).
  INST_2x(addps, kInstAddps, XmmReg, XmmReg)
  //! \overload
  INST_2x(addps, kInstAddps, XmmReg, Mem)

  //! Scalar SP-FP add (SSE).
  INST_2x(addss, kInstAddss, XmmReg, XmmReg)
  //! \overload
  INST_2x(addss, kInstAddss, XmmReg, Mem)

  //! Packed SP-FP bitwise and-not (SSE).
  INST_2x(andnps, kInstAndnps, XmmReg, XmmReg)
  //! \overload
  INST_2x(andnps, kInstAndnps, XmmReg, Mem)

  //! Packed SP-FP bitwise and (SSE).
  INST_2x(andps, kInstAndps, XmmReg, XmmReg)
  //! \overload
  INST_2x(andps, kInstAndps, XmmReg, Mem)

  //! Packed SP-FP compare (SSE).
  INST_3i(cmpps, kInstCmpps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(cmpps, kInstCmpps, XmmReg, Mem, Imm)

  //! Compare scalar SP-FP (SSE).
  INST_3i(cmpss, kInstCmpss, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(cmpss, kInstCmpss, XmmReg, Mem, Imm)

  //! Scalar ordered SP-FP compare and set EFLAGS (SSE).
  INST_2x(comiss, kInstComiss, XmmReg, XmmReg)
  //! \overload
  INST_2x(comiss, kInstComiss, XmmReg, Mem)

  //! Packed signed INT32 to packed SP-FP conversion (SSE).
  INST_2x(cvtpi2ps, kInstCvtpi2ps, XmmReg, MmReg)
  //! \overload
  INST_2x(cvtpi2ps, kInstCvtpi2ps, XmmReg, Mem)

  //! Packed SP-FP to packed INT32 conversion (SSE).
  INST_2x(cvtps2pi, kInstCvtps2pi, MmReg, XmmReg)
  //! \overload
  INST_2x(cvtps2pi, kInstCvtps2pi, MmReg, Mem)

  //! Convert scalar INT32 to SP-FP (SSE).
  INST_2x(cvtsi2ss, kInstCvtsi2ss, XmmReg, GpReg)
  //! \overload
  INST_2x(cvtsi2ss, kInstCvtsi2ss, XmmReg, Mem)

  //! Convert scalar SP-FP to INT32 (SSE).
  INST_2x(cvtss2si, kInstCvtss2si, GpReg, XmmReg)
  //! \overload
  INST_2x(cvtss2si, kInstCvtss2si, GpReg, Mem)

  //! Convert with truncation packed SP-FP to packed INT32 (SSE).
  INST_2x(cvttps2pi, kInstCvttps2pi, MmReg, XmmReg)
  //! \overload
  INST_2x(cvttps2pi, kInstCvttps2pi, MmReg, Mem)

  //! Convert with truncation scalar SP-FP to INT32 (SSE).
  INST_2x(cvttss2si, kInstCvttss2si, GpReg, XmmReg)
  //! \overload
  INST_2x(cvttss2si, kInstCvttss2si, GpReg, Mem)

  //! Packed SP-FP divide (SSE).
  INST_2x(divps, kInstDivps, XmmReg, XmmReg)
  //! \overload
  INST_2x(divps, kInstDivps, XmmReg, Mem)

  //! Scalar SP-FP divide (SSE).
  INST_2x(divss, kInstDivss, XmmReg, XmmReg)
  //! \overload
  INST_2x(divss, kInstDivss, XmmReg, Mem)

  //! Load streaming SIMD extension control/status (SSE).
  INST_1x(ldmxcsr, kInstLdmxcsr, Mem)

  //! Byte mask write to DS:EDI/RDI (SSE).
  INST_2x(maskmovq, kInstMaskmovq, MmReg, MmReg)

  //! Packed SP-FP maximum (SSE).
  INST_2x(maxps, kInstMaxps, XmmReg, XmmReg)
  //! \overload
  INST_2x(maxps, kInstMaxps, XmmReg, Mem)

  //! Scalar SP-FP maximum (SSE).
  INST_2x(maxss, kInstMaxss, XmmReg, XmmReg)
  //! \overload
  INST_2x(maxss, kInstMaxss, XmmReg, Mem)

  //! Packed SP-FP minimum (SSE).
  INST_2x(minps, kInstMinps, XmmReg, XmmReg)
  //! \overload
  INST_2x(minps, kInstMinps, XmmReg, Mem)

  //! Scalar SP-FP minimum (SSE).
  INST_2x(minss, kInstMinss, XmmReg, XmmReg)
  //! \overload
  INST_2x(minss, kInstMinss, XmmReg, Mem)

  //! Move aligned packed SP-FP (SSE).
  INST_2x(movaps, kInstMovaps, XmmReg, XmmReg)
  //! \overload
  INST_2x(movaps, kInstMovaps, XmmReg, Mem)
  //! Move aligned packed SP-FP (SSE).
  INST_2x(movaps, kInstMovaps, Mem, XmmReg)

  //! Move DWORD.
  INST_2x(movd, kInstMovd, Mem, XmmReg)
  //! \overload
  INST_2x(movd, kInstMovd, GpReg, XmmReg)
  //! \overload
  INST_2x(movd, kInstMovd, XmmReg, Mem)
  //! \overload
  INST_2x(movd, kInstMovd, XmmReg, GpReg)

  //! Move QWORD (SSE).
  INST_2x(movq, kInstMovq, XmmReg, XmmReg)
  //! \overload
  INST_2x(movq, kInstMovq, Mem, XmmReg)
  //! \overload
  INST_2x(movq, kInstMovq, XmmReg, Mem)

  //! Move QWORD using NT hint (SSE).
  INST_2x(movntq, kInstMovntq, Mem, MmReg)

  //! Move high to low packed SP-FP (SSE).
  INST_2x(movhlps, kInstMovhlps, XmmReg, XmmReg)

  //! Move high packed SP-FP (SSE).
  INST_2x(movhps, kInstMovhps, XmmReg, Mem)
  //! Move high packed SP-FP (SSE).
  INST_2x(movhps, kInstMovhps, Mem, XmmReg)

  //! Move low to high packed SP-FP (SSE).
  INST_2x(movlhps, kInstMovlhps, XmmReg, XmmReg)

  //! Move low packed SP-FP (SSE).
  INST_2x(movlps, kInstMovlps, XmmReg, Mem)
  //! Move low packed SP-FP (SSE).
  INST_2x(movlps, kInstMovlps, Mem, XmmReg)

  //! Move aligned packed SP-FP using NT hint (SSE).
  INST_2x(movntps, kInstMovntps, Mem, XmmReg)

  //! Move scalar SP-FP (SSE).
  INST_2x(movss, kInstMovss, XmmReg, XmmReg)
  //! \overload
  INST_2x(movss, kInstMovss, XmmReg, Mem)
  //! \overload
  INST_2x(movss, kInstMovss, Mem, XmmReg)

  //! Move unaligned packed SP-FP (SSE).
  INST_2x(movups, kInstMovups, XmmReg, XmmReg)
  //! \overload
  INST_2x(movups, kInstMovups, XmmReg, Mem)
  //! \overload
  INST_2x(movups, kInstMovups, Mem, XmmReg)

  //! Packed SP-FP multiply (SSE).
  INST_2x(mulps, kInstMulps, XmmReg, XmmReg)
  //! \overload
  INST_2x(mulps, kInstMulps, XmmReg, Mem)

  //! Scalar SP-FP multiply (SSE).
  INST_2x(mulss, kInstMulss, XmmReg, XmmReg)
  //! \overload
  INST_2x(mulss, kInstMulss, XmmReg, Mem)

  //! Packed SP-FP bitwise or (SSE).
  INST_2x(orps, kInstOrps, XmmReg, XmmReg)
  //! \overload
  INST_2x(orps, kInstOrps, XmmReg, Mem)

  //! Packed BYTE average (SSE).
  INST_2x(pavgb, kInstPavgb, MmReg, MmReg)
  //! \overload
  INST_2x(pavgb, kInstPavgb, MmReg, Mem)

  //! Packed WORD average (SSE).
  INST_2x(pavgw, kInstPavgw, MmReg, MmReg)
  //! \overload
  INST_2x(pavgw, kInstPavgw, MmReg, Mem)

  //! Extract WORD based on selector (SSE).
  INST_3i(pextrw, kInstPextrw, GpReg, MmReg, Imm)

  //! Insert WORD based on selector (SSE).
  INST_3i(pinsrw, kInstPinsrw, MmReg, GpReg, Imm)
  //! \overload
  INST_3i(pinsrw, kInstPinsrw, MmReg, Mem, Imm)

  //! Packed WORD maximum (SSE).
  INST_2x(pmaxsw, kInstPmaxsw, MmReg, MmReg)
  //! \overload
  INST_2x(pmaxsw, kInstPmaxsw, MmReg, Mem)

  //! Packed BYTE unsigned maximum (SSE).
  INST_2x(pmaxub, kInstPmaxub, MmReg, MmReg)
  //! \overload
  INST_2x(pmaxub, kInstPmaxub, MmReg, Mem)

  //! Packed WORD minimum (SSE).
  INST_2x(pminsw, kInstPminsw, MmReg, MmReg)
  //! \overload
  INST_2x(pminsw, kInstPminsw, MmReg, Mem)

  //! Packed BYTE unsigned minimum (SSE).
  INST_2x(pminub, kInstPminub, MmReg, MmReg)
  //! \overload
  INST_2x(pminub, kInstPminub, MmReg, Mem)

  //! Move Byte mask to integer (SSE).
  INST_2x(pmovmskb, kInstPmovmskb, GpReg, MmReg)

  //! Packed WORD unsigned multiply high (SSE).
  INST_2x(pmulhuw, kInstPmulhuw, MmReg, MmReg)
  //! \overload
  INST_2x(pmulhuw, kInstPmulhuw, MmReg, Mem)

  //! Packed WORD sum of absolute differences (SSE).
  INST_2x(psadbw, kInstPsadbw, MmReg, MmReg)
  //! \overload
  INST_2x(psadbw, kInstPsadbw, MmReg, Mem)

  //! Packed WORD shuffle (SSE).
  INST_3i(pshufw, kInstPshufw, MmReg, MmReg, Imm)
  //! \overload
  INST_3i(pshufw, kInstPshufw, MmReg, Mem, Imm)

  //! Packed SP-FP reciprocal (SSE).
  INST_2x(rcpps, kInstRcpps, XmmReg, XmmReg)
  //! \overload
  INST_2x(rcpps, kInstRcpps, XmmReg, Mem)

  //! Scalar SP-FP reciprocal (SSE).
  INST_2x(rcpss, kInstRcpss, XmmReg, XmmReg)
  //! \overload
  INST_2x(rcpss, kInstRcpss, XmmReg, Mem)

  //! Prefetch (SSE).
  INST_2i(prefetch, kInstPrefetch, Mem, Imm)

  //! Packed WORD sum of absolute differences (SSE).
  INST_2x(psadbw, kInstPsadbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psadbw, kInstPsadbw, XmmReg, Mem)

  //! Packed SP-FP square root reciprocal (SSE).
  INST_2x(rsqrtps, kInstRsqrtps, XmmReg, XmmReg)
  //! \overload
  INST_2x(rsqrtps, kInstRsqrtps, XmmReg, Mem)

  //! Scalar SP-FP square root reciprocal (SSE).
  INST_2x(rsqrtss, kInstRsqrtss, XmmReg, XmmReg)
  //! \overload
  INST_2x(rsqrtss, kInstRsqrtss, XmmReg, Mem)

  //! Store fence (SSE).
  INST_0x(sfence, kInstSfence)

  //! Shuffle SP-FP (SSE).
  INST_3i(shufps, kInstShufps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(shufps, kInstShufps, XmmReg, Mem, Imm)

  //! Packed SP-FP square root (SSE).
  INST_2x(sqrtps, kInstSqrtps, XmmReg, XmmReg)
  //! \overload
  INST_2x(sqrtps, kInstSqrtps, XmmReg, Mem)

  //! Scalar SP-FP square root (SSE).
  INST_2x(sqrtss, kInstSqrtss, XmmReg, XmmReg)
  //! \overload
  INST_2x(sqrtss, kInstSqrtss, XmmReg, Mem)

  //! Store streaming SIMD extension control/status (SSE).
  INST_1x(stmxcsr, kInstStmxcsr, Mem)

  //! Packed SP-FP subtract (SSE).
  INST_2x(subps, kInstSubps, XmmReg, XmmReg)
  //! \overload
  INST_2x(subps, kInstSubps, XmmReg, Mem)

  //! Scalar SP-FP subtract (SSE).
  INST_2x(subss, kInstSubss, XmmReg, XmmReg)
  //! \overload
  INST_2x(subss, kInstSubss, XmmReg, Mem)

  //! Unordered scalar SP-FP compare and set EFLAGS (SSE).
  INST_2x(ucomiss, kInstUcomiss, XmmReg, XmmReg)
  //! \overload
  INST_2x(ucomiss, kInstUcomiss, XmmReg, Mem)

  //! Unpack high packed SP-FP data (SSE).
  INST_2x(unpckhps, kInstUnpckhps, XmmReg, XmmReg)
  //! \overload
  INST_2x(unpckhps, kInstUnpckhps, XmmReg, Mem)

  //! Unpack low packed SP-FP data (SSE).
  INST_2x(unpcklps, kInstUnpcklps, XmmReg, XmmReg)
  //! \overload
  INST_2x(unpcklps, kInstUnpcklps, XmmReg, Mem)

  //! Packed SP-FP bitwise xor (SSE).
  INST_2x(xorps, kInstXorps, XmmReg, XmmReg)
  //! \overload
  INST_2x(xorps, kInstXorps, XmmReg, Mem)

  // --------------------------------------------------------------------------
  // [SSE2]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add (SSE2).
  INST_2x(addpd, kInstAddpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(addpd, kInstAddpd, XmmReg, Mem)

  //! Scalar DP-FP add (SSE2).
  INST_2x(addsd, kInstAddsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(addsd, kInstAddsd, XmmReg, Mem)

  //! Packed DP-FP bitwise and-not (SSE2).
  INST_2x(andnpd, kInstAndnpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(andnpd, kInstAndnpd, XmmReg, Mem)

  //! Packed DP-FP bitwise and (SSE2).
  INST_2x(andpd, kInstAndpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(andpd, kInstAndpd, XmmReg, Mem)

  //! Flush cache line (SSE2).
  INST_1x(clflush, kInstClflush, Mem)

  //! Packed DP-FP compare (SSE2).
  INST_3i(cmppd, kInstCmppd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(cmppd, kInstCmppd, XmmReg, Mem, Imm)

  //! Scalar SP-FP compare (SSE2).
  INST_3i(cmpsd, kInstCmpsd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(cmpsd, kInstCmpsd, XmmReg, Mem, Imm)

  //! Scalar ordered DP-FP compare and set EFLAGS (SSE2).
  INST_2x(comisd, kInstComisd, XmmReg, XmmReg)
  //! \overload
  INST_2x(comisd, kInstComisd, XmmReg, Mem)

  //! Convert packed QWORDs to packed DP-FP (SSE2).
  INST_2x(cvtdq2pd, kInstCvtdq2pd, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtdq2pd, kInstCvtdq2pd, XmmReg, Mem)

  //! Convert packed QWORDs to packed SP-FP (SSE2).
  INST_2x(cvtdq2ps, kInstCvtdq2ps, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtdq2ps, kInstCvtdq2ps, XmmReg, Mem)

  //! Convert packed DP-FP to packed QWORDs (SSE2).
  INST_2x(cvtpd2dq, kInstCvtpd2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtpd2dq, kInstCvtpd2dq, XmmReg, Mem)

  //! Convert packed DP-FP to packed QRODSs (SSE2).
  INST_2x(cvtpd2pi, kInstCvtpd2pi, MmReg, XmmReg)
  //! \overload
  INST_2x(cvtpd2pi, kInstCvtpd2pi, MmReg, Mem)

  //! Convert packed DP-FP to packed SP-FP (SSE2).
  INST_2x(cvtpd2ps, kInstCvtpd2ps, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtpd2ps, kInstCvtpd2ps, XmmReg, Mem)

  //! Convert packed DWORDs integers to packed DP-FP (SSE2).
  INST_2x(cvtpi2pd, kInstCvtpi2pd, XmmReg, MmReg)
  //! \overload
  INST_2x(cvtpi2pd, kInstCvtpi2pd, XmmReg, Mem)

  //! Convert packed SP-FP to packed QWORDs (SSE2).
  INST_2x(cvtps2dq, kInstCvtps2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtps2dq, kInstCvtps2dq, XmmReg, Mem)

  //! Convert packed SP-FP to packed DP-FP (SSE2).
  INST_2x(cvtps2pd, kInstCvtps2pd, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtps2pd, kInstCvtps2pd, XmmReg, Mem)

  //! Convert scalar DP-FP to DWORD integer (SSE2).
  INST_2x(cvtsd2si, kInstCvtsd2si, GpReg, XmmReg)
  //! \overload
  INST_2x(cvtsd2si, kInstCvtsd2si, GpReg, Mem)

  //! Convert scalar DP-FP to scalar SP-FP (SSE2).
  INST_2x(cvtsd2ss, kInstCvtsd2ss, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtsd2ss, kInstCvtsd2ss, XmmReg, Mem)

  //! Convert DWORD integer to scalar DP-FP (SSE2).
  INST_2x(cvtsi2sd, kInstCvtsi2sd, XmmReg, GpReg)
  //! \overload
  INST_2x(cvtsi2sd, kInstCvtsi2sd, XmmReg, Mem)

  //! Convert scalar SP-FP to DP-FP (SSE2).
  INST_2x(cvtss2sd, kInstCvtss2sd, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvtss2sd, kInstCvtss2sd, XmmReg, Mem)

  //! Convert with truncation packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvttpd2pi, kInstCvttpd2pi, MmReg, XmmReg)
  //! \overload
  INST_2x(cvttpd2pi, kInstCvttpd2pi, MmReg, Mem)

  //! Convert with truncation packed DP-FP to packed QWORDs (SSE2).
  INST_2x(cvttpd2dq, kInstCvttpd2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvttpd2dq, kInstCvttpd2dq, XmmReg, Mem)

  //! Convert with truncation packed SP-FP to packed QWORDs (SSE2).
  INST_2x(cvttps2dq, kInstCvttps2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(cvttps2dq, kInstCvttps2dq, XmmReg, Mem)

  //! Convert with truncation scalar DP-FP to signed DWORDs (SSE2).
  INST_2x(cvttsd2si, kInstCvttsd2si, GpReg, XmmReg)
  //! \overload
  INST_2x(cvttsd2si, kInstCvttsd2si, GpReg, Mem)

  //! Packed DP-FP divide (SSE2).
  INST_2x(divpd, kInstDivpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(divpd, kInstDivpd, XmmReg, Mem)

  //! Scalar DP-FP divide (SSE2).
  INST_2x(divsd, kInstDivsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(divsd, kInstDivsd, XmmReg, Mem)

  //! Load fence (SSE2).
  INST_0x(lfence, kInstLfence)

  //! Store selected bytes of OWORD to DS:EDI/RDI (SSE2).
  INST_2x(maskmovdqu, kInstMaskmovdqu, XmmReg, XmmReg)

  //! Packed DP-FP maximum (SSE2).
  INST_2x(maxpd, kInstMaxpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(maxpd, kInstMaxpd, XmmReg, Mem)

  //! Scalar DP-FP maximum (SSE2).
  INST_2x(maxsd, kInstMaxsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(maxsd, kInstMaxsd, XmmReg, Mem)

  //! Memory fence (SSE2).
  INST_0x(mfence, kInstMfence)

  //! Packed DP-FP minimum (SSE2).
  INST_2x(minpd, kInstMinpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(minpd, kInstMinpd, XmmReg, Mem)

  //! Scalar DP-FP minimum (SSE2).
  INST_2x(minsd, kInstMinsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(minsd, kInstMinsd, XmmReg, Mem)

  //! Move aligned OWORD (SSE2).
  INST_2x(movdqa, kInstMovdqa, XmmReg, XmmReg)
  //! \overload
  INST_2x(movdqa, kInstMovdqa, XmmReg, Mem)
  //! \overload
  INST_2x(movdqa, kInstMovdqa, Mem, XmmReg)

  //! Move unaligned OWORD (SSE2).
  INST_2x(movdqu, kInstMovdqu, XmmReg, XmmReg)
  //! \overload
  INST_2x(movdqu, kInstMovdqu, XmmReg, Mem)
  //! \overload
  INST_2x(movdqu, kInstMovdqu, Mem, XmmReg)

  //! Extract packed SP-FP sign mask (SSE2).
  INST_2x(movmskps, kInstMovmskps, GpReg, XmmReg)

  //! Extract packed DP-FP sign mask (SSE2).
  INST_2x(movmskpd, kInstMovmskpd, GpReg, XmmReg)

  //! Move scalar DP-FP (SSE2).
  INST_2x(movsd, kInstMovsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(movsd, kInstMovsd, XmmReg, Mem)
  //! \overload
  INST_2x(movsd, kInstMovsd, Mem, XmmReg)

  //! Move aligned packed DP-FP (SSE2).
  INST_2x(movapd, kInstMovapd, XmmReg, XmmReg)
  //! \overload
  INST_2x(movapd, kInstMovapd, XmmReg, Mem)
  //! \overload
  INST_2x(movapd, kInstMovapd, Mem, XmmReg)

  //! Move QWORD from Xmm to Mm register (SSE2).
  INST_2x(movdq2q, kInstMovdq2q, MmReg, XmmReg)

  //! Move QWORD from Mm to Xmm register (SSE2).
  INST_2x(movq2dq, kInstMovq2dq, XmmReg, MmReg)

  //! Move high packed DP-FP (SSE2).
  INST_2x(movhpd, kInstMovhpd, XmmReg, Mem)
  //! \overload
  INST_2x(movhpd, kInstMovhpd, Mem, XmmReg)

  //! Move low packed DP-FP (SSE2).
  INST_2x(movlpd, kInstMovlpd, XmmReg, Mem)
  //! \overload
  INST_2x(movlpd, kInstMovlpd, Mem, XmmReg)

  //! Store OWORD using NT hint (SSE2).
  INST_2x(movntdq, kInstMovntdq, Mem, XmmReg)

  //! Store DWORD using NT hint (SSE2).
  INST_2x(movnti, kInstMovnti, Mem, GpReg)

  //! Store packed DP-FP using NT hint (SSE2).
  INST_2x(movntpd, kInstMovntpd, Mem, XmmReg)

  //! Move unaligned packed DP-FP (SSE2).
  INST_2x(movupd, kInstMovupd, XmmReg, XmmReg)
  //! \overload
  INST_2x(movupd, kInstMovupd, XmmReg, Mem)
  //! \overload
  INST_2x(movupd, kInstMovupd, Mem, XmmReg)

  //! Packed DP-FP multiply (SSE2).
  INST_2x(mulpd, kInstMulpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(mulpd, kInstMulpd, XmmReg, Mem)

  //! Scalar DP-FP multiply (SSE2).
  INST_2x(mulsd, kInstMulsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(mulsd, kInstMulsd, XmmReg, Mem)

  //! Packed DP-FP bitwise or (SSE2).
  INST_2x(orpd, kInstOrpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(orpd, kInstOrpd, XmmReg, Mem)

  //! Pack WORDs to BYTEs with signed saturation (SSE2).
  INST_2x(packsswb, kInstPacksswb, XmmReg, XmmReg)
  //! \overload
  INST_2x(packsswb, kInstPacksswb, XmmReg, Mem)

  //! Pack DWORDs to WORDs with signed saturation (SSE2).
  INST_2x(packssdw, kInstPackssdw, XmmReg, XmmReg)
  //! \overload
  INST_2x(packssdw, kInstPackssdw, XmmReg, Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (SSE2).
  INST_2x(packuswb, kInstPackuswb, XmmReg, XmmReg)
  //! \overload
  INST_2x(packuswb, kInstPackuswb, XmmReg, Mem)

  //! Packed BYTE Add (SSE2).
  INST_2x(paddb, kInstPaddb, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddb, kInstPaddb, XmmReg, Mem)

  //! Packed WORD add (SSE2).
  INST_2x(paddw, kInstPaddw, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddw, kInstPaddw, XmmReg, Mem)

  //! Packed DWORD add (SSE2).
  INST_2x(paddd, kInstPaddd, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddd, kInstPaddd, XmmReg, Mem)

  //! Packed QWORD add (SSE2).
  INST_2x(paddq, kInstPaddq, MmReg, MmReg)
  //! \overload
  INST_2x(paddq, kInstPaddq, MmReg, Mem)

  //! Packed QWORD add (SSE2).
  INST_2x(paddq, kInstPaddq, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddq, kInstPaddq, XmmReg, Mem)

  //! Packed BYTE add with saturation (SSE2).
  INST_2x(paddsb, kInstPaddsb, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddsb, kInstPaddsb, XmmReg, Mem)

  //! Packed WORD add with saturation (SSE2).
  INST_2x(paddsw, kInstPaddsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddsw, kInstPaddsw, XmmReg, Mem)

  //! Packed BYTE add with unsigned saturation (SSE2).
  INST_2x(paddusb, kInstPaddusb, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddusb, kInstPaddusb, XmmReg, Mem)

  //! Packed WORD add with unsigned saturation (SSE2).
  INST_2x(paddusw, kInstPaddusw, XmmReg, XmmReg)
  //! \overload
  INST_2x(paddusw, kInstPaddusw, XmmReg, Mem)

  //! Packed bitwise and (SSE2).
  INST_2x(pand, kInstPand, XmmReg, XmmReg)
  //! \overload
  INST_2x(pand, kInstPand, XmmReg, Mem)

  //! Packed bitwise and-not (SSE2).
  INST_2x(pandn, kInstPandn, XmmReg, XmmReg)
  //! \overload
  INST_2x(pandn, kInstPandn, XmmReg, Mem)

  //! Spin loop hint (SSE2).
  INST_0x(pause, kInstPause)

  //! Packed BYTE average (SSE2).
  INST_2x(pavgb, kInstPavgb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pavgb, kInstPavgb, XmmReg, Mem)

  //! Packed WORD average (SSE2).
  INST_2x(pavgw, kInstPavgw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pavgw, kInstPavgw, XmmReg, Mem)

  //! Packed BYTE compare for equality (SSE2).
  INST_2x(pcmpeqb, kInstPcmpeqb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpeqb, kInstPcmpeqb, XmmReg, Mem)

  //! Packed WORD compare for equality (SSE2).
  INST_2x(pcmpeqw, kInstPcmpeqw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpeqw, kInstPcmpeqw, XmmReg, Mem)

  //! Packed DWORD compare for equality (SSE2).
  INST_2x(pcmpeqd, kInstPcmpeqd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpeqd, kInstPcmpeqd, XmmReg, Mem)

  //! Packed BYTE compare if greater than (SSE2).
  INST_2x(pcmpgtb, kInstPcmpgtb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpgtb, kInstPcmpgtb, XmmReg, Mem)

  //! Packed WORD compare if greater than (SSE2).
  INST_2x(pcmpgtw, kInstPcmpgtw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpgtw, kInstPcmpgtw, XmmReg, Mem)

  //! Packed DWORD compare if greater than (SSE2).
  INST_2x(pcmpgtd, kInstPcmpgtd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpgtd, kInstPcmpgtd, XmmReg, Mem)

  //! Extract WORD based on selector (SSE2).
  INST_3i(pextrw, kInstPextrw, GpReg, XmmReg, Imm)

  //! Insert WORD based on selector (SSE2).
  INST_3i(pinsrw, kInstPinsrw, XmmReg, GpReg, Imm)
  //! \overload
  INST_3i(pinsrw, kInstPinsrw, XmmReg, Mem, Imm)

  //! Packed WORD maximum (SSE2).
  INST_2x(pmaxsw, kInstPmaxsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaxsw, kInstPmaxsw, XmmReg, Mem)

  //! Packed BYTE unsigned maximum (SSE2).
  INST_2x(pmaxub, kInstPmaxub, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaxub, kInstPmaxub, XmmReg, Mem)

  //! Packed WORD minimum (SSE2).
  INST_2x(pminsw, kInstPminsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pminsw, kInstPminsw, XmmReg, Mem)

  //! Packed BYTE unsigned minimum (SSE2).
  INST_2x(pminub, kInstPminub, XmmReg, XmmReg)
  //! \overload
  INST_2x(pminub, kInstPminub, XmmReg, Mem)

  //! Move byte mask (SSE2).
  INST_2x(pmovmskb, kInstPmovmskb, GpReg, XmmReg)

  //! Packed WORD multiply high (SSE2).
  INST_2x(pmulhw, kInstPmulhw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmulhw, kInstPmulhw, XmmReg, Mem)

  //! Packed WORD unsigned multiply high (SSE2).
  INST_2x(pmulhuw, kInstPmulhuw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmulhuw, kInstPmulhuw, XmmReg, Mem)

  //! Packed WORD multiply low (SSE2).
  INST_2x(pmullw, kInstPmullw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmullw, kInstPmullw, XmmReg, Mem)

  //! Packed DWORD multiply to QWORD (SSE2).
  INST_2x(pmuludq, kInstPmuludq, MmReg, MmReg)
  //! \overload
  INST_2x(pmuludq, kInstPmuludq, MmReg, Mem)

  //! Packed DWORD multiply to QWORD (SSE2).
  INST_2x(pmuludq, kInstPmuludq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmuludq, kInstPmuludq, XmmReg, Mem)

  //! Packed bitwise or (SSE2).
  INST_2x(por, kInstPor, XmmReg, XmmReg)
  //! \overload
  INST_2x(por, kInstPor, XmmReg, Mem)

  //! Packed DWORD shift left logical (SSE2).
  INST_2x(pslld, kInstPslld, XmmReg, XmmReg)
  //! \overload
  INST_2x(pslld, kInstPslld, XmmReg, Mem)
  //! \overload
  INST_2i(pslld, kInstPslld, XmmReg, Imm)

  //! Packed QWORD shift left logical (SSE2).
  INST_2x(psllq, kInstPsllq, XmmReg, XmmReg)
  //! \overload
  INST_2x(psllq, kInstPsllq, XmmReg, Mem)
  //! \overload
  INST_2i(psllq, kInstPsllq, XmmReg, Imm)

  //! Packed WORD shift left logical (SSE2).
  INST_2x(psllw, kInstPsllw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psllw, kInstPsllw, XmmReg, Mem)
  //! \overload
  INST_2i(psllw, kInstPsllw, XmmReg, Imm)

  //! Packed OWORD shift left logical (SSE2).
  INST_2i(pslldq, kInstPslldq, XmmReg, Imm)

  //! Packed DWORD shift right arithmetic (SSE2).
  INST_2x(psrad, kInstPsrad, XmmReg, XmmReg)
  //! \overload
  INST_2x(psrad, kInstPsrad, XmmReg, Mem)
  //! \overload
  INST_2i(psrad, kInstPsrad, XmmReg, Imm)

  //! Packed WORD shift right arithmetic (SSE2).
  INST_2x(psraw, kInstPsraw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psraw, kInstPsraw, XmmReg, Mem)
  //! \overload
  INST_2i(psraw, kInstPsraw, XmmReg, Imm)

  //! Packed BYTE subtract (SSE2).
  INST_2x(psubb, kInstPsubb, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubb, kInstPsubb, XmmReg, Mem)

  //! Packed DWORD subtract (SSE2).
  INST_2x(psubd, kInstPsubd, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubd, kInstPsubd, XmmReg, Mem)

  //! Packed QWORD subtract (SSE2).
  INST_2x(psubq, kInstPsubq, MmReg, MmReg)
  //! \overload
  INST_2x(psubq, kInstPsubq, MmReg, Mem)

  //! Packed QWORD subtract (SSE2).
  INST_2x(psubq, kInstPsubq, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubq, kInstPsubq, XmmReg, Mem)

  //! Packed WORD subtract (SSE2).
  INST_2x(psubw, kInstPsubw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubw, kInstPsubw, XmmReg, Mem)

  //! Packed WORD to DWORD multiply and add (SSE2).
  INST_2x(pmaddwd, kInstPmaddwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaddwd, kInstPmaddwd, XmmReg, Mem)

  //! Packed DWORD shuffle (SSE2).
  INST_3i(pshufd, kInstPshufd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pshufd, kInstPshufd, XmmReg, Mem, Imm)

  //! Packed WORD shuffle high (SSE2).
  INST_3i(pshufhw, kInstPshufhw, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pshufhw, kInstPshufhw, XmmReg, Mem, Imm)

  //! Packed WORD shuffle low (SSE2).
  INST_3i(pshuflw, kInstPshuflw, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pshuflw, kInstPshuflw, XmmReg, Mem, Imm)

  //! Packed DWORD shift right logical (SSE2).
  INST_2x(psrld, kInstPsrld, XmmReg, XmmReg)
  //! \overload
  INST_2x(psrld, kInstPsrld, XmmReg, Mem)
  //! \overload
  INST_2i(psrld, kInstPsrld, XmmReg, Imm)

  //! Packed QWORD shift right logical (SSE2).
  INST_2x(psrlq, kInstPsrlq, XmmReg, XmmReg)
  //! \overload
  INST_2x(psrlq, kInstPsrlq, XmmReg, Mem)
  //! \overload
  INST_2i(psrlq, kInstPsrlq, XmmReg, Imm)

  //! Scalar OWORD shift right logical (SSE2).
  INST_2i(psrldq, kInstPsrldq, XmmReg, Imm)

  //! Packed WORD shift right logical (SSE2).
  INST_2x(psrlw, kInstPsrlw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psrlw, kInstPsrlw, XmmReg, Mem)
  //! \overload
  INST_2i(psrlw, kInstPsrlw, XmmReg, Imm)

  //! Packed BYTE subtract with saturation (SSE2).
  INST_2x(psubsb, kInstPsubsb, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubsb, kInstPsubsb, XmmReg, Mem)

  //! Packed WORD subtract with saturation (SSE2).
  INST_2x(psubsw, kInstPsubsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubsw, kInstPsubsw, XmmReg, Mem)

  //! Packed BYTE subtract with unsigned saturation (SSE2).
  INST_2x(psubusb, kInstPsubusb, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubusb, kInstPsubusb, XmmReg, Mem)

  //! Packed WORD subtract with unsigned saturation (SSE2).
  INST_2x(psubusw, kInstPsubusw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psubusw, kInstPsubusw, XmmReg, Mem)

  //! Unpack high packed BYTEs to WORDs (SSE2).
  INST_2x(punpckhbw, kInstPunpckhbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpckhbw, kInstPunpckhbw, XmmReg, Mem)

  //! Unpack high packed DWORDs to QWORDs (SSE2).
  INST_2x(punpckhdq, kInstPunpckhdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpckhdq, kInstPunpckhdq, XmmReg, Mem)

  //! Unpack high packed QWORDs to OWORD (SSE2).
  INST_2x(punpckhqdq, kInstPunpckhqdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpckhqdq, kInstPunpckhqdq, XmmReg, Mem)

  //! Unpack high packed WORDs to DWORDs (SSE2).
  INST_2x(punpckhwd, kInstPunpckhwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpckhwd, kInstPunpckhwd, XmmReg, Mem)

  //! Unpack low packed BYTEs to WORDs (SSE2).
  INST_2x(punpcklbw, kInstPunpcklbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpcklbw, kInstPunpcklbw, XmmReg, Mem)

  //! Unpack low packed DWORDs to QWORDs (SSE2).
  INST_2x(punpckldq, kInstPunpckldq, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpckldq, kInstPunpckldq, XmmReg, Mem)

  //! Unpack low packed QWORDs to OWORD (SSE2).
  INST_2x(punpcklqdq, kInstPunpcklqdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpcklqdq, kInstPunpcklqdq, XmmReg, Mem)

  //! Unpack low packed WORDs to DWORDs (SSE2).
  INST_2x(punpcklwd, kInstPunpcklwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(punpcklwd, kInstPunpcklwd, XmmReg, Mem)

  //! Packed bitwise xor (SSE2).
  INST_2x(pxor, kInstPxor, XmmReg, XmmReg)
  //! \overload
  INST_2x(pxor, kInstPxor, XmmReg, Mem)

  //! Shuffle DP-FP (SSE2).
  INST_3i(shufpd, kInstShufpd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(shufpd, kInstShufpd, XmmReg, Mem, Imm)

  //! Packed DP-FP square root (SSE2).
  INST_2x(sqrtpd, kInstSqrtpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(sqrtpd, kInstSqrtpd, XmmReg, Mem)

  //! Scalar DP-FP square root (SSE2).
  INST_2x(sqrtsd, kInstSqrtsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(sqrtsd, kInstSqrtsd, XmmReg, Mem)

  //! Packed DP-FP subtract (SSE2).
  INST_2x(subpd, kInstSubpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(subpd, kInstSubpd, XmmReg, Mem)

  //! Scalar DP-FP subtract (SSE2).
  INST_2x(subsd, kInstSubsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(subsd, kInstSubsd, XmmReg, Mem)

  //! Scalar DP-FP unordered compare and set EFLAGS (SSE2).
  INST_2x(ucomisd, kInstUcomisd, XmmReg, XmmReg)
  //! \overload
  INST_2x(ucomisd, kInstUcomisd, XmmReg, Mem)

  //! Unpack and interleave high packed DP-FP (SSE2).
  INST_2x(unpckhpd, kInstUnpckhpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(unpckhpd, kInstUnpckhpd, XmmReg, Mem)

  //! Unpack and interleave low packed DP-FP (SSE2).
  INST_2x(unpcklpd, kInstUnpcklpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(unpcklpd, kInstUnpcklpd, XmmReg, Mem)

  //! Packed DP-FP bitwise xor (SSE2).
  INST_2x(xorpd, kInstXorpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(xorpd, kInstXorpd, XmmReg, Mem)

  // --------------------------------------------------------------------------
  // [SSE3]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add/subtract (SSE3).
  INST_2x(addsubpd, kInstAddsubpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(addsubpd, kInstAddsubpd, XmmReg, Mem)

  //! Packed SP-FP add/subtract (SSE3).
  INST_2x(addsubps, kInstAddsubps, XmmReg, XmmReg)
  //! \overload
  INST_2x(addsubps, kInstAddsubps, XmmReg, Mem)

  //! Store integer with truncation (SSE3).
  INST_1x(fisttp, kInstFisttp, Mem)

  //! Packed DP-FP horizontal add (SSE3).
  INST_2x(haddpd, kInstHaddpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(haddpd, kInstHaddpd, XmmReg, Mem)

  //! Packed SP-FP horizontal add (SSE3).
  INST_2x(haddps, kInstHaddps, XmmReg, XmmReg)
  //! \overload
  INST_2x(haddps, kInstHaddps, XmmReg, Mem)

  //! Packed DP-FP horizontal subtract (SSE3).
  INST_2x(hsubpd, kInstHsubpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(hsubpd, kInstHsubpd, XmmReg, Mem)

  //! Packed SP-FP horizontal subtract (SSE3).
  INST_2x(hsubps, kInstHsubps, XmmReg, XmmReg)
  //! \overload
  INST_2x(hsubps, kInstHsubps, XmmReg, Mem)

  //! Load 128-bits unaligned (SSE3).
  INST_2x(lddqu, kInstLddqu, XmmReg, Mem)

  //! Setup monitor address (SSE3).
  INST_0x(monitor, kInstMonitor)

  //! Move one DP-FP and duplicate (SSE3).
  INST_2x(movddup, kInstMovddup, XmmReg, XmmReg)
  //! \overload
  INST_2x(movddup, kInstMovddup, XmmReg, Mem)

  //! Move packed SP-FP high and duplicate (SSE3).
  INST_2x(movshdup, kInstMovshdup, XmmReg, XmmReg)
  //! \overload
  INST_2x(movshdup, kInstMovshdup, XmmReg, Mem)

  //! Move packed SP-FP low and duplicate (SSE3).
  INST_2x(movsldup, kInstMovsldup, XmmReg, XmmReg)
  //! \overload
  INST_2x(movsldup, kInstMovsldup, XmmReg, Mem)

  //! Monitor wait (SSE3).
  INST_0x(mwait, kInstMwait)

  // --------------------------------------------------------------------------
  // [SSSE3]
  // --------------------------------------------------------------------------

  //! Packed BYTE sign (SSSE3).
  INST_2x(psignb, kInstPsignb, MmReg, MmReg)
  //! \overload
  INST_2x(psignb, kInstPsignb, MmReg, Mem)

  //! Packed BYTE sign (SSSE3).
  INST_2x(psignb, kInstPsignb, XmmReg, XmmReg)
  //! \overload
  INST_2x(psignb, kInstPsignb, XmmReg, Mem)

  //! Packed DWORD sign (SSSE3).
  INST_2x(psignd, kInstPsignd, MmReg, MmReg)
  //! \overload
  INST_2x(psignd, kInstPsignd, MmReg, Mem)

  //! Packed DWORD sign (SSSE3).
  INST_2x(psignd, kInstPsignd, XmmReg, XmmReg)
  //! \overload
  INST_2x(psignd, kInstPsignd, XmmReg, Mem)

  //! Packed WORD sign (SSSE3).
  INST_2x(psignw, kInstPsignw, MmReg, MmReg)
  //! \overload
  INST_2x(psignw, kInstPsignw, MmReg, Mem)

  //! Packed WORD sign (SSSE3).
  INST_2x(psignw, kInstPsignw, XmmReg, XmmReg)
  //! \overload
  INST_2x(psignw, kInstPsignw, XmmReg, Mem)

  //! Packed DWORD horizontal add (SSSE3).
  INST_2x(phaddd, kInstPhaddd, MmReg, MmReg)
  //! \overload
  INST_2x(phaddd, kInstPhaddd, MmReg, Mem)

  //! Packed DWORD horizontal add (SSSE3).
  INST_2x(phaddd, kInstPhaddd, XmmReg, XmmReg)
  //! \overload
  INST_2x(phaddd, kInstPhaddd, XmmReg, Mem)

  //! Packed WORD horizontal add with saturation (SSSE3).
  INST_2x(phaddsw, kInstPhaddsw, MmReg, MmReg)
  //! \overload
  INST_2x(phaddsw, kInstPhaddsw, MmReg, Mem)

  //! Packed WORD horizontal add with saturation (SSSE3).
  INST_2x(phaddsw, kInstPhaddsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(phaddsw, kInstPhaddsw, XmmReg, Mem)

  //! Packed WORD horizontal add (SSSE3).
  INST_2x(phaddw, kInstPhaddw, MmReg, MmReg)
  //! \overload
  INST_2x(phaddw, kInstPhaddw, MmReg, Mem)

  //! Packed WORD horizontal add (SSSE3).
  INST_2x(phaddw, kInstPhaddw, XmmReg, XmmReg)
  //! \overload
  INST_2x(phaddw, kInstPhaddw, XmmReg, Mem)

  //! Packed DWORD horizontal subtract (SSSE3).
  INST_2x(phsubd, kInstPhsubd, MmReg, MmReg)
  //! \overload
  INST_2x(phsubd, kInstPhsubd, MmReg, Mem)

  //! Packed DWORD horizontal subtract (SSSE3).
  INST_2x(phsubd, kInstPhsubd, XmmReg, XmmReg)
  //! \overload
  INST_2x(phsubd, kInstPhsubd, XmmReg, Mem)

  //! Packed WORD horizontal subtract with saturation (SSSE3).
  INST_2x(phsubsw, kInstPhsubsw, MmReg, MmReg)
  //! \overload
  INST_2x(phsubsw, kInstPhsubsw, MmReg, Mem)

  //! Packed WORD horizontal subtract with saturation (SSSE3).
  INST_2x(phsubsw, kInstPhsubsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(phsubsw, kInstPhsubsw, XmmReg, Mem)

  //! Packed WORD horizontal subtract (SSSE3).
  INST_2x(phsubw, kInstPhsubw, MmReg, MmReg)
  //! \overload
  INST_2x(phsubw, kInstPhsubw, MmReg, Mem)

  //! Packed WORD horizontal subtract (SSSE3).
  INST_2x(phsubw, kInstPhsubw, XmmReg, XmmReg)
  //! \overload
  INST_2x(phsubw, kInstPhsubw, XmmReg, Mem)

  //! Packed multiply and add signed and unsigned bytes (SSSE3).
  INST_2x(pmaddubsw, kInstPmaddubsw, MmReg, MmReg)
  //! \overload
  INST_2x(pmaddubsw, kInstPmaddubsw, MmReg, Mem)

  //! Packed multiply and add signed and unsigned bytes (SSSE3).
  INST_2x(pmaddubsw, kInstPmaddubsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaddubsw, kInstPmaddubsw, XmmReg, Mem)

  //! Packed BYTE absolute value (SSSE3).
  INST_2x(pabsb, kInstPabsb, MmReg, MmReg)
  //! \overload
  INST_2x(pabsb, kInstPabsb, MmReg, Mem)

  //! Packed BYTE absolute value (SSSE3).
  INST_2x(pabsb, kInstPabsb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pabsb, kInstPabsb, XmmReg, Mem)

  //! Packed DWORD absolute value (SSSE3).
  INST_2x(pabsd, kInstPabsd, MmReg, MmReg)
  //! \overload
  INST_2x(pabsd, kInstPabsd, MmReg, Mem)

  //! Packed DWORD absolute value (SSSE3).
  INST_2x(pabsd, kInstPabsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pabsd, kInstPabsd, XmmReg, Mem)

  //! Packed WORD absolute value (SSSE3).
  INST_2x(pabsw, kInstPabsw, MmReg, MmReg)
  //! \overload
  INST_2x(pabsw, kInstPabsw, MmReg, Mem)

  //! Packed WORD absolute value (SSSE3).
  INST_2x(pabsw, kInstPabsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pabsw, kInstPabsw, XmmReg, Mem)

  //! Packed WORD multiply high, round and scale (SSSE3).
  INST_2x(pmulhrsw, kInstPmulhrsw, MmReg, MmReg)
  //! \overload
  INST_2x(pmulhrsw, kInstPmulhrsw, MmReg, Mem)

  //! Packed WORD multiply high, round and scale (SSSE3).
  INST_2x(pmulhrsw, kInstPmulhrsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmulhrsw, kInstPmulhrsw, XmmReg, Mem)

  //! Packed BYTE shuffle (SSSE3).
  INST_2x(pshufb, kInstPshufb, MmReg, MmReg)
  //! \overload
  INST_2x(pshufb, kInstPshufb, MmReg, Mem)

  //! Packed BYTE shuffle (SSSE3).
  INST_2x(pshufb, kInstPshufb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pshufb, kInstPshufb, XmmReg, Mem)

  //! Packed align right (SSSE3).
  INST_3i(palignr, kInstPalignr, MmReg, MmReg, Imm)
  //! \overload
  INST_3i(palignr, kInstPalignr, MmReg, Mem, Imm)

  //! Packed align right (SSSE3).
  INST_3i(palignr, kInstPalignr, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(palignr, kInstPalignr, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [SSE4.1]
  // --------------------------------------------------------------------------

  //! Packed DP-FP blend (SSE4.1).
  INST_3i(blendpd, kInstBlendpd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(blendpd, kInstBlendpd, XmmReg, Mem, Imm)

  //! Packed SP-FP blend (SSE4.1).
  INST_3i(blendps, kInstBlendps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(blendps, kInstBlendps, XmmReg, Mem, Imm)

  //! Packed DP-FP variable blend (SSE4.1).
  INST_2x(blendvpd, kInstBlendvpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(blendvpd, kInstBlendvpd, XmmReg, Mem)

  //! Packed SP-FP variable blend (SSE4.1).
  INST_2x(blendvps, kInstBlendvps, XmmReg, XmmReg)
  //! \overload
  INST_2x(blendvps, kInstBlendvps, XmmReg, Mem)

  //! Packed DP-FP dot product (SSE4.1).
  INST_3i(dppd, kInstDppd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(dppd, kInstDppd, XmmReg, Mem, Imm)

  //! Packed SP-FP dot product (SSE4.1).
  INST_3i(dpps, kInstDpps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(dpps, kInstDpps, XmmReg, Mem, Imm)

  //! Extract SP-FP based on selector (SSE4.1).
  INST_3i(extractps, kInstExtractps, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(extractps, kInstExtractps, Mem, XmmReg, Imm)

  //! Insert SP-FP based on selector (SSE4.1).
  INST_3i(insertps, kInstInsertps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(insertps, kInstInsertps, XmmReg, Mem, Imm)

  //! Load OWORD aligned using NT hint (SSE4.1).
  INST_2x(movntdqa, kInstMovntdqa, XmmReg, Mem)

  //! Packed WORD sums of absolute difference (SSE4.1).
  INST_3i(mpsadbw, kInstMpsadbw, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(mpsadbw, kInstMpsadbw, XmmReg, Mem, Imm)

  //! Pack DWORDs to WORDs with unsigned saturation (SSE4.1).
  INST_2x(packusdw, kInstPackusdw, XmmReg, XmmReg)
  //! \overload
  INST_2x(packusdw, kInstPackusdw, XmmReg, Mem)

  //! Packed BYTE variable blend (SSE4.1).
  INST_2x(pblendvb, kInstPblendvb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pblendvb, kInstPblendvb, XmmReg, Mem)

  //! Packed WORD blend (SSE4.1).
  INST_3i(pblendw, kInstPblendw, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pblendw, kInstPblendw, XmmReg, Mem, Imm)

  //! Packed QWORD compare for equality (SSE4.1).
  INST_2x(pcmpeqq, kInstPcmpeqq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpeqq, kInstPcmpeqq, XmmReg, Mem)

  //! Extract BYTE based on selector (SSE4.1).
  INST_3i(pextrb, kInstPextrb, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(pextrb, kInstPextrb, Mem, XmmReg, Imm)

  //! Extract DWORD based on selector (SSE4.1).
  INST_3i(pextrd, kInstPextrd, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(pextrd, kInstPextrd, Mem, XmmReg, Imm)

  //! Extract QWORD based on selector (SSE4.1).
  INST_3i(pextrq, kInstPextrq, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(pextrq, kInstPextrq, Mem, XmmReg, Imm)

  //! Extract WORD based on selector (SSE4.1).
  INST_3i(pextrw, kInstPextrw, Mem, XmmReg, Imm)

  //! Packed WORD horizontal minimum (SSE4.1).
  INST_2x(phminposuw, kInstPhminposuw, XmmReg, XmmReg)
  //! \overload
  INST_2x(phminposuw, kInstPhminposuw, XmmReg, Mem)

  //! Insert BYTE based on selector (SSE4.1).
  INST_3i(pinsrb, kInstPinsrb, XmmReg, GpReg, Imm)
  //! \overload
  INST_3i(pinsrb, kInstPinsrb, XmmReg, Mem, Imm)

  //! Insert DWORD based on selector (SSE4.1).
  INST_3i(pinsrd, kInstPinsrd, XmmReg, GpReg, Imm)
  //! \overload
  INST_3i(pinsrd, kInstPinsrd, XmmReg, Mem, Imm)

  //! Insert QWORD based on selector (SSE4.1).
  INST_3i(pinsrq, kInstPinsrq, XmmReg, GpReg, Imm)
  //! \overload
  INST_3i(pinsrq, kInstPinsrq, XmmReg, Mem, Imm)

  //! Packed BYTE maximum (SSE4.1).
  INST_2x(pmaxsb, kInstPmaxsb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaxsb, kInstPmaxsb, XmmReg, Mem)

  //! Packed DWORD maximum (SSE4.1).
  INST_2x(pmaxsd, kInstPmaxsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaxsd, kInstPmaxsd, XmmReg, Mem)

  //! Packed DWORD unsigned maximum (SSE4.1).
  INST_2x(pmaxud, kInstPmaxud, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaxud,kInstPmaxud , XmmReg, Mem)

  //! Packed WORD unsigned maximum (SSE4.1).
  INST_2x(pmaxuw, kInstPmaxuw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmaxuw, kInstPmaxuw, XmmReg, Mem)

  //! Packed BYTE minimum (SSE4.1).
  INST_2x(pminsb, kInstPminsb, XmmReg, XmmReg)
  //! \overload
  INST_2x(pminsb, kInstPminsb, XmmReg, Mem)

  //! Packed DWORD minimum (SSE4.1).
  INST_2x(pminsd, kInstPminsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pminsd, kInstPminsd, XmmReg, Mem)

  //! Packed WORD unsigned minimum (SSE4.1).
  INST_2x(pminuw, kInstPminuw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pminuw, kInstPminuw, XmmReg, Mem)

  //! Packed DWORD unsigned minimum (SSE4.1).
  INST_2x(pminud, kInstPminud, XmmReg, XmmReg)
  //! \overload
  INST_2x(pminud, kInstPminud, XmmReg, Mem)

  //! BYTE to DWORD with sign extend (SSE4.1).
  INST_2x(pmovsxbd, kInstPmovsxbd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovsxbd, kInstPmovsxbd, XmmReg, Mem)

  //! Packed BYTE to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxbq, kInstPmovsxbq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovsxbq, kInstPmovsxbq, XmmReg, Mem)

  //! Packed BYTE to WORD with sign extend (SSE4.1).
  INST_2x(pmovsxbw, kInstPmovsxbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovsxbw, kInstPmovsxbw, XmmReg, Mem)

  //! Packed DWORD to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxdq, kInstPmovsxdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovsxdq, kInstPmovsxdq, XmmReg, Mem)

  //! Packed WORD to DWORD with sign extend (SSE4.1).
  INST_2x(pmovsxwd, kInstPmovsxwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovsxwd, kInstPmovsxwd, XmmReg, Mem)

  //! Packed WORD to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxwq, kInstPmovsxwq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovsxwq, kInstPmovsxwq, XmmReg, Mem)

  //! BYTE to DWORD with zero extend (SSE4.1).
  INST_2x(pmovzxbd, kInstPmovzxbd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovzxbd, kInstPmovzxbd, XmmReg, Mem)

  //! Packed BYTE to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxbq, kInstPmovzxbq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovzxbq, kInstPmovzxbq, XmmReg, Mem)

  //! BYTE to WORD with zero extend (SSE4.1).
  INST_2x(pmovzxbw, kInstPmovzxbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovzxbw, kInstPmovzxbw, XmmReg, Mem)

  //! Packed DWORD to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxdq, kInstPmovzxdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovzxdq, kInstPmovzxdq, XmmReg, Mem)

  //! Packed WORD to DWORD with zero extend (SSE4.1).
  INST_2x(pmovzxwd, kInstPmovzxwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovzxwd, kInstPmovzxwd, XmmReg, Mem)

  //! Packed WORD to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxwq, kInstPmovzxwq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmovzxwq, kInstPmovzxwq, XmmReg, Mem)

  //! Packed DWORD to QWORD multiply (SSE4.1).
  INST_2x(pmuldq, kInstPmuldq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmuldq, kInstPmuldq, XmmReg, Mem)

  //! Packed DWORD multiply low (SSE4.1).
  INST_2x(pmulld, kInstPmulld, XmmReg, XmmReg)
  //! \overload
  INST_2x(pmulld, kInstPmulld, XmmReg, Mem)

  //! Logical compare (SSE4.1).
  INST_2x(ptest, kInstPtest, XmmReg, XmmReg)
  //! \overload
  INST_2x(ptest, kInstPtest, XmmReg, Mem)

  //! Packed DP-FP round (SSE4.1).
  INST_3i(roundpd, kInstRoundpd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(roundpd, kInstRoundpd, XmmReg, Mem, Imm)

  //! Packed SP-FP round (SSE4.1).
  INST_3i(roundps, kInstRoundps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(roundps, kInstRoundps, XmmReg, Mem, Imm)

  //! Scalar DP-FP round (SSE4.1).
  INST_3i(roundsd, kInstRoundsd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(roundsd, kInstRoundsd, XmmReg, Mem, Imm)

  //! Scalar SP-FP round (SSE4.1).
  INST_3i(roundss, kInstRoundss, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(roundss, kInstRoundss, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [SSE4.2]
  // --------------------------------------------------------------------------

  //! Packed compare explicit length strings, return index (SSE4.2).
  INST_3i(pcmpestri, kInstPcmpestri, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pcmpestri, kInstPcmpestri, XmmReg, Mem, Imm)

  //! Packed compare explicit length strings, return mask (SSE4.2).
  INST_3i(pcmpestrm, kInstPcmpestrm, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pcmpestrm, kInstPcmpestrm, XmmReg, Mem, Imm)

  //! Packed compare implicit length strings, return index (SSE4.2).
  INST_3i(pcmpistri, kInstPcmpistri, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pcmpistri, kInstPcmpistri, XmmReg, Mem, Imm)

  //! Packed compare implicit length strings, return mask (SSE4.2).
  INST_3i(pcmpistrm, kInstPcmpistrm, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pcmpistrm, kInstPcmpistrm, XmmReg, Mem, Imm)

  //! Packed QWORD compare if greater than (SSE4.2).
  INST_2x(pcmpgtq, kInstPcmpgtq, XmmReg, XmmReg)
  //! \overload
  INST_2x(pcmpgtq, kInstPcmpgtq, XmmReg, Mem)

  // --------------------------------------------------------------------------
  // [AESNI]
  // --------------------------------------------------------------------------

  //! Perform a single round of the AES decryption flow (AESNI).
  INST_2x(aesdec, kInstAesdec, XmmReg, XmmReg)
  //! \overload
  INST_2x(aesdec, kInstAesdec, XmmReg, Mem)

  //! Perform the last round of the AES decryption flow (AESNI).
  INST_2x(aesdeclast, kInstAesdeclast, XmmReg, XmmReg)
  //! \overload
  INST_2x(aesdeclast, kInstAesdeclast, XmmReg, Mem)

  //! Perform a single round of the AES encryption flow (AESNI).
  INST_2x(aesenc, kInstAesenc, XmmReg, XmmReg)
  //! \overload
  INST_2x(aesenc, kInstAesenc, XmmReg, Mem)

  //! Perform the last round of the AES encryption flow (AESNI).
  INST_2x(aesenclast, kInstAesenclast, XmmReg, XmmReg)
  //! \overload
  INST_2x(aesenclast, kInstAesenclast, XmmReg, Mem)

  //! Perform the InvMixColumns transformation (AESNI).
  INST_2x(aesimc, kInstAesimc, XmmReg, XmmReg)
  //! \overload
  INST_2x(aesimc, kInstAesimc, XmmReg, Mem)

  //! Assist in expanding the AES cipher key (AESNI).
  INST_3i(aeskeygenassist, kInstAeskeygenassist, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(aeskeygenassist, kInstAeskeygenassist, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [PCLMULQDQ]
  // --------------------------------------------------------------------------

  //! Packed QWORD to OWORD carry-less multiply (PCLMULQDQ).
  INST_3i(pclmulqdq, kInstPclmulqdq, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(pclmulqdq, kInstPclmulqdq, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [AVX]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add (AVX).
  INST_3x(vaddpd, kInstVaddpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaddpd, kInstVaddpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vaddpd, kInstVaddpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vaddpd, kInstVaddpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP add (AVX).
  INST_3x(vaddps, kInstVaddps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaddps, kInstVaddps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vaddps, kInstVaddps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vaddps, kInstVaddps, YmmReg, YmmReg, Mem)

  //! Scalar DP-FP add (AVX)
  INST_3x(vaddsd, kInstVaddsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaddsd, kInstVaddsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP add (AVX)
  INST_3x(vaddss, kInstVaddss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaddss, kInstVaddss, XmmReg, XmmReg, Mem)

  //! Packed DP-FP add/subtract (AVX).
  INST_3x(vaddsubpd, kInstVaddsubpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaddsubpd, kInstVaddsubpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vaddsubpd, kInstVaddsubpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vaddsubpd, kInstVaddsubpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP add/subtract (AVX).
  INST_3x(vaddsubps, kInstVaddsubps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaddsubps, kInstVaddsubps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vaddsubps, kInstVaddsubps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vaddsubps, kInstVaddsubps, YmmReg, YmmReg, Mem)

  //! Packed DP-FP bitwise and (AVX).
  INST_3x(vandpd, kInstVandpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vandpd, kInstVandpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vandpd, kInstVandpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vandpd, kInstVandpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP bitwise and (AVX).
  INST_3x(vandps, kInstVandps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vandps, kInstVandps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vandps, kInstVandps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vandps, kInstVandps, YmmReg, YmmReg, Mem)

  //! Packed DP-FP bitwise and-not (AVX).
  INST_3x(vandnpd, kInstVandnpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vandnpd, kInstVandnpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vandnpd, kInstVandnpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vandnpd, kInstVandnpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP bitwise and-not (AVX).
  INST_3x(vandnps, kInstVandnps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vandnps, kInstVandnps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vandnps, kInstVandnps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vandnps, kInstVandnps, YmmReg, YmmReg, Mem)

  //! Packed DP-FP blend (AVX).
  INST_4i(vblendpd, kInstVblendpd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vblendpd, kInstVblendpd, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vblendpd, kInstVblendpd, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vblendpd, kInstVblendpd, YmmReg, YmmReg, Mem, Imm)

  //! Packed SP-FP blend (AVX).
  INST_4i(vblendps, kInstVblendps, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vblendps, kInstVblendps, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vblendps, kInstVblendps, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vblendps, kInstVblendps, YmmReg, YmmReg, Mem, Imm)

  //! Packed DP-FP variable blend (AVX).
  INST_4x(vblendvpd, kInstVblendvpd, XmmReg, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_4x(vblendvpd, kInstVblendvpd, XmmReg, XmmReg, Mem, XmmReg)
  //! \overload
  INST_4x(vblendvpd, kInstVblendvpd, YmmReg, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_4x(vblendvpd, kInstVblendvpd, YmmReg, YmmReg, Mem, YmmReg)

  //! Packed SP-FP variable blend (AVX).
  INST_4x(vblendvps, kInstVblendvps, XmmReg, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_4x(vblendvps, kInstVblendvps, XmmReg, XmmReg, Mem, XmmReg)
  //! \overload
  INST_4x(vblendvps, kInstVblendvps, YmmReg, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_4x(vblendvps, kInstVblendvps, YmmReg, YmmReg, Mem, YmmReg)

  //! Broadcast 128-bits of FP data in `o1` to low and high 128-bits in `o0` (AVX).
  INST_2x(vbroadcastf128, kInstVbroadcastf128, YmmReg, Mem)
  //! Broadcast DP-FP element in `o1` to four locations in `o0` (AVX).
  INST_2x(vbroadcastsd, kInstVbroadcastsd, YmmReg, Mem)
  //! Broadcast SP-FP element in `o1` to four locations in `o0` (AVX).
  INST_2x(vbroadcastss, kInstVbroadcastss, XmmReg, Mem)
  //! Broadcast SP-FP element in `o1` to eight locations in `o0` (AVX).
  INST_2x(vbroadcastss, kInstVbroadcastss, YmmReg, Mem)

  //! Packed DP-FP compare (AVX).
  INST_4i(vcmppd, kInstVcmppd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vcmppd, kInstVcmppd, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vcmppd, kInstVcmppd, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vcmppd, kInstVcmppd, YmmReg, YmmReg, Mem, Imm)

  //! Packed SP-FP compare (AVX).
  INST_4i(vcmpps, kInstVcmpps, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vcmpps, kInstVcmpps, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vcmpps, kInstVcmpps, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vcmpps, kInstVcmpps, YmmReg, YmmReg, Mem, Imm)

  //! Scalar DP-FP compare (AVX).
  INST_4i(vcmpsd, kInstVcmpsd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vcmpsd, kInstVcmpsd, XmmReg, XmmReg, Mem, Imm)

  //! Scalar SP-FP compare (AVX).
  INST_4i(vcmpss, kInstVcmpss, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vcmpss, kInstVcmpss, XmmReg, XmmReg, Mem, Imm)

  //! Scalar DP-FP ordered compare and set EFLAGS (AVX).
  INST_2x(vcomisd, kInstVcomisd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcomisd, kInstVcomisd, XmmReg, Mem)

  //! Scalar SP-FP ordered compare and set EFLAGS (AVX).
  INST_2x(vcomiss, kInstVcomiss, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcomiss, kInstVcomiss, XmmReg, Mem)

  //! Convert packed QWORDs to packed DP-FP (AVX).
  INST_2x(vcvtdq2pd, kInstVcvtdq2pd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtdq2pd, kInstVcvtdq2pd, XmmReg, Mem)
  //! \overload
  INST_2x(vcvtdq2pd, kInstVcvtdq2pd, YmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtdq2pd, kInstVcvtdq2pd, YmmReg, Mem)

  //! Convert packed QWORDs to packed SP-FP (AVX).
  INST_2x(vcvtdq2ps, kInstVcvtdq2ps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtdq2ps, kInstVcvtdq2ps, XmmReg, Mem)
  //! \overload
  INST_2x(vcvtdq2ps, kInstVcvtdq2ps, YmmReg, YmmReg)
  //! \overload
  INST_2x(vcvtdq2ps, kInstVcvtdq2ps, YmmReg, Mem)

  //! Convert packed DP-FP to packed QWORDs (AVX).
  INST_2x(vcvtpd2dq, kInstVcvtpd2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtpd2dq, kInstVcvtpd2dq, XmmReg, YmmReg)
  //! \overload
  INST_2x(vcvtpd2dq, kInstVcvtpd2dq, XmmReg, Mem)

  //! Convert packed DP-FP to packed SP-FP (AVX).
  INST_2x(vcvtpd2ps, kInstVcvtpd2ps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtpd2ps, kInstVcvtpd2ps, XmmReg, YmmReg)
  //! \overload
  INST_2x(vcvtpd2ps, kInstVcvtpd2ps, XmmReg, Mem)

  //! Convert packed SP-FP to packed QWORDs (AVX).
  INST_2x(vcvtps2dq, kInstVcvtps2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtps2dq, kInstVcvtps2dq, XmmReg, Mem)
  //! \overload
  INST_2x(vcvtps2dq, kInstVcvtps2dq, YmmReg, YmmReg)
  //! \overload
  INST_2x(vcvtps2dq, kInstVcvtps2dq, YmmReg, Mem)

  //! Convert packed SP-FP to packed DP-FP (AVX).
  INST_2x(vcvtps2pd, kInstVcvtps2pd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtps2pd, kInstVcvtps2pd, XmmReg, Mem)
  //! \overload
  INST_2x(vcvtps2pd, kInstVcvtps2pd, YmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtps2pd, kInstVcvtps2pd, YmmReg, Mem)

  //! Convert scalar DP-FP to DWORD (AVX).
  INST_2x(vcvtsd2si, kInstVcvtsd2si, GpReg, XmmReg)
  //! \overload
  INST_2x(vcvtsd2si, kInstVcvtsd2si, GpReg, Mem)

  //! Convert scalar DP-FP to scalar SP-FP (AVX).
  INST_3x(vcvtsd2ss, kInstVcvtsd2ss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vcvtsd2ss, kInstVcvtsd2ss, XmmReg, XmmReg, Mem)

  //! Convert DWORD integer to scalar DP-FP (AVX).
  INST_3x(vcvtsi2sd, kInstVcvtsi2sd, XmmReg, XmmReg, GpReg)
  //! \overload
  INST_3x(vcvtsi2sd, kInstVcvtsi2sd, XmmReg, XmmReg, Mem)

  //! Convert scalar INT32 to SP-FP (AVX).
  INST_3x(vcvtsi2ss, kInstVcvtsi2ss, XmmReg, XmmReg, GpReg)
  //! \overload
  INST_3x(vcvtsi2ss, kInstVcvtsi2ss, XmmReg, XmmReg, Mem)

  //! Convert scalar SP-FP to DP-FP (AVX).
  INST_3x(vcvtss2sd, kInstVcvtss2sd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vcvtss2sd, kInstVcvtss2sd, XmmReg, XmmReg, Mem)

  //! Convert scalar SP-FP to INT32 (AVX).
  INST_2x(vcvtss2si, kInstVcvtss2si, GpReg, XmmReg)
  //! \overload
  INST_2x(vcvtss2si, kInstVcvtss2si, GpReg, Mem)

  //! Convert with truncation packed DP-FP to packed QWORDs (AVX).
  INST_2x(vcvttpd2dq, kInstVcvttpd2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvttpd2dq, kInstVcvttpd2dq, XmmReg, YmmReg)
  //! \overload
  INST_2x(vcvttpd2dq, kInstVcvttpd2dq, XmmReg, Mem)

  //! Convert with truncation packed SP-FP to packed QWORDs (AVX).
  INST_2x(vcvttps2dq, kInstVcvttps2dq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvttps2dq, kInstVcvttps2dq, XmmReg, Mem)
  //! \overload
  INST_2x(vcvttps2dq, kInstVcvttps2dq, YmmReg, YmmReg)
  //! \overload
  INST_2x(vcvttps2dq, kInstVcvttps2dq, YmmReg, Mem)

  //! Convert with truncation scalar DP-FP to DWORD (AVX).
  INST_2x(vcvttsd2si, kInstVcvttsd2si, GpReg, XmmReg)
  //! \overload
  INST_2x(vcvttsd2si, kInstVcvttsd2si, GpReg, Mem)

  //! Convert with truncation scalar SP-FP to INT32 (AVX).
  INST_2x(vcvttss2si, kInstVcvttss2si, GpReg, XmmReg)
  //! \overload
  INST_2x(vcvttss2si, kInstVcvttss2si, GpReg, Mem)

  //! Packed DP-FP divide (AVX).
  INST_3x(vdivpd, kInstVdivpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vdivpd, kInstVdivpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vdivpd, kInstVdivpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vdivpd, kInstVdivpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP divide (AVX).
  INST_3x(vdivps, kInstVdivps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vdivps, kInstVdivps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vdivps, kInstVdivps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vdivps, kInstVdivps, YmmReg, YmmReg, Mem)

  //! Scalar DP-FP divide (AVX).
  INST_3x(vdivsd, kInstVdivsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vdivsd, kInstVdivsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP divide (AVX).
  INST_3x(vdivss, kInstVdivss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vdivss, kInstVdivss, XmmReg, XmmReg, Mem)

  //! Packed DP-FP dot product (AVX).
  INST_4i(vdppd, kInstVdppd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vdppd, kInstVdppd, XmmReg, XmmReg, Mem, Imm)

  //! Packed SP-FP dot product (AVX).
  INST_4i(vdpps, kInstVdpps, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vdpps, kInstVdpps, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vdpps, kInstVdpps, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vdpps, kInstVdpps, YmmReg, YmmReg, Mem, Imm)

  //! Extract 128 bits of packed FP data from `o1` and store results in `o0` (AVX).
  INST_3i(vextractf128, kInstVextractf128, XmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vextractf128, kInstVextractf128, Mem, YmmReg, Imm)

  //! Extract SP-FP based on selector (AVX).
  INST_3i(vextractps, kInstVextractps, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(vextractps, kInstVextractps, Mem, XmmReg, Imm)

  //! Packed DP-FP horizontal add (AVX).
  INST_3x(vhaddpd, kInstVhaddpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vhaddpd, kInstVhaddpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vhaddpd, kInstVhaddpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vhaddpd, kInstVhaddpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP horizontal add (AVX).
  INST_3x(vhaddps, kInstVhaddps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vhaddps, kInstVhaddps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vhaddps, kInstVhaddps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vhaddps, kInstVhaddps, YmmReg, YmmReg, Mem)

  //! Packed DP-FP horizontal subtract (AVX).
  INST_3x(vhsubpd, kInstVhsubpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vhsubpd, kInstVhsubpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vhsubpd, kInstVhsubpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vhsubpd, kInstVhsubpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP horizontal subtract (AVX).
  INST_3x(vhsubps, kInstVhsubps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vhsubps, kInstVhsubps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vhsubps, kInstVhsubps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vhsubps, kInstVhsubps, YmmReg, YmmReg, Mem)

  //! Insert 128-bit of packed FP data based on selector (AVX).
  INST_4i(vinsertf128, kInstVinsertf128, YmmReg, YmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vinsertf128, kInstVinsertf128, YmmReg, YmmReg, Mem, Imm)

  //! Insert SP-FP based on selector (AVX).
  INST_4i(vinsertps, kInstVinsertps, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vinsertps, kInstVinsertps, XmmReg, XmmReg, Mem, Imm)

  //! Load 128-bits unaligned (AVX).
  INST_2x(vlddqu, kInstVlddqu, XmmReg, Mem)
  //! Load 256-bits unaligned (AVX).
  INST_2x(vlddqu, kInstVlddqu, YmmReg, Mem)

  //! Load streaming SIMD extension control/status (AVX).
  INST_1x(vldmxcsr, kInstVldmxcsr, Mem)

  //! Store selected bytes of OWORD to DS:EDI/RDI (AVX).
  INST_2x(vmaskmovdqu, kInstVmaskmovdqu, XmmReg, XmmReg)

  //! Conditionally load packed DP-FP from `o2` using mask in `o1 and store in `o0` (AVX).
  INST_3x(vmaskmovpd, kInstVmaskmovpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vmaskmovpd, kInstVmaskmovpd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vmaskmovpd, kInstVmaskmovpd, Mem, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmaskmovpd, kInstVmaskmovpd, Mem, YmmReg, YmmReg)

  //! Conditionally load packed SP-FP from `o2` using mask in `o1 and store in `o0` (AVX).
  INST_3x(vmaskmovps, kInstVmaskmovps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vmaskmovps, kInstVmaskmovps, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vmaskmovps, kInstVmaskmovps, Mem, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmaskmovps, kInstVmaskmovps, Mem, YmmReg, YmmReg)

  //! Packed DP-FP maximum (AVX).
  INST_3x(vmaxpd, kInstVmaxpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmaxpd, kInstVmaxpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vmaxpd, kInstVmaxpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vmaxpd, kInstVmaxpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP maximum (AVX).
  INST_3x(vmaxps, kInstVmaxps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmaxps, kInstVmaxps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vmaxps, kInstVmaxps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vmaxps, kInstVmaxps, YmmReg, YmmReg, Mem)

  //! Scalar DP-FP maximum (AVX).
  INST_3x(vmaxsd, kInstVmaxsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmaxsd, kInstVmaxsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP maximum (AVX).
  INST_3x(vmaxss, kInstVmaxss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmaxss, kInstVmaxss, XmmReg, XmmReg, Mem)

  //! Packed DP-FP minimum (AVX).
  INST_3x(vminpd, kInstVminpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vminpd, kInstVminpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vminpd, kInstVminpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vminpd, kInstVminpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP minimum (AVX).
  INST_3x(vminps, kInstVminps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vminps, kInstVminps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vminps, kInstVminps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vminps, kInstVminps, YmmReg, YmmReg, Mem)

  //! Scalar DP-FP minimum (AVX).
  INST_3x(vminsd, kInstVminsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vminsd, kInstVminsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP minimum (AVX).
  INST_3x(vminss, kInstVminss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vminss, kInstVminss, XmmReg, XmmReg, Mem)

  //! Move 128-bits of aligned packed DP-FP (AVX).
  INST_2x(vmovapd, kInstVmovapd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovapd, kInstVmovapd, XmmReg, Mem)
  //! \overload
  INST_2x(vmovapd, kInstVmovapd, Mem, XmmReg)
  //! Move 256-bits of aligned packed DP-FP (AVX).
  INST_2x(vmovapd, kInstVmovapd, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovapd, kInstVmovapd, YmmReg, Mem)
  //! \overload
  INST_2x(vmovapd, kInstVmovapd, Mem, YmmReg)

  //! Move 128-bits of aligned packed SP-FP (AVX).
  INST_2x(vmovaps, kInstVmovaps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovaps, kInstVmovaps, XmmReg, Mem)
  //! \overload
  INST_2x(vmovaps, kInstVmovaps, Mem, XmmReg)
  //! Move 256-bits of aligned packed SP-FP (AVX).
  INST_2x(vmovaps, kInstVmovaps, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovaps, kInstVmovaps, YmmReg, Mem)
  //! \overload
  INST_2x(vmovaps, kInstVmovaps, Mem, YmmReg)

  //! Move DWORD (AVX).
  INST_2x(vmovd, kInstVmovd, XmmReg, GpReg)
  //! \overload
  INST_2x(vmovd, kInstVmovd, XmmReg, Mem)
  //! \overload
  INST_2x(vmovd, kInstVmovd, GpReg, XmmReg)
  //! \overload
  INST_2x(vmovd, kInstVmovd, Mem, XmmReg)

  //! Move QWORD (AVX).
  INST_2x(vmovq, kInstVmovq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovq, kInstVmovq, XmmReg, Mem)
  //! \overload
  INST_2x(vmovq, kInstVmovq, Mem, XmmReg)

  //! Move one DP-FP and duplicate (AVX).
  INST_2x(vmovddup, kInstVmovddup, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovddup, kInstVmovddup, XmmReg, Mem)
  //! \overload
  INST_2x(vmovddup, kInstVmovddup, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovddup, kInstVmovddup, YmmReg, Mem)

  //! Move 128-bits aligned (AVX).
  INST_2x(vmovdqa, kInstVmovdqa, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovdqa, kInstVmovdqa, XmmReg, Mem)
  //! \overload
  INST_2x(vmovdqa, kInstVmovdqa, Mem, XmmReg)
  //! Move 256-bits aligned (AVX).
  INST_2x(vmovdqa, kInstVmovdqa, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovdqa, kInstVmovdqa, YmmReg, Mem)
  //! \overload
  INST_2x(vmovdqa, kInstVmovdqa, Mem, YmmReg)

  //! Move 128-bits unaligned (AVX).
  INST_2x(vmovdqu, kInstVmovdqu, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovdqu, kInstVmovdqu, XmmReg, Mem)
  //! \overload
  INST_2x(vmovdqu, kInstVmovdqu, Mem, XmmReg)
  //! Move 256-bits unaligned (AVX).
  INST_2x(vmovdqu, kInstVmovdqu, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovdqu, kInstVmovdqu, YmmReg, Mem)
  //! \overload
  INST_2x(vmovdqu, kInstVmovdqu, Mem, YmmReg)

  //! High to low packed SP-FP (AVX).
  INST_3x(vmovhlps, kInstVmovhlps, XmmReg, XmmReg, XmmReg)

  //! Move high packed DP-FP (AVX).
  INST_3x(vmovhpd, kInstVmovhpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_2x(vmovhpd, kInstVmovhpd, Mem, XmmReg)

  //! Move high packed SP-FP (AVX).
  INST_3x(vmovhps, kInstVmovhps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_2x(vmovhps, kInstVmovhps, Mem, XmmReg)

  //! Move low to high packed SP-FP (AVX).
  INST_3x(vmovlhps, kInstVmovlhps, XmmReg, XmmReg, XmmReg)

  //! Move low packed DP-FP (AVX).
  INST_3x(vmovlpd, kInstVmovlpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_2x(vmovlpd, kInstVmovlpd, Mem, XmmReg)

  //! Move low packed SP-FP (AVX).
  INST_3x(vmovlps, kInstVmovlps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_2x(vmovlps, kInstVmovlps, Mem, XmmReg)

  //! Extract packed DP-FP sign mask (AVX).
  INST_2x(vmovmskpd, kInstVmovmskpd, GpReg, XmmReg)
  //! \overload
  INST_2x(vmovmskpd, kInstVmovmskpd, GpReg, YmmReg)

  //! Extract packed SP-FP sign mask (AVX).
  INST_2x(vmovmskps, kInstVmovmskps, GpReg, XmmReg)
  //! \overload
  INST_2x(vmovmskps, kInstVmovmskps, GpReg, YmmReg)

  //! Store 128-bits using NT hint (AVX).
  INST_2x(vmovntdq, kInstVmovntdq, Mem, XmmReg)
  //! Store 256-bits using NT hint (AVX).
  INST_2x(vmovntdq, kInstVmovntdq, Mem, YmmReg)

  //! Store 128-bits aligned using NT hint (AVX).
  INST_2x(vmovntdqa, kInstVmovntdqa, XmmReg, Mem)

  //! Store packed DP-FP (128-bits) using NT hint (AVX).
  INST_2x(vmovntpd, kInstVmovntpd, Mem, XmmReg)
  //! Store packed DP-FP (256-bits) using NT hint (AVX).
  INST_2x(vmovntpd, kInstVmovntpd, Mem, YmmReg)

  //! Store packed SP-FP (128-bits) using NT hint (AVX).
  INST_2x(vmovntps, kInstVmovntps, Mem, XmmReg)
  //! Store packed SP-FP (256-bits) using NT hint (AVX).
  INST_2x(vmovntps, kInstVmovntps, Mem, YmmReg)

  //! Move scalar DP-FP (AVX).
  INST_3x(vmovsd, kInstVmovsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovsd, kInstVmovsd, XmmReg, Mem)
  //! \overload
  INST_2x(vmovsd, kInstVmovsd, Mem, XmmReg)

  //! Move packed SP-FP high and duplicate (AVX).
  INST_2x(vmovshdup, kInstVmovshdup, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovshdup, kInstVmovshdup, XmmReg, Mem)
  //! \overload
  INST_2x(vmovshdup, kInstVmovshdup, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovshdup, kInstVmovshdup, YmmReg, Mem)

  //! Move packed SP-FP low and duplicate (AVX).
  INST_2x(vmovsldup, kInstVmovsldup, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovsldup, kInstVmovsldup, XmmReg, Mem)
  //! \overload
  INST_2x(vmovsldup, kInstVmovsldup, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovsldup, kInstVmovsldup, YmmReg, Mem)

  //! Move scalar SP-FP (AVX).
  INST_3x(vmovss, kInstVmovss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovss, kInstVmovss, XmmReg, Mem)
  //! \overload
  INST_2x(vmovss, kInstVmovss, Mem, XmmReg)

  //! Move 128-bits of unaligned packed DP-FP (AVX).
  INST_2x(vmovupd, kInstVmovupd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovupd, kInstVmovupd, XmmReg, Mem)
  //! \overload
  INST_2x(vmovupd, kInstVmovupd, Mem, XmmReg)
  //! Move 256-bits of unaligned packed DP-FP (AVX).
  INST_2x(vmovupd, kInstVmovupd, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovupd, kInstVmovupd, YmmReg, Mem)
  //! \overload
  INST_2x(vmovupd, kInstVmovupd, Mem, YmmReg)

  //! Move 128-bits of unaligned packed SP-FP (AVX).
  INST_2x(vmovups, kInstVmovups, XmmReg, XmmReg)
  //! \overload
  INST_2x(vmovups, kInstVmovups, XmmReg, Mem)
  //! \overload
  INST_2x(vmovups, kInstVmovups, Mem, XmmReg)
  //! Move 256-bits of unaligned packed SP-FP (AVX).
  INST_2x(vmovups, kInstVmovups, YmmReg, YmmReg)
  //! \overload
  INST_2x(vmovups, kInstVmovups, YmmReg, Mem)
  //! \overload
  INST_2x(vmovups, kInstVmovups, Mem, YmmReg)

  //! Packed WORD sums of absolute difference (AVX).
  INST_4i(vmpsadbw, kInstVmpsadbw, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vmpsadbw, kInstVmpsadbw, XmmReg, XmmReg, Mem, Imm)

  //! Packed DP-FP multiply (AVX).
  INST_3x(vmulpd, kInstVmulpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmulpd, kInstVmulpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vmulpd, kInstVmulpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vmulpd, kInstVmulpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP multiply (AVX).
  INST_3x(vmulps, kInstVmulps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmulps, kInstVmulps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vmulps, kInstVmulps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vmulps, kInstVmulps, YmmReg, YmmReg, Mem)

  //! Packed SP-FP multiply (AVX).
  INST_3x(vmulsd, kInstVmulsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmulsd, kInstVmulsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP multiply (AVX).
  INST_3x(vmulss, kInstVmulss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vmulss, kInstVmulss, XmmReg, XmmReg, Mem)

  //! Packed DP-FP bitwise or (AVX).
  INST_3x(vorpd, kInstVorpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vorpd, kInstVorpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vorpd, kInstVorpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vorpd, kInstVorpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP bitwise or (AVX).
  INST_3x(vorps, kInstVorps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vorps, kInstVorps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vorps, kInstVorps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vorps, kInstVorps, YmmReg, YmmReg, Mem)

  //! Packed BYTE absolute value (AVX).
  INST_2x(vpabsb, kInstVpabsb, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpabsb, kInstVpabsb, XmmReg, Mem)

  //! Packed DWORD absolute value (AVX).
  INST_2x(vpabsd, kInstVpabsd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpabsd, kInstVpabsd, XmmReg, Mem)

  //! Packed WORD absolute value (AVX).
  INST_2x(vpabsw, kInstVpabsw, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpabsw, kInstVpabsw, XmmReg, Mem)

  //! Pack DWORDs to WORDs with signed saturation (AVX).
  INST_3x(vpackssdw, kInstVpackssdw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpackssdw, kInstVpackssdw, XmmReg, XmmReg, Mem)

  //! Pack WORDs to BYTEs with signed saturation (AVX).
  INST_3x(vpacksswb, kInstVpacksswb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpacksswb, kInstVpacksswb, XmmReg, XmmReg, Mem)

  //! Pack DWORDs to WORDs with unsigned saturation (AVX).
  INST_3x(vpackusdw, kInstVpackusdw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpackusdw, kInstVpackusdw, XmmReg, XmmReg, Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (AVX).
  INST_3x(vpackuswb, kInstVpackuswb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpackuswb, kInstVpackuswb, XmmReg, XmmReg, Mem)

  //! Packed BYTE add (AVX).
  INST_3x(vpaddb, kInstVpaddb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddb, kInstVpaddb, XmmReg, XmmReg, Mem)

  //! Packed DWORD add (AVX).
  INST_3x(vpaddd, kInstVpaddd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddd, kInstVpaddd, XmmReg, XmmReg, Mem)

  //! Packed QWORD add (AVX).
  INST_3x(vpaddq, kInstVpaddq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddq, kInstVpaddq, XmmReg, XmmReg, Mem)

  //! Packed WORD add (AVX).
  INST_3x(vpaddw, kInstVpaddw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddw, kInstVpaddw, XmmReg, XmmReg, Mem)

  //! Packed BYTE add with saturation (AVX).
  INST_3x(vpaddsb, kInstVpaddsb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddsb, kInstVpaddsb, XmmReg, XmmReg, Mem)

  //! Packed WORD add with saturation (AVX).
  INST_3x(vpaddsw, kInstVpaddsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddsw, kInstVpaddsw, XmmReg, XmmReg, Mem)

  //! Packed BYTE add with unsigned saturation (AVX).
  INST_3x(vpaddusb, kInstVpaddusb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddusb, kInstVpaddusb, XmmReg, XmmReg, Mem)

  //! Packed WORD add with unsigned saturation (AVX).
  INST_3x(vpaddusw, kInstVpaddusw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpaddusw, kInstVpaddusw, XmmReg, XmmReg, Mem)

  //! Packed align right (AVX).
  INST_4i(vpalignr, kInstVpalignr, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vpalignr, kInstVpalignr, XmmReg, XmmReg, Mem, Imm)

  //! Packed bitwise and (AVX).
  INST_3x(vpand, kInstVpand, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpand, kInstVpand, XmmReg, XmmReg, Mem)

  //! Packed bitwise and-not (AVX).
  INST_3x(vpandn, kInstVpandn, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpandn, kInstVpandn, XmmReg, XmmReg, Mem)

  //! Packed BYTE average (AVX).
  INST_3x(vpavgb, kInstVpavgb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpavgb, kInstVpavgb, XmmReg, XmmReg, Mem)

  //! Packed WORD average (AVX).
  INST_3x(vpavgw, kInstVpavgw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpavgw, kInstVpavgw, XmmReg, XmmReg, Mem)

  //! Packed BYTE variable blend (AVX).
  INST_4x(vpblendvb, kInstVpblendvb, XmmReg, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_4x(vpblendvb, kInstVpblendvb, XmmReg, XmmReg, Mem, XmmReg)

  //! Packed WORD blend (AVX).
  INST_4i(vpblendw, kInstVpblendw, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vpblendw, kInstVpblendw, XmmReg, XmmReg, Mem, Imm)

  //! Packed BYTEs compare for equality (AVX).
  INST_3x(vpcmpeqb, kInstVpcmpeqb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpeqb, kInstVpcmpeqb, XmmReg, XmmReg, Mem)

  //! Packed DWORDs compare for equality (AVX).
  INST_3x(vpcmpeqd, kInstVpcmpeqd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpeqd, kInstVpcmpeqd, XmmReg, XmmReg, Mem)

  //! Packed QWORDs compare for equality (AVX).
  INST_3x(vpcmpeqq, kInstVpcmpeqq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpeqq, kInstVpcmpeqq, XmmReg, XmmReg, Mem)

  //! Packed WORDs compare for equality (AVX).
  INST_3x(vpcmpeqw, kInstVpcmpeqw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpeqw, kInstVpcmpeqw, XmmReg, XmmReg, Mem)

  //! Packed BYTEs compare if greater than (AVX).
  INST_3x(vpcmpgtb, kInstVpcmpgtb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpgtb, kInstVpcmpgtb, XmmReg, XmmReg, Mem)

  //! Packed DWORDs compare if greater than (AVX).
  INST_3x(vpcmpgtd, kInstVpcmpgtd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpgtd, kInstVpcmpgtd, XmmReg, XmmReg, Mem)

  //! Packed QWORDs compare if greater than (AVX).
  INST_3x(vpcmpgtq, kInstVpcmpgtq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpgtq, kInstVpcmpgtq, XmmReg, XmmReg, Mem)

  //! Packed WORDs compare if greater than (AVX).
  INST_3x(vpcmpgtw, kInstVpcmpgtw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpcmpgtw, kInstVpcmpgtw, XmmReg, XmmReg, Mem)

  //! Packed compare explicit length strings, return index (AVX).
  INST_3i(vpcmpestri, kInstVpcmpestri, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpestri, kInstVpcmpestri, XmmReg, Mem, Imm)

  //! Packed compare explicit length strings, return mask (AVX).
  INST_3i(vpcmpestrm, kInstVpcmpestrm, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpestrm, kInstVpcmpestrm, XmmReg, Mem, Imm)

  //! Packed compare implicit length strings, return index (AVX).
  INST_3i(vpcmpistri, kInstVpcmpistri, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpistri, kInstVpcmpistri, XmmReg, Mem, Imm)

  //! Packed compare implicit length strings, return mask (AVX).
  INST_3i(vpcmpistrm, kInstVpcmpistrm, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpistrm, kInstVpcmpistrm, XmmReg, Mem, Imm)

  //! Packed DP-FP permute (AVX).
  INST_3x(vpermilpd, kInstVpermilpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpermilpd, kInstVpermilpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vpermilpd, kInstVpermilpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpermilpd, kInstVpermilpd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3i(vpermilpd, kInstVpermilpd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpermilpd, kInstVpermilpd, XmmReg, Mem, Imm)
  //! \overload
  INST_3i(vpermilpd, kInstVpermilpd, YmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vpermilpd, kInstVpermilpd, YmmReg, Mem, Imm)

  //! Packed SP-FP permute (AVX).
  INST_3x(vpermilps, kInstVpermilps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpermilps, kInstVpermilps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vpermilps, kInstVpermilps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpermilps, kInstVpermilps, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3i(vpermilps, kInstVpermilps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpermilps, kInstVpermilps, XmmReg, Mem, Imm)
  //! \overload
  INST_3i(vpermilps, kInstVpermilps, YmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vpermilps, kInstVpermilps, YmmReg, Mem, Imm)

  //! Packed 128-bit FP permute (AVX).
  INST_4i(vperm2f128, kInstVperm2f128, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vperm2f128, kInstVperm2f128, YmmReg, YmmReg, Mem, Imm)

  //! Extract BYTE (AVX).
  INST_3i(vpextrb, kInstVpextrb, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpextrb, kInstVpextrb, Mem, XmmReg, Imm)

  //! Extract DWORD (AVX).
  INST_3i(vpextrd, kInstVpextrd, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpextrd, kInstVpextrd, Mem, XmmReg, Imm)

  //! Extract WORD (AVX).
  INST_3i(vpextrw, kInstVpextrw, GpReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpextrw, kInstVpextrw, Mem, XmmReg, Imm)

  //! Packed DWORD horizontal add (AVX).
  INST_3x(vphaddd, kInstVphaddd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vphaddd, kInstVphaddd, XmmReg, XmmReg, Mem)

  //! Packed WORD horizontal add with saturation (AVX).
  INST_3x(vphaddsw, kInstVphaddsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vphaddsw, kInstVphaddsw, XmmReg, XmmReg, Mem)

  //! Packed WORD horizontal add (AVX).
  INST_3x(vphaddw, kInstVphaddw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vphaddw, kInstVphaddw, XmmReg, XmmReg, Mem)

  //! Packed WORD horizontal minimum (AVX).
  INST_2x(vphminposuw, kInstVphminposuw, XmmReg, XmmReg)
  //! \overload
  INST_2x(vphminposuw, kInstVphminposuw, XmmReg, Mem)

  //! Packed DWORD horizontal subtract (AVX).
  INST_3x(vphsubd, kInstVphsubd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vphsubd, kInstVphsubd, XmmReg, XmmReg, Mem)

  //! Packed WORD horizontal subtract with saturation (AVX).
  INST_3x(vphsubsw, kInstVphsubsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vphsubsw, kInstVphsubsw, XmmReg, XmmReg, Mem)

  //! Packed WORD horizontal subtract (AVX).
  INST_3x(vphsubw, kInstVphsubw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vphsubw, kInstVphsubw, XmmReg, XmmReg, Mem)

  //! Insert BYTE based on selector (AVX).
  INST_4i(vpinsrb, kInstVpinsrb, XmmReg, XmmReg, GpReg, Imm)
  //! \overload
  INST_4i(vpinsrb, kInstVpinsrb, XmmReg, XmmReg, Mem, Imm)

  //! Insert DWORD based on selector (AVX).
  INST_4i(vpinsrd, kInstVpinsrd, XmmReg, XmmReg, GpReg, Imm)
  //! \overload
  INST_4i(vpinsrd, kInstVpinsrd, XmmReg, XmmReg, Mem, Imm)

  //! Insert WORD based on selector (AVX).
  INST_4i(vpinsrw, kInstVpinsrw, XmmReg, XmmReg, GpReg, Imm)
  //! \overload
  INST_4i(vpinsrw, kInstVpinsrw, XmmReg, XmmReg, Mem, Imm)

  //! Packed multiply and add signed and unsigned bytes (AVX).
  INST_3x(vpmaddubsw, kInstVpmaddubsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaddubsw, kInstVpmaddubsw, XmmReg, XmmReg, Mem)

  //! Packed WORD multiply and add to packed DWORD (AVX).
  INST_3x(vpmaddwd, kInstVpmaddwd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaddwd, kInstVpmaddwd, XmmReg, XmmReg, Mem)

  //! Packed BYTE maximum (AVX).
  INST_3x(vpmaxsb, kInstVpmaxsb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaxsb, kInstVpmaxsb, XmmReg, XmmReg, Mem)

  //! Packed DWORD maximum (AVX).
  INST_3x(vpmaxsd, kInstVpmaxsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaxsd, kInstVpmaxsd, XmmReg, XmmReg, Mem)

  //! Packed WORD maximum (AVX).
  INST_3x(vpmaxsw, kInstVpmaxsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaxsw, kInstVpmaxsw, XmmReg, XmmReg, Mem)

  //! Packed BYTE unsigned maximum (AVX).
  INST_3x(vpmaxub, kInstVpmaxub, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaxub, kInstVpmaxub, XmmReg, XmmReg, Mem)

  //! Packed DWORD unsigned maximum (AVX).
  INST_3x(vpmaxud, kInstVpmaxud, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaxud, kInstVpmaxud, XmmReg, XmmReg, Mem)

  //! Packed WORD unsigned maximum (AVX).
  INST_3x(vpmaxuw, kInstVpmaxuw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmaxuw, kInstVpmaxuw, XmmReg, XmmReg, Mem)

  //! Packed BYTE minimum (AVX).
  INST_3x(vpminsb, kInstVpminsb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpminsb, kInstVpminsb, XmmReg, XmmReg, Mem)

  //! Packed DWORD minimum (AVX).
  INST_3x(vpminsd, kInstVpminsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpminsd, kInstVpminsd, XmmReg, XmmReg, Mem)

  //! Packed WORD minimum (AVX).
  INST_3x(vpminsw, kInstVpminsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpminsw, kInstVpminsw, XmmReg, XmmReg, Mem)

  //! Packed BYTE unsigned minimum (AVX).
  INST_3x(vpminub, kInstVpminub, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpminub, kInstVpminub, XmmReg, XmmReg, Mem)

  //! Packed DWORD unsigned minimum (AVX).
  INST_3x(vpminud, kInstVpminud, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpminud, kInstVpminud, XmmReg, XmmReg, Mem)

  //! Packed WORD unsigned minimum (AVX).
  INST_3x(vpminuw, kInstVpminuw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpminuw, kInstVpminuw, XmmReg, XmmReg, Mem)

  //! Move Byte mask to integer (AVX).
  INST_2x(vpmovmskb, kInstVpmovmskb, GpReg, XmmReg)

  //! BYTE to DWORD with sign extend (AVX).
  INST_2x(vpmovsxbd, kInstVpmovsxbd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovsxbd, kInstVpmovsxbd, XmmReg, Mem)

  //! Packed BYTE to QWORD with sign extend (AVX).
  INST_2x(vpmovsxbq, kInstVpmovsxbq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovsxbq, kInstVpmovsxbq, XmmReg, Mem)

  //! Packed BYTE to WORD with sign extend (AVX).
  INST_2x(vpmovsxbw, kInstVpmovsxbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovsxbw, kInstVpmovsxbw, XmmReg, Mem)

  //! Packed DWORD to QWORD with sign extend (AVX).
  INST_2x(vpmovsxdq, kInstVpmovsxdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovsxdq, kInstVpmovsxdq, XmmReg, Mem)

  //! Packed WORD to DWORD with sign extend (AVX).
  INST_2x(vpmovsxwd, kInstVpmovsxwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovsxwd, kInstVpmovsxwd, XmmReg, Mem)

  //! Packed WORD to QWORD with sign extend (AVX).
  INST_2x(vpmovsxwq, kInstVpmovsxwq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovsxwq, kInstVpmovsxwq, XmmReg, Mem)

  //! BYTE to DWORD with zero extend (AVX).
  INST_2x(vpmovzxbd, kInstVpmovzxbd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovzxbd, kInstVpmovzxbd, XmmReg, Mem)

  //! Packed BYTE to QWORD with zero extend (AVX).
  INST_2x(vpmovzxbq, kInstVpmovzxbq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovzxbq, kInstVpmovzxbq, XmmReg, Mem)

  //! BYTE to WORD with zero extend (AVX).
  INST_2x(vpmovzxbw, kInstVpmovzxbw, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovzxbw, kInstVpmovzxbw, XmmReg, Mem)

  //! Packed DWORD to QWORD with zero extend (AVX).
  INST_2x(vpmovzxdq, kInstVpmovzxdq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovzxdq, kInstVpmovzxdq, XmmReg, Mem)

  //! Packed WORD to DWORD with zero extend (AVX).
  INST_2x(vpmovzxwd, kInstVpmovzxwd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovzxwd, kInstVpmovzxwd, XmmReg, Mem)

  //! Packed WORD to QWORD with zero extend (AVX).
  INST_2x(vpmovzxwq, kInstVpmovzxwq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpmovzxwq, kInstVpmovzxwq, XmmReg, Mem)

  //! Packed DWORD to QWORD multiply (AVX).
  INST_3x(vpmuldq, kInstVpmuldq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmuldq, kInstVpmuldq, XmmReg, XmmReg, Mem)

  //! Packed WORD multiply high, round and scale (AVX).
  INST_3x(vpmulhrsw, kInstVpmulhrsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmulhrsw, kInstVpmulhrsw, XmmReg, XmmReg, Mem)

  //! Packed WORD unsigned multiply high (AVX).
  INST_3x(vpmulhuw, kInstVpmulhuw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmulhuw, kInstVpmulhuw, XmmReg, XmmReg, Mem)

  //! Packed WORD multiply high (AVX).
  INST_3x(vpmulhw, kInstVpmulhw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmulhw, kInstVpmulhw, XmmReg, XmmReg, Mem)

  //! Packed DWORD multiply low (AVX).
  INST_3x(vpmulld, kInstVpmulld, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmulld, kInstVpmulld, XmmReg, XmmReg, Mem)

  //! Packed WORDs multiply low (AVX).
  INST_3x(vpmullw, kInstVpmullw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmullw, kInstVpmullw, XmmReg, XmmReg, Mem)

  //! Packed DWORD multiply to QWORD (AVX).
  INST_3x(vpmuludq, kInstVpmuludq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpmuludq, kInstVpmuludq, XmmReg, XmmReg, Mem)

  //! Packed bitwise or (AVX).
  INST_3x(vpor, kInstVpor, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpor, kInstVpor, XmmReg, XmmReg, Mem)

  //! Packed WORD sum of absolute differences (AVX).
  INST_3x(vpsadbw, kInstVpsadbw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsadbw, kInstVpsadbw, XmmReg, XmmReg, Mem)

  //! Packed BYTE shuffle (AVX).
  INST_3x(vpshufb, kInstVpshufb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpshufb, kInstVpshufb, XmmReg, XmmReg, Mem)

  //! Packed DWORD shuffle (AVX).
  INST_3i(vpshufd, kInstVpshufd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpshufd, kInstVpshufd, XmmReg, Mem, Imm)

  //! Packed WORD shuffle high (AVX).
  INST_3i(vpshufhw, kInstVpshufhw, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpshufhw, kInstVpshufhw, XmmReg, Mem, Imm)

  //! Packed WORD shuffle low (AVX).
  INST_3i(vpshuflw, kInstVpshuflw, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vpshuflw, kInstVpshuflw, XmmReg, Mem, Imm)

  //! Packed BYTE sign (AVX).
  INST_3x(vpsignb, kInstVpsignb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsignb, kInstVpsignb, XmmReg, XmmReg, Mem)

  //! Packed DWORD sign (AVX).
  INST_3x(vpsignd, kInstVpsignd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsignd, kInstVpsignd, XmmReg, XmmReg, Mem)

  //! Packed WORD sign (AVX).
  INST_3x(vpsignw, kInstVpsignw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsignw, kInstVpsignw, XmmReg, XmmReg, Mem)

  //! Packed DWORD shift left logical (AVX).
  INST_3x(vpslld, kInstVpslld, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpslld, kInstVpslld, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpslld, kInstVpslld, XmmReg, XmmReg, Imm)

  //! Packed OWORD shift left logical (AVX).
  INST_3i(vpslldq, kInstVpslldq, XmmReg, XmmReg, Imm)

  //! Packed QWORD shift left logical (AVX).
  INST_3x(vpsllq, kInstVpsllq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsllq, kInstVpsllq, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsllq, kInstVpsllq, XmmReg, XmmReg, Imm)

  //! Packed WORD shift left logical (AVX).
  INST_3x(vpsllw, kInstVpsllw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsllw, kInstVpsllw, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsllw, kInstVpsllw, XmmReg, XmmReg, Imm)

  //! Packed DWORD shift right arithmetic (AVX).
  INST_3x(vpsrad, kInstVpsrad, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsrad, kInstVpsrad, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsrad, kInstVpsrad, XmmReg, XmmReg, Imm)

  //! Packed WORD shift right arithmetic (AVX).
  INST_3x(vpsraw, kInstVpsraw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsraw, kInstVpsraw, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsraw, kInstVpsraw, XmmReg, XmmReg, Imm)

  //! Packed DWORD shift right logical (AVX).
  INST_3x(vpsrld, kInstVpsrld, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsrld, kInstVpsrld, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsrld, kInstVpsrld, XmmReg, XmmReg, Imm)

  //! Scalar OWORD shift right logical (AVX).
  INST_3i(vpsrldq, kInstVpsrldq, XmmReg, XmmReg, Imm)

  //! Packed QWORD shift right logical (AVX).
  INST_3x(vpsrlq, kInstVpsrlq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsrlq, kInstVpsrlq, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsrlq, kInstVpsrlq, XmmReg, XmmReg, Imm)

  //! Packed WORD shift right logical (AVX).
  INST_3x(vpsrlw, kInstVpsrlw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsrlw, kInstVpsrlw, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3i(vpsrlw, kInstVpsrlw, XmmReg, XmmReg, Imm)

  //! Packed BYTE subtract (AVX).
  INST_3x(vpsubb, kInstVpsubb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubb, kInstVpsubb, XmmReg, XmmReg, Mem)

  //! Packed DWORD subtract (AVX).
  INST_3x(vpsubd, kInstVpsubd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubd, kInstVpsubd, XmmReg, XmmReg, Mem)

  //! Packed QWORD subtract (AVX).
  INST_3x(vpsubq, kInstVpsubq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubq, kInstVpsubq, XmmReg, XmmReg, Mem)

  //! Packed WORD subtract (AVX).
  INST_3x(vpsubw, kInstVpsubw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubw, kInstVpsubw, XmmReg, XmmReg, Mem)

  //! Packed BYTE subtract with saturation (AVX).
  INST_3x(vpsubsb, kInstVpsubsb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubsb, kInstVpsubsb, XmmReg, XmmReg, Mem)

  //! Packed WORD subtract with saturation (AVX).
  INST_3x(vpsubsw, kInstVpsubsw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubsw, kInstVpsubsw, XmmReg, XmmReg, Mem)

  //! Packed BYTE subtract with unsigned saturation (AVX).
  INST_3x(vpsubusb, kInstVpsubusb, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubusb, kInstVpsubusb, XmmReg, XmmReg, Mem)

  //! Packed WORD subtract with unsigned saturation (AVX).
  INST_3x(vpsubusw, kInstVpsubusw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpsubusw, kInstVpsubusw, XmmReg, XmmReg, Mem)

  //! Logical compare (AVX).
  INST_2x(vptest, kInstVptest, XmmReg, XmmReg)
  //! \overload
  INST_2x(vptest, kInstVptest, XmmReg, Mem)
  //! \overload
  INST_2x(vptest, kInstVptest, YmmReg, YmmReg)
  //! \overload
  INST_2x(vptest, kInstVptest, YmmReg, Mem)

  //! Unpack high packed BYTEs to WORDs (AVX).
  INST_3x(vpunpckhbw, kInstVpunpckhbw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpckhbw, kInstVpunpckhbw, XmmReg, XmmReg, Mem)

  //! Unpack high packed DWORDs to QWORDs (AVX).
  INST_3x(vpunpckhdq, kInstVpunpckhdq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpckhdq, kInstVpunpckhdq, XmmReg, XmmReg, Mem)

  //! Unpack high packed QWORDs to OWORD (AVX).
  INST_3x(vpunpckhqdq, kInstVpunpckhqdq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpckhqdq, kInstVpunpckhqdq, XmmReg, XmmReg, Mem)

  //! Unpack high packed WORDs to DWORDs (AVX).
  INST_3x(vpunpckhwd, kInstVpunpckhwd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpckhwd, kInstVpunpckhwd, XmmReg, XmmReg, Mem)

  //! Unpack low packed BYTEs to WORDs (AVX).
  INST_3x(vpunpcklbw, kInstVpunpcklbw, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpcklbw, kInstVpunpcklbw, XmmReg, XmmReg, Mem)

  //! Unpack low packed DWORDs to QWORDs (AVX).
  INST_3x(vpunpckldq, kInstVpunpckldq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpckldq, kInstVpunpckldq, XmmReg, XmmReg, Mem)

  //! Unpack low packed QWORDs to OWORD (AVX).
  INST_3x(vpunpcklqdq, kInstVpunpcklqdq, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpcklqdq, kInstVpunpcklqdq, XmmReg, XmmReg, Mem)

  //! Unpack low packed WORDs to DWORDs (AVX).
  INST_3x(vpunpcklwd, kInstVpunpcklwd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpunpcklwd, kInstVpunpcklwd, XmmReg, XmmReg, Mem)

  //! Packed bitwise xor (AVX).
  INST_3x(vpxor, kInstVpxor, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vpxor, kInstVpxor, XmmReg, XmmReg, Mem)

  //! Packed SP-FP reciprocal (AVX).
  INST_2x(vrcpps, kInstVrcpps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vrcpps, kInstVrcpps, XmmReg, Mem)
  //! \overload
  INST_2x(vrcpps, kInstVrcpps, YmmReg, YmmReg)
  //! \overload
  INST_2x(vrcpps, kInstVrcpps, YmmReg, Mem)

  //! Scalar SP-FP reciprocal (AVX).
  INST_3x(vrcpss, kInstVrcpss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vrcpss, kInstVrcpss, XmmReg, XmmReg, Mem)

  //! Packed SP-FP square root reciprocal (AVX).
  INST_2x(vrsqrtps, kInstVrsqrtps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vrsqrtps, kInstVrsqrtps, XmmReg, Mem)
  //! \overload
  INST_2x(vrsqrtps, kInstVrsqrtps, YmmReg, YmmReg)
  //! \overload
  INST_2x(vrsqrtps, kInstVrsqrtps, YmmReg, Mem)

  //! Scalar SP-FP square root reciprocal (AVX).
  INST_3x(vrsqrtss, kInstVrsqrtss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vrsqrtss, kInstVrsqrtss, XmmReg, XmmReg, Mem)

  //! Packed DP-FP round (AVX).
  INST_3i(vroundpd, kInstVroundpd, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vroundpd, kInstVroundpd, XmmReg, Mem, Imm)
  //! \overload
  INST_3i(vroundpd, kInstVroundpd, YmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vroundpd, kInstVroundpd, YmmReg, Mem, Imm)

  //! Packed SP-FP round (AVX).
  INST_3i(vroundps, kInstVroundps, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vroundps, kInstVroundps, XmmReg, Mem, Imm)
  //! \overload
  INST_3i(vroundps, kInstVroundps, YmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vroundps, kInstVroundps, YmmReg, Mem, Imm)

  //! Scalar DP-FP round (AVX).
  INST_4i(vroundsd, kInstVroundsd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vroundsd, kInstVroundsd, XmmReg, XmmReg, Mem, Imm)

  //! Scalar SP-FP round (AVX).
  INST_4i(vroundss, kInstVroundss, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vroundss, kInstVroundss, XmmReg, XmmReg, Mem, Imm)

  //! Shuffle DP-FP (AVX).
  INST_4i(vshufpd, kInstVshufpd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vshufpd, kInstVshufpd, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vshufpd, kInstVshufpd, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vshufpd, kInstVshufpd, YmmReg, YmmReg, Mem, Imm)

  //! Shuffle SP-FP (AVX).
  INST_4i(vshufps, kInstVshufps, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vshufps, kInstVshufps, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vshufps, kInstVshufps, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vshufps, kInstVshufps, YmmReg, YmmReg, Mem, Imm)

  //! Packed DP-FP square root (AVX).
  INST_2x(vsqrtpd, kInstVsqrtpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vsqrtpd, kInstVsqrtpd, XmmReg, Mem)
  //! \overload
  INST_2x(vsqrtpd, kInstVsqrtpd, YmmReg, YmmReg)
  //! \overload
  INST_2x(vsqrtpd, kInstVsqrtpd, YmmReg, Mem)

  //! Packed SP-FP square root (AVX).
  INST_2x(vsqrtps, kInstVsqrtps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vsqrtps, kInstVsqrtps, XmmReg, Mem)
  //! \overload
  INST_2x(vsqrtps, kInstVsqrtps, YmmReg, YmmReg)
  //! \overload
  INST_2x(vsqrtps, kInstVsqrtps, YmmReg, Mem)

  //! Scalar DP-FP square root (AVX).
  INST_3x(vsqrtsd, kInstVsqrtsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vsqrtsd, kInstVsqrtsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP square root (AVX).
  INST_3x(vsqrtss, kInstVsqrtss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vsqrtss, kInstVsqrtss, XmmReg, XmmReg, Mem)

  //! Store streaming SIMD extension control/status (AVX).
  INST_1x(vstmxcsr, kInstVstmxcsr, Mem)

  //! Packed DP-FP subtract (AVX).
  INST_3x(vsubpd, kInstVsubpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vsubpd, kInstVsubpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vsubpd, kInstVsubpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vsubpd, kInstVsubpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP subtract (AVX).
  INST_3x(vsubps, kInstVsubps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vsubps, kInstVsubps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vsubps, kInstVsubps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vsubps, kInstVsubps, YmmReg, YmmReg, Mem)

  //! Scalar DP-FP subtract (AVX).
  INST_3x(vsubsd, kInstVsubsd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vsubsd, kInstVsubsd, XmmReg, XmmReg, Mem)

  //! Scalar SP-FP subtract (AVX).
  INST_3x(vsubss, kInstVsubss, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vsubss, kInstVsubss, XmmReg, XmmReg, Mem)

  //! Logical compare DP-FP (AVX).
  INST_2x(vtestpd, kInstVtestpd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vtestpd, kInstVtestpd, XmmReg, Mem)
  //! \overload
  INST_2x(vtestpd, kInstVtestpd, YmmReg, YmmReg)
  //! \overload
  INST_2x(vtestpd, kInstVtestpd, YmmReg, Mem)

  //! Logical compare SP-FP (AVX).
  INST_2x(vtestps, kInstVtestps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vtestps, kInstVtestps, XmmReg, Mem)
  //! \overload
  INST_2x(vtestps, kInstVtestps, YmmReg, YmmReg)
  //! \overload
  INST_2x(vtestps, kInstVtestps, YmmReg, Mem)

  //! Scalar DP-FP unordered compare and set EFLAGS (AVX).
  INST_2x(vucomisd, kInstVucomisd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vucomisd, kInstVucomisd, XmmReg, Mem)

  //! Unordered scalar SP-FP compare and set EFLAGS (AVX).
  INST_2x(vucomiss, kInstVucomiss, XmmReg, XmmReg)
  //! \overload
  INST_2x(vucomiss, kInstVucomiss, XmmReg, Mem)

  //! Unpack and interleave high packed DP-FP (AVX).
  INST_3x(vunpckhpd, kInstVunpckhpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vunpckhpd, kInstVunpckhpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vunpckhpd, kInstVunpckhpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vunpckhpd, kInstVunpckhpd, YmmReg, YmmReg, Mem)

  //! Unpack high packed SP-FP data (AVX).
  INST_3x(vunpckhps, kInstVunpckhps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vunpckhps, kInstVunpckhps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vunpckhps, kInstVunpckhps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vunpckhps, kInstVunpckhps, YmmReg, YmmReg, Mem)

  //! Unpack and interleave low packed DP-FP (AVX).
  INST_3x(vunpcklpd, kInstVunpcklpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vunpcklpd, kInstVunpcklpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vunpcklpd, kInstVunpcklpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vunpcklpd, kInstVunpcklpd, YmmReg, YmmReg, Mem)

  //! Unpack low packed SP-FP data (AVX).
  INST_3x(vunpcklps, kInstVunpcklps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vunpcklps, kInstVunpcklps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vunpcklps, kInstVunpcklps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vunpcklps, kInstVunpcklps, YmmReg, YmmReg, Mem)

  //! Packed DP-FP bitwise xor (AVX).
  INST_3x(vxorpd, kInstVxorpd, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vxorpd, kInstVxorpd, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vxorpd, kInstVxorpd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vxorpd, kInstVxorpd, YmmReg, YmmReg, Mem)

  //! Packed SP-FP bitwise xor (AVX).
  INST_3x(vxorps, kInstVxorps, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vxorps, kInstVxorps, XmmReg, XmmReg, Mem)
  //! \overload
  INST_3x(vxorps, kInstVxorps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vxorps, kInstVxorps, YmmReg, YmmReg, Mem)

  //! Zero all Ymm registers.
  INST_0x(vzeroall, kInstVzeroall)
  //! Zero upper 128-bits of all Ymm registers.
  INST_0x(vzeroupper, kInstVzeroupper)

  // --------------------------------------------------------------------------
  // [AVX+AESNI]
  // --------------------------------------------------------------------------

  //! Perform a single round of the AES decryption flow (AVX+AESNI).
  INST_3x(vaesdec, kInstVaesdec, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaesdec, kInstVaesdec, XmmReg, XmmReg, Mem)

  //! Perform the last round of the AES decryption flow (AVX+AESNI).
  INST_3x(vaesdeclast, kInstVaesdeclast, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaesdeclast, kInstVaesdeclast, XmmReg, XmmReg, Mem)

  //! Perform a single round of the AES encryption flow (AVX+AESNI).
  INST_3x(vaesenc, kInstVaesenc, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaesenc, kInstVaesenc, XmmReg, XmmReg, Mem)

  //! Perform the last round of the AES encryption flow (AVX+AESNI).
  INST_3x(vaesenclast, kInstVaesenclast, XmmReg, XmmReg, XmmReg)
  //! \overload
  INST_3x(vaesenclast, kInstVaesenclast, XmmReg, XmmReg, Mem)

  //! Perform the InvMixColumns transformation (AVX+AESNI).
  INST_2x(vaesimc, kInstVaesimc, XmmReg, XmmReg)
  //! \overload
  INST_2x(vaesimc, kInstVaesimc, XmmReg, Mem)

  //! Assist in expanding the AES cipher key (AVX+AESNI).
  INST_3i(vaeskeygenassist, kInstVaeskeygenassist, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vaeskeygenassist, kInstVaeskeygenassist, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [AVX+PCLMULQDQ]
  // --------------------------------------------------------------------------

  //! Carry-less multiplication QWORD (AVX+PCLMULQDQ).
  INST_4i(vpclmulqdq, kInstVpclmulqdq, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vpclmulqdq, kInstVpclmulqdq, XmmReg, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [AVX2]
  // --------------------------------------------------------------------------

  //! Broadcast low 128-bit element in `o1` to `o0` (AVX2).
  INST_2x(vbroadcasti128, kInstVbroadcasti128, YmmReg, Mem)
  //! Broadcast low DP-FP element in `o1` to `o0` (AVX2).
  INST_2x(vbroadcastsd, kInstVbroadcastsd, YmmReg, XmmReg)
  //! Broadcast low SP-FP element in `o1` to `o0` (AVX2).
  INST_2x(vbroadcastss, kInstVbroadcastss, XmmReg, XmmReg)
  //! \overload
  INST_2x(vbroadcastss, kInstVbroadcastss, YmmReg, XmmReg)

  //! Extract 128-bit element from `o1` to `o0` based on selector (AVX2).
  INST_3i(vextracti128, kInstVextracti128, XmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vextracti128, kInstVextracti128, Mem, YmmReg, Imm)

  //! Gather DP-FP from DWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherdpd, kInstVgatherdpd, XmmReg, Mem, XmmReg)
  //! \overload
  INST_3x(vgatherdpd, kInstVgatherdpd, YmmReg, Mem, YmmReg)

  //! Gather SP-FP from DWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherdps, kInstVgatherdps, XmmReg, Mem, XmmReg)
  //! \overload
  INST_3x(vgatherdps, kInstVgatherdps, YmmReg, Mem, YmmReg)

  //! Gather DP-FP from QWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherqpd, kInstVgatherqpd, XmmReg, Mem, XmmReg)
  //! \overload
  INST_3x(vgatherqpd, kInstVgatherqpd, YmmReg, Mem, YmmReg)

  //! Gather SP-FP from QWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherqps, kInstVgatherqps, XmmReg, Mem, XmmReg)

  //! Insert 128-bit of packed data based on selector (AVX2).
  INST_4i(vinserti128, kInstVinserti128, YmmReg, YmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vinserti128, kInstVinserti128, YmmReg, YmmReg, Mem, Imm)

  //! Load 256-bits aligned using NT hint (AVX2).
  INST_2x(vmovntdqa, kInstVmovntdqa, YmmReg, Mem)

  //! Packed WORD sums of absolute difference (AVX2).
  INST_4i(vmpsadbw, kInstVmpsadbw, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vmpsadbw, kInstVmpsadbw, YmmReg, YmmReg, Mem, Imm)

  //! Packed BYTE absolute value (AVX2).
  INST_2x(vpabsb, kInstVpabsb, YmmReg, YmmReg)
  //! \overload
  INST_2x(vpabsb, kInstVpabsb, YmmReg, Mem)

  //! Packed DWORD absolute value (AVX2).
  INST_2x(vpabsd, kInstVpabsd, YmmReg, YmmReg)
  //! \overload
  INST_2x(vpabsd, kInstVpabsd, YmmReg, Mem)

  //! Packed WORD absolute value (AVX2).
  INST_2x(vpabsw, kInstVpabsw, YmmReg, YmmReg)
  //! \overload
  INST_2x(vpabsw, kInstVpabsw, YmmReg, Mem)

  //! Pack DWORDs to WORDs with signed saturation (AVX2).
  INST_3x(vpackssdw, kInstVpackssdw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpackssdw, kInstVpackssdw, YmmReg, YmmReg, Mem)

  //! Pack WORDs to BYTEs with signed saturation (AVX2).
  INST_3x(vpacksswb, kInstVpacksswb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpacksswb, kInstVpacksswb, YmmReg, YmmReg, Mem)

  //! Pack DWORDs to WORDs with unsigned saturation (AVX2).
  INST_3x(vpackusdw, kInstVpackusdw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpackusdw, kInstVpackusdw, YmmReg, YmmReg, Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (AVX2).
  INST_3x(vpackuswb, kInstVpackuswb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpackuswb, kInstVpackuswb, YmmReg, YmmReg, Mem)

  //! Packed BYTE add (AVX2).
  INST_3x(vpaddb, kInstVpaddb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddb, kInstVpaddb, YmmReg, YmmReg, Mem)

  //! Packed DWORD add (AVX2).
  INST_3x(vpaddd, kInstVpaddd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddd, kInstVpaddd, YmmReg, YmmReg, Mem)

  //! Packed QDWORD add (AVX2).
  INST_3x(vpaddq, kInstVpaddq, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddq, kInstVpaddq, YmmReg, YmmReg, Mem)

  //! Packed WORD add (AVX2).
  INST_3x(vpaddw, kInstVpaddw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddw, kInstVpaddw, YmmReg, YmmReg, Mem)

  //! Packed BYTE add with saturation (AVX2).
  INST_3x(vpaddsb, kInstVpaddsb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddsb, kInstVpaddsb, YmmReg, YmmReg, Mem)

  //! Packed WORD add with saturation (AVX2).
  INST_3x(vpaddsw, kInstVpaddsw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddsw, kInstVpaddsw, YmmReg, YmmReg, Mem)

  //! Packed BYTE add with unsigned saturation (AVX2).
  INST_3x(vpaddusb, kInstVpaddusb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddusb, kInstVpaddusb, YmmReg, YmmReg, Mem)

  //! Packed WORD add with unsigned saturation (AVX2).
  INST_3x(vpaddusw, kInstVpaddusw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpaddusw, kInstVpaddusw, YmmReg, YmmReg, Mem)

  //! Packed align right (AVX2).
  INST_4i(vpalignr, kInstVpalignr, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vpalignr, kInstVpalignr, YmmReg, YmmReg, Mem, Imm)

  //! Packed bitwise and (AVX2).
  INST_3x(vpand, kInstVpand, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpand, kInstVpand, YmmReg, YmmReg, Mem)

  //! Packed bitwise and-not (AVX2).
  INST_3x(vpandn, kInstVpandn, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpandn, kInstVpandn, YmmReg, YmmReg, Mem)

  //! Packed BYTE average (AVX2).
  INST_3x(vpavgb, kInstVpavgb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpavgb, kInstVpavgb, YmmReg, YmmReg, Mem)

  //! Packed WORD average (AVX2).
  INST_3x(vpavgw, kInstVpavgw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpavgw, kInstVpavgw, YmmReg, YmmReg, Mem)

  //! Packed DWORD blend (AVX2).
  INST_4i(vpblendd, kInstVpblendd, XmmReg, XmmReg, XmmReg, Imm)
  //! \overload
  INST_4i(vpblendd, kInstVpblendd, XmmReg, XmmReg, Mem, Imm)
  //! \overload
  INST_4i(vpblendd, kInstVpblendd, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vpblendd, kInstVpblendd, YmmReg, YmmReg, Mem, Imm)

  //! Packed DWORD variable blend (AVX2).
  INST_4x(vpblendvb, kInstVpblendvb, YmmReg, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_4x(vpblendvb, kInstVpblendvb, YmmReg, YmmReg, Mem, YmmReg)

  //! Packed WORD blend (AVX2).
  INST_4i(vpblendw, kInstVpblendw, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vpblendw, kInstVpblendw, YmmReg, YmmReg, Mem, Imm)

  //! Broadcast BYTE from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastb, kInstVpbroadcastb, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastb, kInstVpbroadcastb, XmmReg, Mem)
  //! Broadcast BYTE from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastb, kInstVpbroadcastb, YmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastb, kInstVpbroadcastb, YmmReg, Mem)

  //! Broadcast DWORD from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastd, kInstVpbroadcastd, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastd, kInstVpbroadcastd, XmmReg, Mem)
  //! Broadcast DWORD from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastd, kInstVpbroadcastd, YmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastd, kInstVpbroadcastd, YmmReg, Mem)

  //! Broadcast QWORD from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastq, kInstVpbroadcastq, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastq, kInstVpbroadcastq, XmmReg, Mem)
  //! Broadcast QWORD from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastq, kInstVpbroadcastq, YmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastq, kInstVpbroadcastq, YmmReg, Mem)

  //! Broadcast WORD from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastw, kInstVpbroadcastw, XmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastw, kInstVpbroadcastw, XmmReg, Mem)
  //! Broadcast WORD from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastw, kInstVpbroadcastw, YmmReg, XmmReg)
  //! \overload
  INST_2x(vpbroadcastw, kInstVpbroadcastw, YmmReg, Mem)

  //! Packed BYTEs compare for equality (AVX2).
  INST_3x(vpcmpeqb, kInstVpcmpeqb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpeqb, kInstVpcmpeqb, YmmReg, YmmReg, Mem)

  //! Packed DWORDs compare for equality (AVX2).
  INST_3x(vpcmpeqd, kInstVpcmpeqd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpeqd, kInstVpcmpeqd, YmmReg, YmmReg, Mem)

  //! Packed QWORDs compare for equality (AVX2).
  INST_3x(vpcmpeqq, kInstVpcmpeqq, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpeqq, kInstVpcmpeqq, YmmReg, YmmReg, Mem)

  //! Packed WORDs compare for equality (AVX2).
  INST_3x(vpcmpeqw, kInstVpcmpeqw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpeqw, kInstVpcmpeqw, YmmReg, YmmReg, Mem)

  //! Packed BYTEs compare if greater than (AVX2).
  INST_3x(vpcmpgtb, kInstVpcmpgtb, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpgtb, kInstVpcmpgtb, YmmReg, YmmReg, Mem)

  //! Packed DWORDs compare if greater than (AVX2).
  INST_3x(vpcmpgtd, kInstVpcmpgtd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpgtd, kInstVpcmpgtd, YmmReg, YmmReg, Mem)

  //! Packed QWORDs compare if greater than (AVX2).
  INST_3x(vpcmpgtq, kInstVpcmpgtq, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpgtq, kInstVpcmpgtq, YmmReg, YmmReg, Mem)

  //! Packed WORDs compare if greater than (AVX2).
  INST_3x(vpcmpgtw, kInstVpcmpgtw, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpcmpgtw, kInstVpcmpgtw, YmmReg, YmmReg, Mem)

  //! Packed OWORD permute (AVX2).
  INST_4i(vperm2i128, kInstVperm2i128, YmmReg, YmmReg, YmmReg, Imm)
  //! \overload
  INST_4i(vperm2i128, kInstVperm2i128, YmmReg, YmmReg, Mem, Imm)

  //! Packed DWORD permute (AVX2).
  INST_3x(vpermd, kInstVpermd, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpermd, kInstVpermd, YmmReg, YmmReg, Mem)

  //! Packed DP-FP permute (AVX2).
  INST_3i(vpermpd, kInstVpermpd, YmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vpermpd, kInstVpermpd, YmmReg, Mem, Imm)

  //! Packed SP-FP permute (AVX2).
  INST_3x(vpermps, kInstVpermps, YmmReg, YmmReg, YmmReg)
  //! \overload
  INST_3x(vpermps, kInstVpermps, YmmReg, YmmReg, Mem)

  //! Packed QWORD permute (AVX2).
  INST_3i(vpermq, kInstVpermq, YmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vpermq, kInstVpermq, YmmReg, Mem, Imm)

  INST_3x(vpgatherdd, kInstVpgatherdd, XmmReg, Mem, XmmReg)
  INST_3x(vpgatherdd, kInstVpgatherdd, YmmReg, Mem, YmmReg)

  INST_3x(vpgatherdq, kInstVpgatherdq, XmmReg, Mem, XmmReg)
  INST_3x(vpgatherdq, kInstVpgatherdq, YmmReg, Mem, YmmReg)

  INST_3x(vpgatherqd, kInstVpgatherqd, XmmReg, Mem, XmmReg)

  INST_3x(vpgatherqq, kInstVpgatherqq, XmmReg, Mem, XmmReg)
  INST_3x(vpgatherqq, kInstVpgatherqq, YmmReg, Mem, YmmReg)

  //! Packed DWORD horizontal add (AVX2).
  INST_3x(vphaddd, kInstVphaddd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vphaddd, kInstVphaddd, YmmReg, YmmReg, YmmReg)

  //! Packed WORD horizontal add with saturation (AVX2).
  INST_3x(vphaddsw, kInstVphaddsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vphaddsw, kInstVphaddsw, YmmReg, YmmReg, YmmReg)

  //! Packed WORD horizontal add (AVX2).
  INST_3x(vphaddw, kInstVphaddw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vphaddw, kInstVphaddw, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD horizontal subtract (AVX2).
  INST_3x(vphsubd, kInstVphsubd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vphsubd, kInstVphsubd, YmmReg, YmmReg, YmmReg)

  //! Packed WORD horizontal subtract with saturation (AVX2).
  INST_3x(vphsubsw, kInstVphsubsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vphsubsw, kInstVphsubsw, YmmReg, YmmReg, YmmReg)

  //! Packed WORD horizontal subtract (AVX2).
  INST_3x(vphsubw, kInstVphsubw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vphsubw, kInstVphsubw, YmmReg, YmmReg, YmmReg)

  //! Move Byte mask to integer (AVX2).
  INST_2x(vpmovmskb, kInstVpmovmskb, GpReg, YmmReg)

  //! BYTE to DWORD with sign extend (AVX).
  INST_2x(vpmovsxbd, kInstVpmovsxbd, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovsxbd, kInstVpmovsxbd, YmmReg, XmmReg)

  //! Packed BYTE to QWORD with sign extend (AVX2).
  INST_2x(vpmovsxbq, kInstVpmovsxbq, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovsxbq, kInstVpmovsxbq, YmmReg, XmmReg)

  //! Packed BYTE to WORD with sign extend (AVX2).
  INST_2x(vpmovsxbw, kInstVpmovsxbw, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovsxbw, kInstVpmovsxbw, YmmReg, XmmReg)

  //! Packed DWORD to QWORD with sign extend (AVX2).
  INST_2x(vpmovsxdq, kInstVpmovsxdq, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovsxdq, kInstVpmovsxdq, YmmReg, XmmReg)

  //! Packed WORD to DWORD with sign extend (AVX2).
  INST_2x(vpmovsxwd, kInstVpmovsxwd, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovsxwd, kInstVpmovsxwd, YmmReg, XmmReg)

  //! Packed WORD to QWORD with sign extend (AVX2).
  INST_2x(vpmovsxwq, kInstVpmovsxwq, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovsxwq, kInstVpmovsxwq, YmmReg, XmmReg)

  //! BYTE to DWORD with zero extend (AVX2).
  INST_2x(vpmovzxbd, kInstVpmovzxbd, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovzxbd, kInstVpmovzxbd, YmmReg, XmmReg)

  //! Packed BYTE to QWORD with zero extend (AVX2).
  INST_2x(vpmovzxbq, kInstVpmovzxbq, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovzxbq, kInstVpmovzxbq, YmmReg, XmmReg)

  //! BYTE to WORD with zero extend (AVX2).
  INST_2x(vpmovzxbw, kInstVpmovzxbw, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovzxbw, kInstVpmovzxbw, YmmReg, XmmReg)

  //! Packed DWORD to QWORD with zero extend (AVX2).
  INST_2x(vpmovzxdq, kInstVpmovzxdq, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovzxdq, kInstVpmovzxdq, YmmReg, XmmReg)

  //! Packed WORD to DWORD with zero extend (AVX2).
  INST_2x(vpmovzxwd, kInstVpmovzxwd, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovzxwd, kInstVpmovzxwd, YmmReg, XmmReg)

  //! Packed WORD to QWORD with zero extend (AVX2).
  INST_2x(vpmovzxwq, kInstVpmovzxwq, YmmReg, Mem)
  //! \overload
  INST_2x(vpmovzxwq, kInstVpmovzxwq, YmmReg, XmmReg)

  //! Packed multiply and add signed and unsigned bytes (AVX2).
  INST_3x(vpmaddubsw, kInstVpmaddubsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaddubsw, kInstVpmaddubsw, YmmReg, YmmReg, YmmReg)

  //! Packed WORD multiply and add to packed DWORD (AVX2).
  INST_3x(vpmaddwd, kInstVpmaddwd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaddwd, kInstVpmaddwd, YmmReg, YmmReg, YmmReg)

  INST_3x(vpmaskmovd, kInstVpmaskmovd, Mem, XmmReg, XmmReg)
  INST_3x(vpmaskmovd, kInstVpmaskmovd, Mem, YmmReg, YmmReg)
  INST_3x(vpmaskmovd, kInstVpmaskmovd, XmmReg, XmmReg, Mem)
  INST_3x(vpmaskmovd, kInstVpmaskmovd, YmmReg, YmmReg, Mem)

  INST_3x(vpmaskmovq, kInstVpmaskmovq, Mem, XmmReg, XmmReg)
  INST_3x(vpmaskmovq, kInstVpmaskmovq, Mem, YmmReg, YmmReg)
  INST_3x(vpmaskmovq, kInstVpmaskmovq, XmmReg, XmmReg, Mem)
  INST_3x(vpmaskmovq, kInstVpmaskmovq, YmmReg, YmmReg, Mem)

  //! Packed BYTE maximum (AVX2).
  INST_3x(vpmaxsb, kInstVpmaxsb, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaxsb, kInstVpmaxsb, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD maximum (AVX2).
  INST_3x(vpmaxsd, kInstVpmaxsd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaxsd, kInstVpmaxsd, YmmReg, YmmReg, YmmReg)

  //! Packed WORD maximum (AVX2).
  INST_3x(vpmaxsw, kInstVpmaxsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaxsw, kInstVpmaxsw, YmmReg, YmmReg, YmmReg)

  //! Packed BYTE unsigned maximum (AVX2).
  INST_3x(vpmaxub, kInstVpmaxub, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaxub, kInstVpmaxub, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD unsigned maximum (AVX2).
  INST_3x(vpmaxud, kInstVpmaxud, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaxud, kInstVpmaxud, YmmReg, YmmReg, YmmReg)

  //! Packed WORD unsigned maximum (AVX2).
  INST_3x(vpmaxuw, kInstVpmaxuw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmaxuw, kInstVpmaxuw, YmmReg, YmmReg, YmmReg)

  //! Packed BYTE minimum (AVX2).
  INST_3x(vpminsb, kInstVpminsb, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpminsb, kInstVpminsb, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD minimum (AVX2).
  INST_3x(vpminsd, kInstVpminsd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpminsd, kInstVpminsd, YmmReg, YmmReg, YmmReg)

  //! Packed WORD minimum (AVX2).
  INST_3x(vpminsw, kInstVpminsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpminsw, kInstVpminsw, YmmReg, YmmReg, YmmReg)

  //! Packed BYTE unsigned minimum (AVX2).
  INST_3x(vpminub, kInstVpminub, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpminub, kInstVpminub, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD unsigned minimum (AVX2).
  INST_3x(vpminud, kInstVpminud, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpminud, kInstVpminud, YmmReg, YmmReg, YmmReg)

  //! Packed WORD unsigned minimum (AVX2).
  INST_3x(vpminuw, kInstVpminuw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpminuw, kInstVpminuw, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD to QWORD multiply (AVX2).
  INST_3x(vpmuldq, kInstVpmuldq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmuldq, kInstVpmuldq, YmmReg, YmmReg, YmmReg)

  //! Packed WORD multiply high, round and scale (AVX2).
  INST_3x(vpmulhrsw, kInstVpmulhrsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmulhrsw, kInstVpmulhrsw, YmmReg, YmmReg, YmmReg)

  //! Packed WORD unsigned multiply high (AVX2).
  INST_3x(vpmulhuw, kInstVpmulhuw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmulhuw, kInstVpmulhuw, YmmReg, YmmReg, YmmReg)

  //! Packed WORD multiply high (AVX2).
  INST_3x(vpmulhw, kInstVpmulhw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmulhw, kInstVpmulhw, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD multiply low (AVX2).
  INST_3x(vpmulld, kInstVpmulld, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmulld, kInstVpmulld, YmmReg, YmmReg, YmmReg)

  //! Packed WORDs multiply low (AVX2).
  INST_3x(vpmullw, kInstVpmullw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmullw, kInstVpmullw, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD multiply to QWORD (AVX2).
  INST_3x(vpmuludq, kInstVpmuludq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpmuludq, kInstVpmuludq, YmmReg, YmmReg, YmmReg)

  //! Packed bitwise or (AVX2).
  INST_3x(vpor, kInstVpor, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpor, kInstVpor, YmmReg, YmmReg, YmmReg)

  //! Packed WORD sum of absolute differences (AVX2).
  INST_3x(vpsadbw, kInstVpsadbw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsadbw, kInstVpsadbw, YmmReg, YmmReg, YmmReg)

  //! Packed BYTE shuffle (AVX2).
  INST_3x(vpshufb, kInstVpshufb, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpshufb, kInstVpshufb, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD shuffle (AVX2).
  INST_3i(vpshufd, kInstVpshufd, YmmReg, Mem, Imm)
  //! \overload
  INST_3i(vpshufd, kInstVpshufd, YmmReg, YmmReg, Imm)

  //! Packed WORD shuffle high (AVX2).
  INST_3i(vpshufhw, kInstVpshufhw, YmmReg, Mem, Imm)
  //! \overload
  INST_3i(vpshufhw, kInstVpshufhw, YmmReg, YmmReg, Imm)

  //! Packed WORD shuffle low (AVX2).
  INST_3i(vpshuflw, kInstVpshuflw, YmmReg, Mem, Imm)
  //! \overload
  INST_3i(vpshuflw, kInstVpshuflw, YmmReg, YmmReg, Imm)

  //! Packed BYTE sign (AVX2).
  INST_3x(vpsignb, kInstVpsignb, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsignb, kInstVpsignb, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD sign (AVX2).
  INST_3x(vpsignd, kInstVpsignd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsignd, kInstVpsignd, YmmReg, YmmReg, YmmReg)

  //! Packed WORD sign (AVX2).
  INST_3x(vpsignw, kInstVpsignw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsignw, kInstVpsignw, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD shift left logical (AVX2).
  INST_3x(vpslld, kInstVpslld, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpslld, kInstVpslld, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpslld, kInstVpslld, YmmReg, YmmReg, Imm)

  //! Packed OWORD shift left logical (AVX2).
  INST_3i(vpslldq, kInstVpslldq, YmmReg, YmmReg, Imm)

  //! Packed QWORD shift left logical (AVX2).
  INST_3x(vpsllq, kInstVpsllq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsllq, kInstVpsllq, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpsllq, kInstVpsllq, YmmReg, YmmReg, Imm)

  INST_3x(vpsllvd, kInstVpsllvd, XmmReg, XmmReg, Mem)
  INST_3x(vpsllvd, kInstVpsllvd, XmmReg, XmmReg, XmmReg)
  INST_3x(vpsllvd, kInstVpsllvd, YmmReg, YmmReg, Mem)
  INST_3x(vpsllvd, kInstVpsllvd, YmmReg, YmmReg, YmmReg)

  INST_3x(vpsllvq, kInstVpsllvq, XmmReg, XmmReg, Mem)
  INST_3x(vpsllvq, kInstVpsllvq, XmmReg, XmmReg, XmmReg)
  INST_3x(vpsllvq, kInstVpsllvq, YmmReg, YmmReg, Mem)
  INST_3x(vpsllvq, kInstVpsllvq, YmmReg, YmmReg, YmmReg)

  //! Packed WORD shift left logical (AVX2).
  INST_3x(vpsllw, kInstVpsllw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsllw, kInstVpsllw, YmmReg, YmmReg, XmmReg)
  //! Packed WORD shift left logical (AVX2).
  INST_3i(vpsllw, kInstVpsllw, YmmReg, YmmReg, Imm)

  //! Packed DWORD shift right arithmetic (AVX2).
  INST_3x(vpsrad, kInstVpsrad, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsrad, kInstVpsrad, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpsrad, kInstVpsrad, YmmReg, YmmReg, Imm)

  INST_3x(vpsravd, kInstVpsravd, XmmReg, XmmReg, Mem)
  INST_3x(vpsravd, kInstVpsravd, XmmReg, XmmReg, XmmReg)
  INST_3x(vpsravd, kInstVpsravd, YmmReg, YmmReg, Mem)
  INST_3x(vpsravd, kInstVpsravd, YmmReg, YmmReg, YmmReg)

  //! Packed WORD shift right arithmetic (AVX2).
  INST_3x(vpsraw, kInstVpsraw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsraw, kInstVpsraw, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpsraw, kInstVpsraw, YmmReg, YmmReg, Imm)

  //! Packed DWORD shift right logical (AVX2).
  INST_3x(vpsrld, kInstVpsrld, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsrld, kInstVpsrld, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpsrld, kInstVpsrld, YmmReg, YmmReg, Imm)

  //! Scalar OWORD shift right logical (AVX2).
  INST_3i(vpsrldq, kInstVpsrldq, YmmReg, YmmReg, Imm)

  //! Packed QWORD shift right logical (AVX2).
  INST_3x(vpsrlq, kInstVpsrlq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsrlq, kInstVpsrlq, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpsrlq, kInstVpsrlq, YmmReg, YmmReg, Imm)

  INST_3x(vpsrlvd, kInstVpsrlvd, XmmReg, XmmReg, Mem)
  INST_3x(vpsrlvd, kInstVpsrlvd, XmmReg, XmmReg, XmmReg)
  INST_3x(vpsrlvd, kInstVpsrlvd, YmmReg, YmmReg, Mem)
  INST_3x(vpsrlvd, kInstVpsrlvd, YmmReg, YmmReg, YmmReg)

  INST_3x(vpsrlvq, kInstVpsrlvq, XmmReg, XmmReg, Mem)
  INST_3x(vpsrlvq, kInstVpsrlvq, XmmReg, XmmReg, XmmReg)
  INST_3x(vpsrlvq, kInstVpsrlvq, YmmReg, YmmReg, Mem)
  INST_3x(vpsrlvq, kInstVpsrlvq, YmmReg, YmmReg, YmmReg)

  //! Packed WORD shift right logical (AVX2).
  INST_3x(vpsrlw, kInstVpsrlw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsrlw, kInstVpsrlw, YmmReg, YmmReg, XmmReg)
  //! \overload
  INST_3i(vpsrlw, kInstVpsrlw, YmmReg, YmmReg, Imm)

  //! Packed BYTE subtract (AVX2).
  INST_3x(vpsubb, kInstVpsubb, YmmReg, YmmReg, Mem)
  INST_3x(vpsubb, kInstVpsubb, YmmReg, YmmReg, YmmReg)

  //! Packed DWORD subtract (AVX2).
  INST_3x(vpsubd, kInstVpsubd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubd, kInstVpsubd, YmmReg, YmmReg, YmmReg)

  //! Packed QWORD subtract (AVX2).
  INST_3x(vpsubq, kInstVpsubq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubq, kInstVpsubq, YmmReg, YmmReg, YmmReg)

  //! Packed BYTE subtract with saturation (AVX2).
  INST_3x(vpsubsb, kInstVpsubsb, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubsb, kInstVpsubsb, YmmReg, YmmReg, YmmReg)

  //! Packed WORD subtract with saturation (AVX2).
  INST_3x(vpsubsw, kInstVpsubsw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubsw, kInstVpsubsw, YmmReg, YmmReg, YmmReg)

  //! Packed BYTE subtract with unsigned saturation (AVX2).
  INST_3x(vpsubusb, kInstVpsubusb, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubusb, kInstVpsubusb, YmmReg, YmmReg, YmmReg)

  //! Packed WORD subtract with unsigned saturation (AVX2).
  INST_3x(vpsubusw, kInstVpsubusw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubusw, kInstVpsubusw, YmmReg, YmmReg, YmmReg)

  //! Packed WORD subtract (AVX2).
  INST_3x(vpsubw, kInstVpsubw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpsubw, kInstVpsubw, YmmReg, YmmReg, YmmReg)

  //! Unpack high packed BYTEs to WORDs (AVX2).
  INST_3x(vpunpckhbw, kInstVpunpckhbw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpckhbw, kInstVpunpckhbw, YmmReg, YmmReg, YmmReg)

  //! Unpack high packed DWORDs to QWORDs (AVX2).
  INST_3x(vpunpckhdq, kInstVpunpckhdq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpckhdq, kInstVpunpckhdq, YmmReg, YmmReg, YmmReg)

  //! Unpack high packed QWORDs to OWORD (AVX2).
  INST_3x(vpunpckhqdq, kInstVpunpckhqdq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpckhqdq, kInstVpunpckhqdq, YmmReg, YmmReg, YmmReg)

  //! Unpack high packed WORDs to DWORDs (AVX2).
  INST_3x(vpunpckhwd, kInstVpunpckhwd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpckhwd, kInstVpunpckhwd, YmmReg, YmmReg, YmmReg)

  //! Unpack low packed BYTEs to WORDs (AVX2).
  INST_3x(vpunpcklbw, kInstVpunpcklbw, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpcklbw, kInstVpunpcklbw, YmmReg, YmmReg, YmmReg)

  //! Unpack low packed DWORDs to QWORDs (AVX2).
  INST_3x(vpunpckldq, kInstVpunpckldq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpckldq, kInstVpunpckldq, YmmReg, YmmReg, YmmReg)

  //! Unpack low packed QWORDs to OWORD (AVX2).
  INST_3x(vpunpcklqdq, kInstVpunpcklqdq, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpcklqdq, kInstVpunpcklqdq, YmmReg, YmmReg, YmmReg)

  //! Unpack low packed WORDs to DWORDs (AVX2).
  INST_3x(vpunpcklwd, kInstVpunpcklwd, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpunpcklwd, kInstVpunpcklwd, YmmReg, YmmReg, YmmReg)

  //! Packed bitwise xor (AVX2).
  INST_3x(vpxor, kInstVpxor, YmmReg, YmmReg, Mem)
  //! \overload
  INST_3x(vpxor, kInstVpxor, YmmReg, YmmReg, YmmReg)

  // --------------------------------------------------------------------------
  // [FMA3]
  // --------------------------------------------------------------------------

  INST_3x(vfmadd132pd, kInstVfmadd132pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd132pd, kInstVfmadd132pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmadd132pd, kInstVfmadd132pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmadd132pd, kInstVfmadd132pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmadd132ps, kInstVfmadd132ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd132ps, kInstVfmadd132ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmadd132ps, kInstVfmadd132ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmadd132ps, kInstVfmadd132ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmadd132sd, kInstVfmadd132sd, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd132sd, kInstVfmadd132sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmadd132ss, kInstVfmadd132ss, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd132ss, kInstVfmadd132ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmadd213pd, kInstVfmadd213pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd213pd, kInstVfmadd213pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmadd213pd, kInstVfmadd213pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmadd213pd, kInstVfmadd213pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmadd213ps, kInstVfmadd213ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd213ps, kInstVfmadd213ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmadd213ps, kInstVfmadd213ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmadd213ps, kInstVfmadd213ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmadd213sd, kInstVfmadd213sd, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd213sd, kInstVfmadd213sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmadd213ss, kInstVfmadd213ss, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd213ss, kInstVfmadd213ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmadd231pd, kInstVfmadd231pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd231pd, kInstVfmadd231pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmadd231pd, kInstVfmadd231pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmadd231pd, kInstVfmadd231pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmadd231ps, kInstVfmadd231ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd231ps, kInstVfmadd231ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmadd231ps, kInstVfmadd231ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmadd231ps, kInstVfmadd231ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmadd231sd, kInstVfmadd231sd, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd231sd, kInstVfmadd231sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmadd231ss, kInstVfmadd231ss, XmmReg, XmmReg, Mem)
  INST_3x(vfmadd231ss, kInstVfmadd231ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmaddsub132pd, kInstVfmaddsub132pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmaddsub132pd, kInstVfmaddsub132pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmaddsub132pd, kInstVfmaddsub132pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmaddsub132pd, kInstVfmaddsub132pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmaddsub132ps, kInstVfmaddsub132ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmaddsub132ps, kInstVfmaddsub132ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmaddsub132ps, kInstVfmaddsub132ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmaddsub132ps, kInstVfmaddsub132ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmaddsub213pd, kInstVfmaddsub213pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmaddsub213pd, kInstVfmaddsub213pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmaddsub213pd, kInstVfmaddsub213pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmaddsub213pd, kInstVfmaddsub213pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmaddsub213ps, kInstVfmaddsub213ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmaddsub213ps, kInstVfmaddsub213ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmaddsub213ps, kInstVfmaddsub213ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmaddsub213ps, kInstVfmaddsub213ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmaddsub231pd, kInstVfmaddsub231pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmaddsub231pd, kInstVfmaddsub231pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmaddsub231pd, kInstVfmaddsub231pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmaddsub231pd, kInstVfmaddsub231pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmaddsub231ps, kInstVfmaddsub231ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmaddsub231ps, kInstVfmaddsub231ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmaddsub231ps, kInstVfmaddsub231ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmaddsub231ps, kInstVfmaddsub231ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub132pd, kInstVfmsub132pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub132pd, kInstVfmsub132pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsub132pd, kInstVfmsub132pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmsub132pd, kInstVfmsub132pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub132ps, kInstVfmsub132ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub132ps, kInstVfmsub132ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsub132ps, kInstVfmsub132ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmsub132ps, kInstVfmsub132ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub132sd, kInstVfmsub132sd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub132sd, kInstVfmsub132sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmsub132ss, kInstVfmsub132ss, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub132ss, kInstVfmsub132ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmsub213pd, kInstVfmsub213pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub213pd, kInstVfmsub213pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsub213pd, kInstVfmsub213pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmsub213pd, kInstVfmsub213pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub213ps, kInstVfmsub213ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub213ps, kInstVfmsub213ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsub213ps, kInstVfmsub213ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmsub213ps, kInstVfmsub213ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub213sd, kInstVfmsub213sd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub213sd, kInstVfmsub213sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmsub213ss, kInstVfmsub213ss, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub213ss, kInstVfmsub213ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmsub231pd, kInstVfmsub231pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub231pd, kInstVfmsub231pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsub231pd, kInstVfmsub231pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmsub231pd, kInstVfmsub231pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub231ps, kInstVfmsub231ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub231ps, kInstVfmsub231ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsub231ps, kInstVfmsub231ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmsub231ps, kInstVfmsub231ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsub231sd, kInstVfmsub231sd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub231sd, kInstVfmsub231sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmsub231ss, kInstVfmsub231ss, XmmReg, XmmReg, Mem)
  INST_3x(vfmsub231ss, kInstVfmsub231ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfmsubadd132pd, kInstVfmsubadd132pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsubadd132pd, kInstVfmsubadd132pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsubadd132pd, kInstVfmsubadd132pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmsubadd132pd, kInstVfmsubadd132pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsubadd132ps, kInstVfmsubadd132ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmsubadd132ps, kInstVfmsubadd132ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsubadd132ps, kInstVfmsubadd132ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmsubadd132ps, kInstVfmsubadd132ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsubadd213pd, kInstVfmsubadd213pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsubadd213pd, kInstVfmsubadd213pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsubadd213pd, kInstVfmsubadd213pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmsubadd213pd, kInstVfmsubadd213pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsubadd213ps, kInstVfmsubadd213ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmsubadd213ps, kInstVfmsubadd213ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsubadd213ps, kInstVfmsubadd213ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmsubadd213ps, kInstVfmsubadd213ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsubadd231pd, kInstVfmsubadd231pd, XmmReg, XmmReg, Mem)
  INST_3x(vfmsubadd231pd, kInstVfmsubadd231pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsubadd231pd, kInstVfmsubadd231pd, YmmReg, YmmReg, Mem)
  INST_3x(vfmsubadd231pd, kInstVfmsubadd231pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfmsubadd231ps, kInstVfmsubadd231ps, XmmReg, XmmReg, Mem)
  INST_3x(vfmsubadd231ps, kInstVfmsubadd231ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfmsubadd231ps, kInstVfmsubadd231ps, YmmReg, YmmReg, Mem)
  INST_3x(vfmsubadd231ps, kInstVfmsubadd231ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd132pd, kInstVfnmadd132pd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd132pd, kInstVfnmadd132pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmadd132pd, kInstVfnmadd132pd, YmmReg, YmmReg, Mem)
  INST_3x(vfnmadd132pd, kInstVfnmadd132pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd132ps, kInstVfnmadd132ps, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd132ps, kInstVfnmadd132ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmadd132ps, kInstVfnmadd132ps, YmmReg, YmmReg, Mem)
  INST_3x(vfnmadd132ps, kInstVfnmadd132ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd132sd, kInstVfnmadd132sd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd132sd, kInstVfnmadd132sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmadd132ss, kInstVfnmadd132ss, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd132ss, kInstVfnmadd132ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmadd213pd, kInstVfnmadd213pd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd213pd, kInstVfnmadd213pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmadd213pd, kInstVfnmadd213pd, YmmReg, YmmReg, Mem)
  INST_3x(vfnmadd213pd, kInstVfnmadd213pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd213ps, kInstVfnmadd213ps, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd213ps, kInstVfnmadd213ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmadd213ps, kInstVfnmadd213ps, YmmReg, YmmReg, Mem)
  INST_3x(vfnmadd213ps, kInstVfnmadd213ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd213sd, kInstVfnmadd213sd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd213sd, kInstVfnmadd213sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmadd213ss, kInstVfnmadd213ss, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd213ss, kInstVfnmadd213ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmadd231pd, kInstVfnmadd231pd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd231pd, kInstVfnmadd231pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmadd231pd, kInstVfnmadd231pd, YmmReg, YmmReg, Mem)
  INST_3x(vfnmadd231pd, kInstVfnmadd231pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd231ps, kInstVfnmadd231ps, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd231ps, kInstVfnmadd231ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmadd231ps, kInstVfnmadd231ps, YmmReg, YmmReg, Mem)
  INST_3x(vfnmadd231ps, kInstVfnmadd231ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmadd231sd, kInstVfnmadd231sd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd231sd, kInstVfnmadd231sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmadd231ss, kInstVfnmadd231ss, XmmReg, XmmReg, Mem)
  INST_3x(vfnmadd231ss, kInstVfnmadd231ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmsub132pd, kInstVfnmsub132pd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub132pd, kInstVfnmsub132pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmsub132pd, kInstVfnmsub132pd, YmmReg, YmmReg, Mem)
  INST_3x(vfnmsub132pd, kInstVfnmsub132pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmsub132ps, kInstVfnmsub132ps, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub132ps, kInstVfnmsub132ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmsub132ps, kInstVfnmsub132ps, YmmReg, YmmReg, Mem)
  INST_3x(vfnmsub132ps, kInstVfnmsub132ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmsub132sd, kInstVfnmsub132sd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub132sd, kInstVfnmsub132sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmsub132ss, kInstVfnmsub132ss, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub132ss, kInstVfnmsub132ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmsub213pd, kInstVfnmsub213pd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub213pd, kInstVfnmsub213pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmsub213pd, kInstVfnmsub213pd, YmmReg, YmmReg, Mem)
  INST_3x(vfnmsub213pd, kInstVfnmsub213pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmsub213ps, kInstVfnmsub213ps, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub213ps, kInstVfnmsub213ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmsub213ps, kInstVfnmsub213ps, YmmReg, YmmReg, Mem)
  INST_3x(vfnmsub213ps, kInstVfnmsub213ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmsub213sd, kInstVfnmsub213sd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub213sd, kInstVfnmsub213sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmsub213ss, kInstVfnmsub213ss, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub213ss, kInstVfnmsub213ss, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmsub231pd, kInstVfnmsub231pd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub231pd, kInstVfnmsub231pd, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmsub231pd, kInstVfnmsub231pd, YmmReg, YmmReg, Mem)
  INST_3x(vfnmsub231pd, kInstVfnmsub231pd, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmsub231ps, kInstVfnmsub231ps, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub231ps, kInstVfnmsub231ps, XmmReg, XmmReg, XmmReg)
  INST_3x(vfnmsub231ps, kInstVfnmsub231ps, YmmReg, YmmReg, Mem)
  INST_3x(vfnmsub231ps, kInstVfnmsub231ps, YmmReg, YmmReg, YmmReg)

  INST_3x(vfnmsub231sd, kInstVfnmsub231sd, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub231sd, kInstVfnmsub231sd, XmmReg, XmmReg, XmmReg)

  INST_3x(vfnmsub231ss, kInstVfnmsub231ss, XmmReg, XmmReg, Mem)
  INST_3x(vfnmsub231ss, kInstVfnmsub231ss, XmmReg, XmmReg, XmmReg)

  // --------------------------------------------------------------------------
  // [FMA4]
  // --------------------------------------------------------------------------

  INST_4x(vfmaddpd, kInstVfmaddpd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmaddpd, kInstVfmaddpd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmaddpd, kInstVfmaddpd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmaddpd, kInstVfmaddpd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmaddpd, kInstVfmaddpd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmaddpd, kInstVfmaddpd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmaddps, kInstVfmaddps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmaddps, kInstVfmaddps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmaddps, kInstVfmaddps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmaddps, kInstVfmaddps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmaddps, kInstVfmaddps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmaddps, kInstVfmaddps, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmaddsd, kInstVfmaddsd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmaddsd, kInstVfmaddsd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmaddsd, kInstVfmaddsd, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfmaddss, kInstVfmaddss, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmaddss, kInstVfmaddss, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmaddss, kInstVfmaddss, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfmaddsubpd, kInstVfmaddsubpd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmaddsubpd, kInstVfmaddsubpd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmaddsubpd, kInstVfmaddsubpd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmaddsubpd, kInstVfmaddsubpd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmaddsubpd, kInstVfmaddsubpd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmaddsubpd, kInstVfmaddsubpd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmaddsubps, kInstVfmaddsubps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmaddsubps, kInstVfmaddsubps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmaddsubps, kInstVfmaddsubps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmaddsubps, kInstVfmaddsubps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmaddsubps, kInstVfmaddsubps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmaddsubps, kInstVfmaddsubps, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmsubaddpd, kInstVfmsubaddpd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmsubaddpd, kInstVfmsubaddpd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmsubaddpd, kInstVfmsubaddpd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmsubaddpd, kInstVfmsubaddpd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmsubaddpd, kInstVfmsubaddpd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmsubaddpd, kInstVfmsubaddpd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmsubaddps, kInstVfmsubaddps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmsubaddps, kInstVfmsubaddps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmsubaddps, kInstVfmsubaddps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmsubaddps, kInstVfmsubaddps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmsubaddps, kInstVfmsubaddps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmsubaddps, kInstVfmsubaddps, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmsubpd, kInstVfmsubpd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmsubpd, kInstVfmsubpd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmsubpd, kInstVfmsubpd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmsubpd, kInstVfmsubpd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmsubpd, kInstVfmsubpd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmsubpd, kInstVfmsubpd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmsubps, kInstVfmsubps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmsubps, kInstVfmsubps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmsubps, kInstVfmsubps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfmsubps, kInstVfmsubps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfmsubps, kInstVfmsubps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfmsubps, kInstVfmsubps, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfmsubsd, kInstVfmsubsd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmsubsd, kInstVfmsubsd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmsubsd, kInstVfmsubsd, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfmsubss, kInstVfmsubss, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfmsubss, kInstVfmsubss, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfmsubss, kInstVfmsubss, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfnmaddpd, kInstVfnmaddpd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmaddpd, kInstVfnmaddpd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmaddpd, kInstVfnmaddpd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfnmaddpd, kInstVfnmaddpd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfnmaddpd, kInstVfnmaddpd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfnmaddpd, kInstVfnmaddpd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfnmaddps, kInstVfnmaddps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmaddps, kInstVfnmaddps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmaddps, kInstVfnmaddps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfnmaddps, kInstVfnmaddps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfnmaddps, kInstVfnmaddps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfnmaddps, kInstVfnmaddps, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfnmaddsd, kInstVfnmaddsd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmaddsd, kInstVfnmaddsd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmaddsd, kInstVfnmaddsd, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfnmaddss, kInstVfnmaddss, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmaddss, kInstVfnmaddss, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmaddss, kInstVfnmaddss, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfnmsubpd, kInstVfnmsubpd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmsubpd, kInstVfnmsubpd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmsubpd, kInstVfnmsubpd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfnmsubpd, kInstVfnmsubpd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfnmsubpd, kInstVfnmsubpd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfnmsubpd, kInstVfnmsubpd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfnmsubps, kInstVfnmsubps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmsubps, kInstVfnmsubps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmsubps, kInstVfnmsubps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vfnmsubps, kInstVfnmsubps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vfnmsubps, kInstVfnmsubps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vfnmsubps, kInstVfnmsubps, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vfnmsubsd, kInstVfnmsubsd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmsubsd, kInstVfnmsubsd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmsubsd, kInstVfnmsubsd, XmmReg, XmmReg, XmmReg, Mem)

  INST_4x(vfnmsubss, kInstVfnmsubss, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vfnmsubss, kInstVfnmsubss, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vfnmsubss, kInstVfnmsubss, XmmReg, XmmReg, XmmReg, Mem)

  // --------------------------------------------------------------------------
  // [XOP]
  // --------------------------------------------------------------------------

  INST_2x(vfrczpd, kInstVfrczpd, XmmReg, XmmReg)
  INST_2x(vfrczpd, kInstVfrczpd, XmmReg, Mem)
  INST_2x(vfrczpd, kInstVfrczpd, YmmReg, YmmReg)
  INST_2x(vfrczpd, kInstVfrczpd, YmmReg, Mem)

  INST_2x(vfrczps, kInstVfrczps, XmmReg, XmmReg)
  INST_2x(vfrczps, kInstVfrczps, XmmReg, Mem)
  INST_2x(vfrczps, kInstVfrczps, YmmReg, YmmReg)
  INST_2x(vfrczps, kInstVfrczps, YmmReg, Mem)

  INST_2x(vfrczsd, kInstVfrczsd, XmmReg, XmmReg)
  INST_2x(vfrczsd, kInstVfrczsd, XmmReg, Mem)

  INST_2x(vfrczss, kInstVfrczss, XmmReg, XmmReg)
  INST_2x(vfrczss, kInstVfrczss, XmmReg, Mem)

  INST_4x(vpcmov, kInstVpcmov, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpcmov, kInstVpcmov, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpcmov, kInstVpcmov, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vpcmov, kInstVpcmov, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vpcmov, kInstVpcmov, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vpcmov, kInstVpcmov, YmmReg, YmmReg, YmmReg, Mem)

  INST_4i(vpcomb, kInstVpcomb, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomb, kInstVpcomb, XmmReg, XmmReg, Mem, Imm)
  INST_4i(vpcomd, kInstVpcomd, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomd, kInstVpcomd, XmmReg, XmmReg, Mem, Imm)
  INST_4i(vpcomq, kInstVpcomq, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomq, kInstVpcomq, XmmReg, XmmReg, Mem, Imm)
  INST_4i(vpcomw, kInstVpcomw, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomw, kInstVpcomw, XmmReg, XmmReg, Mem, Imm)

  INST_4i(vpcomub, kInstVpcomub, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomub, kInstVpcomub, XmmReg, XmmReg, Mem, Imm)
  INST_4i(vpcomud, kInstVpcomud, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomud, kInstVpcomud, XmmReg, XmmReg, Mem, Imm)
  INST_4i(vpcomuq, kInstVpcomuq, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomuq, kInstVpcomuq, XmmReg, XmmReg, Mem, Imm)
  INST_4i(vpcomuw, kInstVpcomuw, XmmReg, XmmReg, XmmReg, Imm)
  INST_4i(vpcomuw, kInstVpcomuw, XmmReg, XmmReg, Mem, Imm)

  INST_4x(vpermil2pd, kInstVpermil2pd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpermil2pd, kInstVpermil2pd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpermil2pd, kInstVpermil2pd, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vpermil2pd, kInstVpermil2pd, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vpermil2pd, kInstVpermil2pd, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vpermil2pd, kInstVpermil2pd, YmmReg, YmmReg, YmmReg, Mem)

  INST_4x(vpermil2ps, kInstVpermil2ps, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpermil2ps, kInstVpermil2ps, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpermil2ps, kInstVpermil2ps, XmmReg, XmmReg, XmmReg, Mem)
  INST_4x(vpermil2ps, kInstVpermil2ps, YmmReg, YmmReg, YmmReg, YmmReg)
  INST_4x(vpermil2ps, kInstVpermil2ps, YmmReg, YmmReg, Mem, YmmReg)
  INST_4x(vpermil2ps, kInstVpermil2ps, YmmReg, YmmReg, YmmReg, Mem)

  INST_2x(vphaddbd, kInstVphaddbd, XmmReg, XmmReg)
  INST_2x(vphaddbd, kInstVphaddbd, XmmReg, Mem)
  INST_2x(vphaddbq, kInstVphaddbq, XmmReg, XmmReg)
  INST_2x(vphaddbq, kInstVphaddbq, XmmReg, Mem)
  INST_2x(vphaddbw, kInstVphaddbw, XmmReg, XmmReg)
  INST_2x(vphaddbw, kInstVphaddbw, XmmReg, Mem)
  INST_2x(vphadddq, kInstVphadddq, XmmReg, XmmReg)
  INST_2x(vphadddq, kInstVphadddq, XmmReg, Mem)
  INST_2x(vphaddwd, kInstVphaddwd, XmmReg, XmmReg)
  INST_2x(vphaddwd, kInstVphaddwd, XmmReg, Mem)
  INST_2x(vphaddwq, kInstVphaddwq, XmmReg, XmmReg)
  INST_2x(vphaddwq, kInstVphaddwq, XmmReg, Mem)

  INST_2x(vphaddubd, kInstVphaddubd, XmmReg, XmmReg)
  INST_2x(vphaddubd, kInstVphaddubd, XmmReg, Mem)
  INST_2x(vphaddubq, kInstVphaddubq, XmmReg, XmmReg)
  INST_2x(vphaddubq, kInstVphaddubq, XmmReg, Mem)
  INST_2x(vphaddubw, kInstVphaddubw, XmmReg, XmmReg)
  INST_2x(vphaddubw, kInstVphaddubw, XmmReg, Mem)
  INST_2x(vphaddudq, kInstVphaddudq, XmmReg, XmmReg)
  INST_2x(vphaddudq, kInstVphaddudq, XmmReg, Mem)
  INST_2x(vphadduwd, kInstVphadduwd, XmmReg, XmmReg)
  INST_2x(vphadduwd, kInstVphadduwd, XmmReg, Mem)
  INST_2x(vphadduwq, kInstVphadduwq, XmmReg, XmmReg)
  INST_2x(vphadduwq, kInstVphadduwq, XmmReg, Mem)

  INST_2x(vphsubbw, kInstVphsubbw, XmmReg, XmmReg)
  INST_2x(vphsubbw, kInstVphsubbw, XmmReg, Mem)
  INST_2x(vphsubdq, kInstVphsubdq, XmmReg, XmmReg)
  INST_2x(vphsubdq, kInstVphsubdq, XmmReg, Mem)
  INST_2x(vphsubwd, kInstVphsubwd, XmmReg, XmmReg)
  INST_2x(vphsubwd, kInstVphsubwd, XmmReg, Mem)

  INST_4x(vpmacsdd, kInstVpmacsdd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacsdd, kInstVpmacsdd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacsdqh, kInstVpmacsdqh, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacsdqh, kInstVpmacsdqh, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacsdql, kInstVpmacsdql, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacsdql, kInstVpmacsdql, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacswd, kInstVpmacswd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacswd, kInstVpmacswd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacsww, kInstVpmacsww, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacsww, kInstVpmacsww, XmmReg, XmmReg, Mem, XmmReg)

  INST_4x(vpmacssdd, kInstVpmacssdd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacssdd, kInstVpmacssdd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacssdqh, kInstVpmacssdqh, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacssdqh, kInstVpmacssdqh, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacssdql, kInstVpmacssdql, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacssdql, kInstVpmacssdql, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacsswd, kInstVpmacsswd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacsswd, kInstVpmacsswd, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpmacssww, kInstVpmacssww, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmacssww, kInstVpmacssww, XmmReg, XmmReg, Mem, XmmReg)

  INST_4x(vpmadcsswd, kInstVpmadcsswd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmadcsswd, kInstVpmadcsswd, XmmReg, XmmReg, Mem, XmmReg)

  INST_4x(vpmadcswd, kInstVpmadcswd, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpmadcswd, kInstVpmadcswd, XmmReg, XmmReg, Mem, XmmReg)

  INST_4x(vpperm, kInstVpperm, XmmReg, XmmReg, XmmReg, XmmReg)
  INST_4x(vpperm, kInstVpperm, XmmReg, XmmReg, Mem, XmmReg)
  INST_4x(vpperm, kInstVpperm, XmmReg, XmmReg, XmmReg, Mem)

  INST_3x(vprotb, kInstVprotb, XmmReg, XmmReg, XmmReg)
  INST_3x(vprotb, kInstVprotb, XmmReg, Mem, XmmReg)
  INST_3x(vprotb, kInstVprotb, XmmReg, XmmReg, Mem)
  INST_3i(vprotb, kInstVprotb, XmmReg, XmmReg, Imm)
  INST_3i(vprotb, kInstVprotb, XmmReg, Mem, Imm)

  INST_3x(vprotd, kInstVprotd, XmmReg, XmmReg, XmmReg)
  INST_3x(vprotd, kInstVprotd, XmmReg, Mem, XmmReg)
  INST_3x(vprotd, kInstVprotd, XmmReg, XmmReg, Mem)
  INST_3i(vprotd, kInstVprotd, XmmReg, XmmReg, Imm)
  INST_3i(vprotd, kInstVprotd, XmmReg, Mem, Imm)

  INST_3x(vprotq, kInstVprotq, XmmReg, XmmReg, XmmReg)
  INST_3x(vprotq, kInstVprotq, XmmReg, Mem, XmmReg)
  INST_3x(vprotq, kInstVprotq, XmmReg, XmmReg, Mem)
  INST_3i(vprotq, kInstVprotq, XmmReg, XmmReg, Imm)
  INST_3i(vprotq, kInstVprotq, XmmReg, Mem, Imm)

  INST_3x(vprotw, kInstVprotw, XmmReg, XmmReg, XmmReg)
  INST_3x(vprotw, kInstVprotw, XmmReg, Mem, XmmReg)
  INST_3x(vprotw, kInstVprotw, XmmReg, XmmReg, Mem)
  INST_3i(vprotw, kInstVprotw, XmmReg, XmmReg, Imm)
  INST_3i(vprotw, kInstVprotw, XmmReg, Mem, Imm)

  INST_3x(vpshab, kInstVpshab, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshab, kInstVpshab, XmmReg, Mem, XmmReg)
  INST_3x(vpshab, kInstVpshab, XmmReg, XmmReg, Mem)

  INST_3x(vpshad, kInstVpshad, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshad, kInstVpshad, XmmReg, Mem, XmmReg)
  INST_3x(vpshad, kInstVpshad, XmmReg, XmmReg, Mem)

  INST_3x(vpshaq, kInstVpshaq, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshaq, kInstVpshaq, XmmReg, Mem, XmmReg)
  INST_3x(vpshaq, kInstVpshaq, XmmReg, XmmReg, Mem)

  INST_3x(vpshaw, kInstVpshaw, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshaw, kInstVpshaw, XmmReg, Mem, XmmReg)
  INST_3x(vpshaw, kInstVpshaw, XmmReg, XmmReg, Mem)

  INST_3x(vpshlb, kInstVpshlb, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshlb, kInstVpshlb, XmmReg, Mem, XmmReg)
  INST_3x(vpshlb, kInstVpshlb, XmmReg, XmmReg, Mem)

  INST_3x(vpshld, kInstVpshld, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshld, kInstVpshld, XmmReg, Mem, XmmReg)
  INST_3x(vpshld, kInstVpshld, XmmReg, XmmReg, Mem)

  INST_3x(vpshlq, kInstVpshlq, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshlq, kInstVpshlq, XmmReg, Mem, XmmReg)
  INST_3x(vpshlq, kInstVpshlq, XmmReg, XmmReg, Mem)

  INST_3x(vpshlw, kInstVpshlw, XmmReg, XmmReg, XmmReg)
  INST_3x(vpshlw, kInstVpshlw, XmmReg, Mem, XmmReg)
  INST_3x(vpshlw, kInstVpshlw, XmmReg, XmmReg, Mem)

  // --------------------------------------------------------------------------
  // [BMI]
  // --------------------------------------------------------------------------

  //! Bitwise and-not (BMI).
  INST_3x(andn, kInstAndn, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(andn, kInstAndn, GpReg, GpReg, Mem)

  //! Bit field extract (BMI).
  INST_3x(bextr, kInstBextr, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(bextr, kInstBextr, GpReg, Mem, GpReg)

  //! Extract lower set isolated bit (BMI).
  INST_2x(blsi, kInstBlsi, GpReg, GpReg)
  //! \overload
  INST_2x(blsi, kInstBlsi, GpReg, Mem)

  //! Get mask up to lowest set bit (BMI).
  INST_2x(blsmsk, kInstBlsmsk, GpReg, GpReg)
  //! \overload
  INST_2x(blsmsk, kInstBlsmsk, GpReg, Mem)

  //! Reset lowest set bit (BMI).
  INST_2x(blsr, kInstBlsr, GpReg, GpReg)
  //! \overload
  INST_2x(blsr, kInstBlsr, GpReg, Mem)

  //! Count the number of trailing zero bits (BMI).
  INST_2x(tzcnt, kInstTzcnt, GpReg, GpReg)
  //! \overload
  INST_2x(tzcnt, kInstTzcnt, GpReg, Mem)

  // --------------------------------------------------------------------------
  // [LZCNT]
  // --------------------------------------------------------------------------

  //! Count the number of leading zero bits (LZCNT).
  INST_2x(lzcnt, kInstLzcnt, GpReg, GpReg)
  //! \overload
  INST_2x(lzcnt, kInstLzcnt, GpReg, Mem)

  // --------------------------------------------------------------------------
  // [BMI2]
  // --------------------------------------------------------------------------

  //! Zero high bits starting with specified bit position (BMI2).
  INST_3x(bzhi, kInstBzhi, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(bzhi, kInstBzhi, GpReg, Mem, GpReg)

  //! Unsigned multiply without affecting flags (BMI2).
  INST_3x(mulx, kInstMulx, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(mulx, kInstMulx, GpReg, GpReg, Mem)

  //! Parallel bits deposit (BMI2).
  INST_3x(pdep, kInstPdep, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(pdep, kInstPdep, GpReg, GpReg, Mem)

  //! Parallel bits extract (BMI2).
  INST_3x(pext, kInstPext, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(pext, kInstPext, GpReg, GpReg, Mem)

  //! Rotate right without affecting flags (BMI2).
  INST_3i(rorx, kInstRorx, GpReg, GpReg, Imm)
  //! \overload
  INST_3i(rorx, kInstRorx, GpReg, Mem, Imm)

  //! Shift arithmetic right without affecting flags (BMI2).
  INST_3x(sarx, kInstSarx, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(sarx, kInstSarx, GpReg, Mem, GpReg)

  //! Shift logical left without affecting flags (BMI2).
  INST_3x(shlx, kInstShlx, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(shlx, kInstShlx, GpReg, Mem, GpReg)

  //! Shift logical right without affecting flags (BMI2).
  INST_3x(shrx, kInstShrx, GpReg, GpReg, GpReg)
  //! \overload
  INST_3x(shrx, kInstShrx, GpReg, Mem, GpReg)

  // --------------------------------------------------------------------------
  // [RDRAND]
  // --------------------------------------------------------------------------

  //! Store a random number in destination register.
  //!
  //! Please do not use this instruction in cryptographic software. The result
  //! doesn't necessarily have to be random which may cause a major security
  //! issue in the software that relies on it.
  INST_1x(rdrand, kInstRdrand, GpReg)

  // --------------------------------------------------------------------------
  // [F16C]
  // --------------------------------------------------------------------------

  //! Convert packed HP-FP to SP-FP.
  INST_2x(vcvtph2ps, kInstVcvtph2ps, XmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtph2ps, kInstVcvtph2ps, XmmReg, Mem)
  //! \overload
  INST_2x(vcvtph2ps, kInstVcvtph2ps, YmmReg, XmmReg)
  //! \overload
  INST_2x(vcvtph2ps, kInstVcvtph2ps, YmmReg, Mem)

  //! Convert packed SP-FP to HP-FP.
  INST_3i(vcvtps2ph, kInstVcvtps2ph, XmmReg, XmmReg, Imm)
  //! \overload
  INST_3i(vcvtps2ph, kInstVcvtps2ph, Mem, XmmReg, Imm)
  //! \overload
  INST_3i(vcvtps2ph, kInstVcvtps2ph, XmmReg, YmmReg, Imm)
  //! \overload
  INST_3i(vcvtps2ph, kInstVcvtps2ph, Mem, YmmReg, Imm)
};

//! \}

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

//! \addtogroup asmjit_x86x64_general
//! \{

//! X86-only assembler.
struct Assembler : public X86X64Assembler {
  ASMJIT_NO_COPY(Assembler)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API Assembler(Runtime* runtime);
  ASMJIT_API virtual ~Assembler();

  // --------------------------------------------------------------------------
  // [Reloc]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual size_t _relocCode(void* dst, Ptr base) const;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3);

  // -------------------------------------------------------------------------
  // [Options]
  // -------------------------------------------------------------------------

  ASMJIT_X86X64_EMIT_OPTIONS(Assembler)

  // --------------------------------------------------------------------------
  // [X86-Only Instructions]
  // --------------------------------------------------------------------------

  //! Decimal adjust AL after addition (32-bit).
  INST_0x(daa, kInstDaa)
  //! Decimal adjust AL after subtraction (32-bit).
  INST_0x(das, kInstDas)

  //! Pop all Gp registers - EDI|ESI|EBP|Ign|EBX|EDX|ECX|EAX.
  INST_0x(popa, kInstPopa)
  //! Push all Gp registers - EAX|ECX|EDX|EBX|ESP|EBP|ESI|EDI.
  INST_0x(pusha, kInstPusha)
};

//! \}

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)

namespace asmjit {
namespace x64 {

//! \addtogroup asmjit_x86x64_general
//! \{

//! X64-only assembler.
struct Assembler : public X86X64Assembler {
  ASMJIT_NO_COPY(Assembler)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API Assembler(Runtime* runtime);
  ASMJIT_API virtual ~Assembler();

  // --------------------------------------------------------------------------
  // [Reloc]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual size_t _relocCode(void* dst, Ptr base) const;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3);

  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

  ASMJIT_X86X64_EMIT_OPTIONS(Assembler)

  // --------------------------------------------------------------------------
  // [X64-Only Instructions]
  // --------------------------------------------------------------------------

  //! Convert DWORD to QWORD (RAX <- Sign Extend EAX).
  INST_0x(cdqe, kInstCdqe)
  //! Convert QWORD to OWORD (RDX:RAX <- Sign Extend RAX).
  INST_0x(cqo, kInstCqo)

  //! Compares the 128-bit value in RDX:RAX with the memory operand (64-bit).
  INST_1x(cmpxchg16b, kInstCmpxchg16b, Mem)

  //! Move DWORD to QWORD with sign-extension.
  INST_2x(movsxd, kInstMovsxd, GpReg, GpReg)
  //! \overload
  INST_2x(movsxd, kInstMovsxd, GpReg, Mem)

  //! Load Ecx/Rcx QWORDs from ds:[Esi/Rri] to Rax.
  INST_0x(rep_lodsq, kInstRepLodsq)
  //! Move Ecx/Rcx QWORDs from ds:[Esi/Rsi] to es:[Edi/Rdi].
  INST_0x(rep_movsq, kInstRepMovsq)
  //! Fill Ecx/Rcx QWORDs at es:[Edi/Rdi] with Rax.
  INST_0x(rep_stosq, kInstRepStosq)

  //! Repeated find nonmatching QWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repe_cmpsq, kInstRepeCmpsq)
  //! Find non-rax QWORD starting at ES:[EDI/RDI].
  INST_0x(repe_scasq, kInstRepeScasq)

  //! Repeated find nonmatching QWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_0x(repne_cmpsq, kInstRepneCmpsq)
  //! Find RAX, starting at ES:[EDI/RDI].
  INST_0x(repne_scasq, kInstRepneScasq)

  using X86X64Assembler::movq;

  //! \overload
  INST_2x(movq, kInstMovq, GpReg, MmReg)
  //! \overload
  INST_2x(movq, kInstMovq, MmReg, GpReg)

  //! \overload
  INST_2x(movq, kInstMovq, GpReg, XmmReg)
  //! \overload
  INST_2x(movq, kInstMovq, XmmReg, GpReg)

  // --------------------------------------------------------------------------
  // [AVX]
  // --------------------------------------------------------------------------

  INST_2x(vmovq, kInstVmovq, XmmReg, GpReg)
  INST_2x(vmovq, kInstVmovq, GpReg, XmmReg)

  INST_3i(vpextrq, kInstVpextrq, GpReg, XmmReg, Imm)
  INST_3i(vpextrq, kInstVpextrq, Mem, XmmReg, Imm)

  INST_4i(vpinsrq, kInstVpinsrq, XmmReg, XmmReg, GpReg, Imm)
  INST_4i(vpinsrq, kInstVpinsrq, XmmReg, XmmReg, Mem, Imm)

  // --------------------------------------------------------------------------
  // [FSGSBASE]
  // --------------------------------------------------------------------------

  INST_1x(rdfsbase, kInstRdfsbase, GpReg)
  INST_1x(rdgsbase, kInstRdgsbase, GpReg)
  INST_1x(wrfsbase, kInstWrfsbase, GpReg)
  INST_1x(wrgsbase, kInstWrgsbase, GpReg)
};

//! \}

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

// ============================================================================
// [CodeGen-End]
// ============================================================================

#undef INST_0x

#undef INST_1x
#undef INST_1x_
#undef INST_1i
#undef INST_1cc

#undef INST_2x
#undef INST_2x_
#undef INST_2i
#undef INST_2cc

#undef INST_3x
#undef INST_3x_
#undef INST_3i

#undef INST_4x
#undef INST_4x_
#undef INST_4i

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86ASSEMBLER_H
