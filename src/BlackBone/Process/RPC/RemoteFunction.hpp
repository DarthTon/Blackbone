#pragma once

#include "../../Asm/AsmHelperBase.h"
#include "../Process.h"

// TODO: Find more elegant way to deduce calling convention
//       than defining each one manually

namespace blackbone
{
    
/// <summary>
/// Base class for remote function pointer
/// </summary>
template< typename Fn >
class RemoteFuncBase
{
public:
    typedef Fn type;    // Function pointer type

protected:

    RemoteFuncBase( Process& proc, eCalligConvention conv )
        : _callConv( conv )
        , _process( proc )
        , _pfn( nullptr ) { }

    RemoteFuncBase( Process& proc, type ptr, eCalligConvention conv )
        : _callConv( conv )
        , _process( proc )
        , _pfn( ptr )  { }

// conditional expression is constant
#pragma warning(disable : 4127)

    /// <summary>
    /// Perform remote function call
    /// </summary>
    /// <param name="result">Function result</param>
    /// <param name="args">Function arguments</param>
    /// <param name="contextThread">Execution thread</param>
    /// <returns>Call status</returns>
    template<typename T>
    call_result_t<T> Call( std::vector<AsmVariant>& args, ThreadPtr contextThread = nullptr )
    {
        T result;
        uint64_t tmpResult = 0;
        NTSTATUS status = STATUS_SUCCESS;
        AsmJitHelper a;

        // Ensure RPC environment exists
        status = _process.remote().CreateRPCEnvironment( contextThread == _process.remote().getWorker(), contextThread != nullptr );
        if (!NT_SUCCESS( status ))
            return call_result_t<T>( result, status );

        // FPU check
        constexpr bool isFloat  = std::is_same<T, float>::value;
        constexpr bool isDouble = std::is_same<T, double>::value || std::is_same<T, long double>::value;

        // Deduce return type
        eReturnType retType = rt_int32;

        if (isFloat)
            retType = rt_float;
        else if (isDouble)
            retType = rt_double;
        else if (sizeof(T) == sizeof(uint64_t))
            retType = rt_int64;
        else if (!std::is_reference<T>::value && sizeof(T) > sizeof(uint64_t))
            retType = rt_struct;

        auto pfnNew = brutal_cast<const void*>(_pfn);

        _process.remote().PrepareCallAssembly( a, pfnNew, args, _callConv, retType );

        // Choose execution thread
        if (!contextThread)
            status = _process.remote().ExecInNewThread( a->make(), a->getCodeSize(), tmpResult );
        else if (contextThread == _process.remote()._hWorkThd)
            status = _process.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), tmpResult );
        else
            status = _process.remote().ExecInAnyThread( a->make(), a->getCodeSize(), tmpResult, contextThread );

        // Get function return value
        if (!NT_SUCCESS( status ) || !NT_SUCCESS( status = _process.remote().GetCallResult( result ) ))
            return call_result_t<T>( result, status );

        return call_result_t<T>( result, STATUS_SUCCESS );
    }

#pragma warning(default : 4127)

    inline type ptr() const { return _pfn; }

private:
    RemoteFuncBase( const RemoteFuncBase& ) = delete;
    RemoteFuncBase& operator =(const RemoteFuncBase&) = delete;

private:
    eCalligConvention _callConv;    // Calling convention
    Process&          _process;     // Underlying process
    type              _pfn;         // Function pointer
};

// Function arguments
template<typename... Args>
class FuncArguments
{
    static const size_t arg_count = sizeof...(Args);

public:

    // Get arguments. Either in variant form, or as declared type
    inline std::vector<AsmVariant>& getArgsRaw() const { return _args; }
    inline const std::tuple<Args...>& getArgs() const { return _targs; }

    // Get argument by index
    // Index is zero-based
    template<int pos>
    decltype(std::get<pos>( std::tuple<Args...>() )) getArg()
    {
        return std::get<pos>( this->_targs );
    }

    // Update any changes to arguments passed by reference or pointer
    void updateArgs() const
    {
        for (auto& arg : _args)
            if (arg.type == AsmVariant::dataPtr)
                _process.memory().Read( arg.new_imm_val, arg.size, (void*)arg.imm_val );
    }

