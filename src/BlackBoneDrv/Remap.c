#include "Remap.h"
#include "Utils.h"
#include <Ntstrsafe.h>

RTL_AVL_TABLE g_ProcessPageTables;      // Mapping table
KGUARDED_MUTEX g_globalLock;            // ProcessPageTables mutex

/// <summary>
/// Walk region list and create MDL for each region
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="pList">Region list</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareMDLList( IN PEPROCESS pProcess, IN PLIST_ENTRY pList );

/// <summary>
/// Build MDL for memory region
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="pEntry">Region data</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareMDLListEntry( IN PEPROCESS pProcess, IN PMAP_ENTRY pEntry );

/// <summary>
/// Map locked physical pages into caller process
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="noWarning">If set to FALSE, warning will be printed if region is already mapped</param>
/// <returns>Status code</returns>
NTSTATUS BBMapRegionListIntoCurrentProcess( IN PLIST_ENTRY pList, IN BOOLEAN noWarning );

/// <summary>
/// Map locked physical pages into caller process
/// </summary>
/// <param name="pEntry">Memory region</param>
/// <param name="pPrevEntry">Last mapped region</param>
/// <returns>Status code</returns>
NTSTATUS BBMapRegionIntoCurrentProcess( IN PMAP_ENTRY pEntry, IN PMAP_ENTRY pPrevEntry );


/// <summary>
/// Allocate kernel page from NonPaged pool and build MDL for it
/// </summary>
/// <param name="pPage">Resulting address</param>
/// <param name="pResultMDL">Resulting MDL</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateSharedPage( OUT PVOID* pPage, OUT PMDL* pResultMDL );

/// <summary>
/// Map kernel page into current process address space
/// </summary>
/// <param name="pMDL">Page MDL</param>
/// <param name="pResult">Mapped address</param>
/// <returns>Status code</returns>
NTSTATUS BBMapSharedPage( IN PMDL pMDL, OUT PVOID* pResult );

/// <summary>
/// Consolidate adjacent memory regions
/// </summary>
/// <param name="pList">Region list</param>
/// <returns>Status code</returns>
NTSTATUS BBConsolidateRegionList( IN PLIST_ENTRY pList );

/// <summary>
/// Process section object pages
/// Function will attempt to trigger copy-on-write for underlying pages to convert them into private
/// If copy-on-write fails, region will be then mapped as read-only
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="pEntry">Region data</param>
/// <returns>Status code</returns>
NTSTATUS BBHandleSharedRegion( IN PEPROCESS pProcess, IN PMAP_ENTRY pEntry );

/// <summary>
/// Unmap memory region, release corresponding MDL, and remove region form list
/// </summary>
/// <param name="pPageEntry">Region data</param>
/// <param name="pFoundEntry">Process data</param>
/// <returns>Status code</returns>
NTSTATUS BBUnmapRegionEntry( IN PMAP_ENTRY pPageEntry, IN PPROCESS_MAP_ENTRY pFoundEntry );

//
// Lookup routines
//

/// <summary>
/// Find memory region containing at least one byte from specific region
/// </summary>
/// <param name="pList">Region list to search in</param>
/// <param name="baseAddress">Base of region to find</param>
/// <param name="size">Size of the region to find</param>
/// <returns>Status code</returns>
PMAP_ENTRY BBFindPageEntry( IN PLIST_ENTRY pList, IN ULONG_PTR baseAddress, IN ULONG_PTR size );

//
// Cleanup routines
//

/// <summary>
/// Unmap pages, destroy MDLs, remove entry from list
/// </summary>
/// <param name="attached">TRUE indicates that function is executed 
/// in the context of host process and pages can be safely unmapped</param>
/// <param name="pList">Region list</param>
VOID BBCleanupPageList( IN BOOLEAN attached, IN PLIST_ENTRY pList );

/// <summary>
/// Close handle if process isn't in signaled state
/// </summary>
/// <param name="pProcess">Process object</param>
/// <param name="handle">Handle</param>
/// <param name="mode">KernelMode or UserMode handle</param>
/// <returns>Status code</returns>
NTSTATUS BBSafeHandleClose( IN PEPROCESS pProcess, IN HANDLE handle, IN KPROCESSOR_MODE mode );

#pragma alloc_text(PAGE, BBBuildProcessRegionListForRange)
#pragma alloc_text(PAGE, BBPrepareMDLList)
#pragma alloc_text(PAGE, BBPrepareMDLListEntry)
#pragma alloc_text(PAGE, BBMapRegionListIntoCurrentProcess)
#pragma alloc_text(PAGE, BBMapRegionIntoCurrentProcess)

#pragma alloc_text(PAGE, BBAllocateSharedPage)
#pragma alloc_text(PAGE, BBMapSharedPage)
#pragma alloc_text(PAGE, BBConsolidateRegionList)
#pragma alloc_text(PAGE, BBHandleSharedRegion)
#pragma alloc_text(PAGE, BBUnmapRegionEntry)

#pragma alloc_text(PAGE, BBMapMemory)
#pragma alloc_text(PAGE, BBMapMemoryRegion)
#pragma alloc_text(PAGE, BBUnmapMemory)
#pragma alloc_text(PAGE, BBUnmapMemoryRegion)

#pragma alloc_text(PAGE, BBGetRequiredRemapOutputSize)

#pragma alloc_text(PAGE, BBLookupProcessEntry)
#pragma alloc_text(PAGE, BBFindPageEntry)

#pragma alloc_text(PAGE, BBCleanupProcessTable)
#pragma alloc_text(PAGE, BBCleanupProcessEntry)
#pragma alloc_text(PAGE, BBCleanupHostProcess)
#pragma alloc_text(PAGE, BBCleanupPageList)
#pragma alloc_text(PAGE, BBSafeHandleClose)

#pragma alloc_text(PAGE, AvlCompare)
#pragma alloc_text(PAGE, AvlAllocate)
#pragma alloc_text(PAGE, AvlFree)



