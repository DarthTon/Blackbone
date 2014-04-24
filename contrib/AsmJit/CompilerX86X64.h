// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_COMPILERX86X64_H
#define _ASMJIT_COMPILERX86X64_H

#if !defined(_ASMJIT_COMPILER_H)
#warning "AsmJit/CompilerX86X64.h can be only included by AsmJit/Compiler.h"
#endif // _ASMJIT_COMPILER_H

// [Dependencies]
#include "Build.h"
#include "Assembler.h"
#include "Defs.h"
#include "Operand.h"
#include "Util.h"

#include <string.h>

// A little bit C++.
#include <new>

// [Api-Begin]
#include "ApiBegin.h"

//! @internal
//!
//! @brief Mark methods not supported by @ref Compiler. These methods are
//! usually used only in function prologs/epilogs or to manage stack.
#define ASMJIT_NOT_SUPPORTED_BY_COMPILER 0

namespace AsmJit {

#pragma warning(disable : 4201)

//! @addtogroup AsmJit_Compiler
//! @{

// ============================================================================
// [AsmJit::TypeToId]
// ============================================================================

// Skip documenting this.
#if !defined(ASMJIT_NODOC)

ASMJIT_DECLARE_TYPE_AS_ID(int8_t, VARIABLE_TYPE_GPD);
ASMJIT_DECLARE_TYPE_AS_ID(uint8_t, VARIABLE_TYPE_GPD);

ASMJIT_DECLARE_TYPE_AS_ID(int16_t, VARIABLE_TYPE_GPD);
ASMJIT_DECLARE_TYPE_AS_ID(uint16_t, VARIABLE_TYPE_GPD);

ASMJIT_DECLARE_TYPE_AS_ID(int32_t, VARIABLE_TYPE_GPD);
ASMJIT_DECLARE_TYPE_AS_ID(uint32_t, VARIABLE_TYPE_GPD);

#if defined(ASMJIT_X64)
ASMJIT_DECLARE_TYPE_AS_ID(int64_t, VARIABLE_TYPE_GPQ);
ASMJIT_DECLARE_TYPE_AS_ID(uint64_t, VARIABLE_TYPE_GPQ);
#endif // ASMJIT_X64

ASMJIT_DECLARE_TYPE_AS_ID(float, VARIABLE_TYPE_FLOAT);
ASMJIT_DECLARE_TYPE_AS_ID(double, VARIABLE_TYPE_DOUBLE);

#endif // !ASMJIT_NODOC

// ============================================================================
// [AsmJit::FunctionPrototype]
// ============================================================================

//! @brief Calling convention and function argument handling.
struct ASMJIT_API FunctionPrototype
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref FunctionPrototype instance.
  FunctionPrototype() ASMJIT_NOTHROW;
  //! @brief Destroy the @ref FunctionPrototype instance.
  ~FunctionPrototype() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Argument]
  // --------------------------------------------------------------------------

  //! @brief Function argument location.
  struct Argument
  {
    //! @brief Variable type, see @c VARIABLE_TYPE.
    uint32_t variableType;
    //! @brief Register index if argument is passed through register, otherwise
    //! @c INVALID_VALUE.
    uint32_t registerIndex;
    //! @brief Stack offset if argument is passed through stack, otherwise
    //! @c INVALID_VALUE.
    int32_t stackOffset;

    //! @brief Get whether the argument is assigned, for private use only.
    inline bool isAssigned() const ASMJIT_NOTHROW
    { return registerIndex != INVALID_VALUE || stackOffset != (int32_t)INVALID_VALUE; }
  };

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Set function prototype.
  //!
  //! This will set function calling convention and setup arguments variables.
  //!
  //! @note This function will allocate variables, it can be called only once.
  void setPrototype(
    uint32_t callingConvention,
    const uint32_t* arguments, 
    uint32_t argumentsCount,
    uint32_t returnValue) ASMJIT_NOTHROW;

  //! @brief Get function calling convention, see @c CALL_CONV.
  inline uint32_t getCallingConvention() const ASMJIT_NOTHROW { return _callingConvention; }

  //! @brief Get whether the callee pops the stack.
  inline uint32_t getCalleePopsStack() const ASMJIT_NOTHROW { return _calleePopsStack; }

  //! @brief Get function arguments.
  inline Argument* getArguments() ASMJIT_NOTHROW { return _arguments; }
  //! @brief Get function arguments (const version).
  inline const Argument* getArguments() const ASMJIT_NOTHROW { return _arguments; }

  //! @brief Get count of arguments.
  inline uint32_t getArgumentsCount() const ASMJIT_NOTHROW { return _argumentsCount; }

  //! @brief Get function return value or @ref INVALID_VALUE if it's void.
  inline uint32_t getReturnValue() const ASMJIT_NOTHROW { return _returnValue; }

  //! @brief Get direction of arguments passed on the stack.
  //!
  //! Direction should be always @c ARGUMENT_DIR_RIGHT_TO_LEFT.
  //!
  //! @note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  inline uint32_t getArgumentsDirection() const ASMJIT_NOTHROW { return _argumentsDirection; }

  //! @brief Get stack size needed for function arguments passed on the stack.
  inline uint32_t getArgumentsStackSize() const ASMJIT_NOTHROW { return _argumentsStackSize; }

  //! @brief Get registers used to pass first integer parameters by current
  //! calling convention.
  //!
  //! @note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  inline const uint32_t* getArgumentsGPList() const ASMJIT_NOTHROW { return _argumentsGPList; }

  //! @brief Get registers used to pass first SP-FP or DP-FPparameters by
  //! current calling convention.
  //!
  //! @note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  inline const uint32_t* getArgumentsXMMList() const ASMJIT_NOTHROW { return _argumentsXMMList; }

  //! @brief Get bitmask of GP registers which might be used for arguments.
  inline uint32_t getArgumentsGP() const ASMJIT_NOTHROW { return _argumentsGP; }
  //! @brief Get bitmask of MM registers which might be used for arguments.
  inline uint32_t getArgumentsMM() const ASMJIT_NOTHROW { return _argumentsMM; }
  //! @brief Get bitmask of XMM registers which might be used for arguments.
  inline uint32_t getArgumentsXMM() const ASMJIT_NOTHROW { return _argumentsXMM; }

  //! @brief Get bitmask of general purpose registers that's preserved
  //! (non-volatile).
  //!
  //! @note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  inline uint32_t getPreservedGP() const ASMJIT_NOTHROW { return _preservedGP; }

  //! @brief Get bitmask of MM registers that's preserved (non-volatile).
  //!
  //! @note No standardized calling function is not preserving MM registers.
  //! This member is here for extension writers who need for some reason custom
  //! calling convention that can be called through code generated by AsmJit
  //! (or other runtime code generator).
  inline uint32_t getPreservedMM() const ASMJIT_NOTHROW { return _preservedMM; }

  //! @brief Return bitmask of XMM registers that's preserved (non-volatile).
  //!
  //! @note This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  inline uint32_t getPreservedXMM() const ASMJIT_NOTHROW { return _preservedXMM; }

  //! @brief Get mask of all GP registers used to pass function arguments.
  inline uint32_t getPassedGP() const ASMJIT_NOTHROW { return _passedGP; }
  //! @brief Get mask of all MM registers used to pass function arguments.
  inline uint32_t getPassedMM() const ASMJIT_NOTHROW { return _passedMM; }
  //! @brief Get mask of all XMM registers used to pass function arguments.
  inline uint32_t getPassedXMM() const ASMJIT_NOTHROW { return _passedXMM; }

  //! @brief Find argument (id) by the register code. Used mainly by @ref ECall
  //! emittable.
  uint32_t findArgumentByRegisterCode(uint32_t regCode) const ASMJIT_NOTHROW;

protected:

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

  void _clear() ASMJIT_NOTHROW;
  void _setCallingConvention(uint32_t callingConvention) ASMJIT_NOTHROW;
  void _setPrototype(
    const uint32_t* arguments,
    uint32_t argumentsCount,
    uint32_t returnValue) ASMJIT_NOTHROW;
  void _setReturnValue(uint32_t valueId) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Calling convention.
  uint32_t _callingConvention;
  //! @brief Whether callee pops stack.
  uint32_t _calleePopsStack;

  //! @brief List of arguments, their register codes or stack locations.
  Argument _arguments[FUNC_MAX_ARGS];

  //! @brief Function return value.
  uint32_t _returnValue;

  //! @brief Count of arguments (in @c _argumentsList).
  uint32_t _argumentsCount;
  //! @brief Direction for arguments passed on the stack, see @c ARGUMENT_DIR.
  uint32_t _argumentsDirection;
  //! @brief Count of bytes consumed by arguments on the stack.
  uint32_t _argumentsStackSize;

  //! @brief List of registers that's used for first GP arguments.
  uint32_t _argumentsGPList[16];
  //! @brief List of registers that's used for first XMM arguments.
  uint32_t _argumentsXMMList[16];

  //! @brief Bitmask for preserved GP registers.
  uint32_t _argumentsGP;
  //! @brief Bitmask for preserved MM registers.
  uint32_t _argumentsMM;
  //! @brief Bitmask for preserved XMM registers.
  uint32_t _argumentsXMM;

  //! @brief Bitmask for preserved GP registers.
  uint32_t _preservedGP;
  //! @brief Bitmask for preserved MM registers.
  uint32_t _preservedMM;
  //! @brief Bitmask for preserved XMM registers.
  uint32_t _preservedXMM;

  // Set by _setPrototype().

  //! @brief Bitmask for GP registers used as function arguments.
  uint32_t _passedGP;
  //! @brief Bitmask for GP registers used as function arguments.
  uint32_t _passedMM;
  //! @brief Bitmask for GP registers used as function arguments.
  uint32_t _passedXMM;
};

// ============================================================================
// [AsmJit::VarData]
// ============================================================================

//! @brief Variable data (used internally by @c Compiler).
struct VarData
{
  // --------------------------------------------------------------------------
  // [Scope]
  // --------------------------------------------------------------------------

  //! @brief Scope (NULL if variable is global).
  EFunction* scope;

  //! @brief The first emittable where the variable is accessed.
  //!
  //! @note If this member is @c NULL then variable is unused.
  Emittable* firstEmittable;
  //! @brief The first callable (ECall) which is after the @c firstEmittable.
  ECall* firstCallable;
  //! @brief The last emittable where the variable is accessed.
  Emittable* lastEmittable;

  // --------------------------------------------------------------------------
  // [Id / Name]
  // --------------------------------------------------------------------------

  //! @brief Variable name.
  const char* name;
  //! @brief Variable id.
  uint32_t id;
  //! @brief Variable type.
  uint32_t type;
  //! @brief Variable size.
  uint32_t size;

  // --------------------------------------------------------------------------
  // [Home]
  // --------------------------------------------------------------------------

  //! @brief Home register index or @c INVALID_VALUE (used by register allocator).
  uint32_t homeRegisterIndex;
  //! @brief Preferred register index.
  uint32_t prefRegisterMask;

  //! @brief Home memory address offset.
  int32_t homeMemoryOffset;
  //! @brief Used by @c CompilerContext, do not touch (NULL when created).
  void* homeMemoryData;

  // --------------------------------------------------------------------------
  // [Actual]
  // --------------------------------------------------------------------------

  //! @brief Actual register index (connected with actual @c StateData).
  uint32_t registerIndex;
  //! @brief Actual working offset. This member is set before register allocator
  //! is called. If workOffset is same as CompilerContext::_currentOffset then
  //! this variable is probably used in next instruction and can't be spilled.
  uint32_t workOffset;

  //! @brief Next active variable in circular double-linked list.
  VarData* nextActive;
  //! @brief Previous active variable in circular double-linked list.
  VarData* prevActive;

  // --------------------------------------------------------------------------
  // [Flags]
  // --------------------------------------------------------------------------

  //! @brief Variable priority.
  uint8_t priority;
  //! @brief Whether variable content can be calculated by simple instruction
  //!
  //! This is used mainly by mmx or sse2 code and variable allocator will
  //! never reserve space for this variable. Calculated variables are for
  //! example all zeros, all ones, etc.
  uint8_t calculated;
  //! @brief Whether variable is argument passed through register.
  uint8_t isRegArgument;
  //! @brief Whether variable is argument passed through memory.
  uint8_t isMemArgument;

  //! @brief Variable state (connected with actual @c StateData).
  uint8_t state;
  //! @brief Whether variable was changed (connected with actual @c StateData).
  uint8_t changed;
  //! @brief Save on unuse (at end of the variable scope).
  uint8_t saveOnUnuse;

  // --------------------------------------------------------------------------
  // [Statistics]
  // --------------------------------------------------------------------------

  //! @brief Register read statistics (used by instructions where this variable needs
  //! to be read only).
  uint32_t registerReadCount;
  //! @brief Register write statistics (used by instructions where this variable needs
  //! to be write only).
  uint32_t registerWriteCount;
  //! @brief Register read+write statistics (used by instructions where this variable
  //! needs to be read and write).
  uint32_t registerRWCount;

  //! @brief Register GPB.LO statistics (for code generator).
  uint32_t registerGPBLoCount;
  //! @brief Register GPB.HI statistics (for code generator).
  uint32_t registerGPBHiCount;

  //! @brief Memory read statistics.
  uint32_t memoryReadCount;
  //! @brief Memory write statistics.
  uint32_t memoryWriteCount;
  //! @brief Memory read+write statistics.
  uint32_t memoryRWCount;

  // --------------------------------------------------------------------------
  // [Temporary]
  // --------------------------------------------------------------------------

  //! @brief Temporary data that can be used in prepare/translate stage.
  //!
  //! Initial value is NULL and each emittable/code that will use it must also
  //! clear it.
  //!
  //! This temporary data is designed to be used by algorithms that need to
  //! set some state into the variables, do something and then cleanup. See
  //! state-switch and function call.
  union
  {
    void* tempPtr;
    sysint_t tempInt;
  };
};

// ============================================================================
// [AsmJit::VarMemBlock]
// ============================================================================

struct VarMemBlock
{
  int32_t offset;
  uint32_t size;

  VarMemBlock* nextUsed;
  VarMemBlock* nextFree;
};

// ============================================================================
// [AsmJit::VarAllocRecord]
// ============================================================================

//! @brief Variable alloc record (for each instruction that uses variables).
//!
//! Variable record contains pointer to variable data and register allocation
//! flags. These flags are important to determine the best alloc instruction.
struct VarAllocRecord
{
  //! @brief Variable data (the structure owned by @c Compiler).
  VarData* vdata;
  //! @brief Variable alloc flags, see @c VARIABLE_ALLOC.
  uint32_t vflags;
  //! @brief Register mask (default is 0).
  uint32_t regMask;
};

// ============================================================================
// [AsmJit::VarCallRecord]
// ============================================================================

//! @brief Variable call-fn record (for each callable that uses variables).
//!
//! This record contains variables that are used to call a function (using 
//! @c ECall emittable). Each variable contains the registers where it must
//! be and registers where the value will be returned.
struct VarCallRecord
{
  //! @brief Variable data (the structure owned by @c Compiler).
  VarData* vdata;
  uint32_t flags;

  uint8_t inCount;
  uint8_t inDone;

  uint8_t outCount;
  uint8_t outDone;

  enum FLAGS
  {
    FLAG_IN_GP = 0x0001,
    FLAG_IN_MM = 0x0002,
    FLAG_IN_XMM = 0x0004,
    FLAG_IN_STACK = 0x0008,

    FLAG_OUT_EAX = 0x0010,
    FLAG_OUT_EDX = 0x0020,
    FLAG_OUT_ST0 = 0x0040,
    FLAG_OUT_ST1 = 0x0080,
    FLAG_OUT_MM0 = 0x0100,
    FLAG_OUT_XMM0 = 0x0400,
    FLAG_OUT_XMM1 = 0x0800,

    FLAG_IN_MEM_PTR = 0x1000,
    FLAG_CALL_OPERAND_REG = 0x2000,
    FLAG_CALL_OPERAND_MEM = 0x4000,

    FLAG_UNUSE_AFTER_USE = 0x8000
  };
};

// ============================================================================
// [AsmJit::VarHintRecord]
// ============================================================================

struct VarHintRecord
{
  VarData* vdata;
  uint32_t hint;
};

// ============================================================================
// [AsmJit::StateData]
// ============================================================================

//! @brief State data.
struct StateData
{
  enum { NUM_REGS = 16 + 8 + 16 };

  inline void clear() ASMJIT_NOTHROW
  {
    memset(this, 0, sizeof(*this));
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union
  {
    //! @brief All allocated variables in one array.
    VarData* regs[NUM_REGS];

    struct
    {
      //! @brief Allocated GP registers.
      VarData* gp[16];
      //! @brief Allocated MM registers.
      VarData* mm[8];
      //! @brief Allocated XMM registers.
      VarData* xmm[16];
    };
  };

  //! @brief Used GP registers bitmask.
  uint32_t usedGP;
  //! @brief Used MM registers bitmask.
  uint32_t usedMM;
  //! @brief Used XMM registers bitmask.
  uint32_t usedXMM;

  //! @brief Changed GP registers bitmask.
  uint32_t changedGP;
  //! @brief Changed MM registers bitmask.
  uint32_t changedMM;
  //! @brief Changed XMM registers bitmask.
  uint32_t changedXMM;

  //! @brief Count of variables in @c memVarsData.
  uint32_t memVarsCount;
  //! @brief Variables stored in memory (@c VARIABLE_STATE_MEMORY).
  //!
  //! When saving / restoring state it's important to keep registers which are
  //! still in memory. Register is always unused when it is going out-of-scope.
  //! All variables which are not here are unused (@c VARIABLE_STATE_UNUSED).
  VarData* memVarsData[1];
};

// ============================================================================
// [AsmJit::ForwardJumpData]
// ============================================================================

struct ForwardJumpData
{
  EJmp* inst;
  StateData* state;
  ForwardJumpData* next;
};

// ============================================================================
// [AsmJit::EVariableHint]
// ============================================================================

//! @brief Variable hint.
struct ASMJIT_API EVariableHint : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EVariableHint instance.
  EVariableHint(Compiler* c, VarData* vdata, uint32_t hintId, uint32_t hintValue) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EVariableHInt instance.
  virtual ~EVariableHint() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Hint]
  // --------------------------------------------------------------------------

  //! @brief Get assigned variable (data).
  inline VarData* getVar() const ASMJIT_NOTHROW { return _vdata; }

  //! @brief Get hint it (see @ref VARIABLE_HINT).
  inline uint32_t getHintId() const ASMJIT_NOTHROW { return _hintId; }
  //! @brief Get hint value.
  inline uint32_t getHintValue() const ASMJIT_NOTHROW { return _hintValue; }

  //! @brief Set hint it (see @ref VARIABLE_HINT).
  inline void setHintId(uint32_t hintId) ASMJIT_NOTHROW { _hintId = hintId; }
  //! @brief Set hint value.
  inline void setHintValue(uint32_t hintValue) ASMJIT_NOTHROW { _hintValue = hintValue; }

  VarData* _vdata;
  uint32_t _hintId;
  uint32_t _hintValue;
};

// ============================================================================
// [AsmJit::EInstruction]
// ============================================================================

//! @brief Emittable that represents single instruction and its operands.
struct ASMJIT_API EInstruction : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EInstruction instance.
  EInstruction(Compiler* c, uint32_t code, Operand* operandsData, uint32_t operandsCount) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EInstruction instance.
  virtual ~EInstruction() ASMJIT_NOTHROW;

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
  virtual bool _tryUnuseVar(VarData* v) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Instruction Code]
  // --------------------------------------------------------------------------

  //! @brief Get whether the instruction is special.
  inline bool isSpecial() const ASMJIT_NOTHROW { return _isSpecial; }

  //! @brief Get whether the instruction is FPU.
  inline bool isFPU() const ASMJIT_NOTHROW { return _isFPU; }

  //! @brief Get instruction code, see @c INST_CODE.
  inline uint32_t getCode() const ASMJIT_NOTHROW { return _code; }

  //! @brief Set instruction code to @a code.
  //!
  //! Please do not modify instruction code if you are not know what you are
  //! doing. Incorrect instruction code or operands can raise assertion() at
  //! runtime.
  inline void setCode(uint32_t code) ASMJIT_NOTHROW { _code = code; }

  // --------------------------------------------------------------------------
  // [Operands]
  // --------------------------------------------------------------------------

  //! @brief Get count of operands in operands array (number between 0 to 2 inclusive).
  inline uint32_t getOperandsCount() const ASMJIT_NOTHROW { return _operandsCount; }

  //! @brief Get operands array (3 operands total).
  inline Operand* getOperands() ASMJIT_NOTHROW { return _operands; }
  //! @brief Get operands array (3 operands total).
  inline const Operand* getOperands() const ASMJIT_NOTHROW { return _operands; }

  //! @brief Get memory operand.
  inline Mem* getMemOp() ASMJIT_NOTHROW { return _memOp; }
  //! @brief Set memory operand.
  inline void setMemOp(Mem* op) ASMJIT_NOTHROW { _memOp = op; }

  // --------------------------------------------------------------------------
  // [Variables]
  // --------------------------------------------------------------------------

  //! @brief Get count of variables in instruction operands (and in variables array).
  inline uint32_t getVariablesCount() const ASMJIT_NOTHROW { return _variablesCount; }

  //! @brief Get operands array (3 operands total).
  inline VarAllocRecord* getVariables() ASMJIT_NOTHROW { return _variables; }
  //! @brief Get operands array (3 operands total).
  inline const VarAllocRecord* getVariables() const ASMJIT_NOTHROW { return _variables; }

  // --------------------------------------------------------------------------
  // [Jump]
  // --------------------------------------------------------------------------

  //! @brief Get possible jump target.
  //!
  //! If this instruction is conditional or normal jump then return value is
  //! label location (ETarget instance), otherwise return value is @c NULL.
  virtual ETarget* getJumpTarget() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Instruction code, see @c INST_CODE.
  uint32_t _code;

  //! @brief Emit options, see @c EMIT_OPTIONS.
  uint32_t _emitOptions;

  //! @brief Operands count.
  uint32_t _operandsCount;

  //! @brief Variables count.
  uint32_t _variablesCount;

  //! @brief Operands.
  Operand* _operands;
  //! @brief Memory operand (if instruction contains any).
  Mem* _memOp;

  //! @brief Variables (extracted from operands).
  VarAllocRecord* _variables;

  //! @brief Whether the instruction is special.
  bool _isSpecial;
  //! @brief Whether the instruction is FPU.
  bool _isFPU;

  //! @brief Whether the one of the operands is GPB.Lo register.
  bool _isGPBLoUsed;
  //! @brief Whether the one of the operands is GPB.Hi register.
  bool _isGPBHiUsed;

  friend struct EFunction;
  friend struct CompilerContext;
  friend struct CompilerCore;

private:
  ASMJIT_DISABLE_COPY(EInstruction)
};

// ============================================================================
// [AsmJit::EJmp]
// ============================================================================

//! @brief Emittable that represents single instruction that can jump somewhere.
struct ASMJIT_API EJmp : public EInstruction
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  EJmp(Compiler* c, uint32_t code, Operand* operandsData, uint32_t operandsCount) ASMJIT_NOTHROW;
  virtual ~EJmp() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  void _doJump(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Jump]
  // --------------------------------------------------------------------------

  virtual ETarget* getJumpTarget() const ASMJIT_NOTHROW;

  inline EJmp* getJumpNext() const ASMJIT_NOTHROW { return _jumpNext; }
  inline bool isTaken() const ASMJIT_NOTHROW { return _isTaken; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  ETarget* _jumpTarget;
  EJmp *_jumpNext;
  StateData* _state;
  bool _isTaken;

  friend struct EFunction;
  friend struct CompilerContext;
  friend struct CompilerCore;

private:
  ASMJIT_DISABLE_COPY(EJmp)
};

