#pragma once

#include "HookHandlers.h"
#include "../Process/Process.h"

namespace blackbone
{

template<typename Fn, class C = NoClass>
class Detour: public HookHandler<Fn, C>
{
public:
    using type    = typename HookHandler<Fn, C>::type;
    using hktype  = typename HookHandler<Fn, C>::hktype;
    using hktypeC = typename HookHandler<Fn, C>::hktypeC;

public:  
    Detour()
    {
        this->_internalHandler = &HookHandler<Fn, C>::Handler;
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
    bool Hook(
        type ptr,
        hktype hkPtr,
        HookType::e type,
        CallOrder::e order = CallOrder::HookFirst,
        ReturnMethod::e retType = ReturnMethod::UseOriginal
        )
    { 
        if (this->_hooked)
            return false;

        this->_type  = type;
        this->_order = order;
        this->_retType = retType;
        this->_callOriginal = this->_original = ptr;
        this->_callback = hkPtr;

        if (!AllocateBuffer( reinterpret_cast<uint8_t*>(ptr) ))
            return false;

        switch (this->_type)
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
    bool Hook(
        type Ptr,
        hktypeC hkPtr,
        C* pClass,
        HookType::e type,
        CallOrder::e order = CallOrder::HookFirst,
        ReturnMethod::e retType = ReturnMethod::UseOriginal
        )
    {
        this->_callbackClass = pClass;
        return Hook( Ptr, brutal_cast<hktype>(hkPtr), type, order, retType );
    }


    /// <summary>
    /// Restore hooked function
    /// </summary>
    /// <returns>true on success, false if not hooked</returns>
    bool Restore()
    {
        if (!this->_hooked)
            return false;
        
        switch (this->_type)
        {
            case HookType::Inline:
            case HookType::InternalInline:
			case HookType::Int3:
			{
				DWORD flOld = 0;
				if (!VirtualProtect(this->_original, this->_origSize, PAGE_EXECUTE_READWRITE, &flOld))
					return false;
				memcpy(this->_original, this->_origCode, this->_origSize);
				VirtualProtect(this->_original, this->_origSize, flOld, &flOld);
			}
			break;


            case HookType::HWBP:
                {
                    Process thisProc;
                    thisProc.Attach( GetCurrentProcessId() );

                    for (auto& thd : thisProc.threads().getAll())
                        thd->RemoveHWBP( reinterpret_cast<ptr_t>(this->_original) );

                    this->_hwbpIdx.clear();
                }
                break;

            default:
                break;
        }

        this->_hooked = false;
        return true;
    }

private:

    /// <summary>
    /// Perform inline hook
    /// </summary>
    /// <returns>true on success</returns>
    bool HookInline()
    {
        auto jmpToHook  = AsmFactory::GetAssembler();
        auto jmpToThunk = AsmFactory::GetAssembler();

        //
        // Construct jump to thunk
        //
#ifdef USE64
        (*jmpToThunk)->mov( asmjit::host::rax, (uint64_t)this->_buf );
        (*jmpToThunk)->jmp( asmjit::host::rax );

        this->_origSize = (*jmpToThunk)->getCodeSize();
#else
        (*jmpToThunk)->jmp( (asmjit::Ptr)this->_buf );
        this->_origSize = (*jmpToThunk)->getCodeSize();
#endif
        
        DetourBase::CopyOldCode( (uint8_t*)this->_original );

        // Construct jump to hook handler
#ifdef USE64
        // mov gs:[0x28], this
        (*jmpToHook)->mov( asmjit::host::rax, (uint64_t)this );
        (*jmpToHook)->mov( asmjit::host::qword_ptr_abs( 0x28 ).setSegment( asmjit::host::gs ), asmjit::host::rax );
#else
        // mov fs:[0x14], this
        (*jmpToHook)->mov( asmjit::host::dword_ptr_abs( 0x14 ).setSegment( asmjit::host::fs ) , (uint32_t)this );
#endif // USE64

        (*jmpToHook)->jmp( (asmjit::Ptr)&HookHandler<Fn, C>::Handler );
        (*jmpToHook)->relocCode( this->_buf );

        (*jmpToThunk)->setBaseAddress( (uintptr_t)this->_original );
        auto codeSize = (*jmpToThunk)->relocCode( this->_newCode );

        DWORD flOld = 0;
        if (!VirtualProtect( this->_original, codeSize, PAGE_EXECUTE_READWRITE, &flOld ))
            return false;

        memcpy( this->_original, this->_newCode, codeSize );

        VirtualProtect( this->_original, codeSize, flOld, &flOld );

        this->_hooked = (codeSize != 0);
        return this->_hooked;
    }

    /// <summary>
    /// Perform int3 hook
    /// </summary>
    /// <returns>true on success</returns>
    bool HookInt3()
    {
        this->_newCode[0] = 0xCC;
        this->_origSize = sizeof( this->_newCode[0] );

        // Setup handler
        if (this->_vecHandler == nullptr)
            this->_vecHandler = AddVectoredExceptionHandler( 1, &DetourBase::VectoredHandler );

        if (!this->_vecHandler)
            return false;

        this->_breakpoints.insert( std::make_pair( this->_original, (DetourBase*)this ) );

        // Save original code
        memcpy( this->_origCode, this->_original, this->_origSize );

        // Write break instruction
        DWORD flOld = 0;
        if (!VirtualProtect(this->_original, this->_origSize, PAGE_EXECUTE_READWRITE, &flOld))
			return false;
        memcpy( this->_original, this->_newCode, this->_origSize );
        VirtualProtect( this->_original, this->_origSize, flOld, &flOld );

        return this->_hooked = TRUE;
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
        if (this->_vecHandler == nullptr)
            this->_vecHandler = AddVectoredExceptionHandler( 1, &DetourBase::VectoredHandler );

        if (!this->_vecHandler)
            return false;

        this->_breakpoints.insert( std::make_pair( this->_original, (DetourBase*)this ) );

        // Add breakpoint to every thread
        for (auto& thd : thisProc.threads().getAll())
            this->_hwbpIdx[thd->id()] = thd->AddHWBP( reinterpret_cast<ptr_t>(this->_original), hwbp_execute, hwbp_1 ).result();
    
        return this->_hooked = true;
    }
};

}
