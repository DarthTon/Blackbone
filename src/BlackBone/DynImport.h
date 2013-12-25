#pragma once

#include "Types.h"
#include "Winheaders.h"

#include <unordered_map>
#include <mutex>

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
        std::lock_guard<std::mutex> lg( _mapGuard );

        auto iter = _funcs.find( name );
        if (iter != _funcs.end())
            return reinterpret_cast<T>(iter->second);

        return nullptr;
    }

    /// <summary>
    /// Load function into database
    /// </summary>
    /// <param name="name">Function name</param>
    /// <param name="module">Module name</param>
    /// <returns>true on success</returns>
    static bool load( const std::string& name, const std::wstring& module );

    /// <summary>
    /// Load function into database
    /// </summary>
    /// <param name="name">Function name</param>
    /// <param name="hMod">Module base</param>
    /// <returns>true on success</returns>
    static bool load( const std::string& name, HMODULE hMod );

private:
    static std::unordered_map<std::string, FARPROC> _funcs;     // function database
    static std::mutex _mapGuard;                                // function database guard
};

// Syntax sugar
#define GET_IMPORT(name) (DynImport::get<fn ## name>( #name ))

}