#include "NtLoader.h"
#include "../../Process/Process.h"
#include "../../Include/Macro.h"
#include "../../Misc/Utils.h"
#include "../../Misc/DynImport.h"
#include "../../Misc/trace.hpp"
#include "../../Symbols/SymbolData.h"

#include <3rd_party/VersionApi.h>

namespace blackbone
{

NtLdr::NtLdr( Process* proc )
    : _process( proc )
{
}

/// <summary>
/// Initialize some loader stuff
/// </summary>
/// <param name="initFor">Target module type</param>
void NtLdr::Init( eModType initFor /*= mt_default*/ )
{
    // Sanity check, already initialized
    assert( initFor != _initializedFor );
    if (initFor == _initializedFor)
        return;

    // Detect target module type
    _initializedFor = initFor;
    if (_initializedFor == mt_unknown)
        _initializedFor = _process->core().isWow64() ? mt_mod32 : mt_mod64;

    auto callNoThrow = [this]( auto fn, auto... args )
    {
        try
        {
            (this->*fn)(args...);
        }
        catch (const std::exception& e)
        {
            BLACKBONE_TRACE( "NativeLdr: %s", e.what() );
        }
    };

    auto findAll = [this, callNoThrow]( auto val )
    {
        callNoThrow( &NtLdr::FindLdrHeap<decltype(val)> );
        callNoThrow( &NtLdr::FindLdrpHashTable<decltype(val)> );
        if (IsWindows8OrGreater())
            callNoThrow( &NtLdr::FindLdrpModuleIndexBase<decltype(val)> );
    };

    // Select loader version
    if (_initializedFor == mt_mod32)
        findAll( uint32_t{} );
    else
        findAll( uint64_t{} );

    _nodeMap.clear();

    // Report errors
#ifndef BLACKBONE_NO_TRACE
    if (_LdrHeapBase == 0)
        BLACKBONE_TRACE( "NativeLdr: LdrHeapBase not found" );
    if (_LdrpHashTable == 0)
        BLACKBONE_TRACE( "NativeLdr: LdrpHashTable not found" );
    if (IsWindows8OrGreater() && _LdrpModuleIndexBase == 0)
        BLACKBONE_TRACE( "NativeLdr: LdrpModuleIndexBase not found" );
#endif
}

/// <summary>
/// Add module to some loader structures 
/// (LdrpHashTable, LdrpModuleIndex( win8+ only ), InMemoryOrderModuleList( win7 only ))
/// </summary>
/// <param name="mod">Module data</param>
void NtLdr::CreateNTReference( NtLdrEntry& mod )
{
    // Skip
    if (mod.flags == Ldr_None)
        return;

    // Check if reinitialization is required
    if (_initializedFor != mod.type)
        Init( mod.type );

    bool x64Image = (mod.type == mt_mod64);
    bool w8 = IsWindows8OrGreater();

    // Win 8 and higher
    if (w8)
        mod.ldrPtr = CALL_64_86( x64Image, InitW8Node, mod );
    // Windows 7 and earlier
    else
        mod.ldrPtr = CALL_64_86( x64Image, InitW7Node, mod );

    // Should not happen, exception should have been thrown earlier
    if (mod.ldrPtr == 0)
        THROW_AND_LOG( "failed to allocated ldr node memory" );

    _nodeMap.emplace( mod.baseAddress, mod.ldrPtr );

    // Insert into module graph
    if (mod.flags & Ldr_ModList && w8)
        CALL_64_86( x64Image, InsertTreeNode, mod.ldrPtr, mod );

    // Insert into LdrpHashTable
    if (mod.flags & Ldr_HashTable)
    {
        auto ptr = FIELD_PTR_64_86( x64Image, mod.ldrPtr, _LDR_DATA_TABLE_ENTRY_BASE_T, HashLinks );
        CALL_64_86( x64Image, InsertHashNode, ptr, mod.hash );
    }

    // Insert into ldr lists
    if (mod.flags & Ldr_ThdCall || (!w8 && mod.flags & Ldr_ModList))
    {
        _process->memory().Write( FIELD_PTR_64_86( x64Image, mod.ldrPtr, _LDR_DATA_TABLE_ENTRY_BASE_T, Flags ), 0x80004 );
        ptr_t loadPtr = 0, initptr = 0;

        loadPtr = FIELD_PTR_64_86( x64Image, mod.ldrPtr, _LDR_DATA_TABLE_ENTRY_BASE_T, InLoadOrderLinks );
        if (w8)
            initptr = FIELD_PTR_64_86( x64Image, mod.ldrPtr, _LDR_DATA_TABLE_ENTRY_BASE_T, InInitializationOrderLinks );

        CALL_64_86( x64Image, InsertMemModuleNode, 0, loadPtr, initptr );
    }
}

/// <summary>
/// Get module native node ptr or create new
/// </summary>
/// <param name="ptr">node pointer (if nullptr - new dummy node is allocated)</param>
/// <param name="pModule">Module base address</param>
/// <returns>Node address</returns>
template<typename T, typename Module>
ptr_t NtLdr::SetNode( ptr_t ptr, Module pModule )
{
    if (ptr == 0)
    {
        auto mem = _process->memory().Allocate( sizeof( T ), PAGE_READWRITE, 0, false );
        ptr = mem.ptr();
        mem.Write( offsetOf( &T::DllBase ), pModule );
    }

    return ptr;
}

/// <summary>
/// Create thread static TLS array
/// </summary>
/// <param name="mod">Module data</param>
/// <param name="tlsPtr">TLS directory of target image</param>
void NtLdr::AddStaticTLSEntry( NtLdrEntry& mod, ptr_t tlsPtr )
{
    bool wxp = IsWindowsXPOrGreater() && !IsWindowsVistaOrGreater();
    ptr_t pNode = _nodeMap.count( mod.baseAddress ) ? _nodeMap[mod.baseAddress] : 0;

    // Allocate appropriate structure
    ptr_t LdrpHandleTlsData = 0;
    if (mod.type == mt_mod64)
    {
        LdrpHandleTlsData = g_symbols.LdrpHandleTlsData64;
        pNode = SetNode<_LDR_DATA_TABLE_ENTRY_BASE64>( pNode, mod.baseAddress );
    }
    else
    {
        LdrpHandleTlsData = g_symbols.LdrpHandleTlsData32;
        pNode = SetNode<_LDR_DATA_TABLE_ENTRY_BASE32>( pNode, mod.baseAddress );
    }

    // Update ptr
    if (mod.ldrPtr == 0)
        mod.ldrPtr = pNode;

    // Manually add TLS table
    if (wxp && tlsPtr != 0)
    {
        ptr_t pTeb = _process->remote().getExecThread()->teb32();

        auto tlsStore = _process->memory().Allocate( 0x1000, PAGE_READWRITE, 0, false );

        IMAGE_TLS_DIRECTORY remoteTls = { };
        _process->memory().Read( tlsPtr, sizeof( remoteTls ), &remoteTls );

        auto size = remoteTls.EndAddressOfRawData - remoteTls.StartAddressOfRawData;
        std::unique_ptr<uint8_t[]> buf( new uint8_t[size]() );

        _process->memory().Read( remoteTls.StartAddressOfRawData, size, buf.get() );

        tlsStore.Write( 0, tlsStore.ptr<uintptr_t>() + 0x800 );
        tlsStore.Write( 0x800, size, buf.get() );

        _process->memory().Write( fieldPtr( pTeb, &_TEB32::ThreadLocalStoragePointer ), tlsStore.ptr<uint32_t>() );
    }
    // Use native method
    if (!LdrpHandleTlsData)
        THROW_AND_LOG( "LdrpHandleTlsData not found" );

    auto a = AsmFactory::GetAssembler( mod.type );

    a->GenPrologue();
    a->GenCall( LdrpHandleTlsData, { pNode }, IsWindows8Point1OrGreater() ? cc_thiscall : cc_stdcall );
    _process->remote().AddReturnWithEvent( *a );
    a->GenEpilogue();

    NTSTATUS status = static_cast<NTSTATUS>(_process->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() ));
    THROW_ON_FAIL_AND_LOG( status, "LdrpHandleTlsData failed" );
}

