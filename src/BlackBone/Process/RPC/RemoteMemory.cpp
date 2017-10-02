#include "RemoteMemory.h"
#include "../Process.h"
#include "../../Misc/Trace.hpp"

namespace blackbone
{

RemoteMemory::RemoteMemory( Process* process )
    : _process( process )
{
    _pipeName = Utils::RandomANString() + L"_" + std::to_wstring( _process->pid() ) + L"_" + std::to_wstring( GetCurrentProcessId() );
}

RemoteMemory::~RemoteMemory()
{
    reset();
}

/// <summary>
/// Map entire process address space
/// </summary>
/// <param name="mapSections">Set to true to map section objects. They are converted to private pages before locking</param>
/// <returns>Status code</returns>
NTSTATUS RemoteMemory::Map( bool mapSections )
{
    MapMemoryResult result = { 0 };

    // IPC
    if (!_hPipe)
        _hPipe = CreateNamedPipeW( (L"\\\\.\\pipe\\" + _pipeName).c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, 1, 0, 0, 0, NULL );

    Driver().EnsureLoaded();
    NTSTATUS status = Driver().MapMemory( _process->pid(), _pipeName, mapSections, result );

    if (NT_SUCCESS( status ))
    {
        std::swap( _mapDatabase, result.regions );

        _pSharedData = (PageContext*)result.hostSharedPage;
        _targetShare = result.targetSharedPage;
        _targetPipe = result.targetPipe;
    }

    return status;
}

/// <summary>
/// Map specific memory region
/// </summary>
/// <param name="base">Region base</param>
/// <param name="size">Region size</param>
/// <returns>Status code</returns>
NTSTATUS RemoteMemory::Map( ptr_t base, uint32_t size )
{
    MapMemoryRegionResult memRes =  { 0 };

    // IPC
    if (!_hPipe)
        _hPipe = CreateNamedPipeW( (L"\\\\.\\pipe\\" + _pipeName).c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, 1, 0, 0, 0, NULL );

    Driver().EnsureLoaded();
    NTSTATUS status = Driver().MapMemoryRegion( _process->pid(), base, size, memRes );

    // Update regions
    if (NT_SUCCESS( status ))
    {
        MapMemoryResult rgnRes = { };

        if (NT_SUCCESS( Driver().MapMemory( _process->pid(), _pipeName, false, rgnRes ) ))
            std::swap( _mapDatabase, rgnRes.regions );
    }

    return status;
}

/// <summary>
/// Unmap process address space from current process
/// </summary>
/// <returns>Status code</returns>
NTSTATUS RemoteMemory::Unmap( )
{
    NTSTATUS status = Driver().UnmapMemory( _process->pid() );

    if (NT_SUCCESS( status ))
    {
        _mapDatabase.clear();

        _pSharedData = nullptr;
        _targetShare = 0;
        _targetPipe = NULL;
    }

    return status;
}

/// <summary>
/// Unmap specific memory region from current process
/// </summary>
/// <param name="base">Region base</param>
/// <param name="size">Region size</param>
/// <returns>Status code</returns>
NTSTATUS RemoteMemory::Unmap( ptr_t base, uint32_t size )
{
    auto searchFn = [&base]( const decltype(_mapDatabase)::value_type& val )
    {
        return base >= val.first.first && base < val.first.first + val.first.second;
    };

    NTSTATUS status = Driver().UnmapMemoryRegion( _process->pid(), base, size );

    if (NT_SUCCESS( status ))
    {
        // Remove region
        auto iter = std::find_if( _mapDatabase.begin(), _mapDatabase.end(), searchFn );
        if (iter != _mapDatabase.end())
            _mapDatabase.erase( iter );
    }

    return status;
}


/// <summary>
/// Translate target address accordingly to current address space
/// </summary>
/// <param name="address">Address to translate</param>
/// <param name="resolveFault">If set to true, routine will try to map non-existing region upon translation failure</param>
/// <returns>Translated address</returns>
blackbone::ptr_t RemoteMemory::TranslateAddress( ptr_t address, bool resolveFault /*= true */ )
{
    auto searchFn = [&address]( const decltype(_mapDatabase)::value_type& val )
    {
        return address >= val.first.first && address < val.first.first + val.first.second;
    };

    auto iter = std::find_if( _mapDatabase.begin(), _mapDatabase.end(), searchFn );
                              
    // Primitive Page fault. Try to resolve missing page
    if (iter == _mapDatabase.end() && resolveFault && NT_SUCCESS( Map( address, 1 ) ))
        // Second chance
        iter = std::find_if( _mapDatabase.begin(), _mapDatabase.end(), searchFn );           

    if (iter != _mapDatabase.end())
        return iter->second + (address - iter->first.first);    

    return 0;
}

/// <summary>
/// Setup one of the 4 possible memory hooks:
/// </summary>
/// <param name="hkType">
/// Type of hook to install
/// MemVirtualAlloc - hook NtAllocateVirtualMemory
/// MemVirtualFree  - hook NtFreeVirtualMemory
/// MemMapSection   - hook NtMapViewOfSection
/// MemUnmapSection - hook NtUnmapViewOfSection
/// </param>
/// <returns>true on success</returns>
NTSTATUS RemoteMemory::SetupHook( OperationType hkType )
{
    static const char* procNames[] = { "NtAllocateVirtualMemory", "NtFreeVirtualMemory", "NtMapViewOfSection", "NtUnmapViewOfSection" };

    uint8_t* pTranslated = nullptr;
    ptr_t pProc = 0;

    // Can't setup hook without target pipe and shared data
    if (_targetPipe == NULL || !_pSharedData || !_targetShare)
        return STATUS_NONE_MAPPED;

    // Cross-architecture code generation isn't supported yet
    auto barrier = _process->barrier().type;
    if (barrier != wow_32_32 && barrier != wow_64_64)
        return STATUS_CONTEXT_MISMATCH;

    // Already hooked
    if (_hooked[hkType])
        return STATUS_ALREADY_REGISTERED;

    auto& modules = _process->modules();

    // Local and remote process address
    pProc = modules.GetExport( modules.GetModule( L"ntdll.dll" ), procNames[hkType] ).result( exportData() ).procAddress;
    pTranslated = (uint8_t*)TranslateAddress( pProc );
    if (!pTranslated)
        return STATUS_INVALID_ADDRESS;

    // IPC
    if (!_hPipe)
        _hPipe = CreateNamedPipeW( (L"\\\\.\\pipe\\" + _pipeName).c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, 1, 0, 0, 0, NULL );

    // Listening thread
    if (_hThread == NULL)
    {
        _hThread = CreateThread( NULL, 0, &RemoteMemory::HookThreadWrap, this, 0, NULL );
        InitializeCriticalSection( &_pSharedData->csLock );
    }
    
    // Setup hook data
    BuildTrampoline( hkType, (uintptr_t)pProc, pTranslated );
    BuildGenericHookFn( hkType );

    // Copy hook
    memcpy( pTranslated, (uint8_t*)_pSharedData + sizeof( HookData ) * hkType + FIELD_OFFSET( HookData, jump_buf ), sizeof( _pSharedData->hkVirtualAlloc.jump_buf ) );
    
    _hooked[hkType] = true;
    return STATUS_SUCCESS;
}

/// <summary>
/// Restore previously hooked function
/// </summary>
/// <param name="hkType">Hook type. For more info see SetupHook</param>
/// <returns>true on success</returns>
bool RemoteMemory::RestoreHook( OperationType hkType )
{
    uint8_t* pTranslated = nullptr;
    ptr_t pProc = 0;
    static const char* procNames[] = { "NtAllocateVirtualMemory", "NtFreeVirtualMemory", "NtMapViewOfSection", "NtUnmapViewOfSection" };

    // Not hooked
    if (!_hooked[hkType])
        return false;

    auto& modules = _process->modules();

    // Local and remote proc address
    pProc = modules.GetExport( modules.GetModule( L"ntdll.dll" ), procNames[hkType] ).result( exportData() ).procAddress;
    pTranslated = (uint8_t*)TranslateAddress( pProc );
    if (!pTranslated)
        return false;

    // Restore bytes
    memcpy( pTranslated, (uint8_t*)_pSharedData + sizeof( HookData ) * hkType + FIELD_OFFSET( HookData, original_code ),
            sizeof( _pSharedData->hkVirtualAlloc.original_code ) / 2 );

    // Reset hook data
    memset( (uint8_t*)_pSharedData + sizeof( HookData ) * hkType, 0x00, sizeof( HookData ) );

    _hooked[hkType] = false;
    return true;
}


/// <summary>
/// Unmap any mapped memory, restore hooks and free resources
/// </summary>
void RemoteMemory::reset()
{
    _active = false;

    if (_hThread != NULL)
    {
        TerminateThread( _hThread, 0 );
        _hThread = NULL;
    }

    _hPipe.reset();

    for (int i = 0; i < 4; i++)
        RestoreHook( (OperationType)i );

    if (!_mapDatabase.empty() && !NT_SUCCESS( Unmap() ))
    {
        _mapDatabase.clear();

        _pSharedData = nullptr;
        _targetShare = 0;
        _targetPipe = NULL;
    }
}

/// <summary>
/// Hook thread wrapper
/// </summary>
/// <param name="lpParam">RemoteMemory instance</param>
/// <returns>0</returns>
DWORD CALLBACK RemoteMemory::HookThreadWrap( LPVOID lpParam )
{
    ((RemoteMemory*)lpParam)->HookThread();
    return 0;
}

/// <summary>
/// Thread responsible for mapping and unmapping regions intercepted by remote hooks
/// </summary>
void RemoteMemory::HookThread()
{
    DWORD bytes = 0;

    // Wait for target process to connect
    ConnectNamedPipe( _hPipe, NULL );
    _active = true;

    while (_active)
    {
        OperationData opData = { 0 };

        // Target endpoint closed
        if (!ReadFile( _hPipe, &opData, sizeof( opData ), &bytes, NULL ))
        {
            _active = false;
            _hThread = NULL;
            return;
        }

        // Update mapping
        if (opData.allocType == MemVirtualAlloc || opData.allocType == MemMapSection)
        {
            BLACKBONE_TRACE( L"Allocated 0x%x bytes at %p", opData.allocSize, opData.allocAddress );
            Map( opData.allocAddress, opData.allocSize );
        }
        else
        {
            BLACKBONE_TRACE( L"Freed 0x%x bytes at %p", opData.allocSize, opData.allocAddress );
            Unmap( opData.allocAddress, opData.allocSize );
        }
    }
}

#ifdef USE64

/// <summary>
/// Build remote hook function
/// </summary>
/// <param name="opType">Hooked function</param>
void RemoteMemory::BuildGenericHookFn( OperationType opType )
{
    static int argc[] = { 6, 4, 10, 2 };
    static int allocIdx[] = { 1, 1, 2, 1 };
    static int sizeIdx[] = { 3, 2, 6, -1 };
    static int testIdx[] = { 4, -1, -1, -1 };

    int hookDataOfs = sizeof( HookData ) * opType;

    auto pAsm = AsmFactory::GetAssembler( _process->core().isWow64() );
    auto& a = *pAsm;
    AsmStackAllocator sa( a.assembler(), 0x60 );
    asmjit::Label skip1 = a->newLabel();
    ALLOC_STACK_VAR( sa, data, OperationData );
    ALLOC_STACK_VAR( sa, junk, SIZE_T );
    size_t stack_size = Align( sa.getTotalSize(), 0x10 );

    auto& modules = _process->modules();

    auto pEnterCS = modules.GetExport( modules.GetModule( L"ntdll.dll" ), "RtlEnterCriticalSection" ).result( exportData() );
    auto pLeaveCS = modules.GetExport( modules.GetModule( L"ntdll.dll" ), "RtlLeaveCriticalSection" ).result( exportData() );
    auto pWrite = modules.GetExport( modules.GetModule( L"kernel32.dll" ), "WriteFile" ).result( exportData() );

    a.GenPrologue();
    a.EnableX64CallStack( false );
    a->sub( asmjit::host::rsp, stack_size );

    // Call original function
    for (int i = 0; i < argc[opType] - 4; i++)
    {
        a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rsp, (int32_t)stack_size + 0x8 + 0x20 + i * 8 ) );
        a->mov( asmjit::host::qword_ptr( asmjit::host::rsp, 0x20 + i * 8 ), asmjit::host::rax );
    }

    a->mov( asmjit::host::rax, _targetShare + hookDataOfs + FIELD_OFFSET( HookData, original_code ) );
    a->call( asmjit::host::rax );

    // Test if call was successful
    a->test( asmjit::host::rax, asmjit::host::rax );
    a->jnz( skip1 );

    // Test if memory was committed
    if (opType == MemVirtualAlloc)
    {
        a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::rsp, (int32_t)stack_size + 0x8 + 0x20 ) );
        a->cmp( asmjit::host::eax, MEM_COMMIT );
        a->jne( skip1 );
    }

    // RtlEnterCriticalSection
    a.GenCall( (uintptr_t)pEnterCS.procAddress, { _targetShare + FIELD_OFFSET( PageContext, csLock ) } );

    // Storage pointer
    a->lea( asmjit::host::rdx, data );

    // Allocation address
    if (allocIdx[opType] > 3)
        a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rsp, (int32_t)stack_size + 0x8 + 0x20 + (allocIdx[opType] - 4) * 8 ) );
    else
        a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rsp, (int32_t)stack_size + 0x8 + allocIdx[opType] * 8 ) );

    if (opType != MemUnmapSection)
        a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rax ) );

    a->mov( asmjit::host::qword_ptr( asmjit::host::rdx, FIELD_OFFSET( OperationData, allocAddress ) ), asmjit::host::rax );

    if (sizeIdx[opType] != -1)
    {
        // Region size
        if (sizeIdx[opType] > 3)
            a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rsp, (int32_t)stack_size + 0x8 + 0x20 + (sizeIdx[opType] - 4) * 8 ) );
        else
            a->mov( asmjit::host::rax, asmjit::host::qword_ptr( asmjit::host::rsp, (int32_t)stack_size + 0x8 + sizeIdx[opType] * 8 ) );

        a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::rax ) );
        a->mov( asmjit::host::dword_ptr( asmjit::host::rdx, FIELD_OFFSET( OperationData, allocSize ) ), asmjit::host::eax );
    }
    else
        a->mov( asmjit::host::dword_ptr( asmjit::host::rdx, FIELD_OFFSET( OperationData, allocSize ) ), 0 );

    // Operation type
    a->mov( asmjit::host::dword_ptr( asmjit::host::rdx, FIELD_OFFSET( OperationData, allocType ) ), opType );

    a.GenCall( (uintptr_t)pWrite.procAddress, { (uint64_t)_targetPipe, &data, sizeof( OperationData ), &junk, 0 } );

    // RtlEnterCriticalSection
    a.GenCall( (uintptr_t)pLeaveCS.procAddress, { _targetShare + FIELD_OFFSET( PageContext, csLock ) } );

    // Ignore return value
    a->xor_( asmjit::host::rax, asmjit::host::rax );

    a->bind( skip1 );
    a->add( asmjit::host::rsp, stack_size );
    a->ret();

    a->setBaseAddress( _targetShare + hookDataOfs + FIELD_OFFSET( HookData, hook_code ) );
    a->relocCode( (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, hook_code ) );
}

