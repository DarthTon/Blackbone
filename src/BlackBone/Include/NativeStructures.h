#pragma once

#include "../Config.h"
#include "NativeEnums.h"

#include "Winheaders.h"
#include <stdint.h>

#include <type_traits>

namespace blackbone
{
// nonstandard extension used : nameless struct/union
#pragma warning(disable : 4201)

template <typename T>
struct _LIST_ENTRY_T
{
    T Flink;
    T Blink;
};

template <typename T>
struct _UNICODE_STRING_T
{
    using type = T;

    uint16_t Length;
    uint16_t MaximumLength;
    T Buffer;
};

template <typename T>
struct _NT_TIB_T
{
    T ExceptionList;
    T StackBase;
    T StackLimit;
    T SubSystemTib;
    T FiberData;
    T ArbitraryUserPointer;
    T Self;
};

template <typename T>
struct _CLIENT_ID_T
{
    T UniqueProcess;
    T UniqueThread;
};

template <typename T>
struct _GDI_TEB_BATCH_T
{
    uint32_t Offset;
    T HDC;
    uint32_t Buffer[310];
};

template <typename T>
struct _TEB_T
{
    using type = T;

    _NT_TIB_T<T> NtTib;
    T EnvironmentPointer;
    _CLIENT_ID_T<T> ClientId;
    T ActiveRpcHandle;
    T ThreadLocalStoragePointer;
    T ProcessEnvironmentBlock;
    uint32_t LastErrorValue;
    uint32_t CountOfOwnedCriticalSections;
    T CsrClientThread;
    T Win32ThreadInfo;
    uint32_t User32Reserved[26];
    T UserReserved[5];
    T WOW32Reserved;
    uint32_t CurrentLocale;
    uint32_t FpSoftwareStatusRegister;
    T SystemReserved1[54];
    uint32_t ExceptionCode;
    T ActivationContextStackPointer;
    uint8_t SpareBytes[36];
    uint32_t TxFsContext;
    _GDI_TEB_BATCH_T<T> GdiTebBatch;
    _CLIENT_ID_T<T> RealClientId;
    T GdiCachedProcessHandle;
    uint32_t GdiClientPID;
    uint32_t GdiClientTID;
    T GdiThreadLocalInfo;
    T Win32ClientInfo[62];
    T glDispatchTable[233];
    T glReserved1[29];
    T glReserved2;
    T glSectionInfo;
    T glSection;
    T glTable;
    T glCurrentRC;
    T glContext;
    uint32_t LastStatusValue;
    _UNICODE_STRING_T<T> StaticUnicodeString;
    wchar_t StaticUnicodeBuffer[261];
    T DeallocationStack;
    T TlsSlots[64];
    _LIST_ENTRY_T<T> TlsLinks;
    T Vdm;
    T ReservedForNtRpc;
    T DbgSsReserved[2];
    uint32_t HardErrorMode; 
    T Instrumentation[11];
    _GUID ActivityId;
    T SubProcessTag;
    T PerflibData;
    T EtwTraceData;
    T WinSockData;
    uint32_t GdiBatchCount;             // TEB64 pointer
    uint32_t IdealProcessorValue;
    uint32_t GuaranteedStackBytes;
    T ReservedForPerf;
    T ReservedForOle;
    uint32_t WaitingOnLoaderLock;
    T SavedPriorityState;
    T ReservedForCodeCoverage;
    T ThreadPoolData;
    T TlsExpansionSlots;
    T DeallocationBStore;
    T BStoreLimit;
    uint32_t MuiGeneration;
    uint32_t IsImpersonating;
    T NlsCache;
    T pShimData;
    uint16_t HeapVirtualAffinity;
    uint16_t LowFragHeapDataSlot;
    T CurrentTransactionHandle;
    T ActiveFrame;
    T FlsData;
    T PreferredLanguages;
    T UserPrefLanguages;
    T MergedPrefLanguages;
    uint32_t MuiImpersonation;
    uint16_t CrossTebFlags;
    uint16_t SameTebFlags;
    T TxnScopeEnterCallback;
    T TxnScopeExitCallback;
    T TxnScopeContext;
    uint32_t LockCount;
    uint32_t SpareUlong0; 
    T ResourceRetValue;
    T ReservedForWdf;
};

template<typename T>
struct _PEB_T
{
    static_assert(
        std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value, 
        "T must be uint32_t or uint64_t"
        );

