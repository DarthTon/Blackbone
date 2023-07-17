#include "ProcessMemory.h"
#include "Process.h"
#include "../Include/Exception.h"

namespace blackbone
{

ProcessMemory::ProcessMemory( Process* process )
    : RemoteMemory( process )
    , _process( process )
    , _core( &process->core() )  
{
}

/// <summary>
/// Allocate new memory block
/// </summary>
/// <param name="size">Block size</param>
/// <param name="protection">Memory protection</param>
/// <param name="desired">Desired base address of new block</param>
/// <param name="own">false if caller will be responsible for block deallocation</param>
/// <returns>Memory block</returns>
MemBlock ProcessMemory::Allocate( size_t size, DWORD protection /*= PAGE_EXECUTE_READWRITE*/, ptr_t desired /*= 0*/, bool own /*= true*/ )
{
    return MemBlock::Allocate( *this, size, desired, protection, own );
}

MemBlock ProcessMemory::AllocateClosest( size_t size, DWORD protection /*= PAGE_EXECUTE_READWRITE*/, ptr_t desired /*= 0*/, bool own /*= true*/ )
{
    return MemBlock::AllocateClosest( *this, size, desired, protection, own );
}

/// <summary>
/// Free memory
/// </summary>
/// <param name="pAddr">Memory address to release.</param>
/// <param name="size">Region size</param>
/// <param name="freeType">Release/decommit</param>
/// <returns>Status</returns>
NTSTATUS ProcessMemory::Free( ptr_t pAddr, size_t size /*= 0*/, DWORD freeType /*= MEM_RELEASE*/ )
{
#ifdef _DEBUG
    assert( freeType != MEM_RELEASE || size == 0 );
    if (freeType == MEM_DECOMMIT) {
        BLACKBONE_TRACE( L"Free: Decommit at address 0x%p (0x%x bytes)", static_cast<uintptr_t>(pAddr), size );
    } else {
        BLACKBONE_TRACE( L"Free: Free at address 0x%p", static_cast<uintptr_t>(pAddr) );
    }
#endif
    return _core->native()->VirtualFreeExT( pAddr, size, freeType );
}

/// <summary>
/// Get memory region info
/// </summary>
/// <param name="pAddr">Memory address</param>
/// <param name="pInfo">Retrieved info</param>
/// <returns>Status</returns>
NTSTATUS ProcessMemory::Query( ptr_t pAddr, PMEMORY_BASIC_INFORMATION64 pInfo )
{
    return _core->native()->VirtualQueryExT( pAddr, pInfo );
}

/// <summary>
/// Change memory protection
/// </summary>
/// <param name="pAddr">Memory address</param>
/// <param name="size">Region size</param>
/// <param name="flProtect">New memory protection flags</param>
/// <param name="pOld">Old protection flags</param>
/// <returns>Status</returns>
NTSTATUS ProcessMemory::Protect( ptr_t pAddr, size_t size, DWORD flProtect, DWORD *pOld /*= NULL*/ )
{
    DWORD junk = 0;
    if (pOld == nullptr)
        pOld = &junk;

    DWORD finalProt = flProtect;
    if (_casting == MemProtectionCasting::useDep)
        finalProt = CastProtection( flProtect, _core->DEP() );

    return _core->native()->VirtualProtectExT( pAddr, size, finalProt, pOld );
}

/// <summary>
/// Read data
/// </summary>
/// <param name="dwAddress">Memory address to read from</param>
/// <param name="dwSize">Size of data to read</param>
/// <param name="pResult">Output buffer</param>
/// <param name="handleHoles">
/// If true, function will try to read all committed pages in range ignoring uncommitted ones.
/// Otherwise function will fail if there is at least one non-committed page in region.
/// </param>
/// <returns>Status</returns>
NTSTATUS ProcessMemory::ReadNoThrow( ptr_t dwAddress, size_t dwSize, PVOID pResult, bool handleHoles /*= false*/ )
{
    DWORD64 dwRead = 0;
    if (dwAddress == 0)
        return STATUS_INVALID_ADDRESS;

    // Simple read
    if (!handleHoles)
        return _core->native()->ReadProcessMemoryT( dwAddress, pResult, dwSize, &dwRead );

    // Read all committed memory regions
    MEMORY_BASIC_INFORMATION64 mbi = { };
    for (ptr_t memptr = dwAddress; memptr < dwAddress + dwSize; memptr = mbi.BaseAddress + mbi.RegionSize)
    {
        if (!NT_SUCCESS( _core->native()->VirtualQueryExT( memptr, &mbi ) ))
            continue;

        // Filter empty regions
        if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS)
            continue;

        uint64_t region_ptr = memptr - dwAddress;

        NTSTATUS status = _core->native()->ReadProcessMemoryT(
            mbi.BaseAddress,
            reinterpret_cast<uint8_t*>(pResult) + region_ptr,
            static_cast<size_t>(mbi.RegionSize),
            &dwRead
        );

        if (!NT_SUCCESS( status ))
            continue;
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Read data
/// </summary>
/// <param name="dwAddress">Memory address to read from</param>
/// <param name="dwSize">Size of data to read</param>
/// <param name="pResult">Output buffer</param>
/// <param name="handleHoles">
/// If true, function will try to read all committed pages in range ignoring uncommitted ones.
/// Otherwise function will fail if there is at least one non-committed page in region.
/// </param>
void ProcessMemory::Read( ptr_t dwAddress, size_t dwSize, PVOID pResult, bool handleHoles /*= false*/ )
{
    NTSTATUS status = ReadNoThrow( dwAddress, dwSize, pResult, handleHoles );
    THROW_ON_FAIL_AND_LOG( status, "failed to read from address 0x%p", dwAddress );
}

/// <summary>
/// Read data
/// </summary>
/// <param name="adrList">Base address + list of offsets</param>
/// <param name="dwSize">Size of data to read</param>
/// <param name="pResult">Output buffer</param>
/// <param name="handleHoles">
/// If true, function will try to read all committed pages in range ignoring uncommitted ones.
/// Otherwise function will fail if there is at least one non-committed page in region.
/// </param>
void ProcessMemory::Read( const std::vector<ptr_t>& adrList, size_t dwSize, PVOID pResult, bool handleHoles /*= false */ )
{
    if (adrList.empty())
        THROW_AND_LOG( "address list is empty" );

    if (adrList.size() == 1)
        return Read( adrList.front(), dwSize, pResult, handleHoles );

    bool wow64 = _process->barrier().targetWow64;
    ptr_t ptr = wow64 ? Read<uint32_t>( adrList[0] ): Read<ptr_t>( adrList[0] );

    for (size_t i = 1; i < adrList.size() - 1; i++)
        ptr = wow64 ? Read<uint32_t>( ptr + adrList[i] ) : Read<ptr_t>( ptr + adrList[i] );

    return Read( ptr + adrList.back(), dwSize, pResult, handleHoles );
}

/// <summary>
/// Write data
/// </summary>
/// <param name="pAddress">Memory address to write to</param>
/// <param name="dwSize">Size of data to write</param>
/// <param name="pData">Buffer to write</param>
/// <returns>Status</returns>
NTSTATUS ProcessMemory::WriteNoThrow( ptr_t pAddress, size_t dwSize, const void * pData )
{
    return _core->native()->WriteProcessMemoryT( pAddress, pData, dwSize );
}

/// <summary>
/// Write data
/// </summary>
/// <param name="pAddress">Memory address to write to</param>
/// <param name="dwSize">Size of data to write</param>
/// <param name="pData">Buffer to write</param>
void ProcessMemory::Write( ptr_t pAddress, size_t dwSize, const void* pData )
{
    NTSTATUS status = _core->native()->WriteProcessMemoryT( pAddress, pData, dwSize );
    THROW_ON_FAIL_AND_LOG( status, "failed to write to address 0x%p", pAddress );
}

/// <summary>
/// Write data
/// </summary>
/// <param name="adrList">Base address + list of offsets</param>
/// <param name="dwSize">Size of data to write</param>
/// <param name="pData">Buffer to write</param>
void ProcessMemory::Write( const std::vector<ptr_t>& adrList, size_t dwSize, const void* pData )
{
    if (adrList.empty())
        THROW_AND_LOG( "address list is empty" );

    if (adrList.size() == 1)
        return Write( adrList.front(), dwSize, pData );

    bool wow64 = _process->barrier().targetWow64;
    ptr_t ptr = wow64 ? Read<uint32_t>( adrList[0] ): Read<ptr_t>( adrList[0] );

    for (size_t i = 1; i < adrList.size() - 1; i++)
        ptr = wow64 ? Read<uint32_t>( ptr + adrList[i] ) : Read<ptr_t>( ptr + adrList[i] );

    Write( ptr + adrList.back(), dwSize, pData );
}

/// <summary>
/// Enumerate valid memory regions
/// </summary>
/// <param name="includeFree">If true - non-allocated regions will be included in list</param>
/// <returns>Found regions</returns>
std::vector<MEMORY_BASIC_INFORMATION64> ProcessMemory::EnumRegions( bool includeFree /*= false*/ )
{
    return _core->native()->EnumRegions( includeFree );
}

}
