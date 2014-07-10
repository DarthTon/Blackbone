#include "Tests.h"

/*
    Try to map calc.exe into current process
*/
void TestMMap()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    auto ldrDef = []( void* /*context*/, const ModuleData& modInfo )
    {
        //auto pProc = reinterpret_cast<Process*>(context);
        if (modInfo.name == L"calc.exe")
            return Ldr_ModList | Ldr_HashTable;

        return Ldr_None;
    };

    std::wcout << L"Manual image mapping test\n";
    std::wcout << L"Trying to map C:\\windows\\system32\\calc.exe into current process\n";

    eLoadFlags flags = ManualImports | RebaseProcess | NoDelayLoad;

    if (thisProc.mmap().MapImage( L"C:\\windows\\system32\\calc.exe", flags, ldrDef, &thisProc ) == 0)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << LastNtStatus()
                   << L". " << Utils::GetErrorDescription( LastNtStatus() ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    thisProc.mmap().UnmapAllModules();
}
