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
        BLACKBONE_API IAsmHelper( uint32_t arch = asmjit::Environment::kArchHost )
        {
            _code.init( asmjit::Environment( arch ) );
            _code.attach( &_assembler );
        }

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
            _assembler.mov( asmjit::x86::dword_ptr( asmjit::x86::esp, 4 ), 0x23 );
            _assembler.add( asmjit::x86::dword_ptr( asmjit::x86::esp ), 0xD );
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
            //_assembler.add( asmjit::x86::dword_ptr( asmjit::x86::esp ), 5 );
            _assembler.dd( '\x83\x04\x24\x05' );
            _assembler.db( 0xCB );    // retf
        }

        BLACKBONE_API void* make()
        {
            auto buffer = _code.textSection()->buffer();
            _code.growBuffer( &buffer, _code.codeSize() );

            auto err = relocateCode( buffer.data() );
            if (err != asmjit::kErrorOk)
                return nullptr;

            DWORD flOld = 0;
            VirtualProtect( buffer.data(), _code.codeSize(), PAGE_EXECUTE_READWRITE, &flOld );

            return buffer.data();
        }

        BLACKBONE_API size_t getCodeSize()
        {
            return _code.codeSize();
        }

        BLACKBONE_API asmjit::Error relocateCode( void* target, uint64_t baseAddress = 0 )
        {
            if (baseAddress == 0)
                baseAddress = reinterpret_cast<uint64_t>(target);

            _code.flatten();
            _code.resolveUnresolvedLinks();

            auto err = _code.relocateToBase( baseAddress );
            if (err != asmjit::kErrorOk)
                return err;

            for (const auto section : _code.sections())
            {
                size_t offset = size_t( section->offset() );
                size_t bufferSize = size_t( section->bufferSize() );
                size_t virtualSize = size_t( section->virtualSize() );

                memcpy( static_cast<uint8_t*>(target) + offset, section->data(), bufferSize );

                if (virtualSize > bufferSize)
                    memset( static_cast<uint8_t*>(target) + offset + bufferSize, 0, virtualSize - bufferSize );
            }

            return asmjit::kErrorOk;
        }

        BLACKBONE_API asmjit::x86::Assembler* assembler() { return &_assembler; }
        BLACKBONE_API asmjit::x86::Assembler* operator ->() { return &_assembler; }

    private:
        IAsmHelper( const IAsmHelper& ) = delete;
        IAsmHelper& operator =(const IAsmHelper&) = delete;

    protected:
        asmjit::CodeHolder _code;
        asmjit::x86::Assembler _assembler;
    };
}