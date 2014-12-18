// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86COMPILER_H
#define _ASMJIT_X86_X86COMPILER_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies - AsmJit]
#include "../base/compiler.h"
#include "../base/vectypes.h"
#include "../x86/x86assembler.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct X86CallNode;
struct X86FuncNode;
struct X86VarState;

//! \addtogroup asmjit_x86_compiler
//! \{

// ============================================================================
// [asmjit::k86VarType]
// ============================================================================

//! X86/X64 variable type.
ASMJIT_ENUM(kX86VarType) {
  //! Variable is SP-FP (x87).
  kX86VarTypeFp32 = kVarTypeFp32,
  //! Variable is DP-FP (x87).
  kX86VarTypeFp64 = kVarTypeFp64,

  //! Variable is Mm (MMX).
  kX86VarTypeMm = 12,

  //! Variable is K (AVX512+)
  kX86VarTypeK,

  //! Variable is Xmm (SSE+).
  kX86VarTypeXmm,
  //! Variable is a scalar Xmm SP-FP number.
  kX86VarTypeXmmSs,
  //! Variable is a packed Xmm SP-FP number (4 floats).
  kX86VarTypeXmmPs,
  //! Variable is a scalar Xmm DP-FP number.
  kX86VarTypeXmmSd,
  //! Variable is a packed Xmm DP-FP number (2 doubles).
  kX86VarTypeXmmPd,

  //! Variable is Ymm (AVX+).
  kX86VarTypeYmm,
  //! Variable is a packed Ymm SP-FP number (8 floats).
  kX86VarTypeYmmPs,
  //! Variable is a packed Ymm DP-FP number (4 doubles).
  kX86VarTypeYmmPd,

  //! Variable is Zmm (AVX512+).
  kX86VarTypeZmm,
  //! Variable is a packed Zmm SP-FP number (16 floats).
  kX86VarTypeZmmPs,
  //! Variable is a packed Zmm DP-FP number (8 doubles).
  kX86VarTypeZmmPd,

  //! Count of variable types.
  kX86VarTypeCount,

  //! \internal
  //! \{
  _kX86VarTypeMmStart = kX86VarTypeMm,
  _kX86VarTypeMmEnd = kX86VarTypeMm,

  _kX86VarTypeXmmStart = kX86VarTypeXmm,
  _kX86VarTypeXmmEnd = kX86VarTypeXmmPd,

  _kX86VarTypeYmmStart = kX86VarTypeYmm,
  _kX86VarTypeYmmEnd = kX86VarTypeYmmPd,

  _kX86VarTypeZmmStart = kX86VarTypeZmm,
  _kX86VarTypeZmmEnd = kX86VarTypeZmmPd
  //! \}
};

// ============================================================================
// [asmjit::kX86VarAttr]
// ============================================================================

//! X86/X64 VarAttr flags.
ASMJIT_ENUM(kX86VarAttr) {
  kX86VarAttrGpbLo = 0x10000000,
  kX86VarAttrGpbHi = 0x20000000
};

// ============================================================================
// [asmjit::kX86FuncConv]
// ============================================================================

//! X86 function calling conventions.
//!
//! Calling convention is scheme how function arguments are passed into
//! function and how functions returns values. In assembler programming
//! it's needed to always comply with function calling conventions, because
//! even small inconsistency can cause undefined behavior or crash.
//!
//! List of calling conventions for 32-bit x86 mode:
//! - `kX86FuncConvCDecl` - Calling convention for C runtime.
//! - `kX86FuncConvStdCall` - Calling convention for WinAPI functions.
//! - `kX86FuncConvMsThisCall` - Calling convention for C++ members under
//!    Windows (produced by MSVC and all MSVC compatible compilers).
//! - `kX86FuncConvMsFastCall` - Fastest calling convention that can be used
//!    by MSVC compiler.
//! - `kX86FuncConvBorlandFastCall` - Borland fastcall convention.
//! - `kX86FuncConvGccFastCall` - GCC fastcall convention (2 register arguments).
//! - `kX86FuncConvGccRegParm1` - GCC regparm(1) convention.
//! - `kX86FuncConvGccRegParm2` - GCC regparm(2) convention.
//! - `kX86FuncConvGccRegParm3` - GCC regparm(3) convention.
//!
//! List of calling conventions for 64-bit x86 mode (x64):
//! - `kX86FuncConvW64` - Windows 64-bit calling convention (WIN64 ABI).
//! - `kX86FuncConvU64` - Unix 64-bit calling convention (AMD64 ABI).
//!
//! There is also `kFuncConvHost` that is defined to fit the host calling
//! convention.
//!
//! These types are used together with `Compiler::addFunc()` method.
ASMJIT_ENUM(kX86FuncConv) {
  // --------------------------------------------------------------------------
  // [X64]
  // --------------------------------------------------------------------------

  //! X64 calling convention for Windows platform (WIN64 ABI).
  //!
  //! For first four arguments are used these registers:
  //! - 1. 32/64-bit integer or floating point argument - rcx/xmm0
  //! - 2. 32/64-bit integer or floating point argument - rdx/xmm1
  //! - 3. 32/64-bit integer or floating point argument - r8/xmm2
  //! - 4. 32/64-bit integer or floating point argument - r9/xmm3
  //!
  //! Note first four arguments here means arguments at positions from 1 to 4
  //! (included). For example if second argument is not passed in register then
  //! rdx/xmm1 register is unused.
  //!
  //! All other arguments are pushed on the stack in right-to-left direction.
  //! Stack is aligned by 16 bytes. There is 32-byte shadow space on the stack
  //! that can be used to save up to four 64-bit registers (probably designed to
  //! be used to save first four arguments passed in registers).
  //!
  //! Arguments direction:
  //! - Right to Left (except for first 4 parameters that's in registers)
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - Rax register.
  //! - Floating points - Xmm0 register.
  //!
  //! Stack is always aligned by 16 bytes.
  //!
  //! More information about this calling convention can be found on MSDN:
  //! http://msdn.microsoft.com/en-us/library/9b372w95.aspx .
  kX86FuncConvW64 = 1,

  //! X64 calling convention for Unix platforms (AMD64 ABI).
  //!
  //! First six 32 or 64-bit integer arguments are passed in rdi, rsi, rdx,
  //! rcx, r8, r9 registers. First eight floating point or Xmm arguments
  //! are passed in xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7 registers.
  //! This means that in registers can be transferred up to 14 arguments total.
  //!
  //! There is also RED ZONE below the stack pointer that can be used for
  //! temporary storage. The red zone is the space from [rsp-128] to [rsp-8].
  //!
  //! Arguments direction:
  //! - Right to Left (Except for arguments passed in registers).
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - Rax register.
  //! - Floating points - Xmm0 register.
  //!
  //! Stack is always aligned by 16 bytes.
  kX86FuncConvU64 = 2,

  // --------------------------------------------------------------------------
  // [X86]
  // --------------------------------------------------------------------------

  //! Cdecl calling convention (used by C runtime).
  //!
  //! Compatible across MSVC and GCC.
  //!
  //! Arguments direction:
  //! - Right to Left
  //!
  //! Stack is cleaned by:
  //! - Caller.
  kX86FuncConvCDecl = 3,

  //! Stdcall calling convention (used by WinAPI).
  //!
  //! Compatible across MSVC and GCC.
  //!
  //! Arguments direction:
  //! - Right to Left
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  kX86FuncConvStdCall = 4,

  //! MSVC specific calling convention used by MSVC/Intel compilers
  //! for struct/class methods.
  //!
  //! This is MSVC (and Intel) only calling convention used in Windows
  //! world for C++ class methods. Implicit 'this' pointer is stored in
  //! ECX register instead of storing it on the stack.
  //!
  //! Arguments direction:
  //! - Right to Left (except this pointer in ECX)
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  //!
  //! C++ class methods that have variable count of arguments uses different
  //! calling convention called cdecl.
  //!
  //! \note This calling convention is always used by MSVC for class methods,
  //! it's implicit and there is no way how to override it.
  kX86FuncConvMsThisCall = 5,

  //! MSVC specific fastcall.
  //!
  //! Two first parameters (evaluated from left-to-right) are in ECX:EDX
  //! registers, all others on the stack in right-to-left order.
  //!
  //! Arguments direction:
  //! - Right to Left (except to first two integer arguments in ECX:EDX)
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  //!
  //! \note This calling convention differs to GCC one in stack cleaning
  //! mechanism.
  kX86FuncConvMsFastCall = 6,

  //! Borland specific fastcall with 2 parameters in registers.
  //!
  //! Two first parameters (evaluated from left-to-right) are in ECX:EDX
  //! registers, all others on the stack in left-to-right order.
  //!
  //! Arguments direction:
  //! - Left to Right (except to first two integer arguments in ECX:EDX)
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  //!
  //! \note Arguments on the stack are in left-to-right order that differs
  //! to other fastcall conventions used in different compilers.
  kX86FuncConvBorlandFastCall = 7,

  //! GCC specific fastcall convention.
  //!
  //! Two first parameters (evaluated from left-to-right) are in ECX:EDX
  //! registers, all others on the stack in right-to-left order.
  //!
  //! Arguments direction:
  //! - Right to Left (except to first two integer arguments in ECX:EDX)
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  //!
  //! \note This calling convention should be compatible with `kX86FuncConvMsFastCall`.
  kX86FuncConvGccFastCall = 8,

  //! GCC specific regparm(1) convention.
  //!
  //! The first parameter (evaluated from left-to-right) is in EAX register,
  //! all others on the stack in right-to-left order.
  //!
  //! Arguments direction:
  //! - Right to Left (except to first one integer argument in EAX)
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  kX86FuncConvGccRegParm1 = 9,

  //! GCC specific regparm(2) convention.
  //!
  //! Two first parameters (evaluated from left-to-right) are in EAX:EDX
  //! registers, all others on the stack in right-to-left order.
  //!
  //! Arguments direction:
  //! - Right to Left (except to first two integer arguments in EAX:EDX)
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  kX86FuncConvGccRegParm2 = 10,

  //! GCC specific fastcall with 3 parameters in registers.
  //!
  //! Three first parameters (evaluated from left-to-right) are in
  //! EAX:EDX:ECX registers, all others on the stack in right-to-left order.
  //!
  //! Arguments direction:
  //! - Right to Left (except to first three integer arguments in EAX:EDX:ECX)
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - EAX:EDX registers.
  //! - Floating points - fp0 register.
  kX86FuncConvGccRegParm3 = 11,

  //! \internal
  //!
  //! Count of function calling conventions.
  _kX86FuncConvCount = 12
};

#if !defined(ASMJIT_DOCGEN)
// X86/X64 Host Support - documented in base/compiler.h.
#if defined(ASMJIT_HOST_X86)
enum {
  // X86.
  kFuncConvHost = kX86FuncConvCDecl,
  kFuncConvHostCDecl = kX86FuncConvCDecl,
  kFuncConvHostStdCall = kX86FuncConvStdCall,
#if defined(_MSC_VER)
  kFuncConvHostFastCall = kX86FuncConvMsFastCall
#elif defined(__GNUC__)
  kFuncConvHostFastCall = kX86FuncConvGccFastCall
#elif defined(__BORLANDC__)
  kFuncConvHostFastCall = kX86FuncConvBorlandFastCall
#else
#error "kFuncConvHostFastCall not determined."
#endif
};
#endif // ASMJIT_HOST_X86

#if defined(ASMJIT_HOST_X64)
enum {
#if defined(ASMJIT_OS_WINDOWS)
  kFuncConvHost = kX86FuncConvW64,
#else
  kFuncConvHost = kX86FuncConvU64,
#endif
  kFuncConvHostCDecl = kFuncConvHost,
  kFuncConvHostStdCall = kFuncConvHost,
  kFuncConvHostFastCall = kFuncConvHost
};
#endif // ASMJIT_HOST_X64
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::kX86FuncHint]
// ============================================================================

//! X86 function hints.
ASMJIT_ENUM(kX86FuncHint) {
  //! Use push/pop sequences instead of mov sequences in function prolog
  //! and epilog.
  kX86FuncHintPushPop = 16,
  //! Add emms instruction to the function epilog.
  kX86FuncHintEmms = 17,
  //! Add sfence instruction to the function epilog.
  kX86FuncHintSFence = 18,
  //! Add lfence instruction to the function epilog.
  kX86FuncHintLFence = 19
};

// ============================================================================
// [asmjit::kX86FuncFlags]
// ============================================================================

//! X86 function flags.
ASMJIT_ENUM(kX86FuncFlags) {
  //! Whether to emit register load/save sequence using push/pop pairs.
  kX86FuncFlagPushPop = 0x00010000,

  //! Whether to emit `enter` instead of three instructions in case
  //! that the function is not naked or misaligned.
  kX86FuncFlagEnter = 0x00020000,

  //! Whether to emit `leave` instead of two instructions in case
  //! that the function is not naked or misaligned.
  kX86FuncFlagLeave = 0x00040000,

  //! Whether it's required to move arguments to a new stack location,
  //! because of manual aligning.
  kX86FuncFlagMoveArgs = 0x00080000,

  //! Whether to emit `emms` instruction in epilog (auto-detected).
  kX86FuncFlagEmms = 0x01000000,

  //! Whether to emit `sfence` instruction in epilog (auto-detected).
  //!
  //! `kX86FuncFlagSFence` with `kX86FuncFlagLFence` results in emitting `mfence`.
  kX86FuncFlagSFence = 0x02000000,

  //! Whether to emit `lfence` instruction in epilog (auto-detected).
  //!
  //! `kX86FuncFlagSFence` with `kX86FuncFlagLFence` results in emitting `mfence`.
  kX86FuncFlagLFence = 0x04000000
};

// ============================================================================
// [asmjit::X86VarInfo]
// ============================================================================

//! \internal
//!
//! X86 variable information.
struct X86VarInfo {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get register type, see `kX86RegType`.
  ASMJIT_INLINE uint32_t getReg() const { return _reg; }
  //! Get register size in bytes.
  ASMJIT_INLINE uint32_t getSize() const { return _size; }
  //! Get variable class, see `kRegClass`.
  ASMJIT_INLINE uint32_t getClass() const { return _class; }
  //! Get variable description, see `kVarFlag`.
  ASMJIT_INLINE uint32_t getDesc() const { return _desc; }
  //! Get variable type name.
  ASMJIT_INLINE const char* getName() const { return _name; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Register type, see `kX86RegType`.
  uint8_t _reg;
  //! Register size in bytes.
  uint8_t _size;
  //! Register class, see `kRegClass`.
  uint8_t _class;
  //! Variable flags, see `kVarFlag`.
  uint8_t _desc;
  //! Variable type name.
  char _name[4];
};

//! \internal
ASMJIT_VAR const X86VarInfo _x86VarInfo[];

#if defined(ASMJIT_BUILD_X86)
//! \internal
//!
//! Mapping of x86 variables into their real IDs.
//!
//! This mapping translates the following:
//! - `kVarTypeInt64` to `kInvalidVar`.
//! - `kVarTypeUInt64` to `kInvalidVar`.
//! - `kVarTypeIntPtr` to `kVarTypeInt32`.
//! - `kVarTypeUIntPtr` to `kVarTypeUInt32`.
ASMJIT_VAR const uint8_t _x86VarMapping[kX86VarTypeCount];
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_X64)
//! \internal
//!
//! Mapping of x64 variables into their real IDs.
//!
//! This mapping translates the following:
//! - `kVarTypeIntPtr` to `kVarTypeInt64`.
//! - `kVarTypeUIntPtr` to `kVarTypeUInt64`.
ASMJIT_VAR const uint8_t _x64VarMapping[kX86VarTypeCount];
#endif // ASMJIT_BUILD_X64

// ============================================================================
// [asmjit::X86Var]
// ============================================================================

//! Base class for all X86 variables.
struct X86Var : public Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86Var() : Var(NoInit) {
    reset();
  }

  ASMJIT_INLINE X86Var(const X86Var& other) : Var(other) {}

  explicit ASMJIT_INLINE X86Var(const _NoInit&) : Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86Var Specific]
  // --------------------------------------------------------------------------

  //! Clone X86Var operand.
  ASMJIT_INLINE X86Var clone() const {
    return X86Var(*this);
  }

  // --------------------------------------------------------------------------
  // [Type]
  // --------------------------------------------------------------------------

  //! Get register type.
  ASMJIT_INLINE uint32_t getRegType() const { return _vreg.type; }
  //! Get variable type.
  ASMJIT_INLINE uint32_t getVarType() const { return _vreg.vType; }

  //! Get whether the variable is Gp register.
  ASMJIT_INLINE bool isGp() const { return _vreg.type <= kX86RegTypeGpq; }
  //! Get whether the variable is Gpb (8-bit) register.
  ASMJIT_INLINE bool isGpb() const { return _vreg.type <= kX86RegTypeGpbHi; }
  //! Get whether the variable is Gpb-lo (8-bit) register.
  ASMJIT_INLINE bool isGpbLo() const { return _vreg.type == kX86RegTypeGpbLo; }
  //! Get whether the variable is Gpb-hi (8-bit) register.
  ASMJIT_INLINE bool isGpbHi() const { return _vreg.type == kX86RegTypeGpbHi; }
  //! Get whether the variable is Gpw (16-bit) register.
  ASMJIT_INLINE bool isGpw() const { return _vreg.type == kX86RegTypeGpw; }
  //! Get whether the variable is Gpd (32-bit) register.
  ASMJIT_INLINE bool isGpd() const { return _vreg.type == kX86RegTypeGpd; }
  //! Get whether the variable is Gpq (64-bit) register.
  ASMJIT_INLINE bool isGpq() const { return _vreg.type == kX86RegTypeGpq; }

  //! Get whether the variable is Mm (64-bit) register.
  ASMJIT_INLINE bool isMm() const { return _vreg.type == kX86RegTypeMm; }
  //! Get whether the variable is K (64-bit) register.
  ASMJIT_INLINE bool isK() const { return _vreg.type == kX86RegTypeK; }

  //! Get whether the variable is Xmm (128-bit) register.
  ASMJIT_INLINE bool isXmm() const { return _vreg.type == kX86RegTypeXmm; }
  //! Get whether the variable is Ymm (256-bit) register.
  ASMJIT_INLINE bool isYmm() const { return _vreg.type == kX86RegTypeYmm; }
  //! Get whether the variable is Zmm (512-bit) register.
  ASMJIT_INLINE bool isZmm() const { return _vreg.type == kX86RegTypeZmm; }

  // --------------------------------------------------------------------------
  // [Memory Cast]
  // --------------------------------------------------------------------------

  //! Cast this variable to a memory operand.
  //!
  //! \note Size of operand depends on native variable type, you can use other
  //! variants if you want specific one.
  ASMJIT_INLINE X86Mem m(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, getSize());
  }

  //! \overload
  ASMJIT_INLINE X86Mem m(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, getSize());
  }

  //! Cast this variable to 8-bit memory operand.
  ASMJIT_INLINE X86Mem m8(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 1);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m8(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 1);
  }

  //! Cast this variable to 16-bit memory operand.
  ASMJIT_INLINE X86Mem m16(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 2);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m16(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 2);
  }

  //! Cast this variable to 32-bit memory operand.
  ASMJIT_INLINE X86Mem m32(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 4);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m32(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 4);
  }

  //! Cast this variable to 64-bit memory operand.
  ASMJIT_INLINE X86Mem m64(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 8);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m64(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 8);
  }

  //! Cast this variable to 80-bit memory operand (long double).
  ASMJIT_INLINE X86Mem m80(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 10);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m80(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 10);
  }

  //! Cast this variable to 128-bit memory operand.
  ASMJIT_INLINE X86Mem m128(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 16);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m128(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 16);
  }

  //! Cast this variable to 256-bit memory operand.
  ASMJIT_INLINE X86Mem m256(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 32);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m256(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 32);
  }

  //! Cast this variable to 256-bit memory operand.
  ASMJIT_INLINE X86Mem m512(int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, disp, 64);
  }

  //! \overload
  ASMJIT_INLINE X86Mem m512(const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) const {
    return X86Mem(Init, kMemTypeStackIndex, *this, index, shift, disp, 64);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86Var& operator=(const X86Var& other) {
    _copy(other);
    return *this;
  }

  ASMJIT_INLINE bool operator==(const X86Var& other) const {
    return _packed[0] == other._packed[0];
  }

  ASMJIT_INLINE bool operator!=(const X86Var& other) const {
    return _packed[0] != other._packed[0];
  }

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

protected:
  ASMJIT_INLINE X86Var(const X86Var& other, uint32_t reg, uint32_t size) : Var(NoInit) {
    _init_packed_op_sz_w0_id(kOperandTypeVar, size, (reg << 8) + other._vreg.index, other._base.id);
    _vreg.vType = other._vreg.vType;
  }
};

// ============================================================================
// [asmjit::X86GpVar]
// ============================================================================

//! Gp variable.
struct X86GpVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `X86GpVar` instance.
  ASMJIT_INLINE X86GpVar() : X86Var() {}

  //! Create a new initialized `X86GpVar` instance.
  ASMJIT_INLINE X86GpVar(Compiler& c, uint32_t type = kVarTypeIntPtr, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE X86GpVar(const X86GpVar& other) : X86Var(other) {}

  //! Create a new uninitialized `X86GpVar` instance (internal).
  explicit ASMJIT_INLINE X86GpVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86GpVar Specific]
  // --------------------------------------------------------------------------

  //! Clone X86GpVar operand.
  ASMJIT_INLINE X86GpVar clone() const {
    return X86GpVar(*this);
  }

  //! Reset X86GpVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [X86GpVar Cast]
  // --------------------------------------------------------------------------

  //! Cast this variable to 8-bit (LO) part of variable
  ASMJIT_INLINE X86GpVar r8() const { return X86GpVar(*this, kX86RegTypeGpbLo, 1); }
  //! Cast this variable to 8-bit (LO) part of variable
  ASMJIT_INLINE X86GpVar r8Lo() const { return X86GpVar(*this, kX86RegTypeGpbLo, 1); }
  //! Cast this variable to 8-bit (HI) part of variable
  ASMJIT_INLINE X86GpVar r8Hi() const { return X86GpVar(*this, kX86RegTypeGpbHi, 1); }

  //! Cast this variable to 16-bit part of variable
  ASMJIT_INLINE X86GpVar r16() const { return X86GpVar(*this, kX86RegTypeGpw, 2); }
  //! Cast this variable to 32-bit part of variable
  ASMJIT_INLINE X86GpVar r32() const { return X86GpVar(*this, kX86RegTypeGpd, 4); }
  //! Cast this variable to 64-bit part of variable
  ASMJIT_INLINE X86GpVar r64() const { return X86GpVar(*this, kX86RegTypeGpq, 8); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86GpVar& operator=(const X86GpVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const X86GpVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const X86GpVar& other) const { return X86Var::operator!=(other); }

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

protected:
  ASMJIT_INLINE X86GpVar(const X86GpVar& other, uint32_t reg, uint32_t size) : X86Var(other, reg, size) {}
};

// ============================================================================
// [asmjit::X86MmVar]
// ============================================================================

//! Mm variable.
struct X86MmVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `X86MmVar` instance.
  ASMJIT_INLINE X86MmVar() : X86Var() {}
  //! Create a new initialized `X86MmVar` instance.
  ASMJIT_INLINE X86MmVar(Compiler& c, uint32_t type = kX86VarTypeMm, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE X86MmVar(const X86MmVar& other) : X86Var(other) {}

  //! Create a new uninitialized `X86MmVar` instance (internal).
  explicit ASMJIT_INLINE X86MmVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86MmVar Specific]
  // --------------------------------------------------------------------------

  //! Clone X86MmVar operand.
  ASMJIT_INLINE X86MmVar clone() const {
    return X86MmVar(*this);
  }

  //! Reset X86MmVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86MmVar& operator=(const X86MmVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const X86MmVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const X86MmVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::X86XmmVar]
// ============================================================================

