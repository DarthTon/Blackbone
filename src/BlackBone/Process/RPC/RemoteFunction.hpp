#pragma once

#include "../../Include/CallResult.h"
#include "../../Asm/IAsmHelper.h"
#include "../Process.h"

#include <type_traits>

// TODO: Find more elegant way to deduce calling convention
//       than defining each one manually

namespace blackbone
{
template<eCalligConvention Conv, typename R, typename... Args>
class RemoteFunctionBase
{
public:
    using ReturnType = std::conditional_t<std::is_same_v<R, void>, int, R>;

    struct CallArguments
    {
        CallArguments( const Args&... args )
            : arguments{ AsmVariant( args )... }
        { 
        }

        template<size_t... S>
        CallArguments( const std::tuple<Args...>& args, std::index_sequence<S...> )
            : arguments{ std::get<S>( args )... }
        {
        }

        CallArguments( const std::initializer_list<AsmVariant>& args )
            : arguments{ args }
        {
            // Since initializer_list can't be moved from, dataStruct types must be fixed
            for (auto& arg : arguments)
            {
                if (!arg.buf.empty())
                    arg.imm_val = reinterpret_cast<uintptr_t>(arg.buf.data());
            }
        }

        // Manually set argument to custom value
        void set( int pos, const AsmVariant& newVal )
        {
            if (arguments.size() > static_cast<size_t>(pos))
                arguments[pos] = newVal;
        }

        std::vector<AsmVariant> arguments;
    };

public:
    RemoteFunctionBase( Process& proc, ptr_t ptr, ThreadPtr boundThread = nullptr )
        : _process( proc )
        , _ptr( ptr )
        , _boundThread( boundThread )
    {
        static_assert(
            (... && !std::is_reference_v<Args>),
            "Please replace reference type with pointer type in function type specification"
            );
    }

    call_result_t<ReturnType> Call( CallArguments& args, ThreadPtr contextThread = nullptr )
    {
        ReturnType result = {};
        uint64_t tmpResult = 0;
        NTSTATUS status = STATUS_SUCCESS;
        auto a = AsmFactory::GetAssembler( _process.core().isWow64() );

        if (!contextThread)
            contextThread = _boundThread;

        // Ensure RPC environment exists
        status = _process.remote().CreateRPCEnvironment( Worker_None, contextThread != nullptr );
        if (!NT_SUCCESS( status ))
            return call_result_t<ReturnType>( result, status );

        // FPU check
        constexpr bool isFloat = std::is_same_v<ReturnType, float>;
        constexpr bool isDouble = std::is_same_v<ReturnType, double> || std::is_same_v<ReturnType, long double>;

        // Deduce return type
        eReturnType retType = rt_int32;

        if constexpr (isFloat)
            retType = rt_float;
        else if constexpr (isDouble)
            retType = rt_double;
        else if constexpr (sizeof( ReturnType ) == sizeof( uint64_t ))
            retType = rt_int64;
        else if constexpr (!std::is_reference_v<ReturnType> && sizeof( ReturnType ) > sizeof( uint64_t ))
            retType = rt_struct;

        _process.remote().PrepareCallAssembly( *a, _ptr, args.arguments, Conv, retType );

        // Choose execution thread
        if (!contextThread)
        {
            status = _process.remote().ExecInNewThread( (*a)->make(), (*a)->getCodeSize(), tmpResult );
        }
        else if (contextThread == _process.remote().getWorker())
        {
            status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), tmpResult );
        }
        else
        {
            status = _process.remote().ExecInAnyThread( (*a)->make(), (*a)->getCodeSize(), tmpResult, contextThread );
        }

        // Get function return value
        if (!NT_SUCCESS( status ) || !NT_SUCCESS( status = _process.remote().GetCallResult( result ) ))
            return call_result_t<ReturnType>( result, status );

        // Update arguments
        for (auto& arg : args.arguments)
            if (arg.type == AsmVariant::dataPtr)
                _process.memory().Read( arg.new_imm_val, arg.size, reinterpret_cast<void*>(arg.imm_val) );

