#include "Private.h"
#include "Routines.h"
#include "Loader.h"
#include <Ntstrsafe.h>

#define CODE_OFFSET     0x000
#define STRING_OFFSET   0x200
#define MOD_OFFSET      0x1F0
#define COMPLETE_OFFSET 0x1E0

#define CALL_COMPLETE   0xC0371E7E

extern DYNAMIC_DATA dynData;

PVOID BBGetWow64Code( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath );
PVOID BBGetNativeCode( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath );

NTSTATUS BBApcInject( IN PVOID pUserBuf, IN HANDLE pid );
NTSTATUS BBLookupProcessThread( IN HANDLE pid, OUT PETHREAD* ppThread );

VOID KernelApcPrepareCallback( PKAPC, PKNORMAL_ROUTINE*, PVOID*, PVOID*, PVOID* );
VOID KernelApcInjectCallback( PKAPC, PKNORMAL_ROUTINE*, PVOID*, PVOID*, PVOID* );
VOID ApcWaitThread( IN PVOID pUserBuf );

#pragma alloc_text(PAGE, BBInjectDll)
#pragma alloc_text(PAGE, BBGetWow64Code)
#pragma alloc_text(PAGE, BBGetNativeCode)
#pragma alloc_text(PAGE, BBExecuteInNewThread)
#pragma alloc_text(PAGE, BBApcInject)
#pragma alloc_text(PAGE, BBQueueUserApc)
#pragma alloc_text(PAGE, BBLookupProcessThread)
#pragma alloc_text(PAGE, ApcWaitThread)

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
        SET_PROC_PROTECTION prot = { 0 };
        PVOID pNtdll = NULL;
        PVOID LdrLoadDll = NULL;
        BOOLEAN isWow64 = (PsGetProcessWow64Process( pProcess ) != NULL) ? TRUE : FALSE;

        KeStackAttachProcess( pProcess, &apc );

        RtlInitUnicodeString( &ustrPath, pData->FullDllPath );
        RtlInitUnicodeString( &ustrNtdll, L"Ntdll.dll" );

        // Get ntdll base
        pNtdll = BBGetUserModuleBase( pProcess, &ustrNtdll, isWow64 );

        if (!pNtdll)
        {
            DPRINT( "BlackBone: %s: Failed to get Ntdll base\n", __FUNCTION__ );
            status = STATUS_NOT_FOUND;
        }

        // Get LdrLoadDll address
        if (NT_SUCCESS( status ))
        {
            LdrLoadDll = BBGetModuleExport( pNtdll, "LdrLoadDll" );
            if (!LdrLoadDll)
            {
                DPRINT( "BlackBone: %s: Failed to get LdrLoadDll address\n", __FUNCTION__ );
                status = STATUS_NOT_FOUND;
            }
        }

        // If process is protected - temporarily disable protection
        if (PsIsProtectedProcess( pProcess ))
        {
            prot.pid = pData->pid;
            prot.enableState = FALSE;
            BBSetProtection( &prot );
        }

        // Call LdrLoadDll
        if (NT_SUCCESS( status ))
        {
            SIZE_T size = 0;
            PVOID pUserBuf = isWow64 ? BBGetWow64Code( LdrLoadDll, &ustrPath ) : BBGetNativeCode( LdrLoadDll, &ustrPath );

            if (pData->type == IT_Thread)
            {
                NTSTATUS threadStatus = STATUS_SUCCESS;

                status = BBExecuteInNewThread( pUserBuf, NULL, THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, pData->wait, &threadStatus );

                // Injection failed
                if (!NT_SUCCESS( threadStatus ))
                {
                    status = threadStatus;
                    DPRINT( "BlackBone: %s: User thread failed with status - 0x%X\n", __FUNCTION__, status );
                }
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

            ZwFreeVirtualMemory( ZwCurrentProcess(), &pUserBuf, &size, MEM_RELEASE );
        }

        // Restore protection
        if (prot.pid != 0)
        {
            prot.enableState = TRUE;
            BBSetProtection( &prot );
        }

        KeUnstackDetachProcess( &apc );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Create new thread in the target process
/// Must be running in target process context
/// </summary>
/// <param name="pBaseAddress">Thread start address</param>
/// <param name="pParam">Thread argument</param>
/// <param name="flags">Thread creation flags</param>
/// <param name="wait">If set to TRUE - wait for thread completion</param>
/// <param name="pExitStatus">Thread exit status</param>
/// <returns>Status code</returns>
NTSTATUS BBExecuteInNewThread(
    IN PVOID pBaseAddress,
    IN PVOID pParam,
    IN ULONG flags,
    IN BOOLEAN wait,
    OUT PNTSTATUS pExitStatus 
    )
{
    HANDLE hThread = NULL;
    OBJECT_ATTRIBUTES ob = { 0 };
    ob.Length = sizeof( ob );

    NTSTATUS status = ZwCreateThreadEx( &hThread, THREAD_ALL_ACCESS, &ob, ZwCurrentProcess(), pBaseAddress,
                                        pParam, flags, 0, 0x1000, 0x100000, NULL );

    // Wait for completion
    if (NT_SUCCESS( status ) && wait != FALSE)
    {
        PETHREAD pThread = NULL;
        status = ObReferenceObjectByHandle( hThread, THREAD_ALL_ACCESS, *PsThreadType, UserMode, &pThread, NULL );

        if (NT_SUCCESS( status ))
        {
            // Force 60 sec timeout
            LARGE_INTEGER timeout = { 0 };
            timeout.QuadPart = -(60ll * 10 * 1000 * 1000);

            KeWaitForSingleObject( pThread, Executive, KernelMode, TRUE, &timeout );

            // Get exit code
            if (pExitStatus)
                *pExitStatus = *(NTSTATUS*)((PUCHAR)pThread + dynData.ExitStatus);
        }

        if (pThread)
            ObDereferenceObject( pThread );
    }
    else
        DPRINT( "BlackBone: %s: ZwCreateThreadEx failed with status 0x%X\n", __FUNCTION__, status );

    return status;
}

/// <summary>
/// Build injection code for wow64 process
/// Must be running in target process context
/// </summary>
/// <param name="LdrLoadDll">LdrLoadDll address</param>
/// <param name="pPath">Path to the dll</param>
/// <returns>Code pointer. When not needed, it should be freed with ZwFreeVirtualMemory</returns>
PVOID BBGetWow64Code( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    SIZE_T size = PAGE_SIZE;

    // Code
    UCHAR code[] = 
    { 
        0x68, 0, 0, 0, 0,                       // push ModuleHandle            offset +1 
        0x68, 0, 0, 0, 0,                       // push ModuleFileName          offset +6
        0x6A, 0,                                // push Flags  
        0x6A, 0,                                // push PathToFile
        0xE8, 0, 0, 0, 0,                       // call LdrLoadDll              offset +15
        0xBA, 0, 0, 0, 0,                       // mov edx, COMPLETE_OFFSET     offset +20
        0xC7, 0x02, 0x7E, 0x1E, 0x37, 0xC0,     // mov [edx], CALL_COMPLETE     
        0xC2, 0x04, 0x00                        // ret 4
    };

    status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pBuffer, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (NT_SUCCESS( status ))
    { 
        // Copy path
        PUNICODE_STRING32 pUserPath = (PUNICODE_STRING32)((PUCHAR)pBuffer + STRING_OFFSET);
        pUserPath->Length = pPath->Length;
        pUserPath->MaximumLength = pPath->MaximumLength;
        pUserPath->Buffer = (ULONG)(pUserPath + 1);

        // Copy path
        memcpy( (PVOID)pUserPath->Buffer, pPath->Buffer, pPath->Length );

        // Copy code
        memcpy( pBuffer, code, sizeof( code ) );

        // Fill stubs
        *(ULONG*)((PUCHAR)pBuffer + 1)  = (ULONG)pBuffer + MOD_OFFSET;
        *(ULONG*)((PUCHAR)pBuffer + 6)  = (ULONG)pUserPath;
        *(ULONG*)((PUCHAR)pBuffer + 15) = (ULONG)((ULONG_PTR)LdrLoadDll - ((ULONG_PTR)pBuffer + 15) - 5 + 1);
        *(ULONG*)((PUCHAR)pBuffer + 20) = (ULONG)pBuffer + COMPLETE_OFFSET;

        return pBuffer;
    }

    UNREFERENCED_PARAMETER( pPath );
    return NULL;
}

/// <summary>
/// Build injection code for native x64 process
/// Must be running in target process context
/// </summary>
/// <param name="LdrLoadDll">LdrLoadDll address</param>
/// <param name="pPath">Path to the dll</param>
/// <returns>Code pointer. When not needed it should be freed with ZwFreeVirtualMemory</returns>
PVOID BBGetNativeCode( IN PVOID LdrLoadDll, IN PUNICODE_STRING pPath )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    SIZE_T size = PAGE_SIZE;

    // Code
    UCHAR code[] =
    {
        0x48, 0x83, 0xEC, 0x28,                 // sub rsp, 0x28
        0x48, 0x31, 0xC9,                       // xor rcx, rcx
        0x48, 0x31, 0xD2,                       // xor rdx, rdx
        0x49, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,     // mov r8, ModuleFileName   offset +12
        0x49, 0xB9, 0, 0, 0, 0, 0, 0, 0, 0,     // mov r9, ModuleHandle     offset +28
        0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,     // mov rax, LdrLoadDll      offset +32
        0xFF, 0xD0,                             // call rax
        0x48, 0xBA, 0, 0, 0, 0, 0, 0, 0, 0,     // mov rdx, COMPLETE_OFFSET offset +44
        0xC7, 0x02, 0x7E, 0x1E, 0x37, 0xC0,     // mov [rdx], CALL_COMPLETE 
        0x48, 0x83, 0xC4, 0x28,                 // add rsp, 0x28
        0xC3                                    // ret
    };

    status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &pBuffer, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if (NT_SUCCESS( status ))
    {
        // Copy path
        PUNICODE_STRING pUserPath = (PUNICODE_STRING)((PUCHAR)pBuffer + STRING_OFFSET);
        pUserPath->Length = 0;
        pUserPath->MaximumLength = (USHORT)size - STRING_OFFSET;
        pUserPath->Buffer = (PWCH)(pUserPath + 1);

        RtlUnicodeStringCopy( pUserPath, pPath );

        // Copy code
        memcpy( pBuffer, code, sizeof( code ) );

        // Fill stubs
        *(ULONGLONG*)((PUCHAR)pBuffer + 12) = (ULONGLONG)pUserPath;
        *(ULONGLONG*)((PUCHAR)pBuffer + 22) = (ULONGLONG)pBuffer + MOD_OFFSET;
        *(ULONGLONG*)((PUCHAR)pBuffer + 32) = (ULONGLONG)LdrLoadDll;
        *(ULONGLONG*)((PUCHAR)pBuffer + 44) = (ULONGLONG)pBuffer + COMPLETE_OFFSET;

        return pBuffer;
    }

    UNREFERENCED_PARAMETER( pPath );
    return NULL;
}

