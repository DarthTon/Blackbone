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

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86_general
//! \{

// ============================================================================
// [asmjit::X86Assembler]
// ============================================================================

// \internal
#define ASMJIT_X86_EMIT_OPTIONS(_Class_) \
  /*! Force short form of jmp/jcc instruction. */ \
  ASMJIT_INLINE _Class_& short_() { \
    _instOptions |= kInstOptionShortForm; \
    return *this; \
  } \
  \
  /*! Force long form of jmp/jcc instruction. */ \
  ASMJIT_INLINE _Class_& long_() { \
    _instOptions |= kInstOptionLongForm; \
    return *this; \
  } \
  \
  /*! Condition is likely to be taken (has only benefit on P4). */ \
  ASMJIT_INLINE _Class_& taken() { \
    _instOptions |= kInstOptionTaken; \
    return *this; \
  } \
  \
  /*! Condition is unlikely to be taken (has only benefit on P4). */ \
  ASMJIT_INLINE _Class_& notTaken() { \
    _instOptions |= kInstOptionNotTaken; \
    return *this; \
  } \
  \
  /*! Use LOCK prefix. */ \
  ASMJIT_INLINE _Class_& lock() { \
    _instOptions |= kX86InstOptionLock; \
    return *this; \
  } \
  \
  /*! Force REX prefix (X64). */ \
  ASMJIT_INLINE _Class_& rex() { \
    _instOptions |= kX86InstOptionRex; \
    return *this; \
  } \
  \
  /*! Force 3-byte VEX prefix (AVX+). */ \
  ASMJIT_INLINE _Class_& vex3() { \
    _instOptions |= kX86InstOptionVex3; \
    return *this; \
  } \
  \
  /*! Force 4-byte EVEX prefix (AVX512+). */ \
  ASMJIT_INLINE _Class_& evex() { \
    _instOptions |= kX86InstOptionEvex; \
    return *this; \
  } \
  \
  /*! Use zeroing instead of merging (AVX512+). */ \
  ASMJIT_INLINE _Class_& z() { \
    _instOptions |= kX86InstOptionEvexZero; \
    return *this; \
  } \
  \
  /*! Broadcast one element to all other elements (AVX512+). */ \
  ASMJIT_INLINE _Class_& _1ToN() { \
    _instOptions |= kX86InstOptionEvexOneN; \
    return *this; \
  } \
  \
  /*! Suppress all exceptions (AVX512+). */ \
  ASMJIT_INLINE _Class_& sae() { \
    _instOptions |= kX86InstOptionEvexSae; \
    return *this; \
  } \
  \
  /*! Static rounding mode `round-to-nearest` (even) and `SAE` (AVX512+). */ \
  ASMJIT_INLINE _Class_& rn_sae() { \
    _instOptions |= kX86InstOptionEvexRnSae; \
    return *this; \
  } \
  \
  /*! Static rounding mode `round-down` (toward -inf) and `SAE` (AVX512+). */ \
  ASMJIT_INLINE _Class_& rd_sae() { \
    _instOptions |= kX86InstOptionEvexRdSae; \
    return *this; \
  } \
  \
  /*! Static rounding mode `round-up` (toward +inf) and `SAE` (AVX512+). */ \
  ASMJIT_INLINE _Class_& ru_sae() { \
    _instOptions |= kX86InstOptionEvexRuSae; \
    return *this; \
  } \
  \
  /*! Static rounding mode `round-toward-zero` (truncate) and `SAE` (AVX512+). */ \
  ASMJIT_INLINE _Class_& rz_sae() { \
    _instOptions |= kX86InstOptionEvexRzSae; \
    return *this; \
  }

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
//! code to be emitted. It will assert in debug mode and put the `Assembler`
//! instance to an  error state in production mode.
//!
//! Code Generation
//! ---------------
//!
//! To generate code is only needed to create instance of `Assembler`
//! and to use intrinsics. See example how to do that:
//!
//! ~~~
//! // Use asmjit namespace.
//! using namespace asmjit;
//! using namespace asmjit::x86;
//!
//! // Create X86Assembler instance.
//! X86Assembler a;
//!
//! // Prolog.
//! a.push(ebp);
//! a.mov(ebp, esp);
//!
//! // Mov 1024 to EAX, EAX is also return value.
//! a.mov(eax, 1024);
//!
//! // Epilog.
//! a.mov(esp, ebp);
//! a.pop(ebp);
//!
//! // Return.
//! a.ret();
//! ~~~
//!
//! You can see that syntax is very close to Intel one. Only difference is that
//! you are calling functions that emit binary code for you. All registers are
//! in `asmjit::x86` namespace, so it's very comfortable to use it (look at the
//! `use namespace` section). Without importing `asmjit::x86` registers would
//! have to be written as `x86::eax`, `x86::esp`, and so on.
//!
//! There is also possibility to use memory addresses and immediates. Use
//! `ptr()`, `byte_ptr()`, `word_ptr()`, `dword_ptr()` and similar functions to
//! build a memory address operand. In most cases `ptr()` is enough, because an
//! information related to the operand size is needed only in rare cases, that
//! is an instruction without having any register operands, such as `inc [mem]`.
//!
//! for example, `a` is an `X86Assembler` instance:
//!
//! ~~~
//! a.mov(ptr(eax), 0);             // mov ptr [eax], 0
//! a.mov(ptr(eax), edx);           // mov ptr [eax], edx
//! ~~~
//!
//! But it's also possible to create complex addresses offered by x86 architecture:
//!
//! ~~~
//! // eax + ecx*x addresses
//! a.mov(ptr(eax, ecx, 0), 0);     // mov ptr [eax + ecx], 0
//! a.mov(ptr(eax, ecx, 1), 0);     // mov ptr [eax + ecx * 2], 0
//! a.mov(ptr(eax, ecx, 2), 0);     // mov ptr [eax + ecx * 4], 0
//! a.mov(ptr(eax, ecx, 3), 0);     // mov ptr [eax + ecx * 8], 0
//! // eax + ecx*x + disp addresses
//! a.mov(ptr(eax, ecx, 0,  4), 0); // mov ptr [eax + ecx     +  4], 0
//! a.mov(ptr(eax, ecx, 1,  8), 0); // mov ptr [eax + ecx * 2 +  8], 0
//! a.mov(ptr(eax, ecx, 2, 12), 0); // mov ptr [eax + ecx * 4 + 12], 0
//! a.mov(ptr(eax, ecx, 3, 16), 0); // mov ptr [eax + ecx * 8 + 16], 0
//! ~~~
//!
//! All addresses shown are using `x86::ptr()` to make memory operand. Some
//! assembler instructions using a single operand need to know the size of
//! the operand to avoid ambiguity. For example `a.inc(ptr(eax))` is ambiguous
//! and would cause a runtime error. This problem can be fixed by using memory
//! operand with size specified - `byte_ptr`, `word_ptr`, `dword_ptr`, see the
//! code below:
//!
//! ~~~
//! // [byte] address.
//! a.inc(byte_ptr(eax));           // Inc byte ptr [eax].
//! a.dec(byte_ptr(eax));           // Dec byte ptr [eax].
//! // [word] address.
//! a.inc(word_ptr(eax));           // Inc word ptr [eax].
//! a.dec(word_ptr(eax));           // Dec word ptr [eax].
//! // [dword] address.
//! a.inc(dword_ptr(eax));          // Inc dword ptr [eax].
//! a.dec(dword_ptr(eax));          // Dec dword ptr [eax].
//! // [dword] address.
//! a.inc(dword_ptr(rax));          // Inc qword ptr [rax].
//! a.dec(dword_ptr(rax));          // Dec qword ptr [rax].
//! ~~~
//!
//! Calling JIT Code
//! ----------------
//!
//! After you are finished with emitting instructions, you can make your function
//! callable by using `Assembler::make()` method. This method will use memory
//! manager to allocate virtual memory and relocates generated code to it. The
//! memory is allocated through `Runtime` instance provided to `X86Assembler`
//! constructor.
//!
//! The size of the code generated can be retrieved by `getCodeSize()` and
//! `getOffset()` methods. The `getOffset()` method returns the current offset
//! (that is mostly equal to the final code size, if called after the code
//! generation) and `getCodeSize()` returns the final code size with possible
//! trampolines. The `takeCode()` method can be used to take the internal buffer
//! and reset the code generator, but the buffer returned has to be freed manually
//! in such case.
//!
//! Machine code can be executed only in memory that is marked executable. This
//! mark is usually not set for memory returned by a C/C++ `malloc()` function.
//! The `VMemUtil::alloc()` function can be used allocate a memory where the code
//! can be executed. Please note that `VMemUtil` is a low-level class that works
//! at memory page level. High level interface that is similar to malloc/free is
//! provided by `VMemMgr` class.
//!
//! The next example shows how to allocate memory where the code can be executed:
//!
//! ~~~
//! using namespace asmjit;
//!
//! JitRuntime runtime;
//! X86Assembler a(&runtime);
//!
//! ... Code generation ...
//!
//! // The function prototype.
//! typedef void (*MyFunc)();
//!
//! // Make the function.
//! MyFunc func = asmjit_cast<MyFunc>(a.make());
//!
//! // Call the function.
//! func();
//!
//! // Release the function if not needed anymore.
//! runtime.release(func);
//! ~~~
//!
//! This was a very primitive example showing how the generated code can be.
//! executed by using the foundation of classes AsmJit offers. In production
//! nobody is likely to generate a function that is only called once and freed
//! immediately after it's been called, however, the concept of releasing code
//! that is not needed anymore should be clear.
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
//! a.push(ebp);
//! a.mov(ebp, esp);
//! a.push(esi);
//! a.push(edi);
//!
//! // Fetch arguments
//! a.mov(esi, dword_ptr(ebp, arg_offset + 0)); // Get dst.
//! a.mov(edi, dword_ptr(ebp, arg_offset + 4)); // Get src.
//! a.mov(ecx, dword_ptr(ebp, arg_offset + 8)); // Get count.
//!
//! // Bind L_Loop label to here.
//! a.bind(L_Loop);
//!
//! Copy 4 bytes.
//! a.mov(eax, dword_ptr(esi));
//! a.mov(dword_ptr(edi), eax);
//!
//! // Increment pointers.
//! a.add(esi, 4);
//! a.add(edi, 4);
//!
//! // Repeat loop until (--ecx != 0).
//! a.dec(ecx);
//! a.jz(L_Loop);
//!
//! // Epilog.
//! a.pop(edi);
//! a.pop(esi);
//! a.mov(esp, ebp);
//! a.pop(ebp);
//!
//! // Return: Pop the stack by `arg_size` as defined by `STDCALL` convention.
//! a.ret(arg_size);
//! ~~~
//!
//! If you need more abstraction for generating assembler code and you want
//! to hide calling conventions between 32-bit and 64-bit operating systems,
//! look at `Compiler` class that is designed for higher level code
//! generation.
//!
//! Advanced Code Generation
//! ------------------------
//!
//! This section describes some advanced generation features of `Assembler`
//! class which can be simply overlooked. The first thing that is very likely
//! needed is generic register support. In previous example the named registers
//! were used. AsmJit contains functions which can convert register index into
//! operand and back.
//!
//! Let's define function which can be used to generate some abstract code:
//!
//! ~~~
//! // Simple function that generates dword copy.
//! void genCopyDWord(Assembler& a, const X86GpReg& dst, const X86GpReg& src, const X86GpReg& tmp) {
//!   a.mov(tmp, dword_ptr(src));
//!   a.mov(dword_ptr(dst), tmp);
//! }
//! ~~~
//!
//! This function can be called like `genCopyDWord(a, edi, esi, ebx)` or by
//! using existing `X86GpReg` instances. This abstraction allows to join more
//! code sections together without rewriting each to use specific registers.
//! You need to take care only about implicit registers which may be used by
//! several instructions (like mul, imul, div, idiv, shifting, etc...).
//!
//! Next, more advanced, but often needed technique is that you can build your
//! own registers allocator. X86 architecture contains 8 general purpose
//! registers, 8 Mm registers and 8 Xmm/Ymm/Zmm registers. X64 architecture
//! extends the count of Gp registers and Xmm/Ymm/Zmm registers to 16. AVX-512
//! architecture extends Xmm/Ymm/Zmm SIMD registers to 32.
//!
//! To create a general purpose register operand from register index use
//! `gpb_lo()`, `gpb_hi()`, `gpw()`, `gpd()`, `gpq()`. To create registers of
//! other types there `fp()`, `mm()`, `k()`, `xmm()`, `ymm()` and `zmm()`
//! functions available that return a new register operand.
//!
//! \sa X86Compiler.
struct ASMJIT_VCLASS X86Assembler : public Assembler {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API X86Assembler(Runtime* runtime, uint32_t arch
#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
    = kArchHost
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64
  );
  ASMJIT_API virtual ~X86Assembler();

  // --------------------------------------------------------------------------
  // [Arch]
  // --------------------------------------------------------------------------

  //! Get count of registers of the current architecture and mode.
  ASMJIT_INLINE const X86RegCount& getRegCount() const {
    return _regCount;
  }

  //! Get Gpd or Gpq register depending on the current architecture.
  ASMJIT_INLINE X86GpReg gpz(uint32_t index) const {
    return X86GpReg(zax, index);
  }

