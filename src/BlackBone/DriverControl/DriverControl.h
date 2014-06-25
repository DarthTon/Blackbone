#pragma once

#include "../Include/Winheaders.h"
#include "../Include/Types.h"
#include "../../BlackBoneDrv/BlackBoneDef.h"

#include <string>
#include <map>

namespace blackbone
{

// [Original ptr, size] <--> [Mapped ptr]
typedef std::map < std::pair<ptr_t, uint32_t>, ptr_t > mapMemoryMap;

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
    DriverControl();
    ~DriverControl();

    static DriverControl& Instance();

    /// <summary>
    /// Try to load driver if it isn't loaded
    /// </summary>
    /// <param name="path">Path to the driver file</param>
    /// <returns>Status code</returns>
    NTSTATUS EnsureLoaded( const std::wstring& path = L"" );

    /// <summary>
    /// Reload driver
    /// </summary>
    /// <param name="path">Path to the driver file</param>
    /// <returns>Status code</returns>
    NTSTATUS Reload( std::wstring path = L"" );

    /// <summary>
    /// Unload driver
    /// </summary>
    /// <returns>Status code</returns>
    NTSTATUS Unload();

    /// <summary>
    /// Disable DEP for process
    /// Has no effect on native x64 processes
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <returns>Status code</returns>
    NTSTATUS DisableDEP( DWORD pid );

    /// <summary>
    /// Change process protection flag
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="enable">true to enable protection, false to disable</param>
    /// <returns>Status code</returns>
    NTSTATUS ProtectProcess( DWORD pid, bool enable );

    /// <summary>
    /// Change handle access rights
    /// </summary>
    /// <param name="pid">Target PID.</param>
    /// <param name="handle">Handle</param>
    /// <param name="access">New access</param>
    /// <returns>Status code</returns>
    NTSTATUS PromoteHandle( DWORD pid, HANDLE handle, DWORD access );

    /// <summary>
    /// Allocate virtual memory
    /// </summary>
    /// <param name="pid">Tarhet PID</param>
    /// <param name="base">Desired base. If 0 address is chosed by the system</param>
    /// <param name="size">Region size</param>
    /// <param name="type">Allocation type - MEM_RESERVE/MEM_COMMIT</param>
    /// <param name="protection">Memory protection</param>
    /// <returns>Status code</returns>
    NTSTATUS AllocateMem( DWORD pid, ptr_t& base, ptr_t& size, DWORD type, DWORD protection );

    /// <summary>
    /// Free virtual memory
    /// </summary>
    /// <param name="pid">Tarhet PID</param>
    /// <param name="base">Desired base. If 0 address is chosed by the system</param>
    /// <param name="size">Region size</param>
    /// <param name="type">Free type - MEM_RELEASE/MEM_DECOMMIT</param>
    /// <returns>Status code</returns>
    NTSTATUS FreeMem( DWORD pid, ptr_t base, ptr_t size, DWORD type );

    /// <summary>
    /// Read process memory
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Target base</param>
    /// <param name="size">Data size</param>
    /// <param name="buffer">Buffer address</param>
    /// <returns>Status code</returns>
    NTSTATUS ReadMem( DWORD pid, ptr_t base, ptr_t size, PVOID buffer );

    /// <summary>
    /// Write process memory
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Target base</param>
    /// <param name="size">Data size</param>
    /// <param name="buffer">Buffer address</param>
    /// <returns>Status code</returns>
    NTSTATUS WriteMem( DWORD pid, ptr_t base, ptr_t size, PVOID buffer );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="pid">Target PID.</param>
    /// <param name="base">Regiod base address</param>
    /// <param name="size">Region size</param>
    /// <param name="protection">New protection</param>
    /// <returns>Status code</returns>
    NTSTATUS ProtectMem( DWORD pid, ptr_t base, ptr_t size, DWORD protection );

    /// <summary>
    /// Maps target process memory into current process
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="pipeName">Pipe name to use for hook data transfer</param>
    /// <param name="mapSections">The map sections.</param>
    /// <param name="result">Results</param>
    /// <returns>Status code </returns>
    NTSTATUS MapMemory( DWORD pid, const std::wstring& pipeName, bool mapSections, MapMemoryResult& result );

    /// <summary>
    /// Maps single memory region into current process
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <param name="base">Region base address</param>
    /// <param name="size">Region size</param>
    /// <param name="result">Mapped region info</param>
    /// <returns>Status code</returns>
    NTSTATUS MapMemoryRegion( DWORD pid, ptr_t base, uint32_t size, MapMemoryRegionResult& result );

    /// <summary>
    /// Unmap memory of the target process from current
    /// </summary>
    /// <param name="pid">Target PID</param>
    /// <returns>Status code</returns>
    NTSTATUS UnmapMemory( DWORD pid );

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
    NTSTATUS UnmapMemoryRegion( DWORD pid, ptr_t base, uint32_t size );

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
    HANDLE _hDriver = INVALID_HANDLE_VALUE;
};

// Syntax sugar
inline DriverControl& Driver() { return DriverControl::Instance(); }

}