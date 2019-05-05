#include "Common.h"
#include <set>

namespace Testing
{
    TEST_CLASS( Guard )
    {
    public:
        TEST_METHOD_INITIALIZE( ClassInitialize )
        {
            _closed.clear();
        }

        TEST_METHOD( ValidHandle )
        {
            {
                auto h = test_handle( testValue );
                AssertEx::IsTrue( h.valid() );
                AssertEx::IsTrue( bool( h ) );
                AssertEx::AreEqual( testValue, h.get() );
                AssertEx::IsTrue( h == testValue );
                AssertEx::IsTrue( h < testValue + 1 );
            }

            AssertEx::IsTrue( was_closed( testValue ) );
        }

        TEST_METHOD( InvalidHandle )
        {
            {
                auto h = test_handle( -testValue );
                AssertEx::IsFalse( h.valid() );
                AssertEx::IsFalse( bool( h ) );
                AssertEx::AreEqual( -testValue, h.get() );
            }

            AssertEx::IsTrue( nothing_closed() );
        }

        TEST_METHOD( Release )
        {
            {
                auto h = test_handle( testValue );
                const auto v = h.release();
                AssertEx::AreEqual( testValue, v );
            }

            AssertEx::IsTrue( nothing_closed() );
        }

        TEST_METHOD( Reset )
        {
            {
                auto h = test_handle( 0 );

                h.reset( testValue );
                AssertEx::IsTrue( nothing_closed() );

                h.reset( testValue );
                AssertEx::IsTrue( nothing_closed() );

                h.reset( testValue + 5 );
            }

            AssertEx::IsTrue( was_closed( testValue + 5 ) );
        }

        TEST_METHOD( Assign )
        {
            {
                auto h = test_handle( testValue );
                h = testValue + 5;
                AssertEx::IsTrue( was_closed( testValue ) );
            }

            AssertEx::IsTrue( was_closed( testValue + 5 ) );
        }

        TEST_METHOD( Move )
        {
            {
                auto h1 = test_handle( testValue );
                auto h2 = test_handle( testValue + 5 );

                h1 = std::move( h1 );
                AssertEx::IsTrue( nothing_closed() );

                h1 = std::move( h2 );
            }

            AssertEx::IsTrue( was_closed( testValue ) );
            AssertEx::IsTrue( was_closed( testValue + 5 ) );
        }

        TEST_METHOD( GetAddress )
        {
            {
                auto h = test_handle( testValue );
                []( int* target, int value ) { *target = value; }( &h, testValue + 5 );
            }

            AssertEx::IsFalse( was_closed( testValue ) );
            AssertEx::IsTrue( was_closed( testValue + 5 ) );
        }

        TEST_METHOD( CurrentProcess )
        {
            {
                test_handle_process h;
                AssertEx::IsFalse( bool( h ) );
                AssertEx::IsFalse( h.valid() );

                h = static_cast<int>(reinterpret_cast<intptr_t>(GetCurrentProcess()));
                AssertEx::IsTrue( bool( h ) );
                AssertEx::IsTrue( h.valid() );
            }

            AssertEx::IsTrue( nothing_closed() );
        }

        TEST_METHOD( ValidProcess )
        {
            {
                auto h = test_handle_process( testValue );
                AssertEx::IsTrue( bool( h ) );
                AssertEx::IsTrue( h.valid() );
            }

            AssertEx::IsTrue( was_closed( testValue ) );
        }

        TEST_METHOD( InvalidProcess )
        {
            {
                auto h = test_handle_process( -testValue );
                AssertEx::IsFalse( bool( h ) );
                AssertEx::IsFalse( h.valid() );
            }

            AssertEx::IsTrue( nothing_closed() );
        }  

        TEST_METHOD( InvalidFile )
        {
            {
                auto h = HandleGuard<int, &Guard::close>( static_cast<int>(reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE)) );
                AssertEx::IsFalse( bool( h ) );
                AssertEx::IsFalse( h.valid() );
            }

            AssertEx::IsTrue( nothing_closed() );
        }     

    private:
        static void close( int value )
        {
            _closed.emplace( value );
        }

        static bool was_closed( int value )
        {
            return _closed.find( value ) != _closed.end();
        }

        static bool nothing_closed()
        {
            return _closed.empty();
        }

    private:
        using test_handle = HandleGuard<int, &Guard::close>;
        using test_handle_process = HandleGuard<int, &Guard::close, with_pseudo<non_negative>::type>;

        static constexpr int testValue = 10;
        static inline std::set<int> _closed;
    };
}