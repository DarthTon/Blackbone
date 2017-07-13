#include "../Config.h"
#include "ImageNET.h"

#ifdef COMPILER_MSVC

#include "mscoree.h"
#include "metahost.h"

#include <vector>

namespace blackbone
{

ImageNET::ImageNET( void )
{
}

ImageNET::~ImageNET(void)
{
    if (!_path.empty())
        CoUninitialize();
}

/// <summary>
/// Initialize COM classes
/// </summary>
/// <param name="path">Image file path</param>
/// <returns>true on success</returns>
bool ImageNET::Init( const std::wstring& path )
{
    HRESULT hr;
    VARIANT value;

    _path = path;

    if(!_pMetaDisp)
    {
        if (FAILED( CoInitialize( 0 ) ))
            return false;

        hr = CoCreateInstance(
            CLSID_CorMetaDataDispenser, NULL, CLSCTX_INPROC_SERVER,
            IID_IMetaDataDispenserEx, reinterpret_cast<void**>(&_pMetaDisp)
            );

        if (FAILED( hr ))
            return false;
    }

    //
    // Query required interfaces
    //
    if(!_pMetaImport)
    {
        hr = _pMetaDisp->OpenScope( _path.c_str(), 0, IID_IMetaDataImport, reinterpret_cast<IUnknown**>(&_pMetaImport) );
        if (hr == CLDB_E_BADUPDATEMODE)
        {
            V_VT( &value ) = VT_UI4;
            V_UI4( &value ) = MDUpdateIncremental;

            if (FAILED( hr = _pMetaDisp->SetOption( MetaDataSetUpdate, &value ) ))
                return false;

            hr = _pMetaDisp->OpenScope( _path.c_str(), 0, IID_IMetaDataImport, reinterpret_cast<IUnknown**>(&_pMetaImport) );
        }

        if (FAILED( hr ))
            return false;
    }

    if(!_pAssemblyImport)
    {
        hr = _pMetaImport->QueryInterface( IID_IMetaDataAssemblyImport, reinterpret_cast<void**>(&_pAssemblyImport) );
        if (FAILED( hr ))
            return false;
    }

    return true;
}

/// <summary>
/// Extract methods from image
/// </summary>
/// <param name="methods">Found Methods</param>
/// <returns>true on success</returns>
bool ImageNET::Parse( mapMethodRVA* methods /*= nullptr*/ )
{
    DWORD dwcTypeDefs, dwTypeDefFlags, dwcTokens, dwSigBlobSize;

    HCORENUM  hceTypeDefs    = 0;
    mdToken   tExtends       = 0;
    HCORENUM  hceMethods     = 0;
    mdTypeRef rTypeDefs[10]  = { 0 };
    WCHAR     wcName[1024]   = { 0 };
    mdToken   rTokens[10]    = { 0 };

    while (SUCCEEDED( _pMetaImport->EnumTypeDefs( &hceTypeDefs, rTypeDefs, ARRAYSIZE( rTypeDefs ), &dwcTypeDefs ) )
        && dwcTypeDefs > 0)
    {
        for (UINT i = 0; i < dwcTypeDefs; i++)
        {
            HRESULT hr = _pMetaImport->GetTypeDefProps( rTypeDefs[i], wcName, ARRAYSIZE( wcName ), NULL, &dwTypeDefFlags, &tExtends );
            if (FAILED( hr ))
                continue;

            while (SUCCEEDED( _pMetaImport->EnumMethods( &hceMethods, rTypeDefs[i], rTokens, ARRAYSIZE( rTokens ), &dwcTokens ) )
                && dwcTokens > 0)
            {
                DWORD            dwCodeRVA, dwAttr;
                WCHAR            wmName[1024] = { 0 };
                PCCOR_SIGNATURE  pbySigBlob = nullptr;

                for (UINT j = 0; j < dwcTokens; j++)
                {
                    // get method information
                    hr = _pMetaImport->GetMemberProps(
                        rTokens[j], NULL, wmName,
                        ARRAYSIZE( wmName ), NULL,
                        &dwAttr, &pbySigBlob, &dwSigBlobSize,
                        &dwCodeRVA, NULL, NULL, NULL, NULL 
                        );

                    if (FAILED( hr ))
                        continue;

                    _methods.emplace( std::make_pair( wcName, wmName ), dwCodeRVA );
                }
            }
        }
    }

    if(methods)
        *methods = _methods;

    return true;
}

using fnGetRequestedRuntimeVersion = decltype(&GetRequestedRuntimeVersion);
using fnCLRCreateInstancen = decltype(&CLRCreateInstance);

/// <summary>
/// Get image .NET runtime version
/// </summary>
/// <returns>runtime version, "n/a" if nothing found</returns>
std::wstring ImageNET::GetImageRuntimeVer( const wchar_t* ImagePath )
{
    std::wstring LatestVersion = L"n/a";

    CComPtr<ICLRMetaHost> MetaHost;
    
    // Check if .NET 4 or higher is present
    auto clrCreate = reinterpret_cast<fnCLRCreateInstancen>(
        GetProcAddress( LoadLibraryW( L"mscoree.dll" ), "CLRCreateInstance" ));

    // Legacy runtime. Get exact required version
    if(!clrCreate)
    {
        wchar_t ver[64] = { 0 };
        DWORD bytes = 0;

        auto clrGetVer = reinterpret_cast<fnGetRequestedRuntimeVersion>(
            GetProcAddress( GetModuleHandleW( L"mscoree.dll" ), "GetRequestedRuntimeVersion" ));

        clrGetVer( const_cast<LPWSTR>(ImagePath), ver, 64, &bytes );

        FreeLibrary( GetModuleHandleW( L"mscoree.dll" ) );
        return ver;
    }
    // Get highest available
    else
    {
        if (FAILED( clrCreate( CLSID_CLRMetaHost, IID_ICLRMetaHost, reinterpret_cast<LPVOID*>(&MetaHost) ) ))
            return LatestVersion;

        CComPtr<IEnumUnknown> Runtimes;
        if (FAILED( MetaHost->EnumerateInstalledRuntimes( &Runtimes ) ))
            return LatestVersion;

        CComPtr<IUnknown> Runtime;
        CComPtr<ICLRRuntimeInfo> Latest;

        while (Runtimes->Next( 1, &Runtime, NULL ) == S_OK)
        {
            CComPtr<ICLRRuntimeInfo> Current;
            wchar_t tmpString[MAX_PATH];
            DWORD tmp = MAX_PATH * sizeof(wchar_t);

            if (SUCCEEDED( Runtime->QueryInterface( IID_PPV_ARGS( &Current ) ) ))
            {
                if (!Latest)
                {
                    if (SUCCEEDED( Current->QueryInterface( IID_PPV_ARGS( &Latest ) ) ))
                    {
                        Latest->GetVersionString( tmpString, &tmp );
                        LatestVersion = tmpString;
                    }
                }
                else
                {
                    if (SUCCEEDED( Current->GetVersionString( tmpString, &tmp ) ))
                    {
                        std::wstring CurrentVersion = tmpString;
                        if (CurrentVersion.compare( LatestVersion ) > 0)
                        {
                            LatestVersion = CurrentVersion;
                            Latest.Release();
                            Current->QueryInterface( IID_PPV_ARGS( &Latest ) );
                        }
                    }
                }
            }

            Runtime.Release();
        }

        return LatestVersion;
    }
    
}

}

#endif