#include "LocalHookBase.h"

namespace blackbone
{
std::unordered_map<void*, DetourBase*> DetourBase::_breakpoints;
void* DetourBase::_vecHandler = nullptr;

DetourBase::DetourBase()
{
}

DetourBase::~DetourBase()
{
    VirtualFree( _buf, 0, MEM_RELEASE );
}

/// <summary>
/// Allocate detour buffer as close to target as possible
/// </summary>
/// <param name="nearest">Target address</param>
/// <returns>true on success</returns>
bool DetourBase::AllocateBuffer( uint8_t* nearest )
{
    if (_buf != nullptr)
        return true;

    MEMORY_BASIC_INFORMATION mbi = { 0 };
    for (SIZE_T addr = (SIZE_T)nearest; addr > (SIZE_T)nearest - 0x80000000; addr = (SIZE_T)mbi.BaseAddress - 1)
    {
        if (!VirtualQuery( (LPCVOID)addr, &mbi, sizeof( mbi ) ))
            break;

        if (mbi.State == MEM_FREE)
        {
            _buf = (uint8_t*)VirtualAlloc(
                (uint8_t*)mbi.BaseAddress + mbi.RegionSize - 0x1000, 0x1000,
                MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE
                );

            if (_buf)
                break;
        }
    }

    // If allocating a memory page failed, allocate first suitable
    if (_buf == nullptr)
        _buf = (uint8_t*)VirtualAlloc( nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );

    _origCode = _buf + 0x100;
    _origThunk = _buf + 0x200;
    _newCode  = _buf + 0x300;

    return _buf != nullptr;
}


/// <summary>
/// Temporarily disable hook
/// </summary>
/// <returns>true on success</returns>
bool DetourBase::DisableHook()
{
    if (_type == HookType::InternalInline || _type == HookType::Int3)
        WriteProcessMemory( GetCurrentProcess(), _original, _origCode, _origSize, NULL );
    else if (_type == HookType::HWBP)
        ToggleHBP( _hwbpIdx[GetCurrentThreadId()], false );

    return true;
}

/// <summary>
/// Enable disabled hook
/// </summary>
/// <returns>true on success</returns>
bool DetourBase::EnableHook()
{
    if (_type == HookType::InternalInline || _type == HookType::Int3)
        WriteProcessMemory( GetCurrentProcess(), _original, _newCode, _origSize, NULL );
    else if (_type == HookType::HWBP)
        ToggleHBP( _hwbpIdx[GetCurrentThreadId()], true );

    return true;
}

/// <summary>
/// Toggle hardware breakpoint for current thread
/// </summary>
/// <param name="index">Breakpoint index ( 0-4 )</param>
/// <param name="enable">true to enable, false to disable</param>
/// <returns>true on success</returns>
bool DetourBase::ToggleHBP( int index, bool enable )
{
    CONTEXT context = { 0 };
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (GetThreadContext( GetCurrentThread(), &context ) != TRUE)
        return FALSE;

    if (enable)
        BitTestAndSetT( (LONG_PTR*)&context.Dr7, 2 * index );
    else
        BitTestAndResetT( (LONG_PTR*)&context.Dr7, 2 * index );

    return (SetThreadContext( GetCurrentThread(), &context ) != FALSE);
}

/// <summary>
/// Copy original function bytes
/// </summary>
/// <param name="Ptr">Origianl function address</param>
void DetourBase::CopyOldCode( uint8_t* ptr )
{ 
    // Store original bytes
    uint8_t* src = ptr;
    uint8_t* thunk = _origThunk, *original = _origCode;
    uint32_t all_len = 0;
    ldasm_data ld = { 0 };

    do 
    {
        uint32_t len = ldasm( src, &ld, is_x64 );

        // Determine code end
        if (ld.flags & F_INVALID
                || (len == 1 && (src[ld.opcd_offset] == 0xCC || src[ld.opcd_offset] == 0xC3))
                || (len == 3 && src[ld.opcd_offset] == 0xC2)
                || len + all_len > 128)
        {
            break;
        }

        // move instruction 
        memcpy( original, src, len );
        memcpy( thunk, src, len );

        // if instruction has relative offset, calculate new offset 
        if (ld.flags & F_RELATIVE)
        {
            int32_t diff = 0;
            const uintptr_t ofst = (ld.disp_offset != 0 ? ld.disp_offset : ld.imm_offset);
            const uintptr_t sz = ld.disp_size != 0 ? ld.disp_size : ld.imm_size;

            memcpy( &diff, src + ofst, sz );

        #ifdef USE64
            // exit if jump is greater then 2GB
            if (_abs64( src + len + diff - thunk ) > INT_MAX)
            {
                break;
            }
            else
            {
                diff += static_cast<int32_t>(src - thunk);
                memcpy( thunk + ofst, &diff, sz );
            }
        #else
            diff += src - thunk;
            memcpy( thunk + ofst, &diff, sz );
        #endif
        }

        src += len;
        thunk += len;
        original += len;
        all_len += len;
    } while (all_len < _origSize);

    // Failed to copy old code, use backup plan
    if (all_len < _origSize)
    {
        _type = HookType::InternalInline;
        memcpy( _origCode, ptr, _origSize );
    }         
    else
    {
        SET_JUMP( thunk, src );
        _callOriginal = _origThunk;
    } 
}


/// <summary>
/// Exception handler
/// </summary>
/// <param name="excpt">Exception information</param>
/// <returns>Exception disposition</returns>
LONG NTAPI DetourBase::VectoredHandler( PEXCEPTION_POINTERS excpt )
{
    switch (excpt->ExceptionRecord->ExceptionCode)
    {
        case static_cast<DWORD>(EXCEPTION_BREAKPOINT):
            return Int3Handler( excpt );

        case static_cast<DWORD>(EXCEPTION_ACCESS_VIOLATION):
            return AVHandler( excpt );

        case static_cast<DWORD>(EXCEPTION_SINGLE_STEP):
            return StepHandler( excpt );

        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }
}

/// <summary>
/// Int3 exception handler
/// </summary>
/// <param name="excpt">Exception information</param>
/// <returns>Exception disposition</returns>
LONG NTAPI DetourBase::Int3Handler( PEXCEPTION_POINTERS excpt )
{
    if (_breakpoints.count( excpt->ExceptionRecord->ExceptionAddress ))
    {
        DetourBase* pInst = _breakpoints[excpt->ExceptionRecord->ExceptionAddress];

        ((_NT_TIB*)NtCurrentTeb())->ArbitraryUserPointer = (void*)pInst;
        excpt->ContextRecord->NIP = (uintptr_t)pInst->_internalHandler;

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/// <summary>
/// Access violation handler
/// </summary>
/// <param name="excpt">Exception information</param>
/// <returns>Exception disposition</returns>
LONG NTAPI DetourBase::AVHandler( PEXCEPTION_POINTERS /*excpt*/ )
{
    return EXCEPTION_CONTINUE_SEARCH;
}

/// <summary>
/// Single step exception handler
/// </summary>
/// <param name="excpt">Exception information</param>
/// <returns>Exception disposition</returns>
LONG NTAPI DetourBase::StepHandler( PEXCEPTION_POINTERS excpt )
{
    DWORD index = 0;
    int found = _BitScanForward( &index, static_cast<DWORD>(excpt->ContextRecord->Dr6) );

    if (found != 0 && index < 4 && _breakpoints.count( excpt->ExceptionRecord->ExceptionAddress ))
    {
        DetourBase* pInst = _breakpoints[excpt->ExceptionRecord->ExceptionAddress];

        // Disable breakpoint at current index
        BitTestAndResetT( (LONG_PTR*)&excpt->ContextRecord->Dr7, 2 * index );

        ((_NT_TIB*)NtCurrentTeb())->ArbitraryUserPointer = (void*)pInst;
        excpt->ContextRecord->NIP = (uintptr_t)pInst->_internalHandler;

        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

}