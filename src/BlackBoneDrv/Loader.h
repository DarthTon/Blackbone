#pragma once

#include "Imports.h"
#include "NativeStructs.h"

// Module type
typedef enum _ModType
{
    mt_mod32,       // 64 bit module
    mt_mod64,       // 32 bit module
    mt_default,     // type is deduced from target process
    mt_unknown      // Failed to detect type
} ModType;

typedef struct _MODULE_DATA
{
    LIST_ENTRY link;            // List link
    PUCHAR baseAddress;         // Base image address in target process
    PUCHAR localBase;           // Base image address in system space
    UNICODE_STRING name;        // File name
    UNICODE_STRING fullPath;    // Full file path
    SIZE_T size;                // Size of image
    ModType type;               // Module type
    enum MMapFlags flags;       // Flags
    BOOLEAN manual;             // Image is manually mapped
    BOOLEAN initialized;        // DllMain was already called
} MODULE_DATA, *PMODULE_DATA;

/// <summary>
/// Initialize loader stuff
/// </summary>
/// <param name="pThisModule">Any valid system module</param>
/// <returns>Status code</returns>
NTSTATUS BBInitLdrData( IN PKLDR_DATA_TABLE_ENTRY pThisModule );

/// <summary>
/// Get address of a system module
/// Either 'pName' or 'pAddress' is required to perform search
/// </summary>
/// <param name="pName">Base name of the image (e.g. hal.dll)</param>
/// <param name="pAddress">Address inside module</param>
/// <returns>Found loader entry. NULL if nothing found</returns>
PKLDR_DATA_TABLE_ENTRY BBGetSystemModule( IN PUNICODE_STRING pName, IN PVOID pAddress );

/// <summary>
/// Get module base address by name
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="ModuleName">Nodule name to search for</param>
/// <param name="isWow64">If TRUE - search in 32-bit PEB</param>
/// <returns>Found address, NULL if not found</returns>
PVOID BBGetUserModule( IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64 );

/// <summary>
/// Unlink user-mode module from Loader lists
/// </summary>
/// <param name="pProcess">Target process</param>
/// <param name="pBase">Module base</param>
/// <param name="isWow64">If TRUE - unlink from PEB32 Loader, otherwise use PEB64</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkFromLoader( IN PEPROCESS pProcess, IN PVOID pBase, IN BOOLEAN isWow64 );

/// <summary>
/// Get exported function address
/// </summary>
/// <param name="pBase">Module base</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <returns>Found address, NULL if not found</returns>
PVOID BBGetModuleExport( IN PVOID pBase, IN PCCHAR name_ord );

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
    IN INT flags,
    OUT PMODULE_DATA pImage
    );


NTSTATUS BBResolveReferences( IN PVOID pImageBase, IN BOOLEAN systemImage, IN PEPROCESS pProcess, IN BOOLEAN wow64Image );

/// <summary>
/// Find first thread of the target process
/// </summary>
/// <param name="pid">Target PID.</param>
/// <param name="ppThread">Found thread. Thread object reference count is increased by 1</param>
/// <returns>Status code</returns>
NTSTATUS BBLookupProcessThread( IN HANDLE pid, OUT PETHREAD* ppThread );

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
    BOOLEAN bForce
    );


/// <summary>
/// Manually map driver into system space
/// </summary>
/// <param name="pPath">Fully qualified native path to the driver</param>
/// <returns>Status code</returns>
NTSTATUS BBMMapDriver( IN PUNICODE_STRING pPath );

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