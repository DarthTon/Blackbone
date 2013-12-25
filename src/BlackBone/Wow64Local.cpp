#include "Wow64Local.h"

#include <memory>

namespace blackbone
{

Wow64Local::Wow64Local(void)
{
}

Wow64Local::~Wow64Local(void)
{
}


#ifdef _M_IX86
/// <summary>
/// Copy memory beyond 4GB limit
/// </summary>
/// <param name="dst">Destination address</param>
/// <param name="src">Source address</param>
/// <param name="size">Region size</param>
void __declspec(naked, noinline) Wow64Local::memcpy64( DWORD64 /*dst*/, DWORD64 /*src*/, DWORD /*size*/ )
{
    __asm
    {
        push ebp
        mov ebp, esp
        pushad

        X64_Start();

        /*
            mov rdi, QWORD PTR [rbp + 0x8]
            mov rsi, QWORD PTR [rbp + 0x10]
            mov ecx, DWORD PTR [rbp + 0x18]

          loop1:
            mov al, BYTE PTR [rsi]
            mov BYTE PTR [rdi], al
            add rsi, 0x1
            add rdi, 0x1
            sub ecx, 0x1
            test ecx, ecx
          jnz loop1
        */
        EMIT( 0x48 ) EMIT( 0x8B ) EMIT( 0x7D ) EMIT( 0x08 )
        EMIT( 0x48 ) EMIT( 0x8B ) EMIT( 0x75 ) EMIT( 0x10 )
        EMIT( 0x8B ) EMIT( 0x4D ) EMIT( 0x18 )
        EMIT( 0x8A ) EMIT( 0x06 )
        EMIT( 0x88 ) EMIT( 0x07 )
        EMIT( 0x48 ) EMIT( 0x83 ) EMIT( 0xC6 ) EMIT( 0x01 )
        EMIT( 0x48 ) EMIT( 0x83 ) EMIT( 0xC7 ) EMIT( 0x01 )
        EMIT( 0x83 ) EMIT( 0xE9 ) EMIT( 0x01 )
        EMIT( 0x85 ) EMIT( 0xC9 )
        EMIT( 0x75 ) EMIT( 0xED )

        X64_End();

        popad
        mov esp, ebp
        pop ebp
        retn 20
    }
}
#else
// Stub
void __declspec(noinline) Wow64Local::memcpy64( DWORD64 /*dst*/, DWORD64 /*src*/, DWORD /*size*/ )
{
}

#endif

/// <summary>
/// Get native bit TEB
/// </summary>
/// <param name="out">TEB structure</param>
/// <returns>TEB pointer</returns>
DWORD64 Wow64Local::getTEB64( _TEB64& out )
{
    UNREFERENCED_PARAMETER( out );

    reg64 reg;
    reg.v = 0;

#ifdef _M_IX86

    _asm
    {
        X64_Start();

        //R12 register should always contain pointer to TEB64 in WoW64 processes
        X64_Push(_R12);

        //below pop will pop QWORD from stack, as we're in x64 mode now
        __asm pop reg.dw[0]

        X64_End();
    }

    memcpy64( (DWORD64)&out, reg.v, sizeof(out) );
#endif

    return reg.v;
}

/// <summary>
/// Gets 64 bit module base
/// </summary>
/// <param name="lpModuleName">Module name</param>
/// <param name="pSize">Found module size</param>
/// <returns>Module base address, 0 if not found</returns>
DWORD64 Wow64Local::GetModuleHandle64( const wchar_t* lpModuleName, DWORD* pSize /*= nullptr*/ )
{
    DWORD64 module = 0;
    _TEB64 teb64 = { 0 };
    _PEB64 peb64 = { 0 };
    _PEB_LDR_DATA264 ldr = { 0 };

    getTEB64( teb64 ); 

    memcpy64( (DWORD64)&peb64, teb64.ProcessEnvironmentBlock, sizeof(peb64) );
    memcpy64( (DWORD64)&ldr, peb64.Ldr, sizeof(ldr) );

    // Traverse 64bit modules
    for (DWORD64 head = ldr.InLoadOrderModuleList.Flink;
          head != (peb64.Ldr + FIELD_OFFSET( _PEB_LDR_DATA264, InLoadOrderModuleList ));
          memcpy64( (DWORD64)&head, (DWORD64)head, sizeof(head) ))
    {
        wchar_t localbuf[512] = { 0 };
        _LDR_DATA_TABLE_ENTRY_BASE64 localdata = { 0 };

        memcpy64( (DWORD64)&localdata, head, sizeof(localdata) );
        memcpy64( (DWORD64)localbuf, localdata.BaseDllName.Buffer, localdata.BaseDllName.Length );

        if (_wcsicmp( localbuf, lpModuleName ) == 0)
        {
            module = localdata.DllBase;
            if (pSize)
                *pSize = localdata.SizeOfImage;

            break;
        }
    }

    return module;
}

/// <summary>
/// Get 64 bit ntdll base
/// </summary>
/// <param name="pSize">Image size</param>
/// <returns>ntdll address</returns>
DWORD64 Wow64Local::getNTDLL64( DWORD* pSize /*= nullptr*/ )
{
    if(_ntdll64 != 0)
    {
        if (pSize)
            *pSize = _ntdll64Size;

        return _ntdll64;
    }

    _ntdll64 = GetModuleHandle64( L"ntdll.dll", &_ntdll64Size );
    if (pSize)
        *pSize = _ntdll64Size;

    return _ntdll64;
}

/// <summary>
/// Get 'LdrGetProcedureAddress' address
/// </summary>
/// <returns>'LdrGetProcedureAddress' address, 0 if not found</returns>
DWORD64 Wow64Local::getLdrGetProcedureAddress()
{
    DWORD ntSize = 0;
    DWORD64 modBase = getNTDLL64( &ntSize );

    // Sanity check
    if (modBase == 0 || ntSize == 0)
        return 0;

    std::unique_ptr<uint8_t[]> buf( new uint8_t[ntSize]() );
  
    memcpy64( (DWORD64)buf.get(), modBase, ntSize );

    IMAGE_NT_HEADERS64* inh = (IMAGE_NT_HEADERS64*)(buf.get() + ((IMAGE_DOS_HEADER*)buf.get())->e_lfanew);
    IMAGE_DATA_DIRECTORY& idd = inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (0 == idd.VirtualAddress)
        return 0;

    IMAGE_EXPORT_DIRECTORY* ied = (IMAGE_EXPORT_DIRECTORY*)(buf.get() + idd.VirtualAddress);

    DWORD* rvaTable  = (DWORD*)(buf.get() + ied->AddressOfFunctions);
    WORD* ordTable   = (WORD*) (buf.get() + ied->AddressOfNameOrdinals);
    DWORD* nameTable = (DWORD*)(buf.get() + ied->AddressOfNames);

    // lazy search, there is no need to use binsearch for just one function
    for (DWORD i = 0; i < ied->NumberOfFunctions; i++)
    {
        if (strcmp( (char*)buf.get() + nameTable[i], "LdrGetProcedureAddress" ) != 0)
            continue;
        else
            return (DWORD64)(modBase + rvaTable[ordTable[i]]);
    }

    return 0;
}

/// <summary>
/// 64 bit implementation of GetProcAddress
/// </summary>
/// <param name="hModule">Module base</param>
/// <param name="funcName">Function name or ordinal</param>
/// <returns>Procedure address, 0 if not found</returns>
DWORD64 Wow64Local::GetProcAddress64( DWORD64 hModule, const char* funcName )
{
    if (0 == _LdrGetProcedureAddress)
    {
        _LdrGetProcedureAddress = getLdrGetProcedureAddress();
        if (0 == _LdrGetProcedureAddress)
            return 0;
    }

    _UNICODE_STRING_T<DWORD64> fName = { 0 };
    fName.Buffer = (DWORD64)funcName;
    fName.Length = (WORD)strlen( funcName );
    fName.MaximumLength = fName.Length + 1;

    DWORD64 funcRet = 0;
    X64Call( _LdrGetProcedureAddress, hModule, &fName, 0, &funcRet );

    return funcRet;
}

/// <summary>
/// Load 64 bit module into current process
/// </summary>
/// <param name="path">Module path</param>
/// <returns>Module base address</returns>
DWORD64 Wow64Local::LoadLibrary64( const wchar_t* path )
{
    _UNICODE_STRING_T<DWORD64> upath = {0};

    DWORD64 hModule     = 0;
    DWORD64 pfnLdrLoad  = (DWORD64)GetProcAddress64( getNTDLL64(), "LdrLoadDll" );
    upath.Length        = (WORD)wcslen( path ) * sizeof(wchar_t);
    upath.MaximumLength = (WORD)upath.Length;
    upath.Buffer        = (DWORD64)path;

    X64Call( pfnLdrLoad, NULL, 0, &upath, &hModule );

    return hModule;
};


}