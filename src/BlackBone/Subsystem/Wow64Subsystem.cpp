#include "Wow64Subsystem.h"
#include "../Misc/DynImport.h"
#include "../Include/Macro.h"

namespace blackbone
{

NativeWow64::NativeWow64( HANDLE hProcess )
    : Native( hProcess )
{
    HMODULE ntdll32 = GetModuleHandleW( L"Ntdll.dll" );

    LOAD_IMPORT( "NtWow64QueryInformationProcess64", ntdll32 );
    LOAD_IMPORT( "NtWow64AllocateVirtualMemory64",   ntdll32 );
    LOAD_IMPORT( "NtWow64QueryVirtualMemory64",      ntdll32 );
    LOAD_IMPORT( "NtWow64ReadVirtualMemory64",       ntdll32 );
    LOAD_IMPORT( "NtWow64WriteVirtualMemory64",      ntdll32 );
}

NativeWow64::~NativeWow64()
{
}

/// <summary>
/// Allocate virtual memory
/// </summary>
/// <param name="lpAddress">Allocation address</param>
/// <param name="dwSize">Region size</param>
/// <param name="flAllocationType">Allocation type</param>
/// <param name="flProtect">Memory protection</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::VirtualAllocExT( ptr_t& lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect )
{
    DWORD64 size64 = dwSize;
    static ptr_t ntavm = _local.GetProcAddress64( _local.getNTDLL64(), "NtAllocateVirtualMemory" );
    if (ntavm == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( ntavm, _hProcess, &lpAddress, 0, &size64, flAllocationType, flProtect ));
}

/// <summary>
/// Free virtual memory
/// </summary>
/// <param name="lpAddress">Memory address</param>
/// <param name="dwSize">Region size</param>
/// <param name="dwFreeType">Memory release type.</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::VirtualFreeExT( ptr_t lpAddress, size_t dwSize, DWORD dwFreeType )
{
    static ptr_t ntfvm = _local.GetProcAddress64( _local.getNTDLL64( ), "NtFreeVirtualMemory" );
    if (ntfvm == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    DWORD64 tmpAddr = lpAddress;
    DWORD64 tmpSize = dwSize;

    return static_cast<NTSTATUS>(_local.X64Call( ntfvm, _hProcess, &tmpAddr, &tmpSize, dwFreeType ));
}

/// <summary>
/// Query virtual memory
/// </summary>
/// <param name="lpAddress">Address to query</param>
/// <param name="lpBuffer">Retrieved memory info</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::VirtualQueryExT( ptr_t lpAddress, PMEMORY_BASIC_INFORMATION64 lpBuffer )
{
    static ptr_t ntqvm = _local.GetProcAddress64( _local.getNTDLL64(), "NtQueryVirtualMemory" );
    if (ntqvm == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( ntqvm, _hProcess, lpAddress, 0, lpBuffer, sizeof( MEMORY_BASIC_INFORMATION64 ), 0 ));
}

/// <summary>
/// Query virtual memory
/// </summary>
/// <param name="lpAddress">Address to query</param>
/// <param name="lpBuffer">Retrieved memory info</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::VirtualQueryExT( ptr_t lpAddress, MEMORY_INFORMATION_CLASS infoClass, LPVOID lpBuffer, size_t bufSize )
{
    static ptr_t ntqvm = _local.GetProcAddress64( _local.getNTDLL64(), "NtQueryVirtualMemory" );
    if (ntqvm == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( ntqvm, _hProcess, lpAddress, infoClass, lpBuffer, bufSize, 0 ));
}

/// <summary>
/// Change memory protection
/// </summary>
/// <param name="lpAddress">Memory address.</param>
/// <param name="dwSize">Region size</param>
/// <param name="flProtect">New protection.</param>
/// <param name="flOld">Old protection</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::VirtualProtectExT( ptr_t lpAddress, DWORD64 dwSize, DWORD flProtect, DWORD* flOld )
{
    static ptr_t ntpvm = _local.GetProcAddress64( _local.getNTDLL64(), "NtProtectVirtualMemory" );
    if (ntpvm == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( ntpvm, _hProcess, &lpAddress, &dwSize, flProtect, flOld ));
}

/// <summary>
/// Read virtual memory
/// </summary>
/// <param name="lpBaseAddress">Memory address</param>
/// <param name="lpBuffer">Output buffer</param>
/// <param name="nSize">Number of bytes to read</param>
/// <param name="lpBytes">Mumber of bytes read</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::ReadProcessMemoryT( ptr_t lpBaseAddress, LPVOID lpBuffer, size_t nSize, DWORD64 *lpBytes /*= nullptr */ )
{
    DWORD64 junk = 0;
    if (lpBytes == nullptr)
        lpBytes = &junk;

    return SAFE_NATIVE_CALL( NtWow64ReadVirtualMemory64, _hProcess, lpBaseAddress, lpBuffer, nSize, lpBytes );
}

/// <summary>
/// Write virtual memory
/// </summary>
/// <param name="lpBaseAddress">Memory address</param>
/// <param name="lpBuffer">Buffer to write</param>
/// <param name="nSize">Number of bytes to read</param>
/// <param name="lpBytes">Mumber of bytes read</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::WriteProcessMemoryT( ptr_t lpBaseAddress, LPCVOID lpBuffer, size_t nSize, DWORD64 *lpBytes /*= nullptr */ )
{
    DWORD64 junk = 0;
    if (lpBytes == nullptr)
        lpBytes = &junk;

    return SAFE_NATIVE_CALL( NtWow64WriteVirtualMemory64, _hProcess, lpBaseAddress, (LPVOID)lpBuffer, nSize, lpBytes );
}

/// <summary>
/// Call NtQueryInformationProcess for underlying process
/// </summary>
/// <param name="infoClass">Information class</param>
/// <param name="lpBuffer">Output buffer</param>
/// <param name="bufSize">Buffer size</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::QueryProcessInfoT( PROCESSINFOCLASS infoClass, LPVOID lpBuffer, uint32_t bufSize )
{
    ULONG length = 0;
    return SAFE_NATIVE_CALL( NtWow64QueryInformationProcess64, _hProcess, infoClass, lpBuffer, bufSize, &length );
}


/// <summary>
/// Call NtSetInformationProcess for underlying process
/// </summary>
/// <param name="infoClass">Information class</param>
/// <param name="lpBuffer">Input buffer</param>
/// <param name="bufSize">Buffer size</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::SetProcessInfoT( PROCESSINFOCLASS infoClass, LPVOID lpBuffer, uint32_t bufSize )
{
    static ptr_t ntspi = _local.GetProcAddress64( _local.getNTDLL64(), "NtSetInformationProcess" );
    if (ntspi == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( ntspi, _hProcess, infoClass, lpBuffer, bufSize ));
}

/// <summary>
/// Creates new thread in the remote process
/// </summary>
/// <param name="hThread">Created thread handle</param>
/// <param name="entry">Thread entry point</param>
/// <param name="arg">Thread argument</param>
/// <param name="flags">Creation flags</param>
/// <returns>Status code</returns>*/
NTSTATUS NativeWow64::CreateRemoteThreadT( HANDLE& hThread, ptr_t entry, ptr_t arg, CreateThreadFlags flags, DWORD access )
{
    // Try to use default routine if possible
    /*if(_wowBarrier.targetWow64 == true)
    {
        return Native::CreateRemoteThreadT( hThread, entry, arg, flags, access );
    }
    else*/
    {
        LastNtStatus( STATUS_SUCCESS );

        static DWORD64 NtCreateThreadEx = _local.GetProcAddress64( _local.getNTDLL64(), "NtCreateThreadEx" );

        if (NtCreateThreadEx == 0)
            return LastNtStatus( STATUS_ORDINAL_NOT_FOUND );

        // hThread can't be used directly because x64Call will zero stack space near variable
        DWORD64 hThd2 = NULL;

        NTSTATUS status = static_cast<NTSTATUS>(_local.X64Call(
            NtCreateThreadEx, &hThd2, access, NULL,
            _hProcess, entry, arg, flags,
            0, 0x1000, 0x100000, NULL
            ));

        hThread = reinterpret_cast<HANDLE>(hThd2);
        return status;
    }
}


/// <summary>
/// Get WOW64 thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::GetThreadContextT( HANDLE hThread, _CONTEXT32& ctx )
{
    // Target process is x64. 32bit CONTEXT is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return STATUS_NOT_SUPPORTED;
    }
    else
    {
        LastNtStatus( STATUS_SUCCESS );
        GetThreadContext( hThread, reinterpret_cast<PCONTEXT>(&ctx) );
        return LastNtStatus();
    }
}


/// <summary>
/// Get native thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::GetThreadContextT( HANDLE hThread, _CONTEXT64& ctx )
{
    static ptr_t gtc = _local.GetProcAddress64( _local.getNTDLL64(), "NtGetContextThread" );
    if (gtc == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( gtc, hThread, &ctx ));
}

/// <summary>
/// Set WOW64 thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::SetThreadContextT( HANDLE hThread, _CONTEXT32& ctx )
{
    // Target process is x64. 32bit CONTEXT is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return STATUS_NOT_SUPPORTED;
    }
    else
    {
        LastNtStatus( STATUS_SUCCESS );
        SetThreadContext( hThread, reinterpret_cast<const CONTEXT*>(&ctx) );
        return LastNtStatus();
    }
}

