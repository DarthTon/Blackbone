#include "NtLoader.h"
#include "../../Process/Process.h"
#include "../../Include/Macro.h"
#include "../../Misc/Utils.h"
#include "../../Misc/DynImport.h"
#include "../../Misc/trace.hpp"
#include "../../Misc/PattrernLoader.h"

#include "../contrib/VersionHelpers.h"

namespace blackbone
{

NtLdr::NtLdr( Process& proc )
    : _process( proc )
{
}

NtLdr::~NtLdr(void)
{
}

/// <summary>
/// Initialize some loader stuff
/// </summary>
/// <returns></returns>
bool NtLdr::Init()
{
    FindLdrpHashTable();
    FindLdrHeap();

    if (IsWindows8OrGreater())
        FindLdrpModuleIndexBase();

    ScanPatterns();
    _nodeMap.clear();

    // Report errors
#ifndef BLACBONE_NO_TRACE
    auto barrier = _process.barrier().type;
    if (_LdrpHashTable == 0)
        BLACKBONE_TRACE( "NativeLdr: LdrpHashTable not found" );
    if (IsWindows8OrGreater() && _LdrpModuleIndexBase == 0)
        BLACKBONE_TRACE( "NativeLdr: LdrpModuleIndexBase not found" );
    if (_LdrHeapBase == 0 && (barrier == wow_32_32 || barrier == wow_64_64))
        BLACKBONE_TRACE( "NativeLdr: LdrHeapBase not found" );
#endif

    return true;
}

/// <summary>
/// Add module to some loader structures 
/// (LdrpHashTable, LdrpModuleIndex( win8+ only ), InMemoryOrderModuleList( win7 only ))
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>true on success</returns>
bool NtLdr::CreateNTReference( NtLdrEntry& mod )
{
    // Skip
    if (mod.flags == Ldr_None)
        return true;

    // Win 8 and higher
    if (IsWindows8OrGreater())
    {
        ULONG hash = 0;
        _LDR_DATA_TABLE_ENTRY_W8 *pEntry = InitW8Node( mod );
        _nodeMap.emplace( std::make_pair( mod.baseAddress, pEntry ) );

        // Insert into LdrpHashTable
        if (mod.flags & Ldr_HashTable)
            InsertHashNode( GET_FIELD_PTR( pEntry, HashLinks ), hash );

        // Insert into module graph
        if (mod.flags & Ldr_ModList)
            InsertTreeNode( pEntry, mod );

        if (mod.flags & Ldr_ThdCall)
        {
            _process.memory().Write( GET_FIELD_PTR( pEntry, Flags ), 0x80004 );
            InsertMemModuleNode( 0, GET_FIELD_PTR( pEntry, InLoadOrderLinks ), 
                                 GET_FIELD_PTR( pEntry, InInitializationOrderLinks ) );
        }
    }
    // Windows 7 and earlier
    else
    {
        ULONG hash = 0;
        _LDR_DATA_TABLE_ENTRY_W7 *pEntry = InitW7Node( mod );
        _nodeMap.emplace( std::make_pair( mod.baseAddress, pEntry ) );

        // Insert into LdrpHashTable
        if (mod.flags & Ldr_HashTable)
            InsertHashNode( GET_FIELD_PTR( pEntry, HashLinks ), hash );

        // Insert into LDR list
        if (mod.flags & (Ldr_ModList | Ldr_ThdCall))
        {
            _process.memory().Write( GET_FIELD_PTR( pEntry, Flags ), 0x80004 );
            InsertMemModuleNode( 0, GET_FIELD_PTR( pEntry, InLoadOrderLinks ), 0 );
        }
	}

    return true;
}

/// <summary>
/// Get module native node ptr or create new
/// </summary>
/// <param name="ptr">node pointer (if nullptr - new dummy node is allocated)</param>
/// <param name="pModule">Module base address</param>
/// <returns>Node address</returns>
template<typename T> 
T* NtLdr::SetNode( T* ptr, void* pModule )
{
    if(ptr == nullptr)
    {
        auto mem = _process.memory().Allocate( sizeof( T ), PAGE_READWRITE, 0, false );
        if (!mem)
            return nullptr;

        ptr = mem->ptr<T*>();
        mem->Write( FIELD_OFFSET2( T*, DllBase ), pModule );
    }

    return ptr;
}

/// <summary>
/// Create thread static TLS array
/// </summary>
/// <param name="mod">Module data</param>
/// <param name="pTls">TLS directory of target image</param>
/// <returns>Status code</returns>
NTSTATUS NtLdr::AddStaticTLSEntry( NtLdrEntry& mod, IMAGE_TLS_DIRECTORY *pTls )
{
    bool wxp = IsWindowsXPOrGreater() && !IsWindowsVistaOrGreater();
    void* pNode = _nodeMap.count( mod.baseAddress ) ? _nodeMap[mod.baseAddress] : nullptr;

    // Allocate appropriate structure
    if ((pNode = SetNode( reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(pNode), reinterpret_cast<void*>(mod.baseAddress) )) == nullptr)
        return STATUS_NO_MEMORY;

    // Manually add TLS table
    if ((wxp || _process.barrier().type == wow_64_32) && pTls != nullptr)
    {
        ptr_t pTeb = 0;
        if (wxp)
            pTeb = _process.remote().getWorker()->teb( static_cast<_TEB32*>(nullptr) );
        else
            pTeb = _process.remote().getWorker()->teb( static_cast<_TEB64*>(nullptr) );

        auto mem = _process.memory().Allocate( 0x1000, PAGE_READWRITE, 0, false );
        if (!mem)
            return mem.status;

        auto tlsStore = std::move( mem.result() );

        IMAGE_TLS_DIRECTORY remoteTls = { 0 };
        _process.memory().Read( reinterpret_cast<ptr_t>(pTls), sizeof( remoteTls ), &remoteTls );

        auto size = remoteTls.EndAddressOfRawData - remoteTls.StartAddressOfRawData;
        std::unique_ptr<uint8_t[]> buf( new uint8_t[size]() );

        _process.memory().Read( remoteTls.StartAddressOfRawData, size, buf.get() );

        tlsStore.Write( 0, tlsStore.ptr<uintptr_t>() + 0x800 );
        tlsStore.Write( 0x800, size, buf.get() );

        if (wxp)
            _process.memory().Write( pTeb + FIELD_OFFSET( _TEB32, ThreadLocalStoragePointer ), tlsStore.ptr<DWORD32>() );
        else
            _process.memory().Write( pTeb + FIELD_OFFSET( _TEB64, ThreadLocalStoragePointer ), tlsStore.ptr<DWORD64>() );

        return STATUS_SUCCESS;
    }

    // Use native method
    if (_LdrpHandleTlsData)
    {
        auto a = AsmFactory::GetAssembler( mod.type );
        uint64_t result = 0;

        a->GenPrologue();
        a->GenCall( _LdrpHandleTlsData, { reinterpret_cast<uintptr_t>(pNode) }, IsWindows8Point1OrGreater() ? cc_thiscall : cc_stdcall );
        _process.remote().AddReturnWithEvent( *a );
        a->GenEpilogue();

        auto status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
        if (!NT_SUCCESS( status ))
            return status;

        return static_cast<NTSTATUS>(result);
    }
    else
        return STATUS_ORDINAL_NOT_FOUND;
}

/// <summary>
/// Create module record in LdrpInvertedFunctionTable
/// Used to create fake SAFESEH entries
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>true on success</returns>
bool NtLdr::InsertInvertedFunctionTable( NtLdrEntry& mod )
{ 
    RTL_INVERTED_FUNCTION_TABLE7 table = { 0 };
    PRTL_INVERTED_FUNCTION_TABLE_ENTRY Entries = nullptr;
    uint64_t result = 0;
    auto a = AsmFactory::GetAssembler( mod.type );
    auto& memory = _process.memory();

    // Invalid addresses. Probably pattern scan has failed
    if (_RtlInsertInvertedFunctionTable == 0 || _LdrpInvertedFunctionTable == 0)
        return false;
    
    if (IsWindows8OrGreater())
        Entries = reinterpret_cast<decltype(Entries)>(GET_FIELD_PTR( reinterpret_cast<PRTL_INVERTED_FUNCTION_TABLE8>(&table), Entries ));
    else
        Entries = reinterpret_cast<decltype(Entries)>(GET_FIELD_PTR( &table, Entries ));

    //
    // Try to find module address in table. 
    // If found - no additional work is required.
    //
    memory.Read( _LdrpInvertedFunctionTable, sizeof( table ), &table );
    for (ULONG i = 0; i < table.Count; i++)
        if (reinterpret_cast<ptr_t>(Entries[i].ImageBase) == mod.baseAddress)
            return true;

    a->GenPrologue();

    if (IsWindows8Point1OrGreater())
        a->GenCall( _RtlInsertInvertedFunctionTable, { mod.baseAddress, mod.size }, cc_fastcall );
    else if (IsWindows8OrGreater())
        a->GenCall( _RtlInsertInvertedFunctionTable, { mod.baseAddress, mod.size } );
    else
        a->GenCall( _RtlInsertInvertedFunctionTable, { _LdrpInvertedFunctionTable, mod.baseAddress, mod.size } );

    _process.remote().AddReturnWithEvent( *a );
    a->GenEpilogue();

    _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
    memory.Read( _LdrpInvertedFunctionTable, sizeof( table ), &table );

    for(DWORD i = 0; i < table.Count; i++)
    {
        if (reinterpret_cast<ptr_t>(Entries[i].ImageBase) != mod.baseAddress)
            continue;
     
        // If Image has SAFESEH, RtlInsertInvertedFunctionTable is enough
        if (Entries[i].SizeOfTable != 0)
            return mod.safeSEH = true;

        //
        // Create fake Exception directory
        // Directory will be filled later, during exception handling
        //
        PIMAGE_RUNTIME_FUNCTION_ENTRY pImgEntry = nullptr;

        // Allocate memory for 512 possible handlers
        auto mem = memory.Allocate( sizeof( DWORD ) * 0x200, PAGE_READWRITE, 0, false );
        if (!mem)
            return false;

        pImgEntry = mem->ptr<decltype(pImgEntry)>();
        auto pEncoded = EncodeSystemPointer( pImgEntry );

        // m_LdrpInvertedFunctionTable->Entries[i].ExceptionDirectory
        uintptr_t field_ofst = reinterpret_cast<uintptr_t>(&Entries[i].ExceptionDirectory)
            - reinterpret_cast<uintptr_t>(&table);

        // In Win10 _LdrpInvertedFunctionTable is located in mrdata section
        // mrdata is read-only by default 
        // LdrProtectMrdata is used to make it writable when needed
        DWORD flOld = 0;
        memory.Protect( _LdrpInvertedFunctionTable + field_ofst, sizeof( uintptr_t ), PAGE_EXECUTE_READWRITE, &flOld );
        auto status = memory.Write( _LdrpInvertedFunctionTable + field_ofst, pEncoded );
        memory.Protect( _LdrpInvertedFunctionTable + field_ofst, sizeof( uintptr_t ), flOld, &flOld );

        return NT_SUCCESS( status );
    }

    return false;
}

/// <summary>
/// Initialize OS-specific module entry
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Pointer to created entry</returns>
_LDR_DATA_TABLE_ENTRY_W8* NtLdr::InitW8Node( NtLdrEntry& mod )
{
    UNICODE_STRING strLocal = { 0 };

    // Allocate space for Unicode string
    auto mem = _process.memory().Allocate( 0x1000, PAGE_READWRITE, 0, false );
    if (!mem)
        return nullptr;

    auto StringBuf = std::move( mem.result() );
    _LDR_DATA_TABLE_ENTRY_W8* pEntry = reinterpret_cast<decltype(pEntry)>(AllocateInHeap( 
        mod.type, sizeof( *pEntry ) 
    ).result( 0 ));

    if(pEntry)
    {
        _LDR_DDAG_NODE* pDdagNode = reinterpret_cast<decltype(pDdagNode)>(AllocateInHeap( 
            mod.type, sizeof( *pDdagNode ) 
        ).result(0));

        if(pDdagNode)
        {
            // pEntry->DllBase = ModuleBase;
            _process.memory().Write( GET_FIELD_PTR( pEntry, DllBase ), static_cast<uintptr_t>(mod.baseAddress) );

            // pEntry->SizeOfImage = ImageSize;
            _process.memory().Write( GET_FIELD_PTR( pEntry, SizeOfImage ), mod.size );

            // pEntry->EntryPoint = entryPoint;
            _process.memory().Write( GET_FIELD_PTR( pEntry, EntryPoint ), static_cast<uintptr_t>(mod.entryPoint) );

            // Dll name and name hash
            SAFE_CALL( RtlInitUnicodeString, &strLocal, mod.name.c_str() );
            mod.hash = HashString( mod.name );

            // Write into buffer
            strLocal.Buffer = StringBuf.ptr<PWSTR>();
            StringBuf.Write( 0, mod.name.length() * sizeof(wchar_t) + 2, mod.name.c_str() );

            // pEntry->BaseDllName = strLocal;
            _process.memory().Write( GET_FIELD_PTR( pEntry, BaseDllName ), strLocal );

            // Dll full path
            SAFE_CALL( RtlInitUnicodeString, &strLocal, mod.fullPath.c_str() );
            strLocal.Buffer = reinterpret_cast<PWSTR>(StringBuf.ptr<uint8_t*>() + 0x800);
            StringBuf.Write( 0x800, mod.fullPath.length() * sizeof(wchar_t) + 2, mod.fullPath.c_str() );
              
            // pEntry->FullDllName = strLocal;
            _process.memory().Write( GET_FIELD_PTR( pEntry, FullDllName ), strLocal );

            // pEntry->BaseNameHashValue = hash;
            _process.memory().Write( GET_FIELD_PTR( pEntry, BaseNameHashValue ), mod.hash );

            //
            // Ddag node
            //

            // pEntry->DdagNode = pDdagNode;
            _process.memory().Write( GET_FIELD_PTR( pEntry, DdagNode ), pDdagNode );

            // pDdagNode->State = LdrModulesReadyToRun;
            _process.memory().Write( GET_FIELD_PTR( pDdagNode, State ), LdrModulesReadyToRun );

            // pDdagNode->ReferenceCount = 1;
            _process.memory().Write( GET_FIELD_PTR( pDdagNode, ReferenceCount ), 1 );

            // pDdagNode->LoadCount = -1;
            _process.memory().Write( GET_FIELD_PTR( pDdagNode, LoadCount ), -1 );

            return pEntry;
        }

        return nullptr;
    }

    return nullptr;
}

/// <summary>
///  Initialize OS-specific module entry
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Pointer to created entry</returns>
_LDR_DATA_TABLE_ENTRY_W7* NtLdr::InitW7Node( NtLdrEntry& mod )
{
    UNICODE_STRING strLocal = { 0 };

    // Allocate space for Unicode string
    auto mem = _process.memory().Allocate( MAX_PATH, PAGE_READWRITE, 0, false );
    if (!mem)
        return nullptr;

    auto StringBuf = std::move( mem.result() );

    _LDR_DATA_TABLE_ENTRY_W7* pEntry = reinterpret_cast<decltype(pEntry)>(AllocateInHeap( 
        mod.type, sizeof( *pEntry ) 
    ).result( 0 ));

    if(pEntry)
    {
        // pEntry->DllBase = ModuleBase;
        _process.memory().Write( GET_FIELD_PTR( pEntry, DllBase ), static_cast<uintptr_t>(mod.baseAddress) );

        // pEntry->SizeOfImage = ImageSize;
        _process.memory().Write( GET_FIELD_PTR( pEntry, SizeOfImage ), mod.size );

        // pEntry->EntryPoint = entryPoint;
        _process.memory().Write( GET_FIELD_PTR( pEntry, EntryPoint ), static_cast<uintptr_t>(mod.entryPoint) );

        // pEntry->LoadCount = -1;
        _process.memory().Write( GET_FIELD_PTR( pEntry, LoadCount ), -1 );

        // Dll name
        SAFE_CALL( RtlInitUnicodeString, &strLocal, mod.name.c_str() );

        // Name hash
        mod.hash = HashString( mod.name );

        // Write into buffer
        strLocal.Buffer = StringBuf.ptr<PWSTR>();
        StringBuf.Write( 0, mod.name.length() * sizeof( wchar_t ) + 2, mod.name.c_str() );

        // pEntry->BaseDllName = strLocal;
        _process.memory().Write( GET_FIELD_PTR( pEntry, BaseDllName ), strLocal );

        // Dll full path
        SAFE_CALL( RtlInitUnicodeString, &strLocal, mod.fullPath.c_str() );
        strLocal.Buffer = reinterpret_cast<PWSTR>(StringBuf.ptr<uint8_t*>() + 0x800);
        StringBuf.Write( 0x800, mod.fullPath.length() * sizeof(wchar_t) + 2, mod.fullPath.c_str() );

        // pEntry->FullDllName = strLocal;
        _process.memory().Write( GET_FIELD_PTR( pEntry, FullDllName ), strLocal );

        // Forward Links
        _process.memory().Write( GET_FIELD_PTR( pEntry, ForwarderLinks ), GET_FIELD_PTR( pEntry, ForwarderLinks ) );
        _process.memory().Write( GET_FIELD_PTR( pEntry, ForwarderLinks ) + WordSize, GET_FIELD_PTR( pEntry, ForwarderLinks ) );

        return pEntry;
    }

    return nullptr;
}

/// <summary>
/// Insert entry into win8 module graph
/// </summary>
/// <param name="pNode">Node to insert</param>
/// <param name="mod">Module data</param>
void NtLdr::InsertTreeNode( _LDR_DATA_TABLE_ENTRY_W8* pNode, const NtLdrEntry& mod )
{
    //
    // Win8 module tree
    //
    auto root = _process.memory().Read<uintptr_t>( _LdrpModuleIndexBase );
    if (!root)
        return;

    _LDR_DATA_TABLE_ENTRY_W8 *pLdrNode = CONTAINING_RECORD( root.result(), _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode );
    _LDR_DATA_TABLE_ENTRY_W8 LdrNode = _process.memory().Read<_LDR_DATA_TABLE_ENTRY_W8>( reinterpret_cast<ptr_t>(pLdrNode) ).result( _LDR_DATA_TABLE_ENTRY_W8() );

    bool bRight = false;

    // Walk tree
    for (;;)
    {
        if (static_cast<uintptr_t>(mod.baseAddress) < LdrNode.DllBase)
        {
            if (LdrNode.BaseAddressIndexNode.Left)
            {
                pLdrNode = CONTAINING_RECORD( LdrNode.BaseAddressIndexNode.Left, _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode );
                _process.memory().Read( reinterpret_cast<ptr_t>(pLdrNode), sizeof(LdrNode), &LdrNode );
            }
            else
                break;
        }
        else if (static_cast<uintptr_t>(mod.baseAddress)  > LdrNode.DllBase)
        {
            if (LdrNode.BaseAddressIndexNode.Right)
            {
                pLdrNode = CONTAINING_RECORD( LdrNode.BaseAddressIndexNode.Right, _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode );
                _process.memory().Read( reinterpret_cast<ptr_t>(pLdrNode), sizeof(LdrNode), &LdrNode );
            }
            else
            {
                bRight = true;
                break;
            }
        }
        // Already in tree (increase ref counter)
        else
        {
            // pLdrNode->DdagNode->ReferenceCount++;
            auto Ddag = _process.memory().Read<_LDR_DDAG_NODE>( reinterpret_cast<ptr_t>(LdrNode.DdagNode) ).result( _LDR_DDAG_NODE() );

            Ddag.ReferenceCount++;

            _process.memory().Write( reinterpret_cast<ptr_t>(LdrNode.DdagNode), Ddag );

            return;
        }
    }

    // Insert using RtlRbInsertNodeEx
    auto a = AsmFactory::GetAssembler( mod.type );
    uint64_t result = 0;
    auto RtlRbInsertNodeEx = _process.modules().GetNtdllExport( "RtlRbInsertNodeEx", mod.type );
    if (!RtlRbInsertNodeEx)
        return;

    a->GenPrologue();
    a->GenCall( static_cast<uintptr_t>(RtlRbInsertNodeEx->procAddress),
    {
        _LdrpModuleIndexBase,
        GET_FIELD_PTR( pLdrNode, BaseAddressIndexNode ),
        static_cast<uintptr_t>(bRight),
        GET_FIELD_PTR( pNode, BaseAddressIndexNode )
    } );

    _process.remote().AddReturnWithEvent( *a, mod.type );
    a->GenEpilogue();

    _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
}

/// <summary>
/// Insert entry into InLoadOrderModuleList and InMemoryOrderModuleList
/// </summary>
/// <param name="pNodeMemoryOrderLink">InMemoryOrderModuleList link of entry to be inserted</param>
/// <param name="pNodeLoadOrderLink">InLoadOrderModuleList link of entry to be inserted</param>
void NtLdr::InsertMemModuleNode( uintptr_t pNodeMemoryOrderLink, uintptr_t pNodeLoadOrderLink, uintptr_t pNodeInitOrderLink )
{
    PEB_T* pPeb = reinterpret_cast<PEB_T*>(_process.core().peb());
    PEB_LDR_DATA_T* pLdr = nullptr;

    if (pPeb)
        pLdr = _process.memory().Read<PEB_LDR_DATA_T*>( GET_FIELD_PTR( pPeb, Ldr ) ).result( nullptr );
                
    if (pLdr)
    {
        // pLdr->InMemoryOrderModuleList
        if (pNodeMemoryOrderLink)
            InsertTailList( GET_FIELD_PTR( pLdr, InMemoryOrderModuleList ), pNodeMemoryOrderLink );

        // pLdr->InLoadOrderModuleList
        if (pNodeLoadOrderLink)
            InsertTailList( GET_FIELD_PTR( pLdr, InLoadOrderModuleList ), pNodeLoadOrderLink );

        // pLdr->InInitializationOrderModuleList
        if (pNodeInitOrderLink)
            InsertTailList( GET_FIELD_PTR( pLdr, InInitializationOrderModuleList ), pNodeInitOrderLink );
    }
}

/// <summary>
/// Insert entry into LdrpHashTable[]
/// </summary>
/// <param name="pNodeLink">Link of entry to be inserted</param>
/// <param name="hash">Module hash</param>
void NtLdr::InsertHashNode( uintptr_t pNodeLink, ULONG hash )
{
    if(pNodeLink)
    {
        // LrpHashTable record
        auto pHashList = _process.memory().Read<uintptr_t>( _LdrpHashTable + sizeof(LIST_ENTRY)*(hash & 0x1F) );
        if(pHashList)
            InsertTailList( pHashList.result(), pNodeLink );
    }
}

/// <summary>
/// Insert entry into standard double linked list
/// </summary>
/// <param name="ListHead">List head pointer</param>
/// <param name="Entry">Entry list link to be inserted</param>
void NtLdr::InsertTailList( uintptr_t ListHead, uintptr_t Entry )
{
    // PrevEntry = ListHead->Blink;
    auto PrevEntry = _process.memory().Read<uintptr_t>( GET_FIELD_PTR( reinterpret_cast<PLIST_ENTRY>(ListHead), Blink ) ).result( 0 );

    // Entry->Flink = ListHead;
    // Entry->Blink = PrevEntry;
    _process.memory().Write( GET_FIELD_PTR( reinterpret_cast<PLIST_ENTRY>(Entry), Flink ), ListHead );
    _process.memory().Write( GET_FIELD_PTR( reinterpret_cast<PLIST_ENTRY>(Entry), Blink ), PrevEntry );

    // PrevEntry->Flink = Entry;
    // ListHead->Blink  = Entry;
    _process.memory().Write( GET_FIELD_PTR( reinterpret_cast<PLIST_ENTRY>(PrevEntry), Flink ), Entry );
    _process.memory().Write( GET_FIELD_PTR( reinterpret_cast<PLIST_ENTRY>(ListHead), Blink ), Entry );
}

/// <summary>
/// Hash image name
/// </summary>
/// <param name="str">Iamge name</param>
/// <returns>Hash</returns>
ULONG NtLdr::HashString( const std::wstring& str )
{
    ULONG hash = 0;

    if (IsWindows8OrGreater())
    {
        UNICODE_STRING ustr;
        SAFE_CALL( RtlInitUnicodeString, &ustr, str.c_str() );
        SAFE_NATIVE_CALL( RtlHashUnicodeString, &ustr, (BOOLEAN)TRUE, 0, &hash );
    }
    else
    {
        for (auto& ch : str)
            hash += 0x1003F * static_cast<unsigned short>(SAFE_CALL( RtlUpcaseUnicodeChar, ch ));
    }

    return hash;
}

/// <summary>
/// Allocate memory from heap if possible
/// </summary>
/// <param name="size">Module type</param>
/// <param name="size">Size to allocate</param>
/// <returns>Allocated address</returns>
call_result_t<ptr_t> NtLdr::AllocateInHeap( eModType mt, size_t size )
{
    NTSTATUS status = STATUS_SUCCESS;
    auto RtlAllocateHeap = _process.modules().GetNtdllExport( "RtlAllocateHeap", mt );
    if (_LdrHeapBase && RtlAllocateHeap)
    {
        auto a = AsmFactory::GetAssembler( mt );

        //
        // HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        //
        a->GenPrologue();
        a->GenCall( RtlAllocateHeap->procAddress, { _LdrHeapBase, HEAP_ZERO_MEMORY, size } );
        _process.remote().AddReturnWithEvent( (*a), mt );
        a->GenEpilogue();

        uint64_t result = 0;
        status = _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );
        if (NT_SUCCESS( status ))
            return result;
    }
    else
        status = STATUS_ORDINAL_NOT_FOUND;

