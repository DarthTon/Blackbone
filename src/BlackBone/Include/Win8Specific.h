#pragma once

#include "Winheaders.h"

namespace blackbone
{
#pragma warning(disable : 4201)

    typedef struct _RTL_RB_TREE
    {
        struct _RTL_BALANCED_NODE * Root;
        struct _RTL_BALANCED_NODE * Min;
    } RTL_RB_TREE, *PRTL_RB_TREE;

    typedef struct _RTL_BALANCED_NODE
    {
        union
        {
            struct _RTL_BALANCED_NODE * Children[2];
            struct
            {
                struct _RTL_BALANCED_NODE * Left;
                struct _RTL_BALANCED_NODE * Right;
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

            uintptr_t ParentValue;
        };
    } RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;

    struct _LDR_DDAG_NODE
    {
        _LIST_ENTRY Modules;
        struct _LDR_SERVICE_TAG_RECORD * ServiceTagList;
        unsigned long LoadCount;
        unsigned long ReferenceCount;
        unsigned long DependencyCount;
        _SINGLE_LIST_ENTRY RemovalLink;
        void* IncomingDependencies;
        _LDR_DDAG_STATE State;
        struct _SINGLE_LIST_ENTRY CondenseLink;
        unsigned long PreorderNumber;
        unsigned long LowestLink;
    };

    struct _LDR_DATA_TABLE_ENTRY_W8 : LDR_DATA_TABLE_ENTRY_BASE_T
    {
        _LDR_DDAG_NODE * DdagNode;
        _LIST_ENTRY NodeModuleLink;
        struct _LDRP_DLL_SNAP_CONTEXT * SnapContext;
        void * ParentDllBase;
        void * SwitchBackContext;
        _RTL_BALANCED_NODE BaseAddressIndexNode;
        _RTL_BALANCED_NODE MappingInfoIndexNode;
        void * OriginalBase;
        _LARGE_INTEGER LoadTime;
        unsigned long BaseNameHashValue;
        _LDR_DLL_LOAD_REASON LoadReason;
        unsigned long ImplicitPathOptions;
    };

    typedef struct _RTL_INVERTED_FUNCTION_TABLE8
    {
        ULONG Count;
        ULONG MaxCount;
        ULONG Epoch;
        UCHAR Overflow;
        RTL_INVERTED_FUNCTION_TABLE_ENTRY Entries[0x200];

    } RTL_INVERTED_FUNCTION_TABLE8, *PRTL_INVERTED_FUNCTION_TABLE8;

#pragma warning(default : 4201)
}