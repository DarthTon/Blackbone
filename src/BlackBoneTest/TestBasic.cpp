#include "Common.h"

namespace Testing
{
    TEST_MODULE_INITIALIZE( ModuleInit )
    {
        InitializeOnce();
    }

    TEST_CLASS( Generic )
    {
    public:
        static DWORD CALLBACK VoidFn( void* )
        {
            return 0;
        }

        TEST_METHOD( InvalidHandles )
        {
            auto procBase = Process::CreateNew( GetTestHelperHost32() );
            auto proc1 = Process( procBase.pid(), PROCESS_ALL_ACCESS & ~PROCESS_CREATE_THREAD );

            try
            {
                auto thread = proc1.threads().CreateNew( reinterpret_cast<ptr_t>(&VoidFn), 0 );
                AssertEx::Fail( L"Must throw" );
            }
            catch (const nt_exception& e)
            {
                AssertEx::AreEqual( STATUS_ACCESS_DENIED, e.status() );
            }
            
            auto proc2 = Process( procBase.pid(), PROCESS_ALL_ACCESS & ~PROCESS_VM_READ );

            try
            {
                PEB_T peb = { };
                proc2.core().peb( &peb );
            }
            catch (const nt_exception& e)
            {
                AssertEx::AreEqual( STATUS_ACCESS_DENIED, e.status() );
            }

            procBase.Terminate();
        }
    };
}