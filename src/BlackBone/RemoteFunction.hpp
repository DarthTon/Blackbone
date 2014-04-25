#pragma once

#include "AsmHelperBase.h"
#include "Process.h"

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
protected:
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
    NTSTATUS Call( T& result, std::vector<AsmVariant>& args, Thread* contextThread = nullptr )
    {
        uint64_t result2 = 0;
        AsmJit::Assembler a;

        // Ensure RPC environment exists
        if (_process.remote().CreateRPCEnvironment() != STATUS_SUCCESS)
            return LastNtStatus();

        // FPU check
        bool isFloat  = std::is_same<T, float>::value;
        bool isDouble = std::is_same<T, double>::value || std::is_same<T, long double>::value;

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
        if (contextThread == nullptr)
            _process.remote().ExecInNewThread( a.make(), a.getCodeSize(), result2 );
        else if (*contextThread == _process.remote()._hWorkThd)
            _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result2 );
        else
            _process.remote().ExecInAnyThread( a.make(), a.getCodeSize(), result2, *contextThread );

        // Get function return value
        _process.remote().GetCallResult<T>( result );

        return STATUS_SUCCESS;
    }

#pragma warning(default : 4127)

    inline type ptr() const { return _pfn; }

private:
    RemoteFuncBase( const RemoteFuncBase& ) = delete;
    RemoteFuncBase& operator =(const RemoteFuncBase&) = delete;

private:
    eCalligConvention _callConv;    // Calling convention
    type              _pfn;         // Function pointer
    Process&          _process;     // Underlying process
};

// Function arguments
template<typename... Args>
class FuncArguments
{
    static const size_t arg_count = sizeof...(Args);

public:

    // Get arguments. Either in variant form, or as declared type
    inline std::vector<AsmVariant>& getArgsRaw() const { return _args; }
    inline std::tuple<Args...>& getArgs() const { return _targs; }

    // Get argument by index
    // Index is zero-based
    template<int pos>
    auto getArg() -> decltype(std::get<pos>( _targs ))
    {
        return std::get<pos>( _targs );
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
        : _process( proc )
        , _targs( static_cast<Args&&>(args)... )
        , _args( std::vector<AsmVariant>{ static_cast<Args&&>(args)... } ) { }

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

#define DECLPFN(CALL_OPT, CALL_DEF, ...) \
template<__VA_ARGS__ typename R, typename... Args > \
class RemoteFunction< R( CALL_OPT* )(Args...) > : public RemoteFuncBase<R( CALL_OPT* )(Args...)>, public FuncArguments<Args...> \
{ \
public: \
    typedef typename std::conditional<std::is_same<R, void>::value, int, R>::type ReturnType; \
    \
public: \
    template<typename... TArgs> \
    RemoteFunction( Process& proc, typename RemoteFuncBase::type ptr, TArgs&&... args ) \
        : RemoteFuncBase( proc, ptr, CALL_DEF ) \
        , FuncArguments( proc, static_cast<Args&&>(args)... ) { } \
        \
    inline DWORD Call( ReturnType& result, Thread* contextThread = nullptr ) \
    { \
        NTSTATUS status = RemoteFuncBase::Call( result, getArgsRaw(), contextThread ); \
        FuncArguments::updateArgs( ); \
        return status; \
    } \
}

//
// Calling convention specialization
//
DECLPFN( __cdecl, cc_cdecl );

// Under AMD64 these will be same declarations as __cdecl, so compilation will fail.
#ifdef _M_IX86
DECLPFN( __stdcall,  cc_stdcall );
DECLPFN( __thiscall, cc_thiscall );
DECLPFN( __fastcall, cc_fastcall );
#endif

// Class member function
template< class C, typename R, typename... Args >
class RemoteFunction< R( C::* )(Args...) > : public RemoteFuncBase<R( C::* )(const C*, Args...)>, public FuncArguments<const C*, Args...>
{
public: 
    typedef typename std::conditional<std::is_same<R, void>::value, int, R>::type ReturnType;
    typedef typename ReturnType( C::*type )(Args...);

public: 
    RemoteFunction( Process& proc, type ptr )
        : RemoteFuncBase( proc, ptr, cc_thiscall )
        , FuncArguments( proc ) { }
    
    RemoteFunction( Process& proc, type ptr, const C* pClass, const Args&... args )
        : RemoteFuncBase( proc, (typename RemoteFuncBase::type)ptr, cc_thiscall )
        , FuncArguments( proc, pClass, args... ) { }
        
    inline DWORD Call( R& result, Thread* contextThread = nullptr ) 
    {
        NTSTATUS status = RemoteFuncBase::Call( result, getArgsRaw(), contextThread );
        FuncArguments::updateArgs();
        return status;
    } 
};

}