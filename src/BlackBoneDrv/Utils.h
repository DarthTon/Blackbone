#pragma once
#include <wdm.h>

/// <summary>
/// Get file name from full path
/// </summary>
/// <param name="path">Path.</param>
/// <param name="name">Resultingf name</param>
/// <returns>Status code</returns>
NTSTATUS StripPath( IN PUNICODE_STRING path, OUT PUNICODE_STRING name );