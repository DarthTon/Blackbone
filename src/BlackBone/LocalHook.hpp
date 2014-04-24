#pragma once

#include "HookHandlers.h"
#include "Process.h"

namespace blackbone
{

template<typename Fn, class C = NoClass>
class Detour : public HookHandler<Fn, C>
{
public:
    typedef typename HookHandler<Fn, C>::type type;
    typedef typename HookHandler<Fn, C>::hktype hktype;
    typedef typename HookHandler<Fn, C>::hktypeC hktypeC;

public:  
    Detour()
    {
        _internalHandler = &HookHandler<Fn, C>::Handler;
    }

    ~Detour()
    {
        Restore();
    }

    /// <summary>
    /// Hook function
    /// </summary>
    /// <param name="ptr">Target function address</param>
    /// <param name="hkPtr">Hook function address</param>
    /// <param name="type">Hooking method</param>
    /// <param name="order">Call order. Hook before original or vice versa</param>
    /// <param name="retType">Return value. Use origianl or value from hook</param>
    /// <returns>true on success</returns>
    bool Hook( type ptr, hktype hkPtr, HookType::e type,
               CallOrder::e order = CallOrder::HookFirst,
               ReturnMethod::e retType = ReturnMethod::UseOriginal )
    { 
        if (_hooked)
            return false;

        _type  = type;
        _order = order;
        _retType = retType;
        _callOriginal = _original = ptr;
        _callback = hkPtr;

        switch (_type)
        {
            case HookType::Inline:
                return HookInline();

            case HookType::Int3:
                return HookInt3();

            case HookType::HWBP:
                return HookHWBP();

            default:
                return false;
        }
    }

    /// <summary>
    /// Hook function
    /// </summary>
    /// <param name="Ptr">Target function address</param>
    /// <param name="hkPtr">Hook class member address</param>
    /// <param name="pClass">Hook class address</param>
    /// <param name="type">Hooking method</param>
    /// <param name="order">Call order. Hook before original or vice versa</param>
    /// <param name="retType">Return value. Use origianl or value from hook</param>
    /// <returns>true on success</returns>
    bool Hook( type Ptr, hktypeC hkPtr, C* pClass, HookType::e type,
               CallOrder::e order = CallOrder::HookFirst,
               ReturnMethod::e retType = ReturnMethod::UseOriginal )
    {
        _callbackClass = pClass;
        return Hook( Ptr, brutal_cast<hktype>(hkPtr), type, order, retType );
    }


    /// <summary>
    /// Restore hooked function
    /// </summary>
    /// <returns>true on success, false if not hooked</returns>
    bool Restore()
    {
        if (!_hooked)
            return false;
        
        switch (_type)
        {
            case HookType::Inline:
            case HookType::InternalInline:
            case HookType::Int3:
                WriteProcessMemory( GetCurrentProcess(), _original, _origCode, _origSize, NULL );
                break;

            case HookType::HWBP:
                {
                    Process thisProc;
                    thisProc.Attach( GetCurrentProcessId() );

                    for (auto& thd : thisProc.threads().getAll())
                        thd.RemoveHWBP( reinterpret_cast<ptr_t>(_original) );

                    _hwbpIdx.clear();
                }
                break;

            default:
                break;
        }

        _hooked = false;
        return true;
    }

private:

    /// <summary>
    /// Perform inline hook
    /// </summary>
    /// <returns>true on success</returns>
    bool HookInline()
    {
        AsmJit::Assembler jmpToHook, jmpToThunk; 

        //
        // Construct jump to thunk
        //
#ifdef _M_AMD64
        jmpToThunk.mov( AsmJit::rax, (uint64_t)_buf );
        jmpToThunk.jmp( AsmJit::rax );

        _origSize = jmpToThunk.getCodeSize( );
#else
        jmpToThunk.jmp( _buf );
        _origSize = jmpToThunk.getCodeSize( );
#endif
        
        DetourBase::CopyOldCode( (uint8_t*)_original );

        // Construct jump to hook handler
#ifdef _M_AMD64
        // mov gs:[0x28], this
        jmpToHook.mov( AsmJit::rax, (uint64_t)this );
        jmpToHook.mov( AsmJit::qword_ptr_abs( (void*)0x28, 0, AsmJit::SEGMENT_GS ), AsmJit::rax );
#else
        // mov fs:[0x14], this
        jmpToHook.mov( AsmJit::dword_ptr_abs( (void*)0x14, 0, AsmJit::SEGMENT_FS ), (uint32_t)this );
#endif // _M_AMD64

        jmpToHook.jmp( &HookHandler<Fn, C>::Handler );
        jmpToHook.relocCode( _buf );

        BOOL res = WriteProcessMemory( GetCurrentProcess(), _original, _newCode, jmpToThunk.relocCode( _newCode, (sysuint_t)_original ), NULL );
        
        return (_hooked = (res == TRUE));
    }

    /// <summary>
    /// Perform int3 hook
    /// </summary>
    /// <returns>true on success</returns>
    bool HookInt3()
    {
        _newCode[0] = 0xCC;
        _origSize = sizeof(_newCode[0]);

        // Setup handler
        if (_vecHandler == nullptr)
            _vecHandler = AddVectoredExceptionHandler( 1, &DetourBase::VectoredHandler );

        if (!_vecHandler)
            return false;

        _breakpoints.insert( std::make_pair( _original, (DetourBase*)this ) );

        // Save original code
        memcpy( _origCode, _original, _origSize );

        // Write break instruction
        BOOL res = WriteProcessMemory( GetCurrentProcess(), _original, _newCode, _origSize, NULL );

        return (_hooked = (res == TRUE));
    }

    /// <summary>
    /// Perform hardware breakpoint hook
    /// </summary>
    /// <returns>true on success</returns>
    bool HookHWBP()
    {
        Process thisProc;
        thisProc.Attach( GetCurrentProcessId() );

        // Setup handler
        if (_vecHandler == nullptr)
            _vecHandler = AddVectoredExceptionHandler( 1, &DetourBase::VectoredHandler );

        if (!_vecHandler)
            return false;

        _breakpoints.insert( std::make_pair( _original, (DetourBase*)this ) );

        // Add breakpoint to every thread
        for (auto& thd : thisProc.threads().getAll())
            _hwbpIdx[thd.id()] = thd.AddHWBP( reinterpret_cast<ptr_t>(_original), hwbp_execute, hwbp_1 );
    
        return _hooked = true;
    }
};

}