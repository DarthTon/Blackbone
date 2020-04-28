#pragma once

#include "../Include/Winheaders.h"
#include "../Include/Types.h"
#include "../Include/Macro.h"

#include <string>
#include <list>
#include <vector>
#include <unordered_set>
#include <cassert>


namespace blackbone
{

enum CreateThreadFlags
{
    NoThreadFlags   = 0x0000,
    CreateSuspended = 0x0001,
    NoDllCallbacks  = 0x0002,
    HideFromDebug   = 0x0004,
};

ENUM_OPS(CreateThreadFlags)

class Native
{
public:
    BLACKBONE_API Native( HANDLE hProcess, bool x86OS = false );
    BLACKBONE_API ~Native();

    BLACKBONE_API inline const Wow64Barrier& GetWow64Barrier() const { return _wowBarrier; }

    /// <summary>
    /// Allocate virtual memory
    /// </summary>
    /// <param name="lpAddress">Allocation address</param>
    /// <param name="dwSize">Region size</param>
    /// <param name="flAllocationType">Allocation type</param>
    /// <param name="flProtect">Memory protection</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS VirtualAllocExT( ptr_t& lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect );

    /// <summary>
    /// Free virtual memory
    /// </summary>
    /// <param name="lpAddress">Memory address</param>
    /// <param name="dwSize">Region size</param>
    /// <param name="dwFreeType">Memory release type.</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS VirtualFreeExT( ptr_t lpAddress, size_t dwSize, DWORD dwFreeType );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="lpAddress">Memory address.</param>
    /// <param name="dwSize">Region size</param>
    /// <param name="flProtect">New protection.</param>
    /// <param name="flOld">Old protection</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS VirtualProtectExT( ptr_t lpAddress, DWORD64 dwSize, DWORD flProtect, DWORD* flOld );

    /// <summary>
    /// Read virtual memory
    /// </summary>
    /// <param name="lpBaseAddress">Memory address</param>
    /// <param name="lpBuffer">Output buffer</param>
    /// <param name="nSize">Number of bytes to read</param>
    /// <param name="lpBytes">Mumber of bytes read</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS ReadProcessMemoryT( ptr_t lpBaseAddress, LPVOID lpBuffer, size_t nSize, DWORD64 *lpBytes = nullptr );

    /// <summary>
    /// Write virtual memory
    /// </summary>
    /// <param name="lpBaseAddress">Memory address</param>
    /// <param name="lpBuffer">Buffer to write</param>
    /// <param name="nSize">Number of bytes to read</param>
    /// <param name="lpBytes">Mumber of bytes read</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS WriteProcessMemoryT( ptr_t lpBaseAddress, LPCVOID lpBuffer, size_t nSize, DWORD64 *lpBytes = nullptr );

    /// <summary>
    /// Query virtual memory
    /// </summary>
    /// <param name="lpAddress">Address to query</param>
    /// <param name="lpBuffer">Retrieved memory info</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS VirtualQueryExT( ptr_t lpAddress, PMEMORY_BASIC_INFORMATION64 lpBuffer );

    /// <summary>
    /// Query virtual memory
    /// </summary>
    /// <param name="lpAddress">Address to query</param>
    /// <param name="lpBuffer">Retrieved memory info</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS VirtualQueryExT( ptr_t lpAddress, MEMORY_INFORMATION_CLASS infoClass, LPVOID lpBuffer, size_t bufSize );

    /// <summary>
    /// Call NtQueryInformationProcess for underlying process
    /// </summary>
    /// <param name="infoClass">Information class</param>
    /// <param name="lpBuffer">Output buffer</param>
    /// <param name="bufSize">Buffer size</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS QueryProcessInfoT( PROCESSINFOCLASS infoClass, LPVOID lpBuffer, uint32_t bufSize );

    /// <summary>
    /// Call NtSetInformationProcess for underlying process
    /// </summary>
    /// <param name="infoClass">Information class</param>
    /// <param name="lpBuffer">Input buffer</param>
    /// <param name="bufSize">Buffer size</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS SetProcessInfoT( PROCESSINFOCLASS infoClass, LPVOID lpBuffer, uint32_t bufSize );

