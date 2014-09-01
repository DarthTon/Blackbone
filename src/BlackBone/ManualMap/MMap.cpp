#include "MMap.h"
#include "../Process/Process.h"
#include "../Misc/NameResolve.h"
#include "../Misc/Utils.h"
#include "../Misc/DynImport.h"
#include "../Misc/Trace.hpp"
#include "../DriverControl/DriverControl.h"

#include <random>
#include <VersionHelpers.h>

namespace blackbone
{

MMap::MMap( Process& proc )
    : MExcept( proc )
    , _process( proc )
{
}

MMap::~MMap(void)
{
    //UnmapAllModules();
}

/// <summary>
/// Manually map PE image into underlying target process
/// </summary>
/// <param name="path">Image path</param>
/// <param name="flags">Image mapping flags</param>
/// <param name="ldrCallback">Loader callback. Triggers for each mapped module</param>
/// <param name="ldrContext">User-supplied Loader callback context</param>
/// <returns>Mapped image info </returns>
const ModuleData* MMap::MapImage(
    const std::wstring& path,
    eLoadFlags flags /*= NoFlags*/,
    LdrCallback ldrCallback /*= nullptr*/,
    void* ldrContext /*= nullptr*/
    )
{
    return MapImage( path, nullptr, 0, false, flags, ldrCallback, ldrContext );
}

/// <summary>
/// Manually map PE image into underlying target process
/// </summary>
/// <param name="buffer">Image data buffer</param>
/// <param name="size">Buffer size.</param>
/// <param name="asImage">If set to true - buffer has image memory layout</param>
/// <param name="flags">Image mapping flags</param>
/// <param name="ldrCallback">Loader callback. Triggers for each mapped module</param>
/// <param name="ldrContext">User-supplied Loader callback context</param>
/// <returns>Mapped image info</returns>
const ModuleData* MMap::MapImage(
    void* buffer, size_t size,
    bool asImage /*= false*/,
    eLoadFlags flags /*= NoFlags*/,
    LdrCallback ldrCallback /*= nullptr*/,
    void* ldrContext /*= nullptr */
    )
{
    // Create fake path
    wchar_t path[64];
    wsprintfW( path, L"MemoryImage_0x%p", buffer );

    return MapImage( path, buffer, size, asImage, flags, ldrCallback, ldrContext );
}

/// <summary>
/// Manually map PE image into underlying target process
/// </summary>
/// <param name="path">Image path</param>
/// <param name="buffer">Image data buffer</param>
/// <param name="size">Buffer size.</param>
/// <param name="asImage">If set to true - buffer has image memory layout</param>
/// <param name="flags">Image mapping flags</param>
/// <param name="ldrCallback">Loader callback. Triggers for each mapped module</param>
/// <param name="ldrContext">User-supplied Loader callback context</param>
/// <returns>Mapped image info</returns>
const ModuleData* MMap::MapImage(
    const std::wstring& path,
    void* buffer, size_t size,
    bool asImage /*= false*/,
    eLoadFlags flags /*= NoFlags*/,
    LdrCallback ldrCallback /*= nullptr*/,
    void* ldrContext /*= nullptr*/
    )
{
    // Already loaded
    if (auto hMod = _process.modules().GetModule( path ))
        return hMod;

    // Prepare target process
    if (_process.remote().CreateRPCEnvironment() != STATUS_SUCCESS)
    {
        Cleanup();
        return nullptr;
    }

    // No need to support exceptions if DEP is disabled
    if (_process.core().DEP() == false)
        flags |= NoExceptions;

    // Ignore MapInHighMem for native x64 process
    if (!_process.core().isWow64())
        flags &= ~MapInHighMem;

    // Set native loader callback
    _ldrCallback = ldrCallback;
    _ldrContext = ldrContext;

    BLACBONE_TRACE( L"ManualMap: Mapping image '%ls' with flags 0x%x", path.c_str(), flags );

    // Map module and all dependencies
    auto mod = FindOrMapModule( path, buffer, size, asImage, flags );
    if (mod == nullptr)
    {
        Cleanup();
        return nullptr;
    }

    // Change process base module address if needed
    if (flags & RebaseProcess && !_images.empty() && _images.rbegin()->get()->PEImage.IsExe())
    {
        BLACBONE_TRACE( L"ManualMap: Rebasing process to address 0x%p", static_cast<size_t>(mod->baseAddress) );
        _process.memory().Write( _process.core().peb() + 2 * WordSize, WordSize, &mod->baseAddress );
    }

    // Run initializers
    for (auto& img : _images)
    {
        // Init once
        if (!img->initialized)
        {
            if (!RunModuleInitializers( img.get(), DLL_PROCESS_ATTACH ))
                return nullptr;

            // Wipe header
            if (img->flags & WipeHeader)
            {
                if (img->imgMem.Free( img->PEImage.headersSize() ) != STATUS_SUCCESS)
                    img->imgMem.Protect( PAGE_NOACCESS, 0, img->PEImage.headersSize() );
            }

            img->initialized = true;
        }
    }

    Cleanup();

    return mod;
}

/// <summary>
/// Get existing module or map it if absent
/// </summary>
/// <param name="path">Image path</param>
/// <param name="buffer">Image data buffer</param>
/// <param name="size">Buffer size.</param>
/// <param name="asImage">If set to true - buffer has image memory layout</param>
/// <param name="flags">Mapping flags</param>
/// <returns>Module info</returns>
const ModuleData* MMap::FindOrMapModule( const std::wstring& path,
                                         void* buffer, size_t size, bool asImage,
                                         eLoadFlags flags /*= NoFlags*/ )
{
    std::unique_ptr<ImageContext> pImage( new ImageContext() );

    pImage->FilePath = path;
    pImage->FileName = Utils::StripPath( pImage->FilePath );
    pImage->flags = flags;

    // Load and parse image
    if (buffer)
        pImage->FileImage.Assign( buffer, size, !asImage );
    else
        pImage->FileImage.Project( path );

    if (!pImage->PEImage.Parse( pImage->FileImage.base(), pImage->FileImage.isPlainData() ))
        return nullptr;

    // Check if already loaded
    if (auto hMod = _process.modules().GetModule( path, LdrList, pImage->PEImage.mType() ))
        return hMod;

    BLACBONE_TRACE( L"ManualMap: Loading new image '%ls'", path.c_str() );

    // Try to map image in high (>4GB) memory range
    if (flags & MapInHighMem)
    {
        AllocateInHighMem( pImage->imgMem, pImage->PEImage.imageSize() );
    }
    // Try to map image at it's original ASRL-aware base
    else if (flags & UnlinkVAD)
    {      
        ptr_t base = pImage->PEImage.imageBase();
        ptr_t size = pImage->PEImage.imageSize();

        // Allocate as physical
        Driver().EnsureLoaded();
        auto status = Driver().AllocateMem( _process.pid(), base, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE, true );

        // Allocate at any base
        if (!NT_SUCCESS( status ))
        {
            size = pImage->PEImage.imageSize();
            status = Driver().AllocateMem( _process.pid(), base, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE, true );
        }

        // Store allocated region
        if (NT_SUCCESS( status ))
        {
            pImage->imgMem = MemBlock( &_process.memory(), base, static_cast<size_t>(size), PAGE_EXECUTE_READWRITE, true, true );
        }
        // Remove flag if failed
        else
        {
            BLACBONE_TRACE( L"ManualMap: Failed to allocate physical memory for image, status 0x%d", status );
            flags &= ~UnlinkVAD;
        }
    }

    // Allocate normally if something went wrong
    if (pImage->imgMem == 0)
        pImage->imgMem = _process.memory().Allocate( pImage->PEImage.imageSize(), PAGE_EXECUTE_READWRITE, pImage->PEImage.imageBase() );

    BLACBONE_TRACE( L"ManualMap: Image base allocated at 0x%p", pImage->imgMem.ptr<size_t>() );

    if (!pImage->imgMem.valid())
        return nullptr;

    // Create Activation context for SxS
    // .exe files usually contain manifest under id of 1
    // .dll files have manifest under id of 2
    if (pImage->FileImage.manifestID() == 0)
        flags |= NoSxS;

    if (!(flags & NoSxS))
        CreateActx( path, pImage->FileImage.manifestID() );

    // Core image mapping operations
    if (!CopyImage( pImage.get() ) || !RelocateImage( pImage.get() ))
        return nullptr;

    auto mt = pImage->PEImage.mType();
    auto pMod = _process.modules().AddManualModule( pImage->FilePath, pImage->imgMem.ptr<module_t>(), pImage->imgMem.size(), mt );

    // Import tables
    if (!ResolveImport( pImage.get() ) || (!(flags & NoDelayLoad) && !ResolveImport( pImage.get(), true )))
    {
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }

    // Apply proper memory protection for sections
    ProtectImageMemory( pImage.get() );

    // Make exception handling possible (C and C++)
    if (!(flags & NoExceptions) && !EnableExceptions( pImage.get() ))
    {
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }

    // Unlink image from VAD list
    if (flags & UnlinkVAD)
        HideVad( pImage->imgMem );

    // Initialize security cookie
    if (!InitializeCookie( pImage.get() ))
    {
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }

    // Get entry point
    pImage->EntryPoint = pImage->PEImage.entryPoint( pImage->imgMem.ptr<ptr_t>() );

    // Create reference for native loader functions
    LdrRefFlags ldrFlags = flags & CreateLdrRef ? Ldr_All: Ldr_None;
    if (_ldrCallback != nullptr)
        ldrFlags = _ldrCallback( _ldrContext, *pMod );

    if (ldrFlags != Ldr_None)
        _process.nativeLdr().CreateNTReference( pImage->imgMem.ptr<HMODULE>(), 
                                                pImage->PEImage.imageSize(), 
                                                pImage->FilePath, 
                                                static_cast<size_t>(pImage->EntryPoint),
                                                ldrFlags );

    // Static TLS data
    if (!(flags & NoTLS) &&! InitStaticTLS( pImage.get( ) ))
    {
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }
    
    // Fill TLS callbacks
    pImage->PEImage.GetTLSCallbacks( pImage->imgMem.ptr<ptr_t>(), pImage->tlsCallbacks );

    // Unload local copy
    pImage->FileImage.Release();

    // Release image memory block
    pImage->imgMem.Release();

    // Store image
    _images.emplace_back( std::move( pImage ) );
    return pMod;
}

/// <summary>
/// Map pure IL image
/// Not supported yet
/// </summary>
/// <returns>Image base</returns>
module_t MMap::MapPureManaged( )
{
    /*if(!netImg.Init(_pTopImage->FilePath))
    {
        SetLastError(0x1337);
        return 0;
    }*/

    //netImg.Parse();

    LastNtStatus( STATUS_NOT_IMPLEMENTED );
    return 0;
}

/// <summary>
/// Unmap all manually mapped modules
/// </summary>
/// <returns>true on success</returns>
bool MMap::UnmapAllModules()
{
    for (auto img = _images.rbegin(); img != _images.rend(); ++img)
    {
        auto pImage = img->get();
        BLACBONE_TRACE( L"ManualMap: Unmapping image '%ls'", pImage->FileName.c_str() );

        // Call main
        RunModuleInitializers( pImage, DLL_PROCESS_DETACH );

        // Remove VEH
        if (!(pImage->flags & NoExceptions))
            DisableExceptions( pImage );

        // Remove from loader
        auto mod = _process.modules().GetModule( pImage->FileName );
        _process.modules().Unlink( mod );

        // Free memory
        pImage->imgMem.Free();

        // Remove reference from local modules list
        _process.modules().RemoveManualModule( pImage->FilePath, pImage->PEImage.mType() );
    } 

    Cleanup();
    reset();

    return true;
}

/// <summary>
/// Copies image into target process
/// </summary>
/// <param name="pImage">Image data</param>
/// <returns>true on success</returns>
bool MMap::CopyImage( ImageContext* pImage )
{
    BLACBONE_TRACE( L"ManualMap: Performing image copy" );

    // offset to first section equals to header size
    size_t dwHeaderSize = pImage->PEImage.headersSize();

    // Copy header
    if (pImage->imgMem.Write( 0, dwHeaderSize, pImage->FileImage.base() ) != STATUS_SUCCESS)
    {
        BLACBONE_TRACE( L"ManualMap: Failed to copy image headers. Status = 0x%x", LastNtStatus() );
        return false;
    }

    // Set header protection
    if (pImage->imgMem.Protect( PAGE_READONLY, 0, dwHeaderSize ) != STATUS_SUCCESS)
    {
        BLACBONE_TRACE( L"ManualMap: Failed to set header memory protection. Status = 0x%x", LastNtStatus() );
        return false;
    }

    auto& sections = pImage->PEImage.sections();

    // Copy sections
    for( auto& section : sections)
    {
        // Skip discardable sections
        if (section.Characteristics & (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE) &&
             !(section.Characteristics & IMAGE_SCN_MEM_DISCARDABLE))
        {
            uint8_t* pSource = reinterpret_cast<uint8_t*>(pImage->PEImage.ResolveRVAToVA( section.VirtualAddress ));

            if (section.SizeOfRawData != 0 && 
                 pImage->imgMem.Write( section.VirtualAddress, section.SizeOfRawData, pSource ) != STATUS_SUCCESS)
            {
                BLACBONE_TRACE( L"ManualMap: Failed to copy image section at offset 0x%x. Status = 0x%x", 
                                section.VirtualAddress, LastNtStatus() );
                return false;
            }
        } 
    }

    return true;
}

/// <summary>
/// Adjust image memory protection
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::ProtectImageMemory( ImageContext* pImage )
{
    // Set section memory protection
    for (auto& section : pImage->PEImage.sections())
    {
        auto prot = GetSectionProt( section.Characteristics );
        if (prot != PAGE_NOACCESS)
        {
            if (pImage->imgMem.Protect( prot, section.VirtualAddress, section.Misc.VirtualSize ) != STATUS_SUCCESS)
            {
                BLACBONE_TRACE( L"ManualMap: Failed to set section memory protection at offset 0x%x. Status = 0x%x", 
                                section.VirtualAddress, LastNtStatus() );
                return false;
            }
        }
        // Decommit pages with NO_ACCESS protection
        else
        {
            _process.memory().Free( pImage->imgMem.ptr() + section.VirtualAddress, section.Misc.VirtualSize, MEM_DECOMMIT );
        }
    }

    return true;
}

/// <summary>
///  Fix relocations if image wasn't loaded at base address
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::RelocateImage( ImageContext* pImage )
{
    BLACBONE_TRACE( L"ManualMap: Relocating image '%ls'", pImage->FilePath.c_str() );

    // Reloc delta
    size_t Delta = pImage->imgMem.ptr<size_t>() - static_cast<size_t>(pImage->PEImage.imageBase());

    // No need to relocate
    if (Delta == 0)
    {
        BLACBONE_TRACE( L"ManualMap: No need for relocation" );
        LastNtStatus( STATUS_SUCCESS );
        return true;
    }

    auto start = pImage->PEImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_BASERELOC );
    auto end = start + pImage->PEImage.DirectorySize( IMAGE_DIRECTORY_ENTRY_BASERELOC );

