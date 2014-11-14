#include "Loader.h"
#include "Private.h"
#include "BlackBoneDef.h"
#include "Utils.h"
#include <ntstrsafe.h>

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

PLIST_ENTRY PsLoadedModuleList;
extern DYNAMIC_DATA dynData;

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
    );

/// <summary>
/// Try to resolve image via API SET map
/// </summary>
/// <param name="pProcess">Target process. Must be run in the context of this process</param>
/// <param name="name">Name to resolve</param>
/// <param name="baseImage">Parent image name</param>
/// <param name="resolved">Resolved name if any</param>
/// <returns>Status code</returns>
NTSTATUS BBResolveApiSet( IN PEPROCESS pProcess, IN PUNICODE_STRING name, IN PUNICODE_STRING baseImage, OUT PUNICODE_STRING resolved );

/// <summary>
///Try to resolve image via SxS isolation
/// </summary>
/// <param name="pContext">Loader context.</param>
/// <param name="name">Name to resolve</param>
/// <param name="resolved">Resolved name if any</param>
/// <returns>Status code</returns>
NTSTATUS BBResolveSxS( IN PMMAP_CONTEXT pContext, IN PUNICODE_STRING name, OUT PUNICODE_STRING resolved );

/// <summary>
/// Call module initialization routines
/// </summary>
/// <param name="pContext">Map context</param>
void BBCallInitRoutine( IN PMMAP_CONTEXT pContext );

/// <summary>
/// Create activation context
/// </summary>
/// <param name="pPath">Image path</param>
/// <param name="pContext">Loader context</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareACTX( IN PUNICODE_STRING pPath, IN PMMAP_CONTEXT pContext );

/// <summary>
/// Load image from disk into system memory
/// </summary>
/// <param name="path">Image path</param>
/// <param name="pBase">Mapped base</param>
/// <returns>Status code</returns>
NTSTATUS BBLoadLocalImage( IN PUNICODE_STRING path, OUT PVOID* pBase );

/// <summary>
/// Create worker thread for user-mode calls
/// </summary>
/// <param name="pContext">Map context</param>
/// <returns>Status code</returns>
NTSTATUS BBCreateWorkerThread( IN PMMAP_CONTEXT pContext );

/// <summary>
/// Call arbitrary function
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="pRoutine">Routine to call.</param>
/// <param name="argc">Number of arguments.</param>
/// <param name="...">Arguments</param>
/// <returns>Status code</returns>
NTSTATUS BBCallRoutine( IN PMMAP_CONTEXT pContext, IN PVOID pRoutine, IN INT argc, ... );

/// <summary>
/// Find existing module in the list
/// </summary>
/// <param name="pList">Module list</param>
/// <param name="pName">Module name</param>
/// <param name="type">Module type</param>
/// <returns>Module info address; NULL if not found</returns>
PMODULE_DATA BBLookupMappedModule( IN PLIST_ENTRY pList, IN PUNICODE_STRING pName, IN ModType type );

/// <summary>
/// Get memory protection form section characteristics
/// </summary>
/// <param name="characteristics">Characteristics</param>
/// <returns>Memory protection value</returns>
ULONG BBCastSectionProtection( IN ULONG characteristics );

