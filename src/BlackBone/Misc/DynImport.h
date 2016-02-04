#pragma once

#include "../Include/Types.h"
#include "../Include/Winheaders.h"
#include "Utils.h"

#include <unordered_map>

namespace blackbone
{

/// <summary>
/// Dynamic import
/// </summary>
class DynImport
{
public:
    /// <summary>
    /// Get dll function
    /// </summary>
    /// <param name="name">Function name</param>
    /// <returns>Function pointer</returns>
    template<typename T>
    inline static T get( const std::string& name ) 
    {
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
    inline static NTSTATUS safeNativeCall( const std::string& name, Args&&... args )
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
    inline static auto safeCall( const std::string& name, Args&&... args ) -> typename std::result_of<T(Args...)>::type
    {
        auto pfn = DynImport::get<T>( name );
        return pfn ? pfn( std::forward<Args>( args )... ) : (std::result_of<T( Args... )>::type)(0);
    }

    /// <summary>
    /// Load function into database
    /// </summary>
    /// <param name="name">Function name</param>
    /// <param name="module">Module name</param>
    /// <returns>true on success</returns>
    BLACKBONE_API static FARPROC load( const std::string& name, const std::wstring& module );

    /// <summary>
    /// Load function into database
    /// </summary>
    /// <param name="name">Function name</param>
    /// <param name="hMod">Module base</param>
    /// <returns>true on success</returns>
    BLACKBONE_API static FARPROC load( const std::string& name, HMODULE hMod );

private:
    BLACKBONE_API static std::unordered_map<std::string, FARPROC> _funcs;     // function database
    BLACKBONE_API static CriticalSection _mapGuard;                           // function database guard
};

// Syntax sugar
#define GET_IMPORT(name) (DynImport::get<fn ## name>( #name ))
#define SAFE_NATIVE_CALL(name, ...) (DynImport::safeNativeCall<fn ## name>( #name, __VA_ARGS__ ))
#define SAFE_CALL(name, ...) (DynImport::safeCall<fn ## name>( #name, __VA_ARGS__ ))

}