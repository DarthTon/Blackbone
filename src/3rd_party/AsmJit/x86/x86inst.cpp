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
#include "../x86/x86inst.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Enums (Internal)]
// ============================================================================

//! \internal
enum {
  kX86InstTable_L__ = (0) << kX86InstOpCode_L_Shift,
  kX86InstTable_L_I = (0) << kX86InstOpCode_L_Shift,
  kX86InstTable_L_0 = (0) << kX86InstOpCode_L_Shift,
  kX86InstTable_L_L = (1) << kX86InstOpCode_L_Shift,

  kX86InstTable_W__ = (0) << kX86InstOpCode_W_Shift,
  kX86InstTable_W_I = (0) << kX86InstOpCode_W_Shift,
  kX86InstTable_W_0 = (0) << kX86InstOpCode_W_Shift,
  kX86InstTable_W_1 = (1) << kX86InstOpCode_W_Shift,
  kX86InstTable_W_W = (1) << kX86InstOpCode_W_Shift,

  kX86InstTable_E__ = (0) << kX86InstOpCode_EW_Shift,
  kX86InstTable_E_I = (0) << kX86InstOpCode_EW_Shift,
  kX86InstTable_E_0 = (0) << kX86InstOpCode_EW_Shift,
  kX86InstTable_E_1 = (1) << kX86InstOpCode_EW_Shift
};

//! \internal
//!
//! Combined flags.
enum kX86InstOpInternal {
  kX86InstOpI        = kX86InstOpImm,
  kX86InstOpL        = kX86InstOpLabel,
  kX86InstOpLbImm    = kX86InstOpLabel | kX86InstOpImm,

  kX86InstOpGwb      = kX86InstOpGw    | kX86InstOpGb,
  kX86InstOpGqd      = kX86InstOpGq    | kX86InstOpGd,
  kX86InstOpGqdw     = kX86InstOpGq    | kX86InstOpGd | kX86InstOpGw,
  kX86InstOpGqdwb    = kX86InstOpGq    | kX86InstOpGd | kX86InstOpGw | kX86InstOpGb,

  kX86InstOpGbMem    = kX86InstOpGb    | kX86InstOpMem,
  kX86InstOpGwMem    = kX86InstOpGw    | kX86InstOpMem,
  kX86InstOpGdMem    = kX86InstOpGd    | kX86InstOpMem,
  kX86InstOpGqMem    = kX86InstOpGq    | kX86InstOpMem,
  kX86InstOpGwbMem   = kX86InstOpGwb   | kX86InstOpMem,
  kX86InstOpGqdMem   = kX86InstOpGqd   | kX86InstOpMem,
  kX86InstOpGqdwMem  = kX86InstOpGqdw  | kX86InstOpMem,
  kX86InstOpGqdwbMem = kX86InstOpGqdwb | kX86InstOpMem,

  kX86InstOpFpMem    = kX86InstOpFp    | kX86InstOpMem,
  kX86InstOpMmMem    = kX86InstOpMm    | kX86InstOpMem,
  kX86InstOpKMem     = kX86InstOpK     | kX86InstOpMem,
  kX86InstOpXmmMem   = kX86InstOpXmm   | kX86InstOpMem,
  kX86InstOpYmmMem   = kX86InstOpYmm   | kX86InstOpMem,
  kX86InstOpZmmMem   = kX86InstOpZmm   | kX86InstOpMem,

  kX86InstOpMmXmm    = kX86InstOpMm    | kX86InstOpXmm,
  kX86InstOpMmXmmMem = kX86InstOpMmXmm | kX86InstOpMem,

  kX86InstOpXy       = kX86InstOpXmm   | kX86InstOpYmm,
  kX86InstOpXyMem    = kX86InstOpXy    | kX86InstOpMem,

  kX86InstOpXyz      = kX86InstOpXy    | kX86InstOpZmm,
  kX86InstOpXyzMem   = kX86InstOpXyz   | kX86InstOpMem
};

//! \internal
//!
//! X86/X64 Instruction AVX-512 flags (combined).
ASMJIT_ENUM(kX86InstFlagsInternal) {
  // FPU.
  kX86InstFlagMem2_4           = kX86InstFlagMem2        | kX86InstFlagMem4,
  kX86InstFlagMem2_4_8         = kX86InstFlagMem2_4      | kX86InstFlagMem8,
  kX86InstFlagMem4_8           = kX86InstFlagMem4        | kX86InstFlagMem8,
  kX86InstFlagMem4_8_10        = kX86InstFlagMem4_8      | kX86InstFlagMem10
};

// ============================================================================
// [Macros]
// ============================================================================

#if !defined(ASMJIT_DISABLE_NAMES)
# define INST_NAME_INDEX(_Code_) _Code_##_NameIndex
#else
# define INST_NAME_INDEX(_Code_) 0
#endif

// Undefined. Used to distinguish between zero and field that is not used.
#define U 0

// Instruction opcodes.
#define O_000000(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_00   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_000F00(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_0F   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_000F01(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_0F01 | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_000F0F(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_0F   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_000F38(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_0F38 | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_000F3A(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_0F3A | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_660000(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_66 | kX86InstOpCode_MM_00   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_660F00(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_66 | kX86InstOpCode_MM_0F   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_660F38(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_66 | kX86InstOpCode_MM_0F38 | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_660F3A(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_66 | kX86InstOpCode_MM_0F3A | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_9B0000(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_9B | kX86InstOpCode_MM_00   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F20000(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F2 | kX86InstOpCode_MM_00   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F20F00(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F2 | kX86InstOpCode_MM_0F   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F20F38(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F2 | kX86InstOpCode_MM_0F38 | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F20F3A(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F2 | kX86InstOpCode_MM_0F3A | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F30000(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F3 | kX86InstOpCode_MM_00   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F30F00(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F3 | kX86InstOpCode_MM_0F   | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F30F38(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F3 | kX86InstOpCode_MM_0F38 | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_F30F3A(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_F3 | kX86InstOpCode_MM_0F3A | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)

#define O_00_M08(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_01000| (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_00_M09(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_00 | kX86InstOpCode_MM_01001| (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)
#define O_66_M03(_OpCode_, _O_, _L_, _W_, _EVEX_W_) (kX86InstOpCode_PP_66 | kX86InstOpCode_MM_00011| (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift) | kX86InstTable_L_##_L_ | kX86InstTable_W_##_W_ | kX86InstTable_E_##_EVEX_W_)

#define O_00_X(_OpCode_, _O_) (kX86InstOpCode_PP_00 | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift))
#define O_9B_X(_OpCode_, _O_) (kX86InstOpCode_PP_9B | (0x##_OpCode_) | ((_O_) << kX86InstOpCode_O_Shift))

// Instruction Encoding `Enc(...)`.
#define Enc(_Id_) kX86InstEncodingId##_Id_

// Instruction Flags `F(...)` and AVX-512 `AVX(...)`flags.
#define F(_Flags_) kX86InstFlag##_Flags_

#define A(_Flags_) kX86InstFlagAvx512##_Flags_

// Instruction EFLAGS `E(OSZAPCDX)`.
#define EF(_Flags_) 0

// Instruction Operands' Flags `O(...)`.
#define O(_Op_) kX86InstOp##_Op_

