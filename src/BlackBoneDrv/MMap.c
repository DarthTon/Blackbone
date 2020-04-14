#include "Loader.h"
#include "Private.h"
#include "Routines.h"
#include "BlackBoneDef.h"
#include "Utils.h"
#include "apiset.h"
#include <ntstrsafe.h>

#define IMAGE32(hdr) (hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
#define IMAGE64(hdr) (hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)

#define HEADER_VAL_T(hdr, val) (IMAGE64(hdr) ? ((PIMAGE_NT_HEADERS64)hdr)->OptionalHeader.val : ((PIMAGE_NT_HEADERS32)hdr)->OptionalHeader.val)
#define THUNK_VAL_T(hdr, ptr, val) (IMAGE64(hdr) ? ((PIMAGE_THUNK_DATA64)ptr)->val : ((PIMAGE_THUNK_DATA32)ptr)->val)
#define TLS_VAL_T(hdr, ptr, val) (IMAGE64(hdr) ? ((PIMAGE_TLS_DIRECTORY64)ptr)->val : ((PIMAGE_TLS_DIRECTORY32)ptr)->val)
#define CFG_DIR_VAL_T(hdr, dir, val) (IMAGE64(hdr) ? ((PIMAGE_LOAD_CONFIG_DIRECTORY64)dir)->val : ((PIMAGE_LOAD_CONFIG_DIRECTORY32)dir)->val)

#if defined (_WIN10_)
typedef PAPI_SET_VALUE_ENTRY_10     PAPISET_VALUE_ENTRY;
typedef PAPI_SET_VALUE_ARRAY_10     PAPISET_VALUE_ARRAY;
typedef PAPI_SET_NAMESPACE_ENTRY_10 PAPISET_NAMESPACE_ENTRY;
typedef PAPI_SET_NAMESPACE_ARRAY_10 PAPISET_NAMESPACE_ARRAY;
#elif defined (_WIN81_)
typedef PAPI_SET_VALUE_ENTRY        PAPISET_VALUE_ENTRY;
typedef PAPI_SET_VALUE_ARRAY        PAPISET_VALUE_ARRAY;
typedef PAPI_SET_NAMESPACE_ENTRY    PAPISET_NAMESPACE_ENTRY;
typedef PAPI_SET_NAMESPACE_ARRAY    PAPISET_NAMESPACE_ARRAY;
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
    IN KMmapFlags flags,
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
/// <param name="noTLS">If TRUE - TLS callbacks will no be invoked</param>
/// <returns>Status code</returns>
NTSTATUS BBCallInitializers( IN PMMAP_CONTEXT pContext, IN BOOLEAN noTLS );

/// <summary>
/// Call TLS callbacks
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="imageBase">Target image base</param>
void BBCallTlsInitializers( IN PMMAP_CONTEXT pContext, IN PVOID imageBase );

/// <summary>
/// Create activation context
/// </summary>
/// <param name="pPath">Manifest path</param>
/// <param name="asImage">Target file is a PE image</param>
/// <param name="manifestID">Manifest ID</param>
/// <param name="pContext">Loader context</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareACTX( IN PUNICODE_STRING pPath, IN BOOLEAN asImage, IN LONG manifestID, IN PMMAP_CONTEXT pContext );

/// <summary>
/// Create exception table for x64 image
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="imageBase">Image base</param>
/// <returns>Status code</returns>
NTSTATUS BBCreateExceptionTable64( IN PMMAP_CONTEXT pContext, IN PVOID imageBase );

/// <summary>
/// Setup image security cookie
/// </summary>
/// <param name="imageBase">Image base</param>
/// <returns>Status code</returns>
NTSTATUS BBCreateCookie( IN PVOID imageBase );

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
/// Find suitable thread to be used as worker for user-mode calls
/// </summary>
/// <param name="pContext">Map context</param>
/// <returns>Status code</returns>
NTSTATUS BBFindWokerThread( IN PMMAP_CONTEXT pContext );

/// <summary>
/// Call arbitrary function
/// </summary>
/// <param name="newThread">Perform call in a separate thread</param>
/// <param name="pContext">Loader context</param>
/// <param name="pRoutine">Routine to call.</param>
/// <param name="argc">Number of arguments.</param>
/// <param name="...">Arguments</param>
/// <returns>Status code</returns>
NTSTATUS BBCallRoutine( IN BOOLEAN newThread, IN PMMAP_CONTEXT pContext, IN PVOID pRoutine, IN INT argc, ... );

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
/// <param name="noDEP">Set to TRUE if DEP is disabled. This will omit creation of executable pages</param>
/// <returns>Memory protection value</returns>
ULONG BBCastSectionProtection( IN ULONG characteristics, IN BOOLEAN noDEP );

/// <summary>
/// Get image manifest data
/// </summary>
/// <param name="pImageBase">Image base</param>
/// <param name="pSize">Manifest size</param>
/// <param name="pID">Manifest ID</param>
/// <returns>Manifest data pointer or NULL if no manifest</returns>
PVOID BBImageManifest( IN PVOID pImageBase, OUT PULONG_PTR pSize, OUT PLONG pID );

/// <summary>
/// Translate RVA address to VA
/// </summary>
/// <param name="rva">RVA</param>
/// <param name="MappedAsImage">Image is mapped using image memory layout</param>
/// <param name="imageBase">Image base</param>
/// <returns>Translated address</returns>
PVOID BBRvaToVa( IN ULONG rva, IN BOOLEAN MappedAsImage, IN PVOID imageBase );

#pragma alloc_text(PAGE, BBMapUserImage)
#pragma alloc_text(PAGE, BBFindOrMapModule)
#pragma alloc_text(PAGE, BBResolveImageRefs)
#pragma alloc_text(PAGE, BBResolveApiSet)
#pragma alloc_text(PAGE, BBResolveSxS)
#pragma alloc_text(PAGE, BBResolveImagePath)
#pragma alloc_text(PAGE, BBCallInitializers)
#pragma alloc_text(PAGE, BBPrepareACTX)
#pragma alloc_text(PAGE, BBCreateExceptionTable64)
#pragma alloc_text(PAGE, BBCreateCookie)
#pragma alloc_text(PAGE, BBLoadLocalImage)
#pragma alloc_text(PAGE, BBCreateWorkerThread)
#pragma alloc_text(PAGE, BBCallRoutine)
#pragma alloc_text(PAGE, BBLookupMappedModule)
#pragma alloc_text(PAGE, BBCastSectionProtection)
#pragma alloc_text(PAGE, BBImageManifest)
#pragma alloc_text(PAGE, BBRvaToVa)

