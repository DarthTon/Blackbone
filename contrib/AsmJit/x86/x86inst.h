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
namespace x86x64 {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct InstInfo;

//! \addtogroup asmjit_x86x64_inst
//! \{

// ============================================================================
// [asmjit::x86x64::Inst/Cond - Globals]
// ============================================================================

#if !defined(ASMJIT_DISABLE_INST_NAMES)
//! \internal
//!
//! X86/X64 instructions' names.
ASMJIT_VAR const char _instName[];
#endif // !ASMJIT_DISABLE_INST_NAMES

//! \internal
//!
//! X86/X64 instructions' information.
ASMJIT_VAR const InstInfo _instInfo[];

// ============================================================================
// [asmjit::x86x64::kInstCode]
// ============================================================================

//! X86/X64 instruction codes.
//!
//! Note that these instruction codes are AsmJit specific. Each instruction has
//! a unique ID that is used as an index to AsmJit instruction table.
ASMJIT_ENUM(kInstCode) {
  kInstAdc = 1,         // X86/X64
  kInstAdd,             // X86/X64
  kInstAddpd,           // SSE2
  kInstAddps,           // SSE
  kInstAddsd,           // SSE2
  kInstAddss,           // SSE
  kInstAddsubpd,        // SSE3
  kInstAddsubps,        // SSE3
  kInstAesdec,          // AESNI
  kInstAesdeclast,      // AESNI
  kInstAesenc,          // AESNI
  kInstAesenclast,      // AESNI
  kInstAesimc,          // AESNI
  kInstAeskeygenassist, // AESNI
  kInstAnd,             // X86/X64
  kInstAndn,            // BMI
  kInstAndnpd,          // SSE2
  kInstAndnps,          // SSE
  kInstAndpd,           // SSE2
  kInstAndps,           // SSE
  kInstBextr,           // BMI
  kInstBlendpd,         // SSE4.1
  kInstBlendps,         // SSE4.1
  kInstBlendvpd,        // SSE4.1
  kInstBlendvps,        // SSE4.1
  kInstBlsi,            // BMI
  kInstBlsmsk,          // BMI
  kInstBlsr,            // BMI
  kInstBsf,             // X86/X64
  kInstBsr,             // X86/X64
  kInstBswap,           // X86/X64 (i486)
  kInstBt,              // X86/X64
  kInstBtc,             // X86/X64
  kInstBtr,             // X86/X64
  kInstBts,             // X86/X64
  kInstBzhi,            // BMI2
  kInstCall,            // X86/X64
  kInstCbw,             // X86/X64
  kInstCdq,             // X86/X64
  kInstCdqe,            // X64 only
  kInstClc,             // X86/X64
  kInstCld,             // X86/X64
  kInstClflush,         // SSE2
  kInstCmc,             // X86/X64
  kInstCmova,           // X86/X64 (cmovcc) (i586)
  kInstCmovae,          // X86/X64 (cmovcc) (i586)
  kInstCmovb,           // X86/X64 (cmovcc) (i586)
  kInstCmovbe,          // X86/X64 (cmovcc) (i586)
  kInstCmovc,           // X86/X64 (cmovcc) (i586)
  kInstCmove,           // X86/X64 (cmovcc) (i586)
  kInstCmovg,           // X86/X64 (cmovcc) (i586)
  kInstCmovge,          // X86/X64 (cmovcc) (i586)
  kInstCmovl,           // X86/X64 (cmovcc) (i586)
  kInstCmovle,          // X86/X64 (cmovcc) (i586)
  kInstCmovna,          // X86/X64 (cmovcc) (i586)
  kInstCmovnae,         // X86/X64 (cmovcc) (i586)
  kInstCmovnb,          // X86/X64 (cmovcc) (i586)
  kInstCmovnbe,         // X86/X64 (cmovcc) (i586)
  kInstCmovnc,          // X86/X64 (cmovcc) (i586)
  kInstCmovne,          // X86/X64 (cmovcc) (i586)
  kInstCmovng,          // X86/X64 (cmovcc) (i586)
  kInstCmovnge,         // X86/X64 (cmovcc) (i586)
  kInstCmovnl,          // X86/X64 (cmovcc) (i586)
  kInstCmovnle,         // X86/X64 (cmovcc) (i586)
  kInstCmovno,          // X86/X64 (cmovcc) (i586)
  kInstCmovnp,          // X86/X64 (cmovcc) (i586)
  kInstCmovns,          // X86/X64 (cmovcc) (i586)
  kInstCmovnz,          // X86/X64 (cmovcc) (i586)
  kInstCmovo,           // X86/X64 (cmovcc) (i586)
  kInstCmovp,           // X86/X64 (cmovcc) (i586)
  kInstCmovpe,          // X86/X64 (cmovcc) (i586)
  kInstCmovpo,          // X86/X64 (cmovcc) (i586)
  kInstCmovs,           // X86/X64 (cmovcc) (i586)
  kInstCmovz,           // X86/X64 (cmovcc) (i586)
  kInstCmp,             // X86/X64
  kInstCmppd,           // SSE2
  kInstCmpps,           // SSE
  kInstCmpsd,           // SSE2
  kInstCmpss,           // SSE
  kInstCmpxchg,         // X86/X64 (i486)
  kInstCmpxchg16b,      // X64 only
  kInstCmpxchg8b,       // X86/X64 (i586)
  kInstComisd,          // SSE2
  kInstComiss,          // SSE
  kInstCpuid,           // X86/X64 (i486)
  kInstCqo,             // X64 only
  kInstCrc32,           // SSE4.2
  kInstCvtdq2pd,        // SSE2
  kInstCvtdq2ps,        // SSE2
  kInstCvtpd2dq,        // SSE2
  kInstCvtpd2pi,        // SSE2
  kInstCvtpd2ps,        // SSE2
  kInstCvtpi2pd,        // SSE2
  kInstCvtpi2ps,        // SSE
  kInstCvtps2dq,        // SSE2
  kInstCvtps2pd,        // SSE2
  kInstCvtps2pi,        // SSE
  kInstCvtsd2si,        // SSE2
  kInstCvtsd2ss,        // SSE2
  kInstCvtsi2sd,        // SSE2
  kInstCvtsi2ss,        // SSE
  kInstCvtss2sd,        // SSE2
  kInstCvtss2si,        // SSE
  kInstCvttpd2dq,       // SSE2
  kInstCvttpd2pi,       // SSE2
  kInstCvttps2dq,       // SSE2
  kInstCvttps2pi,       // SSE
  kInstCvttsd2si,       // SSE2
  kInstCvttss2si,       // SSE
  kInstCwd,             // X86/X64
  kInstCwde,            // X86/X64
  kInstDaa,             // X86 only
  kInstDas,             // X86 only
  kInstDec,             // X86/X64
  kInstDiv,             // X86/X64
  kInstDivpd,           // SSE2
  kInstDivps,           // SSE
  kInstDivsd,           // SSE2
  kInstDivss,           // SSE
  kInstDppd,            // SSE4.1
  kInstDpps,            // SSE4.1
  kInstEmms,            // MMX
  kInstEnter,           // X86/X64
  kInstExtractps,       // SSE4.1
  kInstF2xm1,           // FPU
  kInstFabs,            // FPU
  kInstFadd,            // FPU
  kInstFaddp,           // FPU
  kInstFbld,            // FPU
  kInstFbstp,           // FPU
  kInstFchs,            // FPU
  kInstFclex,           // FPU
  kInstFcmovb,          // FPU
  kInstFcmovbe,         // FPU
  kInstFcmove,          // FPU
  kInstFcmovnb,         // FPU
  kInstFcmovnbe,        // FPU
  kInstFcmovne,         // FPU
  kInstFcmovnu,         // FPU
  kInstFcmovu,          // FPU
  kInstFcom,            // FPU
  kInstFcomi,           // FPU
  kInstFcomip,          // FPU
  kInstFcomp,           // FPU
  kInstFcompp,          // FPU
  kInstFcos,            // FPU
  kInstFdecstp,         // FPU
  kInstFdiv,            // FPU
  kInstFdivp,           // FPU
  kInstFdivr,           // FPU
  kInstFdivrp,          // FPU
  kInstFemms,           // 3dNow!
  kInstFfree,           // FPU
  kInstFiadd,           // FPU
  kInstFicom,           // FPU
  kInstFicomp,          // FPU
  kInstFidiv,           // FPU
  kInstFidivr,          // FPU
  kInstFild,            // FPU
  kInstFimul,           // FPU
  kInstFincstp,         // FPU
  kInstFinit,           // FPU
  kInstFist,            // FPU
  kInstFistp,           // FPU
  kInstFisttp,          // SSE3
  kInstFisub,           // FPU
  kInstFisubr,          // FPU
  kInstFld,             // FPU
  kInstFld1,            // FPU
  kInstFldcw,           // FPU
  kInstFldenv,          // FPU
  kInstFldl2e,          // FPU
  kInstFldl2t,          // FPU
  kInstFldlg2,          // FPU
  kInstFldln2,          // FPU
  kInstFldpi,           // FPU
  kInstFldz,            // FPU
  kInstFmul,            // FPU
  kInstFmulp,           // FPU
  kInstFnclex,          // FPU
  kInstFninit,          // FPU
  kInstFnop,            // FPU
  kInstFnsave,          // FPU
  kInstFnstcw,          // FPU
  kInstFnstenv,         // FPU
  kInstFnstsw,          // FPU
  kInstFpatan,          // FPU
  kInstFprem,           // FPU
  kInstFprem1,          // FPU
  kInstFptan,           // FPU
  kInstFrndint,         // FPU
  kInstFrstor,          // FPU
  kInstFsave,           // FPU
  kInstFscale,          // FPU
  kInstFsin,            // FPU
  kInstFsincos,         // FPU
  kInstFsqrt,           // FPU
  kInstFst,             // FPU
  kInstFstcw,           // FPU
  kInstFstenv,          // FPU
  kInstFstp,            // FPU
  kInstFstsw,           // FPU
  kInstFsub,            // FPU
  kInstFsubp,           // FPU
  kInstFsubr,           // FPU
  kInstFsubrp,          // FPU
  kInstFtst,            // FPU
  kInstFucom,           // FPU
  kInstFucomi,          // FPU
  kInstFucomip,         // FPU
  kInstFucomp,          // FPU
  kInstFucompp,         // FPU
  kInstFwait,           // FPU
  kInstFxam,            // FPU
  kInstFxch,            // FPU
  kInstFxrstor,         // FPU
  kInstFxsave,          // FPU
  kInstFxtract,         // FPU
  kInstFyl2x,           // FPU
  kInstFyl2xp1,         // FPU
  kInstHaddpd,          // SSE3
  kInstHaddps,          // SSE3
  kInstHsubpd,          // SSE3
  kInstHsubps,          // SSE3
  kInstIdiv,            // X86/X64
  kInstImul,            // X86/X64
  kInstInc,             // X86/X64
  kInstInsertps,        // SSE4.1
  kInstInt,             // X86/X64
  kInstJa,              // X86/X64 (jcc)
  kInstJae,             // X86/X64 (jcc)
  kInstJb,              // X86/X64 (jcc)
  kInstJbe,             // X86/X64 (jcc)
  kInstJc,              // X86/X64 (jcc)
  kInstJe,              // X86/X64 (jcc)
  kInstJg,              // X86/X64 (jcc)
  kInstJge,             // X86/X64 (jcc)
  kInstJl,              // X86/X64 (jcc)
  kInstJle,             // X86/X64 (jcc)
  kInstJna,             // X86/X64 (jcc)
  kInstJnae,            // X86/X64 (jcc)
  kInstJnb,             // X86/X64 (jcc)
  kInstJnbe,            // X86/X64 (jcc)
  kInstJnc,             // X86/X64 (jcc)
  kInstJne,             // X86/X64 (jcc)
  kInstJng,             // X86/X64 (jcc)
  kInstJnge,            // X86/X64 (jcc)
  kInstJnl,             // X86/X64 (jcc)
  kInstJnle,            // X86/X64 (jcc)
  kInstJno,             // X86/X64 (jcc)
  kInstJnp,             // X86/X64 (jcc)
  kInstJns,             // X86/X64 (jcc)
  kInstJnz,             // X86/X64 (jcc)
  kInstJo,              // X86/X64 (jcc)
  kInstJp,              // X86/X64 (jcc)
  kInstJpe,             // X86/X64 (jcc)
  kInstJpo,             // X86/X64 (jcc)
  kInstJs,              // X86/X64 (jcc)
  kInstJz,              // X86/X64 (jcc)
  kInstJecxz,           // X86/X64 (jcxz/jecxz/jrcxz)
  kInstJmp,             // X86/X64 (jmp)
  kInstLahf,            // X86/X64 (CPUID NEEDED)
  kInstLddqu,           // SSE3
  kInstLdmxcsr,         // SSE
  kInstLea,             // X86/X64
  kInstLeave,           // X86/X64
  kInstLfence,          // SSE2
  kInstLzcnt,           // LZCNT
  kInstMaskmovdqu,      // SSE2
  kInstMaskmovq,        // MMX-Ext
  kInstMaxpd,           // SSE2
  kInstMaxps,           // SSE
  kInstMaxsd,           // SSE2
  kInstMaxss,           // SSE
  kInstMfence,          // SSE2
  kInstMinpd,           // SSE2
  kInstMinps,           // SSE
  kInstMinsd,           // SSE2
  kInstMinss,           // SSE
  kInstMonitor,         // SSE3
  kInstMov,             // X86/X64
  kInstMovPtr,          // X86/X64
  kInstMovapd,          // SSE2
  kInstMovaps,          // SSE
  kInstMovbe,           // SSE3 - Intel-Atom
  kInstMovd,            // MMX/SSE2
  kInstMovddup,         // SSE3
  kInstMovdq2q,         // SSE2
  kInstMovdqa,          // SSE2
  kInstMovdqu,          // SSE2
  kInstMovhlps,         // SSE
  kInstMovhpd,          // SSE2
  kInstMovhps,          // SSE
  kInstMovlhps,         // SSE
  kInstMovlpd,          // SSE2
  kInstMovlps,          // SSE
  kInstMovmskpd,        // SSE2
  kInstMovmskps,        // SSE2
  kInstMovntdq,         // SSE2
  kInstMovntdqa,        // SSE4.1
  kInstMovnti,          // SSE2
  kInstMovntpd,         // SSE2
  kInstMovntps,         // SSE
  kInstMovntq,          // MMX-Ext
  kInstMovq,            // MMX/SSE/SSE2
  kInstMovq2dq,         // SSE2
  kInstMovsd,           // SSE2
  kInstMovshdup,        // SSE3
  kInstMovsldup,        // SSE3
  kInstMovss,           // SSE
  kInstMovsx,           // X86/X64
  kInstMovsxd,          // X86/X64
  kInstMovupd,          // SSE2
  kInstMovups,          // SSE
  kInstMovzx,           // X86/X64
  kInstMpsadbw,         // SSE4.1
  kInstMul,             // X86/X64
  kInstMulpd,           // SSE2
  kInstMulps,           // SSE
  kInstMulsd,           // SSE2
  kInstMulss,           // SSE
  kInstMulx,            // BMI2
  kInstMwait,           // SSE3
  kInstNeg,             // X86/X64
  kInstNop,             // X86/X64
  kInstNot,             // X86/X64
  kInstOr,              // X86/X64
  kInstOrpd,            // SSE2
  kInstOrps,            // SSE
  kInstPabsb,           // SSSE3
  kInstPabsd,           // SSSE3
  kInstPabsw,           // SSSE3
  kInstPackssdw,        // MMX/SSE2
  kInstPacksswb,        // MMX/SSE2
  kInstPackusdw,        // SSE4.1
  kInstPackuswb,        // MMX/SSE2
  kInstPaddb,           // MMX/SSE2
  kInstPaddd,           // MMX/SSE2
  kInstPaddq,           // SSE2
  kInstPaddsb,          // MMX/SSE2
  kInstPaddsw,          // MMX/SSE2
  kInstPaddusb,         // MMX/SSE2
  kInstPaddusw,         // MMX/SSE2
  kInstPaddw,           // MMX/SSE2
  kInstPalignr,         // SSSE3
  kInstPand,            // MMX/SSE2
  kInstPandn,           // MMX/SSE2
  kInstPause,           // SSE2.
  kInstPavgb,           // MMX-Ext
  kInstPavgw,           // MMX-Ext
  kInstPblendvb,        // SSE4.1
  kInstPblendw,         // SSE4.1
  kInstPclmulqdq,       // PCLMULQDQ
  kInstPcmpeqb,         // MMX/SSE2
  kInstPcmpeqd,         // MMX/SSE2
  kInstPcmpeqq,         // SSE4.1
  kInstPcmpeqw,         // MMX/SSE2
  kInstPcmpestri,       // SSE4.2
  kInstPcmpestrm,       // SSE4.2
  kInstPcmpgtb,         // MMX/SSE2
  kInstPcmpgtd,         // MMX/SSE2
  kInstPcmpgtq,         // SSE4.2
  kInstPcmpgtw,         // MMX/SSE2
  kInstPcmpistri,       // SSE4.2
  kInstPcmpistrm,       // SSE4.2
  kInstPdep,            // BMI2
  kInstPext,            // BMI2
  kInstPextrb,          // SSE4.1
  kInstPextrd,          // SSE4.1
  kInstPextrq,          // SSE4.1
  kInstPextrw,          // MMX-Ext/SSE2
  kInstPf2id,           // 3dNow!
  kInstPf2iw,           // Enhanced 3dNow!
  kInstPfacc,           // 3dNow!
  kInstPfadd,           // 3dNow!
  kInstPfcmpeq,         // 3dNow!
  kInstPfcmpge,         // 3dNow!
  kInstPfcmpgt,         // 3dNow!
  kInstPfmax,           // 3dNow!
  kInstPfmin,           // 3dNow!
  kInstPfmul,           // 3dNow!
  kInstPfnacc,          // Enhanced 3dNow!
  kInstPfpnacc,         // Enhanced 3dNow!
  kInstPfrcp,           // 3dNow!
  kInstPfrcpit1,        // 3dNow!
  kInstPfrcpit2,        // 3dNow!
  kInstPfrsqit1,        // 3dNow!
  kInstPfrsqrt,         // 3dNow!
  kInstPfsub,           // 3dNow!
  kInstPfsubr,          // 3dNow!
  kInstPhaddd,          // SSSE3
  kInstPhaddsw,         // SSSE3
  kInstPhaddw,          // SSSE3
  kInstPhminposuw,      // SSE4.1
  kInstPhsubd,          // SSSE3
  kInstPhsubsw,         // SSSE3
  kInstPhsubw,          // SSSE3
  kInstPi2fd,           // 3dNow!
  kInstPi2fw,           // Enhanced 3dNow!
  kInstPinsrb,          // SSE4.1
  kInstPinsrd,          // SSE4.1
  kInstPinsrq,          // SSE4.1
  kInstPinsrw,          // MMX-Ext
  kInstPmaddubsw,       // SSSE3
  kInstPmaddwd,         // MMX/SSE2
  kInstPmaxsb,          // SSE4.1
  kInstPmaxsd,          // SSE4.1
  kInstPmaxsw,          // MMX-Ext
  kInstPmaxub,          // MMX-Ext
  kInstPmaxud,          // SSE4.1
  kInstPmaxuw,          // SSE4.1
  kInstPminsb,          // SSE4.1
  kInstPminsd,          // SSE4.1
  kInstPminsw,          // MMX-Ext
  kInstPminub,          // MMX-Ext
  kInstPminud,          // SSE4.1
  kInstPminuw,          // SSE4.1
  kInstPmovmskb,        // MMX-Ext
  kInstPmovsxbd,        // SSE4.1
  kInstPmovsxbq,        // SSE4.1
  kInstPmovsxbw,        // SSE4.1
  kInstPmovsxdq,        // SSE4.1
  kInstPmovsxwd,        // SSE4.1
  kInstPmovsxwq,        // SSE4.1
  kInstPmovzxbd,        // SSE4.1
  kInstPmovzxbq,        // SSE4.1
  kInstPmovzxbw,        // SSE4.1
  kInstPmovzxdq,        // SSE4.1
  kInstPmovzxwd,        // SSE4.1
  kInstPmovzxwq,        // SSE4.1
  kInstPmuldq,          // SSE4.1
  kInstPmulhrsw,        // SSSE3
  kInstPmulhuw,         // MMX-Ext
  kInstPmulhw,          // MMX/SSE2
  kInstPmulld,          // SSE4.1
  kInstPmullw,          // MMX/SSE2
  kInstPmuludq,         // SSE2
  kInstPop,             // X86/X64
  kInstPopa,            // X86 only
  kInstPopcnt,          // SSE4.2
  kInstPopf,            // X86/X64
  kInstPor,             // MMX/SSE2
  kInstPrefetch,        // MMX-Ext/SSE
  kInstPrefetch3dNow,   // 3dNow!
  kInstPrefetchw3dNow,  // 3dNow!
  kInstPsadbw,          // MMX-Ext
  kInstPshufb,          // SSSE3
  kInstPshufd,          // SSE2
  kInstPshufhw,         // SSE2
  kInstPshuflw,         // SSE2
  kInstPshufw,          // MMX-Ext
  kInstPsignb,          // SSSE3
  kInstPsignd,          // SSSE3
  kInstPsignw,          // SSSE3
  kInstPslld,           // MMX/SSE2
  kInstPslldq,          // SSE2
  kInstPsllq,           // MMX/SSE2
  kInstPsllw,           // MMX/SSE2
  kInstPsrad,           // MMX/SSE2
  kInstPsraw,           // MMX/SSE2
  kInstPsrld,           // MMX/SSE2
  kInstPsrldq,          // SSE2
  kInstPsrlq,           // MMX/SSE2
  kInstPsrlw,           // MMX/SSE2
  kInstPsubb,           // MMX/SSE2
  kInstPsubd,           // MMX/SSE2
  kInstPsubq,           // SSE2
  kInstPsubsb,          // MMX/SSE2
  kInstPsubsw,          // MMX/SSE2
  kInstPsubusb,         // MMX/SSE2
  kInstPsubusw,         // MMX/SSE2
  kInstPsubw,           // MMX/SSE2
  kInstPswapd,          // Enhanced 3dNow!
  kInstPtest,           // SSE4.1
  kInstPunpckhbw,       // MMX/SSE2
  kInstPunpckhdq,       // MMX/SSE2
  kInstPunpckhqdq,      // SSE2
  kInstPunpckhwd,       // MMX/SSE2
  kInstPunpcklbw,       // MMX/SSE2
  kInstPunpckldq,       // MMX/SSE2
  kInstPunpcklqdq,      // SSE2
  kInstPunpcklwd,       // MMX/SSE2
  kInstPush,            // X86/X64
  kInstPusha,           // X86 only
  kInstPushf,           // X86/X64
  kInstPxor,            // MMX/SSE2
  kInstRcl,             // X86/X64
  kInstRcpps,           // SSE
  kInstRcpss,           // SSE
  kInstRcr,             // X86/X64
  kInstRdfsbase,        // FSGSBASE (x64)
  kInstRdgsbase,        // FSGSBASE (x64)
  kInstRdrand,          // RDRAND
  kInstRdtsc,           // X86/X64
  kInstRdtscp,          // X86/X64
  kInstRepLodsb,        // X86/X64 (REP)
  kInstRepLodsd,        // X86/X64 (REP)
  kInstRepLodsq,        // X64 only (REP)
  kInstRepLodsw,        // X86/X64 (REP)
  kInstRepMovsb,        // X86/X64 (REP)
  kInstRepMovsd,        // X86/X64 (REP)
  kInstRepMovsq,        // X64 only (REP)
  kInstRepMovsw,        // X86/X64 (REP)
  kInstRepStosb,        // X86/X64 (REP)
  kInstRepStosd,        // X86/X64 (REP)
  kInstRepStosq,        // X64 only (REP)
  kInstRepStosw,        // X86/X64 (REP)
  kInstRepeCmpsb,       // X86/X64 (REP)
  kInstRepeCmpsd,       // X86/X64 (REP)
  kInstRepeCmpsq,       // X64 only (REP)
  kInstRepeCmpsw,       // X86/X64 (REP)
  kInstRepeScasb,       // X86/X64 (REP)
  kInstRepeScasd,       // X86/X64 (REP)
  kInstRepeScasq,       // X64 only (REP)
  kInstRepeScasw,       // X86/X64 (REP)
  kInstRepneCmpsb,      // X86/X64 (REP)
  kInstRepneCmpsd,      // X86/X64 (REP)
  kInstRepneCmpsq,      // X64 only (REP)
  kInstRepneCmpsw,      // X86/X64 (REP)
  kInstRepneScasb,      // X86/X64 (REP)
  kInstRepneScasd,      // X86/X64 (REP)
  kInstRepneScasq,      // X64 only (REP)
  kInstRepneScasw,      // X86/X64 (REP)
  kInstRet,             // X86/X64
  kInstRol,             // X86/X64
  kInstRor,             // X86/X64
  kInstRorx,            // BMI2
  kInstRoundpd,         // SSE4.1
  kInstRoundps,         // SSE4.1
  kInstRoundsd,         // SSE4.1
  kInstRoundss,         // SSE4.1
  kInstRsqrtps,         // SSE
  kInstRsqrtss,         // SSE
  kInstSahf,            // X86/X64 (CPUID NEEDED)
  kInstSal,             // X86/X64
  kInstSar,             // X86/X64
  kInstSarx,            // BMI2
  kInstSbb,             // X86/X64
  kInstSeta,            // X86/X64 (setcc)
  kInstSetae,           // X86/X64 (setcc)
  kInstSetb,            // X86/X64 (setcc)
  kInstSetbe,           // X86/X64 (setcc)
  kInstSetc,            // X86/X64 (setcc)
  kInstSete,            // X86/X64 (setcc)
  kInstSetg,            // X86/X64 (setcc)
  kInstSetge,           // X86/X64 (setcc)
  kInstSetl,            // X86/X64 (setcc)
  kInstSetle,           // X86/X64 (setcc)
  kInstSetna,           // X86/X64 (setcc)
  kInstSetnae,          // X86/X64 (setcc)
  kInstSetnb,           // X86/X64 (setcc)
  kInstSetnbe,          // X86/X64 (setcc)
  kInstSetnc,           // X86/X64 (setcc)
  kInstSetne,           // X86/X64 (setcc)
  kInstSetng,           // X86/X64 (setcc)
  kInstSetnge,          // X86/X64 (setcc)
  kInstSetnl,           // X86/X64 (setcc)
  kInstSetnle,          // X86/X64 (setcc)
  kInstSetno,           // X86/X64 (setcc)
  kInstSetnp,           // X86/X64 (setcc)
  kInstSetns,           // X86/X64 (setcc)
  kInstSetnz,           // X86/X64 (setcc)
  kInstSeto,            // X86/X64 (setcc)
  kInstSetp,            // X86/X64 (setcc)
  kInstSetpe,           // X86/X64 (setcc)
  kInstSetpo,           // X86/X64 (setcc)
  kInstSets,            // X86/X64 (setcc)
  kInstSetz,            // X86/X64 (setcc)
  kInstSfence,          // MMX-Ext/SSE
  kInstShl,             // X86/X64
  kInstShld,            // X86/X64
  kInstShlx,            // BMI2
  kInstShr,             // X86/X64
  kInstShrd,            // X86/X64
  kInstShrx,            // BMI2
  kInstShufpd,          // SSE2
  kInstShufps,          // SSE
  kInstSqrtpd,          // SSE2
  kInstSqrtps,          // SSE
  kInstSqrtsd,          // SSE2
  kInstSqrtss,          // SSE
  kInstStc,             // X86/X64
  kInstStd,             // X86/X64
  kInstStmxcsr,         // SSE
  kInstSub,             // X86/X64
  kInstSubpd,           // SSE2
  kInstSubps,           // SSE
  kInstSubsd,           // SSE2
  kInstSubss,           // SSE
  kInstTest,            // X86/X64
  kInstTzcnt,           // TZCNT
  kInstUcomisd,         // SSE2
  kInstUcomiss,         // SSE
  kInstUd2,             // X86/X64
  kInstUnpckhpd,        // SSE2
  kInstUnpckhps,        // SSE
  kInstUnpcklpd,        // SSE2
  kInstUnpcklps,        // SSE
  kInstVaddpd,          // AVX
  kInstVaddps,          // AVX
  kInstVaddsd,          // AVX
  kInstVaddss,          // AVX
  kInstVaddsubpd,       // AVX
  kInstVaddsubps,       // AVX
  kInstVaesdec,         // AVX+AESNI
  kInstVaesdeclast,     // AVX+AESNI
  kInstVaesenc,         // AVX+AESNI
  kInstVaesenclast,     // AVX+AESNI
  kInstVaesimc,         // AVX+AESNI
  kInstVaeskeygenassist,// AVX+AESNI
  kInstVandnpd,         // AVX
  kInstVandnps,         // AVX
  kInstVandpd,          // AVX
  kInstVandps,          // AVX
  kInstVblendpd,        // AVX
  kInstVblendps,        // AVX
  kInstVblendvpd,       // AVX
  kInstVblendvps,       // AVX
  kInstVbroadcastf128,  // AVX
  kInstVbroadcasti128,  // AVX2
  kInstVbroadcastsd,    // AVX/AVX2
  kInstVbroadcastss,    // AVX/AVX2
  kInstVcmppd,          // AVX
  kInstVcmpps,          // AVX
  kInstVcmpsd,          // AVX
  kInstVcmpss,          // AVX
  kInstVcomisd,         // AVX
  kInstVcomiss,         // AVX
  kInstVcvtdq2pd,       // AVX
  kInstVcvtdq2ps,       // AVX
  kInstVcvtpd2dq,       // AVX
  kInstVcvtpd2ps,       // AVX
  kInstVcvtph2ps,       // F16C
  kInstVcvtps2dq,       // AVX
  kInstVcvtps2pd,       // AVX
  kInstVcvtps2ph,       // F16C
  kInstVcvtsd2si,       // AVX
  kInstVcvtsd2ss,       // AVX
  kInstVcvtsi2sd,       // AVX
  kInstVcvtsi2ss,       // AVX
  kInstVcvtss2sd,       // AVX
  kInstVcvtss2si,       // AVX
  kInstVcvttpd2dq,      // AVX
  kInstVcvttps2dq,      // AVX
  kInstVcvttsd2si,      // AVX
  kInstVcvttss2si,      // AVX
  kInstVdivpd,          // AVX
  kInstVdivps,          // AVX
  kInstVdivsd,          // AVX
  kInstVdivss,          // AVX
  kInstVdppd,           // AVX
  kInstVdpps,           // AVX
  kInstVextractf128,    // AVX
  kInstVextracti128,    // AVX2
  kInstVextractps,      // AVX
  kInstVfmadd132pd,     // FMA3
  kInstVfmadd132ps,     // FMA3
  kInstVfmadd132sd,     // FMA3
  kInstVfmadd132ss,     // FMA3
  kInstVfmadd213pd,     // FMA3
  kInstVfmadd213ps,     // FMA3
  kInstVfmadd213sd,     // FMA3
  kInstVfmadd213ss,     // FMA3
  kInstVfmadd231pd,     // FMA3
  kInstVfmadd231ps,     // FMA3
  kInstVfmadd231sd,     // FMA3
  kInstVfmadd231ss,     // FMA3
  kInstVfmaddpd,        // FMA4
  kInstVfmaddps,        // FMA4
  kInstVfmaddsd,        // FMA4
  kInstVfmaddss,        // FMA4
  kInstVfmaddsub132pd,  // FMA3
  kInstVfmaddsub132ps,  // FMA3
  kInstVfmaddsub213pd,  // FMA3
  kInstVfmaddsub213ps,  // FMA3
  kInstVfmaddsub231pd,  // FMA3
  kInstVfmaddsub231ps,  // FMA3
  kInstVfmaddsubpd,     // FMA4
  kInstVfmaddsubps,     // FMA4
  kInstVfmsub132pd,     // FMA3
  kInstVfmsub132ps,     // FMA3
  kInstVfmsub132sd,     // FMA3
  kInstVfmsub132ss,     // FMA3
  kInstVfmsub213pd,     // FMA3
  kInstVfmsub213ps,     // FMA3
  kInstVfmsub213sd,     // FMA3
  kInstVfmsub213ss,     // FMA3
  kInstVfmsub231pd,     // FMA3
  kInstVfmsub231ps,     // FMA3
  kInstVfmsub231sd,     // FMA3
  kInstVfmsub231ss,     // FMA3
  kInstVfmsubadd132pd,  // FMA3
  kInstVfmsubadd132ps,  // FMA3
  kInstVfmsubadd213pd,  // FMA3
  kInstVfmsubadd213ps,  // FMA3
  kInstVfmsubadd231pd,  // FMA3
  kInstVfmsubadd231ps,  // FMA3
  kInstVfmsubaddpd,     // FMA4
  kInstVfmsubaddps,     // FMA4
  kInstVfmsubpd,        // FMA4
  kInstVfmsubps,        // FMA4
  kInstVfmsubsd,        // FMA4
  kInstVfmsubss,        // FMA4
  kInstVfnmadd132pd,    // FMA3
  kInstVfnmadd132ps,    // FMA3
  kInstVfnmadd132sd,    // FMA3
  kInstVfnmadd132ss,    // FMA3
  kInstVfnmadd213pd,    // FMA3
  kInstVfnmadd213ps,    // FMA3
  kInstVfnmadd213sd,    // FMA3
  kInstVfnmadd213ss,    // FMA3
  kInstVfnmadd231pd,    // FMA3
  kInstVfnmadd231ps,    // FMA3
  kInstVfnmadd231sd,    // FMA3
  kInstVfnmadd231ss,    // FMA3
  kInstVfnmaddpd,       // FMA4
  kInstVfnmaddps,       // FMA4
  kInstVfnmaddsd,       // FMA4
  kInstVfnmaddss,       // FMA4
  kInstVfnmsub132pd,    // FMA3
  kInstVfnmsub132ps,    // FMA3
  kInstVfnmsub132sd,    // FMA3
  kInstVfnmsub132ss,    // FMA3
  kInstVfnmsub213pd,    // FMA3
  kInstVfnmsub213ps,    // FMA3
  kInstVfnmsub213sd,    // FMA3
  kInstVfnmsub213ss,    // FMA3
  kInstVfnmsub231pd,    // FMA3
  kInstVfnmsub231ps,    // FMA3
  kInstVfnmsub231sd,    // FMA3
  kInstVfnmsub231ss,    // FMA3
  kInstVfnmsubpd,       // FMA4
  kInstVfnmsubps,       // FMA4
  kInstVfnmsubsd,       // FMA4
  kInstVfnmsubss,       // FMA4
  kInstVfrczpd,         // XOP
  kInstVfrczps,         // XOP
  kInstVfrczsd,         // XOP
  kInstVfrczss,         // XOP
  kInstVgatherdpd,      // AVX2
  kInstVgatherdps,      // AVX2
  kInstVgatherqpd,      // AVX2
  kInstVgatherqps,      // AVX2
  kInstVhaddpd,         // AVX
  kInstVhaddps,         // AVX
  kInstVhsubpd,         // AVX
  kInstVhsubps,         // AVX
  kInstVinsertf128,     // AVX
  kInstVinserti128,     // AVX2
  kInstVinsertps,       // AVX
  kInstVlddqu,          // AVX
  kInstVldmxcsr,        // AVX
  kInstVmaskmovdqu,     // AVX
  kInstVmaskmovpd,      // AVX
  kInstVmaskmovps,      // AVX
  kInstVmaxpd,          // AVX
  kInstVmaxps,          // AVX
  kInstVmaxsd,          // AVX
  kInstVmaxss,          // AVX
  kInstVminpd,          // AVX
  kInstVminps,          // AVX
  kInstVminsd,          // AVX
  kInstVminss,          // AVX
  kInstVmovapd,         // AVX
  kInstVmovaps,         // AVX
  kInstVmovd,           // AVX
  kInstVmovddup,        // AVX
  kInstVmovdqa,         // AVX
  kInstVmovdqu,         // AVX
  kInstVmovhlps,        // AVX
  kInstVmovhpd,         // AVX
  kInstVmovhps,         // AVX
  kInstVmovlhps,        // AVX
  kInstVmovlpd,         // AVX
  kInstVmovlps,         // AVX
  kInstVmovmskpd,       // AVX
  kInstVmovmskps,       // AVX
  kInstVmovntdq,        // AVX
  kInstVmovntdqa,       // AVX/AVX2
  kInstVmovntpd,        // AVX
  kInstVmovntps,        // AVX
  kInstVmovq,           // AVX
  kInstVmovsd,          // AVX
  kInstVmovshdup,       // AVX
  kInstVmovsldup,       // AVX
  kInstVmovss,          // AVX
  kInstVmovupd,         // AVX
  kInstVmovups,         // AVX
  kInstVmpsadbw,        // AVX/AVX2
  kInstVmulpd,          // AVX
  kInstVmulps,          // AVX
  kInstVmulsd,          // AVX
  kInstVmulss,          // AVX
  kInstVorpd,           // AVX
  kInstVorps,           // AVX
  kInstVpabsb,          // AVX2
  kInstVpabsd,          // AVX2
  kInstVpabsw,          // AVX2
  kInstVpackssdw,       // AVX2
  kInstVpacksswb,       // AVX2
  kInstVpackusdw,       // AVX2
  kInstVpackuswb,       // AVX2
  kInstVpaddb,          // AVX2
  kInstVpaddd,          // AVX2
  kInstVpaddq,          // AVX2
  kInstVpaddsb,         // AVX2
  kInstVpaddsw,         // AVX2
  kInstVpaddusb,        // AVX2
  kInstVpaddusw,        // AVX2
  kInstVpaddw,          // AVX2
  kInstVpalignr,        // AVX2
  kInstVpand,           // AVX2
  kInstVpandn,          // AVX2
  kInstVpavgb,          // AVX2
  kInstVpavgw,          // AVX2
  kInstVpblendd,        // AVX2
  kInstVpblendvb,       // AVX2
  kInstVpblendw,        // AVX2
  kInstVpbroadcastb,    // AVX2
  kInstVpbroadcastd,    // AVX2
  kInstVpbroadcastq,    // AVX2
  kInstVpbroadcastw,    // AVX2
  kInstVpclmulqdq,      // AVX+PCLMULQDQ
  kInstVpcmov,          // XOP
  kInstVpcmpeqb,        // AVX2
  kInstVpcmpeqd,        // AVX2
  kInstVpcmpeqq,        // AVX2
  kInstVpcmpeqw,        // AVX2
  kInstVpcmpestri,      // AVX
  kInstVpcmpestrm,      // AVX
  kInstVpcmpgtb,        // AVX2
  kInstVpcmpgtd,        // AVX2
  kInstVpcmpgtq,        // AVX2
  kInstVpcmpgtw,        // AVX2
  kInstVpcmpistri,      // AVX
  kInstVpcmpistrm,      // AVX
  kInstVpcomb,          // XOP
  kInstVpcomd,          // XOP
  kInstVpcomq,          // XOP
  kInstVpcomub,         // XOP
  kInstVpcomud,         // XOP
  kInstVpcomuq,         // XOP
  kInstVpcomuw,         // XOP
  kInstVpcomw,          // XOP
  kInstVperm2f128,      // AVX
  kInstVperm2i128,      // AVX2
  kInstVpermd,          // AVX2
  kInstVpermil2pd,      // XOP
  kInstVpermil2ps,      // XOP
  kInstVpermilpd,       // AVX
  kInstVpermilps,       // AVX
  kInstVpermpd,         // AVX2
  kInstVpermps,         // AVX2
  kInstVpermq,          // AVX2
  kInstVpextrb,         // AVX
  kInstVpextrd,         // AVX
  kInstVpextrq,         // AVX (x64 only)
  kInstVpextrw,         // AVX
  kInstVpgatherdd,      // AVX2
  kInstVpgatherdq,      // AVX2
  kInstVpgatherqd,      // AVX2
  kInstVpgatherqq,      // AVX2
  kInstVphaddbd,        // XOP
  kInstVphaddbq,        // XOP
  kInstVphaddbw,        // XOP
  kInstVphaddd,         // AVX2
  kInstVphadddq,        // XOP
  kInstVphaddsw,        // AVX2
  kInstVphaddubd,       // XOP
  kInstVphaddubq,       // XOP
  kInstVphaddubw,       // XOP
  kInstVphaddudq,       // XOP
  kInstVphadduwd,       // XOP
  kInstVphadduwq,       // XOP
  kInstVphaddw,         // AVX2
  kInstVphaddwd,        // XOP
  kInstVphaddwq,        // XOP
  kInstVphminposuw,     // AVX
  kInstVphsubbw,        // XOP
  kInstVphsubd,         // AVX2
  kInstVphsubdq,        // XOP
  kInstVphsubsw,        // AVX2
  kInstVphsubw,         // AVX2
  kInstVphsubwd,        // XOP
  kInstVpinsrb,         // AVX
  kInstVpinsrd,         // AVX
  kInstVpinsrq,         // AVX (x64 only)
  kInstVpinsrw,         // AVX
  kInstVpmacsdd,        // XOP
  kInstVpmacsdqh,       // XOP
  kInstVpmacsdql,       // XOP
  kInstVpmacssdd,       // XOP
  kInstVpmacssdqh,      // XOP
  kInstVpmacssdql,      // XOP
  kInstVpmacsswd,       // XOP
  kInstVpmacssww,       // XOP
  kInstVpmacswd,        // XOP
  kInstVpmacsww,        // XOP
  kInstVpmadcsswd,      // XOP
  kInstVpmadcswd,       // XOP
  kInstVpmaddubsw,      // AVX/AVX2
  kInstVpmaddwd,        // AVX/AVX2
  kInstVpmaskmovd,      // AVX2
  kInstVpmaskmovq,      // AVX2
  kInstVpmaxsb,         // AVX/AVX2
  kInstVpmaxsd,         // AVX/AVX2
  kInstVpmaxsw,         // AVX/AVX2
  kInstVpmaxub,         // AVX/AVX2
  kInstVpmaxud,         // AVX/AVX2
  kInstVpmaxuw,         // AVX/AVX2
  kInstVpminsb,         // AVX/AVX2
  kInstVpminsd,         // AVX/AVX2
  kInstVpminsw,         // AVX/AVX2
  kInstVpminub,         // AVX/AVX2
  kInstVpminud,         // AVX/AVX2
  kInstVpminuw,         // AVX/AVX2
  kInstVpmovmskb,       // AVX/AVX2
  kInstVpmovsxbd,       // AVX/AVX2
  kInstVpmovsxbq,       // AVX/AVX2
  kInstVpmovsxbw,       // AVX/AVX2
  kInstVpmovsxdq,       // AVX/AVX2
  kInstVpmovsxwd,       // AVX/AVX2
  kInstVpmovsxwq,       // AVX/AVX2
  kInstVpmovzxbd,       // AVX/AVX2
  kInstVpmovzxbq,       // AVX/AVX2
  kInstVpmovzxbw,       // AVX/AVX2
  kInstVpmovzxdq,       // AVX/AVX2
  kInstVpmovzxwd,       // AVX/AVX2
  kInstVpmovzxwq,       // AVX/AVX2
  kInstVpmuldq,         // AVX/AVX2
  kInstVpmulhrsw,       // AVX/AVX2
  kInstVpmulhuw,        // AVX/AVX2
  kInstVpmulhw,         // AVX/AVX2
  kInstVpmulld,         // AVX/AVX2
  kInstVpmullw,         // AVX/AVX2
  kInstVpmuludq,        // AVX/AVX2
  kInstVpor,            // AVX/AVX2
  kInstVpperm,          // XOP
  kInstVprotb,          // XOP
  kInstVprotd,          // XOP
  kInstVprotq,          // XOP
  kInstVprotw,          // XOP
  kInstVpsadbw,         // AVX/AVX2
  kInstVpshab,          // XOP
  kInstVpshad,          // XOP
  kInstVpshaq,          // XOP
  kInstVpshaw,          // XOP
  kInstVpshlb,          // XOP
  kInstVpshld,          // XOP
  kInstVpshlq,          // XOP
  kInstVpshlw,          // XOP
  kInstVpshufb,         // AVX/AVX2
  kInstVpshufd,         // AVX/AVX2
  kInstVpshufhw,        // AVX/AVX2
  kInstVpshuflw,        // AVX/AVX2
  kInstVpsignb,         // AVX/AVX2
  kInstVpsignd,         // AVX/AVX2
  kInstVpsignw,         // AVX/AVX2
  kInstVpslld,          // AVX/AVX2
  kInstVpslldq,         // AVX/AVX2
  kInstVpsllq,          // AVX/AVX2
  kInstVpsllvd,         // AVX2
  kInstVpsllvq,         // AVX2
  kInstVpsllw,          // AVX/AVX2
  kInstVpsrad,          // AVX/AVX2
  kInstVpsravd,         // AVX2
  kInstVpsraw,          // AVX/AVX2
  kInstVpsrld,          // AVX/AVX2
  kInstVpsrldq,         // AVX/AVX2
  kInstVpsrlq,          // AVX/AVX2
  kInstVpsrlvd,         // AVX2
  kInstVpsrlvq,         // AVX2
  kInstVpsrlw,          // AVX/AVX2
  kInstVpsubb,          // AVX/AVX2
  kInstVpsubd,          // AVX/AVX2
  kInstVpsubq,          // AVX/AVX2
  kInstVpsubsb,         // AVX/AVX2
  kInstVpsubsw,         // AVX/AVX2
  kInstVpsubusb,        // AVX/AVX2
  kInstVpsubusw,        // AVX/AVX2
  kInstVpsubw,          // AVX/AVX2
  kInstVptest,          // AVX
  kInstVpunpckhbw,      // AVX/AVX2
  kInstVpunpckhdq,      // AVX/AVX2
  kInstVpunpckhqdq,     // AVX/AVX2
  kInstVpunpckhwd,      // AVX/AVX2
  kInstVpunpcklbw,      // AVX/AVX2
  kInstVpunpckldq,      // AVX/AVX2
  kInstVpunpcklqdq,     // AVX/AVX2
  kInstVpunpcklwd,      // AVX/AVX2
  kInstVpxor,           // AVX/AVX2
  kInstVrcpps,          // AVX
  kInstVrcpss,          // AVX
  kInstVroundpd,        // AVX
  kInstVroundps,        // AVX
  kInstVroundsd,        // AVX
  kInstVroundss,        // AVX
  kInstVrsqrtps,        // AVX
  kInstVrsqrtss,        // AVX
  kInstVshufpd,         // AVX
  kInstVshufps,         // AVX
  kInstVsqrtpd,         // AVX
  kInstVsqrtps,         // AVX
  kInstVsqrtsd,         // AVX
  kInstVsqrtss,         // AVX
  kInstVstmxcsr,        // AVX
  kInstVsubpd,          // AVX
  kInstVsubps,          // AVX
  kInstVsubsd,          // AVX
  kInstVsubss,          // AVX
  kInstVtestpd,         // AVX
  kInstVtestps,         // AVX
  kInstVucomisd,        // AVX
  kInstVucomiss,        // AVX
  kInstVunpckhpd,       // AVX
  kInstVunpckhps,       // AVX
  kInstVunpcklpd,       // AVX
  kInstVunpcklps,       // AVX
  kInstVxorpd,          // AVX
  kInstVxorps,          // AVX
  kInstVzeroall,        // AVX
  kInstVzeroupper,      // AVX
  kInstWrfsbase,        // FSGSBASE (x64)
  kInstWrgsbase,        // FSGSBASE (x64)
  kInstXadd,            // X86/X64 (i486)
  kInstXchg,            // X86/X64 (i386)
  kInstXor,             // X86/X64
  kInstXorpd,           // SSE2
  kInstXorps,           // SSE

  _kInstCount,

  _kInstCmovcc = kInstCmova,
  _kInstJcc = kInstJa,
  _kInstSetcc = kInstSeta,

  _kInstJbegin = kInstJa,
  _kInstJend = kInstJmp
};

// ============================================================================
// [asmjit::x86x64::kInstOptions]
// ============================================================================

//! X86/X64 instruction emit options, mainly for internal purposes.
ASMJIT_ENUM(kInstOptions) {
  //! Emit instruction with LOCK prefix.
  //!
  //! If this option is used and instruction doesn't support LOCK prefix an
  //! invalid instruction error is generated.
  kInstOptionLock = 0x10,

  //! Force REX prefix to be emitted.
  //!
  //! This option should be used carefully, because there are unencodable
  //! combinations. If you want to access ah, bh, ch or dh registers the REX
  //! prefix can't be emitted, otherwise illegal instruction error will be
  //! returned.
  kInstOptionRex = 0x40,

  //! Force three-byte VEX prefix to be emitted (instead of more compact
  //! two-byte VEX prefix).
  //!
  //! Ignored if the instruction doesn't use VEX prefix.
  kInstOptionVex3 = 0x80
};

// ============================================================================
// [asmjit::x86x64::kInstGroup]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction groups.
//!
//! This group is specific to AsmJit and only used by `x86x64::X86X64Assembler`.
ASMJIT_ENUM(kInstGroup) {
  //! Never used.
  kInstGroupNone,

  kInstGroupX86Op,
  kInstGroupX86Rm,
  kInstGroupX86Rm_B,
  kInstGroupX86RmReg,
  kInstGroupX86RegRm,
  kInstGroupX86M,
  //! Adc/Add/And/Cmp/Or/Sbb/Sub/Xor.
  kInstGroupX86Arith,
  //! Bswap.
  kInstGroupX86BSwap,
  //! Bt/Btc/Btr/Bts.
  kInstGroupX86BTest,
  //! Call.
  kInstGroupX86Call,
  //! Enter.
  kInstGroupX86Enter,
  //! Imul.
  kInstGroupX86Imul,
  //! Inc/Dec.
  kInstGroupX86IncDec,
  //! Int.
  kInstGroupX86Int,
  //! Jcc.
  kInstGroupX86Jcc,
  //! Jcxz/Jecxz/Jrcxz.
  kInstGroupX86Jecxz,
  //! Jmp.
  kInstGroupX86Jmp,
  //! Lea->
  kInstGroupX86Lea,
  //! Mov.
  kInstGroupX86Mov,
  //! Movsx/Movzx.
  kInstGroupX86MovSxZx,
  //! Movsxd.
  kInstGroupX86MovSxd,
  //! Mov having absolute memory operand (x86/x64).
  kInstGroupX86MovPtr,
  //! Push.
  kInstGroupX86Push,
  //! Pop.
  kInstGroupX86Pop,
  //! Rep/Repe/Repne LodsX/MovsX/StosX/CmpsX/ScasX.
  kInstGroupX86Rep,
  //! Ret.
  kInstGroupX86Ret,
  //! Rcl/Rcr/Rol/Ror/Sal/Sar/Shl/Shr.
  kInstGroupX86Rot,
  //! Setcc.
  kInstGroupX86Set,
  //! Shld/Rhrd.
  kInstGroupX86Shlrd,
  //! Test.
  kInstGroupX86Test,
  //! Xadd.
  kInstGroupX86Xadd,
  //! Xchg.
  kInstGroupX86Xchg,

  //! Fincstp/Finit/FldX/Fnclex/Fninit/Fnop/Fpatan/Fprem/Fprem1/Fptan/Frndint/Fscale/Fsin/Fsincos/Fsqrt/Ftst/Fucompp/Fxam/Fxtract/Fyl2x/Fyl2xp1.
  kInstGroupFpuOp,
  //! Fadd/Fdiv/Fdivr/Fmul/Fsub/Fsubr.
  kInstGroupFpuArith,
  //! Fcom/Fcomp.
  kInstGroupFpuCom,
  //! Fld/Fst/Fstp.
  kInstGroupFpuFldFst,
  //! Fiadd/Ficom/Ficomp/Fidiv/Fidivr/Fild/Fimul/Fist/Fistp/Fisttp/Fisub/Fisubr.
  kInstGroupFpuM,
  //! Fcmov/Fcomi/Fcomip/Ffree/Fucom/Fucomi/Fucomip/Fucomp/Fxch.
  kInstGroupFpuR,
  //! Faddp/Fdivp/Fdivrp/Fmulp/Fsubp/Fsubrp.
  kInstGroupFpuRDef,
  //! Fnstsw/Fstsw.
  kInstGroupFpuStsw,

  //! Mm/Xmm instruction.
  kInstGroupExtRm,
  //! Mm/Xmm instruction (propagates 66H if the instruction uses Xmm register).
  kInstGroupExtRm_P,
  //! Mm/Xmm instruction (propagates REX.W if GPQ is used).
  kInstGroupExtRm_Q,
  //! Mm/Xmm instruction (propagates 66H and REX.W).
  kInstGroupExtRm_PQ,
  //! Mm/Xmm instruction having Rm/Ri encodings.
  kInstGroupExtRmRi,
  //! Mm/Xmm instruction having Rm/Ri encodings (propagates 66H if the instruction uses Xmm register).
  kInstGroupExtRmRi_P,
  //! Mm/Xmm instruction having Rmi encoding.
  kInstGroupExtRmi,
  //! Mm/Xmm instruction having Rmi encoding (propagates 66H if the instruction uses Xmm register).
  kInstGroupExtRmi_P,
  //! Crc32.
  kInstGroupExtCrc,
  //! Pextrb/Pextrw/Pextrd/Pextrq/Extractps.
  kInstGroupExtExtract,
  //! Lfence/Mfence/Sfence.
  kInstGroupExtFence,
  //! Mov Mm/Xmm.
  //!
  //! 0x66 prefix must be set manually in opcodes.
  //!
  //! - Primary opcode is used for instructions in (X)Mm <- (X)Mm/Mem format,
  //! - Secondary opcode is used for instructions in (X)Mm/Mem <- (X)Mm format.
  kInstGroupExtMov,
  //! Mov Mm/Xmm.
  kInstGroupExtMovNoRexW,
  //! Movbe.
  kInstGroupExtMovBe,
  //! Movd.
  kInstGroupExtMovD,
  //! Movq.
  kInstGroupExtMovQ,
  //! Prefetch.
  kInstGroupExtPrefetch,

  //! 3dNow instruction.
  kInstGroup3dNow,

  //! AVX instruction without operands.
  kInstGroupAvxOp,
  //! AVX instruction encoded as 'M'.
  kInstGroupAvxM,
  //! AVX instruction encoded as 'MR'.
  kInstGroupAvxMr,
  //! AVX instruction encoded as 'MR' (Propagates AVX.L if Ymm used).
  kInstGroupAvxMr_P,
  //! AVX instruction encoded as 'MRI'.
  kInstGroupAvxMri,
  //! AVX instruction encoded as 'MRI' (Propagates AVX.L if Ymm used).
  kInstGroupAvxMri_P,
  //! AVX instruction encoded as 'RM'.
  kInstGroupAvxRm,
  //! AVX instruction encoded as 'RM' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRm_P,
  //! AVX instruction encoded as 'RMI'.
  kInstGroupAvxRmi,
  //! AVX instruction encoded as 'RMI' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRmi_P,
  //! AVX instruction encoded as 'RVM'.
  kInstGroupAvxRvm,
  //! AVX instruction encoded as 'RVM' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvm_P,
  //! AVX instruction encoded as 'RVMR'.
  kInstGroupAvxRvmr,
  //! AVX instruction encoded as 'RVMR' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvmr_P,
  //! AVX instruction encoded as 'RVMI'.
  kInstGroupAvxRvmi,
  //! AVX instruction encoded as 'RVMI' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvmi_P,
  //! AVX instruction encoded as 'RMV'.
  kInstGroupAvxRmv,
  //! AVX instruction encoded as 'RMVI'.
  kInstGroupAvxRmvi,
  //! AVX instruction encoded as 'RM' or 'MR'.
  kInstGroupAvxRmMr,
  //! AVX instruction encoded as 'RM' or 'MR' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRmMr_P,
  //! AVX instruction encoded as 'RVM' or 'RMI'.
  kInstGroupAvxRvmRmi,
  //! AVX instruction encoded as 'RVM' or 'RMI' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvmRmi_P,
  //! AVX instruction encoded as 'RVM' or 'MR'.
  kInstGroupAvxRvmMr,
  //! AVX instruction encoded as 'RVM' or 'MVR'.
  kInstGroupAvxRvmMvr,
  //! AVX instruction encoded as 'RVM' or 'MVR' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvmMvr_P,
  //! AVX instruction encoded as 'RVM' or 'VMI'.
  kInstGroupAvxRvmVmi,
  //! AVX instruction encoded as 'RVM' or 'VMI' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvmVmi_P,
  //! AVX instruction encoded as 'VM'.
  kInstGroupAvxVm,
  //! AVX instruction encoded as 'VMI'.
  kInstGroupAvxVmi,
  //! AVX instruction encoded as 'VMI' (Propagates AVX.L if Ymm used).
  kInstGroupAvxVmi_P,
  //! AVX instruction encoded as 'RVRM' or 'RVMR'.
  kInstGroupAvxRvrmRvmr,
  //! AVX instruction encoded as 'RVRM' or 'RVMR' (Propagates AVX.L if Ymm used).
  kInstGroupAvxRvrmRvmr_P,
  //! Vmovss/Vmovsd.
  kInstGroupAvxMovSsSd,
  //! AVX2 gather family instructions (VSIB).
  kInstGroupAvxGather,
  //! AVX2 gather family instructions (VSIB), differs only in mem operand.
  kInstGroupAvxGatherEx,

  //! FMA4 instruction in form [R, R, R/M, R/M].
  kInstGroupFma4,
  //! FMA4 instruction in form [R, R, R/M, R/M] (Propagates AVX.L if Ymm used).
  kInstGroupFma4_P,

  //! XOP instruction encoded as 'RM'.
  kInstGroupXopRm,
  //! XOP instruction encoded as 'RM' (Propagates AVX.L if Ymm used).
  kInstGroupXopRm_P,
  //! XOP instruction encoded as 'RVM' or 'RMV'.
  kInstGroupXopRvmRmv,
  //! XOP instruction encoded as 'RVM' or 'RMI'.
  kInstGroupXopRvmRmi,
  //! XOP instruction encoded as 'RVMR'.
  kInstGroupXopRvmr,
  //! XOP instruction encoded as 'RVMR' (Propagates AVX.L if Ymm used).
  kInstGroupXopRvmr_P,
  //! XOP instruction encoded as 'RVMI'.
  kInstGroupXopRvmi,
  //! XOP instruction encoded as 'RVMI' (Propagates AVX.L if Ymm used).
  kInstGroupXopRvmi_P,
  //! XOP instruction encoded as 'RVRM' or 'RVMR'.
  kInstGroupXopRvrmRvmr,
  //! XOP instruction encoded as 'RVRM' or 'RVMR' (Propagates AVX.L if Ymm used).
  kInstGroupXopRvrmRvmr_P
};

// ============================================================================
// [asmjit::x86x64::kInstOpCode]
// ============================================================================

//! \internal
//!
//! Instruction OpCode encoding used by asmjit 'InstInfo' table.
//!
//! The schema was izspired by AVX/AVX2 features.
ASMJIT_ENUM(kInstOpCode) {
  // 'MMMMM' field in AVX/XOP instruction.
  // 'OpCode' leading bytes in legacy encoding.
  kInstOpCode_MM_Shift = 16,
  kInstOpCode_MM_Mask  = 0x0FU << kInstOpCode_MM_Shift,
  kInstOpCode_MM_00    = 0x00U << kInstOpCode_MM_Shift,
  kInstOpCode_MM_0F    = 0x01U << kInstOpCode_MM_Shift,
  kInstOpCode_MM_0F38  = 0x02U << kInstOpCode_MM_Shift,
  kInstOpCode_MM_0F3A  = 0x03U << kInstOpCode_MM_Shift,
  kInstOpCode_MM_0F01  = 0x0FU << kInstOpCode_MM_Shift, // Ext/Not part of AVX.

  kInstOpCode_MM_00011 = 0x03U << kInstOpCode_MM_Shift,
  kInstOpCode_MM_01000 = 0x08U << kInstOpCode_MM_Shift,
  kInstOpCode_MM_01001 = 0x09U << kInstOpCode_MM_Shift,

  // 'PP' field in AVX/XOP instruction.
  // 'Mandatory Prefix' in legacy encoding.
  kInstOpCode_PP_Shift = 21,
  kInstOpCode_PP_Mask  = 0x07U << kInstOpCode_PP_Shift,
  kInstOpCode_PP_00    = 0x00U << kInstOpCode_PP_Shift,
  kInstOpCode_PP_66    = 0x01U << kInstOpCode_PP_Shift,
  kInstOpCode_PP_F3    = 0x02U << kInstOpCode_PP_Shift,
  kInstOpCode_PP_F2    = 0x03U << kInstOpCode_PP_Shift,
  kInstOpCode_PP_9B    = 0x07U << kInstOpCode_PP_Shift, // Ext/Not part of AVX.

  // 'L' field in AVX/XOP instruction.
  kInstOpCode_L_Shift  = 24,
  kInstOpCode_L_Mask   = 0x01U << kInstOpCode_L_Shift,
  kInstOpCode_L_False  = 0x00U << kInstOpCode_L_Shift,
  kInstOpCode_L_True   = 0x01U << kInstOpCode_L_Shift,

  // 'O' field.
  kInstOpCode_O_Shift  = 29,
  kInstOpCode_O_Mask   = 0x07U << kInstOpCode_O_Shift
};

// ============================================================================
// [asmjit::x86x64::kInstFlags]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction type flags.
ASMJIT_ENUM(kInstFlags) {
  //! No flags.
  kInstFlagNone = 0x0000,

  //! Instruction is a control-flow instruction.
  //!
  //! Control flow instructions are jmp, jcc, call and ret.
  kInstFlagFlow = 0x0001,

  //! Instruction is a compare/test like instruction.
  kInstFlagTest = 0x0002,

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
  kInstFlagMove = 0x0004,

  //! Instruction is an exchange like instruction.
  //!
  //! Exchange instruction typically overwrite first and second operand. So
  //! far only the instructions 'xchg' and 'xadd' are considered.
  kInstFlagXchg = 0x0008,

  //! Instruction accesses Fp register(s).
  kInstFlagFp = 0x0010,

  //! Instruction can be prefixed by using the LOCK prefix.
  kInstFlagLock = 0x0020,

  //! Instruction is special, this is for `BaseCompiler`.
  kInstFlagSpecial = 0x0040,

  //! Instruction always performs memory access.
  //!
  //! This flag is always combined with `kInstFlagSpecial` and signalizes
  //! that there is an implicit address which is accessed (usually EDI/RDI or
  //! ESI/EDI).
  kInstFlagSpecialMem = 0x0080,

  //! Instruction memory operand can refer to 16-bit address (used by FPU).
  kInstFlagMem2 = 0x0100,
  //! Instruction memory operand can refer to 32-bit address (used by FPU).
  kInstFlagMem4 = 0x0200,
  //! Instruction memory operand can refer to 64-bit address (used by FPU).
  kInstFlagMem8 = 0x0400,
  //! Instruction memory operand can refer to 80-bit address (used by FPU).
  kInstFlagMem10 = 0x0800,

  //! \internal
  //!
  //! Combination of `kInstFlagMem2` and `kInstFlagMem4`.
  kInstFlagMem2_4 = kInstFlagMem2 | kInstFlagMem4,

  //! \internal
  //!
  //! Combination of `kInstFlagMem2`, `kInstFlagMem4` and `kInstFlagMem8`.
  kInstFlagMem2_4_8 = kInstFlagMem2_4 | kInstFlagMem8,

  //! \internal
  //!
  //! Combination of `kInstFlagMem4` and `kInstFlagMem8`.
  kInstFlagMem4_8 = kInstFlagMem4 | kInstFlagMem8,

  //! \internal
  //!
  //! Combination of `kInstFlagMem4`, `kInstFlagMem8` and `kInstFlagMem10`.
  kInstFlagMem4_8_10 = kInstFlagMem4_8 | kInstFlagMem10,

  //! Zeroes the rest of the register if the source operand is memory.
  kInstFlagZeroIfMem = 0x1000,

  //! REX.W/VEX.W by default.
  kInstFlagW = 0x8000
};

// ============================================================================
// [asmjit::x86x64::kInstOp]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction operand flags.
ASMJIT_ENUM(kInstOp) {
  //! Instruction operand can be 8-bit Gpb register.
  kInstOpGb = 0x0001,
  //! Instruction operand can be 16-bit Gpw register.
  kInstOpGw = 0x0002,
  //! Instruction operand can be 32-bit Gpd register.
  kInstOpGd = 0x0004,
  //! Instruction operand can be 64-bit Gpq register.
  kInstOpGq = 0x0008,
  //! Instruction operand can be Fp register.
  kInstOpFp = 0x0010,
  //! Instruction operand can be 64-bit Mmx register.
  kInstOpMm = 0x0020,
  //! Instruction operand can be 128-bit Xmm register.
  kInstOpXmm = 0x0100,
  //! Instruction operand can be 256-bit Ymm register.
  kInstOpYmm = 0x0200,
  //! Instruction operand can be 512-bit Zmm register.
  kInstOpZmm = 0x0400,

  //! Instruction operand can be memory.
  kInstOpMem = 0x2000,
  //! Instruction operand can be immediate.
  kInstOpImm = 0x4000,
  //! Instruction operand can be label.
  kInstOpLabel = 0x8000,

  //! \internal
  //!
  //! Combined flags.
  //!
  //! \{

  kInstOpGwb           = kInstOpGw     | kInstOpGb,
  kInstOpGqd           = kInstOpGq     | kInstOpGd,
  kInstOpGqdw          = kInstOpGq     | kInstOpGd | kInstOpGw,
  kInstOpGqdwb         = kInstOpGq     | kInstOpGd | kInstOpGw | kInstOpGb,

  kInstOpGbMem         = kInstOpGb     | kInstOpMem,
  kInstOpGwMem         = kInstOpGw     | kInstOpMem,
  kInstOpGdMem         = kInstOpGd     | kInstOpMem,
  kInstOpGqMem         = kInstOpGq     | kInstOpMem,
  kInstOpGwbMem        = kInstOpGwb    | kInstOpMem,
  kInstOpGqdMem        = kInstOpGqd    | kInstOpMem,
  kInstOpGqdwMem       = kInstOpGqdw   | kInstOpMem,
  kInstOpGqdwbMem      = kInstOpGqdwb  | kInstOpMem,

  kInstOpFpMem         = kInstOpFp     | kInstOpMem,
  kInstOpMmMem         = kInstOpMm     | kInstOpMem,
  kInstOpXmmMem        = kInstOpXmm    | kInstOpMem,
  kInstOpYmmMem        = kInstOpYmm    | kInstOpMem,

  kInstOpMmXmm         = kInstOpMm     | kInstOpXmm,
  kInstOpMmXmmMem      = kInstOpMmXmm  | kInstOpMem,

  kInstOpXmmYmm        = kInstOpXmm    | kInstOpYmm,
  kInstOpXmmYmmMem     = kInstOpXmmYmm | kInstOpMem

  //! \}
};

// ============================================================================
// [asmjit::x86x64::kCond]
// ============================================================================

//! X86/X64 Condition codes.
ASMJIT_ENUM(kCond) {
  kCondA               = 0x07, // CF==0 & ZF==0          (unsigned)
  kCondAE              = 0x03, // CF==0                  (unsigned)
  kCondB               = 0x02, // CF==1                  (unsigned)
  kCondBE              = 0x06, // CF==1 | ZF==1          (unsigned)
  kCondC               = 0x02, // CF==1
  kCondE               = 0x04, //         ZF==1          (signed/unsigned)
  kCondG               = 0x0F, //         ZF==0 & SF==OF (signed)
  kCondGE              = 0x0D, //                 SF==OF (signed)
  kCondL               = 0x0C, //                 SF!=OF (signed)
  kCondLE              = 0x0E, //         ZF==1 | SF!=OF (signed)
  kCondNA              = 0x06, // CF==1 | ZF==1          (unsigned)
  kCondNAE             = 0x02, // CF==1                  (unsigned)
  kCondNB              = 0x03, // CF==0                  (unsigned)
  kCondNBE             = 0x07, // CF==0 & ZF==0          (unsigned)
  kCondNC              = 0x03, // CF==0
  kCondNE              = 0x05, //         ZF==0          (signed/unsigned)
  kCondNG              = 0x0E, //         ZF==1 | SF!=OF (signed)
  kCondNGE             = 0x0C, //                 SF!=OF (signed)
  kCondNL              = 0x0D, //                 SF==OF (signed)
  kCondNLE             = 0x0F, //         ZF==0 & SF==OF (signed)
  kCondNO              = 0x01, //                 OF==0
  kCondNP              = 0x0B, // PF==0
  kCondNS              = 0x09, //                 SF==0
  kCondNZ              = 0x05, //         ZF==0
  kCondO               = 0x00, //                 OF==1
  kCondP               = 0x0A, // PF==1
  kCondPE              = 0x0A, // PF==1
  kCondPO              = 0x0B, // PF==0
  kCondS               = 0x08, //                 SF==1
  kCondZ               = 0x04, //         ZF==1

  // Simplified condition codes.
  kCondOverflow        = 0x00,
  kCondNotOverflow     = 0x01,
  kCondBelow           = 0x02, //!< Unsigned comparison.
  kCondAboveEqual      = 0x03, //!< Unsigned comparison.
  kCondEqual           = 0x04,
  kCondNotEqual        = 0x05,
  kCondBelowEqual      = 0x06, //!< Unsigned comparison.
  kCondAbove           = 0x07, //!< Unsigned comparison.
  kCondSign            = 0x08,
  kCondNotSign         = 0x09,
  kCondParityEven      = 0x0A,
  kCondParityOdd       = 0x0B,
  kCondLess            = 0x0C, //!< Signed comparison.
  kCondGreaterEqual    = 0x0D, //!< Signed comparison.
  kCondLessEqual       = 0x0E, //!< Signed comparison.
  kCondGreater         = 0x0F, //!< Signed comparison.

  // Aliases.
  kCondZero            = 0x04,
  kCondNotZero         = 0x05,
  kCondNegative        = 0x08,
  kCondPositive        = 0x09,

  // Fpu-only.
  kCondFpuUnordered    = 0x10,
  kCondFpuNotUnordered = 0x11,

  //! No condition code.
  kCondNone            = 0x12
};

// ============================================================================
// [asmjit::x86x64::kPrefetchHint]
// ============================================================================

//! X86/X64 Prefetch hints.
ASMJIT_ENUM(kPrefetchHint) {
  //! Prefetch using NT hint.
  kPrefetchNta = 0,
  //! Prefetch to L0 cache.
  kPrefetchT0 = 1,
  //! Prefetch to L1 cache.
  kPrefetchT1 = 2,
  //! Prefetch to L2 cache.
  kPrefetchT2 = 3
};

// ============================================================================
// [asmjit::x86x64::kFPSW]
// ============================================================================

//! X86/X64 FPU status Word.
ASMJIT_ENUM(kFPSW) {
  kFPSW_Invalid        = 0x0001,
  kFPSW_Denormalized   = 0x0002,
  kFPSW_DivByZero      = 0x0004,
  kFPSW_Overflow       = 0x0008,
  kFPSW_Underflow      = 0x0010,
  kFPSW_Precision      = 0x0020,
  kFPSW_StackFault     = 0x0040,
  kFPSW_Interrupt      = 0x0080,
  kFPSW_C0             = 0x0100,
  kFPSW_C1             = 0x0200,
  kFPSW_C2             = 0x0400,
  kFPSW_Top            = 0x3800,
  kFPSW_C3             = 0x4000,
  kFPSW_Busy           = 0x8000
};

// ============================================================================
// [asmjit::x86x64::kFPCW]
// ============================================================================

//! X86/X64 FPU control Word.
ASMJIT_ENUM(kFPCW) {
  kFPCW_EM_Mask        = 0x003F, // Bits 0-5.
  kFPCW_EM_Invalid     = 0x0001,
  kFPCW_EM_Denormal    = 0x0002,
  kFPCW_EM_DivByZero   = 0x0004,
  kFPCW_EM_Overflow    = 0x0008,
  kFPCW_EM_Underflow   = 0x0010,
  kFPCW_EM_Inexact     = 0x0020,

  kFPCW_PC_Mask        = 0x0300, // Bits 8-9.
  kFPCW_PC_Float       = 0x0000,
  kFPCW_PC_Reserved    = 0x0100,
  kFPCW_PC_Double      = 0x0200,
  kFPCW_PC_Extended    = 0x0300,

  kFPCW_RC_Mask        = 0x0C00, // Bits 10-11.
  kFPCW_RC_Nearest     = 0x0000,
  kFPCW_RC_Down        = 0x0400,
  kFPCW_RC_Up          = 0x0800,
  kFPCW_RC_Truncate    = 0x0C00,

  kFPCW_IC_Mask        = 0x1000, // Bit 12.
  kFPCW_IC_Projective  = 0x0000,
  kFPCW_IC_Affine      = 0x1000
};

// ============================================================================
// [asmjit::x86x64::InstInfo]
// ============================================================================

//! \internal
//!
//! X86/X64 instruction information.
struct InstInfo {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_DISABLE_INST_NAMES)
  //! Get instruction name string (null terminated).
  ASMJIT_INLINE const char* getName() const {
    return _instName + static_cast<uint32_t>(_nameIndex);
  }

