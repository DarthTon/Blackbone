#include "BlackBoneDrv.h"
#include "Routines.h"
#include "Utils.h"
#include <Ntstrsafe.h>

LIST_ENTRY g_PhysProcesses;
PVOID g_kernelPage = NULL;  // Trampoline buffer page
LONG g_trIndex = 0;         // Trampoline global index

/// <summary>
/// Find allocated memory region entry
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="pBase">Region base</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_ENTRY BBLookupPhysMemEntry( IN PLIST_ENTRY pList, IN PVOID pBase );
VOID BBWriteTrampoline( IN PUCHAR place, IN PVOID pfn );
BOOLEAN BBHandleCallback(
#if !defined(_WIN7_)
    IN PHANDLE_TABLE HandleTable,
#endif
    IN PHANDLE_TABLE_ENTRY HandleTableEntry, 
    IN HANDLE Handle, 
    IN PVOID EnumParameter 
    );

#pragma alloc_text(PAGE, BBDisableDEP)
#pragma alloc_text(PAGE, BBSetProtection)
#pragma alloc_text(PAGE, BBHandleCallback)
#pragma alloc_text(PAGE, BBGrantAccess)
#pragma alloc_text(PAGE, BBCopyMemory)
#pragma alloc_text(PAGE, BBAllocateFreeMemory)
#pragma alloc_text(PAGE, BBAllocateFreePhysical)
#pragma alloc_text(PAGE, BBProtectMemory)
#pragma alloc_text(PAGE, BBWriteTrampoline)
#pragma alloc_text(PAGE, BBHookSSDT)

#pragma alloc_text(PAGE, BBLookupPhysProcessEntry)
#pragma alloc_text(PAGE, BBLookupPhysMemEntry)

#pragma alloc_text(PAGE, BBCleanupPhysMemEntry)
#pragma alloc_text(PAGE, BBCleanupProcessPhysEntry)
#pragma alloc_text(PAGE, BBCleanupProcessPhysList)

