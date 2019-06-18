#include "Common.h"

namespace Testing
{
    TEST_CLASS( PebTeb )
    {
    public:
        TEST_METHOD_INITIALIZE( ClassInitialize )
        {
            _proc32.CreateAndAttach( GetTestHelperHost32() );
            _proc64.CreateAndAttach( GetTestHelperHost64() );
            Sleep( 500 );
        }

        TEST_METHOD( Peb32 )
        {
            _PEB32 peb32 = { };
            auto ppeb32 = _proc32.core().peb32( &peb32 );

            AssertEx::IsNotZero( ppeb32 );
            AssertEx::IsNotZero( peb32.Ldr );
        }

        TEST_METHOD( NoPeb32 )
        {
            try
            {
                auto ppeb32 = _proc64.core().peb32();
            }
            catch (const nt_exception& e)
            {
                AssertEx::AreEqual( e.status(), STATUS_NOT_SUPPORTED );
            }
        }

        TEST_METHOD( Peb64 )
        {
            _PEB64 peb64 = { };
            auto ppeb64 = _proc64.core().peb64( &peb64 );

            AssertEx::IsNotZero( ppeb64 );
            AssertEx::IsNotZero( peb64.Ldr );
        }

        TEST_METHOD( Teb32 )
        {
            _TEB32 teb32 = { };
            auto pteb32 = _proc32.threads().getMain()->teb32( &teb32 );

            AssertEx::IsNotZero( pteb32 );
            AssertEx::IsNotZero( teb32.ClientId.UniqueThread );
        }

        TEST_METHOD( NoTeb32 )
        {
            try
            {
                auto pteb32 = _proc64.threads().getMain()->teb32();
            }
            catch (const nt_exception& e)
            {
                AssertEx::AreEqual( e.status(), STATUS_NOT_SUPPORTED );
            }
        }

        TEST_METHOD( Teb64 )
        {
            _TEB64 teb64 = { };
            auto pteb64 = _proc64.threads().getMain()->teb64( &teb64 );

            AssertEx::IsNotZero( pteb64 );
            AssertEx::IsNotZero( teb64.ClientId.UniqueThread );
        }

    private:
        Process _proc32, _proc64;
    };
}