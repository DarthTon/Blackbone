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
    std::vector<DWORD> found;
    Process::EnumByName( L"taskmgr.exe", found );

    std::wcout << L"Function hooking test\n";
    std::wcout << L"Searching for taskmgr.exe... ";

    if (found.size() > 0)
    {
        std::wcout << L"Found. Attaching to process " << std::dec << found.front() << std::endl;

        if (!NT_SUCCESS( hclass.procTaskMgr.Attach( found.front() ) ))
        {
            std::wcout << L"Failed to attach to taskmgr.exe...\n\n";
            return;
        }

        if (hclass.procTaskMgr.core().native()->GetWow64Barrier().type == wow_32_64)
        {
            std::wcout << L"Can't hook functions inside x64 process from x86 one, aborting\n\n";
            return;
        }

        std::wcout << L"Searching for NtOpenProcess... ";

        // Get function
        auto pHookFn = hclass.procTaskMgr.modules().GetExport(
            hclass.procTaskMgr.modules().GetModule( L"ntdll.dll" ), 
            "NtOpenProcess"
            );

        if (pHookFn.procAddress != 0)
        {
            std::wcout << L"Found. Hooking...\n";

            // Hook and wait some time.
            auto status = hclass.procTaskMgr.hooks().Apply( RemoteHook::hwbp, pHookFn.procAddress, &HookClass::HookFn, hclass );
            if (NT_SUCCESS( status ))
            {
                std::wcout << L"Hooked successfully. Try to terminate TestApp.exe from taskmgr now.\n";
                Sleep( 20000 );
            }
            else
                std::wcout << L"Failed to install hook. Status" << std::hex << status << "\n";
        }
        else
            std::wcout << L"Not found, aborting\n";
    }
    else
        std::wcout << L"Not found, aborting\n";

    std::wcout << std::endl;
#endif
}
