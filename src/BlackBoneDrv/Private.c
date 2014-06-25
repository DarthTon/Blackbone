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

    if (TableCode != 0)
    {
        if (TableCode == 1)
        {
            return (PHANDLE_TABLE_ENTRY)(*(ULONG_PTR*)(HandleTable->TableCode + 8 * (tHandle.Value >> 10) - 1) + 4 * tHandle.Value & 0x3FF);
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
        status = STATUS_UNHANDLED_EXCEPTION;
    }

    if (pMods)
        ExFreePoolWithTag( pMods, BB_POOL_TAG );

    return g_KernelBase;
}

/// <summary>
/// Gets SSDT base - KiSystemServiceTable
/// </summary>
/// <returns>SSDT base, NULL if not found</returns>
PVOID GetSSDTBase()
{
    UNICODE_STRING fnName;
    PUCHAR ntosBase = NULL;
    PUCHAR pFn = NULL;

    // Already found
    if (g_SSDT != NULL)
        return g_SSDT;

    ntosBase = GetKernelBase();
    RtlUnicodeStringInit( &fnName, L"NtSetSecurityObject" );
    pFn = MmGetSystemRoutineAddress( &fnName );

    if (ntosBase && pFn)
    {
        PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)ntosBase;
        PIMAGE_NT_HEADERS pHdr = (PIMAGE_NT_HEADERS)(ntosBase + pDos->e_lfanew);
        PIMAGE_SECTION_HEADER pFirstSec = (PIMAGE_SECTION_HEADER)(pHdr + 1);

        for (PIMAGE_SECTION_HEADER pSec = pFirstSec; pSec < pFirstSec + pHdr->FileHeader.NumberOfSections; pSec++)
        {
            // Non-paged, executable sections
            if (pSec->Characteristics & 0x08000000 && pSec->Characteristics & 0x20000000)
            {
                // Scan section
                for (ULONG_PTR* pPtr = (ULONG_PTR*)(ntosBase + pSec->VirtualAddress);
                      pPtr < (ULONG_PTR*)(ntosBase + pSec->VirtualAddress + pSec->Misc.VirtualSize);
                      pPtr++)
                {
                    // Found NtSetSecurityObject address
                    if (*pPtr == (ULONG_PTR)pFn)
                        // Search for SSDT start
                        for (ULONG_PTR* pPtr2 = pPtr; pPtr2 > ( ULONG_PTR* )(ntosBase + pSec->VirtualAddress); pPtr2--)
                            if (*pPtr2 == 0x9090909090909090)
                                return g_SSDT = pPtr2 + 1;
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
PMMPTE_HARDWARE64 GetPTEForVA( IN PVOID pAddress )
{
    // Check if large page
    PMMPTE_HARDWARE64 pPDE = MiGetHardwarePdeAddress( pAddress );
    if (pPDE->LargePage)
        return pPDE;

    return MiGetHardwarePteAddress( pAddress );
}

#if defined(_WIN8_) || defined (_WIN7_)

// 'type cast' : from data pointer 'PVOID' to function pointer 'fnNtProtectVirtualMemory'
#pragma warning(disable : 4055)
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

    fnNtProtectVirtualMemory NtProtectVirtualMemory = (fnNtProtectVirtualMemory)GetSSDTEntry( dynData.NtProtectIndex );
    if (NtProtectVirtualMemory)
    {
        //
        // If previous mode is UserMode, addresses passed into NtProtectVirtualMemory must be in user-mode space
        // Switching to KernelMode will allow usage of kernel-mode addresses
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
#pragma  warning(default: 4055)
#endif