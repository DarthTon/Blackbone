#include "BlackBoneDrv.h"
#include "Remap.h"
#include "Loader.h"
#include "Utils.h"
#include <Ntstrsafe.h>

// OS Dependant data
DYNAMIC_DATA dynData;

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath );
NTSTATUS BBInitDynamicData( IN OUT PDYNAMIC_DATA pData );
NTSTATUS BBGetBuildNO( OUT PULONG pBuildNo );
NTSTATUS BBScanSection( IN PCCHAR section, IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, OUT PVOID* ppFound );
NTSTATUS BBLocatePageTables( IN OUT PDYNAMIC_DATA pData );
VOID     BBUnload( IN PDRIVER_OBJECT DriverObject );

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, BBInitDynamicData)
#pragma alloc_text(INIT, BBGetBuildNO)
#pragma alloc_text(INIT, BBScanSection)
#pragma alloc_text(INIT, BBLocatePageTables)

/*
*/
NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    UNICODE_STRING deviceName;
    UNICODE_STRING deviceLink;

    UNREFERENCED_PARAMETER( RegistryPath );

    // Get OS Dependant offsets
    status = BBInitDynamicData( &dynData );
    if (!NT_SUCCESS( status ))
    {
        DPRINT( "BlackBone: %s: Unsupported OS version. Aborting\n", __FUNCTION__ );
        return status;
    }

    // Initialize some loader structures
    status = BBInitLdrData( (PKLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection );
    if (!NT_SUCCESS( status ))
        return status;
    //
    // Globals init
    //
    InitializeListHead( &g_PhysProcesses );
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

    // Cleanup physical regions
    BBCleanupProcessPhysList();

    // Cleanup process mapping info
    BBCleanupProcessTable();

    RtlUnicodeStringInit( &deviceLinkUnicodeString, DOS_DEVICE_NAME );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );
    IoDeleteDevice( DriverObject->DeviceObject );

    return;
}

/// <summary>
/// Find pattern in kernel PE section
/// </summary>
/// <param name="section">Section name</param>
/// <param name="pattern">Pattern data</param>
/// <param name="wildcard">Pattern wildcard symbol</param>
/// <param name="len">Pattern length</param>
/// <param name="ppFound">Found address</param>
/// <returns>Status code</returns>
NTSTATUS BBScanSection( IN PCCHAR section, IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, OUT PVOID* ppFound )
{
    ASSERT( ppFound != NULL );
    if (ppFound == NULL)
        return STATUS_INVALID_PARAMETER;

    PVOID base = GetKernelBase( NULL);
    if (!base)
        return STATUS_NOT_FOUND;

    PIMAGE_NT_HEADERS64 pHdr = RtlImageNtHeader( base );
    if (!pHdr)
        return STATUS_INVALID_IMAGE_FORMAT;

    PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(pHdr + 1);
    for (PIMAGE_SECTION_HEADER pSection = pFirstSection; pSection < pFirstSection + pHdr->FileHeader.NumberOfSections; pSection++)
    {
        ANSI_STRING s1, s2;
        RtlInitAnsiString( &s1, section );
        RtlInitAnsiString( &s2, (PCCHAR)pSection->Name );
        if (RtlCompareString( &s1, &s2, TRUE ) == 0)
        {
            PVOID ptr = NULL;
            NTSTATUS status = BBSearchPattern( pattern, wildcard, len, (PUCHAR)base + pSection->VirtualAddress, pSection->Misc.VirtualSize, &ptr );
            if (NT_SUCCESS( status ))
                *(PULONG)ppFound = (ULONG)((PUCHAR)ptr - (PUCHAR)base);

            return status;
        }       
    }

    return STATUS_NOT_FOUND;
}

