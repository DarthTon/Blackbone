#include "../BlackBone/Process.h"
#include "../BlackBone/PatternSearch.h"
#include "../BlackBone/PEParser.h"
#include "../BlackBone/RemoteFunction.hpp"
#include "../BlackBone/Utils.h"
#include "../BlackBone/DynImport.h"

#include <iostream>

using namespace blackbone;

/*
*/
void TestPEB( Process& proc )
{
   _PEB32 peb32 = { 0 };
   _PEB64 peb64 = { 0 };

    std::wcout << L"PEB info test\n";

    auto ppeb32 = proc.core().peb( &peb32 );
    std::wcout << L"PEB32 address 0x" << std::hex << ppeb32 
               << L". Ldr address 0x" << peb32.Ldr << std::endl;

    auto ppeb64 = proc.core().peb( &peb64 );
    std::wcout << L"PEB64 address 0x" << std::hex << ppeb64 
               << L". Ldr address 0x" << peb64.Ldr << std::endl << std::endl;
}

/*
*/
void TestTEB( Process& proc )
{
    _TEB32 teb32 = { 0 };
    _TEB64 teb64 = { 0 };

    std::wcout << L"TEB info test\n";

    auto pteb32 = proc.threads().getMain()->teb( &teb32 );
    std::wcout << L"TEB32 address 0x" << std::hex << pteb32 << L". Thread ID 0x" 
               << teb32.ClientId.UniqueThread << std::endl;

    auto pteb64 = proc.threads().getMain()->teb( &teb64 );
    std::wcout << L"TEB64 address 0x" << std::hex << pteb64 << L". Thread ID 0x" 
               << teb64.ClientId.UniqueThread << std::endl << std::endl;
}

/*
    Prevent termination of current process in task manager.
*/
void TestHook()
{
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

        hclass.procTaskMgr.Attach( found.front() );

        if (hclass.procTaskMgr.core().native()->GetWow64Barrier().type == wow_32_64)
        {
            std::wcout << L"Can't hook functions inside x64 process from x86 one, aborting\n\n";
            return;
        }

        std::wcout << L"Searching for NtOpenProcess... ";

        // Get function
        auto pHookFn = hclass.procTaskMgr.modules().GetExport( 
            hclass.procTaskMgr.modules().GetModule( L"ntdll.dll" ), "NtOpenProcess" );

        if (pHookFn.procAddress != 0)
        {
            std::wcout << L"Found. Hooking...\n";

            // Hook and wait some time.
            if(hclass.procTaskMgr.hooks().Apply( RemoteHook::ht_hwbp, pHookFn.procAddress, &HookClass::HookFn, hclass ))
            {
                std::wcout << L"Hooked successfully. Try to terminate TestApp.exe from taskmgr now.\n";
                Sleep( 20000 );
            }
        }
        else
            std::wcout << L"Not found, aborting\n";
    }
    else
        std::wcout << L"Not found, aborting\n";

    std::wcout << std::endl;
}

/*
    Try to map calc.exe into current process
*/
void TestMMap()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    std::wcout << L"Manual image mapping test\n";
    std::wcout << L"Trying to map C:\\windows\\system32\\calc.exe into current process\n";

    int flags = CreateLdrRef | ManualImports | RebaseProcess | NoDelayLoad;

    if (thisProc.mmap().MapImage( L"C:\\windows\\system32\\calc.exe", flags ) == 0)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << LastNtStatus()
                   << L". " << Utils::GetErrorDescription( LastNtStatus() ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    thisProc.mmap().UnmapAllModules();
}

/*
    Get csrss.exe path
*/
void TestRemoteCall()
{
    std::wcout << L"Remote function call test\n";
    std::wcout << L"Searching for explorer.exe... ";

    Process csrss;
    std::vector<DWORD> found;
    Process::EnumByName( L"explorer.exe", found );

    if (found.size() > 0)
    {
        std::wcout << L"Found. Attaching to process " << std::dec << found.front() << std::endl;

        if(csrss.Attach( found.front() ) != STATUS_SUCCESS)
        {
            std::wcout << L"Can't attach to process, status code " << LastNtStatus() << " aborting\n\n";
            return;
        }

        auto barrier = csrss.core().native()->GetWow64Barrier().type;

        if (barrier != wow_32_32 && barrier != wow_64_64)
        {
            std::wcout << L"Can't execute call through WOW64 barrier, aborting\n\n";
            return;
        }

        std::wcout << L"Searching for NtQueryVirtualMemory... ";

        auto hMainMod = csrss.modules().GetMainModule();
        auto pRemote = csrss.modules().GetExport( 
            csrss.modules().GetModule( L"ntdll.dll" ), "NtQueryVirtualMemory" );

        if (hMainMod && pRemote.procAddress)
        {
            std::wcout << L"Found. Executing...\n";
            uint8_t buf[1024] = { 0 };

            RemoteFunction<fnNtQueryVirtualMemory> pFN( csrss, (fnNtQueryVirtualMemory)pRemote.procAddress,
                                                        INVALID_HANDLE_VALUE, (LPVOID)hMainMod->baseAddress, 
                                                        MemorySectionName, buf, sizeof(buf), nullptr );

            decltype(pFN)::ReturnType result;

            pFN.setArg( 3, AsmVariant( buf, sizeof(buf) ) );
            pFN.Call( result );

            wchar_t* pStr = (wchar_t*)(buf + sizeof(UNICODE_STRING));

            std::wcout << L"Call result " << result << L" . Module path " << pStr << std::endl;
        }
        else
            std::wcout << L"Not found, aborting\n";
    }
    else
        std::wcout << L"Not found, aborting\n";

    std::wcout << std::endl;
}

int wmain( int /*argc*/, wchar_t* /*argv*/[] )
{
    Process proc;
    proc.Attach( GetCurrentProcessId() );

    TestPEB( proc );
    TestTEB( proc );
    TestRemoteCall();
    //TestHook();
    TestMMap();

	return 0;
}
