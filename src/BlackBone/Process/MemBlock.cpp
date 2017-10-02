#include "MemBlock.h"
#include "ProcessMemory.h"
#include "ProcessCore.h"
#include "../Subsystem/NativeSubsystem.h"
#include "../Misc/Trace.hpp"

namespace blackbone
{
/// <summary>
/// MemBlock ctor
/// </summary>
/// <param name="mem">Process memory routines</param>
/// <param name="ptr">Memory address</param>
/// <param name="size">Block size</param>
/// <param name="prot">Memory protection</param>
/// <param name="own">false if caller will be responsible for block deallocation</param>
MemBlock::MemBlock( 
    ProcessMemory* mem, 
    ptr_t ptr, 
    size_t size, 
    DWORD prot, 
    bool own /*= true*/, 
    bool physical /*= false*/ 
    )
    : _pImpl( new MemBlockImpl( mem, ptr, size, prot, own, physical ) )
{
}

/// <summary>
/// MemBlock_p ctor
/// </summary>
/// <param name="mem">Process memory routines</param>
/// <param name="ptr">Memory address</param>
/// <param name="size">Block size</param>
/// <param name="prot">Memory protection</param>
/// <param name="own">false if caller will be responsible for block deallocation</param>
MemBlock::MemBlockImpl::MemBlockImpl( 
    class ProcessMemory* mem, 
    ptr_t ptr, size_t size, 
    DWORD prot, 
    bool own /*= true*/, 
    bool physical /*= false */ 
    )
    : _ptr( ptr )
    , _size( size )
    , _protection( prot )
    , _own( own )
    , _physical( physical )
    , _memory( mem )
{
}

/// <summary>
/// MemBlock ctor
/// </summary>
/// <param name="mem">Process memory routines</param>
/// <param name="ptr">Memory address</param>
/// <param name="own">false if caller will be responsible for block deallocation</param>
MemBlock::MemBlock( ProcessMemory* mem, ptr_t ptr, bool own /*= true*/ )
    : _pImpl( new MemBlockImpl )
{
    _pImpl->_ptr = ptr;
    _pImpl->_own = own;
    _pImpl->_memory = mem;

    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    mem->Query( _pImpl->_ptr, &mbi );

    _pImpl->_protection = mbi.Protect;
    _pImpl->_size = (size_t)mbi.RegionSize;
}


/// <summary>
/// Allocate new memory block
/// </summary>
/// <param name="process">Process memory routines</param>
/// <param name="size">Block size</param>
/// <param name="desired">Desired base address of new block</param>
/// <param name="protection">Memory protection</param>
/// <param name="own">false if caller will be responsible for block deallocation</param>
/// <returns>Memory block. If failed - returned block will be invalid</returns>
call_result_t<MemBlock> MemBlock::Allocate( 
    ProcessMemory& process, 
    size_t size, 
    ptr_t desired /*= 0*/, 
    DWORD protection /*= PAGE_EXECUTE_READWRITE */, 
    bool own /*= true*/ 
    )
{
    ptr_t desired64 = desired;
    DWORD newProt = CastProtection( protection, process.core().DEP() );
    
    NTSTATUS status = process.core().native()->VirtualAllocExT( desired64, size, MEM_RESERVE | MEM_COMMIT, newProt );
    if (!NT_SUCCESS( status ))
    {
        desired64 = 0;
        status = process.core().native()->VirtualAllocExT( desired64, size, MEM_COMMIT, newProt );
        if (NT_SUCCESS( status ))
            return call_result_t<MemBlock>( MemBlock( &process, desired64, size, protection, own ), STATUS_IMAGE_NOT_AT_BASE );
        else
            return status;
    }
#ifdef _DEBUG
    BLACKBONE_TRACE(L"Allocate: Allocating at address 0x%p (0x%X bytes)", static_cast<uintptr_t>(desired64), size);
#endif
    return MemBlock( &process, desired64, size, protection, own );
}

/// <summary>
/// Reallocate existing block for new size
/// </summary>
/// <param name="size">New block size</param>
/// <param name="desired">Desired base address of new block</param>
/// <param name="protection">Memory protection</param>
/// <returns>New block address</returns>
call_result_t<ptr_t> MemBlock::Realloc( size_t size, ptr_t desired /*= 0*/, DWORD protection /*= PAGE_EXECUTE_READWRITE*/ )
{
    if (!_pImpl)
        return STATUS_MEMORY_NOT_ALLOCATED;

    ptr_t desired64 = desired;
    auto status = _pImpl->_memory->core().native()->VirtualAllocExT( desired64, size, MEM_COMMIT, protection );
    if (!desired64)
    {
        desired64 = 0;
        status = _pImpl->_memory->core().native()->VirtualAllocExT( desired64, size, MEM_COMMIT, protection );
        if (!NT_SUCCESS( status ))
            return status;

        status = STATUS_IMAGE_NOT_AT_BASE;
    }

    // Replace current instance
    if (desired64)
    {
        Free();

        _pImpl->_ptr = desired64;
        _pImpl->_size = size;
        _pImpl->_protection = protection;
    }

    return call_result_t<ptr_t>( desired64, status );
}

/// <summary>
/// Change memory protection
/// </summary>
/// <param name="protection">New protection flags</param>
/// <param name="offset">Memory offset in block</param>
/// <param name="size">Block size</param>
/// <param name="pOld">Old protection flags</param>
/// <returns>Status</returns>
NTSTATUS MemBlock::Protect( DWORD protection, uintptr_t offset /*= 0*/, size_t size /*= 0*/, DWORD* pOld /*= nullptr */ )
{
    if (!_pImpl)
        return STATUS_MEMORY_NOT_ALLOCATED;

    auto prot = CastProtection( protection, _pImpl->_memory->core().DEP() );

    if (size == 0)
        size = _pImpl->_size;

    return _pImpl->_physical ? Driver().ProtectMem( _pImpl->_memory->core().pid(), _pImpl->_ptr + offset, size, prot ) :
        _pImpl->_memory->Protect( _pImpl->_ptr + offset, size, prot, pOld );
}

/// <summary>
/// Free memory
/// </summary>
/// <param name="size">Size of memory chunk to free. If 0 - whole block is freed</param>
NTSTATUS MemBlock::Free( size_t size /*= 0*/ )
{
    if (!_pImpl)
        return STATUS_MEMORY_NOT_ALLOCATED;

    return _pImpl->Free();
}

/// <summary>
/// Free memory
/// </summary>
/// <param name="size">Size of memory chunk to free. If 0 - whole block is freed</param>
NTSTATUS MemBlock::MemBlockImpl::Free( size_t size /*= 0 */ )
{
    if (_ptr == 0)
        return STATUS_MEMORY_NOT_ALLOCATED;

    size = Align( size, 0x1000 );

    NTSTATUS status = _physical ? Driver().FreeMem( _memory->core().pid(), _ptr, size, MEM_RELEASE ) :
        _memory->Free( _ptr, size, size == 0 ? MEM_RELEASE : MEM_DECOMMIT );

    if (!NT_SUCCESS( status ))
        return LastNtStatus();

    if (size == 0)
    {
        _ptr = 0;
        _size = 0;
        _protection = 0;
    }
    else
    {
        _ptr += size;
        _size -= size;
    }

    return STATUS_SUCCESS;
}

/// <summary>
/// Read data
/// </summary>
/// <param name="offset">Data offset in block</param>
/// <param name="size">Size of data to read</param>
/// <param name="pResult">Output buffer</param>
/// <param name="handleHoles">
/// If true, function will try to read all committed pages in range ignoring uncommitted.
/// Otherwise function will fail if there is at least one non-committed page in region.
/// </param>
/// <returns>Status</returns>
NTSTATUS MemBlock::Read( uintptr_t offset, size_t size, PVOID pResult, bool handleHoles /*= false*/ )
{
    if (!_pImpl)
        return STATUS_MEMORY_NOT_ALLOCATED;

    return _pImpl->_memory->Read( _pImpl->_ptr + offset, size, pResult, handleHoles );
}


/// <summary>
/// Write data
/// </summary>
/// <param name="offset">Data offset in block</param>
/// <param name="size">Size of data to write</param>
/// <param name="pData">Buffer to write</param>
/// <returns>Status</returns>
NTSTATUS MemBlock::Write( uintptr_t offset, size_t size, const void* pData )
{
    if (!_pImpl)
        return STATUS_MEMORY_NOT_ALLOCATED;

    return _pImpl->_memory->Write( _pImpl->_ptr + offset, size, pData );
}

/// <summary>
/// Try to free memory and reset pointers
/// </summary>
void MemBlock::Reset()
{
    _pImpl.reset();
}

}