/// <summary>
/// Map new user module
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="path">Image path</param>
/// <param name="buffer">Image buffer</param>
/// <param name="size">Image buffer size</param>
/// <param name="asImage">Buffer has image memory layout</param>
/// <param name="flags">Mapping flags</param>
/// <param name="initRVA">Init routine RVA</param>
/// <param name="initArg">Init argument</param>
/// <param name="pImage">Mapped image data</param>
/// <returns>Status code</returns>
NTSTATUS BBMapUserImage(
    IN PEPROCESS pProcess,
    IN PUNICODE_STRING path,
    IN PVOID buffer, IN ULONG_PTR size,
    IN BOOLEAN asImage,
    IN KMmapFlags flags,
    IN ULONG initRVA,
    IN PWCH initArg,
    OUT PMODULE_DATA pImage
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    MMAP_CONTEXT context = { 0 };

    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
    {
        DPRINT( "BlackBone: %s: No process provided\n", __FUNCTION__ );
        return STATUS_INVALID_PARAMETER;
    }

    context.pProcess = pProcess;
    InitializeListHead( &context.modules );

    DPRINT( "BlackBone: %s: Mapping image '%wZ' with flags 0x%X\n", __FUNCTION__, path, flags );

    // Create or find worker thread
    context.noThreads = (flags & KNoThreads) != 0;
    status = context.noThreads ? BBFindWokerThread( &context ) : BBCreateWorkerThread( &context );
    if (NT_SUCCESS( status ))
    {
        SIZE_T mapSize = 0x2000;
        status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &context.userMem, 0, &mapSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
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

    // Map module
    if (NT_SUCCESS( status ))
        status = BBFindOrMapModule( pProcess, path, buffer, size, asImage, flags, &context, pImage );

    // Enable exceptions for WOW64 process
    if (NT_SUCCESS( status ) && !(flags & KNoExceptions))
    {
        if (dynData.KExecOpt != 0)
        {
            PKEXECUTE_OPTIONS pExecOpt = (PKEXECUTE_OPTIONS)((PUCHAR)pProcess + dynData.KExecOpt);

            pExecOpt->Flags.ImageDispatchEnable = 1;
            pExecOpt->Flags.ExecuteDispatchEnable = 1;
            pExecOpt->Flags.Permanent = 1;
        }
        else
            DPRINT( "BlackBone: %s: Invalid KEXECUTE_OPTIONS offset\n", __FUNCTION__ );
    }

    // Rebase process
    if (NT_SUCCESS( status ) && flags & KRebaseProcess)
    {
        PPEB pPeb = (PPEB)PsGetProcessPeb( pProcess );
        PPEB32 pPeb32 = (PPEB32)PsGetProcessWow64Process( pProcess );

        pPeb->ImageBaseAddress = pImage->baseAddress;
        if (pPeb32)
            pPeb32->ImageBaseAddress = (ULONG)(ULONG_PTR)pImage->baseAddress;
    }

    // Run module initializers
    if (NT_SUCCESS( status ))
    {
        __try{
            status = BBCallInitializers( &context, (flags & KNoTLS) != 0 );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            DPRINT( "BlackBone: %s: Exception during initialization phase.Exception code 0x%X\n", __FUNCTION__, status );
        }
    }

    // Run user initializer
    if (NT_SUCCESS( status ) && initRVA != 0)
    {
        PVOID pInitRoutine = (PUCHAR)pImage->baseAddress + initRVA;
        memcpy( context.userMem->buffer, initArg, 512 * sizeof( WCHAR ) );
        BBCallRoutine( FALSE, &context, pInitRoutine, 1, context.userMem->buffer );
    }

    //
    // Cleanup
    //

    // Event
    if (context.pSync)
        ObDereferenceObject( context.pSync );

    if (context.hSync && !BBCheckProcessTermination( PsGetCurrentProcess() ))
        ZwClose( context.hSync );

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
        SIZE_T mapSize = 0;
        ZwFreeVirtualMemory( ZwCurrentProcess(), &context.pWorkerBuf, &mapSize, MEM_RELEASE );
    }

    // Code buffer
    if (context.userMem)
    {
        SIZE_T mapSize = 0;
        ZwFreeVirtualMemory( ZwCurrentProcess(), &context.userMem, &mapSize, MEM_RELEASE );
    }

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
    IN KMmapFlags flags,
    IN PMMAP_CONTEXT pContext,
    OUT PMODULE_DATA pImage
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMODULE_DATA pLocalImage = NULL;
    PIMAGE_NT_HEADERS pHeaders = NULL;
    BOOLEAN wow64 = PsGetProcessWow64Process( pProcess ) != NULL;

    UNREFERENCED_PARAMETER( size );

    // Allocate image context record
    pLocalImage = ExAllocatePoolWithTag( PagedPool, sizeof( MODULE_DATA ), BB_POOL_TAG );
    RtlZeroMemory( pLocalImage, sizeof( MODULE_DATA ) );

    BBSafeInitString( &pLocalImage->fullPath, path );
    BBStripPath( &pLocalImage->fullPath, &pLocalImage->name );
    pLocalImage->flags = flags;

    // Check if image is already loaded
    pLocalImage->type = wow64 ? mt_mod32 : mt_mod64;
    PMODULE_DATA pFoundMod = BBLookupMappedModule( &pContext->modules, &pLocalImage->name, pLocalImage->type );
    if (pFoundMod != NULL)
    {
        RtlFreeUnicodeString( &pLocalImage->fullPath );
        ExFreePoolWithTag( pLocalImage, BB_POOL_TAG );

        *pImage = *pFoundMod;
        return STATUS_SUCCESS;
    }

    DPRINT( "BlackBone: %s: Loading image %wZ\n", __FUNCTION__, &pLocalImage->fullPath );

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
 
    // Prepare activation context
    if (!(flags & KNoSxS) && NT_SUCCESS( status ))
    {
        LONG manifestID = 0;
        ULONG_PTR manifestSize = 0;
        PVOID pManifest = BBImageManifest( pLocalImage->localBase, &manifestSize, &manifestID );
        if (pManifest && manifestID != 0)
        {
            // Save manifest into tmp file
            if (buffer != NULL)
            {
                UNICODE_STRING manifestPathNt = { 0 };
                UNICODE_STRING manifestPath = { 0 };
                NTSTATUS status2 = STATUS_SUCCESS;
                HANDLE hFile = NULL;
                OBJECT_ATTRIBUTES obAttr = { 0 };
                IO_STATUS_BLOCK statusBlock = { 0 };

                BBSafeAllocateString( &manifestPath, 512 );
                RtlInitUnicodeString( &manifestPathNt, L"\\SystemRoot\\Temp\\BBImage.manifest" );
                InitializeObjectAttributes( &obAttr, &manifestPathNt, OBJ_KERNEL_HANDLE, NULL, NULL );
                
                status2 = ZwCreateFile(
                    &hFile, FILE_WRITE_DATA | DELETE | SYNCHRONIZE, &obAttr,
                    &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ, FILE_SUPERSEDE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
                    );

                if (NT_SUCCESS( status2 ))
                {
                    status2 = ZwWriteFile( hFile, NULL, NULL, NULL, &statusBlock, pManifest, (ULONG)manifestSize, NULL, NULL );
                    ZwClose( hFile );
                }                   

                if (NT_SUCCESS( status2 ))
                {
                    // Cast path
                    RtlUnicodeStringPrintf( &manifestPath, L"%ls\\%ls", SharedUserData->NtSystemRoot, L"\\Temp\\BBImage.manifest" );
                    BBPrepareACTX( &manifestPath, FALSE, manifestID, pContext );
                }
                else
                    DPRINT( "BlackBone: %s: Failed to create temporary manifest. Status code: 0x%X\n", __FUNCTION__, status2 );

                ZwDeleteFile( &obAttr );
                RtlFreeUnicodeString( &manifestPath );
            }
            else
                BBPrepareACTX( path, TRUE, manifestID, pContext );
        }
    }


    // Allocate memory in target process
    if (NT_SUCCESS( status ))
    {
        // Allocate physical memory
        if (flags & KHideVAD)
        {
            ALLOCATE_FREE_MEMORY request = { 0 };
            ALLOCATE_FREE_MEMORY_RESULT mapResult = { 0 };

            request.pid = (ULONG)(ULONG_PTR)PsGetProcessId( pProcess );
            request.allocate = TRUE;
            request.physical = TRUE;
            request.protection = PAGE_EXECUTE_READWRITE;
            request.size = HEADER_VAL_T( pHeaders, SizeOfImage );

            status = BBAllocateFreePhysical( pProcess, &request, &mapResult );
            if (NT_SUCCESS( status ))
            {
                pLocalImage->baseAddress = (PUCHAR)mapResult.address;
                pLocalImage->size = mapResult.size;
            }
        }
        else
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
                 pSection->SizeOfRawData == 0)
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

    InsertHeadList( &pContext->modules, &pLocalImage->link );

    // Resolve imports
    if (NT_SUCCESS( status ))
        status = BBResolveImageRefs( pLocalImage->baseAddress, FALSE, pProcess, IMAGE32( pHeaders ), pContext, flags );

    // Set image memory protection
    if (NT_SUCCESS( status ))
    {
        if (flags & KHideVAD)
        {
            status = BBProtectVAD( pProcess, (ULONG_PTR)pLocalImage->baseAddress, MM_ZERO_ACCESS );
        }
        else
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
                ULONG prot = BBCastSectionProtection( pSection->Characteristics, FALSE /*!(flags & KNoExceptions) && wow64*/ );
                PUCHAR pAddr = pLocalImage->baseAddress + pSection->VirtualAddress;
                tmpSize = pSection->Misc.VirtualSize;

                // Decommit pages with NO_ACCESS protection
                if (prot != PAGE_NOACCESS)
                    status |= ZwProtectVirtualMemory( ZwCurrentProcess(), &pAddr, &tmpSize, prot, NULL );
                else
                    ZwFreeVirtualMemory( ZwCurrentProcess(), &pAddr, &tmpSize, MEM_DECOMMIT );
            }
        }
    }

    // Setup security cookie
    if (NT_SUCCESS( status ))
        BBCreateCookie( pLocalImage->baseAddress );

    // Enable SEH support for x64 image
    if (!(flags & KNoExceptions) && NT_SUCCESS( status ) && wow64 == FALSE)
        BBCreateExceptionTable64( pContext, pLocalImage->baseAddress );

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
    }
    else if (pLocalImage)
    {
        // Remove entry from list
        RemoveEntryList( &pLocalImage->link );

        // Delete remote image
        if (pLocalImage->baseAddress)
        {
            if (flags & KHideVAD)
            {
                ALLOCATE_FREE_MEMORY request = { 0 };
                ALLOCATE_FREE_MEMORY_RESULT mapResult = { 0 };

                request.pid = (ULONG)(ULONG_PTR)PsGetProcessId( pProcess );
                request.allocate = FALSE;
                request.physical = TRUE;

                BBAllocateFreePhysical( pProcess, &request, &mapResult );
            } 
            else
            {
                SIZE_T tmpSize = 0;
                ZwFreeVirtualMemory( ZwCurrentProcess(), &pLocalImage->baseAddress, &tmpSize, MEM_RELEASE );
            }
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
    IN PMMAP_CONTEXT pContext,
    IN KMmapFlags flags
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG impSize = 0;
    PIMAGE_NT_HEADERS pHeader = RtlImageNtHeader( pImageBase );
    PIMAGE_IMPORT_DESCRIPTOR pImportTbl = RtlImageDirectoryEntryToData( pImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &impSize );

    // No import libs
    if (pImportTbl == NULL)
        return STATUS_SUCCESS;

    for (; pImportTbl->Name && NT_SUCCESS( status ); ++pImportTbl)
    {
        PVOID pThunk = ((PUCHAR)pImageBase + (pImportTbl->OriginalFirstThunk ? pImportTbl->OriginalFirstThunk : pImportTbl->FirstThunk));
        UNICODE_STRING ustrImpDll = { 0 };
        UNICODE_STRING resolved = { 0 };
        UNICODE_STRING resolvedName = { 0 };
        ANSI_STRING strImpDll = { 0 };
        ULONG IAT_Index = 0;
        PCCHAR impFunc = NULL;
        union
        {
            PVOID address;
            PKLDR_DATA_TABLE_ENTRY ldrEntry;
        } pModule = { 0 };

        RtlInitAnsiString( &strImpDll, (PCHAR)pImageBase + pImportTbl->Name );
        RtlAnsiStringToUnicodeString( &ustrImpDll, &strImpDll, TRUE );

        // Resolve image name
        if(!systemImage)
        {
            status = BBResolveImagePath( pContext, pProcess, 0, &ustrImpDll, NULL, &resolved );
            BBStripPath( &resolved, &resolvedName );

            // Something went terribly wrong
            if (status == STATUS_UNHANDLED_EXCEPTION)
            {
                RtlFreeUnicodeString( &ustrImpDll );
                RtlFreeUnicodeString( &resolved );

                return STATUS_NOT_FOUND;
            }
        }
        else
        {
            BBSafeInitString( &resolved, &ustrImpDll );
        }

        // Get import module
        pModule.address = systemImage ? BBGetSystemModule( &ustrImpDll, NULL ) : BBGetUserModule( pProcess, &resolvedName, wow64Image );

        // Load missing import
        if (!pModule.address && !systemImage)
        {
            if (flags & KManualImports)
            {
                MODULE_DATA modData = { 0 };
                
                status = BBFindOrMapModule( pProcess, &resolved, NULL, 0, FALSE, flags | KNoSxS, pContext, &modData );
                if (NT_SUCCESS( status ))
                    pModule.address = modData.baseAddress;
            }
            else
            {
                DPRINT( "BlackBone: %s: Loading missing import %wZ\n", __FUNCTION__, &resolved );

                if (IMAGE64( pHeader ))
                {
                    RtlInitEmptyUnicodeString( &pContext->userMem->ustr, pContext->userMem->buffer, sizeof( pContext->userMem->buffer ) );
                    RtlCopyUnicodeString( &pContext->userMem->ustr, &resolvedName );
                }
                else
                {
                    pContext->userMem->ustr32.Buffer = (ULONG)(ULONG_PTR)pContext->userMem->buffer;
                    pContext->userMem->ustr32.MaximumLength = sizeof( pContext->userMem->buffer );
                    pContext->userMem->ustr32.Length = resolvedName.Length;

                    RtlZeroMemory( pContext->userMem->buffer, sizeof( pContext->userMem->buffer ) );
                    memcpy( pContext->userMem->buffer, resolvedName.Buffer, resolvedName.Length );
                }

                // Invoke LdrLoadDll
                pContext->userMem->status = STATUS_SUCCESS;

                // Calling LdrLoadDll in worker thread breaks further APC delivery, so use new thread
                BBCallRoutine( !pContext->noThreads, pContext, pContext->pLoadImage, 4, NULL, NULL, &pContext->userMem->ustr, &pContext->userMem->ptr );
                pModule.address = pContext->userMem->ptr;
                if (!pModule.address)
                    status = pContext->userMem->status;
            }
        }

        // Failed to load
        if (!pModule.address)
        {
            DPRINT( "BlackBone: %s: Failed to load import '%wZ'. Status code: 0x%X\n", __FUNCTION__, ustrImpDll, status );
            RtlFreeUnicodeString( &ustrImpDll );
            RtlFreeUnicodeString( &resolved );

            return STATUS_NOT_FOUND;
        }

        while (THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData ))
        {
            PIMAGE_IMPORT_BY_NAME pAddressTable = (PIMAGE_IMPORT_BY_NAME)((PUCHAR)pImageBase + THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData ));
            PVOID pFunc = NULL;

            // import by name
            if (THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData ) < (IMAGE64( pHeader ) ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) &&
                 pAddressTable->Name[0])
            {
                impFunc = pAddressTable->Name;
            }
            // import by ordinal
            else
            {
                impFunc = (PCCHAR)(THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData ) & 0xFFFF);
            }

            pFunc = BBGetModuleExport(
                systemImage ? pModule.ldrEntry->DllBase : pModule.address,
                impFunc,
                systemImage ?  NULL : pContext->pProcess,
                &resolved 
                );

            // No export found
            if (!pFunc)
            {
                if (THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData ) <  (IMAGE64( pHeader ) ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) && pAddressTable->Name[0])
                    DPRINT( "BlackBone: %s: Failed to resolve import '%wZ' : '%s'\n", __FUNCTION__, ustrImpDll, pAddressTable->Name );
                else
                    DPRINT( "BlackBone: %s: Failed to resolve import '%wZ' : '%d'\n", __FUNCTION__, ustrImpDll, THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData ) & 0xFFFF );

                status = STATUS_NOT_FOUND;
                break;
            }

            if(IMAGE64( pHeader ))
            {
                // Save address to IAT
                if (pImportTbl->FirstThunk)
                    *(PULONG_PTR)((PUCHAR)pImageBase + pImportTbl->FirstThunk + IAT_Index) = (ULONG_PTR)pFunc;
                // Save address to OrigianlFirstThunk
                else
                    *(PULONG_PTR)((PUCHAR)pImageBase + THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData )) = (ULONG_PTR)pFunc;
            }
            else
            {
                // Save address to IAT
                if (pImportTbl->FirstThunk)
                    *(PULONG)((PUCHAR)pImageBase + pImportTbl->FirstThunk + IAT_Index) = (ULONG)(ULONG_PTR)pFunc;
                // Save address to OrigianlFirstThunk
                else
                    *(PULONG)((PUCHAR)pImageBase + THUNK_VAL_T( pHeader, pThunk, u1.AddressOfData )) = (ULONG)(ULONG_PTR)pFunc;
            }

            // Go to next entry
            pThunk = (PUCHAR)pThunk + (IMAGE64( pHeader ) ? sizeof( IMAGE_THUNK_DATA64 ) : sizeof( IMAGE_THUNK_DATA32 ));
            IAT_Index += (IMAGE64( pHeader ) ? sizeof( ULONGLONG ) : sizeof( ULONG ));
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

    // Invalid name
    if ( name == NULL || name->Length < 4 * sizeof(WCHAR) || name->Buffer == NULL ||
        (memcmp( name->Buffer, L"api-", 4 ) != 0 && memcmp( name->Buffer, L"ext-", 4 ) != 0))
        return STATUS_NOT_FOUND;

    // Iterate api set map
    for (ULONG i = 0; i < pApiSetMap->Count; i++)
    {
        PAPISET_NAMESPACE_ENTRY pDescriptor = NULL;
        PAPISET_VALUE_ARRAY pHostArray = NULL;
        wchar_t apiNameBuf[255] = { 0 };
        UNICODE_STRING apiName = { 0 };

#ifdef _WIN10_
        pDescriptor = (PAPISET_NAMESPACE_ENTRY)((PUCHAR)pApiSetMap + pApiSetMap->End + i * sizeof( API_SET_NAMESPACE_ENTRY_10 ));
        pHostArray = (PAPISET_VALUE_ARRAY)((PUCHAR)pApiSetMap + pApiSetMap->Start + sizeof( API_SET_VALUE_ARRAY_10 ) * pDescriptor->Size);

        memcpy( apiNameBuf, (PUCHAR)pApiSetMap + pHostArray->NameOffset, pHostArray->NameLength );
#else
        pDescriptor = pApiSetMap->Array + i;
        memcpy( apiNameBuf, (PUCHAR)pApiSetMap + pDescriptor->NameOffset, pDescriptor->NameLength );
#endif   
        RtlUnicodeStringInit( &apiName, apiNameBuf );

        // Check if this is a target api
        if (BBSafeSearchString( name, &apiName, TRUE ) >= 0)
        {      
            PAPISET_VALUE_ENTRY pHost = NULL;
            wchar_t apiHostNameBuf[255] = { 0 };
            UNICODE_STRING apiHostName = { 0 };

#ifdef _WIN10_
            pHost = (PAPISET_VALUE_ENTRY)((PUCHAR)pApiSetMap + pHostArray->DataOffset);
#else
            pHostArray = (PAPISET_VALUE_ARRAY)((PUCHAR)pApiSetMap + pDescriptor->DataOffset);
            pHost = pHostArray->Array;
#endif
            // Sanity check
            if (pHostArray->Count < 1)
                return STATUS_NOT_FOUND;

            memcpy( apiHostNameBuf, (PUCHAR)pApiSetMap + pHost->ValueOffset, pHost->ValueLength );
            RtlUnicodeStringInit( &apiHostName, apiHostNameBuf );

            // No base name redirection
            if (pHostArray->Count == 1 || baseImage == NULL || baseImage->Buffer[0] == 0)
            {
                BBSafeInitString( resolved, &apiHostName );
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
                    BBSafeInitString( resolved, &apiHostName );
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
            ULONG pResolved32;
        };
        wchar_t origBuf[0x100];
        wchar_t staticBuf[0x200];
    } STRIBG_BUF, *PSTRIBG_BUF;

    PSTRIBG_BUF pStringBuf = (PSTRIBG_BUF)pContext->userMem->buffer;

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
    if (wow64)
    {
        pStringBuf->origName32.Buffer = (ULONG)(ULONG_PTR)pStringBuf->origBuf;
        pStringBuf->origName32.MaximumLength = sizeof( pStringBuf->origBuf );
        pStringBuf->origName32.Length = name->Length;

        pStringBuf->name132.Buffer = (ULONG)(ULONG_PTR)pStringBuf->staticBuf;
        pStringBuf->name132.MaximumLength = sizeof( pStringBuf->staticBuf );
        pStringBuf->name132.Length = 0;

        pStringBuf->name232.Buffer = 0;
        pStringBuf->name232.Length = pStringBuf->name232.MaximumLength =  0;
    }
    else
    {
        RtlInitUnicodeString( &pStringBuf->origName, pStringBuf->origBuf );
        RtlInitEmptyUnicodeString( &pStringBuf->name1, pStringBuf->staticBuf, sizeof( pStringBuf->staticBuf ) );
        RtlInitEmptyUnicodeString( &pStringBuf->name2, NULL, 0 );
    }


    // Prevent some unpredictable shit
    __try
    {
        // RtlDosApplyFileIsolationRedirection_Ustr
        status = BBCallRoutine(
            FALSE, pContext, pQueryName, 9,
            (PVOID)TRUE, &pStringBuf->origName, NULL,
            &pStringBuf->name1, &pStringBuf->name2, &pStringBuf->pResolved,
            NULL, NULL, NULL
            );

        if (NT_SUCCESS( status ) && NT_SUCCESS( pContext->userMem->status ))
        {
            if (wow64)
            {
                ULONG tmp = ((PUNICODE_STRING32)pStringBuf->pResolved32)->Buffer;
                pStringBuf->pResolved = &pStringBuf->name1;
                pStringBuf->pResolved->Buffer = (PWCH)tmp;
            }

            RtlDowncaseUnicodeString( resolved, pStringBuf->pResolved, TRUE );   
            // TODO: name2 cleanup
        }

        return NT_SUCCESS( status ) ? pContext->userMem->status : status;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception. Code: 0x%X\n", __FUNCTION__, GetExceptionCode() );
        return STATUS_UNHANDLED_EXCEPTION;
    }
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
    IN ResolveFlags flags,
    IN PUNICODE_STRING path,
    IN PUNICODE_STRING baseImage,
    OUT PUNICODE_STRING resolved
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING pathLow = { 0 };
    UNICODE_STRING filename = { 0 };
    UNICODE_STRING fullResolved = { 0 };

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
        BBSafeAllocateString( &fullResolved, 512 );
        
        // Perpend system directory
        if (PsGetProcessWow64Process( pProcess ) != NULL)
            RtlUnicodeStringCatString( &fullResolved, L"\\SystemRoot\\syswow64\\" );
        else
            RtlUnicodeStringCatString( &fullResolved, L"\\SystemRoot\\system32\\" );

        RtlUnicodeStringCat( &fullResolved, resolved );
        RtlFreeUnicodeString( resolved );
        RtlFreeUnicodeString( &pathLow );

        //DPRINT( "BlackBone: %s: Resolved image '%wZ' to '%wZ' by ApiSetSchema\n", __FUNCTION__, path, fullResolved );

        *resolved = fullResolved;
        return STATUS_SUCCESS;
    }

    // Api schema only
    if (flags & KApiShemaOnly)
        goto skip;

    if (flags & KSkipSxS)
        goto SkipSxS;

    // SxS
    status = BBResolveSxS( pContext, &filename, resolved );
    if (pContext && NT_SUCCESS( status ))
    {
        BBSafeAllocateString( &fullResolved, 1024 );
        RtlUnicodeStringCatString( &fullResolved, L"\\??\\" );
        RtlUnicodeStringCat( &fullResolved, resolved );

        RtlFreeUnicodeString( resolved );
        RtlFreeUnicodeString( &pathLow );

        *resolved = fullResolved;
        return STATUS_SUCCESS;
    }
    else if (status == STATUS_UNHANDLED_EXCEPTION)
    {
        *resolved = pathLow;
        return status;
    }
    else
        status = STATUS_SUCCESS;
       
