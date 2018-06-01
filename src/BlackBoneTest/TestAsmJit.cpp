#include "Common.h"

namespace Microsoft::VisualStudio::CppUnitTestFramework {
    template<> inline std::wstring ToString<__int64>( const __int64& t ) { RETURN_WIDE_STRING( t ); }
}

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

            Assert::AreEqual( intptr_t( 15 ), func( 10, 5 ) );
            Assert::AreEqual( intptr_t( 5 ), func( 10, -5 ) );
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
            Assert::AreNotEqual( name.npos, name.rfind( Utils::ToLower( ModuleName ) ) );
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

            auto stackSize = stack.getTotalSize();
            if (a.assembler()->getArch() == asmjit::kArchX64)
                stackSize = Align( stackSize, 0x100 );      

            a.GenPrologue();
            a->sub( a->zsp, stackSize );
            a.GenCall( reinterpret_cast<uintptr_t>(&CreateFileW), { a->zcx, GENERIC_WRITE, 0x7, nullptr, CREATE_ALWAYS, 0, nullptr } );
            a->mov( handle, a->zax );
            a->cmp( a->zax, reinterpret_cast<uintptr_t>(INVALID_HANDLE_VALUE) );
            a->je( skip );
            a.GenCall( reinterpret_cast<uintptr_t>(&WriteFile), { handle, writeBuf, sizeof( writeBuf ), &bytes, nullptr } );
            a->test( a->zax, a->zax );
            a->jz( skip );
            a.GenCall( reinterpret_cast<uintptr_t>(&CloseHandle), { handle } );
            a->bind( skip );
            a->add( a->zsp, stackSize );
            a.GenEpilogue();
            
            auto func = reinterpret_cast<BOOL( __fastcall * )(LPCWSTR)>(a->make());
            BOOL b = func( filePath );
            Assert::IsTrue( b );

            // Check locally
            FileHandle hFile = FileHandle( CreateFileW( filePath, GENERIC_READ, 0x7, nullptr, OPEN_EXISTING, FILE_DELETE_ON_CLOSE, nullptr ) );
            Assert::IsTrue( hFile );

            uint8_t readBuf[sizeof( writeBuf )] = { };
            b = ReadFile( hFile, readBuf, sizeof( readBuf ), &bytes, nullptr );

            Assert::IsTrue( b );
            Assert::AreEqual( static_cast<DWORD>(sizeof( readBuf )), bytes );
            AssertEx::IsZero( memcmp( writeBuf, readBuf, sizeof( readBuf ) ) );
        }
    };
}