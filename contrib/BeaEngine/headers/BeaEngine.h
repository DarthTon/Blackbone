/*  Header for BeaEngine 4.x    */
#ifndef _BEA_ENGINE_
#define _BEA_ENGINE_

#include "Includes/export.h"
#include "Includes/basic_types.h"

#if !defined(BEA_ENGINE_STATIC)
	#if defined(BUILD_BEA_ENGINE_DLL)
		#define BEA_API bea__api_export__
	#else
		#define BEA_API bea__api_import__
	#endif
#else
	#define BEA_API
#endif


#define INSTRUCT_LENGTH 64

#pragma pack(1)
typedef struct {
   UInt8 W_;
   UInt8 R_;
   UInt8 X_;
   UInt8 B_;
   UInt8 state;
} REX_Struct  ;
#pragma pack()

#pragma pack(1)
typedef struct {
   int Number;
   int NbUndefined;
   UInt8 LockPrefix;
   UInt8 OperandSize;
   UInt8 AddressSize;
   UInt8 RepnePrefix;
   UInt8 RepPrefix;
   UInt8 FSPrefix;
   UInt8 SSPrefix;
   UInt8 GSPrefix;
   UInt8 ESPrefix;
   UInt8 CSPrefix;
   UInt8 DSPrefix;
   UInt8 BranchTaken;
   UInt8 BranchNotTaken;
   REX_Struct REX;
   char alignment[2];
} PREFIXINFO  ;
#pragma pack()

#pragma pack(1)
typedef struct {
   UInt8 OF_;
   UInt8 SF_;
   UInt8 ZF_;
   UInt8 AF_;
   UInt8 PF_;
   UInt8 CF_;
   UInt8 TF_;
   UInt8 IF_;
   UInt8 DF_;
   UInt8 NT_;
   UInt8 RF_;
   UInt8 alignment;
} EFLStruct  ;
#pragma pack()

#pragma pack(4)
typedef struct {
   Int32 BaseRegister;
   Int32 IndexRegister;
   Int32 Scale;
   Int64 Displacement;
} MEMORYTYPE ;
#pragma pack()


#pragma pack(1)
typedef struct  {
   Int32 Category;
   Int32 Opcode;
   char Mnemonic[16];
   Int32 BranchType;
   EFLStruct Flags;
   UInt64 AddrValue;
   Int64 Immediat;
   UInt32 ImplicitModifiedRegs;
} INSTRTYPE;
#pragma pack()

#pragma pack(1)
typedef struct  {
   char ArgMnemonic[64];
   Int32 ArgType;
   Int32 ArgSize;
   Int32 ArgPosition;
   UInt32 AccessMode;
   MEMORYTYPE Memory;
   UInt32 SegmentReg;
} ARGTYPE;
#pragma pack()



#pragma pack(1)
typedef struct _Disasm {
   UIntPtr EIP;
   UInt64 VirtualAddr;
   UInt32 SecurityBlock;
   char CompleteInstr[INSTRUCT_LENGTH];
   UInt32 Archi;
   UInt64 Options;
   INSTRTYPE Instruction;
   ARGTYPE Argument1;
   ARGTYPE Argument2;
   ARGTYPE Argument3;
   PREFIXINFO Prefix;
   UInt32 Reserved_[40];
} DISASM, *PDISASM, *LPDISASM;
#pragma pack()

#define ESReg 1
#define DSReg 2
#define FSReg 3
#define GSReg 4
#define CSReg 5
#define SSReg 6

#define InvalidPrefix 4
#define SuperfluousPrefix 2
#define NotUsedPrefix 0
#define MandatoryPrefix 8
#define InUsePrefix 1

#define LowPosition 0
#define HighPosition 1

enum INSTRUCTION_TYPE
{
  GENERAL_PURPOSE_INSTRUCTION   =    0x10000,
  FPU_INSTRUCTION               =    0x20000,
  MMX_INSTRUCTION               =    0x40000,
  SSE_INSTRUCTION               =    0x80000,
  SSE2_INSTRUCTION              =   0x100000,
  SSE3_INSTRUCTION              =   0x200000,
  SSSE3_INSTRUCTION             =   0x400000,
  SSE41_INSTRUCTION             =   0x800000,
  SSE42_INSTRUCTION             =  0x1000000,
  SYSTEM_INSTRUCTION            =  0x2000000,
  VM_INSTRUCTION                =  0x4000000,
  UNDOCUMENTED_INSTRUCTION      =  0x8000000,
  AMD_INSTRUCTION               = 0x10000000,
  ILLEGAL_INSTRUCTION           = 0x20000000,
  AES_INSTRUCTION               = 0x40000000,
  CLMUL_INSTRUCTION             = (int)0x80000000,


