#pragma once
#include "Winheaders.h"

namespace blackbone
{

/// <summary>
/// Strong exception guarantee
/// </summary>
template<typename handle_t, auto close_fn>
class HandleGuard
{
public:
    explicit HandleGuard( handle_t handle = zero_handle ) noexcept
        : _handle( handle ) 
    { 
    }

    HandleGuard( HandleGuard&& rhs ) noexcept
        : _handle( rhs._handle )
    {
        rhs._handle = zero_handle;
    }

    ~HandleGuard()
    {
        if (valid())
            close_fn( _handle );
    }

    HandleGuard( const HandleGuard& ) = delete;
    HandleGuard& operator =( const HandleGuard& ) = delete;

    HandleGuard& operator =( HandleGuard&& rhs ) noexcept
    {
        if (std::addressof( rhs ) == this)
            return *this;

        reset( rhs._handle );
        rhs._handle = zero_handle;

        return *this;
    }

    HandleGuard& operator =( handle_t handle ) noexcept
    {
        reset( handle );
        return *this;
    }

    void reset( handle_t handle = zero_handle ) noexcept
    {
        if (handle == _handle)
            return;

        if (valid())
            close_fn( _handle );

        _handle = handle;
    }

    handle_t release() noexcept
    {
        auto tmp = _handle;
        _handle = zero_handle;
        return tmp;
    }

    handle_t get() const noexcept { return _handle; }
    bool valid() const noexcept { return reinterpret_cast<intptr_t>(_handle) > reinterpret_cast<intptr_t>(zero_handle); }

    operator handle_t() const noexcept { return _handle; }
    explicit operator bool() const noexcept { return valid(); }

    handle_t* operator &() noexcept { return &_handle; }

    bool operator ==( const HandleGuard& rhs ) const noexcept { return _handle == rhs._handle; }
    bool operator <( const HandleGuard& rhs ) const noexcept { return _handle < rhs._handle; }

private:
    static constexpr handle_t zero_handle = handle_t( 0 );
    handle_t _handle;
};

using Handle        = HandleGuard<HANDLE, &CloseHandle>;
using ACtxHandle    = HandleGuard<HANDLE, &ReleaseActCtx>;
using RegHandle     = HandleGuard<HKEY,   &RegCloseKey>;

}