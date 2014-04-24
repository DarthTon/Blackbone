#pragma once

#include "Winheaders.h"
#include "AsmHelper.h"
#include "LDasm.h"
#include "Macro.h"

#include <tuple>
#include <unordered_map>

namespace blackbone
{

namespace CallOrder
{
enum e
{
    HookFirst,
    HookLast,
    NoOriginal,
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
    UseNew,
    UseOriginal
};
}

class DetourBase
{
    typedef std::unordered_map<DWORD, int> mapIdx;

public:
    DetourBase();
    ~DetourBase();

protected:

    /// <summary>
    /// Temporarily disable hook
    /// </summary>
    /// <returns>true on success</returns>
    bool DisableHook();

    /// <summary>
    /// Enable disabled hook
    /// </summary>
    /// <returns>true on success</returns>
    bool EnableHook();

    /// <summary>
    /// Toggle hardware breakpoint for current thread
    /// </summary>
    /// <param name="index">Breakpoint index ( 0-4 )</param>
    /// <param name="enable">true to enable, false to disable</param>
    /// <returns>true on success</returns>
    bool ToggleHBP( int index, bool enable );

    /// <summary>
    /// Copy original function bytes
    /// </summary>
    /// <param name="Ptr">Origianl function address</param>
    void CopyOldCode( uint8_t* Ptr );

    /// <summary>
    /// Exception handlers
    /// </summary>
    /// <param name="excpt">Exception information</param>
    /// <returns>Exception disposition</returns>
    static LONG NTAPI VectoredHandler   ( PEXCEPTION_POINTERS excpt );
    static LONG NTAPI Int3Handler       ( PEXCEPTION_POINTERS excpt );
    static LONG NTAPI AVHandler         ( PEXCEPTION_POINTERS excpt );
    static LONG NTAPI StepHandler       ( PEXCEPTION_POINTERS excpt );

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
    uint8_t* _newCode = nullptr;        // Trampoline bytes
    
    HookType::e _type = HookType::Inline;
    CallOrder::e _order = CallOrder::HookFirst;
    ReturnMethod::e _retType = ReturnMethod::UseOriginal;

    // Global hook instances relationship
    static std::unordered_map<void*, DetourBase*> _breakpoints;

    // Exception handler
    static void* _vecHandler;        
};

}