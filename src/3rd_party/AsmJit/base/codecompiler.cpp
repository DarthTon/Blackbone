// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codecompiler.h"
#include "../base/cpuinfo.h"
#include "../base/logging.h"
#include "../base/regalloc_p.h"
#include "../base/utils.h"
#include <stdarg.h>

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Constants]
// ============================================================================

static const char noName[1] = { '\0' };

// ============================================================================
// [asmjit::CCFuncCall - Arg / Ret]
// ============================================================================

bool CCFuncCall::_setArg(uint32_t i, const Operand_& op) noexcept {
  if ((i & ~kFuncArgHi) >= _funcDetail.getArgCount())
    return false;

  _args[i] = op;
  return true;
}

bool CCFuncCall::_setRet(uint32_t i, const Operand_& op) noexcept {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::CodeCompiler - Construction / Destruction]
// ============================================================================

CodeCompiler::CodeCompiler() noexcept
  : CodeBuilder(),
    _func(nullptr),
    _vRegZone(4096 - Zone::kZoneOverhead),
    _vRegArray(),
    _localConstPool(nullptr),
    _globalConstPool(nullptr) {

  _type = kTypeCompiler;
}
CodeCompiler::~CodeCompiler() noexcept {}

// ============================================================================
// [asmjit::CodeCompiler - Events]
// ============================================================================

Error CodeCompiler::onAttach(CodeHolder* code) noexcept {
  return Base::onAttach(code);
}

Error CodeCompiler::onDetach(CodeHolder* code) noexcept {
  _func = nullptr;

  _localConstPool = nullptr;
  _globalConstPool = nullptr;

  _vRegArray.reset();
  _vRegZone.reset(false);

  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::CodeCompiler - Node-Factory]
// ============================================================================

CCHint* CodeCompiler::newHintNode(Reg& r, uint32_t hint, uint32_t value) noexcept {
  if (!r.isVirtReg()) return nullptr;

  VirtReg* vr = getVirtReg(r);
  return newNodeT<CCHint>(vr, hint, value);
}

// ============================================================================
// [asmjit::CodeCompiler - Func]
// ============================================================================

CCFunc* CodeCompiler::newFunc(const FuncSignature& sign) noexcept {
  Error err;

  CCFunc* func = newNodeT<CCFunc>();
  if (!func) goto _NoMemory;

  err = registerLabelNode(func);
  if (ASMJIT_UNLIKELY(err)) {
    // TODO: Calls setLastError, maybe rethink noexcept?
    setLastError(err);
    return nullptr;
  }

  // Create helper nodes.
  func->_exitNode = newLabelNode();
  func->_end = newNodeT<CBSentinel>();

  if (!func->_exitNode || !func->_end)
    goto _NoMemory;

  // Function prototype.
  err = func->getDetail().init(sign);
  if (err != kErrorOk) {
    setLastError(err);
    return nullptr;
  }

  // If the CodeInfo guarantees higher alignment honor it.
  if (_codeInfo.getStackAlignment() > func->_funcDetail._callConv.getNaturalStackAlignment())
    func->_funcDetail._callConv.setNaturalStackAlignment(_codeInfo.getStackAlignment());

  // Allocate space for function arguments.
  func->_args = nullptr;
  if (func->getArgCount() != 0) {
    func->_args = _cbHeap.allocT<VirtReg*>(func->getArgCount() * sizeof(VirtReg*));
    if (!func->_args) goto _NoMemory;

    ::memset(func->_args, 0, func->getArgCount() * sizeof(VirtReg*));
  }

  return func;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

CCFunc* CodeCompiler::addFunc(CCFunc* func) {
  ASMJIT_ASSERT(_func == nullptr);
  _func = func;

  addNode(func);                 // Function node.
  CBNode* cursor = getCursor();  // {CURSOR}.
  addNode(func->getExitNode());  // Function exit label.
  addNode(func->getEnd());       // Function end marker.

  _setCursor(cursor);
  return func;
}

CCFunc* CodeCompiler::addFunc(const FuncSignature& sign) {
  CCFunc* func = newFunc(sign);

  if (!func) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  return addFunc(func);
}

CBSentinel* CodeCompiler::endFunc() {
  CCFunc* func = getFunc();
  if (!func) {
    // TODO:
    return nullptr;
  }

  // Add the local constant pool at the end of the function (if exists).
  if (_localConstPool) {
    setCursor(func->getEnd()->getPrev());
    addNode(_localConstPool);
    _localConstPool = nullptr;
  }

  // Mark as finished.
  func->_isFinished = true;
  _func = nullptr;

  CBSentinel* end = func->getEnd();
  setCursor(end);
  return end;
}

// ============================================================================
// [asmjit::CodeCompiler - Ret]
// ============================================================================

CCFuncRet* CodeCompiler::newRet(const Operand_& o0, const Operand_& o1) noexcept {
  CCFuncRet* node = newNodeT<CCFuncRet>(o0, o1);
  if (!node) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }
  return node;
}

