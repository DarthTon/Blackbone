// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_COMPILER_H
#define _ASMJIT_COMPILER_H

// [Dependencies]
#include "Build.h"
#include "Defs.h"
#include "Operand.h"
#include "Util.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct Assembler;
struct CodeGenerator;
struct Compiler;
struct CompilerContext;
struct CompilerCore;
struct CompilerIntrinsics;
struct MemoryManager;

struct FunctionDefinition;

struct ForwardJumpData;

struct VarData;
struct VarAllocRecord;
struct StateData;

struct Emittable;
struct EAlign;
struct ECall;
struct EComment;
struct EData;
struct EEpilog;
struct EFunction;
struct EFunctionEnd;
struct EInstruction;
struct EJmp;
struct EProlog;
struct ERet;

// ============================================================================
// [AsmJit::TypeToId]
// ============================================================================

#if !defined(ASMJIT_NODOC)

#if defined(ASMJIT_HAS_PARTIAL_TEMPLATE_SPECIALIZATION)

template<typename T>
struct TypeToId
{
#if defined(ASMJIT_NODOC)
  enum { Id = INVALID_VALUE };
#endif // ASMJIT_NODOC
};

template<typename T>
struct TypeToId<T*> { enum { Id = VARIABLE_TYPE_INTPTR }; };

#else

// Same trict is used in Qt, Boost, Fog and all other libraries that need
// something similar.
//
// It's easy. It's needed to use sizeof() to determine the size
// of return value of this function. If size will be sizeof(char)
// (this is our type) then type is pointer, otherwise it's not.
template<typename T>
char TypeToId_NoPtiHelper(T*(*)());
// And specialization.
void* TypeToId_NoPtiHelper(...);

template<typename T>
struct TypeToId
{
  // TypeInfo constants
  enum
  {
    // This is the hackery result.
    Id = (sizeof(char) == sizeof( TypeToId_NoPtiHelper((T(*)())0) )
      ? VARIABLE_TYPE_INTPTR
      : INVALID_VALUE)
  };
};

#endif // ASMJIT_HAS_PARTIAL_TEMPLATE_SPECIALIZATION

#define ASMJIT_DECLARE_TYPE_AS_ID(__T__, __Id__) \
  template<> \
  struct TypeToId<__T__> { enum { Id = __Id__ }; }

// Declare void type and alternative.
struct Void {};
ASMJIT_DECLARE_TYPE_AS_ID(void, INVALID_VALUE);
ASMJIT_DECLARE_TYPE_AS_ID(Void, INVALID_VALUE);

#endif // ASMJIT_NODOC

// ============================================================================
// [AsmJit::Function Builder]
// ============================================================================

struct FunctionDefinition
{
  //! @brief Get function arguments IDs.
  inline const uint32_t* getArguments() const
  {
    return _arguments;
  }

  //! @brief Get function arguments count.
  inline uint32_t getArgumentsCount() const
  {
    return _argumentsCount;
  }

  inline uint32_t getArgument(uint32_t id) const
  {
    ASMJIT_ASSERT(id < _argumentsCount);
    return _arguments[id];
  }

  //! @brief Get function return value.
  inline uint32_t getReturnValue() const
  {
    return _returnValue;
  }

protected:
  inline void _setDefinition(const uint32_t* arguments, uint32_t argumentsCount, uint32_t returnValue)
  {
    _arguments = arguments;
    _argumentsCount = argumentsCount;
    _returnValue = returnValue;
  }

  const uint32_t* _arguments;
  uint32_t _argumentsCount;
  uint32_t _returnValue;
};

//! @brief Custom function builder for up to 32 function arguments.
struct FunctionBuilderX : public FunctionDefinition
{
  inline FunctionBuilderX()
  {
    _setDefinition(_argumentsData, 0, INVALID_VALUE);
  }

  template<typename T>
  inline void addArgument()
  {
    addArgumentRaw(TypeToId<ASMJIT_TYPE_TO_TYPE(T)>::Id);
  }

  template<typename T>
  inline void setArgument(uint32_t id)
  {
    setArgumentRaw(id, TypeToId<ASMJIT_TYPE_TO_TYPE(T)>::Id);
  }

  template<typename T>
  inline void setReturnValue()
  {
    setReturnValueRaw(TypeToId<ASMJIT_TYPE_TO_TYPE(T)>::Id);
  }

