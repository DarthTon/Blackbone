#include "NativeSubsystem.h"
#include "../Misc/Utils.h"
#include "../Misc/DynImport.h"
#include "../Include/Macro.h"

#include <type_traits>
#include <Psapi.h>

namespace blackbone
{

Native::Native( HANDLE hProcess, bool x86OS /*= false*/ )
    : _hProcess( hProcess )
{
    SYSTEM_INFO info = { { 0 } };
    GetNativeSystemInfo( &info );
    _pageSize = info.dwPageSize;

    // x86 OS, emulate WOW64 processes
    if (x86OS)
    {
        _wowBarrier.sourceWow64 = true;
        _wowBarrier.targetWow64 = true;
        _wowBarrier.type = wow_32_32;
        _wowBarrier.x86OS = true;
    }
    else
    {
        BOOL wowSrc = FALSE, wowTgt = FALSE;
        IsWow64Process( GetCurrentProcess(), &wowSrc );
        IsWow64Process( _hProcess, &wowTgt );

        _wowBarrier.sourceWow64 = (wowSrc == TRUE);
        _wowBarrier.targetWow64 = (wowTgt == TRUE);

        if (wowSrc == TRUE && wowTgt == TRUE)
            _wowBarrier.type = wow_32_32;
        else if (wowSrc == FALSE && wowTgt == FALSE)
            _wowBarrier.type = wow_64_64;
        else if (wowSrc == TRUE)
        {
            _wowBarrier.type = wow_32_64;
            _wowBarrier.mismatch = true;
        }
        else
        {
            _wowBarrier.type = wow_64_32;
            _wowBarrier.mismatch = true;
        }
    } 
}

/*
*/
Native::~Native()
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
NTSTATUS Native::VirtualAllocExT( ptr_t& lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect )
{
    SetLastNtStatus( STATUS_SUCCESS );
    lpAddress = reinterpret_cast<ptr_t>(VirtualAllocEx( _hProcess, reinterpret_cast<LPVOID>(lpAddress), dwSize, flAllocationType, flProtect ));
    return LastNtStatus();
}


/// <summary>
/// Free virtual memory
/// </summary>
/// <param name="lpAddress">Memory address</param>
/// <param name="dwSize">Region size</param>
/// <param name="dwFreeType">Memory release type.</param>
/// <returns>Status code</returns>
NTSTATUS Native::VirtualFreeExT( ptr_t lpAddress, size_t dwSize, DWORD dwFreeType )
{
    SetLastNtStatus( STATUS_SUCCESS );
    VirtualFreeEx( _hProcess, reinterpret_cast<LPVOID>(lpAddress), dwSize, dwFreeType );
    return LastNtStatus();
}

/// <summary>
/// Query virtual memory
/// </summary>
/// <param name="lpAddress">Address to query</param>
/// <param name="lpBuffer">Retrieved memory info</param>
/// <returns>Status code</returns>
NTSTATUS Native::VirtualQueryExT( ptr_t lpAddress, PMEMORY_BASIC_INFORMATION64 lpBuffer )
{
    SetLastNtStatus( STATUS_SUCCESS );
    VirtualQueryEx(
        _hProcess, reinterpret_cast<LPCVOID>(lpAddress),
        reinterpret_cast<PMEMORY_BASIC_INFORMATION>(lpBuffer),
        sizeof( MEMORY_BASIC_INFORMATION )
        );

    return LastNtStatus();
}

/// <summary>
/// Query virtual memory
/// </summary>
/// <param name="lpAddress">Address to query</param>
/// <param name="lpBuffer">Retrieved memory info</param>
/// <returns>Status code</returns>
NTSTATUS Native::VirtualQueryExT( ptr_t lpAddress, MEMORY_INFORMATION_CLASS infoClass, LPVOID lpBuffer, size_t bufSize )
{
    SIZE_T retLen = 0;

    SetLastNtStatus( STATUS_SUCCESS );   
    return SAFE_NATIVE_CALL(
        NtQueryVirtualMemory, _hProcess, reinterpret_cast<LPVOID>(lpAddress),
        infoClass, lpBuffer, bufSize, &retLen
        );
}

/// <summary>
/// Change memory protection
/// </summary>
/// <param name="lpAddress">Memory address.</param>
/// <param name="dwSize">Region size</param>
/// <param name="flProtect">New protection.</param>
/// <param name="flOld">Old protection</param>
/// <returns>Status code</returns>
NTSTATUS Native::VirtualProtectExT( ptr_t lpAddress, DWORD64 dwSize, DWORD flProtect, DWORD* flOld )
{
    DWORD junk = 0;
    if (!flOld)
        flOld = &junk;

    SetLastNtStatus( STATUS_SUCCESS );
    VirtualProtectEx( _hProcess, reinterpret_cast<LPVOID>(lpAddress), static_cast<SIZE_T>(dwSize), flProtect, flOld );

    return LastNtStatus();
}

/// <summary>
/// Read virtual memory
/// </summary>
/// <param name="lpBaseAddress">Memory address</param>
/// <param name="lpBuffer">Output buffer</param>
/// <param name="nSize">Number of bytes to read</param>
/// <param name="lpBytes">Mumber of bytes read</param>
/// <returns>Status code</returns>
NTSTATUS Native::ReadProcessMemoryT( ptr_t lpBaseAddress, LPVOID lpBuffer, size_t nSize, DWORD64 *lpBytes /*= nullptr */ )
{
    SetLastNtStatus( STATUS_SUCCESS );
    ReadProcessMemory( _hProcess, reinterpret_cast<LPVOID>(lpBaseAddress), lpBuffer, nSize, reinterpret_cast<SIZE_T*>(lpBytes) );
    return LastNtStatus();
}

/// <summary>
/// Write virtual memory
/// </summary>
/// <param name="lpBaseAddress">Memory address</param>
/// <param name="lpBuffer">Buffer to write</param>
/// <param name="nSize">Number of bytes to read</param>
/// <param name="lpBytes">Mumber of bytes read</param>
/// <returns>Status code</returns>
NTSTATUS Native::WriteProcessMemoryT( ptr_t lpBaseAddress, LPCVOID lpBuffer, size_t nSize, DWORD64 *lpBytes /*= nullptr */ )
{
    SetLastNtStatus( STATUS_SUCCESS );
    WriteProcessMemory( _hProcess, reinterpret_cast<LPVOID>(lpBaseAddress), lpBuffer, nSize, reinterpret_cast<SIZE_T*>(lpBytes) );
    return LastNtStatus();
}

/// <summary>
/// Call NtQueryInformationProcess for underlying process
/// </summary>
/// <param name="infoClass">Information class</param>
/// <param name="lpBuffer">Output buffer</param>
/// <param name="bufSize">Buffer size</param>
/// <returns>Status code</returns>
NTSTATUS Native::QueryProcessInfoT( PROCESSINFOCLASS infoClass, LPVOID lpBuffer, uint32_t bufSize )
{
    ULONG length = 0;
    return SAFE_NATIVE_CALL( NtQueryInformationProcess, _hProcess, infoClass, lpBuffer, bufSize, &length );
}

/// <summary>
/// Call NtSetInformationProcess for underlying process
/// </summary>
/// <param name="infoClass">Information class</param>
/// <param name="lpBuffer">Input buffer</param>
/// <param name="bufSize">Buffer size</param>
/// <returns>Status code</returns>
NTSTATUS Native::SetProcessInfoT( PROCESSINFOCLASS infoClass, LPVOID lpBuffer, uint32_t bufSize )
{
    return SAFE_NATIVE_CALL( NtSetInformationProcess, _hProcess, infoClass, lpBuffer, bufSize );
}

/// <summary>
/// Creates new thread in the remote process
/// </summary>
/// <param name="hThread">Created thread handle</param>
/// <param name="entry">Thread entry point</param>
/// <param name="arg">Thread argument</param>
/// <param name="flags">Creation flags</param>
/// <param name="access">Access override</param>
/// <returns>Status code</returns>
NTSTATUS Native::CreateRemoteThreadT( HANDLE& hThread, ptr_t entry, ptr_t arg, CreateThreadFlags flags, DWORD access /*= THREAD_ALL_ACCESS*/ )
{
    SetLastNtStatus( STATUS_SUCCESS );
    NTSTATUS status = 0; 
    auto pCreateThread = GET_IMPORT( NtCreateThreadEx );

    if (pCreateThread)
    {
        status = pCreateThread(
            &hThread, access, NULL,
            _hProcess, reinterpret_cast<PTHREAD_START_ROUTINE>(entry),
            reinterpret_cast<LPVOID>(arg), static_cast<DWORD>(flags),
            0, 0x1000, 0x100000, NULL
            );

        if (!NT_SUCCESS( status ))
            hThread = NULL;
    }
    else
    {
        DWORD win32Flags = 0;

        if (flags & CreateSuspended)
            win32Flags = CREATE_SUSPENDED;

        hThread = CreateRemoteThread( 
            _hProcess, NULL, 0, reinterpret_cast<PTHREAD_START_ROUTINE>(entry),
            reinterpret_cast<LPVOID>(arg), win32Flags, NULL
            );

        status = LastNtStatus();
    }

    return status;
}

/// <summary>
/// Get native thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS Native::GetThreadContextT( HANDLE hThread, _CONTEXT64& ctx )
{
    SetLastNtStatus( STATUS_SUCCESS );
    GetThreadContext( hThread, reinterpret_cast<PCONTEXT>(&ctx) );
    return LastNtStatus();
}

/// <summary>
/// Get WOW64 thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS Native::GetThreadContextT( HANDLE hThread, _CONTEXT32& ctx )
{
    // Target process is x64. WOW64 CONTEXT is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return 0;
    }
    else
    {
        SetLastNtStatus( STATUS_SUCCESS );
        SAFE_CALL( Wow64GetThreadContext, hThread, reinterpret_cast<PWOW64_CONTEXT>(&ctx) );
        return LastNtStatus();
    }
}

