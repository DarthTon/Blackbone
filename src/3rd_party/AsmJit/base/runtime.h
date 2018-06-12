// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_RUNTIME_H
#define _ASMJIT_BASE_RUNTIME_H

// [Dependencies]
#include "../base/codeholder.h"
#include "../base/vmem.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

class CodeHolder;

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::Runtime]
// ============================================================================

//! Base runtime.
class ASMJIT_VIRTAPI Runtime {
public:
  ASMJIT_NONCOPYABLE(Runtime)

  ASMJIT_ENUM(RuntimeType) {
    kRuntimeNone = 0,
    kRuntimeJit = 1,
    kRuntimeRemote = 2
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `Runtime` instance.
  ASMJIT_API Runtime() noexcept;
  //! Destroy the `Runtime` instance.
  ASMJIT_API virtual ~Runtime() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get CodeInfo of this runtime.
  //!
  //! CodeInfo can be used to setup a CodeHolder in case you plan to generate a
  //! code compatible and executable by this Runtime.
  ASMJIT_INLINE const CodeInfo& getCodeInfo() const noexcept { return _codeInfo; }

  //! Get the Runtime's architecture type, see \ref ArchInfo::Type.
  ASMJIT_INLINE uint32_t getArchType() const noexcept { return _codeInfo.getArchType(); }
  //! Get the Runtime's architecture sub-type, see \ref ArchInfo::SubType.
  ASMJIT_INLINE uint32_t getArchSubType() const noexcept { return _codeInfo.getArchSubType(); }

  //! Get the runtime type, see \ref Type.
  ASMJIT_INLINE uint32_t getRuntimeType() const noexcept { return _runtimeType; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  // NOTE: To allow passing function pointers to `add()` and `release()` the
  // virtual methods are prefixed with `_` and called from templates.

  template<typename Func>
  ASMJIT_INLINE Error add(Func* dst, CodeHolder* code) noexcept {
    return _add(Internal::ptr_cast<void**, Func*>(dst), code);
  }

  template<typename Func>
  ASMJIT_INLINE Error release(Func dst) noexcept {
    return _release(Internal::ptr_cast<void*, Func>(dst));
  }

  //! Allocate a memory needed for a code stored in the \ref CodeHolder and
  //! relocate it to the target location.
  //!
  //! The beginning of the memory allocated for the function is returned in
  //! `dst`. If failed the \ref Error code is returned and `dst` is set to null
  //! (this means that you don't have to set it to null before calling `add()`).
  virtual Error _add(void** dst, CodeHolder* code) noexcept = 0;

  //! Release `p` allocated by `add()`.
  virtual Error _release(void* p) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CodeInfo _codeInfo;                    //!< Basic information about the Runtime's code.
  uint8_t _runtimeType;                  //!< Type of the runtime.
  uint8_t _allocType;                    //!< Type of the allocator the Runtime uses.
  uint8_t _reserved[6];                  //!< \internal
};

// ============================================================================
// [asmjit::HostRuntime]
// ============================================================================

//! Runtime designed to be used in the same process the code is generated in.
class ASMJIT_VIRTAPI HostRuntime : public Runtime {
public:
  ASMJIT_NONCOPYABLE(HostRuntime)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `HostRuntime` instance.
  ASMJIT_API HostRuntime() noexcept;
  //! Destroy the `HostRuntime` instance.
  ASMJIT_API virtual ~HostRuntime() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Flush an instruction cache.
  //!
  //! This member function is called after the code has been copied to the
  //! destination buffer. It is only useful for JIT code generation as it
  //! causes a flush of the processor's cache.
  //!
  //! Flushing is basically a NOP under X86/X64, but is needed by architectures
  //! that do not have a transparent instruction cache like ARM.
  //!
  //! This function can also be overridden to improve compatibility with tools
  //! such as Valgrind, however, it's not an official part of AsmJit.
  ASMJIT_API virtual void flush(const void* p, size_t size) noexcept;
};

// ============================================================================
// [asmjit::JitRuntime]
// ============================================================================

//! Runtime designed to store and execute code generated at runtime (JIT).
class ASMJIT_VIRTAPI JitRuntime : public HostRuntime {
public:
  ASMJIT_NONCOPYABLE(JitRuntime)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `JitRuntime` instance.
  ASMJIT_API JitRuntime() noexcept;
  //! Destroy the `JitRuntime` instance.
  ASMJIT_API virtual ~JitRuntime() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the type of allocation.
  ASMJIT_INLINE uint32_t getAllocType() const noexcept { return _allocType; }
  //! Set the type of allocation.
  ASMJIT_INLINE void setAllocType(uint32_t allocType) noexcept { _allocType = allocType; }

  //! Get the virtual memory manager.
  ASMJIT_INLINE VMemMgr* getMemMgr() const noexcept { return const_cast<VMemMgr*>(&_memMgr); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _add(void** dst, CodeHolder* code) noexcept override;
  ASMJIT_API Error _release(void* p) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Virtual memory manager.
  VMemMgr _memMgr;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_RUNTIME_H