    if (!NT_SUCCESS( status ))
    {
        auto mem = _process.memory().Allocate( size, PAGE_READWRITE, 0, false );
        if (!mem)
            return mem.status;

        return mem->ptr();
    }

    return STATUS_UNSUCCESSFUL;
}

/// <summary>
/// Find LdrpHashTable[] variable
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrpHashTable()
{
    PEB_LDR_DATA_T *Ldr = 
        reinterpret_cast<PEB_LDR_DATA_T*>(
        reinterpret_cast<PEB_T*>(
        reinterpret_cast<TEB_T*>(NtCurrentTeb())->ProcessEnvironmentBlock)->Ldr);

    auto Ntdll = CONTAINING_RECORD( Ldr->InInitializationOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY_BASE_T, InInitializationOrderLinks );

    ULONG NtdllHashIndex = HashString( reinterpret_cast<wchar_t*>(Ntdll->BaseDllName.Buffer) ) & 0x1F;

    ULONG_PTR NtdllBase = static_cast<ULONG_PTR>(Ntdll->DllBase);
    ULONG_PTR NtdllEndAddress = NtdllBase + Ntdll->SizeOfImage - 1;

    // scan hash list to the head (head is located within ntdll)
    PLIST_ENTRY pNtdllHashHead = nullptr;

    for (auto e = reinterpret_cast<PLIST_ENTRY>(Ntdll->HashLinks.Flink); e != reinterpret_cast<PLIST_ENTRY>(&Ntdll->HashLinks); e = e->Flink)
    {
        if (reinterpret_cast<ULONG_PTR>(e) >= NtdllBase && reinterpret_cast<ULONG_PTR>(e) < NtdllEndAddress)
        {
            pNtdllHashHead = e;
            break;
        }
    }

    if (pNtdllHashHead != nullptr)
        _LdrpHashTable = reinterpret_cast<uintptr_t>(pNtdllHashHead - NtdllHashIndex);

    return _LdrpHashTable != 0;
}


