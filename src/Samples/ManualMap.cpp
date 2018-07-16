#include <BlackBone/Process/Process.h>
#include <3rd_party/VersionApi.h>

#include <iostream>
#include <set>
using namespace blackbone;

std::set<std::wstring> nativeMods, modList;

/*
    Try to map calc.exe into current process
*/
void MapCalcFromFile()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    nativeMods.clear();
    modList.clear();

    nativeMods.emplace( L"combase.dll" );
    nativeMods.emplace( L"user32.dll" );
    if (WinVer().ver == Win7)
    {
        nativeMods.emplace( L"gdi32.dll" );
        nativeMods.emplace( L"msvcr120.dll" );
        nativeMods.emplace( L"msvcp120.dll" );
    }

    modList.emplace( L"windows.storage.dll" );
    modList.emplace( L"shell32.dll" );
    modList.emplace( L"shlwapi.dll" );

    auto callback = []( CallbackType type, void* /*context*/, Process& /*process*/, const ModuleData& modInfo )
    {
        if(type == PreCallback)
        {
            if(nativeMods.count(modInfo.name))
                return LoadData( MT_Native, Ldr_None );
        }
        else
        {
            if (modList.count( modInfo.name ))
                return LoadData( MT_Default, Ldr_ModList );
        }

        return LoadData( MT_Default, Ldr_None );
    };

    std::wcout << L"Manual image mapping test" << std::endl;
    std::wcout << L"Trying to map C:\\windows\\system32\\calc.exe into current process" << std::endl;

    auto image = thisProc.mmap().MapImage( L"C:\\windows\\system32\\calc.exe", ManualImports | RebaseProcess, callback );
    if (!image)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << image.status
                   << L". " << Utils::GetErrorDescription( image.status ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    thisProc.mmap().UnmapAllModules();
}

/*
    Try to map cmd.exe into current process from buffer
*/
void MapCmdFromMem()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    void* buf = nullptr;
    auto size = 0;

    std::wcout << L"Manual image mapping from buffer test" << std::endl;
    std::wcout << L"Trying to map C:\\windows\\system32\\cmd.exe into current process" << std::endl;

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

    auto image = thisProc.mmap().MapImage( size, buf, false, CreateLdrRef | RebaseProcess | NoDelayLoad );
    if (!image)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << image.status
                   << L". " << Utils::GetErrorDescription( image.status ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    VirtualFree( buf, 0, MEM_RELEASE );

    thisProc.mmap().UnmapAllModules();
}
