#define CATCH_CONFIG_FAST_COMPILE
#include "Tests.h"
#include "../BlackBone/DriverControl/DriverControl.h"

TEST_CASE( "08. Driver" )
{
    std::cout << "Driver functionality testing" << std::endl;

    Process proc, thisProc;
    REQUIRE_NT_SUCCESS( proc.Attach( L"explorer.exe" ) );
    REQUIRE_NT_SUCCESS( thisProc.Attach( GetCurrentProcessId() ) );

    NTSTATUS status = Driver().EnsureLoaded();
    if (!NT_SUCCESS( status ))
    {
        WARN( "Failed to load driver, testing aborted" );
        CHECK( status == STATUS_OBJECT_NAME_NOT_FOUND );
        return;
    }

    DWORD depFlags = 0;
    BOOL perm = FALSE;
    uint8_t buf[0x1000] = { 0 };
    MEMORY_BASIC_INFORMATION64 memInfo = { 0 };

    // Disable DEP, x86 only;
    if (!proc.barrier().targetWow64)
    {
        status = Driver().DisableDEP( GetCurrentProcessId() );
        SAFE_CALL( GetProcessDEPPolicy, GetCurrentProcess(), &depFlags, &perm );
        CHECK_NT_SUCCESS( status );
        CHECK( (depFlags & PROCESS_DEP_ENABLE) == 0 );
    }
    else
        WARN("Can't run DEP test for x64 executable");

    // Make current process protected
    CHECK_NT_SUCCESS( status = Driver().ProtectProcess( GetCurrentProcessId(), Policy_Enable ) );
    CHECK( thisProc.core().isProtected() );
    if (NT_SUCCESS( status ))
        Driver().ProtectProcess( GetCurrentProcessId(), Policy_Disable );

    // Grant explorer.exe handle full access
    CHECK_NT_SUCCESS( status = Driver().PromoteHandle( GetCurrentProcessId(), proc.core().handle(), PROCESS_ALL_ACCESS ) );
    if (NT_SUCCESS( status ))
    {
        PUBLIC_OBJECT_BASIC_INFORMATION info = { 0 };
        ULONG sz = 0;
        status = SAFE_CALL( NtQueryObject, proc.core().handle(), ObjectBasicInformation, &info, (ULONG)sizeof( info ), &sz );
        CHECK_NT_SUCCESS( status );
        CHECK( info.GrantedAccess == PROCESS_ALL_ACCESS );
    }

    // Read explorer.exe PE header
    auto address = proc.modules().GetMainModule()->baseAddress;
    ptr_t size = 0x1000;

    CHECK_NT_SUCCESS( status = Driver().ReadMem( proc.pid(), address, size, buf ) );
    CHECK( *(uint16_t*)buf == IMAGE_DOS_SIGNATURE );

    // Allocate some memory
    ptr_t address2 = 0;
    CHECK_NT_SUCCESS( status = Driver().AllocateMem( proc.pid(), address2, size, MEM_COMMIT, PAGE_READWRITE ) );
    if (NT_SUCCESS( status ))
        Driver().FreeMem( proc.pid(), address2, size, MEM_RELEASE );

    // Make explorer.exe PE header writable
    status = Driver().ProtectMem( proc.pid(), address, 0x1000, PAGE_READWRITE );
    proc.memory().Query( address, &memInfo );
    CHECK_NT_SUCCESS( status );
    CHECK( (memInfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY)) );
    if (NT_SUCCESS( status ))
        proc.memory().Protect( address, 0x1000, PAGE_READONLY );

    // Unlink handle table
    //CHECK_NT_SUCCESS( status = Driver().UnlinkHandleTable( proc.pid() ) );

    // Enum regions
    std::vector<MEMORY_BASIC_INFORMATION64> info;
    CHECK_NT_SUCCESS( status = Driver().EnumMemoryRegions( proc.pid(), info ) );
    CHECK( info.size() > 0 );
}