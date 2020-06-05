#include "Common.h"

namespace Testing
{

TEST_CLASS( Modules )
{
public:
    TEST_METHOD_INITIALIZE( ClassInitialize )
    {
        AssertEx::NtSuccess( _proc.Attach( GetCurrentProcessId() ) );
    }

    TEST_METHOD( MainModule )
    {
        auto mod = _proc.modules().GetMainModule();
        ValidateModule( *mod, (module_t)GetModuleHandleW( nullptr ) );
    }

    TEST_METHOD( EnumModules )
    {
        auto name = L"kernel32.dll";

        auto manuals = _proc.modules().GetManualModules();
        AssertEx::IsTrue( manuals.empty() );

        auto byLdrList = _proc.modules().GetModule( name, LdrList );

        _proc.modules().reset();
        auto byHeaders = _proc.modules().GetModule( name, Sections );

        ValidateModule( *byLdrList, reinterpret_cast<module_t>(GetModuleHandleW( name )) );
        ValidateModule( *byHeaders, reinterpret_cast<module_t>(GetModuleHandleW( name )) );

        AssertEx::AreEqual( byLdrList->baseAddress, byHeaders->baseAddress );
        AssertEx::AreEqual( byLdrList->name, byHeaders->name );
    }

    void ValidateModule( const ModuleData& mod, module_t expectedBase )
    {
        AssertEx::AreEqual( expectedBase, mod.baseAddress );
        AssertEx::IsNotZero( mod.size );
        AssertEx::IsFalse( mod.manual );
        AssertEx::IsFalse( mod.name.empty() );
        AssertEx::IsFalse( mod.fullPath.empty() );

#ifdef USE64
        AssertEx::AreEqual( static_cast<uint32_t>(mt_mod64), static_cast<uint32_t>(mod.type) );
#else
        AssertEx::AreEqual( static_cast<uint32_t>(mt_mod32), static_cast<uint32_t>(mod.type) );
#endif
    }

    TEST_METHOD( Export )
    {
        auto expected = GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "CreateFileW" );
        AssertEx::IsNotNull( reinterpret_cast<void*>(expected) );

        auto result = _proc.modules().GetExport( L"kernel32.dll", "CreateFileW" );
        AssertEx::IsTrue( result.success() );
        AssertEx::AreEqual( reinterpret_cast<ptr_t>(expected), result->procAddress );
    }

private:
    Process _proc;
};
}