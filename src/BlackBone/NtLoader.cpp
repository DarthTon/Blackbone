#include "NtLoader.h"
#include "Process.h"
#include "PatternSearch.h"
#include "Utils.h"
#include "Macro.h"
#include "DynImport.h"

#include <versionhelpers.h>

namespace blackbone
{

NtLdr::NtLdr( Process& proc )
    : _process( proc )
{
    HMODULE hNtdll = GetModuleHandleW( L"ntdll.dll" );

    DynImport::load( "RtlInitUnicodeString",   hNtdll );
    DynImport::load( "RtlHashUnicodeString",   hNtdll );
    DynImport::load( "RtlUpcaseUnicodeChar",   hNtdll );
    DynImport::load( "RtlEncodeSystemPointer", hNtdll );
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
    FindLdrpModuleIndexBase();
    FindLdrpModuleBase();
    ScanPatterns();
    FindLdrHeap();

    _nodeMap.clear();

    return true;
}

/// <summary>
/// Add module to some loader structures 
/// (LdrpHashTable, LdrpModuleIndex( win8 only ), InMemoryOrderModuleList( win7 only ))
/// </summary>
/// <param name="hMod">Module base address</param>
/// <param name="ImageSize">Size of image</param>
/// <param name="DllBasePath">Full-qualified image path.</param>
/// <returns>true on success</returns>
bool NtLdr::CreateNTReference( HMODULE hMod, size_t ImageSize, const std::wstring& DllBasePath )
{
    // Win 8 and higher
    if (IsWindows8OrGreater())
    {
        ULONG hash = 0;
        _LDR_DATA_TABLE_ENTRY_W8 *pEntry = InitW8Node( reinterpret_cast<void*>(hMod), ImageSize, DllBasePath, hash );
        _nodeMap.emplace( std::make_pair( hMod, pEntry ) );

        // Insert into LdrpHashTable
        InsertHashNode( reinterpret_cast<PLIST_ENTRY>(GET_FIELD_PTR( pEntry, HashLinks )), hash );

        // Insert into module graph
        InsertTreeNode( pEntry, reinterpret_cast<size_t>(hMod) );
    }
    // Windows 7 and earlier
    else
    {
        ULONG hash = 0;
        _LDR_DATA_TABLE_ENTRY_W7 *pEntry = InitW7Node( reinterpret_cast<void*>(hMod), ImageSize, DllBasePath, hash );
        _nodeMap.emplace( std::make_pair( hMod, pEntry ) );

        // Insert into LdrpHashTable
        InsertHashNode( reinterpret_cast<PLIST_ENTRY>(GET_FIELD_PTR( pEntry, HashLinks )), hash );

        // Insert into LDR list
        InsertMemModuleNode( reinterpret_cast<PLIST_ENTRY>(GET_FIELD_PTR( pEntry, InMemoryOrderLinks )),
                             reinterpret_cast<PLIST_ENTRY>(GET_FIELD_PTR( pEntry, InLoadOrderLinks )) );
	}

    return false;
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
        /*AsmJit::Assembler a;
        AsmJitHelper ah(a);

        ah.GenPrologue();
        ah.GenCall(&GetProcessHeap, {  });
        ah.GenCall(&HeapAlloc, { AsmJit::nax, HEAP_ZERO_MEMORY, sizeof(T) });
        ah.SaveRetValAndSignalEvent();
        ah.GenEpilogue();

        m_memory.ExecInWorkerThread(a.make(), a.getCodeSize(), (size_t&)ptr);*/

        auto rgn = _process.memory().Allocate( sizeof(T), PAGE_READWRITE );
        if (rgn.valid())
        {
            rgn.Release();

            ptr = rgn.ptr<T*>();
            rgn.Write( FIELD_OFFSET2( T*, DllBase ), pModule );
        }
    }

    return ptr;
}

