#pragma once

#include "../Config.h"
#include "../Include/Winheaders.h"
#include "../Include/Types.h"
#include "../Include/HandleGuard.h"
#include "../Misc/Utils.h"

#ifdef COMPILER_MSVC
#include "ImageNET.h"
#endif // COMPILER_MSVC

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <list>

namespace blackbone
{

namespace pe
{

enum AddressType
{
    RVA,    // Relative virtual
    VA,     // Absolute virtual
    RPA,    // Relative physical
};

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
 
/// <summary>
/// Import information
/// </summary>
struct ImportData
{
    std::string importName;     // Function name
    uintptr_t ptrRVA;            // Function pointer RVA in
    WORD importOrdinal;         // Function ordinal
    bool importByOrd;           // Function is imported by ordinal
};

/// <summary>
/// Export function info
/// </summary>
struct ExportData
{
    std::string name;
    uint32_t RVA = 0;

    ExportData( const std::string& name_, uint32_t rva_ )
        : name( name_ )
        , RVA( rva_ ) { }

    bool operator == (const ExportData& other)
    {
        return name == other.name;
    }

    bool operator < (const ExportData& other)
    {
        return name < other.name;
    }
};

// Imports and sections related
using mapImports  = std::unordered_map<std::wstring, std::vector<ImportData>>;
using vecSections = std::vector<IMAGE_SECTION_HEADER>;
using vecExports  = std::vector<ExportData>;

/// <summary>
/// Primitive PE parsing class
/// </summary>
class PEImage
{
    using PCHDR32 = const IMAGE_NT_HEADERS32*;
    using PCHDR64 = const IMAGE_NT_HEADERS64*;
    
public:
    BLACKBONE_API PEImage( void );
    BLACKBONE_API ~PEImage( void );

    BLACKBONE_API PEImage( PEImage&& other ) = default;

    /// <summary>
    /// Load image from file
    /// </summary>
    /// <param name="path">File path</param>
    /// <param name="skipActx">If true - do not initialize activation context</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Load( const std::wstring& path, bool skipActx = false );

    /// <summary>
    /// Load image from memory location
    /// </summary>
    /// <param name="pData">Image data</param>
    /// <param name="size">Data size.</param>
    /// <param name="plainData">If false - data has image layout</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Load( void* pData, size_t size, bool plainData = true );

    /// <summary>
    /// Reload closed image
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Reload();

    /// <summary>
    /// Release mapping, if any
    /// </summary>
    /// <param name="temporary">Preserve file paths for file reopening</param>
    BLACKBONE_API void Release( bool temporary = false );

    /// <summary>
    /// Parses PE image
    /// </summary>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS Parse( void* pImageBase = nullptr );

    /// <summary>
    /// Processes image imports
    /// </summary>
    /// <param name="useDelayed">Process delayed import instead</param>
    /// <returns>Import data</returns>
    BLACKBONE_API mapImports& GetImports( bool useDelayed = false );

    /// <summary>
    /// Retrieve all exported functions with names
    /// </summary>
    /// <param name="names">Found exports</param>
    BLACKBONE_API void GetExports( vecExports& exports );

    /// <summary>
    /// Retrieve image TLS callbacks
    /// Callbacks are rebased for target image
    /// </summary>
    /// <param name="targetBase">Target image base</param>
    /// <param name="result">Found callbacks</param>
    /// <returns>Number of TLS callbacks in image</returns>
    BLACKBONE_API int GetTLSCallbacks( module_t targetBase, std::vector<ptr_t>& result ) const;

    /// <summary>
    /// Retrieve data directory address
    /// </summary>
    /// <param name="index">Directory index</param>
    /// <param name="keepRelative">Keep address relative to image base</param>
    /// <returns>Directory address</returns>
    BLACKBONE_API uintptr_t DirectoryAddress( int index, AddressType type = VA ) const;

    /// <summary>
    /// Get data directory size
    /// </summary>
    /// <param name="index">Data directory index</param>
    /// <returns>Data directory size</returns>
    BLACKBONE_API size_t DirectorySize( int index ) const;

    /// <summary>
    /// Resolve virtual memory address to physical file offset
    /// </summary>
    /// <param name="Rva">Memory address</param>
    /// <param name="type">Address type to return</param>
    /// <returns>Resolved address</returns>
    BLACKBONE_API uintptr_t ResolveRVAToVA( uintptr_t Rva, AddressType type = VA ) const;

    /// <summary>
    /// Get image path
    /// </summary>
    /// <returns>Image path</returns>
    BLACKBONE_API inline const std::wstring& path() const { return _imagePath; }

    /// <summary>
    /// Get image name
    /// </summary>
    /// <returns>Image name</returns>
    BLACKBONE_API inline std::wstring name() const { return Utils::StripPath( _imagePath ); }

    /// <summary>
    /// Get image load address
    /// </summary>
    /// <returns>Image base</returns>
    BLACKBONE_API inline void* base() const { return _pFileBase; }

    /// <summary>
    /// Get image base address
    /// </summary>
    /// <returns>Image base</returns>
    BLACKBONE_API inline module_t imageBase() const { return _imgBase; }

    /// <summary>
    /// Get image size in bytes
    /// </summary>
    /// <returns>Image size</returns>
    BLACKBONE_API inline uint32_t imageSize() const { return _imgSize; }

