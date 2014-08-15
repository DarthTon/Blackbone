#pragma once

#include "../Config.h"
#include "../Include/Winheaders.h"
#include <string>

namespace blackbone
{

/// <summary>
/// Load file as PE image
/// </summary>
class FileProjection
{
public:
    BLACKBONE_API FileProjection(void);
    BLACKBONE_API FileProjection( const std::wstring& path );
    BLACKBONE_API ~FileProjection(void);

    /// <summary>
    /// Memory-map specified file
    /// </summary>
    /// <param name="path">Image path</param>
    /// <returns>File address in memory, nullptr if failed</returns>
    BLACKBONE_API void* Project( const std::wstring& path );

    /// <summary>
    /// Assign existing image buffer
    /// </summary>
    /// <param name="pData">Image data</param>
    /// <param name="size">Buffer size</param>
    /// <param name="plainData">Buffer is plain file</param>
    /// <returns>File address in memory, nullptr if failed</returns>
    BLACKBONE_API void* Assign( void* pData, size_t size, bool plainData = true );

    /// <summary>
    /// Release mapping, if any
    /// </summary>
    BLACKBONE_API void Release();

    /// <summary>
    /// Base of file in memory
    /// </summary>
    /// <returns>Base address</returns>
    BLACKBONE_API inline void* base() const { return _pData; }

    /// <summary>
    /// Size of projection
    /// </summary>
    /// <returns>Mapping size</returns>
    BLACKBONE_API inline size_t size() const { return _size; }

    /// <summary>
    /// Get activation context handle
    /// </summary>
    /// <returns>Actx handle</returns>
    BLACKBONE_API inline HANDLE actx() const { return _hctx; }

    /// <summary>
    /// true if image is mapped as plain data file
    /// </summary>
    /// <returns>true if mapped as plain data file, false if mapped as image</returns>
    BLACKBONE_API inline bool isPlainData() const { return _plainData; }

    /// <summary>
    /// Get manifest resource ID
    /// </summary>
    /// <returns>Manifest resource ID</returns>
    BLACKBONE_API inline int manifestID() const { return _manifestIdx; }

    /// <summary>
    /// Base of file in memory
    /// </summary>
    /// <returns>Mapping base</returns>
    BLACKBONE_API inline operator void*() const { return _pData; }

private:
    HANDLE  _hFile = INVALID_HANDLE_VALUE;  // Target file HANDLE
    HANDLE  _hMapping = NULL;               // Memory mapping object
    void*   _pData = nullptr;               // Mapping base
    size_t  _size = 0;                      // Image size
    bool    _plainData = false;             // File mapped as plain data file
    HANDLE  _hctx = INVALID_HANDLE_VALUE;   // Activation context
    int     _manifestIdx = 0;               // Manifest resource ID
};

};
