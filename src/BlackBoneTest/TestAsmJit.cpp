#include "Common.h"

namespace Testing
{
    TEST_CLASS( AsmJIT )
    {
    public:
        TEST_METHOD( Add )
        {
            auto asmPtr = AsmFactory::GetAssembler();
            auto& a = *asmPtr;

            a.GenPrologue();
            a->add( a->zcx, a->zdx );
            a->mov( a->zax, a->zcx );
            a.GenEpilogue();

            auto func = reinterpret_cast<intptr_t( __fastcall* )(intptr_t, intptr_t)>(a->make());

            AssertEx::AreEqual( intptr_t( 15 ), func( 10, 5 ) );
            AssertEx::AreEqual( intptr_t( 5 ), func( 10, -5 ) );
        }

        TEST_METHOD( Call )
        {
            auto asmPtr = AsmFactory::GetAssembler();
            auto& a = *asmPtr;

            wchar_t buf[MAX_PATH] = { };

            a.GenPrologue();
            a.GenCall( reinterpret_cast<uintptr_t>(&GetModuleHandle), { ModuleName } );
            a.GenCall( reinterpret_cast<uintptr_t>(&GetModuleFileNameW), { a->zax, buf, _countof( buf ) } );
            a.GenEpilogue();

            auto func = reinterpret_cast<void(*)()>(a->make());
            func();

            std::wstring name = Utils::ToLower( buf );
            AssertEx::AreNotEqual( name.npos, name.rfind( Utils::ToLower( ModuleName ) ) );
        }

        TEST_METHOD( MultiCall )
        {
            auto asmPtr = AsmFactory::GetAssembler();
            auto& a = *asmPtr;

            a.EnableX64CallStack( false );
            auto skip = a->newLabel();

            uint8_t writeBuf[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
            auto filePath = L"DummyFile_MultiCall.dat";
            DWORD bytes = 0;

            AsmStackAllocator stack( a.assembler() );
            ALLOC_STACK_VAR( stack, handle, HANDLE );

            a.GenPrologue();
            a->sub( a->zsp, 0x48 );
            a.GenCall( reinterpret_cast<uintptr_t>(&CreateFileW), { a->zcx, GENERIC_WRITE, 0x7, nullptr, CREATE_ALWAYS, 0, nullptr } );
            a->mov( handle, a->zax );
            a->cmp( a->zax, reinterpret_cast<uintptr_t>(INVALID_HANDLE_VALUE) );
            a->je( skip );
            a.GenCall( reinterpret_cast<uintptr_t>(&WriteFile), { handle, writeBuf, sizeof( writeBuf ), &bytes, nullptr } );
            a->test( a->zax, a->zax );
            a->jz( skip );
            a.GenCall( reinterpret_cast<uintptr_t>(&CloseHandle), { handle } );
            a->bind( skip );
            a->add( a->zsp, 0x48 );
            a.GenEpilogue();
            
            auto func = reinterpret_cast<BOOL( __fastcall * )(LPCWSTR)>(a->make());
            BOOL b = func( filePath );
            AssertEx::IsTrue( b );

            // Check locally
            auto hFile = Handle( CreateFileW( filePath, GENERIC_READ, 0x7, nullptr, OPEN_EXISTING, FILE_DELETE_ON_CLOSE, nullptr ) );
            AssertEx::IsTrue( hFile );

            uint8_t readBuf[sizeof( writeBuf )] = { };
            b = ReadFile( hFile, readBuf, sizeof( readBuf ), &bytes, nullptr );

            AssertEx::IsTrue( b );
            AssertEx::AreEqual( static_cast<DWORD>(sizeof( readBuf )), bytes );
            AssertEx::IsZero( memcmp( writeBuf, readBuf, sizeof( readBuf ) ) );
        }
    };
}