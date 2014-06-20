// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_ASSEMBLER_H
#define _ASMJIT_BASE_ASSEMBLER_H

// [Dependencies - AsmJit]
#include "../base/codegen.h"
#include "../base/error.h"
#include "../base/logger.h"
#include "../base/operand.h"
#include "../base/podlist.h"
#include "../base/podvector.h"
#include "../base/runtime.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_general
//! \{

// ============================================================================
// [asmjit::kInstCode]
// ============================================================================

//! Instruction codes (stub).
ASMJIT_ENUM(kInstCode) {
  //! No instruction.
  kInstNone = 0
};

// ============================================================================
// [asmjit::kInstOptions]
// ============================================================================

//! Instruction options (stub).
ASMJIT_ENUM(kInstOptions) {
  //! No instruction options.
  kInstOptionNone = 0x00,

  //! Emit short form of the instruction.
  //!
  //! X86/X64:
  //!
  //! Short form is mostly related to jmp and jcc instructions, but can be used
  //! by other instructions supporting 8-bit or 32-bit immediates. This option
  //! can be dangerous if the short jmp/jcc is required, but not encodable due
  //! to large displacement, in such case an error happens and the whole
  //! assembler/compiler stream is unusable.
  kInstOptionShortForm = 0x01,
  //! Emit long form of the instruction.
  //!
  //! X86/X64:
  //!
  //! Long form is mosrlt related to jmp and jcc instructions, but like the
  //! `kInstOptionShortForm` option it can be used by other instructions
  //! supporting both 8-bit and 32-bit immediates.
  kInstOptionLongForm = 0x02,

  //! Condition is likely to be taken.
  kInstOptionTaken = 0x04,
  //! Condition is unlikely to be taken.
  kInstOptionNotTaken = 0x08
};

// ============================================================================
// [asmjit::LabelLink]
// ============================================================================

//! \internal
//!
//! Data structure used to link linked-labels.
struct LabelLink {
  //! Previous link.
  LabelLink* prev;
  //! Offset.
  intptr_t offset;
  //! Inlined displacement.
  intptr_t displacement;
  //! RelocId if link must be absolute when relocated.
  intptr_t relocId;
};

// ============================================================================
// [asmjit::LabelData]
// ============================================================================

//! \internal
//!
//! Label data.
struct LabelData {
  //! Label offset.
  intptr_t offset;
  //! Label links chain.
  LabelLink* links;
};

// ============================================================================
// [asmjit::RelocData]
// ============================================================================

//! \internal
//!
//! Code relocation data (relative vs absolute addresses).
//!
//! X86/X64:
//!
//! X86 architecture uses 32-bit absolute addressing model by memory operands,
//! but 64-bit mode uses relative addressing model (RIP + displacement). In
//! code we are always using relative addressing model for referencing labels
//! and embedded data. In 32-bit mode we must patch all references to absolute
//! address before we can call generated function.
struct RelocData {
  //! Type of relocation.
  uint32_t type;
  //! Size of relocation (4 or 8 bytes).
  uint32_t size;

  //! Offset from code begin address.
  Ptr from;

  //! Relative displacement from code begin address (not to `offset`) or
  //! absolute address.
  Ptr data;
};

// ============================================================================
// [asmjit::BaseAssembler]
// ============================================================================

//! Base assembler.
//!
//! This class implements the base interface to an assembler. The architecture
//! specific API is implemented by backends.
//!
//! @sa BaseCompiler.
struct BaseAssembler : public CodeGen {
  ASMJIT_NO_COPY(BaseAssembler)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `BaseAssembler` instance.
  ASMJIT_API BaseAssembler(Runtime* runtime);
  //! Destroy the `BaseAssembler` instance.
  ASMJIT_API virtual ~BaseAssembler();

  // --------------------------------------------------------------------------
  // [Clear / Reset]
  // --------------------------------------------------------------------------

  //! Clear everything, but not deallocate buffers.
  ASMJIT_API void clear();
  //! Reset everything (means also to free all buffers).
  ASMJIT_API void reset();
  //! Called by clear() and reset() to clear all data related to derived class
  //! implementation.
  ASMJIT_API virtual void _purge();

