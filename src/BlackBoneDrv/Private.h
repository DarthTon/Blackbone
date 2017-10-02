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

#ifndef PTE_SHIFT
#define PTE_SHIFT 3
#endif
#ifndef PTI_SHIFT
#define PTI_SHIFT 12
#endif
#ifndef PDI_SHIFT
#define PDI_SHIFT 21
#endif
#ifndef PPI_SHIFT
#define PPI_SHIFT 30
#endif
#ifndef PXI_SHIFT
#define PXI_SHIFT 39
#endif

#ifndef PXE_BASE
#define PXE_BASE    0xFFFFF6FB7DBED000UI64
#endif
#ifndef PXE_SELFMAP
#define PXE_SELFMAP 0xFFFFF6FB7DBEDF68UI64
#endif
#ifndef PPE_BASE
#define PPE_BASE    0xFFFFF6FB7DA00000UI64
#endif
#ifndef PDE_BASE
#define PDE_BASE    0xFFFFF6FB40000000UI64
#endif
#ifndef PTE_BASE
#define PTE_BASE    0xFFFFF68000000000UI64
#endif

#define ObpDecodeGrantedAccess( Access ) \
    ((Access)& ~ObpAccessProtectCloseBit)

#define ObpDecodeObject( Object ) (PVOID)(((LONG_PTR)Object >> 0x10) & ~(ULONG_PTR)0xF)

#define PTE_PER_PAGE 512
#define PDE_PER_PAGE 512
#define PPE_PER_PAGE 512
#define PXE_PER_PAGE 512

#define PPI_MASK (PPE_PER_PAGE - 1)
#define PXI_MASK (PXE_PER_PAGE - 1)

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

// Workaround for compiler warning
#define FN_CAST(T, p)   (T)(ULONG_PTR)p
#define FN_CAST_V(p)    (PVOID)(ULONG_PTR)p

// Get SSDT index from function pointer
#define SSDTIndex(pfn)  *(PULONG)((ULONG_PTR)pfn + 0x15)

typedef ULONG WIN32_PROTECTION_MASK;
typedef PULONG PWIN32_PROTECTION_MASK;

typedef enum _WinVer
{
    WINVER_7     = 0x0610,
    WINVER_7_SP1 = 0x0611,
    WINVER_8     = 0x0620,
    WINVER_81    = 0x0630,
    WINVER_10    = 0x0A00,
    WINVER_10_AU = 0x0A01,
    WINVER_10_CU = 0x0A02,
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
    ULONG EProcessFlags2;   // EPROCESS::Flags2
    ULONG ObjTable;         // EPROCESS::ObjectTable
    ULONG VadRoot;          // EPROCESS::VadRoot
    ULONG NtProtectIndex;   // NtProtectVirtualMemory SSDT index
    ULONG NtCreateThdIndex; // NtCreateThreadEx SSDT index
    ULONG NtTermThdIndex;   // NtTerminateThread SSDT index
    ULONG PrevMode;         // KTHREAD::PreviousMode
    ULONG ExitStatus;       // ETHREAD::ExitStatus
    ULONG MiAllocPage;      // MiAllocateDriverPage offset
    ULONG ExRemoveTable;    // Ex(p)RemoveHandleTable offset

    ULONG_PTR DYN_PDE_BASE; // Win10 AU+ relocated PDE base VA
    ULONG_PTR DYN_PTE_BASE; // Win10 AU+ relocated PTE base VA
} DYNAMIC_DATA, *PDYNAMIC_DATA;


typedef struct _NOPPROCINFO
{
    KDPC DpcTraps[MAXIMUM_PROCESSORS];
    LONG ActiveCores;
    LONG DPCCount;
    ULONG Cores;
    KIRQL SavedIrql;
    KPRIORITY SavedPriority;
    LONG IsCodeExecuted;
}NOPPROCINFO, *PNOPPROCINFO;

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
    IN OUT PVOID* BaseAddress,
    IN OUT SIZE_T* NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection 
    );


#else
NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory( 
    IN HANDLE ProcessHandle,
    IN OUT PVOID* BaseAddress,
    IN OUT SIZE_T* NumberOfBytesToProtect,
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
/// <param name="pSize">Size of module</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetKernelBase( OUT PULONG pSize );

/// <summary>
/// Gets SSDT base - KiSystemServiceTable
/// </summary>
/// <returns>SSDT base, NULL if not found</returns>
PSYSTEM_SERVICE_DESCRIPTOR_TABLE GetSSDTBase();

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

/// <summary>
/// Initialize structure for processor start/stop
/// </summary>
/// <param name="Info">>Processors data</param>
VOID InitializeStopProcessors( OUT NOPPROCINFO* Info );

/// <summary>
/// Stall all but current active processors 
/// </summary>
/// <param name="Info">Processors data</param>
VOID StopProcessors( IN NOPPROCINFO* Info );

/// <summary>
/// Resume all stopped active processors 
/// </summary>
/// <param name="Info">Processors data</param>
VOID StartProcessors( IN NOPPROCINFO* Info );

/// <summary>
/// Allocate memory in one of the ntoskrnl discarded section
/// </summary>
/// <param name="SizeOfImage">Block size to allocate</param>
/// <param name="ppFoundBase">Allocated address</param>
/// <returns>Status code</returns>
NTSTATUS AllocateInDiscardedMemory( IN ULONG size, OUT PVOID* ppFoundBase );
