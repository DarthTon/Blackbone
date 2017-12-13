#pragma once
#include "../Symbols/SymbolData.h"
#include "../PE/PEImage.h"

namespace blackbone
{

/// <summary>
/// Scan ntdll for internal loader data
/// </summary>
/// <param name="ntdll32">Mapped x86 ntdll</param>
/// <param name="ntdll64">Mapped x64 ntdll</param>
/// <param name="result">Result</param>
/// <returns>Status code</returns>
NTSTATUS ScanSymbolPatterns( const pe::PEImage& ntdll32, const pe::PEImage& ntdll64, SymbolData& result );

}