//! Xmm variable.
struct X86XmmVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `X86XmmVar` instance.
  ASMJIT_INLINE X86XmmVar() : X86Var() {}
  //! Create a new initialized `X86XmmVar` instance.
  ASMJIT_INLINE X86XmmVar(Compiler& c, uint32_t type = kX86VarTypeXmm, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE X86XmmVar(const X86XmmVar& other) : X86Var(other) {}

  //! Create a new uninitialized `X86XmmVar` instance (internal).
  explicit ASMJIT_INLINE X86XmmVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86XmmVar Specific]
  // --------------------------------------------------------------------------

  //! Clone X86XmmVar operand.
  ASMJIT_INLINE X86XmmVar clone() const {
    return X86XmmVar(*this);
  }

  //! Reset X86XmmVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86XmmVar& operator=(const X86XmmVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const X86XmmVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const X86XmmVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::X86YmmVar]
// ============================================================================

//! Ymm variable.
struct X86YmmVar : public X86Var {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new uninitialized `X86YmmVar` instance.
  ASMJIT_INLINE X86YmmVar() : X86Var() {}
  //! Create a new initialized `X86YmmVar` instance.
  ASMJIT_INLINE X86YmmVar(Compiler& c, uint32_t type = kX86VarTypeYmm, const char* name = NULL) : X86Var(NoInit) {
    c._newVar(this, type, name);
  }

  //! Create a clone of `other`.
  ASMJIT_INLINE X86YmmVar(const X86YmmVar& other) : X86Var(other) {}

  //! Create a new uninitialized `X86YmmVar` instance (internal).
  explicit ASMJIT_INLINE X86YmmVar(const _NoInit&) : X86Var(NoInit) {}

  // --------------------------------------------------------------------------
  // [X86YmmVar Specific]
  // --------------------------------------------------------------------------

  //! Clone X86YmmVar operand.
  ASMJIT_INLINE X86YmmVar clone() const {
    return X86YmmVar(*this);
  }

  //! Reset X86YmmVar operand.
  ASMJIT_INLINE void reset() {
    X86Var::reset();
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86YmmVar& operator=(const X86YmmVar& other) { _copy(other); return *this; }

  ASMJIT_INLINE bool operator==(const X86YmmVar& other) const { return X86Var::operator==(other); }
  ASMJIT_INLINE bool operator!=(const X86YmmVar& other) const { return X86Var::operator!=(other); }
};

// ============================================================================
// [asmjit::X86VarMap]
// ============================================================================

struct X86VarMap : public VarMap {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get variable-attributes list as VarAttr data.
  ASMJIT_INLINE VarAttr* getVaList() const {
    return const_cast<VarAttr*>(_list);
  }

  //! Get variable-attributes list as VarAttr data (by class).
  ASMJIT_INLINE VarAttr* getVaListByClass(uint32_t c) const {
    return const_cast<VarAttr*>(_list) + _start.get(c);
  }

  //! Get position of variables (by class).
  ASMJIT_INLINE uint32_t getVaStart(uint32_t c) const {
    return _start.get(c);
  }

  //! Get count of variables (by class).
  ASMJIT_INLINE uint32_t getVaCountByClass(uint32_t c) const {
    return _count.get(c);
  }

  //! Get VarAttr at `index`.
  ASMJIT_INLINE VarAttr* getVa(uint32_t index) const {
    ASMJIT_ASSERT(index < _vaCount);
    return getVaList() + index;
  }

  //! Get VarAttr of `c` class at `index`.
  ASMJIT_INLINE VarAttr* getVaByClass(uint32_t c, uint32_t index) const {
    ASMJIT_ASSERT(index < _count._regs[c]);
    return getVaListByClass(c) + index;
  }

  // --------------------------------------------------------------------------
  // [Utils]
  // --------------------------------------------------------------------------

  //! Find VarAttr.
  ASMJIT_INLINE VarAttr* findVa(VarData* vd) const {
    VarAttr* list = getVaList();
    uint32_t count = getVaCount();

    for (uint32_t i = 0; i < count; i++)
      if (list[i].getVd() == vd)
        return &list[i];

    return NULL;
  }

  //! Find VarAttr (by class).
  ASMJIT_INLINE VarAttr* findVaByClass(uint32_t c, VarData* vd) const {
    VarAttr* list = getVaListByClass(c);
    uint32_t count = getVaCountByClass(c);

    for (uint32_t i = 0; i < count; i++)
      if (list[i].getVd() == vd)
        return &list[i];

    return NULL;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Special registers on input.
  //!
  //! Special register(s) restricted to one or more physical register. If there
  //! is more than one special register it means that we have to duplicate the
  //! variable content to all of them (it means that the same varible was used
  //! by two or more operands). We forget about duplicates after the register
  //! allocation finishes and marks all duplicates as non-assigned.
  X86RegMask _inRegs;

  //! Special registers on output.
  //!
  //! Special register(s) used on output. Each variable can have only one
  //! special register on the output, 'X86VarMap' contains all registers from
  //! all 'VarAttr's.
  X86RegMask _outRegs;

  //! Clobbered registers (by a function call).
  X86RegMask _clobberedRegs;

  //! Start indexes of variables per register class.
  X86RegCount _start;
  //! Count of variables per register class.
  X86RegCount _count;

  //! VarAttr list.
  VarAttr _list[1];
};

// ============================================================================
// [asmjit::X86StateCell]
// ============================================================================

//! X86/X64 state-cell.
union X86StateCell {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getState() const {
    return _state;
  }