    DATA_TRANSFER = 0x1,
    ARITHMETIC_INSTRUCTION,
    LOGICAL_INSTRUCTION,
    SHIFT_ROTATE,
    BIT_UInt8,
    CONTROL_TRANSFER,
    STRING_INSTRUCTION,
    InOutINSTRUCTION,
    ENTER_LEAVE_INSTRUCTION,
    FLAG_CONTROL_INSTRUCTION,
    SEGMENT_REGISTER,
    MISCELLANEOUS_INSTRUCTION,
    COMPARISON_INSTRUCTION,
    LOGARITHMIC_INSTRUCTION,
    TRIGONOMETRIC_INSTRUCTION,
    UNSUPPORTED_INSTRUCTION,
    LOAD_CONSTANTS,
    FPUCONTROL,
    STATE_MANAGEMENT,
    CONVERSION_INSTRUCTION,
    SHUFFLE_UNPACK,
    PACKED_SINGLE_PRECISION,
    SIMD128bits,
    SIMD64bits,
    CACHEABILITY_CONTROL,
    FP_INTEGER_CONVERSION,
    SPECIALIZED_128bits,
    SIMD_FP_PACKED,
    SIMD_FP_HORIZONTAL ,
    AGENT_SYNCHRONISATION,
    PACKED_ALIGN_RIGHT  ,
    PACKED_SIGN,
    PACKED_BLENDING_INSTRUCTION,
    PACKED_TEST,
    PACKED_MINMAX,
    HORIZONTAL_SEARCH,
    PACKED_EQUALITY,
    STREAMING_LOAD,
    INSERTION_EXTRACTION,
    DOT_PRODUCT,
    SAD_INSTRUCTION,
    ACCELERATOR_INSTRUCTION,    /* crc32, popcnt (sse4.2) */
    ROUND_INSTRUCTION

};

enum EFLAGS_STATES
{
  TE_ = 1,
  MO_ = 2,
  RE_ = 4,
  SE_ = 8,
  UN_ = 0x10,
  PR_ = 0x20
};

enum BRANCH_TYPE
{
  JO = 1,
  JC = 2,
  JE = 3,
  JA = 4,
  JS = 5,
  JP = 6,
  JL = 7,
  JG = 8,
  JB = 2,       /* JC == JB */
  JECXZ = 10,
  JmpType = 11,
  CallType = 12,
  RetType = 13,
  JNO = -1,
  JNC = -2,
  JNE = -3,
  JNA = -4,
  JNS = -5,
  JNP = -6,
  JNL = -7,
  JNG = -8,
  JNB = -2      /* JNC == JNB */
};

enum ARGUMENTS_TYPE
{
  NO_ARGUMENT = 0x10000000,
  REGISTER_TYPE = 0x20000000,
  MEMORY_TYPE = 0x40000000,
  CONSTANT_TYPE = (int)0x80000000,

  MMX_REG = 0x10000,
  GENERAL_REG = 0x20000,
  FPU_REG = 0x40000,
  SSE_REG = 0x80000,
  CR_REG = 0x100000,
  DR_REG = 0x200000,
  SPECIAL_REG = 0x400000,
  MEMORY_MANAGEMENT_REG = 0x800000,
  SEGMENT_REG = 0x1000000,

  RELATIVE_ = 0x4000000,
  ABSOLUTE_ = 0x8000000,

  READ = 0x1,
  WRITE = 0x2,

  REG0 = 0x1,
  REG1 = 0x2,
  REG2 = 0x4,
  REG3 = 0x8,
  REG4 = 0x10,
  REG5 = 0x20,
  REG6 = 0x40,
  REG7 = 0x80,
  REG8 = 0x100,
  REG9 = 0x200,
  REG10 = 0x400,
  REG11 = 0x800,
  REG12 = 0x1000,
  REG13 = 0x2000,
  REG14 = 0x4000,
  REG15 = 0x8000
};

enum SPECIAL_INFO
{
  UNKNOWN_OPCODE = -1,
  OUT_OF_BLOCK = 0,

  /* === mask = 0xff */
  NoTabulation      = 0x00000000,
  Tabulation        = 0x00000001,

  /* === mask = 0xff00 */
  MasmSyntax        = 0x00000000,
  GoAsmSyntax       = 0x00000100,
  NasmSyntax        = 0x00000200,
  ATSyntax          = 0x00000400,

  /* === mask = 0xff0000 */
  PrefixedNumeral   = 0x00010000,
  SuffixedNumeral   = 0x00000000,

  /* === mask = 0xff000000 */
  ShowSegmentRegs   = 0x01000000
};


#ifdef __cplusplus
extern "C"
#endif

BEA_API int __bea_callspec__ Disasm (LPDISASM pDisAsm);
BEA_API const__ char* __bea_callspec__ BeaEngineVersion (void);
BEA_API const__ char* __bea_callspec__ BeaEngineRevision (void);
#endif
