#pragma once

#include "Winheaders.h"
#include "Types.h"
#include "ImageNET.h"

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>

namespace blackbone
{

namespace pe
{

// Relocation block information
struct RelocData
{
    ULONG PageRVA;
    ULONG BlockSize;

    struct
    {
        WORD Offset : 12; 
        WORD Type   : 4; 
    }Item[1];
};

// Import information
struct ImportData
{
    std::string importName;     // Function name
    size_t ptrRVA;              // Function pointer RVA in
    WORD importOrdinal;         // Function ordinal
    bool importByOrd;           // Function is imported by ordinal
};

// Imports and sections related
typedef std::unordered_map<std::wstring, std::vector<ImportData>> mapImports;
typedef std::vector<IMAGE_SECTION_HEADER> vecSections;

/// <summary>
/// Primitive PE parsing class
/// </summary>
class PEParser
{
    typedef const IMAGE_NT_HEADERS32* PCHDR32;
    typedef const IMAGE_NT_HEADERS64* PCHDR64;
    
public:
    PEParser( void );
    ~PEParser( void );

    /// <summary>
    /// Parses PE image
    /// </summary>
    /// <param name="pFileBase">File memory location</param>
    /// <param name="isPlainData">Treat file as plain datafile</param>
    /// <returns>true on success</returns>
    bool Parse( const void* pFileBase, bool isPlainData = false );

    /// <summary>
    /// Processes image imports
    /// </summary>
    /// <param name="useDelayed">Process delayed import instead</param>
    /// <returns>Import data</returns>
    mapImports& ProcessImports( bool useDelayed = false );

    /// <summary>
    /// Retrieve all exported functions with names
    /// </summary>
    /// <param name="names">Found exports</param>
    void GetExportNames( std::list<std::string>& names );

    /// <summary>
    /// Retrieve image TLS callbacks
    /// Callbacks are rebased for target image
    /// </summary>
    /// <param name="targetBase">Target image base</param>
    /// <param name="result">Found callbacks</param>
    /// <returns>Number of TLS callbacks in image</returns>
    int GetTLSCallbacks( module_t targetBase, std::vector<ptr_t>& result ) const;

    /// <summary>
    /// Retrieve data directory address
    /// </summary>
    /// <param name="index">Directory index</param>
    /// <param name="keepRelative">Keep address relative to image base</param>
    /// <returns>Directory address</returns>
    size_t DirectoryAddress( int index, bool keepRelative = false ) const;

    /// <summary>
    /// Get data directory size
    /// </summary>
    /// <param name="index">Data directory index</param>
    /// <returns>Data directory size</returns>
    size_t DirectorySize( int index ) const;

    /// <summary>
    /// Resolve virtual memory address to physical file offset
    /// </summary>
    /// <param name="Rva">Memory address</param>
    /// <param name="keepRelative">Keep address relative to file start</param>
    /// <returns>Resolved address</returns>
    size_t ResolveRVAToVA( size_t Rva, bool keepRelative = false ) const;

    /// <summary>
    /// Get image base address
    /// </summary>
    /// <returns>Image base</returns>
    inline module_t imageBase() const { return _imgBase; }

    /// <summary>
    /// Get image size in bytes
    /// </summary>
    /// <returns>Image size</returns>
    inline size_t imageSize() const { return _imgSize; }

    /// <summary>
    /// Get size of image headers
    /// </summary>
    /// <returns>Size of image headers</returns>
    inline size_t headersSize() const { return _hdrSize; }

    /// <summary>
    /// Get image entry point rebased to another image base
    /// </summary>
    /// <param name="base">New image base</param>
    /// <returns>New entry point address</returns>
    inline ptr_t entryPoint( module_t base ) const { return _epRVA + base; };

    /// <summary>
    /// Get image sections
    /// </summary>
    /// <returns>Image sections</returns>
    inline const vecSections& sections() const { return _sections; }

    /// <summary>
    /// Check if image is an executable file and not a dll
    /// </summary>
    /// <returns>true if image is an *.exe</returns>
    inline bool IsExe() const { return _isExe; }


    /// <summary>
    /// Check if image is pure IL image
    /// </summary>
    /// <returns>true on success</returns>
    inline bool IsPureManaged() const  { return _isPureIL; }

    /// <summary>
    /// Get image type. 32/64 bit
    /// </summary>
    /// <returns>Image type</returns>
    inline eModType mType() const { return _is64 ? mt_mod64 : mt_mod32; }

    /// <summary>
    /// .NET image parser
    /// </summary>
    /// <returns>.NET image parser</returns>
    ImageNET& net() { return _netImage; }

private:
    bool        _isPlainData = false;       // File mapped as plain data file
    bool        _is64 = false;              // Image is 64 bit
    bool        _isExe = false;             // Image is an .exe file
    bool        _isPureIL = false;          // Pure IL image
    const void *_pFileBase = nullptr;       // File mapping base address
    PCHDR32     _pImageHdr32 = nullptr;     // PE header info
    PCHDR64     _pImageHdr64 = nullptr;     // PE header info
    ptr_t       _imgBase = 0;               // Image base
    size_t      _imgSize = 0;               // Image size
    size_t      _epRVA = 0;                 // Entry point RVA
    size_t      _hdrSize = 0;               // Size of headers

    vecSections _sections;                  // Section info
    mapImports  _imports;                   // Import functions
    mapImports  _delayImports;              // Import functions
    ImageNET    _netImage;                  // .net image info
};

}
}
