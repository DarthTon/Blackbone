// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86INST_H
#define _ASMJIT_X86_X86INST_H

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../base/compiler.h"
#include "../base/globals.h"
#include "../base/intutil.h"
#include "../base/operand.h"
#include "../base/vectypes.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct X86InstInfo;
struct X86InstExtendedInfo;

//! \addtogroup asmjit_x86_inst
//! \{

// ============================================================================
// [asmjit::X86Inst/X86Cond - Globals]
// ============================================================================

#if !defined(ASMJIT_DISABLE_NAMES)
//! \internal
//!
//! X86/X64 instructions' names, accessible through `X86InstInfo`.
ASMJIT_VAR const char _x86InstName[];
#endif // !ASMJIT_DISABLE_NAMES

//! \internal
//!
//! X86/X64 instructions' extended information, accessible through `X86InstInfo`.
ASMJIT_VAR const X86InstExtendedInfo _x86InstExtendedInfo[];

//! \internal
//!
//! X86/X64 instructions' information.
ASMJIT_VAR const X86InstInfo _x86InstInfo[];

//! \internal
//!
//! X86/X64 condition codes to reversed condition codes map.
ASMJIT_VAR const uint32_t _x86ReverseCond[20];

//! \internal
//!
//! X86/X64 condition codes to "cmovcc" group map.
ASMJIT_VAR const uint32_t _x86CondToCmovcc[20];

//! \internal
//!
//! X86/X64 condition codes to "jcc" group map.
ASMJIT_VAR const uint32_t _x86CondToJcc[20];

//! \internal
//!
//! X86/X64 condition codes to "setcc" group map.
ASMJIT_VAR const uint32_t _x86CondToSetcc[20];

// ============================================================================
// [asmjit::kX86InstId]
// ============================================================================