/// <summary>
/// Get kernel build number
/// </summary>
/// <param name="pBuildNO">Build number.</param>
/// <returns>Status code</returns>
NTSTATUS BBGetBuildNO( OUT PULONG pBuildNo )
{
    ASSERT( pBuildNo != NULL );
    if (pBuildNo == 0)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING strRegKey = { 0 };
    UNICODE_STRING strRegValue = { 0 };
    UNICODE_STRING strVerVal = { 0 };
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES keyAttr = { 0 };
    RtlUnicodeStringInit( &strRegKey, L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion" );
    RtlUnicodeStringInit( &strRegValue, L"BuildLabEx" );

    InitializeObjectAttributes( &keyAttr, &strRegKey, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL );

    status = ZwOpenKey( &hKey, KEY_READ, &keyAttr );
    if (NT_SUCCESS( status ))
    {
        PKEY_VALUE_FULL_INFORMATION pValueInfo = ExAllocatePoolWithTag( PagedPool, 0x1000, BB_POOL_TAG );
        ULONG bytes = 0;

        if (pValueInfo)
        {
            status = ZwQueryValueKey( hKey, &strRegValue, KeyValueFullInformation, pValueInfo, 0x1000, &bytes );
            if (NT_SUCCESS( status ))
            {
                PWCHAR pData = (PWCHAR)((PUCHAR)pValueInfo->Name + pValueInfo->NameLength);
                for (ULONG i = 0; i < pValueInfo->DataLength; i++)
                {
                    if (pData[i] == L'.')
                    {
                        for (ULONG j = i + 1; j < pValueInfo->DataLength; j++)
                        {
                            if (pData[j] == L'.')
                            {
                                strVerVal.Buffer = &pData[i] + 1;
                                strVerVal.Length = strVerVal.MaximumLength = (USHORT)((j - i) * sizeof( WCHAR ));
                                status = RtlUnicodeStringToInteger( &strVerVal, 10, pBuildNo );

                                goto skip1;
                            }
                        }
                    }
                }

            skip1:;
            }

            ExFreePoolWithTag( pValueInfo, BB_POOL_TAG );
        }
        else
            status = STATUS_NO_MEMORY;

        ZwClose( hKey );
    }
    else
        DPRINT( "BlackBone: %s: ZwOpenKey failed with status 0x%X\n", __FUNCTION__, status );

    return status;

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
    ULONG buildNo = 0;

    if (pData == NULL)
        return STATUS_INVALID_ADDRESS;

    RtlZeroMemory( pData, sizeof( DYNAMIC_DATA ) );
    pData->DYN_PDE_BASE = PDE_BASE;
    pData->DYN_PTE_BASE = PTE_BASE;

    verInfo.dwOSVersionInfoSize = sizeof( verInfo );
    status = RtlGetVersion( (PRTL_OSVERSIONINFOW)&verInfo );

    if (status == STATUS_SUCCESS)
    {
        ULONG ver_short = (verInfo.dwMajorVersion << 8) | (verInfo.dwMinorVersion << 4) | verInfo.wServicePackMajor;
        pData->ver = (WinVer)ver_short;

        // Get kernel build number
        status = BBGetBuildNO( &buildNo );

        // Validate current driver version
        pData->correctBuild = TRUE;
    #if defined(_WIN7_)
        if (ver_short != WINVER_7 && ver_short != WINVER_7_SP1)
            return STATUS_NOT_SUPPORTED;
    #elif defined(_WIN8_)
        if (ver_short != WINVER_8)
            return STATUS_NOT_SUPPORTED;
    #elif defined (_WIN81_)
        if (ver_short != WINVER_81)
            return STATUS_NOT_SUPPORTED;
    #elif defined (_WIN10_)
        if (ver_short != WINVER_10 && ver_short != WINVER_10_AU && ver_short != WINVER_10_CU)
            return STATUS_NOT_SUPPORTED;
    #endif

        DPRINT( 
            "BlackBone: OS version %d.%d.%d.%d.%d - 0x%x\n",
            verInfo.dwMajorVersion,
            verInfo.dwMinorVersion,
            verInfo.dwBuildNumber,
            verInfo.wServicePackMajor,
            buildNo,
            ver_short
            );

        switch (ver_short)
        {
                // Windows 7
                // Windows 7 SP1
            case WINVER_7:
            case WINVER_7_SP1:
                pData->KExecOpt         = 0x0D2;
                pData->Protection       = 0x43C;  // Bitfield, bit index - 0xB
                pData->ObjTable         = 0x200;
                pData->VadRoot          = 0x448;
                pData->NtProtectIndex   = 0x04D;
                pData->NtCreateThdIndex = 0x0A5;
                pData->NtTermThdIndex   = 0x50;
                pData->PrevMode         = 0x1F6;
                pData->ExitStatus       = 0x380;
                pData->MiAllocPage      = (ver_short == WINVER_7_SP1) ? 0 : 0;
                if (ver_short == WINVER_7_SP1)
                {
                    if (NT_SUCCESS( BBScanSection( "PAGE", (PCUCHAR)"\x48\x8D\x56\x20\x48\x8B\x42\x08", 0xCC, 8, (PVOID)&pData->ExRemoveTable ) ))
                        pData->ExRemoveTable -= 0x36;
                }
                else
                    pData->ExRemoveTable = 0x32D404;
                break;

                // Windows 8
            case WINVER_8:
                pData->KExecOpt         = 0x1B7;
                pData->Protection       = 0x648;
                pData->ObjTable         = 0x408;
                pData->VadRoot          = 0x590;
                pData->NtProtectIndex   = 0x04E;
                pData->NtCreateThdIndex = 0x0AF;
                pData->NtTermThdIndex   = 0x51;
                pData->PrevMode         = 0x232;
                pData->ExitStatus       = 0x450;
                pData->MiAllocPage      = 0x3AF374;
                pData->ExRemoveTable    = 0x487518;
                break;

                // Windows 8.1
            case WINVER_81:
                pData->KExecOpt         = 0x1B7;
                pData->Protection       = 0x67A;
                pData->EProcessFlags2   = 0x2F8;
                pData->ObjTable         = 0x408;
                pData->VadRoot          = 0x5D8;
                pData->NtCreateThdIndex = 0xB0;
                pData->NtTermThdIndex   = 0x52;
                pData->PrevMode         = 0x232;
                pData->ExitStatus       = 0x6D8;
                pData->MiAllocPage      = 0;
                pData->ExRemoveTable    = 0x432A88; // 0x38E320;
                if (NT_SUCCESS( BBScanSection( "PAGE", (PCUCHAR)"\x48\x8D\x7D\x18\x48\x8B", 0xCC, 6, (PVOID)&pData->ExRemoveTable ) ))
                    pData->ExRemoveTable -= 0x5E;
                break;

                // Windows 10, build 15063/14393/10586
            case WINVER_10:
                if (verInfo.dwBuildNumber == 10586)
                {
                    pData->KExecOpt         = 0x1BF;
                    pData->Protection       = 0x6B2;
                    pData->EProcessFlags2   = 0x300;
                    pData->ObjTable         = 0x418;
                    pData->VadRoot          = 0x610;
                    pData->NtCreateThdIndex = 0xB4;
                    pData->NtTermThdIndex   = 0x53;
                    pData->PrevMode         = 0x232;
                    pData->ExitStatus       = 0x6E0;
                    pData->MiAllocPage      = 0;
                    if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8D\x7D\x18\x48\x8B", 0xCC, 6, (PVOID)&pData->ExRemoveTable)))
                        pData->ExRemoveTable -= 0x5C;
                    break;
                }
                else if (verInfo.dwBuildNumber == 14393)
                {
                    pData->ver              = WINVER_10_AU;
                    pData->KExecOpt         = 0x1BF;
                    pData->Protection       = 0x6C2;
                    pData->EProcessFlags2   = 0x300;
                    pData->ObjTable         = 0x418;
                    pData->VadRoot          = 0x620;
                    pData->NtCreateThdIndex = 0xB6;
                    pData->NtTermThdIndex   = 0x53;
                    pData->PrevMode         = 0x232;
                    pData->ExitStatus       = 0x6F0;
                    pData->MiAllocPage      = 0;
                    if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8D\x7D\x18\x48\x8B", 0xCC, 6, (PVOID)&pData->ExRemoveTable)))
                        pData->ExRemoveTable -= 0x60;

                    status = BBLocatePageTables( pData );
                    break;
                }
                else if (verInfo.dwBuildNumber == 15063)
                {
                    pData->ver              = WINVER_10_CU;
                    pData->KExecOpt         = 0x1BF;
                    pData->Protection       = 0x6CA;
                    pData->EProcessFlags2   = 0x300;
                    pData->ObjTable         = 0x418;
                    pData->VadRoot          = 0x628;
                    pData->NtCreateThdIndex = 0xB9;
                    pData->NtTermThdIndex   = 0x53;
                    pData->PrevMode         = 0x232;
                    pData->ExitStatus       = 0x6F8;
                    pData->MiAllocPage      = 0;
                    if (NT_SUCCESS(BBScanSection("PAGE", (PCUCHAR)"\x48\x8B\x47\x20\x48\x83\xC7\x18", 0xCC, 8, (PVOID)&pData->ExRemoveTable)))
                        pData->ExRemoveTable -= 0x34;

                    status = BBLocatePageTables( pData );
                    break;
                }
                else
                {
                    return STATUS_NOT_SUPPORTED;
                }
            default:
                break;
        }

        if (pData->ExRemoveTable != 0)
            pData->correctBuild = TRUE;

        DPRINT(
            "BlackBone: Dynamic search status: SSDT - %s, ExRemoveTable - %s\n",
            GetSSDTBase() != NULL ? "SUCCESS" : "FAIL", 
            pData->ExRemoveTable != 0 ? "SUCCESS" : "FAIL" 
            );
        
        return (pData->VadRoot != 0 ? status : STATUS_INVALID_KERNEL_INFO_VERSION);
    }

    return status;
}

/// <summary>
/// Get relocated PTE and PDE bases
/// </summary>
/// <param name="pData">Dynamic data</param>
/// <returns>Status code</returns>
NTSTATUS BBLocatePageTables( IN OUT PDYNAMIC_DATA pData )
{
    UNICODE_STRING uName = RTL_CONSTANT_STRING( L"MmGetPhysicalAddress" );
    PUCHAR pMmGetPhysicalAddress = MmGetSystemRoutineAddress( &uName );
    if (pMmGetPhysicalAddress)
    {
        PUCHAR pMiGetPhysicalAddress = *(PLONG)(pMmGetPhysicalAddress + 0xE + 1) + pMmGetPhysicalAddress + 0xE + 5;
        if (pData->ver >= WINVER_10_CU)
        {
            pData->DYN_PDE_BASE = *(PULONG_PTR)(pMiGetPhysicalAddress + 0x43 + 2);
            pData->DYN_PTE_BASE = *(PULONG_PTR)(pMiGetPhysicalAddress + 0x50 + 2);
        }
        else
        {
            pData->DYN_PDE_BASE = *(PULONG_PTR)(pMiGetPhysicalAddress + 0x49 + 2);
            pData->DYN_PTE_BASE = *(PULONG_PTR)(pMiGetPhysicalAddress + 0x56 + 2);
        }

        DPRINT( "BlackBone: PDE_BASE: %p, PTE_BASE: %p\n", pData->DYN_PDE_BASE, pData->DYN_PTE_BASE );
        return STATUS_SUCCESS;
    }

    DPRINT( "BlackBone: PDE_BASE/PTE_BASE not found \n" );
    return STATUS_NOT_FOUND;
}