    using type = T;
    using const34 = std::integral_constant<int, 34>;
    using const60 = std::integral_constant<int, 60>;
    using is32Bit = std::integral_constant<bool, std::is_same_v<T, uint32_t>>;
    using BufferSize_t = std::conditional_t<is32Bit::value, const34, const60>;

    uint8_t InheritedAddressSpace;
    uint8_t ReadImageFileExecOptions;
    uint8_t BeingDebugged;
    union
    {
        uint8_t BitField;
        struct
        {
            uint8_t ImageUsesLargePages : 1;
            uint8_t IsProtectedProcess : 1;
            uint8_t IsImageDynamicallyRelocated : 1;
            uint8_t SkipPatchingUser32Forwarders : 1;
            uint8_t IsPackagedProcess : 1;
            uint8_t IsAppContainer : 1;
            uint8_t IsProtectedProcessLight : 1;
            uint8_t SpareBits : 1;
        };
    };
    T Mutant;
    T ImageBaseAddress;
    T Ldr;
    T ProcessParameters;
    T SubSystemData;
    T ProcessHeap;
    T FastPebLock;
    T AtlThunkSListPtr;
    T IFEOKey;
    union
    {
        T CrossProcessFlags;
        struct
        {
            uint32_t ProcessInJob : 1;
            uint32_t ProcessInitializing : 1;
            uint32_t ProcessUsingVEH : 1;
            uint32_t ProcessUsingVCH : 1;
            uint32_t ProcessUsingFTH : 1;
            uint32_t ReservedBits0 : 27;
        };
    };
    union
    {
        T KernelCallbackTable;
        T UserSharedInfoPtr;
    };
    uint32_t SystemReserved;
    uint32_t AtlThunkSListPtr32;
    T ApiSetMap;
    union
    {
        uint32_t TlsExpansionCounter;
        T Padding2;
    };
    T TlsBitmap;
    uint32_t TlsBitmapBits[2];
    T ReadOnlySharedMemoryBase;
    T SparePvoid0;
    T ReadOnlyStaticServerData;
    T AnsiCodePageData;
    T OemCodePageData;
    T UnicodeCaseTableData;
    uint32_t NumberOfProcessors;
    uint32_t NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    T HeapSegmentReserve;
    T HeapSegmentCommit;
    T HeapDeCommitTotalFreeThreshold;
    T HeapDeCommitFreeBlockThreshold;
    uint32_t NumberOfHeaps;
    uint32_t MaximumNumberOfHeaps;
    T ProcessHeaps;
    T GdiSharedHandleTable;
    T ProcessStarterHelper;
    union
    {
        uint32_t GdiDCAttributeList;
        T Padding3;
    };
    T LoaderLock;
    uint32_t OSMajorVersion;
    uint32_t OSMinorVersion;
    uint16_t OSBuildNumber;
    uint16_t OSCSDVersion;
    uint32_t OSPlatformId;
    uint32_t ImageSubsystem;
    uint32_t ImageSubsystemMajorVersion;
    union
    {
        uint32_t ImageSubsystemMinorVersion;
        T Padding4;
    };
    T ActiveProcessAffinityMask;
    uint32_t GdiHandleBuffer[BufferSize_t::value];
    T PostProcessInitRoutine;
    T TlsExpansionBitmap;
    uint32_t TlsExpansionBitmapBits[32];
    union
    {
        uint32_t SessionId;
        T Padding5;
    };
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    T pShimData;
    T AppCompatInfo;
    _UNICODE_STRING_T<T> CSDVersion;
    T ActivationContextData;
    T ProcessAssemblyStorageMap;
    T SystemDefaultActivationContextData;
    T SystemAssemblyStorageMap;
    T MinimumStackCommit;
    T FlsCallback;
    _LIST_ENTRY_T<T> FlsListHead;
    T FlsBitmap;
    uint32_t FlsBitmapBits[4];
    uint32_t FlsHighIndex;
    T WerRegistrationData;
    T WerShipAssertPtr;
    T pUnused;
    T pImageHeaderHash;
    union
    {
        uint64_t TracingFlags;
        struct
        {
            uint32_t HeapTracingEnabled : 1;
            uint32_t CritSecTracingEnabled : 1;
            uint32_t LibLoaderTracingEnabled : 1;
            uint32_t SpareTracingBits : 29;
        };
    };
    T CsrServerReadOnlySharedMemoryBase;
};

#pragma warning(default : 4201)

template<typename T>
struct _ACTCTXW_T
{
    uint32_t  cbSize;
    uint32_t  dwFlags;
    T      lpSource;
    uint16_t wProcessorArchitecture;
    LANGID wLangId;
    T      lpAssemblyDirectory;
    T      lpResourceName;
    T      lpApplicationName;
    T      hModule;
};

template<typename T>
struct _PROCESS_BASIC_INFORMATION_T
{
    NTSTATUS ExitStatus;
    uint32_t    Reserved0;
    T	     PebBaseAddress;
    T	     AffinityMask;
    LONG	 BasePriority;
    ULONG	 Reserved1;
    T	     uUniqueProcessId;
    T	     uInheritedFromUniqueProcessId;
};

template<typename T>
struct _SECTION_BASIC_INFORMATION_T 
{
    T             Base;
    uint32_t         Attributes;
    LARGE_INTEGER Size;
};

template<typename T>
struct _PROCESS_EXTENDED_BASIC_INFORMATION_T
{
    T Size;  // Must be set to structure size on input
    _PROCESS_BASIC_INFORMATION_T<T> BasicInfo;
    struct
    {
        uint32_t IsProtectedProcess : 1;
        uint32_t IsWow64Process : 1;
        uint32_t IsProcessDeleting : 1;
        uint32_t IsCrossSessionCreate : 1;
        uint32_t IsFrozen : 1;
        uint32_t IsBackground : 1;
        uint32_t IsStronglyNamed : 1;
        uint32_t SpareBits : 25;
    }Flags;
};

template<typename T>
struct _THREAD_BASIC_INFORMATION_T
{
    NTSTATUS ExitStatus;
    T TebBaseAddress;
    _CLIENT_ID_T<T> ClientID;
    T AffinityMask;
    LONG Priority;
    LONG BasePriority;
};

template<typename T>
struct _VM_COUNTERS_T
{
    T PeakVirtualSize;
    T VirtualSize;
    uint32_t PageFaultCount;
    T PeakWorkingSetSize;
    T WorkingSetSize;
    T QuotaPeakPagedPoolUsage;
    T QuotaPagedPoolUsage;
    T QuotaPeakNonPagedPoolUsage;
    T QuotaNonPagedPoolUsage;
    T PagefileUsage;
    T PeakPagefileUsage;
};

template<typename T>
struct _SYSTEM_THREAD_INFORMATION_T
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    uint32_t WaitTime;
    T StartAddress;
    _CLIENT_ID_T<T> ClientId;
    LONG Priority;
    LONG BasePriority;
    uint32_t ContextSwitches;
    uint32_t ThreadState;
    uint32_t WaitReason;
};

