#include "Tests.h"


TEST_CASE( "!01. AsmVariant init" )
{
    std::cout << "AsmVariant initialization test" << std::endl;

    const volatile int int_val = 2;

    enum e1 : char
    {
        e1_val0 = 3,
    };

    enum class ec : unsigned short
    {
        ec_val0 = 4,
    };

    struct S2
    {
        int16_t ival = 10;
    };

    struct S12
    {
        int64_t ival = 20;
        float fval = 24.0f;
    };

    const char* cstring = "Text";
    const char  str_array[] = "Text";
    char* str_ptr = const_cast<char*>(str_array);

    const wchar_t* cwstring = L"Text";
    const wchar_t  wstr_array[] = L"Text";
    wchar_t* wstr_ptr = const_cast<wchar_t*>(wstr_array);

    const void* vptr = reinterpret_cast<const void*>(0xDEAD);
    const auto& vptr_ref = vptr;

    S2 s2;
    S12 s12;

    AsmVariant a01( 1 );
    AsmVariant a02( int_val );
    AsmVariant a03( e1_val0 );
    AsmVariant a04( ec::ec_val0 );
    CHECK( a01.type == AsmVariant::imm );
    CHECK( a01.size == sizeof( 5 ) );
    CHECK( a01.imm_val64 == 1 );
    CHECK( a02.type == AsmVariant::imm );
    CHECK( a02.size == sizeof( int_val ) );
    CHECK( a02.imm_val64 == int_val );
    CHECK( a03.type == AsmVariant::imm );
    CHECK( a03.size == sizeof( e1_val0 ) );
    CHECK( a03.imm_val64 == e1_val0 );
    CHECK( a04.type == AsmVariant::imm );
    CHECK( a04.size == sizeof( ec::ec_val0 ) );
    CHECK( a04.imm_val64 == static_cast<uint64_t>(ec::ec_val0) );

    AsmVariant a05( 2.0f );
    CHECK( a05.type == AsmVariant::imm_float );
    CHECK( a05.size == sizeof( float ) );
    CHECK( a05.imm_float_val == 2.0f );

    AsmVariant a06( 3.0 );
    CHECK( a06.type == AsmVariant::imm_double );
    CHECK( a06.size == sizeof( double ) );
    CHECK( a06.imm_double_val == 3.0f );

    AsmVariant a07( "Text" );
    AsmVariant a08( cstring );
    AsmVariant a09( str_array );
    AsmVariant a10( str_ptr );
    CHECK( a07.type == AsmVariant::dataPtr );
    CHECK( a08.type == AsmVariant::dataPtr );
    CHECK( a08.imm_val64 == reinterpret_cast<uint64_t>(cstring) );
    CHECK( a09.type == AsmVariant::dataPtr );
    CHECK( a09.imm_val64 == reinterpret_cast<uint64_t>(str_array) );
    CHECK( a10.type == AsmVariant::dataPtr );
    CHECK( a10.imm_val64 == reinterpret_cast<uint64_t>(str_ptr) );

    AsmVariant a11( L"Text" );
    AsmVariant a12( cwstring );
    AsmVariant a13( wstr_array );
    AsmVariant a14( wstr_ptr );
    CHECK( a11.type == AsmVariant::dataPtr );
    CHECK( a12.type == AsmVariant::dataPtr );
    CHECK( a12.imm_val64 == reinterpret_cast<uint64_t>(cwstring) );
    CHECK( a13.type == AsmVariant::dataPtr );
    CHECK( a13.imm_val64 == reinterpret_cast<uint64_t>(wstr_array) );
    CHECK( a14.type == AsmVariant::dataPtr );
    CHECK( a14.imm_val64 == reinterpret_cast<uint64_t>(wstr_ptr) );

    AsmVariant a15( vptr );
    AsmVariant a16( vptr_ref );
    CHECK( a15.type == AsmVariant::imm );
    CHECK( a15.imm_val64 == reinterpret_cast<uint64_t>(vptr) );
    CHECK( a16.type == AsmVariant::imm );
    CHECK( a16.imm_val64 == reinterpret_cast<uint64_t>(vptr_ref) );

    AsmVariant a17( &s2 );
    AsmVariant a18( &s12 );
    CHECK( a17.type == AsmVariant::dataPtr );
    CHECK( a17.size == sizeof( s2 ) );
    CHECK( a17.imm_val64 == reinterpret_cast<uint64_t>(&s2) );
    CHECK( a18.type == AsmVariant::dataPtr );
    CHECK( a18.size == sizeof( s12 ) );
    CHECK( a18.imm_val64 == reinterpret_cast<uint64_t>(&s12) );

    AsmVariant a19( s2 );
    AsmVariant a20( s12 );
    CHECK( a19.type == AsmVariant::imm );
    CHECK( a19.size == sizeof( s2 ) );
    CHECK( memcmp( &s2, &a19.imm_val64, sizeof( s2 ) ) == 0 );
    CHECK( a20.type == AsmVariant::dataStruct );
    CHECK( a20.size == sizeof( s12 ) );
    CHECK( memcmp( &s12, a20.buf.data(), sizeof( s12 ) ) == 0 );

    AsmVariant a21( &GetCurrentProcessId );
    AsmVariant a22( GetCurrentProcessId );
    CHECK( a21.type == AsmVariant::imm );
    CHECK( a21.imm_val64 == reinterpret_cast<uint64_t>(&GetCurrentProcessId) );
    CHECK( a22.type == AsmVariant::imm );
    CHECK( a22.imm_val64 == reinterpret_cast<uint64_t>(&GetCurrentProcessId) );

    AsmVariant a23( &s12, 10 );
    CHECK( a23.type == AsmVariant::dataPtr );
    CHECK( a23.size == 10 );
    CHECK( a23.imm_val64 == reinterpret_cast<uint64_t>(&s12) );
}