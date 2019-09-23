#pragma once

#include "../../Include/Winheaders.h"
#include "../../PE/PEImage.h"
#include "../../Include/Types.h"
#include "../../Include/NativeStructures.h"
#include "../../Include/Macro.h"
#include "../../Include/CallResult.h"

namespace blackbone
{
enum LdrRefFlags
{
    Ldr_None      = 0x00,   // Do not create any reference
    Ldr_ModList   = 0x01,   // Add to module list -  LdrpModuleIndex( win8 only ), InMemoryOrderModuleList( win7 only )
    Ldr_HashTable = 0x02,   // Add to LdrpHashTable
    Ldr_ThdCall   = 0x04,   // Add to thread callback list (dllmain will be called with THREAD_ATTACH/DETACH reasons)
    Ldr_All       = 0xFF,   // Add to everything
    Ldr_Ignore    = 0xDE    // Only valid in mod callback, mod callback value will be ignored
};

ENUM_OPS( LdrRefFlags )

struct NtLdrEntry : ModuleData
{
    LdrRefFlags flags = Ldr_None;
    ptr_t entryPoint = 0;
    ULONG hash = 0;
    bool safeSEH = false;
};

class NtLdr
{
public:
    BLACKBONE_API NtLdr( class Process& proc );

    /// <summary>
    /// Initialize some loader stuff
    /// </summary>
    /// <param name="initFor">Target module type</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Init( eModType initFor = mt_default );

    /// <summary>
    /// Add module to some loader structures 
    /// (LdrpHashTable, LdrpModuleIndex( win8 only ), InMemoryOrderModuleList( win7 only ))
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool CreateNTReference( NtLdrEntry& mod );

    /// <summary>
    /// Create thread static TLS array
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <param name="tlsPtr">TLS directory of target image</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS AddStaticTLSEntry( NtLdrEntry& mod, ptr_t tlsPtr );

    /// <summary>
    /// Create module record in LdrpInvertedFunctionTable
    /// Used to create fake SAFESEH entries
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool InsertInvertedFunctionTable( NtLdrEntry& mod );

    /// <summary>
    /// Free static TLS
    /// </summary>
    /// <param name="mod">Target module</param>
    /// <param name="noThread">Don't create new threads during remote call</param>
    /// <returns>Status code</returns>
    BLACKBONE_API NTSTATUS UnloadTLS( const NtLdrEntry& mod, bool noThread = false );

    /// <summary>
    /// Unlink module from Ntdll loader
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <param name="noThread">Don't create new threads during unlink</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool Unlink( const ModuleData& mod, bool noThread = false );
private:

    /// <summary>
    /// Find LdrpHashTable[] variable
    /// </summary>
    /// <returns>true on success</returns>
    template<typename T>
    bool FindLdrpHashTable();

    /// <summary>
    /// Find LdrpModuleIndex variable under win8
    /// </summary>
    /// <returns>true on success</returns>
    template<typename T>
    bool FindLdrpModuleIndexBase();

    /// <summary>
    /// Find Loader heap base
    /// </summary>
    /// <returns>true on success</returns>
    template<typename T>
    bool FindLdrHeap();

    /// <summary>
    ///  Initialize OS-specific module entry
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>Pointer to created entry</returns>
    template<typename T>
    ptr_t InitBaseNode( NtLdrEntry& mod );

    /// <summary>
    ///  Initialize OS-specific module entry
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>Pointer to created entry</returns>
    template<typename T>
    ptr_t InitW8Node( NtLdrEntry& mod );

    /// <summary>
    ///  Initialize OS-specific module entry
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>Pointer to created entry</returns>
    template<typename T>
    ptr_t InitW7Node( NtLdrEntry& mod );

    /// <summary>
    /// Insert entry into win8 module graph
    /// </summary>
    /// <param name="nodePtr">Node to insert</param>
    /// <param name="mod">Module data</param>
    template<typename T>
    void InsertTreeNode( ptr_t nodePtr, const NtLdrEntry& mod );