/// <summary>
/// Find LdrpModuleIndex variable under win8
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrpModuleIndexBase()
{
    PEB_T* pPeb = reinterpret_cast<PEB_T*>(reinterpret_cast<TEB_T*>(NtCurrentTeb())->ProcessEnvironmentBlock);

    if(pPeb)
    {
        PRTL_BALANCED_NODE lastNode = 0;
        auto Ldr = reinterpret_cast<PEB_LDR_DATA_T*>(pPeb->Ldr);
        auto Ntdll = CONTAINING_RECORD( Ldr->InInitializationOrderModuleList.Flink, _LDR_DATA_TABLE_ENTRY_W8, InInitializationOrderLinks );

        PRTL_BALANCED_NODE pNode = &Ntdll->BaseAddressIndexNode;

        // Get root node
        for(; pNode->ParentValue; )
        {
            // Ignore last few bits
            lastNode = reinterpret_cast<PRTL_BALANCED_NODE>(pNode->ParentValue & uintptr_t( -8 ));
            pNode = lastNode;
        }

        // Get pointer to root
        pe::PEImage ntdll;
        uintptr_t* pStart = nullptr;
        uintptr_t* pEnd = nullptr;

        ntdll.Parse( reinterpret_cast<void*>(Ntdll->DllBase) );

        for (auto& section : ntdll.sections())
            if (_stricmp( reinterpret_cast<LPCSTR>(section.Name), ".data") == 0 )
            {
                pStart = reinterpret_cast<uintptr_t*>(Ntdll->DllBase + section.VirtualAddress);
                pEnd   = reinterpret_cast<uintptr_t*>(Ntdll->DllBase + section.VirtualAddress + section.Misc.VirtualSize);

                break;
            }

        auto iter = std::find( pStart, pEnd, reinterpret_cast<uintptr_t>(lastNode) );

        if (iter != pEnd)
        {
            _LdrpModuleIndexBase = reinterpret_cast<uintptr_t>(iter);
            return true;
        }
    }

    return false;
}