        return call_result_t<ReturnType>( result, STATUS_SUCCESS );
    }

    call_result_t<ReturnType> Call( const Args&... args )
    {
        CallArguments a( args... );
        return Call( a, nullptr );
    }

    call_result_t<ReturnType> Call( const std::tuple<Args...>& args, ThreadPtr contextThread = nullptr ) 
    { 
        CallArguments a( args, std::index_sequence_for<Args...>() ); 
        return Call( a, contextThread ); 
    } 

    call_result_t<ReturnType> Call( const std::initializer_list<AsmVariant>& args, ThreadPtr contextThread = nullptr ) 
    { 
        CallArguments a( args ); 
        return Call( a, contextThread ); 
    } 

    call_result_t<ReturnType> operator()( const Args&... args ) 
    { 
        CallArguments a( args... ); 
        return Call( a ); 
    } 

    auto MakeArguments( const Args&... args ) 
    { 
        return CallArguments( args... ); 
    } 
        
    auto MakeArguments( const std::initializer_list<AsmVariant>& args ) 
    { 
        return CallArguments( args ); 
    } 

    auto MakeArguments( const std::tuple<Args...>& args ) 
    { 
        return CallArguments( args, std::index_sequence_for<Args...>() ); 
    }

    void BindToThread(ThreadPtr thread) { _boundThread = thread; }

    bool valid() const { return _ptr != 0; }
    explicit operator bool() const { return valid(); }

private:
    Process& _process;
    ptr_t _ptr = 0;
    ThreadPtr _boundThread = nullptr;
};

// Remote function pointer
template<typename Fn>
class RemoteFunction;

//
// Calling convention specialization
//
template<typename R, typename... Args> \
class RemoteFunction <R( __cdecl* )(Args...)> : public RemoteFunctionBase<cc_cdecl, R, Args...>
{
public:
    using RemoteFunctionBase<cc_cdecl, R, Args...>::RemoteFunctionBase;

    RemoteFunction( Process& proc, R( __cdecl* ptr )(Args...), ThreadPtr boundThread = nullptr )
        : RemoteFunctionBase<cc_cdecl, R, Args...>( proc, reinterpret_cast<ptr_t>(ptr), boundThread )
    { 
    }
};

// Under AMD64 these will be same declarations as __cdecl, so compilation will fail.
#ifdef USE32
template<typename R, typename... Args>
class RemoteFunction <R( __stdcall* )(Args...)> : public RemoteFunctionBase<cc_stdcall, R, Args...>
{
public:
    using RemoteFunctionBase<cc_stdcall, R, Args...>::RemoteFunctionBase;

    RemoteFunction( Process& proc, R( __stdcall* ptr )(Args...), ThreadPtr boundThread = nullptr )
        : RemoteFunctionBase<cc_stdcall, R, Args...>( proc, reinterpret_cast<ptr_t>(ptr), boundThread )
    { 
    }
};

template<typename R, typename... Args>
class RemoteFunction <R( __thiscall* )(Args...)> : public RemoteFunctionBase<cc_thiscall, R, Args...>
{
public:
    using RemoteFunctionBase<cc_thiscall, R, Args...>::RemoteFunctionBase;

    RemoteFunction( Process& proc, R( __thiscall* ptr )(Args...), ThreadPtr boundThread = nullptr )
        : RemoteFunctionBase<cc_thiscall, R, Args...>( proc, reinterpret_cast<ptr_t>(ptr), boundThread )
    { 
    }
};

template<typename R, typename... Args>
class RemoteFunction <R( __fastcall* )(Args...)> : public RemoteFunctionBase<cc_fastcall, R, Args...>
{
public:
    using RemoteFunctionBase<cc_fastcall, R, Args...>::RemoteFunctionBase;

    RemoteFunction( Process& proc, R( __fastcall* ptr )(Args...), ThreadPtr boundThread = nullptr )
        : RemoteFunctionBase<cc_fastcall, R, Args...>( proc, reinterpret_cast<ptr_t>(ptr), boundThread )
    { 
    }
};
#endif

/// <summary>
/// Get remote function object
/// </summary>
/// <param name="ptr">Function address in the remote process</param>
/// <returns>Function object</returns>
template<typename T>
RemoteFunction<T> MakeRemoteFunction( Process& process, ptr_t ptr, ThreadPtr boundThread = nullptr )
{
    return RemoteFunction<T>( process, ptr, boundThread );
}

/// <summary>
/// Get remote function object
/// </summary>
/// <param name="ptr">Function address in the remote process</param>
/// <returns>Function object</returns>
template<typename T>
RemoteFunction<T> MakeRemoteFunction( Process& process, T ptr, ThreadPtr boundThread = nullptr )
{
    return RemoteFunction<T>( process, ptr, boundThread );
}

/// <summary>
/// Get remote function object
/// </summary>
/// <param name="modName">Remote module name</param>
/// <param name="name_ord">Function name or ordinal</param>
/// <returns>Function object</returns>
template<typename T>
RemoteFunction<T> MakeRemoteFunction( Process& process, const std::wstring& modName, const char* name_ord, ThreadPtr boundThread = nullptr )
{
    auto ptr = process.modules().GetExport( modName, name_ord );
    return RemoteFunction<T>( process, ptr ? ptr->procAddress : 0, boundThread );
}

}
