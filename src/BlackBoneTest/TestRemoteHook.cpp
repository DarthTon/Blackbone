#include "Common.h"

namespace Testing
{
    struct HookClass
    {
        void HookFn( RemoteContext& context )
        {
            //
            // Get process ID
            //
            _CLIENT_ID_T<DWORD_PTR> cid = { };
            auto pcid = context.getArg( 3 );
            process.memory().Read( pcid, sizeof( cid ), &cid );
            DWORD pid = static_cast<DWORD>(cid.UniqueProcess);

            if (pid == GetCurrentProcessId())
            {
                // Remove PROCESS_TERMINATE access right
                auto mask = context.getArg( 1 );
                if (mask & PROCESS_TERMINATE)
                {
                    context.setArg( 1, mask & ~PROCESS_TERMINATE );
                    calls++;
                }
            }
        }

        Process process;
        int calls = 0;
    };

    TEST_CLASS( RemoteHooking )
    {
    public:
        TEST_METHOD( NtOpenProcess )
        {
            HookClass hooker;

            auto path = GetTestHelperHost();
            Assert::IsFalse( path.empty() );


            // Give process some time to initialize
            AssertEx::NtSuccess( hooker.process.CreateAndAttach( path ) );
            Sleep( 100 );

            // Remote helper function
            auto terminatePtr = hooker.process.modules().GetExport( hooker.process.modules().GetMainModule(), "TerminateProc" );
            Assert::IsTrue( terminatePtr.success() );

            // Get function
            auto pHookFn = hooker.process.modules().GetNtdllExport( "NtOpenProcess" );
            Assert::IsTrue( pHookFn.success() );

            // Hook and try to terminate from remote process
            AssertEx::NtSuccess( hooker.process.hooks().Apply( RemoteHook::hwbp, pHookFn->procAddress, &HookClass::HookFn, hooker ) );

            RemoteFunction<long( *)(DWORD)> terminate( hooker.process, terminatePtr->procAddress );
            auto result = terminate.Call( GetCurrentProcessId() );

            hooker.process.Terminate();

            Assert::IsTrue( result.success() );
            Assert::AreEqual( ERROR_ACCESS_DENIED, result.result() );
            Assert::AreEqual( 1, hooker.calls );
        }
    };
}