#include "MMap.h"
#include "../Process/Process.h"
#include "../Misc/NameResolve.h"
#include "../Misc/Utils.h"
#include "../Misc/DynImport.h"
#include "../Misc/Trace.hpp"
#include "../DriverControl/DriverControl.h"

#include <random>
#include <3rd_party/VersionApi.h>

#ifndef STATUS_INVALID_EXCEPTION_HANDLER
#define STATUS_INVALID_EXCEPTION_HANDLER ((NTSTATUS)0xC00001A5L)
#endif

namespace blackbone
{

MMap::MMap( Process& proc )
    : _process( proc )
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
call_result_t<ModuleDataPtr> MMap::MapImage(
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
call_result_t<ModuleDataPtr> MMap::MapImage(
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
call_result_t<ModuleDataPtr> MMap::MapImageInternal(
    const std::wstring& path,
    void* buffer, size_t size,
    bool asImage /*= false*/,
    eLoadFlags flags /*= NoFlags*/,
    MapCallback mapCallback /*= nullptr*/,
    void* context /*= nullptr*/,
    CustomArgs_t* pCustomArgs /*= nullptr*/
    )
{
    if (!(flags & ForceRemap))
    {
        // Already loaded
        if (auto hMod = _process.modules().GetModule(path))
            return hMod;
    }

    // Prepare target process
    auto mode = (flags & NoThreads) ? Worker_UseExisting : Worker_CreateNew;
    auto status = _process.remote().CreateRPCEnvironment( mode, true );
    if (!NT_SUCCESS( status ))
    {
        Cleanup();
        return status;
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

    BLACKBONE_TRACE( L"ManualMap: Mapping image '%ls' with flags 0x%x", path.c_str(), flags );

    // Map module and all dependencies
    auto mod = FindOrMapModule( path, buffer, size, asImage, flags );
    if (!mod)
    {
        Cleanup();
        return mod.status;
    }

    // Change process base module address if needed
    if (flags & RebaseProcess && !_images.empty() && _images.rbegin()->get()->peImage.isExe())
    {
        BLACKBONE_TRACE( L"ManualMap: Rebasing process to address 0x%p", (*mod)->baseAddress );

        // Managed path fix
        if (_images.rbegin()->get()->peImage.pureIL() && !path.empty())
        {
            CALL_64_86(
                (*mod)->type == mt_mod64,
                FixManagedPath, 
                _process.modules().GetMainModule()->baseAddress, 
                path 
            );
        }

        // PEB64
        _process.memory().Write( 
            fieldPtr( _process.core().peb64(), &_PEB64::ImageBaseAddress ),
            sizeof( uint64_t ), 
            &(*mod)->baseAddress
        );

        // PEB32
        if(_process.core().isWow64())
        {
            _process.memory().Write(
                fieldPtr(_process.core().peb32(), &_PEB32::ImageBaseAddress ),
                sizeof( uint32_t ), 
                &(*mod)->baseAddress
            );
        }
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
            proc.memory().Protect( img->imgMem.ptr() + offset, size, PAGE_NOACCESS );
            proc.memory().Free( img->imgMem.ptr() + offset, size, MEM_DECOMMIT );
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

            // Don't run initializer for pure IL dlls
            if (!img->peImage.pureIL() || img->peImage.isExe())
                status = RunModuleInitializers( img, DLL_PROCESS_ATTACH, pCustomArgs ).status;

            if (!NT_SUCCESS( status ))
            {
                BLACKBONE_TRACE( L"ManualMap: ModuleInitializers failed for '%ls', status: 0x%X", img->ldrEntry.name.c_str(), status );
                Cleanup();
                return status;
            }

            // Wipe header
            if (img->flags & WipeHeader)
                wipeMemory( _process, img.get(), 0, img->peImage.headersSize() );

            // Wipe discardable sections
            if(!img->peImage.pureIL())
            {
                for (auto& sec : img->peImage.sections())
                    if (sec.Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
                        wipeMemory( _process, img.get(), sec.VirtualAddress, sec.Misc.VirtualSize );
            }

            img->initialized = true;
        }
    }

    //Cleanup();
    return mod;
}

/// <summary>
/// Fix image path for pure managed mapping
/// </summary>
/// <param name="base">Image base</param>
/// <param name="path">New image path</param>
template<typename T>
void MMap::FixManagedPath( ptr_t base, const std::wstring &path )
{
    _PEB_T<T> peb = { 0 };
    _PEB_LDR_DATA2_T<T> ldr = { 0 };

    if (_process.core().peb( &peb ) != 0 && _process.memory().Read( peb.Ldr, sizeof( ldr ), &ldr ) == STATUS_SUCCESS)
    {
        // Get PEB loader entry
        for (auto head = ldr.InLoadOrderModuleList.Flink;
            head != fieldPtr( peb.Ldr, &_PEB_LDR_DATA2_T<T>::InLoadOrderModuleList );
            head = _process.memory().Read<T>( head ).result( 0 ))
        {
            _LDR_DATA_TABLE_ENTRY_BASE_T<T> localdata = { { 0 } };

            _process.memory().Read( head, sizeof( localdata ), &localdata );
            if (localdata.DllBase == base)
            {
                auto len = path.length()* sizeof( wchar_t );
                _process.memory().Write( localdata.FullDllName.Buffer, len + 2, path.c_str() );
                _process.memory().Write<short>(
                    head + FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE_T<T>, FullDllName.Length ),
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
call_result_t<ModuleDataPtr> MMap::FindOrMapModule(
    const std::wstring& path,
    void* buffer, size_t size, bool asImage,
    eLoadFlags flags /*= NoFlags*/ 
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ImageContextPtr pImage( new ImageContext() );
    auto& ldrEntry = pImage->ldrEntry;

    ldrEntry.fullPath = Utils::ToLower( path );
    ldrEntry.name = Utils::StripPath( ldrEntry.fullPath );
    pImage->flags = flags;

    // Load and parse image
    status = buffer ? pImage->peImage.Load( buffer, size, !asImage ) : pImage->peImage.Load( path, flags & NoSxS ? true : false );
    if (!NT_SUCCESS( status ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to load image '%ls'/0x%p. Status 0x%X", path.c_str(), buffer, status );
        pImage->peImage.Release();
        return status;
    }

    // Check if already loaded, but only if doesn't explicitly excluded
    if (!(flags & ForceRemap))
    {
        if (auto hMod = _process.modules().GetModule( path, LdrList, pImage->peImage.mType() ))
        {
            pImage->peImage.Release();
            return hMod;
        }
    }

    // Check architecture
    if (pImage->peImage.mType() == mt_mod32 && !_process.core().isWow64())
    {
        BLACKBONE_TRACE( L"ManualMap: Can't map x86 dll '%ls' into native x64 process", path.c_str() );
        pImage->peImage.Release();
        return STATUS_INVALID_IMAGE_WIN_32;
    }

    BLACKBONE_TRACE( L"ManualMap: Loading new image '%ls'", path.c_str() );

    ldrEntry.type = pImage->peImage.mType();

    // Try to map image in high (>4GB) memory range
    if (flags & MapInHighMem)
    {
        AllocateInHighMem( pImage->imgMem, pImage->peImage.imageSize() );
    }
    // Try to map image at it's original ASRL-aware base
    else if (flags & HideVAD)
    {      
        ptr_t base  = pImage->peImage.imageBase();
        ptr_t image_size = pImage->peImage.imageSize();

        if (!NT_SUCCESS( Driver().EnsureLoaded() ))
        {
            pImage->peImage.Release();
            return Driver().status();
        }

        // Allocate as physical at desired base
        status = Driver().AllocateMem( _process.pid(), base, image_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE, true );

        // Allocate at any base
        if (!NT_SUCCESS( status ))
        {
            base = 0;
            image_size = pImage->peImage.imageSize();
            status = Driver().AllocateMem( _process.pid(), base, image_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE, true );
        }

        // Store allocated region
        if (NT_SUCCESS( status ))
        {
            pImage->imgMem = MemBlock( &_process.memory(), base, static_cast<size_t>(image_size), PAGE_EXECUTE_READWRITE, true, true );
        }
        // Stop mapping
        else
        {
            //flags &= ~HideVAD;
            BLACKBONE_TRACE( L"ManualMap: Failed to allocate physical memory for image, status 0x%X", status );
            pImage->peImage.Release();
            return status;
        }
    }

    // Allocate normally if something went wrong
    if (!pImage->imgMem.valid())
    {
        auto mem = _process.memory().Allocate( pImage->peImage.imageSize(), PAGE_EXECUTE_READWRITE, pImage->peImage.imageBase() );
        if (!mem)
        {
            BLACKBONE_TRACE( L"ManualMap: Failed to allocate memory for image, status 0x%X", status );
            pImage->peImage.Release();
            return mem.status;
        }

        pImage->imgMem = std::move( mem.result() );
    }

    ldrEntry.baseAddress = pImage->imgMem.ptr();
    ldrEntry.size = pImage->peImage.imageSize();

    BLACKBONE_TRACE( L"ManualMap: Image base allocated at 0x%016llx", pImage->imgMem.ptr() );

    // Create Activation context for SxS
    if (pImage->peImage.manifestID() == 0)
        flags |= NoSxS;

    if (!(flags & NoSxS))
    {
        status = CreateActx( pImage->peImage );
        if (!NT_SUCCESS( status ))
        {
            pImage->peImage.Release();
            return status;
        }
    }

    // Core image mapping operations
    if (!NT_SUCCESS( status = CopyImage( pImage ) ))
    {
        pImage->peImage.Release();
        return status;
    }

    if (!NT_SUCCESS( status = RelocateImage( pImage ) ))
    {
        pImage->peImage.Release();
        return status;
    }

    auto mt = ldrEntry.type;
	ModuleDataPtr pMod;

    if (flags & ForceRemap)
        pMod = std::make_shared<const ModuleData>( _process.modules().Canonicalize( ldrEntry, true ) );
    else
        pMod = _process.modules().AddManualModule( ldrEntry );

	{
        // Handle x64 system32 dlls for wow64 process
        bool fsRedirect = !(flags & IsDependency) && mt == mt_mod64 && _process.barrier().sourceWow64;

        FsRedirector fsr( fsRedirect );

        // Import
        if (!NT_SUCCESS( status = ResolveImport( pImage ) ))
        {
            pImage->peImage.Release();
            _process.modules().RemoveManualModule( ldrEntry.name, mt );
            return status;
        }

        // Delayed import
        if (!(flags & NoDelayLoad) && !NT_SUCCESS( status = ResolveImport( pImage, true ) ))
        {
            pImage->peImage.Release();
            _process.modules().RemoveManualModule( ldrEntry.name, mt );
            return status;
        }
    }

    // Apply proper memory protection for sections
    if (!(flags & HideVAD))
        ProtectImageMemory( pImage );

    // Make exception handling possible (C and C++)
    if (!(flags & NoExceptions))
    {
        if (!NT_SUCCESS( status = EnableExceptions( pImage ) ) && status != STATUS_NOT_FOUND)
        {
            BLACKBONE_TRACE( L"ManualMap: Failed to enable exception handling for image %ls", ldrEntry.name.c_str() );
            pImage->peImage.Release();
            _process.modules().RemoveManualModule( ldrEntry.name, mt );
            return status;
        }
    }

    // Initialize security cookie
    if (!NT_SUCCESS ( status = InitializeCookie( pImage ) ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to initialize cookie for image %ls", ldrEntry.name.c_str() );
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( ldrEntry.name, mt );
        return status;
    }

    // Unlink image from VAD list
    if (flags & HideVAD && !NT_SUCCESS( status = ConcealVad( pImage->imgMem ) ))
    {
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( ldrEntry.name, mt );
        return status;
    }

    // Get entry point
    pImage->ldrEntry.entryPoint = pImage->peImage.entryPoint( pImage->imgMem.ptr<ptr_t>() );

    // Create reference for native loader functions
    pImage->ldrEntry.flags = flags & CreateLdrRef ? Ldr_All : Ldr_None;
    if (_mapCallback != nullptr)
    {
        auto mapData = _mapCallback( PostCallback, _userContext, _process, *pMod );
        if(mapData.ldrFlags != Ldr_Ignore)
            pImage->ldrEntry.flags = mapData.ldrFlags;
    }

    if (pImage->ldrEntry.flags != Ldr_None)
    {       
        if (!_process.nativeLdr().CreateNTReference( pImage->ldrEntry ))
        {
            BLACKBONE_TRACE( L"ManualMap: Failed to add loader reference for image %ls", ldrEntry.name.c_str() );
        }
    }

    // Static TLS data
    if (!(flags & NoTLS) && !NT_SUCCESS( status = InitStaticTLS( pImage ) ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to initialize static TLS for image %ls, status 0x%X", ldrEntry.name.c_str(), status );
        pImage->peImage.Release();
        _process.modules().RemoveManualModule( ldrEntry.name, mt );
        return status;
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
/// <returns>Status code</returns>
NTSTATUS MMap::UnmapAllModules()
{
    for (auto img = _images.rbegin(); img != _images.rend(); ++img)
    {
        auto pImage = *img;
        BLACKBONE_TRACE( L"ManualMap: Unmapping image '%ls'", pImage->ldrEntry.name.c_str() );

        // Call main
        RunModuleInitializers( pImage, DLL_PROCESS_DETACH );

        // Remove VEH
        if (!(pImage->flags & NoExceptions))
            DisableExceptions( pImage );

        _process.nativeLdr().UnloadTLS( pImage->ldrEntry );

        // Remove from loader
        if (pImage->ldrEntry.flags != Ldr_None)
            _process.nativeLdr().Unlink( pImage->ldrEntry );

        // Free memory
        pImage->imgMem.Free();

        // Remove reference from local modules list
        _process.modules().RemoveManualModule( pImage->ldrEntry.name, pImage->peImage.mType() );
    } 

    Cleanup();
    return STATUS_SUCCESS;
}

/// <summary>
/// Copies image into target process
/// </summary>
/// <param name="pImage">Image data</param>
/// <returns>Status code</returns>
NTSTATUS MMap::CopyImage( ImageContextPtr pImage )
{
    NTSTATUS status = STATUS_SUCCESS;

    BLACKBONE_TRACE( L"ManualMap: Performing image copy" );

    // offset to first section equals to header size
    size_t dwHeaderSize = pImage->peImage.headersSize();

    // Copy header
    if (pImage->flags & HideVAD)
        status = Driver().WriteMem( _process.pid(), pImage->imgMem.ptr(), dwHeaderSize, pImage->peImage.base() );
    else
        status = pImage->imgMem.Write( 0, dwHeaderSize, pImage->peImage.base() );

    if (!NT_SUCCESS( status ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to copy image headers. Status = 0x%x", status );
        return status;
    }

    // Set header protection
    if (!(pImage->flags & HideVAD))
    {
        status = pImage->imgMem.Protect( PAGE_READONLY, 0, dwHeaderSize );
        if (!NT_SUCCESS( status ))
        {
            BLACKBONE_TRACE( L"ManualMap: Failed to set header memory protection. Status = 0x%x", status );
            return status;
        }
    }

    // Copy sections
    for (auto& section : pImage->peImage.sections())
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
                BLACKBONE_TRACE(
                    L"ManualMap: Failed to copy image section at offset 0x%x. Status = 0x%x",
                    section.VirtualAddress, status
                    );

                return status;
            }
        } 
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Adjust image memory protection
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>Status code</returns>
NTSTATUS MMap::ProtectImageMemory( ImageContextPtr pImage )
{
    // Set section memory protection
    for (auto& section : pImage->peImage.sections())
    {
        auto prot = GetSectionProt( section.Characteristics );
        if (prot != PAGE_NOACCESS)
        {
            auto status = pImage->imgMem.Protect( prot, section.VirtualAddress, section.Misc.VirtualSize );
            if (!NT_SUCCESS( status ))
            {
                BLACKBONE_TRACE(
                    L"ManualMap: Failed to set section memory protection at offset 0x%x. Status = 0x%x",
                    section.VirtualAddress, status
                    );

                return status;
            }
        }
        // Decommit pages with NO_ACCESS protection
        else
        {
            auto status = _process.memory().Free( pImage->imgMem.ptr() + section.VirtualAddress, section.Misc.VirtualSize, MEM_DECOMMIT );
            if (!NT_SUCCESS( status ))
            {
                BLACKBONE_TRACE(
                    L"ManualMap: Failed to set section memory protection at offset 0x%x. Status = 0x%x",
                    section.VirtualAddress, status
                );
            }
        }
    }

    return STATUS_SUCCESS;
}

/// <summary>
///  Fix relocations if image wasn't loaded at base address
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
NTSTATUS MMap::RelocateImage( ImageContextPtr pImage )
{
    NTSTATUS status = STATUS_SUCCESS;
    BLACKBONE_TRACE( L"ManualMap: Relocating image '%ls'", pImage->ldrEntry.fullPath.c_str() );

    // Reloc delta
    ptr_t Delta = pImage->imgMem.ptr() - pImage->peImage.imageBase();

    // No need to relocate
    if (Delta == 0)
    {
        BLACKBONE_TRACE( L"ManualMap: No need for relocation" );
        return STATUS_SUCCESS;
    }

    // Dll can't be relocated
    if (!(pImage->peImage.DllCharacteristics() & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE))
    {
        BLACKBONE_TRACE( L"ManualMap: Can't relocate image, no relocation flag" );
        return STATUS_INVALID_IMAGE_HASH;
    }

    auto start = pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_BASERELOC );
    auto end = start + pImage->peImage.DirectorySize( IMAGE_DIRECTORY_ENTRY_BASERELOC );
    auto fixrec = reinterpret_cast<pe::RelocData*>(start);

    // No relocatable data
    if (fixrec == nullptr)
    {
        BLACKBONE_TRACE( L"ManualMap: Image does not use relocations" );
        return STATUS_SUCCESS;
    }

    // Read whole image to process it locally
    std::unique_ptr<uint8_t[]> localImage( new uint8_t[pImage->ldrEntry.size] );
    auto pLocal = localImage.get();
    _process.memory().Read( pImage->imgMem.ptr(), pImage->ldrEntry.size, pLocal );

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
                if (pImage->ldrEntry.type == mt_mod64)
                {
                    uint64_t val = *reinterpret_cast<uint64_t*>(pLocal + fixRVA) + Delta;
                    *reinterpret_cast<uint64_t*>(pLocal + fixRVA) = val;
                }
                else
                {
                    uint32_t val = *reinterpret_cast<uint32_t*>(pLocal + fixRVA) + static_cast<uint32_t>(Delta);
                    *reinterpret_cast<uint32_t*>(pLocal + fixRVA) = static_cast<uint32_t>(val);
                }
            }
            else
            {
                // TODO: support for all remaining relocations
                BLACKBONE_TRACE( L"ManualMap: Abnormal relocation type %d. Aborting", fixtype );
                return STATUS_INVALID_IMAGE_FORMAT;
            }
        }

        // next reloc entry
        fixrec = reinterpret_cast<pe::RelocData*>(reinterpret_cast<uintptr_t>(fixrec) + fixrec->BlockSize);
    }

    // Apply relocations, skip header
    if (pImage->flags & HideVAD)
        status = Driver().WriteMem( _process.pid(), pImage->ldrEntry.baseAddress + 0x1000, pImage->ldrEntry.size - 0x1000, pLocal + 0x1000 );
    else
        status = _process.memory().Write( pImage->ldrEntry.baseAddress + 0x1000, pImage->ldrEntry.size - 0x1000, pLocal + 0x1000 );

    if (!NT_SUCCESS( status ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to apply relocations. Status = 0x%x", status );
    }

    return status;
}

/// <summary>
/// Return existing or load missing dependency
/// </summary>
/// <param name="pImage">Currently napped image data</param>
/// <param name="path">Dependency path</param>
/// <returns></returns>
call_result_t<ModuleDataPtr> MMap::FindOrMapDependency( ImageContextPtr pImage, std::wstring& path )
{
    // Already loaded
    auto hMod = _process.modules().GetModule( path, LdrList, pImage->peImage.mType(), pImage->ldrEntry.fullPath.c_str() );
    if (hMod)
        return hMod;

    BLACKBONE_TRACE( L"ManualMap: Loading new dependency '%ls'", path.c_str() );

    auto flags = NameResolve::EnsureFullPath;

    // Wow64 fs redirection
    if (pImage->ldrEntry.type == mt_mod32 && !_process.barrier().sourceWow64)
        flags = static_cast<NameResolve::eResolveFlag>(static_cast<int32_t>(flags) | NameResolve::Wow64);

    auto basedir = pImage->peImage.noPhysFile() ? Utils::GetExeDirectory() : Utils::GetParent( pImage->ldrEntry.fullPath );
    auto status = NameResolve::Instance().ResolvePath( 
        path,
        pImage->ldrEntry.name, 
        basedir, 
        flags, 
        _process, 
        pImage->peImage.actx() 
    );

    // Do remote SxS probe
    if (status == STATUS_SXS_IDENTITIES_DIFFERENT)
    {
        status = ProbeRemoteSxS( path );
    }

    if (!NT_SUCCESS( status ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to resolve dependency path '%ls', status 0x%x", path.c_str(), status );
        return status;
    }

    BLACKBONE_TRACE( L"ManualMap: Dependency path resolved to '%ls'", path.c_str() );

    LoadData data;
    if (_mapCallback != nullptr)
    {
        ModuleData tmpData;
        tmpData.baseAddress = 0;
        tmpData.manual = ((pImage->flags & ManualImports) != 0);
        tmpData.fullPath = path;
        tmpData.name = Utils::ToLower( Utils::StripPath( path ) );
        tmpData.size = 0;
        tmpData.type = pImage->ldrEntry.type;

        data = _mapCallback( PreCallback, _userContext, _process, tmpData );
    }

    // Loading method
    if (data.mtype == MT_Manual || (data.mtype == MT_Default && pImage->flags & ManualImports))
    {
        return FindOrMapModule( path, nullptr, 0, false, pImage->flags | NoSxS | NoDelayLoad | PartialExcept | IsDependency );
    }
    else if (data.mtype != MT_None)
    {
        return _process.modules().Inject( path );
    }
    // Aborted by user
    else
    {
        return STATUS_REQUEST_CANCELED;
    }
};

/// <summary>
/// Resolves image import or delayed image import
/// </summary>
/// <param name="pImage">Image data</param>
/// <param name="useDelayed">Resolve delayed import instead</param>
/// <returns>Status code</returns>
NTSTATUS MMap::ResolveImport( ImageContextPtr pImage, bool useDelayed /*= false */ )
{
    auto imports = pImage->peImage.GetImports( useDelayed );
    if (imports.empty())
        return STATUS_SUCCESS;

    // Read whole image to process it locally
    std::unique_ptr<uint8_t[]> localImage( new uint8_t[pImage->ldrEntry.size] );
    auto pLocal = localImage.get();
    _process.memory().Read( pImage->imgMem.ptr(), pImage->ldrEntry.size, pLocal );

    // Traverse entries
    for (auto& importMod : imports)
    {
        std::wstring wstrDll = importMod.first;

        // Load dependency if needed
        auto hMod = FindOrMapDependency( pImage, wstrDll );
        if (!hMod)
        {
            BLACKBONE_TRACE( L"ManualMap: Failed to load dependency '%ls'. Status 0x%x", wstrDll.c_str(), hMod.status );
            return hMod.status;
        }

        for (auto& importFn : importMod.second)
        {
            call_result_t<exportData> expData;

            if (importFn.importByOrd)
                expData = _process.modules().GetExport( hMod.result(), reinterpret_cast<const char*>(importFn.importOrdinal) );
            else
                expData = _process.modules().GetExport( hMod.result(), importFn.importName.c_str() );

            // Still forwarded, load missing modules
            while (expData && expData->procAddress && expData->isForwarded)
            {
                std::wstring wdllpath = expData->forwardModule;

                // Ensure module is loaded
                auto hFwdMod = FindOrMapDependency( pImage, wdllpath );
                if (!hFwdMod)
                {
                    BLACKBONE_TRACE( L"ManualMap: Failed to load forwarded dependency '%ls'. Status 0x%x", wstrDll.c_str(), hFwdMod.status );
                    return hFwdMod.status;
                }

                if (expData->forwardByOrd)
                {
                    expData = _process.modules().GetExport(
                        hFwdMod.result(),
                        reinterpret_cast<const char*>(expData->forwardOrdinal),
                        wdllpath.c_str()
                    );
                }
                else
                    expData = _process.modules().GetExport( hFwdMod.result(), expData->forwardName.c_str(), wdllpath.c_str() );
            }

            // Failed to resolve import
            if (!expData)
            {
                if (importFn.importByOrd)
                {
                    BLACKBONE_TRACE(
                        L"ManualMap: Failed to get import #%d from image '%ls'",
                        importFn.importOrdinal,
                        wstrDll.c_str()
                    );
                }
                else
                {
                    BLACKBONE_TRACE(
                        L"ManualMap: Failed to get import '%ls' from image '%ls'",
                        Utils::AnsiToWstring( importFn.importName ).c_str(),
                        wstrDll.c_str()
                    );
                }

                return expData.status;
            }

            if (pImage->ldrEntry.type == mt_mod64)
                *reinterpret_cast<uint64_t*>(pLocal + importFn.ptrRVA) = expData->procAddress;
            else
                *reinterpret_cast<uint32_t*>(pLocal + importFn.ptrRVA) = static_cast<uint32_t>(expData->procAddress);
        }
    }

    auto status = STATUS_SUCCESS;

    // Apply imports, skip header
    if (pImage->flags & HideVAD)
    {
        status = Driver().WriteMem(
            _process.pid(),
            pImage->ldrEntry.baseAddress + 0x1000,
            pImage->ldrEntry.size - 0x1000,
            pLocal + 0x1000
        );
    }
    else
    {
        status = _process.memory().Write(
            pImage->ldrEntry.baseAddress + 0x1000,
            pImage->ldrEntry.size - 0x1000,
            pLocal + 0x1000
        );
    }

    // Write function address
    if (!NT_SUCCESS( status ))
    {
        BLACKBONE_TRACE( L"ManualMap: Failed to write import function. Status 0x%x", status );
    }

    return status;
}

/// <summary>
/// Set custom exception handler to bypass SafeSEH under DEP 
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
NTSTATUS MMap::EnableExceptions( ImageContextPtr pImage )
{
    BLACKBONE_TRACE( L"ManualMap: Enabling exception support for image '%ls'", pImage->ldrEntry.name.c_str() );
    bool partial = (pImage->flags & PartialExcept) != 0;
    bool success = _process.nativeLdr().InsertInvertedFunctionTable( pImage->ldrEntry );

    if (pImage->ldrEntry.type == mt_mod64)
    {
        // Try RtlIsertInvertedTable
        if (!success)
        {
            // Retry with documented method
            auto expTableRVA = pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_EXCEPTION, pe::RVA );
            size_t size = pImage->peImage.DirectorySize( IMAGE_DIRECTORY_ENTRY_EXCEPTION );

            // Invoke RtlAddFunctionTable
            if (expTableRVA)
            {
                auto a = AsmFactory::GetAssembler( pImage->ldrEntry.type );
                uint64_t result = 0;

                pImage->pExpTableAddr = expTableRVA + pImage->imgMem.ptr<ptr_t>();
                auto pAddTable = _process.modules().GetNtdllExport( "RtlAddFunctionTable", pImage->ldrEntry.type );
                if (!pAddTable)
                    return pAddTable.status;

                a->GenPrologue();
                a->GenCall(
                    pAddTable->procAddress, {
                    pImage->pExpTableAddr,
                    size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY ),
                    pImage->imgMem.ptr() }
                );

                _process.remote().AddReturnWithEvent( *a, pImage->ldrEntry.type );
                a->GenEpilogue();

                auto status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
                if (!NT_SUCCESS( status ))
                    return status;
            }
            // No exception table
            else
                return STATUS_NOT_FOUND;
        }
    }
    else if (!success)
        return STATUS_INVALID_EXCEPTION_HANDLER;

    // Custom handler not required
    if (pImage->ldrEntry.safeSEH || (pImage->ldrEntry.type == mt_mod64 && (pImage->flags & CreateLdrRef || success)))
        return STATUS_SUCCESS;

    return _expMgr.CreateVEH( _process, pImage->ldrEntry, partial );
}

/// <summary>
/// Remove custom exception handler
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>true on success</returns>
NTSTATUS MMap::DisableExceptions( ImageContextPtr pImage )
{
    BLACKBONE_TRACE( L"ManualMap: Disabling exception support for image '%ls'", pImage->ldrEntry.name.c_str() );
    bool partial = false;

    if (pImage->ldrEntry.type == mt_mod64)
    {
        if (!pImage->pExpTableAddr)
            return STATUS_NOT_FOUND;

        auto a = AsmFactory::GetAssembler( pImage->ldrEntry.type );
        uint64_t result = 0;

        auto pRemoveTable = _process.modules().GetNtdllExport( "RtlDeleteFunctionTable", pImage->ldrEntry.type );
        if (!pRemoveTable)
            return pRemoveTable.status;

        a->GenPrologue();
        // RtlDeleteFunctionTable(pExpTable);
        a->GenCall(pRemoveTable->procAddress, { pImage->pExpTableAddr } );
        _process.remote().AddReturnWithEvent( *a );
        a->GenEpilogue();

        auto status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
        if (!NT_SUCCESS( status ))
            return status;
    }

    partial = (pImage->flags & PartialExcept) != 0;
    return _expMgr.RemoveVEH( _process, partial, pImage->peImage.mType() );
}

/// <summary>
/// Resolve static TLS storage
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>Status code</returns>
NTSTATUS MMap::InitStaticTLS( ImageContextPtr pImage )
{
    auto pTls = reinterpret_cast<PIMAGE_TLS_DIRECTORY>(pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_TLS ));
    auto rebasedTlsPtr = REBASE( pTls, pImage->peImage.base(), pImage->imgMem.ptr() );

    // Use native TLS initialization
    if (pTls && pTls->AddressOfIndex)
    {
        BLACKBONE_TRACE( L"ManualMap: Performing static TLS initialization for image '%ls'", pImage->ldrEntry.name.c_str() );
        return _process.nativeLdr().AddStaticTLSEntry( pImage->ldrEntry, rebasedTlsPtr );
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Calculate and set security cookie
/// </summary>
/// <param name="pImage">image data</param>
/// <returns>Status code</returns>
NTSTATUS MMap::InitializeCookie( ImageContextPtr pImage )
{
    auto pLoadConfig32 = reinterpret_cast<PIMAGE_LOAD_CONFIG_DIRECTORY32>(pImage->peImage.DirectoryAddress( IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ));
    auto pLoadConfig64 = reinterpret_cast<PIMAGE_LOAD_CONFIG_DIRECTORY64>(pLoadConfig32);
    if (!pLoadConfig32)
        return STATUS_SUCCESS;

    ptr_t pCookie = pLoadConfig32->SecurityCookie;
    if (pImage->ldrEntry.type == mt_mod64)
        pCookie = pLoadConfig64->SecurityCookie;

    if (!pCookie)
        return STATUS_SUCCESS;

    //
    // Cookie generation based on MSVC++ compiler
    //
    BLACKBONE_TRACE( L"ManualMap: Performing security cookie initializtion for image '%ls'", pImage->ldrEntry.name.c_str() );

    FILETIME systime = { 0 };
    LARGE_INTEGER PerformanceCount = { { 0 } };
    size_t size = sizeof( uint32_t );

    GetSystemTimeAsFileTime( &systime );
    QueryPerformanceCounter( &PerformanceCount );

    ptr_t cookie = _process.pid() ^ _process.remote().getExecThread()->id() ^ reinterpret_cast<uintptr_t>(&cookie);

    if (pImage->ldrEntry.type == mt_mod64)
    {
        size = sizeof( uint64_t );

        cookie ^= *reinterpret_cast<uint64_t*>(&systime);
        cookie ^= (PerformanceCount.QuadPart << 32) ^ PerformanceCount.QuadPart;
        cookie &= 0xFFFFFFFFFFFF;

        if (cookie == 0x2B992DDFA232)
            cookie++;
    }
    else
    {
        cookie &= 0xFFFFFFFF;
        cookie ^= systime.dwHighDateTime ^ systime.dwLowDateTime;
        cookie ^= PerformanceCount.LowPart;
        cookie ^= PerformanceCount.HighPart;

        if (cookie == 0xBB40E64E)
            cookie++;
        else if (!(cookie & 0xFFFF0000))
            cookie |= (cookie | 0x4711) << 16;
    }

    return _process.memory().Write( REBASE( pCookie, pImage->peImage.imageBase(), pImage->imgMem.ptr() ), size, &cookie );
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
/// <returns>DllMain result</returns>
call_result_t<uint64_t> MMap::RunModuleInitializers( ImageContextPtr pImage, DWORD dwReason, CustomArgs_t* pCustomArgs /*= nullptr*/ )
{
    auto a = AsmFactory::GetAssembler( pImage->ldrEntry.type );
    uint64_t result = 0;

    auto hNtdll = _process.modules().GetModule( L"ntdll.dll", LdrList, pImage->peImage.mType() );
    auto pActivateActx = _process.modules().GetExport( hNtdll, "RtlActivateActivationContext" );
    auto pDeactivateActx = _process.modules().GetExport( hNtdll, "RtlDeactivateActivationContext" );

    a->GenPrologue();

    // ActivateActCtx
    if (_pAContext.valid() && pActivateActx)
    {
        (*a)->mov( (*a)->zax, _pAContext.ptr() );
        (*a)->mov( (*a)->zax, asmjit::host::dword_ptr( (*a)->zax ) );
        a->GenCall( pActivateActx->procAddress, { 0, (*a)->zax, _pAContext.ptr() + sizeof( ptr_t ) } );
    }

    // Prepare custom arguments
    ptr_t customArgumentsAddress = 0;
    if (pCustomArgs)
    {
        auto memBuf = _process.memory().Allocate( pCustomArgs->size() + sizeof( uint64_t ), PAGE_EXECUTE_READWRITE, 0, false );
        if (!memBuf)
            return memBuf.status;

        memBuf->Write( 0, pCustomArgs->size() );
        memBuf->Write( sizeof( uint64_t ), pCustomArgs->size(), pCustomArgs->data() );
        customArgumentsAddress = memBuf->ptr();
    }

    // Function order
    // TLS first, entry point last
    if (!(pImage->flags & NoTLS))
    {
        // PTLS_CALLBACK_FUNCTION(pImage->ImageBase, dwReason, NULL);
        for (auto& pCallback : pImage->tlsCallbacks)
        {
            BLACKBONE_TRACE( 
                L"ManualMap: Calling TLS callback at 0x%016llx for '%ls', Reason: %d",
                pCallback, pImage->ldrEntry.name.c_str(), dwReason 
            );

            a->GenCall( pCallback, { pImage->imgMem.ptr(), dwReason, customArgumentsAddress } );
        }
    }

    // DllMain
    if (pImage->ldrEntry.entryPoint != 0)
    {
        BLACKBONE_TRACE( L"ManualMap: Calling entry point for '%ls', Reason: %d", pImage->ldrEntry.name.c_str(), dwReason );
        a->GenCall( pImage->ldrEntry.entryPoint, { pImage->imgMem.ptr(), dwReason, customArgumentsAddress } );
        _process.remote().SaveCallResult( *a );
    }

    // DeactivateActCtx
    if (_pAContext.valid() && pDeactivateActx)
    {
        (*a)->mov( (*a)->zax, _pAContext.ptr() + sizeof( ptr_t ) );
        (*a)->mov( (*a)->zax, asmjit::host::dword_ptr( (*a)->zax ) );
        a->GenCall( pDeactivateActx->procAddress, { 0, (*a)->zax } );
    }

    // Set invalid return code offset to preserve one from DllMain
    _process.remote().AddReturnWithEvent( *a, pImage->ldrEntry.type, rt_int32, ARGS_OFFSET );
    a->GenEpilogue();

    NTSTATUS status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
    if (!NT_SUCCESS( status ))
        return status;

    if (pImage->ldrEntry.entryPoint == 0)
        return call_result_t<uint64_t>( ERROR_SUCCESS, STATUS_SUCCESS );
    
    BLACKBONE_TRACE( L"ManualMap: DllMain of '%ls' returned %lld", pImage->ldrEntry.name.c_str(), result );
    return result;
}


/// <summary>
/// Create activation context
/// Target memory layout:
/// -----------------------------
/// | hCtx | ACTCTX | file_path |
/// -----------------------------
/// </summary>
/// <param name="image">Source umage</param>
/// <returns>true on success</returns>
NTSTATUS MMap::CreateActx( const pe::PEImage& image  )
{   
    auto a = AsmFactory::GetAssembler( image.mType() );

    NTSTATUS status = STATUS_SUCCESS;
    uint64_t result = 0;

    auto mem = _process.memory().Allocate( 512, PAGE_READWRITE );
    if (!mem)
        return mem.status;

    _pAContext = std::move( mem.result() );
    
    bool switchMode = image.mType() == mt_mod64 && _process.core().isWow64();
    auto kernel32 = _process.modules().GetModule( L"kernel32.dll" );
    if (!kernel32)
    {
        BLACKBONE_TRACE( 
            L"ManualMap: Failed to get kernel32 base, error 0x%08x. Possibly LDR is not available yet", 
            LastNtStatus() 
        );
        return LastNtStatus();
    }

    auto pCreateActx = _process.modules().GetExport( kernel32, "CreateActCtxW" );
    if (!pCreateActx)
    {
        BLACKBONE_TRACE( 
            L"ManualMap: Failed to create activation context for image '%ls'. 'CreateActCtxW' is absent", 
            image.manifestFile().c_str() 
        );
        return STATUS_ORDINAL_NOT_FOUND;
    }

    // CreateActCtx(&act)
    // Emulate Wow64
    if (switchMode)
    {
        _ACTCTXW_T<uint32_t> act32 = { 0 };

        act32.cbSize = sizeof(act32);
        act32.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act32.lpSource = _pAContext.ptr<uint32_t>() + sizeof( ptr_t ) + sizeof( act32 );
        act32.lpResourceName = image.manifestID();

        (*a)->push( _pAContext.ptr<uint32_t>() + static_cast<uint32_t>(sizeof( ptr_t )) );
        (*a)->mov( asmjit::host::eax, static_cast<uint32_t>(pCreateActx->procAddress) );
        (*a)->call( (*a)->zax );
        (*a)->mov( asmjit::host::edx, _pAContext.ptr<uint32_t>() );
        //a->mov( asmjit::host::dword_ptr( asmjit::host::edx ), asmjit::host::eax );
        (*a)->dw( '\x01\x02' );

        auto pTermThd = _process.modules().GetNtdllExport( "NtTerminateThread", mt_mod32 );
        (*a)->push( (*a)->zax );
        (*a)->push( uint32_t( 0 ) );
        (*a)->mov( asmjit::host::eax, static_cast<uint32_t>(pTermThd->procAddress) );
        (*a)->call( (*a)->zax );
        (*a)->ret( 4 );
        
        // Write path to file
        _pAContext.Write( sizeof( ptr_t ), act32 );
        _pAContext.Write( 
            sizeof( ptr_t ) + sizeof( act32 ), 
            (image.manifestFile().length() + 1) * sizeof( wchar_t ),
            image.manifestFile().c_str()
        );

        auto pCode = _process.memory().Allocate( 0x1000 );
        if (!pCode)
            return pCode.status;

        pCode->Write( 0, (*a)->getCodeSize(), (*a)->make() );

        result = _process.remote().ExecDirect( pCode->ptr<ptr_t>(), _pAContext.ptr<size_t>() + sizeof( ptr_t ) );
    }
    // Native way
    else
    {
        auto fillACTX = [this, &image]( auto act )
        {
            memset( &act, 0, sizeof( act ) );

            act.cbSize = sizeof( act );
            act.lpSource = static_cast<decltype(act.lpSource)>(this->_pAContext.ptr() + sizeof( ptr_t ) + sizeof( act ));

            // Ignore some fields for pure manifest file
            if (!image.noPhysFile())
            {
                act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
                act.lpResourceName = static_cast<decltype(act.lpResourceName)>(image.manifestID());
            }

            // Write path to file
            NTSTATUS status = this->_pAContext.Write( sizeof( ptr_t ), act );
            status |= this->_pAContext.Write(
                sizeof( ptr_t ) + sizeof( act ),
                (image.manifestFile().length() + 1) * sizeof( wchar_t ),
                image.manifestFile().c_str()
            );

            return status;
        };

        if (_process.core().isWow64())
            status = fillACTX( _ACTCTXW32() );
        else
            status = fillACTX( _ACTCTXW64() );

        a->GenPrologue();
        a->GenCall( pCreateActx->procAddress, { _pAContext.ptr() + sizeof( ptr_t ) } );

        (*a)->mov( (*a)->zdx, _pAContext.ptr() );
        (*a)->mov( (*a)->intptr_ptr( (*a)->zdx ), (*a)->zax );

        _process.remote().AddReturnWithEvent( *a, image.mType() );
        a->GenEpilogue();

        status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
        if (!NT_SUCCESS( status ))
            return status;
    }


    if (reinterpret_cast<HANDLE>(result) == INVALID_HANDLE_VALUE)
    {
        _pAContext.Free();

        status = _process.remote().GetLastStatus();
        BLACKBONE_TRACE( L"ManualMap: Failed to create activation context for image '%ls'. Status: 0x%x", image.manifestFile().c_str(), status );
        return status;
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Do SxS path probing in the target process
/// </summary>
/// <param name="path">Path to probe</param>
/// <returns>Status code</returns>
NTSTATUS MMap::ProbeRemoteSxS( std::wstring& path )
{
    NTSTATUS status = STATUS_SUCCESS;

    constexpr uint32_t memSize    = 0x1000;
    constexpr uint32_t dll1Offset = 0x300;
    constexpr uint32_t dll2Offset = 0x600;
    constexpr uint32_t pathOffset = 0x800;
    constexpr uint32_t strOffset  = 0xA00;
    constexpr uint16_t strSize    = 0x200;

    // No underlying function
    auto ProbeFn = _process.modules().GetNtdllExport( "RtlDosApplyFileIsolationRedirection_Ustr" );
    if (!ProbeFn)
        return ProbeFn.status;

    auto actx = _pAContext.ptr();
    auto pActivateActx = _process.modules().GetNtdllExport( "RtlActivateActivationContext" );
    auto pDeactivateActx = _process.modules().GetNtdllExport( "RtlDeactivateActivationContext" );
    auto pAsm = AsmFactory::GetAssembler( _process.barrier().targetWow64 );
    auto& a = *pAsm.get();

    // REmote buffer
    auto memBuf = _process.memory().Allocate( memSize, PAGE_READWRITE );
    if (!memBuf)
        return memBuf.status;

    // Fill Unicode strings
    auto memPtr = memBuf->ptr();
    auto fillStr = [&]( auto&& OriginalName )
    {
        std::remove_reference_t<decltype(OriginalName)> DllName1 = { 0 };

        OriginalName.Length = static_cast<uint16_t>(path.length() * sizeof( wchar_t ));
        OriginalName.MaximumLength = OriginalName.Length;
        OriginalName.Buffer = static_cast<decltype(OriginalName.Buffer)>(memPtr + sizeof( OriginalName ));

        auto status = _process.memory().Write( memPtr, OriginalName );
        status = _process.memory().Write( memPtr + sizeof( OriginalName ), OriginalName.Length + 2, path.c_str() );
        if (!NT_SUCCESS( status ))
            return status;

        DllName1.Length = 0;
        DllName1.MaximumLength = strSize;
        DllName1.Buffer = static_cast<decltype(DllName1.Buffer)>(memPtr + strOffset);

        return _process.memory().Write( memPtr + dll1Offset, DllName1 );
    };

    if (_process.barrier().targetWow64)
        status = fillStr( _UNICODE_STRING_T<uint32_t>() );
    else
        status = fillStr( _UNICODE_STRING_T<uint64_t>() );

    if (!NT_SUCCESS( status ))
        return status;

    a.GenPrologue();

    // ActivateActCtx
    if (actx && pActivateActx)
    {
        a->mov( a->zax, actx );
        a->mov( a->zax, asmjit::host::dword_ptr( a->zax ) );
        a.GenCall( pActivateActx->procAddress, { 0, a->zax, actx + sizeof( ptr_t ) } );
    }

    // RtlDosApplyFileIsolationRedirection_Ustr
    a.GenCall( ProbeFn->procAddress,
    {
        TRUE,
        memPtr + 0, 0,
        memPtr + dll1Offset,
        memPtr + dll2Offset,
        memPtr + pathOffset,
        0, 0, 0
    } );

    _process.remote().SaveCallResult( a );

    // DeactivateActCtx
    if (actx && pDeactivateActx)
    {
        a->mov( a->zax, actx + sizeof( ptr_t ) );
        a->mov( a->zax, asmjit::host::dword_ptr( a->zax ) );
        a.GenCall( pDeactivateActx->procAddress, { 0, a->zax } );
    }

    _process.remote().AddReturnWithEvent( a, mt_default, rt_int32, ARGS_OFFSET );
    a.GenEpilogue();

    uint64_t result = 0;
    if (!NT_SUCCESS( status = _process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result ) ))
        return status;

    status = static_cast<NTSTATUS>(result);
    if (NT_SUCCESS( status ))
    {
        // Read result back
        std::unique_ptr<uint8_t[]> localBuf( new uint8_t[memSize] );
        if (NT_SUCCESS( status = memBuf->Read( 0, memSize, localBuf.get() ) ))
            path = reinterpret_cast<wchar_t*>(localBuf.get() + strOffset);
    }

    return status;
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
        _usedBlocks.emplace_back( ptr, size  );

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
    _expMgr.reset();
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
