#pragma once

#include "../Include/Winheaders.h"
#include "../Include/NativeStructures.h"
#include "../Include/Macro.h"
#include "../Include/Types.h"

#include <vector>
#include <type_traits>

namespace blackbone
{

/// <summary>
/// WOW64-x64 interface
/// </summary>
class Wow64Local
{
public:
    Wow64Local( void );
    ~Wow64Local( void );


// unreferenced formal parameter, invalid operand size
#pragma warning(disable : 4409 4100)    

    /// <summary>
    /// Call 64 bit function
    /// </summary>
    /// <param name="func">Function pointer</param>
    /// <param name="args">Function argumetns</param>
    /// <returns>Function return value</returns>
    template< typename... Args >
    DWORD64 X64Call( ptr_t func, Args... args )
    {
#ifdef _M_IX86
        int argC = sizeof...(Args);
        int i = 0;
        std::vector<DWORD64> vargs{ ((DWORD64)args)... };

        DWORD64 _rcx = (i < argC) ? vargs[i++] : 0;
        DWORD64 _rdx = (i < argC) ? vargs[i++] : 0;
        DWORD64 _r8 = (i < argC) ? vargs[i++] : 0;
        DWORD64 _r9 = (i < argC) ? vargs[i++] : 0;
        reg64 _rax;
        _rax.v = 0;

        DWORD64 restArgs = (i < argC) ? (DWORD64)&vargs[i] : 0;
	
	    // conversion to QWORD for easier use in inline assembly
	    DWORD64 _argC = argC - i;
	    DWORD64 _func = func;

	    DWORD back_esp = 0;

	    __asm
	    {
            mov    back_esp, esp
		
            ;// align esp to 16
            and    esp, 0xFFFFFFF0

            X64_Start();

            ;// fill first four arguments
            push   _rcx
            X64_Pop(_RCX); 
            push   _rdx
            X64_Pop(_RDX);
            push   _r8
            X64_Pop(_R8);
            push   _r9
            X64_Pop(_R9);
	
            push   edi

            ;// Preserve R10 for win10 WOW64 call
            X64_Push( _R10 )

            push   restArgs
            X64_Pop(_RDI);

            push   _argC
            X64_Pop(_RAX);

            ;// put rest of arguments on the stack
            test   eax, eax
            jz     _ls_e
            lea    edi, dword ptr[edi + 8 * eax - 8]

            _ls:
            test   eax, eax
            jz     _ls_e
            push   dword ptr [edi]
            sub    edi, 8
            sub    eax, 1
            jmp    _ls
            _ls_e:

            ;// create stack space for spilling registers
            sub    esp, 0x20

            call   _func

            ;// cleanup stack
            push   _argC
            X64_Pop(_RCX);
            lea    esp, dword ptr[esp + 8 * ecx + 0x20]

            X64_Pop( _R10 )
            pop    edi

            ;// set return value
            X64_Push(_RAX);
            pop    _rax.dw[0]

            X64_End();

			mov    ax, ds
			mov    ss, ax
			
            mov    esp, back_esp
	    }
        
        return _rax.v;
#else
        return (DWORD64)STATUS_NOT_SUPPORTED;
#endif	
    }

