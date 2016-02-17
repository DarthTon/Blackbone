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
/// <param name="mapCallback">Mapping callback. Triggers for each mapped module</param>
/// <param name="context">User-supplied callback context</param>
/// <returns>Mapped image info </returns>
const ModuleData* MMap::MapImage(
    const std::wstring& path,
    eLoadFlags flags /*= NoFlags*/,
    MapCallback mapCallback /*= nullptr*/,
    void* context /*= nullptr*/,
    CustomArgs_t* pCustomArgs /*= nullptr*/
    )
{
    return MapImageInternal( path, nullptr, 0, false, flags, mapCallback, context, pCustomArgs );
}

/// <summary>
///Manually map PE image into underlying target process
/// </summary>
/// <param name="buffer">Image data buffer</param>
/// <param name="size">Buffer size.</param>
/// <param name="asImage">If set to true - buffer has image memory layout</param>
/// <param name="flags">Image mapping flags</param>
/// <param name="mapCallback">Mapping callback. Triggers for each mapped module</param>
/// <param name="context">User-supplied callback context</param>
/// <returns>Mapped image info</returns>
const ModuleData* MMap::MapImage(
    size_t size, void* buffer,
    bool asImage /*= false*/,
    eLoadFlags flags /*= NoFlags*/,
    MapCallback mapCallback /*= nullptr*/,
    void* context /*= nullptr*/,
    CustomArgs_t* pCustomArgs /*= nullptr*/
    )
{
    // Create fake path
    wchar_t path[64];
    wsprintfW( path, L"MemoryImage_0x%p", buffer );

    return MapImageInternal( path, buffer, size, asImage, flags, mapCallback, context, pCustomArgs );
}

