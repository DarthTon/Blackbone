#include "Private.h"
#include "Loader.h"
#include "BlackBoneDef.h"
#include "Utils.h"
#include <Ntstrsafe.h>

NTSTATUS BBMapWorker             ( IN PVOID pArg );
VOID     KernelApcPrepareCallback( PKAPC, PKNORMAL_ROUTINE*, PVOID*, PVOID*, PVOID* );
VOID     KernelApcInjectCallback ( PKAPC, PKNORMAL_ROUTINE*, PVOID*, PVOID*, PVOID* );
BOOLEAN  BBSkipThread            ( IN PETHREAD pThread, IN BOOLEAN isWow64 );


extern DYNAMIC_DATA dynData;

#pragma alloc_text(PAGE, BBInitLdrData)
#pragma alloc_text(PAGE, BBGetSystemModule)
#pragma alloc_text(PAGE, BBGetUserModule)
#pragma alloc_text(PAGE, BBUnlinkFromLoader)
#pragma alloc_text(PAGE, BBGetModuleExport)
#pragma alloc_text(PAGE, BBLookupProcessThread)
#pragma alloc_text(PAGE, BBQueueUserApc)
#pragma alloc_text(PAGE, KernelApcPrepareCallback)
#pragma alloc_text(PAGE, KernelApcInjectCallback)
#pragma alloc_text(PAGE, BBMapWorker)
#pragma alloc_text(PAGE, BBMMapDriver)
#pragma alloc_text(PAGE, BBSkipThread)

/// <summary>
/// Remove list entry
/// </summary>
/// <param name="Entry">Entry link</param>
FORCEINLINE VOID RemoveEntryList32( IN PLIST_ENTRY32 Entry )
{
    ((PLIST_ENTRY32)Entry->Blink)->Flink = Entry->Flink;
    ((PLIST_ENTRY32)Entry->Flink)->Blink = Entry->Blink;
}