/// <summary>
/// Set native thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS Native::SetThreadContextT( HANDLE hThread, _CONTEXT64& ctx )
{
    SetLastNtStatus( STATUS_SUCCESS );
    SetThreadContext( hThread, reinterpret_cast<PCONTEXT>(&ctx) );
    return LastNtStatus();
}

/// <summary>
/// Set WOW64 thread context
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="ctx">Thread context</param>
/// <returns>Status code</returns>
NTSTATUS Native::SetThreadContextT( HANDLE hThread, _CONTEXT32& ctx )
{
    // Target process is x64. 32bit CONTEXT is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return 0;
    }
    else
    {
        SetLastNtStatus( STATUS_SUCCESS );
        SAFE_CALL( Wow64SetThreadContext, hThread, reinterpret_cast<PWOW64_CONTEXT>(&ctx));
        return LastNtStatus();
    }
}

/// <summary>
/// NtQueueApcThread
/// </summary>
/// <param name="hThread">Thread handle.</param>
/// <param name="func">APC function</param>
/// <param name="arg">APC argument</param>
/// <returns>Status code</returns>
NTSTATUS Native::QueueApcT( HANDLE hThread, ptr_t func, ptr_t arg )
{
    if (_wowBarrier.type == wow_64_32)
    {
        return SAFE_NATIVE_CALL( RtlQueueApcWow64Thread, hThread, reinterpret_cast<PVOID>(func), reinterpret_cast<PVOID>(arg), nullptr, nullptr );
        //func = (~func) << 2;
    }

    return SAFE_NATIVE_CALL( NtQueueApcThread, hThread, reinterpret_cast<PVOID>(func), reinterpret_cast<PVOID>(arg), nullptr, nullptr );
}

