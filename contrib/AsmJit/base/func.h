// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_FUNC_H
#define _ASMJIT_BASE_FUNC_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies - AsmJit]
#include "../base/operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_tree
//! \{

// ============================================================================
// [asmjit::kFuncConv]
// ============================================================================

//! Function calling convention.
//!
//! For a platform specific calling conventions, see:
//!   - `x86x64::kFuncConv` - X86/X64 calling conventions.
ASMJIT_ENUM(kFuncConv) {
  //! Calling convention is invalid (can't be used).
  kFuncConvNone = 0
};

// ============================================================================
// [asmjit::kFuncHint]
// ============================================================================

//! Function hints.
//!
//! For a platform specific calling conventions, see:
//!   - `x86x64::kFuncHint` - X86/X64 function hints.
ASMJIT_ENUM(kFuncHint) {
  //! Make a naked function (default true).
  //!
  //! Naked function is function without using standard prolog/epilog sequence).
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Standard prolog sequence is:
  //!
  //! ~~~
  //! push zbp
  //! mov zsp, zbp
  //! sub zsp, StackAdjustment
  //! ~~~
  //!
  //! which is an equivalent to:
  //!
  //! ~~~
  //! enter StackAdjustment, 0
  //! ~~~
  //!
  //! Standard epilog sequence is:
  //!
  //! ~~~
  //! mov zsp, zbp
  //! pop zbp
  //! ~~~
  //!
  //! which is an equavalent to:
  //!
  //! ~~~
  //! leave
  //! ~~~
  //!
  //! Naked functions can omit the prolog/epilog sequence. The advantage of
  //! doing such modification is that EBP/RBP register can be used by the
  //! register allocator which can result in less spills/allocs.
  kFuncHintNaked = 0,

  //! Generate compact function prolog/epilog if possible.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Use shorter, but possible slower prolog/epilog sequence to save/restore
  //! registers.
  kFuncHintCompact = 1
};

// ============================================================================
// [asmjit::kFuncFlags]
// ============================================================================

//! Function flags.
//!
//! For a platform specific calling conventions, see:
//!   - `x86x64::kFuncFlags` - X86/X64 function flags.
ASMJIT_ENUM(kFuncFlags) {
  //! Whether the function is using naked (minimal) prolog / epilog.
  kFuncFlagIsNaked = 0x00000001,

  //! Whether an another function is called from this function.
  kFuncFlagIsCaller = 0x00000002,

  //! Whether the stack is not aligned to the required stack alignment,
  //! thus it has to be aligned manually.
  kFuncFlagIsStackMisaligned = 0x00000004,

  //! Whether the stack pointer is adjusted by the stack size needed
  //! to save registers and function variables.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Stack pointer (ESP/RSP) is adjusted by 'sub' instruction in prolog and by
  //! 'add' instruction in epilog (only if function is not naked). If function
  //! needs to perform manual stack alignment more instructions are used to
  //! adjust the stack (like "and zsp, -Alignment").
  kFuncFlagIsStackAdjusted = 0x00000008,

  //! Whether the function is finished using `BaseCompiler::endFunc()`.
  kFuncFlagIsFinished = 0x80000000
};

// ============================================================================
// [asmjit::kFuncDir]
// ============================================================================

//! Function arguments direction.
ASMJIT_ENUM(kFuncDir) {
  //! Arguments are passed left to right.
  //!
  //! This arguments direction is unusual in C, however it's used in Pascal.
  kFuncDirLtr = 0,

  //! Arguments are passed right ro left
  //!
  //! This is the default argument direction in C.
  kFuncDirRtl = 1
};

// ============================================================================
// [asmjit::kFuncArg]
// ============================================================================

//! Function argument (lo/hi) specification.
ASMJIT_ENUM(kFuncArg) {
  //! Maxumum number of function arguments supported by AsmJit.
  kFuncArgCount = 16,
  //! Extended maximum number of arguments (used internally).
  kFuncArgCountLoHi = kFuncArgCount * 2,

  //! Index to the LO part of function argument (default).
  //!
  //! This value is typically omitted and added only if there is HI argument
  //! accessed.
  kFuncArgLo = 0,
  //! Index to the HI part of function argument.
  //!
  //! HI part of function argument depends on target architecture. On x86 it's
  //! typically used to transfer 64-bit integers (they form a pair of 32-bit
  //! integers).
  kFuncArgHi = kFuncArgCount
};

