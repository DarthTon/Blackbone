#pragma once

#include "Winheaders.h"

namespace blackbone
{
#pragma warning(disable : 4201)

    template<typename T>
    struct _RTL_RB_TREE
    {
        T Root;
        T Min;
    };

    template<typename T>
    struct _RTL_BALANCED_NODE
    {
        union
        {
            T Children[2];
            struct
            {
                T Left;
                T Right;
            };
        };
        union
        {
            union
            {
                struct
                {
                    unsigned char Red : 1;
                };
                struct
                {
                    unsigned char Balance : 2;
                };
            };

            T ParentValue;
        };
    };

    template<typename T>
    struct _LDR_DDAG_NODE
    {
        _LIST_ENTRY_T<T> Modules;
        T ServiceTagList;
        ULONG LoadCount;
        ULONG ReferenceCount;
        ULONG DependencyCount;
        T RemovalLink;
        T IncomingDependencies;
        _LDR_DDAG_STATE State;
        T CondenseLink;
        ULONG PreorderNumber;
        ULONG LowestLink;
    };

    template<typename T>
    struct _LDR_DATA_TABLE_ENTRY_W8 : _LDR_DATA_TABLE_ENTRY_BASE<T>
    {
        T DdagNode;                                     // _LDR_DDAG_NODE*
        _LIST_ENTRY_T<T> NodeModuleLink;
        T SnapContext;
        T ParentDllBase;
        T SwitchBackContext;
        _RTL_BALANCED_NODE<T> BaseAddressIndexNode;
        _RTL_BALANCED_NODE<T> MappingInfoIndexNode;
        T OriginalBase;
        _LARGE_INTEGER LoadTime;
        ULONG BaseNameHashValue;
        _LDR_DLL_LOAD_REASON LoadReason;
        ULONG ImplicitPathOptions;
    };

    template<typename T>
    struct _RTL_INVERTED_FUNCTION_TABLE8
    {
        ULONG Count;
        ULONG MaxCount;
        ULONG Epoch;
        UCHAR Overflow;
        _RTL_INVERTED_FUNCTION_TABLE_ENTRY<T> Entries[0x200];
    };

    typedef _LDR_DATA_TABLE_ENTRY_W8<DWORD>     _LDR_DATA_TABLE_ENTRY_W832;
    typedef _LDR_DATA_TABLE_ENTRY_W8<DWORD64>   _LDR_DATA_TABLE_ENTRY_W864;
    typedef _LDR_DATA_TABLE_ENTRY_W8<DWORD_PTR> LDR_DATA_TABLE_ENTRY_W8T;

    typedef _LDR_DDAG_NODE<DWORD>               _LDR_DDAG_NODE_32;
    typedef _LDR_DDAG_NODE<DWORD64>             _LDR_DDAG_NODE_64;
    typedef _LDR_DDAG_NODE<DWORD_PTR>           LDR_DDAG_NODE_T;

#pragma warning(default : 4201)
}