// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64)

// [Dependencies - AsmJit]
#include "../x86/x86util.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {
namespace x86x64 {

// ============================================================================
// [asmjit::x86x64::Condition Codes]
// ============================================================================

#define CC_TO_INST(_Inst_) { \
  _Inst_##o,  \
  _Inst_##no, \
  _Inst_##b,  \
  _Inst_##ae, \
  _Inst_##e,  \
  _Inst_##ne, \
  _Inst_##be, \
  _Inst_##a,  \
  _Inst_##s,  \
  _Inst_##ns, \
  _Inst_##pe, \
  _Inst_##po, \
  _Inst_##l,  \
  _Inst_##ge, \
  _Inst_##le, \
  _Inst_##g,  \
  \
  kInstNone,  \
  kInstNone,  \
  kInstNone,  \
  kInstNone   \
}

const uint32_t _reverseCond[20] = {
  /* kCondO  -> */ kCondO,
  /* kCondNO -> */ kCondNO,
  /* kCondB  -> */ kCondA,
  /* kCondAE -> */ kCondBE,
  /* kCondE  -> */ kCondE,
  /* kCondNE -> */ kCondNE,
  /* kCondBE -> */ kCondAE,
  /* kCondA  -> */ kCondB,
  /* kCondS  -> */ kCondS,
  /* kCondNS -> */ kCondNS,
  /* kCondPE -> */ kCondPE,
  /* kCondPO -> */ kCondPO,

  /* kCondL  -> */ kCondG,
  /* kCondGE -> */ kCondLE,

  /* kCondLE -> */ kCondGE,
  /* kCondG  -> */ kCondL,

  /* kCondFpuUnordered    -> */ kCondFpuUnordered,
  /* kCondFpuNotUnordered -> */ kCondFpuNotUnordered,

  0x12,
  0x13
};

const uint32_t _condToCmovcc[20] = CC_TO_INST(kInstCmov);
const uint32_t _condToJcc   [20] = CC_TO_INST(kInstJ   );
const uint32_t _condToSetcc [20] = CC_TO_INST(kInstSet );

#undef CC_TO_INST

} // x86x64 namespace
} // asmjit namespace

#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