/// <summary>
/// Create module record in LdrpInvertedFunctionTable
/// Used to create fake SAFESEH entries
/// </summary>
/// <param name="mod">Module data</param>
void NtLdr::InsertInvertedFunctionTable( NtLdrEntry& mod )
{
    ptr_t RtlInsertInvertedFunctionTable = g_symbols.RtlInsertInvertedFunctionTable64;
    ptr_t LdrpInvertedFunctionTable = g_symbols.LdrpInvertedFunctionTable64;
    if (mod.type == mt_mod32)
    {
        RtlInsertInvertedFunctionTable = g_symbols.RtlInsertInvertedFunctionTable32;
        LdrpInvertedFunctionTable = g_symbols.LdrpInvertedFunctionTable32;
    }

    // Invalid addresses. Probably pattern scan has failed
    if (!RtlInsertInvertedFunctionTable || !LdrpInvertedFunctionTable)
        THROW_AND_LOG( "RtlInsertInvertedFunctionTable or LdrpInvertedFunctionTable not found" );

    auto InsertP = [&]( auto table )
    {
        auto a = AsmFactory::GetAssembler( mod.type );
        auto& memory = _process->memory();

        memory.Read( LdrpInvertedFunctionTable, sizeof( table ), &table );
        for (ULONG i = 0; i < table.Count; i++)
            if (table.Entries[i].ImageBase == mod.baseAddress)
                return;

        a->GenPrologue();

        if (IsWindows8Point1OrGreater())
            a->GenCall( RtlInsertInvertedFunctionTable, { mod.baseAddress, mod.size }, cc_fastcall );
        else if (IsWindows8OrGreater())
            a->GenCall( RtlInsertInvertedFunctionTable, { mod.baseAddress, mod.size } );
        else
            a->GenCall( RtlInsertInvertedFunctionTable, { LdrpInvertedFunctionTable, mod.baseAddress, mod.size } );

        _process->remote().AddReturnWithEvent( *a );
        a->GenEpilogue();

        _process->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() );
        memory.Read( LdrpInvertedFunctionTable, sizeof( table ), &table );

        for (DWORD i = 0; i < table.Count; i++)
        {
            if (table.Entries[i].ImageBase != mod.baseAddress)
                continue;

            // If Image has SAFESEH, RtlInsertInvertedFunctionTable is enough
            if (table.Entries[i].SizeOfTable != 0)
            {
                mod.safeSEH = true;
                return;
            }

            //
            // Create fake Exception directory
            // Directory will be filled later, during exception handling
            //

            // Allocate memory for 2048 possible handlers
            auto mem = memory.Allocate( sizeof( DWORD ) * 0x800, PAGE_READWRITE, 0, false );

            // EncodeSystemPointer( mem->ptr() )
            uint32_t size = 0;
            ptr_t pEncoded = 0;
            auto cookie = *reinterpret_cast<uint32_t*>(0x7FFE0330);
            if (mod.type == mt_mod64)
            {
                size = sizeof( uint64_t );
                pEncoded = _rotr64( cookie ^ mem.ptr<uint64_t>(), cookie & 0x3F );
            }
            else
            {
                size = sizeof( uint32_t );
                pEncoded = _rotr( cookie ^ mem.ptr<uint32_t>(), cookie & 0x1F );
            }

            // m_LdrpInvertedFunctionTable->Entries[i].ExceptionDirectory
            uintptr_t field_ofst = reinterpret_cast<ptr_t>(&table.Entries[i].ExceptionDirectory)
                - reinterpret_cast<ptr_t>(&table);

            // In Win10 LdrpInvertedFunctionTable is located in mrdata section
            // mrdata is read-only by default 
            // LdrProtectMrdata is used to make it writable when needed
            DWORD flOld = 0;
            memory.Protect( LdrpInvertedFunctionTable + field_ofst, sizeof( ptr_t ), PAGE_EXECUTE_READWRITE, &flOld );
            memory.Write( LdrpInvertedFunctionTable + field_ofst, size, &pEncoded );
            memory.Protect( LdrpInvertedFunctionTable + field_ofst, sizeof( ptr_t ), flOld, &flOld );

            return;
        }

        THROW_AND_LOG( "could not find target module in LdrpInvertedFunctionTable" );
    };

    if (IsWindows8OrGreater())
    {
        if (mod.type == mt_mod64)
            return InsertP( _RTL_INVERTED_FUNCTION_TABLE8<DWORD64>() );
        else
            return InsertP( _RTL_INVERTED_FUNCTION_TABLE8<DWORD>() );
    }
    else
    {
        if (mod.type == mt_mod64)
            return InsertP( _RTL_INVERTED_FUNCTION_TABLE7<DWORD64>() );
        else
            return InsertP( _RTL_INVERTED_FUNCTION_TABLE7<DWORD>() );
    }
}

