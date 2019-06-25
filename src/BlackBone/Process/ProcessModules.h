#pragma once

#include "../Config.h"
#include "../Include/Winheaders.h"
#include "../Include/CallResult.h"
#include "../Include/Types.h"
#include "../PE/PEImage.h"
#include "../Misc/Utils.h"
#include "Threads/Thread.h"

#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>

namespace std
{
    template <>
    struct hash < struct pair<wstring, blackbone::eModType> >
    {
        size_t operator()( const pair<wstring, blackbone::eModType>& value ) const
        {
            hash<wstring> sh;
            return sh( value.first ) ^ value.second;
        }
    };
}


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
    using mapModules = std::unordered_map<std::pair<std::wstring, eModType>, ModuleDataPtr> ;

public:
    BLACKBONE_API ProcessModules( class Process& proc );
    BLACKBONE_API ~ProcessModules();

    /// <summary>
    /// Get module by name
    /// </summary>
    /// <param name="name">Module name</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <param name="search">Search type.</param>
    /// <returns>Module data. nullptr if not found</returns>
    BLACKBONE_API ModuleDataPtr GetModule(
        const std::wstring& name,
        eModSeachType search = LdrList,
        eModType type = mt_default
        );

    /// <summary>
    /// Get module by name
    /// </summary>
    /// <param name="name">Module name.</param>
    /// <param name="search">Search type.</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <param name="baseModule">Import module name. Used only to resolve ApiSchema during manual map</param>
    /// <returns>Module data. nullptr if not found</returns>
    BLACKBONE_API ModuleDataPtr GetModule(
        std::wstring& name,
        eModSeachType search = LdrList,
        eModType type = mt_default,
        const wchar_t* baseModule = L""
        );

    /// <summary>
    /// Get module by base address
    /// </summary>
    /// <param name="modBase">Module base address</param>
    /// <param name="strict">If true modBase must exactly match module base address</param>
    /// <param name="search">Search type.</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <returns>Module data. nullptr if not found</returns>
    BLACKBONE_API ModuleDataPtr GetModule(
        module_t modBase,
        bool strict = true,
        eModSeachType search = LdrList,
        eModType type = mt_default
        );

    /// <summary>
    /// Get process main module
    /// </summary>
    /// <returns>Module data. nullptr if not found</returns>
    BLACKBONE_API ModuleDataPtr GetMainModule();

    /// <summary>
    /// Enumerate all process modules
    /// </summary>
    /// <param name="search">Search method</param>
    /// <returns>Module list</returns>
    BLACKBONE_API const mapModules& GetAllModules( eModSeachType search = LdrList );

    /// <summary>
    /// Get list of manually mapped modules
    /// </summary>
    /// <returns>List of modules</returns>
    BLACKBONE_API mapModules GetManualModules();

    /// <summary>
    /// Get export address. Forwarded exports will be automatically resolved if forward module is present
    /// </summary>
    /// <param name="hMod">Module to search in</param>
    /// <param name="name_ord">Function name or ordinal</param>
    /// <param name="baseModule">Import module name. Only used to resolve ApiSchema during manual map.</param>
    /// <returns>Export info. If failed procAddress field is 0</returns>
    BLACKBONE_API call_result_t<exportData> GetExport( 
        const ModuleDataPtr& hMod, 
        const char* name_ord, 
        const wchar_t* baseModule = L"" 
    );

    /// <summary>
    /// Get export address. Forwarded exports will be automatically resolved if forward module is present
    /// </summary>
    /// <param name="hMod">Module to search in</param>
    /// <param name="name_ord">Function name or ordinal</param>
    /// <param name="baseModule">Import module name. Only used to resolve ApiSchema during manual map.</param>
    /// <returns>Export info. If failed procAddress field is 0</returns>
    BLACKBONE_API call_result_t<exportData> GetExport(
        const ModuleData& hMod,
        const char* name_ord,
        const wchar_t* baseModule = L""
    );

    /// <summary>
    /// Get export address. Forwarded exports will be automatically resolved if forward module is present
    /// </summary>
    /// <param name="modName">Module name to search in</param>
    /// <param name="name_ord">Function name or ordinal</param>
    /// <returns>Export info. If failed procAddress field is 0</returns>
    BLACKBONE_API call_result_t<exportData> GetExport( const std::wstring& modName, const char* name_ord );

    /// <summary>
    /// Get export from ntdll
    /// </summary>
    /// <param name="name_ord">Function name or ordinal</param>
    /// <param name="type">Module type. 32 bit or 64 bit</param>
    /// <param name="search">Search type.</param>
    /// <returns>Export info. If failed procAddress field is 0</returns>
    BLACKBONE_API call_result_t<exportData> GetNtdllExport(
        const char* name_ord, 
        eModType type = mt_default, 
        eModSeachType search = LdrList 
    );

    /// <summary>
    /// Inject image into target process
    /// </summary>
    /// <param name="path">Full-qualified image path</param>
    /// <returns>Module info. nullptr if failed</returns>
    BLACKBONE_API call_result_t<ModuleDataPtr> Inject( const std::wstring& path, ThreadPtr pThread = nullptr );