    pe::RelocData* fixrec = reinterpret_cast<pe::RelocData*>(start);
    if (fixrec == nullptr)
    {
        // TODO: return proper error code
        BLACBONE_TRACE( L"ManualMap: Can't relocate image, no relocation data" );
        LastNtStatus( STATUS_IMAGE_NOT_AT_BASE );
        return false;
    }

    while ((size_t)fixrec < end && fixrec->BlockSize)
    {
        DWORD count = (fixrec->BlockSize - 8) >> 1;             // records count

        for (DWORD i = 0; i < count; ++i)
        {
            WORD fixtype = (fixrec->Item[i].Type);              // fixup type
            WORD fixoffset = (fixrec->Item[i].Offset) % 4096;   // offset in 4K block

            // no fixup required
            if (fixtype == IMAGE_REL_BASED_ABSOLUTE)
                continue;

            // add delta 
            if (fixtype == IMAGE_REL_BASED_HIGHLOW || fixtype == IMAGE_REL_BASED_DIR64)
            {
                size_t fixRVA = static_cast<ULONG>(fixoffset) + fixrec->PageRVA;
                size_t val = *reinterpret_cast<size_t*>(pImage->PEImage.ResolveRVAToVA( fixoffset + fixrec->PageRVA )) + Delta;

                // Apply relocation
                if (pImage->imgMem.Write( fixRVA, val ) != STATUS_SUCCESS)
                {
                    BLACBONE_TRACE( L"ManualMap: Failed to apply relocation at offset 0x%x. Status = 0x%x", fixRVA, LastNtStatus() );
                    return false;
                }
            }
            else
            {
                // TODO: support for all remaining relocations
                BLACBONE_TRACE( L"ManualMap: Abnormal relocation type %d. Aborting", fixtype );
                LastNtStatus( STATUS_INVALID_IMAGE_FORMAT );
                return false;
            }
        }

        // next reloc entry
        fixrec = reinterpret_cast<pe::RelocData*>(reinterpret_cast<size_t>(fixrec) + fixrec->BlockSize);
    }