    /// <summary>
    /// Insert entry into LdrpHashTable[]
    /// </summary>
    /// <param name="pNodeLink">Link of entry to be inserted</param>
    /// <param name="hash">Module hash</param>
    template<typename T>
    void InsertHashNode( ptr_t pNodeLink, ULONG hash );

    /// <summary>
    /// Insert entry into InLoadOrderModuleList and InMemoryOrderModuleList
    /// </summary>
    /// <param name="pNodeMemoryOrderLink">InMemoryOrderModuleList link of entry to be inserted</param>
    /// <param name="pNodeLoadOrderLink">InLoadOrderModuleList link of entry to be inserted</param>
    template<typename T>
    void InsertMemModuleNode( ptr_t pNodeMemoryOrderLink, ptr_t pNodeLoadOrderLink, ptr_t pNodeInitOrderLink );

    /// <summary>
    /// Insert entry into standard double linked list
    /// </summary>
    /// <param name="ListHead">List head pointer</param>
    /// <param name="Entry">Entry list link to be inserted</param>
    template<typename T>
    void InsertTailList( ptr_t ListHead, ptr_t Entry );

    /// <summary>
    /// Hash image name
    /// </summary>
    /// <param name="str">Image name</param>
    /// <returns>Hash</returns>
    ULONG HashString( const std::wstring& str );

    /// <summary>
    /// Allocate memory from heap if possible
    /// </summary>
    /// <param name="size">Module type</param>
    /// <param name="size">Size to allocate</param>
    /// <returns>Allocated address</returns>
    call_result_t<ptr_t> AllocateInHeap( eModType mt, size_t size );

    /// <summary>
    /// Get module native node ptr or create new
    /// </summary>
    /// <param name="ptr">node pointer (if nullptr - new dummy node is allocated)</param>
    /// <param name="pModule">Module base address</param>
    /// <returns>Node address</returns>
    template<typename T, typename Module>
    ptr_t SetNode( ptr_t ptr, Module pModule );

    /// <summary>
    /// Unlink module from PEB_LDR_DATA
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <returns>Address of removed record</returns>
    template<typename T>
    ptr_t UnlinkFromLdr( const ModuleData& mod );

    /// <summary>
    /// Finds LDR entry for module
    /// </summary>
    /// <param name="moduleBase">Target module base</param>
    /// <param name="found">Found entry</param>
    /// <returns>Found LDR entry address</returns>
    template<typename T>
    ptr_t FindLdrEntry( module_t moduleBase, _LDR_DATA_TABLE_ENTRY_BASE_T<T>* found = nullptr );

    /// <summary>
    ///  Remove record from LIST_ENTRY structure
    /// </summary>
    /// <param name="pListLink">Entry link</param>
    template<typename T>
    void UnlinkListEntry( ptr_t pListLink );

    /// <summary>
    /// Unlink from module graph
    /// </summary>
    /// <param name="mod">Module data</param>
    /// <param name="ldrEntry">Module LDR entry</param>
    /// <param name="noThread">Don't create new threads during unlink</param>
    /// <returns>Address of removed record</returns>
    template<typename T>
    ptr_t UnlinkTreeNode( const ModuleData& mod, ptr_t ldrEntry, bool noThread = false );

    NtLdr( const NtLdr& ) = delete;
    NtLdr& operator =( const NtLdr& ) = delete;

private:
    class Process& _process;                // Process memory routines

    ptr_t _LdrpHashTable = 0;               // LdrpHashTable address
    ptr_t _LdrpModuleIndexBase = 0;         // LdrpModuleIndex address
    ptr_t _LdrHeapBase = 0;                 // Loader heap base address

    eModType _initializedFor = mt_unknown;  // Loader initialization target
    std::map<ptr_t, ptr_t> _nodeMap;        // Allocated native structures
};
}
