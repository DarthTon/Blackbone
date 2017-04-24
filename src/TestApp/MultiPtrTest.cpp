#define CATCH_CONFIG_FAST_COMPILE
#include "Tests.h"
#include "../BlackBone/Process/MultPtr.hpp"
#include "../BlackBone/Asm/LDasm.h"

constexpr intptr_t off[] = { 0x10, 0x20, 0x30 };

struct s_end
{
    int ival = 74;
    float fval = 12.0f;

    virtual int fn1() { return 1; }
    virtual int fn2() { return 2; }
    virtual int fn3() { return ival + static_cast<int>(fval); }
};

struct s1
{
    uint8_t pad[off[2]];
    s_end* pEnd = new s_end();
};

struct s2
{
    uint8_t pad[off[1]];
    s1* pS1 = new s1();
};

struct s3
{
    uint8_t pad[off[0]];
    s2* pS2 = new s2();
};

// stupid C3865 : "'__thiscall' : can only be used on native member functions", even for a type declaration
typedef int( __fastcall* pfnClass )(s_end* _this, void* zdx);

//void TestMultiPtr()
TEST_CASE( "03. Multi-level pointer" )
{
    std::unique_ptr<s3> pS3( new s3() );
    REQUIRE( pS3 != nullptr );

    Process proc;
    REQUIRE_NT_SUCCESS( proc.Attach( GetCurrentProcessId() ) );

    auto orig = pS3->pS2->pS1->pEnd->fval;
    REQUIRE( orig == Approx( 12.0f ) );
    REQUIRE( pS3->pS2->pS1->pEnd->fn3() == (pS3->pS2->pS1->pEnd->ival + static_cast<int>(pS3->pS2->pS1->pEnd->fval)) );

    SECTION( "Local pointers" )
    {
        std::wcout << L"Local multi-level pointers" << std::endl;

        auto ppS3 = pS3.get();
        uintptr_t ps3 = (uintptr_t)&ppS3;

        multi_ptr<s_end*> struct_ptr( ps3, { off[0], off[1], off[2] } );
        multi_ptr<pfnClass> func_ptr( ps3, { off[0], off[1], off[2], 0, 2 * sizeof( pfnClass ) } );
        multi_ptr<float> float_ptr( ps3, { off[0], off[1], off[2], FIELD_OFFSET( s_end, fval ) } );

        s_end* pEnd = struct_ptr;

        CHECK( pEnd == pS3->pS2->pS1->pEnd );
        WHEN( "Float value changed" )
        {
            const float newVal = 15.7f;
            *float_ptr = 15.7f;
            CHECK( pEnd->fval == Approx( newVal ) );
        }

        CHECK( func_ptr( pEnd, nullptr ) == (pEnd->ival + static_cast<int>(pEnd->fval)) );
    }
  
    SECTION( "Remote pointers" )
    {
        std::wcout << L"Remote multi-level pointer" << std::endl;
        auto ppS3 = pS3.get();

        const float newVal = 25.0f;

        multi_ptr_ex<s_end*> ptr_ex( &proc, (uintptr_t)&ppS3, { off[0], off[1], off[2] } );
        auto pVal_ex = ptr_ex.get();
        REQUIRE( pVal_ex != nullptr );
        REQUIRE( pVal_ex->fval != 0 );

        pVal_ex->fval = 25.0;
        CHECK( ptr_ex.commit() == STATUS_SUCCESS );
        pVal_ex = ptr_ex.get();

        REQUIRE( pVal_ex != nullptr );
        CHECK( pVal_ex->fval == Approx( newVal ) );
    }
}
