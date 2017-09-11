#pragma once

#include "../Include/Winheaders.h"
#include "../Include/Types.h"

#include <unordered_map>
#include <vector>
#include <string>

namespace blackbone
{

class NameResolve
{
    using mapApiSchema = std::unordered_map<std::wstring, std::vector<std::wstring>>;
   
public:
    enum eResolveFlag
    {
        Default         = 0,    // Full resolve
        ApiSchemaOnly   = 1,    // Resolve only Api schema dlls
        EnsureFullPath  = 2,    // Make sure resulting path is full-qualified
        NoSearch        = 4,    // Don't perform file search, only resolve name
        Wow64           = 8,    // Redirect System32 files to SysWow64
    };

public:
    BLACKBONE_API ~NameResolve() = default;

    BLACKBONE_API static NameResolve& Instance();

    /// <summary>
    /// Initialize api set map
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API bool Initialize();

    /// <summary>
    /// Resolve image path.
    /// </summary>
    /// <param name="path">Image to resolve</param>
    /// <param name="baseName">Name of parent image. Used only when resolving import images</param>
    /// <param name="searchDir">Directory where source image is located</param>
    /// <param name="flags">Resolve flags</param>
    /// <param name="proc">Process. Used to search process executable directory</param>
    /// <param name="actx">Activation context</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS ResolvePath( 
        std::wstring& path,
        const std::wstring& baseName,
        const std::wstring& searchDir,
        eResolveFlag flags,
        class Process& proc,
        HANDLE actx = INVALID_HANDLE_VALUE
        );

    /// <summary>
    /// Try SxS redirection
    /// </summary>
    /// <param name="path">Image path.</param>
    /// <param name="proc">Process. Used to search process executable directory</param>
    /// <param name="actx">Activation context</param>
    /// <returns></returns>
    BLACKBONE_API NTSTATUS ProbeSxSRedirect( std::wstring& path, class Process& proc, HANDLE actx = INVALID_HANDLE_VALUE );

private:
    // Ensure singleton
    NameResolve() = default;
    NameResolve( const NameResolve& ) = delete;
    NameResolve& operator =( const NameResolve& ) = delete;

    /// <summary>
    /// Gets the process executable directory
    /// </summary>
    /// <param name="pid">Process ID</param>
    /// <returns>Process executable directory</returns>
    std::wstring GetProcessDirectory( DWORD pid );

    /// <summary>
    /// OS dependent api set initialization
    /// </summary>
    /// <returns>true on success</returns>
    template<typename PApiSetMap, typename PApiSetEntry, typename PHostArray, typename PHostEntry>
    bool InitializeP();

private:
    mapApiSchema _apiSchema;    // Api schema table
};


}