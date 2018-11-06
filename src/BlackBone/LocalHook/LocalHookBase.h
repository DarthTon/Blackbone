#pragma once

#include "../Config.h"
#include "../Include/Winheaders.h"
#include "../Asm/AsmFactory.h"
#include "../Asm/LDasm.h"
#include "../Include/Macro.h"

#include <tuple>
#include <unordered_map>

namespace blackbone
{

namespace CallOrder
{
enum e
{
    HookFirst,      // Hook called before original function
    HookLast,       // Hook called after original function
    NoOriginal,     // Original function doesn't get called
};
}

namespace HookType
{
    enum e
    {
        Inline,         // Patch first few bytes
        Int3,           // Place Int3 breakpoint
        HWBP,           // Set hardware breakpoint

        // Reserved for internal use
        VTable,
        InternalInline  
    };
}

namespace ReturnMethod
{
    enum e
    {
        UseNew,         // Return value returned by hook
        UseOriginal     // Return original function value
    };
}

class DetourBase
{
    using mapIdx = std::unordered_map<DWORD, int>;

public:
    BLACKBONE_API DetourBase();
    BLACKBONE_API ~DetourBase();

protected:

    /// <summary>
    /// Allocate detour buffer as close to target as possible
    /// </summary>
    /// <param name="nearest">Target address</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool AllocateBuffer( uint8_t* nearest );

    /// <summary>
    /// Temporarily disable hook
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API bool DisableHook();

    /// <summary>
    /// Enable disabled hook
    /// </summary>
    /// <returns>true on success</returns>
    BLACKBONE_API bool EnableHook();

    /// <summary>
    /// Toggle hardware breakpoint for current thread
    /// </summary>
    /// <param name="index">Breakpoint index ( 0-4 )</param>
    /// <param name="enable">true to enable, false to disable</param>
    /// <returns>true on success</returns>
    BLACKBONE_API bool ToggleHBP( int index, bool enable );

    /// <summary>
    /// Copy original function bytes
    /// </summary>
    /// <param name="Ptr">Origianl function address</param>
    BLACKBONE_API void CopyOldCode( uint8_t* Ptr );

    /// <summary>
    /// Exception handlers
    /// </summary>
    /// <param name="excpt">Exception information</param>
    /// <returns>Exception disposition</returns>
    BLACKBONE_API static LONG NTAPI VectoredHandler ( PEXCEPTION_POINTERS excpt );
    BLACKBONE_API static LONG NTAPI Int3Handler     ( PEXCEPTION_POINTERS excpt );
    BLACKBONE_API static LONG NTAPI AVHandler       ( PEXCEPTION_POINTERS excpt );
    BLACKBONE_API static LONG NTAPI StepHandler     ( PEXCEPTION_POINTERS excpt );

protected:
    bool _hooked = false;               // Hook is installed

    void* _callback = nullptr;          // User supplied hook function
    void* _callbackClass = nullptr;     // Class pointer for user hook
    void* _original = nullptr;          // Original function address
    void* _internalHandler = nullptr;   // Pointer to hook handler
    void* _callOriginal = nullptr;      // Pointer to original function 
    
    mapIdx   _hwbpIdx;                  // Thread HWBP index
    size_t   _origSize = 0;             // Original code size
    uint8_t* _buf = nullptr;            // Trampoline buffer
    uint8_t* _origCode = nullptr;       // Original function bytes
    uint8_t* _origThunk = nullptr;      // Original bytes adjusted for relocation
    uint8_t* _newCode = nullptr;        // Trampoline bytes
    
    HookType::e _type = HookType::Inline;
    CallOrder::e _order = CallOrder::HookFirst;
    ReturnMethod::e _retType = ReturnMethod::UseOriginal;

    // Global hook instances relationship
    BLACKBONE_API static std::unordered_map<void*, DetourBase*> _breakpoints;

    // Exception handler
    BLACKBONE_API static void* _vecHandler;
};

}