#include "PatternLoader.h"
#include "../PE/PEImage.h"
#include "../Include/Winheaders.h"
#include "../Misc/Trace.hpp"
#include "../../../contrib/VersionHelpers.h"

namespace blackbone
{
/// <summary>
/// Scan ntdll for internal loader data
/// </summary>
/// <returns>Found data</returns>
const PatternData& PatternLoader::DoSearch()
{
    CheckSystem();

    pe::PEImage ntdll32;
    pe::PEImage ntdll64;
    ptr_t scanStart32 = 0, scanSize32 = 0;
    ptr_t scanStart64 = 0, scanSize64 = 0;
    int64_t diff32 = 0, diff64 = 0;

    wchar_t buf[MAX_PATH] = { 0 };
    GetWindowsDirectoryW( buf, MAX_PATH );

    std::wstring windir( buf );

    // Load ntdlls
    if (_x86OS)
    {
        ntdll32.Load( std::wstring( windir + L"\\System32\\ntdll.dll" ), true );
        diff32 = static_cast<int64_t>(ntdll32.imageBase()) - reinterpret_cast<int64_t>(ntdll32.base());
    }
    else
    {
        FsRedirector fsr( _wow64Process );
        ntdll64.Load( std::wstring( windir + L"\\System32\\ntdll.dll" ), true );
        ntdll32.Load( std::wstring( windir + L"\\SysWOW64\\ntdll.dll" ), true );

        diff32 = static_cast<int64_t>(ntdll32.imageBase()) - reinterpret_cast<int64_t>(ntdll32.base());
        diff64 = static_cast<int64_t>(ntdll64.imageBase()) - reinterpret_cast<int64_t>(ntdll64.base());
    }

    auto fillRanges = []( const pe::PEImage& file, ptr_t& start, ptr_t& size )
    {
        for (auto& sec : file.sections())
            if (_stricmp( reinterpret_cast<const char*>(sec.Name), ".text" ) == 0)
            {
                start = reinterpret_cast<ptr_t>(file.base()) + sec.VirtualAddress;
                size = sec.Misc.VirtualSize;
                break;
            }
    };

    // Get code section bounds
    fillRanges( ntdll32, scanStart32, scanSize32 );
    if (!_x86OS)
        fillRanges( ntdll64, scanStart64, scanSize64 );

    auto scan = [&]( OffsetData& rule, auto& result )
    {
        ptr_t scanStart = 0, scanSize = 0;
        int64_t diff = 0;
        std::vector<ptr_t> found;

        if (rule.bit64)
        {
            scanStart = scanStart64;
            scanSize = scanSize64;
            diff = diff64;
        }
        else
        {
            scanStart = scanStart32;
            scanSize = scanSize32;
            diff = diff32;
        }

        rule.pattern.Search( reinterpret_cast<void*>(scanStart), static_cast<size_t>(scanSize), found );
        if (!found.empty())
        {
            // Plain pointer sum
            if (rule.functionOffset != -1)
            {
                result = found.front() - rule.functionOffset + diff;
            }
            // Pointer dereference inside instruction
            else if (rule.dataStartOffset != 0)
            {
                if (rule.bit64)
                {
                    result = *reinterpret_cast<int32_t*>(found.front() + (rule.dataStartOffset + rule.dataOperandOffset)) +
                        (found.front() + rule.dataStartOffset + rule.dataInstructionSize) + diff;
                }
                else
                    result = *reinterpret_cast<int32_t*>(found.front() + rule.dataStartOffset);
            }
        }
    };

    std::unordered_map<ptr_t*, OffsetData> patterns;
    OSFillPatterns( patterns );

    // Final search
    for (auto& e : patterns)
        scan( e.second, *e.first );

    // Retry with old patterns
    if (_data.RtlInsertInvertedFunctionTable32 == 0 && IsWindows8Point1OrGreater() && !IsWindows10CreatorsOrGreater())
    {
        // RtlInsertInvertedFunctionTable
        // 8D 45 F4 89 55 F8 50 8D 55 FC
        OffsetData rule1{ "\x8d\x45\xf4\x89\x55\xf8\x50\x8d\x55\xfc", false, 0xB };
        OffsetData rule2{ "\x8d\x45\xf4\x89\x55\xf8\x50\x8d\x55\xfc", false, -1, 0x1D };

        scan( rule1, _data.RtlInsertInvertedFunctionTable32 );
        scan( rule2, _data.LdrpInvertedFunctionTable32 );
    }

    // Report errors
#ifndef BLACKBONE_NO_TRACE
    if (_data.LdrpHandleTlsData64 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpHandleTlsData64 not found" );
    if (_data.LdrpHandleTlsData32 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpHandleTlsData32 not found" );
    if (IsWindows8Point1OrGreater() && _data.LdrpInvertedFunctionTable64 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpInvertedFunctionTable64 not found" );
    if (_data.LdrpInvertedFunctionTable32 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpInvertedFunctionTable32 not found" );
    if (IsWindows8Point1OrGreater() && _data.RtlInsertInvertedFunctionTable64 == 0)
        BLACKBONE_TRACE( "PatternData: RtlInsertInvertedFunctionTable64 not found" );
    if (_data.RtlInsertInvertedFunctionTable32 == 0)
        BLACKBONE_TRACE( "PatternData: RtlInsertInvertedFunctionTable32 not found" );
    if (IsWindows8Point1OrGreater() && _data.LdrProtectMrdata == 0)
        BLACKBONE_TRACE( "PatternData: LdrProtectMrdata not found" );
    if (IsWindows7OrGreater() && !IsWindows8OrGreater())
    {
        if (_data.LdrKernel32PatchAddress == 0)
            BLACKBONE_TRACE( "PatternData: LdrKernel32PatchAddress not found" );
        if (_data.APC64PatchAddress == 0)
            BLACKBONE_TRACE( "PatternData: APC64PatchAddress not found" );
    }
#endif
    return _data;
}

/// <summary>
/// Detect process and OS architecture
/// </summary>
void PatternLoader::CheckSystem()
{
    SYSTEM_INFO info = { { 0 } };
    GetNativeSystemInfo( &info );

    if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        _x86OS = true;
    }
    else
    {
        BOOL wowSrc = FALSE;
        IsWow64Process( GetCurrentProcess(), &wowSrc );
        _wow64Process = wowSrc != 0;
    }
}

/// <summary>
/// Fill OS-dependent patterns
/// </summary>
/// <param name="patterns">Pattern collection</param>
void PatternLoader::OSFillPatterns( std::unordered_map<ptr_t*, OffsetData>& patterns )
{
    if (IsWindows10CreatorsOrGreater())
    {
        // LdrpHandleTlsData
        // 74 33 44 8D 43 09
        patterns.emplace( &_data.LdrpHandleTlsData64, OffsetData{ "\x74\x33\x44\x8d\x43\x09", true, 0x43 } );

        // RtlInsertInvertedFunctionTable
        // 8B FA 49 8D 43 20
        patterns.emplace( &_data.RtlInsertInvertedFunctionTable64, OffsetData{ "\x8b\xfa\x49\x8d\x43\x20", true, 0x10 } );

        // RtlpInsertInvertedFunctionTableEntry
        // 49 8B E8 48 8B FA 0F 84
        patterns.emplace( &_data.LdrpInvertedFunctionTable64, OffsetData{ "\x49\x8b\xe8\x48\x8b\xfa\x0f\x84", true, -1, -0xF, 2, 6 } );

        // RtlInsertInvertedFunctionTable
        // 8D 45 F0 89 55 F8 50 8D 55 F4
        patterns.emplace( &_data.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8d\x45\xf0\x89\x55\xf8\x50\x8d\x55\xf4", false, 0xB } );
        patterns.emplace( &_data.LdrpInvertedFunctionTable32, OffsetData{ "\x8d\x45\xf0\x89\x55\xf8\x50\x8d\x55\xf4", false, -1, 0x4C } );

        // LdrpHandleTlsData
        // 8B C1 8D 4D BC 51
        patterns.emplace( &_data.LdrpHandleTlsData32, OffsetData{ "\x8b\xc1\x8d\x4d\xbc\x51", false, 0x18 } );

        // LdrProtectMrdata
        // 75 24 85 F6 75 08
        patterns.emplace( &_data.LdrProtectMrdata, OffsetData{ "\x75\x24\x85\xf6\x75\x08", false, 0x1C } );
    }
    else if (IsWindows8Point1OrGreater())
    {
        // LdrpHandleTlsData
        // 44 8D 43 09 4C 8D 4C 24 38
        patterns.emplace( &_data.LdrpHandleTlsData64, OffsetData{ "\x44\x8d\x43\x09\x4c\x8d\x4c\x24\x38", true, 0x43 } );

        // RtlInsertInvertedFunctionTable
        // 8B C3 2B D3 48 8D 48 01
        patterns.emplace( &_data.RtlInsertInvertedFunctionTable64, OffsetData{ "\x8b\xc3\x2b\xd3\x48\x8d\x48\x01", true, 0x84 } );
        patterns.emplace( &_data.LdrpInvertedFunctionTable64, OffsetData{ "\x8b\xc3\x2b\xd3\x48\x8d\x48\x01", true, -1, -0x27, 3, 7 } );

        // RtlInsertInvertedFunctionTable
        // 53 56 57 8B DA 8B F9 50
        patterns.emplace( &_data.RtlInsertInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8b\xda\x8b\xf9\x50", false, 0xB } );

        if (IsWindows10OrGreater())
            patterns.emplace( &_data.LdrpInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8b\xda\x8b\xf9\x50", false, -1, 0x22 } );
        else
            patterns.emplace( &_data.LdrpInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8b\xda\x8b\xf9\x50", false, -1, 0x23 } );

        // LdrpHandleTlsData
        // 50 6A 09 6A 01 8B C1
        patterns.emplace( &_data.LdrpHandleTlsData32, OffsetData{ "\x50\x6a\x09\x6a\x01\x8b\xc1", false, 0x1B } );

        // LdrProtectMrdata
        // 83 7D 08 00 8B 35
        patterns.emplace( &_data.LdrProtectMrdata, OffsetData{ PatternSearch( "\x83\x7d\x08\x00\x8b\x35", 6 ), false, 0x12 } );
    }
    else if (IsWindows8OrGreater())
    {
        // LdrpHandleTlsData
        // 48 8B 79 30 45 8D 66 01
        patterns.emplace( &_data.LdrpHandleTlsData64, OffsetData{ "\x48\x8b\x79\x30\x45\x8d\x66\x01", true, 0x49 } );

        // RtlInsertInvertedFunctionTable
        // 8B FF 55 8B EC 51 51 53 57 8B 7D 08 8D
        patterns.emplace( &_data.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8b\xff\x55\x8b\xec\x51\x51\x53\x57\x8b\x7d\x08\x8d", false, 0 } );
        patterns.emplace( &_data.LdrpInvertedFunctionTable32, OffsetData{ "\x8b\xff\x55\x8b\xec\x51\x51\x53\x57\x8b\x7d\x08\x8d", false, -1, 0x26 } );

        // LdrpHandleTlsData
        // 8B 45 08 89 45 A0
        patterns.emplace( &_data.LdrpHandleTlsData32, OffsetData{ "\x8b\x45\x08\x89\x45\xa0", false, 0xC } );
    }
    else if (IsWindows7OrGreater())
    {
        // LdrpHandleTlsData
        // 41 B8 09 00 00 00 48 8D 44 24 38
        patterns.emplace( &_data.LdrpHandleTlsData64, OffsetData{ PatternSearch( "\x41\xb8\x09\x00\x00\x00\x48\x8d\x44\x24\x38", 11 ), true, 0x27 } );

        // LdrpFindOrMapDll patch address
        // 48 8D 8C 24 98 00 00 00 41 b0 01
        patterns.emplace( &_data.LdrKernel32PatchAddress, OffsetData{ PatternSearch( "\x48\x8D\x8C\x24\x98\x00\x00\x00\x41\xb0\x01", 11 ), true, -0x12 } );

        // KiUserApcDispatcher patch address
        // 48 8B 4C 24 18 48 8B C1 4C
        patterns.emplace( &_data.APC64PatchAddress, OffsetData{ "\x48\x8b\x4c\x24\x18\x48\x8b\xc1\x4c", true, 0 } );

        // RtlInsertInvertedFunctionTable
        // 8B FF 55 8B EC 56 68
        patterns.emplace( &_data.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8b\xff\x55\x8b\xec\x56\x68", false, 0 } );

        // RtlLookupFunctionTable + 0x11
        // 89 5D E0 38
        patterns.emplace( &_data.LdrpInvertedFunctionTable32, OffsetData{ "\x89\x5D\xE0\x38", false, -1, 0x1B } );

        // LdrpHandleTlsData
        // 74 20 8D 45 D4 50 6A 09 
        patterns.emplace( &_data.LdrpHandleTlsData32, OffsetData{ "\x74\x20\x8d\x45\xd4\x50\x6a\x09", false, 0x14 } );
    }
}

}