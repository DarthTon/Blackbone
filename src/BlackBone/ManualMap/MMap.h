#pragma once

#include "../Config.h"
#include "../Include/Winheaders.h"
#include "../Include/Macro.h"
#include "../PE/PEImage.h"

#include "../Process/MemBlock.h"
#include "MExcept.h"

#include <map>
#include <stack>

namespace blackbone
{

// Loader flags

enum eLoadFlags
{
    NoFlags         = 0x00,     // No flags
    ManualImports   = 0x01,     // Manually map import libraries
    CreateLdrRef    = 0x02,     // Create module references for native loader
    WipeHeader      = 0x04,     // Wipe image PE headers
    HideVAD         = 0x10,     // Make image appear as PAGE_NOACESS region
    MapInHighMem    = 0x20,     // Try to map image in address space beyond 4GB limit
    RebaseProcess   = 0x40,     // If target image is an .exe file, process base address will be replaced with mapped module value

    NoExceptions    = 0x01000,   // Do not create custom exception handler
    PartialExcept   = 0x02000,   // Only create Inverted function table, without VEH
    NoDelayLoad     = 0x04000,   // Do not resolve delay import
    NoSxS           = 0x08000,   // Do not apply SxS activation context
    NoTLS           = 0x10000,   // Skip TLS initialization and don't execute TLS callbacks
};

ENUM_OPS( eLoadFlags )

// Native loader flags callback
typedef enum LdrRefFlags( *LdrCallback )(void* context, const ModuleData& modInfo);


/// <summary>
/// Image data
/// </summary>
struct ImageContext
{
    typedef std::vector<ptr_t> vecPtr;

    pe::PEImage    peImage;                 // PE image data
    MemBlock       imgMem;                  // Target image memory region
    std::wstring   FilePath;                // path to image being mapped
    std::wstring   FileName;                // File name string
    vecPtr         tlsCallbacks;            // TLS callback routines
    ptr_t          pExpTableAddr = 0;       // Exception table address (amd64 only)
    ptr_t          EntryPoint = 0;          // Target image entry point
    eLoadFlags     flags = NoFlags;         // Image loader flags
    bool           initialized = false;     // Image entry point was called
};

typedef std::vector<std::unique_ptr<ImageContext>> vecImageCtx;

/// <summary>
/// Manual image mapper
/// </summary>
class MMap : public MExcept
{
        
public:
    BLACKBONE_API MMap( class Process& proc );
    BLACKBONE_API ~MMap( void );

    /// <summary>
    /// Manually map PE image into underlying target process
    /// </summary>
    /// <param name="path">Image path</param>
    /// <param name="flags">Image mapping flags</param>
    /// <param name="ldrCallback">Loader callback. Triggers for each mapped module</param>
    /// <param name="ldrContext">User-supplied Loader callback context</param>
    /// <returns>Mapped image info </returns>
    BLACKBONE_API const ModuleData* MapImage(
        const std::wstring& path,
        eLoadFlags flags = NoFlags,
        LdrCallback ldrCallback = nullptr,
        void* ldrContext = nullptr
        );

    /// <summary>
    ///Manually map PE image into underlying target process
    /// </summary>
    /// <param name="buffer">Image data buffer</param>
    /// <param name="size">Buffer size.</param>
    /// <param name="asImage">If set to true - buffer has image memory layout</param>
    /// <param name="flags">Image mapping flags</param>
    /// <param name="ldrCallback">Loader callback. Triggers for each mapped module</param>
    /// <param name="ldrContext">User-supplied Loader callback context</param>
    /// <returns>Mapped image info</returns>
    BLACKBONE_API const ModuleData* MapImage(
        size_t size, void* buffer,
        bool asImage = false,
        eLoadFlags flags = NoFlags,
        LdrCallback ldrCallback = nullptr,
        void* ldrContext = nullptr
        );

    /// <summary>
    /// Unmap all manually mapped modules
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API bool UnmapAllModules();

    /// <summary>
    /// Remove any traces from remote process
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API void Cleanup();

    /// <summary>
    /// Reset local data
    /// </summary>
    BLACKBONE_API inline void reset() { _images.clear(); _pAContext.Reset(); _usedBlocks.clear(); }

private:
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
    const ModuleData* MapImageInternal(
        const std::wstring& path,
        void* buffer, size_t size,
        bool asImage = false,
        eLoadFlags flags = NoFlags,
        LdrCallback ldrCallback = nullptr,
        void* ldrContext = nullptr
        );

