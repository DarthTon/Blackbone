#include "../BlackBone/Config.h"

#ifdef COMPILER_MSVC
#include "Tests.h"
#include "../BlackBone/Process/RPC/RemoteFunction.hpp"
#endif

/*
    Get explorer.exe path
*/
TEST_CASE( "04. Remote function call" )
{
#ifdef COMPILER_MSVC
    std::wcout << L"Remote function call test inside 'explorer.exe'" << std::endl;

    Process explorer;
    REQUIRE_NT_SUCCESS( explorer.Attach( L"explorer.exe" ) );

    auto barrier = explorer.barrier().type;
    bool validArch = (barrier == wow_64_64 || barrier == wow_32_32);
    if (!validArch)
    {
        WARN( "Can't call function through WOW64 barrier" );
        return;
    }
    //std::wcout << L"Searching for NtQueryVirtualMemory... ";

    auto hMainMod = explorer.modules().GetMainModule();
    auto pRemote = explorer.modules().GetExport( explorer.modules().GetModule( L"ntdll.dll" ), "NtQueryVirtualMemory" );

    REQUIRE( hMainMod );
    REQUIRE( pRemote );

    //std::wcout << L"Found. Executing...\n";
    uint8_t buf[1024] = { 0 };

    RemoteFunction<fnNtQueryVirtualMemory> pFN(
        explorer,
        pRemote->procAddress,
        INVALID_HANDLE_VALUE,
        reinterpret_cast<LPVOID>(hMainMod->baseAddress),
        MemorySectionName,
        reinterpret_cast<LPVOID>(buf),
        static_cast<SIZE_T>(sizeof( buf )),
        reinterpret_cast<PSIZE_T>(0)
    );

    pFN.setArg( 3, AsmVariant( buf, sizeof( buf ) ) );
    auto result = pFN.Call();
    REQUIRE_NT_SUCCESS( result.status );

    std::wstring name( (wchar_t*)(buf + sizeof( UNICODE_STRING )) );
    CHECK( name.find( L"explorer.exe" ) != name.npos );

    //std::wcout << L"Call result 0x" << std::hex << result << std::dec << L" . Module path " << pStr << std::endl;
#endif // COMPILER_MSVC
}