// Defines an X86/X64 instruction.
#define INST(_Id_, _Name_, _OpCode0_, _OpCode1_, _Encoding_, _IFlags_, _EFlags_, _WriteIndex_, _WriteSize_, _Op0_, _Op_, _Op2_, _Op3_, _Op4_) \
  { INST_NAME_INDEX(_Id_), _Id_##_ExtendedIndex, _OpCode0_ }

// ============================================================================
// [asmjit::X86Inst]
// ============================================================================

// ${X86InstData:Begin}
// Automatically generated, do not edit.
#if !defined(ASMJIT_DISABLE_NAMES)
const char _x86InstName[] =
  "\0"
  "adc\0"
  "add\0"
  "addpd\0"
  "addps\0"
  "addsd\0"
  "addss\0"
  "addsubpd\0"
  "addsubps\0"
  "aesdec\0"
  "aesdeclast\0"
  "aesenc\0"
  "aesenclast\0"
  "aesimc\0"
  "aeskeygenassist\0"
  "and\0"
  "andn\0"
  "andnpd\0"
  "andnps\0"
  "andpd\0"
  "andps\0"
  "bextr\0"
  "blendpd\0"
  "blendps\0"
  "blendvpd\0"
  "blendvps\0"
  "blsi\0"
  "blsmsk\0"
  "blsr\0"
  "bsf\0"
  "bsr\0"
  "bswap\0"
  "bt\0"
  "btc\0"
  "btr\0"
  "bts\0"
  "bzhi\0"
  "call\0"
  "cbw\0"
  "cdq\0"
  "cdqe\0"
  "clc\0"
  "cld\0"
  "clflush\0"
  "cmc\0"
  "cmova\0"
  "cmovae\0"
  "cmovb\0"
  "cmovbe\0"
  "cmovc\0"
  "cmove\0"
  "cmovg\0"
  "cmovge\0"
  "cmovl\0"
  "cmovle\0"
  "cmovna\0"
  "cmovnae\0"
  "cmovnb\0"
  "cmovnbe\0"
  "cmovnc\0"
  "cmovne\0"
  "cmovng\0"
  "cmovnge\0"
  "cmovnl\0"
  "cmovnle\0"
  "cmovno\0"
  "cmovnp\0"
  "cmovns\0"
  "cmovnz\0"
  "cmovo\0"
  "cmovp\0"
  "cmovpe\0"
  "cmovpo\0"
  "cmovs\0"
  "cmovz\0"
  "cmp\0"
  "cmppd\0"
  "cmpps\0"
  "cmps_b\0"
  "cmps_d\0"
  "cmps_q\0"
  "cmps_w\0"
  "cmpsd\0"
  "cmpss\0"
  "cmpxchg\0"
  "cmpxchg16b\0"
  "cmpxchg8b\0"
  "comisd\0"
  "comiss\0"
  "cpuid\0"
  "cqo\0"
  "crc32\0"
  "cvtdq2pd\0"
  "cvtdq2ps\0"
  "cvtpd2dq\0"
  "cvtpd2pi\0"
  "cvtpd2ps\0"
  "cvtpi2pd\0"
  "cvtpi2ps\0"
  "cvtps2dq\0"
  "cvtps2pd\0"
  "cvtps2pi\0"
  "cvtsd2si\0"
  "cvtsd2ss\0"
  "cvtsi2sd\0"
  "cvtsi2ss\0"
  "cvtss2sd\0"
  "cvtss2si\0"
  "cvttpd2dq\0"
  "cvttpd2pi\0"
  "cvttps2dq\0"
  "cvttps2pi\0"
  "cvttsd2si\0"
  "cvttss2si\0"
  "cwd\0"
  "cwde\0"
  "daa\0"
  "das\0"
  "dec\0"
  "div\0"
  "divpd\0"
  "divps\0"
  "divsd\0"
  "divss\0"
  "dppd\0"
  "dpps\0"
  "emms\0"
  "enter\0"
  "extractps\0"
  "extrq\0"
  "f2xm1\0"
  "fabs\0"
  "fadd\0"
  "faddp\0"
  "fbld\0"
  "fbstp\0"
  "fchs\0"
  "fclex\0"
  "fcmovb\0"
  "fcmovbe\0"
  "fcmove\0"
  "fcmovnb\0"
  "fcmovnbe\0"
  "fcmovne\0"
  "fcmovnu\0"
  "fcmovu\0"
  "fcom\0"
  "fcomi\0"
  "fcomip\0"
  "fcomp\0"
  "fcompp\0"
  "fcos\0"
  "fdecstp\0"
  "fdiv\0"
  "fdivp\0"
  "fdivr\0"
  "fdivrp\0"
  "femms\0"
  "ffree\0"
  "fiadd\0"
  "ficom\0"
  "ficomp\0"
  "fidiv\0"
  "fidivr\0"
  "fild\0"
  "fimul\0"
  "fincstp\0"
  "finit\0"
  "fist\0"
  "fistp\0"
  "fisttp\0"
  "fisub\0"
  "fisubr\0"
  "fld\0"
  "fld1\0"
  "fldcw\0"
  "fldenv\0"
  "fldl2e\0"
  "fldl2t\0"
  "fldlg2\0"
  "fldln2\0"
  "fldpi\0"
  "fldz\0"
  "fmul\0"
  "fmulp\0"
  "fnclex\0"
  "fninit\0"
  "fnop\0"
  "fnsave\0"
  "fnstcw\0"
  "fnstenv\0"
  "fnstsw\0"
  "fpatan\0"
  "fprem\0"
  "fprem1\0"
  "fptan\0"
  "frndint\0"
  "frstor\0"
  "fsave\0"
  "fscale\0"
  "fsin\0"
  "fsincos\0"
  "fsqrt\0"
  "fst\0"
  "fstcw\0"
  "fstenv\0"
  "fstp\0"
  "fstsw\0"
  "fsub\0"
  "fsubp\0"
  "fsubr\0"
  "fsubrp\0"
  "ftst\0"
  "fucom\0"
  "fucomi\0"
  "fucomip\0"
  "fucomp\0"
  "fucompp\0"
  "fwait\0"
  "fxam\0"
  "fxch\0"
  "fxrstor\0"
  "fxsave\0"
  "fxtract\0"
  "fyl2x\0"
  "fyl2xp1\0"
  "haddpd\0"
  "haddps\0"
  "hsubpd\0"
  "hsubps\0"
  "idiv\0"
  "imul\0"
  "inc\0"
  "insertps\0"
  "insertq\0"
  "int\0"
  "ja\0"
  "jae\0"
  "jb\0"
  "jbe\0"
  "jc\0"
  "je\0"
  "jg\0"
  "jge\0"
  "jl\0"
  "jle\0"
  "jna\0"
  "jnae\0"
  "jnb\0"
  "jnbe\0"
  "jnc\0"
  "jne\0"
  "jng\0"
  "jnge\0"
  "jnl\0"
  "jnle\0"
  "jno\0"
  "jnp\0"
  "jns\0"
  "jnz\0"
  "jo\0"
  "jp\0"
  "jpe\0"
  "jpo\0"
  "js\0"
  "jz\0"
  "jecxz\0"
  "jmp\0"
  "lahf\0"
  "lddqu\0"
  "ldmxcsr\0"
  "lea\0"
  "leave\0"
  "lfence\0"
  "lods_b\0"
  "lods_d\0"
  "lods_q\0"
  "lods_w\0"
  "lzcnt\0"
  "maskmovdqu\0"
  "maskmovq\0"
  "maxpd\0"
  "maxps\0"
  "maxsd\0"
  "maxss\0"
  "mfence\0"
  "minpd\0"
  "minps\0"
  "minsd\0"
  "minss\0"
  "monitor\0"
  "mov\0"
  "mov_ptr\0"
  "movapd\0"
  "movaps\0"
  "movbe\0"
  "movd\0"
  "movddup\0"
  "movdq2q\0"
  "movdqa\0"
  "movdqu\0"
  "movhlps\0"
  "movhpd\0"
  "movhps\0"
  "movlhps\0"
  "movlpd\0"
  "movlps\0"
  "movmskpd\0"
  "movmskps\0"
  "movntdq\0"
  "movntdqa\0"
  "movnti\0"
  "movntpd\0"
  "movntps\0"
  "movntq\0"
  "movntsd\0"
  "movntss\0"
  "movq\0"
  "movq2dq\0"
  "movs_b\0"
  "movs_d\0"
  "movs_q\0"
  "movs_w\0"
  "movsd\0"
  "movshdup\0"
  "movsldup\0"
  "movss\0"
  "movsx\0"
  "movsxd\0"
  "movupd\0"
  "movups\0"
  "movzx\0"
  "mpsadbw\0"
  "mul\0"
  "mulpd\0"
  "mulps\0"
  "mulsd\0"
  "mulss\0"
  "mulx\0"
  "mwait\0"
  "neg\0"
  "nop\0"
  "not\0"
  "or\0"
  "orpd\0"
  "orps\0"
  "pabsb\0"
  "pabsd\0"
  "pabsw\0"
  "packssdw\0"
  "packsswb\0"
  "packusdw\0"
  "packuswb\0"
  "paddb\0"
  "paddd\0"
  "paddq\0"
  "paddsb\0"
  "paddsw\0"
  "paddusb\0"
  "paddusw\0"
  "paddw\0"
  "palignr\0"
  "pand\0"
  "pandn\0"
  "pause\0"
  "pavgb\0"
  "pavgw\0"
  "pblendvb\0"
  "pblendw\0"
  "pclmulqdq\0"
  "pcmpeqb\0"
  "pcmpeqd\0"
  "pcmpeqq\0"
  "pcmpeqw\0"
  "pcmpestri\0"
  "pcmpestrm\0"
  "pcmpgtb\0"
  "pcmpgtd\0"
  "pcmpgtq\0"
  "pcmpgtw\0"
  "pcmpistri\0"
  "pcmpistrm\0"
  "pdep\0"
  "pext\0"
  "pextrb\0"
  "pextrd\0"
  "pextrq\0"
  "pextrw\0"
  "pf2id\0"
  "pf2iw\0"
  "pfacc\0"
  "pfadd\0"
  "pfcmpeq\0"
  "pfcmpge\0"
  "pfcmpgt\0"
  "pfmax\0"
  "pfmin\0"
  "pfmul\0"
  "pfnacc\0"
  "pfpnacc\0"
  "pfrcp\0"
  "pfrcpit1\0"
  "pfrcpit2\0"
  "pfrsqit1\0"
  "pfrsqrt\0"
  "pfsub\0"
  "pfsubr\0"
  "phaddd\0"
  "phaddsw\0"
  "phaddw\0"
  "phminposuw\0"
  "phsubd\0"
  "phsubsw\0"
  "phsubw\0"
  "pi2fd\0"
  "pi2fw\0"
  "pinsrb\0"
  "pinsrd\0"
  "pinsrq\0"
  "pinsrw\0"
  "pmaddubsw\0"
  "pmaddwd\0"
  "pmaxsb\0"
  "pmaxsd\0"
  "pmaxsw\0"
  "pmaxub\0"
  "pmaxud\0"
  "pmaxuw\0"
  "pminsb\0"
  "pminsd\0"
  "pminsw\0"
  "pminub\0"
  "pminud\0"
  "pminuw\0"
  "pmovmskb\0"
  "pmovsxbd\0"
  "pmovsxbq\0"
  "pmovsxbw\0"
  "pmovsxdq\0"
  "pmovsxwd\0"
  "pmovsxwq\0"
  "pmovzxbd\0"
  "pmovzxbq\0"
  "pmovzxbw\0"
  "pmovzxdq\0"
  "pmovzxwd\0"
  "pmovzxwq\0"
  "pmuldq\0"
  "pmulhrsw\0"
  "pmulhuw\0"
  "pmulhw\0"
  "pmulld\0"
  "pmullw\0"
  "pmuludq\0"
  "pop\0"
  "popa\0"
  "popcnt\0"
  "popf\0"
  "por\0"
  "prefetch\0"
  "prefetch_3dnow\0"
  "prefetchw_3dnow\0"
  "psadbw\0"
  "pshufb\0"
  "pshufd\0"
  "pshufhw\0"
  "pshuflw\0"
  "pshufw\0"
  "psignb\0"
  "psignd\0"
  "psignw\0"
  "pslld\0"
  "pslldq\0"
  "psllq\0"
  "psllw\0"
  "psrad\0"
  "psraw\0"
  "psrld\0"
  "psrldq\0"
  "psrlq\0"
  "psrlw\0"
  "psubb\0"
  "psubd\0"
  "psubq\0"
  "psubsb\0"
  "psubsw\0"
  "psubusb\0"
  "psubusw\0"
  "psubw\0"
  "pswapd\0"
  "ptest\0"
  "punpckhbw\0"
  "punpckhdq\0"
  "punpckhqdq\0"
  "punpckhwd\0"
  "punpcklbw\0"
  "punpckldq\0"
  "punpcklqdq\0"
  "punpcklwd\0"
  "push\0"
  "pusha\0"
  "pushf\0"
  "pxor\0"
  "rcl\0"
  "rcpps\0"
  "rcpss\0"
  "rcr\0"
  "rdfsbase\0"
  "rdgsbase\0"
  "rdrand\0"
  "rdtsc\0"
  "rdtscp\0"
  "rep lods_b\0"
  "rep lods_d\0"
  "rep lods_q\0"
  "rep lods_w\0"
  "rep movs_b\0"
  "rep movs_d\0"
  "rep movs_q\0"
  "rep movs_w\0"
  "rep stos_b\0"
  "rep stos_d\0"
  "rep stos_q\0"
  "rep stos_w\0"
  "repe cmps_b\0"
  "repe cmps_d\0"
  "repe cmps_q\0"
  "repe cmps_w\0"
  "repe scas_b\0"
  "repe scas_d\0"
  "repe scas_q\0"
  "repe scas_w\0"
  "repne cmps_b\0"
  "repne cmps_d\0"
  "repne cmps_q\0"
  "repne cmps_w\0"
  "repne scas_b\0"
  "repne scas_d\0"
  "repne scas_q\0"
  "repne scas_w\0"
  "ret\0"
  "rol\0"
  "ror\0"
  "rorx\0"
  "roundpd\0"
  "roundps\0"
  "roundsd\0"
  "roundss\0"
  "rsqrtps\0"
  "rsqrtss\0"
  "sahf\0"
  "sal\0"
  "sar\0"
  "sarx\0"
  "sbb\0"
  "scas_b\0"
  "scas_d\0"
  "scas_q\0"
  "scas_w\0"
  "seta\0"
  "setae\0"
  "setb\0"
  "setbe\0"
  "setc\0"
  "sete\0"
  "setg\0"
  "setge\0"
  "setl\0"
  "setle\0"
  "setna\0"
  "setnae\0"
  "setnb\0"
  "setnbe\0"
  "setnc\0"
  "setne\0"
  "setng\0"
  "setnge\0"
  "setnl\0"
  "setnle\0"
  "setno\0"
  "setnp\0"
  "setns\0"
  "setnz\0"
  "seto\0"
  "setp\0"
  "setpe\0"
  "setpo\0"
  "sets\0"
  "setz\0"
  "sfence\0"
  "shl\0"
  "shld\0"
  "shlx\0"
  "shr\0"
  "shrd\0"
  "shrx\0"
  "shufpd\0"
  "shufps\0"
  "sqrtpd\0"
  "sqrtps\0"
  "sqrtsd\0"
  "sqrtss\0"
  "stc\0"
  "std\0"
  "stmxcsr\0"
  "stos_b\0"
  "stos_d\0"
  "stos_q\0"
  "stos_w\0"
  "sub\0"
  "subpd\0"
  "subps\0"
  "subsd\0"
  "subss\0"
  "test\0"
  "tzcnt\0"
  "ucomisd\0"
  "ucomiss\0"
  "ud2\0"
  "unpckhpd\0"
  "unpckhps\0"
  "unpcklpd\0"
  "unpcklps\0"
  "vaddpd\0"
  "vaddps\0"
  "vaddsd\0"
  "vaddss\0"
  "vaddsubpd\0"
  "vaddsubps\0"
  "vaesdec\0"
  "vaesdeclast\0"
  "vaesenc\0"
  "vaesenclast\0"
  "vaesimc\0"
  "vaeskeygenassist\0"
  "vandnpd\0"
  "vandnps\0"
  "vandpd\0"
  "vandps\0"
  "vblendpd\0"
  "vblendps\0"
  "vblendvpd\0"
  "vblendvps\0"
  "vbroadcastf128\0"
  "vbroadcasti128\0"
  "vbroadcastsd\0"
  "vbroadcastss\0"
  "vcmppd\0"
  "vcmpps\0"
  "vcmpsd\0"
  "vcmpss\0"
  "vcomisd\0"
  "vcomiss\0"
  "vcvtdq2pd\0"
  "vcvtdq2ps\0"
  "vcvtpd2dq\0"
  "vcvtpd2ps\0"
  "vcvtph2ps\0"
  "vcvtps2dq\0"
  "vcvtps2pd\0"
  "vcvtps2ph\0"
  "vcvtsd2si\0"
  "vcvtsd2ss\0"
  "vcvtsi2sd\0"
  "vcvtsi2ss\0"
  "vcvtss2sd\0"
  "vcvtss2si\0"
  "vcvttpd2dq\0"
  "vcvttps2dq\0"
  "vcvttsd2si\0"
  "vcvttss2si\0"
  "vdivpd\0"
  "vdivps\0"
  "vdivsd\0"
  "vdivss\0"
  "vdppd\0"
  "vdpps\0"
  "vextractf128\0"
  "vextracti128\0"
  "vextractps\0"
  "vfmadd132pd\0"
  "vfmadd132ps\0"
  "vfmadd132sd\0"
  "vfmadd132ss\0"
  "vfmadd213pd\0"
  "vfmadd213ps\0"
  "vfmadd213sd\0"
  "vfmadd213ss\0"
  "vfmadd231pd\0"
  "vfmadd231ps\0"
  "vfmadd231sd\0"
  "vfmadd231ss\0"
  "vfmaddpd\0"
  "vfmaddps\0"
  "vfmaddsd\0"
  "vfmaddss\0"
  "vfmaddsub132pd\0"
  "vfmaddsub132ps\0"
  "vfmaddsub213pd\0"
  "vfmaddsub213ps\0"
  "vfmaddsub231pd\0"
  "vfmaddsub231ps\0"
  "vfmaddsubpd\0"
  "vfmaddsubps\0"
  "vfmsub132pd\0"
  "vfmsub132ps\0"
  "vfmsub132sd\0"
  "vfmsub132ss\0"
  "vfmsub213pd\0"
  "vfmsub213ps\0"
  "vfmsub213sd\0"
  "vfmsub213ss\0"
  "vfmsub231pd\0"
  "vfmsub231ps\0"
  "vfmsub231sd\0"
  "vfmsub231ss\0"
  "vfmsubadd132pd\0"
  "vfmsubadd132ps\0"
  "vfmsubadd213pd\0"
  "vfmsubadd213ps\0"
  "vfmsubadd231pd\0"
  "vfmsubadd231ps\0"
  "vfmsubaddpd\0"
  "vfmsubaddps\0"
  "vfmsubpd\0"
  "vfmsubps\0"
  "vfmsubsd\0"
  "vfmsubss\0"
  "vfnmadd132pd\0"
  "vfnmadd132ps\0"
  "vfnmadd132sd\0"
  "vfnmadd132ss\0"
  "vfnmadd213pd\0"
  "vfnmadd213ps\0"
  "vfnmadd213sd\0"
  "vfnmadd213ss\0"
  "vfnmadd231pd\0"
  "vfnmadd231ps\0"
  "vfnmadd231sd\0"
  "vfnmadd231ss\0"
  "vfnmaddpd\0"
  "vfnmaddps\0"
  "vfnmaddsd\0"
  "vfnmaddss\0"
  "vfnmsub132pd\0"
  "vfnmsub132ps\0"
  "vfnmsub132sd\0"
  "vfnmsub132ss\0"
  "vfnmsub213pd\0"
  "vfnmsub213ps\0"
  "vfnmsub213sd\0"
  "vfnmsub213ss\0"
  "vfnmsub231pd\0"
  "vfnmsub231ps\0"
  "vfnmsub231sd\0"
  "vfnmsub231ss\0"
  "vfnmsubpd\0"
  "vfnmsubps\0"
  "vfnmsubsd\0"
  "vfnmsubss\0"
  "vfrczpd\0"
  "vfrczps\0"
  "vfrczsd\0"
  "vfrczss\0"
  "vgatherdpd\0"
  "vgatherdps\0"
  "vgatherqpd\0"
  "vgatherqps\0"
  "vhaddpd\0"
  "vhaddps\0"
  "vhsubpd\0"
  "vhsubps\0"
  "vinsertf128\0"
  "vinserti128\0"
  "vinsertps\0"
  "vlddqu\0"
  "vldmxcsr\0"
  "vmaskmovdqu\0"
  "vmaskmovpd\0"
  "vmaskmovps\0"
  "vmaxpd\0"
  "vmaxps\0"
  "vmaxsd\0"
  "vmaxss\0"
  "vminpd\0"
  "vminps\0"
  "vminsd\0"
  "vminss\0"
  "vmovapd\0"
  "vmovaps\0"
  "vmovd\0"
  "vmovddup\0"
  "vmovdqa\0"
  "vmovdqu\0"
  "vmovhlps\0"
  "vmovhpd\0"
  "vmovhps\0"
  "vmovlhps\0"
  "vmovlpd\0"
  "vmovlps\0"
  "vmovmskpd\0"
  "vmovmskps\0"
  "vmovntdq\0"
  "vmovntdqa\0"
  "vmovntpd\0"
  "vmovntps\0"
  "vmovq\0"
  "vmovsd\0"
  "vmovshdup\0"
  "vmovsldup\0"
  "vmovss\0"
  "vmovupd\0"
  "vmovups\0"
  "vmpsadbw\0"
  "vmulpd\0"
  "vmulps\0"
  "vmulsd\0"
  "vmulss\0"
  "vorpd\0"
  "vorps\0"
  "vpabsb\0"
  "vpabsd\0"
  "vpabsw\0"
  "vpackssdw\0"
  "vpacksswb\0"
  "vpackusdw\0"
  "vpackuswb\0"
  "vpaddb\0"
  "vpaddd\0"
  "vpaddq\0"
  "vpaddsb\0"
  "vpaddsw\0"
  "vpaddusb\0"
  "vpaddusw\0"
  "vpaddw\0"
  "vpalignr\0"
  "vpand\0"
  "vpandn\0"
  "vpavgb\0"
  "vpavgw\0"
  "vpblendd\0"
  "vpblendvb\0"
  "vpblendw\0"
  "vpbroadcastb\0"
  "vpbroadcastd\0"
  "vpbroadcastq\0"
  "vpbroadcastw\0"
  "vpclmulqdq\0"
  "vpcmov\0"
  "vpcmpeqb\0"
  "vpcmpeqd\0"
  "vpcmpeqq\0"
  "vpcmpeqw\0"
  "vpcmpestri\0"
  "vpcmpestrm\0"
  "vpcmpgtb\0"
  "vpcmpgtd\0"
  "vpcmpgtq\0"
  "vpcmpgtw\0"
  "vpcmpistri\0"
  "vpcmpistrm\0"
  "vpcomb\0"
  "vpcomd\0"
  "vpcomq\0"
  "vpcomub\0"
  "vpcomud\0"
  "vpcomuq\0"
  "vpcomuw\0"
  "vpcomw\0"
  "vperm2f128\0"
  "vperm2i128\0"
  "vpermd\0"
  "vpermil2pd\0"
  "vpermil2ps\0"
  "vpermilpd\0"
  "vpermilps\0"
  "vpermpd\0"
  "vpermps\0"
  "vpermq\0"
  "vpextrb\0"
  "vpextrd\0"
  "vpextrq\0"
  "vpextrw\0"
  "vpgatherdd\0"
  "vpgatherdq\0"
  "vpgatherqd\0"
  "vpgatherqq\0"
  "vphaddbd\0"
  "vphaddbq\0"
  "vphaddbw\0"
  "vphaddd\0"
  "vphadddq\0"
  "vphaddsw\0"
  "vphaddubd\0"
  "vphaddubq\0"
  "vphaddubw\0"
  "vphaddudq\0"
  "vphadduwd\0"
  "vphadduwq\0"
  "vphaddw\0"
  "vphaddwd\0"
  "vphaddwq\0"
  "vphminposuw\0"
  "vphsubbw\0"
  "vphsubd\0"
  "vphsubdq\0"
  "vphsubsw\0"
  "vphsubw\0"
  "vphsubwd\0"
  "vpinsrb\0"
  "vpinsrd\0"
  "vpinsrq\0"
  "vpinsrw\0"
  "vpmacsdd\0"
  "vpmacsdqh\0"
  "vpmacsdql\0"
  "vpmacssdd\0"
  "vpmacssdqh\0"
  "vpmacssdql\0"
  "vpmacsswd\0"
  "vpmacssww\0"
  "vpmacswd\0"
  "vpmacsww\0"
  "vpmadcsswd\0"
  "vpmadcswd\0"
  "vpmaddubsw\0"
  "vpmaddwd\0"
  "vpmaskmovd\0"
  "vpmaskmovq\0"
  "vpmaxsb\0"
  "vpmaxsd\0"
  "vpmaxsw\0"
  "vpmaxub\0"
  "vpmaxud\0"
  "vpmaxuw\0"
  "vpminsb\0"
  "vpminsd\0"
  "vpminsw\0"
  "vpminub\0"
  "vpminud\0"
  "vpminuw\0"
  "vpmovmskb\0"
  "vpmovsxbd\0"
  "vpmovsxbq\0"
  "vpmovsxbw\0"
  "vpmovsxdq\0"
  "vpmovsxwd\0"
  "vpmovsxwq\0"
  "vpmovzxbd\0"
  "vpmovzxbq\0"
  "vpmovzxbw\0"
  "vpmovzxdq\0"
  "vpmovzxwd\0"
  "vpmovzxwq\0"
  "vpmuldq\0"
  "vpmulhrsw\0"
  "vpmulhuw\0"
  "vpmulhw\0"
  "vpmulld\0"
  "vpmullw\0"
  "vpmuludq\0"
  "vpor\0"
  "vpperm\0"
  "vprotb\0"
  "vprotd\0"
  "vprotq\0"
  "vprotw\0"
  "vpsadbw\0"
  "vpshab\0"
  "vpshad\0"
  "vpshaq\0"
  "vpshaw\0"
  "vpshlb\0"
  "vpshld\0"
  "vpshlq\0"
  "vpshlw\0"
  "vpshufb\0"
  "vpshufd\0"
  "vpshufhw\0"
  "vpshuflw\0"
  "vpsignb\0"
  "vpsignd\0"
  "vpsignw\0"
  "vpslld\0"
  "vpslldq\0"
  "vpsllq\0"
  "vpsllvd\0"
  "vpsllvq\0"
  "vpsllw\0"
  "vpsrad\0"
  "vpsravd\0"
  "vpsraw\0"
  "vpsrld\0"
  "vpsrldq\0"
  "vpsrlq\0"
  "vpsrlvd\0"
  "vpsrlvq\0"
  "vpsrlw\0"
  "vpsubb\0"
  "vpsubd\0"
  "vpsubq\0"
  "vpsubsb\0"
  "vpsubsw\0"
  "vpsubusb\0"
  "vpsubusw\0"
  "vpsubw\0"
  "vptest\0"
  "vpunpckhbw\0"
  "vpunpckhdq\0"
  "vpunpckhqdq\0"
  "vpunpckhwd\0"
  "vpunpcklbw\0"
  "vpunpckldq\0"
  "vpunpcklqdq\0"
  "vpunpcklwd\0"
  "vpxor\0"
  "vrcpps\0"
  "vrcpss\0"
  "vroundpd\0"
  "vroundps\0"
  "vroundsd\0"
  "vroundss\0"
  "vrsqrtps\0"
  "vrsqrtss\0"
  "vshufpd\0"
  "vshufps\0"
  "vsqrtpd\0"
  "vsqrtps\0"
  "vsqrtsd\0"
  "vsqrtss\0"
  "vstmxcsr\0"
  "vsubpd\0"
  "vsubps\0"
  "vsubsd\0"
  "vsubss\0"
  "vtestpd\0"
  "vtestps\0"
  "vucomisd\0"
  "vucomiss\0"
  "vunpckhpd\0"
  "vunpckhps\0"
  "vunpcklpd\0"
  "vunpcklps\0"
  "vxorpd\0"
  "vxorps\0"
  "vzeroall\0"
  "vzeroupper\0"
  "wrfsbase\0"
  "wrgsbase\0"
  "xadd\0"
  "xchg\0"
  "xgetbv\0"
  "xor\0"
  "xorpd\0"
  "xorps\0"
  "xrstor\0"
  "xrstor64\0"
  "xsave\0"
  "xsave64\0"
  "xsaveopt\0"
  "xsaveopt64\0"
  "xsetbv\0";

// Automatically generated, do not edit.
enum kX86InstAlphaIndex {
  kX86InstAlphaIndexFirst = 'a',
  kX86InstAlphaIndexLast = 'z',
  kX86InstAlphaIndexInvalid = 0xFFFF
};

// Automatically generated, do not edit.
static const uint16_t _x86InstAlphaIndex[26] = {
  kX86InstIdAdc,
  kX86InstIdBextr,
  kX86InstIdCall,
  kX86InstIdDaa,
  kX86InstIdEmms,
  kX86InstIdF2xm1,
  0xFFFF,
  kX86InstIdHaddpd,
  kX86InstIdIdiv,
  kX86InstIdJa,
  0xFFFF,
  kX86InstIdLahf,
  kX86InstIdMaskmovdqu,
  kX86InstIdNeg,
  kX86InstIdOr,
  kX86InstIdPabsb,
  0xFFFF,
  kX86InstIdRcl,
  kX86InstIdSahf,
  kX86InstIdTest,
  kX86InstIdUcomisd,
  kX86InstIdVaddpd,
  kX86InstIdWrfsbase,
  kX86InstIdXadd,
  0xFFFF,
  0xFFFF
};

// Automatically generated, do not edit.
enum kX86InstData_NameIndex {
  kInstIdNone_NameIndex = 0,
  kX86InstIdAdc_NameIndex = 1,
  kX86InstIdAdd_NameIndex = 5,
  kX86InstIdAddpd_NameIndex = 9,
  kX86InstIdAddps_NameIndex = 15,
  kX86InstIdAddsd_NameIndex = 21,
  kX86InstIdAddss_NameIndex = 27,
  kX86InstIdAddsubpd_NameIndex = 33,
  kX86InstIdAddsubps_NameIndex = 42,
  kX86InstIdAesdec_NameIndex = 51,
  kX86InstIdAesdeclast_NameIndex = 58,
  kX86InstIdAesenc_NameIndex = 69,
  kX86InstIdAesenclast_NameIndex = 76,
  kX86InstIdAesimc_NameIndex = 87,
  kX86InstIdAeskeygenassist_NameIndex = 94,
  kX86InstIdAnd_NameIndex = 110,
  kX86InstIdAndn_NameIndex = 114,
  kX86InstIdAndnpd_NameIndex = 119,
  kX86InstIdAndnps_NameIndex = 126,
  kX86InstIdAndpd_NameIndex = 133,
  kX86InstIdAndps_NameIndex = 139,
  kX86InstIdBextr_NameIndex = 145,
  kX86InstIdBlendpd_NameIndex = 151,
  kX86InstIdBlendps_NameIndex = 159,
  kX86InstIdBlendvpd_NameIndex = 167,
  kX86InstIdBlendvps_NameIndex = 176,
  kX86InstIdBlsi_NameIndex = 185,
  kX86InstIdBlsmsk_NameIndex = 190,
  kX86InstIdBlsr_NameIndex = 197,
  kX86InstIdBsf_NameIndex = 202,
  kX86InstIdBsr_NameIndex = 206,
  kX86InstIdBswap_NameIndex = 210,
  kX86InstIdBt_NameIndex = 216,
  kX86InstIdBtc_NameIndex = 219,
  kX86InstIdBtr_NameIndex = 223,
  kX86InstIdBts_NameIndex = 227,
  kX86InstIdBzhi_NameIndex = 231,
  kX86InstIdCall_NameIndex = 236,
  kX86InstIdCbw_NameIndex = 241,
  kX86InstIdCdq_NameIndex = 245,
  kX86InstIdCdqe_NameIndex = 249,
  kX86InstIdClc_NameIndex = 254,
  kX86InstIdCld_NameIndex = 258,
  kX86InstIdClflush_NameIndex = 262,
  kX86InstIdCmc_NameIndex = 270,
  kX86InstIdCmova_NameIndex = 274,
  kX86InstIdCmovae_NameIndex = 280,
  kX86InstIdCmovb_NameIndex = 287,
  kX86InstIdCmovbe_NameIndex = 293,
  kX86InstIdCmovc_NameIndex = 300,
  kX86InstIdCmove_NameIndex = 306,
  kX86InstIdCmovg_NameIndex = 312,
  kX86InstIdCmovge_NameIndex = 318,
  kX86InstIdCmovl_NameIndex = 325,
  kX86InstIdCmovle_NameIndex = 331,
  kX86InstIdCmovna_NameIndex = 338,
  kX86InstIdCmovnae_NameIndex = 345,
  kX86InstIdCmovnb_NameIndex = 353,
  kX86InstIdCmovnbe_NameIndex = 360,
  kX86InstIdCmovnc_NameIndex = 368,
  kX86InstIdCmovne_NameIndex = 375,
  kX86InstIdCmovng_NameIndex = 382,
  kX86InstIdCmovnge_NameIndex = 389,
  kX86InstIdCmovnl_NameIndex = 397,
  kX86InstIdCmovnle_NameIndex = 404,
  kX86InstIdCmovno_NameIndex = 412,
  kX86InstIdCmovnp_NameIndex = 419,
  kX86InstIdCmovns_NameIndex = 426,
  kX86InstIdCmovnz_NameIndex = 433,
  kX86InstIdCmovo_NameIndex = 440,
  kX86InstIdCmovp_NameIndex = 446,
  kX86InstIdCmovpe_NameIndex = 452,
  kX86InstIdCmovpo_NameIndex = 459,
  kX86InstIdCmovs_NameIndex = 466,
  kX86InstIdCmovz_NameIndex = 472,
  kX86InstIdCmp_NameIndex = 478,
  kX86InstIdCmppd_NameIndex = 482,
  kX86InstIdCmpps_NameIndex = 488,
  kX86InstIdCmpsB_NameIndex = 494,
  kX86InstIdCmpsD_NameIndex = 501,
  kX86InstIdCmpsQ_NameIndex = 508,
  kX86InstIdCmpsW_NameIndex = 515,
  kX86InstIdCmpsd_NameIndex = 522,
  kX86InstIdCmpss_NameIndex = 528,
  kX86InstIdCmpxchg_NameIndex = 534,
  kX86InstIdCmpxchg16b_NameIndex = 542,
  kX86InstIdCmpxchg8b_NameIndex = 553,
  kX86InstIdComisd_NameIndex = 563,
  kX86InstIdComiss_NameIndex = 570,
  kX86InstIdCpuid_NameIndex = 577,
  kX86InstIdCqo_NameIndex = 583,
  kX86InstIdCrc32_NameIndex = 587,
  kX86InstIdCvtdq2pd_NameIndex = 593,
  kX86InstIdCvtdq2ps_NameIndex = 602,
  kX86InstIdCvtpd2dq_NameIndex = 611,
  kX86InstIdCvtpd2pi_NameIndex = 620,
  kX86InstIdCvtpd2ps_NameIndex = 629,
  kX86InstIdCvtpi2pd_NameIndex = 638,
  kX86InstIdCvtpi2ps_NameIndex = 647,
  kX86InstIdCvtps2dq_NameIndex = 656,
  kX86InstIdCvtps2pd_NameIndex = 665,
  kX86InstIdCvtps2pi_NameIndex = 674,
  kX86InstIdCvtsd2si_NameIndex = 683,
  kX86InstIdCvtsd2ss_NameIndex = 692,
  kX86InstIdCvtsi2sd_NameIndex = 701,
  kX86InstIdCvtsi2ss_NameIndex = 710,
  kX86InstIdCvtss2sd_NameIndex = 719,
  kX86InstIdCvtss2si_NameIndex = 728,
  kX86InstIdCvttpd2dq_NameIndex = 737,
  kX86InstIdCvttpd2pi_NameIndex = 747,
  kX86InstIdCvttps2dq_NameIndex = 757,
  kX86InstIdCvttps2pi_NameIndex = 767,
  kX86InstIdCvttsd2si_NameIndex = 777,
  kX86InstIdCvttss2si_NameIndex = 787,
  kX86InstIdCwd_NameIndex = 797,
  kX86InstIdCwde_NameIndex = 801,
  kX86InstIdDaa_NameIndex = 806,
  kX86InstIdDas_NameIndex = 810,
  kX86InstIdDec_NameIndex = 814,
  kX86InstIdDiv_NameIndex = 818,
  kX86InstIdDivpd_NameIndex = 822,
  kX86InstIdDivps_NameIndex = 828,
  kX86InstIdDivsd_NameIndex = 834,
  kX86InstIdDivss_NameIndex = 840,
  kX86InstIdDppd_NameIndex = 846,
  kX86InstIdDpps_NameIndex = 851,
  kX86InstIdEmms_NameIndex = 856,
  kX86InstIdEnter_NameIndex = 861,
  kX86InstIdExtractps_NameIndex = 867,
  kX86InstIdExtrq_NameIndex = 877,
  kX86InstIdF2xm1_NameIndex = 883,
  kX86InstIdFabs_NameIndex = 889,
  kX86InstIdFadd_NameIndex = 894,
  kX86InstIdFaddp_NameIndex = 899,
  kX86InstIdFbld_NameIndex = 905,
  kX86InstIdFbstp_NameIndex = 910,
  kX86InstIdFchs_NameIndex = 916,
  kX86InstIdFclex_NameIndex = 921,
  kX86InstIdFcmovb_NameIndex = 927,
  kX86InstIdFcmovbe_NameIndex = 934,
  kX86InstIdFcmove_NameIndex = 942,
  kX86InstIdFcmovnb_NameIndex = 949,
  kX86InstIdFcmovnbe_NameIndex = 957,
  kX86InstIdFcmovne_NameIndex = 966,
  kX86InstIdFcmovnu_NameIndex = 974,
  kX86InstIdFcmovu_NameIndex = 982,
  kX86InstIdFcom_NameIndex = 989,
  kX86InstIdFcomi_NameIndex = 994,
  kX86InstIdFcomip_NameIndex = 1000,
  kX86InstIdFcomp_NameIndex = 1007,
  kX86InstIdFcompp_NameIndex = 1013,
  kX86InstIdFcos_NameIndex = 1020,
  kX86InstIdFdecstp_NameIndex = 1025,
  kX86InstIdFdiv_NameIndex = 1033,
  kX86InstIdFdivp_NameIndex = 1038,
  kX86InstIdFdivr_NameIndex = 1044,
  kX86InstIdFdivrp_NameIndex = 1050,
  kX86InstIdFemms_NameIndex = 1057,
  kX86InstIdFfree_NameIndex = 1063,
  kX86InstIdFiadd_NameIndex = 1069,
  kX86InstIdFicom_NameIndex = 1075,
  kX86InstIdFicomp_NameIndex = 1081,
  kX86InstIdFidiv_NameIndex = 1088,
  kX86InstIdFidivr_NameIndex = 1094,
  kX86InstIdFild_NameIndex = 1101,
  kX86InstIdFimul_NameIndex = 1106,
  kX86InstIdFincstp_NameIndex = 1112,
  kX86InstIdFinit_NameIndex = 1120,
  kX86InstIdFist_NameIndex = 1126,
  kX86InstIdFistp_NameIndex = 1131,
  kX86InstIdFisttp_NameIndex = 1137,
  kX86InstIdFisub_NameIndex = 1144,
  kX86InstIdFisubr_NameIndex = 1150,
  kX86InstIdFld_NameIndex = 1157,
  kX86InstIdFld1_NameIndex = 1161,
  kX86InstIdFldcw_NameIndex = 1166,
  kX86InstIdFldenv_NameIndex = 1172,
  kX86InstIdFldl2e_NameIndex = 1179,
  kX86InstIdFldl2t_NameIndex = 1186,
  kX86InstIdFldlg2_NameIndex = 1193,
  kX86InstIdFldln2_NameIndex = 1200,
  kX86InstIdFldpi_NameIndex = 1207,
  kX86InstIdFldz_NameIndex = 1213,
  kX86InstIdFmul_NameIndex = 1218,
  kX86InstIdFmulp_NameIndex = 1223,
  kX86InstIdFnclex_NameIndex = 1229,
  kX86InstIdFninit_NameIndex = 1236,
  kX86InstIdFnop_NameIndex = 1243,
  kX86InstIdFnsave_NameIndex = 1248,
  kX86InstIdFnstcw_NameIndex = 1255,
  kX86InstIdFnstenv_NameIndex = 1262,
  kX86InstIdFnstsw_NameIndex = 1270,
  kX86InstIdFpatan_NameIndex = 1277,
  kX86InstIdFprem_NameIndex = 1284,
  kX86InstIdFprem1_NameIndex = 1290,
  kX86InstIdFptan_NameIndex = 1297,
  kX86InstIdFrndint_NameIndex = 1303,
  kX86InstIdFrstor_NameIndex = 1311,
  kX86InstIdFsave_NameIndex = 1318,
  kX86InstIdFscale_NameIndex = 1324,
  kX86InstIdFsin_NameIndex = 1331,
  kX86InstIdFsincos_NameIndex = 1336,
  kX86InstIdFsqrt_NameIndex = 1344,
  kX86InstIdFst_NameIndex = 1350,
  kX86InstIdFstcw_NameIndex = 1354,
  kX86InstIdFstenv_NameIndex = 1360,
  kX86InstIdFstp_NameIndex = 1367,
  kX86InstIdFstsw_NameIndex = 1372,
  kX86InstIdFsub_NameIndex = 1378,
  kX86InstIdFsubp_NameIndex = 1383,
  kX86InstIdFsubr_NameIndex = 1389,
  kX86InstIdFsubrp_NameIndex = 1395,
  kX86InstIdFtst_NameIndex = 1402,
  kX86InstIdFucom_NameIndex = 1407,
  kX86InstIdFucomi_NameIndex = 1413,
  kX86InstIdFucomip_NameIndex = 1420,
  kX86InstIdFucomp_NameIndex = 1428,
  kX86InstIdFucompp_NameIndex = 1435,
  kX86InstIdFwait_NameIndex = 1443,
  kX86InstIdFxam_NameIndex = 1449,
  kX86InstIdFxch_NameIndex = 1454,
  kX86InstIdFxrstor_NameIndex = 1459,
  kX86InstIdFxsave_NameIndex = 1467,
  kX86InstIdFxtract_NameIndex = 1474,
  kX86InstIdFyl2x_NameIndex = 1482,
  kX86InstIdFyl2xp1_NameIndex = 1488,
  kX86InstIdHaddpd_NameIndex = 1496,
  kX86InstIdHaddps_NameIndex = 1503,
  kX86InstIdHsubpd_NameIndex = 1510,
  kX86InstIdHsubps_NameIndex = 1517,
  kX86InstIdIdiv_NameIndex = 1524,
  kX86InstIdImul_NameIndex = 1529,
  kX86InstIdInc_NameIndex = 1534,
  kX86InstIdInsertps_NameIndex = 1538,
  kX86InstIdInsertq_NameIndex = 1547,
  kX86InstIdInt_NameIndex = 1555,
  kX86InstIdJa_NameIndex = 1559,
  kX86InstIdJae_NameIndex = 1562,
  kX86InstIdJb_NameIndex = 1566,
  kX86InstIdJbe_NameIndex = 1569,
  kX86InstIdJc_NameIndex = 1573,
  kX86InstIdJe_NameIndex = 1576,
  kX86InstIdJg_NameIndex = 1579,
  kX86InstIdJge_NameIndex = 1582,
  kX86InstIdJl_NameIndex = 1586,
  kX86InstIdJle_NameIndex = 1589,
  kX86InstIdJna_NameIndex = 1593,
  kX86InstIdJnae_NameIndex = 1597,
  kX86InstIdJnb_NameIndex = 1602,
  kX86InstIdJnbe_NameIndex = 1606,
  kX86InstIdJnc_NameIndex = 1611,
  kX86InstIdJne_NameIndex = 1615,
  kX86InstIdJng_NameIndex = 1619,
  kX86InstIdJnge_NameIndex = 1623,
  kX86InstIdJnl_NameIndex = 1628,
  kX86InstIdJnle_NameIndex = 1632,
  kX86InstIdJno_NameIndex = 1637,
  kX86InstIdJnp_NameIndex = 1641,
  kX86InstIdJns_NameIndex = 1645,
  kX86InstIdJnz_NameIndex = 1649,
  kX86InstIdJo_NameIndex = 1653,
  kX86InstIdJp_NameIndex = 1656,
  kX86InstIdJpe_NameIndex = 1659,
  kX86InstIdJpo_NameIndex = 1663,
  kX86InstIdJs_NameIndex = 1667,
  kX86InstIdJz_NameIndex = 1670,
  kX86InstIdJecxz_NameIndex = 1673,
  kX86InstIdJmp_NameIndex = 1679,
  kX86InstIdLahf_NameIndex = 1683,
  kX86InstIdLddqu_NameIndex = 1688,
  kX86InstIdLdmxcsr_NameIndex = 1694,
  kX86InstIdLea_NameIndex = 1702,
  kX86InstIdLeave_NameIndex = 1706,
  kX86InstIdLfence_NameIndex = 1712,
  kX86InstIdLodsB_NameIndex = 1719,
  kX86InstIdLodsD_NameIndex = 1726,
  kX86InstIdLodsQ_NameIndex = 1733,
  kX86InstIdLodsW_NameIndex = 1740,
  kX86InstIdLzcnt_NameIndex = 1747,
  kX86InstIdMaskmovdqu_NameIndex = 1753,
  kX86InstIdMaskmovq_NameIndex = 1764,
  kX86InstIdMaxpd_NameIndex = 1773,
  kX86InstIdMaxps_NameIndex = 1779,
  kX86InstIdMaxsd_NameIndex = 1785,
  kX86InstIdMaxss_NameIndex = 1791,
  kX86InstIdMfence_NameIndex = 1797,
  kX86InstIdMinpd_NameIndex = 1804,
  kX86InstIdMinps_NameIndex = 1810,
  kX86InstIdMinsd_NameIndex = 1816,
  kX86InstIdMinss_NameIndex = 1822,
  kX86InstIdMonitor_NameIndex = 1828,
  kX86InstIdMov_NameIndex = 1836,
  kX86InstIdMovPtr_NameIndex = 1840,
  kX86InstIdMovapd_NameIndex = 1848,
  kX86InstIdMovaps_NameIndex = 1855,
  kX86InstIdMovbe_NameIndex = 1862,
  kX86InstIdMovd_NameIndex = 1868,
  kX86InstIdMovddup_NameIndex = 1873,
  kX86InstIdMovdq2q_NameIndex = 1881,
  kX86InstIdMovdqa_NameIndex = 1889,
  kX86InstIdMovdqu_NameIndex = 1896,
  kX86InstIdMovhlps_NameIndex = 1903,
  kX86InstIdMovhpd_NameIndex = 1911,
  kX86InstIdMovhps_NameIndex = 1918,
  kX86InstIdMovlhps_NameIndex = 1925,
  kX86InstIdMovlpd_NameIndex = 1933,
  kX86InstIdMovlps_NameIndex = 1940,
  kX86InstIdMovmskpd_NameIndex = 1947,
  kX86InstIdMovmskps_NameIndex = 1956,
  kX86InstIdMovntdq_NameIndex = 1965,
  kX86InstIdMovntdqa_NameIndex = 1973,
  kX86InstIdMovnti_NameIndex = 1982,
  kX86InstIdMovntpd_NameIndex = 1989,
  kX86InstIdMovntps_NameIndex = 1997,
  kX86InstIdMovntq_NameIndex = 2005,
  kX86InstIdMovntsd_NameIndex = 2012,
  kX86InstIdMovntss_NameIndex = 2020,
  kX86InstIdMovq_NameIndex = 2028,
  kX86InstIdMovq2dq_NameIndex = 2033,
  kX86InstIdMovsB_NameIndex = 2041,
  kX86InstIdMovsD_NameIndex = 2048,
  kX86InstIdMovsQ_NameIndex = 2055,
  kX86InstIdMovsW_NameIndex = 2062,
  kX86InstIdMovsd_NameIndex = 2069,
  kX86InstIdMovshdup_NameIndex = 2075,
  kX86InstIdMovsldup_NameIndex = 2084,
  kX86InstIdMovss_NameIndex = 2093,
  kX86InstIdMovsx_NameIndex = 2099,
  kX86InstIdMovsxd_NameIndex = 2105,
  kX86InstIdMovupd_NameIndex = 2112,
  kX86InstIdMovups_NameIndex = 2119,
  kX86InstIdMovzx_NameIndex = 2126,
  kX86InstIdMpsadbw_NameIndex = 2132,
  kX86InstIdMul_NameIndex = 2140,
  kX86InstIdMulpd_NameIndex = 2144,
  kX86InstIdMulps_NameIndex = 2150,
  kX86InstIdMulsd_NameIndex = 2156,
  kX86InstIdMulss_NameIndex = 2162,
  kX86InstIdMulx_NameIndex = 2168,
  kX86InstIdMwait_NameIndex = 2173,
  kX86InstIdNeg_NameIndex = 2179,
  kX86InstIdNop_NameIndex = 2183,
  kX86InstIdNot_NameIndex = 2187,
  kX86InstIdOr_NameIndex = 2191,
  kX86InstIdOrpd_NameIndex = 2194,
  kX86InstIdOrps_NameIndex = 2199,
  kX86InstIdPabsb_NameIndex = 2204,
  kX86InstIdPabsd_NameIndex = 2210,
  kX86InstIdPabsw_NameIndex = 2216,
  kX86InstIdPackssdw_NameIndex = 2222,
  kX86InstIdPacksswb_NameIndex = 2231,
  kX86InstIdPackusdw_NameIndex = 2240,
  kX86InstIdPackuswb_NameIndex = 2249,
  kX86InstIdPaddb_NameIndex = 2258,
  kX86InstIdPaddd_NameIndex = 2264,
  kX86InstIdPaddq_NameIndex = 2270,
  kX86InstIdPaddsb_NameIndex = 2276,
  kX86InstIdPaddsw_NameIndex = 2283,
  kX86InstIdPaddusb_NameIndex = 2290,
  kX86InstIdPaddusw_NameIndex = 2298,
  kX86InstIdPaddw_NameIndex = 2306,
  kX86InstIdPalignr_NameIndex = 2312,
  kX86InstIdPand_NameIndex = 2320,
  kX86InstIdPandn_NameIndex = 2325,
  kX86InstIdPause_NameIndex = 2331,
  kX86InstIdPavgb_NameIndex = 2337,
  kX86InstIdPavgw_NameIndex = 2343,
  kX86InstIdPblendvb_NameIndex = 2349,
  kX86InstIdPblendw_NameIndex = 2358,
  kX86InstIdPclmulqdq_NameIndex = 2366,
  kX86InstIdPcmpeqb_NameIndex = 2376,
  kX86InstIdPcmpeqd_NameIndex = 2384,
  kX86InstIdPcmpeqq_NameIndex = 2392,
  kX86InstIdPcmpeqw_NameIndex = 2400,
  kX86InstIdPcmpestri_NameIndex = 2408,
  kX86InstIdPcmpestrm_NameIndex = 2418,
  kX86InstIdPcmpgtb_NameIndex = 2428,
  kX86InstIdPcmpgtd_NameIndex = 2436,
  kX86InstIdPcmpgtq_NameIndex = 2444,
  kX86InstIdPcmpgtw_NameIndex = 2452,
  kX86InstIdPcmpistri_NameIndex = 2460,
  kX86InstIdPcmpistrm_NameIndex = 2470,
  kX86InstIdPdep_NameIndex = 2480,
  kX86InstIdPext_NameIndex = 2485,
  kX86InstIdPextrb_NameIndex = 2490,
  kX86InstIdPextrd_NameIndex = 2497,
  kX86InstIdPextrq_NameIndex = 2504,
  kX86InstIdPextrw_NameIndex = 2511,
  kX86InstIdPf2id_NameIndex = 2518,
  kX86InstIdPf2iw_NameIndex = 2524,
  kX86InstIdPfacc_NameIndex = 2530,
  kX86InstIdPfadd_NameIndex = 2536,
  kX86InstIdPfcmpeq_NameIndex = 2542,
  kX86InstIdPfcmpge_NameIndex = 2550,
  kX86InstIdPfcmpgt_NameIndex = 2558,
  kX86InstIdPfmax_NameIndex = 2566,
  kX86InstIdPfmin_NameIndex = 2572,
  kX86InstIdPfmul_NameIndex = 2578,
  kX86InstIdPfnacc_NameIndex = 2584,
  kX86InstIdPfpnacc_NameIndex = 2591,
  kX86InstIdPfrcp_NameIndex = 2599,
  kX86InstIdPfrcpit1_NameIndex = 2605,
  kX86InstIdPfrcpit2_NameIndex = 2614,
  kX86InstIdPfrsqit1_NameIndex = 2623,
  kX86InstIdPfrsqrt_NameIndex = 2632,
  kX86InstIdPfsub_NameIndex = 2640,
  kX86InstIdPfsubr_NameIndex = 2646,
  kX86InstIdPhaddd_NameIndex = 2653,
  kX86InstIdPhaddsw_NameIndex = 2660,
  kX86InstIdPhaddw_NameIndex = 2668,
  kX86InstIdPhminposuw_NameIndex = 2675,
  kX86InstIdPhsubd_NameIndex = 2686,
  kX86InstIdPhsubsw_NameIndex = 2693,
  kX86InstIdPhsubw_NameIndex = 2701,
  kX86InstIdPi2fd_NameIndex = 2708,
  kX86InstIdPi2fw_NameIndex = 2714,
  kX86InstIdPinsrb_NameIndex = 2720,
  kX86InstIdPinsrd_NameIndex = 2727,
  kX86InstIdPinsrq_NameIndex = 2734,
  kX86InstIdPinsrw_NameIndex = 2741,
  kX86InstIdPmaddubsw_NameIndex = 2748,
  kX86InstIdPmaddwd_NameIndex = 2758,
  kX86InstIdPmaxsb_NameIndex = 2766,
  kX86InstIdPmaxsd_NameIndex = 2773,
  kX86InstIdPmaxsw_NameIndex = 2780,
  kX86InstIdPmaxub_NameIndex = 2787,
  kX86InstIdPmaxud_NameIndex = 2794,
  kX86InstIdPmaxuw_NameIndex = 2801,
  kX86InstIdPminsb_NameIndex = 2808,
  kX86InstIdPminsd_NameIndex = 2815,
  kX86InstIdPminsw_NameIndex = 2822,
  kX86InstIdPminub_NameIndex = 2829,
  kX86InstIdPminud_NameIndex = 2836,
  kX86InstIdPminuw_NameIndex = 2843,
  kX86InstIdPmovmskb_NameIndex = 2850,
  kX86InstIdPmovsxbd_NameIndex = 2859,
  kX86InstIdPmovsxbq_NameIndex = 2868,
  kX86InstIdPmovsxbw_NameIndex = 2877,
  kX86InstIdPmovsxdq_NameIndex = 2886,
  kX86InstIdPmovsxwd_NameIndex = 2895,
  kX86InstIdPmovsxwq_NameIndex = 2904,
  kX86InstIdPmovzxbd_NameIndex = 2913,
  kX86InstIdPmovzxbq_NameIndex = 2922,
  kX86InstIdPmovzxbw_NameIndex = 2931,
  kX86InstIdPmovzxdq_NameIndex = 2940,
  kX86InstIdPmovzxwd_NameIndex = 2949,
  kX86InstIdPmovzxwq_NameIndex = 2958,
  kX86InstIdPmuldq_NameIndex = 2967,
  kX86InstIdPmulhrsw_NameIndex = 2974,
  kX86InstIdPmulhuw_NameIndex = 2983,
  kX86InstIdPmulhw_NameIndex = 2991,
  kX86InstIdPmulld_NameIndex = 2998,
  kX86InstIdPmullw_NameIndex = 3005,
  kX86InstIdPmuludq_NameIndex = 3012,
  kX86InstIdPop_NameIndex = 3020,
  kX86InstIdPopa_NameIndex = 3024,
  kX86InstIdPopcnt_NameIndex = 3029,
  kX86InstIdPopf_NameIndex = 3036,
  kX86InstIdPor_NameIndex = 3041,
  kX86InstIdPrefetch_NameIndex = 3045,
  kX86InstIdPrefetch3dNow_NameIndex = 3054,
  kX86InstIdPrefetchw3dNow_NameIndex = 3069,
  kX86InstIdPsadbw_NameIndex = 3085,
  kX86InstIdPshufb_NameIndex = 3092,
  kX86InstIdPshufd_NameIndex = 3099,
  kX86InstIdPshufhw_NameIndex = 3106,
  kX86InstIdPshuflw_NameIndex = 3114,
  kX86InstIdPshufw_NameIndex = 3122,
  kX86InstIdPsignb_NameIndex = 3129,
  kX86InstIdPsignd_NameIndex = 3136,
  kX86InstIdPsignw_NameIndex = 3143,
  kX86InstIdPslld_NameIndex = 3150,
  kX86InstIdPslldq_NameIndex = 3156,
  kX86InstIdPsllq_NameIndex = 3163,
  kX86InstIdPsllw_NameIndex = 3169,
  kX86InstIdPsrad_NameIndex = 3175,
  kX86InstIdPsraw_NameIndex = 3181,
  kX86InstIdPsrld_NameIndex = 3187,
  kX86InstIdPsrldq_NameIndex = 3193,
  kX86InstIdPsrlq_NameIndex = 3200,
  kX86InstIdPsrlw_NameIndex = 3206,
  kX86InstIdPsubb_NameIndex = 3212,
  kX86InstIdPsubd_NameIndex = 3218,
  kX86InstIdPsubq_NameIndex = 3224,
  kX86InstIdPsubsb_NameIndex = 3230,
  kX86InstIdPsubsw_NameIndex = 3237,
  kX86InstIdPsubusb_NameIndex = 3244,
  kX86InstIdPsubusw_NameIndex = 3252,
  kX86InstIdPsubw_NameIndex = 3260,
  kX86InstIdPswapd_NameIndex = 3266,
  kX86InstIdPtest_NameIndex = 3273,
  kX86InstIdPunpckhbw_NameIndex = 3279,
  kX86InstIdPunpckhdq_NameIndex = 3289,
  kX86InstIdPunpckhqdq_NameIndex = 3299,
  kX86InstIdPunpckhwd_NameIndex = 3310,
  kX86InstIdPunpcklbw_NameIndex = 3320,
  kX86InstIdPunpckldq_NameIndex = 3330,
  kX86InstIdPunpcklqdq_NameIndex = 3340,
  kX86InstIdPunpcklwd_NameIndex = 3351,
  kX86InstIdPush_NameIndex = 3361,
  kX86InstIdPusha_NameIndex = 3366,
  kX86InstIdPushf_NameIndex = 3372,
  kX86InstIdPxor_NameIndex = 3378,
  kX86InstIdRcl_NameIndex = 3383,
  kX86InstIdRcpps_NameIndex = 3387,
  kX86InstIdRcpss_NameIndex = 3393,
  kX86InstIdRcr_NameIndex = 3399,
  kX86InstIdRdfsbase_NameIndex = 3403,
  kX86InstIdRdgsbase_NameIndex = 3412,
  kX86InstIdRdrand_NameIndex = 3421,
  kX86InstIdRdtsc_NameIndex = 3428,
  kX86InstIdRdtscp_NameIndex = 3434,
  kX86InstIdRepLodsB_NameIndex = 3441,
  kX86InstIdRepLodsD_NameIndex = 3452,
  kX86InstIdRepLodsQ_NameIndex = 3463,
  kX86InstIdRepLodsW_NameIndex = 3474,
  kX86InstIdRepMovsB_NameIndex = 3485,
  kX86InstIdRepMovsD_NameIndex = 3496,
  kX86InstIdRepMovsQ_NameIndex = 3507,
  kX86InstIdRepMovsW_NameIndex = 3518,
  kX86InstIdRepStosB_NameIndex = 3529,
  kX86InstIdRepStosD_NameIndex = 3540,
  kX86InstIdRepStosQ_NameIndex = 3551,
  kX86InstIdRepStosW_NameIndex = 3562,
  kX86InstIdRepeCmpsB_NameIndex = 3573,
  kX86InstIdRepeCmpsD_NameIndex = 3585,
  kX86InstIdRepeCmpsQ_NameIndex = 3597,
  kX86InstIdRepeCmpsW_NameIndex = 3609,
  kX86InstIdRepeScasB_NameIndex = 3621,
  kX86InstIdRepeScasD_NameIndex = 3633,
  kX86InstIdRepeScasQ_NameIndex = 3645,
  kX86InstIdRepeScasW_NameIndex = 3657,
  kX86InstIdRepneCmpsB_NameIndex = 3669,
  kX86InstIdRepneCmpsD_NameIndex = 3682,
  kX86InstIdRepneCmpsQ_NameIndex = 3695,
  kX86InstIdRepneCmpsW_NameIndex = 3708,
  kX86InstIdRepneScasB_NameIndex = 3721,
  kX86InstIdRepneScasD_NameIndex = 3734,
  kX86InstIdRepneScasQ_NameIndex = 3747,
  kX86InstIdRepneScasW_NameIndex = 3760,
  kX86InstIdRet_NameIndex = 3773,
  kX86InstIdRol_NameIndex = 3777,
  kX86InstIdRor_NameIndex = 3781,
  kX86InstIdRorx_NameIndex = 3785,
  kX86InstIdRoundpd_NameIndex = 3790,
  kX86InstIdRoundps_NameIndex = 3798,
  kX86InstIdRoundsd_NameIndex = 3806,
  kX86InstIdRoundss_NameIndex = 3814,
  kX86InstIdRsqrtps_NameIndex = 3822,
  kX86InstIdRsqrtss_NameIndex = 3830,
  kX86InstIdSahf_NameIndex = 3838,
  kX86InstIdSal_NameIndex = 3843,
  kX86InstIdSar_NameIndex = 3847,
  kX86InstIdSarx_NameIndex = 3851,
  kX86InstIdSbb_NameIndex = 3856,
  kX86InstIdScasB_NameIndex = 3860,
  kX86InstIdScasD_NameIndex = 3867,
  kX86InstIdScasQ_NameIndex = 3874,
  kX86InstIdScasW_NameIndex = 3881,
  kX86InstIdSeta_NameIndex = 3888,
  kX86InstIdSetae_NameIndex = 3893,
  kX86InstIdSetb_NameIndex = 3899,
  kX86InstIdSetbe_NameIndex = 3904,
  kX86InstIdSetc_NameIndex = 3910,
  kX86InstIdSete_NameIndex = 3915,
  kX86InstIdSetg_NameIndex = 3920,
  kX86InstIdSetge_NameIndex = 3925,
  kX86InstIdSetl_NameIndex = 3931,
  kX86InstIdSetle_NameIndex = 3936,
  kX86InstIdSetna_NameIndex = 3942,
  kX86InstIdSetnae_NameIndex = 3948,
  kX86InstIdSetnb_NameIndex = 3955,
  kX86InstIdSetnbe_NameIndex = 3961,
  kX86InstIdSetnc_NameIndex = 3968,
  kX86InstIdSetne_NameIndex = 3974,
  kX86InstIdSetng_NameIndex = 3980,
  kX86InstIdSetnge_NameIndex = 3986,
  kX86InstIdSetnl_NameIndex = 3993,
  kX86InstIdSetnle_NameIndex = 3999,
  kX86InstIdSetno_NameIndex = 4006,
  kX86InstIdSetnp_NameIndex = 4012,
  kX86InstIdSetns_NameIndex = 4018,
  kX86InstIdSetnz_NameIndex = 4024,
  kX86InstIdSeto_NameIndex = 4030,
  kX86InstIdSetp_NameIndex = 4035,
  kX86InstIdSetpe_NameIndex = 4040,
  kX86InstIdSetpo_NameIndex = 4046,
  kX86InstIdSets_NameIndex = 4052,
  kX86InstIdSetz_NameIndex = 4057,
  kX86InstIdSfence_NameIndex = 4062,
  kX86InstIdShl_NameIndex = 4069,
  kX86InstIdShld_NameIndex = 4073,
  kX86InstIdShlx_NameIndex = 4078,
  kX86InstIdShr_NameIndex = 4083,
  kX86InstIdShrd_NameIndex = 4087,
  kX86InstIdShrx_NameIndex = 4092,
  kX86InstIdShufpd_NameIndex = 4097,
  kX86InstIdShufps_NameIndex = 4104,
  kX86InstIdSqrtpd_NameIndex = 4111,
  kX86InstIdSqrtps_NameIndex = 4118,
  kX86InstIdSqrtsd_NameIndex = 4125,
  kX86InstIdSqrtss_NameIndex = 4132,
  kX86InstIdStc_NameIndex = 4139,
  kX86InstIdStd_NameIndex = 4143,
  kX86InstIdStmxcsr_NameIndex = 4147,
  kX86InstIdStosB_NameIndex = 4155,
  kX86InstIdStosD_NameIndex = 4162,
  kX86InstIdStosQ_NameIndex = 4169,
  kX86InstIdStosW_NameIndex = 4176,
  kX86InstIdSub_NameIndex = 4183,
  kX86InstIdSubpd_NameIndex = 4187,
  kX86InstIdSubps_NameIndex = 4193,
  kX86InstIdSubsd_NameIndex = 4199,
  kX86InstIdSubss_NameIndex = 4205,
  kX86InstIdTest_NameIndex = 4211,
  kX86InstIdTzcnt_NameIndex = 4216,
  kX86InstIdUcomisd_NameIndex = 4222,
  kX86InstIdUcomiss_NameIndex = 4230,
  kX86InstIdUd2_NameIndex = 4238,
  kX86InstIdUnpckhpd_NameIndex = 4242,
  kX86InstIdUnpckhps_NameIndex = 4251,
  kX86InstIdUnpcklpd_NameIndex = 4260,
  kX86InstIdUnpcklps_NameIndex = 4269,
  kX86InstIdVaddpd_NameIndex = 4278,
  kX86InstIdVaddps_NameIndex = 4285,
  kX86InstIdVaddsd_NameIndex = 4292,
  kX86InstIdVaddss_NameIndex = 4299,
  kX86InstIdVaddsubpd_NameIndex = 4306,
  kX86InstIdVaddsubps_NameIndex = 4316,
  kX86InstIdVaesdec_NameIndex = 4326,
  kX86InstIdVaesdeclast_NameIndex = 4334,
  kX86InstIdVaesenc_NameIndex = 4346,
  kX86InstIdVaesenclast_NameIndex = 4354,
  kX86InstIdVaesimc_NameIndex = 4366,
  kX86InstIdVaeskeygenassist_NameIndex = 4374,
  kX86InstIdVandnpd_NameIndex = 4391,
  kX86InstIdVandnps_NameIndex = 4399,
  kX86InstIdVandpd_NameIndex = 4407,
  kX86InstIdVandps_NameIndex = 4414,
  kX86InstIdVblendpd_NameIndex = 4421,
  kX86InstIdVblendps_NameIndex = 4430,
  kX86InstIdVblendvpd_NameIndex = 4439,
  kX86InstIdVblendvps_NameIndex = 4449,
  kX86InstIdVbroadcastf128_NameIndex = 4459,
  kX86InstIdVbroadcasti128_NameIndex = 4474,
  kX86InstIdVbroadcastsd_NameIndex = 4489,
  kX86InstIdVbroadcastss_NameIndex = 4502,
  kX86InstIdVcmppd_NameIndex = 4515,
  kX86InstIdVcmpps_NameIndex = 4522,
  kX86InstIdVcmpsd_NameIndex = 4529,
  kX86InstIdVcmpss_NameIndex = 4536,
  kX86InstIdVcomisd_NameIndex = 4543,
  kX86InstIdVcomiss_NameIndex = 4551,
  kX86InstIdVcvtdq2pd_NameIndex = 4559,
  kX86InstIdVcvtdq2ps_NameIndex = 4569,
  kX86InstIdVcvtpd2dq_NameIndex = 4579,
  kX86InstIdVcvtpd2ps_NameIndex = 4589,
  kX86InstIdVcvtph2ps_NameIndex = 4599,
  kX86InstIdVcvtps2dq_NameIndex = 4609,
  kX86InstIdVcvtps2pd_NameIndex = 4619,
  kX86InstIdVcvtps2ph_NameIndex = 4629,
  kX86InstIdVcvtsd2si_NameIndex = 4639,
  kX86InstIdVcvtsd2ss_NameIndex = 4649,
  kX86InstIdVcvtsi2sd_NameIndex = 4659,
  kX86InstIdVcvtsi2ss_NameIndex = 4669,
  kX86InstIdVcvtss2sd_NameIndex = 4679,
  kX86InstIdVcvtss2si_NameIndex = 4689,
  kX86InstIdVcvttpd2dq_NameIndex = 4699,
  kX86InstIdVcvttps2dq_NameIndex = 4710,
  kX86InstIdVcvttsd2si_NameIndex = 4721,
  kX86InstIdVcvttss2si_NameIndex = 4732,
  kX86InstIdVdivpd_NameIndex = 4743,
  kX86InstIdVdivps_NameIndex = 4750,
  kX86InstIdVdivsd_NameIndex = 4757,
  kX86InstIdVdivss_NameIndex = 4764,
  kX86InstIdVdppd_NameIndex = 4771,
  kX86InstIdVdpps_NameIndex = 4777,
  kX86InstIdVextractf128_NameIndex = 4783,
  kX86InstIdVextracti128_NameIndex = 4796,
  kX86InstIdVextractps_NameIndex = 4809,
  kX86InstIdVfmadd132pd_NameIndex = 4820,
  kX86InstIdVfmadd132ps_NameIndex = 4832,
  kX86InstIdVfmadd132sd_NameIndex = 4844,
  kX86InstIdVfmadd132ss_NameIndex = 4856,
  kX86InstIdVfmadd213pd_NameIndex = 4868,
  kX86InstIdVfmadd213ps_NameIndex = 4880,
  kX86InstIdVfmadd213sd_NameIndex = 4892,
  kX86InstIdVfmadd213ss_NameIndex = 4904,
  kX86InstIdVfmadd231pd_NameIndex = 4916,
  kX86InstIdVfmadd231ps_NameIndex = 4928,
  kX86InstIdVfmadd231sd_NameIndex = 4940,
  kX86InstIdVfmadd231ss_NameIndex = 4952,
  kX86InstIdVfmaddpd_NameIndex = 4964,
  kX86InstIdVfmaddps_NameIndex = 4973,
  kX86InstIdVfmaddsd_NameIndex = 4982,
  kX86InstIdVfmaddss_NameIndex = 4991,
  kX86InstIdVfmaddsub132pd_NameIndex = 5000,
  kX86InstIdVfmaddsub132ps_NameIndex = 5015,
  kX86InstIdVfmaddsub213pd_NameIndex = 5030,
  kX86InstIdVfmaddsub213ps_NameIndex = 5045,
  kX86InstIdVfmaddsub231pd_NameIndex = 5060,
  kX86InstIdVfmaddsub231ps_NameIndex = 5075,
  kX86InstIdVfmaddsubpd_NameIndex = 5090,
  kX86InstIdVfmaddsubps_NameIndex = 5102,
  kX86InstIdVfmsub132pd_NameIndex = 5114,
  kX86InstIdVfmsub132ps_NameIndex = 5126,
  kX86InstIdVfmsub132sd_NameIndex = 5138,
  kX86InstIdVfmsub132ss_NameIndex = 5150,
  kX86InstIdVfmsub213pd_NameIndex = 5162,
  kX86InstIdVfmsub213ps_NameIndex = 5174,
  kX86InstIdVfmsub213sd_NameIndex = 5186,
  kX86InstIdVfmsub213ss_NameIndex = 5198,
  kX86InstIdVfmsub231pd_NameIndex = 5210,
  kX86InstIdVfmsub231ps_NameIndex = 5222,
  kX86InstIdVfmsub231sd_NameIndex = 5234,
  kX86InstIdVfmsub231ss_NameIndex = 5246,
  kX86InstIdVfmsubadd132pd_NameIndex = 5258,
  kX86InstIdVfmsubadd132ps_NameIndex = 5273,
  kX86InstIdVfmsubadd213pd_NameIndex = 5288,
  kX86InstIdVfmsubadd213ps_NameIndex = 5303,
  kX86InstIdVfmsubadd231pd_NameIndex = 5318,
  kX86InstIdVfmsubadd231ps_NameIndex = 5333,
  kX86InstIdVfmsubaddpd_NameIndex = 5348,
  kX86InstIdVfmsubaddps_NameIndex = 5360,
  kX86InstIdVfmsubpd_NameIndex = 5372,
  kX86InstIdVfmsubps_NameIndex = 5381,
  kX86InstIdVfmsubsd_NameIndex = 5390,
  kX86InstIdVfmsubss_NameIndex = 5399,
  kX86InstIdVfnmadd132pd_NameIndex = 5408,
  kX86InstIdVfnmadd132ps_NameIndex = 5421,
  kX86InstIdVfnmadd132sd_NameIndex = 5434,
  kX86InstIdVfnmadd132ss_NameIndex = 5447,
  kX86InstIdVfnmadd213pd_NameIndex = 5460,
  kX86InstIdVfnmadd213ps_NameIndex = 5473,
  kX86InstIdVfnmadd213sd_NameIndex = 5486,
  kX86InstIdVfnmadd213ss_NameIndex = 5499,
  kX86InstIdVfnmadd231pd_NameIndex = 5512,
  kX86InstIdVfnmadd231ps_NameIndex = 5525,
  kX86InstIdVfnmadd231sd_NameIndex = 5538,
  kX86InstIdVfnmadd231ss_NameIndex = 5551,
  kX86InstIdVfnmaddpd_NameIndex = 5564,
  kX86InstIdVfnmaddps_NameIndex = 5574,
  kX86InstIdVfnmaddsd_NameIndex = 5584,
  kX86InstIdVfnmaddss_NameIndex = 5594,
  kX86InstIdVfnmsub132pd_NameIndex = 5604,
  kX86InstIdVfnmsub132ps_NameIndex = 5617,
  kX86InstIdVfnmsub132sd_NameIndex = 5630,
  kX86InstIdVfnmsub132ss_NameIndex = 5643,
  kX86InstIdVfnmsub213pd_NameIndex = 5656,
  kX86InstIdVfnmsub213ps_NameIndex = 5669,
  kX86InstIdVfnmsub213sd_NameIndex = 5682,
  kX86InstIdVfnmsub213ss_NameIndex = 5695,
  kX86InstIdVfnmsub231pd_NameIndex = 5708,
  kX86InstIdVfnmsub231ps_NameIndex = 5721,
  kX86InstIdVfnmsub231sd_NameIndex = 5734,
  kX86InstIdVfnmsub231ss_NameIndex = 5747,
  kX86InstIdVfnmsubpd_NameIndex = 5760,
  kX86InstIdVfnmsubps_NameIndex = 5770,
  kX86InstIdVfnmsubsd_NameIndex = 5780,
  kX86InstIdVfnmsubss_NameIndex = 5790,
  kX86InstIdVfrczpd_NameIndex = 5800,
  kX86InstIdVfrczps_NameIndex = 5808,
  kX86InstIdVfrczsd_NameIndex = 5816,
  kX86InstIdVfrczss_NameIndex = 5824,
  kX86InstIdVgatherdpd_NameIndex = 5832,
  kX86InstIdVgatherdps_NameIndex = 5843,
  kX86InstIdVgatherqpd_NameIndex = 5854,
  kX86InstIdVgatherqps_NameIndex = 5865,
  kX86InstIdVhaddpd_NameIndex = 5876,
  kX86InstIdVhaddps_NameIndex = 5884,
  kX86InstIdVhsubpd_NameIndex = 5892,
  kX86InstIdVhsubps_NameIndex = 5900,
  kX86InstIdVinsertf128_NameIndex = 5908,
  kX86InstIdVinserti128_NameIndex = 5920,
  kX86InstIdVinsertps_NameIndex = 5932,
  kX86InstIdVlddqu_NameIndex = 5942,
  kX86InstIdVldmxcsr_NameIndex = 5949,
  kX86InstIdVmaskmovdqu_NameIndex = 5958,
  kX86InstIdVmaskmovpd_NameIndex = 5970,
  kX86InstIdVmaskmovps_NameIndex = 5981,
  kX86InstIdVmaxpd_NameIndex = 5992,
  kX86InstIdVmaxps_NameIndex = 5999,
  kX86InstIdVmaxsd_NameIndex = 6006,
  kX86InstIdVmaxss_NameIndex = 6013,
  kX86InstIdVminpd_NameIndex = 6020,
  kX86InstIdVminps_NameIndex = 6027,
  kX86InstIdVminsd_NameIndex = 6034,
  kX86InstIdVminss_NameIndex = 6041,
  kX86InstIdVmovapd_NameIndex = 6048,
  kX86InstIdVmovaps_NameIndex = 6056,
  kX86InstIdVmovd_NameIndex = 6064,
  kX86InstIdVmovddup_NameIndex = 6070,
  kX86InstIdVmovdqa_NameIndex = 6079,
  kX86InstIdVmovdqu_NameIndex = 6087,
  kX86InstIdVmovhlps_NameIndex = 6095,
  kX86InstIdVmovhpd_NameIndex = 6104,
  kX86InstIdVmovhps_NameIndex = 6112,
  kX86InstIdVmovlhps_NameIndex = 6120,
  kX86InstIdVmovlpd_NameIndex = 6129,
  kX86InstIdVmovlps_NameIndex = 6137,
  kX86InstIdVmovmskpd_NameIndex = 6145,
  kX86InstIdVmovmskps_NameIndex = 6155,
  kX86InstIdVmovntdq_NameIndex = 6165,
  kX86InstIdVmovntdqa_NameIndex = 6174,
  kX86InstIdVmovntpd_NameIndex = 6184,
  kX86InstIdVmovntps_NameIndex = 6193,
  kX86InstIdVmovq_NameIndex = 6202,
  kX86InstIdVmovsd_NameIndex = 6208,
  kX86InstIdVmovshdup_NameIndex = 6215,
  kX86InstIdVmovsldup_NameIndex = 6225,
  kX86InstIdVmovss_NameIndex = 6235,
  kX86InstIdVmovupd_NameIndex = 6242,
  kX86InstIdVmovups_NameIndex = 6250,
  kX86InstIdVmpsadbw_NameIndex = 6258,
  kX86InstIdVmulpd_NameIndex = 6267,
  kX86InstIdVmulps_NameIndex = 6274,
  kX86InstIdVmulsd_NameIndex = 6281,
  kX86InstIdVmulss_NameIndex = 6288,
  kX86InstIdVorpd_NameIndex = 6295,
  kX86InstIdVorps_NameIndex = 6301,
  kX86InstIdVpabsb_NameIndex = 6307,
  kX86InstIdVpabsd_NameIndex = 6314,
  kX86InstIdVpabsw_NameIndex = 6321,
  kX86InstIdVpackssdw_NameIndex = 6328,
  kX86InstIdVpacksswb_NameIndex = 6338,
  kX86InstIdVpackusdw_NameIndex = 6348,
  kX86InstIdVpackuswb_NameIndex = 6358,
  kX86InstIdVpaddb_NameIndex = 6368,
  kX86InstIdVpaddd_NameIndex = 6375,
  kX86InstIdVpaddq_NameIndex = 6382,
  kX86InstIdVpaddsb_NameIndex = 6389,
  kX86InstIdVpaddsw_NameIndex = 6397,
  kX86InstIdVpaddusb_NameIndex = 6405,
  kX86InstIdVpaddusw_NameIndex = 6414,
  kX86InstIdVpaddw_NameIndex = 6423,
  kX86InstIdVpalignr_NameIndex = 6430,
  kX86InstIdVpand_NameIndex = 6439,
  kX86InstIdVpandn_NameIndex = 6445,
  kX86InstIdVpavgb_NameIndex = 6452,
  kX86InstIdVpavgw_NameIndex = 6459,
  kX86InstIdVpblendd_NameIndex = 6466,
  kX86InstIdVpblendvb_NameIndex = 6475,
  kX86InstIdVpblendw_NameIndex = 6485,
  kX86InstIdVpbroadcastb_NameIndex = 6494,
  kX86InstIdVpbroadcastd_NameIndex = 6507,
  kX86InstIdVpbroadcastq_NameIndex = 6520,
  kX86InstIdVpbroadcastw_NameIndex = 6533,
  kX86InstIdVpclmulqdq_NameIndex = 6546,
  kX86InstIdVpcmov_NameIndex = 6557,
  kX86InstIdVpcmpeqb_NameIndex = 6564,
  kX86InstIdVpcmpeqd_NameIndex = 6573,
  kX86InstIdVpcmpeqq_NameIndex = 6582,
  kX86InstIdVpcmpeqw_NameIndex = 6591,
  kX86InstIdVpcmpestri_NameIndex = 6600,
  kX86InstIdVpcmpestrm_NameIndex = 6611,
  kX86InstIdVpcmpgtb_NameIndex = 6622,
  kX86InstIdVpcmpgtd_NameIndex = 6631,
  kX86InstIdVpcmpgtq_NameIndex = 6640,
  kX86InstIdVpcmpgtw_NameIndex = 6649,
  kX86InstIdVpcmpistri_NameIndex = 6658,
  kX86InstIdVpcmpistrm_NameIndex = 6669,
  kX86InstIdVpcomb_NameIndex = 6680,
  kX86InstIdVpcomd_NameIndex = 6687,
  kX86InstIdVpcomq_NameIndex = 6694,
  kX86InstIdVpcomub_NameIndex = 6701,
  kX86InstIdVpcomud_NameIndex = 6709,
  kX86InstIdVpcomuq_NameIndex = 6717,
  kX86InstIdVpcomuw_NameIndex = 6725,
  kX86InstIdVpcomw_NameIndex = 6733,
  kX86InstIdVperm2f128_NameIndex = 6740,
  kX86InstIdVperm2i128_NameIndex = 6751,
  kX86InstIdVpermd_NameIndex = 6762,
  kX86InstIdVpermil2pd_NameIndex = 6769,
  kX86InstIdVpermil2ps_NameIndex = 6780,
  kX86InstIdVpermilpd_NameIndex = 6791,
  kX86InstIdVpermilps_NameIndex = 6801,
  kX86InstIdVpermpd_NameIndex = 6811,
  kX86InstIdVpermps_NameIndex = 6819,
  kX86InstIdVpermq_NameIndex = 6827,
  kX86InstIdVpextrb_NameIndex = 6834,
  kX86InstIdVpextrd_NameIndex = 6842,
  kX86InstIdVpextrq_NameIndex = 6850,
  kX86InstIdVpextrw_NameIndex = 6858,
  kX86InstIdVpgatherdd_NameIndex = 6866,
  kX86InstIdVpgatherdq_NameIndex = 6877,
  kX86InstIdVpgatherqd_NameIndex = 6888,
  kX86InstIdVpgatherqq_NameIndex = 6899,
  kX86InstIdVphaddbd_NameIndex = 6910,
  kX86InstIdVphaddbq_NameIndex = 6919,
  kX86InstIdVphaddbw_NameIndex = 6928,
  kX86InstIdVphaddd_NameIndex = 6937,
  kX86InstIdVphadddq_NameIndex = 6945,
  kX86InstIdVphaddsw_NameIndex = 6954,
  kX86InstIdVphaddubd_NameIndex = 6963,
  kX86InstIdVphaddubq_NameIndex = 6973,
  kX86InstIdVphaddubw_NameIndex = 6983,
  kX86InstIdVphaddudq_NameIndex = 6993,
  kX86InstIdVphadduwd_NameIndex = 7003,
  kX86InstIdVphadduwq_NameIndex = 7013,
  kX86InstIdVphaddw_NameIndex = 7023,
  kX86InstIdVphaddwd_NameIndex = 7031,
  kX86InstIdVphaddwq_NameIndex = 7040,
  kX86InstIdVphminposuw_NameIndex = 7049,
  kX86InstIdVphsubbw_NameIndex = 7061,
  kX86InstIdVphsubd_NameIndex = 7070,
  kX86InstIdVphsubdq_NameIndex = 7078,
  kX86InstIdVphsubsw_NameIndex = 7087,
  kX86InstIdVphsubw_NameIndex = 7096,
  kX86InstIdVphsubwd_NameIndex = 7104,
  kX86InstIdVpinsrb_NameIndex = 7113,
  kX86InstIdVpinsrd_NameIndex = 7121,
  kX86InstIdVpinsrq_NameIndex = 7129,
  kX86InstIdVpinsrw_NameIndex = 7137,
  kX86InstIdVpmacsdd_NameIndex = 7145,
  kX86InstIdVpmacsdqh_NameIndex = 7154,
  kX86InstIdVpmacsdql_NameIndex = 7164,
  kX86InstIdVpmacssdd_NameIndex = 7174,
  kX86InstIdVpmacssdqh_NameIndex = 7184,
  kX86InstIdVpmacssdql_NameIndex = 7195,
  kX86InstIdVpmacsswd_NameIndex = 7206,
  kX86InstIdVpmacssww_NameIndex = 7216,
  kX86InstIdVpmacswd_NameIndex = 7226,
  kX86InstIdVpmacsww_NameIndex = 7235,
  kX86InstIdVpmadcsswd_NameIndex = 7244,
  kX86InstIdVpmadcswd_NameIndex = 7255,
  kX86InstIdVpmaddubsw_NameIndex = 7265,
  kX86InstIdVpmaddwd_NameIndex = 7276,
  kX86InstIdVpmaskmovd_NameIndex = 7285,
  kX86InstIdVpmaskmovq_NameIndex = 7296,
  kX86InstIdVpmaxsb_NameIndex = 7307,
  kX86InstIdVpmaxsd_NameIndex = 7315,
  kX86InstIdVpmaxsw_NameIndex = 7323,
  kX86InstIdVpmaxub_NameIndex = 7331,
  kX86InstIdVpmaxud_NameIndex = 7339,
  kX86InstIdVpmaxuw_NameIndex = 7347,
  kX86InstIdVpminsb_NameIndex = 7355,
  kX86InstIdVpminsd_NameIndex = 7363,
  kX86InstIdVpminsw_NameIndex = 7371,
  kX86InstIdVpminub_NameIndex = 7379,
  kX86InstIdVpminud_NameIndex = 7387,
  kX86InstIdVpminuw_NameIndex = 7395,
  kX86InstIdVpmovmskb_NameIndex = 7403,
  kX86InstIdVpmovsxbd_NameIndex = 7413,
  kX86InstIdVpmovsxbq_NameIndex = 7423,
  kX86InstIdVpmovsxbw_NameIndex = 7433,
  kX86InstIdVpmovsxdq_NameIndex = 7443,
  kX86InstIdVpmovsxwd_NameIndex = 7453,
  kX86InstIdVpmovsxwq_NameIndex = 7463,
  kX86InstIdVpmovzxbd_NameIndex = 7473,
  kX86InstIdVpmovzxbq_NameIndex = 7483,
  kX86InstIdVpmovzxbw_NameIndex = 7493,
  kX86InstIdVpmovzxdq_NameIndex = 7503,
  kX86InstIdVpmovzxwd_NameIndex = 7513,
  kX86InstIdVpmovzxwq_NameIndex = 7523,
  kX86InstIdVpmuldq_NameIndex = 7533,
  kX86InstIdVpmulhrsw_NameIndex = 7541,
  kX86InstIdVpmulhuw_NameIndex = 7551,
  kX86InstIdVpmulhw_NameIndex = 7560,
  kX86InstIdVpmulld_NameIndex = 7568,
  kX86InstIdVpmullw_NameIndex = 7576,
  kX86InstIdVpmuludq_NameIndex = 7584,
  kX86InstIdVpor_NameIndex = 7593,
  kX86InstIdVpperm_NameIndex = 7598,
  kX86InstIdVprotb_NameIndex = 7605,
  kX86InstIdVprotd_NameIndex = 7612,
  kX86InstIdVprotq_NameIndex = 7619,
  kX86InstIdVprotw_NameIndex = 7626,
  kX86InstIdVpsadbw_NameIndex = 7633,
  kX86InstIdVpshab_NameIndex = 7641,
  kX86InstIdVpshad_NameIndex = 7648,
  kX86InstIdVpshaq_NameIndex = 7655,
  kX86InstIdVpshaw_NameIndex = 7662,
  kX86InstIdVpshlb_NameIndex = 7669,
  kX86InstIdVpshld_NameIndex = 7676,
  kX86InstIdVpshlq_NameIndex = 7683,
  kX86InstIdVpshlw_NameIndex = 7690,
  kX86InstIdVpshufb_NameIndex = 7697,
  kX86InstIdVpshufd_NameIndex = 7705,
  kX86InstIdVpshufhw_NameIndex = 7713,
  kX86InstIdVpshuflw_NameIndex = 7722,
  kX86InstIdVpsignb_NameIndex = 7731,
  kX86InstIdVpsignd_NameIndex = 7739,
  kX86InstIdVpsignw_NameIndex = 7747,
  kX86InstIdVpslld_NameIndex = 7755,
  kX86InstIdVpslldq_NameIndex = 7762,
  kX86InstIdVpsllq_NameIndex = 7770,
  kX86InstIdVpsllvd_NameIndex = 7777,
  kX86InstIdVpsllvq_NameIndex = 7785,
  kX86InstIdVpsllw_NameIndex = 7793,
  kX86InstIdVpsrad_NameIndex = 7800,
  kX86InstIdVpsravd_NameIndex = 7807,
  kX86InstIdVpsraw_NameIndex = 7815,
  kX86InstIdVpsrld_NameIndex = 7822,
  kX86InstIdVpsrldq_NameIndex = 7829,
  kX86InstIdVpsrlq_NameIndex = 7837,
  kX86InstIdVpsrlvd_NameIndex = 7844,
  kX86InstIdVpsrlvq_NameIndex = 7852,
  kX86InstIdVpsrlw_NameIndex = 7860,
  kX86InstIdVpsubb_NameIndex = 7867,
  kX86InstIdVpsubd_NameIndex = 7874,
  kX86InstIdVpsubq_NameIndex = 7881,
  kX86InstIdVpsubsb_NameIndex = 7888,
  kX86InstIdVpsubsw_NameIndex = 7896,
  kX86InstIdVpsubusb_NameIndex = 7904,
  kX86InstIdVpsubusw_NameIndex = 7913,
  kX86InstIdVpsubw_NameIndex = 7922,
  kX86InstIdVptest_NameIndex = 7929,
  kX86InstIdVpunpckhbw_NameIndex = 7936,
  kX86InstIdVpunpckhdq_NameIndex = 7947,
  kX86InstIdVpunpckhqdq_NameIndex = 7958,
  kX86InstIdVpunpckhwd_NameIndex = 7970,
  kX86InstIdVpunpcklbw_NameIndex = 7981,
  kX86InstIdVpunpckldq_NameIndex = 7992,
  kX86InstIdVpunpcklqdq_NameIndex = 8003,
  kX86InstIdVpunpcklwd_NameIndex = 8015,
  kX86InstIdVpxor_NameIndex = 8026,
  kX86InstIdVrcpps_NameIndex = 8032,
  kX86InstIdVrcpss_NameIndex = 8039,
  kX86InstIdVroundpd_NameIndex = 8046,
  kX86InstIdVroundps_NameIndex = 8055,
  kX86InstIdVroundsd_NameIndex = 8064,
  kX86InstIdVroundss_NameIndex = 8073,
  kX86InstIdVrsqrtps_NameIndex = 8082,
  kX86InstIdVrsqrtss_NameIndex = 8091,
  kX86InstIdVshufpd_NameIndex = 8100,
  kX86InstIdVshufps_NameIndex = 8108,
  kX86InstIdVsqrtpd_NameIndex = 8116,
  kX86InstIdVsqrtps_NameIndex = 8124,
  kX86InstIdVsqrtsd_NameIndex = 8132,
  kX86InstIdVsqrtss_NameIndex = 8140,
  kX86InstIdVstmxcsr_NameIndex = 8148,
  kX86InstIdVsubpd_NameIndex = 8157,
  kX86InstIdVsubps_NameIndex = 8164,
  kX86InstIdVsubsd_NameIndex = 8171,
  kX86InstIdVsubss_NameIndex = 8178,
  kX86InstIdVtestpd_NameIndex = 8185,
  kX86InstIdVtestps_NameIndex = 8193,
  kX86InstIdVucomisd_NameIndex = 8201,
  kX86InstIdVucomiss_NameIndex = 8210,
  kX86InstIdVunpckhpd_NameIndex = 8219,
  kX86InstIdVunpckhps_NameIndex = 8229,
  kX86InstIdVunpcklpd_NameIndex = 8239,
  kX86InstIdVunpcklps_NameIndex = 8249,
  kX86InstIdVxorpd_NameIndex = 8259,
  kX86InstIdVxorps_NameIndex = 8266,
  kX86InstIdVzeroall_NameIndex = 8273,
  kX86InstIdVzeroupper_NameIndex = 8282,
  kX86InstIdWrfsbase_NameIndex = 8293,
  kX86InstIdWrgsbase_NameIndex = 8302,
  kX86InstIdXadd_NameIndex = 8311,
  kX86InstIdXchg_NameIndex = 8316,
  kX86InstIdXgetbv_NameIndex = 8321,
  kX86InstIdXor_NameIndex = 8328,
  kX86InstIdXorpd_NameIndex = 8332,
  kX86InstIdXorps_NameIndex = 8338,
  kX86InstIdXrstor_NameIndex = 8344,
  kX86InstIdXrstor64_NameIndex = 8351,
  kX86InstIdXsave_NameIndex = 8360,
  kX86InstIdXsave64_NameIndex = 8366,
  kX86InstIdXsaveopt_NameIndex = 8374,
  kX86InstIdXsaveopt64_NameIndex = 8383,
  kX86InstIdXsetbv_NameIndex = 8394
};
#endif // !ASMJIT_DISABLE_NAMES

// Automatically generated, do not edit.
const X86InstExtendedInfo _x86InstExtendedInfo[] = {
  { Enc(None)         , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Arith)     , 0 , 0 , 0x20, 0x3F, 0, { O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                  }, F(Lock)                            , U                    },
  { Enc(X86Arith)     , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                  }, F(Lock)                            , U                    },
  { Enc(ExtRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(AvxRvm)       , 0 , 0 , 0x00, 0x3F, 0, { O(Gqd)            , O(Gqd)            , O(GqdMem)         , U                 , U                  }, F(None)                            , U                    },
  { Enc(AvxRmv)       , 0 , 0 , 0x00, 0x3F, 0, { O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(AvxVm)        , 0 , 0 , 0x00, 0x3F, 0, { O(Gqd)            , O(GqdMem)         , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x00, 0x3F, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86BSwap)     , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86BTest)     , 0 , 0 , 0x00, 0x3B, 0, { O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                  }, F(Test)                            , O_000F00(BA,4,_,_,_) },
  { Enc(X86BTest)     , 0 , 0 , 0x00, 0x3B, 0, { O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                  }, F(Lock)                            , O_000F00(BA,7,_,_,_) },
  { Enc(X86BTest)     , 0 , 0 , 0x00, 0x3B, 0, { O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                  }, F(Lock)                            , O_000F00(BA,6,_,_,_) },
  { Enc(X86BTest)     , 0 , 0 , 0x00, 0x3B, 0, { O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                  }, F(Lock)                            , O_000F00(BA,5,_,_,_) },
  { Enc(X86Call)      , 0 , 0 , 0x00, 0x00, 0, { O(GqdMem)|O(LbImm), U                 , U                 , U                 , U                  }, F(Flow)                            , O_000000(E8,U,_,_,_) },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x20, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x40, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86M)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x20, 0x20, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x24, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x20, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x04, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x07, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x03, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x01, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x10, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86RegRm)     , 0 , 0 , 0x02, 0x00, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Arith)     , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                  }, F(Test)                            , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x40, 0x3F, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Op_66H)    , 0 , 0 , 0x40, 0x3F, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86RmReg)     , 0 , 0 , 0x00, 0x3F, 0, { U                 , U                 , U                 , U                 , U                  }, F(Lock)|F(Special)                 , U                    },
  { Enc(X86M)         , 0 , 0 , 0x00, 0x04, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(ExtRm)        , 0 , 0 , 0x00, 0x3F, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Test)                            , U                    },
  { Enc(ExtCrc)       , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(GqdwbMem)       , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRm)        , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm)        , 0 , 8 , 0x00, 0x00, 0, { O(Mm)             , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm)        , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(MmMem)          , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm)        , 0 , 8 , 0x00, 0x00, 0, { O(Xmm)            , O(MmMem)          , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm_Q)      , 0 , 8 , 0x00, 0x00, 0, { O(Gqd)            , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm)        , 0 , 4 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm_Q)      , 0 , 8 , 0x00, 0x00, 0, { O(Xmm)            , O(GqdMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm_Q)      , 0 , 4 , 0x00, 0x00, 0, { O(Xmm)            , O(GqdMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRm)        , 0 , 8 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x28, 0x3F, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86IncDec)    , 0 , 0 , 0x00, 0x1F, 0, { O(GqdwbMem)       , U                 , U                 , U                 , U                  }, F(Lock)                            , O_000000(48,U,_,_,_) },
  { Enc(X86Rm_B)      , 0 , 0 , 0x00, 0x3F, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Enter)     , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(ExtExtract)   , 0 , 8 , 0x00, 0x00, 0, { O(GqdMem)         , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_660F3A(17,U,_,_,_) },
  { Enc(ExtExtrq)     , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)|O(Imm)     , O(None)|O(Imm)    , U                 , U                  }, F(None)                            , O_660F00(78,0,_,_,_) },
  { Enc(FpuOp)        , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuArith)     , 0 , 0 , 0x00, 0x00, 0, { O(FpMem)          , O(Fp)             , U                 , U                 , U                  }, F(Fp)|F(Mem4_8)                    , U                    },
  { Enc(FpuRDef)      , 0 , 0 , 0x00, 0x00, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(X86M)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuR)         , 0 , 0 , 0x20, 0x00, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuR)         , 0 , 0 , 0x24, 0x00, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuR)         , 0 , 0 , 0x04, 0x00, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuR)         , 0 , 0 , 0x10, 0x00, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuCom)       , 0 , 0 , 0x00, 0x00, 0, { O(Fp)|O(Mem)      , O(Fp)             , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuR)         , 0 , 0 , 0x00, 0x3F, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuR)         , 0 , 0 , 0x00, 0x00, 0, { O(Fp)             , U                 , U                 , U                 , U                  }, F(Fp)                              , U                    },
  { Enc(FpuM)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem2_4)                    , U                    },
  { Enc(FpuM)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem2_4_8)                  , O_000000(DF,5,_,_,_) },
  { Enc(FpuM)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem2_4_8)                  , O_000000(DF,7,_,_,_) },
  { Enc(FpuM)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem2_4_8)                  , O_000000(DD,1,_,_,_) },
  { Enc(FpuFldFst)    , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem4_8_10)                 , O_000000(DB,5,_,_,_) },
  { Enc(FpuStsw)      , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)                              , O_00_X(DFE0,U)       },
  { Enc(FpuFldFst)    , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem4_8)                    , U                    },
  { Enc(FpuFldFst)    , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)|F(Mem4_8_10)                 , O_000000(DB,7,_,_,_) },
  { Enc(FpuStsw)      , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Fp)                              , O_9B_X(DFE0,U)       },
  { Enc(X86Rm_B)      , 0 , 0 , 0x00, 0x3F, 0, { 0                 , 0                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Imul)      , 0 , 0 , 0x00, 0x3F, 0, { 0                 , 0                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86IncDec)    , 0 , 0 , 0x00, 0x1F, 0, { O(GqdwbMem)       , U                 , U                 , U                 , U                  }, F(Lock)                            , O_000000(40,U,_,_,_) },
  { Enc(ExtInsertq)   , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(None)|O(Imm)    , O(None)|O(Imm)    , U                  }, F(None)                            , O_F20F00(78,U,_,_,_) },
  { Enc(X86Int)       , 0 , 0 , 0x00, 0x80, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x24, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x20, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x04, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x07, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x03, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x01, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x10, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jcc)       , 0 , 0 , 0x02, 0x00, 0, { O(Label)          , U                 , U                 , U                 , U                  }, F(Flow)                            , U                    },
  { Enc(X86Jecxz)     , 0 , 0 , 0x00, 0x00, 0, { O(Gqdw)           , O(Label)          , U                 , U                 , U                  }, F(Flow)|F(Special)                 , U                    },
  { Enc(X86Jmp)       , 0 , 0 , 0x00, 0x00, 0, { O(Label)|O(Imm)   , U                 , U                 , U                 , U                  }, F(Flow)                            , O_000000(E9,U,_,_,_) },
  { Enc(X86Op)        , 0 , 0 , 0x3E, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(ExtRm)        , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(Mem)            , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Lea)       , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(Mem)            , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtFence)     , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Op)        , 0 , 1 , 0x40, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Move)|F(Special)                 , U                    },
  { Enc(X86Op)        , 0 , 4 , 0x40, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Move)|F(Special)                 , U                    },
  { Enc(X86Op)        , 0 , 8 , 0x40, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Move)|F(Special)                 , U                    },
  { Enc(X86Op_66H)    , 0 , 2 , 0x40, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Move)|F(Special)                 , U                    },
  { Enc(ExtRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(ExtRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Mm)             , O(Mm)             , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Mov)       , 0 , 0 , 0x00, 0x00, 0, { O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86MovPtr)    , 0 , 0 , 0x00, 0x00, 0, { O(Gqdwb)          , O(Imm)            , U                 , U                 , U                  }, F(Move)|F(Special)                 , O_000000(A2,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_660F00(29,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_000F00(29,U,_,_,_) },
  { Enc(ExtMovBe)     , 0 , 0 , 0x00, 0x00, 0, { O(GqdwMem)        , O(GqdwMem)        , U                 , U                 , U                  }, F(Move)                            , O_000F38(F1,U,_,_,_) },
  { Enc(ExtMovD)      , 0 , 16, 0x00, 0x00, 0, { O(Gd)|O(MmXmmMem) , O(Gd)|O(MmXmmMem) , U                 , U                 , U                  }, F(Move)                            , O_000F00(7E,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(Mm)             , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_660F00(7F,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_F30F00(7F,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtMov)       , 8 , 8 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(None)                            , O_660F00(17,U,_,_,_) },
  { Enc(ExtMov)       , 8 , 8 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(None)                            , O_000F00(17,U,_,_,_) },
  { Enc(ExtMov)       , 8 , 8 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_660F00(13,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_000F00(13,U,_,_,_) },
  { Enc(ExtMovNoRexW) , 0 , 8 , 0x00, 0x00, 0, { O(Gqd)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(Mem)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_660F00(E7,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(Mem)            , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(Mem)            , O(Gqd)            , U                 , U                 , U                  }, F(Move)                            , O_000F00(C3,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(Mem)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_660F00(2B,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(Mem)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_000F00(2B,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(Mem)            , O(Mm)             , U                 , U                 , U                  }, F(Move)                            , O_000F00(E7,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(Mem)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_F20F00(2B,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 4 , 0x00, 0x00, 0, { O(Mem)            , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_F30F00(2B,U,_,_,_) },
  { Enc(ExtMovQ)      , 0 , 16, 0x00, 0x00, 0, { O(Gq)|O(MmXmmMem) , O(Gq)|O(MmXmmMem) , U                 , U                 , U                  }, F(Move)                            , O_000F00(7E,U,_,W,_) },
  { Enc(ExtRm)        , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(Mm)             , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Move)|F(Special)                 , U                    },
  { Enc(X86Op_66H)    , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(ExtMov)       , 0 , 8 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)           |F(Z)            , O_F20F00(11,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 4 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)           |F(Z)            , O_F30F00(11,U,_,_,_) },
  { Enc(X86MovSxZx)   , 0 , 0 , 0x00, 0x00, 0, { O(Gqdw)           , O(GwbMem)         , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86MovSxd)    , 0 , 0 , 0x00, 0x00, 0, { O(Gq)             , O(GdMem)          , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_660F00(11,U,_,_,_) },
  { Enc(ExtMov)       , 0 , 16, 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Move)                            , O_000F00(11,U,_,_,_) },
  { Enc(AvxRvm)       , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(Gqd)            , O(GqdMem)         , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Rm_B)      , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , U                 , U                 , U                 , U                  }, F(Lock)                            , U                    },
  { Enc(X86Rm_B)      , 0 , 0 , 0x00, 0x00, 0, { O(GqdwbMem)       , U                 , U                 , U                 , U                  }, F(Lock)                            , U                    },
  { Enc(ExtRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi_P)     , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)       , O(Imm)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtExtract)   , 0 , 8 , 0x00, 0x00, 0, { O(Gd)|O(Gb)|O(Mem), O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_000F3A(14,U,_,_,_) },
  { Enc(ExtExtract)   , 0 , 8 , 0x00, 0x00, 0, { O(GdMem)          , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_000F3A(16,U,_,_,_) },
  { Enc(ExtExtract)   , 0 , 8 , 0x00, 0x00, 0, { O(GqdMem)         , O(Xmm)            , U                 , U                 , U                  }, F(Move)                            , O_000F3A(16,U,_,W,_) },
  { Enc(ExtExtract)   , 0 , 8 , 0x00, 0x00, 0, { O(GdMem)          , O(MmXmm)          , U                 , U                 , U                  }, F(Move)                            , O_000F3A(15,U,_,_,_) },
  { Enc(3dNow)        , 0 , 0 , 0x00, 0x00, 0, { O(Mm)             , O(MmMem)          , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(GdMem)          , O(Imm)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(GqMem)          , O(Imm)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi_P)     , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(GdMem)          , O(Imm)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRm_PQ)     , 0 , 8 , 0x00, 0x00, 0, { O(Gqd)            , O(MmXmm)          , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Pop)       , 0 , 0 , 0x00, 0x00, 0, { 0                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , O_000000(58,U,_,_,_) },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0xFF, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(ExtPrefetch)  , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , O(Imm)            , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi)       , 0 , 16, 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRmi_P)     , 0 , 8 , 0x00, 0x00, 0, { O(Mm)             , O(MmMem)          , O(Imm)            , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(72,6,_,_,_) },
  { Enc(ExtRmRi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Imm)            , U                 , U                 , U                  }, F(None)                            , O_660F00(73,7,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(73,6,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(71,6,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(72,4,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(71,4,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(72,2,_,_,_) },
  { Enc(ExtRmRi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Imm)            , U                 , U                 , U                  }, F(None)                            , O_660F00(73,3,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(73,2,_,_,_) },
  { Enc(ExtRmRi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                  }, F(None)                            , O_000F00(71,2,_,_,_) },
  { Enc(X86Push)      , 0 , 0 , 0x00, 0x00, 0, { 0                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , O_000000(50,U,_,_,_) },
  { Enc(X86Op)        , 0 , 0 , 0xFF, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Rot)       , 0 , 0 , 0x20, 0x21, 0, { O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Rm)        , 0 , 8 , 0x00, 0x00, 0, { O(Gqd)            , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Rm)        , 0 , 8 , 0x00, 0x3F, 0, { O(Gqdw)           , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Rep)       , 0 , 0 , 0x40, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Rep)       , 0 , 0 , 0x40, 0x00, 0, { O(Mem)            , O(Mem)            , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Rep)       , 0 , 0 , 0x40, 0x3F, 0, { O(Mem)            , O(Mem)            , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Ret)       , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Rot)       , 0 , 0 , 0x00, 0x21, 0, { O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(AvxRmi)       , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(GqdMem)         , O(Imm)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(ExtRmi)       , 0 , 8 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                  }, F(Move)                            , U                    },
  { Enc(ExtRmi)       , 0 , 4 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x00, 0x3E, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Rot)       , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(AvxRmv)       , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x24, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x20, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x04, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x07, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x03, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x01, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x10, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Set)       , 0 , 1 , 0x02, 0x00, 0, { O(GbMem)          , U                 , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(X86Shlrd)     , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(Gb)             , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Shlrd)     , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(Gqdwb)          , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Op)        , 0 , 0 , 0x40, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Op_66H)    , 0 , 0 , 0x40, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    },
  { Enc(X86Test)      , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(Gqdwb)|O(Imm)   , U                 , U                 , U                  }, F(Test)                            , O_000000(F6,U,_,_,_) },
  { Enc(X86RegRm)     , 0 , 0 , 0x00, 0x3F, 0, { O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                  }, F(Move)                            , U                    },
  { Enc(AvxRvm_P)     , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvm)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRmi)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi_P)    , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , O(Xy)             , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Ymm)            , O(Mem)            , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Ymm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMri_P)     , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Xy)             , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvm)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(GqdMem)         , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMri)       , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Ymm)            , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMri)       , 0 , 0 , 0x00, 0x00, 0, { O(GqdMem)         , O(Xmm)            , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(Fma4_P)       , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                  }, F(Avx)                             , U                    },
  { Enc(Fma4)         , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                  }, F(Avx)                             , U                    },
  { Enc(XopRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(XopRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxGather)    , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxGatherEx)  , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Mem)            , O(Xmm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Ymm)            , O(Ymm)            , O(XmmMem)         , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Mem)            , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxM)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmMvr_P)  , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                  }, F(Avx)                             , O_660F38(2F,U,_,_,_) },
  { Enc(AvxRvmMvr_P)  , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                  }, F(Avx)                             , O_660F38(2E,U,_,_,_) },
  { Enc(AvxRmMr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , O_660F00(29,U,_,_,_) },
  { Enc(AvxRmMr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , O_000F00(29,U,_,_,_) },
  { Enc(AvxRmMr)      , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , O_660F00(7E,U,_,_,_) },
  { Enc(AvxRmMr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , O_660F00(7F,U,_,_,_) },
  { Enc(AvxRmMr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , O_F30F00(7F,U,_,_,_) },
  { Enc(AvxRvm)       , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(Xmm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmMr)     , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                  }, F(Avx)                             , O_660F00(17,U,_,_,_) },
  { Enc(AvxRvmMr)     , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                  }, F(Avx)                             , O_000F00(17,U,_,_,_) },
  { Enc(AvxRvmMr)     , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                  }, F(Avx)                             , O_660F00(13,U,_,_,_) },
  { Enc(AvxRvmMr)     , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                  }, F(Avx)                             , O_000F00(13,U,_,_,_) },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , O(Xy)             , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMr)        , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , O(Xy)             , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMr_P)      , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , O(Xy)             , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMovSsSd)   , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(XmmMem)         , O(Xmm)            , U                 , U                  }, F(Avx)                             , O_F20F00(11,U,_,_,_) },
  { Enc(AvxMovSsSd)   , 0 , 0 , 0x00, 0x00, 0, { O(XmmMem)         , O(Xmm)            , O(Xmm)            , U                 , U                  }, F(Avx)                             , O_F30F00(11,U,_,_,_) },
  { Enc(AvxRmMr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , O_660F00(11,U,_,_,_) },
  { Enc(AvxRmMr_P)    , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , O_000F00(11,U,_,_,_) },
  { Enc(AvxRvmr)      , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , O(Xy)             , U                  }, F(Avx)                             , U                    },
  { Enc(XopRvrmRvmr_P), 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                  }, F(Avx)                             , U                    },
  { Enc(XopRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Ymm)            , O(Ymm)            , O(YmmMem)         , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvm)       , 0 , 0 , 0x00, 0x00, 0, { O(Ymm)            , O(Ymm)            , O(YmmMem)         , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvrmRvmr_P), 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmRmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F3A(05,U,_,_,_) },
  { Enc(AvxRvmRmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F3A(04,U,_,_,_) },
  { Enc(AvxRmi)       , 0 , 0 , 0x00, 0x00, 0, { O(Ymm)            , O(YmmMem)         , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMri)       , 0 , 0 , 0x00, 0x00, 0, { O(GqdwbMem)       , O(Xmm)            , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMri)       , 0 , 0 , 0x00, 0x00, 0, { O(GqMem)          , O(Xmm)            , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxMri)       , 0 , 0 , 0x00, 0x00, 0, { O(GqdwMem)        , O(Xmm)            , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(GqdwbMem)       , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(GqdMem)         , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(GqMem)          , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmi)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(GqdwMem)        , O(Imm)            , U                  }, F(Avx)                             , U                    },
  { Enc(XopRvmr)      , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmMvr_P)  , 0 , 0 , 0x00, 0x00, 0, { O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                  }, F(Avx)                             , O_660F38(8E,U,_,_,_) },
  { Enc(XopRvrmRvmr)  , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                  }, F(Avx)                             , U                    },
  { Enc(XopRvmRmi)    , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                  }, F(Avx)                             , O_00_M08(C0,U,_,_,_) },
  { Enc(XopRvmRmi)    , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                  }, F(Avx)                             , O_00_M08(C2,U,_,_,_) },
  { Enc(XopRvmRmi)    , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                  }, F(Avx)                             , O_00_M08(C3,U,_,_,_) },
  { Enc(XopRvmRmi)    , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                  }, F(Avx)                             , O_00_M08(C1,U,_,_,_) },
  { Enc(XopRvmRmv)    , 0 , 0 , 0x00, 0x00, 0, { O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRmi_P)     , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(72,6,_,_,_) },
  { Enc(AvxVmi_P)     , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(73,6,_,_,_) },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(71,6,_,_,_) },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(72,4,_,_,_) },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(71,4,_,_,_) },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(72,2,_,_,_) },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(73,2,_,_,_) },
  { Enc(AvxRvmVmi_P)  , 0 , 0 , 0x00, 0x00, 0, { O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                  }, F(Avx)                             , O_660F00(71,2,_,_,_) },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x3F, 0, { O(Xy)             , O(XyMem)          , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxRm_P)      , 0 , 0 , 0x00, 0x3F, 0, { O(Xy)             , O(XyMem)          , U                 , U                 , U                  }, F(Test)                            , U                    },
  { Enc(AvxRm)        , 0 , 0 , 0x00, 0x3F, 0, { O(Xmm)            , O(XmmMem)         , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(AvxOp)        , 0 , 0 , 0x00, 0x00, 0, { U                 , U                 , U                 , U                 , U                  }, F(Avx)                             , U                    },
  { Enc(X86Rm)        , 0 , 0 , 0x00, 0x00, 0, { O(Gqd)            , U                 , U                 , U                 , U                  }, F(None)                            , U                    },
  { Enc(X86Xadd)      , 0 , 0 , 0x00, 0x3F, 0, { O(GqdwbMem)       , O(Gqdwb)          , U                 , U                 , U                  }, F(Xchg)|F(Lock)                    , U                    },
  { Enc(X86Xchg)      , 0 , 0 , 0x00, 0x00, 0, { O(GqdwbMem)       , O(Gqdwb)          , U                 , U                 , U                  }, F(Xchg)|F(Lock)                    , U                    },
  { Enc(X86M)         , 0 , 0 , 0x00, 0x00, 0, { O(Mem)            , U                 , U                 , U                 , U                  }, F(None)|F(Special)                 , U                    }
};