    /// <summary>
    /// Get existing module or map it if absent
    /// </summary>
    /// <param name="path">Image path</param>
    /// <param name="flags">Mapping flags</param>
    /// <returns>Module info</returns>
    const ModuleData* FindOrMapModule(
        const std::wstring& path,
        void* buffer, size_t size, bool asImage,
        eLoadFlags flags = NoFlags
        );

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
    bool RunModuleInitializers( ImageContext* pImage, DWORD dwReason );

    /// <summary>
    /// Copies image into target process
    /// </summary>
    /// <param name="pImage">Image data</param>
    /// <returns>true on success</returns>
    bool CopyImage( ImageContext* pImage );

    /// <summary>
    /// Adjust image memory protection
    /// </summary>
    /// <param name="pImage">image data</param>
    /// <returns>true on success</returns>
    bool ProtectImageMemory( ImageContext* pImage );

    /// <summary>
    ///  Fix relocations if image wasn't loaded at base address
    /// </summary>
    /// <param name="pImage">image data</param>
    /// <returns>true on success</returns>
    bool RelocateImage( ImageContext* pImage );

    /// <summary>
    /// Resolves image import or delayed image import
    /// </summary>
    /// <param name="pImage">Image data</param>
    /// <param name="useDelayed">Resolve delayed import instead</param>
    /// <returns>true on success</returns>
    bool ResolveImport( ImageContext* pImage, bool useDelayed = false );

    /// <summary>
    /// Resolve static TLS storage
    /// </summary>
    /// <param name="pImage">image data</param>
    /// <returns>true on success</returns>
    bool InitStaticTLS( ImageContext* pImage );

    /// <summary>
    /// Set custom exception handler to bypass SafeSEH under DEP 
    /// </summary>
    /// <param name="pImage">image data</param>
    /// <returns>true on success</returns>
    NTSTATUS EnableExceptions( ImageContext* pImage );

    /// <summary>
    /// Remove custom exception handler
    /// </summary>
    /// <param name="pImage">image data</param>
    /// <returns>true on success</returns>
    NTSTATUS DisableExceptions( ImageContext* pImage );

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
    bool CreateActx( const std::wstring& path, int id = 2, bool asImage = true );

    /// <summary>
    /// Calculate and set security cookie
    /// </summary>
    /// <param name="pImage">image data</param>
    /// <returns>true on success</returns>
    bool InitializeCookie( ImageContext* pImage );

    /// <summary>
    /// Hide memory VAD node
    /// </summary>
    /// <param name="imageMem">Image to purge</param>
    /// <returns>Status code</returns>
    NTSTATUS ConcealVad( const MemBlock& imageMem );

    /// <summary>
    /// Allocates memory region beyond 4GB limit
    /// </summary>
    /// <param name="imageMem">Image data</param>
    /// <param name="size">Block size</param>
    /// <returns>Status code</returns>
    NTSTATUS AllocateInHighMem( MemBlock& imageMem, size_t size );

    /// <summary>
    /// Return existing or load missing dependency
    /// </summary>
    /// <param name="pImage">Currently mapped image data</param>
    /// <param name="path">Dependency path</param>
    /// <returns></returns>
    const ModuleData* FindOrMapDependency( ImageContext* pImage, std::wstring& path );

    /// <summary>
    /// Map pure IL image
    /// Not supported yet
    /// </summary>
    /// <returns>Image base</returns>
    module_t MapPureManaged();

    /// <summary>
    /// Transform section characteristics into memory protection flags
    /// </summary>
    /// <param name="characteristics">Section characteristics</param>
    /// <returns>Memory protection value</returns>
    DWORD GetSectionProt( DWORD characteristics );

private:
    vecImageCtx     _images;                // Mapped images
    class Process&  _process;               // Target process manager
    MemBlock        _pAContext;             // SxS activation context memory address
    LdrCallback     _ldrCallback = nullptr; // Loader callback for adding image into loader lists
    void*           _ldrContext = nullptr;  // user context for _ldrCallback       

    std::vector<std::pair<ptr_t, size_t>> _usedBlocks;   // Used memory blocks 
};

}

