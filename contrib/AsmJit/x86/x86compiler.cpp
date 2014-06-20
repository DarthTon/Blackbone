// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER) && (defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64))

// [Dependencies - AsmJit]
#include "../base/intutil.h"
#include "../base/string.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86context_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [Debug]
// ============================================================================

#if !defined(ASMJIT_DEBUG)
#define ASMJIT_DETECT_UNINITIALIZED(op) \
  do {} while(0)
#else
#define ASMJIT_DETECT_UNINITIALIZED(op) \
  do { \
    if (op.isVar() || op.isLabel()) { \
      ASMJIT_ASSERT(op.getId() != kInvalidValue); \
    } \
  } while(0)
#endif

// ============================================================================
// [asmjit::x86x64::X86X64CallNode - Prototype]
// ============================================================================

Error X86X64CallNode::setPrototype(uint32_t conv, const FuncPrototype& p) {
  return _x86Decl.setPrototype(conv, p);
}

// ============================================================================
// [asmjit::x86x64::X86X64CallNode - Arg / Ret]
// ============================================================================

bool X86X64CallNode::_setArg(uint32_t i, const Operand& op) {
  if ((i & ~kFuncArgHi) >= _x86Decl.getArgCount())
    return false;

  _args[i] = op;
  return true;
}

bool X86X64CallNode::_setRet(uint32_t i, const Operand& op) {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Helpers (Private)]
// ============================================================================

static Error X86X64Compiler_emitConstPool(X86X64Compiler* self,
  Label& label, ConstPool& pool) {

  if (label.getId() == kInvalidValue)
    return kErrorOk;

  self->align(kAlignData, static_cast<uint32_t>(pool.getAlignment()));
  self->bind(label);

  EmbedNode* embedNode = self->embed(NULL, static_cast<uint32_t>(pool.getSize()));
  if (embedNode == NULL)
    return kErrorNoHeapMemory;

  pool.fill(embedNode->getData());
  pool.reset();
  label.reset();

  return kErrorOk;
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Construction / Destruction]
// ============================================================================

X86X64Compiler::X86X64Compiler(Runtime* runtime) : BaseCompiler(runtime) {}
X86X64Compiler::~X86X64Compiler() {}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Inst]
// ============================================================================

//! Get compiler instruction item size without operands assigned.
static ASMJIT_INLINE size_t X86X64Compiler_getInstSize(uint32_t code) {
  return (IntUtil::inInterval<uint32_t>(code, _kInstJbegin, _kInstJend)) ? sizeof(JumpNode) : sizeof(InstNode);
}

static InstNode* X86X64Compiler_newInst(X86X64Compiler* self, void* p, uint32_t code, uint32_t options, Operand* opList, uint32_t opCount) {
  if (IntUtil::inInterval<uint32_t>(code, _kInstJbegin, _kInstJend)) {
    JumpNode* node = new(p) JumpNode(self, code, options, opList, opCount);
    TargetNode* jTarget = self->getTargetById(opList[0].getId());

    node->addFlags(code == kInstJmp ? kNodeFlagIsJmp | kNodeFlagIsTaken : kNodeFlagIsJcc);
    node->_target = jTarget;
    node->_jumpNext = static_cast<JumpNode*>(jTarget->_from);

    jTarget->_from = node;
    jTarget->addNumRefs();

    // The 'jmp' is always taken, conditional jump can contain hint, we detect it.
    if (code == kInstJmp)
      node->addFlags(kNodeFlagIsTaken);
    else if (options & kInstOptionTaken)
      node->addFlags(kNodeFlagIsTaken);

    node->addOptions(options);
    return node;
  }
  else {
    InstNode* node = new(p) InstNode(self, code, options, opList, opCount);
    node->addOptions(options);
    return node;
  }
}

