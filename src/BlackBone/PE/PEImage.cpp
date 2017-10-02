#include "../PE/PEImage.h"
#include "../Include/Macro.h"
#include "../Misc/Utils.h"
#include "../Misc/DynImport.h"

#include <algorithm>

#define TLS32(ptr) ((const IMAGE_TLS_DIRECTORY32*)ptr)  // TLS directory
#define TLS64(ptr) ((const IMAGE_TLS_DIRECTORY64*)ptr)  // TLS directory
#define THK32(ptr) ((const IMAGE_THUNK_DATA32*)ptr)     // Import thunk data
#define THK64(ptr) ((const IMAGE_THUNK_DATA64*)ptr)     // Import thunk data

namespace blackbone
{

namespace pe
{

PEImage::PEImage( void )
{
}

PEImage::~PEImage( void )
{
    Release();
}

/// <summary>
/// Load image from file
/// </summary>
/// <param name="path">File path</param>
/// <param name="skipActx">If true - do not initialize activation context</param>
/// <returns>Status code</returns>
NTSTATUS PEImage::Load( const std::wstring& path, bool skipActx /*= false*/ )
{
    Release( true );
    _imagePath = path;
    _noFile = false;

    _hFile = CreateFileW(
        path.c_str(), FILE_GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL
        );

    if (_hFile)
    {
        // Try mapping as image
        _hMapping = CreateFileMappingW( _hFile, NULL, SEC_IMAGE | PAGE_READONLY, 0, 0, NULL );
        if (_hMapping)
        {
            _isPlainData = false;
            _pFileBase = MapViewOfFile( _hMapping, FILE_MAP_READ, 0, 0, 0 );
        }
        // Map as simple datafile
        else
        {
            _isPlainData = true;
            _hMapping = CreateFileMappingW( _hFile, NULL, PAGE_READONLY, 0, 0, NULL );

            if (_hMapping)
                _pFileBase = MapViewOfFile( _hMapping, FILE_MAP_READ, 0, 0, 0 );
        }

        // Mapping failed
        if (!_pFileBase)
            return LastNtStatus();
    }
    else
        return LastNtStatus();

    auto status = Parse();
    if (!NT_SUCCESS( status ))
        return status;

    return skipActx ? status : PrepareACTX( _imagePath.c_str() );
}

/// <summary>
/// Load image from memory location
/// </summary>
/// <param name="pData">Image data</param>
/// <param name="size">Data size.</param>
/// <param name="plainData">If false - data has image layout</param>
/// <returns>Status code</returns>
NTSTATUS PEImage::Load( void* pData, size_t size, bool plainData /*= true */ )
{
    Release( true );

    _noFile = true;
    _pFileBase = pData;
    _isPlainData = plainData;

    auto status = Parse();
    if (!NT_SUCCESS( status ))
        return status;

    return PrepareACTX();
}

/// <summary>
/// Reload closed image
/// </summary>
/// <returns>Status code</returns>
NTSTATUS PEImage::Reload()
{
    return Load( _imagePath );
}

/// <summary>
/// Release mapping, if any
/// </summary>
/// <param name="temporary">Preserve file paths for file reopening</param>
void PEImage::Release( bool temporary /*= false*/ )
{
    if (_pFileBase)
    {
        UnmapViewOfFile( _pFileBase );
        _pFileBase = nullptr;
    }

    _hMapping.reset();
    _hFile.reset();
    _hctx.reset();

    // Reset pointers to data
    _pImageHdr32 = nullptr;
    _pImageHdr64 = nullptr;

    if(!temporary)
    {
        _imagePath.clear();

        // Ensure temporary file is deleted
        if (_noFile)
            DeleteFileW( _manifestPath.c_str() );

        _manifestPath.clear();
    }
}

/// <summary>
/// Parses PE image
/// </summary>
/// <returns>Status code</returns>
NTSTATUS PEImage::Parse( void* pImageBase /*= nullptr*/ )
{
    const IMAGE_DOS_HEADER *pDosHdr = nullptr;
    const IMAGE_SECTION_HEADER *pSection = nullptr;

    if (pImageBase != nullptr)
        _pFileBase = pImageBase;

    // Something went wrong
    if (!_pFileBase)
        return STATUS_INVALID_ADDRESS;

    // Get DOS header
    pDosHdr = reinterpret_cast<const IMAGE_DOS_HEADER*>(_pFileBase);

    // File not a valid PE file
    if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;

    // Get image header
    _pImageHdr32 = reinterpret_cast<PCHDR32>(reinterpret_cast<const uint8_t*>(pDosHdr) + pDosHdr->e_lfanew);
    _pImageHdr64 = reinterpret_cast<PCHDR64>(_pImageHdr32);

    // File not a valid PE file
    if (_pImageHdr32->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;

    auto GetHeaderData = [this, &pSection]( auto pImageHeader )
    {
        _imgBase = pImageHeader->OptionalHeader.ImageBase;
        _imgSize = pImageHeader->OptionalHeader.SizeOfImage;
        _hdrSize = pImageHeader->OptionalHeader.SizeOfHeaders;
        _epRVA = pImageHeader->OptionalHeader.AddressOfEntryPoint;
        _subsystem = pImageHeader->OptionalHeader.Subsystem;
        _DllCharacteristics = pImageHeader->OptionalHeader.DllCharacteristics;

        pSection = reinterpret_cast<const IMAGE_SECTION_HEADER*>(pImageHeader + 1);
    };

    // Detect x64 image
    if (_pImageHdr32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        _is64 = true;
        GetHeaderData( _pImageHdr64 );
    }
    else
    {
        _is64 = false;
        GetHeaderData( _pImageHdr32 );
    }

    // Exe file
    _isExe = !(_pImageHdr32->FileHeader.Characteristics & IMAGE_FILE_DLL);

    // Pure IL image
    auto pCorHdr = reinterpret_cast<PIMAGE_COR20_HEADER>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR ));

