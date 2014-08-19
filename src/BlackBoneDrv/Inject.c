#include "Private.h"
#include "Routines.h"
#include <Ntstrsafe.h>

extern DYNAMIC_DATA dynData;

PVOID BBGetWow64Code( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath );
PVOID BBGetNativeCode( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath );

NTSTATUS BBThreadInject( IN PVOID pUserBuf, IN BOOLEAN wait );
NTSTATUS BBApcInject( IN PVOID pUserBuf, IN HANDLE pid );

#pragma alloc_text(PAGE, BBInjectDll)
#pragma alloc_text(PAGE, BBGetWow64Code)
#pragma alloc_text(PAGE, BBGetNativeCode)
#pragma alloc_text(PAGE, BBThreadInject)
#pragma alloc_text(PAGE, BBApcInject)


/// <summary>
/// Inject dll into process
/// </summary>
/// <param name="pid">Target PID</param>
/// <param name="pPath">TFull-qualified dll path</param>
/// <returns>Status code</returns>
NTSTATUS BBInjectDll( IN PINJECT_DLL pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess);
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        UNICODE_STRING ustrPath, ustrNtdll;
        PVOID pNtdll = NULL;
        PVOID LdrLoadDll = NULL;
        BOOLEAN isWow64 = (PsGetProcessWow64Process( pProcess ) != NULL) ? TRUE : FALSE;

        KeStackAttachProcess( pProcess, &apc );

        RtlInitUnicodeString( &ustrPath, pData->FullDllPath );
        RtlInitUnicodeString( &ustrNtdll, L"Ntdll.dll" );

        // Get ntdll base
        pNtdll = GetModuleBase( pProcess, &ustrNtdll, isWow64 );

        if (!pNtdll)
        {
            DPRINT( "BlackBone: %s: Failed to get Ntdll base\n", __FUNCTION__ );
            status = STATUS_NOT_FOUND;
        }

        // Get LdrLoadDll address
        if (NT_SUCCESS( status ))
        {
            LdrLoadDll = GetModuleExport( pNtdll, "LdrLoadDll" );
            if (!LdrLoadDll)
            {
                DPRINT( "BlackBone: %s: Failed to get LdrLoadDll address\n", __FUNCTION__ );
                status = STATUS_NOT_FOUND;
            }
        }

        // Execute LdrLoadDll
        if (NT_SUCCESS( status ))
        {
            SIZE_T size = 0;
            PVOID pUserBuf = isWow64 ? BBGetWow64Code( LdrLoadDll, &ustrPath ) : BBGetNativeCode( LdrLoadDll, &ustrPath );

            if (pData->type == IT_Thread)
            {
                status = BBThreadInject( pUserBuf, pData->wait );
            }
            else if (pData->type == IT_Apc)
            {
                status = BBApcInject( pUserBuf, (HANDLE)pData->pid );                       
            }
            else
            {
                DPRINT( "BlackBone: %s: Invalid injection type specified - %d\n", __FUNCTION__, pData->type );
                status = STATUS_INVALID_PARAMETER;
            }

            // Memory remains allocated in APC case
            if (pData->type != IT_Apc)
                ZwFreeVirtualMemory( ZwCurrentProcess(), &pUserBuf, &size, MEM_RELEASE );
        }

        KeUnstackDetachProcess( &apc );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

