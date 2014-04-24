// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_ASSEMBLERX86X64_H
#define _ASMJIT_ASSEMBLERX86X64_H

#if !defined(_ASMJIT_ASSEMBLER_H)
#warning "AsmJit/AssemblerX86X64.h can be only included by AsmJit/Assembler.h"
#endif // _ASMJIT_ASSEMBLER_H

// [Dependencies]
#include "Build.h"
#include "Defs.h"
#include "Operand.h"
#include "Util.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

#pragma warning(disable : 4701 4389 4245 4244 4100 4189)

//! @addtogroup AsmJit_Core
//! @{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct CodeGenerator;

// ============================================================================
// [AsmJit::AssemblerCore]
// ============================================================================

//! @brief AssemblerCore is part of @c Assembler class.
//!
//! @c AssemblerCore class implements part of assembler serializing API. The
//! reason why @c Assembler class was split is that we want to hide exported
//! symbols in dynamically linked libraries.
//!
//! Please always use @c Assembler class instead.
//!
//! @sa @c Assembler.
struct ASMJIT_API AssemblerCore
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Creates Assembler instance.
  AssemblerCore(CodeGenerator* codeGenerator) ASMJIT_NOTHROW;
  //! @brief Destroys Assembler instance
  virtual ~AssemblerCore() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [LabelLink]
  // --------------------------------------------------------------------------

  //! @brief Data structure used to link linked-labels.
  struct LabelLink
  {
    //! @brief Previous link.
    LabelLink* prev;
    //! @brief Offset.
    sysint_t offset;
    //! @brief Inlined displacement.
    sysint_t displacement;
    //! @brief RelocId if link must be absolute when relocated.
    sysint_t relocId;
  };

  // --------------------------------------------------------------------------
  // [LabelData]
  // --------------------------------------------------------------------------

  //! @brief Label data.
  struct LabelData
  {
    //! @brief Label offset.
    sysint_t offset;
    //! @brief Label links chain.
    LabelLink* links;
  };

  // --------------------------------------------------------------------------
  // [RelocData]
  // --------------------------------------------------------------------------

  // X86 architecture uses 32-bit absolute addressing model by memory operands,
  // but 64-bit mode uses relative addressing model (RIP + displacement). In
  // code we are always using relative addressing model for referencing labels
  // and embedded data. In 32-bit mode we must patch all references to absolute
  // address before we can call generated function. We are patching only memory
  // operands.

  //! @brief Code relocation data (relative vs absolute addresses).
  struct RelocData
  {
    enum Type
    {
      ABSOLUTE_TO_ABSOLUTE = 0,
      RELATIVE_TO_ABSOLUTE = 1,
      ABSOLUTE_TO_RELATIVE = 2,
      ABSOLUTE_TO_RELATIVE_TRAMPOLINE = 3
    };

    //! @brief Type of relocation.
    uint32_t type;

    //! @brief Size of relocation (4 or 8 bytes).
    uint32_t size;

    //! @brief Offset from code begin address.
    sysint_t offset;

    //! @brief Relative displacement or absolute address.
    union
    {
      //! @brief Relative displacement from code begin address (not to @c offset).
      sysint_t destination;
      //! @brief Absolute address where to jump;
      void* address;
    };
  };

  // --------------------------------------------------------------------------
  // [Code Generator]
  // --------------------------------------------------------------------------

  //! @brief Get code generator.
  inline CodeGenerator* getCodeGenerator() const { return _codeGenerator; }

  // --------------------------------------------------------------------------
  // [Memory Management]
  // --------------------------------------------------------------------------

  //! @brief Get zone memory manager.
  inline Zone& getZone() ASMJIT_NOTHROW { return _zone; }

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

  //! @brief Get assembler property.
  uint32_t getProperty(uint32_t propertyId);
  //! @brief Set assembler property.
  void setProperty(uint32_t propertyId, uint32_t value);

  // --------------------------------------------------------------------------
  // [Buffer Getters / Setters]
  // --------------------------------------------------------------------------

  //! @brief Return start of assembler code buffer.
  //!
  //! Note that buffer address can change if you emit instruction or something
  //! else. Use this pointer only when you finished or make sure you do not
  //! use returned pointer after emitting.
  inline uint8_t* getCode() const ASMJIT_NOTHROW
  { return _buffer.getData(); }

  //! @brief Ensure space for next instruction.
  //!
  //! Note that this method can return false. It's rare and probably you never
  //! get this, but in some situations it's still possible.
  inline bool ensureSpace() ASMJIT_NOTHROW
  { return _buffer.ensureSpace(); }

  //! @brief Return current offset in buffer).
  inline sysint_t getOffset() const ASMJIT_NOTHROW
  { return _buffer.getOffset(); }

  //! @brief Return current offset in buffer (same as getCffset() + getTramplineSize()).
  inline sysint_t getCodeSize() const ASMJIT_NOTHROW
  { return _buffer.getOffset() + getTrampolineSize(); }

  //! @brief Get size of all possible trampolines needed to successfuly generate
  //! relative jumps to absolute addresses. This value is only non-zero if jmp
  //! of call instructions were used with immediate operand (this means jump or
  //! call absolute address directly).
  //!
  //! Currently only _emitJmpOrCallReloc() method can increase trampoline size
  //! value.
  inline sysint_t getTrampolineSize() const ASMJIT_NOTHROW
  { return _trampolineSize; }

  //! @brief Set offset to @a o and returns previous offset.
  //!
  //! This method can be used to truncate code (previous offset is not
  //! recorded) or to overwrite instruction stream at position @a o.
  //!
  //! @return Previous offset value that can be uset to set offset back later.
  inline sysint_t toOffset(sysint_t o) ASMJIT_NOTHROW
  { return _buffer.toOffset(o); }

  //! @brief Get capacity of internal code buffer.
  inline sysint_t getCapacity() const ASMJIT_NOTHROW
  { return _buffer.getCapacity(); }

  //! @brief Clear everything, but not deallocate buffers.
  void clear() ASMJIT_NOTHROW;

  //! @brief Free internal buffer and NULL all pointers.
  void free() ASMJIT_NOTHROW;

  //! @brief Take internal code buffer and NULL all pointers (you take the ownership).
  uint8_t* takeCode() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Stream Setters / Getters]
  // --------------------------------------------------------------------------

  //! @brief Set byte at position @a pos.
  inline uint8_t getByteAt(sysint_t pos) const ASMJIT_NOTHROW
  { return _buffer.getByteAt(pos); }
  
  //! @brief Set word at position @a pos.
  inline uint16_t getWordAt(sysint_t pos) const ASMJIT_NOTHROW
  { return _buffer.getWordAt(pos); }
  
  //! @brief Set word at position @a pos.
  inline uint32_t getDWordAt(sysint_t pos) const ASMJIT_NOTHROW
  { return _buffer.getDWordAt(pos); }
  
  //! @brief Set word at position @a pos.
  inline uint64_t getQWordAt(sysint_t pos) const ASMJIT_NOTHROW
  { return _buffer.getQWordAt(pos); }

  //! @brief Set byte at position @a pos.
  inline void setByteAt(sysint_t pos, uint8_t x) ASMJIT_NOTHROW
  { _buffer.setByteAt(pos, x); }
  
  //! @brief Set word at position @a pos.
  inline void setWordAt(sysint_t pos, uint16_t x) ASMJIT_NOTHROW
  { _buffer.setWordAt(pos, x); }
  
  //! @brief Set word at position @a pos.
  inline void setDWordAt(sysint_t pos, uint32_t x) ASMJIT_NOTHROW
  { _buffer.setDWordAt(pos, x); }
  
  //! @brief Set word at position @a pos.
  inline void setQWordAt(sysint_t pos, uint64_t x) ASMJIT_NOTHROW
  { _buffer.setQWordAt(pos, x); }

  //! @brief Set word at position @a pos.
  inline int32_t getInt32At(sysint_t pos) const ASMJIT_NOTHROW
  { return (int32_t)_buffer.getDWordAt(pos); }
  
  //! @brief Set int32 at position @a pos.
  inline void setInt32At(sysint_t pos, int32_t x) ASMJIT_NOTHROW
  { _buffer.setDWordAt(pos, (int32_t)x); }

  //! @brief Set custom variable @a imm at position @a pos.
  //!
  //! @note This function is used to patch existing code.
  void setVarAt(sysint_t pos, sysint_t i, uint8_t isUnsigned, uint32_t size) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Assembler Emitters]
  //
  // These emitters are not protecting buffer from overrun, this must be 
  // done is emitX86() methods by:
  //   if (!canEmit()) return;
  // --------------------------------------------------------------------------

  //! @brief Get whether next instruction can be emitted.
  //!
  //! This function behaves like @c ensureSpace(), but it also checks if
  //! assembler is in error state and in that case it returns @c false.
  //! Assembler internally always uses this function before new instruction is
  //! emitted.
  //!
  //! It's implemented like:
  //!   <code>return ensureSpace() && !getError();</code>
  bool canEmit() ASMJIT_NOTHROW;

  //! @brief Emit Byte to internal buffer.
  inline void _emitByte(uint8_t x) ASMJIT_NOTHROW
  { _buffer.emitByte(x); }

  //! @brief Emit Word (2 bytes) to internal buffer.
  inline void _emitWord(uint16_t x) ASMJIT_NOTHROW
  { _buffer.emitWord(x); }

  //! @brief Emit DWord (4 bytes) to internal buffer.
  inline void _emitDWord(uint32_t x) ASMJIT_NOTHROW
  { _buffer.emitDWord(x); }

  //! @brief Emit QWord (8 bytes) to internal buffer.
  inline void _emitQWord(uint64_t x) ASMJIT_NOTHROW
  { _buffer.emitQWord(x); }

  //! @brief Emit Int32 (4 bytes) to internal buffer.
  inline void _emitInt32(int32_t x) ASMJIT_NOTHROW
  { _buffer.emitDWord((uint32_t)x); }

  //! @brief Emit system signed integer (4 or 8 bytes) to internal buffer.
  inline void _emitSysInt(sysint_t x) ASMJIT_NOTHROW
  { _buffer.emitSysInt(x); }

  //! @brief Emit system unsigned integer (4 or 8 bytes) to internal buffer.
  inline void _emitSysUInt(sysuint_t x) ASMJIT_NOTHROW
  { _buffer.emitSysUInt(x); }

  //! @brief Emit single @a opCode without operands.
  inline void _emitOpCode(uint32_t opCode) ASMJIT_NOTHROW
  {
    // instruction prefix
    if (opCode & 0xFF000000) _emitByte((uint8_t)((opCode & 0xFF000000) >> 24));
    // instruction opcodes
    if (opCode & 0x00FF0000) _emitByte((uint8_t)((opCode & 0x00FF0000) >> 16));
    if (opCode & 0x0000FF00) _emitByte((uint8_t)((opCode & 0x0000FF00) >>  8));
    // last opcode is always emitted (can be also 0x00)
    _emitByte((uint8_t)(opCode & 0x000000FF));
  }

  //! @brief Emit CS (code segmend) prefix.
  //!
  //! Behavior of this function is to emit code prefix only if memory operand
  //! address uses code segment. Code segment is used through memory operand
  //! with attached @c AsmJit::Label.
  void _emitSegmentPrefix(const Operand& rm) ASMJIT_NOTHROW;

  //! @brief Emit MODR/M byte.
  inline void _emitMod(uint8_t m, uint8_t o, uint8_t r) ASMJIT_NOTHROW
  { _emitByte(((m & 0x03) << 6) | ((o & 0x07) << 3) | (r & 0x07)); }

  //! @brief Emit SIB byte.
  inline void _emitSib(uint8_t s, uint8_t i, uint8_t b) ASMJIT_NOTHROW
  { _emitByte(((s & 0x03) << 6) | ((i & 0x07) << 3) | (b & 0x07)); }

  //! @brief Emit REX prefix (64-bit mode only).
  inline void _emitRexR(uint8_t w, uint8_t opReg, uint8_t regCode, bool forceRexPrefix) ASMJIT_NOTHROW
  {
#if defined(ASMJIT_X64)
    uint8_t r = (opReg & 0x8) != 0;
    uint8_t b = (regCode & 0x8) != 0;

    // w Default operand size(0=Default, 1=64-bit).
    // r Register field (1=high bit extension of the ModR/M REG field).
    // x Index field not used in RexR
    // b Base field (1=high bit extension of the ModR/M or SIB Base field).
    if (w || r || b || forceRexPrefix)
    {
      _emitByte(0x40 | (w << 3) | (r << 2) | b);
    }
#else
    ASMJIT_UNUSED(w);
    ASMJIT_UNUSED(opReg);
    ASMJIT_UNUSED(regCode);
    ASMJIT_UNUSED(forceRexPrefix);
#endif // ASMJIT_X64
  }

  //! @brief Emit REX prefix (64-bit mode only).
  inline void _emitRexRM(uint8_t w, uint8_t opReg, const Operand& rm, bool forceRexPrefix) ASMJIT_NOTHROW
  {
#if defined(ASMJIT_X64)
    uint8_t r = (opReg & 0x8) != 0;
    uint8_t x = 0;
    uint8_t b = 0;

    if (rm.isReg())
    {
      b = (reinterpret_cast<const BaseReg&>(rm).getRegCode() & 0x8) != 0;
    }
    else if (rm.isMem())
    {
      x = ((reinterpret_cast<const Mem&>(rm).getIndex() & 0x8) != 0) & (reinterpret_cast<const Mem&>(rm).getIndex() != INVALID_VALUE);
      b = ((reinterpret_cast<const Mem&>(rm).getBase() & 0x8) != 0) & (reinterpret_cast<const Mem&>(rm).getBase() != INVALID_VALUE);
    }

    // w Default operand size(0=Default, 1=64-bit).
    // r Register field (1=high bit extension of the ModR/M REG field).
    // x Index field (1=high bit extension of the SIB Index field).
    // b Base field (1=high bit extension of the ModR/M or SIB Base field).
    if (w || r || x || b || forceRexPrefix)
    {
      _emitByte(0x40 | (w << 3) | (r << 2) | (x << 1) | b);
    }
#else
    ASMJIT_UNUSED(w);
    ASMJIT_UNUSED(opReg);
    ASMJIT_UNUSED(rm);
#endif // ASMJIT_X64
  }

  //! @brief Emit Register / Register - calls _emitMod(3, opReg, r)
  inline void _emitModR(uint8_t opReg, uint8_t r) ASMJIT_NOTHROW
  { _emitMod(3, opReg, r); }

  //! @brief Emit Register / Register - calls _emitMod(3, opReg, r.code())
  inline void _emitModR(uint8_t opReg, const BaseReg& r) ASMJIT_NOTHROW
  { _emitMod(3, opReg, r.getRegCode()); }

  //! @brief Emit register / memory address combination to buffer.
  //!
  //! This method can hangle addresses from simple to complex ones with
  //! index and displacement.
  void _emitModM(uint8_t opReg, const Mem& mem, sysint_t immSize) ASMJIT_NOTHROW;

  //! @brief Emit Reg<-Reg or Reg<-Reg|Mem ModRM (can be followed by SIB 
  //! and displacement) to buffer.
  //!
  //! This function internally calls @c _emitModM() or _emitModR() that depends
  //! to @a op type.
  //!
  //! @note @a opReg is usually real register ID (see @c R) but some instructions
  //! have specific format and in that cases @a opReg is part of opcode.
  void _emitModRM(uint8_t opReg, const Operand& op, sysint_t immSize) ASMJIT_NOTHROW;

  //! @brief Emit instruction where register is inlined to opcode.
  void _emitX86Inl(uint32_t opCode, uint8_t i16bit, uint8_t rexw, uint8_t reg, bool forceRexPrefix) ASMJIT_NOTHROW;

  //! @brief Emit instruction with reg/memory operand.
  void _emitX86RM(uint32_t opCode, uint8_t i16bit, uint8_t rexw, uint8_t o,
    const Operand& op, sysint_t immSize, bool forceRexPrefix) ASMJIT_NOTHROW;

  //! @brief Emit FPU instruction with no operands.
  void _emitFpu(uint32_t opCode) ASMJIT_NOTHROW;

  //! @brief Emit FPU instruction with one operand @a sti (index of FPU register).
  void _emitFpuSTI(uint32_t opCode, uint32_t sti) ASMJIT_NOTHROW;

  //! @brief Emit FPU instruction with one operand @a opReg and memory operand @a mem.
  void _emitFpuMEM(uint32_t opCode, uint8_t opReg, const Mem& mem) ASMJIT_NOTHROW;

  //! @brief Emit MMX/SSE instruction.
  void _emitMmu(uint32_t opCode, uint8_t rexw, uint8_t opReg, const Operand& src,
    sysint_t immSize) ASMJIT_NOTHROW;

  //! @brief Emit displacement.
  LabelLink* _emitDisplacement(LabelData& l_data, sysint_t inlinedDisplacement, int size) ASMJIT_NOTHROW;

  //! @brief Emit relative relocation to absolute pointer @a target. It's needed
  //! to add what instruction is emitting this, because in x64 mode the relative
  //! displacement can be impossible to calculate and in this case the trampoline
  //! is used.
  void _emitJmpOrCallReloc(uint32_t instruction, void* target) ASMJIT_NOTHROW;

  // Helpers to decrease binary code size. These four emit methods are just
  // helpers thats used by assembler. They call emitX86() adding NULLs
  // to first, second and third operand, if needed.

  //! @brief Emit X86/FPU or MM/XMM instruction.
  void _emitInstruction(uint32_t code) ASMJIT_NOTHROW;

  //! @brief Emit X86/FPU or MM/XMM instruction.
  void _emitInstruction(uint32_t code, const Operand* o0) ASMJIT_NOTHROW;

  //! @brief Emit X86/FPU or MM/XMM instruction.
  void _emitInstruction(uint32_t code, const Operand* o0, const Operand* o1) ASMJIT_NOTHROW;

  //! @brief Emit X86/FPU or MM/XMM instruction.
  //!
  //! Operands @a o1, @a o2 or @a o3 can be @c NULL if they are not used.
  //!
  //! Hint: Use @c emitX86() helpers to emit instructions.
  void _emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2) ASMJIT_NOTHROW;

  //! @brief Private method for emitting jcc.
  void _emitJcc(uint32_t code, const Label* label, uint32_t hint) ASMJIT_NOTHROW;

  //! @brief Private method for emitting short jcc.
  inline void _emitShortJcc(uint32_t code, const Label* label, uint32_t hint)
  {
    _emitOptions |= EMIT_OPTION_SHORT_JUMP;
    _emitJcc(code, label, hint);
  }

  // --------------------------------------------------------------------------
  // [Relocation helpers]
  // --------------------------------------------------------------------------

  //! @brief Relocate code to a given address @a dst.
  //!
  //! @param dst Where the relocated code should me stored. The pointer can be
  //! address returned by virtual memory allocator or your own address if you
  //! want only to store the code for later reuse (or load, etc...).
  //! @param addressBase Base address used for relocation. When using JIT code
  //! generation, this will be the same as @a dst, only casted to system
  //! integer type. But when generating code for remote process then the value
  //! can be different.
  //!
  //! @retval The bytes used. Code-generator can create trampolines which are
  //! used when calling other functions inside the JIT code. However, these
  //! trampolines can be unused so the relocCode() returns the exact size needed
  //! for the function.
  //!
  //! A given buffer will be overwritten, to get number of bytes required use
  //! @c getCodeSize() .
  virtual sysuint_t relocCode(void* dst, sysuint_t addressBase) const ASMJIT_NOTHROW;

  //! @brief Simplifed version of @c relocCode() method.
  inline sysuint_t relocCode(void* dst) const ASMJIT_NOTHROW
  {
    return relocCode(dst, (sysuint_t)dst);
  }

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! @brief Embed data into instruction stream.
  void embed(const void* data, sysuint_t length) ASMJIT_NOTHROW;
  //! @brief Embed absolute label pointer (4 or 8 bytes).
  void embedLabel(const Label& label) ASMJIT_NOTHROW;

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

  //! @brief Register labels (used by @c Compiler).
  void registerLabels(sysuint_t count) ASMJIT_NOTHROW;

  //! @brief Bind label to the current offset.
  //!
  //! @note Label can be bound only once!
  void bind(const Label& label) ASMJIT_NOTHROW;

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

  // --------------------------------------------------------------------------
  // [Links]
  // --------------------------------------------------------------------------

  LabelLink* _newLabelLink() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

