#pragma once

#pragma warning(push)
#pragma warning(disable : 4100)
#include "../../../contrib/AsmJit/AsmJit.h"
#pragma warning(pop)

#include "../Include/Macro.h"

#include <stdint.h>

namespace blackbone
{

class AsmStackAllocator
{
public:
    BLACKBONE_API AsmStackAllocator( asmjit::X86Assembler* pAsm, int32_t baseval = 0x28 )
        : _pAsm( pAsm )
        , disp_ofst( sizeof( uint64_t ) )
    {
        if (pAsm->getArch() == asmjit::kArch::kArchX64)
            disp_ofst = baseval;
        else
            baseval = 0;
    }

    /// <summary>
    /// Allocate stack variable
    /// </summary>
    /// <param name="size">Variable size</param>
    /// <returns>Variable memory object</returns>
    BLACKBONE_API asmjit::Mem AllocVar( int32_t size )
    {
        // Align on word length
        size = static_cast<int32_t>(Align( size, sizeof( uint64_t ) ));

        asmjit::Mem val;
        if (_pAsm->getArch() == asmjit::kArch::kArchX64)
            val = asmjit::Mem( _pAsm->zsp, disp_ofst, size );
        else
            val = asmjit::Mem( _pAsm->zbp, -disp_ofst - size, size );

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
    BLACKBONE_API bool AllocArray( asmjit::Mem arr[], int count, int32_t size )
    {
        for (int i = 0; i < count; i++)
        {
            if (_pAsm->getArch() == asmjit::kArch::kArchX64)
                arr[i] = asmjit::Mem( _pAsm->zsp, disp_ofst, size );
            else
                arr[i] = asmjit::Mem( _pAsm->zbp, -disp_ofst - size, size );

            disp_ofst += size;
        }

        return true;
    }

    /// <summary>
    /// Get total size of all stack variables
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API inline intptr_t getTotalSize() const { return disp_ofst; };

private:
    asmjit::X86Assembler* _pAsm;    // Underlying assembler
    int32_t disp_ofst;              // Next variable stack offset
};

//
//  Helpers
//
#define ALLOC_STACK_VAR(worker, name, type) asmjit::Mem name( worker.AllocVar( sizeof(type) ) );
#define ALLOC_STACK_VAR_S(worker, name, size) asmjit::Mem name( worker.AllocVar( size ) );

#define ALLOC_STACK_ARRAY(worker, name, type, count) \
    asmjit::Mem name[count]; \
    worker.AllocArray( name, count, sizeof(type) );

}