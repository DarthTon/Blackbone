#pragma once
#include <utility>

namespace blackbone
{

#define SCOPE_EXIT_CAT2(x, y) x##y
#define SCOPE_EXIT_CAT(x, y) SCOPE_EXIT_CAT2(x, y)
#define ON_SCOPE_EXIT auto SCOPE_EXIT_CAT(scopeExit_, __COUNTER__) = MakeScopeExit() += [&]

template<typename F>
class ScopeExit
{
public:
    ScopeExit( F&& f )
        : _callback( f )
    {
    }

    ~ScopeExit()
    {
        _callback();
    }

    ScopeExit( ScopeExit&& other ) = default;
    ScopeExit( const ScopeExit& ) = delete;
    ScopeExit& operator=( const ScopeExit& ) = delete;

private:
    F _callback;
};

struct MakeScopeExit
{
    template<typename F>
    ScopeExit<F> operator+=( F&& f )
    {
        return ScopeExit<F>( std::move( f ) );
    }
};

}