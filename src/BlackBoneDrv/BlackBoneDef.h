#pragma once

#define BLACKBONE_DEVICE_NAME           L"BlackBone"
#define BLACKBONE_DEVICE_FILE           L"\\\\.\\" BLACKBONE_DEVICE_NAME

#define FILE_DEVICE_BLACKBONE           0x8005

#define BLACKBONE_FILE_EXT              L".sys"
#define BLACKBONE_FILE_SUFFIX           L"Drv"
#define BLACKBONE_FILE_NAME             BLACKBONE_DEVICE_NAME BLACKBONE_FILE_SUFFIX       BLACKBONE_FILE_EXT
#define BLACKBONE_FILE_NAME_7           BLACKBONE_DEVICE_NAME BLACKBONE_FILE_SUFFIX L"7"  BLACKBONE_FILE_EXT
#define BLACKBONE_FILE_NAME_8           BLACKBONE_DEVICE_NAME BLACKBONE_FILE_SUFFIX L"8"  BLACKBONE_FILE_EXT
#define BLACKBONE_FILE_NAME_81          BLACKBONE_DEVICE_NAME BLACKBONE_FILE_SUFFIX L"81" BLACKBONE_FILE_EXT
#define BLACKBONE_FILE_NAME_10          BLACKBONE_DEVICE_NAME BLACKBONE_FILE_SUFFIX L"10" BLACKBONE_FILE_EXT

/*
    Disable process DEP

    Input:
        DISABLE_DEP

    Input size: 
        sizeof(DISABLE_DEP)

    Output:
        void

    Output size:
        0
*/
#define IOCTL_BLACKBONE_DISABLE_DEP  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Change process protection state

    Input:
       SET_PROC_PROTECTION

    Input size: 
        sizeof(SET_PROC_PROTECTION)

    Output:
        void

    Output size:
        0
*/
#define IOCTL_BLACKBONE_SET_PROTECTION  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Change handle access rights

    Input:
       GRANT_ACCESS

    Input size: 
        sizeof(GRANT_ACCESS)

    Output:
        void

    Output size:
        0
*/
#define IOCTL_BLACKBONE_GRANT_ACCESS   (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Read or write virtual memory of target process

    Input:
       COPY_MEMORY

    Input size: 
        sizeof(COPY_MEMORY)

    Output:
        void

    Output size:
        0
*/
#define IOCTL_BLACKBONE_COPY_MEMORY  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Allocate or free memory

    Input:
       ALLOCATE_FREE_MEMORY

    Input size: 
        sizeof(ALLOCATE_FREE_MEMORY)

    Output:
        ALLOCATE_FREE_MEMORY_RESULT

    Output size:
        sizeof(ALLOCATE_FREE_MEMORY_RESULT)
*/
#define IOCTL_BLACKBONE_ALLOCATE_FREE_MEMORY  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Change protection of memory region

    Input:
       PROTECT_MEMORY

    Input size: 
        sizeof(PROTECT_MEMORY)

    Output:
        void

    Output size:
        0
*/
#define IOCTL_BLACKBONE_PROTECT_MEMORY  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Map entire address space of target process into calling process

    Input:
       MAP_MEMORY

    Input size: 
        sizeof(MAP_MEMORY)

    Output:
        ULONG sizeRequired - if output buffer isn't large enough to hold output data
        MAP_MEMORY_REGION_RESULT result - if buffer is large enough to hold output data

    Output size:
        sizeof(ULONG) - if output buffer isn't large enough to hold output data
        >= sizeof(MAP_MEMORY_REGION_RESULT) - if buffer is large enough to hold output data
*/
#define IOCTL_BLACKBONE_MAP_MEMORY  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Map single memory region into calling process

    Input:
       MAP_MEMORY_REGION

    Input size: 
        sizeof(MAP_MEMORY_REGION)

    Output:
        MAP_MEMORY_REGION_RESULT

    Output size:
        sizeof(MAP_MEMORY_REGION_RESULT) 
