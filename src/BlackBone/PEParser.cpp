#include "PEParser.h"
#include "Macro.h"
#include "Utils.h"

#define TLS32(ptr) ((const IMAGE_TLS_DIRECTORY32*)ptr)  // TLS directory
#define TLS64(ptr) ((const IMAGE_TLS_DIRECTORY64*)ptr)  // TLS directory
#define THK32(ptr) ((const IMAGE_THUNK_DATA32*)ptr)     // Import thunk data
#define THK64(ptr) ((const IMAGE_THUNK_DATA64*)ptr)     // Import thunk data

namespace blackbone
{

namespace pe
{

PEParser::PEParser( void )
{
}

PEParser::~PEParser( void )
{
}

/// <summary>
/// Parses PE image
/// </summary>
/// <param name="pFileBase">File memory location</param>
/// <param name="isPlainData">Treat file as plain datafile</param>
/// <returns>true on success</returns>
bool PEParser::Parse( const void* pFileBase, bool isPlainData /*= false*/ )
{
    const IMAGE_DOS_HEADER *pDosHdr = nullptr;
    const IMAGE_SECTION_HEADER *pSection = nullptr;

    if (!pFileBase)
    {
        //SetLastError(err::pe::NoFile);
        return false;
    }

    _isPlainData = isPlainData;

    // Get DOS header
    _pFileBase = pFileBase;
    pDosHdr = reinterpret_cast<const IMAGE_DOS_HEADER*>(_pFileBase);

    // File not a valid PE file
    if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        //SetLastError(err::pe::NoSignature);
        return false;
    }

    // Get image header
    _pImageHdr32 = reinterpret_cast<PCHDR32>(reinterpret_cast<const uint8_t*>(pDosHdr) + pDosHdr->e_lfanew);
    _pImageHdr64 = reinterpret_cast<PCHDR64>(_pImageHdr32);

    // File not a valid PE file
    if (_pImageHdr32->Signature != IMAGE_NT_SIGNATURE)
    {
        //SetLastError(err::pe::NoSignature);
        return false;
    }

    // Detect x64 image
    if (_pImageHdr32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        _is64 = true;

        _imgBase = _pImageHdr64->OptionalHeader.ImageBase;
        _imgSize = _pImageHdr64->OptionalHeader.SizeOfImage;
        _hdrSize = _pImageHdr64->OptionalHeader.SizeOfHeaders;
        _epRVA   = _pImageHdr64->OptionalHeader.AddressOfEntryPoint;

        pSection = reinterpret_cast<const IMAGE_SECTION_HEADER*>(_pImageHdr64 + 1);
    }
    else
    {
        _is64 = false;

        _imgBase = _pImageHdr32->OptionalHeader.ImageBase;
        _imgSize = _pImageHdr32->OptionalHeader.SizeOfImage;
        _hdrSize = _pImageHdr32->OptionalHeader.SizeOfHeaders;
        _epRVA   = _pImageHdr32->OptionalHeader.AddressOfEntryPoint;

        pSection = reinterpret_cast<const IMAGE_SECTION_HEADER*>(_pImageHdr32 + 1);
    }

    // Exe file
    _isExe = !(_pImageHdr32->FileHeader.Characteristics & IMAGE_FILE_DLL);

    // Pure IL image
    auto pCorHdr = reinterpret_cast<PIMAGE_COR20_HEADER>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR ));

    if (pCorHdr != nullptr && (pCorHdr->Flags & COMIMAGE_FLAGS_ILONLY))
        _isPureIL = true;
    else
        _isPureIL = false;

    // Sections
    for (int i = 0; i < _pImageHdr32->FileHeader.NumberOfSections; ++i, pSection++)
        _sections.push_back( *pSection );

    return true;
}