template<typename T>
struct _SYSTEM_EXTENDED_THREAD_INFORMATION_T
{
    _SYSTEM_THREAD_INFORMATION_T<T> ThreadInfo;
    T StackBase;
    T StackLimit;
    T Win32StartAddress;
    T TebBase;
    T Reserved[3];
};

template<typename T>
struct _SYSTEM_PROCESS_INFORMATION_T
{
    uint32_t NextEntryOffset;
    uint32_t NumberOfThreads;
    LARGE_INTEGER WorkingSetPrivateSize;
    uint32_t HardFaultCount;
    uint32_t NumberOfThreadsHighWatermark;
    ULONGLONG CycleTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    _UNICODE_STRING_T<T> ImageName;
    LONG BasePriority;
    T UniqueProcessId;
    T InheritedFromUniqueProcessId;
    uint32_t HandleCount;
    uint32_t SessionId;
    T UniqueProcessKey;
    _VM_COUNTERS_T<T> VmCounters;
    T PrivatePageCount;
    IO_COUNTERS IoCounters;
    _SYSTEM_EXTENDED_THREAD_INFORMATION_T<T> Threads[1];
};

template<typename T>
struct _SYSTEM_HANDLE_T
{
    uint32_t ProcessId;
    uint8_t ObjectTypeNumber;
    uint8_t Flags;
    uint16_t Handle;
    T Object;
    ACCESS_MASK GrantedAccess;
};

