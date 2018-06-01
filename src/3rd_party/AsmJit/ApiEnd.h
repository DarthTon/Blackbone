// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// ============================================================================
// [MSVC]
// ============================================================================

#if defined(_MSC_VER)
// Pop disabled warnings by ApiBegin.h
# pragma warning(pop)
// Rename symbols back.
# if defined(ASMJIT_DEFINED_VSNPRINTF)
#  undef ASMJIT_DEFINED_VSNPRINTF
#  undef vsnprintf
# endif // ASMJIT_DEFINED_VSNPRINTF
# if defined(ASMJIT_DEFINED_SNPRINTF)
#  undef ASMJIT_DEFINED_SNPRINTF
#  undef snprintf
# endif // ASMJIT_DEFINED_SNPRINTF
#endif // _MSC_VER

// ============================================================================
// [GNUC]
// ============================================================================

#if defined(__GNUC__) && !defined(__clang__)
# if __GNUC__ >= 4 && !defined(__MINGW32__)
#  pragma GCC visibility pop
# endif // __GNUC__ >= 4
#endif // __GNUC__
