// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// We are using sprintf() here.
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

// [Dependencies]
#include "Assembler.h"
#include "CodeGenerator.h"
#include "Compiler.h"
#include "CpuInfo.h"
#include "Logger.h"
#include "Util_p.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

#pragma warning(disable : 4389 4189 4100)


// ============================================================================
// [Helpers - Logging]
// ============================================================================

// Defined in AssemblerX86X64.cpp.
ASMJIT_HIDDEN char* dumpRegister(char* buf, uint32_t type, uint32_t index) ASMJIT_NOTHROW;
ASMJIT_HIDDEN char* dumpOperand(char* buf, const Operand* op, uint32_t memRegType) ASMJIT_NOTHROW;

// ============================================================================
// [Helpers - Variables]
// ============================================================================

struct VariableInfo
{
  enum CLASS_INFO
  {
    CLASS_NONE   = 0x00,
    CLASS_GP     = 0x01,
    CLASS_X87    = 0x02,
    CLASS_MM     = 0x04,
    CLASS_XMM    = 0x08,
  };

  enum FLAGS
  {
    FLAG_SP_FP  = 0x10,
    FLAG_DP_FP  = 0x20,
    FLAG_VECTOR = 0x40
  };

  uint32_t code;
  uint8_t size;
  uint8_t clazz;
  uint8_t flags;
  uint8_t reserved_0;
  char name[8];
};

#define C(c) VariableInfo::CLASS_##c
#define F(f) VariableInfo::FLAG_##f
static const VariableInfo variableInfo[] =
{
  /*  0 */ { REG_TYPE_GPD   , 4 , C(GP) , 0                   , 0, "GP.D"        },
  /*  1 */ { REG_TYPE_GPQ   , 8 , C(GP) , 0                   , 0, "GP.Q"        },
  /*  2 */ { REG_TYPE_X87   , 4 , C(X87), F(SP_FP)            , 0, "X87"         },
  /*  3 */ { REG_TYPE_X87   , 4 , C(X87), F(SP_FP)            , 0, "X87.1F"      },
  /*  4 */ { REG_TYPE_X87   , 8 , C(X87), F(DP_FP)            , 0, "X87.1D"      },
  /*  5 */ { REG_TYPE_MM    , 8 , C(MM) ,            F(VECTOR), 0, "MM"          },
  /*  6 */ { REG_TYPE_XMM   , 16, C(XMM), 0                   , 0, "XMM"         },
  /*  7 */ { REG_TYPE_XMM   , 4 , C(XMM), F(SP_FP)            , 0, "XMM.1F"      },
  /*  8 */ { REG_TYPE_XMM   , 8 , C(XMM), F(DP_FP)            , 0, "XMM.1D"      },
  /*  9 */ { REG_TYPE_XMM   , 16, C(XMM), F(SP_FP) | F(VECTOR), 0, "XMM.4F"      },
  /* 10 */ { REG_TYPE_XMM   , 16, C(XMM), F(DP_FP) | F(VECTOR), 0, "XMM.2D"      }
};
#undef F
#undef C

static uint32_t getVariableClass(uint32_t type)
{
  ASMJIT_ASSERT(type < ASMJIT_ARRAY_SIZE(variableInfo));
  return variableInfo[type].clazz;
}

static uint32_t getVariableSize(uint32_t type)
{
  ASMJIT_ASSERT(type < ASMJIT_ARRAY_SIZE(variableInfo));
  return variableInfo[type].size;
}

static uint32_t getVariableRegisterCode(uint32_t type, uint32_t index)
{
  ASMJIT_ASSERT(type < ASMJIT_ARRAY_SIZE(variableInfo));
  return variableInfo[type].code | index;
}

static bool isVariableInteger(uint32_t type)
{
  ASMJIT_ASSERT(type < ASMJIT_ARRAY_SIZE(variableInfo));
  return (variableInfo[type].clazz & VariableInfo::CLASS_GP) != 0;
}

static bool isVariableFloat(uint32_t type)
{
  ASMJIT_ASSERT(type < ASMJIT_ARRAY_SIZE(variableInfo));
  return (variableInfo[type].flags & (VariableInfo::FLAG_SP_FP | VariableInfo::FLAG_DP_FP)) != 0;
}

static GPVar GPVarFromData(VarData* vdata)
{
  GPVar var;
  var._var.id = vdata->id;
  var._var.size = vdata->size;
  var._var.registerCode = variableInfo[vdata->type].code;
  var._var.variableType = vdata->type;
  return var;
}

static MMVar MMVarFromData(VarData* vdata)
{
  MMVar var;
  var._var.id = vdata->id;
  var._var.size = vdata->size;
  var._var.registerCode = variableInfo[vdata->type].code;
  var._var.variableType = vdata->type;
  return var;
}

static XMMVar XMMVarFromData(VarData* vdata)
{
  XMMVar var;
  var._var.id = vdata->id;
  var._var.size = vdata->size;
  var._var.registerCode = variableInfo[vdata->type].code;
  var._var.variableType = vdata->type;
  return var;
}

// ============================================================================
// [Helpers - Emittables]
// ============================================================================

static void delAll(Emittable* first) ASMJIT_NOTHROW
{
  Emittable* cur = first;

  while (cur)
  {
    Emittable* next = cur->getNext();
    cur->~Emittable();
    cur = next;
  }
}

// ============================================================================
// [Helpers - Compiler]
// ============================================================================

template<typename T>
inline T* Compiler_newObject(CompilerCore* self) ASMJIT_NOTHROW
{
  void* addr = self->getZone().zalloc(sizeof(T));
  return new(addr) T(reinterpret_cast<Compiler*>(self));
}

template<typename T, typename P1>
inline T* Compiler_newObject(CompilerCore* self, P1 p1) ASMJIT_NOTHROW
{
  void* addr = self->getZone().zalloc(sizeof(T));
  return new(addr) T(reinterpret_cast<Compiler*>(self), p1);
}

template<typename T, typename P1, typename P2>
inline T* Compiler_newObject(CompilerCore* self, P1 p1, P2 p2) ASMJIT_NOTHROW
{
  void* addr = self->getZone().zalloc(sizeof(T));
  return new(addr) T(reinterpret_cast<Compiler*>(self), p1, p2);
}

template<typename T, typename P1, typename P2, typename P3>
inline T* Compiler_newObject(CompilerCore* self, P1 p1, P2 p2, P3 p3) ASMJIT_NOTHROW
{
  void* addr = self->getZone().zalloc(sizeof(T));
  return new(addr) T(reinterpret_cast<Compiler*>(self), p1, p2, p3);
}

template<typename T, typename P1, typename P2, typename P3, typename P4>
inline T* Compiler_newObject(CompilerCore* self, P1 p1, P2 p2, P3 p3, P4 p4) ASMJIT_NOTHROW
{
  void* addr = self->getZone().zalloc(sizeof(T));
  return new(addr) T(reinterpret_cast<Compiler*>(self), p1, p2, p3, p4);
}

template<typename T, typename P1, typename P2, typename P3, typename P4, typename P5>
inline T* Compiler_newObject(CompilerCore* self, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) ASMJIT_NOTHROW
{
  void* addr = self->getZone().zalloc(sizeof(T));
  return new(addr) T(reinterpret_cast<Compiler*>(self), p1, p2, p3, p4, p5);
}

// ============================================================================
// [AsmJit::FunctionPrototype]
// ============================================================================

FunctionPrototype::FunctionPrototype() ASMJIT_NOTHROW
{
  // Safe defaults.
  _clear();
}

FunctionPrototype::~FunctionPrototype() ASMJIT_NOTHROW
{
}

void FunctionPrototype::setPrototype(
  uint32_t callingConvention,
  const uint32_t* arguments,
  uint32_t argumentsCount,
  uint32_t returnValue) ASMJIT_NOTHROW
{
  _setCallingConvention(callingConvention);

  if (argumentsCount > 32) argumentsCount = 32;
  _setPrototype(arguments, argumentsCount, returnValue);
}

uint32_t FunctionPrototype::findArgumentByRegisterCode(uint32_t regCode) const ASMJIT_NOTHROW
{
  uint32_t type = regCode & REG_TYPE_MASK;
  uint32_t idx = regCode & REG_INDEX_MASK;

  uint32_t clazz;
  uint32_t i;

  switch (type)
  {
    case REG_TYPE_GPD:
    case REG_TYPE_GPQ:
      clazz = VariableInfo::CLASS_GP;
      break;

    case REG_TYPE_MM:
      clazz = VariableInfo::CLASS_MM;
      break;

    case REG_TYPE_XMM:
      clazz = VariableInfo::CLASS_XMM;
      break;

    default:
      return INVALID_VALUE;
  }

  for (i = 0; i < _argumentsCount; i++)
  {
    const Argument& arg = _arguments[i];
    if ((getVariableClass(arg.variableType) & clazz) != 0 && (arg.registerIndex == idx))
      return i;
  }

  return INVALID_VALUE;
}

void FunctionPrototype::_clear() ASMJIT_NOTHROW
{
  _callingConvention = CALL_CONV_NONE;
  _calleePopsStack = false;

  _argumentsCount = 0;
  _argumentsDirection = ARGUMENT_DIR_RIGHT_TO_LEFT;
  _argumentsStackSize = 0;

  _returnValue = INVALID_VALUE;

  Util::memset32(_argumentsGPList , INVALID_VALUE, ASMJIT_ARRAY_SIZE(_argumentsGPList ));
  Util::memset32(_argumentsXMMList, INVALID_VALUE, ASMJIT_ARRAY_SIZE(_argumentsXMMList));

  _argumentsGP  = 0;
  _argumentsMM  = 0;
  _argumentsXMM = 0;

  _preservedGP  = 0;
  _preservedMM  = 0;
  _preservedXMM = 0;

  _passedGP  = 0;
  _passedMM  = 0;
  _passedXMM = 0;
}

void FunctionPrototype::_setCallingConvention(uint32_t callingConvention) ASMJIT_NOTHROW
{
  // Safe defaults.
  _clear();

  _callingConvention = callingConvention;

  // --------------------------------------------------------------------------
  // [X86 Calling Conventions]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_X86)
  _preservedGP  = (1 << REG_INDEX_EBX) |
                  (1 << REG_INDEX_ESP) |
                  (1 << REG_INDEX_EBP) |
                  (1 << REG_INDEX_ESI) |
                  (1 << REG_INDEX_EDI) ;
  _preservedXMM = 0;

  switch (_callingConvention)
  {
    case CALL_CONV_CDECL:
      break;

    case CALL_CONV_STDCALL:
      _calleePopsStack = true;
      break;

    case CALL_CONV_MSTHISCALL:
      _calleePopsStack = true;
      _argumentsGPList[0] = REG_INDEX_ECX;

      _argumentsGP =  (1 << REG_INDEX_ECX);
      break;

    case CALL_CONV_MSFASTCALL:
      _calleePopsStack = true;
      _argumentsGPList[0] = REG_INDEX_ECX;
      _argumentsGPList[1] = REG_INDEX_EDX;

      _argumentsGP =  (1 << REG_INDEX_ECX) |
                      (1 << REG_INDEX_EDX) ;
      break;

    case CALL_CONV_BORLANDFASTCALL:
      _calleePopsStack = true;
      _argumentsDirection = ARGUMENT_DIR_LEFT_TO_RIGHT;
      _argumentsGPList[0] = REG_INDEX_EAX;
      _argumentsGPList[1] = REG_INDEX_EDX;
      _argumentsGPList[2] = REG_INDEX_ECX;

      _argumentsGP =  (1 << REG_INDEX_EAX) |
                      (1 << REG_INDEX_EDX) |
                      (1 << REG_INDEX_ECX) ;
      break;

    case CALL_CONV_GCCFASTCALL:
      _calleePopsStack = true;
      _argumentsGPList[0] = REG_INDEX_ECX;
      _argumentsGPList[1] = REG_INDEX_EDX;

      _argumentsGP =  (1 << REG_INDEX_ECX) |
                      (1 << REG_INDEX_EDX) ;
      break;

    case CALL_CONV_GCCREGPARM_1:
      _calleePopsStack = false;
      _argumentsGPList[0] = REG_INDEX_EAX;

      _argumentsGP =  (1 << REG_INDEX_EAX) ;
      break;

    case CALL_CONV_GCCREGPARM_2:
      _calleePopsStack = false;
      _argumentsGPList[0] = REG_INDEX_EAX;
      _argumentsGPList[1] = REG_INDEX_EDX;

      _argumentsGP =  (1 << REG_INDEX_EAX) |
                      (1 << REG_INDEX_EDX) ;
      break;

    case CALL_CONV_GCCREGPARM_3:
      _calleePopsStack = false;
      _argumentsGPList[0] = REG_INDEX_EAX;
      _argumentsGPList[1] = REG_INDEX_EDX;
      _argumentsGPList[2] = REG_INDEX_ECX;

      _argumentsGP =  (1 << REG_INDEX_EAX) |
                      (1 << REG_INDEX_EDX) |
                      (1 << REG_INDEX_ECX) ;
      break;

    default:
      // Illegal calling convention.
      ASMJIT_ASSERT(0);
  }
#endif // ASMJIT_X86

  // --------------------------------------------------------------------------
  // [X64 Calling Conventions]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_X64)
  switch (_callingConvention)
  {
    case CALL_CONV_X64W:
      _argumentsGPList[0] = REG_INDEX_RCX;
      _argumentsGPList[1] = REG_INDEX_RDX;
      _argumentsGPList[2] = REG_INDEX_R8;
      _argumentsGPList[3] = REG_INDEX_R9;

      _argumentsXMMList[0] = REG_INDEX_XMM0;
      _argumentsXMMList[1] = REG_INDEX_XMM1;
      _argumentsXMMList[2] = REG_INDEX_XMM2;
      _argumentsXMMList[3] = REG_INDEX_XMM3;

      _argumentsGP =  (1 << REG_INDEX_RCX  ) |
                      (1 << REG_INDEX_RDX  ) |
                      (1 << REG_INDEX_R8   ) |
                      (1 << REG_INDEX_R9   ) ;

      _argumentsXMM = (1 << REG_INDEX_XMM0 ) |
                      (1 << REG_INDEX_XMM1 ) |
                      (1 << REG_INDEX_XMM2 ) |
                      (1 << REG_INDEX_XMM3 ) ;

      _preservedGP =  (1 << REG_INDEX_RBX  ) |
                      (1 << REG_INDEX_RSP  ) |
                      (1 << REG_INDEX_RBP  ) |
                      (1 << REG_INDEX_RSI  ) |
                      (1 << REG_INDEX_RDI  ) |
                      (1 << REG_INDEX_R12  ) |
                      (1 << REG_INDEX_R13  ) |
                      (1 << REG_INDEX_R14  ) |
                      (1 << REG_INDEX_R15  ) ;

      _preservedXMM = (1 << REG_INDEX_XMM6 ) |
                      (1 << REG_INDEX_XMM7 ) |
                      (1 << REG_INDEX_XMM8 ) |
                      (1 << REG_INDEX_XMM9 ) |
                      (1 << REG_INDEX_XMM10) |
                      (1 << REG_INDEX_XMM11) |
                      (1 << REG_INDEX_XMM12) |
                      (1 << REG_INDEX_XMM13) |
                      (1 << REG_INDEX_XMM14) |
                      (1 << REG_INDEX_XMM15) ;
      break;

    case CALL_CONV_X64U:
      _argumentsGPList[0] = REG_INDEX_RDI;
      _argumentsGPList[1] = REG_INDEX_RSI;
      _argumentsGPList[2] = REG_INDEX_RDX;
      _argumentsGPList[3] = REG_INDEX_RCX;
      _argumentsGPList[4] = REG_INDEX_R8;
      _argumentsGPList[5] = REG_INDEX_R9;

      _argumentsXMMList[0] = REG_INDEX_XMM0;
      _argumentsXMMList[1] = REG_INDEX_XMM1;
      _argumentsXMMList[2] = REG_INDEX_XMM2;
      _argumentsXMMList[3] = REG_INDEX_XMM3;
      _argumentsXMMList[4] = REG_INDEX_XMM4;
      _argumentsXMMList[5] = REG_INDEX_XMM5;
      _argumentsXMMList[6] = REG_INDEX_XMM6;
      _argumentsXMMList[7] = REG_INDEX_XMM7;

      _argumentsGP =  (1 << REG_INDEX_RDI  ) |
                      (1 << REG_INDEX_RSI  ) |
                      (1 << REG_INDEX_RDX  ) |
                      (1 << REG_INDEX_RCX  ) |
                      (1 << REG_INDEX_R8   ) |
                      (1 << REG_INDEX_R9   ) ;

      _argumentsXMM = (1 << REG_INDEX_XMM0 ) |
                      (1 << REG_INDEX_XMM1 ) |
                      (1 << REG_INDEX_XMM2 ) |
                      (1 << REG_INDEX_XMM3 ) |
                      (1 << REG_INDEX_XMM4 ) |
                      (1 << REG_INDEX_XMM5 ) |
                      (1 << REG_INDEX_XMM6 ) |
                      (1 << REG_INDEX_XMM7 ) ;

      _preservedGP =  (1 << REG_INDEX_RBX  ) |
                      (1 << REG_INDEX_RSP  ) |
                      (1 << REG_INDEX_RBP  ) |
                      (1 << REG_INDEX_R12  ) |
                      (1 << REG_INDEX_R13  ) |
                      (1 << REG_INDEX_R14  ) |
                      (1 << REG_INDEX_R15  ) ;
      break;

    default:
      // Illegal calling convention.
      ASMJIT_ASSERT(0);
  }
#endif // ASMJIT_X64
}

void FunctionPrototype::_setPrototype(
  const uint32_t* argumentsData,
  uint32_t argumentsCount,
  uint32_t returnValue) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(argumentsCount <= 32);

  int32_t i;

  int32_t posGP = 0;
  int32_t posXMM = 0;
  int32_t stackOffset = 0;

  _returnValue = returnValue;

  for (i = 0; i < (sysint_t)argumentsCount; i++)
  {
    Argument& a = _arguments[i];
    a.variableType = argumentsData[i];
    a.registerIndex = INVALID_VALUE;
    a.stackOffset = INVALID_VALUE;
  }

  _argumentsCount = (uint32_t)argumentsCount;
  if (_argumentsCount == 0) return;

  // --------------------------------------------------------------------------
  // [X86 Calling Conventions (32-bit)]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_X86)
  // Register arguments (Integer), always left-to-right.
  for (i = 0; i != argumentsCount; i++)
  {
    Argument& a = _arguments[i];
    if (isVariableInteger(a.variableType) && posGP < 16 && _argumentsGPList[posGP] != INVALID_VALUE)
    {
      a.registerIndex = _argumentsGPList[posGP++];
      _passedGP |= Util::maskFromIndex(a.registerIndex);
    }
  }

  // Stack arguments.
  bool ltr = _argumentsDirection == ARGUMENT_DIR_LEFT_TO_RIGHT;
  sysint_t istart = ltr ? 0 : (sysint_t)argumentsCount - 1;
  sysint_t iend   = ltr ? (sysint_t)argumentsCount : -1;
  sysint_t istep  = ltr ? 1 : -1;

  for (i = istart; i != iend; i += istep)
  {
    Argument& a = _arguments[i];
    if (a.registerIndex != INVALID_VALUE) continue;

    if (isVariableInteger(a.variableType))
    {
      stackOffset -= 4;
      a.stackOffset = stackOffset;
    }
    else if (isVariableFloat(a.variableType))
    {
      int32_t size = (int32_t)variableInfo[a.variableType].size;
      stackOffset -= size;
      a.stackOffset = stackOffset;
    }
  }
#endif // ASMJIT_X86

  // --------------------------------------------------------------------------
  // [X64 Calling Conventions (64-bit)]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_X64)
  // Windows 64-bit specific.
  if (_callingConvention == CALL_CONV_X64W)
  {
    sysint_t max = argumentsCount < 4 ? argumentsCount : 4;

    // Register arguments (Integer / FP), always left to right.
    for (i = 0; i != max; i++)
    {
      Argument& a = _arguments[i];

      if (isVariableInteger(a.variableType))
      {
        a.registerIndex = _argumentsGPList[i];
        _passedGP |= Util::maskFromIndex(a.registerIndex);
      }
      else if (isVariableFloat(a.variableType))
      {
        a.registerIndex = _argumentsXMMList[i];
        _passedXMM |= Util::maskFromIndex(a.registerIndex);
      }
    }

    // Stack arguments (always right-to-left).
    for (i = argumentsCount - 1; i != -1; i--)
    {
      Argument& a = _arguments[i];
      if (a.isAssigned()) continue;

      if (isVariableInteger(a.variableType))
      {
        stackOffset -= 8; // Always 8 bytes.
        a.stackOffset = stackOffset;
      }
      else if (isVariableFloat(a.variableType))
      {
        int32_t size = (int32_t)variableInfo[a.variableType].size;
        stackOffset -= size;
        a.stackOffset = stackOffset;
      }
    }

    // 32 bytes shadow space (X64W calling convention specific).
    stackOffset -= 4 * 8;
  }
  // Linux/Unix 64-bit (AMD64 calling convention).
  else
  {
    // Register arguments (Integer), always left to right.
    for (i = 0; i != argumentsCount; i++)
    {
      Argument& a = _arguments[i];
      if (isVariableInteger(a.variableType) && posGP < 32 && _argumentsGPList[posGP] != INVALID_VALUE)
      {
        a.registerIndex = _argumentsGPList[posGP++];
        _passedGP |= Util::maskFromIndex(a.registerIndex);
      }
    }

    // Register arguments (FP), always left to right.
    for (i = 0; i != argumentsCount; i++)
    {
      Argument& a = _arguments[i];
      if (isVariableFloat(a.variableType))
      {
        a.registerIndex = _argumentsXMMList[posXMM++];
        _passedXMM |= Util::maskFromIndex(a.registerIndex);
      }
    }

    // Stack arguments.
    for (i = argumentsCount - 1; i != -1; i--)
    {
      Argument& a = _arguments[i];
      if (a.isAssigned()) continue;

      if (isVariableInteger(a.variableType))
      {
        stackOffset -= 8;
        a.stackOffset = stackOffset;
      }
      else if (isVariableFloat(a.variableType))
      {
        int32_t size = (int32_t)variableInfo[a.variableType].size;

        stackOffset -= size;
        a.stackOffset = stackOffset;
      }
    }
  }
#endif // ASMJIT_X64

  // Modify stack offset (all function parameters will be in positive stack
  // offset that is never zero).
  for (i = 0; i < (sysint_t)argumentsCount; i++)
  {
    if (_arguments[i].registerIndex == INVALID_VALUE)
      _arguments[i].stackOffset += sizeof(sysint_t) - stackOffset;
  }

  _argumentsStackSize = (uint32_t)(-stackOffset);
}

void FunctionPrototype::_setReturnValue(uint32_t valueId) ASMJIT_NOTHROW
{
  // TODO.
}

// ============================================================================
// [AsmJit::EVariableHint]
// ============================================================================

EVariableHint::EVariableHint(Compiler* c, VarData* vdata, uint32_t hintId, uint32_t hintValue) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_VARIABLE_HINT),
  _vdata(vdata),
  _hintId(hintId),
  _hintValue(hintValue)
{
  ASMJIT_ASSERT(_vdata != NULL);
}

EVariableHint::~EVariableHint() ASMJIT_NOTHROW
{
}

void EVariableHint::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset;

  // First emittable (begin of variable scope).
  if (_vdata->firstEmittable == NULL) _vdata->firstEmittable = this;

  Emittable* oldLast = _vdata->lastEmittable;

  // Last emittable (end of variable scope).
  _vdata->lastEmittable = this;

  switch (_hintId)
  {
    case VARIABLE_HINT_ALLOC:
    case VARIABLE_HINT_SPILL:
    case VARIABLE_HINT_SAVE:
      if (!cc._isActive(_vdata)) cc._addActive(_vdata);
      break;
    case VARIABLE_HINT_SAVE_AND_UNUSE:
      if (!cc._isActive(_vdata)) cc._addActive(_vdata);
      break;
    case VARIABLE_HINT_UNUSE:
      if (oldLast) oldLast->_tryUnuseVar(_vdata);
      break;
  }
}

Emittable* EVariableHint::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  switch (_hintId)
  {
    case VARIABLE_HINT_ALLOC:
      cc.allocVar(_vdata, _hintValue, VARIABLE_ALLOC_READ);
      break;
    case VARIABLE_HINT_SPILL:
      if (_vdata->state == VARIABLE_STATE_REGISTER)
        cc.spillVar(_vdata);
      break;
    case VARIABLE_HINT_SAVE:
    case VARIABLE_HINT_SAVE_AND_UNUSE:
      if (_vdata->state == VARIABLE_STATE_REGISTER && _vdata->changed)
      {
        cc.emitSaveVar(_vdata, _vdata->registerIndex);
        _vdata->changed = false;
      }
      if (_hintId == VARIABLE_HINT_SAVE_AND_UNUSE) goto unuse;
      break;
    case VARIABLE_HINT_UNUSE:
unuse:
      cc.unuseVar(_vdata, VARIABLE_STATE_UNUSED);
      goto end;
  }

  cc._unuseVarOnEndOfScope(this, _vdata);

end:
  return translated();
}

int EVariableHint::getMaxSize() const ASMJIT_NOTHROW
{
  // Variable hint is NOP, but it can generate other emittables which can do
  // something.
  return 0;
}

// ============================================================================
// [AsmJit::EInstruction]
// ============================================================================