/// <summary>
/// Create thread static TLS array
/// </summary>
/// <param name="pModule">Module base address</param>
/// <param name="pTls">TLS directory of target image</param>
/// <returns>true on success</returns>
bool NtLdr::AddStaticTLSEntry( void* pModule, IMAGE_TLS_DIRECTORY *pTls )
{
    void* pNode = _nodeMap.count( reinterpret_cast<HMODULE>(pModule) ) ? 
        _nodeMap[reinterpret_cast<HMODULE>(pModule)] : nullptr;

    // Allocate appropriate structure
    if ((pNode = SetNode( reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(pNode), pModule )) == nullptr)
        return false;

    // Manually add TLS table
    if (_process.core().native()->GetWow64Barrier().type == wow_64_32 && pTls != nullptr)
    {
        ptr_t pteb64 = _process.remote().getWorker()->teb( static_cast<_TEB64*>(nullptr) );

        auto tlsStore = _process.memory().Allocate( 0x1000, PAGE_READWRITE );
        tlsStore.Release();

        IMAGE_TLS_DIRECTORY remoteTls = { 0 };
        _process.memory( ).Read( reinterpret_cast<ptr_t>(pTls), sizeof(remoteTls), &remoteTls );

        auto size = remoteTls.EndAddressOfRawData - remoteTls.StartAddressOfRawData;
        std::unique_ptr<uint8_t[]> buf( new uint8_t[size]() );

        _process.memory().Read( remoteTls.StartAddressOfRawData, size, buf.get() );

        tlsStore.Write( 0, tlsStore.ptr<size_t>() + 0x800 );
        tlsStore.Write( 0x800, size, buf.get() );

        _process.memory().Write( pteb64 + FIELD_OFFSET( _TEB64, ThreadLocalStoragePointer ), tlsStore.ptr<DWORD64>() );

        return true;
    }

    // Use native method
    if(_LdrpHandleTlsData)
    {
        AsmJit::Assembler a;
        AsmJitHelper ah( a );
        uint64_t result = 0;

        ah.GenPrologue();
        ah.GenCall( _LdrpHandleTlsData, { reinterpret_cast<size_t>(pNode) }, IsWindows8Point1OrGreater() ? cc_thiscall : cc_stdcall );
        _process.remote().AddReturnWithEvent( ah );
        ah.GenEpilogue();

        _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
    }
    else
        return false;

    return true;
}

/// <summary>
/// Create module record in LdrpInvertedFunctionTable
/// Used to create fake SAFESEH entries
/// </summary>
/// <param name="ModuleBase">Module base address</param>
/// <param name="ImageSize">Size of image</param>
/// <returns>true on success</returns>
bool NtLdr::InsertInvertedFunctionTable( void* ModuleBase, size_t ImageSize )
{ 
    RTL_INVERTED_FUNCTION_TABLE7 table = { 0 };
    PRTL_INVERTED_FUNCTION_TABLE_ENTRY Entries = nullptr;
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    uint64_t result = 0;

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
    _process.memory().Read( _LdrpInvertedFunctionTable, sizeof(table), &table );
    for (DWORD i = 0; i < table.Count; i++)
        if(Entries[i].ImageBase == ModuleBase)
            return true;

    ah.GenPrologue();

    if (IsWindows8Point1OrGreater())
        ah.GenCall( _RtlInsertInvertedFunctionTable, { reinterpret_cast<size_t>(ModuleBase), ImageSize }, cc_fastcall );
    else if (IsWindows8OrGreater())
        ah.GenCall( _RtlInsertInvertedFunctionTable, { reinterpret_cast<size_t>(ModuleBase), ImageSize } );
    else
        ah.GenCall( _RtlInsertInvertedFunctionTable, { _LdrpInvertedFunctionTable, reinterpret_cast<size_t>(ModuleBase), ImageSize } );

    _process.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
    _process.memory().Read( _LdrpInvertedFunctionTable, sizeof(table), &table );

    for(DWORD i = 0; i < table.Count; i++)
    {
        if(Entries[i].ImageBase == ModuleBase)
        {
            // If Image has SAFESEH, RtlInsertInvertedFunctionTable is enough
            if(Entries[i].SizeOfTable != 0)
                return true;

            //
            // Create fake Exception directory
            // Directory will be filled later, during exception handling
            //
            PIMAGE_RUNTIME_FUNCTION_ENTRY pImgEntry = nullptr;

            // Allocate memory for 256 possible handlers
            auto block = _process.memory().Allocate( sizeof(DWORD)* 0x100, PAGE_READWRITE );
            block.Release();
            pImgEntry = block.ptr<decltype(pImgEntry)>();

            // m_LdrpInvertedFunctionTable->Entries[i].ExceptionDirectory
            size_t field_ofst = reinterpret_cast<size_t>(&Entries[i].ExceptionDirectory) 
                              - reinterpret_cast<size_t>(&table);

            return (_process.memory().Write( _LdrpInvertedFunctionTable + field_ofst,
                GET_IMPORT( RtlEncodeSystemPointer )(pImgEntry) ) == STATUS_SUCCESS);
        }
    }

    return false;
}

