#pragma once
#include "SymbolData.h"

namespace blackbone
{

namespace pe
{
    class PEImage;
}

class SymbolLoader
{
public:
    BLACKBONE_API SymbolLoader();

    /// <summary>
    /// Load symbol addresses from PDB or and pattern scans
    /// </summary>
    /// <param name="result">Found symbols</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Load( SymbolData& result );

    /// <summary>
    /// Load symbol addresses from PDBs
    /// </summary>
    /// <param name="ntdll32">Loaded x86 ntdll image</param>
    /// <param name="ntdll64">Loaded x64 ntdll image</param>
    /// <param name="result">Found symbols</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS LoadFromSymbols( const pe::PEImage& ntdll32, const pe::PEImage& ntdll64, SymbolData& result );

    /// <summary>
    /// Load symbol addresses from pattern scans
    /// </summary>
    /// <param name="ntdll32">Loaded x86 ntdll image</param>
    /// <param name="ntdll64">Loaded x64 ntdll image</param>
    /// <param name="result">Found symbols</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS LoadFromPatterns( const pe::PEImage& ntdll32, const pe::PEImage& ntdll64, SymbolData& result );

    /// <summary>
    /// Load ntdll images from the disk
    /// </summary>
    /// <returns>Loaded x86 and x64 ntdll</returns>
    BLACKBONE_API std::pair<pe::PEImage, pe::PEImage> LoadImages();

private:
    bool _x86OS;            // x86 OS
    bool _wow64Process;     // Current process is wow64 process
};

}