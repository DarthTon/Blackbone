#include "Tests.h"


/*
    Try to map calc.exe into current process
*/
void TestMMap()
{
    Process thisProc;
    thisProc.Attach( GetCurrentProcessId() );

    std::wcout << L"Manual image mapping test\n";
    std::wcout << L"Trying to map C:\\windows\\system32\\calc.exe into current process\n";

    int flags = CreateLdrRef | ManualImports | RebaseProcess | NoDelayLoad;

    if (thisProc.mmap().MapImage( L"C:\\windows\\system32\\calc.exe", flags ) == 0)
    {
        std::wcout << L"Mapping failed with error 0x" << std::hex << LastNtStatus()
                   << L". " << Utils::GetErrorDescription( LastNtStatus() ) << std::endl << std::endl;
    }
    else
        std::wcout << L"Successfully mapped, unmapping\n";

    thisProc.mmap().UnmapAllModules();
}
