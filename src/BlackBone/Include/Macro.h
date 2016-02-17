#pragma once
#include "../Config.h"

// Architecture-dependent pointer size
#define WordSize sizeof(void*)

// Rebase address
#define MAKE_PTR(T, pRVA, base)           (T)((ptr_t)pRVA + (ptr_t)base)
#define REBASE(pRVA, baseOld, baseNew)       ((ptr_t)pRVA - (ptr_t)baseOld + (ptr_t)baseNew)
#define REBASE2(T, rva, baseOld, baseNew) (T)((uintptr_t)rva - (uintptr_t)baseOld + (uintptr_t)baseNew)

// Field offset info
#define FIELD_OFFSET2(type, field)  ((LONG)(LONG_PTR)&(((type)0)->field))
#define GET_FIELD_PTR(entry, field) (uintptr_t)((uint8_t*)entry + FIELD_OFFSET2(decltype(entry), field))

#define LODWORD(l) ((uint32_t)(((uint64_t)(l)) & 0xffffffff))
#define HIDWORD(l) ((uint32_t)((((uint64_t)(l)) >> 32) & 0xffffffff))

// Set or reset particular bit
#define SET_BIT(v, b)   v |= (1ull << b)
#define RESET_BIT(v, b) v &= ~(1ull << b)

// Register aliases
#ifdef USE64
#define NAX Rax
#define NSP Rsp
#define NIP Rip
#define NDI Rdi

#define BitScanForwardT     _BitScanForward64
#define BitScanReverseT     _BitScanReverse64
#define BitTestAndSetT      _bittestandset64
#define BitTestAndResetT    _bittestandreset64

#define SET_JUMP(_src,_dst) *(uintptr_t*)(_src) = 0x25FF; *(uintptr_t*)((_src) + 6) = (uintptr_t)_dst;
#else
#define NAX Eax
#define NSP Esp
#define NIP Eip
#define NDI Edi

#define BitScanForwardT     _BitScanForward
#define BitScanReverseT     _BitScanReverse
#define BitTestAndSetT      _bittestandset
#define BitTestAndResetT    _bittestandreset

#define SET_JUMP(_src,_dst) *(uint8_t*)(_src) = 0xE9; *(uintptr_t*)((_src) + 1) = (uintptr_t)(_dst) - (uintptr_t)(_src) - 5
#endif

#define ENUM_OPS(e) \
    inline e operator |(e a1, e a2) { \
        return static_cast<e>(static_cast<int>(a1) | static_cast<int>(a2)); \
    } \
    \
    inline e operator |= (e& a1, e a2) { \
        return a1 = a1 | a2; \
    } \
    \
    inline e operator &(e a1, e a2) { \
        return static_cast<e>(static_cast<int>(a1)& static_cast<int>(a2)); \
    } \
    \
    inline e operator &= (e& a1, e a2) { \
        return a1 = a1 & a2; \
    } \
    \
    inline e operator ~(e a1) { \
        return static_cast<e>(~static_cast<int>(a1)); \
    }


template<int s> 
struct CompileTimeSizeOf;

// Type-unsafe cast.
template<typename _Tgt, typename _Src>
inline _Tgt brutal_cast( const _Src& src )
{
    static_assert(sizeof( _Tgt ) == sizeof( _Src ), "Operand size mismatch");
    union _u { _Src s; _Tgt t; } u;
    u.s = src;
    return u.t;
}

// Align value
inline size_t Align( size_t val, size_t alignment )
{
    return (val % alignment == 0) ? val : (val / alignment + 1) * alignment;
}

// Offset of 'LastStatus' field in TEB
#define LAST_STATUS_OFS (0x598 + 0x197 * WordSize)

typedef long NTSTATUS;

/// <summary>
/// Get last NT status
/// </summary>
/// <returns></returns>
inline NTSTATUS LastNtStatus()
{
    return *(NTSTATUS*)((unsigned char*)NtCurrentTeb() + LAST_STATUS_OFS);
}

/// <summary>
/// Set last NT status
/// </summary>
/// <param name="status">The status.</param>
/// <returns></returns>
inline NTSTATUS LastNtStatus( NTSTATUS status )
{
    return *(NTSTATUS*)((unsigned char*)NtCurrentTeb() + LAST_STATUS_OFS) = status;
}

#define EMIT(a) __asm __emit (a)

// Switch processor to long mode
#define X64_Start_with_CS(_cs) \
	{ \
	EMIT(0x6A) EMIT(_cs)                         /*  push   _cs             */ \
	EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)   /*  call   $+5             */ \
	EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(5)        /*  add    dword [esp], 5  */ \
	EMIT(0xCB)                                   /*  retf                   */ \
	}

// Switch processor to WOW64 mode
#define X64_End_with_CS(_cs) \
	{ \
	EMIT(0xE8) EMIT(0) EMIT(0) EMIT(0) EMIT(0)                                 /*  call   $+5                   */ \
	EMIT(0xC7) EMIT(0x44) EMIT(0x24) EMIT(4) EMIT(_cs) EMIT(0) EMIT(0) EMIT(0) /*  mov    dword [rsp + 4], _cs  */ \
	EMIT(0x83) EMIT(4) EMIT(0x24) EMIT(0xD)                                    /*  add    dword [rsp], 0xD      */ \
	EMIT(0xCB)                                                                 /*  retf                         */ \
	}

//
// 64bit assembly helpers
//
#define X64_Start() X64_Start_with_CS(0x33)
#define X64_End() X64_End_with_CS(0x23)

#define _RAX  0
#define _RCX  1
#define _RDX  2
#define _RBX  3
#define _RSP  4
#define _RBP  5
#define _RSI  6
#define _RDI  7
#define _R8   8
#define _R9   9
#define _R10 10
#define _R11 11
#define _R12 12
#define _R13 13
#define _R14 14
#define _R15 15

#define X64_Push(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x50 | ((r) & 7))
#define X64_Pop(r) EMIT(0x48 | ((r) >> 3)) EMIT(0x58 | ((r) & 7))
