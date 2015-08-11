#include "RemoteLocalHook.h"

#include "../Process.h"
#include "../../Asm/LDasm.h"

#ifdef USE64
#define HOOK_CTX_T(v, ctx) HookCtx64& v = ctx.hook64
#else
#define HOOK_CTX_T(v, ctx) HookCtx32& v = ctx.hook32
#endif

namespace blackbone
{

RemoteLocalHook::RemoteLocalHook( class Process& process )
    : _process( process )
{
}


RemoteLocalHook::~RemoteLocalHook()
{
}

NTSTATUS RemoteLocalHook::SetHook( ptr_t /*address*/, asmjit::Assembler& /*hook*/ )
{
    HookCtx ctx = { { 0 } };
    HOOK_CTX_T( ctx_t, ctx );
    UNREFERENCED_PARAMETER( ctx_t );

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS RemoteLocalHook::Restore( ptr_t address )
{
    // No such hook
    if (_hooks.count( address ) == 0)
        return STATUS_NOT_FOUND;

    HOOK_CTX_T( ctx_t, _hooks[address] );
    UNREFERENCED_PARAMETER( ctx_t );

    _hooks.erase( address );

    return STATUS_SUCCESS;
}

}