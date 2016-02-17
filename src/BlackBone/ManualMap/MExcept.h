#pragma once

#include "../Include/Winheaders.h"
#include "../Process/MemBlock.h"

namespace blackbone
{

/// <summary>
/// x64 exception module info 
/// </summary>
struct ExceptionModule
{
    ptr_t base;
    ptr_t size;
};


/// <summary>
/// x64 module table
/// </summary>
struct ModuleTable
{
    ptr_t count;                    // Number of used entries
    ExceptionModule entry[250];     // Module data
};

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
    /// <param name="mt">Mosule type</param>
    /// <param name="partial">Partial exception support</param>
    /// <returns>Error code</returns>
    BLACKBONE_API NTSTATUS CreateVEH( uintptr_t pTargetBase, size_t imageSize, eModType mt, bool partial );

    /// <summary>
    /// Removes VEH from target process
    /// </summary>
    /// <param name="partial">Partial exception support</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS RemoveVEH( bool partial );

    /// <summary>
    /// Reset data
    /// </summary>
    BLACKBONE_API  inline void reset() { _pModTable.Free(); }

private:
    MExcept( const MExcept& ) = delete;
    MExcept& operator =(const MExcept&) = delete;

private:
    class Process& _proc;   // Underlying process
    MemBlock  _pVEHCode;    // VEH function codecave
    MemBlock  _pModTable;   // x64 module address range table
    uintptr_t _hVEH = 0;    // VEH handle
};

}