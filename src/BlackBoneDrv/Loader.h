#pragma once

#include "Imports.h"
#include "NativeStructs.h"


NTSTATUS InitLdrData( IN PKLDR_DATA_TABLE_ENTRY pThisModule );

PKLDR_DATA_TABLE_ENTRY GetSystemModule( IN PUNICODE_STRING pName, IN PVOID pAddress );

/// <summary>
/// Get module base address by name
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="ModuleName">Nodule name to search for</param>
/// <param name="isWow64">If TRUE - search in 32-bit PEB</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetUserModuleBase( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64 );

/// <summary>
/// Get exported function address
/// </summary>
/// <param name="pBase">Module base</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <returns>Found address, NULL if not found</returns>
PVOID GetModuleExport( IN PVOID pBase, IN PCCHAR name_ord );

NTSTATUS BBMMapDriver( IN PUNICODE_STRING pPath );


NTSTATUS ResolveReferences( IN PVOID pImageBase );

PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONGLONG Diff
    );

NTSTATUS
LdrRelocateImage (
    IN PVOID NewBase,
    IN NTSTATUS Success,
    IN NTSTATUS Conflict,
    IN NTSTATUS Invalid
    );


NTSTATUS
LdrRelocateImageWithBias(
    IN PVOID NewBase,
    IN LONGLONG AdditionalBias,
    IN NTSTATUS Success,
    IN NTSTATUS Conflict,
    IN NTSTATUS Invalid
    );

PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG_PTR Diff
    );

PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONGLONG Diff
    );