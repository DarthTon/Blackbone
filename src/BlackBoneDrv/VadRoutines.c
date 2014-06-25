#include "VadRoutines.h"

#pragma alloc_text(PAGE, BBProtectVAD)
#pragma alloc_text(PAGE, BBHideVAD)


extern DYNAMIC_DATA dynData;


/// <summary>
/// Change VAD protection flags
/// </summary>
/// <param name="pProcess">Process object</param>
/// <param name="address">target address</param>
/// <param name="prot">New protection flags</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectVAD( IN PEPROCESS pProcess, IN ULONG_PTR address, IN ULONG prot )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR vpnStart = address >> PAGE_SHIFT;

    ASSERT( pProcess != NULL );
    if (pProcess == NULL)
        return STATUS_INVALID_PARAMETER;

    if (dynData.VadRoot == 0)
    {
        DPRINT( "BlackBone: %s: Invalid VadRoot offset\n", __FUNCTION__ );
        status = STATUS_INVALID_ADDRESS;
    }

    __try
    {
        PMM_AVL_TABLE pTable = (PMM_AVL_TABLE)((PUCHAR)pProcess + dynData.VadRoot);
        PMM_AVL_NODE pNode = GET_VAD_ROOT( pTable );

        // Search VAD
        if (MiFindNodeOrParent( pTable, (ULONG_PTR)vpnStart, &pNode ) == TableFoundNode)
        {
            PMMVAD_SHORT pVad = (PMMVAD_SHORT)pNode;
            pVad->u.VadFlags.Protection = prot;
        }
        else
        {
            DPRINT( "BlackBone: %s: VAD entry for address 0x%p not found\n", __FUNCTION__, address );
            status = STATUS_NOT_FOUND;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
        status = STATUS_UNHANDLED_EXCEPTION;
    }

    return status;
}


/// <summary>
/// Hide memory from NtQueryVirtualMemory
/// </summary>
/// <param name="pProcess">Process object</param>
/// <param name="address">Target address</param>
/// <returns>Status code</returns>
NTSTATUS BBHideVAD( IN PEPROCESS pProcess, IN ULONG_PTR address )
{
    //NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR vpnStart = address >> PAGE_SHIFT;
    UNREFERENCED_PARAMETER( pProcess );
    UNREFERENCED_PARAMETER( vpnStart );

    // Not ready yet
    return STATUS_NOT_IMPLEMENTED;
    /*
    if (ofst_current.ofsVadRoot == 0)
    {
    DPRINT( "BlackBone: %s: Invalid VadRoot offset\n", __FUNCTION__ );
    status = STATUS_INVALID_ADDRESS;
    }

    if (pProcess == NULL)
    pProcess = IoGetCurrentProcess();

    __try
    {
    PMM_AVL_TABLE pTable = (PMM_AVL_TABLE)((PUCHAR)pProcess + ofst_current.ofsVadRoot);
    PMM_AVL_NODE pNode = GET_VAD_ROOT( pTable );

    // Search VAD
    if (MiFindNodeOrParent( pTable, (ULONG_PTR)vpnStart, &pNode ) == TableFoundNode)
    {
    PMMVAD_SHORT pVad = (PMMVAD_SHORT)pNode;

    // Set VAD type to physical mapping and prevent access
    pVad->u.VadFlags.VadType = VadDevicePhysicalMemory;
    pVad->u.VadFlags.Protection = MM_ZERO_ACCESS;
    }
    else
    {
    DPRINT( "BlackBone: %s: VAD entry for address 0x%p not found\n", __FUNCTION__, address );
    status = STATUS_NOT_FOUND;
    }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
    status = STATUS_UNHANDLED_EXCEPTION;
    }

    return status;*/
}