// ============================================================================
// [AsmJit::EFunction]
// ============================================================================

//! @brief Function emittable used to generate C/C++ functions.
//!
//! Functions are base blocks for generating assembler output. Each generated
//! assembler stream needs standard entry and leave sequences thats compatible
//! to the operating system conventions - Application Binary Interface (ABI).
//!
//! Function class can be used to generate entry (prolog) and leave (epilog)
//! sequences that is compatible to a given calling convention and to allocate
//! and manage variables that can be allocated to registers or spilled.
//!
//! @note To create function use @c AsmJit::Compiler::newFunction() method, do
//! not create @c EFunction instances using other ways.
//!
//! @sa @c State, @c Var.
struct ASMJIT_API EFunction : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new @c Function instance.
  //!
  //! @note Always use @c AsmJit::Compiler::newFunction() to create @c Function
  //! instance.
  EFunction(Compiler* c) ASMJIT_NOTHROW;
  //! @brief Destroy @c Function instance.
  virtual ~EFunction() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Function Prototype (Calling Convention + Arguments) / Return Value]
  // --------------------------------------------------------------------------

  inline const FunctionPrototype& getPrototype() const ASMJIT_NOTHROW { return _functionPrototype; }
  inline uint32_t getHint(uint32_t hint) ASMJIT_NOTHROW { return _hints[hint]; }

  void setPrototype(
    uint32_t callingConvention, 
    const uint32_t* arguments, 
    uint32_t argumentsCount,
    uint32_t returnValue) ASMJIT_NOTHROW;
  void setHint(uint32_t hint, uint32_t value) ASMJIT_NOTHROW;

  inline EProlog* getProlog() const ASMJIT_NOTHROW { return _prolog; }
  inline EEpilog* getEpilog() const ASMJIT_NOTHROW { return _epilog; }

  inline EFunctionEnd* getEnd() const ASMJIT_NOTHROW { return _end; }

  //! @brief Create variables from FunctionPrototype declaration. This is just
  //! parsing what FunctionPrototype generated for current function calling
  //! convention and arguments.
  void _createVariables() ASMJIT_NOTHROW;

  //! @brief Prepare variables (ids, names, scope, registers).
  void _prepareVariables(Emittable* first) ASMJIT_NOTHROW;

  //! @brief Allocate variables (setting correct state, changing masks, etc).
  void _allocVariables(CompilerContext& cc) ASMJIT_NOTHROW;

  void _preparePrologEpilog(CompilerContext& cc) ASMJIT_NOTHROW;
  void _dumpFunction(CompilerContext& cc) ASMJIT_NOTHROW;
  void _emitProlog(CompilerContext& cc) ASMJIT_NOTHROW;
  void _emitEpilog(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Function-Call]
  // --------------------------------------------------------------------------

  //! @brief Reserve stack for calling other function and mark function as
  //! callee.
  void reserveStackForFunctionCall(int32_t size);

  // --------------------------------------------------------------------------
  // [Labels]
  // --------------------------------------------------------------------------

  //! @brief Get function entry label.
  //!
  //! Entry label can be used to call this function from another code that's
  //! being generated.
  inline const Label& getEntryLabel() const ASMJIT_NOTHROW { return _entryLabel; }

  //! @brief Get function exit label.
  //!
  //! Use exit label to jump to function epilog.
  inline const Label& getExitLabel() const ASMJIT_NOTHROW { return _exitLabel; }

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  //! @brief Set the _isEspAdjusted member to true.
  //!
  //! This method is used to tell compiler that the ESP/RSP must be adjusted in
  //! function prolog/epilog, because the stack is manipulated (usually caused
  //! by the function call, see @c ECall).
  inline void mustAdjustEsp() { _isEspAdjusted = true; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Function prototype.
  FunctionPrototype _functionPrototype;
  //! @brief Function arguments (variable IDs).
  VarData** _argumentVariables;
  //! @brief Function hints.
  uint32_t _hints[16];

  //! @brief Whether the function stack is aligned by 16-bytes by OS.
  //!
  //! This is always true for 64-bit mode and for linux.
  bool _isStackAlignedByOsTo16Bytes;

  //! @brief Whether the function stack (for variables) is aligned manually
  //! by function to 16-bytes.
  //!
  //! This makes sense only if _isStackAlignedByOsTo16Bytes is false and MOVDQA
  //! instruction or other SSE/SSE2 instructions are used to work with variable
  //! stored on the stack.
  //!
  //! Value is determined automatically by these factors, expectations are:
  //!
  //!   1. There is 16-byte wide variable which address was used (alloc, spill,
  //!      op).
  //!   2. Function can't be naked.
  bool _isStackAlignedByFnTo16Bytes;

  //! @brief Whether the function is using naked prolog / epilog
  //!
  //! Naked prolog / epilog means to omit saving and restoring EBP.
  bool _isNaked;

  //! @brief Whether the ESP register is adjusted by the stack size needed
  //! to save registers and function variables.
  //!
  //! Esp is adjusted by 'sub' instruction in prolog and by add function in
  //! epilog (only if function is not naked).
  bool _isEspAdjusted;

  //! @brief Whether another function is called from this function.
  //!
  //! If another function is called from this function, it's needed to prepare
  //! stack for it. If this member is true then it's likely that true will be
  //! also @c _isEspAdjusted one.
  bool _isCaller;

  //! @brief Whether to emit prolog / epilog sequence using push & pop
  //! instructions (the default).
  bool _pePushPop;

  //! @brief Whether to emit EMMS instruction in epilog (auto-detected).
  bool _emitEMMS;

  //! @brief Whether to emit SFence instruction in epilog (auto-detected).
  //!
  //! @note Combination of @c _emitSFence and @c _emitLFence will result in
  //! emitting mfence.
  bool _emitSFence;

  //! @brief Whether to emit LFence instruction in epilog (auto-detected).
  //!
  //! @note Combination of @c _emitSFence and @c _emitLFence will result in
  //! emitting mfence.
  bool _emitLFence;

  //! @brief Whether the function is finished using @c Compiler::endFunction().
  bool _finished;

  //! @brief Bitfield containing modified and preserved GP registers.
  uint32_t _modifiedAndPreservedGP;

  //! @brief Bitfield containing modified and preserved MM registers.
  uint32_t _modifiedAndPreservedMM;

  //! @brief Bitfield containing modified and preserved XMM registers.
  uint32_t _modifiedAndPreservedXMM;

  //! @brief ID mov movdqa instruction (@c INST_MOVDQA or @c INST_MOVDQU).
  //!
  //! The value is based on stack alignment. If it's guaranteed that stack
  //! is aligned to 16-bytes then @c INST_MOVDQA instruction is used, otherwise
  //! the @c INST_MOVDQU instruction is used for 16-byte mov.
  uint32_t _movDqaInstruction;

  //! @brief Prolog / epilog stack size for PUSH/POP sequences.
  int32_t _pePushPopStackSize;
  //! @brief Prolog / epilog stack size for MOV sequences.
  int32_t _peMovStackSize;
  //! @brief Prolog / epilog stack adjust size (to make it 16-byte aligned).
  int32_t _peAdjustStackSize;

  //! @brief Memory stack size (for all variables and temporary memory).
  int32_t _memStackSize;
  //! @brief Like @c _memStackSize, but aligned to 16-bytes.
  int32_t _memStackSize16;

  //! @brief Stack size needed to call other functions.
  int32_t _functionCallStackSize;

  //! @brief Function entry label.
  Label _entryLabel;
  //! @brief Function exit label.
  Label _exitLabel;

  //! @brief Function prolog emittable.
  EProlog* _prolog;
  //! @brief Function epilog emittable.
  EEpilog* _epilog;
  //! @brief Dummy emittable, signalizes end of function.
  EFunctionEnd* _end;

private:
  friend struct CompilerContext;
  friend struct CompilerCore;
  friend struct EProlog;
  friend struct EEpilog;
};

// ============================================================================
// [AsmJit::EProlog]
// ============================================================================

//! @brief Prolog emittable.
struct ASMJIT_API EProlog : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EProlog instance.
  EProlog(Compiler* c, EFunction* f) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EProlog instance.
  virtual ~EProlog() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Get function associated with this prolog.
  inline EFunction* getFunction() const ASMJIT_NOTHROW { return _function; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Prolog owner function.
  EFunction* _function;

private:
  friend struct CompilerCore;
  friend struct EFunction;
};

// ============================================================================
// [AsmJit::EEpilog]
// ============================================================================

//! @brief Epilog emittable.
struct ASMJIT_API EEpilog : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref EEpilog instance.
  EEpilog(Compiler* c, EFunction* f) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref EProlog instance.
  virtual ~EEpilog() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Get function associated with this epilog.
  inline EFunction* getFunction() const ASMJIT_NOTHROW { return _function; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Epilog owner function.
  EFunction* _function;

private:
  friend struct CompilerCore;
  friend struct EFunction;
};

// ============================================================================
// [AsmJit::ECall]
// ============================================================================

//! @brief Function call.
struct ASMJIT_API ECall : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref ECall instance.
  ECall(Compiler* c, EFunction* caller, const Operand* target) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref ECall instance.
  virtual ~ECall() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  virtual Emittable* translate(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  virtual int getMaxSize() const ASMJIT_NOTHROW;
  virtual bool _tryUnuseVar(VarData* v) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Internal]
  // --------------------------------------------------------------------------

protected:

  uint32_t _findTemporaryGpRegister(CompilerContext& cc) ASMJIT_NOTHROW;
  uint32_t _findTemporaryXmmRegister(CompilerContext& cc) ASMJIT_NOTHROW;

  VarData* _getOverlappingVariable(CompilerContext& cc,
    const FunctionPrototype::Argument& argType) const ASMJIT_NOTHROW;

  void _moveAllocatedVariableToStack(CompilerContext& cc,
    VarData* vdata, const FunctionPrototype::Argument& argType) ASMJIT_NOTHROW;

  void _moveSpilledVariableToStack(CompilerContext& cc,
    VarData* vdata, const FunctionPrototype::Argument& argType,
    uint32_t temporaryGpReg,
    uint32_t temporaryXmmReg) ASMJIT_NOTHROW;

  void _moveSrcVariableToRegister(CompilerContext& cc,
    VarData* vdata, const FunctionPrototype::Argument& argType) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Function Prototype (Calling Convention + Arguments) / Return Value]
  // --------------------------------------------------------------------------

public:

  //! @brief Get function prototype.
  inline const FunctionPrototype& getPrototype() const ASMJIT_NOTHROW { return _functionPrototype; }

  //! @brief Set function prototype.
  inline void setPrototype(uint32_t cconv, const FunctionDefinition& def) ASMJIT_NOTHROW
  {
    _setPrototype(
      cconv,
      def.getArguments(), 
      def.getArgumentsCount(), 
      def.getReturnValue());
  }

  //! @brief Set function prototype (internal).
  void _setPrototype(
    uint32_t callingConvention,
    const uint32_t* arguments,
    uint32_t argumentsCount,
    uint32_t returnValue) ASMJIT_NOTHROW;

  //! @brief Set function argument @a i to @a var.
  bool setArgument(uint32_t i, const BaseVar& var) ASMJIT_NOTHROW;
  //! @brief Set function argument @a i to @a imm.
  bool setArgument(uint32_t i, const Imm& imm) ASMJIT_NOTHROW;

  //! @brief Set return value to 
  bool setReturn(const Operand& first, const Operand& second = Operand()) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  //! @brief Get caller.
  inline EFunction* getCaller() const ASMJIT_NOTHROW { return _caller; }

  //! @brief Get operand (function address).
  inline Operand& getTarget() ASMJIT_NOTHROW { return _target; }
  //! @overload
  inline const Operand& getTarget() const ASMJIT_NOTHROW { return _target; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Function prototype.
  FunctionPrototype _functionPrototype;

  //! @brief Callee (the function that calls me).
  EFunction* _caller;

  //! @brief Arguments (operands).
  Operand* _args;

  //! @brief Operand (address of function, register, label, ...)
  Operand _target;

  //! @brief Return value (operands)
  Operand _ret[2];

  //! @brief Mask of GP registers used as function arguments.
  uint32_t _gpParams;
  //! @brief Mask of MM registers used as function arguments.
  uint32_t _mmParams;
  //! @brief Mask of XMM registers used as function arguments.
  uint32_t _xmmParams;

  //! @brief Variables count.
  uint32_t _variablesCount;

  //! @brief Variables (extracted from operands).
  VarCallRecord* _variables;
  //! @brief Argument index to @c VarCallRecord.
  VarCallRecord* _argumentToVarRecord[FUNC_MAX_ARGS];

private:
  friend struct CompilerCore;
};

// ============================================================================
// [AsmJit::ERet]
// ============================================================================

//! @brief Function return.
struct ASMJIT_API ERet : public Emittable
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref ERet instance.
  ERet(Compiler* c, EFunction* function, const Operand* first, const Operand* second) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref ERet instance.
  virtual ~ERet() ASMJIT_NOTHROW;

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

  //! @Brief Get function.
  inline EFunction* getFunction() ASMJIT_NOTHROW { return _function; }

  //! @brief Get operand (function address).
  inline Operand& getFirst() ASMJIT_NOTHROW { return _ret[0]; }
  //! @brief Get operand (function address).
  inline Operand& getSecond() ASMJIT_NOTHROW { return _ret[1]; }
  //! @overload
  inline const Operand& getFirst() const ASMJIT_NOTHROW { return _ret[0]; }
  //! @overload
  inline const Operand& getSecond() const ASMJIT_NOTHROW { return _ret[1]; }

  //! @brief Get whether jump to epilog have to be emitted.
  bool shouldEmitJumpToEpilog() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Function.
  EFunction* _function;
  //! @brief Return value (operands)
  Operand _ret[2];

private:
  friend struct CompilerCore;
};

// ============================================================================
// [AsmJit::CompilerContext]
// ============================================================================

//! @internal
//!
//! @brief Compiler context is used by @ref Compiler.
//!
//! Compiler context is used during compilation and normally developer doesn't
//! need access to it. The context is user per function (it's reset after each
//! function is generated).
struct ASMJIT_API CompilerContext
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref CompilerContext instance.
  CompilerContext(Compiler* compiler) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref CompilerContext instance.
  ~CompilerContext() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

  //! @brief Clear context, preparing it for next function generation.
  void _clear() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Register Allocator]
  // --------------------------------------------------------------------------

  //! @brief Allocate variable
  //!
  //! Calls @c allocGPVar, @c allocMMVar or @c allocXMMVar methods.
  void allocVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW;
  //! @brief Save variable.
  //!
  //! Calls @c saveGPVar, @c saveMMVar or @c saveXMMVar methods.
  void saveVar(VarData* vdata) ASMJIT_NOTHROW;
  //! @brief Spill variable.
  //!
  //! Calls @c spillGPVar, @c spillMMVar or @c spillXMMVar methods.
  void spillVar(VarData* vdata) ASMJIT_NOTHROW;
  //! @brief Unuse variable (didn't spill, just forget about it).
  void unuseVar(VarData* vdata, uint32_t toState) ASMJIT_NOTHROW;

  //! @brief Helper method that is called for each variable per emittable.
  inline void _unuseVarOnEndOfScope(Emittable* e, VarData* v)
  {
    if (v->lastEmittable == e)
      unuseVar(v, VARIABLE_STATE_UNUSED);
  }
  //! @overload
  inline void _unuseVarOnEndOfScope(Emittable* e, VarAllocRecord* rec)
  {
    VarData* v = rec->vdata;
    if (v->lastEmittable == e || (rec->vflags & VARIABLE_ALLOC_UNUSE_AFTER_USE))
      unuseVar(v, VARIABLE_STATE_UNUSED);
  }
  //! @overload
  inline void _unuseVarOnEndOfScope(Emittable* e, VarCallRecord* rec)
  {
    VarData* v = rec->vdata;
    if (v->lastEmittable == e || (rec->flags & VarCallRecord::FLAG_UNUSE_AFTER_USE))
      unuseVar(v, VARIABLE_STATE_UNUSED);
  }

  //! @brief Allocate variable (GP).
  void allocGPVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW;
  //! @brief Save variable (GP).
  void saveGPVar(VarData* vdata) ASMJIT_NOTHROW;
  //! @brief Spill variable (GP).
  void spillGPVar(VarData* vdata) ASMJIT_NOTHROW;

  //! @brief Allocate variable (MM).
  void allocMMVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW;
  //! @brief Save variable (MM).
  void saveMMVar(VarData* vdata) ASMJIT_NOTHROW;
  //! @brief Spill variable (MM).
  void spillMMVar(VarData* vdata) ASMJIT_NOTHROW;

  //! @brief Allocate variable (XMM).
  void allocXMMVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW;
  //! @brief Save variable (XMM).
  void saveXMMVar(VarData* vdata) ASMJIT_NOTHROW;
  //! @brief Spill variable (XMM).
  void spillXMMVar(VarData* vdata) ASMJIT_NOTHROW;

  //! @brief Emit load variable instruction(s).
  void emitLoadVar(VarData* vdata, uint32_t regIndex) ASMJIT_NOTHROW;
  //! @brief Emit save variable instruction(s).
  void emitSaveVar(VarData* vdata, uint32_t regIndex) ASMJIT_NOTHROW;

  //! @brief Emit move variable instruction(s).
  void emitMoveVar(VarData* vdata, uint32_t regIndex, uint32_t vflags) ASMJIT_NOTHROW;
  //! @brief Emit exchange variable instruction(s).
  void emitExchangeVar(VarData* vdata, uint32_t regIndex, uint32_t vflags, VarData* other) ASMJIT_NOTHROW;

  //! @brief Called each time a variable is alloceted.
  void _postAlloc(VarData* vdata, uint32_t vflags) ASMJIT_NOTHROW;
  //! @brief Marks variable home memory as used (must be called at least once
  //! for each variable that uses function local memory - stack).
  void _markMemoryUsed(VarData* vdata) ASMJIT_NOTHROW;

  Mem _getVarMem(VarData* vdata) ASMJIT_NOTHROW;

  VarData* _getSpillCandidateGP() ASMJIT_NOTHROW;
  VarData* _getSpillCandidateMM() ASMJIT_NOTHROW;
  VarData* _getSpillCandidateXMM() ASMJIT_NOTHROW;
  VarData* _getSpillCandidateGeneric(VarData** varArray, uint32_t count) ASMJIT_NOTHROW;

  inline bool _isActive(VarData* vdata) ASMJIT_NOTHROW { return vdata->nextActive != NULL; }
  void _addActive(VarData* vdata) ASMJIT_NOTHROW;
  void _freeActive(VarData* vdata) ASMJIT_NOTHROW;
  void _freeAllActive() ASMJIT_NOTHROW;

  void _allocatedVariable(VarData* vdata) ASMJIT_NOTHROW;

  inline void _allocatedGPRegister(uint32_t index) ASMJIT_NOTHROW { _state.usedGP |= Util::maskFromIndex(index); _modifiedGPRegisters |= Util::maskFromIndex(index); }
  inline void _allocatedMMRegister(uint32_t index) ASMJIT_NOTHROW { _state.usedMM |= Util::maskFromIndex(index); _modifiedMMRegisters |= Util::maskFromIndex(index); }
  inline void _allocatedXMMRegister(uint32_t index) ASMJIT_NOTHROW { _state.usedXMM |= Util::maskFromIndex(index); _modifiedXMMRegisters |= Util::maskFromIndex(index); }

  inline void _freedGPRegister(uint32_t index) ASMJIT_NOTHROW { _state.usedGP &= ~Util::maskFromIndex(index); }
  inline void _freedMMRegister(uint32_t index) ASMJIT_NOTHROW { _state.usedMM &= ~Util::maskFromIndex(index); }
  inline void _freedXMMRegister(uint32_t index) ASMJIT_NOTHROW { _state.usedXMM &= ~Util::maskFromIndex(index); }

  inline void _markGPRegisterModified(uint32_t index) ASMJIT_NOTHROW { _modifiedGPRegisters |= Util::maskFromIndex(index); }
  inline void _markMMRegisterModified(uint32_t index) ASMJIT_NOTHROW { _modifiedMMRegisters |= Util::maskFromIndex(index); }
  inline void _markXMMRegisterModified(uint32_t index) ASMJIT_NOTHROW { _modifiedXMMRegisters |= Util::maskFromIndex(index); }

  // TODO: Find code which uses this and improve.
  inline void _newRegisterHomeIndex(VarData* vdata, uint32_t idx)
  {
    if (vdata->homeRegisterIndex == INVALID_VALUE) vdata->homeRegisterIndex = idx;
    vdata->prefRegisterMask |= (1U << idx);
  }

  // TODO: Find code which uses this and improve.
  inline void _newRegisterHomeMask(VarData* vdata, uint32_t mask)
  {
    vdata->prefRegisterMask |= mask;
  }

  // --------------------------------------------------------------------------
  // [Operand Patcher]
  // --------------------------------------------------------------------------

  void translateOperands(Operand* operands, uint32_t count) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline Compiler* getCompiler() const ASMJIT_NOTHROW { return _compiler; }
  inline EFunction* getFunction() const ASMJIT_NOTHROW { return _function; }

  inline Emittable* getExtraBlock() const ASMJIT_NOTHROW { return _extraBlock; }
  inline void setExtraBlock(Emittable* e) ASMJIT_NOTHROW { _extraBlock = e; }

  // --------------------------------------------------------------------------
  // [Backward Code]
  // --------------------------------------------------------------------------

  void addBackwardCode(EJmp* from) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Forward Jump]
  // --------------------------------------------------------------------------

  void addForwardJump(EJmp* inst) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  StateData* _saveState() ASMJIT_NOTHROW;
  void _assignState(StateData* state) ASMJIT_NOTHROW;
  void _restoreState(StateData* state, uint32_t targetOffset = INVALID_VALUE) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Memory Allocator]
  // --------------------------------------------------------------------------

  VarMemBlock* _allocMemBlock(uint32_t size) ASMJIT_NOTHROW;
  void _freeMemBlock(VarMemBlock* mem) ASMJIT_NOTHROW;

  void _allocMemoryOperands() ASMJIT_NOTHROW;
  void _patchMemoryOperands(Emittable* start, Emittable* stop) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Zone memory manager.
  Zone _zone;

  //! @brief Compiler instance.
  Compiler* _compiler;
  //! @brief Function emittable.
  EFunction* _function;

  //! @brief Current active scope start emittable.
  Emittable* _start;
  //! @brief Current active scope end emittable.
  Emittable* _stop;
  //! @brief Emittable that is used to insert some code after the function body.
  Emittable* _extraBlock;

  //! @brief Current state (register allocator).
  StateData _state;
  //! @brief Link to circular double-linked list containing all active variables
  //! (for current state).
  VarData* _active;

  //! @brief Forward jumps (single linked list).
  ForwardJumpData* _forwardJumps;

  //! @brief Current offset, used in prepare() stage. Each emittable should increment it.
  uint32_t _currentOffset;

  //! @brief Whether current code is unreachable.
  uint32_t _unreachable;

  //! @brief Global modified GP registers mask (per function).
  uint32_t _modifiedGPRegisters;
  //! @brief Global modified MM registers mask (per function).
  uint32_t _modifiedMMRegisters;
  //! @brief Global modified XMM registers mask (per function).
  uint32_t _modifiedXMMRegisters;

  //! @brief Whether the EBP/RBP register can be used by register allocator.
  uint32_t _allocableEBP;

  //! @brief ESP adjust constant (changed during PUSH/POP or when using
  //! stack.
  int _adjustESP;

  //! @brief Function arguments base pointer (register).
  uint32_t _argumentsBaseReg;
  //! @brief Function arguments base offset.
  int32_t _argumentsBaseOffset;
  //! @brief Function arguments displacement.
  int32_t _argumentsActualDisp;

  //! @brief Function variables base pointer (register).
  uint32_t _variablesBaseReg;
  //! @brief Function variables base offset.
  int32_t _variablesBaseOffset;
  //! @brief Function variables displacement.
  int32_t _variablesActualDisp;

  //! @brief Used memory blocks (for variables, here is each created mem block
  //! that can be also in _memFree list).
  VarMemBlock* _memUsed;
  //! @brief Free memory blocks (freed, prepared for another allocation).
  VarMemBlock* _memFree;
  //! @brief Count of 4-byte memory blocks used by the function.
  uint32_t _mem4BlocksCount;
  //! @brief Count of 8-byte memory blocks used by the function.
  uint32_t _mem8BlocksCount;
  //! @brief Count of 16-byte memory blocks used by the function.
  uint32_t _mem16BlocksCount;
  //! @brief Count of total bytes of stack memory used by the function.
  uint32_t _memBytesTotal;

  //! @brief Whether to emit comments.
  bool _emitComments;

  //! @brief List of emittables which need to be translated. These emittables
  //! are filled by @c addBackwardCode().
  PodVector<EJmp*> _backCode;
  //! @brief Backward code position (starts at 0).
  sysuint_t _backPos;
};