EInstruction::EInstruction(Compiler* c, uint32_t code, Operand* operandsData, uint32_t operandsCount) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_INSTRUCTION)
{
  _code = code;
  _emitOptions = c->_emitOptions;
  // Each created instruction takes emit options and clears it.
  c->_emitOptions = 0;

  _operands = operandsData;
  _operandsCount = operandsCount;
  _memOp = NULL;

  _variables = NULL;
  _variablesCount = 0;

  uint32_t i;
  for (i = 0; i < operandsCount; i++)
  {
    if (_operands[i].isMem())
    {
      _memOp = reinterpret_cast<Mem*>(&_operands[i]);
      break;
    }
  }

  const InstructionDescription* id = &instructionDescription[_code];
  _isSpecial = id->isSpecial();
  _isFPU = id->isFPU();
  _isGPBLoUsed = false;
  _isGPBHiUsed = false;

  if (_isSpecial)
  {
    // ${SPECIAL_INSTRUCTION_HANDLING_BEGIN}
    switch (_code)
    {
      case INST_CPUID:
        // Special...
        break;

      case INST_CBW:
      case INST_CDQE:
      case INST_CWDE:
        // Special...
        break;

      case INST_CMPXCHG:
      case INST_CMPXCHG8B:
#if defined(ASMJIT_X64)
      case INST_CMPXCHG16B:
#endif // ASMJIT_X64
        // Special...
        break;

#if defined(ASMJIT_X86)
      case INST_DAA:
      case INST_DAS:
        // Special...
        break;
#endif // ASMJIT_X86

      case INST_IMUL:
        switch (operandsCount)
        {
          case 2:
            // IMUL dst, src is not special instruction.
            _isSpecial = false;
            break;
          case 3:
            if (!(_operands[0].isVar() && _operands[1].isVar() && _operands[2].isVarMem()))
            {
              // Only IMUL dst_hi, dst_lo, reg/mem is special, all others don't.
              _isSpecial = false;
            }
            break;
        }
        break;
      case INST_MUL:
      case INST_IDIV:
      case INST_DIV:
        // Special...
        break;

      case INST_MOV_PTR:
        // Special...
        break;

      case INST_LAHF:
      case INST_SAHF:
        // Special...
        break;

      case INST_MASKMOVQ:
      case INST_MASKMOVDQU:
        // Special...
        break;

      case INST_ENTER:
      case INST_LEAVE:
        // Special...
        break;

      case INST_RET:
        // Special...
        break;

      case INST_MONITOR:
      case INST_MWAIT:
        // Special...
        break;

      case INST_POP:
      case INST_POPAD:
      case INST_POPFD:
      case INST_POPFQ:
        // Special...
        break;

      case INST_PUSH:
      case INST_PUSHAD:
      case INST_PUSHFD:
      case INST_PUSHFQ:
        // Special...
        break;

      case INST_RCL:
      case INST_RCR:
      case INST_ROL:
      case INST_ROR:
      case INST_SAL:
      case INST_SAR:
      case INST_SHL:
      case INST_SHR:
        // Rot instruction is special only if last operand is variable (register).
        _isSpecial = _operands[1].isVar();
        break;

      case INST_SHLD:
      case INST_SHRD:
        // Shld/Shrd instruction is special only if last operand is variable (register).
        _isSpecial = _operands[2].isVar();
        break;

      case INST_RDTSC:
      case INST_RDTSCP:
        // Special...
        break;

      case INST_REP_LODSB:
      case INST_REP_LODSD:
      case INST_REP_LODSQ:
      case INST_REP_LODSW:
      case INST_REP_MOVSB:
      case INST_REP_MOVSD:
      case INST_REP_MOVSQ:
      case INST_REP_MOVSW:
      case INST_REP_STOSB:
      case INST_REP_STOSD:
      case INST_REP_STOSQ:
      case INST_REP_STOSW:
      case INST_REPE_CMPSB:
      case INST_REPE_CMPSD:
      case INST_REPE_CMPSQ:
      case INST_REPE_CMPSW:
      case INST_REPE_SCASB:
      case INST_REPE_SCASD:
      case INST_REPE_SCASQ:
      case INST_REPE_SCASW:
      case INST_REPNE_CMPSB:
      case INST_REPNE_CMPSD:
      case INST_REPNE_CMPSQ:
      case INST_REPNE_CMPSW:
      case INST_REPNE_SCASB:
      case INST_REPNE_SCASD:
      case INST_REPNE_SCASQ:
      case INST_REPNE_SCASW:
        // Special...
        break;

      default:
        ASMJIT_ASSERT(0);
    }
    // ${SPECIAL_INSTRUCTION_HANDLING_END}
  }
}

EInstruction::~EInstruction() ASMJIT_NOTHROW
{
}

void EInstruction::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
#define __GET_VARIABLE(__vardata__) \
  { \
    VarData* _candidate = __vardata__; \
    \
    for (var = cur; ;) \
    { \
      if (var == _variables) \
      { \
        var = cur++; \
        var->vdata = _candidate; \
        var->vflags = 0; \
        var->regMask = 0xFFFFFFFF; \
        break; \
      } \
      \
      var--; \
      \
      if (var->vdata == _candidate) \
      { \
        break; \
      } \
    } \
    \
    ASMJIT_ASSERT(var != NULL); \
  }

  _offset = cc._currentOffset;

  const InstructionDescription* id = &instructionDescription[_code];

  uint32_t i, len = _operandsCount;
  uint32_t variablesCount = 0;

  for (i = 0; i < len; i++)
  {
    Operand& o = _operands[i];

    if (o.isVar())
    {
      ASMJIT_ASSERT(o.getId() != INVALID_VALUE);
      VarData* vdata = _compiler->_getVarData(o.getId());
      ASMJIT_ASSERT(vdata != NULL);

      if (reinterpret_cast<BaseVar*>(&o)->isGPVar())
      {
        if (reinterpret_cast<GPVar*>(&o)->isGPBLo()) { _isGPBLoUsed = true; vdata->registerGPBLoCount++; };
        if (reinterpret_cast<GPVar*>(&o)->isGPBHi()) { _isGPBHiUsed = true; vdata->registerGPBHiCount++; };
      }

      if (vdata->workOffset != _offset)
      {
        if (!cc._isActive(vdata)) cc._addActive(vdata);

        vdata->workOffset = _offset;
        variablesCount++;
      }
    }
    else if (o.isMem())
    {
      if ((o.getId() & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o.getId());
        ASMJIT_ASSERT(vdata != NULL);

        cc._markMemoryUsed(vdata);

        if (vdata->workOffset != _offset)
        {
          if (!cc._isActive(vdata)) cc._addActive(vdata);

          vdata->workOffset = _offset;
          variablesCount++;
        }
      }
      else if ((o._mem.base & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o._mem.base);
        ASMJIT_ASSERT(vdata != NULL);

        if (vdata->workOffset != _offset)
        {
          if (!cc._isActive(vdata)) cc._addActive(vdata);

          vdata->workOffset = _offset;
          variablesCount++;
        }
      }

      if ((o._mem.index & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o._mem.index);
        ASMJIT_ASSERT(vdata != NULL);

        if (vdata->workOffset != _offset)
        {
          if (!cc._isActive(vdata)) cc._addActive(vdata);

          vdata->workOffset = _offset;
          variablesCount++;
        }
      }
    }
  }

  if (!variablesCount)
  {
    cc._currentOffset++;
    return;
  }

  _variables = reinterpret_cast<VarAllocRecord*>(_compiler->getZone().zalloc(sizeof(VarAllocRecord) * variablesCount));
  if (!_variables)
  {
    _compiler->setError(ERROR_NO_HEAP_MEMORY);
    cc._currentOffset++;
    return;
  }

  _variablesCount = variablesCount;

  VarAllocRecord* cur = _variables;
  VarAllocRecord* var = NULL;

  bool _isGPBUsed = _isGPBLoUsed | _isGPBHiUsed;
  uint32_t gpRestrictMask = Util::maskUpToIndex(REG_NUM_GP);

#if defined(ASMJIT_X64)
  if (_isGPBHiUsed)
  {
    gpRestrictMask &= Util::maskFromIndex(REG_INDEX_EAX) |
                      Util::maskFromIndex(REG_INDEX_EBX) |
                      Util::maskFromIndex(REG_INDEX_ECX) |
                      Util::maskFromIndex(REG_INDEX_EDX) |
                      Util::maskFromIndex(REG_INDEX_EBP) |
                      Util::maskFromIndex(REG_INDEX_ESI) |
                      Util::maskFromIndex(REG_INDEX_EDI) ;
  }
#endif // ASMJIT_X64

  for (i = 0; i < len; i++)
  {
    Operand& o = _operands[i];

    if (o.isVar())
    {
      VarData* vdata = _compiler->_getVarData(o.getId());
      ASMJIT_ASSERT(vdata != NULL);

      __GET_VARIABLE(vdata)
      var->vflags |= VARIABLE_ALLOC_REGISTER;

      if (_isGPBUsed)
      {
#if defined(ASMJIT_X86)
        if (reinterpret_cast<GPVar*>(&o)->isGPB())
        {
          var->regMask &= Util::maskFromIndex(REG_INDEX_EAX) |
                          Util::maskFromIndex(REG_INDEX_EBX) |
                          Util::maskFromIndex(REG_INDEX_ECX) |
                          Util::maskFromIndex(REG_INDEX_EDX) ;
        }
#else
        // Restrict all BYTE registers to RAX/RBX/RCX/RDX if HI BYTE register
        // is used (REX prefix makes HI BYTE addressing unencodable).
        if (_isGPBHiUsed)
        {
          if (reinterpret_cast<GPVar*>(&o)->isGPB())
          {
            var->regMask &= Util::maskFromIndex(REG_INDEX_EAX) |
                            Util::maskFromIndex(REG_INDEX_EBX) |
                            Util::maskFromIndex(REG_INDEX_ECX) |
                            Util::maskFromIndex(REG_INDEX_EDX) ;
          }
        }
#endif // ASMJIT_X86/X64
      }

      if (isSpecial())
      {
        // ${SPECIAL_INSTRUCTION_HANDLING_BEGIN}
        switch (_code)
        {
          case INST_CPUID:
            switch (i)
            {
              case 0:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EBX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 3:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDX);
                gpRestrictMask &= ~var->regMask;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_CBW:
          case INST_CDQE:
          case INST_CWDE:
            switch (i)
            {
              case 0:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_CMPXCHG:
            switch (i)
            {
              case 0:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE;
                break;
              case 2:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_CMPXCHG8B:
#if defined(ASMJIT_X64)
          case INST_CMPXCHG16B:
#endif // ASMJIT_X64
            switch (i)
            {
              case 0:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 3:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EBX);
                gpRestrictMask &= ~var->regMask;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

#if defined(ASMJIT_X86)
          case INST_DAA:
          case INST_DAS:
            ASMJIT_ASSERT(i == 0);
            vdata->registerRWCount++;
            var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
            var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
            gpRestrictMask &= ~var->regMask;
            break;
#endif // ASMJIT_X86

          case INST_IMUL:
          case INST_MUL:
          case INST_IDIV:
          case INST_DIV:
            switch (i)
            {
              case 0:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_MOV_PTR:
            switch (i)
            {
              case 0:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_LAHF:
            ASMJIT_ASSERT(i == 0);
            vdata->registerWriteCount++;
            var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
            var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
            gpRestrictMask &= ~var->regMask;
            break;

          case INST_SAHF:
            ASMJIT_ASSERT(i == 0);
            vdata->registerReadCount++;
            var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
            var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
            gpRestrictMask &= ~var->regMask;
            break;

          case INST_MASKMOVQ:
          case INST_MASKMOVDQU:
            switch (i)
            {
              case 0:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDI);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
              case 2:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ;
                break;
            }
            break;

          case INST_ENTER:
          case INST_LEAVE:
            // TODO: SPECIAL INSTRUCTION.
            break;

          case INST_RET:
            // TODO: SPECIAL INSTRUCTION.
            break;

          case INST_MONITOR:
          case INST_MWAIT:
            // TODO: MONITOR/MWAIT (COMPILER).
            break;

          case INST_POP:
            // TODO: SPECIAL INSTRUCTION.
            break;

          case INST_POPAD:
          case INST_POPFD:
          case INST_POPFQ:
            // TODO: SPECIAL INSTRUCTION.
            break;

          case INST_PUSH:
            // TODO: SPECIAL INSTRUCTION.
            break;

          case INST_PUSHAD:
          case INST_PUSHFD:
          case INST_PUSHFQ:
            // TODO: SPECIAL INSTRUCTION.
            break;

          case INST_RCL:
          case INST_RCR:
          case INST_ROL:
          case INST_ROR:
          case INST_SAL:
          case INST_SAR:
          case INST_SHL:
          case INST_SHR:
            switch (i)
            {
              case 0:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_SHLD:
          case INST_SHRD:
            switch (i)
            {
              case 0:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ;
                break;
              case 2:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_RDTSC:
          case INST_RDTSCP:
            switch (i)
            {
              case 0:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                ASMJIT_ASSERT(_code == INST_RDTSCP);
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;

              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_REP_LODSB:
          case INST_REP_LODSD:
          case INST_REP_LODSQ:
          case INST_REP_LODSW:
            switch (i)
            {
              case 0:
                vdata->registerWriteCount++;
                var->vflags |= VARIABLE_ALLOC_WRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ESI);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;
              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_REP_MOVSB:
          case INST_REP_MOVSD:
          case INST_REP_MOVSQ:
          case INST_REP_MOVSW:
          case INST_REPE_CMPSB:
          case INST_REPE_CMPSD:
          case INST_REPE_CMPSQ:
          case INST_REPE_CMPSW:
          case INST_REPNE_CMPSB:
          case INST_REPNE_CMPSD:
          case INST_REPNE_CMPSQ:
          case INST_REPNE_CMPSW:
            switch (i)
            {
              case 0:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDI);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ESI);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;
              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_REP_STOSB:
          case INST_REP_STOSD:
          case INST_REP_STOSQ:
          case INST_REP_STOSW:
            switch (i)
            {
              case 0:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDI);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;
              default:
                ASMJIT_ASSERT(0);
            }
            break;

          case INST_REPE_SCASB:
          case INST_REPE_SCASD:
          case INST_REPE_SCASQ:
          case INST_REPE_SCASW:
          case INST_REPNE_SCASB:
          case INST_REPNE_SCASD:
          case INST_REPNE_SCASQ:
          case INST_REPNE_SCASW:
            switch (i)
            {
              case 0:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EDI);
                gpRestrictMask &= ~var->regMask;
                break;
              case 1:
                vdata->registerReadCount++;
                var->vflags |= VARIABLE_ALLOC_READ | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_EAX);
                gpRestrictMask &= ~var->regMask;
                break;
              case 2:
                vdata->registerRWCount++;
                var->vflags |= VARIABLE_ALLOC_READWRITE | VARIABLE_ALLOC_SPECIAL;
                var->regMask = Util::maskFromIndex(REG_INDEX_ECX);
                gpRestrictMask &= ~var->regMask;
                break;
              default:
                ASMJIT_ASSERT(0);
            }
            break;

          default:
            ASMJIT_ASSERT(0);
        }
        // ${SPECIAL_INSTRUCTION_HANDLING_END}
      }
      else
      {
        if (i == 0)
        {
          // CMP/TEST instruction.
          if (id->code == INST_CMP || id->code == INST_TEST)
          {
            // Read-only case.
            vdata->registerReadCount++;
            var->vflags |= VARIABLE_ALLOC_READ;
          }
          // CVTTSD2SI/CVTTSS2SI instructions.
          else if (id->code == INST_CVTTSD2SI || id->code == INST_CVTTSS2SI)
          {
            // In 32-bit mode the whole destination is replaced. In 64-bit mode
            // we need to check whether the destination operand size is 64-bits.
#if defined(ASMJIT_X64)
            if (_operands[0].isRegType(REG_TYPE_GPQ))
            {
#endif // ASMJIT_X64
              // Write-only case.
              vdata->registerWriteCount++;
              var->vflags |= VARIABLE_ALLOC_WRITE;
#if defined(ASMJIT_X64)
            }
            else
            {
              // Read/Write.
              vdata->registerRWCount++;
              var->vflags |= VARIABLE_ALLOC_READWRITE;
            }
#endif // ASMJIT_X64
          }
          // MOV/MOVSS/MOVSD instructions.
          //
          // If instruction is MOV (source replaces the destination) or 
          // MOVSS/MOVSD and source operand is memory location then register
          // allocator should know that previous destination value is lost 
          // (write only operation).
          else if ((id->isMov()) ||
                  ((id->code == INST_MOVSS || id->code == INST_MOVSD) /* && _operands[1].isMem() */) ||
                  (id->code == INST_IMUL && _operandsCount == 3 && !isSpecial()))
          {
            // Write-only case.
            vdata->registerWriteCount++;
            var->vflags |= VARIABLE_ALLOC_WRITE;
          }
          else if (id->code == INST_LEA)
          {
            // Write.
            vdata->registerWriteCount++;
            var->vflags |= VARIABLE_ALLOC_WRITE;
          }
          else
          {
            // Read/Write.
            vdata->registerRWCount++;
            var->vflags |= VARIABLE_ALLOC_READWRITE;
          }
        }
        else
        {
          // Second, third, ... operands are read-only.
          vdata->registerReadCount++;
          var->vflags |= VARIABLE_ALLOC_READ;
        }

        if (!_memOp && i < 2 && (id->oflags[i] & InstructionDescription::O_MEM) != 0)
        {
          var->vflags |= VARIABLE_ALLOC_MEMORY;
        }
      }

      // If variable must be in specific register we could add some hint to allocator.
      if (var->vflags & VARIABLE_ALLOC_SPECIAL)
      {
        vdata->prefRegisterMask |= Util::maskFromIndex(var->regMask);
        cc._newRegisterHomeIndex(vdata, Util::findFirstBit(var->regMask));
      }
    }
    else if (o.isMem())
    {
      if ((o.getId() & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o.getId());
        ASMJIT_ASSERT(vdata != NULL);

        __GET_VARIABLE(vdata)

        if (i == 0)
        {
          // If variable is MOV instruction type (source replaces the destination)
          // or variable is MOVSS/MOVSD instruction then register allocator should
          // know that previous destination value is lost (write only operation).
          if (id->isMov() || ((id->code == INST_MOVSS || id->code == INST_MOVSD)))
          {
            // Write only case.
            vdata->memoryWriteCount++;
          }
          else
          {
            vdata->memoryRWCount++;
          }
        }
        else
        {
          vdata->memoryReadCount++;
        }
      }
      else if ((o._mem.base & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(reinterpret_cast<Mem&>(o).getBase());
        ASMJIT_ASSERT(vdata != NULL);

        __GET_VARIABLE(vdata)
        vdata->registerReadCount++;
        var->vflags |= VARIABLE_ALLOC_REGISTER | VARIABLE_ALLOC_READ;
        var->regMask &= gpRestrictMask;
      }

      if ((o._mem.index & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(reinterpret_cast<Mem&>(o).getIndex());
        ASMJIT_ASSERT(vdata != NULL);

        __GET_VARIABLE(vdata)
        vdata->registerReadCount++;
        var->vflags |= VARIABLE_ALLOC_REGISTER | VARIABLE_ALLOC_READ;
        var->regMask &= gpRestrictMask;
      }
    }
  }

  // Traverse all variables and update firstEmittable / lastEmittable. This
  // function is called from iterator that scans emittables using forward
  // direction so we can use this knowledge to optimize the process.
  //
  // Similar to ECall::prepare().
  for (i = 0; i < _variablesCount; i++)
  {
    VarData* v = _variables[i].vdata;

    // Update GP register allocator restrictions.
    if (isVariableInteger(v->type))
    {
      if (_variables[i].regMask == 0xFFFFFFFF) _variables[i].regMask &= gpRestrictMask;
    }

    // Update first/last emittable (begin of variable scope).
    if (v->firstEmittable == NULL) v->firstEmittable = this;
    v->lastEmittable = this;
  }

  // There are some instructions that can be used to clear register or to set
  // register to some value (ideal case is all zeros or all ones).
  //
  // xor/pxor reg, reg    ; Set all bits in reg to 0.
  // sub/psub reg, reg    ; Set all bits in reg to 0.
  // andn reg, reg        ; Set all bits in reg to 0.
  // pcmpgt reg, reg      ; Set all bits in reg to 0.
  // pcmpeq reg, reg      ; Set all bits in reg to 1.

  if (_variablesCount == 1 &&
      _operandsCount > 1 &&
      _operands[0].isVar() &&
      _operands[1].isVar() &&
      !_memOp)
  {
    switch (_code)
    {
      // XOR Instructions.
      case INST_XOR:
      case INST_XORPD:
      case INST_XORPS:
      case INST_PXOR:

      // ANDN Instructions.
      case INST_PANDN:

      // SUB Instructions.
      case INST_SUB:
      case INST_PSUBB:
      case INST_PSUBW:
      case INST_PSUBD:
      case INST_PSUBQ:
      case INST_PSUBSB:
      case INST_PSUBSW:
      case INST_PSUBUSB:
      case INST_PSUBUSW:

      // PCMPEQ Instructions.
      case INST_PCMPEQB:
      case INST_PCMPEQW:
      case INST_PCMPEQD:
      case INST_PCMPEQQ:

      // PCMPGT Instructions.
      case INST_PCMPGTB:
      case INST_PCMPGTW:
      case INST_PCMPGTD:
      case INST_PCMPGTQ:
        // Clear the read flag. This prevents variable alloc/spill.
        _variables[0].vflags = VARIABLE_ALLOC_WRITE;
        _variables[0].vdata->registerReadCount--;
        break;
    }
  }
  cc._currentOffset++;

#undef __GET_VARIABLE
}

Emittable* EInstruction::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  uint32_t i;
  uint32_t variablesCount = _variablesCount;

  if (variablesCount > 0)
  {
    // These variables are used by the instruction and we set current offset
    // to their work offsets -> getSpillCandidate never return the variable
    // used this instruction.
    for (i = 0; i < variablesCount; i++)
    {
      _variables->vdata->workOffset = cc._currentOffset;
    }

    // Alloc variables used by the instruction (special first).
    for (i = 0; i < variablesCount; i++)
    {
      VarAllocRecord& r = _variables[i];
      // Alloc variables with specific register first.
      if ((r.vflags & VARIABLE_ALLOC_SPECIAL) != 0)
        cc.allocVar(r.vdata, r.regMask, r.vflags);
    }

    for (i = 0; i < variablesCount; i++)
    {
      VarAllocRecord& r = _variables[i];
      // Alloc variables without specific register last.
      if ((r.vflags & VARIABLE_ALLOC_SPECIAL) == 0)
        cc.allocVar(r.vdata, r.regMask, r.vflags);
    }

    cc.translateOperands(_operands, _operandsCount);
  }

  if (_memOp && (_memOp->getId() & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
  {
    VarData* vdata = _compiler->_getVarData(_memOp->getId());
    ASMJIT_ASSERT(vdata != NULL);

    switch (vdata->state)
    {
      case VARIABLE_STATE_UNUSED:
        vdata->state = VARIABLE_STATE_MEMORY;
        break;
      case VARIABLE_STATE_REGISTER:
        vdata->changed = false;
        cc.unuseVar(vdata, VARIABLE_STATE_MEMORY);
        break;
    }
  }

  for (i = 0; i < variablesCount; i++)
  {
    cc._unuseVarOnEndOfScope(this, &_variables[i]);
  }

  return translated();
}

void EInstruction::emit(Assembler& a) ASMJIT_NOTHROW
{
  a._comment = _comment;
  a._emitOptions = _emitOptions;

  if (isSpecial())
  {
    // ${SPECIAL_INSTRUCTION_HANDLING_BEGIN}
    switch (_code)
    {
      case INST_CPUID:
        a._emitInstruction(_code);
        return;

      case INST_CBW:
      case INST_CDQE:
      case INST_CWDE:
        a._emitInstruction(_code);
        return;

      case INST_CMPXCHG:
        a._emitInstruction(_code, &_operands[1], &_operands[2]);
        return;

      case INST_CMPXCHG8B:
#if defined(ASMJIT_X64)
      case INST_CMPXCHG16B:
#endif // ASMJIT_X64
        a._emitInstruction(_code, &_operands[4]);
        return;

#if defined(ASMJIT_X86)
      case INST_DAA:
      case INST_DAS:
        a._emitInstruction(_code);
        return;
#endif // ASMJIT_X86

      case INST_IMUL:
      case INST_MUL:
      case INST_IDIV:
      case INST_DIV:
        // INST dst_lo (implicit), dst_hi (implicit), src (explicit)
        ASMJIT_ASSERT(_operandsCount == 3);
        a._emitInstruction(_code, &_operands[2]);
        return;

      case INST_MOV_PTR:
        break;

      case INST_LAHF:
      case INST_SAHF:
        a._emitInstruction(_code);
        return;

      case INST_MASKMOVQ:
      case INST_MASKMOVDQU:
        a._emitInstruction(_code, &_operands[1], &_operands[2]);
        return;

      case INST_ENTER:
      case INST_LEAVE:
        // TODO: SPECIAL INSTRUCTION.
        break;

      case INST_RET:
        // TODO: SPECIAL INSTRUCTION.
        break;

      case INST_MONITOR:
      case INST_MWAIT:
        // TODO: MONITOR/MWAIT (COMPILER).
        break;

      case INST_POP:
      case INST_POPAD:
      case INST_POPFD:
      case INST_POPFQ:
        // TODO: SPECIAL INSTRUCTION.
        break;

      case INST_PUSH:
      case INST_PUSHAD:
      case INST_PUSHFD:
      case INST_PUSHFQ:
        // TODO: SPECIAL INSTRUCTION.
        break;

      case INST_RCL:
      case INST_RCR:
      case INST_ROL:
      case INST_ROR:
      case INST_SAL:
      case INST_SAR:
      case INST_SHL:
      case INST_SHR:
        a._emitInstruction(_code, &_operands[0], &cl);
        return;

      case INST_SHLD:
      case INST_SHRD:
        a._emitInstruction(_code, &_operands[0], &_operands[1], &cl);
        return;

      case INST_RDTSC:
      case INST_RDTSCP:
        a._emitInstruction(_code);
        return;

      case INST_REP_LODSB:
      case INST_REP_LODSD:
      case INST_REP_LODSQ:
      case INST_REP_LODSW:
      case INST_REP_MOVSB:
      case INST_REP_MOVSD:
      case INST_REP_MOVSQ:
      case INST_REP_MOVSW:
      case INST_REP_STOSB:
      case INST_REP_STOSD:
      case INST_REP_STOSQ:
      case INST_REP_STOSW:
      case INST_REPE_CMPSB:
      case INST_REPE_CMPSD:
      case INST_REPE_CMPSQ:
      case INST_REPE_CMPSW:
      case INST_REPE_SCASB:
      case INST_REPE_SCASD:
      case INST_REPE_SCASQ:
      case INST_REPE_SCASW:
      case INST_REPNE_CMPSB:
      case INST_REPNE_CMPSD:
      case INST_REPNE_CMPSQ:
      case INST_REPNE_CMPSW:
      case INST_REPNE_SCASB:
      case INST_REPNE_SCASD:
      case INST_REPNE_SCASQ:
      case INST_REPNE_SCASW:
        a._emitInstruction(_code);
        return;

      default:
        ASMJIT_ASSERT(0);
    }
    // ${SPECIAL_INSTRUCTION_HANDLING_END}
  }

  switch (_operandsCount)
  {
    case 0:
      a._emitInstruction(_code);
      break;
    case 1:
      a._emitInstruction(_code, &_operands[0]);
      break;
    case 2:
      a._emitInstruction(_code, &_operands[0], &_operands[1]);
      break;
    case 3:
      a._emitInstruction(_code, &_operands[0], &_operands[1], &_operands[2]);
      break;
    default:
      ASMJIT_ASSERT(0);
      break;
  }
}

int EInstruction::getMaxSize() const ASMJIT_NOTHROW
{
  // TODO: Do something more exact.
  return 15;
}

bool EInstruction::_tryUnuseVar(VarData* v) ASMJIT_NOTHROW
{
  for (uint32_t i = 0; i < _variablesCount; i++)
  {
    if (_variables[i].vdata == v)
    {
      _variables[i].vflags |= VARIABLE_ALLOC_UNUSE_AFTER_USE;
      return true;
    }
  }

  return false;
}

ETarget* EInstruction::getJumpTarget() const ASMJIT_NOTHROW
{
  return NULL;
}

// ============================================================================
// [AsmJit::EJmp]
// ============================================================================

EJmp::EJmp(Compiler* c, uint32_t code, Operand* operandsData, uint32_t operandsCount) ASMJIT_NOTHROW :
  EInstruction(c, code, operandsData, operandsCount)
{
  _jumpTarget = _compiler->_getTarget(_operands[0].getId());
  _jumpTarget->_jumpsCount++;

  _jumpNext = _jumpTarget->_from;
  _jumpTarget->_from = this;

  // The 'jmp' is always taken, conditional jump can contain hint, we detect it.
  _isTaken = (getCode() == INST_JMP) || 
             (operandsCount > 1 &&
              operandsData[1].isImm() &&
              reinterpret_cast<Imm*>(&operandsData[1])->getValue() == HINT_TAKEN);
}

EJmp::~EJmp() ASMJIT_NOTHROW
{
}

void EJmp::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset;

  // Update _isTaken to true if this is conditional backward jump. This behavior
  // can be overridden by using HINT_NOT_TAKEN when using the instruction.
  if (getCode() != INST_JMP &&
      _operandsCount == 1 &&
      _jumpTarget->getOffset() < getOffset())
  {
    _isTaken = true;
  }

  // Now patch all variables where jump location is in the active range.
  if (_jumpTarget->getOffset() != INVALID_VALUE && cc._active)
  {
    VarData* first = cc._active;
    VarData* var = first;
    uint32_t jumpOffset = _jumpTarget->getOffset();

    do {
      if (var->firstEmittable)
      {
        ASMJIT_ASSERT(var->lastEmittable != NULL);
        uint32_t start = var->firstEmittable->getOffset();
        uint32_t end = var->lastEmittable->getOffset();

        if (jumpOffset >= start && jumpOffset <= end) var->lastEmittable = this;
      }
      var = var->nextActive;
    } while (var != first);
  }

  cc._currentOffset++;
}

Emittable* EJmp::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  // Translate using EInstruction.
  Emittable* ret = EInstruction::translate(cc);

  // We jump with emittable if its INST_JUMP (not based on condiiton) and it
  // points into yet unknown location.
  if (_code == INST_JMP && !_jumpTarget->isTranslated())
  {
    cc.addBackwardCode(this);
    ret = _jumpTarget;
  }
  else
  {
    _state = cc._saveState();
    if (_jumpTarget->isTranslated())
    {
      _doJump(cc);
    }
    else
    {
      // State is not known, so we need to call _doJump() later. Compiler will
      // do it for us.
      cc.addForwardJump(this);
      _jumpTarget->_state = _state;
    }

    // Mark next code as unreachable, cleared by a next label (ETarget).
    if (_code == INST_JMP) cc._unreachable = 1;
  }

  // Need to traverse over all active variables and unuse them if their scope ends
  // here. 
  if (cc._active)
  {
    VarData* first = cc._active;
    VarData* var = first;

    do {
      cc._unuseVarOnEndOfScope(this, var);
      var = var->nextActive;
    } while (var != first);
  }

  return ret;
}

void EJmp::emit(Assembler& a) ASMJIT_NOTHROW
{
  static const uint MAXIMUM_SHORT_JMP_SIZE = 127;

  // Try to minimize size of jump using SHORT jump (8-bit displacement) by 
  // traversing into the target and calculating the maximum code size. We
  // end when code size reaches MAXIMUM_SHORT_JMP_SIZE.
  if (!(_emitOptions & EMIT_OPTION_SHORT_JUMP) && getJumpTarget()->getOffset() > getOffset())
  {
    // Calculate the code size.
    uint codeSize = 0;
    Emittable* cur = this->getNext();
    Emittable* target = getJumpTarget();

    while (cur)
    {
      if (cur == target)
      {
        // Target found, we can tell assembler to generate short form of jump.
        _emitOptions |= EMIT_OPTION_SHORT_JUMP;
        goto end;
      }

      int s = cur->getMaxSize();
      if (s == -1) break;

      codeSize += (uint)s;
      if (codeSize > MAXIMUM_SHORT_JMP_SIZE) break;

      cur = cur->getNext();
    }
  }

end:
  EInstruction::emit(a);
}

void EJmp::_doJump(CompilerContext& cc) ASMJIT_NOTHROW
{
  // The state have to be already known. The _doJump() method is called by
  // translate() or by Compiler in case that it's forward jump.
  ASMJIT_ASSERT(_jumpTarget->getState());

  if (getCode() == INST_JMP || (isTaken() && _jumpTarget->getOffset() < getOffset()))
  {
    // Instruction type is JMP or conditional jump that should be taken (likely).
    // We can set state here instead of jumping out, setting state and jumping
    // to _jumpTarget.
    //
    // NOTE: We can't use this technique if instruction is forward conditional
    // jump. The reason is that when generating code we can't change state here,
    // because the next instruction depends on it.
    cc._restoreState(_jumpTarget->getState(), _jumpTarget->getOffset());
  }
  else
  {
    // Instruction type is JMP or conditional jump that should be not normally
    // taken. If we need add code that will switch between different states we
    // add it after the end of function body (after epilog, using 'ExtraBlock').
    Compiler* compiler = cc.getCompiler();

    Emittable* ext = cc.getExtraBlock();
    Emittable* old = compiler->setCurrentEmittable(ext);

    cc._restoreState(_jumpTarget->getState(), _jumpTarget->getOffset());

    if (compiler->getCurrentEmittable() != ext)
    {
      // Add the jump to the target.
      compiler->jmp(_jumpTarget->_label);
      ext = compiler->getCurrentEmittable();

      // The cc._restoreState() method emitted some instructions so we need to
      // patch the jump.
      Label L = compiler->newLabel();
      compiler->setCurrentEmittable(cc.getExtraBlock());
      compiler->bind(L);

      // Finally, patch the jump target.
      ASMJIT_ASSERT(_operandsCount > 0);
      _operands[0] = L;                              // Operand part (Label).
      _jumpTarget = compiler->_getTarget(L.getId()); // Emittable part (ETarget).
    }

    cc.setExtraBlock(ext);
    compiler->setCurrentEmittable(old);

    // Assign state back.
    cc._assignState(_state);
  }
}

ETarget* EJmp::getJumpTarget() const ASMJIT_NOTHROW
{
  return _jumpTarget;
}

// ============================================================================
// [AsmJit::EFunction]
// ============================================================================

EFunction::EFunction(Compiler* c) ASMJIT_NOTHROW : Emittable(c, EMITTABLE_FUNCTION)
{
  _argumentVariables = NULL;
  Util::memset32(_hints, INVALID_VALUE, ASMJIT_ARRAY_SIZE(_hints));

  // Stack is always aligned to 16-bytes when using 64-bit OS.
  _isStackAlignedByOsTo16Bytes = CompilerUtil::isStack16ByteAligned();

  // Manual aligning is autodetected by prepare() method.
  _isStackAlignedByFnTo16Bytes = false;

  // Just clear to safe defaults.
  _isNaked = false;
  _isEspAdjusted = false;
  _isCaller = false;

  _pePushPop = true;
  _emitEMMS = false;
  _emitSFence = false;
  _emitLFence = false;

  _finished = false;

  _modifiedAndPreservedGP = 0;
  _modifiedAndPreservedMM = 0;
  _modifiedAndPreservedXMM = 0;

  _pePushPopStackSize = 0;
  _peMovStackSize = 0;
  _peAdjustStackSize = 0;

  _memStackSize = 0;
  _memStackSize16 = 0;

  _functionCallStackSize = 0;

  _entryLabel = c->newLabel();
  _exitLabel = c->newLabel();

  _prolog = Compiler_newObject<EProlog>(c, this);
  _epilog = Compiler_newObject<EEpilog>(c, this);
  _end = Compiler_newObject<EFunctionEnd>(c);
}

EFunction::~EFunction() ASMJIT_NOTHROW
{
}

void EFunction::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset++;
}

int EFunction::getMaxSize() const ASMJIT_NOTHROW
{
  // EFunction is NOP.
  return 0;
}

void EFunction::setPrototype(
  uint32_t callingConvention,
  const uint32_t* arguments,
  uint32_t argumentsCount,
  uint32_t returnValue) ASMJIT_NOTHROW
{
  _functionPrototype.setPrototype(callingConvention, arguments, argumentsCount, returnValue);
}

void EFunction::setHint(uint32_t hint, uint32_t value) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(hint < ASMJIT_ARRAY_SIZE(_hints));
  _hints[hint] = value;
}