    return true;
}

/// <summary>
/// Return existing or load missing dependency
/// </summary>
/// <param name="pImage">Currently napped image data</param>
/// <param name="path">Dependency path</param>
/// <returns></returns>
const ModuleData* MMap::FindOrMapDependency( ImageContext* pImage, std::wstring& path )
{
    // For win32 one exception handler is enough
    // For amd64 each image must have it's own handler to resolve C++ exceptions properly
#ifdef USE64
    eLoadFlags newFlags = static_cast<eLoadFlags>(pImage->flags | NoSxS | NoDelayLoad);
#else
    eLoadFlags newFlags = static_cast<eLoadFlags>(pImage->flags | NoSxS | NoDelayLoad | PartialExcept);
#endif

    // Already loaded
    auto hMod = _process.modules().GetModule( path, LdrList, pImage->PEImage.mType(), pImage->FileName.c_str() );
    if (hMod)
        return hMod;

    BLACBONE_TRACE( L"ManualMap: Loading new dependency '%ls'", path.c_str() );

    NameResolve::Instance().ResolvePath( path, pImage->FileName,
                                         Utils::GetParent( pImage->FilePath ),
                                         NameResolve::EnsureFullPath,
                                         _process.pid(),
                                         pImage->FileImage.actx() );

    BLACBONE_TRACE( L"ManualMap: Dependency path resolved to '%ls'", path.c_str() );

    // Loading method
    if (pImage->flags & ManualImports)
        return FindOrMapModule( path, nullptr, 0, false, newFlags );
    else
        return _process.modules().Inject( path );
};

