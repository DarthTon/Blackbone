#pragma once

#ifdef _M_IX86
#error "x86 systems are not supported"
#endif

#include <ntifs.h>

#include "BlackBoneDef.h"
#include "Routines.h"
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
