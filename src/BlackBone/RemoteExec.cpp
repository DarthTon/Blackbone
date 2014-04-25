#include "RemoteExec.h"
#include "Process.h"
#include "DynImport.h"

#include <VersionHelpers.h>

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
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    NTSTATUS dwResult = STATUS_SUCCESS;

    // Write code
    dwResult = CopyCode( pCode, size );
    if (dwResult != STATUS_SUCCESS)
        return dwResult;

    bool switchMode = (_proc.core().native()->GetWow64Barrier().type == wow_64_32);
    auto pExitThread = _mods.GetExport( _mods.GetModule( L"ntdll.dll" ), "NtTerminateThread" ).procAddress;
    if (pExitThread == 0)
        return LastNtStatus( STATUS_NOT_FOUND );

    ah.GenPrologue( switchMode );

    // Prepare thread to run in x64 mode
    if(switchMode)
    {
        // Allocate new x64 activation stack
        auto createActStack = _mods.GetExport( _mods.GetModule( L"ntdll.dll", LdrList, mt_mod64 ),
                                               "RtlAllocateActivationContextStack" ).procAddress;
        if (createActStack)
        {
            ah.GenCall( static_cast<size_t>(createActStack), { _userData.ptr<size_t>() + 0x3100 } );
            a.mov( AsmJit::nax, _userData.ptr<size_t>( ) + 0x3100 );
            a.mov( AsmJit::nax, AsmJit::sysint_ptr( AsmJit::nax ) );

            ah.SetTebPtr();
            a.mov( AsmJit::sysint_ptr( AsmJit::ndx, 0x2c8 ), AsmJit::nax );
        }
    }

    ah.GenCall( _userCode.ptr<size_t>(), { } );
    ah.ExitThreadWithStatus( pExitThread, _userData.ptr<size_t>( ) + INTRET_OFFSET );
    
    // Execute code in newly created thread
    if (_userCode.Write( size, a.getCodeSize(), a.make() ) == STATUS_SUCCESS)
    {
        auto thread = _threads.CreateNew( _userCode.ptr<ptr_t>() + size, _userData.ptr<ptr_t>() );

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
#ifdef _M_AMD64
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
        dwResult = WaitForSingleObject( _hWaitEvent, INFINITE );
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
    CreateRPCEnvironment( true );

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
        AsmJit::Assembler a;
        AsmJitHelper ah( a );

#ifdef _M_AMD64
        const int count = 15;
        AsmJit::GPReg regs[] = { AsmJit::rax, AsmJit::rbx, AsmJit::rcx, AsmJit::rdx, AsmJit::rsi,
                                 AsmJit::rdi, AsmJit::r8,  AsmJit::r9,  AsmJit::r10, AsmJit::r11,
                                 AsmJit::r12, AsmJit::r13, AsmJit::r14, AsmJit::r15, AsmJit::rbp };
        //
        // Preserve thread context
        // I don't care about FPU, XMM and anything else
        //
        a.sub(AsmJit::rsp, count * WordSize);  // Stack must be aligned on 16 bytes 
        a.pushfq();                            //

        // Save registers
        for (int i = 0; i < count; i++)
            a.mov( AsmJit::Mem( AsmJit::rsp, i * WordSize ), regs[i] );

        ah.GenCall( _userCode.ptr<size_t>(), { _userData.ptr<size_t>() } );
        AddReturnWithEvent( ah, rt_int32, INTRET_OFFSET );

        // Restore registers
        for (int i = 0; i < count; i++)
            a.mov( regs[i], AsmJit::Mem( AsmJit::rsp, i * WordSize ) );

        a.popfq();
        a.add( AsmJit::rsp, count * WordSize );

        a.jmp( ctx.Rip );
    #else
        a.pushad();
        a.pushfd();

        ah.GenCall( _userCode.ptr<size_t>(), { _userData.ptr<size_t>() } );
        AddReturnWithEvent( ah, rt_int32, INTRET_OFFSET );

        a.popfd();
        a.popad();

        a.push( ctx.NIP );
        a.ret();
    #endif

        if (_userCode.Write( size, a.getCodeSize(), a.make() ) == STATUS_SUCCESS)
        {
            ctx.NIP = _userCode.ptr<size_t>() + size;

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
        callResult = _userData.Read<size_t>( INTRET_OFFSET, 0 );
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
    auto thread = _threads.CreateNew( pCode, arg );

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
/// <param name="noThread">Create only codecave and sync event, without thread</param>
/// <returns>Status</returns>
NTSTATUS RemoteExec::CreateRPCEnvironment( bool noThread /*= false*/ )
{
    NTSTATUS dwResult = STATUS_SUCCESS;
    DWORD thdID = 0;
    bool status = true;

    //
    // Allocate environment codecave
    //
    if (!_workerCode.valid())
        _workerCode = _memory.Allocate( 0x1000 );

    if (!_userData.valid())
        _userData = _memory.Allocate( 0x4000, PAGE_READWRITE );

    if (!_userCode.valid())
        _userCode = _memory.Allocate( 0x1000 );

    // Create RPC thread and sync event
    if (noThread == false)
    {
        thdID = CreateWorkerThread();
        if (thdID)
            status = CreateAPCEvent( thdID );
    }
             
    if (thdID == 0 || status == false)
        dwResult = LastNtStatus();

    return dwResult;
}

/// <summary>
/// Create worker RPC thread
/// </summary>
/// <returns>Thread ID</returns>
DWORD RemoteExec::CreateWorkerThread()
{
    AsmJit::Assembler a;
    AsmJitHelper ah( a );
    AsmJit::Label l_loop = a.newLabel();

    //
    // Create execution thread
    //
    if(!_hWorkThd.valid())
    {
        eModType mt = mt_default;
        if (_memory.core().native()->GetWow64Barrier().type == wow_64_32)
        {
            mt = mt_mod64;

            ah.SwitchTo64();

            // Align stack on 16 byte boundary
            a.and_( AsmJit::nsp, -16 );

            // Allocate new x64 activation stack
            auto createActStack = _mods.GetExport( _mods.GetModule( L"ntdll.dll", LdrList, mt ),
                                                   "RtlAllocateActivationContextStack" ).procAddress;
            if(createActStack)
            {
                ah.GenCall( static_cast<size_t>(createActStack), { _userData.ptr<size_t>() + 0x3000 } );
                a.mov( AsmJit::nax, _userData.ptr<size_t>() + 0x3000 );
                a.mov( AsmJit::nax, AsmJit::sysint_ptr( AsmJit::nax ) );

                ah.SetTebPtr();
                a.mov( AsmJit::sysint_ptr( AsmJit::ndx, 0x2c8 ), AsmJit::nax );
            }
        }          

        auto proc = _mods.GetExport( _mods.GetModule( L"ntdll.dll", LdrList, mt ), "NtDelayExecution" ).procAddress;
        auto pExitThread = _mods.GetExport( _mods.GetModule( L"ntdll.dll" ), "NtTerminateThread" ).procAddress;
        if (proc == 0 || pExitThread == 0)
            return 0;

        /*
            for(;;)
                SleepEx(5, TRUE);

            ExitThread(SetEvent(m_hWaitEvent));
        */
        a.bind( l_loop );
        ah.GenCall( static_cast<size_t>(proc), { TRUE, _workerCode.ptr<size_t>() } );
        a.jmp( l_loop );

        ah.ExitThreadWithStatus( pExitThread, _userData.ptr<size_t>() );

        // Write code into process
        LARGE_INTEGER liDelay = { 0 };
        liDelay.QuadPart = -10 * 1000 * 5;

        _workerCode.Write( 0, liDelay );
        _workerCode.Write( sizeof(LARGE_INTEGER), a.getCodeSize(), a.make() );

        _hWorkThd = _threads.CreateNew( _workerCode.ptr<size_t>() + sizeof(LARGE_INTEGER), _userData.ptr<size_t>() );
    }

    return _hWorkThd.id();
}

/// <summary>
/// Create event to synchronize APC procedures
/// </summary>
/// <param name="threadID">The thread identifier.</param>
/// <returns>true on success</returns>
bool RemoteExec::CreateAPCEvent( DWORD threadID )
{         
    if(_hWaitEvent == NULL)
    {
        AsmJit::Assembler a;
        AsmJitHelper ah( a );

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

        // Prepare Arguments
        ustr.Length = static_cast<USHORT>(wcslen( pEventName ) * sizeof(wchar_t));
        ustr.MaximumLength = static_cast<USHORT>(sizeof(pEventName));
        ustr.Buffer = reinterpret_cast<PWSTR>(_userData.ptr<size_t>() + ARGS_OFFSET + sizeof(obAttr) + sizeof(ustr));

        obAttr.ObjectName = reinterpret_cast<PUNICODE_STRING>(_userData.ptr<size_t>() + ARGS_OFFSET + sizeof(obAttr));
        obAttr.Length = sizeof(obAttr);

        auto pCreateEvent = _mods.GetExport( _mods.GetModule( L"ntdll.dll", LdrList, mt ), "NtCreateEvent" ).procAddress;
        if (pCreateEvent == 0)
            return false;

        ah.GenCall( static_cast<size_t>(pCreateEvent), { 
            _userData.ptr<size_t>( ) + EVENT_OFFSET, EVENT_ALL_ACCESS,
            _userData.ptr<size_t>() + ARGS_OFFSET, 0, FALSE } );

        // Save status
        a.mov( AsmJit::ndx, _userData.ptr<size_t>() + ERR_OFFSET );
        a.mov( AsmJit::dword_ptr( AsmJit::ndx ), AsmJit::eax );

        a.ret();

        NTSTATUS status = _userData.Write( ARGS_OFFSET, obAttr );
        status |= _userData.Write( ARGS_OFFSET + sizeof(obAttr), ustr );
        status |= _userData.Write( ARGS_OFFSET + sizeof(obAttr) + sizeof(ustr), len, pEventName );

        if (status != STATUS_SUCCESS)
            return false;

        ExecInNewThread( a.make(), a.getCodeSize(), dwResult );

        status = _userData.Read<NTSTATUS>( ERR_OFFSET, -1 );
        if (status != STATUS_SUCCESS)
            return false;

        ustr.Buffer = pEventName;
        obAttr.ObjectName = &ustr;

        // Open created event
        status = GET_IMPORT( NtOpenEvent )( &_hWaitEvent, SYNCHRONIZE | EVENT_MODIFY_STATE, &obAttr );

        if (status != STATUS_SUCCESS)
            return false;
    }

    return true;
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
bool RemoteExec::PrepareCallAssembly( AsmJit::Assembler& a, 
                                      const void* pfn, 
                                      std::vector<AsmVariant>& args, 
                                      eCalligConvention cc, 
                                      eReturnType retType )
{
    size_t data_offset = ARGS_OFFSET;
    AsmJitHelper ah( a );

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
            arg.new_imm_val = _userData.ptr<size_t>() + data_offset;

            // Add some padding after data
            data_offset += arg.size + 0x10;
        }
    }

    // Insert hidden variable if return type is struct.
    // This variable contains address of buffer in which return value is copied
    if (retType == rt_struct)
    {
        args.emplace( args.begin(), AsmVariant( _userData.ptr<size_t>() + ARGS_OFFSET ) );
        args.front().new_imm_val = args.front().imm_val;
        args.front().type = AsmVariant::structRet;
    }
        
    ah.GenPrologue();
    ah.GenCall( pfn, args, cc );

    // Retrieve result from XMM0 or ST0
    if (retType == rt_float || retType == rt_double)
    {
        a.mov( AsmJit::nax, _userData.ptr<size_t>() + RET_OFFSET );

#ifdef _M_AMD64
        if (retType == rt_double)
            a.movsd( AsmJit::Mem( AsmJit::nax, 0 ), AsmJit::xmm0 );
        else
            a.movss( AsmJit::Mem( AsmJit::nax, 0 ), AsmJit::xmm0 );
#else
        a.fstp( AsmJit::Mem( AsmJit::nax, 0, retType * sizeof(float) ) );
#endif
    }

    AddReturnWithEvent( ah, retType );
    ah.GenEpilogue();

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
/// <param name="ah">Target assembly helper</param>
/// <param name="retType">Function return type</param>
/// <param name="retOffset">Return value offset</param>
void RemoteExec::AddReturnWithEvent( AsmHelperBase& ah, eReturnType retType /*= rt_int32 */, uint32_t retOffset /*= RET_OFFSET*/ )
{
    size_t ptr = _userData.ptr<size_t>();
    auto pSetEvent = _proc.modules().GetExport( _proc.modules().GetModule( L"ntdll.dll" ), "NtSetEvent" );
    ah.SaveRetValAndSignalEvent( pSetEvent.procAddress, ptr + retOffset, ptr + EVENT_OFFSET, ptr + ERR_OFFSET, retType );
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
        _workerCode.Free();
    }
}

/// <summary>
/// Reset instance
/// </summary>
void RemoteExec::reset()
{
    TerminateWorker();

    _hWorkThd = Thread( (HANDLE)NULL, &_proc.core() );

    _userCode.Reset();
    _userData.Reset();
    _workerCode.Reset();

    _apcPatched = false;
}

}