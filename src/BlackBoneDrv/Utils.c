#include "Utils.h"
#include <ntstrsafe.h>

#pragma alloc_text(PAGE, BBStripPath)
#pragma alloc_text(PAGE, BBFileExists)

/// <summary>
/// Get file name from full path
/// </summary>
/// <param name="path">Path.</param>
/// <param name="name">Resultingf name</param>
/// <returns>Status code</returns>
NTSTATUS BBStripPath( IN PUNICODE_STRING path, OUT PUNICODE_STRING name )
{
    ASSERT( path != NULL && name );
    if (path == NULL || name == NULL)
        return STATUS_INVALID_PARAMETER;

    for (USHORT i = (path->Length / sizeof( WCHAR )) - 1; i != 0; i--)
    {
        if (path->Buffer[i] == L'\\' || path->Buffer[i] == L'/')
        {
            RtlUnicodeStringInit( name, &path->Buffer[i + 1] );
            return STATUS_SUCCESS;
        }
    }

    *name = *path;
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


ULONG GenPrologue32( IN PUCHAR pBuf )
{
    *pBuf = 0x55;
    *(PUSHORT)(pBuf + 1) = 0xE589;
    return 3;
}

ULONG GenEpilogue32( IN PUCHAR pBuf)
{
    *(PUSHORT)pBuf = 0xEC89;
    *(pBuf + 1) = 0x5D;   
    return 3;
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

ULONG GenEpilogue64( IN PUCHAR pBuf )
{
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

ULONG GenCall64V( IN PUCHAR pBuf, IN PVOID pFn, INT argc, IN va_list vl )
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