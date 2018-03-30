#include "Tests.h"
#include "../BlackBone/Syscalls/Syscall.h"

TEST_CASE( "10. Syscall test" )
{
    std::cout << "Dynamic syscall test" << std::endl;

    constexpr SIZE_T requested = 0x3000;
    PVOID ptr = nullptr;
    SIZE_T size = requested;

    auto status = syscall::nt_syscall(
        syscall::get_index( "NtAllocateVirtualMemory" ),
        GetCurrentProcess(),
        &ptr,
        0,
        &size,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE
        );

#ifdef USE32
    // Not ready yet
    CHECK( status == STATUS_NOT_SUPPORTED );
#else
    REQUIRE_NT_SUCCESS( status );
    CHECK( ptr != nullptr );
    CHECK( size == requested );

    VirtualFree( ptr, 0, MEM_RELEASE );
#endif
}