/// <summary>
/// Search for RtlInsertInvertedFunctionTable, LdrpInvertedFunctionTable, LdrpHandleTlsData.
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::ScanPatterns( )
{
    auto& data = g_PatternLoader->data();

    _LdrKernel32PatchAddress = static_cast<uintptr_t>(data.LdrKernel32PatchAddress);
    _APC64PatchAddress = static_cast<uintptr_t>(data.APC64PatchAddress);
    _LdrProtectMrdata = static_cast<uintptr_t>(data.LdrProtectMrdata);

    if (_process.core().isWow64())
    {
        _LdrpHandleTlsData = static_cast<uintptr_t>(data.LdrpHandleTlsData32);
        _LdrpInvertedFunctionTable = static_cast<uintptr_t>(data.LdrpInvertedFunctionTable32);
        _RtlInsertInvertedFunctionTable = static_cast<uintptr_t>(data.RtlInsertInvertedFunctionTable32);
    }
    else
    {
        _LdrpHandleTlsData = static_cast<uintptr_t>(data.LdrpHandleTlsData64);
        _LdrpInvertedFunctionTable = static_cast<uintptr_t>(data.LdrpInvertedFunctionTable64);
        _RtlInsertInvertedFunctionTable = static_cast<uintptr_t>(data.RtlInsertInvertedFunctionTable64);
    }

    return true;
}