/// <summary>
/// Disable process DEP
/// Has no effect on native x64 process
/// </summary>
/// <param name="pData">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBDisableDEP( IN PDISABLE_DEP pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        if (dynData.KExecOpt != 0)
        {
            PKEXECUTE_OPTIONS pExecOpt = (PKEXECUTE_OPTIONS)((PUCHAR)pProcess + dynData.KExecOpt);

            // Reset all flags
            pExecOpt->ExecuteOptions = 0;

            pExecOpt->Flags.ExecuteEnable = 1;          //
            pExecOpt->Flags.ImageDispatchEnable = 1;    // Disable all checks
            pExecOpt->Flags.ExecuteDispatchEnable = 1;  //
        }
        else
        {
            DPRINT( "BlackBone: %s: Invalid _KEXECUTE_OPTIONS offset\n", __FUNCTION__ );
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Enable/disable process protection flag
/// </summary>
/// <param name="pProtection">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBSetProtection( IN PSET_PROC_PROTECTION pProtection )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pProtection->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        if (dynData.Protection != 0)
        {
            PUCHAR pValue = (PUCHAR)pProcess + dynData.Protection;

            // Win7
            if (dynData.ver <= WINVER_7_SP1)
            {
                if (pProtection->protection == Policy_Enable)
                    *(PULONG)pValue |= 1 << 0xB;
                else if (pProtection->protection == Policy_Disable)
                    *(PULONG)pValue &= ~(1 << 0xB);
            }
            // Win8
            else if (dynData.ver == WINVER_8)
            {
                if (pProtection->protection != Policy_Keep)
                    *pValue = pProtection->protection;
            }
            // Win8.1
            else if (dynData.ver >= WINVER_81)
            {
                // Protection
                if (pProtection->protection == Policy_Disable)
                {
                    *pValue = 0;
                }
                else if(pProtection->protection == Policy_Enable)
                {
                    PS_PROTECTION protBuf = { 0 };

                    protBuf.Flags.Signer = PsProtectedSignerWinTcb;
                    protBuf.Flags.Type = PsProtectedTypeProtected;
                    *pValue = protBuf.Level;
                }

                // Dynamic code
                if (pProtection->dynamicCode != Policy_Keep && dynData.EProcessFlags2 != 0)
                {
                    PEPROCESS_FLAGS2 pFlags2 = (PEPROCESS_FLAGS2)((PUCHAR)pProcess + dynData.EProcessFlags2);
                    pFlags2->DisableDynamicCode = pProtection->dynamicCode;
                }
                
                // Binary signature
                if (pProtection->signature != Policy_Keep)
                {
                    PSE_SIGNING_LEVEL pSignLevel = (PSE_SIGNING_LEVEL)((PUCHAR)pProcess + dynData.Protection - 2);
                    PSE_SIGNING_LEVEL pSignLevelSection = (PSE_SIGNING_LEVEL)((PUCHAR)pProcess + dynData.Protection - 1);

                    if(pProtection->signature == Policy_Enable)
                        *pSignLevel = *pSignLevelSection = SE_SIGNING_LEVEL_MICROSOFT;
                    else
                        *pSignLevel = *pSignLevelSection = SE_SIGNING_LEVEL_UNCHECKED;
                }                
            }
            else
                status = STATUS_NOT_SUPPORTED;
        }
        else
        {
            DPRINT( "BlackBone: %s: Invalid protection flag offset\n", __FUNCTION__ );
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Handle enumeration callback
/// </summary>
/// <param name="HandleTable">Process handle table</param>
/// <param name="HandleTableEntry">Handle entry</param>
/// <param name="Handle">Handle value</param>
/// <param name="EnumParameter">User context</param>
/// <returns>TRUE when desired handle is found</returns>
BOOLEAN BBHandleCallback(
#if !defined(_WIN7_)
    IN PHANDLE_TABLE HandleTable,
#endif
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN HANDLE Handle,
    IN PVOID EnumParameter
    )
{

    BOOLEAN result = FALSE;
    ASSERT( EnumParameter );

    if (EnumParameter != NULL)
    {
        PHANDLE_GRANT_ACCESS pAccess = (PHANDLE_GRANT_ACCESS)EnumParameter;
        if (Handle == (HANDLE)pAccess->handle)
        {
            if (ExpIsValidObjectEntry( HandleTableEntry ))
            {
                // Update access
                HandleTableEntry->GrantedAccessBits = pAccess->access;
                result = TRUE;
            }
            else
                DPRINT( "BlackBone: %s: 0x%X:0x%X handle is invalid\n. HandleEntry = 0x%p",
                    __FUNCTION__, pAccess->pid, pAccess->handle, HandleTableEntry
                    );
        }
    }

#if !defined(_WIN7_)
    // Release implicit locks
    _InterlockedExchangeAdd8( (char*)&HandleTableEntry->VolatileLowValue, 1 );  // Set Unlocked flag to 1
    if (HandleTable != NULL && HandleTable->HandleContentionEvent)
        ExfUnblockPushLock( &HandleTable->HandleContentionEvent, NULL );
#endif

    return result;
}

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBGrantAccess( IN PHANDLE_GRANT_ACCESS pAccess )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    // Validate dynamic offset
    if (dynData.ObjTable == 0)
    {
        DPRINT( "BlackBone: %s: Invalid ObjTable address\n", __FUNCTION__ );
        return STATUS_INVALID_ADDRESS;
    }

    status = PsLookupProcessByProcessId( (HANDLE)pAccess->pid, &pProcess );
    if (NT_SUCCESS( status ) && BBCheckProcessTermination( pProcess ))
        status = STATUS_PROCESS_IS_TERMINATING;

    if (NT_SUCCESS( status ))
    {
        PHANDLE_TABLE pTable = *(PHANDLE_TABLE*)((PUCHAR)pProcess + dynData.ObjTable);
        BOOLEAN found = ExEnumHandleTable( pTable, &BBHandleCallback, pAccess, NULL );
        if (found == FALSE)
            status = STATUS_NOT_FOUND;
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkHandleTable( IN PUNLINK_HTABLE pUnlink )
{
    NTSTATUS  status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    // Validate dynamic offset
    if (dynData.ExRemoveTable == 0 || dynData.ObjTable == 0)
    {
        DPRINT( "BlackBone: %s: Invalid ExRemoveTable/ObjTable address\n", __FUNCTION__ );
        return STATUS_INVALID_ADDRESS;
    }

    // Validate build
    if (dynData.correctBuild == FALSE)
    {
        DPRINT( "BlackBone: %s: Unsupported kernel build version\n", __FUNCTION__ );
        return STATUS_INVALID_KERNEL_INFO_VERSION;
    }

    status = PsLookupProcessByProcessId( (HANDLE)pUnlink->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        PHANDLE_TABLE pTable = *(PHANDLE_TABLE*)((PUCHAR)pProcess + dynData.ObjTable);

        // Unlink process handle table
        fnExRemoveHandleTable ExRemoveHandleTable = (fnExRemoveHandleTable)((ULONG_PTR)GetKernelBase( NULL ) + dynData.ExRemoveTable);
        //DPRINT( "BlackBone: %s: ExRemoveHandleTable address 0x%p. Object Table offset: 0x%X\n", 
               // __FUNCTION__, ExRemoveHandleTable, dynData.ObjTable );

        ExRemoveHandleTable( pTable );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Read/write process memory
/// </summary>
/// <param name="pCopy">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBCopyMemory( IN PCOPY_MEMORY pCopy )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL, pSourceProc = NULL, pTargetProc = NULL;
    PVOID pSource = NULL, pTarget = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pCopy->pid, &pProcess );

    if (NT_SUCCESS( status ))
    {
        SIZE_T bytes = 0;

        // Write
        if (pCopy->write != FALSE)
        {
            pSourceProc = PsGetCurrentProcess();
            pTargetProc = pProcess;
            pSource = (PVOID)pCopy->localbuf;
            pTarget = (PVOID)pCopy->targetPtr;
        }
        // Read
        else
        {
            pSourceProc = pProcess;
            pTargetProc = PsGetCurrentProcess();
            pSource = (PVOID)pCopy->targetPtr;
            pTarget = (PVOID)pCopy->localbuf;
        }

        status = MmCopyVirtualMemory( pSourceProc, pSource, pTargetProc, pTarget, pCopy->size, KernelMode, &bytes );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Allocate/Free process memory
/// </summary>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreeMemory( IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    ASSERT( pResult != NULL );
    if (pResult == NULL)
        return STATUS_INVALID_PARAMETER;

    status = PsLookupProcessByProcessId( (HANDLE)pAllocFree->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        PVOID base = (PVOID)pAllocFree->base;
        ULONG_PTR size = pAllocFree->size;

        KeStackAttachProcess( pProcess, &apc );

        if (pAllocFree->allocate)
        {
            if (pAllocFree->physical != FALSE)
            {
                status = BBAllocateFreePhysical( pProcess, pAllocFree, pResult );
            }
            else
            {
                status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &base, 0, &size, pAllocFree->type, pAllocFree->protection );
                pResult->address = (ULONGLONG)base;
                pResult->size = size;
            }
        }
        else
        {
            MI_VAD_TYPE vadType = VadNone;
            BBGetVadType( pProcess, pAllocFree->base, &vadType );

            if (vadType == VadDevicePhysicalMemory)
                status = BBAllocateFreePhysical( pProcess, pAllocFree, pResult );
            else
                status = ZwFreeVirtualMemory( ZwCurrentProcess(), &base, &size, pAllocFree->type );
        }

        KeUnstackDetachProcess( &apc );        
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Allocate kernel memory and map into User space. Or free previously allocated memory
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreePhysical( IN PEPROCESS pProcess, IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pRegionBase = NULL;
    PMDL pMDL = NULL;

    ASSERT( pProcess != NULL && pResult != NULL );
    if (pProcess == NULL || pResult == NULL)
        return STATUS_INVALID_PARAMETER;

    // MDL doesn't support regions this large
    if (pAllocFree->size > 0xFFFFFFFF)
    {
        DPRINT( "BlackBone: %s: Region size if too big: 0x%p\n", __FUNCTION__, pAllocFree->size );
        return STATUS_INVALID_PARAMETER;
    }

    // Align on page boundaries   
    pAllocFree->base = (ULONGLONG)PAGE_ALIGN( pAllocFree->base );
    pAllocFree->size = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pAllocFree->base, pAllocFree->size ) << PAGE_SHIFT;

    // Allocate
    if (pAllocFree->allocate != FALSE)
    {
        PMMVAD_SHORT pVad = NULL;
        if (pAllocFree->base != 0 && BBFindVAD( pProcess, pAllocFree->base, &pVad ) != STATUS_NOT_FOUND)
            return STATUS_ALREADY_COMMITTED;

        pRegionBase = ExAllocatePoolWithTag( NonPagedPool, pAllocFree->size, BB_POOL_TAG );
        if (!pRegionBase)
            return STATUS_NO_MEMORY;

        // Cleanup buffer before mapping it into UserMode to prevent exposure of kernel data
        RtlZeroMemory( pRegionBase, pAllocFree->size );

        pMDL = IoAllocateMdl( pRegionBase, (ULONG)pAllocFree->size, FALSE, FALSE, NULL );
        if (pMDL == NULL)
        {
            ExFreePoolWithTag( pRegionBase, BB_POOL_TAG );
            return STATUS_NO_MEMORY;
        }

        MmBuildMdlForNonPagedPool( pMDL );

        // Map at original base
        __try {
            pResult->address = (ULONGLONG)MmMapLockedPagesSpecifyCache( 
                pMDL, UserMode, MmCached, (PVOID)pAllocFree->base, FALSE, NormalPagePriority 
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }

        // Map at any suitable
        if (pResult->address == 0 && pAllocFree->base != 0)
        {
            __try {
                pResult->address = (ULONGLONG)MmMapLockedPagesSpecifyCache(
                    pMDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

        if (pResult->address)
        {
            PMEM_PHYS_PROCESS_ENTRY pEntry = NULL;
            PMEM_PHYS_ENTRY pMemEntry = NULL;

            pResult->size = pAllocFree->size;

            // Set initial protection
            BBProtectVAD( pProcess, pResult->address, BBConvertProtection( pAllocFree->protection, FALSE ) );

            // Make pages executable
            if (pAllocFree->protection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
            {
                for (ULONG_PTR pAdress = pResult->address; pAdress < pResult->address + pResult->size; pAdress += PAGE_SIZE)
                    GetPTEForVA( (PVOID)pAdress )->u.Hard.NoExecute = 0;
            }

            // Add to list
            pEntry = BBLookupPhysProcessEntry( (HANDLE)pAllocFree->pid );
            if (pEntry == NULL)
            {
                pEntry = ExAllocatePoolWithTag( PagedPool, sizeof( MEM_PHYS_PROCESS_ENTRY ), BB_POOL_TAG );
                pEntry->pid = (HANDLE)pAllocFree->pid;

                InitializeListHead( &pEntry->pVadList );
                InsertTailList( &g_PhysProcesses, &pEntry->link );
            }

            pMemEntry = ExAllocatePoolWithTag( PagedPool, sizeof( MEM_PHYS_ENTRY ), BB_POOL_TAG );
            
            pMemEntry->pMapped = (PVOID)pResult->address;
            pMemEntry->pMDL = pMDL;
            pMemEntry->ptr = pRegionBase;
            pMemEntry->size = pAllocFree->size;

            InsertTailList( &pEntry->pVadList, &pMemEntry->link );
        }
        else
        {
            // Failed, cleanup
            IoFreeMdl( pMDL );
            ExFreePoolWithTag( pRegionBase, BB_POOL_TAG );

            status = STATUS_NONE_MAPPED;
        }
    }
    // Free
    else
    {
        PMEM_PHYS_PROCESS_ENTRY pEntry = BBLookupPhysProcessEntry( (HANDLE)pAllocFree->pid );

        if (pEntry != NULL)
        {
            PMEM_PHYS_ENTRY pMemEntry = BBLookupPhysMemEntry( &pEntry->pVadList, (PVOID)pAllocFree->base );

            if (pMemEntry != NULL)
                BBCleanupPhysMemEntry( pMemEntry, TRUE );
            else
                status = STATUS_NOT_FOUND;
        }
        else
            status = STATUS_NOT_FOUND;
    }

    return status;
}

/// <summary>
/// Change process memory protection
/// </summary>
/// <param name="pProtect">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectMemory( IN PPROTECT_MEMORY pProtect )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pProtect->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        MI_VAD_TYPE vadType = VadNone;
        PVOID base = (PVOID)pProtect->base;
        SIZE_T size = (SIZE_T)pProtect->size;
        ULONG oldProt = 0;

        KeStackAttachProcess( pProcess, &apc );

        // Handle physical allocations
        status = BBGetVadType( pProcess, pProtect->base, &vadType );
        if (NT_SUCCESS( status ))
        {
            if (vadType == VadDevicePhysicalMemory)
            {
                // Align on page boundaries   
                pProtect->base = (ULONGLONG)PAGE_ALIGN( pProtect->base );
                pProtect->size = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pProtect->base, pProtect->size ) << PAGE_SHIFT;

                status = BBProtectVAD( pProcess, pProtect->base, BBConvertProtection( pProtect->newProtection, FALSE ) );

                // Update PTE
                for (ULONG_PTR pAdress = pProtect->base; pAdress < pProtect->base + pProtect->size; pAdress += PAGE_SIZE)
                {
                    PMMPTE pPTE = GetPTEForVA( (PVOID)pAdress );

                    // Executable
                    if (pProtect->newProtection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
                        pPTE->u.Hard.NoExecute = 0;

                    // Read-only
                    if (pProtect->newProtection & (PAGE_READONLY | PAGE_EXECUTE | PAGE_EXECUTE_READ))
                        pPTE->u.Hard.Dirty1 = pPTE->u.Hard.Write = 0;
                }
            }
            else
                status = ZwProtectVirtualMemory( ZwCurrentProcess(), &base, &size, pProtect->newProtection, &oldProt );
        }

        KeUnstackDetachProcess( &apc );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Hide VAD containing target address
/// </summary>
/// <param name="pData">Address info</param>
/// <returns>Status code</returns>
NTSTATUS BBHideVAD( IN PHIDE_VAD pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess );
    if (NT_SUCCESS( status ))
        status = BBUnlinkVAD( pProcess, pData->base );
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Enumerate committed, accessible, non-guarded memory regions
/// </summary>
/// <param name="pData">Target process ID</param>
/// <param name="pResult">Result</param>
/// <returns>Status code</returns>
NTSTATUS BBEnumMemRegions( IN PENUM_REGIONS pData, OUT PENUM_REGIONS_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;
    LIST_ENTRY memList;
    ULONG totalCount = 0;

    ASSERT( pResult != NULL && pData != NULL && pData->pid != 0 );
    if (pResult == NULL || pData == NULL || pData->pid == 0)
        return STATUS_INVALID_PARAMETER;

    InitializeListHead( &memList );

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        KeStackAttachProcess( pProcess, &apc );
        status = BBBuildProcessRegionListForRange( &memList, (ULONG_PTR)MM_LOWEST_USER_ADDRESS, (ULONG_PTR)MM_HIGHEST_USER_ADDRESS, TRUE );
        KeUnstackDetachProcess( &apc );
    }

    if (NT_SUCCESS( status ))
    {
        for (PLIST_ENTRY pListEntry = memList.Flink; pListEntry != &memList; pListEntry = pListEntry->Flink, totalCount++);

        // Not enough memory
        if (pResult->count < totalCount)
        {
            pResult->count = totalCount;
            status = STATUS_BUFFER_TOO_SMALL;
        }
        // Copy from list to array
        else
        {
            ULONG i = 0;
            pResult->count = totalCount;
            for (PLIST_ENTRY pListEntry = memList.Flink; pListEntry != &memList; pListEntry = pListEntry->Flink, i++)
            {
                PMEMORY_BASIC_INFORMATION pMem = &CONTAINING_RECORD( pListEntry, MAP_ENTRY, link )->mem;
                pResult->regions[i].AllocationBase = (ULONGLONG)pMem->AllocationBase;
                pResult->regions[i].AllocationProtect = pMem->AllocationProtect;
                pResult->regions[i].BaseAddress = (ULONGLONG)pMem->BaseAddress;
                pResult->regions[i].Protect = pMem->Protect;
                pResult->regions[i].RegionSize = pMem->RegionSize;
                pResult->regions[i].State = pMem->State;
                pResult->regions[i].Type = pMem->Type;
            }
        }
    }

    // Cleanup list
    while (!IsListEmpty( &memList ))
    {
        PMAP_ENTRY ptr = CONTAINING_RECORD( memList.Flink, MAP_ENTRY, link );
        RemoveEntryList( &ptr->link );
        ExFreePoolWithTag( ptr, BB_POOL_TAG );
    }

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Create hook trampoline
/// </summary>
/// <param name="place">Trampoline address</param>
/// <param name="pfn">Function pointer</param>
VOID BBWriteTrampoline( IN PUCHAR place, IN PVOID pfn )
{
    if (!place || !pfn)
        return;

    // jmp [rip + 0]
    ULONGLONG cr0 = __readcr0();
    __writecr0( cr0 & 0xFFFEFFFF );
    *(PUSHORT)place = 0x25FF;
    *(PULONG)(place + 2) = 0;
    *(PVOID**)(place + 6) = pfn;
    __writecr0( cr0 );
}

PVOID BBFindTrampolineSpace( IN PVOID lowest )
{
    ASSERT( lowest != NULL );
    if (lowest == NULL)
        return NULL;

    const ULONG size = sizeof( USHORT ) + sizeof( ULONG ) + sizeof( PVOID );
    PUCHAR ntosBase = GetKernelBase( NULL );
    if (!ntosBase)
        return NULL;

    PIMAGE_NT_HEADERS pHdr = RtlImageNtHeader( ntosBase );
    PIMAGE_SECTION_HEADER pFirstSec = (PIMAGE_SECTION_HEADER)(pHdr + 1);
    for (PIMAGE_SECTION_HEADER pSec = pFirstSec; pSec < pFirstSec + pHdr->FileHeader.NumberOfSections; pSec++)
    {
        // First appropriate section
        if ((PUCHAR)lowest < ntosBase + pSec->VirtualAddress || (PUCHAR)lowest < ntosBase + pSec->VirtualAddress + (ULONG_PTR)PAGE_ALIGN( pSec->Misc.VirtualSize ))
        {
            if (pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
                !(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
                (*(PULONG)pSec->Name != 'TINI'))
            {
                ULONG_PTR offset = 0;
                if((PUCHAR)lowest >= ntosBase + pSec->VirtualAddress)
                    offset = (PUCHAR)lowest - ntosBase - pSec->VirtualAddress;

                for (ULONG_PTR i = offset, bytes = 0; i < (ULONG_PTR)PAGE_ALIGN( pSec->Misc.VirtualSize - size ); i++)
                {
                    // int3, nop, or inside unused section space
                    if (ntosBase[pSec->VirtualAddress + i] == 0xCC || ntosBase[pSec->VirtualAddress + i] == 0x90 || i > pSec->Misc.VirtualSize - size)
                        bytes++;
                    else
                        bytes = 0;

                    if (bytes >= size)
                        return &ntosBase[pSec->VirtualAddress + i - bytes + 1];
                }
            }
        }
    }

    return NULL;
}

/// <summary>
/// Hook SSDT entry
/// </summary>
/// <param name="index">SSDT index to hook</param>
/// <param name="newAddr">Hook function</param>
/// <param name="ppOldAddr">Original function pointer</param>
/// <returns>Status code</returns>
NTSTATUS BBHookSSDT( IN ULONG index, IN PVOID newAddr, OUT PVOID *ppOldAddr )
{
    ASSERT( newAddr != NULL );
    if (newAddr == NULL)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS status = STATUS_SUCCESS;
    PSYSTEM_SERVICE_DESCRIPTOR_TABLE pSSDT = GetSSDTBase();
    if (!pSSDT)
        return STATUS_NOT_FOUND;

    if (ppOldAddr)
        *ppOldAddr = (PUCHAR)pSSDT->ServiceTableBase + (((PLONG)pSSDT->ServiceTableBase)[index] >> 4);

    ULONG_PTR offset = (ULONG_PTR)newAddr - (ULONG_PTR)pSSDT->ServiceTableBase;

    // Intermediate jump is required
    if (offset > 0x07FFFFFF)
    {
        // Allocate trampoline, if required
        PVOID pTrampoline = BBFindTrampolineSpace( pSSDT->ServiceTableBase );
        if (!pTrampoline)
            return STATUS_NOT_FOUND;

        // Write jmp
        BBWriteTrampoline( pTrampoline, newAddr );
        offset = ((((ULONG_PTR)pTrampoline - (ULONG_PTR)pSSDT->ServiceTableBase) << 4) & 0xFFFFFFF0) | (pSSDT->ParamTableBase[index] >> 2);
    }
    // Direct jump
    else
        offset = ((offset << 4) & 0xFFFFFFF0) | (pSSDT->ParamTableBase[index] >> 2);

    // Update pointer
    ULONGLONG cr0 = __readcr0();
    __writecr0( cr0 & 0xFFFEFFFF );
    InterlockedExchange( (PLONG)pSSDT->ServiceTableBase + index, (LONG)offset );
    __writecr0( cr0 );

    return status;
}

/// <summary>
/// Restore SSDT hook
/// </summary>
/// <param name="index">SSDT index to restore</param>
/// <param name="origAddr">Original function address</param>
/// <returns>Status code</returns>
NTSTATUS BBRestoreSSDT( IN ULONG index, IN PVOID origAddr )
{
    ASSERT( origAddr != NULL );
    if (origAddr == NULL)
        return STATUS_INVALID_PARAMETER;

    PSYSTEM_SERVICE_DESCRIPTOR_TABLE pSSDT = GetSSDTBase();
    if (!pSSDT)
        return STATUS_NOT_FOUND;

    ULONG_PTR offset = (((ULONG_PTR)origAddr - (ULONG_PTR)pSSDT->ServiceTableBase) << 4) | (pSSDT->ParamTableBase[index] >> 2);
    ULONGLONG cr0 = __readcr0();
    __writecr0( cr0 & 0xFFFEFFFF );
    InterlockedExchange( (PLONG)pSSDT->ServiceTableBase + index, (LONG)offset );
    __writecr0( cr0 );

    return STATUS_SUCCESS;
}


NTSTATUS BBHookInline( IN PVOID origAddr, IN PVOID newAddr )
{
    UNREFERENCED_PARAMETER( origAddr );
    UNREFERENCED_PARAMETER( newAddr );

    NOPPROCINFO info;
    InitializeStopProcessors( &info );
    StopProcessors( &info );
    ULONGLONG cr0 = __readcr0();
    __writecr0( cr0 & 0xFFFEFFFF );

    __writecr0( cr0 );
    StartProcessors( &info );
    return STATUS_SUCCESS;
}

/// <summary>
/// Find memory allocation process entry
/// </summary>
/// <param name="pid">Target PID</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_PROCESS_ENTRY BBLookupPhysProcessEntry( IN HANDLE pid )
{
    for (PLIST_ENTRY pListEntry = g_PhysProcesses.Flink; pListEntry != &g_PhysProcesses; pListEntry = pListEntry->Flink)
    {
        PMEM_PHYS_PROCESS_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MEM_PHYS_PROCESS_ENTRY, link );
        if (pEntry->pid == pid)
            return pEntry;
    }

    return NULL;
}


/// <summary>
/// Find allocated memory region entry
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="pBase">Region base</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_ENTRY BBLookupPhysMemEntry( IN PLIST_ENTRY pList, IN PVOID pBase )
{
    ASSERT( pList != NULL );
    if (pList == NULL)
        return NULL;

    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
    {
        PMEM_PHYS_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MEM_PHYS_ENTRY, link );
        if (pBase >= pEntry->pMapped && pBase < (PVOID)((ULONG_PTR)pEntry->pMapped + pEntry->size))
            return pEntry;
    }

    return NULL;
}

//
// Cleanup routines
//

void BBCleanupPhysMemEntry( IN PMEM_PHYS_ENTRY pEntry, BOOLEAN attached )
{
    ASSERT( pEntry != NULL );
    if (pEntry == NULL)
        return;

    if (attached)
        MmUnmapLockedPages( pEntry->pMapped, pEntry->pMDL );

    IoFreeMdl( pEntry->pMDL );
    ExFreePoolWithTag( pEntry->ptr, BB_POOL_TAG );

    RemoveEntryList( &pEntry->link );
    ExFreePoolWithTag( pEntry, BB_POOL_TAG );
}

void BBCleanupProcessPhysEntry( IN PMEM_PHYS_PROCESS_ENTRY pEntry, BOOLEAN attached )
{
    ASSERT( pEntry != NULL );
    if (pEntry == NULL)
        return;

    while (!IsListEmpty( &pEntry->pVadList ))
        BBCleanupPhysMemEntry( (PMEM_PHYS_ENTRY)pEntry->pVadList.Flink, attached );

    RemoveEntryList( &pEntry->link );
    ExFreePoolWithTag( pEntry, BB_POOL_TAG );
}

void BBCleanupProcessPhysList()
{
    while (!IsListEmpty( &g_PhysProcesses ))
        BBCleanupProcessPhysEntry( (PMEM_PHYS_PROCESS_ENTRY)g_PhysProcesses.Flink, FALSE );
}