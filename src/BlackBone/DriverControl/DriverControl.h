#pragma once

#include "../Include/Winheaders.h"
#include "../Include/Types.h"
#include "../Include/Macro.h"
#include "../Include/HandleGuard.h"
#include "../../BlackBoneDrv/BlackBoneDef.h"

#include <string>
#include <map>
#include <vector>

ENUM_OPS( KMmapFlags );

namespace blackbone
{
// [Original ptr, size] <--> [Mapped ptr]
using mapMemoryMap = std::map<std::pair<ptr_t, uint32_t>, ptr_t>;

struct MapMemoryResult
{
    ptr_t hostSharedPage;       // Shared page address in current process
    ptr_t targetSharedPage;     // Shared page address in target process
    HANDLE targetPipe;          // Hook pipe handle in the target process
            
    mapMemoryMap regions;       // Mapped regions info
};

struct MapMemoryRegionResult
{
    ptr_t originalPtr;          // Address of region in the target process
    ptr_t newPtr;               // Address of mapped region in the current process
    ptr_t removedPtr;           // Address of region unmapped because of address conflict 
    uint32_t size;              // Size of mapped region
    uint32_t removedSize;       // Size of unmapped region
};


class DriverControl
{
public:
    BLACKBONE_API DriverControl();
    BLACKBONE_API ~DriverControl();

    BLACKBONE_API static DriverControl& Instance();

    /// <summary>
    /// Try to load driver if it isn't loaded
    /// </summary>
    /// <param name="path">Path to the driver file</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS EnsureLoaded( const std::wstring& path = L"" );

    /// <summary>
    /// Reload driver
    /// </summary>
    /// <param name="path">Path to the driver file</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Reload( std::wstring path = L"" );

    /// <summary>
    /// Unload driver
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Unload();

    /// <summary>
    /// Disable DEP for process
    /// Has no effect on native x64 processes
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS DisableDEP( DWORD pid );

    /// <summary>
    /// Change process protection flag
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="protection">Process protection policy</param>
    /// <param name="dynamicCode">Prohibit dynamic code</param>
    /// <param name="binarySignature">Prohibit loading non-microsoft dlls</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS ProtectProcess( 
        DWORD pid, 
        PolicyOpt protection, 
        PolicyOpt dynamicCode = Policy_Keep,
        PolicyOpt binarySignature = Policy_Keep
    );

    /// <summary>
    /// Change handle access rights
    /// </summary>
    /// <param name="pid">Target PID.</param>
    /// <param name="handle">Handle</param>
    /// <param name="access">New access</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS PromoteHandle( DWORD pid, HANDLE handle, DWORD access );

    /// <summary>
    /// Allocate virtual memory
    /// </summary>
    /// <param name="pid">Tarhet PID</param>
    /// <param name="base">Desired base. If 0 address is chosed by the system</param>
    /// <param name="size">Region size</param>
    /// <param name="type">Allocation type - MEM_RESERVE/MEM_COMMIT</param>
    /// <param name="protection">Memory protection</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS AllocateMem( DWORD pid, ptr_t& base, ptr_t& size, DWORD type, DWORD protection, bool physical = false );

    /// <summary>
    /// Free virtual memory
    /// </summary>
    /// <param name="pid">Tarhet PID</param>
    /// <param name="base">Desired base. If 0 address is chosed by the system</param>
    /// <param name="size">Region size</param>
    /// <param name="type">Free type - MEM_RELEASE/MEM_DECOMMIT</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS FreeMem( DWORD pid, ptr_t base, ptr_t size, DWORD type );

    /// <summary>
    /// Read process memory
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Target base</param>
    /// <param name="size">Data size</param>
    /// <param name="buffer">Buffer address</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS ReadMem( DWORD pid, ptr_t base, ptr_t size, PVOID buffer );

    /// <summary>
    /// Write process memory
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Target base</param>
    /// <param name="size">Data size</param>
    /// <param name="buffer">Buffer address</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS WriteMem( DWORD pid, ptr_t base, ptr_t size, PVOID buffer );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="pid">Target PID.</param>
    /// <param name="base">Regiod base address</param>
    /// <param name="size">Region size</param>
    /// <param name="protection">New protection</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS ProtectMem( DWORD pid, ptr_t base, ptr_t size, DWORD protection );

    /// <summary>
    /// Maps target process memory into current process
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="pipeName">Pipe name to use for hook data transfer</param>
    /// <param name="mapSections">The map sections.</param>
    /// <param name="result">Results</param>
    /// <returns>Status code </returns>
    BLACKBONE_API NTSTATUS MapMemory( DWORD pid, const std::wstring& pipeName, bool mapSections, MapMemoryResult& result );