/// <summary>
///  Initialize OS-specific module entry
/// </summary>
/// <param name="ModuleBase">Module base address.</param>
/// <param name="ImageSize">Size of image.</param>
/// <param name="dllpath">Full-qualified image path</param>
/// <param name="outHash">Iamge name hash</param>
/// <returns>Pointer to created entry</returns>
_LDR_DATA_TABLE_ENTRY_W8* NtLdr::InitW8Node( void* ModuleBase, size_t ImageSize, const std::wstring& dllpath, ULONG& outHash )
{
    std::wstring dllname = Utils::StripPath( dllpath );
    UNICODE_STRING strLocal = { 0 };
    uint64_t result = 0;

    _LDR_DATA_TABLE_ENTRY_W8 *pEntry = nullptr; 
    _LDR_DDAG_NODE *pDdagNode = nullptr;

    AsmJit::Assembler a;
    AsmJitHelper ah( a );

    // Allocate space for Unicode string
    auto StringBuf = _process.memory().Allocate( 0x1000, PAGE_READWRITE );
    StringBuf.Release();

    eModType mt = _process.core().native()->GetWow64Barrier().sourceWow64 ? mt_mod32 : mt_mod64;

    auto RtlAllocateHeap = _process.modules().GetExport(
        _process.modules().GetModule( L"ntdll.dll", LdrList, mt ), "RtlAllocateHeap" ).procAddress;

    //
    // HeapAlloc(LdrHeap, HEAP_ZERO_MEMORY, sizeof(_LDR_DATA_TABLE_ENTRY_W8));
    //
    ah.GenPrologue();

    ah.GenCall( static_cast<size_t>(RtlAllocateHeap), { _LdrHeapBase, HEAP_ZERO_MEMORY, sizeof(_LDR_DATA_TABLE_ENTRY_W8) } );

    _process.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
    pEntry = reinterpret_cast<decltype(pEntry)>(result);

    if(pEntry)
    {
        a.clear();

        //
        // HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(_LDR_DDAG_NODE));
        //
        ah.GenPrologue();

        ah.GenCall( static_cast<size_t>(RtlAllocateHeap), { _LdrHeapBase, HEAP_ZERO_MEMORY, sizeof(_LDR_DDAG_NODE) } );

        _process.remote().AddReturnWithEvent( ah );
        ah.GenEpilogue();

        _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
        pDdagNode = reinterpret_cast<decltype(pDdagNode)>(result);

        if(pDdagNode)
        {
            // pEntry->DllBase = ModuleBase;
            _process.memory().Write( GET_FIELD_PTR( pEntry, DllBase ), ModuleBase );

            // pEntry->SizeOfImage = ImageSize;
            _process.memory().Write( GET_FIELD_PTR( pEntry, SizeOfImage ), static_cast<ULONG>(ImageSize) );

            // Dll name and name hash
            GET_IMPORT( RtlInitUnicodeString )( &strLocal, dllname.c_str() );
            outHash = HashString( dllname );

            // Write into buffer
            strLocal.Buffer = StringBuf.ptr<PWSTR>();
            StringBuf.Write( 0, dllname.length() * sizeof(wchar_t) + 2, dllname.c_str() );

            // pEntry->BaseDllName = strLocal;
            _process.memory().Write( GET_FIELD_PTR( pEntry, BaseDllName ), strLocal );

            // Dll full path
            GET_IMPORT( RtlInitUnicodeString )( &strLocal, dllpath.c_str() );
            strLocal.Buffer = reinterpret_cast<PWSTR>(StringBuf.ptr<uint8_t*>( ) + 0x800);
            StringBuf.Write( 0x800, dllpath.length() * sizeof(wchar_t) + 2, dllpath.c_str() );
              
            // pEntry->FullDllName = strLocal;
            _process.memory().Write( GET_FIELD_PTR( pEntry, FullDllName ), strLocal );

            // pEntry->BaseNameHashValue = hash;
            _process.memory().Write( GET_FIELD_PTR( pEntry, BaseNameHashValue ), outHash );

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
/// <param name="ModuleBase">Module base address.</param>
/// <param name="ImageSize">Size of image.</param>
/// <param name="dllpath">Full-qualified image path</param>
/// <param name="outHash">Iamge name hash</param>
/// <returns>Pointer to created entry</returns>
_LDR_DATA_TABLE_ENTRY_W7* NtLdr::InitW7Node( void* ModuleBase, size_t ImageSize, const std::wstring& dllpath, ULONG& outHash )
{
    std::wstring dllname = Utils::StripPath( dllpath );
    UNICODE_STRING strLocal = { 0 };
    uint64_t result = 0;

    _LDR_DATA_TABLE_ENTRY_W7 *pEntry = nullptr; 

    AsmJit::Assembler a;
    AsmJitHelper ah(a);

    // Allocate space for Unicode string
    auto StringBuf = _process.memory().Allocate( MAX_PATH, PAGE_READWRITE );
    StringBuf.Release();

    eModType mt = _process.core().native()->GetWow64Barrier().sourceWow64 ? mt_mod32 : mt_mod64;

    auto RtlAllocateHeap = _process.modules().GetExport(
        _process.modules().GetModule( L"ntdll.dll", LdrList, mt ), "RtlAllocateHeap" ).procAddress;

    //
    // HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(_LDR_DATA_TABLE_ENTRY_W8));
    //
    ah.GenPrologue();

    ah.GenCall( static_cast<size_t>(RtlAllocateHeap), { _LdrHeapBase, HEAP_ZERO_MEMORY, sizeof(_LDR_DATA_TABLE_ENTRY_W7) } );

    _process.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
    pEntry = reinterpret_cast<decltype(pEntry)>(result);

    if(pEntry)
    {
        // pEntry->DllBase = ModuleBase;
        _process.memory().Write( GET_FIELD_PTR( pEntry, DllBase ), ModuleBase );

        // pEntry->SizeOfImage = ImageSize;
        _process.memory().Write( GET_FIELD_PTR( pEntry, SizeOfImage ), static_cast<ULONG>(ImageSize) );

        // pEntry->LoadCount = -1;
        _process.memory().Write( GET_FIELD_PTR( pEntry, LoadCount ), -1 );

        // Dll name
        GET_IMPORT( RtlInitUnicodeString )( &strLocal, dllname.c_str() );

        // Name hash
        outHash = HashString( dllname );

        // Write into buffer
        strLocal.Buffer = StringBuf.ptr<PWSTR>();
        StringBuf.Write( 0, dllname.length() * sizeof(wchar_t)+2, dllname.c_str() );

        // pEntry->BaseDllName = strLocal;
        _process.memory().Write( GET_FIELD_PTR( pEntry, BaseDllName ), strLocal );

        // Dll full path
        GET_IMPORT( RtlInitUnicodeString )( &strLocal, dllpath.c_str() );
        strLocal.Buffer = reinterpret_cast<PWSTR>(StringBuf.ptr<uint8_t*>() + 0x800);
        StringBuf.Write( 0x800, dllpath.length() * sizeof(wchar_t) + 2, dllpath.c_str() );

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
void NtLdr::InsertTreeNode( _LDR_DATA_TABLE_ENTRY_W8* pNode, size_t modBase )
{
    //
    // Win8 module tree
    //
    size_t root = _process.memory().Read<size_t>( _LdrpModuleIndexBase );

    _LDR_DATA_TABLE_ENTRY_W8 *pLdrNode = CONTAINING_RECORD( root, _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode );
    _LDR_DATA_TABLE_ENTRY_W8 LdrNode = _process.memory().Read<_LDR_DATA_TABLE_ENTRY_W8>( reinterpret_cast<ptr_t>(pLdrNode) );

    bool bRight = false;

    // Walk tree
    for (;;)
    {
        if (modBase < LdrNode.DllBase)
        {
            if (LdrNode.BaseAddressIndexNode.Left)
            {
                pLdrNode = CONTAINING_RECORD( LdrNode.BaseAddressIndexNode.Left, _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode );
                _process.memory().Read( reinterpret_cast<ptr_t>(pLdrNode), sizeof(LdrNode), &LdrNode );
            }
            else
                break;
        }
        else if (modBase  > LdrNode.DllBase)
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
            _LDR_DDAG_NODE Ddag = _process.memory().Read<_LDR_DDAG_NODE>( reinterpret_cast<ptr_t>(LdrNode.DdagNode) );

            Ddag.ReferenceCount++;

            _process.memory().Write( reinterpret_cast<ptr_t>(LdrNode.DdagNode), Ddag );

            return;
        }
    }

    // Insert using RtlRbInsertNodeEx
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    uint64_t result = 0;

    eModType mt = _process.core().native()->GetWow64Barrier().sourceWow64 ? mt_mod32 : mt_mod64;

    auto RtlRbInsertNodeEx = _process.modules().GetExport( 
        _process.modules().GetModule( L"ntdll.dll", LdrList, mt ), "RtlRbInsertNodeEx" ).procAddress;

    ah.GenPrologue( );
    ah.GenCall( static_cast<size_t>(RtlRbInsertNodeEx), { _LdrpModuleIndexBase,
                                                           GET_FIELD_PTR( pLdrNode, BaseAddressIndexNode ), 
                                                           static_cast<size_t>(bRight), GET_FIELD_PTR( pNode, BaseAddressIndexNode ) } );

    _process.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );
}

/// <summary>
/// Insert entry into InLoadOrderModuleList and InMemoryOrderModuleList
/// </summary>
/// <param name="pNodeMemoryOrderLink">InMemoryOrderModuleList link of entry to be inserted</param>
/// <param name="pNodeLoadOrderLink">InLoadOrderModuleList link of entry to be inserted</param>
void NtLdr::InsertMemModuleNode( PLIST_ENTRY pNodeMemoryOrderLink, PLIST_ENTRY pNodeLoadOrderLink )
{
    PEB_T* pPeb = reinterpret_cast<PEB_T*>(_process.core().peb());

    if(pPeb)
    {
        PPEB_LDR_DATA pLdr = _process.memory().Read<PPEB_LDR_DATA>( GET_FIELD_PTR( pPeb, Ldr ) );
                
        // Insert into pLdr->InMemoryOrderModuleList
        if (pLdr)
            InsertTailList( reinterpret_cast<PLIST_ENTRY>(GET_FIELD_PTR( pLdr, InMemoryOrderModuleList )), pNodeMemoryOrderLink );

        // Module list
        PLIST_ENTRY pModuleList = _process.memory().Read<PLIST_ENTRY>( _LdrpModuleBase );

        if (pModuleList)
            InsertTailList( pModuleList, pNodeLoadOrderLink );
    }
}

/// <summary>
/// Insert entry into LdrpHashTable[]
/// </summary>
/// <param name="pNodeLink">Link of entry to be inserted</param>
/// <param name="hash">Module hash</param>
void NtLdr::InsertHashNode( PLIST_ENTRY pNodeLink, ULONG hash )
{
    if(pNodeLink)
    {
        // LrpHashTable record
        PLIST_ENTRY pHashList = _process.memory().Read<PLIST_ENTRY>( _LdrpHashTable + sizeof(LIST_ENTRY)*(hash & 0x1F) );
        InsertTailList( pHashList, pNodeLink );
    }
}

/// <summary>
/// Insert entry into standard double linked list
/// </summary>
/// <param name="ListHead">List head pointer</param>
/// <param name="Entry">Entry list link to be inserted</param>
void NtLdr::InsertTailList( PLIST_ENTRY ListHead, PLIST_ENTRY Entry )
{
    PLIST_ENTRY PrevEntry;

    // PrevEntry = ListHead->Blink;
    PrevEntry = _process.memory().Read<PLIST_ENTRY>( GET_FIELD_PTR( ListHead, Blink ) );

    // Entry->Flink = ListHead;
    // Entry->Blink = PrevEntry;
    _process.memory().Write( GET_FIELD_PTR( Entry, Flink ), ListHead );
    _process.memory().Write( GET_FIELD_PTR( Entry, Blink ), PrevEntry );

    // PrevEntry->Flink = Entry;
    // ListHead->Blink  = Entry;
    _process.memory().Write( GET_FIELD_PTR( PrevEntry, Flink ), Entry );
    _process.memory().Write( GET_FIELD_PTR( ListHead, Blink ), Entry );
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
        GET_IMPORT( RtlInitUnicodeString )(&ustr, str.c_str( ));
        GET_IMPORT( RtlHashUnicodeString )(&ustr, TRUE, 0, &hash);
    }
    else
    {
        for (auto& ch : str)
            hash += 0x1003F * static_cast<unsigned short>(GET_IMPORT( RtlUpcaseUnicodeChar )(ch));
    }

    return hash;
}

/// <summary>
/// Find LdrpHashTable[] variable
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrpHashTable( )
{
    PEB_LDR_DATA_T *Ldr = reinterpret_cast<PEB_LDR_DATA_T*>(NtCurrentTeb()->ProcessEnvironmentBlock->Ldr);

    LDR_DATA_TABLE_ENTRY_BASE_T *Ntdll = CONTAINING_RECORD( Ldr->InInitializationOrderModuleList.Flink,
                                                             LDR_DATA_TABLE_ENTRY_BASE_T, InInitializationOrderLinks );

    ULONG NtdllHashIndex = HashString( reinterpret_cast<wchar_t*>(Ntdll->BaseDllName.Buffer) ) & 0x1F;

    ULONG_PTR NtdllBase = static_cast<ULONG_PTR>(Ntdll->DllBase);
    ULONG_PTR NtdllEndAddress = NtdllBase + Ntdll->SizeOfImage - 1;

    // scan hash list to the head (head is located within ntdll)
    bool bHeadFound = false;
    PLIST_ENTRY pNtdllHashHead = NULL;

    for (PLIST_ENTRY e = reinterpret_cast<PLIST_ENTRY>(Ntdll->HashLinks.Flink);
          e != reinterpret_cast<PLIST_ENTRY>(&Ntdll->HashLinks);
          e = e->Flink)
    {
        if (reinterpret_cast<ULONG_PTR>(e) >= NtdllBase && 
            reinterpret_cast<ULONG_PTR>(e) < NtdllEndAddress)
        {
            bHeadFound = true;
            pNtdllHashHead = e;
            break;
        }
    }

    if (bHeadFound)
        _LdrpHashTable = reinterpret_cast<size_t>(pNtdllHashHead - NtdllHashIndex);

   return bHeadFound;
}


/// <summary>
/// Find LdrpModuleIndex variable under win8
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrpModuleIndexBase( )
{
    PEB_T* pPeb = reinterpret_cast<PEB_T*>(NtCurrentTeb( )->ProcessEnvironmentBlock);

    if(pPeb)
    {
        PRTL_BALANCED_NODE lastNode = 0;
        PEB_LDR_DATA_T* Ldr = reinterpret_cast<PEB_LDR_DATA_T*>(pPeb->Ldr);
        _LDR_DATA_TABLE_ENTRY_W8 *Ntdll = CONTAINING_RECORD( Ldr->InInitializationOrderModuleList.Flink, 
                                                             _LDR_DATA_TABLE_ENTRY_W8, InInitializationOrderLinks );

        PRTL_BALANCED_NODE pNode = &Ntdll->BaseAddressIndexNode;

        // Get root node
        for(; pNode->ParentValue; )
        {
            // Ignore last few bits
            lastNode = reinterpret_cast<PRTL_BALANCED_NODE>(pNode->ParentValue & size_t( -8 ));
            pNode = lastNode;
        }

        // Get pointer to root
        pe::PEParser ntdll;
        size_t* pStart = nullptr;
        size_t* pEnd = nullptr;

        ntdll.Parse( reinterpret_cast<void*>(Ntdll->DllBase) );

        for (auto& section : ntdll.sections())
            if (_stricmp( reinterpret_cast<LPCSTR>(section.Name), ".data") == 0 )
            {
                pStart = reinterpret_cast<size_t*>(Ntdll->DllBase + section.VirtualAddress);
                pEnd   = reinterpret_cast<size_t*>(Ntdll->DllBase + section.VirtualAddress + section.Misc.VirtualSize);

                break;
            }

        auto iter = std::find( pStart, pEnd, reinterpret_cast<size_t>(lastNode) );

        if (iter != pEnd)
        {
            _LdrpModuleIndexBase = reinterpret_cast<size_t>(iter);
            return true;
        }
    }

    return false;
}

/// <summary>
/// Get PEB->Ldr->InLoadOrderModuleList address
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrpModuleBase()
{
    PEB_T peb = { 0 };
    _process.core().peb( &peb );
    _LdrpModuleBase = peb.Ldr + FIELD_OFFSET( PEB_LDR_DATA_T, InLoadOrderModuleList );

    return true;
}

/// <summary>
/// Search for RtlInsertInvertedFunctionTable, LdrpInvertedFunctionTable, LdrpHandleTlsDatas.
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::ScanPatterns( )
{
    std::vector<ptr_t> foundData;
    pe::PEParser ntdll;
    void* pStart  = nullptr;
    size_t scanSize = 0;

    HMODULE hNtdll = GetModuleHandle( L"ntdll.dll" );
    ntdll.Parse( hNtdll );

    // Find ntdll code section
    for (auto& section : ntdll.sections())
    {
        if (_stricmp( reinterpret_cast<LPCSTR>(section.Name), ".text" ) == 0)
        {
            pStart = reinterpret_cast<void*>(reinterpret_cast<size_t>(hNtdll) + section.VirtualAddress);
            scanSize = section.Misc.VirtualSize;

            break;
        }
    }

    // Code section not found
    if(pStart == nullptr)
        return false;

    // Win 8.1 and later
    if (IsWindows8Point1OrGreater())
    {
    #ifdef _M_AMD64
        // RtlInsertInvertedFunctionTable
        // 48 83 EC 20 8B F2 4C 8D 4C 24
        PatternSearch ps1( "\x48\x83\xec\x20\x8b\xf2\x4c\x8d\x4c\x24" );
        ps1.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
        {
            _RtlInsertInvertedFunctionTable = static_cast<size_t>(foundData.front() - 0xE);
            _LdrpInvertedFunctionTable = (size_t)hNtdll + 0x129C50;//*reinterpret_cast<size_t*>(foundData.front() + 0x4A);
            foundData.clear();
        }

        // LdrpHandleTlsData
        // 44 8D 43 09 4C 8D 4C 24 38
        PatternSearch ps2( "\x44\x8d\x43\x09\x4c\x8d\x4c\x24\x38" );
        ps2.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
            _LdrpHandleTlsData = static_cast<size_t>(foundData.front() - 0x43);
    #else
        // RtlInsertInvertedFunctionTable
        // 8D 45 F4 89 55 F8 50 8D
        PatternSearch ps1( "\x8d\x45\xf4\x89\x55\xf8\x50\x8d\x55\xfc" );
        ps1.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
        {
            _RtlInsertInvertedFunctionTable = static_cast<size_t>(foundData.front() - 0xB);
            _LdrpInvertedFunctionTable = *reinterpret_cast<size_t*>(foundData.front( ) + 0x1D);
            foundData.clear();
        }

        // LdrpHandleTlsData
        // 8D 45 A8 50 6A 09
        PatternSearch ps2( "\x8d\x45\xa8\x50\x6a\x09" );
        ps2.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
            _LdrpHandleTlsData = static_cast<size_t>(foundData.front() - 0x18);
    #endif
    }
    // Win 8
    else if (IsWindows8OrGreater())
    {
    #ifdef _M_AMD64
        // LdrpHandleTlsData
        // 48 8B 79 30 45 8D 66 01
        PatternSearch ps( "\x48\x8b\x79\x30\x45\x8d\x66\x01" );
        ps.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
            _LdrpHandleTlsData = static_cast<size_t>(foundData.front() - 0x49);
    #else
        // RtlInsertInvertedFunctionTable
        // 8B FF 55 8B EC 51 51 53 57 8B 7D 08 8D
        PatternSearch ps1( "\x8b\xff\x55\x8b\xec\x51\x51\x53\x57\x8b\x7d\x08\x8d" );
        ps1.Search(pStart, scanSize, foundData );

        if(!foundData.empty())
        {
            _RtlInsertInvertedFunctionTable = static_cast<size_t>(foundData.front());
            _LdrpInvertedFunctionTable = *reinterpret_cast<size_t*>(_RtlInsertInvertedFunctionTable + 0x26);
            foundData.clear();
        }

        // LdrpHandleTlsData
        // 8B 45 08 89 45 A0
        PatternSearch ps2( "\x8b\x45\x08\x89\x45\xa0" );
        ps2.Search( pStart, scanSize, foundData );

        if(!foundData.empty())
            _LdrpHandleTlsData = static_cast<size_t>(foundData.front() - 0xC);
                 
    #endif

    }
    // Win 7 and earlier
    else
    {
    #ifdef _M_AMD64
        // LdrpHandleTlsData
        // 41 B8 09 00 00 00 48 8D 44 24 38
        PatternSearch ps1( "\x41\xb8\x09\x00\x00\x00\x48\x8d\x44\x24\x38", 11 );
        ps1.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
        {
            _LdrpHandleTlsData = static_cast<size_t>(foundData.front() - 0x27);
            foundData.clear();            
        }

        // LdrpFindOrMapDll patch address
        // 48 8D 8C 24 98 00 00 00 41 b0 01
        PatternSearch ps2( "\x48\x8D\x8C\x24\x98\x00\x00\x00\x41\xb0\x01", 11 );
        ps2.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
        {
            _LdrKernel32PatchAddress = static_cast<size_t>(foundData.front() + 0x12);
            foundData.clear();            
        }

        // KiUserApcDispatcher patch address
        // 48 8B 4C 24 18 48 8B C1 4C
        PatternSearch ps3( "\x48\x8b\x4c\x24\x18\x48\x8b\xc1\x4c");
        ps3.Search( pStart, scanSize, foundData );

        if (!foundData.empty())
        {
            _APC64PatchAddress = static_cast<size_t>(foundData.front());
            foundData.clear();            
        }
    #else
        // RtlInsertInvertedFunctionTable
        // 8B FF 55 8B EC 56 68
        PatternSearch ps1( "\x8b\xff\x55\x8b\xec\x56\x68" );
        ps1.Search( pStart, scanSize, foundData);

        if(!foundData.empty())
        {
            _RtlInsertInvertedFunctionTable = static_cast<size_t>(foundData.front( ));
            foundData.clear();
        }

        // RtlLookupFunctionTable + 0x11
        // 89 5D E0 38
        PatternSearch ps2( "\x89\x5D\xE0\x38" );
        ps2.Search( pStart, scanSize, foundData );
                
        if(!foundData.empty())
        {
            _LdrpInvertedFunctionTable = *reinterpret_cast<size_t*>(foundData.front() + 0x1B);
            foundData.clear();
        }

        // LdrpHandleTlsData
        // 74 20 8D 45 D4 50 6A 09 
        PatternSearch ps3( "\x74\x20\x8d\x45\xd4\x50\x6a\x09" );
        ps3.Search( pStart, scanSize, foundData );

        if(!foundData.empty())
            _LdrpHandleTlsData = static_cast<size_t>(foundData.front() - 0x14);

    #endif
    }

    return true;
}


/// <summary>
/// Find Loader heap base
/// </summary>
/// <returns>true on success</returns>
bool NtLdr::FindLdrHeap()
{
    PEB_T* pPeb = reinterpret_cast<PEB_T*>(_process.core().peb());
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };

    if (pPeb)
    {
        PEB_LDR_DATA Ldr = _process.memory().Read<PEB_LDR_DATA>( _process.memory().Read<size_t>( GET_FIELD_PTR( pPeb, Ldr ) ) );
        PLDR_DATA_TABLE_ENTRY NtdllEntry = CONTAINING_RECORD( Ldr.InMemoryOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );

        if (_process.core().native()->VirtualQueryExT( reinterpret_cast<ptr_t>(NtdllEntry), &mbi ) == STATUS_SUCCESS)
        {
            _LdrHeapBase = static_cast<size_t>(mbi.AllocationBase);
            return true;
        }
    }

    return false;
}

/// <summary>
/// Unlink module from Ntdll loader
/// </summary>
/// <param name="baseAddress">Module base address</param>
/// <param name="type">32 or 64 bit.</param>
/// <returns>true on success</returns>
bool NtLdr::Unlink( ptr_t baseAddress, eModType type )
{
    ptr_t ldrEntry = 0;

    // Unlink from linked lists
    if (type == mt_mod32)
        ldrEntry = UnlinkFromLdr<DWORD>( baseAddress );
    else
        ldrEntry = UnlinkFromLdr<DWORD64>( baseAddress );

    auto barrier = _process.core().native()->GetWow64Barrier();

    // Unlink from graph
    // TODO: Unlink from _LdrpMappingInfoIndex. Still can't decide if it is required.
    if ((barrier.type == wow_32_32 || barrier.type == wow_64_64) && ldrEntry && IsWindows8OrGreater())
        UnlinkTreeNode( ldrEntry );

    return ldrEntry != 0;
}

/// <summary>
/// Unlink module from PEB_LDR_DATA
/// </summary>
/// <param name="baseAddress">Module base address.</param>
/// <returns>Address of removed record</returns>
template<typename T>
ptr_t NtLdr::UnlinkFromLdr( ptr_t baseAddress )
{
    _PEB_T2<T>::type peb = { 0 };
    _PEB_LDR_DATA2<T> ldr = { 0 };

    auto native = _process.core().native();

    if (native->getPEB( &peb ) != 0 && native->ReadProcessMemoryT( peb.Ldr, &ldr, sizeof(ldr), 0 ) == STATUS_SUCCESS)
    {
        ptr_t ldrEntry = 0;

        // InLoadOrderModuleList
        ldrEntry = UnlinkListEntry( ldr.InLoadOrderModuleList,
                                    peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<T>, InLoadOrderModuleList ),
                                    FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, InLoadOrderLinks ),
                                    baseAddress );

        // InMemoryOrderModuleList
        UnlinkListEntry( ldr.InMemoryOrderModuleList,
                         peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<T>, InMemoryOrderModuleList ),
                         FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, InMemoryOrderLinks ),
                         baseAddress );

        // InInitializationOrderModuleList
        UnlinkListEntry( ldr.InInitializationOrderModuleList,
                         peb.Ldr + FIELD_OFFSET( _PEB_LDR_DATA2<T>, InInitializationOrderModuleList ),
                         FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, InInitializationOrderLinks ),
                         baseAddress );

        if(ldrEntry)
        {
            _LDR_DATA_TABLE_ENTRY_BASE<T> modData = { 0 };

            native->ReadProcessMemoryT( ldrEntry, &modData, sizeof(modData), 0 );

            // HashLinks
            UnlinkListEntry( modData.HashLinks,
                             peb.Ldr + FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, HashLinks ),
                             FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_BASE<T>, HashLinks ),
                             baseAddress );
        }

        return ldrEntry;
    }

    return 0;
}