/// <summary>
/// Inject dll using APC
/// Must be running in target process context
/// </summary>
/// <param name="pUserBuf">Injcetion code</param>
/// <param name="pid">Target process ID</param>
/// <returns></returns>
NTSTATUS BBApcInject( IN PVOID pUserBuf, IN HANDLE pid )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD pThread = NULL;

    // Get suitable thread
    status = BBLookupProcessThread( pid, &pThread );

    if (NT_SUCCESS( status ))
    {
        status = BBQueueUserApc( pThread, pUserBuf, NULL, NULL );

        // Wait for completion
        if (NT_SUCCESS( status ))
        {
            LARGE_INTEGER interval = { 0 };
            interval.QuadPart = -(5LL * 10 * 1000);

            // Protect from UserMode AV
            __try
            {
                ULONG val = *(PULONG)((PUCHAR)pUserBuf + COMPLETE_OFFSET);
                for (ULONG i = 0; val != CALL_COMPLETE && i < 10000; i++)
                {
                    KeDelayExecutionThread( KernelMode, FALSE, &interval );
                    val = *(PULONG)((PUCHAR)pUserBuf + COMPLETE_OFFSET);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
                status = STATUS_ACCESS_VIOLATION;
            }            
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to locate thread\n", __FUNCTION__ );

    if (pThread)
        ObDereferenceObject( pThread );

    return status;
}

// 'type cast' : from data pointer 'PVOID' to function pointer 'PKNORMAL_ROUTINE'
#pragma warning(disable: 4055)

/// <summary>
/// Send user-mode APC to the target thread
/// </summary>
/// <param name="pThread">Target thread</param>
/// <param name="pUserFunc">APC function</param>
/// <param name="Arg1">Argument 1</param>
/// <param name="Arg2">Argument 2</param>
/// <returns>Status code</returns>
NTSTATUS BBQueueUserApc( IN PETHREAD pThread, IN PVOID pUserFunc, IN PVOID Arg1, IN PVOID Arg2 )
{
    ASSERT( pThread != NULL );
    if (pThread == NULL)
        return STATUS_INVALID_PARAMETER;

    // Allocate APC
    PKAPC pInjectApc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), BB_POOL_TAG );
    PKAPC pPrepareApc = ExAllocatePoolWithTag( NonPagedPool, sizeof( KAPC ), BB_POOL_TAG );

    if (pInjectApc == NULL)
    {
        DPRINT( "BlackBone: %s: Failed to allocate APC\n", __FUNCTION__ );
        return STATUS_NO_MEMORY;
    }

    // This APC will make thread alertable
    KeInitializeApc( pPrepareApc, (PKTHREAD)pThread, OriginalApcEnvironment, &KernelApcPrepareCallback,
                     NULL, NULL, KernelMode, NULL );

    // Actual APC
    KeInitializeApc( pInjectApc, (PKTHREAD)pThread, OriginalApcEnvironment, &KernelApcInjectCallback,
                     NULL, (PKNORMAL_ROUTINE)pUserFunc, UserMode, NULL );

    // Enforce kernel APC
    if (KeInsertQueueApc( pInjectApc, Arg1, Arg2, 0 ))
    {
        KeInsertQueueApc( pPrepareApc, NULL, NULL, 0 );
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT( "BlackBone: %s: Failed to insert APC\n", __FUNCTION__ );

        ExFreePoolWithTag( pInjectApc, BB_POOL_TAG );
        return STATUS_NOT_CAPABLE;
    }
}

