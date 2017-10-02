#pragma once

#include "../Include/Winheaders.h"
#include "../Include/Macro.h"
#include "../Include/Types.h"
#include "../Include/CallResult.h"

#include <stdint.h>
#include <memory>

namespace blackbone
{

/// <summary>
/// Get rid of EXECUTABLE flag if DEP isn't enabled
/// </summary>
/// <param name="prot">Memry protection flags</param>
/// <param name="bDEP">DEP flag</param>
/// <returns>New protection flags</returns>
inline DWORD CastProtection( DWORD prot, bool bDEP )
{
    if (bDEP == true)
    {
        return prot;
    }
    else
    {
        if (prot == PAGE_EXECUTE_READ)
            return PAGE_READONLY;
        else if (prot == PAGE_EXECUTE_READWRITE)
            return PAGE_READWRITE;
        else if (prot == PAGE_EXECUTE_WRITECOPY)
            return PAGE_WRITECOPY;
        else
            return prot;
    }
}

class MemBlock
{
public:
    class MemBlockImpl
    {
        friend class MemBlock;

    public:
        MemBlockImpl() = default;

        /// <summary>
        /// MemBlock_p ctor
        /// </summary>
        /// <param name="mem">Process memory routines</param>
        /// <param name="ptr">Memory address</param>
        /// <param name="size">Block size</param>
        /// <param name="prot">Memory protection</param>
        /// <param name="own">true if caller will be responsible for block deallocation</param>
        MemBlockImpl( class ProcessMemory* mem, ptr_t ptr, size_t size, DWORD prot, bool own = true, bool physical = false );

        ~MemBlockImpl()
        {
            if (_own)
                Free();
        }

        /// <summary>
        /// Free memory
        /// </summary>
        /// <param name="size">Size of memory chunk to free. If 0 - whole block is freed</param>
        NTSTATUS Free( size_t size = 0 );

    private:
        ptr_t  _ptr = 0;                // Raw memory pointer
        size_t _size = 0;               // Region size
        DWORD  _protection = 0;         // Region protection
        bool   _own = true;             // Memory will be freed in destructor
        bool   _physical = false;       // Memory allocated as direct physical
        class ProcessMemory* _memory;   // Target process routines
    }; 

public:
    /// <summary>
    /// MemBlock ctor
    /// </summary>
    BLACKBONE_API MemBlock() = default;

    /// <summary>
    /// MemBlock ctor
    /// </summary>
    /// <param name="mem">Process memory routines</param>
    /// <param name="ptr">Memory address</param>
    /// <param name="own">true if caller will be responsible for block deallocation</param>
    BLACKBONE_API MemBlock( class ProcessMemory* mem, ptr_t ptr, bool own = true );

    /// <summary>
    /// MemBlock ctor
    /// </summary>
    /// <param name="mem">Process memory routines</param>
    /// <param name="ptr">Memory address</param>
    /// <param name="size">Block size</param>
    /// <param name="prot">Memory protection</param>
    /// <param name="own">true if caller will be responsible for block deallocation</param>
    BLACKBONE_API MemBlock( class ProcessMemory* mem, ptr_t ptr, size_t size, DWORD prot, bool own = true, bool physical = false );

    /// <summary>
    /// Move ctor
    /// </summary>
    /// <param name="rhs">Move from</param>
    BLACKBONE_API MemBlock( MemBlock&& rhs ) { _pImpl.swap( rhs._pImpl ); }
    BLACKBONE_API MemBlock& operator = ( MemBlock&& rhs )
    { 
        // Self assign
        if (_pImpl == rhs._pImpl)
            return *this;

        _pImpl.swap( rhs._pImpl ); 
        return *this;
    }

