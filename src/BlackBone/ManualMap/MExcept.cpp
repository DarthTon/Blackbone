#include "MExcept.h"
#include "../Process/Process.h"
#include "../Include/Macro.h"
#include "../Misc/Trace.hpp"
#include "../Asm/LDasm.h"

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
/// <param name="mt">Mosule type</param>
/// <param name="partial">Partial exception support</param>
/// <returns>Error code</returns>
NTSTATUS MExcept::CreateVEH( uintptr_t pTargetBase, size_t imageSize, eModType mt, bool partial )
{    
    AsmJitHelper a;
    uint64_t result = 0;
    auto& mods = _proc.modules();

#ifdef USE64 

    // Add module to module table
    if (!_pModTable.valid())
    {
        _pModTable = _proc.memory().Allocate( 0x1000 );
        _pModTable.Release();
        if (!_pModTable.valid())
            return LastNtStatus();
    }

    ModuleTable table;
    _pModTable.Read ( 0, table );

    // Add new entry to the table
    table.entry[table.count].base = pTargetBase;
    table.entry[table.count].size = imageSize;
    table.count++;

    _pModTable.Write( 0, table );

    // No handler required
    if (partial)
        return STATUS_SUCCESS;

    // VEH codecave
    _pVEHCode = _proc.memory().Allocate( 0x2000 );
    _pVEHCode.Release();
    if (!_pVEHCode.valid())
        return LastNtStatus();

    BLACBONE_TRACE( "ManualMap: Vectored hander: 0x%p\n", _pVEHCode.ptr() );

    asmjit::Label lExit  = a->newLabel();
    asmjit::Label lLoop1 = a->newLabel();
    asmjit::Label skip1  = a->newLabel();
    asmjit::Label found1 = a->newLabel();

    //
    // Assembly code for VectoredHandler64
    // 0x10 - EXCEPTION_RECORD.ExceptionAddress
    // 0x20 - EXCEPTION_RECORD.ExceptionInformation[0]
    // 0x30 - EXCEPTION_RECORD.ExceptionInformation[2]
    // 0x38 - EXCEPTION_RECORD.ExceptionInformation[3]
    //
    a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rcx ) );
    a->cmp( asmjit::host::dword_ptr( asmjit::host::rax ), EH_EXCEPTION_NUMBER );            // Exception code
    a->jne( lExit );
    a->cmp( asmjit::host::qword_ptr( asmjit::host::rax, 0x20 ), EH_PURE_MAGIC_NUMBER1 );    // Sub code
    a->jne( lExit );
    a->cmp( asmjit::host::qword_ptr( asmjit::host::rax, 0x38 ), 0 );                        // Image base
    a->jne( lExit );

    a->mov( asmjit::host::r9, _pModTable.ptr() );
    a->mov( asmjit::host::rdx, asmjit::host::qword_ptr( asmjit::host::r9 ) );               // Record count
    a->add( asmjit::host::r9, sizeof( table.count ) );
    a->xor_( asmjit::host::r10, asmjit::host::r10 );

    a->bind( lLoop1 );

    a->mov( asmjit::host::r8, asmjit::host::qword_ptr( asmjit::host::rax, 0x30 ) );
    a->mov( asmjit::host::r11, asmjit::host::qword_ptr( asmjit::host::r9 ) );
    a->cmp( asmjit::host::r8, asmjit::host::r11 );
    a->jl( skip1 );
    a->add( asmjit::host::r11, asmjit::host::qword_ptr( asmjit::host::r9, sizeof( table.entry[0].base ) ) );    // Size
    a->cmp( asmjit::host::r8, asmjit::host::r11 );
    a->jg( skip1 );

    a->jmp( found1 );

    a->bind( skip1 );
    a->add( asmjit::host::r9, sizeof( ExceptionModule ) );
    a->add( asmjit::host::r10, 1 );
    a->cmp( asmjit::host::r10, asmjit::host::rdx );
    a->jne( lLoop1 );

    a->jmp( lExit );
    
    a->bind( found1 );
    a->mov( asmjit::host::qword_ptr( asmjit::host::rax, 0x20 ), EH_MAGIC_NUMBER1 );
    a->mov( asmjit::host::rcx, asmjit::host::qword_ptr( asmjit::host::rcx ) );
    a->mov( asmjit::host::rdx, asmjit::host::qword_ptr( asmjit::host::r9 ) );
    a->mov( asmjit::host::qword_ptr( asmjit::host::rax, 0x38 ), asmjit::host::rdx );

    a->bind( lExit );
    a->xor_( asmjit::host::rax, asmjit::host::rax );
    a->ret();
    a->db( 0xCC );
    a->db( 0xCC );
    a->db( 0xCC );

    if (_pVEHCode.Write( 0, a->getCodeSize(), a->make() ) != STATUS_SUCCESS)
    {
        _pVEHCode.Free();
        return LastNtStatus();
    }