/// <summary>
/// Free static TLS
/// </summary>
/// <param name="mod">Target module</param>
/// <param name="noThread">Don't create new threads during remote call</param>
/// <returns>Status code</returns>
NTSTATUS NtLdr::UnloadTLS( const NtLdrEntry& mod, bool noThread /*= false*/ )
{
    // No loader entry to free
    if (mod.ldrPtr == 0)
        return STATUS_INVALID_ADDRESS;

    ptr_t LdrpReleaseTlsEntry = g_symbols.LdrpReleaseTlsEntry64;
    if (mod.type == mt_mod32)
        LdrpReleaseTlsEntry = g_symbols.LdrpReleaseTlsEntry32;

    // Not available
    if (LdrpReleaseTlsEntry == 0)
        return STATUS_ORDINAL_NOT_FOUND;

    auto a = AsmFactory::GetAssembler( mod.type );

    a->GenPrologue();
    a->GenCall( LdrpReleaseTlsEntry, { mod.ldrPtr, 0 }, IsWindows8Point1OrGreater() ? cc_fastcall : cc_stdcall );

    _process->remote().AddReturnWithEvent( *a );
    a->GenEpilogue();

    _process->remote().CreateRPCEnvironment( noThread ? Worker_UseExisting : Worker_CreateNew, true );
    _process->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() );

    return STATUS_SUCCESS;
}

