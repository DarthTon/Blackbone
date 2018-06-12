// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/assembler.h"
#include "../base/utils.h"
#include "../base/vmem.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::ErrorHandler]
// ============================================================================

ErrorHandler::ErrorHandler() noexcept {}
ErrorHandler::~ErrorHandler() noexcept {}

// ============================================================================
// [asmjit::CodeHolder - Utilities]
// ============================================================================

static void CodeHolder_setGlobalOption(CodeHolder* self, uint32_t clear, uint32_t add) noexcept {
  // Modify global options of `CodeHolder` itself.
  self->_globalOptions = (self->_globalOptions & ~clear) | add;

  // Modify all global options of all `CodeEmitter`s attached.
  CodeEmitter* emitter = self->_emitters;
  while (emitter) {
    emitter->_globalOptions = (emitter->_globalOptions & ~clear) | add;
    emitter = emitter->_nextEmitter;
  }
}

static void CodeHolder_resetInternal(CodeHolder* self, bool releaseMemory) noexcept {
  // Detach all `CodeEmitter`s.
  while (self->_emitters)
    self->detach(self->_emitters);

  // Reset everything into its construction state.
  self->_codeInfo.reset();
  self->_globalHints = 0;
  self->_globalOptions = 0;
  self->_logger = nullptr;
  self->_errorHandler = nullptr;

  self->_unresolvedLabelsCount = 0;
  self->_trampolinesSize = 0;

  // Reset all sections.
  size_t numSections = self->_sections.getLength();
  for (size_t i = 0; i < numSections; i++) {
    SectionEntry* section = self->_sections[i];
    if (section->_buffer.hasData() && !section->_buffer.isExternal())
      Internal::releaseMemory(section->_buffer._data);
    section->_buffer._data = nullptr;
    section->_buffer._capacity = 0;
  }

  // Reset zone allocator and all containers using it.
  ZoneHeap* heap = &self->_baseHeap;

  self->_namedLabels.reset(heap);
  self->_relocations.reset();
  self->_labels.reset();
  self->_sections.reset();

  heap->reset(&self->_baseZone);
  self->_baseZone.reset(releaseMemory);
}

// ============================================================================
// [asmjit::CodeHolder - Construction / Destruction]
// ============================================================================

CodeHolder::CodeHolder() noexcept
  : _codeInfo(),
    _globalHints(0),
    _globalOptions(0),
    _emitters(nullptr),
    _cgAsm(nullptr),
    _logger(nullptr),
    _errorHandler(nullptr),
    _unresolvedLabelsCount(0),
    _trampolinesSize(0),
    _baseZone(16384 - Zone::kZoneOverhead),
    _dataZone(16384 - Zone::kZoneOverhead),
    _baseHeap(&_baseZone),
    _namedLabels(&_baseHeap) {}

CodeHolder::~CodeHolder() noexcept {
  CodeHolder_resetInternal(this, true);
}

// ============================================================================
// [asmjit::CodeHolder - Init / Reset]
// ============================================================================

Error CodeHolder::init(const CodeInfo& info) noexcept {
  // Cannot reinitialize if it's locked or there is one or more CodeEmitter
  // attached.
  if (isInitialized())
    return DebugUtils::errored(kErrorAlreadyInitialized);

  // If we are just initializing there should be no emitters attached).
  ASMJIT_ASSERT(_emitters == nullptr);

  // Create the default section and insert it to the `_sections` array.
  Error err = _sections.willGrow(&_baseHeap);
  if (err == kErrorOk) {
    SectionEntry* se = _baseZone.allocZeroedT<SectionEntry>();
    if (ASMJIT_LIKELY(se)) {
      se->_flags = SectionEntry::kFlagExec | SectionEntry::kFlagConst;
      se->_setDefaultName('.', 't', 'e', 'x', 't');
      _sections.appendUnsafe(se);
    }
    else {
      err = DebugUtils::errored(kErrorNoHeapMemory);
    }
  }

  if (ASMJIT_UNLIKELY(err)) {
    _baseZone.reset(false);
    return err;
  }
  else {
    _codeInfo = info;
    return kErrorOk;
  }
}

