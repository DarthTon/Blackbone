#pragma once
#include "../Config.h"
#include <stdint.h>

// Architecture-dependent pointer size
#define BlackBoneWordSize sizeof(void*)

// Rebase address
#define MAKE_PTR(T, pRVA, base)           (T)((ptr_t)pRVA + (ptr_t)base)
#define REBASE(pRVA, baseOld, baseNew)       ((ptr_t)pRVA - (ptr_t)baseOld + (ptr_t)baseNew)

// Field offset info
#define FIELD_OFFSET2(type, field)  ((LONG)(LONG_PTR)&(((type)0)->field))
#define GET_FIELD_PTR(entry, field) (uintptr_t)((uint8_t*)entry + FIELD_OFFSET2(decltype(entry), field))

#define CALL_64_86(b, f, ...) (b ? f<uint64_t>(__VA_ARGS__) : f<uint32_t>(__VA_ARGS__))
#define FIELD_PTR_64_86(b, e, t, f) (b ? fieldPtr( e, &t<uint64_t>::f ) : fieldPtr( e, &t<uint32_t>::f ))

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

// offsetof alternative
template<typename T, typename U>
constexpr size_t offsetOf( U T::*member )
{
    return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(nullptr)->*member));
}

template<typename T, typename U>
constexpr uint64_t fieldPtr( uint64_t base, U T::*member )
{
    return base + offsetOf( member );
}

// CONTAINING_RECORD alternative
template<typename T, typename U>
constexpr uint64_t structBase( uint64_t ptr, U T::*member )
{
    return ptr - offsetOf( member );
}

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
#define LAST_STATUS_OFS (0x598 + 0x197 * BlackBoneWordSize)

using NTSTATUS = long;

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
inline NTSTATUS SetLastNtStatus( NTSTATUS status )
{
    return *(NTSTATUS*)((unsigned char*)NtCurrentTeb() + LAST_STATUS_OFS) = status;
}

#define SharedUserData32 ((KUSER_SHARED_DATA* const)0x7FFE0000)