//! X86/X64 instruction IDs.
//!
//! Note that these instruction codes are AsmJit specific. Each instruction has
//! a unique ID that is used as an index to AsmJit instruction table. The list
//! is sorted alphabetically except instructions starting with `j`, because the
//! `jcc` instruction is composition of an opcode and condition code. It means
//! that these instructions are sorted as `jcc`, `jecxz` and `jmp`. Please use
//! \ref X86Util::getInstIdByName() if you need instruction name to ID mapping
//! and are not aware on how to handle such case.
ASMJIT_ENUM(kX86InstId) {
  kX86InstIdAdc = 1,         // X86/X64
  kX86InstIdAdd,             // X86/X64
  kX86InstIdAddpd,           // SSE2
  kX86InstIdAddps,           // SSE
  kX86InstIdAddsd,           // SSE2
  kX86InstIdAddss,           // SSE
  kX86InstIdAddsubpd,        // SSE3
  kX86InstIdAddsubps,        // SSE3
  kX86InstIdAesdec,          // AESNI
  kX86InstIdAesdeclast,      // AESNI
  kX86InstIdAesenc,          // AESNI
  kX86InstIdAesenclast,      // AESNI
  kX86InstIdAesimc,          // AESNI
  kX86InstIdAeskeygenassist, // AESNI
  kX86InstIdAnd,             // X86/X64
  kX86InstIdAndn,            // BMI
  kX86InstIdAndnpd,          // SSE2
  kX86InstIdAndnps,          // SSE
  kX86InstIdAndpd,           // SSE2
  kX86InstIdAndps,           // SSE
  kX86InstIdBextr,           // BMI
  kX86InstIdBlendpd,         // SSE4.1
  kX86InstIdBlendps,         // SSE4.1
  kX86InstIdBlendvpd,        // SSE4.1
  kX86InstIdBlendvps,        // SSE4.1
  kX86InstIdBlsi,            // BMI
  kX86InstIdBlsmsk,          // BMI
  kX86InstIdBlsr,            // BMI
  kX86InstIdBsf,             // X86/X64
  kX86InstIdBsr,             // X86/X64
  kX86InstIdBswap,           // X86/X64 (i486+)
  kX86InstIdBt,              // X86/X64
  kX86InstIdBtc,             // X86/X64
  kX86InstIdBtr,             // X86/X64
  kX86InstIdBts,             // X86/X64
  kX86InstIdBzhi,            // BMI2
  kX86InstIdCall,            // X86/X64
  kX86InstIdCbw,             // X86/X64
  kX86InstIdCdq,             // X86/X64
  kX86InstIdCdqe,            // X64 only
  kX86InstIdClc,             // X86/X64
  kX86InstIdCld,             // X86/X64
  kX86InstIdClflush,         // SSE2
  kX86InstIdCmc,             // X86/X64
  kX86InstIdCmova,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovae,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovb,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovbe,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovc,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmove,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovg,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovge,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovl,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovle,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovna,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnae,         // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnb,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnbe,         // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnc,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovne,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovng,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnge,         // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnl,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnle,         // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovno,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnp,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovns,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovnz,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovo,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovp,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovpe,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovpo,          // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovs,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmovz,           // X86/X64 (cmovcc) (i586+)
  kX86InstIdCmp,             // X86/X64
  kX86InstIdCmppd,           // SSE2
  kX86InstIdCmpps,           // SSE
  kX86InstIdCmpsB,           // CMPS - X86/X64
  kX86InstIdCmpsD,           // CMPS - X86/X64
  kX86InstIdCmpsQ,           // CMPS - X64
  kX86InstIdCmpsW,           // CMPS - X86/X64
  kX86InstIdCmpsd,           // SSE2
  kX86InstIdCmpss,           // SSE
  kX86InstIdCmpxchg,         // X86/X64 (i486+)
  kX86InstIdCmpxchg16b,      // X64 only
  kX86InstIdCmpxchg8b,       // X86/X64 (i586+)
  kX86InstIdComisd,          // SSE2
  kX86InstIdComiss,          // SSE
  kX86InstIdCpuid,           // X86/X64 (i486/i586+)
  kX86InstIdCqo,             // X64 only
  kX86InstIdCrc32,           // SSE4.2
  kX86InstIdCvtdq2pd,        // SSE2
  kX86InstIdCvtdq2ps,        // SSE2
  kX86InstIdCvtpd2dq,        // SSE2
  kX86InstIdCvtpd2pi,        // SSE2
  kX86InstIdCvtpd2ps,        // SSE2
  kX86InstIdCvtpi2pd,        // SSE2
  kX86InstIdCvtpi2ps,        // SSE
  kX86InstIdCvtps2dq,        // SSE2
  kX86InstIdCvtps2pd,        // SSE2
  kX86InstIdCvtps2pi,        // SSE
  kX86InstIdCvtsd2si,        // SSE2
  kX86InstIdCvtsd2ss,        // SSE2
  kX86InstIdCvtsi2sd,        // SSE2
  kX86InstIdCvtsi2ss,        // SSE
  kX86InstIdCvtss2sd,        // SSE2
  kX86InstIdCvtss2si,        // SSE
  kX86InstIdCvttpd2dq,       // SSE2
  kX86InstIdCvttpd2pi,       // SSE2
  kX86InstIdCvttps2dq,       // SSE2
  kX86InstIdCvttps2pi,       // SSE
  kX86InstIdCvttsd2si,       // SSE2
  kX86InstIdCvttss2si,       // SSE
  kX86InstIdCwd,             // X86/X64
  kX86InstIdCwde,            // X86/X64
  kX86InstIdDaa,             // X86 only
  kX86InstIdDas,             // X86 only
  kX86InstIdDec,             // X86/X64
  kX86InstIdDiv,             // X86/X64
  kX86InstIdDivpd,           // SSE2
  kX86InstIdDivps,           // SSE
  kX86InstIdDivsd,           // SSE2
  kX86InstIdDivss,           // SSE
  kX86InstIdDppd,            // SSE4.1
  kX86InstIdDpps,            // SSE4.1
  kX86InstIdEmms,            // MMX
  kX86InstIdEnter,           // X86/X64
  kX86InstIdExtractps,       // SSE4.1
  kX86InstIdExtrq,           // SSE4a
  kX86InstIdF2xm1,           // FPU
  kX86InstIdFabs,            // FPU
  kX86InstIdFadd,            // FPU
  kX86InstIdFaddp,           // FPU
  kX86InstIdFbld,            // FPU
  kX86InstIdFbstp,           // FPU
  kX86InstIdFchs,            // FPU
  kX86InstIdFclex,           // FPU
  kX86InstIdFcmovb,          // FPU
  kX86InstIdFcmovbe,         // FPU
  kX86InstIdFcmove,          // FPU
  kX86InstIdFcmovnb,         // FPU
  kX86InstIdFcmovnbe,        // FPU
  kX86InstIdFcmovne,         // FPU
  kX86InstIdFcmovnu,         // FPU
  kX86InstIdFcmovu,          // FPU
  kX86InstIdFcom,            // FPU
  kX86InstIdFcomi,           // FPU
  kX86InstIdFcomip,          // FPU
  kX86InstIdFcomp,           // FPU
  kX86InstIdFcompp,          // FPU
  kX86InstIdFcos,            // FPU
  kX86InstIdFdecstp,         // FPU
  kX86InstIdFdiv,            // FPU
  kX86InstIdFdivp,           // FPU
  kX86InstIdFdivr,           // FPU
  kX86InstIdFdivrp,          // FPU
  kX86InstIdFemms,           // 3dNow!
  kX86InstIdFfree,           // FPU
  kX86InstIdFiadd,           // FPU
  kX86InstIdFicom,           // FPU
  kX86InstIdFicomp,          // FPU
  kX86InstIdFidiv,           // FPU
  kX86InstIdFidivr,          // FPU
  kX86InstIdFild,            // FPU
  kX86InstIdFimul,           // FPU
  kX86InstIdFincstp,         // FPU
  kX86InstIdFinit,           // FPU
  kX86InstIdFist,            // FPU
  kX86InstIdFistp,           // FPU
  kX86InstIdFisttp,          // SSE3
  kX86InstIdFisub,           // FPU
  kX86InstIdFisubr,          // FPU
  kX86InstIdFld,             // FPU
  kX86InstIdFld1,            // FPU
  kX86InstIdFldcw,           // FPU
  kX86InstIdFldenv,          // FPU
  kX86InstIdFldl2e,          // FPU
  kX86InstIdFldl2t,          // FPU
  kX86InstIdFldlg2,          // FPU
  kX86InstIdFldln2,          // FPU
  kX86InstIdFldpi,           // FPU
  kX86InstIdFldz,            // FPU
  kX86InstIdFmul,            // FPU
  kX86InstIdFmulp,           // FPU
  kX86InstIdFnclex,          // FPU
  kX86InstIdFninit,          // FPU
  kX86InstIdFnop,            // FPU
  kX86InstIdFnsave,          // FPU
  kX86InstIdFnstcw,          // FPU
  kX86InstIdFnstenv,         // FPU
  kX86InstIdFnstsw,          // FPU
  kX86InstIdFpatan,          // FPU
  kX86InstIdFprem,           // FPU
  kX86InstIdFprem1,          // FPU
  kX86InstIdFptan,           // FPU
  kX86InstIdFrndint,         // FPU
  kX86InstIdFrstor,          // FPU
  kX86InstIdFsave,           // FPU
  kX86InstIdFscale,          // FPU
  kX86InstIdFsin,            // FPU
  kX86InstIdFsincos,         // FPU
  kX86InstIdFsqrt,           // FPU
  kX86InstIdFst,             // FPU
  kX86InstIdFstcw,           // FPU
  kX86InstIdFstenv,          // FPU
  kX86InstIdFstp,            // FPU
  kX86InstIdFstsw,           // FPU
  kX86InstIdFsub,            // FPU
  kX86InstIdFsubp,           // FPU
  kX86InstIdFsubr,           // FPU
  kX86InstIdFsubrp,          // FPU
  kX86InstIdFtst,            // FPU
  kX86InstIdFucom,           // FPU
  kX86InstIdFucomi,          // FPU
  kX86InstIdFucomip,         // FPU
  kX86InstIdFucomp,          // FPU
  kX86InstIdFucompp,         // FPU
  kX86InstIdFwait,           // FPU
  kX86InstIdFxam,            // FPU
  kX86InstIdFxch,            // FPU
  kX86InstIdFxrstor,         // FPU
  kX86InstIdFxsave,          // FPU
  kX86InstIdFxtract,         // FPU
  kX86InstIdFyl2x,           // FPU
  kX86InstIdFyl2xp1,         // FPU
  kX86InstIdHaddpd,          // SSE3
  kX86InstIdHaddps,          // SSE3
  kX86InstIdHsubpd,          // SSE3
  kX86InstIdHsubps,          // SSE3
  kX86InstIdIdiv,            // X86/X64
  kX86InstIdImul,            // X86/X64
  kX86InstIdInc,             // X86/X64
  kX86InstIdInsertps,        // SSE4.1
  kX86InstIdInsertq,         // SSE4a
  kX86InstIdInt,             // X86/X64
  kX86InstIdJa,              // X86/X64 (jcc)
  kX86InstIdJae,             // X86/X64 (jcc)
  kX86InstIdJb,              // X86/X64 (jcc)
  kX86InstIdJbe,             // X86/X64 (jcc)
  kX86InstIdJc,              // X86/X64 (jcc)
  kX86InstIdJe,              // X86/X64 (jcc)
  kX86InstIdJg,              // X86/X64 (jcc)
  kX86InstIdJge,             // X86/X64 (jcc)
  kX86InstIdJl,              // X86/X64 (jcc)
  kX86InstIdJle,             // X86/X64 (jcc)
  kX86InstIdJna,             // X86/X64 (jcc)
  kX86InstIdJnae,            // X86/X64 (jcc)
  kX86InstIdJnb,             // X86/X64 (jcc)
  kX86InstIdJnbe,            // X86/X64 (jcc)
  kX86InstIdJnc,             // X86/X64 (jcc)
  kX86InstIdJne,             // X86/X64 (jcc)
  kX86InstIdJng,             // X86/X64 (jcc)
  kX86InstIdJnge,            // X86/X64 (jcc)
  kX86InstIdJnl,             // X86/X64 (jcc)
  kX86InstIdJnle,            // X86/X64 (jcc)
  kX86InstIdJno,             // X86/X64 (jcc)
  kX86InstIdJnp,             // X86/X64 (jcc)
  kX86InstIdJns,             // X86/X64 (jcc)
  kX86InstIdJnz,             // X86/X64 (jcc)
  kX86InstIdJo,              // X86/X64 (jcc)
  kX86InstIdJp,              // X86/X64 (jcc)
  kX86InstIdJpe,             // X86/X64 (jcc)
  kX86InstIdJpo,             // X86/X64 (jcc)
  kX86InstIdJs,              // X86/X64 (jcc)
  kX86InstIdJz,              // X86/X64 (jcc)
  kX86InstIdJecxz,           // X86/X64 (jcxz/jecxz/jrcxz)
  kX86InstIdJmp,             // X86/X64 (jmp)
  kX86InstIdLahf,            // X86/X64 (CPUID NEEDED)
  kX86InstIdLddqu,           // SSE3
  kX86InstIdLdmxcsr,         // SSE
  kX86InstIdLea,             // X86/X64
  kX86InstIdLeave,           // X86/X64
  kX86InstIdLfence,          // SSE2
  kX86InstIdLodsB,           // LODS - X86/X64
  kX86InstIdLodsD,           // LODS - X86/X64
  kX86InstIdLodsQ,           // LODS - X86/X64
  kX86InstIdLodsW,           // LODS - X86/X64
  kX86InstIdLzcnt,           // LZCNT
  kX86InstIdMaskmovdqu,      // SSE2
  kX86InstIdMaskmovq,        // MMX-Ext
  kX86InstIdMaxpd,           // SSE2
  kX86InstIdMaxps,           // SSE
  kX86InstIdMaxsd,           // SSE2
  kX86InstIdMaxss,           // SSE
  kX86InstIdMfence,          // SSE2
  kX86InstIdMinpd,           // SSE2
  kX86InstIdMinps,           // SSE
  kX86InstIdMinsd,           // SSE2
  kX86InstIdMinss,           // SSE
  kX86InstIdMonitor,         // SSE3
  kX86InstIdMov,             // X86/X64
  kX86InstIdMovPtr,          // X86/X64
  kX86InstIdMovapd,          // SSE2
  kX86InstIdMovaps,          // SSE
  kX86InstIdMovbe,           // SSE3 - Intel-Atom
  kX86InstIdMovd,            // MMX/SSE2
  kX86InstIdMovddup,         // SSE3
  kX86InstIdMovdq2q,         // SSE2
  kX86InstIdMovdqa,          // SSE2
  kX86InstIdMovdqu,          // SSE2
  kX86InstIdMovhlps,         // SSE
  kX86InstIdMovhpd,          // SSE2
  kX86InstIdMovhps,          // SSE
  kX86InstIdMovlhps,         // SSE
  kX86InstIdMovlpd,          // SSE2
  kX86InstIdMovlps,          // SSE
  kX86InstIdMovmskpd,        // SSE2
  kX86InstIdMovmskps,        // SSE2
  kX86InstIdMovntdq,         // SSE2
  kX86InstIdMovntdqa,        // SSE4.1
  kX86InstIdMovnti,          // SSE2
  kX86InstIdMovntpd,         // SSE2
  kX86InstIdMovntps,         // SSE
  kX86InstIdMovntq,          // MMX-Ext
  kX86InstIdMovntsd,         // SSE4a
  kX86InstIdMovntss,         // SSE4a
  kX86InstIdMovq,            // MMX/SSE/SSE2
  kX86InstIdMovq2dq,         // SSE2
  kX86InstIdMovsB,           // MOVS - X86/X64
  kX86InstIdMovsD,           // MOVS - X86/X64
  kX86InstIdMovsQ,           // MOVS - X64
  kX86InstIdMovsW,           // MOVS - X86/X64
  kX86InstIdMovsd,           // SSE2
  kX86InstIdMovshdup,        // SSE3
  kX86InstIdMovsldup,        // SSE3
  kX86InstIdMovss,           // SSE
  kX86InstIdMovsx,           // X86/X64
  kX86InstIdMovsxd,          // X86/X64
  kX86InstIdMovupd,          // SSE2
  kX86InstIdMovups,          // SSE
  kX86InstIdMovzx,           // X86/X64
  kX86InstIdMpsadbw,         // SSE4.1
  kX86InstIdMul,             // X86/X64
  kX86InstIdMulpd,           // SSE2
  kX86InstIdMulps,           // SSE
  kX86InstIdMulsd,           // SSE2
  kX86InstIdMulss,           // SSE
  kX86InstIdMulx,            // BMI2
  kX86InstIdMwait,           // SSE3
  kX86InstIdNeg,             // X86/X64
  kX86InstIdNop,             // X86/X64
  kX86InstIdNot,             // X86/X64
  kX86InstIdOr,              // X86/X64
  kX86InstIdOrpd,            // SSE2
  kX86InstIdOrps,            // SSE
  kX86InstIdPabsb,           // SSSE3
  kX86InstIdPabsd,           // SSSE3
  kX86InstIdPabsw,           // SSSE3
  kX86InstIdPackssdw,        // MMX/SSE2
  kX86InstIdPacksswb,        // MMX/SSE2
  kX86InstIdPackusdw,        // SSE4.1
  kX86InstIdPackuswb,        // MMX/SSE2
  kX86InstIdPaddb,           // MMX/SSE2
  kX86InstIdPaddd,           // MMX/SSE2
  kX86InstIdPaddq,           // SSE2
  kX86InstIdPaddsb,          // MMX/SSE2
  kX86InstIdPaddsw,          // MMX/SSE2
  kX86InstIdPaddusb,         // MMX/SSE2
  kX86InstIdPaddusw,         // MMX/SSE2
  kX86InstIdPaddw,           // MMX/SSE2
  kX86InstIdPalignr,         // SSSE3
  kX86InstIdPand,            // MMX/SSE2
  kX86InstIdPandn,           // MMX/SSE2
  kX86InstIdPause,           // SSE2.
  kX86InstIdPavgb,           // MMX-Ext
  kX86InstIdPavgw,           // MMX-Ext
  kX86InstIdPblendvb,        // SSE4.1
  kX86InstIdPblendw,         // SSE4.1
  kX86InstIdPclmulqdq,       // PCLMULQDQ
  kX86InstIdPcmpeqb,         // MMX/SSE2
  kX86InstIdPcmpeqd,         // MMX/SSE2
  kX86InstIdPcmpeqq,         // SSE4.1
  kX86InstIdPcmpeqw,         // MMX/SSE2
  kX86InstIdPcmpestri,       // SSE4.2
  kX86InstIdPcmpestrm,       // SSE4.2
  kX86InstIdPcmpgtb,         // MMX/SSE2
  kX86InstIdPcmpgtd,         // MMX/SSE2
  kX86InstIdPcmpgtq,         // SSE4.2
  kX86InstIdPcmpgtw,         // MMX/SSE2
  kX86InstIdPcmpistri,       // SSE4.2
  kX86InstIdPcmpistrm,       // SSE4.2
  kX86InstIdPdep,            // BMI2
  kX86InstIdPext,            // BMI2
  kX86InstIdPextrb,          // SSE4.1
  kX86InstIdPextrd,          // SSE4.1
  kX86InstIdPextrq,          // SSE4.1
  kX86InstIdPextrw,          // MMX-Ext/SSE2
  kX86InstIdPf2id,           // 3dNow!
  kX86InstIdPf2iw,           // Enhanced 3dNow!
  kX86InstIdPfacc,           // 3dNow!
  kX86InstIdPfadd,           // 3dNow!
  kX86InstIdPfcmpeq,         // 3dNow!
  kX86InstIdPfcmpge,         // 3dNow!
  kX86InstIdPfcmpgt,         // 3dNow!
  kX86InstIdPfmax,           // 3dNow!
  kX86InstIdPfmin,           // 3dNow!
  kX86InstIdPfmul,           // 3dNow!
  kX86InstIdPfnacc,          // Enhanced 3dNow!
  kX86InstIdPfpnacc,         // Enhanced 3dNow!
  kX86InstIdPfrcp,           // 3dNow!
  kX86InstIdPfrcpit1,        // 3dNow!
  kX86InstIdPfrcpit2,        // 3dNow!
  kX86InstIdPfrsqit1,        // 3dNow!
  kX86InstIdPfrsqrt,         // 3dNow!
  kX86InstIdPfsub,           // 3dNow!
  kX86InstIdPfsubr,          // 3dNow!
  kX86InstIdPhaddd,          // SSSE3
  kX86InstIdPhaddsw,         // SSSE3
  kX86InstIdPhaddw,          // SSSE3
  kX86InstIdPhminposuw,      // SSE4.1
  kX86InstIdPhsubd,          // SSSE3
  kX86InstIdPhsubsw,         // SSSE3
  kX86InstIdPhsubw,          // SSSE3
  kX86InstIdPi2fd,           // 3dNow!
  kX86InstIdPi2fw,           // Enhanced 3dNow!
  kX86InstIdPinsrb,          // SSE4.1
  kX86InstIdPinsrd,          // SSE4.1
  kX86InstIdPinsrq,          // SSE4.1
  kX86InstIdPinsrw,          // MMX-Ext
  kX86InstIdPmaddubsw,       // SSSE3
  kX86InstIdPmaddwd,         // MMX/SSE2
  kX86InstIdPmaxsb,          // SSE4.1
  kX86InstIdPmaxsd,          // SSE4.1
  kX86InstIdPmaxsw,          // MMX-Ext
  kX86InstIdPmaxub,          // MMX-Ext
  kX86InstIdPmaxud,          // SSE4.1
  kX86InstIdPmaxuw,          // SSE4.1
  kX86InstIdPminsb,          // SSE4.1
  kX86InstIdPminsd,          // SSE4.1
  kX86InstIdPminsw,          // MMX-Ext
  kX86InstIdPminub,          // MMX-Ext
  kX86InstIdPminud,          // SSE4.1
  kX86InstIdPminuw,          // SSE4.1
  kX86InstIdPmovmskb,        // MMX-Ext
  kX86InstIdPmovsxbd,        // SSE4.1
  kX86InstIdPmovsxbq,        // SSE4.1
  kX86InstIdPmovsxbw,        // SSE4.1
  kX86InstIdPmovsxdq,        // SSE4.1
  kX86InstIdPmovsxwd,        // SSE4.1
  kX86InstIdPmovsxwq,        // SSE4.1
  kX86InstIdPmovzxbd,        // SSE4.1
  kX86InstIdPmovzxbq,        // SSE4.1
  kX86InstIdPmovzxbw,        // SSE4.1
  kX86InstIdPmovzxdq,        // SSE4.1
  kX86InstIdPmovzxwd,        // SSE4.1
  kX86InstIdPmovzxwq,        // SSE4.1
  kX86InstIdPmuldq,          // SSE4.1
  kX86InstIdPmulhrsw,        // SSSE3
  kX86InstIdPmulhuw,         // MMX-Ext
  kX86InstIdPmulhw,          // MMX/SSE2
  kX86InstIdPmulld,          // SSE4.1
  kX86InstIdPmullw,          // MMX/SSE2
  kX86InstIdPmuludq,         // SSE2
  kX86InstIdPop,             // X86/X64
  kX86InstIdPopa,            // X86 only
  kX86InstIdPopcnt,          // SSE4.2
  kX86InstIdPopf,            // X86/X64
  kX86InstIdPor,             // MMX/SSE2
  kX86InstIdPrefetch,        // MMX-Ext/SSE
  kX86InstIdPrefetch3dNow,   // 3dNow!
  kX86InstIdPrefetchw3dNow,  // 3dNow!
  kX86InstIdPsadbw,          // MMX-Ext
  kX86InstIdPshufb,          // SSSE3
  kX86InstIdPshufd,          // SSE2
  kX86InstIdPshufhw,         // SSE2
  kX86InstIdPshuflw,         // SSE2
  kX86InstIdPshufw,          // MMX-Ext
  kX86InstIdPsignb,          // SSSE3
  kX86InstIdPsignd,          // SSSE3
  kX86InstIdPsignw,          // SSSE3
  kX86InstIdPslld,           // MMX/SSE2
  kX86InstIdPslldq,          // SSE2
  kX86InstIdPsllq,           // MMX/SSE2
  kX86InstIdPsllw,           // MMX/SSE2
  kX86InstIdPsrad,           // MMX/SSE2
  kX86InstIdPsraw,           // MMX/SSE2
  kX86InstIdPsrld,           // MMX/SSE2
  kX86InstIdPsrldq,          // SSE2
  kX86InstIdPsrlq,           // MMX/SSE2
  kX86InstIdPsrlw,           // MMX/SSE2
  kX86InstIdPsubb,           // MMX/SSE2
  kX86InstIdPsubd,           // MMX/SSE2
  kX86InstIdPsubq,           // SSE2
  kX86InstIdPsubsb,          // MMX/SSE2
  kX86InstIdPsubsw,          // MMX/SSE2
  kX86InstIdPsubusb,         // MMX/SSE2
  kX86InstIdPsubusw,         // MMX/SSE2
  kX86InstIdPsubw,           // MMX/SSE2
  kX86InstIdPswapd,          // Enhanced 3dNow!
  kX86InstIdPtest,           // SSE4.1
  kX86InstIdPunpckhbw,       // MMX/SSE2
  kX86InstIdPunpckhdq,       // MMX/SSE2
  kX86InstIdPunpckhqdq,      // SSE2
  kX86InstIdPunpckhwd,       // MMX/SSE2
  kX86InstIdPunpcklbw,       // MMX/SSE2
  kX86InstIdPunpckldq,       // MMX/SSE2
  kX86InstIdPunpcklqdq,      // SSE2
  kX86InstIdPunpcklwd,       // MMX/SSE2
  kX86InstIdPush,            // X86/X64
  kX86InstIdPusha,           // X86 only
  kX86InstIdPushf,           // X86/X64
  kX86InstIdPxor,            // MMX/SSE2
  kX86InstIdRcl,             // X86/X64
  kX86InstIdRcpps,           // SSE
  kX86InstIdRcpss,           // SSE
  kX86InstIdRcr,             // X86/X64
  kX86InstIdRdfsbase,        // FSGSBASE (x64)
  kX86InstIdRdgsbase,        // FSGSBASE (x64)
  kX86InstIdRdrand,          // RDRAND
  kX86InstIdRdtsc,           // X86/X64
  kX86InstIdRdtscp,          // X86/X64
  kX86InstIdRepLodsB,        // X86/X64 (REP)
  kX86InstIdRepLodsD,        // X86/X64 (REP)
  kX86InstIdRepLodsQ,        // X64 only (REP)
  kX86InstIdRepLodsW,        // X86/X64 (REP)
  kX86InstIdRepMovsB,        // X86/X64 (REP)
  kX86InstIdRepMovsD,        // X86/X64 (REP)
  kX86InstIdRepMovsQ,        // X64 only (REP)
  kX86InstIdRepMovsW,        // X86/X64 (REP)
  kX86InstIdRepStosB,        // X86/X64 (REP)
  kX86InstIdRepStosD,        // X86/X64 (REP)
  kX86InstIdRepStosQ,        // X64 only (REP)
  kX86InstIdRepStosW,        // X86/X64 (REP)
  kX86InstIdRepeCmpsB,       // X86/X64 (REP)
  kX86InstIdRepeCmpsD,       // X86/X64 (REP)
  kX86InstIdRepeCmpsQ,       // X64 only (REP)
  kX86InstIdRepeCmpsW,       // X86/X64 (REP)
  kX86InstIdRepeScasB,       // X86/X64 (REP)
  kX86InstIdRepeScasD,       // X86/X64 (REP)
  kX86InstIdRepeScasQ,       // X64 only (REP)
  kX86InstIdRepeScasW,       // X86/X64 (REP)
  kX86InstIdRepneCmpsB,      // X86/X64 (REP)
  kX86InstIdRepneCmpsD,      // X86/X64 (REP)
  kX86InstIdRepneCmpsQ,      // X64 only (REP)
  kX86InstIdRepneCmpsW,      // X86/X64 (REP)
  kX86InstIdRepneScasB,      // X86/X64 (REP)
  kX86InstIdRepneScasD,      // X86/X64 (REP)
  kX86InstIdRepneScasQ,      // X64 only (REP)
  kX86InstIdRepneScasW,      // X86/X64 (REP)
  kX86InstIdRet,             // X86/X64
  kX86InstIdRol,             // X86/X64
  kX86InstIdRor,             // X86/X64
  kX86InstIdRorx,            // BMI2
  kX86InstIdRoundpd,         // SSE4.1
  kX86InstIdRoundps,         // SSE4.1
  kX86InstIdRoundsd,         // SSE4.1
  kX86InstIdRoundss,         // SSE4.1
  kX86InstIdRsqrtps,         // SSE
  kX86InstIdRsqrtss,         // SSE
  kX86InstIdSahf,            // X86/X64 (CPUID NEEDED)
  kX86InstIdSal,             // X86/X64
  kX86InstIdSar,             // X86/X64
  kX86InstIdSarx,            // BMI2
  kX86InstIdSbb,             // X86/X64
  kX86InstIdScasB,           // SCAS - X86/X64
  kX86InstIdScasD,           // SCAS - X86/X64
  kX86InstIdScasQ,           // SCAS - X64
  kX86InstIdScasW,           // SCAS - X86/X64
  kX86InstIdSeta,            // X86/X64 (setcc)
  kX86InstIdSetae,           // X86/X64 (setcc)
  kX86InstIdSetb,            // X86/X64 (setcc)
  kX86InstIdSetbe,           // X86/X64 (setcc)
  kX86InstIdSetc,            // X86/X64 (setcc)
  kX86InstIdSete,            // X86/X64 (setcc)
  kX86InstIdSetg,            // X86/X64 (setcc)
  kX86InstIdSetge,           // X86/X64 (setcc)
  kX86InstIdSetl,            // X86/X64 (setcc)
  kX86InstIdSetle,           // X86/X64 (setcc)
  kX86InstIdSetna,           // X86/X64 (setcc)
  kX86InstIdSetnae,          // X86/X64 (setcc)
  kX86InstIdSetnb,           // X86/X64 (setcc)
  kX86InstIdSetnbe,          // X86/X64 (setcc)
  kX86InstIdSetnc,           // X86/X64 (setcc)
  kX86InstIdSetne,           // X86/X64 (setcc)
  kX86InstIdSetng,           // X86/X64 (setcc)
  kX86InstIdSetnge,          // X86/X64 (setcc)
  kX86InstIdSetnl,           // X86/X64 (setcc)
  kX86InstIdSetnle,          // X86/X64 (setcc)
  kX86InstIdSetno,           // X86/X64 (setcc)
  kX86InstIdSetnp,           // X86/X64 (setcc)
  kX86InstIdSetns,           // X86/X64 (setcc)
  kX86InstIdSetnz,           // X86/X64 (setcc)
  kX86InstIdSeto,            // X86/X64 (setcc)
  kX86InstIdSetp,            // X86/X64 (setcc)
  kX86InstIdSetpe,           // X86/X64 (setcc)
  kX86InstIdSetpo,           // X86/X64 (setcc)
  kX86InstIdSets,            // X86/X64 (setcc)
  kX86InstIdSetz,            // X86/X64 (setcc)
  kX86InstIdSfence,          // MMX-Ext/SSE
  kX86InstIdShl,             // X86/X64
  kX86InstIdShld,            // X86/X64
  kX86InstIdShlx,            // BMI2
  kX86InstIdShr,             // X86/X64
  kX86InstIdShrd,            // X86/X64
  kX86InstIdShrx,            // BMI2
  kX86InstIdShufpd,          // SSE2
  kX86InstIdShufps,          // SSE
  kX86InstIdSqrtpd,          // SSE2
  kX86InstIdSqrtps,          // SSE
  kX86InstIdSqrtsd,          // SSE2
  kX86InstIdSqrtss,          // SSE
  kX86InstIdStc,             // X86/X64
  kX86InstIdStd,             // X86/X64
  kX86InstIdStmxcsr,         // SSE
  kX86InstIdStosB,           // STOS - X86/X64
  kX86InstIdStosD,           // STOS - X86/X64
  kX86InstIdStosQ,           // STOS - X64
  kX86InstIdStosW,           // STOS - X86/X64
  kX86InstIdSub,             // X86/X64
  kX86InstIdSubpd,           // SSE2
  kX86InstIdSubps,           // SSE
  kX86InstIdSubsd,           // SSE2
  kX86InstIdSubss,           // SSE
  kX86InstIdTest,            // X86/X64
  kX86InstIdTzcnt,           // TZCNT
  kX86InstIdUcomisd,         // SSE2
  kX86InstIdUcomiss,         // SSE
  kX86InstIdUd2,             // X86/X64
  kX86InstIdUnpckhpd,        // SSE2
  kX86InstIdUnpckhps,        // SSE
  kX86InstIdUnpcklpd,        // SSE2
  kX86InstIdUnpcklps,        // SSE
  kX86InstIdVaddpd,          // AVX
  kX86InstIdVaddps,          // AVX
  kX86InstIdVaddsd,          // AVX
  kX86InstIdVaddss,          // AVX
  kX86InstIdVaddsubpd,       // AVX
  kX86InstIdVaddsubps,       // AVX
  kX86InstIdVaesdec,         // AVX+AESNI
  kX86InstIdVaesdeclast,     // AVX+AESNI
  kX86InstIdVaesenc,         // AVX+AESNI
  kX86InstIdVaesenclast,     // AVX+AESNI
  kX86InstIdVaesimc,         // AVX+AESNI
  kX86InstIdVaeskeygenassist,// AVX+AESNI
  kX86InstIdVandnpd,         // AVX
  kX86InstIdVandnps,         // AVX
  kX86InstIdVandpd,          // AVX
  kX86InstIdVandps,          // AVX
  kX86InstIdVblendpd,        // AVX
  kX86InstIdVblendps,        // AVX
  kX86InstIdVblendvpd,       // AVX
  kX86InstIdVblendvps,       // AVX
  kX86InstIdVbroadcastf128,  // AVX
  kX86InstIdVbroadcasti128,  // AVX2
  kX86InstIdVbroadcastsd,    // AVX/AVX2
  kX86InstIdVbroadcastss,    // AVX/AVX2
  kX86InstIdVcmppd,          // AVX
  kX86InstIdVcmpps,          // AVX
  kX86InstIdVcmpsd,          // AVX
  kX86InstIdVcmpss,          // AVX
  kX86InstIdVcomisd,         // AVX
  kX86InstIdVcomiss,         // AVX
  kX86InstIdVcvtdq2pd,       // AVX
  kX86InstIdVcvtdq2ps,       // AVX
  kX86InstIdVcvtpd2dq,       // AVX
  kX86InstIdVcvtpd2ps,       // AVX
  kX86InstIdVcvtph2ps,       // F16C
  kX86InstIdVcvtps2dq,       // AVX
  kX86InstIdVcvtps2pd,       // AVX
  kX86InstIdVcvtps2ph,       // F16C
  kX86InstIdVcvtsd2si,       // AVX
  kX86InstIdVcvtsd2ss,       // AVX
  kX86InstIdVcvtsi2sd,       // AVX
  kX86InstIdVcvtsi2ss,       // AVX
  kX86InstIdVcvtss2sd,       // AVX
  kX86InstIdVcvtss2si,       // AVX
  kX86InstIdVcvttpd2dq,      // AVX
  kX86InstIdVcvttps2dq,      // AVX
  kX86InstIdVcvttsd2si,      // AVX
  kX86InstIdVcvttss2si,      // AVX
  kX86InstIdVdivpd,          // AVX
  kX86InstIdVdivps,          // AVX
  kX86InstIdVdivsd,          // AVX
  kX86InstIdVdivss,          // AVX
  kX86InstIdVdppd,           // AVX
  kX86InstIdVdpps,           // AVX
  kX86InstIdVextractf128,    // AVX
  kX86InstIdVextracti128,    // AVX2
  kX86InstIdVextractps,      // AVX
  kX86InstIdVfmadd132pd,     // FMA3
  kX86InstIdVfmadd132ps,     // FMA3
  kX86InstIdVfmadd132sd,     // FMA3
  kX86InstIdVfmadd132ss,     // FMA3
  kX86InstIdVfmadd213pd,     // FMA3
  kX86InstIdVfmadd213ps,     // FMA3
  kX86InstIdVfmadd213sd,     // FMA3
  kX86InstIdVfmadd213ss,     // FMA3
  kX86InstIdVfmadd231pd,     // FMA3
  kX86InstIdVfmadd231ps,     // FMA3
  kX86InstIdVfmadd231sd,     // FMA3
  kX86InstIdVfmadd231ss,     // FMA3
  kX86InstIdVfmaddpd,        // FMA4
  kX86InstIdVfmaddps,        // FMA4
  kX86InstIdVfmaddsd,        // FMA4
  kX86InstIdVfmaddss,        // FMA4
  kX86InstIdVfmaddsub132pd,  // FMA3
  kX86InstIdVfmaddsub132ps,  // FMA3
  kX86InstIdVfmaddsub213pd,  // FMA3
  kX86InstIdVfmaddsub213ps,  // FMA3
  kX86InstIdVfmaddsub231pd,  // FMA3
  kX86InstIdVfmaddsub231ps,  // FMA3
  kX86InstIdVfmaddsubpd,     // FMA4
  kX86InstIdVfmaddsubps,     // FMA4
  kX86InstIdVfmsub132pd,     // FMA3
  kX86InstIdVfmsub132ps,     // FMA3
  kX86InstIdVfmsub132sd,     // FMA3
  kX86InstIdVfmsub132ss,     // FMA3
  kX86InstIdVfmsub213pd,     // FMA3
  kX86InstIdVfmsub213ps,     // FMA3
  kX86InstIdVfmsub213sd,     // FMA3
  kX86InstIdVfmsub213ss,     // FMA3
  kX86InstIdVfmsub231pd,     // FMA3
  kX86InstIdVfmsub231ps,     // FMA3
  kX86InstIdVfmsub231sd,     // FMA3
  kX86InstIdVfmsub231ss,     // FMA3
  kX86InstIdVfmsubadd132pd,  // FMA3
  kX86InstIdVfmsubadd132ps,  // FMA3
  kX86InstIdVfmsubadd213pd,  // FMA3
  kX86InstIdVfmsubadd213ps,  // FMA3
  kX86InstIdVfmsubadd231pd,  // FMA3
  kX86InstIdVfmsubadd231ps,  // FMA3
  kX86InstIdVfmsubaddpd,     // FMA4
  kX86InstIdVfmsubaddps,     // FMA4
  kX86InstIdVfmsubpd,        // FMA4
  kX86InstIdVfmsubps,        // FMA4
  kX86InstIdVfmsubsd,        // FMA4
  kX86InstIdVfmsubss,        // FMA4
  kX86InstIdVfnmadd132pd,    // FMA3
  kX86InstIdVfnmadd132ps,    // FMA3
  kX86InstIdVfnmadd132sd,    // FMA3
  kX86InstIdVfnmadd132ss,    // FMA3
  kX86InstIdVfnmadd213pd,    // FMA3
  kX86InstIdVfnmadd213ps,    // FMA3
  kX86InstIdVfnmadd213sd,    // FMA3
  kX86InstIdVfnmadd213ss,    // FMA3
  kX86InstIdVfnmadd231pd,    // FMA3
  kX86InstIdVfnmadd231ps,    // FMA3
  kX86InstIdVfnmadd231sd,    // FMA3
  kX86InstIdVfnmadd231ss,    // FMA3
  kX86InstIdVfnmaddpd,       // FMA4
  kX86InstIdVfnmaddps,       // FMA4
  kX86InstIdVfnmaddsd,       // FMA4
  kX86InstIdVfnmaddss,       // FMA4
  kX86InstIdVfnmsub132pd,    // FMA3
  kX86InstIdVfnmsub132ps,    // FMA3
  kX86InstIdVfnmsub132sd,    // FMA3
  kX86InstIdVfnmsub132ss,    // FMA3
  kX86InstIdVfnmsub213pd,    // FMA3
  kX86InstIdVfnmsub213ps,    // FMA3
  kX86InstIdVfnmsub213sd,    // FMA3
  kX86InstIdVfnmsub213ss,    // FMA3
  kX86InstIdVfnmsub231pd,    // FMA3
  kX86InstIdVfnmsub231ps,    // FMA3
  kX86InstIdVfnmsub231sd,    // FMA3
  kX86InstIdVfnmsub231ss,    // FMA3
  kX86InstIdVfnmsubpd,       // FMA4
  kX86InstIdVfnmsubps,       // FMA4
  kX86InstIdVfnmsubsd,       // FMA4
  kX86InstIdVfnmsubss,       // FMA4
  kX86InstIdVfrczpd,         // XOP
  kX86InstIdVfrczps,         // XOP
  kX86InstIdVfrczsd,         // XOP
  kX86InstIdVfrczss,         // XOP
  kX86InstIdVgatherdpd,      // AVX2
  kX86InstIdVgatherdps,      // AVX2
  kX86InstIdVgatherqpd,      // AVX2
  kX86InstIdVgatherqps,      // AVX2
  kX86InstIdVhaddpd,         // AVX
  kX86InstIdVhaddps,         // AVX
  kX86InstIdVhsubpd,         // AVX
  kX86InstIdVhsubps,         // AVX
  kX86InstIdVinsertf128,     // AVX
  kX86InstIdVinserti128,     // AVX2
  kX86InstIdVinsertps,       // AVX
  kX86InstIdVlddqu,          // AVX
  kX86InstIdVldmxcsr,        // AVX
  kX86InstIdVmaskmovdqu,     // AVX
  kX86InstIdVmaskmovpd,      // AVX
  kX86InstIdVmaskmovps,      // AVX
  kX86InstIdVmaxpd,          // AVX
  kX86InstIdVmaxps,          // AVX
  kX86InstIdVmaxsd,          // AVX
  kX86InstIdVmaxss,          // AVX
  kX86InstIdVminpd,          // AVX
  kX86InstIdVminps,          // AVX
  kX86InstIdVminsd,          // AVX
  kX86InstIdVminss,          // AVX
  kX86InstIdVmovapd,         // AVX
  kX86InstIdVmovaps,         // AVX
  kX86InstIdVmovd,           // AVX
  kX86InstIdVmovddup,        // AVX
  kX86InstIdVmovdqa,         // AVX
  kX86InstIdVmovdqu,         // AVX
  kX86InstIdVmovhlps,        // AVX
  kX86InstIdVmovhpd,         // AVX
  kX86InstIdVmovhps,         // AVX
  kX86InstIdVmovlhps,        // AVX
  kX86InstIdVmovlpd,         // AVX
  kX86InstIdVmovlps,         // AVX
  kX86InstIdVmovmskpd,       // AVX
  kX86InstIdVmovmskps,       // AVX
  kX86InstIdVmovntdq,        // AVX
  kX86InstIdVmovntdqa,       // AVX/AVX2
  kX86InstIdVmovntpd,        // AVX
  kX86InstIdVmovntps,        // AVX
  kX86InstIdVmovq,           // AVX
  kX86InstIdVmovsd,          // AVX
  kX86InstIdVmovshdup,       // AVX
  kX86InstIdVmovsldup,       // AVX
  kX86InstIdVmovss,          // AVX
  kX86InstIdVmovupd,         // AVX
  kX86InstIdVmovups,         // AVX
  kX86InstIdVmpsadbw,        // AVX/AVX2
  kX86InstIdVmulpd,          // AVX
  kX86InstIdVmulps,          // AVX
  kX86InstIdVmulsd,          // AVX
  kX86InstIdVmulss,          // AVX
  kX86InstIdVorpd,           // AVX
  kX86InstIdVorps,           // AVX
  kX86InstIdVpabsb,          // AVX2
  kX86InstIdVpabsd,          // AVX2
  kX86InstIdVpabsw,          // AVX2
  kX86InstIdVpackssdw,       // AVX2
  kX86InstIdVpacksswb,       // AVX2
  kX86InstIdVpackusdw,       // AVX2
  kX86InstIdVpackuswb,       // AVX2
  kX86InstIdVpaddb,          // AVX2
  kX86InstIdVpaddd,          // AVX2
  kX86InstIdVpaddq,          // AVX2
  kX86InstIdVpaddsb,         // AVX2
  kX86InstIdVpaddsw,         // AVX2
  kX86InstIdVpaddusb,        // AVX2
  kX86InstIdVpaddusw,        // AVX2
  kX86InstIdVpaddw,          // AVX2
  kX86InstIdVpalignr,        // AVX2
  kX86InstIdVpand,           // AVX2
  kX86InstIdVpandn,          // AVX2
  kX86InstIdVpavgb,          // AVX2
  kX86InstIdVpavgw,          // AVX2
  kX86InstIdVpblendd,        // AVX2
  kX86InstIdVpblendvb,       // AVX2
  kX86InstIdVpblendw,        // AVX2
  kX86InstIdVpbroadcastb,    // AVX2
  kX86InstIdVpbroadcastd,    // AVX2
  kX86InstIdVpbroadcastq,    // AVX2
  kX86InstIdVpbroadcastw,    // AVX2
  kX86InstIdVpclmulqdq,      // AVX+PCLMULQDQ
  kX86InstIdVpcmov,          // XOP
  kX86InstIdVpcmpeqb,        // AVX2
  kX86InstIdVpcmpeqd,        // AVX2
  kX86InstIdVpcmpeqq,        // AVX2
  kX86InstIdVpcmpeqw,        // AVX2
  kX86InstIdVpcmpestri,      // AVX
  kX86InstIdVpcmpestrm,      // AVX
  kX86InstIdVpcmpgtb,        // AVX2
  kX86InstIdVpcmpgtd,        // AVX2
  kX86InstIdVpcmpgtq,        // AVX2
  kX86InstIdVpcmpgtw,        // AVX2
  kX86InstIdVpcmpistri,      // AVX
  kX86InstIdVpcmpistrm,      // AVX
  kX86InstIdVpcomb,          // XOP
  kX86InstIdVpcomd,          // XOP
  kX86InstIdVpcomq,          // XOP
  kX86InstIdVpcomub,         // XOP
  kX86InstIdVpcomud,         // XOP
  kX86InstIdVpcomuq,         // XOP
  kX86InstIdVpcomuw,         // XOP
  kX86InstIdVpcomw,          // XOP
  kX86InstIdVperm2f128,      // AVX
  kX86InstIdVperm2i128,      // AVX2
  kX86InstIdVpermd,          // AVX2
  kX86InstIdVpermil2pd,      // XOP
  kX86InstIdVpermil2ps,      // XOP
  kX86InstIdVpermilpd,       // AVX
  kX86InstIdVpermilps,       // AVX
  kX86InstIdVpermpd,         // AVX2
  kX86InstIdVpermps,         // AVX2
  kX86InstIdVpermq,          // AVX2
  kX86InstIdVpextrb,         // AVX
  kX86InstIdVpextrd,         // AVX
  kX86InstIdVpextrq,         // AVX (x64 only)
  kX86InstIdVpextrw,         // AVX
  kX86InstIdVpgatherdd,      // AVX2
  kX86InstIdVpgatherdq,      // AVX2
  kX86InstIdVpgatherqd,      // AVX2
  kX86InstIdVpgatherqq,      // AVX2
  kX86InstIdVphaddbd,        // XOP
  kX86InstIdVphaddbq,        // XOP
  kX86InstIdVphaddbw,        // XOP
  kX86InstIdVphaddd,         // AVX2
  kX86InstIdVphadddq,        // XOP
  kX86InstIdVphaddsw,        // AVX2
  kX86InstIdVphaddubd,       // XOP
  kX86InstIdVphaddubq,       // XOP
  kX86InstIdVphaddubw,       // XOP
  kX86InstIdVphaddudq,       // XOP
  kX86InstIdVphadduwd,       // XOP
  kX86InstIdVphadduwq,       // XOP
  kX86InstIdVphaddw,         // AVX2
  kX86InstIdVphaddwd,        // XOP
  kX86InstIdVphaddwq,        // XOP
  kX86InstIdVphminposuw,     // AVX
  kX86InstIdVphsubbw,        // XOP
  kX86InstIdVphsubd,         // AVX2
  kX86InstIdVphsubdq,        // XOP
  kX86InstIdVphsubsw,        // AVX2
  kX86InstIdVphsubw,         // AVX2
  kX86InstIdVphsubwd,        // XOP
  kX86InstIdVpinsrb,         // AVX
  kX86InstIdVpinsrd,         // AVX
  kX86InstIdVpinsrq,         // AVX (x64 only)
  kX86InstIdVpinsrw,         // AVX
  kX86InstIdVpmacsdd,        // XOP
  kX86InstIdVpmacsdqh,       // XOP
  kX86InstIdVpmacsdql,       // XOP
  kX86InstIdVpmacssdd,       // XOP
  kX86InstIdVpmacssdqh,      // XOP
  kX86InstIdVpmacssdql,      // XOP
  kX86InstIdVpmacsswd,       // XOP
  kX86InstIdVpmacssww,       // XOP
  kX86InstIdVpmacswd,        // XOP
  kX86InstIdVpmacsww,        // XOP
  kX86InstIdVpmadcsswd,      // XOP
  kX86InstIdVpmadcswd,       // XOP
  kX86InstIdVpmaddubsw,      // AVX/AVX2
  kX86InstIdVpmaddwd,        // AVX/AVX2
  kX86InstIdVpmaskmovd,      // AVX2
  kX86InstIdVpmaskmovq,      // AVX2
  kX86InstIdVpmaxsb,         // AVX/AVX2
  kX86InstIdVpmaxsd,         // AVX/AVX2
  kX86InstIdVpmaxsw,         // AVX/AVX2
  kX86InstIdVpmaxub,         // AVX/AVX2
  kX86InstIdVpmaxud,         // AVX/AVX2
  kX86InstIdVpmaxuw,         // AVX/AVX2
  kX86InstIdVpminsb,         // AVX/AVX2
  kX86InstIdVpminsd,         // AVX/AVX2
  kX86InstIdVpminsw,         // AVX/AVX2
  kX86InstIdVpminub,         // AVX/AVX2
  kX86InstIdVpminud,         // AVX/AVX2
  kX86InstIdVpminuw,         // AVX/AVX2
  kX86InstIdVpmovmskb,       // AVX/AVX2
  kX86InstIdVpmovsxbd,       // AVX/AVX2
  kX86InstIdVpmovsxbq,       // AVX/AVX2
  kX86InstIdVpmovsxbw,       // AVX/AVX2
  kX86InstIdVpmovsxdq,       // AVX/AVX2
  kX86InstIdVpmovsxwd,       // AVX/AVX2
  kX86InstIdVpmovsxwq,       // AVX/AVX2
  kX86InstIdVpmovzxbd,       // AVX/AVX2
  kX86InstIdVpmovzxbq,       // AVX/AVX2
  kX86InstIdVpmovzxbw,       // AVX/AVX2
  kX86InstIdVpmovzxdq,       // AVX/AVX2
  kX86InstIdVpmovzxwd,       // AVX/AVX2
  kX86InstIdVpmovzxwq,       // AVX/AVX2
  kX86InstIdVpmuldq,         // AVX/AVX2
  kX86InstIdVpmulhrsw,       // AVX/AVX2
  kX86InstIdVpmulhuw,        // AVX/AVX2
  kX86InstIdVpmulhw,         // AVX/AVX2
  kX86InstIdVpmulld,         // AVX/AVX2
  kX86InstIdVpmullw,         // AVX/AVX2
  kX86InstIdVpmuludq,        // AVX/AVX2
  kX86InstIdVpor,            // AVX/AVX2
  kX86InstIdVpperm,          // XOP
  kX86InstIdVprotb,          // XOP
  kX86InstIdVprotd,          // XOP
  kX86InstIdVprotq,          // XOP
  kX86InstIdVprotw,          // XOP
  kX86InstIdVpsadbw,         // AVX/AVX2
  kX86InstIdVpshab,          // XOP
  kX86InstIdVpshad,          // XOP
  kX86InstIdVpshaq,          // XOP
  kX86InstIdVpshaw,          // XOP
  kX86InstIdVpshlb,          // XOP
  kX86InstIdVpshld,          // XOP
  kX86InstIdVpshlq,          // XOP
  kX86InstIdVpshlw,          // XOP
  kX86InstIdVpshufb,         // AVX/AVX2
  kX86InstIdVpshufd,         // AVX/AVX2
  kX86InstIdVpshufhw,        // AVX/AVX2
  kX86InstIdVpshuflw,        // AVX/AVX2
  kX86InstIdVpsignb,         // AVX/AVX2
  kX86InstIdVpsignd,         // AVX/AVX2
  kX86InstIdVpsignw,         // AVX/AVX2
  kX86InstIdVpslld,          // AVX/AVX2
  kX86InstIdVpslldq,         // AVX/AVX2
  kX86InstIdVpsllq,          // AVX/AVX2
  kX86InstIdVpsllvd,         // AVX2
  kX86InstIdVpsllvq,         // AVX2
  kX86InstIdVpsllw,          // AVX/AVX2
  kX86InstIdVpsrad,          // AVX/AVX2
  kX86InstIdVpsravd,         // AVX2
  kX86InstIdVpsraw,          // AVX/AVX2
  kX86InstIdVpsrld,          // AVX/AVX2
  kX86InstIdVpsrldq,         // AVX/AVX2
  kX86InstIdVpsrlq,          // AVX/AVX2
  kX86InstIdVpsrlvd,         // AVX2
  kX86InstIdVpsrlvq,         // AVX2
  kX86InstIdVpsrlw,          // AVX/AVX2
  kX86InstIdVpsubb,          // AVX/AVX2
  kX86InstIdVpsubd,          // AVX/AVX2
  kX86InstIdVpsubq,          // AVX/AVX2
  kX86InstIdVpsubsb,         // AVX/AVX2
  kX86InstIdVpsubsw,         // AVX/AVX2
  kX86InstIdVpsubusb,        // AVX/AVX2
  kX86InstIdVpsubusw,        // AVX/AVX2
  kX86InstIdVpsubw,          // AVX/AVX2
  kX86InstIdVptest,          // AVX
  kX86InstIdVpunpckhbw,      // AVX/AVX2
  kX86InstIdVpunpckhdq,      // AVX/AVX2
  kX86InstIdVpunpckhqdq,     // AVX/AVX2
  kX86InstIdVpunpckhwd,      // AVX/AVX2
  kX86InstIdVpunpcklbw,      // AVX/AVX2
  kX86InstIdVpunpckldq,      // AVX/AVX2
  kX86InstIdVpunpcklqdq,     // AVX/AVX2
  kX86InstIdVpunpcklwd,      // AVX/AVX2
  kX86InstIdVpxor,           // AVX/AVX2
  kX86InstIdVrcpps,          // AVX
  kX86InstIdVrcpss,          // AVX
  kX86InstIdVroundpd,        // AVX
  kX86InstIdVroundps,        // AVX
  kX86InstIdVroundsd,        // AVX
  kX86InstIdVroundss,        // AVX
  kX86InstIdVrsqrtps,        // AVX
  kX86InstIdVrsqrtss,        // AVX
  kX86InstIdVshufpd,         // AVX
  kX86InstIdVshufps,         // AVX
  kX86InstIdVsqrtpd,         // AVX
  kX86InstIdVsqrtps,         // AVX
  kX86InstIdVsqrtsd,         // AVX
  kX86InstIdVsqrtss,         // AVX
  kX86InstIdVstmxcsr,        // AVX
  kX86InstIdVsubpd,          // AVX
  kX86InstIdVsubps,          // AVX
  kX86InstIdVsubsd,          // AVX
  kX86InstIdVsubss,          // AVX
  kX86InstIdVtestpd,         // AVX
  kX86InstIdVtestps,         // AVX
  kX86InstIdVucomisd,        // AVX
  kX86InstIdVucomiss,        // AVX
  kX86InstIdVunpckhpd,       // AVX
  kX86InstIdVunpckhps,       // AVX
  kX86InstIdVunpcklpd,       // AVX
  kX86InstIdVunpcklps,       // AVX
  kX86InstIdVxorpd,          // AVX
  kX86InstIdVxorps,          // AVX
  kX86InstIdVzeroall,        // AVX
  kX86InstIdVzeroupper,      // AVX
  kX86InstIdWrfsbase,        // FSGSBASE (x64)
  kX86InstIdWrgsbase,        // FSGSBASE (x64)
  kX86InstIdXadd,            // X86/X64 (i486+)
  kX86InstIdXchg,            // X86/X64
  kX86InstIdXgetbv,          // XSAVE
  kX86InstIdXor,             // X86/X64
  kX86InstIdXorpd,           // SSE2
  kX86InstIdXorps,           // SSE
  kX86InstIdXrstor,          // XSAVE
  kX86InstIdXrstor64,        // XSAVE
  kX86InstIdXsave,           // XSAVE
  kX86InstIdXsave64,         // XSAVE
  kX86InstIdXsaveopt,        // XSAVE
  kX86InstIdXsaveopt64,      // XSAVE
  kX86InstIdXsetbv,          // XSAVE

  _kX86InstIdCount,

  _kX86InstIdCmovcc = kX86InstIdCmova,
  _kX86InstIdJcc = kX86InstIdJa,
  _kX86InstIdSetcc = kX86InstIdSeta,

  _kX86InstIdJbegin = kX86InstIdJa,
  _kX86InstIdJend = kX86InstIdJmp
};