    /// <summary>
    /// Allocate new memory block
    /// </summary>
    /// <param name="process">Process memory routines</param>
    /// <param name="size">Block size</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <param name="protection">Win32 Memory protection flags</param>
    /// <param name="own">false if caller will be responsible for block deallocation</param>
    /// <returns>Memory block. If failed - returned block will be invalid</returns>
    BLACKBONE_API static call_result_t<MemBlock> Allocate(
        class ProcessMemory& process,
        size_t size,
        ptr_t desired = 0,
        DWORD protection = PAGE_EXECUTE_READWRITE,
        bool own = true
        );

    /// <summary>
    /// Reallocate existing block for new size
    /// </summary>
    /// <param name="size">New block size</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <param name="protection">Memory protection</param>
    /// <returns>New block address</returns>
    BLACKBONE_API call_result_t<ptr_t> Realloc( size_t size, ptr_t desired = 0, DWORD protection = PAGE_EXECUTE_READWRITE );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="protection">New protection flags</param>
    /// <param name="offset">Memory offset in block</param>
    /// <param name="size">Block size</param>
    /// <param name="pOld">Old protection flags</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Protect( DWORD protection, uintptr_t offset = 0, size_t size = 0, DWORD* pOld = nullptr );

    /// <summary>
    /// Free memory
    /// </summary>
    /// <param name="size">Size of memory chunk to free. If 0 - whole block is freed</param>
    BLACKBONE_API NTSTATUS Free( size_t size = 0 );

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
    BLACKBONE_API NTSTATUS Read( uintptr_t offset, size_t size, PVOID pResult, bool handleHoles = false );

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="offset">Data offset in block</param>
    /// <param name="size">Size of data to write</param>
    /// <param name="pData">Buffer to write</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Write( uintptr_t offset, size_t size, const void* pData );

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="offset">Data offset in block</param>
    /// <param name="def_val">Defult return value if read has failed</param>
    /// <returns>Read data</returns>
    template<typename T>
    T Read( uintptr_t offset, const T& def_val )
    {
        T res = def_val;
        Read( offset, sizeof( T ), &res );
        return res;
    }

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="offset">Data offset in block</param>
    /// <param name="def_val">Read data</param>
    /// <returns>Status code</returns>
    template<typename T>
    NTSTATUS Read( size_t offset, T& val )
    {
        return Read( offset, sizeof( val ), &val );
    }

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="offset">Offset in block</param>
    /// <param name="data">Data to write</param>
    /// <returns>Status</returns>
    template<typename T>
    NTSTATUS Write( uintptr_t offset, const T& data )
    {
        return Write( offset, sizeof( data ), &data );
    }

    /// <summary>
    /// Try to free memory and reset pointers
    /// </summary>
    BLACKBONE_API void Reset();

    /// <summary>
    /// Memory will not be deallocated upon object destruction
    /// </summary>
    BLACKBONE_API inline void Release() { if (_pImpl) _pImpl->_own = false; }

    /// <summary>
    /// Get memory pointer
    /// </summary>
    /// <returns>Memory pointer</returns>
    template<typename T = ptr_t>
    inline T ptr() const { return _pImpl ? (T)_pImpl->_ptr : T( 0 ); }

    /// <summary>
    /// Get block size
    /// </summary>
    /// <returns>Block size</returns>
    BLACKBONE_API inline size_t size() const { return _pImpl ? _pImpl->_size : 0; }

    /// <summary>
    /// Get block memory protection
    /// </summary>
    /// <returns>Memory protection flags</returns>
    BLACKBONE_API inline DWORD  protection() const { return _pImpl ? _pImpl->_protection : 0; }

    /// <summary>
    /// Validate memory block
    /// <returns>true if memory pointer isn't 0</returns>
    BLACKBONE_API inline bool valid() const { return( _pImpl.get() != nullptr && _pImpl->_ptr != 0); }

    /// <summary>
    /// Get memory pointer
    /// </summary>
    /// <returns>Memory pointer</returns>
    BLACKBONE_API inline operator ptr_t() const  { return _pImpl ? _pImpl->_ptr : 0; }

private:
    std::shared_ptr<MemBlockImpl> _pImpl;
};

}