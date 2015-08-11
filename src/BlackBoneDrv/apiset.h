/*++

Copyright (c) 2008  Microsoft Corporation

Module Name:

    apiset.h

Abstract:

    This module contains definitions related to the management of API namespaces.

Author:

    Arun Kishan (arunki) 14-Sep-2008

--*/

#ifndef _API_SET_H_
#define _API_SET_H_

//
// API schema DLL information.
//

#define API_SET_PREFIX_NAME_A           "API-"
#define API_SET_PREFIX_NAME_A_SIZE      (sizeof(API_SET_PREFIX_NAME_A) - sizeof(CHAR))
#define API_SET_PREFIX_NAME_U           TEXT(API_SET_PREFIX_NAME_A)
#define API_SET_PREFIX_NAME_U_SIZE      (sizeof(API_SET_PREFIX_NAME_U) - sizeof(WCHAR))
#define API_SET_PREFIX_NAME_LENGTH      (RTL_NUMBER_OF(API_SET_PREFIX_NAME_U) - 1)

#define API_SET_EXTENSION_NAME_A        "EXT-"
#define API_SET_EXTENSION_NAME_A_SIZE   (sizeof(API_SET_EXTENSION_NAME_A) - sizeof(CHAR))
#define API_SET_EXTENSION_NAME_U        TEXT(API_SET_EXTENSION_NAME_A)
#define API_SET_EXTENSION_NAME_U_SIZE   (sizeof(API_SET_EXTENSION_NAME_U) - sizeof(WCHAR))
#define API_SET_EXTENSION_NAME_LENGTH   (RTL_NUMBER_OF(API_SET_EXTENSION_NAME_U) - 1)

#define API_SET_SCHEMA_NAME             ApiSetSchema
#define API_SET_SECTION_NAME            ".apiset"
#define API_SET_SCHEMA_SUFFIX           L".sys"

//
// All previous schema versions are listed here. Some are supported in offline
// mode because they shipped externally. Some are not supported in offline mode
// since they were only ever released internally.
//

#define API_SET_SCHEMA_VERSION_V2       0x00000002UL
#define API_SET_SCHEMA_VERSION_V3       0x00000003UL // No offline support.
#define API_SET_SCHEMA_VERSION_V4       0x00000004UL
#define API_SET_SCHEMA_VERSION          API_SET_SCHEMA_VERSION_V4

#define API_SET_HELPER_NAME             ApiSetHelp

#define API_SET_LOAD_SCHEMA_ORDINAL     1
#define API_SET_LOOKUP_ORDINAL          2
#define API_SET_RELEASE_SCHEMA_ORDINAL  3

#define API_SET_STRING_X(s)             #s
#define API_SET_STRING(s)               API_SET_STRING_X(s)
#define API_SET_STRING_U_Y(s)           L ## s
#define API_SET_STRING_U_X(s)           API_SET_STRING_U_Y(s)
#define API_SET_STRING_U(s)             API_SET_STRING_U_X(API_SET_STRING(s))

//
// API set interface definitions.
//

#define API_SET_OVERRIDE(X)             X##Implementation
#define API_SET_LEGACY_OVERRIDE_DEF(X)  X = API_SET_OVERRIDE(X)
#define API_SET_OVERRIDE_DEF(X)         API_SET_LEGACY_OVERRIDE_DEF(X) PRIVATE

#ifdef _API_SET_HOST

#undef API_SET_LIBRARY

#define API_SET_LIBRARY(X)

#undef API_SET
#undef API_SET_PRIVATE
#undef API_SET_BY_ORDINAL
#undef API_SET_BY_ORDINAL_PRIVATE

#define API_SET(X)                         X PRIVATE
#define API_SET_PRIVATE(X)                 X PRIVATE
#define API_SET_BY_ORDINAL(X,O,PO)         X @##O NONAME PRIVATE
#define API_SET_BY_ORDINAL_PRIVATE(X,O,PO) X @##O NONAME PRIVATE

#else

#ifndef _API_SET_LEGACY_TARGET

#define API_SET(X)                         X
#define API_SET_PRIVATE(X)                 X PRIVATE
#define API_SET_BY_ORDINAL(X,O,PO)         X @##O NONAME
#define API_SET_BY_ORDINAL_PRIVATE(X,O,PO) X @##O NONAME PRIVATE

#else

#define API_SET(X)                         X = _API_SET_LEGACY_TARGET##.##X
#define API_SET_PRIVATE(X)                 X = _API_SET_LEGACY_TARGET##.##X PRIVATE
#define API_SET_BY_ORDINAL(X,O,PO)         X = _API_SET_LEGACY_TARGET##.##PO @##O NONAME
#define API_SET_BY_ORDINAL_PRIVATE(X,O,PO) X = _API_SET_LEGACY_TARGET##.##PO @##O NONAME PRIVATE

#endif // _API_SET_LEGACY_TARGET

#define API_SET_LIBRARY(X)              LIBRARY X

#endif // _API_SET_HOST

#ifdef _NTDEF_

//
// Data structures.
//

typedef struct _API_SET_VALUE_ENTRY {
    ULONG Flags;
    ULONG NameOffset;
    _Field_range_(0, UNICODE_STRING_MAX_BYTES) ULONG NameLength;
    ULONG ValueOffset;
    _Field_range_(0, UNICODE_STRING_MAX_BYTES) ULONG ValueLength;
} API_SET_VALUE_ENTRY, *PAPI_SET_VALUE_ENTRY;

_Struct_size_bytes_(FIELD_OFFSET(API_SET_VALUE_ARRAY, Array) + (sizeof(API_SET_VALUE_ENTRY) * Count))
typedef struct _API_SET_VALUE_ARRAY {
    ULONG Flags;
    ULONG Count;
    _Field_size_full_(Count) API_SET_VALUE_ENTRY Array[ANYSIZE_ARRAY];
} API_SET_VALUE_ARRAY,*PAPI_SET_VALUE_ARRAY;