/// <summary>
/// Resolves image import or delayed image import
/// </summary>
/// <param name="pImage">Image data</param>
/// <param name="useDelayed">Resolve delayed import instead</param>
/// <returns>true on success</returns>
bool MMap::ResolveImport( ImageContext* pImage, bool useDelayed /*= false */ )
{
    auto imports = pImage->PEImage.ProcessImports( useDelayed );
    if (imports.empty())
        return true;

    // Traverse entries
    for (auto& importMod : imports)
    {
        std::wstring wstrDll = importMod.first;

        // Load dependency if needed
        auto hMod = FindOrMapDependency( pImage, wstrDll );
        if (!hMod)
        {
            // TODO: Add error code
            BLACBONE_TRACE( L"ManualMap: Failed to load dependency '%ls'. Status = 0x%x", wstrDll.c_str(), LastNtStatus() );
            return false;
        }

        for (auto& importFn : importMod.second)
        {
            exportData expData;

            if (importFn.importByOrd)
                expData = _process.modules().GetExport( hMod, reinterpret_cast<const char*>(importFn.importOrdinal) );
            else
                expData = _process.modules().GetExport( hMod, importFn.importName.c_str() );

            // Still forwarded, load missing modules
            while (expData.procAddress && expData.isForwarded)
            {
                std::wstring wdllpath = expData.forwardModule;

                // Ensure module is loaded
                auto hFwdMod = FindOrMapDependency( pImage, wdllpath );
                if (!hFwdMod)
                {
                    // TODO: Add error code
                    BLACBONE_TRACE( L"ManualMap: Failed to load forwarded dependency '%ls'. Status = 0x%x", wstrDll.c_str(), LastNtStatus() );
                    return false;
                }

                if (expData.forwardByOrd)
                    expData = _process.modules().GetExport( hFwdMod, reinterpret_cast<const char*>(expData.forwardOrdinal), wdllpath.c_str() );
                else
                    expData = _process.modules().GetExport( hFwdMod, expData.forwardName.c_str(), wdllpath.c_str() );
            }

            // Failed to resolve import
            if (expData.procAddress == 0)
            {
                // TODO: Add error code
                if (importFn.importByOrd)
                    BLACBONE_TRACE( L"ManualMap: Failed to get import #%d from image '%ls'. Status = 0x%x", 
                                    importFn.importOrdinal, wstrDll.c_str(), LastNtStatus() );
                else
                    BLACBONE_TRACE( L"ManualMap: Failed to get import '%s' from image '%ls'. Status = 0x%x",
                                    importFn.importName.c_str(), wstrDll.c_str(), LastNtStatus() );
                return false;
            }

            // Write function address
            if (pImage->imgMem.Write( importFn.ptrRVA, static_cast<size_t>(expData.procAddress) ) != STATUS_SUCCESS)
            {
                BLACBONE_TRACE( L"ManualMap: Failed to write import function address at offset 0x%x. Status = 0x%x",
                                importFn.ptrRVA, LastNtStatus() );
                return false;
            }
        }
    }

    return true;
}

