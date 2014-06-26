#include "Tests.h"
#include "../BlackBoneDrv/BlackBoneDef.h"

void TestRemoteMem()
{
    Process proc;
    std::vector<DWORD> procIDs;
    NTSTATUS status = STATUS_SUCCESS;

    std::wcout << L"Remote memory mapping test\r\n";

    Process::EnumByName( L"explorer.exe", procIDs );

    if (procIDs.empty())
    {
        std::cout << "TestRemoteMem: Can't find any explorer.exe process for tests\r\n\r\n";
        return;
    }

    proc.Attach( procIDs.front() );
    
    status = proc.memory().Map( false );
    if (!NT_SUCCESS( status ))
    {
        std::cout << "TestRemoteMem: Process address space mapping failed with status 0x" << std::hex << status << "\r\n\r\n";
        return;
    }
    
    // Translate main module base address
    auto addr = proc.modules().GetMainModule()->baseAddress;
    auto translated = proc.memory().TranslateAddress( addr );

    std::cout << "TestRemoteMem: Translated " << std::hex << addr << " --> " << translated << "\r\n";

    status |= proc.memory().SetupHook( RemoteMemory::MemVirtualAlloc );
    status |= proc.memory().SetupHook( RemoteMemory::MemVirtualFree );
    status |= proc.memory().SetupHook( RemoteMemory::MemMapSection );
    status |= proc.memory().SetupHook( RemoteMemory::MemUnmapSection );

    if (!NT_SUCCESS( status ))
        std::cout << "TestRemoteMem: Failed to install one of the memory hooks\r\n";
    else
    {
        std::cout << "TestRemoteMem: Hooks installed, pausing for 10s\r\n";
        Sleep( 10000 );
    }

    std::cout << "\r\n";
}