/// <summary>
/// Initialize loader stuff
/// </summary>
/// <param name="pThisModule">Any valid system module</param>
/// <returns>Status code</returns>
NTSTATUS BBInitLdrData( IN PKLDR_DATA_TABLE_ENTRY pThisModule )
{
    PVOID kernelBase = GetKernelBase( NULL );
    if (kernelBase == NULL)
    {
        DPRINT( "BlackBone: %s: Failed to retrieve Kernel base address. Aborting\n", __FUNCTION__ );
        return STATUS_NOT_FOUND;
    }

    // Get PsLoadedModuleList address
    for (PLIST_ENTRY pListEntry = pThisModule->InLoadOrderLinks.Flink; pListEntry != &pThisModule->InLoadOrderLinks; pListEntry = pListEntry->Flink)
    {
        // Search for Ntoskrnl entry
        PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD( pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
        if (kernelBase == pEntry->DllBase)
        {
            // Ntoskrnl is always first entry in the list
            // Check if found pointer belongs to Ntoskrnl module
            if ((PVOID)pListEntry->Blink >= pEntry->DllBase && (PUCHAR)pListEntry->Blink < (PUCHAR)pEntry->DllBase + pEntry->SizeOfImage)
            {
                PsLoadedModuleList = pListEntry->Blink;
                break;
            }
        }
    }

    if (!PsLoadedModuleList)
    {
        DPRINT( "BlackBone: %s: Failed to retrieve PsLoadedModuleList address. Aborting\n", __FUNCTION__ );
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Get address of a system module
/// Either 'pName' or 'pAddress' is required to perform search
/// </summary>
/// <param name="pName">Base name of the image (e.g. hal.dll)</param>
/// <param name="pAddress">Address inside module</param>
/// <returns>Found loader entry. NULL if nothing found</returns>
PKLDR_DATA_TABLE_ENTRY BBGetSystemModule( IN PUNICODE_STRING pName, IN PVOID pAddress )
{
    ASSERT( (pName != NULL || pAddress != NULL) && PsLoadedModuleList != NULL );
    if ((pName == NULL && pAddress == NULL) || PsLoadedModuleList == NULL)
        return NULL;

    // No images
    if (IsListEmpty( PsLoadedModuleList ))
        return NULL;

    // Search in PsLoadedModuleList
    for (PLIST_ENTRY pListEntry = PsLoadedModuleList->Flink; pListEntry != PsLoadedModuleList; pListEntry = pListEntry->Flink)
    {
        PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD( pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks );

        // Check by name or by address
        if ((pName && RtlCompareUnicodeString( &pEntry->BaseDllName, pName, TRUE ) == 0) ||
             (pAddress && pAddress >= pEntry->DllBase && (PUCHAR)pAddress < (PUCHAR)pEntry->DllBase + pEntry->SizeOfImage))
        {
            return pEntry;
        }
    }

    return NULL;
}

/// <summary>
/// Get module base address by name
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="ModuleName">Nodule name to search for</param>
/// <param name="isWow64">If TRUE - search in 32-bit PEB</param>
/// <returns>Found address, NULL if not found</returns>
PVOID BBGetUserModule( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64 )
{
    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
        return NULL;

    // Protect from UserMode AV
    __try
    {
        LARGE_INTEGER time = { 0 };
        time.QuadPart = -250ll * 10 * 1000;     // 250 msec.

        // Wow64 process
        if (isWow64)
        {
            PPEB32 pPeb32 = (PPEB32)PsGetProcessWow64Process( pProcess );
            if (pPeb32 == NULL)
            {
                DPRINT( "BlackBone: %s: No PEB present. Aborting\n", __FUNCTION__ );
                return NULL;
            }

            // Wait for loader a bit
            for (INT i = 0; !pPeb32->Ldr && i < 10; i++)
            {
                DPRINT( "BlackBone: %s: Loader not intialiezd, waiting\n", __FUNCTION__ );
                KeDelayExecutionThread( KernelMode, TRUE, &time );
            }

            // Still no loader
            if (!pPeb32->Ldr)
            {
                DPRINT( "BlackBone: %s: Loader was not intialiezd in time. Aborting\n", __FUNCTION__ );
                return NULL;
            }

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
            {
                DPRINT( "BlackBone: %s: No PEB present. Aborting\n", __FUNCTION__ );
                return NULL;
            }

            // Wait for loader a bit
            for (INT i = 0; !pPeb->Ldr && i < 10; i++)
            {
                DPRINT( "BlackBone: %s: Loader not intialiezd, waiting\n", __FUNCTION__ );
                KeDelayExecutionThread( KernelMode, TRUE, &time );
            }

            // Still no loader
            if (!pPeb->Ldr)
            {
                DPRINT( "BlackBone: %s: Loader was not intialiezd in time. Aborting\n", __FUNCTION__ );
                return NULL;
            }

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
        DPRINT( "BlackBone: %s: Exception, Code: 0x%X\n", __FUNCTION__, GetExceptionCode() );
    }

    return NULL;
}

/// <summary>
/// Unlink user-mode module from Loader lists
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="pBase">Module base</param>
/// <param name="isWow64">If TRUE - unlink from PEB32 Loader, otherwise use PEB64</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkFromLoader( IN PEPROCESS pProcess, IN PVOID pBase, IN BOOLEAN isWow64 )
{
    NTSTATUS status = STATUS_SUCCESS;
    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
        return STATUS_INVALID_PARAMETER;

    // Protect from UserMode AV
    __try
    {
        // Wow64 process
        if (isWow64)
        {
            PPEB32 pPeb32 = (PPEB32)PsGetProcessWow64Process( pProcess );
            if (pPeb32 == NULL)
            {
                DPRINT( "BlackBone: %s: No PEB present. Aborting\n", __FUNCTION__ );
                return STATUS_NOT_FOUND;
            }

            // Search in InLoadOrderModuleList
            for (PLIST_ENTRY32 pListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList.Flink;
                  pListEntry != &((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList;
                  pListEntry = (PLIST_ENTRY32)pListEntry->Flink)
            {
                PLDR_DATA_TABLE_ENTRY32 pEntry = CONTAINING_RECORD( pListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks );

                // Unlink
                if ((PVOID)pEntry->DllBase == pBase)
                {
                    RemoveEntryList32( &pEntry->InLoadOrderLinks );
                    RemoveEntryList32( &pEntry->InInitializationOrderLinks );
                    RemoveEntryList32( &pEntry->InMemoryOrderLinks );
                    RemoveEntryList32( &pEntry->HashLinks );

                    break;
                }
            }
        }
        // Native process
        else
        {
            PPEB pPeb = PsGetProcessPeb( pProcess );
            if (!pPeb)
            {
                DPRINT( "BlackBone: %s: No PEB present. Aborting\n", __FUNCTION__ );
                return STATUS_NOT_FOUND;
            }

            // Search in InLoadOrderModuleList
            for (PLIST_ENTRY pListEntry = pPeb->Ldr->InLoadOrderModuleList.Flink;
                  pListEntry != &pPeb->Ldr->InLoadOrderModuleList;
                  pListEntry = pListEntry->Flink)
            {
                PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD( pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
                
                // Unlink
                if (pEntry->DllBase == pBase)
                {
                    RemoveEntryList( &pEntry->InLoadOrderLinks );
                    RemoveEntryList( &pEntry->InInitializationOrderLinks );
                    RemoveEntryList( &pEntry->InMemoryOrderLinks );
                    RemoveEntryList( &pEntry->HashLinks );

                    break;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception, Code: 0x%X\n", __FUNCTION__, GetExceptionCode() );
    }

    return status;
}

/// <summary>
/// Get exported function address
/// </summary>
/// <param name="pBase">Module base</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <param name="pProcess">Target process for user module</param>
/// <param name="baseName">Dll name for api schema</param>
/// <returns>Found address, NULL if not found</returns>
PVOID BBGetModuleExport( IN PVOID pBase, IN PCCHAR name_ord, IN PEPROCESS pProcess, IN PUNICODE_STRING baseName )
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

    /// Not a PE file
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

    PUSHORT pAddressOfOrds = (PUSHORT)(pExport->AddressOfNameOrdinals + (ULONG_PTR)pBase);
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
                WCHAR strbuf[256] = { 0 };
                ANSI_STRING forwarder = { 0 };
                ANSI_STRING import = { 0 };

                UNICODE_STRING uForwarder = { 0 };
                ULONG delimIdx = 0;
                PVOID forwardBase = NULL;
                PVOID result = NULL;

                // System image, not supported
                if (pProcess == NULL)
                    return NULL;

                RtlInitAnsiString( &forwarder, (PCSZ)pAddress );
                RtlInitEmptyUnicodeString( &uForwarder, strbuf, sizeof( strbuf ) );

                RtlAnsiStringToUnicodeString( &uForwarder, &forwarder, FALSE );
                for (ULONG j = 0; j < uForwarder.Length / sizeof( WCHAR ); j++)
                {
                    if (uForwarder.Buffer[j] == L'.')
                    {
                        uForwarder.Length = (USHORT)(j * sizeof( WCHAR ));
                        uForwarder.Buffer[j] = L'\0';
                        delimIdx = j;
                        break;
                    }
                }

                // Get forward function name/ordinal
                RtlInitAnsiString( &import, forwarder.Buffer + delimIdx + 1 );
                RtlAppendUnicodeToString( &uForwarder, L".dll" );

                //
                // Check forwarded module
                //
                UNICODE_STRING resolved = { 0 };
                UNICODE_STRING resolvedName = { 0 };
                BBResolveImagePath( NULL, pProcess, KApiShemaOnly, &uForwarder, baseName, &resolved );
                BBStripPath( &resolved, &resolvedName );

                forwardBase = BBGetUserModule( pProcess, &resolvedName, PsGetProcessWow64Process( pProcess ) != NULL );
                result = BBGetModuleExport( forwardBase, import.Buffer, pProcess, &resolvedName );
                RtlFreeUnicodeString( &resolved );

                return result;
            }

            break;
        }
    }

    return (PVOID)pAddress;
}

/// <summary>
/// Check if thread does not satisfy APC requirements
/// </summary>
/// <param name="pThread">Thread to check</param>
/// /// <param name="isWow64">If TRUE - check Wow64 TEB</param>
/// <returns>If TRUE - BBLookupProcessThread should skip thread</returns>
BOOLEAN BBSkipThread( IN PETHREAD pThread, IN BOOLEAN isWow64 )
{
    PUCHAR pTeb64 = PsGetThreadTeb( pThread );
    if (!pTeb64)
        return TRUE;

    // Skip GUI treads. APC to GUI thread causes ZwUserGetMessage to fail
    // TEB64 + 0x78  = Win32ThreadInfo
    if (*(PULONG64)(pTeb64 + 0x78) != 0)
        return TRUE;

    // Skip threads with no ActivationContext
    // Skip threads with no TLS pointer
    if (isWow64)
    {
        PUCHAR pTeb32 = pTeb64 + 0x2000;

        // TEB32 + 0x1A8 = ActivationContextStackPointer
        if (*(PULONG32)(pTeb32 + 0x1A8) == 0)
            return TRUE;

        // TEB64 + 0x2C = ThreadLocalStoragePointer
        if (*(PULONG32)(pTeb32 + 0x2C) == 0)
            return TRUE;
    }
    else
    {
        // TEB64 + 0x2C8 = ActivationContextStackPointer
        if (*(PULONG64)(pTeb64 + 0x2C8) == 0)
            return TRUE;

        // TEB64 + 0x58 = ThreadLocalStoragePointer
        if (*(PULONG64)(pTeb64 + 0x58) == 0)
            return TRUE;
    }

    return FALSE;
}

/// <summary>
/// Find first thread of the target process
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="ppThread">Found thread. Thread object reference count is increased by 1</param>
/// <returns>Status code</returns>
NTSTATUS BBLookupProcessThread( IN PEPROCESS pProcess, OUT PETHREAD* ppThread )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE pid = PsGetProcessId( pProcess );
    PVOID pBuf = ExAllocatePoolWithTag( NonPagedPool, 1024 * 1024, BB_POOL_TAG );
    PSYSTEM_PROCESS_INFO pInfo = (PSYSTEM_PROCESS_INFO)pBuf;

    ASSERT( ppThread != NULL );
    if (ppThread == NULL)
        return STATUS_INVALID_PARAMETER;

    if (!pInfo)
    {
        DPRINT( "BlackBone: %s: Failed to allocate memory for process list\n", __FUNCTION__ );
        return STATUS_NO_MEMORY;
    }

    // Get the process thread list
    status = ZwQuerySystemInformation( SystemProcessInformation, pInfo, 1024 * 1024, NULL );
    if (!NT_SUCCESS( status ))
    {
        ExFreePoolWithTag( pBuf, BB_POOL_TAG );
        return status;
    }

    // Find target thread
    if (NT_SUCCESS( status ))
    {
        status = STATUS_NOT_FOUND;
        for (;;)
        {
            if (pInfo->UniqueProcessId == pid)
            {
                status = STATUS_SUCCESS;
                break;
            }
            else if (pInfo->NextEntryOffset)
                pInfo = (PSYSTEM_PROCESS_INFO)((PUCHAR)pInfo + pInfo->NextEntryOffset);
            else
                break;
        }
    }

    BOOLEAN wow64 = PsGetProcessWow64Process( pProcess ) != NULL;

    // Reference target thread
    if (NT_SUCCESS( status ))
    {
        status = STATUS_NOT_FOUND;

        // Get first thread
        for (ULONG i = 0; i < pInfo->NumberOfThreads; i++)
        {
            // Skip current thread
            if (/*pInfo->Threads[i].WaitReason == Suspended ||
                 pInfo->Threads[i].ThreadState == 5 ||*/
                 pInfo->Threads[i].ClientId.UniqueThread == PsGetCurrentThreadId())
            {
                continue;
            }

            status = PsLookupThreadByThreadId( pInfo->Threads[i].ClientId.UniqueThread, ppThread );

            // Skip specific threads
            if (*ppThread && BBSkipThread( *ppThread, wow64 ))
            {
                ObDereferenceObject( *ppThread );
                *ppThread = NULL;
                continue;
            }

            break;
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to locate process\n", __FUNCTION__ );

    if (pBuf)
        ExFreePoolWithTag( pBuf, BB_POOL_TAG );

    // No suitable thread
    if (!*ppThread)
        status = STATUS_NOT_FOUND;

    return status;
}

/// <summary>
/// Create new thread in the target process
/// Must be running in target process context
/// </summary>
/// <param name="pBaseAddress">Thread start address</param>
/// <param name="pParam">Thread argument</param>
/// <param name="flags">Thread creation flags</param>
/// <param name="wait">If set to TRUE - wait for thread completion</param>
/// <param name="pExitStatus">Thread exit status</param>
/// <returns>Status code</returns>
NTSTATUS BBExecuteInNewThread(
    IN PVOID pBaseAddress,
    IN PVOID pParam,
    IN ULONG flags,
    IN BOOLEAN wait,
    OUT PNTSTATUS pExitStatus
    )
{
    HANDLE hThread = NULL;
    OBJECT_ATTRIBUTES ob = { 0 };

    InitializeObjectAttributes( &ob, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );

    NTSTATUS status = ZwCreateThreadEx(
        &hThread, THREAD_QUERY_LIMITED_INFORMATION, &ob,
        ZwCurrentProcess(), pBaseAddress, pParam, flags,
        0, 0x1000, 0x100000, NULL
        );

    // Wait for completion
    if (NT_SUCCESS( status ) && wait != FALSE)
    {
        // Force 60 sec timeout
        LARGE_INTEGER timeout = { 0 };
        timeout.QuadPart = -(60ll * 10 * 1000 * 1000);

        status = ZwWaitForSingleObject( hThread, TRUE, &timeout );
        if (NT_SUCCESS( status ))
        {
            THREAD_BASIC_INFORMATION info = { 0 };
            ULONG bytes = 0;

            status = ZwQueryInformationThread( hThread, ThreadBasicInformation, &info, sizeof( info ), &bytes );
            if (NT_SUCCESS( status ) && pExitStatus)
            {
                *pExitStatus = info.ExitStatus;
            }
            else if (!NT_SUCCESS( status ))
            {
                DPRINT( "BlackBone: %s: ZwQueryInformationThread failed with status 0x%X\n", __FUNCTION__, status );
            }
        }
        else
            DPRINT( "BlackBone: %s: ZwWaitForSingleObject failed with status 0x%X\n", __FUNCTION__, status );
    }
    else
        DPRINT( "BlackBone: %s: ZwCreateThreadEx failed with status 0x%X\n", __FUNCTION__, status );

    if (hThread)
        ZwClose( hThread );

    return status;
}

/// <summary>
/// Queue user-mode APC to the target thread
/// </summary>
/// <param name="pThread">Target thread</param>
/// <param name="pUserFunc">APC function</param>
/// <param name="Arg1">Argument 1</param>
/// <param name="Arg2">Argument 2</param>
/// <param name="Arg3">Argument 3</param>
/// <param name="bForce">If TRUE - force delivery by issuing special kernel APC</param>
/// <returns>Status code</returns>
NTSTATUS BBQueueUserApc(
    IN PETHREAD pThread,
    IN PVOID pUserFunc,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN BOOLEAN bForce
    )
{
    ASSERT( pThread != NULL );
    if (pThread == NULL)
        return STATUS_INVALID_PARAMETER;

    // Allocate APC
    PKAPC pPrepareApc = NULL;
    PKAPC pInjectApc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), BB_POOL_TAG );

    if (pInjectApc == NULL)
    {
        DPRINT( "BlackBone: %s: Failed to allocate APC\n", __FUNCTION__ );
        return STATUS_NO_MEMORY;
    }

    // Actual APC
    KeInitializeApc(
        pInjectApc, (PKTHREAD)pThread,
        OriginalApcEnvironment, &KernelApcInjectCallback,
        NULL, (PKNORMAL_ROUTINE)(ULONG_PTR)pUserFunc, UserMode, Arg1
        );

    // Setup force-delivery APC
    if (bForce)
    {
        pPrepareApc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), BB_POOL_TAG );
        KeInitializeApc(
            pPrepareApc, (PKTHREAD)pThread,
            OriginalApcEnvironment, &KernelApcPrepareCallback,
            NULL, NULL, KernelMode, NULL
            );
    }

    // Insert APC
    if (KeInsertQueueApc( pInjectApc, Arg2, Arg3, 0 ))
    {
        if (bForce && pPrepareApc)
            KeInsertQueueApc( pPrepareApc, NULL, NULL, 0 );

        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT( "BlackBone: %s: Failed to insert APC\n", __FUNCTION__ );

        ExFreePoolWithTag( pInjectApc, BB_POOL_TAG );

        if (pPrepareApc)
            ExFreePoolWithTag( pPrepareApc, BB_POOL_TAG );

        return STATUS_NOT_CAPABLE;
    }
}

//
// Injection APC routines
//
VOID KernelApcPrepareCallback(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2
    )
{
    UNREFERENCED_PARAMETER( NormalRoutine );
    UNREFERENCED_PARAMETER( NormalContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    //DPRINT( "BlackBone: %s: Called\n", __FUNCTION__ );

    // Alert current thread
    KeTestAlertThread( UserMode );
    ExFreePoolWithTag( Apc, BB_POOL_TAG );
}

VOID KernelApcInjectCallback(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2
    )
{
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    //DPRINT( "BlackBone: %s: Called. NormalRoutine = 0x%p\n", __FUNCTION__, *NormalRoutine );

    // Skip execution
    if (PsIsThreadTerminating( PsGetCurrentThread() ))
        *NormalRoutine = NULL;

    // Fix Wow64 APC
    if (PsGetCurrentProcessWow64Process() != NULL)
        PsWrapApcWow64Thread( NormalContext, (PVOID*)NormalRoutine );

    ExFreePoolWithTag( Apc, BB_POOL_TAG );
}

/// <summary>
/// System worker thread that performs actual mapping
/// </summary>
/// <param name="pArg">Path to the driver - PUNICODE_STRING type</param>
/// <returns>Status code</returns>
NTSTATUS BBMapWorker( IN PVOID pArg )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;
    PUNICODE_STRING pPath = (PUNICODE_STRING)pArg;
    OBJECT_ATTRIBUTES obAttr = { 0 };
    IO_STATUS_BLOCK statusBlock = { 0 };
    PVOID fileData = NULL;
    PIMAGE_NT_HEADERS pNTHeader = NULL;
    PVOID imageSection = NULL;
    PMDL pMDL = NULL;
    FILE_STANDARD_INFORMATION fileInfo = { 0 };

    InitializeObjectAttributes( &obAttr, pPath, OBJ_KERNEL_HANDLE, NULL, NULL );

    // Open driver file
    status = ZwCreateFile( 
        &hFile, FILE_READ_DATA | SYNCHRONIZE, &obAttr, 
        &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, 
        FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
        );

    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to open '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );
        PsTerminateSystemThread( status );
        return status;
    }

    // Allocate memory for file contents
    status = ZwQueryInformationFile( hFile, &statusBlock, &fileInfo, sizeof( fileInfo ), FileStandardInformation );
    if (NT_SUCCESS( status ))
        fileData = ExAllocatePoolWithTag( PagedPool, fileInfo.EndOfFile.QuadPart, BB_POOL_TAG );
    else
        DPRINT( "BlackBone: %s: Failed to get '%wZ' size. Status: 0x%X\n", __FUNCTION__, pPath, status );

    // Get file contents
    status = ZwReadFile( hFile, NULL, NULL, NULL, &statusBlock, fileData, fileInfo.EndOfFile.LowPart, NULL, NULL );
    if (NT_SUCCESS( status ))
    {
        pNTHeader = RtlImageNtHeader( fileData );
        if (!pNTHeader)
        {
            DPRINT( "BlackBone: %s: Failed to obtaint NT Header for '%wZ'\n", __FUNCTION__, pPath );
            status = STATUS_INVALID_IMAGE_FORMAT;
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to read '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );

    ZwClose( hFile );

    if (NT_SUCCESS( status ))
    {
        //
        // Allocate memory from System PTEs
        //
        PHYSICAL_ADDRESS start = { 0 }, end = { 0 };
        end.QuadPart = MAXULONG64;

        pMDL = MmAllocatePagesForMdl( start, end, start, pNTHeader->OptionalHeader.SizeOfImage );
        imageSection = MmGetSystemAddressForMdlSafe( pMDL, NormalPagePriority );

        if (NT_SUCCESS( status ) && imageSection)
        {
            // Copy header
            RtlCopyMemory( imageSection, fileData, pNTHeader->OptionalHeader.SizeOfHeaders );

            // Copy sections
            for (PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)(pNTHeader + 1);
                pSection < (PIMAGE_SECTION_HEADER)(pNTHeader + 1) + pNTHeader->FileHeader.NumberOfSections;
                pSection++)
            {
                RtlCopyMemory(
                    (PUCHAR)imageSection + pSection->VirtualAddress,
                    (PUCHAR)fileData + pSection->PointerToRawData,
                    pSection->SizeOfRawData
                    );
            }

            // Relocate image
            status = LdrRelocateImage( imageSection, STATUS_SUCCESS, STATUS_CONFLICTING_ADDRESSES, STATUS_INVALID_IMAGE_FORMAT );
            if (!NT_SUCCESS( status ))
                DPRINT( "BlackBone: %s: Failed to relocate image '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );

            // Fill IAT
            if (NT_SUCCESS( status ))
                status = BBResolveImageRefs( imageSection, TRUE, NULL, FALSE, NULL, 0 );
        }
        else
        {
            DPRINT( "BlackBone: %s: Failed to allocate memory for image '%wZ'\n", __FUNCTION__, pPath );
            status = STATUS_MEMORY_NOT_ALLOCATED;
        }
    }

    // SEH support
    /*if (NT_SUCCESS( status ))
    {
        //NTSTATUS( NTAPI* RtlInsertInvertedFunctionTable )(PVOID, SIZE_T) = (NTSTATUS( *)(PVOID, SIZE_T))((ULONG_PTR)GetKernelBase( NULL ) + 0x9B0A8);
        NTSTATUS( NTAPI* RtlInsertInvertedFunctionTable )(PVOID, PVOID, SIZE_T) = (NTSTATUS( *)(PVOID, PVOID, SIZE_T))((ULONG_PTR)GetKernelBase( NULL ) + 0x11E4C0);
        RtlInsertInvertedFunctionTable((PUCHAR)GetKernelBase( NULL ) + 0x1ED450, imageSection, pNTHeader->OptionalHeader.SizeOfImage );
    }*/

    // Initialize kernel security cookie
	if (NT_SUCCESS( status ))
		BBCreateCookie( imageSection );
    
    // Call entry point
    if (NT_SUCCESS( status ) && pNTHeader->OptionalHeader.AddressOfEntryPoint)
    {
        PDRIVER_INITIALIZE pEntryPoint = (PDRIVER_INITIALIZE)((ULONG_PTR)imageSection + pNTHeader->OptionalHeader.AddressOfEntryPoint);
        pEntryPoint( NULL, NULL );
    }

    // Wipe header
    if (NT_SUCCESS( status ) && imageSection)
        RtlZeroMemory( imageSection, pNTHeader->OptionalHeader.SizeOfHeaders );

    // Erase info about allocated region
    if (pMDL)
    {
        // Free image memory in case of failure
        if(!NT_SUCCESS( status ))
            MmFreePagesFromMdl( pMDL );

        ExFreePool( pMDL );
    }

    if (fileData)
        ExFreePoolWithTag( fileData, BB_POOL_TAG );

    if (NT_SUCCESS( status ))
        DPRINT( "BlackBone: %s: Successfully mapped '%wZ' at 0x%p\n", __FUNCTION__, pPath, imageSection );

    PsTerminateSystemThread( status );
    return status;
}

/// <summary>
/// Manually map driver into system space
/// </summary>
/// <param name="pPath">Fully qualified native path to the driver</param>
/// <returns>Status code</returns>
NTSTATUS BBMMapDriver( IN PUNICODE_STRING pPath )
{
    HANDLE hThread = NULL;
    CLIENT_ID clientID = { 0 };
    OBJECT_ATTRIBUTES obAttr = { 0 };
    PETHREAD pThread = NULL;
    OBJECT_HANDLE_INFORMATION handleInfo = { 0 };

    InitializeObjectAttributes( &obAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );

    ASSERT( pPath != NULL );
    if (pPath == NULL)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS status = PsCreateSystemThread( &hThread, THREAD_ALL_ACCESS, &obAttr, NULL, &clientID, &BBMapWorker, pPath );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to create worker thread. Status: 0x%X\n", __FUNCTION__, status );
        return status;
    }

    // Wait on worker thread
    status = ObReferenceObjectByHandle( hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThread, &handleInfo );
    if (NT_SUCCESS( status ))
    {
        THREAD_BASIC_INFORMATION info = { 0 };
        ULONG bytes = 0;

        status = KeWaitForSingleObject( pThread, Executive, KernelMode, TRUE, NULL );
        status = ZwQueryInformationThread( hThread, ThreadBasicInformation, &info, sizeof( info ), &bytes );
        if (NT_SUCCESS( status ));
            status = info.ExitStatus;
    }

    if (pThread)
        ObDereferenceObject( pThread );

    return status;
}
