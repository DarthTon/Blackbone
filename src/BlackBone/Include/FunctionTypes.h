#pragma once

#include "../Config.h"
#include "NativeStructures.h"

namespace blackbone
{

// NtCreateEvent
typedef NTSTATUS( NTAPI* fnNtCreateEvent )(
    OUT PHANDLE             EventHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN ULONG                EventType,
    IN BOOLEAN              InitialState
    );

// NtOpenEvent
typedef NTSTATUS( NTAPI* fnNtOpenEvent )(
    OUT PHANDLE             EventHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes
    );

// NtQueryVirtualMemory
typedef NTSTATUS( NTAPI* fnNtQueryVirtualMemory )(
    IN HANDLE   ProcessHandle,
    IN PVOID    BaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID   MemoryInformation,
    IN SIZE_T   MemoryInformationLength,
    OUT PSIZE_T ReturnLength
    );

// NtWow64QueryInformationProcess64
typedef NTSTATUS( NTAPI *fnNtWow64QueryInformationProcess64 )(
    IN  HANDLE ProcessHandle,
    IN  ULONG  ProcessInformationClass,
    OUT PVOID  ProcessInformation64,
    IN  ULONG  Length,
    OUT PULONG ReturnLength OPTIONAL
    );

// NtWow64ReadVirtualMemory64
typedef NTSTATUS( NTAPI *fnNtWow64ReadVirtualMemory64 )(
    IN  HANDLE   ProcessHandle,
    IN  ULONG64  BaseAddress,
    OUT PVOID    Buffer,
    IN  ULONG64  BufferLength,
    OUT PULONG64 ReturnLength OPTIONAL
    );

// NtWow64WriteVirtualMemory64
using fnNtWow64WriteVirtualMemory64 = fnNtWow64ReadVirtualMemory64;

// NtWow64AllocateVirtualMemory64
typedef NTSTATUS( NTAPI *fnNtWow64AllocateVirtualMemory64 )(
    IN  HANDLE   ProcessHandle,
    IN  PULONG64 BaseAddress,
    IN  ULONG64  ZeroBits,
    IN  PULONG64 Size,
    IN  ULONG    AllocationType,
    IN  ULONG    Protection
    );

// NtWow64QueryVirtualMemory64
typedef NTSTATUS( NTAPI *fnNtWow64QueryVirtualMemory64 )(
    IN HANDLE   ProcessHandle,
    IN ULONG64  BaseAddress,
    IN DWORD    MemoryInformationClass,
    OUT PVOID   Buffer,
    IN ULONG64  Length,
    OUT PULONG  ResultLength OPTIONAL
    );

// RtlDosApplyFileIsolationRedirection_Ustr
typedef NTSTATUS( NTAPI *fnRtlDosApplyFileIsolationRedirection_Ustr )(
    IN ULONG Flags,
    IN PUNICODE_STRING OriginalName,
    IN PUNICODE_STRING Extension,
    IN OUT PUNICODE_STRING StaticString,
    IN OUT PUNICODE_STRING DynamicString,
    IN OUT PUNICODE_STRING *NewName,
    IN PULONG  NewFlags,
    IN PSIZE_T FileNameSize,
    IN PSIZE_T RequiredLength
    );

// RtlDosPathNameToNtPathName_U
typedef BOOLEAN( NTAPI *fnRtlDosPathNameToNtPathName_U )(
    IN PCWSTR DosFileName,
    OUT PUNICODE_STRING NtFileName,
    OUT OPTIONAL PWSTR *FilePart,
    OUT OPTIONAL PVOID RelativeName
    );

// RtlHashUnicodeString
typedef NTSTATUS( NTAPI *fnRtlHashUnicodeString )(
    IN   PCUNICODE_STRING String,
    IN   BOOLEAN CaseInSensitive,
    IN   ULONG HashAlgorithm,
    OUT  PULONG HashValue
    );

// RtlRemoteCall
typedef NTSTATUS( NTAPI *fnRtlRemoteCall )(
    IN HANDLE Process,
    IN HANDLE Thread,
    IN PVOID CallSite,
    IN ULONG ArgumentCount,
    IN PULONG Arguments,
    IN BOOLEAN PassContext,
    IN BOOLEAN AlreadySuspended
    );

// NtCreateThreadEx
typedef NTSTATUS( NTAPI* fnNtCreateThreadEx )(
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN LPVOID ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD Flags,
    IN SIZE_T StackZeroBits,
    IN SIZE_T SizeOfStackCommit,
    IN SIZE_T SizeOfStackReserve,
    OUT LPVOID lpBytesBuffer
    );

// NtLockVirtualMemory
typedef NTSTATUS( NTAPI* fnNtLockVirtualMemory )(
    IN HANDLE process,
    IN OUT PVOID* baseAddress,
    IN OUT ULONG* size,
    IN ULONG flags
    );

// RtlRbInsertNodeEx
typedef int (NTAPI* fnRtlRbInsertNodeEx)(
    _RTL_RB_TREE<DWORD_PTR>*        Tree,
    _RTL_BALANCED_NODE<DWORD_PTR>*  Parent,
    BOOLEAN                         Right,
    _RTL_BALANCED_NODE<DWORD_PTR> * Node
    );

// NtSetInformationProcess
typedef NTSTATUS( NTAPI* fnNtSetInformationProcess )(
    IN HANDLE   ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID    ProcessInformation,
    IN ULONG    ProcessInformationLength
    );

// NtDuplicateObject
typedef NTSTATUS( NTAPI* fnNtDuplicateObject )(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    IN PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Attributes,
    IN ULONG Options
    );

// RtlRbRemoveNode
typedef int (NTAPI* fnRtlRbRemoveNode)(
    _RTL_RB_TREE<DWORD_PTR>*        Tree,
    _RTL_BALANCED_NODE<DWORD_PTR>*  Node
    );

// RtlUpcaseUnicodeChar
typedef WCHAR( NTAPI *fnRtlUpcaseUnicodeChar )(
    WCHAR chr
    );

// RtlEncodeSystemPointer
typedef PVOID( NTAPI *fnRtlEncodeSystemPointer )(
    IN PVOID Pointer
    );

// NtLoadDriver
typedef NTSTATUS( NTAPI* fnNtLoadDriver )(
    IN PUNICODE_STRING path
    );

// NtUnloadDriver
typedef NTSTATUS( NTAPI* fnNtUnloadDriver )(
    IN PUNICODE_STRING path
    );

// NtQuerySection
typedef DWORD( NTAPI* fnNtQuerySection )(
    HANDLE hSection,
    SECTION_INFORMATION_CLASS InfoClass,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG ReturnLength
    );

// NtSuspendProcess
typedef NTSTATUS( NTAPI* fnNtSuspendProcess )(
    HANDLE ProcessHandle
    );

// NtResumeProcess
typedef NTSTATUS( NTAPI* fnNtResumeProcess )(
    HANDLE ProcessHandle
    );

// RtlCreateActivationContext
typedef NTSTATUS( NTAPI *fnRtlCreateActivationContext )(
    IN ULONG    Flags,
    IN PACTCTXW ActivationContextData,
    IN ULONG    ExtraBytes,
    IN PVOID    NotificationRoutine,
    IN PVOID    NotificationContext,
    OUT PVOID*  ActCtx
    );

// NtQueueApcThread
typedef NTSTATUS( NTAPI* fnNtQueueApcThread )(
    IN HANDLE ThreadHandle,
    IN PVOID ApcRoutine, /*PKNORMAL_ROUTINE*/
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

// RtlQueueApcWow64Thread
using fnRtlQueueApcWow64Thread = fnNtQueueApcThread;

// RtlImageNtHeader
typedef PIMAGE_NT_HEADERS( NTAPI* fnRtlImageNtHeader )(
    IN PVOID ModuleAddress
    );

// RtlInitUnicodeString
using fnRtlInitUnicodeString = decltype(&RtlInitUnicodeString);

// RtlFreeUnicodeString
using fnRtlFreeUnicodeString = decltype(&RtlFreeUnicodeString);

// NtQuerySystemInformation
using fnNtQuerySystemInformation = decltype(&NtQuerySystemInformation);

// NtQueryInformationProcess
using fnNtQueryInformationProcess = decltype(&NtQueryInformationProcess);

// NtQueryInformationThread
using fnNtQueryInformationThread = decltype(&NtQueryInformationThread);

// NtQueryObject
using fnNtQueryObject = decltype(&NtQueryObject);

//
// GCC compatibility
//

// Wow64GetThreadContext
typedef BOOL( __stdcall* fnWow64GetThreadContext )
    (
        HANDLE hThread,
        PWOW64_CONTEXT lpContext
    );

// Wow64SetThreadContext
typedef BOOL( __stdcall* fnWow64SetThreadContext )
    (
        HANDLE hThread,
        const WOW64_CONTEXT *lpContext
    );

// Wow64SuspendThread
typedef DWORD( __stdcall* fnWow64SuspendThread )
    (
        HANDLE hThread
    );

// GetProcessDEPPolicy
typedef BOOL( __stdcall* fnGetProcessDEPPolicy )
    (
        HANDLE  hProcess,
        LPDWORD lpFlags,
        PBOOL   lpPermanent
    );

// QueryFullProcessImageNameW
typedef BOOL( __stdcall* fnQueryFullProcessImageNameW)
    (
        HANDLE hProcess,
        DWORD  dwFlags,
        PWSTR  lpExeName,
        PDWORD lpdwSize
    );

}