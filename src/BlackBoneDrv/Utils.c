#include "Utils.h"
#include <ntstrsafe.h>

#pragma alloc_text(PAGE, StripPath)

/// <summary>
/// Get file name from full path
/// </summary>
/// <param name="path">Path.</param>
/// <param name="name">Resultingf name</param>
/// <returns>Status code</returns>
NTSTATUS StripPath( IN PUNICODE_STRING path, OUT PUNICODE_STRING name )
{
    ASSERT( path != NULL && name );
    if (path == NULL || name == NULL)
        return STATUS_INVALID_PARAMETER;

    for (USHORT i = (path->Length / sizeof( WCHAR )) - 1; i != 0; i--)
    {
        if (path->Buffer[i] == L'\\' || path->Buffer[i] == L'/')
        {
            RtlUnicodeStringInit( name, &path->Buffer[i + 1] );
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}