#ifdef COMPILER_MSVC
    /// <summary>
    /// Inject pure IL image.
    /// </summary>
    /// <param name="netVersion">NET runtime version to use</param>
    /// <param name="netAssemblyPath">Path to image</param>
    /// <param name="netAssemblyMethod">Method to call</param>
    /// <param name="netAssemblyArgs">Arguments passed into method</param>
    /// <param name="returnCode">Return code</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool InjectPureIL(
        const std::wstring& netVersion,
        const std::wstring& netAssemblyPath,
        const std::wstring& netAssemblyMethod,
        const std::wstring& netAssemblyArgs,
        DWORD& returnCode
        );
#endif

    /// <summary>
    /// Unload specific module from target process. Can't be used to unload manually mapped modules
    /// </summary>
    /// <param name="hMod">Module to unload</param>
    /// <returns>true on success</returns>
    BLACKBONE_API NTSTATUS Unload( const ModuleDataPtr& hMod );

    /// <summary>
    /// Unlink module from most loader structures
    /// </summary>
    /// <param name="mod">Module to unlink</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Unlink( const ModuleDataPtr& mod );
    BLACKBONE_API bool Unlink( const ModuleData& mod );

    /// <summary>
    /// Store manually mapped module in module list
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>Module info</returns>
    BLACKBONE_API ModuleDataPtr AddManualModule( const ModuleData& mod );

    /// <summary>
    /// Canonicalize paths and set module type to manual if requested
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <param name="manual">Value to set ModuleData::manual to</param>
    /// <returns>Module data</returns>
    BLACKBONE_API ModuleData Canonicalize( const ModuleData& mod, bool manual );

    /// <summary>
    /// Remove module from module list
    /// </summary>
    /// <param name="filename">Module file name</param>
    /// <param name="mt">Module type. 32 bit or 64 bit</param>
    BLACKBONE_API void RemoveManualModule( const std::wstring& filename, eModType mt );

    /// <summary>
    /// Ensure module is a valid PE image
    /// </summary>
    /// <param name="base">Module base address</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool ValidateModule( module_t base );

    /// <summary>
    /// Reset local data
    /// </summary>
    BLACKBONE_API void reset();

private:
    ProcessModules( const ProcessModules& ) = delete;
    ProcessModules operator =(const ProcessModules&) = delete;

    void UpdateModuleCache( eModSeachType search, eModType type );

private:
    class Process&       _proc;
    class ProcessMemory& _memory;
    class ProcessCore&   _core;

    mapModules _modules;            // Fast lookup cache
    CriticalSection _modGuard;      // Module guard        
    bool _ldrPatched;               // Win7 loader patch flag
};

};