  // --------------------------------------------------------------------------
  // [Buffer]
  // --------------------------------------------------------------------------

  //! Get capacity of the code buffer.
  ASMJIT_INLINE size_t getCapacity() const {
    return (size_t)(_end - _buffer);
  }

  //! Get the number of remaining bytes (space between cursor and the end of
  //! the buffer).
  ASMJIT_INLINE size_t getRemainingSpace() const {
    return (size_t)(_end - _cursor);
  }

  //! Get buffer.
  ASMJIT_INLINE uint8_t* getBuffer() const {
    return _buffer;
  }

  //! Get the end of the buffer (points to the first byte that is outside).
  ASMJIT_INLINE uint8_t* getEnd() const {
    return _end;
  }

  //! Get the current position in the buffer.
  ASMJIT_INLINE uint8_t* getCursor() const {
    return _cursor;
  }

  //! Set the current position in the buffer.
  ASMJIT_INLINE void setCursor(uint8_t* cursor) {
    ASMJIT_ASSERT(cursor >= _buffer && cursor <= _end);
    _cursor = cursor;
  }

  //! Get the current offset in the buffer.
  ASMJIT_INLINE size_t getOffset() const {
    return (size_t)(_cursor - _buffer);
  }

  //! Set the current offset in the buffer to `offset` and get the previous
  //! offset value.
  ASMJIT_INLINE size_t setOffset(size_t offset) {
    ASMJIT_ASSERT(offset < getCapacity());

    size_t oldOffset = (size_t)(_cursor - _buffer);
    _cursor = _buffer + offset;
    return oldOffset;
  }

  //! Grow the internal buffer.
  //!
  //! The internal buffer will grow at least by `n` bytes so `n` bytes can be
  //! added to it. If `n` is zero or `getOffset() + n` is not greater than the
  //! current capacity of the buffer this function does nothing.
  ASMJIT_API Error _grow(size_t n);

  //! Reserve the internal buffer to at least `n` bytes.
  ASMJIT_API Error _reserve(size_t n);

  //! Get BYTE at position `pos`.
  ASMJIT_INLINE uint8_t getByteAt(size_t pos) const {
    ASMJIT_ASSERT(pos + 1 <= (size_t)(_end - _buffer));
    return *reinterpret_cast<const uint8_t*>(_buffer + pos);
  }

  //! Get WORD at position `pos`.
  ASMJIT_INLINE uint16_t getWordAt(size_t pos) const {
    ASMJIT_ASSERT(pos + 2 <= (size_t)(_end - _buffer));
    return *reinterpret_cast<const uint16_t*>(_buffer + pos);
  }

  //! Get DWORD at position `pos`.
  ASMJIT_INLINE uint32_t getDWordAt(size_t pos) const {
    ASMJIT_ASSERT(pos + 4 <= (size_t)(_end - _buffer));
    return *reinterpret_cast<const uint32_t*>(_buffer + pos);
  }

  //! Get QWORD at position `pos`.
  ASMJIT_INLINE uint64_t getQWordAt(size_t pos) const {
    ASMJIT_ASSERT(pos + 8 <= (size_t)(_end - _buffer));
    return *reinterpret_cast<const uint64_t*>(_buffer + pos);
  }

  //! Get int32_t at position `pos`.
  ASMJIT_INLINE int32_t getInt32At(size_t pos) const {
    ASMJIT_ASSERT(pos + 4 <= (size_t)(_end - _buffer));
    return *reinterpret_cast<const int32_t*>(_buffer + pos);
  }

  //! Get uint32_t at position `pos`.
  ASMJIT_INLINE uint32_t getUInt32At(size_t pos) const {
    ASMJIT_ASSERT(pos + 4 <= (size_t)(_end - _buffer));
    return *reinterpret_cast<const uint32_t*>(_buffer + pos);
  }

