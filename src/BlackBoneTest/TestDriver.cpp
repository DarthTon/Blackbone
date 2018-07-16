#include "Common.h"

namespace Testing
{
    TEST_CLASS( DriverTest )
    {
#ifdef CI_BUILD
#define CHECK_AND_SKIP
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE( L"Ignore", L"true" )
    END_TEST_CLASS_ATTRIBUTE()
#else
#define CHECK_AND_SKIP if(_mustSkip) return
#endif

    public:
        TEST_METHOD_INITIALIZE( ClassInitialize )
        {
            AssertEx::NtSuccess( _explorer.Attach( L"explorer.exe" ) );
            NTSTATUS status = Driver().EnsureLoaded();
            if (!NT_SUCCESS( status ))
            {
                _mustSkip = status == STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }

        TEST_METHOD( DEP )
        {
            CHECK_AND_SKIP;

            // Disable DEP, x86 only;
            if (_explorer.barrier().targetWow64)
            {
                DWORD depFlags = 0;
                BOOL perm = FALSE;

                NTSTATUS status = Driver().DisableDEP( GetCurrentProcessId() );
                SAFE_CALL( GetProcessDEPPolicy, GetCurrentProcess(), &depFlags, &perm );

                AssertEx::NtSuccess( status );
                AssertEx::IsZero( depFlags & PROCESS_DEP_ENABLE );
            }
        }

        TEST_METHOD( ProtectProcess )
        {
            CHECK_AND_SKIP;

            Process thisProc;
            AssertEx::NtSuccess( thisProc.Attach( GetCurrentProcessId() ) );

            // Make current process protected
            AssertEx::NtSuccess( Driver().ProtectProcess( GetCurrentProcessId(), Policy_Enable ) );
            AssertEx::IsTrue( thisProc.core().isProtected() );

            Driver().ProtectProcess( GetCurrentProcessId(), Policy_Disable );
        }

        TEST_METHOD( PromoteHandle )
        {
            CHECK_AND_SKIP;

            // Grant explorer.exe handle full access
            NTSTATUS status = Driver().PromoteHandle( GetCurrentProcessId(), _explorer.core().handle(), PROCESS_ALL_ACCESS );
            AssertEx::NtSuccess( status );

            PUBLIC_OBJECT_BASIC_INFORMATION info = { };
            ULONG sz = 0;
            status = SAFE_CALL( NtQueryObject, _explorer.core().handle(), ObjectBasicInformation, &info, static_cast<ULONG>(sizeof( info )), &sz );

            AssertEx::NtSuccess( status );
            AssertEx::AreEqual( ACCESS_MASK( PROCESS_ALL_ACCESS ), info.GrantedAccess );
        }

        TEST_METHOD( ReadMemory )
        {
            CHECK_AND_SKIP;

            uint8_t buf[0x1000] = { };

            // Read explorer.exe PE header
            auto address = _explorer.modules().GetMainModule()->baseAddress;
            ptr_t size = 0x1000;

            AssertEx::NtSuccess( Driver().ReadMem( _explorer.pid(), address, size, buf ) );
            AssertEx::AreEqual( static_cast<short>(IMAGE_DOS_SIGNATURE), *reinterpret_cast<short*>(buf) );
        }

        TEST_METHOD( AllocateMemory )
        {
            CHECK_AND_SKIP;

            ptr_t size = 0x1000;
            auto address = _explorer.modules().GetMainModule()->baseAddress;

            // Allocate some memory
            ptr_t address2 = 0;
            AssertEx::NtSuccess( Driver().AllocateMem( _explorer.pid(), address2, size, MEM_COMMIT, PAGE_READWRITE ) );

            Driver().FreeMem( _explorer.pid(), address2, size, MEM_RELEASE );
        }

        TEST_METHOD( ProtectMemory )
        {
            CHECK_AND_SKIP;

            MEMORY_BASIC_INFORMATION64 memInfo = { };
            auto address = _explorer.modules().GetMainModule()->baseAddress;

            // Make explorer.exe PE header writable
            NTSTATUS status = Driver().ProtectMem( _explorer.pid(), address, 0x1000, PAGE_READWRITE );
            _explorer.memory().Query( address, &memInfo );

            AssertEx::NtSuccess( status );
            AssertEx::IsNotZero( memInfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY) );

            _explorer.memory().Protect( address, 0x1000, PAGE_READONLY );
        }

        TEST_METHOD( UnlinkHandleTable )
        {
            CHECK_AND_SKIP;

            // Unlink handle table
            //CHECK_NT_SUCCESS( status = Driver().UnlinkHandleTable( proc.pid() ) );
        }

        TEST_METHOD( EnumRegions )
        {
            CHECK_AND_SKIP;

            // Enum regions
            std::vector<MEMORY_BASIC_INFORMATION64> info;

            AssertEx::NtSuccess( Driver().EnumMemoryRegions( _explorer.pid(), info ) );
            AssertEx::IsNotZero( info.size() );
        }

    private:
        Process _explorer;
        bool _mustSkip = false;
    };

}