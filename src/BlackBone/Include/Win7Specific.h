#pragma once

#include "Winheaders.h"

namespace blackbone
{
#pragma warning(disable : 4201)

    template<typename T>
    struct _LDR_DATA_TABLE_ENTRY_W7 : LDR_DATA_TABLE_ENTRY_BASE_T
    {
        _LIST_ENTRY_T<T> ForwarderLinks;
        _LIST_ENTRY_T<T> ServiceTagLinks;
        _LIST_ENTRY_T<T> StaticLinks;
        T ContextInformation;
        ULONG OriginalBase;
        _LARGE_INTEGER LoadTime;
    };

    template<typename T>
    struct _RTL_INVERTED_FUNCTION_TABLE7
    {
        ULONG Count;
        ULONG MaxCount;
        ULONG Epoch;
        _RTL_INVERTED_FUNCTION_TABLE_ENTRY<T> Entries[0x200];
    };

#pragma warning(default : 4201)
}