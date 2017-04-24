#define CATCH_CONFIG_FAST_COMPILE
#include "Tests.h"
#include "../BlackBone/Patterns/PatternSearch.h"

TEST_CASE( "04. Patterns" )
{
    Process explorer;

    REQUIRE_NT_SUCCESS( explorer.Attach( L"explorer.exe" ) );

    // Scan all allocated process memory
    SECTION( "Pattern '48 89 D0'" )
    {
        std::wcout << L"Local pattern match in 'explorer.exe'" << std::endl;

        PatternSearch ps1( "\x48\x89\xD0" );

        std::vector<ptr_t> results;
        ps1.SearchRemoteWhole( explorer, false, 0, results );
        CHECK( results.size() > 0 );
    }   

    // Scan only inside 'explorer.exe' module
    SECTION( "Pattern '56 57 ?? 55' with CC wildcard" )
    {
        std::wcout << L"Remote pattern match in 'explorer.exe'" << std::endl;

        PatternSearch ps2{ 0x56, 0x57, 0xCC, 0x55 };

        auto pMainMod = explorer.modules().GetMainModule();
        REQUIRE( pMainMod.get() != nullptr );

        std::vector<ptr_t> results;
        ps2.SearchRemote( explorer, 0xCC, pMainMod->baseAddress, pMainMod->size, results );
        CHECK( results.size() > 0 );
    }
}