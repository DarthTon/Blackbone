#include "MExcept.h"
#include "Process.h"
#include "Macro.h"
#include "LDasm.h"

LONG CALLBACK VectoredHandler( PEXCEPTION_POINTERS ExceptionInfo );

namespace blackbone
{

// taken from CRT include <Ehdata.h>
#define EH_MAGIC_NUMBER1        0x19930520    
#define EH_PURE_MAGIC_NUMBER1   0x01994000
#define EH_EXCEPTION_NUMBER     ('msc' | 0xE0000000)


void*  MExcept::g_pImageBase = nullptr;
size_t MExcept::g_imageSize = 0;

MExcept::MExcept( class Process& proc )
    : _proc( proc )
{
    //AddVectoredExceptionHandler( 0, &VectoredHandler );
}

MExcept::~MExcept()
{
}

/// <summary>
/// Inject VEH wrapper into process
/// Used to enable execution of SEH handlers out of image
/// </summary>
/// <param name="pTargetBase">Target image base address</param>
/// <param name="imageSize">Size of the image</param>
/// <returns>Error code</returns>
NTSTATUS MExcept::CreateVEH( size_t pTargetBase, size_t imageSize )
{
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    uint64_t result = 0;
    
    // VEH codecave
    _pVEHCode = _proc.memory().Allocate( 0x2000 );
    _pVEHCode.Release();
    if (!_pVEHCode.valid())
        return LastNtStatus();

#ifdef _M_AMD64 
    AsmJit::Assembler ea;
    AsmJit::Label lExit = ea.newLabel();

    //
    // Assembly code for VectoredHandler64
    // 0x10 - EXCEPTION_RECORD.ExceptionAddress
    // 0x20 - EXCEPTION_RECORD.ExceptionInformation[0]
    // 0x30 - EXCEPTION_RECORD.ExceptionInformation[2]
    // 0x38 - EXCEPTION_RECORD.ExceptionInformation[3]
    //
    ea.mov( AsmJit::rax, qword_ptr( AsmJit::rcx ) );
    ea.cmp( AsmJit::dword_ptr( AsmJit::rax ), EH_EXCEPTION_NUMBER );
    ea.jne( lExit );
    ea.mov( AsmJit::rdx, pTargetBase );
    ea.mov( AsmJit::r8, AsmJit::qword_ptr( AsmJit::rax, 0x30 ) );
    ea.cmp( AsmJit::r8, AsmJit::rdx );;
    ea.jl( lExit );
    ea.add( AsmJit::rdx, imageSize );
    ea.cmp( AsmJit::r8, AsmJit::rdx );;
    ea.jg( lExit );
    ea.cmp( AsmJit::qword_ptr( AsmJit::rax, 0x20 ), EH_PURE_MAGIC_NUMBER1 );;
    ea.jne( lExit );
    ea.cmp( AsmJit::qword_ptr( AsmJit::rax, 0x38 ), 0 );
    ea.jne( lExit );
    ea.mov( AsmJit::qword_ptr( AsmJit::rax, 0x20 ), EH_MAGIC_NUMBER1 );
    ea.mov( AsmJit::rcx, qword_ptr( AsmJit::rcx ) );
    ea.mov( AsmJit::rdx, pTargetBase );
    ea.mov( AsmJit::qword_ptr( AsmJit::rax, 0x38 ), AsmJit::rdx );
    ea.bind( lExit );
    ea.xor_( AsmJit::rax, AsmJit::rax );
    ea.ret();
    ea._emitByte( 0xCC );
    ea._emitByte( 0xCC );
    ea._emitByte( 0xCC );

    if (_pVEHCode.Write( 0, ea.getCodeSize(), ea.make() ) != STATUS_SUCCESS)
    {
        _pVEHCode.Free();
        return LastNtStatus();
    }
#else
    UNREFERENCED_PARAMETER( pTargetBase );
    UNREFERENCED_PARAMETER( imageSize );

    // Resolve compiler incremental table address if any
    void *pFunc = ResolveJmp( &VectoredHandler );
    size_t fnSize = static_cast<size_t>(SizeOfProc( pFunc ));
    size_t dataOfs = 0, code_ofs = 0;

    // Find and replace magic values
    for (uint8_t *pData = reinterpret_cast<uint8_t*>(pFunc);
          pData < reinterpret_cast<uint8_t*>(pFunc) + fnSize - 4; 
          pData++)
    {
        // LdrpInvertedFunctionTable
        if(*(size_t*)pData == 0xDEADDA7A)
        {
            dataOfs = pData - reinterpret_cast<uint8_t*>(pFunc);
            continue;
        }

        // DecodeSystemPointer address
        if(*(size_t*)pData == 0xDEADC0DE)
        {
            code_ofs = pData - reinterpret_cast<uint8_t*>(pFunc);
            break;
        }  
    }

    // Write handler data into target process
    if (_pVEHCode.Write( 0, fnSize, pFunc ) != STATUS_SUCCESS ||
         _pVEHCode.Write( dataOfs, _proc.nativeLdr().LdrpInvertedFunctionTable() ) != STATUS_SUCCESS ||
         _pVEHCode.Write( code_ofs, &DecodeSystemPointer ) != STATUS_SUCCESS)
    {
        _pVEHCode.Free();
        return LastNtStatus();
    }

#endif

    ah.GenPrologue();

    // AddVectoredExceptionHandler(0, pHandler);
    ah.GenCall( reinterpret_cast<void*>(&AddVectoredExceptionHandler), { 0, _pVEHCode.ptr<size_t>() } );

    _proc.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _proc.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
    _hVEH = static_cast<size_t>(result);

    return (result == 0 ? STATUS_NOT_FOUND : STATUS_SUCCESS);
}

/// <summary>
/// Removes VEH from target process
/// </summary>
/// <returns></returns>
NTSTATUS MExcept::RemoveVEH()
{
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    uint64_t result = 0;

    ah.GenPrologue();

    // RemoveVectoredExceptionHandler(pHandler);
    ah.GenCall( reinterpret_cast<void*>(&RemoveVectoredExceptionHandler), { _hVEH } );

    _proc.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _proc.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
    _pVEHCode.Free();

    return STATUS_SUCCESS;
}

}

