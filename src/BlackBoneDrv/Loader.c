#include "Private.h"
#include "Loader.h"
#include "BlackBoneDef.h"
#include "Utils.h"
#include <Ntstrsafe.h>

#define IMAGE32(hdr) hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC
#define IMAGE64(hdr) hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC
#define HEADER_VAL_T(hdr, val) (IMAGE64(hdr) ? ((PIMAGE_NT_HEADERS64)hdr)->OptionalHeader.val : ((PIMAGE_NT_HEADERS32)hdr)->OptionalHeader.val)


#if defined (_WIN81_) || defined (_WIN10_)
typedef PAPI_SET_VALUE_ENTRY     PAPISET_VALUE_ENTRY;
typedef PAPI_SET_VALUE_ARRAY     PAPISET_VALUE_ARRAY;
typedef PAPI_SET_NAMESPACE_ENTRY PAPISET_NAMESPACE_ENTRY;
typedef PAPI_SET_NAMESPACE_ARRAY PAPISET_NAMESPACE_ARRAY;
#else
typedef PAPI_SET_VALUE_ENTRY_V2     PAPISET_VALUE_ENTRY;
typedef PAPI_SET_VALUE_ARRAY_V2     PAPISET_VALUE_ARRAY;
typedef PAPI_SET_NAMESPACE_ENTRY_V2 PAPISET_NAMESPACE_ENTRY;
typedef PAPI_SET_NAMESPACE_ARRAY_V2 PAPISET_NAMESPACE_ARRAY;
#endif




VOID KernelApcPrepareCallback( PKAPC, PKNORMAL_ROUTINE*, PVOID*, PVOID*, PVOID* );
VOID KernelApcInjectCallback( PKAPC, PKNORMAL_ROUTINE*, PVOID*, PVOID*, PVOID* );

NTSTATUS BBFindOrMapModule(
    IN PEPROCESS pProcess,
    IN PUNICODE_STRING path,
    IN PVOID buffer, IN ULONG_PTR size,
    IN BOOLEAN asImage,
    IN MMmapFlags flags,
    IN PMMAP_CONTEXT pContext,
    OUT PMODULE_DATA pImage
    );

NTSTATUS BBResolveImagePath(
    IN PEPROCESS pProcess,
    IN PUNICODE_STRING path,
    IN PUNICODE_STRING baseImage,
    OUT PUNICODE_STRING resolved
    );

void BBCallInitRoutine( IN PMMAP_CONTEXT pContext );
NTSTATUS BBLoadLocalImage( IN PUNICODE_STRING path, OUT PVOID* pBase );
NTSTATUS BBCreateWorkerThread( IN PMMAP_CONTEXT pContext );
PMODULE_DATA BBLookupMappedModule( IN PLIST_ENTRY pList, IN PUNICODE_STRING pName, IN ModType type );
ULONG BBCastSectionProtection( ULONG characteristics );

NTSTATUS BBMapWorker( IN PVOID pArg );

PLIST_ENTRY PsLoadedModuleList;
extern DYNAMIC_DATA dynData;

#pragma alloc_text(PAGE, BBInitLdrData)
#pragma alloc_text(PAGE, BBGetSystemModule)
#pragma alloc_text(PAGE, BBGetUserModule)
#pragma alloc_text(PAGE, BBGetModuleExport)

#pragma alloc_text(PAGE, BBFindOrMapModule)
#pragma alloc_text(PAGE, BBResolveReferences)
#pragma alloc_text(PAGE, BBCallInitRoutine)
#pragma alloc_text(PAGE, BBLoadLocalImage)
#pragma alloc_text(PAGE, BBLookupMappedModule)
#pragma alloc_text(PAGE, BBCastSectionProtection)
#pragma alloc_text(PAGE, BBFileExists)