SkipSxS:
    BBSafeAllocateString( &fullResolved, 0x400 );

    //
    // Executable directory
    //
    ULONG bytes = 0;
    if (NT_SUCCESS( ZwQueryInformationProcess( ZwCurrentProcess(), ProcessImageFileName, fullResolved.Buffer + 0x100, 0x200, &bytes ) ))
    {
        PUNICODE_STRING pPath = (PUNICODE_STRING)(fullResolved.Buffer + 0x100);
        UNICODE_STRING parentDir = { 0 };
        BBStripFilename( pPath, &parentDir );

        RtlCopyUnicodeString( &fullResolved, &parentDir );
        RtlUnicodeStringCatString( &fullResolved, L"\\" );
        RtlUnicodeStringCat( &fullResolved, &filename );

        if (NT_SUCCESS( BBFileExists( &fullResolved ) ))
        {
            RtlFreeUnicodeString( resolved );
            RtlFreeUnicodeString( &pathLow );

            *resolved = fullResolved;
            return STATUS_SUCCESS;
        }
    }
    
    fullResolved.Length = 0;
    RtlZeroMemory( fullResolved.Buffer, 0x400 );

    //
    // System directory
    //
    if (PsGetProcessWow64Process( pProcess ) != NULL)
        RtlUnicodeStringCatString( &fullResolved, L"\\SystemRoot\\SysWOW64\\" );
    else
        RtlUnicodeStringCatString( &fullResolved, L"\\SystemRoot\\System32\\" );

    RtlUnicodeStringCat( &fullResolved, &filename );
    if (NT_SUCCESS( BBFileExists( &fullResolved ) ))
    {
        RtlFreeUnicodeString( resolved );
        RtlFreeUnicodeString( &pathLow );

        *resolved = fullResolved;
        return STATUS_SUCCESS;
    }
    
    RtlFreeUnicodeString( &fullResolved );
    
    // Nothing found
