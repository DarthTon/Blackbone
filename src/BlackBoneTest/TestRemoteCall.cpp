#include "Common.h"

namespace Testing
{
    constexpr char g_string[] = "The quick brown fox jumps over the lazy dog";
    constexpr wchar_t g_wstring[] = L"The quick brown fox jumps over the lazy dog";

    struct Dummy
    {
        uint32_t ival = 0;
        float fval = 0.0f;
        uint64_t uval = 0ull;
    };

    int TestFn( int a1, float a2, double a3, double* a4, int64_t a5, char* a6, wchar_t* a7, Dummy* a8, Dummy a9 )
    {
        *a4 = a3 + a2;
        strcpy_s( a6, MAX_PATH, g_string );
        wcscpy_s( a7, MAX_PATH, g_wstring );
        *a8 = a9;

        return static_cast<int>(a1 + a5);
    }

    TEST_CLASS( RemoteCall )
    {
    public:
        template<typename T>
        void ValidateArg( const AsmVariant& arg, AsmVariant::eType type, size_t size, T value )
        {
            AssertEx::AreEqual( type, arg.type );
            AssertEx::AreEqual( size, arg.size );

            if constexpr (std::is_same_v<T, float>)
            {
                AssertEx::AreEqual( value, arg.imm_float_val );
            }
            else if constexpr(std::is_same_v<T, double>)
            {
                AssertEx::AreEqual( value, arg.imm_double_val );
            }
            else if constexpr (std::is_pointer_v<std::decay_t<T>>)
            {
                AssertEx::AreEqual( reinterpret_cast<uint64_t>(value), arg.imm_val64 );
            }
            else if (value != 0)
            {
                AssertEx::AreEqual( uint64_t( value ), arg.imm_val64 );
            }
        }

        TEST_METHOD( ConstructDirect )
        {
            Process process;
            RemoteFunction<decltype(&TestFn)> pFN( process, ptr_t( 0 ) );

            float f = 2.0;
            double d = 3.0;
            double& ref = d;

            decltype(pFN)::CallArguments args( 1, f, ref, &d, 5ll, _cbuf, _wbuf, &_output, _input );

            AssertEx::AreEqual( size_t( 9 ), args.arguments.size() );

            ValidateArg( args.arguments[0], AsmVariant::imm, sizeof( int ), 1 );
            ValidateArg( args.arguments[1], AsmVariant::imm_float, sizeof( float ), 2.0f );
            ValidateArg( args.arguments[2], AsmVariant::imm_double, sizeof( double ), 3.0 );
            ValidateArg( args.arguments[3], AsmVariant::dataPtr, sizeof( double ), &d );
            ValidateArg( args.arguments[4], AsmVariant::imm, sizeof( int64_t ), 5 );
            ValidateArg( args.arguments[5], AsmVariant::dataPtr, sizeof( '\0' ), _cbuf );
            ValidateArg( args.arguments[6], AsmVariant::dataPtr, sizeof( L'\0' ), _wbuf );
            ValidateArg( args.arguments[7], AsmVariant::dataPtr, sizeof( _output ), &_output );
            ValidateArg( args.arguments[8], AsmVariant::dataStruct, sizeof( _input ), 0 );
        }

        TEST_METHOD( ConstructFromList )
        {
            Process process;
            RemoteFunction<decltype(&TestFn)> pFN( process, ptr_t( 0 ) );

            float f = 2.0;
            double d = 3.0;
            double& ref = d;

            decltype(pFN)::CallArguments args{ 1, f, ref, &d, 5ll, _cbuf, _wbuf, &_output, _input };

            AssertEx::AreEqual( size_t( 9 ), args.arguments.size() );

            ValidateArg( args.arguments[0], AsmVariant::imm, sizeof( int ), 1 );
            ValidateArg( args.arguments[1], AsmVariant::imm_float, sizeof( float ), 2.0f );
            ValidateArg( args.arguments[2], AsmVariant::imm_double, sizeof( double ), 3.0 );
            ValidateArg( args.arguments[3], AsmVariant::dataPtr, sizeof( double ), &d );
            ValidateArg( args.arguments[4], AsmVariant::imm, sizeof( int64_t ), 5 );
            ValidateArg( args.arguments[5], AsmVariant::dataPtr, sizeof( _cbuf ), _cbuf );
            ValidateArg( args.arguments[6], AsmVariant::dataPtr, sizeof( _wbuf ), _wbuf );
            ValidateArg( args.arguments[7], AsmVariant::dataPtr, sizeof( _output ), &_output );
            ValidateArg( args.arguments[8], AsmVariant::dataStruct, sizeof( _input ), 0 );
        }

        TEST_METHOD( LocalCall )
        {
            Process process;
            AssertEx::NtSuccess( process.Attach( GetCurrentProcessId() ) );
            auto pFN = MakeRemoteFunction<decltype(&TestFn)>( process, &TestFn );
            double d = 0.0;

            _input.ival = 0xDEAD;
            _input.fval = 1337.0f;
            _input.uval = 0xDEADC0DEA4DBEEFull;

            auto[status, result] = pFN.Call( { 1, 2.0f, 3.0, &d, 5ll, _cbuf, _wbuf, &_output, _input } );
            AssertEx::NtSuccess( status );
            AssertEx::IsTrue( result.has_value() );

            AssertEx::AreEqual( 1 + 5, result.value() );
            AssertEx::AreEqual( 3.0 + 2.0f, d, 0.001 );
            AssertEx::AreEqual( _cbuf, g_string );
            AssertEx::AreEqual( _wbuf, g_wstring );
            AssertEx::IsZero( memcmp( &_input, &_output, sizeof( _input ) ) );
        }

        TEST_METHOD( CallLoop )
        {
            Process process;
            AssertEx::NtSuccess( process.Attach( GetCurrentProcessId() ) );
            AssertEx::NtSuccess( process.remote().CreateRPCEnvironment( Worker_CreateNew, true ) );

            auto pFN = MakeRemoteFunction<decltype(&TestFn)>( process, &TestFn );
            auto worker = process.remote().getWorker();
            double d = 0.0;

            _input.ival = 0xDEAD;
            _input.fval = 1337.0f;
            _input.uval = 0xDEADC0DEA4DBEEFull;

            for(auto i = 0; i < 10000; i++)
            {
                auto [status, result] = pFN.Call( { 1, 2.0f, 3.0, &d, 5ll, _cbuf, _wbuf, &_output, _input }, worker );
                AssertEx::NtSuccess( status );
                AssertEx::IsTrue( result.has_value() );
                AssertEx::AreEqual( 1 + 5, result.value() );
            }
        }

        TEST_METHOD( BoundThread )
        {
            Process process;
            AssertEx::NtSuccess( process.Attach( GetCurrentProcessId() ) );

            DWORD id = 0;
            auto code = []( void* ) -> DWORD
            {
                for (;;)
                    Sleep( 1 );

                return ERROR_SUCCESS;
            };

            HANDLE hThread = CreateThread( nullptr, 0, code, nullptr, 0, &id );

            AssertEx::IsNotNull( hThread );
            AssertEx::IsNotZero( id );

            auto thread = process.threads().get( id );
            AssertEx::IsNotNull( thread.get() );

            auto pFN = MakeRemoteFunction<decltype(&TestFn)>( process, &TestFn, thread );
            double d = 0.0;

            _input.ival = 0xDEAD;
            _input.fval = 1337.0f;
            _input.uval = 0xDEADC0DEA4DBEEFull;

            for (auto i = 0; i < 100; i++)
            {
                auto [status, result] = pFN.Call( { 1, 2.0f, 3.0, &d, 5ll, _cbuf, _wbuf, &_output, _input } );
                AssertEx::NtSuccess( status );
                AssertEx::IsTrue( result.has_value() );
                AssertEx::AreEqual( 1 + 5, result.value() );
            }

            TerminateThread( hThread, ERROR_SUCCESS );
            CloseHandle( hThread );
        }

        TEST_METHOD( NtQueryVirtualMemory )
        {
            auto path = GetTestHelperHost();
            AssertEx::IsTrue( Utils::FileExists( path ) );

            // Give process some time to initialize
            Process process;
            AssertEx::NtSuccess( process.CreateAndAttach( path ) );
            Sleep( 100 );

            auto hMainMod = process.modules().GetMainModule();
            AssertEx::IsNotNull( hMainMod.get() );

            auto pFN = MakeRemoteFunction<fnNtQueryVirtualMemory>( process, L"ntdll.dll", "NtQueryVirtualMemory" );
            AssertEx::IsTrue( pFN.valid() );

            uint8_t buf[0x200] = { };
            auto result = pFN.Call( {
                INVALID_HANDLE_VALUE,
                static_cast<uintptr_t>(hMainMod->baseAddress),
                MemorySectionName,
                buf,
                sizeof( buf ),
                nullptr
                } );

            process.Terminate();

            AssertEx::NtSuccess( result.status );
            AssertEx::NtSuccess( result.result() );

            std::wstring name( reinterpret_cast<wchar_t*>(buf + sizeof( UNICODE_STRING )) );
            AssertEx::AreNotEqual( name.npos, name.rfind( Utils::StripPath( path ) ) );
        }

        TEST_METHOD( FileOP )
        {
            auto path = GetTestHelperHost();
            AssertEx::IsTrue( Utils::FileExists( path ) );

            // Give process some time to initialize
            Process process;
            AssertEx::NtSuccess( process.CreateAndAttach( path ) );
            Sleep( 100 );

            auto CreateFileWPtr = process.modules().GetExport( L"kernel32.dll", "CreateFileW" );
            auto WriteFilePtr = process.modules().GetExport( L"kernel32.dll", "WriteFile" );
            auto CloseHandlePtr = process.modules().GetExport( L"kernel32.dll", "CloseHandle" );

            AssertEx::IsTrue( CreateFileWPtr.success() );
            AssertEx::IsTrue( WriteFilePtr.success() );
            AssertEx::IsTrue( CloseHandlePtr.success() );

            uint8_t writeBuf[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

            RemoteFunction<decltype(&CreateFileW)> pCreateFile( process, CreateFileWPtr->procAddress );
            RemoteFunction<decltype(&WriteFile)> pWriteFile( process, WriteFilePtr->procAddress );
            RemoteFunction<decltype(&CloseHandle)> pCloseHandle( process, CloseHandlePtr->procAddress );

            auto filePath = L"DummyFile_FileOP.dat";
            auto handle = pCreateFile.Call( filePath, GENERIC_WRITE, 0x7, nullptr, CREATE_ALWAYS, 0, nullptr );

            AssertEx::NtSuccess( handle.status );
            AssertEx::IsNotNull( handle.result() );

            DWORD bytes = 0;
            auto success = pWriteFile.Call( { handle.result(), writeBuf, sizeof( writeBuf ), &bytes, nullptr } );
            pCloseHandle.Call( handle.result(), nullptr );

            process.Terminate();

            AssertEx::NtSuccess( success.status );
            AssertEx::IsTrue( success.result() );
            AssertEx::AreEqual( static_cast<DWORD>(sizeof( writeBuf )), bytes );

            // Check locally
            auto hFile = Handle( CreateFileW( filePath, GENERIC_READ, 0x7, nullptr, OPEN_EXISTING, FILE_DELETE_ON_CLOSE, nullptr ) );
            AssertEx::IsTrue( hFile );

            uint8_t readBuf[sizeof( writeBuf )] = { };
            BOOL b = ReadFile( hFile, readBuf, sizeof( readBuf ), &bytes, nullptr );

            AssertEx::IsTrue( b );
            AssertEx::AreEqual( static_cast<DWORD>(sizeof( readBuf )), bytes );
            AssertEx::IsZero( memcmp( writeBuf, readBuf, sizeof( readBuf ) ) );
        }

    private:
        char _cbuf[MAX_PATH] = { };
        wchar_t _wbuf[MAX_PATH] = { };
        Dummy _input, _output;
    };
}