protected:
  //! @brief Zone memory management.
  Zone _zone;

  //! @brief Code generator instance.
  CodeGenerator* _codeGenerator;

  //! @brief Logger.
  Logger* _logger;

  //! @brief Last error code.
  uint32_t _error;

  //! @brief Properties.
  uint32_t _properties;

  //! @brief Emit flags for next instruction (cleared after emit).
  uint32_t _emitOptions;

  //! @brief Binary code buffer.
  Buffer _buffer;

  //! @brief Size of possible trampolines.
  sysint_t _trampolineSize;

  //! @brief Linked list of unused links (@c LabelLink* structures)
  LabelLink* _unusedLinks;

  //! @brief Labels data.
  PodVector<LabelData> _labelData;

  //! @brief Relocations data.
  PodVector<RelocData> _relocData;

  //! @brief Comment that will be logger by next emitted instruction. After
  //! instruction is logger the _comment is set to NULL.
  const char* _comment;

  friend struct CompilerCore;
  friend struct EInstruction;
};

// ============================================================================
// [AsmJit::AssemblerIntrinsics]
// ============================================================================

//! @brief AssemblerIntrinsics is part of @c Assembler class.
//!
//! @c AssemblerIntrinsics class implements part of assembler serializing API.
//! The reason why @c Assembler class was split is that we want to hide exported
//! symbols in dynamically linked libraries.
//!
//! Please always use @c Assembler class instead.
//!
//! @sa @c Assembler.
struct ASMJIT_HIDDEN AssemblerIntrinsics : public AssemblerCore
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  inline AssemblerIntrinsics(CodeGenerator* codeGenerator) ASMJIT_NOTHROW :
    AssemblerCore(codeGenerator)
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
  // [X86 Instructions]
  // --------------------------------------------------------------------------

  //! @brief Add with Carry.
  inline void adc(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }
  //! @brief Add with Carry.
  inline void adc(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ADC, &dst, &src);
  }

  //! @brief Add.
  inline void add(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }
  //! @brief Add.
  inline void add(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ADD, &dst, &src);
  }

  //! @brief Logical And.
  inline void and_(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }
  //! @brief Logical And.
  inline void and_(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_AND, &dst, &src);
  }

  //! @brief Bit Scan Forward.
  inline void bsf(const GPReg& dst, const GPReg& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSF, &dst, &src);
  }
  //! @brief Bit Scan Forward.
  inline void bsf(const GPReg& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSF, &dst, &src);
  }

  //! @brief Bit Scan Reverse.
  inline void bsr(const GPReg& dst, const GPReg& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSR, &dst, &src);
  }
  //! @brief Bit Scan Reverse.
  inline void bsr(const GPReg& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_BSR, &dst, &src);
  }

  //! @brief Byte swap (32-bit or 64-bit registers only) (i486).
  inline void bswap(const GPReg& dst)
  {
    ASMJIT_ASSERT(dst.getRegType() == REG_TYPE_GPD || dst.getRegType() == REG_TYPE_GPQ);
    _emitInstruction(INST_BSWAP, &dst);
  }

  //! @brief Bit test.
  inline void bt(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }
  //! @brief Bit test.
  inline void bt(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }
  //! @brief Bit test.
  inline void bt(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }
  //! @brief Bit test.
  inline void bt(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BT, &dst, &src);
  }

  //! @brief Bit test and complement.
  inline void btc(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }
  //! @brief Bit test and complement.
  inline void btc(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }
  //! @brief Bit test and complement.
  inline void btc(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }
  //! @brief Bit test and complement.
  inline void btc(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BTC, &dst, &src);
  }

  //! @brief Bit test and reset.
  inline void btr(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }
  //! @brief Bit test and reset.
  inline void btr(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }
  //! @brief Bit test and reset.
  inline void btr(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }
  //! @brief Bit test and reset.
  inline void btr(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BTR, &dst, &src);
  }

  //! @brief Bit test and set.
  inline void bts(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }
  //! @brief Bit test and set.
  inline void bts(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }
  //! @brief Bit test and set.
  inline void bts(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }
  //! @brief Bit test and set.
  inline void bts(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_BTS, &dst, &src);
  }

  //! @brief Call Procedure.
  inline void call(const GPReg& dst)
  {
    ASMJIT_ASSERT(dst.isRegType(REG_TYPE_GPN));
    _emitInstruction(INST_CALL, &dst);
  }
  //! @brief Call Procedure.
  inline void call(const Mem& dst)
  {
    _emitInstruction(INST_CALL, &dst);
  }
  //! @brief Call Procedure.
  inline void call(const Imm& dst)
  {
    _emitInstruction(INST_CALL, &dst);
  }
  //! @brief Call Procedure.
  //! @overload
  inline void call(void* dst)
  {
    Imm imm((sysint_t)dst);
    _emitInstruction(INST_CALL, &imm);
  }

  //! @brief Call Procedure.
  inline void call(const Label& label)
  {
    _emitInstruction(INST_CALL, &label);
  }

  //! @brief Convert Byte to Word (Sign Extend).
  //!
  //! AX <- Sign Extend AL
  inline void cbw()
  {
    _emitInstruction(INST_CBW);
  }

  //! @brief Convert Word to DWord (Sign Extend).
  //!
  //! EAX <- Sign Extend AX
  inline void cwde()
  {
    _emitInstruction(INST_CWDE);
  }