// ============================================================================
// [AsmJit::CompilerUtil]
// ============================================================================

//! @brief Static class that contains utility methods.
struct ASMJIT_API CompilerUtil
{
  static bool isStack16ByteAligned();
};

// ============================================================================
// [AsmJit::CompilerCore]
// ============================================================================

//! @brief Compiler core.
//!
//! @sa @c AsmJit::Compiler.
struct ASMJIT_API CompilerCore
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new (empty) instance of @c Compiler.
  CompilerCore(CodeGenerator* codeGenerator) ASMJIT_NOTHROW;
  //! @brief Destroy @c Compiler instance.
  virtual ~CompilerCore() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Code Generator]
  // --------------------------------------------------------------------------

  //! @brief Get code generator.
  inline CodeGenerator* getCodeGenerator() const { return _codeGenerator; }

  // --------------------------------------------------------------------------
  // [Memory Management]
  // --------------------------------------------------------------------------

  //! @brief Get zone memory manager.
  inline Zone& getZone() { return _zone; }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  //! @brief Get logger.
  inline Logger* getLogger() const ASMJIT_NOTHROW { return _logger; }

  //! @brief Set logger to @a logger.
  virtual void setLogger(Logger* logger) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Error Handling]
  // --------------------------------------------------------------------------

  //! @brief Get error code.
  inline uint32_t getError() const ASMJIT_NOTHROW { return _error; }

  //! @brief Set error code.
  //!
  //! This method is virtual, because higher classes can use it to catch all
  //! errors.
  virtual void setError(uint32_t error) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Properties]
  // --------------------------------------------------------------------------

  //! @brief Get compiler property.
  uint32_t getProperty(uint32_t propertyId);
  //! @brief Set compiler property.
  void setProperty(uint32_t propertyId, uint32_t value);

  // --------------------------------------------------------------------------
  // [Buffer]
  // --------------------------------------------------------------------------

  //! @brief Clear everything, but not deallocate buffers.
  //!
  //! @note This method will destroy your code.
  void clear() ASMJIT_NOTHROW;

  //! @brief Free internal buffer, all emitters and NULL all pointers.
  //!
  //! @note This method will destroy your code.
  void free() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Emittables]
  // --------------------------------------------------------------------------

  //! @brief Get first emittable.
  inline Emittable* getFirstEmittable() const ASMJIT_NOTHROW { return _first; }

  //! @brief Get last emittable.
  inline Emittable* getLastEmittable() const ASMJIT_NOTHROW { return _last; }

  //! @brief Get current emittable.
  //!
  //! @note If this method return @c NULL, it means that nothing emitted yet.
  inline Emittable* getCurrentEmittable() const ASMJIT_NOTHROW { return _current; }

  //! @brief Set new current emittable and return previous one.
  Emittable* setCurrentEmittable(Emittable* current) ASMJIT_NOTHROW;

  //! @brief Add emittable after current and set current to @a emittable.
  void addEmittable(Emittable* emittable) ASMJIT_NOTHROW;

  //! @brief Add emittable after @a ref.
  void addEmittableAfter(Emittable* emittable, Emittable* ref) ASMJIT_NOTHROW;

  //! @brief Remove emittable (and if needed set current to previous).
  void removeEmittable(Emittable* emittable) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Comment]
  // --------------------------------------------------------------------------

  //! @brief Emit a single comment line that will be logged.
  //!
  //! Emitting comments are useful to log something. Because assembler can be
  //! generated from AST or other data structures, you may sometimes need to
  //! log data characteristics or statistics.
  //!
  //! @note Emitting comment is not directly sent to logger, but instead it's
  //! stored in @c AsmJit::Compiler and emitted when @c serialize() method is
  //! called. Each comment keeps correct order.
  void comment(const char* fmt, ...) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Function Builder]
  // --------------------------------------------------------------------------

  //! @brief Create a new function.
  //!
  //! @param cconv Calling convention to use (see @c CALL_CONV enum)
  //! @param params Function arguments prototype.
  //!
  //! This method is usually used as a first step when generating functions
  //! by @c Compiler. First parameter @a cconv specifies function calling
  //! convention to use. Second parameter @a params specifies function
  //! arguments. To create function arguments are used templates
  //! @c BuildFunction0<>, @c BuildFunction1<...>, @c BuildFunction2<...>,
  //! etc...
  //!
  //! Templates with BuildFunction prefix are used to generate argument IDs
  //! based on real C++ types. See next example how to generate function with
  //! two 32-bit integer arguments.
  //!
  //! @code
  //! // Building function using AsmJit::Compiler example.
  //!
  //! // Compiler instance
  //! Compiler c;
  //!
  //! // Begin of function (also emits function @c Prolog)
  //! c.newFunction(
  //!   // Default calling convention (32-bit cdecl or 64-bit for host OS)
  //!   CALL_CONV_DEFAULT,
  //!   // Using function builder to generate arguments list
  //!   BuildFunction2<int, int>());
  //!
  //! // End of function (also emits function @c Epilog)
  //! c.endFunction();
  //! @endcode
  //!
  //! You can see that building functions is really easy. Previous code snipped
  //! will generate code for function with two 32-bit integer arguments. You
  //! can access arguments by @c AsmJit::Function::argument() method. Arguments
  //! are indexed from 0 (like everything in C).
  //!
  //! @code
  //! // Accessing function arguments through AsmJit::Function example.
  //!
  //! // Compiler instance
  //! Compiler c;
  //!
  //! // Begin of function (also emits function @c Prolog)
  //! c.newFunction(
  //!   // Default calling convention (32-bit cdecl or 64-bit for host OS)
  //!   CALL_CONV_DEFAULT,
  //!   // Using function builder to generate arguments list
  //!   BuildFunction2<int, int>());
  //!
  //! // Arguments are like other variables, you need to reference them by
  //! // variable operands:
  //! GPVar a0 = c.argGP(0);
  //! GPVar a1 = c.argGP(1);
  //!
  //! // Use them.
  //! c.add(a0, a1);
  //!
  //! // End of function (emits function epilog and return)
  //! c.endFunction();
  //! @endcode
  //!
  //! Arguments are like variables. How to manipulate with variables is
  //! documented in @c AsmJit::Compiler, variables section.
  //!
  //! @note To get current function use @c currentFunction() method or save
  //! pointer to @c AsmJit::Function returned by @c AsmJit::Compiler::newFunction<>
  //! method. Recommended is to save the pointer.
  //!
  //! @sa @c BuildFunction0, @c BuildFunction1, @c BuildFunction2, ...
  inline EFunction* newFunction(uint32_t cconv, const FunctionDefinition& def) ASMJIT_NOTHROW
  {
    return newFunction_(
      cconv,
      def.getArguments(),
      def.getArgumentsCount(),
      def.getReturnValue());
  }

  //! @brief Create a new function (low level version).
  //!
  //! @param cconv Function calling convention (see @c AsmJit::CALL_CONV).
  //! @param args Function arguments (see @c AsmJit::VARIABLE_TYPE).
  //! @param count Arguments count.
  //!
  //! This method is internally called from @c newFunction() method and
  //! contains arguments thats used internally by @c AsmJit::Compiler.
  //!
  //! @note To get current function use @c currentFunction() method.
  EFunction* newFunction_(
    uint32_t cconv,
    const uint32_t* arguments,
    uint32_t argumentsCount,
    uint32_t returnValue) ASMJIT_NOTHROW;

  //! @brief Get current function.
  //!
  //! This method can be called within @c newFunction() and @c endFunction()
  //! block to get current function you are working with. It's recommended
  //! to store @c AsmJit::Function pointer returned by @c newFunction<> method,
  //! because this allows you in future implement function sections outside of
  //! function itself (yeah, this is possible!).
  inline EFunction* getFunction() const ASMJIT_NOTHROW { return _function; }

  //! @brief End of current function scope and all variables.
  EFunction* endFunction() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Memory Management]
  // --------------------------------------------------------------------------

  inline EInstruction* newInstruction(uint32_t code, Operand* operandsData, uint32_t operandsCount) ASMJIT_NOTHROW
  {
    if (code >= _INST_J_BEGIN && code <= _INST_J_END)
    {
      void* addr = _zone.zalloc(sizeof(EJmp));

      return new(addr) EJmp(
        reinterpret_cast<Compiler*>(this), code, operandsData, operandsCount);
    }
    else
    {
      void* addr = _zone.zalloc(sizeof(EInstruction) + operandsCount * sizeof(Operand));

      return new(addr) EInstruction(
        reinterpret_cast<Compiler*>(this), code, operandsData, operandsCount);
    }
  }

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  //! @brief Emit instruction with no operand.
  void _emitInstruction(uint32_t code) ASMJIT_NOTHROW;

  //! @brief Emit instruction with one operand.
  void _emitInstruction(uint32_t code, const Operand* o0) ASMJIT_NOTHROW;

  //! @brief Emit instruction with two operands.
  void _emitInstruction(uint32_t code, const Operand* o0, const Operand* o1) ASMJIT_NOTHROW;

  //! @brief Emit instruction with three operands.
  void _emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2) ASMJIT_NOTHROW;

  //! @brief Emit instruction with four operands (Special instructions).
  void _emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2, const Operand* o3) ASMJIT_NOTHROW;

  //! @brief Emit instruction with five operands (Special instructions).
  void _emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2, const Operand* o3, const Operand* o4) ASMJIT_NOTHROW;

  //! @brief Private method for emitting jcc.
  void _emitJcc(uint32_t code, const Label* label, uint32_t hint) ASMJIT_NOTHROW;

  //! @brief Private method for emitting function call.
  ECall* _emitCall(const Operand* o0) ASMJIT_NOTHROW;

  //! @brief Private method for returning a value from the function.
  void _emitReturn(const Operand* first, const Operand* second) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! @brief Embed data into instruction stream.
  void embed(const void* data, sysuint_t len) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  //! @brief Align target buffer to @a m bytes.
  //!
  //! Typical usage of this is to align labels at start of the inner loops.
  //!
  //! Inserts @c nop() instructions or CPU optimized NOPs.
  void align(uint32_t m) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Label]
  // --------------------------------------------------------------------------

  //! @brief Create and return new label.
  Label newLabel() ASMJIT_NOTHROW;

  //! @brief Bind label to the current offset.
  //!
  //! @note Label can be bound only once!
  void bind(const Label& label) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Variables]
  // --------------------------------------------------------------------------

  //! @internal
  //!
  //! @brief Create a new variable data.
  VarData* _newVarData(const char* name, uint32_t type, uint32_t size) ASMJIT_NOTHROW;

  //! @internal
  //!
  //! @brief Get variable data.
  inline VarData* _getVarData(uint32_t id) const ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(id != INVALID_VALUE);
    return _varData[id & OPERAND_ID_VALUE_MASK];
  }

  //! @brief Create a new general-purpose variable.
  GPVar newGP(uint32_t variableType = VARIABLE_TYPE_GPN, const char* name = NULL) ASMJIT_NOTHROW;
  //! @brief Get argument as general-purpose variable.
  GPVar argGP(uint32_t index) ASMJIT_NOTHROW;

  //! @brief Create a new MM variable.
  MMVar newMM(uint32_t variableType = VARIABLE_TYPE_MM, const char* name = NULL) ASMJIT_NOTHROW;
  //! @brief Get argument as MM variable.
  MMVar argMM(uint32_t index) ASMJIT_NOTHROW;

  //! @brief Create a new XMM variable.
  XMMVar newXMM(uint32_t variableType = VARIABLE_TYPE_XMM, const char* name = NULL) ASMJIT_NOTHROW;
  //! @brief Get argument as XMM variable.
  XMMVar argXMM(uint32_t index) ASMJIT_NOTHROW;

  //! @internal
  //!
  //! @brief Serialize variable hint.
  void _vhint(BaseVar& var, uint32_t hintId, uint32_t hintValue) ASMJIT_NOTHROW;

  //! @brief Alloc variable @a var.
  void alloc(BaseVar& var) ASMJIT_NOTHROW;
  //! @brief Alloc variable @a var using @a regIndex as a register index.
  void alloc(BaseVar& var, uint32_t regIndex) ASMJIT_NOTHROW;
  //! @brief Alloc variable @a var using @a reg as a demanded register.
  void alloc(BaseVar& var, const BaseReg& reg) ASMJIT_NOTHROW;
  //! @brief Spill variable @a var.
  void spill(BaseVar& var) ASMJIT_NOTHROW;
  //! @brief Save variable @a var if modified.
  void save(BaseVar& var) ASMJIT_NOTHROW;
  //! @brief Unuse variable @a var.
  void unuse(BaseVar& var) ASMJIT_NOTHROW;

  //! @brief Get memory home of variable @a var.
  void getMemoryHome(BaseVar& var, GPVar* home, int* displacement = NULL);

  //! @brief Set memory home of variable @a var.
  //!
  //! Default memory home location is on stack (ESP/RSP), but when needed the
  //! bebahior can be changed by this method.
  //!
  //! It is an error to chaining memory home locations. For example the given 
  //! code is invalid:
  //!
  //! @code
  //! Compiler c;
  //!
  //! ...
  //! GPVar v0 = c.newGP();
  //! GPVar v1 = c.newGP();
  //! GPVar v2 = c.newGP();
  //! GPVar v3 = c.newGP();
  //!
  //! c.setMemoryHome(v1, v0, 0); // Allowed, [v0] is memory home for v1.
  //! c.setMemoryHome(v2, v0, 4); // Allowed, [v0+4] is memory home for v2.
  //! c.setMemoryHome(v3, v2);    // CHAINING, NOT ALLOWED!
  //! @endcode
  void setMemoryHome(BaseVar& var, const GPVar& home, int displacement = 0);

  //! @brief Get priority of variable @a var.
  uint32_t getPriority(BaseVar& var) const ASMJIT_NOTHROW;
  //! @brief Set priority of variable @a var to @a priority.
  void setPriority(BaseVar& var, uint32_t priority) ASMJIT_NOTHROW;

  //! @brief Get save-on-unuse @a var property.
  bool getSaveOnUnuse(BaseVar& var) const ASMJIT_NOTHROW;
  //! @brief Set save-on-unuse @a var property to @a value.
  void setSaveOnUnuse(BaseVar& var, bool value) ASMJIT_NOTHROW;

  //! @brief Rename variable @a var to @a name.
  //!
  //! @note Only new name will appear in the logger.
  void rename(BaseVar& var, const char* name) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! @internal
  //!
  //! @brief Create a new @ref StateData instance.
  StateData* _newStateData(uint32_t memVarsCount) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Make]
  // --------------------------------------------------------------------------

  //! @brief Make is convenience method to make currently serialized code and
  //! return pointer to generated function.
  //!
  //! What you need is only to cast this pointer to your function type and call
  //! it. Note that if there was an error and calling @c getError() method not
  //! returns @c ERROR_NONE (zero) then this function always return @c NULL and
  //! error value remains the same.
  virtual void* make() ASMJIT_NOTHROW;

  //! @brief Method that will emit everything to @c Assembler instance @a a.
  virtual void serialize(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  //! @internal
  //!
  //! @brief Get target (emittable) from operand @a id (label id).
  inline ETarget* _getTarget(uint32_t id)
  {
    ASMJIT_ASSERT((id & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_LABEL);
    return _targetData[id & OPERAND_ID_VALUE_MASK];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:

  //! @brief Zone memory management.
  Zone _zone;

  //! @brief Code generator.
  CodeGenerator* _codeGenerator;

  //! @brief Logger.
  Logger* _logger;

  //! @brief Last error code.
  uint32_t _error;

  //! @brief Properties.
  uint32_t _properties;

  //! @brief Contains options for next emitted instruction, clear after each emit.
  uint32_t _emitOptions;

  //! @brief Whether compiler was finished the job (register allocator, etc...).
  uint32_t _finished;

  //! @brief First emittable.
  Emittable* _first;
  //! @brief Last emittable.
  Emittable* _last;
  //! @brief Current emittable.
  Emittable* _current;

  //! @brief Current function.
  EFunction* _function;

  //! @brief Label data.
  PodVector<ETarget*> _targetData;

  //! @brief Variable data.
  PodVector<VarData*> _varData;

  //! @brief Variable name id (used to generate unique names per function).
  int _varNameId;

  //! @brief Compiler context instance, only available after prepare().
  CompilerContext* _cc;

  friend struct BaseVar;
  friend struct CompilerContext;
  friend struct EFunction;
  friend struct EInstruction;
};

// ============================================================================
// [AsmJit::CompilerIntrinsics]
// ============================================================================

//! @brief Implementation of @c Compiler intrinsics.
//!
//! Methods in this class are implemented here, because we wan't to hide them
//! in shared libraries. These methods should be never exported by C++ compiler.
//!
//! @sa @c AsmJit::Compiler.
struct ASMJIT_HIDDEN CompilerIntrinsics : public CompilerCore
{
  // Special X86 instructions:
  // - cpuid,
  // - cbw, cwde, cdqe,
  // - cmpxchg
  // - cmpxchg8b, cmpxchg16b,
  // - daa, das,
  // - imul, mul, idiv, div,
  // - mov_ptr
  // - lahf, sahf
  // - maskmovq, maskmovdqu
  // - enter, leave
  // - ret
  // - monitor, mwait
  // - pop, popad, popfd, popfq,
  // - push, pushad, pushfd, pushfq
  // - rcl, rcr, rol, ror, sal, sar, shl, shr
  // - shld, shrd
  // - rdtsc. rdtscp
  // - lodsb, lodsd, lodsq, lodsw
  // - movsb, movsd, movsq, movsw
  // - stosb, stosd, stosq, stosw
  // - cmpsb, cmpsd, cmpsq, cmpsw
  // - scasb, scasd, scasq, scasw
  //
  // Special X87 instructions:
  // - fisttp

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create @c CompilerIntrinsics instance. Always use @c AsmJit::Compiler.
  inline CompilerIntrinsics(CodeGenerator* codeGenerator) ASMJIT_NOTHROW : 
    CompilerCore(codeGenerator)
  {
  }

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! @brief Add 8-bit integer data to the instuction stream.
  inline void db(uint8_t  x) ASMJIT_NOTHROW { embed(&x, 1); }
  //! @brief Add 16-bit integer data to the instuction stream.
  inline void dw(uint16_t x) ASMJIT_NOTHROW { embed(&x, 2); }
  //! @brief Add 32-bit integer data to the instuction stream.
  inline void dd(uint32_t x) ASMJIT_NOTHROW { embed(&x, 4); }
  //! @brief Add 64-bit integer data to the instuction stream.
  inline void dq(uint64_t x) ASMJIT_NOTHROW { embed(&x, 8); }

  //! @brief Add 8-bit integer data to the instuction stream.
  inline void dint8(int8_t x) ASMJIT_NOTHROW { embed(&x, sizeof(int8_t)); }
  //! @brief Add 8-bit integer data to the instuction stream.
  inline void duint8(uint8_t x) ASMJIT_NOTHROW { embed(&x, sizeof(uint8_t)); }

  //! @brief Add 16-bit integer data to the instuction stream.
  inline void dint16(int16_t x) ASMJIT_NOTHROW { embed(&x, sizeof(int16_t)); }
  //! @brief Add 16-bit integer data to the instuction stream.
  inline void duint16(uint16_t x) ASMJIT_NOTHROW { embed(&x, sizeof(uint16_t)); }

  //! @brief Add 32-bit integer data to the instuction stream.
  inline void dint32(int32_t x) ASMJIT_NOTHROW { embed(&x, sizeof(int32_t)); }
  //! @brief Add 32-bit integer data to the instuction stream.
  inline void duint32(uint32_t x) ASMJIT_NOTHROW { embed(&x, sizeof(uint32_t)); }

  //! @brief Add 64-bit integer data to the instuction stream.
  inline void dint64(int64_t x) ASMJIT_NOTHROW { embed(&x, sizeof(int64_t)); }
  //! @brief Add 64-bit integer data to the instuction stream.
  inline void duint64(uint64_t x) ASMJIT_NOTHROW { embed(&x, sizeof(uint64_t)); }

  //! @brief Add system-integer data to the instuction stream.
  inline void dsysint(sysint_t x) ASMJIT_NOTHROW { embed(&x, sizeof(sysint_t)); }
  //! @brief Add system-integer data to the instuction stream.
  inline void dsysuint(sysuint_t x) ASMJIT_NOTHROW { embed(&x, sizeof(sysuint_t)); }

  //! @brief Add float data to the instuction stream.
  inline void dfloat(float x) ASMJIT_NOTHROW { embed(&x, sizeof(float)); }
  //! @brief Add double data to the instuction stream.
  inline void ddouble(double x) ASMJIT_NOTHROW { embed(&x, sizeof(double)); }

  //! @brief Add pointer data to the instuction stream.
  inline void dptr(void* x) ASMJIT_NOTHROW { embed(&x, sizeof(void*)); }

  //! @brief Add MM data to the instuction stream.
  inline void dmm(const MMData& x) ASMJIT_NOTHROW { embed(&x, sizeof(MMData)); }
  //! @brief Add XMM data to the instuction stream.
  inline void dxmm(const XMMData& x) ASMJIT_NOTHROW { embed(&x, sizeof(XMMData)); }

  //! @brief Add data to the instuction stream.
  inline void data(const void* data, sysuint_t size) ASMJIT_NOTHROW { embed(data, size); }

  //! @brief Add data in a given structure instance to the instuction stream.
  template<typename T>
  inline void dstruct(const T& x) ASMJIT_NOTHROW { embed(&x, sizeof(T)); }

  // --------------------------------------------------------------------------
  // [Custom Instructions]
  // --------------------------------------------------------------------------

  // These emitters are used by custom compiler code (register alloc / spill,
  // prolog / epilog generator, ...).

  inline void emit(uint32_t code) ASMJIT_NOTHROW
  {
    _emitInstruction(code);
  }

  inline void emit(uint32_t code, const Operand& o0) ASMJIT_NOTHROW
  {
    _emitInstruction(code, &o0);
  }

  inline void emit(uint32_t code, const Operand& o0, const Operand& o1) ASMJIT_NOTHROW
  {
    _emitInstruction(code, &o0, &o1);
  }

  inline void emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) ASMJIT_NOTHROW
  {
    _emitInstruction(code, &o0, &o1, &o2);
  }

  // --------------------------------------------------------------------------
  // [X86 Instructions]
  // --------------------------------------------------------------------------

  //! @brief Add with Carry.
  inline void adc(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }

  //! @brief Add.
  inline void add(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }

  //! @brief Logical And.
  inline void and_(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }

  //! @brief Bit Scan Forward.
  inline void bsf(const GPVar& dst, const GPVar& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSF, &dst, &src);
  }
  //! @brief Bit Scan Forward.
  inline void bsf(const GPVar& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSF, &dst, &src);
  }

  //! @brief Bit Scan Reverse.
  inline void bsr(const GPVar& dst, const GPVar& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSR, &dst, &src);
  }
  //! @brief Bit Scan Reverse.
  inline void bsr(const GPVar& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSR, &dst, &src);
  }

  //! @brief Byte swap (32-bit or 64-bit registers only) (i486).
  inline void bswap(const GPVar& dst)
  {
    // ASMJIT_ASSERT(dst.getRegType() == REG_GPD || dst.getRegType() == REG_GPQ);
    _emitInstruction(INST_BSWAP, &dst);
  }

  //! @brief Bit test.
  inline void bt(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }
  //! @brief Bit test.
  inline void bt(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }
  //! @brief Bit test.
  inline void bt(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }
  //! @brief Bit test.
  inline void bt(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }

  //! @brief Bit test and complement.
  inline void btc(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }
  //! @brief Bit test and complement.
  inline void btc(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }
  //! @brief Bit test and complement.
  inline void btc(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }
  //! @brief Bit test and complement.
  inline void btc(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }

  //! @brief Bit test and reset.
  inline void btr(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }
  //! @brief Bit test and reset.
  inline void btr(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }
  //! @brief Bit test and reset.
  inline void btr(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }
  //! @brief Bit test and reset.
  inline void btr(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }

  //! @brief Bit test and set.
  inline void bts(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }
  //! @brief Bit test and set.
  inline void bts(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }
  //! @brief Bit test and set.
  inline void bts(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }
  //! @brief Bit test and set.
  inline void bts(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }

  //! @brief Call Procedure.
  inline ECall* call(const GPVar& dst)
  {
    return _emitCall(&dst);
  }
  //! @brief Call Procedure.
  inline ECall* call(const Mem& dst)
  {
    return _emitCall(&dst);
  }
  //! @brief Call Procedure.
  inline ECall* call(const Imm& dst)
  {
    return _emitCall(&dst);
  }
  //! @brief Call Procedure.
  //! @overload
  inline ECall* call(void* dst)
  {
    Imm imm((sysint_t)dst);
    return _emitCall(&imm);
  }

  //! @brief Call Procedure.
  inline ECall* call(const Label& label)
  {
    return _emitCall(&label);
  }

  //! @brief Convert Byte to Word (Sign Extend).
  inline void cbw(const GPVar& dst)
  {
    _emitInstruction(INST_CBW, &dst);
  }

  //! @brief Convert Word to DWord (Sign Extend).
  inline void cwde(const GPVar& dst)
  {
    _emitInstruction(INST_CWDE, &dst);
  }

