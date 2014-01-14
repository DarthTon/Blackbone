#pragma once

#include "Winheaders.h"
#include "PEParser.h"

#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <mutex>

template <>
struct std::hash< std::pair<std::wstring, blackbone::eModType> >
{

public:
    size_t operator()( const std::pair<std::wstring, blackbone::eModType>& value ) const
    {
        const hash<std::wstring> ah;
        return ah( value.first ) ^ value.second;
    }
};


namespace blackbone
{


struct exportData
{
    ptr_t procAddress = 0;          // Function address

    std::wstring forwardModule;     // Name of forward module
    std::string forwardName;        // Forwarded function name
    WORD forwardOrdinal = 0;        // Forwarded function ordinal

    bool isForwarded = false;       // Function is forwarded to another module
    bool forwardByOrd = false;      // Forward is done by ordinal
};

class ProcessModules
{
public:
    typedef std::unordered_map<std::pair<std::wstring, eModType>, ModuleData> mapModules;

public:
    ProcessModules( class Process& proc );
    ~ProcessModules();

    /// <summary>
    /// Get module by name
    /// </summary>
    /// <param name="name">Module name</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <param name="search">Saerch type.</param>
    /// <returns>Module data. nullptr if not found</returns>
    const ModuleData* GetModule( const std::wstring& name, 
                                 eModSeachType search = LdrList,
                                 eModType type = mt_default );

    /// <summary>
    /// Get module by name
    /// </summary>
    /// <param name="name">TModule name.</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <param name="baseModule">Import module name. Used only to resolve ApiSchema during manual map</param>
    /// <returns>Module data. nullptr if not found</returns>
    const ModuleData* GetModule( std::wstring& name, 
                                 eModSeachType search = LdrList,
                                 eModType type = mt_default,
                                 const wchar_t* baseModule = L"" );

    /// <summary>
    /// Get module by base address
    /// </summary>
    /// <param name="modBase">Module base address</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <param name="search">Saerch type.</param>
    /// <returns>Module data. nullptr if not found</returns>
    const ModuleData* GetModule( module_t modBase,
                                 eModSeachType search = LdrList,
                                 eModType type = mt_default );

    /// <summary>
    /// Get process main module
    /// </summary>
    /// <returns>Module data. nullptr if not found</returns>
    const ModuleData* GetMainModule();

    /// <summary>
    /// Enumerate all process modules
    /// </summary>
    /// <param name="search">Search method</param>
    /// <returns>Module list</returns>
    const ProcessModules::mapModules& GetAllModules( eModSeachType search = LdrList);

    /// <summary>
    /// Get list of manually mapped modules
    /// </summary>
    /// <param name="mods">List of modules</param>
    void GetManualModules( ProcessModules::mapModules& mods );

    /// <summary>
    /// Get export address. Forwarded exports will be automatically resolved if forward module is present
    /// </summary>
    /// <param name="hMod">Module to search in</param>
    /// <param name="name_ord">Function name or ordinal</param>
    /// <param name="baseModule">Import module name. Only used to resolve ApiSchema during manual map.</param>
    /// <returns>Export info. If failed procAddress field is 0</returns>
    exportData GetExport( const ModuleData* hMod, const char* name_ord, const wchar_t* baseModule = L"" );

    /// <summary>
    /// Inject image into target process
    /// </summary>
    /// <param name="path">Full-qualified image path</param>
    /// <returns>Module info. nullptr if failed</returns>
    const ModuleData* Inject( const std::wstring& path );

    /// <summary>
    /// Inject pure IL image.
    /// </summary>
    /// <param name="netVersion">NET runtime version to use</param>
    /// <param name="netAssemblyPath">Path to image</param>
    /// <param name="netAssemblyMethod">Method to call</param>
    /// <param name="netAssemblyArgs">Arguments passed into method</param>
    /// <param name="returnCode">Return code</param>
    /// <returns>true on success</returns>
    bool InjectPureIL( const std::wstring& netVersion,
                       const std::wstring& netAssemblyPath,
                       const std::wstring& netAssemblyMethod, 
                       const std::wstring& netAssemblyArgs, 
                       DWORD& returnCode );

    /// <summary>
    /// Unload specific module from target process. Can't be used to unload manually mapped modules
    /// </summary>
    /// <param name="hMod">Module to unload</param>
    /// <returns>true on success</returns>
    bool Unload( const ModuleData* hMod );

    /// <summary>
    /// Unlink module from most loader structures
    /// </summary>
    /// <param name="mod">Module to unlink</param>
    /// <returns>true on success</returns>
    bool Unlink( const ModuleData* mod );

    /// <summary>
    /// Store manually mapped module in module list
    /// </summary>
    /// <param name="FilePath">Full qualified module path</param>
    /// <param name="base">Base address</param>
    /// <param name="size">Module size</param>
    /// <param name="mt">Module type. 32 bit or 64 bit</param>
    /// <returns>Module info</returns>
    const ModuleData* AddManualModule( const std::wstring& FilePath, module_t base, size_t size, eModType mt );

    /// <summary>
    /// Remove module from module list
    /// </summary>
    /// <param name="filename">Module file name</param>
    /// <param name="mt">Module type. 32 bit or 64 bit</param>
    void RemoveManualModule( const std::wstring& filename, eModType mt );

    /// <summary>
    /// Ensure module is a valid PE image
    /// </summary>
    /// <param name="base">Module base address</param>
    /// <returns>true on success</returns>
    bool ValidateModule( module_t base );

    /// <summary>
    /// Reset local data
    /// </summary>
    void reset();

private:
    ProcessModules( const ProcessModules& ) = delete;
    ProcessModules operator =(const ProcessModules&) = delete;

private:
    class Process&       _proc;
    class ProcessMemory& _memory;
    class ProcessCore&   _core;

    mapModules _modules;    // Fast lookup cache
    std::mutex _modGuard;   // Module guard        
    bool _ldrPatched;       // Win7 loader patch flag
};

};
