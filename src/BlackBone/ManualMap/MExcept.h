#pragma once

#include "../Include/Winheaders.h"
#include "../Process/MemBlock.h"

namespace blackbone
{

/// <summary>
/// Exception handling support for arbitrary code
/// </summary>
class MExcept
{
public:
    // For debug purposes only
    BLACKBONE_API static void*  g_pImageBase;
    BLACKBONE_API static size_t g_imageSize;

protected:
    BLACKBONE_API MExcept( class Process& proc );
    BLACKBONE_API ~MExcept();

    /// <summary>
    /// Inject VEH wrapper into process
    /// Used to enable execution of SEH handlers out of image
    /// </summary>
    /// <param name="pTargetBase">Target image base address</param>
    /// <param name="imageSize">Size of the image</param>
    /// <returns>Error code</returns>
    BLACKBONE_API NTSTATUS CreateVEH( size_t pTargetBase, size_t imageSize );

    /// <summary>
    /// Removes VEH from target process
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API NTSTATUS RemoveVEH();

private:
    MExcept( const MExcept& ) = delete;
    MExcept& operator =(const MExcept&) = delete;

private:
    class Process& _proc;   // Underlying process
    MemBlock _pVEHCode;     // VEH function codecave
    size_t   _hVEH = 0;     // VEH handle
};

}