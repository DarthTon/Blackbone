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
    BLACKBONE_API MExcept() = default;
    BLACKBONE_API ~MExcept() = default;

    MExcept( const MExcept& ) = delete;
    MExcept& operator =( const MExcept& ) = delete;

    /// <summary>
    /// Inject VEH wrapper into process
    /// Used to enable execution of SEH handlers out of image
    /// </summary>
    /// <param name="proc">Target process</param>
    /// <param name="mod">Target module</param>
    /// <param name="partial">Partial exception support</param>
    /// <returns>Error code</returns>
    BLACKBONE_API NTSTATUS CreateVEH( class Process& proc, ModuleData& mod, bool partial );

    /// <summary>
    /// Removes VEH from target process
    /// </summary>
    /// <param name="proc">Target process</param>
    /// <param name="partial">Partial exception support</param>
    /// <param name="mt">Module type</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS RemoveVEH( class Process& proc, bool partial, eModType mt );

    /// <summary>
    /// Reset data
    /// </summary>
    BLACKBONE_API void reset() { _pModTable.Free(); }

private:
    MemBlock _pVEHCode;    // VEH function codecave
    MemBlock _pModTable;   // x64 module address range table
    uint64_t _hVEH = 0;    // VEH handle

    static uint8_t _handler32[];
    static uint8_t _handler64[];
};
}
