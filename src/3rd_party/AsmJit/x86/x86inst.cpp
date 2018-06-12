// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// ----------------------------------------------------------------------------
// IMPORTANT: AsmJit now uses an external instruction database to populate
// static tables within this file. Perform the following steps to regenerate
// all tables enclosed by ${...}:
//
//   1. Install node.js environment <https://nodejs.org>
//   2. Go to asmjit/tools directory
//   3. Install either asmdb package by executing `npm install asmdb` or get
//      the latest asmdb from <https://github.com/asmjit/asmdb> and copy/link
//      the `asmdb` directory to `asmjit/tools/asmdb`.
//   4. Execute `node generate-x86.js`
//
// Instruction encoding and opcodes were added to the `x86inst.cpp` database
// manually in the past and they are not updated by the script as they seem
// consistent. However, everything else is updated including instruction
// operands and tables required to validate them, instruction read/write
// information (including registers and flags), and all indexes to all tables.
// ----------------------------------------------------------------------------

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/cpuinfo.h"
#include "../base/utils.h"
#include "../x86/x86inst.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Enums (Internal)]
// ============================================================================

//! \internal
enum ODATA_ {
  // PREFIX.
  ODATA_000000  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_00,
  ODATA_000F00  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F,
  ODATA_000F01  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F01,
  ODATA_000F38  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F38,
  ODATA_000F3A  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F3A,
  ODATA_660000  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_00,
  ODATA_660F00  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F,
  ODATA_660F38  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F38,
  ODATA_660F3A  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F3A,
  ODATA_F20000  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_00,
  ODATA_F20F00  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F,
  ODATA_F20F38  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F38,
  ODATA_F20F3A  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F3A,
  ODATA_F30000  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_00,
  ODATA_F30F00  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F,
  ODATA_F30F38  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F38,
  ODATA_F30F3A  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F3A,
  ODATA_000F0F  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F, // 3DNOW, special case.

  ODATA_FPU_00  = X86Inst::kOpCode_PP_00,
  ODATA_FPU_9B  = X86Inst::kOpCode_PP_9B,

  ODATA_XOP_M8  = X86Inst::kOpCode_MM_XOP08,
  ODATA_XOP_M9  = X86Inst::kOpCode_MM_XOP09,

  ODATA_O__     = 0,
  ODATA_O_0     = 0 << X86Inst::kOpCode_O_Shift,
  ODATA_O_1     = 1 << X86Inst::kOpCode_O_Shift,
  ODATA_O_2     = 2 << X86Inst::kOpCode_O_Shift,
  ODATA_O_3     = 3 << X86Inst::kOpCode_O_Shift,
  ODATA_O_4     = 4 << X86Inst::kOpCode_O_Shift,
  ODATA_O_5     = 5 << X86Inst::kOpCode_O_Shift,
  ODATA_O_6     = 6 << X86Inst::kOpCode_O_Shift,
  ODATA_O_7     = 7 << X86Inst::kOpCode_O_Shift,

  // REX/VEX.
  ODATA_LL__    = 0,                                  // L is unspecified.
  ODATA_LL_x    = 0,                                  // L is based on operand(s).
  ODATA_LL_I    = 0,                                  // L is ignored (LIG).
  ODATA_LL_0    = 0,                                  // L has to be zero (L.128).
  ODATA_LL_1    = X86Inst::kOpCode_LL_256,            // L has to be one (L.256).
  ODATA_LL_2    = X86Inst::kOpCode_LL_512,            // L has to be two (L.512).

  ODATA_W__     = 0,                                  // W is unspecified.
  ODATA_W_x     = 0,                                  // W is based on operand(s).
  ODATA_W_I     = 0,                                  // W is ignored (WIG).
  ODATA_W_0     = 0,                                  // W has to be zero (W0).
  ODATA_W_1     = X86Inst::kOpCode_W,                 // W has to be one (W1).

  // EVEX.
  ODATA_EvexW__ = 0,                                  // Not EVEX instruction.
  ODATA_EvexW_x = 0,                                  // EVEX.W is based on operand(s).
  ODATA_EvexW_I = 0,                                  // EVEX.W is ignored     (EVEX.WIG).
  ODATA_EvexW_0 = 0,                                  // EVEX.W has to be zero (EVEX.W0).
  ODATA_EvexW_1 = X86Inst::kOpCode_EW,                // EVEX.W has to be one  (EVEX.W1).

  ODATA_N__      = 0,                                 // Base element size not used.
  ODATA_N_0      = 0 << X86Inst::kOpCode_CDSHL_Shift, // N << 0 (BYTE).
  ODATA_N_1      = 1 << X86Inst::kOpCode_CDSHL_Shift, // N << 1 (WORD).
  ODATA_N_2      = 2 << X86Inst::kOpCode_CDSHL_Shift, // N << 2 (DWORD).
  ODATA_N_3      = 3 << X86Inst::kOpCode_CDSHL_Shift, // N << 3 (QWORD).
  ODATA_N_4      = 4 << X86Inst::kOpCode_CDSHL_Shift, // N << 4 (OWORD).
  ODATA_N_5      = 5 << X86Inst::kOpCode_CDSHL_Shift, // N << 5 (YWORD).

  ODATA_TT__     = 0,
  ODATA_TT_FV    = X86Inst::kOpCode_CDTT_FV,
  ODATA_TT_HV    = X86Inst::kOpCode_CDTT_HV,
  ODATA_TT_FVM   = X86Inst::kOpCode_CDTT_FVM,
  ODATA_TT_T1S   = X86Inst::kOpCode_CDTT_T1S,
  ODATA_TT_T1F   = X86Inst::kOpCode_CDTT_T1F,
  ODATA_TT_T1W   = X86Inst::kOpCode_CDTT_T1W,
  ODATA_TT_T2    = X86Inst::kOpCode_CDTT_T2,
  ODATA_TT_T4    = X86Inst::kOpCode_CDTT_T4,
  ODATA_TT_T8    = X86Inst::kOpCode_CDTT_T8,
  ODATA_TT_HVM   = X86Inst::kOpCode_CDTT_HVM,
  ODATA_TT_OVM   = X86Inst::kOpCode_CDTT_OVM,
  ODATA_TT_QVM   = X86Inst::kOpCode_CDTT_QVM,
  ODATA_TT_128   = X86Inst::kOpCode_CDTT_128,
  ODATA_TT_DUP   = X86Inst::kOpCode_CDTT_DUP,
  ODATA_TT_T4X   = X86Inst::kOpCode_CDTT_T1_4X
};

// ============================================================================
// [asmjit::X86Inst]
// ============================================================================

// Instruction opcode definitions:
//   - `O` encodes X86|MMX|SSE instructions.
//   - `V` encodes VEX|XOP|EVEX instructions.
#define O_ENCODE(VEX, PREFIX, OPCODE, O, L, W, EvexW, N, TT) \
  ((PREFIX) | (OPCODE) | (O) | (L) | (W) | (EvexW) | (N) | (TT) | \
   (VEX && ((PREFIX) & X86Inst::kOpCode_MM_Mask) != X86Inst::kOpCode_MM_0F ? int(X86Inst::kOpCode_MM_ForceVex3) : 0))

#define O(PREFIX, OPCODE, O, LL, W, EvexW, N, TT) (O_ENCODE(0, ODATA_##PREFIX, 0x##OPCODE, ODATA_O_##O, ODATA_LL_##LL, ODATA_W_##W, ODATA_EvexW_##EvexW, ODATA_N_##N, ODATA_TT_##TT))
#define V(PREFIX, OPCODE, O, LL, W, EvexW, N, TT) (O_ENCODE(1, ODATA_##PREFIX, 0x##OPCODE, ODATA_O_##O, ODATA_LL_##LL, ODATA_W_##W, ODATA_EvexW_##EvexW, ODATA_N_##N, ODATA_TT_##TT))

#define O_FPU(PREFIX, OPCODE, O) (ODATA_FPU_##PREFIX | (0x##OPCODE & 0xFFU) | ((0x##OPCODE >> 8) << X86Inst::kOpCode_FPU_2B_Shift) | ODATA_O_##O)

// Don't store `_nameDataIndex` if instruction names are disabled. Since some
// APIs can use `_nameDataIndex` it's much safer if it's zero if it's not used.
#if defined(ASMJIT_DISABLE_TEXT)
# define NAME_DATA_INDEX(X) 0
#else
# define NAME_DATA_INDEX(X) X
#endif

// Defines an X86/X64 instruction.
#define INST(id, encoding, opcode0, opcode1, writeIndex, writeSize, nameDataIndex, commonDataIndex, operationDataIndex, seeToAvxDataIndex) { \
  uint32_t(X86Inst::kEncoding##encoding),   \
  uint32_t(NAME_DATA_INDEX(nameDataIndex)), \
  uint32_t(commonDataIndex),                \
  uint32_t(operationDataIndex),             \
  uint32_t(seeToAvxDataIndex),              \
  0,                                        \
  opcode0                                   \
}
const X86Inst X86InstDB::instData[] = {
  // <-----------------+--------------------+------------------+--------+------------------+--------+-------+-----+----+----+---+
  //                   |                    |    Main OpCode   |#0 EVEX |Alternative OpCode|#1 EVEX | Write |     |    |    |Sse|
  //    Instruction    |   Inst. Encoding   |                  +--------+                  +--------+---+---+NameX|ComX|OpnX|<->|
  //                   |                    |#0:PP-MMM OP/O L|W|W|N|TT. |#1:PP-MMM OP/O L|W|W|N|TT. |Idx|Cnt|     |    |    |Avx|
  // <-----------------+--------------------+------------------+--------+------------------+--------+---+---+-----+----+----+---+
  // ${instData:Begin}
  INST(None            , None               , 0                         , 0                         , 0 , 0 , 0   , 0  , 0  , 0 ),
  INST(Aaa             , X86Op_xAX          , O(000000,37,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1   , 1  , 1  , 0 ),
  INST(Aad             , X86I_xAX           , O(000000,D5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5   , 2  , 1  , 0 ),
  INST(Aam             , X86I_xAX           , O(000000,D4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9   , 2  , 1  , 0 ),
  INST(Aas             , X86Op_xAX          , O(000000,3F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 13  , 1  , 1  , 0 ),
  INST(Adc             , X86Arith           , O(000000,10,2,_,x,_,_,_  ), 0                         , 0 , 0 , 17  , 3  , 2  , 0 ),
  INST(Adcx            , X86Rm              , O(660F38,F6,_,_,x,_,_,_  ), 0                         , 0 , 0 , 21  , 4  , 3  , 0 ),
  INST(Add             , X86Arith           , O(000000,00,0,_,x,_,_,_  ), 0                         , 0 , 0 , 732 , 3  , 1  , 0 ),
  INST(Addpd           , ExtRm              , O(660F00,58,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4619, 5  , 4  , 1 ),
  INST(Addps           , ExtRm              , O(000F00,58,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4631, 5  , 5  , 1 ),
  INST(Addsd           , ExtRm              , O(F20F00,58,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4853, 6  , 4  , 1 ),
  INST(Addss           , ExtRm              , O(F30F00,58,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4863, 7  , 5  , 1 ),
  INST(Addsubpd        , ExtRm              , O(660F00,D0,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4358, 5  , 6  , 1 ),
  INST(Addsubps        , ExtRm              , O(F20F00,D0,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4370, 5  , 6  , 1 ),
  INST(Adox            , X86Rm              , O(F30F38,F6,_,_,x,_,_,_  ), 0                         , 0 , 0 , 26  , 4  , 7  , 0 ),
  INST(Aesdec          , ExtRm              , O(660F38,DE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2854, 5  , 8  , 2 ),
  INST(Aesdeclast      , ExtRm              , O(660F38,DF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2862, 5  , 8  , 2 ),
  INST(Aesenc          , ExtRm              , O(660F38,DC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2874, 5  , 8  , 2 ),
  INST(Aesenclast      , ExtRm              , O(660F38,DD,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2882, 5  , 8  , 2 ),
  INST(Aesimc          , ExtRm              , O(660F38,DB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2894, 8  , 8  , 3 ),
  INST(Aeskeygenassist , ExtRmi             , O(660F3A,DF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2902, 9  , 8  , 3 ),
  INST(And             , X86Arith           , O(000000,20,4,_,x,_,_,_  ), 0                         , 0 , 0 , 2317, 10 , 1  , 0 ),
  INST(Andn            , VexRvm_Wx          , V(000F38,F2,_,0,x,_,_,_  ), 0                         , 0 , 0 , 6150, 11 , 9  , 0 ),
  INST(Andnpd          , ExtRm              , O(660F00,55,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2935, 5  , 4  , 2 ),
  INST(Andnps          , ExtRm              , O(000F00,55,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2943, 5  , 5  , 2 ),
  INST(Andpd           , ExtRm              , O(660F00,54,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3872, 12 , 4  , 2 ),
  INST(Andps           , ExtRm              , O(000F00,54,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3882, 12 , 5  , 2 ),
  INST(Arpl            , X86Mr_NoSize       , O(000000,63,_,_,_,_,_,_  ), 0                         , 0 , 0 , 31  , 13 , 10 , 0 ),
  INST(Bextr           , VexRmv_Wx          , V(000F38,F7,_,0,x,_,_,_  ), 0                         , 0 , 0 , 36  , 14 , 9  , 0 ),
  INST(Blcfill         , VexVm_Wx           , V(XOP_M9,01,1,0,x,_,_,_  ), 0                         , 0 , 0 , 42  , 15 , 11 , 0 ),
  INST(Blci            , VexVm_Wx           , V(XOP_M9,02,6,0,x,_,_,_  ), 0                         , 0 , 0 , 50  , 15 , 11 , 0 ),
  INST(Blcic           , VexVm_Wx           , V(XOP_M9,01,5,0,x,_,_,_  ), 0                         , 0 , 0 , 55  , 15 , 11 , 0 ),
  INST(Blcmsk          , VexVm_Wx           , V(XOP_M9,02,1,0,x,_,_,_  ), 0                         , 0 , 0 , 61  , 15 , 11 , 0 ),
  INST(Blcs            , VexVm_Wx           , V(XOP_M9,01,3,0,x,_,_,_  ), 0                         , 0 , 0 , 68  , 15 , 11 , 0 ),
  INST(Blendpd         , ExtRmi             , O(660F3A,0D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3021, 16 , 12 , 4 ),
  INST(Blendps         , ExtRmi             , O(660F3A,0C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3030, 16 , 12 , 4 ),
  INST(Blendvpd        , ExtRm_XMM0         , O(660F38,15,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3039, 17 , 12 , 5 ),
  INST(Blendvps        , ExtRm_XMM0         , O(660F38,14,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3049, 17 , 12 , 5 ),
  INST(Blsfill         , VexVm_Wx           , V(XOP_M9,01,2,0,x,_,_,_  ), 0                         , 0 , 0 , 73  , 15 , 11 , 0 ),
  INST(Blsi            , VexVm_Wx           , V(000F38,F3,3,0,x,_,_,_  ), 0                         , 0 , 0 , 81  , 15 , 9  , 0 ),
  INST(Blsic           , VexVm_Wx           , V(XOP_M9,01,6,0,x,_,_,_  ), 0                         , 0 , 0 , 86  , 15 , 11 , 0 ),
  INST(Blsmsk          , VexVm_Wx           , V(000F38,F3,2,0,x,_,_,_  ), 0                         , 0 , 0 , 92  , 15 , 9  , 0 ),
  INST(Blsr            , VexVm_Wx           , V(000F38,F3,1,0,x,_,_,_  ), 0                         , 0 , 0 , 99  , 15 , 9  , 0 ),
  INST(Bndcl           , X86Rm              , O(F30F00,1A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 104 , 18 , 13 , 0 ),
  INST(Bndcn           , X86Rm              , O(F20F00,1B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 110 , 18 , 13 , 0 ),
  INST(Bndcu           , X86Rm              , O(F20F00,1A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 116 , 18 , 13 , 0 ),
  INST(Bndldx          , X86Rm              , O(000F00,1A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 122 , 19 , 13 , 0 ),
  INST(Bndmk           , X86Rm              , O(F30F00,1B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 129 , 20 , 13 , 0 ),
  INST(Bndmov          , X86Bndmov          , O(660F00,1A,_,_,_,_,_,_  ), O(660F00,1B,_,_,_,_,_,_  ), 0 , 0 , 135 , 21 , 13 , 0 ),
  INST(Bndstx          , X86Mr              , O(000F00,1B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 142 , 22 , 13 , 0 ),
  INST(Bound           , X86Rm              , O(000000,62,_,_,_,_,_,_  ), 0                         , 0 , 0 , 149 , 23 , 0  , 0 ),
  INST(Bsf             , X86Rm              , O(000F00,BC,_,_,x,_,_,_  ), 0                         , 0 , 0 , 155 , 24 , 1  , 0 ),
  INST(Bsr             , X86Rm              , O(000F00,BD,_,_,x,_,_,_  ), 0                         , 0 , 0 , 159 , 24 , 1  , 0 ),
  INST(Bswap           , X86Bswap           , O(000F00,C8,_,_,x,_,_,_  ), 0                         , 0 , 0 , 163 , 25 , 0  , 0 ),
  INST(Bt              , X86Bt              , O(000F00,A3,_,_,x,_,_,_  ), O(000F00,BA,4,_,x,_,_,_  ), 0 , 0 , 169 , 26 , 14 , 0 ),
  INST(Btc             , X86Bt              , O(000F00,BB,_,_,x,_,_,_  ), O(000F00,BA,7,_,x,_,_,_  ), 0 , 0 , 172 , 27 , 14 , 0 ),
  INST(Btr             , X86Bt              , O(000F00,B3,_,_,x,_,_,_  ), O(000F00,BA,6,_,x,_,_,_  ), 0 , 0 , 176 , 28 , 14 , 0 ),
  INST(Bts             , X86Bt              , O(000F00,AB,_,_,x,_,_,_  ), O(000F00,BA,5,_,x,_,_,_  ), 0 , 0 , 180 , 29 , 14 , 0 ),
  INST(Bzhi            , VexRmv_Wx          , V(000F38,F5,_,0,x,_,_,_  ), 0                         , 0 , 0 , 184 , 14 , 15 , 0 ),
  INST(Call            , X86Call            , O(000000,FF,2,_,_,_,_,_  ), 0                         , 0 , 0 , 2713, 30 , 16 , 0 ),
  INST(Cbw             , X86Op_xAX          , O(660000,98,_,_,_,_,_,_  ), 0                         , 0 , 0 , 189 , 31 , 0  , 0 ),
  INST(Cdq             , X86Op_xDX_xAX      , O(000000,99,_,_,_,_,_,_  ), 0                         , 0 , 0 , 193 , 32 , 0  , 0 ),
  INST(Cdqe            , X86Op_xAX          , O(000000,98,_,_,1,_,_,_  ), 0                         , 0 , 0 , 197 , 33 , 0  , 0 ),
  INST(Clac            , X86Op              , O(000F01,CA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 202 , 34 , 17 , 0 ),
  INST(Clc             , X86Op              , O(000000,F8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 207 , 34 , 18 , 0 ),
  INST(Cld             , X86Op              , O(000000,FC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 211 , 34 , 19 , 0 ),
  INST(Clflush         , X86M_Only          , O(000F00,AE,7,_,_,_,_,_  ), 0                         , 0 , 0 , 215 , 35 , 20 , 0 ),
  INST(Clflushopt      , X86M_Only          , O(660F00,AE,7,_,_,_,_,_  ), 0                         , 0 , 0 , 223 , 35 , 21 , 0 ),
  INST(Cli             , X86Op              , O(000000,FA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 234 , 34 , 22 , 0 ),
  INST(Clts            , X86Op              , O(000F00,06,_,_,_,_,_,_  ), 0                         , 0 , 0 , 238 , 34 , 23 , 0 ),
  INST(Clwb            , X86M_Only          , O(660F00,AE,6,_,_,_,_,_  ), 0                         , 0 , 0 , 243 , 35 , 24 , 0 ),
  INST(Clzero          , X86Op_ZAX          , O(000F01,FC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 248 , 36 , 25 , 0 ),
  INST(Cmc             , X86Op              , O(000000,F5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 255 , 34 , 26 , 0 ),
  INST(Cmova           , X86Rm              , O(000F00,47,_,_,x,_,_,_  ), 0                         , 0 , 0 , 259 , 24 , 27 , 0 ),
  INST(Cmovae          , X86Rm              , O(000F00,43,_,_,x,_,_,_  ), 0                         , 0 , 0 , 265 , 24 , 28 , 0 ),
  INST(Cmovb           , X86Rm              , O(000F00,42,_,_,x,_,_,_  ), 0                         , 0 , 0 , 589 , 24 , 28 , 0 ),
  INST(Cmovbe          , X86Rm              , O(000F00,46,_,_,x,_,_,_  ), 0                         , 0 , 0 , 596 , 24 , 27 , 0 ),
  INST(Cmovc           , X86Rm              , O(000F00,42,_,_,x,_,_,_  ), 0                         , 0 , 0 , 272 , 24 , 28 , 0 ),
  INST(Cmove           , X86Rm              , O(000F00,44,_,_,x,_,_,_  ), 0                         , 0 , 0 , 604 , 24 , 29 , 0 ),
  INST(Cmovg           , X86Rm              , O(000F00,4F,_,_,x,_,_,_  ), 0                         , 0 , 0 , 278 , 24 , 30 , 0 ),
  INST(Cmovge          , X86Rm              , O(000F00,4D,_,_,x,_,_,_  ), 0                         , 0 , 0 , 284 , 24 , 31 , 0 ),
  INST(Cmovl           , X86Rm              , O(000F00,4C,_,_,x,_,_,_  ), 0                         , 0 , 0 , 291 , 24 , 31 , 0 ),
  INST(Cmovle          , X86Rm              , O(000F00,4E,_,_,x,_,_,_  ), 0                         , 0 , 0 , 297 , 24 , 30 , 0 ),
  INST(Cmovna          , X86Rm              , O(000F00,46,_,_,x,_,_,_  ), 0                         , 0 , 0 , 304 , 24 , 27 , 0 ),
  INST(Cmovnae         , X86Rm              , O(000F00,42,_,_,x,_,_,_  ), 0                         , 0 , 0 , 311 , 24 , 28 , 0 ),
  INST(Cmovnb          , X86Rm              , O(000F00,43,_,_,x,_,_,_  ), 0                         , 0 , 0 , 611 , 24 , 28 , 0 ),
  INST(Cmovnbe         , X86Rm              , O(000F00,47,_,_,x,_,_,_  ), 0                         , 0 , 0 , 619 , 24 , 27 , 0 ),
  INST(Cmovnc          , X86Rm              , O(000F00,43,_,_,x,_,_,_  ), 0                         , 0 , 0 , 319 , 24 , 28 , 0 ),
  INST(Cmovne          , X86Rm              , O(000F00,45,_,_,x,_,_,_  ), 0                         , 0 , 0 , 628 , 24 , 29 , 0 ),
  INST(Cmovng          , X86Rm              , O(000F00,4E,_,_,x,_,_,_  ), 0                         , 0 , 0 , 326 , 24 , 30 , 0 ),
  INST(Cmovnge         , X86Rm              , O(000F00,4C,_,_,x,_,_,_  ), 0                         , 0 , 0 , 333 , 24 , 31 , 0 ),
  INST(Cmovnl          , X86Rm              , O(000F00,4D,_,_,x,_,_,_  ), 0                         , 0 , 0 , 341 , 24 , 31 , 0 ),
  INST(Cmovnle         , X86Rm              , O(000F00,4F,_,_,x,_,_,_  ), 0                         , 0 , 0 , 348 , 24 , 30 , 0 ),
  INST(Cmovno          , X86Rm              , O(000F00,41,_,_,x,_,_,_  ), 0                         , 0 , 0 , 356 , 24 , 32 , 0 ),
  INST(Cmovnp          , X86Rm              , O(000F00,4B,_,_,x,_,_,_  ), 0                         , 0 , 0 , 363 , 24 , 33 , 0 ),
  INST(Cmovns          , X86Rm              , O(000F00,49,_,_,x,_,_,_  ), 0                         , 0 , 0 , 370 , 24 , 34 , 0 ),
  INST(Cmovnz          , X86Rm              , O(000F00,45,_,_,x,_,_,_  ), 0                         , 0 , 0 , 377 , 24 , 29 , 0 ),
  INST(Cmovo           , X86Rm              , O(000F00,40,_,_,x,_,_,_  ), 0                         , 0 , 0 , 384 , 24 , 32 , 0 ),
  INST(Cmovp           , X86Rm              , O(000F00,4A,_,_,x,_,_,_  ), 0                         , 0 , 0 , 390 , 24 , 33 , 0 ),
  INST(Cmovpe          , X86Rm              , O(000F00,4A,_,_,x,_,_,_  ), 0                         , 0 , 0 , 396 , 24 , 33 , 0 ),
  INST(Cmovpo          , X86Rm              , O(000F00,4B,_,_,x,_,_,_  ), 0                         , 0 , 0 , 403 , 24 , 33 , 0 ),
  INST(Cmovs           , X86Rm              , O(000F00,48,_,_,x,_,_,_  ), 0                         , 0 , 0 , 410 , 24 , 34 , 0 ),
  INST(Cmovz           , X86Rm              , O(000F00,44,_,_,x,_,_,_  ), 0                         , 0 , 0 , 416 , 24 , 29 , 0 ),
  INST(Cmp             , X86Arith           , O(000000,38,7,_,x,_,_,_  ), 0                         , 0 , 0 , 422 , 37 , 1  , 0 ),
  INST(Cmppd           , ExtRmi             , O(660F00,C2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3275, 16 , 4  , 6 ),
  INST(Cmpps           , ExtRmi             , O(000F00,C2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3282, 16 , 5  , 6 ),
  INST(Cmps            , X86StrMm           , O(000000,A6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 426 , 38 , 35 , 0 ),
  INST(Cmpsd           , ExtRmi             , O(F20F00,C2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3289, 39 , 4  , 7 ),
  INST(Cmpss           , ExtRmi             , O(F30F00,C2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3296, 40 , 5  , 7 ),
  INST(Cmpxchg         , X86Cmpxchg         , O(000F00,B0,_,_,x,_,_,_  ), 0                         , 0 , 0 , 431 , 41 , 36 , 0 ),
  INST(Cmpxchg16b      , X86M_Only          , O(000F00,C7,1,_,1,_,_,_  ), 0                         , 0 , 0 , 439 , 42 , 37 , 0 ),
  INST(Cmpxchg8b       , X86M_Only          , O(000F00,C7,1,_,_,_,_,_  ), 0                         , 0 , 0 , 450 , 43 , 38 , 0 ),
  INST(Comisd          , ExtRm              , O(660F00,2F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9371, 44 , 39 , 8 ),
  INST(Comiss          , ExtRm              , O(000F00,2F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9380, 45 , 40 , 8 ),
  INST(Cpuid           , X86Op              , O(000F00,A2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 460 , 46 , 41 , 0 ),
  INST(Cqo             , X86Op_xDX_xAX      , O(000000,99,_,_,1,_,_,_  ), 0                         , 0 , 0 , 466 , 47 , 0  , 0 ),
  INST(Crc32           , X86Crc             , O(F20F38,F0,_,_,x,_,_,_  ), 0                         , 0 , 0 , 470 , 48 , 42 , 0 ),
  INST(Cvtdq2pd        , ExtRm              , O(F30F00,E6,_,_,_,_,_,_  ), 0                         , 0 , 16, 3343, 49 , 4  , 9 ),
  INST(Cvtdq2ps        , ExtRm              , O(000F00,5B,_,_,_,_,_,_  ), 0                         , 0 , 16, 3353, 50 , 4  , 9 ),
  INST(Cvtpd2dq        , ExtRm              , O(F20F00,E6,_,_,_,_,_,_  ), 0                         , 0 , 16, 3363, 50 , 4  , 9 ),
  INST(Cvtpd2pi        , ExtRm              , O(660F00,2D,_,_,_,_,_,_  ), 0                         , 0 , 8 , 476 , 51 , 4  , 0 ),
  INST(Cvtpd2ps        , ExtRm              , O(660F00,5A,_,_,_,_,_,_  ), 0                         , 0 , 16, 3373, 50 , 4  , 10),
  INST(Cvtpi2pd        , ExtRm              , O(660F00,2A,_,_,_,_,_,_  ), 0                         , 0 , 16, 485 , 52 , 4  , 0 ),
  INST(Cvtpi2ps        , ExtRm              , O(000F00,2A,_,_,_,_,_,_  ), 0                         , 0 , 8 , 494 , 53 , 5  , 0 ),
  INST(Cvtps2dq        , ExtRm              , O(660F00,5B,_,_,_,_,_,_  ), 0                         , 0 , 16, 3425, 50 , 4  , 8 ),
  INST(Cvtps2pd        , ExtRm              , O(000F00,5A,_,_,_,_,_,_  ), 0                         , 0 , 16, 3435, 49 , 4  , 8 ),
  INST(Cvtps2pi        , ExtRm              , O(000F00,2D,_,_,_,_,_,_  ), 0                         , 0 , 8 , 503 , 54 , 5  , 0 ),
  INST(Cvtsd2si        , ExtRm_Wx           , O(F20F00,2D,_,_,x,_,_,_  ), 0                         , 0 , 8 , 3507, 55 , 4  , 11),
  INST(Cvtsd2ss        , ExtRm              , O(F20F00,5A,_,_,_,_,_,_  ), 0                         , 0 , 4 , 3517, 56 , 4  , 12),
  INST(Cvtsi2sd        , ExtRm_Wx           , O(F20F00,2A,_,_,x,_,_,_  ), 0                         , 0 , 8 , 3538, 57 , 4  , 13),
  INST(Cvtsi2ss        , ExtRm_Wx           , O(F30F00,2A,_,_,x,_,_,_  ), 0                         , 0 , 4 , 3548, 58 , 5  , 13),
  INST(Cvtss2sd        , ExtRm              , O(F30F00,5A,_,_,_,_,_,_  ), 0                         , 0 , 8 , 3558, 59 , 4  , 13),
  INST(Cvtss2si        , ExtRm_Wx           , O(F30F00,2D,_,_,x,_,_,_  ), 0                         , 0 , 8 , 3568, 60 , 5  , 14),
  INST(Cvttpd2dq       , ExtRm              , O(660F00,E6,_,_,_,_,_,_  ), 0                         , 0 , 16, 3589, 50 , 4  , 15),
  INST(Cvttpd2pi       , ExtRm              , O(660F00,2C,_,_,_,_,_,_  ), 0                         , 0 , 8 , 512 , 51 , 4  , 0 ),
  INST(Cvttps2dq       , ExtRm              , O(F30F00,5B,_,_,_,_,_,_  ), 0                         , 0 , 16, 3635, 50 , 4  , 16),
  INST(Cvttps2pi       , ExtRm              , O(000F00,2C,_,_,_,_,_,_  ), 0                         , 0 , 8 , 522 , 54 , 5  , 0 ),
  INST(Cvttsd2si       , ExtRm_Wx           , O(F20F00,2C,_,_,x,_,_,_  ), 0                         , 0 , 8 , 3681, 55 , 4  , 17),
  INST(Cvttss2si       , ExtRm_Wx           , O(F30F00,2C,_,_,x,_,_,_  ), 0                         , 0 , 8 , 3704, 60 , 5  , 18),
  INST(Cwd             , X86Op_xDX_xAX      , O(660000,99,_,_,_,_,_,_  ), 0                         , 0 , 0 , 532 , 61 , 0  , 0 ),
  INST(Cwde            , X86Op_xAX          , O(000000,98,_,_,_,_,_,_  ), 0                         , 0 , 0 , 536 , 62 , 0  , 0 ),
  INST(Daa             , X86Op              , O(000000,27,_,_,_,_,_,_  ), 0                         , 0 , 0 , 541 , 1  , 1  , 0 ),
  INST(Das             , X86Op              , O(000000,2F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 545 , 1  , 1  , 0 ),
  INST(Dec             , X86IncDec          , O(000000,FE,1,_,x,_,_,_  ), O(000000,48,_,_,x,_,_,_  ), 0 , 0 , 2857, 63 , 43 , 0 ),
  INST(Div             , X86M_GPB_MulDiv    , O(000000,F6,6,_,x,_,_,_  ), 0                         , 0 , 0 , 751 , 64 , 1  , 0 ),
  INST(Divpd           , ExtRm              , O(660F00,5E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3803, 5  , 4  , 19),
  INST(Divps           , ExtRm              , O(000F00,5E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3810, 5  , 5  , 19),
  INST(Divsd           , ExtRm              , O(F20F00,5E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3817, 6  , 4  , 19),
  INST(Divss           , ExtRm              , O(F30F00,5E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3824, 7  , 5  , 19),
  INST(Dppd            , ExtRmi             , O(660F3A,41,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3831, 16 , 12 , 19),
  INST(Dpps            , ExtRmi             , O(660F3A,40,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3837, 16 , 12 , 19),
  INST(Emms            , X86Op              , O(000F00,77,_,_,_,_,_,_  ), 0                         , 0 , 0 , 719 , 65 , 44 , 0 ),
  INST(Enter           , X86Enter           , O(000000,C8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2721, 66 , 45 , 0 ),
  INST(Extractps       , ExtExtract         , O(660F3A,17,_,_,_,_,_,_  ), 0                         , 0 , 8 , 4027, 67 , 12 , 20),
  INST(Extrq           , ExtExtrq           , O(660F00,79,_,_,_,_,_,_  ), O(660F00,78,0,_,_,_,_,_  ), 0 , 0 , 6864, 68 , 46 , 0 ),
  INST(F2xm1           , FpuOp              , O_FPU(00,D9F0,_)          , 0                         , 0 , 0 , 549 , 34 , 47 , 0 ),
  INST(Fabs            , FpuOp              , O_FPU(00,D9E1,_)          , 0                         , 0 , 0 , 555 , 34 , 47 , 0 ),
  INST(Fadd            , FpuArith           , O_FPU(00,C0C0,0)          , 0                         , 0 , 0 , 1957, 69 , 47 , 0 ),
  INST(Faddp           , FpuRDef            , O_FPU(00,DEC0,_)          , 0                         , 0 , 0 , 560 , 70 , 47 , 0 ),
  INST(Fbld            , X86M_Only          , O_FPU(00,00DF,4)          , 0                         , 0 , 0 , 566 , 71 , 47 , 0 ),
  INST(Fbstp           , X86M_Only          , O_FPU(00,00DF,6)          , 0                         , 0 , 0 , 571 , 72 , 47 , 0 ),
  INST(Fchs            , FpuOp              , O_FPU(00,D9E0,_)          , 0                         , 0 , 0 , 577 , 34 , 47 , 0 ),
  INST(Fclex           , FpuOp              , O_FPU(9B,DBE2,_)          , 0                         , 0 , 0 , 582 , 34 , 47 , 0 ),
  INST(Fcmovb          , FpuR               , O_FPU(00,DAC0,_)          , 0                         , 0 , 0 , 588 , 73 , 48 , 0 ),
  INST(Fcmovbe         , FpuR               , O_FPU(00,DAD0,_)          , 0                         , 0 , 0 , 595 , 73 , 48 , 0 ),
  INST(Fcmove          , FpuR               , O_FPU(00,DAC8,_)          , 0                         , 0 , 0 , 603 , 73 , 48 , 0 ),
  INST(Fcmovnb         , FpuR               , O_FPU(00,DBC0,_)          , 0                         , 0 , 0 , 610 , 73 , 48 , 0 ),
  INST(Fcmovnbe        , FpuR               , O_FPU(00,DBD0,_)          , 0                         , 0 , 0 , 618 , 73 , 48 , 0 ),
  INST(Fcmovne         , FpuR               , O_FPU(00,DBC8,_)          , 0                         , 0 , 0 , 627 , 73 , 48 , 0 ),
  INST(Fcmovnu         , FpuR               , O_FPU(00,DBD8,_)          , 0                         , 0 , 0 , 635 , 73 , 48 , 0 ),
  INST(Fcmovu          , FpuR               , O_FPU(00,DAD8,_)          , 0                         , 0 , 0 , 643 , 73 , 48 , 0 ),
  INST(Fcom            , FpuCom             , O_FPU(00,D0D0,2)          , 0                         , 0 , 0 , 650 , 74 , 47 , 0 ),
  INST(Fcomi           , FpuR               , O_FPU(00,DBF0,_)          , 0                         , 0 , 0 , 655 , 75 , 49 , 0 ),
  INST(Fcomip          , FpuR               , O_FPU(00,DFF0,_)          , 0                         , 0 , 0 , 661 , 75 , 49 , 0 ),
  INST(Fcomp           , FpuCom             , O_FPU(00,D8D8,3)          , 0                         , 0 , 0 , 668 , 74 , 47 , 0 ),
  INST(Fcompp          , FpuOp              , O_FPU(00,DED9,_)          , 0                         , 0 , 0 , 674 , 34 , 47 , 0 ),
  INST(Fcos            , FpuOp              , O_FPU(00,D9FF,_)          , 0                         , 0 , 0 , 681 , 34 , 47 , 0 ),
  INST(Fdecstp         , FpuOp              , O_FPU(00,D9F6,_)          , 0                         , 0 , 0 , 686 , 34 , 47 , 0 ),
  INST(Fdiv            , FpuArith           , O_FPU(00,F0F8,6)          , 0                         , 0 , 0 , 694 , 69 , 47 , 0 ),
  INST(Fdivp           , FpuRDef            , O_FPU(00,DEF8,_)          , 0                         , 0 , 0 , 699 , 70 , 47 , 0 ),
  INST(Fdivr           , FpuArith           , O_FPU(00,F8F0,7)          , 0                         , 0 , 0 , 705 , 69 , 47 , 0 ),
  INST(Fdivrp          , FpuRDef            , O_FPU(00,DEF0,_)          , 0                         , 0 , 0 , 711 , 70 , 47 , 0 ),
  INST(Femms           , X86Op              , O(000F00,0E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 718 , 34 , 50 , 0 ),
  INST(Ffree           , FpuR               , O_FPU(00,DDC0,_)          , 0                         , 0 , 0 , 724 , 73 , 47 , 0 ),
  INST(Fiadd           , FpuM               , O_FPU(00,00DA,0)          , 0                         , 0 , 0 , 730 , 76 , 47 , 0 ),
  INST(Ficom           , FpuM               , O_FPU(00,00DA,2)          , 0                         , 0 , 0 , 736 , 76 , 47 , 0 ),
  INST(Ficomp          , FpuM               , O_FPU(00,00DA,3)          , 0                         , 0 , 0 , 742 , 76 , 47 , 0 ),
  INST(Fidiv           , FpuM               , O_FPU(00,00DA,6)          , 0                         , 0 , 0 , 749 , 76 , 47 , 0 ),
  INST(Fidivr          , FpuM               , O_FPU(00,00DA,7)          , 0                         , 0 , 0 , 755 , 76 , 47 , 0 ),
  INST(Fild            , FpuM               , O_FPU(00,00DB,0)          , O_FPU(00,00DF,5)          , 0 , 0 , 762 , 77 , 47 , 0 ),
  INST(Fimul           , FpuM               , O_FPU(00,00DA,1)          , 0                         , 0 , 0 , 767 , 76 , 47 , 0 ),
  INST(Fincstp         , FpuOp              , O_FPU(00,D9F7,_)          , 0                         , 0 , 0 , 773 , 34 , 47 , 0 ),
  INST(Finit           , FpuOp              , O_FPU(9B,DBE3,_)          , 0                         , 0 , 0 , 781 , 34 , 47 , 0 ),
  INST(Fist            , FpuM               , O_FPU(00,00DB,2)          , 0                         , 0 , 0 , 787 , 78 , 47 , 0 ),
  INST(Fistp           , FpuM               , O_FPU(00,00DB,3)          , O_FPU(00,00DF,7)          , 0 , 0 , 792 , 79 , 47 , 0 ),
  INST(Fisttp          , FpuM               , O_FPU(00,00DB,1)          , O_FPU(00,00DD,1)          , 0 , 0 , 798 , 80 , 51 , 0 ),
  INST(Fisub           , FpuM               , O_FPU(00,00DA,4)          , 0                         , 0 , 0 , 805 , 76 , 47 , 0 ),
  INST(Fisubr          , FpuM               , O_FPU(00,00DA,5)          , 0                         , 0 , 0 , 811 , 76 , 47 , 0 ),
  INST(Fld             , FpuFldFst          , O_FPU(00,00D9,0)          , O_FPU(00,00DB,5)          , 0 , 0 , 818 , 81 , 47 , 0 ),
  INST(Fld1            , FpuOp              , O_FPU(00,D9E8,_)          , 0                         , 0 , 0 , 822 , 34 , 47 , 0 ),
  INST(Fldcw           , X86M_Only          , O_FPU(00,00D9,5)          , 0                         , 0 , 0 , 827 , 82 , 47 , 0 ),
  INST(Fldenv          , X86M_Only          , O_FPU(00,00D9,4)          , 0                         , 0 , 0 , 833 , 35 , 47 , 0 ),
  INST(Fldl2e          , FpuOp              , O_FPU(00,D9EA,_)          , 0                         , 0 , 0 , 840 , 34 , 47 , 0 ),
  INST(Fldl2t          , FpuOp              , O_FPU(00,D9E9,_)          , 0                         , 0 , 0 , 847 , 34 , 47 , 0 ),
  INST(Fldlg2          , FpuOp              , O_FPU(00,D9EC,_)          , 0                         , 0 , 0 , 854 , 34 , 47 , 0 ),
  INST(Fldln2          , FpuOp              , O_FPU(00,D9ED,_)          , 0                         , 0 , 0 , 861 , 34 , 47 , 0 ),
  INST(Fldpi           , FpuOp              , O_FPU(00,D9EB,_)          , 0                         , 0 , 0 , 868 , 34 , 47 , 0 ),
  INST(Fldz            , FpuOp              , O_FPU(00,D9EE,_)          , 0                         , 0 , 0 , 874 , 34 , 47 , 0 ),
  INST(Fmul            , FpuArith           , O_FPU(00,C8C8,1)          , 0                         , 0 , 0 , 1999, 83 , 47 , 0 ),
  INST(Fmulp           , FpuRDef            , O_FPU(00,DEC8,_)          , 0                         , 0 , 0 , 879 , 70 , 47 , 0 ),
  INST(Fnclex          , FpuOp              , O_FPU(00,DBE2,_)          , 0                         , 0 , 0 , 885 , 34 , 47 , 0 ),
  INST(Fninit          , FpuOp              , O_FPU(00,DBE3,_)          , 0                         , 0 , 0 , 892 , 34 , 47 , 0 ),
  INST(Fnop            , FpuOp              , O_FPU(00,D9D0,_)          , 0                         , 0 , 0 , 899 , 34 , 47 , 0 ),
  INST(Fnsave          , X86M_Only          , O_FPU(00,00DD,6)          , 0                         , 0 , 0 , 904 , 84 , 47 , 0 ),
  INST(Fnstcw          , X86M_Only          , O_FPU(00,00D9,7)          , 0                         , 0 , 0 , 911 , 85 , 47 , 0 ),
  INST(Fnstenv         , X86M_Only          , O_FPU(00,00D9,6)          , 0                         , 0 , 0 , 918 , 84 , 47 , 0 ),
  INST(Fnstsw          , FpuStsw            , O_FPU(00,00DD,7)          , O_FPU(00,DFE0,_)          , 0 , 0 , 926 , 86 , 47 , 0 ),
  INST(Fpatan          , FpuOp              , O_FPU(00,D9F3,_)          , 0                         , 0 , 0 , 933 , 34 , 47 , 0 ),
  INST(Fprem           , FpuOp              , O_FPU(00,D9F8,_)          , 0                         , 0 , 0 , 940 , 34 , 47 , 0 ),
  INST(Fprem1          , FpuOp              , O_FPU(00,D9F5,_)          , 0                         , 0 , 0 , 946 , 34 , 47 , 0 ),
  INST(Fptan           , FpuOp              , O_FPU(00,D9F2,_)          , 0                         , 0 , 0 , 953 , 34 , 47 , 0 ),
  INST(Frndint         , FpuOp              , O_FPU(00,D9FC,_)          , 0                         , 0 , 0 , 959 , 34 , 47 , 0 ),
  INST(Frstor          , X86M_Only          , O_FPU(00,00DD,4)          , 0                         , 0 , 0 , 967 , 35 , 47 , 0 ),
  INST(Fsave           , X86M_Only          , O_FPU(9B,00DD,6)          , 0                         , 0 , 0 , 974 , 84 , 47 , 0 ),
  INST(Fscale          , FpuOp              , O_FPU(00,D9FD,_)          , 0                         , 0 , 0 , 980 , 34 , 47 , 0 ),
  INST(Fsin            , FpuOp              , O_FPU(00,D9FE,_)          , 0                         , 0 , 0 , 987 , 34 , 47 , 0 ),
  INST(Fsincos         , FpuOp              , O_FPU(00,D9FB,_)          , 0                         , 0 , 0 , 992 , 34 , 47 , 0 ),
  INST(Fsqrt           , FpuOp              , O_FPU(00,D9FA,_)          , 0                         , 0 , 0 , 1000, 34 , 47 , 0 ),
  INST(Fst             , FpuFldFst          , O_FPU(00,00D9,2)          , 0                         , 0 , 0 , 1006, 87 , 47 , 0 ),
  INST(Fstcw           , X86M_Only          , O_FPU(9B,00D9,7)          , 0                         , 0 , 0 , 1010, 85 , 47 , 0 ),
  INST(Fstenv          , X86M_Only          , O_FPU(9B,00D9,6)          , 0                         , 0 , 0 , 1016, 84 , 47 , 0 ),
  INST(Fstp            , FpuFldFst          , O_FPU(00,00D9,3)          , O(000000,DB,7,_,_,_,_,_  ), 0 , 0 , 1023, 88 , 47 , 0 ),
  INST(Fstsw           , FpuStsw            , O_FPU(9B,00DD,7)          , O_FPU(9B,DFE0,_)          , 0 , 0 , 1028, 89 , 47 , 0 ),
  INST(Fsub            , FpuArith           , O_FPU(00,E0E8,4)          , 0                         , 0 , 0 , 2077, 69 , 47 , 0 ),
  INST(Fsubp           , FpuRDef            , O_FPU(00,DEE8,_)          , 0                         , 0 , 0 , 1034, 70 , 47 , 0 ),
  INST(Fsubr           , FpuArith           , O_FPU(00,E8E0,5)          , 0                         , 0 , 0 , 2083, 69 , 47 , 0 ),
  INST(Fsubrp          , FpuRDef            , O_FPU(00,DEE0,_)          , 0                         , 0 , 0 , 1040, 70 , 47 , 0 ),
  INST(Ftst            , FpuOp              , O_FPU(00,D9E4,_)          , 0                         , 0 , 0 , 1047, 34 , 47 , 0 ),
  INST(Fucom           , FpuRDef            , O_FPU(00,DDE0,_)          , 0                         , 0 , 0 , 1052, 90 , 47 , 0 ),
  INST(Fucomi          , FpuR               , O_FPU(00,DBE8,_)          , 0                         , 0 , 0 , 1058, 75 , 49 , 0 ),
  INST(Fucomip         , FpuR               , O_FPU(00,DFE8,_)          , 0                         , 0 , 0 , 1065, 75 , 49 , 0 ),
  INST(Fucomp          , FpuRDef            , O_FPU(00,DDE8,_)          , 0                         , 0 , 0 , 1073, 90 , 47 , 0 ),
  INST(Fucompp         , FpuOp              , O_FPU(00,DAE9,_)          , 0                         , 0 , 0 , 1080, 34 , 47 , 0 ),
  INST(Fwait           , X86Op              , O_FPU(00,00DB,_)          , 0                         , 0 , 0 , 1088, 34 , 47 , 0 ),
  INST(Fxam            , FpuOp              , O_FPU(00,D9E5,_)          , 0                         , 0 , 0 , 1094, 34 , 47 , 0 ),
  INST(Fxch            , FpuR               , O_FPU(00,D9C8,_)          , 0                         , 0 , 0 , 1099, 70 , 47 , 0 ),
  INST(Fxrstor         , X86M_Only          , O(000F00,AE,1,_,_,_,_,_  ), 0                         , 0 , 0 , 1104, 35 , 52 , 0 ),
  INST(Fxrstor64       , X86M_Only          , O(000F00,AE,1,_,1,_,_,_  ), 0                         , 0 , 0 , 1112, 91 , 52 , 0 ),
  INST(Fxsave          , X86M_Only          , O(000F00,AE,0,_,_,_,_,_  ), 0                         , 0 , 0 , 1122, 84 , 53 , 0 ),
  INST(Fxsave64        , X86M_Only          , O(000F00,AE,0,_,1,_,_,_  ), 0                         , 0 , 0 , 1129, 92 , 53 , 0 ),
  INST(Fxtract         , FpuOp              , O_FPU(00,D9F4,_)          , 0                         , 0 , 0 , 1138, 34 , 47 , 0 ),
  INST(Fyl2x           , FpuOp              , O_FPU(00,D9F1,_)          , 0                         , 0 , 0 , 1146, 34 , 47 , 0 ),
  INST(Fyl2xp1         , FpuOp              , O_FPU(00,D9F9,_)          , 0                         , 0 , 0 , 1152, 34 , 47 , 0 ),
  INST(Haddpd          , ExtRm              , O(660F00,7C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5382, 5  , 6  , 21),
  INST(Haddps          , ExtRm              , O(F20F00,7C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5390, 5  , 6  , 21),
  INST(Hlt             , X86Op              , O(000000,F4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1160, 34 , 23 , 0 ),
  INST(Hsubpd          , ExtRm              , O(660F00,7D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5398, 5  , 6  , 22),
  INST(Hsubps          , ExtRm              , O(F20F00,7D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5406, 5  , 6  , 22),
  INST(Idiv            , X86M_GPB_MulDiv    , O(000000,F6,7,_,x,_,_,_  ), 0                         , 0 , 0 , 750 , 64 , 1  , 0 ),
  INST(Imul            , X86Imul            , O(000000,F6,5,_,x,_,_,_  ), 0                         , 0 , 0 , 768 , 93 , 1  , 0 ),
  INST(In              , X86In              , O(000000,EC,_,_,_,_,_,_  ), O(000000,E4,_,_,_,_,_,_  ), 0 , 0 , 9508, 94 , 45 , 0 ),
  INST(Inc             , X86IncDec          , O(000000,FE,0,_,x,_,_,_  ), O(000000,40,_,_,x,_,_,_  ), 0 , 0 , 1164, 95 , 43 , 0 ),
  INST(Ins             , X86Ins             , O(000000,6C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1168, 96 , 45 , 0 ),
  INST(Insertps        , ExtRmi             , O(660F3A,21,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5542, 40 , 12 , 23),
  INST(Insertq         , ExtInsertq         , O(F20F00,79,_,_,_,_,_,_  ), O(F20F00,78,_,_,_,_,_,_  ), 0 , 0 , 1172, 97 , 46 , 0 ),
  INST(Int             , X86Int             , O(000000,CD,_,_,_,_,_,_  ), 0                         , 0 , 0 , 963 , 98 , 45 , 0 ),
  INST(Int3            , X86Op              , O(000000,CC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1180, 34 , 45 , 0 ),
  INST(Into            , X86Op              , O(000000,CE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1185, 99 , 54 , 0 ),
  INST(Invd            , X86Op              , O(000F00,08,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9463, 34 , 55 , 0 ),
  INST(Invlpg          , X86M_Only          , O(000F00,01,7,_,_,_,_,_  ), 0                         , 0 , 0 , 1190, 35 , 55 , 0 ),
  INST(Invpcid         , X86Rm_NoRexW       , O(660F38,82,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1197, 100, 55 , 0 ),
  INST(Iret            , X86Op              , O(000000,CF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1205, 34 , 16 , 0 ),
  INST(Iretd           , X86Op              , O(000000,CF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1210, 34 , 16 , 0 ),
  INST(Iretq           , X86Op              , O(000000,CF,_,_,1,_,_,_  ), 0                         , 0 , 0 , 1216, 101, 16 , 0 ),
  INST(Iretw           , X86Op              , O(660000,CF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1222, 34 , 16 , 0 ),
  INST(Ja              , X86Jcc             , O(000F00,87,_,_,_,_,_,_  ), O(000000,77,_,_,_,_,_,_  ), 0 , 0 , 1228, 102, 56 , 0 ),
  INST(Jae             , X86Jcc             , O(000F00,83,_,_,_,_,_,_  ), O(000000,73,_,_,_,_,_,_  ), 0 , 0 , 1231, 103, 57 , 0 ),
  INST(Jb              , X86Jcc             , O(000F00,82,_,_,_,_,_,_  ), O(000000,72,_,_,_,_,_,_  ), 0 , 0 , 1235, 104, 57 , 0 ),
  INST(Jbe             , X86Jcc             , O(000F00,86,_,_,_,_,_,_  ), O(000000,76,_,_,_,_,_,_  ), 0 , 0 , 1238, 105, 56 , 0 ),
  INST(Jc              , X86Jcc             , O(000F00,82,_,_,_,_,_,_  ), O(000000,72,_,_,_,_,_,_  ), 0 , 0 , 1242, 106, 57 , 0 ),
  INST(Je              , X86Jcc             , O(000F00,84,_,_,_,_,_,_  ), O(000000,74,_,_,_,_,_,_  ), 0 , 0 , 1245, 107, 58 , 0 ),
  INST(Jecxz           , X86JecxzLoop       , 0                         , O(000000,E3,_,_,_,_,_,_  ), 0 , 0 , 1248, 108, 45 , 0 ),
  INST(Jg              , X86Jcc             , O(000F00,8F,_,_,_,_,_,_  ), O(000000,7F,_,_,_,_,_,_  ), 0 , 0 , 1254, 109, 59 , 0 ),
  INST(Jge             , X86Jcc             , O(000F00,8D,_,_,_,_,_,_  ), O(000000,7D,_,_,_,_,_,_  ), 0 , 0 , 1257, 110, 60 , 0 ),
  INST(Jl              , X86Jcc             , O(000F00,8C,_,_,_,_,_,_  ), O(000000,7C,_,_,_,_,_,_  ), 0 , 0 , 1261, 111, 60 , 0 ),
  INST(Jle             , X86Jcc             , O(000F00,8E,_,_,_,_,_,_  ), O(000000,7E,_,_,_,_,_,_  ), 0 , 0 , 1264, 112, 59 , 0 ),
  INST(Jmp             , X86Jmp             , O(000000,FF,4,_,_,_,_,_  ), O(000000,EB,_,_,_,_,_,_  ), 0 , 0 , 1268, 113, 45 , 0 ),
  INST(Jna             , X86Jcc             , O(000F00,86,_,_,_,_,_,_  ), O(000000,76,_,_,_,_,_,_  ), 0 , 0 , 1272, 105, 56 , 0 ),
  INST(Jnae            , X86Jcc             , O(000F00,82,_,_,_,_,_,_  ), O(000000,72,_,_,_,_,_,_  ), 0 , 0 , 1276, 104, 57 , 0 ),
  INST(Jnb             , X86Jcc             , O(000F00,83,_,_,_,_,_,_  ), O(000000,73,_,_,_,_,_,_  ), 0 , 0 , 1281, 103, 57 , 0 ),
  INST(Jnbe            , X86Jcc             , O(000F00,87,_,_,_,_,_,_  ), O(000000,77,_,_,_,_,_,_  ), 0 , 0 , 1285, 102, 56 , 0 ),
  INST(Jnc             , X86Jcc             , O(000F00,83,_,_,_,_,_,_  ), O(000000,73,_,_,_,_,_,_  ), 0 , 0 , 1290, 114, 57 , 0 ),
  INST(Jne             , X86Jcc             , O(000F00,85,_,_,_,_,_,_  ), O(000000,75,_,_,_,_,_,_  ), 0 , 0 , 1294, 115, 58 , 0 ),
  INST(Jng             , X86Jcc             , O(000F00,8E,_,_,_,_,_,_  ), O(000000,7E,_,_,_,_,_,_  ), 0 , 0 , 1298, 112, 59 , 0 ),
  INST(Jnge            , X86Jcc             , O(000F00,8C,_,_,_,_,_,_  ), O(000000,7C,_,_,_,_,_,_  ), 0 , 0 , 1302, 111, 60 , 0 ),
  INST(Jnl             , X86Jcc             , O(000F00,8D,_,_,_,_,_,_  ), O(000000,7D,_,_,_,_,_,_  ), 0 , 0 , 1307, 110, 60 , 0 ),
  INST(Jnle            , X86Jcc             , O(000F00,8F,_,_,_,_,_,_  ), O(000000,7F,_,_,_,_,_,_  ), 0 , 0 , 1311, 109, 59 , 0 ),
  INST(Jno             , X86Jcc             , O(000F00,81,_,_,_,_,_,_  ), O(000000,71,_,_,_,_,_,_  ), 0 , 0 , 1316, 116, 54 , 0 ),
  INST(Jnp             , X86Jcc             , O(000F00,8B,_,_,_,_,_,_  ), O(000000,7B,_,_,_,_,_,_  ), 0 , 0 , 1320, 117, 61 , 0 ),
  INST(Jns             , X86Jcc             , O(000F00,89,_,_,_,_,_,_  ), O(000000,79,_,_,_,_,_,_  ), 0 , 0 , 1324, 118, 62 , 0 ),
  INST(Jnz             , X86Jcc             , O(000F00,85,_,_,_,_,_,_  ), O(000000,75,_,_,_,_,_,_  ), 0 , 0 , 1328, 115, 58 , 0 ),
  INST(Jo              , X86Jcc             , O(000F00,80,_,_,_,_,_,_  ), O(000000,70,_,_,_,_,_,_  ), 0 , 0 , 1332, 119, 54 , 0 ),
  INST(Jp              , X86Jcc             , O(000F00,8A,_,_,_,_,_,_  ), O(000000,7A,_,_,_,_,_,_  ), 0 , 0 , 1335, 120, 61 , 0 ),
  INST(Jpe             , X86Jcc             , O(000F00,8A,_,_,_,_,_,_  ), O(000000,7A,_,_,_,_,_,_  ), 0 , 0 , 1338, 120, 61 , 0 ),
  INST(Jpo             , X86Jcc             , O(000F00,8B,_,_,_,_,_,_  ), O(000000,7B,_,_,_,_,_,_  ), 0 , 0 , 1342, 117, 61 , 0 ),
  INST(Js              , X86Jcc             , O(000F00,88,_,_,_,_,_,_  ), O(000000,78,_,_,_,_,_,_  ), 0 , 0 , 1346, 121, 62 , 0 ),
  INST(Jz              , X86Jcc             , O(000F00,84,_,_,_,_,_,_  ), O(000000,74,_,_,_,_,_,_  ), 0 , 0 , 1349, 107, 58 , 0 ),
  INST(Kaddb           , VexRvm             , V(660F00,4A,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1352, 122, 63 , 0 ),
  INST(Kaddd           , VexRvm             , V(660F00,4A,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1358, 122, 64 , 0 ),
  INST(Kaddq           , VexRvm             , V(000F00,4A,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1364, 122, 64 , 0 ),
  INST(Kaddw           , VexRvm             , V(000F00,4A,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1370, 122, 63 , 0 ),
  INST(Kandb           , VexRvm             , V(660F00,41,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1376, 122, 63 , 0 ),
  INST(Kandd           , VexRvm             , V(660F00,41,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1382, 122, 64 , 0 ),
  INST(Kandnb          , VexRvm             , V(660F00,42,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1388, 122, 63 , 0 ),
  INST(Kandnd          , VexRvm             , V(660F00,42,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1395, 122, 64 , 0 ),
  INST(Kandnq          , VexRvm             , V(000F00,42,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1402, 122, 64 , 0 ),
  INST(Kandnw          , VexRvm             , V(000F00,42,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1409, 122, 65 , 0 ),
  INST(Kandq           , VexRvm             , V(000F00,41,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1416, 122, 64 , 0 ),
  INST(Kandw           , VexRvm             , V(000F00,41,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1422, 122, 65 , 0 ),
  INST(Kmovb           , VexKmov            , V(660F00,90,_,0,0,_,_,_  ), V(660F00,92,_,0,0,_,_,_  ), 0 , 0 , 1428, 123, 63 , 0 ),
  INST(Kmovd           , VexKmov            , V(660F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,0,_,_,_  ), 0 , 0 , 7344, 124, 64 , 0 ),
  INST(Kmovq           , VexKmov            , V(000F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,1,_,_,_  ), 0 , 0 , 7355, 125, 64 , 0 ),
  INST(Kmovw           , VexKmov            , V(000F00,90,_,0,0,_,_,_  ), V(000F00,92,_,0,0,_,_,_  ), 0 , 0 , 1434, 126, 65 , 0 ),
  INST(Knotb           , VexRm              , V(660F00,44,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1440, 127, 63 , 0 ),
  INST(Knotd           , VexRm              , V(660F00,44,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1446, 127, 64 , 0 ),
  INST(Knotq           , VexRm              , V(000F00,44,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1452, 127, 64 , 0 ),
  INST(Knotw           , VexRm              , V(000F00,44,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1458, 127, 65 , 0 ),
  INST(Korb            , VexRvm             , V(660F00,45,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1464, 122, 63 , 0 ),
  INST(Kord            , VexRvm             , V(660F00,45,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1469, 122, 64 , 0 ),
  INST(Korq            , VexRvm             , V(000F00,45,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1474, 122, 64 , 0 ),
  INST(Kortestb        , VexRm              , V(660F00,98,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1479, 128, 66 , 0 ),
  INST(Kortestd        , VexRm              , V(660F00,98,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1488, 128, 67 , 0 ),
  INST(Kortestq        , VexRm              , V(000F00,98,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1497, 128, 67 , 0 ),
  INST(Kortestw        , VexRm              , V(000F00,98,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1506, 128, 68 , 0 ),
  INST(Korw            , VexRvm             , V(000F00,45,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1515, 122, 65 , 0 ),
  INST(Kshiftlb        , VexRmi             , V(660F3A,32,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1520, 129, 63 , 0 ),
  INST(Kshiftld        , VexRmi             , V(660F3A,33,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1529, 129, 64 , 0 ),
  INST(Kshiftlq        , VexRmi             , V(660F3A,33,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1538, 129, 64 , 0 ),
  INST(Kshiftlw        , VexRmi             , V(660F3A,32,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1547, 129, 65 , 0 ),
  INST(Kshiftrb        , VexRmi             , V(660F3A,30,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1556, 129, 63 , 0 ),
  INST(Kshiftrd        , VexRmi             , V(660F3A,31,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1565, 129, 64 , 0 ),
  INST(Kshiftrq        , VexRmi             , V(660F3A,31,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1574, 129, 64 , 0 ),
  INST(Kshiftrw        , VexRmi             , V(660F3A,30,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1583, 129, 65 , 0 ),
  INST(Ktestb          , VexRm              , V(660F00,99,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1592, 128, 66 , 0 ),
  INST(Ktestd          , VexRm              , V(660F00,99,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1599, 128, 67 , 0 ),
  INST(Ktestq          , VexRm              , V(000F00,99,_,0,1,_,_,_  ), 0                         , 0 , 0 , 1606, 128, 67 , 0 ),
  INST(Ktestw          , VexRm              , V(000F00,99,_,0,0,_,_,_  ), 0                         , 0 , 0 , 1613, 128, 66 , 0 ),
  INST(Kunpckbw        , VexRvm             , V(660F00,4B,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1620, 122, 65 , 0 ),
  INST(Kunpckdq        , VexRvm             , V(000F00,4B,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1629, 122, 64 , 0 ),
  INST(Kunpckwd        , VexRvm             , V(000F00,4B,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1638, 122, 64 , 0 ),
  INST(Kxnorb          , VexRvm             , V(660F00,46,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1647, 122, 63 , 0 ),
  INST(Kxnord          , VexRvm             , V(660F00,46,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1654, 122, 64 , 0 ),
  INST(Kxnorq          , VexRvm             , V(000F00,46,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1661, 122, 64 , 0 ),
  INST(Kxnorw          , VexRvm             , V(000F00,46,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1668, 122, 65 , 0 ),
  INST(Kxorb           , VexRvm             , V(660F00,47,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1675, 122, 63 , 0 ),
  INST(Kxord           , VexRvm             , V(660F00,47,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1681, 122, 64 , 0 ),
  INST(Kxorq           , VexRvm             , V(000F00,47,_,1,1,_,_,_  ), 0                         , 0 , 0 , 1687, 122, 64 , 0 ),
  INST(Kxorw           , VexRvm             , V(000F00,47,_,1,0,_,_,_  ), 0                         , 0 , 0 , 1693, 122, 65 , 0 ),
  INST(Lahf            , X86Op              , O(000000,9F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1699, 130, 69 , 0 ),
  INST(Lar             , X86Rm              , O(000F00,02,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1704, 131, 70 , 0 ),
  INST(Lddqu           , ExtRm              , O(F20F00,F0,_,_,_,_,_,_  ), 0                         , 0 , 16, 5552, 132, 6  , 24),
  INST(Ldmxcsr         , X86M_Only          , O(000F00,AE,2,_,_,_,_,_  ), 0                         , 0 , 0 , 5559, 133, 5  , 0 ),
  INST(Lds             , X86Rm              , O(000000,C5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1708, 134, 45 , 0 ),
  INST(Lea             , X86Lea             , O(000000,8D,_,_,x,_,_,_  ), 0                         , 0 , 0 , 1712, 135, 0  , 0 ),
  INST(Leave           , X86Op              , O(000000,C9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1716, 34 , 45 , 0 ),
  INST(Les             , X86Rm              , O(000000,C4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1722, 134, 45 , 0 ),
  INST(Lfence          , X86Fence           , O(000F00,AE,5,_,_,_,_,_  ), 0                         , 0 , 0 , 1726, 34 , 71 , 0 ),
  INST(Lfs             , X86Rm              , O(000F00,B4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1733, 136, 45 , 0 ),
  INST(Lgdt            , X86M_Only          , O(000F00,01,2,_,_,_,_,_  ), 0                         , 0 , 0 , 1737, 35 , 23 , 0 ),
  INST(Lgs             , X86Rm              , O(000F00,B5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1742, 136, 45 , 0 ),
  INST(Lidt            , X86M_Only          , O(000F00,01,3,_,_,_,_,_  ), 0                         , 0 , 0 , 1746, 35 , 23 , 0 ),
  INST(Lldt            , X86M               , O(000F00,00,2,_,_,_,_,_  ), 0                         , 0 , 0 , 1751, 137, 23 , 0 ),
  INST(Lmsw            , X86M               , O(000F00,01,6,_,_,_,_,_  ), 0                         , 0 , 0 , 1756, 137, 23 , 0 ),
  INST(Lods            , X86StrRm           , O(000000,AC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1761, 138, 72 , 0 ),
  INST(Loop            , X86JecxzLoop       , 0                         , O(000000,E2,_,_,_,_,_,_  ), 0 , 0 , 1766, 139, 45 , 0 ),
  INST(Loope           , X86JecxzLoop       , 0                         , O(000000,E1,_,_,_,_,_,_  ), 0 , 0 , 1771, 140, 58 , 0 ),
  INST(Loopne          , X86JecxzLoop       , 0                         , O(000000,E0,_,_,_,_,_,_  ), 0 , 0 , 1777, 141, 58 , 0 ),
  INST(Lsl             , X86Rm              , O(000F00,03,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1784, 142, 70 , 0 ),
  INST(Lss             , X86Rm              , O(000F00,B2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5959, 136, 45 , 0 ),
  INST(Ltr             , X86M               , O(000F00,00,3,_,_,_,_,_  ), 0                         , 0 , 0 , 1788, 137, 23 , 0 ),
  INST(Lzcnt           , X86Rm_Raw66H       , O(F30F00,BD,_,_,x,_,_,_  ), 0                         , 0 , 0 , 1792, 143, 73 , 0 ),
  INST(Maskmovdqu      , ExtRm_ZDI          , O(660F00,57,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5568, 144, 4  , 25),
  INST(Maskmovq        , ExtRm_ZDI          , O(000F00,F7,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7352, 145, 74 , 0 ),
  INST(Maxpd           , ExtRm              , O(660F00,5F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5602, 5  , 4  , 26),
  INST(Maxps           , ExtRm              , O(000F00,5F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5609, 5  , 5  , 26),
  INST(Maxsd           , ExtRm              , O(F20F00,5F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7371, 6  , 4  , 26),
  INST(Maxss           , ExtRm              , O(F30F00,5F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5623, 7  , 5  , 26),
  INST(Mfence          , X86Fence           , O(000F00,AE,6,_,_,_,_,_  ), 0                         , 0 , 0 , 1798, 34 , 71 , 0 ),
  INST(Minpd           , ExtRm              , O(660F00,5D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5630, 5  , 4  , 27),
  INST(Minps           , ExtRm              , O(000F00,5D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5637, 5  , 5  , 27),
  INST(Minsd           , ExtRm              , O(F20F00,5D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7435, 6  , 4  , 27),
  INST(Minss           , ExtRm              , O(F30F00,5D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5651, 7  , 5  , 27),
  INST(Monitor         , X86Op              , O(000F01,C8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1805, 146, 75 , 0 ),
  INST(Mov             , X86Mov             , 0                         , 0                         , 0 , 0 , 138 , 147, 76 , 0 ),
  INST(Movapd          , ExtMov             , O(660F00,28,_,_,_,_,_,_  ), O(660F00,29,_,_,_,_,_,_  ), 0 , 16, 5658, 148, 4  , 28),
  INST(Movaps          , ExtMov             , O(000F00,28,_,_,_,_,_,_  ), O(000F00,29,_,_,_,_,_,_  ), 0 , 16, 5666, 149, 5  , 28),
  INST(Movbe           , ExtMovbe           , O(000F38,F0,_,_,x,_,_,_  ), O(000F38,F1,_,_,x,_,_,_  ), 0 , 0 , 597 , 150, 77 , 0 ),
  INST(Movd            , ExtMovd            , O(000F00,6E,_,_,_,_,_,_  ), O(000F00,7E,_,_,_,_,_,_  ), 0 , 16, 7345, 151, 78 , 29),
  INST(Movddup         , ExtMov             , O(F20F00,12,_,_,_,_,_,_  ), 0                         , 0 , 16, 5680, 49 , 6  , 29),
  INST(Movdq2q         , ExtMov             , O(F20F00,D6,_,_,_,_,_,_  ), 0                         , 0 , 8 , 1813, 152, 4  , 0 ),
  INST(Movdqa          , ExtMov             , O(660F00,6F,_,_,_,_,_,_  ), O(660F00,7F,_,_,_,_,_,_  ), 0 , 16, 5689, 153, 4  , 30),
  INST(Movdqu          , ExtMov             , O(F30F00,6F,_,_,_,_,_,_  ), O(F30F00,7F,_,_,_,_,_,_  ), 0 , 16, 5572, 154, 4  , 28),
  INST(Movhlps         , ExtMov             , O(000F00,12,_,_,_,_,_,_  ), 0                         , 0 , 8 , 5764, 155, 5  , 31),
  INST(Movhpd          , ExtMov             , O(660F00,16,_,_,_,_,_,_  ), O(660F00,17,_,_,_,_,_,_  ), 8 , 8 , 5773, 156, 4  , 32),
  INST(Movhps          , ExtMov             , O(000F00,16,_,_,_,_,_,_  ), O(000F00,17,_,_,_,_,_,_  ), 8 , 8 , 5781, 157, 5  , 32),
  INST(Movlhps         , ExtMov             , O(000F00,16,_,_,_,_,_,_  ), 0                         , 8 , 8 , 5789, 158, 5  , 31),
  INST(Movlpd          , ExtMov             , O(660F00,12,_,_,_,_,_,_  ), O(660F00,13,_,_,_,_,_,_  ), 0 , 8 , 5798, 159, 4  , 32),
  INST(Movlps          , ExtMov             , O(000F00,12,_,_,_,_,_,_  ), O(000F00,13,_,_,_,_,_,_  ), 0 , 8 , 5806, 160, 5  , 32),
  INST(Movmskpd        , ExtMov             , O(660F00,50,_,_,_,_,_,_  ), 0                         , 0 , 8 , 5814, 161, 4  , 33),
  INST(Movmskps        , ExtMov             , O(000F00,50,_,_,_,_,_,_  ), 0                         , 0 , 8 , 5824, 161, 5  , 33),
  INST(Movntdq         , ExtMov             , 0                         , O(660F00,E7,_,_,_,_,_,_  ), 0 , 16, 5834, 162, 4  , 33),
  INST(Movntdqa        , ExtMov             , O(660F38,2A,_,_,_,_,_,_  ), 0                         , 0 , 16, 5843, 132, 12 , 33),
  INST(Movnti          , ExtMovnti          , O(000F00,C3,_,_,x,_,_,_  ), 0                         , 0 , 8 , 1821, 163, 4  , 0 ),
  INST(Movntpd         , ExtMov             , 0                         , O(660F00,2B,_,_,_,_,_,_  ), 0 , 16, 5853, 164, 4  , 34),
  INST(Movntps         , ExtMov             , 0                         , O(000F00,2B,_,_,_,_,_,_  ), 0 , 16, 5862, 165, 5  , 34),
  INST(Movntq          , ExtMov             , 0                         , O(000F00,E7,_,_,_,_,_,_  ), 0 , 8 , 1828, 166, 74 , 0 ),
  INST(Movntsd         , ExtMov             , 0                         , O(F20F00,2B,_,_,_,_,_,_  ), 0 , 8 , 1835, 167, 46 , 0 ),
  INST(Movntss         , ExtMov             , 0                         , O(F30F00,2B,_,_,_,_,_,_  ), 0 , 4 , 1843, 168, 46 , 0 ),
  INST(Movq            , ExtMovq            , O(000F00,6E,_,_,x,_,_,_  ), O(000F00,7E,_,_,x,_,_,_  ), 0 , 16, 7356, 169, 78 , 28),
  INST(Movq2dq         , ExtRm              , O(F30F00,D6,_,_,_,_,_,_  ), 0                         , 0 , 16, 1851, 170, 4  , 0 ),
  INST(Movs            , X86StrMm           , O(000000,A4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 411 , 171, 72 , 0 ),
  INST(Movsd           , ExtMov             , O(F20F00,10,_,_,_,_,_,_  ), O(F20F00,11,_,_,_,_,_,_  ), 0 , 8 , 5877, 172, 79 , 35),
  INST(Movshdup        , ExtRm              , O(F30F00,16,_,_,_,_,_,_  ), 0                         , 0 , 16, 5884, 50 , 6  , 30),
  INST(Movsldup        , ExtRm              , O(F30F00,12,_,_,_,_,_,_  ), 0                         , 0 , 16, 5894, 50 , 6  , 30),
  INST(Movss           , ExtMov             , O(F30F00,10,_,_,_,_,_,_  ), O(F30F00,11,_,_,_,_,_,_  ), 0 , 4 , 5904, 173, 80 , 35),
  INST(Movsx           , X86MovsxMovzx      , O(000F00,BE,_,_,x,_,_,_  ), 0                         , 0 , 0 , 1859, 174, 0  , 0 ),
  INST(Movsxd          , X86Rm              , O(000000,63,_,_,1,_,_,_  ), 0                         , 0 , 0 , 1865, 175, 0  , 0 ),
  INST(Movupd          , ExtMov             , O(660F00,10,_,_,_,_,_,_  ), O(660F00,11,_,_,_,_,_,_  ), 0 , 16, 5911, 176, 4  , 36),
  INST(Movups          , ExtMov             , O(000F00,10,_,_,_,_,_,_  ), O(000F00,11,_,_,_,_,_,_  ), 0 , 16, 5919, 177, 5  , 36),
  INST(Movzx           , X86MovsxMovzx      , O(000F00,B6,_,_,x,_,_,_  ), 0                         , 0 , 0 , 1872, 174, 0  , 0 ),
  INST(Mpsadbw         , ExtRmi             , O(660F3A,42,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5927, 16 , 12 , 37),
  INST(Mul             , X86M_GPB_MulDiv    , O(000000,F6,4,_,x,_,_,_  ), 0                         , 0 , 0 , 769 , 178, 1  , 0 ),
  INST(Mulpd           , ExtRm              , O(660F00,59,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5936, 5  , 4  , 38),
  INST(Mulps           , ExtRm              , O(000F00,59,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5943, 5  , 5  , 38),
  INST(Mulsd           , ExtRm              , O(F20F00,59,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5950, 6  , 4  , 38),
  INST(Mulss           , ExtRm              , O(F30F00,59,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5957, 7  , 5  , 38),
  INST(Mulx            , VexRvm_ZDX_Wx      , V(F20F38,F6,_,0,x,_,_,_  ), 0                         , 0 , 0 , 1878, 179, 81 , 0 ),
  INST(Mwait           , X86Op              , O(000F01,C9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1883, 180, 75 , 0 ),
  INST(Neg             , X86M_GPB           , O(000000,F6,3,_,x,_,_,_  ), 0                         , 0 , 0 , 1889, 181, 1  , 0 ),
  INST(Nop             , X86Op              , O(000000,90,_,_,_,_,_,_  ), 0                         , 0 , 0 , 900 , 182, 0  , 0 ),
  INST(Not             , X86M_GPB           , O(000000,F6,2,_,x,_,_,_  ), 0                         , 0 , 0 , 1893, 181, 0  , 0 ),
  INST(Or              , X86Arith           , O(000000,08,1,_,x,_,_,_  ), 0                         , 0 , 0 , 1109, 183, 1  , 0 ),
  INST(Orpd            , ExtRm              , O(660F00,56,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9429, 12 , 4  , 39),
  INST(Orps            , ExtRm              , O(000F00,56,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9436, 12 , 5  , 39),
  INST(Out             , X86Out             , O(000000,EE,_,_,_,_,_,_  ), O(000000,E6,_,_,_,_,_,_  ), 0 , 0 , 1897, 184, 45 , 0 ),
  INST(Outs            , X86Outs            , O(000000,6E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1901, 185, 45 , 0 ),
  INST(Pabsb           , ExtRm_P            , O(000F38,1C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5997, 186, 82 , 40),
  INST(Pabsd           , ExtRm_P            , O(000F38,1E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6004, 186, 82 , 40),
  INST(Pabsw           , ExtRm_P            , O(000F38,1D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6018, 186, 82 , 41),
  INST(Packssdw        , ExtRm_P            , O(000F00,6B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6025, 187, 78 , 42),
  INST(Packsswb        , ExtRm_P            , O(000F00,63,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6035, 187, 78 , 42),
  INST(Packusdw        , ExtRm              , O(660F38,2B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6045, 5  , 12 , 42),
  INST(Packuswb        , ExtRm_P            , O(000F00,67,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6055, 187, 78 , 42),
  INST(Paddb           , ExtRm_P            , O(000F00,FC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6065, 187, 78 , 42),
  INST(Paddd           , ExtRm_P            , O(000F00,FE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6072, 187, 78 , 42),
  INST(Paddq           , ExtRm_P            , O(000F00,D4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6079, 187, 4  , 42),
  INST(Paddsb          , ExtRm_P            , O(000F00,EC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6086, 187, 78 , 42),
  INST(Paddsw          , ExtRm_P            , O(000F00,ED,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6094, 187, 78 , 42),
  INST(Paddusb         , ExtRm_P            , O(000F00,DC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6102, 187, 78 , 42),
  INST(Paddusw         , ExtRm_P            , O(000F00,DD,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6111, 187, 78 , 42),
  INST(Paddw           , ExtRm_P            , O(000F00,FD,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6120, 187, 78 , 42),
  INST(Palignr         , ExtRmi_P           , O(000F3A,0F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6127, 188, 6  , 42),
  INST(Pand            , ExtRm_P            , O(000F00,DB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6136, 189, 78 , 42),
  INST(Pandn           , ExtRm_P            , O(000F00,DF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6149, 190, 78 , 43),
  INST(Pause           , X86Op              , O(F30000,90,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1906, 34 , 45 , 0 ),
  INST(Pavgb           , ExtRm_P            , O(000F00,E0,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6179, 187, 83 , 44),
  INST(Pavgusb         , Ext3dNow           , O(000F0F,BF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1912, 191, 84 , 0 ),
  INST(Pavgw           , ExtRm_P            , O(000F00,E3,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6186, 187, 83 , 45),
  INST(Pblendvb        , ExtRm_XMM0         , O(660F38,10,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6202, 17 , 12 , 46),
  INST(Pblendw         , ExtRmi             , O(660F3A,0E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6212, 16 , 12 , 44),
  INST(Pclmulqdq       , ExtRmi             , O(660F3A,44,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6305, 16 , 85 , 47),
  INST(Pcmpeqb         , ExtRm_P            , O(000F00,74,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6337, 190, 78 , 48),
  INST(Pcmpeqd         , ExtRm_P            , O(000F00,76,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6346, 190, 78 , 48),
  INST(Pcmpeqq         , ExtRm              , O(660F38,29,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6355, 192, 12 , 48),
  INST(Pcmpeqw         , ExtRm_P            , O(000F00,75,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6364, 190, 78 , 48),
  INST(Pcmpestri       , ExtRmi             , O(660F3A,61,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6373, 193, 86 , 49),
  INST(Pcmpestrm       , ExtRmi             , O(660F3A,60,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6384, 194, 86 , 49),
  INST(Pcmpgtb         , ExtRm_P            , O(000F00,64,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6395, 190, 78 , 48),
  INST(Pcmpgtd         , ExtRm_P            , O(000F00,66,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6404, 190, 78 , 48),
  INST(Pcmpgtq         , ExtRm              , O(660F38,37,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6413, 192, 42 , 48),
  INST(Pcmpgtw         , ExtRm_P            , O(000F00,65,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6422, 190, 78 , 48),
  INST(Pcmpistri       , ExtRmi             , O(660F3A,63,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6431, 195, 86 , 49),
  INST(Pcmpistrm       , ExtRmi             , O(660F3A,62,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6442, 196, 86 , 49),
  INST(Pcommit         , X86Op_O            , O(660F00,AE,7,_,_,_,_,_  ), 0                         , 0 , 0 , 1920, 34 , 87 , 0 ),
  INST(Pdep            , VexRvm_Wx          , V(F20F38,F5,_,0,x,_,_,_  ), 0                         , 0 , 0 , 1928, 11 , 81 , 0 ),
  INST(Pext            , VexRvm_Wx          , V(F30F38,F5,_,0,x,_,_,_  ), 0                         , 0 , 0 , 1933, 11 , 81 , 0 ),
  INST(Pextrb          , ExtExtract         , O(000F3A,14,_,_,_,_,_,_  ), 0                         , 0 , 8 , 6847, 197, 12 , 50),
  INST(Pextrd          , ExtExtract         , O(000F3A,16,_,_,_,_,_,_  ), 0                         , 0 , 8 , 6855, 67 , 12 , 50),
  INST(Pextrq          , ExtExtract         , O(000F3A,16,_,_,1,_,_,_  ), 0                         , 0 , 8 , 6863, 198, 12 , 50),
  INST(Pextrw          , ExtPextrw          , O(000F00,C5,_,_,_,_,_,_  ), O(000F3A,15,_,_,_,_,_,_  ), 0 , 8 , 6871, 199, 88 , 50),
  INST(Pf2id           , Ext3dNow           , O(000F0F,1D,_,_,_,_,_,_  ), 0                         , 0 , 8 , 1938, 200, 84 , 0 ),
  INST(Pf2iw           , Ext3dNow           , O(000F0F,1C,_,_,_,_,_,_  ), 0                         , 0 , 8 , 1944, 200, 89 , 0 ),
  INST(Pfacc           , Ext3dNow           , O(000F0F,AE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1950, 191, 84 , 0 ),
  INST(Pfadd           , Ext3dNow           , O(000F0F,9E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1956, 191, 84 , 0 ),
  INST(Pfcmpeq         , Ext3dNow           , O(000F0F,B0,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1962, 191, 84 , 0 ),
  INST(Pfcmpge         , Ext3dNow           , O(000F0F,90,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1970, 191, 84 , 0 ),
  INST(Pfcmpgt         , Ext3dNow           , O(000F0F,A0,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1978, 191, 84 , 0 ),
  INST(Pfmax           , Ext3dNow           , O(000F0F,A4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1986, 191, 84 , 0 ),
  INST(Pfmin           , Ext3dNow           , O(000F0F,94,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1992, 191, 84 , 0 ),
  INST(Pfmul           , Ext3dNow           , O(000F0F,B4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 1998, 191, 84 , 0 ),
  INST(Pfnacc          , Ext3dNow           , O(000F0F,8A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2004, 191, 89 , 0 ),
  INST(Pfpnacc         , Ext3dNow           , O(000F0F,8E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2011, 191, 89 , 0 ),
  INST(Pfrcp           , Ext3dNow           , O(000F0F,96,_,_,_,_,_,_  ), 0                         , 0 , 8 , 2019, 200, 84 , 0 ),
  INST(Pfrcpit1        , Ext3dNow           , O(000F0F,A6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2025, 191, 84 , 0 ),
  INST(Pfrcpit2        , Ext3dNow           , O(000F0F,B6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2034, 191, 84 , 0 ),
  INST(Pfrcpv          , Ext3dNow           , O(000F0F,86,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2043, 191, 90 , 0 ),
  INST(Pfrsqit1        , Ext3dNow           , O(000F0F,A7,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2050, 201, 84 , 0 ),
  INST(Pfrsqrt         , Ext3dNow           , O(000F0F,97,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2059, 201, 84 , 0 ),
  INST(Pfrsqrtv        , Ext3dNow           , O(000F0F,87,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2067, 191, 90 , 0 ),
  INST(Pfsub           , Ext3dNow           , O(000F0F,9A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2076, 191, 84 , 0 ),
  INST(Pfsubr          , Ext3dNow           , O(000F0F,AA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2082, 191, 84 , 0 ),
  INST(Phaddd          , ExtRm_P            , O(000F38,02,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6950, 187, 82 , 51),
  INST(Phaddsw         , ExtRm_P            , O(000F38,03,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6967, 187, 82 , 52),
  INST(Phaddw          , ExtRm_P            , O(000F38,01,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7036, 187, 82 , 53),
  INST(Phminposuw      , ExtRm              , O(660F38,41,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7062, 5  , 12 , 54),
  INST(Phsubd          , ExtRm_P            , O(000F38,06,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7083, 187, 82 , 55),
  INST(Phsubsw         , ExtRm_P            , O(000F38,07,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7100, 187, 82 , 56),
  INST(Phsubw          , ExtRm_P            , O(000F38,05,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7109, 187, 82 , 56),
  INST(Pi2fd           , Ext3dNow           , O(000F0F,0D,_,_,_,_,_,_  ), 0                         , 0 , 8 , 2089, 200, 84 , 0 ),
  INST(Pi2fw           , Ext3dNow           , O(000F0F,0C,_,_,_,_,_,_  ), 0                         , 0 , 8 , 2095, 200, 89 , 0 ),
  INST(Pinsrb          , ExtRmi             , O(660F3A,20,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7126, 202, 12 , 57),
  INST(Pinsrd          , ExtRmi             , O(660F3A,22,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7134, 203, 12 , 57),
  INST(Pinsrq          , ExtRmi             , O(660F3A,22,_,_,1,_,_,_  ), 0                         , 0 , 0 , 7142, 204, 12 , 57),
  INST(Pinsrw          , ExtRmi_P           , O(000F00,C4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7150, 205, 83 , 55),
  INST(Pmaddubsw       , ExtRm_P            , O(000F38,04,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7320, 187, 82 , 58),
  INST(Pmaddwd         , ExtRm_P            , O(000F00,F5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7331, 187, 78 , 58),
  INST(Pmaxsb          , ExtRm              , O(660F38,3C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7362, 12 , 12 , 59),
  INST(Pmaxsd          , ExtRm              , O(660F38,3D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7370, 12 , 12 , 59),
  INST(Pmaxsw          , ExtRm_P            , O(000F00,EE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7386, 189, 83 , 60),
  INST(Pmaxub          , ExtRm_P            , O(000F00,DE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7394, 189, 83 , 60),
  INST(Pmaxud          , ExtRm              , O(660F38,3F,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7402, 12 , 12 , 60),
  INST(Pmaxuw          , ExtRm              , O(660F38,3E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7418, 12 , 12 , 61),
  INST(Pminsb          , ExtRm              , O(660F38,38,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7426, 12 , 12 , 61),
  INST(Pminsd          , ExtRm              , O(660F38,39,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7434, 12 , 12 , 61),
  INST(Pminsw          , ExtRm_P            , O(000F00,EA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7450, 189, 83 , 62),
  INST(Pminub          , ExtRm_P            , O(000F00,DA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7458, 189, 83 , 62),
  INST(Pminud          , ExtRm              , O(660F38,3B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7466, 12 , 12 , 62),
  INST(Pminuw          , ExtRm              , O(660F38,3A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7482, 12 , 12 , 63),
  INST(Pmovmskb        , ExtRm_P            , O(000F00,D7,_,_,_,_,_,_  ), 0                         , 0 , 8 , 7560, 206, 83 , 64),
  INST(Pmovsxbd        , ExtRm              , O(660F38,21,_,_,_,_,_,_  ), 0                         , 0 , 16, 7657, 207, 12 , 14),
  INST(Pmovsxbq        , ExtRm              , O(660F38,22,_,_,_,_,_,_  ), 0                         , 0 , 16, 7667, 208, 12 , 14),
  INST(Pmovsxbw        , ExtRm              , O(660F38,20,_,_,_,_,_,_  ), 0                         , 0 , 16, 7677, 49 , 12 , 14),
  INST(Pmovsxdq        , ExtRm              , O(660F38,25,_,_,_,_,_,_  ), 0                         , 0 , 16, 7687, 49 , 12 , 14),
  INST(Pmovsxwd        , ExtRm              , O(660F38,23,_,_,_,_,_,_  ), 0                         , 0 , 16, 7697, 49 , 12 , 14),
  INST(Pmovsxwq        , ExtRm              , O(660F38,24,_,_,_,_,_,_  ), 0                         , 0 , 16, 7707, 207, 12 , 14),
  INST(Pmovzxbd        , ExtRm              , O(660F38,31,_,_,_,_,_,_  ), 0                         , 0 , 16, 7794, 207, 12 , 65),
  INST(Pmovzxbq        , ExtRm              , O(660F38,32,_,_,_,_,_,_  ), 0                         , 0 , 16, 7804, 208, 12 , 65),
  INST(Pmovzxbw        , ExtRm              , O(660F38,30,_,_,_,_,_,_  ), 0                         , 0 , 16, 7814, 49 , 12 , 65),
  INST(Pmovzxdq        , ExtRm              , O(660F38,35,_,_,_,_,_,_  ), 0                         , 0 , 16, 7824, 49 , 12 , 65),
  INST(Pmovzxwd        , ExtRm              , O(660F38,33,_,_,_,_,_,_  ), 0                         , 0 , 16, 7834, 49 , 12 , 65),
  INST(Pmovzxwq        , ExtRm              , O(660F38,34,_,_,_,_,_,_  ), 0                         , 0 , 16, 7844, 207, 12 , 65),
  INST(Pmuldq          , ExtRm              , O(660F38,28,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7854, 5  , 12 , 19),
  INST(Pmulhrsw        , ExtRm_P            , O(000F38,0B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7862, 187, 82 , 19),
  INST(Pmulhrw         , Ext3dNow           , O(000F0F,B7,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2101, 191, 84 , 0 ),
  INST(Pmulhuw         , ExtRm_P            , O(000F00,E4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7872, 187, 83 , 66),
  INST(Pmulhw          , ExtRm_P            , O(000F00,E5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7881, 187, 78 , 66),
  INST(Pmulld          , ExtRm              , O(660F38,40,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7889, 5  , 12 , 66),
  INST(Pmullw          , ExtRm_P            , O(000F00,D5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7905, 187, 78 , 19),
  INST(Pmuludq         , ExtRm_P            , O(000F00,F4,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7928, 187, 4  , 67),
  INST(Pop             , X86Pop             , O(000000,8F,0,_,_,_,_,_  ), O(000000,58,_,_,_,_,_,_  ), 0 , 0 , 2109, 209, 45 , 0 ),
  INST(Popa            , X86Op              , O(660000,61,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2113, 99 , 45 , 0 ),
  INST(Popad           , X86Op              , O(000000,61,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2118, 99 , 45 , 0 ),
  INST(Popcnt          , X86Rm_Raw66H       , O(F30F00,B8,_,_,x,_,_,_  ), 0                         , 0 , 0 , 2124, 143, 91 , 0 ),
  INST(Popf            , X86Op              , O(660000,9D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2131, 34 , 16 , 0 ),
  INST(Popfd           , X86Op              , O(000000,9D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2136, 99 , 16 , 0 ),
  INST(Popfq           , X86Op              , O(000000,9D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2142, 101, 16 , 0 ),
  INST(Por             , ExtRm_P            , O(000F00,EB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 7955, 189, 78 , 68),
  INST(Prefetch        , X86M_Only          , O(000F00,0D,0,_,_,_,_,_  ), 0                         , 0 , 0 , 2148, 35 , 92 , 0 ),
  INST(Prefetchnta     , X86M_Only          , O(000F00,18,0,_,_,_,_,_  ), 0                         , 0 , 0 , 2157, 35 , 93 , 0 ),
  INST(Prefetcht0      , X86M_Only          , O(000F00,18,1,_,_,_,_,_  ), 0                         , 0 , 0 , 2169, 35 , 93 , 0 ),
  INST(Prefetcht1      , X86M_Only          , O(000F00,18,2,_,_,_,_,_  ), 0                         , 0 , 0 , 2180, 35 , 93 , 0 ),
  INST(Prefetcht2      , X86M_Only          , O(000F00,18,3,_,_,_,_,_  ), 0                         , 0 , 0 , 2191, 35 , 93 , 0 ),
  INST(Prefetchw       , X86M_Only          , O(000F00,0D,1,_,_,_,_,_  ), 0                         , 0 , 0 , 2202, 35 , 94 , 0 ),
  INST(Prefetchwt1     , X86M_Only          , O(000F00,0D,2,_,_,_,_,_  ), 0                         , 0 , 0 , 2212, 35 , 95 , 0 ),
  INST(Psadbw          , ExtRm_P            , O(000F00,F6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 3795, 187, 83 , 69),
  INST(Pshufb          , ExtRm_P            , O(000F38,00,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8179, 187, 82 , 70),
  INST(Pshufd          , ExtRmi             , O(660F00,70,_,_,_,_,_,_  ), 0                         , 0 , 16, 8187, 210, 4  , 71),
  INST(Pshufhw         , ExtRmi             , O(F30F00,70,_,_,_,_,_,_  ), 0                         , 0 , 16, 8195, 210, 4  , 71),
  INST(Pshuflw         , ExtRmi             , O(F20F00,70,_,_,_,_,_,_  ), 0                         , 0 , 16, 8204, 210, 4  , 71),
  INST(Pshufw          , ExtRmi_P           , O(000F00,70,_,_,_,_,_,_  ), 0                         , 0 , 8 , 2224, 211, 74 , 0 ),
  INST(Psignb          , ExtRm_P            , O(000F38,08,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8213, 187, 82 , 72),
  INST(Psignd          , ExtRm_P            , O(000F38,0A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8221, 187, 82 , 72),
  INST(Psignw          , ExtRm_P            , O(000F38,09,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8229, 187, 82 , 72),
  INST(Pslld           , ExtRmRi_P          , O(000F00,F2,_,_,_,_,_,_  ), O(000F00,72,6,_,_,_,_,_  ), 0 , 0 , 8237, 212, 78 , 72),
  INST(Pslldq          , ExtRmRi            , 0                         , O(660F00,73,7,_,_,_,_,_  ), 0 , 0 , 8244, 213, 4  , 72),
  INST(Psllq           , ExtRmRi_P          , O(000F00,F3,_,_,_,_,_,_  ), O(000F00,73,6,_,_,_,_,_  ), 0 , 0 , 8252, 214, 78 , 72),
  INST(Psllw           , ExtRmRi_P          , O(000F00,F1,_,_,_,_,_,_  ), O(000F00,71,6,_,_,_,_,_  ), 0 , 0 , 8283, 215, 78 , 73),
  INST(Psrad           , ExtRmRi_P          , O(000F00,E2,_,_,_,_,_,_  ), O(000F00,72,4,_,_,_,_,_  ), 0 , 0 , 8290, 216, 78 , 73),
  INST(Psraw           , ExtRmRi_P          , O(000F00,E1,_,_,_,_,_,_  ), O(000F00,71,4,_,_,_,_,_  ), 0 , 0 , 8328, 217, 78 , 74),
  INST(Psrld           , ExtRmRi_P          , O(000F00,D2,_,_,_,_,_,_  ), O(000F00,72,2,_,_,_,_,_  ), 0 , 0 , 8335, 218, 78 , 74),
  INST(Psrldq          , ExtRmRi            , 0                         , O(660F00,73,3,_,_,_,_,_  ), 0 , 0 , 8342, 219, 4  , 74),
  INST(Psrlq           , ExtRmRi_P          , O(000F00,D3,_,_,_,_,_,_  ), O(000F00,73,2,_,_,_,_,_  ), 0 , 0 , 8350, 220, 78 , 74),
  INST(Psrlw           , ExtRmRi_P          , O(000F00,D1,_,_,_,_,_,_  ), O(000F00,71,2,_,_,_,_,_  ), 0 , 0 , 8381, 221, 78 , 75),
  INST(Psubb           , ExtRm_P            , O(000F00,F8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8388, 190, 78 , 75),
  INST(Psubd           , ExtRm_P            , O(000F00,FA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8395, 190, 78 , 75),
  INST(Psubq           , ExtRm_P            , O(000F00,FB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8402, 190, 4  , 75),
  INST(Psubsb          , ExtRm_P            , O(000F00,E8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8409, 190, 78 , 75),
  INST(Psubsw          , ExtRm_P            , O(000F00,E9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8417, 190, 78 , 75),
  INST(Psubusb         , ExtRm_P            , O(000F00,D8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8425, 190, 78 , 75),
  INST(Psubusw         , ExtRm_P            , O(000F00,D9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8434, 190, 78 , 75),
  INST(Psubw           , ExtRm_P            , O(000F00,F9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8443, 190, 78 , 75),
  INST(Pswapd          , Ext3dNow           , O(000F0F,BB,_,_,_,_,_,_  ), 0                         , 0 , 8 , 2231, 200, 89 , 0 ),
  INST(Ptest           , ExtRm              , O(660F38,17,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8472, 222, 96 , 76),
  INST(Punpckhbw       , ExtRm_P            , O(000F00,68,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8555, 187, 78 , 77),
  INST(Punpckhdq       , ExtRm_P            , O(000F00,6A,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8566, 187, 78 , 77),
  INST(Punpckhqdq      , ExtRm              , O(660F00,6D,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8577, 5  , 4  , 77),
  INST(Punpckhwd       , ExtRm_P            , O(000F00,69,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8589, 187, 78 , 77),
  INST(Punpcklbw       , ExtRm_P            , O(000F00,60,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8600, 187, 78 , 77),
  INST(Punpckldq       , ExtRm_P            , O(000F00,62,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8611, 187, 78 , 77),
  INST(Punpcklqdq      , ExtRm              , O(660F00,6C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8622, 5  , 4  , 77),
  INST(Punpcklwd       , ExtRm_P            , O(000F00,61,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8634, 187, 78 , 77),
  INST(Push            , X86Push            , O(000000,FF,6,_,_,_,_,_  ), O(000000,50,_,_,_,_,_,_  ), 0 , 0 , 2238, 223, 45 , 0 ),
  INST(Pusha           , X86Op              , O(660000,60,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2243, 99 , 45 , 0 ),
  INST(Pushad          , X86Op              , O(000000,60,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2249, 99 , 45 , 0 ),
  INST(Pushf           , X86Op              , O(660000,9C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2256, 34 , 45 , 0 ),
  INST(Pushfd          , X86Op              , O(000000,9C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2262, 99 , 45 , 0 ),
  INST(Pushfq          , X86Op              , O(000000,9C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2269, 101, 45 , 0 ),
  INST(Pxor            , ExtRm_P            , O(000F00,EF,_,_,_,_,_,_  ), 0                         , 0 , 0 , 8645, 190, 78 , 78),
  INST(Rcl             , X86Rot             , O(000000,D0,2,_,x,_,_,_  ), 0                         , 0 , 0 , 2276, 224, 97 , 0 ),
  INST(Rcpps           , ExtRm              , O(000F00,53,_,_,_,_,_,_  ), 0                         , 0 , 16, 8773, 50 , 5  , 79),
  INST(Rcpss           , ExtRm              , O(F30F00,53,_,_,_,_,_,_  ), 0                         , 0 , 4 , 8780, 225, 5  , 80),
  INST(Rcr             , X86Rot             , O(000000,D0,3,_,x,_,_,_  ), 0                         , 0 , 0 , 2280, 224, 97 , 0 ),
  INST(Rdfsbase        , X86M               , O(F30F00,AE,0,_,x,_,_,_  ), 0                         , 0 , 8 , 2284, 226, 98 , 0 ),
  INST(Rdgsbase        , X86M               , O(F30F00,AE,1,_,x,_,_,_  ), 0                         , 0 , 8 , 2293, 226, 98 , 0 ),
  INST(Rdmsr           , X86Op              , O(000F00,32,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2302, 227, 99 , 0 ),
  INST(Rdpmc           , X86Op              , O(000F00,33,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2308, 227, 23 , 0 ),
  INST(Rdrand          , X86M               , O(000F00,C7,6,_,x,_,_,_  ), 0                         , 0 , 8 , 2314, 228, 100, 0 ),
  INST(Rdseed          , X86M               , O(000F00,C7,7,_,x,_,_,_  ), 0                         , 0 , 8 , 2321, 228, 101, 0 ),
  INST(Rdtsc           , X86Op              , O(000F00,31,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2328, 229, 102, 0 ),
  INST(Rdtscp          , X86Op              , O(000F01,F9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2334, 230, 103, 0 ),
  INST(Ret             , X86Ret             , O(000000,C2,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2748, 231, 45 , 0 ),
  INST(Rol             , X86Rot             , O(000000,D0,0,_,x,_,_,_  ), 0                         , 0 , 0 , 2341, 224, 97 , 0 ),
  INST(Ror             , X86Rot             , O(000000,D0,1,_,x,_,_,_  ), 0                         , 0 , 0 , 2345, 224, 97 , 0 ),
  INST(Rorx            , VexRmi_Wx          , V(F20F3A,F0,_,0,x,_,_,_  ), 0                         , 0 , 0 , 2349, 232, 81 , 0 ),
  INST(Roundpd         , ExtRmi             , O(660F3A,09,_,_,_,_,_,_  ), 0                         , 0 , 16, 8875, 210, 12 , 81),
  INST(Roundps         , ExtRmi             , O(660F3A,08,_,_,_,_,_,_  ), 0                         , 0 , 16, 8884, 210, 12 , 81),
  INST(Roundsd         , ExtRmi             , O(660F3A,0B,_,_,_,_,_,_  ), 0                         , 0 , 8 , 8893, 233, 12 , 82),
  INST(Roundss         , ExtRmi             , O(660F3A,0A,_,_,_,_,_,_  ), 0                         , 0 , 4 , 8902, 234, 12 , 82),
  INST(Rsm             , X86Op              , O(000F00,AA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2354, 99 , 16 , 0 ),
  INST(Rsqrtps         , ExtRm              , O(000F00,52,_,_,_,_,_,_  ), 0                         , 0 , 16, 8999, 50 , 5  , 3 ),
  INST(Rsqrtss         , ExtRm              , O(F30F00,52,_,_,_,_,_,_  ), 0                         , 0 , 4 , 9008, 225, 5  , 2 ),
  INST(Sahf            , X86Op              , O(000000,9E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2358, 235, 104, 0 ),
  INST(Sal             , X86Rot             , O(000000,D0,4,_,x,_,_,_  ), 0                         , 0 , 0 , 2363, 224, 1  , 0 ),
  INST(Sar             , X86Rot             , O(000000,D0,7,_,x,_,_,_  ), 0                         , 0 , 0 , 2367, 224, 1  , 0 ),
  INST(Sarx            , VexRmv_Wx          , V(F30F38,F7,_,0,x,_,_,_  ), 0                         , 0 , 0 , 2371, 14 , 81 , 0 ),
  INST(Sbb             , X86Arith           , O(000000,18,3,_,x,_,_,_  ), 0                         , 0 , 0 , 2376, 3  , 2  , 0 ),
  INST(Scas            , X86StrRm           , O(000000,AE,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2380, 236, 35 , 0 ),
  INST(Seta            , X86Set             , O(000F00,97,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2385, 237, 105, 0 ),
  INST(Setae           , X86Set             , O(000F00,93,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2390, 237, 106, 0 ),
  INST(Setb            , X86Set             , O(000F00,92,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2396, 237, 106, 0 ),
  INST(Setbe           , X86Set             , O(000F00,96,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2401, 237, 105, 0 ),
  INST(Setc            , X86Set             , O(000F00,92,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2407, 237, 106, 0 ),
  INST(Sete            , X86Set             , O(000F00,94,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2412, 237, 107, 0 ),
  INST(Setg            , X86Set             , O(000F00,9F,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2417, 237, 108, 0 ),
  INST(Setge           , X86Set             , O(000F00,9D,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2422, 237, 109, 0 ),
  INST(Setl            , X86Set             , O(000F00,9C,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2428, 237, 109, 0 ),
  INST(Setle           , X86Set             , O(000F00,9E,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2433, 237, 108, 0 ),
  INST(Setna           , X86Set             , O(000F00,96,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2439, 237, 105, 0 ),
  INST(Setnae          , X86Set             , O(000F00,92,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2445, 237, 106, 0 ),
  INST(Setnb           , X86Set             , O(000F00,93,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2452, 237, 106, 0 ),
  INST(Setnbe          , X86Set             , O(000F00,97,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2458, 237, 105, 0 ),
  INST(Setnc           , X86Set             , O(000F00,93,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2465, 237, 106, 0 ),
  INST(Setne           , X86Set             , O(000F00,95,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2471, 237, 107, 0 ),
  INST(Setng           , X86Set             , O(000F00,9E,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2477, 237, 108, 0 ),
  INST(Setnge          , X86Set             , O(000F00,9C,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2483, 237, 109, 0 ),
  INST(Setnl           , X86Set             , O(000F00,9D,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2490, 237, 109, 0 ),
  INST(Setnle          , X86Set             , O(000F00,9F,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2496, 237, 108, 0 ),
  INST(Setno           , X86Set             , O(000F00,91,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2503, 237, 110, 0 ),
  INST(Setnp           , X86Set             , O(000F00,9B,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2509, 237, 111, 0 ),
  INST(Setns           , X86Set             , O(000F00,99,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2515, 237, 112, 0 ),
  INST(Setnz           , X86Set             , O(000F00,95,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2521, 237, 107, 0 ),
  INST(Seto            , X86Set             , O(000F00,90,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2527, 237, 110, 0 ),
  INST(Setp            , X86Set             , O(000F00,9A,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2532, 237, 111, 0 ),
  INST(Setpe           , X86Set             , O(000F00,9A,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2537, 237, 111, 0 ),
  INST(Setpo           , X86Set             , O(000F00,9B,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2543, 237, 111, 0 ),
  INST(Sets            , X86Set             , O(000F00,98,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2549, 237, 112, 0 ),
  INST(Setz            , X86Set             , O(000F00,94,_,_,_,_,_,_  ), 0                         , 0 , 1 , 2554, 237, 107, 0 ),
  INST(Sfence          , X86Fence           , O(000F00,AE,7,_,_,_,_,_  ), 0                         , 0 , 0 , 2559, 34 , 113, 0 ),
  INST(Sgdt            , X86M_Only          , O(000F00,01,0,_,_,_,_,_  ), 0                         , 0 , 0 , 2566, 84 , 45 , 0 ),
  INST(Sha1msg1        , ExtRm              , O(000F38,C9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2571, 5  , 114, 0 ),
  INST(Sha1msg2        , ExtRm              , O(000F38,CA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2580, 5  , 114, 0 ),
  INST(Sha1nexte       , ExtRm              , O(000F38,C8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2589, 5  , 114, 0 ),
  INST(Sha1rnds4       , ExtRmi             , O(000F3A,CC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2599, 16 , 114, 0 ),
  INST(Sha256msg1      , ExtRm              , O(000F38,CC,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2609, 5  , 114, 0 ),
  INST(Sha256msg2      , ExtRm              , O(000F38,CD,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2620, 5  , 114, 0 ),
  INST(Sha256rnds2     , ExtRm_XMM0         , O(000F38,CB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2631, 17 , 114, 0 ),
  INST(Shl             , X86Rot             , O(000000,D0,4,_,x,_,_,_  ), 0                         , 0 , 0 , 2643, 224, 1  , 0 ),
  INST(Shld            , X86ShldShrd        , O(000F00,A4,_,_,x,_,_,_  ), 0                         , 0 , 0 , 8159, 238, 1  , 0 ),
  INST(Shlx            , VexRmv_Wx          , V(660F38,F7,_,0,x,_,_,_  ), 0                         , 0 , 0 , 2647, 14 , 81 , 0 ),
  INST(Shr             , X86Rot             , O(000000,D0,5,_,x,_,_,_  ), 0                         , 0 , 0 , 2652, 224, 1  , 0 ),
  INST(Shrd            , X86ShldShrd        , O(000F00,AC,_,_,x,_,_,_  ), 0                         , 0 , 0 , 2656, 238, 1  , 0 ),
  INST(Shrx            , VexRmv_Wx          , V(F20F38,F7,_,0,x,_,_,_  ), 0                         , 0 , 0 , 2661, 14 , 81 , 0 ),
  INST(Shufpd          , ExtRmi             , O(660F00,C6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9269, 16 , 4  , 83),
  INST(Shufps          , ExtRmi             , O(000F00,C6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9277, 16 , 5  , 83),
  INST(Sidt            , X86M_Only          , O(000F00,01,1,_,_,_,_,_  ), 0                         , 0 , 0 , 2666, 84 , 45 , 0 ),
  INST(Sldt            , X86M               , O(000F00,00,0,_,_,_,_,_  ), 0                         , 0 , 0 , 2671, 239, 45 , 0 ),
  INST(Smsw            , X86M               , O(000F00,01,4,_,_,_,_,_  ), 0                         , 0 , 0 , 2676, 239, 45 , 0 ),
  INST(Sqrtpd          , ExtRm              , O(660F00,51,_,_,_,_,_,_  ), 0                         , 0 , 16, 9285, 50 , 4  , 84),
  INST(Sqrtps          , ExtRm              , O(000F00,51,_,_,_,_,_,_  ), 0                         , 0 , 16, 9000, 50 , 5  , 84),
  INST(Sqrtsd          , ExtRm              , O(F20F00,51,_,_,_,_,_,_  ), 0                         , 0 , 8 , 9301, 240, 4  , 85),
  INST(Sqrtss          , ExtRm              , O(F30F00,51,_,_,_,_,_,_  ), 0                         , 0 , 4 , 9009, 225, 5  , 85),
  INST(Stac            , X86Op              , O(000F01,CB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2681, 34 , 17 , 0 ),
  INST(Stc             , X86Op              , O(000000,F9,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2686, 34 , 18 , 0 ),
  INST(Std             , X86Op              , O(000000,FD,_,_,_,_,_,_  ), 0                         , 0 , 0 , 6242, 34 , 19 , 0 ),
  INST(Sti             , X86Op              , O(000000,FB,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2690, 34 , 22 , 0 ),
  INST(Stmxcsr         , X86M_Only          , O(000F00,AE,3,_,_,_,_,_  ), 0                         , 0 , 0 , 9317, 241, 5  , 0 ),
  INST(Stos            , X86StrMr           , O(000000,AA,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2694, 242, 72 , 0 ),
  INST(Str             , X86M               , O(000F00,00,1,_,_,_,_,_  ), 0                         , 0 , 0 , 2699, 239, 45 , 0 ),
  INST(Sub             , X86Arith           , O(000000,28,5,_,x,_,_,_  ), 0                         , 0 , 0 , 807 , 243, 1  , 0 ),
  INST(Subpd           , ExtRm              , O(660F00,5C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4361, 5  , 4  , 86),
  INST(Subps           , ExtRm              , O(000F00,5C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 4373, 5  , 5  , 86),
  INST(Subsd           , ExtRm              , O(F20F00,5C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5049, 6  , 4  , 86),
  INST(Subss           , ExtRm              , O(F30F00,5C,_,_,_,_,_,_  ), 0                         , 0 , 0 , 5059, 7  , 5  , 86),
  INST(Swapgs          , X86Op              , O(000F01,F8,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2703, 101, 23 , 0 ),
  INST(Syscall         , X86Op              , O(000F00,05,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2710, 101, 45 , 0 ),
  INST(Sysenter        , X86Op              , O(000F00,34,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2718, 34 , 45 , 0 ),
  INST(Sysexit         , X86Op              , O(000F00,35,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2727, 34 , 23 , 0 ),
  INST(Sysexit64       , X86Op              , O(000F00,35,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2735, 34 , 23 , 0 ),
  INST(Sysret          , X86Op              , O(000F00,07,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2745, 101, 23 , 0 ),
  INST(Sysret64        , X86Op              , O(000F00,07,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2752, 101, 23 , 0 ),
  INST(T1mskc          , VexVm_Wx           , V(XOP_M9,01,7,0,x,_,_,_  ), 0                         , 0 , 0 , 2761, 15 , 11 , 0 ),
  INST(Test            , X86Test            , O(000000,84,_,_,x,_,_,_  ), O(000000,F6,_,_,x,_,_,_  ), 0 , 0 , 8473, 244, 1  , 0 ),
  INST(Tzcnt           , X86Rm_Raw66H       , O(F30F00,BC,_,_,x,_,_,_  ), 0                         , 0 , 0 , 2768, 143, 9  , 0 ),
  INST(Tzmsk           , VexVm_Wx           , V(XOP_M9,01,4,0,x,_,_,_  ), 0                         , 0 , 0 , 2774, 15 , 11 , 0 ),
  INST(Ucomisd         , ExtRm              , O(660F00,2E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9370, 44 , 39 , 15),
  INST(Ucomiss         , ExtRm              , O(000F00,2E,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9379, 45 , 40 , 15),
  INST(Ud2             , X86Op              , O(000F00,0B,_,_,_,_,_,_  ), 0                         , 0 , 0 , 2780, 34 , 0  , 0 ),
  INST(Unpckhpd        , ExtRm              , O(660F00,15,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9388, 5  , 4  , 13),
  INST(Unpckhps        , ExtRm              , O(000F00,15,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9398, 5  , 5  , 13),
  INST(Unpcklpd        , ExtRm              , O(660F00,14,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9408, 5  , 4  , 13),
  INST(Unpcklps        , ExtRm              , O(000F00,14,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9418, 5  , 5  , 13),
  INST(V4fmaddps       , VexRm_T1_4X        , V(F20F38,9A,_,2,_,0,2,T4X), 0                         , 0 , 0 , 2784, 245, 115, 0 ),
  INST(V4fnmaddps      , VexRm_T1_4X        , V(F20F38,AA,_,2,_,0,2,T4X), 0                         , 0 , 0 , 2794, 245, 115, 0 ),
  INST(Vaddpd          , VexRvm_Lx          , V(660F00,58,_,x,I,1,4,FV ), 0                         , 0 , 0 , 2805, 246, 116, 1 ),
  INST(Vaddps          , VexRvm_Lx          , V(000F00,58,_,x,I,0,4,FV ), 0                         , 0 , 0 , 2812, 247, 116, 1 ),
  INST(Vaddsd          , VexRvm             , V(F20F00,58,_,I,I,1,3,T1S), 0                         , 0 , 0 , 2819, 248, 117, 1 ),
  INST(Vaddss          , VexRvm             , V(F30F00,58,_,I,I,0,2,T1S), 0                         , 0 , 0 , 2826, 249, 117, 1 ),
  INST(Vaddsubpd       , VexRvm_Lx          , V(660F00,D0,_,x,I,_,_,_  ), 0                         , 0 , 0 , 2833, 250, 118, 1 ),
  INST(Vaddsubps       , VexRvm_Lx          , V(F20F00,D0,_,x,I,_,_,_  ), 0                         , 0 , 0 , 2843, 250, 118, 1 ),
  INST(Vaesdec         , VexRvm             , V(660F38,DE,_,0,I,_,_,_  ), 0                         , 0 , 0 , 2853, 251, 119, 2 ),
  INST(Vaesdeclast     , VexRvm             , V(660F38,DF,_,0,I,_,_,_  ), 0                         , 0 , 0 , 2861, 251, 119, 2 ),
  INST(Vaesenc         , VexRvm             , V(660F38,DC,_,0,I,_,_,_  ), 0                         , 0 , 0 , 2873, 251, 119, 2 ),
  INST(Vaesenclast     , VexRvm             , V(660F38,DD,_,0,I,_,_,_  ), 0                         , 0 , 0 , 2881, 251, 119, 2 ),
  INST(Vaesimc         , VexRm              , V(660F38,DB,_,0,I,_,_,_  ), 0                         , 0 , 0 , 2893, 252, 119, 3 ),
  INST(Vaeskeygenassist, VexRmi             , V(660F3A,DF,_,0,I,_,_,_  ), 0                         , 0 , 0 , 2901, 253, 119, 3 ),
  INST(Valignd         , VexRvmi_Lx         , V(660F3A,03,_,x,_,0,4,FV ), 0                         , 0 , 0 , 2918, 254, 120, 0 ),
  INST(Valignq         , VexRvmi_Lx         , V(660F3A,03,_,x,_,1,4,FV ), 0                         , 0 , 0 , 2926, 255, 120, 0 ),
  INST(Vandnpd         , VexRvm_Lx          , V(660F00,55,_,x,I,1,4,FV ), 0                         , 0 , 0 , 2934, 256, 121, 2 ),
  INST(Vandnps         , VexRvm_Lx          , V(000F00,55,_,x,I,0,4,FV ), 0                         , 0 , 0 , 2942, 257, 121, 2 ),
  INST(Vandpd          , VexRvm_Lx          , V(660F00,54,_,x,I,1,4,FV ), 0                         , 0 , 0 , 2950, 258, 121, 2 ),
  INST(Vandps          , VexRvm_Lx          , V(000F00,54,_,x,I,0,4,FV ), 0                         , 0 , 0 , 2957, 259, 121, 2 ),
  INST(Vblendmb        , VexRvm_Lx          , V(660F38,66,_,x,_,0,4,FVM), 0                         , 0 , 0 , 2964, 260, 122, 0 ),
  INST(Vblendmd        , VexRvm_Lx          , V(660F38,64,_,x,_,0,4,FV ), 0                         , 0 , 0 , 2973, 261, 120, 0 ),
  INST(Vblendmpd       , VexRvm_Lx          , V(660F38,65,_,x,_,1,4,FV ), 0                         , 0 , 0 , 2982, 262, 120, 0 ),
  INST(Vblendmps       , VexRvm_Lx          , V(660F38,65,_,x,_,0,4,FV ), 0                         , 0 , 0 , 2992, 261, 120, 0 ),
  INST(Vblendmq        , VexRvm_Lx          , V(660F38,64,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3002, 262, 120, 0 ),
  INST(Vblendmw        , VexRvm_Lx          , V(660F38,66,_,x,_,1,4,FVM), 0                         , 0 , 0 , 3011, 260, 122, 0 ),
  INST(Vblendpd        , VexRvmi_Lx         , V(660F3A,0D,_,x,I,_,_,_  ), 0                         , 0 , 0 , 3020, 263, 118, 4 ),
  INST(Vblendps        , VexRvmi_Lx         , V(660F3A,0C,_,x,I,_,_,_  ), 0                         , 0 , 0 , 3029, 263, 118, 4 ),
  INST(Vblendvpd       , VexRvmr_Lx         , V(660F3A,4B,_,x,0,_,_,_  ), 0                         , 0 , 0 , 3038, 264, 118, 5 ),
  INST(Vblendvps       , VexRvmr_Lx         , V(660F3A,4A,_,x,0,_,_,_  ), 0                         , 0 , 0 , 3048, 264, 118, 5 ),
  INST(Vbroadcastf128  , VexRm              , V(660F38,1A,_,1,0,_,_,_  ), 0                         , 0 , 0 , 3058, 265, 118, 0 ),
  INST(Vbroadcastf32x2 , VexRm_Lx           , V(660F38,19,_,x,_,0,3,T2 ), 0                         , 0 , 0 , 3073, 266, 123, 0 ),
  INST(Vbroadcastf32x4 , VexRm_Lx           , V(660F38,1A,_,x,_,0,4,T4 ), 0                         , 0 , 0 , 3089, 267, 65 , 0 ),
  INST(Vbroadcastf32x8 , VexRm              , V(660F38,1B,_,2,_,0,5,T8 ), 0                         , 0 , 0 , 3105, 268, 63 , 0 ),
  INST(Vbroadcastf64x2 , VexRm_Lx           , V(660F38,1A,_,x,_,1,4,T2 ), 0                         , 0 , 0 , 3121, 267, 123, 0 ),
  INST(Vbroadcastf64x4 , VexRm              , V(660F38,1B,_,2,_,1,5,T4 ), 0                         , 0 , 0 , 3137, 268, 65 , 0 ),
  INST(Vbroadcasti128  , VexRm              , V(660F38,5A,_,1,0,_,_,_  ), 0                         , 0 , 0 , 3153, 265, 124, 0 ),
  INST(Vbroadcasti32x2 , VexRm_Lx           , V(660F38,59,_,x,_,0,3,T2 ), 0                         , 0 , 0 , 3168, 269, 123, 0 ),
  INST(Vbroadcasti32x4 , VexRm_Lx           , V(660F38,5A,_,x,_,0,4,T4 ), 0                         , 0 , 0 , 3184, 266, 120, 0 ),
  INST(Vbroadcasti32x8 , VexRm              , V(660F38,5B,_,2,_,0,5,T8 ), 0                         , 0 , 0 , 3200, 270, 63 , 0 ),
  INST(Vbroadcasti64x2 , VexRm_Lx           , V(660F38,5A,_,x,_,1,4,T2 ), 0                         , 0 , 0 , 3216, 266, 123, 0 ),
  INST(Vbroadcasti64x4 , VexRm              , V(660F38,5B,_,2,_,1,5,T4 ), 0                         , 0 , 0 , 3232, 270, 65 , 0 ),
  INST(Vbroadcastsd    , VexRm_Lx           , V(660F38,19,_,x,0,1,3,T1S), 0                         , 0 , 0 , 3248, 271, 125, 0 ),
  INST(Vbroadcastss    , VexRm_Lx           , V(660F38,18,_,x,0,0,2,T1S), 0                         , 0 , 0 , 3261, 272, 125, 0 ),
  INST(Vcmppd          , VexRvmi_Lx         , V(660F00,C2,_,x,I,1,4,FV ), 0                         , 0 , 0 , 3274, 273, 116, 6 ),
  INST(Vcmpps          , VexRvmi_Lx         , V(000F00,C2,_,x,I,0,4,FV ), 0                         , 0 , 0 , 3281, 274, 116, 6 ),
  INST(Vcmpsd          , VexRvmi            , V(F20F00,C2,_,I,I,1,3,T1S), 0                         , 0 , 0 , 3288, 275, 117, 7 ),
  INST(Vcmpss          , VexRvmi            , V(F30F00,C2,_,I,I,0,2,T1S), 0                         , 0 , 0 , 3295, 276, 117, 7 ),
  INST(Vcomisd         , VexRm              , V(660F00,2F,_,I,I,1,3,T1S), 0                         , 0 , 0 , 3302, 277, 126, 8 ),
  INST(Vcomiss         , VexRm              , V(000F00,2F,_,I,I,0,2,T1S), 0                         , 0 , 0 , 3310, 278, 126, 8 ),
  INST(Vcompresspd     , VexMr_Lx           , V(660F38,8A,_,x,_,1,3,T1S), 0                         , 0 , 0 , 3318, 279, 120, 0 ),
  INST(Vcompressps     , VexMr_Lx           , V(660F38,8A,_,x,_,0,2,T1S), 0                         , 0 , 0 , 3330, 279, 120, 0 ),
  INST(Vcvtdq2pd       , VexRm_Lx           , V(F30F00,E6,_,x,I,0,3,HV ), 0                         , 0 , 0 , 3342, 280, 116, 9 ),
  INST(Vcvtdq2ps       , VexRm_Lx           , V(000F00,5B,_,x,I,0,4,FV ), 0                         , 0 , 0 , 3352, 281, 116, 9 ),
  INST(Vcvtpd2dq       , VexRm_Lx           , V(F20F00,E6,_,x,I,1,4,FV ), 0                         , 0 , 0 , 3362, 282, 116, 9 ),
  INST(Vcvtpd2ps       , VexRm_Lx           , V(660F00,5A,_,x,I,1,4,FV ), 0                         , 0 , 0 , 3372, 283, 116, 10),
  INST(Vcvtpd2qq       , VexRm_Lx           , V(660F00,7B,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3382, 284, 123, 0 ),
  INST(Vcvtpd2udq      , VexRm_Lx           , V(000F00,79,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3392, 285, 120, 0 ),
  INST(Vcvtpd2uqq      , VexRm_Lx           , V(660F00,79,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3403, 284, 123, 0 ),
  INST(Vcvtph2ps       , VexRm_Lx           , V(660F38,13,_,x,0,0,3,HVM), 0                         , 0 , 0 , 3414, 286, 127, 0 ),
  INST(Vcvtps2dq       , VexRm_Lx           , V(660F00,5B,_,x,I,0,4,FV ), 0                         , 0 , 0 , 3424, 281, 116, 8 ),
  INST(Vcvtps2pd       , VexRm_Lx           , V(000F00,5A,_,x,I,0,4,HV ), 0                         , 0 , 0 , 3434, 287, 116, 8 ),
  INST(Vcvtps2ph       , VexMri_Lx          , V(660F3A,1D,_,x,0,0,3,HVM), 0                         , 0 , 0 , 3444, 288, 127, 0 ),
  INST(Vcvtps2qq       , VexRm_Lx           , V(660F00,7B,_,x,_,0,3,HV ), 0                         , 0 , 0 , 3454, 289, 123, 0 ),
  INST(Vcvtps2udq      , VexRm_Lx           , V(000F00,79,_,x,_,0,4,FV ), 0                         , 0 , 0 , 3464, 290, 120, 0 ),
  INST(Vcvtps2uqq      , VexRm_Lx           , V(660F00,79,_,x,_,0,3,HV ), 0                         , 0 , 0 , 3475, 289, 123, 0 ),
  INST(Vcvtqq2pd       , VexRm_Lx           , V(F30F00,E6,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3486, 284, 123, 0 ),
  INST(Vcvtqq2ps       , VexRm_Lx           , V(000F00,5B,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3496, 285, 123, 0 ),
  INST(Vcvtsd2si       , VexRm_Wx           , V(F20F00,2D,_,I,x,x,3,T1F), 0                         , 0 , 0 , 3506, 291, 117, 11),
  INST(Vcvtsd2ss       , VexRvm             , V(F20F00,5A,_,I,I,1,3,T1S), 0                         , 0 , 0 , 3516, 248, 117, 12),
  INST(Vcvtsd2usi      , VexRm_Wx           , V(F20F00,79,_,I,_,x,3,T1F), 0                         , 0 , 0 , 3526, 292, 65 , 0 ),
  INST(Vcvtsi2sd       , VexRvm_Wx          , V(F20F00,2A,_,I,x,x,2,T1W), 0                         , 0 , 0 , 3537, 293, 117, 13),
  INST(Vcvtsi2ss       , VexRvm_Wx          , V(F30F00,2A,_,I,x,x,2,T1W), 0                         , 0 , 0 , 3547, 293, 117, 13),
  INST(Vcvtss2sd       , VexRvm             , V(F30F00,5A,_,I,I,0,2,T1S), 0                         , 0 , 0 , 3557, 294, 117, 13),
  INST(Vcvtss2si       , VexRm_Wx           , V(F30F00,2D,_,I,x,x,2,T1F), 0                         , 0 , 0 , 3567, 295, 117, 14),
  INST(Vcvtss2usi      , VexRm_Wx           , V(F30F00,79,_,I,_,x,2,T1F), 0                         , 0 , 0 , 3577, 296, 65 , 0 ),
  INST(Vcvttpd2dq      , VexRm_Lx           , V(660F00,E6,_,x,I,1,4,FV ), 0                         , 0 , 0 , 3588, 297, 116, 15),
  INST(Vcvttpd2qq      , VexRm_Lx           , V(660F00,7A,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3599, 298, 120, 0 ),
  INST(Vcvttpd2udq     , VexRm_Lx           , V(000F00,78,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3610, 299, 120, 0 ),
  INST(Vcvttpd2uqq     , VexRm_Lx           , V(660F00,78,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3622, 298, 123, 0 ),
  INST(Vcvttps2dq      , VexRm_Lx           , V(F30F00,5B,_,x,I,0,4,FV ), 0                         , 0 , 0 , 3634, 300, 116, 16),
  INST(Vcvttps2qq      , VexRm_Lx           , V(660F00,7A,_,x,_,0,3,HV ), 0                         , 0 , 0 , 3645, 301, 123, 0 ),
  INST(Vcvttps2udq     , VexRm_Lx           , V(000F00,78,_,x,_,0,4,FV ), 0                         , 0 , 0 , 3656, 302, 120, 0 ),
  INST(Vcvttps2uqq     , VexRm_Lx           , V(660F00,78,_,x,_,0,3,HV ), 0                         , 0 , 0 , 3668, 301, 123, 0 ),
  INST(Vcvttsd2si      , VexRm_Wx           , V(F20F00,2C,_,I,x,x,3,T1F), 0                         , 0 , 0 , 3680, 303, 117, 17),
  INST(Vcvttsd2usi     , VexRm_Wx           , V(F20F00,78,_,I,_,x,3,T1F), 0                         , 0 , 0 , 3691, 304, 65 , 0 ),
  INST(Vcvttss2si      , VexRm_Wx           , V(F30F00,2C,_,I,x,x,2,T1F), 0                         , 0 , 0 , 3703, 305, 117, 18),
  INST(Vcvttss2usi     , VexRm_Wx           , V(F30F00,78,_,I,_,x,2,T1F), 0                         , 0 , 0 , 3714, 306, 65 , 0 ),
  INST(Vcvtudq2pd      , VexRm_Lx           , V(F30F00,7A,_,x,_,0,3,HV ), 0                         , 0 , 0 , 3726, 307, 120, 0 ),
  INST(Vcvtudq2ps      , VexRm_Lx           , V(F20F00,7A,_,x,_,0,4,FV ), 0                         , 0 , 0 , 3737, 290, 120, 0 ),
  INST(Vcvtuqq2pd      , VexRm_Lx           , V(F30F00,7A,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3748, 284, 123, 0 ),
  INST(Vcvtuqq2ps      , VexRm_Lx           , V(F20F00,7A,_,x,_,1,4,FV ), 0                         , 0 , 0 , 3759, 285, 123, 0 ),
  INST(Vcvtusi2sd      , VexRvm_Wx          , V(F20F00,7B,_,I,_,x,2,T1W), 0                         , 0 , 0 , 3770, 308, 65 , 0 ),
  INST(Vcvtusi2ss      , VexRvm_Wx          , V(F30F00,7B,_,I,_,x,2,T1W), 0                         , 0 , 0 , 3781, 308, 65 , 0 ),
  INST(Vdbpsadbw       , VexRvmi_Lx         , V(660F3A,42,_,x,_,0,4,FVM), 0                         , 0 , 0 , 3792, 309, 122, 0 ),
  INST(Vdivpd          , VexRvm_Lx          , V(660F00,5E,_,x,I,1,4,FV ), 0                         , 0 , 0 , 3802, 246, 116, 19),
  INST(Vdivps          , VexRvm_Lx          , V(000F00,5E,_,x,I,0,4,FV ), 0                         , 0 , 0 , 3809, 247, 116, 19),
  INST(Vdivsd          , VexRvm             , V(F20F00,5E,_,I,I,1,3,T1S), 0                         , 0 , 0 , 3816, 248, 117, 19),
  INST(Vdivss          , VexRvm             , V(F30F00,5E,_,I,I,0,2,T1S), 0                         , 0 , 0 , 3823, 249, 117, 19),
  INST(Vdppd           , VexRvmi_Lx         , V(660F3A,41,_,x,I,_,_,_  ), 0                         , 0 , 0 , 3830, 310, 118, 19),
  INST(Vdpps           , VexRvmi_Lx         , V(660F3A,40,_,x,I,_,_,_  ), 0                         , 0 , 0 , 3836, 263, 118, 19),
  INST(Verr            , X86M               , O(000F00,00,4,_,_,_,_,_  ), 0                         , 0 , 0 , 3842, 137, 70 , 0 ),
  INST(Verw            , X86M               , O(000F00,00,5,_,_,_,_,_  ), 0                         , 0 , 0 , 3847, 137, 70 , 0 ),
  INST(Vexp2pd         , VexRm              , V(660F38,C8,_,2,_,1,4,FV ), 0                         , 0 , 0 , 3852, 311, 128, 0 ),
  INST(Vexp2ps         , VexRm              , V(660F38,C8,_,2,_,0,4,FV ), 0                         , 0 , 0 , 3860, 312, 128, 0 ),
  INST(Vexpandpd       , VexRm_Lx           , V(660F38,88,_,x,_,1,3,T1S), 0                         , 0 , 0 , 3868, 313, 120, 0 ),
  INST(Vexpandps       , VexRm_Lx           , V(660F38,88,_,x,_,0,2,T1S), 0                         , 0 , 0 , 3878, 313, 120, 0 ),
  INST(Vextractf128    , VexMri             , V(660F3A,19,_,1,0,_,_,_  ), 0                         , 0 , 0 , 3888, 314, 118, 0 ),
  INST(Vextractf32x4   , VexMri_Lx          , V(660F3A,19,_,x,_,0,4,T4 ), 0                         , 0 , 0 , 3901, 315, 120, 0 ),
  INST(Vextractf32x8   , VexMri             , V(660F3A,1B,_,2,_,0,5,T8 ), 0                         , 0 , 0 , 3915, 316, 63 , 0 ),
  INST(Vextractf64x2   , VexMri_Lx          , V(660F3A,19,_,x,_,1,4,T2 ), 0                         , 0 , 0 , 3929, 315, 123, 0 ),
  INST(Vextractf64x4   , VexMri             , V(660F3A,1B,_,2,_,1,5,T4 ), 0                         , 0 , 0 , 3943, 316, 65 , 0 ),
  INST(Vextracti128    , VexMri             , V(660F3A,39,_,1,0,_,_,_  ), 0                         , 0 , 0 , 3957, 314, 124, 0 ),
  INST(Vextracti32x4   , VexMri_Lx          , V(660F3A,39,_,x,_,0,4,T4 ), 0                         , 0 , 0 , 3970, 315, 120, 0 ),
  INST(Vextracti32x8   , VexMri             , V(660F3A,3B,_,2,_,0,5,T8 ), 0                         , 0 , 0 , 3984, 316, 63 , 0 ),
  INST(Vextracti64x2   , VexMri_Lx          , V(660F3A,39,_,x,_,1,4,T2 ), 0                         , 0 , 0 , 3998, 315, 123, 0 ),
  INST(Vextracti64x4   , VexMri             , V(660F3A,3B,_,2,_,1,5,T4 ), 0                         , 0 , 0 , 4012, 316, 65 , 0 ),
  INST(Vextractps      , VexMri             , V(660F3A,17,_,0,I,I,2,T1S), 0                         , 0 , 0 , 4026, 317, 117, 20),
  INST(Vfixupimmpd     , VexRvmi_Lx         , V(660F3A,54,_,x,_,1,4,FV ), 0                         , 0 , 0 , 4037, 318, 120, 0 ),
  INST(Vfixupimmps     , VexRvmi_Lx         , V(660F3A,54,_,x,_,0,4,FV ), 0                         , 0 , 0 , 4049, 319, 120, 0 ),
  INST(Vfixupimmsd     , VexRvmi            , V(660F3A,55,_,I,_,1,3,T1S), 0                         , 0 , 0 , 4061, 320, 65 , 0 ),
  INST(Vfixupimmss     , VexRvmi            , V(660F3A,55,_,I,_,0,2,T1S), 0                         , 0 , 0 , 4073, 321, 65 , 0 ),
  INST(Vfmadd132pd     , VexRvm_Lx          , V(660F38,98,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4085, 322, 129, 0 ),
  INST(Vfmadd132ps     , VexRvm_Lx          , V(660F38,98,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4097, 323, 129, 0 ),
  INST(Vfmadd132sd     , VexRvm             , V(660F38,99,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4109, 324, 130, 0 ),
  INST(Vfmadd132ss     , VexRvm             , V(660F38,99,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4121, 325, 130, 0 ),
  INST(Vfmadd213pd     , VexRvm_Lx          , V(660F38,A8,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4133, 322, 129, 0 ),
  INST(Vfmadd213ps     , VexRvm_Lx          , V(660F38,A8,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4145, 323, 129, 0 ),
  INST(Vfmadd213sd     , VexRvm             , V(660F38,A9,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4157, 324, 130, 0 ),
  INST(Vfmadd213ss     , VexRvm             , V(660F38,A9,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4169, 325, 130, 0 ),
  INST(Vfmadd231pd     , VexRvm_Lx          , V(660F38,B8,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4181, 322, 129, 0 ),
  INST(Vfmadd231ps     , VexRvm_Lx          , V(660F38,B8,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4193, 323, 129, 0 ),
  INST(Vfmadd231sd     , VexRvm             , V(660F38,B9,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4205, 324, 130, 0 ),
  INST(Vfmadd231ss     , VexRvm             , V(660F38,B9,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4217, 325, 130, 0 ),
  INST(Vfmaddpd        , Fma4_Lx            , V(660F3A,69,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4229, 326, 131, 0 ),
  INST(Vfmaddps        , Fma4_Lx            , V(660F3A,68,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4238, 326, 131, 0 ),
  INST(Vfmaddsd        , Fma4               , V(660F3A,6B,_,0,x,_,_,_  ), 0                         , 0 , 0 , 4247, 327, 131, 0 ),
  INST(Vfmaddss        , Fma4               , V(660F3A,6A,_,0,x,_,_,_  ), 0                         , 0 , 0 , 4256, 328, 131, 0 ),
  INST(Vfmaddsub132pd  , VexRvm_Lx          , V(660F38,96,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4265, 322, 129, 0 ),
  INST(Vfmaddsub132ps  , VexRvm_Lx          , V(660F38,96,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4280, 323, 129, 0 ),
  INST(Vfmaddsub213pd  , VexRvm_Lx          , V(660F38,A6,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4295, 322, 129, 0 ),
  INST(Vfmaddsub213ps  , VexRvm_Lx          , V(660F38,A6,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4310, 323, 129, 0 ),
  INST(Vfmaddsub231pd  , VexRvm_Lx          , V(660F38,B6,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4325, 322, 129, 0 ),
  INST(Vfmaddsub231ps  , VexRvm_Lx          , V(660F38,B6,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4340, 323, 129, 0 ),
  INST(Vfmaddsubpd     , Fma4_Lx            , V(660F3A,5D,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4355, 326, 131, 0 ),
  INST(Vfmaddsubps     , Fma4_Lx            , V(660F3A,5C,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4367, 326, 131, 0 ),
  INST(Vfmsub132pd     , VexRvm_Lx          , V(660F38,9A,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4379, 322, 129, 0 ),
  INST(Vfmsub132ps     , VexRvm_Lx          , V(660F38,9A,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4391, 323, 129, 0 ),
  INST(Vfmsub132sd     , VexRvm             , V(660F38,9B,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4403, 324, 130, 0 ),
  INST(Vfmsub132ss     , VexRvm             , V(660F38,9B,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4415, 325, 130, 0 ),
  INST(Vfmsub213pd     , VexRvm_Lx          , V(660F38,AA,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4427, 322, 129, 0 ),
  INST(Vfmsub213ps     , VexRvm_Lx          , V(660F38,AA,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4439, 323, 129, 0 ),
  INST(Vfmsub213sd     , VexRvm             , V(660F38,AB,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4451, 324, 130, 0 ),
  INST(Vfmsub213ss     , VexRvm             , V(660F38,AB,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4463, 325, 130, 0 ),
  INST(Vfmsub231pd     , VexRvm_Lx          , V(660F38,BA,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4475, 322, 129, 0 ),
  INST(Vfmsub231ps     , VexRvm_Lx          , V(660F38,BA,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4487, 323, 129, 0 ),
  INST(Vfmsub231sd     , VexRvm             , V(660F38,BB,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4499, 324, 130, 0 ),
  INST(Vfmsub231ss     , VexRvm             , V(660F38,BB,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4511, 325, 130, 0 ),
  INST(Vfmsubadd132pd  , VexRvm_Lx          , V(660F38,97,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4523, 322, 129, 0 ),
  INST(Vfmsubadd132ps  , VexRvm_Lx          , V(660F38,97,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4538, 323, 129, 0 ),
  INST(Vfmsubadd213pd  , VexRvm_Lx          , V(660F38,A7,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4553, 322, 129, 0 ),
  INST(Vfmsubadd213ps  , VexRvm_Lx          , V(660F38,A7,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4568, 323, 129, 0 ),
  INST(Vfmsubadd231pd  , VexRvm_Lx          , V(660F38,B7,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4583, 322, 129, 0 ),
  INST(Vfmsubadd231ps  , VexRvm_Lx          , V(660F38,B7,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4598, 323, 129, 0 ),
  INST(Vfmsubaddpd     , Fma4_Lx            , V(660F3A,5F,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4613, 326, 131, 0 ),
  INST(Vfmsubaddps     , Fma4_Lx            , V(660F3A,5E,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4625, 326, 131, 0 ),
  INST(Vfmsubpd        , Fma4_Lx            , V(660F3A,6D,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4637, 326, 131, 0 ),
  INST(Vfmsubps        , Fma4_Lx            , V(660F3A,6C,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4646, 326, 131, 0 ),
  INST(Vfmsubsd        , Fma4               , V(660F3A,6F,_,0,x,_,_,_  ), 0                         , 0 , 0 , 4655, 327, 131, 0 ),
  INST(Vfmsubss        , Fma4               , V(660F3A,6E,_,0,x,_,_,_  ), 0                         , 0 , 0 , 4664, 328, 131, 0 ),
  INST(Vfnmadd132pd    , VexRvm_Lx          , V(660F38,9C,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4673, 322, 129, 0 ),
  INST(Vfnmadd132ps    , VexRvm_Lx          , V(660F38,9C,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4686, 323, 129, 0 ),
  INST(Vfnmadd132sd    , VexRvm             , V(660F38,9D,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4699, 324, 130, 0 ),
  INST(Vfnmadd132ss    , VexRvm             , V(660F38,9D,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4712, 325, 130, 0 ),
  INST(Vfnmadd213pd    , VexRvm_Lx          , V(660F38,AC,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4725, 322, 129, 0 ),
  INST(Vfnmadd213ps    , VexRvm_Lx          , V(660F38,AC,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4738, 323, 129, 0 ),
  INST(Vfnmadd213sd    , VexRvm             , V(660F38,AD,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4751, 324, 130, 0 ),
  INST(Vfnmadd213ss    , VexRvm             , V(660F38,AD,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4764, 325, 130, 0 ),
  INST(Vfnmadd231pd    , VexRvm_Lx          , V(660F38,BC,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4777, 322, 129, 0 ),
  INST(Vfnmadd231ps    , VexRvm_Lx          , V(660F38,BC,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4790, 323, 129, 0 ),
  INST(Vfnmadd231sd    , VexRvm             , V(660F38,BC,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4803, 324, 130, 0 ),
  INST(Vfnmadd231ss    , VexRvm             , V(660F38,BC,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4816, 325, 130, 0 ),
  INST(Vfnmaddpd       , Fma4_Lx            , V(660F3A,79,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4829, 326, 131, 0 ),
  INST(Vfnmaddps       , Fma4_Lx            , V(660F3A,78,_,x,x,_,_,_  ), 0                         , 0 , 0 , 4839, 326, 131, 0 ),
  INST(Vfnmaddsd       , Fma4               , V(660F3A,7B,_,0,x,_,_,_  ), 0                         , 0 , 0 , 4849, 327, 131, 0 ),
  INST(Vfnmaddss       , Fma4               , V(660F3A,7A,_,0,x,_,_,_  ), 0                         , 0 , 0 , 4859, 328, 131, 0 ),
  INST(Vfnmsub132pd    , VexRvm_Lx          , V(660F38,9E,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4869, 322, 129, 0 ),
  INST(Vfnmsub132ps    , VexRvm_Lx          , V(660F38,9E,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4882, 323, 129, 0 ),
  INST(Vfnmsub132sd    , VexRvm             , V(660F38,9F,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4895, 324, 130, 0 ),
  INST(Vfnmsub132ss    , VexRvm             , V(660F38,9F,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4908, 325, 130, 0 ),
  INST(Vfnmsub213pd    , VexRvm_Lx          , V(660F38,AE,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4921, 322, 129, 0 ),
  INST(Vfnmsub213ps    , VexRvm_Lx          , V(660F38,AE,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4934, 323, 129, 0 ),
  INST(Vfnmsub213sd    , VexRvm             , V(660F38,AF,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4947, 324, 130, 0 ),
  INST(Vfnmsub213ss    , VexRvm             , V(660F38,AF,_,I,0,0,2,T1S), 0                         , 0 , 0 , 4960, 325, 130, 0 ),
  INST(Vfnmsub231pd    , VexRvm_Lx          , V(660F38,BE,_,x,1,1,4,FV ), 0                         , 0 , 0 , 4973, 322, 129, 0 ),
  INST(Vfnmsub231ps    , VexRvm_Lx          , V(660F38,BE,_,x,0,0,4,FV ), 0                         , 0 , 0 , 4986, 323, 129, 0 ),
  INST(Vfnmsub231sd    , VexRvm             , V(660F38,BF,_,I,1,1,3,T1S), 0                         , 0 , 0 , 4999, 324, 130, 0 ),
  INST(Vfnmsub231ss    , VexRvm             , V(660F38,BF,_,I,0,0,2,T1S), 0                         , 0 , 0 , 5012, 325, 130, 0 ),
  INST(Vfnmsubpd       , Fma4_Lx            , V(660F3A,7D,_,x,x,_,_,_  ), 0                         , 0 , 0 , 5025, 326, 131, 0 ),
  INST(Vfnmsubps       , Fma4_Lx            , V(660F3A,7C,_,x,x,_,_,_  ), 0                         , 0 , 0 , 5035, 326, 131, 0 ),
  INST(Vfnmsubsd       , Fma4               , V(660F3A,7F,_,0,x,_,_,_  ), 0                         , 0 , 0 , 5045, 327, 131, 0 ),
  INST(Vfnmsubss       , Fma4               , V(660F3A,7E,_,0,x,_,_,_  ), 0                         , 0 , 0 , 5055, 328, 131, 0 ),
  INST(Vfpclasspd      , VexRmi_Lx          , V(660F3A,66,_,x,_,1,4,FV ), 0                         , 0 , 0 , 5065, 329, 123, 0 ),
  INST(Vfpclassps      , VexRmi_Lx          , V(660F3A,66,_,x,_,0,4,FV ), 0                         , 0 , 0 , 5076, 330, 123, 0 ),
  INST(Vfpclasssd      , VexRmi_Lx          , V(660F3A,67,_,I,_,1,3,T1S), 0                         , 0 , 0 , 5087, 331, 63 , 0 ),
  INST(Vfpclassss      , VexRmi_Lx          , V(660F3A,67,_,I,_,0,2,T1S), 0                         , 0 , 0 , 5098, 332, 63 , 0 ),
  INST(Vfrczpd         , VexRm_Lx           , V(XOP_M9,81,_,x,0,_,_,_  ), 0                         , 0 , 0 , 5109, 333, 132, 0 ),
  INST(Vfrczps         , VexRm_Lx           , V(XOP_M9,80,_,x,0,_,_,_  ), 0                         , 0 , 0 , 5117, 333, 132, 0 ),
  INST(Vfrczsd         , VexRm              , V(XOP_M9,83,_,0,0,_,_,_  ), 0                         , 0 , 0 , 5125, 334, 132, 0 ),
  INST(Vfrczss         , VexRm              , V(XOP_M9,82,_,0,0,_,_,_  ), 0                         , 0 , 0 , 5133, 335, 132, 0 ),
  INST(Vgatherdpd      , VexRmvRm_VM        , V(660F38,92,_,x,1,_,_,_  ), V(660F38,92,_,x,_,1,3,T1S), 0 , 0 , 5141, 336, 133, 0 ),
  INST(Vgatherdps      , VexRmvRm_VM        , V(660F38,92,_,x,0,_,_,_  ), V(660F38,92,_,x,_,0,2,T1S), 0 , 0 , 5152, 337, 133, 0 ),
  INST(Vgatherpf0dpd   , VexM_VM            , V(660F38,C6,1,2,_,1,3,T1S), 0                         , 0 , 0 , 5163, 338, 134, 0 ),
  INST(Vgatherpf0dps   , VexM_VM            , V(660F38,C6,1,2,_,0,2,T1S), 0                         , 0 , 0 , 5177, 339, 134, 0 ),
  INST(Vgatherpf0qpd   , VexM_VM            , V(660F38,C7,1,2,_,1,3,T1S), 0                         , 0 , 0 , 5191, 340, 134, 0 ),
  INST(Vgatherpf0qps   , VexM_VM            , V(660F38,C7,1,2,_,0,2,T1S), 0                         , 0 , 0 , 5205, 340, 134, 0 ),
  INST(Vgatherpf1dpd   , VexM_VM            , V(660F38,C6,2,2,_,1,3,T1S), 0                         , 0 , 0 , 5219, 338, 134, 0 ),
  INST(Vgatherpf1dps   , VexM_VM            , V(660F38,C6,2,2,_,0,2,T1S), 0                         , 0 , 0 , 5233, 339, 134, 0 ),
  INST(Vgatherpf1qpd   , VexM_VM            , V(660F38,C7,2,2,_,1,3,T1S), 0                         , 0 , 0 , 5247, 340, 134, 0 ),
  INST(Vgatherpf1qps   , VexM_VM            , V(660F38,C7,2,2,_,0,2,T1S), 0                         , 0 , 0 , 5261, 340, 134, 0 ),
  INST(Vgatherqpd      , VexRmvRm_VM        , V(660F38,93,_,x,1,_,_,_  ), V(660F38,93,_,x,_,1,3,T1S), 0 , 0 , 5275, 341, 133, 0 ),
  INST(Vgatherqps      , VexRmvRm_VM        , V(660F38,93,_,x,0,_,_,_  ), V(660F38,93,_,x,_,0,2,T1S), 0 , 0 , 5286, 342, 133, 0 ),
  INST(Vgetexppd       , VexRm_Lx           , V(660F38,42,_,x,_,1,4,FV ), 0                         , 0 , 0 , 5297, 298, 120, 0 ),
  INST(Vgetexpps       , VexRm_Lx           , V(660F38,42,_,x,_,0,4,FV ), 0                         , 0 , 0 , 5307, 302, 120, 0 ),
  INST(Vgetexpsd       , VexRm              , V(660F38,43,_,I,_,1,3,T1S), 0                         , 0 , 0 , 5317, 343, 65 , 0 ),
  INST(Vgetexpss       , VexRm              , V(660F38,43,_,I,_,0,2,T1S), 0                         , 0 , 0 , 5327, 344, 65 , 0 ),
  INST(Vgetmantpd      , VexRmi_Lx          , V(660F3A,26,_,x,_,1,4,FV ), 0                         , 0 , 0 , 5337, 345, 120, 0 ),
  INST(Vgetmantps      , VexRmi_Lx          , V(660F3A,26,_,x,_,0,4,FV ), 0                         , 0 , 0 , 5348, 346, 120, 0 ),
  INST(Vgetmantsd      , VexRmi             , V(660F3A,27,_,I,_,1,3,T1S), 0                         , 0 , 0 , 5359, 347, 65 , 0 ),
  INST(Vgetmantss      , VexRmi             , V(660F3A,27,_,I,_,0,2,T1S), 0                         , 0 , 0 , 5370, 348, 65 , 0 ),
  INST(Vhaddpd         , VexRvm_Lx          , V(660F00,7C,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5381, 250, 118, 21),
  INST(Vhaddps         , VexRvm_Lx          , V(F20F00,7C,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5389, 250, 118, 21),
  INST(Vhsubpd         , VexRvm_Lx          , V(660F00,7D,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5397, 250, 118, 22),
  INST(Vhsubps         , VexRvm_Lx          , V(F20F00,7D,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5405, 250, 118, 22),
  INST(Vinsertf128     , VexRvmi            , V(660F3A,18,_,1,0,_,_,_  ), 0                         , 0 , 0 , 5413, 349, 118, 0 ),
  INST(Vinsertf32x4    , VexRvmi_Lx         , V(660F3A,18,_,x,_,0,4,T4 ), 0                         , 0 , 0 , 5425, 350, 120, 0 ),
  INST(Vinsertf32x8    , VexRvmi            , V(660F3A,1A,_,2,_,0,5,T8 ), 0                         , 0 , 0 , 5438, 351, 63 , 0 ),
  INST(Vinsertf64x2    , VexRvmi_Lx         , V(660F3A,18,_,x,_,1,4,T2 ), 0                         , 0 , 0 , 5451, 350, 123, 0 ),
  INST(Vinsertf64x4    , VexRvmi            , V(660F3A,1A,_,2,_,1,5,T4 ), 0                         , 0 , 0 , 5464, 351, 65 , 0 ),
  INST(Vinserti128     , VexRvmi            , V(660F3A,38,_,1,0,_,_,_  ), 0                         , 0 , 0 , 5477, 349, 124, 0 ),
  INST(Vinserti32x4    , VexRvmi_Lx         , V(660F3A,38,_,x,_,0,4,T4 ), 0                         , 0 , 0 , 5489, 350, 120, 0 ),
  INST(Vinserti32x8    , VexRvmi            , V(660F3A,3A,_,2,_,0,5,T8 ), 0                         , 0 , 0 , 5502, 351, 63 , 0 ),
  INST(Vinserti64x2    , VexRvmi_Lx         , V(660F3A,38,_,x,_,1,4,T2 ), 0                         , 0 , 0 , 5515, 350, 123, 0 ),
  INST(Vinserti64x4    , VexRvmi            , V(660F3A,3A,_,2,_,1,5,T4 ), 0                         , 0 , 0 , 5528, 351, 65 , 0 ),
  INST(Vinsertps       , VexRvmi            , V(660F3A,21,_,0,I,0,2,T1S), 0                         , 0 , 0 , 5541, 352, 117, 23),
  INST(Vlddqu          , VexRm_Lx           , V(F20F00,F0,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5551, 353, 118, 24),
  INST(Vldmxcsr        , VexM               , V(000F00,AE,2,0,I,_,_,_  ), 0                         , 0 , 0 , 5558, 354, 118, 0 ),
  INST(Vmaskmovdqu     , VexRm_ZDI          , V(660F00,F7,_,0,I,_,_,_  ), 0                         , 0 , 0 , 5567, 355, 118, 25),
  INST(Vmaskmovpd      , VexRvmMvr_Lx       , V(660F38,2D,_,x,0,_,_,_  ), V(660F38,2F,_,x,0,_,_,_  ), 0 , 0 , 5579, 356, 118, 0 ),
  INST(Vmaskmovps      , VexRvmMvr_Lx       , V(660F38,2C,_,x,0,_,_,_  ), V(660F38,2E,_,x,0,_,_,_  ), 0 , 0 , 5590, 357, 118, 0 ),
  INST(Vmaxpd          , VexRvm_Lx          , V(660F00,5F,_,x,I,1,4,FV ), 0                         , 0 , 0 , 5601, 358, 116, 26),
  INST(Vmaxps          , VexRvm_Lx          , V(000F00,5F,_,x,I,0,4,FV ), 0                         , 0 , 0 , 5608, 359, 116, 26),
  INST(Vmaxsd          , VexRvm             , V(F20F00,5F,_,I,I,1,3,T1S), 0                         , 0 , 0 , 5615, 360, 116, 26),
  INST(Vmaxss          , VexRvm             , V(F30F00,5F,_,I,I,0,2,T1S), 0                         , 0 , 0 , 5622, 294, 116, 26),
  INST(Vminpd          , VexRvm_Lx          , V(660F00,5D,_,x,I,1,4,FV ), 0                         , 0 , 0 , 5629, 358, 116, 27),
  INST(Vminps          , VexRvm_Lx          , V(000F00,5D,_,x,I,0,4,FV ), 0                         , 0 , 0 , 5636, 359, 116, 27),
  INST(Vminsd          , VexRvm             , V(F20F00,5D,_,I,I,1,3,T1S), 0                         , 0 , 0 , 5643, 360, 116, 27),
  INST(Vminss          , VexRvm             , V(F30F00,5D,_,I,I,0,2,T1S), 0                         , 0 , 0 , 5650, 294, 116, 27),
  INST(Vmovapd         , VexRmMr_Lx         , V(660F00,28,_,x,I,1,4,FVM), V(660F00,29,_,x,I,1,4,FVM), 0 , 0 , 5657, 361, 116, 28),
  INST(Vmovaps         , VexRmMr_Lx         , V(000F00,28,_,x,I,0,4,FVM), V(000F00,29,_,x,I,0,4,FVM), 0 , 0 , 5665, 362, 116, 28),
  INST(Vmovd           , VexMovdMovq        , V(660F00,6E,_,0,0,0,2,T1S), V(660F00,7E,_,0,0,0,2,T1S), 0 , 0 , 5673, 363, 117, 29),
  INST(Vmovddup        , VexRm_Lx           , V(F20F00,12,_,x,I,1,3,DUP), 0                         , 0 , 0 , 5679, 364, 116, 29),
  INST(Vmovdqa         , VexRmMr_Lx         , V(660F00,6F,_,x,I,_,_,_  ), V(660F00,7F,_,x,I,_,_,_  ), 0 , 0 , 5688, 365, 118, 30),
  INST(Vmovdqa32       , VexRmMr_Lx         , V(660F00,6F,_,x,_,0,4,FVM), V(660F00,7F,_,x,_,0,4,FVM), 0 , 0 , 5696, 366, 120, 0 ),
  INST(Vmovdqa64       , VexRmMr_Lx         , V(660F00,6F,_,x,_,1,4,FVM), V(660F00,7F,_,x,_,1,4,FVM), 0 , 0 , 5706, 367, 120, 0 ),
  INST(Vmovdqu         , VexRmMr_Lx         , V(F30F00,6F,_,x,I,_,_,_  ), V(F30F00,7F,_,x,I,_,_,_  ), 0 , 0 , 5716, 368, 118, 28),
  INST(Vmovdqu16       , VexRmMr_Lx         , V(F20F00,6F,_,x,_,1,4,FVM), V(F20F00,7F,_,x,_,1,4,FVM), 0 , 0 , 5724, 369, 122, 0 ),
  INST(Vmovdqu32       , VexRmMr_Lx         , V(F30F00,6F,_,x,_,0,4,FVM), V(F30F00,7F,_,x,_,0,4,FVM), 0 , 0 , 5734, 370, 120, 0 ),
  INST(Vmovdqu64       , VexRmMr_Lx         , V(F30F00,6F,_,x,_,1,4,FVM), V(F30F00,7F,_,x,_,1,4,FVM), 0 , 0 , 5744, 371, 120, 0 ),
  INST(Vmovdqu8        , VexRmMr_Lx         , V(F20F00,6F,_,x,_,0,4,FVM), V(F20F00,7F,_,x,_,0,4,FVM), 0 , 0 , 5754, 372, 122, 0 ),
  INST(Vmovhlps        , VexRvm             , V(000F00,12,_,0,I,0,_,_  ), 0                         , 0 , 0 , 5763, 373, 117, 31),
  INST(Vmovhpd         , VexRvmMr           , V(660F00,16,_,0,I,1,3,T1S), V(660F00,17,_,0,I,1,3,T1S), 0 , 0 , 5772, 374, 117, 32),
  INST(Vmovhps         , VexRvmMr           , V(000F00,16,_,0,I,0,3,T2 ), V(000F00,17,_,0,I,0,3,T2 ), 0 , 0 , 5780, 375, 117, 32),
  INST(Vmovlhps        , VexRvm             , V(000F00,16,_,0,I,0,_,_  ), 0                         , 0 , 0 , 5788, 373, 117, 31),
  INST(Vmovlpd         , VexRvmMr           , V(660F00,12,_,0,I,1,3,T1S), V(660F00,13,_,0,I,1,3,T1S), 0 , 0 , 5797, 376, 117, 32),
  INST(Vmovlps         , VexRvmMr           , V(000F00,12,_,0,I,0,3,T2 ), V(000F00,13,_,0,I,0,3,T2 ), 0 , 0 , 5805, 377, 117, 32),
  INST(Vmovmskpd       , VexRm_Lx           , V(660F00,50,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5813, 378, 118, 33),
  INST(Vmovmskps       , VexRm_Lx           , V(000F00,50,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5823, 378, 118, 33),
  INST(Vmovntdq        , VexMr_Lx           , V(660F00,E7,_,x,I,0,4,FVM), 0                         , 0 , 0 , 5833, 379, 116, 33),
  INST(Vmovntdqa       , VexRm_Lx           , V(660F38,2A,_,x,I,0,4,FVM), 0                         , 0 , 0 , 5842, 380, 125, 33),
  INST(Vmovntpd        , VexMr_Lx           , V(660F00,2B,_,x,I,1,4,FVM), 0                         , 0 , 0 , 5852, 379, 116, 34),
  INST(Vmovntps        , VexMr_Lx           , V(000F00,2B,_,x,I,0,4,FVM), 0                         , 0 , 0 , 5861, 379, 116, 34),
  INST(Vmovq           , VexMovdMovq        , V(660F00,6E,_,0,I,1,3,T1S), V(660F00,7E,_,0,I,1,3,T1S), 0 , 0 , 5870, 381, 117, 28),
  INST(Vmovsd          , VexMovssMovsd      , V(F20F00,10,_,I,I,1,3,T1S), V(F20F00,11,_,I,I,1,3,T1S), 0 , 0 , 5876, 382, 117, 35),
  INST(Vmovshdup       , VexRm_Lx           , V(F30F00,16,_,x,I,0,4,FVM), 0                         , 0 , 0 , 5883, 383, 116, 30),
  INST(Vmovsldup       , VexRm_Lx           , V(F30F00,12,_,x,I,0,4,FVM), 0                         , 0 , 0 , 5893, 383, 116, 30),
  INST(Vmovss          , VexMovssMovsd      , V(F30F00,10,_,I,I,0,2,T1S), V(F30F00,11,_,I,I,0,2,T1S), 0 , 0 , 5903, 384, 117, 35),
  INST(Vmovupd         , VexRmMr_Lx         , V(660F00,10,_,x,I,1,4,FVM), V(660F00,11,_,x,I,1,4,FVM), 0 , 0 , 5910, 385, 116, 36),
  INST(Vmovups         , VexRmMr_Lx         , V(000F00,10,_,x,I,0,4,FVM), V(000F00,11,_,x,I,0,4,FVM), 0 , 0 , 5918, 386, 116, 36),
  INST(Vmpsadbw        , VexRvmi_Lx         , V(660F3A,42,_,x,I,_,_,_  ), 0                         , 0 , 0 , 5926, 263, 135, 37),
  INST(Vmulpd          , VexRvm_Lx          , V(660F00,59,_,x,I,1,4,FV ), 0                         , 0 , 0 , 5935, 246, 116, 38),
  INST(Vmulps          , VexRvm_Lx          , V(000F00,59,_,x,I,0,4,FV ), 0                         , 0 , 0 , 5942, 247, 116, 38),
  INST(Vmulsd          , VexRvm_Lx          , V(F20F00,59,_,I,I,1,3,T1S), 0                         , 0 , 0 , 5949, 248, 117, 38),
  INST(Vmulss          , VexRvm_Lx          , V(F30F00,59,_,I,I,0,2,T1S), 0                         , 0 , 0 , 5956, 249, 117, 38),
  INST(Vorpd           , VexRvm_Lx          , V(660F00,56,_,x,I,1,4,FV ), 0                         , 0 , 0 , 5963, 258, 121, 39),
  INST(Vorps           , VexRvm_Lx          , V(000F00,56,_,x,I,0,4,FV ), 0                         , 0 , 0 , 5969, 259, 116, 39),
  INST(Vp4dpwssd       , VexRm_T1_4X        , V(F20F38,52,_,2,_,0,2,T4X), 0                         , 0 , 0 , 5975, 387, 136, 0 ),
  INST(Vp4dpwssds      , VexRm_T1_4X        , V(F20F38,53,_,2,_,0,2,T4X), 0                         , 0 , 0 , 5985, 387, 136, 0 ),
  INST(Vpabsb          , VexRm_Lx           , V(660F38,1C,_,x,I,_,4,FVM), 0                         , 0 , 0 , 5996, 383, 137, 40),
  INST(Vpabsd          , VexRm_Lx           , V(660F38,1E,_,x,I,0,4,FV ), 0                         , 0 , 0 , 6003, 383, 125, 40),
  INST(Vpabsq          , VexRm_Lx           , V(660F38,1F,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6010, 313, 120, 0 ),
  INST(Vpabsw          , VexRm_Lx           , V(660F38,1D,_,x,I,_,4,FVM), 0                         , 0 , 0 , 6017, 383, 137, 41),
  INST(Vpackssdw       , VexRvm_Lx          , V(660F00,6B,_,x,I,0,4,FV ), 0                         , 0 , 0 , 6024, 257, 137, 42),
  INST(Vpacksswb       , VexRvm_Lx          , V(660F00,63,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6034, 388, 137, 42),
  INST(Vpackusdw       , VexRvm_Lx          , V(660F38,2B,_,x,I,0,4,FV ), 0                         , 0 , 0 , 6044, 257, 137, 42),
  INST(Vpackuswb       , VexRvm_Lx          , V(660F00,67,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6054, 388, 137, 42),
  INST(Vpaddb          , VexRvm_Lx          , V(660F00,FC,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6064, 388, 137, 42),
  INST(Vpaddd          , VexRvm_Lx          , V(660F00,FE,_,x,I,0,4,FV ), 0                         , 0 , 0 , 6071, 257, 125, 42),
  INST(Vpaddq          , VexRvm_Lx          , V(660F00,D4,_,x,I,1,4,FV ), 0                         , 0 , 0 , 6078, 256, 125, 42),
  INST(Vpaddsb         , VexRvm_Lx          , V(660F00,EC,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6085, 388, 137, 42),
  INST(Vpaddsw         , VexRvm_Lx          , V(660F00,ED,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6093, 388, 137, 42),
  INST(Vpaddusb        , VexRvm_Lx          , V(660F00,DC,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6101, 388, 137, 42),
  INST(Vpaddusw        , VexRvm_Lx          , V(660F00,DD,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6110, 388, 137, 42),
  INST(Vpaddw          , VexRvm_Lx          , V(660F00,FD,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6119, 388, 137, 42),
  INST(Vpalignr        , VexRvmi_Lx         , V(660F3A,0F,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6126, 389, 137, 42),
  INST(Vpand           , VexRvm_Lx          , V(660F00,DB,_,x,I,_,_,_  ), 0                         , 0 , 0 , 6135, 390, 135, 42),
  INST(Vpandd          , VexRvm_Lx          , V(660F00,DB,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6141, 391, 120, 0 ),
  INST(Vpandn          , VexRvm_Lx          , V(660F00,DF,_,x,I,_,_,_  ), 0                         , 0 , 0 , 6148, 392, 135, 43),
  INST(Vpandnd         , VexRvm_Lx          , V(660F00,DF,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6155, 393, 120, 0 ),
  INST(Vpandnq         , VexRvm_Lx          , V(660F00,DF,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6163, 394, 120, 0 ),
  INST(Vpandq          , VexRvm_Lx          , V(660F00,DB,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6171, 395, 120, 0 ),
  INST(Vpavgb          , VexRvm_Lx          , V(660F00,E0,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6178, 388, 137, 44),
  INST(Vpavgw          , VexRvm_Lx          , V(660F00,E3,_,x,I,I,4,FVM), 0                         , 0 , 0 , 6185, 388, 137, 45),
  INST(Vpblendd        , VexRvmi_Lx         , V(660F3A,02,_,x,0,_,_,_  ), 0                         , 0 , 0 , 6192, 263, 124, 0 ),
  INST(Vpblendvb       , VexRvmr            , V(660F3A,4C,_,x,0,_,_,_  ), 0                         , 0 , 0 , 6201, 264, 135, 46),
  INST(Vpblendw        , VexRvmi_Lx         , V(660F3A,0E,_,x,I,_,_,_  ), 0                         , 0 , 0 , 6211, 263, 135, 44),
  INST(Vpbroadcastb    , VexRm_Lx           , V(660F38,78,_,x,0,0,0,T1S), 0                         , 0 , 0 , 6220, 396, 138, 0 ),
  INST(Vpbroadcastd    , VexRm_Lx           , V(660F38,58,_,x,0,0,2,T1S), 0                         , 0 , 0 , 6233, 397, 133, 0 ),
  INST(Vpbroadcastmb2d , VexRm_Lx           , V(F30F38,3A,_,x,_,0,_,_  ), 0                         , 0 , 0 , 6246, 398, 139, 0 ),
  INST(Vpbroadcastmb2q , VexRm_Lx           , V(F30F38,2A,_,x,_,1,_,_  ), 0                         , 0 , 0 , 6262, 398, 139, 0 ),
  INST(Vpbroadcastq    , VexRm_Lx           , V(660F38,59,_,x,0,1,3,T1S), 0                         , 0 , 0 , 6278, 399, 133, 0 ),
  INST(Vpbroadcastw    , VexRm_Lx           , V(660F38,79,_,x,0,0,1,T1S), 0                         , 0 , 0 , 6291, 400, 138, 0 ),
  INST(Vpclmulqdq      , VexRvmi            , V(660F3A,44,_,0,I,_,_,_  ), 0                         , 0 , 0 , 6304, 310, 140, 47),
  INST(Vpcmov          , VexRvrmRvmr_Lx     , V(XOP_M8,A2,_,x,x,_,_,_  ), 0                         , 0 , 0 , 6315, 326, 132, 0 ),
  INST(Vpcmpb          , VexRvmi_Lx         , V(660F3A,3F,_,x,_,0,4,FVM), 0                         , 0 , 0 , 6322, 401, 122, 0 ),
  INST(Vpcmpd          , VexRvmi_Lx         , V(660F3A,1F,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6329, 402, 120, 0 ),
  INST(Vpcmpeqb        , VexRvm_Lx          , V(660F00,74,_,x,I,I,4,FV ), 0                         , 0 , 0 , 6336, 403, 137, 48),
  INST(Vpcmpeqd        , VexRvm_Lx          , V(660F00,76,_,x,I,0,4,FVM), 0                         , 0 , 0 , 6345, 404, 125, 48),
  INST(Vpcmpeqq        , VexRvm_Lx          , V(660F38,29,_,x,I,1,4,FVM), 0                         , 0 , 0 , 6354, 405, 125, 48),
  INST(Vpcmpeqw        , VexRvm_Lx          , V(660F00,75,_,x,I,I,4,FV ), 0                         , 0 , 0 , 6363, 403, 137, 48),
  INST(Vpcmpestri      , VexRmi             , V(660F3A,61,_,0,I,_,_,_  ), 0                         , 0 , 0 , 6372, 406, 141, 49),
  INST(Vpcmpestrm      , VexRmi             , V(660F3A,60,_,0,I,_,_,_  ), 0                         , 0 , 0 , 6383, 407, 141, 49),
  INST(Vpcmpgtb        , VexRvm_Lx          , V(660F00,64,_,x,I,I,4,FV ), 0                         , 0 , 0 , 6394, 403, 137, 48),
  INST(Vpcmpgtd        , VexRvm_Lx          , V(660F00,66,_,x,I,0,4,FVM), 0                         , 0 , 0 , 6403, 404, 125, 48),
  INST(Vpcmpgtq        , VexRvm_Lx          , V(660F38,37,_,x,I,1,4,FVM), 0                         , 0 , 0 , 6412, 405, 125, 48),
  INST(Vpcmpgtw        , VexRvm_Lx          , V(660F00,65,_,x,I,I,4,FV ), 0                         , 0 , 0 , 6421, 403, 137, 48),
  INST(Vpcmpistri      , VexRmi             , V(660F3A,63,_,0,I,_,_,_  ), 0                         , 0 , 0 , 6430, 408, 141, 49),
  INST(Vpcmpistrm      , VexRmi             , V(660F3A,62,_,0,I,_,_,_  ), 0                         , 0 , 0 , 6441, 409, 141, 49),
  INST(Vpcmpq          , VexRvmi_Lx         , V(660F3A,1F,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6452, 410, 120, 0 ),
  INST(Vpcmpub         , VexRvmi_Lx         , V(660F3A,3E,_,x,_,0,4,FVM), 0                         , 0 , 0 , 6459, 401, 122, 0 ),
  INST(Vpcmpud         , VexRvmi_Lx         , V(660F3A,1E,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6467, 402, 120, 0 ),
  INST(Vpcmpuq         , VexRvmi_Lx         , V(660F3A,1E,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6475, 410, 120, 0 ),
  INST(Vpcmpuw         , VexRvmi_Lx         , V(660F3A,3E,_,x,_,1,4,FVM), 0                         , 0 , 0 , 6483, 410, 122, 0 ),
  INST(Vpcmpw          , VexRvmi_Lx         , V(660F3A,3F,_,x,_,1,4,FVM), 0                         , 0 , 0 , 6491, 410, 122, 0 ),
  INST(Vpcomb          , VexRvmi            , V(XOP_M8,CC,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6498, 310, 132, 0 ),
  INST(Vpcomd          , VexRvmi            , V(XOP_M8,CE,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6505, 310, 132, 0 ),
  INST(Vpcompressd     , VexMr_Lx           , V(660F38,8B,_,x,_,0,2,T1S), 0                         , 0 , 0 , 6512, 279, 120, 0 ),
  INST(Vpcompressq     , VexMr_Lx           , V(660F38,8B,_,x,_,1,3,T1S), 0                         , 0 , 0 , 6524, 279, 120, 0 ),
  INST(Vpcomq          , VexRvmi            , V(XOP_M8,CF,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6536, 310, 132, 0 ),
  INST(Vpcomub         , VexRvmi            , V(XOP_M8,EC,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6543, 310, 132, 0 ),
  INST(Vpcomud         , VexRvmi            , V(XOP_M8,EE,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6551, 310, 132, 0 ),
  INST(Vpcomuq         , VexRvmi            , V(XOP_M8,EF,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6559, 310, 132, 0 ),
  INST(Vpcomuw         , VexRvmi            , V(XOP_M8,ED,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6567, 310, 132, 0 ),
  INST(Vpcomw          , VexRvmi            , V(XOP_M8,CD,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6575, 310, 132, 0 ),
  INST(Vpconflictd     , VexRm_Lx           , V(660F38,C4,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6582, 411, 139, 0 ),
  INST(Vpconflictq     , VexRm_Lx           , V(660F38,C4,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6594, 411, 139, 0 ),
  INST(Vperm2f128      , VexRvmi            , V(660F3A,06,_,1,0,_,_,_  ), 0                         , 0 , 0 , 6606, 412, 118, 0 ),
  INST(Vperm2i128      , VexRvmi            , V(660F3A,46,_,1,0,_,_,_  ), 0                         , 0 , 0 , 6617, 412, 124, 0 ),
  INST(Vpermb          , VexRvm_Lx          , V(660F38,8D,_,x,_,0,4,FVM), 0                         , 0 , 0 , 6628, 260, 142, 0 ),
  INST(Vpermd          , VexRvm_Lx          , V(660F38,36,_,x,0,0,4,FV ), 0                         , 0 , 0 , 6635, 413, 133, 0 ),
  INST(Vpermi2b        , VexRvm_Lx          , V(660F38,75,_,x,_,0,4,FVM), 0                         , 0 , 0 , 6642, 260, 142, 0 ),
  INST(Vpermi2d        , VexRvm_Lx          , V(660F38,76,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6651, 414, 120, 0 ),
  INST(Vpermi2pd       , VexRvm_Lx          , V(660F38,77,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6660, 262, 120, 0 ),
  INST(Vpermi2ps       , VexRvm_Lx          , V(660F38,77,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6670, 261, 120, 0 ),
  INST(Vpermi2q        , VexRvm_Lx          , V(660F38,76,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6680, 415, 120, 0 ),
  INST(Vpermi2w        , VexRvm_Lx          , V(660F38,75,_,x,_,1,4,FVM), 0                         , 0 , 0 , 6689, 416, 122, 0 ),
  INST(Vpermil2pd      , VexRvrmiRvmri_Lx   , V(660F3A,49,_,x,x,_,_,_  ), 0                         , 0 , 0 , 6698, 417, 132, 0 ),
  INST(Vpermil2ps      , VexRvrmiRvmri_Lx   , V(660F3A,48,_,x,x,_,_,_  ), 0                         , 0 , 0 , 6709, 417, 132, 0 ),
  INST(Vpermilpd       , VexRvmRmi_Lx       , V(660F38,0D,_,x,0,1,4,FV ), V(660F3A,05,_,x,0,1,4,FV ), 0 , 0 , 6720, 418, 116, 0 ),
  INST(Vpermilps       , VexRvmRmi_Lx       , V(660F38,0C,_,x,0,0,4,FV ), V(660F3A,04,_,x,0,0,4,FV ), 0 , 0 , 6730, 419, 116, 0 ),
  INST(Vpermpd         , VexRmi             , V(660F3A,01,_,1,1,_,_,_  ), 0                         , 0 , 0 , 6740, 420, 124, 0 ),
  INST(Vpermps         , VexRvm             , V(660F38,16,_,1,0,_,_,_  ), 0                         , 0 , 0 , 6748, 421, 124, 0 ),
  INST(Vpermq          , VexRvmRmi_Lx       , V(660F38,36,_,x,_,1,4,FV ), V(660F3A,00,_,x,1,1,4,FV ), 0 , 0 , 6756, 422, 133, 0 ),
  INST(Vpermt2b        , VexRvm_Lx          , V(660F38,7D,_,x,_,0,4,FVM), 0                         , 0 , 0 , 6763, 260, 142, 0 ),
  INST(Vpermt2d        , VexRvm_Lx          , V(660F38,7E,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6772, 414, 120, 0 ),
  INST(Vpermt2pd       , VexRvm_Lx          , V(660F38,7F,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6781, 415, 120, 0 ),
  INST(Vpermt2ps       , VexRvm_Lx          , V(660F38,7F,_,x,_,0,4,FV ), 0                         , 0 , 0 , 6791, 414, 120, 0 ),
  INST(Vpermt2q        , VexRvm_Lx          , V(660F38,7E,_,x,_,1,4,FV ), 0                         , 0 , 0 , 6801, 415, 120, 0 ),
  INST(Vpermt2w        , VexRvm_Lx          , V(660F38,7D,_,x,_,1,4,FVM), 0                         , 0 , 0 , 6810, 416, 122, 0 ),
  INST(Vpermw          , VexRvm_Lx          , V(660F38,8D,_,x,_,1,4,FVM), 0                         , 0 , 0 , 6819, 260, 122, 0 ),
  INST(Vpexpandd       , VexRm_Lx           , V(660F38,89,_,x,_,0,2,T1S), 0                         , 0 , 0 , 6826, 313, 120, 0 ),
  INST(Vpexpandq       , VexRm_Lx           , V(660F38,89,_,x,_,1,3,T1S), 0                         , 0 , 0 , 6836, 313, 120, 0 ),
  INST(Vpextrb         , VexMri             , V(660F3A,14,_,0,0,I,0,T1S), 0                         , 0 , 0 , 6846, 423, 143, 50),
  INST(Vpextrd         , VexMri             , V(660F3A,16,_,0,0,0,2,T1S), 0                         , 0 , 0 , 6854, 317, 144, 50),
  INST(Vpextrq         , VexMri             , V(660F3A,16,_,0,1,1,3,T1S), 0                         , 0 , 0 , 6862, 424, 144, 50),
  INST(Vpextrw         , VexMri             , V(660F3A,15,_,0,0,I,1,T1S), 0                         , 0 , 0 , 6870, 425, 143, 50),
  INST(Vpgatherdd      , VexRmvRm_VM        , V(660F38,90,_,x,0,_,_,_  ), V(660F38,90,_,x,_,0,2,T1S), 0 , 0 , 6878, 426, 133, 0 ),
  INST(Vpgatherdq      , VexRmvRm_VM        , V(660F38,90,_,x,1,_,_,_  ), V(660F38,90,_,x,_,1,3,T1S), 0 , 0 , 6889, 427, 133, 0 ),
  INST(Vpgatherqd      , VexRmvRm_VM        , V(660F38,91,_,x,0,_,_,_  ), V(660F38,91,_,x,_,0,2,T1S), 0 , 0 , 6900, 428, 133, 0 ),
  INST(Vpgatherqq      , VexRmvRm_VM        , V(660F38,91,_,x,1,_,_,_  ), V(660F38,91,_,x,_,1,3,T1S), 0 , 0 , 6911, 429, 133, 0 ),
  INST(Vphaddbd        , VexRm              , V(XOP_M9,C2,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6922, 252, 132, 0 ),
  INST(Vphaddbq        , VexRm              , V(XOP_M9,C3,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6931, 252, 132, 0 ),
  INST(Vphaddbw        , VexRm              , V(XOP_M9,C1,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6940, 252, 132, 0 ),
  INST(Vphaddd         , VexRvm_Lx          , V(660F38,02,_,x,I,_,_,_  ), 0                         , 0 , 0 , 6949, 250, 135, 51),
  INST(Vphadddq        , VexRm              , V(XOP_M9,CB,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6957, 252, 132, 0 ),
  INST(Vphaddsw        , VexRvm_Lx          , V(660F38,03,_,x,I,_,_,_  ), 0                         , 0 , 0 , 6966, 250, 135, 52),
  INST(Vphaddubd       , VexRm              , V(XOP_M9,D2,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6975, 252, 132, 0 ),
  INST(Vphaddubq       , VexRm              , V(XOP_M9,D3,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6985, 252, 132, 0 ),
  INST(Vphaddubw       , VexRm              , V(XOP_M9,D1,_,0,0,_,_,_  ), 0                         , 0 , 0 , 6995, 252, 132, 0 ),
  INST(Vphaddudq       , VexRm              , V(XOP_M9,DB,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7005, 252, 132, 0 ),
  INST(Vphadduwd       , VexRm              , V(XOP_M9,D6,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7015, 252, 132, 0 ),
  INST(Vphadduwq       , VexRm              , V(XOP_M9,D7,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7025, 252, 132, 0 ),
  INST(Vphaddw         , VexRvm_Lx          , V(660F38,01,_,x,I,_,_,_  ), 0                         , 0 , 0 , 7035, 250, 135, 53),
  INST(Vphaddwd        , VexRm              , V(XOP_M9,C6,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7043, 252, 132, 0 ),
  INST(Vphaddwq        , VexRm              , V(XOP_M9,C7,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7052, 252, 132, 0 ),
  INST(Vphminposuw     , VexRm              , V(660F38,41,_,0,I,_,_,_  ), 0                         , 0 , 0 , 7061, 252, 118, 54),
  INST(Vphsubbw        , VexRm              , V(XOP_M9,E1,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7073, 252, 132, 0 ),
  INST(Vphsubd         , VexRvm_Lx          , V(660F38,06,_,x,I,_,_,_  ), 0                         , 0 , 0 , 7082, 250, 135, 55),
  INST(Vphsubdq        , VexRm              , V(XOP_M9,E3,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7090, 252, 132, 0 ),
  INST(Vphsubsw        , VexRvm_Lx          , V(660F38,07,_,x,I,_,_,_  ), 0                         , 0 , 0 , 7099, 250, 135, 56),
  INST(Vphsubw         , VexRvm_Lx          , V(660F38,05,_,x,I,_,_,_  ), 0                         , 0 , 0 , 7108, 250, 135, 56),
  INST(Vphsubwd        , VexRm              , V(XOP_M9,E2,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7116, 252, 132, 0 ),
  INST(Vpinsrb         , VexRvmi            , V(660F3A,20,_,0,0,I,0,T1S), 0                         , 0 , 0 , 7125, 430, 143, 57),
  INST(Vpinsrd         , VexRvmi            , V(660F3A,22,_,0,0,0,2,T1S), 0                         , 0 , 0 , 7133, 431, 144, 57),
  INST(Vpinsrq         , VexRvmi            , V(660F3A,22,_,0,1,1,3,T1S), 0                         , 0 , 0 , 7141, 432, 144, 57),
  INST(Vpinsrw         , VexRvmi            , V(660F00,C4,_,0,0,I,1,T1S), 0                         , 0 , 0 , 7149, 433, 143, 55),
  INST(Vplzcntd        , VexRm_Lx           , V(660F38,44,_,x,_,0,4,FV ), 0                         , 0 , 0 , 7157, 411, 139, 0 ),
  INST(Vplzcntq        , VexRm_Lx           , V(660F38,44,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7166, 434, 139, 0 ),
  INST(Vpmacsdd        , VexRvmr            , V(XOP_M8,9E,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7175, 435, 132, 0 ),
  INST(Vpmacsdqh       , VexRvmr            , V(XOP_M8,9F,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7184, 435, 132, 0 ),
  INST(Vpmacsdql       , VexRvmr            , V(XOP_M8,97,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7194, 435, 132, 0 ),
  INST(Vpmacssdd       , VexRvmr            , V(XOP_M8,8E,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7204, 435, 132, 0 ),
  INST(Vpmacssdqh      , VexRvmr            , V(XOP_M8,8F,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7214, 435, 132, 0 ),
  INST(Vpmacssdql      , VexRvmr            , V(XOP_M8,87,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7225, 435, 132, 0 ),
  INST(Vpmacsswd       , VexRvmr            , V(XOP_M8,86,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7236, 435, 132, 0 ),
  INST(Vpmacssww       , VexRvmr            , V(XOP_M8,85,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7246, 435, 132, 0 ),
  INST(Vpmacswd        , VexRvmr            , V(XOP_M8,96,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7256, 435, 132, 0 ),
  INST(Vpmacsww        , VexRvmr            , V(XOP_M8,95,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7265, 435, 132, 0 ),
  INST(Vpmadcsswd      , VexRvmr            , V(XOP_M8,A6,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7274, 435, 132, 0 ),
  INST(Vpmadcswd       , VexRvmr            , V(XOP_M8,B6,_,0,0,_,_,_  ), 0                         , 0 , 0 , 7285, 435, 132, 0 ),
  INST(Vpmadd52huq     , VexRvm_Lx          , V(660F38,B5,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7295, 262, 145, 0 ),
  INST(Vpmadd52luq     , VexRvm_Lx          , V(660F38,B4,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7307, 262, 145, 0 ),
  INST(Vpmaddubsw      , VexRvm_Lx          , V(660F38,04,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7319, 388, 137, 58),
  INST(Vpmaddwd        , VexRvm_Lx          , V(660F00,F5,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7330, 388, 137, 58),
  INST(Vpmaskmovd      , VexRvmMvr_Lx       , V(660F38,8C,_,x,0,_,_,_  ), V(660F38,8E,_,x,0,_,_,_  ), 0 , 0 , 7339, 436, 124, 0 ),
  INST(Vpmaskmovq      , VexRvmMvr_Lx       , V(660F38,8C,_,x,1,_,_,_  ), V(660F38,8E,_,x,1,_,_,_  ), 0 , 0 , 7350, 437, 124, 0 ),
  INST(Vpmaxsb         , VexRvm_Lx          , V(660F38,3C,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7361, 438, 137, 59),
  INST(Vpmaxsd         , VexRvm_Lx          , V(660F38,3D,_,x,I,0,4,FV ), 0                         , 0 , 0 , 7369, 259, 125, 59),
  INST(Vpmaxsq         , VexRvm_Lx          , V(660F38,3D,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7377, 262, 120, 0 ),
  INST(Vpmaxsw         , VexRvm_Lx          , V(660F00,EE,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7385, 438, 137, 60),
  INST(Vpmaxub         , VexRvm_Lx          , V(660F00,DE,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7393, 438, 137, 60),
  INST(Vpmaxud         , VexRvm_Lx          , V(660F38,3F,_,x,I,0,4,FV ), 0                         , 0 , 0 , 7401, 259, 125, 60),
  INST(Vpmaxuq         , VexRvm_Lx          , V(660F38,3F,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7409, 262, 120, 0 ),
  INST(Vpmaxuw         , VexRvm_Lx          , V(660F38,3E,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7417, 438, 137, 61),
  INST(Vpminsb         , VexRvm_Lx          , V(660F38,38,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7425, 438, 137, 61),
  INST(Vpminsd         , VexRvm_Lx          , V(660F38,39,_,x,I,0,4,FV ), 0                         , 0 , 0 , 7433, 259, 125, 61),
  INST(Vpminsq         , VexRvm_Lx          , V(660F38,39,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7441, 262, 120, 0 ),
  INST(Vpminsw         , VexRvm_Lx          , V(660F00,EA,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7449, 438, 137, 62),
  INST(Vpminub         , VexRvm_Lx          , V(660F00,DA,_,x,I,_,4,FVM), 0                         , 0 , 0 , 7457, 438, 137, 62),
  INST(Vpminud         , VexRvm_Lx          , V(660F38,3B,_,x,I,0,4,FV ), 0                         , 0 , 0 , 7465, 259, 125, 62),
  INST(Vpminuq         , VexRvm_Lx          , V(660F38,3B,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7473, 262, 120, 0 ),
  INST(Vpminuw         , VexRvm_Lx          , V(660F38,3A,_,x,I,_,4,FVM), 0                         , 0 , 0 , 7481, 438, 137, 63),
  INST(Vpmovb2m        , VexRm_Lx           , V(F30F38,29,_,x,_,0,_,_  ), 0                         , 0 , 0 , 7489, 439, 122, 0 ),
  INST(Vpmovd2m        , VexRm_Lx           , V(F30F38,39,_,x,_,0,_,_  ), 0                         , 0 , 0 , 7498, 439, 123, 0 ),
  INST(Vpmovdb         , VexMr_Lx           , V(F30F38,31,_,x,_,0,2,QVM), 0                         , 0 , 0 , 7507, 440, 120, 0 ),
  INST(Vpmovdw         , VexMr_Lx           , V(F30F38,33,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7515, 441, 120, 0 ),
  INST(Vpmovm2b        , VexRm_Lx           , V(F30F38,28,_,x,_,0,_,_  ), 0                         , 0 , 0 , 7523, 398, 122, 0 ),
  INST(Vpmovm2d        , VexRm_Lx           , V(F30F38,38,_,x,_,0,_,_  ), 0                         , 0 , 0 , 7532, 398, 123, 0 ),
  INST(Vpmovm2q        , VexRm_Lx           , V(F30F38,38,_,x,_,1,_,_  ), 0                         , 0 , 0 , 7541, 398, 123, 0 ),
  INST(Vpmovm2w        , VexRm_Lx           , V(F30F38,28,_,x,_,1,_,_  ), 0                         , 0 , 0 , 7550, 398, 122, 0 ),
  INST(Vpmovmskb       , VexRm_Lx           , V(660F00,D7,_,x,I,_,_,_  ), 0                         , 0 , 0 , 7559, 378, 135, 64),
  INST(Vpmovq2m        , VexRm_Lx           , V(F30F38,39,_,x,_,1,_,_  ), 0                         , 0 , 0 , 7569, 439, 123, 0 ),
  INST(Vpmovqb         , VexMr_Lx           , V(F30F38,32,_,x,_,0,1,OVM), 0                         , 0 , 0 , 7578, 442, 120, 0 ),
  INST(Vpmovqd         , VexMr_Lx           , V(F30F38,35,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7586, 441, 120, 0 ),
  INST(Vpmovqw         , VexMr_Lx           , V(F30F38,34,_,x,_,0,2,QVM), 0                         , 0 , 0 , 7594, 440, 120, 0 ),
  INST(Vpmovsdb        , VexMr_Lx           , V(F30F38,21,_,x,_,0,2,QVM), 0                         , 0 , 0 , 7602, 440, 120, 0 ),
  INST(Vpmovsdw        , VexMr_Lx           , V(F30F38,23,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7611, 441, 120, 0 ),
  INST(Vpmovsqb        , VexMr_Lx           , V(F30F38,22,_,x,_,0,1,OVM), 0                         , 0 , 0 , 7620, 442, 120, 0 ),
  INST(Vpmovsqd        , VexMr_Lx           , V(F30F38,25,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7629, 441, 120, 0 ),
  INST(Vpmovsqw        , VexMr_Lx           , V(F30F38,24,_,x,_,0,2,QVM), 0                         , 0 , 0 , 7638, 440, 120, 0 ),
  INST(Vpmovswb        , VexMr_Lx           , V(F30F38,20,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7647, 441, 122, 0 ),
  INST(Vpmovsxbd       , VexRm_Lx           , V(660F38,21,_,x,I,I,2,QVM), 0                         , 0 , 0 , 7656, 443, 125, 14),
  INST(Vpmovsxbq       , VexRm_Lx           , V(660F38,22,_,x,I,I,1,OVM), 0                         , 0 , 0 , 7666, 444, 125, 14),
  INST(Vpmovsxbw       , VexRm_Lx           , V(660F38,20,_,x,I,I,3,HVM), 0                         , 0 , 0 , 7676, 445, 137, 14),
  INST(Vpmovsxdq       , VexRm_Lx           , V(660F38,25,_,x,I,0,3,HVM), 0                         , 0 , 0 , 7686, 446, 125, 14),
  INST(Vpmovsxwd       , VexRm_Lx           , V(660F38,23,_,x,I,I,3,HVM), 0                         , 0 , 0 , 7696, 445, 125, 14),
  INST(Vpmovsxwq       , VexRm_Lx           , V(660F38,24,_,x,I,I,2,QVM), 0                         , 0 , 0 , 7706, 443, 125, 14),
  INST(Vpmovusdb       , VexMr_Lx           , V(F30F38,11,_,x,_,0,2,QVM), 0                         , 0 , 0 , 7716, 440, 120, 0 ),
  INST(Vpmovusdw       , VexMr_Lx           , V(F30F38,13,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7726, 441, 120, 0 ),
  INST(Vpmovusqb       , VexMr_Lx           , V(F30F38,12,_,x,_,0,1,OVM), 0                         , 0 , 0 , 7736, 442, 120, 0 ),
  INST(Vpmovusqd       , VexMr_Lx           , V(F30F38,15,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7746, 441, 120, 0 ),
  INST(Vpmovusqw       , VexMr_Lx           , V(F30F38,14,_,x,_,0,2,QVM), 0                         , 0 , 0 , 7756, 440, 120, 0 ),
  INST(Vpmovuswb       , VexMr_Lx           , V(F30F38,10,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7766, 441, 122, 0 ),
  INST(Vpmovw2m        , VexRm_Lx           , V(F30F38,29,_,x,_,1,_,_  ), 0                         , 0 , 0 , 7776, 439, 122, 0 ),
  INST(Vpmovwb         , VexMr_Lx           , V(F30F38,30,_,x,_,0,3,HVM), 0                         , 0 , 0 , 7785, 441, 122, 0 ),
  INST(Vpmovzxbd       , VexRm_Lx           , V(660F38,31,_,x,I,I,2,QVM), 0                         , 0 , 0 , 7793, 443, 125, 65),
  INST(Vpmovzxbq       , VexRm_Lx           , V(660F38,32,_,x,I,I,1,OVM), 0                         , 0 , 0 , 7803, 444, 125, 65),
  INST(Vpmovzxbw       , VexRm_Lx           , V(660F38,30,_,x,I,I,3,HVM), 0                         , 0 , 0 , 7813, 445, 137, 65),
  INST(Vpmovzxdq       , VexRm_Lx           , V(660F38,35,_,x,I,0,3,HVM), 0                         , 0 , 0 , 7823, 446, 125, 65),
  INST(Vpmovzxwd       , VexRm_Lx           , V(660F38,33,_,x,I,I,3,HVM), 0                         , 0 , 0 , 7833, 445, 125, 65),
  INST(Vpmovzxwq       , VexRm_Lx           , V(660F38,34,_,x,I,I,2,QVM), 0                         , 0 , 0 , 7843, 443, 125, 65),
  INST(Vpmuldq         , VexRvm_Lx          , V(660F38,28,_,x,I,1,4,FV ), 0                         , 0 , 0 , 7853, 256, 125, 19),
  INST(Vpmulhrsw       , VexRvm_Lx          , V(660F38,0B,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7861, 388, 137, 19),
  INST(Vpmulhuw        , VexRvm_Lx          , V(660F00,E4,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7871, 388, 137, 66),
  INST(Vpmulhw         , VexRvm_Lx          , V(660F00,E5,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7880, 388, 137, 66),
  INST(Vpmulld         , VexRvm_Lx          , V(660F38,40,_,x,I,0,4,FV ), 0                         , 0 , 0 , 7888, 257, 125, 66),
  INST(Vpmullq         , VexRvm_Lx          , V(660F38,40,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7896, 262, 123, 0 ),
  INST(Vpmullw         , VexRvm_Lx          , V(660F00,D5,_,x,I,I,4,FVM), 0                         , 0 , 0 , 7904, 388, 137, 19),
  INST(Vpmultishiftqb  , VexRvm_Lx          , V(660F38,83,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7912, 262, 142, 0 ),
  INST(Vpmuludq        , VexRvm_Lx          , V(660F00,F4,_,x,I,1,4,FV ), 0                         , 0 , 0 , 7927, 256, 125, 67),
  INST(Vpopcntd        , VexRm              , V(660F38,55,_,2,_,0,4,FVM), 0                         , 0 , 0 , 7936, 447, 146, 0 ),
  INST(Vpopcntq        , VexRm              , V(660F38,55,_,2,_,1,4,FVM), 0                         , 0 , 0 , 7945, 448, 146, 0 ),
  INST(Vpor            , VexRvm_Lx          , V(660F00,EB,_,x,I,_,_,_  ), 0                         , 0 , 0 , 7954, 390, 135, 68),
  INST(Vpord           , VexRvm_Lx          , V(660F00,EB,_,x,_,0,4,FV ), 0                         , 0 , 0 , 7959, 391, 120, 0 ),
  INST(Vporq           , VexRvm_Lx          , V(660F00,EB,_,x,_,1,4,FV ), 0                         , 0 , 0 , 7965, 395, 120, 0 ),
  INST(Vpperm          , VexRvrmRvmr        , V(XOP_M8,A3,_,0,x,_,_,_  ), 0                         , 0 , 0 , 7971, 449, 132, 0 ),
  INST(Vprold          , VexVmi_Lx          , V(660F00,72,1,x,_,0,4,FV ), 0                         , 0 , 0 , 7978, 450, 120, 0 ),
  INST(Vprolq          , VexVmi_Lx          , V(660F00,72,1,x,_,1,4,FV ), 0                         , 0 , 0 , 7985, 451, 120, 0 ),
  INST(Vprolvd         , VexRvm_Lx          , V(660F38,15,_,x,_,0,4,FV ), 0                         , 0 , 0 , 7992, 261, 120, 0 ),
  INST(Vprolvq         , VexRvm_Lx          , V(660F38,15,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8000, 262, 120, 0 ),
  INST(Vprord          , VexVmi_Lx          , V(660F00,72,0,x,_,0,4,FV ), 0                         , 0 , 0 , 8008, 450, 120, 0 ),
  INST(Vprorq          , VexVmi_Lx          , V(660F00,72,0,x,_,1,4,FV ), 0                         , 0 , 0 , 8015, 451, 120, 0 ),
  INST(Vprorvd         , VexRvm_Lx          , V(660F38,14,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8022, 261, 120, 0 ),
  INST(Vprorvq         , VexRvm_Lx          , V(660F38,14,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8030, 262, 120, 0 ),
  INST(Vprotb          , VexRvmRmvRmi       , V(XOP_M9,90,_,0,x,_,_,_  ), V(XOP_M8,C0,_,0,x,_,_,_  ), 0 , 0 , 8038, 452, 132, 0 ),
  INST(Vprotd          , VexRvmRmvRmi       , V(XOP_M9,92,_,0,x,_,_,_  ), V(XOP_M8,C2,_,0,x,_,_,_  ), 0 , 0 , 8045, 453, 132, 0 ),
  INST(Vprotq          , VexRvmRmvRmi       , V(XOP_M9,93,_,0,x,_,_,_  ), V(XOP_M8,C3,_,0,x,_,_,_  ), 0 , 0 , 8052, 454, 132, 0 ),
  INST(Vprotw          , VexRvmRmvRmi       , V(XOP_M9,91,_,0,x,_,_,_  ), V(XOP_M8,C1,_,0,x,_,_,_  ), 0 , 0 , 8059, 455, 132, 0 ),
  INST(Vpsadbw         , VexRvm_Lx          , V(660F00,F6,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8066, 456, 137, 69),
  INST(Vpscatterdd     , VexMr_VM           , V(660F38,A0,_,x,_,0,2,T1S), 0                         , 0 , 0 , 8074, 457, 120, 0 ),
  INST(Vpscatterdq     , VexMr_VM           , V(660F38,A0,_,x,_,1,3,T1S), 0                         , 0 , 0 , 8086, 457, 120, 0 ),
  INST(Vpscatterqd     , VexMr_VM           , V(660F38,A1,_,x,_,0,2,T1S), 0                         , 0 , 0 , 8098, 458, 120, 0 ),
  INST(Vpscatterqq     , VexMr_VM           , V(660F38,A1,_,x,_,1,3,T1S), 0                         , 0 , 0 , 8110, 459, 120, 0 ),
  INST(Vpshab          , VexRvmRmv          , V(XOP_M9,98,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8122, 460, 132, 0 ),
  INST(Vpshad          , VexRvmRmv          , V(XOP_M9,9A,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8129, 460, 132, 0 ),
  INST(Vpshaq          , VexRvmRmv          , V(XOP_M9,9B,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8136, 460, 132, 0 ),
  INST(Vpshaw          , VexRvmRmv          , V(XOP_M9,99,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8143, 460, 132, 0 ),
  INST(Vpshlb          , VexRvmRmv          , V(XOP_M9,94,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8150, 460, 132, 0 ),
  INST(Vpshld          , VexRvmRmv          , V(XOP_M9,96,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8157, 460, 132, 0 ),
  INST(Vpshlq          , VexRvmRmv          , V(XOP_M9,97,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8164, 460, 132, 0 ),
  INST(Vpshlw          , VexRvmRmv          , V(XOP_M9,95,_,0,x,_,_,_  ), 0                         , 0 , 0 , 8171, 460, 132, 0 ),
  INST(Vpshufb         , VexRvm_Lx          , V(660F38,00,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8178, 388, 137, 70),
  INST(Vpshufd         , VexRmi_Lx          , V(660F00,70,_,x,I,0,4,FV ), 0                         , 0 , 0 , 8186, 461, 125, 71),
  INST(Vpshufhw        , VexRmi_Lx          , V(F30F00,70,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8194, 462, 137, 71),
  INST(Vpshuflw        , VexRmi_Lx          , V(F20F00,70,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8203, 462, 137, 71),
  INST(Vpsignb         , VexRvm_Lx          , V(660F38,08,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8212, 250, 135, 72),
  INST(Vpsignd         , VexRvm_Lx          , V(660F38,0A,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8220, 250, 135, 72),
  INST(Vpsignw         , VexRvm_Lx          , V(660F38,09,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8228, 250, 135, 72),
  INST(Vpslld          , VexRvmVmi_Lx       , V(660F00,F2,_,x,I,0,4,128), V(660F00,72,6,x,I,0,4,FV ), 0 , 0 , 8236, 463, 125, 72),
  INST(Vpslldq         , VexEvexVmi_Lx      , V(660F00,73,7,x,I,I,4,FVM), 0                         , 0 , 0 , 8243, 464, 137, 72),
  INST(Vpsllq          , VexRvmVmi_Lx       , V(660F00,F3,_,x,I,1,4,128), V(660F00,73,6,x,I,1,4,FV ), 0 , 0 , 8251, 465, 125, 72),
  INST(Vpsllvd         , VexRvm_Lx          , V(660F38,47,_,x,0,0,4,FV ), 0                         , 0 , 0 , 8258, 257, 133, 0 ),
  INST(Vpsllvq         , VexRvm_Lx          , V(660F38,47,_,x,1,1,4,FV ), 0                         , 0 , 0 , 8266, 256, 133, 0 ),
  INST(Vpsllvw         , VexRvm_Lx          , V(660F38,12,_,x,_,1,4,FVM), 0                         , 0 , 0 , 8274, 260, 122, 0 ),
  INST(Vpsllw          , VexRvmVmi_Lx       , V(660F00,F1,_,x,I,I,4,FVM), V(660F00,71,6,x,I,I,4,FVM), 0 , 0 , 8282, 466, 137, 73),
  INST(Vpsrad          , VexRvmVmi_Lx       , V(660F00,E2,_,x,I,0,4,128), V(660F00,72,4,x,I,0,4,FV ), 0 , 0 , 8289, 467, 125, 73),
  INST(Vpsraq          , VexRvmVmi_Lx       , V(660F00,E2,_,x,_,1,4,128), V(660F00,72,4,x,_,1,4,FV ), 0 , 0 , 8296, 468, 120, 0 ),
  INST(Vpsravd         , VexRvm_Lx          , V(660F38,46,_,x,0,0,4,FV ), 0                         , 0 , 0 , 8303, 257, 133, 0 ),
  INST(Vpsravq         , VexRvm_Lx          , V(660F38,46,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8311, 262, 120, 0 ),
  INST(Vpsravw         , VexRvm_Lx          , V(660F38,11,_,x,_,1,4,FVM), 0                         , 0 , 0 , 8319, 260, 122, 0 ),
  INST(Vpsraw          , VexRvmVmi_Lx       , V(660F00,E1,_,x,I,I,4,128), V(660F00,71,4,x,I,I,4,FVM), 0 , 0 , 8327, 469, 137, 74),
  INST(Vpsrld          , VexRvmVmi_Lx       , V(660F00,D2,_,x,I,0,4,128), V(660F00,72,2,x,I,0,4,FV ), 0 , 0 , 8334, 470, 125, 74),
  INST(Vpsrldq         , VexEvexVmi_Lx      , V(660F00,73,3,x,I,I,4,FVM), 0                         , 0 , 0 , 8341, 464, 137, 74),
  INST(Vpsrlq          , VexRvmVmi_Lx       , V(660F00,D3,_,x,I,1,4,128), V(660F00,73,2,x,I,1,4,FV ), 0 , 0 , 8349, 471, 125, 74),
  INST(Vpsrlvd         , VexRvm_Lx          , V(660F38,45,_,x,0,0,4,FV ), 0                         , 0 , 0 , 8356, 257, 133, 0 ),
  INST(Vpsrlvq         , VexRvm_Lx          , V(660F38,45,_,x,1,1,4,FV ), 0                         , 0 , 0 , 8364, 256, 133, 0 ),
  INST(Vpsrlvw         , VexRvm_Lx          , V(660F38,10,_,x,_,1,4,FVM), 0                         , 0 , 0 , 8372, 260, 122, 0 ),
  INST(Vpsrlw          , VexRvmVmi_Lx       , V(660F00,D1,_,x,I,I,4,128), V(660F00,71,2,x,I,I,4,FVM), 0 , 0 , 8380, 472, 137, 75),
  INST(Vpsubb          , VexRvm_Lx          , V(660F00,F8,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8387, 473, 137, 75),
  INST(Vpsubd          , VexRvm_Lx          , V(660F00,FA,_,x,I,0,4,FV ), 0                         , 0 , 0 , 8394, 474, 125, 75),
  INST(Vpsubq          , VexRvm_Lx          , V(660F00,FB,_,x,I,1,4,FV ), 0                         , 0 , 0 , 8401, 475, 125, 75),
  INST(Vpsubsb         , VexRvm_Lx          , V(660F00,E8,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8408, 473, 137, 75),
  INST(Vpsubsw         , VexRvm_Lx          , V(660F00,E9,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8416, 473, 137, 75),
  INST(Vpsubusb        , VexRvm_Lx          , V(660F00,D8,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8424, 473, 137, 75),
  INST(Vpsubusw        , VexRvm_Lx          , V(660F00,D9,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8433, 473, 137, 75),
  INST(Vpsubw          , VexRvm_Lx          , V(660F00,F9,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8442, 473, 137, 75),
  INST(Vpternlogd      , VexRvmi_Lx         , V(660F3A,25,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8449, 476, 120, 0 ),
  INST(Vpternlogq      , VexRvmi_Lx         , V(660F3A,25,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8460, 477, 120, 0 ),
  INST(Vptest          , VexRm_Lx           , V(660F38,17,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8471, 478, 141, 76),
  INST(Vptestmb        , VexRvm_Lx          , V(660F38,26,_,x,_,0,4,FVM), 0                         , 0 , 0 , 8478, 479, 122, 0 ),
  INST(Vptestmd        , VexRvm_Lx          , V(660F38,27,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8487, 480, 120, 0 ),
  INST(Vptestmq        , VexRvm_Lx          , V(660F38,27,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8496, 481, 120, 0 ),
  INST(Vptestmw        , VexRvm_Lx          , V(660F38,26,_,x,_,1,4,FVM), 0                         , 0 , 0 , 8505, 479, 122, 0 ),
  INST(Vptestnmb       , VexRvm_Lx          , V(F30F38,26,_,x,_,0,4,FVM), 0                         , 0 , 0 , 8514, 479, 122, 0 ),
  INST(Vptestnmd       , VexRvm_Lx          , V(F30F38,27,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8524, 480, 120, 0 ),
  INST(Vptestnmq       , VexRvm_Lx          , V(F30F38,27,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8534, 481, 120, 0 ),
  INST(Vptestnmw       , VexRvm_Lx          , V(F30F38,26,_,x,_,1,4,FVM), 0                         , 0 , 0 , 8544, 479, 122, 0 ),
  INST(Vpunpckhbw      , VexRvm_Lx          , V(660F00,68,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8554, 388, 137, 77),
  INST(Vpunpckhdq      , VexRvm_Lx          , V(660F00,6A,_,x,I,0,4,FV ), 0                         , 0 , 0 , 8565, 257, 125, 77),
  INST(Vpunpckhqdq     , VexRvm_Lx          , V(660F00,6D,_,x,I,1,4,FV ), 0                         , 0 , 0 , 8576, 256, 125, 77),
  INST(Vpunpckhwd      , VexRvm_Lx          , V(660F00,69,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8588, 388, 137, 77),
  INST(Vpunpcklbw      , VexRvm_Lx          , V(660F00,60,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8599, 388, 137, 77),
  INST(Vpunpckldq      , VexRvm_Lx          , V(660F00,62,_,x,I,0,4,FV ), 0                         , 0 , 0 , 8610, 257, 125, 77),
  INST(Vpunpcklqdq     , VexRvm_Lx          , V(660F00,6C,_,x,I,1,4,FV ), 0                         , 0 , 0 , 8621, 256, 125, 77),
  INST(Vpunpcklwd      , VexRvm_Lx          , V(660F00,61,_,x,I,I,4,FVM), 0                         , 0 , 0 , 8633, 388, 137, 77),
  INST(Vpxor           , VexRvm_Lx          , V(660F00,EF,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8644, 392, 135, 78),
  INST(Vpxord          , VexRvm_Lx          , V(660F00,EF,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8650, 393, 120, 0 ),
  INST(Vpxorq          , VexRvm_Lx          , V(660F00,EF,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8657, 394, 120, 0 ),
  INST(Vrangepd        , VexRvmi_Lx         , V(660F3A,50,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8664, 482, 123, 0 ),
  INST(Vrangeps        , VexRvmi_Lx         , V(660F3A,50,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8673, 483, 123, 0 ),
  INST(Vrangesd        , VexRvmi            , V(660F3A,51,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8682, 484, 63 , 0 ),
  INST(Vrangess        , VexRvmi            , V(660F3A,51,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8691, 485, 63 , 0 ),
  INST(Vrcp14pd        , VexRm_Lx           , V(660F38,4C,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8700, 434, 120, 0 ),
  INST(Vrcp14ps        , VexRm_Lx           , V(660F38,4C,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8709, 411, 120, 0 ),
  INST(Vrcp14sd        , VexRvm             , V(660F38,4D,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8718, 486, 65 , 0 ),
  INST(Vrcp14ss        , VexRvm             , V(660F38,4D,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8727, 487, 65 , 0 ),
  INST(Vrcp28pd        , VexRm              , V(660F38,CA,_,2,_,1,4,FV ), 0                         , 0 , 0 , 8736, 311, 128, 0 ),
  INST(Vrcp28ps        , VexRm              , V(660F38,CA,_,2,_,0,4,FV ), 0                         , 0 , 0 , 8745, 312, 128, 0 ),
  INST(Vrcp28sd        , VexRvm             , V(660F38,CB,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8754, 488, 128, 0 ),
  INST(Vrcp28ss        , VexRvm             , V(660F38,CB,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8763, 489, 128, 0 ),
  INST(Vrcpps          , VexRm_Lx           , V(000F00,53,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8772, 333, 118, 79),
  INST(Vrcpss          , VexRvm             , V(F30F00,53,_,I,I,_,_,_  ), 0                         , 0 , 0 , 8779, 490, 118, 80),
  INST(Vreducepd       , VexRmi_Lx          , V(660F3A,56,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8786, 451, 123, 0 ),
  INST(Vreduceps       , VexRmi_Lx          , V(660F3A,56,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8796, 450, 123, 0 ),
  INST(Vreducesd       , VexRvmi            , V(660F3A,57,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8806, 491, 63 , 0 ),
  INST(Vreducess       , VexRvmi            , V(660F3A,57,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8816, 492, 63 , 0 ),
  INST(Vrndscalepd     , VexRmi_Lx          , V(660F3A,09,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8826, 345, 120, 0 ),
  INST(Vrndscaleps     , VexRmi_Lx          , V(660F3A,08,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8838, 346, 120, 0 ),
  INST(Vrndscalesd     , VexRvmi            , V(660F3A,0B,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8850, 484, 65 , 0 ),
  INST(Vrndscaless     , VexRvmi            , V(660F3A,0A,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8862, 485, 65 , 0 ),
  INST(Vroundpd        , VexRmi_Lx          , V(660F3A,09,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8874, 493, 118, 81),
  INST(Vroundps        , VexRmi_Lx          , V(660F3A,08,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8883, 493, 118, 81),
  INST(Vroundsd        , VexRvmi            , V(660F3A,0B,_,I,I,_,_,_  ), 0                         , 0 , 0 , 8892, 494, 118, 82),
  INST(Vroundss        , VexRvmi            , V(660F3A,0A,_,I,I,_,_,_  ), 0                         , 0 , 0 , 8901, 495, 118, 82),
  INST(Vrsqrt14pd      , VexRm_Lx           , V(660F38,4E,_,x,_,1,4,FV ), 0                         , 0 , 0 , 8910, 434, 120, 0 ),
  INST(Vrsqrt14ps      , VexRm_Lx           , V(660F38,4E,_,x,_,0,4,FV ), 0                         , 0 , 0 , 8921, 411, 120, 0 ),
  INST(Vrsqrt14sd      , VexRvm             , V(660F38,4F,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8932, 486, 65 , 0 ),
  INST(Vrsqrt14ss      , VexRvm             , V(660F38,4F,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8943, 487, 65 , 0 ),
  INST(Vrsqrt28pd      , VexRm              , V(660F38,CC,_,2,_,1,4,FV ), 0                         , 0 , 0 , 8954, 311, 128, 0 ),
  INST(Vrsqrt28ps      , VexRm              , V(660F38,CC,_,2,_,0,4,FV ), 0                         , 0 , 0 , 8965, 312, 128, 0 ),
  INST(Vrsqrt28sd      , VexRvm             , V(660F38,CD,_,I,_,1,3,T1S), 0                         , 0 , 0 , 8976, 488, 128, 0 ),
  INST(Vrsqrt28ss      , VexRvm             , V(660F38,CD,_,I,_,0,2,T1S), 0                         , 0 , 0 , 8987, 489, 128, 0 ),
  INST(Vrsqrtps        , VexRm_Lx           , V(000F00,52,_,x,I,_,_,_  ), 0                         , 0 , 0 , 8998, 333, 118, 3 ),
  INST(Vrsqrtss        , VexRvm             , V(F30F00,52,_,I,I,_,_,_  ), 0                         , 0 , 0 , 9007, 490, 118, 2 ),
  INST(Vscalefpd       , VexRvm_Lx          , V(660F38,2C,_,x,_,1,4,FV ), 0                         , 0 , 0 , 9016, 496, 120, 0 ),
  INST(Vscalefps       , VexRvm_Lx          , V(660F38,2C,_,x,_,0,4,FV ), 0                         , 0 , 0 , 9026, 497, 120, 0 ),
  INST(Vscalefsd       , VexRvm             , V(660F38,2D,_,I,_,1,3,T1S), 0                         , 0 , 0 , 9036, 498, 65 , 0 ),
  INST(Vscalefss       , VexRvm             , V(660F38,2D,_,I,_,0,2,T1S), 0                         , 0 , 0 , 9046, 499, 65 , 0 ),
  INST(Vscatterdpd     , VexMr_Lx           , V(660F38,A2,_,x,_,1,3,T1S), 0                         , 0 , 0 , 9056, 500, 120, 0 ),
  INST(Vscatterdps     , VexMr_Lx           , V(660F38,A2,_,x,_,0,2,T1S), 0                         , 0 , 0 , 9068, 457, 120, 0 ),
  INST(Vscatterpf0dpd  , VexM_VM            , V(660F38,C6,5,2,_,1,3,T1S), 0                         , 0 , 0 , 9080, 338, 134, 0 ),
  INST(Vscatterpf0dps  , VexM_VM            , V(660F38,C6,5,2,_,0,2,T1S), 0                         , 0 , 0 , 9095, 339, 134, 0 ),
  INST(Vscatterpf0qpd  , VexM_VM            , V(660F38,C7,5,2,_,1,3,T1S), 0                         , 0 , 0 , 9110, 340, 134, 0 ),
  INST(Vscatterpf0qps  , VexM_VM            , V(660F38,C7,5,2,_,0,2,T1S), 0                         , 0 , 0 , 9125, 340, 134, 0 ),
  INST(Vscatterpf1dpd  , VexM_VM            , V(660F38,C6,6,2,_,1,3,T1S), 0                         , 0 , 0 , 9140, 338, 134, 0 ),
  INST(Vscatterpf1dps  , VexM_VM            , V(660F38,C6,6,2,_,0,2,T1S), 0                         , 0 , 0 , 9155, 339, 134, 0 ),
  INST(Vscatterpf1qpd  , VexM_VM            , V(660F38,C7,6,2,_,1,3,T1S), 0                         , 0 , 0 , 9170, 340, 134, 0 ),
  INST(Vscatterpf1qps  , VexM_VM            , V(660F38,C7,6,2,_,0,2,T1S), 0                         , 0 , 0 , 9185, 340, 134, 0 ),
  INST(Vscatterqpd     , VexMr_Lx           , V(660F38,A3,_,x,_,1,3,T1S), 0                         , 0 , 0 , 9200, 459, 120, 0 ),
  INST(Vscatterqps     , VexMr_Lx           , V(660F38,A3,_,x,_,0,2,T1S), 0                         , 0 , 0 , 9212, 458, 120, 0 ),
  INST(Vshuff32x4      , VexRvmi_Lx         , V(660F3A,23,_,x,_,0,4,FV ), 0                         , 0 , 0 , 9224, 501, 120, 0 ),
  INST(Vshuff64x2      , VexRvmi_Lx         , V(660F3A,23,_,x,_,1,4,FV ), 0                         , 0 , 0 , 9235, 502, 120, 0 ),
  INST(Vshufi32x4      , VexRvmi_Lx         , V(660F3A,43,_,x,_,0,4,FV ), 0                         , 0 , 0 , 9246, 501, 120, 0 ),
  INST(Vshufi64x2      , VexRvmi_Lx         , V(660F3A,43,_,x,_,1,4,FV ), 0                         , 0 , 0 , 9257, 502, 120, 0 ),
  INST(Vshufpd         , VexRvmi_Lx         , V(660F00,C6,_,x,I,1,4,FV ), 0                         , 0 , 0 , 9268, 503, 116, 83),
  INST(Vshufps         , VexRvmi_Lx         , V(000F00,C6,_,x,I,0,4,FV ), 0                         , 0 , 0 , 9276, 504, 116, 83),
  INST(Vsqrtpd         , VexRm_Lx           , V(660F00,51,_,x,I,1,4,FV ), 0                         , 0 , 0 , 9284, 505, 116, 84),
  INST(Vsqrtps         , VexRm_Lx           , V(000F00,51,_,x,I,0,4,FV ), 0                         , 0 , 0 , 9292, 281, 116, 84),
  INST(Vsqrtsd         , VexRvm             , V(F20F00,51,_,I,I,1,3,T1S), 0                         , 0 , 0 , 9300, 248, 117, 85),
  INST(Vsqrtss         , VexRvm             , V(F30F00,51,_,I,I,0,2,T1S), 0                         , 0 , 0 , 9308, 249, 117, 85),
  INST(Vstmxcsr        , VexM               , V(000F00,AE,3,0,I,_,_,_  ), 0                         , 0 , 0 , 9316, 506, 118, 0 ),
  INST(Vsubpd          , VexRvm_Lx          , V(660F00,5C,_,x,I,1,4,FV ), 0                         , 0 , 0 , 9325, 246, 116, 86),
  INST(Vsubps          , VexRvm_Lx          , V(000F00,5C,_,x,I,0,4,FV ), 0                         , 0 , 0 , 9332, 247, 116, 86),
  INST(Vsubsd          , VexRvm             , V(F20F00,5C,_,I,I,1,3,T1S), 0                         , 0 , 0 , 9339, 248, 117, 86),
  INST(Vsubss          , VexRvm             , V(F30F00,5C,_,I,I,0,2,T1S), 0                         , 0 , 0 , 9346, 249, 117, 86),
  INST(Vtestpd         , VexRm_Lx           , V(660F38,0F,_,x,0,_,_,_  ), 0                         , 0 , 0 , 9353, 478, 141, 0 ),
  INST(Vtestps         , VexRm_Lx           , V(660F38,0E,_,x,0,_,_,_  ), 0                         , 0 , 0 , 9361, 478, 141, 0 ),
  INST(Vucomisd        , VexRm              , V(660F00,2E,_,I,I,1,3,T1S), 0                         , 0 , 0 , 9369, 277, 126, 15),
  INST(Vucomiss        , VexRm              , V(000F00,2E,_,I,I,0,2,T1S), 0                         , 0 , 0 , 9378, 278, 126, 15),
  INST(Vunpckhpd       , VexRvm_Lx          , V(660F00,15,_,x,I,1,4,FV ), 0                         , 0 , 0 , 9387, 256, 116, 13),
  INST(Vunpckhps       , VexRvm_Lx          , V(000F00,15,_,x,I,0,4,FV ), 0                         , 0 , 0 , 9397, 257, 116, 13),
  INST(Vunpcklpd       , VexRvm_Lx          , V(660F00,14,_,x,I,1,4,FV ), 0                         , 0 , 0 , 9407, 256, 116, 13),
  INST(Vunpcklps       , VexRvm_Lx          , V(000F00,14,_,x,I,0,4,FV ), 0                         , 0 , 0 , 9417, 257, 116, 13),
  INST(Vxorpd          , VexRvm_Lx          , V(660F00,57,_,x,I,1,4,FV ), 0                         , 0 , 0 , 9427, 475, 121, 87),
  INST(Vxorps          , VexRvm_Lx          , V(000F00,57,_,x,I,0,4,FV ), 0                         , 0 , 0 , 9434, 474, 121, 87),
  INST(Vzeroall        , VexOp              , V(000F00,77,_,1,I,_,_,_  ), 0                         , 0 , 0 , 9441, 507, 118, 0 ),
  INST(Vzeroupper      , VexOp              , V(000F00,77,_,0,I,_,_,_  ), 0                         , 0 , 0 , 9450, 507, 118, 0 ),
  INST(Wbinvd          , X86Op              , O(000F00,09,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9461, 34 , 23 , 0 ),
  INST(Wrfsbase        , X86M               , O(F30F00,AE,2,_,x,_,_,_  ), 0                         , 0 , 0 , 9468, 508, 98 , 0 ),
  INST(Wrgsbase        , X86M               , O(F30F00,AE,3,_,x,_,_,_  ), 0                         , 0 , 0 , 9477, 508, 98 , 0 ),
  INST(Wrmsr           , X86Op              , O(000F00,30,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9486, 509, 147, 0 ),
  INST(Xabort          , X86Op_O_I8         , O(000000,C6,7,_,_,_,_,_  ), 0                         , 0 , 0 , 9492, 98 , 148, 0 ),
  INST(Xadd            , X86Xadd            , O(000F00,C0,_,_,x,_,_,_  ), 0                         , 0 , 0 , 9499, 510, 36 , 0 ),
  INST(Xbegin          , X86JmpRel          , O(000000,C7,7,_,_,_,_,_  ), 0                         , 0 , 0 , 9504, 511, 148, 0 ),
  INST(Xchg            , X86Xchg            , O(000000,86,_,_,x,_,_,_  ), 0                         , 0 , 0 , 434 , 512, 0  , 0 ),
  INST(Xend            , X86Op              , O(000F01,D5,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9511, 34 , 148, 0 ),
  INST(Xgetbv          , X86Op              , O(000F01,D0,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9516, 227, 149, 0 ),
  INST(Xlatb           , X86Op              , O(000000,D7,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9523, 34 , 45 , 0 ),
  INST(Xor             , X86Arith           , O(000000,30,6,_,x,_,_,_  ), 0                         , 0 , 0 , 8646, 243, 1  , 0 ),
  INST(Xorpd           , ExtRm              , O(660F00,57,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9428, 192, 4  , 87),
  INST(Xorps           , ExtRm              , O(000F00,57,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9435, 192, 5  , 87),
  INST(Xrstor          , X86M_Only          , O(000F00,AE,5,_,_,_,_,_  ), 0                         , 0 , 0 , 1105, 513, 149, 0 ),
  INST(Xrstor64        , X86M_Only          , O(000F00,AE,5,_,1,_,_,_  ), 0                         , 0 , 0 , 1113, 514, 149, 0 ),
  INST(Xrstors         , X86M_Only          , O(000F00,C7,3,_,_,_,_,_  ), 0                         , 0 , 0 , 9529, 513, 150, 0 ),
  INST(Xrstors64       , X86M_Only          , O(000F00,C7,3,_,1,_,_,_  ), 0                         , 0 , 0 , 9537, 514, 150, 0 ),
  INST(Xsave           , X86M_Only          , O(000F00,AE,4,_,_,_,_,_  ), 0                         , 0 , 0 , 1123, 515, 149, 0 ),
  INST(Xsave64         , X86M_Only          , O(000F00,AE,4,_,1,_,_,_  ), 0                         , 0 , 0 , 1130, 516, 149, 0 ),
  INST(Xsavec          , X86M_Only          , O(000F00,C7,4,_,_,_,_,_  ), 0                         , 0 , 0 , 9547, 515, 151, 0 ),
  INST(Xsavec64        , X86M_Only          , O(000F00,C7,4,_,1,_,_,_  ), 0                         , 0 , 0 , 9554, 516, 151, 0 ),
  INST(Xsaveopt        , X86M_Only          , O(000F00,AE,6,_,_,_,_,_  ), 0                         , 0 , 0 , 9563, 515, 152, 0 ),
  INST(Xsaveopt64      , X86M_Only          , O(000F00,AE,6,_,1,_,_,_  ), 0                         , 0 , 0 , 9572, 516, 152, 0 ),
  INST(Xsaves          , X86M_Only          , O(000F00,C7,5,_,_,_,_,_  ), 0                         , 0 , 0 , 9583, 515, 150, 0 ),
  INST(Xsaves64        , X86M_Only          , O(000F00,C7,5,_,1,_,_,_  ), 0                         , 0 , 0 , 9590, 516, 150, 0 ),
  INST(Xsetbv          , X86Op              , O(000F01,D1,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9599, 509, 153, 0 ),
  INST(Xtest           , X86Op              , O(000F01,D6,_,_,_,_,_,_  ), 0                         , 0 , 0 , 9606, 34 , 154, 0 )
  // ${instData:End}
};
#undef NAME_DATA_INDEX
#undef INST

// ${altOpCodeData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const uint32_t X86InstDB::altOpCodeData[] = {
  0                         , // #0
  O(660F00,1B,_,_,_,_,_,_  ), // #1
  O(000F00,BA,4,_,x,_,_,_  ), // #2
  O(000F00,BA,7,_,x,_,_,_  ), // #3
  O(000F00,BA,6,_,x,_,_,_  ), // #4
  O(000F00,BA,5,_,x,_,_,_  ), // #5
  O(000000,48,_,_,x,_,_,_  ), // #6
  O(660F00,78,0,_,_,_,_,_  ), // #7
  O_FPU(00,00DF,5)          , // #8
  O_FPU(00,00DF,7)          , // #9
  O_FPU(00,00DD,1)          , // #10
  O_FPU(00,00DB,5)          , // #11
  O_FPU(00,DFE0,_)          , // #12
  O(000000,DB,7,_,_,_,_,_  ), // #13
  O_FPU(9B,DFE0,_)          , // #14
  O(000000,E4,_,_,_,_,_,_  ), // #15
  O(000000,40,_,_,x,_,_,_  ), // #16
  O(F20F00,78,_,_,_,_,_,_  ), // #17
  O(000000,77,_,_,_,_,_,_  ), // #18
  O(000000,73,_,_,_,_,_,_  ), // #19
  O(000000,72,_,_,_,_,_,_  ), // #20
  O(000000,76,_,_,_,_,_,_  ), // #21
  O(000000,74,_,_,_,_,_,_  ), // #22
  O(000000,E3,_,_,_,_,_,_  ), // #23
  O(000000,7F,_,_,_,_,_,_  ), // #24
  O(000000,7D,_,_,_,_,_,_  ), // #25
  O(000000,7C,_,_,_,_,_,_  ), // #26
  O(000000,7E,_,_,_,_,_,_  ), // #27
  O(000000,EB,_,_,_,_,_,_  ), // #28
  O(000000,75,_,_,_,_,_,_  ), // #29
  O(000000,71,_,_,_,_,_,_  ), // #30
  O(000000,7B,_,_,_,_,_,_  ), // #31
  O(000000,79,_,_,_,_,_,_  ), // #32
  O(000000,70,_,_,_,_,_,_  ), // #33
  O(000000,7A,_,_,_,_,_,_  ), // #34
  O(000000,78,_,_,_,_,_,_  ), // #35
  V(660F00,92,_,0,0,_,_,_  ), // #36
  V(F20F00,92,_,0,0,_,_,_  ), // #37
  V(F20F00,92,_,0,1,_,_,_  ), // #38
  V(000F00,92,_,0,0,_,_,_  ), // #39
  O(000000,E2,_,_,_,_,_,_  ), // #40
  O(000000,E1,_,_,_,_,_,_  ), // #41
  O(000000,E0,_,_,_,_,_,_  ), // #42
  O(660F00,29,_,_,_,_,_,_  ), // #43
  O(000F00,29,_,_,_,_,_,_  ), // #44
  O(000F38,F1,_,_,x,_,_,_  ), // #45
  O(000F00,7E,_,_,_,_,_,_  ), // #46
  O(660F00,7F,_,_,_,_,_,_  ), // #47
  O(F30F00,7F,_,_,_,_,_,_  ), // #48
  O(660F00,17,_,_,_,_,_,_  ), // #49
  O(000F00,17,_,_,_,_,_,_  ), // #50
  O(660F00,13,_,_,_,_,_,_  ), // #51
  O(000F00,13,_,_,_,_,_,_  ), // #52
  O(660F00,E7,_,_,_,_,_,_  ), // #53
  O(660F00,2B,_,_,_,_,_,_  ), // #54
  O(000F00,2B,_,_,_,_,_,_  ), // #55
  O(000F00,E7,_,_,_,_,_,_  ), // #56
  O(F20F00,2B,_,_,_,_,_,_  ), // #57
  O(F30F00,2B,_,_,_,_,_,_  ), // #58
  O(000F00,7E,_,_,x,_,_,_  ), // #59
  O(F20F00,11,_,_,_,_,_,_  ), // #60
  O(F30F00,11,_,_,_,_,_,_  ), // #61
  O(660F00,11,_,_,_,_,_,_  ), // #62
  O(000F00,11,_,_,_,_,_,_  ), // #63
  O(000000,E6,_,_,_,_,_,_  ), // #64
  O(000F3A,15,_,_,_,_,_,_  ), // #65
  O(000000,58,_,_,_,_,_,_  ), // #66
  O(000F00,72,6,_,_,_,_,_  ), // #67
  O(660F00,73,7,_,_,_,_,_  ), // #68
  O(000F00,73,6,_,_,_,_,_  ), // #69
  O(000F00,71,6,_,_,_,_,_  ), // #70
  O(000F00,72,4,_,_,_,_,_  ), // #71
  O(000F00,71,4,_,_,_,_,_  ), // #72
  O(000F00,72,2,_,_,_,_,_  ), // #73
  O(660F00,73,3,_,_,_,_,_  ), // #74
  O(000F00,73,2,_,_,_,_,_  ), // #75
  O(000F00,71,2,_,_,_,_,_  ), // #76
  O(000000,50,_,_,_,_,_,_  ), // #77
  O(000000,F6,_,_,x,_,_,_  ), // #78
  V(660F38,92,_,x,_,1,3,T1S), // #79
  V(660F38,92,_,x,_,0,2,T1S), // #80
  V(660F38,93,_,x,_,1,3,T1S), // #81
  V(660F38,93,_,x,_,0,2,T1S), // #82
  V(660F38,2F,_,x,0,_,_,_  ), // #83
  V(660F38,2E,_,x,0,_,_,_  ), // #84
  V(660F00,29,_,x,I,1,4,FVM), // #85
  V(000F00,29,_,x,I,0,4,FVM), // #86
  V(660F00,7E,_,0,0,0,2,T1S), // #87
  V(660F00,7F,_,x,I,_,_,_  ), // #88
  V(660F00,7F,_,x,_,0,4,FVM), // #89
  V(660F00,7F,_,x,_,1,4,FVM), // #90
  V(F30F00,7F,_,x,I,_,_,_  ), // #91
  V(F20F00,7F,_,x,_,1,4,FVM), // #92
  V(F30F00,7F,_,x,_,0,4,FVM), // #93
  V(F30F00,7F,_,x,_,1,4,FVM), // #94
  V(F20F00,7F,_,x,_,0,4,FVM), // #95
  V(660F00,17,_,0,I,1,3,T1S), // #96
  V(000F00,17,_,0,I,0,3,T2 ), // #97
  V(660F00,13,_,0,I,1,3,T1S), // #98
  V(000F00,13,_,0,I,0,3,T2 ), // #99
  V(660F00,7E,_,0,I,1,3,T1S), // #100
  V(F20F00,11,_,I,I,1,3,T1S), // #101
  V(F30F00,11,_,I,I,0,2,T1S), // #102
  V(660F00,11,_,x,I,1,4,FVM), // #103
  V(000F00,11,_,x,I,0,4,FVM), // #104
  V(660F3A,05,_,x,0,1,4,FV ), // #105
  V(660F3A,04,_,x,0,0,4,FV ), // #106
  V(660F3A,00,_,x,1,1,4,FV ), // #107
  V(660F38,90,_,x,_,0,2,T1S), // #108
  V(660F38,90,_,x,_,1,3,T1S), // #109
  V(660F38,91,_,x,_,0,2,T1S), // #110
  V(660F38,91,_,x,_,1,3,T1S), // #111
  V(660F38,8E,_,x,0,_,_,_  ), // #112
  V(660F38,8E,_,x,1,_,_,_  ), // #113
  V(XOP_M8,C0,_,0,x,_,_,_  ), // #114
  V(XOP_M8,C2,_,0,x,_,_,_  ), // #115
  V(XOP_M8,C3,_,0,x,_,_,_  ), // #116
  V(XOP_M8,C1,_,0,x,_,_,_  ), // #117
  V(660F00,72,6,x,I,0,4,FV ), // #118
  V(660F00,73,6,x,I,1,4,FV ), // #119
  V(660F00,71,6,x,I,I,4,FVM), // #120
  V(660F00,72,4,x,I,0,4,FV ), // #121
  V(660F00,72,4,x,_,1,4,FV ), // #122
  V(660F00,71,4,x,I,I,4,FVM), // #123
  V(660F00,72,2,x,I,0,4,FV ), // #124
  V(660F00,73,2,x,I,1,4,FV ), // #125
  V(660F00,71,2,x,I,I,4,FVM)  // #126
};
// ----------------------------------------------------------------------------
// ${altOpCodeData:End}

#undef O_FPU
#undef O
#undef V

// ${commonData:Begin}
// ------------------- Automatically generated, do not edit -------------------
#define F(VAL) X86Inst::kFlag##VAL
#define JUMP_TYPE(VAL) Inst::kJumpType##VAL
#define SINGLE_REG(VAL) X86Inst::kSingleReg##VAL
const X86Inst::CommonData X86InstDB::commonData[] = {
  { F(UseR)                                               , 0  , 0  , 0  , 0  , 0 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #0
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 383, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #1
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 384, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #2
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 0  , 15 , 12, JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #3
  { F(UseX)                                               , 0  , 0  , 0  , 25 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #4
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 336, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #5
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 385, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #6
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 386, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #7
  { F(UseW)|F(Vec)                                        , 0  , 0  , 0  , 87 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #8
  { F(UseW)|F(Vec)                                        , 0  , 0  , 0  , 94 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #9
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 0  , 39 , 11, JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #10
  { F(UseW)|F(Vex)                                        , 0  , 0  , 0  , 273, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #11
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 336, 1 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #12
  { F(UseX)                                               , 0  , 0  , 0  , 387, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #13
  { F(UseW)|F(Vex)                                        , 0  , 0  , 0  , 275, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #14
  { F(UseW)|F(Vex)                                        , 0  , 0  , 0  , 184, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #15
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 338, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #16
  { F(UseX)|F(FixedReg)|F(Vec)                            , 0  , 0  , 0  , 388, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #17
  { F(UseR)                                               , 0  , 0  , 0  , 277, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #18
  { F(UseW)|F(Mib)                                        , 0  , 0  , 0  , 389, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #19
  { F(UseW)                                               , 0  , 0  , 0  , 390, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #20
  { F(UseW)                                               , 0  , 0  , 1  , 279, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #21
  { F(UseW)|F(Mib)                                        , 0  , 0  , 0  , 391, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #22
  { F(UseR)                                               , 0  , 0  , 0  , 281, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #23
  { F(UseX)                                               , 0  , 0  , 0  , 24 , 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #24
  { F(UseX)                                               , 0  , 0  , 0  , 392, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #25
  { F(UseR)                                               , 0  , 0  , 2  , 126, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #26
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 3  , 130, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #27
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 4  , 130, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #28
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 5  , 130, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #29
  { F(UseR)                                               , 0  , 0  , 0  , 283, 2 , JUMP_TYPE(Call)       , SINGLE_REG(None), 0 }, // #30
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 393, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #31
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 394, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #32
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 395, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #33
  { F(UseR)                                               , 0  , 0  , 0  , 291, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #34
  { F(UseR)                                               , 0  , 0  , 0  , 396, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #35
  { F(UseR)|F(FixedRM)                                    , 0  , 0  , 0  , 397, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #36
  { F(UseR)                                               , 0  , 0  , 0  , 27 , 12, JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #37
  { F(UseX)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 398, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #38
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 399, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #39
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 400, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #40
  { F(UseX)|F(FixedReg)|F(Lock)|F(XAcquire)|F(XRelease)   , 0  , 0  , 0  , 134, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #41
  { F(UseX)|F(FixedReg)|F(Lock)|F(XAcquire)|F(XRelease)   , 0  , 0  , 0  , 401, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #42
  { F(UseX)|F(FixedReg)|F(Lock)|F(XAcquire)|F(XRelease)   , 0  , 0  , 0  , 402, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #43
  { F(UseR)|F(Vec)                                        , 0  , 0  , 0  , 403, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #44
  { F(UseR)|F(Vec)                                        , 0  , 0  , 0  , 404, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #45
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 405, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #46
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 406, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #47
  { F(UseX)                                               , 0  , 0  , 0  , 285, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #48
  { F(UseW)|F(Vec)                                        , 0  , 16 , 0  , 85 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #49
  { F(UseW)|F(Vec)                                        , 0  , 16 , 0  , 87 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #50
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 8  , 0  , 407, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #51
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 16 , 0  , 408, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #52
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 8  , 0  , 408, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #53
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 8  , 0  , 409, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #54
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 410, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #55
  { F(UseW)|F(Vec)                                        , 0  , 4  , 0  , 85 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #56
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 411, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #57
  { F(UseW)|F(Vec)                                        , 0  , 4  , 0  , 411, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #58
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 255, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #59
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 412, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #60
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 413, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #61
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 414, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #62
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 6  , 287, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #63
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 138, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #64
  { F(UseR)|F(Mmx)                                        , 0  , 0  , 0  , 291, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #65
  { F(UseR)                                               , 0  , 0  , 0  , 415, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #66
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 416, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #67
  { F(UseX)|F(Vec)                                        , 0  , 0  , 7  , 289, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #68
  { F(UseA)|F(FixedReg)|F(FpuM32)|F(FpuM64)               , 0  , 0  , 0  , 174, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #69
  { F(UseX)                                               , 0  , 0  , 0  , 291, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #70
  { F(UseR)|F(FpuM80)                                     , 0  , 0  , 0  , 417, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #71
  { F(UseW)|F(FpuM80)                                     , 0  , 0  , 0  , 418, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #72
  { F(UseX)                                               , 0  , 0  , 0  , 292, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #73
  { F(UseR)|F(FpuM32)|F(FpuM64)                           , 0  , 0  , 0  , 293, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #74
  { F(UseR)                                               , 0  , 0  , 0  , 296, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #75
  { F(UseR)|F(FpuM16)|F(FpuM32)                           , 0  , 0  , 0  , 419, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #76
  { F(UseR)|F(FpuM16)|F(FpuM32)|F(FpuM64)                 , 0  , 0  , 8  , 420, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #77
  { F(UseW)|F(FpuM16)|F(FpuM32)                           , 0  , 0  , 0  , 421, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #78
  { F(UseW)|F(FpuM16)|F(FpuM32)|F(FpuM64)                 , 0  , 0  , 9  , 422, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #79
  { F(UseW)|F(FpuM16)|F(FpuM32)|F(FpuM64)                 , 0  , 0  , 10 , 422, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #80
  { F(UseR)|F(FpuM32)|F(FpuM64)|F(FpuM80)                 , 0  , 0  , 11 , 423, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #81
  { F(UseR)|F(FpuM16)                                     , 0  , 0  , 0  , 424, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #82
  { F(UseX)|F(FixedReg)|F(FpuM32)|F(FpuM64)               , 0  , 0  , 0  , 177, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #83
  { F(UseW)                                               , 0  , 0  , 0  , 425, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #84
  { F(UseW)|F(FpuM16)                                     , 0  , 0  , 0  , 426, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #85
  { F(UseW)|F(FixedReg)|F(FpuM16)                         , 0  , 0  , 12 , 427, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #86
  { F(UseW)|F(FpuM32)|F(FpuM64)                           , 0  , 0  , 0  , 428, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #87
  { F(UseW)|F(FpuM32)|F(FpuM64)|F(FpuM80)                 , 0  , 0  , 13 , 429, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #88
  { F(UseW)|F(FixedReg)|F(FpuM16)                         , 0  , 0  , 14 , 427, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #89
  { F(UseR)                                               , 0  , 0  , 0  , 295, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #90
  { F(UseR)                                               , 0  , 0  , 0  , 430, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #91
  { F(UseW)                                               , 0  , 0  , 0  , 431, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #92
  { F(UseA)|F(FixedReg)                                   , 0  , 0  , 0  , 50 , 10, JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #93
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 15 , 432, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #94
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 16 , 287, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #95
  { F(UseW)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 433, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #96
  { F(UseX)|F(Vec)                                        , 0  , 0  , 17 , 297, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #97
  { F(UseR)                                               , 0  , 0  , 0  , 434, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #98
  { F(UseR)                                               , 0  , 0  , 0  , 435, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #99
  { F(UseR)                                               , 0  , 0  , 0  , 299, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #100
  { F(UseR)                                               , 0  , 0  , 0  , 436, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #101
  { F(UseR)                                               , 0  , 0  , 18 , 437, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #102
  { F(UseR)                                               , 0  , 0  , 19 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #103
  { F(UseR)                                               , 0  , 0  , 20 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #104
  { F(UseR)                                               , 0  , 0  , 21 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #105
  { F(UseR)                                               , 0  , 0  , 20 , 438, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #106
  { F(UseR)                                               , 0  , 0  , 22 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #107
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 23 , 301, 2 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #108
  { F(UseR)                                               , 0  , 0  , 24 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #109
  { F(UseR)                                               , 0  , 0  , 25 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #110
  { F(UseR)                                               , 0  , 0  , 26 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #111
  { F(UseR)                                               , 0  , 0  , 27 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #112
  { F(UseR)                                               , 0  , 0  , 28 , 303, 2 , JUMP_TYPE(Direct)     , SINGLE_REG(None), 0 }, // #113
  { F(UseR)                                               , 0  , 0  , 19 , 438, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #114
  { F(UseR)                                               , 0  , 0  , 29 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #115
  { F(UseR)                                               , 0  , 0  , 30 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #116
  { F(UseR)                                               , 0  , 0  , 31 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #117
  { F(UseR)                                               , 0  , 0  , 32 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #118
  { F(UseR)                                               , 0  , 0  , 33 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #119
  { F(UseR)                                               , 0  , 0  , 34 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #120
  { F(UseR)                                               , 0  , 0  , 35 , 437, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #121
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 439, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #122
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 36 , 305, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #123
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 37 , 307, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #124
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 38 , 309, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #125
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 39 , 311, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #126
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 440, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #127
  { F(UseR)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 441, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #128
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 442, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #129
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 443, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #130
  { F(UseW)                                               , 0  , 0  , 0  , 313, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #131
  { F(UseW)|F(Vec)                                        , 0  , 16 , 0  , 228, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #132
  { F(UseR)                                               , 0  , 0  , 0  , 444, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #133
  { F(UseX)                                               , 0  , 0  , 0  , 315, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #134
  { F(UseW)                                               , 0  , 0  , 0  , 445, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #135
  { F(UseX)                                               , 0  , 0  , 0  , 180, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #136
  { F(UseR)                                               , 0  , 0  , 0  , 446, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #137
  { F(UseW)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 447, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #138
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 40 , 317, 2 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #139
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 41 , 317, 2 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #140
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 42 , 317, 2 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #141
  { F(UseW)                                               , 0  , 0  , 0  , 319, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #142
  { F(UseW)                                               , 0  , 0  , 0  , 183, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #143
  { F(UseX)|F(FixedRM)|F(Vec)                             , 0  , 0  , 0  , 448, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #144
  { F(UseX)|F(FixedRM)|F(Mmx)                             , 0  , 0  , 0  , 449, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #145
  { F(UseR)|F(FixedRM)                                    , 0  , 0  , 0  , 450, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #146
  { F(UseW)|F(XRelease)                                   , 0  , 0  , 0  , 0  , 15, JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #147
  { F(UseW)|F(Vec)                                        , 0  , 16 , 43 , 87 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #148
  { F(UseW)|F(Vec)                                        , 0  , 16 , 44 , 87 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #149
  { F(UseW)                                               , 0  , 0  , 45 , 75 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #150
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 16 , 46 , 321, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #151
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 8  , 0  , 451, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #152
  { F(UseW)|F(Vec)                                        , 0  , 16 , 47 , 87 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #153
  { F(UseW)|F(Vec)                                        , 0  , 16 , 48 , 87 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #154
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 452, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #155
  { F(UseW)|F(Vec)                                        , 8  , 8  , 49 , 234, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #156
  { F(UseW)|F(Vec)                                        , 8  , 8  , 50 , 234, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #157
  { F(UseW)|F(Vec)                                        , 8  , 8  , 0  , 452, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #158
  { F(UseW)|F(Vec)                                        , 0  , 8  , 51 , 234, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #159
  { F(UseW)|F(Vec)                                        , 0  , 8  , 52 , 234, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #160
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 453, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #161
  { F(UseW)|F(Vec)                                        , 0  , 16 , 53 , 225, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #162
  { F(UseW)                                               , 0  , 8  , 0  , 79 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #163
  { F(UseW)|F(Vec)                                        , 0  , 16 , 54 , 225, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #164
  { F(UseW)|F(Vec)                                        , 0  , 16 , 55 , 225, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #165
  { F(UseW)|F(Mmx)                                        , 0  , 8  , 56 , 454, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #166
  { F(UseW)|F(Vec)                                        , 0  , 8  , 57 , 234, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #167
  { F(UseW)|F(Vec)                                        , 0  , 4  , 58 , 237, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #168
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 16 , 59 , 81 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #169
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 16 , 0  , 455, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #170
  { F(UseX)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 456, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #171
  { F(UseW)|F(Vec)                                        , 0  , 8  , 60 , 323, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #172
  { F(UseW)|F(Vec)                                        , 0  , 4  , 61 , 325, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #173
  { F(UseW)                                               , 0  , 0  , 0  , 327, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #174
  { F(UseW)                                               , 0  , 0  , 0  , 457, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #175
  { F(UseW)|F(Vec)                                        , 0  , 16 , 62 , 87 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #176
  { F(UseW)|F(Vec)                                        , 0  , 16 , 63 , 87 , 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #177
  { F(UseA)|F(FixedReg)                                   , 0  , 0  , 0  , 50 , 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #178
  { F(UseW)|F(FixedReg)|F(Vex)                            , 0  , 0  , 0  , 329, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #179
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 0  , 458, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #180
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 0  , 288, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #181
  { F(UseR)                                               , 0  , 0  , 0  , 331, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #182
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 0  , 15 , 12, JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #183
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 64 , 459, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #184
  { F(UseR)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 460, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #185
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 0  , 0  , 333, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #186
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 0  , 335, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #187
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 0  , 337, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #188
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 0  , 335, 2 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #189
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 0  , 335, 2 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #190
  { F(UseX)|F(Mmx)                                        , 0  , 0  , 0  , 335, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #191
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 336, 1 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #192
  { F(UseR)|F(FixedReg)|F(Vec)                            , 0  , 0  , 0  , 461, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #193
  { F(UseR)|F(FixedReg)|F(Vec)                            , 0  , 0  , 0  , 462, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #194
  { F(UseR)|F(FixedReg)|F(Vec)                            , 0  , 0  , 0  , 463, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #195
  { F(UseR)|F(FixedReg)|F(Vec)                            , 0  , 0  , 0  , 464, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #196
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 465, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #197
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 466, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #198
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 8  , 65 , 339, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #199
  { F(UseW)|F(Mmx)                                        , 0  , 8  , 0  , 333, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #200
  { F(UseW)|F(Mmx)                                        , 0  , 0  , 0  , 333, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #201
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 467, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #202
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 468, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #203
  { F(UseX)|F(Vec)                                        , 0  , 0  , 0  , 469, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #204
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 0  , 470, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #205
  { F(UseW)|F(Mmx)|F(Vec)                                 , 0  , 8  , 0  , 471, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #206
  { F(UseW)|F(Vec)                                        , 0  , 16 , 0  , 255, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #207
  { F(UseW)|F(Vec)                                        , 0  , 16 , 0  , 258, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #208
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 66 , 142, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #209
  { F(UseW)|F(Vec)                                        , 0  , 16 , 0  , 94 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #210
  { F(UseW)|F(Mmx)                                        , 0  , 8  , 0  , 472, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #211
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 67 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #212
  { F(UseX)|F(Vec)                                        , 0  , 0  , 68 , 473, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #213
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 69 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #214
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 70 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #215
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 71 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #216
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 72 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #217
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 73 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #218
  { F(UseX)|F(Vec)                                        , 0  , 0  , 74 , 473, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #219
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 75 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #220
  { F(UseX)|F(Mmx)|F(Vec)                                 , 0  , 0  , 76 , 341, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #221
  { F(UseR)|F(Vec)                                        , 0  , 0  , 0  , 379, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #222
  { F(UseA)|F(FixedReg)                                   , 0  , 0  , 77 , 146, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #223
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 474, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #224
  { F(UseW)|F(Vec)                                        , 0  , 4  , 0  , 255, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #225
  { F(UseW)                                               , 0  , 8  , 0  , 475, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #226
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 476, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #227
  { F(UseW)                                               , 0  , 8  , 0  , 477, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #228
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 478, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #229
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 479, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #230
  { F(UseR)                                               , 0  , 0  , 0  , 343, 2 , JUMP_TYPE(Return)     , SINGLE_REG(None), 0 }, // #231
  { F(UseW)|F(Vex)                                        , 0  , 0  , 0  , 345, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #232
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 480, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #233
  { F(UseW)|F(Vec)                                        , 0  , 4  , 0  , 481, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #234
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 0  , 482, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #235
  { F(UseR)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 483, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #236
  { F(UseW)                                               , 0  , 1  , 0  , 484, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #237
  { F(UseX)|F(FixedReg)                                   , 0  , 0  , 0  , 186, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #238
  { F(UseW)                                               , 0  , 0  , 0  , 485, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #239
  { F(UseW)|F(Vec)                                        , 0  , 8  , 0  , 85 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #240
  { F(UseW)                                               , 0  , 0  , 0  , 486, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #241
  { F(UseX)|F(FixedRM)|F(Rep)|F(Repnz)                    , 0  , 0  , 0  , 487, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #242
  { F(UseX)|F(Lock)|F(XAcquire)|F(XRelease)               , 0  , 0  , 0  , 15 , 12, JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #243
  { F(UseR)                                               , 0  , 0  , 78 , 68 , 7 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #244
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512T4X)|F(Avx512KZ)       , 0  , 0  , 0  , 488, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #245
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B64)  , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #246
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B32)  , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #247
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE)      , 0  , 0  , 0  , 489, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #248
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE)      , 0  , 0  , 0  , 490, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #249
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 189, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #250
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 93 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #251
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 87 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #252
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 94 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #253
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #254
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #255
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #256
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #257
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #258
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #259
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #260
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #261
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #262
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 192, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #263
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 347, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #264
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 491, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #265
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 492, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #266
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 493, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #267
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 494, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #268
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 495, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #269
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 260, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #270
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 492, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #271
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 363, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #272
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE_B64)     , 0  , 0  , 0  , 195, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #273
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE_B32)     , 0  , 0  , 0  , 195, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #274
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE)         , 0  , 0  , 0  , 496, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #275
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE)         , 0  , 0  , 0  , 497, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #276
  { F(UseR)|F(Vec)|F(Vex)|F(Evex)|F(Avx512SAE)            , 0  , 0  , 0  , 403, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #277
  { F(UseR)|F(Vec)|F(Vex)|F(Evex)|F(Avx512SAE)            , 0  , 0  , 0  , 404, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #278
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 198, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #279
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #280
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B32)  , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #281
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B64)  , 0  , 0  , 0  , 349, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #282
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B64)  , 0  , 0  , 0  , 207, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #283
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE_B64)         , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #284
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE_B64)         , 0  , 0  , 0  , 349, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #285
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE)         , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #286
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B32)  , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #287
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE)         , 0  , 0  , 0  , 210, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #288
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE_B32)         , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #289
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE_B32)         , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #290
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512ER_SAE)         , 0  , 0  , 0  , 410, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #291
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512ER_SAE)                , 0  , 0  , 0  , 410, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #292
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512ER_SAE)         , 0  , 0  , 0  , 498, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #293
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE)         , 0  , 0  , 0  , 490, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #294
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512ER_SAE)         , 0  , 0  , 0  , 412, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #295
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512ER_SAE)                , 0  , 0  , 0  , 412, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #296
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE_B64)     , 0  , 0  , 0  , 349, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #297
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B64)            , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #298
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B64)            , 0  , 0  , 0  , 349, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #299
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE_B32)     , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #300
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B32)            , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #301
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B32)            , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #302
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512SAE)            , 0  , 0  , 0  , 410, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #303
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512SAE)                   , 0  , 0  , 0  , 410, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #304
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512SAE)            , 0  , 0  , 0  , 412, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #305
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512SAE)                   , 0  , 0  , 0  , 412, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #306
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #307
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512ER_SAE)                , 0  , 0  , 0  , 498, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #308
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #309
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 192, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #310
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B64)            , 0  , 0  , 0  , 91 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #311
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B32)            , 0  , 0  , 0  , 91 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #312
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #313
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 211, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #314
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 499, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #315
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 212, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #316
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 416, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #317
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B64)            , 0  , 0  , 0  , 213, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #318
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B32)            , 0  , 0  , 0  , 213, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #319
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 500, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #320
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 501, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #321
  { F(UseX)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B64)  , 0  , 0  , 0  , 216, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #322
  { F(UseX)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B32)  , 0  , 0  , 0  , 216, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #323
  { F(UseX)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE)      , 0  , 0  , 0  , 502, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #324
  { F(UseX)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE)      , 0  , 0  , 0  , 503, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #325
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 150, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #326
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 351, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #327
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 353, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #328
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K_B64)                 , 0  , 0  , 0  , 504, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #329
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K_B32)                 , 0  , 0  , 0  , 504, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #330
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K)                     , 0  , 0  , 0  , 505, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #331
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K)                     , 0  , 0  , 0  , 506, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #332
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 204, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #333
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 85 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #334
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 255, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #335
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 79 , 111, 5 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #336
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 80 , 116, 5 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #337
  { F(UseR)|F(Vsib)|F(Evex)|F(Avx512K)                    , 0  , 0  , 0  , 507, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #338
  { F(UseR)|F(Vsib)|F(Evex)|F(Avx512K)                    , 0  , 0  , 0  , 508, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #339
  { F(UseR)|F(Vsib)|F(Evex)|F(Avx512K)                    , 0  , 0  , 0  , 509, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #340
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 81 , 121, 5 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #341
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 82 , 154, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #342
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 85 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #343
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 255, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #344
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B64)            , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #345
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B32)            , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #346
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 480, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #347
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 481, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #348
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 355, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #349
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 355, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #350
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 510, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #351
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 511, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #352
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 228, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #353
  { F(UseR)|F(Vex)                                        , 0  , 0  , 0  , 444, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #354
  { F(UseR)|F(FixedRM)|F(Vec)|F(Vex)                      , 0  , 0  , 0  , 512, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #355
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 83 , 158, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #356
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 84 , 158, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #357
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE_B64)     , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #358
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE_B32)     , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #359
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_SAE)         , 0  , 0  , 0  , 489, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #360
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 85 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #361
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 86 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #362
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 87 , 357, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #363
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 222, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #364
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 88 , 87 , 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #365
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 89 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #366
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 90 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #367
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 91 , 87 , 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #368
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 92 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #369
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 93 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #370
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 94 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #371
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 95 , 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #372
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 236, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #373
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 96 , 359, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #374
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 97 , 359, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #375
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 98 , 359, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #376
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 99 , 359, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #377
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 513, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #378
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 225, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #379
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 228, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #380
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 100, 231, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #381
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 101, 234, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #382
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #383
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 102, 237, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #384
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 103, 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #385
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 104, 87 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #386
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512T4X)|F(Avx512KZ)       , 0  , 0  , 0  , 514, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #387
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #388
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #389
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 189, 2 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #390
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #391
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 189, 2 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #392
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #393
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #394
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #395
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 361, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #396
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 363, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #397
  { F(UseW)|F(Vec)|F(Evex)                                , 0  , 0  , 0  , 515, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #398
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 516, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #399
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 365, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #400
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K)                     , 0  , 0  , 0  , 240, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #401
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K_B32)                 , 0  , 0  , 0  , 240, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #402
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512K)              , 0  , 0  , 0  , 243, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #403
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512K_B32)          , 0  , 0  , 0  , 243, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #404
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512K_B64)          , 0  , 0  , 0  , 243, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #405
  { F(UseR)|F(FixedReg)|F(Vec)|F(Vex)                     , 0  , 0  , 0  , 461, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #406
  { F(UseR)|F(FixedReg)|F(Vec)|F(Vex)                     , 0  , 0  , 0  , 462, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #407
  { F(UseR)|F(FixedReg)|F(Vec)|F(Vex)                     , 0  , 0  , 0  , 463, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #408
  { F(UseR)|F(FixedReg)|F(Vec)|F(Vex)                     , 0  , 0  , 0  , 464, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #409
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K_B64)                 , 0  , 0  , 0  , 240, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #410
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #411
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 193, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #412
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 167, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #413
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 216, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #414
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 216, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #415
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 216, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #416
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 162, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #417
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 105, 93 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #418
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 106, 93 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #419
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 96 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #420
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 95 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #421
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 107, 166, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #422
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 465, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #423
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 466, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #424
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 340, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #425
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 108, 116, 5 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #426
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 109, 111, 5 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #427
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 110, 154, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #428
  { F(UseX)|F(Vec)|F(Vsib)|F(Vex)|F(Evex)|F(Avx512K)      , 0  , 0  , 111, 121, 5 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #429
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 367, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #430
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 369, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #431
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 371, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #432
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 517, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #433
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #434
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 151, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #435
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 112, 158, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #436
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 113, 158, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #437
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #438
  { F(UseW)|F(Vec)|F(Evex)                                , 0  , 0  , 0  , 518, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #439
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 246, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #440
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 249, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #441
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 252, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #442
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 255, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #443
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 258, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #444
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 201, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #445
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 261, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #446
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 91 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #447
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 91 , 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #448
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 150, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #449
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #450
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #451
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 114, 373, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #452
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 115, 373, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #453
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 116, 373, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #454
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 117, 373, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #455
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #456
  { F(UseW)|F(Vec)|F(Vsib)|F(Evex)|F(Avx512K)             , 0  , 0  , 0  , 264, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #457
  { F(UseW)|F(Vec)|F(Vsib)|F(Evex)|F(Avx512K)             , 0  , 0  , 0  , 375, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #458
  { F(UseW)|F(Vec)|F(Vsib)|F(Evex)|F(Avx512K)             , 0  , 0  , 0  , 267, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #459
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 377, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #460
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #461
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #462
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 118, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #463
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)                         , 0  , 0  , 0  , 219, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #464
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 119, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #465
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 120, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #466
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 121, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #467
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 122, 105, 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #468
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 123, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #469
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 124, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #470
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 125, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #471
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 126, 99 , 6 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #472
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ)             , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #473
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #474
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(WO)  , 0 }, // #475
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 213, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #476
  { F(UseX)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 213, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #477
  { F(UseR)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 379, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #478
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K)                     , 0  , 0  , 0  , 270, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #479
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K_B32)                 , 0  , 0  , 0  , 270, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #480
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512K_B64)                 , 0  , 0  , 0  , 270, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #481
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B64)            , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #482
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE_B32)            , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #483
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 519, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #484
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 511, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #485
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 489, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #486
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 490, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #487
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 489, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #488
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_SAE)                , 0  , 0  , 0  , 490, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #489
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 490, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #490
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 519, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #491
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ)                    , 0  , 0  , 0  , 511, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #492
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 101, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #493
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 519, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #494
  { F(UseW)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 511, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #495
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE_B64)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #496
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE_B32)         , 0  , 0  , 0  , 189, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #497
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE)             , 0  , 0  , 0  , 489, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #498
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_ER_SAE)             , 0  , 0  , 0  , 490, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #499
  { F(UseW)|F(Vec)|F(Vsib)|F(Evex)|F(Avx512K)             , 0  , 0  , 0  , 381, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #500
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B32)                , 0  , 0  , 0  , 193, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #501
  { F(UseW)|F(Vec)|F(Evex)|F(Avx512KZ_B64)                , 0  , 0  , 0  , 193, 2 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #502
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B32)         , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #503
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_B64)         , 0  , 0  , 0  , 192, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #504
  { F(UseW)|F(Vec)|F(Vex)|F(Evex)|F(Avx512KZ_ER_SAE_B64)  , 0  , 0  , 0  , 204, 3 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #505
  { F(UseW)|F(Vex)                                        , 0  , 0  , 0  , 486, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #506
  { F(UseR)|F(Vec)|F(Vex)                                 , 0  , 0  , 0  , 291, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #507
  { F(UseR)                                               , 0  , 0  , 0  , 520, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #508
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 0  , 521, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #509
  { F(UseX)|F(UseXX)|F(Lock)|F(XAcquire)|F(XRelease)      , 0  , 0  , 0  , 170, 4 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #510
  { F(UseR)                                               , 0  , 0  , 0  , 522, 1 , JUMP_TYPE(Conditional), SINGLE_REG(None), 0 }, // #511
  { F(UseX)|F(UseXX)|F(Lock)|F(XAcquire)                  , 0  , 0  , 0  , 60 , 8 , JUMP_TYPE(None)       , SINGLE_REG(RO)  , 0 }, // #512
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 0  , 523, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #513
  { F(UseR)|F(FixedReg)                                   , 0  , 0  , 0  , 524, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #514
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 525, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }, // #515
  { F(UseW)|F(FixedReg)                                   , 0  , 0  , 0  , 526, 1 , JUMP_TYPE(None)       , SINGLE_REG(None), 0 }  // #516
};
#undef SINGLE_REG
#undef JUMP_TYPE
#undef F
// ----------------------------------------------------------------------------
// ${commonData:End}

#pragma warning(push)
#pragma warning(disable : 4838)
// ${operationData:Begin}
// ------------------- Automatically generated, do not edit -------------------
#define OP_FLAG(F) X86Inst::kOperation##F
#define FEATURE(F) CpuInfo::kX86Feature##F
#define SPECIAL(F) x86::kSpecialReg_##F
const X86Inst::OperationData X86InstDB::operationData[] = {
  { 0, { 0 }, 0, 0 }, // #0
  { 0, { 0 }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #1
  { 0, { 0 }, SPECIAL(FLAGS_CF), SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #2
  { 0, { FEATURE(ADX) }, SPECIAL(FLAGS_CF), SPECIAL(FLAGS_CF) }, // #3
  { 0, { FEATURE(SSE2) }, 0, 0 }, // #4
  { 0, { FEATURE(SSE) }, 0, 0 }, // #5
  { 0, { FEATURE(SSE3) }, 0, 0 }, // #6
  { 0, { FEATURE(ADX) }, SPECIAL(FLAGS_OF), SPECIAL(FLAGS_OF) }, // #7
  { 0, { FEATURE(AESNI) }, 0, 0 }, // #8
  { 0, { FEATURE(BMI) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #9
  { 0, { 0 }, 0, SPECIAL(FLAGS_ZF) }, // #10
  { 0, { FEATURE(TBM) }, 0, 0 }, // #11
  { 0, { FEATURE(SSE4_1) }, 0, 0 }, // #12
  { 0, { FEATURE(MPX) }, 0, 0 }, // #13
  { 0, { 0 }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) }, // #14
  { 0, { FEATURE(BMI2) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #15
  { OP_FLAG(Volatile), { 0 }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #16
  { OP_FLAG(Volatile), { FEATURE(SMAP) }, 0, SPECIAL(FLAGS_AC) }, // #17
  { 0, { 0 }, 0, SPECIAL(FLAGS_CF) }, // #18
  { 0, { 0 }, 0, SPECIAL(FLAGS_DF) }, // #19
  { OP_FLAG(Volatile), { FEATURE(CLFLUSH) }, 0, 0 }, // #20
  { OP_FLAG(Volatile), { FEATURE(CLFLUSHOPT) }, 0, 0 }, // #21
  { OP_FLAG(Volatile), { 0 }, 0, SPECIAL(FLAGS_IF) }, // #22
  { OP_FLAG(Volatile) | OP_FLAG(Privileged), { 0 }, 0, 0 }, // #23
  { OP_FLAG(Volatile), { FEATURE(CLWB) }, 0, 0 }, // #24
  { OP_FLAG(Volatile), { FEATURE(CLZERO) }, 0, 0 }, // #25
  { 0, { 0 }, SPECIAL(FLAGS_CF), SPECIAL(FLAGS_CF) }, // #26
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_ZF), 0 }, // #27
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_CF), 0 }, // #28
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_ZF), 0 }, // #29
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF), 0 }, // #30
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_SF), 0 }, // #31
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_OF), 0 }, // #32
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_PF), 0 }, // #33
  { 0, { FEATURE(CMOV) }, SPECIAL(FLAGS_SF), 0 }, // #34
  { 0, { 0 }, SPECIAL(FLAGS_DF), SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #35
  { 0, { FEATURE(I486) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #36
  { OP_FLAG(Volatile), { FEATURE(CMPXCHG16B) }, 0, SPECIAL(FLAGS_ZF) }, // #37
  { OP_FLAG(Volatile), { FEATURE(CMPXCHG8B) }, 0, SPECIAL(FLAGS_ZF) }, // #38
  { 0, { FEATURE(SSE2) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #39
  { 0, { FEATURE(SSE) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #40
  { OP_FLAG(Volatile), { FEATURE(I486) }, 0, 0 }, // #41
  { 0, { FEATURE(SSE4_2) }, 0, 0 }, // #42
  { 0, { 0 }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #43
  { OP_FLAG(Volatile), { FEATURE(MMX) }, 0, 0 }, // #44
  { OP_FLAG(Volatile), { 0 }, 0, 0 }, // #45
  { 0, { FEATURE(SSE4A) }, 0, 0 }, // #46
  { 0, { 0 }, 0, SPECIAL(X87SW_C0) | SPECIAL(X87SW_C1) | SPECIAL(X87SW_C2) | SPECIAL(X87SW_C3) }, // #47
  { 0, { FEATURE(CMOV) }, 0, SPECIAL(X87SW_C0) | SPECIAL(X87SW_C1) | SPECIAL(X87SW_C2) | SPECIAL(X87SW_C3) }, // #48
  { 0, { 0 }, 0, SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_ZF) | SPECIAL(X87SW_C1) }, // #49
  { OP_FLAG(Volatile), { FEATURE(3DNOW) }, 0, 0 }, // #50
  { 0, { FEATURE(SSE3) }, 0, SPECIAL(X87SW_C0) | SPECIAL(X87SW_C1) | SPECIAL(X87SW_C2) | SPECIAL(X87SW_C3) }, // #51
  { OP_FLAG(Volatile), { FEATURE(FXSR) }, 0, SPECIAL(X87SW_C0) | SPECIAL(X87SW_C1) | SPECIAL(X87SW_C2) | SPECIAL(X87SW_C3) }, // #52
  { OP_FLAG(Volatile), { FEATURE(FXSR) }, 0, 0 }, // #53
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_OF), 0 }, // #54
  { OP_FLAG(Volatile) | OP_FLAG(Privileged), { FEATURE(I486) }, 0, 0 }, // #55
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_ZF), 0 }, // #56
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_CF), 0 }, // #57
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_ZF), 0 }, // #58
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF), 0 }, // #59
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_SF), 0 }, // #60
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_PF), 0 }, // #61
  { OP_FLAG(Volatile), { 0 }, SPECIAL(FLAGS_SF), 0 }, // #62
  { 0, { FEATURE(AVX512_DQ) }, 0, 0 }, // #63
  { 0, { FEATURE(AVX512_BW) }, 0, 0 }, // #64
  { 0, { FEATURE(AVX512_F) }, 0, 0 }, // #65
  { 0, { FEATURE(AVX512_DQ) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #66
  { 0, { FEATURE(AVX512_BW) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #67
  { 0, { FEATURE(AVX512_F) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #68
  { OP_FLAG(Volatile), { FEATURE(LAHFSAHF) }, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF), 0 }, // #69
  { OP_FLAG(Volatile), { 0 }, 0, SPECIAL(FLAGS_ZF) }, // #70
  { OP_FLAG(Barrier) | OP_FLAG(Volatile), { FEATURE(SSE2) }, 0, 0 }, // #71
  { 0, { 0 }, SPECIAL(FLAGS_DF), 0 }, // #72
  { 0, { FEATURE(LZCNT) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #73
  { 0, { FEATURE(MMX2) }, 0, 0 }, // #74
  { OP_FLAG(Volatile) | OP_FLAG(Privileged), { FEATURE(MONITOR) }, 0, 0 }, // #75
  { OP_FLAG(MovCrDr), { 0 }, 0, 0 }, // #76
  { 0, { FEATURE(MOVBE) }, 0, 0 }, // #77
  { 0, { FEATURE(MMX), FEATURE(SSE2) }, 0, 0 }, // #78
  { OP_FLAG(MovSsSd), { FEATURE(SSE2) }, 0, 0 }, // #79
  { OP_FLAG(MovSsSd), { FEATURE(SSE) }, 0, 0 }, // #80
  { 0, { FEATURE(BMI2) }, 0, 0 }, // #81
  { 0, { FEATURE(SSSE3) }, 0, 0 }, // #82
  { 0, { FEATURE(MMX2), FEATURE(SSE2) }, 0, 0 }, // #83
  { 0, { FEATURE(3DNOW) }, 0, 0 }, // #84
  { 0, { FEATURE(PCLMULQDQ) }, 0, 0 }, // #85
  { 0, { FEATURE(SSE4_2) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #86
  { OP_FLAG(Volatile), { FEATURE(PCOMMIT) }, 0, 0 }, // #87
  { 0, { FEATURE(MMX2), FEATURE(SSE2), FEATURE(SSE4_1) }, 0, 0 }, // #88
  { 0, { FEATURE(3DNOW2) }, 0, 0 }, // #89
  { 0, { FEATURE(GEODE) }, 0, 0 }, // #90
  { 0, { FEATURE(POPCNT) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #91
  { OP_FLAG(Prefetch), { FEATURE(3DNOW) }, 0, 0 }, // #92
  { OP_FLAG(Prefetch), { FEATURE(MMX2) }, 0, 0 }, // #93
  { OP_FLAG(Prefetch), { FEATURE(PREFETCHW) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #94
  { OP_FLAG(Prefetch), { FEATURE(PREFETCHWT1) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #95
  { 0, { FEATURE(SSE4_1) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #96
  { 0, { 0 }, 0, SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) }, // #97
  { OP_FLAG(Volatile), { FEATURE(FSGSBASE) }, 0, 0 }, // #98
  { OP_FLAG(Volatile) | OP_FLAG(Privileged), { FEATURE(MSR) }, SPECIAL(MSR), 0 }, // #99
  { OP_FLAG(Volatile), { FEATURE(RDRAND) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #100
  { OP_FLAG(Volatile), { FEATURE(RDSEED) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #101
  { OP_FLAG(Volatile), { FEATURE(RDTSC) }, 0, 0 }, // #102
  { OP_FLAG(Volatile), { FEATURE(RDTSCP) }, 0, 0 }, // #103
  { OP_FLAG(Volatile), { FEATURE(LAHFSAHF) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #104
  { 0, { 0 }, SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_ZF), 0 }, // #105
  { 0, { 0 }, SPECIAL(FLAGS_CF), 0 }, // #106
  { 0, { 0 }, SPECIAL(FLAGS_ZF), 0 }, // #107
  { 0, { 0 }, SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF), 0 }, // #108
  { 0, { 0 }, SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_SF), 0 }, // #109
  { 0, { 0 }, SPECIAL(FLAGS_OF), 0 }, // #110
  { 0, { 0 }, SPECIAL(FLAGS_PF), 0 }, // #111
  { 0, { 0 }, SPECIAL(FLAGS_SF), 0 }, // #112
  { OP_FLAG(Barrier) | OP_FLAG(Volatile), { FEATURE(MMX2) }, 0, 0 }, // #113
  { 0, { FEATURE(SHA) }, 0, 0 }, // #114
  { 0, { FEATURE(AVX512_4FMAPS) }, 0, 0 }, // #115
  { 0, { FEATURE(AVX), FEATURE(AVX512_F), FEATURE(AVX512_VL) }, 0, 0 }, // #116
  { 0, { FEATURE(AVX), FEATURE(AVX512_F) }, 0, 0 }, // #117
  { 0, { FEATURE(AVX) }, 0, 0 }, // #118
  { 0, { FEATURE(AESNI), FEATURE(AVX) }, 0, 0 }, // #119
  { 0, { FEATURE(AVX512_F), FEATURE(AVX512_VL) }, 0, 0 }, // #120
  { 0, { FEATURE(AVX), FEATURE(AVX512_DQ), FEATURE(AVX512_VL) }, 0, 0 }, // #121
  { 0, { FEATURE(AVX512_BW), FEATURE(AVX512_VL) }, 0, 0 }, // #122
  { 0, { FEATURE(AVX512_DQ), FEATURE(AVX512_VL) }, 0, 0 }, // #123
  { 0, { FEATURE(AVX2) }, 0, 0 }, // #124
  { 0, { FEATURE(AVX), FEATURE(AVX2), FEATURE(AVX512_F), FEATURE(AVX512_VL) }, 0, 0 }, // #125
  { 0, { FEATURE(AVX), FEATURE(AVX512_F) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #126
  { 0, { FEATURE(AVX512_F), FEATURE(AVX512_VL), FEATURE(F16C) }, 0, 0 }, // #127
  { 0, { FEATURE(AVX512_ERI) }, 0, 0 }, // #128
  { 0, { FEATURE(AVX512_F), FEATURE(AVX512_VL), FEATURE(FMA) }, 0, 0 }, // #129
  { 0, { FEATURE(AVX512_F), FEATURE(FMA) }, 0, 0 }, // #130
  { 0, { FEATURE(FMA4) }, 0, 0 }, // #131
  { 0, { FEATURE(XOP) }, 0, 0 }, // #132
  { 0, { FEATURE(AVX2), FEATURE(AVX512_F), FEATURE(AVX512_VL) }, 0, 0 }, // #133
  { 0, { FEATURE(AVX512_PFI) }, 0, 0 }, // #134
  { 0, { FEATURE(AVX), FEATURE(AVX2) }, 0, 0 }, // #135
  { 0, { FEATURE(AVX512_4VNNIW) }, 0, 0 }, // #136
  { 0, { FEATURE(AVX), FEATURE(AVX2), FEATURE(AVX512_BW), FEATURE(AVX512_VL) }, 0, 0 }, // #137
  { 0, { FEATURE(AVX2), FEATURE(AVX512_BW), FEATURE(AVX512_VL) }, 0, 0 }, // #138
  { 0, { FEATURE(AVX512_CDI), FEATURE(AVX512_VL) }, 0, 0 }, // #139
  { 0, { FEATURE(AVX), FEATURE(PCLMULQDQ) }, 0, 0 }, // #140
  { 0, { FEATURE(AVX) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }, // #141
  { 0, { FEATURE(AVX512_VBMI), FEATURE(AVX512_VL) }, 0, 0 }, // #142
  { 0, { FEATURE(AVX), FEATURE(AVX512_BW) }, 0, 0 }, // #143
  { 0, { FEATURE(AVX), FEATURE(AVX512_DQ) }, 0, 0 }, // #144
  { 0, { FEATURE(AVX512_IFMA), FEATURE(AVX512_VL) }, 0, 0 }, // #145
  { 0, { FEATURE(AVX512_VPOPCNTDQ) }, 0, 0 }, // #146
  { OP_FLAG(Volatile) | OP_FLAG(Privileged), { FEATURE(MSR) }, 0, SPECIAL(MSR) }, // #147
  { OP_FLAG(Volatile), { FEATURE(RTM) }, 0, 0 }, // #148
  { OP_FLAG(Volatile), { FEATURE(XSAVE) }, SPECIAL(XCR), 0 }, // #149
  { OP_FLAG(Volatile), { FEATURE(XSAVES) }, SPECIAL(XCR), 0 }, // #150
  { OP_FLAG(Volatile), { FEATURE(XSAVEC) }, SPECIAL(XCR), 0 }, // #151
  { OP_FLAG(Volatile), { FEATURE(XSAVEOPT) }, SPECIAL(XCR), 0 }, // #152
  { OP_FLAG(Volatile) | OP_FLAG(Privileged), { FEATURE(XSAVE) }, 0, SPECIAL(XCR) }, // #153
  { OP_FLAG(Volatile), { FEATURE(TSX) }, 0, SPECIAL(FLAGS_AF) | SPECIAL(FLAGS_CF) | SPECIAL(FLAGS_OF) | SPECIAL(FLAGS_PF) | SPECIAL(FLAGS_SF) | SPECIAL(FLAGS_ZF) }  // #154
};
#undef SPECIAL
#undef FEATURE
#undef OP_FLAG
// ----------------------------------------------------------------------------
// ${operationData:End}
#pragma warning(pop)

// ${sseToAvxData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const X86Inst::SseToAvxData X86InstDB::sseToAvxData[] = {
  { X86Inst::kSseToAvxNone     , 0    }, // #0
  { X86Inst::kSseToAvxExtend   , 725  }, // #1
  { X86Inst::kSseToAvxExtend   , 724  }, // #2
  { X86Inst::kSseToAvxMove     , 724  }, // #3
  { X86Inst::kSseToAvxExtend   , 723  }, // #4
  { X86Inst::kSseToAvxBlend    , 723  }, // #5
  { X86Inst::kSseToAvxExtend   , 671  }, // #6
  { X86Inst::kSseToAvxExtend   , 670  }, // #7
  { X86Inst::kSseToAvxMove     , 667  }, // #8
  { X86Inst::kSseToAvxMove     , 666  }, // #9
  { X86Inst::kSseToAvxMove     , 665  }, // #10
  { X86Inst::kSseToAvxMove     , 672  }, // #11
  { X86Inst::kSseToAvxExtend   , 672  }, // #12
  { X86Inst::kSseToAvxExtend   , 673  }, // #13
  { X86Inst::kSseToAvxMove     , 673  }, // #14
  { X86Inst::kSseToAvxMove     , 674  }, // #15
  { X86Inst::kSseToAvxMove     , 676  }, // #16
  { X86Inst::kSseToAvxMove     , 678  }, // #17
  { X86Inst::kSseToAvxMove     , 679  }, // #18
  { X86Inst::kSseToAvxExtend   , 681  }, // #19
  { X86Inst::kSseToAvxMove     , 695  }, // #20
  { X86Inst::kSseToAvxExtend   , 708  }, // #21
  { X86Inst::kSseToAvxExtend   , 707  }, // #22
  { X86Inst::kSseToAvxExtend   , 712  }, // #23
  { X86Inst::kSseToAvxMove     , 616  }, // #24
  { X86Inst::kSseToAvxMove     , 597  }, // #25
  { X86Inst::kSseToAvxExtend   , 598  }, // #26
  { X86Inst::kSseToAvxExtend   , 597  }, // #27
  { X86Inst::kSseToAvxMove     , 595  }, // #28
  { X86Inst::kSseToAvxMove     , 594  }, // #29
  { X86Inst::kSseToAvxMove     , 593  }, // #30
  { X86Inst::kSseToAvxExtend   , 599  }, // #31
  { X86Inst::kSseToAvxMoveIfMem, 599  }, // #32
  { X86Inst::kSseToAvxMove     , 599  }, // #33
  { X86Inst::kSseToAvxMove     , 598  }, // #34
  { X86Inst::kSseToAvxMoveIfMem, 593  }, // #35
  { X86Inst::kSseToAvxMove     , 591  }, // #36
  { X86Inst::kSseToAvxExtend   , 590  }, // #37
  { X86Inst::kSseToAvxExtend   , 589  }, // #38
  { X86Inst::kSseToAvxExtend   , 583  }, // #39
  { X86Inst::kSseToAvxMove     , 583  }, // #40
  { X86Inst::kSseToAvxMove     , 584  }, // #41
  { X86Inst::kSseToAvxExtend   , 584  }, // #42
  { X86Inst::kSseToAvxExtend   , 585  }, // #43
  { X86Inst::kSseToAvxExtend   , 587  }, // #44
  { X86Inst::kSseToAvxExtend   , 586  }, // #45
  { X86Inst::kSseToAvxBlend    , 587  }, // #46
  { X86Inst::kSseToAvxExtend   , 593  }, // #47
  { X86Inst::kSseToAvxExtend   , 596  }, // #48
  { X86Inst::kSseToAvxMove     , 596  }, // #49
  { X86Inst::kSseToAvxMove     , 637  }, // #50
  { X86Inst::kSseToAvxExtend   , 623  }, // #51
  { X86Inst::kSseToAvxExtend   , 624  }, // #52
  { X86Inst::kSseToAvxExtend   , 630  }, // #53
  { X86Inst::kSseToAvxMove     , 632  }, // #54
  { X86Inst::kSseToAvxExtend   , 633  }, // #55
  { X86Inst::kSseToAvxExtend   , 634  }, // #56
  { X86Inst::kSseToAvxMove     , 633  }, // #57
  { X86Inst::kSseToAvxExtend   , 649  }, // #58
  { X86Inst::kSseToAvxExtend   , 651  }, // #59
  { X86Inst::kSseToAvxExtend   , 652  }, // #60
  { X86Inst::kSseToAvxExtend   , 653  }, // #61
  { X86Inst::kSseToAvxExtend   , 654  }, // #62
  { X86Inst::kSseToAvxExtend   , 655  }, // #63
  { X86Inst::kSseToAvxMove     , 663  }, // #64
  { X86Inst::kSseToAvxMove     , 681  }, // #65
  { X86Inst::kSseToAvxExtend   , 680  }, // #66
  { X86Inst::kSseToAvxExtend   , 682  }, // #67
  { X86Inst::kSseToAvxExtend   , 677  }, // #68
  { X86Inst::kSseToAvxExtend   , 685  }, // #69
  { X86Inst::kSseToAvxExtend   , 697  }, // #70
  { X86Inst::kSseToAvxMove     , 697  }, // #71
  { X86Inst::kSseToAvxExtend   , 696  }, // #72
  { X86Inst::kSseToAvxExtend   , 699  }, // #73
  { X86Inst::kSseToAvxExtend   , 703  }, // #74
  { X86Inst::kSseToAvxExtend   , 706  }, // #75
  { X86Inst::kSseToAvxMove     , 707  }, // #76
  { X86Inst::kSseToAvxExtend   , 715  }, // #77
  { X86Inst::kSseToAvxExtend   , 709  }, // #78
  { X86Inst::kSseToAvxMove     , 722  }, // #79
  { X86Inst::kSseToAvxExtend   , 722  }, // #80
  { X86Inst::kSseToAvxMove     , 717  }, // #81
  { X86Inst::kSseToAvxExtend   , 717  }, // #82
  { X86Inst::kSseToAvxExtend   , 693  }, // #83
  { X86Inst::kSseToAvxMove     , 690  }, // #84
  { X86Inst::kSseToAvxExtend   , 690  }, // #85
  { X86Inst::kSseToAvxExtend   , 683  }, // #86
  { X86Inst::kSseToAvxExtend   , -16  }  // #87
};
// ----------------------------------------------------------------------------
// ${sseToAvxData:End}

// ============================================================================
// [asmjit::X86Inst - Id <-> Name]
// ============================================================================

#if !defined(ASMJIT_DISABLE_TEXT)
// ${nameData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const char X86InstDB::nameData[] =
  "\0" "aaa\0" "aad\0" "aam\0" "aas\0" "adc\0" "adcx\0" "adox\0" "arpl\0"
  "bextr\0" "blcfill\0" "blci\0" "blcic\0" "blcmsk\0" "blcs\0" "blsfill\0"
  "blsi\0" "blsic\0" "blsmsk\0" "blsr\0" "bndcl\0" "bndcn\0" "bndcu\0"
  "bndldx\0" "bndmk\0" "bndmov\0" "bndstx\0" "bound\0" "bsf\0" "bsr\0"
  "bswap\0" "bt\0" "btc\0" "btr\0" "bts\0" "bzhi\0" "cbw\0" "cdq\0" "cdqe\0"
  "clac\0" "clc\0" "cld\0" "clflush\0" "clflushopt\0" "cli\0" "clts\0" "clwb\0"
  "clzero\0" "cmc\0" "cmova\0" "cmovae\0" "cmovc\0" "cmovg\0" "cmovge\0"
  "cmovl\0" "cmovle\0" "cmovna\0" "cmovnae\0" "cmovnc\0" "cmovng\0" "cmovnge\0"
  "cmovnl\0" "cmovnle\0" "cmovno\0" "cmovnp\0" "cmovns\0" "cmovnz\0" "cmovo\0"
  "cmovp\0" "cmovpe\0" "cmovpo\0" "cmovs\0" "cmovz\0" "cmp\0" "cmps\0"
  "cmpxchg\0" "cmpxchg16b\0" "cmpxchg8b\0" "cpuid\0" "cqo\0" "crc32\0"
  "cvtpd2pi\0" "cvtpi2pd\0" "cvtpi2ps\0" "cvtps2pi\0" "cvttpd2pi\0"
  "cvttps2pi\0" "cwd\0" "cwde\0" "daa\0" "das\0" "f2xm1\0" "fabs\0" "faddp\0"
  "fbld\0" "fbstp\0" "fchs\0" "fclex\0" "fcmovb\0" "fcmovbe\0" "fcmove\0"
  "fcmovnb\0" "fcmovnbe\0" "fcmovne\0" "fcmovnu\0" "fcmovu\0" "fcom\0"
  "fcomi\0" "fcomip\0" "fcomp\0" "fcompp\0" "fcos\0" "fdecstp\0" "fdiv\0"
  "fdivp\0" "fdivr\0" "fdivrp\0" "femms\0" "ffree\0" "fiadd\0" "ficom\0"
  "ficomp\0" "fidiv\0" "fidivr\0" "fild\0" "fimul\0" "fincstp\0" "finit\0"
  "fist\0" "fistp\0" "fisttp\0" "fisub\0" "fisubr\0" "fld\0" "fld1\0" "fldcw\0"
  "fldenv\0" "fldl2e\0" "fldl2t\0" "fldlg2\0" "fldln2\0" "fldpi\0" "fldz\0"
  "fmulp\0" "fnclex\0" "fninit\0" "fnop\0" "fnsave\0" "fnstcw\0" "fnstenv\0"
  "fnstsw\0" "fpatan\0" "fprem\0" "fprem1\0" "fptan\0" "frndint\0" "frstor\0"
  "fsave\0" "fscale\0" "fsin\0" "fsincos\0" "fsqrt\0" "fst\0" "fstcw\0"
  "fstenv\0" "fstp\0" "fstsw\0" "fsubp\0" "fsubrp\0" "ftst\0" "fucom\0"
  "fucomi\0" "fucomip\0" "fucomp\0" "fucompp\0" "fwait\0" "fxam\0" "fxch\0"
  "fxrstor\0" "fxrstor64\0" "fxsave\0" "fxsave64\0" "fxtract\0" "fyl2x\0"
  "fyl2xp1\0" "hlt\0" "inc\0" "ins\0" "insertq\0" "int3\0" "into\0" "invlpg\0"
  "invpcid\0" "iret\0" "iretd\0" "iretq\0" "iretw\0" "ja\0" "jae\0" "jb\0"
  "jbe\0" "jc\0" "je\0" "jecxz\0" "jg\0" "jge\0" "jl\0" "jle\0" "jmp\0" "jna\0"
  "jnae\0" "jnb\0" "jnbe\0" "jnc\0" "jne\0" "jng\0" "jnge\0" "jnl\0" "jnle\0"
  "jno\0" "jnp\0" "jns\0" "jnz\0" "jo\0" "jp\0" "jpe\0" "jpo\0" "js\0" "jz\0"
  "kaddb\0" "kaddd\0" "kaddq\0" "kaddw\0" "kandb\0" "kandd\0" "kandnb\0"
  "kandnd\0" "kandnq\0" "kandnw\0" "kandq\0" "kandw\0" "kmovb\0" "kmovw\0"
  "knotb\0" "knotd\0" "knotq\0" "knotw\0" "korb\0" "kord\0" "korq\0"
  "kortestb\0" "kortestd\0" "kortestq\0" "kortestw\0" "korw\0" "kshiftlb\0"
  "kshiftld\0" "kshiftlq\0" "kshiftlw\0" "kshiftrb\0" "kshiftrd\0" "kshiftrq\0"
  "kshiftrw\0" "ktestb\0" "ktestd\0" "ktestq\0" "ktestw\0" "kunpckbw\0"
  "kunpckdq\0" "kunpckwd\0" "kxnorb\0" "kxnord\0" "kxnorq\0" "kxnorw\0"
  "kxorb\0" "kxord\0" "kxorq\0" "kxorw\0" "lahf\0" "lar\0" "lds\0" "lea\0"
  "leave\0" "les\0" "lfence\0" "lfs\0" "lgdt\0" "lgs\0" "lidt\0" "lldt\0"
  "lmsw\0" "lods\0" "loop\0" "loope\0" "loopne\0" "lsl\0" "ltr\0" "lzcnt\0"
  "mfence\0" "monitor\0" "movdq2q\0" "movnti\0" "movntq\0" "movntsd\0"
  "movntss\0" "movq2dq\0" "movsx\0" "movsxd\0" "movzx\0" "mulx\0" "mwait\0"
  "neg\0" "not\0" "out\0" "outs\0" "pause\0" "pavgusb\0" "pcommit\0" "pdep\0"
  "pext\0" "pf2id\0" "pf2iw\0" "pfacc\0" "pfadd\0" "pfcmpeq\0" "pfcmpge\0"
  "pfcmpgt\0" "pfmax\0" "pfmin\0" "pfmul\0" "pfnacc\0" "pfpnacc\0" "pfrcp\0"
  "pfrcpit1\0" "pfrcpit2\0" "pfrcpv\0" "pfrsqit1\0" "pfrsqrt\0" "pfrsqrtv\0"
  "pfsub\0" "pfsubr\0" "pi2fd\0" "pi2fw\0" "pmulhrw\0" "pop\0" "popa\0"
  "popad\0" "popcnt\0" "popf\0" "popfd\0" "popfq\0" "prefetch\0"
  "prefetchnta\0" "prefetcht0\0" "prefetcht1\0" "prefetcht2\0" "prefetchw\0"
  "prefetchwt1\0" "pshufw\0" "pswapd\0" "push\0" "pusha\0" "pushad\0" "pushf\0"
  "pushfd\0" "pushfq\0" "rcl\0" "rcr\0" "rdfsbase\0" "rdgsbase\0" "rdmsr\0"
  "rdpmc\0" "rdrand\0" "rdseed\0" "rdtsc\0" "rdtscp\0" "rol\0" "ror\0" "rorx\0"
  "rsm\0" "sahf\0" "sal\0" "sar\0" "sarx\0" "sbb\0" "scas\0" "seta\0" "setae\0"
  "setb\0" "setbe\0" "setc\0" "sete\0" "setg\0" "setge\0" "setl\0" "setle\0"
  "setna\0" "setnae\0" "setnb\0" "setnbe\0" "setnc\0" "setne\0" "setng\0"
  "setnge\0" "setnl\0" "setnle\0" "setno\0" "setnp\0" "setns\0" "setnz\0"
  "seto\0" "setp\0" "setpe\0" "setpo\0" "sets\0" "setz\0" "sfence\0" "sgdt\0"
  "sha1msg1\0" "sha1msg2\0" "sha1nexte\0" "sha1rnds4\0" "sha256msg1\0"
  "sha256msg2\0" "sha256rnds2\0" "shl\0" "shlx\0" "shr\0" "shrd\0" "shrx\0"
  "sidt\0" "sldt\0" "smsw\0" "stac\0" "stc\0" "sti\0" "stos\0" "str\0"
  "swapgs\0" "syscall\0" "sysenter\0" "sysexit\0" "sysexit64\0" "sysret\0"
  "sysret64\0" "t1mskc\0" "tzcnt\0" "tzmsk\0" "ud2\0" "v4fmaddps\0"
  "v4fnmaddps\0" "vaddpd\0" "vaddps\0" "vaddsd\0" "vaddss\0" "vaddsubpd\0"
  "vaddsubps\0" "vaesdec\0" "vaesdeclast\0" "vaesenc\0" "vaesenclast\0"
  "vaesimc\0" "vaeskeygenassist\0" "valignd\0" "valignq\0" "vandnpd\0"
  "vandnps\0" "vandpd\0" "vandps\0" "vblendmb\0" "vblendmd\0" "vblendmpd\0"
  "vblendmps\0" "vblendmq\0" "vblendmw\0" "vblendpd\0" "vblendps\0"
  "vblendvpd\0" "vblendvps\0" "vbroadcastf128\0" "vbroadcastf32x2\0"
  "vbroadcastf32x4\0" "vbroadcastf32x8\0" "vbroadcastf64x2\0"
  "vbroadcastf64x4\0" "vbroadcasti128\0" "vbroadcasti32x2\0"
  "vbroadcasti32x4\0" "vbroadcasti32x8\0" "vbroadcasti64x2\0"
  "vbroadcasti64x4\0" "vbroadcastsd\0" "vbroadcastss\0" "vcmppd\0" "vcmpps\0"
  "vcmpsd\0" "vcmpss\0" "vcomisd\0" "vcomiss\0" "vcompresspd\0" "vcompressps\0"
  "vcvtdq2pd\0" "vcvtdq2ps\0" "vcvtpd2dq\0" "vcvtpd2ps\0" "vcvtpd2qq\0"
  "vcvtpd2udq\0" "vcvtpd2uqq\0" "vcvtph2ps\0" "vcvtps2dq\0" "vcvtps2pd\0"
  "vcvtps2ph\0" "vcvtps2qq\0" "vcvtps2udq\0" "vcvtps2uqq\0" "vcvtqq2pd\0"
  "vcvtqq2ps\0" "vcvtsd2si\0" "vcvtsd2ss\0" "vcvtsd2usi\0" "vcvtsi2sd\0"
  "vcvtsi2ss\0" "vcvtss2sd\0" "vcvtss2si\0" "vcvtss2usi\0" "vcvttpd2dq\0"
  "vcvttpd2qq\0" "vcvttpd2udq\0" "vcvttpd2uqq\0" "vcvttps2dq\0" "vcvttps2qq\0"
  "vcvttps2udq\0" "vcvttps2uqq\0" "vcvttsd2si\0" "vcvttsd2usi\0" "vcvttss2si\0"
  "vcvttss2usi\0" "vcvtudq2pd\0" "vcvtudq2ps\0" "vcvtuqq2pd\0" "vcvtuqq2ps\0"
  "vcvtusi2sd\0" "vcvtusi2ss\0" "vdbpsadbw\0" "vdivpd\0" "vdivps\0" "vdivsd\0"
  "vdivss\0" "vdppd\0" "vdpps\0" "verr\0" "verw\0" "vexp2pd\0" "vexp2ps\0"
  "vexpandpd\0" "vexpandps\0" "vextractf128\0" "vextractf32x4\0"
  "vextractf32x8\0" "vextractf64x2\0" "vextractf64x4\0" "vextracti128\0"
  "vextracti32x4\0" "vextracti32x8\0" "vextracti64x2\0" "vextracti64x4\0"
  "vextractps\0" "vfixupimmpd\0" "vfixupimmps\0" "vfixupimmsd\0"
  "vfixupimmss\0" "vfmadd132pd\0" "vfmadd132ps\0" "vfmadd132sd\0"
  "vfmadd132ss\0" "vfmadd213pd\0" "vfmadd213ps\0" "vfmadd213sd\0"
  "vfmadd213ss\0" "vfmadd231pd\0" "vfmadd231ps\0" "vfmadd231sd\0"
  "vfmadd231ss\0" "vfmaddpd\0" "vfmaddps\0" "vfmaddsd\0" "vfmaddss\0"
  "vfmaddsub132pd\0" "vfmaddsub132ps\0" "vfmaddsub213pd\0" "vfmaddsub213ps\0"
  "vfmaddsub231pd\0" "vfmaddsub231ps\0" "vfmaddsubpd\0" "vfmaddsubps\0"
  "vfmsub132pd\0" "vfmsub132ps\0" "vfmsub132sd\0" "vfmsub132ss\0"
  "vfmsub213pd\0" "vfmsub213ps\0" "vfmsub213sd\0" "vfmsub213ss\0"
  "vfmsub231pd\0" "vfmsub231ps\0" "vfmsub231sd\0" "vfmsub231ss\0"
  "vfmsubadd132pd\0" "vfmsubadd132ps\0" "vfmsubadd213pd\0" "vfmsubadd213ps\0"
  "vfmsubadd231pd\0" "vfmsubadd231ps\0" "vfmsubaddpd\0" "vfmsubaddps\0"
  "vfmsubpd\0" "vfmsubps\0" "vfmsubsd\0" "vfmsubss\0" "vfnmadd132pd\0"
  "vfnmadd132ps\0" "vfnmadd132sd\0" "vfnmadd132ss\0" "vfnmadd213pd\0"
  "vfnmadd213ps\0" "vfnmadd213sd\0" "vfnmadd213ss\0" "vfnmadd231pd\0"
  "vfnmadd231ps\0" "vfnmadd231sd\0" "vfnmadd231ss\0" "vfnmaddpd\0"
  "vfnmaddps\0" "vfnmaddsd\0" "vfnmaddss\0" "vfnmsub132pd\0" "vfnmsub132ps\0"
  "vfnmsub132sd\0" "vfnmsub132ss\0" "vfnmsub213pd\0" "vfnmsub213ps\0"
  "vfnmsub213sd\0" "vfnmsub213ss\0" "vfnmsub231pd\0" "vfnmsub231ps\0"
  "vfnmsub231sd\0" "vfnmsub231ss\0" "vfnmsubpd\0" "vfnmsubps\0" "vfnmsubsd\0"
  "vfnmsubss\0" "vfpclasspd\0" "vfpclassps\0" "vfpclasssd\0" "vfpclassss\0"
  "vfrczpd\0" "vfrczps\0" "vfrczsd\0" "vfrczss\0" "vgatherdpd\0" "vgatherdps\0"
  "vgatherpf0dpd\0" "vgatherpf0dps\0" "vgatherpf0qpd\0" "vgatherpf0qps\0"
  "vgatherpf1dpd\0" "vgatherpf1dps\0" "vgatherpf1qpd\0" "vgatherpf1qps\0"
  "vgatherqpd\0" "vgatherqps\0" "vgetexppd\0" "vgetexpps\0" "vgetexpsd\0"
  "vgetexpss\0" "vgetmantpd\0" "vgetmantps\0" "vgetmantsd\0" "vgetmantss\0"
  "vhaddpd\0" "vhaddps\0" "vhsubpd\0" "vhsubps\0" "vinsertf128\0"
  "vinsertf32x4\0" "vinsertf32x8\0" "vinsertf64x2\0" "vinsertf64x4\0"
  "vinserti128\0" "vinserti32x4\0" "vinserti32x8\0" "vinserti64x2\0"
  "vinserti64x4\0" "vinsertps\0" "vlddqu\0" "vldmxcsr\0" "vmaskmovdqu\0"
  "vmaskmovpd\0" "vmaskmovps\0" "vmaxpd\0" "vmaxps\0" "vmaxsd\0" "vmaxss\0"
  "vminpd\0" "vminps\0" "vminsd\0" "vminss\0" "vmovapd\0" "vmovaps\0" "vmovd\0"
  "vmovddup\0" "vmovdqa\0" "vmovdqa32\0" "vmovdqa64\0" "vmovdqu\0"
  "vmovdqu16\0" "vmovdqu32\0" "vmovdqu64\0" "vmovdqu8\0" "vmovhlps\0"
  "vmovhpd\0" "vmovhps\0" "vmovlhps\0" "vmovlpd\0" "vmovlps\0" "vmovmskpd\0"
  "vmovmskps\0" "vmovntdq\0" "vmovntdqa\0" "vmovntpd\0" "vmovntps\0" "vmovq\0"
  "vmovsd\0" "vmovshdup\0" "vmovsldup\0" "vmovss\0" "vmovupd\0" "vmovups\0"
  "vmpsadbw\0" "vmulpd\0" "vmulps\0" "vmulsd\0" "vmulss\0" "vorpd\0" "vorps\0"
  "vp4dpwssd\0" "vp4dpwssds\0" "vpabsb\0" "vpabsd\0" "vpabsq\0" "vpabsw\0"
  "vpackssdw\0" "vpacksswb\0" "vpackusdw\0" "vpackuswb\0" "vpaddb\0" "vpaddd\0"
  "vpaddq\0" "vpaddsb\0" "vpaddsw\0" "vpaddusb\0" "vpaddusw\0" "vpaddw\0"
  "vpalignr\0" "vpand\0" "vpandd\0" "vpandn\0" "vpandnd\0" "vpandnq\0"
  "vpandq\0" "vpavgb\0" "vpavgw\0" "vpblendd\0" "vpblendvb\0" "vpblendw\0"
  "vpbroadcastb\0" "vpbroadcastd\0" "vpbroadcastmb2d\0" "vpbroadcastmb2q\0"
  "vpbroadcastq\0" "vpbroadcastw\0" "vpclmulqdq\0" "vpcmov\0" "vpcmpb\0"
  "vpcmpd\0" "vpcmpeqb\0" "vpcmpeqd\0" "vpcmpeqq\0" "vpcmpeqw\0" "vpcmpestri\0"
  "vpcmpestrm\0" "vpcmpgtb\0" "vpcmpgtd\0" "vpcmpgtq\0" "vpcmpgtw\0"
  "vpcmpistri\0" "vpcmpistrm\0" "vpcmpq\0" "vpcmpub\0" "vpcmpud\0" "vpcmpuq\0"
  "vpcmpuw\0" "vpcmpw\0" "vpcomb\0" "vpcomd\0" "vpcompressd\0" "vpcompressq\0"
  "vpcomq\0" "vpcomub\0" "vpcomud\0" "vpcomuq\0" "vpcomuw\0" "vpcomw\0"
  "vpconflictd\0" "vpconflictq\0" "vperm2f128\0" "vperm2i128\0" "vpermb\0"
  "vpermd\0" "vpermi2b\0" "vpermi2d\0" "vpermi2pd\0" "vpermi2ps\0" "vpermi2q\0"
  "vpermi2w\0" "vpermil2pd\0" "vpermil2ps\0" "vpermilpd\0" "vpermilps\0"
  "vpermpd\0" "vpermps\0" "vpermq\0" "vpermt2b\0" "vpermt2d\0" "vpermt2pd\0"
  "vpermt2ps\0" "vpermt2q\0" "vpermt2w\0" "vpermw\0" "vpexpandd\0"
  "vpexpandq\0" "vpextrb\0" "vpextrd\0" "vpextrq\0" "vpextrw\0" "vpgatherdd\0"
  "vpgatherdq\0" "vpgatherqd\0" "vpgatherqq\0" "vphaddbd\0" "vphaddbq\0"
  "vphaddbw\0" "vphaddd\0" "vphadddq\0" "vphaddsw\0" "vphaddubd\0"
  "vphaddubq\0" "vphaddubw\0" "vphaddudq\0" "vphadduwd\0" "vphadduwq\0"
  "vphaddw\0" "vphaddwd\0" "vphaddwq\0" "vphminposuw\0" "vphsubbw\0"
  "vphsubd\0" "vphsubdq\0" "vphsubsw\0" "vphsubw\0" "vphsubwd\0" "vpinsrb\0"
  "vpinsrd\0" "vpinsrq\0" "vpinsrw\0" "vplzcntd\0" "vplzcntq\0" "vpmacsdd\0"
  "vpmacsdqh\0" "vpmacsdql\0" "vpmacssdd\0" "vpmacssdqh\0" "vpmacssdql\0"
  "vpmacsswd\0" "vpmacssww\0" "vpmacswd\0" "vpmacsww\0" "vpmadcsswd\0"
  "vpmadcswd\0" "vpmadd52huq\0" "vpmadd52luq\0" "vpmaddubsw\0" "vpmaddwd\0"
  "vpmaskmovd\0" "vpmaskmovq\0" "vpmaxsb\0" "vpmaxsd\0" "vpmaxsq\0" "vpmaxsw\0"
  "vpmaxub\0" "vpmaxud\0" "vpmaxuq\0" "vpmaxuw\0" "vpminsb\0" "vpminsd\0"
  "vpminsq\0" "vpminsw\0" "vpminub\0" "vpminud\0" "vpminuq\0" "vpminuw\0"
  "vpmovb2m\0" "vpmovd2m\0" "vpmovdb\0" "vpmovdw\0" "vpmovm2b\0" "vpmovm2d\0"
  "vpmovm2q\0" "vpmovm2w\0" "vpmovmskb\0" "vpmovq2m\0" "vpmovqb\0" "vpmovqd\0"
  "vpmovqw\0" "vpmovsdb\0" "vpmovsdw\0" "vpmovsqb\0" "vpmovsqd\0" "vpmovsqw\0"
  "vpmovswb\0" "vpmovsxbd\0" "vpmovsxbq\0" "vpmovsxbw\0" "vpmovsxdq\0"
  "vpmovsxwd\0" "vpmovsxwq\0" "vpmovusdb\0" "vpmovusdw\0" "vpmovusqb\0"
  "vpmovusqd\0" "vpmovusqw\0" "vpmovuswb\0" "vpmovw2m\0" "vpmovwb\0"
  "vpmovzxbd\0" "vpmovzxbq\0" "vpmovzxbw\0" "vpmovzxdq\0" "vpmovzxwd\0"
  "vpmovzxwq\0" "vpmuldq\0" "vpmulhrsw\0" "vpmulhuw\0" "vpmulhw\0" "vpmulld\0"
  "vpmullq\0" "vpmullw\0" "vpmultishiftqb\0" "vpmuludq\0" "vpopcntd\0"
  "vpopcntq\0" "vpor\0" "vpord\0" "vporq\0" "vpperm\0" "vprold\0" "vprolq\0"
  "vprolvd\0" "vprolvq\0" "vprord\0" "vprorq\0" "vprorvd\0" "vprorvq\0"
  "vprotb\0" "vprotd\0" "vprotq\0" "vprotw\0" "vpsadbw\0" "vpscatterdd\0"
  "vpscatterdq\0" "vpscatterqd\0" "vpscatterqq\0" "vpshab\0" "vpshad\0"
  "vpshaq\0" "vpshaw\0" "vpshlb\0" "vpshld\0" "vpshlq\0" "vpshlw\0" "vpshufb\0"
  "vpshufd\0" "vpshufhw\0" "vpshuflw\0" "vpsignb\0" "vpsignd\0" "vpsignw\0"
  "vpslld\0" "vpslldq\0" "vpsllq\0" "vpsllvd\0" "vpsllvq\0" "vpsllvw\0"
  "vpsllw\0" "vpsrad\0" "vpsraq\0" "vpsravd\0" "vpsravq\0" "vpsravw\0"
  "vpsraw\0" "vpsrld\0" "vpsrldq\0" "vpsrlq\0" "vpsrlvd\0" "vpsrlvq\0"
  "vpsrlvw\0" "vpsrlw\0" "vpsubb\0" "vpsubd\0" "vpsubq\0" "vpsubsb\0"
  "vpsubsw\0" "vpsubusb\0" "vpsubusw\0" "vpsubw\0" "vpternlogd\0"
  "vpternlogq\0" "vptest\0" "vptestmb\0" "vptestmd\0" "vptestmq\0" "vptestmw\0"
  "vptestnmb\0" "vptestnmd\0" "vptestnmq\0" "vptestnmw\0" "vpunpckhbw\0"
  "vpunpckhdq\0" "vpunpckhqdq\0" "vpunpckhwd\0" "vpunpcklbw\0" "vpunpckldq\0"
  "vpunpcklqdq\0" "vpunpcklwd\0" "vpxor\0" "vpxord\0" "vpxorq\0" "vrangepd\0"
  "vrangeps\0" "vrangesd\0" "vrangess\0" "vrcp14pd\0" "vrcp14ps\0" "vrcp14sd\0"
  "vrcp14ss\0" "vrcp28pd\0" "vrcp28ps\0" "vrcp28sd\0" "vrcp28ss\0" "vrcpps\0"
  "vrcpss\0" "vreducepd\0" "vreduceps\0" "vreducesd\0" "vreducess\0"
  "vrndscalepd\0" "vrndscaleps\0" "vrndscalesd\0" "vrndscaless\0" "vroundpd\0"
  "vroundps\0" "vroundsd\0" "vroundss\0" "vrsqrt14pd\0" "vrsqrt14ps\0"
  "vrsqrt14sd\0" "vrsqrt14ss\0" "vrsqrt28pd\0" "vrsqrt28ps\0" "vrsqrt28sd\0"
  "vrsqrt28ss\0" "vrsqrtps\0" "vrsqrtss\0" "vscalefpd\0" "vscalefps\0"
  "vscalefsd\0" "vscalefss\0" "vscatterdpd\0" "vscatterdps\0"
  "vscatterpf0dpd\0" "vscatterpf0dps\0" "vscatterpf0qpd\0" "vscatterpf0qps\0"
  "vscatterpf1dpd\0" "vscatterpf1dps\0" "vscatterpf1qpd\0" "vscatterpf1qps\0"
  "vscatterqpd\0" "vscatterqps\0" "vshuff32x4\0" "vshuff64x2\0" "vshufi32x4\0"
  "vshufi64x2\0" "vshufpd\0" "vshufps\0" "vsqrtpd\0" "vsqrtps\0" "vsqrtsd\0"
  "vsqrtss\0" "vstmxcsr\0" "vsubpd\0" "vsubps\0" "vsubsd\0" "vsubss\0"
  "vtestpd\0" "vtestps\0" "vucomisd\0" "vucomiss\0" "vunpckhpd\0" "vunpckhps\0"
  "vunpcklpd\0" "vunpcklps\0" "vxorpd\0" "vxorps\0" "vzeroall\0" "vzeroupper\0"
  "wbinvd\0" "wrfsbase\0" "wrgsbase\0" "wrmsr\0" "xabort\0" "xadd\0" "xbegin\0"
  "xend\0" "xgetbv\0" "xlatb\0" "xrstors\0" "xrstors64\0" "xsavec\0"
  "xsavec64\0" "xsaveopt\0" "xsaveopt64\0" "xsaves\0" "xsaves64\0" "xsetbv\0"
  "xtest";

enum {
  kX86InstMaxLength = 16
};

struct InstNameAZ {
  uint16_t start;
  uint16_t end;
};

static const InstNameAZ X86InstNameAZ[26] = {
  { X86Inst::kIdAaa       , X86Inst::kIdArpl       + 1 },
  { X86Inst::kIdBextr     , X86Inst::kIdBzhi       + 1 },
  { X86Inst::kIdCall      , X86Inst::kIdCwde       + 1 },
  { X86Inst::kIdDaa       , X86Inst::kIdDpps       + 1 },
  { X86Inst::kIdEmms      , X86Inst::kIdExtrq      + 1 },
  { X86Inst::kIdF2xm1     , X86Inst::kIdFyl2xp1    + 1 },
  { X86Inst::kIdNone      , X86Inst::kIdNone       + 1 },
  { X86Inst::kIdHaddpd    , X86Inst::kIdHsubps     + 1 },
  { X86Inst::kIdIdiv      , X86Inst::kIdIretw      + 1 },
  { X86Inst::kIdJa        , X86Inst::kIdJz         + 1 },
  { X86Inst::kIdKaddb     , X86Inst::kIdKxorw      + 1 },
  { X86Inst::kIdLahf      , X86Inst::kIdLzcnt      + 1 },
  { X86Inst::kIdMaskmovdqu, X86Inst::kIdMwait      + 1 },
  { X86Inst::kIdNeg       , X86Inst::kIdNot        + 1 },
  { X86Inst::kIdOr        , X86Inst::kIdOuts       + 1 },
  { X86Inst::kIdPabsb     , X86Inst::kIdPxor       + 1 },
  { X86Inst::kIdNone      , X86Inst::kIdNone       + 1 },
  { X86Inst::kIdRcl       , X86Inst::kIdRsqrtss    + 1 },
  { X86Inst::kIdSahf      , X86Inst::kIdSysret64   + 1 },
  { X86Inst::kIdT1mskc    , X86Inst::kIdTzmsk      + 1 },
  { X86Inst::kIdUcomisd   , X86Inst::kIdUnpcklps   + 1 },
  { X86Inst::kIdV4fmaddps , X86Inst::kIdVzeroupper + 1 },
  { X86Inst::kIdWbinvd    , X86Inst::kIdWrmsr      + 1 },
  { X86Inst::kIdXabort    , X86Inst::kIdXtest      + 1 },
  { X86Inst::kIdNone      , X86Inst::kIdNone       + 1 },
  { X86Inst::kIdNone      , X86Inst::kIdNone       + 1 }
};
// ----------------------------------------------------------------------------
// ${nameData:End}

uint32_t X86Inst::getIdByName(const char* name, size_t len) noexcept {
  if (ASMJIT_UNLIKELY(!name))
    return Inst::kIdNone;

  if (len == Globals::kInvalidIndex)
    len = ::strlen(name);

  if (ASMJIT_UNLIKELY(len == 0 || len > kX86InstMaxLength))
    return Inst::kIdNone;

  uint32_t prefix = static_cast<uint32_t>(name[0]) - 'a';
  if (ASMJIT_UNLIKELY(prefix > 'z' - 'a'))
    return Inst::kIdNone;

  uint32_t index = X86InstNameAZ[prefix].start;
  if (ASMJIT_UNLIKELY(!index))
    return Inst::kIdNone;

  const char* nameData = X86InstDB::nameData;
  const X86Inst* instData = X86InstDB::instData;

  const X86Inst* base = instData + index;
  const X86Inst* end  = instData + X86InstNameAZ[prefix].end;

  for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
    const X86Inst* cur = base + (lim >> 1);
    int result = Utils::cmpInstName(nameData + cur[0].getNameDataIndex(), name, len);

    if (result < 0) {
      base = cur + 1;
      lim--;
      continue;
    }

    if (result > 0)
      continue;

    return static_cast<uint32_t>((size_t)(cur - instData));
  }

  return Inst::kIdNone;
}

const char* X86Inst::getNameById(uint32_t id) noexcept {
  if (ASMJIT_UNLIKELY(id >= X86Inst::_kIdCount))
    return nullptr;
  return X86Inst::getInst(id).getName();
}
#else
const char X86InstDB::nameData[] = "";
#endif // !ASMJIT_DISABLE_TEXT

// ============================================================================
// [asmjit::X86Inst - Validation]
// ============================================================================

#if !defined(ASMJIT_DISABLE_VALIDATION)
// ${signatureData:Begin}
// ------------------- Automatically generated, do not edit -------------------
#define FLAG(flag) X86Inst::kOp##flag
#define MEM(mem) X86Inst::kMemOp##mem
#define OSIGNATURE(flags, memFlags, extFlags, regId) \
  { uint32_t(flags), uint16_t(memFlags), uint8_t(extFlags), uint8_t(regId) }
const X86Inst::OSignature X86InstDB::oSignatureData[] = {
  OSIGNATURE(0, 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Seg), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Seg) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Seg) | FLAG(I32), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem) | FLAG(I8) | FLAG(U8), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Seg), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Seg) | FLAG(Mem) | FLAG(I32) | FLAG(U32), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Seg), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I16) | FLAG(U16), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Cr) | FLAG(Dr) | FLAG(I64) | FLAG(U64), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(U8), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I32) | FLAG(U32), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Cr) | FLAG(Dr), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Cr) | FLAG(Dr), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(I32), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M8) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x04),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem) | FLAG(I8) | FLAG(I16), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem) | FLAG(I8) | FLAG(I32), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem) | FLAG(I8) | FLAG(I32), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I16) | FLAG(U16), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I32) | FLAG(U32), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I32), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mm) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Mm) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Mem), MEM(Any) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Ymm) | FLAG(Mem), MEM(Any) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Zmm) | FLAG(Mem), MEM(Any) | MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Zmm) | FLAG(Mem), MEM(Any) | MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(U8), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem) | FLAG(U8), MEM(Any) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32x), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32y), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32z), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64x), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64z), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(U8), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbLo), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x04),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M16) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Seg), 0, 0, 0x1A),
  OSIGNATURE(FLAG(W) | FLAG(Seg), 0, 0, 0x60),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpq) | FLAG(Mem) | FLAG(I8) | FLAG(I16) | FLAG(I32), MEM(Any) | MEM(M16) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Seg), 0, 0, 0x1E),
  OSIGNATURE(FLAG(R) | FLAG(Seg), 0, 0, 0x60),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64x) | MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(U4), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Fp), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Fp), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Fp), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Fp), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M48), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M80), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(U8), 0, 0, 0x02),
  OSIGNATURE(FLAG(W) | FLAG(K) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(K) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(K), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Mem), MEM(M64) | MEM(M128) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Mem), MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32x), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32y), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32z), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64x), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64z), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Bnd), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Bnd), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Bnd) | FLAG(Mem), MEM(Any), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Bnd) | FLAG(Mem), MEM(Any), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem) | FLAG(I32) | FLAG(I64) | FLAG(Rel32), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(I32) | FLAG(I64) | FLAG(Rel8), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem) | FLAG(I32) | FLAG(I64) | FLAG(Rel8) | FLAG(Rel32), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(K), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x02),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x04),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x04),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Mem) | FLAG(U8), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(U16), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Mem), MEM(M128) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(U8), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64x) | MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Xmm), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Mib), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Mib), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Mem), MEM(BaseOnly) | MEM(Ds), 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Mem), MEM(BaseOnly) | MEM(Ds), 0, 0x40),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Mem), MEM(BaseOnly) | MEM(Es), 0, 0x80),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(Any) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x08),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(Any) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x08),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x08),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any) | MEM(M80), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64) | MEM(M80), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(Any), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64) | MEM(M80), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(U8), 0, 0, 0x04),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(BaseOnly) | MEM(Es), 0, 0x80),
  OSIGNATURE(FLAG(R) | FLAG(Gpw), 0, 0, 0x04),
  OSIGNATURE(FLAG(R) | FLAG(I32) | FLAG(I64) | FLAG(Rel8) | FLAG(Rel32), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(GpbHi), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any) | MEM(M8) | MEM(M16) | MEM(M32) | MEM(M48) | MEM(M64) | MEM(M80) | MEM(M128) | MEM(M256) | MEM(M512) | MEM(M1024), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(Any) | MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(GpbLo) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Mem), MEM(BaseOnly) | MEM(Ds), 0, 0x80),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(BaseOnly) | MEM(Ds), 0, 0x40),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Xmm), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbHi), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbLo) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm) | FLAG(Mem), MEM(M128) | MEM(M256) | MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I32) | FLAG(I64) | FLAG(Rel32), 0, 0, 0x00)
};
#undef OSIGNATURE
#undef MEM
#undef FLAG

#define ISIGNATURE(count, x86, x64, implicit, o0, o1, o2, o3, o4, o5) \
  { count, (x86 ? uint8_t(X86Inst::kArchMaskX86) : uint8_t(0)) |      \
           (x64 ? uint8_t(X86Inst::kArchMaskX64) : uint8_t(0)) ,      \
    implicit,                                                         \
    0,                                                                \
    { o0, o1, o2, o3, o4, o5 }                                        \
  }
const X86Inst::ISignature X86InstDB::iSignatureData[] = {
  ISIGNATURE(2, 1, 1, 0, 1  , 2  , 0  , 0  , 0  , 0  ), // #0   {W:r8lo|r8hi|m8|mem, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 3  , 4  , 0  , 0  , 0  , 0  ), //      {W:r16|m16|mem, R:r16|sreg}
  ISIGNATURE(2, 1, 1, 0, 5  , 6  , 0  , 0  , 0  , 0  ), //      {W:r32|m32|mem|sreg, R:r32}
  ISIGNATURE(2, 0, 1, 0, 7  , 8  , 0  , 0  , 0  , 0  ), //      {W:r64|m64|mem, R:r64|sreg|i32}
  ISIGNATURE(2, 1, 1, 0, 9  , 10 , 0  , 0  , 0  , 0  ), //      {W:r8lo|r8hi, R:r8lo|r8hi|m8|mem|i8|u8}
  ISIGNATURE(2, 1, 1, 0, 11 , 12 , 0  , 0  , 0  , 0  ), //      {W:r16|sreg, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 13 , 14 , 0  , 0  , 0  , 0  ), //      {W:r32, R:r32|m32|mem|sreg|i32|u32}
  ISIGNATURE(2, 0, 1, 0, 15 , 16 , 0  , 0  , 0  , 0  ), //      {W:r64|sreg, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 17 , 18 , 0  , 0  , 0  , 0  ), //      {W:r16|m16, R:i16|u16}
  ISIGNATURE(2, 0, 1, 0, 19 , 20 , 0  , 0  , 0  , 0  ), //      {W:r64, R:i64|u64|creg|dreg}
  ISIGNATURE(2, 1, 1, 0, 21 , 22 , 0  , 0  , 0  , 0  ), //      {W:r8lo|r8hi|m8, R:i8|u8}
  ISIGNATURE(2, 1, 1, 0, 23 , 24 , 0  , 0  , 0  , 0  ), //      {W:r32|m32, R:i32|u32}
  ISIGNATURE(2, 1, 0, 0, 13 , 25 , 0  , 0  , 0  , 0  ), //      {W:r32, R:creg|dreg}
  ISIGNATURE(2, 1, 0, 0, 26 , 6  , 0  , 0  , 0  , 0  ), //      {W:creg|dreg, R:r32}
  ISIGNATURE(2, 0, 1, 0, 26 , 27 , 0  , 0  , 0  , 0  ), //      {W:creg|dreg, R:r64}
  ISIGNATURE(2, 1, 1, 0, 28 , 22 , 0  , 0  , 0  , 0  ), // #15  {X:r8lo|r8hi|m8, R:i8|u8}
  ISIGNATURE(2, 1, 1, 0, 29 , 18 , 0  , 0  , 0  , 0  ), //      {X:r16|m16, R:i16|u16}
  ISIGNATURE(2, 1, 1, 0, 30 , 24 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, R:i32|u32}
  ISIGNATURE(2, 0, 1, 0, 31 , 32 , 0  , 0  , 0  , 0  ), //      {X:r64|m64|mem, R:i32|r64}
  ISIGNATURE(2, 1, 1, 0, 33 , 34 , 0  , 0  , 0  , 0  ), //      {X:r16|m16|r32|m32|r64|m64|mem, R:i8}
  ISIGNATURE(2, 1, 1, 0, 35 , 2  , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8|mem, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 36 , 37 , 0  , 0  , 0  , 0  ), //      {X:r16|m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 38 , 6  , 0  , 0  , 0  , 0  ), //      {X:r32|m32|mem, R:r32}
  ISIGNATURE(2, 1, 1, 0, 39 , 40 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, R:r8lo|r8hi|m8|mem}
  ISIGNATURE(2, 1, 1, 0, 41 , 12 , 0  , 0  , 0  , 0  ), // #24  {X:r16, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 42 , 43 , 0  , 0  , 0  , 0  ), // #25  {X:r32, R:r32|m32|mem}
  ISIGNATURE(2, 0, 1, 0, 44 , 16 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 45 , 22 , 0  , 0  , 0  , 0  ), // #27  {R:r8lo|r8hi|m8, R:i8|u8}
  ISIGNATURE(2, 1, 1, 0, 46 , 18 , 0  , 0  , 0  , 0  ), //      {R:r16|m16, R:i16|u16}
  ISIGNATURE(2, 1, 1, 0, 47 , 24 , 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:i32|u32}
  ISIGNATURE(2, 0, 1, 0, 16 , 32 , 0  , 0  , 0  , 0  ), //      {R:r64|m64|mem, R:i32|r64}
  ISIGNATURE(2, 1, 1, 0, 48 , 34 , 0  , 0  , 0  , 0  ), //      {R:r16|m16|r32|m32|r64|m64|mem, R:i8}
  ISIGNATURE(2, 1, 1, 0, 40 , 2  , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi|m8|mem, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 12 , 37 , 0  , 0  , 0  , 0  ), //      {R:r16|m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 43 , 6  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|mem, R:r32}
  ISIGNATURE(2, 1, 1, 0, 2  , 40 , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi, R:r8lo|r8hi|m8|mem}
  ISIGNATURE(2, 1, 1, 0, 37 , 12 , 0  , 0  , 0  , 0  ), //      {R:r16, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 6  , 43 , 0  , 0  , 0  , 0  ), //      {R:r32, R:r32|m32|mem}
  ISIGNATURE(2, 0, 1, 0, 27 , 16 , 0  , 0  , 0  , 0  ), //      {R:r64, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 49 , 22 , 0  , 0  , 0  , 0  ), // #39  {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64|mem, R:i8|u8}
  ISIGNATURE(2, 1, 1, 0, 29 , 18 , 0  , 0  , 0  , 0  ), //      {X:r16|m16, R:i16|u16}
  ISIGNATURE(2, 1, 1, 0, 30 , 24 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, R:i32|u32}
  ISIGNATURE(2, 0, 1, 0, 31 , 32 , 0  , 0  , 0  , 0  ), //      {X:r64|m64|mem, R:i32|r64}
  ISIGNATURE(2, 1, 1, 0, 35 , 2  , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8|mem, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 36 , 37 , 0  , 0  , 0  , 0  ), //      {X:r16|m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 38 , 6  , 0  , 0  , 0  , 0  ), //      {X:r32|m32|mem, R:r32}
  ISIGNATURE(2, 1, 1, 0, 39 , 40 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, R:r8lo|r8hi|m8|mem}
  ISIGNATURE(2, 1, 1, 0, 41 , 12 , 0  , 0  , 0  , 0  ), //      {X:r16, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 42 , 43 , 0  , 0  , 0  , 0  ), //      {X:r32, R:r32|m32|mem}
  ISIGNATURE(2, 0, 1, 0, 44 , 16 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 1, 50 , 40 , 0  , 0  , 0  , 0  ), // #50  {X:<ax>, R:r8lo|r8hi|m8|mem}
  ISIGNATURE(3, 1, 1, 2, 51 , 50 , 12 , 0  , 0  , 0  ), //      {W:<dx>, X:<ax>, R:r16|m16|mem}
  ISIGNATURE(3, 1, 1, 2, 52 , 53 , 43 , 0  , 0  , 0  ), //      {W:<edx>, X:<eax>, R:r32|m32|mem}
  ISIGNATURE(3, 0, 1, 2, 54 , 55 , 16 , 0  , 0  , 0  ), //      {W:<rdx>, X:<rax>, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 41 , 56 , 0  , 0  , 0  , 0  ), //      {X:r16, R:r16|m16|mem|i8|i16}
  ISIGNATURE(2, 1, 1, 0, 42 , 57 , 0  , 0  , 0  , 0  ), //      {X:r32, R:r32|m32|mem|i8|i32}
  ISIGNATURE(2, 0, 1, 0, 44 , 58 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64|mem|i8|i32}
  ISIGNATURE(3, 1, 1, 0, 59 , 12 , 60 , 0  , 0  , 0  ), //      {W:r16, R:r16|m16|mem, R:i8|i16|u16}
  ISIGNATURE(3, 1, 1, 0, 13 , 43 , 61 , 0  , 0  , 0  ), //      {W:r32, R:r32|m32|mem, R:i8|i32|u32}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 62 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64|mem, R:i8|i32}
  ISIGNATURE(2, 1, 1, 0, 36 , 41 , 0  , 0  , 0  , 0  ), // #60  {X:r16|m16|mem, X:r16}
  ISIGNATURE(2, 1, 1, 0, 38 , 42 , 0  , 0  , 0  , 0  ), //      {X:r32|m32|mem, X:r32}
  ISIGNATURE(2, 0, 1, 0, 31 , 44 , 0  , 0  , 0  , 0  ), //      {X:r64|m64|mem, X:r64}
  ISIGNATURE(2, 1, 1, 0, 41 , 36 , 0  , 0  , 0  , 0  ), //      {X:r16, X:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 42 , 38 , 0  , 0  , 0  , 0  ), //      {X:r32, X:r32|m32|mem}
  ISIGNATURE(2, 0, 1, 0, 44 , 31 , 0  , 0  , 0  , 0  ), //      {X:r64, X:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 35 , 39 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8|mem, X:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 39 , 35 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, X:r8lo|r8hi|m8|mem}
  ISIGNATURE(2, 1, 1, 0, 45 , 22 , 0  , 0  , 0  , 0  ), // #68  {R:r8lo|r8hi|m8, R:i8|u8}
  ISIGNATURE(2, 1, 1, 0, 46 , 18 , 0  , 0  , 0  , 0  ), //      {R:r16|m16, R:i16|u16}
  ISIGNATURE(2, 1, 1, 0, 47 , 24 , 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:i32|u32}
  ISIGNATURE(2, 0, 1, 0, 16 , 32 , 0  , 0  , 0  , 0  ), //      {R:r64|m64|mem, R:i32|r64}
  ISIGNATURE(2, 1, 1, 0, 40 , 2  , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi|m8|mem, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 12 , 37 , 0  , 0  , 0  , 0  ), //      {R:r16|m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 43 , 6  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|mem, R:r32}
  ISIGNATURE(2, 1, 1, 0, 59 , 63 , 0  , 0  , 0  , 0  ), // #75  {W:r16, R:m16|mem}
  ISIGNATURE(2, 1, 1, 0, 13 , 64 , 0  , 0  , 0  , 0  ), //      {W:r32, R:m32|mem}
  ISIGNATURE(2, 0, 1, 0, 19 , 65 , 0  , 0  , 0  , 0  ), //      {W:r64, R:m64|mem}
  ISIGNATURE(2, 1, 1, 0, 66 , 37 , 0  , 0  , 0  , 0  ), //      {W:m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 67 , 6  , 0  , 0  , 0  , 0  ), // #79  {W:m32|mem, R:r32}
  ISIGNATURE(2, 0, 1, 0, 68 , 27 , 0  , 0  , 0  , 0  ), //      {W:m64|mem, R:r64}
  ISIGNATURE(2, 1, 1, 0, 69 , 70 , 0  , 0  , 0  , 0  ), // #81  {W:mm, R:mm|m64|mem|r64|xmm}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:mm|m64|mem|r64|xmm, R:mm}
  ISIGNATURE(2, 0, 1, 0, 7  , 73 , 0  , 0  , 0  , 0  ), //      {W:r64|m64|mem, R:xmm}
  ISIGNATURE(2, 0, 1, 0, 74 , 16 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 75 , 0  , 0  , 0  , 0  ), // #85  {W:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 76 , 73 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 77 , 0  , 0  , 0  , 0  ), // #87  {W:xmm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 78 , 73 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 79 , 80 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256|mem}
  ISIGNATURE(2, 1, 1, 0, 81 , 82 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 83 , 84 , 0  , 0  , 0  , 0  ), // #91  {W:zmm, R:zmm|m512|mem}
  ISIGNATURE(2, 1, 1, 0, 85 , 86 , 0  , 0  , 0  , 0  ), //      {W:zmm|m512|mem, R:zmm}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 77 , 0  , 0  , 0  ), // #93  {W:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 77 , 87 , 0  , 0  , 0  ), // #94  {W:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 79 , 82 , 80 , 0  , 0  , 0  ), // #95  {W:ymm, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(3, 1, 1, 0, 79 , 80 , 87 , 0  , 0  , 0  ), // #96  {W:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 83 , 86 , 84 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512|mem}
  ISIGNATURE(3, 1, 1, 0, 83 , 84 , 87 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 88 , 0  , 0  , 0  ), // #99  {W:xmm, R:xmm, R:u8|xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 79 , 82 , 88 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:u8|xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 77 , 87 , 0  , 0  , 0  ), // #101 {W:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 79 , 80 , 87 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 83 , 86 , 77 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 83 , 84 , 87 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 77 , 0  , 0  , 0  ), // #105 {W:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 77 , 87 , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 79 , 82 , 77 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 79 , 80 , 87 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 83 , 86 , 77 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 83 , 84 , 87 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 90 , 89 , 0  , 0  , 0  ), // #111 {X:xmm, R:vm32x, X:xmm}
  ISIGNATURE(3, 1, 1, 0, 91 , 90 , 91 , 0  , 0  , 0  ), //      {X:ymm, R:vm32x, X:ymm}
  ISIGNATURE(2, 1, 1, 0, 89 , 90 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:vm32x}
  ISIGNATURE(2, 1, 1, 0, 91 , 92 , 0  , 0  , 0  , 0  ), //      {X:ymm, R:vm32y}
  ISIGNATURE(2, 1, 1, 0, 93 , 94 , 0  , 0  , 0  , 0  ), //      {X:zmm, R:vm32z}
  ISIGNATURE(3, 1, 1, 0, 89 , 90 , 89 , 0  , 0  , 0  ), // #116 {X:xmm, R:vm32x, X:xmm}
  ISIGNATURE(3, 1, 1, 0, 91 , 92 , 91 , 0  , 0  , 0  ), //      {X:ymm, R:vm32y, X:ymm}
  ISIGNATURE(2, 1, 1, 0, 89 , 90 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:vm32x}
  ISIGNATURE(2, 1, 1, 0, 91 , 92 , 0  , 0  , 0  , 0  ), //      {X:ymm, R:vm32y}
  ISIGNATURE(2, 1, 1, 0, 93 , 94 , 0  , 0  , 0  , 0  ), //      {X:zmm, R:vm32z}
  ISIGNATURE(3, 1, 1, 0, 89 , 95 , 89 , 0  , 0  , 0  ), // #121 {X:xmm, R:vm64x, X:xmm}
  ISIGNATURE(3, 1, 1, 0, 91 , 96 , 91 , 0  , 0  , 0  ), //      {X:ymm, R:vm64y, X:ymm}
  ISIGNATURE(2, 1, 1, 0, 89 , 95 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:vm64x}
  ISIGNATURE(2, 1, 1, 0, 91 , 96 , 0  , 0  , 0  , 0  ), //      {X:ymm, R:vm64y}
  ISIGNATURE(2, 1, 1, 0, 93 , 97 , 0  , 0  , 0  , 0  ), //      {X:zmm, R:vm64z}
  ISIGNATURE(2, 1, 1, 0, 12 , 37 , 0  , 0  , 0  , 0  ), // #126 {R:r16|m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 43 , 6  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|mem, R:r32}
  ISIGNATURE(2, 0, 1, 0, 16 , 98 , 0  , 0  , 0  , 0  ), //      {R:r64|m64|mem, R:r64|u8}
  ISIGNATURE(2, 1, 1, 0, 99 , 87 , 0  , 0  , 0  , 0  ), //      {R:r16|m16|r32|m32, R:u8}
  ISIGNATURE(2, 1, 1, 0, 36 , 37 , 0  , 0  , 0  , 0  ), // #130 {X:r16|m16|mem, R:r16}
  ISIGNATURE(2, 1, 1, 0, 38 , 6  , 0  , 0  , 0  , 0  ), //      {X:r32|m32|mem, R:r32}
  ISIGNATURE(2, 0, 1, 0, 31 , 98 , 0  , 0  , 0  , 0  ), //      {X:r64|m64|mem, R:r64|u8}
  ISIGNATURE(2, 1, 1, 0, 100, 87 , 0  , 0  , 0  , 0  ), //      {X:r16|m16|r32|m32, R:u8}
  ISIGNATURE(3, 1, 1, 1, 35 , 2  , 101, 0  , 0  , 0  ), // #134 {X:r8lo|r8hi|m8|mem, R:r8lo|r8hi, R:<al>}
  ISIGNATURE(3, 1, 1, 1, 36 , 37 , 102, 0  , 0  , 0  ), //      {X:r16|m16|mem, R:r16, R:<ax>}
  ISIGNATURE(3, 1, 1, 1, 38 , 6  , 103, 0  , 0  , 0  ), //      {X:r32|m32|mem, R:r32, R:<eax>}
  ISIGNATURE(3, 0, 1, 1, 31 , 27 , 104, 0  , 0  , 0  ), //      {X:r64|m64|mem, R:r64, R:<rax>}
  ISIGNATURE(2, 1, 1, 1, 50 , 40 , 0  , 0  , 0  , 0  ), // #138 {X:<ax>, R:r8lo|r8hi|m8|mem}
  ISIGNATURE(3, 1, 1, 2, 105, 50 , 12 , 0  , 0  , 0  ), //      {X:<dx>, X:<ax>, R:r16|m16|mem}
  ISIGNATURE(3, 1, 1, 2, 106, 53 , 43 , 0  , 0  , 0  ), //      {X:<edx>, X:<eax>, R:r32|m32|mem}
  ISIGNATURE(3, 0, 1, 2, 107, 55 , 16 , 0  , 0  , 0  ), //      {X:<rdx>, X:<rax>, R:r64|m64|mem}
  ISIGNATURE(1, 1, 1, 0, 108, 0  , 0  , 0  , 0  , 0  ), // #142 {W:r16|m16|r64|m64|mem}
  ISIGNATURE(1, 1, 0, 0, 23 , 0  , 0  , 0  , 0  , 0  ), //      {W:r32|m32}
  ISIGNATURE(1, 1, 0, 0, 109, 0  , 0  , 0  , 0  , 0  ), //      {W:ds|es|ss}
  ISIGNATURE(1, 1, 1, 0, 110, 0  , 0  , 0  , 0  , 0  ), //      {W:fs|gs}
  ISIGNATURE(1, 1, 1, 0, 111, 0  , 0  , 0  , 0  , 0  ), // #146 {R:r16|m16|r64|m64|mem|i8|i16|i32}
  ISIGNATURE(1, 1, 0, 0, 47 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32}
  ISIGNATURE(1, 1, 0, 0, 112, 0  , 0  , 0  , 0  , 0  ), //      {R:cs|ss|ds|es}
  ISIGNATURE(1, 1, 1, 0, 113, 0  , 0  , 0  , 0  , 0  ), //      {R:fs|gs}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 73 , 77 , 0  , 0  ), // #150 {W:xmm, R:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 77 , 73 , 0  , 0  ), // #151 {W:xmm, R:xmm, R:xmm|m128|mem, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 79 , 82 , 82 , 80 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(4, 1, 1, 0, 79 , 82 , 80 , 82 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256|mem, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 89 , 114, 89 , 0  , 0  , 0  ), // #154 {X:xmm, R:vm64x|vm64y, X:xmm}
  ISIGNATURE(2, 1, 1, 0, 89 , 95 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:vm64x}
  ISIGNATURE(2, 1, 1, 0, 91 , 96 , 0  , 0  , 0  , 0  ), //      {X:ymm, R:vm64y}
  ISIGNATURE(2, 1, 1, 0, 93 , 97 , 0  , 0  , 0  , 0  ), //      {X:zmm, R:vm64z}
  ISIGNATURE(3, 1, 1, 0, 115, 73 , 73 , 0  , 0  , 0  ), // #158 {W:m128|mem, R:xmm, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 116, 82 , 82 , 0  , 0  , 0  ), //      {W:m256|mem, R:ymm, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 117, 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:m128|mem}
  ISIGNATURE(3, 1, 1, 0, 79 , 82 , 118, 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:m256|mem}
  ISIGNATURE(5, 1, 1, 0, 74 , 73 , 77 , 73 , 119, 0  ), // #162 {W:xmm, R:xmm, R:xmm|m128|mem, R:xmm, R:u4}
  ISIGNATURE(5, 1, 1, 0, 74 , 73 , 73 , 77 , 119, 0  ), //      {W:xmm, R:xmm, R:xmm, R:xmm|m128|mem, R:u4}
  ISIGNATURE(5, 1, 1, 0, 79 , 82 , 80 , 82 , 119, 0  ), //      {W:ymm, R:ymm, R:ymm|m256|mem, R:ymm, R:u4}
  ISIGNATURE(5, 1, 1, 0, 79 , 82 , 82 , 80 , 119, 0  ), //      {W:ymm, R:ymm, R:ymm, R:ymm|m256|mem, R:u4}
  ISIGNATURE(3, 1, 1, 0, 79 , 80 , 87 , 0  , 0  , 0  ), // #166 {W:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 79 , 82 , 80 , 0  , 0  , 0  ), // #167 {W:ymm, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(3, 1, 1, 0, 83 , 86 , 84 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512|mem}
  ISIGNATURE(3, 1, 1, 0, 83 , 84 , 87 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(2, 1, 1, 0, 35 , 39 , 0  , 0  , 0  , 0  ), // #170 {X:r8lo|r8hi|m8|mem, X:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 36 , 41 , 0  , 0  , 0  , 0  ), //      {X:r16|m16|mem, X:r16}
  ISIGNATURE(2, 1, 1, 0, 38 , 42 , 0  , 0  , 0  , 0  ), //      {X:r32|m32|mem, X:r32}
  ISIGNATURE(2, 0, 1, 0, 31 , 44 , 0  , 0  , 0  , 0  ), //      {X:r64|m64|mem, X:r64}
  ISIGNATURE(1, 1, 1, 0, 120, 0  , 0  , 0  , 0  , 0  ), // #174 {R:m32|m64}
  ISIGNATURE(2, 1, 1, 0, 121, 122, 0  , 0  , 0  , 0  ), //      {X:fp0, R:fp}
  ISIGNATURE(2, 1, 1, 0, 123, 124, 0  , 0  , 0  , 0  ), //      {X:fp, R:fp0}
  ISIGNATURE(1, 1, 1, 0, 125, 0  , 0  , 0  , 0  , 0  ), // #177 {X:m32|m64}
  ISIGNATURE(2, 1, 1, 0, 121, 122, 0  , 0  , 0  , 0  ), //      {X:fp0, R:fp}
  ISIGNATURE(2, 1, 1, 0, 123, 124, 0  , 0  , 0  , 0  ), //      {X:fp, R:fp0}
  ISIGNATURE(2, 1, 1, 0, 41 , 64 , 0  , 0  , 0  , 0  ), // #180 {X:r16, R:m32|mem}
  ISIGNATURE(2, 1, 1, 0, 42 , 126, 0  , 0  , 0  , 0  ), //      {X:r32, R:m48|mem}
  ISIGNATURE(2, 0, 1, 0, 44 , 127, 0  , 0  , 0  , 0  ), //      {X:r64, R:m80|mem}
  ISIGNATURE(2, 1, 1, 0, 59 , 12 , 0  , 0  , 0  , 0  ), // #183 {W:r16, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 13 , 43 , 0  , 0  , 0  , 0  ), // #184 {W:r32, R:r32|m32|mem}
  ISIGNATURE(2, 0, 1, 0, 19 , 16 , 0  , 0  , 0  , 0  ), //      {W:r64, R:r64|m64|mem}
  ISIGNATURE(3, 1, 1, 0, 36 , 37 , 128, 0  , 0  , 0  ), // #186 {X:r16|m16|mem, R:r16, R:u8|cl}
  ISIGNATURE(3, 1, 1, 0, 38 , 6  , 128, 0  , 0  , 0  ), //      {X:r32|m32|mem, R:r32, R:u8|cl}
  ISIGNATURE(3, 0, 1, 0, 31 , 27 , 128, 0  , 0  , 0  ), //      {X:r64|m64|mem, R:r64, R:u8|cl}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 77 , 0  , 0  , 0  ), // #189 {W:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 79 , 82 , 80 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(3, 1, 1, 0, 83 , 86 , 84 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512|mem}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 77 , 87 , 0  , 0  ), // #192 {W:xmm, R:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 79 , 82 , 80 , 87 , 0  , 0  ), // #193 {W:ymm, R:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 83 , 86 , 84 , 87 , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 129, 73 , 77 , 87 , 0  , 0  ), // #195 {W:xmm|k, R:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 130, 82 , 80 , 87 , 0  , 0  ), //      {W:ymm|k, R:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 131, 86 , 84 , 87 , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(2, 1, 1, 0, 78 , 73 , 0  , 0  , 0  , 0  ), // #198 {W:xmm|m128|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 81 , 82 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 85 , 86 , 0  , 0  , 0  , 0  ), //      {W:zmm|m512|mem, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 75 , 0  , 0  , 0  , 0  ), // #201 {W:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 77 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 80 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:ymm|m256|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 77 , 0  , 0  , 0  , 0  ), // #204 {W:xmm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 80 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 84 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 132, 0  , 0  , 0  , 0  ), // #207 {W:xmm, R:xmm|m128|ymm|m256|m64}
  ISIGNATURE(2, 1, 1, 0, 79 , 133, 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 83 , 134, 0  , 0  , 0  , 0  ), //      {W:zmm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 76 , 73 , 87 , 0  , 0  , 0  ), // #210 {W:xmm|m64|mem, R:xmm, R:u8}
  ISIGNATURE(3, 1, 1, 0, 78 , 82 , 87 , 0  , 0  , 0  ), // #211 {W:xmm|m128|mem, R:ymm, R:u8}
  ISIGNATURE(3, 1, 1, 0, 81 , 86 , 87 , 0  , 0  , 0  ), // #212 {W:ymm|m256|mem, R:zmm, R:u8}
  ISIGNATURE(4, 1, 1, 0, 89 , 73 , 77 , 87 , 0  , 0  ), // #213 {X:xmm, R:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 91 , 82 , 80 , 87 , 0  , 0  ), //      {X:ymm, R:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 93 , 86 , 84 , 87 , 0  , 0  ), //      {X:zmm, R:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 73 , 77 , 0  , 0  , 0  ), // #216 {X:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 91 , 82 , 80 , 0  , 0  , 0  ), //      {X:ymm, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(3, 1, 1, 0, 93 , 86 , 84 , 0  , 0  , 0  ), //      {X:zmm, R:zmm, R:zmm|m512|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 77 , 87 , 0  , 0  , 0  ), // #219 {W:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 79 , 80 , 87 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 83 , 84 , 87 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(2, 1, 1, 0, 74 , 75 , 0  , 0  , 0  , 0  ), // #222 {W:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 80 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 84 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512|mem}
  ISIGNATURE(2, 1, 1, 0, 115, 73 , 0  , 0  , 0  , 0  ), // #225 {W:m128|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 116, 82 , 0  , 0  , 0  , 0  ), //      {W:m256|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 135, 86 , 0  , 0  , 0  , 0  ), //      {W:m512|mem, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 117, 0  , 0  , 0  , 0  ), // #228 {W:xmm, R:m128|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 118, 0  , 0  , 0  , 0  ), //      {W:ymm, R:m256|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 136, 0  , 0  , 0  , 0  ), //      {W:zmm, R:m512|mem}
  ISIGNATURE(2, 0, 1, 0, 7  , 73 , 0  , 0  , 0  , 0  ), // #231 {W:r64|m64|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 137, 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m64|mem|r64}
  ISIGNATURE(2, 1, 1, 0, 76 , 73 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 73 , 0  , 0  , 0  , 0  ), // #234 {W:m64|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:m64|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 73 , 0  , 0  , 0  ), // #236 {W:xmm, R:xmm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 67 , 73 , 0  , 0  , 0  , 0  ), // #237 {W:m32|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 64 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:m32|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 73 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 131, 73 , 77 , 87 , 0  , 0  ), // #240 {W:k, R:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 131, 82 , 80 , 87 , 0  , 0  ), //      {W:k, R:ymm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 131, 86 , 84 , 87 , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 129, 73 , 77 , 0  , 0  , 0  ), // #243 {W:xmm|k, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 130, 82 , 80 , 0  , 0  , 0  ), //      {W:ymm|k, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(3, 1, 1, 0, 131, 86 , 84 , 0  , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512|mem}
  ISIGNATURE(2, 1, 1, 0, 138, 73 , 0  , 0  , 0  , 0  ), // #246 {W:xmm|m32|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 76 , 82 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 78 , 86 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128|mem, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 76 , 73 , 0  , 0  , 0  , 0  ), // #249 {W:xmm|m64|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 78 , 82 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 81 , 86 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256|mem, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 139, 73 , 0  , 0  , 0  , 0  ), // #252 {W:xmm|m16|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 138, 82 , 0  , 0  , 0  , 0  ), //      {W:xmm|m32|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 76 , 86 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64|mem, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 140, 0  , 0  , 0  , 0  ), // #255 {W:xmm, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 75 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 77 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 141, 0  , 0  , 0  , 0  ), // #258 {W:xmm, R:xmm|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 140, 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 75 , 0  , 0  , 0  , 0  ), // #260 {W:zmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 75 , 0  , 0  , 0  , 0  ), // #261 {W:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 77 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 142, 0  , 0  , 0  , 0  ), //      {W:zmm, R:xmm|m256|mem}
  ISIGNATURE(2, 1, 1, 0, 143, 73 , 0  , 0  , 0  , 0  ), // #264 {W:vm32x, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 144, 82 , 0  , 0  , 0  , 0  ), //      {W:vm32y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 145, 86 , 0  , 0  , 0  , 0  ), //      {W:vm32z, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 146, 73 , 0  , 0  , 0  , 0  ), // #267 {W:vm64x, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 147, 82 , 0  , 0  , 0  , 0  ), //      {W:vm64y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 148, 86 , 0  , 0  , 0  , 0  ), //      {W:vm64z, R:zmm}
  ISIGNATURE(3, 1, 1, 0, 131, 73 , 77 , 0  , 0  , 0  ), // #270 {W:k, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 131, 82 , 80 , 0  , 0  , 0  ), //      {W:k, R:ymm, R:ymm|m256|mem}
  ISIGNATURE(3, 1, 1, 0, 131, 86 , 84 , 0  , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512|mem}
  ISIGNATURE(3, 1, 1, 0, 13 , 6  , 43 , 0  , 0  , 0  ), // #273 {W:r32, R:r32, R:r32|m32|mem}
  ISIGNATURE(3, 0, 1, 0, 19 , 27 , 16 , 0  , 0  , 0  ), //      {W:r64, R:r64, R:r64|m64|mem}
  ISIGNATURE(3, 1, 1, 0, 13 , 43 , 6  , 0  , 0  , 0  ), // #275 {W:r32, R:r32|m32|mem, R:r32}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 27 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64|mem, R:r64}
  ISIGNATURE(2, 1, 0, 0, 149, 43 , 0  , 0  , 0  , 0  ), // #277 {R:bnd, R:r32|m32|mem}
  ISIGNATURE(2, 0, 1, 0, 149, 16 , 0  , 0  , 0  , 0  ), //      {R:bnd, R:r64|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 150, 151, 0  , 0  , 0  , 0  ), // #279 {W:bnd, R:bnd|mem}
  ISIGNATURE(2, 1, 1, 0, 152, 149, 0  , 0  , 0  , 0  ), //      {W:bnd|mem, R:bnd}
  ISIGNATURE(2, 1, 0, 0, 37 , 64 , 0  , 0  , 0  , 0  ), // #281 {R:r16, R:m32|mem}
  ISIGNATURE(2, 1, 0, 0, 6  , 65 , 0  , 0  , 0  , 0  ), //      {R:r32, R:m64|mem}
  ISIGNATURE(1, 1, 1, 0, 153, 0  , 0  , 0  , 0  , 0  ), // #283 {R:rel32|r64|m64|mem}
  ISIGNATURE(1, 1, 0, 0, 43 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 42 , 154, 0  , 0  , 0  , 0  ), // #285 {X:r32, R:r8lo|r8hi|m8|r16|m16|r32|m32}
  ISIGNATURE(2, 0, 1, 0, 44 , 155, 0  , 0  , 0  , 0  ), //      {X:r64, R:r8lo|r8hi|m8|r64|m64}
  ISIGNATURE(1, 1, 0, 0, 156, 0  , 0  , 0  , 0  , 0  ), // #287 {X:r16|r32}
  ISIGNATURE(1, 1, 1, 0, 49 , 0  , 0  , 0  , 0  , 0  ), // #288 {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64|mem}
  ISIGNATURE(3, 1, 1, 0, 89 , 87 , 87 , 0  , 0  , 0  ), // #289 {X:xmm, R:u8, R:u8}
  ISIGNATURE(2, 1, 1, 0, 89 , 73 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:xmm}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #291 {}
  ISIGNATURE(1, 1, 1, 0, 123, 0  , 0  , 0  , 0  , 0  ), // #292 {X:fp}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #293 {}
  ISIGNATURE(1, 1, 1, 0, 157, 0  , 0  , 0  , 0  , 0  ), //      {R:m32|m64|fp}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #295 {}
  ISIGNATURE(1, 1, 1, 0, 122, 0  , 0  , 0  , 0  , 0  ), // #296 {R:fp}
  ISIGNATURE(2, 1, 1, 0, 89 , 73 , 0  , 0  , 0  , 0  ), // #297 {X:xmm, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 89 , 73 , 87 , 87 , 0  , 0  ), //      {X:xmm, R:xmm, R:u8, R:u8}
  ISIGNATURE(2, 1, 0, 0, 6  , 117, 0  , 0  , 0  , 0  ), // #299 {R:r32, R:m128|mem}
  ISIGNATURE(2, 0, 1, 0, 27 , 117, 0  , 0  , 0  , 0  ), //      {R:r64, R:m128|mem}
  ISIGNATURE(2, 1, 0, 1, 158, 159, 0  , 0  , 0  , 0  ), // #301 {R:<cx|ecx>, R:rel8}
  ISIGNATURE(2, 0, 1, 1, 160, 159, 0  , 0  , 0  , 0  ), //      {R:<ecx|rcx>, R:rel8}
  ISIGNATURE(1, 1, 1, 0, 161, 0  , 0  , 0  , 0  , 0  ), // #303 {R:rel8|rel32|r64|m64|mem}
  ISIGNATURE(1, 1, 0, 0, 43 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 131, 162, 0  , 0  , 0  , 0  ), // #305 {W:k, R:k|m8|mem|r32|r64|r8lo|r8hi|r16}
  ISIGNATURE(2, 1, 1, 0, 163, 164, 0  , 0  , 0  , 0  ), //      {W:m8|mem|r32|r64|r8lo|r8hi|r16, R:k}
  ISIGNATURE(2, 1, 1, 0, 131, 165, 0  , 0  , 0  , 0  ), // #307 {W:k, R:k|m32|mem|r32|r64}
  ISIGNATURE(2, 1, 1, 0, 166, 164, 0  , 0  , 0  , 0  ), //      {W:m32|mem|r32|r64, R:k}
  ISIGNATURE(2, 1, 1, 0, 131, 167, 0  , 0  , 0  , 0  ), // #309 {W:k, R:k|m64|mem|r64}
  ISIGNATURE(2, 1, 1, 0, 7  , 164, 0  , 0  , 0  , 0  ), //      {W:m64|mem|r64, R:k}
  ISIGNATURE(2, 1, 1, 0, 131, 168, 0  , 0  , 0  , 0  ), // #311 {W:k, R:k|m16|mem|r32|r64|r16}
  ISIGNATURE(2, 1, 1, 0, 169, 164, 0  , 0  , 0  , 0  ), //      {W:m16|mem|r32|r64|r16, R:k}
  ISIGNATURE(2, 1, 1, 0, 59 , 12 , 0  , 0  , 0  , 0  ), // #313 {W:r16, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 13 , 170, 0  , 0  , 0  , 0  ), //      {W:r32, R:r32|m16|mem|r16}
  ISIGNATURE(2, 1, 0, 0, 41 , 64 , 0  , 0  , 0  , 0  ), // #315 {X:r16, R:m32|mem}
  ISIGNATURE(2, 1, 0, 0, 42 , 126, 0  , 0  , 0  , 0  ), //      {X:r32, R:m48|mem}
  ISIGNATURE(2, 1, 0, 1, 171, 159, 0  , 0  , 0  , 0  ), // #317 {X:<cx|ecx>, R:rel8}
  ISIGNATURE(2, 0, 1, 1, 172, 159, 0  , 0  , 0  , 0  ), //      {X:<ecx|rcx>, R:rel8}
  ISIGNATURE(2, 1, 1, 0, 59 , 12 , 0  , 0  , 0  , 0  ), // #319 {W:r16, R:r16|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 173, 170, 0  , 0  , 0  , 0  ), //      {W:r32|r64, R:r32|m16|mem|r16}
  ISIGNATURE(2, 1, 1, 0, 174, 175, 0  , 0  , 0  , 0  ), // #321 {W:mm|xmm, R:r32|m32|mem|r64}
  ISIGNATURE(2, 1, 1, 0, 166, 176, 0  , 0  , 0  , 0  ), //      {W:r32|m32|mem|r64, R:mm|xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 75 , 0  , 0  , 0  , 0  ), // #323 {W:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 68 , 73 , 0  , 0  , 0  , 0  ), //      {W:m64|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 140, 0  , 0  , 0  , 0  ), // #325 {W:xmm, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 67 , 73 , 0  , 0  , 0  , 0  ), //      {W:m32|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 177, 45 , 0  , 0  , 0  , 0  ), // #327 {W:r16|r32|r64, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 173, 46 , 0  , 0  , 0  , 0  ), //      {W:r32|r64, R:r16|m16}
  ISIGNATURE(4, 1, 1, 1, 13 , 13 , 43 , 178, 0  , 0  ), // #329 {W:r32, W:r32, R:r32|m32|mem, R:<edx>}
  ISIGNATURE(4, 0, 1, 1, 19 , 19 , 16 , 179, 0  , 0  ), //      {W:r64, W:r64, R:r64|m64|mem, R:<rdx>}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #331 {}
  ISIGNATURE(1, 1, 1, 0, 99 , 0  , 0  , 0  , 0  , 0  ), //      {R:r16|m16|r32|m32}
  ISIGNATURE(2, 1, 1, 0, 69 , 180, 0  , 0  , 0  , 0  ), // #333 {W:mm, R:mm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 77 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 181, 180, 0  , 0  , 0  , 0  ), // #335 {X:mm, R:mm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 89 , 77 , 0  , 0  , 0  , 0  ), // #336 {X:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 181, 180, 87 , 0  , 0  , 0  ), // #337 {X:mm, R:mm|m64|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 77 , 87 , 0  , 0  , 0  ), // #338 {X:xmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 173, 72 , 87 , 0  , 0  , 0  ), // #339 {W:r32|r64, R:mm, R:u8}
  ISIGNATURE(3, 1, 1, 0, 169, 73 , 87 , 0  , 0  , 0  ), // #340 {W:r32|r64|m16|mem|r16, R:xmm, R:u8}
  ISIGNATURE(2, 1, 1, 0, 181, 182, 0  , 0  , 0  , 0  ), // #341 {X:mm, R:u8|mm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 89 , 88 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:u8|xmm|m128|mem}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #343 {}
  ISIGNATURE(1, 1, 1, 0, 183, 0  , 0  , 0  , 0  , 0  ), //      {R:u16}
  ISIGNATURE(3, 1, 1, 0, 13 , 43 , 87 , 0  , 0  , 0  ), // #345 {W:r32, R:r32|m32|mem, R:u8}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 87 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 77 , 73 , 0  , 0  ), // #347 {W:xmm, R:xmm, R:xmm|m128|mem, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 79 , 82 , 80 , 82 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256|mem, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 74 , 184, 0  , 0  , 0  , 0  ), // #349 {W:xmm, R:xmm|m128|ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 79 , 84 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:zmm|m512|mem}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 73 , 75 , 0  , 0  ), // #351 {W:xmm, R:xmm, R:xmm, R:xmm|m64|mem}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 75 , 73 , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm|m64|mem, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 73 , 140, 0  , 0  ), // #353 {W:xmm, R:xmm, R:xmm, R:xmm|m32|mem}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 140, 73 , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm|m32|mem, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 79 , 82 , 77 , 87 , 0  , 0  ), // #355 {W:ymm, R:ymm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 83 , 86 , 77 , 87 , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128|mem, R:u8}
  ISIGNATURE(2, 1, 1, 0, 166, 73 , 0  , 0  , 0  , 0  ), // #357 {W:r32|m32|mem|r64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 175, 0  , 0  , 0  , 0  ), //      {W:xmm, R:r32|m32|mem|r64}
  ISIGNATURE(2, 1, 1, 0, 68 , 73 , 0  , 0  , 0  , 0  ), // #359 {W:m64|mem, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 65 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:m64|mem}
  ISIGNATURE(2, 1, 1, 0, 185, 186, 0  , 0  , 0  , 0  ), // #361 {W:xmm|ymm|zmm, R:xmm|m8|mem}
  ISIGNATURE(2, 1, 1, 0, 185, 187, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(2, 1, 1, 0, 185, 140, 0  , 0  , 0  , 0  ), // #363 {W:xmm|ymm|zmm, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 185, 187, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(2, 1, 1, 0, 185, 141, 0  , 0  , 0  , 0  ), // #365 {W:xmm|ymm|zmm, R:xmm|m16|mem}
  ISIGNATURE(2, 1, 1, 0, 185, 187, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(3, 1, 1, 0, 74 , 188, 87 , 0  , 0  , 0  ), // #367 {W:xmm, R:r32|m8|mem|r8lo|r8hi|r16|r64, R:u8}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 188, 87 , 0  , 0  ), //      {W:xmm, R:xmm, R:r32|m8|mem|r8lo|r8hi|r16|r64, R:u8}
  ISIGNATURE(3, 1, 1, 0, 74 , 175, 87 , 0  , 0  , 0  ), // #369 {W:xmm, R:r32|m32|mem|r64, R:u8}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 175, 87 , 0  , 0  ), //      {W:xmm, R:xmm, R:r32|m32|mem|r64, R:u8}
  ISIGNATURE(3, 0, 1, 0, 74 , 16 , 87 , 0  , 0  , 0  ), // #371 {W:xmm, R:r64|m64|mem, R:u8}
  ISIGNATURE(4, 0, 1, 0, 74 , 73 , 16 , 87 , 0  , 0  ), //      {W:xmm, R:xmm, R:r64|m64|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 77 , 0  , 0  , 0  ), // #373 {W:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 77 , 189, 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128|mem, R:u8|xmm}
  ISIGNATURE(2, 1, 1, 0, 190, 73 , 0  , 0  , 0  , 0  ), // #375 {W:vm64x|vm64y, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 148, 82 , 0  , 0  , 0  , 0  ), //      {W:vm64z, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 77 , 0  , 0  , 0  ), // #377 {W:xmm, R:xmm, R:xmm|m128|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 77 , 73 , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128|mem, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 73 , 77 , 0  , 0  , 0  , 0  ), // #379 {R:xmm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 82 , 80 , 0  , 0  , 0  , 0  ), //      {R:ymm, R:ymm|m256|mem}
  ISIGNATURE(2, 1, 1, 0, 143, 191, 0  , 0  , 0  , 0  ), // #381 {W:vm32x, R:xmm|ymm}
  ISIGNATURE(2, 1, 1, 0, 144, 86 , 0  , 0  , 0  , 0  ), //      {W:vm32y, R:zmm}
  ISIGNATURE(1, 1, 0, 1, 50 , 0  , 0  , 0  , 0  , 0  ), // #383 {X:<ax>}
  ISIGNATURE(2, 1, 0, 1, 50 , 87 , 0  , 0  , 0  , 0  ), // #384 {X:<ax>, R:u8}
  ISIGNATURE(2, 1, 1, 0, 89 , 75 , 0  , 0  , 0  , 0  ), // #385 {X:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 89 , 140, 0  , 0  , 0  , 0  ), // #386 {X:xmm, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 0, 0, 36 , 37 , 0  , 0  , 0  , 0  ), // #387 {X:r16|m16|mem, R:r16}
  ISIGNATURE(3, 1, 1, 1, 89 , 77 , 192, 0  , 0  , 0  ), // #388 {X:xmm, R:xmm|m128|mem, R:<xmm0>}
  ISIGNATURE(2, 1, 1, 0, 150, 193, 0  , 0  , 0  , 0  ), // #389 {W:bnd, R:mib}
  ISIGNATURE(2, 1, 1, 0, 150, 194, 0  , 0  , 0  , 0  ), // #390 {W:bnd, R:mem}
  ISIGNATURE(2, 1, 1, 0, 195, 149, 0  , 0  , 0  , 0  ), // #391 {W:mib, R:bnd}
  ISIGNATURE(1, 1, 1, 0, 196, 0  , 0  , 0  , 0  , 0  ), // #392 {X:r32|r64}
  ISIGNATURE(1, 1, 1, 1, 50 , 0  , 0  , 0  , 0  , 0  ), // #393 {X:<ax>}
  ISIGNATURE(2, 1, 1, 2, 52 , 103, 0  , 0  , 0  , 0  ), // #394 {W:<edx>, R:<eax>}
  ISIGNATURE(1, 0, 1, 1, 55 , 0  , 0  , 0  , 0  , 0  ), // #395 {X:<rax>}
  ISIGNATURE(1, 1, 1, 0, 194, 0  , 0  , 0  , 0  , 0  ), // #396 {R:mem}
  ISIGNATURE(1, 1, 1, 1, 197, 0  , 0  , 0  , 0  , 0  ), // #397 {R:<ds:[zax]>}
  ISIGNATURE(2, 1, 1, 2, 198, 199, 0  , 0  , 0  , 0  ), // #398 {X:<ds:[zsi]>, X:<es:[zdi]>}
  ISIGNATURE(3, 1, 1, 0, 89 , 75 , 87 , 0  , 0  , 0  ), // #399 {X:xmm, R:xmm|m64|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 140, 87 , 0  , 0  , 0  ), // #400 {X:xmm, R:xmm|m32|mem, R:u8}
  ISIGNATURE(5, 0, 1, 4, 200, 107, 55 , 201, 202, 0  ), // #401 {X:m128|mem, X:<rdx>, X:<rax>, R:<rcx>, R:<rbx>}
  ISIGNATURE(5, 1, 1, 4, 203, 106, 53 , 204, 205, 0  ), // #402 {X:m64|mem, X:<edx>, X:<eax>, R:<ecx>, R:<ebx>}
  ISIGNATURE(2, 1, 1, 0, 73 , 75 , 0  , 0  , 0  , 0  ), // #403 {R:xmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 73 , 140, 0  , 0  , 0  , 0  ), // #404 {R:xmm, R:xmm|m32|mem}
  ISIGNATURE(4, 1, 1, 4, 53 , 206, 207, 52 , 0  , 0  ), // #405 {X:<eax>, W:<ebx>, X:<ecx>, W:<edx>}
  ISIGNATURE(2, 0, 1, 2, 54 , 104, 0  , 0  , 0  , 0  ), // #406 {W:<rdx>, R:<rax>}
  ISIGNATURE(2, 1, 1, 0, 69 , 77 , 0  , 0  , 0  , 0  ), // #407 {W:mm, R:xmm|m128|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 180, 0  , 0  , 0  , 0  ), // #408 {W:xmm, R:mm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 69 , 75 , 0  , 0  , 0  , 0  ), // #409 {W:mm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 173, 75 , 0  , 0  , 0  , 0  ), // #410 {W:r32|r64, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 74 , 208, 0  , 0  , 0  , 0  ), // #411 {W:xmm, R:r32|m32|mem|r64|m64}
  ISIGNATURE(2, 1, 1, 0, 173, 140, 0  , 0  , 0  , 0  ), // #412 {W:r32|r64, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 1, 2, 51 , 102, 0  , 0  , 0  , 0  ), // #413 {W:<dx>, R:<ax>}
  ISIGNATURE(1, 1, 1, 1, 53 , 0  , 0  , 0  , 0  , 0  ), // #414 {X:<eax>}
  ISIGNATURE(2, 1, 1, 0, 183, 87 , 0  , 0  , 0  , 0  ), // #415 {R:u16, R:u8}
  ISIGNATURE(3, 1, 1, 0, 166, 73 , 87 , 0  , 0  , 0  ), // #416 {W:r32|m32|mem|r64, R:xmm, R:u8}
  ISIGNATURE(1, 1, 1, 0, 127, 0  , 0  , 0  , 0  , 0  ), // #417 {R:m80|mem}
  ISIGNATURE(1, 1, 1, 0, 209, 0  , 0  , 0  , 0  , 0  ), // #418 {W:m80|mem}
  ISIGNATURE(1, 1, 1, 0, 210, 0  , 0  , 0  , 0  , 0  ), // #419 {R:m16|m32}
  ISIGNATURE(1, 1, 1, 0, 211, 0  , 0  , 0  , 0  , 0  ), // #420 {R:m16|m32|m64}
  ISIGNATURE(1, 1, 1, 0, 212, 0  , 0  , 0  , 0  , 0  ), // #421 {W:m16|m32}
  ISIGNATURE(1, 1, 1, 0, 213, 0  , 0  , 0  , 0  , 0  ), // #422 {W:m16|m32|m64}
  ISIGNATURE(1, 1, 1, 0, 214, 0  , 0  , 0  , 0  , 0  ), // #423 {R:m32|m64|m80|fp}
  ISIGNATURE(1, 1, 1, 0, 63 , 0  , 0  , 0  , 0  , 0  ), // #424 {R:m16|mem}
  ISIGNATURE(1, 1, 1, 0, 215, 0  , 0  , 0  , 0  , 0  ), // #425 {W:mem}
  ISIGNATURE(1, 1, 1, 0, 66 , 0  , 0  , 0  , 0  , 0  ), // #426 {W:m16|mem}
  ISIGNATURE(1, 1, 1, 0, 216, 0  , 0  , 0  , 0  , 0  ), // #427 {W:ax|m16|mem}
  ISIGNATURE(1, 1, 1, 0, 217, 0  , 0  , 0  , 0  , 0  ), // #428 {W:m32|m64|fp}
  ISIGNATURE(1, 1, 1, 0, 218, 0  , 0  , 0  , 0  , 0  ), // #429 {W:m32|m64|m80|fp}
  ISIGNATURE(1, 0, 1, 0, 194, 0  , 0  , 0  , 0  , 0  ), // #430 {R:mem}
  ISIGNATURE(1, 0, 1, 0, 215, 0  , 0  , 0  , 0  , 0  ), // #431 {W:mem}
  ISIGNATURE(2, 1, 1, 0, 219, 220, 0  , 0  , 0  , 0  ), // #432 {W:al|ax|eax, R:u8|dx}
  ISIGNATURE(2, 1, 1, 0, 221, 222, 0  , 0  , 0  , 0  ), // #433 {W:es:[zdi], R:dx}
  ISIGNATURE(1, 1, 1, 0, 87 , 0  , 0  , 0  , 0  , 0  ), // #434 {R:u8}
  ISIGNATURE(0, 1, 0, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #435 {}
  ISIGNATURE(0, 0, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #436 {}
  ISIGNATURE(1, 1, 1, 0, 223, 0  , 0  , 0  , 0  , 0  ), // #437 {R:rel8|rel32}
  ISIGNATURE(1, 1, 1, 0, 159, 0  , 0  , 0  , 0  , 0  ), // #438 {R:rel8}
  ISIGNATURE(3, 1, 1, 0, 131, 164, 164, 0  , 0  , 0  ), // #439 {W:k, R:k, R:k}
  ISIGNATURE(2, 1, 1, 0, 131, 164, 0  , 0  , 0  , 0  ), // #440 {W:k, R:k}
  ISIGNATURE(2, 1, 1, 0, 164, 164, 0  , 0  , 0  , 0  ), // #441 {R:k, R:k}
  ISIGNATURE(3, 1, 1, 0, 131, 164, 87 , 0  , 0  , 0  ), // #442 {W:k, R:k, R:u8}
  ISIGNATURE(1, 1, 1, 1, 224, 0  , 0  , 0  , 0  , 0  ), // #443 {W:<ah>}
  ISIGNATURE(1, 1, 1, 0, 64 , 0  , 0  , 0  , 0  , 0  ), // #444 {R:m32|mem}
  ISIGNATURE(2, 1, 1, 0, 177, 225, 0  , 0  , 0  , 0  ), // #445 {W:r16|r32|r64, R:mem|m8|m16|m32|m48|m64|m80|m128|m256|m512|m1024}
  ISIGNATURE(1, 1, 1, 0, 226, 0  , 0  , 0  , 0  , 0  ), // #446 {R:r16|m16|mem|r32|r64}
  ISIGNATURE(2, 1, 1, 2, 227, 198, 0  , 0  , 0  , 0  ), // #447 {W:<al|ax|eax|rax>, X:<ds:[zsi]>}
  ISIGNATURE(3, 1, 1, 1, 89 , 73 , 228, 0  , 0  , 0  ), // #448 {X:xmm, R:xmm, R:<ds:[zdi]>}
  ISIGNATURE(3, 1, 1, 1, 181, 72 , 228, 0  , 0  , 0  ), // #449 {X:mm, R:mm, R:<ds:[zdi]>}
  ISIGNATURE(3, 1, 1, 3, 197, 204, 178, 0  , 0  , 0  ), // #450 {R:<ds:[zax]>, R:<ecx>, R:<edx>}
  ISIGNATURE(2, 1, 1, 0, 69 , 73 , 0  , 0  , 0  , 0  ), // #451 {W:mm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 74 , 73 , 0  , 0  , 0  , 0  ), // #452 {W:xmm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 173, 73 , 0  , 0  , 0  , 0  ), // #453 {W:r32|r64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 72 , 0  , 0  , 0  , 0  ), // #454 {W:m64|mem, R:mm}
  ISIGNATURE(2, 1, 1, 0, 74 , 72 , 0  , 0  , 0  , 0  ), // #455 {W:xmm, R:mm}
  ISIGNATURE(2, 1, 1, 2, 199, 198, 0  , 0  , 0  , 0  ), // #456 {X:<es:[zdi]>, X:<ds:[zsi]>}
  ISIGNATURE(2, 0, 1, 0, 19 , 43 , 0  , 0  , 0  , 0  ), // #457 {W:r64, R:r32|m32|mem}
  ISIGNATURE(2, 1, 1, 2, 103, 204, 0  , 0  , 0  , 0  ), // #458 {R:<eax>, R:<ecx>}
  ISIGNATURE(2, 1, 1, 0, 220, 229, 0  , 0  , 0  , 0  ), // #459 {R:u8|dx, R:al|ax|eax}
  ISIGNATURE(2, 1, 1, 0, 222, 230, 0  , 0  , 0  , 0  ), // #460 {R:dx, R:ds:[zsi]}
  ISIGNATURE(6, 1, 1, 3, 73 , 77 , 87 , 231, 103, 178), // #461 {R:xmm, R:xmm|m128|mem, R:u8, W:<ecx>, R:<eax>, R:<edx>}
  ISIGNATURE(6, 1, 1, 3, 73 , 77 , 87 , 232, 103, 178), // #462 {R:xmm, R:xmm|m128|mem, R:u8, W:<xmm0>, R:<eax>, R:<edx>}
  ISIGNATURE(4, 1, 1, 1, 73 , 77 , 87 , 231, 0  , 0  ), // #463 {R:xmm, R:xmm|m128|mem, R:u8, W:<ecx>}
  ISIGNATURE(4, 1, 1, 1, 73 , 77 , 87 , 232, 0  , 0  ), // #464 {R:xmm, R:xmm|m128|mem, R:u8, W:<xmm0>}
  ISIGNATURE(3, 1, 1, 0, 163, 73 , 87 , 0  , 0  , 0  ), // #465 {W:r32|m8|mem|r8lo|r8hi|r16|r64, R:xmm, R:u8}
  ISIGNATURE(3, 0, 1, 0, 7  , 73 , 87 , 0  , 0  , 0  ), // #466 {W:r64|m64|mem, R:xmm, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 188, 87 , 0  , 0  , 0  ), // #467 {X:xmm, R:r32|m8|mem|r8lo|r8hi|r16|r64, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 175, 87 , 0  , 0  , 0  ), // #468 {X:xmm, R:r32|m32|mem|r64, R:u8}
  ISIGNATURE(3, 0, 1, 0, 89 , 16 , 87 , 0  , 0  , 0  ), // #469 {X:xmm, R:r64|m64|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 233, 226, 87 , 0  , 0  , 0  ), // #470 {X:mm|xmm, R:r32|m16|mem|r16|r64, R:u8}
  ISIGNATURE(2, 1, 1, 0, 173, 176, 0  , 0  , 0  , 0  ), // #471 {W:r32|r64, R:mm|xmm}
  ISIGNATURE(3, 1, 1, 0, 69 , 180, 87 , 0  , 0  , 0  ), // #472 {W:mm, R:mm|m64|mem, R:u8}
  ISIGNATURE(2, 1, 1, 0, 89 , 87 , 0  , 0  , 0  , 0  ), // #473 {X:xmm, R:u8}
  ISIGNATURE(2, 1, 1, 0, 49 , 128, 0  , 0  , 0  , 0  ), // #474 {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64|mem, R:cl|u8}
  ISIGNATURE(1, 0, 1, 0, 173, 0  , 0  , 0  , 0  , 0  ), // #475 {W:r32|r64}
  ISIGNATURE(3, 1, 1, 3, 52 , 234, 204, 0  , 0  , 0  ), // #476 {W:<edx>, W:<eax>, R:<ecx>}
  ISIGNATURE(1, 1, 1, 0, 177, 0  , 0  , 0  , 0  , 0  ), // #477 {W:r16|r32|r64}
  ISIGNATURE(2, 1, 1, 2, 52 , 234, 0  , 0  , 0  , 0  ), // #478 {W:<edx>, W:<eax>}
  ISIGNATURE(3, 1, 1, 3, 52 , 234, 231, 0  , 0  , 0  ), // #479 {W:<edx>, W:<eax>, W:<ecx>}
  ISIGNATURE(3, 1, 1, 0, 74 , 75 , 87 , 0  , 0  , 0  ), // #480 {W:xmm, R:xmm|m64|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 74 , 140, 87 , 0  , 0  , 0  ), // #481 {W:xmm, R:xmm|m32|mem, R:u8}
  ISIGNATURE(1, 1, 1, 1, 235, 0  , 0  , 0  , 0  , 0  ), // #482 {R:<ah>}
  ISIGNATURE(2, 1, 1, 2, 236, 199, 0  , 0  , 0  , 0  ), // #483 {R:<al|ax|eax|rax>, X:<es:[zdi]>}
  ISIGNATURE(1, 1, 1, 0, 1  , 0  , 0  , 0  , 0  , 0  ), // #484 {W:r8lo|r8hi|m8|mem}
  ISIGNATURE(1, 1, 1, 0, 169, 0  , 0  , 0  , 0  , 0  ), // #485 {W:r16|m16|mem|r32|r64}
  ISIGNATURE(1, 1, 1, 0, 67 , 0  , 0  , 0  , 0  , 0  ), // #486 {W:m32|mem}
  ISIGNATURE(2, 1, 1, 2, 199, 236, 0  , 0  , 0  , 0  ), // #487 {X:<es:[zdi]>, R:<al|ax|eax|rax>}
  ISIGNATURE(6, 1, 1, 0, 93 , 86 , 86 , 86 , 86 , 117), // #488 {X:zmm, R:zmm, R:zmm, R:zmm, R:zmm, R:m128|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 75 , 0  , 0  , 0  ), // #489 {W:xmm, R:xmm, R:xmm|m64|mem}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 140, 0  , 0  , 0  ), // #490 {W:xmm, R:xmm, R:xmm|m32|mem}
  ISIGNATURE(2, 1, 1, 0, 79 , 117, 0  , 0  , 0  , 0  ), // #491 {W:ymm, R:m128|mem}
  ISIGNATURE(2, 1, 1, 0, 237, 75 , 0  , 0  , 0  , 0  ), // #492 {W:ymm|zmm, R:xmm|m64|mem}
  ISIGNATURE(2, 1, 1, 0, 237, 117, 0  , 0  , 0  , 0  ), // #493 {W:ymm|zmm, R:m128|mem}
  ISIGNATURE(2, 1, 1, 0, 83 , 118, 0  , 0  , 0  , 0  ), // #494 {W:zmm, R:m256|mem}
  ISIGNATURE(2, 1, 1, 0, 185, 75 , 0  , 0  , 0  , 0  ), // #495 {W:xmm|ymm|zmm, R:xmm|m64|mem}
  ISIGNATURE(4, 1, 1, 0, 129, 73 , 75 , 87 , 0  , 0  ), // #496 {W:xmm|k, R:xmm, R:xmm|m64|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 129, 73 , 140, 87 , 0  , 0  ), // #497 {W:xmm|k, R:xmm, R:xmm|m32|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 74 , 73 , 208, 0  , 0  , 0  ), // #498 {W:xmm, R:xmm, R:r32|m32|mem|r64|m64}
  ISIGNATURE(3, 1, 1, 0, 78 , 238, 87 , 0  , 0  , 0  ), // #499 {W:xmm|m128|mem, R:ymm|zmm, R:u8}
  ISIGNATURE(4, 1, 1, 0, 89 , 73 , 75 , 87 , 0  , 0  ), // #500 {X:xmm, R:xmm, R:xmm|m64|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 89 , 73 , 140, 87 , 0  , 0  ), // #501 {X:xmm, R:xmm, R:xmm|m32|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 89 , 73 , 75 , 0  , 0  , 0  ), // #502 {X:xmm, R:xmm, R:xmm|m64|mem}
  ISIGNATURE(3, 1, 1, 0, 89 , 73 , 140, 0  , 0  , 0  ), // #503 {X:xmm, R:xmm, R:xmm|m32|mem}
  ISIGNATURE(3, 1, 1, 0, 131, 239, 87 , 0  , 0  , 0  ), // #504 {W:k, R:xmm|m128|ymm|m256|zmm|m512, R:u8}
  ISIGNATURE(3, 1, 1, 0, 131, 75 , 87 , 0  , 0  , 0  ), // #505 {W:k, R:xmm|m64|mem, R:u8}
  ISIGNATURE(3, 1, 1, 0, 131, 140, 87 , 0  , 0  , 0  ), // #506 {W:k, R:xmm|m32|mem, R:u8}
  ISIGNATURE(1, 1, 1, 0, 92 , 0  , 0  , 0  , 0  , 0  ), // #507 {R:vm32y}
  ISIGNATURE(1, 1, 1, 0, 94 , 0  , 0  , 0  , 0  , 0  ), // #508 {R:vm32z}
  ISIGNATURE(1, 1, 1, 0, 97 , 0  , 0  , 0  , 0  , 0  ), // #509 {R:vm64z}
  ISIGNATURE(4, 1, 1, 0, 83 , 86 , 80 , 87 , 0  , 0  ), // #510 {W:zmm, R:zmm, R:ymm|m256|mem, R:u8}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 140, 87 , 0  , 0  ), // #511 {W:xmm, R:xmm, R:xmm|m32|mem, R:u8}
  ISIGNATURE(3, 1, 1, 1, 73 , 73 , 228, 0  , 0  , 0  ), // #512 {R:xmm, R:xmm, R:<ds:[zdi]>}
  ISIGNATURE(2, 1, 1, 0, 173, 191, 0  , 0  , 0  , 0  ), // #513 {W:r32|r64, R:xmm|ymm}
  ISIGNATURE(6, 1, 1, 0, 83 , 86 , 86 , 86 , 86 , 117), // #514 {W:zmm, R:zmm, R:zmm, R:zmm, R:zmm, R:m128|mem}
  ISIGNATURE(2, 1, 1, 0, 185, 164, 0  , 0  , 0  , 0  ), // #515 {W:xmm|ymm|zmm, R:k}
  ISIGNATURE(2, 1, 1, 0, 185, 137, 0  , 0  , 0  , 0  ), // #516 {W:xmm|ymm|zmm, R:xmm|m64|mem|r64}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 226, 87 , 0  , 0  ), // #517 {W:xmm, R:xmm, R:r32|m16|mem|r16|r64, R:u8}
  ISIGNATURE(2, 1, 1, 0, 131, 240, 0  , 0  , 0  , 0  ), // #518 {W:k, R:xmm|ymm|zmm}
  ISIGNATURE(4, 1, 1, 0, 74 , 73 , 75 , 87 , 0  , 0  ), // #519 {W:xmm, R:xmm, R:xmm|m64|mem, R:u8}
  ISIGNATURE(1, 0, 1, 0, 187, 0  , 0  , 0  , 0  , 0  ), // #520 {R:r32|r64}
  ISIGNATURE(3, 1, 1, 3, 178, 103, 204, 0  , 0  , 0  ), // #521 {R:<edx>, R:<eax>, R:<ecx>}
  ISIGNATURE(1, 1, 1, 0, 241, 0  , 0  , 0  , 0  , 0  ), // #522 {R:rel16|rel32}
  ISIGNATURE(3, 1, 1, 2, 194, 178, 103, 0  , 0  , 0  ), // #523 {R:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 0, 1, 2, 194, 178, 103, 0  , 0  , 0  ), // #524 {R:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 1, 1, 2, 215, 178, 103, 0  , 0  , 0  ), // #525 {W:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 0, 1, 2, 215, 178, 103, 0  , 0  , 0  )  // #526 {W:mem, R:<edx>, R:<eax>}
};
#undef ISIGNATURE
// ----------------------------------------------------------------------------
// ${signatureData:End}
#endif // !ASMJIT_DISABLE_VALIDATION

// ============================================================================
// [asmjit::X86Inst - MiscData]
// ============================================================================

#define CC_TO_INST(inst) {               \
  inst##o, inst##no, inst##b , inst##ae, \
  inst##e, inst##ne, inst##be, inst##a , \
  inst##s, inst##ns, inst##pe, inst##po, \
  inst##l, inst##ge, inst##le, inst##g   \
}

const X86Inst::MiscData X86InstDB::miscData = {
  CC_TO_INST(X86Inst::kIdJ),
  CC_TO_INST(X86Inst::kIdSet),
  CC_TO_INST(X86Inst::kIdCmov),

  // ReversedCond[]:
  {
    x86::kCondO, x86::kCondNO, x86::kCondA , x86::kCondBE, // O|NO|B |AE
    x86::kCondE, x86::kCondNE, x86::kCondAE, x86::kCondB , // E|NE|BE|A
    x86::kCondS, x86::kCondNS, x86::kCondPE, x86::kCondPO, // S|NS|PE|PO
    x86::kCondG, x86::kCondLE, x86::kCondGE, x86::kCondL   // L|GE|LE|G
  }
};

#undef CC_TO_INST

// ============================================================================
// [asmjit::X86Inst - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(x86_inst_bits) {
  INFO("Checking validity of X86Inst enums");

  // Cross-validate prefixes.
  EXPECT(X86Inst::kOptionRex  == 0x80000000U, "REX prefix must be at 0x80000000");
  EXPECT(X86Inst::kOptionVex3 == 0x00000400U, "VEX3 prefix must be at 0x00000400");
  EXPECT(X86Inst::kOptionEvex == 0x00001000U, "EVEX prefix must be at 0x00001000");

  // These could be combined together to form a valid REX prefix, they must match.
  EXPECT(int(X86Inst::kOptionOpCodeB) == int(X86Inst::kOpCode_B));
  EXPECT(int(X86Inst::kOptionOpCodeX) == int(X86Inst::kOpCode_X));
  EXPECT(int(X86Inst::kOptionOpCodeR) == int(X86Inst::kOpCode_R));
  EXPECT(int(X86Inst::kOptionOpCodeW) == int(X86Inst::kOpCode_W));

  uint32_t rex_rb = (X86Inst::kOpCode_R >> X86Inst::kOpCode_REX_Shift) |
                    (X86Inst::kOpCode_B >> X86Inst::kOpCode_REX_Shift) | 0x40;
  uint32_t rex_rw = (X86Inst::kOpCode_R >> X86Inst::kOpCode_REX_Shift) |
                    (X86Inst::kOpCode_W >> X86Inst::kOpCode_REX_Shift) | 0x40;
  EXPECT(rex_rb == 0x45, "kOpCode_R|B must form a valid REX prefix 0x45 if combined with 0x40");
  EXPECT(rex_rw == 0x4C, "kOpCode_R|W must form a valid REX prefix 0x4C if combined with 0x40");
}
#endif // ASMJIT_TEST

#if defined(ASMJIT_TEST) && !defined(ASMJIT_DISABLE_TEXT)
UNIT(x86_inst_names) {
  // All known instructions should be matched.
  INFO("Matching all X86/X64 instructions");
  for (uint32_t a = 0; a < X86Inst::_kIdCount; a++) {
    uint32_t b = X86Inst::getIdByName(X86Inst::getInst(a).getName());
    EXPECT(a == b,
      "Should match existing instruction \"%s\" {id:%u} != \"%s\" {id:%u}",
        X86Inst::getInst(a).getName(), a,
        X86Inst::getInst(b).getName(), b);
  }

  // Everything else should return `Inst::kIdNone`.
  INFO("Trying to look-up instructions that don't exist");
  EXPECT(X86Inst::getIdByName(nullptr)  == Inst::kIdNone, "Should return Inst::kIdNone for null input");
  EXPECT(X86Inst::getIdByName("")       == Inst::kIdNone, "Should return Inst::kIdNone for empty string");
  EXPECT(X86Inst::getIdByName("_")      == Inst::kIdNone, "Should return Inst::kIdNone for unknown instruction");
  EXPECT(X86Inst::getIdByName("123xyz") == Inst::kIdNone, "Should return Inst::kIdNone for unknown instruction");
}
#endif // ASMJIT_TEST && !ASMJIT_DISABLE_TEXT

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
