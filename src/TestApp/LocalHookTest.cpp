#include "Tests.h"
#include "../BlackBone/localHook/VTableHook.hpp"

class TestClass
{
public:
    int Test( int a, int b, int c )
    {
        std::cout << "TestClass::Test called. Class data = " << junk << "\r\n";
        return a + b + c - junk;
    }

    virtual int Vfunc( int a, int b )
    {
        std::cout << "TestClass::Vfunc called. Class data = " << junk << "\r\n";
        return a + b - junk;
    }

public:
    int junk = 32;
};

class TestClassChild: public TestClass
{
public:
    int Test( int a, int b, int c )
    {
        std::cout << "TestClassChild::Test called. Class data = " << junk << "\r\n";     
        return TestClass::Test( a, b, c );
    }

    virtual int Vfunc( int a, int b )
    {
        std::cout << "TestClassChild::Vfunc called. Class data = " << junk << "\r\n";
        return 2 * (a + b) - junk;
    }

public:
    int junk = 64;
};

class MyMook
{
public:
    BOOL hkCloseHandle( HANDLE& hObj )
    {
        std::cout << "MyMook::hkCloseHandle called. Handle " << std::hex << hObj << std::dec << "\r\n";
        return TRUE;
    }

    int hkTest( void*& _this, int&, int&, int& )
    {
        std::cout << "MyMook::hkTest called. Target class data = " << ((TestClassChild*)_this)->junk << "\r\n";

        ((TestClass*)_this)->junk = 32;
        return 0;
    }

    int hkVFunc( void*& _this, int&, int& )
    {
        std::cout << "MyMook::hkVFunc called. Target class data = " << ((TestClassChild*)_this)->junk << "\r\n";

        ((TestClassChild*)_this)->junk = 48;
        return 0;
    }

private:
    int junk = 16;
};

void __fastcall TestFastcall( int& a1, float )
{
    std::cout << "TestFastcall called. Argument = " << a1 << "\r\n";

    a1 = a1 * 3;
}

void __fastcall hkTestFastcall( int& a1, float& )
{
    std::cout << "hkTestFastcall called. Argument = " << a1 << "\r\n";

    a1 /= 2;
}

int __stdcall hkTest( void*& _this, int& a1, int&, int& )
{
    std::cout << "hkTest called. Class data = " << ((TestClass*)_this)->junk << "\r\n";

    ((TestClass*)_this)->junk = 16;
    a1 /= 2;

    return 5;
}

void TestLocalHook()
{
#ifdef COMPILER_MSVC
    MyMook mh;
    TestClassChild ts;
    Process proc;

    auto ptr = brutal_cast<int( __thiscall* )(void*, int, int, int)>(&TestClass::Test);

    Detour<decltype(&CloseHandle), MyMook> det;
    Detour<decltype(&TestFastcall)> det2;
    Detour<decltype(ptr)> det3;
    VTableDetour<int( __thiscall* )(void*, int, int), MyMook> det4;

    std::cout << "Free function into class member HWBP hook test\r\n";

    det.Hook( &CloseHandle, &MyMook::hkCloseHandle, &mh, HookType::HWBP );
    det2.Hook( &TestFastcall, &hkTestFastcall, HookType::Inline);
    det3.Hook( ptr, &::hkTest, HookType::Int3 );
    det4.Hook( (void**)&ts, 0, &MyMook::hkVFunc, &mh );

    int a = 10;

    std::cout << "__fastcall with reference arg Inline hook test. Argument = " << a << "\r\n";
    TestFastcall( a, 5.5f );
    std::cout << "Argument = " << a << "\r\n";

    std::cout << "Class member into free function Int3 hook test:\r\n";
    int val = ts.Test( 0x10, 0x20, 0x30 );
    std::cout << "Returned value = " << val << "\r\n";

    std::cout << "VTable with copy hook test:\r\n";
    val = ((TestClass*)&ts)->Vfunc( 0x10, 0x20 );
    std::cout << "Returned value = " <<  val << "\r\n";

    proc.Attach( GetCurrentProcessId() );
#endif
}