typedef struct _API_SET_NAMESPACE_ENTRY {
    ULONG Flags;
    ULONG NameOffset;
    _Field_range_(0, UNICODE_STRING_MAX_BYTES) ULONG NameLength;
    ULONG AliasOffset;
    _Field_range_(0, UNICODE_STRING_MAX_BYTES) ULONG AliasLength;
    ULONG DataOffset;   // API_SET_VALUE_ARRAY
} API_SET_NAMESPACE_ENTRY, *PAPI_SET_NAMESPACE_ENTRY;

_Struct_size_bytes_(Size)
typedef struct _API_SET_NAMESPACE_ARRAY {
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG Count;
    _Field_size_full_(Count) API_SET_NAMESPACE_ENTRY Array[ANYSIZE_ARRAY];
} API_SET_NAMESPACE_ARRAY, *PAPI_SET_NAMESPACE_ARRAY;

typedef const API_SET_VALUE_ENTRY *PCAPI_SET_VALUE_ENTRY;
typedef const API_SET_VALUE_ARRAY *PCAPI_SET_VALUE_ARRAY;
typedef const API_SET_NAMESPACE_ENTRY *PCAPI_SET_NAMESPACE_ENTRY;
typedef const API_SET_NAMESPACE_ARRAY *PCAPI_SET_NAMESPACE_ARRAY;

#define API_SET_SCHEMA_FLAGS_SEALED              0x00000001UL
#define API_SET_SCHEMA_FLAGS_HOST_EXTENSION      0x00000002UL

#define API_SET_SCHEMA_ENTRY_FLAGS_SEALED        0x00000001UL
#define API_SET_SCHEMA_ENTRY_FLAGS_EXTENSION     0x00000002UL

//
// Support for downlevel API set schema versions.
//

typedef struct _API_SET_VALUE_ENTRY_V2 {
    ULONG NameOffset;
    ULONG NameLength;
    ULONG ValueOffset;
    ULONG ValueLength;
} API_SET_VALUE_ENTRY_V2, *PAPI_SET_VALUE_ENTRY_V2;

typedef struct _API_SET_VALUE_ARRAY_V2 {
    ULONG Count;
    API_SET_VALUE_ENTRY_V2 Array[ANYSIZE_ARRAY];
} API_SET_VALUE_ARRAY_V2,*PAPI_SET_VALUE_ARRAY_V2;

typedef struct _API_SET_NAMESPACE_ENTRY_V2 {
    ULONG NameOffset;
    ULONG NameLength;
    ULONG DataOffset;   // API_SET_VALUE_ARRAY
} API_SET_NAMESPACE_ENTRY_V2, *PAPI_SET_NAMESPACE_ENTRY_V2;

typedef struct _API_SET_NAMESPACE_ARRAY_V2 {
    ULONG Version;
    ULONG Count;
    _Field_size_full_(Count) API_SET_NAMESPACE_ENTRY_V2 Array[ANYSIZE_ARRAY];
} API_SET_NAMESPACE_ARRAY_V2, *PAPI_SET_NAMESPACE_ARRAY_V2;

typedef const API_SET_VALUE_ENTRY_V2 *PCAPI_SET_VALUE_ENTRY_V2;
typedef const API_SET_VALUE_ARRAY_V2 *PCAPI_SET_VALUE_ARRAY_V2;
typedef const API_SET_NAMESPACE_ENTRY *PCAPI_SET_NAMESPACE_ENTRY_V2;
typedef const API_SET_NAMESPACE_ARRAY *PCAPI_SET_NAMESPACE_ARRAY_V2;

//
// Offline API set resolution support.
//

typedef
_Function_class_(API_SET_LOAD_SCHEMA_RTN)
NTSTATUS
(NTAPI API_SET_LOAD_SCHEMA_RTN) (
    _In_ PCSTR SchemaPath,
    _Outptr_ PCAPI_SET_NAMESPACE_ARRAY *Schema,
    _Outptr_ PVOID *Context
    );

typedef API_SET_LOAD_SCHEMA_RTN *PAPI_SET_LOAD_SCHEMA_RTN;

typedef
_Function_class_(API_SET_LOOKUP_HELPER_RTN)
NTSTATUS
(NTAPI API_SET_LOOKUP_HELPER_RTN) (
    _In_ PCAPI_SET_NAMESPACE_ARRAY Schema,
    _In_ PCSTR FileName,
    _In_ PCSTR ParentName,
    _Out_ PBOOLEAN Resolved,
    _Out_writes_bytes_(Length) PSTR ResolvedName,
    _In_ ULONG Length
    );

typedef API_SET_LOOKUP_HELPER_RTN *PAPI_SET_LOOKUP_HELPER_RTN;

typedef
_Function_class_(API_SET_RELEASE_SCHEMA_RTN)
NTSTATUS
(NTAPI API_SET_RELEASE_SCHEMA_RTN) (
    _In_ PVOID Context
    );

typedef API_SET_RELEASE_SCHEMA_RTN *PAPI_SET_RELEASE_SCHEMA_RTN;

//
// Prototypes
//

NTSTATUS
ApiSetResolveToHost (
    _In_ PCAPI_SET_NAMESPACE_ARRAY Schema,
    _In_ PCUNICODE_STRING FileNameIn,
    _In_opt_ PCUNICODE_STRING ParentName,
    _Out_ PBOOLEAN Resolved,
    _Out_ PUNICODE_STRING HostBinary
    );

#endif // _NTDEF_

#endif // _API_SET_H_