/// <summary>
/// Get WOW64 PEB
/// </summary>
/// <param name="ppeb">Retrieved PEB</param>
/// <returns>PEB pointer</returns>
ptr_t Native::getPEB( _PEB32* ppeb )
{
    // Target process is x64. PEB32 is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return 0;
    }
    else
    {
        ptr_t ptr = 0;
        if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationProcess, _hProcess, ProcessWow64Information, &ptr, (ULONG)sizeof( ptr ), nullptr ) ) && ppeb)
            ReadProcessMemory( _hProcess, reinterpret_cast<LPCVOID>(ptr), ppeb, sizeof(_PEB32), NULL );

        return ptr;
    }
}

/// <summary>
/// Get native PEB
/// </summary>
/// <param name="ppeb">Retrieved PEB</param>
/// <returns>PEB pointer</returns>
ptr_t Native::getPEB( _PEB64* ppeb )
{
    PROCESS_BASIC_INFORMATION pbi = { 0 };
    ULONG bytes = 0;

    if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationProcess, _hProcess, ProcessBasicInformation, &pbi, (ULONG)sizeof( pbi ), &bytes ) ) && ppeb)
        ReadProcessMemory( _hProcess, pbi.PebBaseAddress, ppeb, sizeof(_PEB32), NULL );

    return reinterpret_cast<ptr_t>(pbi.PebBaseAddress);
}