/// <summary>
/// Set custom exception handler to bypass SafeSEH under DEP 
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::EnableExceptions( ImageContext* pImage )
{
    BLACBONE_TRACE( L"ManualMap: Enabling exception support for image '%ls'", pImage->FileName.c_str() );
#ifdef USE64
    size_t size = pImage->PEImage.DirectorySize( IMAGE_DIRECTORY_ENTRY_EXCEPTION );
    IMAGE_RUNTIME_FUNCTION_ENTRY *pExpTable = 
        reinterpret_cast<decltype(pExpTable)>(pImage->PEImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_EXCEPTION ));

    // Invoke RtlAddFunctionTable
    if(pExpTable)
    {     
        AsmJitHelper a;
        uint64_t result = 0;

        pImage->pExpTableAddr = REBASE( pExpTable, pImage->FileImage.base(), pImage->imgMem.ptr<ptr_t>() );
        auto pAddTable = _process.modules().GetExport( _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->PEImage.mType() ),
                                                       "RtlAddFunctionTable" );

        a.GenPrologue();
        a.GenCall( static_cast<size_t>(pAddTable.procAddress), { pImage->pExpTableAddr, 
                                                                  size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY),
                                                                  pImage->imgMem.ptr<size_t>() } );

        _process.remote().AddReturnWithEvent( a, pImage->PEImage.mType() );
        a.GenEpilogue();

        if (_process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result ) != STATUS_SUCCESS)
            return false;

        if (pImage->flags & CreateLdrRef)
            return true;
        else
            return (MExcept::CreateVEH( pImage->imgMem.ptr<size_t>(), pImage->PEImage.imageSize(), pImage->PEImage.mType() ) == STATUS_SUCCESS);
    }
    else
        return false;
#else
    bool safeseh = false;
    _process.nativeLdr().InsertInvertedFunctionTable( pImage->imgMem.ptr<void*>(), pImage->PEImage.imageSize(), safeseh );

    if ((pImage->flags & PartialExcept) || safeseh)
        return true;
    else
        return (MExcept::CreateVEH( pImage->imgMem.ptr<size_t>(), pImage->PEImage.imageSize(), pImage->PEImage.mType() ) == STATUS_SUCCESS);
#endif

}

/// <summary>
/// Remove custom exception handler
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::DisableExceptions( ImageContext* pImage )
{
    BLACBONE_TRACE( L"ManualMap: Disabling exception support for image '%ls'", pImage->FileName.c_str() );
#ifdef USE64
    if(pImage->pExpTableAddr)
    {      
        AsmJitHelper a;
        size_t result = 0;

        a.GenPrologue();

        auto pRmoveTable = _process.modules().GetExport( _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->PEImage.mType() ),
                                                         "RtlDeleteFunctionTable" );

        // RtlDeleteFunctionTable(pExpTable);
        a.GenCall( static_cast<size_t>(pRmoveTable.procAddress), { pImage->pExpTableAddr } );
        _process.remote().AddReturnWithEvent( a );
        a.GenEpilogue();

        if (_process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result ) != STATUS_SUCCESS)
            return false;

        if (pImage->flags & CreateLdrRef)
            return true;
        else
            return (MExcept::RemoveVEH() == STATUS_SUCCESS);
    }
    else
        return false;
