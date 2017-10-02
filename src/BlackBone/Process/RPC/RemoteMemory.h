#pragma once

#include "../../Config.h"
#include "../../DriverControl/DriverControl.h"

#include <string>
#include <map>

namespace blackbone
{


class RemoteMemory
{
public:
#ifdef USE64
    struct HookData
    {
        uint8_t original_code[24];  // Original 5 bytes + jmp
        uint8_t jump_buf[12];       // Trampoline
        uint8_t hook_code[384];     // Hook function
    };
#else
    struct HookData
    {
        uint8_t original_code[10];  // Original 5 bytes + jmp
        uint8_t jump_buf[5];        // Trampoline
        uint8_t hook_code[384];     // Hook function
    };
#endif

    enum OperationType
    {
        MemVirtualAlloc = 0,        // MemVirtualAlloc
        MemVirtualFree,             // MemVirtualFree 
        MemMapSection,              // MemMapSection  
        MemUnmapSection,            // MemUnmapSection
    };

    struct OperationData
    {
        ptr_t allocAddress;         // Allocated region base
        uint32_t allocSize;         // Allocated region size
        OperationType allocType;    // Operation type
    };

    struct PageContext
    {
        HookData hkVirtualAlloc;    // NtAllocateVirtualMemory context
        HookData hkVirtualFree;     // NtFreeVirtualMemory context
        HookData hkMapSection;      // NtMapViewOfSection context
        HookData hkUnmapSection;    // NtUnmapViewOfSection context
        CRITICAL_SECTION csLock;    // Lock to sync multi-threaded mappings
    };

public:
    BLACKBONE_API RemoteMemory( class Process* process );
    BLACKBONE_API ~RemoteMemory();

    /// <summary>
    /// Map entire process address space
    /// </summary>
    /// <param name="mapSections">Set to true to map section objects. They are converted to private pages before locking</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Map( bool mapSections );

    /// <summary>
    /// Map specific memory region
    /// </summary>
    /// <param name="base">Region base</param>
    /// <param name="size">Region size</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Map( ptr_t base, uint32_t size );

    /// <summary>
    /// Unmap process address space from current process
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Unmap();

    /// <summary>
    /// Unmap specific memory region from current process
    /// </summary>
    /// <param name="base">Region base</param>
    /// <param name="size">Region size</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Unmap( ptr_t base, uint32_t size );

    /// <summary>
    /// Translate target address accordingly to current address space
    /// </summary>
    /// <param name="address">Address to translate</param>
    /// <param name="resolveFault">If set to true, routine will try to map non-existing region upon translation failure</param>
    /// <returns>Translated address</returns>
    BLACKBONE_API ptr_t TranslateAddress( ptr_t address, bool resolveFault = true );

    /// <summary>
    /// Setup one of the 4 possible memory hooks:
    /// </summary>
    /// <param name="hkType">
    /// Type of hook to install
    /// MemVirtualAlloc - hook NtAllocateVirtualMemory
    /// MemVirtualFree  - hook NtFreeVirtualMemory
    /// MemMapSection   - hook NtMapViewOfSection
    /// MemUnmapSection - hook NtUnmapViewOfSection
    /// </param>
    /// <returns>true on success</returns>
    BLACKBONE_API NTSTATUS SetupHook( OperationType hkType );

    /// <summary>
    /// Restore previously hooked function
    /// </summary>
    /// <param name="hkType">Hook type. For more info see SetupHook</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool RestoreHook( OperationType hkType );

    /// <summary>
    /// Unmap any mapped memory, restore hooks and free resources
    /// </summary>
    BLACKBONE_API void reset();

private:
    /// <summary>
    /// Hook thread wrapper
    /// </summary>
    /// <param name="lpParam">RemoteMemory instance</param>
    /// <returns>0</returns>
    static DWORD CALLBACK HookThreadWrap( LPVOID lpParam );

    /// <summary>
    /// Thread responsible for mapping and unmapping regions intercepted by remote hooks
    /// </summary>
    void HookThread();

    /// <summary>
    /// Build remote hook function
    /// </summary>
    /// <param name="opType">Hooked function</param>
    void BuildGenericHookFn( OperationType opType );

    /// <summary>
    /// Build hook trampoline
    /// </summary>
    /// <param name="opType">Hooked function type</param>
    /// <param name="pOriginal">Original function ptr</param>
    /// <param name="pOriginalLocal">Original function address in local address space</param>
    void BuildTrampoline( OperationType opType, uintptr_t pOriginal, uint8_t* pOriginalLocal );

private:
    class Process* _process = nullptr;      // Target process
    mapMemoryMap _mapDatabase;              // Region map
    std::wstring _pipeName;                 // Pipe name used to gather hook data
    Handle _hPipe;                          // Hook pipe handle
    HANDLE _targetPipe = NULL;              // Hook pipe handle in target process
    HANDLE _hThread = NULL;                 // Hook thread listener
    PageContext* _pSharedData = nullptr;    // Hook related data, shared between processes
    ptr_t _targetShare = 0;                 // Address of shared in data in target process
    bool _active = false;                   // Hook thread activity flag
    bool _hooked[4] = { 0 };                // Hook state
};

}