    _isPureIL = (pCorHdr && (pCorHdr->Flags & COMIMAGE_FLAGS_ILONLY)) ? true : false;

    if (_isPureIL)
    {
        _ILFlagOffset = static_cast<int32_t>(
            reinterpret_cast<uint8_t*>(pCorHdr)
            - reinterpret_cast<uint8_t*>(_pFileBase)
            + static_cast<int32_t>(offsetof( IMAGE_COR20_HEADER, Flags )));

#ifdef COMPILER_MSVC
        if (_netImage.Init( _imagePath ))
            _netImage.Parse();
#endif
    }

    // Sections
    for (int i = 0; i < _pImageHdr32->FileHeader.NumberOfSections; ++i, ++pSection)
        _sections.emplace_back( *pSection );

    return STATUS_SUCCESS;
}

/// <summary>
/// Processes image imports
/// </summary>
/// <param name="useDelayed">Process delayed import instead</param>
/// <returns>Import data</returns>
mapImports& PEImage::GetImports( bool useDelayed /*= false*/ )
{
    if(useDelayed)
    {
        auto pImportTbl = reinterpret_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT ));
        if (!pImportTbl)
            return _delayImports;

        // Delayed Imports
        for (; pImportTbl->DllNameRVA; ++pImportTbl)
        {
            uint8_t* pRVA = nullptr;
            DWORD IAT_Index = 0;
            char *pDllName = reinterpret_cast<char*>(ResolveRVAToVA( pImportTbl->DllNameRVA ));
            auto dllStr = Utils::AnsiToWstring( pDllName );

            pRVA = reinterpret_cast<uint8_t*>(ResolveRVAToVA( pImportTbl->ImportNameTableRVA ));

            while (_is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData)
            {
                uint64_t AddressOfData = _is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData;
                auto pAddressTable = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(ResolveRVAToVA( static_cast<uintptr_t>(AddressOfData) ));
                ImportData data;

                // import by name
                if (AddressOfData < (_is64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) && pAddressTable->Name[0])
                {
                    data.importByOrd = false;
                    data.importName = reinterpret_cast<const char*>(pAddressTable->Name);
                    data.importOrdinal = 0;
                }
                // import by ordinal
                else
                {
                    data.importByOrd = true;
                    data.importOrdinal = static_cast<WORD>(AddressOfData & 0xFFFF);
                }

                data.ptrRVA = pImportTbl->ImportAddressTableRVA + IAT_Index;

                _delayImports[dllStr].emplace_back( data );

                // Go to next entry
                pRVA += _is64 ? sizeof(IMAGE_THUNK_DATA64) : sizeof(IMAGE_THUNK_DATA32);
                IAT_Index += _is64 ? sizeof(uint64_t) : sizeof(uint32_t);
            }
        }