    // Manually set argument to custom value
    void setArg( int pos, const AsmVariant& newVal )
    {
        if (_args.size() > (size_t)pos)
            _args[pos] = newVal;
    }

protected:
    template<typename... TArgs>
    FuncArguments( Process& proc, TArgs&&... args )
        : _args( std::vector < AsmVariant > { static_cast<Args&&>(args)... } ) 
        , _targs( static_cast<Args&&>(args)... )
        , _process( proc ) { }

private:
    FuncArguments( const FuncArguments& ) = delete;
    FuncArguments& operator =(const FuncArguments&) = delete;

private:
    mutable std::vector<AsmVariant> _args;  // Generic arguments
    mutable std::tuple<Args...> _targs;     // Real arguments
    Process& _process;                      // Process routines
};

// Remote function pointer
template< typename Fn >
class RemoteFunction;

#define DECLPFN(CALL_OPT, CALL_DEF) \
template< typename R, typename... Args > \
class RemoteFunction< R( CALL_OPT* )(Args...) > : public RemoteFuncBase<R( CALL_OPT* )(Args...)>, public FuncArguments<Args...> \
{ \
public: \
    typedef typename std::conditional<std::is_same<R, void>::value, int, R>::type ReturnType; \
    \
public: \
    template<typename... TArgs> \
    RemoteFunction( Process& proc, typename RemoteFuncBase<R( CALL_OPT* )(Args...)>::type ptr, TArgs&&... args ) \
        : RemoteFuncBase<R( CALL_OPT* )(Args...)>( proc, ptr, CALL_DEF ) \
        , FuncArguments<Args...>( proc, std::forward<Args>(args)... ) { } \
        \
    template<typename... TArgs> \
    RemoteFunction( Process& proc, ptr_t ptr, TArgs&&... args ) \
        : RemoteFuncBase<R( CALL_OPT* )(Args...)>( proc, reinterpret_cast<typename RemoteFuncBase<R( CALL_OPT* )(Args...)>::type>(ptr), CALL_DEF ) \
        , FuncArguments<Args...>( proc, std::forward<Args>(args)... ) { } \
        \
    inline call_result_t<ReturnType> Call( ThreadPtr contextThread = nullptr ) \
    { \
        auto r = RemoteFuncBase<R( CALL_OPT* )(Args...)>::Call<ReturnType>( FuncArguments<Args...>::getArgsRaw(), contextThread ); \
        FuncArguments<Args...>::updateArgs( ); \
        return r; \
    } \
}

//
// Calling convention specialization
//
DECLPFN( __cdecl, cc_cdecl );

// Under AMD64 these will be same declarations as __cdecl, so compilation will fail.
#ifdef USE32
DECLPFN( __stdcall,  cc_stdcall  );
DECLPFN( __thiscall, cc_thiscall );
DECLPFN( __fastcall, cc_fastcall );
#endif

// Class member function
template< class C, typename R, typename... Args >
class RemoteFunction< R( C::* )(Args...) > : public RemoteFuncBase<R( C::* )(C*, Args...)>, public FuncArguments<C*, Args...>
{
public: 
    typedef typename std::conditional<std::is_same<R, void>::value, int, R>::type ReturnType;
    typedef ReturnType( C::*type )(Args...);

public: 
    RemoteFunction( Process& proc, type ptr )
        : RemoteFuncBase( proc, ptr, cc_thiscall )
        , FuncArguments( proc ) { }
    
    RemoteFunction( Process& proc, type ptr, C* pClass, Args&&... args )
        : RemoteFuncBase( proc, reinterpret_cast<typename RemoteFuncBase::type>(ptr), cc_thiscall )
        , FuncArguments( proc, pClass, args... ) { }
        
    inline call_result_t<R> Call( ThreadPtr contextThread = nullptr )
    {
        auto r = RemoteFuncBase<R( C::* )(C*, Args...)>::Call<R>( getArgsRaw(), contextThread );
        FuncArguments<C*, Args...>::updateArgs();
        return r;
    } 
};

}