PVOID BBGetWow64Code( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    SIZE_T size = 0x2000;

    // Code
    UCHAR code[] = 
    { 
        0x68, 0, 0, 0, 0,   // push ModuleHandle    offset +1 
        0x68, 0, 0, 0, 0,   // push ModuleFileName  offset +6
        0x6A, 0,            // push Flags  
        0x6A, 0,            // push PathToFile
        0xE8, 0, 0, 0, 0,   // call LdrLoadDll      offset +15
        0xC2, 4, 0          // ret 4
    };

    status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pBuffer, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (NT_SUCCESS( status ))
    { 
        // Copy path
        PUNICODE_STRING32 pUserPath = (PUNICODE_STRING32)((PUCHAR)pBuffer + 0x1000);
        pUserPath->Length = pPath->Length;
        pUserPath->MaximumLength = pPath->MaximumLength;
        pUserPath->Buffer = (ULONG)(pUserPath + 1);

        // Copy path
        memcpy( (PVOID)pUserPath->Buffer, pPath->Buffer, pPath->Length );

        // Copy code
        memcpy( pBuffer, code, sizeof( code ) );

        // Fill stubs
        *(ULONG*)((PUCHAR)pBuffer + 1)  = (ULONG)pBuffer + 0x500;
        *(ULONG*)((PUCHAR)pBuffer + 6)  = (ULONG)pUserPath;
        *(ULONG*)((PUCHAR)pBuffer + 15) = (ULONG)((ULONG_PTR)LdrLoadDll - ((ULONG_PTR)pBuffer + 15) - 5 + 1);

        return pBuffer;
    }

    UNREFERENCED_PARAMETER( pPath );
    return NULL;
}

PVOID BBGetNativeCode( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    SIZE_T size = 0x2000;

    // Code
    UCHAR code[] =
    {
        0x48, 0x83, 0xEC, 0x28,                 // sub rsp, 0x28
        0x48, 0x31, 0xC9,                       // xor rcx, rcx
        0x48, 0x31, 0xD2,                       // xor rdx, rdx
        0x49, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,     // mov r8, ModuleFileName  offset +12
        0x49, 0xB9, 0, 0, 0, 0, 0, 0, 0, 0,     // mov r9, ModuleHandle    offset +28
        0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,     // mov rax, LdrLoadDll     offset +32
        0xFF, 0xD0,                             // call rax
        0x48, 0x83, 0xC4, 0x28,                 // add rsp, 0x28
        0xC3                                    // ret
    };

    status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pBuffer, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (NT_SUCCESS( status ))
    {
        // Copy path
        PUNICODE_STRING pUserPath = (PUNICODE_STRING)((PUCHAR)pBuffer + 0x1000);
        pUserPath->Length = 0;
        pUserPath->MaximumLength = 0x1000;
        pUserPath->Buffer = (PWCH)(pUserPath + 1);

        RtlUnicodeStringCopy( pUserPath, pPath );

        // Copy code
        memcpy( pBuffer, code, sizeof( code ) );

        // Fill stubs
        *(ULONGLONG*)((PUCHAR)pBuffer + 12) = (ULONGLONG)pUserPath;
        *(ULONGLONG*)((PUCHAR)pBuffer + 22) = (ULONGLONG)pBuffer + 0x500;
        *(ULONGLONG*)((PUCHAR)pBuffer + 32) = (ULONGLONG)LdrLoadDll;

        return pBuffer;
    }

    UNREFERENCED_PARAMETER( pPath );
    return NULL;
}

NTSTATUS BBThreadInject( IN PVOID pUserBuf, IN BOOLEAN wait )
{
    HANDLE hThread = NULL;
    OBJECT_ATTRIBUTES ob = { 0 };
    ob.Length = sizeof( ob );

    NTSTATUS status = ZwCreateThreadEx( &hThread, THREAD_ALL_ACCESS, &ob, ZwCurrentProcess(), pUserBuf,
                                        NULL, 0, 0, 0x1000, 0x100000, NULL );

    // Wait for completion
    if (NT_SUCCESS( status ) && wait != FALSE)
    {
        PETHREAD pThread = NULL;
        status = ObReferenceObjectByHandle( hThread, THREAD_ALL_ACCESS, *PsThreadType, UserMode, &pThread, NULL );

        if (NT_SUCCESS( status ))
            KeWaitForSingleObject( pThread, Executive, KernelMode, TRUE, NULL );

        if (pThread)
            ObDereferenceObject( pThread );
    }
    else
        DPRINT( "BlackBone: %s: ZwCreateThreadEx failed with status 0x%X\n", __FUNCTION__, status );

    return status;
}

