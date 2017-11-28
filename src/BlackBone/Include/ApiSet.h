#pragma once

#include "Winheaders.h"

//
// Api schema structures
//   

//
// Win 10
//
typedef struct _API_SET_VALUE_ENTRY_10
{
    ULONG Flags;
    ULONG NameOffset;
    ULONG NameLength;
    ULONG ValueOffset;
    ULONG ValueLength;
} API_SET_VALUE_ENTRY_10, *PAPI_SET_VALUE_ENTRY_10;

typedef struct _API_SET_VALUE_ARRAY_10
{
    ULONG Flags;
    ULONG NameOffset;
    ULONG Unk;
    ULONG NameLength;
    ULONG DataOffset;
    ULONG Count;

    inline PAPI_SET_VALUE_ENTRY_10 entry( void* pApiSet, DWORD i )
    {
        return (PAPI_SET_VALUE_ENTRY_10)((BYTE*)pApiSet + DataOffset + i * sizeof( API_SET_VALUE_ENTRY_10 ));
    }
} API_SET_VALUE_ARRAY_10, *PAPI_SET_VALUE_ARRAY_10;

typedef struct _API_SET_NAMESPACE_ENTRY_10
{
    ULONG Limit;
    ULONG Size;
} API_SET_NAMESPACE_ENTRY_10, *PAPI_SET_NAMESPACE_ENTRY_10;

typedef struct _API_SET_NAMESPACE_ARRAY_10
{
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG Count;
    ULONG Start;
    ULONG End;
    ULONG Unk[2];

    inline PAPI_SET_NAMESPACE_ENTRY_10 entry( DWORD i )
    {
        return (PAPI_SET_NAMESPACE_ENTRY_10)((BYTE*)this + End + i * sizeof( API_SET_NAMESPACE_ENTRY_10 ));
    }

    inline PAPI_SET_VALUE_ARRAY_10 valArray( PAPI_SET_NAMESPACE_ENTRY_10 pEntry )
    {
        return (PAPI_SET_VALUE_ARRAY_10)((BYTE*)this + Start + sizeof( API_SET_VALUE_ARRAY_10 ) * pEntry->Size);
    }

    inline ULONG apiName( PAPI_SET_NAMESPACE_ENTRY_10 pEntry, wchar_t* output )
    {
        auto pArray = valArray( pEntry );
        memcpy( output, (char*)this + pArray->NameOffset, pArray->NameLength );
        return  pArray->NameLength;
    }
} API_SET_NAMESPACE_ARRAY_10, *PAPI_SET_NAMESPACE_ARRAY_10;




//
// Win 8.1
//
typedef struct _API_SET_VALUE_ENTRY
{
    ULONG Flags;
    ULONG NameOffset;
    ULONG NameLength;
    ULONG ValueOffset;
    ULONG ValueLength;
} API_SET_VALUE_ENTRY, *PAPI_SET_VALUE_ENTRY;

typedef struct _API_SET_VALUE_ARRAY
{
    ULONG Flags;
    ULONG Count;
    API_SET_VALUE_ENTRY Array[ANYSIZE_ARRAY];

    inline PAPI_SET_VALUE_ENTRY entry( void* /*pApiSet*/, DWORD i )
    {
        return Array + i;
    }
} API_SET_VALUE_ARRAY, *PAPI_SET_VALUE_ARRAY;

typedef struct _API_SET_NAMESPACE_ENTRY
{
    ULONG Flags;
    ULONG NameOffset;
    ULONG NameLength;
    ULONG AliasOffset;
    ULONG AliasLength;
    ULONG DataOffset;
} API_SET_NAMESPACE_ENTRY, *PAPI_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_NAMESPACE_ARRAY
{
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG Count;
    API_SET_NAMESPACE_ENTRY Array[ANYSIZE_ARRAY];

    inline PAPI_SET_NAMESPACE_ENTRY entry( DWORD i )
    {
        return Array + i;
    }

    inline PAPI_SET_VALUE_ARRAY valArray( PAPI_SET_NAMESPACE_ENTRY pEntry )
    {
        return (PAPI_SET_VALUE_ARRAY)((BYTE*)this + pEntry->DataOffset);
    }

    inline ULONG apiName( PAPI_SET_NAMESPACE_ENTRY pEntry, wchar_t* output )
    {
        memcpy( output, (char*)this + pEntry->NameOffset, pEntry->NameLength );
        return pEntry->NameLength;
    }
} API_SET_NAMESPACE_ARRAY, *PAPI_SET_NAMESPACE_ARRAY;


//
// Win 8 and 7
//
typedef struct _API_SET_VALUE_ENTRY_V2
{
    ULONG NameOffset;
    ULONG NameLength;
    ULONG ValueOffset;
    ULONG ValueLength;
} API_SET_VALUE_ENTRY_V2, *PAPI_SET_VALUE_ENTRY_V2;

typedef struct _API_SET_VALUE_ARRAY_V2
{
    ULONG Count;
    API_SET_VALUE_ENTRY_V2 Array[ANYSIZE_ARRAY];

    inline PAPI_SET_VALUE_ENTRY_V2 entry( void* /*pApiSet*/, DWORD i )
    {
        return Array + i;
    }
} API_SET_VALUE_ARRAY_V2, *PAPI_SET_VALUE_ARRAY_V2;

typedef struct _API_SET_NAMESPACE_ENTRY_V2
{
    ULONG NameOffset;
    ULONG NameLength;
    ULONG DataOffset;   // API_SET_VALUE_ARRAY
} API_SET_NAMESPACE_ENTRY_V2, *PAPI_SET_NAMESPACE_ENTRY_V2;

typedef struct _API_SET_NAMESPACE_ARRAY_V2
{
    ULONG Version;
    ULONG Count;
    API_SET_NAMESPACE_ENTRY_V2 Array[ANYSIZE_ARRAY];

    inline PAPI_SET_NAMESPACE_ENTRY_V2 entry( DWORD i )
    {
        return Array + i;
    }

    inline PAPI_SET_VALUE_ARRAY_V2 valArray( PAPI_SET_NAMESPACE_ENTRY_V2 pEntry )
    {
        return (PAPI_SET_VALUE_ARRAY_V2)((BYTE*)this + pEntry->DataOffset);
    }

    inline ULONG apiName( PAPI_SET_NAMESPACE_ENTRY_V2 pEntry, wchar_t* output )
    {
        memcpy( output, (char*)this + pEntry->NameOffset, pEntry->NameLength );
        return pEntry->NameLength;
    }
} API_SET_NAMESPACE_ARRAY_V2, *PAPI_SET_NAMESPACE_ARRAY_V2;