void EFunction::_createVariables() ASMJIT_NOTHROW
{
  uint32_t i, count = _functionPrototype.getArgumentsCount();
  if (count == 0) return;

  _argumentVariables = reinterpret_cast<VarData**>(_compiler->getZone().zalloc(count * sizeof(VarData*)));
  if (_argumentVariables == NULL)
  {
    _compiler->setError(ERROR_NO_HEAP_MEMORY);
    return;
  }

  char argNameStorage[64];
  char* argName = NULL;

  bool debug = _compiler->getLogger() != NULL;
  if (debug) argName = argNameStorage;

  for (i = 0; i < count; i++)
  {
    FunctionPrototype::Argument& a = _functionPrototype.getArguments()[i];
    if (debug) snprintf(argName, ASMJIT_ARRAY_SIZE(argNameStorage), "arg_%u", i);

    uint32_t size = getVariableSize(a.variableType);
    VarData* vdata = _compiler->_newVarData(argName, a.variableType, size);

    if (a.registerIndex != (uint32_t)INVALID_VALUE)
    {
      vdata->isRegArgument = true;
      vdata->registerIndex = a.registerIndex;
    }

    if (a.stackOffset != (int32_t)INVALID_VALUE)
    {
      vdata->isMemArgument = true;
      vdata->homeMemoryOffset = a.stackOffset;
    }

    _argumentVariables[i] = vdata;
  }
}

void EFunction::_prepareVariables(Emittable* first) ASMJIT_NOTHROW
{
  uint32_t i, count = _functionPrototype.getArgumentsCount();
  if (count == 0) return;

  for (i = 0; i < count; i++)
  {
    VarData* vdata = _argumentVariables[i];

    // This is where variable scope starts.
    vdata->firstEmittable = first;
    // If this will not be changed then it will be deallocated immediately.
    vdata->lastEmittable = first;
  }
}

void EFunction::_allocVariables(CompilerContext& cc) ASMJIT_NOTHROW
{
  uint32_t i, count = _functionPrototype.getArgumentsCount();
  if (count == 0) return;

  for (i = 0; i < count; i++)
  {
    VarData* vdata = _argumentVariables[i];

    if (vdata->firstEmittable != NULL ||
        vdata->isRegArgument ||
        vdata->isMemArgument)
    {
      // Variable is used.
      if (vdata->registerIndex != INVALID_VALUE)
      {
        vdata->state = VARIABLE_STATE_REGISTER;
        // If variable is in register -> mark it as changed so it will not be
        // lost by first spill.
        vdata->changed = true;
        cc._allocatedVariable(vdata);
      }
      else if (vdata->isMemArgument)
      {
        vdata->state = VARIABLE_STATE_MEMORY;
      }
    }
    else
    {
      // Variable is not used.
      vdata->registerIndex = INVALID_VALUE;
    }
  }
}

void EFunction::_preparePrologEpilog(CompilerContext& cc) ASMJIT_NOTHROW
{
  const CpuInfo* cpuInfo = getCpuInfo();

  _pePushPop = true;
  _emitEMMS = false;
  _emitSFence = false;
  _emitLFence = false;

  uint32_t accessibleMemoryBelowStack = 0;
  if (_functionPrototype.getCallingConvention() == CALL_CONV_X64U) 
    accessibleMemoryBelowStack = 128;

  if (_isCaller && (cc._memBytesTotal > 0 || _isStackAlignedByOsTo16Bytes))
    _isEspAdjusted = true;

  if (cc._memBytesTotal > accessibleMemoryBelowStack)
    _isEspAdjusted = true;

  if (_hints[FUNCTION_HINT_NAKED] != INVALID_VALUE)
    _isNaked = (bool)_hints[FUNCTION_HINT_NAKED];

  if (_hints[FUNCTION_HINT_PUSH_POP_SEQUENCE] != INVALID_VALUE)
    _pePushPop = (bool)_hints[FUNCTION_HINT_PUSH_POP_SEQUENCE];

  if (_hints[FUNCTION_HINT_EMMS] != INVALID_VALUE)
    _emitEMMS = (bool)_hints[FUNCTION_HINT_EMMS];

  if (_hints[FUNCTION_HINT_SFENCE] != INVALID_VALUE)
    _emitSFence = (bool)_hints[FUNCTION_HINT_SFENCE];

  if (_hints[FUNCTION_HINT_LFENCE] != INVALID_VALUE)
    _emitLFence = (bool)_hints[FUNCTION_HINT_LFENCE];

  // Updated to respect comment from issue #47, align also when using MMX code.
  if (!_isStackAlignedByOsTo16Bytes && !_isNaked && (cc._mem16BlocksCount + (cc._mem8BlocksCount > 0)))
  {
    // Have to align stack to 16-bytes.
    _isStackAlignedByFnTo16Bytes = true;
    _isEspAdjusted = true;
  }

  _modifiedAndPreservedGP  = cc._modifiedGPRegisters  & _functionPrototype.getPreservedGP() & ~Util::maskFromIndex(REG_INDEX_ESP);
  _modifiedAndPreservedMM  = cc._modifiedMMRegisters  & _functionPrototype.getPreservedMM();
  _modifiedAndPreservedXMM = cc._modifiedXMMRegisters & _functionPrototype.getPreservedXMM();

  _movDqaInstruction = (_isStackAlignedByOsTo16Bytes || !_isNaked) ? INST_MOVDQA : INST_MOVDQU;

  // Prolog & Epilog stack size.
  {
    int32_t memGP = Util::bitCount(_modifiedAndPreservedGP) * sizeof(sysint_t);
    int32_t memMM = Util::bitCount(_modifiedAndPreservedMM) * 8;
    int32_t memXMM = Util::bitCount(_modifiedAndPreservedXMM) * 16;

    if (_pePushPop)
    {
      _pePushPopStackSize = memGP;
      _peMovStackSize = memXMM + Util::alignTo16(memMM);
    }
    else
    {
      _pePushPopStackSize = 0;
      _peMovStackSize = memXMM + Util::alignTo16(memMM + memGP);
    }
  }

  if (_isStackAlignedByFnTo16Bytes)
  {
    _peAdjustStackSize += Util::deltaTo16(_pePushPopStackSize);
  }
  else
  {
    int32_t v = 16 - sizeof(sysint_t);
    if (!_isNaked) v -= sizeof(sysint_t);

    v -= _pePushPopStackSize & 15;
    if (v < 0) v += 16;
    _peAdjustStackSize = v;

    //_peAdjustStackSize += Util::deltaTo16(_pePushPopStackSize + v);
  }

  // Memory stack size.
  _memStackSize = cc._memBytesTotal;
  _memStackSize16 = Util::alignTo16(_memStackSize);

  if (_isNaked)
  {
    cc._argumentsBaseReg = REG_INDEX_ESP;
    cc._argumentsBaseOffset = (_isEspAdjusted)
      ? (_functionCallStackSize + _memStackSize16 + _peMovStackSize + _pePushPopStackSize + _peAdjustStackSize)
      : (_pePushPopStackSize);
  }
  else
  {
    cc._argumentsBaseReg = REG_INDEX_EBP;
    cc._argumentsBaseOffset = sizeof(sysint_t);
  }

  cc._variablesBaseReg = REG_INDEX_ESP;
  cc._variablesBaseOffset = _functionCallStackSize;
  if (!_isEspAdjusted)
    cc._variablesBaseOffset = -_memStackSize16 - _peMovStackSize - _peAdjustStackSize;
}

void EFunction::_dumpFunction(CompilerContext& cc) ASMJIT_NOTHROW
{
  Logger* logger = _compiler->getLogger();
  ASMJIT_ASSERT(logger != NULL);

  uint32_t i;
  char _buf[1024];
  char* p;

  // Log function prototype.
  {
    uint32_t argumentsCount = _functionPrototype.getArgumentsCount();
    bool first = true;

    logger->logString("; Function Prototype:\n");
    logger->logString(";\n");

    for (i = 0; i < argumentsCount; i++)
    {
      const FunctionPrototype::Argument& a = _functionPrototype.getArguments()[i];
      VarData* vdata = _argumentVariables[i];

      if (first)
      {
        logger->logString("; IDX| Type     | Sz | Home           |\n");
        logger->logString("; ---+----------+----+----------------+\n");
      }

      char* memHome = memHome = _buf;

      if (a.registerIndex != INVALID_VALUE)
      {
        BaseReg regOp(a.registerIndex | REG_TYPE_GPN, 0);
        dumpOperand(memHome, &regOp, REG_TYPE_GPN)[0] = '\0';
      }
      else
      {
        Mem memOp;
        memOp._mem.base = REG_INDEX_ESP;
        memOp._mem.displacement = a.stackOffset;
        dumpOperand(memHome, &memOp, REG_TYPE_GPN)[0] = '\0';
      }

      logger->logFormat("; %-3u| %-9s| %-3u| %-15s|\n",
        // Argument index.
        i,
        // Argument type.
        vdata->type < _VARIABLE_TYPE_COUNT ? variableInfo[vdata->type].name : "invalid",
        // Argument size.
        vdata->size,
        // Argument memory home.
        memHome
      );

      first = false;
    }
    logger->logString(";\n");
  }

  // Log variables.
  {
    uint32_t variablesCount = (uint32_t)_compiler->_varData.getLength();
    bool first = true;

    logger->logString("; Variables:\n");
    logger->logString(";\n");

    for (i = 0; i < variablesCount; i++)
    {
      VarData* vdata = _compiler->_varData[i];

      // If this variable is not related to this function then skip it.
      if (vdata->scope != this) continue;

      // Get some information about variable type.
      const VariableInfo& vinfo = variableInfo[vdata->type];

      if (first)
      {
        logger->logString("; ID | Type     | Sz | Home           | Register Access   | Memory Access     |\n");
        logger->logString("; ---+----------+----+----------------+-------------------+-------------------+\n");
      }

      char* memHome = (char*)"[None]";
      if (vdata->homeMemoryData != NULL)
      {
        VarMemBlock* memBlock = reinterpret_cast<VarMemBlock*>(vdata->homeMemoryData);
        memHome = _buf;

        Mem memOp;
        if (vdata->isMemArgument)
        {
          const FunctionPrototype::Argument& a = _functionPrototype.getArguments()[i];

          memOp._mem.base = cc._argumentsBaseReg;
          memOp._mem.displacement += cc._argumentsBaseOffset;
          memOp._mem.displacement += a.stackOffset;
        }
        else
        {
          memOp._mem.base = cc._variablesBaseReg;
          memOp._mem.displacement += cc._variablesBaseOffset;
          memOp._mem.displacement += memBlock->offset;
        }
        dumpOperand(memHome, &memOp, REG_TYPE_GPN)[0] = '\0';
      }

      logger->logFormat("; %-3u| %-9s| %-3u| %-15s| r=%-4uw=%-4ux=%-4u| r=%-4uw=%-4ux=%-4u|\n",
        // Variable id.
        (uint)(i & OPERAND_ID_VALUE_MASK),
        // Variable type.
        vdata->type < _VARIABLE_TYPE_COUNT ? vinfo.name : "invalid",
        // Variable size.
        vdata->size,
        // Variable memory home.
        memHome,
        // Register access count.
        (unsigned int)vdata->registerReadCount,
        (unsigned int)vdata->registerWriteCount,
        (unsigned int)vdata->registerRWCount,
        // Memory access count.
        (unsigned int)vdata->memoryReadCount,
        (unsigned int)vdata->memoryWriteCount,
        (unsigned int)vdata->memoryRWCount
      );
      first = false;
    }
    logger->logString(";\n");
  }

  // Log modified registers.
  {
    p = _buf;

    uint32_t r;
    uint32_t modifiedRegisters = 0;

    for (r = 0; r < 3; r++)
    {
      bool first = true;
      uint32_t regs;
      uint32_t type;

      switch (r)
      {
        case 0:
          regs = cc._modifiedGPRegisters;
          type = REG_TYPE_GPN;
          p = Util::mycpy(p, "; GP : ");
          break;
        case 1:
          regs = cc._modifiedMMRegisters;
          type = REG_TYPE_MM;
          p = Util::mycpy(p, "; MM : ");
          break;
        case 2:
          regs = cc._modifiedXMMRegisters;
          type = REG_TYPE_XMM;
          p = Util::mycpy(p, "; XMM: ");
          break;
        default:
          ASMJIT_ASSERT(0);
      }

      for (i = 0; i < REG_NUM_BASE; i++)
      {
        if ((regs & Util::maskFromIndex(i)) != 0)
        {
          if (!first) { *p++ = ','; *p++ = ' '; }
          p = dumpRegister(p, type, i);
          first = false;
          modifiedRegisters++;
        }
      }
      *p++ = '\n';
    }
    *p = '\0';

    logger->logFormat("; Modified registers (%u):\n", (unsigned int)modifiedRegisters);
    logger->logString(_buf);
  }

  logger->logString("\n");
}

void EFunction::_emitProlog(CompilerContext& cc) ASMJIT_NOTHROW
{
  uint32_t i, mask;
  uint32_t preservedGP  = _modifiedAndPreservedGP;
  uint32_t preservedMM  = _modifiedAndPreservedMM;
  uint32_t preservedXMM = _modifiedAndPreservedXMM;

  int32_t stackSubtract =
    _functionCallStackSize +
    _memStackSize16 + 
    _peMovStackSize + 
    _peAdjustStackSize;
  int32_t nspPos;

  if (_compiler->getLogger())
  {
    // Here function prolog starts.
    _compiler->comment("Prolog");
  }

  // Emit standard prolog entry code (but don't do it if function is set to be
  // naked).
  //
  // Also see the _prologEpilogStackAdjust variable. If function is naked (so
  // prolog and epilog will not contain "push ebp" and "mov ebp, esp", we need
  // to adjust stack by 8 bytes in 64-bit mode (this will give us that stack
  // will remain aligned to 16 bytes).
  if (!_isNaked)
  {
    _compiler->emit(INST_PUSH, nbp);
    _compiler->emit(INST_MOV, nbp, nsp);
  }

  // Align manually stack-pointer to 16-bytes.
  if (_isStackAlignedByFnTo16Bytes)
  {
    ASMJIT_ASSERT(!_isNaked);
    _compiler->emit(INST_AND, nsp, imm(-16));
  }

  // Save GP registers using PUSH/POP.
  if (preservedGP && _pePushPop)
  {
    for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
    {
      if (preservedGP & mask) _compiler->emit(INST_PUSH, gpn(i));
    }
  }

  if (_isEspAdjusted)
  {
    nspPos = _memStackSize16;
    if (stackSubtract) _compiler->emit(INST_SUB, nsp, imm(stackSubtract));
  }
  else
  {
    nspPos = -(_peMovStackSize + _peAdjustStackSize);
    //if (_pePushPop) nspPos += Util::bitCount(preservedGP) * sizeof(sysint_t);
  }

  // Save XMM registers using MOVDQA/MOVDQU.
  if (preservedXMM)
  {
    for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
    {
      if (preservedXMM & mask)
      {
        _compiler->emit(_movDqaInstruction, dqword_ptr(nsp, nspPos), xmm(i));
        nspPos += 16;
      }
    }
  }

  // Save MM registers using MOVQ.
  if (preservedMM)
  {
    for (i = 0, mask = 1; i < 8; i++, mask <<= 1)
    {
      if (preservedMM & mask)
      {
        _compiler->emit(INST_MOVQ, qword_ptr(nsp, nspPos), mm(i));
        nspPos += 8;
      }
    }
  }

  // Save GP registers using MOV.
  if (preservedGP && !_pePushPop)
  {
    for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
    {
      if (preservedGP & mask)
      {
        _compiler->emit(INST_MOV, sysint_ptr(nsp, nspPos), gpn(i));
        nspPos += sizeof(sysint_t);
      }
    }
  }

  if (_compiler->getLogger())
  {
    _compiler->comment("Body");
  }
}

void EFunction::_emitEpilog(CompilerContext& cc) ASMJIT_NOTHROW
{
  const CpuInfo* cpuInfo = getCpuInfo();

  uint32_t i, mask;
  uint32_t preservedGP  = _modifiedAndPreservedGP;
  uint32_t preservedMM  = _modifiedAndPreservedMM;
  uint32_t preservedXMM = _modifiedAndPreservedXMM;

  int32_t stackAdd =
    _functionCallStackSize +
    _memStackSize16 +
    _peMovStackSize +
    _peAdjustStackSize;
  int32_t nspPos;

  nspPos = (_isEspAdjusted)
    ? (_memStackSize16)
    : -(_peMovStackSize + _peAdjustStackSize);

  if (_compiler->getLogger())
  {
    _compiler->comment("Epilog");
  }

  // Restore XMM registers using MOVDQA/MOVDQU.
  if (preservedXMM)
  {
    for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
    {
      if (preservedXMM & mask)
      {
        _compiler->emit(_movDqaInstruction, xmm(i), dqword_ptr(nsp, nspPos));
        nspPos += 16;
      }
    }
  }

  // Restore MM registers using MOVQ.
  if (preservedMM)
  {
    for (i = 0, mask = 1; i < 8; i++, mask <<= 1)
    {
      if (preservedMM & mask)
      {
        _compiler->emit(INST_MOVQ, mm(i), qword_ptr(nsp, nspPos));
        nspPos += 8;
      }
    }
  }

  // Restore GP registers using MOV.
  if (preservedGP && !_pePushPop)
  {
    for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
    {
      if (preservedGP & mask)
      {
        _compiler->emit(INST_MOV, gpn(i), sysint_ptr(nsp, nspPos));
        nspPos += sizeof(sysint_t);
      }
    }
  }

  if (_isEspAdjusted && stackAdd != 0)
    _compiler->emit(INST_ADD, nsp, imm(stackAdd));

  // Restore GP registers using POP.
  if (preservedGP && _pePushPop)
  {
    for (i = REG_NUM_GP - 1, mask = 1 << i; (int32_t)i >= 0; i--, mask >>= 1)
    {
      if (preservedGP & mask)
      {
        _compiler->emit(INST_POP, gpn(i));
      }
    }
  }

  // Emit Emms.
  if (_emitEMMS) _compiler->emit(INST_EMMS);

  // Emit SFence / LFence / MFence.
  if ( _emitSFence &&  _emitLFence) _compiler->emit(INST_MFENCE); // MFence == SFence & LFence.
  if ( _emitSFence && !_emitLFence) _compiler->emit(INST_SFENCE); // Only SFence.
  if (!_emitSFence &&  _emitLFence) _compiler->emit(INST_LFENCE); // Only LFence.

  // Emit standard epilog leave code (if needed).
  if (!_isNaked)
  {
    if (cpuInfo->vendorId == CPU_VENDOR_AMD)
    {
      // AMD seems to prefer LEAVE instead of MOV/POP sequence.
      _compiler->emit(INST_LEAVE);
    }
    else
    {
      _compiler->emit(INST_MOV, nsp, nbp);
      _compiler->emit(INST_POP, nbp);
    }
  }

  // Emit return using correct instruction.
  if (_functionPrototype.getCalleePopsStack())
    _compiler->emit(INST_RET, imm((int16_t)_functionPrototype.getArgumentsStackSize()));
  else
    _compiler->emit(INST_RET);
}

