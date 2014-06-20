// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_PODLIST_H
#define _ASMJIT_BASE_PODLIST_H

// [Dependencies - AsmJit]
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_util
//! \{

// ============================================================================
// [asmjit::PodList<T>]
// ============================================================================

//! \internal
template <typename T>
struct PodList {
  ASMJIT_NO_COPY(PodList<T>)

  // --------------------------------------------------------------------------
  // [Link]
  // --------------------------------------------------------------------------

  struct Link {
    // --------------------------------------------------------------------------
    // [Accessors]
    // --------------------------------------------------------------------------

    //! Get next node.
    ASMJIT_INLINE Link* getNext() const { return _next; }

    //! Get value.
    ASMJIT_INLINE T getValue() const { return _value; }
    //! Set value to `value`.
    ASMJIT_INLINE void setValue(const T& value) { _value = value; }

    // --------------------------------------------------------------------------
    // [Members]
    // --------------------------------------------------------------------------

    Link* _next;
    T _value;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE PodList() : _first(NULL), _last(NULL) {}
  ASMJIT_INLINE ~PodList() {}

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isEmpty() const { return _first != NULL; }

  ASMJIT_INLINE Link* getFirst() const { return _first; }
  ASMJIT_INLINE Link* getLast() const { return _last; }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void clear() {
    reset();
  }

  ASMJIT_INLINE void reset() {
    _first = NULL;
    _last = NULL;
  }

  ASMJIT_INLINE void prepend(Link* link) {
    link->_next = _first;
    if (_first == NULL)
      _last = link;
    _first = link;
  }

  ASMJIT_INLINE void append(Link* link) {
    link->_next = NULL;
    if (_first == NULL)
      _first = link;
    else
      _last->_next = link;
    _last = link;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Link* _first;
  Link* _last;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_PODLIST_H