/*
*/
RTL_GENERIC_COMPARE_RESULTS AvlCompare( IN RTL_AVL_TABLE *Table, IN PVOID FirstStruct, IN PVOID SecondStruct )
{
    UNREFERENCED_PARAMETER( Table );
    PPROCESS_MAP_ENTRY pFirstEntry = (PPROCESS_MAP_ENTRY)FirstStruct;
    PPROCESS_MAP_ENTRY pSecondEntry = (PPROCESS_MAP_ENTRY)SecondStruct;

    // Find by target
    if (pFirstEntry->target.pid != 0 && pSecondEntry->target.pid != 0)
    {
        if (pFirstEntry->target.pid == pSecondEntry->target.pid)
            return GenericEqual;

        if (pSecondEntry->target.pid > pFirstEntry->target.pid)
            return GenericGreaterThan;

        return GenericLessThan;
    }
    // Find by Host
    else
    {
        if (pFirstEntry->host.pid == pSecondEntry->host.pid)
            return GenericEqual;

        if (pSecondEntry->host.pid > pFirstEntry->host.pid)
            return GenericGreaterThan;

        return GenericLessThan;
    }
}

/*
*/
PVOID AvlAllocate( IN RTL_AVL_TABLE *Table, IN CLONG ByteSize )
{
    UNREFERENCED_PARAMETER( Table );
    return ExAllocatePoolWithTag( PagedPool, ByteSize, BB_POOL_TAG );
}

/*
*/
VOID AvlFree( IN RTL_AVL_TABLE *Table, IN PVOID Buffer )
{
    UNREFERENCED_PARAMETER( Table );
    ExFreePoolWithTag( Buffer, BB_POOL_TAG );
}

/// <summary>
/// Enumerate committed, accessible, non-guarded memory regions
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="start">Region start</param>
/// <param name="end">Region end</param>
/// <param name="mapSections">If set to FALSE, section objects will be excluded from list</param>
/// <returns>Status code</returns>
NTSTATUS BBBuildProcessRegionListForRange( IN PLIST_ENTRY pList, IN ULONG_PTR start, IN ULONG_PTR end, IN BOOLEAN mapSections )
{
    NTSTATUS status = STATUS_SUCCESS;
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    SIZE_T length = 0;
    ULONG_PTR memptr = 0;
    PMAP_ENTRY pEntry = NULL;

    for (memptr = start; memptr < end; memptr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize)
    {
        status = ZwQueryVirtualMemory( ZwCurrentProcess(), (PVOID)memptr, MemoryBasicInformationEx, &mbi, sizeof( mbi ), &length );

        if (!NT_SUCCESS( status ))
        {
            DPRINT( "BlackBone: %s: ZwQueryVirtualMemory for address 0x%p returned status 0x%X\n", __FUNCTION__, memptr, status );

            // STATUS_INVALID_PARAMETER is a normal status for last secured VAD under Win7
            return status == STATUS_INVALID_PARAMETER ? STATUS_SUCCESS : status;
        }
        // Skip non-committed, no-access and guard pages
        else if (mbi.State == MEM_COMMIT &&  mbi.Protect != PAGE_NOACCESS && !(mbi.Protect & PAGE_GUARD))
        {
            // Ignore shared memory if required
            if (mbi.Type != MEM_PRIVATE && !mapSections)
                continue;

            pEntry = ExAllocatePoolWithTag( PagedPool, sizeof( MAP_ENTRY ), BB_POOL_TAG );
            if (pEntry == NULL)
            {
                DPRINT( "BlackBone: %s: Failed to allocate memory for Remap descriptor\n", __FUNCTION__ );
                BBCleanupPageList( FALSE, pList );
                return STATUS_NO_MEMORY;
            }

            pEntry->mem = mbi;
            pEntry->newPtr = 0;
            pEntry->pMdl = NULL;
            pEntry->locked = FALSE;
            pEntry->shared = mbi.Type != MEM_PRIVATE;

            InsertTailList( pList, &pEntry->link );
        }
    }

    //if (NT_SUCCESS( status ))
        //status = BBConsolidateRegionList( pList );

    return status;
}


/// <summary>
/// Consolidate adjacent memory regions
/// </summary>
/// <param name="pList">Region list</param>
/// <returns>Status code</returns>
NTSTATUS BBConsolidateRegionList( IN PLIST_ENTRY pList )
{
    NTSTATUS status = STATUS_SUCCESS;

    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; )
    {
        PMAP_ENTRY pNextEntry = NULL;
        PMAP_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MAP_ENTRY, link );

        // End of list
        if (pListEntry->Flink == pList)
            break;

        pNextEntry = CONTAINING_RECORD( pListEntry->Flink, MAP_ENTRY, link );

        // Consolidate only non-mapped entries. Both entries must have same sharing flags
        if (pEntry->pMdl == NULL && pNextEntry->pMdl == NULL && pEntry->shared == pNextEntry->shared)
        {
            // Regions are adjacent
            if ((PUCHAR)pEntry->mem.BaseAddress + pEntry->mem.RegionSize == pNextEntry->mem.BaseAddress)
            {
                pEntry->mem.RegionSize += pNextEntry->mem.RegionSize;
                RemoveEntryList( &pNextEntry->link );
            }
            else
                pListEntry = pListEntry->Flink;
        }  
        else
            pListEntry = pListEntry->Flink;
    }

    return status;
}



/// <summary>
/// Unmap memory region, release corresponding MDL, and remove region form list
/// </summary>
/// <param name="pPageEntry">Region data</param>
/// <param name="pFoundEntry">Process data</param>
/// <returns>Status code</returns>
NTSTATUS BBUnmapRegionEntry( IN PMAP_ENTRY pPageEntry, IN PPROCESS_MAP_ENTRY pFoundEntry )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( pFoundEntry );

    // MDL is valid
    if (pPageEntry->pMdl)
    {
        // If MDL is mapped
        if (pPageEntry->newPtr)
        {
            DPRINT( "BlackBone: %s: Unmapping region at 0x%p from process %u\n", __FUNCTION__, pPageEntry->newPtr, pFoundEntry->target.pid );
            MmUnmapLockedPages( (PVOID)pPageEntry->newPtr, pPageEntry->pMdl );
            pPageEntry->newPtr = 0;
        }

        if (pPageEntry->locked)
            MmUnlockPages( pPageEntry->pMdl );

        IoFreeMdl( pPageEntry->pMdl );
    }

    RemoveEntryList( &pPageEntry->link );
    ExFreePoolWithTag( pPageEntry, BB_POOL_TAG );

    return status;
}

