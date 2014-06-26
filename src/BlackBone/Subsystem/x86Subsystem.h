#pragma once

#include "NativeSubsystem.h"

namespace blackbone
{

/// <summary>
/// X86 OS subsystem. Used 
/// </summary>
class x86Native : public Native
{
public:
    BLACKBONE_API x86Native( HANDLE hProcess );
    BLACKBONE_API ~x86Native();

    /// <summary>
    /// Query virtual memory
    /// </summary>
    /// <param name="lpAddress">Address to query</param>
    /// <param name="lpBuffer">Retrieved memory info</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS VirtualQueryExT( ptr_t lpAddress, PMEMORY_BASIC_INFORMATION64 lpBuffer );

    /// <summary>
    /// Get WOW64 thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS GetThreadContextT( HANDLE hThread, _CONTEXT32& ctx );

    /// <summary>
    /// Get native thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS GetThreadContextT( HANDLE hThread, _CONTEXT64& ctx );

    /// <summary>
    /// Set WOW64 thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS SetThreadContextT( HANDLE hThread, _CONTEXT32& ctx );

    /// <summary>
    /// Set native thread context
    /// </summary>
    /// <param name="hThread">Thread handle.</param>
    /// <param name="ctx">Thread context</param>
    /// <returns>Status code</returns>
    virtual NTSTATUS SetThreadContextT( HANDLE hThread, _CONTEXT64& ctx );

    /// <summary>
    /// Gets WOW64 PEB
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

private:
};

}