#if defined(ASMJIT_X64)
  //! @brief Convert DWord to QWord (Sign Extend).
  //!
  //! RAX <- Sign Extend EAX
  inline void cdqe()
  {
    _emitInstruction(INST_CDQE);
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
  inline void cmov(CONDITION cc, const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(ConditionToInstruction::toCMovCC(cc), &dst, &src);
  }

  //! @brief Conditional Move.
  inline void cmov(CONDITION cc, const GPReg& dst, const Mem& src)
  {
    _emitInstruction(ConditionToInstruction::toCMovCC(cc), &dst, &src);
  }

  //! @brief Conditional Move.
  inline void cmova  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVA  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmova  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVA  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovae (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVAE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovae (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVAE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovb  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVB  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovb  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVB  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovbe (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVBE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovbe (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVBE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovc  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVC  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovc  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVC  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmove  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVE  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmove  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVE  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovg  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVG  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovg  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVG  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovge (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVGE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovge (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVGE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovl  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVL  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovl  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVL  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovle (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVLE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovle (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVLE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovna (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNA , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovna (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNA , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnae(const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNAE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnae(const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNAE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnb (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNB , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnb (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNB , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnbe(const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNBE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnbe(const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNBE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnc (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNC , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnc (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNC , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovne (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovne (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovng (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNG , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovng (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNG , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnge(const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNGE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnge(const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNGE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnl (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNL , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnl (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNL , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnle(const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNLE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnle(const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNLE, &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovno (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovno (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnp (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNP , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnp (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNP , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovns (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNS , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovns (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNS , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnz (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVNZ , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovnz (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVNZ , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovo  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVO  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovo  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVO  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovp  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVP  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovp  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVP  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpe (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVPE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpe (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVPE , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpo (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVPO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovpo (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVPO , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovs  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVS  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovs  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVS  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovz  (const GPReg& dst, const GPReg& src) { _emitInstruction(INST_CMOVZ  , &dst, &src); }
  //! @brief Conditional Move.
  inline void cmovz  (const GPReg& dst, const Mem& src)   { _emitInstruction(INST_CMOVZ  , &dst, &src); }

  //! @brief Compare Two Operands.
  inline void cmp(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }
  //! @brief Compare Two Operands.
  inline void cmp(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_CMP, &dst, &src);
  }

  //! @brief Compare and Exchange (i486).
  inline void cmpxchg(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_CMPXCHG, &dst, &src);
  }
  //! @brief Compare and Exchange (i486).
  inline void cmpxchg(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_CMPXCHG, &dst, &src);
  }

  //! @brief Compares the 64-bit value in EDX:EAX with the memory operand (Pentium).
  //!
  //! If the values are equal, then this instruction stores the 64-bit value
  //! in ECX:EBX into the memory operand and sets the zero flag. Otherwise,
  //! this instruction copies the 64-bit memory operand into the EDX:EAX
  //! registers and clears the zero flag.
  inline void cmpxchg8b(const Mem& dst)
  {
    _emitInstruction(INST_CMPXCHG8B, &dst);
  }

#if defined(ASMJIT_X64)
  //! @brief Compares the 128-bit value in RDX:RAX with the memory operand (X64).
  //!
  //! If the values are equal, then this instruction stores the 128-bit value
  //! in RCX:RBX into the memory operand and sets the zero flag. Otherwise,
  //! this instruction copies the 128-bit memory operand into the RDX:RAX
  //! registers and clears the zero flag.
  inline void cmpxchg16b(const Mem& dst)
  {
    _emitInstruction(INST_CMPXCHG16B, &dst);
  }
#endif // ASMJIT_X64

  //! @brief CPU Identification (i486).
  inline void cpuid()
  {
    _emitInstruction(INST_CPUID);
  }

#if defined(ASMJIT_X86)
  //! @brief Decimal adjust AL after addition
  //!
  //! This instruction adjusts the sum of two packed BCD values to create
  //! a packed BCD result.
  //!
  //! @note This instruction is only available in 32-bit mode.
  inline void daa()
  {
    _emitInstruction(INST_DAA);
  }
#endif // ASMJIT_X86

#if defined(ASMJIT_X86)
  //! @brief Decimal adjust AL after subtraction
  //!
  //! This instruction adjusts the result of the subtraction of two packed
  //! BCD values to create a packed BCD result.
  //!
  //! @note This instruction is only available in 32-bit mode.
  inline void das()
  {
    _emitInstruction(INST_DAS);
  }
#endif // ASMJIT_X86

  //! @brief Decrement by 1.
  //! @note This instruction can be slower than sub(dst, 1)
  inline void dec(const GPReg& dst)
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
  inline void div(const GPReg& src)
  {
    _emitInstruction(INST_DIV, &src);
  }
  //! @brief Unsigned divide.
  //! @overload
  inline void div(const Mem& src)
  {
    _emitInstruction(INST_DIV, &src);
  }

  //! @brief Make Stack Frame for Procedure Parameters.
  inline void enter(const Imm& imm16, const Imm& imm8)
  {
    _emitInstruction(INST_ENTER, &imm16, &imm8);
  }

  //! @brief Signed divide.
  //!
  //! This instruction divides (signed) the value in the AL, AX, or EAX
  //! register by the source operand and stores the result in the AX,
  //! DX:AX, or EDX:EAX registers.
  inline void idiv(const GPReg& src)
  {
    _emitInstruction(INST_IDIV, &src);
  }
  //! @brief Signed divide.
  //! @overload
  inline void idiv(const Mem& src)
  {
    _emitInstruction(INST_IDIV, &src);
  }

  //! @brief Signed multiply.
  //!
  //! Source operand (in a general-purpose register or memory location)
  //! is multiplied by the value in the AL, AX, or EAX register (depending
  //! on the operand size) and the product is stored in the AX, DX:AX, or
  //! EDX:EAX registers, respectively.
  inline void imul(const GPReg& src)
  {
    _emitInstruction(INST_IMUL, &src);
  }
  //! @overload
  inline void imul(const Mem& src)
  {
    _emitInstruction(INST_IMUL, &src);
  }

  //! @brief Signed multiply.
  //!
  //! Destination operand (the first operand) is multiplied by the source
  //! operand (second operand). The destination operand is a general-purpose
  //! register and the source operand is an immediate value, a general-purpose
  //! register, or a memory location. The product is then stored in the
  //! destination operand location.
  inline void imul(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_IMUL, &dst, &src);
  }
  //! @brief Signed multiply.
  //! @overload
  inline void imul(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_IMUL, &dst, &src);
  }
  //! @brief Signed multiply.
  //! @overload
  inline void imul(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_IMUL, &dst, &src);
  }

  //! @brief Signed multiply.
  //!
  //! source operand (which can be a general-purpose register or a memory
  //! location) is multiplied by the second source operand (an immediate
  //! value). The product is then stored in the destination operand
  //! (a general-purpose register).
  inline void imul(const GPReg& dst, const GPReg& src, const Imm& imm)
  {
    _emitInstruction(INST_IMUL, &dst, &src, &imm);
  }
  //! @overload
  inline void imul(const GPReg& dst, const Mem& src, const Imm& imm)
  {
    _emitInstruction(INST_IMUL, &dst, &src, &imm);
  }

  //! @brief Increment by 1.
  //! @note This instruction can be slower than add(dst, 1)
  inline void inc(const GPReg& dst)
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

  //! @brief Short jump to label @a label if condition @a cc is met.
  //! @sa j()
  inline void short_j(CONDITION cc, const Label& label, uint32_t hint = HINT_NONE)
  {
    _emitOptions |= EMIT_OPTION_SHORT_JUMP;
    j(cc, label, hint);
  }

  //! @brief Short jump to label @a label if condition is met.
  inline void short_ja  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JA  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jae (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JAE , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jb  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JB  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jbe (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JBE , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jc  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JC  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_je  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JE  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jg  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JG  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jge (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JGE , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jl  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JL  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jle (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JLE , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jna (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNA , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnae(const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNAE, &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnb (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNB , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnbe(const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNBE, &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnc (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNC , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jne (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNE , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jng (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNG , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnge(const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNGE, &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnl (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNL , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnle(const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNLE, &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jno (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNO , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnp (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNP , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jns (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNS , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jnz (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JNZ , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jo  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JO  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jp  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JP  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jpe (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JPE , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jpo (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JPO , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_js  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JS  , &label, hint); }
  //! @brief Short jump to label @a label if condition is met.
  inline void short_jz  (const Label& label, uint32_t hint = HINT_NONE) { _emitShortJcc(INST_JZ  , &label, hint); }

  //! @brief Jump.
  //! @overload
  inline void jmp(const GPReg& dst)
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

  //! @brief Short jump.
  //! @sa jmp()
  inline void short_jmp(const Label& label)
  {
    _emitOptions |= EMIT_OPTION_SHORT_JUMP;
    _emitInstruction(INST_JMP, &label);
  }

  //! @brief Load Effective Address
  //!
  //! This instruction computes the effective address of the second
  //! operand (the source operand) and stores it in the first operand
  //! (destination operand). The source operand is a memory address
  //! (offset part) specified with one of the processors addressing modes.
  //! The destination operand is a general-purpose register.
  inline void lea(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_LEA, &dst, &src);
  }

  //! @brief High Level Procedure Exit.
  inline void leave()
  {
    _emitInstruction(INST_LEAVE);
  }

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
  inline void mov(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_MOV, &dst, &src);
  }
  //! @brief Move.
  //! @overload
  inline void mov(const Mem& dst, const GPReg& src)
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
  inline void mov(const GPReg& dst, const SegmentReg& src)
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
  inline void mov(const SegmentReg& dst, const GPReg& src)
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
  inline void mov_ptr(const GPReg& dst, void* src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0);
    Imm imm((sysint_t)src);
    _emitInstruction(INST_MOV_PTR, &dst, &imm);
  }

  //! @brief Move byte, word, dword or qword from AL, AX, EAX or RAX register
  //! to absolute address @a dst.
  inline void mov_ptr(void* dst, const GPReg& src)
  {
    ASMJIT_ASSERT(src.getRegIndex() == 0);
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
  void movsx(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVSX, &dst, &src);
  }
  //! @brief Move with Sign-Extension.
  //! @overload
  void movsx(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSX, &dst, &src);
  }

#if defined(ASMJIT_X64)
  //! @brief Move DWord to QWord with sign-extension.
  inline void movsxd(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVSXD, &dst, &src);
  }
  //! @brief Move DWord to QWord with sign-extension.
  //! @overload
  inline void movsxd(const GPReg& dst, const Mem& src)
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
  inline void movzx(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVZX, &dst, &src);
  }
  //! @brief Move with Zero-Extend.
  //! @brief Overload
  inline void movzx(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVZX, &dst, &src);
  }

  //! @brief Unsigned multiply.
  //!
  //! Source operand (in a general-purpose register or memory location)
  //! is multiplied by the value in the AL, AX, or EAX register (depending
  //! on the operand size) and the product is stored in the AX, DX:AX, or
  //! EDX:EAX registers, respectively.
  inline void mul(const GPReg& src)
  {
    _emitInstruction(INST_MUL, &src);
  }
  //! @brief Unsigned multiply.
  //! @overload
  inline void mul(const Mem& src)
  {
    _emitInstruction(INST_MUL, &src);
  }

  //! @brief Two's Complement Negation.
  inline void neg(const GPReg& dst)
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
  inline void not_(const GPReg& dst)
  {
    _emitInstruction(INST_NOT, &dst);
  }
  //! @brief One's Complement Negation.
  inline void not_(const Mem& dst)
  {
    _emitInstruction(INST_NOT, &dst);
  }

  //! @brief Logical Inclusive OR.
  inline void or_(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_OR, &dst, &src);
  }
  //! @brief Logical Inclusive OR.
  inline void or_(const Mem& dst, const GPReg& src)
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
  inline void pop(const GPReg& dst)
  {
    ASMJIT_ASSERT(dst.isRegType(REG_TYPE_GPW) || dst.isRegType(REG_TYPE_GPN));
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
  inline void push(const GPReg& src)
  {
    ASMJIT_ASSERT(src.isRegType(REG_TYPE_GPW) || src.isRegType(REG_TYPE_GPN));
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
  inline void rcl(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_RCL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  inline void rcl(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_RCL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rcl(const Mem& dst, const GPReg& src)
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
  inline void rcr(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void rcr(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  //! @note @a src register can be only @c cl.
  inline void rcr(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void rcr(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_RCR, &dst, &src);
  }

  //! @brief Read Time-Stamp Counter (Pentium).
  inline void rdtsc()
  {
    _emitInstruction(INST_RDTSC);
  }

  //! @brief Read Time-Stamp Counter and Processor ID (New).
  inline void rdtscp()
  {
    _emitInstruction(INST_RDTSCP);
  }

  //! @brief Load ECX/RCX BYTEs from DS:[ESI/RSI] to AL.
  inline void rep_lodsb()
  {
    _emitInstruction(INST_REP_LODSB);
  }

  //! @brief Load ECX/RCX DWORDs from DS:[ESI/RSI] to EAX.
  inline void rep_lodsd()
  {
    _emitInstruction(INST_REP_LODSD);
  }

#if defined(ASMJIT_X64)
  //! @brief Load ECX/RCX QWORDs from DS:[ESI/RSI] to RAX.
  inline void rep_lodsq()
  {
    _emitInstruction(INST_REP_LODSQ);
  }
#endif // ASMJIT_X64

  //! @brief Load ECX/RCX WORDs from DS:[ESI/RSI] to AX.
  inline void rep_lodsw()
  {
    _emitInstruction(INST_REP_LODSW);
  }

  //! @brief Move ECX/RCX BYTEs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsb()
  {
    _emitInstruction(INST_REP_MOVSB);
  }

  //! @brief Move ECX/RCX DWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsd()
  {
    _emitInstruction(INST_REP_MOVSD);
  }

#if defined(ASMJIT_X64)
  //! @brief Move ECX/RCX QWORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsq()
  {
    _emitInstruction(INST_REP_MOVSQ);
  }
#endif // ASMJIT_X64

  //! @brief Move ECX/RCX WORDs from DS:[ESI/RSI] to ES:[EDI/RDI].
  inline void rep_movsw()
  {
    _emitInstruction(INST_REP_MOVSW);
  }

  //! @brief Fill ECX/RCX BYTEs at ES:[EDI/RDI] with AL.
  inline void rep_stosb()
  {
    _emitInstruction(INST_REP_STOSB);
  }

  //! @brief Fill ECX/RCX DWORDs at ES:[EDI/RDI] with EAX.
  inline void rep_stosd()
  {
    _emitInstruction(INST_REP_STOSD);
  }

#if defined(ASMJIT_X64)
  //! @brief Fill ECX/RCX QWORDs at ES:[EDI/RDI] with RAX.
  inline void rep_stosq()
  {
    _emitInstruction(INST_REP_STOSQ);
  }
#endif // ASMJIT_X64

  //! @brief Fill ECX/RCX WORDs at ES:[EDI/RDI] with AX.
  inline void rep_stosw()
  {
    _emitInstruction(INST_REP_STOSW);
  }

  //! @brief Repeated find nonmatching BYTEs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsb()
  {
    _emitInstruction(INST_REPE_CMPSB);
  }
  
  //! @brief Repeated find nonmatching DWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsd()
  {
    _emitInstruction(INST_REPE_CMPSD);
  }

#if defined(ASMJIT_X64)
  //! @brief Repeated find nonmatching QWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsq()
  {
    _emitInstruction(INST_REPE_CMPSQ);
  }
#endif // ASMJIT_X64
  
  //! @brief Repeated find nonmatching WORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repe_cmpsw()
  {
    _emitInstruction(INST_REPE_CMPSW);
  }

  //! @brief Find non-AL BYTE starting at ES:[EDI/RDI].
  inline void repe_scasb()
  {
    _emitInstruction(INST_REPE_SCASB);
  }
  
  //! @brief Find non-EAX DWORD starting at ES:[EDI/RDI].
  inline void repe_scasd()
  {
    _emitInstruction(INST_REPE_SCASD);
  }

#if defined(ASMJIT_X64)
  //! @brief Find non-RAX QWORD starting at ES:[EDI/RDI].
  inline void repe_scasq()
  {
    _emitInstruction(INST_REPE_SCASQ);
  }
#endif // ASMJIT_X64

  //! @brief Find non-AX WORD starting at ES:[EDI/RDI].
  inline void repe_scasw()
  {
    _emitInstruction(INST_REPE_SCASW);
  }

  //! @brief Repeated find nonmatching BYTEs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repne_cmpsb()
  {
    _emitInstruction(INST_REPNE_CMPSB);
  }

  //! @brief Repeated find nonmatching DWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repne_cmpsd()
  {
    _emitInstruction(INST_REPNE_CMPSD);
  }

#if defined(ASMJIT_X64)
  //! @brief Repeated find nonmatching QWORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repne_cmpsq()
  {
    _emitInstruction(INST_REPNE_CMPSQ);
  }
#endif // ASMJIT_X64

  //! @brief Repeated find nonmatching WORDs in ES:[EDI/RDI] and DS:[ESI/RDI].
  inline void repne_cmpsw()
  {
    _emitInstruction(INST_REPNE_CMPSW);
  }

  //! @brief Find AL, starting at ES:[EDI/RDI].
  inline void repne_scasb()
  {
    _emitInstruction(INST_REPNE_SCASB);
  }

  //! @brief Find EAX, starting at ES:[EDI/RDI].
  inline void repne_scasd()
  {
    _emitInstruction(INST_REPNE_SCASD);
  }

#if defined(ASMJIT_X64)
  //! @brief Find RAX, starting at ES:[EDI/RDI].
  inline void repne_scasq()
  {
    _emitInstruction(INST_REPNE_SCASQ);
  }
#endif // ASMJIT_X64

  //! @brief Find AX, starting at ES:[EDI/RDI].
  inline void repne_scasw()
  {
    _emitInstruction(INST_REPNE_SCASW);
  }

  //! @brief Return from Procedure.
  inline void ret()
  {
    _emitInstruction(INST_RET);
  }

  //! @brief Return from Procedure.
  inline void ret(const Imm& imm16)
  {
    _emitInstruction(INST_RET, &imm16);
  }

  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rol(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_ROL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  inline void rol(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_ROL, &dst, &src);
  }
  //! @brief Rotate Bits Left.
  //! @note @a src register can be only @c cl.
  inline void rol(const Mem& dst, const GPReg& src)
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
  inline void ror(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void ror(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  //! @note @a src register can be only @c cl.
  inline void ror(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }
  //! @brief Rotate Bits Right.
  inline void ror(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_ROR, &dst, &src);
  }

#if defined(ASMJIT_X86)
  //! @brief Store AH into Flags.
  inline void sahf()
  {
    _emitInstruction(INST_SAHF);
  }
#endif // ASMJIT_X86

  //! @brief Integer subtraction with borrow.
  inline void sbb(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_SBB, &dst, &src);
  }
  //! @brief Integer subtraction with borrow.
  inline void sbb(const Mem& dst, const GPReg& src)
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
  inline void sal(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_SAL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  inline void sal(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_SAL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void sal(const Mem& dst, const GPReg& src)
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
  inline void sar(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void sar(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  //! @note @a src register can be only @c cl.
  inline void sar(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void sar(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SAR, &dst, &src);
  }

  //! @brief Set Byte on Condition.
  inline void set(CONDITION cc, const GPReg& dst)
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
  inline void seta  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETA  , &dst); }
  //! @brief Set Byte on Condition.
  inline void seta  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETA  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setae (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETAE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setae (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETAE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setb  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETB  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setb  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETB  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setbe (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETBE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setbe (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETBE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setc  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETC  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setc  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETC  , &dst); }
  //! @brief Set Byte on Condition.
  inline void sete  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETE  , &dst); }
  //! @brief Set Byte on Condition.
  inline void sete  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETE  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setg  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETG  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setg  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETG  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setge (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETGE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setge (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETGE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setl  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETL  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setl  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETL  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setle (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETLE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setle (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETLE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setna (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNA , &dst); }
  //! @brief Set Byte on Condition.
  inline void setna (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNA , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnae(const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNAE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnae(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNAE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnb (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNB , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnb (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNB , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnbe(const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNBE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnbe(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNBE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnc (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNC , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnc (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNC , &dst); }
  //! @brief Set Byte on Condition.
  inline void setne (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setne (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setng (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNG , &dst); }
  //! @brief Set Byte on Condition.
  inline void setng (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNG , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnge(const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNGE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnge(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNGE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnl (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNL , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnl (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNL , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnle(const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNLE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setnle(const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNLE, &dst); }
  //! @brief Set Byte on Condition.
  inline void setno (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNO , &dst); }
  //! @brief Set Byte on Condition.
  inline void setno (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNO , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnp (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNP , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnp (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNP , &dst); }
  //! @brief Set Byte on Condition.
  inline void setns (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNS , &dst); }
  //! @brief Set Byte on Condition.
  inline void setns (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNS , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnz (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETNZ , &dst); }
  //! @brief Set Byte on Condition.
  inline void setnz (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETNZ , &dst); }
  //! @brief Set Byte on Condition.
  inline void seto  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETO  , &dst); }
  //! @brief Set Byte on Condition.
  inline void seto  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETO  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setp  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETP  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setp  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETP  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpe (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETPE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpe (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETPE , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpo (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETPO , &dst); }
  //! @brief Set Byte on Condition.
  inline void setpo (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETPO , &dst); }
  //! @brief Set Byte on Condition.
  inline void sets  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETS  , &dst); }
  //! @brief Set Byte on Condition.
  inline void sets  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETS  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setz  (const GPReg& dst) { ASMJIT_ASSERT(dst.getSize() == 1); _emitInstruction(INST_SETZ  , &dst); }
  //! @brief Set Byte on Condition.
  inline void setz  (const Mem& dst)   { ASMJIT_ASSERT(dst.getSize() <= 1); _emitInstruction(INST_SETZ  , &dst); }

  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void shl(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_SHL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  inline void shl(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_SHL, &dst, &src);
  }
  //! @brief Shift Bits Left.
  //! @note @a src register can be only @c cl.
  inline void shl(const Mem& dst, const GPReg& src)
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
  inline void shr(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_SHR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  inline void shr(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_SHR, &dst, &src);
  }
  //! @brief Shift Bits Right.
  //! @note @a src register can be only @c cl.
  inline void shr(const Mem& dst, const GPReg& src)
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
  inline void shld(const GPReg& dst, const GPReg& src1, const GPReg& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Left.
  inline void shld(const GPReg& dst, const GPReg& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Left.
  //! @note src2 register can be only @c cl register.
  inline void shld(const Mem& dst, const GPReg& src1, const GPReg& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Left.
  inline void shld(const Mem& dst, const GPReg& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHLD, &dst, &src1, &src2);
  }

  //! @brief Double Precision Shift Right.
  //! @note src2 register can be only @c cl register.
  inline void shrd(const GPReg& dst, const GPReg& src1, const GPReg& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Right.
  inline void shrd(const GPReg& dst, const GPReg& src1, const Imm& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Right.
  //! @note src2 register can be only @c cl register.
  inline void shrd(const Mem& dst, const GPReg& src1, const GPReg& src2)
  {
    _emitInstruction(INST_SHRD, &dst, &src1, &src2);
  }
  //! @brief Double Precision Shift Right.
  inline void shrd(const Mem& dst, const GPReg& src1, const Imm& src2)
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
  inline void sub(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }
  //! @brief Subtract.
  inline void sub(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_SUB, &dst, &src);
  }

  //! @brief Logical Compare.
  inline void test(const GPReg& op1, const GPReg& op2)
  {
    _emitInstruction(INST_TEST, &op1, &op2);
  }
  //! @brief Logical Compare.
  inline void test(const GPReg& op1, const Imm& op2)
  {
    _emitInstruction(INST_TEST, &op1, &op2);
  }
  //! @brief Logical Compare.
  inline void test(const Mem& op1, const GPReg& op2)
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
  inline void xadd(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_XADD, &dst, &src);
  }
  //! @brief Exchange and Add.
  inline void xadd(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_XADD, &dst, &src);
  }

  //! @brief Exchange Register/Memory with Register.
  inline void xchg(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_XCHG, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xchg(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_XCHG, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xchg(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_XCHG, &src, &dst);
  }

  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const GPReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const GPReg& dst, const Imm& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }
  //! @brief Exchange Register/Memory with Register.
  inline void xor_(const Mem& dst, const Imm& src)
  {
    _emitInstruction(INST_XOR, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [X87 Instructions (FPU)]
  // --------------------------------------------------------------------------

  //! @brief Compute 2^x - 1 (FPU).
  inline void f2xm1()
  {
    _emitInstruction(INST_F2XM1);
  }

  //! @brief Absolute Value of st(0) (FPU).
  inline void fabs()
  {
    _emitInstruction(INST_FABS);
  }

  //! @brief Add @a src to @a dst and store result in @a dst (FPU).
  //!
  //! @note One of dst or src must be st(0).
  inline void fadd(const X87Reg& dst, const X87Reg& src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0 || src.getRegIndex() == 0);
    _emitInstruction(INST_FADD, &dst, &src);
  }

  //! @brief Add @a src to st(0) and store result in st(0) (FPU).
  //!
  //! @note SP-FP or DP-FP determined by @a adr size.
  inline void fadd(const Mem& src)
  {
    _emitInstruction(INST_FADD, &src);
  }

  //! @brief Add st(0) to @a dst and POP register stack (FPU).
  inline void faddp(const X87Reg& dst = st(1))
  {
    _emitInstruction(INST_FADDP, &dst);
  }

  //! @brief Load Binary Coded Decimal (FPU).
  inline void fbld(const Mem& src)
  {
    _emitInstruction(INST_FBLD, &src);
  }

  //! @brief Store BCD Integer and Pop (FPU).
  inline void fbstp(const Mem& dst)
  {
    _emitInstruction(INST_FBSTP, &dst);
  }

  //! @brief Change st(0) Sign (FPU).
  inline void fchs()
  {
    _emitInstruction(INST_FCHS);
  }

  //! @brief Clear Exceptions (FPU).
  //!
  //! Clear floating-point exception flags after checking for pending unmasked
  //! floating-point exceptions.
  //!
  //! Clears the floating-point exception flags (PE, UE, OE, ZE, DE, and IE),
  //! the exception summary status flag (ES), the stack fault flag (SF), and
  //! the busy flag (B) in the FPU status word. The FCLEX instruction checks
  //! for and handles any pending unmasked floating-point exceptions before
  //! clearing the exception flags.
  inline void fclex()
  {
    _emitInstruction(INST_FCLEX);
  }

  //! @brief FP Conditional Move (FPU).
  inline void fcmovb(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVB, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmovbe(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVBE, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmove(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVE, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmovnb(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVNB, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmovnbe(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVNBE, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmovne(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVNE, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmovnu(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVNU, &src);
  }
  //! @brief FP Conditional Move (FPU).
  inline void fcmovu(const X87Reg& src)
  {
    _emitInstruction(INST_FCMOVU, &src);
  }

  //! @brief Compare st(0) with @a reg (FPU).
  inline void fcom(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FCOM, &reg);
  }
  //! @brief Compare st(0) with 4-byte or 8-byte FP at @a src (FPU).
  inline void fcom(const Mem& src)
  {
    _emitInstruction(INST_FCOM, &src);
  }

  //! @brief Compare st(0) with @a reg and pop the stack (FPU).
  inline void fcomp(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FCOMP, &reg);
  }
  //! @brief Compare st(0) with 4-byte or 8-byte FP at @a adr and pop the
  //! stack (FPU).
  inline void fcomp(const Mem& mem)
  {
    _emitInstruction(INST_FCOMP, &mem);
  }

  //! @brief Compare st(0) with st(1) and pop register stack twice (FPU).
  inline void fcompp()
  {
    _emitInstruction(INST_FCOMPP);
  }

  //! @brief Compare st(0) and @a reg and Set EFLAGS (FPU).
  inline void fcomi(const X87Reg& reg)
  {
    _emitInstruction(INST_FCOMI, &reg);
  }

  //! @brief Compare st(0) and @a reg and Set EFLAGS and pop the stack (FPU).
  inline void fcomip(const X87Reg& reg)
  {
    _emitInstruction(INST_FCOMIP, &reg);
  }

  //! @brief Cosine (FPU).
  //!
  //! This instruction calculates the cosine of the source operand in
  //! register st(0) and stores the result in st(0).
  inline void fcos()
  {
    _emitInstruction(INST_FCOS);
  }

  //! @brief Decrement Stack-Top Pointer (FPU).
  //!
  //! Subtracts one from the TOP field of the FPU status word (decrements
  //! the top-ofstack pointer). If the TOP field contains a 0, it is set
  //! to 7. The effect of this instruction is to rotate the stack by one
  //! position. The contents of the FPU data registers and tag register
  //! are not affected.
  inline void fdecstp()
  {
    _emitInstruction(INST_FDECSTP);
  }

  //! @brief Divide @a dst by @a src (FPU).
  //!
  //! @note One of @a dst or @a src register must be st(0).
  inline void fdiv(const X87Reg& dst, const X87Reg& src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0 || src.getRegIndex() == 0);
    _emitInstruction(INST_FDIV, &dst, &src);
  }
  //! @brief Divide st(0) by 32-bit or 64-bit FP value (FPU).
  inline void fdiv(const Mem& src)
  {
    _emitInstruction(INST_FDIV, &src);
  }

  //! @brief Divide @a reg by st(0) (FPU).
  inline void fdivp(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FDIVP, &reg);
  }

  //! @brief Reverse Divide @a dst by @a src (FPU).
  //!
  //! @note One of @a dst or @a src register must be st(0).
  inline void fdivr(const X87Reg& dst, const X87Reg& src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0 || src.getRegIndex() == 0);
    _emitInstruction(INST_FDIVR, &dst, &src);
  }
  //! @brief Reverse Divide st(0) by 32-bit or 64-bit FP value (FPU).
  inline void fdivr(const Mem& src)
  {
    _emitInstruction(INST_FDIVR, &src);
  }

  //! @brief Reverse Divide @a reg by st(0) (FPU).
  inline void fdivrp(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FDIVRP, &reg);
  }

  //! @brief Free Floating-Point Register (FPU).
  //!
  //! Sets the tag in the FPU tag register associated with register @a reg
  //! to empty (11B). The contents of @a reg and the FPU stack-top pointer
  //! (TOP) are not affected.
  inline void ffree(const X87Reg& reg)
  {
    _emitInstruction(INST_FFREE, &reg);
  }

  //! @brief Add 16-bit or 32-bit integer to st(0) (FPU).
  inline void fiadd(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FIADD, &src);
  }

  //! @brief Compare st(0) with 16-bit or 32-bit Integer (FPU).
  inline void ficom(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FICOM, &src);
  }

  //! @brief Compare st(0) with 16-bit or 32-bit Integer and pop the stack (FPU).
  inline void ficomp(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FICOMP, &src);
  }

  //! @brief Divide st(0) by 32-bit or 16-bit integer (@a src) (FPU).
  inline void fidiv(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FIDIV, &src);
  }

  //! @brief Reverse Divide st(0) by 32-bit or 16-bit integer (@a src) (FPU).
  inline void fidivr(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FIDIVR, &src);
  }

  //! @brief Load 16-bit, 32-bit or 64-bit Integer and push it to the stack (FPU).
  //!
  //! Converts the signed-integer source operand into double extended-precision
  //! floating point format and pushes the value onto the FPU register stack.
  //! The source operand can be a word, doubleword, or quadword integer. It is
  //! loaded without rounding errors. The sign of the source operand is
  //! preserved.
  inline void fild(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4 || src.getSize() == 8);
    _emitInstruction(INST_FILD, &src);
  }

  //! @brief Multiply st(0) by 16-bit or 32-bit integer and store it
  //! to st(0) (FPU).
  inline void fimul(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FIMUL, &src);
  }

  //! @brief Increment Stack-Top Pointer (FPU).
  //!
  //! Adds one to the TOP field of the FPU status word (increments the
  //! top-of-stack pointer). If the TOP field contains a 7, it is set to 0.
  //! The effect of this instruction is to rotate the stack by one position.
  //! The contents of the FPU data registers and tag register are not affected.
  //! This operation is not equivalent to popping the stack, because the tag
  //! for the previous top-of-stack register is not marked empty.
  inline void fincstp()
  {
    _emitInstruction(INST_FINCSTP);
  }

  //! @brief Initialize Floating-Point Unit (FPU).
  //!
  //! Initialize FPU after checking for pending unmasked floating-point
  //! exceptions.
  inline void finit()
  {
    _emitInstruction(INST_FINIT);
  }

  //! @brief Subtract 16-bit or 32-bit integer from st(0) and store result to
  //! st(0) (FPU).
  inline void fisub(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FISUB, &src);
  }

  //! @brief Reverse Subtract 16-bit or 32-bit integer from st(0) and
  //! store result to  st(0) (FPU).
  inline void fisubr(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 2 || src.getSize() == 4);
    _emitInstruction(INST_FISUBR, &src);
  }

  //! @brief Initialize Floating-Point Unit (FPU).
  //!
  //! Initialize FPU without checking for pending unmasked floating-point
  //! exceptions.
  inline void fninit()
  {
    _emitInstruction(INST_FNINIT);
  }

  //! @brief Store st(0) as 16-bit or 32-bit Integer to @a dst (FPU).
  inline void fist(const Mem& dst)
  {
    ASMJIT_ASSERT(dst.getSize() == 2 || dst.getSize() == 4);
    _emitInstruction(INST_FIST, &dst);
  }

  //! @brief Store st(0) as 16-bit, 32-bit or 64-bit Integer to @a dst and pop
  //! stack (FPU).
  inline void fistp(const Mem& dst)
  {
    ASMJIT_ASSERT(dst.getSize() == 2 || dst.getSize() == 4 || dst.getSize() == 8);
    _emitInstruction(INST_FISTP, &dst);
  }

  //! @brief Push 32-bit, 64-bit or 80-bit Floating Point Value onto the FPU
  //! register stack (FPU).
  inline void fld(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 4 || src.getSize() == 8 || src.getSize() == 10);
    _emitInstruction(INST_FLD, &src);
  }

  //! @brief Push @a reg onto the FPU register stack (FPU).
  inline void fld(const X87Reg& reg)
  {
    _emitInstruction(INST_FLD, &reg);
  }

  //! @brief Push +1.0 onto the FPU register stack (FPU).
  inline void fld1()
  {
    _emitInstruction(INST_FLD1);
  }

  //! @brief Push log2(10) onto the FPU register stack (FPU).
  inline void fldl2t()
  {
    _emitInstruction(INST_FLDL2T);
  }

  //! @brief Push log2(e) onto the FPU register stack (FPU).
  inline void fldl2e()
  {
    _emitInstruction(INST_FLDL2E);
  }

  //! @brief Push pi onto the FPU register stack (FPU).
  inline void fldpi()
  {
    _emitInstruction(INST_FLDPI);
  }

  //! @brief Push log10(2) onto the FPU register stack (FPU).
  inline void fldlg2()
  {
    _emitInstruction(INST_FLDLG2);
  }

  //! @brief Push ln(2) onto the FPU register stack (FPU).
  inline void fldln2()
  {
    _emitInstruction(INST_FLDLN2);
  }

  //! @brief Push +0.0 onto the FPU register stack (FPU).
  inline void fldz()
  {
    _emitInstruction(INST_FLDZ);
  }

  //! @brief Load x87 FPU Control Word (2 bytes) (FPU).
  inline void fldcw(const Mem& src)
  {
    _emitInstruction(INST_FLDCW, &src);
  }

  //! @brief Load x87 FPU Environment (14 or 28 bytes) (FPU).
  inline void fldenv(const Mem& src)
  {
    _emitInstruction(INST_FLDENV, &src);
  }

  //! @brief Multiply @a dst by @a src and store result in @a dst (FPU).
  //!
  //! @note One of dst or src must be st(0).
  inline void fmul(const X87Reg& dst, const X87Reg& src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0 || src.getRegIndex() == 0);
    _emitInstruction(INST_FMUL, &dst, &src);
  }
  //! @brief Multiply st(0) by @a src and store result in st(0) (FPU).
  //!
  //! @note SP-FP or DP-FP determined by @a adr size.
  inline void fmul(const Mem& src)
  {
    _emitInstruction(INST_FMUL, &src);
  }

  //! @brief Multiply st(0) by @a dst and POP register stack (FPU).
  inline void fmulp(const X87Reg& dst = st(1))
  {
    _emitInstruction(INST_FMULP, &dst);
  }

  //! @brief Clear Exceptions (FPU).
  //!
  //! Clear floating-point exception flags without checking for pending
  //! unmasked floating-point exceptions.
  //!
  //! Clears the floating-point exception flags (PE, UE, OE, ZE, DE, and IE),
  //! the exception summary status flag (ES), the stack fault flag (SF), and
  //! the busy flag (B) in the FPU status word. The FCLEX instruction does
  //! not checks for and handles any pending unmasked floating-point exceptions
  //! before clearing the exception flags.
  inline void fnclex()
  {
    _emitInstruction(INST_FNCLEX);
  }

  //! @brief No Operation (FPU).
  inline void fnop()
  {
    _emitInstruction(INST_FNOP);
  }

  //! @brief Save FPU State (FPU).
  //!
  //! Store FPU environment to m94byte or m108byte without
  //! checking for pending unmasked FP exceptions.
  //! Then re-initialize the FPU.
  inline void fnsave(const Mem& dst)
  {
    _emitInstruction(INST_FNSAVE, &dst);
  }

  //! @brief Store x87 FPU Environment (FPU).
  //!
  //! Store FPU environment to @a dst (14 or 28 Bytes) without checking for
  //! pending unmasked floating-point exceptions. Then mask all floating
  //! point exceptions.
  inline void fnstenv(const Mem& dst)
  {
    _emitInstruction(INST_FNSTENV, &dst);
  }

  //! @brief Store x87 FPU Control Word (FPU).
  //!
  //! Store FPU control word to @a dst (2 Bytes) without checking for pending
  //! unmasked floating-point exceptions.
  inline void fnstcw(const Mem& dst)
  {
    _emitInstruction(INST_FNSTCW, &dst);
  }

  //! @brief Store x87 FPU Status Word (2 Bytes) (FPU).
  inline void fnstsw(const GPReg& dst)
  {
    ASMJIT_ASSERT(dst.isRegCode(REG_AX));
    _emitInstruction(INST_FNSTSW, &dst);
  }
  //! @brief Store x87 FPU Status Word (2 Bytes) (FPU).
  inline void fnstsw(const Mem& dst)
  {
    _emitInstruction(INST_FNSTSW, &dst);
  }

  //! @brief Partial Arctangent (FPU).
  //!
  //! Replace st(1) with arctan(st(1)/st(0)) and pop the register stack.
  inline void fpatan()
  {
    _emitInstruction(INST_FPATAN);
  }

  //! @brief Partial Remainder (FPU).
  //!
  //! Replace st(0) with the remainder obtained from dividing st(0) by st(1).
  inline void fprem()
  {
    _emitInstruction(INST_FPREM);
  }

  //! @brief Partial Remainder (FPU).
  //!
  //! Replace st(0) with the IEEE remainder obtained from dividing st(0) by
  //! st(1).
  inline void fprem1()
  {
    _emitInstruction(INST_FPREM1);
  }

  //! @brief Partial Tangent (FPU).
  //!
  //! Replace st(0) with its tangent and push 1 onto the FPU stack.
  inline void fptan()
  {
    _emitInstruction(INST_FPTAN);
  }

  //! @brief Round to Integer (FPU).
  //!
  //! Rount st(0) to an Integer.
  inline void frndint()
  {
    _emitInstruction(INST_FRNDINT);
  }

  //! @brief Restore FPU State (FPU).
  //!
  //! Load FPU state from src (94 or 108 bytes).
  inline void frstor(const Mem& src)
  {
    _emitInstruction(INST_FRSTOR, &src);
  }

  //! @brief Save FPU State (FPU).
  //!
  //! Store FPU state to 94 or 108-bytes after checking for
  //! pending unmasked FP exceptions. Then reinitialize
  //! the FPU.
  inline void fsave(const Mem& dst)
  {
    _emitInstruction(INST_FSAVE, &dst);
  }

  //! @brief Scale (FPU).
  //!
  //! Scale st(0) by st(1).
  inline void fscale()
  {
    _emitInstruction(INST_FSCALE);
  }

  //! @brief Sine (FPU).
  //!
  //! This instruction calculates the sine of the source operand in
  //! register st(0) and stores the result in st(0).
  inline void fsin()
  {
    _emitInstruction(INST_FSIN);
  }

  //! @brief Sine and Cosine (FPU).
  //!
  //! Compute the sine and cosine of st(0); replace st(0) with
  //! the sine, and push the cosine onto the register stack.
  inline void fsincos()
  {
    _emitInstruction(INST_FSINCOS);
  }

  //! @brief Square Root (FPU).
  //!
  //! Calculates square root of st(0) and stores the result in st(0).
  inline void fsqrt()
  {
    _emitInstruction(INST_FSQRT);
  }

  //! @brief Store Floating Point Value (FPU).
  //!
  //! Store st(0) as 32-bit or 64-bit floating point value to @a dst.
  inline void fst(const Mem& dst)
  {
    ASMJIT_ASSERT(dst.getSize() == 4 || dst.getSize() == 8);
    _emitInstruction(INST_FST, &dst);
  }

  //! @brief Store Floating Point Value (FPU).
  //!
  //! Store st(0) to @a reg.
  inline void fst(const X87Reg& reg)
  {
    _emitInstruction(INST_FST, &reg);
  }

  //! @brief Store Floating Point Value and Pop Register Stack (FPU).
  //!
  //! Store st(0) as 32-bit or 64-bit floating point value to @a dst
  //! and pop register stack.
  inline void fstp(const Mem& dst)
  {
    ASMJIT_ASSERT(dst.getSize() == 4 || dst.getSize() == 8 || dst.getSize() == 10);
    _emitInstruction(INST_FSTP, &dst);
  }

  //! @brief Store Floating Point Value and Pop Register Stack (FPU).
  //!
  //! Store st(0) to @a reg and pop register stack.
  inline void fstp(const X87Reg& reg)
  {
    _emitInstruction(INST_FSTP, &reg);
  }

  //! @brief Store x87 FPU Control Word (FPU).
  //!
  //! Store FPU control word to @a dst (2 Bytes) after checking for pending
  //! unmasked floating-point exceptions.
  inline void fstcw(const Mem& dst)
  {
    _emitInstruction(INST_FSTCW, &dst);
  }

  //! @brief Store x87 FPU Environment (FPU).
  //!
  //! Store FPU environment to @a dst (14 or 28 Bytes) after checking for
  //! pending unmasked floating-point exceptions. Then mask all floating
  //! point exceptions.
  inline void fstenv(const Mem& dst)
  {
    _emitInstruction(INST_FSTENV, &dst);
  }

  //! @brief Store x87 FPU Status Word (2 Bytes) (FPU).
  inline void fstsw(const GPReg& dst)
  {
    ASMJIT_ASSERT(dst.isRegCode(REG_AX));
    _emitInstruction(INST_FSTSW, &dst);
  }
  //! @brief Store x87 FPU Status Word (2 Bytes) (FPU).
  inline void fstsw(const Mem& dst)
  {
    _emitInstruction(INST_FSTSW, &dst);
  }

  //! @brief Subtract @a src from @a dst and store result in @a dst (FPU).
  //!
  //! @note One of dst or src must be st(0).
  inline void fsub(const X87Reg& dst, const X87Reg& src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0 || src.getRegIndex() == 0);
    _emitInstruction(INST_FSUB, &dst, &src);
  }
  //! @brief Subtract @a src from st(0) and store result in st(0) (FPU).
  //!
  //! @note SP-FP or DP-FP determined by @a adr size.
  inline void fsub(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 4 || src.getSize() == 8);
    _emitInstruction(INST_FSUB, &src);
  }

  //! @brief Subtract st(0) from @a dst and POP register stack (FPU).
  inline void fsubp(const X87Reg& dst = st(1))
  {
    _emitInstruction(INST_FSUBP, &dst);
  }

  //! @brief Reverse Subtract @a src from @a dst and store result in @a dst (FPU).
  //!
  //! @note One of dst or src must be st(0).
  inline void fsubr(const X87Reg& dst, const X87Reg& src)
  {
    ASMJIT_ASSERT(dst.getRegIndex() == 0 || src.getRegIndex() == 0);
    _emitInstruction(INST_FSUBR, &dst, &src);
  }

  //! @brief Reverse Subtract @a src from st(0) and store result in st(0) (FPU).
  //!
  //! @note SP-FP or DP-FP determined by @a adr size.
  inline void fsubr(const Mem& src)
  {
    ASMJIT_ASSERT(src.getSize() == 4 || src.getSize() == 8);
    _emitInstruction(INST_FSUBR, &src);
  }

  //! @brief Reverse Subtract st(0) from @a dst and POP register stack (FPU).
  inline void fsubrp(const X87Reg& dst = st(1))
  {
    _emitInstruction(INST_FSUBRP, &dst);
  }

  //! @brief Floating point test - Compare st(0) with 0.0. (FPU).
  inline void ftst()
  {
    _emitInstruction(INST_FTST);
  }

  //! @brief Unordered Compare st(0) with @a reg (FPU).
  inline void fucom(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FUCOM, &reg);
  }

  //! @brief Unordered Compare st(0) and @a reg, check for ordered values
  //! and Set EFLAGS (FPU).
  inline void fucomi(const X87Reg& reg)
  {
    _emitInstruction(INST_FUCOMI, &reg);
  }

  //! @brief UnorderedCompare st(0) and @a reg, Check for ordered values
  //! and Set EFLAGS and pop the stack (FPU).
  inline void fucomip(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FUCOMIP, &reg);
  }

  //! @brief Unordered Compare st(0) with @a reg and pop register stack (FPU).
  inline void fucomp(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FUCOMP, &reg);
  }

  //! @brief Unordered compare st(0) with st(1) and pop register stack twice
  //! (FPU).
  inline void fucompp()
  {
    _emitInstruction(INST_FUCOMPP);
  }

  inline void fwait()
  {
    _emitInstruction(INST_FWAIT);
  }

  //! @brief Examine st(0) (FPU).
  //!
  //! Examines the contents of the ST(0) register and sets the condition code
  //! flags C0, C2, and C3 in the FPU status word to indicate the class of
  //! value or number in the register.
  inline void fxam()
  {
    _emitInstruction(INST_FXAM);
  }

  //! @brief Exchange Register Contents (FPU).
  //!
  //! Exchange content of st(0) with @a reg.
  inline void fxch(const X87Reg& reg = st(1))
  {
    _emitInstruction(INST_FXCH, &reg);
  }

  //! @brief Restore FP And MMX(tm) State And Streaming SIMD Extension State
  //! (FPU, MMX, SSE).
  //!
  //! Load FP and MMX(tm) technology and Streaming SIMD Extension state from
  //! src (512 bytes).
  inline void fxrstor(const Mem& src)
  {
    _emitInstruction(INST_FXRSTOR, &src);
  }

  //! @brief Store FP and MMX(tm) State and Streaming SIMD Extension State
  //! (FPU, MMX, SSE).
  //!
  //! Store FP and MMX(tm) technology state and Streaming SIMD Extension state
  //! to dst (512 bytes).
  inline void fxsave(const Mem& dst)
  {
    _emitInstruction(INST_FXSAVE, &dst);
  }

  //! @brief Extract Exponent and Significand (FPU).
  //!
  //! Separate value in st(0) into exponent and significand, store exponent
  //! in st(0), and push the significand onto the register stack.
  inline void fxtract()
  {
    _emitInstruction(INST_FXTRACT);
  }

  //! @brief Compute y * log2(x).
  //!
  //! Replace st(1) with (st(1) * log2st(0)) and pop the register stack.
  inline void fyl2x()
  {
    _emitInstruction(INST_FYL2X);
  }

  //! @brief Compute y * log_2(x+1).
  //!
  //! Replace st(1) with (st(1) * (log2st(0) + 1.0)) and pop the register stack.
  inline void fyl2xp1()
  {
    _emitInstruction(INST_FYL2XP1);
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
  inline void movd(const Mem& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord (MMX).
  inline void movd(const GPReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord (MMX).
  inline void movd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord (MMX).
  inline void movd(const MMReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }

  //! @brief Move QWord (MMX).
  inline void movq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
  //! @brief Move QWord (MMX).
  inline void movq(const Mem& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (MMX).
  inline void movq(const GPReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif
  //! @brief Move QWord (MMX).
  inline void movq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (MMX).
  inline void movq(const MMReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif

  //! @brief Pack with Signed Saturation (MMX).
  inline void packsswb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (MMX).
  inline void packsswb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }

  //! @brief Pack with Signed Saturation (MMX).
  inline void packssdw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (MMX).
  inline void packssdw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }

  //! @brief Pack with Unsigned Saturation (MMX).
  inline void packuswb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }
  //! @brief Pack with Unsigned Saturation (MMX).
  inline void packuswb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }

  //! @brief Packed BYTE Add (MMX).
  inline void paddb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }
  //! @brief Packed BYTE Add (MMX).
  inline void paddb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }

  //! @brief Packed WORD Add (MMX).
  inline void paddw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }
  //! @brief Packed WORD Add (MMX).
  inline void paddw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }

  //! @brief Packed DWORD Add (MMX).
  inline void paddd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }
  //! @brief Packed DWORD Add (MMX).
  inline void paddd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }

  //! @brief Packed Add with Saturation (MMX).
  inline void paddsb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }
  //! @brief Packed Add with Saturation (MMX).
  inline void paddsb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }

  //! @brief Packed Add with Saturation (MMX).
  inline void paddsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }
  //! @brief Packed Add with Saturation (MMX).
  inline void paddsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (MMX).
  inline void paddusw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }

  //! @brief Logical AND (MMX).
  inline void pand(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }
  //! @brief Logical AND (MMX).
  inline void pand(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }

  //! @brief Logical AND Not (MMX).
  inline void pandn(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }
  //! @brief Logical AND Not (MMX).
  inline void pandn(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }

  //! @brief Packed Compare for Equal (BYTES) (MMX).
  inline void pcmpeqb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }
  //! @brief Packed Compare for Equal (BYTES) (MMX).
  inline void pcmpeqb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }

  //! @brief Packed Compare for Equal (WORDS) (MMX).
  inline void pcmpeqw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }
  //! @brief Packed Compare for Equal (WORDS) (MMX).
  inline void pcmpeqw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }

  //! @brief Packed Compare for Equal (DWORDS) (MMX).
  inline void pcmpeqd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }
  //! @brief Packed Compare for Equal (DWORDS) (MMX).
  inline void pcmpeqd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (BYTES) (MMX).
  inline void pcmpgtb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (BYTES) (MMX).
  inline void pcmpgtb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (WORDS) (MMX).
  inline void pcmpgtw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (WORDS) (MMX).
  inline void pcmpgtw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (DWORDS) (MMX).
  inline void pcmpgtd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (DWORDS) (MMX).
  inline void pcmpgtd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }

  //! @brief Packed Multiply High (MMX).
  inline void pmulhw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }
  //! @brief Packed Multiply High (MMX).
  inline void pmulhw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }

  //! @brief Packed Multiply Low (MMX).
  inline void pmullw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }
  //! @brief Packed Multiply Low (MMX).
  inline void pmullw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }

  //! @brief Bitwise Logical OR (MMX).
  inline void por(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }
  //! @brief Bitwise Logical OR (MMX).
  inline void por(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }

  //! @brief Packed Multiply and Add (MMX).
  inline void pmaddwd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }
  //! @brief Packed Multiply and Add (MMX).
  inline void pmaddwd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (MMX).
  inline void pslld(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void pslld(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void pslld(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (MMX).
  inline void psllq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllq(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (MMX).
  inline void psllw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (MMX).
  inline void psllw(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psrad(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psrad(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psrad(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psraw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psraw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (MMX).
  inline void psraw(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (MMX).
  inline void psrld(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrld(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrld(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlq(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (MMX).
  inline void psrlw(const MMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }

  //! @brief Packed Subtract (MMX).
  inline void psubb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }
  //! @brief Packed Subtract (MMX).
  inline void psubb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }

  //! @brief Packed Subtract (MMX).
  inline void psubw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }
  //! @brief Packed Subtract (MMX).
  inline void psubw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }

  //! @brief Packed Subtract (MMX).
  inline void psubd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }
  //! @brief Packed Subtract (MMX).
  inline void psubd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (MMX).
  inline void psubsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (MMX).
  inline void psubusw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhbw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhbw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhwd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhwd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhdq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckhdq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklbw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklbw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklwd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpcklwd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }

  //! @brief Unpack High Packed Data (MMX).
  inline void punpckldq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }
  //! @brief Unpack High Packed Data (MMX).
  inline void punpckldq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }

  //! @brief Bitwise Exclusive OR (MMX).
  inline void pxor(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }
  //! @brief Bitwise Exclusive OR (MMX).
  inline void pxor(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }

  // -------------------------------------------------------------------------
  // [3dNow]
  // -------------------------------------------------------------------------

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
  inline void pf2id(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PF2ID, &dst, &src);
  }
  //! @brief Packed SP-FP to Integer Convert (3dNow!).
  inline void pf2id(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PF2ID, &dst, &src);
  }

  //! @brief  Packed SP-FP to Integer Word Convert (3dNow!).
  inline void pf2iw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PF2IW, &dst, &src);
  }
  //! @brief  Packed SP-FP to Integer Word Convert (3dNow!).
  inline void pf2iw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PF2IW, &dst, &src);
  }

  //! @brief Packed SP-FP Accumulate (3dNow!).
  inline void pfacc(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFACC, &dst, &src);
  }
  //! @brief Packed SP-FP Accumulate (3dNow!).
  inline void pfacc(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFACC, &dst, &src);
  }

  //! @brief Packed SP-FP Addition (3dNow!).
  inline void pfadd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFADD, &dst, &src);
  }
  //! @brief Packed SP-FP Addition (3dNow!).
  inline void pfadd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFADD, &dst, &src);
  }

  //! @brief Packed SP-FP Compare - dst == src (3dNow!).
  inline void pfcmpeq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFCMPEQ, &dst, &src);
  }
  //! @brief Packed SP-FP Compare - dst == src (3dNow!).
  inline void pfcmpeq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFCMPEQ, &dst, &src);
  }

  //! @brief Packed SP-FP Compare - dst >= src (3dNow!).
  inline void pfcmpge(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFCMPGE, &dst, &src);
  }
  //! @brief Packed SP-FP Compare - dst >= src (3dNow!).
  inline void pfcmpge(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFCMPGE, &dst, &src);
  }

  //! @brief Packed SP-FP Compare - dst > src (3dNow!).
  inline void pfcmpgt(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFCMPGT, &dst, &src);
  }
  //! @brief Packed SP-FP Compare - dst > src (3dNow!).
  inline void pfcmpgt(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFCMPGT, &dst, &src);
  }

  //! @brief Packed SP-FP Maximum (3dNow!).
  inline void pfmax(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFMAX, &dst, &src);
  }
  //! @brief Packed SP-FP Maximum (3dNow!).
  inline void pfmax(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFMAX, &dst, &src);
  }

  //! @brief Packed SP-FP Minimum (3dNow!).
  inline void pfmin(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFMIN, &dst, &src);
  }
  //! @brief Packed SP-FP Minimum (3dNow!).
  inline void pfmin(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFMIN, &dst, &src);
  }

  //! @brief Packed SP-FP Multiply (3dNow!).
  inline void pfmul(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFMUL, &dst, &src);
  }
  //! @brief Packed SP-FP Multiply (3dNow!).
  inline void pfmul(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFMUL, &dst, &src);
  }

  //! @brief Packed SP-FP Negative Accumulate (3dNow!).
  inline void pfnacc(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFNACC, &dst, &src);
  }
  //! @brief Packed SP-FP Negative Accumulate (3dNow!).
  inline void pfnacc(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFNACC, &dst, &src);
  }

  //! @brief Packed SP-FP Mixed Accumulate (3dNow!).
  inline void pfpnaxx(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFPNACC, &dst, &src);
  }
  //! @brief Packed SP-FP Mixed Accumulate (3dNow!).
  inline void pfpnacc(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFPNACC, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal Approximation (3dNow!).
  inline void pfrcp(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFRCP, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal Approximation (3dNow!).
  inline void pfrcp(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRCP, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal, First Iteration Step (3dNow!).
  inline void pfrcpit1(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFRCPIT1, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal, First Iteration Step (3dNow!).
  inline void pfrcpit1(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRCPIT1, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal, Second Iteration Step (3dNow!).
  inline void pfrcpit2(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFRCPIT2, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal, Second Iteration Step (3dNow!).
  inline void pfrcpit2(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRCPIT2, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal Square Root, First Iteration Step (3dNow!).
  inline void pfrsqit1(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFRSQIT1, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal Square Root, First Iteration Step (3dNow!).
  inline void pfrsqit1(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRSQIT1, &dst, &src);
  }

  //! @brief Packed SP-FP Reciprocal Square Root Approximation (3dNow!).
  inline void pfrsqrt(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFRSQRT, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal Square Root Approximation (3dNow!).
  inline void pfrsqrt(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFRSQRT, &dst, &src);
  }

  //! @brief Packed SP-FP Subtract (3dNow!).
  inline void pfsub(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFSUB, &dst, &src);
  }
  //! @brief Packed SP-FP Subtract (3dNow!).
  inline void pfsub(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFSUB, &dst, &src);
  }

  //! @brief Packed SP-FP Reverse Subtract (3dNow!).
  inline void pfsubr(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PFSUBR, &dst, &src);
  }
  //! @brief Packed SP-FP Reverse Subtract (3dNow!).
  inline void pfsubr(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PFSUBR, &dst, &src);
  }

  //! @brief Packed DWords to SP-FP (3dNow!).
  inline void pi2fd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PI2FD, &dst, &src);
  }
  //! @brief Packed DWords to SP-FP (3dNow!).
  inline void pi2fd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PI2FD, &dst, &src);
  }

  //! @brief Packed Words to SP-FP (3dNow!).
  inline void pi2fw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PI2FW, &dst, &src);
  }
  //! @brief Packed Words to SP-FP (3dNow!).
  inline void pi2fw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PI2FW, &dst, &src);
  }

  //! @brief Packed swap DWord (3dNow!)
  inline void pswapd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSWAPD, &dst, &src);
  }
  //! @brief Packed swap DWord (3dNow!)
  inline void pswapd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSWAPD, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [SSE]
  // --------------------------------------------------------------------------

  //! @brief Packed SP-FP Add (SSE).
  inline void addps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ADDPS, &dst, &src);
  }
  //! @brief Packed SP-FP Add (SSE).
  inline void addps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Add (SSE).
  inline void addss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ADDSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Add (SSE).
  inline void addss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSS, &dst, &src);
  }

  //! @brief Bit-wise Logical And Not For SP-FP (SSE).
  inline void andnps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ANDNPS, &dst, &src);
  }
  //! @brief Bit-wise Logical And Not For SP-FP (SSE).
  inline void andnps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDNPS, &dst, &src);
  }

  //! @brief Bit-wise Logical And For SP-FP (SSE).
  inline void andps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ANDPS, &dst, &src);
  }
  //! @brief Bit-wise Logical And For SP-FP (SSE).
  inline void andps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDPS, &dst, &src);
  }

  //! @brief Packed SP-FP Compare (SSE).
  inline void cmpps(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPS, &dst, &src, &imm8);
  }
  //! @brief Packed SP-FP Compare (SSE).
  inline void cmpps(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPS, &dst, &src, &imm8);
  }

  //! @brief Compare Scalar SP-FP Values (SSE).
  inline void cmpss(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSS, &dst, &src, &imm8);
  }
  //! @brief Compare Scalar SP-FP Values (SSE).
  inline void cmpss(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSS, &dst, &src, &imm8);
  }

  //! @brief Scalar Ordered SP-FP Compare and Set EFLAGS (SSE).
  inline void comiss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_COMISS, &dst, &src);
  }
  //! @brief Scalar Ordered SP-FP Compare and Set EFLAGS (SSE).
  inline void comiss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_COMISS, &dst, &src);
  }

  //! @brief Packed Signed INT32 to Packed SP-FP Conversion (SSE).
  inline void cvtpi2ps(const XMMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_CVTPI2PS, &dst, &src);
  }
  //! @brief Packed Signed INT32 to Packed SP-FP Conversion (SSE).
  inline void cvtpi2ps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPI2PS, &dst, &src);
  }

  //! @brief Packed SP-FP to Packed INT32 Conversion (SSE).
  inline void cvtps2pi(const MMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTPS2PI, &dst, &src);
  }
  //! @brief Packed SP-FP to Packed INT32 Conversion (SSE).
  inline void cvtps2pi(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPS2PI, &dst, &src);
  }

  //! @brief Scalar Signed INT32 to SP-FP Conversion (SSE).
  inline void cvtsi2ss(const XMMReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_CVTSI2SS, &dst, &src);
  }
  //! @brief Scalar Signed INT32 to SP-FP Conversion (SSE).
  inline void cvtsi2ss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSI2SS, &dst, &src);
  }

  //! @brief Scalar SP-FP to Signed INT32 Conversion (SSE).
  inline void cvtss2si(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTSS2SI, &dst, &src);
  }
  //! @brief Scalar SP-FP to Signed INT32 Conversion (SSE).
  inline void cvtss2si(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSS2SI, &dst, &src);
  }

  //! @brief Packed SP-FP to Packed INT32 Conversion (truncate) (SSE).
  inline void cvttps2pi(const MMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTTPS2PI, &dst, &src);
  }
  //! @brief Packed SP-FP to Packed INT32 Conversion (truncate) (SSE).
  inline void cvttps2pi(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPS2PI, &dst, &src);
  }

  //! @brief Scalar SP-FP to Signed INT32 Conversion (truncate) (SSE).
  inline void cvttss2si(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTTSS2SI, &dst, &src);
  }
  //! @brief Scalar SP-FP to Signed INT32 Conversion (truncate) (SSE).
  inline void cvttss2si(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTSS2SI, &dst, &src);
  }

  //! @brief Packed SP-FP Divide (SSE).
  inline void divps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_DIVPS, &dst, &src);
  }
  //! @brief Packed SP-FP Divide (SSE).
  inline void divps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_DIVPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Divide (SSE).
  inline void divss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_DIVSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Divide (SSE).
  inline void divss(const XMMReg& dst, const Mem& src)
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
  inline void maskmovq(const MMReg& data, const MMReg& mask)
  {
    _emitInstruction(INST_MASKMOVQ, &data, &mask);
  }

  //! @brief Packed SP-FP Maximum (SSE).
  inline void maxps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MAXPS, &dst, &src);
  }
  //! @brief Packed SP-FP Maximum (SSE).
  inline void maxps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Maximum (SSE).
  inline void maxss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MAXSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Maximum (SSE).
  inline void maxss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXSS, &dst, &src);
  }

  //! @brief Packed SP-FP Minimum (SSE).
  inline void minps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MINPS, &dst, &src);
  }
  //! @brief Packed SP-FP Minimum (SSE).
  inline void minps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MINPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Minimum (SSE).
  inline void minss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MINSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Minimum (SSE).
  inline void minss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MINSS, &dst, &src);
  }

  //! @brief Move Aligned Packed SP-FP Values (SSE).
  inline void movaps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVAPS, &dst, &src);
  }
  //! @brief Move Aligned Packed SP-FP Values (SSE).
  inline void movaps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVAPS, &dst, &src);
  }

  //! @brief Move Aligned Packed SP-FP Values (SSE).
  inline void movaps(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVAPS, &dst, &src);
  }

  //! @brief Move DWord.
  inline void movd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord.
  inline void movd(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord.
  inline void movd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }
  //! @brief Move DWord.
  inline void movd(const XMMReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVD, &dst, &src);
  }

  //! @brief Move QWord (SSE).
  inline void movq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
  //! @brief Move QWord (SSE).
  inline void movq(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (SSE).
  inline void movq(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif // ASMJIT_X64
  //! @brief Move QWord (SSE).
  inline void movq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#if defined(ASMJIT_X64)
  //! @brief Move QWord (SSE).
  inline void movq(const XMMReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVQ, &dst, &src);
  }
#endif // ASMJIT_X64

  //! @brief Move 64 Bits Non Temporal (SSE).
  inline void movntq(const Mem& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVNTQ, &dst, &src);
  }

  //! @brief High to Low Packed SP-FP (SSE).
  inline void movhlps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVHLPS, &dst, &src);
  }

  //! @brief Move High Packed SP-FP (SSE).
  inline void movhps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVHPS, &dst, &src);
  }

  //! @brief Move High Packed SP-FP (SSE).
  inline void movhps(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVHPS, &dst, &src);
  }

  //! @brief Move Low to High Packed SP-FP (SSE).
  inline void movlhps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVLHPS, &dst, &src);
  }

  //! @brief Move Low Packed SP-FP (SSE).
  inline void movlps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVLPS, &dst, &src);
  }

  //! @brief Move Low Packed SP-FP (SSE).
  inline void movlps(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVLPS, &dst, &src);
  }

  //! @brief Move Aligned Four Packed SP-FP Non Temporal (SSE).
  inline void movntps(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVNTPS, &dst, &src);
  }

  //! @brief Move Scalar SP-FP (SSE).
  inline void movss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVSS, &dst, &src);
  }

  //! @brief Move Scalar SP-FP (SSE).
  inline void movss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSS, &dst, &src);
  }

  //! @brief Move Scalar SP-FP (SSE).
  inline void movss(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVSS, &dst, &src);
  }

  //! @brief Move Unaligned Packed SP-FP Values (SSE).
  inline void movups(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVUPS, &dst, &src);
  }
  //! @brief Move Unaligned Packed SP-FP Values (SSE).
  inline void movups(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVUPS, &dst, &src);
  }

  //! @brief Move Unaligned Packed SP-FP Values (SSE).
  inline void movups(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVUPS, &dst, &src);
  }

  //! @brief Packed SP-FP Multiply (SSE).
  inline void mulps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MULPS, &dst, &src);
  }
  //! @brief Packed SP-FP Multiply (SSE).
  inline void mulps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MULPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Multiply (SSE).
  inline void mulss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MULSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Multiply (SSE).
  inline void mulss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MULSS, &dst, &src);
  }

  //! @brief Bit-wise Logical OR for SP-FP Data (SSE).
  inline void orps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ORPS, &dst, &src);
  }
  //! @brief Bit-wise Logical OR for SP-FP Data (SSE).
  inline void orps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ORPS, &dst, &src);
  }

  //! @brief Packed Average (SSE).
  inline void pavgb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }
  //! @brief Packed Average (SSE).
  inline void pavgb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }

  //! @brief Packed Average (SSE).
  inline void pavgw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }
  //! @brief Packed Average (SSE).
  inline void pavgw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }

  //! @brief Extract Word (SSE).
  inline void pextrw(const GPReg& dst, const MMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRW, &dst, &src, &imm8);
  }

  //! @brief Insert Word (SSE).
  inline void pinsrw(const MMReg& dst, const GPReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }
  //! @brief Insert Word (SSE).
  inline void pinsrw(const MMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }

  //! @brief Packed Signed Integer Word Maximum (SSE).
  inline void pmaxsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Maximum (SSE).
  inline void pmaxsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Maximum (SSE).
  inline void pmaxub(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Maximum (SSE).
  inline void pmaxub(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }

  //! @brief Packed Signed Integer Word Minimum (SSE).
  inline void pminsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Minimum (SSE).
  inline void pminsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Minimum (SSE).
  inline void pminub(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Minimum (SSE).
  inline void pminub(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }

  //! @brief Move Byte Mask To Integer (SSE).
  inline void pmovmskb(const GPReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMOVMSKB, &dst, &src);
  }

  //! @brief Packed Multiply High Unsigned (SSE).
  inline void pmulhuw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }
  //! @brief Packed Multiply High Unsigned (SSE).
  inline void pmulhuw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }

  //! @brief Packed Sum of Absolute Differences (SSE).
  inline void psadbw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }
  //! @brief Packed Sum of Absolute Differences (SSE).
  inline void psadbw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }

  //! @brief Packed Shuffle word (SSE).
  inline void pshufw(const MMReg& dst, const MMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFW, &dst, &src, &imm8);
  }
  //! @brief Packed Shuffle word (SSE).
  inline void pshufw(const MMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFW, &dst, &src, &imm8);
  }

  //! @brief Packed SP-FP Reciprocal (SSE).
  inline void rcpps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_RCPPS, &dst, &src);
  }
  //! @brief Packed SP-FP Reciprocal (SSE).
  inline void rcpps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_RCPPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Reciprocal (SSE).
  inline void rcpss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_RCPSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Reciprocal (SSE).
  inline void rcpss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_RCPSS, &dst, &src);
  }

  //! @brief Prefetch (SSE).
  inline void prefetch(const Mem& mem, const Imm& hint)
  {
    _emitInstruction(INST_PREFETCH, &mem, &hint);
  }

  //! @brief Compute Sum of Absolute Differences (SSE).
  inline void psadbw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }
  //! @brief Compute Sum of Absolute Differences (SSE).
  inline void psadbw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSADBW, &dst, &src);
  }

  //! @brief Packed SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_RSQRTPS, &dst, &src);
  }
  //! @brief Packed SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_RSQRTPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_RSQRTSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Square Root Reciprocal (SSE).
  inline void rsqrtss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_RSQRTSS, &dst, &src);
  }

  //! @brief Store fence (SSE).
  inline void sfence()
  {
    _emitInstruction(INST_SFENCE);
  }

  //! @brief Shuffle SP-FP (SSE).
  inline void shufps(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPS, &dst, &src, &imm8);
  }
  //! @brief Shuffle SP-FP (SSE).
  inline void shufps(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPS, &dst, &src, &imm8);
  }

  //! @brief Packed SP-FP Square Root (SSE).
  inline void sqrtps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SQRTPS, &dst, &src);
  }
  //! @brief Packed SP-FP Square Root (SSE).
  inline void sqrtps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Square Root (SSE).
  inline void sqrtss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SQRTSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Square Root (SSE).
  inline void sqrtss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTSS, &dst, &src);
  }

  //! @brief Store Streaming SIMD Extension Control/Status (SSE).
  inline void stmxcsr(const Mem& dst)
  {
    _emitInstruction(INST_STMXCSR, &dst);
  }

  //! @brief Packed SP-FP Subtract (SSE).
  inline void subps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SUBPS, &dst, &src);
  }
  //! @brief Packed SP-FP Subtract (SSE).
  inline void subps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBPS, &dst, &src);
  }

  //! @brief Scalar SP-FP Subtract (SSE).
  inline void subss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SUBSS, &dst, &src);
  }
  //! @brief Scalar SP-FP Subtract (SSE).
  inline void subss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBSS, &dst, &src);
  }

  //! @brief Unordered Scalar SP-FP compare and set EFLAGS (SSE).
  inline void ucomiss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_UCOMISS, &dst, &src);
  }
  //! @brief Unordered Scalar SP-FP compare and set EFLAGS (SSE).
  inline void ucomiss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_UCOMISS, &dst, &src);
  }

  //! @brief Unpack High Packed SP-FP Data (SSE).
  inline void unpckhps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_UNPCKHPS, &dst, &src);
  }
  //! @brief Unpack High Packed SP-FP Data (SSE).
  inline void unpckhps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKHPS, &dst, &src);
  }

  //! @brief Unpack Low Packed SP-FP Data (SSE).
  inline void unpcklps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_UNPCKLPS, &dst, &src);
  }
  //! @brief Unpack Low Packed SP-FP Data (SSE).
  inline void unpcklps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKLPS, &dst, &src);
  }

  //! @brief Bit-wise Logical Xor for SP-FP Data (SSE).
  inline void xorps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_XORPS, &dst, &src);
  }
  //! @brief Bit-wise Logical Xor for SP-FP Data (SSE).
  inline void xorps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_XORPS, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [SSE2]
  // --------------------------------------------------------------------------

  //! @brief Packed DP-FP Add (SSE2).
  inline void addpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ADDPD, &dst, &src);
  }
  //! @brief Packed DP-FP Add (SSE2).
  inline void addpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Add (SSE2).
  inline void addsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ADDSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Add (SSE2).
  inline void addsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSD, &dst, &src);
  }

  //! @brief Bit-wise Logical And Not For DP-FP (SSE2).
  inline void andnpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ANDNPD, &dst, &src);
  }
  //! @brief Bit-wise Logical And Not For DP-FP (SSE2).
  inline void andnpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDNPD, &dst, &src);
  }

  //! @brief Bit-wise Logical And For DP-FP (SSE2).
  inline void andpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ANDPD, &dst, &src);
  }
  //! @brief Bit-wise Logical And For DP-FP (SSE2).
  inline void andpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ANDPD, &dst, &src);
  }

  //! @brief Flush Cache Line (SSE2).
  inline void clflush(const Mem& mem)
  {
    _emitInstruction(INST_CLFLUSH, &mem);
  }

  //! @brief Packed DP-FP Compare (SSE2).
  inline void cmppd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPD, &dst, &src, &imm8);
  }
  //! @brief Packed DP-FP Compare (SSE2).
  inline void cmppd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPPD, &dst, &src, &imm8);
  }

  //! @brief Compare Scalar SP-FP Values (SSE2).
  inline void cmpsd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSD, &dst, &src, &imm8);
  }
  //! @brief Compare Scalar SP-FP Values (SSE2).
  inline void cmpsd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_CMPSD, &dst, &src, &imm8);
  }

  //! @brief Scalar Ordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void comisd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_COMISD, &dst, &src);
  }
  //! @brief Scalar Ordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void comisd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_COMISD, &dst, &src);
  }

  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtdq2pd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTDQ2PD, &dst, &src);
  }
  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtdq2pd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTDQ2PD, &dst, &src);
  }

  //! @brief Convert Packed Dword Integers to Packed SP-FP Values (SSE2).
  inline void cvtdq2ps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTDQ2PS, &dst, &src);
  }
  //! @brief Convert Packed Dword Integers to Packed SP-FP Values (SSE2).
  inline void cvtdq2ps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTDQ2PS, &dst, &src);
  }

  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2dq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTPD2DQ, &dst, &src);
  }
  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2dq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPD2DQ, &dst, &src);
  }

  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2pi(const MMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTPD2PI, &dst, &src);
  }
  //! @brief Convert Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtpd2pi(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPD2PI, &dst, &src);
  }

  //! @brief Convert Packed DP-FP Values to Packed SP-FP Values (SSE2).
  inline void cvtpd2ps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTPD2PS, &dst, &src);
  }
  //! @brief Convert Packed DP-FP Values to Packed SP-FP Values (SSE2).
  inline void cvtpd2ps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPD2PS, &dst, &src);
  }

  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtpi2pd(const XMMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_CVTPI2PD, &dst, &src);
  }
  //! @brief Convert Packed Dword Integers to Packed DP-FP Values (SSE2).
  inline void cvtpi2pd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPI2PD, &dst, &src);
  }

  //! @brief Convert Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtps2dq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTPS2DQ, &dst, &src);
  }
  //! @brief Convert Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvtps2dq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPS2DQ, &dst, &src);
  }

  //! @brief Convert Packed SP-FP Values to Packed DP-FP Values (SSE2).
  inline void cvtps2pd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTPS2PD, &dst, &src);
  }
  //! @brief Convert Packed SP-FP Values to Packed DP-FP Values (SSE2).
  inline void cvtps2pd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTPS2PD, &dst, &src);
  }

  //! @brief Convert Scalar DP-FP Value to Dword Integer (SSE2).
  inline void cvtsd2si(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTSD2SI, &dst, &src);
  }
  //! @brief Convert Scalar DP-FP Value to Dword Integer (SSE2).
  inline void cvtsd2si(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSD2SI, &dst, &src);
  }

  //! @brief Convert Scalar DP-FP Value to Scalar SP-FP Value (SSE2).
  inline void cvtsd2ss(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTSD2SS, &dst, &src);
  }
  //! @brief Convert Scalar DP-FP Value to Scalar SP-FP Value (SSE2).
  inline void cvtsd2ss(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSD2SS, &dst, &src);
  }

  //! @brief Convert Dword Integer to Scalar DP-FP Value (SSE2).
  inline void cvtsi2sd(const XMMReg& dst, const GPReg& src)
  {
    _emitInstruction(INST_CVTSI2SD, &dst, &src);
  }
  //! @brief Convert Dword Integer to Scalar DP-FP Value (SSE2).
  inline void cvtsi2sd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSI2SD, &dst, &src);
  }

  //! @brief Convert Scalar SP-FP Value to Scalar DP-FP Value (SSE2).
  inline void cvtss2sd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTSS2SD, &dst, &src);
  }
  //! @brief Convert Scalar SP-FP Value to Scalar DP-FP Value (SSE2).
  inline void cvtss2sd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTSS2SD, &dst, &src);
  }

  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2pi(const MMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTTPD2PI, &dst, &src);
  }
  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2pi(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPD2PI, &dst, &src);
  }

  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2dq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTTPD2DQ, &dst, &src);
  }
  //! @brief Convert with Truncation Packed DP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttpd2dq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPD2DQ, &dst, &src);
  }

  //! @brief Convert with Truncation Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttps2dq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTTPS2DQ, &dst, &src);
  }
  //! @brief Convert with Truncation Packed SP-FP Values to Packed Dword Integers (SSE2).
  inline void cvttps2dq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTPS2DQ, &dst, &src);
  }

  //! @brief Convert with Truncation Scalar DP-FP Value to Signed Dword Integer (SSE2).
  inline void cvttsd2si(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_CVTTSD2SI, &dst, &src);
  }
  //! @brief Convert with Truncation Scalar DP-FP Value to Signed Dword Integer (SSE2).
  inline void cvttsd2si(const GPReg& dst, const Mem& src)
  {
    _emitInstruction(INST_CVTTSD2SI, &dst, &src);
  }

  //! @brief Packed DP-FP Divide (SSE2).
  inline void divpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_DIVPD, &dst, &src);
  }
  //! @brief Packed DP-FP Divide (SSE2).
  inline void divpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_DIVPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Divide (SSE2).
  inline void divsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_DIVSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Divide (SSE2).
  inline void divsd(const XMMReg& dst, const Mem& src)
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
  inline void maskmovdqu(const XMMReg& src, const XMMReg& mask)
  {
    _emitInstruction(INST_MASKMOVDQU, &src, &mask);
  }

  //! @brief Return Maximum Packed Double-Precision FP Values (SSE2).
  inline void maxpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MAXPD, &dst, &src);
  }
  //! @brief Return Maximum Packed Double-Precision FP Values (SSE2).
  inline void maxpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXPD, &dst, &src);
  }

  //! @brief Return Maximum Scalar Double-Precision FP Value (SSE2).
  inline void maxsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MAXSD, &dst, &src);
  }
  //! @brief Return Maximum Scalar Double-Precision FP Value (SSE2).
  inline void maxsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MAXSD, &dst, &src);
  }

  //! @brief Memory Fence (SSE2).
  inline void mfence()
  {
    _emitInstruction(INST_MFENCE);
  }

  //! @brief Return Minimum Packed DP-FP Values (SSE2).
  inline void minpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MINPD, &dst, &src);
  }
  //! @brief Return Minimum Packed DP-FP Values (SSE2).
  inline void minpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MINPD, &dst, &src);
  }

  //! @brief Return Minimum Scalar DP-FP Value (SSE2).
  inline void minsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MINSD, &dst, &src);
  }
  //! @brief Return Minimum Scalar DP-FP Value (SSE2).
  inline void minsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MINSD, &dst, &src);
  }

  //! @brief Move Aligned DQWord (SSE2).
  inline void movdqa(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVDQA, &dst, &src);
  }
  //! @brief Move Aligned DQWord (SSE2).
  inline void movdqa(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVDQA, &dst, &src);
  }

  //! @brief Move Aligned DQWord (SSE2).
  inline void movdqa(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVDQA, &dst, &src);
  }

  //! @brief Move Unaligned Double Quadword (SSE2).
  inline void movdqu(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVDQU, &dst, &src);
  }
  //! @brief Move Unaligned Double Quadword (SSE2).
  inline void movdqu(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVDQU, &dst, &src);
  }

  //! @brief Move Unaligned Double Quadword (SSE2).
  inline void movdqu(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVDQU, &dst, &src);
  }

  //! @brief Extract Packed SP-FP Sign Mask (SSE2).
  inline void movmskps(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVMSKPS, &dst, &src);
  }

  //! @brief Extract Packed DP-FP Sign Mask (SSE2).
  inline void movmskpd(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVMSKPD, &dst, &src);
  }

  //! @brief Move Scalar Double-Precision FP Value (SSE2).
  inline void movsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVSD, &dst, &src);
  }
  //! @brief Move Scalar Double-Precision FP Value (SSE2).
  inline void movsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSD, &dst, &src);
  }

  //! @brief Move Scalar Double-Precision FP Value (SSE2).
  inline void movsd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVSD, &dst, &src);
  }

  //! @brief Move Aligned Packed Double-Precision FP Values (SSE2).
  inline void movapd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVAPD, &dst, &src);
  }

  //! @brief Move Aligned Packed Double-Precision FP Values (SSE2).
  inline void movapd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVAPD, &dst, &src);
  }

  //! @brief Move Aligned Packed Double-Precision FP Values (SSE2).
  inline void movapd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVAPD, &dst, &src);
  }

  //! @brief Move Quadword from XMM to MMX Technology Register (SSE2).
  inline void movdq2q(const MMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVDQ2Q, &dst, &src);
  }

  //! @brief Move Quadword from MMX Technology to XMM Register (SSE2).
  inline void movq2dq(const XMMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_MOVQ2DQ, &dst, &src);
  }

  //! @brief Move High Packed Double-Precision FP Value (SSE2).
  inline void movhpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVHPD, &dst, &src);
  }

  //! @brief Move High Packed Double-Precision FP Value (SSE2).
  inline void movhpd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVHPD, &dst, &src);
  }

  //! @brief Move Low Packed Double-Precision FP Value (SSE2).
  inline void movlpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVLPD, &dst, &src);
  }

  //! @brief Move Low Packed Double-Precision FP Value (SSE2).
  inline void movlpd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVLPD, &dst, &src);
  }

  //! @brief Store Double Quadword Using Non-Temporal Hint (SSE2).
  inline void movntdq(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVNTDQ, &dst, &src);
  }

  //! @brief Store Store DWORD Using Non-Temporal Hint (SSE2).
  inline void movnti(const Mem& dst, const GPReg& src)
  {
    _emitInstruction(INST_MOVNTI, &dst, &src);
  }

  //! @brief Store Packed Double-Precision FP Values Using Non-Temporal Hint (SSE2).
  inline void movntpd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVNTPD, &dst, &src);
  }

  //! @brief Move Unaligned Packed Double-Precision FP Values (SSE2).
  inline void movupd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVUPD, &dst, &src);
  }

  //! @brief Move Unaligned Packed Double-Precision FP Values (SSE2).
  inline void movupd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVUPD, &dst, &src);
  }

  //! @brief Move Unaligned Packed Double-Precision FP Values (SSE2).
  inline void movupd(const Mem& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVUPD, &dst, &src);
  }

  //! @brief Packed DP-FP Multiply (SSE2).
  inline void mulpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MULPD, &dst, &src);
  }
  //! @brief Packed DP-FP Multiply (SSE2).
  inline void mulpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MULPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Multiply (SSE2).
  inline void mulsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MULSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Multiply (SSE2).
  inline void mulsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MULSD, &dst, &src);
  }

  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void orpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ORPD, &dst, &src);
  }
  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void orpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ORPD, &dst, &src);
  }

  //! @brief Pack with Signed Saturation (SSE2).
  inline void packsswb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (SSE2).
  inline void packsswb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSWB, &dst, &src);
  }

  //! @brief Pack with Signed Saturation (SSE2).
  inline void packssdw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }
  //! @brief Pack with Signed Saturation (SSE2).
  inline void packssdw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKSSDW, &dst, &src);
  }

  //! @brief Pack with Unsigned Saturation (SSE2).
  inline void packuswb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }
  //! @brief Pack with Unsigned Saturation (SSE2).
  inline void packuswb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKUSWB, &dst, &src);
  }

  //! @brief Packed BYTE Add (SSE2).
  inline void paddb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }
  //! @brief Packed BYTE Add (SSE2).
  inline void paddb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDB, &dst, &src);
  }

  //! @brief Packed WORD Add (SSE2).
  inline void paddw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }
  //! @brief Packed WORD Add (SSE2).
  inline void paddw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDW, &dst, &src);
  }

  //! @brief Packed DWORD Add (SSE2).
  inline void paddd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }
  //! @brief Packed DWORD Add (SSE2).
  inline void paddd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDD, &dst, &src);
  }

  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }
  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }

  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }
  //! @brief Packed QWORD Add (SSE2).
  inline void paddq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDQ, &dst, &src);
  }

  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }
  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSB, &dst, &src);
  }

  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }
  //! @brief Packed Add with Saturation (SSE2).
  inline void paddsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDSW, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSB, &dst, &src);
  }

  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }
  //! @brief Packed Add Unsigned with Saturation (SSE2).
  inline void paddusw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PADDUSW, &dst, &src);
  }

  //! @brief Logical AND (SSE2).
  inline void pand(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }
  //! @brief Logical AND (SSE2).
  inline void pand(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PAND, &dst, &src);
  }

  //! @brief Logical AND Not (SSE2).
  inline void pandn(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }
  //! @brief Logical AND Not (SSE2).
  inline void pandn(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PANDN, &dst, &src);
  }

  //! @brief Spin Loop Hint (SSE2).
  inline void pause()
  {
    _emitInstruction(INST_PAUSE);
  }

  //! @brief Packed Average (SSE2).
  inline void pavgb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }
  //! @brief Packed Average (SSE2).
  inline void pavgb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGB, &dst, &src);
  }

  //! @brief Packed Average (SSE2).
  inline void pavgw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }
  //! @brief Packed Average (SSE2).
  inline void pavgw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PAVGW, &dst, &src);
  }

  //! @brief Packed Compare for Equal (BYTES) (SSE2).
  inline void pcmpeqb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }
  //! @brief Packed Compare for Equal (BYTES) (SSE2).
  inline void pcmpeqb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQB, &dst, &src);
  }

  //! @brief Packed Compare for Equal (WORDS) (SSE2).
  inline void pcmpeqw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }
  //! @brief Packed Compare for Equal (WORDS) (SSE2).
  inline void pcmpeqw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQW, &dst, &src);
  }

  //! @brief Packed Compare for Equal (DWORDS) (SSE2).
  inline void pcmpeqd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }
  //! @brief Packed Compare for Equal (DWORDS) (SSE2).
  inline void pcmpeqd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQD, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (BYTES) (SSE2).
  inline void pcmpgtb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (BYTES) (SSE2).
  inline void pcmpgtb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTB, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (WORDS) (SSE2).
  inline void pcmpgtw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (WORDS) (SSE2).
  inline void pcmpgtw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTW, &dst, &src);
  }

  //! @brief Packed Compare for Greater Than (DWORDS) (SSE2).
  inline void pcmpgtd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }
  //! @brief Packed Compare for Greater Than (DWORDS) (SSE2).
  inline void pcmpgtd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTD, &dst, &src);
  }

  //! @brief Packed Signed Integer Word Maximum (SSE2).
  inline void pmaxsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Maximum (SSE2).
  inline void pmaxsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Maximum (SSE2).
  inline void pmaxub(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Maximum (SSE2).
  inline void pmaxub(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUB, &dst, &src);
  }

  //! @brief Packed Signed Integer Word Minimum (SSE2).
  inline void pminsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }
  //! @brief Packed Signed Integer Word Minimum (SSE2).
  inline void pminsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSW, &dst, &src);
  }

  //! @brief Packed Unsigned Integer Byte Minimum (SSE2).
  inline void pminub(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }
  //! @brief Packed Unsigned Integer Byte Minimum (SSE2).
  inline void pminub(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUB, &dst, &src);
  }

  //! @brief Move Byte Mask (SSE2).
  inline void pmovmskb(const GPReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVMSKB, &dst, &src);
  }

  //! @brief Packed Multiply High (SSE2).
  inline void pmulhw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }
  //! @brief Packed Multiply High (SSE2).
  inline void pmulhw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHW, &dst, &src);
  }

  //! @brief Packed Multiply High Unsigned (SSE2).
  inline void pmulhuw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }
  //! @brief Packed Multiply High Unsigned (SSE2).
  inline void pmulhuw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHUW, &dst, &src);
  }

  //! @brief Packed Multiply Low (SSE2).
  inline void pmullw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }
  //! @brief Packed Multiply Low (SSE2).
  inline void pmullw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULLW, &dst, &src);
  }

  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }
  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }

  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }
  //! @brief Packed Multiply to QWORD (SSE2).
  inline void pmuludq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULUDQ, &dst, &src);
  }

  //! @brief Bitwise Logical OR (SSE2).
  inline void por(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }
  //! @brief Bitwise Logical OR (SSE2).
  inline void por(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_POR, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslld(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslld(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslld(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLD, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllq(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLQ, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }
  //! @brief Packed Shift Left Logical (SSE2).
  inline void psllw(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLW, &dst, &src);
  }

  //! @brief Packed Shift Left Logical (SSE2).
  inline void pslldq(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSLLDQ, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psrad(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psrad(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psrad(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAD, &dst, &src);
  }

  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psraw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psraw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }
  //! @brief Packed Shift Right Arithmetic (SSE2).
  inline void psraw(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRAW, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBB, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBW, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBD, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubq(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubq(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }

  //! @brief Packed Subtract (SSE2).
  inline void psubq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }
  //! @brief Packed Subtract (SSE2).
  inline void psubq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBQ, &dst, &src);
  }

  //! @brief Packed Multiply and Add (SSE2).
  inline void pmaddwd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }
  //! @brief Packed Multiply and Add (SSE2).
  inline void pmaddwd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDWD, &dst, &src);
  }

  //! @brief Shuffle Packed DWORDs (SSE2).
  inline void pshufd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFD, &dst, &src, &imm8);
  }
  //! @brief Shuffle Packed DWORDs (SSE2).
  inline void pshufd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFD, &dst, &src, &imm8);
  }

  //! @brief Shuffle Packed High Words (SSE2).
  inline void pshufhw(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFHW, &dst, &src, &imm8);
  }
  //! @brief Shuffle Packed High Words (SSE2).
  inline void pshufhw(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFHW, &dst, &src, &imm8);
  }

  //! @brief Shuffle Packed Low Words (SSE2).
  inline void pshuflw(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFLW, &dst, &src, &imm8);
  }
  //! @brief Shuffle Packed Low Words (SSE2).
  inline void pshuflw(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PSHUFLW, &dst, &src, &imm8);
  }

  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrld(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrld(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrld(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLD, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlq(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLQ, &dst, &src);
  }

  //! @brief DQWord Shift Right Logical (MMX).
  inline void psrldq(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLDQ, &dst, &src);
  }

  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }
  //! @brief Packed Shift Right Logical (SSE2).
  inline void psrlw(const XMMReg& dst, const Imm& src)
  {
    _emitInstruction(INST_PSRLW, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSB, &dst, &src);
  }

  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }
  //! @brief Packed Subtract with Saturation (SSE2).
  inline void psubsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBSW, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSB, &dst, &src);
  }

  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }
  //! @brief Packed Subtract with Unsigned Saturation (SSE2).
  inline void psubusw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSUBUSW, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhbw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhbw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHBW, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhwd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhwd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHWD, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhdq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhdq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHDQ, &dst, &src);
  }

  //! @brief Unpack High Data (SSE2).
  inline void punpckhqdq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKHQDQ, &dst, &src);
  }
  //! @brief Unpack High Data (SSE2).
  inline void punpckhqdq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKHQDQ, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpcklbw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpcklbw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLBW, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpcklwd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpcklwd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLWD, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpckldq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpckldq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLDQ, &dst, &src);
  }

  //! @brief Unpack Low Data (SSE2).
  inline void punpcklqdq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PUNPCKLQDQ, &dst, &src);
  }
  //! @brief Unpack Low Data (SSE2).
  inline void punpcklqdq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PUNPCKLQDQ, &dst, &src);
  }

  //! @brief Bitwise Exclusive OR (SSE2).
  inline void pxor(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }
  //! @brief Bitwise Exclusive OR (SSE2).
  inline void pxor(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PXOR, &dst, &src);
  }

  //! @brief Shuffle DP-FP (SSE2).
  inline void shufpd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPD, &dst, &src, &imm8);
  }
  //! @brief Shuffle DP-FP (SSE2).
  inline void shufpd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_SHUFPD, &dst, &src, &imm8);
  }

  //! @brief Compute Square Roots of Packed DP-FP Values (SSE2).
  inline void sqrtpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SQRTPD, &dst, &src);
  }
  //! @brief Compute Square Roots of Packed DP-FP Values (SSE2).
  inline void sqrtpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTPD, &dst, &src);
  }

  //! @brief Compute Square Root of Scalar DP-FP Value (SSE2).
  inline void sqrtsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SQRTSD, &dst, &src);
  }
  //! @brief Compute Square Root of Scalar DP-FP Value (SSE2).
  inline void sqrtsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SQRTSD, &dst, &src);
  }

  //! @brief Packed DP-FP Subtract (SSE2).
  inline void subpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SUBPD, &dst, &src);
  }
  //! @brief Packed DP-FP Subtract (SSE2).
  inline void subpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBPD, &dst, &src);
  }

  //! @brief Scalar DP-FP Subtract (SSE2).
  inline void subsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_SUBSD, &dst, &src);
  }
  //! @brief Scalar DP-FP Subtract (SSE2).
  inline void subsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_SUBSD, &dst, &src);
  }

  //! @brief Scalar Unordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void ucomisd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_UCOMISD, &dst, &src);
  }
  //! @brief Scalar Unordered DP-FP Compare and Set EFLAGS (SSE2).
  inline void ucomisd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_UCOMISD, &dst, &src);
  }

  //! @brief Unpack and Interleave High Packed Double-Precision FP Values (SSE2).
  inline void unpckhpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_UNPCKHPD, &dst, &src);
  }
  //! @brief Unpack and Interleave High Packed Double-Precision FP Values (SSE2).
  inline void unpckhpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKHPD, &dst, &src);
  }

  //! @brief Unpack and Interleave Low Packed Double-Precision FP Values (SSE2).
  inline void unpcklpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_UNPCKLPD, &dst, &src);
  }
  //! @brief Unpack and Interleave Low Packed Double-Precision FP Values (SSE2).
  inline void unpcklpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_UNPCKLPD, &dst, &src);
  }

  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void xorpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_XORPD, &dst, &src);
  }
  //! @brief Bit-wise Logical OR for DP-FP Data (SSE2).
  inline void xorpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_XORPD, &dst, &src);
  }

  // --------------------------------------------------------------------------
  // [SSE3]
  // --------------------------------------------------------------------------

  //! @brief Packed DP-FP Add/Subtract (SSE3).
  inline void addsubpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ADDSUBPD, &dst, &src);
  }
  //! @brief Packed DP-FP Add/Subtract (SSE3).
  inline void addsubpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSUBPD, &dst, &src);
  }

  //! @brief Packed SP-FP Add/Subtract (SSE3).
  inline void addsubps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_ADDSUBPS, &dst, &src);
  }
  //! @brief Packed SP-FP Add/Subtract (SSE3).
  inline void addsubps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_ADDSUBPS, &dst, &src);
  }

  //! @brief Store Integer with Truncation (SSE3).
  inline void fisttp(const Mem& dst)
  {
    _emitInstruction(INST_FISTTP, &dst);
  }

  //! @brief Packed DP-FP Horizontal Add (SSE3).
  inline void haddpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_HADDPD, &dst, &src);
  }
  //! @brief Packed DP-FP Horizontal Add (SSE3).
  inline void haddpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_HADDPD, &dst, &src);
  }

  //! @brief Packed SP-FP Horizontal Add (SSE3).
  inline void haddps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_HADDPS, &dst, &src);
  }
  //! @brief Packed SP-FP Horizontal Add (SSE3).
  inline void haddps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_HADDPS, &dst, &src);
  }

  //! @brief Packed DP-FP Horizontal Subtract (SSE3).
  inline void hsubpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_HSUBPD, &dst, &src);
  }
  //! @brief Packed DP-FP Horizontal Subtract (SSE3).
  inline void hsubpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_HSUBPD, &dst, &src);
  }

  //! @brief Packed SP-FP Horizontal Subtract (SSE3).
  inline void hsubps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_HSUBPS, &dst, &src);
  }
  //! @brief Packed SP-FP Horizontal Subtract (SSE3).
  inline void hsubps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_HSUBPS, &dst, &src);
  }

  //! @brief Load Unaligned Integer 128 Bits (SSE3).
  inline void lddqu(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_LDDQU, &dst, &src);
  }

  //! @brief Set Up Monitor Address (SSE3).
  inline void monitor()
  {
    _emitInstruction(INST_MONITOR);
  }

  //! @brief Move One DP-FP and Duplicate (SSE3).
  inline void movddup(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVDDUP, &dst, &src);
  }
  //! @brief Move One DP-FP and Duplicate (SSE3).
  inline void movddup(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVDDUP, &dst, &src);
  }

  //! @brief Move Packed SP-FP High and Duplicate (SSE3).
  inline void movshdup(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVSHDUP, &dst, &src);
  }
  //! @brief Move Packed SP-FP High and Duplicate (SSE3).
  inline void movshdup(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSHDUP, &dst, &src);
  }

  //! @brief Move Packed SP-FP Low and Duplicate (SSE3).
  inline void movsldup(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_MOVSLDUP, &dst, &src);
  }
  //! @brief Move Packed SP-FP Low and Duplicate (SSE3).
  inline void movsldup(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVSLDUP, &dst, &src);
  }

  //! @brief Monitor Wait (SSE3).
  inline void mwait()
  {
    _emitInstruction(INST_MWAIT);
  }

  // --------------------------------------------------------------------------
  // [SSSE3]
  // --------------------------------------------------------------------------

  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNB, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGNW, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }

  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }
  //! @brief Packed SIGN (SSSE3).
  inline void psignd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSIGND, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDW, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }

  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }
  //! @brief Packed Horizontal Add (SSSE3).
  inline void phaddd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDD, &dst, &src);
  }

  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }
  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }

  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }
  //! @brief Packed Horizontal Add and Saturate (SSSE3).
  inline void phaddsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHADDSW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract (SSSE3).
  inline void phsubd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBD, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }

  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }
  //! @brief Packed Horizontal Subtract and Saturate (SSSE3).
  inline void phsubsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHSUBSW, &dst, &src);
  }

  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }
  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }

  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }
  //! @brief Multiply and Add Packed Signed and Unsigned Bytes (SSSE3).
  inline void pmaddubsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMADDUBSW, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSB, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSW, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }

  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }
  //! @brief Packed Absolute Value (SSSE3).
  inline void pabsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PABSD, &dst, &src);
  }

  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }
  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }

  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }
  //! @brief Packed Multiply High with Round and Scale (SSSE3).
  inline void pmulhrsw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULHRSW, &dst, &src);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const MMReg& dst, const MMReg& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const MMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void pshufb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PSHUFB, &dst, &src);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const MMReg& dst, const MMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const MMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }

  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }
  //! @brief Packed Shuffle Bytes (SSSE3).
  inline void palignr(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PALIGNR, &dst, &src, &imm8);
  }

  // --------------------------------------------------------------------------
  // [SSE4.1]
  // --------------------------------------------------------------------------

  //! @brief Blend Packed DP-FP Values (SSE4.1).
  inline void blendpd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPD, &dst, &src, &imm8);
  }
  //! @brief Blend Packed DP-FP Values (SSE4.1).
  inline void blendpd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPD, &dst, &src, &imm8);
  }

  //! @brief Blend Packed SP-FP Values (SSE4.1).
  inline void blendps(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPS, &dst, &src, &imm8);
  }
  //! @brief Blend Packed SP-FP Values (SSE4.1).
  inline void blendps(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_BLENDPS, &dst, &src, &imm8);
  }

  //! @brief Variable Blend Packed DP-FP Values (SSE4.1).
  inline void blendvpd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_BLENDVPD, &dst, &src);
  }
  //! @brief Variable Blend Packed DP-FP Values (SSE4.1).
  inline void blendvpd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_BLENDVPD, &dst, &src);
  }

  //! @brief Variable Blend Packed SP-FP Values (SSE4.1).
  inline void blendvps(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_BLENDVPS, &dst, &src);
  }
  //! @brief Variable Blend Packed SP-FP Values (SSE4.1).
  inline void blendvps(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_BLENDVPS, &dst, &src);
  }

  //! @brief Dot Product of Packed DP-FP Values (SSE4.1).
  inline void dppd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPD, &dst, &src, &imm8);
  }
  //! @brief Dot Product of Packed DP-FP Values (SSE4.1).
  inline void dppd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPD, &dst, &src, &imm8);
  }

  //! @brief Dot Product of Packed SP-FP Values (SSE4.1).
  inline void dpps(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPS, &dst, &src, &imm8);
  }
  //! @brief Dot Product of Packed SP-FP Values (SSE4.1).
  inline void dpps(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_DPPS, &dst, &src, &imm8);
  }

  //! @brief Extract Packed SP-FP Value (SSE4.1).
  inline void extractps(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_EXTRACTPS, &dst, &src, &imm8);
  }
  //! @brief Extract Packed SP-FP Value (SSE4.1).
  inline void extractps(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_EXTRACTPS, &dst, &src, &imm8);
  }

  //! @brief Load Double Quadword Non-Temporal Aligned Hint (SSE4.1).
  inline void movntdqa(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_MOVNTDQA, &dst, &src);
  }

  //! @brief Compute Multiple Packed Sums of Absolute Difference (SSE4.1).
  inline void mpsadbw(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_MPSADBW, &dst, &src, &imm8);
  }
  //! @brief Compute Multiple Packed Sums of Absolute Difference (SSE4.1).
  inline void mpsadbw(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_MPSADBW, &dst, &src, &imm8);
  }

  //! @brief Pack with Unsigned Saturation (SSE4.1).
  inline void packusdw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PACKUSDW, &dst, &src);
  }
  //! @brief Pack with Unsigned Saturation (SSE4.1).
  inline void packusdw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PACKUSDW, &dst, &src);
  }

  //! @brief Variable Blend Packed Bytes (SSE4.1).
  inline void pblendvb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PBLENDVB, &dst, &src);
  }
  //! @brief Variable Blend Packed Bytes (SSE4.1).
  inline void pblendvb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PBLENDVB, &dst, &src);
  }

  //! @brief Blend Packed Words (SSE4.1).
  inline void pblendw(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PBLENDW, &dst, &src, &imm8);
  }
  //! @brief Blend Packed Words (SSE4.1).
  inline void pblendw(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PBLENDW, &dst, &src, &imm8);
  }

  //! @brief Compare Packed Qword Data for Equal (SSE4.1).
  inline void pcmpeqq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPEQQ, &dst, &src);
  }
  //! @brief Compare Packed Qword Data for Equal (SSE4.1).
  inline void pcmpeqq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPEQQ, &dst, &src);
  }

  //! @brief Extract Byte (SSE4.1).
  inline void pextrb(const GPReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRB, &dst, &src, &imm8);
  }
  //! @brief Extract Byte (SSE4.1).
  inline void pextrb(const Mem& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRB, &dst, &src, &imm8);
  }

  //! @brief Extract Dword (SSE4.1).
  inline void pextrd(const GPReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRD, &dst, &src, &imm8);
  }
  //! @brief Extract Dword (SSE4.1).
  inline void pextrd(const Mem& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRD, &dst, &src, &imm8);
  }

  //! @brief Extract Dword (SSE4.1).
  inline void pextrq(const GPReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRQ, &dst, &src, &imm8);
  }
  //! @brief Extract Dword (SSE4.1).
  inline void pextrq(const Mem& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRQ, &dst, &src, &imm8);
  }

  //! @brief Extract Word (SSE4.1).
  inline void pextrw(const GPReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRW, &dst, &src, &imm8);
  }
  //! @brief Extract Word (SSE4.1).
  inline void pextrw(const Mem& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PEXTRW, &dst, &src, &imm8);
  }

  //! @brief Packed Horizontal Word Minimum (SSE4.1).
  inline void phminposuw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PHMINPOSUW, &dst, &src);
  }
  //! @brief Packed Horizontal Word Minimum (SSE4.1).
  inline void phminposuw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PHMINPOSUW, &dst, &src);
  }

  //! @brief Insert Byte (SSE4.1).
  inline void pinsrb(const XMMReg& dst, const GPReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRB, &dst, &src, &imm8);
  }
  //! @brief Insert Byte (SSE4.1).
  inline void pinsrb(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRB, &dst, &src, &imm8);
  }

  //! @brief Insert Dword (SSE4.1).
  inline void pinsrd(const XMMReg& dst, const GPReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRD, &dst, &src, &imm8);
  }
  //! @brief Insert Dword (SSE4.1).
  inline void pinsrd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRD, &dst, &src, &imm8);
  }

  //! @brief Insert Dword (SSE4.1).
  inline void pinsrq(const XMMReg& dst, const GPReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRQ, &dst, &src, &imm8);
  }
  //! @brief Insert Dword (SSE4.1).
  inline void pinsrq(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRQ, &dst, &src, &imm8);
  }

  //! @brief Insert Word (SSE2).
  inline void pinsrw(const XMMReg& dst, const GPReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }
  //! @brief Insert Word (SSE2).
  inline void pinsrw(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PINSRW, &dst, &src, &imm8);
  }

  //! @brief Maximum of Packed Word Integers (SSE4.1).
  inline void pmaxuw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMAXUW, &dst, &src);
  }
  //! @brief Maximum of Packed Word Integers (SSE4.1).
  inline void pmaxuw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUW, &dst, &src);
  }

  //! @brief Maximum of Packed Signed Byte Integers (SSE4.1).
  inline void pmaxsb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMAXSB, &dst, &src);
  }
  //! @brief Maximum of Packed Signed Byte Integers (SSE4.1).
  inline void pmaxsb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSB, &dst, &src);
  }

  //! @brief Maximum of Packed Signed Dword Integers (SSE4.1).
  inline void pmaxsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMAXSD, &dst, &src);
  }
  //! @brief Maximum of Packed Signed Dword Integers (SSE4.1).
  inline void pmaxsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXSD, &dst, &src);
  }

  //! @brief Maximum of Packed Unsigned Dword Integers (SSE4.1).
  inline void pmaxud(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMAXUD, &dst, &src);
  }
  //! @brief Maximum of Packed Unsigned Dword Integers (SSE4.1).
  inline void pmaxud(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMAXUD, &dst, &src);
  }

  //! @brief Minimum of Packed Signed Byte Integers (SSE4.1).
  inline void pminsb(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMINSB, &dst, &src);
  }
  //! @brief Minimum of Packed Signed Byte Integers (SSE4.1).
  inline void pminsb(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSB, &dst, &src);
  }

  //! @brief Minimum of Packed Word Integers (SSE4.1).
  inline void pminuw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMINUW, &dst, &src);
  }
  //! @brief Minimum of Packed Word Integers (SSE4.1).
  inline void pminuw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUW, &dst, &src);
  }

  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminud(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMINUD, &dst, &src);
  }
  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminud(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINUD, &dst, &src);
  }

  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminsd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMINSD, &dst, &src);
  }
  //! @brief Minimum of Packed Dword Integers (SSE4.1).
  inline void pminsd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMINSD, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVSXBW, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXBW, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVSXBD, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXBD, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVSXBQ, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxbq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXBQ, &dst, &src);
  }

  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxwd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVSXWD, &dst, &src);
  }
  //! @brief Packed Move with Sign Extend (SSE4.1).
  inline void pmovsxwd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXWD, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovsxwq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVSXWQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovsxwq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXWQ, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovsxdq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVSXDQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovsxdq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVSXDQ, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbw(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVZXBW, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbw(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXBW, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVZXBD, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXBD, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVZXBQ, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxbq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXBQ, &dst, &src);
  }

  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxwd(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVZXWD, &dst, &src);
  }
  //! @brief Packed Move with Zero Extend (SSE4.1).
  inline void pmovzxwd(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXWD, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovzxwq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVZXWQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovzxwq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXWQ, &dst, &src);
  }

  //! @brief (SSE4.1).
  inline void pmovzxdq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMOVZXDQ, &dst, &src);
  }
  //! @brief (SSE4.1).
  inline void pmovzxdq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMOVZXDQ, &dst, &src);
  }

  //! @brief Multiply Packed Signed Dword Integers (SSE4.1).
  inline void pmuldq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULDQ, &dst, &src);
  }
  //! @brief Multiply Packed Signed Dword Integers (SSE4.1).
  inline void pmuldq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULDQ, &dst, &src);
  }

  //! @brief Multiply Packed Signed Integers and Store Low Result (SSE4.1).
  inline void pmulld(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PMULLD, &dst, &src);
  }
  //! @brief Multiply Packed Signed Integers and Store Low Result (SSE4.1).
  inline void pmulld(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PMULLD, &dst, &src);
  }

  //! @brief Logical Compare (SSE4.1).
  inline void ptest(const XMMReg& op1, const XMMReg& op2)
  {
    _emitInstruction(INST_PTEST, &op1, &op2);
  }
  //! @brief Logical Compare (SSE4.1).
  inline void ptest(const XMMReg& op1, const Mem& op2)
  {
    _emitInstruction(INST_PTEST, &op1, &op2);
  }

  //! Round Packed SP-FP Values @brief (SSE4.1).
  inline void roundps(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPS, &dst, &src, &imm8);
  }
  //! Round Packed SP-FP Values @brief (SSE4.1).
  inline void roundps(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPS, &dst, &src, &imm8);
  }

  //! @brief Round Scalar SP-FP Values (SSE4.1).
  inline void roundss(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSS, &dst, &src, &imm8);
  }
  //! @brief Round Scalar SP-FP Values (SSE4.1).
  inline void roundss(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSS, &dst, &src, &imm8);
  }

  //! @brief Round Packed DP-FP Values (SSE4.1).
  inline void roundpd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPD, &dst, &src, &imm8);
  }
  //! @brief Round Packed DP-FP Values (SSE4.1).
  inline void roundpd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDPD, &dst, &src, &imm8);
  }

  //! @brief Round Scalar DP-FP Values (SSE4.1).
  inline void roundsd(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSD, &dst, &src, &imm8);
  }
  //! @brief Round Scalar DP-FP Values (SSE4.1).
  inline void roundsd(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_ROUNDSD, &dst, &src, &imm8);
  }

  // --------------------------------------------------------------------------
  // [SSE4.2]
  // --------------------------------------------------------------------------

  //! @brief Accumulate CRC32 Value (polynomial 0x11EDC6F41) (SSE4.2).
  inline void crc32(const GPReg& dst, const GPReg& src)
  {
    ASMJIT_ASSERT(dst.isRegType(REG_TYPE_GPD) || dst.isRegType(REG_TYPE_GPQ));
    _emitInstruction(INST_CRC32, &dst, &src);
  }
  //! @brief Accumulate CRC32 Value (polynomial 0x11EDC6F41) (SSE4.2).
  inline void crc32(const GPReg& dst, const Mem& src)
  {
    ASMJIT_ASSERT(dst.isRegType(REG_TYPE_GPD) || dst.isRegType(REG_TYPE_GPQ));
    _emitInstruction(INST_CRC32, &dst, &src);
  }

  //! @brief Packed Compare Explicit Length Strings, Return Index (SSE4.2).
  inline void pcmpestri(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRI, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Explicit Length Strings, Return Index (SSE4.2).
  inline void pcmpestri(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRI, &dst, &src, &imm8);
  }

  //! @brief Packed Compare Explicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpestrm(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRM, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Explicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpestrm(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPESTRM, &dst, &src, &imm8);
  }

  //! @brief Packed Compare Implicit Length Strings, Return Index (SSE4.2).
  inline void pcmpistri(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRI, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Implicit Length Strings, Return Index (SSE4.2).
  inline void pcmpistri(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRI, &dst, &src, &imm8);
  }

  //! @brief Packed Compare Implicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpistrm(const XMMReg& dst, const XMMReg& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRM, &dst, &src, &imm8);
  }
  //! @brief Packed Compare Implicit Length Strings, Return Mask (SSE4.2).
  inline void pcmpistrm(const XMMReg& dst, const Mem& src, const Imm& imm8)
  {
    _emitInstruction(INST_PCMPISTRM, &dst, &src, &imm8);
  }

  //! @brief Compare Packed Data for Greater Than (SSE4.2).
  inline void pcmpgtq(const XMMReg& dst, const XMMReg& src)
  {
    _emitInstruction(INST_PCMPGTQ, &dst, &src);
  }
  //! @brief Compare Packed Data for Greater Than (SSE4.2).
  inline void pcmpgtq(const XMMReg& dst, const Mem& src)
  {
    _emitInstruction(INST_PCMPGTQ, &dst, &src);
  }

  //! @brief Return the Count of Number of Bits Set to 1 (SSE4.2).
  inline void popcnt(const GPReg& dst, const GPReg& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    ASMJIT_ASSERT(src.getRegType() == dst.getRegType());
    _emitInstruction(INST_POPCNT, &dst, &src);
  }
  //! @brief Return the Count of Number of Bits Set to 1 (SSE4.2).
  inline void popcnt(const GPReg& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_POPCNT, &dst, &src);
  }

  // -------------------------------------------------------------------------
  // [AMD only]
  // -------------------------------------------------------------------------

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

  // -------------------------------------------------------------------------
  // [Intel only]
  // -------------------------------------------------------------------------

  //! @brief Move Data After Swapping Bytes (SSE3 - Intel Atom).
  inline void movbe(const GPReg& dst, const Mem& src)
  {
    ASMJIT_ASSERT(!dst.isGPB());
    _emitInstruction(INST_MOVBE, &dst, &src);
  }

  //! @brief Move Data After Swapping Bytes (SSE3 - Intel Atom).
  inline void movbe(const Mem& dst, const GPReg& src)
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
  //!
  //! @sa @c EMIT_OPTION_LOCK_PREFIX.
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
// [AsmJit::Assembler]
// ============================================================================

