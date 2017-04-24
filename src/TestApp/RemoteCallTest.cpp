#include "../BlackBone/Config.h"

#ifdef COMPILER_MSVC
#define CATCH_CONFIG_FAST_COMPILE
#include "Tests.h"
#include "../BlackBone/Process/RPC/RemoteFunction.hpp"
#endif

/*
    Get explorer.exe path
*/
TEST_CASE( "05. Remote function call" )
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

    auto hMainMod = explorer.modules().GetMainModule();
    auto pRemote = explorer.modules().GetNtdllExport( "NtQueryVirtualMemory" );

    REQUIRE( hMainMod );
    REQUIRE( pRemote );

    //std::wcout << L"Found. Executing...\n";
    uint8_t buf[1024] = { 0 };

    RemoteFunction<fnNtQueryVirtualMemory> pFN( explorer, pRemote->procAddress );
    decltype(pFN)::CallArguments args(
        INVALID_HANDLE_VALUE,
        reinterpret_cast<PVOID>(hMainMod->baseAddress),
        MemorySectionName,
        reinterpret_cast<PVOID>(buf),
        static_cast<SIZE_T>(sizeof( buf )),
        reinterpret_cast<PSIZE_T>(0)
        );

    args.set( 3, AsmVariant( buf, sizeof( buf ) ) );
    auto result = pFN.Call( args );
    REQUIRE_NT_SUCCESS( result.status );
    REQUIRE_NT_SUCCESS( result.result() );

    std::wstring name( (wchar_t*)(buf + sizeof( UNICODE_STRING )) );
    CHECK( name.find( L"explorer.exe" ) != name.npos );
#endif // COMPILER_MSVC
}