void EFunction::reserveStackForFunctionCall(int32_t size)
{
  size = Util::alignTo16(size);

  if (size > _functionCallStackSize) _functionCallStackSize = size;
  _isCaller = true;
}

// ============================================================================
// [AsmJit::EProlog]
// ============================================================================

EProlog::EProlog(Compiler* c, EFunction* f) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_PROLOG),
  _function(f)
{
}

EProlog::~EProlog() ASMJIT_NOTHROW
{
}

void EProlog::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset++;
  _function->_prepareVariables(this);
}

Emittable* EProlog::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  _function->_allocVariables(cc);
  return translated();
}

// ============================================================================
// [AsmJit::EEpilog]
// ============================================================================

EEpilog::EEpilog(Compiler* c, EFunction* f) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_EPILOG),
  _function(f)
{
}

EEpilog::~EEpilog() ASMJIT_NOTHROW
{
}

void EEpilog::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset++;
}

Emittable* EEpilog::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  return translated();
}

// ============================================================================
// [AsmJit::ECall]
// ============================================================================

ECall::ECall(Compiler* c, EFunction* caller, const Operand* target) ASMJIT_NOTHROW : 
  Emittable(c, EMITTABLE_CALL),
  _caller(caller),
  _target(*target),
  _args(NULL),
  _gpParams(0),
  _mmParams(0),
  _xmmParams(0),
  _variablesCount(0),
  _variables(NULL)
{
}

ECall::~ECall() ASMJIT_NOTHROW
{
  memset(_argumentToVarRecord, 0, sizeof(VarCallRecord*) * FUNC_MAX_ARGS);
}

void ECall::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  // Prepare is similar to EInstruction::prepare(). We collect unique variables
  // and update statistics, but we don't use standard alloc/free register calls.
  //
  // The calling function is also unique in variable allocator point of view,
  // because we need to alloc some variables that may be destroyed be the 
  // callee (okay, may not, but this is not guaranteed).
  _offset = cc._currentOffset;

  // Tell EFunction that another function will be called inside. It needs this
  // information to reserve stack for the call and to mark esp adjustable.
  getCaller()->reserveStackForFunctionCall(
    (int32_t)getPrototype().getArgumentsStackSize());

  uint32_t i;
  uint32_t argumentsCount = getPrototype().getArgumentsCount();
  uint32_t operandsCount = argumentsCount;
  uint32_t variablesCount = 0;

  // Create registers used as arguments mask.
  for (i = 0; i < argumentsCount; i++)
  {
    const FunctionPrototype::Argument& fArg = getPrototype().getArguments()[i];

    if (fArg.registerIndex != INVALID_VALUE)
    {
      switch (fArg.variableType)
      {
        case VARIABLE_TYPE_GPD:
        case VARIABLE_TYPE_GPQ:
          _gpParams |= Util::maskFromIndex(fArg.registerIndex);
          break;
        case VARIABLE_TYPE_MM:
          _mmParams |= Util::maskFromIndex(fArg.registerIndex);
          break;
        case VARIABLE_TYPE_XMM:
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          _xmmParams |= Util::maskFromIndex(fArg.registerIndex);
          break;
        default:
          ASMJIT_ASSERT(0);
      }
    }
    else
    {
      cc.getFunction()->mustAdjustEsp();
    }
  }

  // Call address.
  operandsCount++;

  // The first and the second return value.
  if (!_ret[0].isNone()) operandsCount++;
  if (!_ret[1].isNone()) operandsCount++;

#define __GET_VARIABLE(__vardata__) \
  { \
    VarData* _candidate = __vardata__; \
    \
    for (var = cur; ;) \
    { \
      if (var == _variables) \
      { \
        var = cur++; \
        var->vdata = _candidate; \
        break; \
      } \
      \
      var--; \
      \
      if (var->vdata == _candidate) \
      { \
        break; \
      } \
    } \
    \
    ASMJIT_ASSERT(var != NULL); \
  }

  for (i = 0; i < operandsCount; i++)
  {
    Operand& o = (i < argumentsCount) 
      ? (_args[i])
      : (i == argumentsCount ? _target : _ret[i - argumentsCount - 1]);

    if (o.isVar())
    {
      ASMJIT_ASSERT(o.getId() != INVALID_VALUE);
      VarData* vdata = _compiler->_getVarData(o.getId());
      ASMJIT_ASSERT(vdata != NULL);

      if (vdata->workOffset == _offset) continue;
      if (!cc._isActive(vdata)) cc._addActive(vdata);

      vdata->workOffset = _offset;
      variablesCount++;
    }
    else if (o.isMem())
    {
      if ((o.getId() & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o.getId());
        ASMJIT_ASSERT(vdata != NULL);

        cc._markMemoryUsed(vdata);
        if (!cc._isActive(vdata)) cc._addActive(vdata);

        continue;
      }
      else if ((o._mem.base & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o._mem.base);
        ASMJIT_ASSERT(vdata != NULL);

        if (vdata->workOffset == _offset) continue;
        if (!cc._isActive(vdata)) cc._addActive(vdata);

        vdata->workOffset = _offset;
        variablesCount++;
      }

      if ((o._mem.index & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o._mem.index);
        ASMJIT_ASSERT(vdata != NULL);

        if (vdata->workOffset == _offset) continue;
        if (!cc._isActive(vdata)) cc._addActive(vdata);

        vdata->workOffset = _offset;
        variablesCount++;
      }
    }
  }

  // Traverse all active variables and set their firstCallable pointer to this
  // call. This information can be used to choose between the preserved-first
  // and preserved-last register allocation.
  if (cc._active)
  {
    VarData* first = cc._active;
    VarData* active = first;
    do {
      if (active->firstCallable == NULL) active->firstCallable = this;
      active = active->nextActive;
    } while (active != first);
  }

  if (!variablesCount)
  {
    cc._currentOffset++;
    return;
  }

  _variables = reinterpret_cast<VarCallRecord*>(_compiler->getZone().zalloc(sizeof(VarCallRecord) * variablesCount));
  if (!_variables)
  {
    _compiler->setError(ERROR_NO_HEAP_MEMORY);
    cc._currentOffset++;
    return;
  }

  _variablesCount = variablesCount;
  memset(_variables, 0, sizeof(VarCallRecord) * variablesCount);

  VarCallRecord* cur = _variables;
  VarCallRecord* var = NULL;

  for (i = 0; i < operandsCount; i++)
  {
    Operand& o = (i < argumentsCount) 
      ? (_args[i])
      : (i == argumentsCount ? _target : _ret[i - argumentsCount - 1]);

    if (o.isVar())
    {
      VarData* vdata = _compiler->_getVarData(o.getId());
      ASMJIT_ASSERT(vdata != NULL);

      __GET_VARIABLE(vdata)
      _argumentToVarRecord[i] = var;

      if (i < argumentsCount)
      {
        const FunctionPrototype::Argument& fArg = getPrototype().getArguments()[i];

        if (fArg.registerIndex != INVALID_VALUE)
        {
          cc._newRegisterHomeIndex(vdata, fArg.registerIndex);

          switch (fArg.variableType)
          {
            case VARIABLE_TYPE_GPD:
            case VARIABLE_TYPE_GPQ:
              var->flags |= VarCallRecord::FLAG_IN_GP;
              var->inCount++;
              break;

            case VARIABLE_TYPE_MM:
              var->flags |= VarCallRecord::FLAG_IN_MM;
              var->inCount++;
              break;

            case VARIABLE_TYPE_XMM:
            case VARIABLE_TYPE_XMM_1F:
            case VARIABLE_TYPE_XMM_4F:
            case VARIABLE_TYPE_XMM_1D:
            case VARIABLE_TYPE_XMM_2D:
              var->flags |= VarCallRecord::FLAG_IN_XMM;
              var->inCount++;
              break;

            default:
              ASMJIT_ASSERT(0);
          }
        }
        else
        {
          var->inCount++;
        }

        vdata->registerReadCount++;
      }
      else if (i == argumentsCount)
      {
        uint32_t mask = ~getPrototype().getPreservedGP() &
                        ~getPrototype().getPassedGP()    & 
                        Util::maskUpToIndex(REG_NUM_GP);

        cc._newRegisterHomeIndex(vdata, Util::findFirstBit(mask));
        cc._newRegisterHomeMask(vdata, mask);

        var->flags |= VarCallRecord::FLAG_CALL_OPERAND_REG;
        vdata->registerReadCount++;
      }
      else
      {
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
          case VARIABLE_TYPE_GPQ:
            if (i == argumentsCount+1)
              var->flags |= VarCallRecord::FLAG_OUT_EAX;
            else
              var->flags |= VarCallRecord::FLAG_OUT_EDX;
            break;

          case VARIABLE_TYPE_X87:
          case VARIABLE_TYPE_X87_1F:
          case VARIABLE_TYPE_X87_1D:
#if defined(ASMJIT_X86)
            if (i == argumentsCount+1)
              var->flags |= VarCallRecord::FLAG_OUT_ST0;
            else
              var->flags |= VarCallRecord::FLAG_OUT_ST1;
#else
            if (i == argumentsCount+1)
              var->flags |= VarCallRecord::FLAG_OUT_XMM0;
            else
              var->flags |= VarCallRecord::FLAG_OUT_XMM1;
#endif
            break;

          case VARIABLE_TYPE_MM:
            var->flags |= VarCallRecord::FLAG_OUT_MM0;
            break;

          case VARIABLE_TYPE_XMM:
          case VARIABLE_TYPE_XMM_4F:
          case VARIABLE_TYPE_XMM_2D:
            if (i == argumentsCount+1)
              var->flags |= VarCallRecord::FLAG_OUT_XMM0;
            else
              var->flags |= VarCallRecord::FLAG_OUT_XMM1;
            break;

          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_1D:
#if defined(ASMJIT_X86)
            if (i == argumentsCount+1)
              var->flags |= VarCallRecord::FLAG_OUT_ST0;
            else
              var->flags |= VarCallRecord::FLAG_OUT_ST1;
#else
            if (i == argumentsCount+1)
              var->flags |= VarCallRecord::FLAG_OUT_XMM0;
            else
              var->flags |= VarCallRecord::FLAG_OUT_XMM1;
#endif
            break;

          default:
            ASMJIT_ASSERT(0);
        }

        vdata->registerWriteCount++;
      }
    }
    else if (o.isMem())
    {
      ASMJIT_ASSERT(i == argumentsCount);

      if ((o.getId() & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o.getId());
        ASMJIT_ASSERT(vdata != NULL);

        vdata->memoryReadCount++;
      }
      else if ((o._mem.base & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(reinterpret_cast<Mem&>(o).getBase());
        ASMJIT_ASSERT(vdata != NULL);

        vdata->registerReadCount++;

        __GET_VARIABLE(vdata)
        var->flags |= VarCallRecord::FLAG_CALL_OPERAND_REG | VarCallRecord::FLAG_CALL_OPERAND_MEM;
      }

      if ((o._mem.index & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(reinterpret_cast<Mem&>(o).getIndex());
        ASMJIT_ASSERT(vdata != NULL);

        vdata->registerReadCount++;

        __GET_VARIABLE(vdata)
        var->flags |= VarCallRecord::FLAG_CALL_OPERAND_REG | VarCallRecord::FLAG_CALL_OPERAND_MEM;
      }
    }
  }

  // Traverse all variables and update firstEmittable / lastEmittable. This
  // function is called from iterator that scans emittables using forward
  // direction so we can use this knowledge to optimize the process.
  //
  // Same code is in EInstruction::prepare().
  for (i = 0; i < _variablesCount; i++)
  {
    VarData* v = _variables[i].vdata;

    // First emittable (begin of variable scope).
    if (v->firstEmittable == NULL) v->firstEmittable = this;

    // Last emittable (end of variable scope).
    v->lastEmittable = this;
  }

  cc._currentOffset++;

#undef __GET_VARIABLE
}

Emittable* ECall::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  uint32_t i;
  uint32_t preserved, mask;

  uint32_t temporaryGpReg;
  uint32_t temporaryXmmReg;

  uint32_t offset = cc._currentOffset;
  Compiler* compiler = cc.getCompiler();

  // Constants.
  const FunctionPrototype::Argument* targs = getPrototype().getArguments();

  uint32_t argumentsCount = getPrototype().getArgumentsCount();
  uint32_t variablesCount = _variablesCount;

  // Processed arguments.
  uint8_t processed[FUNC_MAX_ARGS] = { 0 };

  compiler->comment("Function Call");

  // These variables are used by the instruction and we set current offset
  // to their work offsets -> The getSpillCandidate() method never returns 
  // the variable used by this instruction.
  for (i = 0; i < variablesCount; i++)
  {
    _variables[i].vdata->workOffset = offset;

    // Init back-reference to VarCallRecord.
    _variables[i].vdata->tempPtr = &_variables[i];
  }

  // --------------------------------------------------------------------------
  // STEP 1:
  //
  // Spill variables which are not used by the function call and have to
  // be destroyed. These registers may be used by callee.
  // --------------------------------------------------------------------------

  preserved = getPrototype().getPreservedGP();
  for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
  {
    VarData* vdata = cc._state.gp[i];
    if (vdata && vdata->workOffset != offset && (preserved & mask) == 0)
    {
      cc.spillGPVar(vdata);
    }
  }

  preserved = getPrototype().getPreservedMM();
  for (i = 0, mask = 1; i < REG_NUM_MM; i++, mask <<= 1)
  {
    VarData* vdata = cc._state.mm[i];
    if (vdata && vdata->workOffset != offset && (preserved & mask) == 0)
    {
      cc.spillMMVar(vdata);
    }
  }

  preserved = getPrototype().getPreservedXMM();
  for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
  {
    VarData* vdata = cc._state.xmm[i];
    if (vdata && vdata->workOffset != offset && (preserved & mask) == 0)
    {
      cc.spillXMMVar(vdata);
    }
  }

  // --------------------------------------------------------------------------
  // STEP 2:
  //
  // Move all arguments to the stack which all already in registers.
  // --------------------------------------------------------------------------

  for (i = 0; i < argumentsCount; i++)
  {
    if (processed[i]) continue;

    const FunctionPrototype::Argument& argType = targs[i];
    if (argType.registerIndex != INVALID_VALUE) continue;

    Operand& operand = _args[i];

    if (operand.isVar())
    {
      VarCallRecord* rec = _argumentToVarRecord[i];
      VarData* vdata = compiler->_getVarData(operand.getId());

      if (vdata->registerIndex != INVALID_VALUE)
      {
        _moveAllocatedVariableToStack(cc,
          vdata, argType);

        rec->inDone++;
        processed[i] = true;
      }
    }
  }

  // --------------------------------------------------------------------------
  // STEP 3:
  //
  // Spill all non-preserved variables we moved to stack in STEP #2.
  // --------------------------------------------------------------------------

  for (i = 0; i < argumentsCount; i++)
  {
    VarCallRecord* rec = _argumentToVarRecord[i];
    if (!rec || processed[i]) continue;

    if (rec->inDone >= rec->inCount)
    {
      VarData* vdata = rec->vdata;
      if (vdata->registerIndex == INVALID_VALUE) continue;

      if (rec->outCount)
      {
        // Variable will be rewritten by function return value, it's not needed
        // to spill it. It will be allocated again by ECall.
        cc.unuseVar(rec->vdata, VARIABLE_STATE_UNUSED);
      }
      else
      {
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
          case VARIABLE_TYPE_GPQ:
            if ((getPrototype().getPreservedGP() & Util::maskFromIndex(vdata->registerIndex)) == 0)
              cc.spillGPVar(vdata);
            break;
          case VARIABLE_TYPE_MM:
            if ((getPrototype().getPreservedMM() & Util::maskFromIndex(vdata->registerIndex)) == 0)
              cc.spillMMVar(vdata);
            break;
          case VARIABLE_TYPE_XMM:
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_4F:
          case VARIABLE_TYPE_XMM_2D:
            if ((getPrototype().getPreservedXMM() & Util::maskFromIndex(vdata->registerIndex)) == 0)
              cc.spillXMMVar(vdata);
            break;
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // STEP 4:
  //
  // Get temporary register that we can use to pass input function arguments.
  // Now it's safe to do, because the non-needed variables should be spilled.
  // --------------------------------------------------------------------------

  temporaryGpReg = _findTemporaryGpRegister(cc);
  temporaryXmmReg = _findTemporaryXmmRegister(cc);

  // If failed to get temporary register then we need just to pick one.
  if (temporaryGpReg == INVALID_VALUE)
  {
    // TODO.
  }
  if (temporaryXmmReg == INVALID_VALUE)
  {
    // TODO.
  }

  // --------------------------------------------------------------------------
  // STEP 5:
  //
  // Move all remaining arguments to the stack (we can use temporary register).
  // or allocate it to the primary register. Also move immediates.
  // --------------------------------------------------------------------------

  for (i = 0; i < argumentsCount; i++)
  {
    if (processed[i]) continue;

    const FunctionPrototype::Argument& argType = targs[i];
    if (argType.registerIndex != INVALID_VALUE) continue;

    Operand& operand = _args[i];

    if (operand.isVar())
    {
      VarCallRecord* rec = _argumentToVarRecord[i];
      VarData* vdata = compiler->_getVarData(operand.getId());

      _moveSpilledVariableToStack(cc,
        vdata, argType,
        temporaryGpReg, temporaryXmmReg);

      rec->inDone++;
      processed[i] = true;
    }
    else if (operand.isImm())
    {
      // TODO.
    }
  }

  // --------------------------------------------------------------------------
  // STEP 6:
  //
  // Allocate arguments to registers.
  // --------------------------------------------------------------------------

  bool didWork;

  do {
    didWork = false;

    for (i = 0; i < argumentsCount; i++)
    {
      if (processed[i]) continue;

      VarCallRecord* rsrc = _argumentToVarRecord[i];

      Operand& osrc = _args[i];
      ASMJIT_ASSERT(osrc.isVar());
      VarData* vsrc = compiler->_getVarData(osrc.getId());

      const FunctionPrototype::Argument& srcArgType = targs[i];
      VarData* vdst = _getOverlappingVariable(cc, srcArgType);

      if (vsrc == vdst)
      {
        rsrc->inDone++;
        processed[i] = true;

        didWork = true;
        continue;
      }
      else if (vdst != NULL)
      {
        VarCallRecord* rdst = reinterpret_cast<VarCallRecord*>(vdst->tempPtr);

        if (rdst->inDone >= rdst->inCount && (rdst->flags & VarCallRecord::FLAG_CALL_OPERAND_REG) == 0)
        {
          // Safe to spill.
          if (rdst->outCount || vdst->lastEmittable == this)
            cc.unuseVar(vdst, VARIABLE_STATE_UNUSED);
          else
            cc.spillVar(vdst);
          vdst = NULL;
        }
        else
        {
          uint32_t x = getPrototype().findArgumentByRegisterCode(
            getVariableRegisterCode(vsrc->type, vsrc->registerIndex));
          bool doSpill = true;

          if ((getVariableClass(vdst->type) & VariableInfo::CLASS_GP) != 0)
          {
            // Try to emit mov to register which is possible for call() operand.
            if (x == INVALID_VALUE && (rdst->flags & VarCallRecord::FLAG_CALL_OPERAND_REG) != 0)
            {
              uint32_t rIndex;
              uint32_t rBit;

              // The mask which contains registers which are not-preserved
              // (these that might be clobbered by the callee) and which are
              // not used to pass function arguments. Each register contained
              // in this mask is ideal to be used by call() instruction.
              uint32_t possibleMask = ~getPrototype().getPreservedGP() &
                                      ~getPrototype().getPassedGP()    & 
                                      Util::maskUpToIndex(REG_NUM_GP);

              if (possibleMask != 0)
              {
                for (rIndex = 0, rBit = 1; rIndex < REG_NUM_GP; rIndex++, rBit <<= 1)
                {
                  if ((possibleMask & rBit) != 0)
                  {
                    if (cc._state.gp[rIndex] == NULL) 
                    {
                      // This is the best possible solution, the register is
                      // free. We do not need to continue with this loop, the
                      // rIndex will be used by the call().
                      break;
                    }
                    else
                    {
                      // Wait until the register is freed or try to find another.
                      doSpill = false;
                      didWork = true;
                    }
                  }
                }
              }
              else
              {
                // Try to find a register which is free and which is not used
                // to pass a function argument.
                possibleMask = getPrototype().getPreservedGP();

                for (rIndex = 0, rBit = 1; rIndex < REG_NUM_GP; rIndex++, rBit <<= 1)
                {
                  if ((possibleMask & rBit) != 0)
                  {
                    // Found one.
                    if (cc._state.gp[rIndex] == NULL) break;
                  }
                }
              }

              if (rIndex < REG_NUM_GP)
              {
                if (temporaryGpReg == vsrc->registerIndex) temporaryGpReg = rIndex;
                compiler->emit(INST_MOV, gpn(rIndex), gpn(vsrc->registerIndex));

                cc._state.gp[vsrc->registerIndex] = NULL;
                cc._state.gp[rIndex] = vsrc;

                vsrc->registerIndex = rIndex;
                cc._allocatedGPRegister(rIndex);

                doSpill = false;
                didWork = true;
              }
            }
            // Emit xchg instead of spill/alloc if possible.
            else if (x != INVALID_VALUE)
            {
              const FunctionPrototype::Argument& dstArgType = targs[x];
              if (getVariableClass(dstArgType.variableType) == getVariableClass(srcArgType.variableType))
              {
                uint32_t dstIndex = vdst->registerIndex;
                uint32_t srcIndex = vsrc->registerIndex;

                if (srcIndex == dstArgType.registerIndex)
                {
#if defined(ASMJIT_X64)
                  if (vdst->type != VARIABLE_TYPE_GPD || vsrc->type != VARIABLE_TYPE_GPD)
                    compiler->emit(INST_XCHG, gpq(dstIndex), gpq(srcIndex));
                  else
#endif
                    compiler->emit(INST_XCHG, gpd(dstIndex), gpd(srcIndex));

                  cc._state.gp[srcIndex] = vdst;
                  cc._state.gp[dstIndex] = vsrc;

                  vdst->registerIndex = srcIndex;
                  vsrc->registerIndex = dstIndex;

                  rdst->inDone++;
                  rsrc->inDone++;

                  processed[i] = true;
                  processed[x] = true;

                  doSpill = false;
                }
              }
            }
          }

          if (doSpill)
          {
            cc.spillVar(vdst);
            vdst = NULL;
          }
        }
      }

      if (vdst == NULL)
      {
        VarCallRecord* rec = reinterpret_cast<VarCallRecord*>(vsrc->tempPtr);

        _moveSrcVariableToRegister(cc, vsrc, srcArgType);

        switch (srcArgType.variableType)
        {
          case VARIABLE_TYPE_GPD:
          case VARIABLE_TYPE_GPQ:
            cc._markGPRegisterModified(srcArgType.registerIndex);
            break;
          case VARIABLE_TYPE_MM:
            cc._markMMRegisterModified(srcArgType.registerIndex);
            break;
          case VARIABLE_TYPE_XMM:
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_4F:
          case VARIABLE_TYPE_XMM_2D:
            cc._markMMRegisterModified(srcArgType.registerIndex);
            break;
        }

        rec->inDone++;
        processed[i] = true;
      }
    }
  } while (didWork);

  // --------------------------------------------------------------------------
  // STEP 7:
  //
  // Allocate operand used by CALL instruction.
  // --------------------------------------------------------------------------

  for (i = 0; i < variablesCount; i++)
  {
    VarCallRecord& r = _variables[i];
    if ((r.flags & VarCallRecord::FLAG_CALL_OPERAND_REG) &&
        (r.vdata->registerIndex == INVALID_VALUE))
    {
      // If the register is not allocated and the call form is 'call reg' then
      // it's possible to keep it in memory.
      if ((r.flags & VarCallRecord::FLAG_CALL_OPERAND_MEM) == 0)
      {
        _target = GPVarFromData(r.vdata).m();
        break;
      }

      if (temporaryGpReg == INVALID_VALUE)
        temporaryGpReg = _findTemporaryGpRegister(cc);

      cc.allocGPVar(r.vdata, 
        Util::maskFromIndex(temporaryGpReg),
        VARIABLE_ALLOC_REGISTER | VARIABLE_ALLOC_READ);
    }
  }

  cc.translateOperands(&_target, 1);

  // --------------------------------------------------------------------------
  // STEP 8:
  //
  // Spill all preserved variables.
  // --------------------------------------------------------------------------

  preserved = getPrototype().getPreservedGP();
  for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
  {
    VarData* vdata = cc._state.gp[i];
    if (vdata && (preserved & mask) == 0)
    {
      VarCallRecord* rec = reinterpret_cast<VarCallRecord*>(vdata->tempPtr);
      if (rec && (rec->outCount || rec->flags & VarCallRecord::FLAG_UNUSE_AFTER_USE || vdata->lastEmittable == this))
        cc.unuseVar(vdata, VARIABLE_STATE_UNUSED);
      else
        cc.spillGPVar(vdata);
    }
  }

  preserved = getPrototype().getPreservedMM();
  for (i = 0, mask = 1; i < REG_NUM_MM; i++, mask <<= 1)
  {
    VarData* vdata = cc._state.mm[i];
    if (vdata && (preserved & mask) == 0)
    {
      VarCallRecord* rec = reinterpret_cast<VarCallRecord*>(vdata->tempPtr);
      if (rec && (rec->outCount || vdata->lastEmittable == this))
        cc.unuseVar(vdata, VARIABLE_STATE_UNUSED);
      else
        cc.spillMMVar(vdata);
    }
  }

  preserved = getPrototype().getPreservedXMM();
  for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
  {
    VarData* vdata = cc._state.xmm[i];
    if (vdata && (preserved & mask) == 0)
    {
      VarCallRecord* rec = reinterpret_cast<VarCallRecord*>(vdata->tempPtr);
      if (rec && (rec->outCount || vdata->lastEmittable == this))
        cc.unuseVar(vdata, VARIABLE_STATE_UNUSED);
      else
        cc.spillXMMVar(vdata);
    }
  }

  // --------------------------------------------------------------------------
  // STEP 9:
  //
  // Emit CALL instruction.
  // --------------------------------------------------------------------------

  compiler->emit(INST_CALL, _target);

  // Restore the stack offset.
  if (getPrototype().getCalleePopsStack())
  {
    int32_t s = (int32_t)getPrototype().getArgumentsStackSize();
    if (s) compiler->emit(INST_SUB, nsp, imm(s));
  }

  // --------------------------------------------------------------------------
  // STEP 10:
  //
  // Prepare others for return value(s) and cleanup.
  // --------------------------------------------------------------------------

  // Clear temp data, see AsmJit::VarData::temp why it's needed.
  for (i = 0; i < variablesCount; i++)
  {
    VarCallRecord* rec = &_variables[i];
    VarData* vdata = rec->vdata;

    if (rec->flags & (VarCallRecord::FLAG_OUT_EAX | VarCallRecord::FLAG_OUT_EDX))
    {
      if (getVariableClass(vdata->type) & VariableInfo::CLASS_GP)
      {
        cc.allocGPVar(vdata, 
          Util::maskFromIndex((rec->flags & VarCallRecord::FLAG_OUT_EAX) != 0
            ? REG_INDEX_EAX
            : REG_INDEX_EDX),
          VARIABLE_ALLOC_REGISTER | VARIABLE_ALLOC_WRITE);
        vdata->changed = true;
      }
    }

    if (rec->flags & (VarCallRecord::FLAG_OUT_MM0))
    {
      if (getVariableClass(vdata->type) & VariableInfo::CLASS_MM)
      {
        cc.allocMMVar(vdata, Util::maskFromIndex(REG_INDEX_MM0),
          VARIABLE_ALLOC_REGISTER | VARIABLE_ALLOC_WRITE);
        vdata->changed = true;
      }
    }

    if (rec->flags & (VarCallRecord::FLAG_OUT_XMM0 | VarCallRecord::FLAG_OUT_XMM1))
    {
      if (getVariableClass(vdata->type) & VariableInfo::CLASS_XMM)
      {
        cc.allocXMMVar(vdata, 
          Util::maskFromIndex((rec->flags & VarCallRecord::FLAG_OUT_XMM0) != 0
            ? REG_INDEX_XMM0
            : REG_INDEX_XMM1),
          VARIABLE_ALLOC_REGISTER | VARIABLE_ALLOC_WRITE);
        vdata->changed = true;
      }
    }

    if (rec->flags & (VarCallRecord::FLAG_OUT_ST0 | VarCallRecord::FLAG_OUT_ST1))
    {
      if (getVariableClass(vdata->type) & VariableInfo::CLASS_XMM)
      {
        Mem mem(cc._getVarMem(vdata));
        cc.unuseVar(vdata, VARIABLE_STATE_MEMORY);

        switch (vdata->type)
        {
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
          {
            mem.setSize(4);
            compiler->emit(INST_FSTP, mem);
            break;
          }
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
          {
            mem.setSize(8);
            compiler->emit(INST_FSTP, mem);
            break;
          }
          default:
          {
            compiler->comment("*** WARNING: Can't convert float return value to untyped XMM\n");
            break;
          }
        }
      }
    }

    // Cleanup.
    vdata->tempPtr = NULL;
  }

  for (i = 0; i < variablesCount; i++)
  {
    cc._unuseVarOnEndOfScope(this, &_variables[i]);
  }

  return translated();
}

int ECall::getMaxSize() const ASMJIT_NOTHROW
{
  // TODO: Not optimal.
  return 15;
}

bool ECall::_tryUnuseVar(VarData* v) ASMJIT_NOTHROW
{
  for (uint32_t i = 0; i < _variablesCount; i++)
  {
    if (_variables[i].vdata == v)
    {
      _variables[i].flags |= VarCallRecord::FLAG_UNUSE_AFTER_USE;
      return true;
    }
  }

  return false;
}

uint32_t ECall::_findTemporaryGpRegister(CompilerContext& cc) ASMJIT_NOTHROW
{
  uint32_t i;
  uint32_t mask;

  uint32_t passedGP = getPrototype().getPassedGP();
  uint32_t candidate = INVALID_VALUE;

  // Find all registers used to pass function arguments. We shouldn't use these
  // if possible.
  for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
  {
    if (cc._state.gp[i] == NULL)
    {
      // If this register is used to pass arguments to function, we will mark
      // it and use it only if there is no other one.
      if ((passedGP & mask) != 0)
        candidate = i;
      else
        return i;
    }
  }

  return candidate;
}

uint32_t ECall::_findTemporaryXmmRegister(CompilerContext& cc) ASMJIT_NOTHROW
{
  uint32_t i;
  uint32_t mask;

  uint32_t passedXMM = getPrototype().getPassedXMM();
  uint32_t candidate = INVALID_VALUE;

  // Find all registers used to pass function arguments. We shouldn't use these
  // if possible.
  for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
  {
    if (cc._state.xmm[i] == NULL)
    {
      // If this register is used to pass arguments to function, we will mark
      // it and use it only if there is no other one.
      if ((passedXMM & mask) != 0)
        candidate = i;
      else
        return i;
    }
  }

  return candidate;
}

VarData* ECall::_getOverlappingVariable(CompilerContext& cc,
  const FunctionPrototype::Argument& argType) const ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(argType.variableType != INVALID_VALUE);

  switch (argType.variableType)
  {
    case VARIABLE_TYPE_GPD:
    case VARIABLE_TYPE_GPQ:
      return cc._state.gp[argType.registerIndex];
    case VARIABLE_TYPE_MM:
      return cc._state.mm[argType.registerIndex];
    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_1F:
    case VARIABLE_TYPE_XMM_1D:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_2D:
      return cc._state.xmm[argType.registerIndex];
  }

  return NULL;
}

void ECall::_moveAllocatedVariableToStack(CompilerContext& cc, VarData* vdata, const FunctionPrototype::Argument& argType) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(argType.registerIndex == INVALID_VALUE);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  Compiler* compiler = cc.getCompiler();

  uint32_t src = vdata->registerIndex;
  Mem dst = ptr(nsp, -(int)sizeof(sysint_t) + argType.stackOffset);

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_GPD:
          compiler->emit(INST_MOV, dst, gpd(src));
          return;
#if defined(ASMJIT_X64)
        case VARIABLE_TYPE_GPQ:
        case VARIABLE_TYPE_MM:
          compiler->emit(INST_MOV, dst, gpq(src));
          return;
#endif // ASMJIT_X64
      }
      break;

