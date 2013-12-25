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
#include "Compiler.h"
#include "CpuInfo.h"
#include "Logger.h"
#include "Util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

#pragma warning(disable : 4127 4100)


// ============================================================================
// [AsmJit::Emittable]
// ============================================================================

Emittable::Emittable(Compiler* c, uint32_t type) ASMJIT_NOTHROW :
  _compiler(c),
  _next(NULL),
  _prev(NULL),
  _comment(NULL),
  _type((uint8_t)type),
  _translated(false),
  _reserved0(0),
  _reserved1(0),
  _offset(INVALID_VALUE)
{
}

Emittable::~Emittable() ASMJIT_NOTHROW
{
}

void Emittable::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset;
}

Emittable* Emittable::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  return translated();
}

void Emittable::emit(Assembler& a) ASMJIT_NOTHROW
{
}

void Emittable::post(Assembler& a) ASMJIT_NOTHROW
{
}

int Emittable::getMaxSize() const ASMJIT_NOTHROW
{
  // Default maximum size is -1 which means that it's not known.
  return -1;
}

bool Emittable::_tryUnuseVar(VarData* v) ASMJIT_NOTHROW
{
  return false;
}

void Emittable::setComment(const char* str) ASMJIT_NOTHROW
{
  _comment = _compiler->getZone().zstrdup(str);
}

void Emittable::setCommentF(const char* fmt, ...) ASMJIT_NOTHROW
{
  // I'm really not expecting larger inline comments:)
  char buf[256];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, 255, fmt, ap);
  va_end(ap);

  // I don't know if vsnprintf can produce non-null terminated string, in case
  // it can, we terminate it here.
  buf[255] = '\0';

  setComment(buf);
}

// ============================================================================
// [AsmJit::EDummy]
// ============================================================================

EDummy::EDummy(Compiler* c) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_DUMMY)
{
}

EDummy::~EDummy() ASMJIT_NOTHROW
{
}

int EDummy::getMaxSize() const ASMJIT_NOTHROW
{
  return 0;
}

// ============================================================================
// [AsmJit::EFunctionEnd]
// ============================================================================

EFunctionEnd::EFunctionEnd(Compiler* c) ASMJIT_NOTHROW :
  EDummy(c)
{
  _type = EMITTABLE_FUNCTION_END;
}

EFunctionEnd::~EFunctionEnd() ASMJIT_NOTHROW
{
}

Emittable* EFunctionEnd::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  _translated = true;
  return NULL;
}

// ============================================================================
// [AsmJit::EComment]
// ============================================================================

EComment::EComment(Compiler* c, const char* str) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_COMMENT)
{
  setComment(str);
}

EComment::~EComment() ASMJIT_NOTHROW
{
}

void EComment::emit(Assembler& a) ASMJIT_NOTHROW
{
  if (a.getLogger())
  {
    a.getLogger()->logString(getComment());
  }
}

int EComment::getMaxSize() const ASMJIT_NOTHROW
{
  return 0;
}

// ============================================================================
// [AsmJit::EData]
// ============================================================================

EData::EData(Compiler* c, const void* data, sysuint_t length) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_EMBEDDED_DATA)
{
  _length = length;
  memcpy(_data, data, length);
}

EData::~EData() ASMJIT_NOTHROW
{
}

void EData::emit(Assembler& a) ASMJIT_NOTHROW
{
  a.embed(_data, _length);
}

int EData::getMaxSize() const ASMJIT_NOTHROW
{
  return (int)_length;;
}

// ============================================================================
// [AsmJit::EAlign]
// ============================================================================

EAlign::EAlign(Compiler* c, uint32_t size) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_ALIGN), _size(size)
{
}

EAlign::~EAlign() ASMJIT_NOTHROW
{
}

void EAlign::emit(Assembler& a) ASMJIT_NOTHROW
{
  a.align(_size);
}

int EAlign::getMaxSize() const ASMJIT_NOTHROW
{
  return (_size > 0) ? (int)_size - 1 : 0;
}

// ============================================================================
// [AsmJit::ETarget]
// ============================================================================

ETarget::ETarget(Compiler* c, const Label& label) ASMJIT_NOTHROW :
  Emittable(c, EMITTABLE_TARGET),
  _label(label),
  _from(NULL),
  _state(NULL),
  _jumpsCount(0)
{
}

ETarget::~ETarget() ASMJIT_NOTHROW
{
}

void ETarget::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset++;
}

Emittable* ETarget::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  // If this ETarget was already translated, it's needed to change the current
  // state and return NULL to tell CompilerContext to process next untranslated
  // emittable.
  if (_translated)
  {
    cc._restoreState(_state);
    return NULL;
  }

  if (cc._unreachable)
  {
    cc._unreachable = 0;

    // Assign state to the compiler context. 
    ASMJIT_ASSERT(_state != NULL);
    cc._assignState(_state);
  }
  else
  {
    _state = cc._saveState();
  }

  return translated();
}

void ETarget::emit(Assembler& a) ASMJIT_NOTHROW
{
  a.bind(_label);
}

int ETarget::getMaxSize() const ASMJIT_NOTHROW
{
  return 0;
}

#pragma warning(default : 4127 4100)

} // AsmJit namespace
