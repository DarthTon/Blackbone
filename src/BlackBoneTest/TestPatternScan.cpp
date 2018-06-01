#include "Common.h"

namespace Testing
{
    TEST_CLASS( PatternScan )
    {
    public:
        TEST_METHOD_INITIALIZE( ClassInitialize )
        {
            AssertEx::NtSuccess( _proc.Attach( L"explorer.exe" ) );
        }

        // Scan all allocated process memory
        TEST_METHOD( Simple )
        {
            PatternSearch ps1( "\x48\x89\xD0" );

            std::vector<ptr_t> results;
            ps1.SearchRemoteWhole( _proc, false, 0, results );
            AssertEx::IsTrue( results.size() > 0 );
        }

        // Scan only inside 'explorer.exe' module
        TEST_METHOD( WithWildcard )
        {
            PatternSearch ps2{ 0x56, 0x57, 0xCC, 0x55 };

            auto pMainMod = _proc.modules().GetMainModule();
            AssertEx::IsNotNull( pMainMod.get() );

            std::vector<ptr_t> results;
            ps2.SearchRemote( _proc, 0xCC, pMainMod->baseAddress, pMainMod->size, results );
            AssertEx::IsTrue( results.size() > 0 );
        }

    private:
        Process _proc;
    };
}