#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_GPD:
          compiler->emit(INST_MOV, dst, gpd(src));
          return;
        case VARIABLE_TYPE_GPQ:
          compiler->emit(INST_MOV, dst, gpq(src));
          return;
        case VARIABLE_TYPE_MM:
          compiler->emit(INST_MOVQ, dst, gpq(src));
          return;
      }
      break;
#endif // ASMJIT_X64

    case VARIABLE_TYPE_MM:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_GPD:
        case VARIABLE_TYPE_X87_1F:
        case VARIABLE_TYPE_XMM_1F:
          compiler->emit(INST_MOVD, dst, mm(src));
          return;
        case VARIABLE_TYPE_GPQ:
        case VARIABLE_TYPE_MM:
        case VARIABLE_TYPE_X87_1D:
        case VARIABLE_TYPE_XMM_1D:
          compiler->emit(INST_MOVQ, dst, mm(src));
          return;
      }
      break;

    // We allow incompatible types here, because the called can convert them
    // to correct format before function is called.

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_2D:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_XMM:
          compiler->emit(INST_MOVDQU, dst, xmm(src));
          return;
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
          compiler->emit(INST_MOVUPS, dst, xmm(src));
          return;
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          compiler->emit(INST_MOVUPD, dst, xmm(src));
          return;
      }
      break;

    case VARIABLE_TYPE_XMM_1F:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_X87_1F:
        case VARIABLE_TYPE_XMM:
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          compiler->emit(INST_MOVSS, dst, xmm(src));
          return;
      }
      break;

    case VARIABLE_TYPE_XMM_1D:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_X87_1D:
        case VARIABLE_TYPE_XMM:
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          compiler->emit(INST_MOVSD, dst, xmm(src));
          return;
      }
      break;
  }

  compiler->setError(ERROR_INCOMPATIBLE_ARGUMENT);
}

void ECall::_moveSpilledVariableToStack(CompilerContext& cc,
  VarData* vdata, const FunctionPrototype::Argument& argType,
  uint32_t temporaryGpReg,
  uint32_t temporaryXmmReg) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(argType.registerIndex == INVALID_VALUE);
  ASMJIT_ASSERT(vdata->registerIndex == INVALID_VALUE);

  Compiler* compiler = cc.getCompiler();

  Mem src = cc._getVarMem(vdata);
  Mem dst = ptr(nsp, -(int)sizeof(sysint_t) + argType.stackOffset);

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_GPD:
          compiler->emit(INST_MOV, gpd(temporaryGpReg), src);
          compiler->emit(INST_MOV, dst, gpd(temporaryGpReg));
          return;
#if defined(ASMJIT_X64)
        case VARIABLE_TYPE_GPQ:
        case VARIABLE_TYPE_MM:
          compiler->emit(INST_MOV, gpd(temporaryGpReg), src);
          compiler->emit(INST_MOV, dst, gpq(temporaryGpReg));
          return;
#endif // ASMJIT_X64
      }
      break;

#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_GPD:
          compiler->emit(INST_MOV, gpd(temporaryGpReg), src);
          compiler->emit(INST_MOV, dst, gpd(temporaryGpReg));
          return;
        case VARIABLE_TYPE_GPQ:
        case VARIABLE_TYPE_MM:
          compiler->emit(INST_MOV, gpq(temporaryGpReg), src);
          compiler->emit(INST_MOV, dst, gpq(temporaryGpReg));
          return;
      }
      break;
#endif // ASMJIT_X64

    case VARIABLE_TYPE_MM:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_GPD:
        case VARIABLE_TYPE_X87_1F:
        case VARIABLE_TYPE_XMM_1F:
          compiler->emit(INST_MOV, gpd(temporaryGpReg), src);
          compiler->emit(INST_MOV, dst, gpd(temporaryGpReg));
          return;
        case VARIABLE_TYPE_GPQ:
        case VARIABLE_TYPE_MM:
        case VARIABLE_TYPE_X87_1D:
        case VARIABLE_TYPE_XMM_1D:
          // TODO
          return;
      }
      break;

    // We allow incompatible types here, because the called can convert them
    // to correct format before function is called.

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_2D:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_XMM:
          compiler->emit(INST_MOVDQU, xmm(temporaryXmmReg), src);
          compiler->emit(INST_MOVDQU, dst, xmm(temporaryXmmReg));
          return;
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
          compiler->emit(INST_MOVUPS, xmm(temporaryXmmReg), src);
          compiler->emit(INST_MOVUPS, dst, xmm(temporaryXmmReg));
          return;
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          compiler->emit(INST_MOVUPD, xmm(temporaryXmmReg), src);
          compiler->emit(INST_MOVUPD, dst, xmm(temporaryXmmReg));
          return;
      }
      break;

    case VARIABLE_TYPE_XMM_1F:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_X87_1F:
        case VARIABLE_TYPE_XMM:
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          compiler->emit(INST_MOVSS, xmm(temporaryXmmReg), src);
          compiler->emit(INST_MOVSS, dst, xmm(temporaryXmmReg));
          return;
      }
      break;

    case VARIABLE_TYPE_XMM_1D:
      switch (argType.variableType)
      {
        case VARIABLE_TYPE_X87_1D:
        case VARIABLE_TYPE_XMM:
        case VARIABLE_TYPE_XMM_1F:
        case VARIABLE_TYPE_XMM_4F:
        case VARIABLE_TYPE_XMM_1D:
        case VARIABLE_TYPE_XMM_2D:
          compiler->emit(INST_MOVSD, xmm(temporaryXmmReg), src);
          compiler->emit(INST_MOVSD, dst, xmm(temporaryXmmReg));
          return;
      }
      break;
  }

  compiler->setError(ERROR_INCOMPATIBLE_ARGUMENT);
}

void ECall::_moveSrcVariableToRegister(CompilerContext& cc,
  VarData* vdata, const FunctionPrototype::Argument& argType) ASMJIT_NOTHROW
{
  uint32_t dst = argType.registerIndex;
  uint32_t src = vdata->registerIndex;

  Compiler* compiler = cc.getCompiler();

  if (src != INVALID_VALUE)
  {
    switch (argType.variableType)
    {
      case VARIABLE_TYPE_GPD:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
#if defined(ASMJIT_X64)
          case VARIABLE_TYPE_GPQ:
#endif // ASMJIT_X64
            compiler->emit(INST_MOV, gpd(dst), gpd(src));
            return;
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVD, gpd(dst), mm(src));
            return;
        }
        break;

#if defined(ASMJIT_X64)
      case VARIABLE_TYPE_GPQ:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
            compiler->emit(INST_MOV, gpd(dst), gpd(src));
            return;
          case VARIABLE_TYPE_GPQ:
            compiler->emit(INST_MOV, gpq(dst), gpq(src));
            return;
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, gpq(dst), mm(src));
            return;
        }
        break;
#endif // ASMJIT_X64

      case VARIABLE_TYPE_MM:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
            compiler->emit(INST_MOVD, gpd(dst), gpd(src));
            return;
#if defined(ASMJIT_X64)
          case VARIABLE_TYPE_GPQ:
            compiler->emit(INST_MOVQ, gpq(dst), gpq(src));
            return;
#endif // ASMJIT_X64
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, mm(dst), mm(src));
            return;
        }
        break;

      case VARIABLE_TYPE_XMM:
      case VARIABLE_TYPE_XMM_4F:
      case VARIABLE_TYPE_XMM_2D:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
            compiler->emit(INST_MOVD, xmm(dst), gpd(src));
            return;
#if defined(ASMJIT_X64)
          case VARIABLE_TYPE_GPQ:
            compiler->emit(INST_MOVQ, xmm(dst), gpq(src));
            return;
#endif // ASMJIT_X64
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, xmm(dst), mm(src));
            return;
          case VARIABLE_TYPE_XMM:
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
            compiler->emit(INST_MOVDQA, xmm(dst), xmm(src));
            return;
        }
        break;

      case VARIABLE_TYPE_XMM_1F:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, xmm(dst), mm(src));
            return;

          case VARIABLE_TYPE_XMM:
            compiler->emit(INST_MOVDQA, xmm(dst), xmm(src));
            return;
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
            compiler->emit(INST_MOVSS, xmm(dst), xmm(src));
            return;
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
            compiler->emit(INST_CVTSD2SS, xmm(dst), xmm(src));
            return;
        }
        break;

      case VARIABLE_TYPE_XMM_1D:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, xmm(dst), mm(src));
            return;

          case VARIABLE_TYPE_XMM:
            compiler->emit(INST_MOVDQA, xmm(dst), xmm(src));
            return;
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
            compiler->emit(INST_CVTSS2SD, xmm(dst), xmm(src));
            return;
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
            compiler->emit(INST_MOVSD, xmm(dst), xmm(src));
            return;
        }
        break;
    }
  }
  else
  {
    Mem mem = cc._getVarMem(vdata);

    switch (argType.variableType)
    {
      case VARIABLE_TYPE_GPD:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
#if defined(ASMJIT_X64)
          case VARIABLE_TYPE_GPQ:
#endif // ASMJIT_X64
            compiler->emit(INST_MOV, gpd(dst), mem);
            return;
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVD, gpd(dst), mem);
            return;
        }
        break;

#if defined(ASMJIT_X64)
      case VARIABLE_TYPE_GPQ:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
            compiler->emit(INST_MOV, gpd(dst), mem);
            return;
          case VARIABLE_TYPE_GPQ:
            compiler->emit(INST_MOV, gpq(dst), mem);
            return;
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, gpq(dst), mem);
            return;
        }
        break;
#endif // ASMJIT_X64

      case VARIABLE_TYPE_MM:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
            compiler->emit(INST_MOVD, gpd(dst), mem);
            return;
#if defined(ASMJIT_X64)
          case VARIABLE_TYPE_GPQ:
            compiler->emit(INST_MOVQ, gpq(dst), mem);
            return;
#endif // ASMJIT_X64
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, mm(dst), mem);
            return;
        }
        break;

      case VARIABLE_TYPE_XMM:
      case VARIABLE_TYPE_XMM_4F:
      case VARIABLE_TYPE_XMM_2D:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_GPD:
            compiler->emit(INST_MOVD, xmm(dst), mem);
            return;
#if defined(ASMJIT_X64)
          case VARIABLE_TYPE_GPQ:
            compiler->emit(INST_MOVQ, xmm(dst), mem);
            return;
#endif // ASMJIT_X64
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, xmm(dst), mem);
            return;
          case VARIABLE_TYPE_XMM:
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
            compiler->emit(INST_MOVDQA, xmm(dst), mem);
            return;
        }
        break;

      case VARIABLE_TYPE_XMM_1F:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, xmm(dst), mem);
            return;

          case VARIABLE_TYPE_XMM:
            compiler->emit(INST_MOVDQA, xmm(dst), mem);
            return;
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
            compiler->emit(INST_MOVSS, xmm(dst), mem);
            return;
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
            compiler->emit(INST_CVTSD2SS, xmm(dst), mem);
            return;
        }
        break;

      case VARIABLE_TYPE_XMM_1D:
        switch (vdata->type)
        {
          case VARIABLE_TYPE_MM:
            compiler->emit(INST_MOVQ, xmm(dst), mem);
            return;

          case VARIABLE_TYPE_XMM:
            compiler->emit(INST_MOVDQA, xmm(dst), mem);
            return;
          case VARIABLE_TYPE_XMM_1F:
          case VARIABLE_TYPE_XMM_4F:
            compiler->emit(INST_CVTSS2SD, xmm(dst), mem);
            return;
          case VARIABLE_TYPE_XMM_1D:
          case VARIABLE_TYPE_XMM_2D:
            compiler->emit(INST_MOVSD, xmm(dst), mem);
            return;
        }
        break;
    }
  }

  compiler->setError(ERROR_INCOMPATIBLE_ARGUMENT);
}

// Prototype & Arguments Management.
void ECall::_setPrototype(
  uint32_t callingConvention,
  const uint32_t* arguments,
  uint32_t argumentsCount,
  uint32_t returnValue) ASMJIT_NOTHROW
{
  _functionPrototype.setPrototype(callingConvention, arguments, argumentsCount, returnValue);

  _args = reinterpret_cast<Operand*>(
    getCompiler()->getZone().zalloc(sizeof(Operand) * argumentsCount));
  memset(_args, 0, sizeof(Operand) * argumentsCount);
}

bool ECall::setArgument(uint32_t i, const BaseVar& var) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(i < _functionPrototype.getArgumentsCount());
  if (i >= _functionPrototype.getArgumentsCount()) return false;

  _args[i] = var;
  return true;
}

bool ECall::setArgument(uint32_t i, const Imm& imm) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(i < _functionPrototype.getArgumentsCount());
  if (i >= _functionPrototype.getArgumentsCount()) return false;

  _args[i] = imm;
  return true;
}

bool ECall::setReturn(const Operand& first, const Operand& second) ASMJIT_NOTHROW
{
  _ret[0] = first;
  _ret[1] = second;

  return true;
}

// ============================================================================
// [AsmJit::ERet]
// ============================================================================

ERet::ERet(Compiler* c, EFunction* function, const Operand* first, const Operand* second) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_RET),
  _function(function)
{
  if (first ) _ret[0] = *first;
  if (second) _ret[1] = *second;

/*
  // TODO:?

  // Check whether the return value is compatible.
  uint32_t retValType = function->getPrototype().getReturnValue();
  bool valid = false;

  switch (retValType)
  {
    case VARIABLE_TYPE_GPD:
    case VARIABLE_TYPE_GPQ:
      if ((_ret[0].isVar() && (reinterpret_cast<const BaseVar&>(_ret[0]).isGPVar())) ||
          (_ret[0].isImm()))
      {
        valid = true;
      }
      break;

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      if ((_ret[0].isVar() && (reinterpret_cast<const BaseVar&>(_ret[0]).isX87Var() ||
                               reinterpret_cast<const BaseVar&>(_ret[0]).isXMMVar() )) )
      {
        valid = true;
      }
      break;

    case VARIABLE_TYPE_MM:
      break;

    case INVALID_VALUE:
      if (_ret[0].isNone() && 
          _ret[1].isNone())
      {
        valid = true;
      }
      break;

    default:
      break;
  }

  // Incompatible return value.
  if (!valid)
  {
    c->setError(ERROR_INCOMPATIBLE_RETURN_VALUE);
  }
*/
}

ERet::~ERet() ASMJIT_NOTHROW
{
}

void ERet::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset;

  uint32_t retValType = getFunction()->getPrototype().getReturnValue();
  if (retValType != INVALID_VALUE)
  {
    uint32_t i;
    for (i = 0; i < 2; i++)
    {
      Operand& o = _ret[i];

      if (o.isVar())
      {
        ASMJIT_ASSERT(o.getId() != INVALID_VALUE);
        VarData* vdata = _compiler->_getVarData(o.getId());
        ASMJIT_ASSERT(vdata != NULL);

        // First emittable (begin of variable scope).
        if (vdata->firstEmittable == NULL) vdata->firstEmittable = this;

        // Last emittable (end of variable scope).
        vdata->lastEmittable = this;

        if (vdata->workOffset == _offset) continue;
        if (!cc._isActive(vdata)) cc._addActive(vdata);

        vdata->workOffset = _offset;
        vdata->registerReadCount++;

        if (isVariableInteger(vdata->type) && isVariableInteger(retValType))
        {
          cc._newRegisterHomeIndex(vdata, (i == 0) ? REG_INDEX_EAX : REG_INDEX_EDX);
        }
      }
    }
  }

  cc._currentOffset++;
}

Emittable* ERet::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  Compiler* compiler = cc.getCompiler();

  // Check whether the return value is compatible.
  uint32_t retValType = getFunction()->getPrototype().getReturnValue();
  uint32_t i;

  switch ((int)retValType)
  {
    case VARIABLE_TYPE_GPD:
    case VARIABLE_TYPE_GPQ:
      for (i = 0; i < 2; i++)
      {
        uint32_t dstIndex = (i == 0) ? REG_INDEX_EAX : REG_INDEX_EDX;
        uint32_t srcIndex;

        if (_ret[i].isVar())
        {
          if (reinterpret_cast<const BaseVar&>(_ret[i]).isGPVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srcIndex = vdata->registerIndex;
            if (srcIndex == INVALID_VALUE)
              compiler->emit(INST_MOV, gpn(dstIndex), cc._getVarMem(vdata));
            else if (dstIndex != srcIndex)
              compiler->emit(INST_MOV, gpn(dstIndex), gpn(srcIndex));
          }
        }
        else if (_ret[i].isImm())
        {
          compiler->emit(INST_MOV, gpn(dstIndex), _ret[i]);
        }
      }
      break;

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // There is case that we need to return two values (Unix-ABI specific):
      // - FLD #2
      //-  FLD #1
      i = 2;
      do {
        i--;
        uint32_t dsti = i;
        uint32_t srci;

        if (_ret[i].isVar())
        {
          if (reinterpret_cast<const BaseVar&>(_ret[i]).isX87Var())
          {
            // TODO: X87.
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isXMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            if (srci != INVALID_VALUE) cc.saveXMMVar(vdata);

            switch (vdata->type)
            {
              case VARIABLE_TYPE_XMM_1F:
              case VARIABLE_TYPE_XMM_4F:
                compiler->emit(INST_FLD, _BaseVarMem(reinterpret_cast<BaseVar&>(_ret[i]), 4));
                break;
              case VARIABLE_TYPE_XMM_1D:
              case VARIABLE_TYPE_XMM_2D:
                compiler->emit(INST_FLD, _BaseVarMem(reinterpret_cast<BaseVar&>(_ret[i]), 8));
                break;
            }
          }
        }
      } while (i != 0);
      break;

    case VARIABLE_TYPE_MM:
      for (i = 0; i < 2; i++)
      {
        uint32_t dsti = i;
        uint32_t srci;

        if (_ret[i].isVar())
        {
          if (reinterpret_cast<const BaseVar&>(_ret[i]).isGPVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            uint32_t inst = _ret[i].isRegType(REG_TYPE_GPQ) ? INST_MOVQ : INST_MOVD;

            if (srci == INVALID_VALUE)
              compiler->emit(inst, mm(dsti), cc._getVarMem(vdata));
            else
#if defined(ASMJIT_X86)
              compiler->emit(inst, mm(dsti), gpd(srci));
#else
              compiler->emit(inst, mm(dsti), _ret[i].isRegType(REG_TYPE_GPQ) ? gpq(srci) : gpd(srci));
#endif
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            uint32_t inst = INST_MOVQ;

            if (srci == INVALID_VALUE)
              compiler->emit(inst, mm(dsti), cc._getVarMem(vdata));
            else if (dsti != srci)
              compiler->emit(inst, mm(dsti), mm(srci));
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isXMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            uint32_t inst = INST_MOVQ;
            if (reinterpret_cast<const BaseVar&>(_ret[i]).getVariableType() == VARIABLE_TYPE_XMM_1F) inst = INST_MOVD;

            if (srci == INVALID_VALUE)
              compiler->emit(inst, mm(dsti), cc._getVarMem(vdata));
            else
              compiler->emit(inst, mm(dsti), xmm(srci));
          }
        }
      }
      break;

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_2D:
      for (i = 0; i < 2; i++)
      {
        uint32_t dsti = i;
        uint32_t srci;

        if (_ret[i].isVar())
        {
          if (reinterpret_cast<const BaseVar&>(_ret[i]).isGPVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            uint32_t inst = _ret[i].isRegType(REG_TYPE_GPQ) ? INST_MOVQ : INST_MOVD;

            if (srci == INVALID_VALUE)
              compiler->emit(inst, xmm(dsti), cc._getVarMem(vdata));
            else
#if defined(ASMJIT_X86)
              compiler->emit(inst, xmm(dsti), gpd(srci));
#else
              compiler->emit(inst, xmm(dsti), _ret[i].isRegType(REG_TYPE_GPQ) ? gpq(srci) : gpd(srci));
#endif
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isX87Var())
          {
            // TODO: X87.
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            if (srci == INVALID_VALUE)
              compiler->emit(INST_MOVQ, xmm(dsti), cc._getVarMem(vdata));
            else
              compiler->emit(INST_MOVQ, xmm(dsti), mm(srci));
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isXMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            if (srci == INVALID_VALUE)
              compiler->emit(INST_MOVDQA, xmm(dsti), cc._getVarMem(vdata));
            else if (dsti != srci)
              compiler->emit(INST_MOVDQA, xmm(dsti), xmm(srci));
          }
        }
      }
      break;

    case VARIABLE_TYPE_XMM_1F:
      for (i = 0; i < 2; i++)
      {
        uint32_t dsti = i;
        uint32_t srci;

        if (_ret[i].isVar())
        {
          if (reinterpret_cast<const BaseVar&>(_ret[i]).isX87Var())
          {
            // TODO: X87.
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isXMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            switch (vdata->type)
            {
              case VARIABLE_TYPE_XMM:
                if (srci == INVALID_VALUE)
                  compiler->emit(INST_MOVDQA, xmm(dsti), cc._getVarMem(vdata));
                else if (dsti != srci)
                  compiler->emit(INST_MOVDQA, xmm(dsti), xmm(srci));
                break;
              case VARIABLE_TYPE_XMM_1F:
              case VARIABLE_TYPE_XMM_4F:
                if (srci == INVALID_VALUE)
                  compiler->emit(INST_MOVSS, xmm(dsti), cc._getVarMem(vdata));
                else
                  compiler->emit(INST_MOVSS, xmm(dsti), xmm(srci));
                break;
              case VARIABLE_TYPE_XMM_1D:
              case VARIABLE_TYPE_XMM_2D:
                if (srci == INVALID_VALUE)
                  compiler->emit(INST_CVTSD2SS, xmm(dsti), cc._getVarMem(vdata));
                else if (dsti != srci)
                  compiler->emit(INST_CVTSD2SS, xmm(dsti), xmm(srci));
                break;
            }
          }
        }
      }
      break;

    case VARIABLE_TYPE_XMM_1D:
      for (i = 0; i < 2; i++)
      {
        uint32_t dsti = i;
        uint32_t srci;

        if (_ret[i].isVar())
        {
          if (reinterpret_cast<const BaseVar&>(_ret[i]).isX87Var())
          {
            // TODO: X87.
          }
          else if (reinterpret_cast<const BaseVar&>(_ret[i]).isXMMVar())
          {
            VarData* vdata = compiler->_getVarData(_ret[i].getId());
            ASMJIT_ASSERT(vdata != NULL);

            srci = vdata->registerIndex;
            switch (vdata->type)
            {
              case VARIABLE_TYPE_XMM:
                if (srci == INVALID_VALUE)
                  compiler->emit(INST_MOVDQA, xmm(dsti), cc._getVarMem(vdata));
                else if (dsti != srci)
                  compiler->emit(INST_MOVDQA, xmm(dsti), xmm(srci));
                break;
              case VARIABLE_TYPE_XMM_1F:
              case VARIABLE_TYPE_XMM_4F:
                if (srci == INVALID_VALUE)
                  compiler->emit(INST_CVTSS2SD, xmm(dsti), cc._getVarMem(vdata));
                else
                  compiler->emit(INST_CVTSS2SD, xmm(dsti), xmm(srci));
                break;
              case VARIABLE_TYPE_XMM_1D:
              case VARIABLE_TYPE_XMM_2D:
                if (srci == INVALID_VALUE)
                  compiler->emit(INST_MOVSD, xmm(dsti), cc._getVarMem(vdata));
                else
                  compiler->emit(INST_MOVSD, xmm(dsti), xmm(srci));
                break;
            }
          }
        }
      }
      break;

    case INVALID_VALUE:
    default:
      break;
  }

  if (shouldEmitJumpToEpilog())
  {
    cc._unreachable = 1;
  }

  for (i = 0; i < 2; i++)
  {
    if (_ret[i].isVar())
    {
      VarData* vdata = compiler->_getVarData(_ret[i].getId());
      cc._unuseVarOnEndOfScope(this, vdata);
    }
  }

  return translated();
}