  ASMJIT_INLINE void setState(uint32_t state) {
    _state = static_cast<uint8_t>(state);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() { _packed = 0; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _packed;

  struct {
    uint8_t _state : 2;
    uint8_t _unused : 6;
  };
};

// ============================================================================
// [asmjit::X86VarState]
// ============================================================================

//! X86/X64 state.
struct X86VarState : VarState {
  enum {
    //! Base index of Gp registers.
    kGpIndex = 0,
    //! Count of Gp registers.
    kGpCount = 16,

    //! Base index of Mm registers.
    kMmIndex = kGpIndex + kGpCount,
    //! Count of Mm registers.
    kMmCount = 8,

    //! Base index of Xmm registers.
    kXmmIndex = kMmIndex + kMmCount,
    //! Count of Xmm registers.
    kXmmCount = 16,

    //! Count of all registers in `X86VarState`.
    kAllCount = kXmmIndex + kXmmCount
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE VarData** getList() {
    return _list;
  }

  ASMJIT_INLINE VarData** getListByClass(uint32_t c) {
    switch (c) {
      case kX86RegClassGp : return _listGp;
      case kX86RegClassMm : return _listMm;
      case kX86RegClassXyz: return _listXmm;

      default:
        return NULL;
    }
  }

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset(size_t numCells) {
    ::memset(this, 0, kAllCount * sizeof(VarData*) +
                      2         * sizeof(X86RegMask) +
                      numCells  * sizeof(X86StateCell));
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    //! List of all allocated variables in one array.
    VarData* _list[kAllCount];

    struct {
      //! Allocated Gp registers.
      VarData* _listGp[kGpCount];
      //! Allocated Mm registers.
      VarData* _listMm[kMmCount];
      //! Allocated Xmm registers.
      VarData* _listXmm[kXmmCount];
    };
  };

  //! Occupied registers (mask).
  X86RegMask _occupied;
  //! Modified registers (mask).
  X86RegMask _modified;

  //! Variables data, the length is stored in `X86Context`.
  X86StateCell _cells[1];
};

// ============================================================================
// [asmjit::X86FuncDecl]
// ============================================================================

//! X86 function, including calling convention, arguments and their
//! register indices or stack positions.
struct X86FuncDecl : public FuncDecl {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `X86FuncDecl` instance.
  ASMJIT_INLINE X86FuncDecl() {
    reset();
  }

  // --------------------------------------------------------------------------
  // [Accessors - X86]
  // --------------------------------------------------------------------------

  //! Get used registers (mask).
  //!
  //! \note The result depends on the function calling convention AND the
  //! function prototype. Returned mask contains only registers actually used
  //! to pass function arguments.
  ASMJIT_INLINE uint32_t getUsed(uint32_t c) const {
    return _used.get(c);
  }

  //! Get passed registers (mask).
  //!
  //! \note The result depends on the function calling convention used; the
  //! prototype of the function doesn't affect the mask returned.
  ASMJIT_INLINE uint32_t getPassed(uint32_t c) const {
    return _passed.get(c);
  }

  //! Get preserved registers (mask).
  //!
  //! \note The result depends on the function calling convention used; the
  //! prototype of the function doesn't affect the mask returned.
  ASMJIT_INLINE uint32_t getPreserved(uint32_t c) const {
    return _preserved.get(c);
  }

  //! Get ther order of passed registers (Gp).
  //!
  //! \note The result depends on the function calling convention used; the
  //! prototype of the function doesn't affect the mask returned.
  ASMJIT_INLINE const uint8_t* getPassedOrderGp() const {
    return _passedOrderGp;
  }

  //! Get ther order of passed registers (Xmm).
  //!
  //! \note The result depends on the function calling convention used; the
  //! prototype of the function doesn't affect the mask returned.
  ASMJIT_INLINE const uint8_t* getPassedOrderXmm() const {
    return _passedOrderXmm;
  }

  // --------------------------------------------------------------------------
  // [SetPrototype]
  // --------------------------------------------------------------------------

  //! Set function prototype.
  //!
  //! This will set function calling convention and setup arguments variables.
  //!
  //! \note This function will allocate variables, it can be called only once.
  ASMJIT_API Error setPrototype(uint32_t conv, const FuncPrototype& p);

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_API void reset();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Used registers.
  X86RegMask _used;

  //! Passed registers (defined by the calling convention).
  X86RegMask _passed;
  //! Preserved registers (defined by the calling convention).
  X86RegMask _preserved;

  //! Order of registers defined to pass function arguments (Gp).
  uint8_t _passedOrderGp[8];
  //! Order of registers defined to pass function arguments (Xmm).
  uint8_t _passedOrderXmm[8];
};

// ============================================================================
// [asmjit::X86FuncNode]
// ============================================================================

//! X86/X64 function node.
struct X86FuncNode : public FuncNode {
  ASMJIT_NO_COPY(X86FuncNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `X86FuncNode` instance.
  ASMJIT_INLINE X86FuncNode(Compiler* compiler) : FuncNode(compiler) {
    _decl = &_x86Decl;
    _saveRestoreRegs.reset();

    _alignStackSize = 0;
    _alignedMemStackSize = 0;
    _pushPopStackSize = 0;
    _moveStackSize = 0;
    _extraStackSize = 0;

    _stackFrameRegIndex = kInvalidReg;
    _isStackFrameRegPreserved = false;
    _stackFrameCopyGpIndex[0] = kInvalidReg;
    _stackFrameCopyGpIndex[1] = kInvalidReg;
    _stackFrameCopyGpIndex[2] = kInvalidReg;
    _stackFrameCopyGpIndex[3] = kInvalidReg;
    _stackFrameCopyGpIndex[4] = kInvalidReg;
    _stackFrameCopyGpIndex[5] = kInvalidReg;
  }

  //! Destroy the `X86FuncNode` instance.
  ASMJIT_INLINE ~X86FuncNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function declaration as `X86FuncDecl`.
  ASMJIT_INLINE X86FuncDecl* getDecl() const {
    return const_cast<X86FuncDecl*>(&_x86Decl);
  }

  //! Get argument.
  ASMJIT_INLINE VarData* getArg(uint32_t i) const {
    ASMJIT_ASSERT(i < _x86Decl.getArgCount());
    return static_cast<VarData*>(_argList[i]);
  }

  //! Get registers which have to be saved in prolog/epilog.
  ASMJIT_INLINE uint32_t getSaveRestoreRegs(uint32_t c) { return _saveRestoreRegs.get(c); }

  //! Get stack size needed to align stack back to the nature alignment.
  ASMJIT_INLINE uint32_t getAlignStackSize() const { return _alignStackSize; }
  //! Set stack size needed to align stack back to the nature alignment.
  ASMJIT_INLINE void setAlignStackSize(uint32_t s) { _alignStackSize = s; }

  //! Get aligned stack size used by variables and memory allocated on the stack.
  ASMJIT_INLINE uint32_t getAlignedMemStackSize() const { return _alignedMemStackSize; }

  //! Get stack size used by push/pop sequences in prolog/epilog.
  ASMJIT_INLINE uint32_t getPushPopStackSize() const { return _pushPopStackSize; }
  //! Set stack size used by push/pop sequences in prolog/epilog.
  ASMJIT_INLINE void setPushPopStackSize(uint32_t s) { _pushPopStackSize = s; }

  //! Get stack size used by mov sequences in prolog/epilog.
  ASMJIT_INLINE uint32_t getMoveStackSize() const { return _moveStackSize; }
  //! Set stack size used by mov sequences in prolog/epilog.
  ASMJIT_INLINE void setMoveStackSize(uint32_t s) { _moveStackSize = s; }

  //! Get extra stack size.
  ASMJIT_INLINE uint32_t getExtraStackSize() const { return _extraStackSize; }
  //! Set extra stack size.
  ASMJIT_INLINE void setExtraStackSize(uint32_t s) { _extraStackSize  = s; }

  //! Get whether the function has stack frame register.
  //!
  //! \note Stack frame register can be used for both - aligning purposes or
  //! generating standard prolog/epilog sequence.
  //!
  //! \note Used only when stack is misaligned.
  ASMJIT_INLINE bool hasStackFrameReg() const { return _stackFrameRegIndex != kInvalidReg; }
  //! Get stack frame register index.
  //!
  //! \note Used only when stack is misaligned.
  ASMJIT_INLINE uint32_t getStackFrameRegIndex() const { return _stackFrameRegIndex; }
  //! Get whether the stack frame register is preserved.
  //!
  //! \note Used only when stack is misaligned.
  ASMJIT_INLINE bool isStackFrameRegPreserved() const { return static_cast<bool>(_isStackFrameRegPreserved); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! X86 function decl.
  X86FuncDecl _x86Decl;
  //! Registers which must be saved/restored in prolog/epilog.
  X86RegMask _saveRestoreRegs;

  //! Stack size needed to align function back to the nature alignment.
  uint32_t _alignStackSize;
  //! Like `_memStackSize`, but aligned.
  uint32_t _alignedMemStackSize;

  //! Stack required for push/pop in prolog/epilog (X86/X64 specific).
  uint32_t _pushPopStackSize;
  //! Stack required for movs in prolog/epilog (X86/X64 specific).
  uint32_t _moveStackSize;

  //! Stack required to put extra data (for example function arguments
  //! when manually aligning to requested alignment).
  uint32_t _extraStackSize;

  //! Stack frame register.
  uint8_t _stackFrameRegIndex;
  //! Whether the stack frame register is preserved.
  uint8_t _isStackFrameRegPreserved;
  //! Gp registers indexes that can be used to copy function arguments
  //! to a new location in case we are doing manual stack alignment.
  uint8_t _stackFrameCopyGpIndex[6];
};

// ============================================================================
// [asmjit::X86CallNode]
// ============================================================================

//! X86/X64 function-call node.
struct X86CallNode : public CallNode {
  ASMJIT_NO_COPY(X86CallNode)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `X86CallNode` instance.
  ASMJIT_INLINE X86CallNode(Compiler* compiler, const Operand& target) : CallNode(compiler, target) {
    _decl = &_x86Decl;
    _usedArgs.reset();
  }

  //! Destroy the `X86CallNode` instance.
  ASMJIT_INLINE ~X86CallNode() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function prototype.
  ASMJIT_INLINE X86FuncDecl* getDecl() const {
    return const_cast<X86FuncDecl*>(&_x86Decl);
  }

  // --------------------------------------------------------------------------
  // [Prototype]
  // --------------------------------------------------------------------------

  //! Set function prototype.
  ASMJIT_API Error setPrototype(uint32_t conv, const FuncPrototype& p);

  // --------------------------------------------------------------------------
  // [Arg / Ret]
  // --------------------------------------------------------------------------

  //! Set argument at `i` to `op`.
  ASMJIT_API bool _setArg(uint32_t i, const Operand& op);
  //! Set return at `i` to `op`.
  ASMJIT_API bool _setRet(uint32_t i, const Operand& op);

  //! Set argument at `i` to `var`.
  ASMJIT_INLINE bool setArg(uint32_t i, const Var& var) { return _setArg(i, var); }
  //! Set argument at `i` to `reg` (FP registers only).
  ASMJIT_INLINE bool setArg(uint32_t i, const X86FpReg& reg) { return _setArg(i, reg); }
  //! Set argument at `i` to `imm`.
  ASMJIT_INLINE bool setArg(uint32_t i, const Imm& imm) { return _setArg(i, imm); }

  //! Set return at `i` to `var`.
  ASMJIT_INLINE bool setRet(uint32_t i, const Var& var) { return _setRet(i, var); }
  //! Set return at `i` to `reg` (FP registers only).
  ASMJIT_INLINE bool setRet(uint32_t i, const X86FpReg& reg) { return _setRet(i, reg); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! X86 declaration.
  X86FuncDecl _x86Decl;
  //! Mask of all registers actually used to pass function arguments.
  //!
  //! \note This bit-mask is not the same as `X86Func::_passed`. It contains
  //! only registers actually used to do the call while `X86Func::_passed`
  //! mask contains all registers for all function prototype combinations.
  X86RegMask _usedArgs;
};

// ============================================================================
// [asmjit::X86TypeId / VarMapping]
// ============================================================================

#if !defined(ASMJIT_DOCGEN)
ASMJIT_TYPE_ID(X86MmReg, kX86VarTypeMm);
ASMJIT_TYPE_ID(X86MmVar, kX86VarTypeMm);
ASMJIT_TYPE_ID(X86XmmReg, kX86VarTypeXmm);
ASMJIT_TYPE_ID(X86XmmVar, kX86VarTypeXmm);
ASMJIT_TYPE_ID(X86YmmReg, kX86VarTypeYmm);
ASMJIT_TYPE_ID(X86YmmVar, kX86VarTypeYmm);
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::X86Compiler]
// ============================================================================

//! X86/X64 compiler.
//!
//! This class is used to store instruction stream and allows to modify it on
//! the fly. It uses different concept than `Assembler` class and in fact
//! `Assembler` is only used as a backend. Compiler never emits machine code
//! and each instruction you use is stored to instruction array instead. This
//! allows to modify instruction stream later and for example to reorder
//! instructions to make better performance.
//!
//! `X86Compiler` moves code generation to a higher level. Higher level
//! constructs allows to write more abstract and extensible code that is not
//! possible with pure `X86Assembler`.
//!
//! The Story
//! ---------
//!
//! Before telling you how Compiler works I'd like to write a story. I'd like
//! to cover reasons why this class was created and why I'm recommending to use
//! it. When I released the first version of AsmJit (0.1) it was a toy. The
//! first function I wrote was function which is still available as testjit and
//! which simply returns 1024. The reason why function works for both 32-bit/
//! 64-bit mode and for Windows/Unix specific calling conventions is luck, no
//! arguments usage and no registers usage except returning value in EAX/RAX.
//!
//! Then I started a project called BlitJit which was targetted to generating
//! JIT code for computer graphics. After writing some lines I decided that I
//! can't join pieces of code together without abstraction, should be
//! pixels source pointer in ESI/RSI or EDI/RDI or it's completelly
//! irrellevant? What about destination pointer and SSE2 register for reading
//! input pixels? The simple answer might be "just pick some one and use it".
//!
//! Another reason for abstraction is function calling-conventions. It's really
//! not easy to write assembler code for 32-bit and 64-bit platform supporting
//! three calling conventions (32-bit is similar between Windows and Unix, but
//! 64-bit calling conventions are different).
//!
//! At this time I realized that I can't write code which uses named registers,
//! I need to abstract it. In most cases you don't need specific register, you
//! need to emit instruction that does something with 'virtual' register(s),
//! memory, immediate or label.
//!
//! The first version of AsmJit with Compiler was 0.5 (or 0.6?, can't remember).
//! There was support for 32-bit and 64-bit mode, function calling conventions,
//! but when emitting instructions the developer needed to decide which
//! registers are changed, which are only read or completely overwritten. This
//! model helped a lot when generating code, especially when joining more
//! code-sections together, but there was also small possibility for mistakes.
//! Simply the first version of Compiler was great improvement over low-level
//! Assembler class, but the API design wasn't perfect.
//!
//! The second version of Compiler, completelly rewritten and based on
//! different goals, is part of AsmJit starting at version 1.0. This version
//! was designed after the first one and it contains serious improvements over
//! the old one. The first improvement is that you just use instructions with
//! virtual registers - called variables. When using compiler there is no way
//! to use native registers, there are variables instead. AsmJit is smarter
//! than before and it knows which register is needed only for read (r),
//! read/write (w) or overwrite (x). Supported are also instructions which
//! are using some registers in implicit way (these registers are not part of
//! instruction definition in string form). For example to use CPUID instruction
//! you must give it four variables which will be automatically allocated in
//! input/output registers (EAX, EBX, ECX, EDX).
//!
//! Another improvement is algorithm used by a register allocator. In first
//! version the registers were allocated when creating instruction stream. In
//! new version registers are allocated after calling `Compiler::make()`,
//! thus register allocator has information about scope of all variables and
//! statistics of their usage. The algorithm to allocate registers is very
//! simple and it's always called as a 'linear scan register allocator'. When
//! you get out of registers the all possible variables are scored and the worst
//! is spilled. Of course algorithm ignores the variables used for current
//! instruction.
//!
//! In addition, because registers are allocated after the code stream is
//! generated, the state switches between jumps are handled by Compiler too.
//! You don't need to worry about jumps, compiler always do this dirty work
//! for you.
//!
//! The nearly last thing I'd like to present is calling other functions from
//! the generated code. AsmJit uses a `FuncPrototype` class to hold function
//! parameters, their position in stack (or register index) and return value.
//! This class is used internally, but it can be used to create your own
//! function calling-convention. All standard function calling conventions are
//! implemented.
//!
//! Please enjoy the new version of Compiler, it was created for writing a
//! low-level code using high-level API, leaving developer to concentrate on
//! real problems and not to solving a register puzzle.
//!
//! Code Generation
//! ---------------
//!
//! First that is needed to know about compiler is that compiler never emits
//! machine code. It's used as a middleware between @c asmjit::Assembler and
//! your code. There is also convenience method @c make() that allows to
//! generate machine code directly without creating @c asmjit::Assembler
//! instance.
//!
//! Comparison of generating machine code through @c Assembler and directly
//! by @c Compiler:
//!
//! ~~~
//! // Assembler instance is low level code generation class that emits
//! // machine code.
//! Assembler a;
//!
//! // Compiler instance is high level code generation class that stores all
//! // instructions in internal representation.
//! Compiler c;
//!
//! // ... put your code here ...
//!
//! // Final step - generate code. asmjit::Compiler::serialize() will send all
//! // instructions into Assembler and this ensures generating real machine code.
//! c.serialize(&a);
//!
//! // Your function
//! void* fn = a.make();
//! ~~~
//!
//! Example how to generate machine code using only @c Compiler (preferred):
//!
//! ~~~
//! // Compiler instance is enough.
//! Compiler c;
//!
//! // ... put your code here ...
//!
//! // Your function
//! void* fn = c.make();
//! ~~~
//!
//! You can see that there is @c asmjit::Compiler::serialize() function that
//! emits instructions into @c asmjit::Assembler(). This layered architecture
//! means that each class is used for something different and there is no code
//! duplication. For convenience there is also @c asmjit::Compiler::make()
//! method that can create your function using @c asmjit::Assembler, but
//! internally (this is preferred bahavior when using @c asmjit::Compiler).
//!
//! The @c make() method allocates memory using `Runtime` instance passed
//! into the @c Compiler constructor. If code generator is used to create JIT
//! function then virtual memory allocated by `VMemMgr` is used.
//!
//! ~~~
//! JitRuntime runtime;
//! Compiler c(&runtime);
//!
//! // ... put your code using Compiler instance ...
//!
//! // Your function
//! void* fn = c.make();
//!
//! runtime.release(fn);
//! ~~~
//!
//! Functions
//! ---------
//!
//! To build functions with @c Compiler, see @c asmjit::Compiler::addFunc()
//! method.
//!
//! Variables
//! ---------
//!
//! Compiler is able to manage variables and function arguments. Function
//! arguments are moved to variables by using @c setArg() method, where the
//! first parameter is argument index and second parameter is the variable
//! instance. To declare variable use @c newGpVar(), @c newMmVar() and @c
//! newXmmVar() methods. The @c newXXX() methods accept also parameter
//! describing the variable type. For example the @c newGpVar() method always
//! creates variable which size matches the target architecture size (for
//! 32-bit target the 32-bit variable is created, for 64-bit target the
//! variable size is 64-bit). To override this behavior the variable type
//! must be specified.
//!
//! ~~~
//! // Compiler and function declaration - void f(int*);
//! Compiler c;
//! X86GpVar a0(c, kVarTypeIntPtr);
//!
//! c.addFunc(kFuncConvHost, FuncBuilder1<Void, int*>());
//! c.setArg(0, a0);
//!
//! // Create your variables.
//! X86GpVar x0(c, kVarTypeInt32);
//! X86GpVar x1(c, kVarTypeInt32);
//!
//! // Init your variables.
//! c.mov(x0, 1);
//! c.mov(x1, 2);
//!
//! // ... your code ...
//! c.add(x0, x1);
//! // ... your code ...
//!
//! // Store result to a given pointer in first argument
//! c.mov(dword_ptr(a0), x0);
//!
//! // End of function body.
//! c.endFunc();
//!
//! // Make the function.
//! typedef void (*MyFunc)(int*);
//! MyFunc func = asmjit_cast<MyFunc>(c.make());
//! ~~~
//!
//! This code snipped needs to be explained. You can see that there are more
//! variable types that can be used by `Compiler`. Most useful variables can
//! be allocated using general purpose registers (`X86GpVar`), MMX registers
//! (`X86MmVar`) or SSE/SSE2 registers (`X86XmmVar`).
//!
//! X86/X64 variable types:
//!
//! - `kVarTypeInt8`     - Signed 8-bit integer, mapped to Gpd register (eax, ebx, ...).
//! - `kVarTypeUInt8`    - Unsigned 8-bit integer, mapped to Gpd register (eax, ebx, ...).
//!
//! - `kVarTypeInt16`    - Signed 16-bit integer, mapped to Gpd register (eax, ebx, ...).
//! - `kVarTypeUInt16`   - Unsigned 16-bit integer, mapped to Gpd register (eax, ebx, ...).
//!
//! - `kVarTypeInt32`    - Signed 32-bit integer, mapped to Gpd register (eax, ebx, ...).
//! - `kVarTypeUInt32`   - Unsigned 32-bit integer, mapped to Gpd register (eax, ebx, ...).
//!
//! - `kVarTypeInt64`    - Signed 64-bit integer, mapped to Gpq register (rax, rbx, ...).
//! - `kVarTypeUInt64`   - Unsigned 64-bit integer, mapped to Gpq register (rax, rbx, ...).
//!
//! - `kVarTypeIntPtr`   - intptr_t, mapped to Gpd/Gpq register; depends on target, not host!
//! - `kVarTypeUIntPtr`  - uintptr_t, mapped to Gpd/Gpq register; depends on target, not host!
//!
//! - `kVarTypeFp32`     - 32-bit floating point register (fp0, fp1, ...).
//! - `kVarTypeFp64`     - 64-bit floating point register (fp0, fp1, ...).
//!
//! - `kX86VarTypeMm`       - 64-bit Mm register (mm0, mm1, ...).
//!
//! - `kX86VarTypeXmm`      - 128-bit SSE register.
//! - `kX86VarTypeXmmSs`    - 128-bit SSE register that contains a scalar 32-bit SP-FP value.
//! - `kX86VarTypeXmmSd`    - 128-bit SSE register that contains a scalar 64-bit DP-FP value.
//! - `kX86VarTypeXmmPs`    - 128-bit SSE register that contains 4 packed 32-bit SP-FP values.
//! - `kX86VarTypeXmmPd`    - 128-bit SSE register that contains 2 packed 64-bit DP-FP values.
//!
//! - `kX86VarTypeYmm`      - 256-bit AVX register.
//! - `kX86VarTypeYmmPs`    - 256-bit AVX register that contains 4 packed 32-bit SP-FP values.
//! - `kX86VarTypeYmmPd`    - 256-bit AVX register that contains 2 packed 64-bit DP-FP values.
//!
//! Variable states:
//!
//! - `kVarStateUnused - State that is assigned to newly created variables or
//!    to not used variables (dereferenced to zero).
//! - `kVarStateReg - State that means that variable is currently allocated in
//!    register.
//! - `kVarStateMem - State that means that variable is currently only in
//!    memory location.
//!
//! When you create new variable, initial state is always `kVarStateUnused`,
//! allocating it to register or spilling to memory changes this state to
//! `kVarStateReg` or `kVarStateMem`, respectively. During variable lifetime
//! it's usual that its state is changed multiple times. To generate better
//! code, you can control allocating and spilling by using up to four types
//! of methods that allows it (see next list).
//!
//! Explicit variable allocating / spilling methods:
//!
//! - `Compiler::alloc()` - Explicit method to alloc variable into register.
//!    It can be used to force allocation a variable before a loop for example.
//!
//! - `Compiler::spill()` - Explicit method to spill variable. If variable
//!    is in register and you call this method, it's moved to its home memory
//!    location. If variable is not in register no operation is performed.
//!
//! - `Compiler::unuse()` - Unuse variable (you can use this to end the
//!    variable scope or sub-scope).
//!
//! Please see AsmJit tutorials (testcompiler.cpp and testvariables.cpp) for
//! more complete examples.
//!
//! Memory Management
//! -----------------
//!
//! Compiler Memory management follows these rules:
//!
//! - Everything created by `Compiler` is always freed by `Compiler`.
//! - To get decent performance, compiler always uses larger memory buffer
//!   for objects to allocate and when compiler instance is destroyed, this
//!   buffer is freed. Destructors of active objects are called when
//!   destroying compiler instance. Destructors of abadonded compiler
//!   objects are called immediately after abadonding them.
//! - This type of memory management is called 'zone memory management'.
//!
//! This means that you can't use any `Compiler` object after destructing it,
//! it also means that each object like `Label`, `Var` and others are created
//! and managed by @c Compiler itself. These objects contain ID which is
//! used internally by Compiler to store additional information about these
//! objects.
//!
//! Control-Flow and State Management
//! ---------------------------------
//!
//! The `Compiler` automatically manages state of the variables when using
//! control flow instructions like jumps, conditional jumps and calls. There
//! is minimal heuristics for choosing the method how state is saved or restored.
//!
//! Generally the state can be changed only when using jump or conditional jump
//! instruction. When using non-conditional jump then state change is embedded
//! into the instruction stream before the jump. When using conditional jump
//! the `Compiler` decides whether to restore state before the jump or whether
//! to use another block where state is restored. The last case is that no-code
//! have to be emitted and there is no state change (this is of course ideal).
//!
//! Choosing whether to embed 'restore-state' section before conditional jump
//! is quite simple. If jump is likely to be 'taken' then code is embedded, if
//! jump is unlikely to be taken then the small code section for state-switch
//! will be generated instead.
//!
//! Next example is the situation where the extended code block is used to
//! do state-change:
//!
//! ~~~
//! Compiler c;
//!
//! c.addFunc(kFuncConvHost, FuncBuilder0<Void>());
//!
//! // Labels.
//! Label L0(c);
//!
//! // Variables.
//! X86GpVar var0(c, kVarTypeInt32);
//! X86GpVar var1(c, kVarTypeInt32);
//!
//! // Cleanup. After these two lines, the var0 and var1 will be always stored
//! // in registers. Our example is very small, but in larger code the var0 can
//! // be spilled by xor(var1, var1).
//! c.xor_(var0, var0);
//! c.xor_(var1, var1);
//! c.cmp(var0, var1);
//! // State:
//! //   var0 - register.
//! //   var1 - register.
//!
//! // We manually spill these variables.
//! c.spill(var0);
//! c.spill(var1);
//! // State:
//! //   var0 - memory.
//! //   var1 - memory.
//!
//! // Conditional jump to L0. It will be always taken, but compiler thinks that
//! // it is unlikely taken so it will embed state change code somewhere.
//! c.je(L0);
//!
//! // Do something. The variables var0 and var1 will be allocated again.
//! c.add(var0, 1);
//! c.add(var1, 2);
//! // State:
//! //   var0 - register.
//! //   var1 - register.
//!
//! // Bind label here, the state is not changed.
//! c.bind(L0);
//! // State:
//! //   var0 - register.
//! //   var1 - register.
//!
//! // We need to use var0 and var1, because if compiler detects that variables
//! // are out of scope then it optimizes the state-change.
//! c.sub(var0, var1);
//! // State:
//! //   var0 - register.
//! //   var1 - register.
//!
//! c.endFunc();
//! ~~~
//!
//! The output:
//!
//! ~~~
//! xor eax, eax                    ; xor var_0, var_0
//! xor ecx, ecx                    ; xor var_1, var_1
//! cmp eax, ecx                    ; cmp var_0, var_1
//! mov [esp - 24], eax             ; spill var_0
//! mov [esp - 28], ecx             ; spill var_1
//! je L0_Switch
//! mov eax, [esp - 24]             ; alloc var_0
//! add eax, 1                      ; add var_0, 1
//! mov ecx, [esp - 28]             ; alloc var_1
//! add ecx, 2                      ; add var_1, 2
//! L0:
//! sub eax, ecx                    ; sub var_0, var_1
//! ret
//!
//! ; state-switch begin
//! L0_Switch0:
//! mov eax, [esp - 24]             ; alloc var_0
//! mov ecx, [esp - 28]             ; alloc var_1
//! jmp short L0
//! ; state-switch end
//! ~~~
//!
//! You can see that the state-switch section was generated (see L0_Switch0).
//! The compiler is unable to restore state immediately when emitting the
//! forward jump (the code is generated from first to last instruction and
//! the target state is simply not known at this time).
//!
//! To tell `Compiler` that you want to embed state-switch code before jump
//! it's needed to create backward jump (where also processor expects that it
//! will be taken). To demonstrate the possibility to embed state-switch before
//! jump we use slightly modified code:
//!
//! ~~~
//! Compiler c;
//!
//! c.addFunc(kFuncConvHost, FuncBuilder0<Void>());
//!
//! // Labels.
//! Label L0(c);
//!
//! // Variables.
//! X86GpVar var0(c, kVarTypeInt32);
//! X86GpVar var1(c, kVarTypeInt32);
//!
//! // Cleanup. After these two lines, the var0 and var1 will be always stored
//! // in registers. Our example is very small, but in larger code the var0 can
//! // be spilled by xor(var1, var1).
//! c.xor_(var0, var0);
//! c.xor_(var1, var1);
//! // State:
//! //   var0 - register.
//! //   var1 - register.
//!
//! // We manually spill these variables.
//! c.spill(var0);
//! c.spill(var1);
//! // State:
//! //   var0 - memory.
//! //   var1 - memory.
//!
//! // Bind our label here.
//! c.bind(L0);
//!
//! // Do something, the variables will be allocated again.
//! c.add(var0, 1);
//! c.add(var1, 2);
//! // State:
//! //   var0 - register.
//! //   var1 - register.
//!
//! // Backward conditional jump to L0. The default behavior is that it is taken
//! // so state-change code will be embedded here.
//! c.je(L0);
//!
//! c.endFunc();
//! ~~~
//!
//! The output:
//!
//! ~~~
//! xor ecx, ecx                    ; xor var_0, var_0
//! xor edx, edx                    ; xor var_1, var_1
//! mov [esp - 24], ecx             ; spill var_0
//! mov [esp - 28], edx             ; spill var_1
//! L2:
//! mov ecx, [esp - 24]             ; alloc var_0
//! add ecx, 1                      ; add var_0, 1
//! mov edx, [esp - 28]             ; alloc var_1
//! add edx, 2                      ; add var_1, 2
//!
//! ; state-switch begin
//! mov [esp - 24], ecx             ; spill var_0
//! mov [esp - 28], edx             ; spill var_1
//! ; state-switch end
//!
//! je short L2
//! ret
//! ~~~
//!
//! Please notice where the state-switch section is located. The `Compiler`
//! decided that jump is likely to be taken so the state change is embedded
//! before the conditional jump. To change this behavior into the previous
//! case it's needed to add an option (kInstOptionTaken/kInstOptionNotTaken).
//!
//! Replacing the <code>c.je(L0)</code> by <code>c.taken(); c.je(L0)</code>
//! will generate code like this:
//!
//! ~~~
//! xor ecx, ecx                    ; xor var_0, var_0
//! xor edx, edx                    ; xor var_1, var_1
//! mov [esp - 24], ecx             ; spill var_0
//! mov [esp - 28], edx             ; spill var_1
//! L0:
//! mov ecx, [esp - 24]             ; alloc var_0
//! add ecx, 1                      ; add var_0, a
//! mov edx, [esp - 28]             ; alloc var_1
//! add edx, 2                      ; add var_1, 2
//! je L0_Switch, 2
//! ret
//!
//! ; state-switch begin
//! L0_Switch:
//! mov [esp - 24], ecx             ; spill var_0
//! mov [esp - 28], edx             ; spill var_1
//! jmp short L0
//! ; state-switch end
//! ~~~
//!
//! This section provided information about how state-change works. The
//! behavior is deterministic and it can be overridden.
//!
//! Advanced Code Generation
//! ------------------------
//!
//! This section describes advanced method of code generation available to
//! `Compiler` (but also to `Assembler`). When emitting code to instruction
//! stream the methods like `mov()`, `add()`, `sub()` can be called directly
//! (advantage is static-type control performed also by C++ compiler) or
//! indirectly using `emit()` method. The `emit()` method needs only instruction
//! code and operands.
//!
//! Example of code generating by standard type-safe API:
//!
//! ~~~
//! Compiler c;
//!
//! X86GpVar var0(c, kVarTypeInt32);
//! X86GpVar var1(c, kVarTypeInt32);
//!
//! ...
//!
//! c.mov(var0, 0);
//! c.add(var0, var1);
//! c.sub(var0, var1);
//! ~~~
//!
//! The code above can be rewritten as:
//!
//! ~~~
//! Compiler c;
//!
//! X86GpVar var0(c, kVarTypeInt32);
//! X86GpVar var1(c, kVarTypeInt32);
//!
//! ...
//!
//! c.emit(kX86InstIdMov, var0, 0);
//! c.emit(kX86InstIdAdd, var0, var1);
//! c.emit(kX86InstIdSub, var0, var1);
//! ~~~
//!
//! The advantage of first snippet is very friendly API and type-safe control
//! that is controlled by the C++ compiler. The advantage of second snippet is
//! availability to replace or generate instruction code in different places.
//! See the next example how the `emit()` method can be used to generate abstract
//! code.
//!
//! Use case:
//!
//! ~~~
//! bool emitArithmetic(Compiler& c, X86XmmVar& var0, X86XmmVar& var1, const char* op) {
//!   uint32_t code = kInstIdNone;
//!
//!   if (strcmp(op, "ADD") == 0)
//!     code = kX86InstIdAddss;
//!   else if (::strcmp(op, "SUBTRACT") == 0)
//!     code = kX86InstIdSubss;
//!   else if (::strcmp(op, "MULTIPLY") == 0)
//!     code = kX86InstIdMulss;
//!   else if (::strcmp(op, "DIVIDE") == 0)
//!     code = kX86InstIdDivss;
//!   else
//!     // Invalid parameter?
//!     return false;
//!
//!   c.emit(code, var0, var1);
//! }
//! ~~~
//!
//! Other use cases are waiting for you! Be sure that instruction you are
//! emitting is correct and encodable, because if not, Assembler will set
//! status code to `kErrorUnknownInst`.
struct ASMJIT_VCLASS X86Compiler : public Compiler {
  ASMJIT_NO_COPY(X86Compiler)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `X86Compiler` instance.
  ASMJIT_API X86Compiler(Runtime* runtime, uint32_t arch
#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
    = kArchHost
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64
  );

  //! Destroy the `X86Compiler` instance.
  ASMJIT_API ~X86Compiler();

  // --------------------------------------------------------------------------
  // [Arch]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Set the architecture to `arch`.
  ASMJIT_API Error setArch(uint32_t arch);

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

  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const X86GpVar& base, int32_t disp = 0) {
    return x86::ptr(base, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const X86GpVar& base, const X86GpVar& index, uint32_t shift = 0, int32_t disp = 0) {
    return x86::ptr(base, index, shift, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const Label& label, const X86GpVar& index, uint32_t shift, int32_t disp = 0) {
    return x86::ptr(label, index, shift, disp, _regSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr_abs(Ptr pAbs, const X86GpVar& index, uint32_t shift, int32_t disp = 0) {
    return x86::ptr_abs(pAbs, index, shift, disp, _regSize);
  }

  // --------------------------------------------------------------------------
  // [Inst / Emit]
  // --------------------------------------------------------------------------

  //! Create a new `InstNode`.
  ASMJIT_API InstNode* newInst(uint32_t code);
  //! \overload
  ASMJIT_API InstNode* newInst(uint32_t code, const Operand& o0);
  //! \overload
  ASMJIT_API InstNode* newInst(uint32_t code, const Operand& o0, const Operand& o1);
  //! \overload
  ASMJIT_API InstNode* newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2);
  //! \overload
  ASMJIT_API InstNode* newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3);
  //! \overload
  ASMJIT_API InstNode* newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3, const Operand& o4);

  //! Add a new `InstNode`.
  ASMJIT_API InstNode* emit(uint32_t code);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3, const Operand& o4);

  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, int o0);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, uint64_t o0);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, int o1);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, uint64_t o1);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, int o2);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, uint64_t o2);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, int o3);
  //! \overload
  ASMJIT_API InstNode* emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, uint64_t o3);

  // --------------------------------------------------------------------------
  // [Func]
  // --------------------------------------------------------------------------

  //! Create a new `X86FuncNode`.
  ASMJIT_API X86FuncNode* newFunc(uint32_t conv, const FuncPrototype& p);

  //! Add a new function.
  //!
  //! \param conv Calling convention to use (see \ref kFuncConv enum)
  //! \param params Function arguments prototype.
  //!
  //! This method is usually used as a first step when generating functions
  //! by `Compiler`. First parameter `cconv` specifies function calling
  //! convention to use. Second parameter `params` specifies function
  //! arguments. To create function arguments are used templates
  //! `FuncBuilder0<...>`, `FuncBuilder1<...>`, `FuncBuilder2<...>`, etc...
  //!
  //! Templates with FuncBuilder prefix are used to generate argument IDs
  //! based on real C++ types. See next example how to generate function with
  //! two 32-bit integer arguments.
  //!
  //! ~~~
  //! // Building function using asmjit::Compiler example.
  //!
  //! // Compiler instance
  //! Compiler c;
  //!
  //! // Begin of function, also emits function prolog.
  //! c.addFunc(
  //!   // Default calling convention (32-bit cdecl or 64-bit for host OS)
  //!   kFuncConvHost,
  //!   // Using function builder to generate arguments list
  //!   FuncBuilder2<Void, int, int>());
  //!
  //! // End of function, also emits function epilog.
  //! c.endFunc();
  //! ~~~
  //!
  //! You can see that building functions is really easy. Previous code snipped
  //! will generate code for function with two 32-bit integer arguments. You
  //! can access arguments by `asmjit::Function::getArg()` method. Arguments
  //! are indexed from 0 (like everything in C).
  //!
  //! ~~~
  //! // Accessing function arguments through asmjit::Function example.
  //!
  //! // Compiler instance
  //! Compiler c;
  //! X86GpVar a0(c, kVarTypeInt32);
  //! X86GpVar a1(c, kVarTypeInt32);
  //!
  //! // Begin of function (also emits function prolog)
  //! c.addFunc(
  //!   // Default calling convention (32-bit cdecl or 64-bit for host OS)
  //!   kFuncConvHost,
  //!   // Using function builder to generate arguments list
  //!   FuncBuilder2<Void, int, int>());
  //!
  //! c.setArg(0, a0);
  //! c.setArg(1, a1);
  //!
  //! // Use them.
  //! c.add(a0, a1);
  //!
  //! // End of function - emits function epilog and return instruction.
  //! c.endFunc();
  //! ~~~
  //!
  //! Arguments are like variables. How to manipulate with variables is
  //! documented in `Compiler`, variables section.
  //!
  //! \note To get current function use `getFunc()` method or save pointer to
  //! `FuncNode` returned by `Compiler::addFunc<>` method. The recommended way
  //! is saving the pointer and using it to specify function arguments and
  //! return value.
  //!
  //! \sa FuncBuilder0, FuncBuilder1, FuncBuilder2, ...
  ASMJIT_API X86FuncNode* addFunc(uint32_t conv, const FuncPrototype& p);

  //! End of current function.
  ASMJIT_API EndNode* endFunc();

  //! Get current function as `X86FuncNode`.
  //!
  //! This method can be called within `addFunc()` and `endFunc()` block to get
  //! current function you are working with. It's recommended to store `FuncNode`
  //! pointer returned by `addFunc<>` method, because this allows you in future
  //! implement function sections outside of function itself.
  ASMJIT_INLINE X86FuncNode* getFunc() const {
    return static_cast<X86FuncNode*>(_func);
  }

  // --------------------------------------------------------------------------
  // [Ret]
  // --------------------------------------------------------------------------

  //! Create a new `RetNode`.
  ASMJIT_API RetNode* newRet(const Operand& o0, const Operand& o1);
  //! Add a new `RetNode`.
  ASMJIT_API RetNode* addRet(const Operand& o0, const Operand& o1);

  // --------------------------------------------------------------------------
  // [Call]
  // --------------------------------------------------------------------------

  //! Create a new `X86CallNode`.
  ASMJIT_API X86CallNode* newCall(const Operand& o0, uint32_t conv, const FuncPrototype& p);
  //! Add a new `X86CallNode`.
  ASMJIT_API X86CallNode* addCall(const Operand& o0, uint32_t conv, const FuncPrototype& p);

  // --------------------------------------------------------------------------
  // [Vars]
  // --------------------------------------------------------------------------

  //! Set function argument to `var`.
  ASMJIT_API Error setArg(uint32_t argIndex, Var& var);

  ASMJIT_API virtual Error _newVar(Var* var, uint32_t type, const char* name);

  //! Create a new Gp variable.
  ASMJIT_INLINE X86GpVar newGpVar(uint32_t vType = kVarTypeIntPtr, const char* name = NULL) {
    ASMJIT_ASSERT(vType < kX86VarTypeCount);
    ASMJIT_ASSERT(IntUtil::inInterval<uint32_t>(vType, _kVarTypeIntStart, _kVarTypeIntEnd));

    X86GpVar var(NoInit);
    _newVar(&var, vType, name);
    return var;
  }

  //! Create a new Mm variable.
  ASMJIT_INLINE X86MmVar newMmVar(uint32_t vType = kX86VarTypeMm, const char* name = NULL) {
    ASMJIT_ASSERT(vType < kX86VarTypeCount);
    ASMJIT_ASSERT(IntUtil::inInterval<uint32_t>(vType, _kX86VarTypeMmStart, _kX86VarTypeMmEnd));

    X86MmVar var(NoInit);
    _newVar(&var, vType, name);
    return var;
  }

  //! Create a new Xmm variable.
  ASMJIT_INLINE X86XmmVar newXmmVar(uint32_t vType = kX86VarTypeXmm, const char* name = NULL) {
    ASMJIT_ASSERT(vType < kX86VarTypeCount);
    ASMJIT_ASSERT(IntUtil::inInterval<uint32_t>(vType, _kX86VarTypeXmmStart, _kX86VarTypeXmmEnd));

    X86XmmVar var(NoInit);
    _newVar(&var, vType, name);
    return var;
  }

  //! Create a new Ymm variable.
  ASMJIT_INLINE X86YmmVar newYmmVar(uint32_t vType = kX86VarTypeYmm, const char* name = NULL) {
    ASMJIT_ASSERT(vType < kX86VarTypeCount);
    ASMJIT_ASSERT(IntUtil::inInterval<uint32_t>(vType, _kX86VarTypeYmmStart, _kX86VarTypeYmmEnd));

    X86YmmVar var(NoInit);
    _newVar(&var, vType, name);
    return var;
  }

  // --------------------------------------------------------------------------
  // [Stack]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _newStack(BaseMem* mem, uint32_t size, uint32_t alignment, const char* name);

  //! Create a new memory chunk allocated on the current function's stack.
  ASMJIT_INLINE X86Mem newStack(uint32_t size, uint32_t alignment, const char* name = NULL) {
    X86Mem m(NoInit);
    _newStack(&m, size, alignment, name);
    return m;
  }

  // --------------------------------------------------------------------------
  // [Const]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _newConst(BaseMem* mem, uint32_t scope, const void* data, size_t size);

  //! Put data to a constant-pool and get a memory reference to it.
  ASMJIT_INLINE X86Mem newConst(uint32_t scope, const void* data, size_t size) {
    X86Mem m(NoInit);
    _newConst(&m, scope, data, size);
    return m;
  }

  //! Put a BYTE `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newByteConst(uint32_t scope, uint8_t val) { return newConst(scope, &val, 1); }
  //! Put a WORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newWordConst(uint32_t scope, uint16_t val) { return newConst(scope, &val, 2); }
  //! Put a DWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newDWordConst(uint32_t scope, uint32_t val) { return newConst(scope, &val, 4); }
  //! Put a QWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newQWordConst(uint32_t scope, uint64_t val) { return newConst(scope, &val, 8); }

  //! Put a WORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newInt16Const(uint32_t scope, int16_t val) { return newConst(scope, &val, 2); }
  //! Put a WORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newUInt16Const(uint32_t scope, uint16_t val) { return newConst(scope, &val, 2); }
  //! Put a DWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newInt32Const(uint32_t scope, int32_t val) { return newConst(scope, &val, 4); }
  //! Put a DWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newUInt32Const(uint32_t scope, uint32_t val) { return newConst(scope, &val, 4); }
  //! Put a QWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newInt64Const(uint32_t scope, int64_t val) { return newConst(scope, &val, 8); }
  //! Put a QWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newUInt64Const(uint32_t scope, uint64_t val) { return newConst(scope, &val, 8); }

  //! Put a SP-FP `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newFloatConst(uint32_t scope, float val) { return newConst(scope, &val, 4); }
  //! Put a DP-FP `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newDoubleConst(uint32_t scope, double val) { return newConst(scope, &val, 8); }

  //! Put a MMX `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newMmConst(uint32_t scope, const Vec64& val) { return newConst(scope, &val, 8); }
  //! Put a XMM `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newXmmConst(uint32_t scope, const Vec128& val) { return newConst(scope, &val, 16); }
  //! Put a YMM `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newYmmConst(uint32_t scope, const Vec256& val) { return newConst(scope, &val, 32); }

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* db(uint8_t x) { return embed(&x, 1); }
  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dw(uint16_t x) { return embed(&x, 2); }
  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dd(uint32_t x) { return embed(&x, 4); }
  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dq(uint64_t x) { return embed(&x, 8); }

  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dint8(int8_t x) { return embed(&x, static_cast<uint32_t>(sizeof(int8_t))); }
  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* duint8(uint8_t x) { return embed(&x, static_cast<uint32_t>(sizeof(uint8_t))); }

  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dint16(int16_t x) { return embed(&x, static_cast<uint32_t>(sizeof(int16_t))); }
  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* duint16(uint16_t x) { return embed(&x, static_cast<uint32_t>(sizeof(uint16_t))); }

  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dint32(int32_t x) { return embed(&x, static_cast<uint32_t>(sizeof(int32_t))); }
  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* duint32(uint32_t x) { return embed(&x, static_cast<uint32_t>(sizeof(uint32_t))); }

  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dint64(int64_t x) { return embed(&x, static_cast<uint32_t>(sizeof(int64_t))); }
  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE EmbedNode* duint64(uint64_t x) { return embed(&x, static_cast<uint32_t>(sizeof(uint64_t))); }

  //! Add float data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dfloat(float x) { return embed(&x, static_cast<uint32_t>(sizeof(float))); }
  //! Add double data to the instruction stream.
  ASMJIT_INLINE EmbedNode* ddouble(double x) { return embed(&x, static_cast<uint32_t>(sizeof(double))); }

  //! Add Mm data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dmm(const Vec64& x) { return embed(&x, static_cast<uint32_t>(sizeof(Vec64))); }
  //! Add Xmm data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dxmm(const Vec128& x) { return embed(&x, static_cast<uint32_t>(sizeof(Vec128))); }
  //! Add Ymm data to the instruction stream.
  ASMJIT_INLINE EmbedNode* dymm(const Vec256& x) { return embed(&x, static_cast<uint32_t>(sizeof(Vec256))); }

  //! Add data in a given structure instance to the instruction stream.
  template<typename T>
  ASMJIT_INLINE EmbedNode* dstruct(const T& x) { return embed(&x, static_cast<uint32_t>(sizeof(T))); }

  // --------------------------------------------------------------------------
  // [Make]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void* make();

  // -------------------------------------------------------------------------
  // [Assembler]
  // -------------------------------------------------------------------------

  ASMJIT_API virtual Assembler* _newAssembler();

  // -------------------------------------------------------------------------
  // [Serialize]
  // -------------------------------------------------------------------------

  ASMJIT_API virtual Error serialize(Assembler* assembler);

  // -------------------------------------------------------------------------
  // [Options]
  // -------------------------------------------------------------------------

  ASMJIT_X86_EMIT_OPTIONS(X86Compiler)

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
  ASMJIT_INLINE InstNode* _Inst_() { \
    return emit(_Code_); \
  }

