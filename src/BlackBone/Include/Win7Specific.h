#pragma once

#include "Winheaders.h"

namespace blackbone
{
template<typename T>
struct _LDR_DATA_TABLE_ENTRY_W7 : _LDR_DATA_TABLE_ENTRY_BASE_T<T>
{
    _LIST_ENTRY_T<T> ForwarderLinks;
    _LIST_ENTRY_T<T> ServiceTagLinks;
    _LIST_ENTRY_T<T> StaticLinks;
    T ContextInformation;
    uint32_t OriginalBase;
    LARGE_INTEGER LoadTime;
};

template<typename T>
struct _RTL_INVERTED_FUNCTION_TABLE7
{
    uint32_t Count;
    uint32_t MaxCount;
    uint32_t Epoch;
    _RTL_INVERTED_FUNCTION_TABLE_ENTRY<T> Entries[0x200];
};

}