// Automatically generated, do not edit.
enum kX86InstData_ExtendedIndex {
  kInstIdNone_ExtendedIndex = 0,
  kX86InstIdAdc_ExtendedIndex = 1,
  kX86InstIdAdd_ExtendedIndex = 2,
  kX86InstIdAddpd_ExtendedIndex = 3,
  kX86InstIdAddps_ExtendedIndex = 3,
  kX86InstIdAddsd_ExtendedIndex = 3,
  kX86InstIdAddss_ExtendedIndex = 3,
  kX86InstIdAddsubpd_ExtendedIndex = 3,
  kX86InstIdAddsubps_ExtendedIndex = 3,
  kX86InstIdAesdec_ExtendedIndex = 3,
  kX86InstIdAesdeclast_ExtendedIndex = 3,
  kX86InstIdAesenc_ExtendedIndex = 3,
  kX86InstIdAesenclast_ExtendedIndex = 3,
  kX86InstIdAesimc_ExtendedIndex = 3,
  kX86InstIdAeskeygenassist_ExtendedIndex = 4,
  kX86InstIdAnd_ExtendedIndex = 2,
  kX86InstIdAndn_ExtendedIndex = 5,
  kX86InstIdAndnpd_ExtendedIndex = 3,
  kX86InstIdAndnps_ExtendedIndex = 3,
  kX86InstIdAndpd_ExtendedIndex = 3,
  kX86InstIdAndps_ExtendedIndex = 3,
  kX86InstIdBextr_ExtendedIndex = 6,
  kX86InstIdBlendpd_ExtendedIndex = 4,
  kX86InstIdBlendps_ExtendedIndex = 4,
  kX86InstIdBlendvpd_ExtendedIndex = 7,
  kX86InstIdBlendvps_ExtendedIndex = 7,
  kX86InstIdBlsi_ExtendedIndex = 8,
  kX86InstIdBlsmsk_ExtendedIndex = 8,
  kX86InstIdBlsr_ExtendedIndex = 8,
  kX86InstIdBsf_ExtendedIndex = 9,
  kX86InstIdBsr_ExtendedIndex = 9,
  kX86InstIdBswap_ExtendedIndex = 10,
  kX86InstIdBt_ExtendedIndex = 11,
  kX86InstIdBtc_ExtendedIndex = 12,
  kX86InstIdBtr_ExtendedIndex = 13,
  kX86InstIdBts_ExtendedIndex = 14,
  kX86InstIdBzhi_ExtendedIndex = 6,
  kX86InstIdCall_ExtendedIndex = 15,
  kX86InstIdCbw_ExtendedIndex = 16,
  kX86InstIdCdq_ExtendedIndex = 16,
  kX86InstIdCdqe_ExtendedIndex = 16,
  kX86InstIdClc_ExtendedIndex = 17,
  kX86InstIdCld_ExtendedIndex = 18,
  kX86InstIdClflush_ExtendedIndex = 19,
  kX86InstIdCmc_ExtendedIndex = 20,
  kX86InstIdCmova_ExtendedIndex = 21,
  kX86InstIdCmovae_ExtendedIndex = 22,
  kX86InstIdCmovb_ExtendedIndex = 22,
  kX86InstIdCmovbe_ExtendedIndex = 21,
  kX86InstIdCmovc_ExtendedIndex = 22,
  kX86InstIdCmove_ExtendedIndex = 23,
  kX86InstIdCmovg_ExtendedIndex = 24,
  kX86InstIdCmovge_ExtendedIndex = 25,
  kX86InstIdCmovl_ExtendedIndex = 25,
  kX86InstIdCmovle_ExtendedIndex = 24,
  kX86InstIdCmovna_ExtendedIndex = 21,
  kX86InstIdCmovnae_ExtendedIndex = 22,
  kX86InstIdCmovnb_ExtendedIndex = 22,
  kX86InstIdCmovnbe_ExtendedIndex = 21,
  kX86InstIdCmovnc_ExtendedIndex = 22,
  kX86InstIdCmovne_ExtendedIndex = 23,
  kX86InstIdCmovng_ExtendedIndex = 24,
  kX86InstIdCmovnge_ExtendedIndex = 25,
  kX86InstIdCmovnl_ExtendedIndex = 25,
  kX86InstIdCmovnle_ExtendedIndex = 24,
  kX86InstIdCmovno_ExtendedIndex = 26,
  kX86InstIdCmovnp_ExtendedIndex = 27,
  kX86InstIdCmovns_ExtendedIndex = 28,
  kX86InstIdCmovnz_ExtendedIndex = 23,
  kX86InstIdCmovo_ExtendedIndex = 26,
  kX86InstIdCmovp_ExtendedIndex = 27,
  kX86InstIdCmovpe_ExtendedIndex = 27,
  kX86InstIdCmovpo_ExtendedIndex = 27,
  kX86InstIdCmovs_ExtendedIndex = 28,
  kX86InstIdCmovz_ExtendedIndex = 23,
  kX86InstIdCmp_ExtendedIndex = 29,
  kX86InstIdCmppd_ExtendedIndex = 4,
  kX86InstIdCmpps_ExtendedIndex = 4,
  kX86InstIdCmpsB_ExtendedIndex = 30,
  kX86InstIdCmpsD_ExtendedIndex = 30,
  kX86InstIdCmpsQ_ExtendedIndex = 30,
  kX86InstIdCmpsW_ExtendedIndex = 31,
  kX86InstIdCmpsd_ExtendedIndex = 4,
  kX86InstIdCmpss_ExtendedIndex = 4,
  kX86InstIdCmpxchg_ExtendedIndex = 32,
  kX86InstIdCmpxchg16b_ExtendedIndex = 33,
  kX86InstIdCmpxchg8b_ExtendedIndex = 33,
  kX86InstIdComisd_ExtendedIndex = 34,
  kX86InstIdComiss_ExtendedIndex = 34,
  kX86InstIdCpuid_ExtendedIndex = 16,
  kX86InstIdCqo_ExtendedIndex = 16,
  kX86InstIdCrc32_ExtendedIndex = 35,
  kX86InstIdCvtdq2pd_ExtendedIndex = 36,
  kX86InstIdCvtdq2ps_ExtendedIndex = 36,
  kX86InstIdCvtpd2dq_ExtendedIndex = 36,
  kX86InstIdCvtpd2pi_ExtendedIndex = 37,
  kX86InstIdCvtpd2ps_ExtendedIndex = 36,
  kX86InstIdCvtpi2pd_ExtendedIndex = 38,
  kX86InstIdCvtpi2ps_ExtendedIndex = 39,
  kX86InstIdCvtps2dq_ExtendedIndex = 36,
  kX86InstIdCvtps2pd_ExtendedIndex = 36,
  kX86InstIdCvtps2pi_ExtendedIndex = 37,
  kX86InstIdCvtsd2si_ExtendedIndex = 40,
  kX86InstIdCvtsd2ss_ExtendedIndex = 41,
  kX86InstIdCvtsi2sd_ExtendedIndex = 42,
  kX86InstIdCvtsi2ss_ExtendedIndex = 43,
  kX86InstIdCvtss2sd_ExtendedIndex = 44,
  kX86InstIdCvtss2si_ExtendedIndex = 40,
  kX86InstIdCvttpd2dq_ExtendedIndex = 36,
  kX86InstIdCvttpd2pi_ExtendedIndex = 37,
  kX86InstIdCvttps2dq_ExtendedIndex = 36,
  kX86InstIdCvttps2pi_ExtendedIndex = 37,
  kX86InstIdCvttsd2si_ExtendedIndex = 40,
  kX86InstIdCvttss2si_ExtendedIndex = 40,
  kX86InstIdCwd_ExtendedIndex = 16,
  kX86InstIdCwde_ExtendedIndex = 16,
  kX86InstIdDaa_ExtendedIndex = 45,
  kX86InstIdDas_ExtendedIndex = 45,
  kX86InstIdDec_ExtendedIndex = 46,
  kX86InstIdDiv_ExtendedIndex = 47,
  kX86InstIdDivpd_ExtendedIndex = 3,
  kX86InstIdDivps_ExtendedIndex = 3,
  kX86InstIdDivsd_ExtendedIndex = 3,
  kX86InstIdDivss_ExtendedIndex = 3,
  kX86InstIdDppd_ExtendedIndex = 4,
  kX86InstIdDpps_ExtendedIndex = 4,
  kX86InstIdEmms_ExtendedIndex = 48,
  kX86InstIdEnter_ExtendedIndex = 49,
  kX86InstIdExtractps_ExtendedIndex = 50,
  kX86InstIdExtrq_ExtendedIndex = 51,
  kX86InstIdF2xm1_ExtendedIndex = 52,
  kX86InstIdFabs_ExtendedIndex = 52,
  kX86InstIdFadd_ExtendedIndex = 53,
  kX86InstIdFaddp_ExtendedIndex = 54,
  kX86InstIdFbld_ExtendedIndex = 55,
  kX86InstIdFbstp_ExtendedIndex = 55,
  kX86InstIdFchs_ExtendedIndex = 52,
  kX86InstIdFclex_ExtendedIndex = 52,
  kX86InstIdFcmovb_ExtendedIndex = 56,
  kX86InstIdFcmovbe_ExtendedIndex = 57,
  kX86InstIdFcmove_ExtendedIndex = 58,
  kX86InstIdFcmovnb_ExtendedIndex = 56,
  kX86InstIdFcmovnbe_ExtendedIndex = 57,
  kX86InstIdFcmovne_ExtendedIndex = 58,
  kX86InstIdFcmovnu_ExtendedIndex = 59,
  kX86InstIdFcmovu_ExtendedIndex = 59,
  kX86InstIdFcom_ExtendedIndex = 60,
  kX86InstIdFcomi_ExtendedIndex = 61,
  kX86InstIdFcomip_ExtendedIndex = 61,
  kX86InstIdFcomp_ExtendedIndex = 60,
  kX86InstIdFcompp_ExtendedIndex = 52,
  kX86InstIdFcos_ExtendedIndex = 52,
  kX86InstIdFdecstp_ExtendedIndex = 52,
  kX86InstIdFdiv_ExtendedIndex = 53,
  kX86InstIdFdivp_ExtendedIndex = 54,
  kX86InstIdFdivr_ExtendedIndex = 53,
  kX86InstIdFdivrp_ExtendedIndex = 54,
  kX86InstIdFemms_ExtendedIndex = 62,
  kX86InstIdFfree_ExtendedIndex = 63,
  kX86InstIdFiadd_ExtendedIndex = 64,
  kX86InstIdFicom_ExtendedIndex = 64,
  kX86InstIdFicomp_ExtendedIndex = 64,
  kX86InstIdFidiv_ExtendedIndex = 64,
  kX86InstIdFidivr_ExtendedIndex = 64,
  kX86InstIdFild_ExtendedIndex = 65,
  kX86InstIdFimul_ExtendedIndex = 64,
  kX86InstIdFincstp_ExtendedIndex = 52,
  kX86InstIdFinit_ExtendedIndex = 52,
  kX86InstIdFist_ExtendedIndex = 64,
  kX86InstIdFistp_ExtendedIndex = 66,
  kX86InstIdFisttp_ExtendedIndex = 67,
  kX86InstIdFisub_ExtendedIndex = 64,
  kX86InstIdFisubr_ExtendedIndex = 64,
  kX86InstIdFld_ExtendedIndex = 68,
  kX86InstIdFld1_ExtendedIndex = 52,
  kX86InstIdFldcw_ExtendedIndex = 55,
  kX86InstIdFldenv_ExtendedIndex = 55,
  kX86InstIdFldl2e_ExtendedIndex = 52,
  kX86InstIdFldl2t_ExtendedIndex = 52,
  kX86InstIdFldlg2_ExtendedIndex = 52,
  kX86InstIdFldln2_ExtendedIndex = 52,
  kX86InstIdFldpi_ExtendedIndex = 52,
  kX86InstIdFldz_ExtendedIndex = 52,
  kX86InstIdFmul_ExtendedIndex = 53,
  kX86InstIdFmulp_ExtendedIndex = 54,
  kX86InstIdFnclex_ExtendedIndex = 52,
  kX86InstIdFninit_ExtendedIndex = 52,
  kX86InstIdFnop_ExtendedIndex = 52,
  kX86InstIdFnsave_ExtendedIndex = 55,
  kX86InstIdFnstcw_ExtendedIndex = 55,
  kX86InstIdFnstenv_ExtendedIndex = 55,
  kX86InstIdFnstsw_ExtendedIndex = 69,
  kX86InstIdFpatan_ExtendedIndex = 52,
  kX86InstIdFprem_ExtendedIndex = 52,
  kX86InstIdFprem1_ExtendedIndex = 52,
  kX86InstIdFptan_ExtendedIndex = 52,
  kX86InstIdFrndint_ExtendedIndex = 52,
  kX86InstIdFrstor_ExtendedIndex = 55,
  kX86InstIdFsave_ExtendedIndex = 55,
  kX86InstIdFscale_ExtendedIndex = 52,
  kX86InstIdFsin_ExtendedIndex = 52,
  kX86InstIdFsincos_ExtendedIndex = 52,
  kX86InstIdFsqrt_ExtendedIndex = 52,
  kX86InstIdFst_ExtendedIndex = 70,
  kX86InstIdFstcw_ExtendedIndex = 55,
  kX86InstIdFstenv_ExtendedIndex = 55,
  kX86InstIdFstp_ExtendedIndex = 71,
  kX86InstIdFstsw_ExtendedIndex = 72,
  kX86InstIdFsub_ExtendedIndex = 53,
  kX86InstIdFsubp_ExtendedIndex = 54,
  kX86InstIdFsubr_ExtendedIndex = 53,
  kX86InstIdFsubrp_ExtendedIndex = 54,
  kX86InstIdFtst_ExtendedIndex = 52,
  kX86InstIdFucom_ExtendedIndex = 54,
  kX86InstIdFucomi_ExtendedIndex = 61,
  kX86InstIdFucomip_ExtendedIndex = 61,
  kX86InstIdFucomp_ExtendedIndex = 54,
  kX86InstIdFucompp_ExtendedIndex = 52,
  kX86InstIdFwait_ExtendedIndex = 62,
  kX86InstIdFxam_ExtendedIndex = 52,
  kX86InstIdFxch_ExtendedIndex = 63,
  kX86InstIdFxrstor_ExtendedIndex = 55,
  kX86InstIdFxsave_ExtendedIndex = 55,
  kX86InstIdFxtract_ExtendedIndex = 52,
  kX86InstIdFyl2x_ExtendedIndex = 52,
  kX86InstIdFyl2xp1_ExtendedIndex = 52,
  kX86InstIdHaddpd_ExtendedIndex = 3,
  kX86InstIdHaddps_ExtendedIndex = 3,
  kX86InstIdHsubpd_ExtendedIndex = 3,
  kX86InstIdHsubps_ExtendedIndex = 3,
  kX86InstIdIdiv_ExtendedIndex = 73,
  kX86InstIdImul_ExtendedIndex = 74,
  kX86InstIdInc_ExtendedIndex = 75,
  kX86InstIdInsertps_ExtendedIndex = 4,
  kX86InstIdInsertq_ExtendedIndex = 76,
  kX86InstIdInt_ExtendedIndex = 77,
  kX86InstIdJa_ExtendedIndex = 78,
  kX86InstIdJae_ExtendedIndex = 79,
  kX86InstIdJb_ExtendedIndex = 79,
  kX86InstIdJbe_ExtendedIndex = 78,
  kX86InstIdJc_ExtendedIndex = 79,
  kX86InstIdJe_ExtendedIndex = 80,
  kX86InstIdJg_ExtendedIndex = 81,
  kX86InstIdJge_ExtendedIndex = 82,
  kX86InstIdJl_ExtendedIndex = 82,
  kX86InstIdJle_ExtendedIndex = 81,
  kX86InstIdJna_ExtendedIndex = 78,
  kX86InstIdJnae_ExtendedIndex = 79,
  kX86InstIdJnb_ExtendedIndex = 79,
  kX86InstIdJnbe_ExtendedIndex = 78,
  kX86InstIdJnc_ExtendedIndex = 79,
  kX86InstIdJne_ExtendedIndex = 80,
  kX86InstIdJng_ExtendedIndex = 81,
  kX86InstIdJnge_ExtendedIndex = 82,
  kX86InstIdJnl_ExtendedIndex = 82,
  kX86InstIdJnle_ExtendedIndex = 81,
  kX86InstIdJno_ExtendedIndex = 83,
  kX86InstIdJnp_ExtendedIndex = 84,
  kX86InstIdJns_ExtendedIndex = 85,
  kX86InstIdJnz_ExtendedIndex = 80,
  kX86InstIdJo_ExtendedIndex = 83,
  kX86InstIdJp_ExtendedIndex = 84,
  kX86InstIdJpe_ExtendedIndex = 84,
  kX86InstIdJpo_ExtendedIndex = 84,
  kX86InstIdJs_ExtendedIndex = 85,
  kX86InstIdJz_ExtendedIndex = 80,
  kX86InstIdJecxz_ExtendedIndex = 86,
  kX86InstIdJmp_ExtendedIndex = 87,
  kX86InstIdLahf_ExtendedIndex = 88,
  kX86InstIdLddqu_ExtendedIndex = 89,
  kX86InstIdLdmxcsr_ExtendedIndex = 19,
  kX86InstIdLea_ExtendedIndex = 90,
  kX86InstIdLeave_ExtendedIndex = 16,
  kX86InstIdLfence_ExtendedIndex = 91,
  kX86InstIdLodsB_ExtendedIndex = 92,
  kX86InstIdLodsD_ExtendedIndex = 93,
  kX86InstIdLodsQ_ExtendedIndex = 94,
  kX86InstIdLodsW_ExtendedIndex = 95,
  kX86InstIdLzcnt_ExtendedIndex = 9,
  kX86InstIdMaskmovdqu_ExtendedIndex = 96,
  kX86InstIdMaskmovq_ExtendedIndex = 97,
  kX86InstIdMaxpd_ExtendedIndex = 3,
  kX86InstIdMaxps_ExtendedIndex = 3,
  kX86InstIdMaxsd_ExtendedIndex = 3,
  kX86InstIdMaxss_ExtendedIndex = 3,
  kX86InstIdMfence_ExtendedIndex = 91,
  kX86InstIdMinpd_ExtendedIndex = 3,
  kX86InstIdMinps_ExtendedIndex = 3,
  kX86InstIdMinsd_ExtendedIndex = 3,
  kX86InstIdMinss_ExtendedIndex = 3,
  kX86InstIdMonitor_ExtendedIndex = 16,
  kX86InstIdMov_ExtendedIndex = 98,
  kX86InstIdMovPtr_ExtendedIndex = 99,
  kX86InstIdMovapd_ExtendedIndex = 100,
  kX86InstIdMovaps_ExtendedIndex = 101,
  kX86InstIdMovbe_ExtendedIndex = 102,
  kX86InstIdMovd_ExtendedIndex = 103,
  kX86InstIdMovddup_ExtendedIndex = 104,
  kX86InstIdMovdq2q_ExtendedIndex = 105,
  kX86InstIdMovdqa_ExtendedIndex = 106,
  kX86InstIdMovdqu_ExtendedIndex = 107,
  kX86InstIdMovhlps_ExtendedIndex = 108,
  kX86InstIdMovhpd_ExtendedIndex = 109,
  kX86InstIdMovhps_ExtendedIndex = 110,
  kX86InstIdMovlhps_ExtendedIndex = 111,
  kX86InstIdMovlpd_ExtendedIndex = 112,
  kX86InstIdMovlps_ExtendedIndex = 113,
  kX86InstIdMovmskpd_ExtendedIndex = 114,
  kX86InstIdMovmskps_ExtendedIndex = 114,
  kX86InstIdMovntdq_ExtendedIndex = 115,
  kX86InstIdMovntdqa_ExtendedIndex = 116,
  kX86InstIdMovnti_ExtendedIndex = 117,
  kX86InstIdMovntpd_ExtendedIndex = 118,
  kX86InstIdMovntps_ExtendedIndex = 119,
  kX86InstIdMovntq_ExtendedIndex = 120,
  kX86InstIdMovntsd_ExtendedIndex = 121,
  kX86InstIdMovntss_ExtendedIndex = 122,
  kX86InstIdMovq_ExtendedIndex = 123,
  kX86InstIdMovq2dq_ExtendedIndex = 124,
  kX86InstIdMovsB_ExtendedIndex = 16,
  kX86InstIdMovsD_ExtendedIndex = 125,
  kX86InstIdMovsQ_ExtendedIndex = 16,
  kX86InstIdMovsW_ExtendedIndex = 126,
  kX86InstIdMovsd_ExtendedIndex = 127,
  kX86InstIdMovshdup_ExtendedIndex = 36,
  kX86InstIdMovsldup_ExtendedIndex = 36,
  kX86InstIdMovss_ExtendedIndex = 128,
  kX86InstIdMovsx_ExtendedIndex = 129,
  kX86InstIdMovsxd_ExtendedIndex = 130,
  kX86InstIdMovupd_ExtendedIndex = 131,
  kX86InstIdMovups_ExtendedIndex = 132,
  kX86InstIdMovzx_ExtendedIndex = 129,
  kX86InstIdMpsadbw_ExtendedIndex = 4,
  kX86InstIdMul_ExtendedIndex = 73,
  kX86InstIdMulpd_ExtendedIndex = 3,
  kX86InstIdMulps_ExtendedIndex = 3,
  kX86InstIdMulsd_ExtendedIndex = 3,
  kX86InstIdMulss_ExtendedIndex = 3,
  kX86InstIdMulx_ExtendedIndex = 133,
  kX86InstIdMwait_ExtendedIndex = 16,
  kX86InstIdNeg_ExtendedIndex = 134,
  kX86InstIdNop_ExtendedIndex = 48,
  kX86InstIdNot_ExtendedIndex = 135,
  kX86InstIdOr_ExtendedIndex = 2,
  kX86InstIdOrpd_ExtendedIndex = 3,
  kX86InstIdOrps_ExtendedIndex = 3,
  kX86InstIdPabsb_ExtendedIndex = 136,
  kX86InstIdPabsd_ExtendedIndex = 136,
  kX86InstIdPabsw_ExtendedIndex = 136,
  kX86InstIdPackssdw_ExtendedIndex = 136,
  kX86InstIdPacksswb_ExtendedIndex = 136,
  kX86InstIdPackusdw_ExtendedIndex = 3,
  kX86InstIdPackuswb_ExtendedIndex = 136,
  kX86InstIdPaddb_ExtendedIndex = 136,
  kX86InstIdPaddd_ExtendedIndex = 136,
  kX86InstIdPaddq_ExtendedIndex = 136,
  kX86InstIdPaddsb_ExtendedIndex = 136,
  kX86InstIdPaddsw_ExtendedIndex = 136,
  kX86InstIdPaddusb_ExtendedIndex = 136,
  kX86InstIdPaddusw_ExtendedIndex = 136,
  kX86InstIdPaddw_ExtendedIndex = 136,
  kX86InstIdPalignr_ExtendedIndex = 137,
  kX86InstIdPand_ExtendedIndex = 136,
  kX86InstIdPandn_ExtendedIndex = 136,
  kX86InstIdPause_ExtendedIndex = 48,
  kX86InstIdPavgb_ExtendedIndex = 136,
  kX86InstIdPavgw_ExtendedIndex = 136,
  kX86InstIdPblendvb_ExtendedIndex = 7,
  kX86InstIdPblendw_ExtendedIndex = 4,
  kX86InstIdPclmulqdq_ExtendedIndex = 4,
  kX86InstIdPcmpeqb_ExtendedIndex = 136,
  kX86InstIdPcmpeqd_ExtendedIndex = 136,
  kX86InstIdPcmpeqq_ExtendedIndex = 3,
  kX86InstIdPcmpeqw_ExtendedIndex = 136,
  kX86InstIdPcmpestri_ExtendedIndex = 4,
  kX86InstIdPcmpestrm_ExtendedIndex = 4,
  kX86InstIdPcmpgtb_ExtendedIndex = 136,
  kX86InstIdPcmpgtd_ExtendedIndex = 136,
  kX86InstIdPcmpgtq_ExtendedIndex = 3,
  kX86InstIdPcmpgtw_ExtendedIndex = 136,
  kX86InstIdPcmpistri_ExtendedIndex = 4,
  kX86InstIdPcmpistrm_ExtendedIndex = 4,
  kX86InstIdPdep_ExtendedIndex = 133,
  kX86InstIdPext_ExtendedIndex = 133,
  kX86InstIdPextrb_ExtendedIndex = 138,
  kX86InstIdPextrd_ExtendedIndex = 139,
  kX86InstIdPextrq_ExtendedIndex = 140,
  kX86InstIdPextrw_ExtendedIndex = 141,
  kX86InstIdPf2id_ExtendedIndex = 142,
  kX86InstIdPf2iw_ExtendedIndex = 142,
  kX86InstIdPfacc_ExtendedIndex = 142,
  kX86InstIdPfadd_ExtendedIndex = 142,
  kX86InstIdPfcmpeq_ExtendedIndex = 142,
  kX86InstIdPfcmpge_ExtendedIndex = 142,
  kX86InstIdPfcmpgt_ExtendedIndex = 142,
  kX86InstIdPfmax_ExtendedIndex = 142,
  kX86InstIdPfmin_ExtendedIndex = 142,
  kX86InstIdPfmul_ExtendedIndex = 142,
  kX86InstIdPfnacc_ExtendedIndex = 142,
  kX86InstIdPfpnacc_ExtendedIndex = 142,
  kX86InstIdPfrcp_ExtendedIndex = 142,
  kX86InstIdPfrcpit1_ExtendedIndex = 142,
  kX86InstIdPfrcpit2_ExtendedIndex = 142,
  kX86InstIdPfrsqit1_ExtendedIndex = 142,
  kX86InstIdPfrsqrt_ExtendedIndex = 142,
  kX86InstIdPfsub_ExtendedIndex = 142,
  kX86InstIdPfsubr_ExtendedIndex = 142,
  kX86InstIdPhaddd_ExtendedIndex = 136,
  kX86InstIdPhaddsw_ExtendedIndex = 136,
  kX86InstIdPhaddw_ExtendedIndex = 136,
  kX86InstIdPhminposuw_ExtendedIndex = 3,
  kX86InstIdPhsubd_ExtendedIndex = 136,
  kX86InstIdPhsubsw_ExtendedIndex = 136,
  kX86InstIdPhsubw_ExtendedIndex = 136,
  kX86InstIdPi2fd_ExtendedIndex = 142,
  kX86InstIdPi2fw_ExtendedIndex = 142,
  kX86InstIdPinsrb_ExtendedIndex = 143,
  kX86InstIdPinsrd_ExtendedIndex = 143,
  kX86InstIdPinsrq_ExtendedIndex = 144,
  kX86InstIdPinsrw_ExtendedIndex = 145,
  kX86InstIdPmaddubsw_ExtendedIndex = 136,
  kX86InstIdPmaddwd_ExtendedIndex = 136,
  kX86InstIdPmaxsb_ExtendedIndex = 3,
  kX86InstIdPmaxsd_ExtendedIndex = 3,
  kX86InstIdPmaxsw_ExtendedIndex = 136,
  kX86InstIdPmaxub_ExtendedIndex = 136,
  kX86InstIdPmaxud_ExtendedIndex = 3,
  kX86InstIdPmaxuw_ExtendedIndex = 3,
  kX86InstIdPminsb_ExtendedIndex = 3,
  kX86InstIdPminsd_ExtendedIndex = 3,
  kX86InstIdPminsw_ExtendedIndex = 136,
  kX86InstIdPminub_ExtendedIndex = 136,
  kX86InstIdPminud_ExtendedIndex = 3,
  kX86InstIdPminuw_ExtendedIndex = 3,
  kX86InstIdPmovmskb_ExtendedIndex = 146,
  kX86InstIdPmovsxbd_ExtendedIndex = 36,
  kX86InstIdPmovsxbq_ExtendedIndex = 36,
  kX86InstIdPmovsxbw_ExtendedIndex = 36,
  kX86InstIdPmovsxdq_ExtendedIndex = 36,
  kX86InstIdPmovsxwd_ExtendedIndex = 36,
  kX86InstIdPmovsxwq_ExtendedIndex = 36,
  kX86InstIdPmovzxbd_ExtendedIndex = 36,
  kX86InstIdPmovzxbq_ExtendedIndex = 36,
  kX86InstIdPmovzxbw_ExtendedIndex = 36,
  kX86InstIdPmovzxdq_ExtendedIndex = 36,
  kX86InstIdPmovzxwd_ExtendedIndex = 36,
  kX86InstIdPmovzxwq_ExtendedIndex = 36,
  kX86InstIdPmuldq_ExtendedIndex = 3,
  kX86InstIdPmulhrsw_ExtendedIndex = 136,
  kX86InstIdPmulhuw_ExtendedIndex = 136,
  kX86InstIdPmulhw_ExtendedIndex = 136,
  kX86InstIdPmulld_ExtendedIndex = 3,
  kX86InstIdPmullw_ExtendedIndex = 136,
  kX86InstIdPmuludq_ExtendedIndex = 136,
  kX86InstIdPop_ExtendedIndex = 147,
  kX86InstIdPopa_ExtendedIndex = 16,
  kX86InstIdPopcnt_ExtendedIndex = 9,
  kX86InstIdPopf_ExtendedIndex = 148,
  kX86InstIdPor_ExtendedIndex = 136,
  kX86InstIdPrefetch_ExtendedIndex = 149,
  kX86InstIdPrefetch3dNow_ExtendedIndex = 19,
  kX86InstIdPrefetchw3dNow_ExtendedIndex = 19,
  kX86InstIdPsadbw_ExtendedIndex = 136,
  kX86InstIdPshufb_ExtendedIndex = 136,
  kX86InstIdPshufd_ExtendedIndex = 150,
  kX86InstIdPshufhw_ExtendedIndex = 150,
  kX86InstIdPshuflw_ExtendedIndex = 150,
  kX86InstIdPshufw_ExtendedIndex = 151,
  kX86InstIdPsignb_ExtendedIndex = 136,
  kX86InstIdPsignd_ExtendedIndex = 136,
  kX86InstIdPsignw_ExtendedIndex = 136,
  kX86InstIdPslld_ExtendedIndex = 152,
  kX86InstIdPslldq_ExtendedIndex = 153,
  kX86InstIdPsllq_ExtendedIndex = 154,
  kX86InstIdPsllw_ExtendedIndex = 155,
  kX86InstIdPsrad_ExtendedIndex = 156,
  kX86InstIdPsraw_ExtendedIndex = 157,
  kX86InstIdPsrld_ExtendedIndex = 158,
  kX86InstIdPsrldq_ExtendedIndex = 159,
  kX86InstIdPsrlq_ExtendedIndex = 160,
  kX86InstIdPsrlw_ExtendedIndex = 161,
  kX86InstIdPsubb_ExtendedIndex = 136,
  kX86InstIdPsubd_ExtendedIndex = 136,
  kX86InstIdPsubq_ExtendedIndex = 136,
  kX86InstIdPsubsb_ExtendedIndex = 136,
  kX86InstIdPsubsw_ExtendedIndex = 136,
  kX86InstIdPsubusb_ExtendedIndex = 136,
  kX86InstIdPsubusw_ExtendedIndex = 136,
  kX86InstIdPsubw_ExtendedIndex = 136,
  kX86InstIdPswapd_ExtendedIndex = 142,
  kX86InstIdPtest_ExtendedIndex = 34,
  kX86InstIdPunpckhbw_ExtendedIndex = 136,
  kX86InstIdPunpckhdq_ExtendedIndex = 136,
  kX86InstIdPunpckhqdq_ExtendedIndex = 3,
  kX86InstIdPunpckhwd_ExtendedIndex = 136,
  kX86InstIdPunpcklbw_ExtendedIndex = 136,
  kX86InstIdPunpckldq_ExtendedIndex = 136,
  kX86InstIdPunpcklqdq_ExtendedIndex = 3,
  kX86InstIdPunpcklwd_ExtendedIndex = 136,
  kX86InstIdPush_ExtendedIndex = 162,
  kX86InstIdPusha_ExtendedIndex = 16,
  kX86InstIdPushf_ExtendedIndex = 163,
  kX86InstIdPxor_ExtendedIndex = 136,
  kX86InstIdRcl_ExtendedIndex = 164,
  kX86InstIdRcpps_ExtendedIndex = 36,
  kX86InstIdRcpss_ExtendedIndex = 41,
  kX86InstIdRcr_ExtendedIndex = 164,
  kX86InstIdRdfsbase_ExtendedIndex = 165,
  kX86InstIdRdgsbase_ExtendedIndex = 165,
  kX86InstIdRdrand_ExtendedIndex = 166,
  kX86InstIdRdtsc_ExtendedIndex = 16,
  kX86InstIdRdtscp_ExtendedIndex = 16,
  kX86InstIdRepLodsB_ExtendedIndex = 167,
  kX86InstIdRepLodsD_ExtendedIndex = 167,
  kX86InstIdRepLodsQ_ExtendedIndex = 167,
  kX86InstIdRepLodsW_ExtendedIndex = 167,
  kX86InstIdRepMovsB_ExtendedIndex = 168,
  kX86InstIdRepMovsD_ExtendedIndex = 168,
  kX86InstIdRepMovsQ_ExtendedIndex = 168,
  kX86InstIdRepMovsW_ExtendedIndex = 168,
  kX86InstIdRepStosB_ExtendedIndex = 167,
  kX86InstIdRepStosD_ExtendedIndex = 167,
  kX86InstIdRepStosQ_ExtendedIndex = 167,
  kX86InstIdRepStosW_ExtendedIndex = 167,
  kX86InstIdRepeCmpsB_ExtendedIndex = 169,
  kX86InstIdRepeCmpsD_ExtendedIndex = 169,
  kX86InstIdRepeCmpsQ_ExtendedIndex = 169,
  kX86InstIdRepeCmpsW_ExtendedIndex = 169,
  kX86InstIdRepeScasB_ExtendedIndex = 169,
  kX86InstIdRepeScasD_ExtendedIndex = 169,
  kX86InstIdRepeScasQ_ExtendedIndex = 169,
  kX86InstIdRepeScasW_ExtendedIndex = 169,
  kX86InstIdRepneCmpsB_ExtendedIndex = 169,
  kX86InstIdRepneCmpsD_ExtendedIndex = 169,
  kX86InstIdRepneCmpsQ_ExtendedIndex = 169,
  kX86InstIdRepneCmpsW_ExtendedIndex = 169,
  kX86InstIdRepneScasB_ExtendedIndex = 169,
  kX86InstIdRepneScasD_ExtendedIndex = 169,
  kX86InstIdRepneScasQ_ExtendedIndex = 169,
  kX86InstIdRepneScasW_ExtendedIndex = 169,
  kX86InstIdRet_ExtendedIndex = 170,
  kX86InstIdRol_ExtendedIndex = 171,
  kX86InstIdRor_ExtendedIndex = 171,
  kX86InstIdRorx_ExtendedIndex = 172,
  kX86InstIdRoundpd_ExtendedIndex = 150,
  kX86InstIdRoundps_ExtendedIndex = 150,
  kX86InstIdRoundsd_ExtendedIndex = 173,
  kX86InstIdRoundss_ExtendedIndex = 174,
  kX86InstIdRsqrtps_ExtendedIndex = 36,
  kX86InstIdRsqrtss_ExtendedIndex = 41,
  kX86InstIdSahf_ExtendedIndex = 175,
  kX86InstIdSal_ExtendedIndex = 176,
  kX86InstIdSar_ExtendedIndex = 176,
  kX86InstIdSarx_ExtendedIndex = 177,
  kX86InstIdSbb_ExtendedIndex = 1,
  kX86InstIdScasB_ExtendedIndex = 30,
  kX86InstIdScasD_ExtendedIndex = 30,
  kX86InstIdScasQ_ExtendedIndex = 30,
  kX86InstIdScasW_ExtendedIndex = 31,
  kX86InstIdSeta_ExtendedIndex = 178,
  kX86InstIdSetae_ExtendedIndex = 179,
  kX86InstIdSetb_ExtendedIndex = 179,
  kX86InstIdSetbe_ExtendedIndex = 178,
  kX86InstIdSetc_ExtendedIndex = 179,
  kX86InstIdSete_ExtendedIndex = 180,
  kX86InstIdSetg_ExtendedIndex = 181,
  kX86InstIdSetge_ExtendedIndex = 182,
  kX86InstIdSetl_ExtendedIndex = 182,
  kX86InstIdSetle_ExtendedIndex = 181,
  kX86InstIdSetna_ExtendedIndex = 178,
  kX86InstIdSetnae_ExtendedIndex = 179,
  kX86InstIdSetnb_ExtendedIndex = 179,
  kX86InstIdSetnbe_ExtendedIndex = 178,
  kX86InstIdSetnc_ExtendedIndex = 179,
  kX86InstIdSetne_ExtendedIndex = 180,
  kX86InstIdSetng_ExtendedIndex = 181,
  kX86InstIdSetnge_ExtendedIndex = 182,
  kX86InstIdSetnl_ExtendedIndex = 182,
  kX86InstIdSetnle_ExtendedIndex = 181,
  kX86InstIdSetno_ExtendedIndex = 183,
  kX86InstIdSetnp_ExtendedIndex = 184,
  kX86InstIdSetns_ExtendedIndex = 185,
  kX86InstIdSetnz_ExtendedIndex = 180,
  kX86InstIdSeto_ExtendedIndex = 183,
  kX86InstIdSetp_ExtendedIndex = 184,
  kX86InstIdSetpe_ExtendedIndex = 184,
  kX86InstIdSetpo_ExtendedIndex = 184,
  kX86InstIdSets_ExtendedIndex = 185,
  kX86InstIdSetz_ExtendedIndex = 180,
  kX86InstIdSfence_ExtendedIndex = 91,
  kX86InstIdShl_ExtendedIndex = 176,
  kX86InstIdShld_ExtendedIndex = 186,
  kX86InstIdShlx_ExtendedIndex = 177,
  kX86InstIdShr_ExtendedIndex = 176,
  kX86InstIdShrd_ExtendedIndex = 187,
  kX86InstIdShrx_ExtendedIndex = 177,
  kX86InstIdShufpd_ExtendedIndex = 4,
  kX86InstIdShufps_ExtendedIndex = 4,
  kX86InstIdSqrtpd_ExtendedIndex = 36,
  kX86InstIdSqrtps_ExtendedIndex = 36,
  kX86InstIdSqrtsd_ExtendedIndex = 44,
  kX86InstIdSqrtss_ExtendedIndex = 41,
  kX86InstIdStc_ExtendedIndex = 17,
  kX86InstIdStd_ExtendedIndex = 18,
  kX86InstIdStmxcsr_ExtendedIndex = 19,
  kX86InstIdStosB_ExtendedIndex = 188,
  kX86InstIdStosD_ExtendedIndex = 188,
  kX86InstIdStosQ_ExtendedIndex = 188,
  kX86InstIdStosW_ExtendedIndex = 189,
  kX86InstIdSub_ExtendedIndex = 2,
  kX86InstIdSubpd_ExtendedIndex = 3,
  kX86InstIdSubps_ExtendedIndex = 3,
  kX86InstIdSubsd_ExtendedIndex = 3,
  kX86InstIdSubss_ExtendedIndex = 3,
  kX86InstIdTest_ExtendedIndex = 190,
  kX86InstIdTzcnt_ExtendedIndex = 191,
  kX86InstIdUcomisd_ExtendedIndex = 34,
  kX86InstIdUcomiss_ExtendedIndex = 34,
  kX86InstIdUd2_ExtendedIndex = 48,
  kX86InstIdUnpckhpd_ExtendedIndex = 3,
  kX86InstIdUnpckhps_ExtendedIndex = 3,
  kX86InstIdUnpcklpd_ExtendedIndex = 3,
  kX86InstIdUnpcklps_ExtendedIndex = 3,
  kX86InstIdVaddpd_ExtendedIndex = 192,
  kX86InstIdVaddps_ExtendedIndex = 192,
  kX86InstIdVaddsd_ExtendedIndex = 193,
  kX86InstIdVaddss_ExtendedIndex = 193,
  kX86InstIdVaddsubpd_ExtendedIndex = 192,
  kX86InstIdVaddsubps_ExtendedIndex = 192,
  kX86InstIdVaesdec_ExtendedIndex = 193,
  kX86InstIdVaesdeclast_ExtendedIndex = 193,
  kX86InstIdVaesenc_ExtendedIndex = 193,
  kX86InstIdVaesenclast_ExtendedIndex = 193,
  kX86InstIdVaesimc_ExtendedIndex = 194,
  kX86InstIdVaeskeygenassist_ExtendedIndex = 195,
  kX86InstIdVandnpd_ExtendedIndex = 192,
  kX86InstIdVandnps_ExtendedIndex = 192,
  kX86InstIdVandpd_ExtendedIndex = 192,
  kX86InstIdVandps_ExtendedIndex = 192,
  kX86InstIdVblendpd_ExtendedIndex = 196,
  kX86InstIdVblendps_ExtendedIndex = 196,
  kX86InstIdVblendvpd_ExtendedIndex = 197,
  kX86InstIdVblendvps_ExtendedIndex = 197,
  kX86InstIdVbroadcastf128_ExtendedIndex = 198,
  kX86InstIdVbroadcasti128_ExtendedIndex = 198,
  kX86InstIdVbroadcastsd_ExtendedIndex = 199,
  kX86InstIdVbroadcastss_ExtendedIndex = 200,
  kX86InstIdVcmppd_ExtendedIndex = 196,
  kX86InstIdVcmpps_ExtendedIndex = 196,
  kX86InstIdVcmpsd_ExtendedIndex = 201,
  kX86InstIdVcmpss_ExtendedIndex = 201,
  kX86InstIdVcomisd_ExtendedIndex = 194,
  kX86InstIdVcomiss_ExtendedIndex = 194,
  kX86InstIdVcvtdq2pd_ExtendedIndex = 202,
  kX86InstIdVcvtdq2ps_ExtendedIndex = 203,
  kX86InstIdVcvtpd2dq_ExtendedIndex = 204,
  kX86InstIdVcvtpd2ps_ExtendedIndex = 204,
  kX86InstIdVcvtph2ps_ExtendedIndex = 202,
  kX86InstIdVcvtps2dq_ExtendedIndex = 203,
  kX86InstIdVcvtps2pd_ExtendedIndex = 202,
  kX86InstIdVcvtps2ph_ExtendedIndex = 205,
  kX86InstIdVcvtsd2si_ExtendedIndex = 206,
  kX86InstIdVcvtsd2ss_ExtendedIndex = 193,
  kX86InstIdVcvtsi2sd_ExtendedIndex = 207,
  kX86InstIdVcvtsi2ss_ExtendedIndex = 207,
  kX86InstIdVcvtss2sd_ExtendedIndex = 193,
  kX86InstIdVcvtss2si_ExtendedIndex = 206,
  kX86InstIdVcvttpd2dq_ExtendedIndex = 208,
  kX86InstIdVcvttps2dq_ExtendedIndex = 203,
  kX86InstIdVcvttsd2si_ExtendedIndex = 206,
  kX86InstIdVcvttss2si_ExtendedIndex = 206,
  kX86InstIdVdivpd_ExtendedIndex = 192,
  kX86InstIdVdivps_ExtendedIndex = 192,
  kX86InstIdVdivsd_ExtendedIndex = 193,
  kX86InstIdVdivss_ExtendedIndex = 193,
  kX86InstIdVdppd_ExtendedIndex = 201,
  kX86InstIdVdpps_ExtendedIndex = 196,
  kX86InstIdVextractf128_ExtendedIndex = 209,
  kX86InstIdVextracti128_ExtendedIndex = 209,
  kX86InstIdVextractps_ExtendedIndex = 210,
  kX86InstIdVfmadd132pd_ExtendedIndex = 192,
  kX86InstIdVfmadd132ps_ExtendedIndex = 192,
  kX86InstIdVfmadd132sd_ExtendedIndex = 193,
  kX86InstIdVfmadd132ss_ExtendedIndex = 193,
  kX86InstIdVfmadd213pd_ExtendedIndex = 192,
  kX86InstIdVfmadd213ps_ExtendedIndex = 192,
  kX86InstIdVfmadd213sd_ExtendedIndex = 193,
  kX86InstIdVfmadd213ss_ExtendedIndex = 193,
  kX86InstIdVfmadd231pd_ExtendedIndex = 192,
  kX86InstIdVfmadd231ps_ExtendedIndex = 192,
  kX86InstIdVfmadd231sd_ExtendedIndex = 193,
  kX86InstIdVfmadd231ss_ExtendedIndex = 193,
  kX86InstIdVfmaddpd_ExtendedIndex = 211,
  kX86InstIdVfmaddps_ExtendedIndex = 211,
  kX86InstIdVfmaddsd_ExtendedIndex = 212,
  kX86InstIdVfmaddss_ExtendedIndex = 212,
  kX86InstIdVfmaddsub132pd_ExtendedIndex = 192,
  kX86InstIdVfmaddsub132ps_ExtendedIndex = 192,
  kX86InstIdVfmaddsub213pd_ExtendedIndex = 192,
  kX86InstIdVfmaddsub213ps_ExtendedIndex = 192,
  kX86InstIdVfmaddsub231pd_ExtendedIndex = 192,
  kX86InstIdVfmaddsub231ps_ExtendedIndex = 192,
  kX86InstIdVfmaddsubpd_ExtendedIndex = 211,
  kX86InstIdVfmaddsubps_ExtendedIndex = 211,
  kX86InstIdVfmsub132pd_ExtendedIndex = 192,
  kX86InstIdVfmsub132ps_ExtendedIndex = 192,
  kX86InstIdVfmsub132sd_ExtendedIndex = 193,
  kX86InstIdVfmsub132ss_ExtendedIndex = 193,
  kX86InstIdVfmsub213pd_ExtendedIndex = 192,
  kX86InstIdVfmsub213ps_ExtendedIndex = 192,
  kX86InstIdVfmsub213sd_ExtendedIndex = 193,
  kX86InstIdVfmsub213ss_ExtendedIndex = 193,
  kX86InstIdVfmsub231pd_ExtendedIndex = 192,
  kX86InstIdVfmsub231ps_ExtendedIndex = 192,
  kX86InstIdVfmsub231sd_ExtendedIndex = 193,
  kX86InstIdVfmsub231ss_ExtendedIndex = 193,
  kX86InstIdVfmsubadd132pd_ExtendedIndex = 192,
  kX86InstIdVfmsubadd132ps_ExtendedIndex = 192,
  kX86InstIdVfmsubadd213pd_ExtendedIndex = 192,
  kX86InstIdVfmsubadd213ps_ExtendedIndex = 192,
  kX86InstIdVfmsubadd231pd_ExtendedIndex = 192,
  kX86InstIdVfmsubadd231ps_ExtendedIndex = 192,
  kX86InstIdVfmsubaddpd_ExtendedIndex = 211,
  kX86InstIdVfmsubaddps_ExtendedIndex = 211,
  kX86InstIdVfmsubpd_ExtendedIndex = 211,
  kX86InstIdVfmsubps_ExtendedIndex = 211,
  kX86InstIdVfmsubsd_ExtendedIndex = 212,
  kX86InstIdVfmsubss_ExtendedIndex = 212,
  kX86InstIdVfnmadd132pd_ExtendedIndex = 192,
  kX86InstIdVfnmadd132ps_ExtendedIndex = 192,
  kX86InstIdVfnmadd132sd_ExtendedIndex = 193,
  kX86InstIdVfnmadd132ss_ExtendedIndex = 193,
  kX86InstIdVfnmadd213pd_ExtendedIndex = 192,
  kX86InstIdVfnmadd213ps_ExtendedIndex = 192,
  kX86InstIdVfnmadd213sd_ExtendedIndex = 193,
  kX86InstIdVfnmadd213ss_ExtendedIndex = 193,
  kX86InstIdVfnmadd231pd_ExtendedIndex = 192,
  kX86InstIdVfnmadd231ps_ExtendedIndex = 192,
  kX86InstIdVfnmadd231sd_ExtendedIndex = 193,
  kX86InstIdVfnmadd231ss_ExtendedIndex = 193,
  kX86InstIdVfnmaddpd_ExtendedIndex = 211,
  kX86InstIdVfnmaddps_ExtendedIndex = 211,
  kX86InstIdVfnmaddsd_ExtendedIndex = 212,
  kX86InstIdVfnmaddss_ExtendedIndex = 212,
  kX86InstIdVfnmsub132pd_ExtendedIndex = 192,
  kX86InstIdVfnmsub132ps_ExtendedIndex = 192,
  kX86InstIdVfnmsub132sd_ExtendedIndex = 193,
  kX86InstIdVfnmsub132ss_ExtendedIndex = 193,
  kX86InstIdVfnmsub213pd_ExtendedIndex = 192,
  kX86InstIdVfnmsub213ps_ExtendedIndex = 192,
  kX86InstIdVfnmsub213sd_ExtendedIndex = 193,
  kX86InstIdVfnmsub213ss_ExtendedIndex = 193,
  kX86InstIdVfnmsub231pd_ExtendedIndex = 192,
  kX86InstIdVfnmsub231ps_ExtendedIndex = 192,
  kX86InstIdVfnmsub231sd_ExtendedIndex = 193,
  kX86InstIdVfnmsub231ss_ExtendedIndex = 193,
  kX86InstIdVfnmsubpd_ExtendedIndex = 211,
  kX86InstIdVfnmsubps_ExtendedIndex = 211,
  kX86InstIdVfnmsubsd_ExtendedIndex = 212,
  kX86InstIdVfnmsubss_ExtendedIndex = 212,
  kX86InstIdVfrczpd_ExtendedIndex = 213,
  kX86InstIdVfrczps_ExtendedIndex = 213,
  kX86InstIdVfrczsd_ExtendedIndex = 214,
  kX86InstIdVfrczss_ExtendedIndex = 214,
  kX86InstIdVgatherdpd_ExtendedIndex = 215,
  kX86InstIdVgatherdps_ExtendedIndex = 215,
  kX86InstIdVgatherqpd_ExtendedIndex = 215,
  kX86InstIdVgatherqps_ExtendedIndex = 216,
  kX86InstIdVhaddpd_ExtendedIndex = 192,
  kX86InstIdVhaddps_ExtendedIndex = 192,
  kX86InstIdVhsubpd_ExtendedIndex = 192,
  kX86InstIdVhsubps_ExtendedIndex = 192,
  kX86InstIdVinsertf128_ExtendedIndex = 217,
  kX86InstIdVinserti128_ExtendedIndex = 217,
  kX86InstIdVinsertps_ExtendedIndex = 201,
  kX86InstIdVlddqu_ExtendedIndex = 218,
  kX86InstIdVldmxcsr_ExtendedIndex = 219,
  kX86InstIdVmaskmovdqu_ExtendedIndex = 220,
  kX86InstIdVmaskmovpd_ExtendedIndex = 221,
  kX86InstIdVmaskmovps_ExtendedIndex = 222,
  kX86InstIdVmaxpd_ExtendedIndex = 192,
  kX86InstIdVmaxps_ExtendedIndex = 192,
  kX86InstIdVmaxsd_ExtendedIndex = 192,
  kX86InstIdVmaxss_ExtendedIndex = 192,
  kX86InstIdVminpd_ExtendedIndex = 192,
  kX86InstIdVminps_ExtendedIndex = 192,
  kX86InstIdVminsd_ExtendedIndex = 192,
  kX86InstIdVminss_ExtendedIndex = 192,
  kX86InstIdVmovapd_ExtendedIndex = 223,
  kX86InstIdVmovaps_ExtendedIndex = 224,
  kX86InstIdVmovd_ExtendedIndex = 225,
  kX86InstIdVmovddup_ExtendedIndex = 203,
  kX86InstIdVmovdqa_ExtendedIndex = 226,
  kX86InstIdVmovdqu_ExtendedIndex = 227,
  kX86InstIdVmovhlps_ExtendedIndex = 228,
  kX86InstIdVmovhpd_ExtendedIndex = 229,
  kX86InstIdVmovhps_ExtendedIndex = 230,
  kX86InstIdVmovlhps_ExtendedIndex = 228,
  kX86InstIdVmovlpd_ExtendedIndex = 231,
  kX86InstIdVmovlps_ExtendedIndex = 232,
  kX86InstIdVmovmskpd_ExtendedIndex = 233,
  kX86InstIdVmovmskps_ExtendedIndex = 233,
  kX86InstIdVmovntdq_ExtendedIndex = 234,
  kX86InstIdVmovntdqa_ExtendedIndex = 218,
  kX86InstIdVmovntpd_ExtendedIndex = 235,
  kX86InstIdVmovntps_ExtendedIndex = 235,
  kX86InstIdVmovq_ExtendedIndex = 225,
  kX86InstIdVmovsd_ExtendedIndex = 236,
  kX86InstIdVmovshdup_ExtendedIndex = 203,
  kX86InstIdVmovsldup_ExtendedIndex = 203,
  kX86InstIdVmovss_ExtendedIndex = 237,
  kX86InstIdVmovupd_ExtendedIndex = 238,
  kX86InstIdVmovups_ExtendedIndex = 239,
  kX86InstIdVmpsadbw_ExtendedIndex = 196,
  kX86InstIdVmulpd_ExtendedIndex = 192,
  kX86InstIdVmulps_ExtendedIndex = 192,
  kX86InstIdVmulsd_ExtendedIndex = 192,
  kX86InstIdVmulss_ExtendedIndex = 192,
  kX86InstIdVorpd_ExtendedIndex = 192,
  kX86InstIdVorps_ExtendedIndex = 192,
  kX86InstIdVpabsb_ExtendedIndex = 203,
  kX86InstIdVpabsd_ExtendedIndex = 203,
  kX86InstIdVpabsw_ExtendedIndex = 203,
  kX86InstIdVpackssdw_ExtendedIndex = 192,
  kX86InstIdVpacksswb_ExtendedIndex = 192,
  kX86InstIdVpackusdw_ExtendedIndex = 192,
  kX86InstIdVpackuswb_ExtendedIndex = 192,
  kX86InstIdVpaddb_ExtendedIndex = 192,
  kX86InstIdVpaddd_ExtendedIndex = 192,
  kX86InstIdVpaddq_ExtendedIndex = 192,
  kX86InstIdVpaddsb_ExtendedIndex = 192,
  kX86InstIdVpaddsw_ExtendedIndex = 192,
  kX86InstIdVpaddusb_ExtendedIndex = 192,
  kX86InstIdVpaddusw_ExtendedIndex = 192,
  kX86InstIdVpaddw_ExtendedIndex = 192,
  kX86InstIdVpalignr_ExtendedIndex = 196,
  kX86InstIdVpand_ExtendedIndex = 192,
  kX86InstIdVpandn_ExtendedIndex = 192,
  kX86InstIdVpavgb_ExtendedIndex = 192,
  kX86InstIdVpavgw_ExtendedIndex = 192,
  kX86InstIdVpblendd_ExtendedIndex = 196,
  kX86InstIdVpblendvb_ExtendedIndex = 240,
  kX86InstIdVpblendw_ExtendedIndex = 196,
  kX86InstIdVpbroadcastb_ExtendedIndex = 202,
  kX86InstIdVpbroadcastd_ExtendedIndex = 202,
  kX86InstIdVpbroadcastq_ExtendedIndex = 202,
  kX86InstIdVpbroadcastw_ExtendedIndex = 202,
  kX86InstIdVpclmulqdq_ExtendedIndex = 201,
  kX86InstIdVpcmov_ExtendedIndex = 241,
  kX86InstIdVpcmpeqb_ExtendedIndex = 192,
  kX86InstIdVpcmpeqd_ExtendedIndex = 192,
  kX86InstIdVpcmpeqq_ExtendedIndex = 192,
  kX86InstIdVpcmpeqw_ExtendedIndex = 192,
  kX86InstIdVpcmpestri_ExtendedIndex = 195,
  kX86InstIdVpcmpestrm_ExtendedIndex = 195,
  kX86InstIdVpcmpgtb_ExtendedIndex = 192,
  kX86InstIdVpcmpgtd_ExtendedIndex = 192,
  kX86InstIdVpcmpgtq_ExtendedIndex = 192,
  kX86InstIdVpcmpgtw_ExtendedIndex = 192,
  kX86InstIdVpcmpistri_ExtendedIndex = 195,
  kX86InstIdVpcmpistrm_ExtendedIndex = 195,
  kX86InstIdVpcomb_ExtendedIndex = 242,
  kX86InstIdVpcomd_ExtendedIndex = 242,
  kX86InstIdVpcomq_ExtendedIndex = 242,
  kX86InstIdVpcomub_ExtendedIndex = 242,
  kX86InstIdVpcomud_ExtendedIndex = 242,
  kX86InstIdVpcomuq_ExtendedIndex = 242,
  kX86InstIdVpcomuw_ExtendedIndex = 242,
  kX86InstIdVpcomw_ExtendedIndex = 242,
  kX86InstIdVperm2f128_ExtendedIndex = 243,
  kX86InstIdVperm2i128_ExtendedIndex = 243,
  kX86InstIdVpermd_ExtendedIndex = 244,
  kX86InstIdVpermil2pd_ExtendedIndex = 245,
  kX86InstIdVpermil2ps_ExtendedIndex = 245,
  kX86InstIdVpermilpd_ExtendedIndex = 246,
  kX86InstIdVpermilps_ExtendedIndex = 247,
  kX86InstIdVpermpd_ExtendedIndex = 248,
  kX86InstIdVpermps_ExtendedIndex = 244,
  kX86InstIdVpermq_ExtendedIndex = 248,
  kX86InstIdVpextrb_ExtendedIndex = 249,
  kX86InstIdVpextrd_ExtendedIndex = 210,
  kX86InstIdVpextrq_ExtendedIndex = 250,
  kX86InstIdVpextrw_ExtendedIndex = 251,
  kX86InstIdVpgatherdd_ExtendedIndex = 215,
  kX86InstIdVpgatherdq_ExtendedIndex = 215,
  kX86InstIdVpgatherqd_ExtendedIndex = 216,
  kX86InstIdVpgatherqq_ExtendedIndex = 215,
  kX86InstIdVphaddbd_ExtendedIndex = 214,
  kX86InstIdVphaddbq_ExtendedIndex = 214,
  kX86InstIdVphaddbw_ExtendedIndex = 214,
  kX86InstIdVphaddd_ExtendedIndex = 192,
  kX86InstIdVphadddq_ExtendedIndex = 214,
  kX86InstIdVphaddsw_ExtendedIndex = 192,
  kX86InstIdVphaddubd_ExtendedIndex = 214,
  kX86InstIdVphaddubq_ExtendedIndex = 214,
  kX86InstIdVphaddubw_ExtendedIndex = 214,
  kX86InstIdVphaddudq_ExtendedIndex = 214,
  kX86InstIdVphadduwd_ExtendedIndex = 214,
  kX86InstIdVphadduwq_ExtendedIndex = 214,
  kX86InstIdVphaddw_ExtendedIndex = 192,
  kX86InstIdVphaddwd_ExtendedIndex = 214,
  kX86InstIdVphaddwq_ExtendedIndex = 214,
  kX86InstIdVphminposuw_ExtendedIndex = 194,
  kX86InstIdVphsubbw_ExtendedIndex = 214,
  kX86InstIdVphsubd_ExtendedIndex = 192,
  kX86InstIdVphsubdq_ExtendedIndex = 214,
  kX86InstIdVphsubsw_ExtendedIndex = 192,
  kX86InstIdVphsubw_ExtendedIndex = 192,
  kX86InstIdVphsubwd_ExtendedIndex = 214,
  kX86InstIdVpinsrb_ExtendedIndex = 252,
  kX86InstIdVpinsrd_ExtendedIndex = 253,
  kX86InstIdVpinsrq_ExtendedIndex = 254,
  kX86InstIdVpinsrw_ExtendedIndex = 255,
  kX86InstIdVpmacsdd_ExtendedIndex = 256,
  kX86InstIdVpmacsdqh_ExtendedIndex = 256,
  kX86InstIdVpmacsdql_ExtendedIndex = 256,
  kX86InstIdVpmacssdd_ExtendedIndex = 256,
  kX86InstIdVpmacssdqh_ExtendedIndex = 256,
  kX86InstIdVpmacssdql_ExtendedIndex = 256,
  kX86InstIdVpmacsswd_ExtendedIndex = 256,
  kX86InstIdVpmacssww_ExtendedIndex = 256,
  kX86InstIdVpmacswd_ExtendedIndex = 256,
  kX86InstIdVpmacsww_ExtendedIndex = 256,
  kX86InstIdVpmadcsswd_ExtendedIndex = 256,
  kX86InstIdVpmadcswd_ExtendedIndex = 256,
  kX86InstIdVpmaddubsw_ExtendedIndex = 192,
  kX86InstIdVpmaddwd_ExtendedIndex = 192,
  kX86InstIdVpmaskmovd_ExtendedIndex = 257,
  kX86InstIdVpmaskmovq_ExtendedIndex = 257,
  kX86InstIdVpmaxsb_ExtendedIndex = 192,
  kX86InstIdVpmaxsd_ExtendedIndex = 192,
  kX86InstIdVpmaxsw_ExtendedIndex = 192,
  kX86InstIdVpmaxub_ExtendedIndex = 192,
  kX86InstIdVpmaxud_ExtendedIndex = 192,
  kX86InstIdVpmaxuw_ExtendedIndex = 192,
  kX86InstIdVpminsb_ExtendedIndex = 192,
  kX86InstIdVpminsd_ExtendedIndex = 192,
  kX86InstIdVpminsw_ExtendedIndex = 192,
  kX86InstIdVpminub_ExtendedIndex = 192,
  kX86InstIdVpminud_ExtendedIndex = 192,
  kX86InstIdVpminuw_ExtendedIndex = 192,
  kX86InstIdVpmovmskb_ExtendedIndex = 233,
  kX86InstIdVpmovsxbd_ExtendedIndex = 203,
  kX86InstIdVpmovsxbq_ExtendedIndex = 203,
  kX86InstIdVpmovsxbw_ExtendedIndex = 203,
  kX86InstIdVpmovsxdq_ExtendedIndex = 203,
  kX86InstIdVpmovsxwd_ExtendedIndex = 203,
  kX86InstIdVpmovsxwq_ExtendedIndex = 203,
  kX86InstIdVpmovzxbd_ExtendedIndex = 203,
  kX86InstIdVpmovzxbq_ExtendedIndex = 203,
  kX86InstIdVpmovzxbw_ExtendedIndex = 203,
  kX86InstIdVpmovzxdq_ExtendedIndex = 203,
  kX86InstIdVpmovzxwd_ExtendedIndex = 203,
  kX86InstIdVpmovzxwq_ExtendedIndex = 203,
  kX86InstIdVpmuldq_ExtendedIndex = 192,
  kX86InstIdVpmulhrsw_ExtendedIndex = 192,
  kX86InstIdVpmulhuw_ExtendedIndex = 192,
  kX86InstIdVpmulhw_ExtendedIndex = 192,
  kX86InstIdVpmulld_ExtendedIndex = 192,
  kX86InstIdVpmullw_ExtendedIndex = 192,
  kX86InstIdVpmuludq_ExtendedIndex = 192,
  kX86InstIdVpor_ExtendedIndex = 192,
  kX86InstIdVpperm_ExtendedIndex = 258,
  kX86InstIdVprotb_ExtendedIndex = 259,
  kX86InstIdVprotd_ExtendedIndex = 260,
  kX86InstIdVprotq_ExtendedIndex = 261,
  kX86InstIdVprotw_ExtendedIndex = 262,
  kX86InstIdVpsadbw_ExtendedIndex = 192,
  kX86InstIdVpshab_ExtendedIndex = 263,
  kX86InstIdVpshad_ExtendedIndex = 263,
  kX86InstIdVpshaq_ExtendedIndex = 263,
  kX86InstIdVpshaw_ExtendedIndex = 263,
  kX86InstIdVpshlb_ExtendedIndex = 263,
  kX86InstIdVpshld_ExtendedIndex = 263,
  kX86InstIdVpshlq_ExtendedIndex = 263,
  kX86InstIdVpshlw_ExtendedIndex = 263,
  kX86InstIdVpshufb_ExtendedIndex = 192,
  kX86InstIdVpshufd_ExtendedIndex = 264,
  kX86InstIdVpshufhw_ExtendedIndex = 264,
  kX86InstIdVpshuflw_ExtendedIndex = 264,
  kX86InstIdVpsignb_ExtendedIndex = 192,
  kX86InstIdVpsignd_ExtendedIndex = 192,
  kX86InstIdVpsignw_ExtendedIndex = 192,
  kX86InstIdVpslld_ExtendedIndex = 265,
  kX86InstIdVpslldq_ExtendedIndex = 266,
  kX86InstIdVpsllq_ExtendedIndex = 267,
  kX86InstIdVpsllvd_ExtendedIndex = 192,
  kX86InstIdVpsllvq_ExtendedIndex = 192,
  kX86InstIdVpsllw_ExtendedIndex = 268,
  kX86InstIdVpsrad_ExtendedIndex = 269,
  kX86InstIdVpsravd_ExtendedIndex = 192,
  kX86InstIdVpsraw_ExtendedIndex = 270,
  kX86InstIdVpsrld_ExtendedIndex = 271,
  kX86InstIdVpsrldq_ExtendedIndex = 266,
  kX86InstIdVpsrlq_ExtendedIndex = 272,
  kX86InstIdVpsrlvd_ExtendedIndex = 192,
  kX86InstIdVpsrlvq_ExtendedIndex = 192,
  kX86InstIdVpsrlw_ExtendedIndex = 273,
  kX86InstIdVpsubb_ExtendedIndex = 192,
  kX86InstIdVpsubd_ExtendedIndex = 192,
  kX86InstIdVpsubq_ExtendedIndex = 192,
  kX86InstIdVpsubsb_ExtendedIndex = 192,
  kX86InstIdVpsubsw_ExtendedIndex = 192,
  kX86InstIdVpsubusb_ExtendedIndex = 192,
  kX86InstIdVpsubusw_ExtendedIndex = 192,
  kX86InstIdVpsubw_ExtendedIndex = 192,
  kX86InstIdVptest_ExtendedIndex = 274,
  kX86InstIdVpunpckhbw_ExtendedIndex = 192,
  kX86InstIdVpunpckhdq_ExtendedIndex = 192,
  kX86InstIdVpunpckhqdq_ExtendedIndex = 192,
  kX86InstIdVpunpckhwd_ExtendedIndex = 192,
  kX86InstIdVpunpcklbw_ExtendedIndex = 192,
  kX86InstIdVpunpckldq_ExtendedIndex = 192,
  kX86InstIdVpunpcklqdq_ExtendedIndex = 192,
  kX86InstIdVpunpcklwd_ExtendedIndex = 192,
  kX86InstIdVpxor_ExtendedIndex = 192,
  kX86InstIdVrcpps_ExtendedIndex = 203,
  kX86InstIdVrcpss_ExtendedIndex = 193,
  kX86InstIdVroundpd_ExtendedIndex = 264,
  kX86InstIdVroundps_ExtendedIndex = 264,
  kX86InstIdVroundsd_ExtendedIndex = 201,
  kX86InstIdVroundss_ExtendedIndex = 201,
  kX86InstIdVrsqrtps_ExtendedIndex = 203,
  kX86InstIdVrsqrtss_ExtendedIndex = 193,
  kX86InstIdVshufpd_ExtendedIndex = 196,
  kX86InstIdVshufps_ExtendedIndex = 196,
  kX86InstIdVsqrtpd_ExtendedIndex = 203,
  kX86InstIdVsqrtps_ExtendedIndex = 203,
  kX86InstIdVsqrtsd_ExtendedIndex = 193,
  kX86InstIdVsqrtss_ExtendedIndex = 193,
  kX86InstIdVstmxcsr_ExtendedIndex = 219,
  kX86InstIdVsubpd_ExtendedIndex = 192,
  kX86InstIdVsubps_ExtendedIndex = 192,
  kX86InstIdVsubsd_ExtendedIndex = 193,
  kX86InstIdVsubss_ExtendedIndex = 193,
  kX86InstIdVtestpd_ExtendedIndex = 275,
  kX86InstIdVtestps_ExtendedIndex = 275,
  kX86InstIdVucomisd_ExtendedIndex = 276,
  kX86InstIdVucomiss_ExtendedIndex = 276,
  kX86InstIdVunpckhpd_ExtendedIndex = 192,
  kX86InstIdVunpckhps_ExtendedIndex = 192,
  kX86InstIdVunpcklpd_ExtendedIndex = 192,
  kX86InstIdVunpcklps_ExtendedIndex = 192,
  kX86InstIdVxorpd_ExtendedIndex = 192,
  kX86InstIdVxorps_ExtendedIndex = 192,
  kX86InstIdVzeroall_ExtendedIndex = 277,
  kX86InstIdVzeroupper_ExtendedIndex = 277,
  kX86InstIdWrfsbase_ExtendedIndex = 278,
  kX86InstIdWrgsbase_ExtendedIndex = 278,
  kX86InstIdXadd_ExtendedIndex = 279,
  kX86InstIdXchg_ExtendedIndex = 280,
  kX86InstIdXgetbv_ExtendedIndex = 16,
  kX86InstIdXor_ExtendedIndex = 2,
  kX86InstIdXorpd_ExtendedIndex = 3,
  kX86InstIdXorps_ExtendedIndex = 3,
  kX86InstIdXrstor_ExtendedIndex = 281,
  kX86InstIdXrstor64_ExtendedIndex = 281,
  kX86InstIdXsave_ExtendedIndex = 281,
  kX86InstIdXsave64_ExtendedIndex = 281,
  kX86InstIdXsaveopt_ExtendedIndex = 281,
  kX86InstIdXsaveopt64_ExtendedIndex = 281,
  kX86InstIdXsetbv_ExtendedIndex = 16
};
// ${X86InstData:End}