#pragma alloc_text(PAGE, BBMapWorker)
#pragma alloc_text(PAGE, BBMMapDriver)

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
    PVOID kernelBase = GetKernelBase();
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
        if ((pName && RtlCompareUnicodeString( &pEntry->BaseDllName, pName, FALSE ) == 0) ||
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
/// <param name="baseName">Base dll name for API schema</param>
/// <returns>Found address, NULL if not found</returns>
PVOID BBGetUserModule( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64, IN PUNICODE_STRING baseName )
{
    UNICODE_STRING resolved = { 0 };
    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
        return NULL;

    // Resolve image name
    UNICODE_STRING resolvedName = { 0 };
    BBResolveImagePath( pProcess, ModuleName, baseName, &resolved );
    BBStripPath( &resolved, &resolvedName );

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
                RtlFreeUnicodeString( &resolved );
                return NULL;
            }

            // Wait for loader a bit
            for (INT i = 10; !pPeb32->Ldr && i > 0; i++)
            {
                DPRINT( "BlackBone: %s: Loader not intialiezd, waiting\n", __FUNCTION__ );
                KeDelayExecutionThread( KernelMode, TRUE, &time );
            }

            // Search in InLoadOrderModuleList
            for (PLIST_ENTRY32 pListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList.Flink;
                  pListEntry != &((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList;
                  pListEntry = (PLIST_ENTRY32)pListEntry->Flink)
            {
                UNICODE_STRING ustr;
                PLDR_DATA_TABLE_ENTRY32 pEntry = CONTAINING_RECORD( pListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks );

                RtlUnicodeStringInit( &ustr, (PWCH)pEntry->BaseDllName.Buffer );

                if (RtlCompareUnicodeString( &ustr, &resolvedName, TRUE ) == 0)
                {
                    RtlFreeUnicodeString( &resolved );
                    return (PVOID)pEntry->DllBase;
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
                RtlFreeUnicodeString( &resolved );
                return NULL;
            }

            // Wait for loader a bit
            for (INT i = 10; !pPeb->Ldr && i > 0; i++)
            {
                DPRINT( "BlackBone: %s: Loader not intialiezd, waiting\n", __FUNCTION__ );
                KeDelayExecutionThread( KernelMode, TRUE, &time );
            }

            // Search in InLoadOrderModuleList
            for (PLIST_ENTRY pListEntry = pPeb->Ldr->InLoadOrderModuleList.Flink;
                  pListEntry != &pPeb->Ldr->InLoadOrderModuleList;
                  pListEntry = pListEntry->Flink)
            {
                PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD( pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
                if (RtlCompareUnicodeString( &pEntry->BaseDllName, &resolvedName, TRUE ) == 0)
                {
                    RtlFreeUnicodeString( &resolved );
                    return pEntry->DllBase;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception, Code: 0x%X\n", __FUNCTION__, GetExceptionCode() );
    }

    RtlFreeUnicodeString( &resolved );
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
PVOID BBGetModuleExport( IN PVOID pBase, IN PCCHAR name_ord, IN PEPROCESS pProcess, IN PUNICODE_STRING modName )
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
                    RtlUnicodeStringInit( &uForwarder, strbuf );
                    uForwarder.MaximumLength = sizeof( strbuf );

                    RtlAnsiStringToUnicodeString( &uForwarder, &forwarder, FALSE );
                    for (ULONG i = 0; i < uForwarder.Length / sizeof( WCHAR ); i++)
                    {
                        if (uForwarder.Buffer[i] == L'.')
                        {
                            uForwarder.Length = (USHORT)(i * sizeof( WCHAR ));
                            uForwarder.Buffer[i] = L'\0';
                            delimIdx = i;
                            break;
                        }
                    }

                    // Get forward function name/ordinal
                    RtlInitAnsiString( &import, forwarder.Buffer + delimIdx + 1 );
                    RtlAppendUnicodeToString( &uForwarder, L".dll" );

                    // Check forwarded module
                    forwardBase = BBGetUserModule( pProcess, &uForwarder, PsGetProcessWow64Process( pProcess ) != NULL, modName );
                    result = BBGetModuleExport( forwardBase, import.Buffer, pProcess, NULL );

                    return result;
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


NTSTATUS BBCallRoutine( IN PMMAP_CONTEXT pContext, IN PVOID pRoutine, IN INT argc, ... )
{
    NTSTATUS status = STATUS_SUCCESS;
    va_list vl;

    va_start( vl, argc );
    ULONG ofst = GenPrologue64( pContext->pCodeBuf );
    ofst += GenCall64V( (PUCHAR)pContext->pCodeBuf + ofst, pRoutine, argc, vl );
    ofst += GenSync64( (PUCHAR)pContext->pCodeBuf + ofst, pContext->pLastStatus, pContext->pSetEvent, pContext->hSync );
    ofst += GenEpilogue64( (PUCHAR)pContext->pCodeBuf + ofst );
    
    KeResetEvent( pContext->pSync );
    status = BBQueueUserApc( pContext->pWorker, pContext->pCodeBuf, NULL, NULL, NULL, FALSE );
    if (NT_SUCCESS( status ))
    {
        LARGE_INTEGER timeout = { 0 };
        timeout.QuadPart = -(10ll * 10 * 1000 * 1000);   // 10s

        status = KeWaitForSingleObject( pContext->pSync, Executive, UserMode, TRUE, &timeout );

        timeout.QuadPart = -(1ll * 10 * 1000);          // 1ms
        KeDelayExecutionThread( KernelMode, TRUE, &timeout );
    }
    
    va_end( vl );

    return status;
}


/// <summary>
/// Map new user module
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="path">Image path</param>
/// <param name="buffer">Image buffer</param>
/// <param name="size">Image buffer size</param>
/// <param name="asImage">Buffer has image memory layout</param>
/// <param name="flags">Mapping flags</param>
/// <param name="pImage">Mapped image data</param>
/// <returns>Status code</returns>
NTSTATUS BBMapUserImage(
    IN PEPROCESS pProcess,
    IN PUNICODE_STRING path,
    IN PVOID buffer, IN ULONG_PTR size,
    IN BOOLEAN asImage,
    IN MMmapFlags flags,
    OUT PMODULE_DATA pImage
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID systemBuffer = NULL;
    MMAP_CONTEXT context = { 0 };

    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
    {
        DPRINT( "BlackBone: %s: No process provided\n", __FUNCTION__ );
        return STATUS_INVALID_PARAMETER;
    }

    context.pProcess = pProcess;
    InitializeListHead( &context.modules );

    // Copy buffer to system space
    if (buffer)
    {
        __try
        {
            ProbeForRead( buffer, size, 0 );
            systemBuffer = ExAllocatePoolWithTag( PagedPool, size, BB_POOL_TAG );
            RtlCopyMemory( systemBuffer, buffer, size );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DPRINT( "BlackBone: %s: AV in user buffer: 0x%p - 0x%p\n", __FUNCTION__, buffer, (PUCHAR)buffer + size );
        }   
    }

    DPRINT( "BlackBone: %s: Mapping image '%wZ' with flags 0x%X\n", __FUNCTION__, path, flags );

    // Create worker
    status = BBCreateWorkerThread( &context );
    if (NT_SUCCESS( status ))
    {
        SIZE_T size = 0x2000;
        status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &context.pCodeBuf, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
        context.pLastStatus = (PNTSTATUS)((PUCHAR)context.pCodeBuf + 0x1EF0);
    }

    // Create sync event
    if (NT_SUCCESS( status ))
    {
        OBJECT_ATTRIBUTES eventAttr = { 0 };
        InitializeObjectAttributes( &eventAttr, 0, 0, 0, 0 );
        status = ZwCreateEvent( &context.hSync, EVENT_ALL_ACCESS, &eventAttr, NotificationEvent, FALSE );
        if (NT_SUCCESS( status ))
            status = ObReferenceObjectByHandle( context.hSync, EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, &context.pSync, NULL );

        if (!NT_SUCCESS( status ))
            DPRINT( "BlackBone: %s: Failed to create sync event. Status 0x%X\n", __FUNCTION__, status );

        UNICODE_STRING ustrNtdll;
        RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );

        PVOID pNtdll = BBGetUserModule( context.pProcess, &ustrNtdll, PsGetProcessWow64Process( pProcess ) != NULL, NULL );

        context.pSetEvent = BBGetModuleExport( pNtdll, "NtSetEvent", pProcess, NULL );
        context.pLoadImage = BBGetModuleExport( pNtdll, "LdrLoadDll", pProcess, NULL );
    }

    // Map module
    if (NT_SUCCESS( status ))
        status = BBFindOrMapModule( pProcess, path, systemBuffer, size, asImage, flags, &context, pImage );

    // Rebase process
    if (NT_SUCCESS( status ) && flags & KRebaseProcess)
    {
        PPEB pPeb = (PPEB)PsGetProcessPeb( pProcess );
        PPEB32 pPeb32 = (PPEB32)PsGetProcessWow64Process( pProcess );

        pPeb->ImageBaseAddress = pImage->baseAddress;
        if (pPeb32)
            pPeb32->ImageBaseAddress = (ULONG)pImage->baseAddress;
    }

    // Run module initializers
    if (NT_SUCCESS( status ))
        BBCallInitRoutine( &context );

 
    //
    // Cleanup
    //

    // Event
    if (context.pSync)
        ObDereferenceObject( context.pSync );
    /*if (context.hSync)
        ZwClose( context.hSync );*/

    // Worker thread
    if (context.pWorker)
        ObDereferenceObject( context.pWorker );
    if (context.hWorker)
    {
        LARGE_INTEGER timeout = { 0 };
        timeout.QuadPart = -(10ll * 10 * 1000);    // 10 ms
        ZwTerminateThread( context.hWorker, 0 );
        ZwWaitForSingleObject( context.hWorker, TRUE, &timeout );
        ZwClose( context.hWorker );
    }

    // Worker code buffer
    if (context.pWorkerBuf)
    {
        SIZE_T size = 0;
        ZwFreeVirtualMemory( ZwCurrentProcess(), &context.pWorkerBuf, &size, MEM_RELEASE );
    }

    // Code buffer
    if (context.pCodeBuf)
    {
        SIZE_T size = 0;
        ZwFreeVirtualMemory( ZwCurrentProcess(), &context.pCodeBuf, &size, MEM_RELEASE );
    }

    if (systemBuffer)
        ExFreePoolWithTag( systemBuffer, BB_POOL_TAG );

    // Cleanup module list
    while (!IsListEmpty( &context.modules ))
    {
        PVOID pListEntry = context.modules.Flink;
        PMODULE_DATA pEntry = (PMODULE_DATA)CONTAINING_RECORD( pListEntry, MODULE_DATA, link );

        RemoveHeadList( &context.modules );

        RtlFreeUnicodeString( &pEntry->fullPath );
        ExFreePoolWithTag( pListEntry, BB_POOL_TAG );
    }

    return status;
}


/// <summary>
/// Map new module or return existing
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="path">Image path</param>
/// <param name="buffer">Image buffer</param>
/// <param name="size">Image buffer size</param>
/// <param name="asImage">Buffer has image memory layout</param>
/// <param name="flags">Mapping flags</param>
/// <param name="pContext">Manual map context</param>
/// <param name="pImage">Mapped image data</param>
/// <returns>Status code</returns>
NTSTATUS BBFindOrMapModule(
    IN PEPROCESS pProcess,
    IN PUNICODE_STRING path,
    IN PVOID buffer, IN ULONG_PTR size,
    IN BOOLEAN asImage,
    IN MMmapFlags flags,
    IN PMMAP_CONTEXT pContext,
    OUT PMODULE_DATA pImage
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMODULE_DATA pLocalImage = NULL;
    PIMAGE_NT_HEADERS pHeaders = NULL;

    UNREFERENCED_PARAMETER( size );

    // Allocate image context record
    pLocalImage = ExAllocatePoolWithTag( PagedPool, sizeof( MODULE_DATA ), BB_POOL_TAG );
    RtlZeroMemory( pLocalImage, sizeof( MODULE_DATA ) );

    RtlCreateUnicodeString( &pLocalImage->fullPath, path->Buffer );
    BBStripPath( &pLocalImage->fullPath, &pLocalImage->name );
    pLocalImage->flags = flags;

    // Load image info 
    if (buffer == NULL)
    {
        status = BBLoadLocalImage( path, &pLocalImage->localBase );
        asImage = FALSE;
    }
    else
        pLocalImage->localBase = buffer;

    // Validate PE
    if (NT_SUCCESS( status ))
    {
        pHeaders = RtlImageNtHeader( pLocalImage->localBase );
        if (!pHeaders)
            status = STATUS_INVALID_IMAGE_FORMAT;
    }

    // Check if image already loaded
    if (NT_SUCCESS( status ))
    {
        pLocalImage->type = IMAGE64( pHeaders ) ? mt_mod64 : mt_mod32;
        PMODULE_DATA pFoundMod = BBLookupMappedModule( &pContext->modules, &pLocalImage->name, pLocalImage->type );
        if (pFoundMod != NULL)
        {
            RtlFreeUnicodeString( &pLocalImage->fullPath );
            if (pLocalImage->localBase)
                ExFreePoolWithTag( pLocalImage->localBase, BB_POOL_TAG );

            ExFreePoolWithTag( pLocalImage, BB_POOL_TAG );
            return STATUS_SUCCESS;
        }
    }

    // Allocate memory in target process
    if (NT_SUCCESS( status ))
    {
        pLocalImage->baseAddress = (PUCHAR)HEADER_VAL_T( pHeaders, ImageBase );
        pLocalImage->size = HEADER_VAL_T( pHeaders, SizeOfImage );
        status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pLocalImage->baseAddress, 0, &pLocalImage->size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );

        // Retry with arbitrary base
        if (!NT_SUCCESS( status ))
        {
            pLocalImage->baseAddress = NULL;
            pLocalImage->size = HEADER_VAL_T( pHeaders, SizeOfImage );
            status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pLocalImage->baseAddress, 0, &pLocalImage->size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
        }
    }

    // Copy image memory
    if (NT_SUCCESS( status ))
    {
        // Copy header
        RtlCopyMemory( pLocalImage->baseAddress, pLocalImage->localBase, HEADER_VAL_T( pHeaders, SizeOfHeaders ) );

        //
        // Copy sections
        //
        PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pHeaders + 1);
        if (IMAGE32( pHeaders ))
            pFirstSection = (PIMAGE_SECTION_HEADER)((PIMAGE_NT_HEADERS32)pHeaders + 1);

        for (PIMAGE_SECTION_HEADER pSection = pFirstSection;
              pSection < pFirstSection + pHeaders->FileHeader.NumberOfSections;
              pSection++)
        {
            // Skip invalid sections 
            if (!(pSection->Characteristics & (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE)) ||
                 /*(pSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) ||*/ pSection->SizeOfRawData == 0)
            {
                continue;
            }

            RtlCopyMemory(
                pLocalImage->baseAddress + pSection->VirtualAddress,
                pLocalImage->localBase + pSection->PointerToRawData,
                pSection->SizeOfRawData
                );
        }
    }

    // Relocate image
    if (NT_SUCCESS( status ))
    {
        status = LdrRelocateImage( pLocalImage->baseAddress, STATUS_SUCCESS, STATUS_CONFLICTING_ADDRESSES, STATUS_INVALID_IMAGE_FORMAT );
        if (!NT_SUCCESS( status ))
            DPRINT( "BlackBone: %s: Failed to relocate image '%wZ'. Status: 0x%X\n", __FUNCTION__, path, status );
    }

    // Resolve imports
    if (NT_SUCCESS( status ))
        status = BBResolveReferences( pLocalImage->baseAddress, path, FALSE, pProcess, IMAGE32( pHeaders ), pContext );

    // Set image memory protection
    if (NT_SUCCESS( status ))
    {
        SIZE_T tmpSize = 0;

        // Protect header
        tmpSize = HEADER_VAL_T( pHeaders, SizeOfHeaders );
        status = ZwProtectVirtualMemory( ZwCurrentProcess(), &pLocalImage->baseAddress, &tmpSize, PAGE_READONLY, NULL );

        //
        // Protect sections
        //
        PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pHeaders + 1);
        if (IMAGE32( pHeaders ))
            pFirstSection = (PIMAGE_SECTION_HEADER)((PIMAGE_NT_HEADERS32)pHeaders + 1);

        for (PIMAGE_SECTION_HEADER pSection = pFirstSection;
              pSection < pFirstSection + pHeaders->FileHeader.NumberOfSections;
              pSection++)
        {
            ULONG prot = BBCastSectionProtection( pSection->Characteristics );
            PUCHAR pAddr = pLocalImage->baseAddress + pSection->VirtualAddress;
            tmpSize = pSection->Misc.VirtualSize;

            // Decommit pages with NO_ACCESS protection
            if (prot != PAGE_NOACCESS)
                status |= ZwProtectVirtualMemory( ZwCurrentProcess(), &pAddr, &tmpSize, prot, NULL );
            else
                ZwFreeVirtualMemory( ZwCurrentProcess(), &pAddr, &tmpSize, MEM_DECOMMIT );
        }
    }

    //
    // Cleanup
    //


    // Free system-space image memory
    if (pLocalImage->localBase)
    {
        ExFreePoolWithTag( pLocalImage->localBase, BB_POOL_TAG );
        pLocalImage->localBase = NULL;
    }

    if (NT_SUCCESS( status ))
    {
        // Copy image info
        if (pImage)
            *pImage = *pLocalImage;

        InsertTailList( &pContext->modules, &pLocalImage->link );
    }
    else if (pLocalImage)
    {
        // Delete remote image
        if (pLocalImage->baseAddress)
        {
            SIZE_T tmpSize = 0;
            ZwFreeVirtualMemory( ZwCurrentProcess(), &pLocalImage->baseAddress, &tmpSize, MEM_RELEASE );
        }

        RtlFreeUnicodeString( &pLocalImage->fullPath );
        ExFreePoolWithTag( pLocalImage, BB_POOL_TAG );
    }

    return status;
}


/// <summary>
/// Resolve import table and load missing dependencies
/// </summary>
/// <param name="pImageBase">Target image base</param>
/// <param name="pName">Target image name</param>
/// <param name="systemImage">If TRUE - image is driver</param>
/// <param name="pProcess">Target process</param>
/// <param name="wow64Image">Iamge is 32bit image</param>
/// <param name="PMMAP_CONTEXT">Manual map context</param>
/// <returns>Status code</returns>
NTSTATUS BBResolveReferences(
    IN PVOID pImageBase,
    IN PUNICODE_STRING pName,
    IN BOOLEAN systemImage,
    IN PEPROCESS pProcess,
    IN BOOLEAN wow64Image,
    IN PMMAP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG impSize = 0;
    PIMAGE_IMPORT_DESCRIPTOR pImportTbl = RtlImageDirectoryEntryToData( pImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &impSize );

    // Imports
    for (; pImportTbl->Name && NT_SUCCESS( status ); ++pImportTbl)
    {
        PIMAGE_THUNK_DATA64 pThunk = (PIMAGE_THUNK_DATA64)((PUCHAR)pImageBase + (pImportTbl->OriginalFirstThunk ? pImportTbl->OriginalFirstThunk : pImportTbl->FirstThunk));
        UNICODE_STRING ustrImpDll;
        ANSI_STRING strImpDll;
        ULONG IAT_Index = 0;
        PCCHAR impFunc = NULL;
        PKLDR_DATA_TABLE_ENTRY pModule = NULL;

        RtlInitAnsiString( &strImpDll, (PCHAR)pImageBase + pImportTbl->Name );
        RtlAnsiStringToUnicodeString( &ustrImpDll, &strImpDll, TRUE );

        // Get import module
        pModule = systemImage ? BBGetSystemModule( &ustrImpDll, NULL ) : BBGetUserModule( pProcess, &ustrImpDll, wow64Image, pName );

        // Load missing import
        if (!pModule && !systemImage)
        {
            PVOID* pLoadedBase = (PVOID*)((PUCHAR)pContext->pCodeBuf + 0x1FD0);
            PUNICODE_STRING pStr = (PUNICODE_STRING)((PUCHAR)pContext->pCodeBuf + 0x1000);
            UNICODE_STRING resolved = { 0 };

            pStr->Buffer = (PWCH)(pStr + 1);
            pStr->MaximumLength = 0x200;

            BBResolveImagePath( pProcess, &ustrImpDll, NULL, &resolved );
            RtlCopyUnicodeString( pStr, &resolved );
            RtlFreeUnicodeString( &resolved );

            BBCallRoutine( pContext, pContext->pLoadImage, 4, (PVOID)NULL, (PVOID)0, pStr, pLoadedBase );
            pModule = *pLoadedBase;
        }

        if (!pModule)
        {
            DPRINT( "BlackBone: %s: Failed to resolve import module: '%wZ'\n", __FUNCTION__, ustrImpDll );
            RtlFreeUnicodeString( &ustrImpDll );

            return STATUS_NOT_FOUND;
        }

        while (pThunk->u1.AddressOfData)
        {
            PIMAGE_IMPORT_BY_NAME pAddressTable = (PIMAGE_IMPORT_BY_NAME)((PUCHAR)pImageBase + pThunk->u1.AddressOfData);
            PVOID pFunc = NULL;

            // import by name
            if (pThunk->u1.AddressOfData < IMAGE_ORDINAL_FLAG64 && pAddressTable->Name[0])
                impFunc = pAddressTable->Name;
            // import by ordinal
            else
                impFunc = (PCCHAR)(pThunk->u1.AddressOfData & 0xFFFF);

            pFunc = BBGetModuleExport( systemImage ? pModule->DllBase : pModule, impFunc, pContext->pProcess, &ustrImpDll );

            // No export found
            if (!pFunc)
            {
                if (pThunk->u1.AddressOfData < IMAGE_ORDINAL_FLAG64 && pAddressTable->Name[0])
                    DPRINT( "BlackBone: %s: Failed to resolve import '%wZ' : '%s'\n", __FUNCTION__, ustrImpDll, pAddressTable->Name );
                else
                    DPRINT( "BlackBone: %s: Failed to resolve import '%wZ' : '%d'\n", __FUNCTION__, ustrImpDll, pThunk->u1.AddressOfData & 0xFFFF );

                status = STATUS_NOT_FOUND;
                break;
            }

            // Save address to IAT
            if (pImportTbl->FirstThunk)
                *(PULONG_PTR)((PUCHAR)pImageBase + pImportTbl->FirstThunk + IAT_Index) = (ULONG_PTR)pFunc;
            // Save address to OrigianlFirstThunk
            else
                *(PULONG_PTR)((PUCHAR)pImageBase + pThunk->u1.AddressOfData) = (ULONG_PTR)pFunc;

            // Go to next entry
            pThunk++;
            IAT_Index += sizeof( ULONGLONG );
        }

        RtlFreeUnicodeString( &ustrImpDll );
    }

    return status;
}


NTSTATUS BBResolveApiSet( 
    IN PEPROCESS pProcess, 
    IN PUNICODE_STRING name,
    IN PUNICODE_STRING baseImage,
    OUT PUNICODE_STRING resolved 
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    PPEB32 pPeb32 = (PPEB32)PsGetProcessWow64Process( pProcess );
    PPEB pPeb = PsGetProcessPeb( pProcess );
    PAPISET_NAMESPACE_ARRAY pApiSetMap = (PAPISET_NAMESPACE_ARRAY)(pPeb32 != NULL ? (PVOID)pPeb32->ApiSetMap : pPeb->ApiSetMap);

    if (memcmp( name->Buffer, L"ext-ms-", 7) == 0)
    {
        name->Buffer[0] = L'a'; 
        name->Buffer[1] = L'p'; 
        name->Buffer[2] = L'i'; 
    }

    // Invalid name
    if (memcmp( name->Buffer, L"api-", 4 ) != 0)
    {
        return STATUS_NOT_FOUND;
    }

    // Iterate api set map
    for (ULONG i = 0; i < pApiSetMap->Count; i++)
    {
        PAPISET_NAMESPACE_ENTRY pDescriptor = pApiSetMap->Array + i;
        wchar_t apiNameBuf[255] = { 0 };
        UNICODE_STRING apiName = { 0 };

        wcscpy_s( apiNameBuf, 255, L"api-" );
        memcpy( apiNameBuf + 4, (PUCHAR)pApiSetMap + pDescriptor->NameOffset, pDescriptor->NameLength );
        wcscat_s( apiNameBuf, 255, L".dll" );

        RtlUnicodeStringInit( &apiName, apiNameBuf );

        // Check if this is a target dll
        if (RtlCompareUnicodeString( &apiName, name, TRUE ) == 0)
        {
            PAPISET_VALUE_ARRAY pHostData = (PAPISET_VALUE_ARRAY)((PUCHAR)pApiSetMap + pDescriptor->DataOffset);
            PAPISET_VALUE_ENTRY pHost = pHostData->Array;
            wchar_t apiHostNameBuf[255] = { 0 };
            UNICODE_STRING apiHostName = { 0 };

            // Sanity check
            if (pHostData->Count < 1)
                return STATUS_NOT_FOUND;

            memcpy( apiHostNameBuf, (PUCHAR)pApiSetMap + pHost->ValueOffset, pHost->ValueLength );
            RtlUnicodeStringInit( &apiHostName, apiHostNameBuf );

            // No base name redirection
            if (pHostData->Count == 1 || baseImage == NULL || baseImage->Buffer[0] == 0)
            {
                RtlCreateUnicodeString( resolved, apiHostName.Buffer );
                return STATUS_SUCCESS;
            }
            // Redirect accordingly to base name
            else
            {
                UNICODE_STRING baseImageName = { 0 };
                BBStripPath( baseImage, &baseImageName );

                if (RtlCompareUnicodeString( &apiHostName, &baseImageName, TRUE ) == 0)
                {
                    memset( apiHostNameBuf, 0, sizeof( apiHostNameBuf ) );
                    memcpy( apiHostNameBuf, (PUCHAR)pApiSetMap + pHost[1].ValueOffset, pHost[1].ValueLength );
                    RtlCreateUnicodeString( resolved, apiHostNameBuf );
                    return STATUS_SUCCESS;
                }
                else
                {
                    RtlCreateUnicodeString( resolved, apiHostName.Buffer );
                    return STATUS_SUCCESS;
                }
            }
        }
    }

    return status;
}

NTSTATUS BBResolveSxS(
    IN PUNICODE_STRING name,
    OUT PUNICODE_STRING resolved
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    UNREFERENCED_PARAMETER( name );
    UNREFERENCED_PARAMETER( resolved );

    return status;
}

NTSTATUS BBResolveImagePath( 
    IN PEPROCESS pProcess,
    IN PUNICODE_STRING path,
    IN PUNICODE_STRING baseImage,
    OUT PUNICODE_STRING resolved
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    UNICODE_STRING pathLow = { 0 };
    UNICODE_STRING filename = { 0 };

    UNREFERENCED_PARAMETER( baseImage );

    ASSERT( pProcess != NULL && path != NULL && resolved != NULL );
    if (pProcess == NULL || path == NULL || resolved == NULL)
    {
        DPRINT( "BlackBone: %s: Missing parameter\n", __FUNCTION__ );
        return STATUS_INVALID_PARAMETER;
    }

    RtlDowncaseUnicodeString( &pathLow, path, TRUE );
    BBStripPath( &pathLow, &filename );

    // API Schema
    if (NT_SUCCESS( BBResolveApiSet( pProcess, &filename, baseImage, resolved ) ))
    {
        DPRINT( "BlackBone: %s: Resolved %wZ -> %wZ\n", __FUNCTION__, path, resolved );
        RtlFreeUnicodeString( &pathLow );
        return STATUS_SUCCESS;
    }

    // SxS
    if (NT_SUCCESS( BBResolveSxS( &filename, resolved ) ))
    {
        DPRINT( "BlackBone: %s: Resolved %wZ -> %wZ\n", __FUNCTION__, path, resolved );
        RtlFreeUnicodeString( &pathLow );
        return STATUS_SUCCESS;
    }


    // Nothing found
    DPRINT( "BlackBone: %s: Failed to resolve %wZ\n", __FUNCTION__, path );
    *resolved = pathLow;
    return status;
}

/// <summary>
/// Call module initialization routines
/// </summary>
/// <param name="pContext">Map context</param>
void BBCallInitRoutine( IN PMMAP_CONTEXT pContext )
{
    for (PLIST_ENTRY pListEntry = pContext->modules.Flink; pListEntry != &pContext->modules; pListEntry = pListEntry->Flink)
    {
        PMODULE_DATA pEntry = (PMODULE_DATA)CONTAINING_RECORD( pListEntry, MODULE_DATA, link );
        if (pEntry->initialized)
            continue;

        PIMAGE_NT_HEADERS pHeaders = RtlImageNtHeader( pEntry->baseAddress );
        if (HEADER_VAL_T( pHeaders, AddressOfEntryPoint ))
        {
            PUCHAR entrypoint = pEntry->baseAddress + HEADER_VAL_T( pHeaders, AddressOfEntryPoint );
            BBCallRoutine( pContext, entrypoint, 3, pEntry->baseAddress, (PVOID)1, NULL );
        }

        if (pEntry->flags & KWipeHeader)
            RtlZeroMemory( &pEntry->baseAddress, HEADER_VAL_T( pHeaders, SizeOfHeaders ) );

        pEntry->initialized = TRUE;
    }
}

/// <summary>
/// Load image from disk into system memory
/// </summary>
/// <param name="path">Image path</param>
/// <param name="pBase">Mapped base</param>
/// <returns>Status code</returns>
NTSTATUS BBLoadLocalImage( IN PUNICODE_STRING path, OUT PVOID* pBase )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;
    OBJECT_ATTRIBUTES obAttr = { 0 };
    IO_STATUS_BLOCK statusBlock = { 0 };
    FILE_STANDARD_INFORMATION fileInfo = { 0 };

    ASSERT( path != NULL && pBase != NULL );
    if (path == NULL || pBase == NULL)
    {
        DPRINT( "BlackBone: %s: No image path or output base\n", __FUNCTION__ );
        return STATUS_INVALID_PARAMETER;
    }

    InitializeObjectAttributes( &obAttr, path, OBJ_KERNEL_HANDLE, NULL, NULL );

    // Open image file
    status = ZwCreateFile(
        &hFile, FILE_READ_DATA | SYNCHRONIZE, &obAttr,
        &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
        );

    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to open '%wZ'. Status: 0x%X\n", __FUNCTION__, path, status );
        return status;
    }

    // Allocate memory for file contents
    status = ZwQueryInformationFile( hFile, &statusBlock, &fileInfo, sizeof( fileInfo ), FileStandardInformation );
    if (NT_SUCCESS( status ))
        *pBase = ExAllocatePoolWithTag( PagedPool, fileInfo.EndOfFile.QuadPart, BB_POOL_TAG );
    else
        DPRINT( "BlackBone: %s: Failed to get '%wZ' size. Status: 0x%X\n", __FUNCTION__, path, status );

    // Get file contents
    status = ZwReadFile( hFile, NULL, NULL, NULL, &statusBlock, *pBase, fileInfo.EndOfFile.LowPart, NULL, NULL );
    if (NT_SUCCESS( status ))
    {
        PIMAGE_NT_HEADERS pNTHeader = RtlImageNtHeader( *pBase );
        if (!pNTHeader)
        {
            DPRINT( "BlackBone: %s: Failed to obtaint NT Header for '%wZ'\n", __FUNCTION__, path );
            status = STATUS_INVALID_IMAGE_FORMAT;
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to read '%wZ'. Status: 0x%X\n", __FUNCTION__, path, status );

    ZwClose( hFile );

    if (!NT_SUCCESS( status ) && *pBase)
        ExFreePoolWithTag( *pBase, BB_POOL_TAG );

    return status;
}

/// <summary>
/// Create worker thread for user-mode calls
/// </summary>
/// <param name="pContext">Map context</param>
/// <returns>Status code</returns>
NTSTATUS BBCreateWorkerThread( IN PMMAP_CONTEXT pContext )
{
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T codeSize = 0x1000;
    ASSERT( pContext != NULL );
    if (pContext == NULL )
        return STATUS_INVALID_PARAMETER;

    pContext->pWorkerBuf = NULL;
    status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pContext->pWorkerBuf, 0, &codeSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (NT_SUCCESS( status ))
    {
        PUCHAR pBuf = pContext->pWorkerBuf;
        UNICODE_STRING ustrNtdll;
        RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );
        BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

        PVOID pNtDelayExec = BBGetModuleExport( BBGetUserModule( pContext->pProcess, &ustrNtdll, wow64, NULL ), "NtDelayExecution", NULL, NULL );

        if (pNtDelayExec)
        {
            OBJECT_ATTRIBUTES obattr = { 0 };
            PLARGE_INTEGER pDelay = (PLARGE_INTEGER)(pBuf + 0x100);
            pDelay->QuadPart = -(5ll * 10 * 1000);
            ULONG ofst = 0;

            if (wow64)
            {

            }
            else
            {
                *(PUSHORT)(pBuf + ofst) = 0xB948;           // mov rcx, TRUE
                *(PULONG_PTR)(pBuf + ofst + 2) = TRUE;      //
                ofst += 10;

                *(PUSHORT)(pBuf + ofst) = 0xBA48;           // mov rdx, arg
                *(PVOID*)(pBuf + ofst + 2) = pDelay;        //
                ofst += 10;

                *(PUSHORT)(pBuf + ofst) = 0xB848;           // mov rax, pFn
                *(PVOID*)(pBuf + ofst + 2) = pNtDelayExec;  //
                ofst += 10;

                *(PUSHORT)(pBuf + ofst) = 0xD0FF;           // call rax
                ofst += 2;

                *(PUSHORT)(pBuf + ofst) = 0xDEEB;           // jmp
                ofst += 2;
            }

            InitializeObjectAttributes( &obattr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
            status = ZwCreateThreadEx(
                &pContext->hWorker, THREAD_ALL_ACCESS, &obattr,
                ZwCurrentProcess(), pContext->pWorkerBuf, NULL, 0,
                0, 0x1000, 0x100000, NULL
                );

            if (NT_SUCCESS( status ))
                ObReferenceObjectByHandle( pContext->hWorker, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pContext->pWorker, NULL );
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

/// <summary>
/// Find existing module in the list
/// </summary>
/// <param name="pList">Module list</param>
/// <param name="pName">Module name</param>
/// <param name="type">Module type</param>
/// <returns>Module info address; NULL if not found</returns>
PMODULE_DATA BBLookupMappedModule( IN PLIST_ENTRY pList, IN PUNICODE_STRING pName, IN ModType type )
{
    // Sanity check
    ASSERT( pList != NULL && pName != NULL );
    if (pList == NULL || pName == NULL)
        return NULL;

    if (IsListEmpty( pList ))
        return NULL;

    // Walk list
    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
    {
        PMODULE_DATA pEntry = (PMODULE_DATA)CONTAINING_RECORD( pListEntry, MODULE_DATA, link );
        if (type == pEntry->type && RtlCompareUnicodeString( &pEntry->name, pName, TRUE ) == 0)
            return pEntry;
    }

    return NULL;
}

/// <summary>
/// Get memory protection form section characteristics
/// </summary>
/// <param name="characteristics">Characteristics</param>
/// <returns>Memory protection value</returns>
ULONG BBCastSectionProtection( ULONG characteristics )
{
    ULONG dwResult = PAGE_NOACCESS;

    if (characteristics & IMAGE_SCN_MEM_DISCARDABLE)
    {
        dwResult = PAGE_NOACCESS;
    }
    else if (characteristics & IMAGE_SCN_MEM_EXECUTE)
    {
        if (characteristics & IMAGE_SCN_MEM_WRITE)
            dwResult = PAGE_EXECUTE_READWRITE;
        else if (characteristics & IMAGE_SCN_MEM_READ)
            dwResult = PAGE_EXECUTE_READ;
        else
            dwResult = PAGE_EXECUTE;
    }
    else
    {
        if (characteristics & IMAGE_SCN_MEM_WRITE)
            dwResult = PAGE_READWRITE;
        else if (characteristics & IMAGE_SCN_MEM_READ)
            dwResult = PAGE_READONLY;
        else
            dwResult = PAGE_NOACCESS;
    }

    return dwResult;
}

/// <summary>
/// Find first thread of the target process
/// </summary>
/// <param name="pid">Target PID.</param>
/// <param name="ppThread">Found thread. Thread object reference count is increased by 1</param>
/// <returns>Status code</returns>
NTSTATUS BBLookupProcessThread( IN HANDLE pid, OUT PETHREAD* ppThread )
{
    NTSTATUS status = STATUS_SUCCESS;
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
                 pInfo->Threads[i].ClientId.UniqueThread == PsGetCurrentThread())
            {
                continue;
            }

            status = PsLookupThreadByThreadId( pInfo->Threads[i].ClientId.UniqueThread, ppThread );
            break;
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to locate process\n", __FUNCTION__ );

    if (pBuf)
        ExFreePoolWithTag( pBuf, BB_POOL_TAG );

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
    IN BOOLEAN bForce )
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
        if (pPrepareApc)
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

    DPRINT( "BlackBone: %s: Called\n", __FUNCTION__ );

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

    DPRINT( "BlackBone: %s: Called\n", __FUNCTION__ );

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
    status = ZwCreateFile( &hFile, FILE_READ_DATA | SYNCHRONIZE, &obAttr, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to open '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );
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

    __try
    {
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
                    RtlCopyMemory( (PUCHAR)imageSection + pSection->VirtualAddress,
                                   (PUCHAR)fileData + pSection->PointerToRawData,
                                   pSection->SizeOfRawData );
                }

                // Relocate image
                status = LdrRelocateImage( imageSection, STATUS_SUCCESS, STATUS_CONFLICTING_ADDRESSES, STATUS_INVALID_IMAGE_FORMAT );
                if (!NT_SUCCESS( status ))
                    DPRINT( "BlackBone: %s: Failed to relocate image '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );

                // Fill IAT
                if (NT_SUCCESS( status ))
                    status = BBResolveReferences( imageSection, NULL, TRUE, NULL, FALSE, NULL );
            }
            else
            {
                DPRINT( "BlackBone: %s: Failed to allocate memory for image '%wZ'\n", __FUNCTION__, pPath );
                status = STATUS_MEMORY_NOT_ALLOCATED;
            }
        }

        // Call entrypoint
        if (NT_SUCCESS( status ) && pNTHeader->OptionalHeader.AddressOfEntryPoint)
        {
            PDRIVER_INITIALIZE pEntryPoint = (PDRIVER_INITIALIZE)((ULONG_PTR)imageSection + pNTHeader->OptionalHeader.AddressOfEntryPoint);
            pEntryPoint( NULL, NULL );
        }

        // Wipe header
        if (NT_SUCCESS( status ) && imageSection)
            RtlZeroMemory( imageSection, pNTHeader->OptionalHeader.SizeOfHeaders );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception: 0x%X \n", __FUNCTION__, GetExceptionCode() );
    }

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
        status = info.ExitStatus;
    }

    if (pThread)
        ObDereferenceObject( pThread );

    return status;
}