  //! Get instruction name index to `_instName` array.
  ASMJIT_INLINE uint32_t _getNameIndex() const {
    return _nameIndex;
  }
#endif // !ASMJIT_DISABLE_INST_NAMES

  //! Get instruction group, see `kInstGroup`.
  ASMJIT_INLINE uint32_t getGroup() const {
    return _group;
  }

  //! Get size of move instruction in bytes.
  //!
  //! If zero, the size of MOV instruction is determined by the size of the
  //! destination register (applies mostly for x86 arithmetic). This value is
  //! useful for register allocator when determining if a variable is going to
  //! be overwritten or not. Basically if the move size is equal or greater
  //! than a variable itself it is considered overwritten.
  ASMJIT_INLINE uint32_t getMoveSize() const {
    return _moveSize;
  }

  //! Get instruction flags, see `kInstFlags`.
  ASMJIT_INLINE uint32_t getFlags() const {
    return _flags;
  }

  //! Get whether the instruction is a control-flow intruction.
  //!
  //! Control flow instruction is instruction that modifies instruction pointer,
  //! typically jmp, jcc, call, or ret.
  ASMJIT_INLINE bool isFlow() const {
    return (_flags & kInstFlagFlow) != 0;
  }

  //! Get whether the instruction is a compare/test like intruction.
  ASMJIT_INLINE bool isTest() const {
    return (_flags & kInstFlagTest) != 0;
  }