void CodeHolder::reset(bool releaseMemory) noexcept {
  CodeHolder_resetInternal(this, releaseMemory);
}

// ============================================================================
// [asmjit::CodeHolder - Attach / Detach]
// ============================================================================

Error CodeHolder::attach(CodeEmitter* emitter) noexcept {
  // Catch a possible misuse of the API.
  if (!emitter)
    return DebugUtils::errored(kErrorInvalidArgument);

  uint32_t type = emitter->getType();
  if (type == CodeEmitter::kTypeNone || type >= CodeEmitter::kTypeCount)
    return DebugUtils::errored(kErrorInvalidState);

  // This is suspicious, but don't fail if `emitter` matches.
  if (emitter->_code != nullptr) {
    if (emitter->_code == this) return kErrorOk;
    return DebugUtils::errored(kErrorInvalidState);
  }

  // Special case - attach `Assembler`.
  CodeEmitter** pSlot = nullptr;
  if (type == CodeEmitter::kTypeAssembler) {
    if (_cgAsm)
      return DebugUtils::errored(kErrorSlotOccupied);
    pSlot = reinterpret_cast<CodeEmitter**>(&_cgAsm);
  }

  Error err = emitter->onAttach(this);
  if (err != kErrorOk) return err;

  // Add to a single-linked list of `CodeEmitter`s.
  emitter->_nextEmitter = _emitters;
  _emitters = emitter;
  if (pSlot) *pSlot = emitter;

  // Establish the connection.
  emitter->_code = this;
  return kErrorOk;
}

Error CodeHolder::detach(CodeEmitter* emitter) noexcept {
  if (!emitter)
    return DebugUtils::errored(kErrorInvalidArgument);

  if (emitter->_code != this)
    return DebugUtils::errored(kErrorInvalidState);

  uint32_t type = emitter->getType();
  Error err = kErrorOk;

  // NOTE: We always detach if we were asked to, if error happens during
  // `emitter->onDetach()` we just propagate it, but the CodeEmitter will
  // be detached.
  if (!emitter->_destroyed) {
    if (type == CodeEmitter::kTypeAssembler)
      static_cast<Assembler*>(emitter)->sync();
    err = emitter->onDetach(this);
  }

  // Special case - detach `Assembler`.
  if (type == CodeEmitter::kTypeAssembler)
    _cgAsm = nullptr;

  // Remove from a single-linked list of `CodeEmitter`s.
  CodeEmitter** pPrev = &_emitters;
  for (;;) {
    ASMJIT_ASSERT(*pPrev != nullptr);
    CodeEmitter* cur = *pPrev;

    if (cur == emitter) {
      *pPrev = emitter->_nextEmitter;
      break;
    }

    pPrev = &cur->_nextEmitter;
  }

  emitter->_code = nullptr;
  emitter->_nextEmitter = nullptr;

  return err;
}

// ============================================================================
// [asmjit::CodeHolder - Sync]
// ============================================================================

void CodeHolder::sync() noexcept {
  if (_cgAsm) _cgAsm->sync();
}

// ============================================================================
// [asmjit::CodeHolder - Result Information]
// ============================================================================

size_t CodeHolder::getCodeSize() const noexcept {
  // Reflect all changes first.
  const_cast<CodeHolder*>(this)->sync();

  // TODO: Support sections.
  return _sections[0]->_buffer._length + getTrampolinesSize();
}

// ============================================================================
// [asmjit::CodeHolder - Logging & Error Handling]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGING)
void CodeHolder::setLogger(Logger* logger) noexcept {
  uint32_t opt = 0;
  if (logger) opt = CodeEmitter::kOptionLoggingEnabled;

  _logger = logger;
  CodeHolder_setGlobalOption(this, CodeEmitter::kOptionLoggingEnabled, opt);
}
#endif // !ASMJIT_DISABLE_LOGGING

