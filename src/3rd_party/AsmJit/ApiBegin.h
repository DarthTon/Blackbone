// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#if !defined(_ASMJIT_BUILD_H)
#include "build.h"
#endif // !_ASMJIT_BUILD_H

// ============================================================================
// [MSVC]
// ============================================================================

#if defined(_MSC_VER)
// Disable some warnings we know about
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
# pragma warning(disable: 4201) // nameless struct/union
# pragma warning(disable: 4244) // '+=' : conversion from 'int' to 'x', possible
                                // loss of data
# pragma warning(disable: 4251) // struct needs to have dll-interface to be used
                                // by clients of struct ...
# pragma warning(disable: 4275) // non dll-interface struct ... used as base for
                                // dll-interface struct
# pragma warning(disable: 4355) // this used in base member initializer list
# pragma warning(disable: 4480) // specifying underlying type for enum
# pragma warning(disable: 4800) // forcing value to bool 'true' or 'false'

// Rename symbols.
# if !defined(vsnprintf)
#  define ASMJIT_DEFINED_VSNPRINTF
#  define vsnprintf _vsnprintf
# endif // !vsnprintf
# if !defined(snprintf)
#  define ASMJIT_DEFINED_SNPRINTF
#  define snprintf _snprintf
# endif // !snprintf
#endif // _MSC_VER

// ============================================================================
// [GNUC]
// ============================================================================

#if defined(__GNUC__) && !defined(__clang__)
# if __GNUC__ >= 4 && !defined(__MINGW32__)
#  pragma GCC visibility push(hidden)
# endif // __GNUC__ >= 4
#endif // __GNUC__
