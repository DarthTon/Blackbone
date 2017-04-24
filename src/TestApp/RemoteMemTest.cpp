#define CATCH_CONFIG_FAST_COMPILE
#include "Tests.h"
#include "../BlackBoneDrv/BlackBoneDef.h"

TEST_CASE( "09. Driver remote memory" )
{
    Process proc;

    std::cout << "Remote memory mapping test" << std::endl;
    NTSTATUS status = Driver().EnsureLoaded();
    if (!NT_SUCCESS( status ))
    {
        WARN( "Failed to load driver, testing aborted" );
        CHECK( status == STATUS_OBJECT_NAME_NOT_FOUND );
        return;
    }

    REQUIRE_NT_SUCCESS( proc.Attach( L"explorer.exe" ) );
    REQUIRE_NT_SUCCESS( status = proc.memory().Map( false ) );
    
    // Translate main module base address
    auto addr = proc.modules().GetMainModule()->baseAddress;
    auto translated = proc.memory().TranslateAddress( addr );
    CHECK( translated != 0 );

    CHECK_NT_SUCCESS( status = proc.memory().SetupHook( RemoteMemory::MemVirtualAlloc ) );
    CHECK_NT_SUCCESS( status = proc.memory().SetupHook( RemoteMemory::MemVirtualFree ) );
    CHECK_NT_SUCCESS( status = proc.memory().SetupHook( RemoteMemory::MemMapSection ) );
    CHECK_NT_SUCCESS( status = proc.memory().SetupHook( RemoteMemory::MemUnmapSection ) );

    if (NT_SUCCESS( status ))
    {
        std::wcout << L"TestRemoteMem: Hooks installed, pausing for 10s" << std::endl;
        Sleep( 10000 );
    }

    std::cout << std::endl;
}