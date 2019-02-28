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

// Image name resolve flags
typedef enum _ResolveFlags
{
    KApiShemaOnly = 1,
    KSkipSxS      = 2,
    KFullPath     = 4,
} ResolveFlags;

/// <summary>
/// Module info
/// </summary>
typedef struct _MODULE_DATA
{
    LIST_ENTRY link;            // List link
    PUCHAR baseAddress;         // Base image address in target process
    PUCHAR localBase;           // Base image address in system space
    UNICODE_STRING name;        // File name
    UNICODE_STRING fullPath;    // Full file path
    SIZE_T size;                // Size of image
    ModType type;               // Module type
    enum KMmapFlags flags;      // Flags
    BOOLEAN manual;             // Image is manually mapped
    BOOLEAN initialized;        // DllMain was already called
} MODULE_DATA, *PMODULE_DATA;


/// <summary>
/// User-mode memory region
/// </summary>
typedef struct _USER_CONTEXT
{
    UCHAR code[0x1000];             // Code buffer
    union 
    {
        UNICODE_STRING ustr;
        UNICODE_STRING32 ustr32;
    };
    wchar_t buffer[0x400];          // Buffer for unicode string


    // Activation context data
    union
    {
        ACTCTXW actx;
        ACTCTXW32 actx32;
    };
    HANDLE hCTX;                    
    ULONG hCookie;

    PVOID ptr;                      // Tmp data
    union
    {
        NTSTATUS status;            // Last execution status
        PVOID retVal;               // Function return value
        ULONG retVal32;             // Function return value
    };

    //UCHAR tlsBuf[0x100];
} USER_CONTEXT, *PUSER_CONTEXT;

/// <summary>
/// Manual map context
/// </summary>
typedef struct _MMAP_CONTEXT
{
    PEPROCESS pProcess;     // Target process
    PVOID pWorkerBuf;       // Worker thread code buffer
    HANDLE hWorker;         // Worker thread handle
    PETHREAD pWorker;       // Worker thread object
    LIST_ENTRY modules;     // Manual module list
    PUSER_CONTEXT userMem;  // Tmp buffer in user space
    HANDLE hSync;           // APC sync handle
    PKEVENT pSync;          // APC sync object
    PVOID pSetEvent;        // ZwSetEvent address
    PVOID pLoadImage;       // LdrLoadDll address
    BOOLEAN tlsInitialized; // Static TLS was initialized
    BOOLEAN noThreads;      // No threads should be created
} MMAP_CONTEXT, *PMMAP_CONTEXT;

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
/// <param name="pProcess">Target process for user module</param>
/// <returns>Found address, NULL if not found</returns>
PVOID BBGetModuleExport( IN PVOID pBase, IN PCCHAR name_ord, IN PEPROCESS pProcess, IN PUNICODE_STRING modName );

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
    IN enum KMmapFlags flags,
    IN ULONG initRVA,
    IN PWCH initArg,
    OUT PMODULE_DATA pImage
    );

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
    IN enum KMmapFlags flags
    );

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
    );

/// <summary>
/// Find first thread of the target process
/// </summary>
/// <param name="pProcess">Target process.</param>
/// <param name="ppThread">Found thread. Thread object reference count is increased by 1</param>
/// <returns>Status code</returns>
NTSTATUS BBLookupProcessThread( IN PEPROCESS pProcess, OUT PETHREAD* ppThread );

/// <summary>
/// Create new thread in the target process
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
    );

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