// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_LOCK_H
#define _ASMJIT_BASE_LOCK_H

// [Dependencies - AsmJit]
#include "../build.h"

// [Dependencies - Posix]
#if defined(ASMJIT_OS_POSIX)
# include <pthread.h>
#endif // ASMJIT_OS_POSIX

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_util
//! \{

// ============================================================================
// [asmjit::Lock]
// ============================================================================

//! Lock - used in thread-safe code for locking.
struct Lock {
  ASMJIT_NO_COPY(Lock)

  // --------------------------------------------------------------------------
  // [Windows]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_OS_WINDOWS)
  typedef CRITICAL_SECTION Handle;

  //! Create a new `Lock` instance.
  ASMJIT_INLINE Lock() { InitializeCriticalSection(&_handle); }
  //! Destroy the `Lock` instance.
  ASMJIT_INLINE ~Lock() { DeleteCriticalSection(&_handle); }

  //! Lock.
  ASMJIT_INLINE void lock() { EnterCriticalSection(&_handle); }
  //! Unlock.
  ASMJIT_INLINE void unlock() { LeaveCriticalSection(&_handle); }

#endif // ASMJIT_OS_WINDOWS

  // --------------------------------------------------------------------------
  // [Posix]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_OS_POSIX)
  typedef pthread_mutex_t Handle;

  //! Create a new `Lock` instance.
  ASMJIT_INLINE Lock() { pthread_mutex_init(&_handle, NULL); }
  //! Destroy the `Lock` instance.
  ASMJIT_INLINE ~Lock() { pthread_mutex_destroy(&_handle); }

  //! Lock.
  ASMJIT_INLINE void lock() { pthread_mutex_lock(&_handle); }
  //! Unlock.
  ASMJIT_INLINE void unlock() { pthread_mutex_unlock(&_handle); }
#endif // ASMJIT_OS_POSIX

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get handle.
  ASMJIT_INLINE Handle& getHandle() {
    return _handle;
  }
  //! \overload
  ASMJIT_INLINE const Handle& getHandle() const {
    return _handle;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Handle.
  Handle _handle;
};

// ============================================================================
// [asmjit::AutoLock]
// ============================================================================

//! Scoped lock.
struct AutoLock {
  ASMJIT_NO_COPY(AutoLock)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Autolock `target`, scoped.
  ASMJIT_INLINE AutoLock(Lock& target) : _target(target) {
    _target.lock();
  }

  //! Autounlock `target`.
  ASMJIT_INLINE ~AutoLock() {
    _target.unlock();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Pointer to target (lock).
  Lock& _target;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_LOCK_H