void ERet::emit(Assembler& a) ASMJIT_NOTHROW
{
  if (shouldEmitJumpToEpilog())
  {
    a.jmp(getFunction()->getExitLabel());
  }
}

int ERet::getMaxSize() const ASMJIT_NOTHROW
{
  return shouldEmitJumpToEpilog() ? 15 : 0;
}

bool ERet::shouldEmitJumpToEpilog() const ASMJIT_NOTHROW
{
  // Iterate over next emittables. If we found emittable that emits real 
  // instruction then we must return @c true.
  Emittable* e = this->getNext();

  while (e)
  {
    switch (e->getType())
    {
      // Non-interesting emittables.
      case EMITTABLE_COMMENT:
      case EMITTABLE_DUMMY:
      case EMITTABLE_ALIGN:
      case EMITTABLE_BLOCK:
      case EMITTABLE_VARIABLE_HINT:
      case EMITTABLE_TARGET:
        break;

      // Interesting emittables.
      case EMITTABLE_EMBEDDED_DATA:
      case EMITTABLE_INSTRUCTION:
      case EMITTABLE_JUMP_TABLE:
      case EMITTABLE_CALL:
      case EMITTABLE_RET:
        return true;

      // These emittables shouldn't be here. We are inside function, after
      // prolog.
      case EMITTABLE_FUNCTION:
      case EMITTABLE_PROLOG:
        break;

      // Stop station, we can't go forward from here.
      case EMITTABLE_EPILOG:
        return false;
    }
    e = e->getNext();
  }

  return false;
}

// ============================================================================
// [AsmJit::CompilerContext - Construction / Destruction]
// ============================================================================

CompilerContext::CompilerContext(Compiler* compiler) ASMJIT_NOTHROW :
  _zone(8192 - sizeof(Zone::Chunk) - 32)
{
  _compiler = compiler;
  _clear();

  _emitComments = compiler->getLogger() != NULL;
}

CompilerContext::~CompilerContext() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerContext - Clear]
// ============================================================================

void CompilerContext::_clear() ASMJIT_NOTHROW
{
  _zone.clear();
  _function = NULL;

  _start = NULL;
  _stop = NULL;

  _state.clear();
  _active = NULL;

  _forwardJumps = NULL;

  _currentOffset = 0;
  _unreachable = 0;

  _modifiedGPRegisters = 0;
  _modifiedMMRegisters = 0;
  _modifiedXMMRegisters = 0;

  _allocableEBP = false;

  _adjustESP = 0;

  _argumentsBaseReg = INVALID_VALUE; // Used by patcher.
  _argumentsBaseOffset = 0;          // Used by patcher.
  _argumentsActualDisp = 0;          // Used by translate().

  _variablesBaseReg = INVALID_VALUE; // Used by patcher.
  _variablesBaseOffset = 0;          // Used by patcher.
  _variablesActualDisp = 0;          // Used by translate()

  _memUsed = NULL;
  _memFree = NULL;

  _mem4BlocksCount = 0;
  _mem8BlocksCount = 0;
  _mem16BlocksCount = 0;

  _memBytesTotal = 0;

  _backCode.clear();
  _backPos = 0;
}

// ============================================================================
// [AsmJit::CompilerContext - Construction / Destruction]
// ============================================================================

void CompilerContext::allocVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW
{
  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
#endif // ASMJIT_X64
      allocGPVar(vdata, regMask, vflags);
      break;

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    case VARIABLE_TYPE_MM:
      allocMMVar(vdata, regMask, vflags);
      break;

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_1F:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_1D:
    case VARIABLE_TYPE_XMM_2D:
      allocXMMVar(vdata, regMask, vflags);
      break;
  }

  _postAlloc(vdata, vflags);
}

void CompilerContext::saveVar(VarData* vdata) ASMJIT_NOTHROW
{
  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
#endif // ASMJIT_X64
      saveGPVar(vdata);
      break;

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    case VARIABLE_TYPE_MM:
      saveMMVar(vdata);
      break;

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_1F:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_1D:
    case VARIABLE_TYPE_XMM_2D:
      saveXMMVar(vdata);
      break;
  }
}

void CompilerContext::spillVar(VarData* vdata) ASMJIT_NOTHROW
{
  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
#endif // ASMJIT_X64
      spillGPVar(vdata);
      break;

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    case VARIABLE_TYPE_MM:
      spillMMVar(vdata);
      break;

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_1F:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_1D:
    case VARIABLE_TYPE_XMM_2D:
      spillXMMVar(vdata);
      break;
  }
}

void CompilerContext::unuseVar(VarData* vdata, uint32_t toState) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(toState != VARIABLE_STATE_REGISTER);

  if (vdata->state == VARIABLE_STATE_REGISTER)
  {
    uint32_t registerIndex = vdata->registerIndex;
    switch (vdata->type)
    {
      case VARIABLE_TYPE_GPD:
#if defined(ASMJIT_X64)
      case VARIABLE_TYPE_GPQ:
#endif // ASMJIT_X64
        _state.gp[registerIndex] = NULL;
        _freedGPRegister(registerIndex);
        break;

      case VARIABLE_TYPE_X87:
      case VARIABLE_TYPE_X87_1F:
      case VARIABLE_TYPE_X87_1D:
        // TODO: X87 VARIABLES NOT IMPLEMENTED.
        break;

      case VARIABLE_TYPE_MM:
        _state.mm[registerIndex] = NULL;
        _freedMMRegister(registerIndex);
        break;

      case VARIABLE_TYPE_XMM:
      case VARIABLE_TYPE_XMM_1F:
      case VARIABLE_TYPE_XMM_4F:
      case VARIABLE_TYPE_XMM_1D:
      case VARIABLE_TYPE_XMM_2D:
        _state.xmm[registerIndex] = NULL;
        _freedXMMRegister(registerIndex);
        break;
    }
  }

  vdata->state = toState;
  vdata->changed = false;
  vdata->registerIndex = INVALID_VALUE;
}

void CompilerContext::allocGPVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW
{
  // Fix the regMask (0 or full bit-array means that any register may be used).
  if (regMask == 0) regMask = Util::maskUpToIndex(REG_NUM_GP);
  regMask &= Util::maskUpToIndex(REG_NUM_GP);

  // Working variables.
  uint32_t i;
  uint32_t mask;

  // Last register code (aka home).
  uint32_t home = vdata->homeRegisterIndex;
  // New register code.
  uint32_t idx = INVALID_VALUE;

  // Preserved GP variables.
  uint32_t preservedGP = vdata->scope->getPrototype().getPreservedGP();

  // Spill candidate.
  VarData* spillCandidate = NULL;

  // Whether to alloc the non-preserved variables first.
  bool nonPreservedFirst = true;
  if (getFunction()->_isCaller)
  {
    nonPreservedFirst = vdata->firstCallable == NULL || 
                        vdata->firstCallable->getOffset() >= vdata->lastEmittable->getOffset();
  }

  // --------------------------------------------------------------------------
  // [Already Allocated]
  // --------------------------------------------------------------------------

  // Go away if variable is already allocated.
  if (vdata->state == VARIABLE_STATE_REGISTER)
  {
    uint32_t oldIndex = vdata->registerIndex;

    // Already allocated in the right register.
    if (Util::maskFromIndex(oldIndex) & regMask) return;

    // Try to find unallocated register first.
    mask = regMask & ~_state.usedGP;
    if (mask != 0)
    {
      idx = Util::findFirstBit(
        (nonPreservedFirst && (mask & ~preservedGP) != 0) ? mask & ~preservedGP : mask);
    }
    // Then find the allocated and later exchange.
    else
    {
      idx = Util::findFirstBit(regMask & _state.usedGP);
    }
    ASMJIT_ASSERT(idx != INVALID_VALUE);

    VarData* other = _state.gp[idx];
    emitExchangeVar(vdata, idx, vflags, other);

    _state.gp[oldIndex] = other;
    _state.gp[idx     ] = vdata;

    if (other)
      other->registerIndex = oldIndex;
    else
      _freedGPRegister(oldIndex);

    // Update VarData.
    vdata->state = VARIABLE_STATE_REGISTER;
    vdata->registerIndex = idx;
    vdata->homeRegisterIndex = idx;

    _allocatedGPRegister(idx);
    return;
  }

  // --------------------------------------------------------------------------
  // [Find Unused GP]
  // --------------------------------------------------------------------------

  // If regMask contains restricted registers which may be used then everything
  // is handled in this block.
  if (regMask != Util::maskUpToIndex(REG_NUM_GP))
  {
    // Try to find unallocated register first.
    mask = regMask & ~_state.usedGP;
    if (mask != 0)
    {
      idx = Util::findFirstBit(
        (nonPreservedFirst && (mask & ~preservedGP) != 0) ? (mask & ~preservedGP) : mask);
      ASMJIT_ASSERT(idx != INVALID_VALUE);
    }
    // Then find the allocated and later spill.
    else
    {
      idx = Util::findFirstBit(regMask & _state.usedGP);
      ASMJIT_ASSERT(idx != INVALID_VALUE);

      // Spill register we need.
      spillCandidate = _state.gp[idx];

      // Jump to spill part of allocation.
      goto L_Spill;
    }
  }

  // Home register code.
  if (idx == INVALID_VALUE && home != INVALID_VALUE)
  {
    if ((_state.usedGP & (1U << home)) == 0) idx = home;
  }

  // We start from 1, because EAX/RAX register is sometimes explicitly
  // needed. So we trying to prevent reallocation in near future.
  if (idx == INVALID_VALUE)
  {
    for (i = 1, mask = (1 << i); i < REG_NUM_GP; i++, mask <<= 1)
    {
      if ((_state.usedGP & mask) == 0 && (i != REG_INDEX_EBP || _allocableEBP) && (i != REG_INDEX_ESP))
      {
        // Convenience to alloc non-preserved first or non-preserved last.
        if (nonPreservedFirst)
        {
          if (idx != INVALID_VALUE && (preservedGP & mask) != 0) continue;
          idx = i;
          // If current register is preserved, we should try to find different
          // one that is not. This can save one push / pop in prolog / epilog.
          if ((preservedGP & mask) == 0) break;
        }
        else
        {
          if (idx != INVALID_VALUE && (preservedGP & mask) == 0) continue;
          idx = i;
          // The opposite.
          if ((preservedGP & mask) != 0) break;
        }
      }
    }
  }

  // If not found, try EAX/RAX.
  if (idx == INVALID_VALUE && (_state.usedGP & 1) == 0)
  {
    idx = REG_INDEX_EAX;
  }

  // --------------------------------------------------------------------------
  // [Spill]
  // --------------------------------------------------------------------------

  // If register is still not found, spill other variable.
  if (idx == INVALID_VALUE)
  {
    if (spillCandidate == NULL)
    {
      spillCandidate = _getSpillCandidateGP();
    }

    // Spill candidate not found?
    if (spillCandidate == NULL)
    {
      _compiler->setError(ERROR_NOT_ENOUGH_REGISTERS);
      return;
    }

L_Spill:

    // Prevented variables can't be spilled. _getSpillCandidate() never returns
    // prevented variables, but when jumping to L_spill it can happen.
    if (spillCandidate->workOffset == _currentOffset)
    {
      _compiler->setError(ERROR_REGISTERS_OVERLAP);
      return;
    }

    idx = spillCandidate->registerIndex;
    spillGPVar(spillCandidate);
  }

  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  if (vdata->state == VARIABLE_STATE_MEMORY && (vflags & VARIABLE_ALLOC_READ) != 0)
  {
    emitLoadVar(vdata, idx);
  }

  // Update VarData.
  vdata->state = VARIABLE_STATE_REGISTER;
  vdata->registerIndex = idx;
  vdata->homeRegisterIndex = idx;

  // Update StateData.
  _allocatedVariable(vdata);
}

void CompilerContext::saveGPVar(VarData* vdata) ASMJIT_NOTHROW
{
  // Can't save variable that isn't allocated.
  ASMJIT_ASSERT(vdata->state == VARIABLE_STATE_REGISTER);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  uint32_t idx = vdata->registerIndex;
  emitSaveVar(vdata, idx);

  // Update VarData.
  vdata->changed = false;
}

void CompilerContext::spillGPVar(VarData* vdata) ASMJIT_NOTHROW
{
  // Can't spill variable that isn't allocated.
  ASMJIT_ASSERT(vdata->state == VARIABLE_STATE_REGISTER);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  uint32_t idx = vdata->registerIndex;

  if (vdata->changed) emitSaveVar(vdata, idx);

  // Update VarData.
  vdata->registerIndex = INVALID_VALUE;
  vdata->state = VARIABLE_STATE_MEMORY;
  vdata->changed = false;

  // Update StateData.
  _state.gp[idx] = NULL;
  _freedGPRegister(idx);
}

