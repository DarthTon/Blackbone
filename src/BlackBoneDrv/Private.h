#pragma once

#include "Imports.h"
#include "VadHelpers.h"

#ifdef DBG
#define DPRINT(format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, __VA_ARGS__)
#else
#define DPRINT(...)
#endif

#define BB_POOL_TAG 'enoB'


#define ObpAccessProtectCloseBit 0x2000000

//
// PTE protection values
//
#define MM_ZERO_ACCESS         0
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7

#define VIRTUAL_ADDRESS_BITS 48
#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED        0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH      0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER      0x00000004

#define PTE_SHIFT 3
#define ObpDecodeGrantedAccess( Access ) \
    ((Access)& ~ObpAccessProtectCloseBit)

#define ObpDecodeObject( Object ) (PVOID)(((LONG_PTR)Object >> 0x10) & ~(ULONG_PTR)0xF)

#define MiGetPxeOffset(va) \
    ((ULONG)(((ULONG_PTR)(va) >> PXI_SHIFT) & PXI_MASK))

#define MiGetHardwarePxeAddress(va)   \
    ((PMMPTE_HARDWARE64)PXE_BASE + MiGetPxeOffset(va))

#define MiGetHardwarePpeAddress(va)   \
    ((PMMPTE_HARDWARE64)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PPI_SHIFT) << PTE_SHIFT) + PPE_BASE))

#define MiGetHardwarePdeAddress(va) \
    ((PMMPTE_HARDWARE64)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PDI_SHIFT) << PTE_SHIFT) + PDE_BASE))

#define MiGetHardwarePteAddress(va) \
    ((PMMPTE_HARDWARE64)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + PTE_BASE))

typedef ULONG WIN32_PROTECTION_MASK;
typedef PULONG PWIN32_PROTECTION_MASK;

typedef enum _WinVer
{
    WINVER_7     = 0x610,
    WINVER_7_SP1 = 0x611,
    WINVER_8     = 0x620,
    WINVER_81    = 0x630,
} WinVer;

/// <summary>
/// OS-dependent stuff
/// </summary>
typedef struct _DYNAMIC_DATA
{
    WinVer ver;             // OS version

    ULONG KExecOpt;         // KEXECUTE_OPTIONS offset in KPROCESS
    ULONG Protection;       // Process protection flag offset in EPROCESS
    ULONG ObjTable;         // Process handle table offset in EPROCESS
    ULONG VadRoot;          // VadRoot offset in EPROCESS
    ULONG NtProtectIndex;   // NtProtectVirtualMemory SSDT index
    ULONG NtThdIndex;       // NtCreateThreadEx SSDT index
    ULONG PrevMode;         // PreviousMode offset in KTHREAD
    ULONG ApcState;         // KTHREAD ApcState field
    ULONG ExitStatus;       // ETHREAD ExitStatus field
} DYNAMIC_DATA, *PDYNAMIC_DATA;


typedef NTSTATUS( NTAPI* fnNtCreateThreadEx )
    (
        OUT PHANDLE hThread,
        IN ACCESS_MASK DesiredAccess,
        IN PVOID ObjectAttributes,
        IN HANDLE ProcessHandle,
        IN PVOID lpStartAddress,
        IN PVOID lpParameter,
        IN ULONG Flags,
        IN SIZE_T StackZeroBits,
        IN SIZE_T SizeOfStackCommit,
        IN SIZE_T SizeOfStackReserve,
        OUT PVOID lpBytesBuffer
    );


NTSTATUS 
NTAPI 
ZwCreateThreadEx(
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN PVOID ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN PVOID lpStartAddress,
    IN PVOID lpParameter,
    IN ULONG Flags,
    IN SIZE_T StackZeroBits,
    IN SIZE_T SizeOfStackCommit,
    IN SIZE_T SizeOfStackReserve,
    IN PNT_PROC_THREAD_ATTRIBUTE_LIST AttributeList
    );

#if defined(_WIN8_) || defined (_WIN7_)

typedef NTSTATUS( NTAPI* fnNtProtectVirtualMemory )
    ( 
        IN HANDLE ProcessHandle,
        IN PVOID* BaseAddress,
        IN SIZE_T* NumberOfBytesToProtect,
        IN ULONG NewAccessProtection,
        OUT PULONG OldAccessProtection
    );

NTSTATUS
NTAPI
ZwProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID* BaseAddress,
    IN SIZE_T* NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection 
    );


#else
NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory( 
    IN HANDLE ProcessHandle,
    IN PVOID* BaseAddress,
    IN SIZE_T* NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
    );

#endif

#ifdef _WIN81_

NTSYSAPI 
PVOID 
NTAPI 
RtlAvlRemoveNode( 
    IN PRTL_AVL_TREE pTree, 
    IN PMMADDRESS_NODE pNode 
    );

#endif

/// <summary>
/// Lookup handle in the process handle table
/// </summary>
/// <param name="HandleTable">Handle table</param>
/// <param name="tHandle">Handle to search for</param>
/// <returns>Found entry, NULL if nothing found</returns>
PHANDLE_TABLE_ENTRY ExpLookupHandleTableEntry( IN PHANDLE_TABLE HandleTable, IN EXHANDLE tHandle );

/// <summary>
/// Get ntoskrnl base address
/// </summary>
/// <returns>Found address, NULL if not found</returns>
PVOID GetKernelBase();

/// <summary>
/// Get module base address by name
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="ModuleName">Nodule name to search for</param>
/// <param name="isWow64">If TRUE - search in 32-bit PEB</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetModuleBase( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64 );

/// <summary>
/// Get exported function address
/// </summary>
/// <param name="pBase">Module base</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetModuleExport( IN PVOID pBase, IN PCCHAR name_ord );

/// <summary>
/// Gets SSDT base - KiSystemServiceTable
/// </summary>
/// <returns>SSDT base, NULL if not found</returns>
PVOID GetSSDTBase();

/// <summary>
/// Gets the SSDT entry address by index.
/// </summary>
/// <param name="index">Service index</param>
/// <returns>Found service address, NULL if not found</returns>
PVOID GetSSDTEntry( IN ULONG index );

/// <summary>
/// Get page hardware PTE
/// Address must be valid, otherwise bug check is imminent
/// </summary>
/// <param name="pAddress">Target address</param>
/// <returns>Found PTE</returns>
PMMPTE_HARDWARE64 GetPTEForVA( IN PVOID pAddress );