  //! Set BYTE at position `pos`.
  ASMJIT_INLINE void setByteAt(size_t pos, uint8_t x) {
    ASMJIT_ASSERT(pos + 1 <= (size_t)(_end - _buffer));
    *reinterpret_cast<uint8_t*>(_buffer + pos) = x;
  }

  //! Set WORD at position `pos`.
  ASMJIT_INLINE void setWordAt(size_t pos, uint16_t x) {
    ASMJIT_ASSERT(pos + 2 <= (size_t)(_end - _buffer));
    *reinterpret_cast<uint16_t*>(_buffer + pos) = x;
  }

  //! Set DWORD at position `pos`.
  ASMJIT_INLINE void setDWordAt(size_t pos, uint32_t x) {
    ASMJIT_ASSERT(pos + 4 <= (size_t)(_end - _buffer));
    *reinterpret_cast<uint32_t*>(_buffer + pos) = x;
  }

  //! Set QWORD at position `pos`.
  ASMJIT_INLINE void setQWordAt(size_t pos, uint64_t x) {
    ASMJIT_ASSERT(pos + 8 <= (size_t)(_end - _buffer));
    *reinterpret_cast<uint64_t*>(_buffer + pos) = x;
  }

  //! Set int32_t at position `pos`.
  ASMJIT_INLINE void setInt32At(size_t pos, int32_t x) {
    ASMJIT_ASSERT(pos + 4 <= (size_t)(_end - _buffer));
    *reinterpret_cast<int32_t*>(_buffer + pos) = x;
  }

  //! Set uint32_t at position `pos`.
  ASMJIT_INLINE void setUInt32At(size_t pos, uint32_t x) {
    ASMJIT_ASSERT(pos + 4 <= (size_t)(_end - _buffer));
    *reinterpret_cast<uint32_t*>(_buffer + pos) = x;
  }

  // --------------------------------------------------------------------------
  // [GetCodeSize]
  // --------------------------------------------------------------------------

  //! Get current offset in buffer, same as `getOffset() + getTramplineSize()`.
  ASMJIT_INLINE size_t getCodeSize() const {
    return getOffset() + getTrampolineSize();
  }

  // --------------------------------------------------------------------------
  // [GetTrampolineSize]
  // --------------------------------------------------------------------------

  //! Get size of all possible trampolines.
  //!
  //! Trampolines are needed to successfuly generate relative jumps to absolute
  //! addresses. This value is only non-zero if jmp of call instructions were
  //! used with immediate operand (this means jumping or calling an absolute
  //! address directly).
  ASMJIT_INLINE size_t getTrampolineSize() const {
    return _trampolineSize;
  }

  // --------------------------------------------------------------------------
  // [Label]
  // --------------------------------------------------------------------------

  //! Get count of labels created.
  ASMJIT_INLINE size_t getLabelsCount() const {
    return _labels.getLength();
  }

  //! Get whether `label` is created.
  ASMJIT_INLINE bool isLabelCreated(const Label& label) const {
    return static_cast<size_t>(label.getId()) < _labels.getLength();
  }

  //! \internal
  //!
  //! Get `LabelData` by `label`.
  ASMJIT_INLINE LabelData* getLabelData(const Label& label) const {
    return getLabelDataById(label.getId());
  }

  //! \internal
  //!
  //! Get `LabelData` by `id`.
  ASMJIT_INLINE LabelData* getLabelDataById(uint32_t id) const {
    ASMJIT_ASSERT(id != kInvalidValue);
    ASMJIT_ASSERT(id < _labels.getLength());

    return const_cast<LabelData*>(&_labels[id]);
  }

  //! \internal
  //!
  //! Register labels for other code generator, i.e. `Compiler`.
  ASMJIT_API Error _registerIndexedLabels(size_t index);

  //! \internal
  //!
  //! Create and initialize a new `Label`.
  ASMJIT_API Error _newLabel(Label* dst);

  //! \internal
  //!
  //! New LabelLink instance.
  ASMJIT_API LabelLink* _newLabelLink();

  //! Create and return a new `Label`.
  ASMJIT_INLINE Label newLabel() {
    Label result(NoInit);
    _newLabel(&result);
    return result;
  }