Error CodeHolder::setErrorHandler(ErrorHandler* handler) noexcept {
  _errorHandler = handler;
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeHolder - Sections]
// ============================================================================

static Error CodeHolder_reserveInternal(CodeHolder* self, CodeBuffer* cb, size_t n) noexcept {
  uint8_t* oldData = cb->_data;
  uint8_t* newData;

  if (oldData && !cb->isExternal())
    newData = static_cast<uint8_t*>(Internal::reallocMemory(oldData, n));
  else
    newData = static_cast<uint8_t*>(Internal::allocMemory(n));

  if (ASMJIT_UNLIKELY(!newData))
    return DebugUtils::errored(kErrorNoHeapMemory);

  cb->_data = newData;
  cb->_capacity = n;

  // Update the `Assembler` pointers if attached. Maybe we should introduce an
  // event for this, but since only one Assembler can be attached at a time it
  // should not matter how these pointers are updated.
  Assembler* a = self->_cgAsm;
  if (a && &a->_section->_buffer == cb) {
    size_t offset = a->getOffset();

    a->_bufferData = newData;
    a->_bufferEnd  = newData + n;
    a->_bufferPtr  = newData + offset;
  }

  return kErrorOk;
}

Error CodeHolder::growBuffer(CodeBuffer* cb, size_t n) noexcept {
  // This is most likely called by `Assembler` so `sync()` shouldn't be needed,
  // however, if this is called by the user and the currently attached Assembler
  // did generate some code we could lose that, so sync now and make sure the
  // section length is updated.
  if (_cgAsm) _cgAsm->sync();

  // Now the length of the section must be valid.
  size_t length = cb->getLength();
  if (ASMJIT_UNLIKELY(n > IntTraits<uintptr_t>::maxValue() - length))
    return DebugUtils::errored(kErrorNoHeapMemory);

  // We can now check if growing the buffer is really necessary. It's unlikely
  // that this function is called while there is still room for `n` bytes.
  size_t capacity = cb->getCapacity();
  size_t required = cb->getLength() + n;
  if (ASMJIT_UNLIKELY(required <= capacity)) return kErrorOk;

  if (cb->isFixedSize())
    return DebugUtils::errored(kErrorCodeTooLarge);

  if (capacity < 8096)
    capacity = 8096;
  else
    capacity += Globals::kAllocOverhead;

  do {
    size_t old = capacity;
    if (capacity < Globals::kAllocThreshold)
      capacity *= 2;
    else
      capacity += Globals::kAllocThreshold;

    if (capacity < Globals::kAllocThreshold)
      capacity *= 2;
    else
      capacity += Globals::kAllocThreshold;

    // Overflow.
    if (ASMJIT_UNLIKELY(old > capacity))
      return DebugUtils::errored(kErrorNoHeapMemory);
  } while (capacity - Globals::kAllocOverhead < required);

  return CodeHolder_reserveInternal(this, cb, capacity - Globals::kAllocOverhead);
}

Error CodeHolder::reserveBuffer(CodeBuffer* cb, size_t n) noexcept {
  size_t capacity = cb->getCapacity();
  if (n <= capacity) return kErrorOk;

  if (cb->isFixedSize())
    return DebugUtils::errored(kErrorCodeTooLarge);

  // We must sync, as mentioned in `growBuffer()` as well.
  if (_cgAsm) _cgAsm->sync();

  return CodeHolder_reserveInternal(this, cb, n);
}

// ============================================================================
// [asmjit::CodeHolder - Labels & Symbols]
// ============================================================================

namespace {

//! \internal
//!
//! Only used to lookup a label from `_namedLabels`.
class LabelByName {
public:
  ASMJIT_INLINE LabelByName(const char* name, size_t nameLength, uint32_t hVal) noexcept
    : name(name),
      nameLength(static_cast<uint32_t>(nameLength)) {}

  ASMJIT_INLINE bool matches(const LabelEntry* entry) const noexcept {
    return static_cast<uint32_t>(entry->getNameLength()) == nameLength &&
           ::memcmp(entry->getName(), name, nameLength) == 0;
  }

