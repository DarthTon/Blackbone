#pragma  once

#include "AsmJit/Assembler.h"
#include "AsmJit/MemoryManager.h"

#include <memory>

namespace blackbone
{
    /// <summary>
    /// General purpose assembly variable
    /// </summary>
    class AsmVariant
    {
        friend class AsmHelper32;
        friend class AsmHelper64;
        friend class RemoteExec;

        template<typename... Args>
        friend class FuncArguments;

    public:
        enum eType
        {
            noarg,          // void
            reg,            // register
            imm,            // immediate value (e.g. address)
            imm_double,     // double or long double
            imm_float,      // float
            dataPtr,        // pointer to local data (e.g. string or pointer to structure)
            dataStruct,     // structure passed by value
            structRet,      // pointer to space into which return value is copied (used when returning structures by value)
            mem,            // stack variable
            mem_ptr         // pointer to stack variable
        };

    public:
        AsmVariant( void )
            : AsmVariant( noarg, 0, 0 ) { }

        AsmVariant( int _imm )
            : AsmVariant( imm, sizeof(_imm), static_cast<size_t>(_imm) ) { }

        AsmVariant( unsigned long _imm )
            : AsmVariant( imm, sizeof(_imm), static_cast<size_t>(_imm) ) { }

        AsmVariant( size_t _imm )
            : AsmVariant( imm, sizeof(_imm), _imm ) { }
        
        AsmVariant( char* ptr )
            : AsmVariant( dataPtr, strlen( ptr ) + 1, reinterpret_cast<size_t>(ptr) ) { }

        AsmVariant( const char* ptr )
            : AsmVariant( const_cast<char*>(ptr) ) { }

        AsmVariant( wchar_t* ptr )
            : AsmVariant( dataPtr, (wcslen( ptr ) + 1) * sizeof(wchar_t), reinterpret_cast<size_t>(ptr) ) { }

        AsmVariant( const wchar_t* ptr )
            : AsmVariant( const_cast<wchar_t*>(ptr) ) { }

        AsmVariant( void* _imm )
            : AsmVariant( imm, sizeof(void*), reinterpret_cast<size_t>(_imm) ) { }

        AsmVariant( const void* _imm )
            : AsmVariant( imm, sizeof(void*), reinterpret_cast<size_t>(_imm) ) { }

        // In MSVS compiler 'long double' and 'double' are both 8 bytes long
        AsmVariant( long double _imm_fpu )
            : AsmVariant( (double)_imm_fpu ) { }

        AsmVariant( double _imm_fpu )
            : type( imm_double )
            , size( sizeof(double) )
            , imm_double_val( _imm_fpu )
            , new_imm_val( 0 ) { }

        AsmVariant( float _imm_fpu )
            : type( imm_float )
            , size( sizeof(float) )
            , imm_float_val( _imm_fpu )
            , new_imm_val( 0 ) { }

        AsmVariant( AsmJit::GPReg _reg )
            : type( reg )
            , size( sizeof(size_t) )
            , reg_val( _reg )
            , imm_double_val( -1.0 )
            , new_imm_val( 0 ) { }

        // Stack variable
        AsmVariant( AsmJit::Mem _mem )
            : type( mem )
            , size( sizeof(size_t) )
            , mem_val( _mem )
            , imm_double_val( -1.0 ) { }

        // Pointer to stack address
        AsmVariant( AsmJit::Mem* _mem )
            : type( mem_ptr )
            , size( sizeof(size_t) )
            , mem_val( *_mem )
            , imm_double_val( -1.0 )
            , new_imm_val( 0 ) { }

        AsmVariant( const AsmJit::Mem* _mem )
            : AsmVariant( const_cast<AsmJit::Mem*>(_mem) ) { }

        template <typename T>
        AsmVariant( T* ptr )
            : AsmVariant( dataPtr, sizeof(T), reinterpret_cast<size_t>(ptr) ) { }

        template <typename T>
        AsmVariant( const T* ptr )
        { 
            //
            // Treat function pointer as void*
            //
            const bool isFunction = std::is_function<std::remove_pointer<T>::type>::value;
            typedef typename std::conditional<isFunction, void*, T>::type Type;

            type = isFunction ? imm : dataPtr;
            size = sizeof(Type);
            imm_val = reinterpret_cast<size_t>(ptr);
            new_imm_val = 0;
        }

        template <typename T>
        AsmVariant( const T* ptr, size_t size )
            : AsmVariant( dataPtr, size, reinterpret_cast<size_t>(ptr) ) { }

        template <typename T>
        AsmVariant( T& val )
            : AsmVariant( dataPtr, sizeof(T), reinterpret_cast<size_t>(&val) ) { }

        template <typename T>
        AsmVariant( const T& val )
            : AsmVariant( dataPtr, sizeof(T), reinterpret_cast<size_t>(&val) ) {}

        // Pass argument by value case
        // Real RValue reference types are not supported because of ambiguity
        #pragma warning(disable : 4127)
        template <typename T>
        AsmVariant( T&& val )
            : AsmVariant( dataStruct, sizeof(T), reinterpret_cast<size_t>(&val) )
        {
            // Check if argument fits into register
            if (sizeof(T) <= sizeof(void*) && !std::is_class<T>::value)
            {
                type = imm;
                imm_val = 0;
                memcpy( &imm_val, &val, sizeof(T) );
            }
            else
            {
                buf.reset( new uint8_t[sizeof(T)] );
                imm_val = reinterpret_cast<size_t>(buf.get());
                memcpy( buf.get(), &val, sizeof(T) );
            }
        }
        #pragma warning(default : 4127)

        AsmVariant( AsmVariant&& other )
        {
            type            = other.type;
            size            = other.size;        
            imm_val         = other.imm_val;
            reg_val         = other.reg_val;
            mem_val         = other.mem_val;
            buf             = other.buf;
            imm_double_val  = other.imm_double_val;
            new_imm_val     = other.new_imm_val;
        }

        //
        // Get floating point value as raw data
        //
        inline uint32_t getImm_float()  const { return *(reinterpret_cast<const uint32_t*>(&imm_float_val)); }
        inline uint64_t getImm_double() const { return *(reinterpret_cast<const uint64_t*>(&imm_double_val)); }

        /// <summary>
        /// Check if argument can be passed in x86 register
        /// </summary>
        /// <returns>true if can</returns>
        inline bool reg86Compatible() const
        {
            if (type == dataStruct || type == imm_float || type == imm_double || type == structRet)
                return false;
            else
                return true;
        }

    private:
        // 
        /// <summary>
        /// Generic ctor for major data types
        /// </summary>
        /// <param name="_type">Argument type</param>
        /// <param name="_size">Argument size</param>
        /// <param name="val">Immediate value</param>
        AsmVariant( eType _type, size_t _size, size_t val )
            : type( _type )
            , size( _size )
            , imm_val( val )
            , new_imm_val( 0 ) { }

    private:
        eType         type;             // Variable type
        size_t        size;             // Variable size

        AsmJit::GPReg reg_val;          // General purpose register
        AsmJit::Mem   mem_val;          // Memory pointer

        // Immediate values
        union
        {
            size_t    imm_val;
            double    imm_double_val;
            float     imm_float_val;
        };

        size_t        new_imm_val;      // Replaced immediate value for dataPtr type

        // rvalue reference buffer
        std::shared_ptr<uint8_t> buf;
    };

}