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

s3* pS3 = new s3();

// stupid C3865 : "'__thiscall' : can only be used on native member functions", even for a type declaration
typedef int( __fastcall* pfnClass )(s_end* _this, void* zdx);

void TestMultiPtr()
{
     Process proc;
    proc.Attach( GetCurrentProcessId() );

    std::wcout << L"Local and remote multi-level pointers test" << std::endl;

    uintptr_t ps3 = (uintptr_t)&pS3;
    auto orig = pS3->pS2->pS1->pEnd->fval;

    multi_ptr<s_end*> struct_ptr( ps3, { off[0], off[1], off[2] } );
    multi_ptr<pfnClass> func_ptr( ps3, { off[0], off[1], off[2], 0, 2 * sizeof( pfnClass ) } );
    multi_ptr<float> float_ptr( ps3, { off[0], off[1], off[2], FIELD_OFFSET( s_end, fval ) } );

    s_end* pEnd = struct_ptr;
    *float_ptr = 15.7f;

    std::wcout << L"multi_ptr: s_end : " << (pEnd == pS3->pS2->pS1->pEnd) << std::endl;
    std::wcout << L"multi_ptr: s_end::fn3 result: " << (func_ptr( pEnd, nullptr )) << std::endl;
    std::wcout << L"multi_ptr: changed val : " << orig << L" -> " << pS3->pS2->pS1->pEnd->fval << std::endl;

    multi_ptr_ex<s_end*> ptr_ex( &proc, ps3, { off[0], off[1], off[2] } );
    if (auto pVal_ex = ptr_ex.get())
    {
        orig = pVal_ex->fval;
        pVal_ex->fval = 25.0;
        ptr_ex.commit();
        pVal_ex = ptr_ex.get();
        std::wcout << L"multi_ptr_ex: changed val : " << orig << L" -> " << pVal_ex->fval << std::endl;
    }
    else
        std::wcout << L"multi_ptr_ex failed" << std::endl;

    std::wcout << std::endl;
}
