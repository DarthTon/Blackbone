// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86) && !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../x86/x86builder.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Builder - Construction / Destruction]
// ============================================================================

X86Builder::X86Builder(CodeHolder* code) noexcept : CodeBuilder() {
  if (code)
    code->attach(this);
}
X86Builder::~X86Builder() noexcept {}

// ============================================================================
// [asmjit::X86Builder - Events]
// ============================================================================

Error X86Builder::onAttach(CodeHolder* code) noexcept {
  uint32_t archType = code->getArchType();
  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  ASMJIT_PROPAGATE(Base::onAttach(code));

  if (archType == ArchInfo::kTypeX86)
    _nativeGpArray = x86OpData.gpd;
  else
    _nativeGpArray = x86OpData.gpq;
  _nativeGpReg = _nativeGpArray[0];
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Builder - Inst]
// ============================================================================

Error X86Builder::_emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {
  // TODO:
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 && !ASMJIT_DISABLE_COMPILER
