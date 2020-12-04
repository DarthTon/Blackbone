#include "PDBHelper.h"
#include "../Misc/Trace.hpp"

#include <cstdint>
#include <sstream>

#define CHECK_SUCCESS(e)\
{ \
    HRESULT hresult = (e); \
    if(FAILED(hresult)) \
    { \
        BLACKBONE_TRACE( "PDB: %s: (%s) failed with HRESULT 0x%08x", __FUNCTION__, #e , hresult ); \
        return hresult; \
    } \
}

namespace blackbone
{

PDBHelper::PDBHelper()
{
    CoInitialize( nullptr );

    // If _NT_SYMBOL_PATH exists, use it
    wchar_t path[MAX_PATH] = { };
    if (GetEnvironmentVariableW( L"_NT_SYMBOL_PATH", path, _countof( path ) ) == 0)
    {
        if (GetTempPathW( _countof( path ), path ) != 0)
        {
            std::wstringstream wss;
            wss << L"srv*" << path << L"Symbols*http://msdl.microsoft.com/download/symbols";
            _sympath = wss.str();
        }
    }
}

PDBHelper::~PDBHelper()
{
    // Must be released before CoUninitialize
    _global.Release();
    _session.Release();
    _source.Release();

    CoUninitialize();
}

/// <summary>
/// Initialize symbols for target file
/// </summary>
/// <param name="file">Fully qualified path to target PE file</param>
/// <param name="base">Base ptr to add to RVAs</param>
/// <returns>Status code</returns>
HRESULT PDBHelper::Init( const std::wstring& file, ptr_t base /*= 0*/ )
{
    CHECK_SUCCESS( CoCreateDiaDataSource() );
    CHECK_SUCCESS( _source->loadDataForExe( file.c_str(), _sympath.empty() ? nullptr : _sympath.c_str(), nullptr ) );
    CHECK_SUCCESS( _source->openSession( &_session ) );
    CHECK_SUCCESS( _session->get_globalScope( &_global ) );
    
    _base = base;
    return PopulateSymbols();
}

/// <summary>
/// Get symbol RVA
/// </summary>
/// <param name="symName">Symbol name</param>
/// <param name="result">Resulting RVA</param>
/// <returns>Status code</returns>
HRESULT PDBHelper::GetSymAddress( const std::wstring& symName, ptr_t& result )
{
    auto found = _cache.find( symName );
    if (found != _cache.end())
    {
        result = _base + found->second;
        return S_OK;
    }

    return E_PDB_DBG_NOT_FOUND;
}

/// <summary>
/// Initialize DIA
/// </summary>
/// <returns>Status code</returns>
HRESULT PDBHelper::CoCreateDiaDataSource()
{
    const auto pSource = reinterpret_cast<void**>(&_source);

    // Try to get from COM
    HRESULT hr = CoCreateInstance( CLSID_DiaSource, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), pSource );

    // Retry with direct export call
    if (hr == REGDB_E_CLASSNOTREG)
    {
        HMODULE hMod = LoadLibraryW( L"msdia140.dll" );
        if (!hMod)
        {
            const auto err = GetLastError();
            BLACKBONE_TRACE( "PDB: Failed to load msdia140.dll, error 0x%08x", err );
            return HRESULT_FROM_WIN32( err );
        }

        auto DllGetClassObject = reinterpret_cast<BOOL( WINAPI* )(REFCLSID, REFIID, LPVOID)>(GetProcAddress( hMod, "DllGetClassObject" ));
        if (!DllGetClassObject)
        {
            const auto err = GetLastError();
            BLACKBONE_TRACE( "PDB: Failed to get DllGetClassObject from msdia140.dll" );
            return HRESULT_FROM_WIN32( err );
        }

        CComPtr<IClassFactory> classFactory;
        CHECK_SUCCESS( DllGetClassObject( CLSID_DiaSource, IID_IClassFactory, &classFactory ) );
        hr = classFactory->CreateInstance( nullptr, IID_IDiaDataSource, pSource );
    }

    if (FAILED( hr ))
    {
        BLACKBONE_TRACE( "PDB: %s failed with HRESULT 0x%08x", __FUNCTION__, hr );
    }

    return hr;
}

/// <summary>
/// Build module symbol map
/// </summary>
/// <returns>Status code</returns>
HRESULT PDBHelper::PopulateSymbols()
{
    CComPtr<IDiaEnumSymbols> symbols;
    CHECK_SUCCESS( _global->findChildren( SymTagNull, nullptr, nsNone, &symbols ) );

    ULONG count = 0;
    CComPtr<IDiaSymbol> symbol;
    while (SUCCEEDED( symbols->Next( 1, &symbol, &count ) ) && count != 0)
    {
        DWORD rva = 0;
        CComBSTR name;

        symbol->get_relativeVirtualAddress( &rva );
        symbol->get_undecoratedName( &name );

        if (name && rva)
        {
            std::wstring wname( name );

            // Remove x86 __stdcall decoration
            if (wname[0] == L'@' || wname[0] == L'_')
            {
                wname.erase( 0, 1 );
            }

            auto pos = wname.rfind( L'@' );
            if (pos != wname.npos)
            {
                wname.erase( pos );
            }

            _cache.emplace( std::move( wname ), rva );
        }

        symbol.Release();
    }

    return S_OK;
}

}
