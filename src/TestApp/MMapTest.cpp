#include "Tests.h"

/*
    Try to map calc.exe into current process
*/
void TestMMap()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    auto callback = []( CallbackType type, void* /*context*/, Process& /*process*/, const ModuleData& modInfo )
    {
        if(type == PreCallback)
        {
#ifdef USE64
            if (modInfo.name == L"msvcr120.dll" || modInfo.name == L"msvcp120.dll")
                return LoadData( MT_Native, Ldr_None );
#endif
        }
        else
        {
            if (modInfo.name == L"windows.storage.dll" || modInfo.name == L"shell32.dll")
                return LoadData( MT_Default, Ldr_ModList );
        }

        return LoadData( MT_Default, Ldr_None );
    };

    std::wcout << L"Manual image mapping test\n";
    std::wcout << L"Trying to map C:\\windows\\system32\\calc.exe into current process\n";

    eLoadFlags flags = ManualImports | RebaseProcess;

    if (thisProc.mmap().MapImage( L"C:\\windows\\system32\\calc.exe", flags, callback ) == 0)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << LastNtStatus()
                   << L". " << Utils::GetErrorDescription( LastNtStatus() ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    thisProc.mmap().UnmapAllModules();
}

/*
    Try to map cmd.exe into current process from buffer
*/
void TestMMapFromMem()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    void* buf = nullptr;
    auto size = 0;

    std::wcout << L"Manual image mapping from buffer test\n";
    std::wcout << L"Trying to map C:\\windows\\system32\\cmd.exe into current process\n";

    // Get image context
    HANDLE hFile = CreateFileW( L"C:\\windows\\system32\\cmd.exe", FILE_GENERIC_READ, 0x7, 0, OPEN_EXISTING, 0, 0 );
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytes = 0;
        size = GetFileSize( hFile, NULL );
        buf = VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_READWRITE );
        ReadFile( hFile, buf, size, &bytes, NULL );
        CloseHandle( hFile );
    }

    if (thisProc.mmap().MapImage( size, buf, false, CreateLdrRef | RebaseProcess | NoDelayLoad ) == 0)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << LastNtStatus()
                   << L". " << Utils::GetErrorDescription( LastNtStatus() ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    VirtualFree( buf, 0, MEM_RELEASE );

    thisProc.mmap().UnmapAllModules();
}
