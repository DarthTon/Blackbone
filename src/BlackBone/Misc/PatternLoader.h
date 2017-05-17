#pragma once
#include "../Include/Types.h"
#include "../Patterns/PatternSearch.h"

#include <memory>
#include <unordered_map>

namespace blackbone
{

/// <summary>
/// Ntdll internal pointers
/// </summary>
struct PatternData
{
    ptr_t LdrKernel32PatchAddress = 0;              // Address to patch to enable kernel32 loading under win7
    ptr_t APC64PatchAddress = 0;                    // Address to patch for x64->WOW64 APC dispatching under win7
    ptr_t LdrpHandleTlsData32 = 0;                  // LdrpHandleTlsData address
    ptr_t LdrpHandleTlsData64 = 0;                  // LdrpHandleTlsData address
    ptr_t LdrpInvertedFunctionTable32 = 0;          // LdrpInvertedFunctionTable address
    ptr_t LdrpInvertedFunctionTable64 = 0;          // LdrpInvertedFunctionTable address
    ptr_t RtlInsertInvertedFunctionTable32 = 0;     // RtlInsertInvertedFunctionTable address
    ptr_t RtlInsertInvertedFunctionTable64 = 0;     // RtlInsertInvertedFunctionTable address
    ptr_t LdrProtectMrdata = 0;                     // LdrProtectMrdata address
};

/// <summary>
/// Ntdll scanner
/// </summary>
class PatternLoader
{
    struct OffsetData
    {
        PatternSearch pattern;
        bool bit64;
        int32_t functionOffset;
        int32_t dataStartOffset;
        int32_t dataOperandOffset;
        int32_t dataInstructionSize;
    };

public:
    /// <summary>
    /// Scan ntdll for internal loader data
    /// </summary>
    /// <returns>Found data</returns>
    const PatternData& DoSearch();

    /// <summary>
    /// Get internal loader data
    /// </summary>
    /// <returns>Internal loader data</returns>
    inline const PatternData& data() const { return _data; }
private:
    /// <summary>
    /// Detect process and OS architecture
    /// </summary>
    void CheckSystem();

    /// <summary>
    /// Fill OS-dependent patterns
    /// </summary>
    /// <param name="patterns">Pattern collection</param>
    void OSFillPatterns( std::unordered_map<ptr_t*, OffsetData>& patterns );

private:
    bool _x86OS = false;            // x86 OS
    bool _wow64Process = false;     // Current process is wow64 process
    PatternData _data;              // Ntdll internal loader data
};

extern std::unique_ptr<PatternLoader> g_PatternLoader;

}