#if defined(ASMJIT_X64)
  //! @brief Convert DWord to QWord (Sign Extend).
  inline void cdqe(const GPVar& dst)
  {
    _emitInstruction(INST_CDQE, &dst);
  }
#endif // ASMJIT_X64

  //! @brief Clear Carry flag
  //!
  //! This instruction clears the CF flag in the EFLAGS register.
  inline void clc()
  {
    _emitInstruction(INST_CLC);
  }

  //! @brief Clear Direction flag
  //!
  //! This instruction clears the DF flag in the EFLAGS register.
  inline void cld()
  {
    _emitInstruction(INST_CLD);
  }

  //! @brief Complement Carry Flag.
  //!
  //! This instruction complements the CF flag in the EFLAGS register.
  //! (CF = NOT CF)
  inline void cmc()
  {
    _emitInstruction(INST_CMC);
  }

  //! @brief Conditional Move.
  inline void cmov(CONDITION cc, const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(ConditionToInstruction::toCMovCC(cc), &dst, &src);
  }

  //! @brief Conditional Move.
  inline void cmov(CONDITION cc, const GPVar& dst, const Mem& src)
  {
    _emitInstruction(ConditionToInstruction::toCMovCC(cc), &dst, &src);
  }

  //! @brief Conditional Move.
  inline void cmova  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVA  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmova  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVA  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovae (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVAE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovae (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVAE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovb  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVB  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovb  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVB  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovbe (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVBE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovbe (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVBE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovc  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVC  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovc  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVC  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmove  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVE  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmove  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVE  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovg  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVG  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovg  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVG  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovge (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVGE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovge (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVGE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovl  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVL  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovl  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVL  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovle (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVLE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovle (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVLE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovna (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNA , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovna (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNA , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnae(const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNAE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnae(const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNAE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnb (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNB , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnb (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNB , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnbe(const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNBE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnbe(const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNBE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnc (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNC , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnc (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNC , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovne (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovne (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovng (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNG , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovng (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNG , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnge(const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNGE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnge(const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNGE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnl (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNL , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnl (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNL , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnle(const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNLE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnle(const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNLE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovno (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovno (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnp (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNP , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnp (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNP , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovns (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNS , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovns (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNS , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnz (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVNZ , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnz (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVNZ , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovo  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVO  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovo  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVO  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovp  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVP  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovp  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVP  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpe (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVPE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpe (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVPE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpo (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVPO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpo (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVPO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovs  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVS  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovs  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVS  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovz  (const GPVar& dst, const GPVar& src) { _emitInstruction(INST_CMOVZ  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovz  (const GPVar& dst, const Mem& src)   { _emitInstruction(INST_CMOVZ  , &dst, &src); }

  //! @brief Compare Two Operands.
  inline void cmp(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }

  //! @brief Compare and Exchange (i486).
  inline void cmpxchg(const GPVar cmp_1_eax, const GPVar& cmp_2, const GPVar& src)
  {
    ASMJIT_ASSERT(cmp_1_eax.getId() != src.getId());
    _emitInstruction(INST_CMPXCHG, &cmp_1_eax, &cmp_2, &src);
  }
  //! @brief Compare and Exchange (i486).
  inline void cmpxchg(const GPVar cmp_1_eax, const Mem& cmp_2, const GPVar& src)
  {
    ASMJIT_ASSERT(cmp_1_eax.getId() != src.getId());
    _emitInstruction(INST_CMPXCHG, &cmp_1_eax, &cmp_2, &src);
  }

  //! @brief Compares the 64-bit value in EDX:EAX with the memory operand (Pentium).
  //!
  //! If the values are equal, then this instruction stores the 64-bit value
  //! in ECX:EBX into the memory operand and sets the zero flag. Otherwise,
  //! this instruction copies the 64-bit memory operand into the EDX:EAX
  //! registers and clears the zero flag.
  inline void cmpxchg8b(
    const GPVar& cmp_edx, const GPVar& cmp_eax,
    const GPVar& cmp_ecx, const GPVar& cmp_ebx,
    const Mem& dst)
  {
    ASMJIT_ASSERT(cmp_edx.getId() != cmp_eax.getId() &&
                  cmp_eax.getId() != cmp_ecx.getId() &&
                  cmp_ecx.getId() != cmp_ebx.getId());

    _emitInstruction(INST_CMPXCHG8B, &cmp_edx, &cmp_eax, &cmp_ecx, &cmp_ebx, &dst);
  }

#if defined(ASMJIT_X64)
  //! @brief Compares the 128-bit value in RDX:RAX with the memory operand (X64).
  //!
  //! If the values are equal, then this instruction stores the 128-bit value
  //! in RCX:RBX into the memory operand and sets the zero flag. Otherwise,
  //! this instruction copies the 128-bit memory operand into the RDX:RAX
  //! registers and clears the zero flag.
  inline void cmpxchg16b(
    const GPVar& cmp_edx, const GPVar& cmp_eax,
    const GPVar& cmp_ecx, const GPVar& cmp_ebx,
    const Mem& dst)
  {
    ASMJIT_ASSERT(cmp_edx.getId() != cmp_eax.getId() &&
                  cmp_eax.getId() != cmp_ecx.getId() &&
                  cmp_ecx.getId() != cmp_ebx.getId());

    _emitInstruction(INST_CMPXCHG16B, &cmp_edx, &cmp_eax, &cmp_ecx, &cmp_ebx, &dst);
  }
#endif // ASMJIT_X64

  //! @brief CPU Identification (i486).
  inline void cpuid(
    const GPVar& inout_eax,
    const GPVar& out_ebx,
    const GPVar& out_ecx,
    const GPVar& out_edx)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(inout_eax.getId() != out_ebx.getId() &&
                  out_ebx.getId() != out_ecx.getId() &&
                  out_ecx.getId() != out_edx.getId());

    _emitInstruction(INST_CPUID, &inout_eax, &out_ebx, &out_ecx, &out_edx);
  }

#if defined(ASMJIT_X86)
  inline void daa(const GPVar& dst)
  {
    _emitInstruction(INST_DAA, &dst);
  }
#endif // ASMJIT_X86

#if defined(ASMJIT_X86)
  inline void das(const GPVar& dst)
  {
    _emitInstruction(INST_DAS, &dst);
  }
#endif // ASMJIT_X86

  //! @brief Decrement by 1.
  //! @note This instruction can be slower than sub(dst, 1)
  inline void dec(const GPVar& dst)
  {
    _emitInstruction(INST_DEC, &dst);
  }
  //! @brief Decrement by 1.
  //! @note This instruction can be slower than sub(dst, 1)
  inline void dec(const Mem& dst)
  {
    _emitInstruction(INST_DEC, &dst);
  }

  //! @brief Unsigned divide.
  //!
  //! This instruction divides (unsigned) the value in the AL, AX, or EAX
  //! register by the source operand and stores the result in the AX,
  //! DX:AX, or EDX:EAX registers.
  inline void div(const GPVar& dst_rem, const GPVar& dst_quot, const GPVar& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_rem.getId() != dst_quot.getId());

    _emitInstruction(INST_DIV, &dst_rem, &dst_quot, &src);
  }
  //! @brief Unsigned divide.
  //! @overload
  inline void div(const GPVar& dst_rem, const GPVar& dst_quot, const Mem& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_rem.getId() != dst_quot.getId());

    _emitInstruction(INST_DIV, &dst_rem, &dst_quot, &src);
  }

#if ASMJIT_NOT_SUPPORTED_BY_COMPILER
  //! @brief Make Stack Frame for Procedure Parameters.
  inline void enter(const Imm& imm16, const Imm& imm8)
  {
    _emitInstruction(INST_ENTER, &imm16, &imm8);
  }
#endif // ASMJIT_NOT_SUPPORTED_BY_COMPILER

  //! @brief Signed divide.
  //!
  //! This instruction divides (signed) the value in the AL, AX, or EAX
  //! register by the source operand and stores the result in the AX,
  //! DX:AX, or EDX:EAX registers.
  inline void idiv(const GPVar& dst_rem, const GPVar& dst_quot, const GPVar& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_rem.getId() != dst_quot.getId());

    _emitInstruction(INST_IDIV, &dst_rem, &dst_quot, &src);
  }
  //! @brief Signed divide.
  //! @overload
  inline void idiv(const GPVar& dst_rem, const GPVar& dst_quot, const Mem& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_rem.getId() != dst_quot.getId());

    _emitInstruction(INST_IDIV, &dst_rem, &dst_quot, &src);
  }

  //! @brief Signed multiply.
  //!
  //! [dst_lo:dst_hi] = dst_hi * src.
  inline void imul(const GPVar& dst_hi, const GPVar& dst_lo, const GPVar& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_hi.getId() != dst_lo.getId());

    _emitInstruction(INST_IMUL, &dst_hi, &dst_lo, &src);
  }
  //! @overload
  inline void imul(const GPVar& dst_hi, const GPVar& dst_lo, const Mem& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_hi.getId() != dst_lo.getId());

    _emitInstruction(INST_IMUL, &dst_hi, &dst_lo, &src);
  }

  //! @brief Signed multiply.
  //!
  //! Destination operand (the first operand) is multiplied by the source
  //! operand (second operand). The destination operand is a general-purpose
  //! register and the source operand is an immediate value, a general-purpose
  //! register, or a memory location. The product is then stored in the
  //! destination operand location.
  inline void imul(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_IMUL, &dst, &src);
  }
  //! @brief Signed multiply.
  //! @overload
  inline void imul(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_IMUL, &dst, &src);
  }
  //! @brief Signed multiply.
  //! @overload
  inline void imul(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_IMUL, &dst, &src);
  }

  //! @brief Signed multiply.
  //!
  //! source operand (which can be a general-purpose register or a memory
  //! location) is multiplied by the second source operand (an immediate
  //! value). The product is then stored in the destination operand
  //! (a general-purpose register).
  inline void imul(const GPVar& dst, const GPVar& src, const Imm& imm)
  {
    _emitInstruction(INST_IMUL, &dst, &src, &imm);
  }
  //! @overload
  inline void imul(const GPVar& dst, const Mem& src, const Imm& imm)
  {
    _emitInstruction(INST_IMUL, &dst, &src, &imm);
  }

  //! @brief Increment by 1.
  //! @note This instruction can be slower than add(dst, 1)
  inline void inc(const GPVar& dst)
  {
    _emitInstruction(INST_INC, &dst);
  }
  //! @brief Increment by 1.
  //! @note This instruction can be slower than add(dst, 1)
  inline void inc(const Mem& dst)
  {
    _emitInstruction(INST_INC, &dst);
  }

  //! @brief Interrupt 3 - trap to debugger.
  inline void int3()
  {
    _emitInstruction(INST_INT3);
  }

  //! @brief Jump to label @a label if condition @a cc is met.
  //!
  //! This instruction checks the state of one or more of the status flags in
  //! the EFLAGS register (CF, OF, PF, SF, and ZF) and, if the flags are in the
  //! specified state (condition), performs a jump to the target instruction
  //! specified by the destination operand. A condition code (cc) is associated
  //! with each instruction to indicate the condition being tested for. If the
  //! condition is not satisfied, the jump is not performed and execution
  //! continues with the instruction following the Jcc instruction.
  inline void j(CONDITION cc, const Label& label, uint32_t hint = HINT_NONE)
  {
    _emitJcc(ConditionToInstruction::toJCC(cc), &label, hint);
  }

  //! @brief Jump to label @a label if condition is met.
  inline void ja  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JA  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jae (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JAE , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jb  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JB  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jbe (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JBE , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jc  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JC  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void je  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JE  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jg  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JG  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jge (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JGE , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jl  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JL  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jle (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JLE , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jna (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNA , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnae(const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNAE, &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnb (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNB , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnbe(const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNBE, &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnc (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNC , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jne (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNE , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jng (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNG , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnge(const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNGE, &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnl (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNL , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnle(const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNLE, &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jno (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNO , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnp (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNP , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jns (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNS , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jnz (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JNZ , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jo  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JO  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jp  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JP  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jpe (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JPE , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jpo (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JPO , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void js  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JS  , &label, hint); }
  //! @brief Jump to label @a label if condition is met.
  inline void jz  (const Label& label, uint32_t hint = HINT_NONE) { _emitJcc(INST_JZ  , &label, hint); }

  //! @brief Jump.
  //! @overload
  inline void jmp(const GPVar& dst)
  {
    _emitInstruction(INST_JMP, &dst);
  }
  //! @brief Jump.
  //! @overload
  inline void jmp(const Mem& dst)
  {
    _emitInstruction(INST_JMP, &dst);
  }
  //! @brief Jump.
  //! @overload
  inline void jmp(const Imm& dst)
  {
    _emitInstruction(INST_JMP, &dst);
  }

  //! @brief Jump.
  //! @overload
  inline void jmp(void* dst)
  {
    Imm imm((sysint_t)dst);
    _emitInstruction(INST_JMP, &imm);
  }

  //! @brief Jump.
  //!
  //! This instruction transfers program control to a different point
  //! in the instruction stream without recording return information.
  //! The destination (target) operand specifies the label of the
  //! instruction being jumped to.
  inline void jmp(const Label& label)
  {
    _emitInstruction(INST_JMP, &label);
  }
  //! @brief Load Effective Address
  //!
  //! This instruction computes the effective address of the second
  //! operand (the source operand) and stores it in the first operand
  //! (destination operand). The source operand is a memory address
  //! (offset part) specified with one of the processors addressing modes.
  //! The destination operand is a general-purpose register.
  inline void lea(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_LEA, &dst, &src);
  }

#if ASMJIT_NOT_SUPPORTED_BY_COMPILER
  //! @brief High Level Procedure Exit.
  inline void leave()
  {
    _emitInstruction(INST_LEAVE);
  }
#endif // ASMJIT_NOT_SUPPORTED_BY_COMPILER

  //! @brief Move.
  //!
  //! This instruction copies the second operand (source operand) to the first
  //! operand (destination operand). The source operand can be an immediate
  //! value, general-purpose register, segment register, or memory location.
  //! The destination register can be a general-purpose register, segment
  //! register, or memory location. Both operands must be the same size, which
  //! can be a byte, a word, or a DWORD.
  //!
  //! @note To move MMX or SSE registers to/from GP registers or memory, use
  //! corresponding functions: @c movd(), @c movq(), etc. Passing MMX or SSE
  //! registers to @c mov() is illegal.
  inline void mov(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }

  //! @brief Move from segment register.
  //! @overload.
  inline void mov(const GPVar& dst, const SegmentReg& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  
  //! @brief Move from segment register.
  //! @overload.
  inline void mov(const Mem& dst, const SegmentReg& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }

  //! @brief Move to segment register.
  //! @overload.
  inline void mov(const SegmentReg& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }

  //! @brief Move to segment register.
  //! @overload.
  inline void mov(const SegmentReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }

  //! @brief Move byte, word, dword or qword from absolute address @a src to
  //! AL, AX, EAX or RAX register.
  inline void mov_ptr(const GPVar& dst, void* src)
  {
    Imm imm((sysint_t)src);
    _emitInstruction(INST_MOV_PTR, &dst, &imm);
  }

  //! @brief Move byte, word, dword or qword from AL, AX, EAX or RAX register
  //! to absolute address @a dst.
  inline void mov_ptr(void* dst, const GPVar& src)
  {
    Imm imm((sysint_t)dst);
    _emitInstruction(INST_MOV_PTR, &imm, &src);
  }

  //! @brief Move with Sign-Extension.
  //!
  //! This instruction copies the contents of the source operand (register
  //! or memory location) to the destination operand (register) and sign
  //! extends the value to 16, 32 or 64-bits.
  //!
  //! @sa movsxd().
  void movsx(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVSX, &dst, &src);
  }
  //! @brief Move with Sign-Extension.
  //! @overload
  void movsx(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSX, &dst, &src);
  }

#if defined(ASMJIT_X64)
  //! @brief Move DWord to QWord with sign-extension.
  inline void movsxd(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVSXD, &dst, &src);
  }
  //! @brief Move DWord to QWord with sign-extension.
  //! @overload
  inline void movsxd(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSXD, &dst, &src);
  }
#endif // ASMJIT_X64

  //! @brief Move with Zero-Extend.
  //!
  //! This instruction copies the contents of the source operand (register
  //! or memory location) to the destination operand (register) and zero
  //! extends the value to 16 or 32-bits. The size of the converted value
  //! depends on the operand-size attribute.
  inline void movzx(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVZX, &dst, &src);
  }
  //! @brief Move with Zero-Extend.
  //! @brief Overload
  inline void movzx(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVZX, &dst, &src);
  }

  //! @brief Unsigned multiply.
  //!
  //! Source operand (in a general-purpose register or memory location)
  //! is multiplied by the value in the AL, AX, or EAX register (depending
  //! on the operand size) and the product is stored in the AX, DX:AX, or
  //! EDX:EAX registers, respectively.
  inline void mul(const GPVar& dst_hi, const GPVar& dst_lo, const GPVar& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_hi.getId() != dst_lo.getId());

    _emitInstruction(INST_MUL, &dst_hi, &dst_lo, &src);
  }
  //! @brief Unsigned multiply.
  //! @overload
  inline void mul(const GPVar& dst_hi, const GPVar& dst_lo, const Mem& src)
  {
    // Destination variables must be different.
    ASMJIT_ASSERT(dst_hi.getId() != dst_lo.getId());

    _emitInstruction(INST_MUL, &dst_hi, &dst_lo, &src);
  }

  //! @brief Two's Complement Negation.
  inline void neg(const GPVar& dst)
  {
    _emitInstruction(INST_NEG, &dst);
  }
  //! @brief Two's Complement Negation.
  inline void neg(const Mem& dst)
  {
    _emitInstruction(INST_NEG, &dst);
  }

  //! @brief No Operation.
  //!
  //! This instruction performs no operation. This instruction is a one-byte
  //! instruction that takes up space in the instruction stream but does not
  //! affect the machine context, except the EIP register. The NOP instruction
  //! is an alias mnemonic for the XCHG (E)AX, (E)AX instruction.
  inline void nop()
  {
    _emitInstruction(INST_NOP);
  }

  //! @brief One's Complement Negation.
  inline void not_(const GPVar& dst)
  {
    _emitInstruction(INST_NOT, &dst);
  }
  //! @brief One's Complement Negation.
  inline void not_(const Mem& dst)
  {
    _emitInstruction(INST_NOT, &dst);
  }

  //! @brief Logical Inclusive OR.
  inline void or_(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }

  //! @brief Pop a Value from the Stack.
  //!
  //! This instruction loads the value from the top of the stack to the location
  //! specified with the destination operand and then increments the stack pointer.
  //! The destination operand can be a general purpose register, memory location,
  //! or segment register.
  inline void pop(const GPVar& dst)
  {
    _emitInstruction(INST_POP, &dst);
  }

  inline void pop(const Mem& dst)
  {
    ASMJIT_ASSERT(dst.getSize() == 2 || dst.getSize() == sizeof(sysint_t));
    _emitInstruction(INST_POP, &dst);
  }

#if defined(ASMJIT_X86)
  //! @brief Pop All General-Purpose Registers.
  //!
  //! Pop EDI, ESI, EBP, EBX, EDX, ECX, and EAX.
  inline void popad()
  {
    _emitInstruction(INST_POPAD);
  }
#endif // ASMJIT_X86

  //! @brief Pop Stack into EFLAGS Register (32-bit or 64-bit).
  inline void popf()
  {
#if defined(ASMJIT_X86)
    popfd();
#else
    popfq();
#endif
  }

#if defined(ASMJIT_X86)
  //! @brief Pop Stack into EFLAGS Register (32-bit).
  inline void popfd() { _emitInstruction(INST_POPFD); }
#else
  //! @brief Pop Stack into EFLAGS Register (64-bit).
  inline void popfq() { _emitInstruction(INST_POPFQ); }
#endif

  //! @brief Push WORD/DWORD/QWORD Onto the Stack.
  //!
  //! @note 32-bit architecture pushed DWORD while 64-bit
  //! pushes QWORD. 64-bit mode not provides instruction to
  //! push 32-bit register/memory.
  inline void push(const GPVar& src)
  {
    _emitInstruction(INST_PUSH, &src);
  }
  //! @brief Push WORD/DWORD/QWORD Onto the Stack.
  inline void push(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == sizeof(sysint_t));
    _emitInstruction(INST_PUSH, &src);
  }
  //! @brief Push WORD/DWORD/QWORD Onto the Stack.
  inline void push(const Imm& src)
  {
    _emitInstruction(INST_PUSH, &src);
  }

#if defined(ASMJIT_X86)
  //! @brief Push All General-Purpose Registers.
  //!
  //! Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI.
  inline void pushad()
  {
    _emitInstruction(INST_PUSHAD);
  }
#endif // ASMJIT_X86

  //! @brief Push EFLAGS Register (32-bit or 64-bit) onto the Stack.
  inline void pushf()
  {
#if defined(ASMJIT_X86)
    pushfd();
#else
    pushfq();
#endif
  }

#if defined(ASMJIT_X86)
  //! @brief Push EFLAGS Register (32-bit) onto the Stack.
  inline void pushfd() { _emitInstruction(INST_PUSHFD); }
#else
  //! @brief Push EFLAGS Register (64-bit) onto the Stack.
  inline void pushfq() { _emitInstruction(INST_PUSHFQ); }
#endif // ASMJIT_X86

  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rcl(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_RCL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  inline void rcl(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_RCL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rcl(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_RCL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  inline void rcl(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_RCL, &dst, &src);
  }

  //! @brief Rotate Bits Right.
  //! @note @a src register can be only @c cl.
  inline void rcr(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void rcr(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  //! @note @a src register can be only @c cl.
  inline void rcr(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void rcr(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }

  //! @brief Read Time-Stamp Counter (Pentium).
  inline void rdtsc(const GPVar& dst_edx, const GPVar& dst_eax)
  {
    // Destination registers must be different.
    ASMJIT_ASSERT(dst_edx.getId() != dst_eax.getId());

    _emitInstruction(INST_RDTSC, &dst_edx, &dst_eax);
  }

  //! @brief Read Time-Stamp Counter and Processor ID (New).
  inline void rdtscp(const GPVar& dst_edx, const GPVar& dst_eax, const GPVar& dst_ecx)
  {
    // Destination registers must be different.
    ASMJIT_ASSERT(dst_edx.getId() != dst_eax.getId() &&
                  dst_eax.getId() != dst_ecx.getId());

    _emitInstruction(INST_RDTSCP, &dst_edx, &dst_eax, &dst_ecx);
  }

  //! @brief Load ECX/RCX BYTEs from DS:[ESI/RSI] to AL.
  inline void rep_lodsb(const GPVar& dst_val, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=EAX,RAX, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_val.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_LODSB, &dst_val, &src_addr, &cnt_ecx);
  }

  //! @brief Load ECX/RCX DWORDs from DS:[ESI/RSI] to EAX.
  inline void rep_lodsd(const GPVar& dst_val, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=EAX,RAX, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_val.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_LODSD, &dst_val, &src_addr, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Load ECX/RCX QWORDs from DS:[ESI/RSI] to RAX.
  inline void rep_lodsq(const GPVar& dst_val, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=EAX,RAX, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_val.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_LODSQ, &dst_val, &src_addr, &cnt_ecx);
  }
#endif // ASMJIT_X64
  //! @brief Load ECX/RCX WORDs from DS:[ESI/RSI] to AX.
  inline void rep_lodsw(const GPVar& dst_val, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=EAX,RAX, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_val.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_LODSW, &dst_val, &src_addr, &cnt_ecx);
  }

  //! @brief Move ECX/RCX BYTEs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsb(const GPVar& dst_addr, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_MOVSB, &dst_addr, &src_addr, &cnt_ecx);
  }

  //! @brief Move ECX/RCX DWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsd(const GPVar& dst_addr, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_MOVSD, &dst_addr, &src_addr, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Move ECX/RCX QWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsq(const GPVar& dst_addr, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_MOVSQ, &dst_addr, &src_addr, &cnt_ecx);
  }
#endif // ASMJIT_X64

  //! @brief Move ECX/RCX WORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsw(const GPVar& dst_addr, const GPVar& src_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=DS:ESI/RSI, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_addr.getId() && src_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_MOVSW, &dst_addr, &src_addr, &cnt_ecx);
  }

  //! @brief Fill ECX/RCX BYTEs at ES:[EDI/RDI] with AL.
  inline void rep_stosb(const GPVar& dst_addr, const GPVar& src_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=EAX/RAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_val.getId() && src_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_STOSB, &dst_addr, &src_val, &cnt_ecx);
  }

  //! @brief Fill ECX/RCX DWORDs at ES:[EDI/RDI] with EAX.
  inline void rep_stosd(const GPVar& dst_addr, const GPVar& src_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=EAX/RAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_val.getId() && src_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_STOSD, &dst_addr, &src_val, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Fill ECX/RCX QWORDs at ES:[EDI/RDI] with RAX.
  inline void rep_stosq(const GPVar& dst_addr, const GPVar& src_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=EAX/RAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_val.getId() && src_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_STOSQ, &dst_addr, &src_val, &cnt_ecx);
  }
#endif // ASMJIT_X64

  //! @brief Fill ECX/RCX WORDs at ES:[EDI/RDI] with AX.
  inline void rep_stosw(const GPVar& dst_addr, const GPVar& src_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to dst=ES:EDI,RDI, src=EAX/RAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(dst_addr.getId() != src_val.getId() && src_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REP_STOSW, &dst_addr, &src_val, &cnt_ecx);
  }

  //! @brief Repeated find nonmatching BYTEs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsb(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_CMPSB, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }

  //! @brief Repeated find nonmatching DWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsd(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_CMPSD, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Repeated find nonmatching QWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsq(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_CMPSQ, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }
#endif // ASMJIT_X64

  //! @brief Repeated find nonmatching WORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsw(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_CMPSW, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }

  //! @brief Find non-AL BYTE starting at ES:[EDI/RDI].
  inline void repe_scasb(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=AL, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_SCASB, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }

  //! @brief Find non-EAX DWORD starting at ES:[EDI/RDI].
  inline void repe_scasd(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=EAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_SCASD, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Find non-RAX QWORD starting at ES:[EDI/RDI].
  inline void repe_scasq(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=RAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_SCASQ, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }
#endif // ASMJIT_X64

  //! @brief Find non-AX WORD starting at ES:[EDI/RDI].
  inline void repe_scasw(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=AX, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPE_SCASW, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }

  //! @brief Find matching BYTEs in [RDI] and [RSI].
  inline void repne_cmpsb(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_CMPSB, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }

  //! @brief Find matching DWORDs in [RDI] and [RSI].
  inline void repne_cmpsd(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_CMPSD, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Find matching QWORDs in [RDI] and [RSI].
  inline void repne_cmpsq(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_CMPSQ, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }
#endif // ASMJIT_X64

  //! @brief Find matching WORDs in [RDI] and [RSI].
  inline void repne_cmpsw(const GPVar& cmp1_addr, const GPVar& cmp2_addr, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, cmp2=ES:[EDI/RDI], cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_addr.getId() && cmp2_addr.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_CMPSW, &cmp1_addr, &cmp2_addr, &cnt_ecx);
  }

  //! @brief Find AL, starting at ES:[EDI/RDI].
  inline void repne_scasb(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=AL, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_SCASB, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }

  //! @brief Find EAX, starting at ES:[EDI/RDI].
  inline void repne_scasd(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=EAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_SCASD, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }

#if defined(ASMJIT_X64)
  //! @brief Find RAX, starting at ES:[EDI/RDI].
  inline void repne_scasq(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=RAX, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_SCASQ, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }
#endif // ASMJIT_X64

  //! @brief Find AX, starting at ES:[EDI/RDI].
  inline void repne_scasw(const GPVar& cmp1_addr, const GPVar& cmp2_val, const GPVar& cnt_ecx)
  {
    // All registers must be unique, they will be reallocated to cmp1=ES:EDI,RDI, src=AX, cnt=ECX/RCX.
    ASMJIT_ASSERT(cmp1_addr.getId() != cmp2_val.getId() && cmp2_val.getId() != cnt_ecx.getId());

    _emitInstruction(INST_REPNE_SCASW, &cmp1_addr, &cmp2_val, &cnt_ecx);
  }

  //! @brief Return from Procedure.
  inline void ret()
  {
    _emitReturn(NULL, NULL);
  }

  //! @brief Return from Procedure.
  inline void ret(const GPVar& first)
  {
    _emitReturn(&first, NULL);
  }

  //! @brief Return from Procedure.
  inline void ret(const GPVar& first, const GPVar& second)
  {
    _emitReturn(&first, &second);
  }

  //! @brief Return from Procedure.
  inline void ret(const XMMVar& first)
  {
    _emitReturn(&first, NULL);
  }

  //! @brief Return from Procedure.
  inline void ret(const XMMVar& first, const XMMVar& second)
  {
    _emitReturn(&first, &second);
  }

  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rol(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_ROL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  inline void rol(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_ROL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rol(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_ROL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  inline void rol(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ROL, &dst, &src);
  }

  //! @brief Rotate Bits Right.
  //! @note @a src register can be only @c cl.
  inline void ror(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void ror(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  //! @note @a src register can be only @c cl.
  inline void ror(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void ror(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }

#if defined(ASMJIT_X86)
  //! @brief Store @a var (allocated to AH/AX/EAX/RAX) into Flags.
  inline void sahf(const GPVar& var)
  {
    _emitInstruction(INST_SAHF, &var);
  }
#endif // ASMJIT_X86

  //! @brief Integer subtraction with borrow.
  inline void sbb(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }

  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void sal(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_SAL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  inline void sal(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_SAL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void sal(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_SAL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  inline void sal(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SAL, &dst, &src);
  }

  //! @brief Shift Bits Right.
  //! @note @a src register can be only @c cl.
  inline void sar(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void sar(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  //! @note @a src register can be only @c cl.
  inline void sar(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void sar(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }

  //! @brief Set Byte on Condition.
  inline void set(CONDITION cc, const GPVar& dst)
  {
    ASMJIT_ASSERT(dst.getSize() == 1);
    _emitInstruction(ConditionToInstruction::toSetCC(cc), &dst);
  }

  //! @brief Set Byte on Condition.
  inline void set(CONDITION cc, const Mem& dst)
  {
    ASMJIT_ASSERT(dst.getSize() <= 1);
    _emitInstruction(ConditionToInstruction::toSetCC(cc), &dst);
  }

  //! @brief Set Byte on Condition.
  inline void seta  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETA  , &dst); }
  //! @brief Set Byte on Condition.
  inline void seta  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETA  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setae (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETAE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setae (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETAE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setb  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETB  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setb  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETB  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setbe (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETBE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setbe (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETBE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setc  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETC  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setc  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETC  , &dst); }
  //! @brief Set Byte on Condition.
  inline void sete  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETE  , &dst); }
  //! @brief Set Byte on Condition.
  inline void sete  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETE  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setg  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETG  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setg  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETG  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setge (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETGE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setge (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETGE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setl  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETL  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setl  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETL  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setle (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETLE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setle (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETLE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setna (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNA , &dst); }
  //! @brief Set Byte on Condition.
  inline void setna (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNA , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnae(const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNAE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnae(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNAE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnb (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNB , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnb (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNB , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnbe(const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNBE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnbe(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNBE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnc (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNC , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnc (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNC , &dst); }
  //! @brief Set Byte on Condition.
  inline void setne (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setne (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setng (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNG , &dst); }
  //! @brief Set Byte on Condition.
  inline void setng (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNG , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnge(const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNGE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnge(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNGE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnl (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNL , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnl (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNL , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnle(const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNLE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnle(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNLE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setno (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNO , &dst); }
  //! @brief Set Byte on Condition.
  inline void setno (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNO , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnp (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNP , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnp (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNP , &dst); }
  //! @brief Set Byte on Condition.
  inline void setns (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNS , &dst); }
  //! @brief Set Byte on Condition.
  inline void setns (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNS , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnz (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNZ , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnz (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNZ , &dst); }
  //! @brief Set Byte on Condition.
  inline void seto  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETO  , &dst); }
  //! @brief Set Byte on Condition.
  inline void seto  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETO  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setp  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETP  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setp  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETP  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpe (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETPE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpe (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETPE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpo (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETPO , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpo (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETPO , &dst); }
  //! @brief Set Byte on Condition.
  inline void sets  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETS  , &dst); }
  //! @brief Set Byte on Condition.
  inline void sets  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETS  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setz  (const GPVar& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETZ  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setz  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETZ  , &dst); }

  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void shl(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_SHL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  inline void shl(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_SHL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void shl(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_SHL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  inline void shl(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SHL, &dst, &src);
  }

  //! @brief Shift Bits Right.
  //! @note @a src register can be only @c cl.
  inline void shr(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_SHR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void shr(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_SHR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  //! @note @a src register can be only @c cl.
  inline void shr(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_SHR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void shr(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SHR, &dst, &src);
  }

  //! @brief Double Precision Shift Left.
  //! @note src2 register can be only @c cl register.
  inline void shld(const GPVar& dst, const GPVar& src1, const GPVar& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Left.
  inline void shld(const GPVar& dst, const GPVar& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Left.
  //! @note src2 register can be only @c cl register.
  inline void shld(const Mem& dst, const GPVar& src1, const GPVar& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Left.
  inline void shld(const Mem& dst, const GPVar& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }

  //! @brief Double Precision Shift Right.
  //! @note src2 register can be only @c cl register.
  inline void shrd(const GPVar& dst, const GPVar& src1, const GPVar& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Right.
  inline void shrd(const GPVar& dst, const GPVar& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Right.
  //! @note src2 register can be only @c cl register.
  inline void shrd(const Mem& dst, const GPVar& src1, const GPVar& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Right.
  inline void shrd(const Mem& dst, const GPVar& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }

  //! @brief Set Carry Flag to 1.
  inline void stc()
  {
    _emitInstruction(INST_STC);
  }

  //! @brief Set Direction Flag to 1.
  inline void std()
  {
    _emitInstruction(INST_STD);
  }

  //! @brief Subtract.
  inline void sub(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }

  //! @brief Logical Compare.
  inline void test(const GPVar& op1, const GPVar& op2)
  {
    _emitInstruction(INST_TEST, &op1, &op2);
  }
  //! @brief Logical Compare.
  inline void test(const GPVar& op1, const Imm& op2)
  {
    _emitInstruction(INST_TEST, &op1, &op2);
  }
  //! @brief Logical Compare.
  inline void test(const Mem& op1, const GPVar& op2)
  {
    _emitInstruction(INST_TEST, &op1, &op2);
  }
  //! @brief Logical Compare.
  inline void test(const Mem& op1, const Imm& op2)
  {
    _emitInstruction(INST_TEST, &op1, &op2);
  }

  //! @brief Undefined instruction - Raise invalid opcode exception.
  inline void ud2()
  {
    _emitInstruction(INST_UD2);
  }

  //! @brief Exchange and Add.
  inline void xadd(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_XADD, &dst, &src);
  }
  //! @brief Exchange and Add.
  inline void xadd(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_XADD, &dst, &src);
  }

  //! @brief Exchange Register/Memory with Register.
  inline void xchg(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_XCHG, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xchg(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_XCHG, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xchg(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_XCHG, &src, &dst);
  }

  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const GPVar& dst, const Imm& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [MMX]
  // --------------------------------------------------------------------------

  //! @brief Empty MMX state.
  inline void emms()
  {
    _emitInstruction(INST_EMMS);
  }

  //! @brief Move DWord (MMX).
  inline void movd(const Mem& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord (MMX).
  inline void movd(const GPVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord (MMX).
  inline void movd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord (MMX).
  inline void movd(const MMVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }

  //! @brief Move QWord (MMX).
  inline void movq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
  //! @brief Move QWord (MMX).
  inline void movq(const Mem& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (MMX).
  inline void movq(const GPVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif
  //! @brief Move QWord (MMX).
  inline void movq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (MMX).
  inline void movq(const MMVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif

  //! @brief Pack with Signed Saturation (MMX).
  inline void packsswb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (MMX).
  inline void packsswb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }

  //! @brief Pack with Signed Saturation (MMX).
  inline void packssdw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (MMX).
  inline void packssdw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }

  //! @brief Pack with Unsigned Saturation (MMX).
  inline void packuswb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }
  //! @brief Pack with Unsigned Saturation (MMX).
  inline void packuswb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }

  //! @brief Packed BYTE Add (MMX).
  inline void paddb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }
  //! @brief Packed BYTE Add (MMX).
  inline void paddb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }

  //! @brief Packed WORD Add (MMX).
  inline void paddw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }
  //! @brief Packed WORD Add (MMX).
  inline void paddw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }

  //! @brief Packed DWORD Add (MMX).
  inline void paddd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }
  //! @brief Packed DWORD Add (MMX).
  inline void paddd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }

  //! @brief Packed Add with Saturation (MMX).
  inline void paddsb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }
  //! @brief Packed Add with Saturation (MMX).
  inline void paddsb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }

  //! @brief Packed Add with Saturation (MMX).
  inline void paddsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }
  //! @brief Packed Add with Saturation (MMX).
  inline void paddsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }

  //! @brief Logical AND (MMX).
  inline void pand(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }
  //! @brief Logical AND (MMX).
  inline void pand(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }

  //! @brief Logical AND Not (MMX).
  inline void pandn(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }
  //! @brief Logical AND Not (MMX).
  inline void pandn(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }

  //! @brief Packed Compare for Equal (BYTES) (MMX).
  inline void pcmpeqb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }
  //! @brief Packed Compare for Equal (BYTES) (MMX).
  inline void pcmpeqb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }

  //! @brief Packed Compare for Equal (WORDS) (MMX).
  inline void pcmpeqw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }
  //! @brief Packed Compare for Equal (WORDS) (MMX).
  inline void pcmpeqw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }

  //! @brief Packed Compare for Equal (DWORDS) (MMX).
  inline void pcmpeqd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }
  //! @brief Packed Compare for Equal (DWORDS) (MMX).
  inline void pcmpeqd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (BYTES) (MMX).
  inline void pcmpgtb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (BYTES) (MMX).
  inline void pcmpgtb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (WORDS) (MMX).
  inline void pcmpgtw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (WORDS) (MMX).
  inline void pcmpgtw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (DWORDS) (MMX).
  inline void pcmpgtd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (DWORDS) (MMX).
  inline void pcmpgtd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }

  //! @brief Packed Multiply High (MMX).
  inline void pmulhw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }
  //! @brief Packed Multiply High (MMX).
  inline void pmulhw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }

  //! @brief Packed Multiply Low (MMX).
  inline void pmullw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }
  //! @brief Packed Multiply Low (MMX).
  inline void pmullw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }

  //! @brief Bitwise Logical OR (MMX).
  inline void por(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }
  //! @brief Bitwise Logical OR (MMX).
  inline void por(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }

  //! @brief Packed Multiply and Add (MMX).
  inline void pmaddwd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }
  //! @brief Packed Multiply and Add (MMX).
  inline void pmaddwd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (MMX).
  inline void pslld(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void pslld(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void pslld(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (MMX).
  inline void psllq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllq(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (MMX).
  inline void psllw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllw(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psrad(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psrad(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psrad(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psraw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psraw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psraw(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (MMX).
  inline void psrld(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrld(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrld(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlq(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlw(const MMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }

  //! @brief Packed Subtract (MMX).
  inline void psubb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }
  //! @brief Packed Subtract (MMX).
  inline void psubb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }

  //! @brief Packed Subtract (MMX).
  inline void psubw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }
  //! @brief Packed Subtract (MMX).
  inline void psubw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }

  //! @brief Packed Subtract (MMX).
  inline void psubd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }
  //! @brief Packed Subtract (MMX).
  inline void psubd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhbw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhbw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhwd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhwd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhdq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhdq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklbw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklbw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklwd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklwd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckldq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckldq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }

  //! @brief Bitwise Exclusive OR (MMX).
  inline void pxor(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }
  //! @brief Bitwise Exclusive OR (MMX).
  inline void pxor(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [3dNow]
  // --------------------------------------------------------------------------

  //! @brief Faster EMMS (3dNow!).
  //!
  //! @note Use only for early AMD processors where is only 3dNow! or SSE. If
  //! CPU contains SSE2, it's better to use @c emms() ( @c femms() is mapped
  //! to @c emms() ).
  inline void femms()
  {
    _emitInstruction(INST_FEMMS);
  }

  //! @brief Packed SP-FP to Integer Convert (3dNow!).
  inline void pf2id(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PF2ID, &dst, &src);
  }
  //! @brief Packed SP-FP to Integer Convert (3dNow!).
  inline void pf2id(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PF2ID, &dst, &src);
  }

  //! @brief  Packed SP-FP to Integer Word Convert (3dNow!).
  inline void pf2iw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PF2IW, &dst, &src);
  }
  //! @brief  Packed SP-FP to Integer Word Convert (3dNow!).
  inline void pf2iw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PF2IW, &dst, &src);
  }

  //! @brief Packed SP-FP Accumulate (3dNow!).
  inline void pfacc(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFACC, &dst, &src);
  }
  //! @brief Packed SP-FP Accumulate (3dNow!).
  inline void pfacc(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFACC, &dst, &src);
  }

  //! @brief Packed SP-FP Addition (3dNow!).
  inline void pfadd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFADD, &dst, &src);
  }
  //! @brief Packed SP-FP Addition (3dNow!).
  inline void pfadd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFADD, &dst, &src);
  }

  //! @brief Packed SP-FP Compare - dst == src (3dNow!).
  inline void pfcmpeq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFCMPEQ, &dst, &src);
  }
  //! @brief Packed SP-FP Compare - dst == src (3dNow!).
  inline void pfcmpeq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFCMPEQ, &dst, &src);
  }

  //! @brief Packed SP-FP Compare - dst >= src (3dNow!).
  inline void pfcmpge(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFCMPGE, &dst, &src);
  }
  //! @brief Packed SP-FP Compare - dst >= src (3dNow!).
  inline void pfcmpge(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFCMPGE, &dst, &src);
  }

  //! @brief Packed SP-FP Compare - dst > src (3dNow!).
  inline void pfcmpgt(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFCMPGT, &dst, &src);
  }
  //! @brief Packed SP-FP Compare - dst > src (3dNow!).
  inline void pfcmpgt(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFCMPGT, &dst, &src);
  }

  //! @brief Packed SP-FP Maximum (3dNow!).
  inline void pfmax(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFMAX, &dst, &src);
  }
  //! @brief Packed SP-FP Maximum (3dNow!).
  inline void pfmax(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFMAX, &dst, &src);
  }

  //! @brief Packed SP-FP Minimum (3dNow!).
  inline void pfmin(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFMIN, &dst, &src);
  }
  //! @brief Packed SP-FP Minimum (3dNow!).
  inline void pfmin(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFMIN, &dst, &src);
  }

  //! @brief Packed SP-FP Multiply (3dNow!).
  inline void pfmul(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFMUL, &dst, &src);
  }
  //! @brief Packed SP-FP Multiply (3dNow!).
  inline void pfmul(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFMUL, &dst, &src);
  }

  //! @brief Packed SP-FP Negative Accumulate (3dNow!).
  inline void pfnacc(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFNACC, &dst, &src);
  }
  //! @brief Packed SP-FP Negative Accumulate (3dNow!).
  inline void pfnacc(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFNACC, &dst, &src);
  }

  //! @brief Packed SP-FP Mixed Accumulate (3dNow!).
  inline void pfpnaxx(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFPNACC, &dst, &src);
  }
  //! @brief Packed SP-FP Mixed Accumulate (3dNow!).
  inline void pfpnacc(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFPNACC, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal Approximation (3dNow!).
  inline void pfrcp(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFRCP, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal Approximation (3dNow!).
  inline void pfrcp(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRCP, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal, First Iteration Step (3dNow!).
  inline void pfrcpit1(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFRCPIT1, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal, First Iteration Step (3dNow!).
  inline void pfrcpit1(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRCPIT1, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal, Second Iteration Step (3dNow!).
  inline void pfrcpit2(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFRCPIT2, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal, Second Iteration Step (3dNow!).
  inline void pfrcpit2(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRCPIT2, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal Square Root, First Iteration Step (3dNow!).
  inline void pfrsqit1(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFRSQIT1, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal Square Root, First Iteration Step (3dNow!).
  inline void pfrsqit1(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRSQIT1, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal Square Root Approximation (3dNow!).
  inline void pfrsqrt(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFRSQRT, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal Square Root Approximation (3dNow!).
  inline void pfrsqrt(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRSQRT, &dst, &src);
  }

  //! @brief Packed SP-FP Subtract (3dNow!).
  inline void pfsub(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFSUB, &dst, &src);
  }
  //! @brief Packed SP-FP Subtract (3dNow!).
  inline void pfsub(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFSUB, &dst, &src);
  }

  //! @brief Packed SP-FP Reverse Subtract (3dNow!).
  inline void pfsubr(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PFSUBR, &dst, &src);
  }
  //! @brief Packed SP-FP Reverse Subtract (3dNow!).
  inline void pfsubr(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PFSUBR, &dst, &src);
  }

  //! @brief Packed DWords to SP-FP (3dNow!).
  inline void pi2fd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PI2FD, &dst, &src);
  }
  //! @brief Packed DWords to SP-FP (3dNow!).
  inline void pi2fd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PI2FD, &dst, &src);
  }

  //! @brief Packed Words to SP-FP (3dNow!).
  inline void pi2fw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PI2FW, &dst, &src);
  }
  //! @brief Packed Words to SP-FP (3dNow!).
  inline void pi2fw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PI2FW, &dst, &src);
  }

  //! @brief Packed swap DWord (3dNow!)
  inline void pswapd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSWAPD, &dst, &src);
  }
  //! @brief Packed swap DWord (3dNow!)
  inline void pswapd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSWAPD, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [SSE]
  // --------------------------------------------------------------------------

  //! @brief Packed SP-FP Add (SSE).
  inline void addps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ADDPS, &dst, &src);
  }
  //! @brief Packed SP-FP Add (SSE).
  inline void addps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Add (SSE).
  inline void addss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ADDSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Add (SSE).
  inline void addss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSS, &dst, &src);
  }

  //! @brief Bit-wise Logical And Not For SP-FP (SSE).
  inline void andnps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ANDNPS, &dst, &src);
  }
  //! @brief Bit-wise Logical And Not For SP-FP (SSE).
  inline void andnps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDNPS, &dst, &src);
  }

  //! @brief Bit-wise Logical And For SP-FP (SSE).
  inline void andps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ANDPS, &dst, &src);
  }
  //! @brief Bit-wise Logical And For SP-FP (SSE).
  inline void andps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDPS, &dst, &src);
  }

  //! @brief Packed SP-FP Compare (SSE).
  inline void cmpps(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPS, &dst, &src, &imm8);
  }
  //! @brief Packed SP-FP Compare (SSE).
  inline void cmpps(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPS, &dst, &src, &imm8);
  }

  //! @brief Compare Scalar SP-FP Values (SSE).
  inline void cmpss(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSS, &dst, &src, &imm8);
  }
  //! @brief Compare Scalar SP-FP Values (SSE).
  inline void cmpss(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSS, &dst, &src, &imm8);
  }

  //! @brief Scalar Ordered SP-FP Compare and Set EFLAGS (SSE).
  inline void comiss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_COMISS, &dst, &src);
  }
  //! @brief Scalar Ordered SP-FP Compare and Set EFLAGS (SSE).
  inline void comiss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_COMISS, &dst, &src);
  }

  //! @brief Packed Signed INT32 to Packed SP-FP Conversion (SSE).
  inline void cvtpi2ps(const XMMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_CVTPI2PS, &dst, &src);
  }
  //! @brief Packed Signed INT32 to Packed SP-FP Conversion (SSE).
  inline void cvtpi2ps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPI2PS, &dst, &src);
  }

  //! @brief Packed SP-FP to Packed INT32 Conversion (SSE).
  inline void cvtps2pi(const MMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTPS2PI, &dst, &src);
  }
  //! @brief Packed SP-FP to Packed INT32 Conversion (SSE).
  inline void cvtps2pi(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPS2PI, &dst, &src);
  }

  //! @brief Scalar Signed INT32 to SP-FP Conversion (SSE).
  inline void cvtsi2ss(const XMMVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_CVTSI2SS, &dst, &src);
  }
  //! @brief Scalar Signed INT32 to SP-FP Conversion (SSE).
  inline void cvtsi2ss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSI2SS, &dst, &src);
  }

  //! @brief Scalar SP-FP to Signed INT32 Conversion (SSE).
  inline void cvtss2si(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTSS2SI, &dst, &src);
  }
  //! @brief Scalar SP-FP to Signed INT32 Conversion (SSE).
  inline void cvtss2si(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSS2SI, &dst, &src);
  }

  //! @brief Packed SP-FP to Packed INT32 Conversion (truncate) (SSE).
  inline void cvttps2pi(const MMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTTPS2PI, &dst, &src);
  }
  //! @brief Packed SP-FP to Packed INT32 Conversion (truncate) (SSE).
  inline void cvttps2pi(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPS2PI, &dst, &src);
  }

  //! @brief Scalar SP-FP to Signed INT32 Conversion (truncate) (SSE).
  inline void cvttss2si(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTTSS2SI, &dst, &src);
  }
  //! @brief Scalar SP-FP to Signed INT32 Conversion (truncate) (SSE).
  inline void cvttss2si(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTSS2SI, &dst, &src);
  }

  //! @brief Packed SP-FP Divide (SSE).
  inline void divps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_DIVPS, &dst, &src);
  }
  //! @brief Packed SP-FP Divide (SSE).
  inline void divps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_DIVPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Divide (SSE).
  inline void divss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_DIVSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Divide (SSE).
  inline void divss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_DIVSS, &dst, &src);
  }

  //! @brief Load Streaming SIMD Extension Control/Status (SSE).
  inline void ldmxcsr(const Mem& src)
  {
    _emitInstruction(INST_LDMXCSR, &src);
  }

  //! @brief Byte Mask Write (SSE).
  //!
  //! @note The default memory location is specified by DS:EDI.
  inline void maskmovq(const GPVar& dst_ptr, const MMVar& data, const MMVar& mask)
  {
    _emitInstruction(INST_MASKMOVQ, &dst_ptr, &data, &mask);
  }

  //! @brief Packed SP-FP Maximum (SSE).
  inline void maxps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MAXPS, &dst, &src);
  }
  //! @brief Packed SP-FP Maximum (SSE).
  inline void maxps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Maximum (SSE).
  inline void maxss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MAXSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Maximum (SSE).
  inline void maxss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXSS, &dst, &src);
  }

  //! @brief Packed SP-FP Minimum (SSE).
  inline void minps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MINPS, &dst, &src);
  }
  //! @brief Packed SP-FP Minimum (SSE).
  inline void minps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MINPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Minimum (SSE).
  inline void minss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MINSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Minimum (SSE).
  inline void minss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MINSS, &dst, &src);
  }

  //! @brief Move Aligned Packed SP-FP Values (SSE).
  inline void movaps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVAPS, &dst, &src);
  }
  //! @brief Move Aligned Packed SP-FP Values (SSE).
  inline void movaps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVAPS, &dst, &src);
  }

  //! @brief Move Aligned Packed SP-FP Values (SSE).
  inline void movaps(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVAPS, &dst, &src);
  }

  //! @brief Move DWord.
  inline void movd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord.
  inline void movd(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord.
  inline void movd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord.
  inline void movd(const XMMVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }

  //! @brief Move QWord (SSE).
  inline void movq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
  //! @brief Move QWord (SSE).
  inline void movq(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (SSE).
  inline void movq(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif // ASMJIT_X64
  //! @brief Move QWord (SSE).
  inline void movq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (SSE).
  inline void movq(const XMMVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif // ASMJIT_X64

  //! @brief Move 64 Bits Non Temporal (SSE).
  inline void movntq(const Mem& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVNTQ, &dst, &src);
  }

  //! @brief High to Low Packed SP-FP (SSE).
  inline void movhlps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVHLPS, &dst, &src);
  }

  //! @brief Move High Packed SP-FP (SSE).
  inline void movhps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVHPS, &dst, &src);
  }

  //! @brief Move High Packed SP-FP (SSE).
  inline void movhps(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVHPS, &dst, &src);
  }

  //! @brief Move Low to High Packed SP-FP (SSE).
  inline void movlhps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVLHPS, &dst, &src);
  }

  //! @brief Move Low Packed SP-FP (SSE).
  inline void movlps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVLPS, &dst, &src);
  }

  //! @brief Move Low Packed SP-FP (SSE).
  inline void movlps(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVLPS, &dst, &src);
  }

  //! @brief Move Aligned Four Packed SP-FP Non Temporal (SSE).
  inline void movntps(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVNTPS, &dst, &src);
  }

  //! @brief Move Scalar SP-FP (SSE).
  inline void movss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVSS, &dst, &src);
  }

  //! @brief Move Scalar SP-FP (SSE).
  inline void movss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSS, &dst, &src);
  }

  //! @brief Move Scalar SP-FP (SSE).
  inline void movss(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVSS, &dst, &src);
  }

  //! @brief Move Unaligned Packed SP-FP Values (SSE).
  inline void movups(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVUPS, &dst, &src);
  }
  //! @brief Move Unaligned Packed SP-FP Values (SSE).
  inline void movups(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVUPS, &dst, &src);
  }

  //! @brief Move Unaligned Packed SP-FP Values (SSE).
  inline void movups(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVUPS, &dst, &src);
  }

  //! @brief Packed SP-FP Multiply (SSE).
  inline void mulps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MULPS, &dst, &src);
  }
  //! @brief Packed SP-FP Multiply (SSE).
  inline void mulps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MULPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Multiply (SSE).
  inline void mulss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MULSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Multiply (SSE).
  inline void mulss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MULSS, &dst, &src);
  }

  //! @brief Bit-wise Logical OR for SP-FP Data (SSE).
  inline void orps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ORPS, &dst, &src);
  }
  //! @brief Bit-wise Logical OR for SP-FP Data (SSE).
  inline void orps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ORPS, &dst, &src);
  }

  //! @brief Packed Average (SSE).
  inline void pavgb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }
  //! @brief Packed Average (SSE).
  inline void pavgb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }

  //! @brief Packed Average (SSE).
  inline void pavgw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }
  //! @brief Packed Average (SSE).
  inline void pavgw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }

  //! @brief Extract Word (SSE).
  inline void pextrw(const GPVar& dst, const MMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRW, &dst, &src, &imm8);
  }

  //! @brief Insert Word (SSE).
  inline void pinsrw(const MMVar& dst, const GPVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }
  //! @brief Insert Word (SSE).
  inline void pinsrw(const MMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }

  //! @brief Packed Signed Integer Word Maximum (SSE).
  inline void pmaxsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Maximum (SSE).
  inline void pmaxsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Maximum (SSE).
  inline void pmaxub(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Maximum (SSE).
  inline void pmaxub(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }

  //! @brief Packed Signed Integer Word Minimum (SSE).
  inline void pminsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Minimum (SSE).
  inline void pminsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Minimum (SSE).
  inline void pminub(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Minimum (SSE).
  inline void pminub(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }

  //! @brief Move Byte Mask To Integer (SSE).
  inline void pmovmskb(const GPVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMOVMSKB, &dst, &src);
  }

  //! @brief Packed Multiply High Unsigned (SSE).
  inline void pmulhuw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }
  //! @brief Packed Multiply High Unsigned (SSE).
  inline void pmulhuw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }

  //! @brief Packed Sum of Absolute Differences (SSE).
  inline void psadbw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }
  //! @brief Packed Sum of Absolute Differences (SSE).
  inline void psadbw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }

  //! @brief Packed Shuffle word (SSE).
  inline void pshufw(const MMVar& dst, const MMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFW, &dst, &src, &imm8);
  }
  //! @brief Packed Shuffle word (SSE).
  inline void pshufw(const MMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFW, &dst, &src, &imm8);
  }

  //! @brief Packed SP-FP Reciprocal (SSE).
  inline void rcpps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_RCPPS, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal (SSE).
  inline void rcpps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_RCPPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Reciprocal (SSE).
  inline void rcpss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_RCPSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Reciprocal (SSE).
  inline void rcpss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_RCPSS, &dst, &src);
  }

  //! @brief Prefetch (SSE).
  inline void prefetch(const Mem& mem, const Imm& hint)
  {
    _emitInstruction(INST_PREFETCH, &mem, &hint);
  }

  //! @brief Compute Sum of Absolute Differences (SSE).
  inline void psadbw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }
  //! @brief Compute Sum of Absolute Differences (SSE).
  inline void psadbw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }

  //! @brief Packed SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_RSQRTPS, &dst, &src);
  }
  //! @brief Packed SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_RSQRTPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_RSQRTSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_RSQRTSS, &dst, &src);
  }

  //! @brief Store fence (SSE).
  inline void sfence()
  {
    _emitInstruction(INST_SFENCE);
  }

  //! @brief Shuffle SP-FP (SSE).
  inline void shufps(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPS, &dst, &src, &imm8);
  }
  //! @brief Shuffle SP-FP (SSE).
  inline void shufps(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPS, &dst, &src, &imm8);
  }

  //! @brief Packed SP-FP Square Root (SSE).
  inline void sqrtps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SQRTPS, &dst, &src);
  }
  //! @brief Packed SP-FP Square Root (SSE).
  inline void sqrtps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Square Root (SSE).
  inline void sqrtss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SQRTSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Square Root (SSE).
  inline void sqrtss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTSS, &dst, &src);
  }

  //! @brief Store Streaming SIMD Extension Control/Status (SSE).
  inline void stmxcsr(const Mem& dst)
  {
    _emitInstruction(INST_STMXCSR, &dst);
  }

  //! @brief Packed SP-FP Subtract (SSE).
  inline void subps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SUBPS, &dst, &src);
  }
  //! @brief Packed SP-FP Subtract (SSE).
  inline void subps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Subtract (SSE).
  inline void subss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SUBSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Subtract (SSE).
  inline void subss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBSS, &dst, &src);
  }

  //! @brief Unordered Scalar SP-FP compare and set EFLAGS (SSE).
  inline void ucomiss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_UCOMISS, &dst, &src);
  }
  //! @brief Unordered Scalar SP-FP compare and set EFLAGS (SSE).
  inline void ucomiss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_UCOMISS, &dst, &src);
  }

  //! @brief Unpack High Packed SP-FP Data (SSE).
  inline void unpckhps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_UNPCKHPS, &dst, &src);
  }
  //! @brief Unpack High Packed SP-FP Data (SSE).
  inline void unpckhps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKHPS, &dst, &src);
  }

  //! @brief Unpack Low Packed SP-FP Data (SSE).
  inline void unpcklps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_UNPCKLPS, &dst, &src);
  }
  //! @brief Unpack Low Packed SP-FP Data (SSE).
  inline void unpcklps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKLPS, &dst, &src);
  }

  //! @brief Bit-wise Logical Xor for SP-FP Data (SSE).
  inline void xorps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_XORPS, &dst, &src);
  }
  //! @brief Bit-wise Logical Xor for SP-FP Data (SSE).
  inline void xorps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_XORPS, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [SSE2]
  // --------------------------------------------------------------------------

  //! @brief Packed DP-FP Add (SSE2).
  inline void addpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ADDPD, &dst, &src);
  }
  //! @brief Packed DP-FP Add (SSE2).
  inline void addpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Add (SSE2).
  inline void addsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ADDSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Add (SSE2).
  inline void addsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSD, &dst, &src);
  }

  //! @brief Bit-wise Logical And Not For DP-FP (SSE2).
  inline void andnpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ANDNPD, &dst, &src);
  }
  //! @brief Bit-wise Logical And Not For DP-FP (SSE2).
  inline void andnpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDNPD, &dst, &src);
  }

  //! @brief Bit-wise Logical And For DP-FP (SSE2).
  inline void andpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ANDPD, &dst, &src);
  }
  //! @brief Bit-wise Logical And For DP-FP (SSE2).
  inline void andpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDPD, &dst, &src);
  }

  //! @brief Flush Cache Line (SSE2).
  inline void clflush(const Mem& mem)
  {
    _emitInstruction(INST_CLFLUSH, &mem);
  }

  //! @brief Packed DP-FP Compare (SSE2).
  inline void cmppd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPD, &dst, &src, &imm8);
  }
  //! @brief Packed DP-FP Compare (SSE2).
  inline void cmppd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPD, &dst, &src, &imm8);
  }

  //! @brief Compare Scalar SP-FP Values (SSE2).
  inline void cmpsd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSD, &dst, &src, &imm8);
  }
  //! @brief Compare Scalar SP-FP Values (SSE2).
  inline void cmpsd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSD, &dst, &src, &imm8);
  }

  //! @brief Scalar Ordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void comisd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_COMISD, &dst, &src);
  }
  //! @brief Scalar Ordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void comisd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_COMISD, &dst, &src);
  }

  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtdq2pd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTDQ2PD, &dst, &src);
  }
  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtdq2pd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTDQ2PD, &dst, &src);
  }

  //! @brief Convert Packed Dword Integers to Packed SP-FP Values (SSE2).
  inline void cvtdq2ps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTDQ2PS, &dst, &src);
  }
  //! @brief Convert Packed Dword Integers to Packed SP-FP Values (SSE2).
  inline void cvtdq2ps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTDQ2PS, &dst, &src);
  }

  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2dq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTPD2DQ, &dst, &src);
  }
  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2dq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPD2DQ, &dst, &src);
  }

  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2pi(const MMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTPD2PI, &dst, &src);
  }
  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2pi(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPD2PI, &dst, &src);
  }

  //! @brief Convert Packed DP-FP Values to Packed SP-FP Values (SSE2).
  inline void cvtpd2ps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTPD2PS, &dst, &src);
  }
  //! @brief Convert Packed DP-FP Values to Packed SP-FP Values (SSE2).
  inline void cvtpd2ps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPD2PS, &dst, &src);
  }

  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtpi2pd(const XMMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_CVTPI2PD, &dst, &src);
  }
  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtpi2pd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPI2PD, &dst, &src);
  }

  //! @brief Convert Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtps2dq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTPS2DQ, &dst, &src);
  }
  //! @brief Convert Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtps2dq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPS2DQ, &dst, &src);
  }

  //! @brief Convert Packed SP-FP Values to Packed DP-FP Values (SSE2).
  inline void cvtps2pd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTPS2PD, &dst, &src);
  }
  //! @brief Convert Packed SP-FP Values to Packed DP-FP Values (SSE2).
  inline void cvtps2pd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPS2PD, &dst, &src);
  }

  //! @brief Convert Scalar DP-FP Value to Dword Integer (SSE2).
  inline void cvtsd2si(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTSD2SI, &dst, &src);
  }
  //! @brief Convert Scalar DP-FP Value to Dword Integer (SSE2).
  inline void cvtsd2si(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSD2SI, &dst, &src);
  }

  //! @brief Convert Scalar DP-FP Value to Scalar SP-FP Value (SSE2).
  inline void cvtsd2ss(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTSD2SS, &dst, &src);
  }
  //! @brief Convert Scalar DP-FP Value to Scalar SP-FP Value (SSE2).
  inline void cvtsd2ss(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSD2SS, &dst, &src);
  }

  //! @brief Convert Dword Integer to Scalar DP-FP Value (SSE2).
  inline void cvtsi2sd(const XMMVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_CVTSI2SD, &dst, &src);
  }
  //! @brief Convert Dword Integer to Scalar DP-FP Value (SSE2).
  inline void cvtsi2sd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSI2SD, &dst, &src);
  }

  //! @brief Convert Scalar SP-FP Value to Scalar DP-FP Value (SSE2).
  inline void cvtss2sd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTSS2SD, &dst, &src);
  }
  //! @brief Convert Scalar SP-FP Value to Scalar DP-FP Value (SSE2).
  inline void cvtss2sd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSS2SD, &dst, &src);
  }

  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2pi(const MMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTTPD2PI, &dst, &src);
  }
  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2pi(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPD2PI, &dst, &src);
  }

  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2dq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTTPD2DQ, &dst, &src);
  }
  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2dq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPD2DQ, &dst, &src);
  }

  //! @brief Convert with Truncation Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttps2dq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTTPS2DQ, &dst, &src);
  }
  //! @brief Convert with Truncation Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttps2dq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPS2DQ, &dst, &src);
  }

  //! @brief Convert with Truncation Scalar DP-FP Value to Signed Dword Integer (SSE2).
  inline void cvttsd2si(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_CVTTSD2SI, &dst, &src);
  }
  //! @brief Convert with Truncation Scalar DP-FP Value to Signed Dword Integer (SSE2).
  inline void cvttsd2si(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTSD2SI, &dst, &src);
  }

  //! @brief Packed DP-FP Divide (SSE2).
  inline void divpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_DIVPD, &dst, &src);
  }
  //! @brief Packed DP-FP Divide (SSE2).
  inline void divpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_DIVPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Divide (SSE2).
  inline void divsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_DIVSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Divide (SSE2).
  inline void divsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_DIVSD, &dst, &src);
  }

  //! @brief Load Fence (SSE2).
  inline void lfence()
  {
    _emitInstruction(INST_LFENCE);
  }

  //! @brief Store Selected Bytes of Double Quadword (SSE2).
  //!
  //! @note Target is DS:EDI.
  inline void maskmovdqu(const GPVar& dst_ptr, const XMMVar& src, const XMMVar& mask)
  {
    _emitInstruction(INST_MASKMOVDQU, &dst_ptr, &src, &mask);
  }

  //! @brief Return Maximum Packed Double-Precision FP Values (SSE2).
  inline void maxpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MAXPD, &dst, &src);
  }
  //! @brief Return Maximum Packed Double-Precision FP Values (SSE2).
  inline void maxpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXPD, &dst, &src);
  }

  //! @brief Return Maximum Scalar Double-Precision FP Value (SSE2).
  inline void maxsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MAXSD, &dst, &src);
  }
  //! @brief Return Maximum Scalar Double-Precision FP Value (SSE2).
  inline void maxsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXSD, &dst, &src);
  }

  //! @brief Memory Fence (SSE2).
  inline void mfence()
  {
    _emitInstruction(INST_MFENCE);
  }

  //! @brief Return Minimum Packed DP-FP Values (SSE2).
  inline void minpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MINPD, &dst, &src);
  }
  //! @brief Return Minimum Packed DP-FP Values (SSE2).
  inline void minpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MINPD, &dst, &src);
  }

  //! @brief Return Minimum Scalar DP-FP Value (SSE2).
  inline void minsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MINSD, &dst, &src);
  }
  //! @brief Return Minimum Scalar DP-FP Value (SSE2).
  inline void minsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MINSD, &dst, &src);
  }

  //! @brief Move Aligned DQWord (SSE2).
  inline void movdqa(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVDQA, &dst, &src);
  }
  //! @brief Move Aligned DQWord (SSE2).
  inline void movdqa(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVDQA, &dst, &src);
  }

  //! @brief Move Aligned DQWord (SSE2).
  inline void movdqa(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVDQA, &dst, &src);
  }

  //! @brief Move Unaligned Double Quadword (SSE2).
  inline void movdqu(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVDQU, &dst, &src);
  }
  //! @brief Move Unaligned Double Quadword (SSE2).
  inline void movdqu(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVDQU, &dst, &src);
  }

  //! @brief Move Unaligned Double Quadword (SSE2).
  inline void movdqu(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVDQU, &dst, &src);
  }

  //! @brief Extract Packed SP-FP Sign Mask (SSE2).
  inline void movmskps(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVMSKPS, &dst, &src);
  }

  //! @brief Extract Packed DP-FP Sign Mask (SSE2).
  inline void movmskpd(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVMSKPD, &dst, &src);
  }

  //! @brief Move Scalar Double-Precision FP Value (SSE2).
  inline void movsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVSD, &dst, &src);
  }
  //! @brief Move Scalar Double-Precision FP Value (SSE2).
  inline void movsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSD, &dst, &src);
  }

  //! @brief Move Scalar Double-Precision FP Value (SSE2).
  inline void movsd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVSD, &dst, &src);
  }

  //! @brief Move Aligned Packed Double-Precision FP Values (SSE2).
  inline void movapd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVAPD, &dst, &src);
  }

  //! @brief Move Aligned Packed Double-Precision FP Values (SSE2).
  inline void movapd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVAPD, &dst, &src);
  }

  //! @brief Move Aligned Packed Double-Precision FP Values (SSE2).
  inline void movapd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVAPD, &dst, &src);
  }

  //! @brief Move Quadword from XMM to MMX Technology Register (SSE2).
  inline void movdq2q(const MMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVDQ2Q, &dst, &src);
  }

  //! @brief Move Quadword from MMX Technology to XMM Register (SSE2).
  inline void movq2dq(const XMMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_MOVQ2DQ, &dst, &src);
  }

  //! @brief Move High Packed Double-Precision FP Value (SSE2).
  inline void movhpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVHPD, &dst, &src);
  }

  //! @brief Move High Packed Double-Precision FP Value (SSE2).
  inline void movhpd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVHPD, &dst, &src);
  }

  //! @brief Move Low Packed Double-Precision FP Value (SSE2).
  inline void movlpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVLPD, &dst, &src);
  }

  //! @brief Move Low Packed Double-Precision FP Value (SSE2).
  inline void movlpd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVLPD, &dst, &src);
  }

  //! @brief Store Double Quadword Using Non-Temporal Hint (SSE2).
  inline void movntdq(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVNTDQ, &dst, &src);
  }

  //! @brief Store Store DWORD Using Non-Temporal Hint (SSE2).
  inline void movnti(const Mem& dst, const GPVar& src)
  {
    _emitInstruction(INST_MOVNTI, &dst, &src);
  }

  //! @brief Store Packed Double-Precision FP Values Using Non-Temporal Hint (SSE2).
  inline void movntpd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVNTPD, &dst, &src);
  }

  //! @brief Move Unaligned Packed Double-Precision FP Values (SSE2).
  inline void movupd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVUPD, &dst, &src);
  }

  //! @brief Move Unaligned Packed Double-Precision FP Values (SSE2).
  inline void movupd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVUPD, &dst, &src);
  }

  //! @brief Move Unaligned Packed Double-Precision FP Values (SSE2).
  inline void movupd(const Mem& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVUPD, &dst, &src);
  }

  //! @brief Packed DP-FP Multiply (SSE2).
  inline void mulpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MULPD, &dst, &src);
  }
  //! @brief Packed DP-FP Multiply (SSE2).
  inline void mulpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MULPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Multiply (SSE2).
  inline void mulsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MULSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Multiply (SSE2).
  inline void mulsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MULSD, &dst, &src);
  }

  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void orpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ORPD, &dst, &src);
  }
  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void orpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ORPD, &dst, &src);
  }

  //! @brief Pack with Signed Saturation (SSE2).
  inline void packsswb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (SSE2).
  inline void packsswb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }

  //! @brief Pack with Signed Saturation (SSE2).
  inline void packssdw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (SSE2).
  inline void packssdw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }

  //! @brief Pack with Unsigned Saturation (SSE2).
  inline void packuswb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }
  //! @brief Pack with Unsigned Saturation (SSE2).
  inline void packuswb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }

  //! @brief Packed BYTE Add (SSE2).
  inline void paddb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }
  //! @brief Packed BYTE Add (SSE2).
  inline void paddb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }

  //! @brief Packed WORD Add (SSE2).
  inline void paddw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }
  //! @brief Packed WORD Add (SSE2).
  inline void paddw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }

  //! @brief Packed DWORD Add (SSE2).
  inline void paddd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }
  //! @brief Packed DWORD Add (SSE2).
  inline void paddd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }

  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }
  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }

  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }
  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }

  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }
  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }

  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }
  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }

  //! @brief Logical AND (SSE2).
  inline void pand(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }
  //! @brief Logical AND (SSE2).
  inline void pand(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }

  //! @brief Logical AND Not (SSE2).
  inline void pandn(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }
  //! @brief Logical AND Not (SSE2).
  inline void pandn(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }

  //! @brief Spin Loop Hint (SSE2).
  inline void pause()
  {
    _emitInstruction(INST_PAUSE);
  }

  //! @brief Packed Average (SSE2).
  inline void pavgb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }
  //! @brief Packed Average (SSE2).
  inline void pavgb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }

  //! @brief Packed Average (SSE2).
  inline void pavgw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }
  //! @brief Packed Average (SSE2).
  inline void pavgw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }

  //! @brief Packed Compare for Equal (BYTES) (SSE2).
  inline void pcmpeqb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }
  //! @brief Packed Compare for Equal (BYTES) (SSE2).
  inline void pcmpeqb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }

  //! @brief Packed Compare for Equal (WORDS) (SSE2).
  inline void pcmpeqw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }
  //! @brief Packed Compare for Equal (WORDS) (SSE2).
  inline void pcmpeqw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }

  //! @brief Packed Compare for Equal (DWORDS) (SSE2).
  inline void pcmpeqd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }
  //! @brief Packed Compare for Equal (DWORDS) (SSE2).
  inline void pcmpeqd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (BYTES) (SSE2).
  inline void pcmpgtb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (BYTES) (SSE2).
  inline void pcmpgtb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (WORDS) (SSE2).
  inline void pcmpgtw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (WORDS) (SSE2).
  inline void pcmpgtw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (DWORDS) (SSE2).
  inline void pcmpgtd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (DWORDS) (SSE2).
  inline void pcmpgtd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }

  //! @brief Extract Word (SSE2).
  inline void pextrw(const GPVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRW, &dst, &src, &imm8);
  }
  //! @brief Extract Word (SSE2).
  inline void pextrw(const Mem& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRW, &dst, &src, &imm8);
  }

  //! @brief Packed Signed Integer Word Maximum (SSE2).
  inline void pmaxsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Maximum (SSE2).
  inline void pmaxsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Maximum (SSE2).
  inline void pmaxub(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Maximum (SSE2).
  inline void pmaxub(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }

  //! @brief Packed Signed Integer Word Minimum (SSE2).
  inline void pminsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Minimum (SSE2).
  inline void pminsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Minimum (SSE2).
  inline void pminub(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Minimum (SSE2).
  inline void pminub(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }

  //! @brief Move Byte Mask (SSE2).
  inline void pmovmskb(const GPVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVMSKB, &dst, &src);
  }

  //! @brief Packed Multiply High (SSE2).
  inline void pmulhw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }
  //! @brief Packed Multiply High (SSE2).
  inline void pmulhw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }

  //! @brief Packed Multiply High Unsigned (SSE2).
  inline void pmulhuw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }
  //! @brief Packed Multiply High Unsigned (SSE2).
  inline void pmulhuw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }

  //! @brief Packed Multiply Low (SSE2).
  inline void pmullw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }
  //! @brief Packed Multiply Low (SSE2).
  inline void pmullw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }

  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }
  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }

  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }
  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }

  //! @brief Bitwise Logical OR (SSE2).
  inline void por(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }
  //! @brief Bitwise Logical OR (SSE2).
  inline void por(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslld(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslld(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslld(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllq(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllw(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslldq(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLDQ, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psrad(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psrad(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psrad(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psraw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psraw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psraw(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubq(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubq(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }

  //! @brief Packed Multiply and Add (SSE2).
  inline void pmaddwd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }
  //! @brief Packed Multiply and Add (SSE2).
  inline void pmaddwd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }

  //! @brief Shuffle Packed DWORDs (SSE2).
  inline void pshufd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFD, &dst, &src, &imm8);
  }
  //! @brief Shuffle Packed DWORDs (SSE2).
  inline void pshufd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFD, &dst, &src, &imm8);
  }

  //! @brief Shuffle Packed High Words (SSE2).
  inline void pshufhw(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFHW, &dst, &src, &imm8);
  }
  //! @brief Shuffle Packed High Words (SSE2).
  inline void pshufhw(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFHW, &dst, &src, &imm8);
  }

  //! @brief Shuffle Packed Low Words (SSE2).
  inline void pshuflw(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFLW, &dst, &src, &imm8);
  }
  //! @brief Shuffle Packed Low Words (SSE2).
  inline void pshuflw(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFLW, &dst, &src, &imm8);
  }

  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrld(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrld(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrld(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlq(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }

  //! @brief DQWord Shift Right Logical (MMX).
  inline void psrldq(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLDQ, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlw(const XMMVar& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhbw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhbw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhwd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhwd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhdq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhdq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhqdq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKHQDQ, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhqdq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHQDQ, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpcklbw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpcklbw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpcklwd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpcklwd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpckldq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpckldq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpcklqdq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PUNPCKLQDQ, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpcklqdq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLQDQ, &dst, &src);
  }

  //! @brief Bitwise Exclusive OR (SSE2).
  inline void pxor(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }
  //! @brief Bitwise Exclusive OR (SSE2).
  inline void pxor(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }

  //! @brief Shuffle DP-FP (SSE2).
  inline void shufpd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPD, &dst, &src, &imm8);
  }
  //! @brief Shuffle DP-FP (SSE2).
  inline void shufpd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPD, &dst, &src, &imm8);
  }

  //! @brief Compute Square Roots of Packed DP-FP Values (SSE2).
  inline void sqrtpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SQRTPD, &dst, &src);
  }
  //! @brief Compute Square Roots of Packed DP-FP Values (SSE2).
  inline void sqrtpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTPD, &dst, &src);
  }

  //! @brief Compute Square Root of Scalar DP-FP Value (SSE2).
  inline void sqrtsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SQRTSD, &dst, &src);
  }
  //! @brief Compute Square Root of Scalar DP-FP Value (SSE2).
  inline void sqrtsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTSD, &dst, &src);
  }

  //! @brief Packed DP-FP Subtract (SSE2).
  inline void subpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SUBPD, &dst, &src);
  }
  //! @brief Packed DP-FP Subtract (SSE2).
  inline void subpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Subtract (SSE2).
  inline void subsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_SUBSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Subtract (SSE2).
  inline void subsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBSD, &dst, &src);
  }

  //! @brief Scalar Unordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void ucomisd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_UCOMISD, &dst, &src);
  }
  //! @brief Scalar Unordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void ucomisd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_UCOMISD, &dst, &src);
  }

  //! @brief Unpack and Interleave High Packed Double-Precision FP Values (SSE2).
  inline void unpckhpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_UNPCKHPD, &dst, &src);
  }
  //! @brief Unpack and Interleave High Packed Double-Precision FP Values (SSE2).
  inline void unpckhpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKHPD, &dst, &src);
  }

  //! @brief Unpack and Interleave Low Packed Double-Precision FP Values (SSE2).
  inline void unpcklpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_UNPCKLPD, &dst, &src);
  }
  //! @brief Unpack and Interleave Low Packed Double-Precision FP Values (SSE2).
  inline void unpcklpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKLPD, &dst, &src);
  }

  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void xorpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_XORPD, &dst, &src);
  }
  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void xorpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_XORPD, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [SSE3]
  // --------------------------------------------------------------------------

  //! @brief Packed DP-FP Add/Subtract (SSE3).
  inline void addsubpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ADDSUBPD, &dst, &src);
  }
  //! @brief Packed DP-FP Add/Subtract (SSE3).
  inline void addsubpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSUBPD, &dst, &src);
  }

  //! @brief Packed SP-FP Add/Subtract (SSE3).
  inline void addsubps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_ADDSUBPS, &dst, &src);
  }
  //! @brief Packed SP-FP Add/Subtract (SSE3).
  inline void addsubps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSUBPS, &dst, &src);
  }

