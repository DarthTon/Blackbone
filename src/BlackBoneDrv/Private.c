#include "Private.h"
#include <Ntstrsafe.h>

#pragma alloc_text(PAGE, ExpLookupHandleTableEntry)
#pragma alloc_text(PAGE, GetKernelBase)
#pragma alloc_text(PAGE, GetModuleBase)
#pragma alloc_text(PAGE, GetModuleExport)
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
/// Get module base address by name
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="ModuleName">Nodule name to search for</param>
/// <param name="isWow64">If TRUE - search in 32-bit PEB</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetModuleBase( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64 )
{
    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
        return NULL;

    // Protect from UserMode AV
    __try
    {

        // Wow64 process
        if (isWow64)
        {
            PPEB32 pPeb32 = NULL;
            ZwQueryInformationProcess( ZwCurrentProcess(), ProcessWow64Information, &pPeb32, sizeof( pPeb32 ), NULL );
            if (pPeb32 == NULL)
                return NULL;

            // Search in InLoadOrderModuleList
            for (PLIST_ENTRY32 pListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList.Flink;
                  pListEntry != &((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList;
                  pListEntry = (PLIST_ENTRY32)pListEntry->Flink)
            {
                UNICODE_STRING ustr;
                PLDR_DATA_TABLE_ENTRY32 pEntry = CONTAINING_RECORD( pListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks );

                RtlUnicodeStringInit( &ustr, (PWCH)pEntry->BaseDllName.Buffer );

                if (RtlCompareUnicodeString( &ustr, ModuleName, TRUE ) == 0)
                    return (PVOID)pEntry->DllBase;
            }
        }
        // Native process
        else
        {
            PPEB pPeb = PsGetProcessPeb( pProcess );
            if (!pPeb)
                return NULL;

            // Search in InLoadOrderModuleList
            for (PLIST_ENTRY pListEntry = pPeb->Ldr->InLoadOrderModuleList.Flink;
                  pListEntry != &pPeb->Ldr->InLoadOrderModuleList;
                  pListEntry = pListEntry->Flink)
            {
                PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD( pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
                if (RtlCompareUnicodeString( &pEntry->BaseDllName, ModuleName, TRUE ) == 0)
                    return pEntry->DllBase;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
    }

    return NULL;
}

/// <summary>
/// Get exported function address
/// </summary>
/// <param name="pBase">Module base</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetModuleExport( IN PVOID pBase, IN PCCHAR name_ord )
{
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS32 pNtHdr32 = NULL;
    PIMAGE_NT_HEADERS64 pNtHdr64 = NULL;
    PIMAGE_EXPORT_DIRECTORY pExport = NULL;
    ULONG expSize = 0;
    ULONG_PTR pAddress = 0;

    ASSERT( pBase != NULL );
    if (pBase == NULL)
        return NULL;

    // Protect from UserMode AV
    __try
    {
        // Not a PE file
        if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
            return NULL;

        pNtHdr32 = (PIMAGE_NT_HEADERS32)((PUCHAR)pBase + pDosHdr->e_lfanew);
        pNtHdr64 = (PIMAGE_NT_HEADERS64)((PUCHAR)pBase + pDosHdr->e_lfanew);

        // Not a PE file
        if (pNtHdr32->Signature != IMAGE_NT_SIGNATURE)
            return NULL;

        // 64 bit image
        if (pNtHdr32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            pExport = (PIMAGE_EXPORT_DIRECTORY)(pNtHdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)pBase);
            expSize = pNtHdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        }
        // 32 bit image
        else
        {
            pExport = (PIMAGE_EXPORT_DIRECTORY)(pNtHdr32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)pBase);
            expSize = pNtHdr32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        }

        PUSHORT pAddressOfOrds  = (PUSHORT)(pExport->AddressOfNameOrdinals + (ULONG_PTR)pBase);
        PULONG  pAddressOfNames = (PULONG)(pExport->AddressOfNames + (ULONG_PTR)pBase);
        PULONG  pAddressOfFuncs = (PULONG)(pExport->AddressOfFunctions + (ULONG_PTR)pBase);

        for (ULONG i = 0; i < pExport->NumberOfFunctions; ++i)
        {
            USHORT OrdIndex = 0xFFFF;
            PCHAR  pName = NULL;

            // Find by index
            if ((ULONG_PTR)name_ord <= 0xFFFF)
            {
                OrdIndex = (USHORT)i;
            }
            // Find by name
            else if ((ULONG_PTR)name_ord > 0xFFFF && i < pExport->NumberOfNames)
            {
                pName = (PCHAR)(pAddressOfNames[i] + (ULONG_PTR)pBase);
                OrdIndex = pAddressOfOrds[i];
            }
            // Weird params
            else
                return NULL;

            if (((ULONG_PTR)name_ord <= 0xFFFF && (USHORT)((ULONG_PTR)name_ord) == OrdIndex + pExport->Base) ||
                 ((ULONG_PTR)name_ord > 0xFFFF && strcmp( pName, name_ord ) == 0))
            {
                pAddress = pAddressOfFuncs[OrdIndex] + (ULONG_PTR)pBase;

                // Check forwarded export
                if (pAddress >= (ULONG_PTR)pExport && pAddress <= (ULONG_PTR)pExport + expSize)
                {
                }

                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
    }

    return (PVOID)pAddress;
}

/// <summary>
/// Gets SSDT base - KiServiceTable
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
            // Non-paged, non-discardable, executable sections
            if (pSec->Characteristics & 0x08000000 && pSec->Characteristics & 0x20000000 && !(pSec->Characteristics & 0x02000000))
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
                            if (*pPtr2 == 0x9090909090909090 || *pPtr2 == 0xCCCCCCCCCCCCCCCC)
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

// 'type cast' : from data pointer 'PVOID' to function pointer 'fnNtProtectVirtualMemory'
#pragma warning(disable : 4055)

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

    fnNtProtectVirtualMemory NtProtectVirtualMemory = (fnNtProtectVirtualMemory)GetSSDTEntry( dynData.NtProtectIndex );
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

    fnNtCreateThreadEx NtCreateThreadEx = (fnNtCreateThreadEx)GetSSDTEntry( dynData.NtThdIndex );
    if (NtCreateThreadEx)
    {
        //
        // If previous mode is UserMode, addresses passed into ZwCreateThreadEx must be in user-mode space
        // Switching to KernelMode allows usage of kernel-mode addresses
        //
        PUCHAR pPrevMode = (PUCHAR)PsGetCurrentThread() + dynData.PrevMode;
        UCHAR prevMode = *pPrevMode;
        *pPrevMode = KernelMode;

        status = NtCreateThreadEx( hThread, DesiredAccess, ObjectAttributes,
                                   ProcessHandle, lpStartAddress, lpParameter,
                                   Flags, StackZeroBits, SizeOfStackCommit,
                                   SizeOfStackReserve, AttributeList );
        *pPrevMode = prevMode;
    }
    else
        status = STATUS_NOT_FOUND;

    return status;
}

#pragma  warning(default: 4055)
