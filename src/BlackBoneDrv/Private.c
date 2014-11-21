#include "Private.h"
#include <Ntstrsafe.h>

#pragma alloc_text(PAGE, ExpLookupHandleTableEntry)
#pragma alloc_text(PAGE, GetKernelBase)
#pragma alloc_text(PAGE, GetSSDTBase)
#pragma alloc_text(PAGE, GetSSDTEntry)
#pragma alloc_text(PAGE, GetPTEForVA)

extern DYNAMIC_DATA dynData;

PVOID g_KernelBase = NULL;
PVOID g_SSDT = NULL;

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
/// <returns>Found address, NULL if not found</returns>
PVOID GetKernelBase()
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytes = 0;
    PRTL_PROCESS_MODULES pMods = NULL;
    PVOID checkPtr = NULL;
    UNICODE_STRING routineName;

    // Already found
    if (g_KernelBase != NULL)
        return g_KernelBase;

    RtlUnicodeStringInit( &routineName, L"NtOpenFile" );

    checkPtr = MmGetSystemRoutineAddress( &routineName );
    if (checkPtr == NULL)
        return NULL;

    // Protect from UserMode AV
    __try
    {
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
                    break;
                }
            }
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
    }

    if (pMods)
        ExFreePoolWithTag( pMods, BB_POOL_TAG );

    return g_KernelBase;
}


/// <summary>
/// Gets SSDT base - KiServiceTable
/// </summary>
/// <returns>SSDT base, NULL if not found</returns>
PVOID GetSSDTBase()
{
    UNICODE_STRING fnName, fnZwName;
    PUCHAR ntosBase = NULL;
    PUCHAR pFn = NULL, pZwFn = NULL;

    // Already found
    if (g_SSDT != NULL)
        return g_SSDT;

    ntosBase = GetKernelBase();
    RtlUnicodeStringInit( &fnName,   L"NtRollbackTransaction" );
    RtlUnicodeStringInit( &fnZwName, L"ZwRollbackTransaction" );

    pFn   = MmGetSystemRoutineAddress( &fnName );
    pZwFn = MmGetSystemRoutineAddress( &fnZwName );

    //DPRINT( "BlackBone: %s: NtRollbackTransaction = 0x%p\n", __FUNCTION__, pFn );
    //DPRINT( "BlackBone: %s: ZwRollbackTransaction = 0x%p\n", __FUNCTION__, pZwFn );

    if (ntosBase && pFn && pZwFn)
    {
        PIMAGE_NT_HEADERS pHdr = RtlImageNtHeader( ntosBase );
        PIMAGE_SECTION_HEADER pFirstSec = (PIMAGE_SECTION_HEADER)(pHdr + 1);

        for (PIMAGE_SECTION_HEADER pSec = pFirstSec; pSec < pFirstSec + pHdr->FileHeader.NumberOfSections; pSec++)
        {
            // Non-paged, non-discardable, readable sections
            // Probably still not fool-proof enough...
            if (pSec->Characteristics & IMAGE_SCN_MEM_NOT_PAGED &&
                 pSec->Characteristics & IMAGE_SCN_MEM_READ &&
                 !(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
                 (*(PULONG)pSec->Name != 'TINI') &&
                 (*(PULONG)pSec->Name != 'EGAP'))
            {
                // Scan section
                for (ULONG_PTR* pPtr = (ULONG_PTR*)(ntosBase + pSec->VirtualAddress);
                      pPtr < (ULONG_PTR*)(ntosBase + pSec->VirtualAddress + pSec->Misc.VirtualSize);
                      pPtr++)
                {
                    // Found NtRollbackTransaction address
                    if (*pPtr == (ULONG_PTR)pFn)
                    {
                        //DPRINT( "BlackBone: %s: NtRollbackTransaction PE section: %c%c%c%c%c\n", __FUNCTION__, 
                                //pSec->Name[0], pSec->Name[1], pSec->Name[2], pSec->Name[3], pSec->Name[4] );

                        // Get SSDT index from ZwRollbackTransaction code
                        ULONG idx = *(PULONG)(pZwFn + 0x15);
                        //DPRINT( "BlackBone: %s: NtRollbackTransaction index = 0x%X\n", __FUNCTION__, idx );

                        // Get SSDT base
                        //DPRINT( "BlackBone: %s: SSDT base = 0xp\n", __FUNCTION__, g_SSDT );
                        return g_SSDT = (PUCHAR)pPtr - idx * sizeof( PVOID );
                    }
                }
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
    PVOID pSSDT = GetSSDTBase();

    // TODO: Handle second half of the table containing raw function pointers
    if (pSSDT)
        return (PUCHAR)pSSDT + (*((PULONG)pSSDT + index) >> 4);

    return NULL;
}

/// <summary>
/// Get page hardware PTE
/// Address must be valid, otherwise bug check is imminent
/// </summary>
/// <param name="pAddress">Target address</param>
/// <returns>Found PTE</returns>
PMMPTE GetPTEForVA( IN PVOID pAddress )
{
    // Check if large page
    PMMPTE pPDE = MiGetPdeAddress( pAddress );
    if (pPDE->u.Hard.LargePage)
        return pPDE;

    return MiGetPteAddress( pAddress );
}


#if defined(_WIN8_) || defined (_WIN7_)

NTSTATUS
NTAPI
ZwProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID* BaseAddress,
    IN SIZE_T* NumberOfBytesToProtect,
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
        *pPrevMode = KernelMode;

        status = NtProtectVirtualMemory( ProcessHandle, BaseAddress, NumberOfBytesToProtect, NewAccessProtection, OldAccessProtection );

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