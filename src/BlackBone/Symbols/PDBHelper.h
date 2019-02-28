#pragma once
#include "../include/Types.h"
#include <3rd_party/DIA/dia2.h>
#include <Objbase.h>
#include <atlcomcli.h>
#include <string>
#include <unordered_map>

namespace blackbone
{

class PDBHelper
{
public:
    PDBHelper();
    ~PDBHelper();

    /// <summary>
    /// Initialize symbols for target file
    /// </summary>
    /// <param name="file">Fully qualified path to target PE file</param>
    /// <param name="base">Base ptr to add to RVAs</param>
    /// <returns>Status code</returns>
    HRESULT Init( const std::wstring& file, ptr_t base = 0 );

    /// <summary>
    /// Get symbol RVA
    /// </summary>
    /// <param name="symName">Symbol name</param>
    /// <param name="result">Resulting RVA</param>
    /// <returns>Status code</returns>
    HRESULT GetSymAddress( const std::wstring& symName, ptr_t& result );

private:
    /// <summary>
    /// Initialize DIA
    /// </summary>
    /// <returns>Status code</returns>
    HRESULT CoCreateDiaDataSource();

    /// <summary>
    /// Build module symbol map
    /// </summary>
    /// <returns>Status code</returns>
    HRESULT PopulateSymbols();

private:
    // DIA interfaces
    CComPtr<IDiaDataSource> _source;
    CComPtr<IDiaSession>    _session;
    CComPtr<IDiaSymbol>     _global;

    uint64_t _base = 0;                                     // Base ptr to add to RVAs
    std::wstring _sympath;                                  // Symbol cache directory
    std::unordered_map<std::wstring, uint32_t> _cache;      // Symbol name <--> RVA map
};

}