// Please run tools/src-gendefs.js (by using just node.js, without any dependencies) to regenerate the code above.
const X86InstInfo _x86InstInfo[] = {
  // <----------------------------+--------------------+-------------------------------------------+-------------------+------------------------------------+-------------+-------+---------------------------------------------------------------------------------------------------+
  //                              |                    |           Instruction Opcodes             |                   |         Instruction Flags          |   E-FLAGS   | Write |              Operands (Gp/Fp/Mm/K/Xmm/Ymm/Zmm Regs, Mem, Imm, Label, None/Undefined)              |
  //        Instruction Id        |  Instruction Name  +---------------------+---------------------+  Instruction Enc. +---------------+--------------------+-------------+---+---+-------------------+-------------------+-------------------+-------------------+-------------------+
  //                              |                    | 0:PP-MMM OP/O L/W/EW| 1:PP-MMM OP/O L/W/EW|                   | Global Flags  |A512(ID|VL|kz|rnd|b)| EF:OSZAPCDX |Idx| Sz|  [0] 1st Operand  |  [1] 2nd Operand  |  [2] 3rd Operand  |  [3] 4th Operand  |  [4] 5th Operand  |
  // <----------------------------+--------------------+---------------------+---------------------+-------------------+---------------+--------------------+-------------+---+---+-------------------+-------------------+-------------------+-------------------+-------------------+
  INST(kInstIdNone                , ""                 , U                   , U                   , Enc(None)         , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdAdc              , "adc"              , O_000000(10,2,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWWWX__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdAdd              , "add"              , O_000000(00,0,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWWWW__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdAddpd            , "addpd"            , O_660F00(58,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAddps            , "addps"            , O_000F00(58,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAddsd            , "addsd"            , O_F20F00(58,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAddss            , "addss"            , O_F30F00(58,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAddsubpd         , "addsubpd"         , O_660F00(D0,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAddsubps         , "addsubps"         , O_F20F00(D0,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAesdec           , "aesdec"           , O_660F38(DE,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAesdeclast       , "aesdeclast"       , O_660F38(DF,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAesenc           , "aesenc"           , O_660F38(DC,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAesenclast       , "aesenclast"       , O_660F38(DD,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAesimc           , "aesimc"           , O_660F38(DB,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAeskeygenassist  , "aeskeygenassist"  , O_660F3A(DF,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdAnd              , "and"              , O_000000(20,4,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdAndn             , "andn"             , O_000F38(F2,U,_,_,_), U                   , Enc(AvxRvm)       , F(None)                            , EF(WWWUUW__), 0 , 0 , O(Gqd)            , O(Gqd)            , O(GqdMem)         , U                 , U                 ),
  INST(kX86InstIdAndnpd           , "andnpd"           , O_660F00(55,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAndnps           , "andnps"           , O_000F00(55,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAndpd            , "andpd"            , O_660F00(54,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdAndps            , "andps"            , O_000F00(54,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdBextr            , "bextr"            , O_000F38(F7,U,_,_,_), U                   , Enc(AvxRmv)       , F(None)                            , EF(WUWUUW__), 0 , 0 , O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                 ),
  INST(kX86InstIdBlendpd          , "blendpd"          , O_660F3A(0D,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdBlendps          , "blendps"          , O_660F3A(0C,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdBlendvpd         , "blendvpd"         , O_660F38(15,U,_,_,_), U                   , Enc(ExtRm)        , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdBlendvps         , "blendvps"         , O_660F38(14,U,_,_,_), U                   , Enc(ExtRm)        , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdBlsi             , "blsi"             , O_000F38(F3,3,_,_,_), U                   , Enc(AvxVm)        , F(None)                            , EF(WWWUUW__), 0 , 0 , O(Gqd)            , O(GqdMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdBlsmsk           , "blsmsk"           , O_000F38(F3,2,_,_,_), U                   , Enc(AvxVm)        , F(None)                            , EF(WWWUUW__), 0 , 0 , O(Gqd)            , O(GqdMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdBlsr             , "blsr"             , O_000F38(F3,1,_,_,_), U                   , Enc(AvxVm)        , F(None)                            , EF(WWWUUW__), 0 , 0 , O(Gqd)            , O(GqdMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdBsf              , "bsf"              , O_000F00(BC,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(UUWUUU__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdBsr              , "bsr"              , O_000F00(BD,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(UUWUUU__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdBswap            , "bswap"            , O_000F00(C8,U,_,_,_), U                   , Enc(X86BSwap)     , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdBt               , "bt"               , O_000F00(A3,U,_,_,_), O_000F00(BA,4,_,_,_), Enc(X86BTest)     , F(Test)                            , EF(UU_UUW__), 0 , 0 , O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                 ),
  INST(kX86InstIdBtc              , "btc"              , O_000F00(BB,U,_,_,_), O_000F00(BA,7,_,_,_), Enc(X86BTest)     , F(Lock)                            , EF(UU_UUW__), 0 , 0 , O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                 ),
  INST(kX86InstIdBtr              , "btr"              , O_000F00(B3,U,_,_,_), O_000F00(BA,6,_,_,_), Enc(X86BTest)     , F(Lock)                            , EF(UU_UUW__), 0 , 0 , O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                 ),
  INST(kX86InstIdBts              , "bts"              , O_000F00(AB,U,_,_,_), O_000F00(BA,5,_,_,_), Enc(X86BTest)     , F(Lock)                            , EF(UU_UUW__), 0 , 0 , O(GqdwMem)        , O(Gqdw)|O(Imm)    , U                 , U                 , U                 ),
  INST(kX86InstIdBzhi             , "bzhi"             , O_000F38(F5,U,_,_,_), U                   , Enc(AvxRmv)       , F(None)                            , EF(WWWUUW__), 0 , 0 , O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                 ),
  INST(kX86InstIdCall             , "call"             , O_000000(FF,2,_,_,_), O_000000(E8,U,_,_,_), Enc(X86Call)      , F(Flow)                            , EF(________), 0 , 0 , O(GqdMem)|O(LbImm), U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCbw              , "cbw"              , O_660000(98,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCdq              , "cdq"              , O_000000(99,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCdqe             , "cdqe"             , O_000000(98,U,_,W,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdClc              , "clc"              , O_000000(F8,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(_____W__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCld              , "cld"              , O_000000(FC,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(______W_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdClflush          , "clflush"          , O_000F00(AE,7,_,_,_), U                   , Enc(X86M)         , F(None)                            , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmc              , "cmc"              , O_000000(F5,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(_____X__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmova            , "cmova"            , O_000F00(47,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R__R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovae           , "cmovae"           , O_000F00(43,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_____R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovb            , "cmovb"            , O_000F00(42,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_____R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovbe           , "cmovbe"           , O_000F00(46,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R__R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovc            , "cmovc"            , O_000F00(42,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_____R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmove            , "cmove"            , O_000F00(44,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovg            , "cmovg"            , O_000F00(4F,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RRR_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovge           , "cmovge"           , O_000F00(4D,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RR______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovl            , "cmovl"            , O_000F00(4C,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RR______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovle           , "cmovle"           , O_000F00(4E,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RRR_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovna           , "cmovna"           , O_000F00(46,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R__R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnae          , "cmovnae"          , O_000F00(42,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_____R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnb           , "cmovnb"           , O_000F00(43,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_____R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnbe          , "cmovnbe"          , O_000F00(47,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R__R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnc           , "cmovnc"           , O_000F00(43,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_____R__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovne           , "cmovne"           , O_000F00(45,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovng           , "cmovng"           , O_000F00(4E,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RRR_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnge          , "cmovnge"          , O_000F00(4C,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RR______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnl           , "cmovnl"           , O_000F00(4D,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RR______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnle          , "cmovnle"          , O_000F00(4F,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(RRR_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovno           , "cmovno"           , O_000F00(41,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(R_______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnp           , "cmovnp"           , O_000F00(4B,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(____R___), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovns           , "cmovns"           , O_000F00(49,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_R______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovnz           , "cmovnz"           , O_000F00(45,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovo            , "cmovo"            , O_000F00(40,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(R_______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovp            , "cmovp"            , O_000F00(4A,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(____R___), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovpe           , "cmovpe"           , O_000F00(4A,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(____R___), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovpo           , "cmovpo"           , O_000F00(4B,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(____R___), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovs            , "cmovs"            , O_000F00(48,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(_R______), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmovz            , "cmovz"            , O_000F00(44,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(__R_____), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdCmp              , "cmp"              , O_000000(38,7,_,_,_), U                   , Enc(X86Arith)     , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdCmppd            , "cmppd"            , O_660F00(C2,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdCmpps            , "cmpps"            , O_000F00(C2,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdCmpsB            , "cmps_b"           , O_000000(A6,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmpsD            , "cmps_d"           , O_000000(A7,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmpsQ            , "cmps_q"           , O_000000(A7,U,_,W,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmpsW            , "cmps_w"           , O_000000(A7,U,_,_,_), U                   , Enc(X86Op_66H)    , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmpsd            , "cmpsd"            , O_F20F00(C2,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdCmpss            , "cmpss"            , O_F30F00(C2,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdCmpxchg          , "cmpxchg"          , O_000F00(B0,U,_,_,_), U                   , Enc(X86RmReg)     , F(Lock)|F(Special)                 , EF(WWWWWW__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmpxchg16b       , "cmpxchg16b"       , O_000F00(C7,1,_,W,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(__W_____), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCmpxchg8b        , "cmpxchg8b"        , O_000F00(C7,1,_,_,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(__W_____), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdComisd           , "comisd"           , O_660F00(2F,U,_,_,_), U                   , Enc(ExtRm)        , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdComiss           , "comiss"           , O_000F00(2F,U,_,_,_), U                   , Enc(ExtRm)        , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCpuid            , "cpuid"            , O_000F00(A2,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCqo              , "cqo"              , O_000000(99,U,_,W,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCrc32            , "crc32"            , O_F20F38(F0,U,_,_,_), U                   , Enc(ExtCrc)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(GqdwbMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdCvtdq2pd         , "cvtdq2pd"         , O_F30F00(E6,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtdq2ps         , "cvtdq2ps"         , O_000F00(5B,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtpd2dq         , "cvtpd2dq"         , O_F20F00(E6,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtpd2pi         , "cvtpd2pi"         , O_660F00(2D,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Mm)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtpd2ps         , "cvtpd2ps"         , O_660F00(5A,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtpi2pd         , "cvtpi2pd"         , O_660F00(2A,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdCvtpi2ps         , "cvtpi2ps"         , O_000F00(2A,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Xmm)            , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdCvtps2dq         , "cvtps2dq"         , O_660F00(5B,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtps2pd         , "cvtps2pd"         , O_000F00(5A,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtps2pi         , "cvtps2pi"         , O_000F00(2D,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Mm)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtsd2si         , "cvtsd2si"         , O_F20F00(2D,U,_,_,_), U                   , Enc(ExtRm_Q)      , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtsd2ss         , "cvtsd2ss"         , O_F20F00(5A,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 4 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtsi2sd         , "cvtsi2sd"         , O_F20F00(2A,U,_,_,_), U                   , Enc(ExtRm_Q)      , F(Move)                            , EF(________), 0 , 8 , O(Xmm)            , O(GqdMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtsi2ss         , "cvtsi2ss"         , O_F30F00(2A,U,_,_,_), U                   , Enc(ExtRm_Q)      , F(Move)                            , EF(________), 0 , 4 , O(Xmm)            , O(GqdMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtss2sd         , "cvtss2sd"         , O_F30F00(5A,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvtss2si         , "cvtss2si"         , O_F30F00(2D,U,_,_,_), U                   , Enc(ExtRm_Q)      , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvttpd2dq        , "cvttpd2dq"        , O_660F00(E6,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvttpd2pi        , "cvttpd2pi"        , O_660F00(2C,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Mm)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvttps2dq        , "cvttps2dq"        , O_F30F00(5B,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvttps2pi        , "cvttps2pi"        , O_000F00(2C,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Mm)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvttsd2si        , "cvttsd2si"        , O_F20F00(2C,U,_,_,_), U                   , Enc(ExtRm_Q)      , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCvttss2si        , "cvttss2si"        , O_F30F00(2C,U,_,_,_), U                   , Enc(ExtRm_Q)      , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdCwd              , "cwd"              , O_660000(99,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdCwde             , "cwde"             , O_000000(98,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdDaa              , "daa"              , O_000000(27,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(UWWXWX__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdDas              , "das"              , O_000000(2F,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(UWWXWX__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdDec              , "dec"              , O_000000(FE,1,_,_,_), O_000000(48,U,_,_,_), Enc(X86IncDec)    , F(Lock)                            , EF(WWWWW___), 0 , 0 , O(GqdwbMem)       , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdDiv              , "div"              , O_000000(F6,6,_,_,_), U                   , Enc(X86Rm_B)      , F(None)|F(Special)                 , EF(UUUUUU__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdDivpd            , "divpd"            , O_660F00(5E,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdDivps            , "divps"            , O_000F00(5E,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdDivsd            , "divsd"            , O_F20F00(5E,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdDivss            , "divss"            , O_F30F00(5E,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdDppd             , "dppd"             , O_660F3A(41,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdDpps             , "dpps"             , O_660F3A(40,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdEmms             , "emms"             , O_000F00(77,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdEnter            , "enter"            , O_000000(C8,U,_,_,_), U                   , Enc(X86Enter)     , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdExtractps        , "extractps"        , O_660F3A(17,U,_,_,_), O_660F3A(17,U,_,_,_), Enc(ExtExtract)   , F(Move)                            , EF(________), 0 , 8 , O(GqdMem)         , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdExtrq            , "extrq"            , O_660F00(79,U,_,_,_), O_660F00(78,0,_,_,_), Enc(ExtExtrq)     , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)|O(Imm)     , O(None)|O(Imm)    , U                 , U                 ),
  INST(kX86InstIdF2xm1            , "f2xm1"            , O_00_X(D9F0,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFabs             , "fabs"             , O_00_X(D9E1,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFadd             , "fadd"             , O_00_X(C0C0,0)      , U                   , Enc(FpuArith)     , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(FpMem)          , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFaddp            , "faddp"            , O_00_X(DEC0,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFbld             , "fbld"             , O_000000(DF,4,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFbstp            , "fbstp"            , O_000000(DF,6,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFchs             , "fchs"             , O_00_X(D9E0,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFclex            , "fclex"            , O_9B_X(DBE2,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovb           , "fcmovb"           , O_00_X(DAC0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(_____R__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovbe          , "fcmovbe"          , O_00_X(DAD0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(__R__R__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmove           , "fcmove"           , O_00_X(DAC8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(__R_____), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovnb          , "fcmovnb"          , O_00_X(DBC0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(_____R__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovnbe         , "fcmovnbe"         , O_00_X(DBD0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(__R__R__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovne          , "fcmovne"          , O_00_X(DBC8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(__R_____), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovnu          , "fcmovnu"          , O_00_X(DBD8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(____R___), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcmovu           , "fcmovu"           , O_00_X(DAD8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(____R___), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcom             , "fcom"             , O_00_X(D0D0,2)      , U                   , Enc(FpuCom)       , F(Fp)                              , EF(________), 0 , 0 , O(Fp)|O(Mem)      , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFcomi            , "fcomi"            , O_00_X(DBF0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(WWWWWW__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcomip           , "fcomip"           , O_00_X(DFF0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(WWWWWW__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcomp            , "fcomp"            , O_00_X(D8D8,3)      , U                   , Enc(FpuCom)       , F(Fp)                              , EF(________), 0 , 0 , O(Fp)|O(Mem)      , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFcompp           , "fcompp"           , O_00_X(DED9,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFcos             , "fcos"             , O_00_X(D9FF,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFdecstp          , "fdecstp"          , O_00_X(D9F6,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFdiv             , "fdiv"             , O_00_X(F0F8,6)      , U                   , Enc(FpuArith)     , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(FpMem)          , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFdivp            , "fdivp"            , O_00_X(DEF8,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFdivr            , "fdivr"            , O_00_X(F8F0,7)      , U                   , Enc(FpuArith)     , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(FpMem)          , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFdivrp           , "fdivrp"           , O_00_X(DEF0,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFemms            , "femms"            , O_000F00(0E,U,_,_,_), U                   , Enc(X86Op)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFfree            , "ffree"            , O_00_X(DDC0,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFiadd            , "fiadd"            , O_000000(DA,0,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFicom            , "ficom"            , O_000000(DA,2,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFicomp           , "ficomp"           , O_000000(DA,3,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFidiv            , "fidiv"            , O_000000(DA,6,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFidivr           , "fidivr"           , O_000000(DA,7,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFild             , "fild"             , O_000000(DB,0,_,_,_), O_000000(DF,5,_,_,_), Enc(FpuM)         , F(Fp)|F(Mem2_4_8)                  , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFimul            , "fimul"            , O_000000(DA,1,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFincstp          , "fincstp"          , O_00_X(D9F7,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFinit            , "finit"            , O_9B_X(DBE3,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFist             , "fist"             , O_000000(DB,2,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFistp            , "fistp"            , O_000000(DB,3,_,_,_), O_000000(DF,7,_,_,_), Enc(FpuM)         , F(Fp)|F(Mem2_4_8)                  , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFisttp           , "fisttp"           , O_000000(DB,1,_,_,_), O_000000(DD,1,_,_,_), Enc(FpuM)         , F(Fp)|F(Mem2_4_8)                  , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFisub            , "fisub"            , O_000000(DA,4,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFisubr           , "fisubr"           , O_000000(DA,5,_,_,_), U                   , Enc(FpuM)         , F(Fp)|F(Mem2_4)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFld              , "fld"              , O_000000(D9,0,_,_,_), O_000000(DB,5,_,_,_), Enc(FpuFldFst)    , F(Fp)|F(Mem4_8_10)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFld1             , "fld1"             , O_00_X(D9E8,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldcw            , "fldcw"            , O_000000(D9,5,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldenv           , "fldenv"           , O_000000(D9,4,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldl2e           , "fldl2e"           , O_00_X(D9EA,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldl2t           , "fldl2t"           , O_00_X(D9E9,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldlg2           , "fldlg2"           , O_00_X(D9EC,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldln2           , "fldln2"           , O_00_X(D9ED,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldpi            , "fldpi"            , O_00_X(D9EB,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFldz             , "fldz"             , O_00_X(D9EE,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFmul             , "fmul"             , O_00_X(C8C8,1)      , U                   , Enc(FpuArith)     , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(FpMem)          , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFmulp            , "fmulp"            , O_00_X(DEC8,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFnclex           , "fnclex"           , O_00_X(DBE2,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFninit           , "fninit"           , O_00_X(DBE3,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFnop             , "fnop"             , O_00_X(D9D0,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFnsave           , "fnsave"           , O_000000(DD,6,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFnstcw           , "fnstcw"           , O_000000(D9,7,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFnstenv          , "fnstenv"          , O_000000(D9,6,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFnstsw           , "fnstsw"           , O_000000(DD,7,_,_,_), O_00_X(DFE0,U)      , Enc(FpuStsw)      , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFpatan           , "fpatan"           , O_00_X(D9F3,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFprem            , "fprem"            , O_00_X(D9F8,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFprem1           , "fprem1"           , O_00_X(D9F5,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFptan            , "fptan"            , O_00_X(D9F2,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFrndint          , "frndint"          , O_00_X(D9FC,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFrstor           , "frstor"           , O_000000(DD,4,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFsave            , "fsave"            , O_9B0000(DD,6,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFscale           , "fscale"           , O_00_X(D9FD,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFsin             , "fsin"             , O_00_X(D9FE,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFsincos          , "fsincos"          , O_00_X(D9FB,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFsqrt            , "fsqrt"            , O_00_X(D9FA,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFst              , "fst"              , O_000000(D9,2,_,_,_), U                   , Enc(FpuFldFst)    , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFstcw            , "fstcw"            , O_9B0000(D9,7,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFstenv           , "fstenv"           , O_9B0000(D9,6,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFstp             , "fstp"             , O_000000(D9,3,_,_,_), O_000000(DB,7,_,_,_), Enc(FpuFldFst)    , F(Fp)|F(Mem4_8_10)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFstsw            , "fstsw"            , O_9B0000(DD,7,_,_,_), O_9B_X(DFE0,U)      , Enc(FpuStsw)      , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFsub             , "fsub"             , O_00_X(E0E8,4)      , U                   , Enc(FpuArith)     , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(FpMem)          , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFsubp            , "fsubp"            , O_00_X(DEE8,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFsubr            , "fsubr"            , O_00_X(E8E0,5)      , U                   , Enc(FpuArith)     , F(Fp)|F(Mem4_8)                    , EF(________), 0 , 0 , O(FpMem)          , O(Fp)             , U                 , U                 , U                 ),
  INST(kX86InstIdFsubrp           , "fsubrp"           , O_00_X(DEE0,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFtst             , "ftst"             , O_00_X(D9E4,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFucom            , "fucom"            , O_00_X(DDE0,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFucomi           , "fucomi"           , O_00_X(DBE8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(WWWWWW__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFucomip          , "fucomip"          , O_00_X(DFE8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(WWWWWW__), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFucomp           , "fucomp"           , O_00_X(DDE8,U)      , U                   , Enc(FpuRDef)      , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFucompp          , "fucompp"          , O_00_X(DAE9,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFwait            , "fwait"            , O_000000(DB,U,_,_,_), U                   , Enc(X86Op)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFxam             , "fxam"             , O_00_X(D9E5,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFxch             , "fxch"             , O_00_X(D9C8,U)      , U                   , Enc(FpuR)         , F(Fp)                              , EF(________), 0 , 0 , O(Fp)             , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFxrstor          , "fxrstor"          , O_000F00(AE,1,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFxsave           , "fxsave"           , O_000F00(AE,0,_,_,_), U                   , Enc(X86M)         , F(Fp)                              , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFxtract          , "fxtract"          , O_00_X(D9F4,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFyl2x            , "fyl2x"            , O_00_X(D9F1,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdFyl2xp1          , "fyl2xp1"          , O_00_X(D9F9,U)      , U                   , Enc(FpuOp)        , F(Fp)                              , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdHaddpd           , "haddpd"           , O_660F00(7C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdHaddps           , "haddps"           , O_F20F00(7C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdHsubpd           , "hsubpd"           , O_660F00(7D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdHsubps           , "hsubps"           , O_F20F00(7D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdIdiv             , "idiv"             , O_000000(F6,7,_,_,_), U                   , Enc(X86Rm_B)      , F(None)|F(Special)                 , EF(UUUUUU__), 0 , 0 , 0                 , 0                 , U                 , U                 , U                 ),
  INST(kX86InstIdImul             , "imul"             , U                   , U                   , Enc(X86Imul)      , F(None)|F(Special)                 , EF(WUUUUW__), 0 , 0 , 0                 , 0                 , U                 , U                 , U                 ),
  INST(kX86InstIdInc              , "inc"              , O_000000(FE,0,_,_,_), O_000000(40,U,_,_,_), Enc(X86IncDec)    , F(Lock)                            , EF(WWWWW___), 0 , 0 , O(GqdwbMem)       , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdInsertps         , "insertps"         , O_660F3A(21,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdInsertq          , "insertq"          , O_F20F00(79,U,_,_,_), O_F20F00(78,U,_,_,_), Enc(ExtInsertq)   , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(None)|O(Imm)    , O(None)|O(Imm)    , U                 ),
  INST(kX86InstIdInt              , "int"              , O_000000(CC,U,_,_,_), U                   , Enc(X86Int)       , F(None)                            , EF(_______W), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJa               , "ja"               , O_000000(77,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R__R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJae              , "jae"              , O_000000(73,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_____R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJb               , "jb"               , O_000000(72,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_____R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJbe              , "jbe"              , O_000000(76,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R__R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJc               , "jc"               , O_000000(72,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_____R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJe               , "je"               , O_000000(74,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJg               , "jg"               , O_000000(7F,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RRR_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJge              , "jge"              , O_000000(7D,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RR______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJl               , "jl"               , O_000000(7C,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RR______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJle              , "jle"              , O_000000(7E,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RRR_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJna              , "jna"              , O_000000(76,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R__R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnae             , "jnae"             , O_000000(72,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_____R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnb              , "jnb"              , O_000000(73,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_____R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnbe             , "jnbe"             , O_000000(77,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R__R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnc              , "jnc"              , O_000000(73,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_____R__), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJne              , "jne"              , O_000000(75,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJng              , "jng"              , O_000000(7E,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RRR_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnge             , "jnge"             , O_000000(7C,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RR______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnl              , "jnl"              , O_000000(7D,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RR______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnle             , "jnle"             , O_000000(7F,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(RRR_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJno              , "jno"              , O_000000(71,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(R_______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnp              , "jnp"              , O_000000(7B,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(____R___), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJns              , "jns"              , O_000000(79,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_R______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJnz              , "jnz"              , O_000000(75,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJo               , "jo"               , O_000000(70,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(R_______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJp               , "jp"               , O_000000(7A,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(____R___), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJpe              , "jpe"              , O_000000(7A,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(____R___), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJpo              , "jpo"              , O_000000(7B,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(____R___), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJs               , "js"               , O_000000(78,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(_R______), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJz               , "jz"               , O_000000(74,U,_,_,_), U                   , Enc(X86Jcc)       , F(Flow)                            , EF(__R_____), 0 , 0 , O(Label)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdJecxz            , "jecxz"            , O_000000(E3,U,_,_,_), U                   , Enc(X86Jecxz)     , F(Flow)|F(Special)                 , EF(________), 0 , 0 , O(Gqdw)           , O(Label)          , U                 , U                 , U                 ),
  INST(kX86InstIdJmp              , "jmp"              , O_000000(FF,4,_,_,_), O_000000(E9,U,_,_,_), Enc(X86Jmp)       , F(Flow)                            , EF(________), 0 , 0 , O(Label)|O(Imm)   , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLahf             , "lahf"             , O_000000(9F,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(_RRRRR__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLddqu            , "lddqu"            , O_F20F00(F0,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdLdmxcsr          , "ldmxcsr"          , O_000F00(AE,2,_,_,_), U                   , Enc(X86M)         , F(None)                            , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLea              , "lea"              , O_000000(8D,U,_,_,_), U                   , Enc(X86Lea)       , F(Move)                            , EF(________), 0 , 0 , O(Gqd)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdLeave            , "leave"            , O_000000(C9,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLfence           , "lfence"           , O_000F00(AE,5,_,_,_), U                   , Enc(ExtFence)     , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLodsB            , "lods_b"           , O_000000(AC,U,_,_,_), U                   , Enc(X86Op)        , F(Move)|F(Special)                 , EF(______R_), 0 , 1 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLodsD            , "lods_d"           , O_000000(AD,U,_,_,_), U                   , Enc(X86Op)        , F(Move)|F(Special)                 , EF(______R_), 0 , 4 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLodsQ            , "lods_q"           , O_000000(AD,U,_,W,_), U                   , Enc(X86Op)        , F(Move)|F(Special)                 , EF(______R_), 0 , 8 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLodsW            , "lods_w"           , O_000000(AD,U,_,_,_), U                   , Enc(X86Op_66H)    , F(Move)|F(Special)                 , EF(______R_), 0 , 2 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdLzcnt            , "lzcnt"            , O_F30F00(BD,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(UUWUUW__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdMaskmovdqu       , "maskmovdqu"       , O_660F00(57,U,_,_,_), U                   , Enc(ExtRm)        , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMaskmovq         , "maskmovq"         , O_000F00(F7,U,_,_,_), U                   , Enc(ExtRm)        , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mm)             , O(Mm)             , U                 , U                 , U                 ),
  INST(kX86InstIdMaxpd            , "maxpd"            , O_660F00(5F,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMaxps            , "maxps"            , O_000F00(5F,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMaxsd            , "maxsd"            , O_F20F00(5F,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMaxss            , "maxss"            , O_F30F00(5F,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMfence           , "mfence"           , O_000F00(AE,6,_,_,_), U                   , Enc(ExtFence)     , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdMinpd            , "minpd"            , O_660F00(5D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMinps            , "minps"            , O_000F00(5D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMinsd            , "minsd"            , O_F20F00(5D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMinss            , "minss"            , O_F30F00(5D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMonitor          , "monitor"          , O_000F01(C8,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdMov              , "mov"              , U                   , U                   , Enc(X86Mov)       , F(Move)                            , EF(________), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdMovPtr           , "mov_ptr"          , O_000000(A0,U,_,_,_), O_000000(A2,U,_,_,_), Enc(X86MovPtr)    , F(Move)|F(Special)                 , EF(________), 0 , 0 , O(Gqdwb)          , O(Imm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovapd           , "movapd"           , O_660F00(28,U,_,_,_), O_660F00(29,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovaps           , "movaps"           , O_000F00(28,U,_,_,_), O_000F00(29,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovbe            , "movbe"            , O_000F38(F0,U,_,_,_), O_000F38(F1,U,_,_,_), Enc(ExtMovBe)     , F(Move)                            , EF(________), 0 , 0 , O(GqdwMem)        , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdMovd             , "movd"             , O_000F00(6E,U,_,_,_), O_000F00(7E,U,_,_,_), Enc(ExtMovD)      , F(Move)                            , EF(________), 0 , 16, O(Gd)|O(MmXmmMem) , O(Gd)|O(MmXmmMem) , U                 , U                 , U                 ),
  INST(kX86InstIdMovddup          , "movddup"          , O_F20F00(12,U,_,_,_), U                   , Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovdq2q          , "movdq2q"          , O_F20F00(D6,U,_,_,_), U                   , Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(Mm)             , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovdqa           , "movdqa"           , O_660F00(6F,U,_,_,_), O_660F00(7F,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovdqu           , "movdqu"           , O_F30F00(6F,U,_,_,_), O_F30F00(7F,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovhlps          , "movhlps"          , O_000F00(12,U,_,_,_), U                   , Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(Xmm)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovhpd           , "movhpd"           , O_660F00(16,U,_,_,_), O_660F00(17,U,_,_,_), Enc(ExtMov)       , F(None)                            , EF(________), 8 , 8 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovhps           , "movhps"           , O_000F00(16,U,_,_,_), O_000F00(17,U,_,_,_), Enc(ExtMov)       , F(None)                            , EF(________), 8 , 8 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovlhps          , "movlhps"          , O_000F00(16,U,_,_,_), U                   , Enc(ExtMov)       , F(None)                            , EF(________), 8 , 8 , O(Xmm)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovlpd           , "movlpd"           , O_660F00(12,U,_,_,_), O_660F00(13,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovlps           , "movlps"           , O_000F00(12,U,_,_,_), O_000F00(13,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovmskpd         , "movmskpd"         , O_660F00(50,U,_,_,_), U                   , Enc(ExtMovNoRexW) , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovmskps         , "movmskps"         , O_000F00(50,U,_,_,_), U                   , Enc(ExtMovNoRexW) , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovntdq          , "movntdq"          , U                   , O_660F00(E7,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(Mem)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovntdqa         , "movntdqa"         , O_660F38(2A,U,_,_,_), U                   , Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovnti           , "movnti"           , U                   , O_000F00(C3,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(Mem)            , O(Gqd)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovntpd          , "movntpd"          , U                   , O_660F00(2B,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(Mem)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovntps          , "movntps"          , U                   , O_000F00(2B,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(Mem)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovntq           , "movntq"           , U                   , O_000F00(E7,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(Mem)            , O(Mm)             , U                 , U                 , U                 ),
  INST(kX86InstIdMovntsd          , "movntsd"          , U                   , O_F20F00(2B,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 8 , O(Mem)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovntss          , "movntss"          , U                   , O_F30F00(2B,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 4 , O(Mem)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdMovq             , "movq"             , O_000F00(6E,U,_,W,_), O_000F00(7E,U,_,W,_), Enc(ExtMovQ)      , F(Move)                            , EF(________), 0 , 16, O(Gq)|O(MmXmmMem) , O(Gq)|O(MmXmmMem) , U                 , U                 , U                 ),
  INST(kX86InstIdMovq2dq          , "movq2dq"          , O_F30F00(D6,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(Mm)             , U                 , U                 , U                 ),
  INST(kX86InstIdMovsB            , "movs_b"           , O_000000(A4,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdMovsD            , "movs_d"           , O_000000(A5,U,_,_,_), U                   , Enc(X86Op)        , F(Move)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdMovsQ            , "movs_q"           , O_000000(A5,U,_,W,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdMovsW            , "movs_w"           , O_000000(A5,U,_,_,_), U                   , Enc(X86Op_66H)    , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdMovsd            , "movsd"            , O_F20F00(10,U,_,_,_), O_F20F00(11,U,_,_,_), Enc(ExtMov)       , F(Move)           |F(Z)            , EF(________), 0 , 8 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovshdup         , "movshdup"         , O_F30F00(16,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovsldup         , "movsldup"         , O_F30F00(12,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovss            , "movss"            , O_F30F00(10,U,_,_,_), O_F30F00(11,U,_,_,_), Enc(ExtMov)       , F(Move)           |F(Z)            , EF(________), 0 , 4 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovsx            , "movsx"            , O_000F00(BE,U,_,_,_), U                   , Enc(X86MovSxZx)   , F(Move)                            , EF(________), 0 , 0 , O(Gqdw)           , O(GwbMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovsxd           , "movsxd"           , O_000000(63,U,_,_,_), U                   , Enc(X86MovSxd)    , F(Move)                            , EF(________), 0 , 0 , O(Gq)             , O(GdMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdMovupd           , "movupd"           , O_660F00(10,U,_,_,_), O_660F00(11,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovups           , "movups"           , O_000F00(10,U,_,_,_), O_000F00(11,U,_,_,_), Enc(ExtMov)       , F(Move)                            , EF(________), 0 , 16, O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMovzx            , "movzx"            , O_000F00(B6,U,_,_,_), U                   , Enc(X86MovSxZx)   , F(Move)                            , EF(________), 0 , 0 , O(Gqdw)           , O(GwbMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMpsadbw          , "mpsadbw"          , O_660F3A(42,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdMul              , "mul"              , O_000000(F6,4,_,_,_), U                   , Enc(X86Rm_B)      , F(None)|F(Special)                 , EF(WUUUUW__), 0 , 0 , 0                 , 0                 , U                 , U                 , U                 ),
  INST(kX86InstIdMulpd            , "mulpd"            , O_660F00(59,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMulps            , "mulps"            , O_000F00(59,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMulsd            , "mulsd"            , O_F20F00(59,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMulss            , "mulss"            , O_F30F00(59,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdMulx             , "mulx"             , O_F20F38(F6,U,_,_,_), U                   , Enc(AvxRvm)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(Gqd)            , O(GqdMem)         , U                 , U                 ),
  INST(kX86InstIdMwait            , "mwait"            , O_000F01(C9,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdNeg              , "neg"              , O_000000(F6,3,_,_,_), U                   , Enc(X86Rm_B)      , F(Lock)                            , EF(WWWWWW__), 0 , 0 , O(GqdwbMem)       , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdNop              , "nop"              , O_000000(90,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdNot              , "not"              , O_000000(F6,2,_,_,_), U                   , Enc(X86Rm_B)      , F(Lock)                            , EF(________), 0 , 0 , O(GqdwbMem)       , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdOr               , "or"               , O_000000(08,1,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdOrpd             , "orpd"             , O_660F00(56,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdOrps             , "orps"             , O_000F00(56,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPabsb            , "pabsb"            , O_000F38(1C,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPabsd            , "pabsd"            , O_000F38(1E,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPabsw            , "pabsw"            , O_000F38(1D,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPackssdw         , "packssdw"         , O_000F00(6B,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPacksswb         , "packsswb"         , O_000F00(63,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPackusdw         , "packusdw"         , O_660F38(2B,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPackuswb         , "packuswb"         , O_000F00(67,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddb            , "paddb"            , O_000F00(FC,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddd            , "paddd"            , O_000F00(FE,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddq            , "paddq"            , O_000F00(D4,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddsb           , "paddsb"           , O_000F00(EC,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddsw           , "paddsw"           , O_000F00(ED,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddusb          , "paddusb"          , O_000F00(DC,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddusw          , "paddusw"          , O_000F00(DD,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPaddw            , "paddw"            , O_000F00(FD,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPalignr          , "palignr"          , O_000F3A(0F,U,_,_,_), U                   , Enc(ExtRmi_P)     , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPand             , "pand"             , O_000F00(DB,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPandn            , "pandn"            , O_000F00(DF,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPause            , "pause"            , O_F30000(90,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPavgb            , "pavgb"            , O_000F00(E0,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPavgw            , "pavgw"            , O_000F00(E3,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPblendvb         , "pblendvb"         , O_660F38(10,U,_,_,_), U                   , Enc(ExtRm)        , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPblendw          , "pblendw"          , O_660F3A(0E,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPclmulqdq        , "pclmulqdq"        , O_660F3A(44,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPcmpeqb          , "pcmpeqb"          , O_000F00(74,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpeqd          , "pcmpeqd"          , O_000F00(76,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpeqq          , "pcmpeqq"          , O_660F38(29,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpeqw          , "pcmpeqw"          , O_000F00(75,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpestri        , "pcmpestri"        , O_660F3A(61,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPcmpestrm        , "pcmpestrm"        , O_660F3A(60,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPcmpgtb          , "pcmpgtb"          , O_000F00(64,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpgtd          , "pcmpgtd"          , O_000F00(66,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpgtq          , "pcmpgtq"          , O_660F38(37,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpgtw          , "pcmpgtw"          , O_000F00(65,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPcmpistri        , "pcmpistri"        , O_660F3A(63,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPcmpistrm        , "pcmpistrm"        , O_660F3A(62,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPdep             , "pdep"             , O_F20F38(F5,U,_,_,_), U                   , Enc(AvxRvm)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(Gqd)            , O(GqdMem)         , U                 , U                 ),
  INST(kX86InstIdPext             , "pext"             , O_F30F38(F5,U,_,_,_), U                   , Enc(AvxRvm)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(Gqd)            , O(GqdMem)         , U                 , U                 ),
  INST(kX86InstIdPextrb           , "pextrb"           , O_000F3A(14,U,_,_,_), O_000F3A(14,U,_,_,_), Enc(ExtExtract)   , F(Move)                            , EF(________), 0 , 8 , O(Gd)|O(Gb)|O(Mem), O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdPextrd           , "pextrd"           , O_000F3A(16,U,_,_,_), O_000F3A(16,U,_,_,_), Enc(ExtExtract)   , F(Move)                            , EF(________), 0 , 8 , O(GdMem)          , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdPextrq           , "pextrq"           , O_000F3A(16,U,_,W,_), O_000F3A(16,U,_,W,_), Enc(ExtExtract)   , F(Move)                            , EF(________), 0 , 8 , O(GqdMem)         , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdPextrw           , "pextrw"           , O_000F00(C5,U,_,_,_), O_000F3A(15,U,_,_,_), Enc(ExtExtract)   , F(Move)                            , EF(________), 0 , 8 , O(GdMem)          , O(MmXmm)          , U                 , U                 , U                 ),
  INST(kX86InstIdPf2id            , "pf2id"            , O_000F0F(1D,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPf2iw            , "pf2iw"            , O_000F0F(1C,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfacc            , "pfacc"            , O_000F0F(AE,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfadd            , "pfadd"            , O_000F0F(9E,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfcmpeq          , "pfcmpeq"          , O_000F0F(B0,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfcmpge          , "pfcmpge"          , O_000F0F(90,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfcmpgt          , "pfcmpgt"          , O_000F0F(A0,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfmax            , "pfmax"            , O_000F0F(A4,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfmin            , "pfmin"            , O_000F0F(94,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfmul            , "pfmul"            , O_000F0F(B4,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfnacc           , "pfnacc"           , O_000F0F(8A,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfpnacc          , "pfpnacc"          , O_000F0F(8E,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfrcp            , "pfrcp"            , O_000F0F(96,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfrcpit1         , "pfrcpit1"         , O_000F0F(A6,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfrcpit2         , "pfrcpit2"         , O_000F0F(B6,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfrsqit1         , "pfrsqit1"         , O_000F0F(A7,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfrsqrt          , "pfrsqrt"          , O_000F0F(97,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfsub            , "pfsub"            , O_000F0F(9A,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPfsubr           , "pfsubr"           , O_000F0F(AA,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPhaddd           , "phaddd"           , O_000F38(02,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPhaddsw          , "phaddsw"          , O_000F38(03,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPhaddw           , "phaddw"           , O_000F38(01,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPhminposuw       , "phminposuw"       , O_660F38(41,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPhsubd           , "phsubd"           , O_000F38(06,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPhsubsw          , "phsubsw"          , O_000F38(07,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPhsubw           , "phsubw"           , O_000F38(05,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPi2fd            , "pi2fd"            , O_000F0F(0D,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPi2fw            , "pi2fw"            , O_000F0F(0C,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPinsrb           , "pinsrb"           , O_660F3A(20,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(GdMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPinsrd           , "pinsrd"           , O_660F3A(22,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(GdMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPinsrq           , "pinsrq"           , O_660F3A(22,U,_,W,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(GqMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPinsrw           , "pinsrw"           , O_000F00(C4,U,_,_,_), U                   , Enc(ExtRmi_P)     , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(GdMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPmaddubsw        , "pmaddubsw"        , O_000F38(04,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmaddwd          , "pmaddwd"          , O_000F00(F5,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmaxsb           , "pmaxsb"           , O_660F38(3C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmaxsd           , "pmaxsd"           , O_660F38(3D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmaxsw           , "pmaxsw"           , O_000F00(EE,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmaxub           , "pmaxub"           , O_000F00(DE,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmaxud           , "pmaxud"           , O_660F38(3F,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmaxuw           , "pmaxuw"           , O_660F38(3E,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPminsb           , "pminsb"           , O_660F38(38,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPminsd           , "pminsd"           , O_660F38(39,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPminsw           , "pminsw"           , O_000F00(EA,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPminub           , "pminub"           , O_000F00(DA,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPminud           , "pminud"           , O_660F38(3B,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPminuw           , "pminuw"           , O_660F38(3A,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovmskb         , "pmovmskb"         , O_000F00(D7,U,_,_,_), U                   , Enc(ExtRm_PQ)     , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , O(MmXmm)          , U                 , U                 , U                 ),
  INST(kX86InstIdPmovsxbd         , "pmovsxbd"         , O_660F38(21,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovsxbq         , "pmovsxbq"         , O_660F38(22,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovsxbw         , "pmovsxbw"         , O_660F38(20,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovsxdq         , "pmovsxdq"         , O_660F38(25,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovsxwd         , "pmovsxwd"         , O_660F38(23,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovsxwq         , "pmovsxwq"         , O_660F38(24,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovzxbd         , "pmovzxbd"         , O_660F38(31,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovzxbq         , "pmovzxbq"         , O_660F38(32,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovzxbw         , "pmovzxbw"         , O_660F38(30,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovzxdq         , "pmovzxdq"         , O_660F38(35,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovzxwd         , "pmovzxwd"         , O_660F38(33,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmovzxwq         , "pmovzxwq"         , O_660F38(34,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmuldq           , "pmuldq"           , O_660F38(28,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmulhrsw         , "pmulhrsw"         , O_000F38(0B,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmulhuw          , "pmulhuw"          , O_000F00(E4,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmulhw           , "pmulhw"           , O_000F00(E5,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmulld           , "pmulld"           , O_660F38(40,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPmullw           , "pmullw"           , O_000F00(D5,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPmuludq          , "pmuludq"          , O_000F00(F4,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPop              , "pop"              , O_000000(8F,0,_,_,_), O_000000(58,U,_,_,_), Enc(X86Pop)       , F(None)|F(Special)                 , EF(________), 0 , 0 , 0                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPopa             , "popa"             , O_000000(61,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPopcnt           , "popcnt"           , O_F30F00(B8,U,_,_,_), U                   , Enc(X86RegRm)     , F(None)                            , EF(WWWWWW__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdPopf             , "popf"             , O_000000(9D,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWWW), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPor              , "por"              , O_000F00(EB,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPrefetch         , "prefetch"         , O_000F00(18,U,_,_,_), U                   , Enc(ExtPrefetch)  , F(None)                            , EF(________), 0 , 0 , O(Mem)            , O(Imm)            , U                 , U                 , U                 ),
  INST(kX86InstIdPrefetch3dNow    , "prefetch_3dnow"   , O_000F00(0D,0,_,_,_), U                   , Enc(X86M)         , F(None)                            , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPrefetchw3dNow   , "prefetchw_3dnow"  , O_000F00(0D,1,_,_,_), U                   , Enc(X86M)         , F(None)                            , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPsadbw           , "psadbw"           , O_000F00(F6,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPshufb           , "pshufb"           , O_000F38(00,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPshufd           , "pshufd"           , O_660F00(70,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPshufhw          , "pshufhw"          , O_F30F00(70,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPshuflw          , "pshuflw"          , O_F20F00(70,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPshufw           , "pshufw"           , O_000F00(70,U,_,_,_), U                   , Enc(ExtRmi_P)     , F(Move)                            , EF(________), 0 , 8 , O(Mm)             , O(MmMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdPsignb           , "psignb"           , O_000F38(08,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsignd           , "psignd"           , O_000F38(0A,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsignw           , "psignw"           , O_000F38(09,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPslld            , "pslld"            , O_000F00(F2,U,_,_,_), O_000F00(72,6,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPslldq           , "pslldq"           , U                   , O_660F00(73,7,_,_,_), Enc(ExtRmRi)      , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(Imm)            , U                 , U                 , U                 ),
  INST(kX86InstIdPsllq            , "psllq"            , O_000F00(F3,U,_,_,_), O_000F00(73,6,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsllw            , "psllw"            , O_000F00(F1,U,_,_,_), O_000F00(71,6,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsrad            , "psrad"            , O_000F00(E2,U,_,_,_), O_000F00(72,4,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsraw            , "psraw"            , O_000F00(E1,U,_,_,_), O_000F00(71,4,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsrld            , "psrld"            , O_000F00(D2,U,_,_,_), O_000F00(72,2,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsrldq           , "psrldq"           , U                   , O_660F00(73,3,_,_,_), Enc(ExtRmRi)      , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(Imm)            , U                 , U                 , U                 ),
  INST(kX86InstIdPsrlq            , "psrlq"            , O_000F00(D3,U,_,_,_), O_000F00(73,2,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsrlw            , "psrlw"            , O_000F00(D1,U,_,_,_), O_000F00(71,2,_,_,_), Enc(ExtRmRi_P)    , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdPsubb            , "psubb"            , O_000F00(F8,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubd            , "psubd"            , O_000F00(FA,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubq            , "psubq"            , O_000F00(FB,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubsb           , "psubsb"           , O_000F00(E8,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubsw           , "psubsw"           , O_000F00(E9,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubusb          , "psubusb"          , O_000F00(D8,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubusw          , "psubusw"          , O_000F00(D9,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPsubw            , "psubw"            , O_000F00(F9,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPswapd           , "pswapd"           , O_000F0F(BB,U,_,_,_), U                   , Enc(3dNow)        , F(None)                            , EF(________), 0 , 0 , O(Mm)             , O(MmMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdPtest            , "ptest"            , O_660F38(17,U,_,_,_), U                   , Enc(ExtRm)        , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPunpckhbw        , "punpckhbw"        , O_000F00(68,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPunpckhdq        , "punpckhdq"        , O_000F00(6A,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPunpckhqdq       , "punpckhqdq"       , O_660F00(6D,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPunpckhwd        , "punpckhwd"        , O_000F00(69,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPunpcklbw        , "punpcklbw"        , O_000F00(60,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPunpckldq        , "punpckldq"        , O_000F00(62,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPunpcklqdq       , "punpcklqdq"       , O_660F00(6C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdPunpcklwd        , "punpcklwd"        , O_000F00(61,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdPush             , "push"             , O_000000(FF,6,_,_,_), O_000000(50,U,_,_,_), Enc(X86Push)      , F(None)|F(Special)                 , EF(________), 0 , 0 , 0                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPusha            , "pusha"            , O_000000(60,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPushf            , "pushf"            , O_000000(9C,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(RRRRRRRR), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdPxor             , "pxor"             , O_000F00(EF,U,_,_,_), U                   , Enc(ExtRm_P)      , F(None)                            , EF(________), 0 , 0 , O(MmXmm)          , O(MmXmmMem)       , U                 , U                 , U                 ),
  INST(kX86InstIdRcl              , "rcl"              , O_000000(D0,2,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(W____X__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdRcpps            , "rcpps"            , O_000F00(53,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdRcpss            , "rcpss"            , O_F30F00(53,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 4 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdRcr              , "rcr"              , O_000000(D0,3,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(W____X__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdRdfsbase         , "rdfsbase"         , O_F30F00(AE,0,_,_,_), U                   , Enc(X86Rm)        , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRdgsbase         , "rdgsbase"         , O_F30F00(AE,1,_,_,_), U                   , Enc(X86Rm)        , F(Move)                            , EF(________), 0 , 8 , O(Gqd)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRdrand           , "rdrand"           , O_000F00(C7,6,_,_,_), U                   , Enc(X86Rm)        , F(Move)                            , EF(WWWWWW__), 0 , 8 , O(Gqdw)           , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRdtsc            , "rdtsc"            , O_000F00(31,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRdtscp           , "rdtscp"           , O_000F01(F9,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepLodsB         , "rep lods_b"       , O_000000(AC,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepLodsD         , "rep lods_d"       , O_000000(AD,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepLodsQ         , "rep lods_q"       , O_000000(AD,1,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepLodsW         , "rep lods_w"       , O_660000(AD,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepMovsB         , "rep movs_b"       , O_000000(A4,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepMovsD         , "rep movs_d"       , O_000000(A5,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepMovsQ         , "rep movs_q"       , O_000000(A5,1,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepMovsW         , "rep movs_w"       , O_660000(A5,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepStosB         , "rep stos_b"       , O_000000(AA,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepStosD         , "rep stos_d"       , O_000000(AB,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepStosQ         , "rep stos_q"       , O_000000(AB,1,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepStosW         , "rep stos_w"       , O_660000(AB,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(______R_), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRepeCmpsB        , "repe cmps_b"      , O_000000(A6,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeCmpsD        , "repe cmps_d"      , O_000000(A7,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeCmpsQ        , "repe cmps_q"      , O_000000(A7,1,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeCmpsW        , "repe cmps_w"      , O_660000(A7,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeScasB        , "repe scas_b"      , O_000000(AE,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeScasD        , "repe scas_d"      , O_000000(AF,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeScasQ        , "repe scas_q"      , O_000000(AF,1,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepeScasW        , "repe scas_w"      , O_660000(AF,1,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneCmpsB       , "repne cmps_b"     , O_000000(A6,0,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneCmpsD       , "repne cmps_d"     , O_000000(A7,0,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneCmpsQ       , "repne cmps_q"     , O_000000(A7,0,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneCmpsW       , "repne cmps_w"     , O_660000(A7,0,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneScasB       , "repne scas_b"     , O_000000(AE,0,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneScasD       , "repne scas_d"     , O_000000(AF,0,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneScasQ       , "repne scas_q"     , O_000000(AF,0,_,W,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRepneScasW       , "repne scas_w"     , O_660000(AF,0,_,_,_), U                   , Enc(X86Rep)       , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , O(Mem)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdRet              , "ret"              , O_000000(C2,U,_,_,_), U                   , Enc(X86Ret)       , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdRol              , "rol"              , O_000000(D0,0,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(W____W__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdRor              , "ror"              , O_000000(D0,1,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(W____W__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdRorx             , "rorx"             , O_F20F3A(F0,U,_,_,_), U                   , Enc(AvxRmi)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(GqdMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdRoundpd          , "roundpd"          , O_660F3A(09,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdRoundps          , "roundps"          , O_660F3A(08,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdRoundsd          , "roundsd"          , O_660F3A(0B,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 8 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdRoundss          , "roundss"          , O_660F3A(0A,U,_,_,_), U                   , Enc(ExtRmi)       , F(Move)                            , EF(________), 0 , 4 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdRsqrtps          , "rsqrtps"          , O_000F00(52,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdRsqrtss          , "rsqrtss"          , O_F30F00(52,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 4 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSahf             , "sahf"             , O_000000(9E,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(_WWWWW__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSal              , "sal"              , O_000000(D0,4,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdSar              , "sar"              , O_000000(D0,7,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdSarx             , "sarx"             , O_F30F38(F7,U,_,_,_), U                   , Enc(AvxRmv)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                 ),
  INST(kX86InstIdSbb              , "sbb"              , O_000000(18,3,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWWWX__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdScasB            , "scas_b"           , O_000000(AE,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdScasD            , "scas_d"           , O_000000(AF,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdScasQ            , "scas_q"           , O_000000(AF,U,_,W,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdScasW            , "scas_w"           , O_000000(AF,U,_,_,_), U                   , Enc(X86Op_66H)    , F(None)|F(Special)                 , EF(WWWWWWR_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSeta             , "seta"             , O_000F00(97,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R__R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetae            , "setae"            , O_000F00(93,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_____R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetb             , "setb"             , O_000F00(92,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_____R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetbe            , "setbe"            , O_000F00(96,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R__R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetc             , "setc"             , O_000F00(92,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_____R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSete             , "sete"             , O_000F00(94,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetg             , "setg"             , O_000F00(9F,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RRR_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetge            , "setge"            , O_000F00(9D,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RR______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetl             , "setl"             , O_000F00(9C,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RR______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetle            , "setle"            , O_000F00(9E,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RRR_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetna            , "setna"            , O_000F00(96,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R__R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnae           , "setnae"           , O_000F00(92,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_____R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnb            , "setnb"            , O_000F00(93,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_____R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnbe           , "setnbe"           , O_000F00(97,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R__R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnc            , "setnc"            , O_000F00(93,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_____R__), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetne            , "setne"            , O_000F00(95,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetng            , "setng"            , O_000F00(9E,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RRR_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnge           , "setnge"           , O_000F00(9C,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RR______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnl            , "setnl"            , O_000F00(9D,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RR______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnle           , "setnle"           , O_000F00(9F,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(RRR_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetno            , "setno"            , O_000F00(91,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(R_______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnp            , "setnp"            , O_000F00(9B,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(____R___), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetns            , "setns"            , O_000F00(99,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_R______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetnz            , "setnz"            , O_000F00(95,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSeto             , "seto"             , O_000F00(90,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(R_______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetp             , "setp"             , O_000F00(9A,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(____R___), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetpe            , "setpe"            , O_000F00(9A,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(____R___), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetpo            , "setpo"            , O_000F00(9B,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(____R___), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSets             , "sets"             , O_000F00(98,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(_R______), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSetz             , "setz"             , O_000F00(94,U,_,_,_), U                   , Enc(X86Set)       , F(Move)                            , EF(__R_____), 0 , 1 , O(GbMem)          , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSfence           , "sfence"           , O_000F00(AE,7,_,_,_), U                   , Enc(ExtFence)     , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdShl              , "shl"              , O_000000(D0,4,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdShld             , "shld"             , O_000F00(A4,U,_,_,_), U                   , Enc(X86Shlrd)     , F(None)|F(Special)                 , EF(UWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gb)             , U                 , U                 , U                 ),
  INST(kX86InstIdShlx             , "shlx"             , O_660F38(F7,U,_,_,_), U                   , Enc(AvxRmv)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                 ),
  INST(kX86InstIdShr              , "shr"              , O_000000(D0,5,_,_,_), U                   , Enc(X86Rot)       , F(None)|F(Special)                 , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gb)|O(Imm)      , U                 , U                 , U                 ),
  INST(kX86InstIdShrd             , "shrd"             , O_000F00(AC,U,_,_,_), U                   , Enc(X86Shlrd)     , F(None)|F(Special)                 , EF(UWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gqdwb)          , U                 , U                 , U                 ),
  INST(kX86InstIdShrx             , "shrx"             , O_F20F38(F7,U,_,_,_), U                   , Enc(AvxRmv)       , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , O(GqdMem)         , O(Gqd)            , U                 , U                 ),
  INST(kX86InstIdShufpd           , "shufpd"           , O_660F00(C6,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdShufps           , "shufps"           , O_000F00(C6,U,_,_,_), U                   , Enc(ExtRmi)       , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdSqrtpd           , "sqrtpd"           , O_660F00(51,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSqrtps           , "sqrtps"           , O_000F00(51,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 16, O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSqrtsd           , "sqrtsd"           , O_F20F00(51,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 8 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSqrtss           , "sqrtss"           , O_F30F00(51,U,_,_,_), U                   , Enc(ExtRm)        , F(Move)                            , EF(________), 0 , 4 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdStc              , "stc"              , O_000000(F9,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(_____W__), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdStd              , "std"              , O_000000(FD,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(______W_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdStmxcsr          , "stmxcsr"          , O_000F00(AE,3,_,_,_), U                   , Enc(X86M)         , F(None)                            , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdStosB            , "stos_b"           , O_000000(AA,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(______R_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdStosD            , "stos_d"           , O_000000(AB,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(______R_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdStosQ            , "stos_q"           , O_000000(AB,U,_,W,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(______R_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdStosW            , "stos_w"           , O_000000(AB,U,_,_,_), U                   , Enc(X86Op_66H)    , F(None)|F(Special)                 , EF(______R_), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdSub              , "sub"              , O_000000(28,5,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWWWW__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdSubpd            , "subpd"            , O_660F00(5C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSubps            , "subps"            , O_000F00(5C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSubsd            , "subsd"            , O_F20F00(5C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdSubss            , "subss"            , O_F30F00(5C,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdTest             , "test"             , O_000000(84,U,_,_,_), O_000000(F6,U,_,_,_), Enc(X86Test)      , F(Test)                            , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(Gqdwb)|O(Imm)   , U                 , U                 , U                 ),
  INST(kX86InstIdTzcnt            , "tzcnt"            , O_F30F00(BC,U,_,_,_), U                   , Enc(X86RegRm)     , F(Move)                            , EF(UUWUUW__), 0 , 0 , O(Gqdw)           , O(GqdwMem)        , U                 , U                 , U                 ),
  INST(kX86InstIdUcomisd          , "ucomisd"          , O_660F00(2E,U,_,_,_), U                   , Enc(ExtRm)        , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdUcomiss          , "ucomiss"          , O_000F00(2E,U,_,_,_), U                   , Enc(ExtRm)        , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdUd2              , "ud2"              , O_000F00(0B,U,_,_,_), U                   , Enc(X86Op)        , F(None)                            , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdUnpckhpd         , "unpckhpd"         , O_660F00(15,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdUnpckhps         , "unpckhps"         , O_000F00(15,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdUnpcklpd         , "unpcklpd"         , O_660F00(14,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdUnpcklps         , "unpcklps"         , O_000F00(14,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVaddpd           , "vaddpd"           , O_660F00(58,U,_,I,1), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVaddps           , "vaddps"           , O_000F00(58,U,_,I,0), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVaddsd           , "vaddsd"           , O_F20F00(58,U,0,I,1), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVaddss           , "vaddss"           , O_F30F00(58,U,0,I,0), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVaddsubpd        , "vaddsubpd"        , O_660F00(D0,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVaddsubps        , "vaddsubps"        , O_F20F00(D0,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVaesdec          , "vaesdec"          , O_660F38(DE,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVaesdeclast      , "vaesdeclast"      , O_660F38(DF,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVaesenc          , "vaesenc"          , O_660F38(DC,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVaesenclast      , "vaesenclast"      , O_660F38(DD,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVaesimc          , "vaesimc"          , O_660F38(DB,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVaeskeygenassist , "vaeskeygenassist" , O_660F3A(DF,U,_,_,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVandnpd          , "vandnpd"          , O_660F00(55,U,_,_,1), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVandnps          , "vandnps"          , O_000F00(55,U,_,_,0), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVandpd           , "vandpd"           , O_660F00(54,U,_,_,1), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVandps           , "vandps"           , O_000F00(54,U,_,_,0), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVblendpd         , "vblendpd"         , O_660F3A(0D,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVblendps         , "vblendps"         , O_660F3A(0C,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVblendvpd        , "vblendvpd"        , O_660F3A(4B,U,_,_,_), U                   , Enc(AvxRvmr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Xy)             , U                 ),
  INST(kX86InstIdVblendvps        , "vblendvps"        , O_660F3A(4A,U,_,_,_), U                   , Enc(AvxRvmr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Xy)             , U                 ),
  INST(kX86InstIdVbroadcastf128   , "vbroadcastf128"   , O_660F38(1A,U,L,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdVbroadcasti128   , "vbroadcasti128"   , O_660F38(5A,U,L,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdVbroadcastsd     , "vbroadcastsd"     , O_660F38(19,U,L,0,1), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVbroadcastss     , "vbroadcastss"     , O_660F38(18,U,_,0,0), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcmppd           , "vcmppd"           , O_660F00(C2,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVcmpps           , "vcmpps"           , O_000F00(C2,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVcmpsd           , "vcmpsd"           , O_F20F00(C2,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVcmpss           , "vcmpss"           , O_F30F00(C2,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVcomisd          , "vcomisd"          , O_660F00(2F,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcomiss          , "vcomiss"          , O_000F00(2F,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtdq2pd        , "vcvtdq2pd"        , O_F30F00(E6,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtdq2ps        , "vcvtdq2ps"        , O_000F00(5B,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtpd2dq        , "vcvtpd2dq"        , O_F20F00(E6,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtpd2ps        , "vcvtpd2ps"        , O_660F00(5A,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtph2ps        , "vcvtph2ps"        , O_660F38(13,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtps2dq        , "vcvtps2dq"        , O_660F00(5B,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtps2pd        , "vcvtps2pd"        , O_000F00(5A,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtps2ph        , "vcvtps2ph"        , O_660F3A(1D,U,_,_,_), U                   , Enc(AvxMri_P)     , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Xy)             , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVcvtsd2si        , "vcvtsd2si"        , O_F20F00(2D,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvtsd2ss        , "vcvtsd2ss"        , O_F20F00(5A,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVcvtsi2sd        , "vcvtsi2sd"        , O_F20F00(2A,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(GqdMem)         , U                 , U                 ),
  INST(kX86InstIdVcvtsi2ss        , "vcvtsi2ss"        , O_F30F00(2A,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(GqdMem)         , U                 , U                 ),
  INST(kX86InstIdVcvtss2sd        , "vcvtss2sd"        , O_F30F00(5A,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVcvtss2si        , "vcvtss2si"        , O_F20F00(2D,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvttpd2dq       , "vcvttpd2dq"       , O_660F00(E6,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVcvttps2dq       , "vcvttps2dq"       , O_F30F00(5B,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVcvttsd2si       , "vcvttsd2si"       , O_F20F00(2C,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVcvttss2si       , "vcvttss2si"       , O_F30F00(2C,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVdivpd           , "vdivpd"           , O_660F00(5E,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVdivps           , "vdivps"           , O_000F00(5E,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVdivsd           , "vdivsd"           , O_F20F00(5E,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVdivss           , "vdivss"           , O_F30F00(5E,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVdppd            , "vdppd"            , O_660F3A(41,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVdpps            , "vdpps"            , O_660F3A(40,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVextractf128     , "vextractf128"     , O_660F3A(19,U,L,_,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Ymm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVextracti128     , "vextracti128"     , O_660F3A(39,U,L,_,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Ymm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVextractps       , "vextractps"       , O_660F3A(17,U,_,_,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(GqdMem)         , O(Xmm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVfmadd132pd      , "vfmadd132pd"      , O_660F38(98,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmadd132ps      , "vfmadd132ps"      , O_660F38(98,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmadd132sd      , "vfmadd132sd"      , O_660F38(99,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmadd132ss      , "vfmadd132ss"      , O_660F38(99,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmadd213pd      , "vfmadd213pd"      , O_660F38(A8,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmadd213ps      , "vfmadd213ps"      , O_660F38(A8,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmadd213sd      , "vfmadd213sd"      , O_660F38(A9,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmadd213ss      , "vfmadd213ss"      , O_660F38(A9,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmadd231pd      , "vfmadd231pd"      , O_660F38(B8,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmadd231ps      , "vfmadd231ps"      , O_660F38(B8,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmadd231sd      , "vfmadd231sd"      , O_660F38(B9,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmadd231ss      , "vfmadd231ss"      , O_660F38(B9,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmaddpd         , "vfmaddpd"         , O_660F3A(69,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmaddps         , "vfmaddps"         , O_660F3A(68,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmaddsd         , "vfmaddsd"         , O_660F3A(6B,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfmaddss         , "vfmaddss"         , O_660F3A(6A,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfmaddsub132pd   , "vfmaddsub132pd"   , O_660F38(96,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmaddsub132ps   , "vfmaddsub132ps"   , O_660F38(96,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmaddsub213pd   , "vfmaddsub213pd"   , O_660F38(A6,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmaddsub213ps   , "vfmaddsub213ps"   , O_660F38(A6,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmaddsub231pd   , "vfmaddsub231pd"   , O_660F38(B6,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmaddsub231ps   , "vfmaddsub231ps"   , O_660F38(B6,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmaddsubpd      , "vfmaddsubpd"      , O_660F3A(5D,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmaddsubps      , "vfmaddsubps"      , O_660F3A(5C,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmsub132pd      , "vfmsub132pd"      , O_660F38(9A,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsub132ps      , "vfmsub132ps"      , O_660F38(9A,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsub132sd      , "vfmsub132sd"      , O_660F38(9B,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmsub132ss      , "vfmsub132ss"      , O_660F38(9B,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmsub213pd      , "vfmsub213pd"      , O_660F38(AA,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsub213ps      , "vfmsub213ps"      , O_660F38(AA,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsub213sd      , "vfmsub213sd"      , O_660F38(AB,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmsub213ss      , "vfmsub213ss"      , O_660F38(AB,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmsub231pd      , "vfmsub231pd"      , O_660F38(BA,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsub231ps      , "vfmsub231ps"      , O_660F38(BA,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsub231sd      , "vfmsub231sd"      , O_660F38(BB,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmsub231ss      , "vfmsub231ss"      , O_660F38(BB,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfmsubadd132pd   , "vfmsubadd132pd"   , O_660F38(97,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsubadd132ps   , "vfmsubadd132ps"   , O_660F38(97,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsubadd213pd   , "vfmsubadd213pd"   , O_660F38(A7,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsubadd213ps   , "vfmsubadd213ps"   , O_660F38(A7,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsubadd231pd   , "vfmsubadd231pd"   , O_660F38(B7,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsubadd231ps   , "vfmsubadd231ps"   , O_660F38(B7,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfmsubaddpd      , "vfmsubaddpd"      , O_660F3A(5F,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmsubaddps      , "vfmsubaddps"      , O_660F3A(5E,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmsubpd         , "vfmsubpd"         , O_660F3A(6D,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmsubps         , "vfmsubps"         , O_660F3A(6C,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfmsubsd         , "vfmsubsd"         , O_660F3A(6F,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfmsubss         , "vfmsubss"         , O_660F3A(6E,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfnmadd132pd     , "vfnmadd132pd"     , O_660F38(9C,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmadd132ps     , "vfnmadd132ps"     , O_660F38(9C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmadd132sd     , "vfnmadd132sd"     , O_660F38(9D,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmadd132ss     , "vfnmadd132ss"     , O_660F38(9D,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmadd213pd     , "vfnmadd213pd"     , O_660F38(AC,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmadd213ps     , "vfnmadd213ps"     , O_660F38(AC,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmadd213sd     , "vfnmadd213sd"     , O_660F38(AD,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmadd213ss     , "vfnmadd213ss"     , O_660F38(AD,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmadd231pd     , "vfnmadd231pd"     , O_660F38(BC,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmadd231ps     , "vfnmadd231ps"     , O_660F38(BC,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmadd231sd     , "vfnmadd231sd"     , O_660F38(BC,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmadd231ss     , "vfnmadd231ss"     , O_660F38(BC,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmaddpd        , "vfnmaddpd"        , O_660F3A(79,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfnmaddps        , "vfnmaddps"        , O_660F3A(78,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfnmaddsd        , "vfnmaddsd"        , O_660F3A(7B,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfnmaddss        , "vfnmaddss"        , O_660F3A(7A,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfnmsub132pd     , "vfnmsub132pd"     , O_660F38(9E,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmsub132ps     , "vfnmsub132ps"     , O_660F38(9E,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmsub132sd     , "vfnmsub132sd"     , O_660F38(9F,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmsub132ss     , "vfnmsub132ss"     , O_660F38(9F,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmsub213pd     , "vfnmsub213pd"     , O_660F38(AE,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmsub213ps     , "vfnmsub213ps"     , O_660F38(AE,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmsub213sd     , "vfnmsub213sd"     , O_660F38(AF,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmsub213ss     , "vfnmsub213ss"     , O_660F38(AF,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmsub231pd     , "vfnmsub231pd"     , O_660F38(BE,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmsub231ps     , "vfnmsub231ps"     , O_660F38(BE,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVfnmsub231sd     , "vfnmsub231sd"     , O_660F38(BF,U,_,W,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmsub231ss     , "vfnmsub231ss"     , O_660F38(BF,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVfnmsubpd        , "vfnmsubpd"        , O_660F3A(7D,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfnmsubps        , "vfnmsubps"        , O_660F3A(7C,U,_,_,_), U                   , Enc(Fma4_P)       , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVfnmsubsd        , "vfnmsubsd"        , O_660F3A(7F,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfnmsubss        , "vfnmsubss"        , O_660F3A(7E,U,_,_,_), U                   , Enc(Fma4)         , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVfrczpd          , "vfrczpd"          , O_00_M09(81,U,_,_,_), U                   , Enc(XopRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVfrczps          , "vfrczps"          , O_00_M09(80,U,_,_,_), U                   , Enc(XopRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVfrczsd          , "vfrczsd"          , O_00_M09(83,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVfrczss          , "vfrczss"          , O_00_M09(82,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVgatherdpd       , "vgatherdpd"       , O_660F38(92,U,_,W,_), U                   , Enc(AvxGather)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                 ),
  INST(kX86InstIdVgatherdps       , "vgatherdps"       , O_660F38(92,U,_,_,_), U                   , Enc(AvxGather)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                 ),
  INST(kX86InstIdVgatherqpd       , "vgatherqpd"       , O_660F38(93,U,_,W,_), U                   , Enc(AvxGather)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                 ),
  INST(kX86InstIdVgatherqps       , "vgatherqps"       , O_660F38(93,U,_,_,_), U                   , Enc(AvxGatherEx)  , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Mem)            , O(Xmm)            , U                 , U                 ),
  INST(kX86InstIdVhaddpd          , "vhaddpd"          , O_660F00(7C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVhaddps          , "vhaddps"          , O_F20F00(7C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVhsubpd          , "vhsubpd"          , O_660F00(7D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVhsubps          , "vhsubps"          , O_F20F00(7D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVinsertf128      , "vinsertf128"      , O_660F3A(18,U,L,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Ymm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVinserti128      , "vinserti128"      , O_660F3A(38,U,L,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Ymm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVinsertps        , "vinsertps"        , O_660F3A(21,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVlddqu           , "vlddqu"           , O_F20F00(F0,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdVldmxcsr         , "vldmxcsr"         , O_000F00(AE,2,_,_,_), U                   , Enc(AvxM)         , F(Avx)                             , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdVmaskmovdqu      , "vmaskmovdqu"      , O_660F00(F7,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , U                 , U                 , U                 ),
  INST(kX86InstIdVmaskmovpd       , "vmaskmovpd"       , O_660F38(2D,U,_,_,_), O_660F38(2F,U,_,_,_), Enc(AvxRvmMvr_P)  , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmaskmovps       , "vmaskmovps"       , O_660F38(2C,U,_,_,_), O_660F38(2E,U,_,_,_), Enc(AvxRvmMvr_P)  , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmaxpd           , "vmaxpd"           , O_660F00(5F,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmaxps           , "vmaxps"           , O_000F00(5F,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmaxsd           , "vmaxsd"           , O_F20F00(5F,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmaxss           , "vmaxss"           , O_F30F00(5F,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVminpd           , "vminpd"           , O_660F00(5D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVminps           , "vminps"           , O_000F00(5D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVminsd           , "vminsd"           , O_F20F00(5D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVminss           , "vminss"           , O_F30F00(5D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmovapd          , "vmovapd"          , O_660F00(28,U,_,_,_), O_660F00(29,U,_,_,_), Enc(AvxRmMr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovaps          , "vmovaps"          , O_000F00(28,U,_,_,_), O_000F00(29,U,_,_,_), Enc(AvxRmMr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovd            , "vmovd"            , O_660F00(6E,U,_,_,_), O_660F00(7E,U,_,_,_), Enc(AvxRmMr)      , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVmovddup         , "vmovddup"         , O_F20F00(12,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovdqa          , "vmovdqa"          , O_660F00(6F,U,_,_,_), O_660F00(7F,U,_,_,_), Enc(AvxRmMr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovdqu          , "vmovdqu"          , O_F30F00(6F,U,_,_,_), O_F30F00(7F,U,_,_,_), Enc(AvxRmMr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovhlps         , "vmovhlps"         , O_000F00(12,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(Xmm)            , U                 , U                 ),
  INST(kX86InstIdVmovhpd          , "vmovhpd"          , O_660F00(16,U,_,_,_), O_660F00(17,U,_,_,_), Enc(AvxRvmMr)     , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                 ),
  INST(kX86InstIdVmovhps          , "vmovhps"          , O_000F00(16,U,_,_,_), O_000F00(17,U,_,_,_), Enc(AvxRvmMr)     , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                 ),
  INST(kX86InstIdVmovlhps         , "vmovlhps"         , O_000F00(16,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(Xmm)            , U                 , U                 ),
  INST(kX86InstIdVmovlpd          , "vmovlpd"          , O_660F00(12,U,_,_,_), O_660F00(13,U,_,_,_), Enc(AvxRvmMr)     , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                 ),
  INST(kX86InstIdVmovlps          , "vmovlps"          , O_000F00(12,U,_,_,_), O_000F00(13,U,_,_,_), Enc(AvxRvmMr)     , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Xmm)            , O(Mem)            , U                 , U                 ),
  INST(kX86InstIdVmovmskpd        , "vmovmskpd"        , O_660F00(50,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(Xy)             , U                 , U                 , U                 ),
  INST(kX86InstIdVmovmskps        , "vmovmskps"        , O_000F00(50,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(Xy)             , U                 , U                 , U                 ),
  INST(kX86InstIdVmovntdq         , "vmovntdq"         , O_660F00(E7,U,_,_,_), U                   , Enc(AvxMr)        , F(Avx)                             , EF(________), 0 , 0 , O(Mem)            , O(Xy)             , U                 , U                 , U                 ),
  INST(kX86InstIdVmovntdqa        , "vmovntdqa"        , O_660F38(2A,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , U                 , U                 , U                 ),
  INST(kX86InstIdVmovntpd         , "vmovntpd"         , O_660F00(2B,U,_,_,_), U                   , Enc(AvxMr_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Mem)            , O(Xy)             , U                 , U                 , U                 ),
  INST(kX86InstIdVmovntps         , "vmovntps"         , O_000F00(2B,U,_,_,_), U                   , Enc(AvxMr_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Mem)            , O(Xy)             , U                 , U                 , U                 ),
  INST(kX86InstIdVmovq            , "vmovq"            , O_660F00(6E,U,_,W,_), O_660F00(7E,U,_,_,_), Enc(AvxRmMr)      , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVmovsd           , "vmovsd"           , O_F20F00(10,U,_,_,_), O_F20F00(11,U,_,_,_), Enc(AvxMovSsSd)   , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(XmmMem)         , O(Xmm)            , U                 , U                 ),
  INST(kX86InstIdVmovshdup        , "vmovshdup"        , O_F30F00(16,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovsldup        , "vmovsldup"        , O_F30F00(12,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovss           , "vmovss"           , O_F30F00(10,U,_,_,_), O_F30F00(11,U,_,_,_), Enc(AvxMovSsSd)   , F(Avx)                             , EF(________), 0 , 0 , O(XmmMem)         , O(Xmm)            , O(Xmm)            , U                 , U                 ),
  INST(kX86InstIdVmovupd          , "vmovupd"          , O_660F00(10,U,_,_,_), O_660F00(11,U,_,_,_), Enc(AvxRmMr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmovups          , "vmovups"          , O_000F00(10,U,_,_,_), O_000F00(11,U,_,_,_), Enc(AvxRmMr_P)    , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVmpsadbw         , "vmpsadbw"         , O_660F3A(42,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVmulpd           , "vmulpd"           , O_660F00(59,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmulps           , "vmulps"           , O_000F00(59,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmulsd           , "vmulsd"           , O_F20F00(59,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVmulss           , "vmulss"           , O_F30F00(59,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVorpd            , "vorpd"            , O_660F00(56,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVorps            , "vorps"            , O_000F00(56,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpabsb           , "vpabsb"           , O_660F38(1C,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpabsd           , "vpabsd"           , O_660F38(1E,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpabsw           , "vpabsw"           , O_660F38(1D,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpackssdw        , "vpackssdw"        , O_660F00(6B,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpacksswb        , "vpacksswb"        , O_660F00(63,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpackusdw        , "vpackusdw"        , O_660F38(2B,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpackuswb        , "vpackuswb"        , O_660F00(67,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddb           , "vpaddb"           , O_660F00(FC,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddd           , "vpaddd"           , O_660F00(FE,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddq           , "vpaddq"           , O_660F00(D4,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddsb          , "vpaddsb"          , O_660F00(EC,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddsw          , "vpaddsw"          , O_660F00(ED,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddusb         , "vpaddusb"         , O_660F00(DC,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddusw         , "vpaddusw"         , O_660F00(DD,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpaddw           , "vpaddw"           , O_660F00(FD,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpalignr         , "vpalignr"         , O_660F3A(0F,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVpand            , "vpand"            , O_660F00(DB,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpandn           , "vpandn"           , O_660F00(DF,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpavgb           , "vpavgb"           , O_660F00(E0,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpavgw           , "vpavgw"           , O_660F00(E3,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpblendd         , "vpblendd"         , O_660F3A(02,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVpblendvb        , "vpblendvb"        , O_660F3A(4C,U,_,_,_), U                   , Enc(AvxRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Xy)             , U                 ),
  INST(kX86InstIdVpblendw         , "vpblendw"         , O_660F3A(0E,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVpbroadcastb     , "vpbroadcastb"     , O_660F38(78,U,_,_,0), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVpbroadcastd     , "vpbroadcastd"     , O_660F38(58,U,_,_,0), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVpbroadcastq     , "vpbroadcastq"     , O_660F38(59,U,_,_,1), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVpbroadcastw     , "vpbroadcastw"     , O_660F38(79,U,_,_,0), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVpclmulqdq       , "vpclmulqdq"       , O_660F3A(44,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcmov           , "vpcmov"           , O_00_M08(A2,U,_,_,_), U                   , Enc(XopRvrmRvmr_P), F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVpcmpeqb         , "vpcmpeqb"         , O_660F00(74,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpeqd         , "vpcmpeqd"         , O_660F00(76,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpeqq         , "vpcmpeqq"         , O_660F38(29,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpeqw         , "vpcmpeqw"         , O_660F00(75,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpestri       , "vpcmpestri"       , O_660F3A(61,U,_,_,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpcmpestrm       , "vpcmpestrm"       , O_660F3A(60,U,_,_,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpcmpgtb         , "vpcmpgtb"         , O_660F00(64,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpgtd         , "vpcmpgtd"         , O_660F00(66,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpgtq         , "vpcmpgtq"         , O_660F38(37,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpgtw         , "vpcmpgtw"         , O_660F00(65,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpcmpistri       , "vpcmpistri"       , O_660F3A(63,U,_,_,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpcmpistrm       , "vpcmpistrm"       , O_660F3A(62,U,_,_,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpcomb           , "vpcomb"           , O_00_M08(CC,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomd           , "vpcomd"           , O_00_M08(CE,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomq           , "vpcomq"           , O_00_M08(CF,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomub          , "vpcomub"          , O_00_M08(EC,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomud          , "vpcomud"          , O_00_M08(EE,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomuq          , "vpcomuq"          , O_00_M08(EF,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomuw          , "vpcomuw"          , O_00_M08(ED,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpcomw           , "vpcomw"           , O_00_M08(CD,U,_,_,_), U                   , Enc(XopRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVperm2f128       , "vperm2f128"       , O_660F3A(06,U,L,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Ymm)            , O(YmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVperm2i128       , "vperm2i128"       , O_660F3A(46,U,L,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Ymm)            , O(YmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpermd           , "vpermd"           , O_660F38(36,U,L,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Ymm)            , O(YmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpermil2pd       , "vpermil2pd"       , O_66_M03(49,U,_,_,_), U                   , Enc(AvxRvrmRvmr_P), F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVpermil2ps       , "vpermil2ps"       , O_66_M03(48,U,_,_,_), U                   , Enc(AvxRvrmRvmr_P), F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(XyMem)          , U                 ),
  INST(kX86InstIdVpermilpd        , "vpermilpd"        , O_660F38(0D,U,_,_,_), O_660F3A(05,U,_,_,_), Enc(AvxRvmRmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpermilps        , "vpermilps"        , O_660F38(0C,U,_,_,_), O_660F3A(04,U,_,_,_), Enc(AvxRvmRmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpermpd          , "vpermpd"          , O_660F3A(01,U,L,W,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(YmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpermps          , "vpermps"          , O_660F38(16,U,L,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(Ymm)            , O(YmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpermq           , "vpermq"           , O_660F3A(00,U,L,W,_), U                   , Enc(AvxRmi)       , F(Avx)                             , EF(________), 0 , 0 , O(Ymm)            , O(YmmMem)         , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpextrb          , "vpextrb"          , O_660F3A(14,U,_,_,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(GqdwbMem)       , O(Xmm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpextrd          , "vpextrd"          , O_660F3A(16,U,_,_,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(GqdMem)         , O(Xmm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpextrq          , "vpextrq"          , O_660F3A(16,U,_,W,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(GqMem)          , O(Xmm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpextrw          , "vpextrw"          , O_660F3A(15,U,_,_,_), U                   , Enc(AvxMri)       , F(Avx)                             , EF(________), 0 , 0 , O(GqdwMem)        , O(Xmm)            , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpgatherdd       , "vpgatherdd"       , O_660F38(90,U,_,_,_), U                   , Enc(AvxGather)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                 ),
  INST(kX86InstIdVpgatherdq       , "vpgatherdq"       , O_660F38(90,U,_,W,_), U                   , Enc(AvxGather)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                 ),
  INST(kX86InstIdVpgatherqd       , "vpgatherqd"       , O_660F38(91,U,_,_,_), U                   , Enc(AvxGatherEx)  , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Mem)            , O(Xmm)            , U                 , U                 ),
  INST(kX86InstIdVpgatherqq       , "vpgatherqq"       , O_660F38(91,U,_,W,_), U                   , Enc(AvxGather)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Mem)            , O(Xy)             , U                 , U                 ),
  INST(kX86InstIdVphaddbd         , "vphaddbd"         , O_00_M09(C2,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddbq         , "vphaddbq"         , O_00_M09(C3,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddbw         , "vphaddbw"         , O_00_M09(C1,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddd          , "vphaddd"          , O_660F38(02,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVphadddq         , "vphadddq"         , O_00_M09(CB,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddsw         , "vphaddsw"         , O_660F38(03,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVphaddubd        , "vphaddubd"        , O_00_M09(D2,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddubq        , "vphaddubq"        , O_00_M09(D3,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddubw        , "vphaddubw"        , O_00_M09(D1,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddudq        , "vphaddudq"        , O_00_M09(DB,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphadduwd        , "vphadduwd"        , O_00_M09(D6,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphadduwq        , "vphadduwq"        , O_00_M09(D7,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddw          , "vphaddw"          , O_660F38(01,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVphaddwd         , "vphaddwd"         , O_00_M09(C6,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphaddwq         , "vphaddwq"         , O_00_M09(C7,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphminposuw      , "vphminposuw"      , O_660F38(41,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphsubbw         , "vphsubbw"         , O_00_M09(E1,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphsubd          , "vphsubd"          , O_660F38(06,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVphsubdq         , "vphsubdq"         , O_00_M09(E3,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVphsubsw         , "vphsubsw"         , O_660F38(07,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVphsubw          , "vphsubw"          , O_660F38(05,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVphsubwd         , "vphsubwd"         , O_00_M09(E2,U,_,_,_), U                   , Enc(XopRm)        , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVpinsrb          , "vpinsrb"          , O_660F3A(20,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(GqdwbMem)       , O(Imm)            , U                 ),
  INST(kX86InstIdVpinsrd          , "vpinsrd"          , O_660F3A(22,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(GqdMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVpinsrq          , "vpinsrq"          , O_660F3A(22,U,_,W,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(GqMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVpinsrw          , "vpinsrw"          , O_660F00(C4,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(GqdwMem)        , O(Imm)            , U                 ),
  INST(kX86InstIdVpmacsdd         , "vpmacsdd"         , O_00_M08(9E,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacsdqh        , "vpmacsdqh"        , O_00_M08(9F,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacsdql        , "vpmacsdql"        , O_00_M08(97,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacssdd        , "vpmacssdd"        , O_00_M08(8E,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacssdqh       , "vpmacssdqh"       , O_00_M08(8F,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacssdql       , "vpmacssdql"       , O_00_M08(87,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacsswd        , "vpmacsswd"        , O_00_M08(86,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacssww        , "vpmacssww"        , O_00_M08(85,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacswd         , "vpmacswd"         , O_00_M08(96,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmacsww         , "vpmacsww"         , O_00_M08(95,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmadcsswd       , "vpmadcsswd"       , O_00_M08(A6,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmadcswd        , "vpmadcswd"        , O_00_M08(B6,U,_,_,_), U                   , Enc(XopRvmr)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Xmm)            , U                 ),
  INST(kX86InstIdVpmaddubsw       , "vpmaddubsw"       , O_660F38(04,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaddwd         , "vpmaddwd"         , O_660F00(F5,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaskmovd       , "vpmaskmovd"       , O_660F38(8C,U,_,_,_), O_660F38(8E,U,_,_,_), Enc(AvxRvmMvr_P)  , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaskmovq       , "vpmaskmovq"       , O_660F38(8C,U,_,W,_), O_660F38(8E,U,_,_,_), Enc(AvxRvmMvr_P)  , F(Avx)                             , EF(________), 0 , 0 , O(XyMem)          , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaxsb          , "vpmaxsb"          , O_660F38(3C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaxsd          , "vpmaxsd"          , O_660F38(3D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaxsw          , "vpmaxsw"          , O_660F00(EE,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaxub          , "vpmaxub"          , O_660F00(DE,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaxud          , "vpmaxud"          , O_660F38(3F,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmaxuw          , "vpmaxuw"          , O_660F38(3E,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpminsb          , "vpminsb"          , O_660F38(38,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpminsd          , "vpminsd"          , O_660F38(39,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpminsw          , "vpminsw"          , O_660F00(EA,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpminub          , "vpminub"          , O_660F00(DA,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpminud          , "vpminud"          , O_660F38(3B,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpminuw          , "vpminuw"          , O_660F38(3A,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmovmskb        , "vpmovmskb"        , O_660F00(D7,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Gqd)            , O(Xy)             , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovsxbd        , "vpmovsxbd"        , O_660F38(21,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovsxbq        , "vpmovsxbq"        , O_660F38(22,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovsxbw        , "vpmovsxbw"        , O_660F38(20,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovsxdq        , "vpmovsxdq"        , O_660F38(25,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovsxwd        , "vpmovsxwd"        , O_660F38(23,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovsxwq        , "vpmovsxwq"        , O_660F38(24,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovzxbd        , "vpmovzxbd"        , O_660F38(31,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovzxbq        , "vpmovzxbq"        , O_660F38(32,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovzxbw        , "vpmovzxbw"        , O_660F38(30,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovzxdq        , "vpmovzxdq"        , O_660F38(35,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovzxwd        , "vpmovzxwd"        , O_660F38(33,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmovzxwq        , "vpmovzxwq"        , O_660F38(34,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpmuldq          , "vpmuldq"          , O_660F38(28,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmulhrsw        , "vpmulhrsw"        , O_660F38(0B,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmulhuw         , "vpmulhuw"         , O_660F00(E4,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmulhw          , "vpmulhw"          , O_660F00(E5,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmulld          , "vpmulld"          , O_660F38(40,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmullw          , "vpmullw"          , O_660F00(D5,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpmuludq         , "vpmuludq"         , O_660F00(F4,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpor             , "vpor"             , O_660F00(EB,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpperm           , "vpperm"           , O_00_M08(A3,U,_,_,_), U                   , Enc(XopRvrmRvmr)  , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 ),
  INST(kX86InstIdVprotb           , "vprotb"           , O_00_M09(90,U,_,_,_), O_00_M08(C0,U,_,_,_), Enc(XopRvmRmi)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                 ),
  INST(kX86InstIdVprotd           , "vprotd"           , O_00_M09(92,U,_,_,_), O_00_M08(C2,U,_,_,_), Enc(XopRvmRmi)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                 ),
  INST(kX86InstIdVprotq           , "vprotq"           , O_00_M09(93,U,_,_,_), O_00_M08(C3,U,_,_,_), Enc(XopRvmRmi)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                 ),
  INST(kX86InstIdVprotw           , "vprotw"           , O_00_M09(91,U,_,_,_), O_00_M08(C1,U,_,_,_), Enc(XopRvmRmi)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)|O(Imm)  , U                 , U                 ),
  INST(kX86InstIdVpsadbw          , "vpsadbw"          , O_660F00(F6,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpshab           , "vpshab"           , O_00_M09(98,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshad           , "vpshad"           , O_00_M09(9A,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshaq           , "vpshaq"           , O_00_M09(9B,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshaw           , "vpshaw"           , O_00_M09(99,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshlb           , "vpshlb"           , O_00_M09(94,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshld           , "vpshld"           , O_00_M09(96,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshlq           , "vpshlq"           , O_00_M09(97,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshlw           , "vpshlw"           , O_00_M09(95,U,_,_,_), U                   , Enc(XopRvmRmv)    , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVpshufb          , "vpshufb"          , O_660F38(00,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpshufd          , "vpshufd"          , O_660F00(70,U,_,_,_), U                   , Enc(AvxRmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpshufhw         , "vpshufhw"         , O_F30F00(70,U,_,_,_), U                   , Enc(AvxRmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpshuflw         , "vpshuflw"         , O_F20F00(70,U,_,_,_), U                   , Enc(AvxRmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpsignb          , "vpsignb"          , O_660F38(08,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsignd          , "vpsignd"          , O_660F38(0A,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsignw          , "vpsignw"          , O_660F38(09,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpslld           , "vpslld"           , O_660F00(F2,U,_,_,_), O_660F00(72,6,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpslldq          , "vpslldq"          , O_660F00(73,7,_,_,_), U                   , Enc(AvxVmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpsllq           , "vpsllq"           , O_660F00(F3,U,_,_,_), O_660F00(73,6,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsllvd          , "vpsllvd"          , O_660F38(47,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsllvq          , "vpsllvq"          , O_660F38(47,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsllw           , "vpsllw"           , O_660F00(F1,U,_,_,_), O_660F00(71,6,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsrad           , "vpsrad"           , O_660F00(E2,U,_,_,_), O_660F00(72,4,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsravd          , "vpsravd"          , O_660F38(46,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsraw           , "vpsraw"           , O_660F00(E1,U,_,_,_), O_660F00(71,4,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsrld           , "vpsrld"           , O_660F00(D2,U,_,_,_), O_660F00(72,2,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsrldq          , "vpsrldq"          , O_660F00(73,3,_,_,_), U                   , Enc(AvxVmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVpsrlq           , "vpsrlq"           , O_660F00(D3,U,_,_,_), O_660F00(73,2,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsrlvd          , "vpsrlvd"          , O_660F38(45,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsrlvq          , "vpsrlvq"          , O_660F38(45,U,_,W,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsrlw           , "vpsrlw"           , O_660F00(D1,U,_,_,_), O_660F00(71,2,_,_,_), Enc(AvxRvmVmi_P)  , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(XyMem)|O(Imm)   , U                 , U                 ),
  INST(kX86InstIdVpsubb           , "vpsubb"           , O_660F00(F8,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubd           , "vpsubd"           , O_660F00(FA,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubq           , "vpsubq"           , O_660F00(FB,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubsb          , "vpsubsb"          , O_660F00(E8,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubsw          , "vpsubsw"          , O_660F00(E9,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubusb         , "vpsubusb"         , O_660F00(D8,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubusw         , "vpsubusw"         , O_660F00(D9,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpsubw           , "vpsubw"           , O_660F00(F9,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVptest           , "vptest"           , O_660F38(17,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(WWWWWW__), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVpunpckhbw       , "vpunpckhbw"       , O_660F00(68,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpckhdq       , "vpunpckhdq"       , O_660F00(6A,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpckhqdq      , "vpunpckhqdq"      , O_660F00(6D,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpckhwd       , "vpunpckhwd"       , O_660F00(69,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpcklbw       , "vpunpcklbw"       , O_660F00(60,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpckldq       , "vpunpckldq"       , O_660F00(62,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpcklqdq      , "vpunpcklqdq"      , O_660F00(6C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpunpcklwd       , "vpunpcklwd"       , O_660F00(61,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVpxor            , "vpxor"            , O_660F00(EF,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVrcpps           , "vrcpps"           , O_000F00(53,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVrcpss           , "vrcpss"           , O_F30F00(53,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVroundpd         , "vroundpd"         , O_660F3A(09,U,_,_,_), U                   , Enc(AvxRmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVroundps         , "vroundps"         , O_660F3A(08,U,_,_,_), U                   , Enc(AvxRmi_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , O(Imm)            , U                 , U                 ),
  INST(kX86InstIdVroundsd         , "vroundsd"         , O_660F3A(0B,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVroundss         , "vroundss"         , O_660F3A(0A,U,_,_,_), U                   , Enc(AvxRvmi)      , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , O(Imm)            , U                 ),
  INST(kX86InstIdVrsqrtps         , "vrsqrtps"         , O_000F00(52,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVrsqrtss         , "vrsqrtss"         , O_F30F00(52,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVshufpd          , "vshufpd"          , O_660F00(C6,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVshufps          , "vshufps"          , O_000F00(C6,U,_,_,_), U                   , Enc(AvxRvmi_P)    , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , O(Imm)            , U                 ),
  INST(kX86InstIdVsqrtpd          , "vsqrtpd"          , O_660F00(51,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVsqrtps          , "vsqrtps"          , O_000F00(51,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVsqrtsd          , "vsqrtsd"          , O_F20F00(51,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVsqrtss          , "vsqrtss"          , O_F30F00(51,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVstmxcsr         , "vstmxcsr"         , O_000F00(AE,3,_,_,_), U                   , Enc(AvxM)         , F(Avx)                             , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdVsubpd           , "vsubpd"           , O_660F00(5C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVsubps           , "vsubps"           , O_000F00(5C,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVsubsd           , "vsubsd"           , O_F20F00(5C,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVsubss           , "vsubss"           , O_F30F00(5C,U,_,_,_), U                   , Enc(AvxRvm)       , F(Avx)                             , EF(________), 0 , 0 , O(Xmm)            , O(Xmm)            , O(XmmMem)         , U                 , U                 ),
  INST(kX86InstIdVtestpd          , "vtestpd"          , O_660F38(0F,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVtestps          , "vtestps"          , O_660F38(0E,U,_,_,_), U                   , Enc(AvxRm_P)      , F(Test)                            , EF(WWWWWW__), 0 , 0 , O(Xy)             , O(XyMem)          , U                 , U                 , U                 ),
  INST(kX86InstIdVucomisd         , "vucomisd"         , O_660F00(2E,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVucomiss         , "vucomiss"         , O_000F00(2E,U,_,_,_), U                   , Enc(AvxRm)        , F(Avx)                             , EF(WWWWWW__), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdVunpckhpd        , "vunpckhpd"        , O_660F00(15,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVunpckhps        , "vunpckhps"        , O_000F00(15,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVunpcklpd        , "vunpcklpd"        , O_660F00(14,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVunpcklps        , "vunpcklps"        , O_000F00(14,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVxorpd           , "vxorpd"           , O_660F00(57,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVxorps           , "vxorps"           , O_000F00(57,U,_,_,_), U                   , Enc(AvxRvm_P)     , F(Avx)                             , EF(________), 0 , 0 , O(Xy)             , O(Xy)             , O(XyMem)          , U                 , U                 ),
  INST(kX86InstIdVzeroall         , "vzeroall"         , O_000F00(77,U,L,_,_), U                   , Enc(AvxOp)        , F(Avx)                             , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdVzeroupper       , "vzeroupper"       , O_000F00(77,U,_,_,_), U                   , Enc(AvxOp)        , F(Avx)                             , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdWrfsbase         , "wrfsbase"         , O_F30F00(AE,2,_,_,_), U                   , Enc(X86Rm)        , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdWrgsbase         , "wrgsbase"         , O_F30F00(AE,3,_,_,_), U                   , Enc(X86Rm)        , F(None)                            , EF(________), 0 , 0 , O(Gqd)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXadd             , "xadd"             , O_000F00(C0,U,_,_,_), U                   , Enc(X86Xadd)      , F(Xchg)|F(Lock)                    , EF(WWWWWW__), 0 , 0 , O(GqdwbMem)       , O(Gqdwb)          , U                 , U                 , U                 ),
  INST(kX86InstIdXchg             , "xchg"             , O_000000(86,U,_,_,_), U                   , Enc(X86Xchg)      , F(Xchg)|F(Lock)                    , EF(________), 0 , 0 , O(GqdwbMem)       , O(Gqdwb)          , U                 , U                 , U                 ),
  INST(kX86InstIdXgetbv           , "xgetbv"           , O_000F01(D0,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXor              , "xor"              , O_000000(30,6,_,_,_), U                   , Enc(X86Arith)     , F(Lock)                            , EF(WWWUWW__), 0 , 0 , O(GqdwbMem)       , O(GqdwbMem)|O(Imm), U                 , U                 , U                 ),
  INST(kX86InstIdXorpd            , "xorpd"            , O_660F00(57,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdXorps            , "xorps"            , O_000F00(57,U,_,_,_), U                   , Enc(ExtRm)        , F(None)                            , EF(________), 0 , 0 , O(Xmm)            , O(XmmMem)         , U                 , U                 , U                 ),
  INST(kX86InstIdXrstor           , "xrstor"           , O_000F00(AE,5,_,_,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXrstor64         , "xrstor64"         , O_000F00(AE,5,_,W,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXsave            , "xsave"            , O_000F00(AE,4,_,_,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXsave64          , "xsave64"          , O_000F00(AE,4,_,W,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXsaveopt         , "xsaveopt"         , O_000F00(AE,6,_,_,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXsaveopt64       , "xsaveopt64"       , O_000F00(AE,6,_,W,_), U                   , Enc(X86M)         , F(None)|F(Special)                 , EF(________), 0 , 0 , O(Mem)            , U                 , U                 , U                 , U                 ),
  INST(kX86InstIdXsetbv           , "xsetbv"           , O_000F01(D1,U,_,_,_), U                   , Enc(X86Op)        , F(None)|F(Special)                 , EF(________), 0 , 0 , U                 , U                 , U                 , U                 , U                 )
};

#undef INST

#undef O_00_X
#undef O_9B_X

#undef O_66_M03
#undef O_00_M09
#undef O_00_M08

#undef O_F30F3A
#undef O_F30F38
#undef O_F30F00
#undef O_F30000
#undef O_F20F3A
#undef O_F20F38
#undef O_F20F00
#undef O_F20000
#undef O_9B0000
#undef O_660F3A
#undef O_660F38
#undef O_660F00
#undef O_660000
#undef O_000F3A
#undef O_000F38
#undef O_000F0F
#undef O_000F01
#undef O_000F00
#undef O_000000

#undef O
#undef EF
#undef A
#undef F
#undef Enc

#undef U

// ============================================================================
// [asmjit::X86Cond]
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
  kInstIdNone,  \
  kInstIdNone,  \
  kInstIdNone,  \
  kInstIdNone   \
}

const uint32_t _x86ReverseCond[20] = {
  /* kX86CondO  -> */ kX86CondO,
  /* kX86CondNO -> */ kX86CondNO,
  /* kX86CondB  -> */ kX86CondA,
  /* kX86CondAE -> */ kX86CondBE,
  /* kX86CondE  -> */ kX86CondE,
  /* kX86CondNE -> */ kX86CondNE,
  /* kX86CondBE -> */ kX86CondAE,
  /* kX86CondA  -> */ kX86CondB,
  /* kX86CondS  -> */ kX86CondS,
  /* kX86CondNS -> */ kX86CondNS,
  /* kX86CondPE -> */ kX86CondPE,
  /* kX86CondPO -> */ kX86CondPO,
  /* kX86CondL  -> */ kX86CondG,
  /* kX86CondGE -> */ kX86CondLE,
  /* kX86CondLE -> */ kX86CondGE,
  /* kX86CondG  -> */ kX86CondL,

  /* kX86CondFpuUnordered    -> */ kX86CondFpuUnordered,
  /* kX86CondFpuNotUnordered -> */ kX86CondFpuNotUnordered,

  0x12,
  0x13
};

const uint32_t _x86CondToCmovcc[20] = CC_TO_INST(kX86InstIdCmov);
const uint32_t _x86CondToJcc   [20] = CC_TO_INST(kX86InstIdJ   );
const uint32_t _x86CondToSetcc [20] = CC_TO_INST(kX86InstIdSet );

#undef CC_TO_INST

// ============================================================================
// [asmjit::X86Util]
// ============================================================================

#if !defined(ASMJIT_DISABLE_NAMES)
// Compare two instruction names.
//
// `a` is null terminated instruction name from `_x86InstName[]` table.
// `b` is non-null terminated instruction name passed to `getInstIdByName()`.
static ASMJIT_INLINE int X86Util_cmpInstName(const char* a, const char* b, size_t len) {
  for (size_t i = 0; i < len; i++) {
    int c = static_cast<int>(static_cast<uint8_t>(a[i])) -
            static_cast<int>(static_cast<uint8_t>(b[i])) ;
    if (c != 0)
      return c;
  }

  return static_cast<int>(a[len]);
}

uint32_t X86Util::getInstIdByName(const char* name, size_t len) {
  if (name == NULL)
    return kInstIdNone;

  if (len == kInvalidIndex)
    len = ::strlen(name);

  if (len == 0)
    return kInstIdNone;

  uint32_t prefix = name[0] - kX86InstAlphaIndexFirst;
  if (prefix > kX86InstAlphaIndexLast - kX86InstAlphaIndexFirst)
    return kInstIdNone;

  uint32_t index = _x86InstAlphaIndex[prefix];
  if (index == kX86InstAlphaIndexInvalid)
    return kInstIdNone;

  const X86InstInfo* base = _x86InstInfo + index;
  const X86InstInfo* end = _x86InstInfo + _kX86InstIdCount;

  // Handle instructions starting with 'j' specially. `jcc` instruction breaks
  // the sorting, because of the suffixes (it's considered as one instruction),
  // so basically `jecxz` and `jmp` are stored after all `jcc` instructions.
  bool linearSearch = prefix == ('j' - kX86InstAlphaIndexFirst);

  while (++prefix <= kX86InstAlphaIndexLast - kX86InstAlphaIndexFirst) {
    index = _x86InstAlphaIndex[prefix];
    if (index == kX86InstAlphaIndexInvalid)
      continue;
    end = _x86InstInfo + index;
    break;
  }

  if (linearSearch) {
    while (base != end) {
      if (X86Util_cmpInstName(base->getInstName(), name, len) == 0)
        return static_cast<uint32_t>((size_t)(base - _x86InstInfo));
      base++;
    }
  }
  else {
    for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
      const X86InstInfo* cur = base + (lim >> 1);
      int result = X86Util_cmpInstName(cur->getInstName(), name, len);

      if (result < 0) {
        base = cur + 1;
        lim--;
        continue;
      }

      if (result > 0)
        continue;

      return static_cast<uint32_t>((size_t)(cur - _x86InstInfo));
    }
  }

  return kInstIdNone;
}
#endif // ASMJIT_DISABLE_NAMES

// ============================================================================
// [asmjit::X86Util - Test]
// ============================================================================

#if defined(ASMJIT_TEST) && !defined(ASMJIT_DISABLE_NAMES)
UNIT(x86_inst_name) {
  // All known instructions should be matched.
  INFO("Matching all X86/X64 instructions.");
  for (uint32_t a = 0; a < _kX86InstIdCount; a++) {
    uint32_t b = X86Util::getInstIdByName(_x86InstInfo[a].getInstName());

    EXPECT(a == b,
      "Should match existing instruction \"%s\" {id:%u} != \"%s\" {id:%u}.",
        _x86InstInfo[a].getInstName(), a,
        _x86InstInfo[b].getInstName(), b);
  }

  // Everything else should return kInstIdNone
  INFO("Trying to look-up instructions that don't exist.");
  EXPECT(X86Util::getInstIdByName(NULL) == kInstIdNone,
    "Should return kInstIdNone for NULL input.");

  EXPECT(X86Util::getInstIdByName("") == kInstIdNone,
    "Should return kInstIdNone for empty string.");

  EXPECT(X86Util::getInstIdByName("_") == kInstIdNone,
    "Should return kInstIdNone for unknown instruction.");

  EXPECT(X86Util::getInstIdByName("123xyz") == kInstIdNone,
    "Should return kInstIdNone for unknown instruction.");
}
#endif // ASMJIT_TEST && !ASMJIT_DISABLE_NAMES

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64