template<typename T>
struct _SYSTEM_HANDLE_INFORMATION_T
{
    uint32_t HandleCount;
    _SYSTEM_HANDLE_T<T> Handles[1];
};

template<typename T>
struct _OBJECT_TYPE_INFORMATION_T
{
    _UNICODE_STRING_T<T> Name;
    uint32_t TotalNumberOfObjects;
    uint32_t TotalNumberOfHandles;
    uint32_t TotalPagedPoolUsage;
    uint32_t TotalNonPagedPoolUsage;
    uint32_t TotalNamePoolUsage;
    uint32_t TotalHandleTableUsage;
    uint32_t HighWaterNumberOfObjects;
    uint32_t HighWaterNumberOfHandles;
    uint32_t HighWaterPagedPoolUsage;
    uint32_t HighWaterNonPagedPoolUsage;
    uint32_t HighWaterNamePoolUsage;
    uint32_t HighWaterHandleTableUsage;
    uint32_t InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    uint32_t ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    uint16_t MaintainTypeList;
    POOL_TYPE PoolType;
    uint32_t PagedPoolUsage;
    uint32_t NonPagedPoolUsage;
};

template<typename T>
struct _OBJECT_ATTRIBUTES_T
{
    uint32_t Length;
    T RootDirectory;
    T ObjectName;
    uint32_t Attributes;
    T SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    T SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
};

struct _XSAVE_FORMAT64
{
    uint16_t ControlWord;
    uint16_t StatusWord;
    uint8_t TagWord;
    uint8_t Reserved1;
    uint16_t ErrorOpcode;
    uint32_t ErrorOffset;
    uint16_t ErrorSelector;
    uint16_t Reserved2;
    uint32_t DataOffset;
    uint16_t DataSelector;
    uint16_t Reserved3;
    uint32_t MxCsr;
    uint32_t MxCsr_Mask;
    _M128A FloatRegisters[8];
    _M128A XmmRegisters[16];
    uint8_t Reserved4[96];
};

template<typename T>
struct _CONTEXT_T;

template<>
struct _CONTEXT_T<uint32_t>
{
    uint32_t ContextFlags;
    uint32_t Dr0;
    uint32_t Dr1;
    uint32_t Dr2;
    uint32_t Dr3;
    uint32_t Dr6;
    uint32_t Dr7;
    WOW64_FLOATING_SAVE_AREA FloatSave;
    uint32_t SegGs;
    uint32_t SegFs;
    uint32_t SegEs;
    uint32_t SegDs;
    uint32_t Edi;
    uint32_t Esi;
    uint32_t Ebx;
    uint32_t Edx;
    uint32_t Ecx;
    uint32_t Eax;
    uint32_t Ebp;
    uint32_t Eip;
    uint32_t SegCs;              // MUST BE SANITIZED
    uint32_t EFlags;             // MUST BE SANITIZED
    uint32_t Esp;
    uint32_t SegSs;
    uint8_t  ExtendedRegisters[WOW64_MAXIMUM_SUPPORTED_EXTENSION];
};
    