/// <summary>
/// Find Loader heap base
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrHeap()
{
    int32_t retries = 10;
    PEB_T Peb = { 0 };

    _process.core().peb( &Peb );
    for (; Peb.Ldr == 0 && retries > 0; retries--, Sleep( 10 ))
        _process.core().peb( &Peb );

    if (Peb.Ldr)
    {
        auto Ldr = _process.memory().Read<PEB_LDR_DATA_T>( Peb.Ldr );
        if (!Ldr)
            return Ldr.status;

        MEMORY_BASIC_INFORMATION64 mbi = { 0 };
        auto NtdllEntry = CONTAINING_RECORD( Ldr->InMemoryOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY_BASE_T, InMemoryOrderLinks );
        if (NT_SUCCESS( _process.core().native()->VirtualQueryExT( reinterpret_cast<ptr_t>(NtdllEntry), &mbi ) ))
        {
            _LdrHeapBase = static_cast<uintptr_t>(mbi.AllocationBase);
            return true;
        }
    }

    return false;
}

/// <summary>
/// Unlink module from Ntdll loader
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>true on success</returns>
bool NtLdr::Unlink( const ModuleData& mod )
{
    ptr_t ldrEntry = 0;

    // Unlink from linked lists
    if (mod.type == mt_mod32)
        ldrEntry = UnlinkFromLdr<DWORD>( mod );
    else
        ldrEntry = UnlinkFromLdr<DWORD64>( mod );

    // Unlink from graph
    // TODO: Unlink from _LdrpMappingInfoIndex. Still can't decide if it is required.
    if ((_process.barrier().type == wow_32_32 || _process.barrier().type == wow_64_64) && ldrEntry && IsWindows8OrGreater())
        UnlinkTreeNode( mod, ldrEntry );

    return ldrEntry != 0;
}

