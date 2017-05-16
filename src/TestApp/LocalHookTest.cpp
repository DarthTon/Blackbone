#define CATCH_CONFIG_FAST_COMPILE
#include "Tests.h"
#include "../BlackBone/localHook/VTableHook.hpp"

class TestClass
{
public:
    int Test( int a, int b, int c )
    {
        //std::cout << "TestClass::Test called. Class data = " << junk << "\r\n";
        return a + b + c - junk;
    }

    virtual int Vfunc( int a, int b )
    {
        //std::cout << "TestClass::Vfunc called. Class data = " << junk << "\r\n";
        return a + b - junk;
    }

public:
    int junk = 32;
};

class TestClassChild: public TestClass
{
public:
    int __declspec(noinline) Test( int a, int b, int c )
    {
        //std::cout << "TestClassChild::Test called. Class data = " << junk << "\r\n";     
        return TestClass::Test( a, b, c );
    }

    virtual int Vfunc( int a, int b )
    {
        //std::cout << "TestClassChild::Vfunc called. Class data = " << junk << "\r\n";
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
        //std::cout << "MyMook::hkCloseHandle called. Handle " << std::hex << hObj << std::dec << "\r\n";
        closedHandels++;
        return TRUE;
    }

    int hkTest( void*& _this, int&, int&, int& )
    {
        //std::cout << "MyMook::hkTest called. Target class data = " << ((TestClassChild*)_this)->junk << "\r\n";
        ((TestClass*)_this)->junk = 52;
        return 0;
    }

    int hkVFunc( void*& _this, int&, int& )
    {
        //std::cout << "MyMook::hkVFunc called. Target class data = " << ((TestClassChild*)_this)->junk << "\r\n";
        ((TestClassChild*)_this)->junk = 48;
        return 0;
    }

    inline int handles() const { return closedHandels; }

private:
    int junk = 16;
    int closedHandels = 0;
};

void __declspec(noinline) __fastcall TestFastcall( int& a1, float )
{
    //std::cout << "TestFastcall called. Argument = " << a1 << "\r\n";
    a1 = a1 * 3;
}

void __fastcall hkTestFastcall( int& a1, float& )
{
    //std::cout << "hkTestFastcall called. Argument = " << a1 << "\r\n";
    a1 /= 2;
}

int __stdcall hkTest( void*& _this, int& a1, int&, int& )
{
    //std::cout << "hkTest called. Class data = " << ((TestClass*)_this)->junk << "\r\n";
    ((TestClass*)_this)->junk = 72;
    a1 /= 2;

    return 5;
}

TEST_CASE( "06. Local hooks" )
{
#ifdef COMPILER_MSVC
    MyMook mh;
    TestClassChild testClass;
    TestClass* pTCBase = &testClass;
    const int args[] = { 0x10, 0x20, 0x30 };

    // x86: For some mysterious reason CHECH/REQUIRE/WARN breaks subsequent MapViewOfFile calls
    if (testClass.junk != 64)
    {
        std::cout << "Invalid initial state" << std::endl;
        return;
    }
    if (pTCBase->junk != 32)
    {
        std::cout << "Invalid initial state" << std::endl;
        return;
    }
    if (testClass.Test( args[0], args[1], args[2] ) != (args[0] + args[1] + args[2] - pTCBase->junk))
    {
        std::cout << "Invalid initial state" << std::endl;
        return;
    }
    if (testClass.Vfunc( args[0], args[1] ) != (2 * (args[0] + args[1]) - testClass.junk))
    {
        std::cout << "Invalid initial state" << std::endl;
        return;
    }

    auto ptr = brutal_cast<int( __thiscall* )(void*, int, int, int)>(&TestClass::Test);

    Detour<decltype(&CloseHandle), MyMook> det;
    Detour<decltype(&TestFastcall)> det2;
    Detour<decltype(ptr)> det3;
    VTableDetour<int( __thiscall* )(void*, int, int), MyMook> det4;

    REQUIRE( det.Hook( &CloseHandle, &MyMook::hkCloseHandle, &mh, HookType::HWBP ) );
    REQUIRE( det2.Hook( &TestFastcall, &hkTestFastcall, HookType::Inline ) );
    REQUIRE( det3.Hook( ptr, &::hkTest, HookType::Int3 ) );
    REQUIRE( det4.Hook( (void**)&testClass, 0, &MyMook::hkVFunc, &mh ) );

    int a = args[0];

    TestFastcall( a, 5.5f );
    CHECK( a == (args[0] / 2) * 3 );

    auto val = testClass.Test( args[0], args[1], args[2] );
    CHECK( pTCBase->junk == 72 );
    CHECK( val == (args[0] / 2 + args[1] + args[2] - pTCBase->junk) );

    val = pTCBase->Vfunc( args[0], args[1] );
    CHECK( testClass.junk == 48 );
    CHECK( val == (2 * (args[0] + args[1]) - testClass.junk) );

    CHECK( mh.handles() > 0 );
#endif
}