/// <summary>
/// Manually map PE image into underlying target process
/// </summary>
/// <param name="path">Image path</param>
/// <param name="buffer">Image data buffer</param>
/// <param name="size">Buffer size.</param>
/// <param name="asImage">If set to true - buffer has image memory layout</param>
/// <param name="flags">Image mapping flags</param>
/// <param name="mapCallback">Mapping callback. Triggers for each mapped module</param>
/// <param name="context">User-supplied callback context</param>
/// <returns>Mapped image info</returns>
const ModuleData* MMap::MapImageInternal(
    const std::wstring& path,
    void* buffer, size_t size,
    bool asImage /*= false*/,
    eLoadFlags flags /*= NoFlags*/,
    MapCallback mapCallback /*= nullptr*/,
    void* context /*= nullptr*/,
    CustomArgs_t* pCustomArgs /*= nullptr*/
    )
{
    // Already loaded
    if (auto hMod = _process.modules().GetModule( path ))
        return hMod;

    // Prepare target process
    if (!NT_SUCCESS( _process.remote().CreateRPCEnvironment() ))
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
    _mapCallback = mapCallback;
    _userContext = context;

    BLACBONE_TRACE( L"ManualMap: Mapping image '%ls' with flags 0x%x", path.c_str(), flags );

    // Map module and all dependencies
    auto mod = FindOrMapModule( path, buffer, size, asImage, flags );
    if (mod == nullptr)
    {
        NTSTATUS tmp = LastNtStatus();
        Cleanup();
        LastNtStatus( tmp );
        return nullptr;
    }

    // Change process base module address if needed
    if (flags & RebaseProcess && !_images.empty() && _images.rbegin()->get()->peImage.isExe())
    {
        BLACBONE_TRACE( L"ManualMap: Rebasing process to address 0x%p", static_cast<uintptr_t>(mod->baseAddress) );

        // Managed path fix
        if (_images.rbegin()->get()->peImage.pureIL() && !path.empty())
            FixManagedPath( _process.memory().Read<uintptr_t>( _process.core().peb() + 2 * WordSize ), path );

        _process.memory().Write( _process.core().peb() + 2 * WordSize, WordSize, &mod->baseAddress );
    }

    auto wipeMemory = []( Process& proc, ImageContext* img, uintptr_t offset, uintptr_t size )
    {
        size = Align( size, 0x1000 );
        std::unique_ptr<uint8_t[]> zeroBuf( new uint8_t[size]() );

        if (img->flags & HideVAD)
        {
            Driver().WriteMem( proc.pid(), img->imgMem.ptr() + offset, size, zeroBuf.get() );
        }
        else
        {
            img->imgMem.Write( offset, size, zeroBuf.get() );
            if (!NT_SUCCESS( proc.memory().Free( img->imgMem.ptr() + offset, size ) ))
                proc.memory().Protect( img->imgMem.ptr() + offset, size, PAGE_NOACCESS );
        }
    };

    // Run initializers
    for (auto& img : _images)
    {
        // Init once
        if (!img->initialized)
        {
            // Hack for IL dlls
            if (!img->peImage.isExe() && img->peImage.pureIL())
            {
                DWORD flOld = 0;
                auto flg = img->imgMem.Read( img->peImage.ilFlagOffset(), 0 );
                img->imgMem.Protect( PAGE_EXECUTE_READWRITE, img->peImage.ilFlagOffset(), sizeof( flg ), &flOld );
                img->imgMem.Write( img->peImage.ilFlagOffset(), flg & ~COMIMAGE_FLAGS_ILONLY );
                img->imgMem.Protect( flOld, img->peImage.ilFlagOffset(), sizeof( flg ), &flOld );
            }

            if (!RunModuleInitializers( img.get(), DLL_PROCESS_ATTACH, pCustomArgs ))
                return nullptr;

            // Wipe header
            if (img->flags & WipeHeader)
                wipeMemory( _process, img.get(), 0, img->peImage.headersSize() );

            // Wipe discardable sections for non pure IL images
            for (auto& sec : img->peImage.sections())
                if (sec.Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
                    wipeMemory( _process, img.get(), sec.VirtualAddress, sec.Misc.VirtualSize );

            img->initialized = true;
        }
    }

    Cleanup();

    return mod;
}

/// <summary>
/// Fix image path for pure managed mapping
/// </summary>
/// <param name="base">Image base</param>
/// <param name="path">New image path</param>
void MMap::FixManagedPath( uintptr_t base, const std::wstring &path )
{
    _PEB_T2<DWORD_PTR>::type peb = { { { 0 } } };
    _PEB_LDR_DATA2<DWORD_PTR> ldr = { 0 };

    if (_process.core().peb( &peb ) != 0 && _process.memory().Read( peb.Ldr, sizeof( ldr ), &ldr ) == STATUS_SUCCESS)
    {
        // Get PEB loader entry
        for (auto head = static_cast<DWORD_PTR>(ldr.InLoadOrderModuleList.Flink);
            head != (peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<DWORD_PTR>, InLoadOrderModuleList ));
            head = _process.memory().Read<DWORD_PTR>( head ))
        {
            _LDR_DATA_TABLE_ENTRY_BASE<DWORD_PTR> localdata = { { 0 } };

            _process.memory().Read( head, sizeof( localdata ), &localdata );
            if (localdata.DllBase == base)
            {
                auto len = path.length()* sizeof( wchar_t );
                _process.memory().Write( localdata.FullDllName.Buffer, len + 2, path.c_str() );
                _process.memory().Write<short>(
                    head + FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<DWORD_PTR>, FullDllName.Length ),
                    static_cast<short>(len)
                    );

                return;
            }
        }
    }
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
const ModuleData* MMap::FindOrMapModule(
    const std::wstring& path,
    void* buffer, size_t size, bool asImage,
    eLoadFlags flags /*= NoFlags*/ 
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    std::unique_ptr<ImageContext> pImage( new ImageContext() );

    pImage->FilePath = path;
    pImage->FileName = Utils::StripPath( pImage->FilePath );
    pImage->flags = flags;

    // Load and parse image
    status = buffer ? pImage->peImage.Load( buffer, size, !asImage ) : pImage->peImage.Load( path, flags & NoSxS ? true : false );
    if (!NT_SUCCESS( status ))
    {
        LastNtStatus( status );
        BLACBONE_TRACE( L"ManualMap: Failed to load image '%ls'/0x%p. Status 0x%X", path.c_str(), buffer, status );
        pImage->peImage.Release();
        return nullptr;
    }

    // Check if already loaded
    if (auto hMod = _process.modules().GetModule( path, LdrList, pImage->peImage.mType() ))
    {
        pImage->peImage.Release();
        return hMod;
    }

    BLACBONE_TRACE( L"ManualMap: Loading new image '%ls'", path.c_str() );

    // Try to map image in high (>4GB) memory range
    if (flags & MapInHighMem)
    {
        AllocateInHighMem( pImage->imgMem, pImage->peImage.imageSize() );
    }
    // Try to map image at it's original ASRL-aware base
    else if (flags & HideVAD)
    {      
        ptr_t base  = pImage->peImage.imageBase();
        ptr_t isize = pImage->peImage.imageSize();

        if (!NT_SUCCESS( Driver().EnsureLoaded() ))
        {
            pImage->peImage.Release();
            return nullptr;
        }

        // Allocate as physical at desired base
        status = Driver().AllocateMem( _process.pid(), base, isize, MEM_COMMIT, PAGE_EXECUTE_READWRITE, true );

        // Allocate at any base
        if (!NT_SUCCESS( status ))
        {
            base = 0;
            size = pImage->peImage.imageSize();
            status = Driver().AllocateMem( _process.pid(), base, isize, MEM_COMMIT, PAGE_EXECUTE_READWRITE, true );
        }

        // Store allocated region
        if (NT_SUCCESS( status ))
        {
            pImage->imgMem = MemBlock( &_process.memory(), base, static_cast<size_t>(isize), PAGE_EXECUTE_READWRITE, true, true );
        }
        // Stop mapping
        else
        {
            //flags &= ~HideVAD;
            BLACBONE_TRACE( L"ManualMap: Failed to allocate physical memory for image, status 0x%d", status );
            pImage->peImage.Release();
            return nullptr;
        }
    }

    // Allocate normally if something went wrong
    if (pImage->imgMem == 0)
        pImage->imgMem = _process.memory().Allocate( pImage->peImage.imageSize(), PAGE_EXECUTE_READWRITE, pImage->peImage.imageBase() );

    BLACBONE_TRACE( L"ManualMap: Image base allocated at 0x%p", pImage->imgMem.ptr<uintptr_t>() );

    if (!pImage->imgMem.valid())
        return nullptr;

    // Create Activation context for SxS
    if (pImage->peImage.manifestID() == 0)
        flags |= NoSxS;

    if (!(flags & NoSxS))
        CreateActx( pImage->peImage.manifestFile(), pImage->peImage.manifestID(), !pImage->peImage.noPhysFile() );

    // Core image mapping operations
    if (!CopyImage( pImage.get() ) || !RelocateImage( pImage.get() ))
    {
        pImage->peImage.Release();
        return nullptr;
    }

    auto mt = pImage->peImage.mType();
    auto pMod = _process.modules().AddManualModule( pImage->FilePath, pImage->imgMem.ptr<module_t>(), pImage->imgMem.size(), mt );

    // Import tables
    if (!ResolveImport( pImage.get() ) || (!(flags & NoDelayLoad) && !ResolveImport( pImage.get(), true )))
    {
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }

    // Apply proper memory protection for sections
    if (!(flags & HideVAD))
        ProtectImageMemory( pImage.get() );

    // Make exception handling possible (C and C++)
    if (!(flags & NoExceptions))
    {
        status = EnableExceptions( pImage.get() );
        if (!NT_SUCCESS( status ) && status != STATUS_NOT_FOUND)
        {
            BLACBONE_TRACE( L"ManualMap: Failed to enable exception handling for image %ls", pImage->FileName.c_str() );

            pImage->peImage.Release();
            _process.modules().RemoveManualModule( pImage->FileName, mt );
            return nullptr;
        }
    }

    // Unlink image from VAD list
    if (flags & HideVAD && !NT_SUCCESS( ConcealVad( pImage->imgMem ) ))
    {
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }

    // Initialize security cookie
    if (!InitializeCookie( pImage.get() ))
    {
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }

    // Get entry point
    pImage->EntryPoint = pImage->peImage.entryPoint( pImage->imgMem.ptr<ptr_t>() );

    // Create reference for native loader functions
    LdrRefFlags ldrFlags = flags & CreateLdrRef ? Ldr_All: Ldr_None;
    if (_mapCallback != nullptr)
    {
        auto mapData = _mapCallback( PostCallback, _userContext, _process, *pMod );
        ldrFlags = mapData.ldrFlags;
    }

    if (ldrFlags != Ldr_None)
    {
        _process.nativeLdr().CreateNTReference(
            pImage->imgMem.ptr<HMODULE>(),
            pImage->peImage.imageSize(),
            pImage->FilePath,
            static_cast<uintptr_t>(pImage->EntryPoint),
            ldrFlags
            );
    }

    // Static TLS data
    if (!(flags & NoTLS) &&! InitStaticTLS( pImage.get( ) ))
    {
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( pImage->FileName, mt );
        return nullptr;
    }
    
    // Fill TLS callbacks
    pImage->peImage.GetTLSCallbacks( pImage->imgMem.ptr<ptr_t>(), pImage->tlsCallbacks );

    // Unload local copy
    pImage->peImage.Release();

    // Release ownership of image memory block
    pImage->imgMem.Release();

    // Store image
    _images.emplace_back( std::move( pImage ) );
    return pMod;
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
        _process.modules().RemoveManualModule( pImage->FilePath, pImage->peImage.mType() );
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
    NTSTATUS status = STATUS_SUCCESS;

    BLACBONE_TRACE( L"ManualMap: Performing image copy" );

    // offset to first section equals to header size
    size_t dwHeaderSize = pImage->peImage.headersSize();

    // Copy header
    if (pImage->flags & HideVAD)
        status = Driver().WriteMem( _process.pid(), pImage->imgMem.ptr(), dwHeaderSize, pImage->peImage.base() );
    else
        status = pImage->imgMem.Write( 0, dwHeaderSize, pImage->peImage.base() );

    if (!NT_SUCCESS( status ))
    {
        BLACBONE_TRACE( L"ManualMap: Failed to copy image headers. Status = 0x%x", status );
        return false;
    }

    // Set header protection
    if (!(pImage->flags & HideVAD) && pImage->imgMem.Protect( PAGE_READONLY, 0, dwHeaderSize ) != STATUS_SUCCESS)
    {
        BLACBONE_TRACE( L"ManualMap: Failed to set header memory protection. Status = 0x%x", LastNtStatus() );
        return false;
    }

    auto& sections = pImage->peImage.sections();

    // Copy sections
    for (auto& section : sections)
    {
        // Skip discardable sections
        if (section.Characteristics & (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE))
        {
            if (section.SizeOfRawData == 0)
                continue;

            uint8_t* pSource = reinterpret_cast<uint8_t*>(pImage->peImage.ResolveRVAToVA( section.VirtualAddress ));
            
            // Copy section data
            if (pImage->flags & HideVAD)
            {
                status = Driver().WriteMem(
                    _process.pid(), pImage->imgMem.ptr() + section.VirtualAddress,
                    section.SizeOfRawData, pSource
                    );
            }
            else
            {
                status = pImage->imgMem.Write( section.VirtualAddress, section.SizeOfRawData, pSource );
            }

            if (!NT_SUCCESS( status ))
            {
                BLACBONE_TRACE(
                    L"ManualMap: Failed to copy image section at offset 0x%x. Status = 0x%x",
                    section.VirtualAddress, status
                    );
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
    for (auto& section : pImage->peImage.sections())
    {
        auto prot = GetSectionProt( section.Characteristics );
        if (prot != PAGE_NOACCESS)
        {
            if (pImage->imgMem.Protect( prot, section.VirtualAddress, section.Misc.VirtualSize ) != STATUS_SUCCESS)
            {
                BLACBONE_TRACE(
                    L"ManualMap: Failed to set section memory protection at offset 0x%x. Status = 0x%x",
                    section.VirtualAddress, LastNtStatus()
                    );

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
    uintptr_t Delta = pImage->imgMem.ptr<uintptr_t>() - static_cast<uintptr_t>(pImage->peImage.imageBase());

    // No need to relocate
    if (Delta == 0)
    {
        BLACBONE_TRACE( L"ManualMap: No need for relocation" );
        LastNtStatus( STATUS_SUCCESS );
        return true;
    }

    auto start = pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_BASERELOC );
    auto end = start + pImage->peImage.DirectorySize( IMAGE_DIRECTORY_ENTRY_BASERELOC );

    pe::RelocData* fixrec = reinterpret_cast<pe::RelocData*>(start);
    if (fixrec == nullptr)
    {
        // TODO: return proper error code
        BLACBONE_TRACE( L"ManualMap: Can't relocate image, no relocation data" );
        LastNtStatus( STATUS_IMAGE_NOT_AT_BASE );
        return false;
    }

    while ((uintptr_t)fixrec < end && fixrec->BlockSize)
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
                uintptr_t fixRVA = fixoffset + fixrec->PageRVA;
                uintptr_t val = *reinterpret_cast<uintptr_t*>(pImage->peImage.ResolveRVAToVA( fixoffset + fixrec->PageRVA )) + Delta;

                auto status = STATUS_SUCCESS;

                if (pImage->flags & HideVAD)
                    status = Driver().WriteMem( _process.pid(), pImage->imgMem.ptr() + fixRVA, sizeof( val ), &val );
                else
                    status = pImage->imgMem.Write( fixRVA, val );

                // Apply relocation
                if (!NT_SUCCESS( status ))
                {
                    BLACBONE_TRACE( L"ManualMap: Failed to apply relocation at offset 0x%x. Status = 0x%x", fixRVA, status );
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
        fixrec = reinterpret_cast<pe::RelocData*>(reinterpret_cast<uintptr_t>(fixrec) + fixrec->BlockSize);
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
    // Already loaded
    auto hMod = _process.modules().GetModule( path, LdrList, pImage->peImage.mType(), pImage->FileName.c_str() );
    if (hMod)
        return hMod;

    BLACBONE_TRACE( L"ManualMap: Loading new dependency '%ls'", path.c_str() );

    auto basedir = pImage->peImage.noPhysFile() ? Utils::GetExeDirectory() : Utils::GetParent( pImage->FilePath );
    auto status = NameResolve::Instance().ResolvePath( path, pImage->FileName, basedir, NameResolve::EnsureFullPath, _process.pid(), pImage->peImage.actx() );
    if (!NT_SUCCESS( status ))
    {
        BLACBONE_TRACE( L"ManualMap: Failed to resolve dependency path '%ls'", path.c_str() );
        return nullptr;
    }

    BLACBONE_TRACE( L"ManualMap: Dependency path resolved to '%ls'", path.c_str() );

    LoadData data;
    if (_mapCallback != nullptr)
    {
        ModuleData tmpData;
        tmpData.baseAddress = 0;
        tmpData.manual = ((pImage->flags & ManualImports) != 0);
        tmpData.fullPath = path;
        tmpData.name = Utils::StripPath( path );
        tmpData.size = 0;
        tmpData.type = mt_unknown;

        data = _mapCallback( PreCallback, _userContext, _process, tmpData );
    }

    // Loading method
    if (data.mtype == MT_Manual || (data.mtype == MT_Default && pImage->flags & ManualImports))
    {
        return FindOrMapModule( path, nullptr, 0, false, pImage->flags | NoSxS | NoDelayLoad | PartialExcept );
    }
    else if (data.mtype != MT_None)
    {
        return _process.modules().Inject( path );
    }
    // Aborted by user
    else
    {
        LastNtStatus( STATUS_REQUEST_CANCELED );
        return nullptr;
    }
};

/// <summary>
/// Resolves image import or delayed image import
/// </summary>
/// <param name="pImage">Image data</param>
/// <param name="useDelayed">Resolve delayed import instead</param>
/// <returns>true on success</returns>
bool MMap::ResolveImport( ImageContext* pImage, bool useDelayed /*= false */ )
{
    auto imports = pImage->peImage.GetImports( useDelayed );
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
                LastNtStatus( STATUS_ORDINAL_NOT_FOUND );

                if (importFn.importByOrd)
                    BLACBONE_TRACE( L"ManualMap: Failed to get import #%d from image '%ls'", 
                                    importFn.importOrdinal, wstrDll.c_str() );
                else
                    BLACBONE_TRACE( L"ManualMap: Failed to get import '%ls' from image '%ls'",
                                    Utils::AnsiToWstring(importFn.importName).c_str(), wstrDll.c_str() );
                return false;
            }

            auto status = STATUS_SUCCESS;

            if (pImage->flags & HideVAD)
            {
                uintptr_t address = static_cast<uintptr_t>(expData.procAddress);
                status = Driver().WriteMem( _process.pid(), pImage->imgMem.ptr() + importFn.ptrRVA, sizeof( address ), &address );
            }
            else
                status = pImage->imgMem.Write( importFn.ptrRVA, static_cast<uintptr_t>(expData.procAddress) );

            // Write function address
            if (!NT_SUCCESS( status ))
            {
                BLACBONE_TRACE( L"ManualMap: Failed to write import function address at offset 0x%x. Status = 0x%x",
                                importFn.ptrRVA, status );
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
NTSTATUS MMap::EnableExceptions( ImageContext* pImage )
{
    BLACBONE_TRACE( L"ManualMap: Enabling exception support for image '%ls'", pImage->FileName.c_str() );
    bool partial = (pImage->flags & PartialExcept) != 0;

#ifdef USE64
    size_t size = pImage->peImage.DirectorySize( IMAGE_DIRECTORY_ENTRY_EXCEPTION );
    auto pExpTable = reinterpret_cast<PIMAGE_RUNTIME_FUNCTION_ENTRY>(pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_EXCEPTION ));

    // Invoke RtlAddFunctionTable
    if(pExpTable)
    {     
        AsmJitHelper a;
        uint64_t result = 0;

        pImage->pExpTableAddr = REBASE( pExpTable, pImage->peImage.base(), pImage->imgMem.ptr<ptr_t>() );
        auto pAddTable = _process.modules().GetExport( 
            _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->peImage.mType() ),
            "RtlAddFunctionTable" 
            );

        a.GenPrologue();
        a.GenCall( 
            static_cast<uintptr_t>(pAddTable.procAddress), {
            pImage->pExpTableAddr,
            size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY ),
            pImage->imgMem.ptr<uintptr_t>() }
        );

        _process.remote().AddReturnWithEvent( a, pImage->peImage.mType() );
        a.GenEpilogue();

        auto status = _process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result );
        if (!NT_SUCCESS( status ))
            return status;

        return (pImage->flags & CreateLdrRef) ? STATUS_SUCCESS : 
            MExcept::CreateVEH( pImage->imgMem.ptr<uintptr_t>(), pImage->peImage.imageSize(), pImage->peImage.mType(), partial );
    }
    // No exception table
    else
        return STATUS_NOT_FOUND;
#else
    bool safeseh = false;
    if (!_process.nativeLdr().InsertInvertedFunctionTable( pImage->imgMem.ptr<void*>(), pImage->peImage.imageSize(), safeseh ))
        return STATUS_UNSUCCESSFUL;

    return safeseh ? STATUS_SUCCESS : 
        MExcept::CreateVEH( pImage->imgMem.ptr<uintptr_t>(), pImage->peImage.imageSize(), pImage->peImage.mType(), partial );

#endif
}

/// <summary>
/// Remove custom exception handler
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
NTSTATUS MMap::DisableExceptions( ImageContext* pImage )
{
    BLACBONE_TRACE( L"ManualMap: Disabling exception support for image '%ls'", pImage->FileName.c_str() );

#ifdef USE64
    if (pImage->pExpTableAddr)
    {
        AsmJitHelper a;
        uint64_t result = 0;

        auto pRemoveTable = _process.modules().GetExport(
            _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->peImage.mType() ),
            "RtlDeleteFunctionTable"
            );

        a.GenPrologue();
        // RtlDeleteFunctionTable(pExpTable);
        a.GenCall( static_cast<uintptr_t>(pRemoveTable.procAddress), { pImage->pExpTableAddr } );
        _process.remote().AddReturnWithEvent( a );
        a.GenEpilogue();

        auto status = _process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result );
        if (!NT_SUCCESS( status ))
            return status;

        return MExcept::RemoveVEH( (pImage->flags & CreateLdrRef) != 0 );
    }
    else
        return STATUS_NOT_FOUND;
#else
    return MExcept::RemoveVEH( (pImage->flags & PartialExcept) != 0 );

#endif
}

/// <summary>
/// Resolve static TLS storage
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
bool MMap::InitStaticTLS( ImageContext* pImage )
{
    IMAGE_TLS_DIRECTORY *pTls = reinterpret_cast<decltype(pTls)>(pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_TLS ));
    auto pRebasedTls = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(REBASE( pTls, pImage->peImage.base(), pImage->imgMem.ptr<ptr_t>() ));

    // Use native TLS initialization
    if (pTls && pTls->AddressOfIndex)
    {
        BLACBONE_TRACE( L"ManualMap: Performing static TLS initialization for image '%ls'", pImage->FileName.c_str() );
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
    auto pLC = reinterpret_cast<PIMAGE_LOAD_CONFIG_DIRECTORY>(pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ));

    //
    // Cookie generation based on MSVC++ compiler
    //
    if (pLC && pLC->SecurityCookie)
    {
        BLACBONE_TRACE( L"ManualMap: Performing security cookie initializtion for image '%ls'", pImage->FileName.c_str() );

        FILETIME systime = { 0 };
        LARGE_INTEGER PerformanceCount = { { 0 } };
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

        _process.memory().Write( REBASE( pLC->SecurityCookie, pImage->peImage.imageBase(), pImage->imgMem.ptr<ptr_t>() ), cookie );
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
bool MMap::RunModuleInitializers( ImageContext* pImage, DWORD dwReason, CustomArgs_t* pCustomArgs /*= nullptr*/ )
{
    AsmJitHelper a;
    uint64_t result = 0;

    auto hNtdll = _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->peImage.mType() );
    auto pActivateActx = _process.modules().GetExport( hNtdll, "RtlActivateActivationContext" );
    auto pDeactivateeActx = _process.modules().GetExport( hNtdll, "RtlDeactivateActivationContext" );

    a.GenPrologue();

    // ActivateActCtx
    if (_pAContext.valid() && pActivateActx.procAddress)
    {
        a->mov( a->zax, _pAContext.ptr<uintptr_t>() );
        a->mov( a->zax, asmjit::host::dword_ptr( a->zax ) );
        a.GenCall( static_cast<uintptr_t>( pActivateActx.procAddress ), { 0, a->zax, _pAContext.ptr<uintptr_t>() + sizeof( HANDLE ) } );
    }

    // Prepare custom arguments
    uintptr_t customArgumentsAddress = 0;
    if (pCustomArgs)
    {
        auto memBuf = _process.memory().Allocate( static_cast<size_t>(pCustomArgs->size()) + sizeof( uint64_t ), PAGE_EXECUTE_READWRITE, 0, false );
        memBuf.Write( 0, pCustomArgs->size() );
        memBuf.Write( sizeof( uint64_t ), static_cast<size_t>(pCustomArgs->size()), pCustomArgs->data() );
        customArgumentsAddress = static_cast<uintptr_t>( memBuf.ptr() );
    }

    // Function order
    // TLS first, entry point last
    if (dwReason == DLL_PROCESS_ATTACH || dwReason == DLL_THREAD_ATTACH)
    {
        // PTLS_CALLBACK_FUNCTION(pImage->ImageBase, dwReason, NULL);
        if (!( pImage->flags & NoTLS ))
            for (auto& pCallback : pImage->tlsCallbacks)
            {
                BLACBONE_TRACE( L"ManualMap: Calling TLS callback at 0x%p for '%ls', Reason: %d",
                    static_cast<uintptr_t>( pCallback ), pImage->FileName.c_str(), dwReason );

                a.GenCall( static_cast<uintptr_t>( pCallback ), { pImage->imgMem.ptr<uintptr_t>(), dwReason, customArgumentsAddress } );
            }

        // DllMain
        if (pImage->EntryPoint != 0)
        {
            BLACBONE_TRACE( L"ManualMap: Calling entry point for '%ls', Reason: %d", pImage->FileName.c_str(), dwReason );
            a.GenCall( static_cast<uintptr_t>( pImage->EntryPoint ), { pImage->imgMem.ptr<uintptr_t>(), dwReason, customArgumentsAddress } );
        }
    }
    // Entry point first, TLS last
    else
    {
        // DllMain
        if (pImage->EntryPoint != 0)
        {
            BLACBONE_TRACE( L"ManualMap: Calling entry point for '%ls', Reason: %d", pImage->FileName.c_str(), dwReason );
            a.GenCall( static_cast<uintptr_t>( pImage->EntryPoint ), { pImage->imgMem.ptr<uintptr_t>(), dwReason, customArgumentsAddress } );
        }

        // PTLS_CALLBACK_FUNCTION(pImage->ImageBase, dwReason, NULL);
        if (!( pImage->flags & NoTLS ))
            for (auto& pCallback : pImage->tlsCallbacks)
            {
                BLACBONE_TRACE( L"ManualMap: Calling TLS callback at 0x%p for '%ls', Reason: %d",
                    static_cast<uintptr_t>( pCallback ), pImage->FileName.c_str(), dwReason );

                a.GenCall( static_cast<uintptr_t>( pCallback ), { pImage->imgMem.ptr<uintptr_t>(), dwReason, customArgumentsAddress } );
            }
    }

    // DeactivateActCtx
    if (_pAContext.valid() && pDeactivateeActx.procAddress)
    {
        a->mov( a->zax, _pAContext.ptr<uintptr_t>() + sizeof( HANDLE ) );
        a->mov( a->zax, asmjit::host::dword_ptr( a->zax ) );
        a.GenCall( static_cast<uintptr_t>( pDeactivateeActx.procAddress ), { 0, a->zax } );
    }

    _process.remote().AddReturnWithEvent( a, pImage->peImage.mType() );
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
/// <param name="path">Manifest container path</param>
/// <param name="id">Manifest resource id</param>
/// <param name="asImage">if true - 'path' points to a valid PE file, otherwise - 'path' points to separate manifest file</param>
/// <returns>true on success</returns>
bool MMap::CreateActx( const std::wstring& path, int id /*= 2 */, bool asImage /*= true*/  )
{   
    AsmJitHelper a;

    uint64_t result = 0;
    ACTCTXW act = { 0 };

    _pAContext = _process.memory().Allocate( 512, PAGE_READWRITE );
    
    act.cbSize = sizeof(act);
    act.lpSource = reinterpret_cast<LPCWSTR>(_pAContext.ptr<uintptr_t>() + sizeof( HANDLE ) + sizeof( act ));

    // Ignore some fields for pure manifest file
    if (asImage)
    {
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act.lpResourceName = MAKEINTRESOURCEW( id );
    }

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
        a->call( a->zax );
        a->mov( asmjit::host::edx, _pAContext.ptr<uint32_t>() );
        //a->mov( asmjit::host::dword_ptr( asmjit::host::edx ), asmjit::host::eax );
        a->dw( '\x01\x02' );

        auto pTermThd = _process.modules().GetExport( _process.modules().GetModule( L"ntdll.dll" ), "NtTerminateThread" );
        a->push( a->zax );
        a->push( uint32_t( 0 ) );
        a->mov( asmjit::host::eax, static_cast<uint32_t>(pTermThd.procAddress) );
        a->call( a->zax );
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

        a.GenCall( static_cast<uintptr_t>(pCreateActx.procAddress), { _pAContext.ptr<uintptr_t>() + sizeof(HANDLE) } );

        a->mov( a->zdx, _pAContext.ptr<uintptr_t>() );
        a->mov( a->intptr_ptr( a->zdx ), a->zax );

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
/// Hide memory VAD node
/// </summary>
/// <param name="imageMem">Image to purge</param>
/// <returns>Status code</returns>
NTSTATUS MMap::ConcealVad( const MemBlock& imageMem)
{
    return Driver().ConcealVAD( _process.pid(), imageMem.ptr(), static_cast<uint32_t>(imageMem.size()) );
}

/// <summary>
/// Allocates memory region beyond 4GB limit
/// </summary>
/// <param name="imageMem">Image data</param>
/// <param name="size">Block size</param>
/// <returns>Status code</returns>
NTSTATUS MMap::AllocateInHighMem( MemBlock& imageMem, size_t size )
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
        {
            if (ptr >= entry.first && ptr < entry.first + entry.second)
            {
                found = true;
                break;
            }
        }
    }

    // Not implemented yet
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    // Change protection and save address
    if (NT_SUCCESS( status ))
    {
        _usedBlocks.emplace_back( std::make_pair( ptr, size ) );

        imageMem = MemBlock( &_process.memory(), ptr, size, PAGE_READWRITE, false );
        _process.memory().Protect( ptr, size, PAGE_READWRITE );
        return true;
    }

    return status;
}

/// <summary>
/// Remove any traces from remote process
/// </summary>
/// <returns></returns>
void MMap::Cleanup()
{
    reset();
    MExcept::reset();
    _process.remote().reset();
}


/// <summary>
/// Transform section characteristics into memory protection flags
/// </summary>
/// <param name="characteristics">Section characteristics</param>
/// <returns>Memory protection value</returns>
DWORD MMap::GetSectionProt( DWORD characteristics )
{
    DWORD dwResult = PAGE_NOACCESS;

    if(characteristics & IMAGE_SCN_MEM_EXECUTE) 
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
