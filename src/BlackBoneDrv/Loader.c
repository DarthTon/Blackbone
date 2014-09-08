#include "Private.h"
#include "Loader.h"
#include <Ntstrsafe.h>

PLIST_ENTRY PsLoadedModuleList;

#pragma alloc_text(PAGE, InitLdrData)
#pragma alloc_text(PAGE, GetSystemModule)
#pragma alloc_text(PAGE, GetUserModuleBase)
#pragma alloc_text(PAGE, GetModuleExport)
#pragma alloc_text(PAGE, BBMMapDriver)
#pragma alloc_text(PAGE, ResolveReferences)

NTSTATUS InitLdrData( IN PKLDR_DATA_TABLE_ENTRY pThisModule )
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

PKLDR_DATA_TABLE_ENTRY GetSystemModule( IN PUNICODE_STRING pName, IN PVOID pAddress )
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
/// <returns>Found address, NULL if not found</returns>
PVOID GetUserModuleBase( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64 )
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
                    // TODO: Implement
                    return NULL;
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

NTSTATUS ResolveReferences( IN PVOID pImageBase )
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
        pModule = GetSystemModule( &ustrImpDll, NULL );
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

            pFunc = GetModuleExport( pModule->DllBase, impFunc );

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

NTSTATUS BBMMapDriver( IN PUNICODE_STRING pPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;
    OBJECT_ATTRIBUTES obAttr = { 0 };
    IO_STATUS_BLOCK statusBlock = { 0 };
    PVOID fileData = NULL;
    PIMAGE_NT_HEADERS pNTHeader = NULL;
    PVOID imageData = NULL;
    FILE_STANDARD_INFORMATION fileInfo = { 0 };

    ASSERT( pPath != NULL );
    if (pPath == NULL)
        return STATUS_INVALID_PARAMETER;

    InitializeObjectAttributes( &obAttr, pPath, OBJ_KERNEL_HANDLE, NULL, NULL );

    status = ZwCreateFile( &hFile, FILE_READ_DATA | SYNCHRONIZE, &obAttr, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL, 
                           FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to open '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );
        return status;
    }

    status = ZwQueryInformationFile( hFile, &statusBlock, &fileInfo, sizeof( fileInfo ), FileStandardInformation );
    if (NT_SUCCESS( status ))
        fileData = ExAllocatePoolWithTag( PagedPool, fileInfo.EndOfFile.QuadPart, BB_POOL_TAG );
    else
        DPRINT( "BlackBone: %s: Failed to get '%wZ' size. Status: 0x%X\n", __FUNCTION__, pPath, status );

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
        imageData = ExAllocatePoolWithTag( NonPagedPoolExecute, pNTHeader->OptionalHeader.SizeOfImage, BB_POOL_TAG );

        // Copy header
        RtlCopyMemory( imageData, fileData, pNTHeader->OptionalHeader.SizeOfHeaders );

        // Copy sections
        for (PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)(pNTHeader + 1);
              pSection < (PIMAGE_SECTION_HEADER)(pNTHeader + 1) + pNTHeader->FileHeader.NumberOfSections;
              pSection++)
        {
            RtlCopyMemory( (PUCHAR)imageData + pSection->VirtualAddress, 
                           (PUCHAR)fileData + pSection->PointerToRawData, 
                           pSection->SizeOfRawData );
        }

        // Relocate image
        status = LdrRelocateImage( imageData, STATUS_SUCCESS, STATUS_CONFLICTING_ADDRESSES, STATUS_INVALID_IMAGE_FORMAT );
        if (!NT_SUCCESS(status))
            DPRINT( "BlackBone: %s: Failed to relocate '%wZ'. Status: 0x%X\n", __FUNCTION__, pPath, status );

        if (NT_SUCCESS(status))
            status = ResolveReferences( imageData );
    }

    // Call entrypoint
    if (NT_SUCCESS( status ))
    {
        PDRIVER_INITIALIZE pEntryPoint = (PDRIVER_INITIALIZE)((ULONG_PTR)imageData + pNTHeader->OptionalHeader.AddressOfEntryPoint);
        if (pEntryPoint)
            pEntryPoint( NULL, NULL );
    }

    if (imageData)
        ExFreePoolWithTag( imageData, BB_POOL_TAG );

    if (fileData)
        ExFreePoolWithTag( fileData, BB_POOL_TAG );

    if (NT_SUCCESS(status))
        DPRINT( "BlackBone: %s: Successfully mapped '%wZ' at 0x%p\n", __FUNCTION__, pPath, imageData );

    return status;
}
