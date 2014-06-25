#pragma once

#ifdef _M_IX86
#error "x86 systems are not supported"
#endif

#include <ntifs.h>

#include "BlackBoneDef.h"
#include "Remap.h"

#define DEVICE_NAME     L"\\Device\\"     ## BLACKBONE_DEVICE_NAME
#define DOS_DEVICE_NAME L"\\DosDevices\\" ## BLACKBONE_DEVICE_NAME

/// <summary>
/// CTL dispatcher
/// </summary>
/// <param name="DeviceObject">Device object</param>
/// <param name="Irp">IRP</param>
/// <returns>Status code</returns>
NTSTATUS BBDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

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
NTSTATUS BBGrantAccess( IN PGRANT_ACCESS pAccess );

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