#pragma alloc_text(PAGE, BBMapUserImage)
#pragma alloc_text(PAGE, BBFindOrMapModule)
#pragma alloc_text(PAGE, BBResolveImageRefs)
#pragma alloc_text(PAGE, BBResolveApiSet)
#pragma alloc_text(PAGE, BBResolveSxS)
#pragma alloc_text(PAGE, BBResolveImagePath)
#pragma alloc_text(PAGE, BBCallInitRoutine)
#pragma alloc_text(PAGE, BBPrepareACTX)
#pragma alloc_text(PAGE, BBLoadLocalImage)
#pragma alloc_text(PAGE, BBCreateWorkerThread)
#pragma alloc_text(PAGE, BBCallRoutine)
#pragma alloc_text(PAGE, BBLookupMappedModule)
#pragma alloc_text(PAGE, BBCastSectionProtection)

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
        status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &context.userMem, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
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

        PVOID pNtdll = BBGetUserModule( context.pProcess, &ustrNtdll, PsGetProcessWow64Process( pProcess ) != NULL );

        context.pSetEvent = BBGetModuleExport( pNtdll, "NtSetEvent", pProcess, NULL );
        context.pLoadImage = BBGetModuleExport( pNtdll, "LdrLoadDll", pProcess, NULL );
    }

    // Prepare Activation context
    if (!(flags & KNoSxS))
        status = BBPrepareACTX( path, &context );

    // Map module
    if (NT_SUCCESS( status ))
        status = BBFindOrMapModule( pProcess, path, systemBuffer, size, asImage, flags, &context, pImage );

    // Enable exceptions
    if (NT_SUCCESS( status ) && !(flags & KNoExceptions))
    {
        // TODO: implement native x64 support
        if (PsGetProcessWow64Process( pProcess ))
        {
            if (dynData.KExecOpt != 0)
            {
                PKEXECUTE_OPTIONS pExecOpt = (PKEXECUTE_OPTIONS)((PUCHAR)pProcess + dynData.KExecOpt);

                // Reset all flags
                pExecOpt->ExecuteOptions = 0;

                pExecOpt->Flags.ExecuteEnable = 1;
                pExecOpt->Flags.ImageDispatchEnable = 1;
                pExecOpt->Flags.ExecuteDispatchEnable = 1;
                pExecOpt->Flags.Permanent = 1;
            }
            else
                DPRINT( "BlackBone: %s: Invalid KEXECUTE_OPTIONS offset\n", __FUNCTION__ );
        }
    }

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
    if (context.userMem)
    {
        SIZE_T size = 0;
        ZwFreeVirtualMemory( ZwCurrentProcess(), &context.userMem, &size, MEM_RELEASE );
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
        status = BBResolveImageRefs( pLocalImage->baseAddress, FALSE, pProcess, IMAGE32( pHeaders ), pContext );

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
/// <param name="systemImage">If TRUE - image is driver</param>
/// <param name="pProcess">Target process</param>
/// <param name="wow64Image">Iamge is 32bit image</param>
/// <param name="PMMAP_CONTEXT">Manual map context</param>
/// <returns>Status code</returns>
NTSTATUS BBResolveImageRefs(
    IN PVOID pImageBase,
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
        UNICODE_STRING ustrImpDll = { 0 };
        UNICODE_STRING resolved = { 0 };
        UNICODE_STRING resolvedName = { 0 };
        ANSI_STRING strImpDll = { 0 };
        ULONG IAT_Index = 0;
        PCCHAR impFunc = NULL;
        PKLDR_DATA_TABLE_ENTRY pModule = NULL;

        RtlInitAnsiString( &strImpDll, (PCHAR)pImageBase + pImportTbl->Name );
        RtlAnsiStringToUnicodeString( &ustrImpDll, &strImpDll, TRUE );

        // Resolve image name
        BBResolveImagePath( pContext, pProcess, 0, &ustrImpDll, NULL, &resolved );
        BBStripPath( &resolved, &resolvedName );

        // Get import module
        pModule = systemImage ? BBGetSystemModule( &ustrImpDll, NULL ) : BBGetUserModule( pProcess, &resolved, wow64Image );

        // Load missing import
        if (!pModule && !systemImage)
        {
            RtlInitEmptyUnicodeString( &pContext->userMem->ustr, pContext->userMem->buffer, sizeof( pContext->userMem->buffer ) );
            RtlCopyUnicodeString( &pContext->userMem->ustr, &resolved );

            BBCallRoutine( pContext, pContext->pLoadImage, 4, NULL, NULL, &pContext->userMem->ustr, &pContext->userMem->ptr );
            pModule = pContext->userMem->ptr;
        }

        // Failed to load
        if (!pModule)
        {
            DPRINT( "BlackBone: %s: Failed to resolve import module: '%wZ'\n", __FUNCTION__, ustrImpDll );
            RtlFreeUnicodeString( &ustrImpDll );
            RtlFreeUnicodeString( &resolved );

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

            pFunc = BBGetModuleExport( systemImage ? pModule->DllBase : pModule, impFunc, pContext->pProcess, &resolved );

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
        RtlFreeUnicodeString( &resolved );
    }

    return status;
}


/// <summary>
/// Try to resolve image via API SET map
/// </summary>
/// <param name="pProcess">Target process. Must be run in the context of this process</param>
/// <param name="name">Name to resolve</param>
/// <param name="baseImage">Parent image name</param>
/// <param name="resolved">Resolved name if any</param>
/// <returns>Status code</returns>
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

    if (memcmp( name->Buffer, L"ext-ms-", 7 ) == 0)
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

/// <summary>
///Try to resolve image via SxS isolation
/// </summary>
/// <param name="pContext">Loader context.</param>
/// <param name="name">Name to resolve</param>
/// <param name="resolved">Resolved name if any</param>
/// <returns>Status code</returns>
NTSTATUS BBResolveSxS(
    IN PMMAP_CONTEXT pContext,
    IN PUNICODE_STRING name,
    OUT PUNICODE_STRING resolved
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    UNICODE_STRING ustrNtdll = { 0 };
    BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

    typedef struct _STRIBG_BUF
    {
        union
        {
            UNICODE_STRING name1;
            UNICODE_STRING32 name132;
        };
        union
        {
            UNICODE_STRING name2;
            UNICODE_STRING32 name232;
        };
        union
        {
            UNICODE_STRING origName;
            UNICODE_STRING32 origName32;
        };
        union
        {
            PUNICODE_STRING pResolved;
            PUNICODE_STRING32 pResolved32;
        };
        wchar_t origBuf[0x100];
        wchar_t staticBuf[0x200];
    } STRIBG_BUF, *PSTRIBG_BUF;

    PSTRIBG_BUF pStringBuf = (PSTRIBG_BUF)pContext->userMem->buffer;

    UNREFERENCED_PARAMETER( name );
    UNREFERENCED_PARAMETER( resolved );

    RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );

    PVOID hNtdll = BBGetUserModule( pContext->pProcess, &ustrNtdll, wow64 );
    PVOID pQueryName = BBGetModuleExport( hNtdll, "RtlDosApplyFileIsolationRedirection_Ustr", pContext->pProcess, NULL );

    if (pQueryName == NULL)
    {
        DPRINT( "BlackBone: %s: Failed to get RtlDosApplyFileIsolationRedirection_Ustr\n", __FUNCTION__ );
        return STATUS_NOT_FOUND;
    }

    RtlZeroMemory( pStringBuf->origBuf, sizeof( pStringBuf->origBuf ) );
    RtlZeroMemory( pStringBuf->staticBuf, sizeof( pStringBuf->staticBuf ) );

    // Fill params
    memcpy( pStringBuf->origBuf, name->Buffer, name->Length );
    RtlInitUnicodeString( &pStringBuf->origName, pStringBuf->origBuf );
    RtlInitEmptyUnicodeString( &pStringBuf->name1, pStringBuf->staticBuf, sizeof( pStringBuf->staticBuf ) );
    RtlInitEmptyUnicodeString( &pStringBuf->name2, NULL, 0 );

    // RtlDosApplyFileIsolationRedirection_Ustr
    status = BBCallRoutine(
        pContext, pQueryName, 9,
        (PVOID)TRUE, &pStringBuf->origName, NULL,
        &pStringBuf->name1, &pStringBuf->name2, &pStringBuf->pResolved,
        NULL, NULL, NULL
        );

    if (NT_SUCCESS( status ))
    {
        if (NT_SUCCESS( pContext->userMem->status ))
            RtlDowncaseUnicodeString( resolved, pStringBuf->pResolved, TRUE );
        // TODO: name2 cleanup
    }

    return NT_SUCCESS( status ) ? pContext->userMem->status : status;
}

