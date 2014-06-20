#pragma once

#pragma warning(disable : 4100)
#include "AsmJit/AsmJit.h"
#pragma warning(default : 4100)
#include "Macro.h"

#include <stdint.h>

namespace blackbone
{

class AsmStackAllocator
{
public:
    AsmStackAllocator( intptr_t baseval = 0x28 )
        : disp_ofst( sizeof(size_t) )
    {
#ifdef USE64
        disp_ofst = baseval;
#else
        UNREFERENCED_PARAMETER( baseval );
#endif
    }

    /// <summary>
    /// Allocate stack variable
    /// </summary>
    /// <param name="size">Variable size</param>
    /// <returns>Variable memory object</returns>
    asmjit::host::Mem AllocVar( intptr_t size )
    {
        // Align on word length
        size = Align( size, sizeof(size_t) );

#ifdef USE64
        auto val = asmjit::host::Mem( asmjit::host::zsp, static_cast<int32_t>(disp_ofst), static_cast<int32_t>(size) );
#else
        auto val = asmjit::host::Mem( asmjit::host::zbp, -disp_ofst - size, size );
#endif
        disp_ofst += size;
        return val;
    }

    /// <summary>
    /// Allocate array of stack variables
    /// </summary>
    /// <param name="arr">Output array</param>
    /// <param name="count">Array elements count.</param>
    /// <param name="size">Element size.</param>
    /// <returns>true on success</returns>
    bool AllocArray( asmjit::host::Mem arr[], int count, intptr_t size )
    {
        for (int i = 0; i < count; i++)
        {
#ifdef USE64
            arr[i] = asmjit::host::Mem( asmjit::host::zsp, static_cast<int32_t>(disp_ofst), static_cast<int32_t>(size) );
#else
            arr[i] = asmjit::host::Mem( asmjit::host::zbp, -disp_ofst - size, size );
#endif
            disp_ofst += size;
        }

        return true;
    }

    /// <summary>
    /// Get total size of all stack variables
    /// </summary>
    /// <returns></returns>
    inline intptr_t getTotalSize() const { return disp_ofst; };

private:
    intptr_t disp_ofst;     // Next variable stack offset
};

//
//  Helpers
//
#define ALLOC_STACK_VAR(worker, name, type) asmjit::host::Mem name( worker.AllocVar( sizeof(type) ) );
#define ALLOC_STACK_VAR_S(worker, name, size) asmjit::host::Mem name( worker.AllocVar( size ) );

#define ALLOC_STACK_ARRAY(worker, name, type, count) \
    asmjit::host::Mem name[count]; \
    worker.AllocArray( name, count, sizeof(type) );

}