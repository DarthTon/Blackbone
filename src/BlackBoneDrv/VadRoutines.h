#pragma once

#include "Private.h"

/// <summary>
/// Change VAD protection flags
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">target address</param>
/// <param name="prot">New protection flags</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectVAD( IN PEPROCESS pProcess, IN ULONG_PTR address, IN ULONG prot );

/// <summary>
/// Hide memory from NtQueryVirtualMemory
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Target address</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkVAD( IN PEPROCESS pProcess, IN ULONG_PTR address );

/// <summary>
/// Get region VAD type
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Target address</param>
/// <param name="pType">Resulting VAD type</param>
/// <returns>Status code</returns>
NTSTATUS BBGetVadType( IN PEPROCESS pProcess, IN ULONG_PTR address, OUT PMI_VAD_TYPE pType );

/// <summary>
/// Find VAD that describes target address
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Address to find</param>
/// <param name="pResult">Found VAD. NULL if not found</param>
/// <returns>Status code</returns>
NTSTATUS BBFindVAD( IN PEPROCESS pProcess, IN ULONG_PTR address, OUT PMMVAD_SHORT* pResult );

/// <summary>
/// Convert protection flags
/// </summary>
/// <param name="prot">Protection flags.</param>
/// <param name="fromPTE">If TRUE - convert to PTE protection, if FALSE - convert to Win32 protection</param>
/// <returns>Resulting protection flags</returns>
ULONG BBConvertProtection( IN ULONG prot, IN BOOLEAN fromPTE );