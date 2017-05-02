#pragma once

#include "Winheaders.h"

namespace blackbone
{
#pragma warning(disable : 4201)

    struct _LDR_DATA_TABLE_ENTRY_W7 : LDR_DATA_TABLE_ENTRY_BASE_T
    {
        _LIST_ENTRY ForwarderLinks;
        _LIST_ENTRY ServiceTagLinks;
        _LIST_ENTRY StaticLinks;
        void * ContextInformation;
        unsigned long OriginalBase;
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