// ============================================================================
// [asmjit::kFuncRet]
// ============================================================================

//! Function return value (lo/hi) specification.
ASMJIT_ENUM(kFuncRet) {
  //! Index to the LO part of function return value.
  kFuncRetLo = 0,
  //! Index to the HI part of function return value.
  kFuncRetHi = 1
};

// ============================================================================
// [asmjit::kFuncStackInvalid]
// ============================================================================

enum kFuncMisc {
  //! Invalid stack offset in function or function parameter.
  kFuncStackInvalid = -1
};

// ============================================================================
// [asmjit::FnTypeId]
// ============================================================================

//! Function builder 'void' type.
struct FnVoid {};

//! Function builder 'int8_t' type.
struct FnInt8 {};
//! Function builder 'uint8_t' type.
struct FnUInt8 {};

//! Function builder 'int16_t' type.
struct FnInt16 {};
//! Function builder 'uint16_t' type.
struct FnUInt16 {};

//! Function builder 'int32_t' type.
struct FnInt32 {};
//! Function builder 'uint32_t' type.
struct FnUInt32 {};

//! Function builder 'int64_t' type.
struct FnInt64 {};
//! Function builder 'uint64_t' type.
struct FnUInt64 {};

//! Function builder 'intptr_t' type.
struct FnIntPtr {};
//! Function builder 'uintptr_t' type.
struct FnUIntPtr {};

//! Function builder 'float' type.
struct FnFloat {};
//! Function builder 'double' type.
struct FnDouble {};

#if !defined(ASMJIT_DOCGEN)
template<typename T>
struct FnTypeId;

#define ASMJIT_DECLARE_TYPE_CORE(_PtrId_) \
  template<typename T> \
  struct TypeId { enum { kId = static_cast<int>(::asmjit::kVarTypeInvalid) }; }; \
  \
  template<typename T> \
  struct TypeId<T*> { enum { kId = _PtrId_ }; }

#define ASMJIT_DECLARE_TYPE_ID(_T_, _Id_) \
  template<> \
  struct TypeId<_T_> { enum { kId = _Id_ }; }

ASMJIT_DECLARE_TYPE_CORE(kVarTypeIntPtr);

ASMJIT_DECLARE_TYPE_ID(void, kVarTypeInvalid);
ASMJIT_DECLARE_TYPE_ID(FnVoid, kVarTypeInvalid);

ASMJIT_DECLARE_TYPE_ID(int8_t, kVarTypeInt8);
ASMJIT_DECLARE_TYPE_ID(FnInt8, kVarTypeInt8);

ASMJIT_DECLARE_TYPE_ID(uint8_t, kVarTypeUInt8);
ASMJIT_DECLARE_TYPE_ID(FnUInt8, kVarTypeUInt8);

ASMJIT_DECLARE_TYPE_ID(int16_t, kVarTypeInt16);
ASMJIT_DECLARE_TYPE_ID(FnInt16, kVarTypeInt16);

ASMJIT_DECLARE_TYPE_ID(uint16_t, kVarTypeUInt8);
ASMJIT_DECLARE_TYPE_ID(FnUInt16, kVarTypeUInt8);

ASMJIT_DECLARE_TYPE_ID(int32_t, kVarTypeInt32);
ASMJIT_DECLARE_TYPE_ID(FnInt32, kVarTypeUInt8);

ASMJIT_DECLARE_TYPE_ID(uint32_t, kVarTypeUInt32);
ASMJIT_DECLARE_TYPE_ID(FnUInt32, kVarTypeUInt8);

ASMJIT_DECLARE_TYPE_ID(int64_t, kVarTypeInt64);
ASMJIT_DECLARE_TYPE_ID(FnInt64, kVarTypeUInt8);

ASMJIT_DECLARE_TYPE_ID(uint64_t, kVarTypeUInt64);
ASMJIT_DECLARE_TYPE_ID(FnUInt64, kVarTypeUInt8);

ASMJIT_DECLARE_TYPE_ID(float, kVarTypeFp32);
ASMJIT_DECLARE_TYPE_ID(FnFloat, kVarTypeFp32);

ASMJIT_DECLARE_TYPE_ID(double, kVarTypeFp64);
ASMJIT_DECLARE_TYPE_ID(FnDouble, kVarTypeFp64);
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::FuncInOut]
// ============================================================================

//! Function in/out - argument or return value translated from `FuncPrototype`.
struct FuncInOut {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getVarType() const { return _varType; }

