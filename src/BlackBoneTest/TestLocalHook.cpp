#include "Common.h"

namespace Testing
{
    class TestClassBase
    {
    public:
        int Test( int a, int b, int c )
        {
            return a + b + c - junk;
        }

        virtual int Vfunc( int a, int b )
        {
            return a + b - junk;
        }

    public:
        int junk = 32;
    };

    class TestClassChild : public TestClassBase
    {
    public:
        int __declspec(noinline) Test( int a, int b, int c )
        {
            return TestClassBase::Test( a, b, c );
        }

        virtual int Vfunc( int a, int b )
        {
            return 2 * (a + b) - junk;
        }

    public:
        int junk = 64;
    };

    class MyMook
    {
    public:
        BOOL hkCloseHandle( HANDLE& /*hObj*/ )
        {
            closedHandels++;
            return TRUE;
        }

        int hkTest( void*& _this, int&, int&, int& )
        {
            reinterpret_cast<TestClassBase*>(_this)->junk = 52;
            return 0;
        }

        int hkVFunc( void*& _this, int&, int& )
        {
            reinterpret_cast<TestClassChild*>(_this)->junk = 48;
            return 0;
        }

        inline int handles() const { return closedHandels; }

    private:
        int junk = 16;
        int closedHandels = 0;
    };

    void __declspec(noinline) __fastcall TestFastcall( int& a1, float )
    {
        a1 = a1 * 3;
    }

    void __fastcall hkTestFastcall( int& a1, float& )
    {
        a1 /= 2;
    }

    int __stdcall hkTest( void*& _this, int& a1, int&, int& )
    {
        reinterpret_cast<TestClassBase*>(_this)->junk = 72;
        a1 /= 2;

        return 5;
    }

    TEST_CLASS( LocalHook )
    {
#if !defined(USE64) || defined(DEBUG)
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE( L"Ignore", L"true" )
    END_TEST_CLASS_ATTRIBUTE()
#endif
    public:
        TEST_METHOD_INITIALIZE( ClassInitialize )
        {
            // Validate preconditions
            AssertEx::AreEqual( 64, testClass.junk );
            AssertEx::AreEqual( 32, pTCBase->junk );
            AssertEx::AreEqual( args[0] + args[1] + args[2] - pTCBase->junk, testClass.Test( args[0], args[1], args[2] ));
            AssertEx::AreEqual( 2 * (args[0] + args[1]) - testClass.junk, testClass.Vfunc( args[0], args[1] ) );
        }

        TEST_METHOD( HookHWBP )
        {
            MyMook target;
            Detour<decltype(&CloseHandle), MyMook> hook;

            AssertEx::IsTrue( hook.Hook( &CloseHandle, &MyMook::hkCloseHandle, &target, HookType::HWBP ) );

            Process proc;
            proc.Attach( GetCurrentProcessId() );
            proc.Detach();

            AssertEx::IsTrue( target.handles() > 0 );
        }

        TEST_METHOD( HookInline )
        {
            MyMook target;
            Detour<decltype(&TestFastcall)> hook;

            AssertEx::IsTrue( hook.Hook( &TestFastcall, &hkTestFastcall, HookType::Inline ) );

            int a = args[0];
            TestFastcall( a, 5.5f );

            AssertEx::AreEqual( (args[0] / 2) * 3, a );
        }

        TEST_METHOD( HookInt3 )
        {
            auto ptr = brutal_cast<int( __thiscall* )(void*, int, int, int)>(&TestClassBase::Test);
            Detour<decltype(ptr)> hook;

            AssertEx::IsTrue( hook.Hook( ptr, &hkTest, HookType::Int3 ) );

            auto val = testClass.Test( args[0], args[1], args[2] );

            AssertEx::AreEqual( 72, pTCBase->junk );
            AssertEx::AreEqual( args[0] / 2 + args[1] + args[2] - pTCBase->junk, val );
        }

        TEST_METHOD( HookVtable )
        {
            MyMook target;
            VTableDetour<int( __thiscall* )(void*, int, int), MyMook> hook;

            AssertEx::IsTrue( hook.Hook( reinterpret_cast<void**>(&testClass), 0, &MyMook::hkVFunc, &target ) );

            auto val = pTCBase->Vfunc( args[0], args[1] );

            AssertEx::AreEqual( 48, testClass.junk );
            AssertEx::AreEqual( 2 * (args[0] + args[1]) - testClass.junk, val );
        }

    private:
        TestClassChild testClass;
        TestClassBase* pTCBase = &testClass;
        const int args[3] = { 0x10, 0x20, 0x30 };
    };
}