void CompilerContext::allocMMVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW
{
  // Fix the regMask (0 or full bit-array means that any register may be used).
  if (regMask == 0) regMask = Util::maskUpToIndex(REG_NUM_MM);
  regMask &= Util::maskUpToIndex(REG_NUM_MM);

  // Working variables.
  uint32_t i;
  uint32_t mask;

  // Last register code (aka home).
  uint32_t home = vdata->homeRegisterIndex;
  // New register code.
  uint32_t idx = INVALID_VALUE;

  // Preserved MM variables.
  //
  // NOTE: Currently MM variables are not preserved and there is no calling
  // convention known to me that does that. But on the other side it's possible
  // to write such calling convention.
  uint32_t preservedMM = vdata->scope->getPrototype().getPreservedMM();

  // Spill candidate.
  VarData* spillCandidate = NULL;

  // Whether to alloc non-preserved first or last.
  bool nonPreservedFirst = true;
  if (this->getFunction()->_isCaller)
  {
    nonPreservedFirst = vdata->firstCallable == NULL || 
                        vdata->firstCallable->getOffset() >= vdata->lastEmittable->getOffset();
  }

  // --------------------------------------------------------------------------
  // [Already Allocated]
  // --------------------------------------------------------------------------

  // Go away if variable is already allocated.
  if (vdata->state == VARIABLE_STATE_REGISTER)
  {
    uint32_t oldIndex = vdata->registerIndex;

    // Already allocated in the right register.
    if (Util::maskFromIndex(oldIndex) & regMask) return;

    // Try to find unallocated register first.
    mask = regMask & ~_state.usedMM;
    if (mask != 0)
    {
      idx = Util::findFirstBit(
        (nonPreservedFirst && (mask & ~preservedMM) != 0) ? mask & ~preservedMM : mask);
    }
    // Then find the allocated and later exchange.
    else
    {
      idx = Util::findFirstBit(regMask & _state.usedMM);
    }
    ASMJIT_ASSERT(idx != INVALID_VALUE);

    VarData* other = _state.mm[idx];
    if (other) spillMMVar(other);

    emitMoveVar(vdata, idx, vflags);
    _freedMMRegister(oldIndex);
    _state.mm[idx] = vdata;

    // Update VarData.
    vdata->state = VARIABLE_STATE_REGISTER;
    vdata->registerIndex = idx;
    vdata->homeRegisterIndex = idx;

    _allocatedMMRegister(idx);
    return;
  }

  // --------------------------------------------------------------------------
  // [Find Unused MM]
  // --------------------------------------------------------------------------

  // If regMask contains restricted registers which may be used then everything
  // is handled in this block.
  if (regMask != Util::maskUpToIndex(REG_NUM_MM))
  {
    // Try to find unallocated register first.
    mask = regMask & ~_state.usedMM;
    if (mask != 0)
    {
      idx = Util::findFirstBit(
        (nonPreservedFirst && (mask & ~preservedMM) != 0) ? mask & ~preservedMM : mask);
      ASMJIT_ASSERT(idx != INVALID_VALUE);
    }
    // Then find the allocated and later spill.
    else
    {
      idx = Util::findFirstBit(regMask & _state.usedMM);
      ASMJIT_ASSERT(idx != INVALID_VALUE);

      // Spill register we need.
      spillCandidate = _state.mm[idx];

      // Jump to spill part of allocation.
      goto L_Spill;
    }
  }

  // Home register code.
  if (idx == INVALID_VALUE && home != INVALID_VALUE)
  {
    if ((_state.usedMM & (1U << home)) == 0) idx = home;
  }

  if (idx == INVALID_VALUE)
  {
    for (i = 0, mask = (1 << i); i < REG_NUM_MM; i++, mask <<= 1)
    {
      if ((_state.usedMM & mask) == 0)
      {
        // Convenience to alloc non-preserved first or non-preserved last.
        if (nonPreservedFirst)
        {
          if (idx != INVALID_VALUE && (preservedMM & mask) != 0) continue;
          idx = i;
          // If current register is preserved, we should try to find different
          // one that is not. This can save one push / pop in prolog / epilog.
          if ((preservedMM & mask) == 0) break;
        }
        else
        {
          if (idx != INVALID_VALUE && (preservedMM & mask) == 0) continue;
          idx = i;
          // The opposite.
          if ((preservedMM & mask) != 0) break;
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // [Spill]
  // --------------------------------------------------------------------------

  // If register is still not found, spill other variable.
  if (idx == INVALID_VALUE)
  {
    if (spillCandidate == NULL) spillCandidate = _getSpillCandidateMM();

    // Spill candidate not found?
    if (spillCandidate == NULL)
    {
      _compiler->setError(ERROR_NOT_ENOUGH_REGISTERS);
      return;
    }

L_Spill:

    // Prevented variables can't be spilled. _getSpillCandidate() never returns
    // prevented variables, but when jumping to L_spill it can happen.
    if (spillCandidate->workOffset == _currentOffset)
    {
      _compiler->setError(ERROR_REGISTERS_OVERLAP);
      return;
    }

    idx = spillCandidate->registerIndex;
    spillMMVar(spillCandidate);
  }

  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  if (vdata->state == VARIABLE_STATE_MEMORY && (vflags & VARIABLE_ALLOC_READ) != 0)
  {
    emitLoadVar(vdata, idx);
  }

  // Update VarData.
  vdata->state = VARIABLE_STATE_REGISTER;
  vdata->registerIndex = idx;
  vdata->homeRegisterIndex = idx;

  // Update StateData.
  _allocatedVariable(vdata);
}

void CompilerContext::saveMMVar(VarData* vdata) ASMJIT_NOTHROW
{
  // Can't save variable that isn't allocated.
  ASMJIT_ASSERT(vdata->state == VARIABLE_STATE_REGISTER);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  uint32_t idx = vdata->registerIndex;
  emitSaveVar(vdata, idx);

  // Update VarData.
  vdata->changed = false;
}

void CompilerContext::spillMMVar(VarData* vdata) ASMJIT_NOTHROW
{
  // Can't spill variable that isn't allocated.
  ASMJIT_ASSERT(vdata->state == VARIABLE_STATE_REGISTER);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  uint32_t idx = vdata->registerIndex;

  if (vdata->changed) emitSaveVar(vdata, idx);

  // Update VarData.
  vdata->registerIndex = INVALID_VALUE;
  vdata->state = VARIABLE_STATE_MEMORY;
  vdata->changed = false;

  // Update StateData.
  _state.mm[idx] = NULL;
  _freedMMRegister(idx);
}

void CompilerContext::allocXMMVar(VarData* vdata, uint32_t regMask, uint32_t vflags) ASMJIT_NOTHROW
{
  // Fix the regMask (0 or full bit-array means that any register may be used).
  if (regMask == 0) regMask = Util::maskUpToIndex(REG_NUM_XMM);
  regMask &= Util::maskUpToIndex(REG_NUM_XMM);

  // Working variables.
  uint32_t i;
  uint32_t mask;

  // Last register code (aka home).
  uint32_t home = vdata->homeRegisterIndex;
  // New register code.
  uint32_t idx = INVALID_VALUE;

  // Preserved XMM variables.
  uint32_t preservedXMM = vdata->scope->getPrototype().getPreservedXMM();

  // Spill candidate.
  VarData* spillCandidate = NULL;

  // Whether to alloc non-preserved first or last.
  bool nonPreservedFirst = true;
  if (this->getFunction()->_isCaller)
  {
    nonPreservedFirst = vdata->firstCallable == NULL || 
                        vdata->firstCallable->getOffset() >= vdata->lastEmittable->getOffset();
  }

  // --------------------------------------------------------------------------
  // [Already Allocated]
  // --------------------------------------------------------------------------

  // Go away if variable is already allocated.
  if (vdata->state == VARIABLE_STATE_REGISTER)
  {
    uint32_t oldIndex = vdata->registerIndex;

    // Already allocated in the right register.
    if (Util::maskFromIndex(oldIndex) & regMask) return;

    // Try to find unallocated register first.
    mask = regMask & ~_state.usedXMM;
    if (mask != 0)
    {
      idx = Util::findFirstBit(
        (nonPreservedFirst && (mask & ~preservedXMM) != 0) ? mask & ~preservedXMM : mask);
    }
    // Then find the allocated and later exchange.
    else
    {
      idx = Util::findFirstBit(regMask & _state.usedXMM);
    }
    ASMJIT_ASSERT(idx != INVALID_VALUE);

    VarData* other = _state.xmm[idx];
    if (other) spillXMMVar(other);

    emitMoveVar(vdata, idx, vflags);
    _freedXMMRegister(oldIndex);
    _state.xmm[idx] = vdata;

    // Update VarData.
    vdata->state = VARIABLE_STATE_REGISTER;
    vdata->registerIndex = idx;
    vdata->homeRegisterIndex = idx;

    _allocatedXMMRegister(idx);
    return;
  }

  // --------------------------------------------------------------------------
  // [Find Unused XMM]
  // --------------------------------------------------------------------------

  // If regMask contains restricted registers which may be used then everything
  // is handled in this block.
  if (regMask != Util::maskUpToIndex(REG_NUM_XMM))
  {
    // Try to find unallocated register first.
    mask = regMask & ~_state.usedXMM;
    if (mask != 0)
    {
      idx = Util::findFirstBit(
        (nonPreservedFirst && (mask & ~preservedXMM) != 0) ? mask & ~preservedXMM : mask);
      ASMJIT_ASSERT(idx != INVALID_VALUE);
    }
    // Then find the allocated and later spill.
    else
    {
      idx = Util::findFirstBit(regMask & _state.usedXMM);
      ASMJIT_ASSERT(idx != INVALID_VALUE);

      // Spill register we need.
      spillCandidate = _state.xmm[idx];

      // Jump to spill part of allocation.
      goto L_Spill;
    }
  }

  // Home register code.
  if (idx == INVALID_VALUE && home != INVALID_VALUE)
  {
    if ((_state.usedXMM & (1U << home)) == 0) idx = home;
  }

  if (idx == INVALID_VALUE)
  {
    for (i = 0, mask = (1 << i); i < REG_NUM_XMM; i++, mask <<= 1)
    {
      if ((_state.usedXMM & mask) == 0)
      {
        // Convenience to alloc non-preserved first or non-preserved last.
        if (nonPreservedFirst)
        {
          if (idx != INVALID_VALUE && (preservedXMM & mask) != 0) continue;
          idx = i;
          // If current register is preserved, we should try to find different
          // one that is not. This can save one push / pop in prolog / epilog.
          if ((preservedXMM & mask) == 0) break;
        }
        else
        {
          if (idx != INVALID_VALUE && (preservedXMM & mask) == 0) continue;
          idx = i;
          // The opposite.
          if ((preservedXMM & mask) != 0) break;
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // [Spill]
  // --------------------------------------------------------------------------

  // If register is still not found, spill other variable.
  if (idx == INVALID_VALUE)
  {
    if (spillCandidate == NULL) spillCandidate = _getSpillCandidateXMM();

    // Spill candidate not found?
    if (spillCandidate == NULL)
    {
      _compiler->setError(ERROR_NOT_ENOUGH_REGISTERS);
      return;
    }

L_Spill:

    // Prevented variables can't be spilled. _getSpillCandidate() never returns
    // prevented variables, but when jumping to L_spill it can happen.
    if (spillCandidate->workOffset == _currentOffset)
    {
      _compiler->setError(ERROR_REGISTERS_OVERLAP);
      return;
    }

    idx = spillCandidate->registerIndex;
    spillXMMVar(spillCandidate);
  }

  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  if (vdata->state == VARIABLE_STATE_MEMORY && (vflags & VARIABLE_ALLOC_READ) != 0)
  {
    emitLoadVar(vdata, idx);
  }

  // Update VarData.
  vdata->state = VARIABLE_STATE_REGISTER;
  vdata->registerIndex = idx;
  vdata->homeRegisterIndex = idx;

  // Update StateData.
  _allocatedVariable(vdata);
}

void CompilerContext::saveXMMVar(VarData* vdata) ASMJIT_NOTHROW
{
  // Can't save variable that isn't allocated.
  ASMJIT_ASSERT(vdata->state == VARIABLE_STATE_REGISTER);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  uint32_t idx = vdata->registerIndex;
  emitSaveVar(vdata, idx);

  // Update VarData.
  vdata->changed = false;
}

void CompilerContext::spillXMMVar(VarData* vdata) ASMJIT_NOTHROW
{
  // Can't spill variable that isn't allocated.
  ASMJIT_ASSERT(vdata->state == VARIABLE_STATE_REGISTER);
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  uint32_t idx = vdata->registerIndex;

  if (vdata->changed) emitSaveVar(vdata, idx);

  // Update VarData.
  vdata->registerIndex = INVALID_VALUE;
  vdata->state = VARIABLE_STATE_MEMORY;
  vdata->changed = false;

  // Update StateData.
  _state.xmm[idx] = NULL;
  _freedXMMRegister(idx);
}

void CompilerContext::emitLoadVar(VarData* vdata, uint32_t regIndex) ASMJIT_NOTHROW
{
  Mem m = _getVarMem(vdata);

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
      _compiler->emit(INST_MOV, gpd(regIndex), m);
      if (_emitComments) goto addComment;
      break;
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
      _compiler->emit(INST_MOV, gpq(regIndex), m);
      if (_emitComments) goto addComment;
      break;
#endif // ASMJIT_X64

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    case VARIABLE_TYPE_MM:
      _compiler->emit(INST_MOVQ, mm(regIndex), m);
      if (_emitComments) goto addComment;
      break;

    case VARIABLE_TYPE_XMM:
      _compiler->emit(INST_MOVDQA, xmm(regIndex), m);
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_1F:
      _compiler->emit(INST_MOVSS, xmm(regIndex), m);
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_1D:
      _compiler->emit(INST_MOVSD, xmm(regIndex), m);
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_4F:
      _compiler->emit(INST_MOVAPS, xmm(regIndex), m);
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_2D:
      _compiler->emit(INST_MOVAPD, xmm(regIndex), m);
      if (_emitComments) goto addComment;
      break;
  }
  return;

addComment:
  _compiler->getCurrentEmittable()->setCommentF("Alloc %s", vdata->name);
}

void CompilerContext::emitSaveVar(VarData* vdata, uint32_t regIndex) ASMJIT_NOTHROW
{
  // Caller must ensure that variable is allocated.
  ASMJIT_ASSERT(regIndex != INVALID_VALUE);

  Mem m = _getVarMem(vdata);

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
      _compiler->emit(INST_MOV, m, gpd(regIndex));
      if (_emitComments) goto addComment;
      break;
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
      _compiler->emit(INST_MOV, m, gpq(regIndex));
      if (_emitComments) goto addComment;
      break;
#endif // ASMJIT_X64

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    case VARIABLE_TYPE_MM:
      _compiler->emit(INST_MOVQ, m, mm(regIndex));
      if (_emitComments) goto addComment;
      break;

    case VARIABLE_TYPE_XMM:
      _compiler->emit(INST_MOVDQA, m, xmm(regIndex));
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_1F:
      _compiler->emit(INST_MOVSS, m, xmm(regIndex));
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_1D:
      _compiler->emit(INST_MOVSD, m, xmm(regIndex));
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_4F:
      _compiler->emit(INST_MOVAPS, m, xmm(regIndex));
      if (_emitComments) goto addComment;
      break;
    case VARIABLE_TYPE_XMM_2D:
      _compiler->emit(INST_MOVAPD, m, xmm(regIndex));
      if (_emitComments) goto addComment;
      break;
  }
  return;

addComment:
  _compiler->getCurrentEmittable()->setCommentF("Spill %s", vdata->name);
}

void CompilerContext::emitMoveVar(VarData* vdata, uint32_t regIndex, uint32_t vflags) ASMJIT_NOTHROW
{
  // Caller must ensure that variable is allocated.
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  if ((vflags & VARIABLE_ALLOC_READ) == 0) return;

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
      _compiler->emit(INST_MOV, gpd(regIndex), gpd(vdata->registerIndex));
      break;
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
      _compiler->emit(INST_MOV, gpq(regIndex), gpq(vdata->registerIndex));
      break;
#endif // ASMJIT_X64

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    case VARIABLE_TYPE_MM:
      _compiler->emit(INST_MOVQ, mm(regIndex), mm(vdata->registerIndex));
      break;

    case VARIABLE_TYPE_XMM:
      _compiler->emit(INST_MOVDQA, xmm(regIndex), xmm(vdata->registerIndex));
      break;
    case VARIABLE_TYPE_XMM_1F:
      _compiler->emit(INST_MOVSS, xmm(regIndex), xmm(vdata->registerIndex));
      break;
    case VARIABLE_TYPE_XMM_1D:
      _compiler->emit(INST_MOVSD, xmm(regIndex), xmm(vdata->registerIndex));
      break;
    case VARIABLE_TYPE_XMM_4F:
      _compiler->emit(INST_MOVAPS, xmm(regIndex), xmm(vdata->registerIndex));
      break;
    case VARIABLE_TYPE_XMM_2D:
      _compiler->emit(INST_MOVAPD, xmm(regIndex), xmm(vdata->registerIndex));
      break;
  }
}

void CompilerContext::emitExchangeVar(VarData* vdata, uint32_t regIndex, uint32_t vflags, VarData* other) ASMJIT_NOTHROW
{
  // Caller must ensure that variable is allocated.
  ASMJIT_ASSERT(vdata->registerIndex != INVALID_VALUE);

  // If other is not valid then we can just emit MOV (or other similar instruction).
  if (other == NULL)
  {
    emitMoveVar(vdata, regIndex, vflags);
    return;
  }

  // If we need to alloc for write-only operation then we can move other
  // variable away instead of exchanging them.
  if ((vflags & VARIABLE_ALLOC_READ) == 0)
  {
    emitMoveVar(other, vdata->registerIndex, VARIABLE_ALLOC_READ);
    return;
  }

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
      _compiler->emit(INST_XCHG, gpd(regIndex), gpd(vdata->registerIndex));
      break;
#if defined(ASMJIT_X64)
    case VARIABLE_TYPE_GPQ:
      _compiler->emit(INST_XCHG, gpq(regIndex), gpq(vdata->registerIndex));
      break;
#endif // ASMJIT_X64

    case VARIABLE_TYPE_X87:
    case VARIABLE_TYPE_X87_1F:
    case VARIABLE_TYPE_X87_1D:
      // TODO: X87 VARIABLES NOT IMPLEMENTED.
      break;

    // NOTE: MM and XMM registers shoudln't be exchanged using this way, it's
    // correct, but it sucks.

    case VARIABLE_TYPE_MM:
    {
      MMReg a = mm(regIndex);
      MMReg b = mm(vdata->registerIndex);

      _compiler->emit(INST_PXOR, a, b);
      _compiler->emit(INST_PXOR, b, a);
      _compiler->emit(INST_PXOR, a, b);
      break;
    }

    case VARIABLE_TYPE_XMM_1F:
    case VARIABLE_TYPE_XMM_4F:
    {
      XMMReg a = xmm(regIndex);
      XMMReg b = xmm(vdata->registerIndex);

      _compiler->emit(INST_XORPS, a, b);
      _compiler->emit(INST_XORPS, b, a);
      _compiler->emit(INST_XORPS, a, b);
      break;
    }

    case VARIABLE_TYPE_XMM_1D:
    case VARIABLE_TYPE_XMM_2D:
    {
      XMMReg a = xmm(regIndex);
      XMMReg b = xmm(vdata->registerIndex);

      _compiler->emit(INST_XORPD, a, b);
      _compiler->emit(INST_XORPD, b, a);
      _compiler->emit(INST_XORPD, a, b);
      break;
    }

    case VARIABLE_TYPE_XMM:
    {
      XMMReg a = xmm(regIndex);
      XMMReg b = xmm(vdata->registerIndex);

      _compiler->emit(INST_PXOR, a, b);
      _compiler->emit(INST_PXOR, b, a);
      _compiler->emit(INST_PXOR, a, b);
      break;
    }
  }
}

void CompilerContext::_postAlloc(VarData* vdata, uint32_t vflags) ASMJIT_NOTHROW
{
  if (vflags & VARIABLE_ALLOC_WRITE) vdata->changed = true;
}

void CompilerContext::_markMemoryUsed(VarData* vdata) ASMJIT_NOTHROW
{
  if (vdata->homeMemoryData != NULL) return;

  VarMemBlock* mem = _allocMemBlock(vdata->size);
  if (!mem) return;

  vdata->homeMemoryData = mem;
}

Mem CompilerContext::_getVarMem(VarData* vdata) ASMJIT_NOTHROW
{
  Mem m;
  m._mem.id = vdata->id;
  if (!vdata->isMemArgument) m._mem.displacement = _adjustESP;

  _markMemoryUsed(vdata);
  return m;
}

static int32_t getSpillScore(VarData* v, uint32_t currentOffset)
{
  int32_t score = 0;

  ASMJIT_ASSERT(v->lastEmittable != NULL);
  uint32_t lastOffset = v->lastEmittable->getOffset();

  if (lastOffset >= currentOffset)
    score += (int32_t)(lastOffset - currentOffset);

  // Each write access decreases probability of spill.
  score -= (int32_t)v->registerWriteCount + (int32_t)v->registerRWCount;
  // Each read-only access increases probability of spill.
  score += (int32_t)v->registerReadCount;

  // Each memory access increases probability of spill.
  score += (int32_t)v->memoryWriteCount + (int32_t)v->memoryRWCount;
  score += (int32_t)v->memoryReadCount;

  return score;
}

VarData* CompilerContext::_getSpillCandidateGP() ASMJIT_NOTHROW
{
  return _getSpillCandidateGeneric(_state.gp, REG_NUM_GP);
}

VarData* CompilerContext::_getSpillCandidateMM() ASMJIT_NOTHROW
{
  return _getSpillCandidateGeneric(_state.mm, REG_NUM_MM);
}

VarData* CompilerContext::_getSpillCandidateXMM() ASMJIT_NOTHROW
{
  return _getSpillCandidateGeneric(_state.xmm, REG_NUM_XMM);
}

VarData* CompilerContext::_getSpillCandidateGeneric(VarData** varArray, uint32_t count) ASMJIT_NOTHROW
{
  uint32_t i;

  VarData* candidate = NULL;
  uint32_t candidatePriority = 0;
  int32_t candidateScore = 0;

  uint32_t currentOffset = _compiler->getCurrentEmittable()->getOffset();

  for (i = 0; i < count; i++)
  {
    // Get variable.
    VarData* vdata = varArray[i];

    // Never spill variables needed for next instruction.
    if (vdata == NULL || vdata->workOffset == _currentOffset) continue;

    uint32_t variablePriority = vdata->priority;
    int32_t variableScore = getSpillScore(vdata, currentOffset);

    if ((candidate == NULL) ||
        (variablePriority > candidatePriority) ||
        (variablePriority == candidatePriority && variableScore > candidateScore))
    {
      candidate = vdata;
      candidatePriority = variablePriority;
      candidateScore = variableScore;
    }
  }

  return candidate;
}

void CompilerContext::_addActive(VarData* vdata) ASMJIT_NOTHROW
{
  // Never call with variable that is already in active list.
  ASMJIT_ASSERT(vdata->nextActive == NULL);
  ASMJIT_ASSERT(vdata->prevActive == NULL);

  if (_active == NULL)
  {
    vdata->nextActive = vdata;
    vdata->prevActive = vdata;

    _active = vdata;
  }
  else
  {
    VarData* vlast = _active->prevActive;

    vlast->nextActive = vdata;
    _active->prevActive = vdata;

    vdata->nextActive = _active;
    vdata->prevActive = vlast;
  }
}

void CompilerContext::_freeActive(VarData* vdata) ASMJIT_NOTHROW
{
  VarData* next = vdata->nextActive;
  VarData* prev = vdata->prevActive;

  if (prev == next)
  {
    _active = NULL;
  }
  else
  {
    if (_active == vdata) _active = next;
    prev->nextActive = next;
    next->prevActive = prev;
  }

  vdata->nextActive = NULL;
  vdata->prevActive = NULL;
}

void CompilerContext::_freeAllActive() ASMJIT_NOTHROW
{
  if (_active == NULL) return;

  VarData* cur = _active;
  for (;;)
  {
    VarData* next = cur->nextActive;
    cur->nextActive = NULL;
    cur->prevActive = NULL;
    if (next == _active) break;
  }

  _active = NULL;
}

void CompilerContext::_allocatedVariable(VarData* vdata) ASMJIT_NOTHROW
{
  uint32_t idx = vdata->registerIndex;

  switch (vdata->type)
  {
    case VARIABLE_TYPE_GPD:
    case VARIABLE_TYPE_GPQ:
      _state.gp[idx] = vdata;
      _allocatedGPRegister(idx);
      break;

    case VARIABLE_TYPE_MM:
      _state.mm[idx] = vdata;
      _allocatedMMRegister(idx);
      break;

    case VARIABLE_TYPE_XMM:
    case VARIABLE_TYPE_XMM_1F:
    case VARIABLE_TYPE_XMM_4F:
    case VARIABLE_TYPE_XMM_1D:
    case VARIABLE_TYPE_XMM_2D:
      _state.xmm[idx] = vdata;
      _allocatedXMMRegister(idx);
      break;

    default:
      ASMJIT_ASSERT(0);
      break;
  }
}

void CompilerContext::translateOperands(Operand* operands, uint32_t count) ASMJIT_NOTHROW
{
  uint32_t i;

  // Translate variables to registers.
  for (i = 0; i < count; i++)
  {
    Operand& o = operands[i];

    if (o.isVar())
    {
      VarData* vdata = _compiler->_getVarData(o.getId());
      ASMJIT_ASSERT(vdata != NULL);

      o._reg.op = OPERAND_REG;
      o._reg.code |= vdata->registerIndex;
    }
    else if (o.isMem())
    {
      if ((o.getId() & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        // Memory access. We just increment here actual displacement.
        VarData* vdata = _compiler->_getVarData(o.getId());
        ASMJIT_ASSERT(vdata != NULL);

        o._mem.displacement += vdata->isMemArgument
          ? _argumentsActualDisp
          : _variablesActualDisp;
        // NOTE: This is not enough, variable position will be patched later
        // by CompilerContext::_patchMemoryOperands().
      }
      else if ((o._mem.base & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o._mem.base);
        ASMJIT_ASSERT(vdata != NULL);

        o._mem.base = vdata->registerIndex;
      }

      if ((o._mem.index & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(o._mem.index);
        ASMJIT_ASSERT(vdata != NULL);

        o._mem.index = vdata->registerIndex;
      }
    }
  }
}

void CompilerContext::addBackwardCode(EJmp* from) ASMJIT_NOTHROW
{
  _backCode.append(from);
}

void CompilerContext::addForwardJump(EJmp* inst) ASMJIT_NOTHROW
{
  ForwardJumpData* j =
    reinterpret_cast<ForwardJumpData*>(_zone.zalloc(sizeof(ForwardJumpData)));
  if (j == NULL) { _compiler->setError(ERROR_NO_HEAP_MEMORY); return; }

  j->inst = inst;
  j->state = _saveState();
  j->next = _forwardJumps;
  _forwardJumps = j;
}

StateData* CompilerContext::_saveState() ASMJIT_NOTHROW
{
  // Get count of variables stored in memory.
  uint32_t memVarsCount = 0;
  VarData* cur = _active;
  if (cur)
  {
    do {
      if (cur->state == VARIABLE_STATE_MEMORY) memVarsCount++;
      cur = cur->nextActive;
    } while (cur != _active);
  }

  // Alloc StateData structure (using zone allocator) and copy current state into it.
  StateData* state = _compiler->_newStateData(memVarsCount);
  memcpy(state, &_state, sizeof(StateData));

  // Clear changed flags.
  state->changedGP = 0;
  state->changedMM = 0;
  state->changedXMM = 0;

  uint i;
  uint mask;

  // Save variables stored in REGISTERs and CHANGE flag.
  for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
  {
    if (state->gp[i] && state->gp[i]->changed) state->changedGP |= mask;
  }

  for (i = 0, mask = 1; i < REG_NUM_MM; i++, mask <<= 1)
  {
    if (state->mm[i] && state->mm[i]->changed) state->changedMM |= mask;
  }

  for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
  {
    if (state->xmm[i] && state->xmm[i]->changed) state->changedXMM |= mask;
  }

  // Save variables stored in MEMORY.
  state->memVarsCount = memVarsCount;
  memVarsCount = 0;

  cur = _active;
  if (cur)
  {
    do {
      if (cur->state == VARIABLE_STATE_MEMORY) state->memVarsData[memVarsCount++] = cur;
      cur = cur->nextActive;
    } while (cur != _active);
  }

  // Finished.
  return state;
}

void CompilerContext::_assignState(StateData* state) ASMJIT_NOTHROW
{
  Compiler* compiler = getCompiler();

  memcpy(&_state, state, sizeof(StateData));
  _state.memVarsCount = 0;

  uint i, mask;
  VarData* vdata;

  // Unuse all variables first.
  vdata = _active;
  if (vdata)
  {
    do {
      vdata->state = VARIABLE_STATE_UNUSED;
      vdata = vdata->nextActive;
    } while (vdata != _active);
  }

  // Assign variables stored in memory which are not unused.
  for (i = 0; i < state->memVarsCount; i++)
  {
    state->memVarsData[i]->state = VARIABLE_STATE_MEMORY;
  }

  // Assign allocated variables.
  for (i = 0, mask = 1; i < REG_NUM_GP; i++, mask <<= 1)
  {
    if ((vdata = _state.gp[i]) != NULL)
    {
      vdata->state = VARIABLE_STATE_REGISTER;
      vdata->registerIndex = i;
      vdata->changed = (_state.changedGP & mask) != 0;
    }
  }

  for (i = 0, mask = 1; i < REG_NUM_MM; i++, mask <<= 1)
  {
    if ((vdata = _state.mm[i]) != NULL)
    {
      vdata->state = VARIABLE_STATE_REGISTER;
      vdata->registerIndex = i;
      vdata->changed = (_state.changedMM & mask) != 0;
    }
  }

  for (i = 0, mask = 1; i < REG_NUM_XMM; i++, mask <<= 1)
  {
    if ((vdata = _state.xmm[i]) != NULL)
    {
      vdata->state = VARIABLE_STATE_REGISTER;
      vdata->registerIndex = i;
      vdata->changed = (_state.changedXMM & mask) != 0;
    }
  }
}

void CompilerContext::_restoreState(StateData* state, uint32_t targetOffset) ASMJIT_NOTHROW
{
  // 16 + 8 + 16 = GP + MMX + XMM registers.
  static const uint STATE_REGS_COUNT = 16 + 8 + 16;

  StateData* fromState = &_state;
  StateData* toState = state;

  // No change, rare...
  if (fromState == toState) return;

  uint base;
  uint i;

  // --------------------------------------------------------------------------
  // Set target state to all variables. vdata->tempInt is target state in this
  // function.
  // --------------------------------------------------------------------------

  {
    // UNUSED.
    VarData* vdata = _active;
    if (vdata)
    {
      do {
        vdata->tempInt = VARIABLE_STATE_UNUSED;
        vdata = vdata->nextActive;
      } while (vdata != _active);
    }

    // MEMORY.
    for (i = 0; i < toState->memVarsCount; i++)
    {
      toState->memVarsData[i]->tempInt = VARIABLE_STATE_MEMORY;
    }

    // REGISTER.
    for (i = 0; i < StateData::NUM_REGS; i++)
    {
      if ((vdata = toState->regs[i]) != NULL) vdata->tempInt = VARIABLE_STATE_REGISTER;
    }
  }

  // --------------------------------------------------------------------------
  // [GP-Registers Switch]
  // --------------------------------------------------------------------------

  // TODO.
#if 0
  for (i = 0; i < REG_NUM_GP; i++)
  {
    VarData* fromVar = fromState->gp[i];
    VarData* toVar = toState->gp[i];

    if (fromVar != toVar)
    {
      if (fromVar != NULL)
      {
        if (toVar != NULL)
        {
          if (fromState->gp[to
        }
        else
        {
          // It is possible that variable that was saved in state currently not
          // exists (tempInt is target scope!).
          if (fromVar->tempInt == VARIABLE_STATE_UNUSED)
          {
            unuseVar(fromVar, VARIABLE_STATE_UNUSED);
          }
          else
          {
            spillVar(fromVar);
          }
        }
      }
    }
    else if (fromVar != NULL)
    {
      uint32_t mask = Util::maskFromIndex(i);
      // Variables are the same, we just need to compare changed flags.
      if ((fromState->changedGP & mask) && !(toState->changedGP & mask)) saveVar(fromVar);
    }
  }
#endif

  // Spill.
  for (base = 0, i = 0; i < STATE_REGS_COUNT; i++)
  {
    // Change the base offset (from base offset the register index can be
    // calculated).
    if (i == 16 || i == 16 + 8) base = i;
    uint32_t regIndex = i - base;

    VarData* fromVar = fromState->regs[i];
    VarData* toVar = toState->regs[i];

    if (fromVar != toVar)
    {

      // Spill the register.
      if (fromVar != NULL)
      {
        // It is possible that variable that was saved in state currently not
        // exists (tempInt is target scope!).
        if (fromVar->tempInt == VARIABLE_STATE_UNUSED)
        {
          unuseVar(fromVar, VARIABLE_STATE_UNUSED);
        }
        else
        {
          spillVar(fromVar);
        }
      }
    }
    else if (fromVar != NULL)
    {
      uint32_t mask = Util::maskFromIndex(regIndex);
      // Variables are the same, we just need to compare changed flags.
      if ((fromState->changedGP & mask) && !(toState->changedGP & mask))
      {
        saveVar(fromVar);
      }
    }
  }

  // Alloc.
  for (base = 0, i = 0; i < STATE_REGS_COUNT; i++)
  {
    if (i == 16 || i == 24) base = i;

    VarData* fromVar = fromState->regs[i];
    VarData* toVar = toState->regs[i];

    if (fromVar != toVar)
    {
      uint32_t regIndex = i - base;

      // Alloc register
      if (toVar != NULL)
      {
        allocVar(toVar, Util::maskFromIndex(regIndex), VARIABLE_ALLOC_READ);
      }
    }

    // TODO:
    //if (toVar)
    //{
      // toVar->changed = to->changed;
    //}
  }

  // --------------------------------------------------------------------------
  // Update used masks.
  // --------------------------------------------------------------------------

  _state.usedGP = state->usedGP;
  _state.usedMM = state->usedMM;
  _state.usedXMM = state->usedXMM;

  // --------------------------------------------------------------------------
  // Update changed masks and cleanup.
  // --------------------------------------------------------------------------

  {
    VarData* vdata = _active;
    if (vdata)
    {
      do {
        if (vdata->tempInt != VARIABLE_STATE_REGISTER)
        {
          vdata->state = (int)vdata->tempInt;
          vdata->changed = false;
        }

        vdata->tempInt = 0;
        vdata = vdata->nextActive;
      } while (vdata != _active);
    }
  }
}

VarMemBlock* CompilerContext::_allocMemBlock(uint32_t size) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(size != 0);

  // First try to find mem blocks.
  VarMemBlock* mem = _memFree;
  VarMemBlock* prev = NULL;

  while (mem)
  {
    VarMemBlock* next = mem->nextFree;

    if (mem->size == size)
    {
      if (prev)
        prev->nextFree = next;
      else
        _memFree = next;

      mem->nextFree = NULL;
      return mem;
    }

    prev = mem;
    mem = next;
  }

  // Never mind, create new.
  mem = reinterpret_cast<VarMemBlock*>(_zone.zalloc(sizeof(VarMemBlock)));
  if (!mem)
  {
    _compiler->setError(ERROR_NO_HEAP_MEMORY);
    return NULL;
  }

  mem->offset = 0;
  mem->size = size;

  mem->nextUsed = _memUsed;
  mem->nextFree = NULL;

  _memUsed = mem;

  switch (size)
  {
    case 16: _mem16BlocksCount++; break;
    case 8: _mem8BlocksCount++; break;
    case 4: _mem4BlocksCount++; break;
  }

  return mem;
}

void CompilerContext::_freeMemBlock(VarMemBlock* mem) ASMJIT_NOTHROW
{
  // Add mem to free blocks.
  mem->nextFree = _memFree;
  _memFree = mem;
}

void CompilerContext::_allocMemoryOperands() ASMJIT_NOTHROW
{
  VarMemBlock* mem;

  // Variables are allocated in this order:
  // 1. 16-byte variables.
  // 2. 8-byte variables.
  // 3. 4-byte variables.
  // 4. All others.

  uint32_t start16 = 0;
  uint32_t start8 = start16 + _mem16BlocksCount * 16;
  uint32_t start4 = start8 + _mem8BlocksCount * 8;
  uint32_t startX = (start4 + _mem4BlocksCount * 4 + 15) & ~15;

  for (mem = _memUsed; mem; mem = mem->nextUsed)
  {
    uint32_t size = mem->size;
    uint32_t offset;

    switch (size)
    {
      case 16:
        offset = start16;
        start16 += 16;
        break;

      case 8:
        offset = start8;
        start8 += 8;
        break;

      case 4:
        offset = start4;
        start4 += 4;
        break;

      default:
        // Align to 16 bytes if size is 16 or more.
        if (size >= 16)
        {
          size = (size + 15) & ~15;
          startX = (startX + 15) & ~15;
        }
        offset = startX;
        startX += size;
        break;
    }

    mem->offset = (int32_t)offset;
    _memBytesTotal += size;
  }
}

void CompilerContext::_patchMemoryOperands(Emittable* start, Emittable* stop) ASMJIT_NOTHROW
{
  Emittable* cur;

  for (cur = start;; cur = cur->getNext())
  {
    if (cur->getType() == EMITTABLE_INSTRUCTION)
    {
      Mem* mem = reinterpret_cast<EInstruction*>(cur)->_memOp;

      if (mem && (mem->_mem.id & OPERAND_ID_TYPE_MASK) == OPERAND_ID_TYPE_VAR)
      {
        VarData* vdata = _compiler->_getVarData(mem->_mem.id);
        ASMJIT_ASSERT(vdata != NULL);

        if (vdata->isMemArgument)
        {
          mem->_mem.base = _argumentsBaseReg;
          mem->_mem.displacement += vdata->homeMemoryOffset;
          mem->_mem.displacement += _argumentsBaseOffset;
        }
        else
        {
          VarMemBlock* mb = reinterpret_cast<VarMemBlock*>(vdata->homeMemoryData);
          ASMJIT_ASSERT(mb != NULL);

          mem->_mem.base = _variablesBaseReg;
          mem->_mem.displacement += mb->offset;
          mem->_mem.displacement += _variablesBaseOffset;
        }
      }
    }
    if (cur == stop) break;
  }
}

// ============================================================================
// [AsmJit::CompilerUtil]
// ============================================================================

bool CompilerUtil::isStack16ByteAligned()
{
  // Stack is always aligned to 16-bytes when using 64-bit OS.
  bool result = (sizeof(sysuint_t) == 8);

  // Modern Linux, APPLE and UNIX guarantees stack alignment to 16 bytes by
  // default. I'm really not sure about all UNIX operating systems, because
  // 16-byte alignment is an addition to the older specification.
#if (defined(__linux__)   || \
     defined(__linux)     || \
     defined(linux)       || \
     defined(__unix__)    || \
     defined(__FreeBSD__) || \
     defined(__NetBSD__)  || \
     defined(__OpenBSD__) || \
     defined(__DARWIN__)  || \
     defined(__APPLE__)   )
  result = true;
#endif // __linux__

  return result;
}

// ============================================================================
// [AsmJit::CompilerCore - Construction / Destruction]
// ============================================================================

CompilerCore::CompilerCore(CodeGenerator* codeGenerator) ASMJIT_NOTHROW :
  _zone(16384 - sizeof(Zone::Chunk) - 32),
  _codeGenerator(codeGenerator != NULL ? codeGenerator : CodeGenerator::getGlobal()),
  _logger(NULL),
  _error(0),
  _properties((1 << PROPERTY_OPTIMIZE_ALIGN)),
  _emitOptions(0),
  _finished(false),
  _first(NULL),
  _last(NULL),
  _current(NULL),
  _function(NULL),
  _varNameId(0),
  _cc(NULL)
{
}

CompilerCore::~CompilerCore() ASMJIT_NOTHROW
{
  free();
}

// ============================================================================
// [AsmJit::CompilerCore - Logging]
// ============================================================================

void CompilerCore::setLogger(Logger* logger) ASMJIT_NOTHROW
{
  _logger = logger;
}

// ============================================================================
// [AsmJit::CompilerCore - Error Handling]
// ============================================================================

void CompilerCore::setError(uint32_t error) ASMJIT_NOTHROW
{
  _error = error;
  if (_error == ERROR_NONE) return;

  if (_logger)
  {
    _logger->logFormat("*** COMPILER ERROR: %s (%u).\n",
      getErrorString(error),
      (unsigned int)error);
  }
}

// ============================================================================
// [AsmJit::CompilerCore - Properties]
// ============================================================================

uint32_t CompilerCore::getProperty(uint32_t propertyId)
{
  return (_properties & (1 << propertyId)) != 0;
}

void CompilerCore::setProperty(uint32_t propertyId, uint32_t value)
{
  if (value)
    _properties |= (1 << propertyId);
  else
    _properties &= ~(1 << propertyId);
}

// ============================================================================
// [AsmJit::CompilerCore - Buffer]
// ============================================================================

void CompilerCore::clear() ASMJIT_NOTHROW
{
  _finished = false;

  delAll(_first);
  _first = NULL;
  _last = NULL;
  _current = NULL;

  _zone.freeAll();
  _targetData.clear();
  _varData.clear();

  _cc = NULL;

  if (_error) setError(ERROR_NONE);
}

void CompilerCore::free() ASMJIT_NOTHROW
{
  clear();

  _targetData.free();
  _varData.free();
}

// ============================================================================
// [AsmJit::CompilerCore - Emittables]
// ============================================================================

void CompilerCore::addEmittable(Emittable* emittable) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(emittable != NULL);
  ASMJIT_ASSERT(emittable->_prev == NULL);
  ASMJIT_ASSERT(emittable->_next == NULL);

  if (_current == NULL)
  {
    if (!_first)
    {
      _first = emittable;
      _last = emittable;
    }
    else
    {
      emittable->_next = _first;
      _first->_prev = emittable;
      _first = emittable;
    }
  }
  else
  {
    Emittable* prev = _current;
    Emittable* next = _current->_next;

    emittable->_prev = prev;
    emittable->_next = next;

    prev->_next = emittable;
    if (next)
      next->_prev = emittable;
    else
      _last = emittable;
  }

  _current = emittable;
}

void CompilerCore::addEmittableAfter(Emittable* emittable, Emittable* ref) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(emittable != NULL);
  ASMJIT_ASSERT(emittable->_prev == NULL);
  ASMJIT_ASSERT(emittable->_next == NULL);
  ASMJIT_ASSERT(ref != NULL);

  Emittable* prev = ref;
  Emittable* next = ref->_next;

  emittable->_prev = prev;
  emittable->_next = next;

  prev->_next = emittable;
  if (next)
    next->_prev = emittable;
  else
    _last = emittable;
}

void CompilerCore::removeEmittable(Emittable* emittable) ASMJIT_NOTHROW
{
  Emittable* prev = emittable->_prev;
  Emittable* next = emittable->_next;

  if (_first == emittable) { _first = next; } else { prev->_next = next; }
  if (_last  == emittable) { _last  = prev; } else { next->_prev = prev; }

  emittable->_prev = NULL;
  emittable->_next = NULL;

  if (emittable == _current) _current = prev;
}

Emittable* CompilerCore::setCurrentEmittable(Emittable* current) ASMJIT_NOTHROW
{
  Emittable* old = _current;
  _current = current;
  return old;
}

// ============================================================================
// [AsmJit::CompilerCore - Logging]
// ============================================================================

void CompilerCore::comment(const char* fmt, ...) ASMJIT_NOTHROW
{
  char buf[128];
  char* p = buf;

  if (fmt)
  {
    *p++ = ';';
    *p++ = ' ';

    va_list ap;
    va_start(ap, fmt);
    p += vsnprintf(p, 100, fmt, ap);
    va_end(ap);
  }

  *p++ = '\n';
  *p   = '\0';

  addEmittable(Compiler_newObject<EComment>(this, buf));
}

// ============================================================================
// [AsmJit::CompilerCore - Function Builder]
// ============================================================================

EFunction* CompilerCore::newFunction_(
  uint32_t callingConvention,
  const uint32_t* arguments,
  uint32_t argumentsCount,
  uint32_t returnValue) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(_function == NULL);
  EFunction* f = _function = Compiler_newObject<EFunction>(this);

  f->setPrototype(callingConvention, arguments, argumentsCount, returnValue);
  addEmittable(f);

  bind(f->_entryLabel);
  addEmittable(f->_prolog);

  _varNameId = 0;

  f->_createVariables();
  return f;
}

EFunction* CompilerCore::endFunction() ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(_function != NULL);
  EFunction* f = _function;

  bind(f->_exitLabel);
  addEmittable(f->_epilog);
  addEmittable(f->_end);

  f->_finished = true;
  _function = NULL;

  return f;
}

// ============================================================================
// [AsmJit::CompilerCore - EmitInstruction]
// ============================================================================

void CompilerCore::_emitInstruction(uint32_t code) ASMJIT_NOTHROW
{
  EInstruction* e = newInstruction(code, NULL, 0);
  if (!e) return;

  addEmittable(e);
  if (_cc) { e->_offset = _cc->_currentOffset; e->prepare(*_cc); }
}

void CompilerCore::_emitInstruction(uint32_t code, const Operand* o0) ASMJIT_NOTHROW
{
  Operand* operands = reinterpret_cast<Operand*>(_zone.zalloc(1 * sizeof(Operand)));
  if (!operands) return;

  operands[0] = *o0;

  EInstruction* e = newInstruction(code, operands, 1);
  if (!e) return;

  addEmittable(e);
  if (_cc) { e->_offset = _cc->_currentOffset; e->prepare(*_cc); }
}

void CompilerCore::_emitInstruction(uint32_t code, const Operand* o0, const Operand* o1) ASMJIT_NOTHROW
{
  Operand* operands = reinterpret_cast<Operand*>(_zone.zalloc(2 * sizeof(Operand)));
  if (!operands) return;

  operands[0] = *o0;
  operands[1] = *o1;

  EInstruction* e = newInstruction(code, operands, 2);
  if (!e) return;

  addEmittable(e);
  if (_cc) { e->_offset = _cc->_currentOffset; e->prepare(*_cc); }
}

void CompilerCore::_emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2) ASMJIT_NOTHROW
{
  Operand* operands = reinterpret_cast<Operand*>(_zone.zalloc(3 * sizeof(Operand)));
  if (!operands) return;

  operands[0] = *o0;
  operands[1] = *o1;
  operands[2] = *o2;

  EInstruction* e = newInstruction(code, operands, 3);
  if (!e) return;

  addEmittable(e);
  if (_cc) { e->_offset = _cc->_currentOffset; e->prepare(*_cc); }
}

void CompilerCore::_emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2, const Operand* o3) ASMJIT_NOTHROW
{
  Operand* operands = reinterpret_cast<Operand*>(_zone.zalloc(4 * sizeof(Operand)));
  if (!operands) return;

  operands[0] = *o0;
  operands[1] = *o1;
  operands[2] = *o2;
  operands[3] = *o3;

  EInstruction* e = newInstruction(code, operands, 4);
  if (!e) return;

  addEmittable(e);
  if (_cc) { e->_offset = _cc->_currentOffset; e->prepare(*_cc); }
}