  //! Bind label to the current offset.
  virtual void _bind(const Label& label) = 0;

  //! Bind label to the current offset.
  //!
  //! \note Label can be bound only once!
  ASMJIT_INLINE void bind(const Label& label) {
    _bind(label);
  }

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! Embed data into the code buffer.
  ASMJIT_API Error embed(const void* data, uint32_t size);

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  //! Align target buffer to `m` bytes.
  //!
  //! Typical usage of this is to align labels at start of the inner loops.
  //!
  //! Inserts `nop()` instructions or CPU optimized NOPs.
  ASMJIT_INLINE Error align(uint32_t mode, uint32_t offset) {
    return _align(mode, offset);
  }

  //! \internal
  //!
  //! Align target buffer to `m` bytes.
  virtual Error _align(uint32_t mode, uint32_t offset) = 0;

  // --------------------------------------------------------------------------
  // [Reloc]
  // --------------------------------------------------------------------------

  //! Simplifed version of `relocCode()` method designed for JIT.
  //!
  //! \overload
  ASMJIT_INLINE size_t relocCode(void* dst) const {
    return _relocCode(dst, static_cast<Ptr>((uintptr_t)dst));
  }

  //! Relocate code to a given address `dst`.
  //!
  //! \param dst Refers the location where the relocated code should be copied.
  //! The pointer can be address returned by virtual memory allocator or any
  //! custom address.
  //!
  //! \param base Base address used for relocation. `JitRuntime` always sets
  //! `base` address to be the same as `dst`, but other runtimes do not have
  //! to follow this rule.
  //!
  //! \retval The number bytes used. If the code generator reserved space for
  //! possible trampolines, but these weren't generated, the number of bytes
  //! used can be actually less than the expected worst case. Virtual memory
  //! allocator can in such case return some memory back to the pool.
  //!
  //! A given buffer will be overwritten, to get number of bytes required use
  //! `getCodeSize()`.
  ASMJIT_INLINE size_t relocCode(void* dst, Ptr base) const {
    return _relocCode(dst, base);
  }

  //! \internal
  //!
  //! Reloc code.
  virtual size_t _relocCode(void* dst, Ptr base) const = 0;

  // --------------------------------------------------------------------------
  // [Make]
  // --------------------------------------------------------------------------

  ASMJIT_API void* make();

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  //! Emit an instruction.
  ASMJIT_API Error emit(uint32_t code);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, const Operand& o1);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2);
  //! \overload
  ASMJIT_INLINE Error emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3) {
    return _emit(code, o0, o1, o2, o3);
  }

  //! Emit an instruction with integer immediate operand.
  ASMJIT_API Error emit(uint32_t code, int o0);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, uint64_t o0);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, int o1);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, uint64_t o1);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, const Operand& o1, int o2);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, const Operand& o1, uint64_t o2);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, int o3);
  //! \overload
  ASMJIT_API Error emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, uint64_t o3);

  //! Emit an instruction (virtual).
  virtual Error _emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3) = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Buffer where the code is emitted (either live or temporary).
  //!
  //! This is actually the base pointer of the buffer, to get the current
  //! position (cursor) look at the `_cursor` member.
  uint8_t* _buffer;
  //! The end of the buffer (points to the first invalid byte).
  //!
  //! The end of the buffer is calculated as <code>_buffer + size</code>.
  uint8_t* _end;
  //! The current position in code `_buffer`.
  uint8_t* _cursor;

  //! Size of possible trampolines.
  uint32_t _trampolineSize;

  //! Inline comment that will be logged by the next instruction and
  //! set to NULL.
  const char* _comment;
  //! Unused `LabelLink` structures pool.
  LabelLink* _unusedLinks;

  //! Labels data.
  PodVector<LabelData> _labels;
  //! Relocations data.
  PodVector<RelocData> _relocData;
};

//! \}

// ============================================================================
// [Defined-Later]
// ============================================================================

ASMJIT_INLINE Label::Label(BaseAssembler& a) : Operand(NoInit) {
  a._newLabel(this);
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_ASSEMBLER_H
