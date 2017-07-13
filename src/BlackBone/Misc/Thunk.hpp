#include "../Config.h"
#include <stdint.h>
#include <winnt.h>

// Thunk code
#pragma pack(push, 1)
struct ThunkData
{
#ifndef USE64
    /*
        mov eax, pInstance
        mov fs:[0x14], eax
        mov eax, pMethod
        jmp eax
    */
    uint8_t  mov1  = 0xB8;
    void*    pInst = nullptr;
    uint16_t fs1   = '\x64\xA3';
    uint8_t  fs2   = FIELD_OFFSET( NT_TIB, ArbitraryUserPointer );
    uint8_t  fs3   = 0;
    uint16_t fs4   = 0;
    uint8_t  mov2  = 0xB8;
    void*    pFn   = nullptr;
    uint16_t jmp1  = '\xFF\xE0';
#else
    /*
        mov rax, pInstance
        mov gs:[0x28], rax
        mov rax, pMethod
        jmp rax
    */
    uint16_t mov1  = '\x48\xB8';
    void*    pInst = nullptr;
    uint32_t fs1   = '\x65\x48\x89\x04';
    uint8_t  fs2   = 0x25;
    uint8_t  fs3   = FIELD_OFFSET( NT_TIB, ArbitraryUserPointer );
    uint8_t  fs4   = 0;
    uint16_t fs5   = 0;
    uint16_t mov2  = '\x48\xB8';
    void*    pFn   = nullptr;
    uint16_t jmp1  = '\xFF\xE0';
#endif

    void setup( void* pInstance, void* pMethod )
    {
        pInst = pInstance;
        pFn = pMethod;
    }
};
#pragma pack(pop)


template<typename fn, typename C>
class Win32Thunk;

template<typename R, typename... Args, typename C>
class Win32Thunk < R( __stdcall* )(Args...), C >
{
public:
    using TypeMember = R( C::* )(Args...);
    using TypeFree = R( __stdcall* )(Args...);

public:
    Win32Thunk( TypeMember pfn, C* pInstance )
        : _pMethod( pfn )
        , _pInstance( pInstance )
    {
        DWORD dwOld = 0;
        VirtualProtect( &_thunk, sizeof( _thunk ), PAGE_EXECUTE_READWRITE, &dwOld );
        _thunk.setup( this, &Win32Thunk::WrapHandler );
    }

    /// <summary>
    /// Redirect call
    /// </summary>
    /// <param name="...args">Arguments</param>
    /// <returns>Call result</returns>
    static R __stdcall WrapHandler( Args... args )
    {
        auto _this = reinterpret_cast<Win32Thunk*>(((PNT_TIB)NtCurrentTeb())->ArbitraryUserPointer);
        return (_this->_pInstance->*_this->_pMethod)(args...);
    }

    /// <summary>
    /// Get thunk
    /// </summary>
    /// <returns></returns>
    TypeFree GetThunk()
    {
        return reinterpret_cast<TypeFree>(&_thunk);
    }

private:
    TypeMember _pMethod = nullptr;  // Member function to call
    C* _pInstance = nullptr;        // Bound instance
    ThunkData _thunk;               // Thunk code
};