#else
    UNREFERENCED_PARAMETER( pTargetBase );
    UNREFERENCED_PARAMETER( imageSize );

    // No handler required
    if (partial)
        return STATUS_SUCCESS;

    // VEH codecave
    _pVEHCode = _proc.memory().Allocate( 0x2000 );
    _pVEHCode.Release();
    if (!_pVEHCode.valid())
        return LastNtStatus();

    // Resolve compiler incremental table address, if any
    void *pFunc = ResolveJmp( &VectoredHandler );
    size_t fnSize = static_cast<size_t>(SizeOfProc( pFunc ));
    size_t dataOfs = 0, code_ofs = 0, code_ofs2 = 0;;

    // Find and replace magic values
    for (uint8_t *pData = reinterpret_cast<uint8_t*>(pFunc);
          pData < reinterpret_cast<uint8_t*>(pFunc) + fnSize - 4; 
          pData++)
    {
        // LdrpInvertedFunctionTable
        if(*(uintptr_t*)pData == 0xDEADDA7A)
        {
            dataOfs = pData - reinterpret_cast<uint8_t*>(pFunc);
            continue;
        }

        // DecodeSystemPointer address
        if(*(uintptr_t*)pData == 0xDEADC0DE)
        {
            code_ofs = pData - reinterpret_cast<uint8_t*>(pFunc);
            break;
        }  

        // LdrProtectMrdata address
        if (*(uintptr_t*)pData == 0xDEADC0D2)
        {
            code_ofs2 = pData - reinterpret_cast<uint8_t*>(pFunc);
            continue;
        }
    }

    auto pDecode = mods.GetExport( mods.GetModule( L"ntdll.dll", Sections, mt ), "RtlDecodeSystemPointer" ).procAddress;

    // Write handler data into target process
    if ( !NT_SUCCESS( _pVEHCode.Write( 0, fnSize, pFunc ) ) ||
         !NT_SUCCESS( _pVEHCode.Write( dataOfs, _proc.nativeLdr().LdrpInvertedFunctionTable() ) ) ||
         !NT_SUCCESS( _pVEHCode.Write( code_ofs, static_cast<uintptr_t>(pDecode) ) ) ||
         !NT_SUCCESS( _pVEHCode.Write( code_ofs2, _proc.nativeLdr().LdrProtectMrdata() ) ))
    {
        _pVEHCode.Free();
        return LastNtStatus();
    }

#endif
    // AddVectoredExceptionHandler(0, pHandler);
    auto pAddHandler = mods.GetExport( mods.GetModule( L"ntdll.dll", Sections, mt ), "RtlAddVectoredExceptionHandler" ).procAddress;
    if (pAddHandler == 0)
        return STATUS_NOT_FOUND;

    a->reset();
    a.GenPrologue();

    a.GenCall( static_cast<uintptr_t>(pAddHandler), { 0, _pVEHCode.ptr<uintptr_t>() } );

    _proc.remote().AddReturnWithEvent( a, mt );
    a.GenEpilogue();

    _proc.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result );
    _hVEH = static_cast<uintptr_t>(result);

    return (_hVEH == 0 ? STATUS_NOT_FOUND : STATUS_SUCCESS);
}

