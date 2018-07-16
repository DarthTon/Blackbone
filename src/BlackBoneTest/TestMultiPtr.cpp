#include "Common.h"

namespace Testing
{
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

    TEST_CLASS( MultiPtr )
    {
    public:
        MultiPtr()
            : _object( new s3 )
            , _guard( _object )
            , _objectPtr( reinterpret_cast<uintptr_t>(&_object) )
        {
        }

        TEST_METHOD_INITIALIZE( ClassInitialize )
        {
            AssertEx::IsNotNull( _guard.get() );

            auto orig = _guard->pS2->pS1->pEnd->fval;
            AssertEx::AreEqual( 12.0f, orig, 0.001f );
            AssertEx::AreEqual( _guard->pS2->pS1->pEnd->ival + static_cast<int>(_guard->pS2->pS1->pEnd->fval), _guard->pS2->pS1->pEnd->fn3() );
        }

        TEST_METHOD( LocalStruct )
        {
            multi_ptr<s_end*> struct_ptr( _objectPtr, { off[0], off[1], off[2] } );
            s_end* pEnd = struct_ptr;

            AssertEx::AreEqual( reinterpret_cast<uintptr_t>(_guard->pS2->pS1->pEnd), reinterpret_cast<uintptr_t>(pEnd) );
        }

        TEST_METHOD( LocalFloat )
        {
            multi_ptr<s_end*> struct_ptr( _objectPtr, { off[0], off[1], off[2] } );
            multi_ptr<float> float_ptr( _objectPtr, { off[0], off[1], off[2], static_cast<intptr_t>(offsetOf( &s_end::fval )) } );

            s_end* pEnd = struct_ptr;

            const float newVal = 15.7f;
            *float_ptr = 15.7f;

            AssertEx::AreEqual( pEnd->fval, newVal, 0.001f );
        }

        TEST_METHOD( LocalMethod )
        {
            multi_ptr<s_end*> struct_ptr( _objectPtr, { off[0], off[1], off[2] } );
            multi_ptr<pfnClass> func_ptr( _objectPtr, { off[0], off[1], off[2], 0, 2 * sizeof( pfnClass ) } );

            s_end* pEnd = struct_ptr;

            AssertEx::AreEqual( pEnd->ival + static_cast<int>(pEnd->fval), func_ptr( pEnd, nullptr ) );
        }

        TEST_METHOD( Remote )
        {
            Process proc;
            AssertEx::NtSuccess( proc.Attach( GetCurrentProcessId() ) );

            const float newVal = 25.0f;

            multi_ptr_ex<s_end*> ptr_ex( &proc, _objectPtr, { off[0], off[1], off[2] } );
            auto pVal_ex = ptr_ex.get();
            AssertEx::IsNotNull( pVal_ex );
            AssertEx::IsNotZero( pVal_ex->fval );

            pVal_ex->fval = 25.0;
            AssertEx::AreEqual( STATUS_SUCCESS, ptr_ex.commit() );
            pVal_ex = ptr_ex.get();

            AssertEx::IsNotNull( pVal_ex );
            AssertEx::AreEqual( newVal, pVal_ex->fval, 0.001f );
        }

    private:
        s3 * _object;
        std::unique_ptr<s3> _guard;
        uintptr_t _objectPtr;
    };

}