  inline void addArgumentRaw(uint32_t type)
  {
    ASMJIT_ASSERT(_argumentsCount < FUNC_MAX_ARGS);
    _argumentsData[_argumentsCount++] = type;
  }

  inline void setArgumentRaw(uint32_t id, uint32_t type)
  {
    ASMJIT_ASSERT(id < _argumentsCount);
    _argumentsData[id] = type;
  }

  inline void setReturnValueRaw(uint32_t returnValue)
  {
    _returnValue = returnValue;
  }

protected:
  uint32_t _argumentsData[FUNC_MAX_ARGS];
};

//! @brief Class used to build function without arguments.
template<typename RET>
struct FunctionBuilder0 : public FunctionDefinition
{
  inline FunctionBuilder0()
  {
    _setDefinition(NULL, 0, TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 1 argument.
template<typename RET, typename P0>
struct FunctionBuilder1 : public FunctionDefinition
{
  inline FunctionBuilder1()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 2 arguments.
template<typename RET, typename P0, typename P1>
struct FunctionBuilder2 : public FunctionDefinition
{
  inline FunctionBuilder2()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 3 arguments.
template<typename RET, typename P0, typename P1, typename P2>
struct FunctionBuilder3 : public FunctionDefinition
{
  inline FunctionBuilder3()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 4 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3>
struct FunctionBuilder4 : public FunctionDefinition
{
  inline FunctionBuilder4()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 5 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4>
struct FunctionBuilder5 : public FunctionDefinition
{
  inline FunctionBuilder5()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id,
      TypeToId<P4>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 6 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
struct FunctionBuilder6 : public FunctionDefinition
{
  inline FunctionBuilder6()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id,
      TypeToId<P4>::Id,
      TypeToId<P5>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 7 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct FunctionBuilder7 : public FunctionDefinition
{
  inline FunctionBuilder7()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id,
      TypeToId<P4>::Id,
      TypeToId<P5>::Id,
      TypeToId<P6>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 8 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct FunctionBuilder8 : public FunctionDefinition
{
  inline FunctionBuilder8()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id,
      TypeToId<P4>::Id,
      TypeToId<P5>::Id,
      TypeToId<P6>::Id,
      TypeToId<P7>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 9 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct FunctionBuilder9 : public FunctionDefinition
{
  inline FunctionBuilder9()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id,
      TypeToId<P4>::Id,
      TypeToId<P5>::Id,
      TypeToId<P6>::Id,
      TypeToId<P7>::Id,
      TypeToId<P8>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

//! @brief Class used to build function with 10 arguments.
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct FunctionBuilder10 : public FunctionDefinition
{
  inline FunctionBuilder10()
  {
    static const uint32_t args[] =
    {
      TypeToId<P0>::Id,
      TypeToId<P1>::Id,
      TypeToId<P2>::Id,
      TypeToId<P3>::Id,
      TypeToId<P4>::Id,
      TypeToId<P5>::Id,
      TypeToId<P6>::Id,
      TypeToId<P7>::Id,
      TypeToId<P8>::Id,
      TypeToId<P9>::Id
    };
    _setDefinition(args, ASMJIT_ARRAY_SIZE(args), TypeToId<RET>::Id);
  }
};

// ============================================================================
// [AsmJit::Emittable]
// ============================================================================

//! @brief Emmitable.
//!
//! Emittable is object that can emit single or more instructions. To
//! create your custom emittable it's needed to override the abstract virtual
//! method @c emit().
//!
//! When you are finished serializing instructions to the @c Compiler and you
//! call @c Compiler::make(), it will first call @c prepare() method for each
//! emittable in list, then @c translate(), @c emit() and @c post() is the last.
//! Prepare can be used to calculate something that can be only calculated when
//! emitting instructions is finished (for example @c Function uses @c prepare()
//! to relocate memory home for all memory/spilled variables). The @c emit() should 
//! be used to emit instruction or multiple instructions into @a Assembler stream,
//! and the @c post() is here to allow emitting embedded data (after function
//! declaration), etc.
struct ASMJIT_API Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new emittable.
  //!
  //! Never create @c Emittable by @c new operator or on the stack, use
  //! @c Compiler::newObject template to do that.
  Emittable(Compiler* c, uint32_t type) ASMJIT_NOTHROW;

  //! @brief Destroy emittable.
  //!
  //! @note Never destroy emittable using @c delete keyword, @c Compiler
  //! manages all emittables in internal memory pool and it will destroy
  //! all emittables after you destroy it.
  virtual ~Emittable() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit and Helpers]
  // --------------------------------------------------------------------------

  //! @brief Step 1. Extract emittable variables, update statistics, ...
  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  //! @brief Step 2. Translate instruction, alloc variables, ...
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;
  //! @brief Step 3. Emit to @c Assembler.
  virtual void emit(Assembler& a) ASMJIT_NOTHROW;
  //! @brief Step 4. Last post step (verify, add data, etc).
  virtual void post(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  //! @brief Get maximum size in bytes of this emittable (in binary).
  virtual int getMaxSize() const ASMJIT_NOTHROW;

  //! @brief Try to unuse the variable @a.
  //!
  //! Returns @c true only if the variable will be unused by the instruction,
  //! otherwise @c false is returned.
  virtual bool _tryUnuseVar(VarData* v) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Compiler]
  // --------------------------------------------------------------------------

  //! @brief Get associated compiler instance.
  inline Compiler* getCompiler() const ASMJIT_NOTHROW { return _compiler; }

  // --------------------------------------------------------------------------
  // [Type / Offset]
  // --------------------------------------------------------------------------

  //! @brief Get emittable type, see @c EMITTABLE_TYPE.
  inline uint32_t getType() const ASMJIT_NOTHROW { return _type; }

  //! @brief Get whether the emittable was translated.
  inline uint8_t isTranslated() const ASMJIT_NOTHROW { return _translated; }

  //! @brief Get emittable offset in the stream
  //!
  //! Emittable offset is not byte offset, each emittable increments offset by 1
  //! and this value is then used by register allocator. Emittable offset is
  //! set by compiler by the register allocator, don't use it in your code.
  inline uint32_t getOffset() const ASMJIT_NOTHROW { return _offset; }

  // --------------------------------------------------------------------------
  // [Emittables List]
  // --------------------------------------------------------------------------

  //! @brief Get previous emittable in list.
  inline Emittable* getPrev() const ASMJIT_NOTHROW { return _prev; }
  //! @brief Get next emittable in list.
  inline Emittable* getNext() const ASMJIT_NOTHROW { return _next; }

  // --------------------------------------------------------------------------
  // [Comment]
  // --------------------------------------------------------------------------

  //! @brief Get comment string.
  inline const char* getComment() const ASMJIT_NOTHROW { return _comment; }

  //! @brief Set comment string to @a str.
  void setComment(const char* str) ASMJIT_NOTHROW;

  //! @brief Format comment string using @a fmt string and variable argument list.
  void setCommentF(const char* fmt, ...) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Protected]
  // --------------------------------------------------------------------------

protected:
  //! @brief Mark emittable as translated and return next.
  inline Emittable* translated() ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(_translated == false);

    _translated = true;
    return _next;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Compiler where this emittable is connected to.
  Compiler* _compiler;

  //! @brief Type of emittable, see @c EMITTABLE_TYPE.
  uint8_t _type;
  //! @brief Whether the emittable was translated, see @c translate().
  uint8_t _translated;
  //! @brief Reserved flags for future use.
  uint8_t _reserved0;
  //! @brief Reserved flags for future use.
  uint8_t _reserved1;

  //! @brief Emittable offset.
  uint32_t _offset;

  //! @brief Previous emittable.
  Emittable* _prev;
  //! @brief Next emittable.
  Emittable* _next;

  //! @brief Embedded comment string (also used by a @c Comment emittable).
  const char* _comment;

private:
  friend struct CompilerCore;

  ASMJIT_DISABLE_COPY(Emittable)
};

// ============================================================================
// [AsmJit::EDummy]
// ============================================================================

//! @brief Dummy emittable, used as a mark.
//!
//! This emittable does nothing and it's only used by @ref Compiler to mark 
//! specific location in the code.
struct ASMJIT_API EDummy : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EDummy instance.
  EDummy(Compiler* c) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EDummy instance.
  virtual ~EDummy() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

private:
  ASMJIT_DISABLE_COPY(EDummy)
};

// ============================================================================
// [AsmJit::EFunctionEnd]
// ============================================================================

//! @brief End of function.
//!
//! This emittable does nothing and it's only used by @ref Compiler to mark 
//! specific location in the code. The @c EFunctionEnd is similar to @c EDummy,
//! except that it overrides @c translate() to return @c NULL.
struct ASMJIT_API EFunctionEnd : public EDummy
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EDummy instance.
  EFunctionEnd(Compiler* c) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EDummy instance.
  virtual ~EFunctionEnd() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit and Helpers]
  // --------------------------------------------------------------------------

  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;

private:
  ASMJIT_DISABLE_COPY(EFunctionEnd)
};

// ============================================================================
// [AsmJit::EComment]
// ============================================================================

//! @brief Emittable used to emit comment into @c Assembler logger.
//!
//! Comments allows to comment your assembler stream for better debugging
//! and visualization what's happening. Comments are ignored if logger is
//! not set.
//!
//! Comment string can't be modified after comment was created.
struct ASMJIT_API EComment : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EComment instance.
  EComment(Compiler* c, const char* comment) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EComment instance.
  virtual ~EComment() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

private:
  friend struct CompilerCore;

  ASMJIT_DISABLE_COPY(EComment)
};

// ============================================================================
// [AsmJit::EData]
// ============================================================================

//! @brief Emittable used to emit comment into @c Assembler logger.
//!
//! @note This class is always allocated by @c AsmJit::Compiler.
struct ASMJIT_API EData : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EData instance.
  EData(Compiler* c, const void* data, sysuint_t length) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EData instance.
  virtual ~EData() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  //! @brief Get pointer to embedded data.
  uint8_t* getData() const ASMJIT_NOTHROW { return (uint8_t*)_data; }

  //! @brief Get length of embedded data.
  sysuint_t getLength() const ASMJIT_NOTHROW { return _length; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Data length.
  sysuint_t _length;
  //! @brief Data buffer (that will be embedded to the assembler stream).
  uint8_t _data[sizeof(void*)];

private:
  friend struct CompilerCore;

  ASMJIT_DISABLE_COPY(EData)
};

// ============================================================================
// [AsmJit::EAlign]
// ============================================================================

//! @brief Emittable used to align assembler code.
struct ASMJIT_API EAlign : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EAlign instance.
  EAlign(Compiler* c, uint32_t size = 0) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EAlign instance.
  virtual ~EAlign() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Align Size]
  // --------------------------------------------------------------------------

  //! @brief Get align size in bytes.
  inline uint32_t getSize() const ASMJIT_NOTHROW { return _size; }
  //! @brief Set align size in bytes to @a size.
  inline void setSize(uint32_t size) ASMJIT_NOTHROW { _size = size; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Align size.
  uint32_t _size;

private:
  friend struct CompilerCore;

  ASMJIT_DISABLE_COPY(EAlign)
};

// ============================================================================
// [AsmJit::ETarget]
// ============================================================================

//! @brief Target - the bound label.
struct ASMJIT_API ETarget : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref ETarget instance.
  ETarget(Compiler* c, const Label& target) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref ETarget instance.
  virtual ~ETarget() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Return label bound to this target.
  inline const Label& getLabel() const ASMJIT_NOTHROW { return _label; }

  //! @brief Get first jmp instruction.
  inline EJmp* getFrom() const ASMJIT_NOTHROW { return _from; }

  //! @brief Get register allocator state for this target.
  inline StateData* getState() const ASMJIT_NOTHROW { return _state; }

  //! @brief Get number of jumps to this target.
  inline uint32_t getJumpsCount() const ASMJIT_NOTHROW { return _jumpsCount; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Label.
  Label _label;
  //! @brief First jump instruction that points to this target (label).
  EJmp* _from;
  //! @brief State at this location.
  StateData* _state;

  //! @brief Count of jumps to this target (label).
  uint32_t _jumpsCount;

private:
  friend struct CompilerContext;
  friend struct CompilerCore;
  friend struct EInstruction;
  friend struct EJmp;

  ASMJIT_DISABLE_COPY(ETarget)
};

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// ============================================================================
// [Platform Specific]
// ============================================================================

// [X86 / X64]
#if defined(ASMJIT_X86) || defined(ASMJIT_X64)
#include "CompilerX86X64.h"
#endif // ASMJIT_X86 || ASMJIT_X64

// [Guard]
#endif // _ASMJIT_COMPILER_H
