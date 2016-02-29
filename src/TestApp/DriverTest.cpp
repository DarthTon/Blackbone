#include "Tests.h"
#include "../BlackBone/DriverControl/DriverControl.h"

void TestDriver()
{
    Process proc, thisProc;
    std::vector<DWORD> procIDs;
    Process::EnumByName( L"explorer.exe", procIDs );

    std::wcout << L"Driver functionality test\n";

    if (procIDs.empty())
    {
        std::cout << "TestDriver: Can't find any explorer.exe process for tests\r\n\r\n";
        return;
    }

    proc.Attach( procIDs.front() );
    thisProc.Attach( GetCurrentProcessId() );

    // Load driver
    if (NT_SUCCESS( Driver().EnsureLoaded() ))
    {
        NTSTATUS status = STATUS_SUCCESS;
        DWORD depFlags = 0;
        BOOL perm = FALSE;
        uint8_t buf[0x1000] = { 0 };
        MEMORY_BASIC_INFORMATION64 memInfo = { 0 };

        auto address = proc.modules().GetMainModule()->baseAddress;
        ptr_t address2 = 0;
        ptr_t size = 0x1000;

        // Disable DEP
        status = Driver().DisableDEP( GetCurrentProcessId() );
        SAFE_CALL( GetProcessDEPPolicy, GetCurrentProcess(), &depFlags, &perm );
        if (!NT_SUCCESS( status ) || depFlags & PROCESS_DEP_ENABLE)
            std::cout << "TestDriver: IOCTL_BLACKBONE_DISABLE_DEP failed, status 0x" << std::hex << status << "\r\n";
        else
            std::cout << "TestDriver: IOCTL_BLACKBONE_DISABLE_DEP succeeded\r\n";

        // Make current process protected
        status = Driver().ProtectProcess( GetCurrentProcessId(), true );
        if (!NT_SUCCESS( status ) || !thisProc.core().isProtected())
            std::cout << "TestDriver: IOCTL_BLACKBONE_SET_PROTECTION failed, status 0x" << std::hex << status << "\r\n";
        else
        {
            std::cout << "TestDriver: IOCTL_BLACKBONE_SET_PROTECTION succeeded\r\n";
            Driver().ProtectProcess( GetCurrentProcessId(), false );
        }

        // Grant explorer.exe handle full access
        status = Driver().PromoteHandle( GetCurrentProcessId(), proc.core().handle(), PROCESS_ALL_ACCESS );
        if (!NT_SUCCESS( status ))
        {
            std::cout << "TestDriver: IOCTL_BLACKBONE_GRANT_ACCESS failed, status 0x" << std::hex << status << "\r\n";
        }
        else
        {
            PUBLIC_OBJECT_BASIC_INFORMATION info = { 0 };
            ULONG sz = 0;
            status = SAFE_CALL( NtQueryObject, proc.core().handle(), ObjectBasicInformation, &info, (ULONG)sizeof( info ), &sz );
            if (!NT_SUCCESS( status ))
            {
                std::cout << "TestDriver: IOCTL_BLACKBONE_GRANT_ACCESS failed, status 0x" << std::hex << status << "\r\n";
            }
            else
            {
                if (info.GrantedAccess == PROCESS_ALL_ACCESS)
                    std::cout << "TestDriver: IOCTL_BLACKBONE_GRANT_ACCESS succeeded\r\n";
                else
                    std::cout << "TestDriver: IOCTL_BLACKBONE_GRANT_ACCESS failed, status 0x" << std::hex << STATUS_UNSUCCESSFUL << "\r\n";
            }
        }

        // Read explorer.exe PE header
        status = Driver().ReadMem( proc.pid(), address, size, buf );
        if (!NT_SUCCESS( status ) || *(uint16_t*)buf != IMAGE_DOS_SIGNATURE)
            std::cout << "TestDriver: IOCTL_BLACKBONE_COPY_MEMORY failed, status 0x" << std::hex << status << "\r\n";
        else
            std::cout << "TestDriver: IOCTL_BLACKBONE_COPY_MEMORY succeeded\r\n";

        // Allocate some memory
        status = Driver().AllocateMem( proc.pid(), address2, size, MEM_COMMIT, PAGE_READWRITE );
        if (!NT_SUCCESS( status ) || *(uint16_t*)buf != IMAGE_DOS_SIGNATURE)
            std::cout << "TestDriver: IOCTL_BLACKBONE_ALLOCATE_FREE_MEMORY failed, status 0x" << std::hex << status << "\r\n";
        else
        {
            std::cout << "TestDriver: IOCTL_BLACKBONE_ALLOCATE_FREE_MEMORY succeeded\r\n";
            Driver().FreeMem( proc.pid(), address2, size, MEM_RELEASE );
        }

        // Make explorer.exe PE header writable
        status = Driver().ProtectMem( proc.pid(), address, 0x1000, PAGE_READWRITE );
        proc.memory().Query( address, &memInfo );

        if (!NT_SUCCESS( status ) || !(memInfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY)))
            std::cout << "TestDriver: IOCTL_BLACKBONE_PROTECT_MEMORY failed, status 0x" << std::hex << status << "\r\n";
        else
        {
            std::cout << "TestDriver: IOCTL_BLACKBONE_PROTECT_MEMORY succeeded\r\n";
            proc.memory().Protect( address, 0x1000, PAGE_READONLY );
        }

        // Unlink handle table
        status = Driver().UnlinkHandleTable( proc.pid() );
        if (!NT_SUCCESS( status ))
            std::cout << "TestDriver: IOCTL_BLACKBONE_UNLINK_HTABLE failed, status 0x" << std::hex << status << "\r\n";
        else
            std::cout << "TestDriver: IOCTL_BLACKBONE_UNLINK_HTABLE succeeded\r\n";

        // Enum regions
        std::vector<MEMORY_BASIC_INFORMATION64> info;
        status = Driver().EnumMemoryRegions( proc.pid(), info );
        if (!NT_SUCCESS( status ))
            std::cout << "TestDriver: IOCTL_BLACKBONE_ENUM_REGIONS failed, status 0x" << std::hex << status << "\r\n";
        else
            std::cout << "TestDriver: IOCTL_BLACKBONE_ENUM_REGIONS succeeded\r\n";
    }
    else
        std::cout << "TestDriver: Failed to load driver. Status 0x" << std::hex << LastNtStatus() << "\r\n";

    std::cout << "\r\n";
}