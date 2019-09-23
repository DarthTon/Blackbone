#include "Common.h"

namespace Testing
{
    TEST_CLASS( Symbols )
    {
    public:
        TEST_METHOD( SymbolsMatch )
        {
            SymbolLoader sl;
            SymbolData fromSymbols, fromPatterns;

            auto [ntdll32, ntdll64] = sl.LoadImages();

            AssertEx::IsNotZero( ntdll32.imageBase() );
            AssertEx::IsNotZero( ntdll64.imageBase() );
       
            AssertEx::NtSuccess( sl.LoadFromPatterns( ntdll32, ntdll64, fromPatterns ) );

            if (NT_SUCCESS( sl.LoadFromSymbols( ntdll32, ntdll64, fromSymbols ) ))
            {
                AssertEx::AreEqual( fromSymbols.LdrpHandleTlsData32, fromPatterns.LdrpHandleTlsData32 );
                AssertEx::AreEqual( fromSymbols.LdrpHandleTlsData64, fromPatterns.LdrpHandleTlsData64 );
                AssertEx::AreEqual( fromSymbols.LdrpInvertedFunctionTable32, fromPatterns.LdrpInvertedFunctionTable32 );
                AssertEx::AreEqual( fromSymbols.LdrpInvertedFunctionTable64, fromPatterns.LdrpInvertedFunctionTable64 );
                AssertEx::AreEqual( fromSymbols.LdrProtectMrdata, fromPatterns.LdrProtectMrdata );
                AssertEx::AreEqual( fromSymbols.RtlInsertInvertedFunctionTable32, fromPatterns.RtlInsertInvertedFunctionTable32 );
                AssertEx::AreEqual( fromSymbols.RtlInsertInvertedFunctionTable64, fromPatterns.RtlInsertInvertedFunctionTable64 );

                AssertEx::IsNotZero( fromSymbols.LdrpReleaseTlsEntry32 );
                AssertEx::IsNotZero( fromSymbols.LdrpReleaseTlsEntry64 );
            }
            else
            {
                Logger::WriteMessage( "Failed to load symbols, aborting" );
            }
        }
    };
}