*/
#define IOCTL_BLACKBONE_MAP_REGION  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x807, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Unmap all mapped memory from calling process

    Input:
       UNMAP_MEMORY

    Input size: 
        sizeof(UNMAP_MEMORY)

    Output:
        void

    Output size:
        0
*/
#define IOCTL_BLACKBONE_UNMAP_MEMORY  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x808, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Unmap single memory region from calling process

    Input:
       UNMAP_MEMORY_REGION

    Input size: 
        sizeof(UNMAP_MEMORY_REGION)

    Output:
        UNMAP_MEMORY_REGION_RESULT

    Output size:
        sizeof(UNMAP_MEMORY_REGION_RESULT)
*/
#define IOCTL_BLACKBONE_UNMAP_REGION  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x809, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Unlink target VAD from process VAD tree

    Input:
       HIDE_VAD

    Input size: 
        sizeof(HIDE_VAD)

    Output:
        NULL

    Output size:
        0
*/
#define IOCTL_BLACKBONE_HIDE_VAD  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x80A, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Inject dll into arbitrary process

    Input:
       INJECT_DLL

    Input size: 
        sizeof(INJECT_DLL)

    Output:
        NULL

    Output size:
        0
*/
#define IOCTL_BLACKBONE_INJECT_DLL  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x80B, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Manually map system driver

    Input:
       MMAP_DRIVER

    Input size: 
        sizeof(MMAP_DRIVER)

    Output:
        NULL

    Output size:
        0
*/
#define IOCTL_BLACKBONE_MAP_DRIVER  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x80C, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
    Unlink process handle table from handle table list

    Input:
       UNLINK_HTABLE

    Input size: 
        sizeof(UNLINK_HTABLE)

    Output:
        NULL

    Output size:
        0
*/
#define IOCTL_BLACKBONE_UNLINK_HTABLE  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x80D, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
Map entire address space of target process into calling process

    Input:
       ENUM_REGIONS

    Input size: 
        sizeof(ENUM_REGIONS)

    Output:
        ENUM_REGIONS_RESULT - enumerated regions

    Output size:
        >= sizeof(ENUM_REGIONS_RESULT)
*/
#define IOCTL_BLACKBONE_ENUM_REGIONS  (ULONG)CTL_CODE(FILE_DEVICE_BLACKBONE, 0x80E, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


/// <summary>
/// Input for IOCTL_BLACKBONE_DISABLE_DEP
/// </summary>
typedef struct _DISABLE_DEP
{
    ULONG   pid;            // Process ID
} DISABLE_DEP, *PDISABLE_DEP;

/// <summary>
/// Policy activation option
/// </summary>
typedef enum _PolicyOpt
{
    Policy_Disable,
    Policy_Enable,
    Policy_Keep,        // Don't change current value
} PolicyOpt;

/// <summary>
/// Input for IOCTL_BLACKBONE_SET_PROTECTION
/// </summary>
typedef struct _SET_PROC_PROTECTION
{
    ULONG pid;              // Process ID
    PolicyOpt protection;   // Process protection
    PolicyOpt dynamicCode;  // DynamiCode policy
    PolicyOpt signature;    // BinarySignature policy
} SET_PROC_PROTECTION, *PSET_PROC_PROTECTION;

/// <summary>
/// Input for IOCTL_BLACKBONE_GRANT_ACCESS
/// </summary>
typedef struct _HANDLE_GRANT_ACCESS
{
    ULONGLONG  handle;      // Handle to modify
    ULONG      pid;         // Process ID
    ULONG      access;      // Access flags to grant
} HANDLE_GRANT_ACCESS, *PHANDLE_GRANT_ACCESS;

/// <summary>
/// Input for IOCTL_BLACKBONE_COPY_MEMORY
/// </summary>
typedef struct _COPY_MEMORY
{
    ULONGLONG localbuf;         // Buffer address
    ULONGLONG targetPtr;        // Target address
    ULONGLONG size;             // Buffer size
    ULONG     pid;              // Target process id
    BOOLEAN   write;            // TRUE if write operation, FALSE if read
} COPY_MEMORY, *PCOPY_MEMORY;

