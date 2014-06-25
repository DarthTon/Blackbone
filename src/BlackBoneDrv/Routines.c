#include "BlackBoneDrv.h"
#include <Ntstrsafe.h>

#pragma alloc_text(PAGE, BBDisableDEP)
#pragma alloc_text(PAGE, BBSetProtection)
#pragma alloc_text(PAGE, BBGrantAccess)
#pragma alloc_text(PAGE, BBCopyMemory)
#pragma alloc_text(PAGE, BBAllocateFreeMemory)
#pragma alloc_text(PAGE, BBProtectMemory)

/// <summary>
/// Disable process DEP
/// Has no effect on native x64 process
/// </summary>
/// <param name="pData">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBDisableDEP( IN PDISABLE_DEP pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKEXECUTE_OPTIONS pExecOpt = NULL;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        if (dynData.KExecOpt != 0)
        {
            pExecOpt = (PKEXECUTE_OPTIONS)((PUCHAR)pProcess + dynData.KExecOpt);

            // Reset all flags
            pExecOpt->ExecuteOptions = 0;

            pExecOpt->Flags.ExecuteEnable = 1;          //
            pExecOpt->Flags.ImageDispatchEnable = 1;    // Disable all checks
            pExecOpt->Flags.ExecuteDispatchEnable = 1;  //
        }
        else
        {
            DPRINT( "BlackBone: %s: Invalid _KEXECUTE_OPTIONS offset\n", __FUNCTION__ );
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Enable/disable process protection flag
/// </summary>
/// <param name="pProtection">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBSetProtection( IN PSET_PROC_PROTECTION pProtection )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pProtection->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        if (dynData.Protection != 0)
        {
#ifdef _WIN7_
            if (pProtection->enableState)
                *(PULONG)((PUCHAR)pProcess + dynData.Protection) |= 1 << 0xB;
            else
                *(PULONG)((PUCHAR)pProcess + dynData.Protection) &= ~(1 << 0xB);
#elif _WIN8_
            *((PUCHAR)pProcess + dynData.Protection) = pProtection->enableState;
#else
            PS_PROTECTION protBuf = { 0 };

            if (pProtection->enableState == FALSE)
            {
                protBuf.Level = 0;
            }
            else
            {
                protBuf.Flags.Signer = PsProtectedSignerWinTcb;
                protBuf.Flags.Type = PsProtectedTypeProtected;
            }

            *((PUCHAR)pProcess + dynData.Protection) = protBuf.Level;
#endif
        }
        else
        {
            DPRINT( "BlackBone: %s: Invalid protection flag offset\n", __FUNCTION__ );
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBGrantAccess( IN PGRANT_ACCESS pAccess )
{
    NTSTATUS  status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;
    PHANDLE_TABLE pTable = NULL;
    PHANDLE_TABLE_ENTRY pHandleEntry = NULL;
    EXHANDLE exHandle;

    status = PsLookupProcessByProcessId( (HANDLE)pAccess->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        pTable = *(PHANDLE_TABLE*)((PUCHAR)pProcess + dynData.ObjTable);
        exHandle.Value = (ULONG_PTR)pAccess->handle;

        if (pTable)
            pHandleEntry = ExpLookupHandleTableEntry( pTable, exHandle );

        if (pHandleEntry)
        {
            /*POBJECT_HEADER pHeader = (POBJECT_HEADER)ObpDecodeObject( pHandleEntry->LowValue );

            DPRINT( "BlackBone: %s: pHandleEntry = 0x%p -> 0x%p : 0x%X\n", __FUNCTION__,
                    pHandleEntry, &pHeader->Body, pHandleEntry->GrantedAccessBits );*/

            pHandleEntry->GrantedAccessBits = pAccess->access;
        }
        else
        {
            DPRINT( "BlackBone: %s: ExpLookupHandleTableEntry failed\n", __FUNCTION__ );
            status = STATUS_UNSUCCESSFUL;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    return status;
}

/// <summary>
/// Read/write process memory
/// </summary>
/// <param name="pCopy">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBCopyMemory( IN PCOPY_MEMORY pCopy )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL, pSourceProc = NULL, pTargetProc = NULL;
    PVOID pSource = NULL, pTarget = NULL;

    __try
    {
        status = PsLookupProcessByProcessId( (HANDLE)pCopy->pid, &pProcess );

        if (NT_SUCCESS( status ))
        {
            SIZE_T bytes = 0;

            // Write
            if (pCopy->write != FALSE)
            {
                pSourceProc = PsGetCurrentProcess();
                pTargetProc = pProcess;
                pSource = (PVOID)pCopy->localbuf;
                pTarget = (PVOID)pCopy->targetPtr;
            }
            // Read
            else
            {
                pSourceProc = pProcess;
                pTargetProc = PsGetCurrentProcess();
                pSource = (PVOID)pCopy->targetPtr;
                pTarget = (PVOID)pCopy->localbuf;
            }

            status = MmCopyVirtualMemory( pSourceProc, pSource, pTargetProc, pTarget, pCopy->size, KernelMode, &bytes );
        }
        else
            DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
        status = STATUS_UNHANDLED_EXCEPTION;
    }

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}


/// <summary>
/// Allocate/Free process memory
/// </summary>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreeMemory( IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pAllocFree->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        PVOID base = (PVOID)pAllocFree->base;
        ULONG_PTR size = pAllocFree->size;

        KeStackAttachProcess( pProcess, &apc );

        if (pAllocFree->allocate)
            status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &base, 0, &size, pAllocFree->type, pAllocFree->protection );
        else
            status = ZwFreeVirtualMemory( ZwCurrentProcess(), &base, &size, pAllocFree->type );

        KeUnstackDetachProcess( &apc );

        if (pResult != NULL)
        {
            pResult->address = (ULONGLONG)base;
            pResult->size = size;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Change process memory protection
/// </summary>
/// <param name="pProtect">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectMemory( IN PPROTECT_MEMORY pProtect )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pProtect->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        PVOID base = (PVOID)pProtect->base;
        SIZE_T size = (SIZE_T)pProtect->size;
        ULONG oldProt = 0;

        KeStackAttachProcess( pProcess, &apc );

        status = ZwProtectVirtualMemory( ZwCurrentProcess(), &base, &size, pProtect->newProtection, &oldProt );

        KeUnstackDetachProcess( &apc );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}