// ============================================================================
// [asmjit::kX86InstOptions]
// ============================================================================

//! X86/X64 instruction emit options, mainly for internal purposes.
ASMJIT_ENUM(kX86InstOptions) {
  //! Emit instruction with LOCK prefix.
  //!
  //! If this option is used and instruction doesn't support LOCK prefix an
  //! invalid instruction error is generated.
  kX86InstOptionLock = 0x00000010,

  //! Force REX prefix (X64).
  //!
  //! This option should be used carefully as there are combinations of
  //! instructions and their operands that are not encodable. The REX prefix
  //! can't be used together with AH, BH, CH, and DH registers. AsmJit reports
  //! \ref kErrorIllegalInstruction in such case.
  kX86InstOptionRex = 0x00000040,

  //! \internal
  //!
  //! Reserved by `X86Assembler`, do not use!
  _kX86InstOptionNoRex = 0x00000080,

  //! Force 3-byte VEX prefix even if the instruction is encodable by 2-byte
  //! VEX prefix (AVX).
  //!
  //! Ignored if the instruction is not AVX or `kX86InstOptionEVEX` is used.
  kX86InstOptionVex3 = 0x00000100,

  //! Force 4-byte EVEX prefix even if the instruction is encodable by using
  //! VEX prefix. Please note that all higher bits from `kX86InstOptionEvex`
  //! are reserved for EVEX and forces EVEX encoding to be used implicitly.
  kX86InstOptionEvex = 0x00010000,
  //! Use zeroing instead of merging (AVX512+).
  kX86InstOptionEvexZero = 0x00020000,
  //! Broadcast one element to all other elements (AVX512+).
  kX86InstOptionEvexOneN = 0x00040000,
  //! Suppress all exceptions (AVX512+).
  kX86InstOptionEvexSae = 0x00080000,

  //! Static rounding mode `round-to-nearest` (even) and `SAE` (AVX512+).
  kX86InstOptionEvexRnSae = 0x00100000,
  //! Static rounding mode `round-down` (toward -inf) and `SAE` (AVX512+).
  kX86InstOptionEvexRdSae = 0x00200000,
  //! Static rounding mode `round-up` (toward +inf) and `SAE` (AVX512+).
  kX86InstOptionEvexRuSae = 0x00400000,
  //! Static rounding mode `round-toward-zero` (truncate) and `SAE` (AVX512+).
  kX86InstOptionEvexRzSae = 0x00800000
};

