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
#include "CpuInfo.h"
#include "Defs.h"
#include "Logger.h"
#include "MemoryManager.h"
#include "Platform.h"
#include "Util_p.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// A little bit C++.
#include <new>

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

#pragma warning( disable : 4701 4389 4245 4244 4100 4189)


#if defined(ASMJIT_X64)

// ============================================================================
// [AsmJit::TrampolineWriter]
// ============================================================================

//! @brief Class used to determine size of trampoline and as trampoline writer.
struct ASMJIT_HIDDEN TrampolineWriter
{
  // Size of trampoline
  enum {
    TRAMPOLINE_JMP = 6,
    TRAMPOLINE_ADDR = sizeof(sysint_t),

    TRAMPOLINE_SIZE = TRAMPOLINE_JMP + TRAMPOLINE_ADDR
  };

  // Write trampoline into code at address @a code that will jump to @a target.
  static void writeTrampoline(uint8_t* code, void* target)
  {
    // Jmp.
    code[0] = 0xFF;
    // ModM (RIP addressing).
    code[1] = 0x25;
    // Offset (zero).
    ((uint32_t*)(code + 2))[0] = 0;
    // Absolute address.
    ((sysuint_t*)(code + TRAMPOLINE_JMP))[0] = (sysuint_t)target;
  }
};

#endif // ASMJIT_X64

// ============================================================================
// [AsmJit::AssemblerCore - Construction / Destruction]
// ============================================================================

AssemblerCore::AssemblerCore(CodeGenerator* codeGenerator) ASMJIT_NOTHROW :
  _zone(16384 - sizeof(Zone::Chunk) - 32),
  _codeGenerator(codeGenerator != NULL ? codeGenerator : CodeGenerator::getGlobal()),
  _logger(NULL),
  _error(0),
  _properties((1 << PROPERTY_OPTIMIZE_ALIGN)),
  _emitOptions(0),
  _buffer(32), // Max instruction length is 15, but we can align up to 32 bytes.
  _trampolineSize(0),
  _unusedLinks(NULL),
  _comment(NULL)
{
}

AssemblerCore::~AssemblerCore() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::AssemblerCore - Logging]
// ============================================================================

void AssemblerCore::setLogger(Logger* logger) ASMJIT_NOTHROW
{
  _logger = logger;
}

// ============================================================================
// [AsmJit::AssemblerCore - Error Handling]
// ============================================================================

void AssemblerCore::setError(uint32_t error) ASMJIT_NOTHROW
{
  _error = error;
  if (_error == ERROR_NONE) return;

  if (_logger)
  {
    _logger->logFormat("*** ASSEMBLER ERROR: %s (%u).\n",
      getErrorString(error),
      (unsigned int)error);
  }
}

// ============================================================================
// [AsmJit::AssemblerCore - Properties]
// ============================================================================

uint32_t AssemblerCore::getProperty(uint32_t propertyId)
{
  return (_properties & (1 << propertyId)) != 0;
}

void AssemblerCore::setProperty(uint32_t propertyId, uint32_t value)
{
  if (value)
    _properties |= (1 << propertyId);
  else
    _properties &= ~(1 << propertyId);
}

// ============================================================================
// [AsmJit::AssemblerCore - Buffer]
// ============================================================================

void AssemblerCore::clear() ASMJIT_NOTHROW
{
  _buffer.clear();
  _relocData.clear();
  _zone.clear();

  if (_error) setError(ERROR_NONE);
}

void AssemblerCore::free() ASMJIT_NOTHROW
{
  _zone.freeAll();
  _buffer.free();
  _relocData.free();

  if (_error) setError(ERROR_NONE);
}

uint8_t* AssemblerCore::takeCode() ASMJIT_NOTHROW
{
  uint8_t* code = _buffer.take();
  _relocData.clear();
  _zone.clear();

  if (_error) setError(ERROR_NONE);
  return code;
}

// ============================================================================
// [AsmJit::AssemblerCore - Stream Setters / Getters]
// ============================================================================

void AssemblerCore::setVarAt(sysint_t pos, sysint_t i, uint8_t isUnsigned, uint32_t size) ASMJIT_NOTHROW
{
  if (size == 1 && !isUnsigned) setByteAt (pos, (int8_t  )i);
  else if (size == 1 &&  isUnsigned) setByteAt (pos, (uint8_t )i);
  else if (size == 2 && !isUnsigned) setWordAt (pos, (int16_t )i);
  else if (size == 2 &&  isUnsigned) setWordAt (pos, (uint16_t)i);
  else if (size == 4 && !isUnsigned) setDWordAt(pos, (int32_t )i);
  else if (size == 4 &&  isUnsigned) setDWordAt(pos, (uint32_t)i);
#if defined(ASMJIT_X64)
  else if (size == 8 && !isUnsigned) setQWordAt(pos, (int64_t )i);
  else if (size == 8 &&  isUnsigned) setQWordAt(pos, (uint64_t)i);
#endif // ASMJIT_X64
  else
    ASMJIT_ASSERT(0);
}

// ============================================================================
// [AsmJit::AssemblerCore - Assembler Emitters]
// ============================================================================

bool AssemblerCore::canEmit() ASMJIT_NOTHROW
{
  // If there is an error, we can't emit another instruction until last error
  // is cleared by calling @c setError(ERROR_NONE). If something caused an error
  // while generating code it's probably fatal in all cases. You can't use 
  // generated code, because you are not sure about its status.
  if (_error) return false;

  // The ensureSpace() method returns true on success and false on failure. We
  // are catching return value and setting error code here.
  if (ensureSpace()) return true;

  // If we are here, there is memory allocation error. Note that this is HEAP
  // allocation error, virtual allocation error can be caused only by
  // AsmJit::VirtualMemory class!
  setError(ERROR_NO_HEAP_MEMORY);
  return false;
}

void AssemblerCore::_emitSegmentPrefix(const Operand& rm) ASMJIT_NOTHROW
{
  static const uint8_t segmentPrefixCode[6] =
  {
    0x26, // ES
    0x2E, // SS
    0x36, // SS
    0x3E, // DS
    0x64, // FS
    0x65  // GS
  };

  uint32_t segmentPrefix;
  
  if (!rm.isMem())
    return;

  if ((segmentPrefix = reinterpret_cast<const Mem&>(rm).getSegmentPrefix()) >= REG_NUM_SEGMENT)
    return;

  _emitByte(segmentPrefixCode[segmentPrefix]);
}

void AssemblerCore::_emitModM(
  uint8_t opReg, const Mem& mem, sysint_t immSize) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(mem.getType() == OPERAND_MEM);

  uint8_t baseReg = mem.getBase() & 0x7;
  uint8_t indexReg = mem.getIndex() & 0x7;
  sysint_t disp = mem.getDisplacement();
  uint32_t shift = mem.getShift();

  if (mem.getMemType() == OPERAND_MEM_NATIVE)
  {
    // [base + displacemnt]
    if (!mem.hasIndex())
    {
      // ESP/RSP/R12 == 4
      if (baseReg == 4)
      {
        uint8_t mod = 0;

        if (disp)
        {
          mod = Util::isInt8(disp) ? 1 : 2;
        }

        _emitMod(mod, opReg, 4);
        _emitSib(0, 4, 4);

        if (disp)
        {
          if (Util::isInt8(disp))
            _emitByte((int8_t)disp);
          else
            _emitInt32((int32_t)disp);
        }
      }
      // EBP/RBP/R13 == 5
      else if (baseReg != 5 && disp == 0)
      {
        _emitMod(0, opReg, baseReg);
      }
      else if (Util::isInt8(disp))
      {
        _emitMod(1, opReg, baseReg);
        _emitByte((int8_t)disp);
      }
      else
      {
        _emitMod(2, opReg, baseReg);
        _emitInt32((int32_t)disp);
      }
    }

    // [base + index * scale + displacemnt]
    else
    {
      // ASMJIT_ASSERT(indexReg != RID_ESP);

      // EBP/RBP/R13 == 5
      if (baseReg != 5 && disp == 0)
      {
        _emitMod(0, opReg, 4);
        _emitSib(shift, indexReg, baseReg);
      }
      else if (Util::isInt8(disp))
      {
        _emitMod(1, opReg, 4);
        _emitSib(shift, indexReg, baseReg);
        _emitByte((int8_t)disp);
      }
      else
      {
        _emitMod(2, opReg, 4);
        _emitSib(shift, indexReg, baseReg);
        _emitInt32((int32_t)disp);
      }
    }
  }

  // Address                       | 32-bit mode | 64-bit mode
  // ------------------------------+-------------+---------------
  // [displacement]                |   ABSOLUTE  | RELATIVE (RIP)
  // [index * scale + displacemnt] |   ABSOLUTE  | ABSOLUTE (ZERO EXTENDED)
  else
  {
    // - In 32-bit mode the absolute addressing model is used.
    // - In 64-bit mode the relative addressing model is used together with
    //   the absolute addressing. Main problem is that if instruction
    //   contains SIB then relative addressing (RIP) is not possible.

#if defined(ASMJIT_X86)

    if (mem.hasIndex())
    {
      // ASMJIT_ASSERT(mem.getMemIndex() != 4); // ESP/RSP == 4
      _emitMod(0, opReg, 4);
      _emitSib(shift, indexReg, 5);
    }
    else
    {
      _emitMod(0, opReg, 5);
    }

    // X86 uses absolute addressing model, all relative addresses will be
    // relocated to absolute ones.
    if (mem.getMemType() == OPERAND_MEM_LABEL)
    {
      LabelData& l_data = _labelData[mem._mem.base & OPERAND_ID_VALUE_MASK];
      RelocData r_data;
      uint32_t relocId = _relocData.getLength();

      // Relative addressing will be relocated to absolute address.
      r_data.type = RelocData::RELATIVE_TO_ABSOLUTE;
      r_data.size = 4;
      r_data.offset = getOffset();
      r_data.destination = disp;

      if (l_data.offset != -1)
      {
        // Bound label.
        r_data.destination += l_data.offset;

        // Add a dummy DWORD.
        _emitInt32(0);
      }
      else
      {
        // Non-bound label.
        _emitDisplacement(l_data, -4 - immSize, 4)->relocId = relocId;
      }

      _relocData.append(r_data);
    }
    else
    {
      // Absolute address
      _emitInt32( (int32_t)((uint8_t*)mem._mem.target + disp) );
    }

#else

    // X64 uses relative addressing model
    if (mem.getMemType() == OPERAND_MEM_LABEL)
    {
      LabelData& l_data = _labelData[mem._mem.base & OPERAND_ID_VALUE_MASK];

      if (mem.hasIndex())
      {
        // Indexing is not possible.
        setError(ERROR_ILLEGAL_ADDRESING);
        return;
      }

      // Relative address (RIP +/- displacement).
      _emitMod(0, opReg, 5);

      disp -= (4 + immSize);

      if (l_data.offset != -1)
      {
        // Bound label.
        disp += getOffset() - l_data.offset;

        // Displacement is known.
        _emitInt32((int32_t)disp);
      }
      else
      {
        // Non-bound label.
        _emitDisplacement(l_data, disp, 4);
      }
    }
    else
    {
      // Absolute address (truncated to 32-bits), this kind of address requires
      // SIB byte (4).
      _emitMod(0, opReg, 4);

      if (mem.hasIndex())
      {
        // ASMJIT_ASSERT(mem.getMemIndex() != 4); // ESP/RSP == 4
        _emitSib(shift, indexReg, 5);
      }
      else
      {
        _emitSib(0, 4, 5);
      }

      // Truncate to 32-bits.
      sysuint_t target = (sysuint_t)((uint8_t*)mem._mem.target + disp);

      if (target > (sysuint_t)0xFFFFFFFF)
      {
        if (_logger) 
        {
          _logger->logString("*** ASSEMBER WARNING - Absolute address truncated to 32-bits.\n");
        }
        target &= 0xFFFFFFFF;
      }

      _emitInt32( (int32_t)((uint32_t)target) );
    }

#endif // ASMJIT_X64

  }
}

void AssemblerCore::_emitModRM(
  uint8_t opReg, const Operand& op, sysint_t immSize) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(op.getType() == OPERAND_REG || op.getType() == OPERAND_MEM);

  if (op.getType() == OPERAND_REG)
    _emitModR(opReg, reinterpret_cast<const BaseReg&>(op).getRegCode());
  else
    _emitModM(opReg, reinterpret_cast<const Mem&>(op), immSize);
}