CCFuncRet* CodeCompiler::addRet(const Operand_& o0, const Operand_& o1) noexcept {
  CCFuncRet* node = newRet(o0, o1);
  if (!node) return nullptr;
  return static_cast<CCFuncRet*>(addNode(node));
}

// ============================================================================
// [asmjit::CodeCompiler - Call]
// ============================================================================

CCFuncCall* CodeCompiler::newCall(uint32_t instId, const Operand_& o0, const FuncSignature& sign) noexcept {
  Error err;
  uint32_t nArgs;

  CCFuncCall* node = _cbHeap.allocT<CCFuncCall>(sizeof(CCFuncCall) + sizeof(Operand));
  Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CCFuncCall));

  if (ASMJIT_UNLIKELY(!node))
    goto _NoMemory;

  opArray[0].copyFrom(o0);
  new (node) CCFuncCall(this, instId, 0, opArray, 1);

  if ((err = node->getDetail().init(sign)) != kErrorOk) {
    setLastError(err);
    return nullptr;
  }

  // If there are no arguments skip the allocation.
  if ((nArgs = sign.getArgCount()) == 0)
    return node;

  node->_args = static_cast<Operand*>(_cbHeap.alloc(nArgs * sizeof(Operand)));
  if (!node->_args) goto _NoMemory;

  ::memset(node->_args, 0, nArgs * sizeof(Operand));
  return node;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

CCFuncCall* CodeCompiler::addCall(uint32_t instId, const Operand_& o0, const FuncSignature& sign) noexcept {
  CCFuncCall* node = newCall(instId, o0, sign);
  if (!node) return nullptr;
  return static_cast<CCFuncCall*>(addNode(node));
}

// ============================================================================
// [asmjit::CodeCompiler - Vars]
// ============================================================================

Error CodeCompiler::setArg(uint32_t argIndex, const Reg& r) {
  CCFunc* func = getFunc();

  if (!func)
    return setLastError(DebugUtils::errored(kErrorInvalidState));

  if (!isVirtRegValid(r))
    return setLastError(DebugUtils::errored(kErrorInvalidVirtId));

  VirtReg* vr = getVirtReg(r);
  func->setArg(argIndex, vr);

  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeCompiler - Hint]
// ============================================================================

Error CodeCompiler::_hint(Reg& r, uint32_t hint, uint32_t value) {
  if (!r.isVirtReg()) return kErrorOk;

  CCHint* node = newHintNode(r, hint, value);
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeCompiler - Vars]
// ============================================================================

VirtReg* CodeCompiler::newVirtReg(uint32_t typeId, uint32_t signature, const char* name) noexcept {
  size_t index = _vRegArray.getLength();
  if (ASMJIT_UNLIKELY(index > Operand::kPackedIdCount))
    return nullptr;

  VirtReg* vreg;
  if (_vRegArray.willGrow(&_cbHeap, 1) != kErrorOk || !(vreg = _vRegZone.allocZeroedT<VirtReg>()))
    return nullptr;

  vreg->_id = Operand::packId(static_cast<uint32_t>(index));
  vreg->_regInfo._signature = signature;
  vreg->_name = noName;

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (name && name[0] != '\0')
    vreg->_name = static_cast<char*>(_cbDataZone.dup(name, ::strlen(name), true));
#endif // !ASMJIT_DISABLE_LOGGING

  vreg->_size = TypeId::sizeOf(typeId);
  vreg->_typeId = typeId;
  vreg->_alignment = static_cast<uint8_t>(std::min<uint32_t>(vreg->_size, 64));
  vreg->_priority = 10;

  // The following are only used by `RAPass`.
  vreg->_raId = kInvalidValue;
  vreg->_state = VirtReg::kStateNone;
  vreg->_physId = Globals::kInvalidRegId;

  _vRegArray.appendUnsafe(vreg);
  return vreg;
}