        return _delayImports;
    }
    else
    {
        auto *pImportTbl = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_IMPORT ));
        if (!pImportTbl)
            return _imports;

        // Imports
        for (; pImportTbl->Name; ++pImportTbl)
        {
            uint8_t* pRVA = nullptr;
            DWORD IAT_Index = 0;
            char *pDllName = reinterpret_cast<char*>(ResolveRVAToVA( pImportTbl->Name ));
            auto dllStr = Utils::AnsiToWstring( pDllName );

            pRVA = reinterpret_cast<uint8_t*>(ResolveRVAToVA( 
                pImportTbl->OriginalFirstThunk ? 
                pImportTbl->OriginalFirstThunk : 
                pImportTbl->FirstThunk ));

            while (_is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData)
            {
                uint64_t AddressOfData = _is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData;
                
                auto* pAddressTable = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(ResolveRVAToVA( static_cast<uintptr_t>(AddressOfData) ));
                ImportData data;

                // import by name
                if (AddressOfData < (_is64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) && pAddressTable->Name[0])
                {
                    data.importByOrd   = false;
                    data.importName    = reinterpret_cast<const char*>(pAddressTable->Name);
                    data.importOrdinal = 0;
                }
                // import by ordinal
                else
                {
                    data.importByOrd   = true;
                    data.importOrdinal = static_cast<WORD>(AddressOfData & 0xFFFF);
                }

                // Save address to IAT
                if (pImportTbl->FirstThunk)
                    data.ptrRVA = pImportTbl->FirstThunk + IAT_Index;
                // Save address to OrigianlFirstThunk
                else
                    data.ptrRVA = static_cast<uintptr_t>(AddressOfData) - reinterpret_cast<uintptr_t>(_pFileBase);

                _imports[dllStr].emplace_back( data );

                // Go to next entry
                pRVA += _is64 ? sizeof(IMAGE_THUNK_DATA64) : sizeof(IMAGE_THUNK_DATA32);
                IAT_Index += _is64 ? sizeof(uint64_t) : sizeof(uint32_t);
            }
        }

        return _imports;

    }
}

/// <summary>
/// Retrieve all exported functions with names
/// </summary>
/// <param name="names">Found exports</param>
void PEImage::GetExports( vecExports& exports )
{
    exports.clear();
    Reload();

    auto pExport = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_EXPORT ));
    if (pExport == 0)
        return;

    DWORD *pAddressOfNames = reinterpret_cast<DWORD*>(pExport->AddressOfNames + reinterpret_cast<uintptr_t>(_pFileBase));
    DWORD *pAddressOfFuncs = reinterpret_cast<DWORD*>(pExport->AddressOfFunctions + reinterpret_cast<uintptr_t>(_pFileBase));
    WORD  *pAddressOfOrds  = reinterpret_cast<WORD*> (pExport->AddressOfNameOrdinals + reinterpret_cast<size_t>(_pFileBase));

    for (DWORD i = 0; i < pExport->NumberOfNames; ++i)
        exports.push_back( ExportData( reinterpret_cast<const char*>(_pFileBase)+pAddressOfNames[i], pAddressOfFuncs[pAddressOfOrds[i]] ) );

    std::sort( exports.begin(), exports.end() );
    return Release( true );
}