/// <summary>
/// Process section object pages
/// Function will attempt to trigger copy-on-write for underlying pages to convert them into private
/// If copy-on-write fails, region will be then mapped as read-only
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="pEntry">Region data</param>
/// <returns>Status code</returns>
NTSTATUS BBHandleSharedRegion( IN PEPROCESS pProcess, IN PMAP_ENTRY pEntry )
{
    NTSTATUS status = STATUS_SUCCESS;

    MEMORY_BASIC_INFORMATION mbi = { 0 };
    SIZE_T length = 0;
    ULONG_PTR memptr = 0;

    // Iterate underlying memory regions
    for (memptr = (ULONG_PTR)pEntry->mem.BaseAddress; 
         memptr < (ULONG_PTR)pEntry->mem.BaseAddress + pEntry->mem.RegionSize;
         memptr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize)
    {
        PVOID pBase = NULL;
        SIZE_T size = 0;
        MEMORY_WORKING_SET_EX_INFORMATION wsInfo = { 0 };
        ULONG oldProt = 0;
        BOOLEAN writable = FALSE;

        status = ZwQueryVirtualMemory( ZwCurrentProcess(), (PVOID)memptr, MemoryBasicInformation, &mbi, sizeof( mbi ), &length );
        if (!NT_SUCCESS( status ))
        {
            DPRINT( "BlackBone: %s: ZwQueryVirtualMemory for address 0x%p failed, status 0x%X\n", __FUNCTION__, memptr, status );
            return status;
        }

        // Check if region has SEC_NO_CHANGE attribute
        PMMVAD_SHORT pVad = { 0 };
        if (NT_SUCCESS( BBFindVAD( pProcess, (ULONG_PTR)mbi.BaseAddress, &pVad ) ) && pVad != NULL)
        {
            // Can't change region protection
            if (pVad->u.VadFlags.NoChange)
            {
                return STATUS_SHARING_VIOLATION;
            }
        }

        writable = (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
        pBase = mbi.BaseAddress;
        size = (SIZE_T)mbi.RegionSize;

        // Make readonly pages writable
        if (writable || NT_SUCCESS( status = ZwProtectVirtualMemory( ZwCurrentProcess(), &pBase, &size, PAGE_EXECUTE_READWRITE, &oldProt ) ))
        {
            BOOLEAN failed = FALSE;

            // Touch pages to trigger copy-on-write and create private copies
            // This is crucial to prevent changes on section pages
            __try
            {
                ProbeForWrite( mbi.BaseAddress, mbi.RegionSize, PAGE_SIZE );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT( "BlackBone: %s: Exception while touching address 0x%p\n", __FUNCTION__, mbi.BaseAddress );
                failed = TRUE;
            }

            // Restore protection
            if (writable == FALSE)
                ZwProtectVirtualMemory( ZwCurrentProcess(), &pBase, &size, oldProt, &oldProt );

            if (failed)
                return STATUS_SHARING_VIOLATION;

            // If region was writable, it's safe to map shared pages
            if (writable)
                continue;

            //
            // Ensure pages were made private
            //
            for (ULONG_PTR pPage = (ULONG_PTR)mbi.BaseAddress; pPage < (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize; pPage += PAGE_SIZE)
            {
                RtlZeroMemory( &wsInfo, sizeof( wsInfo ) );

                wsInfo.VirtualAddress = (PVOID)pPage;

                // Check page 'shared' flag
                if (NT_SUCCESS( ZwQueryVirtualMemory( ZwCurrentProcess(), NULL, MemoryWorkingSetExInformation, &wsInfo, sizeof( wsInfo ), &length ) ) &&
                     wsInfo.VirtualAttributes.Shared)
                {
                    DPRINT( "BlackBone: %s: Page at address 0x%p is still shared!", __FUNCTION__, pPage );
                    return STATUS_SHARING_VIOLATION;
                }
            }
        }
        else
        {
            if(status != STATUS_SECTION_PROTECTION)
                DPRINT( "BlackBone: %s: Failed to alter protection of region at 0x%p, status 0x%X\n", __FUNCTION__, mbi.BaseAddress, status );

            return status;
        }
    }

    return status;
}


/// <summary>
/// Build MDL for memory region
/// </summary>
/// <param name="pEntry">Region data</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareMDLListEntry( IN PEPROCESS pProcess, IN OUT PMAP_ENTRY pEntry )
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT( pEntry != NULL );
    if (pEntry == NULL)
        return STATUS_INVALID_PARAMETER;

    // Handle shared pages
    if (pEntry->shared != FALSE && !NT_SUCCESS( BBHandleSharedRegion( pProcess, pEntry ) ))
        pEntry->readonly = TRUE;
    else
        pEntry->readonly = FALSE;

    pEntry->pMdl = IoAllocateMdl( pEntry->mem.BaseAddress, (ULONG)pEntry->mem.RegionSize, FALSE, FALSE, NULL );

    if (pEntry->pMdl == NULL)
    {
        DPRINT( "BlackBone: %s: Failed to allocate MDL for address 0x%p\n", __FUNCTION__, pEntry->mem.BaseAddress );
        return STATUS_NO_MEMORY;
    }

    __try
    {
        MmProbeAndLockPages( pEntry->pMdl, UserMode, IoReadAccess );
        pEntry->locked = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception in MmProbeAndLockPages. Base = 0x%p\n", __FUNCTION__, pEntry->mem.BaseAddress );
        IoFreeMdl( pEntry->pMdl );
        pEntry->pMdl = NULL;
    }

    return status;
}


/// <summary>
/// Walk region list and create MDL for each region
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="pProcess">Target process</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareMDLList( IN PEPROCESS pProcess, IN PLIST_ENTRY pList )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMAP_ENTRY pEntry = NULL;

    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
    {
        pEntry = CONTAINING_RECORD( pListEntry, MAP_ENTRY, link );
        if (pEntry->pMdl == NULL)
            BBPrepareMDLListEntry( pProcess, pEntry );
    }

    return status;
}


