#include "../BlackBone/Config.h"
#include "Tests.h"

/*
    Prevent termination of current process in task manager.
*/
void TestRemoteHook()
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
                    std::wcout << L"Attempt to open current process with PROCESS_TERMINATE rights\n";
                    context.setArg( 1, mask & ~PROCESS_TERMINATE );
                }
            }
        }

        Process procTaskMgr;
    };

    HookClass hclass;
    auto found = Process::EnumByName( L"taskmgr.exe" );

    std::wcout << L"Function hooking test\n";
    std::wcout << L"Searching for taskmgr.exe... ";

    if (found.size() > 0)
    {
        std::wcout << L"Found. Attaching to process " << std::dec << found.front() << std::endl;

        auto status = hclass.procTaskMgr.Attach( found.front() );
        if (!NT_SUCCESS( status ))
        {
            std::wcout << L"Failed to attach to taskmgr.exe. Status 0x" << std::hex << status << std::endl << std::endl;
            return;
        }

        if (hclass.procTaskMgr.barrier().type == wow_32_64)
        {
            std::wcout << L"Can't hook functions inside x64 process from x86 one, aborting\n\n";
            return;
        }

        std::wcout << L"Searching for NtOpenProcess... ";

        // Get function
        auto pHookFn = hclass.procTaskMgr.modules().GetExport( hclass.procTaskMgr.modules().GetModule( L"ntdll.dll" ), "NtOpenProcess" );
        if (pHookFn)
        {
            std::wcout << L"Found. Hooking...\n";

            // Hook and wait some time.
            status = hclass.procTaskMgr.hooks().Apply( RemoteHook::hwbp, pHookFn->procAddress, &HookClass::HookFn, hclass );
            if (NT_SUCCESS( status ))
            {
                std::wcout << L"Hooked successfully. Try to terminate TestApp.exe from taskmgr now.\n";
                Sleep( 20000 );
            }
            else
                std::wcout << L"Failed to install hook. Status 0x" << std::hex << status << std::endl;
        }
        else
            std::wcout << L"Not found, aborting\n";
    }
    else
        std::wcout << L"Not found, aborting\n";

    std::wcout << std::endl;
#endif
}