/// <summary>
/// Retrieve data directory address
/// </summary>
/// <param name="index">Directory index</param>
/// <param name="keepRelative">Keep address relative to image base</param>
/// <returns>Directory address</returns>
uintptr_t PEImage::DirectoryAddress( int index, AddressType type /*= VA*/ ) const
{
    // Sanity check
    if (index < 0 || index >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
        return 0;

    const auto idd = _is64 ? _pImageHdr64->OptionalHeader.DataDirectory  : _pImageHdr32->OptionalHeader.DataDirectory;
    return idd[index].VirtualAddress == 0 ? 0 : ResolveRVAToVA( idd[index].VirtualAddress, type );
}

/// <summary>
/// Get data directory size
/// </summary>
/// <param name="index">Data directory index</param>
/// <returns>Data directory size</returns>
size_t PEImage::DirectorySize( int index ) const
{
    // Sanity check
    if (index < 0 || index >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
        return 0;

    const IMAGE_DATA_DIRECTORY* idd = _is64 ? _pImageHdr64->OptionalHeader.DataDirectory : _pImageHdr32->OptionalHeader.DataDirectory;
    return idd[index].VirtualAddress != 0 ? static_cast<size_t>(idd[index].Size) : 0;
}


/// <summary>
/// Resolve virtual memory address to physical file offset
/// </summary>
/// <param name="Rva">Memory address</param>
/// <param name="keepRelative">Keep address relative to file start</param>
/// <returns>Resolved address</returns>
uintptr_t PEImage::ResolveRVAToVA( uintptr_t Rva, AddressType type /*= VA*/ ) const
{
    switch (type)
    {
    case blackbone::pe::RVA:
        return Rva;

    case blackbone::pe::VA:
    case blackbone::pe::RPA:
        if (_isPlainData)
        {
            for (auto& sec : _sections)
            {
                if (Rva >= sec.VirtualAddress && Rva < sec.VirtualAddress + sec.Misc.VirtualSize)
                    if (type == VA)
                        return reinterpret_cast<uintptr_t>(_pFileBase) + Rva - sec.VirtualAddress + sec.PointerToRawData;
                    else
                        return Rva - sec.VirtualAddress + sec.PointerToRawData;
            }

            return 0;
        }
        else
            return (type == VA) ? (reinterpret_cast<uintptr_t>(_pFileBase) + Rva) : Rva;

    default:
        return 0;
    }

}

/// <summary>
/// Retrieve image TLS callbacks
/// Callbacks are rebased for target image
/// </summary>
/// <param name="targetBase">Target image base</param>
/// <param name="result">Found callbacks</param>
/// <returns>Number of TLS callbacks in image</returns>
int PEImage::GetTLSCallbacks( module_t targetBase, std::vector<ptr_t>& result ) const
{
    uint8_t *pTls = reinterpret_cast<uint8_t*>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_TLS ));
    uint64_t* pCallback = 0;
    if (!pTls)
        return 0;

    uint64_t offset = _is64 ? TLS64( pTls )->AddressOfCallBacks : TLS32( pTls )->AddressOfCallBacks;
    if (offset == 0)
        return 0;

    // Not at base
    if (imageBase() != reinterpret_cast<module_t>(_pFileBase))
        pCallback = reinterpret_cast<uint64_t*>(ResolveRVAToVA( static_cast<size_t>(offset - imageBase()) ));
    else
        pCallback = reinterpret_cast<uint64_t*>(offset);

    if(_is64)
    {
        for (; *pCallback; pCallback++)
            result.push_back( REBASE( *pCallback, imageBase(), targetBase ) );
    }
    else
    {
        for (uint32_t *pCallback2 = reinterpret_cast<uint32_t*>(pCallback); *pCallback2; pCallback2++)
            result.push_back( REBASE( *pCallback2, imageBase(), targetBase ) );
    }

    return (int)result.size();
}

/// <summary>
/// Prepare activation context
/// </summary>
/// <param name="filepath">Path to PE file. If nullptr - manifest is extracted from memory to disk</param>
/// <returns>Status code</returns>
NTSTATUS PEImage::PrepareACTX( const wchar_t* filepath /*= nullptr*/ )
{
    wchar_t tempPath[256] = { 0 };
    uint32_t manifestSize = 0;

    ACTCTXW act = { 0 };
    act.cbSize = sizeof( act );

    // No manifest found, skip
    auto pManifest = GetManifest( manifestSize, _manifestIdx );
    if (!pManifest)
        return STATUS_SUCCESS;

    //
    // Dump manifest to TMP folder
    //
    if (filepath == nullptr)
    {
        wchar_t tempDir[256] = { 0 };

        GetTempPathW( ARRAYSIZE( tempDir ), tempDir );
        if (GetTempFileNameW( tempDir, L"ImageManifest", 0, tempPath ) == 0)
            return STATUS_UNSUCCESSFUL;
     
        auto hTmpFile = FileHandle( CreateFileW( tempPath, FILE_GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL ) );
        if (hTmpFile != INVALID_HANDLE_VALUE)
        {
            DWORD bytes = 0;
            WriteFile( hTmpFile, pManifest, manifestSize, &bytes, NULL );
            hTmpFile.reset();

            act.lpSource = tempPath;
            _manifestPath = tempPath;
        }
        else
            return LastNtStatus();
    }
    else
    {
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act.lpResourceName = MAKEINTRESOURCEW( _manifestIdx );
        act.lpSource = filepath;

        _manifestPath = _imagePath;
    }  
   
    // Create ACTX
    _hctx = CreateActCtxW( &act );

    if (_hctx != INVALID_HANDLE_VALUE)
        return STATUS_SUCCESS;

    // Return success if current process is protected
    if (LastNtStatus() == STATUS_ACCESS_DENIED)
    {
        _manifestIdx = 0;
        return STATUS_SUCCESS;
    }

    return LastNtStatus();
}

