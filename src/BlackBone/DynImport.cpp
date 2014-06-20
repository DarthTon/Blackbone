#include "DynImport.h"

namespace blackbone
{

decltype(DynImport::_funcs)     DynImport::_funcs;
decltype(DynImport::_mapGuard)  DynImport::_mapGuard;

/// <summary>
/// Load function into database
/// </summary>
/// <param name="name">Function name</param>
/// <param name="module">Module name</param>
/// <returns>true on success</returns>
FARPROC DynImport::load( const std::string& name, const std::wstring& module )
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
FARPROC blackbone::DynImport::load( const std::string& name, HMODULE hMod )
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

}