skip:
    *resolved = pathLow;
    return status;
}

/// <summary>
/// Call module initialization routines
/// </summary>
/// <param name="pContext">Map context</param>
/// <param name="noTLS">If TRUE - TLS callbacks will no be invoked</param>
/// <returns>Status code</returns>
NTSTATUS BBCallInitializers( IN PMMAP_CONTEXT pContext, IN BOOLEAN noTLS )
{
    for (PLIST_ENTRY pListEntry = pContext->modules.Flink; pListEntry != &pContext->modules; pListEntry = pListEntry->Flink)
    {
        PMODULE_DATA pEntry = (PMODULE_DATA)CONTAINING_RECORD( pListEntry, MODULE_DATA, link );
        if (pEntry->initialized)
            continue;

        DPRINT( "BlackBone: %s: Calling '%wZ' initializer\n", __FUNCTION__, pEntry->name );

        PIMAGE_NT_HEADERS pHeaders = RtlImageNtHeader( pEntry->baseAddress );

        if (noTLS == FALSE)
            BBCallTlsInitializers( pContext, pEntry->baseAddress );

        NTSTATUS status = STATUS_SUCCESS;
        if (HEADER_VAL_T( pHeaders, AddressOfEntryPoint ))
        {
            PUCHAR entrypoint = pEntry->baseAddress + HEADER_VAL_T( pHeaders, AddressOfEntryPoint );
            status = BBCallRoutine( FALSE, pContext, entrypoint, 3, pEntry->baseAddress, (PVOID)1, NULL );
        }

        // Check if process is terminating
        if (status != STATUS_SUCCESS && BBCheckProcessTermination( PsGetCurrentProcess() ))
        {
            DPRINT( "BlackBone: %s: Process is terminating, map aborted\n", __FUNCTION__ );
            return STATUS_PROCESS_IS_TERMINATING;
        }

        //
        // Wipe discardable sections
        //
        PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pHeaders + 1);
        if (IMAGE32( pHeaders ))
            pFirstSection = (PIMAGE_SECTION_HEADER)((PIMAGE_NT_HEADERS32)pHeaders + 1);

        for (PIMAGE_SECTION_HEADER pSection = pFirstSection;
              pSection < pFirstSection + pHeaders->FileHeader.NumberOfSections;
              pSection++)
        {
            if (!(pSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE))
                continue;

            PVOID pBase = (PUCHAR)pEntry->baseAddress + pSection->VirtualAddress;
            SIZE_T secSize = pSection->Misc.VirtualSize;

            if (pEntry->flags & KHideVAD)
            {
                RtlZeroMemory( pBase, secSize );
            }
            else
            {
                if (NT_SUCCESS( ZwProtectVirtualMemory( ZwCurrentProcess(), &pBase, &secSize, PAGE_READWRITE, NULL ) ))
                    RtlZeroMemory( pBase, secSize );

                ZwProtectVirtualMemory( ZwCurrentProcess(), &pBase, &secSize, PAGE_NOACCESS, NULL );
                ZwFreeVirtualMemory( ZwCurrentProcess(), &pBase, &secSize, MEM_DECOMMIT );
            }
        }

        // Wipe header
        if (pEntry->flags & KWipeHeader)
        {
            SIZE_T hdrSize = HEADER_VAL_T( pHeaders, SizeOfHeaders );

            if (pEntry->flags & KHideVAD)
            {             
                RtlZeroMemory( pEntry->baseAddress, hdrSize );
            }
            else
            {
                PVOID pBase = pEntry->baseAddress;
                if (NT_SUCCESS( ZwProtectVirtualMemory( ZwCurrentProcess(), &pBase, &hdrSize, PAGE_READWRITE, NULL ) ))
                {
                    RtlZeroMemory( pEntry->baseAddress, hdrSize );
                    ZwProtectVirtualMemory( ZwCurrentProcess(), &pBase, &hdrSize, PAGE_READONLY, NULL );
                }
                else
                    DPRINT( "BlackBone: %s: Failed to wipe image header\n", __FUNCTION__ );
            }
        }

        pEntry->initialized = TRUE;
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Call TLS callbacks
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="imageBase">Target image base</param>
void BBCallTlsInitializers( IN PMMAP_CONTEXT pContext, IN PVOID imageBase )
{
    ULONG size = 0;
    PIMAGE_NT_HEADERS pHeaders = RtlImageNtHeader( imageBase );
    if (!pHeaders)
        return;

    // No TLS directory
    PVOID pTLS = RtlImageDirectoryEntryToData( imageBase, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &size );
    if (!pTLS)
        return;

    // Prepare static TLS a bit
    // Still not compatible with LdrpQueueDeferredTlsData in Win8.1, but I'm tired of this shit
    if (!pContext->tlsInitialized && TLS_VAL_T( pHeaders, pTLS, StartAddressOfRawData ) != 0)
    {
        PVOID pTLSMem = NULL;
        SIZE_T tlsSize = 0x1000;

        if (NT_SUCCESS( ZwAllocateVirtualMemory( ZwCurrentProcess(), &pTLSMem, 0, &tlsSize, MEM_COMMIT, PAGE_READWRITE ) ))
        {
            PUCHAR pTeb64 = PsGetThreadTeb( pContext->pWorker );

            // WOW64
            if (PsGetProcessWow64Process( pContext->pProcess ) != NULL)
            {
                // TEB32 is 2 pages after TEB64
                *(PULONG)(pTeb64 + 0x2000 + 0x2C) = (ULONG)(ULONG_PTR)pTLSMem;
                *(PULONG)pTLSMem = (ULONG)TLS_VAL_T( pHeaders, pTLS, StartAddressOfRawData );
            }
            // Native
            else
            {
                *(PVOID*)(pTeb64 + 0x58) = pTLSMem;
                *(PULONGLONG)pTLSMem = TLS_VAL_T( pHeaders, pTLS, StartAddressOfRawData );
            }

            DPRINT( "BlackBone: %s: Static TLS buffer: 0x%p\n", __FUNCTION__, pTLSMem );
            pContext->tlsInitialized = TRUE;
        }
    }
    
    for ( PUCHAR pCallback = (PUCHAR)TLS_VAL_T( pHeaders, pTLS, AddressOfCallBacks );
          ((IMAGE64( pHeaders ) ? *(PULONGLONG)pCallback : *(PULONG)pCallback)) != 0;
          pCallback += IMAGE64( pHeaders ) ? sizeof( ULONGLONG ) : sizeof( ULONG ))
    {
        ULONG_PTR callback = IMAGE64( pHeaders ) ? *(PULONGLONG)pCallback : *(PULONG)pCallback;
        BBCallRoutine( FALSE, pContext, (PVOID)callback, 3, imageBase, (PVOID)1, NULL );
    }
}

/// <summary>
/// Create activation context
/// </summary>
/// <param name="pPath">Manifest path</param>
/// <param name="asImage">Target file is a PE image</param>
/// <param name="manifestID">Manifest ID</param>
/// <param name="pContext">Loader context</param>
/// <returns>Status code</returns>
NTSTATUS BBPrepareACTX( IN PUNICODE_STRING pPath, IN BOOLEAN asImage, IN LONG manifestID, IN PMMAP_CONTEXT pContext )
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
            pContext->userMem->actx32.lpSource = (ULONG)(ULONG_PTR)pContext->userMem->buffer;
            if (asImage)
            {
                pContext->userMem->actx32.lpSource += 4 * sizeof( wchar_t );
                pContext->userMem->actx32.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
                pContext->userMem->actx32.lpResourceName = (ULONG)(ULONG_PTR)MAKEINTRESOURCEW( manifestID );
            }

            BBCallRoutine( FALSE, pContext, pCreateActCtxW, 1, &pContext->userMem->actx32 );
        }
        else
        {
            RtlZeroMemory( &pContext->userMem->actx, sizeof( pContext->userMem->actx ) );

            pContext->userMem->actx.cbSize = sizeof( ACTCTXW );
            pContext->userMem->actx.lpSource = pContext->userMem->buffer;
            if (asImage)
            {
                pContext->userMem->actx.lpSource += 4;
                pContext->userMem->actx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
                pContext->userMem->actx.lpResourceName = MAKEINTRESOURCEW( manifestID );
            }

            BBCallRoutine( FALSE, pContext, pCreateActCtxW, 1, &pContext->userMem->actx );
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
        BBCallRoutine( FALSE, pContext, pActivateCtx, 3, NULL, pContext->userMem->hCTX, &pContext->userMem->hCookie );
        if (!NT_SUCCESS( pContext->userMem->status ))
        {
            DPRINT( "BlackBone: %s: RtlActivateActivationContext failed with code 0x%X\n", __FUNCTION__, pContext->userMem->status );
            status = pContext->userMem->status;
        }
    }

    return status;
}