/// <summary>
/// Get manifest from image data
/// </summary>
/// <param name="size">Manifest size</param>
/// <param name="manifestID">Mmanifest ID</param>
/// <returns>Manifest data</returns>
void* PEImage::GetManifest( uint32_t& size, int32_t& manifestID )
{
    // 3 levels of pointers to nodes
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *pDirNode1 = nullptr;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *pDirNode2 = nullptr;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *pDirNode3 = nullptr;

    // 3 levels of nodes
    const IMAGE_RESOURCE_DIRECTORY       *pDirNodePtr1 = nullptr;
    const IMAGE_RESOURCE_DIRECTORY       *pDirNodePtr2 = nullptr;
    const IMAGE_RESOURCE_DIRECTORY       *pDirNodePtr3 = nullptr;

    // resource entry data
    const IMAGE_RESOURCE_DATA_ENTRY      *pDataNode = nullptr;

    size_t ofst_1 = 0;  // first level nodes offset
    size_t ofst_2 = 0;  // second level nodes offset
    size_t ofst_3 = 0;  // third level nodes offset

    // Get section base
    auto secBase = DirectoryAddress( IMAGE_DIRECTORY_ENTRY_RESOURCE );
    if (secBase == 0)
    {
        size = 0;
        manifestID = 0;
        return nullptr;
    }

    pDirNodePtr1 = reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY*>(secBase);
    ofst_1 += sizeof( IMAGE_RESOURCE_DIRECTORY );

    // first-level nodes
    for (int i = 0; i < pDirNodePtr1->NumberOfIdEntries + pDirNodePtr1->NumberOfNamedEntries; ++i)
    {
        pDirNode1 = reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY_ENTRY*>(secBase + ofst_1);

        // Not a manifest directory
        if (!pDirNode1->DataIsDirectory || pDirNode1->Id != 0x18)
        {
            ofst_1 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
            continue;
        }

        pDirNodePtr2 = reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY*>(secBase + pDirNode1->OffsetToDirectory);
        ofst_2 = pDirNode1->OffsetToDirectory + sizeof( IMAGE_RESOURCE_DIRECTORY );

        // second-level nodes
        for (int j = 0; j < pDirNodePtr2->NumberOfIdEntries + pDirNodePtr2->NumberOfNamedEntries; ++j)
        {
            pDirNode2 = reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY_ENTRY*>(secBase + ofst_2);

            if (!pDirNode2->DataIsDirectory)
            {
                ofst_2 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
                continue;
            }

            // Check if this is a valid manifest resource
            if (pDirNode2->Id == 1 || pDirNode2->Id == 2 || pDirNode2->Id == 3)
            {
                pDirNodePtr3 = reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY*>(secBase + pDirNode2->OffsetToDirectory);
                ofst_3 = pDirNode2->OffsetToDirectory + sizeof( IMAGE_RESOURCE_DIRECTORY );
                pDirNode3 = reinterpret_cast<const IMAGE_RESOURCE_DIRECTORY_ENTRY*>(secBase + ofst_3);
                pDataNode = reinterpret_cast<const IMAGE_RESOURCE_DATA_ENTRY*>(secBase + pDirNode3->OffsetToData);

                manifestID = pDirNode2->Id;
                size = pDataNode->Size;

                return reinterpret_cast<void*>(ResolveRVAToVA( pDataNode->OffsetToData ));
            }

            ofst_2 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
        }

        ofst_1 += sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY );
    }

    return nullptr;
}

}

}
