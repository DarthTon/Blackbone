#include "Tests.h"
#include "../BlackBone/Patterns/PatternSearch.h"

void TestPatterns()
{
    Process explorer;
    std::vector<DWORD> procs;
    std::vector<ptr_t> results;

    std::wcout << L"Remote pattern match test. Using 'explorer.exe as a target'\n";

    Process::EnumByName( L"explorer.exe", procs );

    if (!procs.empty())
    {
        explorer.Attach( procs.front() );
        auto pMainMod = explorer.modules().GetMainModule();

        // Initialize patterns
        PatternSearch ps1( "\x48\x89\xD1" );
        PatternSearch ps2{ 0x56, 0x57, 0xCC, 0x55 };

        // Scan all allocated process memory
        std::wcout << L"Searching for '48 89 D1'... ";
        ps1.SearchRemoteWhole( explorer, false, 0, results );
        std::wcout << L"Found " << results.size() << L" results\n";

        results.clear();

        // Scan only inside 'explorer.exe' module
        std::wcout << L"Searching for '56 57 ?? 55 using CC as a wildcard'... ";
        ps2.SearchRemote( explorer, 0xCC, pMainMod->baseAddress, pMainMod->size, results );
        std::wcout << L"Found " << results.size() << L" results\n\n";
        results.clear();
    }
    else
        std::wcout << L"Can't find explorer.exe, aborting\n\n";
}