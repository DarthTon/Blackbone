#pragma once

namespace blackbone
{

template<typename R, typename... Args, class C>
struct HookHandler<R( __fastcall* )(Args...), C> : public DetourBase
{
    typedef typename std::conditional<std::is_same<R, void>::value, int, R>::type ReturnType;

    typedef R( __fastcall *type )(Args...);
    typedef R( __fastcall *hktype )(Args&...);
    typedef R( C::*hktypeC )(Args&...);

    //
    // Workaround for void return type
    //
    typedef ReturnType( __fastcall *typeR )(Args...);
    typedef ReturnType( __fastcall *hktypeR )(Args&...);
    typedef ReturnType( C::*hktypeCR )(Args&...);

    static __declspec(noinline) ReturnType __fastcall Handler( Args... args )
    {
        HookHandler* pInst = (HookHandler*)((_NT_TIB*)NtCurrentTeb())->ArbitraryUserPointer;
        return pInst->HandlerP( std::forward<Args>( args )... );
    }

    ReturnType HandlerP( Args&&... args )
    {
        ReturnType val_new, val_original;

        DisableHook();

        if (_order == CallOrder::HookFirst)
        {
            val_new = CallCallback( std::forward<Args>( args )... );
            val_original = CallOriginal( std::forward<Args>( args )... );
        }
        else if (_order == CallOrder::HookLast)
        {
            val_original = CallOriginal( std::forward<Args>( args )... );
            val_new = CallCallback( std::forward<Args>( args )... );
        }
        else
        {
            val_original = val_new = CallCallback( std::forward<Args>( args )... );
        }

        if (this->_hooked)
            EnableHook();

        return (_retType == ReturnMethod::UseOriginal ? val_original : val_new);
    }

    inline ReturnType CallOriginal( Args&&... args )
    {
        return (reinterpret_cast<typeR>(_callOriginal))(args...);
    }

    inline ReturnType CallCallback( Args&&... args )
    {
        if (_callbackClass != nullptr)
            return ((C*)_callbackClass->*brutal_cast<hktypeCR>(_callback))(args...);
        else
            return (reinterpret_cast<hktypeR>(_callback))(args...);
    }
};

}