/// <summary>
/// Resolve image name to fully qualified path
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="pProcess">Target process. Must be running in the context of this process</param>
/// <param name="flags">Flags</param>
/// <param name="path">Image name to resolve</param>
/// <param name="baseImage">Base image name for API SET translation</param>
/// <param name="resolved">Resolved image path</param>
/// <returns>Status code</returns>
NTSTATUS BBResolveImagePath(
    IN PMMAP_CONTEXT pContext,
    IN PEPROCESS pProcess,
    IN ULONG flags,
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
        DPRINT( "BlackBone: %s: Resolved via API SET %wZ -> %wZ\n", __FUNCTION__, path, resolved );
        RtlFreeUnicodeString( &pathLow );
        return STATUS_SUCCESS;
    }

    // Api schema only
    if (flags & 1)
        goto skip;

    // SxS
    if (pContext && NT_SUCCESS( BBResolveSxS( pContext, &filename, resolved ) ))
    {
        DPRINT( "BlackBone: %s: Resolved via SxS %wZ -> %wZ\n", __FUNCTION__, path, resolved );
        RtlFreeUnicodeString( &pathLow );
        return STATUS_SUCCESS;
    }


    // Nothing found
    //DPRINT( "BlackBone: %s: Failed to resolve %wZ\n", __FUNCTION__, path );