InstNode* X86X64Compiler::newInst(uint32_t code) {
  size_t size = X86X64Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size));

  if (inst == NULL)
    goto _NoMemory;

  return X86X64Compiler_newInst(this, inst, code, getOptionsAndClear(), NULL, 0);

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86X64Compiler::newInst(uint32_t code, const Operand& o0) {
  size_t size = X86X64Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 1 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    ASMJIT_DETECT_UNINITIALIZED(o0);
    return X86X64Compiler_newInst(this, inst, code, getOptionsAndClear(), opList, 1);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86X64Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1) {
  size_t size = X86X64Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 2 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    ASMJIT_DETECT_UNINITIALIZED(o0);
    ASMJIT_DETECT_UNINITIALIZED(o1);
    return X86X64Compiler_newInst(this, inst, code, getOptionsAndClear(), opList, 2);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86X64Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) {
  size_t size = X86X64Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 3 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    opList[2] = o2;
    ASMJIT_DETECT_UNINITIALIZED(o0);
    ASMJIT_DETECT_UNINITIALIZED(o1);
    ASMJIT_DETECT_UNINITIALIZED(o2);
    return X86X64Compiler_newInst(this, inst, code, getOptionsAndClear(), opList, 3);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86X64Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3) {
  size_t size = X86X64Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 4 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    opList[2] = o2;
    opList[3] = o3;
    ASMJIT_DETECT_UNINITIALIZED(o0);
    ASMJIT_DETECT_UNINITIALIZED(o1);
    ASMJIT_DETECT_UNINITIALIZED(o2);
    ASMJIT_DETECT_UNINITIALIZED(o3);
    return X86X64Compiler_newInst(this, inst, code, getOptionsAndClear(), opList, 4);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86X64Compiler::newInst(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3, const Operand& o4) {
  size_t size = X86X64Compiler_getInstSize(code);
  InstNode* inst = static_cast<InstNode*>(_baseZone.alloc(size + 5 * sizeof(Operand)));

  if (inst == NULL)
    goto _NoMemory;

  {
    Operand* opList = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(inst) + size);
    opList[0] = o0;
    opList[1] = o1;
    opList[2] = o2;
    opList[3] = o3;
    opList[4] = o4;
    ASMJIT_DETECT_UNINITIALIZED(o0);
    ASMJIT_DETECT_UNINITIALIZED(o1);
    ASMJIT_DETECT_UNINITIALIZED(o2);
    ASMJIT_DETECT_UNINITIALIZED(o3);
    ASMJIT_DETECT_UNINITIALIZED(o4);
    return X86X64Compiler_newInst(this, inst, code, getOptionsAndClear(), opList, 5);
  }

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

InstNode* X86X64Compiler::emit(uint32_t code) {
  InstNode* node = newInst(code);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0) {
  InstNode* node = newInst(code, o0);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1){
  InstNode* node = newInst(code, o0, o1);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2) {
  InstNode* node = newInst(code, o0, o1, o2);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3){
  InstNode* node = newInst(code, o0, o1, o2, o3);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, const Operand& o2, const Operand& o3, const Operand& o4) {
  InstNode* node = newInst(code, o0, o1, o2, o3, o4);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, int o0_) {
  Imm o0(o0_);
  InstNode* node = newInst(code, o0);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, uint64_t o0_) {
  Imm o0(o0_);
  InstNode* node = newInst(code, o0);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, int o1_) {
  Imm o1(o1_);
  InstNode* node = newInst(code, o0, o1);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, uint64_t o1_) {
  Imm o1(o1_);
  InstNode* node = newInst(code, o0, o1);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, int o2_) {
  Imm o2(o2_);
  InstNode* node = newInst(code, o0, o1, o2);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

InstNode* X86X64Compiler::emit(uint32_t code, const Operand& o0, const Operand& o1, uint64_t o2_) {
  Imm o2(o2_);
  InstNode* node = newInst(code, o0, o1, o2);
  if (node == NULL)
    return NULL;
  return static_cast<InstNode*>(addNode(node));
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Func]
// ============================================================================

X86X64FuncNode* X86X64Compiler::newFunc(uint32_t conv, const FuncPrototype& p) {
  X86X64FuncNode* func = newNode<X86X64FuncNode>();
  Error error;

  if (func == NULL)
    goto _NoMemory;

  // Create helper nodes.
  func->_entryNode = newTarget();
  func->_exitNode = newTarget();
  func->_end = newNode<EndNode>();

  if (func->_entryNode == NULL || func->_exitNode == NULL || func->_end == NULL)
    goto _NoMemory;

  // Emit push/pop sequence by default.
  func->_funcHints |= IntUtil::mask(kFuncHintPushPop);

  // Function prototype.
  if ((error = func->_x86Decl.setPrototype(conv, p)) != kErrorOk) {
    setError(error);
    return NULL;
  }

  // Function arguments stack size. Since function requires _argStackSize to be
  // set, we have to copy it from X86X64FuncDecl.
  func->_argStackSize = func->_x86Decl.getArgStackSize();
  func->_redZoneSize = static_cast<uint16_t>(func->_x86Decl.getRedZoneSize());
  func->_spillZoneSize = static_cast<uint16_t>(func->_x86Decl.getSpillZoneSize());

  // Expected/Required stack alignment.
  func->_expectedStackAlignment = getRuntime()->getStackAlignment();
  func->_requiredStackAlignment = 0;

  // Allocate space for function arguments.
  func->_argList = NULL;
  if (func->getArgCount() != 0) {
    func->_argList = _baseZone.allocT<VarData*>(func->getArgCount() * sizeof(VarData*));
    if (func->_argList == NULL)
      goto _NoMemory;
    ::memset(func->_argList, 0, func->getArgCount() * sizeof(VarData*));
  }

  return func;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

X86X64FuncNode* X86X64Compiler::addFunc(uint32_t conv, const FuncPrototype& p) {
  X86X64FuncNode* func = newFunc(conv, p);

  if (func == NULL) {
    setError(kErrorNoHeapMemory);
    return NULL;
  }

  ASMJIT_ASSERT(_func == NULL);
  _func = func;

  addNode(func);
  addNode(func->getEntryNode());

  return func;
}

EndNode* X86X64Compiler::endFunc() {
  X86X64FuncNode* func = getFunc();
  ASMJIT_ASSERT(func != NULL);

  // App function exit / epilog marker.
  addNode(func->getExitNode());

  // Add local constant pool at the end of the function (if exist).
  X86X64Compiler_emitConstPool(this, _localConstPoolLabel, _localConstPool);

  // Add function end marker.
  addNode(func->getEnd());

  // Finalize...
  func->addFuncFlags(kFuncFlagIsFinished);
  _func = NULL;

  return func->getEnd();
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Ret]
// ============================================================================

RetNode* X86X64Compiler::newRet(const Operand& o0, const Operand& o1) {
  RetNode* node = newNode<RetNode>(o0, o1);
  if (node == NULL)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

RetNode* X86X64Compiler::addRet(const Operand& o0, const Operand& o1) {
  RetNode* node = newRet(o0, o1);
  if (node == NULL)
    return node;
  return static_cast<RetNode*>(addNode(node));
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Call]
// ============================================================================

X86X64CallNode* X86X64Compiler::newCall(const Operand& o0, uint32_t conv, const FuncPrototype& p) {
  X86X64CallNode* node = newNode<X86X64CallNode>(o0);
  Error error;
  uint32_t nArgs;

  if (node == NULL)
    goto _NoMemory;

  if ((error = node->_x86Decl.setPrototype(conv, p)) != kErrorOk) {
    setError(error);
    return NULL;
  }

  // If there are no arguments skip the allocation.
  if ((nArgs = p.getArgCount()) == 0)
    return node;

  node->_args = static_cast<Operand*>(_baseZone.alloc(nArgs * sizeof(Operand)));
  if (node->_args == NULL)
    goto _NoMemory;

  ::memset(node->_args, 0, nArgs * sizeof(Operand));
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return NULL;
}

X86X64CallNode* X86X64Compiler::addCall(const Operand& o0, uint32_t conv, const FuncPrototype& p) {
  X86X64CallNode* node = newCall(o0, conv, p);
  if (node == NULL)
    return NULL;
  return static_cast<X86X64CallNode*>(addNode(node));
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Vars]
// ============================================================================

Error X86X64Compiler::setArg(uint32_t argIndex, BaseVar& var) {
  X86X64FuncNode* func = getFunc();

  if (func == NULL)
    return kErrorInvalidArgument;

  if (!isVarCreated(var))
    return kErrorInvalidState;

  VarData* vd = getVd(var);
  func->setArg(argIndex, vd);

  return kErrorOk;
}

Error X86X64Compiler::_newVar(BaseVar* var, uint32_t vType, const char* name) {
  ASMJIT_ASSERT(vType < kVarTypeCount);

  vType = _targetVarMapping[vType];
  ASMJIT_ASSERT(vType != kVarTypeInvalid);

  // There is not ASSERT in release mode and this should be checked.
  if (vType == kVarTypeInvalid) {
    static_cast<X86Var*>(var)->reset();
    return kErrorInvalidArgument;
  }

  const VarInfo& vInfo = _varInfo[vType];
  VarData* vd = _newVd(vType, vInfo.getSize(), vInfo.getClass(), name);

  if (vd == NULL) {
    static_cast<X86Var*>(var)->reset();
    return getError();
  }

  var->_init_packed_op_sz_w0_id(kOperandTypeVar, vd->getSize(), vInfo.getReg() << 8, vd->getId());
  var->_vreg.vType = vType;
  return kErrorOk;
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Stack]
// ============================================================================

Error X86X64Compiler::_newStack(BaseMem* mem, uint32_t size, uint32_t alignment, const char* name) {
  if (size == 0)
    return kErrorInvalidArgument;

  if (alignment > 64)
    alignment = 64;

  VarData* vd = _newVd(kVarTypeInvalid, size, kRegClassInvalid, name);
  if (vd == NULL) {
    static_cast<Mem*>(mem)->reset();
    return getError();
  }

  vd->_isStack = true;
  vd->_alignment = static_cast<uint8_t>(alignment);

  static_cast<Mem*>(mem)->_init(kMemTypeStackIndex, vd->getId(), 0, 0);
  return kErrorOk;
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Const]
// ============================================================================

Error X86X64Compiler::_newConst(BaseMem* mem, uint32_t scope, const void* data, size_t size) {
  Error error = kErrorOk;
  size_t offset;

  Label* dstLabel;
  ConstPool* dstPool;

  if (scope == kConstScopeLocal) {
    dstLabel = &_localConstPoolLabel;
    dstPool = &_localConstPool;
  }
  else if (scope == kConstScopeGlobal) {
    dstLabel = &_globalConstPoolLabel;
    dstPool = &_globalConstPool;
  }
  else {
    error = kErrorInvalidArgument;
    goto _OnError;
  }

  error = dstPool->add(data, size, offset);
  if (error != kErrorOk)
    goto _OnError;

  if (dstLabel->getId() == kInvalidValue) {
    error = _newLabel(dstLabel);
    if (error != kErrorOk)
      goto _OnError;
  }

  *static_cast<Mem*>(mem) = ptr(*dstLabel, static_cast<int32_t>(offset), static_cast<uint32_t>(size));
  return kErrorOk;

_OnError:
  return error;
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Make]
// ============================================================================

template<typename Assembler>
static ASMJIT_INLINE void* X86X64Compiler_make(X86X64Compiler* self) {
  Assembler assembler(self->_runtime);

#if !defined(ASMJIT_DISABLE_LOGGER)
  Logger* logger = self->_logger;
  if (logger)
    assembler.setLogger(logger);
#endif // !ASMJIT_DISABLE_LOGGER

  assembler._features = self->_features;

  if (self->serialize(assembler) != kErrorOk) {
    return NULL;
  }

  if (assembler.getError() != kErrorOk) {
    self->setError(assembler.getError());
    return NULL;
  }

  void* result = assembler.make();

#if !defined(ASMJIT_DISABLE_LOGGER)
  if (logger)
    logger->logFormat(kLoggerStyleComment,
      "*** COMPILER SUCCESS - Wrote %u bytes, code: %u, trampolines: %u.\n\n",
      static_cast<unsigned int>(assembler.getCodeSize()),
      static_cast<unsigned int>(assembler.getOffset()),
      static_cast<unsigned int>(assembler.getTrampolineSize()));
#endif // !ASMJIT_DISABLE_LOGGER

  return result;
}

void* X86X64Compiler::make() {
  // Flush global constant pool
  X86X64Compiler_emitConstPool(this, _globalConstPoolLabel, _globalConstPool);

#if defined(ASMJIT_BUILD_X86) && !defined(ASMJIT_BUILD_X64)
  return X86X64Compiler_make<x86::Assembler>(this);
#elif !defined(ASMJIT_BUILD_X86) && defined(ASMJIT_BUILD_X64)
  return X86X64Compiler_make<x64::Assembler>(this);
#else
  if (_arch == kArchX86)
    return X86X64Compiler_make<x86::Assembler>(this);
  else
    return X86X64Compiler_make<x64::Assembler>(this);
#endif // ASMJIT_BUILD_X86 && ASMJIT_BUILD_X64
}

// ============================================================================
// [asmjit::x86x64::X86X64Compiler - Assemble]
// ============================================================================

Error X86X64Compiler::serialize(BaseAssembler& assembler) {
  if (_firstNode == NULL)
    return kErrorOk;

  X86X64Context context(this);
  Error error = kErrorOk;

  Node* node = _firstNode;
  Node* start;

  // Find function and use the context to translate/emit.
  do {
    start = node;

    if (node->getType() == kNodeTypeFunc) {
      node = static_cast<X86X64FuncNode*>(start)->getEnd();
      error = context.compile(static_cast<X86X64FuncNode*>(start));

      if (error != kErrorOk)
        goto _Error;
    }

    do {
      node = node->getNext();
    } while (node != NULL && node->getType() != kNodeTypeFunc);

    error = context.serialize(&assembler, start, node);
    if (error != kErrorOk)
      goto _Error;
    context.cleanup();
  } while (node != NULL);
  return kErrorOk;

_Error:
  context.cleanup();
  return error;
}

} // x86x64 namespace
} // asmjit namespace

// ============================================================================
// [asmjit::x86]
// ============================================================================

#if defined(ASMJIT_BUILD_X86)

namespace asmjit {
namespace x86 {

Compiler::Compiler(Runtime* runtime) : X86X64Compiler(runtime) {
  _arch = kArchX86;
  _regSize = 4;
  _targetVarMapping = _varMapping;
}

Compiler::~Compiler() {}

} // x86 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X86

// ============================================================================
// [asmjit::x64]
// ============================================================================

#if defined(ASMJIT_BUILD_X64)

namespace asmjit {
namespace x64 {

Compiler::Compiler(Runtime* runtime) : X86X64Compiler(runtime) {
  _arch = kArchX64;
  _regSize = 8;
  _targetVarMapping = _varMapping;
}

Compiler::~Compiler() {}

} // x64 namespace
} // asmjit namespace

#endif // ASMJIT_BUILD_X64

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && (ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64)
