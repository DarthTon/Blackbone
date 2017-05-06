#pragma once
#if _MSC_VER >= 1910

#include <optional>
#include <cassert>

namespace blackbone
{
/// <summary>
/// Function result or failure status
/// </summary>
template <typename T>
struct call_result_t
{
    typedef T type;

    NTSTATUS status = STATUS_UNSUCCESSFUL;          // Execution status
    std::optional<T> result_data = std::nullopt;    // Returned value

    call_result_t() = default;

    call_result_t( const T& result_, NTSTATUS status_ = STATUS_SUCCESS )
        : status ( status_ )
        , result_data ( std::move( result_ ) )
    {
        assert( result_data.has_value() );
    }

    call_result_t( NTSTATUS status_ )
        : status ( status_ ) 
    {
        assert( status_ != STATUS_SUCCESS );
    }

    inline bool success() const { return NT_SUCCESS( status ); }
    inline T& result() { return result_data.value(); }
    inline const T& result() const { return result_data.value(); }
    inline T result( const T& def_val ) const { return result_data.value_or( def_val ); }

    inline explicit operator bool() const { return NT_SUCCESS( status ); }
    inline explicit operator T() const    { return result_data.value(); }

    inline T* operator ->() { return &result_data.value(); }
    inline T& operator *()  { return result_data.value(); }
};
}
#endif