void AssemblerCore::_emitX86Inl(
  uint32_t opCode, uint8_t i16bit, uint8_t rexw, uint8_t reg, bool forceRexPrefix) ASMJIT_NOTHROW
{
  // 16-bit prefix.
  if (i16bit) _emitByte(0x66);

  // Instruction prefix.
  if (opCode & 0xFF000000) _emitByte((uint8_t)((opCode & 0xFF000000) >> 24));

  // REX prefix.
#if defined(ASMJIT_X64)
  _emitRexR(rexw, 0, reg, forceRexPrefix);
#endif // ASMJIT_X64

  // Instruction opcodes.
  if (opCode & 0x00FF0000) _emitByte((uint8_t)((opCode & 0x00FF0000) >> 16));
  if (opCode & 0x0000FF00) _emitByte((uint8_t)((opCode & 0x0000FF00) >>  8));

  _emitByte((uint8_t)(opCode & 0x000000FF) + (reg & 0x7));
}

void AssemblerCore::_emitX86RM(
  uint32_t opCode, uint8_t i16bit, uint8_t rexw, uint8_t o,
  const Operand& op, sysint_t immSize, bool forceRexPrefix) ASMJIT_NOTHROW
{
  // 16-bit prefix.
  if (i16bit) _emitByte(0x66);

  // Segment prefix.
  _emitSegmentPrefix(op);

  // Instruction prefix.
  if (opCode & 0xFF000000) _emitByte((uint8_t)((opCode & 0xFF000000) >> 24));

  // REX prefix.
#if defined(ASMJIT_X64)
  _emitRexRM(rexw, o, op, forceRexPrefix);
#endif // ASMJIT_X64

  // Instruction opcodes.
  if (opCode & 0x00FF0000) _emitByte((uint8_t)((opCode & 0x00FF0000) >> 16));
  if (opCode & 0x0000FF00) _emitByte((uint8_t)((opCode & 0x0000FF00) >>  8));
  _emitByte((uint8_t)(opCode & 0x000000FF));

  // Mod R/M.
  _emitModRM(o, op, immSize);
}

void AssemblerCore::_emitFpu(uint32_t opCode) ASMJIT_NOTHROW
{
  _emitOpCode(opCode);
}

void AssemblerCore::_emitFpuSTI(uint32_t opCode, uint32_t sti) ASMJIT_NOTHROW
{
  // Illegal stack offset.
  ASMJIT_ASSERT(0 <= sti && sti < 8);
  _emitOpCode(opCode + sti);
}

void AssemblerCore::_emitFpuMEM(uint32_t opCode, uint8_t opReg, const Mem& mem) ASMJIT_NOTHROW
{
  // Segment prefix.
  _emitSegmentPrefix(mem);

  // Instruction prefix.
  if (opCode & 0xFF000000) _emitByte((uint8_t)((opCode & 0xFF000000) >> 24));

  // REX prefix.
#if defined(ASMJIT_X64)
  _emitRexRM(0, opReg, mem, false);
#endif // ASMJIT_X64

  // Instruction opcodes.
  if (opCode & 0x00FF0000) _emitByte((uint8_t)((opCode & 0x00FF0000) >> 16));
  if (opCode & 0x0000FF00) _emitByte((uint8_t)((opCode & 0x0000FF00) >>  8));

  _emitByte((uint8_t)((opCode & 0x000000FF)));
  _emitModM(opReg, mem, 0);
}

void AssemblerCore::_emitMmu(uint32_t opCode, uint8_t rexw, uint8_t opReg,
  const Operand& src, sysint_t immSize) ASMJIT_NOTHROW
{
  // Segment prefix.
  _emitSegmentPrefix(src);

  // Instruction prefix.
  if (opCode & 0xFF000000) _emitByte((uint8_t)((opCode & 0xFF000000) >> 24));

  // REX prefix.
#if defined(ASMJIT_X64)
  _emitRexRM(rexw, opReg, src, false);
#endif // ASMJIT_X64

  // Instruction opcodes.
  if (opCode & 0x00FF0000) _emitByte((uint8_t)((opCode & 0x00FF0000) >> 16));

  // No checking, MMX/SSE instructions have always two opcodes or more.
  _emitByte((uint8_t)((opCode & 0x0000FF00) >> 8));
  _emitByte((uint8_t)((opCode & 0x000000FF)));

  if (src.isReg())
    _emitModR(opReg, reinterpret_cast<const BaseReg&>(src).getRegCode());
  else
    _emitModM(opReg, reinterpret_cast<const Mem&>(src), immSize);
}

AssemblerCore::LabelLink* AssemblerCore::_emitDisplacement(
  LabelData& l_data, sysint_t inlinedDisplacement, int size) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(l_data.offset == -1);
  ASMJIT_ASSERT(size == 1 || size == 4);

  // Chain with label.
  LabelLink* link = _newLabelLink();
  link->prev = l_data.links;
  link->offset = getOffset();
  link->displacement = inlinedDisplacement;

  l_data.links = link;

  // Emit label size as dummy data.
  if (size == 1)
    _emitByte(0x01);
  else // if (size == 4)
    _emitDWord(0x04040404);

  return link;
}

void AssemblerCore::_emitJmpOrCallReloc(uint32_t instruction, void* target) ASMJIT_NOTHROW
{
  RelocData rd;

  rd.type = RelocData::ABSOLUTE_TO_RELATIVE_TRAMPOLINE;

#if defined(ASMJIT_X64)
  // If we are compiling in 64-bit mode, we can use trampoline if relative jump
  // is not possible.
  _trampolineSize += TrampolineWriter::TRAMPOLINE_SIZE;
#endif // ARCHITECTURE_SPECIFIC

  rd.size = 4;
  rd.offset = getOffset();
  rd.address = target;

  _relocData.append(rd);

  // Emit dummy 32-bit integer (will be overwritten by relocCode()).
  _emitInt32(0);
}

// Logging helpers.
static const char* operandSize[] =
{
  NULL,
  "byte ptr ",
  "word ptr ",
  NULL,
  "dword ptr ",
  NULL,
  NULL,
  NULL,
  "qword ptr ",
  NULL,
  "tword ptr ",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "dqword ptr "
};

static const char segmentPrefixName[] =
  "es:\0"
  "cs:\0"
  "ss:\0"
  "ds:\0"
  "fs:\0"
  "gs:\0"
  "\0\0\0\0";

ASMJIT_HIDDEN char* dumpInstructionName(char* buf, uint32_t code) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(code < _INST_COUNT);
  return Util::mycpy(buf, instructionDescription[code].getName());
}

ASMJIT_HIDDEN char* dumpRegister(char* buf, uint32_t type, uint32_t index) ASMJIT_NOTHROW
{
  // NE == Not-Encodable.
  const char reg8l[] = "al\0\0" "cl\0\0" "dl\0\0" "bl\0\0" "spl\0"  "bpl\0"  "sil\0"  "dil\0" ;
  const char reg8h[] = "ah\0\0" "ch\0\0" "dh\0\0" "bh\0\0" "NE\0\0" "NE\0\0" "NE\0\0" "NE\0\0";
  const char reg16[] = "ax\0\0" "cx\0\0" "dx\0\0" "bx\0\0" "sp\0\0" "bp\0\0" "si\0\0" "di\0\0";

  switch (type)
  {
    case REG_TYPE_GPB_LO:
      if (index < 8)
        return Util::mycpy(buf, &reg8l[index*4]);

      *buf++ = 'r';
      goto _EmitID;

    case REG_TYPE_GPB_HI:
      if (index < 4)
        return Util::mycpy(buf, &reg8h[index*4]);

_EmitNE:
      return Util::mycpy(buf, "NE");

    case REG_TYPE_GPW:
      if (index < 8)
        return Util::mycpy(buf, &reg16[index*4]);

      *buf++ = 'r';
      buf = Util::myutoa(buf, index);
      *buf++ = 'w';
      return buf;

    case REG_TYPE_GPD:
      if (index < 8)
      {
        *buf++ = 'e';
        return Util::mycpy(buf, &reg16[index*4]);
      }

      *buf++ = 'r';
      buf = Util::myutoa(buf, index);
      *buf++ = 'd';
      return buf;
    
    case REG_TYPE_GPQ:
      *buf++ = 'r';

      if (index < 8)
        return Util::mycpy(buf, &reg16[index*4]);

_EmitID:
      return Util::myutoa(buf, index);
    
    case REG_TYPE_X87:
      *buf++ = 's';
      *buf++ = 't';
      goto _EmitID;
    
    case REG_TYPE_MM:
      *buf++ = 'm';
      *buf++ = 'm';
      goto _EmitID;
    
    case REG_TYPE_XMM:
      *buf++ = 'x';
      *buf++ = 'm';
      *buf++ = 'm';
      goto _EmitID;
    
    case REG_TYPE_YMM:
      *buf++ = 'y';
      *buf++ = 'm';
      *buf++ = 'm';
      goto _EmitID;

    case REG_TYPE_SEGMENT:
      if (index < REG_NUM_SEGMENT)
        return Util::mycpy(buf, &segmentPrefixName[index*4], 2);
      
      goto _EmitNE;

    default:
      return buf;
  }
}

ASMJIT_HIDDEN char* dumpOperand(char* buf, const Operand* op, uint32_t memRegType) ASMJIT_NOTHROW
{
  if (op->isReg())
  {
    const BaseReg& reg = reinterpret_cast<const BaseReg&>(*op);
    return dumpRegister(buf, reg.getRegType(), reg.getRegIndex());
  }
  else if (op->isMem())
  {
    const Mem& mem = reinterpret_cast<const Mem&>(*op);
    uint32_t segmentPrefix = mem.getSegmentPrefix();

    bool isAbsolute = false;

    if (op->getSize() <= 16)
    {
      buf = Util::mycpy(buf, operandSize[op->getSize()]);
    }

    if (segmentPrefix < REG_NUM_SEGMENT)
    {
      buf = Util::mycpy(buf, &segmentPrefixName[segmentPrefix * 4]);
    }

    *buf++ = '[';

    switch (mem.getMemType())
    {
      case OPERAND_MEM_NATIVE:
      {
        // [base + index*scale + displacement]
        buf = dumpRegister(buf, memRegType, mem.getBase());
        break;
      }
      case OPERAND_MEM_LABEL:
      {
        // [label + index*scale + displacement]
        buf += sprintf(buf, "L.%u", mem.getBase() & OPERAND_ID_VALUE_MASK);
        break;
      }
      case OPERAND_MEM_ABSOLUTE:
      {
        // [absolute]
        isAbsolute = true;
        buf = Util::myutoa(buf, (sysuint_t)mem.getTarget() + mem.getDisplacement(), 16);
        break;
      }
    }

    if (mem.hasIndex())
    {
      buf = Util::mycpy(buf, " + ");
      buf = dumpRegister(buf, memRegType, mem.getIndex());

      if (mem.getShift())
      {
        buf = Util::mycpy(buf, " * ");
        *buf++ = "1248"[mem.getShift() & 3];
      }
    }

    if (mem.getDisplacement() && !isAbsolute)
    {
      sysint_t d = mem.getDisplacement();
      *buf++ = ' ';
      *buf++ = (d < 0) ? '-' : '+';
      *buf++ = ' ';
      buf = Util::myutoa(buf, d < 0 ? -d : d);
    }

    *buf++ = ']';
    return buf;
  }
  else if (op->isImm())
  {
    const Imm& i = reinterpret_cast<const Imm&>(*op);
    return Util::myitoa(buf, (sysint_t)i.getValue());
  }
  else if (op->isLabel())
  {
    return buf + sprintf(buf, "L.%u", op->getId() & OPERAND_ID_VALUE_MASK);
  }
  else
  {
    return Util::mycpy(buf, "None");
  }
}

static char* dumpInstruction(char* buf,
  uint32_t code,
  uint32_t emitOptions,
  const Operand* o0,
  const Operand* o1,
  const Operand* o2,
  uint32_t memRegType) ASMJIT_NOTHROW
{
  if (emitOptions & EMIT_OPTION_REX_PREFIX ) buf = Util::mycpy(buf, "rex ", 4);
  if (emitOptions & EMIT_OPTION_LOCK_PREFIX) buf = Util::mycpy(buf, "lock ", 5);
  if (emitOptions & EMIT_OPTION_SHORT_JUMP ) buf = Util::mycpy(buf, "short ", 6);

  // Dump instruction.
  buf = dumpInstructionName(buf, code);

  // Dump operands.
  if (!o0->isNone()) { *buf++ = ' '; buf = dumpOperand(buf, o0, memRegType); }
  if (!o1->isNone()) { *buf++ = ','; *buf++ = ' '; buf = dumpOperand(buf, o1, memRegType); }
  if (!o2->isNone()) { *buf++ = ','; *buf++ = ' '; buf = dumpOperand(buf, o2, memRegType); }

  return buf;
}