#define INST_1x(_Inst_, _Code_, _Op0_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0) { \
    return emit(_Code_, o0); \
  }

#define INST_1x_(_Inst_, _Code_, _Op0_, _Cond_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0); \
  }

#define INST_1i(_Inst_, _Code_, _Op0_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0) { \
    return emit(_Code_, o0); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(int o0) { \
    return emit(_Code_, o0); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(unsigned int o0) { \
    return emit(_Code_, static_cast<uint64_t>(o0)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(int64_t o0) { \
    return emit(_Code_, static_cast<uint64_t>(o0)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(uint64_t o0) { \
    return emit(_Code_, o0); \
  }

#define INST_1cc(_Inst_, _Code_, _Translate_, _Op0_) \
  ASMJIT_INLINE InstNode* _Inst_(uint32_t cc, const _Op0_& o0) { \
    return emit(_Translate_(cc), o0); \
  } \
  \
  ASMJIT_INLINE InstNode* _Inst_##a(const _Op0_& o0) { return emit(_Code_##a, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##ae(const _Op0_& o0) { return emit(_Code_##ae, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##b(const _Op0_& o0) { return emit(_Code_##b, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##be(const _Op0_& o0) { return emit(_Code_##be, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##c(const _Op0_& o0) { return emit(_Code_##c, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##e(const _Op0_& o0) { return emit(_Code_##e, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##g(const _Op0_& o0) { return emit(_Code_##g, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##ge(const _Op0_& o0) { return emit(_Code_##ge, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##l(const _Op0_& o0) { return emit(_Code_##l, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##le(const _Op0_& o0) { return emit(_Code_##le, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##na(const _Op0_& o0) { return emit(_Code_##na, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nae(const _Op0_& o0) { return emit(_Code_##nae, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nb(const _Op0_& o0) { return emit(_Code_##nb, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nbe(const _Op0_& o0) { return emit(_Code_##nbe, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nc(const _Op0_& o0) { return emit(_Code_##nc, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##ne(const _Op0_& o0) { return emit(_Code_##ne, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##ng(const _Op0_& o0) { return emit(_Code_##ng, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nge(const _Op0_& o0) { return emit(_Code_##nge, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nl(const _Op0_& o0) { return emit(_Code_##nl, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nle(const _Op0_& o0) { return emit(_Code_##nle, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##no(const _Op0_& o0) { return emit(_Code_##no, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##np(const _Op0_& o0) { return emit(_Code_##np, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##ns(const _Op0_& o0) { return emit(_Code_##ns, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##nz(const _Op0_& o0) { return emit(_Code_##nz, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##o(const _Op0_& o0) { return emit(_Code_##o, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##p(const _Op0_& o0) { return emit(_Code_##p, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##pe(const _Op0_& o0) { return emit(_Code_##pe, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##po(const _Op0_& o0) { return emit(_Code_##po, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##s(const _Op0_& o0) { return emit(_Code_##s, o0); } \
  ASMJIT_INLINE InstNode* _Inst_##z(const _Op0_& o0) { return emit(_Code_##z, o0); }

#define INST_2x(_Inst_, _Code_, _Op0_, _Op1_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1) { \
    return emit(_Code_, o0, o1); \
  }

#define INST_2x_(_Inst_, _Code_, _Op0_, _Op1_, _Cond_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0, o1); \
  }

#define INST_2i(_Inst_, _Code_, _Op0_, _Op1_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1) { \
    return emit(_Code_, o0, o1); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, int o1) { \
    return emit(_Code_, o0, o1); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, unsigned int o1) { \
    return emit(_Code_, o0, static_cast<uint64_t>(o1)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, int64_t o1) { \
    return emit(_Code_, o0, static_cast<uint64_t>(o1)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, uint64_t o1) { \
    return emit(_Code_, o0, o1); \
  }

#define INST_2cc(_Inst_, _Code_, _Translate_, _Op0_, _Op1_) \
  ASMJIT_INLINE InstNode* _Inst_(uint32_t cc, const _Op0_& o0, const _Op1_& o1) { \
    return emit(_Translate_(cc), o0, o1); \
  } \
  \
  ASMJIT_INLINE InstNode* _Inst_##a(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##a, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##ae(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ae, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##b(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##b, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##be(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##be, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##c(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##c, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##e(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##e, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##g(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##g, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##ge(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ge, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##l(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##l, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##le(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##le, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##na(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##na, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nae(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nae, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nb(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nb, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nbe(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nbe, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nc(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nc, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##ne(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ne, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##ng(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ng, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nge(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nge, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nl(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nl, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nle(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nle, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##no(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##no, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##np(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##np, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##ns(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##ns, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##nz(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##nz, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##o(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##o, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##p(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##p, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##pe(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##pe, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##po(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##po, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##s(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##s, o0, o1); } \
  ASMJIT_INLINE InstNode* _Inst_##z(const _Op0_& o0, const _Op1_& o1) { return emit(_Code_##z, o0, o1); }

#define INST_3x(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    return emit(_Code_, o0, o1, o2); \
  }

#define INST_3x_(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Cond_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0, o1, o2); \
  }

#define INST_3i(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    return emit(_Code_, o0, o1, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, int o2) { \
    return emit(_Code_, o0, o1, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, unsigned int o2) { \
    return emit(_Code_, o0, o1, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, int64_t o2) { \
    return emit(_Code_, o0, o1, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, uint64_t o2) { \
    return emit(_Code_, o0, o1, o2); \
  }

#define INST_3ii(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2) { \
    return emit(_Code_, o0, o1, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, int o1, int o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, o2); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, unsigned int o1, unsigned int o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, int64_t o1, int64_t o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, static_cast<uint64_t>(o2)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, uint64_t o1, uint64_t o2) { \
    Imm o1Imm(o1); \
    return emit(_Code_, o0, o1Imm, o2); \
  }

#define INST_4x(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  }

#define INST_4x_(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Cond_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    ASMJIT_ASSERT(_Cond_); \
    return emit(_Code_, o0, o1, o2, o3); \
  }

#define INST_4i(_Inst_, _Code_, _Op0_, _Op1_, _Op2_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, int o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, unsigned int o3) { \
    return emit(_Code_, o0, o1, o2, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, int64_t o3) { \
    return emit(_Code_, o0, o1, o2, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, uint64_t o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  }

#define INST_4ii(_Inst_, _Code_, _Op0_, _Op1_, _Op2_, _Op3_) \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, const _Op2_& o2, const _Op3_& o3) { \
    return emit(_Code_, o0, o1, o2, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, int o2, int o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, o3); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, unsigned int o2, unsigned int o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, int64_t o2, int64_t o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, static_cast<uint64_t>(o3)); \
  } \
  /*! \overload */ \
  ASMJIT_INLINE InstNode* _Inst_(const _Op0_& o0, const _Op1_& o1, uint64_t o2, uint64_t o3) { \
    Imm o2Imm(o2); \
    return emit(_Code_, o0, o1, o2Imm, o3); \
  }

  // --------------------------------------------------------------------------
  // [X86/X64]
  // --------------------------------------------------------------------------

  //! Add with carry.
  INST_2x(adc, kX86InstIdAdc, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(adc, kX86InstIdAdc, X86GpVar, X86Mem)
  //! \overload
  INST_2i(adc, kX86InstIdAdc, X86GpVar, Imm)
  //! \overload
  INST_2x(adc, kX86InstIdAdc, X86Mem, X86GpVar)
  //! \overload
  INST_2i(adc, kX86InstIdAdc, X86Mem, Imm)

  //! Add.
  INST_2x(add, kX86InstIdAdd, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(add, kX86InstIdAdd, X86GpVar, X86Mem)
  //! \overload
  INST_2i(add, kX86InstIdAdd, X86GpVar, Imm)
  //! \overload
  INST_2x(add, kX86InstIdAdd, X86Mem, X86GpVar)
  //! \overload
  INST_2i(add, kX86InstIdAdd, X86Mem, Imm)

  //! And.
  INST_2x(and_, kX86InstIdAnd, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(and_, kX86InstIdAnd, X86GpVar, X86Mem)
  //! \overload
  INST_2i(and_, kX86InstIdAnd, X86GpVar, Imm)
  //! \overload
  INST_2x(and_, kX86InstIdAnd, X86Mem, X86GpVar)
  //! \overload
  INST_2i(and_, kX86InstIdAnd, X86Mem, Imm)

  //! Bit scan forward.
  INST_2x_(bsf, kX86InstIdBsf, X86GpVar, X86GpVar, !o0.isGpb())
  //! \overload
  INST_2x_(bsf, kX86InstIdBsf, X86GpVar, X86Mem, !o0.isGpb())

  //! Bit scan reverse.
  INST_2x_(bsr, kX86InstIdBsr, X86GpVar, X86GpVar, !o0.isGpb())
  //! \overload
  INST_2x_(bsr, kX86InstIdBsr, X86GpVar, X86Mem, !o0.isGpb())

  //! Byte swap (32-bit or 64-bit registers only) (i486).
  INST_1x_(bswap, kX86InstIdBswap, X86GpVar, o0.getSize() >= 4)

  //! Bit test.
  INST_2x(bt, kX86InstIdBt, X86GpVar, X86GpVar)
  //! \overload
  INST_2i(bt, kX86InstIdBt, X86GpVar, Imm)
  //! \overload
  INST_2x(bt, kX86InstIdBt, X86Mem, X86GpVar)
  //! \overload
  INST_2i(bt, kX86InstIdBt, X86Mem, Imm)

  //! Bit test and complement.
  INST_2x(btc, kX86InstIdBtc, X86GpVar, X86GpVar)
  //! \overload
  INST_2i(btc, kX86InstIdBtc, X86GpVar, Imm)
  //! \overload
  INST_2x(btc, kX86InstIdBtc, X86Mem, X86GpVar)
  //! \overload
  INST_2i(btc, kX86InstIdBtc, X86Mem, Imm)

  //! Bit test and reset.
  INST_2x(btr, kX86InstIdBtr, X86GpVar, X86GpVar)
  //! \overload
  INST_2i(btr, kX86InstIdBtr, X86GpVar, Imm)
  //! \overload
  INST_2x(btr, kX86InstIdBtr, X86Mem, X86GpVar)
  //! \overload
  INST_2i(btr, kX86InstIdBtr, X86Mem, Imm)

  //! Bit test and set.
  INST_2x(bts, kX86InstIdBts, X86GpVar, X86GpVar)
  //! \overload
  INST_2i(bts, kX86InstIdBts, X86GpVar, Imm)
  //! \overload
  INST_2x(bts, kX86InstIdBts, X86Mem, X86GpVar)
  //! \overload
  INST_2i(bts, kX86InstIdBts, X86Mem, Imm)

  //! Call a function.
  ASMJIT_INLINE X86CallNode* call(const X86GpVar& dst, uint32_t conv, const FuncPrototype& p) {
    return addCall(dst, conv, p);
  }
  //! \overload
  ASMJIT_INLINE X86CallNode* call(const X86Mem& dst, uint32_t conv, const FuncPrototype& p) {
    return addCall(dst, conv, p);
  }
  //! \overload
  ASMJIT_INLINE X86CallNode* call(const Label& label, uint32_t conv, const FuncPrototype& p) {
    return addCall(label, conv, p);
  }
  //! \overload
  ASMJIT_INLINE X86CallNode* call(const Imm& dst, uint32_t conv, const FuncPrototype& p) {
    return addCall(dst, conv, p);
  }
  //! \overload
  ASMJIT_INLINE X86CallNode* call(Ptr dst, uint32_t conv, const FuncPrototype& p) {
    return addCall(Imm(dst), conv, p);
  }

  //! Clear carry flag
  INST_0x(clc, kX86InstIdClc)
  //! Clear direction flag
  INST_0x(cld, kX86InstIdCld)
  //! Complement carry Flag.
  INST_0x(cmc, kX86InstIdCmc)

  //! Convert BYTE to WORD (AX <- Sign Extend AL).
  INST_1x(cbw, kX86InstIdCbw, X86GpVar  /* al */)
  //! Convert DWORD to QWORD (EDX:EAX <- Sign Extend EAX).
  INST_2x(cdq, kX86InstIdCdq, X86GpVar /* edx */, X86GpVar /* eax */)
  //! Convert DWORD to QWORD (RAX <- Sign Extend EAX) (X64 Only).
  INST_1x(cdqe, kX86InstIdCdqe, X86GpVar /* eax */)
  //! Convert QWORD to OWORD (RDX:RAX <- Sign Extend RAX) (X64 Only).
  INST_2x(cqo, kX86InstIdCdq, X86GpVar /* rdx */, X86GpVar /* rax */)
  //! Convert WORD to DWORD (DX:AX <- Sign Extend AX).
  INST_2x(cwd, kX86InstIdCwd, X86GpVar  /* dx */, X86GpVar /* ax */)
  //! Convert WORD to DWORD (EAX <- Sign Extend AX).
  INST_1x(cwde, kX86InstIdCwde, X86GpVar /* eax */)

  //! Conditional move.
  INST_2cc(cmov, kX86InstIdCmov, X86Util::condToCmovcc, X86GpVar, X86GpVar)
  //! Conditional move.
  INST_2cc(cmov, kX86InstIdCmov, X86Util::condToCmovcc, X86GpVar, X86Mem)

  //! Compare two operands.
  INST_2x(cmp, kX86InstIdCmp, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(cmp, kX86InstIdCmp, X86GpVar, X86Mem)
  //! \overload
  INST_2i(cmp, kX86InstIdCmp, X86GpVar, Imm)
  //! \overload
  INST_2x(cmp, kX86InstIdCmp, X86Mem, X86GpVar)
  //! \overload
  INST_2i(cmp, kX86InstIdCmp, X86Mem, Imm)

  //! Compare BYTE in ES:`o0` and DS:`o1`.
  INST_2x(cmpsb, kX86InstIdCmpsB, X86GpVar, X86GpVar)
  //! Compare DWORD in ES:`o0` and DS:`o1`.
  INST_2x(cmpsd, kX86InstIdCmpsD, X86GpVar, X86GpVar)
  //! Compare QWORD in ES:`o0` and DS:`o1` (X64 Only).
  INST_2x(cmpsq, kX86InstIdCmpsQ, X86GpVar, X86GpVar)
  //! Compare WORD in ES:`o0` and DS:`o1`.
  INST_2x(cmpsw, kX86InstIdCmpsW, X86GpVar, X86GpVar)

  //! Compare and exchange (i486).
  INST_3x(cmpxchg, kX86InstIdCmpxchg, X86GpVar /* eax */, X86GpVar, X86GpVar)
  //! \overload
  INST_3x(cmpxchg, kX86InstIdCmpxchg, X86GpVar /* eax */, X86Mem, X86GpVar)

  //! Compare and exchange 128-bit value in RDX:RAX with `x_mem` (X64 Only).
  ASMJIT_INLINE InstNode* cmpxchg16b(
    const X86GpVar& r_edx, const X86GpVar& r_eax,
    const X86GpVar& r_ecx, const X86GpVar& r_ebx,
    const X86Mem& x_mem) {

    return emit(kX86InstIdCmpxchg16b, r_edx, r_eax, r_ecx, r_ebx, x_mem);
  }

  //! Compare and exchange 64-bit value in EDX:EAX with `x_mem` (Pentium).
  ASMJIT_INLINE InstNode* cmpxchg8b(
    const X86GpVar& r_edx, const X86GpVar& r_eax,
    const X86GpVar& r_ecx, const X86GpVar& r_ebx,
    const X86Mem& x_mem) {

    return emit(kX86InstIdCmpxchg8b, r_edx, r_eax, r_ecx, r_ebx, x_mem);
  }

  //! CPU identification (i486).
  ASMJIT_INLINE InstNode* cpuid(
    const X86GpVar& x_eax,
    const X86GpVar& w_ebx,
    const X86GpVar& x_ecx,
    const X86GpVar& w_edx) {

    // Destination variables must be different.
    ASMJIT_ASSERT(x_eax.getId() != w_ebx.getId() &&
                  w_ebx.getId() != x_ecx.getId() &&
                  x_ecx.getId() != w_edx.getId());

    return emit(kX86InstIdCpuid, x_eax, w_ebx, x_ecx, w_edx);
  }

  //! Decimal adjust AL after addition (X86 Only).
  INST_1x(daa, kX86InstIdDaa, X86GpVar)
  //! Decimal adjust AL after subtraction (X86 Only).
  INST_1x(das, kX86InstIdDas, X86GpVar)

  //! Decrement by 1.
  INST_1x(dec, kX86InstIdDec, X86GpVar)
  //! \overload
  INST_1x(dec, kX86InstIdDec, X86Mem)

  //! Unsigned divide (o0:o1 <- o0:o1 / o2).
  //!
  //! Remainder is stored in `o0`, quotient is stored in `o1`.
  INST_3x_(div, kX86InstIdDiv, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId())
  //! \overload
  INST_3x_(div, kX86InstIdDiv, X86GpVar, X86GpVar, X86Mem, o0.getId() != o1.getId())

  //! Signed divide (o0:o1 <- o0:o1 / o2).
  //!
  //! Remainder is stored in `o0`, quotient is stored in `o1`.
  INST_3x_(idiv, kX86InstIdIdiv, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId())
  //! \overload
  INST_3x_(idiv, kX86InstIdIdiv, X86GpVar, X86GpVar, X86Mem, o0.getId() != o1.getId())

  //! Signed multiply (o0:o1 <- o1 * o2).
  //!
  //! Hi value is stored in `o0`, lo value is stored in `o1`.
  INST_3x_(imul, kX86InstIdImul, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId())
  //! \overload
  INST_3x_(imul, kX86InstIdImul, X86GpVar, X86GpVar, X86Mem, o0.getId() != o1.getId())

  //! Signed multiply.
  INST_2x(imul, kX86InstIdImul, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(imul, kX86InstIdImul, X86GpVar, X86Mem)
  //! \overload
  INST_2i(imul, kX86InstIdImul, X86GpVar, Imm)

  //! Signed multiply.
  INST_3i(imul, kX86InstIdImul, X86GpVar, X86GpVar, Imm)
  //! \overload
  INST_3i(imul, kX86InstIdImul, X86GpVar, X86Mem, Imm)

  //! Increment by 1.
  INST_1x(inc, kX86InstIdInc, X86GpVar)
  //! \overload
  INST_1x(inc, kX86InstIdInc, X86Mem)

  //! Interrupt.
  INST_1i(int_, kX86InstIdInt, Imm)
  //! Interrupt 3 - trap to debugger.
  ASMJIT_INLINE InstNode* int3() { return int_(3); }

  //! Jump to label `label` if condition `cc` is met.
  INST_1cc(j, kX86InstIdJ, X86Util::condToJcc, Label)

  //! Short jump if CX/ECX/RCX is zero.
  INST_2x(jecxz, kX86InstIdJecxz, X86GpVar, Label)

  //! Jump.
  INST_1x(jmp, kX86InstIdJmp, X86GpVar)
  //! \overload
  INST_1x(jmp, kX86InstIdJmp, X86Mem)
  //! \overload
  INST_1x(jmp, kX86InstIdJmp, Label)
  //! \overload
  INST_1x(jmp, kX86InstIdJmp, Imm)
  //! \overload
  ASMJIT_INLINE InstNode* jmp(Ptr dst) { return jmp(Imm(dst)); }

  //! Load AH from flags.
  INST_1x(lahf, kX86InstIdLahf, X86GpVar)

  //! Load effective address
  INST_2x(lea, kX86InstIdLea, X86GpVar, X86Mem)

  //! Load BYTE from DS:`o1` to `o0`.
  INST_2x(lodsb, kX86InstIdLodsB, X86GpVar, X86GpVar)
  //! Load DWORD from DS:`o1` to `o0`.
  INST_2x(lodsd, kX86InstIdLodsD, X86GpVar, X86GpVar)
  //! Load QWORD from DS:`o1` to `o0` (X64 Only).
  INST_2x(lodsq, kX86InstIdLodsQ, X86GpVar, X86GpVar)
  //! Load WORD from DS:`o1` to `o0`.
  INST_2x(lodsw, kX86InstIdLodsW, X86GpVar, X86GpVar)

  //! Move.
  INST_2x(mov, kX86InstIdMov, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86GpVar, X86Mem)
  //! \overload
  INST_2i(mov, kX86InstIdMov, X86GpVar, Imm)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86Mem, X86GpVar)
  //! \overload
  INST_2i(mov, kX86InstIdMov, X86Mem, Imm)

  //! Move from segment register.
  INST_2x(mov, kX86InstIdMov, X86GpVar, X86SegReg)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86Mem, X86SegReg)
  //! Move to segment register.
  INST_2x(mov, kX86InstIdMov, X86SegReg, X86GpVar)
  //! \overload
  INST_2x(mov, kX86InstIdMov, X86SegReg, X86Mem)

  //! Move (AL|AX|EAX|RAX <- absolute address in immediate).
  INST_2x(mov_ptr, kX86InstIdMovPtr, X86GpReg, Imm);
  //! \overload
  ASMJIT_INLINE InstNode* mov_ptr(const X86GpReg& o0, Ptr o1) {
    ASMJIT_ASSERT(o0.getRegIndex() == 0);
    return emit(kX86InstIdMovPtr, o0, Imm(o1));
  }

  //! Move (absolute address in immediate <- AL|AX|EAX|RAX).
  INST_2x(mov_ptr, kX86InstIdMovPtr, Imm, X86GpReg);
  //! \overload
  ASMJIT_INLINE InstNode* mov_ptr(Ptr o0, const X86GpReg& o1) {
    ASMJIT_ASSERT(o1.getRegIndex() == 0);
    return emit(kX86InstIdMovPtr, Imm(o0), o1);
  }

  //! Move data after swapping bytes (SSE3 - Atom).
  INST_2x_(movbe, kX86InstIdMovbe, X86GpVar, X86Mem, !o0.isGpb());
  //! \overload
  INST_2x_(movbe, kX86InstIdMovbe, X86Mem, X86GpVar, !o1.isGpb());

  //! Load BYTE from DS:`o1` to ES:`o0`.
  INST_2x(movsb, kX86InstIdMovsB, X86GpVar, X86GpVar)
  //! Load DWORD from DS:`o1` to ES:`o0`.
  INST_2x(movsd, kX86InstIdMovsD, X86GpVar, X86GpVar)
  //! Load QWORD from DS:`o1` to ES:`o0` (X64 Only).
  INST_2x(movsq, kX86InstIdMovsQ, X86GpVar, X86GpVar)
  //! Load WORD from DS:`o1` to ES:`o0`.
  INST_2x(movsw, kX86InstIdMovsW, X86GpVar, X86GpVar)

  //! Move with sign-extension.
  INST_2x(movsx, kX86InstIdMovsx, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(movsx, kX86InstIdMovsx, X86GpVar, X86Mem)

  //! Move DWORD to QWORD with sign-extension (X64 Only).
  INST_2x(movsxd, kX86InstIdMovsxd, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(movsxd, kX86InstIdMovsxd, X86GpVar, X86Mem)

  //! Move with zero-extension.
  INST_2x(movzx, kX86InstIdMovzx, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(movzx, kX86InstIdMovzx, X86GpVar, X86Mem)

  //! Unsigned multiply (o0:o1 <- o1 * o2).
  INST_3x_(mul, kX86InstIdMul, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId())
  //! \overload
  INST_3x_(mul, kX86InstIdMul, X86GpVar, X86GpVar, X86Mem, o0.getId() != o1.getId())

  //! Two's complement negation.
  INST_1x(neg, kX86InstIdNeg, X86GpVar)
  //! \overload
  INST_1x(neg, kX86InstIdNeg, X86Mem)

  //! No operation.
  INST_0x(nop, kX86InstIdNop)

  //! One's complement negation.
  INST_1x(not_, kX86InstIdNot, X86GpVar)
  //! \overload
  INST_1x(not_, kX86InstIdNot, X86Mem)

  //! Or.
  INST_2x(or_, kX86InstIdOr, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(or_, kX86InstIdOr, X86GpVar, X86Mem)
  //! \overload
  INST_2i(or_, kX86InstIdOr, X86GpVar, Imm)
  //! \overload
  INST_2x(or_, kX86InstIdOr, X86Mem, X86GpVar)
  //! \overload
  INST_2i(or_, kX86InstIdOr, X86Mem, Imm)

  //! Pop a value from the stack.
  INST_1x_(pop, kX86InstIdPop, X86GpVar, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! \overload
  INST_1x_(pop, kX86InstIdPop, X86Mem, o0.getSize() == 2 || o0.getSize() == _regSize)

  //! Pop stack into EFLAGS Register (32-bit or 64-bit).
  INST_0x(popf, kX86InstIdPopf)

  //! Push WORD or DWORD/QWORD on the stack.
  INST_1x_(push, kX86InstIdPush, X86GpVar, o0.getSize() == 2 || o0.getSize() == _regSize)
  //! Push WORD or DWORD/QWORD on the stack.
  INST_1x_(push, kX86InstIdPush, X86Mem,o0.getSize() == 2 || o0.getSize() == _regSize)
  //! Push segment register on the stack.
  INST_1x(push, kX86InstIdPush, X86SegReg)
  //! Push WORD or DWORD/QWORD on the stack.
  INST_1i(push, kX86InstIdPush, Imm)

  //! Push EFLAGS register (32-bit or 64-bit) on the stack.
  INST_0x(pushf, kX86InstIdPushf)

  //! Rotate bits left.
  INST_2x(rcl, kX86InstIdRcl, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(rcl, kX86InstIdRcl, X86Mem, X86GpVar)
  //! Rotate bits left.
  INST_2i(rcl, kX86InstIdRcl, X86GpVar, Imm)
  //! \overload
  INST_2i(rcl, kX86InstIdRcl, X86Mem, Imm)

  //! Rotate bits right.
  INST_2x(rcr, kX86InstIdRcr, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(rcr, kX86InstIdRcr, X86Mem, X86GpVar)
  //! Rotate bits right.
  INST_2i(rcr, kX86InstIdRcr, X86GpVar, Imm)
  //! \overload
  INST_2i(rcr, kX86InstIdRcr, X86Mem, Imm)

  //! Read time-stamp counter (Pentium).
  INST_2x_(rdtsc, kX86InstIdRdtsc, X86GpVar, X86GpVar, o0.getId() != o1.getId())
  //! Read time-stamp counter and processor id (Pentium).
  INST_3x_(rdtscp, kX86InstIdRdtscp, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated load ECX/RCX BYTEs from DS:[ESI/RSI] to AL.
  INST_3x_(rep_lodsb, kX86InstIdRepLodsB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated load ECX/RCX DWORDs from DS:[ESI/RSI] to AL.
  INST_3x_(rep_lodsd, kX86InstIdRepLodsD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated load ECX/RCX QWORDs from DS:[RSI] to RAX (X64 Only).
  INST_3x_(rep_lodsq, kX86InstIdRepLodsQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated load ECX/RCX WORDs from DS:[ESI/RSI] to AX.
  INST_3x_(rep_lodsw, kX86InstIdRepLodsW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated move ECX/RCX BYTEs from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_3x_(rep_movsb, kX86InstIdRepMovsB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated move ECX/RCX DWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_3x_(rep_movsd, kX86InstIdRepMovsD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated move ECX/RCX QWORDs from DS:[RSI] to ES:[RDI] (X64 Only).
  INST_3x_(rep_movsq, kX86InstIdRepMovsQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated move ECX/RCX DWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  INST_3x_(rep_movsw, kX86InstIdRepMovsW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated fill ECX/RCX BYTEs at ES:[EDI/RDI] with AL.
  INST_3x_(rep_stosb, kX86InstIdRepStosB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated fill ECX/RCX DWORDs at ES:[EDI/RDI] with EAX.
  INST_3x_(rep_stosd, kX86InstIdRepStosD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated fill ECX/RCX QWORDs at ES:[RDI] with RAX (X64 Only).
  INST_3x_(rep_stosq, kX86InstIdRepStosQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated fill ECX/RCX WORDs at ES:[EDI/RDI] with AX.
  INST_3x_(rep_stosw, kX86InstIdRepStosW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated find non-AL BYTEs in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_3x_(repe_cmpsb, kX86InstIdRepeCmpsB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find non-EAX DWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_3x_(repe_cmpsd, kX86InstIdRepeCmpsD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find non-RAX QWORDs in ES:[RDI] and DS:[RDI] (X64 Only).
  INST_3x_(repe_cmpsq, kX86InstIdRepeCmpsQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find non-AX WORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  INST_3x_(repe_cmpsw, kX86InstIdRepeCmpsW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated find non-AL BYTE starting at ES:[EDI/RDI].
  INST_3x_(repe_scasb, kX86InstIdRepeScasB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find non-EAX DWORD starting at ES:[EDI/RDI].
  INST_3x_(repe_scasd, kX86InstIdRepeScasD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find non-RAX QWORD starting at ES:[RDI] (X64 Only).
  INST_3x_(repe_scasq, kX86InstIdRepeScasQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find non-AX WORD starting at ES:[EDI/RDI].
  INST_3x_(repe_scasw, kX86InstIdRepeScasW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated find AL BYTEs in [RDI] and [RSI].
  INST_3x_(repne_cmpsb, kX86InstIdRepneCmpsB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find EAX DWORDs in [RDI] and [RSI].
  INST_3x_(repne_cmpsd, kX86InstIdRepneCmpsD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find RAX QWORDs in [RDI] and [RSI] (X64 Only).
  INST_3x_(repne_cmpsq, kX86InstIdRepneCmpsQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find AX WORDs in [RDI] and [RSI].
  INST_3x_(repne_cmpsw, kX86InstIdRepneCmpsW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Repeated Find AL BYTEs, starting at ES:[EDI/RDI].
  INST_3x_(repne_scasb, kX86InstIdRepneScasB, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find EAX DWORDs, starting at ES:[EDI/RDI].
  INST_3x_(repne_scasd, kX86InstIdRepneScasD, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find RAX QWORDs, starting at ES:[RDI] (X64 Only).
  INST_3x_(repne_scasq, kX86InstIdRepneScasQ, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())
  //! Repeated find AX WORDs, starting at ES:[EDI/RDI].
  INST_3x_(repne_scasw, kX86InstIdRepneScasW, X86GpVar, X86GpVar, X86GpVar, o0.getId() != o1.getId() && o1.getId() != o2.getId())

  //! Return.
  ASMJIT_INLINE RetNode* ret() { return addRet(noOperand, noOperand); }
  //! \overload
  ASMJIT_INLINE RetNode* ret(const X86GpVar& o0) { return addRet(o0, noOperand); }
  //! \overload
  ASMJIT_INLINE RetNode* ret(const X86GpVar& o0, const X86GpVar& o1) { return addRet(o0, o1); }
  //! \overload
  ASMJIT_INLINE RetNode* ret(const X86XmmVar& o0) { return addRet(o0, noOperand); }
  //! \overload
  ASMJIT_INLINE RetNode* ret(const X86XmmVar& o0, const X86XmmVar& o1) { return addRet(o0, o1); }

  //! Rotate bits left.
  INST_2x(rol, kX86InstIdRol, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(rol, kX86InstIdRol, X86Mem, X86GpVar)
  //! Rotate bits left.
  INST_2i(rol, kX86InstIdRol, X86GpVar, Imm)
  //! \overload
  INST_2i(rol, kX86InstIdRol, X86Mem, Imm)

  //! Rotate bits right.
  INST_2x(ror, kX86InstIdRor, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(ror, kX86InstIdRor, X86Mem, X86GpVar)
  //! Rotate bits right.
  INST_2i(ror, kX86InstIdRor, X86GpVar, Imm)
  //! \overload
  INST_2i(ror, kX86InstIdRor, X86Mem, Imm)

  //! Store `a` (allocated in AH/AX/EAX/RAX) into Flags.
  INST_1x(sahf, kX86InstIdSahf, X86GpVar)

  //! Integer subtraction with borrow.
  INST_2x(sbb, kX86InstIdSbb, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(sbb, kX86InstIdSbb, X86GpVar, X86Mem)
  //! \overload
  INST_2i(sbb, kX86InstIdSbb, X86GpVar, Imm)
  //! \overload
  INST_2x(sbb, kX86InstIdSbb, X86Mem, X86GpVar)
  //! \overload
  INST_2i(sbb, kX86InstIdSbb, X86Mem, Imm)

  //! Shift bits left.
  INST_2x(sal, kX86InstIdSal, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(sal, kX86InstIdSal, X86Mem, X86GpVar)
  //! Shift bits left.
  INST_2i(sal, kX86InstIdSal, X86GpVar, Imm)
  //! \overload
  INST_2i(sal, kX86InstIdSal, X86Mem, Imm)

  //! Shift bits right.
  INST_2x(sar, kX86InstIdSar, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(sar, kX86InstIdSar, X86Mem, X86GpVar)
  //! Shift bits right.
  INST_2i(sar, kX86InstIdSar, X86GpVar, Imm)
  //! \overload
  INST_2i(sar, kX86InstIdSar, X86Mem, Imm)

  //! Find non `o1` BYTE starting at ES:`o0`.
  INST_2x(scasb, kX86InstIdScasB, X86GpVar, X86GpVar)
  //! Find non `o1` DWORD starting at ES:`o0`.
  INST_2x(scasd, kX86InstIdScasD, X86GpVar, X86GpVar)
  //! Find non `o1` QWORD starting at ES:`o0` (X64 Only).
  INST_2x(scasq, kX86InstIdScasQ, X86GpVar, X86GpVar)
  //! Find non `o1` WORD starting at ES:`o0`.
  INST_2x(scasw, kX86InstIdScasW, X86GpVar, X86GpVar)

  //! Set byte on condition.
  INST_1cc(set, kX86InstIdSet, X86Util::condToSetcc, X86GpVar)
  //! Set byte on condition.
  INST_1cc(set, kX86InstIdSet, X86Util::condToSetcc, X86Mem)

  //! Shift bits left.
  INST_2x(shl, kX86InstIdShl, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(shl, kX86InstIdShl, X86Mem, X86GpVar)
  //! Shift bits left.
  INST_2i(shl, kX86InstIdShl, X86GpVar, Imm)
  //! \overload
  INST_2i(shl, kX86InstIdShl, X86Mem, Imm)

  //! Shift bits right.
  INST_2x(shr, kX86InstIdShr, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(shr, kX86InstIdShr, X86Mem, X86GpVar)
  //! Shift bits right.
  INST_2i(shr, kX86InstIdShr, X86GpVar, Imm)
  //! \overload
  INST_2i(shr, kX86InstIdShr, X86Mem, Imm)

  //! Double precision shift left.
  INST_3x(shld, kX86InstIdShld, X86GpVar, X86GpVar, X86GpVar)
  //! \overload
  INST_3x(shld, kX86InstIdShld, X86Mem, X86GpVar, X86GpVar)
  //! Double precision shift left.
  INST_3i(shld, kX86InstIdShld, X86GpVar, X86GpVar, Imm)
  //! \overload
  INST_3i(shld, kX86InstIdShld, X86Mem, X86GpVar, Imm)

  //! Double precision shift right.
  INST_3x(shrd, kX86InstIdShrd, X86GpVar, X86GpVar, X86GpVar)
  //! \overload
  INST_3x(shrd, kX86InstIdShrd, X86Mem, X86GpVar, X86GpVar)
  //! Double precision shift right.
  INST_3i(shrd, kX86InstIdShrd, X86GpVar, X86GpVar, Imm)
  //! \overload
  INST_3i(shrd, kX86InstIdShrd, X86Mem, X86GpVar, Imm)

  //! Set carry flag to 1.
  INST_0x(stc, kX86InstIdStc)
  //! Set direction flag to 1.
  INST_0x(std, kX86InstIdStd)

  //! Fill BYTE at ES:`o0` with `o1`.
  INST_2x(stosb, kX86InstIdStosB, X86GpVar, X86GpVar)
  //! Fill DWORD at ES:`o0` with `o1`.
  INST_2x(stosd, kX86InstIdStosD, X86GpVar, X86GpVar)
  //! Fill QWORD at ES:`o0` with `o1` (X64 Only).
  INST_2x(stosq, kX86InstIdStosQ, X86GpVar, X86GpVar)
  //! Fill WORD at ES:`o0` with `o1`.
  INST_2x(stosw, kX86InstIdStosW, X86GpVar, X86GpVar)

  //! Subtract.
  INST_2x(sub, kX86InstIdSub, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(sub, kX86InstIdSub, X86GpVar, X86Mem)
  //! \overload
  INST_2i(sub, kX86InstIdSub, X86GpVar, Imm)
  //! \overload
  INST_2x(sub, kX86InstIdSub, X86Mem, X86GpVar)
  //! \overload
  INST_2i(sub, kX86InstIdSub, X86Mem, Imm)

  //! Logical compare.
  INST_2x(test, kX86InstIdTest, X86GpVar, X86GpVar)
  //! \overload
  INST_2i(test, kX86InstIdTest, X86GpVar, Imm)
  //! \overload
  INST_2x(test, kX86InstIdTest, X86Mem, X86GpVar)
  //! \overload
  INST_2i(test, kX86InstIdTest, X86Mem, Imm)

  //! Undefined instruction - Raise #UD exception.
  INST_0x(ud2, kX86InstIdUd2)

  //! Exchange and add.
  INST_2x(xadd, kX86InstIdXadd, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(xadd, kX86InstIdXadd, X86Mem, X86GpVar)

  //! Exchange register/memory with register.
  INST_2x(xchg, kX86InstIdXchg, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(xchg, kX86InstIdXchg, X86Mem, X86GpVar)
  //! \overload
  INST_2x(xchg, kX86InstIdXchg, X86GpVar, X86Mem)

  //! Xor.
  INST_2x(xor_, kX86InstIdXor, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(xor_, kX86InstIdXor, X86GpVar, X86Mem)
  //! \overload
  INST_2i(xor_, kX86InstIdXor, X86GpVar, Imm)
  //! \overload
  INST_2x(xor_, kX86InstIdXor, X86Mem, X86GpVar)
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
  INST_1x(fnstsw, kX86InstIdFnstsw, X86GpVar)
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
  INST_1x(fstsw, kX86InstIdFstsw, X86GpVar)
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
  INST_2x(movd, kX86InstIdMovd, X86Mem, X86MmVar)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86GpVar, X86MmVar)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86MmVar, X86Mem)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86MmVar, X86GpVar)

  //! Move QWORD (MMX).
  INST_2x(movq, kX86InstIdMovq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86Mem, X86MmVar)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86MmVar, X86Mem)

  //! Move QWORD (X64 Only).
  INST_2x(movq, kX86InstIdMovq, X86GpVar, X86MmVar)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86MmVar, X86GpVar)

  //! Pack DWORDs to WORDs with signed saturation (MMX).
  INST_2x(packssdw, kX86InstIdPackssdw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(packssdw, kX86InstIdPackssdw, X86MmVar, X86Mem)

  //! Pack WORDs to BYTEs with signed saturation (MMX).
  INST_2x(packsswb, kX86InstIdPacksswb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(packsswb, kX86InstIdPacksswb, X86MmVar, X86Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (MMX).
  INST_2x(packuswb, kX86InstIdPackuswb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(packuswb, kX86InstIdPackuswb, X86MmVar, X86Mem)

  //! Packed BYTE add (MMX).
  INST_2x(paddb, kX86InstIdPaddb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddb, kX86InstIdPaddb, X86MmVar, X86Mem)

  //! Packed DWORD add (MMX).
  INST_2x(paddd, kX86InstIdPaddd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddd, kX86InstIdPaddd, X86MmVar, X86Mem)

  //! Packed BYTE add with saturation (MMX).
  INST_2x(paddsb, kX86InstIdPaddsb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddsb, kX86InstIdPaddsb, X86MmVar, X86Mem)

  //! Packed WORD add with saturation (MMX).
  INST_2x(paddsw, kX86InstIdPaddsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddsw, kX86InstIdPaddsw, X86MmVar, X86Mem)

  //! Packed BYTE add with unsigned saturation (MMX).
  INST_2x(paddusb, kX86InstIdPaddusb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddusb, kX86InstIdPaddusb, X86MmVar, X86Mem)

  //! Packed WORD add with unsigned saturation (MMX).
  INST_2x(paddusw, kX86InstIdPaddusw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddusw, kX86InstIdPaddusw, X86MmVar, X86Mem)

  //! Packed WORD add (MMX).
  INST_2x(paddw, kX86InstIdPaddw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddw, kX86InstIdPaddw, X86MmVar, X86Mem)

  //! Packed and (MMX).
  INST_2x(pand, kX86InstIdPand, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pand, kX86InstIdPand, X86MmVar, X86Mem)

  //! Packed and-not (MMX).
  INST_2x(pandn, kX86InstIdPandn, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pandn, kX86InstIdPandn, X86MmVar, X86Mem)

  //! Packed BYTEs compare for equality (MMX).
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86MmVar, X86Mem)

  //! Packed DWORDs compare for equality (MMX).
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86MmVar, X86Mem)

  //! Packed WORDs compare for equality (MMX).
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86MmVar, X86Mem)

  //! Packed BYTEs compare if greater than (MMX).
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86MmVar, X86Mem)

  //! Packed DWORDs compare if greater than (MMX).
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86MmVar, X86Mem)

  //! Packed WORDs compare if greater than (MMX).
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86MmVar, X86Mem)

  //! Packed WORD multiply high (MMX).
  INST_2x(pmulhw, kX86InstIdPmulhw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmulhw, kX86InstIdPmulhw, X86MmVar, X86Mem)

  //! Packed WORD multiply low (MMX).
  INST_2x(pmullw, kX86InstIdPmullw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmullw, kX86InstIdPmullw, X86MmVar, X86Mem)

  //! Packed bitwise or (MMX).
  INST_2x(por, kX86InstIdPor, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(por, kX86InstIdPor, X86MmVar, X86Mem)

  //! Packed WORD multiply and add to packed DWORD (MMX).
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86MmVar, X86Mem)

  //! Packed DWORD shift left logical (MMX).
  INST_2x(pslld, kX86InstIdPslld, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pslld, kX86InstIdPslld, X86MmVar, X86Mem)
  //! \overload
  INST_2i(pslld, kX86InstIdPslld, X86MmVar, Imm)

  //! Packed QWORD shift left logical (MMX).
  INST_2x(psllq, kX86InstIdPsllq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psllq, kX86InstIdPsllq, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psllq, kX86InstIdPsllq, X86MmVar, Imm)

  //! Packed WORD shift left logical (MMX).
  INST_2x(psllw, kX86InstIdPsllw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psllw, kX86InstIdPsllw, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psllw, kX86InstIdPsllw, X86MmVar, Imm)

  //! Packed DWORD shift right arithmetic (MMX).
  INST_2x(psrad, kX86InstIdPsrad, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psrad, kX86InstIdPsrad, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psrad, kX86InstIdPsrad, X86MmVar, Imm)

  //! Packed WORD shift right arithmetic (MMX).
  INST_2x(psraw, kX86InstIdPsraw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psraw, kX86InstIdPsraw, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psraw, kX86InstIdPsraw, X86MmVar, Imm)

  //! Packed DWORD shift right logical (MMX).
  INST_2x(psrld, kX86InstIdPsrld, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psrld, kX86InstIdPsrld, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psrld, kX86InstIdPsrld, X86MmVar, Imm)

  //! Packed QWORD shift right logical (MMX).
  INST_2x(psrlq, kX86InstIdPsrlq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psrlq, kX86InstIdPsrlq, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psrlq, kX86InstIdPsrlq, X86MmVar, Imm)

  //! Packed WORD shift right logical (MMX).
  INST_2x(psrlw, kX86InstIdPsrlw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psrlw, kX86InstIdPsrlw, X86MmVar, X86Mem)
  //! \overload
  INST_2i(psrlw, kX86InstIdPsrlw, X86MmVar, Imm)

  //! Packed BYTE subtract (MMX).
  INST_2x(psubb, kX86InstIdPsubb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubb, kX86InstIdPsubb, X86MmVar, X86Mem)

  //! Packed DWORD subtract (MMX).
  INST_2x(psubd, kX86InstIdPsubd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubd, kX86InstIdPsubd, X86MmVar, X86Mem)

  //! Packed BYTE subtract with saturation (MMX).
  INST_2x(psubsb, kX86InstIdPsubsb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubsb, kX86InstIdPsubsb, X86MmVar, X86Mem)

  //! Packed WORD subtract with saturation (MMX).
  INST_2x(psubsw, kX86InstIdPsubsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubsw, kX86InstIdPsubsw, X86MmVar, X86Mem)

  //! Packed BYTE subtract with unsigned saturation (MMX).
  INST_2x(psubusb, kX86InstIdPsubusb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubusb, kX86InstIdPsubusb, X86MmVar, X86Mem)

  //! Packed WORD subtract with unsigned saturation (MMX).
  INST_2x(psubusw, kX86InstIdPsubusw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubusw, kX86InstIdPsubusw, X86MmVar, X86Mem)

  //! Packed WORD subtract (MMX).
  INST_2x(psubw, kX86InstIdPsubw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubw, kX86InstIdPsubw, X86MmVar, X86Mem)

  //! Unpack high packed BYTEs to WORDs (MMX).
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86MmVar, X86Mem)

  //! Unpack high packed DWORDs to QWORDs (MMX).
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86MmVar, X86Mem)

  //! Unpack high packed WORDs to DWORDs (MMX).
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86MmVar, X86Mem)

  //! Unpack low packed BYTEs to WORDs (MMX).
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86MmVar, X86Mem)

  //! Unpack low packed DWORDs to QWORDs (MMX).
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86MmVar, X86Mem)

  //! Unpack low packed WORDs to DWORDs (MMX).
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86MmVar, X86Mem)

  //! Packed bitwise xor (MMX).
  INST_2x(pxor, kX86InstIdPxor, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pxor, kX86InstIdPxor, X86MmVar, X86Mem)

  //! Empty MMX state.
  INST_0x(emms, kX86InstIdEmms)

  // --------------------------------------------------------------------------
  // [3DNOW]
  // --------------------------------------------------------------------------

  //! Packed SP-FP to DWORD convert (3dNow!).
  INST_2x(pf2id, kX86InstIdPf2id, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pf2id, kX86InstIdPf2id, X86MmVar, X86Mem)

  //!  Packed SP-FP to WORD convert (3dNow!).
  INST_2x(pf2iw, kX86InstIdPf2iw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pf2iw, kX86InstIdPf2iw, X86MmVar, X86Mem)

  //! Packed SP-FP accumulate (3dNow!).
  INST_2x(pfacc, kX86InstIdPfacc, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfacc, kX86InstIdPfacc, X86MmVar, X86Mem)

  //! Packed SP-FP addition (3dNow!).
  INST_2x(pfadd, kX86InstIdPfadd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfadd, kX86InstIdPfadd, X86MmVar, X86Mem)

  //! Packed SP-FP compare - dst == src (3dNow!).
  INST_2x(pfcmpeq, kX86InstIdPfcmpeq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfcmpeq, kX86InstIdPfcmpeq, X86MmVar, X86Mem)

  //! Packed SP-FP compare - dst >= src (3dNow!).
  INST_2x(pfcmpge, kX86InstIdPfcmpge, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfcmpge, kX86InstIdPfcmpge, X86MmVar, X86Mem)

  //! Packed SP-FP compare - dst > src (3dNow!).
  INST_2x(pfcmpgt, kX86InstIdPfcmpgt, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfcmpgt, kX86InstIdPfcmpgt, X86MmVar, X86Mem)

  //! Packed SP-FP maximum (3dNow!).
  INST_2x(pfmax, kX86InstIdPfmax, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfmax, kX86InstIdPfmax, X86MmVar, X86Mem)

  //! Packed SP-FP minimum (3dNow!).
  INST_2x(pfmin, kX86InstIdPfmin, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfmin, kX86InstIdPfmin, X86MmVar, X86Mem)

  //! Packed SP-FP multiply (3dNow!).
  INST_2x(pfmul, kX86InstIdPfmul, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfmul, kX86InstIdPfmul, X86MmVar, X86Mem)

  //! Packed SP-FP negative accumulate (3dNow!).
  INST_2x(pfnacc, kX86InstIdPfnacc, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfnacc, kX86InstIdPfnacc, X86MmVar, X86Mem)

  //! Packed SP-FP mixed accumulate (3dNow!).
  INST_2x(pfpnacc, kX86InstIdPfpnacc, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfpnacc, kX86InstIdPfpnacc, X86MmVar, X86Mem)

  //! Packed SP-FP reciprocal approximation (3dNow!).
  INST_2x(pfrcp, kX86InstIdPfrcp, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfrcp, kX86InstIdPfrcp, X86MmVar, X86Mem)

  //! Packed SP-FP reciprocal, first iteration step (3dNow!).
  INST_2x(pfrcpit1, kX86InstIdPfrcpit1, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfrcpit1, kX86InstIdPfrcpit1, X86MmVar, X86Mem)

  //! Packed SP-FP reciprocal, second iteration step (3dNow!).
  INST_2x(pfrcpit2, kX86InstIdPfrcpit2, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfrcpit2, kX86InstIdPfrcpit2, X86MmVar, X86Mem)

  //! Packed SP-FP reciprocal square root, first iteration step (3dNow!).
  INST_2x(pfrsqit1, kX86InstIdPfrsqit1, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfrsqit1, kX86InstIdPfrsqit1, X86MmVar, X86Mem)

  //! Packed SP-FP reciprocal square root approximation (3dNow!).
  INST_2x(pfrsqrt, kX86InstIdPfrsqrt, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfrsqrt, kX86InstIdPfrsqrt, X86MmVar, X86Mem)

  //! Packed SP-FP subtract (3dNow!).
  INST_2x(pfsub, kX86InstIdPfsub, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfsub, kX86InstIdPfsub, X86MmVar, X86Mem)

  //! Packed SP-FP reverse subtract (3dNow!).
  INST_2x(pfsubr, kX86InstIdPfsubr, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pfsubr, kX86InstIdPfsubr, X86MmVar, X86Mem)

  //! Packed DWORDs to SP-FP (3dNow!).
  INST_2x(pi2fd, kX86InstIdPi2fd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pi2fd, kX86InstIdPi2fd, X86MmVar, X86Mem)

  //! Packed WORDs to SP-FP (3dNow!).
  INST_2x(pi2fw, kX86InstIdPi2fw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pi2fw, kX86InstIdPi2fw, X86MmVar, X86Mem)

  //! Packed swap DWORDs (3dNow!)
  INST_2x(pswapd, kX86InstIdPswapd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pswapd, kX86InstIdPswapd, X86MmVar, X86Mem)

  //! Prefetch (3dNow!).
  INST_1x(prefetch_3dnow, kX86InstIdPrefetch3dNow, X86Mem)

  //! Prefetch and set cache to modified (3dNow!).
  INST_1x(prefetchw_3dnow, kX86InstIdPrefetchw3dNow, X86Mem)

  //! Faster EMMS (3dNow!).
  INST_0x(femms, kX86InstIdFemms)

  // --------------------------------------------------------------------------
  // [SSE]
  // --------------------------------------------------------------------------

  //! Packed SP-FP add (SSE).
  INST_2x(addps, kX86InstIdAddps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(addps, kX86InstIdAddps, X86XmmVar, X86Mem)

  //! Scalar SP-FP add (SSE).
  INST_2x(addss, kX86InstIdAddss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(addss, kX86InstIdAddss, X86XmmVar, X86Mem)

  //! Packed SP-FP bitwise and-not (SSE).
  INST_2x(andnps, kX86InstIdAndnps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(andnps, kX86InstIdAndnps, X86XmmVar, X86Mem)

  //! Packed SP-FP bitwise and (SSE).
  INST_2x(andps, kX86InstIdAndps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(andps, kX86InstIdAndps, X86XmmVar, X86Mem)

  //! Packed SP-FP compare (SSE).
  INST_3i(cmpps, kX86InstIdCmpps, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(cmpps, kX86InstIdCmpps, X86XmmVar, X86Mem, Imm)

  //! Compare scalar SP-FP Values (SSE).
  INST_3i(cmpss, kX86InstIdCmpss, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(cmpss, kX86InstIdCmpss, X86XmmVar, X86Mem, Imm)

  //! Scalar ordered SP-FP compare and set EFLAGS (SSE).
  INST_2x(comiss, kX86InstIdComiss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(comiss, kX86InstIdComiss, X86XmmVar, X86Mem)

  //! Packed signed INT32 to packed SP-FP conversion (SSE).
  INST_2x(cvtpi2ps, kX86InstIdCvtpi2ps, X86XmmVar, X86MmVar)
  //! \overload
  INST_2x(cvtpi2ps, kX86InstIdCvtpi2ps, X86XmmVar, X86Mem)

  //! Packed SP-FP to packed INT32 conversion (SSE).
  INST_2x(cvtps2pi, kX86InstIdCvtps2pi, X86MmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtps2pi, kX86InstIdCvtps2pi, X86MmVar, X86Mem)

  //! Convert scalar INT32 to SP-FP (SSE).
  INST_2x(cvtsi2ss, kX86InstIdCvtsi2ss, X86XmmVar, X86GpVar)
  //! \overload
  INST_2x(cvtsi2ss, kX86InstIdCvtsi2ss, X86XmmVar, X86Mem)

  //! Convert scalar SP-FP to INT32 (SSE).
  INST_2x(cvtss2si, kX86InstIdCvtss2si, X86GpVar, X86XmmVar)
  //! \overload
  INST_2x(cvtss2si, kX86InstIdCvtss2si, X86GpVar, X86Mem)

  //! Convert with truncation packed SP-FP to packed INT32 (SSE).
  INST_2x(cvttps2pi, kX86InstIdCvttps2pi, X86MmVar, X86XmmVar)
  //! \overload
  INST_2x(cvttps2pi, kX86InstIdCvttps2pi, X86MmVar, X86Mem)

  //! Convert with truncation scalar SP-FP to INT32 (SSE).
  INST_2x(cvttss2si, kX86InstIdCvttss2si, X86GpVar, X86XmmVar)
  //! \overload
  INST_2x(cvttss2si, kX86InstIdCvttss2si, X86GpVar, X86Mem)

  //! Packed SP-FP divide (SSE).
  INST_2x(divps, kX86InstIdDivps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(divps, kX86InstIdDivps, X86XmmVar, X86Mem)

  //! Scalar SP-FP divide (SSE).
  INST_2x(divss, kX86InstIdDivss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(divss, kX86InstIdDivss, X86XmmVar, X86Mem)

  //! Load streaming SIMD extension control/status (SSE).
  INST_1x(ldmxcsr, kX86InstIdLdmxcsr, X86Mem)

  //! Byte mask write (SSE).
  INST_3x(maskmovq, kX86InstIdMaskmovq, X86GpVar /* zdi */, X86MmVar, X86MmVar)

  //! Packed SP-FP maximum (SSE).
  INST_2x(maxps, kX86InstIdMaxps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(maxps, kX86InstIdMaxps, X86XmmVar, X86Mem)

  //! Scalar SP-FP maximum (SSE).
  INST_2x(maxss, kX86InstIdMaxss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(maxss, kX86InstIdMaxss, X86XmmVar, X86Mem)

  //! Packed SP-FP minimum (SSE).
  INST_2x(minps, kX86InstIdMinps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(minps, kX86InstIdMinps, X86XmmVar, X86Mem)

  //! Scalar SP-FP minimum (SSE).
  INST_2x(minss, kX86InstIdMinss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(minss, kX86InstIdMinss, X86XmmVar, X86Mem)

  //! Move aligned packed SP-FP (SSE).
  INST_2x(movaps, kX86InstIdMovaps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movaps, kX86InstIdMovaps, X86XmmVar, X86Mem)
  //! Move aligned packed SP-FP (SSE).
  INST_2x(movaps, kX86InstIdMovaps, X86Mem, X86XmmVar)

  //! Move DWORD.
  INST_2x(movd, kX86InstIdMovd, X86Mem, X86XmmVar)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86GpVar, X86XmmVar)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movd, kX86InstIdMovd, X86XmmVar, X86GpVar)

  //! Move QWORD (SSE).
  INST_2x(movq, kX86InstIdMovq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86Mem, X86XmmVar)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86XmmVar, X86Mem)

  //! Move QWORD (X64 Only).
  INST_2x(movq, kX86InstIdMovq, X86GpVar, X86XmmVar)
  //! \overload
  INST_2x(movq, kX86InstIdMovq, X86XmmVar, X86GpVar)

  //! Move QWORD using NT hint (SSE).
  INST_2x(movntq, kX86InstIdMovntq, X86Mem, X86MmVar)

  //! Move high to low packed SP-FP (SSE).
  INST_2x(movhlps, kX86InstIdMovhlps, X86XmmVar, X86XmmVar)

  //! Move high packed SP-FP (SSE).
  INST_2x(movhps, kX86InstIdMovhps, X86XmmVar, X86Mem)
  //! Move high packed SP-FP (SSE).
  INST_2x(movhps, kX86InstIdMovhps, X86Mem, X86XmmVar)

  //! Move low to high packed SP-FP (SSE).
  INST_2x(movlhps, kX86InstIdMovlhps, X86XmmVar, X86XmmVar)

  //! Move low packed SP-FP (SSE).
  INST_2x(movlps, kX86InstIdMovlps, X86XmmVar, X86Mem)
  //! Move low packed SP-FP (SSE).
  INST_2x(movlps, kX86InstIdMovlps, X86Mem, X86XmmVar)

  //! Move aligned packed SP-FP using NT hint (SSE).
  INST_2x(movntps, kX86InstIdMovntps, X86Mem, X86XmmVar)

  //! Move scalar SP-FP (SSE).
  INST_2x(movss, kX86InstIdMovss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movss, kX86InstIdMovss, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movss, kX86InstIdMovss, X86Mem, X86XmmVar)

  //! Move unaligned packed SP-FP (SSE).
  INST_2x(movups, kX86InstIdMovups, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movups, kX86InstIdMovups, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movups, kX86InstIdMovups, X86Mem, X86XmmVar)

  //! Packed SP-FP multiply (SSE).
  INST_2x(mulps, kX86InstIdMulps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(mulps, kX86InstIdMulps, X86XmmVar, X86Mem)

  //! Scalar SP-FP multiply (SSE).
  INST_2x(mulss, kX86InstIdMulss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(mulss, kX86InstIdMulss, X86XmmVar, X86Mem)

  //! Packed SP-FP bitwise or (SSE).
  INST_2x(orps, kX86InstIdOrps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(orps, kX86InstIdOrps, X86XmmVar, X86Mem)

  //! Packed BYTE average (SSE).
  INST_2x(pavgb, kX86InstIdPavgb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pavgb, kX86InstIdPavgb, X86MmVar, X86Mem)

  //! Packed WORD average (SSE).
  INST_2x(pavgw, kX86InstIdPavgw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pavgw, kX86InstIdPavgw, X86MmVar, X86Mem)

  //! Extract WORD based on selector (SSE).
  INST_3i(pextrw, kX86InstIdPextrw, X86GpVar, X86MmVar, Imm)

  //! Insert WORD based on selector (SSE).
  INST_3i(pinsrw, kX86InstIdPinsrw, X86MmVar, X86GpVar, Imm)
  //! \overload
  INST_3i(pinsrw, kX86InstIdPinsrw, X86MmVar, X86Mem, Imm)

  //! Packed WORD maximum (SSE).
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86MmVar, X86Mem)

  //! Packed BYTE unsigned maximum (SSE).
  INST_2x(pmaxub, kX86InstIdPmaxub, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmaxub, kX86InstIdPmaxub, X86MmVar, X86Mem)

  //! Packed WORD minimum (SSE).
  INST_2x(pminsw, kX86InstIdPminsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pminsw, kX86InstIdPminsw, X86MmVar, X86Mem)

  //! Packed BYTE unsigned minimum (SSE).
  INST_2x(pminub, kX86InstIdPminub, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pminub, kX86InstIdPminub, X86MmVar, X86Mem)

  //! Move byte mask to integer (SSE).
  INST_2x(pmovmskb, kX86InstIdPmovmskb, X86GpVar, X86MmVar)

  //! Packed WORD unsigned multiply high (SSE).
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86MmVar, X86Mem)

  //! Packed WORD sum of absolute differences (SSE).
  INST_2x(psadbw, kX86InstIdPsadbw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psadbw, kX86InstIdPsadbw, X86MmVar, X86Mem)

  //! Packed WORD shuffle (SSE).
  INST_3i(pshufw, kX86InstIdPshufw, X86MmVar, X86MmVar, Imm)
  //! \overload
  INST_3i(pshufw, kX86InstIdPshufw, X86MmVar, X86Mem, Imm)

  //! Packed SP-FP reciprocal (SSE).
  INST_2x(rcpps, kX86InstIdRcpps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(rcpps, kX86InstIdRcpps, X86XmmVar, X86Mem)

  //! Scalar SP-FP reciprocal (SSE).
  INST_2x(rcpss, kX86InstIdRcpss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(rcpss, kX86InstIdRcpss, X86XmmVar, X86Mem)

  //! Prefetch (SSE).
  INST_2i(prefetch, kX86InstIdPrefetch, X86Mem, Imm)

  //! Packed WORD sum of absolute differences (SSE).
  INST_2x(psadbw, kX86InstIdPsadbw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psadbw, kX86InstIdPsadbw, X86XmmVar, X86Mem)

  //! Packed SP-FP Square root reciprocal (SSE).
  INST_2x(rsqrtps, kX86InstIdRsqrtps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(rsqrtps, kX86InstIdRsqrtps, X86XmmVar, X86Mem)

  //! Scalar SP-FP Square root reciprocal (SSE).
  INST_2x(rsqrtss, kX86InstIdRsqrtss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(rsqrtss, kX86InstIdRsqrtss, X86XmmVar, X86Mem)

  //! Store fence (SSE).
  INST_0x(sfence, kX86InstIdSfence)

  //! Shuffle SP-FP (SSE).
  INST_3i(shufps, kX86InstIdShufps, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(shufps, kX86InstIdShufps, X86XmmVar, X86Mem, Imm)

  //! Packed SP-FP square root (SSE).
  INST_2x(sqrtps, kX86InstIdSqrtps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(sqrtps, kX86InstIdSqrtps, X86XmmVar, X86Mem)

  //! Scalar SP-FP square root (SSE).
  INST_2x(sqrtss, kX86InstIdSqrtss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(sqrtss, kX86InstIdSqrtss, X86XmmVar, X86Mem)

  //! Store streaming SIMD extension control/status (SSE).
  INST_1x(stmxcsr, kX86InstIdStmxcsr, X86Mem)

  //! Packed SP-FP subtract (SSE).
  INST_2x(subps, kX86InstIdSubps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(subps, kX86InstIdSubps, X86XmmVar, X86Mem)

  //! Scalar SP-FP subtract (SSE).
  INST_2x(subss, kX86InstIdSubss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(subss, kX86InstIdSubss, X86XmmVar, X86Mem)

  //! Unordered scalar SP-FP compare and set EFLAGS (SSE).
  INST_2x(ucomiss, kX86InstIdUcomiss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(ucomiss, kX86InstIdUcomiss, X86XmmVar, X86Mem)

  //! Unpack high packed SP-FP data (SSE).
  INST_2x(unpckhps, kX86InstIdUnpckhps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(unpckhps, kX86InstIdUnpckhps, X86XmmVar, X86Mem)

  //! Unpack low packed SP-FP data (SSE).
  INST_2x(unpcklps, kX86InstIdUnpcklps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(unpcklps, kX86InstIdUnpcklps, X86XmmVar, X86Mem)

  //! Packed SP-FP bitwise xor (SSE).
  INST_2x(xorps, kX86InstIdXorps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(xorps, kX86InstIdXorps, X86XmmVar, X86Mem)

  // --------------------------------------------------------------------------
  // [SSE2]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add (SSE2).
  INST_2x(addpd, kX86InstIdAddpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(addpd, kX86InstIdAddpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP add (SSE2).
  INST_2x(addsd, kX86InstIdAddsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(addsd, kX86InstIdAddsd, X86XmmVar, X86Mem)

  //! Packed DP-FP bitwise and-not (SSE2).
  INST_2x(andnpd, kX86InstIdAndnpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(andnpd, kX86InstIdAndnpd, X86XmmVar, X86Mem)

  //! Packed DP-FP bitwise and (SSE2).
  INST_2x(andpd, kX86InstIdAndpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(andpd, kX86InstIdAndpd, X86XmmVar, X86Mem)

  //! Flush cache line (SSE2).
  INST_1x(clflush, kX86InstIdClflush, X86Mem)

  //! Packed DP-FP compare (SSE2).
  INST_3i(cmppd, kX86InstIdCmppd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(cmppd, kX86InstIdCmppd, X86XmmVar, X86Mem, Imm)

  //! Scalar SP-FP compare (SSE2).
  INST_3i(cmpsd, kX86InstIdCmpsd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(cmpsd, kX86InstIdCmpsd, X86XmmVar, X86Mem, Imm)

  //! Scalar ordered DP-FP compare and set EFLAGS (SSE2).
  INST_2x(comisd, kX86InstIdComisd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(comisd, kX86InstIdComisd, X86XmmVar, X86Mem)

  //! Convert packed DWORD integers to packed DP-FP (SSE2).
  INST_2x(cvtdq2pd, kX86InstIdCvtdq2pd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtdq2pd, kX86InstIdCvtdq2pd, X86XmmVar, X86Mem)

  //! Convert packed DWORD integers to packed SP-FP (SSE2).
  INST_2x(cvtdq2ps, kX86InstIdCvtdq2ps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtdq2ps, kX86InstIdCvtdq2ps, X86XmmVar, X86Mem)

  //! Convert packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvtpd2dq, kX86InstIdCvtpd2dq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtpd2dq, kX86InstIdCvtpd2dq, X86XmmVar, X86Mem)

  //! Convert packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvtpd2pi, kX86InstIdCvtpd2pi, X86MmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtpd2pi, kX86InstIdCvtpd2pi, X86MmVar, X86Mem)

  //! Convert packed DP-FP to packed SP-FP (SSE2).
  INST_2x(cvtpd2ps, kX86InstIdCvtpd2ps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtpd2ps, kX86InstIdCvtpd2ps, X86XmmVar, X86Mem)

  //! Convert packed DWORDs to packed DP-FP (SSE2).
  INST_2x(cvtpi2pd, kX86InstIdCvtpi2pd, X86XmmVar, X86MmVar)
  //! \overload
  INST_2x(cvtpi2pd, kX86InstIdCvtpi2pd, X86XmmVar, X86Mem)

  //! Convert packed SP-FP to packed DWORDs (SSE2).
  INST_2x(cvtps2dq, kX86InstIdCvtps2dq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtps2dq, kX86InstIdCvtps2dq, X86XmmVar, X86Mem)

  //! Convert packed SP-FP to packed DP-FP (SSE2).
  INST_2x(cvtps2pd, kX86InstIdCvtps2pd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtps2pd, kX86InstIdCvtps2pd, X86XmmVar, X86Mem)

  //! Convert scalar DP-FP to DWORD (SSE2).
  INST_2x(cvtsd2si, kX86InstIdCvtsd2si, X86GpVar, X86XmmVar)
  //! \overload
  INST_2x(cvtsd2si, kX86InstIdCvtsd2si, X86GpVar, X86Mem)

  //! Convert scalar DP-FP to scalar SP-FP (SSE2).
  INST_2x(cvtsd2ss, kX86InstIdCvtsd2ss, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtsd2ss, kX86InstIdCvtsd2ss, X86XmmVar, X86Mem)

  //! Convert DWORD to scalar DP-FP (SSE2).
  INST_2x(cvtsi2sd, kX86InstIdCvtsi2sd, X86XmmVar, X86GpVar)
  //! \overload
  INST_2x(cvtsi2sd, kX86InstIdCvtsi2sd, X86XmmVar, X86Mem)

  //! Convert scalar SP-FP to DP-FP (SSE2).
  INST_2x(cvtss2sd, kX86InstIdCvtss2sd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvtss2sd, kX86InstIdCvtss2sd, X86XmmVar, X86Mem)

  //! Convert with truncation packed DP-FP to packed DWORDs (SSE2).
  INST_2x(cvttpd2pi, kX86InstIdCvttpd2pi, X86MmVar, X86XmmVar)
  //! \overload
  INST_2x(cvttpd2pi, kX86InstIdCvttpd2pi, X86MmVar, X86Mem)

  //! Convert with truncation packed DP-FP to packed QWORDs (SSE2).
  INST_2x(cvttpd2dq, kX86InstIdCvttpd2dq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvttpd2dq, kX86InstIdCvttpd2dq, X86XmmVar, X86Mem)

  //! Convert with truncation packed SP-FP to packed QWORDs (SSE2).
  INST_2x(cvttps2dq, kX86InstIdCvttps2dq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(cvttps2dq, kX86InstIdCvttps2dq, X86XmmVar, X86Mem)

  //! Convert with truncation scalar DP-FP to DWORD (SSE2).
  INST_2x(cvttsd2si, kX86InstIdCvttsd2si, X86GpVar, X86XmmVar)
  //! \overload
  INST_2x(cvttsd2si, kX86InstIdCvttsd2si, X86GpVar, X86Mem)

  //! Packed DP-FP divide (SSE2).
  INST_2x(divpd, kX86InstIdDivpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(divpd, kX86InstIdDivpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP divide (SSE2).
  INST_2x(divsd, kX86InstIdDivsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(divsd, kX86InstIdDivsd, X86XmmVar, X86Mem)

  //! Load fence (SSE2).
  INST_0x(lfence, kX86InstIdLfence)

  //! Store selected bytes of OWORD (SSE2).
  INST_3x(maskmovdqu, kX86InstIdMaskmovdqu, X86GpVar /* zdi */, X86XmmVar, X86XmmVar)

  //! Packed DP-FP maximum (SSE2).
  INST_2x(maxpd, kX86InstIdMaxpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(maxpd, kX86InstIdMaxpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP maximum (SSE2).
  INST_2x(maxsd, kX86InstIdMaxsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(maxsd, kX86InstIdMaxsd, X86XmmVar, X86Mem)

  //! Memory fence (SSE2).
  INST_0x(mfence, kX86InstIdMfence)

  //! Packed DP-FP minimum (SSE2).
  INST_2x(minpd, kX86InstIdMinpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(minpd, kX86InstIdMinpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP minimum (SSE2).
  INST_2x(minsd, kX86InstIdMinsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(minsd, kX86InstIdMinsd, X86XmmVar, X86Mem)

  //! Move aligned OWORD (SSE2).
  INST_2x(movdqa, kX86InstIdMovdqa, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movdqa, kX86InstIdMovdqa, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movdqa, kX86InstIdMovdqa, X86Mem, X86XmmVar)

  //! Move unaligned OWORD (SSE2).
  INST_2x(movdqu, kX86InstIdMovdqu, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movdqu, kX86InstIdMovdqu, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movdqu, kX86InstIdMovdqu, X86Mem, X86XmmVar)

  //! Extract packed SP-FP sign mask (SSE2).
  INST_2x(movmskps, kX86InstIdMovmskps, X86GpVar, X86XmmVar)

  //! Extract packed DP-FP sign mask (SSE2).
  INST_2x(movmskpd, kX86InstIdMovmskpd, X86GpVar, X86XmmVar)

  //! Move scalar DP-FP (SSE2).
  INST_2x(movsd, kX86InstIdMovsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movsd, kX86InstIdMovsd, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movsd, kX86InstIdMovsd, X86Mem, X86XmmVar)

  //! Move aligned packed DP-FP (SSE2).
  INST_2x(movapd, kX86InstIdMovapd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movapd, kX86InstIdMovapd, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movapd, kX86InstIdMovapd, X86Mem, X86XmmVar)

  //! Move QWORD from Xmm to Mm register (SSE2).
  INST_2x(movdq2q, kX86InstIdMovdq2q, X86MmVar, X86XmmVar)

  //! Move QWORD from Mm to Xmm register (SSE2).
  INST_2x(movq2dq, kX86InstIdMovq2dq, X86XmmVar, X86MmVar)

  //! Move high packed DP-FP (SSE2).
  INST_2x(movhpd, kX86InstIdMovhpd, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movhpd, kX86InstIdMovhpd, X86Mem, X86XmmVar)

  //! Move low packed DP-FP (SSE2).
  INST_2x(movlpd, kX86InstIdMovlpd, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movlpd, kX86InstIdMovlpd, X86Mem, X86XmmVar)

  //! Store OWORD using NT hint (SSE2).
  INST_2x(movntdq, kX86InstIdMovntdq, X86Mem, X86XmmVar)

  //! Store DWORD using NT hint (SSE2).
  INST_2x(movnti, kX86InstIdMovnti, X86Mem, X86GpVar)

  //! Store packed DP-FP using NT hint (SSE2).
  INST_2x(movntpd, kX86InstIdMovntpd, X86Mem, X86XmmVar)

  //! Move unaligned packed DP-FP (SSE2).
  INST_2x(movupd, kX86InstIdMovupd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movupd, kX86InstIdMovupd, X86XmmVar, X86Mem)
  //! \overload
  INST_2x(movupd, kX86InstIdMovupd, X86Mem, X86XmmVar)

  //! Packed DP-FP multiply (SSE2).
  INST_2x(mulpd, kX86InstIdMulpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(mulpd, kX86InstIdMulpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP multiply (SSE2).
  INST_2x(mulsd, kX86InstIdMulsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(mulsd, kX86InstIdMulsd, X86XmmVar, X86Mem)

  //! Packed DP-FP bitwise or (SSE2).
  INST_2x(orpd, kX86InstIdOrpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(orpd, kX86InstIdOrpd, X86XmmVar, X86Mem)

  //! Pack WORDs to BYTEs with signed saturation (SSE2).
  INST_2x(packsswb, kX86InstIdPacksswb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(packsswb, kX86InstIdPacksswb, X86XmmVar, X86Mem)

  //! Pack DWORDs to WORDs with signed saturation (SSE2).
  INST_2x(packssdw, kX86InstIdPackssdw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(packssdw, kX86InstIdPackssdw, X86XmmVar, X86Mem)

  //! Pack WORDs to BYTEs with unsigned saturation (SSE2).
  INST_2x(packuswb, kX86InstIdPackuswb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(packuswb, kX86InstIdPackuswb, X86XmmVar, X86Mem)

  //! Packed BYTE add (SSE2).
  INST_2x(paddb, kX86InstIdPaddb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddb, kX86InstIdPaddb, X86XmmVar, X86Mem)

  //! Packed WORD add (SSE2).
  INST_2x(paddw, kX86InstIdPaddw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddw, kX86InstIdPaddw, X86XmmVar, X86Mem)

  //! Packed DWORD add (SSE2).
  INST_2x(paddd, kX86InstIdPaddd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddd, kX86InstIdPaddd, X86XmmVar, X86Mem)

  //! Packed QWORD add (SSE2).
  INST_2x(paddq, kX86InstIdPaddq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(paddq, kX86InstIdPaddq, X86MmVar, X86Mem)

  //! Packed QWORD add (SSE2).
  INST_2x(paddq, kX86InstIdPaddq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddq, kX86InstIdPaddq, X86XmmVar, X86Mem)

  //! Packed BYTE add with saturation (SSE2).
  INST_2x(paddsb, kX86InstIdPaddsb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddsb, kX86InstIdPaddsb, X86XmmVar, X86Mem)

  //! Packed WORD add with saturation (SSE2).
  INST_2x(paddsw, kX86InstIdPaddsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddsw, kX86InstIdPaddsw, X86XmmVar, X86Mem)

  //! Packed BYTE add with unsigned saturation (SSE2).
  INST_2x(paddusb, kX86InstIdPaddusb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddusb, kX86InstIdPaddusb, X86XmmVar, X86Mem)

  //! Packed WORD add with unsigned saturation (SSE2).
  INST_2x(paddusw, kX86InstIdPaddusw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(paddusw, kX86InstIdPaddusw, X86XmmVar, X86Mem)

  //! Packed bitwise and (SSE2).
  INST_2x(pand, kX86InstIdPand, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pand, kX86InstIdPand, X86XmmVar, X86Mem)

  //! Packed bitwise and-not (SSE2).
  INST_2x(pandn, kX86InstIdPandn, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pandn, kX86InstIdPandn, X86XmmVar, X86Mem)

  //! Spin loop hint (SSE2).
  INST_0x(pause, kX86InstIdPause)

  //! Packed BYTE average (SSE2).
  INST_2x(pavgb, kX86InstIdPavgb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pavgb, kX86InstIdPavgb, X86XmmVar, X86Mem)

  //! Packed WORD average (SSE2).
  INST_2x(pavgw, kX86InstIdPavgw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pavgw, kX86InstIdPavgw, X86XmmVar, X86Mem)

  //! Packed BYTE compare for equality (SSE2).
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpeqb, kX86InstIdPcmpeqb, X86XmmVar, X86Mem)

  //! Packed WROD compare for equality (SSE2).
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpeqw, kX86InstIdPcmpeqw, X86XmmVar, X86Mem)

  //! Packed DWORD compare for equality (SSE2).
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpeqd, kX86InstIdPcmpeqd, X86XmmVar, X86Mem)

  //! Packed BYTE compare if greater than (SSE2).
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpgtb, kX86InstIdPcmpgtb, X86XmmVar, X86Mem)

  //! Packed WORD compare if greater than (SSE2).
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpgtw, kX86InstIdPcmpgtw, X86XmmVar, X86Mem)

  //! Packed DWORD compare if greater than (SSE2).
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpgtd, kX86InstIdPcmpgtd, X86XmmVar, X86Mem)

  //! Extract WORD based on selector (SSE2).
  INST_3i(pextrw, kX86InstIdPextrw, X86GpVar, X86XmmVar, Imm)

  //! Insert WORD based on selector (SSE2).
  INST_3i(pinsrw, kX86InstIdPinsrw, X86XmmVar, X86GpVar, Imm)
  //! \overload
  INST_3i(pinsrw, kX86InstIdPinsrw, X86XmmVar, X86Mem, Imm)

  //! Packed WORD maximum (SSE2).
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaxsw, kX86InstIdPmaxsw, X86XmmVar, X86Mem)

  //! Packed BYTE unsigned maximum (SSE2).
  INST_2x(pmaxub, kX86InstIdPmaxub, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaxub, kX86InstIdPmaxub, X86XmmVar, X86Mem)

  //! Packed WORD minimum (SSE2).
  INST_2x(pminsw, kX86InstIdPminsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pminsw, kX86InstIdPminsw, X86XmmVar, X86Mem)

  //! Packed BYTE unsigned minimum (SSE2).
  INST_2x(pminub, kX86InstIdPminub, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pminub, kX86InstIdPminub, X86XmmVar, X86Mem)

  //! Move BYTE mask (SSE2).
  INST_2x(pmovmskb, kX86InstIdPmovmskb, X86GpVar, X86XmmVar)

  //! Packed WORD multiply high (SSE2).
  INST_2x(pmulhw, kX86InstIdPmulhw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmulhw, kX86InstIdPmulhw, X86XmmVar, X86Mem)

  //! Packed WORD unsigned multiply high (SSE2).
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmulhuw, kX86InstIdPmulhuw, X86XmmVar, X86Mem)

  //! Packed WORD multiply low (SSE2).
  INST_2x(pmullw, kX86InstIdPmullw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmullw, kX86InstIdPmullw, X86XmmVar, X86Mem)

  //! Packed DWORD multiply to QWORD (SSE2).
  INST_2x(pmuludq, kX86InstIdPmuludq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmuludq, kX86InstIdPmuludq, X86MmVar, X86Mem)

  //! Packed DWORD multiply to QWORD (SSE2).
  INST_2x(pmuludq, kX86InstIdPmuludq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmuludq, kX86InstIdPmuludq, X86XmmVar, X86Mem)

  //! Packed bitwise or (SSE2).
  INST_2x(por, kX86InstIdPor, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(por, kX86InstIdPor, X86XmmVar, X86Mem)

  //! Packed DWORD shift left logical (SSE2).
  INST_2x(pslld, kX86InstIdPslld, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pslld, kX86InstIdPslld, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(pslld, kX86InstIdPslld, X86XmmVar, Imm)

  //! Packed QWORD shift left logical (SSE2).
  INST_2x(psllq, kX86InstIdPsllq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psllq, kX86InstIdPsllq, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psllq, kX86InstIdPsllq, X86XmmVar, Imm)

  //! Packed WORD shift left logical (SSE2).
  INST_2x(psllw, kX86InstIdPsllw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psllw, kX86InstIdPsllw, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psllw, kX86InstIdPsllw, X86XmmVar, Imm)

  //! Packed OWORD shift left logical (SSE2).
  INST_2i(pslldq, kX86InstIdPslldq, X86XmmVar, Imm)

  //! Packed DWORD shift right arithmetic (SSE2).
  INST_2x(psrad, kX86InstIdPsrad, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psrad, kX86InstIdPsrad, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psrad, kX86InstIdPsrad, X86XmmVar, Imm)

  //! Packed WORD shift right arithmetic (SSE2).
  INST_2x(psraw, kX86InstIdPsraw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psraw, kX86InstIdPsraw, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psraw, kX86InstIdPsraw, X86XmmVar, Imm)

  //! Packed BYTE subtract (SSE2).
  INST_2x(psubb, kX86InstIdPsubb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubb, kX86InstIdPsubb, X86XmmVar, X86Mem)

  //! Packed DWORD subtract (SSE2).
  INST_2x(psubd, kX86InstIdPsubd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubd, kX86InstIdPsubd, X86XmmVar, X86Mem)

  //! Packed QWORD subtract (SSE2).
  INST_2x(psubq, kX86InstIdPsubq, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psubq, kX86InstIdPsubq, X86MmVar, X86Mem)

  //! Packed QWORD subtract (SSE2).
  INST_2x(psubq, kX86InstIdPsubq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubq, kX86InstIdPsubq, X86XmmVar, X86Mem)

  //! Packed WORD subtract (SSE2).
  INST_2x(psubw, kX86InstIdPsubw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubw, kX86InstIdPsubw, X86XmmVar, X86Mem)

  //! Packed WORD to DWORD multiply and add (SSE2).
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaddwd, kX86InstIdPmaddwd, X86XmmVar, X86Mem)

  //! Packed DWORD shuffle (SSE2).
  INST_3i(pshufd, kX86InstIdPshufd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pshufd, kX86InstIdPshufd, X86XmmVar, X86Mem, Imm)

  //! Packed WORD shuffle high (SSE2).
  INST_3i(pshufhw, kX86InstIdPshufhw, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pshufhw, kX86InstIdPshufhw, X86XmmVar, X86Mem, Imm)

  //! Packed WORD shuffle low (SSE2).
  INST_3i(pshuflw, kX86InstIdPshuflw, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pshuflw, kX86InstIdPshuflw, X86XmmVar, X86Mem, Imm)

  //! Packed DWORD shift right logical (SSE2).
  INST_2x(psrld, kX86InstIdPsrld, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psrld, kX86InstIdPsrld, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psrld, kX86InstIdPsrld, X86XmmVar, Imm)

  //! Packed QWORD shift right logical (SSE2).
  INST_2x(psrlq, kX86InstIdPsrlq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psrlq, kX86InstIdPsrlq, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psrlq, kX86InstIdPsrlq, X86XmmVar, Imm)

  //! Scalar OWORD shift right logical (SSE2).
  INST_2i(psrldq, kX86InstIdPsrldq, X86XmmVar, Imm)

  //! Packed WORD shift right logical (SSE2).
  INST_2x(psrlw, kX86InstIdPsrlw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psrlw, kX86InstIdPsrlw, X86XmmVar, X86Mem)
  //! \overload
  INST_2i(psrlw, kX86InstIdPsrlw, X86XmmVar, Imm)

  //! Packed BYTE subtract with saturation (SSE2).
  INST_2x(psubsb, kX86InstIdPsubsb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubsb, kX86InstIdPsubsb, X86XmmVar, X86Mem)

  //! Packed WORD subtract with saturation (SSE2).
  INST_2x(psubsw, kX86InstIdPsubsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubsw, kX86InstIdPsubsw, X86XmmVar, X86Mem)

  //! Packed BYTE subtract with unsigned saturation (SSE2).
  INST_2x(psubusb, kX86InstIdPsubusb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubusb, kX86InstIdPsubusb, X86XmmVar, X86Mem)

  //! Packed WORD subtract with unsigned saturation (SSE2).
  INST_2x(psubusw, kX86InstIdPsubusw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psubusw, kX86InstIdPsubusw, X86XmmVar, X86Mem)

  //! Unpack high packed BYTEs to WORDs (SSE2).
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpckhbw, kX86InstIdPunpckhbw, X86XmmVar, X86Mem)

  //! Unpack high packed DWORDs to QWORDs (SSE2).
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpckhdq, kX86InstIdPunpckhdq, X86XmmVar, X86Mem)

  //! Unpack high packed QWORDs to OWORD (SSE2).
  INST_2x(punpckhqdq, kX86InstIdPunpckhqdq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpckhqdq, kX86InstIdPunpckhqdq, X86XmmVar, X86Mem)

  //! Unpack high packed WORDs to DWORDs (SSE2).
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpckhwd, kX86InstIdPunpckhwd, X86XmmVar, X86Mem)

  //! Unpack low packed BYTEs to WORDs (SSE2).
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpcklbw, kX86InstIdPunpcklbw, X86XmmVar, X86Mem)

  //! Unpack low packed DWORDs to QWORDs (SSE2).
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpckldq, kX86InstIdPunpckldq, X86XmmVar, X86Mem)

  //! Unpack low packed QWORDs to OWORD (SSE2).
  INST_2x(punpcklqdq, kX86InstIdPunpcklqdq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpcklqdq, kX86InstIdPunpcklqdq, X86XmmVar, X86Mem)

  //! Unpack low packed WORDs to DWORDs (SSE2).
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(punpcklwd, kX86InstIdPunpcklwd, X86XmmVar, X86Mem)

  //! Packed bitwise xor (SSE2).
  INST_2x(pxor, kX86InstIdPxor, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pxor, kX86InstIdPxor, X86XmmVar, X86Mem)

  //! Shuffle DP-FP (SSE2).
  INST_3i(shufpd, kX86InstIdShufpd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(shufpd, kX86InstIdShufpd, X86XmmVar, X86Mem, Imm)

  //! Packed DP-FP square root (SSE2).
  INST_2x(sqrtpd, kX86InstIdSqrtpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(sqrtpd, kX86InstIdSqrtpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP square root (SSE2).
  INST_2x(sqrtsd, kX86InstIdSqrtsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(sqrtsd, kX86InstIdSqrtsd, X86XmmVar, X86Mem)

  //! Packed DP-FP subtract (SSE2).
  INST_2x(subpd, kX86InstIdSubpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(subpd, kX86InstIdSubpd, X86XmmVar, X86Mem)

  //! Scalar DP-FP subtract (SSE2).
  INST_2x(subsd, kX86InstIdSubsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(subsd, kX86InstIdSubsd, X86XmmVar, X86Mem)

  //! Scalar DP-FP unordered compare and set EFLAGS (SSE2).
  INST_2x(ucomisd, kX86InstIdUcomisd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(ucomisd, kX86InstIdUcomisd, X86XmmVar, X86Mem)

  //! Unpack and interleave high packed DP-FP (SSE2).
  INST_2x(unpckhpd, kX86InstIdUnpckhpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(unpckhpd, kX86InstIdUnpckhpd, X86XmmVar, X86Mem)

  //! Unpack and interleave low packed DP-FP (SSE2).
  INST_2x(unpcklpd, kX86InstIdUnpcklpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(unpcklpd, kX86InstIdUnpcklpd, X86XmmVar, X86Mem)

  //! Packed DP-FP bitwise xor (SSE2).
  INST_2x(xorpd, kX86InstIdXorpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(xorpd, kX86InstIdXorpd, X86XmmVar, X86Mem)

  // --------------------------------------------------------------------------
  // [SSE3]
  // --------------------------------------------------------------------------

  //! Packed DP-FP add/subtract (SSE3).
  INST_2x(addsubpd, kX86InstIdAddsubpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(addsubpd, kX86InstIdAddsubpd, X86XmmVar, X86Mem)

  //! Packed SP-FP add/subtract (SSE3).
  INST_2x(addsubps, kX86InstIdAddsubps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(addsubps, kX86InstIdAddsubps, X86XmmVar, X86Mem)

  //! Store truncated `fp0` to `short_or_int_or_long[o0]` and POP (FPU & SSE3).
  INST_1x(fisttp, kX86InstIdFisttp, X86Mem)

  //! Packed DP-FP horizontal add (SSE3).
  INST_2x(haddpd, kX86InstIdHaddpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(haddpd, kX86InstIdHaddpd, X86XmmVar, X86Mem)

  //! Packed SP-FP horizontal add (SSE3).
  INST_2x(haddps, kX86InstIdHaddps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(haddps, kX86InstIdHaddps, X86XmmVar, X86Mem)

  //! Packed DP-FP horizontal subtract (SSE3).
  INST_2x(hsubpd, kX86InstIdHsubpd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(hsubpd, kX86InstIdHsubpd, X86XmmVar, X86Mem)

  //! Packed SP-FP horizontal subtract (SSE3).
  INST_2x(hsubps, kX86InstIdHsubps, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(hsubps, kX86InstIdHsubps, X86XmmVar, X86Mem)

  //! Load 128-bits unaligned (SSE3).
  INST_2x(lddqu, kX86InstIdLddqu, X86XmmVar, X86Mem)

  // //! Setup monitor address (SSE3).
  // INST_0x(monitor, kX86InstIdMonitor)

  //! Move one DP-FP and duplicate (SSE3).
  INST_2x(movddup, kX86InstIdMovddup, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movddup, kX86InstIdMovddup, X86XmmVar, X86Mem)

  //! Move packed SP-FP high and duplicate (SSE3).
  INST_2x(movshdup, kX86InstIdMovshdup, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movshdup, kX86InstIdMovshdup, X86XmmVar, X86Mem)

  //! Move packed SP-FP low and duplicate (SSE3).
  INST_2x(movsldup, kX86InstIdMovsldup, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(movsldup, kX86InstIdMovsldup, X86XmmVar, X86Mem)

  // //! Monitor wait (SSE3).
  // INST_0x(mwait, kX86InstIdMwait)

  // --------------------------------------------------------------------------
  // [SSSE3]
  // --------------------------------------------------------------------------

  //! Packed BYTE sign (SSSE3).
  INST_2x(psignb, kX86InstIdPsignb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psignb, kX86InstIdPsignb, X86MmVar, X86Mem)

  //! PackedBYTE  sign (SSSE3).
  INST_2x(psignb, kX86InstIdPsignb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psignb, kX86InstIdPsignb, X86XmmVar, X86Mem)

  //! Packed DWORD sign (SSSE3).
  INST_2x(psignd, kX86InstIdPsignd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psignd, kX86InstIdPsignd, X86MmVar, X86Mem)

  //! Packed DWORD sign (SSSE3).
  INST_2x(psignd, kX86InstIdPsignd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psignd, kX86InstIdPsignd, X86XmmVar, X86Mem)

  //! Packed WORD sign (SSSE3).
  INST_2x(psignw, kX86InstIdPsignw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(psignw, kX86InstIdPsignw, X86MmVar, X86Mem)

  //! Packed WORD sign (SSSE3).
  INST_2x(psignw, kX86InstIdPsignw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(psignw, kX86InstIdPsignw, X86XmmVar, X86Mem)

  //! Packed DWORD horizontal add (SSSE3).
  INST_2x(phaddd, kX86InstIdPhaddd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(phaddd, kX86InstIdPhaddd, X86MmVar, X86Mem)

  //! Packed DWORD horizontal add (SSSE3).
  INST_2x(phaddd, kX86InstIdPhaddd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phaddd, kX86InstIdPhaddd, X86XmmVar, X86Mem)

  //! Packed WORD horizontal add with saturation (SSSE3).
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86MmVar, X86Mem)

  //! Packed WORD horizontal add with with saturation (SSSE3).
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phaddsw, kX86InstIdPhaddsw, X86XmmVar, X86Mem)

  //! Packed WORD horizontal add (SSSE3).
  INST_2x(phaddw, kX86InstIdPhaddw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(phaddw, kX86InstIdPhaddw, X86MmVar, X86Mem)

  //! Packed WORD horizontal add (SSSE3).
  INST_2x(phaddw, kX86InstIdPhaddw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phaddw, kX86InstIdPhaddw, X86XmmVar, X86Mem)

  //! Packed DWORD horizontal subtract (SSSE3).
  INST_2x(phsubd, kX86InstIdPhsubd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(phsubd, kX86InstIdPhsubd, X86MmVar, X86Mem)

  //! Packed DWORD horizontal subtract (SSSE3).
  INST_2x(phsubd, kX86InstIdPhsubd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phsubd, kX86InstIdPhsubd, X86XmmVar, X86Mem)

  //! Packed WORD horizontal subtract with saturation (SSSE3).
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86MmVar, X86Mem)

  //! Packed WORD horizontal subtract with saturation (SSSE3).
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phsubsw, kX86InstIdPhsubsw, X86XmmVar, X86Mem)

  //! Packed WORD horizontal subtract (SSSE3).
  INST_2x(phsubw, kX86InstIdPhsubw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(phsubw, kX86InstIdPhsubw, X86MmVar, X86Mem)

  //! Packed WORD horizontal subtract (SSSE3).
  INST_2x(phsubw, kX86InstIdPhsubw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phsubw, kX86InstIdPhsubw, X86XmmVar, X86Mem)

  //! Packed multiply and add signed and unsigned bytes (SSSE3).
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86MmVar, X86Mem)

  //! Packed multiply and add signed and unsigned bytes (SSSE3).
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaddubsw, kX86InstIdPmaddubsw, X86XmmVar, X86Mem)

  //! Packed BYTE absolute value (SSSE3).
  INST_2x(pabsb, kX86InstIdPabsb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pabsb, kX86InstIdPabsb, X86MmVar, X86Mem)

  //! Packed BYTE absolute value (SSSE3).
  INST_2x(pabsb, kX86InstIdPabsb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pabsb, kX86InstIdPabsb, X86XmmVar, X86Mem)

  //! Packed DWORD absolute value (SSSE3).
  INST_2x(pabsd, kX86InstIdPabsd, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pabsd, kX86InstIdPabsd, X86MmVar, X86Mem)

  //! Packed DWORD absolute value (SSSE3).
  INST_2x(pabsd, kX86InstIdPabsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pabsd, kX86InstIdPabsd, X86XmmVar, X86Mem)

  //! Packed WORD absolute value (SSSE3).
  INST_2x(pabsw, kX86InstIdPabsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pabsw, kX86InstIdPabsw, X86MmVar, X86Mem)

  //! Packed WORD absolute value (SSSE3).
  INST_2x(pabsw, kX86InstIdPabsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pabsw, kX86InstIdPabsw, X86XmmVar, X86Mem)

  //! Packed WORD multiply high, round and scale (SSSE3).
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86MmVar, X86Mem)

  //! Packed WORD multiply high, round and scale (SSSE3).
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmulhrsw, kX86InstIdPmulhrsw, X86XmmVar, X86Mem)

  //! Packed BYTE shuffle (SSSE3).
  INST_2x(pshufb, kX86InstIdPshufb, X86MmVar, X86MmVar)
  //! \overload
  INST_2x(pshufb, kX86InstIdPshufb, X86MmVar, X86Mem)

  //! Packed BYTE shuffle (SSSE3).
  INST_2x(pshufb, kX86InstIdPshufb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pshufb, kX86InstIdPshufb, X86XmmVar, X86Mem)

  //! Packed align right (SSSE3).
  INST_3i(palignr, kX86InstIdPalignr, X86MmVar, X86MmVar, Imm)
  //! \overload
  INST_3i(palignr, kX86InstIdPalignr, X86MmVar, X86Mem, Imm)

  //! Packed align right (SSSE3).
  INST_3i(palignr, kX86InstIdPalignr, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(palignr, kX86InstIdPalignr, X86XmmVar, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [SSE4.1]
  // --------------------------------------------------------------------------

  //! Packed DP-FP blend (SSE4.1).
  INST_3i(blendpd, kX86InstIdBlendpd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(blendpd, kX86InstIdBlendpd, X86XmmVar, X86Mem, Imm)

  //! Packed SP-FP blend (SSE4.1).
  INST_3i(blendps, kX86InstIdBlendps, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(blendps, kX86InstIdBlendps, X86XmmVar, X86Mem, Imm)

  //! Packed DP-FP variable blend (SSE4.1).
  INST_3x(blendvpd, kX86InstIdBlendvpd, X86XmmVar, X86XmmVar, X86XmmVar)
  //! \overload
  INST_3x(blendvpd, kX86InstIdBlendvpd, X86XmmVar, X86Mem, X86XmmVar)

  //! Packed SP-FP variable blend (SSE4.1).
  INST_3x(blendvps, kX86InstIdBlendvps, X86XmmVar, X86XmmVar, X86XmmVar)
  //! \overload
  INST_3x(blendvps, kX86InstIdBlendvps, X86XmmVar, X86Mem, X86XmmVar)

  //! Packed DP-FP dot product (SSE4.1).
  INST_3i(dppd, kX86InstIdDppd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(dppd, kX86InstIdDppd, X86XmmVar, X86Mem, Imm)

  //! Packed SP-FP dot product (SSE4.1).
  INST_3i(dpps, kX86InstIdDpps, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(dpps, kX86InstIdDpps, X86XmmVar, X86Mem, Imm)

  //! Extract SP-FP based on selector (SSE4.1).
  INST_3i(extractps, kX86InstIdExtractps, X86GpVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(extractps, kX86InstIdExtractps, X86Mem, X86XmmVar, Imm)

  //! Insert SP-FP based on selector (SSE4.1).
  INST_3i(insertps, kX86InstIdInsertps, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(insertps, kX86InstIdInsertps, X86XmmVar, X86Mem, Imm)

  //! Load OWORD aligned using NT hint (SSE4.1).
  INST_2x(movntdqa, kX86InstIdMovntdqa, X86XmmVar, X86Mem)

  //! Packed WORD sums of absolute difference (SSE4.1).
  INST_3i(mpsadbw, kX86InstIdMpsadbw, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(mpsadbw, kX86InstIdMpsadbw, X86XmmVar, X86Mem, Imm)

  //! Pack DWORDs to WORDs with unsigned saturation (SSE4.1).
  INST_2x(packusdw, kX86InstIdPackusdw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(packusdw, kX86InstIdPackusdw, X86XmmVar, X86Mem)

  //! Packed BYTE variable blend (SSE4.1).
  INST_3x(pblendvb, kX86InstIdPblendvb, X86XmmVar, X86XmmVar, X86XmmVar)
  //! \overload
  INST_3x(pblendvb, kX86InstIdPblendvb, X86XmmVar, X86Mem, X86XmmVar)

  //! Packed WORD blend (SSE4.1).
  INST_3i(pblendw, kX86InstIdPblendw, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pblendw, kX86InstIdPblendw, X86XmmVar, X86Mem, Imm)

  //! Packed QWORD compare for equality (SSE4.1).
  INST_2x(pcmpeqq, kX86InstIdPcmpeqq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpeqq, kX86InstIdPcmpeqq, X86XmmVar, X86Mem)

  //! Extract BYTE based on selector (SSE4.1).
  INST_3i(pextrb, kX86InstIdPextrb, X86GpVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pextrb, kX86InstIdPextrb, X86Mem, X86XmmVar, Imm)

  //! Extract DWORD based on selector (SSE4.1).
  INST_3i(pextrd, kX86InstIdPextrd, X86GpVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pextrd, kX86InstIdPextrd, X86Mem, X86XmmVar, Imm)

  //! Extract QWORD based on selector (SSE4.1).
  INST_3i(pextrq, kX86InstIdPextrq, X86GpVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pextrq, kX86InstIdPextrq, X86Mem, X86XmmVar, Imm)

  //! Extract WORD based on selector (SSE4.1).
  INST_3i(pextrw, kX86InstIdPextrw, X86Mem, X86XmmVar, Imm)

  //! Packed WORD horizontal minimum (SSE4.1).
  INST_2x(phminposuw, kX86InstIdPhminposuw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(phminposuw, kX86InstIdPhminposuw, X86XmmVar, X86Mem)

  //! Insert BYTE based on selector (SSE4.1).
  INST_3i(pinsrb, kX86InstIdPinsrb, X86XmmVar, X86GpVar, Imm)
  //! \overload
  INST_3i(pinsrb, kX86InstIdPinsrb, X86XmmVar, X86Mem, Imm)

  //! Insert DWORD based on selector (SSE4.1).
  INST_3i(pinsrd, kX86InstIdPinsrd, X86XmmVar, X86GpVar, Imm)
  //! \overload
  INST_3i(pinsrd, kX86InstIdPinsrd, X86XmmVar, X86Mem, Imm)

  //! Insert QWORD based on selector (SSE4.1).
  INST_3i(pinsrq, kX86InstIdPinsrq, X86XmmVar, X86GpVar, Imm)
  //! \overload
  INST_3i(pinsrq, kX86InstIdPinsrq, X86XmmVar, X86Mem, Imm)

  //! Packed BYTE maximum (SSE4.1).
  INST_2x(pmaxsb, kX86InstIdPmaxsb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaxsb, kX86InstIdPmaxsb, X86XmmVar, X86Mem)

  //! Packed DWORD maximum (SSE4.1).
  INST_2x(pmaxsd, kX86InstIdPmaxsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaxsd, kX86InstIdPmaxsd, X86XmmVar, X86Mem)

  //! Packed DWORD unsigned maximum (SSE4.1).
  INST_2x(pmaxud, kX86InstIdPmaxud, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaxud,kX86InstIdPmaxud , X86XmmVar, X86Mem)

  //! Packed WORD unsigned maximum (SSE4.1).
  INST_2x(pmaxuw, kX86InstIdPmaxuw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmaxuw, kX86InstIdPmaxuw, X86XmmVar, X86Mem)

  //! Packed BYTE minimum (SSE4.1).
  INST_2x(pminsb, kX86InstIdPminsb, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pminsb, kX86InstIdPminsb, X86XmmVar, X86Mem)

  //! Packed DWORD minimum (SSE4.1).
  INST_2x(pminsd, kX86InstIdPminsd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pminsd, kX86InstIdPminsd, X86XmmVar, X86Mem)

  //! Packed WORD unsigned minimum (SSE4.1).
  INST_2x(pminuw, kX86InstIdPminuw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pminuw, kX86InstIdPminuw, X86XmmVar, X86Mem)

  //! Packed DWORD unsigned minimum (SSE4.1).
  INST_2x(pminud, kX86InstIdPminud, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pminud, kX86InstIdPminud, X86XmmVar, X86Mem)

  //! Packed BYTE to DWORD with sign extend (SSE4.1).
  INST_2x(pmovsxbd, kX86InstIdPmovsxbd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovsxbd, kX86InstIdPmovsxbd, X86XmmVar, X86Mem)

  //! Packed BYTE to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxbq, kX86InstIdPmovsxbq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovsxbq, kX86InstIdPmovsxbq, X86XmmVar, X86Mem)

  //! Packed BYTE to WORD with sign extend (SSE4.1).
  INST_2x(pmovsxbw, kX86InstIdPmovsxbw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovsxbw, kX86InstIdPmovsxbw, X86XmmVar, X86Mem)

  //! Packed DWORD to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxdq, kX86InstIdPmovsxdq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovsxdq, kX86InstIdPmovsxdq, X86XmmVar, X86Mem)

  //! Packed WORD to DWORD with sign extend (SSE4.1).
  INST_2x(pmovsxwd, kX86InstIdPmovsxwd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovsxwd, kX86InstIdPmovsxwd, X86XmmVar, X86Mem)

  //! Packed WORD to QWORD with sign extend (SSE4.1).
  INST_2x(pmovsxwq, kX86InstIdPmovsxwq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovsxwq, kX86InstIdPmovsxwq, X86XmmVar, X86Mem)

  //! BYTE to DWORD with zero extend (SSE4.1).
  INST_2x(pmovzxbd, kX86InstIdPmovzxbd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovzxbd, kX86InstIdPmovzxbd, X86XmmVar, X86Mem)

  //! Packed BYTE to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxbq, kX86InstIdPmovzxbq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovzxbq, kX86InstIdPmovzxbq, X86XmmVar, X86Mem)

  //! BYTE to WORD with zero extend (SSE4.1).
  INST_2x(pmovzxbw, kX86InstIdPmovzxbw, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovzxbw, kX86InstIdPmovzxbw, X86XmmVar, X86Mem)

  //! Packed DWORD to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxdq, kX86InstIdPmovzxdq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovzxdq, kX86InstIdPmovzxdq, X86XmmVar, X86Mem)

  //! Packed WORD to DWORD with zero extend (SSE4.1).
  INST_2x(pmovzxwd, kX86InstIdPmovzxwd, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovzxwd, kX86InstIdPmovzxwd, X86XmmVar, X86Mem)

  //! Packed WORD to QWORD with zero extend (SSE4.1).
  INST_2x(pmovzxwq, kX86InstIdPmovzxwq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmovzxwq, kX86InstIdPmovzxwq, X86XmmVar, X86Mem)

  //! Packed DWORD to QWORD multiply (SSE4.1).
  INST_2x(pmuldq, kX86InstIdPmuldq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmuldq, kX86InstIdPmuldq, X86XmmVar, X86Mem)

  //! Packed DWORD multiply low (SSE4.1).
  INST_2x(pmulld, kX86InstIdPmulld, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pmulld, kX86InstIdPmulld, X86XmmVar, X86Mem)

  //! Logical compare (SSE4.1).
  INST_2x(ptest, kX86InstIdPtest, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(ptest, kX86InstIdPtest, X86XmmVar, X86Mem)

  //! Packed DP-FP round (SSE4.1).
  INST_3i(roundpd, kX86InstIdRoundpd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(roundpd, kX86InstIdRoundpd, X86XmmVar, X86Mem, Imm)

  //! Packed SP-FP round (SSE4.1).
  INST_3i(roundps, kX86InstIdRoundps, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(roundps, kX86InstIdRoundps, X86XmmVar, X86Mem, Imm)

  //! Scalar DP-FP round (SSE4.1).
  INST_3i(roundsd, kX86InstIdRoundsd, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(roundsd, kX86InstIdRoundsd, X86XmmVar, X86Mem, Imm)

  //! Scalar SP-FP round (SSE4.1).
  INST_3i(roundss, kX86InstIdRoundss, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(roundss, kX86InstIdRoundss, X86XmmVar, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [SSE4.2]
  // --------------------------------------------------------------------------

  //! Accumulate crc32 value (polynomial 0x11EDC6F41) (SSE4.2).
  INST_2x_(crc32, kX86InstIdCrc32, X86GpVar, X86GpVar, o0.isRegType(kX86RegTypeGpd) || o0.isRegType(kX86RegTypeGpq))
  //! \overload
  INST_2x_(crc32, kX86InstIdCrc32, X86GpVar, X86Mem, o0.isRegType(kX86RegTypeGpd) || o0.isRegType(kX86RegTypeGpq))

  //! Packed compare explicit length strings, return index (SSE4.2).
  INST_3i(pcmpestri, kX86InstIdPcmpestri, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pcmpestri, kX86InstIdPcmpestri, X86XmmVar, X86Mem, Imm)

  //! Packed compare explicit length strings, return mask (SSE4.2).
  INST_3i(pcmpestrm, kX86InstIdPcmpestrm, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pcmpestrm, kX86InstIdPcmpestrm, X86XmmVar, X86Mem, Imm)

  //! Packed compare implicit length strings, return index (SSE4.2).
  INST_3i(pcmpistri, kX86InstIdPcmpistri, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pcmpistri, kX86InstIdPcmpistri, X86XmmVar, X86Mem, Imm)

  //! Packed compare implicit length strings, return mask (SSE4.2).
  INST_3i(pcmpistrm, kX86InstIdPcmpistrm, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(pcmpistrm, kX86InstIdPcmpistrm, X86XmmVar, X86Mem, Imm)

  //! Packed QWORD compare if greater than (SSE4.2).
  INST_2x(pcmpgtq, kX86InstIdPcmpgtq, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(pcmpgtq, kX86InstIdPcmpgtq, X86XmmVar, X86Mem)

  // --------------------------------------------------------------------------
  // [SSE4a]
  // --------------------------------------------------------------------------

  //! Extract Field (SSE4a).
  INST_2x(extrq, kX86InstIdExtrq, X86XmmVar, X86XmmVar)
  //! Extract Field (SSE4a).
  INST_3ii(extrq, kX86InstIdExtrq, X86XmmVar, Imm, Imm)

  //! Insert Field (SSE4a).
  INST_2x(insertq, kX86InstIdInsertq, X86XmmVar, X86XmmVar)
  //! Insert Field (SSE4a).
  INST_4ii(insertq, kX86InstIdInsertq, X86XmmVar, X86XmmVar, Imm, Imm)

  //! Move Non-Temporal Scalar DP-FP (SSE4a).
  INST_2x(movntsd, kX86InstIdMovntsd, X86Mem, X86XmmVar)
  //! Move Non-Temporal Scalar SP-FP (SSE4a).
  INST_2x(movntss, kX86InstIdMovntss, X86Mem, X86XmmVar)

  // --------------------------------------------------------------------------
  // [POPCNT]
  // --------------------------------------------------------------------------

  //! Return the count of number of bits set to 1 (POPCNT).
  INST_2x_(popcnt, kX86InstIdPopcnt, X86GpVar, X86GpVar, !o0.isGpb() && o0.getSize() == o1.getSize())
  //! \overload
  INST_2x_(popcnt, kX86InstIdPopcnt, X86GpVar, X86Mem, !o0.isGpb())

  // --------------------------------------------------------------------------
  // [LZCNT]
  // --------------------------------------------------------------------------

  //! Count the number of leading zero bits (LZCNT).
  INST_2x(lzcnt, kX86InstIdLzcnt, X86GpVar, X86GpVar)
  //! \overload
  INST_2x(lzcnt, kX86InstIdLzcnt, X86GpVar, X86Mem)

  // --------------------------------------------------------------------------
  // [AESNI]
  // --------------------------------------------------------------------------

  //! Perform a single round of the AES decryption flow.
  INST_2x(aesdec, kX86InstIdAesdec, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(aesdec, kX86InstIdAesdec, X86XmmVar, X86Mem)

  //! Perform the last round of the AES decryption flow.
  INST_2x(aesdeclast, kX86InstIdAesdeclast, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(aesdeclast, kX86InstIdAesdeclast, X86XmmVar, X86Mem)

  //! Perform a single round of the AES encryption flow.
  INST_2x(aesenc, kX86InstIdAesenc, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(aesenc, kX86InstIdAesenc, X86XmmVar, X86Mem)

  //! Perform the last round of the AES encryption flow.
  INST_2x(aesenclast, kX86InstIdAesenclast, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(aesenclast, kX86InstIdAesenclast, X86XmmVar, X86Mem)

  //! Perform the InvMixColumns transformation.
  INST_2x(aesimc, kX86InstIdAesimc, X86XmmVar, X86XmmVar)
  //! \overload
  INST_2x(aesimc, kX86InstIdAesimc, X86XmmVar, X86Mem)

  //! Assist in expanding the AES cipher key.
  INST_3i(aeskeygenassist, kX86InstIdAeskeygenassist, X86XmmVar, X86XmmVar, Imm)
  //! \overload
  INST_3i(aeskeygenassist, kX86InstIdAeskeygenassist, X86XmmVar, X86Mem, Imm)

  // --------------------------------------------------------------------------
  // [PCLMULQDQ]
  // --------------------------------------------------------------------------

  //! Packed QWORD to OWORD carry-less multiply (PCLMULQDQ).
  INST_3i(pclmulqdq, kX86InstIdPclmulqdq, X86XmmVar, X86XmmVar, Imm);
  //! \overload
  INST_3i(pclmulqdq, kX86InstIdPclmulqdq, X86XmmVar, X86Mem, Imm);

  // --------------------------------------------------------------------------
  // [XSAVE]
  // --------------------------------------------------------------------------

  //! Restore Processor Extended States specified by `o1:o2` (XSAVE).
  INST_3x(xrstor, kX86InstIdXrstor, X86Mem, X86GpVar, X86GpVar)
  //! Restore Processor Extended States specified by `o1:o2` (XSAVE&X64).
  INST_3x(xrstor64, kX86InstIdXrstor64, X86Mem, X86GpVar, X86GpVar)

  //! Save Processor Extended States specified by `o1:o2` (XSAVE).
  INST_3x(xsave, kX86InstIdXsave, X86Mem, X86GpVar, X86GpVar)
  //! Save Processor Extended States specified by `o1:o2` (XSAVE&X64).
  INST_3x(xsave64, kX86InstIdXsave64, X86Mem, X86GpVar, X86GpVar)

  //! Save Processor Extended States specified by `o1:o2` (Optimized) (XSAVEOPT).
  INST_3x(xsaveopt, kX86InstIdXsave, X86Mem, X86GpVar, X86GpVar)
  //! Save Processor Extended States specified by `o1:o2` (Optimized) (XSAVEOPT&X64).
  INST_3x(xsaveopt64, kX86InstIdXsave64, X86Mem, X86GpVar, X86GpVar)

  //! Get XCR - `o1:o2 <- XCR[o0]` (`EDX:EAX <- XCR[ECX]`) (XSAVE).
  INST_3x(xgetbv, kX86InstIdXgetbv, X86GpVar, X86GpVar, X86GpVar)
  //! Set XCR - `XCR[o0] <- o1:o2` (`XCR[ECX] <- EDX:EAX`) (XSAVE).
  INST_3x(xsetbv, kX86InstIdXsetbv, X86GpVar, X86GpVar, X86GpVar)

  // --------------------------------------------------------------------------
  // [Cleanup]
  // --------------------------------------------------------------------------

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
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_X86_X86COMPILER_H