// ============================================================================
// [asmjit::kX86InstEncodingId]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction groups.
//!
//! This group is specific to AsmJit and only used by `X86Assembler`.
ASMJIT_ENUM(kX86InstEncodingId) {
  //! Never used.
  kX86InstEncodingIdNone = 0,

  kX86InstEncodingIdX86Op,
  kX86InstEncodingIdX86Op_66H,
  kX86InstEncodingIdX86Rm,
  kX86InstEncodingIdX86Rm_B,
  kX86InstEncodingIdX86RmReg,
  kX86InstEncodingIdX86RegRm,
  kX86InstEncodingIdX86M,
  //! Adc/Add/And/Cmp/Or/Sbb/Sub/Xor.
  kX86InstEncodingIdX86Arith,
  //! Bswap.
  kX86InstEncodingIdX86BSwap,
  //! Bt/Btc/Btr/Bts.
  kX86InstEncodingIdX86BTest,
  //! Call.
  kX86InstEncodingIdX86Call,
  //! Enter.
  kX86InstEncodingIdX86Enter,
  //! Imul.
  kX86InstEncodingIdX86Imul,
  //! Inc/Dec.
  kX86InstEncodingIdX86IncDec,
  //! Int.
  kX86InstEncodingIdX86Int,
  //! Jcc.
  kX86InstEncodingIdX86Jcc,
  //! Jcxz/Jecxz/Jrcxz.
  kX86InstEncodingIdX86Jecxz,
  //! Jmp.
  kX86InstEncodingIdX86Jmp,
  //! Lea.
  kX86InstEncodingIdX86Lea,
  //! Mov.
  kX86InstEncodingIdX86Mov,
  //! Movsx/Movzx.
  kX86InstEncodingIdX86MovSxZx,
  //! Movsxd.
  kX86InstEncodingIdX86MovSxd,
  //! Mov having absolute memory operand (x86/x64).
  kX86InstEncodingIdX86MovPtr,
  //! Push.
  kX86InstEncodingIdX86Push,
  //! Pop.
  kX86InstEncodingIdX86Pop,
  //! Rep/Repe/Repne LodsX/MovsX/StosX/CmpsX/ScasX.
  kX86InstEncodingIdX86Rep,
  //! Ret.
  kX86InstEncodingIdX86Ret,
  //! Rcl/Rcr/Rol/Ror/Sal/Sar/Shl/Shr.
  kX86InstEncodingIdX86Rot,
  //! Setcc.
  kX86InstEncodingIdX86Set,
  //! Shld/Rhrd.
  kX86InstEncodingIdX86Shlrd,
  //! Test.
  kX86InstEncodingIdX86Test,
  //! Xadd.
  kX86InstEncodingIdX86Xadd,
  //! Xchg.
  kX86InstEncodingIdX86Xchg,

  //! Fincstp/Finit/FldX/Fnclex/Fninit/Fnop/Fpatan/Fprem/Fprem1/Fptan/Frndint/Fscale/Fsin/Fsincos/Fsqrt/Ftst/Fucompp/Fxam/Fxtract/Fyl2x/Fyl2xp1.
  kX86InstEncodingIdFpuOp,
  //! Fadd/Fdiv/Fdivr/Fmul/Fsub/Fsubr.
  kX86InstEncodingIdFpuArith,
  //! Fcom/Fcomp.
  kX86InstEncodingIdFpuCom,
  //! Fld/Fst/Fstp.
  kX86InstEncodingIdFpuFldFst,
  //! Fiadd/Ficom/Ficomp/Fidiv/Fidivr/Fild/Fimul/Fist/Fistp/Fisttp/Fisub/Fisubr.
  kX86InstEncodingIdFpuM,
  //! Fcmov/Fcomi/Fcomip/Ffree/Fucom/Fucomi/Fucomip/Fucomp/Fxch.
  kX86InstEncodingIdFpuR,
  //! Faddp/Fdivp/Fdivrp/Fmulp/Fsubp/Fsubrp.
  kX86InstEncodingIdFpuRDef,
  //! Fnstsw/Fstsw.
  kX86InstEncodingIdFpuStsw,

  //! Mm/Xmm instruction.
  kX86InstEncodingIdExtRm,
  //! Mm/Xmm instruction (propagates 66H if the instruction uses Xmm register).
  kX86InstEncodingIdExtRm_P,
  //! Mm/Xmm instruction (propagates REX.W if GPQ is used).
  kX86InstEncodingIdExtRm_Q,
  //! Mm/Xmm instruction (propagates 66H and REX.W).
  kX86InstEncodingIdExtRm_PQ,
  //! Mm/Xmm instruction having Rm/Ri encodings.
  kX86InstEncodingIdExtRmRi,
  //! Mm/Xmm instruction having Rm/Ri encodings (propagates 66H if the instruction uses Xmm register).
  kX86InstEncodingIdExtRmRi_P,
  //! Mm/Xmm instruction having Rmi encoding.
  kX86InstEncodingIdExtRmi,
  //! Mm/Xmm instruction having Rmi encoding (propagates 66H if the instruction uses Xmm register).
  kX86InstEncodingIdExtRmi_P,
  //! Crc32.
  kX86InstEncodingIdExtCrc,
  //! Pextrb/Pextrw/Pextrd/Pextrq/Extractps.
  kX86InstEncodingIdExtExtract,
  //! Lfence/Mfence/Sfence.
  kX86InstEncodingIdExtFence,
  //! Mov Mm/Xmm.
  //!
  //! 0x66 prefix must be set manually in opcodes.
  //!
  //! - Primary opcode is used for instructions in (X)Mm <- (X)Mm/X86Mem format,
  //! - Secondary opcode is used for instructions in (X)Mm/X86Mem <- (X)Mm format.
  kX86InstEncodingIdExtMov,
  //! Mov Mm/Xmm.
  kX86InstEncodingIdExtMovNoRexW,
  //! Movbe.
  kX86InstEncodingIdExtMovBe,
  //! Movd.
  kX86InstEncodingIdExtMovD,
  //! Movq.
  kX86InstEncodingIdExtMovQ,
  //! Prefetch.
  kX86InstEncodingIdExtPrefetch,

  //! Extrq (SSE4a).
  kX86InstEncodingIdExtExtrq,
  //! Insrq (SSE4a).
  kX86InstEncodingIdExtInsertq,

  //! 3dNow instruction.
  kX86InstEncodingId3dNow,

  //! AVX instruction without operands.
  kX86InstEncodingIdAvxOp,
  //! AVX instruction encoded as 'M'.
  kX86InstEncodingIdAvxM,
  //! AVX instruction encoded as 'MR'.
  kX86InstEncodingIdAvxMr,
  //! AVX instruction encoded as 'MR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxMr_P,
  //! AVX instruction encoded as 'MRI'.
  kX86InstEncodingIdAvxMri,
  //! AVX instruction encoded as 'MRI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxMri_P,
  //! AVX instruction encoded as 'RM'.
  kX86InstEncodingIdAvxRm,
  //! AVX instruction encoded as 'RM' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRm_P,
  //! AVX instruction encoded as 'RMI'.
  kX86InstEncodingIdAvxRmi,
  //! AVX instruction encoded as 'RMI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRmi_P,
  //! AVX instruction encoded as 'RVM'.
  kX86InstEncodingIdAvxRvm,
  //! AVX instruction encoded as 'RVM' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvm_P,
  //! AVX instruction encoded as 'RVMR'.
  kX86InstEncodingIdAvxRvmr,
  //! AVX instruction encoded as 'RVMR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvmr_P,
  //! AVX instruction encoded as 'RVMI'.
  kX86InstEncodingIdAvxRvmi,
  //! AVX instruction encoded as 'RVMI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvmi_P,
  //! AVX instruction encoded as 'RMV'.
  kX86InstEncodingIdAvxRmv,
  //! AVX instruction encoded as 'RMVI'.
  kX86InstEncodingIdAvxRmvi,
  //! AVX instruction encoded as 'RM' or 'MR'.
  kX86InstEncodingIdAvxRmMr,
  //! AVX instruction encoded as 'RM' or 'MR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRmMr_P,
  //! AVX instruction encoded as 'RVM' or 'RMI'.
  kX86InstEncodingIdAvxRvmRmi,
  //! AVX instruction encoded as 'RVM' or 'RMI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvmRmi_P,
  //! AVX instruction encoded as 'RVM' or 'MR'.
  kX86InstEncodingIdAvxRvmMr,
  //! AVX instruction encoded as 'RVM' or 'MVR'.
  kX86InstEncodingIdAvxRvmMvr,
  //! AVX instruction encoded as 'RVM' or 'MVR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvmMvr_P,
  //! AVX instruction encoded as 'RVM' or 'VMI'.
  kX86InstEncodingIdAvxRvmVmi,
  //! AVX instruction encoded as 'RVM' or 'VMI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvmVmi_P,
  //! AVX instruction encoded as 'VM'.
  kX86InstEncodingIdAvxVm,
  //! AVX instruction encoded as 'VMI'.
  kX86InstEncodingIdAvxVmi,
  //! AVX instruction encoded as 'VMI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxVmi_P,
  //! AVX instruction encoded as 'RVRM' or 'RVMR'.
  kX86InstEncodingIdAvxRvrmRvmr,
  //! AVX instruction encoded as 'RVRM' or 'RVMR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdAvxRvrmRvmr_P,
  //! Vmovss/Vmovsd.
  kX86InstEncodingIdAvxMovSsSd,
  //! AVX2 gather family instructions (VSIB).
  kX86InstEncodingIdAvxGather,
  //! AVX2 gather family instructions (VSIB), differs only in mem operand.
  kX86InstEncodingIdAvxGatherEx,

  //! FMA4 instruction in form [R, R, R/M, R/M].
  kX86InstEncodingIdFma4,
  //! FMA4 instruction in form [R, R, R/M, R/M] (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdFma4_P,

  //! XOP instruction encoded as 'RM'.
  kX86InstEncodingIdXopRm,
  //! XOP instruction encoded as 'RM' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdXopRm_P,
  //! XOP instruction encoded as 'RVM' or 'RMV'.
  kX86InstEncodingIdXopRvmRmv,
  //! XOP instruction encoded as 'RVM' or 'RMI'.
  kX86InstEncodingIdXopRvmRmi,
  //! XOP instruction encoded as 'RVMR'.
  kX86InstEncodingIdXopRvmr,
  //! XOP instruction encoded as 'RVMR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdXopRvmr_P,
  //! XOP instruction encoded as 'RVMI'.
  kX86InstEncodingIdXopRvmi,
  //! XOP instruction encoded as 'RVMI' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdXopRvmi_P,
  //! XOP instruction encoded as 'RVRM' or 'RVMR'.
  kX86InstEncodingIdXopRvrmRvmr,
  //! XOP instruction encoded as 'RVRM' or 'RVMR' (Propagates AVX.L if Ymm used).
  kX86InstEncodingIdXopRvrmRvmr_P,

  //! Count of X86 instruction groups.
  _kX86InstEncodingIdCount
};