static char* dumpComment(char* buf, sysuint_t len, const uint8_t* binaryData, sysuint_t binaryLen, const char* comment)
{
  sysuint_t currentLength = len;
  sysuint_t commentLength = comment ? strlen(comment) : 0;

  if (binaryLen || commentLength)
  {
    sysuint_t align = 32;
    char sep = ';';

    // Truncate if comment is too long (it shouldn't be, larger than 80 seems to
    // be an exploit).
    if (commentLength > 80) commentLength = 80;

    for (sysuint_t i = (binaryLen == 0); i < 2; i++)
    {
      char* bufBegin = buf;

      // Append align.
      if (currentLength < align) 
      {
        buf = Util::myfill(buf, ' ', align - currentLength);
      }

      // Append separator.
      if (sep)
      {
        *buf++ = sep;
        *buf++ = ' ';
      }

      // Append binary data or comment.
      if (i == 0)
      {
        buf = Util::myhex(buf, binaryData, binaryLen);
        if (commentLength == 0) break;
      }
      else
      {
        buf = Util::mycpy(buf, comment, commentLength);
      }

      currentLength += (sysuint_t)(buf - bufBegin);
      align += 18;
      sep = '|';
    }
  }

  *buf++ = '\n';
  return buf;
}

// Used for NULL operands to translate them to OPERAND_NONE.
static const uint8_t _none[sizeof(Operand)] =
{
  0
};

static const Operand::RegData _patchedHiRegs[4] =
{// op         , size, { reserved0, reserved1 }, id           , code
  { OPERAND_REG, 1   , { 0        , 0         }, INVALID_VALUE, REG_TYPE_GPB_LO | 4 },
  { OPERAND_REG, 1   , { 0        , 0         }, INVALID_VALUE, REG_TYPE_GPB_LO | 5 },
  { OPERAND_REG, 1   , { 0        , 0         }, INVALID_VALUE, REG_TYPE_GPB_LO | 6 },
  { OPERAND_REG, 1   , { 0        , 0         }, INVALID_VALUE, REG_TYPE_GPB_LO | 7 }
};

void AssemblerCore::_emitInstruction(uint32_t code) ASMJIT_NOTHROW
{
  _emitInstruction(code, NULL, NULL, NULL);
}

void AssemblerCore::_emitInstruction(uint32_t code, const Operand* o0) ASMJIT_NOTHROW
{
  _emitInstruction(code, o0, NULL, NULL);
}

void AssemblerCore::_emitInstruction(uint32_t code, const Operand* o0, const Operand* o1) ASMJIT_NOTHROW
{
  _emitInstruction(code, o0, o1, NULL);
}