/// <summary>
/// Get WOW64 TEB
/// </summary>
/// <param name="ppeb">Retrieved TEB</param>
/// <returns>TEB pointer</returns>
ptr_t Native::getTEB( HANDLE hThread, _TEB32* pteb )
{
    // Target process is x64. TEB32 is not available.
    if (_wowBarrier.targetWow64 == false)
    {
        return 0;
    }
    // Retrieving TEB32 from x64 process.
    else
    {
        _THREAD_BASIC_INFORMATION_T<DWORD64> tbi = { 0 };
        ULONG bytes = 0;

        if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationThread, hThread, (THREADINFOCLASS)0, &tbi, (ULONG)sizeof( tbi ), &bytes ) ) && pteb)
            ReadProcessMemory( _hProcess, (const uint8_t*)tbi.TebBaseAddress + 0x2000, pteb, sizeof(_TEB32), NULL );

        return tbi.TebBaseAddress + 0x2000;
    }

}

/// <summary>
/// Get native TEB
/// </summary>
/// <param name="ppeb">Retrieved TEB</param>
/// <returns>TEB pointer</returns>
ptr_t Native::getTEB( HANDLE hThread, _TEB64* pteb )
{
    _THREAD_BASIC_INFORMATION_T<DWORD64> tbi = { 0 };
    ULONG bytes = 0;

    if (NT_SUCCESS( SAFE_NATIVE_CALL( NtQueryInformationThread, hThread, (THREADINFOCLASS)0, &tbi, (ULONG)sizeof( tbi ), &bytes ) ) && pteb)
        ReadProcessMemory( _hProcess, reinterpret_cast<LPCVOID>(tbi.TebBaseAddress), pteb, sizeof(_TEB64), NULL );

    return tbi.TebBaseAddress;
}

/// <summary>
/// Enumerate valid memory regions
/// </summary>
/// <param name="includeFree">If true - non-allocated regions will be included in list</param>
/// <returns>Found regions</returns>
std::vector<MEMORY_BASIC_INFORMATION64> Native::EnumRegions( bool includeFree /*= false*/ )
{
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    std::vector<MEMORY_BASIC_INFORMATION64> results;

    for (ptr_t memptr = minAddr(); memptr < maxAddr(); memptr = mbi.BaseAddress + mbi.RegionSize)
    {
        auto status = VirtualQueryExT( memptr, &mbi );

        if (status == STATUS_INVALID_PARAMETER || status == STATUS_ACCESS_DENIED)
            break;
        else if (status != STATUS_SUCCESS)
            continue;

        // Filter, if required
        if (includeFree || mbi.State & (MEM_COMMIT | MEM_RESERVE))
            results.emplace_back( mbi );
    }

    return results;
}

/// <summary>
/// Enumerate process modules
/// </summary>
/// <param name="result">Found modules</param>
/// <returns>Module count</returns>
template<typename T>
std::vector<ModuleDataPtr> Native::EnumModulesT()
{
    NTSTATUS status = STATUS_SUCCESS;
    _PEB_T<T> peb = { 0 };
    _PEB_LDR_DATA2_T<T> ldr = { 0 };
    std::vector<ModuleDataPtr> result;

    if (getPEB( &peb ) != 0 && ReadProcessMemoryT( peb.Ldr, &ldr, sizeof(ldr), 0 ) == STATUS_SUCCESS)
    {
        for (T head = ldr.InLoadOrderModuleList.Flink;
            NT_SUCCESS( status ) && head != (peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2_T<T>, InLoadOrderModuleList ));
            status = ReadProcessMemoryT( static_cast<ptr_t>(head), &head, sizeof( head ) ))
        {
            ModuleData data;
            wchar_t localPath[512] = { 0 };
            _LDR_DATA_TABLE_ENTRY_BASE_T<T> localdata = { { 0 } };

            ReadProcessMemoryT( head, &localdata, sizeof(localdata), 0 );
            ReadProcessMemoryT( localdata.FullDllName.Buffer, localPath, localdata.FullDllName.Length );

            data.baseAddress = localdata.DllBase;
            data.size = localdata.SizeOfImage;
            data.fullPath = Utils::ToLower( localPath );
            data.name = Utils::StripPath( data.fullPath );
            data.type = (sizeof( T ) < sizeof( uint64_t )) ? mt_mod32 : mt_mod64;
            data.ldrPtr = static_cast<ptr_t>(head);
            data.manual = false;

            result.emplace_back( std::make_shared<const ModuleData>( data ) );
        }
    }

    return result;
}

