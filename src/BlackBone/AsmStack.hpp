#pragma once

#include "AsmJit/Assembler.h"
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
#ifdef _M_AMD64
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
    AsmJit::Mem AllocVar( intptr_t size )
    {
        // Align on word length
        size = Align( size, sizeof(size_t) );

#ifdef _M_AMD64
        auto val = AsmJit::Mem( AsmJit::nsp, disp_ofst, size );
#else
        auto val = AsmJit::Mem( AsmJit::nbp, -disp_ofst - size, size );
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
    bool AllocArray( AsmJit::Mem arr[], int count, intptr_t size )
    {
        for (int i = 0; i < count; i++)
        {
#ifdef _M_AMD64
            arr[i] = AsmJit::Mem( AsmJit::nsp, disp_ofst, size );
#else
            arr[i] = AsmJit::Mem( AsmJit::nbp, -disp_ofst - size, size );
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
#define ALLOC_STACK_VAR(worker, name, type) AsmJit::Mem name( worker.AllocVar( sizeof(type) ) );
#define ALLOC_STACK_VAR_S(worker, name, size) AsmJit::Mem name( worker.AllocVar( size ) );

#define ALLOC_STACK_ARRAY(worker, name, type, count) \
    AsmJit::Mem name[count]; \
    worker.AllocArray( name, count, sizeof(type) );

}