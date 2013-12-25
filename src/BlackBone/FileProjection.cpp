#include "FileProjection.h"

namespace blackbone
{

FileProjection::FileProjection( void )
{
}

FileProjection::FileProjection( const std::wstring& path ) 
{
    Project(path);
}


FileProjection::~FileProjection(void)
{
    Release();
}


/// <summary>
/// Memory-map specified file
/// </summary>
/// <param name="path">Image path</param>
/// <returns>File address in memory, nullptr if failed</returns>
void* FileProjection::Project( const std::wstring& path )
{
    Release();

    // Prepare activation context
    ACTCTX act = { 0 };
    act.cbSize = sizeof(act);
    act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    act.lpSource = path.c_str();
    act.lpResourceName = MAKEINTRESOURCEW( 2 );

    _hctx = CreateActCtx( &act );

    // Retry with another resource id
    if (_hctx == INVALID_HANDLE_VALUE)
    {
        act.lpResourceName = MAKEINTRESOURCEW( 1 );

        if ((_hctx = CreateActCtx( &act )) != INVALID_HANDLE_VALUE)
            _manifestIdx = 1;
    }
    else
        _manifestIdx = 2;

    _hFile = CreateFileW( path.c_str(), FILE_GENERIC_READ, 
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          NULL, OPEN_EXISTING, 0, NULL );

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        // Try mapping as image
        _hMapping = CreateFileMappingW( _hFile, NULL, SEC_IMAGE | PAGE_READONLY, 0, 0, NULL );

        if (_hMapping && _hMapping != INVALID_HANDLE_VALUE)
        {
            _pData = MapViewOfFile( _hMapping, FILE_MAP_READ, 0, 0, 0 );
        }
        // Map as simple datafile
        else
        {
            _plainData = true;
            _hMapping  = CreateFileMappingW( _hFile, NULL, PAGE_READONLY, 0, 0, NULL );

            if (_hMapping && _hMapping != INVALID_HANDLE_VALUE)
                _pData = MapViewOfFile( _hMapping, FILE_MAP_READ, 0, 0, 0 );
        }
    }

    return _pData;
}

/// <summary>
/// Release mapping, if any
/// </summary>
void FileProjection::Release()
{
    if (_hctx != INVALID_HANDLE_VALUE)
    {
        ReleaseActCtx( _hctx );
        _hctx = INVALID_HANDLE_VALUE;
    }

    if(_pData)
    {
        UnmapViewOfFile( _pData );
        _pData = nullptr;
    }

    if (_hMapping && _hMapping != INVALID_HANDLE_VALUE)
    {
        CloseHandle( _hMapping );
        _hMapping = NULL;
    }

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle( _hFile );
        _hFile = INVALID_HANDLE_VALUE;
    }
}

};