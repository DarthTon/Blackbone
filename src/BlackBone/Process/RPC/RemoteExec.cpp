#include "RemoteExec.h"
#include "../Process.h"
#include "../../Misc/DynImport.h"

#include <VersionHelpers.h>
#include <sddl.h>
#include <AccCtrl.h>
#include <Aclapi.h>

namespace blackbone
{

RemoteExec::RemoteExec( Process& proc )
    : _proc( proc )
    , _mods( _proc.modules() )
    , _memory( _proc.memory() )
    , _threads( _proc.threads() )
    , _hWorkThd( (DWORD)0, &_memory.core() )
    , _hWaitEvent( NULL )
    , _apcPatched( false )
{
    DynImport::load( "NtOpenEvent", L"ntdll.dll" );
    DynImport::load( "NtCreateEvent", L"ntdll.dll" );
}

RemoteExec::~RemoteExec()
{
    TerminateWorker();
}

/// <summary>
/// Create new thread and execute code in it. Wait until execution ends
/// </summary>
/// <param name="pCode">Code to execute</param>
/// <param name="size">Code size</param>
/// <param name="callResult">Code return value</param>
/// <returns>Status</returns>
NTSTATUS RemoteExec::ExecInNewThread( PVOID pCode, size_t size, uint64_t& callResult )
{
    AsmJitHelper a;
    NTSTATUS dwResult = STATUS_SUCCESS;

    // Write code
    dwResult = CopyCode( pCode, size );
    if (dwResult != STATUS_SUCCESS)
        return dwResult;

    bool switchMode = (_proc.core().native()->GetWow64Barrier().type == wow_64_32);
    auto pExitThread = _mods.GetExport( _mods.GetModule(
        L"ntdll.dll", LdrList, switchMode ? mt_mod64 : mt_default ),
        "NtTerminateThread" ).procAddress;

    if (pExitThread == 0)
        return LastNtStatus( STATUS_NOT_FOUND );

    a.GenPrologue( switchMode );

    // Prepare thread to run in x64 mode
    if(switchMode)
    {
        // Allocate new x64 activation stack
        auto createActStack = _mods.GetExport( _mods.GetModule( L"ntdll.dll", LdrList, mt_mod64 ),
                                               "RtlAllocateActivationContextStack" ).procAddress;
        if (createActStack)
        {
            a.GenCall( static_cast<uintptr_t>(createActStack), { _userData.ptr<uintptr_t>() + 0x3100 } );
            a->mov( a->zax, _userData.ptr<uintptr_t>( ) + 0x3100 );
            a->mov( a->zax, a->intptr_ptr( a->zax ) );

            a.SetTebPtr();
            a->mov( a->intptr_ptr( a->zdx, 0x2C8 ), a->zax );
        }
    }

    a.GenCall( _userCode.ptr<uintptr_t>(), { } );
    a.ExitThreadWithStatus( (uintptr_t)pExitThread, _userData.ptr<uintptr_t>() + INTRET_OFFSET );
    
    // Execute code in newly created thread
    if (_userCode.Write( size, a->getCodeSize(), a->make() ) == STATUS_SUCCESS)
    {
        auto thread = _threads.CreateNew( _userCode.ptr<ptr_t>() + size, _userData.ptr<ptr_t>()/*, HideFromDebug*/ );

        dwResult = thread.Join();
        callResult = _userData.Read<uint64_t>( INTRET_OFFSET, 0 );
    }
    else
        dwResult = LastNtStatus();

    return dwResult;
}

/// <summary>
/// Execute code in context of our worker thread
/// </summary>
/// <param name="pCode">Cde to execute</param>
/// <param name="size">Code size.</param>
/// <param name="callResult">Execution result</param>
/// <returns>Status</returns>
NTSTATUS RemoteExec::ExecInWorkerThread( PVOID pCode, size_t size, uint64_t& callResult )
{
    NTSTATUS dwResult = STATUS_SUCCESS;

    // Create thread if needed
    CreateRPCEnvironment();

    // Write code
    dwResult = CopyCode( pCode, size );
    if (dwResult != STATUS_SUCCESS)
        return dwResult;

    if (_hWaitEvent)
        ResetEvent( _hWaitEvent );

    // Patch KiUserApcDispatcher 
#ifdef USE64
    if (!_apcPatched && IsWindows7OrGreater() && !IsWindows8OrGreater())
    {
        if (_proc.core().native()->GetWow64Barrier().type == wow_64_32)
        {
            auto patchBase = _proc.nativeLdr().APC64PatchAddress();

            if (patchBase != 0)
            {
                DWORD flOld = 0;
                _memory.Protect(patchBase, 6, PAGE_EXECUTE_READWRITE, &flOld);
                _memory.Write(patchBase + 0x2, (uint8_t)0x0C);
                _memory.Write( patchBase + 0x4, (uint8_t)0x90 );
                _memory.Protect( patchBase, 6, flOld, nullptr );
            }

            _apcPatched = true;
        }
        else
            _apcPatched = true;
    }
#endif

    // Execute code in thread context
    if (QueueUserAPC( _userCode.ptr<PAPCFUNC>(), _hWorkThd.handle(), _userCode.ptr<ULONG_PTR>() ))
    {
        dwResult = WaitForSingleObject( _hWaitEvent, 30 * 1000 /*wait 30s*/ );
        callResult = _userData.Read<uint64_t>( RET_OFFSET, 0 );
    }
    else
        return LastNtStatus();

    // Ensure APC function fully returns
    Sleep( 1 );

    return dwResult;
}

/// <summary>
/// Execute code in context of any existing thread
/// </summary>
/// <param name="pCode">Cde to execute</param>
/// <param name="size">Code size.</param>
/// <param name="callResult">Execution result</param>
/// <param name="thd">Target thread</param>
/// <returns>Status</returns>
NTSTATUS RemoteExec::ExecInAnyThread( PVOID pCode, size_t size, uint64_t& callResult, Thread& thd )
{
    NTSTATUS dwResult = STATUS_SUCCESS;
    CONTEXT_T ctx;

    // Prepare for remote exec
    CreateRPCEnvironment( false, true );

    // Write code
    dwResult = CopyCode( pCode, size );
    if (dwResult != STATUS_SUCCESS)
        return dwResult;

    if (_hWaitEvent)
        ResetEvent( _hWaitEvent );

    if (!thd.Suspend())
        return LastNtStatus();

    if (thd.GetContext( ctx, CONTEXT_ALL, true ))
    {
        AsmJitHelper a;

#ifdef USE64
        const int count = 15;
        static const asmjit::GpReg regs[] = 
        {
            asmjit::host::rax, asmjit::host::rbx, asmjit::host::rcx, asmjit::host::rdx, asmjit::host::rsi,
            asmjit::host::rdi, asmjit::host::r8,  asmjit::host::r9,  asmjit::host::r10, asmjit::host::r11,
            asmjit::host::r12, asmjit::host::r13, asmjit::host::r14, asmjit::host::r15, asmjit::host::rbp     
        };

        //
        // Preserve thread context
        // I don't care about FPU, XMM and anything else
        //
        a->sub( asmjit::host::rsp, count * WordSize );  // Stack must be aligned on 16 bytes 
        a->pushf();                                     //

        // Save registers
        for (int i = 0; i < count; i++)
            a->mov( asmjit::Mem( asmjit::host::rsp, i * WordSize ), regs[i] );

        a.GenCall( _userCode.ptr<uintptr_t>(), { _userData.ptr<uintptr_t>() } );
        AddReturnWithEvent( a, mt_default, rt_int32, INTRET_OFFSET );

        // Restore registers
        for (int i = 0; i < count; i++)
            a->mov( regs[i], asmjit::Mem( asmjit::host::rsp, i * WordSize ) );

        a->popf();
        a->add( asmjit::host::rsp, count * WordSize );

        // jmp [rip]
        a->dw( '\xFF\x25' ); a->dd( 0 );
        a->dq( ctx.Rip );
    #else
        a->pusha();
        a->pushf();

        a.GenCall( _userCode.ptr<uintptr_t>(), { _userData.ptr<uintptr_t>() } );
        AddReturnWithEvent( a, mt_default, rt_int32, INTRET_OFFSET );

        a->popf();
        a->popa();

        a->push( (uintptr_t)ctx.NIP );
        a->ret();
    #endif

        if (_userCode.Write( size, a->getCodeSize(), a->make() ) == STATUS_SUCCESS)
        {
            ctx.NIP = _userCode.ptr<uintptr_t>() + size;

            if (!thd.SetContext( ctx, true ))
                dwResult = LastNtStatus();
        }
        else
            dwResult = LastNtStatus();
    }
    else
        dwResult = LastNtStatus();

    thd.Resume();

    if (dwResult == STATUS_SUCCESS)
    {
        WaitForSingleObject( _hWaitEvent, INFINITE );
        callResult = _userData.Read<uintptr_t>( INTRET_OFFSET, 0 );
    }

    return dwResult;
}


/// <summary>
/// Create new thread with specified entry point and argument
/// </summary>
/// <param name="pCode">Entry point</param>
/// <param name="arg">Thread function argument</param>
/// <returns>Thread exit code</returns>
DWORD RemoteExec::ExecDirect( ptr_t pCode, ptr_t arg )
{
    auto thread = _threads.CreateNew( pCode, arg/*, HideFromDebug*/ );

    thread.Join();
    return thread.ExitCode();
}

/// <summary>
/// Create environment for future remote procedure calls
///
/// _userData layout (x86/x64):
/// --------------------------------------------------------------------------------------------------------------------------
/// | Internal return value | Return value |  Last Status code  |  Event handle   |  Space for copied arguments and strings  |
/// -------------------------------------------------------------------------------------------------------------------------
/// |       8/8 bytes       |   8/8 bytes  |      8/8 bytes     |   16/16 bytes   |                                          |
/// --------------------------------------------------------------------------------------------------------------------------
/// </summary>
/// <param name="bThread">Create worker thread</param>
/// <param name="bEvent">Create sync event for worker thread</param>
/// <returns>Status</returns>
NTSTATUS RemoteExec::CreateRPCEnvironment( bool bThread /*= true*/, bool bEvent /*= true*/ )
{
    DWORD thdID = GetTickCount();       // randomize thread id
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Allocate environment codecave
    //
    if (!_workerCode.valid())
        _workerCode = _memory.Allocate( 0x1000 );

    if (!_userData.valid())
        _userData = _memory.Allocate( 0x4000, PAGE_READWRITE );

    if (!_userCode.valid())
        _userCode = _memory.Allocate( 0x1000 );

    // Create RPC thread
    if (bThread)
        thdID = CreateWorkerThread();

    // Create RPC sync event
    if (bEvent)
    {
        if (_proc.core().native()->GetWow64Barrier().type != wow_32_64)
        {
            status = CreateAPCEvent( thdID );
        }
        else
        {
            status = LastNtStatus( STATUS_NOT_SUPPORTED );
        }
    }
   
    if (bThread && thdID == 0)
        status = LastNtStatus();

    return status;
}

/// <summary>
/// Create worker RPC thread
/// </summary>
/// <returns>Thread ID</returns>
DWORD RemoteExec::CreateWorkerThread()
{
    AsmJitHelper a;
    asmjit::Label l_loop = a->newLabel();

    //
    // Create execution thread
    //
    if(!_hWorkThd.valid())
    {
        eModType mt = mt_default;
        if (_memory.core().native()->GetWow64Barrier().type == wow_64_32)
        {
            mt = mt_mod64;
            a.SwitchTo64();

            // Align stack on 16 byte boundary
            a->and_( a->zsp, -16 );

            // Allocate new x64 activation stack
            auto createActStack = _mods.GetExport( _mods.GetModule( L"ntdll.dll", LdrList, mt ),
                                                   "RtlAllocateActivationContextStack" ).procAddress;
            if(createActStack)
            {
                a.GenCall( static_cast<uintptr_t>(createActStack), { _userData.ptr<uintptr_t>() + 0x3000 } );
                a->mov( a->zax, _userData.ptr<uintptr_t>() + 0x3000 );
                a->mov( a->zax, a->intptr_ptr( a->zax ) );

                a.SetTebPtr();
                a->mov( a->intptr_ptr( a->zdx, 0x2c8 ), a->zax );
            }
        }          

        auto ntdll = _mods.GetModule( L"ntdll.dll", Sections, mt );
        auto proc = _mods.GetExport( ntdll, "NtDelayExecution" ).procAddress;
        auto pExitThread = _mods.GetExport( ntdll, "NtTerminateThread" ).procAddress;
        if (proc == 0 || pExitThread == 0)
            return 0;

        /*
            for(;;)
                SleepEx(5, TRUE);

            ExitThread(SetEvent(m_hWaitEvent));
        */
        a->bind( l_loop );
        a.GenCall( static_cast<uintptr_t>(proc), { TRUE, _workerCode.ptr<uintptr_t>() } );
        a->jmp( l_loop );

        a.ExitThreadWithStatus( (uintptr_t)pExitThread, _userData.ptr<uintptr_t>() );

        // Write code into process
        LARGE_INTEGER liDelay = { { 0 } };
        liDelay.QuadPart = -10 * 1000 * 5;

        _workerCode.Write( 0, liDelay );
        _workerCode.Write( sizeof(LARGE_INTEGER), a->getCodeSize(), a->make() );

        _hWorkThd = _threads.CreateNew( _workerCode.ptr<uintptr_t>() + sizeof(LARGE_INTEGER), _userData.ptr<size_t>()/*, HideFromDebug*/ );
    }

    return _hWorkThd.id();
}


/// <summary>
/// Create event to synchronize APC procedures
/// </summary>
/// <param name="threadID">The thread identifier.</param>
/// <returns>Status code</returns>
NTSTATUS RemoteExec::CreateAPCEvent( DWORD threadID )
{         
    NTSTATUS status = STATUS_SUCCESS;

    if(_hWaitEvent == NULL)
    {
        AsmJitHelper a;

        wchar_t pEventName[128] = { 0 };
        uint64_t dwResult = NULL;
        size_t len = sizeof(pEventName);
        OBJECT_ATTRIBUTES obAttr = { 0 };
        UNICODE_STRING ustr = { 0 };

        // Detect ntdll type
        eModType mt = mt_default;
        if (_memory.core().native()->GetWow64Barrier().type == wow_64_32)
            mt = mt_mod64;

        // Event name
        swprintf_s( pEventName, ARRAYSIZE( pEventName ), L"\\BaseNamedObjects\\_MMapEvent_0x%x_0x%x", threadID, GetTickCount() );

        wchar_t* szStringSecurityDis = L"S:(ML;;NW;;;LW)D:(A;;GA;;;S-1-15-2-1)(A;;GA;;;WD)";
        PSECURITY_DESCRIPTOR pDescriptor = nullptr;
        ConvertStringSecurityDescriptorToSecurityDescriptorW( szStringSecurityDis, SDDL_REVISION_1, &pDescriptor, NULL );

        // Prepare Arguments
        ustr.Length = static_cast<USHORT>(wcslen( pEventName ) * sizeof(wchar_t));
        ustr.MaximumLength = static_cast<USHORT>(sizeof(pEventName));
        ustr.Buffer = pEventName;

        obAttr.ObjectName = &ustr;
        obAttr.Length = sizeof(obAttr);
        obAttr.SecurityDescriptor = pDescriptor;

        auto pOpenEvent = _mods.GetExport( _mods.GetModule( L"ntdll.dll", Sections, mt ), "NtOpenEvent" ).procAddress;
        if (pOpenEvent == 0)
            return false;

        status = SAFE_NATIVE_CALL( NtCreateEvent, &_hWaitEvent, EVENT_ALL_ACCESS, &obAttr, 0, static_cast<BOOLEAN>(FALSE) );
        if(pDescriptor)
            LocalFree( pDescriptor );

        if (!NT_SUCCESS( status ))
            return LastNtStatus( status );

        ustr.Buffer = reinterpret_cast<PWSTR>(_userData.ptr<uintptr_t>() + ARGS_OFFSET + sizeof( obAttr ) + sizeof( ustr ));
        obAttr.ObjectName = reinterpret_cast<PUNICODE_STRING>(_userData.ptr<uintptr_t>() + ARGS_OFFSET + sizeof(obAttr));
        obAttr.SecurityDescriptor = nullptr;

        a.GenCall( static_cast<uintptr_t>(pOpenEvent), {
            _userData.ptr<uintptr_t>() + EVENT_OFFSET, EVENT_MODIFY_STATE | SYNCHRONIZE,
            _userData.ptr<uintptr_t>() + ARGS_OFFSET } );

        // Save status
        a->mov( a->zdx, _userData.ptr<uintptr_t>() + ERR_OFFSET );
        a->mov( asmjit::host::dword_ptr( a->zdx ), asmjit::host::eax );

        a->ret();

        status = _userData.Write( ARGS_OFFSET, obAttr );
        status |= _userData.Write( ARGS_OFFSET + sizeof(obAttr), ustr );
        status |= _userData.Write( ARGS_OFFSET + sizeof(obAttr) + sizeof(ustr), len, pEventName );
        if (!NT_SUCCESS( status ))
            return LastNtStatus( status );

        ExecInNewThread( a->make(), a->getCodeSize(), dwResult );
        status = _userData.Read<NTSTATUS>( ERR_OFFSET, -1 );
    }

    return LastNtStatus( status );
}

/// <summary>
/// Generate assembly code for remote call.
/// </summary>
/// <param name="a">Underlying assembler object</param>
/// <param name="pfn">Remote function pointer</param>
/// <param name="args">Function arguments</param>
/// <param name="cc">Calling convention</param>
/// <param name="retType">Return type</param>
/// <returns>true on success</returns>
bool RemoteExec::PrepareCallAssembly( 
    AsmHelperBase& a, 
    const void* pfn,
    std::vector<AsmVariant>& args,
    eCalligConvention cc,
    eReturnType retType
    )
{
    uintptr_t data_offset = ARGS_OFFSET;

    // Invalid calling convention
    if (cc < cc_cdecl || cc > cc_fastcall)
    {
        LastNtStatus( STATUS_INVALID_PARAMETER_3 );
        return false;
    }

    // Copy structures and strings
    for (auto& arg : args)
    {
        if (arg.type == AsmVariant::dataStruct || arg.type == AsmVariant::dataPtr)
        {
            _userData.Write( data_offset, arg.size, reinterpret_cast<const void*>(arg.imm_val) );
            arg.new_imm_val = _userData.ptr<uintptr_t>() + data_offset;

            // Add some padding after data
            data_offset += arg.size + 0x10;
        }
    }

    // Insert hidden variable if return type is struct.
    // This variable contains address of buffer in which return value is copied
    if (retType == rt_struct)
    {
        args.emplace( args.begin(), AsmVariant( _userData.ptr<uintptr_t>() + ARGS_OFFSET ) );
        args.front().new_imm_val = args.front().imm_val;
        args.front().type = AsmVariant::structRet;
    }
        
    a.GenPrologue();
    a.GenCall( pfn, args, cc );

    // Retrieve result from XMM0 or ST0
    if (retType == rt_float || retType == rt_double)
    {
        a->mov( a->zax, _userData.ptr<size_t>() + RET_OFFSET );

#ifdef USE64
        if (retType == rt_double)
            a->movsd( asmjit::Mem( a->zax, 0 ), asmjit::host::xmm0 );
        else
            a->movss( asmjit::Mem( a->zax, 0 ), asmjit::host::xmm0 );
#else
        a->fstp( asmjit::Mem( a->zax, 0, retType * sizeof( float ) ) );
#endif
    }

    AddReturnWithEvent( a, mt_default, retType );
    a.GenEpilogue();

    return true;
}

/// <summary>
/// Copy executable code into remote codecave for future execution
/// </summary>
/// <param name="pCode">Code to copy</param>
/// <param name="size">Code size</param>
/// <returns>Status</returns>
NTSTATUS RemoteExec::CopyCode( PVOID pCode, size_t size )
{
    if (!_userCode.valid())
        _userCode = _memory.Allocate( size );

    // Reallocate for larger code
    if (size > _userCode.size())
        if ((_userCode.Realloc(size)) == 0)
            return LastNtStatus();

    if (_userCode.Write( 0, size, pCode ) != STATUS_SUCCESS)
        return LastNtStatus();

    return STATUS_SUCCESS;
}

/// <summary>
/// Generate return from function with event synchronization
/// </summary>
/// <param name="a">Target assembly helper</param>
/// <param name="mt">32/64bit loader</param>
/// <param name="retType">Function return type</param>
/// <param name="retOffset">Return value offset</param>
void RemoteExec::AddReturnWithEvent(
    AsmHelperBase& a,
    eModType mt /*= mt_default*/,
    eReturnType retType /*= rt_int32 */,
    uint32_t retOffset /*= RET_OFFSET*/ 
    )
{
    // Allocate block if missing
    if (!_userData.valid())
        _userData = _memory.Allocate( 0x4000, PAGE_READWRITE );

    uintptr_t ptr = _userData.ptr<size_t>();
    auto pSetEvent = _proc.modules().GetExport( _proc.modules().GetModule( L"ntdll.dll", LdrList, mt ), "NtSetEvent" );
    a.SaveRetValAndSignalEvent( (uintptr_t)pSetEvent.procAddress, ptr + retOffset, ptr + EVENT_OFFSET, ptr + ERR_OFFSET, retType );
}


/// <summary>
/// Terminate existing worker thread
/// </summary>
void RemoteExec::TerminateWorker()
{
    // Close event
    if(_hWaitEvent)
    {
        CloseHandle( _hWaitEvent );
        _hWaitEvent = NULL;
    }

    // Stop thread
    if(_hWorkThd.valid())
    {
        _hWorkThd.Terminate();
        _hWorkThd.Join();
        _hWorkThd.Close();
        _workerCode.Free();
    }
}

/// <summary>
/// Reset instance
/// </summary>
void RemoteExec::reset()
{
    TerminateWorker();

    _userCode.Reset();
    _userData.Reset();
    _workerCode.Reset();

    _apcPatched = false;
}

}