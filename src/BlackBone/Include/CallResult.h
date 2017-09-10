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
    NTSTATUS status = STATUS_UNSUCCESSFUL;          // Execution status
    std::optional<T> result_data = std::nullopt;    // Returned value

    call_result_t() = default;

    call_result_t( T result_, NTSTATUS status_ = STATUS_SUCCESS )
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

#else

#include <memory>
#include <cassert>

namespace blackbone
{
    /// <summary>
    /// Function result or failure status
    /// </summary>
    template <typename T>
    struct call_result_t
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;          // Execution status
        std::unique_ptr<T> result_data;                 // Returned value

        call_result_t() = default;

        call_result_t(T result_, NTSTATUS status_ = STATUS_SUCCESS)
            : status(status_)
            , result_data(std::make_unique<T>(std::move(result_)))
        {
            assert(result_data.get());
        }

        call_result_t(NTSTATUS status_)
            : status(status_)
        {
            assert(status_ != STATUS_SUCCESS);
        }


    private:
        inline T* value() {
            if (!result_data) {
                throw std::logic_error("bad optional access.");
            }
            return result_data.get();
        }
    public:

        inline bool success() const { return NT_SUCCESS( status ); }
        inline T& result() { return *value(); }
        inline const T& result() const { return *value(); }
        inline T result(const T& def_val) const { return result_data ? *result_data.get() : def_val; }

        inline explicit operator bool() const { return NT_SUCCESS( status ); }
        inline explicit operator T() const { return *value(); }

        inline T* operator ->() { return value(); }
        inline T& operator *() { return *value(); }
    };
}
#endif