#else
    if (pImage->flags & (PartialExcept | NoExceptions))
        return true;
    else
        return (MExcept::RemoveVEH() == STATUS_SUCCESS);

#endif
}

/// <summary>
/// Resolve static TLS storage
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::InitStaticTLS( ImageContext* pImage )
{
    IMAGE_TLS_DIRECTORY *pTls = reinterpret_cast<decltype(pTls)>(pImage->PEImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_TLS ));
    auto pRebasedTls = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(REBASE( pTls, pImage->FileImage.base(), pImage->imgMem.ptr<ptr_t>() ));

    // Use native TLS initialization
    if (pTls && pTls->AddressOfIndex)
    {
        BLACBONE_TRACE( L"ManualMap: Performing statatic TLS initialization for image '%ls'", 
                        pImage->FileName.c_str() );

        _process.nativeLdr().AddStaticTLSEntry( pImage->imgMem.ptr<void*>(), pRebasedTls );
    }

    return true;
}

/// <summary>
/// Calculate and set security cookie
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::InitializeCookie( ImageContext* pImage )
{
    IMAGE_LOAD_CONFIG_DIRECTORY *pLC = reinterpret_cast<decltype(pLC)>(pImage->PEImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ));

    //
    // Cookie generation based on MSVC++ compiler
    //
    if (pLC && pLC->SecurityCookie)
    {
        BLACBONE_TRACE( L"ManualMap: Performing security cookie initializtion for image '%ls'", pImage->FileName.c_str() );

        FILETIME systime = { 0 };
        LARGE_INTEGER PerformanceCount = { 0 };
        uintptr_t cookie = 0;

        GetSystemTimeAsFileTime( &systime );
        QueryPerformanceCounter( &PerformanceCount );

        cookie = _process.pid() ^ _process.remote().getWorker()->id() ^ reinterpret_cast<uintptr_t>(&cookie);

    #ifdef USE64
        cookie ^= *reinterpret_cast<uint64_t*>(&systime);
        cookie ^= (PerformanceCount.QuadPart << 32) ^ PerformanceCount.QuadPart;
        cookie &= 0xFFFFFFFFFFFF;

        if (cookie == 0x2B992DDFA232)
            cookie++;
    #else

        cookie ^= systime.dwHighDateTime ^ systime.dwLowDateTime;
        cookie ^= PerformanceCount.LowPart;
        cookie ^= PerformanceCount.HighPart;

        if (cookie == 0xBB40E64E)
            cookie++;
        else if (!(cookie & 0xFFFF0000))
            cookie |= (cookie | 0x4711) << 16;
    #endif

        _process.memory().Write( REBASE( pLC->SecurityCookie, pImage->PEImage.imageBase(), pImage->imgMem.ptr<ptr_t>() ), cookie );
    }

    return true;
}

/// <summary>
/// Run module initializers(TLS and entry point).
/// </summary>
/// <param name="pImage">Image data</param>
/// <param name="dwReason">one of the following:
/// DLL_PROCESS_ATTACH
/// DLL_THREAD_ATTACH
/// DLL_PROCESS_DETACH
/// DLL_THREAD_DETTACH
/// </param>
/// <returns>true on success</returns>
bool MMap::RunModuleInitializers( ImageContext* pImage, DWORD dwReason )
{
    AsmJitHelper a;
    uint64_t result = 0;

    auto hNtdll = _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->PEImage.mType() );
    auto pActivateActx = _process.modules().GetExport( hNtdll, "RtlActivateActivationContext" );
    auto pDeactivateeActx = _process.modules().GetExport( hNtdll, "RtlDeactivateActivationContext" );

    a.GenPrologue();

    // ActivateActCtx
    if (_pAContext.valid())
    {
        a->mov( asmjit::host::zax, _pAContext.ptr<size_t>() );
        a->mov( asmjit::host::zax, asmjit::host::dword_ptr( asmjit::host::zax ) );
        a.GenCall( static_cast<size_t>(pActivateActx.procAddress), { 0, asmjit::host::zax, _pAContext.ptr<size_t>() + sizeof(HANDLE) } );
    }

    // Function order
    // TLS first, entry point last
    if (dwReason == DLL_PROCESS_ATTACH || dwReason == DLL_THREAD_ATTACH)
    {
        // PTLS_CALLBACK_FUNCTION(pImage->ImageBase, dwReason, NULL);
        if (!(pImage->flags & NoTLS))
            for (auto& pCallback : pImage->tlsCallbacks)
            {
                BLACBONE_TRACE( L"ManualMap: Calling TLS callback at 0x%p for '%ls', Reason: %d", 
                                static_cast<size_t>(pCallback), pImage->FileName.c_str(), dwReason );

                a.GenCall( static_cast<size_t>(pCallback), { pImage->imgMem.ptr<size_t>(), dwReason, NULL } );
            }

        // DllMain
        if (pImage->EntryPoint != 0)
        {
            BLACBONE_TRACE( L"ManualMap: Calling entry point for '%ls', Reason: %d", pImage->FileName.c_str(), dwReason );
            a.GenCall( static_cast<size_t>(pImage->EntryPoint), { pImage->imgMem.ptr<size_t>(), dwReason, NULL } );
        }
    }
    // Entry point first, TLS last
    else
    {
        // DllMain
        if (pImage->EntryPoint != 0)
        {
            BLACBONE_TRACE( L"ManualMap: Calling entry point for '%ls', Reason: %d", pImage->FileName.c_str(), dwReason );
            a.GenCall( static_cast<size_t>(pImage->EntryPoint), { pImage->imgMem.ptr<size_t>(), dwReason, NULL } );
        }

        // PTLS_CALLBACK_FUNCTION(pImage->ImageBase, dwReason, NULL);
        if (!(pImage->flags & NoTLS))
            for (auto& pCallback : pImage->tlsCallbacks)
            {
                BLACBONE_TRACE( L"ManualMap: Calling TLS callback at 0x%p for '%ls', Reason: %d", 
                                static_cast<size_t>(pCallback), pImage->FileName.c_str(), dwReason );

                a.GenCall( static_cast<size_t>(pCallback), { pImage->imgMem.ptr<size_t>(), dwReason, NULL } );
            }
    }

    // DeactivateActCtx
    if (_pAContext.valid())
    {
        a->mov( asmjit::host::zax, _pAContext.ptr<size_t>() + sizeof(HANDLE) );
        a->mov( asmjit::host::zax, asmjit::host::dword_ptr( asmjit::host::zax ) );
        a.GenCall( static_cast<size_t>(pDeactivateeActx.procAddress), { 0, asmjit::host::zax } );
    }

    _process.remote().AddReturnWithEvent( a, pImage->PEImage.mType() );
    a.GenEpilogue();

    _process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result );

    return true;
}