/// <summary>
/// Map locked physical pages into caller process
/// </summary>
/// <param name="pEntry">Memory region</param>
/// <param name="pPrevEntry">Last mapped region</param>
/// <returns>Status code</returns>
NTSTATUS BBMapRegionIntoCurrentProcess( IN PMAP_ENTRY pEntry, IN PMAP_ENTRY pPrevEntry )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER( pPrevEntry );

    ASSERT( pEntry != NULL );
    if (pEntry == NULL)
        return STATUS_INVALID_PARAMETER;

    // Try to map at original address
    __try {
        pEntry->newPtr = (ULONG_PTR)MmMapLockedPagesSpecifyCache( 
            pEntry->pMdl, UserMode, MmCached, pEntry->mem.BaseAddress, FALSE, NormalPagePriority 
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { }

    // Try to map immediately after previous entry
    if (pEntry->newPtr == 0 && pPrevEntry != NULL && pPrevEntry->newPtr != 0)
    {
        __try {
            pEntry->newPtr = (ULONG_PTR)MmMapLockedPagesSpecifyCache( 
                pEntry->pMdl, UserMode, MmCached, (PVOID)(pPrevEntry->newPtr + pPrevEntry->mem.RegionSize), FALSE, NormalPagePriority
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    // Map at arbitrary available location
    if (pEntry->newPtr == 0)
    {
        __try {
            pEntry->newPtr = (ULONG_PTR)MmMapLockedPagesSpecifyCache( pEntry->pMdl, UserMode, MmCached, NULL, FALSE, NormalPagePriority ); 
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    if (pEntry->newPtr != 0)
    {
        //if (pEntry->newPtr != pEntry->originalPtr)
            //DPRINT( "BlackBone: %s: Remapped 0x%p --> 0x%p\n", __FUNCTION__, pEntry->originalPtr, pEntry->newPtr );

        // Update region protection only for the sake of NtQueryVirtualMemory
        // It has no other effect because real protection is determined by underlying PTEs
        BBProtectVAD( PsGetCurrentProcess(), pEntry->newPtr, pEntry->readonly ? MM_READONLY : MM_EXECUTE_READWRITE );

        // ZwProtectVirtualMemory can't update protection of pages that represent Physical View
        // So PTEs are updated directly
        for (ULONG_PTR pAdress = pEntry->newPtr; pAdress < pEntry->newPtr + pEntry->mem.RegionSize; pAdress += PAGE_SIZE)
        {
            PMMPTE pPTE = GetPTEForVA( (PVOID)pAdress );

            if (pEntry->readonly)
                pPTE->u.Hard.Dirty1 = pPTE->u.Hard.Write = 0;
            else
                pPTE->u.Hard.NoExecute = 0;
        }
    }
    else
    {
        DPRINT( "BlackBone: %s: Failed to map region at adress 0x%p\n", __FUNCTION__, pEntry->mem.BaseAddress );
        status = STATUS_NONE_MAPPED;
    }

    return status;
}


/// <summary>
/// Map memory regions into caller process
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="noWarning">If set to FALSE, warning will be printed if region is already mapped</param>
/// <returns>Status code</returns>
NTSTATUS BBMapRegionListIntoCurrentProcess( IN PLIST_ENTRY pList, IN BOOLEAN noWarning )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMAP_ENTRY pEntry = NULL, pPrevEntry = NULL;

    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
    {
        pEntry = CONTAINING_RECORD( pListEntry, MAP_ENTRY, link );

        // Skip already mapped entries
        if (pEntry->newPtr != 0)
        {
            if (noWarning == FALSE)
                DPRINT( "BlackBone: %s: Warning! Region 0x%p already mapped to 0x%p\n", __FUNCTION__, pEntry->mem.BaseAddress, pEntry->newPtr );

            continue;
        }

        // Non-locked pages can't be mapped
        if (pEntry->pMdl && pEntry->locked)
            status |= BBMapRegionIntoCurrentProcess( pEntry, pPrevEntry );
        else
            DPRINT( "BlackBone: %s: No valid MDL for address 0x%p. Either not allocated or not locked\n", __FUNCTION__, pEntry->mem.BaseAddress );

        pPrevEntry = pEntry;
    }

    return status;
}


/// <summary>
/// Allocate kernel page from NonPaged pool and build MDL for it
/// </summary>
/// <param name="pPage">Resulting address</param>
/// <param name="pResultMDL">Resulting MDL</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateSharedPage( OUT PVOID* pPage, OUT PMDL* pResultMDL )
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT( pPage != NULL && pResultMDL != NULL );
    if (pPage == NULL || pResultMDL == NULL)
        return STATUS_INVALID_PARAMETER;

    *pPage = ExAllocatePoolWithTag( NonPagedPool, PAGE_SIZE, BB_POOL_TAG );
    if (*pPage)
    {
        // Cleanup buffer before mapping it into UserMode to prevent exposure of kernel data
        RtlZeroMemory( *pPage, PAGE_SIZE );

        *pResultMDL = IoAllocateMdl( *pPage, PAGE_SIZE, FALSE, FALSE, NULL );
        if (*pResultMDL)
            MmBuildMdlForNonPagedPool( *pResultMDL );
    }
    else
    {
        DPRINT( "BlackBone: %s: Failed to allocate kernel page\n", __FUNCTION__ );
        status = STATUS_MEMORY_NOT_ALLOCATED;
    }

    return status;
}


/// <summary>
/// Map kernel page into current process address space
/// </summary>
/// <param name="pMDL">Page MDL</param>
/// <param name="pResult">Mapped address</param>
/// <returns>Status code</returns>
NTSTATUS BBMapSharedPage( IN PMDL pMDL, OUT PVOID* pResult )
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT( pMDL != NULL && pResult != NULL );
    if (pMDL == NULL || pResult == NULL)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        // Map into user space
        *pResult = MmMapLockedPagesSpecifyCache( pMDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority );
        if (*pResult)
        {
            //DPRINT( "BlackBone: %s: Shared page at 0x%p\n", __FUNCTION__, *pResult );

            // Make executable
            BBProtectVAD( PsGetCurrentProcess(), (ULONG_PTR)*pResult, MM_EXECUTE_READWRITE );
            GetPTEForVA( *pResult )->u.Hard.NoExecute = 0;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (*pResult == NULL)
    {
        DPRINT( "BlackBone: %s: Failed to map kernel page\n", __FUNCTION__ );
        status = STATUS_NOT_MAPPED_DATA;
    }
    

    return status;

}


/// <summary>
/// Map entire address space of target process into current
/// Subsequent calls won't map any additional regions, only return info about existing mapping
/// </summary>
/// <param name="pRemap">Mapping params</param>
/// <param name="ppEntry">Mapped context</param>
/// <returns>Status code</returns>
NTSTATUS BBMapMemory( IN PMAP_MEMORY pRemap, OUT PPROCESS_MAP_ENTRY* ppEntry )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;
    PROCESS_MAP_ENTRY processEntry = { 0 };
    PPROCESS_MAP_ENTRY pFoundEntry = NULL;
    BOOLEAN newEntry = FALSE;

    // Sanity checks
    // Can't remap self
    if ((HANDLE)pRemap->pid == PsGetCurrentProcessId())
        return STATUS_INVALID_PARAMETER;

    processEntry.target.pid = (HANDLE)pRemap->pid;

    KeAcquireGuardedMutex( &g_globalLock );

    status = PsLookupProcessByProcessId( processEntry.target.pid, &pProcess );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to find process with PID %u\n", __FUNCTION__, processEntry.target.pid );

        KeReleaseGuardedMutex( &g_globalLock );
        return status;
    }

    // Process in signaled state, abort any operations
    if (BBCheckProcessTermination( pProcess ))
    {
        DPRINT( "BlackBone: %s: Process %u is terminating. Abort\n", __FUNCTION__, processEntry.target.pid );

        ObDereferenceObject( pProcess );
        KeReleaseGuardedMutex( &g_globalLock );
        return STATUS_PROCESS_IS_TERMINATING;
    }

    pFoundEntry = (PPROCESS_MAP_ENTRY)RtlInsertElementGenericTableAvl( &g_ProcessPageTables, &processEntry, sizeof( processEntry ), &newEntry );
    if (newEntry != FALSE)
    {
        InitializeListHead( &pFoundEntry->pageList );

        // Allocate shared Page
        status = BBAllocateSharedPage( &pFoundEntry->pSharedPage, &pFoundEntry->pMDLShared );

        if (NT_SUCCESS( status ))
        {
            KAPC_STATE apc;
            IO_STATUS_BLOCK ioStatusBlock = { 0 };

            KeStackAttachProcess( pProcess, &apc );
            status = BBBuildProcessRegionListForRange( 
                &pFoundEntry->pageList, (ULONG_PTR)MM_LOWEST_USER_ADDRESS, 
                (ULONG_PTR)MM_HIGHEST_USER_ADDRESS, pRemap->mapSections 
                );

            if (NT_SUCCESS( status ))
                status = BBPrepareMDLList( pProcess, &pFoundEntry->pageList );

            // Map shared page into target process
            if (NT_SUCCESS( status ))
                status = BBMapSharedPage( pFoundEntry->pMDLShared, &pFoundEntry->target.sharedPage );

            // Open pipe endpoint
            if (NT_SUCCESS( status ) && pRemap->pipeName[0] != L'\0')
            {
                WCHAR wbuf[64] = { 0 };
                UNICODE_STRING pipeName = { 0 };
                pipeName.Buffer = wbuf;
                pipeName.MaximumLength = sizeof( wbuf );

                NTSTATUS pipeStatus = RtlUnicodeStringPrintf( &pipeName, L"\\??\\pipe\\%ls", pRemap->pipeName );
                if (NT_SUCCESS( pipeStatus ))
                {
                    OBJECT_ATTRIBUTES attr = { 0 };
                    InitializeObjectAttributes( &attr, &pipeName, OBJ_CASE_INSENSITIVE, NULL, NULL );
                    pipeStatus = ZwOpenFile( &pFoundEntry->targetPipe, GENERIC_WRITE, &attr, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE, 0 );
                }

                if (!NT_SUCCESS( pipeStatus ))
                    DPRINT( "BlackBone: %s: Remap pipe failed with status 0x%X\n", __FUNCTION__, pipeStatus );
            }

            KeUnstackDetachProcess( &apc );
        }
    }
    else
    {
        DPRINT( "BlackBone: %s: Found mapping entry for process %u\n", __FUNCTION__, pFoundEntry->target.pid );

        // Caller is not Host and Host still exists
        // Do not allow remapping into new process
        if (pFoundEntry->host.pid != PsGetCurrentProcessId() && pFoundEntry->host.pid != NULL)
        {
            DPRINT( "BlackBone: %s: Host process %u still exists. Cannot map into new process\n", __FUNCTION__, pFoundEntry->host.pid );
            status = STATUS_ADDRESS_ALREADY_EXISTS;
        }
    }

    // Update host process
    if (NT_SUCCESS( status ) && (newEntry || pFoundEntry->host.pid == NULL))
    {
        // Map memory
        BBMapRegionListIntoCurrentProcess( &pFoundEntry->pageList, FALSE );

        // Map shared page into Host process
        BBMapSharedPage( pFoundEntry->pMDLShared, &pFoundEntry->host.sharedPage );

        pFoundEntry->host.pid = PsGetCurrentProcessId();
    }

    if (ppEntry)
        *ppEntry = pFoundEntry;

    if (pProcess)
        ObDereferenceObject( pProcess );

    // Remove entry if something went wrong
    if (!NT_SUCCESS( status ) && status != STATUS_ADDRESS_ALREADY_EXISTS)
        BBCleanupProcessEntry(pFoundEntry);

    KeReleaseGuardedMutex( &g_globalLock );

    return status;
}

/// <summary>
/// Map specific memory region
/// </summary>
/// <param name="pRegion">Region data</param>
/// <param name="pResult">Mapping results</param>
/// <returns>Status code</returns>
NTSTATUS BBMapMemoryRegion( IN PMAP_MEMORY_REGION pRegion, OUT PMAP_MEMORY_REGION_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pRegion->base, pRegion->size );
    PROCESS_MAP_ENTRY processEntry = { 0 };
    PPROCESS_MAP_ENTRY pFoundEntry = NULL;
    PEPROCESS pProcess = NULL;
    BOOLEAN newEntry = FALSE;
    BOOLEAN alreadyExists = FALSE;
    ULONG_PTR removedBase = 0;
    ULONG removedSize = 0;

    // Don't allow remapping of kernel addresses
    if (pRegion->base >= (ULONGLONG)MM_HIGHEST_USER_ADDRESS || pRegion->base + pRegion->size > (ULONGLONG)MM_HIGHEST_USER_ADDRESS)
    {
        DPRINT( "BlackBone: %s: Invalid address range: 0x%p - 0x%p\n", __FUNCTION__, pRegion->base, pRegion->base + pRegion->size );
        return STATUS_INVALID_ADDRESS;
    }

    KeAcquireGuardedMutex( &g_globalLock );

    // Align on page boundaries
    pRegion->base = (ULONGLONG)PAGE_ALIGN( pRegion->base );
    pRegion->size = pageCount << PAGE_SHIFT;

    processEntry.target.pid = (HANDLE)pRegion->pid;
    processEntry.host.pid = PsGetCurrentProcessId();

    status = PsLookupProcessByProcessId( processEntry.target.pid, &pProcess );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to find process 0x%u\n", __FUNCTION__, processEntry.target.pid );
        KeReleaseGuardedMutex( &g_globalLock );
        return status;
    }

    // Process in signaled state, abort any operations
    if (BBCheckProcessTermination( pProcess ))
    {
        DPRINT( "BlackBone: %s: Process %u is terminating. Abort\n", __FUNCTION__, processEntry.target.pid );

        ObDereferenceObject( pProcess );
        KeReleaseGuardedMutex( &g_globalLock );
        return STATUS_PROCESS_IS_TERMINATING;
    }

    // Store or retrieve context
    pFoundEntry = (PPROCESS_MAP_ENTRY)RtlInsertElementGenericTableAvl( &g_ProcessPageTables, &processEntry, sizeof( processEntry ), &newEntry );
    if (newEntry != FALSE)
    {
        DPRINT( "BlackBone: %s: Process entry 0x%u was not found, creating new\n", __FUNCTION__, processEntry.target.pid );
        InitializeListHead( &pFoundEntry->pageList );

        // Allocate shared Page
        status = BBAllocateSharedPage( &pFoundEntry->pSharedPage, &pFoundEntry->pMDLShared );
    }
    else
    {
        PMAP_ENTRY pPageEntry = NULL;

        // Does not allow call from non-original process
        if (pFoundEntry->host.pid != PsGetCurrentProcessId() && pFoundEntry->host.pid != NULL)
        {
            DPRINT( "BlackBone: %s: Call from non-original process not supported\n", __FUNCTION__ );
            status = STATUS_INVALID_PARAMETER;
        }

        // Conflicting region
        // Target region isn't considered conflicting if it completely fits into existing
        // TODO: Find source of the problem
        pPageEntry = BBFindPageEntry( &pFoundEntry->pageList, pRegion->base, pRegion->size );
        if (pPageEntry)
        {
            if (pRegion->base < (ULONGLONG)pPageEntry->mem.BaseAddress ||
                pRegion->base + pRegion->size > (ULONGLONG)pPageEntry->mem.BaseAddress + pPageEntry->mem.RegionSize)
            {
                DPRINT(
                    "BlackBone: %s: Target region 0%p - 0x%p conficts with existing: 0x%p - 0x%p\n", __FUNCTION__,
                    pRegion->base, pRegion->base + pRegion->size,
                    pPageEntry->mem.BaseAddress, (ULONGLONG)pPageEntry->mem.BaseAddress + pPageEntry->mem.RegionSize
                    );

                // Unmap conflicting region
                removedBase = (ULONG_PTR)pPageEntry->mem.BaseAddress;
                removedSize = (ULONG)pPageEntry->mem.RegionSize;

                status = BBUnmapRegionEntry( pPageEntry, pFoundEntry );
            }
            else
                alreadyExists = TRUE;
        }
    }

    // Map region if required
    if (alreadyExists == FALSE && NT_SUCCESS( status ))
    {
        KAPC_STATE apc;

        // Allocate MDLs and lock pages
        KeStackAttachProcess( pProcess, &apc );
        status = BBBuildProcessRegionListForRange( &pFoundEntry->pageList, pRegion->base, pRegion->base + pRegion->size, TRUE );

        if (NT_SUCCESS( status ))
            status = BBPrepareMDLList( pProcess, &pFoundEntry->pageList );

        // Map shared page into target process
        if (NT_SUCCESS( status ) && newEntry)
            status = BBMapSharedPage( pFoundEntry->pMDLShared, &pFoundEntry->target.sharedPage );

        KeUnstackDetachProcess( &apc );

        // Map shared page into Host process
        if (NT_SUCCESS( status ) && newEntry)
            status = BBMapSharedPage( pFoundEntry->pMDLShared, &pFoundEntry->host.sharedPage );

        if (NT_SUCCESS( status ))
            status = BBMapRegionListIntoCurrentProcess( &pFoundEntry->pageList, TRUE );
    }

    if (pProcess)
        ObDereferenceObject( pProcess );

    // Fill result
    if (NT_SUCCESS( status ) && pResult)
    {
        // FIXME:
        // Target region could be divided into several smaller ones
        // This function will return only first found
        PMAP_ENTRY pEntry = BBFindPageEntry( &pFoundEntry->pageList, pRegion->base, pRegion->size );
        if (pEntry)
        {
            DPRINT( "BlackBone: %s: Updated 0x%p --> 0x%p\n", __FUNCTION__, pEntry->mem.BaseAddress, pEntry->newPtr );

            pResult->originalPtr = (ULONGLONG)pEntry->mem.BaseAddress;
            pResult->newPtr = pEntry->newPtr;
            pResult->removedPtr = removedBase;
            pResult->size = (ULONG)pEntry->mem.RegionSize;
            pResult->removedSize = removedSize;
        }
    }

    // Remove entry if something goes wrong
    if (!NT_SUCCESS( status ))
        BBCleanupProcessEntry( pFoundEntry );

    KeReleaseGuardedMutex( &g_globalLock );

    return status;
}