/// <summary>
/// Input for IOCTL_BLACKBONE_ALLOCATE_FREE_MEMORY
/// </summary>
typedef struct _ALLOCATE_FREE_MEMORY
{
    ULONGLONG base;             // Region base address
    ULONGLONG size;             // Region size
    ULONG     pid;              // Target process id
    ULONG     protection;       // Memory protection for allocation
    ULONG     type;             // MEM_RESERVE/MEM_COMMIT/MEM_DECOMMIT/MEM_RELEASE
    BOOLEAN   allocate;         // TRUE if allocation, FALSE is freeing
    BOOLEAN   physical;         // If set to TRUE, physical pages will be directly mapped into UM space
} ALLOCATE_FREE_MEMORY, *PALLOCATE_FREE_MEMORY;

/// <summary>
/// Output for IOCTL_BLACKBONE_ALLOCATE_FREE_MEMORY
/// </summary>
typedef struct _ALLOCATE_FREE_MEMORY_RESULT
{
    ULONGLONG address;          // Address of allocation
    ULONGLONG size;             // Allocated size
} ALLOCATE_FREE_MEMORY_RESULT, *PALLOCATE_FREE_MEMORY_RESULT;

/// <summary>
/// Input for IOCTL_BLACKBONE_PROTECT_MEMORY
/// </summary>
typedef struct _PROTECT_MEMORY
{
    ULONGLONG base;             // Region base address
    ULONGLONG size;             // Region size
    ULONG     pid;              // Target process id
    ULONG     newProtection;    // New protection value
} PROTECT_MEMORY, *PPROTECT_MEMORY;

/// <summary>
/// Input for IOCTL_BLACKBONE_REMAP_MEMORY
/// </summary>
typedef struct _MAP_MEMORY
{
    ULONG   pid;                // Target process id
    wchar_t pipeName[32];       // Hook pipe name
    BOOLEAN mapSections;        // Set to TRUE to map sections
} MAP_MEMORY, *PMAP_MEMORY;

/// <summary>
/// Remapped region info
/// </summary>
typedef struct _MAP_MEMORY_RESULT_ENTRY
{
    ULONGLONG originalPtr;      // Address in target process
    ULONGLONG newPtr;           // Mapped address in host process
    ULONG size;                 // Region size
} MAP_MEMORY_RESULT_ENTRY, *PMAP_MEMORY_RESULT_ENTRY;

/// <summary>
/// Output for IOCTL_BLACKBONE_REMAP_REGION
/// </summary>
typedef struct _MAP_MEMORY_RESULT
{
    ULONGLONG pipeHandle;       // Pipe handle in target process
    ULONGLONG targetPage;       // Address of shared page in target process
    ULONGLONG hostPage;         // Address of shared page in host process

    ULONG count;                // Number of REMAP_MEMORY_RESULT_ENTRY entries

    // List of remapped regions (variable-sized array)
    MAP_MEMORY_RESULT_ENTRY entries[1];
} MAP_MEMORY_RESULT, *PMAP_MEMORY_RESULT;

/// <summary>
/// Input for IOCTL_BLACKBONE_REMAP_REGION
/// </summary>
typedef struct _MAP_MEMORY_REGION
{
    ULONGLONG base;             // Region base address
    ULONG     pid;              // Target process id
    ULONG     size;             // Region size
} MAP_MEMORY_REGION, *PMAP_MEMORY_REGION;

/// <summary>
/// Output for IOCTL_BLACKBONE_REMAP_REGION
/// </summary>
typedef struct _MAP_MEMORY_REGION_RESULT
{
    ULONGLONG originalPtr;      // Address in target process
    ULONGLONG newPtr;           // Mapped address in host process
    ULONGLONG removedPtr;       // Unmapped region base, in case of conflicting region
    ULONG     size;             // Mapped region size
    ULONG     removedSize;      // Unmapped region size
} MAP_MEMORY_REGION_RESULT, *PMAP_MEMORY_REGION_RESULT;

/// <summary>
/// Input for IOCTL_BLACKBONE_UNMAP_MEMORY
/// </summary>
typedef struct _UNMAP_MEMORY
{
    ULONG     pid;              // Target process ID
} UNMAP_MEMORY, *PUNMAP_MEMORY;