template<>
struct _CONTEXT_T<uint64_t>
{
    uint64_t P1Home;
    uint64_t P2Home;
    uint64_t P3Home;
    uint64_t P4Home;
    uint64_t P5Home;
    uint64_t P6Home;
    uint32_t ContextFlags;
    uint32_t MxCsr;
    uint16_t SegCs;
    uint16_t SegDs;
    uint16_t SegEs;
    uint16_t SegFs;
    uint16_t SegGs;
    uint16_t SegSs;
    uint32_t EFlags;
    uint64_t Dr0;
    uint64_t Dr1;
    uint64_t Dr2;
    uint64_t Dr3;
    uint64_t Dr6;
    uint64_t Dr7;
    uint64_t Rax;
    uint64_t Rcx;
    uint64_t Rdx;
    uint64_t Rbx;
    uint64_t Rsp;
    uint64_t Rbp;
    uint64_t Rsi;
    uint64_t Rdi;
    uint64_t R8;
    uint64_t R9;
    uint64_t R10;
    uint64_t R11;
    uint64_t R12;
    uint64_t R13;
    uint64_t R14;
    uint64_t R15;
    uint64_t Rip;
    _XSAVE_FORMAT64 FltSave;
    _M128A Header[2];
    _M128A Legacy[8];
    _M128A Xmm0;
    _M128A Xmm1;
    _M128A Xmm2;
    _M128A Xmm3;
    _M128A Xmm4;
    _M128A Xmm5;
    _M128A Xmm6;
    _M128A Xmm7;
    _M128A Xmm8;
    _M128A Xmm9;
    _M128A Xmm10;
    _M128A Xmm11;
    _M128A Xmm12;
    _M128A Xmm13;
    _M128A Xmm14;
    _M128A Xmm15;
    _M128A VectorRegister[26];
    uint64_t VectorControl;
    uint64_t DebugControl;
    uint64_t LastBranchToRip;
    uint64_t LastBranchFromRip;
    uint64_t LastExceptionToRip;
    uint64_t LastExceptionFromRip;

    _CONTEXT_T<uint64_t>& FromCtx32( const _CONTEXT_T<uint32_t>& ctx32 )
    {
        ContextFlags = ctx32.ContextFlags;
        Dr0 = ctx32.Dr0;
        Dr1 = ctx32.Dr1;
        Dr2 = ctx32.Dr2;
        Dr3 = ctx32.Dr3;
        Dr6 = ctx32.Dr6;
        Dr7 = ctx32.Dr7;
        SegGs = static_cast<uint16_t>(ctx32.SegGs);
        SegFs = static_cast<uint16_t>(ctx32.SegFs);
        SegEs = static_cast<uint16_t>(ctx32.SegEs);
        SegDs = static_cast<uint16_t>(ctx32.SegDs);
        SegCs = static_cast<uint16_t>(ctx32.SegCs);
        SegSs = static_cast<uint16_t>(ctx32.SegSs);
        Rdi = ctx32.Edi;
        Rsi = ctx32.Esi;
        Rbx = ctx32.Ebx;
        Rdx = ctx32.Edx;
        Rcx = ctx32.Ecx;
        Rax = ctx32.Eax;
        Rbp = ctx32.Ebp;
        Rip = ctx32.Eip;
        Rsp = ctx32.Esp;
        EFlags = ctx32.EFlags;

        return *this;
    }
};

#ifndef CONTEXT_AMD64
#define CONTEXT_AMD64 0x100000
#endif

#define CONTEXT64_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT64_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT64_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT64_FLOATING_POINT  (CONTEXT_AMD64 | 0x8L)
#define CONTEXT64_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)
#define CONTEXT64_FULL (CONTEXT64_CONTROL | CONTEXT64_INTEGER | CONTEXT64_FLOATING_POINT)
#define CONTEXT64_ALL (CONTEXT64_CONTROL | CONTEXT64_INTEGER | CONTEXT64_SEGMENTS | CONTEXT64_FLOATING_POINT | CONTEXT64_DEBUG_REGISTERS)
#define CONTEXT64_XSTATE (CONTEXT_AMD64 | 0x20L)

template<typename T>
struct _PEB_LDR_DATA2_T
{
    uint32_t Length;
    uint8_t Initialized;
    T SsHandle;
    _LIST_ENTRY_T<T> InLoadOrderModuleList;
    _LIST_ENTRY_T<T> InMemoryOrderModuleList;
    _LIST_ENTRY_T<T> InInitializationOrderModuleList;
    T EntryInProgress;
    uint8_t ShutdownInProgress;
    T ShutdownThreadId;
};

template<typename T>
struct _LDR_DATA_TABLE_ENTRY_BASE_T
{
    _LIST_ENTRY_T<T> InLoadOrderLinks;
    _LIST_ENTRY_T<T> InMemoryOrderLinks;
    _LIST_ENTRY_T<T> InInitializationOrderLinks;
    T DllBase;
    T EntryPoint;
    uint32_t SizeOfImage;
    _UNICODE_STRING_T<T> FullDllName;
    _UNICODE_STRING_T<T> BaseDllName;
    uint32_t Flags;
    uint16_t LoadCount;
    uint16_t TlsIndex;
    _LIST_ENTRY_T<T> HashLinks;
    uint32_t TimeDateStamp;
    T EntryPointActivationContext;
    T PatchInformation;
};
  
