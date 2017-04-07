#pragma  once


#include "../Config.h"
#pragma warning(disable : 4100)
#include "../../../contrib/AsmJit/AsmJit.h"
#pragma warning(default : 4100)

#include <memory>


namespace blackbone
{
/// <summary>
/// General purpose assembly variable
/// </summary>
struct AsmVariant
{
    template<typename T>
    using cleanup_t = typename std::decay<typename std::remove_pointer<T>::type>::type;

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

    template<typename T>
    AsmVariant( T&& arg )
        : new_imm_val( 0 )
    {
        Init( std::forward<T>( arg ) );
    }

    // Custom size pointer
    template <typename T>
    explicit AsmVariant( T* ptr, size_t size_ )
        : type( dataPtr )
        , size( size_ )
        , imm_val64( reinterpret_cast<uint64_t>(ptr) ) { }

    // bool, short, int, unsigned long long, etc.
    template<typename T>
    typename std::enable_if<!std::is_pointer<T>::value && (std::is_integral<T>::value || std::is_enum<T>::value)>::type
        Init( T arg )
    {
        set( imm, sizeof( std::decay<T>::type ), static_cast<uint64_t>(arg) );
    }

    // char*, const char*, char[], etc.
    template<typename T>
    typename std::enable_if<std::is_pointer<T>::value && std::is_same<cleanup_t<T>, char>::value>::type
        Init( T arg )
    {
        set( dataPtr, strlen( arg ) + 1, reinterpret_cast<uint64_t>(arg) );
    }

    // wchar_t*, const wchar_t*, wchar[], etc.
    template<typename T>
    typename std::enable_if<std::is_pointer<T>::value && std::is_same<cleanup_t<T>, wchar_t>::value>::type
        Init( T arg )
    {
        set( dataPtr, (wcslen( arg ) + 1) * sizeof( wchar_t ), reinterpret_cast<uint64_t>(arg) );
    }

    // void*, const void*, etc.
    template<typename T>
    typename std::enable_if<std::is_pointer<T>::value && std::is_same<cleanup_t<T>, void>::value>::type
        Init( T arg )
    {
        set( imm, sizeof( T ), reinterpret_cast<uint64_t>(arg) );
    }

    // Function pointer
    template<typename T>
    typename std::enable_if<std::is_function<typename std::remove_pointer<T>::type>::value>::type
        Init( T arg )
    {
        set( imm, sizeof( T ), reinterpret_cast<uint64_t>(arg) );
    }

    // Arbitrary pointer
    template<typename T>
    typename std::enable_if<
        std::is_pointer<T>::value &&
        !std::is_function<typename std::remove_pointer<T>::type>::value &&
        !std::is_same<cleanup_t<T>, void>::value &&
        !std::is_same<cleanup_t<T>, char>::value &&
        !std::is_same<cleanup_t<T>, wchar_t>::value>::type
        Init( T arg )
    {
        set( dataPtr, sizeof( std::remove_pointer<T>::type ), reinterpret_cast<uint64_t>(arg) );
    }

    // Arbitrary variable passed by value
    template<typename T>
    typename std::enable_if<
        !std::is_pointer<typename std::decay<T>::type>::value &&
        !std::is_integral<cleanup_t<T>>::value &&
        !std::is_enum<cleanup_t<T>>::value
        >::type
        Init( T&& arg )
    {
        // Can fit into register
        size = sizeof( T );
        if (size <= sizeof( uintptr_t ))
        {
            type = imm;
            memcpy( &imm_val64, &arg, sizeof( T ) );
        }
        else
        {
            buf.reset( new uint8_t[sizeof( T )] );
            set( dataStruct, sizeof( T ), reinterpret_cast<uint64_t>(buf.get()) );
            memcpy( buf.get(), &arg, sizeof( T ) );
        }  
    }

    BLACKBONE_API AsmVariant( float _imm_fpu )
        : type( imm_float )
        , size( sizeof( float ) )
        , imm_float_val( _imm_fpu ) { }

    BLACKBONE_API AsmVariant( double _imm_fpu )
        : type( imm_double )
        , size( sizeof( double ) )
        , imm_double_val( _imm_fpu ) { }

    BLACKBONE_API AsmVariant( asmjit::GpReg _reg )
        : type( reg )
        , size( sizeof( uintptr_t ) )
        , reg_val( _reg ) { }

    // Stack variable
    BLACKBONE_API AsmVariant( asmjit::Mem _mem )
        : type( mem )
        , size( sizeof( uintptr_t ) )
        , mem_val( _mem ) { }

    // Pointer to stack address
    BLACKBONE_API AsmVariant( asmjit::Mem* _mem )
        : type( mem_ptr )
        , size( sizeof( uintptr_t ) )
        , mem_val( *_mem ) { }

    BLACKBONE_API AsmVariant( const asmjit::Mem* _mem )
        : AsmVariant( const_cast<asmjit::Mem*>(_mem) ) { }

    BLACKBONE_API AsmVariant( const AsmVariant& ) = default;
    BLACKBONE_API AsmVariant( AsmVariant&& ) = default;
    BLACKBONE_API AsmVariant& operator =( const AsmVariant& ) = default;

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
        /*if (type == imm && size > sizeof( uint32_t ))
            return false;*/

        return true;
    }

    inline void set( eType type_, size_t size_, uint64_t val )
    {
        type = type_;
        size = size_;
        imm_val64 = val;
    }

    eType  type = noarg;    // Variable type
    size_t size = 0;        // Variable size
    asmjit::GpReg reg_val;  // General purpose register
    asmjit::Mem   mem_val;  // Memory pointer

    // Immediate values
    union
    {
        uint64_t  imm_val64 = 0;
        uintptr_t imm_val;
        uint32_t  imm_val32;
        double    imm_double_val;
        float     imm_float_val;
    };

    uint64_t new_imm_val = 0;       // Replaced immediate value for dataPtr type
    std::shared_ptr<uint8_t> buf;   // Value buffer
};

/// <summary>
/// Remote function pointer
/// </summary>
struct AsmFunctionPtr: public AsmVariant
{
    AsmFunctionPtr( int ptr )
        : AsmVariant( ptr ) { }

    AsmFunctionPtr( unsigned int ptr )
        : AsmVariant( ptr ) { }

    AsmFunctionPtr( long ptr )
        : AsmVariant( ptr ) { }

    AsmFunctionPtr( unsigned long ptr )
        : AsmVariant( ptr ) { }

    AsmFunctionPtr( long long ptr )
        : AsmVariant( ptr ) { }

    AsmFunctionPtr( unsigned long long ptr )
        : AsmVariant( ptr ) { }

    AsmFunctionPtr( void* ptr )
        : AsmVariant( reinterpret_cast<uintptr_t>(ptr) ) { }

    AsmFunctionPtr( const void* ptr )
        : AsmVariant( reinterpret_cast<uintptr_t>(ptr) ) { }

    AsmFunctionPtr( asmjit::GpReg reg_ )
        : AsmVariant( reg_ ) { }
};

}