/// <summary>
/// Enum process section objects
/// </summary>
/// <param name="result">Found modules</param>
/// <returns>Sections count</returns>
std::vector<ModuleDataPtr> Native::EnumSections()
{
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    ptr_t lastBase = 0;
    std::vector<ModuleDataPtr> result;

    for (ptr_t memptr = minAddr(); memptr < maxAddr(); memptr = mbi.BaseAddress + mbi.RegionSize)
    {
        auto status = VirtualQueryExT( memptr, &mbi );

        if (status == STATUS_INVALID_PARAMETER || status == STATUS_ACCESS_DENIED || status == STATUS_PROCESS_IS_TERMINATING)
            break;
        else if (status != STATUS_SUCCESS)
            continue;

        // Filter non-section regions
        if (mbi.State != MEM_COMMIT || mbi.Type != SEC_IMAGE || lastBase == mbi.AllocationBase)
            continue;

        uint8_t buf[0x1000] = { 0 };
        _UNICODE_STRING_T<uint64_t>* ustr = (decltype(ustr))(buf + 0x800);

        status = VirtualQueryExT( mbi.AllocationBase, MemorySectionName, ustr, sizeof(buf) / 2 );

        // Get additional 
        if (NT_SUCCESS( status ))
        {
            ModuleData data;

            IMAGE_DOS_HEADER* phdrDos = reinterpret_cast<PIMAGE_DOS_HEADER>(buf);
            IMAGE_NT_HEADERS32 *phdrNt32 = nullptr;
            IMAGE_NT_HEADERS64 *phdrNt64 = nullptr;

            if (ReadProcessMemoryT( mbi.AllocationBase, buf, 0x800 ) != STATUS_SUCCESS)
                continue;

            phdrNt32 = reinterpret_cast<PIMAGE_NT_HEADERS32>(buf + phdrDos->e_lfanew);
            phdrNt64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(phdrNt32);

            // If no PE header present
            if (phdrDos->e_magic != IMAGE_DOS_SIGNATURE || phdrNt32->Signature != IMAGE_NT_SIGNATURE)
            {
                // Iterate until region end
                MEMORY_BASIC_INFORMATION64 mbi2 = { 0 };
                for (ptr_t memptr2 = mbi.AllocationBase; memptr2 < maxAddr(); memptr2 = mbi2.BaseAddress + mbi2.RegionSize)
                    if (!NT_SUCCESS( VirtualQueryExT( memptr2, &mbi2 ) ) || mbi2.Type != SEC_IMAGE)
                    {
                        data.size = static_cast<uint32_t>(mbi2.BaseAddress - mbi.AllocationBase);
                        break;
                    }

                data.type = mt_unknown;
            }
            else if( phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
            {
                data.size = phdrNt32->OptionalHeader.SizeOfImage;
                data.type = mt_mod32;
            }
            else if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                data.size = phdrNt64->OptionalHeader.SizeOfImage;
                data.type = mt_mod64;
            }
            else 
                continue;

            // Hack for x86 OS
            if (_wowBarrier.x86OS == true)
            {
                _UNICODE_STRING_T<DWORD>* ustr32 = reinterpret_cast<_UNICODE_STRING_T<DWORD>*>(ustr);
                data.fullPath = Utils::ToLower( reinterpret_cast<wchar_t*>((uintptr_t)ustr32->Buffer) );
            }
            else
                data.fullPath = Utils::ToLower( reinterpret_cast<wchar_t*>((uintptr_t)ustr->Buffer) );

            data.name = Utils::StripPath( data.fullPath );
            data.baseAddress = mbi.AllocationBase;
            data.ldrPtr = 0;
            data.manual = false;

            result.emplace_back( std::make_shared<const ModuleData>( data ) );
        }

        lastBase = mbi.AllocationBase;
    }

    return result;
}