  const char* name;
  uint32_t nameLength;
  uint32_t hVal;
};

// Returns a hash of `name` and fixes `nameLength` if it's `Globals::kInvalidIndex`.
static uint32_t CodeHolder_hashNameAndFixLen(const char* name, size_t& nameLength) noexcept {
  uint32_t hVal = 0;
  if (nameLength == Globals::kInvalidIndex) {
    size_t i = 0;
    for (;;) {
      uint8_t c = static_cast<uint8_t>(name[i]);
      if (!c) break;
      hVal = Utils::hashRound(hVal, c);
      i++;
    }
    nameLength = i;
  }
  else {
    for (size_t i = 0; i < nameLength; i++) {
      uint8_t c = static_cast<uint8_t>(name[i]);
      if (ASMJIT_UNLIKELY(!c)) return DebugUtils::errored(kErrorInvalidLabelName);
      hVal = Utils::hashRound(hVal, c);
    }
  }
  return hVal;
}

} // anonymous namespace

LabelLink* CodeHolder::newLabelLink(LabelEntry* le, uint32_t sectionId, size_t offset, intptr_t rel) noexcept {
  LabelLink* link = _baseHeap.allocT<LabelLink>();
  if (ASMJIT_UNLIKELY(!link)) return nullptr;

  link->prev = le->_links;
  le->_links = link;

  link->sectionId = sectionId;
  link->relocId = RelocEntry::kInvalidId;
  link->offset = offset;
  link->rel = rel;

  _unresolvedLabelsCount++;
  return link;
}

Error CodeHolder::newLabelId(uint32_t& idOut) noexcept {
  idOut = 0;

  size_t index = _labels.getLength();
  if (ASMJIT_LIKELY(index >= Operand::kPackedIdCount))
    return DebugUtils::errored(kErrorLabelIndexOverflow);

  ASMJIT_PROPAGATE(_labels.willGrow(&_baseHeap));
  LabelEntry* le = _baseHeap.allocZeroedT<LabelEntry>();

  if (ASMJIT_UNLIKELY(!le))
    return DebugUtils::errored(kErrorNoHeapMemory);;

  uint32_t id = Operand::packId(static_cast<uint32_t>(index));
  le->_setId(id);
  le->_parentId = 0;
  le->_sectionId = SectionEntry::kInvalidId;
  le->_offset = 0;

  _labels.appendUnsafe(le);
  idOut = id;
  return kErrorOk;
}

