#include "DynImport.h"

std::unordered_map<std::string, FARPROC> blackbone::DynImport::_funcs;
std::mutex blackbone::DynImport::_mapGuard;

namespace blackbone
{

/// <summary>
/// Load function into database
/// </summary>
/// <param name="name">Function name</param>
/// <param name="module">Module name</param>
/// <returns>true on success</returns>
bool DynImport::load( const std::string& name, const std::wstring& module )
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
bool blackbone::DynImport::load( const std::string& name, HMODULE hMod )
{
    std::lock_guard<std::mutex> lg( _mapGuard );

    auto proc = GetProcAddress( hMod, name.c_str() );
    if (proc)
    {
        _funcs.insert( std::make_pair( name, proc ) );
        return true;
    }

    return false;
}

}