    /// <summary>
    /// Creates new thread in the remote process
    /// </summary>
    /// <param name="hThread">Created thread handle</param>
    /// <param name="entry">Thread entry point</param>
    /// <param name="arg">Thread argument</param>
    /// <param name="flags">Creation flags</param>
    /// <param name="access">Access override</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS CreateRemoteThreadT( HANDLE& hThread, ptr_t entry, ptr_t arg, CreateThreadFlags flags, DWORD access = THREAD_ALL_ACCESS );

    /// <summary>
    /// Get native thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS GetThreadContextT( HANDLE hThread, _CONTEXT64& ctx );

    /// <summary>
    /// Get WOW64 thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS GetThreadContextT( HANDLE hThread, _CONTEXT32& ctx );

    /// <summary>
    /// Set native thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS SetThreadContextT( HANDLE hThread, _CONTEXT64& ctx );

    /// <summary>
    /// Set WOW64 thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS SetThreadContextT( HANDLE hThread, _CONTEXT32& ctx );

    /// <summary>
    /// NtQueueApcThread
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="func">APC function</param>
    /// <param name="arg">APC argument</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS QueueApcT( HANDLE hThread, ptr_t func, ptr_t arg );

    /// <summary>
    /// Get WOW64 PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB</param>
    /// <returns>PEB pointer</returns>
    virtual ptr_t getPEB( _PEB32* ppeb );

    /// <summary>
    /// Get native PEB
    /// </summary>
    /// <param name="ppeb">Retrieved PEB</param>
    /// <returns>PEB pointer</returns>
    virtual ptr_t getPEB( _PEB64* ppeb );

    /// <summary>
    /// Get WOW64 TEB
    /// </summary>
    /// <param name="ppeb">Retrieved TEB</param>
    /// <returns>TEB pointer</returns>
    virtual ptr_t getTEB( HANDLE hThread, _TEB32* pteb );

    /// <summary>
    /// Get native TEB
    /// </summary>
    /// <param name="ppeb">Retrieved TEB</param>
    /// <returns>TEB pointer</returns>
    virtual ptr_t getTEB( HANDLE hThread, _TEB64* pteb );

    /// <summary>
    /// Enumerate valid memory regions
    /// </summary>
    /// <param name="includeFree">If true - non-allocated regions will be included in list</param>
    /// <returns>Found regions</returns>>
    BLACKBONE_API std::vector<MEMORY_BASIC_INFORMATION64> EnumRegions( bool includeFree = false );

    /// <summary>
    /// Enumerate process modules
    /// </summary>
    /// <param name="result">Found modules</param>
    /// <param name="mtype">Module type: x86 or x64</param>
    /// <returns>Module count</returns>
    BLACKBONE_API std::vector<ModuleDataPtr> EnumModules( eModSeachType search = LdrList, eModType mtype = mt_default );

    /// <summary>
    /// Get lowest possible valid address value
    /// </summary>
    /// <returns>Address value</returns>
    BLACKBONE_API inline ptr_t minAddr() const { return 0x10000; }

    /// <summary>
    /// Get highest possible valid address value
    /// </summary>
    /// <returns>Address value</returns>
    BLACKBONE_API inline ptr_t maxAddr() const { return 0x7FFFFFFEFFFF; }
    
    /// <summary>
    /// Get highest possible valid address value
    /// </summary>
    /// <returns>Address value</returns>
    BLACKBONE_API inline ptr_t maxAddr32() const { return 0x7FFFFFFF; }
                                                                        
    /// <summary>
    /// Get page size
    /// </summary>
    /// <returns>Address value</returns>
    BLACKBONE_API inline uint32_t pageSize() const { return _pageSize; }
private:

    /// <summary>
    /// Enumerate process modules
    /// </summary>
    /// <param name="result">Found modules</param>
    /// <returns>Module count</returns>
    template<typename T>
    std::vector<ModuleDataPtr> EnumModulesT();

    /// <summary>
    /// Enum process section objects
    /// </summary>
    /// <param name="result">Found modules</param>
    /// <returns>Sections count</returns>
    std::vector<ModuleDataPtr> EnumSections();

    /// <summary>
    /// Enum pages containing valid PE headers
    /// </summary>
    /// <param name="result">Found modules</param>
    /// <returns>Sections count</returns>
    std::vector<ModuleDataPtr> EnumPEHeaders();

protected:
    HANDLE _hProcess;           // Process handle
    Wow64Barrier _wowBarrier;   // WOW64 barrier info
    uint32_t _pageSize;
};

}