/// <summary>
/// Remove record from LIST_ENTRY structure
/// </summary>
/// <param name="pListEntry">List to remove from</param>
/// <param name="head">List head address.</param>
/// <param name="ofst">Offset of link in _LDR_DATA_TABLE_ENTRY_BASE struct</param>
/// <param name="baseAddress">Record to remove.</param>
/// <returns>Address of removed record</returns>
template<typename T>
ptr_t NtLdr::UnlinkListEntry( _LIST_ENTRY_T<T> pListEntry, ptr_t head, size_t ofst, ptr_t baseAddress )
{
    auto native = _process.core().native();

    for (T entry = pListEntry.Flink; entry != head; native->ReadProcessMemoryT( static_cast<size_t>(entry), &entry, sizeof(entry), 0 ))
    {
        _LDR_DATA_TABLE_ENTRY_BASE<T> modData = { 0 };

        native->ReadProcessMemoryT( entry - ofst, &modData, sizeof(modData), 0 );

        // Unlink if found
        if (modData.DllBase == (T)baseAddress)
        {
            T OldFlink = _process.memory().Read<T>( entry + FIELD_OFFSET( _LIST_ENTRY_T<T>, Flink ) );
            T OldBlink = _process.memory().Read<T>( entry + FIELD_OFFSET( _LIST_ENTRY_T<T>, Blink ) );

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
/// Unlink from module graph
/// </summary>
/// <param name="ldrEntry">Module LDR entry</param>
/// <returns>Address of removed record</returns>
ptr_t NtLdr::UnlinkTreeNode( ptr_t ldrEntry )
{
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    uint64_t result = 0;

    auto RtlRbRemoveNode = _process.modules().GetExport(
        _process.modules().GetModule( L"ntdll.dll" ), "RtlRbRemoveNode" ).procAddress;

    ah.GenPrologue();
    ah.GenCall( static_cast<size_t>(RtlRbRemoveNode), 
    { 
        _LdrpModuleIndexBase, 
        static_cast<size_t>(ldrEntry) 
        + FIELD_OFFSET( _LDR_DATA_TABLE_ENTRY_W8, BaseAddressIndexNode ) 
    } );

    _process.remote().AddReturnWithEvent( ah );
    ah.GenEpilogue();

    _process.remote().ExecInWorkerThread( a.make(), a.getCodeSize(), result );

    return ldrEntry;
}

}