/// <summary>
/// Removes VEH from target process
/// </summary>
/// <param name="partial">Partial exception support</param>
/// <returns>Status code</returns>
NTSTATUS MExcept::RemoveVEH( bool partial )
{  
    AsmJitHelper a;
    uint64_t result = 0;

    auto& mods = _proc.modules();
    auto pRemoveHandler = mods.GetExport( mods.GetModule( L"ntdll.dll" ), "RtlRemoveVectoredExceptionHandler" ).procAddress;
    if (pRemoveHandler == 0)
        return STATUS_NOT_FOUND;

    a.GenPrologue();

    // RemoveVectoredExceptionHandler(pHandler);
    a.GenCall( static_cast<uintptr_t>(pRemoveHandler), { _hVEH } );

    _proc.remote().AddReturnWithEvent( a );
    a.GenEpilogue();

    // Destroy table and handler
    if (!partial)
    {
        _proc.remote().ExecInWorkerThread( a->make(), a->getCodeSize(), result );
        _pVEHCode.Free();
        _hVEH = 0;

        _pModTable.Free();
    }        

    return STATUS_SUCCESS;
}

}

using namespace blackbone;

//
// VEH to inject into process
//
#ifdef USE64

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
    PRTL_INVERTED_FUNCTION_TABLE7 pTable;// = (PRTL_INVERTED_FUNCTION_TABLE7)0x777dc2b0;
    PRTL_INVERTED_FUNCTION_TABLE_ENTRY pEntries;
    void( __stdcall* LdrProtectMrdata )(BOOL a);
    PDWORD pDec, pStart;
    DWORD verMajor, verMinor, tmp;
    PEB_T* peb;
    bool newHandler, useNewTable;

    __asm
    {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE
        pushad
        mov eax, 0xDEADDA7A
        mov pTable, eax
        mov eax, 0xDEADC0D2
        mov LdrProtectMrdata, eax
    }

    pFs = reinterpret_cast<decltype(pFs)>(__readfsdword( 0 ));

    // Get OS version
    peb = reinterpret_cast<PEB_T*>(__readfsdword( 0x30 ));
    verMajor = peb->OSMajorVersion;
    verMinor = peb->OSMinorVersion;

    useNewTable = (verMajor >= 6 && verMinor >= 2) || verMajor >= 10;

    if (useNewTable)
        pEntries = reinterpret_cast<decltype(pEntries)>(GET_FIELD_PTR( reinterpret_cast<PRTL_INVERTED_FUNCTION_TABLE8>(pTable), Entries ));
    else
        pEntries = reinterpret_cast<decltype(pEntries)>(GET_FIELD_PTR( pTable, Entries ));


    //LdrProtectMrdata = (decltype(LdrProtectMrdata))0x777043aa;
    if(LdrProtectMrdata)
        LdrProtectMrdata( FALSE );

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
            if ( reinterpret_cast<uintptr_t>(pFs->Handler) >= reinterpret_cast<uintptr_t>(pEntries[imageIndex].ImageBase) &&
                 reinterpret_cast<uintptr_t>(pFs->Handler) <= reinterpret_cast<uintptr_t>(pEntries[imageIndex].ImageBase)
                                                           + pEntries[imageIndex].ImageSize)
            {
                newHandler = false;

                // Win8+ always has ntdll.dll as first image, so we can safely skip its handlers.
                // Also ntdll.dll ExceptionDirectory isn't Encoded via RtlEncodeSystemPointer (it's plain address)
                if (useNewTable && imageIndex == 0)
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
                        *pDec = reinterpret_cast<uintptr_t>(pFs->Handler) - reinterpret_cast<uintptr_t>(pEntries[imageIndex].ImageBase);
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

    if(LdrProtectMrdata)
        LdrProtectMrdata( TRUE );

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

#endif//USE64
