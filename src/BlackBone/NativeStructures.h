#pragma once

#include "Winheaders.h"

namespace blackbone
{

    enum MEMORY_INFORMATION_CLASS
    {
        MemoryBasicInformation = 0,
        MemoryWorkingSetList,
        MemorySectionName,
        MemoryBasicVlmInformation,
        MemoryWorkingSetExList
    };

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
        union
        {
            struct
            {
                WORD Length;
                WORD MaximumLength;
            };
            T dummy;
        };
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
        DWORD Offset;
        T HDC;
        DWORD Buffer[310];
    };

    template <typename T>
    struct _TEB_T
    {
        typedef T type;

        _NT_TIB_T<T> NtTib;
        T EnvironmentPointer;
        _CLIENT_ID_T<T> ClientId;
        T ActiveRpcHandle;
        T ThreadLocalStoragePointer;
        T ProcessEnvironmentBlock;
        DWORD LastErrorValue;
        DWORD CountOfOwnedCriticalSections;
        T CsrClientThread;
        T Win32ThreadInfo;
        DWORD User32Reserved[26];
        T UserReserved[5];
        T WOW32Reserved;
        DWORD CurrentLocale;
        DWORD FpSoftwareStatusRegister;
        T SystemReserved1[54];
        DWORD ExceptionCode;
        T ActivationContextStackPointer;
        BYTE SpareBytes[36];
        DWORD TxFsContext;
        _GDI_TEB_BATCH_T<T> GdiTebBatch;
        _CLIENT_ID_T<T> RealClientId;
        T GdiCachedProcessHandle;
        DWORD GdiClientPID;
        DWORD GdiClientTID;
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
        DWORD LastStatusValue;
        _UNICODE_STRING_T<T> StaticUnicodeString;
        wchar_t StaticUnicodeBuffer[261];
        T DeallocationStack;
        T TlsSlots[64];
        _LIST_ENTRY_T<T> TlsLinks;
        T Vdm;
        T ReservedForNtRpc;
        T DbgSsReserved[2];
        DWORD HardErrorMode; 
        T Instrumentation[11];
        _GUID ActivityId;
        T SubProcessTag;
        T PerflibData;
        T EtwTraceData;
        T WinSockData;
        DWORD GdiBatchCount;
        DWORD IdealProcessorValue;
        DWORD GuaranteedStackBytes;
        T ReservedForPerf;
        T ReservedForOle;
        DWORD WaitingOnLoaderLock;
        T SavedPriorityState;
        T ReservedForCodeCoverage;
        T ThreadPoolData;
        T TlsExpansionSlots;
        T DeallocationBStore;
        T BStoreLimit;
        DWORD MuiGeneration;
        DWORD IsImpersonating;
        T NlsCache;
        T pShimData;
        USHORT HeapVirtualAffinity;
        USHORT LowFragHeapDataSlot;
        T CurrentTransactionHandle;
        T ActiveFrame;
        T FlsData;
        T PreferredLanguages;
        T UserPrefLanguages;
        T MergedPrefLanguages;
        DWORD MuiImpersonation;
        USHORT CrossTebFlags;
        USHORT SameTebFlags;
        T TxnScopeEnterCallback;
        T TxnScopeExitCallback;
        T TxnScopeContext;
        DWORD LockCount;
        DWORD SpareUlong0; 
        T ResourceRetValue;
        T ReservedForWdf;
    };

    template <typename T, typename NGF, int A>
    struct _PEB_T
    {
        typedef T type;

        union
        {
            struct
            {
                BYTE InheritedAddressSpace;
                BYTE ReadImageFileExecOptions;
                BYTE BeingDebugged;
                BYTE BitField;
            };
            T dummy01;
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
        T CrossProcessFlags;
        T UserSharedInfoPtr;
        DWORD SystemReserved;
        DWORD AtlThunkSListPtr32;
        T ApiSetMap;
        T TlsExpansionCounter;
        T TlsBitmap;
        DWORD TlsBitmapBits[2];
        T ReadOnlySharedMemoryBase;
        T HotpatchInformation;
        T ReadOnlyStaticServerData;
        T AnsiCodePageData;
        T OemCodePageData;
        T UnicodeCaseTableData;
        DWORD NumberOfProcessors;
        union
        {
            DWORD NtGlobalFlag;
            NGF dummy02;
        };
        LARGE_INTEGER CriticalSectionTimeout;
        T HeapSegmentReserve;
        T HeapSegmentCommit;
        T HeapDeCommitTotalFreeThreshold;
        T HeapDeCommitFreeBlockThreshold;
        DWORD NumberOfHeaps;
        DWORD MaximumNumberOfHeaps;
        T ProcessHeaps;
        T GdiSharedHandleTable;
        T ProcessStarterHelper;
        T GdiDCAttributeList;
        T LoaderLock;
        DWORD OSMajorVersion;
        DWORD OSMinorVersion;
        WORD OSBuildNumber;
        WORD OSCSDVersion;
        DWORD OSPlatformId;
        DWORD ImageSubsystem;
        DWORD ImageSubsystemMajorVersion;
        T ImageSubsystemMinorVersion;
        T ActiveProcessAffinityMask;
        T GdiHandleBuffer[A];
        T PostProcessInitRoutine;
        T TlsExpansionBitmap;
        DWORD TlsExpansionBitmapBits[32];
        T SessionId;
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
        DWORD FlsBitmapBits[4];
        T FlsHighIndex;
        T WerRegistrationData;
        T WerShipAssertPtr;
        T pContextData;
        T pImageHeaderHash;
        T TracingFlags;
        T CsrServerReadOnlySharedMemoryBase;
    };

#pragma warning(default : 4201)

    template<typename T>
    struct _ACTCTXW_T
    {
        ULONG  cbSize;
        DWORD  dwFlags;
        T      lpSource;
        USHORT wProcessorArchitecture;
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
        ULONG	 Reserved0;
        T	     PebBaseAddress;
        T	     AffinityMask;
        LONG	 BasePriority;
        ULONG	 Reserved1;
        T	     uUniqueProcessId;
        T	     uInheritedFromUniqueProcessId;
    };

    template<typename T>
    struct _THREAD_BASIC_INFORMATION_T
    {
        NTSTATUS  ExitStatus;
        //LONG      Padding;
        T         TebBaseAddress;

        struct
        {
            T  p1;
            T  p2;
        }ClientId;

        T      AffinityMask;
        LONG   Priority;
        LONG   BasePriority;
    };

    struct _XSAVE_FORMAT64
    {
        WORD ControlWord;
        WORD StatusWord;
        BYTE TagWord;
        BYTE Reserved1;
        WORD ErrorOpcode;
        DWORD ErrorOffset;
        WORD ErrorSelector;
        WORD Reserved2;
        DWORD DataOffset;
        WORD DataSelector;
        WORD Reserved3;
        DWORD MxCsr;
        DWORD MxCsr_Mask;
        _M128A FloatRegisters[8];
        _M128A XmmRegisters[16];
        BYTE Reserved4[96];
    };

    template<typename T>
    struct _CONTEXT_T;

    template<>
    struct _CONTEXT_T<DWORD>
    {
        DWORD   ContextFlags;
        DWORD   Dr0;
        DWORD   Dr1;
        DWORD   Dr2;
        DWORD   Dr3;
        DWORD   Dr6;
        DWORD   Dr7;
        WOW64_FLOATING_SAVE_AREA FloatSave;
        DWORD   SegGs;
        DWORD   SegFs;
        DWORD   SegEs;
        DWORD   SegDs;
        DWORD   Edi;
        DWORD   Esi;
        DWORD   Ebx;
        DWORD   Edx;
        DWORD   Ecx;
        DWORD   Eax;
        DWORD   Ebp;
        DWORD   Eip;
        DWORD   SegCs;              // MUST BE SANITIZED
        DWORD   EFlags;             // MUST BE SANITIZED
        DWORD   Esp;
        DWORD   SegSs;
        BYTE    ExtendedRegisters[WOW64_MAXIMUM_SUPPORTED_EXTENSION];
    };
    
    template<>
    struct _CONTEXT_T<DWORD64>
    {
        DWORD64 P1Home;
        DWORD64 P2Home;
        DWORD64 P3Home;
        DWORD64 P4Home;
        DWORD64 P5Home;
        DWORD64 P6Home;
        DWORD ContextFlags;
        DWORD MxCsr;
        WORD SegCs;
        WORD SegDs;
        WORD SegEs;
        WORD SegFs;
        WORD SegGs;
        WORD SegSs;
        DWORD EFlags;
        DWORD64 Dr0;
        DWORD64 Dr1;
        DWORD64 Dr2;
        DWORD64 Dr3;
        DWORD64 Dr6;
        DWORD64 Dr7;
        DWORD64 Rax;
        DWORD64 Rcx;
        DWORD64 Rdx;
        DWORD64 Rbx;
        DWORD64 Rsp;
        DWORD64 Rbp;
        DWORD64 Rsi;
        DWORD64 Rdi;
        DWORD64 R8;
        DWORD64 R9;
        DWORD64 R10;
        DWORD64 R11;
        DWORD64 R12;
        DWORD64 R13;
        DWORD64 R14;
        DWORD64 R15;
        DWORD64 Rip;
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
        DWORD64 VectorControl;
        DWORD64 DebugControl;
        DWORD64 LastBranchToRip;
        DWORD64 LastBranchFromRip;
        DWORD64 LastExceptionToRip;
        DWORD64 LastExceptionFromRip;
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


    //
    // Loader related
    //
    enum _LDR_DDAG_STATE
    {
        LdrModulesMerged = -5,
        LdrModulesInitError = -4,
        LdrModulesSnapError = -3,
        LdrModulesUnloaded = -2,
        LdrModulesUnloading = -1,
        LdrModulesPlaceHolder = 0,
        LdrModulesMapping = 1,
        LdrModulesMapped = 2,
        LdrModulesWaitingForDependencies = 3,
        LdrModulesSnapping = 4,
        LdrModulesSnapped = 5,
        LdrModulesCondensed = 6,
        LdrModulesReadyToInit = 7,
        LdrModulesInitializing = 8,
        LdrModulesReadyToRun = 9
    };

    enum _LDR_DLL_LOAD_REASON
    {
        LoadReasonStaticDependency = 0,
        LoadReasonStaticForwarderDependency = 1,
        LoadReasonDynamicForwarderDependency = 2,
        LoadReasonDelayloadDependency = 3,
        LoadReasonDynamicLoad = 4,
        LoadReasonAsImageLoad = 5,
        LoadReasonAsDataLoad = 6,
        LoadReasonUnknown = -1
    };

    template<typename T>
    struct _PEB_LDR_DATA2
    {
        unsigned long Length;
        unsigned char Initialized;
        T SsHandle;
        _LIST_ENTRY_T<T> InLoadOrderModuleList;
        _LIST_ENTRY_T<T> InMemoryOrderModuleList;
        _LIST_ENTRY_T<T> InInitializationOrderModuleList;
        T EntryInProgress;
        unsigned char ShutdownInProgress;
        T ShutdownThreadId;
    };

    template<typename T>
    struct _LDR_DATA_TABLE_ENTRY_BASE
    {
        _LIST_ENTRY_T<T> InLoadOrderLinks;
        _LIST_ENTRY_T<T> InMemoryOrderLinks;
        _LIST_ENTRY_T<T> InInitializationOrderLinks;
        T DllBase;
        T EntryPoint;
        unsigned long SizeOfImage;
        _UNICODE_STRING_T<T> FullDllName;
        _UNICODE_STRING_T<T> BaseDllName;
        unsigned long Flags;
        unsigned short LoadCount;
        unsigned short TlsIndex;
        _LIST_ENTRY_T<T> HashLinks;
        unsigned long TimeDateStamp;
        T EntryPointActivationContext;
        T PatchInformation;
    };
  
    typedef struct _RTL_INVERTED_FUNCTION_TABLE_ENTRY
    {
        PIMAGE_RUNTIME_FUNCTION_ENTRY ExceptionDirectory;
        PVOID ImageBase;
        ULONG ImageSize;
        ULONG SizeOfTable;
    } RTL_INVERTED_FUNCTION_TABLE_ENTRY, *PRTL_INVERTED_FUNCTION_TABLE_ENTRY;

    //
    // Api schema structures
    //
    typedef struct _API_SET_VALUE_ENTRY
    {
        ULONG Flags;
        ULONG NameOffset;
        ULONG NameLength;
        ULONG ValueOffset;
        ULONG ValueLength;
    } API_SET_VALUE_ENTRY, *PAPI_SET_VALUE_ENTRY;

    typedef struct _API_SET_VALUE_ARRAY
    {
        ULONG Flags;
        ULONG Count;
        API_SET_VALUE_ENTRY Array[ANYSIZE_ARRAY];
   } API_SET_VALUE_ARRAY, *PAPI_SET_VALUE_ARRAY;

    typedef struct _API_SET_NAMESPACE_ENTRY
    {
        ULONG Flags;
        ULONG NameOffset;
        ULONG NameLength;
        ULONG AliasOffset;
        ULONG AliasLength;
        ULONG DataOffset;   // API_SET_VALUE_ARRAY
    } API_SET_NAMESPACE_ENTRY, *PAPI_SET_NAMESPACE_ENTRY;

    typedef struct _API_SET_NAMESPACE_ARRAY
    {
        ULONG Version;
        ULONG Size;
        ULONG Flags;
        ULONG Count;
        API_SET_NAMESPACE_ENTRY Array[ANYSIZE_ARRAY];
    } API_SET_NAMESPACE_ARRAY, *PAPI_SET_NAMESPACE_ARRAY;


    typedef struct _API_SET_VALUE_ENTRY_V2
    {
        ULONG NameOffset;
        ULONG NameLength;
        ULONG ValueOffset;
        ULONG ValueLength;
    } API_SET_VALUE_ENTRY_V2, *PAPI_SET_VALUE_ENTRY_V2;

    typedef struct _API_SET_VALUE_ARRAY_V2
    {
        ULONG Count;
        API_SET_VALUE_ENTRY_V2 Array[ANYSIZE_ARRAY];
    } API_SET_VALUE_ARRAY_V2, *PAPI_SET_VALUE_ARRAY_V2;

    typedef struct _API_SET_NAMESPACE_ENTRY_V2
    {
        ULONG NameOffset;
        ULONG NameLength;
        ULONG DataOffset;   // API_SET_VALUE_ARRAY
    } API_SET_NAMESPACE_ENTRY_V2, *PAPI_SET_NAMESPACE_ENTRY_V2;

    typedef struct _API_SET_NAMESPACE_ARRAY_V2
    {
        ULONG Version;
        ULONG Count;
        API_SET_NAMESPACE_ENTRY_V2 Array[ANYSIZE_ARRAY];
    } API_SET_NAMESPACE_ARRAY_V2, *PAPI_SET_NAMESPACE_ARRAY_V2;

    typedef _PEB_T<DWORD, DWORD64, 34> _PEB32;
    typedef _PEB_T<DWORD64, DWORD, 30> _PEB64;

    typedef _TEB_T<DWORD>     _TEB32;
    typedef _TEB_T<DWORD64>   _TEB64;
    typedef _TEB_T<DWORD_PTR>  TEB_T;

    typedef _PEB_LDR_DATA2<DWORD>     _PEB_LDR_DATA232;
    typedef _PEB_LDR_DATA2<DWORD64>   _PEB_LDR_DATA264;
    typedef _PEB_LDR_DATA2<DWORD_PTR>  PEB_LDR_DATA_T;

    typedef _LDR_DATA_TABLE_ENTRY_BASE<DWORD>     _LDR_DATA_TABLE_ENTRY_BASE32;
    typedef _LDR_DATA_TABLE_ENTRY_BASE<DWORD64>   _LDR_DATA_TABLE_ENTRY_BASE64; 
    typedef _LDR_DATA_TABLE_ENTRY_BASE<DWORD_PTR>  LDR_DATA_TABLE_ENTRY_BASE_T;

    typedef _CONTEXT_T<DWORD>     _CONTEXT32;
    typedef _CONTEXT_T<DWORD64>   _CONTEXT64;
    typedef _CONTEXT_T<DWORD_PTR>  CONTEXT_T;

#ifdef _M_AMD64
    typedef _PEB64 PEB_T;
#else
    typedef _PEB32 PEB_T;
#endif
}

// OS specific structures
#include "Win7Specific.h"
#include "Win8Specific.h"