Error CodeCompiler::_newReg(Reg& out, uint32_t typeId, const char* name) {
  RegInfo regInfo;

  Error err = ArchUtils::typeIdToRegInfo(getArchType(), typeId, regInfo);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  VirtReg* vReg = newVirtReg(typeId, regInfo.getSignature(), name);
  if (ASMJIT_UNLIKELY(!vReg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  out._initReg(regInfo.getSignature(), vReg->getId());
  return kErrorOk;
}

Error CodeCompiler::_newReg(Reg& out, uint32_t typeId, const char* nameFmt, va_list ap) {
  StringBuilderTmp<256> sb;
  sb.appendFormatVA(nameFmt, ap);
  return _newReg(out, typeId, sb.getData());
}

Error CodeCompiler::_newReg(Reg& out, const Reg& ref, const char* name) {
  RegInfo regInfo;
  uint32_t typeId;

  if (isVirtRegValid(ref)) {
    VirtReg* vRef = getVirtReg(ref);
    typeId = vRef->getTypeId();

    // NOTE: It's possible to cast one register type to another if it's the
    // same register kind. However, VirtReg always contains the TypeId that
    // was used to create the register. This means that in some cases we may
    // end up having different size of `ref` and `vRef`. In such case we
    // adjust the TypeId to match the `ref` register type instead of the
    // original register type, which should be the expected behavior.
    uint32_t typeSize = TypeId::sizeOf(typeId);
    uint32_t refSize = ref.getSize();

    if (typeSize != refSize) {
      if (TypeId::isInt(typeId)) {
        // GP register - change TypeId to match `ref`, but keep sign of `vRef`.
        switch (refSize) {
          case  1: typeId = TypeId::kI8  | (typeId & 1); break;
          case  2: typeId = TypeId::kI16 | (typeId & 1); break;
          case  4: typeId = TypeId::kI32 | (typeId & 1); break;
          case  8: typeId = TypeId::kI64 | (typeId & 1); break;
          default: typeId = TypeId::kVoid; break;
        }
      }
      else if (TypeId::isMmx(typeId)) {
        // MMX register - always use 64-bit.
        typeId = TypeId::kMmx64;
      }
      else if (TypeId::isMask(typeId)) {
        // Mask register - change TypeId to match `ref` size.
        switch (refSize) {
          case  1: typeId = TypeId::kMask8; break;
          case  2: typeId = TypeId::kMask16; break;
          case  4: typeId = TypeId::kMask32; break;
          case  8: typeId = TypeId::kMask64; break;
          default: typeId = TypeId::kVoid; break;
        }
      }
      else {
        // VEC register - change TypeId to match `ref` size, keep vector metadata.
        uint32_t elementTypeId = TypeId::elementOf(typeId);

        switch (refSize) {
          case 16: typeId = TypeId::_kVec128Start + (elementTypeId - TypeId::kI8); break;
          case 32: typeId = TypeId::_kVec256Start + (elementTypeId - TypeId::kI8); break;
          case 64: typeId = TypeId::_kVec512Start + (elementTypeId - TypeId::kI8); break;
          default: typeId = TypeId::kVoid; break;
        }
      }

      if (typeId == TypeId::kVoid)
        return setLastError(DebugUtils::errored(kErrorInvalidState));
    }
  }
  else {
    typeId = ref.getType();
  }

  Error err = ArchUtils::typeIdToRegInfo(getArchType(), typeId, regInfo);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  VirtReg* vReg = newVirtReg(typeId, regInfo.getSignature(), name);
  if (ASMJIT_UNLIKELY(!vReg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  out._initReg(regInfo.getSignature(), vReg->getId());
  return kErrorOk;
}

Error CodeCompiler::_newReg(Reg& out, const Reg& ref, const char* nameFmt, va_list ap) {
  StringBuilderTmp<256> sb;
  sb.appendFormatVA(nameFmt, ap);
  return _newReg(out, ref, sb.getData());
}

Error CodeCompiler::_newStack(Mem& out, uint32_t size, uint32_t alignment, const char* name) {
  if (size == 0)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment == 0) alignment = 1;
  if (!Utils::isPowerOf2(alignment))
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment > 64) alignment = 64;

  VirtReg* vReg = newVirtReg(0, 0, name);
  if (ASMJIT_UNLIKELY(!vReg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  vReg->_size = size;
  vReg->_isStack = true;
  vReg->_alignment = static_cast<uint8_t>(alignment);

  // Set the memory operand to GPD/GPQ and its id to VirtReg.
  out = Mem(Init, _nativeGpReg.getType(), vReg->getId(), Reg::kRegNone, kInvalidValue, 0, 0, Mem::kSignatureMemRegHomeFlag);
  return kErrorOk;
}

Error CodeCompiler::_newConst(Mem& out, uint32_t scope, const void* data, size_t size) {
  CBConstPool** pPool;
  if (scope == kConstScopeLocal)
    pPool = &_localConstPool;
  else if (scope == kConstScopeGlobal)
    pPool = &_globalConstPool;
  else
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (!*pPool && !(*pPool = newConstPool()))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  CBConstPool* pool = *pPool;
  size_t off;

  Error err = pool->add(data, size, off);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  out = Mem(Init,
    Label::kLabelTag,             // Base type.
    pool->getId(),                // Base id.
    0,                            // Index type.
    kInvalidValue,                // Index id.
    static_cast<int32_t>(off),    // Offset.
    static_cast<uint32_t>(size),  // Size.
    0);                           // Flags.
  return kErrorOk;
}

Error CodeCompiler::alloc(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintAlloc, kInvalidValue);
}

Error CodeCompiler::alloc(Reg& reg, uint32_t physId) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintAlloc, physId);
}

Error CodeCompiler::alloc(Reg& reg, const Reg& physReg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintAlloc, physReg.getId());
}

