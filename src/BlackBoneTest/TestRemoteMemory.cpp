#include "Common.h"

namespace Testing
{
    TEST_CLASS( RemoteMem )
    {
    public:
        TEST_METHOD( Basic )
        {
            Process proc;

            NTSTATUS status = Driver().EnsureLoaded();
            if (!NT_SUCCESS( status ))
            {
                Assert::AreEqual( STATUS_OBJECT_NAME_NOT_FOUND, status );
                return;
            }

            AssertEx::NtSuccess( proc.Attach( L"explorer.exe" ) );
            AssertEx::NtSuccess( proc.memory().Map( false ) );

            // Translate main module base address
            auto addr = proc.modules().GetMainModule()->baseAddress;
            auto translated = proc.memory().TranslateAddress( addr );
            AssertEx::IsNotZero( translated );

            AssertEx::NtSuccess( proc.memory().SetupHook( RemoteMemory::MemVirtualAlloc ) );
            AssertEx::NtSuccess( proc.memory().SetupHook( RemoteMemory::MemVirtualFree ) );
            AssertEx::NtSuccess( proc.memory().SetupHook( RemoteMemory::MemMapSection ) );
            AssertEx::NtSuccess( proc.memory().SetupHook( RemoteMemory::MemUnmapSection ) );
        }
    };
}