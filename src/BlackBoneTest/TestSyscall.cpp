#include "Common.h"

namespace Testing
{
    TEST_CLASS( Syscall )
    {
    public:
        TEST_METHOD( Basic )
        {
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
            Assert::AreEqual( STATUS_NOT_SUPPORTED, status );
#else
            AssertEx::NtSuccess( status );

            Assert::IsNotNull( ptr );
            Assert::AreEqual( requested, size );

            VirtualFree( ptr, 0, MEM_RELEASE );
#endif
        }
    };
}