/// <summary>
/// Build hook trampoline
/// </summary>
/// <param name="opType">Hooked function type</param>
/// <param name="pOriginal">Original function ptr</param>
/// <param name="pOriginalLocal">Original function address in local address space</param>
void RemoteMemory::BuildTrampoline( OperationType opType, uintptr_t /*pOriginal*/, uint8_t* pOriginalLocal )
{
    int hookDataOfs = sizeof( HookData ) * opType;
    uint8_t * pJumpBuf = (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, jump_buf );

    memcpy( (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, original_code ), pOriginalLocal, 12 );

    // mov rax, pOriginal
    // call rax
    *(uint16_t*)pJumpBuf = 0xB848;
    *(uint64_t*)(pJumpBuf + 2) = _targetShare + hookDataOfs + FIELD_OFFSET( HookData, hook_code );
    *(uint16_t*)(pJumpBuf + 10) = 0xE0FF;
}


#else

/// <summary>
/// Build remote hook function
/// </summary>
/// <param name="opType">Hooked function</param>
void RemoteMemory::BuildGenericHookFn( OperationType opType )
{
    static int argc[] = { 6, 4, 10, 2 };
    static int allocIdx[] = { 1, 1, 2, 1 };
    static int sizeIdx[] = { 3, 2, 6, -1 };
    static int testIdx[] = { 4, -1, -1, -1 };

    int hookDataOfs = sizeof( HookData ) * opType;

    auto pAsm = AsmFactory::GetAssembler( _process->core().isWow64() );
    auto& a = *pAsm;
    AsmStackAllocator sa( a.assembler() );
    asmjit::Label skip1 = a->newLabel();
    ALLOC_STACK_VAR( sa, data, OperationData );
    ALLOC_STACK_VAR( sa, junk, DWORD );

    auto& modules = _process->modules();

    auto pEnterCS = modules.GetExport( modules.GetModule( L"ntdll.dll" ), "RtlEnterCriticalSection" ).result( exportData() ).procAddress;
    auto pLeaveCS = modules.GetExport( modules.GetModule( L"ntdll.dll" ), "RtlLeaveCriticalSection" ).result( exportData() ).procAddress;
    auto pWrite = modules.GetExport( modules.GetModule( L"kernel32.dll" ), "WriteFile" ).result( exportData() ).procAddress;

    a.GenPrologue();
    a->sub( asmjit::host::esp, sa.getTotalSize() );

    // Call original
    for (int i = 0; i < argc[opType]; i++)
        a->push( asmjit::host::dword_ptr( asmjit::host::ebp, (argc[opType] - i) * 4 + 4 ) );

    a->call( _targetShare + hookDataOfs + FIELD_OFFSET( HookData, original_code ) );

    // Test if call was successful
    a->test( asmjit::host::eax, asmjit::host::eax );
    a->jnz( skip1 );

    if (testIdx[opType] != -1)
    {
        // Test if memory was committed
        a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::ebp, 8 + testIdx[opType] * 4 ) );
        a->cmp( asmjit::host::eax, MEM_COMMIT );
        a->jne( skip1 );
    }

    // RtlEnterCriticalSection
    a.GenCall( (uintptr_t)pEnterCS, { (uintptr_t)_targetShare + FIELD_OFFSET( PageContext, csLock ) } );

    // Storage pointer
    a->lea( asmjit::host::edx, data );
    
    //
    // Allocation address
    //
    a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::ebp, 8 + 4 * allocIdx[opType] ) );
    if (opType != MemUnmapSection)
        a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::eax ) );
    a->mov( asmjit::host::dword_ptr( asmjit::host::edx, FIELD_OFFSET( OperationData, allocAddress ) ), asmjit::host::eax );
    a->mov( asmjit::host::dword_ptr( asmjit::host::edx, FIELD_OFFSET( OperationData, allocAddress ) + 4 ), 0 );

    //
    // Region size
    //
    if (sizeIdx[opType] != -1)
    {
        a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::ebp, 8 + 4 * sizeIdx[opType] ) );
        a->mov( asmjit::host::eax, asmjit::host::dword_ptr( asmjit::host::eax ) );
    }
    else
        a->xor_( asmjit::host::eax, asmjit::host::eax );

    a->mov( asmjit::host::dword_ptr( asmjit::host::edx, FIELD_OFFSET( OperationData, allocSize ) ), asmjit::host::eax );

    // Operation type
    a->mov( asmjit::host::dword_ptr( asmjit::host::edx, FIELD_OFFSET( OperationData, allocType ) ), opType );

    a.GenCall( (uintptr_t)pWrite, { (uintptr_t)_targetPipe, asmjit::host::edx, sizeof( OperationData ), &junk, 0 } );
    a.GenCall( (uintptr_t)pLeaveCS, { (uintptr_t)_targetShare + FIELD_OFFSET( PageContext, csLock ) } );

    // Ignore return value
    a->xor_( asmjit::host::eax, asmjit::host::eax );

    a->bind( skip1 );
    a.GenEpilogue( false, argc[opType] * 4 );

    a->setBaseAddress( _targetShare + hookDataOfs + FIELD_OFFSET( HookData, hook_code ) );
    a->relocCode( (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, hook_code ) );
}