/// <summary>
/// Create exception table for x64 image
/// </summary>
/// <param name="pContext">Loader context</param>
/// <param name="imageBase">Image base</param>
/// <returns>Status code</returns>
NTSTATUS BBCreateExceptionTable64( IN PMMAP_CONTEXT pContext, IN PVOID imageBase )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG dirSize = 0;
    _PIMAGE_RUNTIME_FUNCTION_ENTRY pExpTable = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)RtlImageDirectoryEntryToData( 
        imageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &dirSize 
        );

    // Invoke RtlAddFunctionTable
    if (pExpTable)
    {
        UNICODE_STRING ustrNtdll = { 0 };
        BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

        RtlUnicodeStringInit( &ustrNtdll, L"ntdll.dll" );

        PVOID pAddTable = BBGetModuleExport(
            BBGetUserModule( pContext->pProcess, &ustrNtdll, wow64 ),
            "RtlAddFunctionTable", pContext->pProcess, NULL
            );

        if (!pAddTable)
        {
            DPRINT( "BlackBone: %s: Failed to get RtlAddFunctionTable address\n", __FUNCTION__ );
            return STATUS_NOT_FOUND;
        }

        status = BBCallRoutine( FALSE, pContext, pAddTable, 3, pExpTable, (PVOID)(dirSize / sizeof( _IMAGE_RUNTIME_FUNCTION_ENTRY )), imageBase );
        if (NT_SUCCESS( status ))
        {
            if (pContext->userMem->retVal32 == FALSE)
                DPRINT( "BlackBone: %s: RtlAddFunctionTable failed\n", __FUNCTION__ );
        }
        else
            DPRINT( "BlackBone: %s: Failed co call RtlAddFunctionTable. Status code: 0x%X\n", __FUNCTION__, status );

        return status;
    }

    return status;
}

