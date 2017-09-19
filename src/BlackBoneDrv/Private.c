#include "Private.h"
#include "Utils.h"
#include <Ntstrsafe.h>

#pragma alloc_text(PAGE, ExpLookupHandleTableEntry)
#pragma alloc_text(PAGE, GetKernelBase)
#pragma alloc_text(PAGE, GetSSDTBase)
#pragma alloc_text(PAGE, GetSSDTEntry)
#pragma alloc_text(PAGE, GetPTEForVA)
#pragma alloc_text(PAGE, InitializeStopProcessors)
#pragma alloc_text(PAGE, StopProcessors)
#pragma alloc_text(PAGE, StartProcessors)
#pragma alloc_text(PAGE, AllocateInDiscardedMemory)

extern DYNAMIC_DATA dynData;

PVOID g_KernelBase = NULL;
ULONG g_KernelSize = 0;
PSYSTEM_SERVICE_DESCRIPTOR_TABLE g_SSDT = NULL;

MMPTE ValidKernelPte =
{
    MM_PTE_VALID_MASK  |
    MM_PTE_WRITE_MASK  |
    MM_PTE_GLOBAL_MASK |
    MM_PTE_DIRTY_MASK  |
    MM_PTE_ACCESS_MASK
};


/// <summary>
/// Lookup handle in the process handle table
/// </summary>
/// <param name="HandleTable">Handle table</param>
/// <param name="tHandle">Handle to search for</param>
/// <returns>Found entry, NULL if nothing found</returns>
PHANDLE_TABLE_ENTRY ExpLookupHandleTableEntry( IN PHANDLE_TABLE HandleTable, IN EXHANDLE tHandle )
{
    ULONG_PTR TableCode = HandleTable->TableCode & 3;
    if (tHandle.Value >= HandleTable->NextHandleNeedingPool)
        return NULL;

    tHandle.Value &= 0xFFFFFFFFFFFFFFFC;

#if defined ( _WIN10_ )
    if (TableCode != 0)
    {
        if (TableCode == 1)
        {
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(HandleTable->TableCode + 8 * (tHandle.Value >> 11) - 1) + 4 * (tHandle.Value & 0x7FC));
        }
        else
        {
            ULONG_PTR tmp = tHandle.Value >> 11;
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(*(ULONG_PTR*)(HandleTable->TableCode + 8 * (tHandle.Value >> 21) - 2) + 8 * (tmp & 0x3FF)) + 4 * (tHandle.Value & 0x7FC));
        }
    }
    else
    {
        return (PHANDLE_TABLE_ENTRY)(HandleTable->TableCode + 4 * tHandle.Value);
    }
#elif defined ( _WIN7_ )
    ULONG_PTR Diff = HandleTable->TableCode - TableCode;

    if (TableCode != 0)
    {
        if (TableCode == 1)
        {
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(Diff + ((tHandle.Value - tHandle.Value & 0x7FC) >> 9)) + 4 * (tHandle.Value & 0x7FC));
        }
        else
        {
            ULONG_PTR tmp = (tHandle.Value - tHandle.Value & 0x7FC) >> 9;
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(*(ULONG_PTR*)(Diff + ((tHandle.Value - tmp - tmp & 0xFFF) >> 10)) + (tmp & 0xFFF)) + 4 * (tHandle.Value & 0x7FC));
        }
    }
    else
    {
        return (PHANDLE_TABLE_ENTRY)(Diff + 4 * tHandle.Value);
    }
#else
    if (TableCode != 0)
    {
        if (TableCode == 1)
        {
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(HandleTable->TableCode + 8 * (tHandle.Value >> 10) - 1) + 4 * (tHandle.Value & 0x3FF));
        }
        else
        {
            ULONG_PTR tmp = tHandle.Value >> 10;
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(*(ULONG_PTR*)(HandleTable->TableCode + 8 * (tHandle.Value >> 19) - 2) + 8 * (tmp & 0x1FF)) + 4 * (tHandle.Value & 0x3FF));
        }
    }
    else
    {
        return (PHANDLE_TABLE_ENTRY)(HandleTable->TableCode + 4 * tHandle.Value);
    }