skip:
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

            /*UNICODE_STRING ustrNtdll = { 0 };
            BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

            RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );

            PVOID hNtdll = BBGetUserModule( pContext->pProcess, &ustrNtdll, wow64, NULL );

            PVOID pActivateCtx = BBGetModuleExport(hNtdll, "RtlActivateActivationContext", pContext->pProcess, NULL);
            PVOID pDeactivateCtx = BBGetModuleExport( hNtdll, "RtlDeactivateActivationContext", pContext->pProcess, NULL );

            ULONG ofst = GenPrologue64( pContext->userMem->code );

            // RtlActivateActivationContext
            ofst += GenCall64(
            pContext->userMem->code + ofst, pActivateCtx, 3,
            NULL, pContext->userMem->hCTX, &pContext->userMem->hCookie
            );

            // DllMain
            ofst += GenCall64(
            pContext->userMem->code + ofst, entrypoint, 3,
            pEntry->baseAddress, (PVOID)1, NULL
            );

            // RtlDeactivateActivationContext
            ofst += GenCall64(
            pContext->userMem->code + ofst, pDeactivateCtx, 2,
            pEntry->baseAddress, NULL, (PVOID)pContext->userMem->hCookie
            );

            ofst += GenSync64( pContext->userMem->code + ofst, &pContext->userMem->status, pContext->pSetEvent, pContext->hSync );
            ofst += GenEpilogue64( pContext->userMem->code + ofst );

            KeResetEvent( pContext->pSync );
            NTSTATUS status = BBQueueUserApc( pContext->pWorker, pContext->userMem->code, NULL, NULL, NULL, FALSE );
            if (NT_SUCCESS( status ))
            {
            LARGE_INTEGER timeout = { 0 };
            timeout.QuadPart = -(10ll * 10 * 1000 * 1000);   // 10s

            status = KeWaitForSingleObject( pContext->pSync, Executive, UserMode, TRUE, &timeout );

            timeout.QuadPart = -(1ll * 10 * 1000);          // 1ms
            KeDelayExecutionThread( KernelMode, TRUE, &timeout );
            }*/
        }

        if (pEntry->flags & KWipeHeader)
            RtlZeroMemory( &pEntry->baseAddress, HEADER_VAL_T( pHeaders, SizeOfHeaders ) );

        pEntry->initialized = TRUE;
    }
}