// ============================================================================
// [asmjit::kX86InstOpCode]
// ============================================================================

//! \internal
//!
//! X86/X64 Instruction opcode encoding used by asmjit 'X86InstInfo' table.
//!
//! This schema is AsmJit specific and has been designed to allow encoding of
//! all X86 instructions available. X86, MMX, and SSE+ instructions always use
//! `MMMMM` and `PP` fields, which are encoded to corresponding prefixes needed
//! by X86 or SIMD instructions. AVX+ instructions embed `MMMMM` and `PP` fields
//! in a VEX prefix.
//!
//! The instruction opcode definition uses 1 or 2 bytes as an opcode value. 1
//! byte is needed by most of the instructions, 2 bytes are only used by legacy
//! X87-FPU instructions. This means that a second byte is free to by used by
//! AVX and AVX-512 instructions.
//!
//! The fields description:
//!
//! - `MMMMM` field is used to encode prefixes needed by the instruction or as
//!   a part of VEX/EVEX prefix.
//!
//! - `PP` field is used to encode prefixes needed by the instruction or as a
//!   part of VEX/EVEX prefix.
//!
//! - `L` field is used exclusively by AVX+ and AVX512+ instruction sets. It
//!   describes vector size, which is 128-bit for Xmm register `L_128`, 256
//!   for Ymm register `L_256` and 512-bit for Zmm register `L_512`. The `L`
//!   field is omitted in case that instruction supports multiple vector lengths,
//!   however, if the instruction requires specific `L` value it's specified as
//!   a part of the opcode.
//!
//! - `W` field is the most complicated. It was added by 64-bit architecture
//!   to promote default operation width (instructions that perform 32-bit
//!   operation by default require to override the width to 64-bit explicitly).
//!   There is nothing wrong on this, however, some instructions introduced
//!   implicit `W` override, for example a `cdqe` instruction is basically a
//!   `cwde` instructiontion with overridden `W` (set to 1). There are some
//!   others in the base X86 instruction set. More recent instruction sets
//!   started using `W` field more often:
//!
//!   - AVX instructions started using `W` field as an extended opcode for FMA,
//!     GATHER, PERM, and other instructions. It also uses `W` field to override
//!     the default operation width in instructions like `vmovq`. AVX `W` field
//!     is
//!
//!   - AVX-512 instructions started using `W` field as an extended opcode for
//!     all new instructions. This wouldn't have been an issue if the `W` field
//!     of AVX-512 have matched AVX, but this is not the case.
//!
//! - `O` field is an extended opcode field (3) bytes used by ModR/M BYTE.
ASMJIT_ENUM(kX86InstOpCode) {
  // `MMMMM` field in AVX/XOP/AVX-512 instruction (5 bits).
  //
  // `OpCode` leading bytes in legacy encoding.
  //
  // AVX reserves 5 bits for `MMMMM` field, however AVX instructions only use
  // 2 bits and XOP 4 bits. AVX-512 shrinks `MMMMM` field into `MM` so it's
  // safe to assume that `MM` field won't grow in the future as EVEX doesn't
  // use more than 2 bits. There is always a way how a fifth bit can be stored
  // if needed.
  kX86InstOpCode_MM_Shift  = 16,
  kX86InstOpCode_MM_Mask   = 0x0FU << kX86InstOpCode_MM_Shift,
  kX86InstOpCode_MM_00     = 0x00U << kX86InstOpCode_MM_Shift,
  kX86InstOpCode_MM_0F     = 0x01U << kX86InstOpCode_MM_Shift,
  kX86InstOpCode_MM_0F38   = 0x02U << kX86InstOpCode_MM_Shift,
  kX86InstOpCode_MM_0F3A   = 0x03U << kX86InstOpCode_MM_Shift,
  kX86InstOpCode_MM_00011  = 0x03U << kX86InstOpCode_MM_Shift, // XOP.
  kX86InstOpCode_MM_01000  = 0x08U << kX86InstOpCode_MM_Shift, // XOP.
  kX86InstOpCode_MM_01001  = 0x09U << kX86InstOpCode_MM_Shift, // XOP.
  kX86InstOpCode_MM_0F01   = 0x0FU << kX86InstOpCode_MM_Shift, // AsmJit specific, not part of AVX.

  // `PP` field in AVX/XOP/AVX-512 instruction.
  //
  // `Mandatory Prefix` in legacy encoding.
  //
  // AVX reserves 2 bits for `PP` field, but AsmJit extends the storage by 1
  // more bit that is used to emit 9B prefix for some X87-FPU instructions.
  kX86InstOpCode_PP_Shift  = 20,
  kX86InstOpCode_PP_Mask   = 0x07U << kX86InstOpCode_PP_Shift,
  kX86InstOpCode_PP_00     = 0x00U << kX86InstOpCode_PP_Shift,
  kX86InstOpCode_PP_66     = 0x01U << kX86InstOpCode_PP_Shift,
  kX86InstOpCode_PP_F3     = 0x02U << kX86InstOpCode_PP_Shift,
  kX86InstOpCode_PP_F2     = 0x03U << kX86InstOpCode_PP_Shift,
  kX86InstOpCode_PP_9B     = 0x07U << kX86InstOpCode_PP_Shift, // AsmJit specific, not part of AVX.

  // `L` field in AVX/XOP/AVX-512 instruction.
  //
  // AVX/XOP can only use the first bit `L.128` or `L.256`. AVX-512 makes it
  // possible to use also `L.512`.
  //
  // \note If the instruction set manual describes an instruction by using `LIG`
  // it means that the `L` field is ignored. AsmJit emits `0` in such case.
  kX86InstOpCode_L_Shift   = 23,
  kX86InstOpCode_L_Mask    = 0x03U << kX86InstOpCode_L_Shift,
  kX86InstOpCode_L_128     = 0x00U << kX86InstOpCode_L_Shift,
  kX86InstOpCode_L_256     = 0x01U << kX86InstOpCode_L_Shift,
  kX86InstOpCode_L_512     = 0x02U << kX86InstOpCode_L_Shift,

  // `O` field (ModR/M).
  kX86InstOpCode_O_Shift   = 25,
  kX86InstOpCode_O_Mask    = 0x07U << kX86InstOpCode_O_Shift,

  // `W` field used in EVEX instruction encoding.
  kX86InstOpCode_EW_Shift  = 30,
  kX86InstOpCode_EW_Mask   = 0x01U << kX86InstOpCode_EW_Shift,
  kX86InstOpCode_EW        = 0x01U << kX86InstOpCode_EW_Shift,

  // `W` field used in REX/VEX instruction encoding.
  //
  // \note If the instruction set manual describes an instruction by using `WIG`
  // it means that the `W` field is ignored. AsmJit emits `0` in such case.
  kX86InstOpCode_W_Shift   = 31,
  kX86InstOpCode_W_Mask    = 0x01U << kX86InstOpCode_W_Shift,
  kX86InstOpCode_W         = 0x01U << kX86InstOpCode_W_Shift,
};

