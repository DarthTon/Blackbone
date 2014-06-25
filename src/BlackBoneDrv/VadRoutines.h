#pragma once

#include "Private.h"

/// <summary>
/// Change VAD protection flags
/// </summary>
/// <param name="pProcess">Process object</param>
/// <param name="address">target address</param>
/// <param name="prot">New protection flags</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectVAD( IN PEPROCESS pProcess, IN ULONG_PTR address, IN ULONG prot );

/// <summary>
/// Hide memory from NtQueryVirtualMemory
/// </summary>
/// <param name="pProcess">Process object</param>
/// <param name="address">Target address</param>
/// <returns>Status code</returns>
NTSTATUS BBHideVAD( IN PEPROCESS pProcess, IN ULONG_PTR address );