/// <summary>
/// Set native thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS NativeWow64::SetThreadContextT( HANDLE hThread, _CONTEXT64& ctx )
{
    static ptr_t stc = _local.GetProcAddress64( _local.getNTDLL64( ), "NtSetContextThread" );
    if (stc == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    return static_cast<NTSTATUS>(_local.X64Call( stc, hThread, &ctx ));
}

/// <summary>
/// Gets WOW64 PEB
/// </summary>
/// <param name="ppeb">Retrieved PEB</param>
/// <returns>PEB pointer</returns>
ptr_t NativeWow64::getPEB( _PEB32* ppeb )
{
    // Target process is x64. PEB32 is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return 0;
    }
    else
    {
        PROCESS_BASIC_INFORMATION pbi = { 0 };
        ULONG bytes = 0;

        if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationProcess, _hProcess, ProcessBasicInformation, &pbi, (ULONG)sizeof( pbi ), &bytes ) ) && ppeb)
            ReadProcessMemory( _hProcess, pbi.PebBaseAddress, ppeb, sizeof(_PEB32), NULL );

        return reinterpret_cast<ptr_t>(pbi.PebBaseAddress);
    }
}

/// <summary>
/// Get native PEB
/// </summary>
/// <param name="ppeb">Retrieved PEB</param>
/// <returns>PEB pointer</returns>
ptr_t NativeWow64::getPEB( _PEB64* ppeb )
{
    _PROCESS_BASIC_INFORMATION_T<DWORD64> info = { 0 };
    ULONG bytes = 0;

    SAFE_NATIVE_CALL( NtWow64QueryInformationProcess64, _hProcess, ProcessBasicInformation, &info, (ULONG)sizeof( info ), &bytes );
    if (bytes > 0 && NT_SUCCESS( SAFE_NATIVE_CALL( NtWow64ReadVirtualMemory64, _hProcess, info.PebBaseAddress, ppeb, (ULONG)sizeof( _PEB64 ), nullptr ) ))
        return info.PebBaseAddress;

    return 0;
}

