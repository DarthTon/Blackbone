#pragma once

#include "Private.h"
#include "VadRoutines.h"
#include "BlackBoneDef.h"

typedef enum _ATTACHED_CONTEXT
{
    ContextNone,    // Running in system context
    ContextHost,    // Running in the context of host process
    ContextTarget,  // Running in the context of target process
} ATTACHED_CONTEXT;

/// <summary>
/// Process-specific data
/// </summary>
typedef struct _PROCESS_CONTEXT
{
    HANDLE pid;             // Process ID
    PVOID sharedPage;       // Address of kernel shared page
} PROCESS_CONTEXT, *PPROCESS_CONTEXT;


/// <summary>
/// Target - host correspondence
/// </summary>
typedef struct _PROCESS_MAP_ENTRY
{
    PROCESS_CONTEXT host;   // Hosting process context
    PROCESS_CONTEXT target; // Target process context

    PVOID pSharedPage;      // Address of kernel-shared page allocated from non-paged pool
    PMDL  pMDLShared;       // MDL of kernel-shared page

    HANDLE targetPipe;      // Hook pipe handle in target process

    LIST_ENTRY pageList;    // List of REMAP_ENTRY structures
} PROCESS_MAP_ENTRY, *PPROCESS_MAP_ENTRY;


/// <summary>
/// Mapped memory region info
/// </summary>
typedef struct _MAP_ENTRY
{
    LIST_ENTRY link;                // Linked list link
    MEMORY_BASIC_INFORMATION mem;   // Original memory info

    ULONG_PTR newPtr;               // Mapped memory address in host process
    PMDL    pMdl;                   // Region MDL entry
    BOOLEAN locked;                 // MDL is locked
    BOOLEAN shared;                 // Regions has shared pages
    BOOLEAN readonly;               // Region must be mapped as readonly
} MAP_ENTRY, *PMAP_ENTRY;

extern DYNAMIC_DATA dynData;
extern RTL_AVL_TABLE g_ProcessPageTables;
extern KGUARDED_MUTEX g_globalLock;

/// <summary>
/// Map entire address space of target process into current
/// </summary>
/// <param name="pRemap">Mapping params</param>
/// <param name="ppEntry">Mapped context</param>
/// <returns>Status code</returns>
NTSTATUS BBMapMemory( IN PMAP_MEMORY pRemap, OUT PPROCESS_MAP_ENTRY* ppEntry );

/// <summary>
/// Map specific memory region
/// </summary>
/// <param name="pRegion">Region data</param>
/// <param name="pResult">Mapping results</param>
/// <returns>Status code</returns>
NTSTATUS BBMapMemoryRegion( IN PMAP_MEMORY_REGION pRegion, OUT PMAP_MEMORY_REGION_RESULT pResult );

/// <summary>
/// Unmap any mapped memory from host and target processes
/// </summary>
/// <param name="pUnmap">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBUnmapMemory( IN PUNMAP_MEMORY pUnmap );

/// <summary>
/// Unmap specific memory region
/// </summary>
/// <param name="pRegion">Region info</param>
/// <returns>Status ode</returns>
NTSTATUS BBUnmapMemoryRegion( IN PUNMAP_MEMORY_REGION pRegion );

/// <summary>
/// Calculate size required to store mapping info
/// </summary>
/// <param name="pList">Mapped regions list</param>
/// <param name="pSize">Resulting size</param>
/// <returns>Status code</returns>
NTSTATUS BBGetRequiredRemapOutputSize( IN PLIST_ENTRY pList, OUT PULONG_PTR pSize );

/// <summary>
/// Enumerate committed, accessible, non-guarded memory regions
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="start">Region start</param>
/// <param name="end">Region end</param>
/// <param name="mapSections">If set to FALSE, section objects will be excluded from list</param>
/// <returns>Status code</returns>
NTSTATUS BBBuildProcessRegionListForRange( IN PLIST_ENTRY pList, IN ULONG_PTR start, IN ULONG_PTR end, IN BOOLEAN mapSections );

/// <summary>
/// Search process entry in list by PID
/// </summary>
/// <param name="pid">PID.</param>
/// <param name="asHost">If set to TRUE, pid is treated as host PID</param>
/// <returns>Found entry, NULL in not found</returns>
PPROCESS_MAP_ENTRY BBLookupProcessEntry( IN HANDLE pid, IN BOOLEAN asHost );

/// <summary>
/// Unmap all regions, delete MDLs, close handles, remove entry from table
/// </summary>
/// <param name="pProcessEntry">Process entry</param>
VOID BBCleanupProcessEntry( IN PPROCESS_MAP_ENTRY pProcessEntry );

/// <summary>
/// Clear global process map table
/// </summary>
VOID BBCleanupProcessTable();

/// <summary>
/// Unmap any mapped pages from host process
/// </summary>
/// <param name="pProcessEntry">Process entry</param>
VOID BBCleanupHostProcess( IN PPROCESS_MAP_ENTRY pProcessEntry );

//
// AVL table routines
//
RTL_GENERIC_COMPARE_RESULTS AvlCompare( IN RTL_AVL_TABLE *Table, IN PVOID FirstStruct, IN PVOID SecondStruct );
PVOID AvlAllocate( IN RTL_AVL_TABLE *Table, IN CLONG ByteSize );
VOID AvlFree( IN RTL_AVL_TABLE *Table, IN PVOID Buffer );