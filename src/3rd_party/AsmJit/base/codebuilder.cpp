// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_BUILDER)

// [Dependencies]
#include "../base/codebuilder.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CodeBuilder - Construction / Destruction]
// ============================================================================

CodeBuilder::CodeBuilder() noexcept
  : CodeEmitter(kTypeBuilder),
    _cbBaseZone(32768 - Zone::kZoneOverhead),
    _cbDataZone(16384 - Zone::kZoneOverhead),
    _cbPassZone(32768 - Zone::kZoneOverhead),
    _cbHeap(&_cbBaseZone),
    _cbPasses(),
    _cbLabels(),
    _firstNode(nullptr),
    _lastNode(nullptr),
    _cursor(nullptr),
    _position(0),
    _nodeFlags(0) {}
CodeBuilder::~CodeBuilder() noexcept {}

// ============================================================================
// [asmjit::CodeBuilder - Events]
// ============================================================================

Error CodeBuilder::onAttach(CodeHolder* code) noexcept {
  return Base::onAttach(code);
}

Error CodeBuilder::onDetach(CodeHolder* code) noexcept {
  _cbPasses.reset();
  _cbLabels.reset();
  _cbHeap.reset(&_cbBaseZone);

  _cbBaseZone.reset(false);
  _cbDataZone.reset(false);
  _cbPassZone.reset(false);

  _position = 0;
  _nodeFlags = 0;

  _firstNode = nullptr;
  _lastNode = nullptr;
  _cursor = nullptr;

  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::CodeBuilder - Node-Factory]
// ============================================================================

Error CodeBuilder::getCBLabel(CBLabel** pOut, uint32_t id) noexcept {
  if (_lastError) return _lastError;
  ASMJIT_ASSERT(_code != nullptr);

  size_t index = Operand::unpackId(id);
  if (ASMJIT_UNLIKELY(index >= _code->getLabelsCount()))
    return DebugUtils::errored(kErrorInvalidLabel);

  if (index >= _cbLabels.getLength())
    ASMJIT_PROPAGATE(_cbLabels.resize(&_cbHeap, index + 1));

  CBLabel* node = _cbLabels[index];
  if (!node) {
    node = newNodeT<CBLabel>(id);
    if (ASMJIT_UNLIKELY(!node))
      return DebugUtils::errored(kErrorNoHeapMemory);
    _cbLabels[index] = node;
  }

  *pOut = node;
  return kErrorOk;
}

Error CodeBuilder::registerLabelNode(CBLabel* node) noexcept {
  if (_lastError) return _lastError;
  ASMJIT_ASSERT(_code != nullptr);

  // Don't call setLastError() from here, we are noexcept and we are called
  // by `newLabelNode()` and `newFuncNode()`, which are noexcept as well.
  uint32_t id;
  ASMJIT_PROPAGATE(_code->newLabelId(id));
  size_t index = Operand::unpackId(id);

  // We just added one label so it must be true.
  ASMJIT_ASSERT(_cbLabels.getLength() < index + 1);
  ASMJIT_PROPAGATE(_cbLabels.resize(&_cbHeap, index + 1));

  _cbLabels[index] = node;
  node->_id = id;
  return kErrorOk;
}

CBLabel* CodeBuilder::newLabelNode() noexcept {
  CBLabel* node = newNodeT<CBLabel>();
  if (!node || registerLabelNode(node) != kErrorOk)
    return nullptr;
  return node;
}

CBAlign* CodeBuilder::newAlignNode(uint32_t mode, uint32_t alignment) noexcept {
  return newNodeT<CBAlign>(mode, alignment);
}

CBData* CodeBuilder::newDataNode(const void* data, uint32_t size) noexcept {
  if (size > CBData::kInlineBufferSize) {
    void* cloned = _cbDataZone.alloc(size);
    if (!cloned) return nullptr;

    if (data) ::memcpy(cloned, data, size);
    data = cloned;
  }

  return newNodeT<CBData>(const_cast<void*>(data), size);
}

CBConstPool* CodeBuilder::newConstPool() noexcept {
  CBConstPool* node = newNodeT<CBConstPool>();
  if (!node || registerLabelNode(node) != kErrorOk)
    return nullptr;
  return node;
}

CBComment* CodeBuilder::newCommentNode(const char* s, size_t len) noexcept {
  if (s) {
    if (len == Globals::kInvalidIndex) len = ::strlen(s);
    if (len > 0) {
      s = static_cast<char*>(_cbDataZone.dup(s, len, true));
      if (!s) return nullptr;
    }
  }

  return newNodeT<CBComment>(s);
}

// ============================================================================
// [asmjit::CodeBuilder - Code-Emitter]
// ============================================================================

Label CodeBuilder::newLabel() {
  uint32_t id = kInvalidValue;

  if (!_lastError) {
    CBLabel* node = newNodeT<CBLabel>(id);
    if (ASMJIT_UNLIKELY(!node)) {
      setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    }
    else {
      Error err = registerLabelNode(node);
      if (ASMJIT_UNLIKELY(err))
        setLastError(err);
      else
        id = node->getId();
    }
  }

  return Label(id);
}