/// <summary>
///  Initialize OS-specific module entry
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Pointer to created entry</returns>
template<typename T>
ptr_t NtLdr::InitBaseNode( NtLdrEntry& mod )
{
    using EntryType = _LDR_DATA_TABLE_ENTRY_BASE_T<T>;
    ptr_t entryPtr = AllocateInHeap( mod.type, sizeof( _LDR_DATA_TABLE_ENTRY_W8<T> ) );
    assert( entryPtr != 0 );

    // Allocate space for Unicode string
    _UNICODE_STRING_T<T> strLocal = { };
    auto StringBuf = _process->memory().Allocate( 0x1000, PAGE_READWRITE, 0, false );

    // entryPtr->DllBase = ModuleBase;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::DllBase ), static_cast<T>(mod.baseAddress) );

    // entryPtr->SizeOfImage = ImageSize;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::SizeOfImage ), mod.size );

    // entryPtr->EntryPoint = entryPoint;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::EntryPoint ), static_cast<T>(mod.entryPoint) );

    // Dll name hash
    mod.hash = HashString( mod.name );

    // Dll name
    strLocal.Length = static_cast<WORD>(mod.name.length() * sizeof( wchar_t ));
    strLocal.MaximumLength = 0x600;
    strLocal.Buffer = StringBuf.ptr<T>();
    StringBuf.Write( 0, strLocal.Length + 2, mod.name.c_str() );

    // entryPtr->BaseDllName = strLocal;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::BaseDllName ), strLocal );

    // Dll full path
    strLocal.Length = static_cast<WORD>(mod.fullPath.length() * sizeof( wchar_t ));
    strLocal.Buffer = StringBuf.ptr<T>() + 0x800;
    StringBuf.Write( 0x800, strLocal.Length + 2, mod.fullPath.c_str() );

    // entryPtr->FullDllName = strLocal;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::FullDllName ), strLocal );

    // entryPtr->LoadCount = -1;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::LoadCount ), strLocal );

    return entryPtr;
}

/// <summary>
/// Initialize OS-specific module entry
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Pointer to created entry</returns>
template<typename T>
ptr_t NtLdr::InitW8Node( NtLdrEntry& mod )
{
    using EntryType = _LDR_DATA_TABLE_ENTRY_W8<T>;
    using DdagType = _LDR_DDAG_NODE<T>;

    ptr_t entryPtr = InitBaseNode<T>( mod );
    ptr_t DdagNodePtr = AllocateInHeap( mod.type, sizeof( DdagType ) );

    // entryPtr->BaseNameHashValue = hash;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::BaseNameHashValue ), mod.hash );

    //
    // Ddag node
    //

    // entryPtr->DdagNode = pDdagNode;
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::DdagNode ), static_cast<T>(DdagNodePtr) );

    // DdagNodePtr->State = LdrModulesReadyToRun;
    _process->memory().Write( fieldPtr( DdagNodePtr, &DdagType::State ), LdrModulesReadyToRun );

    // DdagNodePtr->ReferenceCount = 1;
    _process->memory().Write( fieldPtr( DdagNodePtr, &DdagType::ReferenceCount ), 1 );

    // DdagNodePtr->LoadCount = -1;
    _process->memory().Write( fieldPtr( DdagNodePtr, &DdagType::LoadCount ), -1 );

    return entryPtr;
}