  ASMJIT_INLINE bool hasRegIndex() const { return _regIndex != kInvalidReg; }
  ASMJIT_INLINE uint32_t getRegIndex() const { return _regIndex; }

  ASMJIT_INLINE bool hasStackOffset() const { return _stackOffset != kFuncStackInvalid; }
  ASMJIT_INLINE int32_t getStackOffset() const { return static_cast<int32_t>(_stackOffset); }

  //! Get whether the argument / return value is assigned.
  ASMJIT_INLINE bool isSet() const {
    return (_regIndex != kInvalidReg) | (_stackOffset != kFuncStackInvalid);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the function argument to "unassigned state".
  ASMJIT_INLINE void reset() { _packed = 0xFFFFFFFF; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! Variable type, see `kVarType`.
      uint8_t _varType;
      //! Register index if argument / return value is a register.
      uint8_t _regIndex;
      //! Stack offset if argument / return value is on the stack.
      int16_t _stackOffset;
    };

    //! All members packed into single 32-bit integer.
    uint32_t _packed;
  };
};

// ============================================================================
// [asmjit::FuncPrototype]
// ============================================================================

//! Function prototype.
//!
//! Function prototype contains information about function return type, count
//! of arguments and their types. Function prototype is a low level structure
//! which doesn't contain platform specific or calling convention specific
//! information. Function prototype is used to create a `FuncDecl`.
struct FuncPrototype {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function return value.
  ASMJIT_INLINE uint32_t getRet() const { return _ret; }

  //! Get function arguments' IDs.
  ASMJIT_INLINE const uint32_t* getArgList() const { return _argList; }
  //! Get count of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const { return _argCount; }

  //! Get argument at index `id`.
  ASMJIT_INLINE uint32_t getArg(uint32_t id) const {
    ASMJIT_ASSERT(id < _argCount);
    return _argList[id];
  }

  //! Set function definition - return type and arguments.
  ASMJIT_INLINE void _setPrototype(uint32_t ret, const uint32_t* argList, uint32_t argCount) {
    _ret = ret;
    _argList = argList;
    _argCount = argCount;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _ret;
  uint32_t _argCount;
  const uint32_t* _argList;
};

// ============================================================================
// [asmjit::FuncDecl]
// ============================================================================

//! Function declaration.
struct FuncDecl {
  // --------------------------------------------------------------------------
  // [Accessors - Calling Convention]
  // --------------------------------------------------------------------------

  //! Get function calling convention, see `kFuncConv`.
  ASMJIT_INLINE uint32_t getConvention() const { return _convention; }

  //! Get whether the callee pops the stack.
  ASMJIT_INLINE uint32_t getCalleePopsStack() const { return _calleePopsStack; }

  //! Get direction of arguments passed on the stack.
  //!
  //! Direction should be always `kFuncDirRtl`.
  //!
  //! \note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  ASMJIT_INLINE uint32_t getDirection() const { return _direction; }

  //! Get stack size needed for function arguments passed on the stack.
  ASMJIT_INLINE uint32_t getArgStackSize() const { return _argStackSize; }
  //! Get size of "Red Zone".
  ASMJIT_INLINE uint32_t getRedZoneSize() const { return _redZoneSize; }
  //! Get size of "Spill Zone".
  ASMJIT_INLINE uint32_t getSpillZoneSize() const { return _spillZoneSize; }

  // --------------------------------------------------------------------------
  // [Accessors - Arguments and Return]
  // --------------------------------------------------------------------------

  //! Get whether the function has a return value.
  ASMJIT_INLINE bool hasRet() const { return _retCount != 0; }
  //! Get count of function return values.
  ASMJIT_INLINE uint32_t getRetCount() const { return _retCount; }

  //! Get function return value.
  ASMJIT_INLINE FuncInOut& getRet(uint32_t index = kFuncRetLo) { return _retList[index]; }
  //! Get function return value.
  ASMJIT_INLINE const FuncInOut& getRet(uint32_t index = kFuncRetLo) const { return _retList[index]; }

  //! Get count of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const { return _argCount; }

  //! Get function arguments array.
  ASMJIT_INLINE FuncInOut* getArgList() { return _argList; }
  //! Get function arguments array (const).
  ASMJIT_INLINE const FuncInOut* getArgList() const { return _argList; }

  //! Get function argument at index `index`.
  ASMJIT_INLINE FuncInOut& getArg(size_t index) {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _argList[index];
  }

