#pragma once
#include <wdm.h>

/// <summary>
/// Get file name from full path
/// </summary>
/// <param name="path">Path.</param>
/// <param name="name">Resultingf name</param>
/// <returns>Status code</returns>
NTSTATUS BBStripPath( IN PUNICODE_STRING path, OUT PUNICODE_STRING name );
NTSTATUS BBFileExists( IN PUNICODE_STRING path );


ULONG GenPrologue32( IN PUCHAR pBuf );
ULONG GenEpilogue32( IN PUCHAR pBuf );

ULONG GenPrologue64( IN PUCHAR pBuf );
ULONG GenEpilogue64( IN PUCHAR pBuf );

ULONG GenCall64( IN PUCHAR pBuf, IN PVOID pFn, INT argc, ... );
ULONG GenCall64V( IN PUCHAR pBuf, IN PVOID pFn, INT argc, va_list vl );

ULONG GenSync64( IN PUCHAR pBuf, IN PNTSTATUS pStatus, IN PVOID pSetEvent, IN HANDLE hEvent );