/// <summary>
/// Unmap any mapped memory from host and target processes
/// </summary>
/// <param name="pUnmap">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBUnmapMemory( IN PUNMAP_MEMORY pUnmap )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPROCESS_MAP_ENTRY pFoundEntry = NULL;

    KeAcquireGuardedMutex( &g_globalLock );

    pFoundEntry = BBLookupProcessEntry( (HANDLE)pUnmap->pid, FALSE );
    if (pFoundEntry == NULL)
    {
        DPRINT( "BlackBone: %s: Process entry with PID %u was not found\n", __FUNCTION__, pUnmap->pid );
        status = STATUS_NOT_FOUND;
    }
    else
    {
        DPRINT( "BlackBone: %s: BBUnmapMemory cleanup for PID %u\n", __FUNCTION__, pUnmap->pid );
        BBCleanupProcessEntry( pFoundEntry );
    }

    KeReleaseGuardedMutex( &g_globalLock );

    return status;
}

/// <summary>
/// Unmap specific memory region
/// </summary>
/// <param name="pRegion">Region info</param>
/// <returns>Status ode</returns>
NTSTATUS BBUnmapMemoryRegion( IN PUNMAP_MEMORY_REGION pRegion )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPROCESS_MAP_ENTRY pFoundEntry = NULL;
    PMAP_ENTRY pPageEntry = NULL;
    PEPROCESS pProcess = NULL;
    ULONG pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(pRegion->base, pRegion->size);

    // Sanity check
    if (pRegion->base >= (ULONGLONG)MM_HIGHEST_USER_ADDRESS || pRegion->base + pRegion->size > (ULONGLONG)MM_HIGHEST_USER_ADDRESS)
        return STATUS_INVALID_ADDRESS;

    KeAcquireGuardedMutex( &g_globalLock );

    status = PsLookupProcessByProcessId( (HANDLE)pRegion->pid, &pProcess );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to find process 0x%u\n", __FUNCTION__, pRegion->pid );
        KeReleaseGuardedMutex( &g_globalLock );
        return status;
    }

    // Process in signaled state, abort any operations
    if (BBCheckProcessTermination( pProcess ))
    {
        DPRINT( "BlackBone: %s: Process %u is terminating. Abort\n", __FUNCTION__, pRegion->pid );

        ObDereferenceObject( pProcess );
        KeReleaseGuardedMutex( &g_globalLock );
        return STATUS_PROCESS_IS_TERMINATING;
    }

    pFoundEntry = BBLookupProcessEntry( (HANDLE)pRegion->pid, FALSE );
    if (pFoundEntry == NULL)
    {
        DPRINT( "BlackBone: %s: Process entry for process %u was not found\n", __FUNCTION__, pRegion->pid );
        status = STATUS_NOT_FOUND;
    }
    else
    {
        // Does not allow call from non-original process
        if (pFoundEntry->host.pid != PsGetCurrentProcessId())
        {
            DPRINT( "BlackBone: %s: Call from non-original process not supported\n", __FUNCTION__ );
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            // Align on page boundaries
            pRegion->base = (ULONGLONG)PAGE_ALIGN( pRegion->base );
            pRegion->size = pageCount << PAGE_SHIFT;

            pPageEntry = BBFindPageEntry( &pFoundEntry->pageList, pRegion->base, pRegion->size );
            if (pPageEntry)
            {               
                status = BBUnmapRegionEntry( pPageEntry, pFoundEntry );
            }
            else
            {
                DPRINT( "BlackBone: %s: Map entry for address 0x%p was not found\n", __FUNCTION__, pRegion->base );
                status = STATUS_INVALID_ADDRESS;
            }
        }
    }

    if (pProcess)
        ObDereferenceObject( pProcess );

    KeReleaseGuardedMutex( &g_globalLock );

    return status;
}


