#include "Tests.h"

/*
*/
void TestPEB( Process& proc )
{
    _PEB32 peb32 = { { { 0 } } };
    _PEB64 peb64 = { { { 0 } } };

    std::wcout << L"PEB info test\n";

    auto ppeb32 = proc.core().peb32( &peb32 );
    std::wcout << L"PEB32 address 0x" << std::hex << ppeb32 
               << L". Ldr address 0x" << peb32.Ldr << std::endl;

    auto ppeb64 = proc.core().peb64( &peb64 );
    std::wcout << L"PEB64 address 0x" << std::hex << ppeb64 
               << L". Ldr address 0x" << peb64.Ldr << std::endl << std::endl;

    std::wcout << std::dec;
}

/*
*/
void TestTEB( Process& proc )
{
    _TEB32 teb32 = { { 0 } };
    _TEB64 teb64 = { { 0 } };

    std::wcout << L"TEB info test\n";

    auto pteb32 = proc.threads().getMain()->teb( &teb32 );
    std::wcout << L"TEB32 address 0x" << std::hex << pteb32 << L". Thread ID 0x" 
               << teb32.ClientId.UniqueThread << std::endl;

    auto pteb64 = proc.threads().getMain()->teb( &teb64 );
    std::wcout << L"TEB64 address 0x" << std::hex << pteb64 << L". Thread ID 0x" 
               << teb64.ClientId.UniqueThread << std::endl << std::endl;

    std::wcout << std::dec;
}

int main( int /*argc*/, char* /*argv*/[] )
{
    Process proc;
    proc.Attach( GetCurrentProcessId() );

    TestPEB( proc );
    TestTEB( proc );
    TestMultiPtr();
    TestPatterns();
    TestLocalHook();
    TestRemoteCall();
    TestRemoteHook();
    TestDriver();
    TestRemoteMem();
    TestMMap();
    
    return 0;
}