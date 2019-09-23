#include "SymbolLoader.h"
#include "PDBHelper.h"
#include "../Symbols/PatternLoader.h"
#include "../PE/PEImage.h"

namespace blackbone
{

SymbolLoader::SymbolLoader() 
    : _x86OS( false )
    , _wow64Process( false )
{
    SYSTEM_INFO info = { };
    GetNativeSystemInfo( &info );

    if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        _x86OS = true;
    }
    else
    {
        BOOL wowSrc = FALSE;
        IsWow64Process( GetCurrentProcess(), &wowSrc );
        _wow64Process = wowSrc != 0;
    }
}

/// <summary>
/// Load symbol addresses from PDB or and pattern scans
/// </summary>
/// <param name="result">Found symbols</param>
/// <returns>Status code</returns>
NTSTATUS SymbolLoader::Load( SymbolData& result )
{
    auto [ntdll32, ntdll64] = LoadImages();

    // Get addresses from pdb
    LoadFromSymbols( ntdll32, ntdll64, result );
   
    // Fill missing symbols from patterns
    return LoadFromPatterns( ntdll32, ntdll64, result );
}

/// <summary>
/// Load symbol addresses from PDBs
/// </summary>
/// <param name="ntdll32">Loaded x86 ntdll image</param>
/// <param name="ntdll64">Loaded x64 ntdll image</param>
/// <param name="result">Found symbols</param>
/// <returns>Status code</returns>
NTSTATUS SymbolLoader::LoadFromSymbols( const pe::PEImage& ntdll32, const pe::PEImage& ntdll64, SymbolData& result )
{
    PDBHelper sym32, sym64;
    HRESULT hr = sym32.Init( ntdll32.path(), ntdll32.imageBase() );
    if (!_x86OS && SUCCEEDED( hr ))
    {
        FsRedirector fsr( _wow64Process );
        hr = sym64.Init( ntdll64.path(), ntdll64.imageBase() );
    }

    if (SUCCEEDED( hr ))
    {
        sym32.GetSymAddress( L"LdrpHandleTlsData", result.LdrpHandleTlsData32 );
        sym64.GetSymAddress( L"LdrpHandleTlsData", result.LdrpHandleTlsData64 );

        sym32.GetSymAddress( L"LdrpInvertedFunctionTable", result.LdrpInvertedFunctionTable32 );
        sym64.GetSymAddress( L"LdrpInvertedFunctionTable", result.LdrpInvertedFunctionTable64 );

        sym32.GetSymAddress( L"RtlInsertInvertedFunctionTable", result.RtlInsertInvertedFunctionTable32 );
        sym64.GetSymAddress( L"RtlInsertInvertedFunctionTable", result.RtlInsertInvertedFunctionTable64 );

        sym32.GetSymAddress( L"LdrpReleaseTlsEntry", result.LdrpReleaseTlsEntry32 );
        sym64.GetSymAddress( L"LdrpReleaseTlsEntry", result.LdrpReleaseTlsEntry64 );

        sym32.GetSymAddress( L"LdrProtectMrdata", result.LdrProtectMrdata );

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/// <summary>
/// Load symbol addresses from pattern scans
/// </summary>
/// <param name="ntdll32">Loaded x86 ntdll image</param>
/// <param name="ntdll64">Loaded x64 ntdll image</param>
/// <param name="result">Found symbols</param>
/// <returns>Status code</returns>
NTSTATUS SymbolLoader::LoadFromPatterns( const pe::PEImage& ntdll32, const pe::PEImage& ntdll64, SymbolData& result )
{
    return ScanSymbolPatterns( ntdll32, ntdll64, result );
}

/// <summary>
/// Load ntdll images from the disk
/// </summary>
/// <returns>Loaded x86 and x64 ntdll</returns>
std::pair<pe::PEImage, pe::PEImage> SymbolLoader::LoadImages()
{
    pe::PEImage ntdll32, ntdll64;

    wchar_t buf[MAX_PATH] = { 0 };
    GetWindowsDirectoryW( buf, MAX_PATH );
    std::wstring windir( buf );

    if (_x86OS)
    {
        ntdll32.Load( std::wstring( windir + L"\\System32\\ntdll.dll" ), true );
    }
    else
    {
        FsRedirector fsr( _wow64Process );

        ntdll64.Load( std::wstring( windir + L"\\System32\\ntdll.dll" ), true );
        ntdll32.Load( std::wstring( windir + L"\\SysWOW64\\ntdll.dll" ), true );
    }

    return std::make_pair( std::move( ntdll32 ), std::move( ntdll64 ) );
}

}