using namespace blackbone;

//
// VEH to inject into process
//
#ifdef _M_AMD64

//
// This thing is fragile as fuck.
// Assumptions were made after some testing and may not be 100% accurate
// Support for C++ exceptions generated by non VC++ compiler wasn't tested at all.
//
LONG CALLBACK VectoredHandler( PEXCEPTION_POINTERS ExceptionInfo )
{
    // Check if it's a VC++ exception
    // for SEH RtlAddFunctionTable is enough
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EH_EXCEPTION_NUMBER)
    {
        // Check exception site image boundaries
        if (ExceptionInfo->ExceptionRecord->ExceptionInformation[2] >= (ULONG_PTR)MExcept::g_pImageBase
             && ExceptionInfo->ExceptionRecord->ExceptionInformation[2] <= ((ULONG_PTR)MExcept::g_pImageBase + MExcept::g_imageSize))
        {
            // Assume that's our exception because ImageBase = 0 and not suitable magic number
            if (ExceptionInfo->ExceptionRecord->ExceptionInformation[0] == EH_PURE_MAGIC_NUMBER1
                && ExceptionInfo->ExceptionRecord->ExceptionInformation[3] == 0)
            {
                // CRT magic number
                ExceptionInfo->ExceptionRecord->ExceptionInformation[0] = (ULONG_PTR)EH_MAGIC_NUMBER1;

                // fix exception image base
                ExceptionInfo->ExceptionRecord->ExceptionInformation[3] = (ULONG_PTR)MExcept::g_pImageBase;
            }
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#else

/// <summary>
/// Handler itself.
/// </summary>
/// <param name="ExceptionInfo">Exception-related inforamtion</param>
/// <returns>Exception status</returns>
LONG __declspec(naked) CALLBACK VectoredHandler( PEXCEPTION_POINTERS /*ExceptionInfo*/ )
{
    PEXCEPTION_REGISTRATION_RECORD pFs;
    PRTL_INVERTED_FUNCTION_TABLE7 pTable; //= (PRTL_INVERTED_FUNCTION_TABLE7)0x773dc000;
    PRTL_INVERTED_FUNCTION_TABLE_ENTRY pEntries;
    PDWORD pDec, pStart;
    DWORD verMajor, verMinor;
    DWORD tmp;
    PEB_T* peb;
    bool newHandler;

    __asm
    {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE
        pushad
        mov eax, 0xDEADDA7A
        mov pTable, eax
    }

    pFs = reinterpret_cast<decltype(pFs)>(__readfsdword( 0 ));

    // Get OS version
    peb = reinterpret_cast<PEB_T*>(__readfsdword( 0x30 ));
    verMajor = peb->OSMajorVersion;
    verMinor = peb->OSMinorVersion;

    if (verMajor >= 6 && verMinor >= 2)
        pEntries = reinterpret_cast<decltype(pEntries)>(GET_FIELD_PTR( reinterpret_cast<PRTL_INVERTED_FUNCTION_TABLE8>(pTable), Entries ));
    else
        pEntries = reinterpret_cast<decltype(pEntries)>(GET_FIELD_PTR( pTable, Entries ));

    //
    // Add each handler to LdrpInvertedFunctionTable
    //
    for (; pFs && pFs != reinterpret_cast<EXCEPTION_REGISTRATION_RECORD*>(0xffffffff) 
          && pFs->Next != reinterpret_cast<EXCEPTION_REGISTRATION_RECORD*>(0xffffffff); 
          pFs = pFs->Next)
    {
        // Find image for handler
        for (ULONG imageIndex = 0; imageIndex < pTable->Count; imageIndex++)
        {
            if ( reinterpret_cast<size_t>(pFs->Handler) >= reinterpret_cast<size_t>(pEntries[imageIndex].ImageBase) &&
                 reinterpret_cast<size_t>(pFs->Handler) <= reinterpret_cast<size_t>(pEntries[imageIndex].ImageBase)
                                                           + pEntries[imageIndex].ImageSize)
            {
                newHandler = false;

                // Win8 always has ntdll.dll as first image, so we can safely skip its handlers.
                // Also ntdll.dll ExceptionDirectory isn't Encoded via RtlEncodeSystemPointer (it's plain address)
                if (verMajor >= 6 && verMinor >= 2 && imageIndex == 0)
                    break;

                //pStart = (DWORD*)DecodeSystemPointer( pEntries[imageIndex].ExceptionDirectory );
                pStart = (DWORD*)((int( __stdcall* )(PVOID))0xDEADC0DE)(pEntries[imageIndex].ExceptionDirectory);

                //
                // Add handler as fake SAFESEH record
                //
                for (pDec = pStart; pDec != nullptr && pDec < pStart + 0x100; pDec++)
                {
                    if (*pDec == 0)
                    {
                        *pDec = reinterpret_cast<size_t>(pFs->Handler) - reinterpret_cast<size_t>(pEntries[imageIndex].ImageBase);
                        pEntries[imageIndex].SizeOfTable++;
                        newHandler = true;

                        break;
                    }
                    // Already in table
                    else if ((*pDec + reinterpret_cast<DWORD>(pEntries[imageIndex].ImageBase)) == reinterpret_cast<DWORD>(pFs->Handler))
                        break;
                }

                // Sort handler addresses
                if (newHandler)
                    for (ULONG i = 0; i < pEntries[imageIndex].SizeOfTable; i++)
                        for (ULONG j = pEntries[imageIndex].SizeOfTable - 1; j > i; j--)
                            if (pStart[j - 1] > pStart[j])
                            {
                                tmp = pStart[j - 1];
                                pStart[j - 1] = pStart[j];
                                pStart[j] = tmp;
                            }
            }
        }
    }

    // Return control to SEH
    //return EXCEPTION_CONTINUE_SEARCH;
    __asm
    {
        popad
        mov esp, ebp
        pop ebp

        mov eax, EXCEPTION_CONTINUE_SEARCH
        ret 4

        // End of function marker
        _emit 0xCC
        _emit 0xCC
        _emit 0xCC
    }
}

#endif//_M_AMD64