// ============================================================================
// [asmjit::kX86InstFlags]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction flags.
ASMJIT_ENUM(kX86InstFlags) {
  //! No flags.
  kX86InstFlagNone = 0x00000000,

  //! Instruction is a control-flow instruction.
  //!
  //! Control flow instructions are jmp, jcc, call and ret.
  kX86InstFlagFlow = 0x00000001,

  //! Instruction is a compare/test like instruction.
  kX86InstFlagTest = 0x00000002,

  //! Instruction is a move like instruction.
  //!
  //! Move instructions typically overwrite the first operand by the second
  //! operand. The first operand can be the exact copy of the second operand
  //! or it can be any kind of conversion or shuffling.
  //!
  //! Mov instructions are 'mov', 'movd', 'movq', movdq', 'lea', multimedia
  //! instructions like 'cvtdq2pd', shuffle instructions like 'pshufb' and
  //! SSE/SSE2 mathematic instructions like 'rcp?', 'round?' and 'rsqrt?'.
  //!
  //! There are some MOV instructions that do only a partial move (for example
  //! 'cvtsi2ss'), register allocator has to know the variable size and use
  //! the flag accordingly to it.
  kX86InstFlagMove = 0x00000004,

  //! Instruction is an exchange like instruction.
  //!
  //! Exchange instruction typically overwrite first and second operand. So
  //! far only the instructions 'xchg' and 'xadd' are considered.
  kX86InstFlagXchg = 0x00000008,

  //! Instruction accesses Fp register(s).
  kX86InstFlagFp = 0x00000010,

  //! Instruction can be prefixed by using the LOCK prefix.
  kX86InstFlagLock = 0x00000020,

  //! Instruction requires special handling, used by \ref Compiler.
  kX86InstFlagSpecial = 0x00000040,

  //! Instruction always performs memory access.
  //!
  //! This flag is always combined with `kX86InstFlagSpecial` and describes
  //! that there is an implicit address which is accessed (usually EDI/RDI or
  //! ESI/EDI).
  kX86InstFlagSpecialMem = 0x00000080,

  //! Instruction memory operand can refer to 16-bit address (used by FPU).
  kX86InstFlagMem2 = 0x00000100,
  //! Instruction memory operand can refer to 32-bit address (used by FPU).
  kX86InstFlagMem4 = 0x00000200,
  //! Instruction memory operand can refer to 64-bit address (used by FPU).
  kX86InstFlagMem8 = 0x00000400,
  //! Instruction memory operand can refer to 80-bit address (used by FPU).
  kX86InstFlagMem10 = 0x00000800,

  //! Zeroes the rest of the register if the source operand is memory.
  //!
  //! Special behavior related to some SIMD load instructions.
  kX86InstFlagZ = 0x00001000,

  //! Instruction is supported by AVX.
  kX86InstFlagAvx = 0x00010000,
  //! Instruction is supported by XOP.
  kX86InstFlagXop = 0x00020000,

  //! Instruction is supported by AVX-512 F (Zmm).
  kX86InstFlagAvx512F  = 0x00100000,
  //! Instruction is supported by AVX-512 CD (Zmm).
  kX86InstFlagAvx512CD = 0x00200000,
  //! Instruction is supported by AVX-512 PF (Zmm).
  kX86InstFlagAvx512PF = 0x00400000,
  //! Instruction is supported by AVX-512 ER (Zmm).
  kX86InstFlagAvx512ER = 0x00800000,
  //! Instruction is supported by AVX-512 DQ (Zmm).
  kX86InstFlagAvx512DQ = 0x01000000,
  //! Instruction is supported by AVX-512 BW (Zmm).
  kX86InstFlagAvx512BW = 0x02000000,
  //! Instruction is supported by AVX-512 VL (Xmm/Ymm).
  kX86InstFlagAvx512VL = 0x04000000,

  //! Instruction supports masking {k0..k7}.
  kX86InstFlagAvx512KMask = 0x08000000,
  //! Instruction supports zeroing of elements {k0z..k7z}.
  kX86InstFlagAvx512KZero = 0x10000000,
  //! Instruction supports broadcast {1toN}.
  kX86InstFlagAvx512Broadcast  = 0x20000000,
  //! Instruction supports suppressing all exceptions {sae}.
  kX86InstFlagAvx512Sae = 0x40000000,
  //! Instruction supports static rounding control with SAE {rnd-sae},
  kX86InstFlagAvx512Rnd = 0x80000000
};

// ============================================================================
// [asmjit::kX86InstOp]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction operand flags.
ASMJIT_ENUM(kX86InstOp) {
  //! Instruction operand can be 8-bit Gpb register.
  kX86InstOpGb = 0x0001,
  //! Instruction operand can be 16-bit Gpw register.
  kX86InstOpGw = 0x0002,
  //! Instruction operand can be 32-bit Gpd register.
  kX86InstOpGd = 0x0004,
  //! Instruction operand can be 64-bit Gpq register.
  kX86InstOpGq = 0x0008,

  //! Instruction operand can be Fp register.
  kX86InstOpFp = 0x0010,
  //! Instruction operand can be 64-bit Mm register.
  kX86InstOpMm = 0x0020,

  //! Instruction operand can be 64-bit K register.
  kX86InstOpK = 0x0040,

  //! Instruction operand can be 128-bit Xmm register.
  kX86InstOpXmm = 0x0100,
  //! Instruction operand can be 256-bit Ymm register.
  kX86InstOpYmm = 0x0200,
  //! Instruction operand can be 512-bit Zmm register.
  kX86InstOpZmm = 0x0400,

  //! Instruction operand can be memory.
  kX86InstOpMem = 0x1000,
  //! Instruction operand can be immediate.
  kX86InstOpImm = 0x2000,
  //! Instruction operand can be label.
  kX86InstOpLabel = 0x4000,
  //! Instruction operand doesn't have to be used.
  //!
  //! \note If no operand is specified the meaning is clear (the operand at the
  //! particular index doesn't exist), however, when one or more operand is
  //! specified, it's not clear whether the operand can be omitted or not. When
  //! `kX86InstOpNone` is used it means that the operand is not used in some
  //! cases.
  kX86InstOpNone = 0x8000
};

// ============================================================================
// [asmjit::kX86Cond]
// ============================================================================

