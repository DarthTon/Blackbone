// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_ZONE_H
#define _ASMJIT_BASE_ZONE_H

// [Dependencies]
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base_util
//! \{

// ============================================================================
// [asmjit::Zone]
// ============================================================================

//! Zone memory allocator.
//!
//! Zone is an incremental memory allocator that allocates memory by simply
//! incrementing a pointer. It allocates blocks of memory by using standard
//! C library `malloc/free`, but divides these blocks into smaller segments
//! requirested by calling `Zone::alloc()` and friends.
//!
//! Zone memory allocators are designed to allocate data of short lifetime. The
//! data used by `Assembler` and `Compiler` has a very short lifetime, thus, is
//! allocated by `Zone`. The advantage is that `Zone` can free all of the data
//! allocated at once by calling `reset()` or by `Zone` destructor.
struct Zone {
  // --------------------------------------------------------------------------
  // [Block]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! A single block of memory.
  struct Block {
    // ------------------------------------------------------------------------
    // [Accessors]
    // ------------------------------------------------------------------------

    //! Get the size of the block.
    ASMJIT_INLINE size_t getBlockSize() const {
      return (size_t)(end - data);
    }

    //! Get count of remaining bytes in the block.
    ASMJIT_INLINE size_t getRemainingSize() const {
      return (size_t)(end - pos);
    }

    // ------------------------------------------------------------------------
    // [Members]
    // ------------------------------------------------------------------------

    //! Current data pointer (pointer to the first available byte).
    uint8_t* pos;
    //! End data pointer (pointer to the first invalid byte).
    uint8_t* end;

    //! Link to the previous block.
    Block* prev;
    //! Link to the next block.
    Block* next;

    //! Data.
    uint8_t data[sizeof(void*)];
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new instance of `Zone` allocator.
  //!
  //! The `blockSize` parameter describes the default size of the block. If the
  //! `size` parameter passed to `alloc()` is greater than the default size
  //! `Zone` will allocate and use a larger block, but it will not change the
  //! default `blockSize`.
  //!
  //! It's not required, but it's good practice to set `blockSize` to a
  //! reasonable value that depends on the usage of `Zone`. Greater block sizes
  //! are generally safer and performs better than unreasonably low values.
  ASMJIT_API Zone(size_t blockSize);

  //! Destroy the `Zone` instance.
  //!
  //! This will destroy the `Zone` instance and release all blocks of memory
  //! allocated by it. It performs implicit `reset(true)`.
  ASMJIT_API ~Zone();

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the `Zone` invalidating all blocks allocated.
  //!
  //! If `releaseMemory` is true all buffers will be released to the system.
  ASMJIT_API void reset(bool releaseMemory = false);

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the default block size.
  ASMJIT_INLINE size_t getBlockSize() const {
    return _blockSize;
  }

  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  //! Allocate `size` bytes of memory.
  //!
  //! Pointer returned is valid until the `Zone` instance is destroyed or reset
  //! by calling `reset()`. If you plan to make an instance of C++ from the
  //! given pointer use placement `new` and `delete` operators:
  //!
  //! ~~~
  //! using namespace asmjit;
  //!
  //! class SomeObject { ... };
  //!
  //! // Create Zone with default block size of 65536 bytes.
  //! Zone zone(65536);
  //!
  //! // Create your objects using zone object allocating, for example:
  //! Object* obj = static_cast<Object*>( zone.alloc(sizeof(SomeClass)) );
  //
  //! if (obj == NULL) {
  //!   // Handle out of memory error.
  //! }
  //!
  //! // To instantiate class placement `new` and `delete` operators can be used.
  //! new(obj) Object();
  //!
  //! // ... lifetime of your objects ...
  //!
  //! // To destroy the instance (if required).
  //! obj->~Object();
  //!
  //! // Reset of destroy `Zone`.
  //! zone.reset();
  //! ~~~
  ASMJIT_INLINE void* alloc(size_t size) {
    Block* cur = _block;

    uint8_t* ptr = cur->pos;
    size_t remainingBytes = (size_t)(cur->end - ptr);

    if (remainingBytes < size)
      return _alloc(size);

    cur->pos += size;
    ASMJIT_ASSERT(cur->pos <= cur->end);

    return (void*)ptr;
  }

  //! Allocate `size` bytes of zeroed memory.
  //!
  //! See \ref alloc() for more details.
  ASMJIT_API void* allocZeroed(size_t size);

  //! Like `alloc()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocT(size_t size = sizeof(T)) {
    return static_cast<T*>(alloc(size));
  }

  //! Like `allocZeroed()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocZeroedT(size_t size = sizeof(T)) {
    return static_cast<T*>(allocZeroed(size));
  }

  //! \internal
  ASMJIT_API void* _alloc(size_t size);

  //! Helper to duplicate data.
  ASMJIT_API void* dup(const void* data, size_t size);

  //! Helper to duplicate string.
  ASMJIT_API char* sdup(const char* str);

  //! Helper to duplicate formatted string, maximum length is 256 bytes.
  ASMJIT_API char* sformat(const char* str, ...);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! The current block.
  Block* _block;
  //! Default block size.
  size_t _blockSize;
};

enum {
  //! Zone allocator overhead.
  kZoneOverhead = static_cast<int>(sizeof(Zone::Block) - sizeof(void*)) + kMemAllocOverhead
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_ZONE_H
