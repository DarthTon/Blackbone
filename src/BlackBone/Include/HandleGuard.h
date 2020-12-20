#pragma once
#include "Winheaders.h"
#include <memory>

namespace blackbone
{

template<typename T>
struct non_zero
{
    static bool call( T handle ) noexcept
    {
        return intptr_t( handle ) != 0;
    }
};

template<typename T>
struct non_negative
{
    static bool call( T handle ) noexcept
    {
        return intptr_t( handle ) > 0;
    }
};

template<template<typename> typename wrapped_t, typename T>
struct with_pseudo_t
{
    static bool call( T handle ) noexcept
    {
        if (wrapped_t<T>::call( handle ))
            return true;

        // Check if it's a pseudo handle
        auto h = (HANDLE)(uintptr_t)handle;
        return h == GetCurrentProcess() || h == GetCurrentThread();
    }
};

template<template<typename> typename wrapped_t>
struct with_pseudo
{
    template<typename T>
    using type = with_pseudo_t<wrapped_t, T>;
};

/// <summary>
/// Strong exception guarantee
/// </summary>
template<typename handle_t, auto close, template<typename> typename is_valid = non_negative>
class HandleGuard
{
public:
    static constexpr handle_t zero_handle = handle_t( 0 );

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
        if (non_negative<handle_t>::call( _handle ))
            close( _handle );
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

        if (non_negative<handle_t>::call( _handle ))
            close( _handle );

        _handle = handle;
    }

    handle_t release() noexcept
    {
        auto tmp = _handle;
        _handle = zero_handle;
        return tmp;
    }

    handle_t get() const noexcept { return _handle; }
    bool valid() const noexcept { return is_valid<handle_t>::call( _handle ); }

    operator handle_t() const noexcept { return _handle; }
    explicit operator bool() const noexcept { return valid(); }

    handle_t* operator &() noexcept { return &_handle; }

    bool operator ==( const HandleGuard& rhs ) const noexcept { return _handle == rhs._handle; }
    bool operator <( const HandleGuard& rhs ) const noexcept { return _handle < rhs._handle; }

private:
    handle_t _handle;
};


using Handle        = HandleGuard<HANDLE, &CloseHandle>;
using ProcessHandle = HandleGuard<HANDLE, &CloseHandle, with_pseudo<non_negative>::type>;
using ACtxHandle    = HandleGuard<HANDLE, &ReleaseActCtx>;
using RegHandle     = HandleGuard<HKEY, &RegCloseKey>;
using Mapping       = HandleGuard<void*, & UnmapViewOfFile, non_zero>;

}