//! @brief Assembler - low level code generation.
//!
//! @c Assembler is the main class in AsmJit for generating low level x86/x64
//! binary stream. It creates internal buffer where opcodes are stored and
//! contains methods that mimics x86/x64 assembler instructions. Code generation
//! should be safe, because basic type-checks are done by the C++ compiler. It's
//! nearly impossible to create invalid instruction (for example
//! <code>mov [eax], [eax]</code> that will not be detected at compile time.
//!
//! Each call to assembler intrinsics directly emits instruction to internal
//! binary stream. Instruction emitting also contains runtime checks so it's
//! impossible to create instruction that is not valid.
//!
//! @c AsmJit::Assembler contains internal buffer where all emitted
//! instructions are stored. Look at @c AsmJit::Buffer for buffer
//! implementation. To generate and allocate memory for function use
//! @c AsmJit::Assembler::make() method that will allocate memory using
//! provided memory manager ( see @c AsmJit::MemoryManager::getGlobal() ) and
//! relocates code to provided address. If you want to create your function
//! manually, you should look at @c AsmJit::VirtualMemory and use
//! @c AsmJit::Assembler::relocCode() method to relocate emitted code into
//! provided memory location. You can also take emitted buffer by @c take()
//! method to do something else with it. If you take buffer, you must free it
//! manually by @c ASMJIT_FREE() macro.
//!
//! @note Always use this class and never use @c AssemblerCore or
//! @c AssemblerIntrinsics classes directly.
//!
//! @section AsmJit_Assembler_CodeGeneration Code Generation
//!
//! To generate code is only needed to create instance of @c AsmJit::Assembler
//! and to use intrinsics. See example how to do that:
//!
//! @code
//! // Use AsmJit namespace.
//! using namespace AsmJit;
//!
//! // Create Assembler instance.
//! Assembler a;
//!
//! // Prolog.
//! a.push(ebp);
//! a.mov(ebp, esp);
//!
//! // Mov 1024 to EAX, EAX is also return value.
//! a.mov(eax, imm(1024));
//!
//! // Epilog.
//! a.mov(esp, ebp);
//! a.pop(ebp);
//!
//! // Return.
//! a.ret();
//! @endcode
//!
//! You can see that syntax is very close to Intel one. Only difference is that
//! you are calling functions that emits the binary code for you. All registers
//! are in @c AsmJit namespace, so it's very comfortable to use it (look at
//! first line). There is also used method @c AsmJit::imm() to create an
//! immediate value. Use @c AsmJit::uimm() to create unsigned immediate value.
//!
//! There is also possibility to use memory addresses and immediates. To build
//! memory address use @c ptr(), @c byte_ptr(), @c word_ptr(), @c dword_ptr()
//! or other friend methods. In most cases you needs only @c ptr() method, but
//! there are instructions where you must specify address size,
//!
//! for example (a is @c AsmJit::Assembler instance):
//!
//! @code
//! a.mov(ptr(eax), imm(0));                   // mov ptr [eax], 0
//! a.mov(ptr(eax), edx);                      // mov ptr [eax], edx
//! @endcode
//!
//! But it's also possible to create complex addresses:
//!
//! @code
//! // eax + ecx*x addresses
//! a.mov(ptr(eax, ecx, TIMES_1), imm(0));     // mov ptr [eax + ecx], 0
//! a.mov(ptr(eax, ecx, TIMES_2), imm(0));     // mov ptr [eax + ecx * 2], 0
//! a.mov(ptr(eax, ecx, TIMES_4), imm(0));     // mov ptr [eax + ecx * 4], 0
//! a.mov(ptr(eax, ecx, TIMES_8), imm(0));     // mov ptr [eax + ecx * 8], 0
//! // eax + ecx*x + disp addresses
//! a.mov(ptr(eax, ecx, TIMES_1,  4), imm(0)); // mov ptr [eax + ecx     +  4], 0
//! a.mov(ptr(eax, ecx, TIMES_2,  8), imm(0)); // mov ptr [eax + ecx * 2 +  8], 0
//! a.mov(ptr(eax, ecx, TIMES_4, 12), imm(0)); // mov ptr [eax + ecx * 4 + 12], 0
//! a.mov(ptr(eax, ecx, TIMES_8, 16), imm(0)); // mov ptr [eax + ecx * 8 + 16], 0
//! @endcode
//!
//! All addresses shown are using @c AsmJit::ptr() to make memory operand.
//! Some assembler instructions (single operand ones) needs to specify memory
//! operand size. For example calling <code>a.inc(ptr(eax))</code> can't be
//! used. @c AsmJit::Assembler::inc(), @c AsmJit::Assembler::dec() and similar
//! instructions can't be serialized without specifying how bytes they are
//! operating on. See next code how assembler works:
//!
//! @code
//! // [byte] address
//! a.inc(byte_ptr(eax));                      // inc byte ptr [eax]
//! a.dec(byte_ptr(eax));                      // dec byte ptr [eax]
//! // [word] address
//! a.inc(word_ptr(eax));                      // inc word ptr [eax]
//! a.dec(word_ptr(eax));                      // dec word ptr [eax]
//! // [dword] address
//! a.inc(dword_ptr(eax));                     // inc dword ptr [eax]
//! a.dec(dword_ptr(eax));                     // dec dword ptr [eax]
//! @endcode
//!
//! @section AsmJit_Assembler_CallingJitCode Calling JIT Code
//!
//! While you are over from emitting instructions, you can make your function
//! using @c AsmJit::Assembler::make() method. This method will use memory
//! manager to allocate virtual memory and relocates generated code to it. For
//! memory allocation is used global memory manager by default and memory is
//! freeable, but of course this default behavior can be overridden specifying
//! your memory manager and allocation type. If you want to do with code
//! something else you can always override make() method and do what you want.
//!
//! You can get size of generated code by @c getCodeSize() or @c getOffset()
//! methods. These methods returns you code size (or more precisely current code 
//! offset) in bytes. Use takeCode() to take internal buffer (all pointers in
//! @c AsmJit::Assembler instance will be zeroed and current buffer returned)
//! to use it. If you don't take it,  @c AsmJit::Assembler destructor will
//! free it automatically. To alloc and run code manually don't use
//! @c malloc()'ed memory, but instead use @c AsmJit::VirtualMemory::alloc()
//! to get memory for executing (specify @c canExecute to @c true) or
//! @c AsmJit::MemoryManager that provides more effective and comfortable way
//! to allocate virtual memory.
//!
//! See next example how to allocate memory where you can execute code created
//! by @c AsmJit::Assembler:
//!
//! @code
//! using namespace AsmJit;
//!
//! Assembler a;
//!
//! // ... your code generation
//!
//! // your function prototype
//! typedef void (*MyFn)();
//!
//! // make your function
//! MyFn fn = function_cast<MyFn>(a.make());
//!
//! // call your function
//! fn();
//!
//! // If you don't need your function again, free it.
//! MemoryManager::getGlobal()->free(fn);
//! @endcode
//!
//! There is also low level alternative how to allocate virtual memory and
//! relocate code to it:
//!
//! @code
//! using namespace AsmJit;
//!
//! Assembler a;
//! // Your code generation ...
//!
//! // Your function prototype.
//! typedef void (*MyFn)();
//!
//! // Alloc memory for your function.
//! MyFn fn = function_cast<MyFn>(
//!   MemoryManager::getGlobal()->alloc(a.getCodeSize());
//!
//! // Relocate the code (will make the function).
//! a.relocCode(fn);
//!
//! // Call the generated function.
//! fn();
//!
//! // If you don't need your function anymore, it should be freed.
//! MemoryManager::getGlobal()->free(fn);
//! @endcode
//!
//! @c note This was very primitive example how to call generated code.
//! In real production code you will never alloc and free code for one run,
//! you will usually use generated code many times.
//!
//! @section AsmJit_Assembler_Labels Labels
//!
//! While generating assembler code, you will usually need to create complex
//! code with labels. Labels are fully supported and you can call @c jmp or
//! @c je (and similar) instructions to initialized or yet uninitialized label.
//! Each label expects to be bound into offset. To bind label to specific
//! offset, use @c bind() method.
//!
//! See next example that contains complete code that creates simple memory
//! copy function (in DWORD entities).
//!
//! @code
//! // Example: Usage of Label (32-bit code).
//! //
//! // Create simple DWORD memory copy function:
//! // ASMJIT_STDCALL void copy32(uint32_t* dst, const uint32_t* src, sysuint_t count);
//! using namespace AsmJit;
//!
//! // Assembler instance.
//! Assembler a;
//!
//! // Constants.
//! const int arg_offset = 8; // Arguments offset (STDCALL EBP).
//! const int arg_size = 12;  // Arguments size.
//!
//! // Labels.
//! Label L_Loop = a.newLabel();
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
//! // Return: STDCALL convention is to pop stack in called function.
//! a.ret(arg_size);
//! @endcode
//!
//! If you need more abstraction for generating assembler code and you want
//! to hide calling conventions between 32-bit and 64-bit operating systems,
//! look at @c Compiler class that is designed for higher level code
//! generation.
//!
//! @section AsmJit_Assembler_AdvancedCodeGeneration Advanced Code Generation
//!
//! This section describes some advanced generation features of @c Assembler
//! class which can be simply overlooked. The first thing that is very likely
//! needed is generic register support. In previous example the named registers
//! were used. AsmJit contains functions which can convert register index into
//! operand and back.
//!
//! Let's define function which can be used to generate some abstract code:
//!
//! @code
//! // Simple function that generates dword copy.
//! void genCopyDWord(
//!   Assembler& a,
//!   const GPReg& dst, const GPReg& src, const GPReg& tmp)
//! {
//!   a.mov(tmp, dword_ptr(src));
//!   a.mov(dword_ptr(dst), tmp);
//! }
//! @endcode
//!
//! This function can be called like <code>genCopyDWord(a, edi, esi, ebx)</code>
//! or by using existing @ref GPReg instances. This abstraction allows to join
//! more code sections together without rewriting each to use specific registers.
//! You need to take care only about implicit registers which may be used by 
//! several instructions (like mul, imul, div, idiv, shifting, etc...).
//!
//! Next, more advanced, but often needed technique is that you can build your
//! own registers allocator. X86 architecture contains 8 general purpose registers,
//! 8 MMX (MM) registers and 8 SSE (XMM) registers. The X64 (AMD64) architecture
//! extends count of general purpose registers and SSE2 registers to 16. Use the
//! @c REG_NUM_BASE constant to get count of GP or XMM registers or @c REG_NUM_GP,
//! @c REG_NUM_MM and @c REG_NUM_XMM constants individually.
//!
//! To build register from index (value from 0 inclusive to REG_NUM_XXX 
//! exclusive) use @ref gpd(), @ref gpq() or @ref gpn() functions. To create
//! a 8 or 16-bit register use @ref gpw(), @ref gpb_lo() or @ref gpb_hi(). 
//! To create other registers there are similar methods @ref mm(), @ref xmm() and
//! @ref st().
//!
//! So our function call to genCopyDWord can be also used like this:
//!
//! @code
//! genCopyDWord(a, gpd(REG_INDEX_EDI), gpd(REG_INDEX_ESI), gpd(REG_INDEX_EBX));
//! @endcode
//!
//! REG_INDEX_XXX are constants defined by @ref REG_INDEX enum. You can use your
//! own register allocator (or register slot manager) to alloc / free registers
//! so REG_INDEX_XXX values can be replaced by your variables (0 to REG_NUM_XXX-1).
//!
//! @sa @c AssemblerCore, @c AssemblerIntrinsics, @c Operand, @c Compiler.
struct ASMJIT_API Assembler : public AssemblerIntrinsics
{
  Assembler(CodeGenerator* codeGenerator = NULL) ASMJIT_NOTHROW;
  virtual ~Assembler() ASMJIT_NOTHROW;
};

//! @}

#pragma warning(default : 4701 4389 4245 4244 4100 4189)


} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_ASSEMBLERX86X64_H