/// <summary>
/// Unmap pages, destroy MDLs, remove entry from list
/// </summary>
/// <param name="attached">TRUE indicates that function is executed 
/// in the context of host process and pages can be safely unmapped</param>
/// <param name="pList">Region list</param>
VOID BBCleanupPageList( IN BOOLEAN attached, IN PLIST_ENTRY pList )
{
    // No work required
    if (pList == NULL || IsListEmpty( pList ))
        return;

    while (!IsListEmpty( pList ))
    {
        PMAP_ENTRY pEntry = CONTAINING_RECORD( pList->Flink, MAP_ENTRY, link );

        if (pEntry->pMdl)
        {
            // If MDL is mapped and we are in the context of corresponding process
            if (attached && pEntry->newPtr)
                MmUnmapLockedPages( (PVOID)pEntry->newPtr, pEntry->pMdl );

            if (pEntry->locked)
                MmUnlockPages( pEntry->pMdl );

            IoFreeMdl( pEntry->pMdl );
        }

        RemoveHeadList( pList );
        ExFreePoolWithTag( pEntry, BB_POOL_TAG );
    }
}


/// <summary>
/// Close handle if process isn't in signaled state
/// </summary>
/// <param name="pProcess">Process object</param>
/// <param name="handle">Handle</param>
/// <param name="mode">KernelMode or UserMode handle</param>
/// <returns>Status code</returns>
NTSTATUS BBSafeHandleClose( IN PEPROCESS pProcess, IN HANDLE handle, IN KPROCESSOR_MODE mode )
{
    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
        return STATUS_INVALID_PARAMETER;

    //
    // If process is in signaled state, ObjectTable is already NULL
    // Thus is will lead to crash in ObCloseHandle->ExpLookupHandleTableEntry
    //
    if (BBCheckProcessTermination( pProcess ))
        return STATUS_PROCESS_IS_TERMINATING;

    return ObCloseHandle( handle, mode );
}


