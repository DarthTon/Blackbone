#include "Utils.h"
#include "Private.h"
#include <ntstrsafe.h>

#pragma alloc_text(PAGE, BBSafeAllocateString)
#pragma alloc_text(PAGE, BBSafeInitString)
#pragma alloc_text(PAGE, BBStripPath)
#pragma alloc_text(PAGE, BBStripFilename)
#pragma alloc_text(PAGE, BBFileExists)

/// <summary>
/// Allocate new Unicode string from Paged pool
/// </summary>
/// <param name="result">Resulting string</param>
/// <param name="size">Buffer size in bytes to alloacate</param>
/// <returns>Status code</returns>
NTSTATUS BBSafeAllocateString( OUT PUNICODE_STRING result, IN USHORT size )
{
    ASSERT( result != NULL );
    if (result == NULL || size == 0)
        return STATUS_INVALID_PARAMETER;

    result->Buffer = ExAllocatePoolWithTag( PagedPool, size, BB_POOL_TAG );
    result->Length = 0;
    result->MaximumLength = size;

    if (result->Buffer)
        RtlZeroMemory( result->Buffer, size );
    else
        return STATUS_NO_MEMORY;

    return STATUS_SUCCESS;
}

/// <summary>
/// Allocate and copy string
/// </summary>
/// <param name="result">Resulting string</param>
/// <param name="source">Source string</param>
/// <returns>Status code</returns>
NTSTATUS BBSafeInitString( OUT PUNICODE_STRING result, IN PUNICODE_STRING source )
{
    ASSERT( result != NULL && source != NULL );
    if (result == NULL || source == NULL || source->Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    // No data to copy
    if (source->Length == 0)
    {
        result->Length = result->MaximumLength = 0;
        result->Buffer = NULL;
        return STATUS_SUCCESS;
    }

    result->Buffer = ExAllocatePoolWithTag( PagedPool, source->MaximumLength, BB_POOL_TAG );
    result->Length = source->Length;
    result->MaximumLength = source->MaximumLength;

    memcpy( result->Buffer, source->Buffer, source->Length );

    return STATUS_SUCCESS;
}

/// <summary>
/// Search for substring
/// </summary>
/// <param name="source">Source string</param>
/// <param name="target">Target string</param>
/// <param name="CaseInSensitive">Case insensitive search</param>
/// <returns>Found position or -1 if not found</returns>
LONG BBSafeSearchString( IN PUNICODE_STRING source, IN PUNICODE_STRING target, IN BOOLEAN CaseInSensitive )
{
    ASSERT( source != NULL && target != NULL );
    if (source == NULL || target == NULL || source->Buffer == NULL || target->Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    // Size mismatch
    if (source->Length < target->Length)
        return -1;

    USHORT diff = source->Length - target->Length;
    for (USHORT i = 0; i < diff; i++)
    {
        if (RtlCompareUnicodeStrings(
            source->Buffer + i / sizeof( WCHAR ),
            target->Length / sizeof( WCHAR ),
            target->Buffer,
            target->Length / sizeof( WCHAR ),
            CaseInSensitive
            ) == 0)
        {
            return i;
        }
    }

    return -1;
}

/// <summary>
/// Get file name from full path
/// </summary>
/// <param name="path">Path.</param>
/// <param name="name">Resulting name</param>
/// <returns>Status code</returns>
NTSTATUS BBStripPath( IN PUNICODE_STRING path, OUT PUNICODE_STRING name )
{
    ASSERT( path != NULL && name );
    if (path == NULL || name == NULL)
        return STATUS_INVALID_PARAMETER;

    // Empty string
    if (path->Length < 2)
    {
        *name = *path;
        return STATUS_NOT_FOUND;
    }

    for (USHORT i = (path->Length / sizeof( WCHAR )) - 1; i != 0; i--)
    {
        if (path->Buffer[i] == L'\\' || path->Buffer[i] == L'/')
        {
            name->Buffer = &path->Buffer[i + 1];
            name->Length = name->MaximumLength = path->Length - (i + 1)*sizeof( WCHAR );
            return STATUS_SUCCESS;
        }
    }

    *name = *path;
    return STATUS_NOT_FOUND;
}

/// <summary>
/// Get directory path name from full path
/// </summary>
/// <param name="path">Path</param>
/// <param name="name">Resulting directory path</param>
/// <returns>Status code</returns>
NTSTATUS BBStripFilename( IN PUNICODE_STRING path, OUT PUNICODE_STRING dir )
{
    ASSERT( path != NULL && dir );
    if (path == NULL || dir == NULL)
        return STATUS_INVALID_PARAMETER;

    // Empty string
    if (path->Length < 2)
    {
        *dir = *path;
        return STATUS_NOT_FOUND;
    }

    for (USHORT i = (path->Length / sizeof( WCHAR )) - 1; i != 0; i--)
    {
        if (path->Buffer[i] == L'\\' || path->Buffer[i] == L'/')
        {
            dir->Buffer = path->Buffer;
            dir->Length = dir->MaximumLength = i*sizeof( WCHAR );
            return STATUS_SUCCESS;
        }
    }

    *dir = *path;
    return STATUS_NOT_FOUND;
}

/// <summary>
/// Check if file exists
/// </summary>
/// <param name="path">Fully qualifid path to a file</param>
/// <returns>Status code</returns>
NTSTATUS BBFileExists( IN PUNICODE_STRING path )
{
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK statusBlock = { 0 };
    OBJECT_ATTRIBUTES obAttr = { 0 };
    InitializeObjectAttributes( &obAttr, path, OBJ_KERNEL_HANDLE, NULL, NULL );

    NTSTATUS status = ZwCreateFile(
        &hFile, FILE_READ_DATA | SYNCHRONIZE, &obAttr,
        &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
        );

    if (NT_SUCCESS( status ))
        ZwClose( hFile );

    return status;
}

/// <summary>
/// Search for pattern
/// </summary>
/// <param name="pattern">Pattern to search for</param>
/// <param name="wildcard">Used wildcard</param>
/// <param name="len">Pattern length</param>
/// <param name="base">Base address for searching</param>
/// <param name="size">Address range to search in</param>
/// <param name="ppFound">Found location</param>
/// <returns>Status code</returns>
NTSTATUS BBSearchPattern( IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, IN const VOID* base, IN ULONG_PTR size, OUT PVOID* ppFound )
{
    ASSERT( ppFound != NULL && pattern != NULL && base != NULL );
    if (ppFound == NULL || pattern == NULL || base == NULL)
        return STATUS_INVALID_PARAMETER;

    for (ULONG_PTR i = 0; i < size - len; i++)
    {
        BOOLEAN found = TRUE;
        for (ULONG_PTR j = 0; j < len; j++)
        {
            if (pattern[j] != wildcard && pattern[j] != ((PCUCHAR)base)[i + j])
            {
                found = FALSE;
                break;
            }
        }

        if (found != FALSE)
        {
            *ppFound = (PUCHAR)base + i;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

/// <summary>
/// Check if process is terminating
/// </summary>
/// <param name="imageBase">Process</param>
/// <returns>If TRUE - terminating</returns>
BOOLEAN BBCheckProcessTermination( PEPROCESS pProcess )
{
    LARGE_INTEGER zeroTime = { 0 };
    return KeWaitForSingleObject( pProcess, Executive, KernelMode, FALSE, &zeroTime ) == STATUS_WAIT_0;
}

ULONG GenPrologue32( IN PUCHAR pBuf )
{
    *pBuf = 0x55;
    *(PUSHORT)(pBuf + 1) = 0xE589;

    return 3;
}

ULONG GenEpilogue32( IN PUCHAR pBuf, IN INT retSize )
{
    *(PUSHORT)pBuf = 0xEC89;
    *(pBuf + 2) = 0x5D;
    *(pBuf + 3) = 0xC2;
    *(PUSHORT)(pBuf + 4) = (USHORT)retSize;

    return 6;
}

ULONG GenCall32( IN PUCHAR pBuf, IN PVOID pFn, IN INT argc, ... )
{
    va_list vl;
    va_start( vl, argc );
    ULONG res = GenCall32V( pBuf, pFn, argc, vl );
    va_end( vl );

    return res;
}

ULONG GenCall32V( IN PUCHAR pBuf, IN PVOID pFn, IN INT argc, IN va_list vl )
{
    ULONG ofst = 0;

    PULONG pArgBuf = ExAllocatePoolWithTag( PagedPool, argc * sizeof( ULONG ), BB_POOL_TAG );

    // cast args
    for (INT i = 0; i < argc; i++)
    {
        PVOID arg = va_arg( vl, PVOID );
        pArgBuf[i] = (ULONG)(ULONG_PTR)arg;
    }

    // push args
    for (INT i = argc - 1; i >= 0; i--)
    {
        *(PUSHORT)(pBuf + ofst) = 0x68;                 // push arg
        *(PULONG)(pBuf + ofst + 1) = pArgBuf[i];        //
        ofst += 5;
    }

    *(PUCHAR)(pBuf + ofst) = 0xB8;                      // mov eax, pFn
    *(PULONG)(pBuf + ofst + 1) = (ULONG)(ULONG_PTR)pFn; //
    ofst += 5;

    *(PUSHORT)(pBuf + ofst) = 0xD0FF;                   // call eax
    ofst += 2;

    ExFreePoolWithTag( pArgBuf, BB_POOL_TAG );

    return ofst;
}

ULONG GenSync32( IN PUCHAR pBuf, IN PNTSTATUS pStatus, IN PVOID pSetEvent, IN HANDLE hEvent )
{
    ULONG ofst = 0;

    *(PUCHAR)(pBuf + ofst) = 0xA3;                  // mov [pStatus], eax
    *(PVOID*)(pBuf + ofst + 1) = pStatus;           //
    ofst += 5;

    *(PUSHORT)(pBuf + ofst) = 0x006A;               // push FALSE
    ofst += 2;

    *(PUCHAR)(pBuf + ofst) = 0x68;                  // push hEvent
    *(PULONG)(pBuf + ofst + 1) = (ULONG)(ULONG_PTR)hEvent;  //
    ofst += 5;

    *(PUCHAR)(pBuf + ofst) = 0xB8;                  // mov eax, pSetEvent
    *(PULONG)(pBuf + ofst + 1) = (ULONG)(ULONG_PTR)pSetEvent;//
    ofst += 5;

    *(PUSHORT)(pBuf + ofst) = 0xD0FF;               // call eax
    ofst += 2;

    return ofst;
}



ULONG GenPrologue64( IN PUCHAR pBuf )
{
    *(PULONG)(pBuf + 0) = 0x244C8948;       // mov [rsp + 0x08], rcx
    *(PUCHAR)(pBuf + 4) = 0x8;              // 
    *(PULONG)(pBuf + 5) = 0x24548948;       // mov [rsp + 0x10], rdx
    *(PUCHAR)(pBuf + 9) = 0x10;             // 
    *(PULONG)(pBuf + 10) = 0x2444894C;      // mov [rsp + 0x18], r8
    *(PUCHAR)(pBuf + 14) = 0x18;            // 
    *(PULONG)(pBuf + 15) = 0x244C894C;      // mov [rsp + 0x20], r9
    *(PUCHAR)(pBuf + 19) = 0x20;            // 
    return 20;
}

ULONG GenEpilogue64( IN PUCHAR pBuf, IN INT retSize )
{
    UNREFERENCED_PARAMETER( retSize );

    *(PULONG)(pBuf + 0) = 0x244C8B48;       // mov rcx, [rsp + 0x08]
    *(PUCHAR)(pBuf + 4) = 0x8;              // 
    *(PULONG)(pBuf + 5) = 0x24548B48;       // mov rdx, [rsp + 0x10]
    *(PUCHAR)(pBuf + 9) = 0x10;             // 
    *(PULONG)(pBuf + 10) = 0x24448B4C;      // mov r8, [rsp + 0x18]
    *(PUCHAR)(pBuf + 14) = 0x18;            // 
    *(PULONG)(pBuf + 15) = 0x244C8B4C;      // mov r9, [rsp + 0x20]
    *(PUCHAR)(pBuf + 19) = 0x20;            // 
    *(PUCHAR)(pBuf + 20) = 0xC3;            // ret
    return 21;
}

ULONG GenCall64( IN PUCHAR pBuf, IN PVOID pFn, INT argc, ... )
{
    va_list vl;
    va_start( vl, argc );
    ULONG res = GenCall64V( pBuf, pFn, argc, vl );
    va_end( vl );

    return res;
}

ULONG GenCall64V( IN PUCHAR pBuf, IN PVOID pFn, IN INT argc, IN va_list vl )
{
    USHORT rsp_diff = 0x28;
    ULONG ofst = 0;
    if (argc > 4)
    {
        rsp_diff = (USHORT)(argc * sizeof( ULONG_PTR ));
        if (rsp_diff % 0x10)
            rsp_diff = ((rsp_diff / 0x10) + 1) * 0x10;
        rsp_diff += 8;
    }

    // sub rsp, rsp_diff
    *(PULONG)(pBuf + ofst) = (0x00EC8348 | rsp_diff << 24);
    ofst += 4;

    if (argc > 0)
    {
        PVOID arg = va_arg( vl, PVOID );
        *(PUSHORT)(pBuf + ofst) = 0xB948;           // mov rcx, arg
        *(PVOID*)(pBuf + ofst + 2) = arg;           //
        ofst += 10;
    }
    if (argc > 1)
    {
        PVOID arg = va_arg( vl, PVOID );
        *(PUSHORT)(pBuf + ofst) = 0xBA48;           // mov rdx, arg
        *(PVOID*)(pBuf + ofst + 2) = arg;           //
        ofst += 10;
    }
    if (argc > 2)
    {
        PVOID arg = va_arg( vl, PVOID );
        *(PUSHORT)(pBuf + ofst) = 0xB849;           // mov r8, arg
        *(PVOID*)(pBuf + ofst + 2) = arg;           //
        ofst += 10;
    }
    if (argc > 3)
    {
        PVOID arg = va_arg( vl, PVOID );
        *(PUSHORT)(pBuf + ofst) = 0xB949;           // mov r9, arg
        *(PVOID*)(pBuf + ofst + 2) = arg;           //
        ofst += 10;
    }

    for (INT i = 4; i < argc; i++)
    {
        PVOID arg = va_arg( vl, PVOID );

        *(PUSHORT)(pBuf + ofst) = 0xB848;           // mov rcx, arg
        *(PVOID*)(pBuf + ofst + 2) = arg;           //
        ofst += 10;

        // mov [rsp + i*8], rax
        *(PULONG)(pBuf + ofst) = 0x24448948;
        *(PUCHAR)(pBuf + ofst + 4) = (UCHAR)(0x20 + (i - 4)*sizeof( arg ));
        ofst += 5;
    }


    *(PUSHORT)(pBuf + ofst) = 0xB848;               // mov rax, pFn
    *(PVOID*)(pBuf + ofst + 2) = pFn;               //
    ofst += 10;

    *(PUSHORT)(pBuf + ofst) = 0xD0FF;               // call rax
    ofst += 2;

    // add rsp, rsp_diff
    *(PULONG)(pBuf + ofst) = (0x00C48348 | rsp_diff << 24);
    ofst += 4;

    return ofst;
}

ULONG GenSync64( IN PUCHAR pBuf, IN PNTSTATUS pStatus, IN PVOID pSetEvent, IN HANDLE hEvent )
{
    ULONG ofst = 0;

    *(PUSHORT)(pBuf + ofst) = 0xA348;           // mov [pStatus], rax
    *(PVOID*)(pBuf + ofst + 2) = pStatus;       //
    ofst += 10;

    *(PUSHORT)(pBuf + ofst) = 0xB948;           // mov rcx, hEvent
    *(PHANDLE)(pBuf + ofst + 2) = hEvent;       //
    ofst += 10;

    *(pBuf + ofst) = 0x48;                      // xor rdx, rdx
    *(PUSHORT)(pBuf + ofst + 1) = 0xD231;       //
    ofst += 3;

    *(PUSHORT)(pBuf + ofst) = 0xB848;           // mov rax, pSetEvent
    *(PVOID*)(pBuf + ofst + 2) = pSetEvent;     //
    ofst += 10;

    *(PUSHORT)(pBuf + ofst) = 0xD0FF;           // call rax
    ofst += 2;

    return ofst;
}



ULONG GenPrologueT( IN BOOLEAN wow64, IN PUCHAR pBuf )
{
    return wow64 ? GenPrologue32( pBuf ) : GenPrologue64( pBuf );
}

ULONG GenEpilogueT( IN BOOLEAN wow64, IN PUCHAR pBuf, IN INT retSize )
{
    return wow64 ? GenEpilogue32( pBuf, retSize ) : GenEpilogue64( pBuf, retSize );
}

ULONG GenCallT( IN BOOLEAN wow64, IN PUCHAR pBuf, IN PVOID pFn, IN INT argc, ... )
{
    va_list vl;
    va_start( vl, argc );
    ULONG res = wow64 ? GenCall32V( pBuf, pFn, argc, vl ) : GenCall64V( pBuf, pFn, argc, vl );
    va_end( vl );

    return res;
}

ULONG GenCallTV( IN BOOLEAN wow64, IN PUCHAR pBuf, IN PVOID pFn, IN INT argc, IN va_list vl )
{
    return wow64 ? GenCall32V( pBuf, pFn, argc, vl ) : GenCall64V( pBuf, pFn, argc, vl );
}

ULONG GenSyncT( IN BOOLEAN wow64, IN PUCHAR pBuf, IN PNTSTATUS pStatus, IN PVOID pSetEvent, IN HANDLE hEvent )
{
    return wow64 ? GenSync32( pBuf, pStatus, pSetEvent, hEvent ) : GenSync64( pBuf, pStatus, pSetEvent, hEvent );
}