#if ASMJIT_NOT_SUPPORTED_BY_COMPILER
  // TODO: NOT IMPLEMENTED BY THE COMPILER.
  //! @brief Store Integer with Truncation (SSE3).
  inline void fisttp(const Mem& dst)
  {
    _emitInstruction(INST_FISTTP, &dst);
  }
#endif // ASMJIT_NOT_SUPPORTED_BY_COMPILER

  //! @brief Packed DP-FP Horizontal Add (SSE3).
  inline void haddpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_HADDPD, &dst, &src);
  }
  //! @brief Packed DP-FP Horizontal Add (SSE3).
  inline void haddpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_HADDPD, &dst, &src);
  }

  //! @brief Packed SP-FP Horizontal Add (SSE3).
  inline void haddps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_HADDPS, &dst, &src);
  }
  //! @brief Packed SP-FP Horizontal Add (SSE3).
  inline void haddps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_HADDPS, &dst, &src);
  }

  //! @brief Packed DP-FP Horizontal Subtract (SSE3).
  inline void hsubpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_HSUBPD, &dst, &src);
  }
  //! @brief Packed DP-FP Horizontal Subtract (SSE3).
  inline void hsubpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_HSUBPD, &dst, &src);
  }

  //! @brief Packed SP-FP Horizontal Subtract (SSE3).
  inline void hsubps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_HSUBPS, &dst, &src);
  }
  //! @brief Packed SP-FP Horizontal Subtract (SSE3).
  inline void hsubps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_HSUBPS, &dst, &src);
  }

  //! @brief Load Unaligned Integer 128 Bits (SSE3).
  inline void lddqu(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_LDDQU, &dst, &src);
  }