/// <summary>
/// Unlink module from PEB_LDR_DATA
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Address of removed record</returns>
template<typename T>
ptr_t NtLdr::UnlinkFromLdr( const ModuleData& mod )
{
    typename _PEB_T2<T>::type peb = { { { 0 } } };
    _PEB_LDR_DATA2<T> ldr = { 0 };

    auto native = _process.core().native();

    if (native->getPEB( &peb ) != 0 && native->ReadProcessMemoryT( peb.Ldr, &ldr, sizeof(ldr), 0 ) == STATUS_SUCCESS)
    {
        ptr_t ldrEntry = 0;

        // InLoadOrderModuleList
        ldrEntry |= UnlinkListEntry(
            ldr.InLoadOrderModuleList,
            peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<T>, InLoadOrderModuleList ),
            FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, InLoadOrderLinks ),
            mod.baseAddress
            );

        // InMemoryOrderModuleList
        ldrEntry |= UnlinkListEntry(
            ldr.InMemoryOrderModuleList,
            peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<T>, InMemoryOrderModuleList ),
            FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, InMemoryOrderLinks ),
            mod.baseAddress
            );

        // InInitializationOrderModuleList
        ldrEntry |= UnlinkListEntry(
            ldr.InInitializationOrderModuleList,
            peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<T>, InInitializationOrderModuleList ),
            FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, InInitializationOrderLinks ),
            mod.baseAddress
            );

        // Hash table
        if (ldrEntry == 0)
        { 
            //
            // Search module in hash list
            //
            auto pHashList = _LdrpHashTable + sizeof( _LIST_ENTRY_T<T> )*(HashString( mod.name ) & 0x1F);
            auto hashList = _process.memory().Read<_LIST_ENTRY_T<T>>( pHashList ).result( _LIST_ENTRY_T<T>() );

            UnlinkListEntry( hashList, pHashList, FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, HashLinks ), mod.baseAddress );
        }
        else
            UnlinkListEntry<T>( ldrEntry + FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, HashLinks ) );

        return ldrEntry;
    }

    return 0;
}

