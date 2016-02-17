#pragma  once


#include "../Config.h"
#pragma warning(disable : 4100)
#include <AsmJit/asmjit.h>
#pragma warning(default : 4100)

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
        BLACKBONE_API AsmVariant( void )
            : AsmVariant( noarg, 0, 0 ) { }

        BLACKBONE_API AsmVariant( int _imm )
            : AsmVariant( imm, sizeof(_imm), static_cast<uintptr_t>(_imm) ) { }

        BLACKBONE_API AsmVariant( unsigned long _imm )
            : AsmVariant( imm, sizeof(_imm), static_cast<uintptr_t>(_imm) ) { }

        BLACKBONE_API AsmVariant( uintptr_t _imm )
            : AsmVariant( imm, sizeof(_imm), _imm ) { }
        
        BLACKBONE_API AsmVariant( char* ptr )
            : AsmVariant( dataPtr, strlen( ptr ) + 1, reinterpret_cast<uintptr_t>(ptr) ) { }

        BLACKBONE_API AsmVariant( const char* ptr )
            : AsmVariant( const_cast<char*>(ptr) ) { }

        BLACKBONE_API AsmVariant( wchar_t* ptr )
            : AsmVariant( dataPtr, (wcslen( ptr ) + 1) * sizeof(wchar_t), reinterpret_cast<uintptr_t>(ptr) ) { }

        BLACKBONE_API AsmVariant( const wchar_t* ptr )
            : AsmVariant( const_cast<wchar_t*>(ptr) ) { }

        BLACKBONE_API AsmVariant( void* _imm )
            : AsmVariant( imm, sizeof(void*), reinterpret_cast<uintptr_t>(_imm) ) { }

        BLACKBONE_API AsmVariant( const void* _imm )
            : AsmVariant( imm, sizeof(void*), reinterpret_cast<uintptr_t>(_imm) ) { }

        // In MSVS compiler 'long double' and 'double' are both 8 bytes long
        BLACKBONE_API  AsmVariant( long double _imm_fpu )
            : AsmVariant( (double)_imm_fpu ) { }

        BLACKBONE_API AsmVariant( double _imm_fpu )
            : type( imm_double )
            , size( sizeof(double) )
            , imm_double_val( _imm_fpu )
            , new_imm_val( 0 ) { }

        BLACKBONE_API AsmVariant( float _imm_fpu )
            : type( imm_float )
            , size( sizeof(float) )
            , imm_float_val( _imm_fpu )
            , new_imm_val( 0 ) { }

        BLACKBONE_API AsmVariant( asmjit::GpReg _reg )
            : type( reg )
            , size( sizeof( uintptr_t ) )
            , reg_val( _reg )
            , imm_double_val( -1.0 )
            , new_imm_val( 0 ) { }

        // Stack variable
        BLACKBONE_API AsmVariant( asmjit::Mem _mem )
            : type( mem )
            , size( sizeof( uintptr_t ) )
            , mem_val( _mem )
            , imm_double_val( -1.0 ) { }

        // Pointer to stack address
        BLACKBONE_API AsmVariant( asmjit::Mem* _mem )
            : type( mem_ptr )
            , size( sizeof( uintptr_t ) )
            , mem_val( *_mem )
            , imm_double_val( -1.0 )
            , new_imm_val( 0 ) { }

        BLACKBONE_API AsmVariant( const asmjit::Mem* _mem )
            : AsmVariant( const_cast<asmjit::Mem*>(_mem) ) { }

        template <typename T>
        AsmVariant( T* ptr )
            : AsmVariant( dataPtr, sizeof(T), reinterpret_cast<uintptr_t>(ptr) ) { }

        template <typename T>
        AsmVariant( const T* ptr )
        { 
            //
            // Treat function pointer as void*
            //
            const bool isFunction = std::is_function<typename std::remove_pointer<T>::type>::value;
            typedef typename std::conditional<isFunction, void*, T>::type Type;

            type = isFunction ? imm : dataPtr;
            size = sizeof(Type);
            imm_val = reinterpret_cast<uintptr_t>(ptr);
            new_imm_val = 0;
        }

        template <typename T>
        AsmVariant( const T* ptr, size_t size )
            : AsmVariant( dataPtr, size, reinterpret_cast<uintptr_t>(ptr) ) { }

        template <typename T>
        AsmVariant( T& val )
            : AsmVariant( dataPtr, sizeof(T), reinterpret_cast<uintptr_t>(&val) ) { }

        template <typename T>
        AsmVariant( const T& val )
            : AsmVariant( dataPtr, sizeof(T), reinterpret_cast<uintptr_t>(&val) ) {}

        // Pass argument by value case
        // Real RValue reference types are not supported because of ambiguity
        #pragma warning(disable : 4127)
        template <typename T>
        AsmVariant( T&& val )
            : AsmVariant( dataStruct, sizeof(T), reinterpret_cast<uintptr_t>(&val) )
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
                imm_val = reinterpret_cast<uintptr_t>(buf.get());
                memcpy( buf.get(), &val, sizeof(T) );
            }
        }
        #pragma warning(default : 4127)

        BLACKBONE_API AsmVariant( AsmVariant&& other )
        {
            *this = std::move( other );
        }

        BLACKBONE_API AsmVariant( const AsmVariant& other ) = default;
        BLACKBONE_API AsmVariant& operator =(const AsmVariant& other) = default;

        //
        // Get floating point value as raw data
        //
        BLACKBONE_API inline uint32_t getImm_float()  const { return *(reinterpret_cast<const uint32_t*>(&imm_float_val)); }
        BLACKBONE_API inline uint64_t getImm_double() const { return *(reinterpret_cast<const uint64_t*>(&imm_double_val)); }

        /// <summary>
        /// Check if argument can be passed in x86 register
        /// </summary>
        /// <returns>true if can</returns>
        BLACKBONE_API inline bool reg86Compatible() const
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
        AsmVariant( eType _type, size_t _size, uintptr_t val )
            : type( _type )
            , size( _size )
            , imm_val( val )
            , new_imm_val( 0 ) { }

    private:
        eType         type;             // Variable type
        size_t        size;             // Variable size

        asmjit::GpReg reg_val;          // General purpose register
        asmjit::Mem   mem_val;          // Memory pointer

        // Immediate values
        union
        {
            uintptr_t imm_val;
            double    imm_double_val;
            float     imm_float_val;
        };

        uintptr_t     new_imm_val;      // Replaced immediate value for dataPtr type

        // rvalue reference buffer
        std::shared_ptr<uint8_t> buf;
    };

}