/// <summary>
/// Unmap all regions, delete MDLs, close handles, remove entry from table
/// </summary>
/// <param name="pProcessEntry">Process entry</param>
VOID BBCleanupProcessEntry( IN PPROCESS_MAP_ENTRY pProcessEntry )
{
    KAPC_STATE apc;
    PEPROCESS pTargetProc = NULL, pHostProc = NULL;
    ATTACHED_CONTEXT context = ContextNone;

    ASSERT( pProcessEntry != NULL );
    if (pProcessEntry == NULL)
        return;

    if (PsGetCurrentProcessId() == pProcessEntry->host.pid)
        context = ContextHost;
    else if (PsGetCurrentProcessId() == pProcessEntry->target.pid)
        context = ContextTarget;

    // Unmap from Host
    if (context == ContextHost || NT_SUCCESS( PsLookupProcessByProcessId( pProcessEntry->host.pid, &pHostProc ) ))
    {
        if (context != ContextHost)
            KeStackAttachProcess( pHostProc, &apc );

        if (pProcessEntry->host.sharedPage)
            MmUnmapLockedPages( pProcessEntry->host.sharedPage, pProcessEntry->pMDLShared );

        BBCleanupPageList( TRUE, &pProcessEntry->pageList );

        if (context != ContextHost)
            KeUnstackDetachProcess( &apc );

        if (pHostProc)
            ObDereferenceObject( pHostProc );
    }

    // Unmap from target
    if (context == ContextTarget || NT_SUCCESS( PsLookupProcessByProcessId( pProcessEntry->target.pid, &pTargetProc ) ))
    {
        if (context != ContextTarget)
            KeStackAttachProcess( pTargetProc, &apc );

        BBSafeHandleClose( context == ContextTarget ? IoGetCurrentProcess() : pTargetProc, pProcessEntry->targetPipe, UserMode );

        if (pProcessEntry->target.sharedPage)
            MmUnmapLockedPages( pProcessEntry->target.sharedPage, pProcessEntry->pMDLShared );

        if (context != ContextTarget)
            KeUnstackDetachProcess( &apc );

        if (pTargetProc)
            ObDereferenceObject( pTargetProc );
    }

    // Free shared page
    if (pProcessEntry->pSharedPage)
    {
        if (pProcessEntry->pMDLShared)
        {
            IoFreeMdl( pProcessEntry->pMDLShared );
            pProcessEntry->pMDLShared = NULL;
        }

        ExFreePoolWithTag( pProcessEntry->pSharedPage, BB_POOL_TAG );
        pProcessEntry->pSharedPage = NULL;
    }

    // Make sure page list is cleaned up even if there is no Host process
    BBCleanupPageList( FALSE, &pProcessEntry->pageList );
    RtlDeleteElementGenericTableAvl( &g_ProcessPageTables, pProcessEntry );
}


