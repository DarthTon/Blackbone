#pragma once

#include "BlackBoneDef.h"
#include "Private.h"
#include "VadRoutines.h"

/// <summary>
/// Allocated physical region entry
/// </summary>
typedef struct _MEM_PHYS_ENTRY
{
    LIST_ENTRY link;
    ULONG_PTR size;     // Region size
    PVOID pMapped;      // Mapped address
    PMDL pMDL;          // Related MDL
    PVOID ptr;          // Actual ptr in NonPagedPool
} MEM_PHYS_ENTRY, *PMEM_PHYS_ENTRY;

/// <summary>
/// Per-process list of physical regions
/// </summary>
typedef struct _MEM_PHYS_PROCESS_ENTRY
{
    LIST_ENTRY link;
    HANDLE pid;             // Process ID
    LIST_ENTRY pVadList;    // List of mapped regions
} MEM_PHYS_PROCESS_ENTRY, *PMEM_PHYS_PROCESS_ENTRY;

extern LIST_ENTRY g_PhysProcesses;


/// <summary>
/// Disable process DEP
/// Has no effect on native x64 process
/// </summary>
/// <param name="pData">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBDisableDEP( IN PDISABLE_DEP pData );

/// <summary>
/// Enable/disable process protection flag
/// </summary>
/// <param name="pProtection">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBSetProtection( IN PSET_PROC_PROTECTION pProtection );

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBGrantAccess( IN PHANDLE_GRANT_ACCESS pAccess );

/// <summary>
/// Allocate/Free process memory
/// </summary>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreeMemory( IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult );

/// <summary>
/// Read/write process memory
/// </summary>
/// <param name="pCopy">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBCopyMemory( IN PCOPY_MEMORY pCopy );

/// <summary>
/// Change process memory protection
/// </summary>
/// <param name="pProtect">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectMemory( IN PPROTECT_MEMORY pProtect );

/// <summary>
/// Hide VAD containing target address
/// </summary>
/// <param name="pData">Address info</param>
/// <returns>Status code</returns>
NTSTATUS BBHideVAD( IN PHIDE_VAD pData );

/// <summary>
/// Inject dll into process
/// </summary>
/// <param name="pid">Target PID</param>
/// <param name="pPath">TFull-qualified dll path</param>
/// <returns>Status code</returns>
NTSTATUS BBInjectDll( IN PINJECT_DLL pData );

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkHandleTable( IN PUNLINK_HTABLE pUnlink );

/// <summary>
/// Process termination handler
/// </summary>
/// <param name="ParentId">Parent PID</param>
/// <param name="ProcessId">PID</param>
/// <param name="Create">TRUE if process was created</param>
VOID BBProcessNotify( IN HANDLE ParentId, IN HANDLE ProcessId, IN BOOLEAN Create );

/// <summary>
/// Find memory allocation process entry
/// </summary>
/// <param name="pid">Target PID</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_PROCESS_ENTRY BBLookupPhysProcessEntry( IN HANDLE pid );

/// <summary>
/// Allocate kernel memory and map into User space. Or free previously allocated memory
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreePhysical( IN PEPROCESS pProcess, IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult );


//
// Memory allocation cleanup routines
//
void BBCleanupPhysMemEntry( IN PMEM_PHYS_ENTRY pEntry, BOOLEAN attached );
void BBCleanupProcessPhysEntry( IN PMEM_PHYS_PROCESS_ENTRY pEntry, BOOLEAN attached );
void BBCleanupProcessPhysList();