    /// <summary>
    /// Get size of image headers
    /// </summary>
    /// <returns>Size of image headers</returns>
    BLACKBONE_API inline size_t headersSize() const { return _hdrSize; }

    /// <summary>
    /// Get image entry point rebased to another image base
    /// </summary>
    /// <param name="base">New image base</param>
    /// <returns>New entry point address</returns>
    BLACKBONE_API inline ptr_t entryPoint( module_t base ) const { return ((_epRVA != 0) ? (_epRVA + base) : 0); };

    /// <summary>
    /// Get image sections
    /// </summary>
    /// <returns>Image sections</returns>
    BLACKBONE_API inline const vecSections& sections() const { return _sections; }

    /// <summary>
    /// Check if image is an executable file and not a dll
    /// </summary>
    /// <returns>true if image is an *.exe</returns>
    BLACKBONE_API inline bool isExe() const { return _isExe; }

    /// <summary>
    /// Check if image is pure IL image
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API inline bool pureIL() const  { return _isPureIL; }
    BLACKBONE_API inline int32_t ilFlagOffset() const { return _ILFlagOffset; }

    /// <summary>
    /// Get image type. 32/64 bit
    /// </summary>
    /// <returns>Image type</returns>
    BLACKBONE_API inline eModType mType() const { return _is64 ? mt_mod64 : mt_mod32; }

    /// <summary>
    /// Get activation context handle
    /// </summary>
    /// <returns>Actx handle</returns>
    BLACKBONE_API inline HANDLE actx() const { return _hctx; }

    /// <summary>
    /// true if image is mapped as plain data file
    /// </summary>
    /// <returns>true if mapped as plain data file, false if mapped as image</returns>
    BLACKBONE_API inline bool isPlainData() const { return _isPlainData; }

    /// <summary>
    /// Get manifest resource ID
    /// </summary>
    /// <returns>Manifest resource ID</returns>
    BLACKBONE_API inline int manifestID() const { return _manifestIdx; }

    /// <summary>
    /// Get image subsystem
    /// </summary>
    /// <returns>Image subsystem</returns>
    BLACKBONE_API inline uint32_t subsystem() const { return _subsystem; }

    /// <summary>
    /// Get manifest resource file
    /// </summary>
    /// <returns>Manifest resource file</returns>
    BLACKBONE_API inline const std::wstring& manifestFile() const { return _manifestPath; }

    /// <summary>
    /// If true - no actual PE file available on disk
    /// </summary>
    /// <returns>Flag</returns>
    BLACKBONE_API inline bool noPhysFile() const { return _noFile; }

    /// <summary>
    /// DllCharacteristics field of header
    /// </summary>
    /// <returns>DllCharacteristics</returns>
    BLACKBONE_API inline uint32_t DllCharacteristics() const { return _DllCharacteristics; }

#ifdef COMPILER_MSVC
    /// <summary>
    /// .NET image parser
    /// </summary>
    /// <returns>.NET image parser</returns>
    BLACKBONE_API ImageNET& net() { return _netImage; }
#endif

private:
    /// <summary>
    /// Prepare activation context
    /// </summary>
    /// <param name="filepath">Path to PE file. If nullptr - manifest is extracted from memory to disk</param>
    /// <returns>Status code</returns>
    NTSTATUS PrepareACTX( const wchar_t* filepath = nullptr );

    /// <summary>
    /// Get manifest from image data
    /// </summary>
    /// <param name="size">Manifest size</param>
    /// <param name="manifestID">Mmanifest ID</param>
    /// <returns>Manifest data</returns>
    void* GetManifest( uint32_t& size, int32_t& manifestID );

private:
    Handle      _hFile;                         // Target file HANDLE
    Handle      _hMapping;                      // Memory mapping object
    Mapping     _pFileBase;                     // Mapping base
    bool        _isPlainData = false;           // File mapped as plain data file
    bool        _is64 = false;                  // Image is 64 bit
    bool        _isExe = false;                 // Image is an .exe file
    bool        _isPureIL = false;              // Pure IL image
    bool        _noFile = false;                // Parsed from memory, no underlying PE file available        
    PCHDR32     _pImageHdr32 = nullptr;         // PE header info
    PCHDR64     _pImageHdr64 = nullptr;         // PE header info
    ptr_t       _imgBase = 0;                   // Image base
    uint32_t    _imgSize = 0;                   // Image size
    uint32_t    _epRVA = 0;                     // Entry point RVA
    uint32_t    _hdrSize = 0;                   // Size of headers
    ACtxHandle  _hctx;                          // Activation context
    int32_t     _manifestIdx = 0;               // Manifest resource ID
    uint32_t    _subsystem = 0;                 // Image subsystem
    int32_t     _ILFlagOffset = 0;              // Offset of pure IL flag
    uint32_t    _DllCharacteristics = 0;        // DllCharacteristics flags

    vecSections _sections;                      // Section info
    mapImports  _imports;                       // Import functions
    mapImports  _delayImports;                  // Import functions

    std::wstring _imagePath;                    // Image path
    std::wstring _manifestPath;                 // Image manifest container

#ifdef COMPILER_MSVC
    ImageNET    _netImage;                      // .net image info
#endif
};

}
}