    /// <summary>
    /// Maps single memory region into current process
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Region base address</param>
    /// <param name="size">Region size</param>
    /// <param name="result">Mapped region info</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS MapMemoryRegion( DWORD pid, ptr_t base, uint32_t size, MapMemoryRegionResult& result );

    /// <summary>
    /// Unmap memory of the target process from current
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS UnmapMemory( DWORD pid );

    /// <summary>
    /// Unmap single memory region
    /// If unmapped region size is smaller than the size specified during map, function will return info about
    /// 2 regions that emerged after unmap
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Region base</param>
    /// <param name="size">Region size</param>
    /// <param name="result">Unampped region info</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS UnmapMemoryRegion( DWORD pid, ptr_t base, uint32_t size );

    /// <summary>
    /// Inject DLL into arbitrary process
    /// </summary>
    /// <param name="pid">Target PID.</param>
    /// <param name="path">Full qualified dll path.</param>
    /// <param name="itype">Injection type</param>
    /// <param name="initRVA">Init routine RVA</param>
    /// <param name="initArg">Init routine argument</param>
    /// <param name="unlink">Unlink module after injection</param>
    /// <param name="erasePE">Erase PE headers after injection</param>
    /// <param name="wait">Wait for injection</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS InjectDll(
        DWORD pid, 
        const std::wstring& path, 
        InjectType itype, 
        uint32_t initRVA = 0, 
        const std::wstring& initArg = L"", 
        bool unlink = false,
        bool erasePE = false,
        bool wait = true
        );

    /// <summary>
    /// Manually map PE image
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="path">Full qualified image path</param>
    /// <param name="flags">Mapping flags</param>
    /// <param name="initRVA">Init routine RVA</param>
    /// <param name="initArg">Init routine argument</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS MmapDll(
        DWORD pid,
        const std::wstring& path,
        KMmapFlags flags,
        uint32_t initRVA = 0,
        const std::wstring& initArg = L""     
        );

    /// <summary>
    /// Manually map PE image
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="address">Memory location of the image to map</param>
    /// <param name="size">Image size</param>
    /// <param name="asImage">Memory chunk has image layout</param>
    /// <param name="flags">Mapping flags</param>
    /// <param name="initRVA">Init routine RVA</param>
    /// <param name="initArg">Init routine argument</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS MmapDll(
        DWORD pid,
        void* address,
        uint32_t size,
        bool asImage,
        KMmapFlags flags,
        uint32_t initRVA = 0,
        const std::wstring& initArg = L""
        );

    /// <summary>
    /// Manually map another system driver into system space
    /// </summary>
    /// <param name="path">Fully quialified path to the drver</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS MMapDriver( const std::wstring& path );

    /// <summary>
    /// Make VAD region appear as PAGE_NO_ACESS to NtQueryVirtualMemory
    /// </summary>
    /// <param name="pid">Target process ID</param>
    /// <param name="base">Region base</param>
    /// <param name="size">Region size</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS ConcealVAD( DWORD pid, ptr_t base, uint32_t size );

    /// <summary>
    /// Unlink process handle table from HandleListHead
    /// </summary>
    /// <param name="pid">Target process ID</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS UnlinkHandleTable( DWORD pid );

    /// <summary>
    ///  Enumerate committed, accessible, non-guarded memory regions
    /// </summary>
    /// <param name="pid">Target process ID</param>
    /// <param name="regions">Found regions</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS EnumMemoryRegions( DWORD pid, std::vector<MEMORY_BASIC_INFORMATION64>& regions );

    /// <summary>
    /// Check if driver is loaded
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline bool loaded() const { return _hDriver.valid(); }
    BLACKBONE_API inline NTSTATUS status() const { return _loadStatus; }

private:
    DriverControl( const DriverControl& ) = delete;
    DriverControl& operator = (const DriverControl&) = delete;

    /// <summary>
    /// Load arbitrary driver
    /// </summary>
    /// <param name="svcName">Driver service name</param>
    /// <param name="path">Driver file path</param>
    /// <returns>Status</returns>
    NTSTATUS LoadDriver( const std::wstring& svcName, const std::wstring& path );

    /// <summary>
    /// Unload arbitrary driver
    /// </summary>
    /// <param name="svcName">Driver service name</param>
    /// <returns>Status</returns>
    NTSTATUS UnloadDriver( const std::wstring& svcName );

    /// <summary>
    /// Fill minimum driver registry entry
    /// </summary>
    /// <param name="svcName">Driver service name</param>
    /// <param name="path">Driver path</param>
    /// <returns>Status code</returns>
    LSTATUS PrepareDriverRegEntry( const std::wstring& svcName, const std::wstring& path );
private:
    Handle _hDriver;
    NTSTATUS _loadStatus = STATUS_NOT_FOUND;
};

// Syntax sugar
inline DriverControl& Driver() { return DriverControl::Instance(); }

}