/// <summary>
///  Initialize OS-specific module entry
/// </summary>
/// <param name="mod">Module data</param>
/// <returns>Pointer to created entry</returns>
template<typename T>
ptr_t NtLdr::InitW7Node( NtLdrEntry& mod )
{
    using EntryType = _LDR_DATA_TABLE_ENTRY_W7<T>;

    ptr_t entryPtr = InitBaseNode<T>( mod );
    if (!entryPtr)
        return 0;

    // Forward Links
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::ForwarderLinks ), fieldPtr( entryPtr, &EntryType::ForwarderLinks ) );
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::ForwarderLinks ) + sizeof( T ), fieldPtr( entryPtr, &EntryType::ForwarderLinks ) );

    // Static links
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::StaticLinks ), fieldPtr( entryPtr, &EntryType::StaticLinks ) );
    _process->memory().Write( fieldPtr( entryPtr, &EntryType::StaticLinks ) + sizeof( T ), fieldPtr( entryPtr, &EntryType::StaticLinks ) );

    return entryPtr;
}

/// <summary>
/// Insert entry into win8 module graph
/// </summary>
/// <param name="nodePtr">Node to insert</param>
/// <param name="mod">Module data</param>
template<typename T>
void NtLdr::InsertTreeNode( ptr_t nodePtr, const NtLdrEntry& mod )
{
    //
    // Win8 module tree
    //
    auto root = _process->memory().Read<T>( _LdrpModuleIndexBase );
    auto LdrNodePtr = structBase( root, &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode );
    auto LdrNode = _process->memory().Read<_LDR_DATA_TABLE_ENTRY_W8<T>>( LdrNodePtr );
    bool bRight = false;

    // Walk tree
    for (;;)
    {
        if (static_cast<T>(mod.baseAddress) < LdrNode.DllBase)
        {
            if (LdrNode.BaseAddressIndexNode.Left)
            {
                LdrNodePtr = structBase( LdrNode.BaseAddressIndexNode.Left, &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode );
                _process->memory().Read( LdrNodePtr, sizeof( LdrNode ), &LdrNode );
            }
            else
                break;
        }
        else if (static_cast<T>(mod.baseAddress) > LdrNode.DllBase)
        {
            if (LdrNode.BaseAddressIndexNode.Right)
            {
                LdrNodePtr = structBase( LdrNode.BaseAddressIndexNode.Right, &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode );
                _process->memory().Read( LdrNodePtr, sizeof( LdrNode ), &LdrNode );
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
            auto Ddag = _process->memory().Read<_LDR_DDAG_NODE<T>>( LdrNode.DdagNode );
            Ddag.ReferenceCount++;
            _process->memory().Write( LdrNode.DdagNode, Ddag );
            return;
        }
    }

    // Insert using RtlRbInsertNodeEx
    auto a = AsmFactory::GetAssembler( mod.type );
    auto RtlRbInsertNodeEx = _process->modules().GetNtdllExport( "RtlRbInsertNodeEx", mod.type );

    a->GenPrologue();
    a->GenCall( RtlRbInsertNodeEx.procAddress,
        {
            _LdrpModuleIndexBase,
            fieldPtr( LdrNodePtr, &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode ),
            static_cast<T>(bRight),
            fieldPtr( nodePtr, &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode )
        } );

    _process->remote().AddReturnWithEvent( *a, mod.type );
    a->GenEpilogue();

    _process->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() );
}

/// <summary>
/// Insert entry into InLoadOrderModuleList and InMemoryOrderModuleList
/// </summary>
/// <param name="pNodeMemoryOrderLink">InMemoryOrderModuleList link of entry to be inserted</param>
/// <param name="pNodeLoadOrderLink">InLoadOrderModuleList link of entry to be inserted</param>
template<typename T>
void NtLdr::InsertMemModuleNode( ptr_t pNodeMemoryOrderLink, ptr_t pNodeLoadOrderLink, ptr_t pNodeInitOrderLink )
{
    ptr_t pPeb = _process->core().peb( static_cast<_PEB_T<T>*>(nullptr) );
    ptr_t pLdr = 0;

    if (pPeb)
        pLdr = _process->memory().Read<T>( fieldPtr( pPeb, &PEB_T::Ldr ) );

    if (pLdr)
    {
        // pLdr->InMemoryOrderModuleList
        if (pNodeMemoryOrderLink)
            InsertTailList<T>( fieldPtr( pLdr, &_PEB_LDR_DATA2_T<T>::InMemoryOrderModuleList ), pNodeMemoryOrderLink );

        // pLdr->InLoadOrderModuleList
        if (pNodeLoadOrderLink)
            InsertTailList<T>( fieldPtr( pLdr, &_PEB_LDR_DATA2_T<T>::InLoadOrderModuleList ), pNodeLoadOrderLink );

        // pLdr->InInitializationOrderModuleList
        if (pNodeInitOrderLink)
            InsertTailList<T>( fieldPtr( pLdr, &_PEB_LDR_DATA2_T<T>::InInitializationOrderModuleList ), pNodeInitOrderLink );
    }
}