/// <summary>
/// Processes image imports
/// </summary>
/// <param name="useDelayed">Process delayed import instead</param>
/// <returns>Import data</returns>
mapImports& PEParser::ProcessImports( bool useDelayed /*= false*/ )
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
            char *pDllName = MAKE_PTR( char*, pImportTbl->DllNameRVA, _pFileBase );
            auto dllStr = Utils::AnsiToWstring( pDllName );

            pRVA = MAKE_PTR( uint8_t*, pImportTbl->ImportNameTableRVA, _pFileBase );

            while (_is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData)
            {
                uint64_t AddressOfData = _is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData;
                IMAGE_IMPORT_BY_NAME* pAddressTable = MAKE_PTR( IMAGE_IMPORT_BY_NAME*, AddressOfData, _pFileBase );
                ImportData data;

                // import by name
                if (AddressOfData < (_is64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) && pAddressTable->Name[0])
                {
                    data.importByOrd = false;
                    data.importName = pAddressTable->Name;
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
            char *pDllName = MAKE_PTR( char*, pImportTbl->Name, _pFileBase );
            auto dllStr = Utils::AnsiToWstring( pDllName );

            if (pImportTbl->OriginalFirstThunk)
                pRVA = MAKE_PTR( uint8_t*, pImportTbl->OriginalFirstThunk, _pFileBase );
            else
                pRVA = MAKE_PTR( uint8_t*, pImportTbl->FirstThunk, _pFileBase );

            while (_is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData)
            {
                uint64_t AddressOfData = _is64 ? THK64( pRVA )->u1.AddressOfData : THK32( pRVA )->u1.AddressOfData;
                IMAGE_IMPORT_BY_NAME* pAddressTable = MAKE_PTR( IMAGE_IMPORT_BY_NAME*, AddressOfData, _pFileBase );
                ImportData data;

                // import by name
                if (AddressOfData < (_is64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) && pAddressTable->Name[0])
                {
                    data.importByOrd   = false;
                    data.importName    = pAddressTable->Name;
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
                    data.ptrRVA = static_cast<size_t>(AddressOfData) - reinterpret_cast<size_t>(_pFileBase);

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
void PEParser::GetExportNames( std::list<std::string>& names )
{
    names.clear();

    auto pExport = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_EXPORT ));
    if (pExport == 0)
        return;

    DWORD *pAddressOfNames = reinterpret_cast<DWORD*>(pExport->AddressOfNames + reinterpret_cast<size_t>(_pFileBase));

    for (DWORD i = 0; i < pExport->NumberOfNames; ++i)
        names.push_back( reinterpret_cast<const char*>(_pFileBase) + pAddressOfNames[i] );

    return;
}

/// <summary>
/// Retrieve data directory address
/// </summary>
/// <param name="index">Directory index</param>
/// <param name="keepRelative">Keep address relative to image base</param>
/// <returns>Directory address</returns>
size_t PEParser::DirectoryAddress( int index, bool keepRelative /*= false*/ ) const
{
    // Sanity check
    if (index < 0 || index >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
        return 0;

    const IMAGE_DATA_DIRECTORY* idd = _is64 ? _pImageHdr64->OptionalHeader.DataDirectory 
                                            : _pImageHdr32->OptionalHeader.DataDirectory;

    if (idd[index].VirtualAddress == 0)
        return 0;
    else
        return ResolveRVAToVA( idd[index].VirtualAddress, keepRelative );
}

/// <summary>
/// Resolve virtual memory address to physical file offset
/// </summary>
/// <param name="Rva">Memory address</param>
/// <param name="keepRelative">Keep address relative to file start</param>
/// <returns>Resolved address</returns>
size_t PEParser::ResolveRVAToVA( size_t Rva, bool keepRelative /*= false*/ ) const
{
    if (_isPlainData)
    {
        for (auto& sec : _sections)
        {
            if (Rva >= sec.VirtualAddress && Rva <= sec.VirtualAddress + sec.Misc.VirtualSize)
            {
                if (keepRelative)
                    return  (Rva - sec.VirtualAddress + sec.PointerToRawData);
                else
                    return reinterpret_cast<size_t>(_pFileBase)+(Rva - sec.VirtualAddress + sec.PointerToRawData);
            }
        }

        return 0;
    }
    else
        return (keepRelative ? Rva : (reinterpret_cast<size_t>(_pFileBase) + Rva));
}

/// <summary>
/// Get data directory size
/// </summary>
/// <param name="index">Data directory index</param>
/// <returns>Data directory size</returns>
size_t PEParser::DirectorySize( int index ) const
{
    // Sanity check
    if (index < 0 || index >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
        return 0;

    const IMAGE_DATA_DIRECTORY* idd = _is64 ? _pImageHdr64->OptionalHeader.DataDirectory 
                                            : _pImageHdr32->OptionalHeader.DataDirectory;

    if (idd[index].VirtualAddress == 0)
        return 0;
    else
        return static_cast<size_t>(idd[index].Size);
}


/// <summary>
/// Retrieve image TLS callbacks
/// Callbacks are rebased for target image
/// </summary>
/// <param name="targetBase">Target image base</param>
/// <param name="result">Found callbacks</param>
/// <returns>Number of TLS callbacks in image</returns>
int PEParser::GetTLSCallbacks( module_t targetBase, std::vector<ptr_t>& result ) const
{
    uint8_t *pTls = reinterpret_cast<uint8_t*>(DirectoryAddress( IMAGE_DIRECTORY_ENTRY_TLS ));
    uint64_t* pCallback = 0;
    if (!pTls)
        return 0;

    uint64_t offset = _is64 ? TLS64( pTls )->AddressOfCallBacks : TLS32( pTls )->AddressOfCallBacks;

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

}

}
