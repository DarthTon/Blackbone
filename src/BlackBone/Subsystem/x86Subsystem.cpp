#include "x86Subsystem.h"
#include "../Misc/DynImport.h"
#include "../Include/Macro.h"

namespace blackbone
{

x86Native::x86Native( HANDLE hProcess ) noexcept
    : Native( hProcess, true )
{
}

/// <summary>
/// Query virtual memory
/// </summary>
/// <param name="lpAddress">Address to query</param>
/// <param name="lpBuffer">Retrieved memory info</param>
/// <returns>Status code</returns>
NTSTATUS x86Native::VirtualQueryExT( ptr_t lpAddress, PMEMORY_BASIC_INFORMATION64 lpBuffer ) noexcept
{
    MEMORY_BASIC_INFORMATION tmp = { };

    NTSTATUS status = SAFE_NATIVE_CALL(
        NtQueryVirtualMemory,
        _hProcess, reinterpret_cast<PVOID>(lpAddress),
        MemoryBasicInformation, &tmp, sizeof( tmp ), nullptr
        );

    if (status != STATUS_SUCCESS)
        return status;

    // Map values
    lpBuffer->AllocationBase    = reinterpret_cast<ULONGLONG>(tmp.AllocationBase);
    lpBuffer->AllocationProtect = tmp.AllocationProtect;
    lpBuffer->BaseAddress       = reinterpret_cast<ULONGLONG>(tmp.BaseAddress);
    lpBuffer->Protect           = tmp.Protect;
    lpBuffer->RegionSize        = tmp.RegionSize;
    lpBuffer->State             = tmp.State;
    lpBuffer->Type              = tmp.Type;

    return status;
}

/// <summary>
/// Get WOW64 thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS x86Native::GetThreadContextT( HANDLE hThread, _CONTEXT32& ctx ) noexcept
{
    auto r = GetThreadContext(hThread, reinterpret_cast<PCONTEXT>(&ctx));
    return r != 0 ? STATUS_SUCCESS : LastNtStatus();
}

/// <summary>
/// Get native thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS x86Native::GetThreadContextT( HANDLE /*hThread*/, _CONTEXT64& /*ctx*/ ) noexcept
{
    // There is no x64 context under x86 OS
    return STATUS_NOT_SUPPORTED;
}

/// <summary>
/// Set WOW64 thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS x86Native::SetThreadContextT( HANDLE hThread, _CONTEXT32& ctx ) noexcept
{
    auto r = SetThreadContext(hThread, reinterpret_cast<const CONTEXT*>(&ctx));
    return r != 0 ? STATUS_SUCCESS : LastNtStatus();
}

/// <summary>
/// Set native thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS x86Native::SetThreadContextT( HANDLE /*hThread*/, _CONTEXT64& /*ctx*/ ) noexcept
{
    // There is no x64 context under x86 OS
    return STATUS_NOT_SUPPORTED;
}

/// <summary>
/// Gets WOW64 PEB
/// </summary>
/// <param name="ppeb">Retrieved PEB</param>
/// <returns>PEB pointer</returns>
ptr_t x86Native::getPEB( _PEB32* ppeb ) noexcept
{
    PROCESS_BASIC_INFORMATION pbi = { };
    ULONG bytes = 0;

    SAFE_NATIVE_CALL( NtQueryInformationProcess, _hProcess, ProcessBasicInformation, &pbi, ULONG( sizeof( pbi ) ), &bytes );
    if (bytes > 0)
    {
        if(!ppeb || ReadProcessMemory( _hProcess, pbi.PebBaseAddress, ppeb, sizeof( _PEB32 ), nullptr ))
            return reinterpret_cast<ptr_t>(pbi.PebBaseAddress);
    }

    return 0;
}

/// <summary>
/// Get native PEB
/// </summary>
/// <param name="ppeb">Retrieved PEB</param>
/// <returns>PEB pointer</returns>
ptr_t x86Native::getPEB( _PEB64* /*ppeb*/ ) noexcept
{
    // There is no x64 PEB under x86 OS
    SetLastNtStatus( STATUS_NOT_SUPPORTED );
    return 0;
}

/// <summary>
/// Get WOW64 TEB
/// </summary>
/// <param name="ppeb">Retrieved TEB</param>
/// <returns>TEB pointer</returns>
ptr_t x86Native::getTEB( HANDLE hThread, _TEB32* pteb ) noexcept
{
    _THREAD_BASIC_INFORMATION_T<DWORD> tbi = { };
    ULONG bytes = 0;

    SAFE_NATIVE_CALL( NtQueryInformationThread, hThread, (THREADINFOCLASS)0, &tbi, (ULONG)sizeof( tbi ), &bytes );
    if (bytes > 0)
    {
        if (!pteb || ReadProcessMemory( _hProcess, (LPCVOID)((uintptr_t)tbi.TebBaseAddress), pteb, sizeof( _TEB32 ), nullptr ))
            return tbi.TebBaseAddress;
    }

    return 0;
}

/// <summary>
/// Get native TEB
/// </summary>
/// <param name="ppeb">Retrieved TEB</param>
/// <returns>TEB pointer</returns>
ptr_t x86Native::getTEB( HANDLE /*hThread*/, _TEB64* /*pteb*/ ) noexcept
{
    // There is no x64 TEB under x86 OS
    SetLastNtStatus( STATUS_NOT_SUPPORTED );
    return 0;
}

}