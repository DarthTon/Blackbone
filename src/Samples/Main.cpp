#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <BlackBone/Syscalls/Syscall.h>
#include <iostream>

using namespace blackbone;

void MapCalcFromFile();
void MapCmdFromMem();

int main( int /*argc*/, char* /*argv[]*/ )
{
    // List all process PIDs matching name
    auto pids = Process::EnumByName( L"explorer.exe" );

    // List all process PIDs matching either by PID only
    auto procInfo = Process::EnumByNameOrPID( 0x1234, L"" );

    // List all processes
    auto all = Process::EnumByNameOrPID( 0, L"" );

    // Attach to a process
    if (Process explorer; !pids.empty() && NT_SUCCESS( explorer.Attach( pids.front() ) ))
    {
        auto& core = explorer.core();

        // Get bitness info about this and target processes
        [[maybe_unused]] auto barrier = explorer.barrier();

        // Get process PID and handle
        [[maybe_unused]] auto pid = core.pid();
        [[maybe_unused]] auto handle = core.handle();

        // Get PEB
        PEB_T peb = { };
        [[maybe_unused]] auto peb_ptr = core.peb( &peb );

        // Get all process handles
        if (auto handles = explorer.EnumHandles(); handles)
        {
            // do stuff with handles...
        }
    }

    // Start new suspended process and attach immediately
    Process notepad; 
    notepad.CreateAndAttach( L"C:\\windows\\system32\\notepad.exe", true );
    {
        // do stuff...
        notepad.Resume();
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    // Process modules manipulation
    {
        auto& modules = notepad.modules();

        // List all modules (both x86 and x64 for WOW64 processes)
        auto mods = modules.GetAllModules();

        // Get main module (.exe)
        auto mainMod = modules.GetMainModule();

        // Get module base address
        [[maybe_unused]] auto base = mainMod->baseAddress;

        // Get export symbol from module found by name
        auto LoadLibraryWPtr = modules.GetExport( L"kernel32.dll", "LoadLibraryW" );
        if (LoadLibraryWPtr)
        {
        }

        // Unlink module from loader structures
        if (modules.Unlink( mainMod ))
        {
        }
    }

    // Process memory manipulation
    {
        auto& memory = notepad.memory();
        auto mainMod = notepad.modules().GetMainModule();

        //
        // Read memory
        //
        IMAGE_DOS_HEADER dosHeader = { };

        // Method 1
        memory.Read( mainMod->baseAddress, dosHeader );

        // Method 2
        memory.Read( mainMod->baseAddress, sizeof( dosHeader ), &dosHeader );

        // Method 3
        auto[status, dosHeader2] = memory.Read<IMAGE_DOS_HEADER>( mainMod->baseAddress );

        // Change memory protection
        if (NT_SUCCESS( memory.Protect( mainMod->baseAddress, sizeof( dosHeader ), PAGE_READWRITE ) ))
        {
            //
            // Write memory
            //

            // Method 1
            memory.Write( mainMod->baseAddress, dosHeader );

            // Method 2
            memory.Write( mainMod->baseAddress, sizeof( dosHeader ), &dosHeader );
        }

        // Allocate memory
        if (auto[status2, block] = memory.Allocate( 0x1000, PAGE_EXECUTE_READWRITE ); NT_SUCCESS( status2 ))
        {
            // Write into memory block
            block->Write( 0x10, 12.0 );

            // Read from memory block
            [[maybe_unused]] auto dval = block->Read<double>( 0x10, 0.0 );
        }

        // Enumerate regions
        auto regions = memory.EnumRegions();
    }

    // Threads manipulation
    {
        // Get all thread
        auto threads = notepad.threads().getAll();

        // Get main thread
        auto mainThread = notepad.threads().getMain();

        // Get thread by TID
        auto thread = notepad.threads().get( mainThread->id() );

        // Get context
        CONTEXT_T ctx = { };
        if (thread->GetContext( ctx, CONTEXT_FLOATING_POINT ))
        {
            // Set context
            thread->SetContext( ctx );
        }

        // Wait for thread exit
        thread->Join( 100 );
    }

    // JIT Assembler
    if (auto asmPtr = AsmFactory::GetAssembler())
    {
        auto& a = *asmPtr;

        a.GenPrologue();
        a->add( a->zcx, a->zdx );
        a->mov( a->zax, a->zcx );
        a.GenEpilogue();

        auto func = reinterpret_cast<uintptr_t( __fastcall* )(uintptr_t, uintptr_t)>(a->make());
        [[maybe_unused]] uintptr_t r = func( 10, 5 );
    }

    // Remote code execution
    {
        auto& remote = notepad.remote();
        remote.CreateRPCEnvironment( Worker_None, true );
        
        auto GetModuleHandleWPtr = notepad.modules().GetExport( L"kernel32.dll", "GetModuleHandleW" );
        if (GetModuleHandleWPtr)
        {
            // Direct execution in the new thread without stub
            [[maybe_unused]] DWORD mod = remote.ExecDirect( GetModuleHandleWPtr->procAddress, 0 );
        }

        // Execute in the new thread using stub
        if (auto asmPtr = AsmFactory::GetAssembler(); asmPtr && GetModuleHandleWPtr)
        {
            auto& a = *asmPtr;

            a.GenPrologue();
            a.GenCall( static_cast<uintptr_t>(GetModuleHandleWPtr->procAddress), { nullptr }, cc_stdcall );
            a.GenEpilogue();

            uint64_t result = 0;
            remote.ExecInNewThread( a->make(), a->getCodeSize(), result );
        }

        // Execute in main thread
        auto mainThread = notepad.threads().getMain();
        if (auto asmPtr = AsmFactory::GetAssembler(); asmPtr && mainThread && GetModuleHandleWPtr)
        {
            auto& a = *asmPtr;

            a.GenPrologue();
            a.GenCall( static_cast<uintptr_t>(GetModuleHandleWPtr->procAddress), { nullptr }, cc_stdcall );
            a.GenEpilogue();

            uint64_t result = 0;
            remote.ExecInAnyThread( a->make(), a->getCodeSize(), result, mainThread );
        }
    }

    // Pattern scanning
    if (Process process; NT_SUCCESS( process.Attach( GetCurrentProcessId() ) ))
    {
        PatternSearch ps{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

        std::vector<ptr_t> results;
        ps.SearchRemoteWhole( process, false, 0, results );
    }

    // Remote function calls
    {
        // Simple direct invocation
        if (auto pMessageBoxW = MakeRemoteFunction<decltype(&MessageBoxW)>( notepad, L"user32.dll", "MessageBoxW" ))
        {
            auto result = pMessageBoxW( HWND_DESKTOP, L"Hello world!", L"Title", MB_OKCANCEL );
            if (*result == IDCANCEL)
            {
            }
        }

        // Call in specific thread
        auto mainThread = notepad.threads().getMain();
        if (auto pIsGUIThread = MakeRemoteFunction<decltype(&IsGUIThread)>( notepad, L"user32.dll", "IsGUIThread" ); pIsGUIThread && mainThread)
        {
            auto result = pIsGUIThread.Call( { FALSE }, mainThread );
            if (*result)
            {
            }
        }

        // Complex args
        if (auto pMultiByteToWideChar = MakeRemoteFunction<decltype(&MultiByteToWideChar)>( notepad, L"kernel32.dll", "MultiByteToWideChar" ))
        {
            auto args = pMultiByteToWideChar.MakeArguments( { CP_ACP, 0, "Sample text", -1, nullptr, 0 } );
            std::wstring converted( 32, L'\0' );

            // Set buffer pointer and size manually
            args.set( 4, AsmVariant( converted.data(), converted.size() * sizeof( wchar_t ) ) );
            args.set( 5, converted.size() );

            auto length = pMultiByteToWideChar.Call( args );
            if (length)
                converted.resize( *length - 1 );
        }
    }

    // Direct syscalls, currently works for x64 only
    {
        uint8_t buf[32] = { };
        uintptr_t bytes = 0;

        NTSTATUS status = syscall::nt_syscall(
            syscall::get_index( "NtReadVirtualMemory" ),
            GetCurrentProcess(),
            GetModuleHandle( nullptr ),
            buf,
            sizeof(buf),
            &bytes
        );

        if (NT_SUCCESS( status ))
        {
        }
    }

    notepad.Terminate();

    // Manual mapping. See following functions for more info
    MapCalcFromFile();
    MapCmdFromMem();

    return 0;
}