/// <summary>
/// Insert entry into LdrpHashTable[]
/// </summary>
/// <param name="pNodeLink">Link of entry to be inserted</param>
/// <param name="hash">Module hash</param>
template<typename T>
void NtLdr::InsertHashNode( ptr_t pNodeLink, ULONG hash )
{
    if (pNodeLink)
    {
        // LrpHashTable record
        auto pHashList = _process->memory().Read<T>( _LdrpHashTable + sizeof( _LIST_ENTRY_T<T> )*(hash & 0x1F) );
        if (pHashList)
            InsertTailList<T>( pHashList, pNodeLink );
    }
}

/// <summary>
/// Insert entry into standard double linked list
/// </summary>
/// <param name="ListHead">List head pointer</param>
/// <param name="Entry">Entry list link to be inserted</param>
template<typename T>
void NtLdr::InsertTailList( ptr_t ListHead, ptr_t Entry )
{
    // PrevEntry = ListHead->Blink;
    auto PrevEntry = _process->memory().Read<T>( fieldPtr( ListHead, &_LIST_ENTRY_T<T>::Blink ) );

    // Entry->Flink = ListHead;
    // Entry->Blink = PrevEntry;
    _process->memory().Write( fieldPtr( Entry, &_LIST_ENTRY_T<T>::Flink ), sizeof( T ), &ListHead );
    _process->memory().Write( fieldPtr( Entry, &_LIST_ENTRY_T<T>::Blink ), sizeof( T ), &PrevEntry );

    // PrevEntry->Flink = Entry;
    // ListHead->Blink  = Entry;
    _process->memory().Write( fieldPtr( PrevEntry, &_LIST_ENTRY_T<T>::Flink ), sizeof( T ), &Entry );
    _process->memory().Write( fieldPtr( ListHead, &_LIST_ENTRY_T<T>::Blink ), sizeof( T ), &Entry );
}