/// <summary>
/// Input for IOCTL_BLACKBONE_UNMAP_REGION
/// </summary>
typedef struct _UNMAP_MEMORY_REGION
{
    ULONGLONG base;             // Region base address
    ULONG     pid;              // Target process ID
    ULONG     size;             // Region size
} UNMAP_MEMORY_REGION, *PUNMAP_MEMORY_REGION;


/// <summary>
/// Input for IOCTL_BLACKBONE_HIDE_VAD
/// </summary>
typedef struct _HIDE_VAD
{
    ULONGLONG base;             // Region base address
    ULONGLONG size;             // Region size
    ULONG pid;                  // Target process ID
} HIDE_VAD, *PHIDE_VAD;

typedef enum _InjectType
{
    IT_Thread,      // CreateThread into LdrLoadDll
    IT_Apc,         // Force user APC into LdrLoadDll
    IT_MMap,        // Manual map
} InjectType;

typedef enum _MmapFlags
{
    KNoFlags         = 0x00,    // No flags
    KManualImports   = 0x01,    // Manually map import libraries
    KWipeHeader      = 0x04,    // Wipe image PE headers
    KHideVAD         = 0x10,    // Make image appear as PAGE_NOACESS region
    KRebaseProcess   = 0x40,    // If target image is an .exe file, process base address will be replaced with mapped module value

    KNoExceptions    = 0x01000, // Do not create custom exception handler
    KNoSxS           = 0x08000, // Do not apply SxS activation context
    KNoTLS           = 0x10000, // Skip TLS initialization and don't execute TLS callbacks
} KMmapFlags;

/// <summary>
/// Input for IOCTL_BLACKBONE_INJECT_DLL
/// </summary>
typedef struct _INJECT_DLL
{
    InjectType type;                // Type of injection
    wchar_t    FullDllPath[512];    // Fully-qualified path to the target dll
    wchar_t    initArg[512];        // Init routine argument
    ULONG      initRVA;             // Init routine RVA, if 0 - no init routine
    ULONG      pid;                 // Target process ID
    BOOLEAN    wait;                // Wait on injection thread
    BOOLEAN    unlink;              // Unlink module after injection
    BOOLEAN    erasePE;             // Erase PE headers after injection   
    KMmapFlags flags;               // Manual map flags
    ULONGLONG  imageBase;           // Image address in memory to manually map
    ULONG      imageSize;           // Size of memory image
    BOOLEAN    asImage;             // Memory chunk has image layout
} INJECT_DLL, *PINJECT_DLL;

/// <summary>
/// Input for IOCTL_BLACKBONE_MAP_DRIVER
/// </summary>
typedef struct _MMAP_DRIVER
{
    wchar_t    FullPath[512];    // Fully-qualified path to the driver
} MMAP_DRIVER, *PMMAP_DRIVER;

/// <summary>
/// Input for IOCTL_BLACKBONE_UNLINK_HTABLE
/// </summary>
typedef struct _UNLINK_HTABLE
{
    ULONG      pid;         // Process ID
} UNLINK_HTABLE, *PUNLINK_HTABLE;

/// <summary>
/// Input for IOCTL_BLACKBONE_ENUM_REGIONS
/// </summary>
typedef struct _ENUM_REGIONS
{
    ULONG      pid;         // Process ID
} ENUM_REGIONS, *PENUM_REGIONS;

typedef struct _MEM_REGION
{
    ULONGLONG BaseAddress;
    ULONGLONG AllocationBase;
    ULONG AllocationProtect;
    ULONGLONG RegionSize;
    ULONG State;
    ULONG Protect;
    ULONG Type;
} MEM_REGION, *PMEM_REGION;

/// <summary>
/// Output for IOCTL_BLACKBONE_ENUM_REGIONS
/// </summary>
typedef struct _ENUM_REGIONS_RESULT
{
    ULONGLONG  count;                   // Number of records
    MEM_REGION regions[1];              // Found regions, variable-sized
} ENUM_REGIONS_RESULT, *PENUM_REGIONS_RESULT;