template<typename T>
struct _RTL_INVERTED_FUNCTION_TABLE_ENTRY
{
    T     ExceptionDirectory;   // PIMAGE_RUNTIME_FUNCTION_ENTRY
    T     ImageBase;
    uint32_t ImageSize;
    uint32_t SizeOfTable;
};

using _UNICODE_STRING32 = _UNICODE_STRING_T<uint32_t>;
using _UNICODE_STRING64 = _UNICODE_STRING_T<uint64_t>;
using UNICODE_STRING_T  = _UNICODE_STRING_T<uintptr_t>;

using _PEB32 = _PEB_T<uint32_t>;
using _PEB64 = _PEB_T<uint64_t>;
using PEB_T  = _PEB_T<uintptr_t>;

using _TEB32 = _TEB_T<uint32_t>;
using _TEB64 = _TEB_T<uint64_t>;
using TEB_T  = _TEB_T<uintptr_t>;

using _PEB_LDR_DATA232 = _PEB_LDR_DATA2_T<uint32_t>;
using _PEB_LDR_DATA264 = _PEB_LDR_DATA2_T<uint64_t>;
using PEB_LDR_DATA_T   = _PEB_LDR_DATA2_T<uintptr_t>;

using _LDR_DATA_TABLE_ENTRY_BASE32 = _LDR_DATA_TABLE_ENTRY_BASE_T<uint32_t>;
using _LDR_DATA_TABLE_ENTRY_BASE64 = _LDR_DATA_TABLE_ENTRY_BASE_T<uint64_t>;
using LDR_DATA_TABLE_ENTRY_BASE_T  = _LDR_DATA_TABLE_ENTRY_BASE_T<uintptr_t>;

using _CONTEXT32 = _CONTEXT_T<uint32_t>;
using _CONTEXT64 = _CONTEXT_T<uint64_t>;
using CONTEXT_T  = _CONTEXT_T<uintptr_t>;

using _SECTION_BASIC_INFORMATION32 = _SECTION_BASIC_INFORMATION_T<uint32_t>;
using _SECTION_BASIC_INFORMATION64 = _SECTION_BASIC_INFORMATION_T<uint64_t>;
using SECTION_BASIC_INFORMATION_T  = _SECTION_BASIC_INFORMATION_T<uintptr_t>;

using _SYSTEM_HANDLE_INFORMATION32 = _SYSTEM_HANDLE_INFORMATION_T<uint32_t>;
using _SYSTEM_HANDLE_INFORMATION64 = _SYSTEM_HANDLE_INFORMATION_T<uint64_t>;
using SYSTEM_HANDLE_INFORMATION_T = _SYSTEM_HANDLE_INFORMATION_T<uintptr_t>;

using _OBJECT_TYPE_INFORMATION32 = _OBJECT_TYPE_INFORMATION_T<uint32_t>;
using _OBJECT_TYPE_INFORMATION64 = _OBJECT_TYPE_INFORMATION_T<uint64_t>;
using OBJECT_TYPE_INFORMATION_T  = _OBJECT_TYPE_INFORMATION_T<uintptr_t>;

using _OBJECT_ATTRIBUTES32 = _OBJECT_ATTRIBUTES_T<uint32_t>;
using _OBJECT_ATTRIBUTES64 = _OBJECT_ATTRIBUTES_T<uint64_t>;
using OBJECT_ATTRIBUTES_T  = _OBJECT_ATTRIBUTES_T<uintptr_t>;

using _ACTCTXW32 = _ACTCTXW_T<uint32_t>;
using _ACTCTXW64 = _ACTCTXW_T<uint64_t>;
using ACTCTXW_T  = _ACTCTXW_T<uintptr_t>;
}

#include "ApiSet.h"

// OS specific structures
#include "Win7Specific.h"
#include "Win8Specific.h"

#ifdef XP_BUILD
#include "WinXPSpecific.h"
#endif