#if ASMJIT_NOT_SUPPORTED_BY_COMPILER
  //! @brief Set Up Monitor Address (SSE3).
  inline void monitor()
  {
    _emitInstruction(INST_MONITOR);
  }
#endif // ASMJIT_NOT_SUPPORTED_BY_COMPILER

  //! @brief Move One DP-FP and Duplicate (SSE3).
  inline void movddup(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVDDUP, &dst, &src);
  }
  //! @brief Move One DP-FP and Duplicate (SSE3).
  inline void movddup(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVDDUP, &dst, &src);
  }

  //! @brief Move Packed SP-FP High and Duplicate (SSE3).
  inline void movshdup(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVSHDUP, &dst, &src);
  }
  //! @brief Move Packed SP-FP High and Duplicate (SSE3).
  inline void movshdup(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSHDUP, &dst, &src);
  }

  //! @brief Move Packed SP-FP Low and Duplicate (SSE3).
  inline void movsldup(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_MOVSLDUP, &dst, &src);
  }
  //! @brief Move Packed SP-FP Low and Duplicate (SSE3).
  inline void movsldup(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSLDUP, &dst, &src);
  }

#if ASMJIT_NOT_SUPPORTED_BY_COMPILER
  //! @brief Monitor Wait (SSE3).
  inline void mwait()
  {
    _emitInstruction(INST_MWAIT);
  }