#endif
}


/// <summary>
/// Get ntoskrnl base address
/// </summary>
/// <param name="pSize">Size of module</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetKernelBase( OUT PULONG pSize )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytes = 0;
    PRTL_PROCESS_MODULES pMods = NULL;
    PVOID checkPtr = NULL;
    UNICODE_STRING routineName;

    // Already found
    if (g_KernelBase != NULL)
    {
        if (pSize)
            *pSize = g_KernelSize;
        return g_KernelBase;
    }

    RtlUnicodeStringInit( &routineName, L"NtOpenFile" );

    checkPtr = MmGetSystemRoutineAddress( &routineName );
    if (checkPtr == NULL)
        return NULL;

    // Protect from UserMode AV
    status = ZwQuerySystemInformation( SystemModuleInformation, 0, bytes, &bytes );
    if (bytes == 0)
    {
        DPRINT( "BlackBone: %s: Invalid SystemModuleInformation size\n", __FUNCTION__ );
        return NULL;
    }

    pMods = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag( NonPagedPool, bytes, BB_POOL_TAG );
    RtlZeroMemory( pMods, bytes );

    status = ZwQuerySystemInformation( SystemModuleInformation, pMods, bytes, &bytes );

    if (NT_SUCCESS( status ))
    {
        PRTL_PROCESS_MODULE_INFORMATION pMod = pMods->Modules;

        for (ULONG i = 0; i < pMods->NumberOfModules; i++)
        {
            // System routine is inside module
            if (checkPtr >= pMod[i].ImageBase &&
                checkPtr < (PVOID)((PUCHAR)pMod[i].ImageBase + pMod[i].ImageSize))
            {
                g_KernelBase = pMod[i].ImageBase;
                g_KernelSize = pMod[i].ImageSize;
                if (pSize)
                    *pSize = g_KernelSize;
                break;
            }
        }
    }

    if (pMods)
        ExFreePoolWithTag( pMods, BB_POOL_TAG );

    return g_KernelBase;
}

