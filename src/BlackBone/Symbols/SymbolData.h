#pragma once
#include "../include/Types.h"

namespace blackbone
{

/// <summary>
/// Ntdll internal pointers
/// </summary>
struct SymbolData
{
    ptr_t LdrKernel32PatchAddress = 0;              // Address to patch to enable kernel32 loading under win7
    ptr_t APC64PatchAddress = 0;                    // Address to patch for x64->WOW64 APC dispatching under win7
    ptr_t LdrpHandleTlsData32 = 0;                  // LdrpHandleTlsData address
    ptr_t LdrpHandleTlsData64 = 0;                  // LdrpHandleTlsData address
    ptr_t LdrpInvertedFunctionTable32 = 0;          // LdrpInvertedFunctionTable address
    ptr_t LdrpInvertedFunctionTable64 = 0;          // LdrpInvertedFunctionTable address
    ptr_t RtlInsertInvertedFunctionTable32 = 0;     // RtlInsertInvertedFunctionTable address
    ptr_t RtlInsertInvertedFunctionTable64 = 0;     // RtlInsertInvertedFunctionTable address
    ptr_t LdrpReleaseTlsEntry32 = 0;                // LdrpReleaseTlsEntry address
    ptr_t LdrpReleaseTlsEntry64 = 0;                // LdrpReleaseTlsEntry address
    ptr_t LdrProtectMrdata = 0;                     // LdrProtectMrdata address
};

extern SymbolData g_symbols;

}