#include "BlackBoneDrv.h"
#include "Remap.h"
#include "Loader.h"

#include <ntstrsafe.h>

#pragma alloc_text(PAGE, BBDispatch)

/// <summary>
/// CTL dispatcher
/// </summary>
/// <param name="DeviceObject">Device object</param>
/// <param name="Irp">IRP</param>
/// <returns>Status code</returns>
NTSTATUS BBDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PVOID ioBuffer = NULL;
    ULONG inputBufferLength = 0;
    ULONG outputBufferLength = 0;
    ULONG ioControlCode = 0;

    UNREFERENCED_PARAMETER( DeviceObject );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    irpStack = IoGetCurrentIrpStackLocation( Irp );
    ioBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:
        {
            ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

            switch (ioControlCode)
            {
                case IOCTL_BLACKBONE_DISABLE_DEP:
                    {
                        if (inputBufferLength >= sizeof( DISABLE_DEP ) && ioBuffer)
                            Irp->IoStatus.Status = BBDisableDEP( (PDISABLE_DEP)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_SET_PROTECTION:
                    {
                        if (inputBufferLength >= sizeof( SET_PROC_PROTECTION ) && ioBuffer)
                            Irp->IoStatus.Status = BBSetProtection( (PSET_PROC_PROTECTION)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_GRANT_ACCESS:
                    {
                        if (inputBufferLength >= sizeof( HANDLE_GRANT_ACCESS ) && ioBuffer)
                            Irp->IoStatus.Status = BBGrantAccess( (PHANDLE_GRANT_ACCESS)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_COPY_MEMORY:
                    {
                        if (inputBufferLength >= sizeof( COPY_MEMORY ) && ioBuffer)
                            Irp->IoStatus.Status = BBCopyMemory( (PCOPY_MEMORY)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_ALLOCATE_FREE_MEMORY:
                    {
                        if (inputBufferLength >= sizeof( ALLOCATE_FREE_MEMORY ) &&
                             outputBufferLength >= sizeof( ALLOCATE_FREE_MEMORY_RESULT ) &&
                             ioBuffer)
                        {
                            ALLOCATE_FREE_MEMORY_RESULT result = { 0 };
                            Irp->IoStatus.Status = BBAllocateFreeMemory( (PALLOCATE_FREE_MEMORY)ioBuffer, &result );

                            if (NT_SUCCESS( Irp->IoStatus.Status ))
                            {
                                RtlCopyMemory( ioBuffer, &result, sizeof( result ) );
                                Irp->IoStatus.Information = sizeof( result );
                            }
                        }
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_PROTECT_MEMORY:
                    {
                        if (inputBufferLength >= sizeof( PROTECT_MEMORY ) && ioBuffer)
                            Irp->IoStatus.Status = BBProtectMemory( (PPROTECT_MEMORY)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_MAP_MEMORY:
                    {
                        if (inputBufferLength >= sizeof( MAP_MEMORY ) && ioBuffer && outputBufferLength >= sizeof( ULONG ))
                        {
                            ULONG_PTR sizeRequired = 0;
                            PPROCESS_MAP_ENTRY pProcessEntry = NULL;

                            Irp->IoStatus.Status = BBMapMemory( (PMAP_MEMORY)ioBuffer, &pProcessEntry );

                            if (NT_SUCCESS( Irp->IoStatus.Status ) && pProcessEntry != NULL)
                            {
                                BBGetRequiredRemapOutputSize( &pProcessEntry->pageList, &sizeRequired );

                                // Return mapping results
                                if (outputBufferLength >= sizeRequired)
                                {
                                    PMAP_MEMORY_RESULT pResult = (PMAP_MEMORY_RESULT)ioBuffer;

                                    //
                                    // Fill output
                                    //
                                    pResult->count = 0;
                                    pResult->hostPage = (ULONGLONG)pProcessEntry->host.sharedPage;
                                    pResult->targetPage = (ULONGLONG)pProcessEntry->target.sharedPage;
                                    pResult->pipeHandle = (ULONGLONG)pProcessEntry->targetPipe;

                                    for (PLIST_ENTRY pListEntry = pProcessEntry->pageList.Flink;
                                          pListEntry != &pProcessEntry->pageList;
                                          pListEntry = pListEntry->Flink)
                                    {
                                        PMAP_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MAP_ENTRY, link );

                                        pResult->entries[pResult->count].originalPtr = (ULONGLONG)pEntry->mem.BaseAddress;
                                        pResult->entries[pResult->count].newPtr = pEntry->newPtr;
                                        pResult->entries[pResult->count].size = (ULONG)pEntry->mem.RegionSize;
                                        pResult->count++;
                                    }

                                    Irp->IoStatus.Information = sizeRequired;
                                }
                                // Return number of bytes required 
                                else
                                {
                                    *(ULONG*)ioBuffer = (ULONG)sizeRequired;
                                    Irp->IoStatus.Information = sizeof( ULONG );
                                }
                            }
                        }
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_MAP_REGION:
                    {
                        if (inputBufferLength >= sizeof( MAP_MEMORY_REGION ) && 
                             outputBufferLength >= sizeof( MAP_MEMORY_REGION_RESULT ) && ioBuffer)
                        {
                            MAP_MEMORY_REGION_RESULT result = { 0 };
                            Irp->IoStatus.Status = BBMapMemoryRegion( (PMAP_MEMORY_REGION)ioBuffer, &result );

                            if (NT_SUCCESS( Irp->IoStatus.Status ))
                            {
                                RtlCopyMemory( ioBuffer, &result, sizeof( result ) );
                                Irp->IoStatus.Information = sizeof( result );
                            }
                        }
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_UNMAP_MEMORY:
                    {
                        if (inputBufferLength >= sizeof( UNMAP_MEMORY ) && ioBuffer)
                            Irp->IoStatus.Status = BBUnmapMemory( (PUNMAP_MEMORY)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_UNMAP_REGION:
                    {
                        if (inputBufferLength >= sizeof( UNMAP_MEMORY_REGION ) && ioBuffer)
                            Irp->IoStatus.Status = BBUnmapMemoryRegion( (PUNMAP_MEMORY_REGION)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_HIDE_VAD:
                    {
                        if (inputBufferLength >= sizeof( HIDE_VAD ) && ioBuffer)
                            Irp->IoStatus.Status = BBHideVAD( (PHIDE_VAD)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_INJECT_DLL:
                    {
                        if (inputBufferLength >= sizeof( INJECT_DLL ) && ioBuffer)
                            Irp->IoStatus.Status = BBInjectDll( (PINJECT_DLL)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_MAP_DRIVER:
                    {
                        if (inputBufferLength >= sizeof( MMAP_DRIVER ) && ioBuffer)
                        {
                            wchar_t buf[sizeof( ((PMMAP_DRIVER)ioBuffer)->FullPath )];
                            UNICODE_STRING ustrPath;

                            RtlCopyMemory( buf, ((PMMAP_DRIVER)ioBuffer)->FullPath, sizeof( ((PMMAP_DRIVER)ioBuffer)->FullPath ) );
                            RtlUnicodeStringInit( &ustrPath, buf );
                            Irp->IoStatus.Status = BBMMapDriver( &ustrPath );
                        }
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_UNLINK_HTABLE:
                    {
                        if (inputBufferLength >= sizeof( UNLINK_HTABLE ) && ioBuffer)
                            Irp->IoStatus.Status = BBUnlinkHandleTable( (PUNLINK_HTABLE)ioBuffer );
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break;

                case IOCTL_BLACKBONE_ENUM_REGIONS:
                    {
                        if (inputBufferLength >= sizeof( ENUM_REGIONS ) && outputBufferLength >= sizeof( ENUM_REGIONS_RESULT ) && ioBuffer)
                        {
                            ULONG count = (outputBufferLength - sizeof( ULONGLONG )) / sizeof( MEMORY_BASIC_INFORMATION );
                            PENUM_REGIONS_RESULT pResult = ExAllocatePoolWithTag( PagedPool, outputBufferLength, BB_POOL_TAG );
                            pResult->count = count;

                            Irp->IoStatus.Status = BBEnumMemRegions( (PENUM_REGIONS)ioBuffer, pResult );

                            // Full info
                            if (NT_SUCCESS( Irp->IoStatus.Status ))
                            {
                                Irp->IoStatus.Information = sizeof( pResult->count ) + pResult->count * sizeof( MEMORY_BASIC_INFORMATION );
                                RtlCopyMemory( ioBuffer, pResult, Irp->IoStatus.Information );
                            }
                            // Size only
                            else
                            {
                                Irp->IoStatus.Status = STATUS_SUCCESS;
                                Irp->IoStatus.Information = sizeof( pResult->count );
                                RtlCopyMemory( ioBuffer, pResult, sizeof( pResult->count ) );
                            }

                            ExFreePoolWithTag( pResult, BB_POOL_TAG );
                        }
                        else
                            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    break; 

                default:
                    DPRINT( "BlackBone: %s: Unknown IRP_MJ_DEVICE_CONTROL 0x%X\n", __FUNCTION__, ioControlCode );
                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    break;
            }
        }
            break;
    }

    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}