    /// <summary>
    /// Perform a syscall
    /// </summary>
    /// <param name="idx">Syscall index</param>
    /// <param name="args">Syscall arguments</param>
    /// <returns>Syscall result</returns>
    template<typename ...Args>
    DWORD64 X64Syscall( int idx, Args... args )
    {
    #ifdef _M_IX86
        int argC = sizeof...(Args);
        int i = 0;
        std::vector<DWORD64> vargs{ ((DWORD64)args)... };

        DWORD64 _rcx = (i < argC) ? vargs[i++] : 0;
        DWORD64 _rdx = (i < argC) ? vargs[i++] : 0;
        DWORD64 _r8 = (i < argC) ? vargs[i++] : 0;
        DWORD64 _r9 = (i < argC) ? vargs[i++] : 0;
        reg64 _rax;
        _rax.v = 0;

        DWORD64 restArgs = (i < argC) ? (DWORD64)&vargs[i] : 0;
	
	    //conversion to QWORD for easier use in inline assembly
	    DWORD64 _argC = argC - i;
	    DWORD back_esp = 0;

	    __asm
	    {
            ;//keep original esp in back_esp variable
            mov    back_esp, esp
		
            ;//align esp to 8, without aligned stack some syscalls may return errors !
            and    esp, 0xFFFFFFF8

            X64_Start();

            ;//fill first four arguments
            push   _rcx
            X64_Pop(_RCX);
            push   _rdx
            X64_Pop(_RDX);
            push   _r8
            X64_Pop(_R8);
            push   _r9
            X64_Pop(_R9);
	
            push   edi
            X64_Push( _R10 )

            push   restArgs
            X64_Pop(_RDI);

            push   _argC
            X64_Pop(_RAX);

            ;//put rest of arguments on the stack
            test   eax, eax
            jz     _ls_e
            lea    edi, dword ptr [edi + 8*eax - 8]

            _ls:
            test   eax, eax
            jz     _ls_e
            push   dword ptr [edi]
            sub    edi, 8
            sub    eax, 1
            jmp    _ls
            _ls_e:

            ;//create stack space for spilling registers
            sub    esp, 0x28    

            mov eax, idx
            push _rcx
            X64_Pop( _R10 );
            EMIT( 0x0F ) EMIT( 0x05 );  // syscall

            ;//cleanup stack
            push   _argC
            X64_Pop(_RCX);
            lea    esp, dword ptr [esp + 8*ecx + 0x20]

            X64_Pop( _R10 )
            pop    edi

            ;//set return value
            X64_Push(_RAX);
            pop    _rax.dw[0]

            X64_End();

			mov    ax, ds
			mov    ss, ax

            mov    esp, back_esp
	    }

	    return _rax.v;
    #else
        return STATUS_NOT_SUPPORTED;
    #endif
    }
#pragma warning(default : 4409 4100)

    /// <summary>
    /// Copy memory beyond 4GB limit
    /// </summary>
    /// <param name="dst">Destination address</param>
    /// <param name="src">Source address</param>
    /// <param name="size">Region size</param>
    BLACKBONE_API void memcpy64( DWORD64 /*dst*/, DWORD64 /*src*/, DWORD /*size*/ );

    /// <summary>
    /// Get native bit TEB
    /// </summary>
    /// <param name="out">TEB structure</param>
    /// <returns>TEB pointer</returns>
    BLACKBONE_API DWORD64 getTEB64( _TEB64& out );

    /// <summary>
    /// Gets 64 bit module base
    /// </summary>
    /// <param name="lpModuleName">Module name</param>
    /// <param name="pSize">Found module size</param>
    /// <returns>Module base address, 0 if not found</returns>
    BLACKBONE_API DWORD64 GetModuleHandle64( const wchar_t* lpModuleName, DWORD* pSize = nullptr );

    /// <summary>
    /// Get 64 bit ntdll base
    /// </summary>
    /// <param name="pSize">Image size</param>
    /// <returns>ntdll address</returns>
    BLACKBONE_API DWORD64 getNTDLL64( DWORD* pSize = nullptr );

    /// <summary>
    /// Get 'LdrGetProcedureAddress' address
    /// </summary>
    /// <returns>'LdrGetProcedureAddress' address, 0 if not found</returns>
    BLACKBONE_API DWORD64 getLdrGetProcedureAddress();

    /// <summary>
    /// 64 bit implementation of GetProcAddress
    /// </summary>
    /// <param name="hModule">Module base</param>
    /// <param name="funcName">Function name or ordinal</param>
    /// <returns>Procedure address, 0 if not found</returns>
    BLACKBONE_API DWORD64 GetProcAddress64( DWORD64 hModule, const char* funcName );

    /// <summary>
    /// Load 64 bit module into current process
    /// </summary>
    /// <param name="path">Module path</param>
    /// <returns>Module base address</returns>
    BLACKBONE_API DWORD64 LoadLibrary64( const wchar_t* path );

private:
    DWORD64 _ntdll64 = 0;                   // 64bit ntdll address
    DWORD64 _LdrGetProcedureAddress = 0;    // LdrGetProcedureAddress address in 64bit ntdll
    DWORD   _ntdll64Size = 0;               // size of ntdll64 image
};

}