#include "Common.h"

namespace Testing
{
    struct DllLoadData
    {
        bool initialized = false;
        bool deinitialized = false;
        bool static_initialized = false;
        bool static_thread_initialized = false;
        bool static_func_initialized = false;
        bool seh_internal = false;
        bool seh_external = false;
        bool ceh_internal = false;
        bool ceh_external = false;
        bool ceh_uncaught = false;
    };

    TEST_CLASS( ManualMapping )
    {
    public:
        TEST_CLASS_INITIALIZE( Initialize )
        {
            nativeMods.emplace( L"combase.dll" );
            nativeMods.emplace( L"user32.dll" );

            modList.emplace( L"windows.storage.dll" );
            modList.emplace( L"shell32.dll" );
            modList.emplace( L"shlwapi.dll" );
        }

        static LoadData MapCallback( CallbackType type, void* /*context*/, Process& /*process*/, const ModuleData& modInfo )
        {
            if (type == PreCallback)
            {
                if (nativeMods.count( modInfo.name ))
                    return LoadData( MT_Native, Ldr_None );
            }
            else
            {
                if (modList.count( modInfo.name ))
                    return LoadData( MT_Default, Ldr_ModList );
            }

            return LoadData( MT_Default, Ldr_None );
        };

        void ValidateDllLoad( const DllLoadData& data )
        {
            AssertEx::IsTrue( data.initialized );
            AssertEx::IsFalse( data.deinitialized );
            AssertEx::IsTrue( data.static_initialized );
            AssertEx::IsTrue( data.static_thread_initialized );
            AssertEx::IsTrue( data.static_func_initialized );
            AssertEx::IsFalse( data.seh_internal );
            AssertEx::IsTrue( data.seh_external );
            AssertEx::IsFalse( data.ceh_internal );
            AssertEx::IsTrue( data.ceh_external );
            AssertEx::IsFalse( data.ceh_uncaught );
        }

        TEST_METHOD( FromFile )
        {
            auto hostPath = GetTestHelperHost();
            auto dllPath = GetTestHelperDll();

            Process proc;
            NTSTATUS status = proc.CreateAndAttach( hostPath );
            AssertEx::NtSuccess( status );

            auto image = proc.mmap().MapImage( dllPath, ManualImports, &MapCallback );
            AssertEx::IsTrue( image.success() );
            AssertEx::IsNotNull( image.result().get() );

            auto g_loadDataPtr = proc.modules().GetExport( image.result(), "g_LoadData" );
            AssertEx::IsTrue( g_loadDataPtr.success() );
            AssertEx::IsNotZero( g_loadDataPtr->procAddress );

            auto g_loadData = proc.memory().Read<DllLoadData>( g_loadDataPtr->procAddress );
            AssertEx::IsTrue( g_loadData.success() );

            proc.Terminate();

            ValidateDllLoad( g_loadData.result() );
        }

        std::pair<std::unique_ptr<uint8_t[]>, uint32_t> GetFileData( const std::wstring& path )
        {
            auto hFile = FileHandle( CreateFileW( path.c_str(), FILE_GENERIC_READ, 0x7, nullptr, OPEN_EXISTING, 0, nullptr ) );
            if (hFile)
            {
                uint32_t size = GetFileSize( hFile, nullptr );
                auto buf = std::make_unique<uint8_t[]>( size );

                DWORD bytes = 0;
                if (ReadFile( hFile, buf.get(), size, &bytes, nullptr ))
                    return std::make_pair( std::move( buf ), size );
            }

            return std::pair<std::unique_ptr<uint8_t[]>, uint32_t>();
        }

        TEST_METHOD( FromMemory )
        {
            auto hostPath = GetTestHelperHost();
            auto dllPath = GetTestHelperDll();

            auto[buf, size] = GetFileData( dllPath );
            AssertEx::IsNotZero( size );

            Process proc;
            NTSTATUS status = proc.CreateAndAttach( hostPath );
            AssertEx::NtSuccess( status );

            auto image = proc.mmap().MapImage( size, buf.get(), false, ManualImports, &MapCallback );
            AssertEx::IsTrue( image.success() );
            AssertEx::IsNotNull( image.result().get() );

            auto g_loadDataPtr = proc.modules().GetExport( image.result(), "g_LoadData" );
            AssertEx::IsTrue( g_loadDataPtr.success() );
            AssertEx::IsNotZero( g_loadDataPtr->procAddress );

            auto g_loadData = proc.memory().Read<DllLoadData>( g_loadDataPtr->procAddress );
            AssertEx::IsTrue( g_loadData.success() );

            proc.Terminate();

            ValidateDllLoad( g_loadData.result() );
        }

    private:
        static inline std::set<std::wstring> nativeMods, modList;
    };
}