#endif // ASMJIT_NOT_SUPPORTED_BY_COMPILER

  // --------------------------------------------------------------------------
  // [SSSE3]
  // --------------------------------------------------------------------------

  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }

  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }
  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }

  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }
  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }

  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }
  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }

  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }
  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }

  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }
  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }

  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }
  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const MMVar& dst, const MMVar& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const MMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const MMVar& dst, const MMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const MMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }

  // --------------------------------------------------------------------------
  // [SSE4.1]
  // --------------------------------------------------------------------------

  //! @brief Blend Packed DP-FP Values (SSE4.1).
  inline void blendpd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPD, &dst, &src, &imm8);
  }
  //! @brief Blend Packed DP-FP Values (SSE4.1).
  inline void blendpd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPD, &dst, &src, &imm8);
  }

  //! @brief Blend Packed SP-FP Values (SSE4.1).
  inline void blendps(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPS, &dst, &src, &imm8);
  }
  //! @brief Blend Packed SP-FP Values (SSE4.1).
  inline void blendps(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPS, &dst, &src, &imm8);
  }

  //! @brief Variable Blend Packed DP-FP Values (SSE4.1).
  inline void blendvpd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_BLENDVPD, &dst, &src);
  }
  //! @brief Variable Blend Packed DP-FP Values (SSE4.1).
  inline void blendvpd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_BLENDVPD, &dst, &src);
  }

  //! @brief Variable Blend Packed SP-FP Values (SSE4.1).
  inline void blendvps(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_BLENDVPS, &dst, &src);
  }
  //! @brief Variable Blend Packed SP-FP Values (SSE4.1).
  inline void blendvps(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_BLENDVPS, &dst, &src);
  }

  //! @brief Dot Product of Packed DP-FP Values (SSE4.1).
  inline void dppd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPD, &dst, &src, &imm8);
  }
  //! @brief Dot Product of Packed DP-FP Values (SSE4.1).
  inline void dppd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPD, &dst, &src, &imm8);
  }

  //! @brief Dot Product of Packed SP-FP Values (SSE4.1).
  inline void dpps(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPS, &dst, &src, &imm8);
  }
  //! @brief Dot Product of Packed SP-FP Values (SSE4.1).
  inline void dpps(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPS, &dst, &src, &imm8);
  }

  //! @brief Extract Packed SP-FP Value (SSE4.1).
  inline void extractps(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_EXTRACTPS, &dst, &src, &imm8);
  }
  //! @brief Extract Packed SP-FP Value (SSE4.1).
  inline void extractps(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_EXTRACTPS, &dst, &src, &imm8);
  }

  //! @brief Load Double Quadword Non-Temporal Aligned Hint (SSE4.1).
  inline void movntdqa(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVNTDQA, &dst, &src);
  }

  //! @brief Compute Multiple Packed Sums of Absolute Difference (SSE4.1).
  inline void mpsadbw(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_MPSADBW, &dst, &src, &imm8);
  }
  //! @brief Compute Multiple Packed Sums of Absolute Difference (SSE4.1).
  inline void mpsadbw(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_MPSADBW, &dst, &src, &imm8);
  }

  //! @brief Pack with Unsigned Saturation (SSE4.1).
  inline void packusdw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PACKUSDW, &dst, &src);
  }
  //! @brief Pack with Unsigned Saturation (SSE4.1).
  inline void packusdw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKUSDW, &dst, &src);
  }

  //! @brief Variable Blend Packed Bytes (SSE4.1).
  inline void pblendvb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PBLENDVB, &dst, &src);
  }
  //! @brief Variable Blend Packed Bytes (SSE4.1).
  inline void pblendvb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PBLENDVB, &dst, &src);
  }

  //! @brief Blend Packed Words (SSE4.1).
  inline void pblendw(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PBLENDW, &dst, &src, &imm8);
  }
  //! @brief Blend Packed Words (SSE4.1).
  inline void pblendw(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PBLENDW, &dst, &src, &imm8);
  }

  //! @brief Compare Packed Qword Data for Equal (SSE4.1).
  inline void pcmpeqq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPEQQ, &dst, &src);
  }
  //! @brief Compare Packed Qword Data for Equal (SSE4.1).
  inline void pcmpeqq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQQ, &dst, &src);
  }

  //! @brief Extract Byte (SSE4.1).
  inline void pextrb(const GPVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRB, &dst, &src, &imm8);
  }
  //! @brief Extract Byte (SSE4.1).
  inline void pextrb(const Mem& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRB, &dst, &src, &imm8);
  }

  //! @brief Extract Dword (SSE4.1).
  inline void pextrd(const GPVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRD, &dst, &src, &imm8);
  }
  //! @brief Extract Dword (SSE4.1).
  inline void pextrd(const Mem& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRD, &dst, &src, &imm8);
  }

  //! @brief Extract Dword (SSE4.1).
  inline void pextrq(const GPVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRQ, &dst, &src, &imm8);
  }
  //! @brief Extract Dword (SSE4.1).
  inline void pextrq(const Mem& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRQ, &dst, &src, &imm8);
  }

  //! @brief Packed Horizontal Word Minimum (SSE4.1).
  inline void phminposuw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PHMINPOSUW, &dst, &src);
  }
  //! @brief Packed Horizontal Word Minimum (SSE4.1).
  inline void phminposuw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PHMINPOSUW, &dst, &src);
  }

  //! @brief Insert Byte (SSE4.1).
  inline void pinsrb(const XMMVar& dst, const GPVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRB, &dst, &src, &imm8);
  }
  //! @brief Insert Byte (SSE4.1).
  inline void pinsrb(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRB, &dst, &src, &imm8);
  }

  //! @brief Insert Dword (SSE4.1).
  inline void pinsrd(const XMMVar& dst, const GPVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRD, &dst, &src, &imm8);
  }
  //! @brief Insert Dword (SSE4.1).
  inline void pinsrd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRD, &dst, &src, &imm8);
  }

  //! @brief Insert Dword (SSE4.1).
  inline void pinsrq(const XMMVar& dst, const GPVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRQ, &dst, &src, &imm8);
  }
  //! @brief Insert Dword (SSE4.1).
  inline void pinsrq(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRQ, &dst, &src, &imm8);
  }

  //! @brief Insert Word (SSE2).
  inline void pinsrw(const XMMVar& dst, const GPVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }
  //! @brief Insert Word (SSE2).
  inline void pinsrw(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }

  //! @brief Maximum of Packed Word Integers (SSE4.1).
  inline void pmaxuw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMAXUW, &dst, &src);
  }
  //! @brief Maximum of Packed Word Integers (SSE4.1).
  inline void pmaxuw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUW, &dst, &src);
  }

  //! @brief Maximum of Packed Signed Byte Integers (SSE4.1).
  inline void pmaxsb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMAXSB, &dst, &src);
  }
  //! @brief Maximum of Packed Signed Byte Integers (SSE4.1).
  inline void pmaxsb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSB, &dst, &src);
  }

  //! @brief Maximum of Packed Signed Dword Integers (SSE4.1).
  inline void pmaxsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMAXSD, &dst, &src);
  }
  //! @brief Maximum of Packed Signed Dword Integers (SSE4.1).
  inline void pmaxsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSD, &dst, &src);
  }

  //! @brief Maximum of Packed Unsigned Dword Integers (SSE4.1).
  inline void pmaxud(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMAXUD, &dst, &src);
  }
  //! @brief Maximum of Packed Unsigned Dword Integers (SSE4.1).
  inline void pmaxud(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUD, &dst, &src);
  }

  //! @brief Minimum of Packed Signed Byte Integers (SSE4.1).
  inline void pminsb(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMINSB, &dst, &src);
  }
  //! @brief Minimum of Packed Signed Byte Integers (SSE4.1).
  inline void pminsb(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSB, &dst, &src);
  }

  //! @brief Minimum of Packed Word Integers (SSE4.1).
  inline void pminuw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMINUW, &dst, &src);
  }
  //! @brief Minimum of Packed Word Integers (SSE4.1).
  inline void pminuw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUW, &dst, &src);
  }

  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminud(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMINUD, &dst, &src);
  }
  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminud(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUD, &dst, &src);
  }

  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminsd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMINSD, &dst, &src);
  }
  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminsd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSD, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVSXBW, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXBW, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVSXBD, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXBD, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVSXBQ, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXBQ, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxwd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVSXWD, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxwd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXWD, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovsxwq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVSXWQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovsxwq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXWQ, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovsxdq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVSXDQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovsxdq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXDQ, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbw(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVZXBW, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbw(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXBW, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVZXBD, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXBD, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVZXBQ, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXBQ, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxwd(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVZXWD, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxwd(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXWD, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovzxwq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVZXWQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovzxwq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXWQ, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovzxdq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMOVZXDQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovzxdq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXDQ, &dst, &src);
  }

  //! @brief Multiply Packed Signed Dword Integers (SSE4.1).
  inline void pmuldq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULDQ, &dst, &src);
  }
  //! @brief Multiply Packed Signed Dword Integers (SSE4.1).
  inline void pmuldq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULDQ, &dst, &src);
  }

  //! @brief Multiply Packed Signed Integers and Store Low Result (SSE4.1).
  inline void pmulld(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PMULLD, &dst, &src);
  }
  //! @brief Multiply Packed Signed Integers and Store Low Result (SSE4.1).
  inline void pmulld(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULLD, &dst, &src);
  }

  //! @brief Logical Compare (SSE4.1).
  inline void ptest(const XMMVar& op1, const XMMVar& op2)
  {
    _emitInstruction(INST_PTEST, &op1, &op2);
  }
  //! @brief Logical Compare (SSE4.1).
  inline void ptest(const XMMVar& op1, const Mem& op2)
  {
    _emitInstruction(INST_PTEST, &op1, &op2);
  }

  //! Round Packed SP-FP Values @brief (SSE4.1).
  inline void roundps(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPS, &dst, &src, &imm8);
  }
  //! Round Packed SP-FP Values @brief (SSE4.1).
  inline void roundps(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPS, &dst, &src, &imm8);
  }

  //! @brief Round Scalar SP-FP Values (SSE4.1).
  inline void roundss(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSS, &dst, &src, &imm8);
  }
  //! @brief Round Scalar SP-FP Values (SSE4.1).
  inline void roundss(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSS, &dst, &src, &imm8);
  }

  //! @brief Round Packed DP-FP Values (SSE4.1).
  inline void roundpd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPD, &dst, &src, &imm8);
  }
  //! @brief Round Packed DP-FP Values (SSE4.1).
  inline void roundpd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPD, &dst, &src, &imm8);
  }

  //! @brief Round Scalar DP-FP Values (SSE4.1).
  inline void roundsd(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSD, &dst, &src, &imm8);
  }
  //! @brief Round Scalar DP-FP Values (SSE4.1).
  inline void roundsd(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSD, &dst, &src, &imm8);
  }

  // --------------------------------------------------------------------------
  // [SSE4.2]
  // --------------------------------------------------------------------------

  //! @brief Accumulate CRC32 Value (polynomial 0x11EDC6F41) (SSE4.2).
  inline void crc32(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_CRC32, &dst, &src);
  }
  //! @brief Accumulate CRC32 Value (polynomial 0x11EDC6F41) (SSE4.2).
  inline void crc32(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_CRC32, &dst, &src);
  }

  //! @brief Packed Compare Explicit Length Strings, Return Index (SSE4.2).
  inline void pcmpestri(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRI, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Explicit Length Strings, Return Index (SSE4.2).
  inline void pcmpestri(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRI, &dst, &src, &imm8);
  }

  //! @brief Packed Compare Explicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpestrm(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRM, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Explicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpestrm(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRM, &dst, &src, &imm8);
  }

  //! @brief Packed Compare Implicit Length Strings, Return Index (SSE4.2).
  inline void pcmpistri(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRI, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Implicit Length Strings, Return Index (SSE4.2).
  inline void pcmpistri(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRI, &dst, &src, &imm8);
  }

  //! @brief Packed Compare Implicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpistrm(const XMMVar& dst, const XMMVar& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRM, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Implicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpistrm(const XMMVar& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRM, &dst, &src, &imm8);
  }

  //! @brief Compare Packed Data for Greater Than (SSE4.2).
  inline void pcmpgtq(const XMMVar& dst, const XMMVar& src)
  {
    _emitInstruction(INST_PCMPGTQ, &dst, &src);
  }
  //! @brief Compare Packed Data for Greater Than (SSE4.2).
  inline void pcmpgtq(const XMMVar& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTQ, &dst, &src);
  }

  //! @brief Return the Count of Number of Bits Set to 1 (SSE4.2).
  inline void popcnt(const GPVar& dst, const GPVar& src)
  {
    _emitInstruction(INST_POPCNT, &dst, &src);
  }
  //! @brief Return the Count of Number of Bits Set to 1 (SSE4.2).
  inline void popcnt(const GPVar& dst, const Mem& src)
  {
    _emitInstruction(INST_POPCNT, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [AMD only]
  // --------------------------------------------------------------------------

  //! @brief Prefetch (3dNow - Amd).
  //!
  //! Loads the entire 64-byte aligned memory sequence containing the
  //! specified memory address into the L1 data cache. The position of
  //! the specified memory address within the 64-byte cache line is
  //! irrelevant. If a cache hit occurs, or if a memory fault is detected,
  //! no bus cycle is initiated and the instruction is treated as a NOP.
  inline void amd_prefetch(const Mem& mem)
  {
    _emitInstruction(INST_AMD_PREFETCH, &mem);
  }

  //! @brief Prefetch and set cache to modified (3dNow - Amd).
  //!
  //! The PREFETCHW instruction loads the prefetched line and sets the
  //! cache-line state to Modified, in anticipation of subsequent data
  //! writes to the line. The PREFETCH instruction, by contrast, typically
  //! sets the cache-line state to Exclusive (depending on the hardware
  //! implementation).
  inline void amd_prefetchw(const Mem& mem)
  {
    _emitInstruction(INST_AMD_PREFETCHW, &mem);
  }

  // --------------------------------------------------------------------------
  // [Intel only]
  // --------------------------------------------------------------------------

  //! @brief Move Data After Swapping Bytes (SSE3 - Intel Atom).
  inline void movbe(const GPVar& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_MOVBE, &dst, &src);
  }

  //! @brief Move Data After Swapping Bytes (SSE3 - Intel Atom).
  inline void movbe(const Mem& dst, const GPVar& src)
  {
    ASMJIT_ASSERT(!src.isGPB());
    _emitInstruction(INST_MOVBE, &dst, &src);
  }

  // -------------------------------------------------------------------------
  // [Emit Options]
  // -------------------------------------------------------------------------

  //! @brief Assert LOCK# Signal Prefix.
  //!
  //! This instruction causes the processor's LOCK# signal to be asserted
  //! during execution of the accompanying instruction (turns the
  //! instruction into an atomic instruction). In a multiprocessor environment,
  //! the LOCK# signal insures that the processor has exclusive use of any shared
  //! memory while the signal is asserted.
  //!
  //! The LOCK prefix can be prepended only to the following instructions and
  //! to those forms of the instructions that use a memory operand: ADD, ADC,
  //! AND, BTC, BTR, BTS, CMPXCHG, DEC, INC, NEG, NOT, OR, SBB, SUB, XOR, XADD,
  //! and XCHG. An undefined opcode exception will be generated if the LOCK
  //! prefix is used with any other instruction. The XCHG instruction always
  //! asserts the LOCK# signal regardless of the presence or absence of the LOCK
  //! prefix.
  inline void lock()
  {
    _emitOptions |= EMIT_OPTION_LOCK_PREFIX;
  }

  //! @brief Force REX prefix to be emitted.
  //!
  //! This option should be used carefully, because there are unencodable
  //! combinations. If you want to access ah, bh, ch or dh registers then you
  //! can't emit REX prefix and it will cause an illegal instruction error.
  //!
  //! @note REX prefix is only valid for X64/AMD64 platform.
  //!
  //! @sa @c EMIT_OPTION_REX_PREFIX.
  inline void rex()
  {
    _emitOptions |= EMIT_OPTION_REX_PREFIX;
  }
};

