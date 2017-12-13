#pragma once
#include "SymbolData.h"

namespace blackbone
{

class SymbolLoader
{
public:
    SymbolLoader();

    /// <summary>
    /// Load symbol addresses from PDB or and pattern scans
    /// </summary>
    /// <param name="result">Found symbols</param>
    /// <returns>Status code</returns>
    NTSTATUS Load( SymbolData& result );

private:
    bool _x86OS;            // x86 OS
    bool _wow64Process;     // Current process is wow64 process
};

}