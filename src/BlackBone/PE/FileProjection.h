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
    /// Release mapping, if any
    /// </summary>
    BLACKBONE_API void Release();

    /// <summary>
    /// Base of file in memory
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline void* base() const { return _pData; }

    /// <summary>
    /// Get activation context handle
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline HANDLE actx() const { return _hctx; }

    /// <summary>
    /// true if image is mapped as plain data file
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline bool  isPlainData() const { return _plainData; }

    /// <summary>
    /// Get manifest resource ID
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline int manifestID() const { return _manifestIdx; }

    /// <summary>
    /// Base of file in memory
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline operator void*() const { return _pData; }

private:
    HANDLE  _hFile = INVALID_HANDLE_VALUE;  // Target file HANDLE
    HANDLE  _hMapping = NULL;               // Memory mapping object
    void*   _pData = nullptr;               // Mapping base
    bool    _plainData = false;             // File mapped as plain data file
    HANDLE  _hctx = INVALID_HANDLE_VALUE;   // Activation context
    int     _manifestIdx = 0;               // Manifest resource ID
};

};
