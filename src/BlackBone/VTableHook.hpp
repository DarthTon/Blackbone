#pragma once

#include "LocalHook.hpp"

namespace blackbone
{

template<typename Fn, class C = NoClass>
class VTableDetour : public Detour<Fn, C>
{
public:
    typedef typename HookHandler<Fn, C>::type type;
    typedef typename HookHandler<Fn, C>::hktype hktype;
    typedef typename HookHandler<Fn, C>::hktypeC hktypeC;

public:
    VTableDetour()
    {
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
    /// <param name="copyVtable">if true, vtable will be copied and edited, otherwise existing vtable will be edited</param>
    /// <param name="vtableLen">Optional. Valid only when copyVtable is true. Number of function in vtable. 
    /// Used to determine number of function to copy</param>
    /// <returns>true on success</returns>
    bool Hook( void** ppVtable, uintptr_t index, hktype hkPtr, bool copyVtable = false, int vtableLen = 0 )
    {
        AsmJit::Assembler jmpToHook;

        //_order = CallOrder::HookFirst;
        //_retType = ReturnMethod::UseOriginal;

        _type = HookType::VTable;
        _callOriginal = _original = (*(void***)ppVtable)[index];
        _callback = hkPtr;
        _internalHandler = &HookHandler<Fn, C>::Handler;
        _ppVtable = ppVtable;
        _pVtable = *ppVtable;
        _vtIndex = index;
        _vtCopied = copyVtable;

        // Construct jump to hook handler
#ifdef _M_AMD64
        // mov gs:[0x28], this
        jmpToHook.mov( AsmJit::rax, (uint64_t)this );
        jmpToHook.mov( AsmJit::qword_ptr_abs( (void*)0x28, 0, AsmJit::SEGMENT_GS ), AsmJit::rax );
#else
        // mov fs:[0x14], this
        jmpToHook.mov( AsmJit::dword_ptr_abs( (void*)0x14, 0, AsmJit::SEGMENT_FS ), (uint32_t)this );
#endif // _M_AMD64

        jmpToHook.jmp( _internalHandler );
        jmpToHook.relocCode( _buf );

        // Modify VTable copy
        if (copyVtable)
        {
            uintptr_t ccpad;
            memset( &ccpad, 0xCC, sizeof(ccpad) );

            // Copy VTable
            if (vtableLen != 0)
            {
                memcpy( _buf + 0x300, *ppVtable, vtableLen * sizeof(void*) );
            }
            else for (;; vtableLen++)
            {
                if ((*(void***)ppVtable)[vtableLen] == nullptr ||
                    (*(void***)ppVtable)[vtableLen] == (void**)ccpad)
                {
                    memcpy( _buf + 0x300, *ppVtable, vtableLen * sizeof(void*) );
                    break;
                }
            }

            // Replace pointer to VTable
            ((void**)_buf + 0x300 / sizeof(uintptr_t))[index] = _buf;
            *ppVtable = _buf + 0x300;
        }
        // Modify pointer in-place
        else
        {
            DWORD flOld = 0;

            VirtualProtect( *(uintptr_t**)ppVtable + index, sizeof(void*), PAGE_EXECUTE_READWRITE, &flOld );
            (*(void***)ppVtable)[index] = _buf;
            VirtualProtect( *(uintptr_t**)ppVtable + index, sizeof(void*), flOld, &flOld );
        }

        return (_hooked = true);
    }

    /// <summary>
    /// Hooks function in vtable
    /// </summary>
    /// <param name="ppVtable">Pointer to vtable pointer</param>
    /// <param name="index">Function index</param>
    /// <param name="hkPtr">Hook class member address</param>
    /// <param name="pClass">THook class address</param>
    /// <param name="copyVtable">if true, vtable will be copied and edited, otherwise existing vtable will be edited</param>
    /// <param name="vtableLen">Optional. Valid only when copyVtable is true. Number of function in vtable. 
    /// Used to determine number of function to copy</param>
    /// <returns>true on success</returns>
    bool Hook( void** ppVtable, uintptr_t index, hktypeC hkPtr, C* pClass, bool copyVtable = false, int vtableLen = 0 )
    {
        _callbackClass = pClass;
        return Hook( ppVtable, index, brutal_cast<hktype>(hkPtr), copyVtable, vtableLen );
    }

    /// <summary>
    /// Restore hooked function
    /// </summary>
    /// <returns>true on success, false if not hooked</returns>
    bool Restore()
    {
        if (!_hooked)
            return false;

        if (_vtCopied)
        {
            *_ppVtable = _pVtable;
        }
        else
        {
            DWORD flOld = 0;

            VirtualProtect( *(uintptr_t**)_ppVtable + _vtIndex, sizeof(void*), PAGE_EXECUTE_READWRITE, &flOld );
            (*(void***)_ppVtable)[_vtIndex] = _original;
            VirtualProtect( *(uintptr_t**)_ppVtable + _vtIndex, sizeof(void*), flOld, &flOld );
        }

        _hooked = false;
        return true;
    }

private:
    bool   _vtCopied = false;           // VTable was copied
    void** _ppVtable = nullptr;         // Pointer to VTable pointer
    void*  _pVtable = nullptr;          // Pointer to VTable
    int    _vtIndex = 0;                // VTable function index
};

}