// ============================================================================
// [AsmJit::Compiler]
// ============================================================================

//! @brief Compiler - high level code generation.
//!
//! This class is used to store instruction stream and allows to modify
//! it on the fly. It uses different concept than @c AsmJit::Assembler class
//! and in fact @c AsmJit::Assembler is only used as a backend. Compiler never
//! emits machine code and each instruction you use is stored to instruction
//! array instead. This allows to modify instruction stream later and for
//! example to reorder instructions to make better performance.
//!
//! Using @c AsmJit::Compiler moves code generation to higher level. Higher
//! level constructs allows to write more abstract and extensible code that
//! is not possible with pure @c AsmJit::Assembler class. Because
//! @c AsmJit::Compiler needs to create many objects and lifetime of these
//! objects is small (same as @c AsmJit::Compiler lifetime itself) it uses
//! very fast memory management model. This model allows to create object
//! instances in nearly zero time (compared to @c malloc() or @c new()
//! operators) so overhead by creating machine code by @c AsmJit::Compiler
//! is minimized.
//!
//! @section AsmJit_Compiler_TheStory The Story
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
//! you must give it four variables which will be automatically allocated to
//! input/output registers (EAX, EBX, ECX, EDX).
//! 
//! Another improvement is algorithm used by a register allocator. In first
//! version the registers were allocated when creating instruction stream. In
//! new version registers are allocated after calling @c Compiler::make(). This
//! means that register allocator has information about scope of all variables
//! and their usage statistics. The algorithm to allocate registers is very
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
//! the generated code. AsmJit uses a @c FunctionPrototype class to hold
//! the function parameters, their position in stack (or register index) and
//! function return value. This class is used internally, but it can be
//! used to create your own function calling-convention. All standard function
//! calling conventions are implemented.
//!
//! Please enjoy the new version of Compiler, it was created for writing a
//! low-level code using high-level API, leaving developer to concentrate to
//! real problems and not to solving a register puzzle.
//!
//! @section AsmJit_Compiler_CodeGeneration Code Generation
//!
//! First that is needed to know about compiler is that compiler never emits
//! machine code. It's used as a middleware between @c AsmJit::Assembler and
//! your code. There is also convenience method @c make() that allows to
//! generate machine code directly without creating @c AsmJit::Assembler
//! instance.
//!
//! Comparison of generating machine code through @c Assembler and directly
//! by @c Compiler:
//!
//! @code
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
//! // Final step - generate code. AsmJit::Compiler::serialize() will serialize
//! // all instructions into Assembler and this ensures generating real machine
//! // code.
//! c.serialize(a);
//!
//! // Your function
//! void* fn = a.make();
//! @endcode
//!
//! Example how to generate machine code using only @c Compiler (preferred):
//!
//! @code
//! // Compiler instance is enough.
//! Compiler c;
//!
//! // ... put your code here ...
//!
//! // Your function
//! void* fn = c.make();
//! @endcode
//!
//! You can see that there is @c AsmJit::Compiler::serialize() function that
//! emits instructions into @c AsmJit::Assembler(). This layered architecture
//! means that each class is used for something different and there is no code
//! duplication. For convenience there is also @c AsmJit::Compiler::make()
//! method that can create your function using @c AsmJit::Assembler, but
//! internally (this is preffered bahavior when using @c AsmJit::Compiler).
//!
//! The @c make() method allocates memory using @c CodeGenerator instance passed
//! into the @c Compiler constructor. If code generator is used to create JIT
//! function then virtual memory allocated by @c MemoryManager is used. To get
//! global memory manager use @c MemoryManager::getGlobal().
//!
//! @code
//! // Compiler instance is enough.
//! Compiler c;
//!
//! // ... put your code using Compiler instance ...
//!
//! // Your function
//! void* fn = c.make();
//!
//! // Free it if you don't want it anymore
//! // (using global memory manager instance)
//! MemoryManager::getGlobal()->free(fn);
//! @endcode
//!
//! @section AsmJit_Compiler_Functions Functions
//!
//! To build functions with @c Compiler, see @c AsmJit::Compiler::newFunction()
//! method.
//!
//! @section AsmJit_Compiler_Variables Variables
//!
//! Compiler is able to manage variables and function arguments. Internally
//! there is no difference between function argument and variable declared
//! inside. To get function argument you use @c argGP() method and to declare
//! variable use @c newGP(), @c newMM() and @c newXMM() methods. The @c newXXX()
//! methods accept also parameter describing the variable type. For example
//! the @c newGP() method always creates variable which size matches the target
//! architecture size (for 32-bit target the 32-bit variable is created, for
//! 64-bit target the variable size is 64-bit). To override this behavior the
//! variable type must be specified.
//!
//! @code
//! // Compiler and function declaration - void f(int*);
//! Compiler c;
//! c.newFunction(CALL_CONV_DEFAULT, BuildFunction1<int*>());
//!
//! // Get argument variable (it's pointer).
//! GPVar a1(c.argGP(0));
//!
//! // Create your variables.
//! GPVar x1(c.newGP(VARIABLE_TYPE_GPD));
//! GPVar x2(c.newGP(VARIABLE_TYPE_GPD));
//!
//! // Init your variables.
//! c.mov(x1, 1);
//! c.mov(x2, 2);
//!
//! // ... your code ...
//! c.add(x1, x2);
//! // ... your code ...
//!
//! // Store result to a given pointer in first argument
//! c.mov(dword_ptr(a1), x1);
//!
//! // End of function body.
//! c.endFunction();
//!
//! // Make the function.
//! typedef void (*MyFn)(int*);
//! MyFn fn = function_cast<MyFn>(c.make());
//! @endcode
//!
//! This code snipped needs to be explained. You can see that there are more 
//! variable types that can be used by @c Compiler. Most useful variables can
//! be allocated using general purpose registers (@c GPVar), MMX registers 
//! (@c MMVar) or SSE registers (@c XMMVar).
//!
//! X86/X64 variable types:
//! 
//! - @c VARIABLE_TYPE_GPD - 32-bit general purpose register (EAX, EBX, ...).
//! - @c VARIABLE_TYPE_GPQ - 64-bit general purpose register (RAX, RBX, ...).
//! - @c VARIABLE_TYPE_GPN - 32-bit or 64-bit general purpose register, depends
//!   to target architecture. Mapped to @c VARIABLE_TYPE_GPD or @c VARIABLE_TYPE_GPQ.
//!
//! - @c VARIABLE_TYPE_X87 - 80-bit floating point stack register st(0 to 7).
//! - @c VARIABLE_TYPE_X87_1F - 32-bit floating point stack register st(0 to 7).
//! - @c VARIABLE_TYPE_X87_1D - 64-bit floating point stack register st(0 to 7).
//!
//! - @c VARIALBE_TYPE_MM - 64-bit MMX register.
//!
//! - @c VARIABLE_TYPE_XMM - 128-bit SSE register.
//! - @c VARIABLE_TYPE_XMM_1F - 128-bit SSE register which contains 
//!   scalar 32-bit single precision floating point.
//! - @c VARIABLE_TYPE_XMM_1D - 128-bit SSE register which contains
//!   scalar 64-bit double precision floating point.
//! - @c VARIABLE_TYPE_XMM_4F - 128-bit SSE register which contains
//!   4 packed 32-bit single precision floating points.
//! - @c VARIABLE_TYPE_XMM_2D - 128-bit SSE register which contains
//!   2 packed 64-bit double precision floating points.
//!
//! Unified variable types:
//!
//! - @c VARIABLE_TYPE_INT32 - 32-bit general purpose register.
//! - @c VARIABLE_TYPE_INT64 - 64-bit general purpose register.
//! - @c VARIABLE_TYPE_INTPTR - 32-bit or 64-bit general purpose register / pointer.
//!
//! - @c VARIABLE_TYPE_FLOAT - 32-bit single precision floating point.
//! - @c VARIABLE_TYPE_DOUBLE - 64-bit double precision floating point.
//!
//! Variable states:
//!
//! - @c VARIABLE_STATE_UNUSED - State that is assigned to newly created
//!   variables or to not used variables (dereferenced to zero).
//! - @c VARIABLE_STATE_REGISTER - State that means that variable is currently
//!   allocated in register.
//! - @c VARIABLE_STATE_MEMORY - State that means that variable is currently
//!   only in memory location.
//!
//! When you create new variable, initial state is always @c VARIABLE_STATE_UNUSED,
//! allocating it to register or spilling to memory changes this state to
//! @c VARIABLE_STATE_REGISTER or @c VARIABLE_STATE_MEMORY, respectively.
//! During variable lifetime it's usual that its state is changed multiple
//! times. To generate better code, you can control allocating and spilling
//! by using up to four types of methods that allows it (see next list).
//!
//! Explicit variable allocating / spilling methods:
//!
//! - @c Compiler::alloc() - Explicit method to alloc variable into
//!      register. You can use this before loops or code blocks.
//!
//! - @c Compiler::spill() - Explicit method to spill variable. If variable
//!      is in register and you call this method, it's moved to its home memory
//!      location. If variable is not in register no operation is performed.
//!
//! - @c Compiler::unuse() - Unuse variable (you can use this to end the
//!      variable scope or sub-scope).
//!
//! Please see AsmJit tutorials (testcompiler.cpp and testvariables.cpp) for
//! more complete examples.
//!
//! @section AsmJit_Compiler_MemoryManagement Memory Management
//!
//! @c Compiler Memory management follows these rules:
//! - Everything created by @c Compiler is always freed by @c Compiler.
//! - To get decent performance, compiler always uses larger memory buffer
//!   for objects to allocate and when compiler instance is destroyed, this
//!   buffer is freed. Destructors of active objects are called when
//!   destroying compiler instance. Destructors of abadonded compiler
//!   objects are called immediately after abadonding them.
//! - This type of memory management is called 'zone memory management'.
//!
//! This means that you can't use any @c Compiler object after destructing it,
//! it also means that each object like @c Label, @c Var and others are created
//! and managed by @c Compiler itself. These objects contain ID which is used
//! internally by Compiler to store additional information about these objects.
//!
//! @section AsmJit_Compiler_StateManagement Control-Flow and State Management.
//!
//! The @c Compiler automatically manages state of the variables when using
//! control flow instructions like jumps, conditional jumps and calls. There
//! is minimal heuristics for choosing the method how state is saved or restored.
//!
//! Generally the state can be changed only when using jump or conditional jump
//! instruction. When using non-conditional jump then state change is embedded
//! into the instruction stream before the jump. When using conditional jump
//! the @c Compiler decides whether to restore state before the jump or whether
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
//! @code
//! Compiler c;
//!
//! c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<Void>());
//! c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
//!
//! // Labels.
//! Label L0 = c.newLabel();
//!
//! // Variables.
//! GPVar var0 = c.newGP();
//! GPVar var1 = c.newGP();
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
//! c.endFunction();
//! @endcode
//!
//! The output:
//!
//! @verbatim
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
//! @endverbatim
//!
//! You can see that the state-switch section was generated (see L0_Switch0).
//! The compiler is unable to restore state immediately when emitting the
//! forward jump (the code is generated from first to last instruction and
//! the target state is simply not known at this time).
//!
//! To tell @c Compiler that you want to embed state-switch code before jump
//! it's needed to create backward jump (where also processor expects that it
//! will be taken). To demonstrate the possibility to embed state-switch before
//! jump we use slightly modified code:
//!
//! @code
//! Compiler c;
//! 
//! c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<Void>());
//! c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
//! 
//! // Labels.
//! Label L0 = c.newLabel();
//! 
//! // Variables.
//! GPVar var0 = c.newGP();
//! GPVar var1 = c.newGP();
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
//! c.endFunction();
//! @endcode
//!
//! The output:
//!
//! @verbatim
//! xor ecx, ecx                    ; xor var_0, var_0
//! xor edx, edx                    ; xor var_1, var_1
//! mov [esp - 24], ecx             ; spill var_0
//! mov [esp - 28], edx             ; spill var_1
//! L.2:
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
//! je short L.2
//! ret
//! @endverbatim
//!
//! Please notice where the state-switch section is located. The @c Compiler 
//! decided that jump is likely to be taken so the state change is embedded
//! before the conditional jump. To change this behavior into the previous
//! case it's needed to add a hint (@c HINT_TAKEN or @c HINT_NOT_TAKEN).
//!
//! Replacing the <code>c.je(L0)</code> by <code>c.je(L0, HINT_NOT_TAKEN)
//! will generate code like this:
//!
//! @verbatim
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
//! @endverbatim
//!
//! This section provided information about how state-change works. The 
//! behavior is deterministic and it can be overridden.
//!
//! @section AsmJit_Compiler_AdvancedCodeGeneration Advanced Code Generation
//!
//! This section describes advanced method of code generation available to
//! @c Compiler (but also to @c Assembler). When emitting code to instruction
//! stream the methods like @c mov(), @c add(), @c sub() can be called directly
//! (advantage is static-type control performed also by C++ compiler) or 
//! indirectly using @c emit() method. The @c emit() method needs only 
//! instruction code and operands.
//!
//! Example of code generating by standard type-safe API:
//!
//! @code
//! Compiler c;
//! GPVar var0 = c.newGP();
//! GPVar var1 = c.newGP();
//!
//! ...
//!
//! c.mov(var0, imm(0));
//! c.add(var0, var1);
//! c.sub(var0, var1);
//! @endcode
//!
//! The code above can be rewritten as:
//!
//! @code
//! Compiler c;
//! GPVar var0 = c.newGP();
//! GPVar var1 = c.newGP();
//!
//! ...
//!
//! c.emit(INST_MOV, var0, imm(0));
//! c.emit(INST_ADD, var0, var1);
//! c.emit(INST_SUB, var0, var1);
//! @endcode
//!
//! The advantage of first snippet is very friendly API and type-safe control
//! that is controlled by the C++ compiler. The advantage of second snippet is
//! availability to replace or generate instruction code in different places.
//! See the next example how the @c emit() method can be used to generate
//! abstract code.
//!
//! Use case:
//!
//! @code
//! bool emitArithmetic(Compiler& c, XMMVar& var0, XMMVar& var1, const char* op)
//! {
//!   uint code = INVALID_VALUE;
//!
//!   if (strcmp(op, "ADD") == 0)
//!     code = INST_ADDSS;
//!   else if (strcmp(op, "SUBTRACT") == 0)
//!     code = INST_SUBSS;
//!   else if (strcmp(op, "MULTIPLY") == 0)
//!     code = INST_MULSS;
//!   else if (strcmp(op, "DIVIDE") == 0)
//!     code = INST_DIVSS;
//!   else
//!     // Invalid parameter?
//!     return false;
//!
//!   c.emit(code, var0, var1);
//! }
//! @endcode
//!
//! Other use cases are waiting for you! Be sure that instruction you are 
//! emitting is correct and encodable, because if not, Assembler will set
//! error code to @c ERROR_UNKNOWN_INSTRUCTION.
//!
//! @section AsmJit_Compiler_CompilerDetails Compiler Details
//!
//! This section is here for people interested in the compiling process. There
//! are few steps that must be done for each compiled function (or your code).
//!
//! When your @c Compiler instance is ready, you can create function and add
//! emittables using intrinsics or higher level methods implemented by the
//! @c AsmJit::Compiler. When you are done (all instructions serialized) you
//! should call @c AsmJit::Compiler::make() method which will analyze your code,
//! allocate registers and memory for local variables and serialize all emittables
//! to @c AsmJit::Assembler instance. Next steps shows what's done internally
//! before code is serialized into @c AsmJit::Assembler
//!   (implemented in @c AsmJit::Compiler::serialize() method).
//!
//! 1. Compiler try to match function and end-function emittables (these
//!    emittables define the function-body or function block).
//!
//! 2. For all emittables inside the function-body the virtual functions
//!    are called in this order:
//!    - Emittable::prepare()
//!    - Emittable::translate()
//!    - Emittable::emit()
//!    - Emittable::post()
//!
//!    There is some extra work when emitting function prolog / epilog and
//!    register allocator.
//!
//! 3. Emit jump tables data.
//!
//! When everything here ends, @c AsmJit::Assembler contains binary stream
//! that needs only relocation to be callable by C/C++ code.
//!
//! @section AsmJit_Compiler_Differences Summary of Differences between @c Assembler and @c Compiler
//!
//! - Instructions are not translated to machine code immediately, they are
//!   stored as emmitables (see @c AsmJit::Emittable, @c AsmJit::EInstruction).
//! - Contains function builder and ability to call other functions.
//! - Contains register allocator and variable management.
//! - Contains a lot of helper methods to simplify the code generation not
//!   available/possible in @c AsmJit::Assembler.
//! - Ability to pre-process or post-process the code which is being generated.
struct ASMJIT_API Compiler : public CompilerIntrinsics
{
  //! @brief Create the @c Compiler instance.
  Compiler(CodeGenerator* codeGenerator = NULL) ASMJIT_NOTHROW;
  //! @brief Destroy the @c Compiler instance.
  virtual ~Compiler() ASMJIT_NOTHROW;
};

//! @}

#pragma warning(default : 4201)

} // AsmJit namespace

#undef ASMJIT_NOT_SUPPORTED_BY_COMPILER

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_COMPILERX86X64_H