VOID KernelApcCallBack( PKAPC Apc,
                        PKNORMAL_ROUTINE* NormalRoutine,
                        PVOID* NormalContext,
                        PVOID* SystemArgument1,
                        PVOID* SystemArgument2 )
{
    UNREFERENCED_PARAMETER( NormalRoutine );
    UNREFERENCED_PARAMETER( NormalContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );
    
    // Skip execution
    if (PsIsThreadTerminating( PsGetCurrentThread() ))
        *NormalRoutine = NULL;

    // Fix Wow64 APC
    if (PsGetProcessWow64Process( PsGetCurrentProcess() ) != NULL)
        PsWrapApcWow64Thread( NormalContext, (PVOID*)NormalRoutine );

    ExFreePoolWithTag( Apc, BB_POOL_TAG );
}

#pragma warning(disable: 4055)

NTSTATUS BBApcInject( IN PVOID pUserBuf, IN HANDLE pid )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKAPC kApc = NULL;
    PETHREAD pThread = NULL;
    PVOID pBuf = ExAllocatePoolWithTag( NonPagedPool, 1024 * 1024, BB_POOL_TAG );
    PSYSTEM_PROCESS_INFO pInfo = (PSYSTEM_PROCESS_INFO)pBuf;

    if (!pInfo)
    {
        DPRINT( "BlackBone: %s: Failed to allocate memory for process list\n", __FUNCTION__ );
        return STATUS_NO_MEMORY;
    }

    // Get the process thread list
    status = ZwQuerySystemInformation( SystemProcessInformation, pInfo, 1024 * 1024, NULL );
    if (!NT_SUCCESS( status ))
    {
        ExFreePoolWithTag( pBuf, BB_POOL_TAG );
        return status;
    }

    // Find a target thread
    if (NT_SUCCESS( status ))        
    {
        status = STATUS_NOT_FOUND;
        for (; pInfo->NextEntryOffset; pInfo = (PSYSTEM_PROCESS_INFO)((PUCHAR)pInfo + pInfo->NextEntryOffset))
        {
            if (pInfo->UniqueProcessId == pid)
            {
                status = STATUS_SUCCESS;
                break;
            }
        }
    }

    // Reference target thread
    if (NT_SUCCESS( status ))
        status = PsLookupThreadByThreadId( pInfo->Threads[0].ClientId.UniqueThread, &pThread );
    else
        DPRINT( "BlackBone: %s: Failed to locate process\n", __FUNCTION__ );

    if (NT_SUCCESS( status ))
    {
        // Allocate APC
        kApc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), BB_POOL_TAG );
        if (kApc == NULL)
        {
            DPRINT( "BlackBone: %s: Failed to allocate APC\n", __FUNCTION__ );
            status = STATUS_NO_MEMORY;
        }

        if (NT_SUCCESS( status ))
        {
            // Queue APC
            KeInitializeApc( kApc, (PKTHREAD)pThread, OriginalApcEnvironment, &KernelApcCallBack,
                             NULL, (PKNORMAL_ROUTINE)pUserBuf, UserMode, NULL );

            // Enforce kernel APC
            if (KeInsertQueueApc( kApc, NULL, NULL, 0 ))
                ((PKAPC_STATE)((PUCHAR)pThread + dynData.ApcState))->UserApcPending = TRUE;
            else
                status = STATUS_NOT_CAPABLE;
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to locate thread\n", __FUNCTION__ );

    if (pThread)
        ObDereferenceObject( pThread );

    if (pBuf)
        ExFreePoolWithTag( pBuf, BB_POOL_TAG );

    if (!NT_SUCCESS( status ) && kApc)
        ExFreePoolWithTag( kApc, BB_POOL_TAG );

    return status;
}

#pragma warning(default: 4055)