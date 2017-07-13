#pragma once
#include "Process.h"
#include "../Misc/Trace.hpp"

#include <stdint.h>
#include <type_traits>
#include <excpt.h>

namespace blackbone
{

/// <summary>
/// Multi-level pointer wrapper for local process
/// </summary>
template<typename T>
class multi_ptr
{
public:
    using type = std::remove_pointer_t<T>;
    using type_ptr = std::add_pointer_t<type>;
    using vecOffsets = std::vector<intptr_t>;

    constexpr static bool type_is_ptr = std::is_pointer_v<T>;

public:
    multi_ptr() = default;

    multi_ptr( uintptr_t base, const vecOffsets& offsets = vecOffsets() )
        : _base( base )
        , _offsets( offsets ) { }

    /// <summary>
    /// Object getters
    /// </summary>
    /// <returns>Object pointer, if valid</returns>
    inline type_ptr get()         { return read(); }
    inline operator type_ptr()    { return read(); }
    inline type_ptr operator ->() { return read(); }

protected:
    /// <summary>
    /// Get object pointer from base and offsets
    /// </summary>
    /// <returns>Object ptr, if valid</returns>
    virtual type_ptr read()
    {
        intptr_t i = -1;
        uintptr_t ptr = _base;

        __try
        {
            if (!ptr)
                return _ptr = nullptr;

            ptr = *reinterpret_cast<intptr_t*>(ptr);
            if (!_offsets.empty())
            {
                for (i = 0; i < static_cast<intptr_t>(_offsets.size()) - 1; i++)
                    ptr = *reinterpret_cast<uintptr_t*>(ptr + _offsets[i]);

                ptr += _offsets.back();
                if (type_is_ptr)
                    ptr = *reinterpret_cast<uintptr_t*>(ptr);
            }

            return _ptr = reinterpret_cast<type_ptr>(ptr);
        }
        // Invalid address
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        {
            BLACKBONE_TRACE(
                "Invalid pointer derefrence: base 0x%p, offset 0x%08x, target address 0x%p",
                _base, (i > 0 ? _offsets[i] : -1), ptr
            );

            return _ptr = nullptr;
        }
    }

protected:
    uintptr_t _base = 0;
    type_ptr _ptr = nullptr;
    std::vector<intptr_t> _offsets;
};

/// <summary>
/// Multi-level pointer wrapper for remote process
/// </summary>
template<typename T>
class multi_ptr_ex : public multi_ptr<T>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="multi_ptr_ex"/> class.
    /// </summary>
    /// <param name="proc">Target process</param>
    /// <param name="base">Base address</param>
    /// <param name="offsets">Offsets</param>
    multi_ptr_ex( Process* proc, uintptr_t base = 0, const vecOffsets& offsets = vecOffsets() )
        : _proc( proc )
        , multi_ptr( base, offsets ) { }

    /// <summary>
    /// Commit changed object into process
    /// </summary>
    /// <returns>Status code</returns>
    NTSTATUS commit()
    {
        auto ptr = get_ptr();
        if (ptr == 0)
            return STATUS_ACCESS_VIOLATION;

        return _proc->memory().Write( ptr, sizeof( _data ), &_data );
    }

private:
    /// <summary>
    /// Read object from pointer
    /// </summary>
    /// <returns>Pointer to local copy or nullptr if invalid</returns>
    virtual type_ptr read()
    { 
        auto ptr = get_ptr();
        if (ptr == 0)
            return nullptr;

        return NT_SUCCESS( _proc->memory().Read( ptr, sizeof( _data ), &_data ) ) ? &_data : nullptr;
    }
    
    /// <summary>
    /// Get target pointer
    /// </summary>
    /// <returns>Pointer value or 0 if chain is invalid</returns>
    uintptr_t get_ptr()
    {
        uintptr_t ptr = _base;
        if (!NT_SUCCESS( _proc->memory().Read( ptr, ptr ) ))
            return 0;

        if (!_offsets.empty())
        {
            for (intptr_t i = 0; i < static_cast<intptr_t>(_offsets.size()) - 1; i++)
                if (!NT_SUCCESS( _proc->memory().Read( ptr + _offsets[i], ptr ) ))
                    return 0;

            ptr += _offsets.back();
            if (type_is_ptr)
                if (!NT_SUCCESS( _proc->memory().Read( ptr, ptr ) ))
                    return 0;
        }

        return ptr;
    }


private:
    Process* _proc = nullptr;       // Target process
    type _data;                     // Local object copy
};
}