#pragma once

#include "../../3rd_party/AsmJit/AsmJit.h"
#include "../Include/Macro.h"

#include <stdint.h>

namespace blackbone
{

class AsmStackAllocator
{
public:
    BLACKBONE_API AsmStackAllocator( asmjit::x86::Assembler* pAsm, int32_t baseval = 0x28 )
        : _pAsm( pAsm )
        , disp_ofst( _pAsm->environment().is64Bit() ? baseval : sizeof( uint64_t ) )
    {
    }

    /// <summary>
    /// Allocate stack variable
    /// </summary>
    /// <param name="size">Variable size</param>
    /// <returns>Variable memory object</returns>
    BLACKBONE_API asmjit::x86::Mem AllocVar( int32_t size )
    {
        bool x64 = _pAsm->environment().is64Bit();

        // Align on word length
        size = static_cast<int32_t>(Align( size, x64 ? sizeof( uint64_t ) : sizeof( uint32_t ) ));

        asmjit::x86::Mem val;
        if (x64)
            val = asmjit::x86::Mem( _pAsm->zsp(), disp_ofst, size );
        else
            val = asmjit::x86::Mem( _pAsm->zbp(), -disp_ofst - size, size );

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
    BLACKBONE_API bool AllocArray( asmjit::x86::Mem arr[], int count, int32_t size )
    {
        for (int i = 0; i < count; i++)
        {
            if (_pAsm->environment().is64Bit())
                arr[i] = asmjit::x86::Mem( _pAsm->zsp(), disp_ofst, size );
            else
                arr[i] = asmjit::x86::Mem( _pAsm->zbp(), -disp_ofst - size, size );

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
    asmjit::x86::Assembler* _pAsm;  // Underlying assembler
    int32_t disp_ofst;              // Next variable stack offset
};

//
//  Helpers
//
#define ALLOC_STACK_VAR(worker, name, type) asmjit::x86::Mem name( worker.AllocVar( sizeof(type) ) );
#define ALLOC_STACK_VAR_S(worker, name, size) asmjit::x86::Mem name( worker.AllocVar( size ) );

#define ALLOC_STACK_ARRAY(worker, name, type, count) \
    asmjit::x86::Mem name[count]; \
    worker.AllocArray( name, count, sizeof(type) );

}