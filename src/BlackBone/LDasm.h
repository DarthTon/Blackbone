#ifndef _LDASM_
#define _LDASM_

#include "string.h"

#ifdef _M_AMD64
    #define is_x64 1
#else
    #define is_x64 0
#endif//_M_AMD64

#ifdef __cplusplus
extern "C"
{
#endif

#define F_INVALID       0x01
#define F_PREFIX        0x02
#define F_REX           0x04
#define F_MODRM         0x08
#define F_SIB           0x10
#define F_DISP          0x20
#define F_IMM           0x40
#define F_RELATIVE      0x80

typedef unsigned char u8;
typedef unsigned long u32;

typedef struct _ldasm_data
{
    u8  flags;
    u8  rex;
    u8  modrm;
    u8  sib;
    u8  opcd_offset;
    u8  opcd_size;
    u8  disp_offset;
    u8  disp_size;
    u8  imm_offset;
    u8  imm_size;
} ldasm_data;

unsigned int  __fastcall ldasm( void *code, ldasm_data *ld, u32 is64 );
unsigned long __fastcall SizeOfProc( void *Proc );
void*         __fastcall ResolveJmp( void *Proc );

#ifdef __cplusplus
}
#endif

#endif//_LDASM_