  //! Get whether the instruction is a typical move instruction.
  //!
  //! Move instructions overwrite the first operand or at least part of it,
  //! This is a very useful hint that is used by variable liveness analysis
  //! and `BaseCompiler` in general to know which variable is completely
  //! overwritten.
  //!
  //! All AVX/XOP instructions that have 3 or more operands are considered to
  //! have move semantics move by default.
  ASMJIT_INLINE bool isMove() const {
    return (_flags & kInstFlagMove) != 0;
  }

  //! Get whether the instruction is a typical Exchange instruction.
  //!
  //! Exchange instructios are 'xchg' and 'xadd'.
  ASMJIT_INLINE bool isXchg() const {
    return (_flags & kInstFlagXchg) != 0;
  }

  //! Get whether the instruction accesses Fp register(s).
  ASMJIT_INLINE bool isFp() const {
    return (_flags & kInstFlagFp) != 0;
  }

  //! Get whether the instruction can be prefixed by LOCK prefix.
  ASMJIT_INLINE bool isLockable() const {
    return (_flags & kInstFlagLock) != 0;
  }

  //! Get whether the instruction is special type (this is used by
  //! `BaseCompiler` to manage additional variables or functionality).
  ASMJIT_INLINE bool isSpecial() const {
    return (_flags & kInstFlagSpecial) != 0;
  }