void CompilerCore::_emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2, const Operand* o3, const Operand* o4) ASMJIT_NOTHROW
{
  Operand* operands = reinterpret_cast<Operand*>(_zone.zalloc(5 * sizeof(Operand)));
  if (!operands) return;

  operands[0] = *o0;
  operands[1] = *o1;
  operands[2] = *o2;
  operands[3] = *o3;
  operands[4] = *o4;

  EInstruction* e = newInstruction(code, operands, 5);
  if (!e) return;

  addEmittable(e);
  if (_cc) { e->_offset = _cc->_currentOffset; e->prepare(*_cc); }
}

void CompilerCore::_emitJcc(uint32_t code, const Label* label, uint32_t hint) ASMJIT_NOTHROW
{
  if (!hint)
  {
    _emitInstruction(code, label);
  }
  else
  {
    Imm imm(hint);
    _emitInstruction(code, label, &imm);
  }
}

ECall* CompilerCore::_emitCall(const Operand* o0) ASMJIT_NOTHROW
{
  EFunction* fn = getFunction();
  if (!fn) { setError(ERROR_NO_FUNCTION); return NULL; }

  ECall* eCall = Compiler_newObject<ECall>(this, fn, o0);
  if (!eCall) { setError(ERROR_NO_HEAP_MEMORY); return NULL; }

  addEmittable(eCall);
  return eCall;
}

void CompilerCore::_emitReturn(const Operand* first, const Operand* second) ASMJIT_NOTHROW
{
  EFunction* fn = getFunction();
  if (!fn) { setError(ERROR_NO_FUNCTION); return; }

  ERet* eRet = Compiler_newObject<ERet>(this, fn, first, second);
  if (!eRet) { setError(ERROR_NO_HEAP_MEMORY); return; }

  addEmittable(eRet);
}

// ============================================================================
// [AsmJit::CompilerCore - Embed]
// ============================================================================

void CompilerCore::embed(const void* data, sysuint_t size) ASMJIT_NOTHROW
{
  // Align length to 16 bytes.
  sysuint_t alignedSize = (size + 15) & ~15;

  EData* e =
    new(_zone.zalloc(sizeof(EData) - sizeof(void*) + alignedSize))
      EData(reinterpret_cast<Compiler*>(this), data, size);
  addEmittable(e);
}

// ============================================================================
// [AsmJit::CompilerCore - Align]
// ============================================================================

void CompilerCore::align(uint32_t m) ASMJIT_NOTHROW
{
  addEmittable(Compiler_newObject<EAlign>(this, m));
}

// ============================================================================
// [AsmJit::CompilerCore - Label]
// ============================================================================

Label CompilerCore::newLabel() ASMJIT_NOTHROW
{
  Label label;
  label._base.id = (uint32_t)_targetData.getLength() | OPERAND_ID_TYPE_LABEL;

  ETarget* target = Compiler_newObject<ETarget>(this, label);
  _targetData.append(target);

  return label;
}

void CompilerCore::bind(const Label& label) ASMJIT_NOTHROW
{
  uint32_t id = label.getId() & OPERAND_ID_VALUE_MASK;
  ASMJIT_ASSERT(id != INVALID_VALUE);
  ASMJIT_ASSERT(id < _targetData.getLength());

  addEmittable(_targetData[id]);
}

// ============================================================================
// [AsmJit::CompilerCore - Variables]
// ============================================================================

VarData* CompilerCore::_newVarData(const char* name, uint32_t type, uint32_t size) ASMJIT_NOTHROW
{
  VarData* vdata = reinterpret_cast<VarData*>(_zone.zalloc(sizeof(VarData)));
  if (vdata == NULL) return NULL;

  char nameBuffer[32];
  if (name == NULL)
  {
    sprintf(nameBuffer, "var_%d", _varNameId);
    name = nameBuffer;
    _varNameId++;
  }

  vdata->scope = getFunction();
  vdata->firstEmittable = NULL;
  vdata->firstCallable = NULL;
  vdata->lastEmittable = NULL;

  vdata->name = _zone.zstrdup(name);
  vdata->id = (uint32_t)_varData.getLength() | OPERAND_ID_TYPE_VAR;
  vdata->type = type;
  vdata->size = size;

  vdata->homeRegisterIndex = INVALID_VALUE;
  vdata->prefRegisterMask = 0;

  vdata->homeMemoryData = NULL;

  vdata->registerIndex = INVALID_VALUE;
  vdata->workOffset = INVALID_VALUE;

  vdata->nextActive = NULL;
  vdata->prevActive = NULL;

  vdata->priority = 10;
  vdata->calculated = false;
  vdata->isRegArgument = false;
  vdata->isMemArgument = false;

  vdata->state = VARIABLE_STATE_UNUSED;
  vdata->changed = false;
  vdata->saveOnUnuse = false;

  vdata->registerReadCount = 0;
  vdata->registerWriteCount = 0;
  vdata->registerRWCount = 0;

  vdata->registerGPBLoCount = 0;
  vdata->registerGPBHiCount = 0;

  vdata->memoryReadCount = 0;
  vdata->memoryWriteCount = 0;
  vdata->memoryRWCount = 0;

  vdata->tempPtr = NULL;

  _varData.append(vdata);
  return vdata;
}

GPVar CompilerCore::newGP(uint32_t variableType, const char* name) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT((variableType < _VARIABLE_TYPE_COUNT) &&
                (variableInfo[variableType].clazz & VariableInfo::CLASS_GP) != 0);

#if defined(ASMJIT_X86)
  if (variableInfo[variableType].size > 4)
  {
    variableType = VARIABLE_TYPE_GPD;
    if (_logger)
    {
      _logger->logString("*** COMPILER WARNING: Translated QWORD variable to DWORD, FIX YOUR CODE! ***\n");
    }
  }
#endif // ASMJIT_X86

  VarData* vdata = _newVarData(name, variableType, variableInfo[variableType].size);
  return GPVarFromData(vdata);
}

GPVar CompilerCore::argGP(uint32_t index) ASMJIT_NOTHROW
{
  GPVar var;
  EFunction* f = getFunction();

  if (f)
  {
    const FunctionPrototype& prototype = f->getPrototype();
    if (index < prototype.getArgumentsCount())
    {
      VarData* vdata = getFunction()->_argumentVariables[index];

      var._var.id = vdata->id;
      var._var.size = vdata->size;
      var._var.registerCode = variableInfo[vdata->type].code;
      var._var.variableType = vdata->type;
    }
  }

  return var;
}

MMVar CompilerCore::newMM(uint32_t variableType, const char* name) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT((variableType < _VARIABLE_TYPE_COUNT) &&
                (variableInfo[variableType].clazz & VariableInfo::CLASS_MM) != 0);

  VarData* vdata = _newVarData(name, variableType, 8);
  return MMVarFromData(vdata);
}

MMVar CompilerCore::argMM(uint32_t index) ASMJIT_NOTHROW
{
  MMVar var;
  EFunction* f = getFunction();

  if (f)
  {
    const FunctionPrototype& prototype = f->getPrototype();
    if (index < prototype.getArgumentsCount())
    {
      VarData* vdata = getFunction()->_argumentVariables[index];

      var._var.id = vdata->id;
      var._var.size = vdata->size;
      var._var.registerCode = variableInfo[vdata->type].code;
      var._var.variableType = vdata->type;
    }
  }

  return var;
}

XMMVar CompilerCore::newXMM(uint32_t variableType, const char* name) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT((variableType < _VARIABLE_TYPE_COUNT) &&
                (variableInfo[variableType].clazz & VariableInfo::CLASS_XMM) != 0);

  VarData* vdata = _newVarData(name, variableType, 16);
  return XMMVarFromData(vdata);
}

XMMVar CompilerCore::argXMM(uint32_t index) ASMJIT_NOTHROW
{
  XMMVar var;
  EFunction* f = getFunction();

  if (f)
  {
    const FunctionPrototype& prototype = f->getPrototype();
    if (index < prototype.getArgumentsCount())
    {
      VarData* vdata = getFunction()->_argumentVariables[index];

      var._var.id = vdata->id;
      var._var.size = vdata->size;
      var._var.registerCode = variableInfo[vdata->type].code;
      var._var.variableType = vdata->type;
    }
  }

  return var;
}

void CompilerCore::_vhint(BaseVar& var, uint32_t hintId, uint32_t hintValue) ASMJIT_NOTHROW
{
  if (var.getId() == INVALID_VALUE)
    return;

  VarData* vdata = _getVarData(var.getId());
  ASMJIT_ASSERT(vdata != NULL);

  EVariableHint* e = Compiler_newObject<EVariableHint>(this, vdata, hintId, hintValue);
  addEmittable(e);
}

void CompilerCore::alloc(BaseVar& var) ASMJIT_NOTHROW
{
  _vhint(var, VARIABLE_HINT_ALLOC, INVALID_VALUE);
}

void CompilerCore::alloc(BaseVar& var, uint32_t regIndex) ASMJIT_NOTHROW
{
  if (regIndex > 31)
    return;

  _vhint(var, VARIABLE_HINT_ALLOC, 1 << regIndex);
}

void CompilerCore::alloc(BaseVar& var, const BaseReg& reg) ASMJIT_NOTHROW
{
  _vhint(var, VARIABLE_HINT_ALLOC, 1 << reg.getRegIndex());
}

void CompilerCore::save(BaseVar& var) ASMJIT_NOTHROW
{
  _vhint(var, VARIABLE_HINT_SAVE, INVALID_VALUE);
}

void CompilerCore::spill(BaseVar& var) ASMJIT_NOTHROW
{
  _vhint(var, VARIABLE_HINT_SPILL, INVALID_VALUE);
}

void CompilerCore::unuse(BaseVar& var) ASMJIT_NOTHROW
{
  _vhint(var, VARIABLE_HINT_UNUSE, INVALID_VALUE);
}

uint32_t CompilerCore::getPriority(BaseVar& var) const ASMJIT_NOTHROW
{
  if (var.getId() == INVALID_VALUE)
    return INVALID_VALUE;

  VarData* vdata = _getVarData(var.getId());
  ASMJIT_ASSERT(vdata != NULL);

  return vdata->priority;
}

void CompilerCore::setPriority(BaseVar& var, uint32_t priority) ASMJIT_NOTHROW
{
  if (var.getId() == INVALID_VALUE)
    return;

  VarData* vdata = _getVarData(var.getId());
  ASMJIT_ASSERT(vdata != NULL);

  if (priority > 100) priority = 100;
  vdata->priority = (uint8_t)priority;
}

bool CompilerCore::getSaveOnUnuse(BaseVar& var) const ASMJIT_NOTHROW
{
  if (var.getId() == INVALID_VALUE)
    return false;

  VarData* vdata = _getVarData(var.getId());
  ASMJIT_ASSERT(vdata != NULL);

  return (bool)vdata->saveOnUnuse;
}

void CompilerCore::setSaveOnUnuse(BaseVar& var, bool value) ASMJIT_NOTHROW
{
  if (var.getId() == INVALID_VALUE)
    return;

  VarData* vdata = _getVarData(var.getId());
  ASMJIT_ASSERT(vdata != NULL);

  vdata->saveOnUnuse = value;
}

void CompilerCore::rename(BaseVar& var, const char* name) ASMJIT_NOTHROW
{
  if (var.getId() == INVALID_VALUE)
    return;

  VarData* vdata = _getVarData(var.getId());
  ASMJIT_ASSERT(vdata != NULL);

  vdata->name = _zone.zstrdup(name);
}

// ============================================================================
// [AsmJit::CompilerCore - State]
// ============================================================================

StateData* CompilerCore::_newStateData(uint32_t memVarsCount) ASMJIT_NOTHROW
{
  StateData* state = reinterpret_cast<StateData*>(_zone.zalloc(sizeof(StateData) + memVarsCount * sizeof(void*)));
  return state;
}

// ============================================================================
// [AsmJit::CompilerCore - Make]
// ============================================================================

void* CompilerCore::make() ASMJIT_NOTHROW
{
  Assembler a(_codeGenerator);

  a._properties = _properties;
  a.setLogger(_logger);

  serialize(a);

  if (this->getError())
  {
    return NULL;
  }

  if (a.getError())
  {
    setError(a.getError());
    return NULL;
  }

  void* result = a.make();

  if (_logger)
  {
    _logger->logFormat("*** COMPILER SUCCESS - Wrote %u bytes, code: %u, trampolines: %u.\n\n",
      (unsigned int)a.getCodeSize(),
      (unsigned int)a.getOffset(),
      (unsigned int)a.getTrampolineSize());
  }

  return result;
}

void CompilerCore::serialize(Assembler& a) ASMJIT_NOTHROW
{
  // Context.
  CompilerContext cc(reinterpret_cast<Compiler*>(this));

  Emittable* start = _first;
  Emittable* stop = NULL;

  // Register all labels.
  a.registerLabels(_targetData.getLength());

  // Make code.
  for (;;)
  {
    _cc = NULL;

    // ------------------------------------------------------------------------
    // Find a function.
    for (;;)
    {
      if (start == NULL) return;
      if (start->getType() == EMITTABLE_FUNCTION)
        break;
      else
        start->emit(a);

      start = start->getNext();
    }
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Setup code generation context.
    Emittable* cur;

    cc._function = reinterpret_cast<EFunction*>(start);
    cc._start = start;
    cc._stop = stop = cc._function->getEnd();
    cc._extraBlock = stop->getPrev();

    // Detect whether the function generation was finished.
    if (!cc._function->_finished || cc._function->getEnd()->getPrev() == NULL)
    {
      setError(ERROR_INCOMPLETE_FUNCTION);
      return;
    }
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Step 1:
    // - Assign/increment offset to each emittable.
    // - Extract variables from instructions.
    // - Prepare variables for register allocator:
    //   - Update read(r) / write(w) / overwrite(x) statistics.
    //   - Update register / memory usage statistics.
    //   - Find scope (first / last emittable) of variables.
    for (cur = start; ; cur = cur->getNext())
    {
      cur->prepare(cc);
      if (cur == stop) break;
    }
    // ------------------------------------------------------------------------

    // We set compiler context also to Compiler so new emitted instructions 
    // can call prepare() to itself.
    _cc = &cc;

    // ------------------------------------------------------------------------
    // Step 2:
    // - Translate special instructions (imul, cmpxchg8b, ...).
    // - Alloc registers.
    // - Translate forward jumps.
    // - Alloc memory operands (variables related).
    // - Emit function prolog.
    // - Emit function epilog.
    // - Patch memory operands (variables related).
    // - Dump function prototype and variable statistics (if enabled).

    // Translate special instructions and run alloc registers.
    cur = start;

    do {
      do {
        // Assign current offset for each emittable back to CompilerContext.
        cc._currentOffset = cur->_offset;
        // Assign previous emittable to compiler so each variable spill/alloc will
        // be emitted before.
        _current = cur->getPrev();

        cur = cur->translate(cc);
      } while (cur);

      cc._unreachable = true;

      sysuint_t len = cc._backCode.getLength();
      while (cc._backPos < len)
      {
        cur = cc._backCode[cc._backPos++]->getNext();
        if (!cur->isTranslated()) break;

        cur = NULL;
      }
    } while (cur);

    // Translate forward jumps.
    {
      ForwardJumpData* j = cc._forwardJumps;
      while (j)
      {
        cc._assignState(j->state);
        _current = j->inst->getPrev();
        j->inst->_doJump(cc);
        j = j->next;
      }
    }

    // Alloc memory operands (variables related).
    cc._allocMemoryOperands();

    // Emit function prolog / epilog.
    cc._function->_preparePrologEpilog(cc);

    _current = cc._function->_prolog;
    cc._function->_emitProlog(cc);

    _current = cc._function->_epilog;
    cc._function->_emitEpilog(cc);

    // Patch memory operands (variables related).
    _current = _last;
    cc._patchMemoryOperands(start, stop);

    // Dump function prototype and variable statistics (if enabled).
    if (_logger)
    {
      cc._function->_dumpFunction(cc);
    }
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Hack: need to register labels that was created by the Step 2.
    if (a._labelData.getLength() < _targetData.getLength())
    {
      a.registerLabels(_targetData.getLength() - a._labelData.getLength());
    }

    Emittable* extraBlock = cc._extraBlock;

    // Step 3:
    // - Emit instructions to Assembler stream.
    for (cur = start; ; cur = cur->getNext())
    {
      cur->emit(a);
      if (cur == extraBlock) break;
    }
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Step 4:
    // - Emit everything else (post action).
    for (cur = start; ; cur = cur->getNext())
    {
      cur->post(a);
      if (cur == extraBlock) break;
    }
    // ------------------------------------------------------------------------

    start = extraBlock->getNext();
    cc._clear();
  }
}

// ============================================================================
// [AsmJit::Compiler - Construction / Destruction]
// ============================================================================

Compiler::Compiler(CodeGenerator* codeGenerator) ASMJIT_NOTHROW : 
  CompilerIntrinsics(codeGenerator)
{
}

Compiler::~Compiler() ASMJIT_NOTHROW
{
}

#pragma warning(default : 4389 4189 4100)

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"
