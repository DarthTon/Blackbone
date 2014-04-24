// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies]
#include "Build.h"
#include "Util_p.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::Util]
// ============================================================================

static const char letters[] = "0123456789ABCDEF";

char* Util::mycpy(char* dst, const char* src, sysuint_t len) ASMJIT_NOTHROW
{
  if (src == NULL) return dst;

  if (len == (sysuint_t)-1)
  {
    while (*src) *dst++ = *src++;
  }
  else
  {
    memcpy(dst, src, len);
    dst += len;
  }

  return dst;
}

char* Util::myfill(char* dst, const int c, sysuint_t len) ASMJIT_NOTHROW
{
  memset(dst, c, len);
  return dst + len;
}

char* Util::myhex(char* dst, const uint8_t* src, sysuint_t len) ASMJIT_NOTHROW
{
  for (sysuint_t i = len; i; i--, dst += 2, src += 1)
  {
    dst[0] = letters[(src[0] >> 4) & 0xF];
    dst[1] = letters[(src[0]     ) & 0xF];
  }

  return dst;
}

// Not too efficient, but this is mainly for debugging:)
char* Util::myutoa(char* dst, sysuint_t i, sysuint_t base) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(base <= 16);

  char buf[128];
  char* p = buf + 128;

  do {
    sysint_t b = i % base;
    *--p = letters[b];
    i /= base;
  } while (i);

  return Util::mycpy(dst, p, (sysuint_t)(buf + 128 - p));
}

char* Util::myitoa(char* dst, sysint_t i, sysuint_t base) ASMJIT_NOTHROW
{
  if (i < 0)
  {
    *dst++ = '-';
    i = -i;
  }

  return Util::myutoa(dst, (sysuint_t)i, base);
}

// ============================================================================
// [AsmJit::Buffer]
// ============================================================================

void Buffer::emitData(const void* dataPtr, sysuint_t dataLen) ASMJIT_NOTHROW
{
  sysint_t max = getCapacity() - getOffset();
  if ((sysuint_t)max < dataLen)
  {
    if (!realloc(getOffset() + dataLen)) return;
  }

  memcpy(_cur, dataPtr, dataLen);
  _cur += dataLen;
}

bool Buffer::realloc(sysint_t to) ASMJIT_NOTHROW
{
  if (getCapacity() < to)
  {
    sysint_t len = getOffset();

    uint8_t *newdata;
    if (_data)
      newdata = (uint8_t*)ASMJIT_REALLOC(_data, to);
    else
      newdata = (uint8_t*)ASMJIT_MALLOC(to);
    if (!newdata) return false;

    _data = newdata;
    _cur = newdata + len;
    _max = newdata + to;
    _max -= (to >= _growThreshold) ? _growThreshold : to;

    _capacity = to;
  }

  return true;
}

bool Buffer::grow() ASMJIT_NOTHROW
{
  sysint_t to = _capacity;

  if (to < 512)
    to = 1024;
  else if (to > 65536)
    to += 65536;
  else
    to <<= 1;

  return realloc(to);
}

void Buffer::clear() ASMJIT_NOTHROW
{
  _cur = _data;
}

void Buffer::free() ASMJIT_NOTHROW
{
  if (!_data) return;
  ASMJIT_FREE(_data);

  _data = NULL;
  _cur = NULL;
  _max = NULL;
  _capacity = 0;
}

uint8_t* Buffer::take() ASMJIT_NOTHROW
{
  uint8_t* data = _data;

  _data = NULL;
  _cur = NULL;
  _max = NULL;
  _capacity = 0;

  return data;
}

// ============================================================================
// [AsmJit::Zone]
// ============================================================================

Zone::Zone(sysuint_t chunkSize) ASMJIT_NOTHROW
{
  _chunks = NULL;
  _total = 0;
  _chunkSize = chunkSize;
}

Zone::~Zone() ASMJIT_NOTHROW
{
  freeAll();
}

void* Zone::zalloc(sysuint_t size) ASMJIT_NOTHROW
{
  // Align to 4 or 8 bytes.
  size = (size + sizeof(sysint_t)-1) & ~(sizeof(sysint_t)-1);

  Chunk* cur = _chunks;

  if (!cur || cur->getRemainingBytes() < size)
  {
    sysuint_t chSize = _chunkSize;
    if (chSize < size) chSize = size;

    cur = (Chunk*)ASMJIT_MALLOC(sizeof(Chunk) - sizeof(void*) + chSize);
    if (!cur) return NULL;

    cur->prev = _chunks;
    cur->pos = 0;
    cur->size = _chunkSize;
    _chunks = cur;
  }

  uint8_t* p = cur->data + cur->pos;
  cur->pos += size;
  _total += size;
  return (void*)p;
}

char* Zone::zstrdup(const char* str) ASMJIT_NOTHROW
{
  if (str == NULL) return NULL;

  sysuint_t len = strlen(str);
  if (len == 0) return NULL;

  // Include NULL terminator.
  len++;

  // Limit string length.
  if (len > 256) len = 256;

  char* m = reinterpret_cast<char*>(zalloc((len + 15) & ~15));
  if (!m) return NULL;

  memcpy(m, str, len);
  m[len-1] = '\0';
  return m;
}

void Zone::clear() ASMJIT_NOTHROW
{
  Chunk* cur = _chunks;
  if (!cur) return;

  _chunks->pos = 0;
  _chunks->prev = NULL;
  _total = 0;

  cur = cur->prev;
  while (cur)
  {
    Chunk* prev = cur->prev;
    ASMJIT_FREE(cur);
    cur = prev;
  }
}

void Zone::freeAll() ASMJIT_NOTHROW
{
  Chunk* cur = _chunks;

  _chunks = NULL;
  _total = 0;

  while (cur)
  {
    Chunk* prev = cur->prev;
    ASMJIT_FREE(cur);
    cur = prev;
  }
}

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"
