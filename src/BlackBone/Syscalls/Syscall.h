#pragma once
#include "../Config.h"
#include "../Include/Winheaders.h"
#include <type_traits>
#include <cstdint>

extern "C" void* syscall_stub();

namespace blackbone
{
namespace syscall
{
    template<typename T>
    using to_int64 = std::conditional_t<sizeof( T ) < sizeof( int64_t ), int64_t, T>;

#pragma warning(push)
#pragma warning(disable : 4100)

    template<typename R, typename... Args>
    R syscall( int index, Args... args )
    {
        auto error = []( NTSTATUS status )
        {
            if constexpr (std::is_same_v<R, NTSTATUS>)
                return status;
            else
                return R();
        };

#ifdef USE32
        return error( STATUS_NOT_SUPPORTED );
#else
        static_assert(sizeof( R ) <= sizeof( void* ), "Return types larger than void* aren't supported");
        if (index == -1)
            return error( STATUS_INVALID_PARAMETER_1 );

        // Cast types that otherwise will be only half-initialized
        auto pfn = reinterpret_cast<R(*)( int, size_t, to_int64<Args>... )>(syscall_stub);
        return pfn( index, sizeof...( Args ), to_int64<Args>( args )... );
#endif
    }

    template<typename... Args>
    NTSTATUS nt_syscall( int index, Args&&... args )
    {
        return syscall<NTSTATUS>( index, std::forward<Args>( args )... );
    }

    inline int get_index( const wchar_t* modName, const char* func )
    {
#ifdef USE32
        // Doesn't work for x86
        return -1;
#else
        const auto pfn = reinterpret_cast<const uint8_t*>(GetProcAddress(
            GetModuleHandleW( modName ),
            func
        ));

        return pfn ? *reinterpret_cast<const int*>(pfn + 4) : -1;
#endif
    }

    inline int get_index( const char* func )
    {
        return get_index( L"ntdll.dll", func );
    }

#pragma warning(pop)
}
}