/// <summary>
/// Enum pages containing valid PE headers
/// </summary>
/// <param name="result">Found modules</param>
/// <returns>Sections count</returns>
std::vector<ModuleDataPtr> Native::EnumPEHeaders()
{
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    uint8_t buf[0x1000];
    ptr_t lastBase = 0;
    std::vector<ModuleDataPtr> result;

    for (ptr_t memptr = minAddr(); memptr < maxAddr(); memptr = mbi.BaseAddress + mbi.RegionSize)
    {
        auto status = VirtualQueryExT( memptr, &mbi );

        if (status == STATUS_INVALID_PARAMETER || status == STATUS_ACCESS_DENIED || status == STATUS_PROCESS_IS_TERMINATING)
            break;
        else if (status != STATUS_SUCCESS)
            continue;

        // Filter regions
        if (mbi.State != MEM_COMMIT ||
            mbi.AllocationProtect == PAGE_NOACCESS ||
            mbi.AllocationProtect & PAGE_GUARD ||
            lastBase == mbi.AllocationBase)
        {
            continue;
        }

        ModuleData data;

        IMAGE_DOS_HEADER* phdrDos = reinterpret_cast<PIMAGE_DOS_HEADER>(buf);
        IMAGE_NT_HEADERS32 *phdrNt32 = nullptr;
        IMAGE_NT_HEADERS64 *phdrNt64 = nullptr;

        if (ReadProcessMemoryT( mbi.AllocationBase, buf, 0x1000 ) != STATUS_SUCCESS)
            continue;

        phdrNt32 = reinterpret_cast<PIMAGE_NT_HEADERS32>(buf + phdrDos->e_lfanew);
        phdrNt64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(phdrNt32);

        if (phdrDos->e_magic != IMAGE_DOS_SIGNATURE || phdrNt32->Signature != IMAGE_NT_SIGNATURE)
            continue;

        if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            data.size = phdrNt32->OptionalHeader.SizeOfImage;
            data.type = mt_mod32;
        }
        else if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            data.size = phdrNt64->OptionalHeader.SizeOfImage;
            data.type = mt_mod64;
        }

        data.baseAddress = mbi.AllocationBase;
        data.ldrPtr = 0;
        data.manual = false;

        // Try to get section name
        _UNICODE_STRING_T<DWORD64>* ustr = (decltype(ustr))buf;
        status = VirtualQueryExT( mbi.AllocationBase, MemorySectionName, ustr, sizeof(buf) );

        if (status == STATUS_SUCCESS)
        {
            // Hack for x86 OS
            if (_wowBarrier.x86OS == true)
            {
                _UNICODE_STRING_T<DWORD>* ustr32 = reinterpret_cast<_UNICODE_STRING_T<DWORD>*>(ustr);
                data.fullPath = Utils::ToLower( reinterpret_cast<wchar_t*>((uintptr_t)ustr32->Buffer) );
            }
            else
                data.fullPath = Utils::ToLower( reinterpret_cast<wchar_t*>((uintptr_t)ustr->Buffer) );

            data.name = Utils::StripPath( data.fullPath );
        }
        else
        {
            wchar_t name[64] = { 0 };
            wsprintfW( name, L"Unknown_0x%I64x", data.baseAddress );

            data.fullPath = name;
            data.name = data.fullPath;
        }

        result.emplace_back( std::make_shared<const ModuleData>( data ) );

        lastBase = mbi.AllocationBase;
    }

    return result;
}

/// <summary>
/// Enumerate process modules
/// </summary>
/// <param name="result">Found modules</param>
/// <param name="mtype">Module type: x86 or x64</param>
/// <returns>Module count</returns>
std::vector<ModuleDataPtr> Native::EnumModules( eModSeachType search/*= LdrList*/, eModType mtype /*= mt_default */ )
{
    if (search == LdrList)
    {
        // Detect module type
        if (mtype == mt_default)
            mtype = _wowBarrier.targetWow64 ? mt_mod32 : mt_mod64;

        return CALL_64_86( mtype == mt_mod64, EnumModulesT );
    }
    else if(search == Sections)
    {
        return EnumSections();
    }
    else if(search == PEHeaders)
    {
        return EnumPEHeaders();
    }

    return std::vector<ModuleDataPtr>();
}


}