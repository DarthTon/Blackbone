#pragma once

#include "NativeStructures.h"
#include "FunctionTypes.h"

#include <stdint.h>
#include <string>
#include <memory>

namespace blackbone
{

using ptr_t = uint64_t;     // Generic pointer in remote process
using module_t = ptr_t;  // Module base pointer

// Type of barrier
enum eBarrier
{
    wow_32_32 = 0,  // Both processes are WoW64 
    wow_64_64,      // Both processes are x64
    wow_32_64,      // Managing x64 process from WoW64 process
    wow_64_32,      // Managing WOW64 process from x64 process
};

struct Wow64Barrier
{
    eBarrier type = wow_32_32;
    bool sourceWow64 = false;
    bool targetWow64 = false;
    bool x86OS = false;
    bool mismatch = false;
};

// Module type
enum eModType
{
    mt_mod32,       // 32 bit module
    mt_mod64,       // 64 bit module
    mt_default,     // type is deduced from target process
    mt_unknown      // Failed to detect type
};

// Module search method
enum eModSeachType
{
    LdrList,        // InLoadOrder list
    Sections,       // Scan for section objects
    PEHeaders,      // Scan for PE headers in memory
};

// Switch created wow64 thread to long mode
enum eThreadModeSwitch
{
    NoSwitch,       // Never switch
    ForceSwitch,    // Always switch
    AutoSwitch      // Switch depending on wow64 barrier
};

// Module info
struct ModuleData
{
    module_t baseAddress;   // Base image address
    std::wstring name;      // File name
    std::wstring fullPath;  // Full file path
    uint32_t size;          // Size of image
    eModType type;          // Module type
    ptr_t ldrPtr;           // LDR_DATA_TABLE_ENTRY_BASE_T address
    bool manual;            // Image is manually mapped


    bool operator ==(const ModuleData& other) const
    {
        return (baseAddress == other.baseAddress);
    }

    bool operator <(const ModuleData& other)
    {
        return baseAddress < other.baseAddress;
    }
};

using ModuleDataPtr = std::shared_ptr<const ModuleData>;

}