/// <summary>
/// Get WOW64 TEB
/// </summary>
/// <param name="ppeb">Retrieved TEB</param>
/// <returns>TEB pointer</returns>
ptr_t NativeWow64::getTEB( HANDLE hThread, _TEB32* pteb )
{
    // Target process is x64. TEB32 is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return 0;
    }
    else
    {
        _THREAD_BASIC_INFORMATION_T<DWORD> tbi = { 0 };
        ULONG bytes = 0;
        
        if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationThread, hThread, (THREADINFOCLASS)0, &tbi, (ULONG)sizeof( tbi ), &bytes ) ) && pteb)
            ReadProcessMemory( _hProcess, (LPCVOID)((uintptr_t)tbi.TebBaseAddress), pteb, sizeof( _TEB32 ), nullptr );

        return static_cast<ptr_t>(tbi.TebBaseAddress);
    }
}

/// <summary>
/// Get native TEB
/// </summary>
/// <param name="ppeb">Retrieved TEB</param>
/// <returns>TEB pointer</returns>
ptr_t NativeWow64::getTEB( HANDLE hThread, _TEB64* pteb )
{
    _THREAD_BASIC_INFORMATION_T<DWORD64> info = { 0 };
    ULONG bytes = 0;

    static ptr_t ntQit = _local.GetProcAddress64( _local.getNTDLL64(), "NtQueryInformationThread" );

    if (ntQit == 0)
    {
        LastNtStatus( STATUS_ORDINAL_NOT_FOUND );
        return 0;
    }

    _local.X64Call( ntQit, hThread, 0, &info, sizeof(info), &bytes );

    if (bytes > 0 && NT_SUCCESS( SAFE_NATIVE_CALL( NtWow64ReadVirtualMemory64, _hProcess, info.TebBaseAddress, pteb, sizeof( _TEB64 ), nullptr ) ))
        return static_cast<ptr_t>(info.TebBaseAddress);

    return 0;
}

}