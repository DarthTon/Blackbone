#pragma once

#include "LocalHook.hpp"
#include "../Misc/DynImport.h"

namespace blackbone
{

template<typename Fn, class C = NoClass>
class VTableDetour : public Detour<Fn, C>
{
public:
    using type    = typename HookHandler<Fn, C>::type;
    using hktype  = typename HookHandler<Fn, C>::hktype;
    using hktypeC = typename HookHandler<Fn, C>::hktypeC;

public:
    VTableDetour()
    {
        DetourBase::AllocateBuffer( nullptr );
    }

    ~VTableDetour()
    {
        Restore();
    }

    /// <summary>
    /// Hook function in vtable
    /// </summary>
    /// <param name="ppVtable">Pointer to vtable pointer</param>
    /// <param name="index">Function index</param>
    /// <param name="hkPtr">Hook function address</param>
    /// <param name="order">Call order. Hook before original or vice versa</param>
    /// <param name="retType">Return value. Use origianl or value from hook</param>
    /// <param name="copyVtable">if true, vtable will be copied and edited, otherwise existing vtable will be edited</param>
    /// <param name="vtableLen">Optional. Valid only when copyVtable is true. Number of function in vtable. 
    /// Used to determine number of function to copy</param>
    /// <returns>true on success</returns>
    bool Hook( 
        void** ppVtable, 
        int index, 
        hktype hkPtr, 
        CallOrder::e order = CallOrder::HookFirst,
        ReturnMethod::e retType = ReturnMethod::UseOriginal,
        bool copyVtable = false, 
        int vtableLen = 0 
        )
    {
        auto jmpToHook = AsmFactory::GetAssembler();

        this->_type = HookType::VTable;
        this->_order = order;
        this->_retType = retType;
        this->_callOriginal = this->_original = (*(void***)ppVtable)[index];
        this->_callback = hkPtr;
        this->_internalHandler = &HookHandler<Fn, C>::Handler;
        this->_ppVtable = ppVtable;
        this->_pVtable = *ppVtable;
        this->_vtIndex = index;
        this->_vtCopied = copyVtable;

        // Construct jump to hook handler
#ifdef USE64
        // mov gs:[0x28], this
        (*jmpToHook)->mov( asmjit::host::rax, (uint64_t)this );
        (*jmpToHook)->mov( asmjit::host::qword_ptr_abs( 0x28 ).setSegment( asmjit::host::gs ), asmjit::host::rax );
#else
        // mov fs:[0x14], this
        (*jmpToHook)->mov( asmjit::host::dword_ptr_abs( 0x14 ).setSegment( asmjit::host::fs ), (uint32_t)this );
#endif // USE64

        (*jmpToHook)->jmp( (asmjit::Ptr)this->_internalHandler );
        (*jmpToHook)->relocCode( this->_buf );

        // Modify VTable copy
        if (copyVtable)
        {
            // Copy VTable
            if (vtableLen != 0)
            {
                memcpy( this->_buf + 0x300 - sizeof( void* ), (*(void***)ppVtable) - 1, vtableLen * sizeof( void* ) );
            }
            else 
            {
                Process proc;
                proc.Attach( GetCurrentProcessId() );
                auto vptr = (*(uintptr_t**)ppVtable)[index];
                auto mod = proc.modules().GetModule( vptr, false );
                uintptr_t imageBase = static_cast<uintptr_t>(mod->baseAddress);
                uintptr_t imageSzie = mod->size;

                for (;; vtableLen++)
                {
                    vptr = (*(uintptr_t**)ppVtable)[vtableLen];
                    if (vptr < imageBase || vptr >= imageBase + imageSzie)
                    {
                        memcpy( this->_buf + 0x300 - sizeof( void* ), (*(void***)ppVtable) - 1, vtableLen * sizeof( void* ) );
                        break;
                    }
                }
            }

            // Replace pointer to VTable
            ((void**)this->_buf + 0x300 / sizeof( uintptr_t ))[index] = this->_buf;
            *ppVtable = this->_buf + 0x300;
        }
        // Modify pointer in-place
        else
        {
            DWORD flOld = 0;

            VirtualProtect( *(uintptr_t**)ppVtable + index, sizeof(void*), PAGE_EXECUTE_READWRITE, &flOld );
            (*(void***)ppVtable)[index] = this->_buf;
            VirtualProtect( *(uintptr_t**)ppVtable + index, sizeof(void*), flOld, &flOld );
        }

        return (this->_hooked = true);
    }

    /// <summary>
    /// Hooks function in vtable
    /// </summary>
    /// <param name="ppVtable">Pointer to vtable pointer</param>
    /// <param name="index">Function index</param>
    /// <param name="hkPtr">Hook class member address</param>
    /// <param name="pClass">Hook class address</param>
    /// <param name="order">Call order. Hook before original or vice versa</param>
    /// <param name="retType">Return value. Use origianl or value from hook</param>
    /// <param name="copyVtable">if true, vtable will be copied and edited, otherwise existing vtable will be edited</param>
    /// <param name="vtableLen">Optional. Valid only when copyVtable is true. Number of function in vtable. 
    /// Used to determine number of function to copy</param>
    /// <returns>true on success</returns>
    bool Hook( 
        void** ppVtable, 
        int index, 
        hktypeC hkPtr, 
        C* pClass,
        CallOrder::e order = CallOrder::HookFirst,
        ReturnMethod::e retType = ReturnMethod::UseOriginal,
        bool copyVtable = false, 
        int vtableLen = 0 
        )
    {
        this->_callbackClass = pClass;
        return Hook( ppVtable, index, brutal_cast<hktype>(hkPtr), order, retType, copyVtable, vtableLen );
    }

    /// <summary>
    /// Restore hooked function
    /// </summary>
    /// <returns>true on success, false if not hooked</returns>
    bool Restore()
    {
        if (!this->_hooked)
            return false;

        if (this->_vtCopied)
        {
            *this->_ppVtable = this->_pVtable;
        }
        else
        {
            DWORD flOld = 0;

            VirtualProtect( *(uintptr_t**)this->_ppVtable + this->_vtIndex, sizeof( void* ), PAGE_EXECUTE_READWRITE, &flOld );
            (*(void***)this->_ppVtable)[this->_vtIndex] = this->_original;
            VirtualProtect( *(uintptr_t**)this->_ppVtable + this->_vtIndex, sizeof( void* ), flOld, &flOld );
        }

        this->_hooked = false;
        return true;
    }

private:
    bool   _vtCopied = false;           // VTable was copied
    void** _ppVtable = nullptr;         // Pointer to VTable pointer
    void*  _pVtable = nullptr;          // Pointer to VTable
    int    _vtIndex = 0;                // VTable function index
};

}