Error CodeCompiler::save(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintSave, kInvalidValue);
}

Error CodeCompiler::spill(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintSpill, kInvalidValue);
}

Error CodeCompiler::unuse(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintUnuse, kInvalidValue);
}

uint32_t CodeCompiler::getPriority(Reg& reg) const {
  if (!reg.isVirtReg()) return 0;
  return getVirtRegById(reg.getId())->getPriority();
}

void CodeCompiler::setPriority(Reg& reg, uint32_t priority) {
  if (!reg.isVirtReg()) return;
  if (priority > 255) priority = 255;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (vreg) vreg->_priority = static_cast<uint8_t>(priority);
}

bool CodeCompiler::getSaveOnUnuse(Reg& reg) const {
  if (!reg.isVirtReg()) return false;

  VirtReg* vreg = getVirtRegById(reg.getId());
  return static_cast<bool>(vreg->_saveOnUnuse);
}

void CodeCompiler::setSaveOnUnuse(Reg& reg, bool value) {
  if (!reg.isVirtReg()) return;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (!vreg) return;

  vreg->_saveOnUnuse = value;
}

void CodeCompiler::rename(Reg& reg, const char* fmt, ...) {
  if (!reg.isVirtReg()) return;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (!vreg) return;

  vreg->_name = noName;
  if (fmt && fmt[0] != '\0') {
    char buf[64];

    va_list ap;
    va_start(ap, fmt);

    vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf), fmt, ap);
    buf[ASMJIT_ARRAY_SIZE(buf) - 1] = '\0';

    vreg->_name = static_cast<char*>(_cbDataZone.dup(buf, ::strlen(buf), true));
    va_end(ap);
  }
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