  //! Get function argument at index `index`.
  ASMJIT_INLINE const FuncInOut& getArg(size_t index) const {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _argList[index];
  }

  ASMJIT_INLINE void resetArg(size_t index) {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    _argList[index].reset();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Calling convention.
  uint8_t _convention;
  //! Whether a callee pops stack.
  uint8_t _calleePopsStack : 1;
  //! Direction for arguments passed on the stack, see `kFuncDir`.
  uint8_t _direction : 1;
  //! Reserved #0 (alignment).
  uint8_t _reserved0 : 6;

  //! Count of arguments in `_argList`.
  uint8_t _argCount;
  //! Count of return value(s).
  uint8_t _retCount;

  //! Count of bytes consumed by arguments on the stack (aligned).
  uint32_t _argStackSize;

  //! Size of "Red Zone".
  //!
  //! \note Used by AMD64-ABI (128 bytes).
  uint16_t _redZoneSize;

  //! Size of "Spill Zone".
  //!
  //! \note Used by WIN64-ABI (32 bytes).
  uint16_t _spillZoneSize;

  //! Function arguments (including HI arguments) mapped to physical
  //! registers and stack offset.
  FuncInOut _argList[kFuncArgCountLoHi];

  //! Function return value(s).
  FuncInOut _retList[2];
};

// ============================================================================
// [asmjit::FuncBuilderX]
// ============================================================================

//! Custom function builder for up to 32 function arguments.
struct FuncBuilderX : public FuncPrototype {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncBuilderX() {
    _setPrototype(kVarTypeInvalid, _builderArgList, 0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Set return type to `retType`.
  ASMJIT_INLINE void setRet(uint32_t retType) {
    _ret = retType;
  }

  ASMJIT_INLINE void setArg(uint32_t id, uint32_t type) {
    ASMJIT_ASSERT(id < _argCount);
    _builderArgList[id] = type;
  }

  ASMJIT_INLINE void addArg(uint32_t type) {
    ASMJIT_ASSERT(_argCount < kFuncArgCount);
    _builderArgList[_argCount++] = type;
  }

  template<typename T>
  ASMJIT_INLINE void setRetT() {
    setRet(TypeId<T>::kId);
  }

  template<typename T>
  ASMJIT_INLINE void setArgT(uint32_t id) {
    setArg(id, TypeId<T>::kId);
  }

  template<typename T>
  ASMJIT_INLINE void addArgT() {
    addArg(TypeId<T>::kId);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _builderArgList[kFuncArgCount];
};

//! \internal
#define _TID(_T_) TypeId<_T_>::kId

//! Function builder (no args).
template<typename RET>
struct FuncBuilder0 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder0() {
    _setPrototype(_TID(RET), NULL, 0);
  }
};

//! Function builder (1 argument).
template<typename RET, typename P0>
struct FuncBuilder1 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder1() {
    static const uint32_t args[] = { _TID(P0) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (2 arguments).
template<typename RET, typename P0, typename P1>
struct FuncBuilder2 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder2() {
    static const uint32_t args[] = { _TID(P0), _TID(P1) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (3 arguments).
template<typename RET, typename P0, typename P1, typename P2>
struct FuncBuilder3 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder3() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (4 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3>
struct FuncBuilder4 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder4() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2), _TID(P3) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (5 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4>
struct FuncBuilder5 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder5() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2), _TID(P3), _TID(P4) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (6 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
struct FuncBuilder6 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder6() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2), _TID(P3), _TID(P4), _TID(P5) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (7 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct FuncBuilder7 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder7() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2), _TID(P3), _TID(P4), _TID(P5), _TID(P6) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (8 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct FuncBuilder8 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder8() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2),_TID(P3), _TID(P4), _TID(P5), _TID(P6), _TID(P7) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (9 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct FuncBuilder9 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder9() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2), _TID(P3), _TID(P4), _TID(P5), _TID(P6), _TID(P7), _TID(P8) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function builder (10 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct FuncBuilder10 : public FuncPrototype {
  ASMJIT_INLINE FuncBuilder10() {
    static const uint32_t args[] = { _TID(P0), _TID(P1), _TID(P2), _TID(P3), _TID(P4), _TID(P5), _TID(P6), _TID(P7), _TID(P8), _TID(P9) };
    _setPrototype(_TID(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

#undef _TID

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_FUNC_H