/// <summary>
/// Hash image name
/// </summary>
/// <param name="str">Image name</param>
/// <returns>Hash</returns>
ULONG NtLdr::HashString( const std::wstring& str )
{
    ULONG hash = 0;

    if (IsWindows8OrGreater())
    {
        UNICODE_STRING ustr;
        SAFE_CALL( RtlInitUnicodeString, &ustr, str.c_str() );
        SAFE_NATIVE_CALL( RtlHashUnicodeString, &ustr, BOOLEAN(TRUE), 0, &hash );
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
ptr_t NtLdr::AllocateInHeap( eModType mt, size_t size )
{
    auto RtlAllocateHeap = _process->modules().GetNtdllExport( "RtlAllocateHeap", mt );
    if (_LdrHeapBase)
    {
        auto a = AsmFactory::GetAssembler( mt );

        //
        // HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        //
        a->GenPrologue();
        a->GenCall( RtlAllocateHeap.procAddress, { _LdrHeapBase, HEAP_ZERO_MEMORY, size } );
        _process->remote().AddReturnWithEvent( (*a), mt );
        a->GenEpilogue();

        if (auto ptr = _process->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() ); ptr)
            return ptr;
    }

    // Fall back to VirtualAlloc
    auto mem = _process->memory().Allocate( size, PAGE_READWRITE, 0, false );
    return mem.ptr();
}

/// <summary>
/// Find LdrpHashTable[] variable
/// </summary>
template<typename T>
void NtLdr::FindLdrpHashTable()
{
    _PEB_T<T> Peb = { };
    _process->core().peb( &Peb );
    if (Peb.ImageBaseAddress == 0)
        THROW_WITH_STATUS_AND_LOG( LastNtStatus(), "failed to get process base image address" );

    auto Ldr = _process->memory().Read<_PEB_LDR_DATA2_T<T>>( Peb.Ldr );

    // Get loader entry
    auto entryPtr = structBase( Ldr.InInitializationOrderModuleList.Flink, &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::InInitializationOrderLinks );
    auto entry = _process->memory().Read<_LDR_DATA_TABLE_ENTRY_BASE_T<T>>( entryPtr );

    wchar_t nameBuf[260] = { };
    _process->memory().Read( entry.BaseDllName.Buffer, entry.BaseDllName.Length + 2, nameBuf );

    ULONG NtdllHashIndex = HashString( nameBuf ) & 0x1F;
    T NtdllBase = static_cast<T>(entry.DllBase);
    T NtdllEndAddress = NtdllBase + entry.SizeOfImage - 1;

    // scan hash list to the head (head is located within ntdll)
    T NtdllHashHeadPtr = 0;
    _LIST_ENTRY_T<T> hashNode = entry.HashLinks;

    for (auto e = hashNode.Flink; e != fieldPtr( entryPtr, &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::HashLinks ); e = hashNode.Flink)
    {
        if (e >= NtdllBase && e < NtdllEndAddress)
        {
            NtdllHashHeadPtr = e;
            break;
        }

        _process->memory().Read( hashNode.Flink, hashNode );
    }

    if (NtdllHashHeadPtr == 0)
        THROW_AND_LOG( "failed to get LdrpHashTableHead ptr" );

    _LdrpHashTable = NtdllHashHeadPtr - NtdllHashIndex * sizeof( _LIST_ENTRY_T<T> );
}


/// <summary>
/// Find LdrpModuleIndex variable under win8
/// </summary>
template<typename T>
void NtLdr::FindLdrpModuleIndexBase()
{
    _PEB_T<T> Peb = { };
    _process->core().peb( &Peb );

    if (Peb.ImageBaseAddress == 0)
        THROW_WITH_STATUS_AND_LOG( LastNtStatus(), "failed to get process base image address" );

    T lastNode = 0;
    auto Ldr = _process->memory().Read<_PEB_LDR_DATA2_T<T>>( Peb.Ldr );
    auto entryPtr = structBase( Ldr.InInitializationOrderModuleList.Flink, &_LDR_DATA_TABLE_ENTRY_W8<T>::InInitializationOrderLinks );

    auto node = _process->memory().Read<_RTL_BALANCED_NODE<T>>( fieldPtr( entryPtr, &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode ) );

    // Get root node
    for (; node.ParentValue; )
    {
        // Ignore last few bits
        lastNode = node.ParentValue & T( -8 );
        _process->memory().Read( lastNode, node );
    }

    // Get pointer to root
    pe::PEImage ntdllPE;
    T* pStart = nullptr;
    T* pEnd = nullptr;

    auto pNtdll = _process->modules().GetModule( L"ntdll.dll" );
    std::unique_ptr<uint8_t[]> localBuf( new uint8_t[pNtdll->size] );
    _process->memory().Read( pNtdll->baseAddress, pNtdll->size, localBuf.get() );

    ntdllPE.Parse( localBuf.get() );

    for (auto& section : ntdllPE.sections())
        if (_stricmp( reinterpret_cast<LPCSTR>(section.Name), ".data" ) == 0)
        {
            pStart = reinterpret_cast<T*>(localBuf.get() + section.VirtualAddress);
            pEnd = reinterpret_cast<T*>(localBuf.get() + section.VirtualAddress + section.Misc.VirtualSize);
            break;
        }

    auto iter = std::find( pStart, pEnd, lastNode );
    if (iter == pEnd)
        THROW_AND_LOG( "lastNode not found" );

    _LdrpModuleIndexBase = REBASE( iter, localBuf.get(), pNtdll->baseAddress );
}

/// <summary>
/// Find Loader heap base
/// </summary>
template<typename T>
void NtLdr::FindLdrHeap()
{
    _PEB_T<T> peb = { };
    if (_process->core().peb( &peb ) != 0)
        _LdrHeapBase = peb.ProcessHeap;
}

/// <summary>
/// Unlink module from Ntdll loader
/// </summary>
/// <param name="mod">Module data</param>
/// <param name="noThread">Don't create new threads during unlink</param>
/// <returns>true on success</returns>
bool NtLdr::Unlink( const ModuleData& mod, bool noThread /*= false*/ )
{
    ptr_t ldrEntry = 0;
    auto x64Image = mod.type == mt_mod64;

    // Reinitialize
    if (_initializedFor != mod.type)
        Init( mod.type );

    // Unlink from linked lists
    ldrEntry = CALL_64_86( x64Image, UnlinkFromLdr, mod );

    // Unlink from graph
    // TODO: Unlink from _LdrpMappingInfoIndex. Still can't decide if it is required.
    if (IsWindows8OrGreater())
        ldrEntry = CALL_64_86( x64Image, UnlinkTreeNode, mod, ldrEntry, noThread );

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
    auto ldrEntry = mod.ldrPtr;
    if (ldrEntry == 0)
        ldrEntry = FindLdrEntry<T>( mod.baseAddress );

    // Unlink from module lists
    if (ldrEntry != 0)
    {
        UnlinkListEntry<T>( fieldPtr( ldrEntry, &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::InLoadOrderLinks ) );
        UnlinkListEntry<T>( fieldPtr( ldrEntry, &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::InMemoryOrderLinks ) );
        UnlinkListEntry<T>( fieldPtr( ldrEntry, &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::InInitializationOrderLinks ) );
        UnlinkListEntry<T>( fieldPtr( ldrEntry, &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::HashLinks ) );
    }

    return ldrEntry;
}

/// <summary>
/// Finds LDR entry for module
/// </summary>
/// <param name="moduleBase">Target module base</param>
/// <param name="found">Found entry</param>
/// <returns>Found LDR entry address</returns>
template<typename T>
ptr_t NtLdr::FindLdrEntry( module_t moduleBase, _LDR_DATA_TABLE_ENTRY_BASE_T<T>* found /*= nullptr*/ )
{
    auto native = _process->core().native();
    _PEB_T<T> peb = { };
    _PEB_LDR_DATA2_T<T> ldr = { };
    _LDR_DATA_TABLE_ENTRY_BASE_T<T> localEntry = { };
    if (found == nullptr)
        found = &localEntry;

    if (native->getPEB( &peb ) != 0 && NT_SUCCESS( native->ReadProcessMemoryT( peb.Ldr, &ldr, sizeof( ldr ) ) ))
    {
        const auto ofst = offsetOf( &_LDR_DATA_TABLE_ENTRY_BASE_T<T>::InLoadOrderLinks );
        const auto head = fieldPtr( peb.Ldr, &_PEB_LDR_DATA2_T<T>::InLoadOrderModuleList );

        for (T entry = ldr.InLoadOrderModuleList.Flink;
            entry != 0 && entry != head;
            native->ReadProcessMemoryT( entry, &entry, sizeof( entry ) )
            )
        {
            native->ReadProcessMemoryT( entry - ofst, found, sizeof( *found ) );
            if (found->DllBase == static_cast<T>(moduleBase))
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
    T OldFlink = _process->memory().Read<T>( fieldPtr( pListLink, &_LIST_ENTRY_T<T>::Flink ) );
    T OldBlink = _process->memory().Read<T>( fieldPtr( pListLink, &_LIST_ENTRY_T<T>::Blink ) );

    // List is empty
    if (OldBlink == 0 || OldFlink == 0 || OldBlink == OldFlink)
        return;

    // OldFlink->Blink = OldBlink;
    _process->memory().Write( fieldPtr( OldFlink, &_LIST_ENTRY_T<T>::Blink ), OldBlink );

    // OldBlink->Flink = OldFlink;
    _process->memory().Write( fieldPtr( OldBlink, &_LIST_ENTRY_T<T>::Flink ), OldFlink );
}

/// <summary>
/// Unlink from module graph
/// </summary>
/// <param name="mod">Module data</param>
/// <param name="ldrEntry">Module LDR entry</param>
/// <param name="noThread">Don't create new threads during unlink</param>
/// <returns>Address of removed record</returns>
template<typename T>
ptr_t NtLdr::UnlinkTreeNode( const ModuleData& mod, ptr_t ldrEntry, bool noThread /*= false*/ )
{
    if (ldrEntry == 0)
        return ldrEntry;

    auto a = AsmFactory::GetAssembler( mod.type );
    auto RtlRbRemoveNode = _process->modules().GetNtdllExport( "RtlRbRemoveNode" );

    a->GenPrologue();
    a->GenCall( RtlRbRemoveNode.procAddress,
        {
            _LdrpModuleIndexBase,
            ldrEntry + offsetOf( &_LDR_DATA_TABLE_ENTRY_W8<T>::BaseAddressIndexNode )
        } );

    _process->remote().AddReturnWithEvent( *a );
    a->GenEpilogue();

    _process->remote().CreateRPCEnvironment( noThread ? Worker_UseExisting : Worker_CreateNew, true );
    _process->remote().ExecInWorkerThread( (*a)->make(), (*a)->getCodeSize() );

    return ldrEntry;
}

}
