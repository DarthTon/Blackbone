#include "../Config.h"
#include "ProcessModules.h"
#include "Process.h"
#include "RPC/RemoteExec.h"
#include "../Include/Exception.h"
#include "../Misc/NameResolve.h"
#include "../Misc/Utils.h"
#include "../Symbols/SymbolData.h"
#include "../Asm/AsmFactory.h"

#include <memory>
#include <type_traits>
#include <iterator>

#ifdef COMPILER_MSVC
#include <mscoree.h>
#include <metahost.h>
#endif

#include <3rd_party/VersionApi.h>

namespace blackbone
{

ProcessModules::ProcessModules( class Process* proc )
    : _proc( proc )
    , _memory( &_proc->memory() )
    , _core( &_proc->core() )
    , _ldrPatched( false )
{
}

/// <summary>
/// Get module by name
/// </summary>
/// <param name="name">Module name</param>
/// <param name="type">Module type. 32 bit or 64 bit</param>
/// <param name="search">Search type.</param>
/// <returns>Module data. nullptr if not found</returns>
ModuleDataPtr ProcessModules::GetModule(
    const std::wstring& name,
    eModSeachType search /*= LdrList*/,
    eModType type /*= mt_default*/
)
{
    std::wstring namecopy( Utils::StripPath( name ) );
    return GetModule( namecopy, search, type );
}

/// <summary>
/// Get module by name
/// </summary>
/// <param name="name">Module name.</param>
/// <param name="search">Search type.</param>
/// <param name="type">Module type. 32 bit or 64 bit</param>
/// <param name="baseModule">Import module name. Used only to resolve ApiSchema during manual map</param>
/// <returns>Module data. nullptr if not found</returns>
ModuleDataPtr ProcessModules::GetModule(
    std::wstring& name,
    eModSeachType search /*= LdrList*/,
    eModType type /*= mt_default*/,
    const wchar_t* baseModule /*= L""*/
)
{
    NameResolve::Instance().ResolvePath( name, Utils::StripPath( baseModule ), L"", NameResolve::ApiSchemaOnly, *_proc );

    // Detect module type
    if (type == mt_default)
        type = _proc->barrier().targetWow64 ? mt_mod32 : mt_mod64;

    CSLock lck( _modGuard );

    auto key = std::make_pair( name, type );

    // Fast lookup
    if (_modules.count( key ) && (_modules[key]->manual || ValidateModule( _modules[key]->baseAddress )))
        return _modules[key];

    UpdateModuleCache( search, type );

    if (_modules.count( key ))
        return _modules[key];

    return nullptr;
}

/// <summary>
/// Get module by base address
/// </summary>
/// <param name="modBase">Module base address</param>
/// <param name="strict">If true modBase must exactly match module base address</param>
/// <param name="search">Search type.</param>
/// <param name="type">Module type. 32 bit or 64 bit</param>
/// <returns>Module data. nullptr if not found</returns>
ModuleDataPtr ProcessModules::GetModule(
    module_t modBase,
    bool strict /*= true*/,
    eModSeachType search /*= LdrList*/,
    eModType type /*= mt_default */
)
{
    // Detect module type
    if (type == mt_default)
        type = _proc->barrier().targetWow64 ? mt_mod32 : mt_mod64;

    CSLock lck( _modGuard );

    auto compFn = [modBase, strict]( const mapModules::value_type& val )
    {
        if (strict)
            return (val.second->baseAddress == modBase);
        else
            return (modBase >= val.second->baseAddress && modBase < val.second->baseAddress + val.second->size);
    };

    auto iter = std::find_if( _modules.begin(), _modules.end(), compFn );

    if (iter != _modules.end())
        return iter->second;

    UpdateModuleCache( search, type );

    iter = std::find_if( _modules.begin(), _modules.end(), compFn );

    if (iter != _modules.end())
        return iter->second;

    return nullptr;
}

/// <summary>
/// Get process main module
/// </summary>
/// <returns>Module data. nullptr if not found</returns>
ModuleDataPtr ProcessModules::GetMainModule()
{
    if (_proc->barrier().targetWow64)
    {
        _PEB32 peb = { };
        if (_proc->core().peb32( &peb ) == 0)
            return nullptr;

        return GetModule( peb.ImageBaseAddress );
    }
    else
    {
        _PEB64 peb = { };
        if (_proc->core().peb64( &peb ) == 0)
            return nullptr;

        return GetModule( peb.ImageBaseAddress );
    }
}

/// <summary>
/// Enumerate all process modules
/// </summary>
/// <param name="search">Search method</param>
/// <returns>Module list</returns>
const ProcessModules::mapModules& ProcessModules::GetAllModules( eModSeachType search /*= LdrList*/ )
{
    eModType mt = _core->isWow64() ? mt_mod32 : mt_mod64;
    CSLock lck( _modGuard );

    // Remove non-manual modules
    for (auto iter = _modules.begin(); iter != _modules.end();)
    {
        if (!iter->second->manual)
            _modules.erase( iter++ );
        else
            ++iter;
    }

    UpdateModuleCache( search, mt );

    // Do additional search in case of loader lists
    // This, however won't search for 32 bit modules in native x64 process
    if (search == LdrList && mt == mt_mod32)
        UpdateModuleCache( search, mt_mod64 );

    return _modules;
}

/// <summary>
/// Get list of manually mapped modules
/// </summary>
/// <returns>List of modules</returns>
ProcessModules::mapModules ProcessModules::GetManualModules()
{
    ProcessModules::mapModules mods;
    std::copy_if(
        _modules.begin(), _modules.end(),
        std::inserter( mods, mods.end() ),
        []( const auto& mod ) { return mod.second->manual; }
    );

    return mods;
}

/// <summary>
/// Get export address. Forwarded exports will be automatically resolved if forward module is present
/// </summary>
/// <param name="hMod">Module to search in</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <param name="baseModule">Import module name. Only used to resolve ApiSchema during manual map.</param>
/// <returns>Export info. If failed procAddress field is 0</returns>
exportData ProcessModules::GetExport( const ModuleDataPtr& hMod, const char* name_ord, const wchar_t* baseModule /*= L""*/ )
{
    return GetExport( *hMod, name_ord, baseModule );
}

/// <summary>
/// Get export address. Forwarded exports will be automatically resolved if forward module is present
/// </summary>
/// <param name="hMod">Module to search in</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <param name="baseModule">Import module name. Only used to resolve ApiSchema during manual map.</param>
/// <returns>Export info. If failed procAddress field is 0</returns>
exportData ProcessModules::GetExport( const ModuleData& hMod, const char* name_ord, const wchar_t* baseModule /*= L"0"*/ )
{
    exportData data;

    // Invalid module
    if (hMod.baseAddress == 0)
        THROW_AND_LOG( "invalid module base address: 0x%x", hMod.baseAddress );

    std::unique_ptr<IMAGE_EXPORT_DIRECTORY, decltype(&free)> expData( nullptr, &free );

    IMAGE_DOS_HEADER hdrDos = { };
    uint8_t hdrNt32[sizeof( IMAGE_NT_HEADERS64 )] = { };
    auto phdrNt32 = reinterpret_cast<PIMAGE_NT_HEADERS32>(hdrNt32);
    auto phdrNt64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(hdrNt32);
    DWORD expSize = 0;
    uintptr_t expBase = 0;

    _memory->Read( hMod.baseAddress, sizeof( hdrDos ), &hdrDos );

    if (hdrDos.e_magic != IMAGE_DOS_SIGNATURE)
        THROW_AND_LOG( "image '%ls' in not a valid PE file, invalid DOS signature", hMod.name.c_str() );

    _memory->Read( hMod.baseAddress + hdrDos.e_lfanew, sizeof( IMAGE_NT_HEADERS64 ), &hdrNt32 );

    if (phdrNt32->Signature != IMAGE_NT_SIGNATURE)
        THROW_AND_LOG( "image '%ls' in not a valid PE file, invalid NT signature", hMod.name.c_str() );

    if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        expBase = phdrNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    else
        expBase = phdrNt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    // Exports are present
    if (expBase)
    {
        if (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            expSize = phdrNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        else
            expSize = phdrNt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

        expData.reset( reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(malloc( expSize )) );
        IMAGE_EXPORT_DIRECTORY* pExpData = expData.get();

        _memory->Read( hMod.baseAddress + expBase, expSize, pExpData );

        // Fix invalid directory size
        if (expSize <= sizeof( IMAGE_EXPORT_DIRECTORY ))
        {
            // New size should take care of max number of present names (max name length is assumed to be 255 chars)
            auto sectionLimit = max( pExpData->NumberOfFunctions, pExpData->NumberOfNames ) * 255;
            expSize = static_cast<DWORD>(pExpData->AddressOfNameOrdinals - expBase + sectionLimit);

            expData.reset( reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(malloc( expSize )) );
            pExpData = expData.get();

            _memory->Read( hMod.baseAddress + expBase, expSize, pExpData );
        }

        WORD* pAddressOfOrds = reinterpret_cast<WORD*>(
            pExpData->AddressOfNameOrdinals + reinterpret_cast<uintptr_t>(pExpData) - expBase);

        DWORD* pAddressOfNames = reinterpret_cast<DWORD*>(
            pExpData->AddressOfNames + reinterpret_cast<uintptr_t>(pExpData) - expBase);

        DWORD* pAddressOfFuncs = reinterpret_cast<DWORD*>(
            pExpData->AddressOfFunctions + reinterpret_cast<uintptr_t>(pExpData) - expBase);

        for (DWORD i = 0; i < pExpData->NumberOfFunctions; ++i)
        {
            WORD OrdIndex = 0xFFFF;
            char *pName = nullptr;

            // Find by index
            if (reinterpret_cast<uintptr_t>(name_ord) <= 0xFFFF)
            {
                OrdIndex = static_cast<WORD>(i);
            }
            // Find by name
            else if (reinterpret_cast<uintptr_t>(name_ord) > 0xFFFF && i < pExpData->NumberOfNames)
            {
                pName = (char*)(pAddressOfNames[i] + reinterpret_cast<uintptr_t>(pExpData) - expBase);
                OrdIndex = static_cast<WORD>(pAddressOfOrds[i]);
            }
            else
            {
                THROW_AND_LOG(
                    "export '%s' not found in the image '%ls', no more NumberOfNames in the export table",
                    Utils::NameOrdToString( name_ord ).c_str(),
                    hMod.name.c_str()
                );
            }

            if ((reinterpret_cast<uintptr_t>(name_ord) <= 0xFFFF && (WORD)((uintptr_t)name_ord) == (OrdIndex + pExpData->Base)) ||
                (reinterpret_cast<uintptr_t>(name_ord) > 0xFFFF && strcmp( pName, name_ord ) == 0))
            {
                data.procAddress = pAddressOfFuncs[OrdIndex] + hMod.baseAddress;

                // Check forwarded export
                if (data.procAddress >= hMod.baseAddress + expBase &&
                    data.procAddress <= hMod.baseAddress + expBase + expSize)
                {
                    char forwardStr[255] = { };

                    _memory->Read( data.procAddress, sizeof( forwardStr ), forwardStr );

                    std::string chainExp( forwardStr );

                    std::string strDll = chainExp.substr( 0, chainExp.find( "." ) ) + ".dll";
                    std::string strName = chainExp.substr( chainExp.find( "." ) + 1, strName.npos );
                    std::wstring wDll( Utils::AnsiToWstring( strDll ) );

                    // Fill export data info
                    data.isForwarded = true;
                    data.forwardModule = wDll;
                    data.forwardByOrd = (strName.find( "#" ) == 0);

                    if (data.forwardByOrd)
                        data.forwardOrdinal = static_cast<WORD>(atoi( strName.c_str() + 1 ));
                    else
                        data.forwardName = strName;

                    // Check if forward mod is loaded
                    auto mt = (phdrNt32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) ? mt_mod32 : mt_mod64;
                    auto hChainMod = GetModule( wDll, LdrList, mt, baseModule );
                    if (hChainMod == nullptr)
                        THROW_AND_LOG( "forward module '%ls' could not be found", wDll.c_str() );

                    // Import by ordinal
                    if (data.forwardByOrd)
                        return GetExport( hChainMod, reinterpret_cast<const char*>(data.forwardOrdinal), wDll.c_str() );

                    // Import by name
                    return GetExport( hChainMod, strName.c_str(), wDll.c_str() );
                }

                return data;
            }
        }
    }

    THROW_AND_LOG( 
        "export '%s' not found in the image '%ls', no more entries in the export table", 
        Utils::NameOrdToString( name_ord ).c_str(), 
        hMod.name.c_str() 
    );
}

/// <summary>
/// Get export address. Forwarded exports will be automatically resolved if forward module is present
/// </summary>
/// <param name="modName">Module name to search in</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <returns>Export info. If failed procAddress field is 0</returns>
exportData ProcessModules::GetExport( const std::wstring& modName, const char* name_ord )
{
    return GetExport( GetModule( modName ), name_ord );
}

/// <summary>
/// Get export from ntdll
/// </summary>
/// <param name="name_ord">Function name or ordinal</param>
/// <param name="type">Module type. 32 bit or 64 bit</param>
/// <param name="search">Search type.</param>
/// <returns>Export info. If failed procAddress field is 0</returns>
exportData ProcessModules::GetNtdllExport(
    const char* name_ord,
    eModType type /*= mt_default*/,
    eModSeachType search /*= LdrList */
)
{
    auto mod = GetModule( L"ntdll.dll", search, type );
    if (!mod)
        THROW_AND_LOG( "ntdll.dll is not loaded" );

    return GetExport( mod, name_ord );
}

/// <summary>
/// Inject image into target process
/// </summary>
/// <param name="path">Full-qualified image path</param>
/// <param name="pStatus">Injection status code</param>
/// <returns>Module info. nullptr if failed</returns>
ModuleDataPtr ProcessModules::Inject( const std::wstring& path, ThreadPtr pThread /*= nullptr*/ )
{
    ModuleDataPtr mod;
    pe::PEImage img;
    uint32_t ustrSize = 0;
    ptr_t res = 0;

    img.Load( path, true );
    img.Release();

    // Already loaded
    if (mod = GetModule( path, LdrList, img.mType() ); mod)
        return mod;

    // Image path
    auto modName = _memory->Allocate( 0x1000, PAGE_READWRITE );

    // Write dll name into target process
    auto fillDllName = [&modName, &path]( auto& ustr )
    {
        ustr.Buffer = modName.ptr<std::decay_t<decltype(ustr)>::type>() + sizeof( ustr );
        ustr.MaximumLength = ustr.Length = static_cast<USHORT>(path.size() * sizeof( wchar_t ));

        modName.Write( 0, ustr );
        modName.Write( sizeof( ustr ), path.size() * sizeof( wchar_t ), path.c_str() );

        return static_cast<uint32_t>(sizeof( ustr ));
    };

    if (img.mType() == mt_mod32)
    {
        _UNICODE_STRING32 ustr = { };
        ustrSize = fillDllName( ustr );
    }
    else if (img.mType() == mt_mod64)
    {
        _UNICODE_STRING64 ustr = { };
        ustrSize = fillDllName( ustr );
    }
    else
        THROW_AND_LOG( "unsupported image format: %d", img.mType() );

    // Image and process have same processor architecture
    bool sameArch = (img.mType() == mt_mod64 && _core->isWow64() == false) || (img.mType() == mt_mod32 && _core->isWow64() == true);
    auto pLoadLibrary = GetExport( GetModule( L"kernel32.dll", LdrList, img.mType() ), "LoadLibraryW" );

    // Can't inject 32bit dll into native process
    if (!_proc->core().isWow64() && img.mType() == mt_mod32)
        THROW_AND_LOG( "can't inject x86 module into x64 process" );

    auto switchMode = NoSwitch;
    if (_proc->core().isWow64() && img.mType() == mt_mod64)
        switchMode = ForceSwitch;

    auto pLdrLoadDll = GetNtdllExport( "LdrLoadDll", img.mType(), Sections );

    // Patch LdrFindOrMapDll to enable kernel32.dll loading
    if (switchMode == ForceSwitch && !_ldrPatched && IsWindows7OrGreater() && !IsWindows8OrGreater())
    {
        uint8_t patch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
        auto patchBase = g_symbols.LdrKernel32PatchAddress;

        if (patchBase != 0)
        {
            DWORD flOld = 0;
            _memory->Protect( patchBase, sizeof( patch ), PAGE_EXECUTE_READWRITE, &flOld );
            _memory->Write( patchBase, sizeof( patch ), patch );
            _memory->Protect( patchBase, sizeof( patch ), flOld, nullptr );
        }

        _ldrPatched = true;
    }

    auto a = AsmFactory::GetAssembler( img.mType() );

    a->GenCall( pLdrLoadDll.procAddress, { 0, 0, modName.ptr(), modName.ptr() + 0x800 } );
    (*a)->ret();

    _proc->remote().CreateRPCEnvironment( Worker_None, true );

    // Execute call
    NTSTATUS status = STATUS_SUCCESS;
    if (pThread != nullptr)
    {
        if (pThread == _proc->remote().getWorker())
            res = _proc->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() );
        else
            res = _proc->remote().ExecInAnyThread( (*a)->make(), (*a)->getCodeSize(), pThread );

        status = static_cast<NTSTATUS>(res);
    }
    else
        _proc->remote().ExecInNewThread( (*a)->make(), (*a)->getCodeSize(), switchMode );

    // Retry with LoadLibrary if possible
    if (!NT_SUCCESS( status ) && sameArch)
    {
        auto result = _proc->remote().ExecDirect( pLoadLibrary.procAddress, modName.ptr() + ustrSize );
        if (result == 0)
            THROW_WITH_STATUS_AND_LOG( status, "failed to load image via LoadLibrary" );
    }

    return GetModule( path, LdrList, img.mType() );
}

/// <summary>
/// Unload specific module from target process. Can't be used to unload manually mapped modules
/// </summary>
/// <param name="hMod">Module to unload</param>
/// <returns>true on success</returns>
NTSTATUS ProcessModules::Unload( const ModuleDataPtr& hMod )
{
    // Module not present or is manually mapped
    if (hMod->manual || !ValidateModule( hMod->baseAddress ))
        THROW_AND_LOG( "can't unload, invalid module at base 0x%x", hMod->baseAddress );

    // Unload routine
    auto pUnload = GetNtdllExport( "LdrUnloadDll", hMod->type );

    // Special case for unloading 64 bit modules from WOW64 process
    auto threadSwitch = NoSwitch;
    if (_proc->core().isWow64() && hMod->type == mt_mod64)
        threadSwitch = ForceSwitch;

    auto a = AsmFactory::GetAssembler( hMod->type );

    a->GenCall( pUnload.procAddress, { hMod->baseAddress } );
    (*a)->ret();

    _proc->remote().ExecInNewThread( (*a)->make(), (*a)->getCodeSize(), threadSwitch );

    // Remove module from cache
    _modules.erase( std::make_pair( hMod->name, hMod->type ) );
    return true;
}

/// <summary>
/// Unlink module from most loader structures
/// </summary>
/// <param name="mod">Module to unlink</param>
/// <returns>true on success</returns>
bool ProcessModules::Unlink( const ModuleDataPtr& mod )
{
    return _proc->nativeLdr().Unlink( *mod );
}

bool ProcessModules::Unlink( const ModuleData& mod )
{
    return _proc->nativeLdr().Unlink( mod );
}

/// <summary>
/// Ensure module is a valid PE image
/// </summary>
/// <param name="base">Module base address</param>
/// <returns>true on success</returns>
bool ProcessModules::ValidateModule( module_t base )
{
    // Validate memory and headers
    auto idh = _memory->Read<IMAGE_DOS_HEADER>( base );
    if (idh.e_magic == IMAGE_DOS_SIGNATURE)
        return _memory->Read<IMAGE_NT_HEADERS>( base + idh.e_lfanew ).Signature == IMAGE_NT_SIGNATURE;

    return false;
}

/// <summary>
/// Store manually mapped module in module list
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Module info</returns>
ModuleDataPtr ProcessModules::AddManualModule( const ModuleData& mod )
{
    auto canonicalized = Canonicalize( mod, true );
    auto key = std::make_pair( canonicalized.name, canonicalized.type );
    return _modules.emplace( key, std::make_shared<const ModuleData>( canonicalized ) ).first->second;
}

/// <summary>
/// Canonicalize paths and set module type to manual if requested
/// </summary>
/// <param name="mod">Module data</param>
/// <param name="manual">Value to set ModuleData::manual to</param>
/// <returns>Module data</returns>
ModuleData ProcessModules::Canonicalize( const ModuleData& mod, bool manual )
{
    ModuleData result = {};

    result.baseAddress = mod.baseAddress;
    result.ldrPtr = mod.ldrPtr;
    result.size = mod.size;
    result.type = mod.type;
    result.fullPath = Utils::ToLower( mod.fullPath );
    result.name = Utils::ToLower( mod.name );
    result.manual = manual;

    return result;
}

/// <summary>
/// Remove module from module list
/// </summary>
/// <param name="filename">Module file name</param>
/// <param name="mt">Module type. 32 bit or 64 bit</param>
void ProcessModules::RemoveManualModule( const std::wstring& filename, eModType mt )
{
    auto key = std::make_pair( Utils::ToLower( Utils::StripPath( filename ) ), mt );
    if (_modules.count( key ))
        _modules.erase( key );
}

void ProcessModules::UpdateModuleCache( eModSeachType search, eModType type )
{
    for (const auto& mod : _core->native()->EnumModules( search, type ))
        _modules.emplace( std::make_pair( mod->name, mod->type ), mod );
}

using namespace asmjit;
using namespace asmjit::host;

#ifdef COMPILER_MSVC

/// <summary>
/// Inject pure IL image.
/// </summary>
/// <param name="netVersion">NET runtime version to use</param>
/// <param name="netAssemblyPath">Path to image</param>
/// <param name="netAssemblyMethod">Method to call</param>
/// <param name="netAssemblyArgs">Arguments passed into method</param>
/// <param name="returnCode">Return code</param>
void ProcessModules::InjectPureIL(
    const std::wstring& netVersion,
    const std::wstring& netAssemblyPath,
    const std::wstring& netAssemblyMethod,
    const std::wstring& netAssemblyArgs,
    DWORD& returnCode
)
{
    // split netAssemblyMethod string into class and method names
    size_t idx = netAssemblyMethod.find_last_of( '.' );
    if (idx == std::wstring::npos)
        THROW_AND_LOG( "invalid entry point, could not detect class name" );

    std::wstring MethodName = netAssemblyMethod.substr( idx + 1 );
    std::wstring tmp = netAssemblyMethod;
    tmp.erase( idx );
    std::wstring ClassName = tmp;

    MemBlock address;
    address = _memory->Allocate( 0x10000 );

    uintptr_t offset = 4;
    uintptr_t address_VersionString, address_netAssemblyDll, address_netAssemblyClass,
        address_netAssemblyMethod, address_netAssemblyArgs;

    const std::wstring* strArr[] = { &netVersion, &netAssemblyPath, &ClassName, &MethodName, &netAssemblyArgs };

    uintptr_t* ofstArr[] = {
        &address_VersionString, &address_netAssemblyDll, &address_netAssemblyClass,
        &address_netAssemblyMethod, &address_netAssemblyArgs
    };

    // Write strings
    for (size_t i = 0; i < std::size( strArr ); i++)
    {
        size_t len = strArr[i]->length();

        *(ofstArr[i]) = address.ptr<uintptr_t>() + offset;

        // runtime version to use
        address.Write( offset, len * sizeof( wchar_t ) + 2, strArr[i]->c_str() );
        offset = Align( offset + len * sizeof( wchar_t ) + 2, sizeof( uint64_t ) );
    }

    offset += 4;

    GUID GArray[] = { CLSID_CLRMetaHost, IID_ICLRMetaHost, IID_ICLRRuntimeInfo, CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost };

    // COM object GUIDs
    address.Write( offset, sizeof( GArray ), GArray );

    uintptr_t address_CLSID_CLRMetaHost = address.ptr<uintptr_t>() + offset + 0;
    uintptr_t address_IID_ICLRMetaHost = address.ptr<uintptr_t>() + offset + 0x10;
    uintptr_t address_IID_ICLRRuntimeInfo = address.ptr<uintptr_t>() + offset + 0x20;
    uintptr_t address_CLSID_CLRRuntimeHost = address.ptr<uintptr_t>() + offset + 0x30;
    uintptr_t address_IID_ICLRRuntimeHost = address.ptr<uintptr_t>() + offset + 0x40;

    offset += sizeof( GArray );

    std::wstring libName = L"mscoree.dll";

    NameResolve::Instance().ResolvePath( libName, L"", L"", NameResolve::EnsureFullPath, *_proc );

    auto pMscoree = Inject( libName );

    // CLRCreateInstance address
    auto pfnCreateInstance = GetExport( pMscoree, "CLRCreateInstance" );

    // Scary assembler code incoming!
    auto pAsm = AsmFactory::GetAssembler( _proc->core().isWow64() );
    auto& a = *pAsm;
    AsmStackAllocator sa( a.assembler(), 0x30 );   // 0x30 - 6 arguments of ExecuteInDefaultAppDomain

    // Stack will be reserved manually
    a.EnableX64CallStack( false );

    Label L_Exit = a->newLabel();
    Label L_Error1 = a->newLabel();
    Label L_Error2 = a->newLabel();
    Label L_Error3 = a->newLabel();
    Label L_Error4 = a->newLabel();
    Label L_Error5 = a->newLabel();
    Label L_Error6 = a->newLabel();
    Label L_SkipStart = a->newLabel();
    Label L_ReleaseInterface = a->newLabel();

    // stack variables for the injected code
    ALLOC_STACK_VAR( sa, stack_MetaHost, ICLRMetaHost* );
    ALLOC_STACK_VAR( sa, stack_RuntimeInfo, ICLRRuntimeInfo* );
    ALLOC_STACK_VAR( sa, stack_RuntimeHost, ICLRRuntimeHost* );
    ALLOC_STACK_VAR( sa, stack_IsStarted, BOOL );
    ALLOC_STACK_VAR( sa, stack_StartupFlags, DWORD );
    ALLOC_STACK_VAR( sa, stack_returnCode, HRESULT );

#ifdef USE64
    GpReg callReg = r13;

#else
    GpReg callReg = edx;

    a->push( a->zbp );
    a->mov( a->zbp, a->zsp );
#endif

    // function prologue  
    a->sub( a->zsp, Align( sa.getTotalSize(), 0x10 ) + 8 );
    a->xor_( a->zsi, a->zsi );

    // CLRCreateInstance()
    a.GenCall( static_cast<uintptr_t>(pfnCreateInstance.procAddress), { address_CLSID_CLRMetaHost, address_IID_ICLRMetaHost, &stack_MetaHost } );
    // success?
    a->test( a->zax, a->zax );
    a->jnz( L_Error1 );

    // pMetaHost->GetRuntime()
    a->mov( a->zax, stack_MetaHost );
    a->mov( a->zcx, a->intptr_ptr( a->zax ) );
    a->mov( callReg, a->intptr_ptr( a->zcx, 3 * sizeof( void* ) ) );
    a.GenCall( callReg, { a->zcx, address_VersionString, address_IID_ICLRRuntimeInfo, &stack_RuntimeInfo } );
    // success?
    a->test( a->zax, a->zax );
    a->jnz( L_Error2 );

    // pRuntimeInterface->IsStarted()
    a->mov( a->zcx, stack_RuntimeInfo );
    a->mov( a->zax, a->intptr_ptr( a->zcx ) );
    a->mov( callReg, a->intptr_ptr( a->zax, 14 * sizeof( void* ) ) );
    a.GenCall( callReg, { a->zcx, &stack_IsStarted, &stack_StartupFlags } );
    // success?
    a->test( a->zax, a->zax );
    a->jnz( L_Error3 );

    // pRuntimeTime->GetInterface()
    a->mov( a->zcx, stack_RuntimeInfo );
    a->mov( a->zax, a->intptr_ptr( a->zcx ) );
    a->mov( callReg, a->intptr_ptr( a->zax, 9 * sizeof( void* ) ) );
    a.GenCall( callReg, { a->zcx, address_CLSID_CLRRuntimeHost, address_IID_ICLRRuntimeHost, &stack_RuntimeHost } );
    // success?
    a->test( a->zax, a->zax );
    a->jnz( L_Error3 );

    // jump if already started
    a->cmp( stack_IsStarted, a->zsi );
    a->jne( L_SkipStart );

    // pRuntimeHost->Start()
    a->mov( a->zcx, stack_RuntimeHost );
    a->mov( a->zax, a->intptr_ptr( a->zcx ) );
    a->mov( callReg, a->intptr_ptr( a->zax, 3 * sizeof( void* ) ) );
    a.GenCall( callReg, { a->zcx } );
    // success?
    a->test( a->zax, a->zax );
    a->jnz( L_Error5 );

    // pRuntimeHost->ExecuteInDefaultAppDomain()
    a->bind( L_SkipStart );

    a->mov( a->zcx, stack_RuntimeHost );
    a->mov( a->zax, a->intptr_ptr( a->zcx ) );
    a->mov( callReg, a->intptr_ptr( a->zax, 11 * sizeof( void* ) ) );
    a.GenCall( callReg, { a->zcx, address_netAssemblyDll, address_netAssemblyClass, address_netAssemblyMethod,
                          address_netAssemblyArgs, &stack_returnCode } );
    // success?
    a->test( a->zax, a->zax );
    a->jnz( L_Error6 );

    // Release unneeded interfaces
    a->mov( a->zcx, stack_RuntimeHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_RuntimeInfo );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_MetaHost );
    a->call( L_ReleaseInterface );

