#pragma once

#include "../../Include/Winheaders.h"
#include "../../PE/PEImage.h"
#include "../../Include/NativeStructures.h"
#include "../../Include/Macro.h"

namespace blackbone
{

enum LdrRefFlags
{
    Ldr_None      = 0x00,   // Do not create any reference
    Ldr_ModList   = 0x01,   // Add to module list -  LdrpModuleIndex( win8 only ), InMemoryOrderModuleList( win7 only )
    Ldr_HashTable = 0x02,   // Add to LdrpHashTable
    Ldr_ThdCall   = 0x04,   // Add to thread callback list (dllmain will be called with THREAD_ATTACH/DETACH reasons)
    Ldr_All       = 0xFF    // Add to everything
};

ENUM_OPS( LdrRefFlags )


class NtLdr
{
public:
    BLACKBONE_API NtLdr( class Process& proc );
    BLACKBONE_API ~NtLdr( void );

    /// <summary>
    /// Initialize some loader stuff
    /// </summary>
    /// <returns></returns>
    BLACKBONE_API bool Init();

    /// <summary>
    /// Add module to some loader structures 
    /// (LdrpHashTable, LdrpModuleIndex( win8 only ), InMemoryOrderModuleList( win7 only ))
    /// </summary>
    /// <param name="hMod">Module base address</param>
    /// <param name="ImageSize">Size of image</param>
    /// <param name="DllBasePath">Full-qualified image path</param>
    /// <param name="entryPoint">Entry point RVA</param>
    /// <param name="flags">Type of references to create</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool CreateNTReference(
        HMODULE hMod,
        size_t ImageSize,
        const std::wstring& DllBasePath,
        uintptr_t entryPoint,
        LdrRefFlags flags = Ldr_All
        );

    /// <summary>
    /// Create thread static TLS array
    /// </summary>
    /// <param name="pModule">Module base address</param>
    /// <param name="pTls">TLS directory of target image</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool AddStaticTLSEntry( void* pModule, IMAGE_TLS_DIRECTORY *pTls );

    /// <summary>
    /// Create module record in LdrpInvertedFunctionTable
    /// Used to create fake SAFESEH entries
    /// </summary>
    /// <param name="ModuleBase">Module base address</param>
    /// <param name="ImageSize">Size of image</param>
    /// <param name="safeseh">Is set into true, if image has SAFESEH handlers</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool InsertInvertedFunctionTable( void* ModuleBase, size_t ImageSize, bool& safeseh );

    /// <summary>
    /// Unlink module from Ntdll loader
    /// </summary>
    /// <param name="baseAddress">Module base address</param>
    /// <param name="type">32 or 64 bit.</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Unlink( ptr_t baseAddress, const std::wstring& name, eModType type );

    // 
    // Get some not exported values
    //
    BLACKBONE_API inline uintptr_t LdrpInvertedFunctionTable( ) const { return _LdrpInvertedFunctionTable; }
    BLACKBONE_API inline uintptr_t LdrKernel32PatchAddress() const { return _LdrKernel32PatchAddress; }
    BLACKBONE_API inline uintptr_t APC64PatchAddress() const { return _APC64PatchAddress; }
    BLACKBONE_API inline uintptr_t LdrProtectMrdata() const { return _LdrProtectMrdata; }
    
private:

    /// <summary>
    /// Find LdrpHashTable[] variable
    /// </summary>
    /// <returns>true on success</returns>
    bool FindLdrpHashTable();

    /// <summary>
    /// Find LdrpModuleIndex variable under win8
    /// </summary>
    /// <returns>true on success</returns>
    bool FindLdrpModuleIndexBase();

    /// <summary>
    /// Search for RtlInsertInvertedFunctionTable, LdrpInvertedFunctionTable, LdrpHandleTlsDatas.
    /// </summary>
    /// <returns>true on success</returns>
    bool ScanPatterns();

    /// <summary>
    /// Find Loader heap base
    /// </summary>
    /// <returns>true on success</returns>
    bool FindLdrHeap();

    /// <summary>
    ///  Initialize OS-specific module entry
    /// </summary>
    /// <param name="ModuleBase">Module base address.</param>
    /// <param name="ImageSize">Size of image.</param>
    /// <param name="dllpath">Full-qualified image path</param>
    /// <param name="entryPoint">Entry point RVA</param>
    /// <param name="outHash">Iamge name hash</param>
    /// <returns>Pointer to created entry</returns>
    _LDR_DATA_TABLE_ENTRY_W8* InitW8Node(
        void* ModuleBase,
        size_t ImageSize,
        const std::wstring& dllpath,
        uintptr_t entryPoint,
        ULONG& outHash
        );