/// <summary>
/// Create activation context
/// Target memory layout:
/// -----------------------------
/// | hCtx | ACTCTX | file_path |
/// -----------------------------
/// </summary>
/// <param name="path">Image file path.</param>
/// <param name="id">Manifest resource id</param>
/// <returns>true on success</returns>
bool MMap::CreateActx( const std::wstring& path, int id /*= 2 */ )
{   
    AsmJitHelper a;

    uint64_t result = 0;
    ACTCTXW act = { 0 };

    _pAContext = _process.memory().Allocate( 512, PAGE_READWRITE );
    
    act.cbSize = sizeof(act);
    act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    act.lpSource = reinterpret_cast<LPCWSTR>(_pAContext.ptr<size_t>() + sizeof(HANDLE) + sizeof(act));
    act.lpResourceName = MAKEINTRESOURCEW( id );

    bool switchMode = (_process.core().native()->GetWow64Barrier().type == wow_64_32);
    auto pCreateActx = _process.modules().GetExport( _process.modules().GetModule( L"kernel32.dll" ), "CreateActCtxW" );
    if (pCreateActx.procAddress == 0)
    {
        BLACBONE_TRACE( L"ManualMap: Failed to create activation context for image '%ls'. 'CreateActCtxW' is absent", path.c_str() );
        return false;
    }

    // CreateActCtx(&act)
    // Emulate Wow64
    if (switchMode)
    {
        _ACTCTXW_T<DWORD> act32 = { 0 };

        act32.cbSize = sizeof(act32);
        act32.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act32.lpSource = _pAContext.ptr<uint32_t>() + sizeof(HANDLE) + sizeof(act32);
        act32.lpResourceName = id ;

        a->push( _pAContext.ptr<uint32_t>() + static_cast<uint32_t>(sizeof( HANDLE )) );
        a->mov( asmjit::host::eax, static_cast<uint32_t>(pCreateActx.procAddress) );
        a->call( asmjit::host::zax );
        a->mov( asmjit::host::edx, _pAContext.ptr<uint32_t>() );
        //a->mov( asmjit::host::dword_ptr( asmjit::host::edx ), asmjit::host::eax );
        a->dw( '\x01\x02' );

        auto pTermThd = _process.modules().GetExport( _process.modules().GetModule( L"ntdll.dll" ), "NtTerminateThread" );
        a->push( asmjit::host::zax );
        a->push( uint32_t( 0 ) );
        a->mov( asmjit::host::eax, static_cast<uint32_t>(pTermThd.procAddress) );
        a->call( asmjit::host::zax );
        a->ret( 4 );
        
        // Write path to file
        _pAContext.Write( sizeof(HANDLE), act32 );
        _pAContext.Write( sizeof(HANDLE) + sizeof(act32), (path.length() + 1) * sizeof(wchar_t), path.c_str() );

        auto pCode = _process.memory().Allocate( 0x1000 );
        pCode.Write( 0, a->getCodeSize(), a->make() );

        result = _process.remote().ExecDirect( pCode.ptr<ptr_t>(), _pAContext.ptr<size_t>() + sizeof(HANDLE) );
    }
    // Native way
    else
    {
        a.GenPrologue();

        a.GenCall( static_cast<size_t>(pCreateActx.procAddress), { _pAContext.ptr<size_t>() + sizeof(HANDLE) } );

        a->mov( asmjit::host::zdx, _pAContext.ptr<size_t>() );
        a->mov( asmjit::host::intptr_ptr( asmjit::host::zdx ), asmjit::host::zax );

        _process.remote().AddReturnWithEvent( a );
        a.GenEpilogue();

        // Write path to file
        _pAContext.Write( sizeof(HANDLE), act );
        _pAContext.Write( sizeof(HANDLE) + sizeof(act), (path.length() + 1) * sizeof(wchar_t), path.c_str() );

        _process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result );
    }


    if (reinterpret_cast<HANDLE>(result) == INVALID_HANDLE_VALUE)
    {
        _pAContext.Free();

        // SetLastError( err::mapping::CantCreateActx );
        BLACBONE_TRACE( L"ManualMap: Failed to create activation context for image '%ls'. Status: 0x%x", 
                        path.c_str(), _process.remote().GetLastStatus() );
        return false;
    }

    return true;
}