//! X86/X64 Condition codes.
ASMJIT_ENUM(kX86Cond) {
  kX86CondA               = 0x07, // CF==0 & ZF==0          (unsigned)
  kX86CondAE              = 0x03, // CF==0                  (unsigned)
  kX86CondB               = 0x02, // CF==1                  (unsigned)
  kX86CondBE              = 0x06, // CF==1 | ZF==1          (unsigned)
  kX86CondC               = 0x02, // CF==1
  kX86CondE               = 0x04, //         ZF==1          (signed/unsigned)
  kX86CondG               = 0x0F, //         ZF==0 & SF==OF (signed)
  kX86CondGE              = 0x0D, //                 SF==OF (signed)
  kX86CondL               = 0x0C, //                 SF!=OF (signed)
  kX86CondLE              = 0x0E, //         ZF==1 | SF!=OF (signed)
  kX86CondNA              = 0x06, // CF==1 | ZF==1          (unsigned)
  kX86CondNAE             = 0x02, // CF==1                  (unsigned)
  kX86CondNB              = 0x03, // CF==0                  (unsigned)
  kX86CondNBE             = 0x07, // CF==0 & ZF==0          (unsigned)
  kX86CondNC              = 0x03, // CF==0
  kX86CondNE              = 0x05, //         ZF==0          (signed/unsigned)
  kX86CondNG              = 0x0E, //         ZF==1 | SF!=OF (signed)
  kX86CondNGE             = 0x0C, //                 SF!=OF (signed)
  kX86CondNL              = 0x0D, //                 SF==OF (signed)
  kX86CondNLE             = 0x0F, //         ZF==0 & SF==OF (signed)
  kX86CondNO              = 0x01, //                 OF==0
  kX86CondNP              = 0x0B, // PF==0
  kX86CondNS              = 0x09, //                 SF==0
  kX86CondNZ              = 0x05, //         ZF==0
  kX86CondO               = 0x00, //                 OF==1
  kX86CondP               = 0x0A, // PF==1
  kX86CondPE              = 0x0A, // PF==1
  kX86CondPO              = 0x0B, // PF==0
  kX86CondS               = 0x08, //                 SF==1
  kX86CondZ               = 0x04, //         ZF==1

  // Simplified condition codes.
  kX86CondSign            = kX86CondS , //!< Sign (S).
  kX86CondNotSign         = kX86CondNS, //!< Not Sign (NS).

  kX86CondOverflow        = kX86CondO , //!< Signed  Overflow (O)
  kX86CondNotOverflow     = kX86CondNO, //!< Not Signed Overflow (NO)

  kX86CondLess            = kX86CondL , //!< Signed     `a <  b` (L  or NGE).
  kX86CondLessEqual       = kX86CondLE, //!< Signed     `a <= b` (LE or NG ).
  kX86CondGreater         = kX86CondG , //!< Signed     `a >  b` (G  or NLE).
  kX86CondGreaterEqual    = kX86CondGE, //!< Signed     `a >= b` (GE or NL ).
  kX86CondBelow           = kX86CondB , //!< Unsigned   `a <  b` (B  or NAE).
  kX86CondBelowEqual      = kX86CondBE, //!< Unsigned   `a <= b` (BE or NA ).
  kX86CondAbove           = kX86CondA , //!< Unsigned   `a >  b` (A  or NBE).
  kX86CondAboveEqual      = kX86CondAE, //!< Unsigned   `a >= b` (AE or NB ).
  kX86CondEqual           = kX86CondE , //!< Equal      `a == b` (E  or Z  ).
  kX86CondNotEqual        = kX86CondNE, //!< Not Equal  `a != b` (NE or NZ ).

  kX86CondParityEven      = kX86CondP,
  kX86CondParityOdd       = kX86CondPO,

  // Aliases.
  kX86CondZero            = kX86CondZ,
  kX86CondNotZero         = kX86CondNZ,
  kX86CondNegative        = kX86CondS,
  kX86CondPositive        = kX86CondNS,

  // Fpu-only.
  kX86CondFpuUnordered    = 0x10,
  kX86CondFpuNotUnordered = 0x11,

  //! No condition code.
  kX86CondNone            = 0x12
};

// ============================================================================
// [asmjit::kX86EFlags]
// ============================================================================

//! X86/X64 EFLAGs bits (AsmJit specific).
//!
//! Each instruction stored in AsmJit database contains flags that instruction
//! uses (reads) and flags that instruction modifies (writes). This is used by
//! instruction reordering, but can be used by third parties as it's part of
//! AsmJit API.
//!
//! \note Flags defined here don't correspond to real flags used by X86/X64
//! architecture, defined in Intel's Manual Section `3.4.3 - EFLAGS Register`.
//!
//! \note Flags are designed to fit in an 8-bit integer.
ASMJIT_ENUM(kX86EFlags) {
  // --------------------------------------------------------------------------
  // src-gendefs.js relies on the values of these masks, the tool has to be
  // changed as you plan to modify `kX86EFlags`.
  // --------------------------------------------------------------------------

  //! Overflow flag (OF).
  //!
  //! Set if the integer result is too large a positive number or too small a
  //! negative number (excluding the sign-bit) to fit in the destination
  //! operand; cleared otherwise. This flag indicates an overflow condition for
  //! signed-integer arithmetic.
  kX86EFlagO = 0x01,

  //! Sign flag (SF).
  //!
  //! Set equal to the most-significant bit of the result, which is the sign
  //! bit of a signed integer (0 == positive value, 1 == negative value).
  kX86EFlagS = 0x02,

  //! Zero flag (ZF).
  //!
  //! Set if the result is zero; cleared otherwise.
  kX86EFlagZ = 0x04,

  //! Adjust flag (AF).
  //!
  //! Set if an arithmetic operation generates a carry or a borrow out of bit
  //! 3 of the result; cleared otherwise. This flag is used in binary-coded
  //! decimal (BCD) arithmetic.
  kX86EFlagA = 0x08,

  //! Parity flag (PF).
  //!
  //! Set if the least-significant byte of the result contains an even number
  //! of 1 bits; cleared otherwise.
  kX86EFlagP = 0x10,

  //! Carry flag (CF).
  //!
  //! Set if an arithmetic operation generates a carry or a borrow out of the
  //! mostsignificant bit of the result; cleared otherwise.
  kX86EFlagC = 0x20,

  //! Direction flag (DF).
  //!
  //! The direction flag controls string instructions `movs`, `cmps`, `scas,
  //! `lods` and `stos`.
  kX86EFlagD = 0x40,

  //! Any other flag that AsmJit doesn't use to keep track of.
  kX86EFlagX = 0x80
};

// ============================================================================
// [asmjit::kX86FpSw]
// ============================================================================

//! X86/X64 FPU status word.
ASMJIT_ENUM(kX86FpSw) {
  kX86FpSw_Invalid        = 0x0001,
  kX86FpSw_Denormalized   = 0x0002,
  kX86FpSw_DivByZero      = 0x0004,
  kX86FpSw_Overflow       = 0x0008,
  kX86FpSw_Underflow      = 0x0010,
  kX86FpSw_Precision      = 0x0020,
  kX86FpSw_StackFault     = 0x0040,
  kX86FpSw_Interrupt      = 0x0080,
  kX86FpSw_C0             = 0x0100,
  kX86FpSw_C1             = 0x0200,
  kX86FpSw_C2             = 0x0400,
  kX86FpSw_Top            = 0x3800,
  kX86FpSw_C3             = 0x4000,
  kX86FpSw_Busy           = 0x8000
};

// ============================================================================
// [asmjit::kX86FpCw]
// ============================================================================

//! X86/X64 FPU control word.
ASMJIT_ENUM(kX86FpCw) {
  kX86FpCw_EM_Mask        = 0x003F, // Bits 0-5.
  kX86FpCw_EM_Invalid     = 0x0001,
  kX86FpCw_EM_Denormal    = 0x0002,
  kX86FpCw_EM_DivByZero   = 0x0004,
  kX86FpCw_EM_Overflow    = 0x0008,
  kX86FpCw_EM_Underflow   = 0x0010,
  kX86FpCw_EM_Inexact     = 0x0020,

  kX86FpCw_PC_Mask        = 0x0300, // Bits 8-9.
  kX86FpCw_PC_Float       = 0x0000,
  kX86FpCw_PC_Reserved    = 0x0100,
  kX86FpCw_PC_Double      = 0x0200,
  kX86FpCw_PC_Extended    = 0x0300,

  kX86FpCw_RC_Mask        = 0x0C00, // Bits 10-11.
  kX86FpCw_RC_Nearest     = 0x0000,
  kX86FpCw_RC_Down        = 0x0400,
  kX86FpCw_RC_Up          = 0x0800,
  kX86FpCw_RC_Truncate    = 0x0C00,

  kX86FpCw_IC_Mask        = 0x1000, // Bit 12.
  kX86FpCw_IC_Projective  = 0x0000,
  kX86FpCw_IC_Affine      = 0x1000
};

// ============================================================================
// [asmjit::kX86Cmp]
// ============================================================================

//! X86/X64 Comparison predicate used by CMP[PD/PS/SD/SS] family instructions.
ASMJIT_ENUM(kX86Cmp) {
  kX86CmpEQ        = 0x00, //!< Equal             (Quite).
  kX86CmpLT        = 0x01, //!< Less              (Signaling).
  kX86CmpLE        = 0x02, //!< Less/Equal        (Signaling).
  kX86CmpUNORD     = 0x03, //!< Unordered         (Quite).
  kX86CmpNEQ       = 0x04, //!< Not Equal         (Quite).
  kX86CmpNLT       = 0x05, //!< Not Less          (Signaling).
  kX86CmpNLE       = 0x06, //!< Not Less/Equal    (Signaling).
  kX86CmpORD       = 0x07  //!< Ordered           (Quite).
};

// ============================================================================
// [asmjit::kX86VCmp]
// ============================================================================

//! X86/X64 Comparison predicate used by VCMP[PD/PS/SD/SS] family instructions.
//!
//! The first 8 are compatible with \ref kX86Cmp.
ASMJIT_ENUM(kX86VCmp) {
  kX86VCmpEQ_OQ    = 0x00, //!< Equal             (Quite, Ordered).
  kX86VCmpLT_OS    = 0x01, //!< Less              (Signaling, Ordered).
  kX86VCmpLE_OS    = 0x02, //!< Less/Equal        (Signaling, Ordered).
  kX86VCmpUNORD_Q  = 0x03, //!< Unordered         (Quite).
  kX86VCmpNEQ_UQ   = 0x04, //!< Not Equal         (Quite, Unordered).
  kX86VCmpNLT_US   = 0x05, //!< Not Less          (Signaling, Unordered).
  kX86VCmpNLE_US   = 0x06, //!< Not Less/Equal    (Signaling, Unordered).
  kX86VCmpORD_Q    = 0x07, //!< Ordered           (Quite).

  kX86VCmpEQ_UQ    = 0x08, //!< Equal             (Quite, Unordered).
  kX86VCmpNGE_US   = 0x09, //!< Not Greater/Equal (Signaling, Unordered).
  kX86VCmpNGT_US   = 0x0A, //!< Not Greater       (Signaling, Unordered).
  kX86VCmpFALSE_OQ = 0x0B, //!< False             (Quite, Ordered).
  kX86VCmpNEQ_OQ   = 0x0C, //!< Not Equal         (Quite, Ordered).
  kX86VCmpGE_OS    = 0x0D, //!< Greater/Equal     (Signaling, Ordered).
  kX86VCmpGT_OS    = 0x0E, //!< Greater           (Signaling, Ordered).
  kX86VCmpTRUE_UQ  = 0x0F, //!< True              (Quite, Unordered).
  kX86VCmpEQ_OS    = 0x10, //!< Equal             (Signaling, Ordered).
  kX86VCmpLT_OQ    = 0x11, //!< Less              (Quite, Ordered).
  kX86VCmpLE_OQ    = 0x12, //!< Less/Equal        (Quite, Ordered).
  kX86VCmpUNORD_S  = 0x13, //!< Unordered         (Signaling).
  kX86VCmpNEQ_US   = 0x14, //!< Not Equal         (Signaling, Unordered).
  kX86VCmpNLT_UQ   = 0x15, //!< Not Less          (Quite, Unordered).
  kX86VCmpNLE_UQ   = 0x16, //!< Not Less/Equal    (Quite, Unordered).
  kX86VCmpORD_S    = 0x17, //!< Ordered           (Signaling).
  kX86VCmpEQ_US    = 0x18, //!< Equal             (Signaling, Unordered).
  kX86VCmpNGE_UQ   = 0x19, //!< Not Greater/Equal (Quite, Unordered).
  kX86VCmpNGT_UQ   = 0x1A, //!< Not Greater       (Quite, Unordered).
  kX86VCmpFALSE_OS = 0x1B, //!< False             (Signaling, Ordered).
  kX86VCmpNEQ_OS   = 0x1C, //!< Not Equal         (Signaling, Ordered).
  kX86VCmpGE_OQ    = 0x1D, //!< Greater/Equal     (Quite, Ordered).
  kX86VCmpGT_OQ    = 0x1E, //!< Greater           (Quite, Ordered).
  kX86VCmpTRUE_US  = 0x1F  //!< True              (Signaling, Unordered).
};

// ============================================================================
// [asmjit::kX86Prefetch]
// ============================================================================

//! X86/X64 Prefetch hints.
ASMJIT_ENUM(kX86Prefetch) {
  //! Prefetch using NT hint.
  kX86PrefetchNTA = 0,
  //! Prefetch to L0 cache.
  kX86PrefetchT0 = 1,
  //! Prefetch to L1 cache.
  kX86PrefetchT1 = 2,
  //! Prefetch to L2 cache.
  kX86PrefetchT2 = 3
};

// ============================================================================
// [asmjit::X86InstExtendedInfo]
// ============================================================================

//! X86/X64 instruction extended information.
//!
//! Extended information has been introduced to minimize data needed for a
//! single instruction, because two or more instructions can share the common
//! data, for example operands definition or secondary opcode, which is only
//! used by few instructions.
struct X86InstExtendedInfo {
  // --------------------------------------------------------------------------
  // [Accessors - Instruction Encoding]
  // --------------------------------------------------------------------------

  //! Get instruction encoding, see \ref kX86InstEncoding.
  ASMJIT_INLINE uint32_t getEncodingId() const {
    return _encodingId;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Instruction Flags]
  // --------------------------------------------------------------------------