/// <summary>
/// Setup image security cookie
/// </summary>
/// <param name="imageBase">Image base</param>
/// <returns>Status code</returns>
NTSTATUS BBCreateCookie( IN PVOID imageBase )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIMAGE_NT_HEADERS pHeader = RtlImageNtHeader( imageBase );
    if (pHeader)
    {
        ULONG cfgSize = 0;
        PVOID pCfgDir = RtlImageDirectoryEntryToData( imageBase, TRUE, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &cfgSize );

        // TODO: implement proper cookie algorithm
        if (pCfgDir && CFG_DIR_VAL_T( pHeader, pCfgDir, SecurityCookie ))
        {
            ULONG seed = (ULONG)(ULONG_PTR)imageBase ^ (ULONG)((ULONG_PTR)imageBase >> 32);
            ULONG_PTR cookie = (ULONG_PTR)imageBase ^  RtlRandomEx( &seed );

            // SecurityCookie value must be rebased by this moment
            if (IMAGE64( pHeader ))
                *(PULONG_PTR)CFG_DIR_VAL_T( pHeader, pCfgDir, SecurityCookie ) = cookie;
            else
                *(PULONG)CFG_DIR_VAL_T( pHeader, pCfgDir, SecurityCookie ) = (ULONG)cookie;
        }
    }
    else
        status = STATUS_INVALID_IMAGE_FORMAT;

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
                *(PUCHAR)(pBuf + ofst) = 0x68;                      // push pDelay
                *(PULONG)(pBuf + ofst + 1) = (ULONG)(ULONG_PTR)pDelay;      //
                ofst += 5;

                *(PUSHORT)(pBuf + ofst) = 0x016A;                   // push TRUE
                ofst += 2;

                *(PUCHAR)(pBuf + ofst) = 0xB8;                      // mov eax, pFn
                *(PULONG)(pBuf + ofst + 1) = (ULONG)(ULONG_PTR)pNtDelayExec;//
                ofst += 5;

                *(PUSHORT)(pBuf + ofst) = 0xD0FF;                   // call eax
                ofst += 2;

                *(PUSHORT)(pBuf + ofst) = 0xF0EB;                   // jmp
                ofst += 2;
            }
            else
            {
                *(PUSHORT)(pBuf + ofst) = 0xB948;           // mov rcx, TRUE
                *(PULONG_PTR)(pBuf + ofst + 2) = TRUE;      //
                ofst += 10;

                *(PUSHORT)(pBuf + ofst) = 0xBA48;           // mov rdx, pDelay
                *(PVOID*)(pBuf + ofst + 2) = pDelay;        //
                ofst += 10;

                *(PUSHORT)(pBuf + ofst) = 0xB848;           // mov rax, pNtDelayExec
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
/// Find suitable thread to be used as worker for user-mode calls
/// </summary>
/// <param name="pContext">Map context</param>
/// <returns>Status code</returns>
NTSTATUS BBFindWokerThread( IN PMMAP_CONTEXT pContext )
{
    return BBLookupProcessThread( pContext->pProcess, &pContext->pWorker );
}

/// <summary>
/// Call arbitrary function
/// </summary>
/// <param name="newThread">Perform call in a separate thread</param>
/// <param name="pContext">Loader context</param>
/// <param name="pRoutine">Routine to call.</param>
/// <param name="argc">Number of arguments.</param>
/// <param name="...">Arguments</param>
/// <returns>Status code</returns>
NTSTATUS BBCallRoutine( IN BOOLEAN newThread, IN PMMAP_CONTEXT pContext, IN PVOID pRoutine, IN INT argc, ... )
{
    NTSTATUS status = STATUS_SUCCESS;
    va_list vl;
    BOOLEAN wow64 = PsGetProcessWow64Process( pContext->pProcess ) != NULL;

    va_start( vl, argc );
    ULONG ofst = GenPrologueT( wow64, pContext->userMem->code );
    ofst += GenCallTV( wow64, pContext->userMem->code + ofst, pRoutine, argc, vl );
    ofst += GenSyncT( wow64, pContext->userMem->code + ofst, &pContext->userMem->status, pContext->pSetEvent, pContext->hSync );
    ofst += GenEpilogueT( wow64, pContext->userMem->code + ofst, argc * sizeof( ULONG ) );

    if (newThread)
    {
        status = BBExecuteInNewThread( pContext->userMem->code, NULL, 0/*THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER*/, TRUE, NULL );
    }
    else
    {
        KeResetEvent( pContext->pSync );
        status = BBQueueUserApc( pContext->pWorker, pContext->userMem->code, NULL, NULL, NULL, pContext->noThreads );
        if (NT_SUCCESS( status ))
        {
            LARGE_INTEGER timeout = { 0 };
            timeout.QuadPart = -(10ll * 10 * 1000 * 1000);  // 10s

            status = KeWaitForSingleObject( pContext->pSync, Executive, UserMode, TRUE, &timeout );

            timeout.QuadPart = -(1ll * 10 * 1000);          // 1ms
            KeDelayExecutionThread( KernelMode, TRUE, &timeout );
        }
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
/// <param name="noDEP">Set to TRUE if DEP is disabled. This will omit creation of executable pages</param>
/// <returns>Memory protection value</returns>
ULONG BBCastSectionProtection( IN ULONG characteristics, IN BOOLEAN noDEP )
{
    ULONG dwResult = PAGE_NOACCESS;

    if (characteristics & IMAGE_SCN_MEM_DISCARDABLE)
    {
        dwResult = PAGE_NOACCESS;
    }
    else if (characteristics & IMAGE_SCN_MEM_EXECUTE)
    {
        if (characteristics & IMAGE_SCN_MEM_WRITE)
            dwResult = noDEP ? PAGE_READWRITE : PAGE_EXECUTE_READWRITE;
        else if (characteristics & IMAGE_SCN_MEM_READ)
            dwResult = noDEP ? PAGE_READONLY : PAGE_EXECUTE_READ;
        else
            dwResult = noDEP ? PAGE_READONLY : PAGE_EXECUTE;
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
/// Get image manifest data
/// </summary>
/// <param name="pImageBase">Image base</param>
/// <param name="pSize">Manifest size</param>
/// <param name="pID">Manifest ID</param>
/// <returns>Manifest data pointer or NULL if no manifest</returns>
PVOID BBImageManifest( IN PVOID pImageBase, OUT PULONG_PTR pSize, OUT PLONG pID )
{
    ULONG secSize = 0;

    ASSERT( pImageBase != NULL && pSize != NULL && pID != NULL );
    if (pImageBase == NULL || pSize == NULL || pID == NULL)
        return NULL;

    // 3 levels of pointers to nodes
    IMAGE_RESOURCE_DIRECTORY_ENTRY *pDirNode1 = NULL;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *pDirNode2 = NULL;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *pDirNode3 = NULL;

    // 3 levels of nodes
    IMAGE_RESOURCE_DIRECTORY *pDirNodePtr1 = NULL;
    IMAGE_RESOURCE_DIRECTORY *pDirNodePtr2 = NULL;

    // resource entry data
    IMAGE_RESOURCE_DATA_ENTRY  *pDataNode = NULL;

    size_t ofst_1 = 0;  // first level nodes offset
    size_t ofst_2 = 0;  // second level nodes offset
    size_t ofst_3 = 0;  // third level nodes offset

    // Get section base
    PUCHAR secBase = RtlImageDirectoryEntryToData( pImageBase, FALSE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &secSize );
    if (secBase == NULL)
    {
        *pSize = 0;
        *pID = 0;
        return NULL;
    }

    pDirNodePtr1 = (PIMAGE_RESOURCE_DIRECTORY)secBase;
    ofst_1 += sizeof( IMAGE_RESOURCE_DIRECTORY );

    // first-level nodes
    for (int i = 0; i < pDirNodePtr1->NumberOfIdEntries + pDirNodePtr1->NumberOfNamedEntries; ++i)
    {
        pDirNode1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(secBase + ofst_1);

        // Not a manifest directory
        if (!pDirNode1->DataIsDirectory || pDirNode1->Id != 0x18)
        {
            ofst_1 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
            continue;
        }

        pDirNodePtr2 = (PIMAGE_RESOURCE_DIRECTORY)(secBase + pDirNode1->OffsetToDirectory);
        ofst_2 = pDirNode1->OffsetToDirectory + sizeof( IMAGE_RESOURCE_DIRECTORY );

        // second-level nodes
        for (int j = 0; j < pDirNodePtr2->NumberOfIdEntries + pDirNodePtr2->NumberOfNamedEntries; ++j)
        {
            pDirNode2 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(secBase + ofst_2);

            if (!pDirNode2->DataIsDirectory)
            {
                ofst_2 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
                continue;
            }

            // Check if this is a valid manifest resource
            if (pDirNode2->Id == 1 || pDirNode2->Id == 2 || pDirNode2->Id == 3)
            {
                ofst_3 = pDirNode2->OffsetToDirectory + sizeof( IMAGE_RESOURCE_DIRECTORY );
                pDirNode3 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(secBase + ofst_3);
                pDataNode = (PIMAGE_RESOURCE_DATA_ENTRY)(secBase + pDirNode3->OffsetToData);

                *pID = pDirNode2->Id;
                *pSize = pDataNode->Size;

                return BBRvaToVa( pDataNode->OffsetToData, FALSE, pImageBase );
            }

            ofst_2 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
        }

        ofst_1 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
    }

    return NULL;
}

/// <summary>
/// Translate RVA address to VA
/// </summary>
/// <param name="rva">RVA</param>
/// <param name="MappedAsImage">Image is mapped using image memory layout</param>
/// <param name="imageBase">Image base</param>
/// <returns>Translated address</returns>
PVOID BBRvaToVa( IN ULONG rva, IN BOOLEAN MappedAsImage, IN PVOID imageBase )
{
    // Simple offset
    if (MappedAsImage)
        return (PUCHAR)imageBase + rva;

    PIMAGE_NT_HEADERS pHeader = RtlImageNtHeader( imageBase );
    if (pHeader)
    {
        PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pHeader + 1);
        if (IMAGE32( pHeader ))
            pFirstSection = (PIMAGE_SECTION_HEADER)((PIMAGE_NT_HEADERS32)pHeader + 1);

        for (PIMAGE_SECTION_HEADER pSection = pFirstSection;
              pSection < pFirstSection + pHeader->FileHeader.NumberOfSections;
              pSection++)
        {
            if (rva >= pSection->VirtualAddress && rva < pSection->VirtualAddress + pSection->Misc.VirtualSize)
                return (PUCHAR)imageBase + pSection->PointerToRawData + (rva - pSection->VirtualAddress);
        }
    }

    return NULL;
}