    // Write the managed code's return value to the first DWORD
    // in the allocated buffer
    a->mov( eax, stack_returnCode );
    a->mov( a->zdx, address.ptr<uintptr_t>() );
    a->mov( dword_ptr( a->zdx ), eax );
    a->mov( a->zax, 0 );

    // stack restoration
    a->bind( L_Exit );

#ifdef USE64
    a->add( a->zsp, Align( sa.getTotalSize(), 0x10 ) + 8 );
#else
    a->mov( a->zsp, a->zbp );
    a->pop( a->zbp );
#endif

    a->ret();

    // CLRCreateInstance() failed
    a->bind( L_Error1 );
    a->mov( a->zax, 1 );
    a->jmp( L_Exit );

    // pMetaHost->GetRuntime() failed
    a->bind( L_Error2 );
    a->mov( a->zcx, stack_MetaHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zax, 2 );
    a->jmp( L_Exit );

    // pRuntimeInterface->IsStarted() failed
    a->bind( L_Error3 );
    a->mov( a->zcx, stack_RuntimeInfo );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_MetaHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zax, 3 );
    a->jmp( L_Exit );

    // pRuntimeTime->GetInterface() failed
    a->bind( L_Error4 );
    a->mov( a->zcx, stack_RuntimeInfo );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_MetaHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zax, 4 );
    a->jmp( L_Exit );

    // pRuntimeHost->Start() failed
    a->bind( L_Error5 );
    a->mov( a->zcx, stack_RuntimeHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_RuntimeInfo );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_MetaHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zax, 5 );
    a->jmp( L_Exit );

    // pRuntimeHost->ExecuteInDefaultAppDomain() failed
    a->bind( L_Error6 );
    a->push( a->zax );
    a->mov( a->zcx, stack_RuntimeHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_RuntimeInfo );
    a->call( L_ReleaseInterface );
    a->mov( a->zcx, stack_MetaHost );
    a->call( L_ReleaseInterface );
    a->mov( a->zax, 6 );
    a->pop( a->zax );
    a->jmp( L_Exit );

    // void __fastcall ReleaseInterface(IUnknown* pInterface)
    a->bind( L_ReleaseInterface );
    a->mov( a->zax, a->zcx );
    a->mov( a->zcx, a->intptr_ptr( a->zax ) );
    a->mov( callReg, a->intptr_ptr( a->zcx, 2 * sizeof( void* ) ) );
    a.GenCall( callReg, { a->zax } );

    a->ret();

    // write JIT code to target
    size_t codeSize = a->getCodeSize();
    ptr_t codeAddress = address.ptr() + offset;

    std::vector<uint8_t> codeBuffer( codeSize );

    a->setBaseAddress( codeAddress );
    a->relocCode( codeBuffer.data() );
    address.Write( offset, codeSize, codeBuffer.data() );

    // run ze codez
    _proc->remote().ExecDirect( (ptr_t)codeAddress, 0 );

    // Get the managed return value
    address.Read( 0, 4, &returnCode );
}

#endif // COMPILER_MSVC


/// <summary>
/// Reset local data
/// </summary>
void ProcessModules::reset()
{
    CSLock lck( _modGuard );

    _modules.clear();
    _ldrPatched = false;
}

}