    /// <summary>
    ///  Initialize OS-specific module entry
    /// </summary>
    /// <param name="ModuleBase">Module base address.</param>
    /// <param name="ImageSize">Size of image.</param>
    /// <param name="dllpath">Full-qualified image path</param>
    /// <param name="entryPoint">Entry point RVA</param>
    /// <param name="outHash">Iamge name hash</param>
    /// <returns>Pointer to created entry</returns>
    _LDR_DATA_TABLE_ENTRY_W7* InitW7Node(
        void* ModuleBase,
        size_t ImageSize,
        const std::wstring& dllpath,
        uintptr_t entryPoint,
        ULONG& outHash
        );

    /// <summary>
    /// Insert entry into win8 module graph
    /// </summary>
    /// <param name="pNode">Node to insert</param>
    void InsertTreeNode( _LDR_DATA_TABLE_ENTRY_W8* pNode, uintptr_t modBase );

    /// <summary>
    /// Insert entry into LdrpHashTable[]
    /// </summary>
    /// <param name="pNodeLink">Link of entry to be inserted</param>
    /// <param name="hash">Module hash</param>
    void InsertHashNode( uintptr_t pNodeLink, ULONG hash );

    /// <summary>
    /// Insert entry into InLoadOrderModuleList and InMemoryOrderModuleList
    /// </summary>
    /// <param name="pNodeMemoryOrderLink">InMemoryOrderModuleList link of entry to be inserted</param>
    /// <param name="pNodeLoadOrderLink">InLoadOrderModuleList link of entry to be inserted</param>
    void InsertMemModuleNode( uintptr_t pNodeMemoryOrderLink, uintptr_t pNodeLoadOrderLink, uintptr_t pNodeInitOrderLink );

    /// <summary>
    /// Insert entry into standard double linked list
    /// </summary>
    /// <param name="ListHead">List head pointer</param>
    /// <param name="Entry">Entry list link to be inserted</param>
    void InsertTailList( uintptr_t ListHead, uintptr_t Entry );

    /// <summary>
    /// Hash image name
    /// </summary>
    /// <param name="str">Iamge name</param>
    /// <returns>Hash</returns>
    ULONG HashString( const std::wstring& str );

    /// <summary>
    /// Get module native node ptr or create new
    /// </summary>
    /// <param name="ptr">node pointer (if nullptr - new dummy node is allocated)</param>
    /// <param name="pModule">Module base address</param>
    /// <returns>Node address</returns>
    template<typename T>
    T* SetNode( T* ptr, void* pModule );

    /// <summary>
    /// Unlink module from PEB_LDR_DATA
    /// </summary>
    /// <param name="baseAddress">Module base address.</param>
    /// <returns>Address of removed record</returns>
    template<typename T> 
    ptr_t UnlinkFromLdr( ptr_t baseAddress, const std::wstring& name );

    /// <summary>
    /// Remove record from LIST_ENTRY structure
    /// </summary>
    /// <param name="pListEntry">List to remove from</param>
    /// <param name="head">List head address.</param>
    /// <param name="ofst">Offset of link in _LDR_DATA_TABLE_ENTRY_BASE struct</param>
    /// <param name="baseAddress">Record to remove.</param>
    /// <returns>Address of removed record</returns>
    template<typename T> 
    ptr_t UnlinkListEntry( _LIST_ENTRY_T<T> pListEntry, ptr_t head, uintptr_t ofst, ptr_t baseAddress );

    template<typename T>
    void UnlinkListEntry( ptr_t pListLink );

    /// <summary>
    /// Unlink from module graph
    /// </summary>
    /// <param name="ldrEntry">Module LDR entry</param>
    /// <returns>Address of removed record</returns>
    ptr_t UnlinkTreeNode( ptr_t ldrEntry );

    NtLdr( const NtLdr& ) = delete;
    NtLdr& operator =(const NtLdr&) = delete;

private:
    class Process& _process;                            // Process memory routines

    uintptr_t _LdrpHashTable = 0;                        // LdrpHashTable address
    uintptr_t _LdrpModuleIndexBase = 0;                  // LdrpModuleIndex address
    uintptr_t _LdrHeapBase = 0;                          // Loader heap base address
    uintptr_t _LdrKernel32PatchAddress = 0;              // Address to patch to enable kernel32 loading under win7
    uintptr_t _APC64PatchAddress = 0;                    // Address to patch for x64->WOW64 APC dispatching under win7
    uintptr_t _LdrpHandleTlsData = 0;                    // LdrpHandleTlsData address
    uintptr_t _LdrpInvertedFunctionTable = 0;            // LdrpInvertedFunctionTable address
    uintptr_t _RtlInsertInvertedFunctionTable = 0;       // RtlInsertInvertedFunctionTable address
    uintptr_t _LdrProtectMrdata = 0;                     // LdrProtectMrdata address

    std::map<HMODULE, void*> _nodeMap;                  // Allocated native structures
};

}