/// <summary>
/// Search and remove record from LIST_ENTRY structure
/// </summary>
/// <param name="pListEntry">List to remove from</param>
/// <param name="head">List head address</param>
/// <param name="ofst">Offset of link in _LDR_DATA_TABLE_ENTRY_BASE struct</param>
/// <param name="baseAddress">Record to remove.</param>
/// <returns>Address of removed record</returns>
template<typename T>
ptr_t NtLdr::UnlinkListEntry( _LIST_ENTRY_T<T> pListEntry, ptr_t head, uintptr_t ofst, ptr_t baseAddress )
{
    auto native = _process.core().native();

    for (T entry = pListEntry.Flink; entry != 0 && entry != head; native->ReadProcessMemoryT( entry, &entry, sizeof( entry ) ))
    {
        _LDR_DATA_TABLE_ENTRY_BASE<T> modData = { { 0 } };

        native->ReadProcessMemoryT( entry - ofst, &modData, sizeof(modData), 0 );

        // Unlink if found
        if (modData.DllBase == (T)baseAddress)
        {
            T OldFlink = _process.memory().Read<T>( entry + FIELD_OFFSET( _LIST_ENTRY_T<T>, Flink ) ).result( 0 );
            T OldBlink = _process.memory().Read<T>( entry + FIELD_OFFSET( _LIST_ENTRY_T<T>, Blink ) ).result( 0 );

            // OldFlink->Blink = OldBlink;
            _process.memory().Write(OldFlink + FIELD_OFFSET( _LIST_ENTRY_T<T>, Blink ), OldBlink);

            // OldBlink->Flink = OldFlink;
            _process.memory().Write(OldBlink + FIELD_OFFSET( _LIST_ENTRY_T<T>, Flink ), OldFlink);

            return entry - ofst;
        }
    }

    return 0;
}