/// <summary>
/// Build hook trampoline
/// </summary>
/// <param name="opType">Hooked function type</param>
/// <param name="pOriginal">Original function ptr</param>
/// <param name="pOriginalLocal">Original function address in local address space</param>
void RemoteMemory::BuildTrampoline( OperationType opType, uintptr_t pOriginal, uint8_t* pOriginalLocal )
{
    int hookDataOfs = sizeof( HookData ) * opType;
    uintptr_t pNew = (uintptr_t)_targetShare + hookDataOfs + FIELD_OFFSET( HookData, hook_code );
    uintptr_t pTrampoline = (uintptr_t)_targetShare + hookDataOfs + FIELD_OFFSET( HookData, original_code );

    uint8_t* pJumpBuf = (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, jump_buf );
    uint8_t* pOriginalBuf = (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, original_code );

    memcpy( (uint8_t*)_pSharedData + hookDataOfs + FIELD_OFFSET( HookData, original_code ), pOriginalLocal, 5 );

    pJumpBuf[0] = 0xE9;
    *(int32_t*)(pJumpBuf + 1) = pNew - pOriginal - 5;

    pOriginalBuf[5] = 0xE9;
    *(int32_t*)(pOriginalBuf + 6) = (pOriginal + 5) - (int32_t)(pTrampoline + 5) - 5;
}

#endif
}