#define CATCH_CONFIG_RUNNER
#include "Tests.h"

TEST_CASE( "01. PEB and TEB" )
{
    Process proc;
    REQUIRE_NT_SUCCESS( proc.Attach( GetCurrentProcessId() ) );

    SECTION( "PEB" ) 
    {
        std::wcout << std::endl << L"PEB test for current process" << std::endl;

        _PEB32 peb32 = { { { 0 } } };
        _PEB64 peb64 = { { { 0 } } };

        auto ppeb32 = proc.core().peb32( &peb32 );
        auto ppeb64 = proc.core().peb64( &peb64 );

        CHECK( ppeb64 != 0 );
        CHECK( peb64.Ldr != 0 );
        if (proc.barrier().targetWow64)
        {
            CHECK( ppeb32 != 0 );
            CHECK( peb32.Ldr != 0 );
        }
    }

    SECTION( "TEB" )
    {
        std::wcout << L"TEB info test for current process" << std::endl;

        _TEB32 teb32 = { { 0 } };
        _TEB64 teb64 = { { 0 } };

        auto pteb32 = proc.threads().getMain()->teb( &teb32 );
        auto pteb64 = proc.threads().getMain()->teb( &teb64 );

        CHECK( pteb64 != 0 );
        CHECK( teb64.ClientId.UniqueThread != 0 );
        if (proc.barrier().targetWow64)
        {
            CHECK( pteb32 != 0 );
            CHECK( teb32.ClientId.UniqueThread != 0 );
            CHECK( teb32.ClientId.UniqueThread == teb64.ClientId.UniqueThread );
        }
    }
}


int main( int argc, char* argv[] )
{
    Catch::Session session;

    session.applyCommandLine( argc, argv );
    session.configData().runOrder = Catch::RunTests::InLexicographicalOrder;

    session.run();
    TestMMap();

    return 0;
}