/// <summary>
/// Unlink memory VAD node
/// </summary>
/// <param name="imageMem">Image to purge</param>
/// <returns>bool on success</returns>
NTSTATUS MMap::HideVad( const MemBlock& imageMem)
{
    NTSTATUS status = Driver().EnsureLoaded();

    if (NT_SUCCESS( status ))
        status = Driver().HideVAD( _process.pid(), imageMem.ptr(), static_cast<uint32_t>(imageMem.size()) );

    return status;
}

/// <summary>
/// Allocates memory region beyond 4GB limit
/// </summary>
/// <param name="imageMem">Image data</param>
/// <param name="size">Block size</param>
/// <returns>Status code</returns>
NTSTATUS MMap::AllocateInHighMem( MemBlock& imageMem, size_t size )
{
    NTSTATUS status = Driver().EnsureLoaded();

    if (NT_SUCCESS( status ))
    {
        ptr_t ptr = 0;

        // Align on page boundary
        size = Align( size, 0x1000 );

        //
        // Get random address 
        //
        bool found = true;
        static std::random_device rd;
        std::uniform_int_distribution<ptr_t> dist( 0x100000, 0x7FFFFFFF );

        // Make sure address is unused
        for (ptr = dist( rd ) * 0x1000; found; ptr = dist( rd ) * 0x1000)
        {
            found = false;

            for (auto& entry : _usedBlocks)
                if (ptr >= entry.first && ptr < entry.first + entry.second)
                {
                    found = true;
                    break;
                }
        }

        // Not implemented yet
        status = STATUS_NOT_IMPLEMENTED;

        // Change protection and save address
        if (NT_SUCCESS( status ))
        {
            _usedBlocks.emplace_back( std::make_pair( ptr, size ) );

            imageMem = MemBlock( &_process.memory(), ptr, size, PAGE_READWRITE, false );
            _process.memory( ).Protect( ptr, size, PAGE_READWRITE );
            return true;
        }
    }

    return status;
}

/// <summary>
/// Remove any traces from remote process
/// </summary>
/// <returns></returns>
void MMap::Cleanup()
{
    _pAContext.Reset();
    _process.remote().reset();
}


/// <summary>
/// Gets VadPurge handle.
/// </summary>
/// <returns>Driver object handle, INVALID_HANDLE_VALUE if failed</returns>
HANDLE MMap::GetDriverHandle()
{
    std::wstring drvPath = Utils::GetExeDirectory() + L"\\";

    if (IsWindows8Point1OrGreater())
        drvPath += L"BlackBoneDrv81.sys";
    else if (IsWindows8OrGreater())
        drvPath += L"BlackBoneDrv8.sys";
    else if (IsWindows7OrGreater())
        drvPath += L"BlackBoneDrv7.sys";

    //NTSTATUS status = DriverControl::Instance().EnsureLoaded( drvPath );

    return INVALID_HANDLE_VALUE;
}


/// <summary>
/// Transform section characteristics into memory protection flags
/// </summary>
/// <param name="characteristics">Section characteristics</param>
/// <returns>Memory protection value</returns>
DWORD MMap::GetSectionProt( DWORD characteristics )
{
    DWORD dwResult = PAGE_NOACCESS;

    if (characteristics & IMAGE_SCN_MEM_DISCARDABLE)
    {
        dwResult = PAGE_NOACCESS;
    }
    else if(characteristics & IMAGE_SCN_MEM_EXECUTE) 
    {
        if(characteristics & IMAGE_SCN_MEM_WRITE)
            dwResult = PAGE_EXECUTE_READWRITE;
        else if(characteristics & IMAGE_SCN_MEM_READ)
            dwResult = PAGE_EXECUTE_READ;
        else
            dwResult = PAGE_EXECUTE;
    } 
    else
    {
        if(characteristics & IMAGE_SCN_MEM_WRITE)
            dwResult = PAGE_READWRITE;
        else if(characteristics & IMAGE_SCN_MEM_READ)
            dwResult = PAGE_READONLY;
        else
            dwResult = PAGE_NOACCESS;
    }

    return dwResult;
}

}