#pragma warning(default: 4055)

/// <summary>
/// Find first thread of the target process
/// </summary>
/// <param name="pid">Target PID.</param>
/// <param name="ppThread">Found thread. Thread object reference count is increased by 1</param>
/// <returns>Status code</returns>
NTSTATUS BBLookupProcessThread( IN HANDLE pid, OUT PETHREAD* ppThread )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pBuf = ExAllocatePoolWithTag( NonPagedPool, 1024 * 1024, BB_POOL_TAG );
    PSYSTEM_PROCESS_INFO pInfo = (PSYSTEM_PROCESS_INFO)pBuf;

    ASSERT( ppThread != NULL );
    if (ppThread == NULL)
        return STATUS_INVALID_PARAMETER;

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
    {
        status = PsLookupThreadByThreadId( pInfo->Threads[0].ClientId.UniqueThread, ppThread );

        // Does not allow usage of currently executing thread
        if (NT_SUCCESS( status ) && *ppThread == PsGetCurrentThread())
        {
            // Try next thread
            if (pInfo->NumberOfThreads > 1)
            {
                ObDereferenceObject( *ppThread );
                *ppThread = NULL;

                status = PsLookupThreadByThreadId( pInfo->Threads[1].ClientId.UniqueThread, ppThread );
            }
            else
                status = STATUS_INVALID_THREAD;
        }
    }
    else
        DPRINT( "BlackBone: %s: Failed to locate process\n", __FUNCTION__ );

    if (pBuf)
        ExFreePoolWithTag( pBuf, BB_POOL_TAG );

    return status;
}

//
// Injection APC routines
//
VOID KernelApcPrepareCallback(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2
    )
{
    UNREFERENCED_PARAMETER( NormalRoutine );
    UNREFERENCED_PARAMETER( NormalContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    DPRINT( "BlackBone: %s: Called\n", __FUNCTION__ );

    // Alert current thread
    KeTestAlertThread( UserMode );
    ExFreePoolWithTag( Apc, BB_POOL_TAG );
}

VOID KernelApcInjectCallback(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2
    )
{
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    DPRINT( "BlackBone: %s: Called\n", __FUNCTION__ );

    // Skip execution
    if (PsIsThreadTerminating( PsGetCurrentThread() ))
        *NormalRoutine = NULL;

    // Fix Wow64 APC
    if (PsGetCurrentProcessWow64Process() != NULL)
        PsWrapApcWow64Thread( NormalContext, (PVOID*)NormalRoutine );

    ExFreePoolWithTag( Apc, BB_POOL_TAG );
}