/// <summary>
/// Unmap any mapped pages from host process
/// </summary>
/// <param name="pProcessEntry">Process entry</param>
VOID BBCleanupHostProcess( IN PPROCESS_MAP_ENTRY pProcessEntry )
{
    ASSERT( pProcessEntry != NULL );
    if (pProcessEntry == NULL)
        return;

    DPRINT( "BlackBone: %s: Host process %u shutdown. Cleanup\n", __FUNCTION__, pProcessEntry->host.pid );

    for (PLIST_ENTRY pListEntry = pProcessEntry->pageList.Flink; pListEntry != &pProcessEntry->pageList; pListEntry = pListEntry->Flink)
    {
        PMAP_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MAP_ENTRY, link );
        if (pEntry->newPtr)
        {
            MmUnmapLockedPages( (PVOID)pEntry->newPtr, pEntry->pMdl );
            pEntry->newPtr = 0;
        }
    }

    //
    // Unmap kernel shared page
    //
    if (pProcessEntry->pSharedPage &&
         pProcessEntry->pMDLShared &&
         pProcessEntry->host.sharedPage)
    {
        MmUnmapLockedPages( (PVOID)pProcessEntry->host.sharedPage, pProcessEntry->pMDLShared );
        pProcessEntry->host.sharedPage = 0;
    }

    // Set host process as none
    pProcessEntry->host.pid = 0;
}


/// <summary>
/// Clear global process map table
/// </summary>
VOID BBCleanupProcessTable()
{
    KeAcquireGuardedMutex( &g_globalLock );

    while (!RtlIsGenericTableEmptyAvl( &g_ProcessPageTables ))
        BBCleanupProcessEntry( (PPROCESS_MAP_ENTRY)RtlEnumerateGenericTableAvl( &g_ProcessPageTables, TRUE ) );

    KeReleaseGuardedMutex( &g_globalLock );
}


/// <summary>
/// Search process entry in list by PID
/// </summary>
/// <param name="pid">PID.</param>
/// <param name="asHost">If set to TRUE, pid is treated as host PID</param>
/// <returns>Found entry, NULL in not found</returns>
PPROCESS_MAP_ENTRY BBLookupProcessEntry( IN HANDLE pid, IN BOOLEAN asHost )
{
    PROCESS_MAP_ENTRY entry = { 0 };

    if (asHost)
        entry.host.pid = pid;
    else
        entry.target.pid = pid;

    return RtlLookupElementGenericTableAvl( &g_ProcessPageTables, &entry );
}


/// <summary>
/// Find memory region containing at least one byte from specific region
/// </summary>
/// <param name="pList">Region list to search in</param>
/// <param name="baseAddress">Base of region to find</param>
/// <param name="size">Size of the region to find</param>
/// <returns>Status code</returns>
PMAP_ENTRY BBFindPageEntry( IN PLIST_ENTRY pList, IN ULONG_PTR baseAddress, IN ULONG_PTR size )
{
    // Test if target region overlaps with any existing one
    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
    {
        PMAP_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MAP_ENTRY, link );

        if (baseAddress >= (ULONG_PTR)pEntry->mem.BaseAddress &&
            baseAddress < (ULONG_PTR)pEntry->mem.BaseAddress + pEntry->mem.RegionSize)
        { 
               return pEntry;
        }

        if (baseAddress + size >= (ULONG_PTR)pEntry->mem.BaseAddress &&
            baseAddress + size < (ULONG_PTR)pEntry->mem.BaseAddress + pEntry->mem.RegionSize)
        {
            return pEntry;
        }
    }

    return NULL;
}

/// <summary>
/// Calculate size required to store mapping info
/// </summary>
/// <param name="pList">Mapped regions list</param>
/// <param name="pSize">Resulting size</param>
/// <returns>Status code</returns>
NTSTATUS BBGetRequiredRemapOutputSize( IN PLIST_ENTRY pList, OUT PULONG_PTR pSize )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR size = sizeof( MAP_MEMORY_RESULT ) - sizeof( MAP_MEMORY_RESULT_ENTRY );

    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
        size += sizeof( MAP_MEMORY_RESULT_ENTRY );

    if (pSize)
        *pSize = size;

    return status;
}