Label CodeBuilder::newNamedLabel(const char* name, size_t nameLength, uint32_t type, uint32_t parentId) {
  uint32_t id = kInvalidValue;

  if (!_lastError) {
    CBLabel* node = newNodeT<CBLabel>(id);
    if (ASMJIT_UNLIKELY(!node)) {
      setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    }
    else {
      Error err = _code->newNamedLabelId(id, name, nameLength, type, parentId);
      if (ASMJIT_UNLIKELY(err))
        setLastError(err);
      else
        id = node->getId();
    }
  }

  return Label(id);
}

Error CodeBuilder::bind(const Label& label) {
  if (_lastError) return _lastError;

  CBLabel* node;
  Error err = getCBLabel(&node, label);
  if (ASMJIT_UNLIKELY(err))
    return setLastError(err);

  addNode(node);
  return kErrorOk;
}

Error CodeBuilder::align(uint32_t mode, uint32_t alignment) {
  if (_lastError) return _lastError;

  CBAlign* node = newAlignNode(mode, alignment);
  if (ASMJIT_UNLIKELY(!node))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

Error CodeBuilder::embed(const void* data, uint32_t size) {
  if (_lastError) return _lastError;

  CBData* node = newDataNode(data, size);
  if (ASMJIT_UNLIKELY(!node))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

Error CodeBuilder::embedLabel(const Label& label) {
  if (_lastError) return _lastError;

  CBLabelData* node = newNodeT<CBLabelData>(label.getId());
  if (ASMJIT_UNLIKELY(!node))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

Error CodeBuilder::embedConstPool(const Label& label, const ConstPool& pool) {
  if (_lastError) return _lastError;

  if (!isLabelValid(label))
    return setLastError(DebugUtils::errored(kErrorInvalidLabel));

  ASMJIT_PROPAGATE(align(kAlignData, static_cast<uint32_t>(pool.getAlignment())));
  ASMJIT_PROPAGATE(bind(label));

  CBData* node = newDataNode(nullptr, static_cast<uint32_t>(pool.getSize()));
  if (ASMJIT_UNLIKELY(!node))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  pool.fill(node->getData());
  addNode(node);
  return kErrorOk;
}

Error CodeBuilder::comment(const char* s, size_t len) {
  if (_lastError) return _lastError;

  CBComment* node = newCommentNode(s, len);
  if (ASMJIT_UNLIKELY(!node))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeBuilder - Node-Management]
// ============================================================================

CBNode* CodeBuilder::addNode(CBNode* node) noexcept {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(node->_prev == nullptr);
  ASMJIT_ASSERT(node->_next == nullptr);

  if (!_cursor) {
    if (!_firstNode) {
      _firstNode = node;
      _lastNode = node;
    }
    else {
      node->_next = _firstNode;
      _firstNode->_prev = node;
      _firstNode = node;
    }
  }
  else {
    CBNode* prev = _cursor;
    CBNode* next = _cursor->_next;

    node->_prev = prev;
    node->_next = next;

    prev->_next = node;
    if (next)
      next->_prev = node;
    else
      _lastNode = node;
  }

  _cursor = node;
  return node;
}

CBNode* CodeBuilder::addAfter(CBNode* node, CBNode* ref) noexcept {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(ref);

  ASMJIT_ASSERT(node->_prev == nullptr);
  ASMJIT_ASSERT(node->_next == nullptr);

  CBNode* prev = ref;
  CBNode* next = ref->_next;

  node->_prev = prev;
  node->_next = next;

  prev->_next = node;
  if (next)
    next->_prev = node;
  else
    _lastNode = node;

  return node;
}

CBNode* CodeBuilder::addBefore(CBNode* node, CBNode* ref) noexcept {
  ASMJIT_ASSERT(node != nullptr);
  ASMJIT_ASSERT(node->_prev == nullptr);
  ASMJIT_ASSERT(node->_next == nullptr);
  ASMJIT_ASSERT(ref != nullptr);

  CBNode* prev = ref->_prev;
  CBNode* next = ref;

  node->_prev = prev;
  node->_next = next;

  next->_prev = node;
  if (prev)
    prev->_next = node;
  else
    _firstNode = node;

  return node;
}

static ASMJIT_INLINE void CodeBuilder_nodeRemoved(CodeBuilder* self, CBNode* node_) noexcept {
  if (node_->isJmpOrJcc()) {
    CBJump* node = static_cast<CBJump*>(node_);
    CBLabel* label = node->getTarget();

    if (label) {
      // Disconnect.
      CBJump** pPrev = &label->_from;
      for (;;) {
        ASMJIT_ASSERT(*pPrev != nullptr);

        CBJump* current = *pPrev;
        if (!current) break;

        if (current == node) {
          *pPrev = node->_jumpNext;
          break;
        }

        pPrev = &current->_jumpNext;
      }

      label->subNumRefs();
    }
  }
}

CBNode* CodeBuilder::removeNode(CBNode* node) noexcept {
  CBNode* prev = node->_prev;
  CBNode* next = node->_next;

  if (_firstNode == node)
    _firstNode = next;
  else
    prev->_next = next;

  if (_lastNode == node)
    _lastNode  = prev;
  else
    next->_prev = prev;

  node->_prev = nullptr;
  node->_next = nullptr;

  if (_cursor == node)
    _cursor = prev;
  CodeBuilder_nodeRemoved(this, node);

  return node;
}

void CodeBuilder::removeNodes(CBNode* first, CBNode* last) noexcept {
  if (first == last) {
    removeNode(first);
    return;
  }

  CBNode* prev = first->_prev;
  CBNode* next = last->_next;

  if (_firstNode == first)
    _firstNode = next;
  else
    prev->_next = next;

  if (_lastNode == last)
    _lastNode  = prev;
  else
    next->_prev = prev;

  CBNode* node = first;
  for (;;) {
    CBNode* next = node->getNext();
    ASMJIT_ASSERT(next != nullptr);

    node->_prev = nullptr;
    node->_next = nullptr;

    if (_cursor == node)
      _cursor = prev;
    CodeBuilder_nodeRemoved(this, node);

    if (node == last)
      break;
    node = next;
  }
}

CBNode* CodeBuilder::setCursor(CBNode* node) noexcept {
  CBNode* old = _cursor;
  _cursor = node;
  return old;
}

// ============================================================================
// [asmjit::CodeBuilder - Passes]
// ============================================================================

ASMJIT_FAVOR_SIZE CBPass* CodeBuilder::getPassByName(const char* name) const noexcept {
  for (size_t i = 0, len = _cbPasses.getLength(); i < len; i++) {
    CBPass* pass = _cbPasses[i];
    if (::strcmp(pass->getName(), name) == 0)
      return pass;
  }

  return nullptr;
}

ASMJIT_FAVOR_SIZE Error CodeBuilder::addPass(CBPass* pass) noexcept {
  if (ASMJIT_UNLIKELY(pass == nullptr)) {
    // Since this is directly called by `addPassT()` we treat `null` argument
    // as out-of-memory condition. Otherwise it would be API misuse.
    return DebugUtils::errored(kErrorNoHeapMemory);
  }
  else if (ASMJIT_UNLIKELY(pass->_cb)) {
    // Kind of weird, but okay...
    if (pass->_cb == this)
      return kErrorOk;
    return DebugUtils::errored(kErrorInvalidState);
  }

  ASMJIT_PROPAGATE(_cbPasses.append(&_cbHeap, pass));
  pass->_cb = this;
  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error CodeBuilder::deletePass(CBPass* pass) noexcept {
  if (ASMJIT_UNLIKELY(pass == nullptr))
    return DebugUtils::errored(kErrorInvalidArgument);

  if (pass->_cb != nullptr) {
    if (pass->_cb != this)
      return DebugUtils::errored(kErrorInvalidState);

    size_t index = _cbPasses.indexOf(pass);
    ASMJIT_ASSERT(index != Globals::kInvalidIndex);

    pass->_cb = nullptr;
    _cbPasses.removeAt(index);
  }

  pass->~CBPass();
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeBuilder - Serialization]
// ============================================================================

Error CodeBuilder::serialize(CodeEmitter* dst) {
  Error err = kErrorOk;
  CBNode* node_ = getFirstNode();

  do {
    dst->setInlineComment(node_->getInlineComment());

    switch (node_->getType()) {
      case CBNode::kNodeAlign: {
        CBAlign* node = static_cast<CBAlign*>(node_);
        err = dst->align(node->getMode(), node->getAlignment());
        break;
      }

      case CBNode::kNodeData: {
        CBData* node = static_cast<CBData*>(node_);
        err = dst->embed(node->getData(), node->getSize());
        break;
      }

      case CBNode::kNodeFunc:
      case CBNode::kNodeLabel: {
        CBLabel* node = static_cast<CBLabel*>(node_);
        err = dst->bind(node->getLabel());
        break;
      }

      case CBNode::kNodeLabelData: {
        CBLabelData* node = static_cast<CBLabelData*>(node_);
        err = dst->embedLabel(node->getLabel());
        break;
      }

      case CBNode::kNodeConstPool: {
        CBConstPool* node = static_cast<CBConstPool*>(node_);
        err = dst->embedConstPool(node->getLabel(), node->getConstPool());
        break;
      }

      case CBNode::kNodeInst:
      case CBNode::kNodeFuncCall: {
        CBInst* node = node_->as<CBInst>();
        dst->setOptions(node->getOptions());
        dst->setExtraReg(node->getExtraReg());
        err = dst->emitOpArray(node->getInstId(), node->getOpArray(), node->getOpCount());
        break;
      }

      case CBNode::kNodeComment: {
        CBComment* node = static_cast<CBComment*>(node_);
        err = dst->comment(node->getInlineComment());
        break;
      }

      default:
        break;
    }

    if (err) break;
    node_ = node_->getNext();
  } while (node_);

  return err;
}

// ============================================================================
// [asmjit::CBPass]
// ============================================================================

CBPass::CBPass(const char* name) noexcept
  : _cb(nullptr),
    _name(name) {}
CBPass::~CBPass() noexcept {}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_BUILDER
