#pragma once

//
// Native structures W8 x64
//
#pragma warning(disable : 4214 4201)

struct _MMVAD_FLAGS // Size=4
{
    unsigned long VadType: 3; // Size=4 Offset=0 BitOffset=0 BitCount=3
    unsigned long Protection: 5; // Size=4 Offset=0 BitOffset=3 BitCount=5
    unsigned long PreferredNode: 6; // Size=4 Offset=0 BitOffset=8 BitCount=6
    unsigned long NoChange: 1; // Size=4 Offset=0 BitOffset=14 BitCount=1
    unsigned long PrivateMemory: 1; // Size=4 Offset=0 BitOffset=15 BitCount=1
    unsigned long Teb: 1; // Size=4 Offset=0 BitOffset=16 BitCount=1
    unsigned long PrivateFixup: 1; // Size=4 Offset=0 BitOffset=17 BitCount=1
    unsigned long Spare: 13; // Size=4 Offset=0 BitOffset=18 BitCount=13
    unsigned long DeleteInProgress: 1; // Size=4 Offset=0 BitOffset=31 BitCount=1
};

struct _MMVAD_FLAGS1 // Size=4
{
    unsigned long CommitCharge: 31; // Size=4 Offset=0 BitOffset=0 BitCount=31
    unsigned long MemCommit: 1; // Size=4 Offset=0 BitOffset=31 BitCount=1
};

struct _MMVAD_FLAGS2 // Size=4
{
    unsigned long FileOffset: 24; // Size=4 Offset=0 BitOffset=0 BitCount=24
    unsigned long Large: 1; // Size=4 Offset=0 BitOffset=24 BitCount=1
    unsigned long TrimBehind: 1; // Size=4 Offset=0 BitOffset=25 BitCount=1
    unsigned long Inherit: 1; // Size=4 Offset=0 BitOffset=26 BitCount=1
    unsigned long CopyOnWrite: 1; // Size=4 Offset=0 BitOffset=27 BitCount=1
    unsigned long NoValidationNeeded: 1; // Size=4 Offset=0 BitOffset=28 BitCount=1
    unsigned long Spare: 3; // Size=4 Offset=0 BitOffset=29 BitCount=3
};

struct _MMVAD_FLAGS3 // Size=8
{
    unsigned __int64 PreferredNode: 6; // Size=8 Offset=0 BitOffset=0 BitCount=6
    unsigned __int64 Teb: 1; // Size=8 Offset=0 BitOffset=6 BitCount=1
    unsigned __int64 Spare: 1; // Size=8 Offset=0 BitOffset=7 BitCount=1
    unsigned __int64 SequentialAccess: 1; // Size=8 Offset=0 BitOffset=8 BitCount=1
    unsigned __int64 LastSequentialTrim: 15; // Size=8 Offset=0 BitOffset=9 BitCount=15
    unsigned __int64 Spare2: 8; // Size=8 Offset=0 BitOffset=24 BitCount=8
    unsigned __int64 LargePageCreating: 1; // Size=8 Offset=0 BitOffset=32 BitCount=1
    unsigned __int64 Spare3: 31; // Size=8 Offset=0 BitOffset=33 BitCount=31
};

struct _MMSECURE_FLAGS // Size=4
{
    unsigned long ReadOnly: 1; // Size=4 Offset=0 BitOffset=0 BitCount=1
    unsigned long NoWrite: 1; // Size=4 Offset=0 BitOffset=1 BitCount=1
    unsigned long Spare: 10; // Size=4 Offset=0 BitOffset=2 BitCount=10
};

struct _MI_VAD_SEQUENTIAL_INFO // Size=8
{
    unsigned __int64 Length: 12; // Size=8 Offset=0 BitOffset=0 BitCount=12
    unsigned __int64 Vpn: 52; // Size=8 Offset=0 BitOffset=12 BitCount=52
};

union ___unnamed1666 // Size=8
{
    struct
    {
        __int64 Balance: 2; // Size=8 Offset=0 BitOffset=0 BitCount=2
    };
    struct _MM_AVL_NODE * Parent; // Size=8 Offset=0
};