/// <summary>
/// Create activation context
/// </summary>
/// <param name="pPath">Image path</param>
/// <param name="pContext">Loader context</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareACTX( IN PUNICODE_STRING pPath, IN PMMAP_CONTEXT pContext )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING ustrKernel = { 0 }, ustrNtdll = { 0 };
    BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

    RtlUnicodeStringInit( &ustrKernel, L"kernel32.dll" );
    RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );

    PVOID pCreateActCtxW = BBGetModuleExport(
        BBGetUserModule( pContext->pProcess, &ustrKernel, wow64 ),
        "CreateActCtxW", pContext->pProcess, NULL
        );

    if (!pCreateActCtxW)
    {
        DPRINT( "BlackBone: %s: Failed to get CreateActCtxW address\n", __FUNCTION__ );
        status = STATUS_NOT_FOUND;
    }

    PVOID pActivateCtx = BBGetModuleExport(
        BBGetUserModule( pContext->pProcess, &ustrNtdll, wow64 ),
        "RtlActivateActivationContext", pContext->pProcess, NULL
        );

    if (!pActivateCtx)
    {
        DPRINT( "BlackBone: %s: Failed to get RtlActivateActivationContext address\n", __FUNCTION__ );
        status = STATUS_NOT_FOUND;
    }

    // Generate activation context
    if (NT_SUCCESS( status ))
    {
        RtlZeroMemory( pContext->userMem->buffer, sizeof( pContext->userMem->buffer ) );
        memcpy( pContext->userMem->buffer, pPath->Buffer, pPath->Length );

        if (wow64 != FALSE)
        {
            RtlZeroMemory( &pContext->userMem->actx32, sizeof( pContext->userMem->actx32 ) );

            pContext->userMem->actx32.cbSize = sizeof( ACTCTXW32 );
            pContext->userMem->actx32.lpSource = (ULONG)pContext->userMem->buffer + 4 * sizeof( wchar_t );
            pContext->userMem->actx32.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
            pContext->userMem->actx32.lpResourceName = (ULONG)MAKEINTRESOURCEW( 1 );

            BBCallRoutine( pContext, pCreateActCtxW, 1, &pContext->userMem->actx32 );
        }
        else
        {
            RtlZeroMemory( &pContext->userMem->actx, sizeof( pContext->userMem->actx ) );

            pContext->userMem->actx.cbSize = sizeof( ACTCTXW );
            pContext->userMem->actx.lpSource = pContext->userMem->buffer + 4;
            pContext->userMem->actx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
            pContext->userMem->actx.lpResourceName = MAKEINTRESOURCEW( 1 );

            BBCallRoutine( pContext, pCreateActCtxW, 1, &pContext->userMem->actx );
        }

        if (pContext->userMem->retVal32 != 0xFFFFFFFF)
        {
            pContext->userMem->hCTX = wow64 ? (PVOID)pContext->userMem->retVal32 : pContext->userMem->retVal;
        }
        else
        {
            DPRINT( "BlackBone: %s: CreateActCtxW failed\n", __FUNCTION__ );
            status = STATUS_SXS_CANT_GEN_ACTCTX;
        }
    }

    // Activate context
    if (NT_SUCCESS( status ))
    {
        BBCallRoutine( pContext, pActivateCtx, 3, NULL, pContext->userMem->hCTX, &pContext->userMem->hCookie );
        if (!NT_SUCCESS( pContext->userMem->status ))
        {
            DPRINT( "BlackBone: %s: RtlActivateActivationContext failed with code 0x%X\n", __FUNCTION__, pContext->userMem->status );
            status = pContext->userMem->status;
        }
    }

    return status;
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
    if (pContext == NULL)
        return STATUS_INVALID_PARAMETER;

    pContext->pWorkerBuf = NULL;
    status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pContext->pWorkerBuf, 0, &codeSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (NT_SUCCESS( status ))
    {
        PUCHAR pBuf = pContext->pWorkerBuf;
        UNICODE_STRING ustrNtdll;
        BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

        RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );

        PVOID pNtDelayExec = BBGetModuleExport(
            BBGetUserModule( pContext->pProcess, &ustrNtdll, wow64 ),
            "NtDelayExecution", pContext->pProcess, NULL
            );

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
/// Call arbitrary function
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="pRoutine">Routine to call.</param>
/// <param name="argc">Number of arguments.</param>
/// <param name="...">Arguments</param>
/// <returns>Status code</returns>
NTSTATUS BBCallRoutine( IN PMMAP_CONTEXT pContext, IN PVOID pRoutine, IN INT argc, ... )
{
    NTSTATUS status = STATUS_SUCCESS;
    va_list vl;

    va_start( vl, argc );
    ULONG ofst = GenPrologue64( pContext->userMem->code );
    ofst += GenCall64V( pContext->userMem->code + ofst, pRoutine, argc, vl );
    ofst += GenSync64( pContext->userMem->code + ofst, &pContext->userMem->status, pContext->pSetEvent, pContext->hSync );
    ofst += GenEpilogue64( pContext->userMem->code + ofst );

    KeResetEvent( pContext->pSync );
    status = BBQueueUserApc( pContext->pWorker, pContext->userMem->code, NULL, NULL, NULL, FALSE );
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