  //! Create an architecture dependent intptr_t memory operand.
  ASMJIT_INLINE X86Mem intptr_ptr(const X86GpReg& base, int32_t disp = 0) const {
    return x86::ptr(base, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const X86GpReg& base, const X86GpReg& index, uint32_t shift = 0, int32_t disp = 0) const {
    return x86::ptr(base, index, shift, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const Label& label, int32_t disp = 0) const {
    return x86::ptr(label, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const Label& label, const X86GpReg& index, uint32_t shift, int32_t disp = 0) const {
    return x86::ptr(label, index, shift, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr_abs(Ptr pAbs, int32_t disp = 0) const {
    return x86::ptr_abs(pAbs, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr_abs(Ptr pAbs, const X86GpReg& index, uint32_t shift, int32_t disp = 0) const {
    return x86::ptr_abs(pAbs, index, shift, disp, _regSize);
  }

  ASMJIT_API Error setArch(uint32_t arch);

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE void db(uint8_t x) { embed(&x, 1); }
  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE void dw(uint16_t x) { embed(&x, 2); }
  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE void dd(uint32_t x) { embed(&x, 4); }
  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE void dq(uint64_t x) { embed(&x, 8); }

  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE void dint8(int8_t x) { embed(&x, sizeof(int8_t)); }
  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE void duint8(uint8_t x) { embed(&x, sizeof(uint8_t)); }

  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE void dint16(int16_t x) { embed(&x, sizeof(int16_t)); }
  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE void duint16(uint16_t x) { embed(&x, sizeof(uint16_t)); }

  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE void dint32(int32_t x) { embed(&x, sizeof(int32_t)); }
  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE void duint32(uint32_t x) { embed(&x, sizeof(uint32_t)); }

  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE void dint64(int64_t x) { embed(&x, sizeof(int64_t)); }
  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE void duint64(uint64_t x) { embed(&x, sizeof(uint64_t)); }

  //! Add float data to the instruction stream.
  ASMJIT_INLINE void dfloat(float x) { embed(&x, sizeof(float)); }
  //! Add double data to the instruction stream.
  ASMJIT_INLINE void ddouble(double x) { embed(&x, sizeof(double)); }

  //! Add Mm data to the instruction stream.
  ASMJIT_INLINE void dmm(const Vec64& x) { embed(&x, sizeof(Vec64)); }
  //! Add Xmm data to the instruction stream.
  ASMJIT_INLINE void dxmm(const Vec128& x) { embed(&x, sizeof(Vec128)); }
  //! Add Ymm data to the instruction stream.
  ASMJIT_INLINE void dymm(const Vec256& x) { embed(&x, sizeof(Vec256)); }

  //! Add data in a given structure instance to the instruction stream.
  template<typename T>
  ASMJIT_INLINE void dstruct(const T& x) { embed(&x, static_cast<uint32_t>(sizeof(T))); }

  //! Embed absolute label pointer (4 or 8 bytes).
  ASMJIT_API Error embedLabel(const Label& op);

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error align(uint32_t mode, uint32_t offset);

  // --------------------------------------------------------------------------
  // [Reloc]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual size_t _relocCode(void* dst, Ptr baseAddress) const;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3);

  // -------------------------------------------------------------------------
  // [Options]
  // -------------------------------------------------------------------------

  ASMJIT_X86_EMIT_OPTIONS(X86Assembler)

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Count of registers depending on the current architecture.
  X86RegCount _regCount;

  //! EAX or RAX register depending on the current architecture.
  X86GpReg zax;
  //! ECX or RCX register depending on the current architecture.
  X86GpReg zcx;
  //! EDX or RDX register depending on the current architecture.
  X86GpReg zdx;
  //! EBX or RBX register depending on the current architecture.
  X86GpReg zbx;
  //! ESP or RSP register depending on the current architecture.
  X86GpReg zsp;
  //! EBP or RBP register depending on the current architecture.
  X86GpReg zbp;
  //! ESI or RSI register depending on the current architecture.
  X86GpReg zsi;
  //! EDI or RDI register depending on the current architecture.
  X86GpReg zdi;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

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

#define INST_3ii(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    return emit(_Code_, o0, o1, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, int o1, int o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, o2); \
  } \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, unsigned int o1, unsigned int o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, int64_t o1, int64_t o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, uint64_t o1, uint64_t o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, o2); \
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

#define INST_4ii(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Op3_) \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, int o2, int o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, unsigned int o2, unsigned int o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, int64_t o2, int64_t o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE Error _Inst_(const _Op0_& o0, const _Op1_& o1, uint64_t o2, uint64_t o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, o3); \
  }

  // --------------------------------------------------------------------------
  // [X86/X64]
  // --------------------------------------------------------------------------

  //! Add with Carry.
  INST_2x(adc, kX86InstIdAdc, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(adc, kX86InstIdAdc, X86GpReg, X86Mem)
  //! \overload
  INST_2i(adc, kX86InstIdAdc, X86GpReg, Imm)
  //! \overload
  INST_2x(adc, kX86InstIdAdc, X86Mem, X86GpReg)
  //! \overload
  INST_2i(adc, kX86InstIdAdc, X86Mem, Imm)

  //! Add.
  INST_2x(add, kX86InstIdAdd, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(add, kX86InstIdAdd, X86GpReg, X86Mem)
  //! \overload
  INST_2i(add, kX86InstIdAdd, X86GpReg, Imm)
  //! \overload
  INST_2x(add, kX86InstIdAdd, X86Mem, X86GpReg)
  //! \overload
  INST_2i(add, kX86InstIdAdd, X86Mem, Imm)

  //! And.
  INST_2x(and_, kX86InstIdAnd, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(and_, kX86InstIdAnd, X86GpReg, X86Mem)
  //! \overload
  INST_2i(and_, kX86InstIdAnd, X86GpReg, Imm)
  //! \overload
  INST_2x(and_, kX86InstIdAnd, X86Mem, X86GpReg)
  //! \overload
  INST_2i(and_, kX86InstIdAnd, X86Mem, Imm)

  //! Bit scan forward.
  INST_2x_(bsf, kX86InstIdBsf, X86GpReg, X86GpReg, !o0.isGpb())
  //! \overload
  INST_2x_(bsf, kX86InstIdBsf, X86GpReg, X86Mem, !o0.isGpb())

  //! Bit scan reverse.
  INST_2x_(bsr, kX86InstIdBsr, X86GpReg, X86GpReg, !o0.isGpb())
  //! \overload
  INST_2x_(bsr, kX86InstIdBsr, X86GpReg, X86Mem, !o0.isGpb())

  //! Byte swap (32-bit or 64-bit registers only) (i486).
  INST_1x_(bswap, kX86InstIdBswap, X86GpReg, o0.getSize() >= 4)

  //! Bit test.
  INST_2x(bt, kX86InstIdBt, X86GpReg, X86GpReg)
  //! \overload
  INST_2i(bt, kX86InstIdBt, X86GpReg, Imm)
  //! \overload
  INST_2x(bt, kX86InstIdBt, X86Mem, X86GpReg)
  //! \overload
  INST_2i(bt, kX86InstIdBt, X86Mem, Imm)

  //! Bit test and complement.
  INST_2x(btc, kX86InstIdBtc, X86GpReg, X86GpReg)
  //! \overload
  INST_2i(btc, kX86InstIdBtc, X86GpReg, Imm)
  //! \overload
  INST_2x(btc, kX86InstIdBtc, X86Mem, X86GpReg)
  //! \overload
  INST_2i(btc, kX86InstIdBtc, X86Mem, Imm)

  //! Bit test and reset.
  INST_2x(btr, kX86InstIdBtr, X86GpReg, X86GpReg)
  //! \overload
  INST_2i(btr, kX86InstIdBtr, X86GpReg, Imm)
  //! \overload
  INST_2x(btr, kX86InstIdBtr, X86Mem, X86GpReg)
  //! \overload
  INST_2i(btr, kX86InstIdBtr, X86Mem, Imm)

  //! Bit test and set.
  INST_2x(bts, kX86InstIdBts, X86GpReg, X86GpReg)
  //! \overload
  INST_2i(bts, kX86InstIdBts, X86GpReg, Imm)
  //! \overload
  INST_2x(bts, kX86InstIdBts, X86Mem, X86GpReg)
  //! \overload
  INST_2i(bts, kX86InstIdBts, X86Mem, Imm)

  //! Call.
  INST_1x(call, kX86InstIdCall, X86GpReg)
  //! \overload
  INST_1x(call, kX86InstIdCall, X86Mem)
  //! \overload
  INST_1x(call, kX86InstIdCall, Label)
  //! \overload
  INST_1x(call, kX86InstIdCall, Imm)
  //! \overload
  ASMJIT_INLINE Error call(Ptr o0) { return call(Imm(o0)); }

  //! Clear carry flag.
  INST_0x(clc, kX86InstIdClc)
  //! Clear direction flag.
  INST_0x(cld, kX86InstIdCld)
  //! Complement carry flag.
  INST_0x(cmc, kX86InstIdCmc)

  //! Convert BYTE to WORD (AX <- Sign Extend AL).
  INST_0x(cbw, kX86InstIdCbw)
  //! Convert DWORD to QWORD (EDX:EAX <- Sign Extend EAX).
  INST_0x(cdq, kX86InstIdCdq)
  //! Convert DWORD to QWORD (RAX <- Sign Extend EAX) (X64 Only).
  INST_0x(cdqe, kX86InstIdCdqe)
  //! Convert QWORD to OWORD (RDX:RAX <- Sign Extend RAX) (X64 Only).
  INST_0x(cqo, kX86InstIdCqo)
  //! Convert WORD to DWORD (DX:AX <- Sign Extend AX).
  INST_0x(cwd, kX86InstIdCwd)
  //! Convert WORD to DWORD (EAX <- Sign Extend AX).
  INST_0x(cwde, kX86InstIdCwde)

  //! Conditional move.
  INST_2cc(cmov, kX86InstIdCmov, X86Util::condToCmovcc, X86GpReg, X86GpReg)
  //! Conditional move.
  INST_2cc(cmov, kX86InstIdCmov, X86Util::condToCmovcc, X86GpReg, X86Mem)

  //! Compare two operands.
  INST_2x(cmp, kX86InstIdCmp, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(cmp, kX86InstIdCmp, X86GpReg, X86Mem)
  //! \overload
  INST_2i(cmp, kX86InstIdCmp, X86GpReg, Imm)
  //! \overload
  INST_2x(cmp, kX86InstIdCmp, X86Mem, X86GpReg)
  //! \overload
  INST_2i(cmp, kX86InstIdCmp, X86Mem, Imm)

  //! Compare BYTE in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(cmpsb, kX86InstIdCmpsB)
  //! Compare DWORD in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(cmpsd, kX86InstIdCmpsD)
  //! Compare QWORD in ES:[RDI] and DS:[RDI] (X64 Only).
  INST_0x(cmpsq, kX86InstIdCmpsQ)
  //! Compare WORD in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(cmpsw, kX86InstIdCmpsW)

  //! Compare and exchange (i486).
  INST_2x(cmpxchg, kX86InstIdCmpxchg, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(cmpxchg, kX86InstIdCmpxchg, X86Mem, X86GpReg)

  //! Compare and exchange 128-bit value in RDX:RAX with the memory operand (X64 Only).
  INST_1x(cmpxchg16b, kX86InstIdCmpxchg16b, X86Mem)
  //! Compare and exchange 64-bit value in EDX:EAX with the memory operand (Pentium).
  INST_1x(cmpxchg8b, kX86InstIdCmpxchg8b, X86Mem)

  //! CPU identification (i486).
  INST_0x(cpuid, kX86InstIdCpuid)

  //! Decimal adjust AL after addition (X86 Only).
  INST_0x(daa, kX86InstIdDaa)
  //! Decimal adjust AL after subtraction (X86 Only).
  INST_0x(das, kX86InstIdDas)

  //! Decrement by 1.
  INST_1x(dec, kX86InstIdDec, X86GpReg)
  //! \overload
  INST_1x(dec, kX86InstIdDec, X86Mem)

  //! Unsigned divide (xDX:xAX <- xDX:xAX / o0).
  INST_1x(div, kX86InstIdDiv, X86GpReg)
  //! \overload
  INST_1x(div, kX86InstIdDiv, X86Mem)

  //! Make stack frame for procedure parameters.
  INST_2x(enter, kX86InstIdEnter, Imm, Imm)

  //! Signed divide (xDX:xAX <- xDX:xAX / op).
  INST_1x(idiv, kX86InstIdIdiv, X86GpReg)
  //! \overload
  INST_1x(idiv, kX86InstIdIdiv, X86Mem)

  //! Signed multiply (xDX:xAX <- xAX * o0).
  INST_1x(imul, kX86InstIdImul, X86GpReg)
  //! \overload
  INST_1x(imul, kX86InstIdImul, X86Mem)

  //! Signed multiply.
  INST_2x(imul, kX86InstIdImul, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(imul, kX86InstIdImul, X86GpReg, X86Mem)
  //! \overload
  INST_2i(imul, kX86InstIdImul, X86GpReg, Imm)

  //! Signed multiply.
  INST_3i(imul, kX86InstIdImul, X86GpReg, X86GpReg, Imm)
  //! \overload
  INST_3i(imul, kX86InstIdImul, X86GpReg, X86Mem, Imm)

  //! Increment by 1.
  INST_1x(inc, kX86InstIdInc, X86GpReg)
  //! \overload
  INST_1x(inc, kX86InstIdInc, X86Mem)

  //! Interrupt.
  INST_1i(int_, kX86InstIdInt, Imm)
  //! Interrupt 3 - trap to debugger.
  ASMJIT_INLINE Error int3() { return int_(3); }

  //! Jump to `label` if condition `cc` is met.
  INST_1cc(j, kX86InstIdJ, X86Util::condToJcc, Label)

  //! Short jump if CX/ECX/RCX is zero.
  INST_2x_(jecxz, kX86InstIdJecxz, X86GpReg, Label, o0.getRegIndex() == kX86RegIndexCx)

  //! Jump.
  INST_1x(jmp, kX86InstIdJmp, X86GpReg)
  //! \overload
  INST_1x(jmp, kX86InstIdJmp, X86Mem)
  //! \overload
  INST_1x(jmp, kX86InstIdJmp, Label)
  //! \overload
  INST_1x(jmp, kX86InstIdJmp, Imm)
  //! \overload
  ASMJIT_INLINE Error jmp(Ptr dst) { return jmp(Imm(dst)); }

  //! Load AH from flags.
  INST_0x(lahf, kX86InstIdLahf)

  //! Load effective address
  INST_2x(lea, kX86InstIdLea, X86GpReg, X86Mem)

  //! High level procedure exit.
  INST_0x(leave, kX86InstIdLeave)

  //! Load BYTE from DS:[ESI/RSI] to AL.
  INST_0x(lodsb, kX86InstIdLodsB)
  //! Load DWORD from DS:[ESI/RSI] to EAX.
  INST_0x(lodsd, kX86InstIdLodsD)
  //! Load QWORD from DS:[RDI] to RAX (X64 Only).
  INST_0x(lodsq, kX86InstIdLodsQ)
  //! Load WORD from DS:[ESI/RSI] to AX.
  INST_0x(lodsw, kX86InstIdLodsW)

  //! Move.
  INST_2x(mov, kX86InstIdMov, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86GpReg, X86Mem)
  //! \overload
  INST_2i(mov, kX86InstIdMov, X86GpReg, Imm)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86Mem, X86GpReg)
  //! \overload
  INST_2i(mov, kX86InstIdMov, X86Mem, Imm)

  //! Move from segment register.
  INST_2x(mov, kX86InstIdMov, X86GpReg, X86SegReg)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86Mem, X86SegReg)
  //! Move to segment register.
  INST_2x(mov, kX86InstIdMov, X86SegReg, X86GpReg)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86SegReg, X86Mem)

  //! Move (AL|AX|EAX|RAX <- absolute address in immediate).
  INST_2x_(mov_ptr, kX86InstIdMovPtr, X86GpReg, Imm, o0.getRegIndex() == 0);
  //! \overload
  ASMJIT_INLINE Error mov_ptr(const X86GpReg& o0, Ptr o1) {
    ASMJIT_ASSERT(o0.getRegIndex() == 0);
    return emit(kX86InstIdMovPtr, o0, Imm(o1));
  }

  //! Move (absolute address in immediate <- AL|AX|EAX|RAX).
  INST_2x_(mov_ptr, kX86InstIdMovPtr, Imm, X86GpReg, o1.getRegIndex() == 0);
  //! \overload
  ASMJIT_INLINE Error mov_ptr(Ptr o0, const X86GpReg& o1) {
    ASMJIT_ASSERT(o1.getRegIndex() == 0);
    return emit(kX86InstIdMovPtr, Imm(o0), o1);
  }

  //! Move data after dwapping bytes (SSE3 - Atom).
  INST_2x_(movbe, kX86InstIdMovbe, X86GpReg, X86Mem, !o0.isGpb());
  //! \overload
  INST_2x_(movbe, kX86InstIdMovbe, X86Mem, X86GpReg, !o1.isGpb());

  //! Move BYTE from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(movsb, kX86InstIdMovsB)
  //! Move DWORD from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(movsd, kX86InstIdMovsD)
  //! Move QWORD from DS:[RSI] to ES:[RDI] (X64 Only).
  INST_0x(movsq, kX86InstIdMovsQ)
  //! Move WORD from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(movsw, kX86InstIdMovsW)

  //! Move with sign-extension.
  INST_2x(movsx, kX86InstIdMovsx, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(movsx, kX86InstIdMovsx, X86GpReg, X86Mem)

  //! Move DWORD to QWORD with sign-extension (X64 Only).
  INST_2x(movsxd, kX86InstIdMovsxd, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(movsxd, kX86InstIdMovsxd, X86GpReg, X86Mem)

  //! Move with zero-extension.
  INST_2x(movzx, kX86InstIdMovzx, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(movzx, kX86InstIdMovzx, X86GpReg, X86Mem)

  //! Unsigned multiply (xDX:xAX <- xAX * o0).
  INST_1x(mul, kX86InstIdMul, X86GpReg)
  //! \overload
  INST_1x(mul, kX86InstIdMul, X86Mem)

  //! Two's complement negation.
  INST_1x(neg, kX86InstIdNeg, X86GpReg)
  //! \overload
  INST_1x(neg, kX86InstIdNeg, X86Mem)

  //! No operation.
  INST_0x(nop, kX86InstIdNop)

  //! One's complement negation.
  INST_1x(not_, kX86InstIdNot, X86GpReg)
  //! \overload
  INST_1x(not_, kX86InstIdNot, X86Mem)

  //! Or.
  INST_2x(or_, kX86InstIdOr, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(or_, kX86InstIdOr, X86GpReg, X86Mem)
  //! \overload
  INST_2i(or_, kX86InstIdOr, X86GpReg, Imm)
  //! \overload
  INST_2x(or_, kX86InstIdOr, X86Mem, X86GpReg)
  //! \overload
  INST_2i(or_, kX86InstIdOr, X86Mem, Imm)

  //! Pop a value from the stack.
  INST_1x_(pop, kX86InstIdPop, X86GpReg, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! \overload
  INST_1x_(pop, kX86InstIdPop, X86Mem, o0.getSize() == 2 || o0.getSize() == _regSize)

  //! Pop a segment register from the stack.
  //!
  //! \note There is no instruction to pop a cs segment register.
  INST_1x_(pop, kX86InstIdPop, X86SegReg, o0.getRegIndex() != kX86SegCs);

  //! Pop all Gp registers - EDI|ESI|EBP|Ign|EBX|EDX|ECX|EAX (X86 Only).
  INST_0x(popa, kX86InstIdPopa)

  //! Pop stack into EFLAGS register (32-bit or 64-bit).
  INST_0x(popf, kX86InstIdPopf)

  //! Push WORD or DWORD/QWORD on the stack.
  INST_1x_(push, kX86InstIdPush, X86GpReg, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! Push WORD or DWORD/QWORD on the stack.
  INST_1x_(push, kX86InstIdPush, X86Mem, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! Push segment register on the stack.
  INST_1x(push, kX86InstIdPush, X86SegReg)
  //! Push WORD or DWORD/QWORD on the stack.
  INST_1i(push, kX86InstIdPush, Imm)

  //! Push all Gp registers - EAX|ECX|EDX|EBX|ESP|EBP|ESI|EDI (X86 Only).
  INST_0x(pusha, kX86InstIdPusha)

  //! Push EFLAGS register (32-bit or 64-bit) on the stack.
  INST_0x(pushf, kX86InstIdPushf)

  //! Rotate bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(rcl, kX86InstIdRcl, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(rcl, kX86InstIdRcl, X86Mem, X86GpReg)
  //! Rotate bits left.
  INST_2i(rcl, kX86InstIdRcl, X86GpReg, Imm)
  //! \overload
  INST_2i(rcl, kX86InstIdRcl, X86Mem, Imm)

  //! Rotate bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(rcr, kX86InstIdRcr, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(rcr, kX86InstIdRcr, X86Mem, X86GpReg)
  //! Rotate bits right.
  INST_2i(rcr, kX86InstIdRcr, X86GpReg, Imm)
  //! \overload
  INST_2i(rcr, kX86InstIdRcr, X86Mem, Imm)

  //! Read time-stamp counter (Pentium).
  INST_0x(rdtsc, kX86InstIdRdtsc)
  //! Read time-stamp counter and processor id (Pentium).
  INST_0x(rdtscp, kX86InstIdRdtscp)

  //! Repeated load ECX/RCX BYTEs from DS:[ESI/RSI] to AL.
  INST_0x(rep_lodsb, kX86InstIdRepLodsB)
  //! Repeated load ECX/RCX DWORDs from DS:[ESI/RSI] to EAX.
  INST_0x(rep_lodsd, kX86InstIdRepLodsD)
  //! Repeated load ECX/RCX QWORDs from DS:[RDI] to RAX (X64 Only).
  INST_0x(rep_lodsq, kX86InstIdRepLodsQ)
  //! Repeated load ECX/RCX WORDs from DS:[ESI/RSI] to AX.
  INST_0x(rep_lodsw, kX86InstIdRepLodsW)

  //! Repeated move ECX/RCX BYTEs from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(rep_movsb, kX86InstIdRepMovsB)
  //! Repeated move ECX/RCX DWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(rep_movsd, kX86InstIdRepMovsD)
  //! Repeated move ECX/RCX QWORDs from DS:[RSI] to ES:[RDI] (X64 Only).
  INST_0x(rep_movsq, kX86InstIdRepMovsQ)
  //! Repeated move ECX/RCX WORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_0x(rep_movsw, kX86InstIdRepMovsW)

  //! Repeated fill ECX/RCX BYTEs at ES:[EDI/RDI] with AL.
  INST_0x(rep_stosb, kX86InstIdRepStosB)
  //! Repeated fill ECX/RCX DWORDs at ES:[EDI/RDI] with EAX.
  INST_0x(rep_stosd, kX86InstIdRepStosD)
  //! Repeated fill ECX/RCX QWORDs at ES:[RDI] with RAX (X64 Only).
  INST_0x(rep_stosq, kX86InstIdRepStosQ)
  //! Repeated fill ECX/RCX WORDs at ES:[EDI/RDI] with AX.
  INST_0x(rep_stosw, kX86InstIdRepStosW)

  //! Repeated find non-AL BYTEs in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(repe_cmpsb, kX86InstIdRepeCmpsB)
  //! Repeated find non-EAX DWORDs in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(repe_cmpsd, kX86InstIdRepeCmpsD)
  //! Repeated find non-RAX QWORDs in ES:[RDI] and DS:[RDI] (X64 Only).
  INST_0x(repe_cmpsq, kX86InstIdRepeCmpsQ)
  //! Repeated find non-AX WORDs in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(repe_cmpsw, kX86InstIdRepeCmpsW)

  //! Repeated find non-AL BYTE starting at ES:[EDI/RDI].
  INST_0x(repe_scasb, kX86InstIdRepeScasB)
  //! Repeated find non-EAX DWORD starting at ES:[EDI/RDI].
  INST_0x(repe_scasd, kX86InstIdRepeScasD)
  //! Repeated find non-RAX QWORD starting at ES:[RDI] (X64 Only).
  INST_0x(repe_scasq, kX86InstIdRepeScasQ)
  //! Repeated find non-AX WORD starting at ES:[EDI/RDI].
  INST_0x(repe_scasw, kX86InstIdRepeScasW)

  //! Repeated find AL BYTEs in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(repne_cmpsb, kX86InstIdRepneCmpsB)
  //! Repeated find EAX DWORDs in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(repne_cmpsd, kX86InstIdRepneCmpsD)
  //! Repeated find RAX QWORDs in ES:[RDI] and DS:[RDI] (X64 Only).
  INST_0x(repne_cmpsq, kX86InstIdRepneCmpsQ)
  //! Repeated find AX WORDs in ES:[EDI/RDI] and DS:[ESI/RSI].
  INST_0x(repne_cmpsw, kX86InstIdRepneCmpsW)

  //! Repeated find AL BYTEs starting at ES:[EDI/RDI].
  INST_0x(repne_scasb, kX86InstIdRepneScasB)
  //! Repeated find EAX DWORDs starting at ES:[EDI/RDI].
  INST_0x(repne_scasd, kX86InstIdRepneScasD)
  //! Repeated find RAX QWORDs starting at ES:[RDI] (X64 Only).
  INST_0x(repne_scasq, kX86InstIdRepneScasQ)
  //! Repeated find AX WORDs starting at ES:[EDI/RDI].
  INST_0x(repne_scasw, kX86InstIdRepneScasW)

  //! Return.
  INST_0x(ret, kX86InstIdRet)
  //! \overload
  INST_1i(ret, kX86InstIdRet, Imm)

  //! Rotate bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(rol, kX86InstIdRol, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(rol, kX86InstIdRol, X86Mem, X86GpReg)
  //! Rotate bits left.
  INST_2i(rol, kX86InstIdRol, X86GpReg, Imm)
  //! \overload
  INST_2i(rol, kX86InstIdRol, X86Mem, Imm)

  //! Rotate bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(ror, kX86InstIdRor, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(ror, kX86InstIdRor, X86Mem, X86GpReg)
  //! Rotate bits right.
  INST_2i(ror, kX86InstIdRor, X86GpReg, Imm)
  //! \overload
  INST_2i(ror, kX86InstIdRor, X86Mem, Imm)

  //! Store AH into flags.
  INST_0x(sahf, kX86InstIdSahf)

  //! Integer subtraction with borrow.
  INST_2x(sbb, kX86InstIdSbb, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(sbb, kX86InstIdSbb, X86GpReg, X86Mem)
  //! \overload
  INST_2i(sbb, kX86InstIdSbb, X86GpReg, Imm)
  //! \overload
  INST_2x(sbb, kX86InstIdSbb, X86Mem, X86GpReg)
  //! \overload
  INST_2i(sbb, kX86InstIdSbb, X86Mem, Imm)

  //! Shift bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(sal, kX86InstIdSal, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(sal, kX86InstIdSal, X86Mem, X86GpReg)
  //! Shift bits left.
  INST_2i(sal, kX86InstIdSal, X86GpReg, Imm)
  //! \overload
  INST_2i(sal, kX86InstIdSal, X86Mem, Imm)

  //! Shift bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(sar, kX86InstIdSar, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(sar, kX86InstIdSar, X86Mem, X86GpReg)
  //! Shift bits right.
  INST_2i(sar, kX86InstIdSar, X86GpReg, Imm)
  //! \overload
  INST_2i(sar, kX86InstIdSar, X86Mem, Imm)

  //! Find non-AL BYTE starting at ES:[EDI/RDI].
  INST_0x(scasb, kX86InstIdScasB)
  //! Find non-EAX DWORD starting at ES:[EDI/RDI].
  INST_0x(scasd, kX86InstIdScasD)
  //! Find non-rax QWORD starting at ES:[RDI] (X64 Only).
  INST_0x(scasq, kX86InstIdScasQ)
  //! Find non-AX WORD starting at ES:[EDI/RDI].
  INST_0x(scasw, kX86InstIdScasW)

  //! Set byte on condition.
  INST_1cc(set, kX86InstIdSet, X86Util::condToSetcc, X86GpReg)
  //! Set byte on condition.
  INST_1cc(set, kX86InstIdSet, X86Util::condToSetcc, X86Mem)

  //! Shift bits left.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(shl, kX86InstIdShl, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(shl, kX86InstIdShl, X86Mem, X86GpReg)
  //! Shift bits left.
  INST_2i(shl, kX86InstIdShl, X86GpReg, Imm)
  //! \overload
  INST_2i(shl, kX86InstIdShl, X86Mem, Imm)

  //! Shift bits right.
  //!
  //! \note `o1` register can be only `cl`.
  INST_2x(shr, kX86InstIdShr, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(shr, kX86InstIdShr, X86Mem, X86GpReg)
  //! Shift bits right.
  INST_2i(shr, kX86InstIdShr, X86GpReg, Imm)
  //! \overload
  INST_2i(shr, kX86InstIdShr, X86Mem, Imm)

  //! Double precision shift left.
  //!
  //! \note `o2` register can be only `cl` register.
  INST_3x(shld, kX86InstIdShld, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(shld, kX86InstIdShld, X86Mem, X86GpReg, X86GpReg)
  //! Double precision shift left.
  INST_3i(shld, kX86InstIdShld, X86GpReg, X86GpReg, Imm)
  //! \overload
  INST_3i(shld, kX86InstIdShld, X86Mem, X86GpReg, Imm)

  //! Double precision shift right.
  //!
  //! \note `o2` register can be only `cl` register.
  INST_3x(shrd, kX86InstIdShrd, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(shrd, kX86InstIdShrd, X86Mem, X86GpReg, X86GpReg)
  //! Double precision shift right.
  INST_3i(shrd, kX86InstIdShrd, X86GpReg, X86GpReg, Imm)
  //! \overload
  INST_3i(shrd, kX86InstIdShrd, X86Mem, X86GpReg, Imm)

  //! Set carry flag to 1.
  INST_0x(stc, kX86InstIdStc)
  //! Set direction flag to 1.
  INST_0x(std, kX86InstIdStd)

  //! Fill BYTE at ES:[EDI/RDI] with AL.
  INST_0x(stosb, kX86InstIdStosB)
  //! Fill DWORD at ES:[EDI/RDI] with EAX.
  INST_0x(stosd, kX86InstIdStosD)
  //! Fill QWORD at ES:[RDI] with RAX (X64 Only).
  INST_0x(stosq, kX86InstIdStosQ)
  //! Fill WORD at ES:[EDI/RDI] with AX.
  INST_0x(stosw, kX86InstIdStosW)

  //! Subtract.
  INST_2x(sub, kX86InstIdSub, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(sub, kX86InstIdSub, X86GpReg, X86Mem)
  //! \overload
  INST_2i(sub, kX86InstIdSub, X86GpReg, Imm)
  //! \overload
  INST_2x(sub, kX86InstIdSub, X86Mem, X86GpReg)
  //! \overload
  INST_2i(sub, kX86InstIdSub, X86Mem, Imm)

  //! Logical compare.
  INST_2x(test, kX86InstIdTest, X86GpReg, X86GpReg)
  //! \overload
  INST_2i(test, kX86InstIdTest, X86GpReg, Imm)
  //! \overload
  INST_2x(test, kX86InstIdTest, X86Mem, X86GpReg)
  //! \overload
  INST_2i(test, kX86InstIdTest, X86Mem, Imm)

  //! Undefined instruction - Raise #UD exception.
  INST_0x(ud2, kX86InstIdUd2)

  //! Exchange and Add.
  INST_2x(xadd, kX86InstIdXadd, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(xadd, kX86InstIdXadd, X86Mem, X86GpReg)

  //! Exchange register/memory with register.
  INST_2x(xchg, kX86InstIdXchg, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(xchg, kX86InstIdXchg, X86Mem, X86GpReg)
  //! \overload
  INST_2x(xchg, kX86InstIdXchg, X86GpReg, X86Mem)

  //! Xor.
  INST_2x(xor_, kX86InstIdXor, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(xor_, kX86InstIdXor, X86GpReg, X86Mem)
  //! \overload
  INST_2i(xor_, kX86InstIdXor, X86GpReg, Imm)
  //! \overload
  INST_2x(xor_, kX86InstIdXor, X86Mem, X86GpReg)
  //! \overload
  INST_2i(xor_, kX86InstIdXor, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [FPU]
  // --------------------------------------------------------------------------

  //! Compute `2^x - 1` - `fp0 = POW(2, fp0) - 1` (FPU).
  INST_0x(f2xm1, kX86InstIdF2xm1)
  //! Abs `fp0 = ABS(fp0)` (FPU).
  INST_0x(fabs, kX86InstIdFabs)

  //! Add `o0 = o0 + o1` (one operand has to be `fp0`) (FPU).
  INST_2x_(fadd, kX86InstIdFadd, X86FpReg, X86FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Add `fp0 = fp0 + float_or_double[o0]` (FPU).
  INST_1x(fadd, kX86InstIdFadd, X86Mem)
  //! Add `o0 = o0 + fp0` and POP (FPU).
  INST_1x(faddp, kX86InstIdFaddp, X86FpReg)
  //! Add `fp1 = fp1 + fp0` and POP (FPU).
  INST_0x(faddp, kX86InstIdFaddp)

  //! Load BCD from `[o0]` and PUSH (FPU).
  INST_1x(fbld, kX86InstIdFbld, X86Mem)
  //! Store BCD-Integer to `[o0]` and POP (FPU).
  INST_1x(fbstp, kX86InstIdFbstp, X86Mem)

  //! Complement Sign `fp0 = -fp0` (FPU).
  INST_0x(fchs, kX86InstIdFchs)

  //! Clear exceptions (FPU).
  INST_0x(fclex, kX86InstIdFclex)

  //! Conditional move `if (CF=1) fp0 = o0` (FPU).
  INST_1x(fcmovb, kX86InstIdFcmovb, X86FpReg)
  //! Conditional move `if (CF|ZF=1) fp0 = o0` (FPU).
  INST_1x(fcmovbe, kX86InstIdFcmovbe, X86FpReg)
  //! Conditional move `if (ZF=1) fp0 = o0` (FPU).
  INST_1x(fcmove, kX86InstIdFcmove, X86FpReg)
  //! Conditional move `if (CF=0) fp0 = o0` (FPU).
  INST_1x(fcmovnb, kX86InstIdFcmovnb, X86FpReg)
  //! Conditional move `if (CF|ZF=0) fp0 = o0` (FPU).
  INST_1x(fcmovnbe, kX86InstIdFcmovnbe, X86FpReg)
  //! Conditional move `if (ZF=0) fp0 = o0` (FPU).
  INST_1x(fcmovne, kX86InstIdFcmovne, X86FpReg)
  //! Conditional move `if (PF=0) fp0 = o0` (FPU).
  INST_1x(fcmovnu, kX86InstIdFcmovnu, X86FpReg)
  //! Conditional move `if (PF=1) fp0 = o0` (FPU).
  INST_1x(fcmovu, kX86InstIdFcmovu, X86FpReg)

  //! Compare `fp0` with `o0` (FPU).
  INST_1x(fcom, kX86InstIdFcom, X86FpReg)
  //! Compare `fp0` with `fp1` (FPU).
  INST_0x(fcom, kX86InstIdFcom)
  //! Compare `fp0` with `float_or_double[o0]` (FPU).
  INST_1x(fcom, kX86InstIdFcom, X86Mem)
  //! Compare `fp0` with `o0` and POP (FPU).
  INST_1x(fcomp, kX86InstIdFcomp, X86FpReg)
  //! Compare `fp0` with `fp1` and POP (FPU).
  INST_0x(fcomp, kX86InstIdFcomp)
  //! Compare `fp0` with `float_or_double[o0]` and POP (FPU).
  INST_1x(fcomp, kX86InstIdFcomp, X86Mem)
  //! Compare `fp0` with `fp1` and POP twice (FPU).
  INST_0x(fcompp, kX86InstIdFcompp)
  //! Compare `fp0` with `o0` and set EFLAGS (FPU).
  INST_1x(fcomi, kX86InstIdFcomi, X86FpReg)
  //! Compare `fp0` with `o0` and set EFLAGS and POP (FPU).
  INST_1x(fcomip, kX86InstIdFcomip, X86FpReg)

  //! Cos `fp0 = cos(fp0)` (FPU).
  INST_0x(fcos, kX86InstIdFcos)

  //! Decrement FPU stack pointer (FPU).
  INST_0x(fdecstp, kX86InstIdFdecstp)

  //! Divide `o0 = o0 / o1` (one has to be `fp0`) (FPU).
  INST_2x_(fdiv, kX86InstIdFdiv, X86FpReg, X86FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Divide `fp0 = fp0 / float_or_double[o0]` (FPU).
  INST_1x(fdiv, kX86InstIdFdiv, X86Mem)
  //! Divide `o0 = o0 / fp0` and POP (FPU).
  INST_1x(fdivp, kX86InstIdFdivp, X86FpReg)
  //! Divide `fp1 = fp1 / fp0` and POP (FPU).
  INST_0x(fdivp, kX86InstIdFdivp)

  //! Reverse divide `o0 = o1 / o0` (one has to be `fp0`) (FPU).
  INST_2x_(fdivr, kX86InstIdFdivr, X86FpReg, X86FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Reverse divide `fp0 = float_or_double[o0] / fp0` (FPU).
  INST_1x(fdivr, kX86InstIdFdivr, X86Mem)
  //! Reverse divide `o0 = fp0 / o0` and POP (FPU).
  INST_1x(fdivrp, kX86InstIdFdivrp, X86FpReg)
  //! Reverse divide `fp1 = fp0 / fp1` and POP (FPU).
  INST_0x(fdivrp, kX86InstIdFdivrp)

  //! Free FP register (FPU).
  INST_1x(ffree, kX86InstIdFfree, X86FpReg)

  //! Add `fp0 = fp0 + short_or_int[o0]` (FPU).
  INST_1x_(fiadd, kX86InstIdFiadd, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Compare `fp0` with `short_or_int[o0]` (FPU).
  INST_1x_(ficom, kX86InstIdFicom, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Compare `fp0` with `short_or_int[o0]` and POP (FPU).
  INST_1x_(ficomp, kX86InstIdFicomp, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Divide `fp0 = fp0 / short_or_int[o0]` (FPU).
  INST_1x_(fidiv, kX86InstIdFidiv, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Reverse divide `fp0 = short_or_int[o0] / fp0` (FPU).
  INST_1x_(fidivr, kX86InstIdFidivr, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Load `short_or_int_or_long[o0]` and PUSH (FPU).
  INST_1x_(fild, kX86InstIdFild, X86Mem, o0.getSize() == 2 || o0.getSize() == 4 || o0.getSize() == 8)
  //! Multiply `fp0 *= short_or_int[o0]` (FPU).
  INST_1x_(fimul, kX86InstIdFimul, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Increment FPU stack pointer (FPU).
  INST_0x(fincstp, kX86InstIdFincstp)
  //! Initialize FPU (FPU).
  INST_0x(finit, kX86InstIdFinit)

  //! Subtract `fp0 = fp0 - short_or_int[o0]` (FPU).
  INST_1x_(fisub, kX86InstIdFisub, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Reverse subtract `fp0 = short_or_int[o0] - fp0` (FPU).
  INST_1x_(fisubr, kX86InstIdFisubr, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)

  //! Initialize FPU without checking for pending unmasked exceptions (FPU).
  INST_0x(fninit, kX86InstIdFninit)

  //! Store `fp0` as `short_or_int[o0]` (FPU).
  INST_1x_(fist, kX86InstIdFist, X86Mem, o0.getSize() == 2 || o0.getSize() == 4)
  //! Store `fp0` as `short_or_int_or_long[o0]` and POP (FPU).
  INST_1x_(fistp, kX86InstIdFistp, X86Mem, o0.getSize() == 2 || o0.getSize() == 4 || o0.getSize() == 8)

  //! Load `float_or_double_or_extended[o0]` and PUSH (FPU).
  INST_1x_(fld, kX86InstIdFld, X86Mem, o0.getSize() == 4 || o0.getSize() == 8 || o0.getSize() == 10)
  //! PUSH `o0` (FPU).
  INST_1x(fld, kX86InstIdFld, X86FpReg)

  //! PUSH `1.0` (FPU).
  INST_0x(fld1, kX86InstIdFld1)
  //! PUSH `log2(10)` (FPU).
  INST_0x(fldl2t, kX86InstIdFldl2t)
  //! PUSH `log2(e)` (FPU).
  INST_0x(fldl2e, kX86InstIdFldl2e)
  //! PUSH `pi` (FPU).
  INST_0x(fldpi, kX86InstIdFldpi)
  //! PUSH `log10(2)` (FPU).
  INST_0x(fldlg2, kX86InstIdFldlg2)
  //! PUSH `ln(2)` (FPU).
  INST_0x(fldln2, kX86InstIdFldln2)
  //! PUSH `+0.0` (FPU).
  INST_0x(fldz, kX86InstIdFldz)

  //! Load x87 FPU control word from `word_ptr[o0]` (FPU).
  INST_1x(fldcw, kX86InstIdFldcw, X86Mem)
  //! Load x87 FPU environment (14 or 28 bytes) from `[o0]` (FPU).
  INST_1x(fldenv, kX86InstIdFldenv, X86Mem)

  //! Multiply `o0 = o0  * o1` (one has to be `fp0`) (FPU).
  INST_2x_(fmul, kX86InstIdFmul, X86FpReg, X86FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Multiply `fp0 = fp0 * float_or_double[o0]` (FPU).
  INST_1x(fmul, kX86InstIdFmul, X86Mem)
  //! Multiply `o0 = o0 * fp0` and POP (FPU).
  INST_1x(fmulp, kX86InstIdFmulp, X86FpReg)
  //! Multiply `fp1 = fp1 * fp0` and POP (FPU).
  INST_0x(fmulp, kX86InstIdFmulp)

  //! Clear exceptions (FPU).
  INST_0x(fnclex, kX86InstIdFnclex)
  //! No operation (FPU).
  INST_0x(fnop, kX86InstIdFnop)
  //! Save FPU state to `[o0]` (FPU).
  INST_1x(fnsave, kX86InstIdFnsave, X86Mem)
  //! Store x87 FPU environment to `[o0]` (FPU).
  INST_1x(fnstenv, kX86InstIdFnstenv, X86Mem)
  //! Store x87 FPU control word to `[o0]` (FPU).
  INST_1x(fnstcw, kX86InstIdFnstcw, X86Mem)

  //! Store x87 FPU status word to `o0` (AX) (FPU).
  INST_1x_(fnstsw, kX86InstIdFnstsw, X86GpReg, o0.isRegCode(kX86RegTypeGpw, kX86RegIndexAx))
  //! Store x87 FPU status word to `word_ptr[o0]` (FPU).
  INST_1x(fnstsw, kX86InstIdFnstsw, X86Mem)

  //! Partial Arctan `fp1 = atan2(fp1, fp0)` and POP (FPU).
  INST_0x(fpatan, kX86InstIdFpatan)
  //! Partial Remainder[Trunc] `fp1 = fp0 % fp1` and POP (FPU).
  INST_0x(fprem, kX86InstIdFprem)
  //! Partial Remainder[Round] `fp1 = fp0 % fp1` and POP (FPU).
  INST_0x(fprem1, kX86InstIdFprem1)
  //! Partial Tan `fp0 = tan(fp0)` and PUSH `1.0` (FPU).
  INST_0x(fptan, kX86InstIdFptan)
  //! Round `fp0 = round(fp0)` (FPU).
  INST_0x(frndint, kX86InstIdFrndint)

  //! Restore FPU state from `[o0]` (94 or 108 bytes) (FPU).
  INST_1x(frstor, kX86InstIdFrstor, X86Mem)
  //! Save FPU state to `[o0]` (94 or 108 bytes) (FPU).
  INST_1x(fsave, kX86InstIdFsave, X86Mem)

  //! Scale `fp0 = fp0 * pow(2, RoundTowardsZero(fp1))` (FPU).
  INST_0x(fscale, kX86InstIdFscale)
  //! Sin `fp0 = sin(fp0)` (FPU).
  INST_0x(fsin, kX86InstIdFsin)
  //! Sincos `fp0 = sin(fp0)` and PUSH `cos(fp0)` (FPU).
  INST_0x(fsincos, kX86InstIdFsincos)
  //! Square root `fp0 = sqrt(fp0)` (FPU).
  INST_0x(fsqrt, kX86InstIdFsqrt)

  //! Store floating point value to `float_or_double[o0]` (FPU).
  INST_1x_(fst, kX86InstIdFst, X86Mem, o0.getSize() == 4 || o0.getSize() == 8)
  //! Copy `o0 = fp0` (FPU).
  INST_1x(fst, kX86InstIdFst, X86FpReg)
  //! Store floating point value to `float_or_double_or_extended[o0]` and POP (FPU).
  INST_1x_(fstp, kX86InstIdFstp, X86Mem, o0.getSize() == 4 || o0.getSize() == 8 || o0.getSize() == 10)
  //! Copy `o0 = fp0` and POP (FPU).
  INST_1x(fstp, kX86InstIdFstp, X86FpReg)

  //! Store x87 FPU control word to `word_ptr[o0]` (FPU).
  INST_1x(fstcw, kX86InstIdFstcw, X86Mem)
  //! Store x87 FPU environment to `[o0]` (14 or 28 bytes) (FPU).
  INST_1x(fstenv, kX86InstIdFstenv, X86Mem)
  //! Store x87 FPU status word to `o0` (AX) (FPU).
  INST_1x_(fstsw, kX86InstIdFstsw, X86GpReg, o0.getRegIndex() == kX86RegIndexAx)
  //! Store x87 FPU status word to `word_ptr[o0]` (FPU).
  INST_1x(fstsw, kX86InstIdFstsw, X86Mem)

  //! Subtract `o0 = o0 - o1` (one has to be `fp0`) (FPU).
  INST_2x_(fsub, kX86InstIdFsub, X86FpReg, X86FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Subtract `fp0 = fp0 - float_or_double[o0]` (FPU).
  INST_1x_(fsub, kX86InstIdFsub, X86Mem, o0.getSize() == 4 || o0.getSize() == 8)
  //! Subtract `o0 = o0 - fp0` and POP (FPU).
  INST_1x(fsubp, kX86InstIdFsubp, X86FpReg)
  //! Subtract `fp1 = fp1 - fp0` and POP (FPU).
  INST_0x(fsubp, kX86InstIdFsubp)

  //! Reverse subtract `o0 = o1 - o0` (one has to be `fp0`) (FPU).
  INST_2x_(fsubr, kX86InstIdFsubr, X86FpReg, X86FpReg, o0.getRegIndex() == 0 || o1.getRegIndex() == 0)
  //! Reverse subtract `fp0 = fp0 - float_or_double[o0]` (FPU).
  INST_1x_(fsubr, kX86InstIdFsubr, X86Mem, o0.getSize() == 4 || o0.getSize() == 8)
  //! Reverse subtract `o0 = o0 - fp0` and POP (FPU).
  INST_1x(fsubrp, kX86InstIdFsubrp, X86FpReg)
  //! Reverse subtract `fp1 = fp1 - fp0` and POP (FPU).
  INST_0x(fsubrp, kX86InstIdFsubrp)

  //! Compare `fp0` with `0.0` (FPU).
  INST_0x(ftst, kX86InstIdFtst)

  //! Unordered compare `fp0` with `o0` (FPU).
  INST_1x(fucom, kX86InstIdFucom, X86FpReg)
  //! Unordered compare `fp0` with `fp1` (FPU).
  INST_0x(fucom, kX86InstIdFucom)
  //! Unordered compare `fp0` with `o0`, check for ordered values and set EFLAGS (FPU).
  INST_1x(fucomi, kX86InstIdFucomi, X86FpReg)
  //! Unordered compare `fp0` with `o0`, check for ordered values and set EFLAGS and POP (FPU).
  INST_1x(fucomip, kX86InstIdFucomip, X86FpReg)
  //! Unordered compare `fp0` with `o0` and POP (FPU).
  INST_1x(fucomp, kX86InstIdFucomp, X86FpReg)
  //! Unordered compare `fp0` with `fp1` and POP (FPU).
  INST_0x(fucomp, kX86InstIdFucomp)
  //! Unordered compare `fp0` with `fp1` and POP twice (FPU).
  INST_0x(fucompp, kX86InstIdFucompp)

  INST_0x(fwait, kX86InstIdFwait)

  //! Examine fp0 (FPU).
  INST_0x(fxam, kX86InstIdFxam)
  //! Exchange `fp0` with `o0` (FPU).
  INST_1x(fxch, kX86InstIdFxch, X86FpReg)

  //! Restore FP/MMX/SIMD extension states to `o0` (512 bytes) (FPU, MMX, SSE).
  INST_1x(fxrstor, kX86InstIdFxrstor, X86Mem)
  //! Store FP/MMX/SIMD extension states to `o0` (512 bytes) (FPU, MMX, SSE).
  INST_1x(fxsave, kX86InstIdFxsave, X86Mem)
  //! Extract `fp0 = exponent(fp0)` and PUSH `significant(fp0)` (FPU).
  INST_0x(fxtract, kX86InstIdFxtract)

  //! Compute `fp1 = fp1 * log2(fp0)` and POP (FPU).
  INST_0x(fyl2x, kX86InstIdFyl2x)
  //! Compute `fp1 = fp1 * log2(fp0 + 1)` and POP (FPU).
  INST_0x(fyl2xp1, kX86InstIdFyl2xp1)

  // --------------------------------------------------------------------------
  // [MMX]
  // --------------------------------------------------------------------------

  //! Move DWORD (MMX).
  INST_2x(movd, kX86InstIdMovd, X86Mem, X86MmReg)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86GpReg, X86MmReg)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86MmReg, X86Mem)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86MmReg, X86GpReg)

  //! Move QWORD (MMX).
  INST_2x(movq, kX86InstIdMovq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86Mem, X86MmReg)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86MmReg, X86Mem)

  //! Move QWORD (X64 Only).
  INST_2x(movq, kX86InstIdMovq, X86GpReg, X86MmReg)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86MmReg, X86GpReg)

  //! Pack DWORDs to WORDs with signed saturation (MMX).
  INST_2x(packssdw, kX86InstIdPackssdw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(packssdw, kX86InstIdPackssdw, X86MmReg, X86Mem)

  //! Pack WORDs to BYTEs with signed saturation (MMX).
  INST_2x(packsswb, kX86InstIdPacksswb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(packsswb, kX86InstIdPacksswb, X86MmReg, X86Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (MMX).
  INST_2x(packuswb, kX86InstIdPackuswb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(packuswb, kX86InstIdPackuswb, X86MmReg, X86Mem)

  //! Packed BYTE add (MMX).
  INST_2x(paddb, kX86InstIdPaddb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddb, kX86InstIdPaddb, X86MmReg, X86Mem)

  //! Packed DWORD add (MMX).
  INST_2x(paddd, kX86InstIdPaddd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddd, kX86InstIdPaddd, X86MmReg, X86Mem)

  //! Packed BYTE add with saturation (MMX).
  INST_2x(paddsb, kX86InstIdPaddsb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddsb, kX86InstIdPaddsb, X86MmReg, X86Mem)

  //! Packed WORD add with saturation (MMX).
  INST_2x(paddsw, kX86InstIdPaddsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddsw, kX86InstIdPaddsw, X86MmReg, X86Mem)

  //! Packed BYTE add with unsigned saturation (MMX).
  INST_2x(paddusb, kX86InstIdPaddusb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddusb, kX86InstIdPaddusb, X86MmReg, X86Mem)

  //! Packed WORD add with unsigned saturation (MMX).
  INST_2x(paddusw, kX86InstIdPaddusw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddusw, kX86InstIdPaddusw, X86MmReg, X86Mem)

  //! Packed WORD add (MMX).
  INST_2x(paddw, kX86InstIdPaddw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddw, kX86InstIdPaddw, X86MmReg, X86Mem)

  //! Packed bitwise and (MMX).
  INST_2x(pand, kX86InstIdPand, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pand, kX86InstIdPand, X86MmReg, X86Mem)

  //! Packed bitwise and-not (MMX).
  INST_2x(pandn, kX86InstIdPandn, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pandn, kX86InstIdPandn, X86MmReg, X86Mem)

  //! Packed BYTEs compare for equality (MMX).
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86MmReg, X86Mem)

  //! Packed DWORDs compare for equality (MMX).
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86MmReg, X86Mem)

  //! Packed WORDs compare for equality (MMX).
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86MmReg, X86Mem)

  //! Packed BYTEs compare if greater than (MMX).
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86MmReg, X86Mem)

  //! Packed DWORDs compare if greater than (MMX).
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86MmReg, X86Mem)

  //! Packed WORDs compare if greater than (MMX).
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86MmReg, X86Mem)

  //! Packed WORDs multiply high (MMX).
  INST_2x(pmulhw, kX86InstIdPmulhw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmulhw, kX86InstIdPmulhw, X86MmReg, X86Mem)

  //! Packed WORDs multiply low (MMX).
  INST_2x(pmullw, kX86InstIdPmullw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmullw, kX86InstIdPmullw, X86MmReg, X86Mem)

  //! Pakced bitwise or (MMX).
  INST_2x(por, kX86InstIdPor, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(por, kX86InstIdPor, X86MmReg, X86Mem)

  //! Packed WORD multiply and add to packed DWORD (MMX).
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86MmReg, X86Mem)

  //! Packed DWORD shift left logical (MMX).
  INST_2x(pslld, kX86InstIdPslld, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pslld, kX86InstIdPslld, X86MmReg, X86Mem)
  //! \overload
  INST_2i(pslld, kX86InstIdPslld, X86MmReg, Imm)

  //! Packed QWORD shift left logical (MMX).
  INST_2x(psllq, kX86InstIdPsllq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psllq, kX86InstIdPsllq, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psllq, kX86InstIdPsllq, X86MmReg, Imm)

  //! Packed WORD shift left logical (MMX).
  INST_2x(psllw, kX86InstIdPsllw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psllw, kX86InstIdPsllw, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psllw, kX86InstIdPsllw, X86MmReg, Imm)

  //! Packed DWORD shift right arithmetic (MMX).
  INST_2x(psrad, kX86InstIdPsrad, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psrad, kX86InstIdPsrad, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psrad, kX86InstIdPsrad, X86MmReg, Imm)

  //! Packed WORD shift right arithmetic (MMX).
  INST_2x(psraw, kX86InstIdPsraw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psraw, kX86InstIdPsraw, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psraw, kX86InstIdPsraw, X86MmReg, Imm)

  //! Packed DWORD shift right logical (MMX).
  INST_2x(psrld, kX86InstIdPsrld, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psrld, kX86InstIdPsrld, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psrld, kX86InstIdPsrld, X86MmReg, Imm)

  //! Packed QWORD shift right logical (MMX).
  INST_2x(psrlq, kX86InstIdPsrlq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psrlq, kX86InstIdPsrlq, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psrlq, kX86InstIdPsrlq, X86MmReg, Imm)

  //! Packed WORD shift right logical (MMX).
  INST_2x(psrlw, kX86InstIdPsrlw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psrlw, kX86InstIdPsrlw, X86MmReg, X86Mem)
  //! \overload
  INST_2i(psrlw, kX86InstIdPsrlw, X86MmReg, Imm)

  //! Packed BYTE subtract (MMX).
  INST_2x(psubb, kX86InstIdPsubb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubb, kX86InstIdPsubb, X86MmReg, X86Mem)

  //! Packed DWORD subtract (MMX).
  INST_2x(psubd, kX86InstIdPsubd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubd, kX86InstIdPsubd, X86MmReg, X86Mem)

  //! Packed BYTE subtract with saturation (MMX).
  INST_2x(psubsb, kX86InstIdPsubsb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubsb, kX86InstIdPsubsb, X86MmReg, X86Mem)

  //! Packed WORD subtract with saturation (MMX).
  INST_2x(psubsw, kX86InstIdPsubsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubsw, kX86InstIdPsubsw, X86MmReg, X86Mem)

  //! Packed BYTE subtract with unsigned saturation (MMX).
  INST_2x(psubusb, kX86InstIdPsubusb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubusb, kX86InstIdPsubusb, X86MmReg, X86Mem)

  //! Packed WORD subtract with unsigned saturation (MMX).
  INST_2x(psubusw, kX86InstIdPsubusw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubusw, kX86InstIdPsubusw, X86MmReg, X86Mem)

  //! Packed WORD subtract (MMX).
  INST_2x(psubw, kX86InstIdPsubw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubw, kX86InstIdPsubw, X86MmReg, X86Mem)

  //! Unpack high packed BYTEs to WORDs (MMX).
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86MmReg, X86Mem)

  //! Unpack high packed DWORDs to QWORDs (MMX).
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86MmReg, X86Mem)

  //! Unpack high packed WORDs to DWORDs (MMX).
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86MmReg, X86Mem)

  //! Unpack low packed BYTEs to WORDs (MMX).
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86MmReg, X86Mem)

  //! Unpack low packed DWORDs to QWORDs (MMX).
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86MmReg, X86Mem)

  //! Unpack low packed WORDs to DWORDs (MMX).
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86MmReg, X86Mem)

  //! Packed bitwise xor (MMX).
  INST_2x(pxor, kX86InstIdPxor, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pxor, kX86InstIdPxor, X86MmReg, X86Mem)

  //! Empty MMX state.
  INST_0x(emms, kX86InstIdEmms)

  // -------------------------------------------------------------------------
  // [3dNow]
  // -------------------------------------------------------------------------

  //! Packed SP-FP to DWORD convert (3dNow!).
  INST_2x(pf2id, kX86InstIdPf2id, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pf2id, kX86InstIdPf2id, X86MmReg, X86Mem)

  //!  Packed SP-FP to WORD convert (3dNow!).
  INST_2x(pf2iw, kX86InstIdPf2iw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pf2iw, kX86InstIdPf2iw, X86MmReg, X86Mem)

  //! Packed SP-FP accumulate (3dNow!).
  INST_2x(pfacc, kX86InstIdPfacc, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfacc, kX86InstIdPfacc, X86MmReg, X86Mem)

  //! Packed SP-FP addition (3dNow!).
  INST_2x(pfadd, kX86InstIdPfadd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfadd, kX86InstIdPfadd, X86MmReg, X86Mem)

  //! Packed SP-FP compare - dst == src (3dNow!).
  INST_2x(pfcmpeq, kX86InstIdPfcmpeq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfcmpeq, kX86InstIdPfcmpeq, X86MmReg, X86Mem)

  //! Packed SP-FP compare - dst >= src (3dNow!).
  INST_2x(pfcmpge, kX86InstIdPfcmpge, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfcmpge, kX86InstIdPfcmpge, X86MmReg, X86Mem)

  //! Packed SP-FP compare - dst > src (3dNow!).
  INST_2x(pfcmpgt, kX86InstIdPfcmpgt, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfcmpgt, kX86InstIdPfcmpgt, X86MmReg, X86Mem)

  //! Packed SP-FP maximum (3dNow!).
  INST_2x(pfmax, kX86InstIdPfmax, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfmax, kX86InstIdPfmax, X86MmReg, X86Mem)

  //! Packed SP-FP minimum (3dNow!).
  INST_2x(pfmin, kX86InstIdPfmin, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfmin, kX86InstIdPfmin, X86MmReg, X86Mem)

  //! Packed SP-FP multiply (3dNow!).
  INST_2x(pfmul, kX86InstIdPfmul, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfmul, kX86InstIdPfmul, X86MmReg, X86Mem)

  //! Packed SP-FP negative accumulate (3dNow!).
  INST_2x(pfnacc, kX86InstIdPfnacc, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfnacc, kX86InstIdPfnacc, X86MmReg, X86Mem)

  //! Packed SP-FP mixed accumulate (3dNow!).
  INST_2x(pfpnacc, kX86InstIdPfpnacc, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfpnacc, kX86InstIdPfpnacc, X86MmReg, X86Mem)

  //! Packed SP-FP reciprocal Approximation (3dNow!).
  INST_2x(pfrcp, kX86InstIdPfrcp, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfrcp, kX86InstIdPfrcp, X86MmReg, X86Mem)

  //! Packed SP-FP reciprocal, first iteration step (3dNow!).
  INST_2x(pfrcpit1, kX86InstIdPfrcpit1, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfrcpit1, kX86InstIdPfrcpit1, X86MmReg, X86Mem)

  //! Packed SP-FP reciprocal, second iteration step (3dNow!).
  INST_2x(pfrcpit2, kX86InstIdPfrcpit2, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfrcpit2, kX86InstIdPfrcpit2, X86MmReg, X86Mem)

  //! Packed SP-FP reciprocal square root, first iteration step (3dNow!).
  INST_2x(pfrsqit1, kX86InstIdPfrsqit1, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfrsqit1, kX86InstIdPfrsqit1, X86MmReg, X86Mem)

  //! Packed SP-FP reciprocal square root approximation (3dNow!).
  INST_2x(pfrsqrt, kX86InstIdPfrsqrt, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfrsqrt, kX86InstIdPfrsqrt, X86MmReg, X86Mem)

  //! Packed SP-FP subtract (3dNow!).
  INST_2x(pfsub, kX86InstIdPfsub, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfsub, kX86InstIdPfsub, X86MmReg, X86Mem)

  //! Packed SP-FP reverse subtract (3dNow!).
  INST_2x(pfsubr, kX86InstIdPfsubr, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pfsubr, kX86InstIdPfsubr, X86MmReg, X86Mem)

  //! Packed DWORDs to SP-FP (3dNow!).
  INST_2x(pi2fd, kX86InstIdPi2fd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pi2fd, kX86InstIdPi2fd, X86MmReg, X86Mem)

  //! Packed WORDs to SP-FP (3dNow!).
  INST_2x(pi2fw, kX86InstIdPi2fw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pi2fw, kX86InstIdPi2fw, X86MmReg, X86Mem)

  //! Packed swap DWORDs (3dNow!)
  INST_2x(pswapd, kX86InstIdPswapd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pswapd, kX86InstIdPswapd, X86MmReg, X86Mem)

  //! Prefetch (3dNow!).
  INST_1x(prefetch3dnow, kX86InstIdPrefetch3dNow, X86Mem)

  //! Prefetch and set cache to modified (3dNow!).
  INST_1x(prefetchw3dnow, kX86InstIdPrefetchw3dNow, X86Mem)

  //! Faster EMMS (3dNow!).
  INST_0x(femms, kX86InstIdFemms)

  // --------------------------------------------------------------------------
  // [SSE]
  // --------------------------------------------------------------------------

  //! Packed SP-FP add (SSE).
  INST_2x(addps, kX86InstIdAddps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(addps, kX86InstIdAddps, X86XmmReg, X86Mem)

  //! Scalar SP-FP add (SSE).
  INST_2x(addss, kX86InstIdAddss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(addss, kX86InstIdAddss, X86XmmReg, X86Mem)

  //! Packed SP-FP bitwise and-not (SSE).
  INST_2x(andnps, kX86InstIdAndnps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(andnps, kX86InstIdAndnps, X86XmmReg, X86Mem)

  //! Packed SP-FP bitwise and (SSE).
  INST_2x(andps, kX86InstIdAndps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(andps, kX86InstIdAndps, X86XmmReg, X86Mem)

  //! Packed SP-FP compare (SSE).
  INST_3i(cmpps, kX86InstIdCmpps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(cmpps, kX86InstIdCmpps, X86XmmReg, X86Mem, Imm)

  //! Compare scalar SP-FP (SSE).
  INST_3i(cmpss, kX86InstIdCmpss, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(cmpss, kX86InstIdCmpss, X86XmmReg, X86Mem, Imm)

  //! Scalar ordered SP-FP compare and set EFLAGS (SSE).
  INST_2x(comiss, kX86InstIdComiss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(comiss, kX86InstIdComiss, X86XmmReg, X86Mem)

  //! Packed signed INT32 to packed SP-FP conversion (SSE).
  INST_2x(cvtpi2ps, kX86InstIdCvtpi2ps, X86XmmReg, X86MmReg)
  //! \overload
  INST_2x(cvtpi2ps, kX86InstIdCvtpi2ps, X86XmmReg, X86Mem)

  //! Packed SP-FP to packed INT32 conversion (SSE).
  INST_2x(cvtps2pi, kX86InstIdCvtps2pi, X86MmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtps2pi, kX86InstIdCvtps2pi, X86MmReg, X86Mem)

  //! Convert scalar INT32 to SP-FP (SSE).
  INST_2x(cvtsi2ss, kX86InstIdCvtsi2ss, X86XmmReg, X86GpReg)
  //! \overload
  INST_2x(cvtsi2ss, kX86InstIdCvtsi2ss, X86XmmReg, X86Mem)

  //! Convert scalar SP-FP to INT32 (SSE).
  INST_2x(cvtss2si, kX86InstIdCvtss2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(cvtss2si, kX86InstIdCvtss2si, X86GpReg, X86Mem)

  //! Convert with truncation packed SP-FP to packed INT32 (SSE).
  INST_2x(cvttps2pi, kX86InstIdCvttps2pi, X86MmReg, X86XmmReg)
  //! \overload
  INST_2x(cvttps2pi, kX86InstIdCvttps2pi, X86MmReg, X86Mem)

  //! Convert with truncation scalar SP-FP to INT32 (SSE).
  INST_2x(cvttss2si, kX86InstIdCvttss2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(cvttss2si, kX86InstIdCvttss2si, X86GpReg, X86Mem)

  //! Packed SP-FP divide (SSE).
  INST_2x(divps, kX86InstIdDivps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(divps, kX86InstIdDivps, X86XmmReg, X86Mem)

  //! Scalar SP-FP divide (SSE).
  INST_2x(divss, kX86InstIdDivss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(divss, kX86InstIdDivss, X86XmmReg, X86Mem)

  //! Load streaming SIMD extension control/status (SSE).
  INST_1x(ldmxcsr, kX86InstIdLdmxcsr, X86Mem)

  //! Byte mask write to DS:EDI/RDI (SSE).
  INST_2x(maskmovq, kX86InstIdMaskmovq, X86MmReg, X86MmReg)

  //! Packed SP-FP maximum (SSE).
  INST_2x(maxps, kX86InstIdMaxps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(maxps, kX86InstIdMaxps, X86XmmReg, X86Mem)

  //! Scalar SP-FP maximum (SSE).
  INST_2x(maxss, kX86InstIdMaxss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(maxss, kX86InstIdMaxss, X86XmmReg, X86Mem)

  //! Packed SP-FP minimum (SSE).
  INST_2x(minps, kX86InstIdMinps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(minps, kX86InstIdMinps, X86XmmReg, X86Mem)

  //! Scalar SP-FP minimum (SSE).
  INST_2x(minss, kX86InstIdMinss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(minss, kX86InstIdMinss, X86XmmReg, X86Mem)

  //! Move aligned packed SP-FP (SSE).
  INST_2x(movaps, kX86InstIdMovaps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movaps, kX86InstIdMovaps, X86XmmReg, X86Mem)
  //! Move aligned packed SP-FP (SSE).
  INST_2x(movaps, kX86InstIdMovaps, X86Mem, X86XmmReg)

  //! Move DWORD.
  INST_2x(movd, kX86InstIdMovd, X86Mem, X86XmmReg)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86XmmReg, X86GpReg)

  //! Move QWORD (SSE).
  INST_2x(movq, kX86InstIdMovq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86Mem, X86XmmReg)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86XmmReg, X86Mem)

  //! Move QWORD (X64 Only).
  INST_2x(movq, kX86InstIdMovq, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86XmmReg, X86GpReg)

  //! Move QWORD using NT hint (SSE).
  INST_2x(movntq, kX86InstIdMovntq, X86Mem, X86MmReg)

  //! Move high to low packed SP-FP (SSE).
  INST_2x(movhlps, kX86InstIdMovhlps, X86XmmReg, X86XmmReg)

  //! Move high packed SP-FP (SSE).
  INST_2x(movhps, kX86InstIdMovhps, X86XmmReg, X86Mem)
  //! Move high packed SP-FP (SSE).
  INST_2x(movhps, kX86InstIdMovhps, X86Mem, X86XmmReg)

  //! Move low to high packed SP-FP (SSE).
  INST_2x(movlhps, kX86InstIdMovlhps, X86XmmReg, X86XmmReg)

  //! Move low packed SP-FP (SSE).
  INST_2x(movlps, kX86InstIdMovlps, X86XmmReg, X86Mem)
  //! Move low packed SP-FP (SSE).
  INST_2x(movlps, kX86InstIdMovlps, X86Mem, X86XmmReg)

  //! Move aligned packed SP-FP using NT hint (SSE).
  INST_2x(movntps, kX86InstIdMovntps, X86Mem, X86XmmReg)

  //! Move scalar SP-FP (SSE).
  INST_2x(movss, kX86InstIdMovss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movss, kX86InstIdMovss, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movss, kX86InstIdMovss, X86Mem, X86XmmReg)

  //! Move unaligned packed SP-FP (SSE).
  INST_2x(movups, kX86InstIdMovups, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movups, kX86InstIdMovups, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movups, kX86InstIdMovups, X86Mem, X86XmmReg)

  //! Packed SP-FP multiply (SSE).
  INST_2x(mulps, kX86InstIdMulps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(mulps, kX86InstIdMulps, X86XmmReg, X86Mem)

  //! Scalar SP-FP multiply (SSE).
  INST_2x(mulss, kX86InstIdMulss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(mulss, kX86InstIdMulss, X86XmmReg, X86Mem)

  //! Packed SP-FP bitwise or (SSE).
  INST_2x(orps, kX86InstIdOrps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(orps, kX86InstIdOrps, X86XmmReg, X86Mem)

  //! Packed BYTE average (SSE).
  INST_2x(pavgb, kX86InstIdPavgb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pavgb, kX86InstIdPavgb, X86MmReg, X86Mem)

  //! Packed WORD average (SSE).
  INST_2x(pavgw, kX86InstIdPavgw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pavgw, kX86InstIdPavgw, X86MmReg, X86Mem)

  //! Extract WORD based on selector (SSE).
  INST_3i(pextrw, kX86InstIdPextrw, X86GpReg, X86MmReg, Imm)

  //! Insert WORD based on selector (SSE).
  INST_3i(pinsrw, kX86InstIdPinsrw, X86MmReg, X86GpReg, Imm)
  //! \overload
  INST_3i(pinsrw, kX86InstIdPinsrw, X86MmReg, X86Mem, Imm)

  //! Packed WORD maximum (SSE).
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86MmReg, X86Mem)

  //! Packed BYTE unsigned maximum (SSE).
  INST_2x(pmaxub, kX86InstIdPmaxub, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmaxub, kX86InstIdPmaxub, X86MmReg, X86Mem)

  //! Packed WORD minimum (SSE).
  INST_2x(pminsw, kX86InstIdPminsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pminsw, kX86InstIdPminsw, X86MmReg, X86Mem)

  //! Packed BYTE unsigned minimum (SSE).
  INST_2x(pminub, kX86InstIdPminub, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pminub, kX86InstIdPminub, X86MmReg, X86Mem)

  //! Move Byte mask to integer (SSE).
  INST_2x(pmovmskb, kX86InstIdPmovmskb, X86GpReg, X86MmReg)

  //! Packed WORD unsigned multiply high (SSE).
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86MmReg, X86Mem)

  //! Packed WORD sum of absolute differences (SSE).
  INST_2x(psadbw, kX86InstIdPsadbw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psadbw, kX86InstIdPsadbw, X86MmReg, X86Mem)

  //! Packed WORD shuffle (SSE).
  INST_3i(pshufw, kX86InstIdPshufw, X86MmReg, X86MmReg, Imm)
  //! \overload
  INST_3i(pshufw, kX86InstIdPshufw, X86MmReg, X86Mem, Imm)

  //! Packed SP-FP reciprocal (SSE).
  INST_2x(rcpps, kX86InstIdRcpps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(rcpps, kX86InstIdRcpps, X86XmmReg, X86Mem)

  //! Scalar SP-FP reciprocal (SSE).
  INST_2x(rcpss, kX86InstIdRcpss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(rcpss, kX86InstIdRcpss, X86XmmReg, X86Mem)

  //! Prefetch (SSE).
  INST_2i(prefetch, kX86InstIdPrefetch, X86Mem, Imm)

  //! Packed WORD sum of absolute differences (SSE).
  INST_2x(psadbw, kX86InstIdPsadbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psadbw, kX86InstIdPsadbw, X86XmmReg, X86Mem)

  //! Packed SP-FP square root reciprocal (SSE).
  INST_2x(rsqrtps, kX86InstIdRsqrtps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(rsqrtps, kX86InstIdRsqrtps, X86XmmReg, X86Mem)

  //! Scalar SP-FP square root reciprocal (SSE).
  INST_2x(rsqrtss, kX86InstIdRsqrtss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(rsqrtss, kX86InstIdRsqrtss, X86XmmReg, X86Mem)

  //! Store fence (SSE).
  INST_0x(sfence, kX86InstIdSfence)

  //! Shuffle SP-FP (SSE).
  INST_3i(shufps, kX86InstIdShufps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(shufps, kX86InstIdShufps, X86XmmReg, X86Mem, Imm)

  //! Packed SP-FP square root (SSE).
  INST_2x(sqrtps, kX86InstIdSqrtps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(sqrtps, kX86InstIdSqrtps, X86XmmReg, X86Mem)

  //! Scalar SP-FP square root (SSE).
  INST_2x(sqrtss, kX86InstIdSqrtss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(sqrtss, kX86InstIdSqrtss, X86XmmReg, X86Mem)

  //! Store streaming SIMD extension control/status (SSE).
  INST_1x(stmxcsr, kX86InstIdStmxcsr, X86Mem)

  //! Packed SP-FP subtract (SSE).
  INST_2x(subps, kX86InstIdSubps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(subps, kX86InstIdSubps, X86XmmReg, X86Mem)

  //! Scalar SP-FP subtract (SSE).
  INST_2x(subss, kX86InstIdSubss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(subss, kX86InstIdSubss, X86XmmReg, X86Mem)

  //! Unordered scalar SP-FP compare and set EFLAGS (SSE).
  INST_2x(ucomiss, kX86InstIdUcomiss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(ucomiss, kX86InstIdUcomiss, X86XmmReg, X86Mem)

  //! Unpack high packed SP-FP data (SSE).
  INST_2x(unpckhps, kX86InstIdUnpckhps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(unpckhps, kX86InstIdUnpckhps, X86XmmReg, X86Mem)

  //! Unpack low packed SP-FP data (SSE).
  INST_2x(unpcklps, kX86InstIdUnpcklps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(unpcklps, kX86InstIdUnpcklps, X86XmmReg, X86Mem)

  //! Packed SP-FP bitwise xor (SSE).
  INST_2x(xorps, kX86InstIdXorps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(xorps, kX86InstIdXorps, X86XmmReg, X86Mem)

  // --------------------------------------------------------------------------
  // [SSE2]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add (SSE2).
  INST_2x(addpd, kX86InstIdAddpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(addpd, kX86InstIdAddpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP add (SSE2).
  INST_2x(addsd, kX86InstIdAddsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(addsd, kX86InstIdAddsd, X86XmmReg, X86Mem)

  //! Packed DP-FP bitwise and-not (SSE2).
  INST_2x(andnpd, kX86InstIdAndnpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(andnpd, kX86InstIdAndnpd, X86XmmReg, X86Mem)

  //! Packed DP-FP bitwise and (SSE2).
  INST_2x(andpd, kX86InstIdAndpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(andpd, kX86InstIdAndpd, X86XmmReg, X86Mem)

  //! Flush cache line (SSE2).
  INST_1x(clflush, kX86InstIdClflush, X86Mem)

  //! Packed DP-FP compare (SSE2).
  INST_3i(cmppd, kX86InstIdCmppd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(cmppd, kX86InstIdCmppd, X86XmmReg, X86Mem, Imm)

  //! Scalar SP-FP compare (SSE2).
  INST_3i(cmpsd, kX86InstIdCmpsd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(cmpsd, kX86InstIdCmpsd, X86XmmReg, X86Mem, Imm)

  //! Scalar ordered DP-FP compare and set EFLAGS (SSE2).
  INST_2x(comisd, kX86InstIdComisd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(comisd, kX86InstIdComisd, X86XmmReg, X86Mem)

  //! Convert packed QWORDs to packed DP-FP (SSE2).
  INST_2x(cvtdq2pd, kX86InstIdCvtdq2pd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtdq2pd, kX86InstIdCvtdq2pd, X86XmmReg, X86Mem)

  //! Convert packed QWORDs to packed SP-FP (SSE2).
  INST_2x(cvtdq2ps, kX86InstIdCvtdq2ps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtdq2ps, kX86InstIdCvtdq2ps, X86XmmReg, X86Mem)

  //! Convert packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvtpd2dq, kX86InstIdCvtpd2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtpd2dq, kX86InstIdCvtpd2dq, X86XmmReg, X86Mem)

  //! Convert packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvtpd2pi, kX86InstIdCvtpd2pi, X86MmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtpd2pi, kX86InstIdCvtpd2pi, X86MmReg, X86Mem)

  //! Convert packed DP-FP to packed SP-FP (SSE2).
  INST_2x(cvtpd2ps, kX86InstIdCvtpd2ps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtpd2ps, kX86InstIdCvtpd2ps, X86XmmReg, X86Mem)

  //! Convert packed DWORDs integers to packed DP-FP (SSE2).
  INST_2x(cvtpi2pd, kX86InstIdCvtpi2pd, X86XmmReg, X86MmReg)
  //! \overload
  INST_2x(cvtpi2pd, kX86InstIdCvtpi2pd, X86XmmReg, X86Mem)

  //! Convert packed SP-FP to packed DWORDs (SSE2).
  INST_2x(cvtps2dq, kX86InstIdCvtps2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtps2dq, kX86InstIdCvtps2dq, X86XmmReg, X86Mem)

  //! Convert packed SP-FP to packed DP-FP (SSE2).
  INST_2x(cvtps2pd, kX86InstIdCvtps2pd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtps2pd, kX86InstIdCvtps2pd, X86XmmReg, X86Mem)

  //! Convert scalar DP-FP to DWORD integer (SSE2).
  INST_2x(cvtsd2si, kX86InstIdCvtsd2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(cvtsd2si, kX86InstIdCvtsd2si, X86GpReg, X86Mem)

  //! Convert scalar DP-FP to scalar SP-FP (SSE2).
  INST_2x(cvtsd2ss, kX86InstIdCvtsd2ss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtsd2ss, kX86InstIdCvtsd2ss, X86XmmReg, X86Mem)

  //! Convert DWORD integer to scalar DP-FP (SSE2).
  INST_2x(cvtsi2sd, kX86InstIdCvtsi2sd, X86XmmReg, X86GpReg)
  //! \overload
  INST_2x(cvtsi2sd, kX86InstIdCvtsi2sd, X86XmmReg, X86Mem)

  //! Convert scalar SP-FP to DP-FP (SSE2).
  INST_2x(cvtss2sd, kX86InstIdCvtss2sd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvtss2sd, kX86InstIdCvtss2sd, X86XmmReg, X86Mem)

  //! Convert with truncation packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvttpd2pi, kX86InstIdCvttpd2pi, X86MmReg, X86XmmReg)
  //! \overload
  INST_2x(cvttpd2pi, kX86InstIdCvttpd2pi, X86MmReg, X86Mem)

  //! Convert with truncation packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvttpd2dq, kX86InstIdCvttpd2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvttpd2dq, kX86InstIdCvttpd2dq, X86XmmReg, X86Mem)

  //! Convert with truncation packed SP-FP to packed DWORDs (SSE2).
  INST_2x(cvttps2dq, kX86InstIdCvttps2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(cvttps2dq, kX86InstIdCvttps2dq, X86XmmReg, X86Mem)

  //! Convert with truncation scalar DP-FP to signed DWORDs (SSE2).
  INST_2x(cvttsd2si, kX86InstIdCvttsd2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(cvttsd2si, kX86InstIdCvttsd2si, X86GpReg, X86Mem)

  //! Packed DP-FP divide (SSE2).
  INST_2x(divpd, kX86InstIdDivpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(divpd, kX86InstIdDivpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP divide (SSE2).
  INST_2x(divsd, kX86InstIdDivsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(divsd, kX86InstIdDivsd, X86XmmReg, X86Mem)

  //! Load fence (SSE2).
  INST_0x(lfence, kX86InstIdLfence)

  //! Store selected bytes of OWORD to DS:EDI/RDI (SSE2).
  INST_2x(maskmovdqu, kX86InstIdMaskmovdqu, X86XmmReg, X86XmmReg)

  //! Packed DP-FP maximum (SSE2).
  INST_2x(maxpd, kX86InstIdMaxpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(maxpd, kX86InstIdMaxpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP maximum (SSE2).
  INST_2x(maxsd, kX86InstIdMaxsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(maxsd, kX86InstIdMaxsd, X86XmmReg, X86Mem)

  //! Memory fence (SSE2).
  INST_0x(mfence, kX86InstIdMfence)

  //! Packed DP-FP minimum (SSE2).
  INST_2x(minpd, kX86InstIdMinpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(minpd, kX86InstIdMinpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP minimum (SSE2).
  INST_2x(minsd, kX86InstIdMinsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(minsd, kX86InstIdMinsd, X86XmmReg, X86Mem)

  //! Move aligned OWORD (SSE2).
  INST_2x(movdqa, kX86InstIdMovdqa, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movdqa, kX86InstIdMovdqa, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movdqa, kX86InstIdMovdqa, X86Mem, X86XmmReg)

  //! Move unaligned OWORD (SSE2).
  INST_2x(movdqu, kX86InstIdMovdqu, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movdqu, kX86InstIdMovdqu, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movdqu, kX86InstIdMovdqu, X86Mem, X86XmmReg)

  //! Extract packed SP-FP sign mask (SSE2).
  INST_2x(movmskps, kX86InstIdMovmskps, X86GpReg, X86XmmReg)

  //! Extract packed DP-FP sign mask (SSE2).
  INST_2x(movmskpd, kX86InstIdMovmskpd, X86GpReg, X86XmmReg)

  //! Move scalar DP-FP (SSE2).
  INST_2x(movsd, kX86InstIdMovsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movsd, kX86InstIdMovsd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movsd, kX86InstIdMovsd, X86Mem, X86XmmReg)

  //! Move aligned packed DP-FP (SSE2).
  INST_2x(movapd, kX86InstIdMovapd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movapd, kX86InstIdMovapd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movapd, kX86InstIdMovapd, X86Mem, X86XmmReg)

  //! Move QWORD from Xmm to Mm register (SSE2).
  INST_2x(movdq2q, kX86InstIdMovdq2q, X86MmReg, X86XmmReg)

  //! Move QWORD from Mm to Xmm register (SSE2).
  INST_2x(movq2dq, kX86InstIdMovq2dq, X86XmmReg, X86MmReg)

  //! Move high packed DP-FP (SSE2).
  INST_2x(movhpd, kX86InstIdMovhpd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movhpd, kX86InstIdMovhpd, X86Mem, X86XmmReg)

  //! Move low packed DP-FP (SSE2).
  INST_2x(movlpd, kX86InstIdMovlpd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movlpd, kX86InstIdMovlpd, X86Mem, X86XmmReg)

  //! Store OWORD using NT hint (SSE2).
  INST_2x(movntdq, kX86InstIdMovntdq, X86Mem, X86XmmReg)

  //! Store DWORD using NT hint (SSE2).
  INST_2x(movnti, kX86InstIdMovnti, X86Mem, X86GpReg)

  //! Store packed DP-FP using NT hint (SSE2).
  INST_2x(movntpd, kX86InstIdMovntpd, X86Mem, X86XmmReg)

  //! Move unaligned packed DP-FP (SSE2).
  INST_2x(movupd, kX86InstIdMovupd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movupd, kX86InstIdMovupd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(movupd, kX86InstIdMovupd, X86Mem, X86XmmReg)

  //! Packed DP-FP multiply (SSE2).
  INST_2x(mulpd, kX86InstIdMulpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(mulpd, kX86InstIdMulpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP multiply (SSE2).
  INST_2x(mulsd, kX86InstIdMulsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(mulsd, kX86InstIdMulsd, X86XmmReg, X86Mem)

  //! Packed DP-FP bitwise or (SSE2).
  INST_2x(orpd, kX86InstIdOrpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(orpd, kX86InstIdOrpd, X86XmmReg, X86Mem)

  //! Pack WORDs to BYTEs with signed saturation (SSE2).
  INST_2x(packsswb, kX86InstIdPacksswb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(packsswb, kX86InstIdPacksswb, X86XmmReg, X86Mem)

  //! Pack DWORDs to WORDs with signed saturation (SSE2).
  INST_2x(packssdw, kX86InstIdPackssdw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(packssdw, kX86InstIdPackssdw, X86XmmReg, X86Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (SSE2).
  INST_2x(packuswb, kX86InstIdPackuswb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(packuswb, kX86InstIdPackuswb, X86XmmReg, X86Mem)

  //! Packed BYTE Add (SSE2).
  INST_2x(paddb, kX86InstIdPaddb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddb, kX86InstIdPaddb, X86XmmReg, X86Mem)

  //! Packed WORD add (SSE2).
  INST_2x(paddw, kX86InstIdPaddw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddw, kX86InstIdPaddw, X86XmmReg, X86Mem)

  //! Packed DWORD add (SSE2).
  INST_2x(paddd, kX86InstIdPaddd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddd, kX86InstIdPaddd, X86XmmReg, X86Mem)

  //! Packed QWORD add (SSE2).
  INST_2x(paddq, kX86InstIdPaddq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(paddq, kX86InstIdPaddq, X86MmReg, X86Mem)

  //! Packed QWORD add (SSE2).
  INST_2x(paddq, kX86InstIdPaddq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddq, kX86InstIdPaddq, X86XmmReg, X86Mem)

  //! Packed BYTE add with saturation (SSE2).
  INST_2x(paddsb, kX86InstIdPaddsb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddsb, kX86InstIdPaddsb, X86XmmReg, X86Mem)

  //! Packed WORD add with saturation (SSE2).
  INST_2x(paddsw, kX86InstIdPaddsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddsw, kX86InstIdPaddsw, X86XmmReg, X86Mem)

  //! Packed BYTE add with unsigned saturation (SSE2).
  INST_2x(paddusb, kX86InstIdPaddusb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddusb, kX86InstIdPaddusb, X86XmmReg, X86Mem)

  //! Packed WORD add with unsigned saturation (SSE2).
  INST_2x(paddusw, kX86InstIdPaddusw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(paddusw, kX86InstIdPaddusw, X86XmmReg, X86Mem)

  //! Packed bitwise and (SSE2).
  INST_2x(pand, kX86InstIdPand, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pand, kX86InstIdPand, X86XmmReg, X86Mem)

  //! Packed bitwise and-not (SSE2).
  INST_2x(pandn, kX86InstIdPandn, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pandn, kX86InstIdPandn, X86XmmReg, X86Mem)

  //! Spin loop hint (SSE2).
  INST_0x(pause, kX86InstIdPause)

  //! Packed BYTE average (SSE2).
  INST_2x(pavgb, kX86InstIdPavgb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pavgb, kX86InstIdPavgb, X86XmmReg, X86Mem)

  //! Packed WORD average (SSE2).
  INST_2x(pavgw, kX86InstIdPavgw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pavgw, kX86InstIdPavgw, X86XmmReg, X86Mem)

  //! Packed BYTE compare for equality (SSE2).
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86XmmReg, X86Mem)

  //! Packed WORD compare for equality (SSE2).
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86XmmReg, X86Mem)

  //! Packed DWORD compare for equality (SSE2).
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86XmmReg, X86Mem)

  //! Packed BYTE compare if greater than (SSE2).
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86XmmReg, X86Mem)

  //! Packed WORD compare if greater than (SSE2).
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86XmmReg, X86Mem)

  //! Packed DWORD compare if greater than (SSE2).
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86XmmReg, X86Mem)

  //! Extract WORD based on selector (SSE2).
  INST_3i(pextrw, kX86InstIdPextrw, X86GpReg, X86XmmReg, Imm)

  //! Insert WORD based on selector (SSE2).
  INST_3i(pinsrw, kX86InstIdPinsrw, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_3i(pinsrw, kX86InstIdPinsrw, X86XmmReg, X86Mem, Imm)

  //! Packed WORD maximum (SSE2).
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86XmmReg, X86Mem)

  //! Packed BYTE unsigned maximum (SSE2).
  INST_2x(pmaxub, kX86InstIdPmaxub, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaxub, kX86InstIdPmaxub, X86XmmReg, X86Mem)

  //! Packed WORD minimum (SSE2).
  INST_2x(pminsw, kX86InstIdPminsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pminsw, kX86InstIdPminsw, X86XmmReg, X86Mem)

  //! Packed BYTE unsigned minimum (SSE2).
  INST_2x(pminub, kX86InstIdPminub, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pminub, kX86InstIdPminub, X86XmmReg, X86Mem)

  //! Move byte mask (SSE2).
  INST_2x(pmovmskb, kX86InstIdPmovmskb, X86GpReg, X86XmmReg)

  //! Packed WORD multiply high (SSE2).
  INST_2x(pmulhw, kX86InstIdPmulhw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmulhw, kX86InstIdPmulhw, X86XmmReg, X86Mem)

  //! Packed WORD unsigned multiply high (SSE2).
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86XmmReg, X86Mem)

  //! Packed WORD multiply low (SSE2).
  INST_2x(pmullw, kX86InstIdPmullw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmullw, kX86InstIdPmullw, X86XmmReg, X86Mem)

  //! Packed DWORD multiply to QWORD (SSE2).
  INST_2x(pmuludq, kX86InstIdPmuludq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmuludq, kX86InstIdPmuludq, X86MmReg, X86Mem)

  //! Packed DWORD multiply to QWORD (SSE2).
  INST_2x(pmuludq, kX86InstIdPmuludq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmuludq, kX86InstIdPmuludq, X86XmmReg, X86Mem)

  //! Packed bitwise or (SSE2).
  INST_2x(por, kX86InstIdPor, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(por, kX86InstIdPor, X86XmmReg, X86Mem)

  //! Packed DWORD shift left logical (SSE2).
  INST_2x(pslld, kX86InstIdPslld, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pslld, kX86InstIdPslld, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(pslld, kX86InstIdPslld, X86XmmReg, Imm)

  //! Packed QWORD shift left logical (SSE2).
  INST_2x(psllq, kX86InstIdPsllq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psllq, kX86InstIdPsllq, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psllq, kX86InstIdPsllq, X86XmmReg, Imm)

  //! Packed WORD shift left logical (SSE2).
  INST_2x(psllw, kX86InstIdPsllw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psllw, kX86InstIdPsllw, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psllw, kX86InstIdPsllw, X86XmmReg, Imm)

  //! Packed OWORD shift left logical (SSE2).
  INST_2i(pslldq, kX86InstIdPslldq, X86XmmReg, Imm)

  //! Packed DWORD shift right arithmetic (SSE2).
  INST_2x(psrad, kX86InstIdPsrad, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psrad, kX86InstIdPsrad, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psrad, kX86InstIdPsrad, X86XmmReg, Imm)

  //! Packed WORD shift right arithmetic (SSE2).
  INST_2x(psraw, kX86InstIdPsraw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psraw, kX86InstIdPsraw, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psraw, kX86InstIdPsraw, X86XmmReg, Imm)

  //! Packed BYTE subtract (SSE2).
  INST_2x(psubb, kX86InstIdPsubb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubb, kX86InstIdPsubb, X86XmmReg, X86Mem)

  //! Packed DWORD subtract (SSE2).
  INST_2x(psubd, kX86InstIdPsubd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubd, kX86InstIdPsubd, X86XmmReg, X86Mem)

  //! Packed QWORD subtract (SSE2).
  INST_2x(psubq, kX86InstIdPsubq, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psubq, kX86InstIdPsubq, X86MmReg, X86Mem)

  //! Packed QWORD subtract (SSE2).
  INST_2x(psubq, kX86InstIdPsubq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubq, kX86InstIdPsubq, X86XmmReg, X86Mem)

  //! Packed WORD subtract (SSE2).
  INST_2x(psubw, kX86InstIdPsubw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubw, kX86InstIdPsubw, X86XmmReg, X86Mem)

  //! Packed WORD to DWORD multiply and add (SSE2).
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86XmmReg, X86Mem)

  //! Packed DWORD shuffle (SSE2).
  INST_3i(pshufd, kX86InstIdPshufd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pshufd, kX86InstIdPshufd, X86XmmReg, X86Mem, Imm)

  //! Packed WORD shuffle high (SSE2).
  INST_3i(pshufhw, kX86InstIdPshufhw, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pshufhw, kX86InstIdPshufhw, X86XmmReg, X86Mem, Imm)

  //! Packed WORD shuffle low (SSE2).
  INST_3i(pshuflw, kX86InstIdPshuflw, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pshuflw, kX86InstIdPshuflw, X86XmmReg, X86Mem, Imm)

  //! Packed DWORD shift right logical (SSE2).
  INST_2x(psrld, kX86InstIdPsrld, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psrld, kX86InstIdPsrld, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psrld, kX86InstIdPsrld, X86XmmReg, Imm)

  //! Packed QWORD shift right logical (SSE2).
  INST_2x(psrlq, kX86InstIdPsrlq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psrlq, kX86InstIdPsrlq, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psrlq, kX86InstIdPsrlq, X86XmmReg, Imm)

  //! Scalar OWORD shift right logical (SSE2).
  INST_2i(psrldq, kX86InstIdPsrldq, X86XmmReg, Imm)

  //! Packed WORD shift right logical (SSE2).
  INST_2x(psrlw, kX86InstIdPsrlw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psrlw, kX86InstIdPsrlw, X86XmmReg, X86Mem)
  //! \overload
  INST_2i(psrlw, kX86InstIdPsrlw, X86XmmReg, Imm)

  //! Packed BYTE subtract with saturation (SSE2).
  INST_2x(psubsb, kX86InstIdPsubsb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubsb, kX86InstIdPsubsb, X86XmmReg, X86Mem)

  //! Packed WORD subtract with saturation (SSE2).
  INST_2x(psubsw, kX86InstIdPsubsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubsw, kX86InstIdPsubsw, X86XmmReg, X86Mem)

  //! Packed BYTE subtract with unsigned saturation (SSE2).
  INST_2x(psubusb, kX86InstIdPsubusb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubusb, kX86InstIdPsubusb, X86XmmReg, X86Mem)

  //! Packed WORD subtract with unsigned saturation (SSE2).
  INST_2x(psubusw, kX86InstIdPsubusw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psubusw, kX86InstIdPsubusw, X86XmmReg, X86Mem)

  //! Unpack high packed BYTEs to WORDs (SSE2).
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86XmmReg, X86Mem)

  //! Unpack high packed DWORDs to QWORDs (SSE2).
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86XmmReg, X86Mem)

  //! Unpack high packed QWORDs to OWORD (SSE2).
  INST_2x(punpckhqdq, kX86InstIdPunpckhqdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpckhqdq, kX86InstIdPunpckhqdq, X86XmmReg, X86Mem)

  //! Unpack high packed WORDs to DWORDs (SSE2).
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86XmmReg, X86Mem)

  //! Unpack low packed BYTEs to WORDs (SSE2).
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86XmmReg, X86Mem)

  //! Unpack low packed DWORDs to QWORDs (SSE2).
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86XmmReg, X86Mem)

  //! Unpack low packed QWORDs to OWORD (SSE2).
  INST_2x(punpcklqdq, kX86InstIdPunpcklqdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpcklqdq, kX86InstIdPunpcklqdq, X86XmmReg, X86Mem)

  //! Unpack low packed WORDs to DWORDs (SSE2).
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86XmmReg, X86Mem)

  //! Packed bitwise xor (SSE2).
  INST_2x(pxor, kX86InstIdPxor, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pxor, kX86InstIdPxor, X86XmmReg, X86Mem)

  //! Shuffle DP-FP (SSE2).
  INST_3i(shufpd, kX86InstIdShufpd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(shufpd, kX86InstIdShufpd, X86XmmReg, X86Mem, Imm)

  //! Packed DP-FP square root (SSE2).
  INST_2x(sqrtpd, kX86InstIdSqrtpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(sqrtpd, kX86InstIdSqrtpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP square root (SSE2).
  INST_2x(sqrtsd, kX86InstIdSqrtsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(sqrtsd, kX86InstIdSqrtsd, X86XmmReg, X86Mem)

  //! Packed DP-FP subtract (SSE2).
  INST_2x(subpd, kX86InstIdSubpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(subpd, kX86InstIdSubpd, X86XmmReg, X86Mem)

  //! Scalar DP-FP subtract (SSE2).
  INST_2x(subsd, kX86InstIdSubsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(subsd, kX86InstIdSubsd, X86XmmReg, X86Mem)

  //! Scalar DP-FP unordered compare and set EFLAGS (SSE2).
  INST_2x(ucomisd, kX86InstIdUcomisd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(ucomisd, kX86InstIdUcomisd, X86XmmReg, X86Mem)

  //! Unpack and interleave high packed DP-FP (SSE2).
  INST_2x(unpckhpd, kX86InstIdUnpckhpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(unpckhpd, kX86InstIdUnpckhpd, X86XmmReg, X86Mem)

  //! Unpack and interleave low packed DP-FP (SSE2).
  INST_2x(unpcklpd, kX86InstIdUnpcklpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(unpcklpd, kX86InstIdUnpcklpd, X86XmmReg, X86Mem)

  //! Packed DP-FP bitwise xor (SSE2).
  INST_2x(xorpd, kX86InstIdXorpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(xorpd, kX86InstIdXorpd, X86XmmReg, X86Mem)

  // --------------------------------------------------------------------------
  // [SSE3]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add/subtract (SSE3).
  INST_2x(addsubpd, kX86InstIdAddsubpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(addsubpd, kX86InstIdAddsubpd, X86XmmReg, X86Mem)

  //! Packed SP-FP add/subtract (SSE3).
  INST_2x(addsubps, kX86InstIdAddsubps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(addsubps, kX86InstIdAddsubps, X86XmmReg, X86Mem)

  //! Store truncated `fp0` to `short_or_int_or_long[o0]` and POP (FPU & SSE3).
  INST_1x(fisttp, kX86InstIdFisttp, X86Mem)

  //! Packed DP-FP horizontal add (SSE3).
  INST_2x(haddpd, kX86InstIdHaddpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(haddpd, kX86InstIdHaddpd, X86XmmReg, X86Mem)

  //! Packed SP-FP horizontal add (SSE3).
  INST_2x(haddps, kX86InstIdHaddps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(haddps, kX86InstIdHaddps, X86XmmReg, X86Mem)

  //! Packed DP-FP horizontal subtract (SSE3).
  INST_2x(hsubpd, kX86InstIdHsubpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(hsubpd, kX86InstIdHsubpd, X86XmmReg, X86Mem)

  //! Packed SP-FP horizontal subtract (SSE3).
  INST_2x(hsubps, kX86InstIdHsubps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(hsubps, kX86InstIdHsubps, X86XmmReg, X86Mem)

  //! Load 128-bits unaligned (SSE3).
  INST_2x(lddqu, kX86InstIdLddqu, X86XmmReg, X86Mem)

  //! Setup monitor address (SSE3).
  INST_0x(monitor, kX86InstIdMonitor)

  //! Move one DP-FP and duplicate (SSE3).
  INST_2x(movddup, kX86InstIdMovddup, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movddup, kX86InstIdMovddup, X86XmmReg, X86Mem)

  //! Move packed SP-FP high and duplicate (SSE3).
  INST_2x(movshdup, kX86InstIdMovshdup, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movshdup, kX86InstIdMovshdup, X86XmmReg, X86Mem)

  //! Move packed SP-FP low and duplicate (SSE3).
  INST_2x(movsldup, kX86InstIdMovsldup, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(movsldup, kX86InstIdMovsldup, X86XmmReg, X86Mem)

  //! Monitor wait (SSE3).
  INST_0x(mwait, kX86InstIdMwait)

  // --------------------------------------------------------------------------
  // [SSSE3]
  // --------------------------------------------------------------------------

  //! Packed BYTE sign (SSSE3).
  INST_2x(psignb, kX86InstIdPsignb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psignb, kX86InstIdPsignb, X86MmReg, X86Mem)

  //! Packed BYTE sign (SSSE3).
  INST_2x(psignb, kX86InstIdPsignb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psignb, kX86InstIdPsignb, X86XmmReg, X86Mem)

  //! Packed DWORD sign (SSSE3).
  INST_2x(psignd, kX86InstIdPsignd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psignd, kX86InstIdPsignd, X86MmReg, X86Mem)

  //! Packed DWORD sign (SSSE3).
  INST_2x(psignd, kX86InstIdPsignd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psignd, kX86InstIdPsignd, X86XmmReg, X86Mem)

  //! Packed WORD sign (SSSE3).
  INST_2x(psignw, kX86InstIdPsignw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(psignw, kX86InstIdPsignw, X86MmReg, X86Mem)

  //! Packed WORD sign (SSSE3).
  INST_2x(psignw, kX86InstIdPsignw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(psignw, kX86InstIdPsignw, X86XmmReg, X86Mem)

  //! Packed DWORD horizontal add (SSSE3).
  INST_2x(phaddd, kX86InstIdPhaddd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(phaddd, kX86InstIdPhaddd, X86MmReg, X86Mem)

  //! Packed DWORD horizontal add (SSSE3).
  INST_2x(phaddd, kX86InstIdPhaddd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phaddd, kX86InstIdPhaddd, X86XmmReg, X86Mem)

  //! Packed WORD horizontal add with saturation (SSSE3).
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86MmReg, X86Mem)

  //! Packed WORD horizontal add with saturation (SSSE3).
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86XmmReg, X86Mem)

  //! Packed WORD horizontal add (SSSE3).
  INST_2x(phaddw, kX86InstIdPhaddw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(phaddw, kX86InstIdPhaddw, X86MmReg, X86Mem)

  //! Packed WORD horizontal add (SSSE3).
  INST_2x(phaddw, kX86InstIdPhaddw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phaddw, kX86InstIdPhaddw, X86XmmReg, X86Mem)

  //! Packed DWORD horizontal subtract (SSSE3).
  INST_2x(phsubd, kX86InstIdPhsubd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(phsubd, kX86InstIdPhsubd, X86MmReg, X86Mem)

  //! Packed DWORD horizontal subtract (SSSE3).
  INST_2x(phsubd, kX86InstIdPhsubd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phsubd, kX86InstIdPhsubd, X86XmmReg, X86Mem)

  //! Packed WORD horizontal subtract with saturation (SSSE3).
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86MmReg, X86Mem)

  //! Packed WORD horizontal subtract with saturation (SSSE3).
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86XmmReg, X86Mem)

  //! Packed WORD horizontal subtract (SSSE3).
  INST_2x(phsubw, kX86InstIdPhsubw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(phsubw, kX86InstIdPhsubw, X86MmReg, X86Mem)

  //! Packed WORD horizontal subtract (SSSE3).
  INST_2x(phsubw, kX86InstIdPhsubw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phsubw, kX86InstIdPhsubw, X86XmmReg, X86Mem)

  //! Packed multiply and add signed and unsigned bytes (SSSE3).
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86MmReg, X86Mem)

  //! Packed multiply and add signed and unsigned bytes (SSSE3).
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86XmmReg, X86Mem)

  //! Packed BYTE absolute value (SSSE3).
  INST_2x(pabsb, kX86InstIdPabsb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pabsb, kX86InstIdPabsb, X86MmReg, X86Mem)

  //! Packed BYTE absolute value (SSSE3).
  INST_2x(pabsb, kX86InstIdPabsb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pabsb, kX86InstIdPabsb, X86XmmReg, X86Mem)

  //! Packed DWORD absolute value (SSSE3).
  INST_2x(pabsd, kX86InstIdPabsd, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pabsd, kX86InstIdPabsd, X86MmReg, X86Mem)

  //! Packed DWORD absolute value (SSSE3).
  INST_2x(pabsd, kX86InstIdPabsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pabsd, kX86InstIdPabsd, X86XmmReg, X86Mem)

  //! Packed WORD absolute value (SSSE3).
  INST_2x(pabsw, kX86InstIdPabsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pabsw, kX86InstIdPabsw, X86MmReg, X86Mem)

  //! Packed WORD absolute value (SSSE3).
  INST_2x(pabsw, kX86InstIdPabsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pabsw, kX86InstIdPabsw, X86XmmReg, X86Mem)

  //! Packed WORD multiply high, round and scale (SSSE3).
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86MmReg, X86Mem)

  //! Packed WORD multiply high, round and scale (SSSE3).
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86XmmReg, X86Mem)

  //! Packed BYTE shuffle (SSSE3).
  INST_2x(pshufb, kX86InstIdPshufb, X86MmReg, X86MmReg)
  //! \overload
  INST_2x(pshufb, kX86InstIdPshufb, X86MmReg, X86Mem)

  //! Packed BYTE shuffle (SSSE3).
  INST_2x(pshufb, kX86InstIdPshufb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pshufb, kX86InstIdPshufb, X86XmmReg, X86Mem)

  //! Packed align right (SSSE3).
  INST_3i(palignr, kX86InstIdPalignr, X86MmReg, X86MmReg, Imm)
  //! \overload
  INST_3i(palignr, kX86InstIdPalignr, X86MmReg, X86Mem, Imm)

  //! Packed align right (SSSE3).
  INST_3i(palignr, kX86InstIdPalignr, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(palignr, kX86InstIdPalignr, X86XmmReg, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [SSE4.1]
  // --------------------------------------------------------------------------

  //! Packed DP-FP blend (SSE4.1).
  INST_3i(blendpd, kX86InstIdBlendpd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(blendpd, kX86InstIdBlendpd, X86XmmReg, X86Mem, Imm)

  //! Packed SP-FP blend (SSE4.1).
  INST_3i(blendps, kX86InstIdBlendps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(blendps, kX86InstIdBlendps, X86XmmReg, X86Mem, Imm)

  //! Packed DP-FP variable blend (SSE4.1).
  INST_2x(blendvpd, kX86InstIdBlendvpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(blendvpd, kX86InstIdBlendvpd, X86XmmReg, X86Mem)

  //! Packed SP-FP variable blend (SSE4.1).
  INST_2x(blendvps, kX86InstIdBlendvps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(blendvps, kX86InstIdBlendvps, X86XmmReg, X86Mem)

  //! Packed DP-FP dot product (SSE4.1).
  INST_3i(dppd, kX86InstIdDppd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(dppd, kX86InstIdDppd, X86XmmReg, X86Mem, Imm)

  //! Packed SP-FP dot product (SSE4.1).
  INST_3i(dpps, kX86InstIdDpps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(dpps, kX86InstIdDpps, X86XmmReg, X86Mem, Imm)

  //! Extract SP-FP based on selector (SSE4.1).
  INST_3i(extractps, kX86InstIdExtractps, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(extractps, kX86InstIdExtractps, X86Mem, X86XmmReg, Imm)

  //! Insert SP-FP based on selector (SSE4.1).
  INST_3i(insertps, kX86InstIdInsertps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(insertps, kX86InstIdInsertps, X86XmmReg, X86Mem, Imm)

  //! Load OWORD aligned using NT hint (SSE4.1).
  INST_2x(movntdqa, kX86InstIdMovntdqa, X86XmmReg, X86Mem)

  //! Packed WORD sums of absolute difference (SSE4.1).
  INST_3i(mpsadbw, kX86InstIdMpsadbw, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(mpsadbw, kX86InstIdMpsadbw, X86XmmReg, X86Mem, Imm)

  //! Pack DWORDs to WORDs with unsigned saturation (SSE4.1).
  INST_2x(packusdw, kX86InstIdPackusdw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(packusdw, kX86InstIdPackusdw, X86XmmReg, X86Mem)

  //! Packed BYTE variable blend (SSE4.1).
  INST_2x(pblendvb, kX86InstIdPblendvb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pblendvb, kX86InstIdPblendvb, X86XmmReg, X86Mem)

  //! Packed WORD blend (SSE4.1).
  INST_3i(pblendw, kX86InstIdPblendw, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pblendw, kX86InstIdPblendw, X86XmmReg, X86Mem, Imm)

  //! Packed QWORD compare for equality (SSE4.1).
  INST_2x(pcmpeqq, kX86InstIdPcmpeqq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpeqq, kX86InstIdPcmpeqq, X86XmmReg, X86Mem)

  //! Extract BYTE based on selector (SSE4.1).
  INST_3i(pextrb, kX86InstIdPextrb, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pextrb, kX86InstIdPextrb, X86Mem, X86XmmReg, Imm)

  //! Extract DWORD based on selector (SSE4.1).
  INST_3i(pextrd, kX86InstIdPextrd, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pextrd, kX86InstIdPextrd, X86Mem, X86XmmReg, Imm)

  //! Extract QWORD based on selector (SSE4.1).
  INST_3i(pextrq, kX86InstIdPextrq, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pextrq, kX86InstIdPextrq, X86Mem, X86XmmReg, Imm)

  //! Extract WORD based on selector (SSE4.1).
  INST_3i(pextrw, kX86InstIdPextrw, X86Mem, X86XmmReg, Imm)

  //! Packed WORD horizontal minimum (SSE4.1).
  INST_2x(phminposuw, kX86InstIdPhminposuw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(phminposuw, kX86InstIdPhminposuw, X86XmmReg, X86Mem)

  //! Insert BYTE based on selector (SSE4.1).
  INST_3i(pinsrb, kX86InstIdPinsrb, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_3i(pinsrb, kX86InstIdPinsrb, X86XmmReg, X86Mem, Imm)

  //! Insert DWORD based on selector (SSE4.1).
  INST_3i(pinsrd, kX86InstIdPinsrd, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_3i(pinsrd, kX86InstIdPinsrd, X86XmmReg, X86Mem, Imm)

  //! Insert QWORD based on selector (SSE4.1).
  INST_3i(pinsrq, kX86InstIdPinsrq, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_3i(pinsrq, kX86InstIdPinsrq, X86XmmReg, X86Mem, Imm)

  //! Packed BYTE maximum (SSE4.1).
  INST_2x(pmaxsb, kX86InstIdPmaxsb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaxsb, kX86InstIdPmaxsb, X86XmmReg, X86Mem)

  //! Packed DWORD maximum (SSE4.1).
  INST_2x(pmaxsd, kX86InstIdPmaxsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaxsd, kX86InstIdPmaxsd, X86XmmReg, X86Mem)

  //! Packed DWORD unsigned maximum (SSE4.1).
  INST_2x(pmaxud, kX86InstIdPmaxud, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaxud,kX86InstIdPmaxud , X86XmmReg, X86Mem)

  //! Packed WORD unsigned maximum (SSE4.1).
  INST_2x(pmaxuw, kX86InstIdPmaxuw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmaxuw, kX86InstIdPmaxuw, X86XmmReg, X86Mem)

  //! Packed BYTE minimum (SSE4.1).
  INST_2x(pminsb, kX86InstIdPminsb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pminsb, kX86InstIdPminsb, X86XmmReg, X86Mem)

  //! Packed DWORD minimum (SSE4.1).
  INST_2x(pminsd, kX86InstIdPminsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pminsd, kX86InstIdPminsd, X86XmmReg, X86Mem)

  //! Packed WORD unsigned minimum (SSE4.1).
  INST_2x(pminuw, kX86InstIdPminuw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pminuw, kX86InstIdPminuw, X86XmmReg, X86Mem)

  //! Packed DWORD unsigned minimum (SSE4.1).
  INST_2x(pminud, kX86InstIdPminud, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pminud, kX86InstIdPminud, X86XmmReg, X86Mem)

  //! BYTE to DWORD with sign extend (SSE4.1).
  INST_2x(pmovsxbd, kX86InstIdPmovsxbd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovsxbd, kX86InstIdPmovsxbd, X86XmmReg, X86Mem)

  //! Packed BYTE to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxbq, kX86InstIdPmovsxbq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovsxbq, kX86InstIdPmovsxbq, X86XmmReg, X86Mem)

  //! Packed BYTE to WORD with sign extend (SSE4.1).
  INST_2x(pmovsxbw, kX86InstIdPmovsxbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovsxbw, kX86InstIdPmovsxbw, X86XmmReg, X86Mem)

  //! Packed DWORD to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxdq, kX86InstIdPmovsxdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovsxdq, kX86InstIdPmovsxdq, X86XmmReg, X86Mem)

  //! Packed WORD to DWORD with sign extend (SSE4.1).
  INST_2x(pmovsxwd, kX86InstIdPmovsxwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovsxwd, kX86InstIdPmovsxwd, X86XmmReg, X86Mem)

  //! Packed WORD to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxwq, kX86InstIdPmovsxwq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovsxwq, kX86InstIdPmovsxwq, X86XmmReg, X86Mem)

  //! BYTE to DWORD with zero extend (SSE4.1).
  INST_2x(pmovzxbd, kX86InstIdPmovzxbd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovzxbd, kX86InstIdPmovzxbd, X86XmmReg, X86Mem)

  //! Packed BYTE to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxbq, kX86InstIdPmovzxbq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovzxbq, kX86InstIdPmovzxbq, X86XmmReg, X86Mem)

  //! BYTE to WORD with zero extend (SSE4.1).
  INST_2x(pmovzxbw, kX86InstIdPmovzxbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovzxbw, kX86InstIdPmovzxbw, X86XmmReg, X86Mem)

  //! Packed DWORD to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxdq, kX86InstIdPmovzxdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovzxdq, kX86InstIdPmovzxdq, X86XmmReg, X86Mem)

  //! Packed WORD to DWORD with zero extend (SSE4.1).
  INST_2x(pmovzxwd, kX86InstIdPmovzxwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovzxwd, kX86InstIdPmovzxwd, X86XmmReg, X86Mem)

  //! Packed WORD to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxwq, kX86InstIdPmovzxwq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmovzxwq, kX86InstIdPmovzxwq, X86XmmReg, X86Mem)

  //! Packed DWORD to QWORD multiply (SSE4.1).
  INST_2x(pmuldq, kX86InstIdPmuldq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmuldq, kX86InstIdPmuldq, X86XmmReg, X86Mem)

  //! Packed DWORD multiply low (SSE4.1).
  INST_2x(pmulld, kX86InstIdPmulld, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pmulld, kX86InstIdPmulld, X86XmmReg, X86Mem)

  //! Logical compare (SSE4.1).
  INST_2x(ptest, kX86InstIdPtest, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(ptest, kX86InstIdPtest, X86XmmReg, X86Mem)

  //! Packed DP-FP round (SSE4.1).
  INST_3i(roundpd, kX86InstIdRoundpd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(roundpd, kX86InstIdRoundpd, X86XmmReg, X86Mem, Imm)

  //! Packed SP-FP round (SSE4.1).
  INST_3i(roundps, kX86InstIdRoundps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(roundps, kX86InstIdRoundps, X86XmmReg, X86Mem, Imm)

  //! Scalar DP-FP round (SSE4.1).
  INST_3i(roundsd, kX86InstIdRoundsd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(roundsd, kX86InstIdRoundsd, X86XmmReg, X86Mem, Imm)

  //! Scalar SP-FP round (SSE4.1).
  INST_3i(roundss, kX86InstIdRoundss, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(roundss, kX86InstIdRoundss, X86XmmReg, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [SSE4.2]
  // --------------------------------------------------------------------------

  //! Accumulate crc32 value (polynomial 0x11EDC6F41) (SSE4.2).
  INST_2x_(crc32, kX86InstIdCrc32, X86GpReg, X86GpReg, o0.isRegType(kX86RegTypeGpd) || o0.isRegType(kX86RegTypeGpq))
  //! \overload
  INST_2x_(crc32, kX86InstIdCrc32, X86GpReg, X86Mem, o0.isRegType(kX86RegTypeGpd) || o0.isRegType(kX86RegTypeGpq))

  //! Packed compare explicit length strings, return index (SSE4.2).
  INST_3i(pcmpestri, kX86InstIdPcmpestri, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pcmpestri, kX86InstIdPcmpestri, X86XmmReg, X86Mem, Imm)

  //! Packed compare explicit length strings, return mask (SSE4.2).
  INST_3i(pcmpestrm, kX86InstIdPcmpestrm, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pcmpestrm, kX86InstIdPcmpestrm, X86XmmReg, X86Mem, Imm)

  //! Packed compare implicit length strings, return index (SSE4.2).
  INST_3i(pcmpistri, kX86InstIdPcmpistri, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pcmpistri, kX86InstIdPcmpistri, X86XmmReg, X86Mem, Imm)

  //! Packed compare implicit length strings, return mask (SSE4.2).
  INST_3i(pcmpistrm, kX86InstIdPcmpistrm, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pcmpistrm, kX86InstIdPcmpistrm, X86XmmReg, X86Mem, Imm)

  //! Packed QWORD compare if greater than (SSE4.2).
  INST_2x(pcmpgtq, kX86InstIdPcmpgtq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(pcmpgtq, kX86InstIdPcmpgtq, X86XmmReg, X86Mem)

  // --------------------------------------------------------------------------
  // [SSE4a]
  // --------------------------------------------------------------------------

  //! Extract Field (SSE4a).
  INST_2x(extrq, kX86InstIdExtrq, X86XmmReg, X86XmmReg)
  //! Extract Field (SSE4a).
  INST_3ii(extrq, kX86InstIdExtrq, X86XmmReg, Imm, Imm)

  //! Insert Field (SSE4a).
  INST_2x(insertq, kX86InstIdInsertq, X86XmmReg, X86XmmReg)
  //! Insert Field (SSE4a).
  INST_4ii(insertq, kX86InstIdInsertq, X86XmmReg, X86XmmReg, Imm, Imm)

  //! Move Non-Temporal Scalar DP-FP (SSE4a).
  INST_2x(movntsd, kX86InstIdMovntsd, X86Mem, X86XmmReg)
  //! Move Non-Temporal Scalar SP-FP (SSE4a).
  INST_2x(movntss, kX86InstIdMovntss, X86Mem, X86XmmReg)

  // --------------------------------------------------------------------------
  // [POPCNT]
  // --------------------------------------------------------------------------

  //! Return the count of number of bits set to 1 (POPCNT).
  INST_2x_(popcnt, kX86InstIdPopcnt, X86GpReg, X86GpReg, !o0.isGpb() && o0.getRegType() == o1.getRegType())
  //! \overload
  INST_2x_(popcnt, kX86InstIdPopcnt, X86GpReg, X86Mem, !o0.isGpb())

  // --------------------------------------------------------------------------
  // [LZCNT]
  // --------------------------------------------------------------------------

  //! Count the number of leading zero bits (LZCNT).
  INST_2x(lzcnt, kX86InstIdLzcnt, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(lzcnt, kX86InstIdLzcnt, X86GpReg, X86Mem)

  // --------------------------------------------------------------------------
  // [AESNI]
  // --------------------------------------------------------------------------

  //! Perform a single round of the AES decryption flow (AESNI).
  INST_2x(aesdec, kX86InstIdAesdec, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(aesdec, kX86InstIdAesdec, X86XmmReg, X86Mem)

  //! Perform the last round of the AES decryption flow (AESNI).
  INST_2x(aesdeclast, kX86InstIdAesdeclast, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(aesdeclast, kX86InstIdAesdeclast, X86XmmReg, X86Mem)

  //! Perform a single round of the AES encryption flow (AESNI).
  INST_2x(aesenc, kX86InstIdAesenc, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(aesenc, kX86InstIdAesenc, X86XmmReg, X86Mem)

  //! Perform the last round of the AES encryption flow (AESNI).
  INST_2x(aesenclast, kX86InstIdAesenclast, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(aesenclast, kX86InstIdAesenclast, X86XmmReg, X86Mem)

  //! Perform the InvMixColumns transformation (AESNI).
  INST_2x(aesimc, kX86InstIdAesimc, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(aesimc, kX86InstIdAesimc, X86XmmReg, X86Mem)

  //! Assist in expanding the AES cipher key (AESNI).
  INST_3i(aeskeygenassist, kX86InstIdAeskeygenassist, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(aeskeygenassist, kX86InstIdAeskeygenassist, X86XmmReg, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [PCLMULQDQ]
  // --------------------------------------------------------------------------

  //! Packed QWORD to OWORD carry-less multiply (PCLMULQDQ).
  INST_3i(pclmulqdq, kX86InstIdPclmulqdq, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(pclmulqdq, kX86InstIdPclmulqdq, X86XmmReg, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [XSAVE]
  // --------------------------------------------------------------------------

  //! Restore Processor Extended States specified by `EDX:EAX` (XSAVE).
  INST_1x(xrstor, kX86InstIdXrstor, X86Mem)
  //! Restore Processor Extended States specified by `EDX:EAX` (XSAVE&X64).
  INST_1x(xrstor64, kX86InstIdXrstor64, X86Mem)

  //! Save Processor Extended States specified by `EDX:EAX` (XSAVE).
  INST_1x(xsave, kX86InstIdXsave, X86Mem)
  //! Save Processor Extended States specified by `EDX:EAX` (XSAVE&X64).
  INST_1x(xsave64, kX86InstIdXsave64, X86Mem)

  //! Save Processor Extended States specified by `EDX:EAX` (Optimized) (XSAVEOPT).
  INST_1x(xsaveopt, kX86InstIdXsave, X86Mem)
  //! Save Processor Extended States specified by `EDX:EAX` (Optimized) (XSAVEOPT&X64).
  INST_1x(xsaveopt64, kX86InstIdXsave64, X86Mem)

  //! Get XCR - `EDX:EAX <- XCR[ECX]` (XSAVE).
  INST_0x(xgetbv, kX86InstIdXgetbv)
  //! Set XCR - `XCR[ECX] <- EDX:EAX` (XSAVE).
  INST_0x(xsetbv, kX86InstIdXsetbv)

  // --------------------------------------------------------------------------
  // [AVX]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add (AVX).
  INST_3x(vaddpd, kX86InstIdVaddpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaddpd, kX86InstIdVaddpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vaddpd, kX86InstIdVaddpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vaddpd, kX86InstIdVaddpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP add (AVX).
  INST_3x(vaddps, kX86InstIdVaddps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaddps, kX86InstIdVaddps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vaddps, kX86InstIdVaddps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vaddps, kX86InstIdVaddps, X86YmmReg, X86YmmReg, X86Mem)

  //! Scalar DP-FP add (AVX)
  INST_3x(vaddsd, kX86InstIdVaddsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaddsd, kX86InstIdVaddsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP add (AVX)
  INST_3x(vaddss, kX86InstIdVaddss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaddss, kX86InstIdVaddss, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DP-FP add/subtract (AVX).
  INST_3x(vaddsubpd, kX86InstIdVaddsubpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaddsubpd, kX86InstIdVaddsubpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vaddsubpd, kX86InstIdVaddsubpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vaddsubpd, kX86InstIdVaddsubpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP add/subtract (AVX).
  INST_3x(vaddsubps, kX86InstIdVaddsubps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaddsubps, kX86InstIdVaddsubps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vaddsubps, kX86InstIdVaddsubps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vaddsubps, kX86InstIdVaddsubps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DP-FP bitwise and (AVX).
  INST_3x(vandpd, kX86InstIdVandpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vandpd, kX86InstIdVandpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vandpd, kX86InstIdVandpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vandpd, kX86InstIdVandpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP bitwise and (AVX).
  INST_3x(vandps, kX86InstIdVandps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vandps, kX86InstIdVandps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vandps, kX86InstIdVandps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vandps, kX86InstIdVandps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DP-FP bitwise and-not (AVX).
  INST_3x(vandnpd, kX86InstIdVandnpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vandnpd, kX86InstIdVandnpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vandnpd, kX86InstIdVandnpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vandnpd, kX86InstIdVandnpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP bitwise and-not (AVX).
  INST_3x(vandnps, kX86InstIdVandnps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vandnps, kX86InstIdVandnps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vandnps, kX86InstIdVandnps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vandnps, kX86InstIdVandnps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DP-FP blend (AVX).
  INST_4i(vblendpd, kX86InstIdVblendpd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vblendpd, kX86InstIdVblendpd, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vblendpd, kX86InstIdVblendpd, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vblendpd, kX86InstIdVblendpd, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed SP-FP blend (AVX).
  INST_4i(vblendps, kX86InstIdVblendps, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vblendps, kX86InstIdVblendps, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vblendps, kX86InstIdVblendps, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vblendps, kX86InstIdVblendps, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed DP-FP variable blend (AVX).
  INST_4x(vblendvpd, kX86InstIdVblendvpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_4x(vblendvpd, kX86InstIdVblendvpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  //! \overload
  INST_4x(vblendvpd, kX86InstIdVblendvpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_4x(vblendvpd, kX86InstIdVblendvpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)

  //! Packed SP-FP variable blend (AVX).
  INST_4x(vblendvps, kX86InstIdVblendvps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_4x(vblendvps, kX86InstIdVblendvps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  //! \overload
  INST_4x(vblendvps, kX86InstIdVblendvps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_4x(vblendvps, kX86InstIdVblendvps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)

  //! Broadcast 128-bits of FP data in `o1` to low and high 128-bits in `o0` (AVX).
  INST_2x(vbroadcastf128, kX86InstIdVbroadcastf128, X86YmmReg, X86Mem)
  //! Broadcast DP-FP element in `o1` to four locations in `o0` (AVX).
  INST_2x(vbroadcastsd, kX86InstIdVbroadcastsd, X86YmmReg, X86Mem)
  //! Broadcast SP-FP element in `o1` to four locations in `o0` (AVX).
  INST_2x(vbroadcastss, kX86InstIdVbroadcastss, X86XmmReg, X86Mem)
  //! Broadcast SP-FP element in `o1` to eight locations in `o0` (AVX).
  INST_2x(vbroadcastss, kX86InstIdVbroadcastss, X86YmmReg, X86Mem)

  //! Packed DP-FP compare (AVX).
  INST_4i(vcmppd, kX86InstIdVcmppd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vcmppd, kX86InstIdVcmppd, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vcmppd, kX86InstIdVcmppd, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vcmppd, kX86InstIdVcmppd, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed SP-FP compare (AVX).
  INST_4i(vcmpps, kX86InstIdVcmpps, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vcmpps, kX86InstIdVcmpps, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vcmpps, kX86InstIdVcmpps, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vcmpps, kX86InstIdVcmpps, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Scalar DP-FP compare (AVX).
  INST_4i(vcmpsd, kX86InstIdVcmpsd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vcmpsd, kX86InstIdVcmpsd, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Scalar SP-FP compare (AVX).
  INST_4i(vcmpss, kX86InstIdVcmpss, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vcmpss, kX86InstIdVcmpss, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Scalar DP-FP ordered compare and set EFLAGS (AVX).
  INST_2x(vcomisd, kX86InstIdVcomisd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcomisd, kX86InstIdVcomisd, X86XmmReg, X86Mem)

  //! Scalar SP-FP ordered compare and set EFLAGS (AVX).
  INST_2x(vcomiss, kX86InstIdVcomiss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcomiss, kX86InstIdVcomiss, X86XmmReg, X86Mem)

  //! Convert packed QWORDs to packed DP-FP (AVX).
  INST_2x(vcvtdq2pd, kX86InstIdVcvtdq2pd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtdq2pd, kX86InstIdVcvtdq2pd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vcvtdq2pd, kX86InstIdVcvtdq2pd, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtdq2pd, kX86InstIdVcvtdq2pd, X86YmmReg, X86Mem)

  //! Convert packed QWORDs to packed SP-FP (AVX).
  INST_2x(vcvtdq2ps, kX86InstIdVcvtdq2ps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtdq2ps, kX86InstIdVcvtdq2ps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vcvtdq2ps, kX86InstIdVcvtdq2ps, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vcvtdq2ps, kX86InstIdVcvtdq2ps, X86YmmReg, X86Mem)

  //! Convert packed DP-FP to packed DWORDs (AVX).
  INST_2x(vcvtpd2dq, kX86InstIdVcvtpd2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtpd2dq, kX86InstIdVcvtpd2dq, X86XmmReg, X86YmmReg)
  //! \overload
  INST_2x(vcvtpd2dq, kX86InstIdVcvtpd2dq, X86XmmReg, X86Mem)

  //! Convert packed DP-FP to packed SP-FP (AVX).
  INST_2x(vcvtpd2ps, kX86InstIdVcvtpd2ps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtpd2ps, kX86InstIdVcvtpd2ps, X86XmmReg, X86YmmReg)
  //! \overload
  INST_2x(vcvtpd2ps, kX86InstIdVcvtpd2ps, X86XmmReg, X86Mem)

  //! Convert packed SP-FP to packed DWORDs (AVX).
  INST_2x(vcvtps2dq, kX86InstIdVcvtps2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtps2dq, kX86InstIdVcvtps2dq, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vcvtps2dq, kX86InstIdVcvtps2dq, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vcvtps2dq, kX86InstIdVcvtps2dq, X86YmmReg, X86Mem)

  //! Convert packed SP-FP to packed DP-FP (AVX).
  INST_2x(vcvtps2pd, kX86InstIdVcvtps2pd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtps2pd, kX86InstIdVcvtps2pd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vcvtps2pd, kX86InstIdVcvtps2pd, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtps2pd, kX86InstIdVcvtps2pd, X86YmmReg, X86Mem)

  //! Convert scalar DP-FP to DWORD (AVX).
  INST_2x(vcvtsd2si, kX86InstIdVcvtsd2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtsd2si, kX86InstIdVcvtsd2si, X86GpReg, X86Mem)

  //! Convert scalar DP-FP to scalar SP-FP (AVX).
  INST_3x(vcvtsd2ss, kX86InstIdVcvtsd2ss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vcvtsd2ss, kX86InstIdVcvtsd2ss, X86XmmReg, X86XmmReg, X86Mem)

  //! Convert DWORD integer to scalar DP-FP (AVX).
  INST_3x(vcvtsi2sd, kX86InstIdVcvtsi2sd, X86XmmReg, X86XmmReg, X86GpReg)
  //! \overload
  INST_3x(vcvtsi2sd, kX86InstIdVcvtsi2sd, X86XmmReg, X86XmmReg, X86Mem)

  //! Convert scalar INT32 to SP-FP (AVX).
  INST_3x(vcvtsi2ss, kX86InstIdVcvtsi2ss, X86XmmReg, X86XmmReg, X86GpReg)
  //! \overload
  INST_3x(vcvtsi2ss, kX86InstIdVcvtsi2ss, X86XmmReg, X86XmmReg, X86Mem)

  //! Convert scalar SP-FP to DP-FP (AVX).
  INST_3x(vcvtss2sd, kX86InstIdVcvtss2sd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vcvtss2sd, kX86InstIdVcvtss2sd, X86XmmReg, X86XmmReg, X86Mem)

  //! Convert scalar SP-FP to INT32 (AVX).
  INST_2x(vcvtss2si, kX86InstIdVcvtss2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtss2si, kX86InstIdVcvtss2si, X86GpReg, X86Mem)

  //! Convert with truncation packed DP-FP to packed DWORDs (AVX).
  INST_2x(vcvttpd2dq, kX86InstIdVcvttpd2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvttpd2dq, kX86InstIdVcvttpd2dq, X86XmmReg, X86YmmReg)
  //! \overload
  INST_2x(vcvttpd2dq, kX86InstIdVcvttpd2dq, X86XmmReg, X86Mem)

  //! Convert with truncation packed SP-FP to packed DWORDs (AVX).
  INST_2x(vcvttps2dq, kX86InstIdVcvttps2dq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvttps2dq, kX86InstIdVcvttps2dq, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vcvttps2dq, kX86InstIdVcvttps2dq, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vcvttps2dq, kX86InstIdVcvttps2dq, X86YmmReg, X86Mem)

  //! Convert with truncation scalar DP-FP to INT32 (AVX).
  INST_2x(vcvttsd2si, kX86InstIdVcvttsd2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vcvttsd2si, kX86InstIdVcvttsd2si, X86GpReg, X86Mem)

  //! Convert with truncation scalar SP-FP to INT32 (AVX).
  INST_2x(vcvttss2si, kX86InstIdVcvttss2si, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vcvttss2si, kX86InstIdVcvttss2si, X86GpReg, X86Mem)

  //! Packed DP-FP divide (AVX).
  INST_3x(vdivpd, kX86InstIdVdivpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vdivpd, kX86InstIdVdivpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vdivpd, kX86InstIdVdivpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vdivpd, kX86InstIdVdivpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP divide (AVX).
  INST_3x(vdivps, kX86InstIdVdivps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vdivps, kX86InstIdVdivps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vdivps, kX86InstIdVdivps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vdivps, kX86InstIdVdivps, X86YmmReg, X86YmmReg, X86Mem)

  //! Scalar DP-FP divide (AVX).
  INST_3x(vdivsd, kX86InstIdVdivsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vdivsd, kX86InstIdVdivsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP divide (AVX).
  INST_3x(vdivss, kX86InstIdVdivss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vdivss, kX86InstIdVdivss, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DP-FP dot product (AVX).
  INST_4i(vdppd, kX86InstIdVdppd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vdppd, kX86InstIdVdppd, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Packed SP-FP dot product (AVX).
  INST_4i(vdpps, kX86InstIdVdpps, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vdpps, kX86InstIdVdpps, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vdpps, kX86InstIdVdpps, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vdpps, kX86InstIdVdpps, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Extract 128 bits of packed FP data from `o1` and store results in `o0` (AVX).
  INST_3i(vextractf128, kX86InstIdVextractf128, X86XmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vextractf128, kX86InstIdVextractf128, X86Mem, X86YmmReg, Imm)

  //! Extract SP-FP based on selector (AVX).
  INST_3i(vextractps, kX86InstIdVextractps, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vextractps, kX86InstIdVextractps, X86Mem, X86XmmReg, Imm)

  //! Packed DP-FP horizontal add (AVX).
  INST_3x(vhaddpd, kX86InstIdVhaddpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vhaddpd, kX86InstIdVhaddpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vhaddpd, kX86InstIdVhaddpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vhaddpd, kX86InstIdVhaddpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP horizontal add (AVX).
  INST_3x(vhaddps, kX86InstIdVhaddps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vhaddps, kX86InstIdVhaddps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vhaddps, kX86InstIdVhaddps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vhaddps, kX86InstIdVhaddps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DP-FP horizontal subtract (AVX).
  INST_3x(vhsubpd, kX86InstIdVhsubpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vhsubpd, kX86InstIdVhsubpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vhsubpd, kX86InstIdVhsubpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vhsubpd, kX86InstIdVhsubpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP horizontal subtract (AVX).
  INST_3x(vhsubps, kX86InstIdVhsubps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vhsubps, kX86InstIdVhsubps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vhsubps, kX86InstIdVhsubps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vhsubps, kX86InstIdVhsubps, X86YmmReg, X86YmmReg, X86Mem)

  //! Insert 128-bit of packed FP data based on selector (AVX).
  INST_4i(vinsertf128, kX86InstIdVinsertf128, X86YmmReg, X86YmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vinsertf128, kX86InstIdVinsertf128, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Insert SP-FP based on selector (AVX).
  INST_4i(vinsertps, kX86InstIdVinsertps, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vinsertps, kX86InstIdVinsertps, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Load 128-bits unaligned (AVX).
  INST_2x(vlddqu, kX86InstIdVlddqu, X86XmmReg, X86Mem)
  //! Load 256-bits unaligned (AVX).
  INST_2x(vlddqu, kX86InstIdVlddqu, X86YmmReg, X86Mem)

  //! Load streaming SIMD extension control/status (AVX).
  INST_1x(vldmxcsr, kX86InstIdVldmxcsr, X86Mem)

  //! Store selected bytes of OWORD to DS:EDI/RDI (AVX).
  INST_2x(vmaskmovdqu, kX86InstIdVmaskmovdqu, X86XmmReg, X86XmmReg)

  //! Conditionally load packed DP-FP from `o2` using mask in `o1 and store in `o0` (AVX).
  INST_3x(vmaskmovpd, kX86InstIdVmaskmovpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vmaskmovpd, kX86InstIdVmaskmovpd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vmaskmovpd, kX86InstIdVmaskmovpd, X86Mem, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmaskmovpd, kX86InstIdVmaskmovpd, X86Mem, X86YmmReg, X86YmmReg)

  //! Conditionally load packed SP-FP from `o2` using mask in `o1 and store in `o0` (AVX).
  INST_3x(vmaskmovps, kX86InstIdVmaskmovps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vmaskmovps, kX86InstIdVmaskmovps, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vmaskmovps, kX86InstIdVmaskmovps, X86Mem, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmaskmovps, kX86InstIdVmaskmovps, X86Mem, X86YmmReg, X86YmmReg)

  //! Packed DP-FP maximum (AVX).
  INST_3x(vmaxpd, kX86InstIdVmaxpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmaxpd, kX86InstIdVmaxpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vmaxpd, kX86InstIdVmaxpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vmaxpd, kX86InstIdVmaxpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP maximum (AVX).
  INST_3x(vmaxps, kX86InstIdVmaxps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmaxps, kX86InstIdVmaxps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vmaxps, kX86InstIdVmaxps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vmaxps, kX86InstIdVmaxps, X86YmmReg, X86YmmReg, X86Mem)

  //! Scalar DP-FP maximum (AVX).
  INST_3x(vmaxsd, kX86InstIdVmaxsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmaxsd, kX86InstIdVmaxsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP maximum (AVX).
  INST_3x(vmaxss, kX86InstIdVmaxss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmaxss, kX86InstIdVmaxss, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DP-FP minimum (AVX).
  INST_3x(vminpd, kX86InstIdVminpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vminpd, kX86InstIdVminpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vminpd, kX86InstIdVminpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vminpd, kX86InstIdVminpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP minimum (AVX).
  INST_3x(vminps, kX86InstIdVminps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vminps, kX86InstIdVminps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vminps, kX86InstIdVminps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vminps, kX86InstIdVminps, X86YmmReg, X86YmmReg, X86Mem)

  //! Scalar DP-FP minimum (AVX).
  INST_3x(vminsd, kX86InstIdVminsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vminsd, kX86InstIdVminsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP minimum (AVX).
  INST_3x(vminss, kX86InstIdVminss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vminss, kX86InstIdVminss, X86XmmReg, X86XmmReg, X86Mem)

  //! Move 128-bits of aligned packed DP-FP (AVX).
  INST_2x(vmovapd, kX86InstIdVmovapd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovapd, kX86InstIdVmovapd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovapd, kX86InstIdVmovapd, X86Mem, X86XmmReg)
  //! Move 256-bits of aligned packed DP-FP (AVX).
  INST_2x(vmovapd, kX86InstIdVmovapd, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovapd, kX86InstIdVmovapd, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vmovapd, kX86InstIdVmovapd, X86Mem, X86YmmReg)

  //! Move 128-bits of aligned packed SP-FP (AVX).
  INST_2x(vmovaps, kX86InstIdVmovaps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovaps, kX86InstIdVmovaps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovaps, kX86InstIdVmovaps, X86Mem, X86XmmReg)
  //! Move 256-bits of aligned packed SP-FP (AVX).
  INST_2x(vmovaps, kX86InstIdVmovaps, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovaps, kX86InstIdVmovaps, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vmovaps, kX86InstIdVmovaps, X86Mem, X86YmmReg)

  //! Move DWORD (AVX).
  INST_2x(vmovd, kX86InstIdVmovd, X86XmmReg, X86GpReg)
  //! \overload
  INST_2x(vmovd, kX86InstIdVmovd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovd, kX86InstIdVmovd, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vmovd, kX86InstIdVmovd, X86Mem, X86XmmReg)

  //! Move QWORD (AVX).
  INST_2x(vmovq, kX86InstIdVmovq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovq, kX86InstIdVmovq, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovq, kX86InstIdVmovq, X86Mem, X86XmmReg)

  //! Move QWORD (AVX and X64 Only).
  INST_2x(vmovq, kX86InstIdVmovq, X86XmmReg, X86GpReg)
  //! \overload
  INST_2x(vmovq, kX86InstIdVmovq, X86GpReg, X86XmmReg)

  //! Move one DP-FP and duplicate (AVX).
  INST_2x(vmovddup, kX86InstIdVmovddup, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovddup, kX86InstIdVmovddup, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovddup, kX86InstIdVmovddup, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovddup, kX86InstIdVmovddup, X86YmmReg, X86Mem)

  //! Move 128-bits aligned (AVX).
  INST_2x(vmovdqa, kX86InstIdVmovdqa, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovdqa, kX86InstIdVmovdqa, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovdqa, kX86InstIdVmovdqa, X86Mem, X86XmmReg)
  //! Move 256-bits aligned (AVX).
  INST_2x(vmovdqa, kX86InstIdVmovdqa, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovdqa, kX86InstIdVmovdqa, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vmovdqa, kX86InstIdVmovdqa, X86Mem, X86YmmReg)

  //! Move 128-bits unaligned (AVX).
  INST_2x(vmovdqu, kX86InstIdVmovdqu, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovdqu, kX86InstIdVmovdqu, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovdqu, kX86InstIdVmovdqu, X86Mem, X86XmmReg)
  //! Move 256-bits unaligned (AVX).
  INST_2x(vmovdqu, kX86InstIdVmovdqu, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovdqu, kX86InstIdVmovdqu, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vmovdqu, kX86InstIdVmovdqu, X86Mem, X86YmmReg)

  //! High to low packed SP-FP (AVX).
  INST_3x(vmovhlps, kX86InstIdVmovhlps, X86XmmReg, X86XmmReg, X86XmmReg)

  //! Move high packed DP-FP (AVX).
  INST_3x(vmovhpd, kX86InstIdVmovhpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovhpd, kX86InstIdVmovhpd, X86Mem, X86XmmReg)

  //! Move high packed SP-FP (AVX).
  INST_3x(vmovhps, kX86InstIdVmovhps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovhps, kX86InstIdVmovhps, X86Mem, X86XmmReg)

  //! Move low to high packed SP-FP (AVX).
  INST_3x(vmovlhps, kX86InstIdVmovlhps, X86XmmReg, X86XmmReg, X86XmmReg)

  //! Move low packed DP-FP (AVX).
  INST_3x(vmovlpd, kX86InstIdVmovlpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovlpd, kX86InstIdVmovlpd, X86Mem, X86XmmReg)

  //! Move low packed SP-FP (AVX).
  INST_3x(vmovlps, kX86InstIdVmovlps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovlps, kX86InstIdVmovlps, X86Mem, X86XmmReg)

  //! Extract packed DP-FP sign mask (AVX).
  INST_2x(vmovmskpd, kX86InstIdVmovmskpd, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vmovmskpd, kX86InstIdVmovmskpd, X86GpReg, X86YmmReg)

  //! Extract packed SP-FP sign mask (AVX).
  INST_2x(vmovmskps, kX86InstIdVmovmskps, X86GpReg, X86XmmReg)
  //! \overload
  INST_2x(vmovmskps, kX86InstIdVmovmskps, X86GpReg, X86YmmReg)

  //! Store 128-bits using NT hint (AVX).
  INST_2x(vmovntdq, kX86InstIdVmovntdq, X86Mem, X86XmmReg)
  //! Store 256-bits using NT hint (AVX).
  INST_2x(vmovntdq, kX86InstIdVmovntdq, X86Mem, X86YmmReg)

  //! Store 128-bits aligned using NT hint (AVX).
  INST_2x(vmovntdqa, kX86InstIdVmovntdqa, X86XmmReg, X86Mem)

  //! Store packed DP-FP (128-bits) using NT hint (AVX).
  INST_2x(vmovntpd, kX86InstIdVmovntpd, X86Mem, X86XmmReg)
  //! Store packed DP-FP (256-bits) using NT hint (AVX).
  INST_2x(vmovntpd, kX86InstIdVmovntpd, X86Mem, X86YmmReg)

  //! Store packed SP-FP (128-bits) using NT hint (AVX).
  INST_2x(vmovntps, kX86InstIdVmovntps, X86Mem, X86XmmReg)
  //! Store packed SP-FP (256-bits) using NT hint (AVX).
  INST_2x(vmovntps, kX86InstIdVmovntps, X86Mem, X86YmmReg)

  //! Move scalar DP-FP (AVX).
  INST_3x(vmovsd, kX86InstIdVmovsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovsd, kX86InstIdVmovsd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovsd, kX86InstIdVmovsd, X86Mem, X86XmmReg)

  //! Move packed SP-FP high and duplicate (AVX).
  INST_2x(vmovshdup, kX86InstIdVmovshdup, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovshdup, kX86InstIdVmovshdup, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovshdup, kX86InstIdVmovshdup, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovshdup, kX86InstIdVmovshdup, X86YmmReg, X86Mem)

  //! Move packed SP-FP low and duplicate (AVX).
  INST_2x(vmovsldup, kX86InstIdVmovsldup, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovsldup, kX86InstIdVmovsldup, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovsldup, kX86InstIdVmovsldup, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovsldup, kX86InstIdVmovsldup, X86YmmReg, X86Mem)

  //! Move scalar SP-FP (AVX).
  INST_3x(vmovss, kX86InstIdVmovss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovss, kX86InstIdVmovss, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovss, kX86InstIdVmovss, X86Mem, X86XmmReg)

  //! Move 128-bits of unaligned packed DP-FP (AVX).
  INST_2x(vmovupd, kX86InstIdVmovupd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovupd, kX86InstIdVmovupd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovupd, kX86InstIdVmovupd, X86Mem, X86XmmReg)
  //! Move 256-bits of unaligned packed DP-FP (AVX).
  INST_2x(vmovupd, kX86InstIdVmovupd, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovupd, kX86InstIdVmovupd, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vmovupd, kX86InstIdVmovupd, X86Mem, X86YmmReg)

  //! Move 128-bits of unaligned packed SP-FP (AVX).
  INST_2x(vmovups, kX86InstIdVmovups, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vmovups, kX86InstIdVmovups, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vmovups, kX86InstIdVmovups, X86Mem, X86XmmReg)
  //! Move 256-bits of unaligned packed SP-FP (AVX).
  INST_2x(vmovups, kX86InstIdVmovups, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vmovups, kX86InstIdVmovups, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vmovups, kX86InstIdVmovups, X86Mem, X86YmmReg)

  //! Packed WORD sums of absolute difference (AVX).
  INST_4i(vmpsadbw, kX86InstIdVmpsadbw, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vmpsadbw, kX86InstIdVmpsadbw, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Packed DP-FP multiply (AVX).
  INST_3x(vmulpd, kX86InstIdVmulpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmulpd, kX86InstIdVmulpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vmulpd, kX86InstIdVmulpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vmulpd, kX86InstIdVmulpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP multiply (AVX).
  INST_3x(vmulps, kX86InstIdVmulps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmulps, kX86InstIdVmulps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vmulps, kX86InstIdVmulps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vmulps, kX86InstIdVmulps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP multiply (AVX).
  INST_3x(vmulsd, kX86InstIdVmulsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmulsd, kX86InstIdVmulsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP multiply (AVX).
  INST_3x(vmulss, kX86InstIdVmulss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vmulss, kX86InstIdVmulss, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DP-FP bitwise or (AVX).
  INST_3x(vorpd, kX86InstIdVorpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vorpd, kX86InstIdVorpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vorpd, kX86InstIdVorpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vorpd, kX86InstIdVorpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP bitwise or (AVX).
  INST_3x(vorps, kX86InstIdVorps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vorps, kX86InstIdVorps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vorps, kX86InstIdVorps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vorps, kX86InstIdVorps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTE absolute value (AVX).
  INST_2x(vpabsb, kX86InstIdVpabsb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpabsb, kX86InstIdVpabsb, X86XmmReg, X86Mem)

  //! Packed DWORD absolute value (AVX).
  INST_2x(vpabsd, kX86InstIdVpabsd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpabsd, kX86InstIdVpabsd, X86XmmReg, X86Mem)

  //! Packed WORD absolute value (AVX).
  INST_2x(vpabsw, kX86InstIdVpabsw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpabsw, kX86InstIdVpabsw, X86XmmReg, X86Mem)

  //! Pack DWORDs to WORDs with signed saturation (AVX).
  INST_3x(vpackssdw, kX86InstIdVpackssdw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpackssdw, kX86InstIdVpackssdw, X86XmmReg, X86XmmReg, X86Mem)

  //! Pack WORDs to BYTEs with signed saturation (AVX).
  INST_3x(vpacksswb, kX86InstIdVpacksswb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpacksswb, kX86InstIdVpacksswb, X86XmmReg, X86XmmReg, X86Mem)

  //! Pack DWORDs to WORDs with unsigned saturation (AVX).
  INST_3x(vpackusdw, kX86InstIdVpackusdw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpackusdw, kX86InstIdVpackusdw, X86XmmReg, X86XmmReg, X86Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (AVX).
  INST_3x(vpackuswb, kX86InstIdVpackuswb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpackuswb, kX86InstIdVpackuswb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE add (AVX).
  INST_3x(vpaddb, kX86InstIdVpaddb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddb, kX86InstIdVpaddb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD add (AVX).
  INST_3x(vpaddd, kX86InstIdVpaddd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddd, kX86InstIdVpaddd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed QWORD add (AVX).
  INST_3x(vpaddq, kX86InstIdVpaddq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddq, kX86InstIdVpaddq, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD add (AVX).
  INST_3x(vpaddw, kX86InstIdVpaddw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddw, kX86InstIdVpaddw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE add with saturation (AVX).
  INST_3x(vpaddsb, kX86InstIdVpaddsb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddsb, kX86InstIdVpaddsb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD add with saturation (AVX).
  INST_3x(vpaddsw, kX86InstIdVpaddsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddsw, kX86InstIdVpaddsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE add with unsigned saturation (AVX).
  INST_3x(vpaddusb, kX86InstIdVpaddusb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddusb, kX86InstIdVpaddusb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD add with unsigned saturation (AVX).
  INST_3x(vpaddusw, kX86InstIdVpaddusw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpaddusw, kX86InstIdVpaddusw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed align right (AVX).
  INST_4i(vpalignr, kX86InstIdVpalignr, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vpalignr, kX86InstIdVpalignr, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Packed bitwise and (AVX).
  INST_3x(vpand, kX86InstIdVpand, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpand, kX86InstIdVpand, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed bitwise and-not (AVX).
  INST_3x(vpandn, kX86InstIdVpandn, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpandn, kX86InstIdVpandn, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE average (AVX).
  INST_3x(vpavgb, kX86InstIdVpavgb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpavgb, kX86InstIdVpavgb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD average (AVX).
  INST_3x(vpavgw, kX86InstIdVpavgw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpavgw, kX86InstIdVpavgw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE variable blend (AVX).
  INST_4x(vpblendvb, kX86InstIdVpblendvb, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_4x(vpblendvb, kX86InstIdVpblendvb, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)

  //! Packed WORD blend (AVX).
  INST_4i(vpblendw, kX86InstIdVpblendw, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vpblendw, kX86InstIdVpblendw, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Packed BYTEs compare for equality (AVX).
  INST_3x(vpcmpeqb, kX86InstIdVpcmpeqb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpeqb, kX86InstIdVpcmpeqb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORDs compare for equality (AVX).
  INST_3x(vpcmpeqd, kX86InstIdVpcmpeqd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpeqd, kX86InstIdVpcmpeqd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed QWORDs compare for equality (AVX).
  INST_3x(vpcmpeqq, kX86InstIdVpcmpeqq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpeqq, kX86InstIdVpcmpeqq, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORDs compare for equality (AVX).
  INST_3x(vpcmpeqw, kX86InstIdVpcmpeqw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpeqw, kX86InstIdVpcmpeqw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTEs compare if greater than (AVX).
  INST_3x(vpcmpgtb, kX86InstIdVpcmpgtb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpgtb, kX86InstIdVpcmpgtb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORDs compare if greater than (AVX).
  INST_3x(vpcmpgtd, kX86InstIdVpcmpgtd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpgtd, kX86InstIdVpcmpgtd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed QWORDs compare if greater than (AVX).
  INST_3x(vpcmpgtq, kX86InstIdVpcmpgtq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpgtq, kX86InstIdVpcmpgtq, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORDs compare if greater than (AVX).
  INST_3x(vpcmpgtw, kX86InstIdVpcmpgtw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpcmpgtw, kX86InstIdVpcmpgtw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed compare explicit length strings, return index (AVX).
  INST_3i(vpcmpestri, kX86InstIdVpcmpestri, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpestri, kX86InstIdVpcmpestri, X86XmmReg, X86Mem, Imm)

  //! Packed compare explicit length strings, return mask (AVX).
  INST_3i(vpcmpestrm, kX86InstIdVpcmpestrm, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpestrm, kX86InstIdVpcmpestrm, X86XmmReg, X86Mem, Imm)

  //! Packed compare implicit length strings, return index (AVX).
  INST_3i(vpcmpistri, kX86InstIdVpcmpistri, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpistri, kX86InstIdVpcmpistri, X86XmmReg, X86Mem, Imm)

  //! Packed compare implicit length strings, return mask (AVX).
  INST_3i(vpcmpistrm, kX86InstIdVpcmpistrm, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpcmpistrm, kX86InstIdVpcmpistrm, X86XmmReg, X86Mem, Imm)

  //! Packed DP-FP permute (AVX).
  INST_3x(vpermilpd, kX86InstIdVpermilpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpermilpd, kX86InstIdVpermilpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vpermilpd, kX86InstIdVpermilpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpermilpd, kX86InstIdVpermilpd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3i(vpermilpd, kX86InstIdVpermilpd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpermilpd, kX86InstIdVpermilpd, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vpermilpd, kX86InstIdVpermilpd, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vpermilpd, kX86InstIdVpermilpd, X86YmmReg, X86Mem, Imm)

  //! Packed SP-FP permute (AVX).
  INST_3x(vpermilps, kX86InstIdVpermilps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpermilps, kX86InstIdVpermilps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vpermilps, kX86InstIdVpermilps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpermilps, kX86InstIdVpermilps, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3i(vpermilps, kX86InstIdVpermilps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpermilps, kX86InstIdVpermilps, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vpermilps, kX86InstIdVpermilps, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vpermilps, kX86InstIdVpermilps, X86YmmReg, X86Mem, Imm)

  //! Packed 128-bit FP permute (AVX).
  INST_4i(vperm2f128, kX86InstIdVperm2f128, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vperm2f128, kX86InstIdVperm2f128, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Extract BYTE (AVX).
  INST_3i(vpextrb, kX86InstIdVpextrb, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpextrb, kX86InstIdVpextrb, X86Mem, X86XmmReg, Imm)

  //! Extract DWORD (AVX).
  INST_3i(vpextrd, kX86InstIdVpextrd, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpextrd, kX86InstIdVpextrd, X86Mem, X86XmmReg, Imm)

  //! Extract QWORD (AVX and X64 Only).
  INST_3i(vpextrq, kX86InstIdVpextrq, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpextrq, kX86InstIdVpextrq, X86Mem, X86XmmReg, Imm)

  //! Extract WORD (AVX).
  INST_3i(vpextrw, kX86InstIdVpextrw, X86GpReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpextrw, kX86InstIdVpextrw, X86Mem, X86XmmReg, Imm)

  //! Packed DWORD horizontal add (AVX).
  INST_3x(vphaddd, kX86InstIdVphaddd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vphaddd, kX86InstIdVphaddd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD horizontal add with saturation (AVX).
  INST_3x(vphaddsw, kX86InstIdVphaddsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vphaddsw, kX86InstIdVphaddsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD horizontal add (AVX).
  INST_3x(vphaddw, kX86InstIdVphaddw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vphaddw, kX86InstIdVphaddw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD horizontal minimum (AVX).
  INST_2x(vphminposuw, kX86InstIdVphminposuw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vphminposuw, kX86InstIdVphminposuw, X86XmmReg, X86Mem)

  //! Packed DWORD horizontal subtract (AVX).
  INST_3x(vphsubd, kX86InstIdVphsubd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vphsubd, kX86InstIdVphsubd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD horizontal subtract with saturation (AVX).
  INST_3x(vphsubsw, kX86InstIdVphsubsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vphsubsw, kX86InstIdVphsubsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD horizontal subtract (AVX).
  INST_3x(vphsubw, kX86InstIdVphsubw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vphsubw, kX86InstIdVphsubw, X86XmmReg, X86XmmReg, X86Mem)

  //! Insert BYTE based on selector (AVX).
  INST_4i(vpinsrb, kX86InstIdVpinsrb, X86XmmReg, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_4i(vpinsrb, kX86InstIdVpinsrb, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Insert DWORD based on selector (AVX).
  INST_4i(vpinsrd, kX86InstIdVpinsrd, X86XmmReg, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_4i(vpinsrd, kX86InstIdVpinsrd, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Insert QWORD based on selector (AVX and X64 Only).
  INST_4i(vpinsrq, kX86InstIdVpinsrq, X86XmmReg, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_4i(vpinsrq, kX86InstIdVpinsrq, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Insert WORD based on selector (AVX).
  INST_4i(vpinsrw, kX86InstIdVpinsrw, X86XmmReg, X86XmmReg, X86GpReg, Imm)
  //! \overload
  INST_4i(vpinsrw, kX86InstIdVpinsrw, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Packed multiply and add signed and unsigned bytes (AVX).
  INST_3x(vpmaddubsw, kX86InstIdVpmaddubsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaddubsw, kX86InstIdVpmaddubsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD multiply and add to packed DWORD (AVX).
  INST_3x(vpmaddwd, kX86InstIdVpmaddwd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaddwd, kX86InstIdVpmaddwd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE maximum (AVX).
  INST_3x(vpmaxsb, kX86InstIdVpmaxsb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaxsb, kX86InstIdVpmaxsb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD maximum (AVX).
  INST_3x(vpmaxsd, kX86InstIdVpmaxsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaxsd, kX86InstIdVpmaxsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD maximum (AVX).
  INST_3x(vpmaxsw, kX86InstIdVpmaxsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaxsw, kX86InstIdVpmaxsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE unsigned maximum (AVX).
  INST_3x(vpmaxub, kX86InstIdVpmaxub, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaxub, kX86InstIdVpmaxub, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD unsigned maximum (AVX).
  INST_3x(vpmaxud, kX86InstIdVpmaxud, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaxud, kX86InstIdVpmaxud, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD unsigned maximum (AVX).
  INST_3x(vpmaxuw, kX86InstIdVpmaxuw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmaxuw, kX86InstIdVpmaxuw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE minimum (AVX).
  INST_3x(vpminsb, kX86InstIdVpminsb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpminsb, kX86InstIdVpminsb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD minimum (AVX).
  INST_3x(vpminsd, kX86InstIdVpminsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpminsd, kX86InstIdVpminsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD minimum (AVX).
  INST_3x(vpminsw, kX86InstIdVpminsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpminsw, kX86InstIdVpminsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE unsigned minimum (AVX).
  INST_3x(vpminub, kX86InstIdVpminub, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpminub, kX86InstIdVpminub, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD unsigned minimum (AVX).
  INST_3x(vpminud, kX86InstIdVpminud, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpminud, kX86InstIdVpminud, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD unsigned minimum (AVX).
  INST_3x(vpminuw, kX86InstIdVpminuw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpminuw, kX86InstIdVpminuw, X86XmmReg, X86XmmReg, X86Mem)

  //! Move Byte mask to integer (AVX).
  INST_2x(vpmovmskb, kX86InstIdVpmovmskb, X86GpReg, X86XmmReg)

  //! BYTE to DWORD with sign extend (AVX).
  INST_2x(vpmovsxbd, kX86InstIdVpmovsxbd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovsxbd, kX86InstIdVpmovsxbd, X86XmmReg, X86Mem)

  //! Packed BYTE to QWORD with sign extend (AVX).
  INST_2x(vpmovsxbq, kX86InstIdVpmovsxbq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovsxbq, kX86InstIdVpmovsxbq, X86XmmReg, X86Mem)

  //! Packed BYTE to WORD with sign extend (AVX).
  INST_2x(vpmovsxbw, kX86InstIdVpmovsxbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovsxbw, kX86InstIdVpmovsxbw, X86XmmReg, X86Mem)

  //! Packed DWORD to QWORD with sign extend (AVX).
  INST_2x(vpmovsxdq, kX86InstIdVpmovsxdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovsxdq, kX86InstIdVpmovsxdq, X86XmmReg, X86Mem)

  //! Packed WORD to DWORD with sign extend (AVX).
  INST_2x(vpmovsxwd, kX86InstIdVpmovsxwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovsxwd, kX86InstIdVpmovsxwd, X86XmmReg, X86Mem)

  //! Packed WORD to QWORD with sign extend (AVX).
  INST_2x(vpmovsxwq, kX86InstIdVpmovsxwq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovsxwq, kX86InstIdVpmovsxwq, X86XmmReg, X86Mem)

  //! BYTE to DWORD with zero extend (AVX).
  INST_2x(vpmovzxbd, kX86InstIdVpmovzxbd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovzxbd, kX86InstIdVpmovzxbd, X86XmmReg, X86Mem)

  //! Packed BYTE to QWORD with zero extend (AVX).
  INST_2x(vpmovzxbq, kX86InstIdVpmovzxbq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovzxbq, kX86InstIdVpmovzxbq, X86XmmReg, X86Mem)

  //! BYTE to WORD with zero extend (AVX).
  INST_2x(vpmovzxbw, kX86InstIdVpmovzxbw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovzxbw, kX86InstIdVpmovzxbw, X86XmmReg, X86Mem)

  //! Packed DWORD to QWORD with zero extend (AVX).
  INST_2x(vpmovzxdq, kX86InstIdVpmovzxdq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovzxdq, kX86InstIdVpmovzxdq, X86XmmReg, X86Mem)

  //! Packed WORD to DWORD with zero extend (AVX).
  INST_2x(vpmovzxwd, kX86InstIdVpmovzxwd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovzxwd, kX86InstIdVpmovzxwd, X86XmmReg, X86Mem)

  //! Packed WORD to QWORD with zero extend (AVX).
  INST_2x(vpmovzxwq, kX86InstIdVpmovzxwq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpmovzxwq, kX86InstIdVpmovzxwq, X86XmmReg, X86Mem)

  //! Packed DWORD to QWORD multiply (AVX).
  INST_3x(vpmuldq, kX86InstIdVpmuldq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmuldq, kX86InstIdVpmuldq, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD multiply high, round and scale (AVX).
  INST_3x(vpmulhrsw, kX86InstIdVpmulhrsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmulhrsw, kX86InstIdVpmulhrsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD unsigned multiply high (AVX).
  INST_3x(vpmulhuw, kX86InstIdVpmulhuw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmulhuw, kX86InstIdVpmulhuw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD multiply high (AVX).
  INST_3x(vpmulhw, kX86InstIdVpmulhw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmulhw, kX86InstIdVpmulhw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD multiply low (AVX).
  INST_3x(vpmulld, kX86InstIdVpmulld, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmulld, kX86InstIdVpmulld, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORDs multiply low (AVX).
  INST_3x(vpmullw, kX86InstIdVpmullw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmullw, kX86InstIdVpmullw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD multiply to QWORD (AVX).
  INST_3x(vpmuludq, kX86InstIdVpmuludq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpmuludq, kX86InstIdVpmuludq, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed bitwise or (AVX).
  INST_3x(vpor, kX86InstIdVpor, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpor, kX86InstIdVpor, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD sum of absolute differences (AVX).
  INST_3x(vpsadbw, kX86InstIdVpsadbw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsadbw, kX86InstIdVpsadbw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE shuffle (AVX).
  INST_3x(vpshufb, kX86InstIdVpshufb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpshufb, kX86InstIdVpshufb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD shuffle (AVX).
  INST_3i(vpshufd, kX86InstIdVpshufd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpshufd, kX86InstIdVpshufd, X86XmmReg, X86Mem, Imm)

  //! Packed WORD shuffle high (AVX).
  INST_3i(vpshufhw, kX86InstIdVpshufhw, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpshufhw, kX86InstIdVpshufhw, X86XmmReg, X86Mem, Imm)

  //! Packed WORD shuffle low (AVX).
  INST_3i(vpshuflw, kX86InstIdVpshuflw, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vpshuflw, kX86InstIdVpshuflw, X86XmmReg, X86Mem, Imm)

  //! Packed BYTE sign (AVX).
  INST_3x(vpsignb, kX86InstIdVpsignb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsignb, kX86InstIdVpsignb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD sign (AVX).
  INST_3x(vpsignd, kX86InstIdVpsignd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsignd, kX86InstIdVpsignd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD sign (AVX).
  INST_3x(vpsignw, kX86InstIdVpsignw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsignw, kX86InstIdVpsignw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD shift left logical (AVX).
  INST_3x(vpslld, kX86InstIdVpslld, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpslld, kX86InstIdVpslld, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpslld, kX86InstIdVpslld, X86XmmReg, X86XmmReg, Imm)

  //! Packed OWORD shift left logical (AVX).
  INST_3i(vpslldq, kX86InstIdVpslldq, X86XmmReg, X86XmmReg, Imm)

  //! Packed QWORD shift left logical (AVX).
  INST_3x(vpsllq, kX86InstIdVpsllq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsllq, kX86InstIdVpsllq, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsllq, kX86InstIdVpsllq, X86XmmReg, X86XmmReg, Imm)

  //! Packed WORD shift left logical (AVX).
  INST_3x(vpsllw, kX86InstIdVpsllw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsllw, kX86InstIdVpsllw, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsllw, kX86InstIdVpsllw, X86XmmReg, X86XmmReg, Imm)

  //! Packed DWORD shift right arithmetic (AVX).
  INST_3x(vpsrad, kX86InstIdVpsrad, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsrad, kX86InstIdVpsrad, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsrad, kX86InstIdVpsrad, X86XmmReg, X86XmmReg, Imm)

  //! Packed WORD shift right arithmetic (AVX).
  INST_3x(vpsraw, kX86InstIdVpsraw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsraw, kX86InstIdVpsraw, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsraw, kX86InstIdVpsraw, X86XmmReg, X86XmmReg, Imm)

  //! Packed DWORD shift right logical (AVX).
  INST_3x(vpsrld, kX86InstIdVpsrld, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsrld, kX86InstIdVpsrld, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsrld, kX86InstIdVpsrld, X86XmmReg, X86XmmReg, Imm)

  //! Scalar OWORD shift right logical (AVX).
  INST_3i(vpsrldq, kX86InstIdVpsrldq, X86XmmReg, X86XmmReg, Imm)

  //! Packed QWORD shift right logical (AVX).
  INST_3x(vpsrlq, kX86InstIdVpsrlq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsrlq, kX86InstIdVpsrlq, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsrlq, kX86InstIdVpsrlq, X86XmmReg, X86XmmReg, Imm)

  //! Packed WORD shift right logical (AVX).
  INST_3x(vpsrlw, kX86InstIdVpsrlw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsrlw, kX86InstIdVpsrlw, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3i(vpsrlw, kX86InstIdVpsrlw, X86XmmReg, X86XmmReg, Imm)

  //! Packed BYTE subtract (AVX).
  INST_3x(vpsubb, kX86InstIdVpsubb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubb, kX86InstIdVpsubb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DWORD subtract (AVX).
  INST_3x(vpsubd, kX86InstIdVpsubd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubd, kX86InstIdVpsubd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed QWORD subtract (AVX).
  INST_3x(vpsubq, kX86InstIdVpsubq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubq, kX86InstIdVpsubq, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD subtract (AVX).
  INST_3x(vpsubw, kX86InstIdVpsubw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubw, kX86InstIdVpsubw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE subtract with saturation (AVX).
  INST_3x(vpsubsb, kX86InstIdVpsubsb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubsb, kX86InstIdVpsubsb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD subtract with saturation (AVX).
  INST_3x(vpsubsw, kX86InstIdVpsubsw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubsw, kX86InstIdVpsubsw, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed BYTE subtract with unsigned saturation (AVX).
  INST_3x(vpsubusb, kX86InstIdVpsubusb, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubusb, kX86InstIdVpsubusb, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed WORD subtract with unsigned saturation (AVX).
  INST_3x(vpsubusw, kX86InstIdVpsubusw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpsubusw, kX86InstIdVpsubusw, X86XmmReg, X86XmmReg, X86Mem)

  //! Logical compare (AVX).
  INST_2x(vptest, kX86InstIdVptest, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vptest, kX86InstIdVptest, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vptest, kX86InstIdVptest, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vptest, kX86InstIdVptest, X86YmmReg, X86Mem)

  //! Unpack high packed BYTEs to WORDs (AVX).
  INST_3x(vpunpckhbw, kX86InstIdVpunpckhbw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpckhbw, kX86InstIdVpunpckhbw, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack high packed DWORDs to QWORDs (AVX).
  INST_3x(vpunpckhdq, kX86InstIdVpunpckhdq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpckhdq, kX86InstIdVpunpckhdq, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack high packed QWORDs to OWORD (AVX).
  INST_3x(vpunpckhqdq, kX86InstIdVpunpckhqdq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpckhqdq, kX86InstIdVpunpckhqdq, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack high packed WORDs to DWORDs (AVX).
  INST_3x(vpunpckhwd, kX86InstIdVpunpckhwd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpckhwd, kX86InstIdVpunpckhwd, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack low packed BYTEs to WORDs (AVX).
  INST_3x(vpunpcklbw, kX86InstIdVpunpcklbw, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpcklbw, kX86InstIdVpunpcklbw, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack low packed DWORDs to QWORDs (AVX).
  INST_3x(vpunpckldq, kX86InstIdVpunpckldq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpckldq, kX86InstIdVpunpckldq, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack low packed QWORDs to OWORD (AVX).
  INST_3x(vpunpcklqdq, kX86InstIdVpunpcklqdq, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpcklqdq, kX86InstIdVpunpcklqdq, X86XmmReg, X86XmmReg, X86Mem)

  //! Unpack low packed WORDs to DWORDs (AVX).
  INST_3x(vpunpcklwd, kX86InstIdVpunpcklwd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpunpcklwd, kX86InstIdVpunpcklwd, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed bitwise xor (AVX).
  INST_3x(vpxor, kX86InstIdVpxor, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vpxor, kX86InstIdVpxor, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed SP-FP reciprocal (AVX).
  INST_2x(vrcpps, kX86InstIdVrcpps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vrcpps, kX86InstIdVrcpps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vrcpps, kX86InstIdVrcpps, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vrcpps, kX86InstIdVrcpps, X86YmmReg, X86Mem)

  //! Scalar SP-FP reciprocal (AVX).
  INST_3x(vrcpss, kX86InstIdVrcpss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vrcpss, kX86InstIdVrcpss, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed SP-FP square root reciprocal (AVX).
  INST_2x(vrsqrtps, kX86InstIdVrsqrtps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vrsqrtps, kX86InstIdVrsqrtps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vrsqrtps, kX86InstIdVrsqrtps, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vrsqrtps, kX86InstIdVrsqrtps, X86YmmReg, X86Mem)

  //! Scalar SP-FP square root reciprocal (AVX).
  INST_3x(vrsqrtss, kX86InstIdVrsqrtss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vrsqrtss, kX86InstIdVrsqrtss, X86XmmReg, X86XmmReg, X86Mem)

  //! Packed DP-FP round (AVX).
  INST_3i(vroundpd, kX86InstIdVroundpd, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vroundpd, kX86InstIdVroundpd, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vroundpd, kX86InstIdVroundpd, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vroundpd, kX86InstIdVroundpd, X86YmmReg, X86Mem, Imm)

  //! Packed SP-FP round (AVX).
  INST_3i(vroundps, kX86InstIdVroundps, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vroundps, kX86InstIdVroundps, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vroundps, kX86InstIdVroundps, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vroundps, kX86InstIdVroundps, X86YmmReg, X86Mem, Imm)

  //! Scalar DP-FP round (AVX).
  INST_4i(vroundsd, kX86InstIdVroundsd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vroundsd, kX86InstIdVroundsd, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Scalar SP-FP round (AVX).
  INST_4i(vroundss, kX86InstIdVroundss, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vroundss, kX86InstIdVroundss, X86XmmReg, X86XmmReg, X86Mem, Imm)

  //! Shuffle DP-FP (AVX).
  INST_4i(vshufpd, kX86InstIdVshufpd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vshufpd, kX86InstIdVshufpd, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vshufpd, kX86InstIdVshufpd, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vshufpd, kX86InstIdVshufpd, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Shuffle SP-FP (AVX).
  INST_4i(vshufps, kX86InstIdVshufps, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vshufps, kX86InstIdVshufps, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vshufps, kX86InstIdVshufps, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vshufps, kX86InstIdVshufps, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed DP-FP square root (AVX).
  INST_2x(vsqrtpd, kX86InstIdVsqrtpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vsqrtpd, kX86InstIdVsqrtpd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vsqrtpd, kX86InstIdVsqrtpd, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vsqrtpd, kX86InstIdVsqrtpd, X86YmmReg, X86Mem)

  //! Packed SP-FP square root (AVX).
  INST_2x(vsqrtps, kX86InstIdVsqrtps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vsqrtps, kX86InstIdVsqrtps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vsqrtps, kX86InstIdVsqrtps, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vsqrtps, kX86InstIdVsqrtps, X86YmmReg, X86Mem)

  //! Scalar DP-FP square root (AVX).
  INST_3x(vsqrtsd, kX86InstIdVsqrtsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vsqrtsd, kX86InstIdVsqrtsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP square root (AVX).
  INST_3x(vsqrtss, kX86InstIdVsqrtss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vsqrtss, kX86InstIdVsqrtss, X86XmmReg, X86XmmReg, X86Mem)

  //! Store streaming SIMD extension control/status (AVX).
  INST_1x(vstmxcsr, kX86InstIdVstmxcsr, X86Mem)

  //! Packed DP-FP subtract (AVX).
  INST_3x(vsubpd, kX86InstIdVsubpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vsubpd, kX86InstIdVsubpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vsubpd, kX86InstIdVsubpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vsubpd, kX86InstIdVsubpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP subtract (AVX).
  INST_3x(vsubps, kX86InstIdVsubps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vsubps, kX86InstIdVsubps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vsubps, kX86InstIdVsubps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vsubps, kX86InstIdVsubps, X86YmmReg, X86YmmReg, X86Mem)

  //! Scalar DP-FP subtract (AVX).
  INST_3x(vsubsd, kX86InstIdVsubsd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vsubsd, kX86InstIdVsubsd, X86XmmReg, X86XmmReg, X86Mem)

  //! Scalar SP-FP subtract (AVX).
  INST_3x(vsubss, kX86InstIdVsubss, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vsubss, kX86InstIdVsubss, X86XmmReg, X86XmmReg, X86Mem)

  //! Logical compare DP-FP (AVX).
  INST_2x(vtestpd, kX86InstIdVtestpd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vtestpd, kX86InstIdVtestpd, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vtestpd, kX86InstIdVtestpd, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vtestpd, kX86InstIdVtestpd, X86YmmReg, X86Mem)

  //! Logical compare SP-FP (AVX).
  INST_2x(vtestps, kX86InstIdVtestps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vtestps, kX86InstIdVtestps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vtestps, kX86InstIdVtestps, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vtestps, kX86InstIdVtestps, X86YmmReg, X86Mem)

  //! Scalar DP-FP unordered compare and set EFLAGS (AVX).
  INST_2x(vucomisd, kX86InstIdVucomisd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vucomisd, kX86InstIdVucomisd, X86XmmReg, X86Mem)

  //! Unordered scalar SP-FP compare and set EFLAGS (AVX).
  INST_2x(vucomiss, kX86InstIdVucomiss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vucomiss, kX86InstIdVucomiss, X86XmmReg, X86Mem)

  //! Unpack and interleave high packed DP-FP (AVX).
  INST_3x(vunpckhpd, kX86InstIdVunpckhpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vunpckhpd, kX86InstIdVunpckhpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vunpckhpd, kX86InstIdVunpckhpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vunpckhpd, kX86InstIdVunpckhpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Unpack high packed SP-FP data (AVX).
  INST_3x(vunpckhps, kX86InstIdVunpckhps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vunpckhps, kX86InstIdVunpckhps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vunpckhps, kX86InstIdVunpckhps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vunpckhps, kX86InstIdVunpckhps, X86YmmReg, X86YmmReg, X86Mem)

  //! Unpack and interleave low packed DP-FP (AVX).
  INST_3x(vunpcklpd, kX86InstIdVunpcklpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vunpcklpd, kX86InstIdVunpcklpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vunpcklpd, kX86InstIdVunpcklpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vunpcklpd, kX86InstIdVunpcklpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Unpack low packed SP-FP data (AVX).
  INST_3x(vunpcklps, kX86InstIdVunpcklps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vunpcklps, kX86InstIdVunpcklps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vunpcklps, kX86InstIdVunpcklps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vunpcklps, kX86InstIdVunpcklps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DP-FP bitwise xor (AVX).
  INST_3x(vxorpd, kX86InstIdVxorpd, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vxorpd, kX86InstIdVxorpd, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vxorpd, kX86InstIdVxorpd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vxorpd, kX86InstIdVxorpd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed SP-FP bitwise xor (AVX).
  INST_3x(vxorps, kX86InstIdVxorps, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vxorps, kX86InstIdVxorps, X86XmmReg, X86XmmReg, X86Mem)
  //! \overload
  INST_3x(vxorps, kX86InstIdVxorps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vxorps, kX86InstIdVxorps, X86YmmReg, X86YmmReg, X86Mem)

  //! Zero all Ymm registers.
  INST_0x(vzeroall, kX86InstIdVzeroall)
  //! Zero upper 128-bits of all Ymm registers.
  INST_0x(vzeroupper, kX86InstIdVzeroupper)

  // --------------------------------------------------------------------------
  // [AVX+AESNI]
  // --------------------------------------------------------------------------

  //! Perform a single round of the AES decryption flow (AVX+AESNI).
  INST_3x(vaesdec, kX86InstIdVaesdec, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaesdec, kX86InstIdVaesdec, X86XmmReg, X86XmmReg, X86Mem)

  //! Perform the last round of the AES decryption flow (AVX+AESNI).
  INST_3x(vaesdeclast, kX86InstIdVaesdeclast, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaesdeclast, kX86InstIdVaesdeclast, X86XmmReg, X86XmmReg, X86Mem)

  //! Perform a single round of the AES encryption flow (AVX+AESNI).
  INST_3x(vaesenc, kX86InstIdVaesenc, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaesenc, kX86InstIdVaesenc, X86XmmReg, X86XmmReg, X86Mem)

  //! Perform the last round of the AES encryption flow (AVX+AESNI).
  INST_3x(vaesenclast, kX86InstIdVaesenclast, X86XmmReg, X86XmmReg, X86XmmReg)
  //! \overload
  INST_3x(vaesenclast, kX86InstIdVaesenclast, X86XmmReg, X86XmmReg, X86Mem)

  //! Perform the InvMixColumns transformation (AVX+AESNI).
  INST_2x(vaesimc, kX86InstIdVaesimc, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vaesimc, kX86InstIdVaesimc, X86XmmReg, X86Mem)

  //! Assist in expanding the AES cipher key (AVX+AESNI).
  INST_3i(vaeskeygenassist, kX86InstIdVaeskeygenassist, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vaeskeygenassist, kX86InstIdVaeskeygenassist, X86XmmReg, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [AVX+PCLMULQDQ]
  // --------------------------------------------------------------------------

  //! Carry-less multiplication QWORD (AVX+PCLMULQDQ).
  INST_4i(vpclmulqdq, kX86InstIdVpclmulqdq, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vpclmulqdq, kX86InstIdVpclmulqdq, X86XmmReg, X86XmmReg, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [AVX2]
  // --------------------------------------------------------------------------

  //! Broadcast low 128-bit element in `o1` to `o0` (AVX2).
  INST_2x(vbroadcasti128, kX86InstIdVbroadcasti128, X86YmmReg, X86Mem)
  //! Broadcast low DP-FP element in `o1` to `o0` (AVX2).
  INST_2x(vbroadcastsd, kX86InstIdVbroadcastsd, X86YmmReg, X86XmmReg)
  //! Broadcast low SP-FP element in `o1` to `o0` (AVX2).
  INST_2x(vbroadcastss, kX86InstIdVbroadcastss, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vbroadcastss, kX86InstIdVbroadcastss, X86YmmReg, X86XmmReg)

  //! Extract 128-bit element from `o1` to `o0` based on selector (AVX2).
  INST_3i(vextracti128, kX86InstIdVextracti128, X86XmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vextracti128, kX86InstIdVextracti128, X86Mem, X86YmmReg, Imm)

  //! Gather DP-FP from DWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherdpd, kX86InstIdVgatherdpd, X86XmmReg, X86Mem, X86XmmReg)
  //! \overload
  INST_3x(vgatherdpd, kX86InstIdVgatherdpd, X86YmmReg, X86Mem, X86YmmReg)

  //! Gather SP-FP from DWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherdps, kX86InstIdVgatherdps, X86XmmReg, X86Mem, X86XmmReg)
  //! \overload
  INST_3x(vgatherdps, kX86InstIdVgatherdps, X86YmmReg, X86Mem, X86YmmReg)

  //! Gather DP-FP from QWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherqpd, kX86InstIdVgatherqpd, X86XmmReg, X86Mem, X86XmmReg)
  //! \overload
  INST_3x(vgatherqpd, kX86InstIdVgatherqpd, X86YmmReg, X86Mem, X86YmmReg)

  //! Gather SP-FP from QWORD indicies specified in `o1`s VSIB (AVX2).
  INST_3x(vgatherqps, kX86InstIdVgatherqps, X86XmmReg, X86Mem, X86XmmReg)

  //! Insert 128-bit of packed data based on selector (AVX2).
  INST_4i(vinserti128, kX86InstIdVinserti128, X86YmmReg, X86YmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vinserti128, kX86InstIdVinserti128, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Load 256-bits aligned using NT hint (AVX2).
  INST_2x(vmovntdqa, kX86InstIdVmovntdqa, X86YmmReg, X86Mem)

  //! Packed WORD sums of absolute difference (AVX2).
  INST_4i(vmpsadbw, kX86InstIdVmpsadbw, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vmpsadbw, kX86InstIdVmpsadbw, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed BYTE absolute value (AVX2).
  INST_2x(vpabsb, kX86InstIdVpabsb, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vpabsb, kX86InstIdVpabsb, X86YmmReg, X86Mem)

  //! Packed DWORD absolute value (AVX2).
  INST_2x(vpabsd, kX86InstIdVpabsd, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vpabsd, kX86InstIdVpabsd, X86YmmReg, X86Mem)

  //! Packed WORD absolute value (AVX2).
  INST_2x(vpabsw, kX86InstIdVpabsw, X86YmmReg, X86YmmReg)
  //! \overload
  INST_2x(vpabsw, kX86InstIdVpabsw, X86YmmReg, X86Mem)

  //! Pack DWORDs to WORDs with signed saturation (AVX2).
  INST_3x(vpackssdw, kX86InstIdVpackssdw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpackssdw, kX86InstIdVpackssdw, X86YmmReg, X86YmmReg, X86Mem)

  //! Pack WORDs to BYTEs with signed saturation (AVX2).
  INST_3x(vpacksswb, kX86InstIdVpacksswb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpacksswb, kX86InstIdVpacksswb, X86YmmReg, X86YmmReg, X86Mem)

  //! Pack DWORDs to WORDs with unsigned saturation (AVX2).
  INST_3x(vpackusdw, kX86InstIdVpackusdw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpackusdw, kX86InstIdVpackusdw, X86YmmReg, X86YmmReg, X86Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (AVX2).
  INST_3x(vpackuswb, kX86InstIdVpackuswb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpackuswb, kX86InstIdVpackuswb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTE add (AVX2).
  INST_3x(vpaddb, kX86InstIdVpaddb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddb, kX86InstIdVpaddb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DWORD add (AVX2).
  INST_3x(vpaddd, kX86InstIdVpaddd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddd, kX86InstIdVpaddd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed QDWORD add (AVX2).
  INST_3x(vpaddq, kX86InstIdVpaddq, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddq, kX86InstIdVpaddq, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed WORD add (AVX2).
  INST_3x(vpaddw, kX86InstIdVpaddw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddw, kX86InstIdVpaddw, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTE add with saturation (AVX2).
  INST_3x(vpaddsb, kX86InstIdVpaddsb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddsb, kX86InstIdVpaddsb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed WORD add with saturation (AVX2).
  INST_3x(vpaddsw, kX86InstIdVpaddsw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddsw, kX86InstIdVpaddsw, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTE add with unsigned saturation (AVX2).
  INST_3x(vpaddusb, kX86InstIdVpaddusb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddusb, kX86InstIdVpaddusb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed WORD add with unsigned saturation (AVX2).
  INST_3x(vpaddusw, kX86InstIdVpaddusw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpaddusw, kX86InstIdVpaddusw, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed align right (AVX2).
  INST_4i(vpalignr, kX86InstIdVpalignr, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vpalignr, kX86InstIdVpalignr, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed bitwise and (AVX2).
  INST_3x(vpand, kX86InstIdVpand, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpand, kX86InstIdVpand, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed bitwise and-not (AVX2).
  INST_3x(vpandn, kX86InstIdVpandn, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpandn, kX86InstIdVpandn, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTE average (AVX2).
  INST_3x(vpavgb, kX86InstIdVpavgb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpavgb, kX86InstIdVpavgb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed WORD average (AVX2).
  INST_3x(vpavgw, kX86InstIdVpavgw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpavgw, kX86InstIdVpavgw, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DWORD blend (AVX2).
  INST_4i(vpblendd, kX86InstIdVpblendd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_4i(vpblendd, kX86InstIdVpblendd, X86XmmReg, X86XmmReg, X86Mem, Imm)
  //! \overload
  INST_4i(vpblendd, kX86InstIdVpblendd, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vpblendd, kX86InstIdVpblendd, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed DWORD variable blend (AVX2).
  INST_4x(vpblendvb, kX86InstIdVpblendvb, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_4x(vpblendvb, kX86InstIdVpblendvb, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)

  //! Packed WORD blend (AVX2).
  INST_4i(vpblendw, kX86InstIdVpblendw, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vpblendw, kX86InstIdVpblendw, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Broadcast BYTE from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastb, kX86InstIdVpbroadcastb, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastb, kX86InstIdVpbroadcastb, X86XmmReg, X86Mem)
  //! Broadcast BYTE from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastb, kX86InstIdVpbroadcastb, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastb, kX86InstIdVpbroadcastb, X86YmmReg, X86Mem)

  //! Broadcast DWORD from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastd, kX86InstIdVpbroadcastd, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastd, kX86InstIdVpbroadcastd, X86XmmReg, X86Mem)
  //! Broadcast DWORD from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastd, kX86InstIdVpbroadcastd, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastd, kX86InstIdVpbroadcastd, X86YmmReg, X86Mem)

  //! Broadcast QWORD from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastq, kX86InstIdVpbroadcastq, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastq, kX86InstIdVpbroadcastq, X86XmmReg, X86Mem)
  //! Broadcast QWORD from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastq, kX86InstIdVpbroadcastq, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastq, kX86InstIdVpbroadcastq, X86YmmReg, X86Mem)

  //! Broadcast WORD from `o1` to 128-bits in `o0` (AVX2).
  INST_2x(vpbroadcastw, kX86InstIdVpbroadcastw, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastw, kX86InstIdVpbroadcastw, X86XmmReg, X86Mem)
  //! Broadcast WORD from `o1` to 256-bits in `o0` (AVX2).
  INST_2x(vpbroadcastw, kX86InstIdVpbroadcastw, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vpbroadcastw, kX86InstIdVpbroadcastw, X86YmmReg, X86Mem)

  //! Packed BYTEs compare for equality (AVX2).
  INST_3x(vpcmpeqb, kX86InstIdVpcmpeqb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpeqb, kX86InstIdVpcmpeqb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DWORDs compare for equality (AVX2).
  INST_3x(vpcmpeqd, kX86InstIdVpcmpeqd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpeqd, kX86InstIdVpcmpeqd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed QWORDs compare for equality (AVX2).
  INST_3x(vpcmpeqq, kX86InstIdVpcmpeqq, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpeqq, kX86InstIdVpcmpeqq, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed WORDs compare for equality (AVX2).
  INST_3x(vpcmpeqw, kX86InstIdVpcmpeqw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpeqw, kX86InstIdVpcmpeqw, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTEs compare if greater than (AVX2).
  INST_3x(vpcmpgtb, kX86InstIdVpcmpgtb, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpgtb, kX86InstIdVpcmpgtb, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DWORDs compare if greater than (AVX2).
  INST_3x(vpcmpgtd, kX86InstIdVpcmpgtd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpgtd, kX86InstIdVpcmpgtd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed QWORDs compare if greater than (AVX2).
  INST_3x(vpcmpgtq, kX86InstIdVpcmpgtq, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpgtq, kX86InstIdVpcmpgtq, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed WORDs compare if greater than (AVX2).
  INST_3x(vpcmpgtw, kX86InstIdVpcmpgtw, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpcmpgtw, kX86InstIdVpcmpgtw, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed OWORD permute (AVX2).
  INST_4i(vperm2i128, kX86InstIdVperm2i128, X86YmmReg, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_4i(vperm2i128, kX86InstIdVperm2i128, X86YmmReg, X86YmmReg, X86Mem, Imm)

  //! Packed DWORD permute (AVX2).
  INST_3x(vpermd, kX86InstIdVpermd, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpermd, kX86InstIdVpermd, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed DP-FP permute (AVX2).
  INST_3i(vpermpd, kX86InstIdVpermpd, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vpermpd, kX86InstIdVpermpd, X86YmmReg, X86Mem, Imm)

  //! Packed SP-FP permute (AVX2).
  INST_3x(vpermps, kX86InstIdVpermps, X86YmmReg, X86YmmReg, X86YmmReg)
  //! \overload
  INST_3x(vpermps, kX86InstIdVpermps, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed QWORD permute (AVX2).
  INST_3i(vpermq, kX86InstIdVpermq, X86YmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vpermq, kX86InstIdVpermq, X86YmmReg, X86Mem, Imm)

  INST_3x(vpgatherdd, kX86InstIdVpgatherdd, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpgatherdd, kX86InstIdVpgatherdd, X86YmmReg, X86Mem, X86YmmReg)

  INST_3x(vpgatherdq, kX86InstIdVpgatherdq, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpgatherdq, kX86InstIdVpgatherdq, X86YmmReg, X86Mem, X86YmmReg)

  INST_3x(vpgatherqd, kX86InstIdVpgatherqd, X86XmmReg, X86Mem, X86XmmReg)

  INST_3x(vpgatherqq, kX86InstIdVpgatherqq, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpgatherqq, kX86InstIdVpgatherqq, X86YmmReg, X86Mem, X86YmmReg)

  //! Packed DWORD horizontal add (AVX2).
  INST_3x(vphaddd, kX86InstIdVphaddd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vphaddd, kX86InstIdVphaddd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD horizontal add with saturation (AVX2).
  INST_3x(vphaddsw, kX86InstIdVphaddsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vphaddsw, kX86InstIdVphaddsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD horizontal add (AVX2).
  INST_3x(vphaddw, kX86InstIdVphaddw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vphaddw, kX86InstIdVphaddw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD horizontal subtract (AVX2).
  INST_3x(vphsubd, kX86InstIdVphsubd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vphsubd, kX86InstIdVphsubd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD horizontal subtract with saturation (AVX2).
  INST_3x(vphsubsw, kX86InstIdVphsubsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vphsubsw, kX86InstIdVphsubsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD horizontal subtract (AVX2).
  INST_3x(vphsubw, kX86InstIdVphsubw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vphsubw, kX86InstIdVphsubw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Move Byte mask to integer (AVX2).
  INST_2x(vpmovmskb, kX86InstIdVpmovmskb, X86GpReg, X86YmmReg)

  //! BYTE to DWORD with sign extend (AVX).
  INST_2x(vpmovsxbd, kX86InstIdVpmovsxbd, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovsxbd, kX86InstIdVpmovsxbd, X86YmmReg, X86XmmReg)

  //! Packed BYTE to QWORD with sign extend (AVX2).
  INST_2x(vpmovsxbq, kX86InstIdVpmovsxbq, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovsxbq, kX86InstIdVpmovsxbq, X86YmmReg, X86XmmReg)

  //! Packed BYTE to WORD with sign extend (AVX2).
  INST_2x(vpmovsxbw, kX86InstIdVpmovsxbw, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovsxbw, kX86InstIdVpmovsxbw, X86YmmReg, X86XmmReg)

  //! Packed DWORD to QWORD with sign extend (AVX2).
  INST_2x(vpmovsxdq, kX86InstIdVpmovsxdq, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovsxdq, kX86InstIdVpmovsxdq, X86YmmReg, X86XmmReg)

  //! Packed WORD to DWORD with sign extend (AVX2).
  INST_2x(vpmovsxwd, kX86InstIdVpmovsxwd, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovsxwd, kX86InstIdVpmovsxwd, X86YmmReg, X86XmmReg)

  //! Packed WORD to QWORD with sign extend (AVX2).
  INST_2x(vpmovsxwq, kX86InstIdVpmovsxwq, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovsxwq, kX86InstIdVpmovsxwq, X86YmmReg, X86XmmReg)

  //! BYTE to DWORD with zero extend (AVX2).
  INST_2x(vpmovzxbd, kX86InstIdVpmovzxbd, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovzxbd, kX86InstIdVpmovzxbd, X86YmmReg, X86XmmReg)

  //! Packed BYTE to QWORD with zero extend (AVX2).
  INST_2x(vpmovzxbq, kX86InstIdVpmovzxbq, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovzxbq, kX86InstIdVpmovzxbq, X86YmmReg, X86XmmReg)

  //! BYTE to WORD with zero extend (AVX2).
  INST_2x(vpmovzxbw, kX86InstIdVpmovzxbw, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovzxbw, kX86InstIdVpmovzxbw, X86YmmReg, X86XmmReg)

  //! Packed DWORD to QWORD with zero extend (AVX2).
  INST_2x(vpmovzxdq, kX86InstIdVpmovzxdq, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovzxdq, kX86InstIdVpmovzxdq, X86YmmReg, X86XmmReg)

  //! Packed WORD to DWORD with zero extend (AVX2).
  INST_2x(vpmovzxwd, kX86InstIdVpmovzxwd, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovzxwd, kX86InstIdVpmovzxwd, X86YmmReg, X86XmmReg)

  //! Packed WORD to QWORD with zero extend (AVX2).
  INST_2x(vpmovzxwq, kX86InstIdVpmovzxwq, X86YmmReg, X86Mem)
  //! \overload
  INST_2x(vpmovzxwq, kX86InstIdVpmovzxwq, X86YmmReg, X86XmmReg)

  //! Packed multiply and add signed and unsigned bytes (AVX2).
  INST_3x(vpmaddubsw, kX86InstIdVpmaddubsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaddubsw, kX86InstIdVpmaddubsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD multiply and add to packed DWORD (AVX2).
  INST_3x(vpmaddwd, kX86InstIdVpmaddwd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaddwd, kX86InstIdVpmaddwd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vpmaskmovd, kX86InstIdVpmaskmovd, X86Mem, X86XmmReg, X86XmmReg)
  INST_3x(vpmaskmovd, kX86InstIdVpmaskmovd, X86Mem, X86YmmReg, X86YmmReg)
  INST_3x(vpmaskmovd, kX86InstIdVpmaskmovd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpmaskmovd, kX86InstIdVpmaskmovd, X86YmmReg, X86YmmReg, X86Mem)

  INST_3x(vpmaskmovq, kX86InstIdVpmaskmovq, X86Mem, X86XmmReg, X86XmmReg)
  INST_3x(vpmaskmovq, kX86InstIdVpmaskmovq, X86Mem, X86YmmReg, X86YmmReg)
  INST_3x(vpmaskmovq, kX86InstIdVpmaskmovq, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpmaskmovq, kX86InstIdVpmaskmovq, X86YmmReg, X86YmmReg, X86Mem)

  //! Packed BYTE maximum (AVX2).
  INST_3x(vpmaxsb, kX86InstIdVpmaxsb, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaxsb, kX86InstIdVpmaxsb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD maximum (AVX2).
  INST_3x(vpmaxsd, kX86InstIdVpmaxsd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaxsd, kX86InstIdVpmaxsd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD maximum (AVX2).
  INST_3x(vpmaxsw, kX86InstIdVpmaxsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaxsw, kX86InstIdVpmaxsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed BYTE unsigned maximum (AVX2).
  INST_3x(vpmaxub, kX86InstIdVpmaxub, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaxub, kX86InstIdVpmaxub, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD unsigned maximum (AVX2).
  INST_3x(vpmaxud, kX86InstIdVpmaxud, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaxud, kX86InstIdVpmaxud, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD unsigned maximum (AVX2).
  INST_3x(vpmaxuw, kX86InstIdVpmaxuw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmaxuw, kX86InstIdVpmaxuw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed BYTE minimum (AVX2).
  INST_3x(vpminsb, kX86InstIdVpminsb, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpminsb, kX86InstIdVpminsb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD minimum (AVX2).
  INST_3x(vpminsd, kX86InstIdVpminsd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpminsd, kX86InstIdVpminsd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD minimum (AVX2).
  INST_3x(vpminsw, kX86InstIdVpminsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpminsw, kX86InstIdVpminsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed BYTE unsigned minimum (AVX2).
  INST_3x(vpminub, kX86InstIdVpminub, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpminub, kX86InstIdVpminub, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD unsigned minimum (AVX2).
  INST_3x(vpminud, kX86InstIdVpminud, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpminud, kX86InstIdVpminud, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD unsigned minimum (AVX2).
  INST_3x(vpminuw, kX86InstIdVpminuw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpminuw, kX86InstIdVpminuw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD to QWORD multiply (AVX2).
  INST_3x(vpmuldq, kX86InstIdVpmuldq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmuldq, kX86InstIdVpmuldq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD multiply high, round and scale (AVX2).
  INST_3x(vpmulhrsw, kX86InstIdVpmulhrsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmulhrsw, kX86InstIdVpmulhrsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD unsigned multiply high (AVX2).
  INST_3x(vpmulhuw, kX86InstIdVpmulhuw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmulhuw, kX86InstIdVpmulhuw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD multiply high (AVX2).
  INST_3x(vpmulhw, kX86InstIdVpmulhw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmulhw, kX86InstIdVpmulhw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD multiply low (AVX2).
  INST_3x(vpmulld, kX86InstIdVpmulld, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmulld, kX86InstIdVpmulld, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORDs multiply low (AVX2).
  INST_3x(vpmullw, kX86InstIdVpmullw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmullw, kX86InstIdVpmullw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD multiply to QWORD (AVX2).
  INST_3x(vpmuludq, kX86InstIdVpmuludq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpmuludq, kX86InstIdVpmuludq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed bitwise or (AVX2).
  INST_3x(vpor, kX86InstIdVpor, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpor, kX86InstIdVpor, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD sum of absolute differences (AVX2).
  INST_3x(vpsadbw, kX86InstIdVpsadbw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsadbw, kX86InstIdVpsadbw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed BYTE shuffle (AVX2).
  INST_3x(vpshufb, kX86InstIdVpshufb, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpshufb, kX86InstIdVpshufb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD shuffle (AVX2).
  INST_3i(vpshufd, kX86InstIdVpshufd, X86YmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vpshufd, kX86InstIdVpshufd, X86YmmReg, X86YmmReg, Imm)

  //! Packed WORD shuffle high (AVX2).
  INST_3i(vpshufhw, kX86InstIdVpshufhw, X86YmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vpshufhw, kX86InstIdVpshufhw, X86YmmReg, X86YmmReg, Imm)

  //! Packed WORD shuffle low (AVX2).
  INST_3i(vpshuflw, kX86InstIdVpshuflw, X86YmmReg, X86Mem, Imm)
  //! \overload
  INST_3i(vpshuflw, kX86InstIdVpshuflw, X86YmmReg, X86YmmReg, Imm)

  //! Packed BYTE sign (AVX2).
  INST_3x(vpsignb, kX86InstIdVpsignb, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsignb, kX86InstIdVpsignb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD sign (AVX2).
  INST_3x(vpsignd, kX86InstIdVpsignd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsignd, kX86InstIdVpsignd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD sign (AVX2).
  INST_3x(vpsignw, kX86InstIdVpsignw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsignw, kX86InstIdVpsignw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD shift left logical (AVX2).
  INST_3x(vpslld, kX86InstIdVpslld, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpslld, kX86InstIdVpslld, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpslld, kX86InstIdVpslld, X86YmmReg, X86YmmReg, Imm)

  //! Packed OWORD shift left logical (AVX2).
  INST_3i(vpslldq, kX86InstIdVpslldq, X86YmmReg, X86YmmReg, Imm)

  //! Packed QWORD shift left logical (AVX2).
  INST_3x(vpsllq, kX86InstIdVpsllq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsllq, kX86InstIdVpsllq, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpsllq, kX86InstIdVpsllq, X86YmmReg, X86YmmReg, Imm)

  INST_3x(vpsllvd, kX86InstIdVpsllvd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpsllvd, kX86InstIdVpsllvd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpsllvd, kX86InstIdVpsllvd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vpsllvd, kX86InstIdVpsllvd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vpsllvq, kX86InstIdVpsllvq, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpsllvq, kX86InstIdVpsllvq, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpsllvq, kX86InstIdVpsllvq, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vpsllvq, kX86InstIdVpsllvq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD shift left logical (AVX2).
  INST_3x(vpsllw, kX86InstIdVpsllw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsllw, kX86InstIdVpsllw, X86YmmReg, X86YmmReg, X86XmmReg)
  //! Packed WORD shift left logical (AVX2).
  INST_3i(vpsllw, kX86InstIdVpsllw, X86YmmReg, X86YmmReg, Imm)

  //! Packed DWORD shift right arithmetic (AVX2).
  INST_3x(vpsrad, kX86InstIdVpsrad, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsrad, kX86InstIdVpsrad, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpsrad, kX86InstIdVpsrad, X86YmmReg, X86YmmReg, Imm)

  INST_3x(vpsravd, kX86InstIdVpsravd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpsravd, kX86InstIdVpsravd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpsravd, kX86InstIdVpsravd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vpsravd, kX86InstIdVpsravd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD shift right arithmetic (AVX2).
  INST_3x(vpsraw, kX86InstIdVpsraw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsraw, kX86InstIdVpsraw, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpsraw, kX86InstIdVpsraw, X86YmmReg, X86YmmReg, Imm)

  //! Packed DWORD shift right logical (AVX2).
  INST_3x(vpsrld, kX86InstIdVpsrld, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsrld, kX86InstIdVpsrld, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpsrld, kX86InstIdVpsrld, X86YmmReg, X86YmmReg, Imm)

  //! Scalar OWORD shift right logical (AVX2).
  INST_3i(vpsrldq, kX86InstIdVpsrldq, X86YmmReg, X86YmmReg, Imm)

  //! Packed QWORD shift right logical (AVX2).
  INST_3x(vpsrlq, kX86InstIdVpsrlq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsrlq, kX86InstIdVpsrlq, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpsrlq, kX86InstIdVpsrlq, X86YmmReg, X86YmmReg, Imm)

  INST_3x(vpsrlvd, kX86InstIdVpsrlvd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpsrlvd, kX86InstIdVpsrlvd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpsrlvd, kX86InstIdVpsrlvd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vpsrlvd, kX86InstIdVpsrlvd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vpsrlvq, kX86InstIdVpsrlvq, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vpsrlvq, kX86InstIdVpsrlvq, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpsrlvq, kX86InstIdVpsrlvq, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vpsrlvq, kX86InstIdVpsrlvq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD shift right logical (AVX2).
  INST_3x(vpsrlw, kX86InstIdVpsrlw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsrlw, kX86InstIdVpsrlw, X86YmmReg, X86YmmReg, X86XmmReg)
  //! \overload
  INST_3i(vpsrlw, kX86InstIdVpsrlw, X86YmmReg, X86YmmReg, Imm)

  //! Packed BYTE subtract (AVX2).
  INST_3x(vpsubb, kX86InstIdVpsubb, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vpsubb, kX86InstIdVpsubb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed DWORD subtract (AVX2).
  INST_3x(vpsubd, kX86InstIdVpsubd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubd, kX86InstIdVpsubd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed QWORD subtract (AVX2).
  INST_3x(vpsubq, kX86InstIdVpsubq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubq, kX86InstIdVpsubq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed BYTE subtract with saturation (AVX2).
  INST_3x(vpsubsb, kX86InstIdVpsubsb, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubsb, kX86InstIdVpsubsb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD subtract with saturation (AVX2).
  INST_3x(vpsubsw, kX86InstIdVpsubsw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubsw, kX86InstIdVpsubsw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed BYTE subtract with unsigned saturation (AVX2).
  INST_3x(vpsubusb, kX86InstIdVpsubusb, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubusb, kX86InstIdVpsubusb, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD subtract with unsigned saturation (AVX2).
  INST_3x(vpsubusw, kX86InstIdVpsubusw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubusw, kX86InstIdVpsubusw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed WORD subtract (AVX2).
  INST_3x(vpsubw, kX86InstIdVpsubw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpsubw, kX86InstIdVpsubw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack high packed BYTEs to WORDs (AVX2).
  INST_3x(vpunpckhbw, kX86InstIdVpunpckhbw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpckhbw, kX86InstIdVpunpckhbw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack high packed DWORDs to QWORDs (AVX2).
  INST_3x(vpunpckhdq, kX86InstIdVpunpckhdq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpckhdq, kX86InstIdVpunpckhdq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack high packed QWORDs to OWORD (AVX2).
  INST_3x(vpunpckhqdq, kX86InstIdVpunpckhqdq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpckhqdq, kX86InstIdVpunpckhqdq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack high packed WORDs to DWORDs (AVX2).
  INST_3x(vpunpckhwd, kX86InstIdVpunpckhwd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpckhwd, kX86InstIdVpunpckhwd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack low packed BYTEs to WORDs (AVX2).
  INST_3x(vpunpcklbw, kX86InstIdVpunpcklbw, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpcklbw, kX86InstIdVpunpcklbw, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack low packed DWORDs to QWORDs (AVX2).
  INST_3x(vpunpckldq, kX86InstIdVpunpckldq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpckldq, kX86InstIdVpunpckldq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack low packed QWORDs to OWORD (AVX2).
  INST_3x(vpunpcklqdq, kX86InstIdVpunpcklqdq, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpcklqdq, kX86InstIdVpunpcklqdq, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Unpack low packed WORDs to DWORDs (AVX2).
  INST_3x(vpunpcklwd, kX86InstIdVpunpcklwd, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpunpcklwd, kX86InstIdVpunpcklwd, X86YmmReg, X86YmmReg, X86YmmReg)

  //! Packed bitwise xor (AVX2).
  INST_3x(vpxor, kX86InstIdVpxor, X86YmmReg, X86YmmReg, X86Mem)
  //! \overload
  INST_3x(vpxor, kX86InstIdVpxor, X86YmmReg, X86YmmReg, X86YmmReg)

  // --------------------------------------------------------------------------
  // [FMA3]
  // --------------------------------------------------------------------------

  INST_3x(vfmadd132pd, kX86InstIdVfmadd132pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd132pd, kX86InstIdVfmadd132pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmadd132pd, kX86InstIdVfmadd132pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmadd132pd, kX86InstIdVfmadd132pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmadd132ps, kX86InstIdVfmadd132ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd132ps, kX86InstIdVfmadd132ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmadd132ps, kX86InstIdVfmadd132ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmadd132ps, kX86InstIdVfmadd132ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmadd132sd, kX86InstIdVfmadd132sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd132sd, kX86InstIdVfmadd132sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmadd132ss, kX86InstIdVfmadd132ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd132ss, kX86InstIdVfmadd132ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmadd213pd, kX86InstIdVfmadd213pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd213pd, kX86InstIdVfmadd213pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmadd213pd, kX86InstIdVfmadd213pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmadd213pd, kX86InstIdVfmadd213pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmadd213ps, kX86InstIdVfmadd213ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd213ps, kX86InstIdVfmadd213ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmadd213ps, kX86InstIdVfmadd213ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmadd213ps, kX86InstIdVfmadd213ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmadd213sd, kX86InstIdVfmadd213sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd213sd, kX86InstIdVfmadd213sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmadd213ss, kX86InstIdVfmadd213ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd213ss, kX86InstIdVfmadd213ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmadd231pd, kX86InstIdVfmadd231pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd231pd, kX86InstIdVfmadd231pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmadd231pd, kX86InstIdVfmadd231pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmadd231pd, kX86InstIdVfmadd231pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmadd231ps, kX86InstIdVfmadd231ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd231ps, kX86InstIdVfmadd231ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmadd231ps, kX86InstIdVfmadd231ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmadd231ps, kX86InstIdVfmadd231ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmadd231sd, kX86InstIdVfmadd231sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd231sd, kX86InstIdVfmadd231sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmadd231ss, kX86InstIdVfmadd231ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmadd231ss, kX86InstIdVfmadd231ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmaddsub132pd, kX86InstIdVfmaddsub132pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmaddsub132pd, kX86InstIdVfmaddsub132pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmaddsub132pd, kX86InstIdVfmaddsub132pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmaddsub132pd, kX86InstIdVfmaddsub132pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmaddsub132ps, kX86InstIdVfmaddsub132ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmaddsub132ps, kX86InstIdVfmaddsub132ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmaddsub132ps, kX86InstIdVfmaddsub132ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmaddsub132ps, kX86InstIdVfmaddsub132ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmaddsub213pd, kX86InstIdVfmaddsub213pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmaddsub213pd, kX86InstIdVfmaddsub213pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmaddsub213pd, kX86InstIdVfmaddsub213pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmaddsub213pd, kX86InstIdVfmaddsub213pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmaddsub213ps, kX86InstIdVfmaddsub213ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmaddsub213ps, kX86InstIdVfmaddsub213ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmaddsub213ps, kX86InstIdVfmaddsub213ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmaddsub213ps, kX86InstIdVfmaddsub213ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmaddsub231pd, kX86InstIdVfmaddsub231pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmaddsub231pd, kX86InstIdVfmaddsub231pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmaddsub231pd, kX86InstIdVfmaddsub231pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmaddsub231pd, kX86InstIdVfmaddsub231pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmaddsub231ps, kX86InstIdVfmaddsub231ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmaddsub231ps, kX86InstIdVfmaddsub231ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmaddsub231ps, kX86InstIdVfmaddsub231ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmaddsub231ps, kX86InstIdVfmaddsub231ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub132pd, kX86InstIdVfmsub132pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub132pd, kX86InstIdVfmsub132pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsub132pd, kX86InstIdVfmsub132pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsub132pd, kX86InstIdVfmsub132pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub132ps, kX86InstIdVfmsub132ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub132ps, kX86InstIdVfmsub132ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsub132ps, kX86InstIdVfmsub132ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsub132ps, kX86InstIdVfmsub132ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub132sd, kX86InstIdVfmsub132sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub132sd, kX86InstIdVfmsub132sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmsub132ss, kX86InstIdVfmsub132ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub132ss, kX86InstIdVfmsub132ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmsub213pd, kX86InstIdVfmsub213pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub213pd, kX86InstIdVfmsub213pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsub213pd, kX86InstIdVfmsub213pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsub213pd, kX86InstIdVfmsub213pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub213ps, kX86InstIdVfmsub213ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub213ps, kX86InstIdVfmsub213ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsub213ps, kX86InstIdVfmsub213ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsub213ps, kX86InstIdVfmsub213ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub213sd, kX86InstIdVfmsub213sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub213sd, kX86InstIdVfmsub213sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmsub213ss, kX86InstIdVfmsub213ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub213ss, kX86InstIdVfmsub213ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmsub231pd, kX86InstIdVfmsub231pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub231pd, kX86InstIdVfmsub231pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsub231pd, kX86InstIdVfmsub231pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsub231pd, kX86InstIdVfmsub231pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub231ps, kX86InstIdVfmsub231ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub231ps, kX86InstIdVfmsub231ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsub231ps, kX86InstIdVfmsub231ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsub231ps, kX86InstIdVfmsub231ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsub231sd, kX86InstIdVfmsub231sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub231sd, kX86InstIdVfmsub231sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmsub231ss, kX86InstIdVfmsub231ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsub231ss, kX86InstIdVfmsub231ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfmsubadd132pd, kX86InstIdVfmsubadd132pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsubadd132pd, kX86InstIdVfmsubadd132pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsubadd132pd, kX86InstIdVfmsubadd132pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsubadd132pd, kX86InstIdVfmsubadd132pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsubadd132ps, kX86InstIdVfmsubadd132ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsubadd132ps, kX86InstIdVfmsubadd132ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsubadd132ps, kX86InstIdVfmsubadd132ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsubadd132ps, kX86InstIdVfmsubadd132ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsubadd213pd, kX86InstIdVfmsubadd213pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsubadd213pd, kX86InstIdVfmsubadd213pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsubadd213pd, kX86InstIdVfmsubadd213pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsubadd213pd, kX86InstIdVfmsubadd213pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsubadd213ps, kX86InstIdVfmsubadd213ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsubadd213ps, kX86InstIdVfmsubadd213ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsubadd213ps, kX86InstIdVfmsubadd213ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsubadd213ps, kX86InstIdVfmsubadd213ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsubadd231pd, kX86InstIdVfmsubadd231pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsubadd231pd, kX86InstIdVfmsubadd231pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsubadd231pd, kX86InstIdVfmsubadd231pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsubadd231pd, kX86InstIdVfmsubadd231pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfmsubadd231ps, kX86InstIdVfmsubadd231ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfmsubadd231ps, kX86InstIdVfmsubadd231ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfmsubadd231ps, kX86InstIdVfmsubadd231ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfmsubadd231ps, kX86InstIdVfmsubadd231ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd132pd, kX86InstIdVfnmadd132pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd132pd, kX86InstIdVfnmadd132pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmadd132pd, kX86InstIdVfnmadd132pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmadd132pd, kX86InstIdVfnmadd132pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd132ps, kX86InstIdVfnmadd132ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd132ps, kX86InstIdVfnmadd132ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmadd132ps, kX86InstIdVfnmadd132ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmadd132ps, kX86InstIdVfnmadd132ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd132sd, kX86InstIdVfnmadd132sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd132sd, kX86InstIdVfnmadd132sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmadd132ss, kX86InstIdVfnmadd132ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd132ss, kX86InstIdVfnmadd132ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmadd213pd, kX86InstIdVfnmadd213pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd213pd, kX86InstIdVfnmadd213pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmadd213pd, kX86InstIdVfnmadd213pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmadd213pd, kX86InstIdVfnmadd213pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd213ps, kX86InstIdVfnmadd213ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd213ps, kX86InstIdVfnmadd213ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmadd213ps, kX86InstIdVfnmadd213ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmadd213ps, kX86InstIdVfnmadd213ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd213sd, kX86InstIdVfnmadd213sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd213sd, kX86InstIdVfnmadd213sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmadd213ss, kX86InstIdVfnmadd213ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd213ss, kX86InstIdVfnmadd213ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmadd231pd, kX86InstIdVfnmadd231pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd231pd, kX86InstIdVfnmadd231pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmadd231pd, kX86InstIdVfnmadd231pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmadd231pd, kX86InstIdVfnmadd231pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd231ps, kX86InstIdVfnmadd231ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd231ps, kX86InstIdVfnmadd231ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmadd231ps, kX86InstIdVfnmadd231ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmadd231ps, kX86InstIdVfnmadd231ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmadd231sd, kX86InstIdVfnmadd231sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd231sd, kX86InstIdVfnmadd231sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmadd231ss, kX86InstIdVfnmadd231ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmadd231ss, kX86InstIdVfnmadd231ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmsub132pd, kX86InstIdVfnmsub132pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub132pd, kX86InstIdVfnmsub132pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmsub132pd, kX86InstIdVfnmsub132pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmsub132pd, kX86InstIdVfnmsub132pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmsub132ps, kX86InstIdVfnmsub132ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub132ps, kX86InstIdVfnmsub132ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmsub132ps, kX86InstIdVfnmsub132ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmsub132ps, kX86InstIdVfnmsub132ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmsub132sd, kX86InstIdVfnmsub132sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub132sd, kX86InstIdVfnmsub132sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmsub132ss, kX86InstIdVfnmsub132ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub132ss, kX86InstIdVfnmsub132ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmsub213pd, kX86InstIdVfnmsub213pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub213pd, kX86InstIdVfnmsub213pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmsub213pd, kX86InstIdVfnmsub213pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmsub213pd, kX86InstIdVfnmsub213pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmsub213ps, kX86InstIdVfnmsub213ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub213ps, kX86InstIdVfnmsub213ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmsub213ps, kX86InstIdVfnmsub213ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmsub213ps, kX86InstIdVfnmsub213ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmsub213sd, kX86InstIdVfnmsub213sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub213sd, kX86InstIdVfnmsub213sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmsub213ss, kX86InstIdVfnmsub213ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub213ss, kX86InstIdVfnmsub213ss, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmsub231pd, kX86InstIdVfnmsub231pd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub231pd, kX86InstIdVfnmsub231pd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmsub231pd, kX86InstIdVfnmsub231pd, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmsub231pd, kX86InstIdVfnmsub231pd, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmsub231ps, kX86InstIdVfnmsub231ps, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub231ps, kX86InstIdVfnmsub231ps, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vfnmsub231ps, kX86InstIdVfnmsub231ps, X86YmmReg, X86YmmReg, X86Mem)
  INST_3x(vfnmsub231ps, kX86InstIdVfnmsub231ps, X86YmmReg, X86YmmReg, X86YmmReg)

  INST_3x(vfnmsub231sd, kX86InstIdVfnmsub231sd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub231sd, kX86InstIdVfnmsub231sd, X86XmmReg, X86XmmReg, X86XmmReg)

  INST_3x(vfnmsub231ss, kX86InstIdVfnmsub231ss, X86XmmReg, X86XmmReg, X86Mem)
  INST_3x(vfnmsub231ss, kX86InstIdVfnmsub231ss, X86XmmReg, X86XmmReg, X86XmmReg)

  // --------------------------------------------------------------------------
  // [FMA4]
  // --------------------------------------------------------------------------

  INST_4x(vfmaddpd, kX86InstIdVfmaddpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmaddpd, kX86InstIdVfmaddpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmaddpd, kX86InstIdVfmaddpd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmaddpd, kX86InstIdVfmaddpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmaddpd, kX86InstIdVfmaddpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmaddpd, kX86InstIdVfmaddpd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmaddps, kX86InstIdVfmaddps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmaddps, kX86InstIdVfmaddps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmaddps, kX86InstIdVfmaddps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmaddps, kX86InstIdVfmaddps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmaddps, kX86InstIdVfmaddps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmaddps, kX86InstIdVfmaddps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmaddsd, kX86InstIdVfmaddsd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmaddsd, kX86InstIdVfmaddsd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmaddsd, kX86InstIdVfmaddsd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfmaddss, kX86InstIdVfmaddss, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmaddss, kX86InstIdVfmaddss, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmaddss, kX86InstIdVfmaddss, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfmaddsubpd, kX86InstIdVfmaddsubpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmaddsubpd, kX86InstIdVfmaddsubpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmaddsubpd, kX86InstIdVfmaddsubpd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmaddsubpd, kX86InstIdVfmaddsubpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmaddsubpd, kX86InstIdVfmaddsubpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmaddsubpd, kX86InstIdVfmaddsubpd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmaddsubps, kX86InstIdVfmaddsubps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmaddsubps, kX86InstIdVfmaddsubps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmaddsubps, kX86InstIdVfmaddsubps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmaddsubps, kX86InstIdVfmaddsubps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmaddsubps, kX86InstIdVfmaddsubps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmaddsubps, kX86InstIdVfmaddsubps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmsubaddpd, kX86InstIdVfmsubaddpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmsubaddpd, kX86InstIdVfmsubaddpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmsubaddpd, kX86InstIdVfmsubaddpd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmsubaddpd, kX86InstIdVfmsubaddpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmsubaddpd, kX86InstIdVfmsubaddpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmsubaddpd, kX86InstIdVfmsubaddpd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmsubaddps, kX86InstIdVfmsubaddps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmsubaddps, kX86InstIdVfmsubaddps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmsubaddps, kX86InstIdVfmsubaddps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmsubaddps, kX86InstIdVfmsubaddps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmsubaddps, kX86InstIdVfmsubaddps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmsubaddps, kX86InstIdVfmsubaddps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmsubpd, kX86InstIdVfmsubpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmsubpd, kX86InstIdVfmsubpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmsubpd, kX86InstIdVfmsubpd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmsubpd, kX86InstIdVfmsubpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmsubpd, kX86InstIdVfmsubpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmsubpd, kX86InstIdVfmsubpd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmsubps, kX86InstIdVfmsubps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmsubps, kX86InstIdVfmsubps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmsubps, kX86InstIdVfmsubps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfmsubps, kX86InstIdVfmsubps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfmsubps, kX86InstIdVfmsubps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfmsubps, kX86InstIdVfmsubps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfmsubsd, kX86InstIdVfmsubsd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmsubsd, kX86InstIdVfmsubsd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmsubsd, kX86InstIdVfmsubsd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfmsubss, kX86InstIdVfmsubss, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfmsubss, kX86InstIdVfmsubss, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfmsubss, kX86InstIdVfmsubss, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfnmaddpd, kX86InstIdVfnmaddpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmaddpd, kX86InstIdVfnmaddpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmaddpd, kX86InstIdVfnmaddpd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfnmaddpd, kX86InstIdVfnmaddpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfnmaddpd, kX86InstIdVfnmaddpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfnmaddpd, kX86InstIdVfnmaddpd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfnmaddps, kX86InstIdVfnmaddps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmaddps, kX86InstIdVfnmaddps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmaddps, kX86InstIdVfnmaddps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfnmaddps, kX86InstIdVfnmaddps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfnmaddps, kX86InstIdVfnmaddps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfnmaddps, kX86InstIdVfnmaddps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfnmaddsd, kX86InstIdVfnmaddsd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmaddsd, kX86InstIdVfnmaddsd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmaddsd, kX86InstIdVfnmaddsd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfnmaddss, kX86InstIdVfnmaddss, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmaddss, kX86InstIdVfnmaddss, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmaddss, kX86InstIdVfnmaddss, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfnmsubpd, kX86InstIdVfnmsubpd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmsubpd, kX86InstIdVfnmsubpd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmsubpd, kX86InstIdVfnmsubpd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfnmsubpd, kX86InstIdVfnmsubpd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfnmsubpd, kX86InstIdVfnmsubpd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfnmsubpd, kX86InstIdVfnmsubpd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfnmsubps, kX86InstIdVfnmsubps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmsubps, kX86InstIdVfnmsubps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmsubps, kX86InstIdVfnmsubps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vfnmsubps, kX86InstIdVfnmsubps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vfnmsubps, kX86InstIdVfnmsubps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vfnmsubps, kX86InstIdVfnmsubps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vfnmsubsd, kX86InstIdVfnmsubsd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmsubsd, kX86InstIdVfnmsubsd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmsubsd, kX86InstIdVfnmsubsd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_4x(vfnmsubss, kX86InstIdVfnmsubss, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vfnmsubss, kX86InstIdVfnmsubss, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vfnmsubss, kX86InstIdVfnmsubss, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  // --------------------------------------------------------------------------
  // [XOP]
  // --------------------------------------------------------------------------

  INST_2x(vfrczpd, kX86InstIdVfrczpd, X86XmmReg, X86XmmReg)
  INST_2x(vfrczpd, kX86InstIdVfrczpd, X86XmmReg, X86Mem)
  INST_2x(vfrczpd, kX86InstIdVfrczpd, X86YmmReg, X86YmmReg)
  INST_2x(vfrczpd, kX86InstIdVfrczpd, X86YmmReg, X86Mem)

  INST_2x(vfrczps, kX86InstIdVfrczps, X86XmmReg, X86XmmReg)
  INST_2x(vfrczps, kX86InstIdVfrczps, X86XmmReg, X86Mem)
  INST_2x(vfrczps, kX86InstIdVfrczps, X86YmmReg, X86YmmReg)
  INST_2x(vfrczps, kX86InstIdVfrczps, X86YmmReg, X86Mem)

  INST_2x(vfrczsd, kX86InstIdVfrczsd, X86XmmReg, X86XmmReg)
  INST_2x(vfrczsd, kX86InstIdVfrczsd, X86XmmReg, X86Mem)

  INST_2x(vfrczss, kX86InstIdVfrczss, X86XmmReg, X86XmmReg)
  INST_2x(vfrczss, kX86InstIdVfrczss, X86XmmReg, X86Mem)

  INST_4x(vpcmov, kX86InstIdVpcmov, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpcmov, kX86InstIdVpcmov, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpcmov, kX86InstIdVpcmov, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vpcmov, kX86InstIdVpcmov, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vpcmov, kX86InstIdVpcmov, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vpcmov, kX86InstIdVpcmov, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4i(vpcomb, kX86InstIdVpcomb, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomb, kX86InstIdVpcomb, X86XmmReg, X86XmmReg, X86Mem, Imm)
  INST_4i(vpcomd, kX86InstIdVpcomd, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomd, kX86InstIdVpcomd, X86XmmReg, X86XmmReg, X86Mem, Imm)
  INST_4i(vpcomq, kX86InstIdVpcomq, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomq, kX86InstIdVpcomq, X86XmmReg, X86XmmReg, X86Mem, Imm)
  INST_4i(vpcomw, kX86InstIdVpcomw, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomw, kX86InstIdVpcomw, X86XmmReg, X86XmmReg, X86Mem, Imm)

  INST_4i(vpcomub, kX86InstIdVpcomub, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomub, kX86InstIdVpcomub, X86XmmReg, X86XmmReg, X86Mem, Imm)
  INST_4i(vpcomud, kX86InstIdVpcomud, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomud, kX86InstIdVpcomud, X86XmmReg, X86XmmReg, X86Mem, Imm)
  INST_4i(vpcomuq, kX86InstIdVpcomuq, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomuq, kX86InstIdVpcomuq, X86XmmReg, X86XmmReg, X86Mem, Imm)
  INST_4i(vpcomuw, kX86InstIdVpcomuw, X86XmmReg, X86XmmReg, X86XmmReg, Imm)
  INST_4i(vpcomuw, kX86InstIdVpcomuw, X86XmmReg, X86XmmReg, X86Mem, Imm)

  INST_4x(vpermil2pd, kX86InstIdVpermil2pd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpermil2pd, kX86InstIdVpermil2pd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpermil2pd, kX86InstIdVpermil2pd, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vpermil2pd, kX86InstIdVpermil2pd, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vpermil2pd, kX86InstIdVpermil2pd, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vpermil2pd, kX86InstIdVpermil2pd, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_4x(vpermil2ps, kX86InstIdVpermil2ps, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpermil2ps, kX86InstIdVpermil2ps, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpermil2ps, kX86InstIdVpermil2ps, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)
  INST_4x(vpermil2ps, kX86InstIdVpermil2ps, X86YmmReg, X86YmmReg, X86YmmReg, X86YmmReg)
  INST_4x(vpermil2ps, kX86InstIdVpermil2ps, X86YmmReg, X86YmmReg, X86Mem, X86YmmReg)
  INST_4x(vpermil2ps, kX86InstIdVpermil2ps, X86YmmReg, X86YmmReg, X86YmmReg, X86Mem)

  INST_2x(vphaddbd, kX86InstIdVphaddbd, X86XmmReg, X86XmmReg)
  INST_2x(vphaddbd, kX86InstIdVphaddbd, X86XmmReg, X86Mem)
  INST_2x(vphaddbq, kX86InstIdVphaddbq, X86XmmReg, X86XmmReg)
  INST_2x(vphaddbq, kX86InstIdVphaddbq, X86XmmReg, X86Mem)
  INST_2x(vphaddbw, kX86InstIdVphaddbw, X86XmmReg, X86XmmReg)
  INST_2x(vphaddbw, kX86InstIdVphaddbw, X86XmmReg, X86Mem)
  INST_2x(vphadddq, kX86InstIdVphadddq, X86XmmReg, X86XmmReg)
  INST_2x(vphadddq, kX86InstIdVphadddq, X86XmmReg, X86Mem)
  INST_2x(vphaddwd, kX86InstIdVphaddwd, X86XmmReg, X86XmmReg)
  INST_2x(vphaddwd, kX86InstIdVphaddwd, X86XmmReg, X86Mem)
  INST_2x(vphaddwq, kX86InstIdVphaddwq, X86XmmReg, X86XmmReg)
  INST_2x(vphaddwq, kX86InstIdVphaddwq, X86XmmReg, X86Mem)

  INST_2x(vphaddubd, kX86InstIdVphaddubd, X86XmmReg, X86XmmReg)
  INST_2x(vphaddubd, kX86InstIdVphaddubd, X86XmmReg, X86Mem)
  INST_2x(vphaddubq, kX86InstIdVphaddubq, X86XmmReg, X86XmmReg)
  INST_2x(vphaddubq, kX86InstIdVphaddubq, X86XmmReg, X86Mem)
  INST_2x(vphaddubw, kX86InstIdVphaddubw, X86XmmReg, X86XmmReg)
  INST_2x(vphaddubw, kX86InstIdVphaddubw, X86XmmReg, X86Mem)
  INST_2x(vphaddudq, kX86InstIdVphaddudq, X86XmmReg, X86XmmReg)
  INST_2x(vphaddudq, kX86InstIdVphaddudq, X86XmmReg, X86Mem)
  INST_2x(vphadduwd, kX86InstIdVphadduwd, X86XmmReg, X86XmmReg)
  INST_2x(vphadduwd, kX86InstIdVphadduwd, X86XmmReg, X86Mem)
  INST_2x(vphadduwq, kX86InstIdVphadduwq, X86XmmReg, X86XmmReg)
  INST_2x(vphadduwq, kX86InstIdVphadduwq, X86XmmReg, X86Mem)

  INST_2x(vphsubbw, kX86InstIdVphsubbw, X86XmmReg, X86XmmReg)
  INST_2x(vphsubbw, kX86InstIdVphsubbw, X86XmmReg, X86Mem)
  INST_2x(vphsubdq, kX86InstIdVphsubdq, X86XmmReg, X86XmmReg)
  INST_2x(vphsubdq, kX86InstIdVphsubdq, X86XmmReg, X86Mem)
  INST_2x(vphsubwd, kX86InstIdVphsubwd, X86XmmReg, X86XmmReg)
  INST_2x(vphsubwd, kX86InstIdVphsubwd, X86XmmReg, X86Mem)

  INST_4x(vpmacsdd, kX86InstIdVpmacsdd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacsdd, kX86InstIdVpmacsdd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacsdqh, kX86InstIdVpmacsdqh, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacsdqh, kX86InstIdVpmacsdqh, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacsdql, kX86InstIdVpmacsdql, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacsdql, kX86InstIdVpmacsdql, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacswd, kX86InstIdVpmacswd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacswd, kX86InstIdVpmacswd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacsww, kX86InstIdVpmacsww, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacsww, kX86InstIdVpmacsww, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)

  INST_4x(vpmacssdd, kX86InstIdVpmacssdd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacssdd, kX86InstIdVpmacssdd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacssdqh, kX86InstIdVpmacssdqh, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacssdqh, kX86InstIdVpmacssdqh, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacssdql, kX86InstIdVpmacssdql, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacssdql, kX86InstIdVpmacssdql, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacsswd, kX86InstIdVpmacsswd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacsswd, kX86InstIdVpmacsswd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpmacssww, kX86InstIdVpmacssww, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmacssww, kX86InstIdVpmacssww, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)

  INST_4x(vpmadcsswd, kX86InstIdVpmadcsswd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmadcsswd, kX86InstIdVpmadcsswd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)

  INST_4x(vpmadcswd, kX86InstIdVpmadcswd, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpmadcswd, kX86InstIdVpmadcswd, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)

  INST_4x(vpperm, kX86InstIdVpperm, X86XmmReg, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_4x(vpperm, kX86InstIdVpperm, X86XmmReg, X86XmmReg, X86Mem, X86XmmReg)
  INST_4x(vpperm, kX86InstIdVpperm, X86XmmReg, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vprotb, kX86InstIdVprotb, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vprotb, kX86InstIdVprotb, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vprotb, kX86InstIdVprotb, X86XmmReg, X86XmmReg, X86Mem)
  INST_3i(vprotb, kX86InstIdVprotb, X86XmmReg, X86XmmReg, Imm)
  INST_3i(vprotb, kX86InstIdVprotb, X86XmmReg, X86Mem, Imm)

  INST_3x(vprotd, kX86InstIdVprotd, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vprotd, kX86InstIdVprotd, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vprotd, kX86InstIdVprotd, X86XmmReg, X86XmmReg, X86Mem)
  INST_3i(vprotd, kX86InstIdVprotd, X86XmmReg, X86XmmReg, Imm)
  INST_3i(vprotd, kX86InstIdVprotd, X86XmmReg, X86Mem, Imm)

  INST_3x(vprotq, kX86InstIdVprotq, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vprotq, kX86InstIdVprotq, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vprotq, kX86InstIdVprotq, X86XmmReg, X86XmmReg, X86Mem)
  INST_3i(vprotq, kX86InstIdVprotq, X86XmmReg, X86XmmReg, Imm)
  INST_3i(vprotq, kX86InstIdVprotq, X86XmmReg, X86Mem, Imm)

  INST_3x(vprotw, kX86InstIdVprotw, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vprotw, kX86InstIdVprotw, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vprotw, kX86InstIdVprotw, X86XmmReg, X86XmmReg, X86Mem)
  INST_3i(vprotw, kX86InstIdVprotw, X86XmmReg, X86XmmReg, Imm)
  INST_3i(vprotw, kX86InstIdVprotw, X86XmmReg, X86Mem, Imm)

  INST_3x(vpshab, kX86InstIdVpshab, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshab, kX86InstIdVpshab, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshab, kX86InstIdVpshab, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshad, kX86InstIdVpshad, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshad, kX86InstIdVpshad, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshad, kX86InstIdVpshad, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshaq, kX86InstIdVpshaq, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshaq, kX86InstIdVpshaq, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshaq, kX86InstIdVpshaq, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshaw, kX86InstIdVpshaw, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshaw, kX86InstIdVpshaw, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshaw, kX86InstIdVpshaw, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshlb, kX86InstIdVpshlb, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshlb, kX86InstIdVpshlb, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshlb, kX86InstIdVpshlb, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshld, kX86InstIdVpshld, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshld, kX86InstIdVpshld, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshld, kX86InstIdVpshld, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshlq, kX86InstIdVpshlq, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshlq, kX86InstIdVpshlq, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshlq, kX86InstIdVpshlq, X86XmmReg, X86XmmReg, X86Mem)

  INST_3x(vpshlw, kX86InstIdVpshlw, X86XmmReg, X86XmmReg, X86XmmReg)
  INST_3x(vpshlw, kX86InstIdVpshlw, X86XmmReg, X86Mem, X86XmmReg)
  INST_3x(vpshlw, kX86InstIdVpshlw, X86XmmReg, X86XmmReg, X86Mem)

  // --------------------------------------------------------------------------
  // [BMI]
  // --------------------------------------------------------------------------

  //! Bitwise and-not (BMI).
  INST_3x(andn, kX86InstIdAndn, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(andn, kX86InstIdAndn, X86GpReg, X86GpReg, X86Mem)

  //! Bit field extract (BMI).
  INST_3x(bextr, kX86InstIdBextr, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(bextr, kX86InstIdBextr, X86GpReg, X86Mem, X86GpReg)

  //! Extract lower set isolated bit (BMI).
  INST_2x(blsi, kX86InstIdBlsi, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(blsi, kX86InstIdBlsi, X86GpReg, X86Mem)

  //! Get mask up to lowest set bit (BMI).
  INST_2x(blsmsk, kX86InstIdBlsmsk, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(blsmsk, kX86InstIdBlsmsk, X86GpReg, X86Mem)

  //! Reset lowest set bit (BMI).
  INST_2x(blsr, kX86InstIdBlsr, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(blsr, kX86InstIdBlsr, X86GpReg, X86Mem)

  //! Count the number of trailing zero bits (BMI).
  INST_2x(tzcnt, kX86InstIdTzcnt, X86GpReg, X86GpReg)
  //! \overload
  INST_2x(tzcnt, kX86InstIdTzcnt, X86GpReg, X86Mem)

  // --------------------------------------------------------------------------
  // [BMI2]
  // --------------------------------------------------------------------------

  //! Zero high bits starting with specified bit position (BMI2).
  INST_3x(bzhi, kX86InstIdBzhi, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(bzhi, kX86InstIdBzhi, X86GpReg, X86Mem, X86GpReg)

  //! Unsigned multiply without affecting flags (BMI2).
  INST_3x(mulx, kX86InstIdMulx, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(mulx, kX86InstIdMulx, X86GpReg, X86GpReg, X86Mem)

  //! Parallel bits deposit (BMI2).
  INST_3x(pdep, kX86InstIdPdep, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(pdep, kX86InstIdPdep, X86GpReg, X86GpReg, X86Mem)

  //! Parallel bits extract (BMI2).
  INST_3x(pext, kX86InstIdPext, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(pext, kX86InstIdPext, X86GpReg, X86GpReg, X86Mem)

  //! Rotate right without affecting flags (BMI2).
  INST_3i(rorx, kX86InstIdRorx, X86GpReg, X86GpReg, Imm)
  //! \overload
  INST_3i(rorx, kX86InstIdRorx, X86GpReg, X86Mem, Imm)

  //! Shift arithmetic right without affecting flags (BMI2).
  INST_3x(sarx, kX86InstIdSarx, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(sarx, kX86InstIdSarx, X86GpReg, X86Mem, X86GpReg)

  //! Shift logical left without affecting flags (BMI2).
  INST_3x(shlx, kX86InstIdShlx, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(shlx, kX86InstIdShlx, X86GpReg, X86Mem, X86GpReg)

  //! Shift logical right without affecting flags (BMI2).
  INST_3x(shrx, kX86InstIdShrx, X86GpReg, X86GpReg, X86GpReg)
  //! \overload
  INST_3x(shrx, kX86InstIdShrx, X86GpReg, X86Mem, X86GpReg)

  // --------------------------------------------------------------------------
  // [RDRAND]
  // --------------------------------------------------------------------------

  //! Store a random number in destination register (RDRAND).
  //!
  //! Please do not use this instruction in cryptographic software. The result
  //! doesn't necessarily have to be random, which may cause a major security
  //! hole in the software.
  INST_1x(rdrand, kX86InstIdRdrand, X86GpReg)

  // --------------------------------------------------------------------------
  // [F16C]
  // --------------------------------------------------------------------------

  //! Convert packed HP-FP to SP-FP.
  INST_2x(vcvtph2ps, kX86InstIdVcvtph2ps, X86XmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtph2ps, kX86InstIdVcvtph2ps, X86XmmReg, X86Mem)
  //! \overload
  INST_2x(vcvtph2ps, kX86InstIdVcvtph2ps, X86YmmReg, X86XmmReg)
  //! \overload
  INST_2x(vcvtph2ps, kX86InstIdVcvtph2ps, X86YmmReg, X86Mem)

  //! Convert packed SP-FP to HP-FP.
  INST_3i(vcvtps2ph, kX86InstIdVcvtps2ph, X86XmmReg, X86XmmReg, Imm)
  //! \overload
  INST_3i(vcvtps2ph, kX86InstIdVcvtps2ph, X86Mem, X86XmmReg, Imm)
  //! \overload
  INST_3i(vcvtps2ph, kX86InstIdVcvtps2ph, X86XmmReg, X86YmmReg, Imm)
  //! \overload
  INST_3i(vcvtps2ph, kX86InstIdVcvtps2ph, X86Mem, X86YmmReg, Imm)

  // --------------------------------------------------------------------------
  // [FSGSBASE]
  // --------------------------------------------------------------------------

  INST_1x(rdfsbase, kX86InstIdRdfsbase, X86GpReg)
  INST_1x(rdgsbase, kX86InstIdRdgsbase, X86GpReg)
  INST_1x(wrfsbase, kX86InstIdWrfsbase, X86GpReg)
  INST_1x(wrgsbase, kX86InstIdWrgsbase, X86GpReg)

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
#undef INST_3ii

#undef INST_4x
#undef INST_4x_
#undef INST_4i
#undef INST_4ii
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86ASSEMBLER_H
