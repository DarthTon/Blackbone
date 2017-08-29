#pragma once
#include "Winheaders.h"

namespace blackbone
{

/// <summary>
/// Strong exception guarantee
/// </summary>
template<typename handle_t, typename close_t, close_t close_fn, handle_t zero_handle>
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
        if (_handle != zero_handle)
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

        if (_handle != zero_handle)
            close_fn( _handle );

        _handle = handle;
    }

    handle_t release() noexcept
    {
        auto tmp = _handle;
        _handle = zero_handle;
        return tmp;
    }

    inline handle_t get() const noexcept { return _handle; }

    inline operator handle_t() const noexcept { return _handle; }
    inline explicit operator bool() const noexcept { return _handle != zero_handle; }

    inline handle_t* operator &() noexcept { return &_handle; }

    inline bool operator ==( const HandleGuard& rhs ) const noexcept { return _handle == rhs._handle; }
    inline bool operator <( const HandleGuard& rhs ) const noexcept { return _handle < rhs._handle; }

private:
    handle_t _handle;
};

using Handle        = HandleGuard<HANDLE, decltype(&CloseHandle), &CloseHandle, nullptr>;
using FileHandle    = HandleGuard<HANDLE, decltype(&CloseHandle), &CloseHandle, INVALID_HANDLE_VALUE>;
using ACtxHandle    = HandleGuard<HANDLE, decltype(&ReleaseActCtx), &ReleaseActCtx, INVALID_HANDLE_VALUE>;
using FileMapHandle = HandleGuard<void*, decltype(&UnmapViewOfFile), &UnmapViewOfFile, nullptr>;
using SnapHandle    = FileHandle;


}