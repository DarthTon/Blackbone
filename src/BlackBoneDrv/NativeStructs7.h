#pragma once

//
// Native structures W7 x64 SP1
//
#pragma warning(disable : 4214 4201)

struct _MMVAD_FLAGS // Size=8
{
    unsigned __int64 CommitCharge: 51; // Size=8 Offset=0 BitOffset=0 BitCount=51
    unsigned __int64 NoChange: 1; // Size=8 Offset=0 BitOffset=51 BitCount=1
    unsigned __int64 VadType: 3; // Size=8 Offset=0 BitOffset=52 BitCount=3
    unsigned __int64 MemCommit: 1; // Size=8 Offset=0 BitOffset=55 BitCount=1
    unsigned __int64 Protection: 5; // Size=8 Offset=0 BitOffset=56 BitCount=5
    unsigned __int64 Spare: 2; // Size=8 Offset=0 BitOffset=61 BitCount=2
    unsigned __int64 PrivateMemory: 1; // Size=8 Offset=0 BitOffset=63 BitCount=1
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

struct _MMVAD_FLAGS2 // Size=4
{
    unsigned int FileOffset: 24; // Size=4 Offset=0 BitOffset=0 BitCount=24
    unsigned int SecNoChange: 1; // Size=4 Offset=0 BitOffset=24 BitCount=1
    unsigned int OneSecured: 1; // Size=4 Offset=0 BitOffset=25 BitCount=1
    unsigned int MultipleSecured: 1; // Size=4 Offset=0 BitOffset=26 BitCount=1
    unsigned int Spare: 1; // Size=4 Offset=0 BitOffset=27 BitCount=1
    unsigned int LongVad: 1; // Size=4 Offset=0 BitOffset=28 BitCount=1
    unsigned int ExtendableFile: 1; // Size=4 Offset=0 BitOffset=29 BitCount=1
    unsigned int Inherit: 1; // Size=4 Offset=0 BitOffset=30 BitCount=1
    unsigned int CopyOnWrite: 1; // Size=4 Offset=0 BitOffset=31 BitCount=1
};

struct _MMSECURE_FLAGS // Size=4
{
    unsigned long ReadOnly: 1; // Size=4 Offset=0 BitOffset=0 BitCount=1
    unsigned long NoWrite: 1; // Size=4 Offset=0 BitOffset=1 BitCount=1
    unsigned long Spare: 10; // Size=4 Offset=0 BitOffset=2 BitCount=10
};

union ___unnamed710 // Size=8
{
    struct
    {
        __int64 Balance: 2; // Size=8 Offset=0 BitOffset=0 BitCount=2
    };
    struct _MMADDRESS_NODE * Parent; // Size=8 Offset=0
};

union ___unnamed712 // Size=8
{
    unsigned __int64 LongFlags; // Size=8 Offset=0
    struct _MMVAD_FLAGS VadFlags; // Size=8 Offset=0
};
union ___unnamed713 // Size=8
{
    unsigned __int64 LongFlags3; // Size=8 Offset=0
    struct _MMVAD_FLAGS3 VadFlags3; // Size=8 Offset=0
};

union ___unnamed715 // Size=4
{
    unsigned long LongFlags2; // Size=4 Offset=0
    struct _MMVAD_FLAGS2 VadFlags2; // Size=4 Offset=0
};

union ___unnamed1322 // Size=8
{
    struct _MMSECURE_FLAGS Flags; // Size=4 Offset=0
    void * StartVa; // Size=8 Offset=0
};

struct _MMADDRESS_LIST // Size=16
{
    union ___unnamed1322 u1; // Size=8 Offset=0
    void * EndVa; // Size=8 Offset=8
};

union ___unnamed1319 // Size=16
{
    struct _LIST_ENTRY List; // Size=16 Offset=0
    struct _MMADDRESS_LIST Secured; // Size=16 Offset=0
};

union ___unnamed1320 // Size=8
{
    struct _MMBANKED_SECTION * Banked; // Size=8 Offset=0
    struct _MMEXTEND_INFO * ExtendedInfo; // Size=8 Offset=0
};

typedef struct _MMADDRESS_NODE // Size=40
{
    union ___unnamed710 u1;
    struct _MMADDRESS_NODE * LeftChild; // Size=8 Offset=8
    struct _MMADDRESS_NODE * RightChild; // Size=8 Offset=16
    unsigned __int64 StartingVpn; // Size=8 Offset=24
    unsigned __int64 EndingVpn; // Size=8 Offset=32

} MMADDRESS_NODE, *PMMADDRESS_NODE, *PMM_AVL_NODE;

typedef struct _MM_AVL_TABLE // Size=64
{
    struct _MMADDRESS_NODE BalancedRoot; // Size=40 Offset=0
    struct
    {
        unsigned __int64 DepthOfTree: 5; // Size=8 Offset=40 BitOffset=0 BitCount=5
        unsigned __int64 Unused: 3; // Size=8 Offset=40 BitOffset=5 BitCount=3
        unsigned __int64 NumberGenericTableElements: 56; // Size=8 Offset=40 BitOffset=8 BitCount=56
    };
    void * NodeHint; // Size=8 Offset=48
    void * NodeFreeHint; // Size=8 Offset=56

} MM_AVL_TABLE, *PMM_AVL_TABLE;

typedef struct _MMVAD_SHORT // Size=64
{
    union ___unnamed710 u1; // Size=8 Offset=0
    struct _MMVAD * LeftChild; // Size=8 Offset=8
    struct _MMVAD * RightChild; // Size=8 Offset=16
    unsigned __int64 StartingVpn; // Size=8 Offset=24
    unsigned __int64 EndingVpn; // Size=8 Offset=32
    union ___unnamed712 u; // Size=8 Offset=40
    void * PushLock; // Size=8 Offset=48
    union ___unnamed713 u5; // Size=8 Offset=56
} MMVAD_SHORT, *PMMVAD_SHORT;

typedef struct _MMVAD // Size=120
{
    MMVAD_SHORT vadShort;
    union ___unnamed715 u2; // Size=4 Offset=64
    unsigned long pad0; // Size=4 Offset=68
    struct _SUBSECTION * Subsection; // Size=8 Offset=72
    struct _MMPTE * FirstPrototypePte; // Size=8 Offset=80
    struct _MMPTE * LastContiguousPte; // Size=8 Offset=88
    struct _LIST_ENTRY ViewLinks; // Size=16 Offset=96
    struct _EPROCESS * VadsProcess; // Size=8 Offset=112
} MMVAD, *PMMVAD;

typedef struct _MMVAD_LONG // Size=144
{
    MMVAD vad;
    union ___unnamed1319 u3; // Size=16 Offset=120
    union ___unnamed1320 u4; // Size=8 Offset=136
} MMVAD_LONG, *PMMVAD_LONG;

typedef struct _POOL_HEADER // Size=16
{
    union
    {
        struct
        {
            unsigned long PreviousSize: 8; // Size=4 Offset=0 BitOffset=0 BitCount=8
            unsigned long PoolIndex: 8; // Size=4 Offset=0 BitOffset=8 BitCount=8
            unsigned long BlockSize: 8; // Size=4 Offset=0 BitOffset=16 BitCount=8
            unsigned long PoolType: 8; // Size=4 Offset=0 BitOffset=24 BitCount=8
        };
        unsigned long Ulong1; // Size=4 Offset=0
    };
    unsigned long PoolTag; // Size=4 Offset=4
    union
    {
        struct _EPROCESS * ProcessBilled; // Size=8 Offset=8
        struct
        {
            unsigned short AllocatorBackTraceIndex; // Size=2 Offset=8
            unsigned short PoolTagHash; // Size=2 Offset=10
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

typedef struct _HANDLE_TABLE
{
    ULONG_PTR TableCode;
    struct _EPROCESS *QuotaProcess;
    HANDLE UniqueProcessId;
    void* HandleLock;
    struct _LIST_ENTRY HandleTableList;
    EX_PUSH_LOCK HandleContentionEvent;
    struct _HANDLE_TRACE_DEBUG_INFO *DebugInfo;
    int ExtraInfoPages;
    ULONG Flags;
    ULONG FirstFreeHandle;
    struct _HANDLE_TABLE_ENTRY *LastFreeHandleEntry;
    ULONG HandleCount;
    ULONG NextHandleNeedingPool;
    // More fields here...
} HANDLE_TABLE, *PHANDLE_TABLE;

#pragma warning(default : 4214 4201)

#define GET_VAD_ROOT(Table) &Table->BalancedRoot