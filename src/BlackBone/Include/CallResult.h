#pragma once
#if _MSC_VER >= 1910

#include <optional>
#include <cassert>

namespace blackbone
{
template <typename T>
struct call_result_t
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    std::optional<T> result_data = std::nullopt;

    call_result_t() = default;

    call_result_t( const std::optional<T>& result_, NTSTATUS status_ = STATUS_SUCCESS )
        : status ( status_ )
        , result_data ( std::move( result_ ) )
    {
        debug_check();
    }

    call_result_t( NTSTATUS status_ )
        : status ( status_ ) 
    {
        debug_check();
    }

    inline bool success() const { return NT_SUCCESS( status ); }
    //inline operator bool() const { return NT_SUCCESS( status ); }

    inline T& result() { return result_data.value(); }
    inline T result( const T& def_val ) const { return result_data.value_or( def_val ); }

    inline void debug_check()
    {
#ifdef _DEBUG
        if (NT_SUCCESS( status ))
            assert( result_data.has_value() );
#endif
    }

};
}
#endif