  //! Get whether the instruction has a `flag`, see `kX86InstFlags`.
  ASMJIT_INLINE bool hasInstFlag(uint32_t flag) const {
    return (_instFlags & flag) != 0;
  }

  //! Get all instruction flags, see `kX86InstFlags`.
  ASMJIT_INLINE uint32_t getInstFlags() const {
    return _instFlags;
  }

  //! Get whether the instruction is a control-flow intruction.
  //!
  //! Control flow instruction is instruction that can perform a branch,
  //! typically `jmp`, `jcc`, `call`, or `ret`.
  ASMJIT_INLINE bool isFlow() const {
    return (getInstFlags() & kX86InstFlagFlow) != 0;
  }

  //! Get whether the instruction is a compare/test like intruction.
  ASMJIT_INLINE bool isTest() const {
    return (getInstFlags() & kX86InstFlagTest) != 0;
  }

  //! Get whether the instruction is a typical move instruction.
  //!
  //! Move instructions overwrite the first operand or at least part of it,
  //! This is a very useful hint that is used by variable liveness analysis
  //! and `Compiler` in general to know which variable is completely
  //! overwritten.
  //!
  //! All AVX/XOP instructions that have 3 or more operands are considered to
  //! have move semantics move by default.
  ASMJIT_INLINE bool isMove() const {
    return (getInstFlags() & kX86InstFlagMove) != 0;
  }

  //! Get whether the instruction is a typical Exchange instruction.
  //!
  //! Exchange instructios are 'xchg' and 'xadd'.
  ASMJIT_INLINE bool isXchg() const {
    return (getInstFlags() & kX86InstFlagXchg) != 0;
  }

  //! Get whether the instruction accesses Fp register(s).
  ASMJIT_INLINE bool isFp() const {
    return (getInstFlags() & kX86InstFlagFp) != 0;
  }

  //! Get whether the instruction can be prefixed by LOCK prefix.
  ASMJIT_INLINE bool isLockable() const {
    return (getInstFlags() & kX86InstFlagLock) != 0;
  }

  //! Get whether the instruction is special type (this is used by `Compiler`
  //! to manage additional variables or functionality).
  ASMJIT_INLINE bool isSpecial() const {
    return (getInstFlags() & kX86InstFlagSpecial) != 0;
  }

  //! Get whether the instruction is special type and it performs memory access.
  ASMJIT_INLINE bool isSpecialMem() const {
    return (getInstFlags() & kX86InstFlagSpecialMem) != 0;
  }

  //! Get whether the move instruction zeroes the rest of the register
  //! if the source is memory operand.
  //!
  //! Basically flag needed only to support `movsd` and `movss` instructions.
  ASMJIT_INLINE bool isZeroIfMem() const {
    return (getInstFlags() & kX86InstFlagZ) != 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors - EFlags]
  // --------------------------------------------------------------------------

  //! Get EFLAGS that the instruction reads, see \ref kX86EFlags.
  ASMJIT_INLINE uint32_t getEFlagsIn() const {
    return _eflagsIn;
  }

  //! Get EFLAGS that the instruction writes, see \ref kX86EFlags.
  ASMJIT_INLINE uint32_t getEFlagsOut() const {
    return _eflagsOut;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Write Index/Size]
  // --------------------------------------------------------------------------

  //! Get the destination index of WRITE operation.
  ASMJIT_INLINE uint32_t getWriteIndex() const {
    return _writeIndex;
  }

  //! Get the number of bytes that will be written by a WRITE operation.
  ASMJIT_INLINE uint32_t getWriteSize() const {
    return _writeSize;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Operand-Flags]
  // --------------------------------------------------------------------------

  //! Get flags of operand at index `index`.
  //!
  //! See \ref X86InstInfo::getOperandFlags() for more details.
  ASMJIT_INLINE uint16_t getOperandFlags(uint32_t index) const {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_opFlags));
    return _opFlags[index];
  }

  // --------------------------------------------------------------------------
  // [Accessors - OpCode]
  // --------------------------------------------------------------------------

  //! Get the secondary instruction opcode, see \ref kX86InstOpCode.
  //!
  //! See \ref X86InstInfo::getSecondaryOpCode() for more details.
  ASMJIT_INLINE uint32_t getSecondaryOpCode() const {
    return _secondaryOpCode;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Instruction encoding ID.
  uint8_t _encodingId;

  //! Destination index of WRITE operation, default 0.
  uint8_t _writeIndex;

  //! Count of bytes affected by a write operation, needed by analysis for all
  //! instructions that do not read the register overwritten. Only used with
  //! `kX86InstFlagMove` flag. If `_writeSize` is zero it is automatically
  //! deduced from the size of the destination register.
  //!
  //! In general most of SSE write-only instructions should use 16 bytes as
  //! this is the size of the register (and of Ymm/Zmm registers). This means
  //! that 16-bytes of the register are changed, the rest remains unchanged.
  //! However, AVX instructions should use the size of Zmm register as every
  //! AVX instruction zeroes the rest of the register (AVX/AVX2 instructions
  //! zero the HI part of Zmm if available).
  uint8_t _writeSize;

  //! EFlags read by the instruction.
  uint8_t _eflagsIn;
  //! EFlags written by the instruction.
  uint8_t _eflagsOut;

  //! \internal
  uint8_t _reserved;

  //! Operands' flags, up to 5 operands.
  uint16_t _opFlags[5];

  //! Instruction flags.
  uint32_t _instFlags;

  //! Secondary opcode.
  uint32_t _secondaryOpCode;
};

// ============================================================================
// [asmjit::X86InstInfo]
// ============================================================================

//! X86/X64 instruction information.
struct X86InstInfo {
  // --------------------------------------------------------------------------
  // [Accessors - Instruction Name]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_DISABLE_NAMES)
  //! Get instruction name string (null terminated).
  ASMJIT_INLINE const char* getInstName() const {
    return _x86InstName + static_cast<uint32_t>(_nameIndex);
  }

  //! Get instruction name index to `_x86InstName` array.
  ASMJIT_INLINE uint32_t _getNameIndex() const {
    return _nameIndex;
  }
#endif // !ASMJIT_DISABLE_NAMES

  // --------------------------------------------------------------------------
  // [Accessors - Extended-Info]
  // --------------------------------------------------------------------------

  //! Get `X86InstExtendedInfo` for this instruction.
  ASMJIT_INLINE const X86InstExtendedInfo& getExtendedInfo() const {
    return _x86InstExtendedInfo[_extendedIndex];
  }

  //! Get index to the `_x86InstExtendedInfo` table.
  ASMJIT_INLINE uint32_t _getExtendedIndex() const {
    return _extendedIndex;
  }

  // --------------------------------------------------------------------------
  // [Accessors - Instruction Encoding]
  // --------------------------------------------------------------------------

  //! Get instruction group, see \ref kX86InstEncodingId.
  ASMJIT_INLINE uint32_t getEncodingId() const {
    return getExtendedInfo().getEncodingId();
  }

  // --------------------------------------------------------------------------
  // [Accessors - Instruction Flags]
  // --------------------------------------------------------------------------

  //! Get instruction flags, see `kX86InstFlags`.
  ASMJIT_INLINE uint32_t getInstFlags() const {
    return getExtendedInfo().getInstFlags();
  }

  //! Get whether the instruction has flag `flag`, see `kX86InstFlags`.
  ASMJIT_INLINE bool hasInstFlag(uint32_t flag) const {
    return (getInstFlags() & flag) != 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors - EFlags]
  // --------------------------------------------------------------------------

  //! Get EFLAGS that the instruction reads, see \ref kX86EFlags.
  ASMJIT_INLINE uint32_t getEFlagsIn() const {
    return getExtendedInfo().getEFlagsIn();
  }

  //! Get EFLAGS that the instruction writes, see \ref kX86EFlags.
  ASMJIT_INLINE uint32_t getEFlagsOut() const {
    return getExtendedInfo().getEFlagsOut();
  }

  // --------------------------------------------------------------------------
  // [Accessors - Write Index/Size]
  // --------------------------------------------------------------------------

  //! Get the destination index of WRITE operation.
  ASMJIT_INLINE uint32_t getWriteIndex() const {
    return getExtendedInfo().getWriteIndex();
  }

  //! Get the number of bytes that will be written by a WRITE operation.
  ASMJIT_INLINE uint32_t getWriteSize() const {
    return getExtendedInfo().getWriteSize();
  }

  // --------------------------------------------------------------------------
  // [Accessors - Operand-Flags]
  // --------------------------------------------------------------------------

  //! Get flags of operand at index `index`.
  ASMJIT_INLINE uint32_t getOperandFlags(uint32_t index) const {
    return getExtendedInfo().getOperandFlags(index);
  }

  // --------------------------------------------------------------------------
  // [Accessors - OpCode]
  // --------------------------------------------------------------------------

  //! Get the primary instruction opcode, see \ref kX86InstOpCode.
  ASMJIT_INLINE uint32_t getPrimaryOpCode() const {
    return _primaryOpCode;
  }

  //! Get the secondary instruction opcode, see \ref kX86InstOpCode.
  ASMJIT_INLINE uint32_t getSecondaryOpCode() const {
    return getExtendedInfo().getSecondaryOpCode();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Instruction name index in `_x86InstName[]` array.
  uint16_t _nameIndex;
  //! Extended information name index in `_x86InstExtendedInfo[]` array.
  uint16_t _extendedIndex;

  //! Primary opcode, secondary opcode is stored in `X86InstExtendedInfo` table.
  uint32_t _primaryOpCode;
};

// ============================================================================
// [asmjit::X86Util]
// ============================================================================

struct X86Util {
  // --------------------------------------------------------------------------
  // [Instruction Info]
  // --------------------------------------------------------------------------

  //! Get instruction information based on `instId`.
  //!
  //! \note `instId` has to be valid instruction ID, it can't be greater than
  //! or equal to `_kX86InstIdCount`. It asserts in debug mode.
  static ASMJIT_INLINE const X86InstInfo& getInstInfo(uint32_t instId) {
    ASMJIT_ASSERT(instId < _kX86InstIdCount);
    return _x86InstInfo[instId];
  }

#if !defined(ASMJIT_DISABLE_NAMES)
  //! Get an instruction ID from a given instruction `name`.
  //!
  //! If there is an exact match the instruction id is returned, otherwise
  //! `kInstIdNone` (zero) is returned.
  //!
  //! The given `name` doesn't have to be null-terminated if `len` is provided.
  ASMJIT_API static uint32_t getInstIdByName(
    const char* name, size_t len = kInvalidIndex);
#endif // !ASMJIT_DISABLE_NAMES

  // --------------------------------------------------------------------------
  // [Condition Codes]
  // --------------------------------------------------------------------------

  //! Corresponds to transposing the operands of a comparison.
  static ASMJIT_INLINE uint32_t reverseCond(uint32_t cond) {
    ASMJIT_ASSERT(cond < ASMJIT_ARRAY_SIZE(_x86ReverseCond));
    return _x86ReverseCond[cond];
  }

  //! Get the equivalent of negated condition code.
  static ASMJIT_INLINE uint32_t negateCond(uint32_t cond) {
    ASMJIT_ASSERT(cond < ASMJIT_ARRAY_SIZE(_x86ReverseCond));
    return static_cast<kX86Cond>(cond ^ static_cast<uint32_t>(cond < kX86CondNone));
  }

  //! Translate condition code `cc` to `cmovcc` instruction code.
  //! \sa \ref kX86InstId, \ref _kX86InstIdCmovcc.
  static ASMJIT_INLINE uint32_t condToCmovcc(uint32_t cond) {
    ASMJIT_ASSERT(static_cast<uint32_t>(cond) < ASMJIT_ARRAY_SIZE(_x86CondToCmovcc));
    return _x86CondToCmovcc[cond];
  }

  //! Translate condition code `cc` to `jcc` instruction code.
  //! \sa \ref kX86InstId, \ref _kX86InstIdJcc.
  static ASMJIT_INLINE uint32_t condToJcc(uint32_t cond) {
    ASMJIT_ASSERT(static_cast<uint32_t>(cond) < ASMJIT_ARRAY_SIZE(_x86CondToJcc));
    return _x86CondToJcc[cond];
  }

  //! Translate condition code `cc` to `setcc` instruction code.
  //! \sa \ref kX86InstId, \ref _kX86InstIdSetcc.
  static ASMJIT_INLINE uint32_t condToSetcc(uint32_t cond) {
    ASMJIT_ASSERT(static_cast<uint32_t>(cond) < ASMJIT_ARRAY_SIZE(_x86CondToSetcc));
    return _x86CondToSetcc[cond];
  }

  // --------------------------------------------------------------------------
  // [MmShuffle]
  // --------------------------------------------------------------------------

  //! Pack a shuffle constant to be used with multimedia instrutions (2 values).
  //!
  //! \param x First component position, number at interval [0, 1] inclusive.
  //! \param y Second component position, number at interval [0, 1] inclusive.
  //!
  //! Shuffle constants can be used to make immediate value for these intrinsics:
  //! - `X86Assembler::shufpd()` and `X86Compiler::shufpd()`
  static ASMJIT_INLINE int mmShuffle(uint32_t x, uint32_t y) {
    return static_cast<int>((x << 1) | y);
  }

  //! Pack a shuffle constant to be used with multimedia instrutions (4 values).
  //!
  //! \param z First component position, number at interval [0, 3] inclusive.
  //! \param x Second component position, number at interval [0, 3] inclusive.
  //! \param y Third component position, number at interval [0, 3] inclusive.
  //! \param w Fourth component position, number at interval [0, 3] inclusive.
  //!
  //! Shuffle constants can be used to make immediate value for these intrinsics:
  //! - `X86Assembler::pshufw()`  and `X86Compiler::pshufw()`
  //! - `X86Assembler::pshufd()`  and `X86Compiler::pshufd()`
  //! - `X86Assembler::pshufhw()` and `X86Compiler::pshufhw()`
  //! - `X86Assembler::pshuflw()` and `X86Compiler::pshuflw()`
  //! - `X86Assembler::shufps()`  and `X86Compiler::shufps()`
  static ASMJIT_INLINE int mmShuffle(uint32_t z, uint32_t y, uint32_t x, uint32_t w) {
    return static_cast<int>((z << 6) | (y << 4) | (x << 2) | w);
  }
};

//! \}

} // asmjit namespace

#undef _OP_ID

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86INST_H
