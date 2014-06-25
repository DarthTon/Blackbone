#include "BlackBoneDrv.h"
#include "Remap.h"
#include <Ntstrsafe.h>

// OS Dependant data
DYNAMIC_DATA dynData;

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath );
NTSTATUS BBInitDynamicData( IN OUT PDYNAMIC_DATA pData );
VOID     BBUnload( IN PDRIVER_OBJECT DriverObject );


#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, BBUnload)
#pragma alloc_text(PAGE, BBInitDynamicData)


typedef struct _KSERVICE_TABLE_DESCRIPTOR 
{
    PULONG_PTR Base;
    PULONG Count;
    ULONG Limit;
    PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

DECLSPEC_CACHEALIGN KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[2];


/*
*/
NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    UNICODE_STRING deviceName;
    UNICODE_STRING deviceLink;

    UNREFERENCED_PARAMETER( DriverObject );
    UNREFERENCED_PARAMETER( RegistryPath );

    // Get OS Dependant offsets
    status = BBInitDynamicData( &dynData );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Unsupported OS version. Aborting\n", __FUNCTION__ );
        return status;
    }

    //
    // Globals init
    //
    RtlInitializeGenericTableAvl( &g_ProcessPageTables, &AvlCompare, &AvlAllocate, &AvlFree, NULL );
    KeInitializeGuardedMutex( &g_globalLock );

    // Setup process termination notifier
    status = PsSetCreateProcessNotifyRoutine( BBProcessNotify, FALSE );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Failed to setup notify routine with staus 0x%X\n", __FUNCTION__, status );
        return status;
    }

    RtlUnicodeStringInit( &deviceName, DEVICE_NAME );
     
    status = IoCreateDevice( DriverObject, 0, &deviceName, FILE_DEVICE_BLACKBONE, 0, FALSE, &deviceObject );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: IoCreateDevice failed with status 0x%X\n", __FUNCTION__, status );
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]          =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = BBDispatch;
    DriverObject->DriverUnload                          = BBUnload;

    RtlUnicodeStringInit( &deviceLink, DOS_DEVICE_NAME );

    status = IoCreateSymbolicLink( &deviceLink, &deviceName );

    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: IoCreateSymbolicLink failed with status 0x%X\n", __FUNCTION__, status );
        IoDeleteDevice (deviceObject);
    }

    return status;
}


/*
*/
VOID BBUnload( IN PDRIVER_OBJECT DriverObject )
{
    UNICODE_STRING deviceLinkUnicodeString;

    // Unregister notification
    PsSetCreateProcessNotifyRoutine( BBProcessNotify, TRUE );

    // Cleanup process mapping info
    BBCleanupProcessTable();

    RtlUnicodeStringInit( &deviceLinkUnicodeString, DOS_DEVICE_NAME );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );
    IoDeleteDevice( DriverObject->DeviceObject );

    return;
}


/// <summary>
/// Initialize dynamic data.
/// </summary>
/// <param name="pData">Data to initialize</param>
/// <returns>Status code</returns>
NTSTATUS BBInitDynamicData( IN OUT PDYNAMIC_DATA pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    RTL_OSVERSIONINFOEXW verInfo = { 0 };

    if (pData == NULL)
        return STATUS_INVALID_ADDRESS;

    RtlZeroMemory( pData, sizeof( DYNAMIC_DATA ) );

    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    status = RtlGetVersion( (PRTL_OSVERSIONINFOW)&verInfo );

    if (status == STATUS_SUCCESS)
    {
        ULONG ver_short = (verInfo.dwMajorVersion << 8) | (verInfo.dwMinorVersion << 4) | verInfo.wServicePackMajor;

        pData->ver = (WinVer)ver_short;

        DPRINT( "BlackBone: OS version %d.%d.%d.%d.%d - 0x%x\n",
                verInfo.dwMajorVersion,
                verInfo.dwMinorVersion,
                verInfo.dwBuildNumber,
                verInfo.wServicePackMajor,
                verInfo.wServicePackMinor,
                ver_short );

        switch (ver_short)
        {
                // Windows 7
            case WINVER_7:
            case WINVER_7_SP1:
                pData->KExecOpt       = 0x0D2;
                pData->Protection     = 0x43C;  // Bitfield, bit index - 0xB
                pData->ObjTable       = 0x200;
                pData->VadRoot        = 0x448;
                pData->NtProtectIndex = 0x04D;
                pData->PrevMode       = 0x1F6;
                break;

                // Windows 8
            case WINVER_8:
                pData->KExecOpt       = 0x1B7;
                pData->Protection     = 0x648;
                pData->ObjTable       = 0x408;
                pData->VadRoot        = 0x590;
                pData->NtProtectIndex = 0x04E;
                pData->PrevMode       = 0x232;
                break;

                // Windows 8.1
            case WINVER_81:
                pData->KExecOpt       = 0x1B7;
                pData->Protection     = 0x67A;
                pData->ObjTable       = 0x408;
                pData->VadRoot        = 0x5D8;
                break;

            default:
                break;
        }

        return (pData->VadRoot != 0 ? status : STATUS_NOT_SUPPORTED);
    }

    return status;
}