/// <summary>
/// Gets SSDT base - KiServiceTable
/// </summary>
/// <returns>SSDT base, NULL if not found</returns>
PSYSTEM_SERVICE_DESCRIPTOR_TABLE GetSSDTBase()
{
    PUCHAR ntosBase = GetKernelBase( NULL );

    // Already found
    if (g_SSDT != NULL)
        return g_SSDT;

    if (!ntosBase)
        return NULL;
    
    PIMAGE_NT_HEADERS pHdr = RtlImageNtHeader( ntosBase );
    PIMAGE_SECTION_HEADER pFirstSec = (PIMAGE_SECTION_HEADER)(pHdr + 1);
    for (PIMAGE_SECTION_HEADER pSec = pFirstSec; pSec < pFirstSec + pHdr->FileHeader.NumberOfSections; pSec++)
    {
        // Non-paged, non-discardable, readable sections
        // Probably still not fool-proof enough...
        if (pSec->Characteristics & IMAGE_SCN_MEM_NOT_PAGED &&
            pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
            !(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
            (*(PULONG)pSec->Name != 'TINI') &&
            (*(PULONG)pSec->Name != 'EGAP'))
        {
            PVOID pFound = NULL;

            // KiSystemServiceRepeat pattern
            UCHAR pattern[] = "\x4c\x8d\x15\xcc\xcc\xcc\xcc\x4c\x8d\x1d\xcc\xcc\xcc\xcc\xf7";
            NTSTATUS status = BBSearchPattern( pattern, 0xCC, sizeof( pattern ) - 1, ntosBase + pSec->VirtualAddress, pSec->Misc.VirtualSize, &pFound );
            if (NT_SUCCESS( status ))
            {
                g_SSDT = (PSYSTEM_SERVICE_DESCRIPTOR_TABLE)((PUCHAR)pFound + *(PULONG)((PUCHAR)pFound + 3) + 7);
                //DPRINT( "BlackBone: %s: KeSystemServiceDescriptorTable = 0x%p\n", __FUNCTION__, g_SSDT );
                return g_SSDT;
            }
        }
    }
   
    return NULL;
}

/// <summary>
/// Gets the SSDT entry address by index.
/// </summary>
/// <param name="index">Service index</param>
/// <returns>Found service address, NULL if not found</returns>
PVOID GetSSDTEntry( IN ULONG index )
{
    ULONG size = 0;
    PSYSTEM_SERVICE_DESCRIPTOR_TABLE pSSDT = GetSSDTBase();
    PVOID pBase = GetKernelBase( &size );

    if (pSSDT && pBase)
    {
        // Index range check
        if (index > pSSDT->NumberOfServices)
            return NULL;

        return (PUCHAR)pSSDT->ServiceTableBase + (((PLONG)pSSDT->ServiceTableBase)[index] >> 4);
    }

    return NULL;
}

/// <summary>
/// Get page hardware PTE.
/// Address must be valid, otherwise bug check is imminent
/// </summary>
/// <param name="pAddress">Target address</param>
/// <returns>Found PTE</returns>
PMMPTE GetPTEForVA( IN PVOID pAddress )
{
    if (dynData.ver >= WINVER_10_AU)
    {
        // Check if large page
        PMMPTE pPDE = (PMMPTE)(((((ULONG_PTR)pAddress >> PDI_SHIFT) << PTE_SHIFT) & 0x3FFFFFF8ull) + dynData.DYN_PDE_BASE);
        if (pPDE->u.Hard.LargePage)
            return pPDE;

        return (PMMPTE)(((((ULONG_PTR)pAddress >> PTI_SHIFT) << PTE_SHIFT) & 0x7FFFFFFFF8ull) + dynData.DYN_PTE_BASE);
    }
    else
    {
        // Check if large page
        PMMPTE pPDE = MiGetPdeAddress( pAddress );
        if (pPDE->u.Hard.LargePage)
            return pPDE;

        return MiGetPteAddress( pAddress );
    }
}

VOID DpcRoutine( KDPC *pDpc, void *pContext, void *pArg1, void *pArg2 )
{
    KIRQL Irql;
    PNOPPROCINFO Info = (PNOPPROCINFO)pContext;

    UNREFERENCED_PARAMETER( pDpc );
    UNREFERENCED_PARAMETER( pArg1 );
    UNREFERENCED_PARAMETER( pArg2 );

    InterlockedIncrement( &Info->DPCCount );
    do
    {
        __nop();
    } while (Info->ActiveCores != Info->DPCCount);

    KeRaiseIrql( HIGH_LEVEL, &Irql );
    do
    {
        __nop();
    } while (!Info->IsCodeExecuted);

    InterlockedDecrement( &Info->DPCCount );
    KeLowerIrql( Irql );
}

/// <summary>
/// Initialize structure for processor start/stop
/// </summary>
/// <param name="Info">>Processors data</param>
VOID InitializeStopProcessors( OUT NOPPROCINFO* Info )
{
    KAFFINITY aff = 0;
    RtlZeroMemory( Info, sizeof( NOPPROCINFO ) );

    Info->Cores = KeQueryActiveProcessorCount( &aff );

    if (Info->Cores > 1)
    {
        for (ULONG i = 0; i < Info->Cores; i++)
        {
            KeInitializeDpc( &Info->DpcTraps[i], DpcRoutine, Info );
            KeSetImportanceDpc( &Info->DpcTraps[i], LowImportance );
            KeSetTargetProcessorDpc( &Info->DpcTraps[i], (CCHAR)i );
        }
    }
}

/// <summary>
/// Stall all but current active processors 
/// </summary>
/// <param name="Info">Processors data</param>
VOID StopProcessors( IN NOPPROCINFO* Info )
{
    ULONG CurrentProcessor;
    KAFFINITY ActiveProcessors;

    if (Info->Cores > 1)
    {
        Info->SavedPriority = KeSetPriorityThread( KeGetCurrentThread(), HIGH_PRIORITY );
        ActiveProcessors = KeQueryActiveProcessors();

        KeRaiseIrql( DISPATCH_LEVEL, &Info->SavedIrql );

        CurrentProcessor = KeGetCurrentProcessorNumber();
        Info->ActiveCores = Info->DPCCount = 0;

        for (ULONG i = 0; i < Info->Cores; i++)
        {
            if ((i != CurrentProcessor) && ((ActiveProcessors & (1ull << i)) != 0))
            {
                InterlockedIncrement( &Info->ActiveCores );
                KeInsertQueueDpc( &Info->DpcTraps[i], &Info, 0 );
            }
        }

        KeLowerIrql( Info->SavedIrql );

        do
        {
            __nop();
        } while (Info->ActiveCores != Info->DPCCount);

        KeRaiseIrql( HIGH_LEVEL, &Info->SavedIrql );
    }
    else
        KeRaiseIrql( HIGH_LEVEL, &Info->SavedIrql );
};

/// <summary>
/// Resume all stopped active processors 
/// </summary>
/// <param name="Info">Processors data</param>
VOID StartProcessors( IN NOPPROCINFO* Info )
{
    if (Info->Cores > 1)
    {
        InterlockedExchange( &Info->IsCodeExecuted, 1 );
        KeLowerIrql( Info->SavedIrql );

        do
        {
            __nop();
        } while (Info->DPCCount > 0);

        KeSetPriorityThread( KeGetCurrentThread(), Info->SavedPriority );
    }
    else
        KeLowerIrql( Info->SavedIrql );
}

/// <summary>
/// Allocate memory in one of the ntoskrnl discarded section
/// </summary>
/// <param name="SizeOfImage">Block size to allocate</param>
/// <param name="ppFoundBase">Allocated address</param>
/// <returns>Status code</returns>
NTSTATUS AllocateInDiscardedMemory( IN ULONG size, OUT PVOID* ppFoundBase )
{
    ASSERT( ppFoundBase != NULL );
    if (ppFoundBase == NULL)
        return STATUS_INVALID_PARAMETER;

    // Ensure MiAllocateDriverPage address is valid
    if (dynData.MiAllocPage == 0)
        return STATUS_INVALID_ADDRESS;

    PVOID pBase = GetKernelBase( NULL );
    fnMiAllocateDriverPage MiAllocateDriverPage = (fnMiAllocateDriverPage)((ULONG_PTR)pBase + dynData.MiAllocPage);

    PIMAGE_NT_HEADERS pNTOSHdr = RtlImageNtHeader( pBase );
    if (!pNTOSHdr)
        return STATUS_INVALID_IMAGE_FORMAT;

    // Walk ntoskrnl section
    PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pNTOSHdr + 1);
    PIMAGE_SECTION_HEADER pLastSection = pFirstSection + pNTOSHdr->FileHeader.NumberOfSections;

    for (PIMAGE_SECTION_HEADER pSection = pLastSection - 1; pSection >= pFirstSection; --pSection)
    {
        // Find first suitable discarded section
        if (pSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE && (ULONG_PTR)PAGE_ALIGN( pSection->Misc.VirtualSize ) >= size)
        {
            // TODO: implement some randomization for starting address
            PVOID pSectionBase = (PUCHAR)pBase + pSection->VirtualAddress;

            // I don't care about large pages
            // If image was mapped using large pages bugcheck is imminent
            ULONG_PTR TotalPTEs = BYTES_TO_PAGES( size );
            PMMPTE pStartPTE = MiGetPteAddress( pSectionBase );
            PMMPTE pEndPTE = pStartPTE + TotalPTEs;
            MMPTE TempPTE = ValidKernelPte;

            // Allocate physical pages for PTEs
            for (PMMPTE pPTE = pStartPTE; pPTE < pEndPTE; ++pPTE)
            {
                PVOID VA = MiGetVirtualAddressMappedByPte( pPTE );

                // Already allocated
                if (MI_IS_PHYSICAL_ADDRESS( VA ))
                {
                    //DPRINT( "BlackBone: %s: VA 0x%p is already backed by PFN: 0x%p\n", __FUNCTION__, VA, pPTE->u.Hard.PageFrameNumber );
                    continue;
                }

                PFN_NUMBER pfn = MiAllocateDriverPage( pPTE );
                if (pfn == 0)
                {
                    DPRINT( "BlackBone: %s: Failed to allocate physical page for PTE 0x%p\n", __FUNCTION__, pPTE );
                    return STATUS_NO_MEMORY;
                }
                else
                {
                    //DPRINT( "BlackBone: %s: VA 0x%p now backed by PFN: 0x%p; PTE: 0x%p\n", __FUNCTION__, VA, pfn, pPTE );
                    TempPTE.u.Hard.PageFrameNumber = pfn;
                    *pPTE = TempPTE;
                }
            }

            *ppFoundBase = pSectionBase;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}



#if defined(_WIN8_) || defined (_WIN7_)

NTSTATUS
NTAPI
ZwProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID* BaseAddress,
    IN OUT SIZE_T* NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    fnNtProtectVirtualMemory NtProtectVirtualMemory = (fnNtProtectVirtualMemory)(ULONG_PTR)GetSSDTEntry( dynData.NtProtectIndex );
    if (NtProtectVirtualMemory)
    {
        //
        // If previous mode is UserMode, addresses passed into NtProtectVirtualMemory must be in user-mode space
        // Switching to KernelMode allows usage of kernel-mode addresses
        //
        PUCHAR pPrevMode = (PUCHAR)PsGetCurrentThread() + dynData.PrevMode;
        UCHAR prevMode = *pPrevMode;
        PVOID BaseCopy = NULL;
        SIZE_T SizeCopy = 0;
        *pPrevMode = KernelMode;

        if (BaseAddress)
            BaseCopy = *BaseAddress;

        if (NumberOfBytesToProtect)
            SizeCopy = *NumberOfBytesToProtect;

        status = NtProtectVirtualMemory( ProcessHandle, &BaseCopy, &SizeCopy, NewAccessProtection, OldAccessProtection );

        *pPrevMode = prevMode;
    }
    else
        status = STATUS_NOT_FOUND;

    return status;
}
#endif

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
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    fnNtCreateThreadEx NtCreateThreadEx = (fnNtCreateThreadEx)(ULONG_PTR)GetSSDTEntry( dynData.NtCreateThdIndex );
    if (NtCreateThreadEx)
    {
        //
        // If previous mode is UserMode, addresses passed into ZwCreateThreadEx must be in user-mode space
        // Switching to KernelMode allows usage of kernel-mode addresses
        //
        PUCHAR pPrevMode = (PUCHAR)PsGetCurrentThread() + dynData.PrevMode;
        UCHAR prevMode = *pPrevMode;
        *pPrevMode = KernelMode;

        status = NtCreateThreadEx(
            hThread, DesiredAccess, ObjectAttributes,
            ProcessHandle, lpStartAddress, lpParameter,
            Flags, StackZeroBits, SizeOfStackCommit,
            SizeOfStackReserve, AttributeList
            );

        *pPrevMode = prevMode;
    }
    else
        status = STATUS_NOT_FOUND;

    return status;
}

NTSTATUS NTAPI ZwTerminateThread( IN HANDLE ThreadHandle, IN NTSTATUS ExitStatus )
{
    NTSTATUS status = STATUS_SUCCESS;

    fnNtTerminateThread NtTerminateThread = (fnNtTerminateThread)(ULONG_PTR)GetSSDTEntry( dynData.NtTermThdIndex );
    if (NtTerminateThread)
    {
        PUCHAR pPrevMode = (PUCHAR)PsGetCurrentThread() + dynData.PrevMode;
        UCHAR prevMode = *pPrevMode;
        *pPrevMode = KernelMode;

        status = NtTerminateThread( ThreadHandle, ExitStatus );
        *pPrevMode = prevMode;
    }
    else
        status = STATUS_NOT_FOUND;

    return status;
}