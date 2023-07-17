#pragma once

#include "../ProcessCore.h"
#include "../ProcessMemory.h"
#include "../Threads/Thread.h"
#include "../../Include/Macro.h"

namespace blackbone
{

/// <summary>
/// Remote function context during hook breakpoint.
/// </summary>
class RemoteContext
{
public:
    BLACKBONE_API RemoteContext(
        ProcessMemory* memory,
        Thread* thd,
        _CONTEXT64* ctx,
        ptr_t frame_ptr,
        BOOL x64,
        int wordSize
    )
        : _memory( memory )
        , _thd( thd )
        , _ctx( ctx )
        , _x64Target( x64 )
        , _wordSize( wordSize )
        , _frame_ptr( frame_ptr != 0 ? frame_ptr : ctx->Rsp )
    {
    }

    RemoteContext( const RemoteContext& ) = delete;
    RemoteContext& operator = ( const RemoteContext& ) = delete;

    // memory object
    BLACKBONE_API ProcessMemory* memory()
    { 
        return _memory; 
    }
    
    // Native context
    BLACKBONE_API _CONTEXT64& native()
    {
        return *_ctx;
    }

    /// <summary>
    /// Get current process thread where exception occurred
    /// </summary>
    /// <returns>Thread</returns>
    BLACKBONE_API Thread& getThread()
    {
        return *_thd;
    }

    /// <summary>
    /// Get current frame return address
    /// </summary>
    /// <returns>Return address</returns>
    BLACKBONE_API ptr_t returnAddress() const
    {
        ptr_t val = 0;
        _memory->Read( _frame_ptr, _wordSize, &val );

        return val;
    }

    /// <summary>
    /// Set return address of current frame
    /// </summary>
    /// <param name="val">New return address</param>
    BLACKBONE_API void returnAddress( ptr_t val ) const
    {
        _memory->Write( _frame_ptr, _wordSize, &val );
    }

    /// <summary>
    /// Set new integer return value. Has no effect on FPU.
    /// Has effect only if called in return callback
    /// </summary>
    /// <param name="val">New return value</param>
    BLACKBONE_API void setReturnValue( ptr_t val ) const
    {
        memcpy( &_ctx->Rax, &val, _wordSize );
    }

    /// <summary>
    /// Raise exception on function return
    /// </summary>
    /// <returns>Masked return address</returns>
    BLACKBONE_API ptr_t hookReturn()
    {
        ptr_t val = returnAddress();
        SET_BIT( val, (_wordSize * 8 - 1) );
        returnAddress( val );

        return val;
    }

    /// <summary>
    /// Remove exception on return
    /// </summary>
    /// <returns>Return address</returns>
    BLACKBONE_API ptr_t unhookReturn()
    {
        auto val = returnAddress();
        RESET_BIT( val, (_wordSize * 8 - 1) );
        returnAddress( val );

        return val;
    }

    /// <summary>
    /// Get argument value.
    /// Argument index is 0 based.
    /// For x86 function works only with stack arguments
    /// For x64 only integer arguments can be retrieved
    /// </summary>
    /// <param name="index">0-based argument index</param>
    /// <returns>Argument value</returns>
    BLACKBONE_API DWORD64 getArg( int index )
    {
        if (_x64Target)
        {
            switch (index)
            {
                case 0:
                    return _ctx->Rcx;
                case 1:
                    return _ctx->Rdx;
                case 2:
                    return _ctx->R8;
                case 3:
                    return _ctx->R9;

                default:
                    return _memory->Read<DWORD64>( _ctx->Rsp + 0x28 + (index - 4) * _wordSize );
            }
        }
        else
        {
            DWORD64 val = 0;
            _memory->Read( _ctx->Rsp + 4 + index * _wordSize, _wordSize, &val );
            return val;
        }
    }

    /// <summary>
    /// Set argument value.
    /// For x86 function works only with stack arguments.
    /// For x64 only integer arguments can be set
    /// </summary>
    /// <param name="index">0-based argument index</param>
    /// <param name="val">New argument value</param>
    BLACKBONE_API void setArg( int index, DWORD64 val )
    {
        if (_x64Target)
        {
            switch (index)
            {
                case 0:
                    _ctx->Rcx = val;
                    break;
                case 1:
                    _ctx->Rdx = val;
                    break;
                case 2:
                    _ctx->R8 = val;
                    break;
                case 3:
                    _ctx->R9 = val;
                    break;

                default:
                    _memory->Write( _ctx->Rsp + 0x28 + (index - 4) * _wordSize, val );
            }
        }
        else
        {
            _memory->Write( _ctx->Rsp + 4 + index * _wordSize, _wordSize, &val );
        }
    }


    /// <summary>
    /// Get last thread error code
    /// </summary>
    /// <returns>Last error code, -1 if function failed</returns>
    BLACKBONE_API DWORD lastError()
    {
        ptr_t pteb = 0;
        size_t offset = 0;

        if (_x64Target)
        {
            pteb = _thd->teb64();
            offset = offsetOf( &_TEB64::LastErrorValue );
        }
        else
        {
            pteb = _thd->teb32();
            offset = offsetOf( &_TEB32::LastErrorValue );
        }

        DWORD code = 0xFFFFFFFF;
        if (pteb)
            _memory->Read( pteb + offset, code );

        return code;
    }

    /// <summary>
    /// Set last thread error code
    /// </summary>
    /// <returns>Last error code, -1 if function failed</returns>
    BLACKBONE_API void lastError( DWORD newError )
    {
        ptr_t pteb = 0;
        size_t offset = 0;

        if (_x64Target)
        {
            pteb = _thd->teb64();
            offset = offsetOf( &_TEB64::LastErrorValue );
        }
        else
        {
            pteb = _thd->teb32();
            offset = offsetOf( &_TEB32::LastErrorValue );
        }

        _memory->Write( pteb + offset, newError );
    }


    /// <summary>
    /// Get arbitrary thread data
    /// </summary>
    /// <returns>Data value</returns>
    BLACKBONE_API ptr_t getUserContext()
    {
        auto pteb = _thd->teb64();
        return _memory->Read<ptr_t>( pteb + offsetOf( &_NT_TIB_T<DWORD64>::ArbitraryUserPointer ) );
    }

    /// <summary>
    /// Set arbitrary thread data
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API void setUserContext( ptr_t context )
    {
        auto pteb = _thd->teb64();
        _memory->Write( pteb + offsetOf( &_NT_TIB_T<DWORD64>::ArbitraryUserPointer ), context );
    }

private:
    ProcessMemory* _memory; // Process memory routines
    Thread* _thd;           // Current thread
    _CONTEXT64* _ctx;       // Current thread context

    BOOL  _x64Target = FALSE;   // Target process is 64 bit
    int   _wordSize = 4;        // 4 for x86, 8 for x64
    ptr_t _frame_ptr = 0;       // Top stack frame pointer
};

}