union ___unnamed1784 // Size=4
{
    unsigned long LongFlags; // Size=4 Offset=0
    struct _MMVAD_FLAGS VadFlags; // Size=4 Offset=0
};
union ___unnamed1785 // Size=4
{
    unsigned long LongFlags1; // Size=4 Offset=0
    struct _MMVAD_FLAGS1 VadFlags1; // Size=4 Offset=0
};

union ___unnamed1883 // Size=4
{
    unsigned long LongFlags2; // Size=4 Offset=0
    struct _MMVAD_FLAGS2 VadFlags2; // Size=4 Offset=0
};

union ___unnamed1885 // Size=8
{
    struct _MI_VAD_SEQUENTIAL_INFO SequentialVa; // Size=8 Offset=0
    struct _MMEXTEND_INFO * ExtendedInfo; // Size=8 Offset=0
};

typedef struct _MM_AVL_NODE // Size=24
{
    union ___unnamed1666 u1; // Size=8 Offset=0
    struct _MM_AVL_NODE * LeftChild; // Size=8 Offset=8
    struct _MM_AVL_NODE * RightChild; // Size=8 Offset=16
} MM_AVL_NODE, *PMM_AVL_NODE, *PMMADDRESS_NODE;

typedef struct _MM_AVL_TABLE // Size=48
{
    struct _MM_AVL_NODE BalancedRoot; // Size=24 Offset=0
    struct
    {
        unsigned __int64 DepthOfTree: 5; // Size=8 Offset=24 BitOffset=0 BitCount=5
        unsigned __int64 TableType: 3; // Size=8 Offset=24 BitOffset=5 BitCount=3
        unsigned __int64 NumberGenericTableElements: 56; // Size=8 Offset=24 BitOffset=8 BitCount=56
    };
    void * NodeHint; // Size=8 Offset=32
    void * NodeFreeHint; // Size=8 Offset=40
} MM_AVL_TABLE, *PMM_AVL_TABLE;

typedef struct _MMVAD_SHORT // Size=64
{
    struct _MM_AVL_NODE VadNode; // Size=24 Offset=0
    unsigned long StartingVpn; // Size=4 Offset=24
    unsigned long EndingVpn; // Size=4 Offset=28
    void * PushLock; // Size=8 Offset=32
    union ___unnamed1784 u; // Size=4 Offset=40
    union ___unnamed1785 u1; // Size=4 Offset=44
    struct _MI_VAD_EVENT_BLOCK * EventList; // Size=8 Offset=48
    long ReferenceCount; // Size=4 Offset=56
} MMVAD_SHORT, *PMMVAD_SHORT;

typedef struct _MMVAD // Size=128
{
    struct _MMVAD_SHORT Core; // Size=64 Offset=0
    union ___unnamed1883 u2; // Size=4 Offset=64
    struct _SUBSECTION * Subsection; // Size=8 Offset=72
    struct _MMPTE * FirstPrototypePte; // Size=8 Offset=80
    struct _MMPTE * LastContiguousPte; // Size=8 Offset=88
    struct _LIST_ENTRY ViewLinks; // Size=16 Offset=96
    struct _EPROCESS * VadsProcess; // Size=8 Offset=112
    union ___unnamed1885 u4; // Size=8 Offset=120
} MMVAD, *PMMVAD;

typedef struct _HANDLE_TABLE
{
    ULONG NextHandleNeedingPool;
    long ExtraInfoPages;
    ULONG_PTR TableCode;
    struct _EPROCESS * QuotaProcess;
    LIST_ENTRY HandleTableList;
    ULONG UniqueProcessId;
    ULONG Flags;
    EX_PUSH_LOCK HandleContentionEvent;
    EX_PUSH_LOCK HandleTableLock;
    // More fields here...
} HANDLE_TABLE, *PHANDLE_TABLE;

#pragma warning(default : 4214 4201)

#define GET_VAD_ROOT(Table) (Table->BalancedRoot.RightChild)