  //! Get whether the instruction is special type and it performs
  //! memory access.
  ASMJIT_INLINE bool isSpecialMem() const {
    return (_flags & kInstFlagSpecialMem) != 0;
  }

  //! Get whether the move instruction zeroes the rest of the register
  //! if the source is memory operand.
  //!
  //! Basically flag needed only to support `movsd` and `movss` instructions.
  ASMJIT_INLINE bool isZeroIfMem() const {
    return (_flags & kInstFlagZeroIfMem) != 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Instruction name index in _instName[] array.
  uint16_t _nameIndex;
  //! Instruction flags.
  uint16_t _flags;
  //! Instruction group, used by `BaseAssembler`.
  uint8_t _group;
  //! Count of bytes overwritten by a move instruction.
  //!
  //! Only used with `kInstFlagMove` flag. If this value is zero move depends
  //! on the destination register size.
  uint8_t _moveSize;
  //! Reserved for future use.
  uint8_t _reserved[2];
  //! Operands' flags.
  uint16_t _opFlags[4];
  //! Primary and secondary opcodes.
  uint32_t _opCode[2];
};

// ============================================================================
// [asmjit::x86x64::X86InstUtil]
// ============================================================================

struct X86InstUtil {
#if !defined(ASMJIT_DISABLE_INST_NAMES)
  //! Get an instruction ID from a given instruction `name`.
  //!
  //! If there is an exact match the instruction id is returned, otherwise
  //! `kInstNone` (zero) is returned.
  //!
  //! The given `name` doesn't have to be null-terminated if `len` is provided.
  ASMJIT_API static uint32_t getInstIdByName(
    const char* name, size_t len = kInvalidIndex);
#endif // !ASMJIT_DISABLE_INST_NAMES
};

//! \}

} // x64 namespace
} // asmjit namespace

#undef _OP_ID

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86INST_H
