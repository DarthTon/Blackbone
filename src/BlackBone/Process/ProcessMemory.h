#pragma once

#include "../Include/Winheaders.h"
#include "../Include/Exception.h"
#include "RPC/RemoteMemory.h"
#include "MemBlock.h"

#include <vector>
#include <list>

namespace blackbone
{


enum class MemProtectionCasting
{
    none,   // Don't change provided memory protection flags
    useDep  // Strip executable flag if DEP is off
};

class ProcessMemory : public RemoteMemory
{
public:
    BLACKBONE_API ProcessMemory( class Process* process );

    ProcessMemory( const ProcessMemory& ) = delete;
    ProcessMemory& operator =( const ProcessMemory& ) = delete;

    /// <summary>
    /// Allocate new memory block
    /// </summary>
    /// <param name="size">Block size</param>
    /// <param name="protection">Memory protection</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <param name="desired">false if caller will be responsible for block deallocation</param>
    /// <returns>Memory block</returns>
    BLACKBONE_API MemBlock Allocate( size_t size, DWORD protection = PAGE_EXECUTE_READWRITE, ptr_t desired = 0, bool own = true );

    /// <summary>
    /// Allocate new memory block as close to a desired location as possible
    /// </summary>
    /// <param name="size">Block size</param>
    /// <param name="protection">Memory protection</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <param name="desired">false if caller will be responsible for block deallocation</param>
    /// <returns>Memory block. If failed - returned block will be invalid</returns>
    BLACKBONE_API MemBlock AllocateClosest( size_t size, DWORD protection = PAGE_EXECUTE_READWRITE, ptr_t desired = 0, bool own = true );

    /// <summary>
    /// Free memory
    /// </summary>
    /// <param name="pAddr">Memory address to release.</param>
    /// <param name="size">Region size</param>
    /// <param name="freeType">Release/decommit</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Free( ptr_t pAddr, size_t size = 0, DWORD freeType = MEM_RELEASE );

    /// <summary>
    /// Get memory region info
    /// </summary>
    /// <param name="pAddr">Memory address</param>
    /// <param name="pInfo">Retrieved info</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Query( ptr_t pAddr, PMEMORY_BASIC_INFORMATION64 pInfo );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="pAddr">Memory address</param>
    /// <param name="size">Region size</param>
    /// <param name="flProtect">New memory protection flags</param>
    /// <param name="pOld">Old protection flags</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Protect( ptr_t pAddr, size_t size, DWORD flProtect, DWORD *pOld = NULL );

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
    BLACKBONE_API NTSTATUS ReadNoThrow( ptr_t dwAddress, size_t dwSize, PVOID pResult, bool handleHoles = false );
    BLACKBONE_API void Read( ptr_t dwAddress, size_t dwSize, PVOID pResult, bool handleHoles = false );

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
    BLACKBONE_API void Read( const std::vector<ptr_t>& adrList, size_t dwSize, PVOID pResult, bool handleHoles = false );

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="pAddress">Memory address to write to</param>
    /// <param name="dwSize">Size of data to write</param>
    /// <param name="pData">Buffer to write</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS WriteNoThrow( ptr_t pAddress, size_t dwSize, const void* pData );
    BLACKBONE_API void Write( ptr_t pAddress, size_t dwSize, const void* pData );

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="adrList">Base address + list of offsets</param>
    /// <param name="dwSize">Size of data to write</param>
    /// <param name="pData">Buffer to write</param>
    BLACKBONE_API void Write( const std::vector<ptr_t>& adrList, size_t dwSize, const void* pData );

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="dwAddress">Address to read from</param>
    /// <returns>Read data</returns>
    template<class T>
    T Read( ptr_t dwAddress )
    {
        auto res = static_cast<T*>(_malloca( sizeof( T ) ));
        Read( dwAddress, sizeof( T ), res );
        return *res;
    };

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="adrList">Base address + list of offsets</param>
    /// <returns>Read data</returns>
    template<class T>
    T Read( std::vector<ptr_t>&& adrList )
    {
        auto res = static_cast<T*>(_malloca( sizeof( T ) ));
        Read( std::forward<std::vector<ptr_t>>( adrList ), sizeof( T ), res );
        return *res;
    }

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="dwAddress">Address to read from</param>
    /// <param name="result">Read data</param>
    template<class T>
    void Read( ptr_t dwAddress, T& result )
    {
        Read( dwAddress, sizeof( result ), &result );
    }

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="adrList">Base address + list of offsets</param>
    /// <param name="result">Read data</param>
    template<class T>
    void Read( std::vector<ptr_t>&& adrList, T& result )
    {
        Read( std::forward<std::vector<ptr_t>>( adrList ), sizeof( result ), &result );
    }

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="dwSize">Size of data to write</param>
    /// <param name="data">Data to write</param>
    /// <returns>Status</returns>
    template<class T>
    void Write( ptr_t dwAddress, const T& data )
    {
        Write( dwAddress, sizeof( T ), &data );
    }

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="adrList">Base address + list of offset</param>
    /// <param name="data">Data to write</param>
    /// <returns>Status</returns>
    template<class T>
    void Write( std::vector<ptr_t>&& adrList, const T& data )
    {
        Write( std::forward<std::vector<ptr_t>>( adrList ), sizeof( T ), &data );
    }

    /// <summary>
    /// Enumerate valid memory regions
    /// </summary>
    /// <param name="includeFree">If true - non-allocated regions will be included in list</param>
    /// <returns>Found regions</returns>
    BLACKBONE_API std::vector<MEMORY_BASIC_INFORMATION64> EnumRegions( bool includeFree = false );

    /// <summary>
    /// Get memory protection casting behavior 
    /// </summary>
    /// <returns>current behavior</returns>
    BLACKBONE_API MemProtectionCasting protectionCasting() {  return _casting; }

    /// <summary>
    /// Set memory protection casting behavior 
    /// </summary>
    /// <param name="flag">new behavior</param>
    BLACKBONE_API void protectionCasting( MemProtectionCasting casting ) { _casting = casting; }

    BLACKBONE_API class ProcessCore* core() { return _core; }
    BLACKBONE_API class Process* process()  { return _process; }

private:
    class Process* _process;    // Owning process object
    class ProcessCore* _core;   // Core routines
    MemProtectionCasting _casting = MemProtectionCasting::useDep;
};

}
