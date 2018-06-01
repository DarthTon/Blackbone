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
#include "../base/containers.h"
#include "../x86/x86scheduler_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Internals]
// ============================================================================

//! \internal
struct X86ScheduleData {
  //! Registers read by the instruction.
  X86RegMask regsIn;
  //! Registers written by the instruction.
  X86RegMask regsOut;

  //! Flags read by the instruction.
  uint8_t flagsIn;
  //! Flags written by the instruction.
  uint8_t flagsOut;

  //! How many `uops` or `cycles` the instruction takes.
  uint8_t ops;
  //! Instruction latency.
  uint8_t latency;

  //! Which ports the instruction can run at.
  uint16_t ports;
  //! \internal
  uint16_t reserved;

  //! All instructions that this instruction depends on.
  PodList<InstNode*>::Link* dependsOn;
  //! All instructions that use the result of this instruction.
  PodList<InstNode*>::Link* usedBy;
};

// ============================================================================
// [asmjit::X86Scheduler - Construction / Destruction]
// ============================================================================

X86Scheduler::X86Scheduler(X86Compiler* compiler, const X86CpuInfo* cpuInfo) :
  _compiler(compiler),
  _cpuInfo(cpuInfo) {}
X86Scheduler::~X86Scheduler() {}

// ============================================================================
// [asmjit::X86Scheduler - Run]
// ============================================================================

Error X86Scheduler::run(Node* start, Node* stop) {
  /*
  ASMJIT_TLOG("[Schedule] === Begin ===");

  Zone zone(8096 - kZoneOverhead);
  Node* node_ = start;

  while (node_ != stop) {
    Node* next = node_->getNext();
    ASMJIT_ASSERT(node_->getType() == kNodeTypeInst);

    printf("  %s\n", X86Util::getInstInfo(static_cast<InstNode*>(node_)->getInstId()).getInstName());
    node_ = next;
  }

  ASMJIT_TLOG("[Schedule] === End ===");
  */
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && (ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64)
