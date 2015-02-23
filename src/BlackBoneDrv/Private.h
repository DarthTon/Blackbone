#pragma once

#include "Imports.h"

//#ifdef DBG
#define DPRINT(format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, __VA_ARGS__)
//#else
//#define DPRINT(...)
//#endif

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

#define MM_PTE_VALID_MASK         0x1
#define MM_PTE_WRITE_MASK         0x800
#define MM_PTE_OWNER_MASK         0x4
#define MM_PTE_WRITE_THROUGH_MASK 0x8
#define MM_PTE_CACHE_DISABLE_MASK 0x10
#define MM_PTE_ACCESS_MASK        0x20
#define MM_PTE_DIRTY_MASK         0x42
#define MM_PTE_LARGE_PAGE_MASK    0x80
#define MM_PTE_GLOBAL_MASK        0x100
#define MM_PTE_COPY_ON_WRITE_MASK 0x200
#define MM_PTE_PROTOTYPE_MASK     0x400
#define MM_PTE_TRANSITION_MASK    0x800

#define VIRTUAL_ADDRESS_BITS 48
#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED        0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH      0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER      0x00000004

#define EX_ADDITIONAL_INFO_SIGNATURE (ULONG_PTR)(-2)

#define KI_USER_SHARED_DATA 0xFFFFF78000000000UI64

#define SharedUserData ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)

#define PTE_SHIFT 3
#define ObpDecodeGrantedAccess( Access ) \
    ((Access)& ~ObpAccessProtectCloseBit)

#define ObpDecodeObject( Object ) (PVOID)(((LONG_PTR)Object >> 0x10) & ~(ULONG_PTR)0xF)

#define MiGetPxeOffset(va) \
    ((ULONG)(((ULONG_PTR)(va) >> PXI_SHIFT) & PXI_MASK))

#define MiGetPxeAddress(va)   \
    ((PMMPTE)PXE_BASE + MiGetPxeOffset(va))

#define MiGetPpeAddress(va)   \
    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PPI_SHIFT) << PTE_SHIFT) + PPE_BASE))

#define MiGetPdeAddress(va) \
    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PDI_SHIFT) << PTE_SHIFT) + PDE_BASE))

#define MiGetPteAddress(va) \
    ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + PTE_BASE))

#define VA_SHIFT (63 - 47)              // address sign extend shift count

#define MiGetVirtualAddressMappedByPte(PTE) \
    ((PVOID)((LONG_PTR)(((LONG_PTR)(PTE) - PTE_BASE) << (PAGE_SHIFT + VA_SHIFT - PTE_SHIFT)) >> VA_SHIFT))

#define MI_IS_PHYSICAL_ADDRESS(Va) \
    ((MiGetPxeAddress(Va)->u.Hard.Valid == 1) && \
     (MiGetPpeAddress(Va)->u.Hard.Valid == 1) && \
     ((MiGetPdeAddress(Va)->u.Long & 0x81) == 0x81) || (MiGetPteAddress(Va)->u.Hard.Valid == 1))

#define ExpIsValidObjectEntry(Entry) \
    ( (Entry != NULL) && (Entry->LowValue != 0) && (Entry->HighValue != EX_ADDITIONAL_INFO_SIGNATURE) )

typedef ULONG WIN32_PROTECTION_MASK;
typedef PULONG PWIN32_PROTECTION_MASK;

typedef enum _WinVer
{
    WINVER_7     = 0x0610,
    WINVER_7_SP1 = 0x0611,
    WINVER_8     = 0x0620,
    WINVER_81    = 0x0630,
    WINVER_10    = 0x0A00,
} WinVer;

extern PLIST_ENTRY PsLoadedModuleList;
extern MMPTE ValidKernelPte;

/// <summary>
/// OS-dependent stuff
/// </summary>
typedef struct _DYNAMIC_DATA
{
    WinVer  ver;            // OS version
    BOOLEAN correctBuild;   // OS kernel build number is correct and supported

    ULONG KExecOpt;         // KPROCESS::ExecuteOptions 
    ULONG Protection;       // EPROCESS::Protection
    ULONG ObjTable;         // EPROCESS::ObjectTable
    ULONG VadRoot;          // EPROCESS::VadRoot
    ULONG NtProtectIndex;   // NtProtectVirtualMemory SSDT index
    ULONG NtCreateThdIndex; // NtCreateThreadEx SSDT index
    ULONG NtTermThdIndex;   // NtTerminateThread SSDT index
    ULONG PrevMode;         // KTHREAD::PreviousMode
    ULONG ExitStatus;       // ETHREAD::ExitStatus
    ULONG MiAllocPage;      // MiAllocateDriverPage offset
    ULONG ExRemoveTable;    // ExRemoveHandleTable offset
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

typedef PFN_NUMBER( NTAPI* fnMiAllocateDriverPage )(IN PMMPTE pPTE);
typedef NTSTATUS( NTAPI* fnNtTerminateThread)( IN HANDLE ThreadHandle, IN NTSTATUS ExitStatus );
typedef NTSTATUS( NTAPI* fnExRemoveHandleTable )(IN PHANDLE_TABLE pTable);

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
PMMPTE GetPTEForVA( IN PVOID pAddress );
