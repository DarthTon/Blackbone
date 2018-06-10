#pragma  once


#include "../Config.h"
#pragma warning(disable : 4100)
#include "../../3rd_party/AsmJit/AsmJit.h"
#pragma warning(default : 4100)

#include <vector>


namespace blackbone
{
/// <summary>
/// General purpose assembly variable
/// </summary>
struct AsmVariant
{
    template<typename T>
    using cleanup_t = std::remove_cv_t<std::remove_pointer_t<T>>;

    template<typename T, typename S>
    static constexpr bool is_string_ptr = (std::is_pointer_v<T> && std::is_same_v<cleanup_t<T>, S>);

    template<typename T>
    static constexpr bool is_number = (std::is_integral_v<T> || std::is_enum_v<T>);

    template<typename T>
    static constexpr bool is_void_ptr = (std::is_pointer_v<T> && std::is_void_v<cleanup_t<T>>);

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
    {
        using RAW_T = std::decay_t<T>;
        constexpr size_t argSize = sizeof( RAW_T );

        // bool, short, int, unsigned long long, etc.
        if constexpr (is_number<RAW_T>)
        {
            set( imm, argSize, static_cast<uint64_t>(arg) );
        }
        // Array of elements
        else if constexpr(std::is_array_v<std::remove_reference_t<T>>)
        {
            set( dataPtr, sizeof( arg ), reinterpret_cast<uint64_t>(arg) );
        }
        // char*, const char*, etc.
        else if constexpr(is_string_ptr<RAW_T, char>)
        {
            set( dataPtr, strlen( arg ) + 1, reinterpret_cast<uint64_t>(arg) );
        }
        // wchar_t*, const wchar_t*, etc.
        else if constexpr(is_string_ptr<RAW_T, wchar_t>)
        {
            set( dataPtr, (wcslen( arg ) + 1) * sizeof( wchar_t ), reinterpret_cast<uint64_t>(arg) );
        }
        // void*, const void*, etc.
        else if constexpr(is_void_ptr<RAW_T>)
        {
            set( imm, argSize, reinterpret_cast<uint64_t>(arg) );
        }
        // dirty hack to threat HWND as a simple pointer
        else if constexpr(std::is_same_v<RAW_T, HWND>)
        {
            set( imm, argSize, reinterpret_cast<uint64_t>(arg) );
        }
        // Function pointer
        else if constexpr(std::is_function_v<cleanup_t<RAW_T>>)
        {
            set( imm, argSize, reinterpret_cast<uint64_t>(arg) );
        }
        // Arbitrary pointer
        else if constexpr(std::is_pointer_v<RAW_T>)
        {
            set( dataPtr, sizeof( cleanup_t<RAW_T> ), reinterpret_cast<uint64_t>(arg) );
        }
        // Arbitrary variable passed by value
        // Can fit into register
        else if constexpr (argSize <= sizeof( uintptr_t ))
        {
            type = imm;
            size = argSize;
            memcpy( &imm_val64, &arg, argSize );
        }
        else
        {
            buf.resize( argSize );
            set( dataStruct, argSize, reinterpret_cast<uint64_t>(buf.data()) );
            memcpy( buf.data(), &arg, argSize );
        }
    }

    // Custom size pointer
    template <typename T>
    explicit AsmVariant( T* ptr, size_t size_ )
        : type( dataPtr )
        , size( size_ )
        , imm_val64( reinterpret_cast<uint64_t>(ptr) ) { }

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
    std::vector<uint8_t> buf;       // Value buffer
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