/// <summary>
///  Remove record from LIST_ENTRY structure
/// </summary>
/// <param name="pListLink">Entry link</param>
template<typename T>
void NtLdr::UnlinkListEntry( ptr_t pListLink )
{
    T OldFlink = _process.memory().Read<T>( pListLink + FIELD_OFFSET( _LIST_ENTRY_T<T>, Flink ) ).result( 0 );
    T OldBlink = _process.memory().Read<T>( pListLink + FIELD_OFFSET( _LIST_ENTRY_T<T>, Blink ) ).result( 0 );

    // List is empty
    if (OldBlink == 0 || OldFlink == 0 || OldBlink == OldFlink)
        return;

    // OldFlink->Blink = OldBlink;
    _process.memory().Write( OldFlink + FIELD_OFFSET( _LIST_ENTRY_T<T>, Blink ), OldBlink );

    // OldBlink->Flink = OldFlink;
    _process.memory().Write( OldBlink + FIELD_OFFSET( _LIST_ENTRY_T<T>, Flink ), OldFlink );
}

/// <summary>
/// Unlink from module graph
/// </summary>
/// <param name="mod">Module data</param>
/// <param name="ldrEntry">Module LDR entry</param>
/// <returns>Address of removed record</returns>
ptr_t NtLdr::UnlinkTreeNode( const ModuleData& mod, ptr_t ldrEntry )
{
    auto a = AsmFactory::GetAssembler( mod.type );
    uint64_t result = 0;

    auto RtlRbRemoveNode = _process.modules().GetNtdllExport( "RtlRbRemoveNode" );
    if (!RtlRbRemoveNode)
        return RtlRbRemoveNode.status;

    a->GenPrologue();
    a->GenCall( static_cast<uintptr_t>(RtlRbRemoveNode->procAddress),
    {
        _LdrpModuleIndexBase,
        ldrEntry + FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode )
    } );

    _process.remote().AddReturnWithEvent( *a );
    a->GenEpilogue();

    _process.remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize(), result );

    return ldrEntry;
}

}
