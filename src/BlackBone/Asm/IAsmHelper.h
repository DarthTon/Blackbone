#pragma once

#include "AsmVariant.hpp"
#include "AsmStack.hpp"
#include "../Include/Macro.h"

#include <initializer_list>
#include <vector>

namespace blackbone
{
    //
    // Function calling convention
    //
    enum eCalligConvention
    {
        cc_cdecl,       // cdecl
        cc_stdcall,     // stdcall
        cc_thiscall,    // thiscall
        cc_fastcall     // fastcall
    };

    //
    // Function return type
    // Do not change numeric values!
    //
    enum eReturnType
    {
        rt_int32  = 4,  // 32bit value
        rt_int64  = 8,  // 64bit value
        rt_float  = 1,  // float value
        rt_double = 2,  // double value
        rt_struct = 3,  // structure returned by value
    };

    // Argument pass method
    enum eArgType
    {
        at_ecx   = 0,   // In ecx
        at_edx   = 1,   // In edx
        at_stack = 2,   // On stack
    };


    /// <summary>
    /// Assembly generation helper
    /// </summary>
    class IAsmHelper
    {
    public:
        BLACKBONE_API IAsmHelper( uint32_t arch = asmjit::kArchHost ) 
            : _assembler( &_runtime, arch ) { }

        virtual ~IAsmHelper() { }

        virtual void GenPrologue( bool switchMode = false ) = 0;
        virtual void GenEpilogue( bool switchMode = false, int retSize = -1) = 0;
        virtual void GenCall( const AsmFunctionPtr&, const std::vector<AsmVariant>& args, eCalligConvention cc = cc_stdcall ) = 0;
        virtual void ExitThreadWithStatus( uint64_t pExitThread, uint64_t resultPtr ) = 0;
        virtual void SaveRetValAndSignalEvent( uint64_t pSetEvent, uint64_t ResultPtr, uint64_t EventPtr, uint64_t errPtr, eReturnType rtype = rt_int32 ) = 0;
        virtual void EnableX64CallStack( bool state ) = 0;

        /// <summary>
        /// Switch processor into WOW64 emulation mode
        /// </summary>
        BLACKBONE_API void SwitchTo86()
        {
            asmjit::Label l = _assembler.newLabel();

            _assembler.call( l ); _assembler.bind( l );
            _assembler.mov( asmjit::host::dword_ptr( asmjit::host::esp, 4 ), 0x23 );
            _assembler.add( asmjit::host::dword_ptr( asmjit::host::esp ), 0xD );
            _assembler.db( 0xCB );    // retf
        }

        /// <summary>
        /// Switch processor into x64 mode (long mode)
        /// </summary>
        BLACKBONE_API void SwitchTo64()
        {
            asmjit::Label l = _assembler.newLabel();

            _assembler.push( 0x33 );
            _assembler.call( l );  _assembler.bind( l );
            //_assembler.add( asmjit::host::dword_ptr( asmjit::host::esp ), 5 );
            _assembler.dd( '\x83\x04\x24\x05' );
            _assembler.db( 0xCB );    // retf
        }

        BLACKBONE_API inline asmjit::X86Assembler* assembler() { return &_assembler; }
        BLACKBONE_API inline asmjit::X86Assembler* operator ->() { return &_assembler; }

    private:
        IAsmHelper( const IAsmHelper& ) = delete;
        IAsmHelper& operator =(const IAsmHelper&) = delete;

    protected:
        asmjit::JitRuntime _runtime;
        asmjit::X86Assembler _assembler;
    };
}