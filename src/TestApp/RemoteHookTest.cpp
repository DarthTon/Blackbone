#define CATCH_CONFIG_FAST_COMPILE
#include "../BlackBone/Config.h"
#include "Tests.h"

/*
    Prevent termination of current process in task manager.
*/
TEST_CASE( "07. Remote hooks" )
{
#ifdef COMPILER_MSVC
    struct HookClass
    {
        void HookFn( RemoteContext& context )
        {
            //
            // Get process ID
            //
            _CLIENT_ID_T<DWORD_PTR> cid = { 0 };
            auto pcid = context.getArg( 3 );
            procTaskMgr.memory().Read( pcid, sizeof(cid), &cid );
            DWORD pid = (DWORD)cid.UniqueProcess;

            if (pid == GetCurrentProcessId())
            {
                // Remove PROCESS_TERMINATE access right
                auto mask = context.getArg( 1 );
                if (mask & PROCESS_TERMINATE)
                {
                    std::wcout << L"Attempt to open current process with PROCESS_TERMINATE rights" << std::endl;
                    context.setArg( 1, mask & ~PROCESS_TERMINATE );
                }
            }
        }

        Process procTaskMgr;
    };

    HookClass hclass;
    std::wcout << L"Function hooking test" << std::endl;
    NTSTATUS status = hclass.procTaskMgr.Attach( L"taskmgr.exe" );
    if (!NT_SUCCESS( status ))
    {
        WARN( "Failed to attach to taskmgr process" );
        CHECK( status == STATUS_NOT_FOUND );
        return;
    }

    //std::wcout << L"Found. Attaching to process " << std::dec << found.front() << std::endl;
    bool validArch = (hclass.procTaskMgr.barrier().type != wow_32_64);
    if (!validArch)
    {
        WARN( "Can't hook functions inside x64 process from x86 one" );
        //std::wcout << L"Can't hook functions inside x64 process from x86 one, aborting\n\n";
        return;
    }

    // Get function
    auto pHookFn = hclass.procTaskMgr.modules().GetNtdllExport( "NtOpenProcess" );
    REQUIRE( pHookFn.success() );

    //std::wcout << L"Found. Hooking...\n";

    // Hook and wait some time.
    status = hclass.procTaskMgr.hooks().Apply( RemoteHook::hwbp, pHookFn->procAddress, &HookClass::HookFn, hclass );
    CHECK_NT_SUCCESS( status );
    if (NT_SUCCESS( status ))
    {
        std::wcout << L"Hooked successfully. Try to terminate TestApp.exe from taskmgr now" << std::endl;
        Sleep( 20000 );
    }
#endif
}
