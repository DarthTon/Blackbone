#pragma once

#include "Winheaders.h"

namespace blackbone
{

template<typename T>
struct _RTL_RB_TREE
{
    T Root;
    T Min;
};

template<typename T>
struct _RTL_BALANCED_NODE
{
    T Left;
    T Right;
    T ParentValue;
};

template<typename T>
struct _LDR_DDAG_NODE
{
    _LIST_ENTRY_T<T> Modules;
    T ServiceTagList;
    uint32_t LoadCount;
    uint32_t ReferenceCount;
    uint32_t DependencyCount;
    T RemovalLink;
    T IncomingDependencies;
    _LDR_DDAG_STATE State;
    T CondenseLink;
    uint32_t PreorderNumber;
    uint32_t LowestLink;
};

template<typename T>
struct _LDR_DATA_TABLE_ENTRY_W8 : _LDR_DATA_TABLE_ENTRY_BASE_T<T>
{
    T DdagNode;                                     // _LDR_DDAG_NODE*
    _LIST_ENTRY_T<T> NodeModuleLink;
    T SnapContext;
    T ParentDllBase;
    T SwitchBackContext;
    _RTL_BALANCED_NODE<T> BaseAddressIndexNode;
    _RTL_BALANCED_NODE<T> MappingInfoIndexNode;
    T OriginalBase;
    LARGE_INTEGER LoadTime;
    uint32_t BaseNameHashValue;
    _LDR_DLL_LOAD_REASON LoadReason;
    uint32_t ImplicitPathOptions;
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

using _LDR_DATA_TABLE_ENTRY_W832 = _LDR_DATA_TABLE_ENTRY_W8<uint32_t>;
using _LDR_DATA_TABLE_ENTRY_W864 = _LDR_DATA_TABLE_ENTRY_W8<uint64_t>;
using LDR_DATA_TABLE_ENTRY_W8T = _LDR_DATA_TABLE_ENTRY_W8<uintptr_t>;

using _LDR_DDAG_NODE_32 = _LDR_DDAG_NODE<uint32_t>;
using _LDR_DDAG_NODE_64 = _LDR_DDAG_NODE<uint64_t>;
using LDR_DDAG_NODE_T = _LDR_DDAG_NODE<uintptr_t>;

}