Error CodeHolder::newNamedLabelId(uint32_t& idOut, const char* name, size_t nameLength, uint32_t type, uint32_t parentId) noexcept {
  idOut = 0;
  uint32_t hVal = CodeHolder_hashNameAndFixLen(name, nameLength);

  if (ASMJIT_UNLIKELY(nameLength == 0))
    return DebugUtils::errored(kErrorInvalidLabelName);

  if (ASMJIT_UNLIKELY(nameLength > Globals::kMaxLabelLength))
    return DebugUtils::errored(kErrorLabelNameTooLong);

  switch (type) {
    case Label::kTypeLocal:
      if (ASMJIT_UNLIKELY(Operand::unpackId(parentId) >= _labels.getLength()))
        return DebugUtils::errored(kErrorInvalidParentLabel);

      hVal ^= parentId;
      break;

    case Label::kTypeGlobal:
      if (ASMJIT_UNLIKELY(parentId != 0))
        return DebugUtils::errored(kErrorNonLocalLabelCantHaveParent);

      break;

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  // Don't allow to insert duplicates. Local labels allow duplicates that have
  // different id, this is already accomplished by having a different hashes
  // between the same label names having different parent labels.
  LabelEntry* le = _namedLabels.get(LabelByName(name, nameLength, hVal));
  if (ASMJIT_UNLIKELY(le))
    return DebugUtils::errored(kErrorLabelAlreadyDefined);

  Error err = kErrorOk;
  size_t index = _labels.getLength();

  if (ASMJIT_UNLIKELY(index >= Operand::kPackedIdCount))
    return DebugUtils::errored(kErrorLabelIndexOverflow);

  ASMJIT_PROPAGATE(_labels.willGrow(&_baseHeap));
  le = _baseHeap.allocZeroedT<LabelEntry>();

  if (ASMJIT_UNLIKELY(!le))
    return  DebugUtils::errored(kErrorNoHeapMemory);

  uint32_t id = Operand::packId(static_cast<uint32_t>(index));
  le->_hVal = hVal;
  le->_setId(id);
  le->_type = static_cast<uint8_t>(type);
  le->_parentId = 0;
  le->_sectionId = SectionEntry::kInvalidId;
  le->_offset = 0;

  if (le->_name.mustEmbed(nameLength)) {
    le->_name.setEmbedded(name, nameLength);
  }
  else {
    char* nameExternal = static_cast<char*>(_dataZone.dup(name, nameLength, true));
    if (ASMJIT_UNLIKELY(!nameExternal))
      return DebugUtils::errored(kErrorNoHeapMemory);
    le->_name.setExternal(nameExternal, nameLength);
  }

  _labels.appendUnsafe(le);
  _namedLabels.put(le);

  idOut = id;
  return err;
}

uint32_t CodeHolder::getLabelIdByName(const char* name, size_t nameLength, uint32_t parentId) noexcept {
  uint32_t hVal = CodeHolder_hashNameAndFixLen(name, nameLength);
  if (ASMJIT_UNLIKELY(!nameLength)) return 0;

  LabelEntry* le = _namedLabels.get(LabelByName(name, nameLength, hVal));
  return le ? le->getId() : static_cast<uint32_t>(0);
}

// ============================================================================
// [asmjit::CodeEmitter - Relocations]
// ============================================================================

//! Encode MOD byte.
static ASMJIT_INLINE uint32_t x86EncodeMod(uint32_t m, uint32_t o, uint32_t rm) noexcept {
  return (m << 6) | (o << 3) | rm;
}

Error CodeHolder::newRelocEntry(RelocEntry** dst, uint32_t type, uint32_t size) noexcept {
  ASMJIT_PROPAGATE(_relocations.willGrow(&_baseHeap));

  size_t index = _relocations.getLength();
  if (ASMJIT_UNLIKELY(index > size_t(0xFFFFFFFFU)))
    return DebugUtils::errored(kErrorRelocIndexOverflow);

  RelocEntry* re = _baseHeap.allocZeroedT<RelocEntry>();
  if (ASMJIT_UNLIKELY(!re))
    return DebugUtils::errored(kErrorNoHeapMemory);

  re->_id = static_cast<uint32_t>(index);
  re->_type = static_cast<uint8_t>(type);
  re->_size = static_cast<uint8_t>(size);
  re->_sourceSectionId = SectionEntry::kInvalidId;
  re->_targetSectionId = SectionEntry::kInvalidId;
  _relocations.appendUnsafe(re);

  *dst = re;
  return kErrorOk;
}

// TODO: Support multiple sections, this only relocates the first.
// TODO: This should go to Runtime as it's responsible for relocating the
//       code, CodeHolder should just hold it.
size_t CodeHolder::relocate(void* _dst, uint64_t baseAddress) const noexcept {
  SectionEntry* section = _sections[0];
  ASMJIT_ASSERT(section != nullptr);

  uint8_t* dst = static_cast<uint8_t*>(_dst);
  if (baseAddress == Globals::kNoBaseAddress)
    baseAddress = static_cast<uint64_t>((uintptr_t)dst);

#if !defined(ASMJIT_DISABLE_LOGGING)
  Logger* logger = getLogger();
#endif // ASMJIT_DISABLE_LOGGING

  size_t minCodeSize = section->getBuffer().getLength(); // Minimum code size.
  size_t maxCodeSize = getCodeSize();                    // Includes all possible trampolines.

  // We will copy the exact size of the generated code. Extra code for trampolines
  // is generated on-the-fly by the relocator (this code doesn't exist at the moment).
  ::memcpy(dst, section->_buffer._data, minCodeSize);

  // Trampoline offset from the beginning of dst/baseAddress.
  size_t trampOffset = minCodeSize;

  // Relocate all recorded locations.
  size_t numRelocs = _relocations.getLength();
  const RelocEntry* const* reArray = _relocations.getData();

  for (size_t i = 0; i < numRelocs; i++) {
    const RelocEntry* re = reArray[i];

    // Possibly deleted or optimized out relocation entry.
    if (re->getType() == RelocEntry::kTypeNone)
      continue;

    uint64_t ptr = re->getData();
    size_t codeOffset = static_cast<size_t>(re->getSourceOffset());

    // Make sure that the `RelocEntry` is correct, we don't want to write
    // out of bounds in `dst`.
    if (ASMJIT_UNLIKELY(codeOffset + re->getSize() > maxCodeSize))
      return DebugUtils::errored(kErrorInvalidRelocEntry);

    // Whether to use trampoline, can be only used if relocation type is `kRelocTrampoline`.
    bool useTrampoline = false;

    switch (re->getType()) {
      case RelocEntry::kTypeAbsToAbs: {
        break;
      }

      case RelocEntry::kTypeRelToAbs: {
        ptr += baseAddress;
        break;
      }

      case RelocEntry::kTypeAbsToRel: {
        ptr -= baseAddress + re->getSourceOffset() + re->getSize();
        break;
      }

      case RelocEntry::kTypeTrampoline: {
        if (re->getSize() != 4)
          return DebugUtils::errored(kErrorInvalidRelocEntry);

        ptr -= baseAddress + re->getSourceOffset() + re->getSize();
        if (!Utils::isInt32(static_cast<int64_t>(ptr))) {
          ptr = (uint64_t)trampOffset - re->getSourceOffset() - re->getSize();
          useTrampoline = true;
        }
        break;
      }

      default:
        return DebugUtils::errored(kErrorInvalidRelocEntry);
    }

    switch (re->getSize()) {
      case 1:
        Utils::writeU8(dst + codeOffset, static_cast<uint32_t>(ptr & 0xFFU));
        break;

      case 4:
        Utils::writeU32u(dst + codeOffset, static_cast<uint32_t>(ptr & 0xFFFFFFFFU));
        break;

      case 8:
        Utils::writeU64u(dst + codeOffset, ptr);
        break;

      default:
        return DebugUtils::errored(kErrorInvalidRelocEntry);
    }

    // Handle the trampoline case.
    if (useTrampoline) {
      // Bytes that replace [REX, OPCODE] bytes.
      uint32_t byte0 = 0xFF;
      uint32_t byte1 = dst[codeOffset - 1];

      if (byte1 == 0xE8) {
        // Patch CALL/MOD byte to FF/2 (-> 0x15).
        byte1 = x86EncodeMod(0, 2, 5);
      }
      else if (byte1 == 0xE9) {
        // Patch JMP/MOD byte to FF/4 (-> 0x25).
        byte1 = x86EncodeMod(0, 4, 5);
      }
      else {
        return DebugUtils::errored(kErrorInvalidRelocEntry);
      }

      // Patch `jmp/call` instruction.
      ASMJIT_ASSERT(codeOffset >= 2);
      dst[codeOffset - 2] = static_cast<uint8_t>(byte0);
      dst[codeOffset - 1] = static_cast<uint8_t>(byte1);

      // Store absolute address and advance the trampoline pointer.
      Utils::writeU64u(dst + trampOffset, re->getData());
      trampOffset += 8;

#if !defined(ASMJIT_DISABLE_LOGGING)
      if (logger)
        logger->logf("[reloc] dq 0x%016llX ; Trampoline\n", re->getData());
#endif // !ASMJIT_DISABLE_LOGGING
    }
  }

  // If there are no trampolines this is the same as `minCodeSize`.
  return trampOffset;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
