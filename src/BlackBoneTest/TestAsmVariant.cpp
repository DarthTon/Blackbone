#include "Common.h"


namespace Testing
{
    struct IntStruct
    {
        int16_t ival = 10;
    };

    struct PairStruct
    {
        int64_t ival = 20;
        float fval = 24.0f;
    };

    TEST_CLASS( AsmVariantTest )
    {
    public:
        TEST_CLASS_INITIALIZE( Init )
        {
            Logger::WriteMessage( "" );
        }

        TEST_METHOD( IntRvalue )
        {
            AsmVariant a( 1 );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( sizeof( 1 ), a.size );
            Assert::AreEqual( 1ull, a.imm_val64 );
        }

        TEST_METHOD( IntLvalue )
        {
            const int int_val = 2;
            AsmVariant a( int_val );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( sizeof( int_val ), a.size );
            Assert::AreEqual( static_cast<uint64_t>(int_val), a.imm_val64 );
        }

        TEST_METHOD( EnumChar )
        {
            enum e1 : char
            {
                e1_val0 = 3,
            };

            AsmVariant a( e1_val0 );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( sizeof( e1_val0 ), a.size );
            Assert::AreEqual( uint64_t( e1_val0 ), a.imm_val64 );
        }

        TEST_METHOD( EnumUshort )
        {
            enum class ec : unsigned short
            {
                ec_val0 = 4,
            };

            AsmVariant a( ec::ec_val0 );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( sizeof( ec::ec_val0 ), a.size );
            Assert::AreEqual( static_cast<uint64_t>(ec::ec_val0), a.imm_val64 );
        }

        TEST_METHOD( Float )
        {
            AsmVariant a( 2.0f );

            Assert::AreEqual( AsmVariant::imm_float, a.type );
            Assert::AreEqual( sizeof( float ), a.size );
            Assert::AreEqual( 2.0f, a.imm_float_val );
        }

        TEST_METHOD( Double )
        {
            AsmVariant a( 3.0 );

            Assert::AreEqual( AsmVariant::imm_double, a.type );
            Assert::AreEqual( sizeof( double ), a.size );
            Assert::AreEqual( 3.0, a.imm_double_val );
        }

        TEST_METHOD( StringInplace )
        {
            AsmVariant a( "Text" );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
        }

        TEST_METHOD( StringConstPtr )
        {
            const char* cstring = "Text";
            AsmVariant a( cstring );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(cstring), a.imm_val64 );
        }

        TEST_METHOD( StringArray )
        {
            const char str_array[] = "Text";
            AsmVariant a( str_array );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(str_array), a.imm_val64 );
            Assert::AreEqual( sizeof( str_array ), a.size );
        }

        TEST_METHOD( StringPtr )
        {
            const char str_array[] = "Text";
            char* str_ptr = const_cast<char*>(str_array);
            AsmVariant a( str_ptr );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(str_ptr), a.imm_val64 );
            Assert::AreEqual( sizeof( str_array ), a.size );
        }

        TEST_METHOD( WStringInplace )
        {
            AsmVariant a( L"Text" );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
        }

        TEST_METHOD( WStringConstPtr )
        {
            const wchar_t* cwstring = L"Text";
            AsmVariant a( cwstring );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(cwstring), a.imm_val64 );
        }

        TEST_METHOD( WStringArray )
        {
            const wchar_t  wstr_array[] = L"Text";
            AsmVariant a( wstr_array );

            Assert::AreEqual( a.type, AsmVariant::dataPtr );
            Assert::AreEqual( reinterpret_cast<uint64_t>(wstr_array), a.imm_val64 );
            Assert::AreEqual( sizeof( wstr_array ), a.size );
        }

        TEST_METHOD( WStringPtr )
        {
            const wchar_t  wstr_array[] = L"Text";
            wchar_t* wstr_ptr = const_cast<wchar_t*>(wstr_array);
            AsmVariant a( wstr_ptr );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(wstr_ptr), a.imm_val64 );
            Assert::AreEqual( sizeof( wstr_array ), a.size );
        }

        TEST_METHOD( VoidPtr )
        {
            const void* vptr = reinterpret_cast<const void*>(0xDEAD);
            AsmVariant a( vptr );

            Assert::AreEqual( a.type, AsmVariant::imm );
            Assert::AreEqual( reinterpret_cast<uint64_t>(vptr), a.imm_val64 );
        }

        TEST_METHOD( VoidPtrRef )
        {
            const void* vptr = reinterpret_cast<const void*>(0xDEAD);
            const auto& vptr_ref = vptr;

            AsmVariant a( vptr_ref );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(vptr_ref), a.imm_val64 );
        }

        TEST_METHOD( StructPtrLittle )
        {
            IntStruct little;
            AsmVariant a( &little );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( sizeof( little ), a.size );
            Assert::AreEqual( reinterpret_cast<uint64_t>(&little), a.imm_val64 );
        }

        TEST_METHOD( StructPtrBig )
        {
            PairStruct big;
            AsmVariant a( &big );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( sizeof( big ), a.size );
            Assert::AreEqual( reinterpret_cast<uint64_t>(&big), a.imm_val64 );
        }

        TEST_METHOD( StructLittle )
        {
            IntStruct little;
            AsmVariant a( little );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( sizeof( little ), a.size );
            Assert::AreEqual( 0, memcmp( &little, &a.imm_val64, sizeof( little ) ) );
        }

        TEST_METHOD( StructBig )
        {
            PairStruct big;
            AsmVariant a( big );

            Assert::AreEqual( AsmVariant::dataStruct, a.type );
            Assert::AreEqual( sizeof( big ), a.size );
            Assert::AreEqual( 0, memcmp( &big, a.buf.data(), sizeof( big ) ) );
        }

        TEST_METHOD( FunctionPtr )
        {
            AsmVariant a( &GetCurrentProcessId );

            Assert::AreEqual( AsmVariant::imm, a.type );
            Assert::AreEqual( reinterpret_cast<uint64_t>(&GetCurrentProcessId), a.imm_val64 );
        }

        TEST_METHOD( StructSlice )
        {
            PairStruct big;
            AsmVariant a( &big, 10 );

            Assert::AreEqual( AsmVariant::dataPtr, a.type );
            Assert::AreEqual( size_t( 10 ), a.size );
            Assert::AreEqual( reinterpret_cast<uint64_t>(&big), a.imm_val64 );
        }
    };
}