void AssemblerCore::_emitInstruction(uint32_t code, const Operand* o0, const Operand* o1, const Operand* o2) ASMJIT_NOTHROW
{
  const Operand* _loggerOperands[3];

  uint32_t bLoHiUsed = 0;
#if defined(ASMJIT_X86)
  uint32_t forceRexPrefix = false;
#else
  uint32_t forceRexPrefix = _emitOptions & EMIT_OPTION_REX_PREFIX;
#endif
  uint32_t memRegType = REG_TYPE_GPN;

#if defined(ASMJIT_DEBUG)
  bool assertIllegal = false;
#endif // ASMJIT_DEBUG

  const Imm* immOperand = NULL;
  uint32_t immSize;

#define _FINISHED() \
  goto end

#define _FINISHED_IMMEDIATE(_Operand_, _Size_) \
  do { \
    immOperand = reinterpret_cast<const Imm*>(_Operand_); \
    immSize = (_Size_); \
    goto emitImmediate; \
  } while (0)

  // Convert operands to OPERAND_NONE if needed.
  if (o0 == NULL) { o0 = reinterpret_cast<const Operand*>(_none); } else if (o0->isReg()) { bLoHiUsed |= o0->_reg.code & (REG_TYPE_GPB_LO | REG_TYPE_GPB_HI); }
  if (o1 == NULL) { o1 = reinterpret_cast<const Operand*>(_none); } else if (o1->isReg()) { bLoHiUsed |= o1->_reg.code & (REG_TYPE_GPB_LO | REG_TYPE_GPB_HI); }
  if (o2 == NULL) { o2 = reinterpret_cast<const Operand*>(_none); } else if (o2->isReg()) { bLoHiUsed |= o2->_reg.code & (REG_TYPE_GPB_LO | REG_TYPE_GPB_HI); }

  sysuint_t beginOffset = getOffset();
  const InstructionDescription* id = &instructionDescription[code];

  if (code >= _INST_COUNT)
  {
    setError(ERROR_UNKNOWN_INSTRUCTION);
    goto cleanup;
  }

  // Check if register operand is BPL, SPL, SIL, DIL and do action that depends
  // to current mode:
  //   - 64-bit: - Force REX prefix.
  //
  // Check if register operand is AH, BH, CH or DH and do action that depends
  // to current mode:
  //   - 32-bit: - Patch operand index (index += 4), because we are using
  //               different index what is used in opcode.
  //   - 64-bit: - Check whether there is REX prefix and raise error if it is.
  //             - Do the same as in 32-bit mode - patch register index.
  //
  // NOTE: This is a hit hacky, but I added this to older code-base and I have
  // no energy to rewrite it. Maybe in future all of this can be cleaned up!
  if (bLoHiUsed | forceRexPrefix)
  {
    _loggerOperands[0] = o0;
    _loggerOperands[1] = o1;
    _loggerOperands[2] = o2;

#if defined(ASMJIT_X64)
    // Check if there is register that makes this instruction un-encodable.

    forceRexPrefix |= (uint32_t)o0->isExtendedRegisterUsed();
    forceRexPrefix |= (uint32_t)o1->isExtendedRegisterUsed();
    forceRexPrefix |= (uint32_t)o2->isExtendedRegisterUsed();

    if      (o0->isRegType(REG_TYPE_GPB_LO) && (o0->_reg.code & REG_INDEX_MASK) >= 4) forceRexPrefix = true;
    else if (o1->isRegType(REG_TYPE_GPB_LO) && (o1->_reg.code & REG_INDEX_MASK) >= 4) forceRexPrefix = true;
    else if (o2->isRegType(REG_TYPE_GPB_LO) && (o2->_reg.code & REG_INDEX_MASK) >= 4) forceRexPrefix = true;

    if ((bLoHiUsed & REG_TYPE_GPB_HI) != 0 && forceRexPrefix)
    {
      goto illegalInstruction;
    }
#endif // ASMJIT_X64

    // Patch GPB.HI operand index.
    if ((bLoHiUsed & REG_TYPE_GPB_HI) != 0)
    {
      if (o0->isRegType(REG_TYPE_GPB_HI)) o0 = reinterpret_cast<const Operand*>(&_patchedHiRegs[o0->_reg.code & REG_INDEX_MASK]);
      if (o1->isRegType(REG_TYPE_GPB_HI)) o1 = reinterpret_cast<const Operand*>(&_patchedHiRegs[o1->_reg.code & REG_INDEX_MASK]);
      if (o2->isRegType(REG_TYPE_GPB_HI)) o2 = reinterpret_cast<const Operand*>(&_patchedHiRegs[o2->_reg.code & REG_INDEX_MASK]);
    }
  }

  // Check for buffer space (and grow if needed).
  if (!canEmit()) goto cleanup;

  if (_emitOptions & EMIT_OPTION_LOCK_PREFIX)
  {
    if (!id->isLockable())
      goto illegalInstruction;
    _emitByte(0xF0);
  }

  switch (id->group)
  {
    case InstructionDescription::G_EMIT:
    {
      _emitOpCode(id->opCode[0]);
      _FINISHED();
    }

    case InstructionDescription::G_ALU:
    {
      uint32_t opCode = id->opCode[0];
      uint8_t opReg = (uint8_t)id->opCodeR;

      // Mem <- Reg
      if (o0->isMem() && o1->isReg())
      {
        _emitX86RM(opCode + (o1->getSize() != 1),
          o1->getSize() == 2,
          o1->getSize() == 8,
          reinterpret_cast<const GPReg&>(*o1).getRegCode(),
          reinterpret_cast<const Operand&>(*o0),
          0, forceRexPrefix);
        _FINISHED();
      }

      // Reg <- Reg|Mem
      if (o0->isReg() && o1->isRegMem())
      {
        _emitX86RM(opCode + 2 + (o0->getSize() != 1),
          o0->getSize() == 2,
          o0->getSize() == 8,
          reinterpret_cast<const GPReg&>(*o0).getRegCode(),
          reinterpret_cast<const Operand&>(*o1),
          0, forceRexPrefix);
        _FINISHED();
      }

      // AL, AX, EAX, RAX register shortcuts
      if (o0->isRegIndex(0) && o1->isImm())
      {
        if (o0->getSize() == 2)
          _emitByte(0x66); // 16-bit.
        else if (o0->getSize() == 8)
          _emitByte(0x48); // REX.W.

        _emitByte((opReg << 3) | (0x04 + (o0->getSize() != 1)));
        _FINISHED_IMMEDIATE(o1, o0->getSize() <= 4 ? o0->getSize() : 4);
      }

      if (o0->isRegMem() && o1->isImm())
      {
        const Imm& imm = reinterpret_cast<const Imm&>(*o1);
        immSize = Util::isInt8(imm.getValue()) ? 1 : (o0->getSize() <= 4 ? o0->getSize() : 4);

        _emitX86RM(id->opCode[1] + (o0->getSize() != 1 ? (immSize != 1 ? 1 : 3) : 0),
          o0->getSize() == 2,
          o0->getSize() == 8,
          opReg, reinterpret_cast<const Operand&>(*o0),
          immSize, forceRexPrefix);
        _FINISHED_IMMEDIATE(&imm, immSize);
      }

      break;
    }

    case InstructionDescription::G_BSWAP:
    {
      if (o0->isReg())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);

#if defined(ASMJIT_X64)
        _emitRexR(dst.getRegType() == REG_TYPE_GPQ, 1, dst.getRegCode(), forceRexPrefix);
#endif // ASMJIT_X64
        _emitByte(0x0F);
        _emitModR(1, dst.getRegCode());
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_BT:
    {
      if (o0->isRegMem() && o1->isReg())
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);
        const GPReg& src = reinterpret_cast<const GPReg&>(*o1);

        _emitX86RM(id->opCode[0],
          src.isRegType(REG_TYPE_GPW),
          src.isRegType(REG_TYPE_GPQ),
          src.getRegCode(),
          dst,
          0, forceRexPrefix);
        _FINISHED();
      }

      if (o0->isRegMem() && o1->isImm())
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);
        const Imm& src = reinterpret_cast<const Imm&>(*o1);

        _emitX86RM(id->opCode[1],
          dst.getSize() == 2,
          dst.getSize() == 8,
          (uint8_t)id->opCodeR,
          dst,
          1, forceRexPrefix);
        _FINISHED_IMMEDIATE(o1, 1);
      }

      break;
    }

    case InstructionDescription::G_CALL:
    {
      if (o0->isRegTypeMem(REG_TYPE_GPN))
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);
        _emitX86RM(0xFF,
          0,
          0, 2, dst,
          0, forceRexPrefix);
        _FINISHED();
      }

      if (o0->isImm())
      {
        const Imm& imm = reinterpret_cast<const Imm&>(*o0);
        _emitByte(0xE8);
        _emitJmpOrCallReloc(InstructionDescription::G_CALL, (void*)imm.getValue());
        _FINISHED();
      }

      if (o0->isLabel())
      {
        LabelData& l_data = _labelData[reinterpret_cast<const Label*>(o0)->getId() & OPERAND_ID_VALUE_MASK];

        if (l_data.offset != -1)
        {
          // Bound label.
          static const sysint_t rel32_size = 5;
          sysint_t offs = l_data.offset - getOffset();

          ASMJIT_ASSERT(offs <= 0);

          _emitByte(0xE8);
          _emitInt32((int32_t)(offs - rel32_size));
        }
        else
        {
          // Non-bound label.
          _emitByte(0xE8);
          _emitDisplacement(l_data, -4, 4);
        }
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_CRC32:
    {
      if (o0->isReg() && o1->isRegMem())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        const Operand& src = reinterpret_cast<const Operand&>(*o1);
        ASMJIT_ASSERT(dst.getRegType() == REG_TYPE_GPD || dst.getRegType() == REG_TYPE_GPQ);

        _emitX86RM(id->opCode[0] + (src.getSize() != 1),
          src.getSize() == 2,
          dst.getRegType() == 8, dst.getRegCode(), src,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_ENTER:
    {
      if (o0->isImm() && o1->isImm())
      {
        _emitByte(0xC8);
        _emitWord((uint16_t)(sysuint_t)reinterpret_cast<const Imm&>(*o2).getValue());
        _emitByte((uint8_t )(sysuint_t)reinterpret_cast<const Imm&>(*o1).getValue());
        _FINISHED();
      }
      break;
    }

    case InstructionDescription::G_IMUL:
    {
      // 1 operand
      if (o0->isRegMem() && o1->isNone() && o2->isNone())
      {
        const Operand& src = reinterpret_cast<const Operand&>(*o0);
        _emitX86RM(0xF6 + (src.getSize() != 1),
          src.getSize() == 2,
          src.getSize() == 8, 5, src,
          0, forceRexPrefix);
        _FINISHED();
      }
      // 2 operands
      else if (o0->isReg() && !o1->isNone() && o2->isNone())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        ASMJIT_ASSERT(!dst.isRegType(REG_TYPE_GPW));

        if (o1->isRegMem())
        {
          const Operand& src = reinterpret_cast<const Operand&>(*o1);

          _emitX86RM(0x0FAF,
            dst.isRegType(REG_TYPE_GPW),
            dst.isRegType(REG_TYPE_GPQ), dst.getRegCode(), src,
            0, forceRexPrefix);
          _FINISHED();
        }
        else if (o1->isImm())
        {
          const Imm& imm = reinterpret_cast<const Imm&>(*o1);

          if (Util::isInt8(imm.getValue()))
          {
            _emitX86RM(0x6B,
              dst.isRegType(REG_TYPE_GPW),
              dst.isRegType(REG_TYPE_GPQ), dst.getRegCode(), dst,
              1, forceRexPrefix);
            _FINISHED_IMMEDIATE(&imm, 1);
          }
          else
          {
            immSize = dst.isRegType(REG_TYPE_GPW) ? 2 : 4;
            _emitX86RM(0x69,
              dst.isRegType(REG_TYPE_GPW),
              dst.isRegType(REG_TYPE_GPQ), dst.getRegCode(), dst,
              immSize, forceRexPrefix);
            _FINISHED_IMMEDIATE(&imm, immSize);
          }
        }
      }
      // 3 operands
      else if (o0->isReg() && o1->isRegMem() && o2->isImm())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        const Operand& src = reinterpret_cast<const Operand&>(*o1);
        const Imm& imm = reinterpret_cast<const Imm&>(*o2);

        if (Util::isInt8(imm.getValue()))
        {
          _emitX86RM(0x6B,
            dst.isRegType(REG_TYPE_GPW),
            dst.isRegType(REG_TYPE_GPQ), dst.getRegCode(), src,
            1, forceRexPrefix);
          _FINISHED_IMMEDIATE(&imm, 1);
        }
        else
        {
          immSize = dst.isRegType(REG_TYPE_GPW) ? 2 : 4;
          _emitX86RM(0x69,
            dst.isRegType(REG_TYPE_GPW),
            dst.isRegType(REG_TYPE_GPQ), dst.getRegCode(), src,
            immSize, forceRexPrefix);
          _FINISHED_IMMEDIATE(&imm, immSize);
        }
      }

      break;
    }

    case InstructionDescription::G_INC_DEC:
    {
      if (o0->isRegMem())
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);

        // INC [r16|r32] in 64-bit mode is not encodable.
#if defined(ASMJIT_X86)
        if ((dst.isReg()) && (dst.isRegType(REG_TYPE_GPW) || dst.isRegType(REG_TYPE_GPD)))
        {
          _emitX86Inl(id->opCode[0],
            dst.isRegType(REG_TYPE_GPW),
            0, reinterpret_cast<const BaseReg&>(dst).getRegCode(),
            false);
          _FINISHED();
        }
#endif // ASMJIT_X86

        _emitX86RM(id->opCode[1] + (dst.getSize() != 1),
          dst.getSize() == 2,
          dst.getSize() == 8, (uint8_t)id->opCodeR, dst,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_J:
    {
      if (o0->isLabel())
      {
        LabelData& l_data = _labelData[reinterpret_cast<const Label*>(o0)->getId() & OPERAND_ID_VALUE_MASK];

        uint32_t hint = (uint32_t)(o1->isImm() ? reinterpret_cast<const Imm&>(*o1).getValue() : 0);
        bool isShortJump = (_emitOptions & EMIT_OPTION_SHORT_JUMP) != 0;

        // Emit jump hint if configured for that.
        if ((hint & (HINT_TAKEN | HINT_NOT_TAKEN)) && (_properties & (1 << PROPERTY_JUMP_HINTS)))
        {
          if (hint & HINT_TAKEN)
            _emitByte(HINT_BYTE_VALUE_TAKEN);
          else if (hint & HINT_NOT_TAKEN)
            _emitByte(HINT_BYTE_VALUE_NOT_TAKEN);
        }

        if (l_data.offset != -1)
        {
          // Bound label.
          static const sysint_t rel8_size = 2;
          static const sysint_t rel32_size = 6;
          sysint_t offs = l_data.offset - getOffset();

          ASMJIT_ASSERT(offs <= 0);

          if (Util::isInt8(offs - rel8_size))
          {
            _emitByte(0x70 | (uint8_t)id->opCode[0]);
            _emitByte((uint8_t)(int8_t)(offs - rel8_size));

            // Change the emit options so logger can log instruction correctly.
            _emitOptions |= EMIT_OPTION_SHORT_JUMP;
          }
          else
          {
            if (isShortJump && _logger)
            {
              _logger->logString("*** ASSEMBLER WARNING: Emitting long conditional jump, but short jump instruction forced!\n");
              _emitOptions &= ~EMIT_OPTION_SHORT_JUMP;
            }

            _emitByte(0x0F);
            _emitByte(0x80 | (uint8_t)id->opCode[0]);
            _emitInt32((int32_t)(offs - rel32_size));
          }
        }
        else
        {
          // Non-bound label.
          if (isShortJump)
          {
            _emitByte(0x70 | (uint8_t)id->opCode[0]);
            _emitDisplacement(l_data, -1, 1);
          }
          else
          {
            _emitByte(0x0F);
            _emitByte(0x80 | (uint8_t)id->opCode[0]);
            _emitDisplacement(l_data, -4, 4);
          }
        }
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_JMP:
    {
      if (o0->isRegMem())
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);

        _emitX86RM(0xFF,
          0,
          0, 4, dst,
          0, forceRexPrefix);
        _FINISHED();
      }

      if (o0->isImm())
      {
        const Imm& imm = reinterpret_cast<const Imm&>(*o0);
        _emitByte(0xE9);
        _emitJmpOrCallReloc(InstructionDescription::G_JMP, (void*)imm.getValue());
        _FINISHED();
      }

      if (o0->isLabel())
      {
        LabelData& l_data = _labelData[reinterpret_cast<const Label*>(o0)->getId() & OPERAND_ID_VALUE_MASK];
        bool isShortJump = (_emitOptions & EMIT_OPTION_SHORT_JUMP) != 0;

        if (l_data.offset != -1)
        {
          // Bound label.
          const sysint_t rel8_size = 2;
          const sysint_t rel32_size = 5;
          sysint_t offs = l_data.offset - getOffset();

          if (Util::isInt8(offs - rel8_size))
          {
            _emitByte(0xEB);
            _emitByte((uint8_t)(int8_t)(offs - rel8_size));

            // Change the emit options so logger can log instruction correctly.
            _emitOptions |= EMIT_OPTION_SHORT_JUMP;
          }
          else
          {
            if (isShortJump)
            {
              if (_logger)
              {
                _logger->logString("*** ASSEMBLER WARNING: Emitting long jump, but short jump instruction forced!\n");
                _emitOptions &= ~EMIT_OPTION_SHORT_JUMP;
              }
            }

            _emitByte(0xE9);
            _emitInt32((int32_t)(offs - rel32_size));
          }
        }
        else
        {
          // Non-bound label.
          if (isShortJump)
          {
            _emitByte(0xEB);
            _emitDisplacement(l_data, -1, 1);
          }
          else
          {
            _emitByte(0xE9);
            _emitDisplacement(l_data, -4, 4);
          }
        }
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_LEA:
    {
      if (o0->isReg() && o1->isMem())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        const Mem& src = reinterpret_cast<const Mem&>(*o1);

        // Size override prefix support.
        if (src.getSizePrefix())
        {
          _emitByte(0x67);
#if defined(ASMJIT_X86)
          memRegType = REG_TYPE_GPW;
#else
          memRegType = REG_TYPE_GPD;
#endif
        }

        _emitX86RM(0x8D,
          dst.isRegType(REG_TYPE_GPW),
          dst.isRegType(REG_TYPE_GPQ), dst.getRegCode(), src,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_M:
    {
      if (o0->isMem())
      {
        _emitX86RM(id->opCode[0], 0, (uint8_t)id->opCode[1], (uint8_t)id->opCodeR, reinterpret_cast<const Mem&>(*o0), 0, forceRexPrefix);
        _FINISHED();
      }
      break;
    }

    case InstructionDescription::G_MOV:
    {
      const Operand& dst = *o0;
      const Operand& src = *o1;

      switch (dst.getType() << 4 | src.getType())
      {
        // Reg <- Reg/Mem
        case (OPERAND_REG << 4) | OPERAND_REG:
        {
          // Reg <- Sreg
          if (src.isRegType(REG_TYPE_SEGMENT))
          {
            ASMJIT_ASSERT(dst.isRegType(REG_TYPE_GPW) ||
                          dst.isRegType(REG_TYPE_GPD) ||
                          dst.isRegType(REG_TYPE_GPQ) );

            _emitX86RM(0x8C,
              dst.getSize() == 2,
              dst.getSize() == 8,
              reinterpret_cast<const SegmentReg&>(src).getRegCode(),
              reinterpret_cast<const Operand&>(dst),
              0, forceRexPrefix);
            _FINISHED();
          }

          // Sreg <- Reg/Mem
          if (dst.isRegType(REG_TYPE_SEGMENT))
          {
            ASMJIT_ASSERT(src.isRegType(REG_TYPE_GPW   ) ||
                          src.isRegType(REG_TYPE_GPD   ) ||
                          src.isRegType(REG_TYPE_GPQ   ) );

_Emit_Mov_Sreg_RM:
            _emitX86RM(0x8E,
              src.getSize() == 2,
              src.getSize() == 8,
              reinterpret_cast<const SegmentReg&>(dst).getRegCode(),
              reinterpret_cast<const Operand&>(src),
              0, forceRexPrefix);
            _FINISHED();
          }

          ASMJIT_ASSERT(src.isRegType(REG_TYPE_GPB_LO) ||
                        src.isRegType(REG_TYPE_GPB_HI) ||
                        src.isRegType(REG_TYPE_GPW   ) ||
                        src.isRegType(REG_TYPE_GPD   ) ||
                        src.isRegType(REG_TYPE_GPQ   ) );
          // ... fall through ...
        }
        case (OPERAND_REG << 4) | OPERAND_MEM:
        {
          // Sreg <- Mem
          if (dst.isRegType(REG_TYPE_SEGMENT))
          {
            goto _Emit_Mov_Sreg_RM;
          }

          ASMJIT_ASSERT(dst.isRegType(REG_TYPE_GPB_LO) ||
                        dst.isRegType(REG_TYPE_GPB_HI) ||
                        dst.isRegType(REG_TYPE_GPW   ) ||
                        dst.isRegType(REG_TYPE_GPD   ) ||
                        dst.isRegType(REG_TYPE_GPQ   ) );

          _emitX86RM(0x0000008A + (dst.getSize() != 1),
            dst.isRegType(REG_TYPE_GPW),
            dst.isRegType(REG_TYPE_GPQ),
            reinterpret_cast<const GPReg&>(dst).getRegCode(),
            reinterpret_cast<const Operand&>(src),
            0, forceRexPrefix);
          _FINISHED();
        }

        // Reg <- Imm
        case (OPERAND_REG << 4) | OPERAND_IMM:
        {
          const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
          const Imm& src = reinterpret_cast<const Imm&>(*o1);

          // In 64-bit mode the immediate can be 64-bits long if the
          // destination operand type is register (otherwise 32-bits).
          immSize = dst.getSize();

#if defined(ASMJIT_X64)
          // Optimize instruction size by using 32-bit immediate if value can
          // fit into it.
          if (immSize == 8 && Util::isInt32(src.getValue()))
          {
            _emitX86RM(0xC7,
              0, // 16BIT
              1, // REX.W
              0, // O
              dst,
              0, forceRexPrefix);
            immSize = 4;
          }
          else
          {
#endif // ASMJIT_X64
            _emitX86Inl((dst.getSize() == 1 ? 0xB0 : 0xB8),
              dst.isRegType(REG_TYPE_GPW),
              dst.isRegType(REG_TYPE_GPQ),
              dst.getRegCode(), forceRexPrefix);
#if defined(ASMJIT_X64)
          }
#endif // ASMJIT_X64

          _FINISHED_IMMEDIATE(&src, immSize);
        }

        // Mem <- Reg/Sreg
        case (OPERAND_MEM << 4) | OPERAND_REG:
        {
          if (src.isRegType(REG_TYPE_SEGMENT))
          {
            // Mem <- Sreg
            _emitX86RM(0x8C,
              dst.getSize() == 2,
              dst.getSize() == 8,
              reinterpret_cast<const SegmentReg&>(src).getRegCode(),
              reinterpret_cast<const Operand&>(dst),
              0, forceRexPrefix);
            _FINISHED();
          }
          else
          {
            // Mem <- Reg
            ASMJIT_ASSERT(src.isRegType(REG_TYPE_GPB_LO) ||
                          src.isRegType(REG_TYPE_GPB_HI) ||
                          src.isRegType(REG_TYPE_GPW   ) ||
                          src.isRegType(REG_TYPE_GPD   ) ||
                          src.isRegType(REG_TYPE_GPQ   ) );

            _emitX86RM(0x88 + (src.getSize() != 1),
              src.isRegType(REG_TYPE_GPW),
              src.isRegType(REG_TYPE_GPQ),
              reinterpret_cast<const GPReg&>(src).getRegCode(),
              reinterpret_cast<const Operand&>(dst),
              0, forceRexPrefix);
            _FINISHED();
          }
        }

        // Mem <- Imm
        case (OPERAND_MEM << 4) | OPERAND_IMM:
        {
          immSize = dst.getSize() <= 4 ? dst.getSize() : 4;

          _emitX86RM(0xC6 + (dst.getSize() != 1),
            dst.getSize() == 2,
            dst.getSize() == 8,
            0,
            reinterpret_cast<const Operand&>(dst),
            immSize, forceRexPrefix);
          _FINISHED_IMMEDIATE(&src, immSize);
        }
      }

      break;
    }

    case InstructionDescription::G_MOV_PTR:
    {
      if ((o0->isReg() && o1->isImm()) || (o0->isImm() && o1->isReg()))
      {
        bool reverse = o1->getType() == OPERAND_REG;
        uint8_t opCode = !reverse ? 0xA0 : 0xA2;
        const GPReg& reg = reinterpret_cast<const GPReg&>(!reverse ? *o0 : *o1);
        const Imm& imm = reinterpret_cast<const Imm&>(!reverse ? *o1 : *o0);

        if (reg.getRegIndex() != 0)
          goto illegalInstruction;

        if (reg.isRegType(REG_TYPE_GPW)) _emitByte(0x66);
#if defined(ASMJIT_X64)
        _emitRexR(reg.getSize() == 8, 0, 0, forceRexPrefix);
#endif // ASMJIT_X64
        _emitByte(opCode + (reg.getSize() != 1));
        _FINISHED_IMMEDIATE(&imm, sizeof(sysint_t));
      }

      break;
    }

    case InstructionDescription::G_MOVSX_MOVZX:
    {
      if (o0->isReg() && o1->isRegMem())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        const Operand& src = reinterpret_cast<const Operand&>(*o1);

        if (dst.getSize() == 1)
          goto illegalInstruction;
        
        if (src.getSize() != 1 && src.getSize() != 2)
          goto illegalInstruction;
        
        if (src.getSize() == 2 && dst.getSize() == 2)
          goto illegalInstruction;

        _emitX86RM(id->opCode[0] + (src.getSize() != 1),
          dst.isRegType(REG_TYPE_GPW),
          dst.isRegType(REG_TYPE_GPQ),
          dst.getRegCode(),
          src,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

#if defined(ASMJIT_X64)
    case InstructionDescription::G_MOVSXD:
    {
      if (o0->isReg() && o1->isRegMem())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        const Operand& src = reinterpret_cast<const Operand&>(*o1);
        _emitX86RM(0x00000063,
          0,
          1, dst.getRegCode(), src,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }
#endif // ASMJIT_X64

    case InstructionDescription::G_PUSH:
    {
      // This section is only for immediates, memory/register operands are handled in G_POP.
      if (o0->isImm())
      {
        const Imm& imm = reinterpret_cast<const Imm&>(*o0);

        if (Util::isInt8(imm.getValue()))
        {
          _emitByte(0x6A);
          _FINISHED_IMMEDIATE(&imm, 1);
        }
        else
        {
          _emitByte(0x68);
          _FINISHED_IMMEDIATE(&imm, 4);
        }
      }

      // ... goto G_POP ...
    }

    case InstructionDescription::G_POP:
    {
      if (o0->isReg())
      {
        ASMJIT_ASSERT(o0->isRegType(REG_TYPE_GPW) || o0->isRegType(REG_TYPE_GPN));
        _emitX86Inl(id->opCode[0], o0->isRegType(REG_TYPE_GPW), 0, reinterpret_cast<const GPReg&>(*o0).getRegCode(), forceRexPrefix);
        _FINISHED();
      }

      if (o0->isMem())
      {
        _emitX86RM(id->opCode[1], o0->getSize() == 2, 0, (uint8_t)id->opCodeR, reinterpret_cast<const Operand&>(*o0), 0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_R_RM:
    {
      if (o0->isReg() && o1->isRegMem())
      {
        const GPReg& dst = reinterpret_cast<const GPReg&>(*o0);
        const Operand& src = reinterpret_cast<const Operand&>(*o1);
        ASMJIT_ASSERT(dst.getSize() != 1);

        _emitX86RM(id->opCode[0],
          dst.getRegType() == REG_TYPE_GPW,
          dst.getRegType() == REG_TYPE_GPQ, dst.getRegCode(), src,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_RM_B:
    {
      if (o0->isRegMem())
      {
        const Operand& op = reinterpret_cast<const Operand&>(*o0);

        // Only BYTE register or BYTE/TYPELESS memory location can be used.
        ASMJIT_ASSERT(op.getSize() <= 1);
        
        _emitX86RM(id->opCode[0], false, false, 0, op, 0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_RM:
    {
      if (o0->isRegMem())
      {
        const Operand& op = reinterpret_cast<const Operand&>(*o0);
        _emitX86RM(id->opCode[0] + (op.getSize() != 1),
          op.getSize() == 2,
          op.getSize() == 8, (uint8_t)id->opCodeR, op,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_RM_R:
    {
      if (o0->isRegMem() && o1->isReg())
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);
        const GPReg& src = reinterpret_cast<const GPReg&>(*o1);
        _emitX86RM(id->opCode[0] + (src.getSize() != 1),
          src.getRegType() == REG_TYPE_GPW,
          src.getRegType() == REG_TYPE_GPQ, src.getRegCode(), dst,
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_REP:
    {
      uint32_t opCode = id->opCode[0];
      uint32_t opSize = id->opCode[1];

      // Emit REP prefix (1 BYTE).
      _emitByte(opCode >> 24);

      if (opSize != 1) opCode++; // D, Q and W form.
      if (opSize == 2) _emitByte(0x66); // 16-bit prefix.
#if defined(ASMJIT_X64)
      else if (opSize == 8) _emitByte(0x48); // REX.W prefix.
#endif // ASMJIT_X64

      // Emit opcode (1 BYTE).
      _emitByte(opCode & 0xFF);
      _FINISHED();
    }

    case InstructionDescription::G_RET:
    {
      if (o0->isNone())
      {
        _emitByte(0xC3);
        _FINISHED();
      }
      else if (o0->isImm())
      {
        const Imm& imm = reinterpret_cast<const Imm&>(*o0);
        ASMJIT_ASSERT(Util::isUInt16(imm.getValue()));

        if (imm.getValue() == 0)
        {
          _emitByte(0xC3);
          _FINISHED();
        }
        else
        {
          _emitByte(0xC2);
          _FINISHED_IMMEDIATE(&imm, 2);
        }
      }

      break;
    }

    case InstructionDescription::G_ROT:
    {
      if (o0->isRegMem() && (o1->isRegCode(REG_CL) || o1->isImm()))
      {
        // generate opcode. For these operations is base 0xC0 or 0xD0.
        bool useImm8 = o1->isImm() && reinterpret_cast<const Imm&>(*o1).getValue() != 1;
        uint32_t opCode = useImm8 ? 0xC0 : 0xD0;

        // size and operand type modifies the opcode
        if (o0->getSize() != 1) opCode |= 0x01;
        if (o1->getType() == OPERAND_REG) opCode |= 0x02;

        _emitX86RM(opCode,
          o0->getSize() == 2,
          o0->getSize() == 8,
          (uint8_t)id->opCodeR, reinterpret_cast<const Operand&>(*o0),
          useImm8 ? 1 : 0, forceRexPrefix);

        if (useImm8)
          _FINISHED_IMMEDIATE(o1, 1);
        else
          _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_SHLD_SHRD:
    {
      if (o0->isRegMem() && o1->isReg() && (o2->isImm() || (o2->isReg() && o2->isRegCode(REG_CL))))
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);
        const GPReg& src1 = reinterpret_cast<const GPReg&>(*o1);
        const Operand& src2 = reinterpret_cast<const Operand&>(*o2);

        ASMJIT_ASSERT(dst.getSize() == src1.getSize());

        _emitX86RM(id->opCode[0] + src2.isReg(),
          src1.isRegType(REG_TYPE_GPW),
          src1.isRegType(REG_TYPE_GPQ),
          src1.getRegCode(), dst,
          src2.isImm() ? 1 : 0, forceRexPrefix);
        if (src2.isImm())
          _FINISHED_IMMEDIATE(&src2, 1);
        else
          _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_TEST:
    {
      if (o0->isRegMem() && o1->isReg())
      {
        ASMJIT_ASSERT(o0->getSize() == o1->getSize());
        _emitX86RM(0x84 + (o1->getSize() != 1),
          o1->getSize() == 2, o1->getSize() == 8,
          reinterpret_cast<const BaseReg&>(*o1).getRegCode(),
          reinterpret_cast<const Operand&>(*o0),
          0, forceRexPrefix);
        _FINISHED();
      }

      if (o0->isRegIndex(0) && o1->isImm())
      {
        immSize = o0->getSize() <= 4 ? o0->getSize() : 4;

        if (o0->getSize() == 2) _emitByte(0x66); // 16-bit.
#if defined(ASMJIT_X64)
        _emitRexRM(o0->getSize() == 8, 0, reinterpret_cast<const Operand&>(*o0), forceRexPrefix);
#endif // ASMJIT_X64
        _emitByte(0xA8 + (o0->getSize() != 1));
        _FINISHED_IMMEDIATE(o1, immSize);
      }

      if (o0->isRegMem() && o1->isImm())
      {
        immSize = o0->getSize() <= 4 ? o0->getSize() : 4;

        if (o0->getSize() == 2) _emitByte(0x66); // 16-bit.
        _emitSegmentPrefix(reinterpret_cast<const Operand&>(*o0)); // Segment prefix.
#if defined(ASMJIT_X64)
        _emitRexRM(o0->getSize() == 8, 0, reinterpret_cast<const Operand&>(*o0), forceRexPrefix);
#endif // ASMJIT_X64
        _emitByte(0xF6 + (o0->getSize() != 1));
        _emitModRM(0, reinterpret_cast<const Operand&>(*o0), immSize);
        _FINISHED_IMMEDIATE(o1, immSize);
      }

      break;
    }

    case InstructionDescription::G_XCHG:
    {
      if (o0->isRegMem() && o1->isReg())
      {
        const Operand& dst = reinterpret_cast<const Operand&>(*o0);
        const GPReg& src = reinterpret_cast<const GPReg&>(*o1);

        if (src.isRegType(REG_TYPE_GPW)) _emitByte(0x66); // 16-bit.
        _emitSegmentPrefix(dst); // segment prefix
#if defined(ASMJIT_X64)
        _emitRexRM(src.isRegType(REG_TYPE_GPQ), src.getRegCode(), dst, forceRexPrefix);
#endif // ASMJIT_X64

        // Special opcode for index 0 registers (AX, EAX, RAX vs register).
        if ((dst.getType() == OPERAND_REG && dst.getSize() > 1) &&
            (reinterpret_cast<const GPReg&>(dst).getRegCode() == 0 ||
             reinterpret_cast<const GPReg&>(src).getRegCode() == 0 ))
        {
          uint8_t index = reinterpret_cast<const GPReg&>(dst).getRegCode() | src.getRegCode();
          _emitByte(0x90 + index);
          _FINISHED();
        }

        _emitByte(0x86 + (src.getSize() != 1));
        _emitModRM(src.getRegCode(), dst, 0);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_MOVBE:
    {
      if (o0->isReg() && o1->isMem())
      {
        _emitX86RM(0x000F38F0,
          o0->isRegType(REG_TYPE_GPW),
          o0->isRegType(REG_TYPE_GPQ),
          reinterpret_cast<const GPReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1),
          0, forceRexPrefix);
        _FINISHED();
      }

      if (o0->isMem() && o1->isReg())
      {
        _emitX86RM(0x000F38F1,
          o1->isRegType(REG_TYPE_GPW),
          o1->isRegType(REG_TYPE_GPQ),
          reinterpret_cast<const GPReg&>(*o1).getRegCode(),
          reinterpret_cast<const Mem&>(*o0),
          0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_X87_FPU:
    {
      if (o0->isRegType(REG_TYPE_X87))
      {
        uint8_t i1 = reinterpret_cast<const X87Reg&>(*o0).getRegIndex();
        uint8_t i2 = 0;

        if (code != INST_FCOM && code != INST_FCOMP)
        {
          if (!o1->isRegType(REG_TYPE_X87))
            goto illegalInstruction;
          i2 = reinterpret_cast<const X87Reg&>(*o1).getRegIndex();
        }
        else if (i1 != 0 && i2 != 0)
        {
          goto illegalInstruction;
        }

        _emitByte(i1 == 0
          ? ((id->opCode[0] & 0xFF000000) >> 24)
          : ((id->opCode[0] & 0x00FF0000) >> 16));
        _emitByte(i1 == 0
          ? ((id->opCode[0] & 0x0000FF00) >>  8) + i2
          : ((id->opCode[0] & 0x000000FF)      ) + i1);
        _FINISHED();
      }

      if (o0->isMem() && (o0->getSize() == 4 || o0->getSize() == 8) && o1->isNone())
      {
        const Mem& m = reinterpret_cast<const Mem&>(*o0);

        // segment prefix
        _emitSegmentPrefix(m);

        _emitByte(o0->getSize() == 4
          ? ((id->opCode[0] & 0xFF000000) >> 24)
          : ((id->opCode[0] & 0x00FF0000) >> 16));
        _emitModM((uint8_t)id->opCodeR, m, 0);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_X87_STI:
    {
      if (o0->isRegType(REG_TYPE_X87))
      {
        uint8_t i = reinterpret_cast<const X87Reg&>(*o0).getRegIndex();
        _emitByte((uint8_t)((id->opCode[0] & 0x0000FF00) >> 8));
        _emitByte((uint8_t)((id->opCode[0] & 0x000000FF) + i));
        _FINISHED();
      }
      break;
    }

    case InstructionDescription::G_X87_FSTSW:
    {
      if (o0->isReg() &&
          reinterpret_cast<const BaseReg&>(*o0).getRegType() <= REG_TYPE_GPQ &&
          reinterpret_cast<const BaseReg&>(*o0).getRegIndex() == 0)
      {
        _emitOpCode(id->opCode[1]);
        _FINISHED();
      }

      if (o0->isMem())
      {
        _emitX86RM(id->opCode[0], 0, 0, (uint8_t)id->opCodeR, reinterpret_cast<const Mem&>(*o0), 0, forceRexPrefix);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_X87_MEM_STI:
    {
      if (o0->isRegType(REG_TYPE_X87))
      {
        _emitByte((uint8_t)((id->opCode[1] & 0xFF000000) >> 24));
        _emitByte((uint8_t)((id->opCode[1] & 0x00FF0000) >> 16) +
          reinterpret_cast<const X87Reg&>(*o0).getRegIndex());
        _FINISHED();
      }

      // ... fall through to G_X87_MEM ...
    }

    case InstructionDescription::G_X87_MEM:
    {
      if (!o0->isMem())
        goto illegalInstruction;
      const Mem& m = reinterpret_cast<const Mem&>(*o0);

      uint8_t opCode = 0x00, mod = 0;

      if (o0->getSize() == 2 && (id->oflags[0] & InstructionDescription::O_FM_2))
      {
        opCode = (uint8_t)((id->opCode[0] & 0xFF000000) >> 24);
        mod    = (uint8_t)id->opCodeR;
      }
      if (o0->getSize() == 4 && (id->oflags[0] & InstructionDescription::O_FM_4))
      {
        opCode = (uint8_t)((id->opCode[0] & 0x00FF0000) >> 16);
        mod    = (uint8_t)id->opCodeR;
      }
      if (o0->getSize() == 8 && (id->oflags[0] & InstructionDescription::O_FM_8))
      {
        opCode = (uint8_t)((id->opCode[0] & 0x0000FF00) >>  8);
        mod    = (uint8_t)((id->opCode[0] & 0x000000FF)      );
      }

      if (opCode)
      {
        _emitSegmentPrefix(m);
        _emitByte(opCode);
        _emitModM(mod, m, 0);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_MMU_MOV:
    {
      ASMJIT_ASSERT(id->oflags[0] != 0);
      ASMJIT_ASSERT(id->oflags[1] != 0);

      // Check parameters (X)MM|GP32_64 <- (X)MM|GP32_64|Mem|Imm
      if ((o0->isMem()                 && (id->oflags[0] & InstructionDescription::O_MEM) == 0) ||
          (o0->isRegType(REG_TYPE_MM ) && (id->oflags[0] & InstructionDescription::O_MM ) == 0) ||
          (o0->isRegType(REG_TYPE_XMM) && (id->oflags[0] & InstructionDescription::O_XMM) == 0) ||
          (o0->isRegType(REG_TYPE_GPD) && (id->oflags[0] & InstructionDescription::O_GD ) == 0) ||
          (o0->isRegType(REG_TYPE_GPQ) && (id->oflags[0] & InstructionDescription::O_GQ ) == 0) ||
          (o1->isRegType(REG_TYPE_MM ) && (id->oflags[1] & InstructionDescription::O_MM ) == 0) ||
          (o1->isRegType(REG_TYPE_XMM) && (id->oflags[1] & InstructionDescription::O_XMM) == 0) ||
          (o1->isRegType(REG_TYPE_GPD) && (id->oflags[1] & InstructionDescription::O_GD ) == 0) ||
          (o1->isRegType(REG_TYPE_GPQ) && (id->oflags[1] & InstructionDescription::O_GQ ) == 0) ||
          (o1->isMem()                 && (id->oflags[1] & InstructionDescription::O_MEM) == 0) )
      {
        goto illegalInstruction;
      }

      // Illegal.
      if (o0->isMem() && o1->isMem())
        goto illegalInstruction;

      uint8_t rexw = ((id->oflags[0] | id->oflags[1]) & InstructionDescription::O_NOREX)
        ? 0
        : o0->isRegType(REG_TYPE_GPQ) | o1->isRegType(REG_TYPE_GPQ);

      // (X)MM|Reg <- (X)MM|Reg
      if (o0->isReg() && o1->isReg())
      {
        _emitMmu(id->opCode[0], rexw,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const BaseReg&>(*o1),
          0);
        _FINISHED();
      }

      // (X)MM|Reg <- Mem
      if (o0->isReg() && o1->isMem())
      {
        _emitMmu(id->opCode[0], rexw,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1),
          0);
        _FINISHED();
      }

      // Mem <- (X)MM|Reg
      if (o0->isMem() && o1->isReg())
      {
        _emitMmu(id->opCode[1], rexw,
          reinterpret_cast<const BaseReg&>(*o1).getRegCode(),
          reinterpret_cast<const Mem&>(*o0),
          0);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_MMU_MOVD:
    {
      if ((o0->isRegType(REG_TYPE_MM) || o0->isRegType(REG_TYPE_XMM)) && (o1->isRegType(REG_TYPE_GPD) || o1->isMem()))
      {
        _emitMmu(o0->isRegType(REG_TYPE_XMM) ? 0x66000F6E : 0x00000F6E, 0,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const Operand&>(*o1),
          0);
        _FINISHED();
      }

      if ((o0->isRegType(REG_TYPE_GPD) || o0->isMem()) && (o1->isRegType(REG_TYPE_MM) || o1->isRegType(REG_TYPE_XMM)))
      {
        _emitMmu(o1->isRegType(REG_TYPE_XMM) ? 0x66000F7E : 0x00000F7E, 0,
          reinterpret_cast<const BaseReg&>(*o1).getRegCode(),
          reinterpret_cast<const Operand&>(*o0),
          0);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_MMU_MOVQ:
    {
      if (o0->isRegType(REG_TYPE_MM) && o1->isRegType(REG_TYPE_MM))
      {
        _emitMmu(0x00000F6F, 0,
          reinterpret_cast<const MMReg&>(*o0).getRegCode(),
          reinterpret_cast<const MMReg&>(*o1),
          0);
        _FINISHED();
      }

      if (o0->isRegType(REG_TYPE_XMM) && o1->isRegType(REG_TYPE_XMM))
      {
        _emitMmu(0xF3000F7E, 0,
          reinterpret_cast<const XMMReg&>(*o0).getRegCode(),
          reinterpret_cast<const XMMReg&>(*o1),
          0);
        _FINISHED();
      }

      // Convenience - movdq2q
      if (o0->isRegType(REG_TYPE_MM) && o1->isRegType(REG_TYPE_XMM))
      {
        _emitMmu(0xF2000FD6, 0,
          reinterpret_cast<const MMReg&>(*o0).getRegCode(),
          reinterpret_cast<const XMMReg&>(*o1),
          0);
        _FINISHED();
      }

      // Convenience - movq2dq
      if (o0->isRegType(REG_TYPE_XMM) && o1->isRegType(REG_TYPE_MM))
      {
        _emitMmu(0xF3000FD6, 0,
          reinterpret_cast<const XMMReg&>(*o0).getRegCode(),
          reinterpret_cast<const MMReg&>(*o1),
          0);
        _FINISHED();
      }

      if (o0->isRegType(REG_TYPE_MM) && o1->isMem())
      {
        _emitMmu(0x00000F6F, 0,
          reinterpret_cast<const MMReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1),
          0);
        _FINISHED();
      }

      if (o0->isRegType(REG_TYPE_XMM) && o1->isMem())
      {
        _emitMmu(0xF3000F7E, 0,
          reinterpret_cast<const XMMReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1),
          0);
        _FINISHED();
      }

      if (o0->isMem() && o1->isRegType(REG_TYPE_MM))
      {
        _emitMmu(0x00000F7F, 0,
          reinterpret_cast<const MMReg&>(*o1).getRegCode(),
          reinterpret_cast<const Mem&>(*o0),
          0);
        _FINISHED();
      }

      if (o0->isMem() && o1->isRegType(REG_TYPE_XMM))
      {
        _emitMmu(0x66000FD6, 0,
          reinterpret_cast<const XMMReg&>(*o1).getRegCode(),
          reinterpret_cast<const Mem&>(*o0),
          0);
        _FINISHED();
      }

#if defined(ASMJIT_X64)
      if ((o0->isRegType(REG_TYPE_MM) || o0->isRegType(REG_TYPE_XMM)) && (o1->isRegType(REG_TYPE_GPQ) || o1->isMem()))
      {
        _emitMmu(o0->isRegType(REG_TYPE_XMM) ? 0x66000F6E : 0x00000F6E, 1,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const Operand&>(*o1),
          0);
        _FINISHED();
      }

      if ((o0->isRegType(REG_TYPE_GPQ) || o0->isMem()) && (o1->isRegType(REG_TYPE_MM) || o1->isRegType(REG_TYPE_XMM)))
      {
        _emitMmu(o1->isRegType(REG_TYPE_XMM) ? 0x66000F7E : 0x00000F7E, 1,
          reinterpret_cast<const BaseReg&>(*o1).getRegCode(),
          reinterpret_cast<const Operand&>(*o0),
          0);
        _FINISHED();
      }
#endif // ASMJIT_X64

      break;
    }

    case InstructionDescription::G_MMU_PREFETCH:
    {
      if (o0->isMem() && o1->isImm())
      {
        const Mem& mem = reinterpret_cast<const Mem&>(*o0);
        const Imm& hint = reinterpret_cast<const Imm&>(*o1);

        _emitMmu(0x00000F18, 0, (uint8_t)hint.getValue(), mem, 0);
        _FINISHED();
      }

      break;
    }

    case InstructionDescription::G_MMU_PEXTR:
    {
      if (!(o0->isRegMem() &&
           (o1->isRegType(REG_TYPE_XMM) || (code == INST_PEXTRW && o1->isRegType(REG_TYPE_MM))) &&
            o2->isImm()))
      {
        goto illegalInstruction;
      }

      uint32_t opCode = id->opCode[0];
      uint8_t isGpdGpq = o0->isRegType(REG_TYPE_GPD) | o0->isRegType(REG_TYPE_GPQ);

      if (code == INST_PEXTRB && (o0->getSize() != 0 && o0->getSize() != 1) && !isGpdGpq)
        goto illegalInstruction;
      if (code == INST_PEXTRW && (o0->getSize() != 0 && o0->getSize() != 2) && !isGpdGpq)
        goto illegalInstruction;
      if (code == INST_PEXTRD && (o0->getSize() != 0 && o0->getSize() != 4) && !isGpdGpq)
        goto illegalInstruction;
      if (code == INST_PEXTRQ && (o0->getSize() != 0 && o0->getSize() != 8) && !isGpdGpq)
        goto illegalInstruction;

      if (o1->isRegType(REG_TYPE_XMM)) opCode |= 0x66000000;

      if (o0->isReg())
      {
        _emitMmu(opCode, id->opCodeR | (uint8_t)o0->isRegType(REG_TYPE_GPQ),
          reinterpret_cast<const BaseReg&>(*o1).getRegCode(),
          reinterpret_cast<const BaseReg&>(*o0), 1);
        _FINISHED_IMMEDIATE(o2, 1);
      }

      if (o0->isMem())
      {
        _emitMmu(opCode, (uint8_t)id->opCodeR,
          reinterpret_cast<const BaseReg&>(*o1).getRegCode(),
          reinterpret_cast<const Mem&>(*o0), 1);
        _FINISHED_IMMEDIATE(o2, 1);
      }

      break;
    }

    case InstructionDescription::G_MMU_RMI:
    {
      ASMJIT_ASSERT(id->oflags[0] != 0);
      ASMJIT_ASSERT(id->oflags[1] != 0);

      // Check parameters (X)MM|GP32_64 <- (X)MM|GP32_64|Mem|Imm
      if (!o0->isReg() ||
          (o0->isRegType(REG_TYPE_MM ) && (id->oflags[0] & InstructionDescription::O_MM ) == 0) ||
          (o0->isRegType(REG_TYPE_XMM) && (id->oflags[0] & InstructionDescription::O_XMM) == 0) ||
          (o0->isRegType(REG_TYPE_GPD) && (id->oflags[0] & InstructionDescription::O_GD ) == 0) ||
          (o0->isRegType(REG_TYPE_GPQ) && (id->oflags[0] & InstructionDescription::O_GQ ) == 0) ||
          (o1->isRegType(REG_TYPE_MM ) && (id->oflags[1] & InstructionDescription::O_MM ) == 0) ||
          (o1->isRegType(REG_TYPE_XMM) && (id->oflags[1] & InstructionDescription::O_XMM) == 0) ||
          (o1->isRegType(REG_TYPE_GPD) && (id->oflags[1] & InstructionDescription::O_GD ) == 0) ||
          (o1->isRegType(REG_TYPE_GPQ) && (id->oflags[1] & InstructionDescription::O_GQ ) == 0) ||
          (o1->isMem()                 && (id->oflags[1] & InstructionDescription::O_MEM) == 0) ||
          (o1->isImm()                 && (id->oflags[1] & InstructionDescription::O_IMM) == 0))
      {
        goto illegalInstruction;
      }

      uint32_t prefix =
        ((id->oflags[0] & InstructionDescription::O_MM_XMM) == InstructionDescription::O_MM_XMM && o0->isRegType(REG_TYPE_XMM)) ||
        ((id->oflags[1] & InstructionDescription::O_MM_XMM) == InstructionDescription::O_MM_XMM && o1->isRegType(REG_TYPE_XMM))
          ? 0x66000000
          : 0x00000000;

      uint8_t rexw = ((id->oflags[0] | id->oflags[1]) & InstructionDescription::O_NOREX)
        ? 0
        : o0->isRegType(REG_TYPE_GPQ) | o1->isRegType(REG_TYPE_GPQ);

      // (X)MM <- (X)MM (opcode0)
      if (o1->isReg())
      {
        if ((id->oflags[1] & (InstructionDescription::O_MM_XMM | InstructionDescription::O_GQD)) == 0)
          goto illegalInstruction;
        _emitMmu(id->opCode[0] | prefix, rexw,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const BaseReg&>(*o1), 0);
        _FINISHED();
      }
      // (X)MM <- Mem (opcode0)
      if (o1->isMem())
      {
        if ((id->oflags[1] & InstructionDescription::O_MEM) == 0)
          goto illegalInstruction;
        _emitMmu(id->opCode[0] | prefix, rexw,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1), 0);
        _FINISHED();
      }
      // (X)MM <- Imm (opcode1+opcodeR)
      if (o1->isImm())
      {
        if ((id->oflags[1] & InstructionDescription::O_IMM) == 0)
          goto illegalInstruction;
        _emitMmu(id->opCode[1] | prefix, rexw,
          (uint8_t)id->opCodeR,
          reinterpret_cast<const BaseReg&>(*o0), 1);
        _FINISHED_IMMEDIATE(o1, 1);
      }

      break;
    }

    case InstructionDescription::G_MMU_RM_IMM8:
    {
      ASMJIT_ASSERT(id->oflags[0] != 0);
      ASMJIT_ASSERT(id->oflags[1] != 0);

      // Check parameters (X)MM|GP32_64 <- (X)MM|GP32_64|Mem|Imm
      if (!o0->isReg() ||
          (o0->isRegType(REG_TYPE_MM ) && (id->oflags[0] & InstructionDescription::O_MM ) == 0) ||
          (o0->isRegType(REG_TYPE_XMM) && (id->oflags[0] & InstructionDescription::O_XMM) == 0) ||
          (o0->isRegType(REG_TYPE_GPD) && (id->oflags[0] & InstructionDescription::O_GD ) == 0) ||
          (o0->isRegType(REG_TYPE_GPQ) && (id->oflags[0] & InstructionDescription::O_GQ ) == 0) ||
          (o1->isRegType(REG_TYPE_MM ) && (id->oflags[1] & InstructionDescription::O_MM ) == 0) ||
          (o1->isRegType(REG_TYPE_XMM) && (id->oflags[1] & InstructionDescription::O_XMM) == 0) ||
          (o1->isRegType(REG_TYPE_GPD) && (id->oflags[1] & InstructionDescription::O_GD ) == 0) ||
          (o1->isRegType(REG_TYPE_GPQ) && (id->oflags[1] & InstructionDescription::O_GQ ) == 0) ||
          (o1->isMem()                 && (id->oflags[1] & InstructionDescription::O_MEM) == 0) ||
          !o2->isImm())
      {
        goto illegalInstruction;
      }

      uint32_t prefix =
        ((id->oflags[0] & InstructionDescription::O_MM_XMM) == InstructionDescription::O_MM_XMM && o0->isRegType(REG_TYPE_XMM)) ||
        ((id->oflags[1] & InstructionDescription::O_MM_XMM) == InstructionDescription::O_MM_XMM && o1->isRegType(REG_TYPE_XMM))
          ? 0x66000000
          : 0x00000000;

      uint8_t rexw = ((id->oflags[0]|id->oflags[1]) & InstructionDescription::O_NOREX)
        ? 0
        : o0->isRegType(REG_TYPE_GPQ) | o1->isRegType(REG_TYPE_GPQ);

      // (X)MM <- (X)MM (opcode0)
      if (o1->isReg())
      {
        if ((id->oflags[1] & (InstructionDescription::O_MM_XMM | InstructionDescription::O_GQD)) == 0)
goto illegalInstruction;
        _emitMmu(id->opCode[0] | prefix, rexw,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const BaseReg&>(*o1), 1);
        _FINISHED_IMMEDIATE(o2, 1);
      }
      // (X)MM <- Mem (opcode0)
      if (o1->isMem())
      {
        if ((id->oflags[1] & InstructionDescription::O_MEM) == 0)
          goto illegalInstruction;
        _emitMmu(id->opCode[0] | prefix, rexw,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1), 1);
        _FINISHED_IMMEDIATE(o2, 1);
      }

      break;
    }

    case InstructionDescription::G_MMU_RM_3DNOW:
    {
      if (o0->isRegType(REG_TYPE_MM) && (o1->isRegType(REG_TYPE_MM) || o1->isMem()))
      {
        _emitMmu(id->opCode[0], 0,
          reinterpret_cast<const BaseReg&>(*o0).getRegCode(),
          reinterpret_cast<const Mem&>(*o1), 1);
        _emitByte((uint8_t)id->opCode[1]);
        _FINISHED();
      }

      break;
    }
  }

illegalInstruction:
  // Set an error. If we run in release mode assertion will be not used, so we
  // must inform about invalid state.
  setError(ERROR_ILLEGAL_INSTRUCTION);

#if defined(ASMJIT_DEBUG)
  assertIllegal = true;
#endif // ASMJIT_DEBUG
  goto end;

emitImmediate:
  {
    sysint_t value = immOperand->getValue();
    switch (immSize)
    {
      case 1: _emitByte ((uint8_t )(sysuint_t)value); break;
      case 2: _emitWord ((uint16_t)(sysuint_t)value); break;
      case 4: _emitDWord((uint32_t)(sysuint_t)value); break;
#if defined(ASMJIT_X64)
      case 8: _emitQWord((uint64_t)(sysuint_t)value); break;
#endif // ASMJIT_X64
      default: ASMJIT_ASSERT(0);
    }
  }

end:
  if (_logger
#if defined(ASMJIT_DEBUG)
      || assertIllegal
#endif // ASMJIT_DEBUG
     )
  {
    char bufStorage[512];
    char* buf = bufStorage;

    // Detect truncated operand.
    Imm immTemporary(0);

    // Use the original operands, because BYTE some of them were replaced.
    if (bLoHiUsed)
    {
      o0 = _loggerOperands[0];
      o1 = _loggerOperands[1];
      o2 = _loggerOperands[2];
    }

    if (immOperand)
    {
      sysint_t value = immOperand->getValue();
      bool isUnsigned = immOperand->isUnsigned();

      switch (immSize)
      {
        case 1: if ( isUnsigned && !Util::isUInt8 (value)) { immTemporary.setValue((uint8_t)(sysuint_t)value, true ); break; }
                if (!isUnsigned && !Util::isInt8  (value)) { immTemporary.setValue((uint8_t)(sysuint_t)value, false); break; }
                break;
        case 2: if ( isUnsigned && !Util::isUInt16(value)) { immTemporary.setValue((uint16_t)(sysuint_t)value, true ); break; }
                if (!isUnsigned && !Util::isInt16 (value)) { immTemporary.setValue((uint16_t)(sysuint_t)value, false); break; }
                break;
        case 4: if ( isUnsigned && !Util::isUInt32(value)) { immTemporary.setValue((uint32_t)(sysuint_t)value, true ); break; }
                if (!isUnsigned && !Util::isInt32 (value)) { immTemporary.setValue((uint32_t)(sysuint_t)value, false); break; }
                break;
      }

      if (immTemporary.getValue() != 0)
      {
        if (o0 == immOperand) o0 = &immTemporary;
        if (o1 == immOperand) o1 = &immTemporary;
        if (o2 == immOperand) o2 = &immTemporary;
      }
    }

    buf = dumpInstruction(buf, code, _emitOptions, o0, o1, o2, memRegType);

    if (_logger->getLogBinary())
      buf = dumpComment(buf, (sysuint_t)(buf - bufStorage), getCode() + beginOffset, getOffset() - beginOffset, _comment);
    else
      buf = dumpComment(buf, (sysuint_t)(buf - bufStorage), NULL, 0, _comment);

    // We don't need to NULL terminate the resulting string.
#if defined(ASMJIT_DEBUG)
    if (_logger)
#endif // ASMJIT_DEBUG
      _logger->logString(bufStorage, (sysuint_t)(buf - bufStorage));

#if defined(ASMJIT_DEBUG)
    if (assertIllegal)
    {
      // Here we need to NULL terminate.
      buf[0] = '\0';

      // Raise an assertion failure, because this situation shouldn't happen.
      assertionFailure(__FILE__, __LINE__, bufStorage);
    }
#endif // ASMJIT_DEBUG
  }

cleanup:
  _comment = NULL;
  _emitOptions = 0;
}

void AssemblerCore::_emitJcc(uint32_t code, const Label* label, uint32_t hint) ASMJIT_NOTHROW
{
  if (!hint)
  {
    _emitInstruction(code, label, NULL, NULL);
  }
  else
  {
    Imm imm(hint);
    _emitInstruction(code, label, &imm, NULL);
  }
}

// ============================================================================
// [AsmJit::AssemblerCore - Relocation helpers]
// ============================================================================

sysuint_t AssemblerCore::relocCode(void* _dst, sysuint_t addressBase) const ASMJIT_NOTHROW
{
  // Copy code to virtual memory (this is a given _dst pointer).
  uint8_t* dst = reinterpret_cast<uint8_t*>(_dst);

  sysint_t coff = _buffer.getOffset();
  sysint_t csize = getCodeSize();

  // We are copying the exact size of the generated code. Extra code for trampolines
  // is generated on-the-fly by relocator (this code doesn't exist at the moment).
  memcpy(dst, _buffer.getData(), coff);

#if defined(ASMJIT_X64)
  // Trampoline pointer.
  uint8_t* tramp = dst + coff;
#endif // ASMJIT_X64

  // Relocate all recorded locations.
  sysint_t i;
  sysint_t len = _relocData.getLength();

  for (i = 0; i < len; i++)
  {
    const RelocData& r = _relocData[i];
    sysint_t val;

#if defined(ASMJIT_X64)
    // Whether to use trampoline, can be only used if relocation type is
    // ABSOLUTE_TO_RELATIVE_TRAMPOLINE.
    bool useTrampoline = false;
#endif // ASMJIT_X64

    // Be sure that reloc data structure is correct.
    ASMJIT_ASSERT((sysint_t)(r.offset + r.size) <= csize);

    switch (r.type)
    {
      case RelocData::ABSOLUTE_TO_ABSOLUTE:
        val = (sysint_t)(r.address);
        break;

      case RelocData::RELATIVE_TO_ABSOLUTE:
        val = (sysint_t)(addressBase + r.destination);
        break;

      case RelocData::ABSOLUTE_TO_RELATIVE:
      case RelocData::ABSOLUTE_TO_RELATIVE_TRAMPOLINE:
        val = (sysint_t)( (sysuint_t)r.address - (addressBase + (sysuint_t)r.offset + 4) );

#if defined(ASMJIT_X64)
        if (r.type == RelocData::ABSOLUTE_TO_RELATIVE_TRAMPOLINE && !Util::isInt32(val))
        {
          val = (sysint_t)( (sysuint_t)tramp - ((sysuint_t)_dst + (sysuint_t)r.offset + 4) );
          useTrampoline = true;
        }
#endif // ASMJIT_X64
        break;

      default:
        ASMJIT_ASSERT(0);
    }

    switch (r.size)
    {
      case 4:
        *reinterpret_cast<int32_t*>(dst + r.offset) = (int32_t)val;
        break;

      case 8:
        *reinterpret_cast<int64_t*>(dst + r.offset) = (int64_t)val;
        break;

      default:
        ASMJIT_ASSERT(0);
    }

#if defined(ASMJIT_X64)
    if (useTrampoline)
    {
      if (getLogger())
      {
        getLogger()->logFormat("; Trampoline from %p -> %p\n", (int8_t*)addressBase + r.offset, r.address);
      }

      TrampolineWriter::writeTrampoline(tramp, r.address);
      tramp += TrampolineWriter::TRAMPOLINE_SIZE;
    }
#endif // ASMJIT_X64
  }

#if defined(ASMJIT_X64)
  return (sysuint_t)(tramp - dst);
#else
  return (sysuint_t)coff;
#endif // ASMJIT_X64
}

// ============================================================================
// [AsmJit::AssemblerCore - Embed]
// ============================================================================

void AssemblerCore::embed(const void* data, sysuint_t size) ASMJIT_NOTHROW
{
  if (!canEmit()) return;

  if (_logger)
  {
    sysuint_t i, j;
    sysuint_t max;
    char buf[128];
    char dot[] = ".data ";
    char* p;

    memcpy(buf, dot, ASMJIT_ARRAY_SIZE(dot) - 1);

    for (i = 0; i < size; i += 16)
    {
      max = (size - i < 16) ? size - i : 16;
      p = buf + ASMJIT_ARRAY_SIZE(dot) - 1;

      for (j = 0; j < max; j++)
        p += sprintf(p, "%0.2X", reinterpret_cast<const uint8_t *>(data)[i+j]);

      *p++ = '\n';
      *p = '\0';

      _logger->logString(buf);
    }
  }

  _buffer.emitData(data, size);
}

void AssemblerCore::embedLabel(const Label& label) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(label.getId() != INVALID_VALUE);
  if (!canEmit()) return;

  LabelData& l_data = _labelData[label.getId() & OPERAND_ID_VALUE_MASK];
  RelocData r_data;

  if (_logger)
  {
    _logger->logFormat(sizeof(sysint_t) == 4 ? ".dd L.%u\n" : ".dq L.%u\n", (uint32_t)label.getId() & OPERAND_ID_VALUE_MASK);
  }

  r_data.type = RelocData::RELATIVE_TO_ABSOLUTE;
  r_data.size = sizeof(sysint_t);
  r_data.offset = getOffset();
  r_data.destination = 0;

  if (l_data.offset != -1)
  {
    // Bound label.
    r_data.destination = l_data.offset;
  }
  else
  {
    // Non-bound label. Need to chain.
    LabelLink* link = _newLabelLink();

    link->prev = (LabelLink*)l_data.links;
    link->offset = getOffset();
    link->displacement = 0;
    link->relocId = _relocData.getLength();

    l_data.links = link;
  }

  _relocData.append(r_data);

  // Emit dummy sysint (4 or 8 bytes that depends on address size).
  _emitSysInt(0);
}

// ============================================================================
// [AsmJit::AssemblerCore - Align]
// ============================================================================

void AssemblerCore::align(uint32_t m) ASMJIT_NOTHROW
{
  if (!canEmit()) return;
  if (_logger) _logger->logFormat(".align %u", (uint)m);

  if (!m) return;

  if (m > 64)
  {
    ASMJIT_ASSERT(0);
    return;
  }

  sysint_t i = m - (getOffset() % m);
  if (i == m) return;

  if (_properties & (1 << PROPERTY_OPTIMIZE_ALIGN))
  {
    const CpuInfo* ci = getCpuInfo();

    // NOPs optimized for Intel:
    //   Intel 64 and IA-32 Architectures Software Developer's Manual
    //   - Volume 2B 
    //   - Instruction Set Reference N-Z
    //     - NOP

    // NOPs optimized for AMD:
    //   Software Optimization Guide for AMD Family 10h Processors (Quad-Core)
    //   - 4.13 - Code Padding with Operand-Size Override and Multibyte NOP

    // Intel and AMD.
    static const uint8_t nop1[] = { 0x90 };
    static const uint8_t nop2[] = { 0x66, 0x90 };
    static const uint8_t nop3[] = { 0x0F, 0x1F, 0x00 };
    static const uint8_t nop4[] = { 0x0F, 0x1F, 0x40, 0x00 };
    static const uint8_t nop5[] = { 0x0F, 0x1F, 0x44, 0x00, 0x00 };
    static const uint8_t nop6[] = { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 };
    static const uint8_t nop7[] = { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t nop8[] = { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t nop9[] = { 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // AMD.
    static const uint8_t nop10[] = { 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t nop11[] = { 0x66, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

    const uint8_t* p;
    sysint_t n;

    if (ci->vendorId == CPU_VENDOR_INTEL && 
       ((ci->family & 0x0F) == 6 || 
        (ci->family & 0x0F) == 15)
       )
    {
      do {
        switch (i)
        {
          case  1: p = nop1; n = 1; break;
          case  2: p = nop2; n = 2; break;
          case  3: p = nop3; n = 3; break;
          case  4: p = nop4; n = 4; break;
          case  5: p = nop5; n = 5; break;
          case  6: p = nop6; n = 6; break;
          case  7: p = nop7; n = 7; break;
          case  8: p = nop8; n = 8; break;
          default: p = nop9; n = 9; break;
        }

        i -= n;
        do { _emitByte(*p++); } while(--n);
      } while (i);

      return;
    }

    if (ci->vendorId == CPU_VENDOR_AMD && 
        ci->family >= 0x0F)
    {
      do {
        switch (i)
        {
          case  1: p = nop1 ; n =  1; break;
          case  2: p = nop2 ; n =  2; break;
          case  3: p = nop3 ; n =  3; break;
          case  4: p = nop4 ; n =  4; break;
          case  5: p = nop5 ; n =  5; break;
          case  6: p = nop6 ; n =  6; break;
          case  7: p = nop7 ; n =  7; break;
          case  8: p = nop8 ; n =  8; break;
          case  9: p = nop9 ; n =  9; break;
          case 10: p = nop10; n = 10; break;
          default: p = nop11; n = 11; break;
        }

        i -= n;
        do { _emitByte(*p++); } while(--n);
      } while (i);

      return;
    }
#if defined(ASMJIT_X86)
    // legacy NOPs, 0x90 with 0x66 prefix.
    do {
      switch (i)
      {
        default: _emitByte(0x66); i--;
        case  3: _emitByte(0x66); i--;
        case  2: _emitByte(0x66); i--;
        case  1: _emitByte(0x90); i--;
      }
    } while(i);
#endif
  }

  // legacy NOPs, only 0x90
  // In 64-bit mode, we can't use 0x66 prefix
  do {
    _emitByte(0x90);
  } while(--i);
}

// ============================================================================
// [AsmJit::AssemblerCore - Label]
// ============================================================================

Label AssemblerCore::newLabel() ASMJIT_NOTHROW
{
  Label label;
  label._base.id = (uint32_t)_labelData.getLength() | OPERAND_ID_TYPE_LABEL;

  LabelData l_data;
  l_data.offset = -1;
  l_data.links = NULL;
  _labelData.append(l_data);

  return label;
}

void AssemblerCore::registerLabels(sysuint_t count) ASMJIT_NOTHROW
{
  // Duplicated newLabel() code, but we are not creating Label instances.
  LabelData l_data;
  l_data.offset = -1;
  l_data.links = NULL;

  for (sysuint_t i = 0; i < count; i++) _labelData.append(l_data);
}

void AssemblerCore::bind(const Label& label) ASMJIT_NOTHROW
{
  // Only labels created by newLabel() can be used by Assembler.
  ASMJIT_ASSERT(label.getId() != INVALID_VALUE);
  // Never go out of bounds.
  ASMJIT_ASSERT((label.getId() & OPERAND_ID_VALUE_MASK) < _labelData.getLength());

  // Get label data based on label id.
  LabelData& l_data = _labelData[label.getId() & OPERAND_ID_VALUE_MASK];

  // Label can be bound only once.
  ASMJIT_ASSERT(l_data.offset == -1);

  // Log.
  if (_logger) _logger->logFormat("L.%u:\n", (uint32_t)label.getId() & OPERAND_ID_VALUE_MASK);

  sysint_t pos = getOffset();

  LabelLink* link = l_data.links;
  LabelLink* prev = NULL;

  while (link)
  {
    sysint_t offset = link->offset;

    if (link->relocId != -1)
    {
      // If linked label points to RelocData then instead of writing relative
      // displacement to assembler stream, we will write it to RelocData.
      _relocData[link->relocId].destination += pos;
    }
    else
    {
      // Not using relocId, this means that we overwriting real displacement
      // in assembler stream.
      int32_t patchedValue = (int32_t)(pos - offset + link->displacement);
      uint32_t size = getByteAt(offset);

      // Only these size specifiers are allowed.
      ASMJIT_ASSERT(size == 1 || size == 4);

      if (size == 4)
      {
        setInt32At(offset, patchedValue);
      }
      else // if (size == 1)
      {
        if (Util::isInt8(patchedValue))
        {
          setByteAt(offset, (uint8_t)(int8_t)patchedValue);
        }
        else
        {
          // Fatal error.
          setError(ERROR_ILLEGAL_SHORT_JUMP);
        }
      }
    }

    prev = link->prev;
    link = prev;
  }

  // Chain unused links.
  link = l_data.links;
  if (link)
  {
    if (prev == NULL) prev = link;

    prev->prev = _unusedLinks;
    _unusedLinks = link;
  }

  // Unlink label if it was linked.
  l_data.offset = pos;
  l_data.links = NULL;
}

// ============================================================================
// [AsmJit::AssemblerCore - Make]
// ============================================================================

void* AssemblerCore::make() ASMJIT_NOTHROW
{
  // Do nothing on error state or when no instruction was emitted.
  if (_error || getCodeSize() == 0)
    return NULL;

  void* p;
  _error = _codeGenerator->generate(&p, reinterpret_cast<Assembler*>(this));
  return p;
}

// ============================================================================
// [AsmJit::AssemblerCore - Links]
// ============================================================================

AssemblerCore::LabelLink* AssemblerCore::_newLabelLink() ASMJIT_NOTHROW
{
  LabelLink* link = _unusedLinks;

  if (link)
  {
    _unusedLinks = link->prev;
  }
  else
  {
    link = (LabelLink*)_zone.zalloc(sizeof(LabelLink));
    if (link == NULL) return NULL;
  }

  // clean link
  link->prev = NULL;
  link->offset = 0;
  link->displacement = 0;
  link->relocId = -1;

  return link;
}

// ============================================================================
// [AsmJit::Assembler - Construction / Destruction]
// ============================================================================

Assembler::Assembler(CodeGenerator* codeGenerator) ASMJIT_NOTHROW :
  AssemblerIntrinsics(codeGenerator)
{
}

Assembler::~Assembler() ASMJIT_NOTHROW
{
}

#pragma warning(default : 4701 4389 4245 4244 4100)

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"
