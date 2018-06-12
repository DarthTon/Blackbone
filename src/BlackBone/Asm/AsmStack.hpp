#pragma once

#pragma warning(push)
#pragma warning(disable : 4100 4804 4245)
#include "../../3rd_party/AsmJit/AsmJit.h"
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
        , _isX64( pAsm->getCode()->getArchType() == asmjit::ArchInfo::kTypeX64 )
        , _disp_ofst( _isX64 ? baseval : sizeof( uint64_t ) )
    {
    }

    /// <summary>
    /// Allocate stack variable
    /// </summary>
    /// <param name="size">Variable size</param>
    /// <returns>Variable memory object</returns>
    BLACKBONE_API asmjit::X86Mem AllocVar( int32_t size )
    {
        // Align on word length
        size = static_cast<int32_t>(Align( size, _isX64 ? sizeof( uint64_t ) : sizeof( uint32_t ) ));
        _disp_ofst += size;

        return _isX64
            ? asmjit::X86Mem( _pAsm->zsp(), _disp_ofst, size )
            : asmjit::X86Mem( _pAsm->zbp(), -_disp_ofst - size, size );
    }

    /// <summary>
    /// Allocate array of stack variables
    /// </summary>
    /// <param name="arr">Output array</param>
    /// <param name="count">Array elements count.</param>
    /// <param name="size">Element size.</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool AllocArray( asmjit::X86Mem arr[], int count, int32_t size )
    {
        for (int i = 0; i < count; i++)
        {
            _disp_ofst += size;

            arr[i] = _isX64
                ? asmjit::X86Mem( _pAsm->zsp(), _disp_ofst, size )
                : asmjit::X86Mem( _pAsm->zbp(), -_disp_ofst - size, size );
        }

        return true;
    }

    /// <summary>
    /// Get total size of all stack variables
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API intptr_t getTotalSize() const { return _disp_ofst; };

private:
    asmjit::X86Assembler* _pAsm;    // Underlying assembler
    bool _isX64;                    // Architecture is x64
    int32_t _disp_ofst;             // Next variable stack offset
};

//
//  Helpers
//
#define ALLOC_STACK_VAR(worker, name, type) asmjit::X86Mem name( worker.AllocVar( sizeof(type) ) );
#define ALLOC_STACK_VAR_S(worker, name, size) asmjit::X86Mem name( worker.AllocVar( size ) );

#define ALLOC_STACK_ARRAY(worker, name, type, count) \
    asmjit::X86Mem name[count]; \
    worker.AllocArray( name, count, sizeof(type) );

}