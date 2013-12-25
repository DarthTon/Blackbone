#pragma once

#include "Winheaders.h"
#include <cor.h>
#include <CorError.h>

#include <map>
#include <atlbase.h>

namespace blackbone
{

/// <summary>
/// .NET metadata parser
/// </summary>
class ImageNET
{
public:
    typedef std::map<std::pair<std::wstring, std::wstring>, size_t> mapMethodRVA;

public:
    ImageNET(void);
    ~ImageNET(void);

    /// <summary>
    /// Initialize COM classes
    /// </summary>
    /// <param name="path">Image file path</param>
    /// <returns>true on success</returns>
    bool Init( const std::wstring& path );

    /// <summary>
    /// Extract methods from image
    /// </summary>
    /// <param name="methods">Found Methods</param>
    /// <returns>true on success</returns>
    bool Parse( mapMethodRVA& methods );

    /// <summary>
    /// Get image .NET runtime version
    /// </summary>
    /// <returns>runtime version, "n/a" if nothing found</returns>
    static std::wstring GetImageRuntimeVer( const wchar_t* ImagePath );
private:
    std::wstring _path;         // Image path
    mapMethodRVA _methods;      // Image methods

    // COM helpers
    CComPtr<IMetaDataDispenserEx>    _pMetaDisp;
    CComPtr<IMetaDataImport>         _pMetaImport;
    CComPtr<IMetaDataAssemblyImport> _pAssemblyImport;
};

}