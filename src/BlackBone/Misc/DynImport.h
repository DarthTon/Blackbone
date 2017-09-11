#pragma once

#include "../Include/Types.h"
#include "../Include/Winheaders.h"
#include "Utils.h"
#include "InitOnce.h"

#include <unordered_map>

namespace blackbone
{

/// <summary>
/// Dynamic import
/// </summary>
class DynImport
{
public:
    static DynImport& Instance()
    {
        static DynImport instance;
        return instance;
    }

    /// <summary>
    /// Get dll function
    /// </summary>
    /// <param name="name">Function name</param>
    /// <returns>Function pointer</returns>
    template<typename T>
    inline T get( const std::string& name ) 
    {
        InitializeOnce();

        CSLock lck( _mapGuard );

        auto iter = _funcs.find( name );
        if (iter != _funcs.end())
            return reinterpret_cast<T>(iter->second);

        return nullptr;
    }

    /// <summary>
    /// Safely call import
    /// If import not found - return STATUS_ORDINAL_NOT_FOUND
    /// </summary>
    /// <param name="name">Import name.</param>
    /// <param name="...args">Function args</param>
    /// <returns>Function result or STATUS_ORDINAL_NOT_FOUND if import not found</returns>
    template<typename T, typename... Args>
    inline NTSTATUS safeNativeCall( const std::string& name, Args&&... args )
    {
        auto pfn = DynImport::get<T>( name );
        return pfn ? pfn( std::forward<Args>( args )... ) : STATUS_ORDINAL_NOT_FOUND;
    }

    /// <summary>
    /// Safely call import
    /// If import not found - return 0
    /// </summary>
    /// <param name="name">Import name.</param>
    /// <param name="...args">Function args</param>
    /// <returns>Function result or 0 if import not found</returns>
    template<typename T, typename... Args>
    inline auto safeCall( const std::string& name, Args&&... args )
    {
        auto pfn = DynImport::get<T>( name );
        return pfn ? pfn( std::forward<Args>( args )... ) : std::result_of_t<T( Args... )>();
    }

    /// <summary>
    /// Load function into database
    /// </summary>
    /// <param name="name">Function name</param>
    /// <param name="module">Module name</param>
    /// <returns>true on success</returns>
    BLACKBONE_API FARPROC load( const std::string& name, const std::wstring& module )
    {
        auto mod = GetModuleHandleW( module.c_str() );
        return load( name, mod );
    }

    /// <summary>
    /// Load function into database
    /// </summary>
    /// <param name="name">Function name</param>
    /// <param name="hMod">Module base</param>
    /// <returns>true on success</returns>
    BLACKBONE_API FARPROC load( const std::string& name, HMODULE hMod )
    {
        CSLock lck( _mapGuard );

        auto proc = GetProcAddress( hMod, name.c_str() );
        if (proc)
        {
            _funcs.insert( std::make_pair( name, proc ) );
            return proc;
        }

        return nullptr;
    }

private:
    DynImport() = default;
    DynImport( const DynImport& ) = delete;

private:
    std::unordered_map<std::string, FARPROC> _funcs;    // function database
    CriticalSection _mapGuard;                          // function database guard
};

// Syntax sugar
#define LOAD_IMPORT(name, module) (DynImport::Instance().load( name, module ))
#define GET_IMPORT(name) (DynImport::Instance().get<fn ## name>( #name ))
#define SAFE_NATIVE_CALL(name, ...) (DynImport::Instance().safeNativeCall<fn ## name>( #name, __VA_ARGS__ ))
#define SAFE_CALL(name, ...) (DynImport::Instance().safeCall<fn ## name>( #name, __VA_ARGS__ ))

}