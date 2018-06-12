// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86EMITTER_H
#define _ASMJIT_X86_X86EMITTER_H

// [Dependencies]
#include "../base/codeemitter.h"
#include "../x86/x86inst.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86EmitterExplicitT]
// ============================================================================

#define ASMJIT_EMIT static_cast<This*>(this)->emit

#define ASMJIT_INST_0x(NAME, ID) \
  ASMJIT_INLINE Error NAME() { return ASMJIT_EMIT(X86Inst::kId##ID); }

#define ASMJIT_INST_1x(NAME, ID, T0) \
  ASMJIT_INLINE Error NAME(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID, o0); }

#define ASMJIT_INST_1i(NAME, ID, T0) \
  ASMJIT_INLINE Error NAME(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID, o0); } \
  ASMJIT_INLINE Error NAME(int o0) { return ASMJIT_EMIT(X86Inst::kId##ID, Utils::asInt(o0)); } \
  ASMJIT_INLINE Error NAME(unsigned int o0) { return ASMJIT_EMIT(X86Inst::kId##ID, Utils::asInt(o0)); } \
  ASMJIT_INLINE Error NAME(int64_t o0) { return ASMJIT_EMIT(X86Inst::kId##ID, Utils::asInt(o0)); } \
  ASMJIT_INLINE Error NAME(uint64_t o0) { return ASMJIT_EMIT(X86Inst::kId##ID, Utils::asInt(o0)); }

#define ASMJIT_INST_1c(NAME, ID, CONV, T0) \
  ASMJIT_INLINE Error NAME(uint32_t cc, const T0& o0) { return ASMJIT_EMIT(CONV(cc), o0); } \
  ASMJIT_INLINE Error NAME##a(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##a, o0); } \
  ASMJIT_INLINE Error NAME##ae(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##ae, o0); } \
  ASMJIT_INLINE Error NAME##b(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##b, o0); } \
  ASMJIT_INLINE Error NAME##be(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##be, o0); } \
  ASMJIT_INLINE Error NAME##c(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##c, o0); } \
  ASMJIT_INLINE Error NAME##e(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##e, o0); } \
  ASMJIT_INLINE Error NAME##g(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##g, o0); } \
  ASMJIT_INLINE Error NAME##ge(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##ge, o0); } \
  ASMJIT_INLINE Error NAME##l(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##l, o0); } \
  ASMJIT_INLINE Error NAME##le(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##le, o0); } \
  ASMJIT_INLINE Error NAME##na(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##na, o0); } \
  ASMJIT_INLINE Error NAME##nae(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nae, o0); } \
  ASMJIT_INLINE Error NAME##nb(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nb, o0); } \
  ASMJIT_INLINE Error NAME##nbe(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nbe, o0); } \
  ASMJIT_INLINE Error NAME##nc(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nc, o0); } \
  ASMJIT_INLINE Error NAME##ne(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##ne, o0); } \
  ASMJIT_INLINE Error NAME##ng(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##ng, o0); } \
  ASMJIT_INLINE Error NAME##nge(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nge, o0); } \
  ASMJIT_INLINE Error NAME##nl(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nl, o0); } \
  ASMJIT_INLINE Error NAME##nle(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nle, o0); } \
  ASMJIT_INLINE Error NAME##no(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##no, o0); } \
  ASMJIT_INLINE Error NAME##np(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##np, o0); } \
  ASMJIT_INLINE Error NAME##ns(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##ns, o0); } \
  ASMJIT_INLINE Error NAME##nz(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##nz, o0); } \
  ASMJIT_INLINE Error NAME##o(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##o, o0); } \
  ASMJIT_INLINE Error NAME##p(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##p, o0); } \
  ASMJIT_INLINE Error NAME##pe(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##pe, o0); } \
  ASMJIT_INLINE Error NAME##po(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##po, o0); } \
  ASMJIT_INLINE Error NAME##s(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##s, o0); } \
  ASMJIT_INLINE Error NAME##z(const T0& o0) { return ASMJIT_EMIT(X86Inst::kId##ID##z, o0); }

#define ASMJIT_INST_2x(NAME, ID, T0, T1) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1) { \
    return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1); \
  }

#define ASMJIT_INST_2i(NAME, ID, T0, T1) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1); } \
  ASMJIT_INLINE Error NAME(const T0& o0, int o1) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, Utils::asInt(o1)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, unsigned int o1) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, Utils::asInt(o1)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, int64_t o1) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, Utils::asInt(o1)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, uint64_t o1) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, Utils::asInt(o1)); }

#define ASMJIT_INST_2c(NAME, ID, CONV, T0, T1) \
  ASMJIT_INLINE Error NAME(uint32_t cc, const T0& o0, const T1& o1) { return ASMJIT_EMIT(CONV(cc), o0, o1); } \
  ASMJIT_INLINE Error NAME##a(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##a, o0, o1); } \
  ASMJIT_INLINE Error NAME##ae(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##ae, o0, o1); } \
  ASMJIT_INLINE Error NAME##b(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##b, o0, o1); } \
  ASMJIT_INLINE Error NAME##be(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##be, o0, o1); } \
  ASMJIT_INLINE Error NAME##c(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##c, o0, o1); } \
  ASMJIT_INLINE Error NAME##e(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##e, o0, o1); } \
  ASMJIT_INLINE Error NAME##g(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##g, o0, o1); } \
  ASMJIT_INLINE Error NAME##ge(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##ge, o0, o1); } \
  ASMJIT_INLINE Error NAME##l(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##l, o0, o1); } \
  ASMJIT_INLINE Error NAME##le(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##le, o0, o1); } \
  ASMJIT_INLINE Error NAME##na(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##na, o0, o1); } \
  ASMJIT_INLINE Error NAME##nae(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nae, o0, o1); } \
  ASMJIT_INLINE Error NAME##nb(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nb, o0, o1); } \
  ASMJIT_INLINE Error NAME##nbe(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nbe, o0, o1); } \
  ASMJIT_INLINE Error NAME##nc(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nc, o0, o1); } \
  ASMJIT_INLINE Error NAME##ne(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##ne, o0, o1); } \
  ASMJIT_INLINE Error NAME##ng(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##ng, o0, o1); } \
  ASMJIT_INLINE Error NAME##nge(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nge, o0, o1); } \
  ASMJIT_INLINE Error NAME##nl(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nl, o0, o1); } \
  ASMJIT_INLINE Error NAME##nle(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nle, o0, o1); } \
  ASMJIT_INLINE Error NAME##no(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##no, o0, o1); } \
  ASMJIT_INLINE Error NAME##np(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##np, o0, o1); } \
  ASMJIT_INLINE Error NAME##ns(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##ns, o0, o1); } \
  ASMJIT_INLINE Error NAME##nz(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##nz, o0, o1); } \
  ASMJIT_INLINE Error NAME##o(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##o, o0, o1); } \
  ASMJIT_INLINE Error NAME##p(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##p, o0, o1); } \
  ASMJIT_INLINE Error NAME##pe(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##pe, o0, o1); } \
  ASMJIT_INLINE Error NAME##po(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##po, o0, o1); } \
  ASMJIT_INLINE Error NAME##s(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##s, o0, o1); } \
  ASMJIT_INLINE Error NAME##z(const T0& o0, const T1& o1) { return ASMJIT_EMIT(X86Inst::kId##ID##z, o0, o1); }

#define ASMJIT_INST_3x(NAME, ID, T0, T1, T2) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2); }

#define ASMJIT_INST_3i(NAME, ID, T0, T1, T2) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, int o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, Utils::asInt(o2)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, unsigned int o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, Utils::asInt(o2)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, int64_t o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, Utils::asInt(o2)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, uint64_t o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, Utils::asInt(o2)); }

#define ASMJIT_INST_3ii(NAME, ID, T0, T1, T2) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2); } \
  ASMJIT_INLINE Error NAME(const T0& o0, int o1, int o2) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, Imm(o1), Utils::asInt(o2)); }

#define ASMJIT_INST_4x(NAME, ID, T0, T1, T2, T3) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3); }

#define ASMJIT_INST_4i(NAME, ID, T0, T1, T2, T3) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, int o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, Utils::asInt(o3)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, unsigned int o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, Utils::asInt(o3)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, int64_t o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, Utils::asInt(o3)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, uint64_t o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, Utils::asInt(o3)); }

#define ASMJIT_INST_4ii(NAME, ID, T0, T1, T2, T3) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, int o2, int o3) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, Imm(o2), Utils::asInt(o3)); }

#define ASMJIT_INST_5x(NAME, ID, T0, T1, T2, T3, T4) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, const T4& o4) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, o4); }

#define ASMJIT_INST_5i(NAME, ID, T0, T1, T2, T3, T4) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, const T4& o4) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, o4); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, int o4) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, Utils::asInt(o4)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, unsigned int o4) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, Utils::asInt(o4)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, int64_t o4) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, Utils::asInt(o4)); } \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, uint64_t o4) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, Utils::asInt(o4)); }

#define ASMJIT_INST_6x(NAME, ID, T0, T1, T2, T3, T4, T5) \
  ASMJIT_INLINE Error NAME(const T0& o0, const T1& o1, const T2& o2, const T3& o3, const T4& o4, const T5& o5) { return ASMJIT_EMIT(X86Inst::kId##ID, o0, o1, o2, o3, o4, o5); }

template<typename This>
struct X86EmitterExplicitT {
  // These typedefs are used to describe implicit operands passed explicitly.
  typedef X86Gp AL;
  typedef X86Gp AH;
  typedef X86Gp CL;
  typedef X86Gp AX;
  typedef X86Gp DX;

  typedef X86Gp EAX;
  typedef X86Gp EBX;
  typedef X86Gp ECX;
  typedef X86Gp EDX;

  typedef X86Gp RAX;
  typedef X86Gp RBX;
  typedef X86Gp RCX;
  typedef X86Gp RDX;

  typedef X86Gp ZAX;
  typedef X86Gp ZBX;
  typedef X86Gp ZCX;
  typedef X86Gp ZDX;

  typedef X86Mem DS_ZAX; // ds:[zax]
  typedef X86Mem DS_ZDI; // ds:[zdi]
  typedef X86Mem ES_ZDI; // es:[zdi]
  typedef X86Mem DS_ZSI; // ds:[zsi]

  typedef X86Xmm XMM0;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get GPD or GPQ register at index `id` depending on the current architecture.
  ASMJIT_INLINE X86Gp gpz(uint32_t id) const noexcept {
    return X86Gp(Init, static_cast<const This*>(this)->_nativeGpReg.getSignature(), id);
  }

  ASMJIT_INLINE const X86Gp& gpzRef(uint32_t id) const noexcept {
    ASMJIT_ASSERT(id < 16);
    return static_cast<const X86Gp&>(static_cast<const This*>(this)->_nativeGpArray[id]);
  }

  ASMJIT_INLINE const X86Gp& zax() const noexcept { return gpzRef(X86Gp::kIdAx); }
  ASMJIT_INLINE const X86Gp& zcx() const noexcept { return gpzRef(X86Gp::kIdCx); }
  ASMJIT_INLINE const X86Gp& zdx() const noexcept { return gpzRef(X86Gp::kIdDx); }
  ASMJIT_INLINE const X86Gp& zbx() const noexcept { return gpzRef(X86Gp::kIdBx); }
  ASMJIT_INLINE const X86Gp& zsp() const noexcept { return gpzRef(X86Gp::kIdSp); }
  ASMJIT_INLINE const X86Gp& zbp() const noexcept { return gpzRef(X86Gp::kIdBp); }
  ASMJIT_INLINE const X86Gp& zsi() const noexcept { return gpzRef(X86Gp::kIdSi); }
  ASMJIT_INLINE const X86Gp& zdi() const noexcept { return gpzRef(X86Gp::kIdDi); }

  //! Create a target dependent pointer of which base register's id is `baseId`.
  ASMJIT_INLINE X86Mem ptr_base(uint32_t baseId, int32_t off = 0, uint32_t size = 0) const noexcept {
    uint32_t baseType = static_cast<const This*>(this)->_nativeGpReg.getType();
    uint32_t flags = 0;
    return X86Mem(Init, baseType, baseId, 0, 0, off, size, flags);
  }

  ASMJIT_INLINE X86Mem ptr_zax(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdAx, off, size); }
  ASMJIT_INLINE X86Mem ptr_zcx(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdCx, off, size); }
  ASMJIT_INLINE X86Mem ptr_zdx(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdDx, off, size); }
  ASMJIT_INLINE X86Mem ptr_zbx(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdBx, off, size); }
  ASMJIT_INLINE X86Mem ptr_zsp(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdSp, off, size); }
  ASMJIT_INLINE X86Mem ptr_zbp(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdBp, off, size); }
  ASMJIT_INLINE X86Mem ptr_zsi(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdSi, off, size); }
  ASMJIT_INLINE X86Mem ptr_zdi(int32_t off = 0, uint32_t size = 0) const noexcept { return ptr_base(X86Gp::kIdDi, off, size); }

  //! Create an `intptr_t` memory operand depending on the current architecture.
  ASMJIT_INLINE X86Mem intptr_ptr(const X86Gp& base, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const X86Gp& base, const X86Gp& index, uint32_t shift = 0, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, index, shift, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const X86Gp& base, const X86Vec& index, uint32_t shift = 0, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, index, shift, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const Label& base, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const Label& base, const X86Gp& index, uint32_t shift, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, index, shift, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const Label& base, const X86Vec& index, uint32_t shift, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, index, shift, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(const X86Rip& rip, int32_t offset = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(rip, offset, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(uint64_t base) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr(uint64_t base, const X86Gp& index, uint32_t shift = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, index, shift, nativeGpSize);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr_abs(uint64_t base) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, nativeGpSize, Mem::kSignatureMemAbs);
  }
  //! \overload
  ASMJIT_INLINE X86Mem intptr_ptr_abs(uint64_t base, const X86Gp& index, uint32_t shift = 0) const noexcept {
    uint32_t nativeGpSize = static_cast<const This*>(this)->getGpSize();
    return X86Mem(base, index, shift, nativeGpSize, Mem::kSignatureMemAbs);
  }

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE Error db(uint8_t x) { return static_cast<This*>(this)->embed(&x, 1); }
  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dw(uint16_t x) { return static_cast<This*>(this)->embed(&x, 2); }
  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dd(uint32_t x) { return static_cast<This*>(this)->embed(&x, 4); }
  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dq(uint64_t x) { return static_cast<This*>(this)->embed(&x, 8); }

  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dint8(int8_t x) { return static_cast<This*>(this)->embed(&x, sizeof(int8_t)); }
  //! Add 8-bit integer data to the instruction stream.
  ASMJIT_INLINE Error duint8(uint8_t x) { return static_cast<This*>(this)->embed(&x, sizeof(uint8_t)); }

  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dint16(int16_t x) { return static_cast<This*>(this)->embed(&x, sizeof(int16_t)); }
  //! Add 16-bit integer data to the instruction stream.
  ASMJIT_INLINE Error duint16(uint16_t x) { return static_cast<This*>(this)->embed(&x, sizeof(uint16_t)); }

  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dint32(int32_t x) { return static_cast<This*>(this)->embed(&x, sizeof(int32_t)); }
  //! Add 32-bit integer data to the instruction stream.
  ASMJIT_INLINE Error duint32(uint32_t x) { return static_cast<This*>(this)->embed(&x, sizeof(uint32_t)); }

  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE Error dint64(int64_t x) { return static_cast<This*>(this)->embed(&x, sizeof(int64_t)); }
  //! Add 64-bit integer data to the instruction stream.
  ASMJIT_INLINE Error duint64(uint64_t x) { return static_cast<This*>(this)->embed(&x, sizeof(uint64_t)); }

  //! Add float data to the instruction stream.
  ASMJIT_INLINE Error dfloat(float x) { return static_cast<This*>(this)->embed(&x, sizeof(float)); }
  //! Add double data to the instruction stream.
  ASMJIT_INLINE Error ddouble(double x) { return static_cast<This*>(this)->embed(&x, sizeof(double)); }

  //! Add MMX data to the instruction stream.
  ASMJIT_INLINE Error dmm(const Data64& x) { return static_cast<This*>(this)->embed(&x, sizeof(Data64)); }
  //! Add XMM data to the instruction stream.
  ASMJIT_INLINE Error dxmm(const Data128& x) { return static_cast<This*>(this)->embed(&x, sizeof(Data128)); }
  //! Add YMM data to the instruction stream.
  ASMJIT_INLINE Error dymm(const Data256& x) { return static_cast<This*>(this)->embed(&x, sizeof(Data256)); }

  //! Add data in a given structure instance to the instruction stream.
  template<typename T>
  ASMJIT_INLINE Error dstruct(const T& x) { return static_cast<This*>(this)->embed(&x, static_cast<uint32_t>(sizeof(T))); }

  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

protected:
  ASMJIT_INLINE This& _addOptions(uint32_t options) noexcept {
    static_cast<This*>(this)->addOptions(options);
    return *static_cast<This*>(this);
  }

public:
  //! Force short form of jmp/jcc instruction.
  ASMJIT_INLINE This& short_() noexcept { return _addOptions(X86Inst::kOptionShortForm); }
  //! Force long form of jmp/jcc instruction.
  ASMJIT_INLINE This& long_() noexcept { return _addOptions(X86Inst::kOptionLongForm); }

  //! Condition is likely to be taken (has only benefit on P4).
  ASMJIT_INLINE This& taken() noexcept { return _addOptions(X86Inst::kOptionTaken); }
  //! Condition is unlikely to be taken (has only benefit on P4).
  ASMJIT_INLINE This& notTaken() noexcept { return _addOptions(X86Inst::kOptionNotTaken); }

  //! Use LOCK prefix.
  ASMJIT_INLINE This& lock() noexcept { return _addOptions(X86Inst::kOptionLock); }

  //! Use REP/REPZ prefix.
  ASMJIT_INLINE This& rep(const X86Gp& zcx) noexcept {
    static_cast<This*>(this)->_extraReg.init(zcx);
    return _addOptions(X86Inst::kOptionRep);
  }
  //! Use REPNZ prefix.
  ASMJIT_INLINE This& repnz(const X86Gp& zcx) noexcept {
    static_cast<This*>(this)->_extraReg.init(zcx);
    return _addOptions(X86Inst::kOptionRepnz);
  }

  //! Use REP/REPZ prefix.
  ASMJIT_INLINE This& repe(const X86Gp& zcx) noexcept { return rep(zcx); }
  //! Use REP/REPZ prefix.
  ASMJIT_INLINE This& repz(const X86Gp& zcx) noexcept { return rep(zcx); }
  //! Use REPNZ prefix.
  ASMJIT_INLINE This& repne(const X86Gp& zcx) noexcept { return repnz(zcx); }

  //! Prefer MOD_MR encoding over MOD_RM (the default) when encoding instruction
  //! that allows both. This option is only applicable to legacy instructions
  //! where both operands are registers.
  ASMJIT_INLINE This& mod_mr() noexcept { return _addOptions(X86Inst::kOptionModMR); }

  //! Force REX prefix to be emitted even when it's not needed (X64).
  //!
  //! NOTE: Don't use when using high 8-bit registers as REX prefix makes them
  //! inaccessible and \ref X86Assembler refuses to encode such instructions.
  ASMJIT_INLINE This& rex() noexcept { return _addOptions(X86Inst::kOptionRex);}

  //! Force REX.B prefix (X64) [It exists for special purposes only].
  ASMJIT_INLINE This& rex_b() noexcept { return _addOptions(X86Inst::kOptionOpCodeB); }
  //! Force REX.X prefix (X64) [It exists for special purposes only].
  ASMJIT_INLINE This& rex_x() noexcept { return _addOptions(X86Inst::kOptionOpCodeX); }
  //! Force REX.R prefix (X64) [It exists for special purposes only].
  ASMJIT_INLINE This& rex_r() noexcept { return _addOptions(X86Inst::kOptionOpCodeR); }
  //! Force REX.W prefix (X64) [It exists for special purposes only].
  ASMJIT_INLINE This& rex_w() noexcept { return _addOptions(X86Inst::kOptionOpCodeW); }

  //! Force 3-byte VEX prefix (AVX+).
  ASMJIT_INLINE This& vex3() noexcept { return _addOptions(X86Inst::kOptionVex3); }
  //! Force 4-byte EVEX prefix (AVX512+).
  ASMJIT_INLINE This& evex() noexcept { return _addOptions(X86Inst::kOptionEvex); }

  //! Use zeroing instead of merging (AVX512+).
  ASMJIT_INLINE This& z() noexcept { return _addOptions(X86Inst::kOptionZMask); }
  //! Broadcast one element to all other elements (AVX512+).
  ASMJIT_INLINE This& _1tox() noexcept { return _addOptions(X86Inst::kOption1ToX); }

  //! Suppress all exceptions (AVX512+).
  ASMJIT_INLINE This& sae() noexcept { return _addOptions(X86Inst::kOptionSAE); }
  //! Static rounding mode {rn} (round-to-nearest even) and {sae} (AVX512+).
  ASMJIT_INLINE This& rn_sae() noexcept { return _addOptions(X86Inst::kOptionER | X86Inst::kOptionRN_SAE); }
  //! Static rounding mode {rd} (round-down, toward -inf) and {sae} (AVX512+).
  ASMJIT_INLINE This& rd_sae() noexcept { return _addOptions(X86Inst::kOptionER | X86Inst::kOptionRD_SAE); }
  //! Static rounding mode {ru} (round-up, toward +inf) and {sae} (AVX512+).
  ASMJIT_INLINE This& ru_sae() noexcept { return _addOptions(X86Inst::kOptionER | X86Inst::kOptionRU_SAE); }
  //! Static rounding mode {rz} (round-toward-zero, truncate) and {sae} (AVX512+).
  ASMJIT_INLINE This& rz_sae() noexcept { return _addOptions(X86Inst::kOptionER | X86Inst::kOptionRZ_SAE); }

  // --------------------------------------------------------------------------
  // [General Purpose and Non-SIMD Instructions]
  // --------------------------------------------------------------------------

  ASMJIT_INST_2x(adc, Adc, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(adc, Adc, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2i(adc, Adc, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(adc, Adc, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(adc, Adc, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(adcx, Adcx, X86Gp, X86Gp)                                    // ADX
  ASMJIT_INST_2x(adcx, Adcx, X86Gp, X86Mem)                                   // ADX
  ASMJIT_INST_2x(add, Add, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(add, Add, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2i(add, Add, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(add, Add, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(add, Add, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(adox, Adox, X86Gp, X86Gp)                                    // ADX
  ASMJIT_INST_2x(adox, Adox, X86Gp, X86Mem)                                   // ADX
  ASMJIT_INST_2x(and_, And, X86Gp, X86Gp)                                     // ANY
  ASMJIT_INST_2x(and_, And, X86Gp, X86Mem)                                    // ANY
  ASMJIT_INST_2i(and_, And, X86Gp, Imm)                                       // ANY
  ASMJIT_INST_2x(and_, And, X86Mem, X86Gp)                                    // ANY
  ASMJIT_INST_2i(and_, And, X86Mem, Imm)                                      // ANY
  ASMJIT_INST_3x(andn, Andn, X86Gp, X86Gp, X86Gp)                             // BMI
  ASMJIT_INST_3x(andn, Andn, X86Gp, X86Gp, X86Mem)                            // BMI
  ASMJIT_INST_2x(arpl, Arpl, X86Gp, X86Gp)                                    // X86
  ASMJIT_INST_2x(arpl, Arpl, X86Mem, X86Gp)                                   // X86
  ASMJIT_INST_3x(bextr, Bextr, X86Gp, X86Gp, X86Gp)                           // BMI
  ASMJIT_INST_3x(bextr, Bextr, X86Gp, X86Mem, X86Gp)                          // BMI
  ASMJIT_INST_2x(blcfill, Blcfill, X86Gp, X86Gp)                              // TBM
  ASMJIT_INST_2x(blcfill, Blcfill, X86Gp, X86Mem)                             // TBM
  ASMJIT_INST_2x(blci, Blci, X86Gp, X86Gp)                                    // TBM
  ASMJIT_INST_2x(blci, Blci, X86Gp, X86Mem)                                   // TBM
  ASMJIT_INST_2x(blcic, Blcic, X86Gp, X86Gp)                                  // TBM
  ASMJIT_INST_2x(blcic, Blcic, X86Gp, X86Mem)                                 // TBM
  ASMJIT_INST_2x(blcmsk, Blcmsk, X86Gp, X86Gp)                                // TBM
  ASMJIT_INST_2x(blcmsk, Blcmsk, X86Gp, X86Mem)                               // TBM
  ASMJIT_INST_2x(blcs, Blcs, X86Gp, X86Gp)                                    // TBM
  ASMJIT_INST_2x(blcs, Blcs, X86Gp, X86Mem)                                   // TBM
  ASMJIT_INST_2x(blsfill, Blsfill, X86Gp, X86Gp)                              // TBM
  ASMJIT_INST_2x(blsfill, Blsfill, X86Gp, X86Mem)                             // TBM
  ASMJIT_INST_2x(blsi, Blsi, X86Gp, X86Gp)                                    // BMI
  ASMJIT_INST_2x(blsi, Blsi, X86Gp, X86Mem)                                   // BMI
  ASMJIT_INST_2x(blsic, Blsic, X86Gp, X86Gp)                                  // TBM
  ASMJIT_INST_2x(blsic, Blsic, X86Gp, X86Mem)                                 // TBM
  ASMJIT_INST_2x(blsmsk, Blsmsk, X86Gp, X86Gp)                                // BMI
  ASMJIT_INST_2x(blsmsk, Blsmsk, X86Gp, X86Mem)                               // BMI
  ASMJIT_INST_2x(blsr, Blsr, X86Gp, X86Gp)                                    // BMI
  ASMJIT_INST_2x(blsr, Blsr, X86Gp, X86Mem)                                   // BMI
  ASMJIT_INST_2x(bndcl, Bndcl, X86Bnd, X86Gp)                                 // MPX
  ASMJIT_INST_2x(bndcl, Bndcl, X86Bnd, X86Mem)                                // MPX
  ASMJIT_INST_2x(bndcn, Bndcn, X86Bnd, X86Gp)                                 // MPX
  ASMJIT_INST_2x(bndcn, Bndcn, X86Bnd, X86Mem)                                // MPX
  ASMJIT_INST_2x(bndcu, Bndcu, X86Bnd, X86Gp)                                 // MPX
  ASMJIT_INST_2x(bndcu, Bndcu, X86Bnd, X86Mem)                                // MPX
  ASMJIT_INST_2x(bndldx, Bndldx, X86Bnd, X86Mem)                              // MPX
  ASMJIT_INST_2x(bndmk, Bndmk, X86Bnd, X86Mem)                                // MPX
  ASMJIT_INST_2x(bndmov, Bndmov, X86Bnd, X86Bnd)                              // MPX
  ASMJIT_INST_2x(bndmov, Bndmov, X86Bnd, X86Mem)                              // MPX
  ASMJIT_INST_2x(bndmov, Bndmov, X86Mem, X86Bnd)                              // MPX
  ASMJIT_INST_2x(bndstx, Bndstx, X86Mem, X86Bnd)                              // MPX
  ASMJIT_INST_2x(bound, Bound, X86Gp, X86Mem)                                 // X86
  ASMJIT_INST_2x(bsf, Bsf, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(bsf, Bsf, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2x(bsr, Bsr, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(bsr, Bsr, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_1x(bswap, Bswap, X86Gp)                                         // ANY
  ASMJIT_INST_2x(bt, Bt, X86Gp, X86Gp)                                        // ANY
  ASMJIT_INST_2i(bt, Bt, X86Gp, Imm)                                          // ANY
  ASMJIT_INST_2x(bt, Bt, X86Mem, X86Gp)                                       // ANY
  ASMJIT_INST_2i(bt, Bt, X86Mem, Imm)                                         // ANY
  ASMJIT_INST_2x(btc, Btc, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2i(btc, Btc, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(btc, Btc, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(btc, Btc, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(btr, Btr, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2i(btr, Btr, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(btr, Btr, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(btr, Btr, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(bts, Bts, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2i(bts, Bts, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(bts, Bts, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(bts, Bts, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_3x(bzhi, Bzhi, X86Gp, X86Gp, X86Gp)                             // BMI2
  ASMJIT_INST_3x(bzhi, Bzhi, X86Gp, X86Mem, X86Gp)                            // BMI2
  ASMJIT_INST_1x(cbw, Cbw, AX)                                                // ANY       [EXPLICIT] AX      <- Sign Extend AL
  ASMJIT_INST_2x(cdq, Cdq, EDX, EAX)                                          // ANY       [EXPLICIT] EDX:EAX <- Sign Extend EAX
  ASMJIT_INST_1x(cdqe, Cdqe, EAX)                                             // X64       [EXPLICIT] RAX     <- Sign Extend EAX
  ASMJIT_INST_2x(cqo, Cqo, RDX, RAX)                                          // X64       [EXPLICIT] RDX:RAX <- Sign Extend RAX
  ASMJIT_INST_2x(cwd, Cwd, DX, AX)                                            // ANY       [EXPLICIT] DX:AX   <- Sign Extend AX
  ASMJIT_INST_1x(cwde, Cwde, EAX)                                             // ANY       [EXPLICIT] EAX     <- Sign Extend AX
  ASMJIT_INST_1x(call, Call, X86Gp)                                           // ANY
  ASMJIT_INST_1x(call, Call, X86Mem)                                          // ANY
  ASMJIT_INST_1x(call, Call, Label)                                           // ANY
  ASMJIT_INST_1i(call, Call, Imm)                                             // ANY
  ASMJIT_INST_0x(clac, Clac)                                                  // SMAP
  ASMJIT_INST_0x(clc, Clc)                                                    // ANY
  ASMJIT_INST_0x(cld, Cld)                                                    // ANY
  ASMJIT_INST_1x(clflush, Clflush, X86Mem)                                    // CLFLUSH
  ASMJIT_INST_1x(clflushopt, Clflushopt, X86Mem)                              // CLFLUSH_OPT
  ASMJIT_INST_0x(cli, Cli)                                                    // ANY
  ASMJIT_INST_0x(clts, Clts)                                                  // ANY
  ASMJIT_INST_1x(clwb, Clwb, X86Mem)                                          // CLWB
  ASMJIT_INST_1x(clzero, Clzero, DS_ZAX)                                      // CLZERO    [EXPLICIT]
  ASMJIT_INST_0x(cmc, Cmc)                                                    // ANY
  ASMJIT_INST_2c(cmov, Cmov, X86Inst::condToCmovcc, X86Gp, X86Gp)             // CMOV
  ASMJIT_INST_2c(cmov, Cmov, X86Inst::condToCmovcc, X86Gp, X86Mem)            // CMOV
  ASMJIT_INST_2x(cmp, Cmp, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(cmp, Cmp, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2i(cmp, Cmp, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(cmp, Cmp, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(cmp, Cmp, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(cmps, Cmps, DS_ZSI, ES_ZDI)                                  // ANY       [EXPLICIT]
  ASMJIT_INST_3x(cmpxchg, Cmpxchg, X86Gp, X86Gp, ZAX)                         // I486      [EXPLICIT]
  ASMJIT_INST_3x(cmpxchg, Cmpxchg, X86Mem, X86Gp, ZAX)                        // I486      [EXPLICIT]
  ASMJIT_INST_5x(cmpxchg16b, Cmpxchg16b, X86Mem, RDX, RAX, RCX, RBX);         // CMPXCHG16B[EXPLICIT] m == EDX:EAX ? m <- ECX:EBX
  ASMJIT_INST_5x(cmpxchg8b, Cmpxchg8b, X86Mem, EDX, EAX, ECX, EBX);           // CMPXCHG8B [EXPLICIT] m == RDX:RAX ? m <- RCX:RBX
  ASMJIT_INST_4x(cpuid, Cpuid, EAX, EBX, ECX, EDX)                            // I486      [EXPLICIT] EAX:EBX:ECX:EDX  <- CPUID[EAX:ECX]
  ASMJIT_INST_2x(crc32, Crc32, X86Gp, X86Gp)                                  // SSE4_2
  ASMJIT_INST_2x(crc32, Crc32, X86Gp, X86Mem)                                 // SSE4_2
  ASMJIT_INST_1x(daa, Daa, X86Gp)                                             // X86       [EXPLICIT]
  ASMJIT_INST_1x(das, Das, X86Gp)                                             // X86       [EXPLICIT]
  ASMJIT_INST_1x(dec, Dec, X86Gp)                                             // ANY
  ASMJIT_INST_1x(dec, Dec, X86Mem)                                            // ANY
  ASMJIT_INST_2x(div, Div, X86Gp, X86Gp)                                      // ANY       [EXPLICIT]  AH[Rem]: AL[Quot] <- AX / r8
  ASMJIT_INST_2x(div, Div, X86Gp, X86Mem)                                     // ANY       [EXPLICIT]  AH[Rem]: AL[Quot] <- AX / m8
  ASMJIT_INST_3x(div, Div, X86Gp, X86Gp, X86Gp)                               // ANY       [EXPLICIT] xDX[Rem]:xAX[Quot] <- xDX:xAX / r16|r32|r64
  ASMJIT_INST_3x(div, Div, X86Gp, X86Gp, X86Mem)                              // ANY       [EXPLICIT] xDX[Rem]:xAX[Quot] <- xDX:xAX / m16|m32|m64
  ASMJIT_INST_0x(emms, Emms)                                                  // MMX
  ASMJIT_INST_2x(enter, Enter, Imm, Imm)                                      // ANY
  ASMJIT_INST_1x(fxrstor, Fxrstor, X86Mem)                                    // FXSR
  ASMJIT_INST_1x(fxrstor64, Fxrstor64, X86Mem)                                // FXSR
  ASMJIT_INST_1x(fxsave, Fxsave, X86Mem)                                      // FXSR
  ASMJIT_INST_1x(fxsave64, Fxsave64, X86Mem)                                  // FXSR
  ASMJIT_INST_0x(hlt, Hlt)                                                    // ANY
  ASMJIT_INST_2x(idiv, Idiv, X86Gp, X86Gp)                                    // ANY       [EXPLICIT]  AH[Rem]: AL[Quot] <- AX / r8
  ASMJIT_INST_2x(idiv, Idiv, X86Gp, X86Mem)                                   // ANY       [EXPLICIT]  AH[Rem]: AL[Quot] <- AX / m8
  ASMJIT_INST_3x(idiv, Idiv, X86Gp, X86Gp, X86Gp)                             // ANY       [EXPLICIT] xDX[Rem]:xAX[Quot] <- xDX:xAX / r16|r32|r64
  ASMJIT_INST_3x(idiv, Idiv, X86Gp, X86Gp, X86Mem)                            // ANY       [EXPLICIT] xDX[Rem]:xAX[Quot] <- xDX:xAX / m16|m32|m64
  ASMJIT_INST_2x(imul, Imul, X86Gp, X86Gp)                                    // ANY       [EXPLICIT] AX <- AL * r8 | ra <- ra * rb
  ASMJIT_INST_2x(imul, Imul, X86Gp, X86Mem)                                   // ANY       [EXPLICIT] AX <- AL * m8 | ra <- ra * m16|m32|m64
  ASMJIT_INST_2i(imul, Imul, X86Gp, Imm)                                      // ANY
  ASMJIT_INST_3i(imul, Imul, X86Gp, X86Gp, Imm)                               // ANY
  ASMJIT_INST_3i(imul, Imul, X86Gp, X86Mem, Imm)                              // ANY
  ASMJIT_INST_3x(imul, Imul, X86Gp, X86Gp, X86Gp)                             // ANY       [EXPLICIT] xDX:xAX <- xAX * r16|r32|r64
  ASMJIT_INST_3x(imul, Imul, X86Gp, X86Gp, X86Mem)                            // ANY       [EXPLICIT] xDX:xAX <- xAX * m16|m32|m64
  ASMJIT_INST_2i(in, In, ZAX, Imm)                                            // ANY
  ASMJIT_INST_2x(in, In, ZAX, DX)                                             // ANY
  ASMJIT_INST_1x(inc, Inc, X86Gp)                                             // ANY
  ASMJIT_INST_1x(inc, Inc, X86Mem)                                            // ANY
  ASMJIT_INST_2x(ins, Ins, ES_ZDI, DX)                                        // ANY
  ASMJIT_INST_1i(int_, Int, Imm)                                              // ANY
  ASMJIT_INST_0x(int3, Int3)                                                  // ANY
  ASMJIT_INST_0x(into, Into)                                                  // ANY
  ASMJIT_INST_0x(invd, Invd)                                                  // ANY
  ASMJIT_INST_1x(invlpg, Invlpg, X86Mem)                                      // ANY
  ASMJIT_INST_2x(invpcid, Invpcid, X86Gp, X86Mem)                             // ANY
  ASMJIT_INST_1c(j, J, X86Inst::condToJcc, Label)                             // ANY
  ASMJIT_INST_1c(j, J, X86Inst::condToJcc, Imm)                               // ANY
  ASMJIT_INST_1c(j, J, X86Inst::condToJcc, uint64_t)                          // ANY
  ASMJIT_INST_2x(jecxz, Jecxz, X86Gp, Label)                                  // ANY       [EXPLICIT] Short jump if CX/ECX/RCX is zero.
  ASMJIT_INST_2x(jecxz, Jecxz, X86Gp, Imm)                                    // ANY       [EXPLICIT] Short jump if CX/ECX/RCX is zero.
  ASMJIT_INST_2x(jecxz, Jecxz, X86Gp, uint64_t)                               // ANY       [EXPLICIT] Short jump if CX/ECX/RCX is zero.
  ASMJIT_INST_1x(jmp, Jmp, X86Gp)                                             // ANY
  ASMJIT_INST_1x(jmp, Jmp, X86Mem)                                            // ANY
  ASMJIT_INST_1x(jmp, Jmp, Label)                                             // ANY
  ASMJIT_INST_1x(jmp, Jmp, Imm)                                               // ANY
  ASMJIT_INST_1x(jmp, Jmp, uint64_t)                                          // ANY
  ASMJIT_INST_1x(lahf, Lahf, AH)                                              // LAHFSAHF  [EXPLICIT] AH <- EFL
  ASMJIT_INST_2x(lar, Lar, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(lar, Lar, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_1x(ldmxcsr, Ldmxcsr, X86Mem)                                    // SSE
  ASMJIT_INST_2x(lds, Lds, X86Gp, X86Mem)                                     // X86
  ASMJIT_INST_2x(lea, Lea, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_0x(leave, Leave)                                                // ANY
  ASMJIT_INST_2x(les, Les, X86Gp, X86Mem)                                     // X86
  ASMJIT_INST_0x(lfence, Lfence)                                              // SSE2
  ASMJIT_INST_2x(lfs, Lfs, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_1x(lgdt, Lgdt, X86Mem)                                          // ANY
  ASMJIT_INST_2x(lgs, Lgs, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_1x(lidt, Lidt, X86Mem)                                          // ANY
  ASMJIT_INST_1x(lldt, Lldt, X86Gp)                                           // ANY
  ASMJIT_INST_1x(lldt, Lldt, X86Mem)                                          // ANY
  ASMJIT_INST_1x(lmsw, Lmsw, X86Gp)                                           // ANY
  ASMJIT_INST_1x(lmsw, Lmsw, X86Mem)                                          // ANY
  ASMJIT_INST_2x(lods, Lods, ZAX, DS_ZSI)                                     // ANY       [EXPLICIT]
  ASMJIT_INST_2x(loop, Loop, ZCX, Label)                                      // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0.
  ASMJIT_INST_2x(loop, Loop, ZCX, Imm)                                        // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0.
  ASMJIT_INST_2x(loop, Loop, ZCX, uint64_t)                                   // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0.
  ASMJIT_INST_2x(loope, Loope, ZCX, Label)                                    // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 1.
  ASMJIT_INST_2x(loope, Loope, ZCX, Imm)                                      // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 1.
  ASMJIT_INST_2x(loope, Loope, ZCX, uint64_t)                                 // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 1.
  ASMJIT_INST_2x(loopne, Loopne, ZCX, Label)                                  // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 0.
  ASMJIT_INST_2x(loopne, Loopne, ZCX, Imm)                                    // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 0.
  ASMJIT_INST_2x(loopne, Loopne, ZCX, uint64_t)                               // ANY       [EXPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 0.
  ASMJIT_INST_2x(lsl, Lsl, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(lsl, Lsl, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2x(lss, Lss, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_1x(ltr, Ltr, X86Gp)                                             // ANY
  ASMJIT_INST_1x(ltr, Ltr, X86Mem)                                            // ANY
  ASMJIT_INST_2x(lzcnt, Lzcnt, X86Gp, X86Gp)                                  // LZCNT
  ASMJIT_INST_2x(lzcnt, Lzcnt, X86Gp, X86Mem)                                 // LZCNT
  ASMJIT_INST_0x(mfence, Mfence)                                              // SSE2
  ASMJIT_INST_2x(mov, Mov, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(mov, Mov, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2i(mov, Mov, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(mov, Mov, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(mov, Mov, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(mov, Mov, X86Gp, X86CReg)                                    // ANY
  ASMJIT_INST_2x(mov, Mov, X86CReg, X86Gp)                                    // ANY
  ASMJIT_INST_2x(mov, Mov, X86Gp, X86DReg)                                    // ANY
  ASMJIT_INST_2x(mov, Mov, X86DReg, X86Gp)                                    // ANY
  ASMJIT_INST_2x(mov, Mov, X86Gp, X86Seg)                                     // ANY
  ASMJIT_INST_2x(mov, Mov, X86Mem, X86Seg)                                    // ANY
  ASMJIT_INST_2x(mov, Mov, X86Seg, X86Gp)                                     // ANY
  ASMJIT_INST_2x(mov, Mov, X86Seg, X86Mem)                                    // ANY
  ASMJIT_INST_2x(movbe, Movbe, X86Gp, X86Mem)                                 // MOVBE
  ASMJIT_INST_2x(movbe, Movbe, X86Mem, X86Gp)                                 // MOVBE
  ASMJIT_INST_2x(movnti, Movnti, X86Mem, X86Gp)                               // SSE2
  ASMJIT_INST_2x(movs, Movs, ES_ZDI, DS_ZSI)                                  // ANY       [EXPLICIT]
  ASMJIT_INST_2x(movsx, Movsx, X86Gp, X86Gp)                                  // ANY
  ASMJIT_INST_2x(movsx, Movsx, X86Gp, X86Mem)                                 // ANY
  ASMJIT_INST_2x(movsxd, Movsxd, X86Gp, X86Gp)                                // X64
  ASMJIT_INST_2x(movsxd, Movsxd, X86Gp, X86Mem)                               // X64
  ASMJIT_INST_2x(movzx, Movzx, X86Gp, X86Gp)                                  // ANY
  ASMJIT_INST_2x(movzx, Movzx, X86Gp, X86Mem)                                 // ANY
  ASMJIT_INST_2x(mul, Mul, AX, X86Gp)                                         // ANY       [EXPLICIT] AX      <-  AL * r8
  ASMJIT_INST_2x(mul, Mul, AX, X86Mem)                                        // ANY       [EXPLICIT] AX      <-  AL * m8
  ASMJIT_INST_3x(mul, Mul, ZDX, ZAX, X86Gp)                                   // ANY       [EXPLICIT] xDX:xAX <- xAX * r16|r32|r64
  ASMJIT_INST_3x(mul, Mul, ZDX, ZAX, X86Mem)                                  // ANY       [EXPLICIT] xDX:xAX <- xAX * m16|m32|m64
  ASMJIT_INST_4x(mulx, Mulx, X86Gp, X86Gp, X86Gp, ZDX)                        // BMI2      [EXPLICIT]
  ASMJIT_INST_4x(mulx, Mulx, X86Gp, X86Gp, X86Mem, ZDX)                       // BMI2      [EXPLICIT]
  ASMJIT_INST_1x(neg, Neg, X86Gp)                                             // ANY
  ASMJIT_INST_1x(neg, Neg, X86Mem)                                            // ANY
  ASMJIT_INST_0x(nop, Nop)                                                    // ANY
  ASMJIT_INST_1x(not_, Not, X86Gp)                                            // ANY
  ASMJIT_INST_1x(not_, Not, X86Mem)                                           // ANY
  ASMJIT_INST_2x(or_, Or, X86Gp, X86Gp)                                       // ANY
  ASMJIT_INST_2x(or_, Or, X86Gp, X86Mem)                                      // ANY
  ASMJIT_INST_2i(or_, Or, X86Gp, Imm)                                         // ANY
  ASMJIT_INST_2x(or_, Or, X86Mem, X86Gp)                                      // ANY
  ASMJIT_INST_2i(or_, Or, X86Mem, Imm)                                        // ANY
  ASMJIT_INST_2x(out, Out, Imm, ZAX)                                          // ANY
  ASMJIT_INST_2i(out, Out, DX, ZAX)                                           // ANY
  ASMJIT_INST_2i(outs, Outs, DX, DS_ZSI)                                      // ANY
  ASMJIT_INST_0x(pause, Pause)                                                // SSE2
  ASMJIT_INST_3x(pdep, Pdep, X86Gp, X86Gp, X86Gp)                             // BMI2
  ASMJIT_INST_3x(pdep, Pdep, X86Gp, X86Gp, X86Mem)                            // BMI2
  ASMJIT_INST_3x(pext, Pext, X86Gp, X86Gp, X86Gp)                             // BMI2
  ASMJIT_INST_3x(pext, Pext, X86Gp, X86Gp, X86Mem)                            // BMI2
  ASMJIT_INST_0x(pcommit, Pcommit)                                            // PCOMMIT
  ASMJIT_INST_1x(pop, Pop, X86Gp)                                             // ANY
  ASMJIT_INST_1x(pop, Pop, X86Mem)                                            // ANY
  ASMJIT_INST_1x(pop, Pop, X86Seg);                                           // ANY
  ASMJIT_INST_0x(popa, Popa)                                                  // X86
  ASMJIT_INST_0x(popad, Popad)                                                // X86
  ASMJIT_INST_2x(popcnt, Popcnt, X86Gp, X86Gp)                                // POPCNT
  ASMJIT_INST_2x(popcnt, Popcnt, X86Gp, X86Mem)                               // POPCNT
  ASMJIT_INST_0x(popf, Popf)                                                  // ANY
  ASMJIT_INST_0x(popfd, Popfd)                                                // X86
  ASMJIT_INST_0x(popfq, Popfq)                                                // X64
  ASMJIT_INST_1x(prefetch, Prefetch, X86Mem)                                  // 3DNOW
  ASMJIT_INST_1x(prefetchnta, Prefetchnta, X86Mem)                            // SSE
  ASMJIT_INST_1x(prefetcht0, Prefetcht0, X86Mem)                              // SSE
  ASMJIT_INST_1x(prefetcht1, Prefetcht1, X86Mem)                              // SSE
  ASMJIT_INST_1x(prefetcht2, Prefetcht2, X86Mem)                              // SSE
  ASMJIT_INST_1x(prefetchw, Prefetchw, X86Mem)                                // PREFETCHW
  ASMJIT_INST_1x(prefetchwt1, Prefetchwt1, X86Mem)                            // PREFETCHW1
  ASMJIT_INST_1x(push, Push, X86Gp)                                           // ANY
  ASMJIT_INST_1x(push, Push, X86Mem)                                          // ANY
  ASMJIT_INST_1x(push, Push, X86Seg)                                          // ANY
  ASMJIT_INST_1i(push, Push, Imm)                                             // ANY
  ASMJIT_INST_0x(pusha, Pusha)                                                // X86
  ASMJIT_INST_0x(pushad, Pushad)                                              // X86
  ASMJIT_INST_0x(pushf, Pushf)                                                // ANY
  ASMJIT_INST_0x(pushfd, Pushfd)                                              // X86
  ASMJIT_INST_0x(pushfq, Pushfq)                                              // X64
  ASMJIT_INST_2x(rcl, Rcl, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(rcl, Rcl, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(rcl, Rcl, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(rcl, Rcl, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(rcr, Rcr, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(rcr, Rcr, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(rcr, Rcr, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(rcr, Rcr, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_1x(rdfsbase, Rdfsbase, X86Gp)                                   // FSGSBASE
  ASMJIT_INST_1x(rdgsbase, Rdgsbase, X86Gp)                                   // FSGSBASE
  ASMJIT_INST_1x(rdrand, Rdrand, X86Gp)                                       // RDRAND
  ASMJIT_INST_1x(rdseed, Rdseed, X86Gp)                                       // RDSEED
  ASMJIT_INST_3x(rdmsr, Rdmsr, EDX, EAX, ECX)                                 // MSR       [EXPLICIT] RDX:EAX     <- MSR[ECX]
  ASMJIT_INST_3x(rdpmc, Rdpmc, EDX, EAX, ECX)                                 // ANY       [EXPLICIT] RDX:EAX     <- PMC[ECX]
  ASMJIT_INST_2x(rdtsc, Rdtsc, EDX, EAX)                                      // RDTSC     [EXPLICIT] EDX:EAX     <- Counter
  ASMJIT_INST_3x(rdtscp, Rdtscp, EDX, EAX, ECX)                               // RDTSCP    [EXPLICIT] EDX:EAX:EXC <- Counter
  ASMJIT_INST_2x(rol, Rol, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(rol, Rol, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(rol, Rol, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(rol, Rol, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(ror, Ror, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(ror, Ror, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(ror, Ror, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(ror, Ror, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_3i(rorx, Rorx, X86Gp, X86Gp, Imm)                               // BMI2
  ASMJIT_INST_3i(rorx, Rorx, X86Gp, X86Mem, Imm)                              // BMI2
  ASMJIT_INST_0x(rsm, Rsm)                                                    // X86
  ASMJIT_INST_2x(sbb, Sbb, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(sbb, Sbb, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2i(sbb, Sbb, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(sbb, Sbb, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(sbb, Sbb, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_1x(sahf, Sahf, AH)                                              // LAHFSAHF  [EXPLICIT] EFL <- AH
  ASMJIT_INST_2x(sal, Sal, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(sal, Sal, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(sal, Sal, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(sal, Sal, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_2x(sar, Sar, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(sar, Sar, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(sar, Sar, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(sar, Sar, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_3x(sarx, Sarx, X86Gp, X86Gp, X86Gp)                             // BMI2
  ASMJIT_INST_3x(sarx, Sarx, X86Gp, X86Mem, X86Gp)                            // BMI2
  ASMJIT_INST_2x(scas, Scas, ZAX, ES_ZDI)                                     // ANY       [EXPLICIT]
  ASMJIT_INST_1c(set, Set, X86Inst::condToSetcc, X86Gp)                       // ANY
  ASMJIT_INST_1c(set, Set, X86Inst::condToSetcc, X86Mem)                      // ANY
  ASMJIT_INST_0x(sfence, Sfence)                                              // SSE
  ASMJIT_INST_1x(sgdt, Sgdt, X86Mem)                                          // ANY
  ASMJIT_INST_2x(shl, Shl, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(shl, Shl, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(shl, Shl, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(shl, Shl, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_3x(shlx, Shlx, X86Gp, X86Gp, X86Gp)                             // BMI2
  ASMJIT_INST_3x(shlx, Shlx, X86Gp, X86Mem, X86Gp)                            // BMI2
  ASMJIT_INST_2x(shr, Shr, X86Gp, CL)                                         // ANY
  ASMJIT_INST_2x(shr, Shr, X86Mem, CL)                                        // ANY
  ASMJIT_INST_2i(shr, Shr, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2i(shr, Shr, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_3x(shrx, Shrx, X86Gp, X86Gp, X86Gp)                             // BMI2
  ASMJIT_INST_3x(shrx, Shrx, X86Gp, X86Mem, X86Gp)                            // BMI2
  ASMJIT_INST_3x(shld, Shld, X86Gp, X86Gp, CL)                                // ANY
  ASMJIT_INST_3x(shld, Shld, X86Mem, X86Gp, CL)                               // ANY
  ASMJIT_INST_3i(shld, Shld, X86Gp, X86Gp, Imm)                               // ANY
  ASMJIT_INST_3i(shld, Shld, X86Mem, X86Gp, Imm)                              // ANY
  ASMJIT_INST_3x(shrd, Shrd, X86Gp, X86Gp, CL)                                // ANY
  ASMJIT_INST_3x(shrd, Shrd, X86Mem, X86Gp, CL)                               // ANY
  ASMJIT_INST_3i(shrd, Shrd, X86Gp, X86Gp, Imm)                               // ANY
  ASMJIT_INST_3i(shrd, Shrd, X86Mem, X86Gp, Imm)                              // ANY
  ASMJIT_INST_1x(sidt, Sidt, X86Mem)                                          // ANY
  ASMJIT_INST_1x(sldt, Sldt, X86Gp)                                           // ANY
  ASMJIT_INST_1x(sldt, Sldt, X86Mem)                                          // ANY
  ASMJIT_INST_1x(smsw, Smsw, X86Gp)                                           // ANY
  ASMJIT_INST_1x(smsw, Smsw, X86Mem)                                          // ANY
  ASMJIT_INST_0x(stac, Stac)                                                  // SMAP
  ASMJIT_INST_0x(stc, Stc)                                                    // ANY
  ASMJIT_INST_0x(std, Std)                                                    // ANY
  ASMJIT_INST_0x(sti, Sti)                                                    // ANY
  ASMJIT_INST_1x(stmxcsr, Stmxcsr, X86Mem)                                    // SSE
  ASMJIT_INST_2x(stos, Stos, ES_ZDI, ZAX)                                     // ANY       [EXPLICIT]
  ASMJIT_INST_1x(str, Str, X86Gp)                                             // ANY
  ASMJIT_INST_1x(str, Str, X86Mem)                                            // ANY
  ASMJIT_INST_2x(sub, Sub, X86Gp, X86Gp)                                      // ANY
  ASMJIT_INST_2x(sub, Sub, X86Gp, X86Mem)                                     // ANY
  ASMJIT_INST_2i(sub, Sub, X86Gp, Imm)                                        // ANY
  ASMJIT_INST_2x(sub, Sub, X86Mem, X86Gp)                                     // ANY
  ASMJIT_INST_2i(sub, Sub, X86Mem, Imm)                                       // ANY
  ASMJIT_INST_0x(swapgs, Swapgs)                                              // X64
  ASMJIT_INST_2x(t1mskc, T1mskc, X86Gp, X86Gp)                                // TBM
  ASMJIT_INST_2x(t1mskc, T1mskc, X86Gp, X86Mem)                               // TBM
  ASMJIT_INST_2x(test, Test, X86Gp, X86Gp)                                    // ANY
  ASMJIT_INST_2i(test, Test, X86Gp, Imm)                                      // ANY
  ASMJIT_INST_2x(test, Test, X86Mem, X86Gp)                                   // ANY
  ASMJIT_INST_2i(test, Test, X86Mem, Imm)                                     // ANY
  ASMJIT_INST_2x(tzcnt, Tzcnt, X86Gp, X86Gp)                                  // BMI
  ASMJIT_INST_2x(tzcnt, Tzcnt, X86Gp, X86Mem)                                 // BMI
  ASMJIT_INST_2x(tzmsk, Tzmsk, X86Gp, X86Gp)                                  // TBM
  ASMJIT_INST_2x(tzmsk, Tzmsk, X86Gp, X86Mem)                                 // TBM
  ASMJIT_INST_0x(ud2, Ud2)                                                    // ANY
  ASMJIT_INST_1x(verr, Verr, X86Gp)                                           // ANY
  ASMJIT_INST_1x(verr, Verr, X86Mem)                                          // ANY
  ASMJIT_INST_1x(verw, Verw, X86Gp)                                           // ANY
  ASMJIT_INST_1x(verw, Verw, X86Mem)                                          // ANY
  ASMJIT_INST_1x(wrfsbase, Wrfsbase, X86Gp)                                   // FSGSBASE
  ASMJIT_INST_1x(wrgsbase, Wrgsbase, X86Gp)                                   // FSGSBASE
  ASMJIT_INST_3x(wrmsr, Wrmsr, EDX, EAX, ECX)                                 // MSR       [EXPLICIT] RDX:EAX     -> MSR[ECX]
  ASMJIT_INST_0x(xabort, Xabort)                                              // RTM
  ASMJIT_INST_2x(xadd, Xadd, X86Gp, X86Gp)                                    // ANY
  ASMJIT_INST_2x(xadd, Xadd, X86Mem, X86Gp)                                   // ANY
  ASMJIT_INST_1x(xbegin, Xbegin, Label)                                       // RTM
  ASMJIT_INST_1x(xbegin, Xbegin, Imm)                                         // RTM
  ASMJIT_INST_1x(xbegin, Xbegin, uint64_t)                                    // RTM
  ASMJIT_INST_2x(xchg, Xchg, X86Gp, X86Gp)                                    // ANY
  ASMJIT_INST_2x(xchg, Xchg, X86Mem, X86Gp)                                   // ANY
  ASMJIT_INST_2x(xchg, Xchg, X86Gp, X86Mem)                                   // ANY
  ASMJIT_INST_0x(xend, Xend)                                                  // RTM
  ASMJIT_INST_3x(xgetbv, Xgetbv, EDX, EAX, ECX)                               // XSAVE     [EXPLICIT] EDX:EAX <- XCR[ECX]
  ASMJIT_INST_2x(xor_, Xor, X86Gp, X86Gp)                                     // ANY
  ASMJIT_INST_2x(xor_, Xor, X86Gp, X86Mem)                                    // ANY
  ASMJIT_INST_2i(xor_, Xor, X86Gp, Imm)                                       // ANY
  ASMJIT_INST_2x(xor_, Xor, X86Mem, X86Gp)                                    // ANY
  ASMJIT_INST_2i(xor_, Xor, X86Mem, Imm)                                      // ANY
  ASMJIT_INST_3x(xsetbv, Xsetbv, EDX, EAX, ECX)                               // XSAVE     [EXPLICIT] XCR[ECX] <- EDX:EAX
  ASMJIT_INST_0x(xtest, Xtest)                                                // TSX

  // --------------------------------------------------------------------------
  // [FPU Instructions]
  // --------------------------------------------------------------------------

  ASMJIT_INST_0x(f2xm1, F2xm1)                                                // FPU
  ASMJIT_INST_0x(fabs, Fabs)                                                  // FPU
  ASMJIT_INST_2x(fadd, Fadd, X86Fp, X86Fp)                                    // FPU
  ASMJIT_INST_1x(fadd, Fadd, X86Mem)                                          // FPU
  ASMJIT_INST_1x(faddp, Faddp, X86Fp)                                         // FPU
  ASMJIT_INST_0x(faddp, Faddp)                                                // FPU
  ASMJIT_INST_1x(fbld, Fbld, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fbstp, Fbstp, X86Mem)                                        // FPU
  ASMJIT_INST_0x(fchs, Fchs)                                                  // FPU
  ASMJIT_INST_0x(fclex, Fclex)                                                // FPU
  ASMJIT_INST_1x(fcmovb, Fcmovb, X86Fp)                                       // FPU
  ASMJIT_INST_1x(fcmovbe, Fcmovbe, X86Fp)                                     // FPU
  ASMJIT_INST_1x(fcmove, Fcmove, X86Fp)                                       // FPU
  ASMJIT_INST_1x(fcmovnb, Fcmovnb, X86Fp)                                     // FPU
  ASMJIT_INST_1x(fcmovnbe, Fcmovnbe, X86Fp)                                   // FPU
  ASMJIT_INST_1x(fcmovne, Fcmovne, X86Fp)                                     // FPU
  ASMJIT_INST_1x(fcmovnu, Fcmovnu, X86Fp)                                     // FPU
  ASMJIT_INST_1x(fcmovu, Fcmovu, X86Fp)                                       // FPU
  ASMJIT_INST_1x(fcom, Fcom, X86Fp)                                           // FPU
  ASMJIT_INST_0x(fcom, Fcom)                                                  // FPU
  ASMJIT_INST_1x(fcom, Fcom, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fcomp, Fcomp, X86Fp)                                         // FPU
  ASMJIT_INST_0x(fcomp, Fcomp)                                                // FPU
  ASMJIT_INST_1x(fcomp, Fcomp, X86Mem)                                        // FPU
  ASMJIT_INST_0x(fcompp, Fcompp)                                              // FPU
  ASMJIT_INST_1x(fcomi, Fcomi, X86Fp)                                         // FPU
  ASMJIT_INST_1x(fcomip, Fcomip, X86Fp)                                       // FPU
  ASMJIT_INST_0x(fcos, Fcos)                                                  // FPU
  ASMJIT_INST_0x(fdecstp, Fdecstp)                                            // FPU
  ASMJIT_INST_2x(fdiv, Fdiv, X86Fp, X86Fp)                                    // FPU
  ASMJIT_INST_1x(fdiv, Fdiv, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fdivp, Fdivp, X86Fp)                                         // FPU
  ASMJIT_INST_0x(fdivp, Fdivp)                                                // FPU
  ASMJIT_INST_2x(fdivr, Fdivr, X86Fp, X86Fp)                                  // FPU
  ASMJIT_INST_1x(fdivr, Fdivr, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fdivrp, Fdivrp, X86Fp)                                       // FPU
  ASMJIT_INST_0x(fdivrp, Fdivrp)                                              // FPU
  ASMJIT_INST_1x(ffree, Ffree, X86Fp)                                         // FPU
  ASMJIT_INST_1x(fiadd, Fiadd, X86Mem)                                        // FPU
  ASMJIT_INST_1x(ficom, Ficom, X86Mem)                                        // FPU
  ASMJIT_INST_1x(ficomp, Ficomp, X86Mem)                                      // FPU
  ASMJIT_INST_1x(fidiv, Fidiv, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fidivr, Fidivr, X86Mem)                                      // FPU
  ASMJIT_INST_1x(fild, Fild, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fimul, Fimul, X86Mem)                                        // FPU
  ASMJIT_INST_0x(fincstp, Fincstp)                                            // FPU
  ASMJIT_INST_0x(finit, Finit)                                                // FPU
  ASMJIT_INST_1x(fisub, Fisub, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fisubr, Fisubr, X86Mem)                                      // FPU
  ASMJIT_INST_0x(fninit, Fninit)                                              // FPU
  ASMJIT_INST_1x(fist, Fist, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fistp, Fistp, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fisttp, Fisttp, X86Mem)                                      // FPU+SSE3
  ASMJIT_INST_1x(fld, Fld, X86Mem)                                            // FPU
  ASMJIT_INST_1x(fld, Fld, X86Fp)                                             // FPU
  ASMJIT_INST_0x(fld1, Fld1)                                                  // FPU
  ASMJIT_INST_0x(fldl2t, Fldl2t)                                              // FPU
  ASMJIT_INST_0x(fldl2e, Fldl2e)                                              // FPU
  ASMJIT_INST_0x(fldpi, Fldpi)                                                // FPU
  ASMJIT_INST_0x(fldlg2, Fldlg2)                                              // FPU
  ASMJIT_INST_0x(fldln2, Fldln2)                                              // FPU
  ASMJIT_INST_0x(fldz, Fldz)                                                  // FPU
  ASMJIT_INST_1x(fldcw, Fldcw, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fldenv, Fldenv, X86Mem)                                      // FPU
  ASMJIT_INST_2x(fmul, Fmul, X86Fp, X86Fp)                                    // FPU
  ASMJIT_INST_1x(fmul, Fmul, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fmulp, Fmulp, X86Fp)                                         // FPU
  ASMJIT_INST_0x(fmulp, Fmulp)                                                // FPU
  ASMJIT_INST_0x(fnclex, Fnclex)                                              // FPU
  ASMJIT_INST_0x(fnop, Fnop)                                                  // FPU
  ASMJIT_INST_1x(fnsave, Fnsave, X86Mem)                                      // FPU
  ASMJIT_INST_1x(fnstenv, Fnstenv, X86Mem)                                    // FPU
  ASMJIT_INST_1x(fnstcw, Fnstcw, X86Mem)                                      // FPU
  ASMJIT_INST_0x(fpatan, Fpatan)                                              // FPU
  ASMJIT_INST_0x(fprem, Fprem)                                                // FPU
  ASMJIT_INST_0x(fprem1, Fprem1)                                              // FPU
  ASMJIT_INST_0x(fptan, Fptan)                                                // FPU
  ASMJIT_INST_0x(frndint, Frndint)                                            // FPU
  ASMJIT_INST_1x(frstor, Frstor, X86Mem)                                      // FPU
  ASMJIT_INST_1x(fsave, Fsave, X86Mem)                                        // FPU
  ASMJIT_INST_0x(fscale, Fscale)                                              // FPU
  ASMJIT_INST_0x(fsin, Fsin)                                                  // FPU
  ASMJIT_INST_0x(fsincos, Fsincos)                                            // FPU
  ASMJIT_INST_0x(fsqrt, Fsqrt)                                                // FPU
  ASMJIT_INST_1x(fst, Fst, X86Mem)                                            // FPU
  ASMJIT_INST_1x(fst, Fst, X86Fp)                                             // FPU
  ASMJIT_INST_1x(fstp, Fstp, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fstp, Fstp, X86Fp)                                           // FPU
  ASMJIT_INST_1x(fstcw, Fstcw, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fstenv, Fstenv, X86Mem)                                      // FPU
  ASMJIT_INST_2x(fsub, Fsub, X86Fp, X86Fp)                                    // FPU
  ASMJIT_INST_1x(fsub, Fsub, X86Mem)                                          // FPU
  ASMJIT_INST_1x(fsubp, Fsubp, X86Fp)                                         // FPU
  ASMJIT_INST_0x(fsubp, Fsubp)                                                // FPU
  ASMJIT_INST_2x(fsubr, Fsubr, X86Fp, X86Fp)                                  // FPU
  ASMJIT_INST_1x(fsubr, Fsubr, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fsubrp, Fsubrp, X86Fp)                                       // FPU
  ASMJIT_INST_0x(fsubrp, Fsubrp)                                              // FPU
  ASMJIT_INST_0x(ftst, Ftst)                                                  // FPU
  ASMJIT_INST_1x(fucom, Fucom, X86Fp)                                         // FPU
  ASMJIT_INST_0x(fucom, Fucom)                                                // FPU
  ASMJIT_INST_1x(fucomi, Fucomi, X86Fp)                                       // FPU
  ASMJIT_INST_1x(fucomip, Fucomip, X86Fp)                                     // FPU
  ASMJIT_INST_1x(fucomp, Fucomp, X86Fp)                                       // FPU
  ASMJIT_INST_0x(fucomp, Fucomp)                                              // FPU
  ASMJIT_INST_0x(fucompp, Fucompp)                                            // FPU
  ASMJIT_INST_0x(fwait, Fwait)                                                // FPU
  ASMJIT_INST_0x(fxam, Fxam)                                                  // FPU
  ASMJIT_INST_1x(fxch, Fxch, X86Fp)                                           // FPU
  ASMJIT_INST_0x(fxtract, Fxtract)                                            // FPU
  ASMJIT_INST_0x(fyl2x, Fyl2x)                                                // FPU
  ASMJIT_INST_0x(fyl2xp1, Fyl2xp1)                                            // FPU
  ASMJIT_INST_1x(fstsw, Fstsw, X86Gp)                                         // FPU
  ASMJIT_INST_1x(fstsw, Fstsw, X86Mem)                                        // FPU
  ASMJIT_INST_1x(fnstsw, Fnstsw, X86Gp)                                       // FPU
  ASMJIT_INST_1x(fnstsw, Fnstsw, X86Mem)                                      // FPU

  // --------------------------------------------------------------------------
  // [MMX & SSE Instructions]
  // --------------------------------------------------------------------------

  ASMJIT_INST_2x(addpd, Addpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(addpd, Addpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(addps, Addps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(addps, Addps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(addsd, Addsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(addsd, Addsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(addss, Addss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(addss, Addss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(addsubpd, Addsubpd, X86Xmm, X86Xmm)                          // SSE3
  ASMJIT_INST_2x(addsubpd, Addsubpd, X86Xmm, X86Mem)                          // SSE3
  ASMJIT_INST_2x(addsubps, Addsubps, X86Xmm, X86Xmm)                          // SSE3
  ASMJIT_INST_2x(addsubps, Addsubps, X86Xmm, X86Mem)                          // SSE3
  ASMJIT_INST_2x(andnpd, Andnpd, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(andnpd, Andnpd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(andnps, Andnps, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(andnps, Andnps, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(andpd, Andpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(andpd, Andpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(andps, Andps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(andps, Andps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_3i(blendpd, Blendpd, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(blendpd, Blendpd, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_3i(blendps, Blendps, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(blendps, Blendps, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_3x(blendvpd, Blendvpd, X86Xmm, X86Xmm, XMM0)                    // SSE4_1 [EXPLICIT]
  ASMJIT_INST_3x(blendvpd, Blendvpd, X86Xmm, X86Mem, XMM0)                    // SSE4_1 [EXPLICIT]
  ASMJIT_INST_3x(blendvps, Blendvps, X86Xmm, X86Xmm, XMM0)                    // SSE4_1 [EXPLICIT]
  ASMJIT_INST_3x(blendvps, Blendvps, X86Xmm, X86Mem, XMM0)                    // SSE4_1 [EXPLICIT]
  ASMJIT_INST_3i(cmppd, Cmppd, X86Xmm, X86Xmm, Imm)                           // SSE2
  ASMJIT_INST_3i(cmppd, Cmppd, X86Xmm, X86Mem, Imm)                           // SSE2
  ASMJIT_INST_3i(cmpps, Cmpps, X86Xmm, X86Xmm, Imm)                           // SSE
  ASMJIT_INST_3i(cmpps, Cmpps, X86Xmm, X86Mem, Imm)                           // SSE
  ASMJIT_INST_3i(cmpsd, Cmpsd, X86Xmm, X86Xmm, Imm)                           // SSE2
  ASMJIT_INST_3i(cmpsd, Cmpsd, X86Xmm, X86Mem, Imm)                           // SSE2
  ASMJIT_INST_3i(cmpss, Cmpss, X86Xmm, X86Xmm, Imm)                           // SSE
  ASMJIT_INST_3i(cmpss, Cmpss, X86Xmm, X86Mem, Imm)                           // SSE
  ASMJIT_INST_2x(comisd, Comisd, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(comisd, Comisd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(comiss, Comiss, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(comiss, Comiss, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(cvtdq2pd, Cvtdq2pd, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtdq2pd, Cvtdq2pd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtdq2ps, Cvtdq2ps, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtdq2ps, Cvtdq2ps, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtpd2dq, Cvtpd2dq, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtpd2dq, Cvtpd2dq, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtpd2pi, Cvtpd2pi, X86Mm, X86Xmm)                           // SSE2
  ASMJIT_INST_2x(cvtpd2pi, Cvtpd2pi, X86Mm, X86Mem)                           // SSE2
  ASMJIT_INST_2x(cvtpd2ps, Cvtpd2ps, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtpd2ps, Cvtpd2ps, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtpi2pd, Cvtpi2pd, X86Xmm, X86Mm)                           // SSE2
  ASMJIT_INST_2x(cvtpi2pd, Cvtpi2pd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtpi2ps, Cvtpi2ps, X86Xmm, X86Mm)                           // SSE
  ASMJIT_INST_2x(cvtpi2ps, Cvtpi2ps, X86Xmm, X86Mem)                          // SSE
  ASMJIT_INST_2x(cvtps2dq, Cvtps2dq, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtps2dq, Cvtps2dq, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtps2pd, Cvtps2pd, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtps2pd, Cvtps2pd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtps2pi, Cvtps2pi, X86Mm, X86Xmm)                           // SSE
  ASMJIT_INST_2x(cvtps2pi, Cvtps2pi, X86Mm, X86Mem)                           // SSE
  ASMJIT_INST_2x(cvtsd2si, Cvtsd2si, X86Gp, X86Xmm)                           // SSE2
  ASMJIT_INST_2x(cvtsd2si, Cvtsd2si, X86Gp, X86Mem)                           // SSE2
  ASMJIT_INST_2x(cvtsd2ss, Cvtsd2ss, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtsd2ss, Cvtsd2ss, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtsi2sd, Cvtsi2sd, X86Xmm, X86Gp)                           // SSE2
  ASMJIT_INST_2x(cvtsi2sd, Cvtsi2sd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtsi2ss, Cvtsi2ss, X86Xmm, X86Gp)                           // SSE
  ASMJIT_INST_2x(cvtsi2ss, Cvtsi2ss, X86Xmm, X86Mem)                          // SSE
  ASMJIT_INST_2x(cvtss2sd, Cvtss2sd, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(cvtss2sd, Cvtss2sd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(cvtss2si, Cvtss2si, X86Gp, X86Xmm)                           // SSE
  ASMJIT_INST_2x(cvtss2si, Cvtss2si, X86Gp, X86Mem)                           // SSE
  ASMJIT_INST_2x(cvttpd2pi, Cvttpd2pi, X86Mm, X86Xmm)                         // SSE2
  ASMJIT_INST_2x(cvttpd2pi, Cvttpd2pi, X86Mm, X86Mem)                         // SSE2
  ASMJIT_INST_2x(cvttpd2dq, Cvttpd2dq, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(cvttpd2dq, Cvttpd2dq, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(cvttps2dq, Cvttps2dq, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(cvttps2dq, Cvttps2dq, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(cvttps2pi, Cvttps2pi, X86Mm, X86Xmm)                         // SSE
  ASMJIT_INST_2x(cvttps2pi, Cvttps2pi, X86Mm, X86Mem)                         // SSE
  ASMJIT_INST_2x(cvttsd2si, Cvttsd2si, X86Gp, X86Xmm)                         // SSE2
  ASMJIT_INST_2x(cvttsd2si, Cvttsd2si, X86Gp, X86Mem)                         // SSE2
  ASMJIT_INST_2x(cvttss2si, Cvttss2si, X86Gp, X86Xmm)                         // SSE
  ASMJIT_INST_2x(cvttss2si, Cvttss2si, X86Gp, X86Mem)                         // SSE
  ASMJIT_INST_2x(divpd, Divpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(divpd, Divpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(divps, Divps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(divps, Divps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(divsd, Divsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(divsd, Divsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(divss, Divss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(divss, Divss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_3i(dppd, Dppd, X86Xmm, X86Xmm, Imm)                             // SSE4_1
  ASMJIT_INST_3i(dppd, Dppd, X86Xmm, X86Mem, Imm)                             // SSE4_1
  ASMJIT_INST_3i(dpps, Dpps, X86Xmm, X86Xmm, Imm)                             // SSE4_1
  ASMJIT_INST_3i(dpps, Dpps, X86Xmm, X86Mem, Imm)                             // SSE4_1
  ASMJIT_INST_3i(extractps, Extractps, X86Gp, X86Xmm, Imm)                    // SSE4_1
  ASMJIT_INST_3i(extractps, Extractps, X86Mem, X86Xmm, Imm)                   // SSE4_1
  ASMJIT_INST_2x(extrq, Extrq, X86Xmm, X86Xmm)                                // SSE4A
  ASMJIT_INST_3ii(extrq, Extrq, X86Xmm, Imm, Imm)                             // SSE4A
  ASMJIT_INST_2x(haddpd, Haddpd, X86Xmm, X86Xmm)                              // SSE3
  ASMJIT_INST_2x(haddpd, Haddpd, X86Xmm, X86Mem)                              // SSE3
  ASMJIT_INST_2x(haddps, Haddps, X86Xmm, X86Xmm)                              // SSE3
  ASMJIT_INST_2x(haddps, Haddps, X86Xmm, X86Mem)                              // SSE3
  ASMJIT_INST_2x(hsubpd, Hsubpd, X86Xmm, X86Xmm)                              // SSE3
  ASMJIT_INST_2x(hsubpd, Hsubpd, X86Xmm, X86Mem)                              // SSE3
  ASMJIT_INST_2x(hsubps, Hsubps, X86Xmm, X86Xmm)                              // SSE3
  ASMJIT_INST_2x(hsubps, Hsubps, X86Xmm, X86Mem)                              // SSE3
  ASMJIT_INST_3i(insertps, Insertps, X86Xmm, X86Xmm, Imm)                     // SSE4_1
  ASMJIT_INST_3i(insertps, Insertps, X86Xmm, X86Mem, Imm)                     // SSE4_1
  ASMJIT_INST_2x(insertq, Insertq, X86Xmm, X86Xmm)                            // SSE4A
  ASMJIT_INST_4ii(insertq, Insertq, X86Xmm, X86Xmm, Imm, Imm)                 // SSE4A
  ASMJIT_INST_2x(lddqu, Lddqu, X86Xmm, X86Mem)                                // SSE3
  ASMJIT_INST_3x(maskmovq, Maskmovq, X86Mm, X86Mm, DS_ZDI)                    // SSE  [EXPLICIT]
  ASMJIT_INST_3x(maskmovdqu, Maskmovdqu, X86Xmm, X86Xmm, DS_ZDI)              // SSE2 [EXPLICIT]
  ASMJIT_INST_2x(maxpd, Maxpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(maxpd, Maxpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(maxps, Maxps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(maxps, Maxps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(maxsd, Maxsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(maxsd, Maxsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(maxss, Maxss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(maxss, Maxss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(minpd, Minpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(minpd, Minpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(minps, Minps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(minps, Minps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(minsd, Minsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(minsd, Minsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(minss, Minss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(minss, Minss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(movapd, Movapd, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movapd, Movapd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(movapd, Movapd, X86Mem, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movaps, Movaps, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(movaps, Movaps, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(movaps, Movaps, X86Mem, X86Xmm)                              // SSE
  ASMJIT_INST_2x(movd, Movd, X86Mem, X86Mm)                                   // MMX
  ASMJIT_INST_2x(movd, Movd, X86Mem, X86Xmm)                                  // SSE
  ASMJIT_INST_2x(movd, Movd, X86Gp, X86Mm)                                    // MMX
  ASMJIT_INST_2x(movd, Movd, X86Gp, X86Xmm)                                   // SSE
  ASMJIT_INST_2x(movd, Movd, X86Mm, X86Mem)                                   // MMX
  ASMJIT_INST_2x(movd, Movd, X86Xmm, X86Mem)                                  // SSE
  ASMJIT_INST_2x(movd, Movd, X86Mm, X86Gp)                                    // MMX
  ASMJIT_INST_2x(movd, Movd, X86Xmm, X86Gp)                                   // SSE
  ASMJIT_INST_2x(movddup, Movddup, X86Xmm, X86Xmm)                            // SSE3
  ASMJIT_INST_2x(movddup, Movddup, X86Xmm, X86Mem)                            // SSE3
  ASMJIT_INST_2x(movdq2q, Movdq2q, X86Mm, X86Xmm)                             // SSE2
  ASMJIT_INST_2x(movdqa, Movdqa, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movdqa, Movdqa, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(movdqa, Movdqa, X86Mem, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movdqu, Movdqu, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movdqu, Movdqu, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(movdqu, Movdqu, X86Mem, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movhlps, Movhlps, X86Xmm, X86Xmm)                            // SSE
  ASMJIT_INST_2x(movhpd, Movhpd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(movhpd, Movhpd, X86Mem, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movhps, Movhps, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(movhps, Movhps, X86Mem, X86Xmm)                              // SSE
  ASMJIT_INST_2x(movlhps, Movlhps, X86Xmm, X86Xmm)                            // SSE
  ASMJIT_INST_2x(movlpd, Movlpd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(movlpd, Movlpd, X86Mem, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movlps, Movlps, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(movlps, Movlps, X86Mem, X86Xmm)                              // SSE
  ASMJIT_INST_2x(movmskps, Movmskps, X86Gp, X86Xmm)                           // SSE2
  ASMJIT_INST_2x(movmskpd, Movmskpd, X86Gp, X86Xmm)                           // SSE2
  ASMJIT_INST_2x(movntdq, Movntdq, X86Mem, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(movntdqa, Movntdqa, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(movntpd, Movntpd, X86Mem, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(movntps, Movntps, X86Mem, X86Xmm)                            // SSE
  ASMJIT_INST_2x(movntsd, Movntsd, X86Mem, X86Xmm)                            // SSE4A
  ASMJIT_INST_2x(movntss, Movntss, X86Mem, X86Xmm)                            // SSE4A
  ASMJIT_INST_2x(movntq, Movntq, X86Mem, X86Mm)                               // SSE
  ASMJIT_INST_2x(movq, Movq, X86Mm, X86Mm)                                    // MMX
  ASMJIT_INST_2x(movq, Movq, X86Xmm, X86Xmm)                                  // SSE
  ASMJIT_INST_2x(movq, Movq, X86Mem, X86Mm)                                   // MMX
  ASMJIT_INST_2x(movq, Movq, X86Mem, X86Xmm)                                  // SSE
  ASMJIT_INST_2x(movq, Movq, X86Mm, X86Mem)                                   // MMX
  ASMJIT_INST_2x(movq, Movq, X86Xmm, X86Mem)                                  // SSE
  ASMJIT_INST_2x(movq, Movq, X86Gp, X86Mm)                                    // MMX
  ASMJIT_INST_2x(movq, Movq, X86Gp, X86Xmm)                                   // SSE+X64.
  ASMJIT_INST_2x(movq, Movq, X86Mm, X86Gp)                                    // MMX
  ASMJIT_INST_2x(movq, Movq, X86Xmm, X86Gp)                                   // SSE+X64.
  ASMJIT_INST_2x(movq2dq, Movq2dq, X86Xmm, X86Mm)                             // SSE2
  ASMJIT_INST_2x(movsd, Movsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(movsd, Movsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(movsd, Movsd, X86Mem, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(movshdup, Movshdup, X86Xmm, X86Xmm)                          // SSE3
  ASMJIT_INST_2x(movshdup, Movshdup, X86Xmm, X86Mem)                          // SSE3
  ASMJIT_INST_2x(movsldup, Movsldup, X86Xmm, X86Xmm)                          // SSE3
  ASMJIT_INST_2x(movsldup, Movsldup, X86Xmm, X86Mem)                          // SSE3
  ASMJIT_INST_2x(movss, Movss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(movss, Movss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(movss, Movss, X86Mem, X86Xmm)                                // SSE
  ASMJIT_INST_2x(movupd, Movupd, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movupd, Movupd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(movupd, Movupd, X86Mem, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(movups, Movups, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(movups, Movups, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(movups, Movups, X86Mem, X86Xmm)                              // SSE
  ASMJIT_INST_3i(mpsadbw, Mpsadbw, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(mpsadbw, Mpsadbw, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_2x(mulpd, Mulpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(mulpd, Mulpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(mulps, Mulps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(mulps, Mulps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(mulsd, Mulsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(mulsd, Mulsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(mulss, Mulss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(mulss, Mulss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(orpd, Orpd, X86Xmm, X86Xmm)                                  // SSE2
  ASMJIT_INST_2x(orpd, Orpd, X86Xmm, X86Mem)                                  // SSE2
  ASMJIT_INST_2x(orps, Orps, X86Xmm, X86Xmm)                                  // SSE
  ASMJIT_INST_2x(orps, Orps, X86Xmm, X86Mem)                                  // SSE
  ASMJIT_INST_2x(packssdw, Packssdw, X86Mm, X86Mm)                            // MMX
  ASMJIT_INST_2x(packssdw, Packssdw, X86Mm, X86Mem)                           // MMX
  ASMJIT_INST_2x(packssdw, Packssdw, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(packssdw, Packssdw, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(packsswb, Packsswb, X86Mm, X86Mm)                            // MMX
  ASMJIT_INST_2x(packsswb, Packsswb, X86Mm, X86Mem)                           // MMX
  ASMJIT_INST_2x(packsswb, Packsswb, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(packsswb, Packsswb, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(packusdw, Packusdw, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(packusdw, Packusdw, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(packuswb, Packuswb, X86Mm, X86Mm)                            // MMX
  ASMJIT_INST_2x(packuswb, Packuswb, X86Mm, X86Mem)                           // MMX
  ASMJIT_INST_2x(packuswb, Packuswb, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(packuswb, Packuswb, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(pabsb, Pabsb, X86Mm, X86Mm)                                  // SSSE3
  ASMJIT_INST_2x(pabsb, Pabsb, X86Mm, X86Mem)                                 // SSSE3
  ASMJIT_INST_2x(pabsb, Pabsb, X86Xmm, X86Xmm)                                // SSSE3
  ASMJIT_INST_2x(pabsb, Pabsb, X86Xmm, X86Mem)                                // SSSE3
  ASMJIT_INST_2x(pabsd, Pabsd, X86Mm, X86Mm)                                  // SSSE3
  ASMJIT_INST_2x(pabsd, Pabsd, X86Mm, X86Mem)                                 // SSSE3
  ASMJIT_INST_2x(pabsd, Pabsd, X86Xmm, X86Xmm)                                // SSSE3
  ASMJIT_INST_2x(pabsd, Pabsd, X86Xmm, X86Mem)                                // SSSE3
  ASMJIT_INST_2x(pabsw, Pabsw, X86Mm, X86Mm)                                  // SSSE3
  ASMJIT_INST_2x(pabsw, Pabsw, X86Mm, X86Mem)                                 // SSSE3
  ASMJIT_INST_2x(pabsw, Pabsw, X86Xmm, X86Xmm)                                // SSSE3
  ASMJIT_INST_2x(pabsw, Pabsw, X86Xmm, X86Mem)                                // SSSE3
  ASMJIT_INST_2x(paddb, Paddb, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(paddb, Paddb, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(paddb, Paddb, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(paddb, Paddb, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(paddd, Paddd, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(paddd, Paddd, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(paddd, Paddd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(paddd, Paddd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(paddq, Paddq, X86Mm, X86Mm)                                  // SSE2
  ASMJIT_INST_2x(paddq, Paddq, X86Mm, X86Mem)                                 // SSE2
  ASMJIT_INST_2x(paddq, Paddq, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(paddq, Paddq, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(paddsb, Paddsb, X86Mm, X86Mm)                                // MMX
  ASMJIT_INST_2x(paddsb, Paddsb, X86Mm, X86Mem)                               // MMX
  ASMJIT_INST_2x(paddsb, Paddsb, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(paddsb, Paddsb, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(paddsw, Paddsw, X86Mm, X86Mm)                                // MMX
  ASMJIT_INST_2x(paddsw, Paddsw, X86Mm, X86Mem)                               // MMX
  ASMJIT_INST_2x(paddsw, Paddsw, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(paddsw, Paddsw, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(paddusb, Paddusb, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(paddusb, Paddusb, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(paddusb, Paddusb, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(paddusb, Paddusb, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(paddusw, Paddusw, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(paddusw, Paddusw, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(paddusw, Paddusw, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(paddusw, Paddusw, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(paddw, Paddw, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(paddw, Paddw, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(paddw, Paddw, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(paddw, Paddw, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_3i(palignr, Palignr, X86Mm, X86Mm, Imm)                         // SSSE3
  ASMJIT_INST_3i(palignr, Palignr, X86Mm, X86Mem, Imm)                        // SSSE3
  ASMJIT_INST_3i(palignr, Palignr, X86Xmm, X86Xmm, Imm)                       // SSSE3
  ASMJIT_INST_3i(palignr, Palignr, X86Xmm, X86Mem, Imm)                       // SSSE3
  ASMJIT_INST_2x(pand, Pand, X86Mm, X86Mm)                                    // MMX
  ASMJIT_INST_2x(pand, Pand, X86Mm, X86Mem)                                   // MMX
  ASMJIT_INST_2x(pand, Pand, X86Xmm, X86Xmm)                                  // SSE2
  ASMJIT_INST_2x(pand, Pand, X86Xmm, X86Mem)                                  // SSE2
  ASMJIT_INST_2x(pandn, Pandn, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(pandn, Pandn, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(pandn, Pandn, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(pandn, Pandn, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(pavgb, Pavgb, X86Mm, X86Mm)                                  // SSE
  ASMJIT_INST_2x(pavgb, Pavgb, X86Mm, X86Mem)                                 // SSE
  ASMJIT_INST_2x(pavgb, Pavgb, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(pavgb, Pavgb, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(pavgw, Pavgw, X86Mm, X86Mm)                                  // SSE
  ASMJIT_INST_2x(pavgw, Pavgw, X86Mm, X86Mem)                                 // SSE
  ASMJIT_INST_2x(pavgw, Pavgw, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(pavgw, Pavgw, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_3x(pblendvb, Pblendvb, X86Xmm, X86Xmm, XMM0)                    // SSE4_1 [EXPLICIT]
  ASMJIT_INST_3x(pblendvb, Pblendvb, X86Xmm, X86Mem, XMM0)                    // SSE4_1 [EXPLICIT]
  ASMJIT_INST_3i(pblendw, Pblendw, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(pblendw, Pblendw, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_3i(pclmulqdq, Pclmulqdq, X86Xmm, X86Xmm, Imm)                   // PCLMULQDQ.
  ASMJIT_INST_3i(pclmulqdq, Pclmulqdq, X86Xmm, X86Mem, Imm)                   // PCLMULQDQ.
  ASMJIT_INST_6x(pcmpestri, Pcmpestri, X86Xmm, X86Xmm, Imm, ECX, EAX, EDX)    // SSE4_2 [EXPLICIT]
  ASMJIT_INST_6x(pcmpestri, Pcmpestri, X86Xmm, X86Mem, Imm, ECX, EAX, EDX)    // SSE4_2 [EXPLICIT]
  ASMJIT_INST_6x(pcmpestrm, Pcmpestrm, X86Xmm, X86Xmm, Imm, XMM0, EAX, EDX)   // SSE4_2 [EXPLICIT]
  ASMJIT_INST_6x(pcmpestrm, Pcmpestrm, X86Xmm, X86Mem, Imm, XMM0, EAX, EDX)   // SSE4_2 [EXPLICIT]
  ASMJIT_INST_2x(pcmpeqb, Pcmpeqb, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pcmpeqb, Pcmpeqb, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pcmpeqb, Pcmpeqb, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pcmpeqb, Pcmpeqb, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pcmpeqd, Pcmpeqd, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pcmpeqd, Pcmpeqd, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pcmpeqd, Pcmpeqd, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pcmpeqd, Pcmpeqd, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pcmpeqq, Pcmpeqq, X86Xmm, X86Xmm)                            // SSE4_1
  ASMJIT_INST_2x(pcmpeqq, Pcmpeqq, X86Xmm, X86Mem)                            // SSE4_1
  ASMJIT_INST_2x(pcmpeqw, Pcmpeqw, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pcmpeqw, Pcmpeqw, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pcmpeqw, Pcmpeqw, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pcmpeqw, Pcmpeqw, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pcmpgtb, Pcmpgtb, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pcmpgtb, Pcmpgtb, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pcmpgtb, Pcmpgtb, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pcmpgtb, Pcmpgtb, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pcmpgtd, Pcmpgtd, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pcmpgtd, Pcmpgtd, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pcmpgtd, Pcmpgtd, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pcmpgtd, Pcmpgtd, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pcmpgtq, Pcmpgtq, X86Xmm, X86Xmm)                            // SSE4_2.
  ASMJIT_INST_2x(pcmpgtq, Pcmpgtq, X86Xmm, X86Mem)                            // SSE4_2.
  ASMJIT_INST_2x(pcmpgtw, Pcmpgtw, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pcmpgtw, Pcmpgtw, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pcmpgtw, Pcmpgtw, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pcmpgtw, Pcmpgtw, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_4x(pcmpistri, Pcmpistri, X86Xmm, X86Xmm, Imm, ECX)              // SSE4_2 [EXPLICIT]
  ASMJIT_INST_4x(pcmpistri, Pcmpistri, X86Xmm, X86Mem, Imm, ECX)              // SSE4_2 [EXPLICIT]
  ASMJIT_INST_4x(pcmpistrm, Pcmpistrm, X86Xmm, X86Xmm, Imm, XMM0)             // SSE4_2 [EXPLICIT]
  ASMJIT_INST_4x(pcmpistrm, Pcmpistrm, X86Xmm, X86Mem, Imm, XMM0)             // SSE4_2 [EXPLICIT]
  ASMJIT_INST_3i(pextrb, Pextrb, X86Gp, X86Xmm, Imm)                          // SSE4_1
  ASMJIT_INST_3i(pextrb, Pextrb, X86Mem, X86Xmm, Imm)                         // SSE4_1
  ASMJIT_INST_3i(pextrd, Pextrd, X86Gp, X86Xmm, Imm)                          // SSE4_1
  ASMJIT_INST_3i(pextrd, Pextrd, X86Mem, X86Xmm, Imm)                         // SSE4_1
  ASMJIT_INST_3i(pextrq, Pextrq, X86Gp, X86Xmm, Imm)                          // SSE4_1
  ASMJIT_INST_3i(pextrq, Pextrq, X86Mem, X86Xmm, Imm)                         // SSE4_1
  ASMJIT_INST_3i(pextrw, Pextrw, X86Gp, X86Mm, Imm)                           // SSE
  ASMJIT_INST_3i(pextrw, Pextrw, X86Gp, X86Xmm, Imm)                          // SSE2
  ASMJIT_INST_3i(pextrw, Pextrw, X86Mem, X86Xmm, Imm)                         // SSE4_1
  ASMJIT_INST_2x(phaddd, Phaddd, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(phaddd, Phaddd, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(phaddd, Phaddd, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(phaddd, Phaddd, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_2x(phaddsw, Phaddsw, X86Mm, X86Mm)                              // SSSE3
  ASMJIT_INST_2x(phaddsw, Phaddsw, X86Mm, X86Mem)                             // SSSE3
  ASMJIT_INST_2x(phaddsw, Phaddsw, X86Xmm, X86Xmm)                            // SSSE3
  ASMJIT_INST_2x(phaddsw, Phaddsw, X86Xmm, X86Mem)                            // SSSE3
  ASMJIT_INST_2x(phaddw, Phaddw, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(phaddw, Phaddw, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(phaddw, Phaddw, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(phaddw, Phaddw, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_2x(phminposuw, Phminposuw, X86Xmm, X86Xmm)                      // SSE4_1
  ASMJIT_INST_2x(phminposuw, Phminposuw, X86Xmm, X86Mem)                      // SSE4_1
  ASMJIT_INST_2x(phsubd, Phsubd, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(phsubd, Phsubd, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(phsubd, Phsubd, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(phsubd, Phsubd, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_2x(phsubsw, Phsubsw, X86Mm, X86Mm)                              // SSSE3
  ASMJIT_INST_2x(phsubsw, Phsubsw, X86Mm, X86Mem)                             // SSSE3
  ASMJIT_INST_2x(phsubsw, Phsubsw, X86Xmm, X86Xmm)                            // SSSE3
  ASMJIT_INST_2x(phsubsw, Phsubsw, X86Xmm, X86Mem)                            // SSSE3
  ASMJIT_INST_2x(phsubw, Phsubw, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(phsubw, Phsubw, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(phsubw, Phsubw, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(phsubw, Phsubw, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_3i(pinsrb, Pinsrb, X86Xmm, X86Gp, Imm)                          // SSE4_1
  ASMJIT_INST_3i(pinsrb, Pinsrb, X86Xmm, X86Mem, Imm)                         // SSE4_1
  ASMJIT_INST_3i(pinsrd, Pinsrd, X86Xmm, X86Gp, Imm)                          // SSE4_1
  ASMJIT_INST_3i(pinsrd, Pinsrd, X86Xmm, X86Mem, Imm)                         // SSE4_1
  ASMJIT_INST_3i(pinsrq, Pinsrq, X86Xmm, X86Gp, Imm)                          // SSE4_1
  ASMJIT_INST_3i(pinsrq, Pinsrq, X86Xmm, X86Mem, Imm)                         // SSE4_1
  ASMJIT_INST_3i(pinsrw, Pinsrw, X86Mm, X86Gp, Imm)                           // SSE
  ASMJIT_INST_3i(pinsrw, Pinsrw, X86Mm, X86Mem, Imm)                          // SSE
  ASMJIT_INST_3i(pinsrw, Pinsrw, X86Xmm, X86Gp, Imm)                          // SSE2
  ASMJIT_INST_3i(pinsrw, Pinsrw, X86Xmm, X86Mem, Imm)                         // SSE2
  ASMJIT_INST_2x(pmaddubsw, Pmaddubsw, X86Mm, X86Mm)                          // SSSE3
  ASMJIT_INST_2x(pmaddubsw, Pmaddubsw, X86Mm, X86Mem)                         // SSSE3
  ASMJIT_INST_2x(pmaddubsw, Pmaddubsw, X86Xmm, X86Xmm)                        // SSSE3
  ASMJIT_INST_2x(pmaddubsw, Pmaddubsw, X86Xmm, X86Mem)                        // SSSE3
  ASMJIT_INST_2x(pmaddwd, Pmaddwd, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(pmaddwd, Pmaddwd, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(pmaddwd, Pmaddwd, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pmaddwd, Pmaddwd, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pmaxsb, Pmaxsb, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pmaxsb, Pmaxsb, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pmaxsd, Pmaxsd, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pmaxsd, Pmaxsd, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pmaxsw, Pmaxsw, X86Mm, X86Mm)                                // SSE
  ASMJIT_INST_2x(pmaxsw, Pmaxsw, X86Mm, X86Mem)                               // SSE
  ASMJIT_INST_2x(pmaxsw, Pmaxsw, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(pmaxsw, Pmaxsw, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(pmaxub, Pmaxub, X86Mm, X86Mm)                                // SSE
  ASMJIT_INST_2x(pmaxub, Pmaxub, X86Mm, X86Mem)                               // SSE
  ASMJIT_INST_2x(pmaxub, Pmaxub, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(pmaxub, Pmaxub, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(pmaxud, Pmaxud, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pmaxud, Pmaxud, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pmaxuw, Pmaxuw, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pmaxuw, Pmaxuw, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pminsb, Pminsb, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pminsb, Pminsb, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pminsd, Pminsd, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pminsd, Pminsd, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pminsw, Pminsw, X86Mm, X86Mm)                                // SSE
  ASMJIT_INST_2x(pminsw, Pminsw, X86Mm, X86Mem)                               // SSE
  ASMJIT_INST_2x(pminsw, Pminsw, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(pminsw, Pminsw, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(pminub, Pminub, X86Mm, X86Mm)                                // SSE
  ASMJIT_INST_2x(pminub, Pminub, X86Mm, X86Mem)                               // SSE
  ASMJIT_INST_2x(pminub, Pminub, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(pminub, Pminub, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(pminud, Pminud, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pminud, Pminud, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pminuw, Pminuw, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pminuw, Pminuw, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pmovmskb, Pmovmskb, X86Gp, X86Mm)                            // SSE
  ASMJIT_INST_2x(pmovmskb, Pmovmskb, X86Gp, X86Xmm)                           // SSE2
  ASMJIT_INST_2x(pmovsxbd, Pmovsxbd, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxbd, Pmovsxbd, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxbq, Pmovsxbq, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxbq, Pmovsxbq, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxbw, Pmovsxbw, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxbw, Pmovsxbw, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxdq, Pmovsxdq, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxdq, Pmovsxdq, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxwd, Pmovsxwd, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxwd, Pmovsxwd, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxwq, Pmovsxwq, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovsxwq, Pmovsxwq, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxbd, Pmovzxbd, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxbd, Pmovzxbd, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxbq, Pmovzxbq, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxbq, Pmovzxbq, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxbw, Pmovzxbw, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxbw, Pmovzxbw, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxdq, Pmovzxdq, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxdq, Pmovzxdq, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxwd, Pmovzxwd, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxwd, Pmovzxwd, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxwq, Pmovzxwq, X86Xmm, X86Xmm)                          // SSE4_1
  ASMJIT_INST_2x(pmovzxwq, Pmovzxwq, X86Xmm, X86Mem)                          // SSE4_1
  ASMJIT_INST_2x(pmuldq, Pmuldq, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pmuldq, Pmuldq, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pmulhrsw, Pmulhrsw, X86Mm, X86Mm)                            // SSSE3
  ASMJIT_INST_2x(pmulhrsw, Pmulhrsw, X86Mm, X86Mem)                           // SSSE3
  ASMJIT_INST_2x(pmulhrsw, Pmulhrsw, X86Xmm, X86Xmm)                          // SSSE3
  ASMJIT_INST_2x(pmulhrsw, Pmulhrsw, X86Xmm, X86Mem)                          // SSSE3
  ASMJIT_INST_2x(pmulhw, Pmulhw, X86Mm, X86Mm)                                // MMX
  ASMJIT_INST_2x(pmulhw, Pmulhw, X86Mm, X86Mem)                               // MMX
  ASMJIT_INST_2x(pmulhw, Pmulhw, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(pmulhw, Pmulhw, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(pmulhuw, Pmulhuw, X86Mm, X86Mm)                              // SSE
  ASMJIT_INST_2x(pmulhuw, Pmulhuw, X86Mm, X86Mem)                             // SSE
  ASMJIT_INST_2x(pmulhuw, Pmulhuw, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pmulhuw, Pmulhuw, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(pmulld, Pmulld, X86Xmm, X86Xmm)                              // SSE4_1
  ASMJIT_INST_2x(pmulld, Pmulld, X86Xmm, X86Mem)                              // SSE4_1
  ASMJIT_INST_2x(pmullw, Pmullw, X86Mm, X86Mm)                                // MMX
  ASMJIT_INST_2x(pmullw, Pmullw, X86Mm, X86Mem)                               // MMX
  ASMJIT_INST_2x(pmullw, Pmullw, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(pmullw, Pmullw, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(pmuludq, Pmuludq, X86Mm, X86Mm)                              // SSE2
  ASMJIT_INST_2x(pmuludq, Pmuludq, X86Mm, X86Mem)                             // SSE2
  ASMJIT_INST_2x(pmuludq, Pmuludq, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(pmuludq, Pmuludq, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(por, Por, X86Mm, X86Mm)                                      // MMX
  ASMJIT_INST_2x(por, Por, X86Mm, X86Mem)                                     // MMX
  ASMJIT_INST_2x(por, Por, X86Xmm, X86Xmm)                                    // SSE2
  ASMJIT_INST_2x(por, Por, X86Xmm, X86Mem)                                    // SSE2
  ASMJIT_INST_2x(psadbw, Psadbw, X86Mm, X86Mm)                                // SSE
  ASMJIT_INST_2x(psadbw, Psadbw, X86Mm, X86Mem)                               // SSE
  ASMJIT_INST_2x(psadbw, Psadbw, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(psadbw, Psadbw, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(pslld, Pslld, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(pslld, Pslld, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(pslld, Pslld, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(pslld, Pslld, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(pslld, Pslld, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(pslld, Pslld, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2i(pslldq, Pslldq, X86Xmm, Imm)                                 // SSE2
  ASMJIT_INST_2x(psllq, Psllq, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psllq, Psllq, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psllq, Psllq, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psllq, Psllq, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psllq, Psllq, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psllq, Psllq, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2x(psllw, Psllw, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psllw, Psllw, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psllw, Psllw, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psllw, Psllw, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psllw, Psllw, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psllw, Psllw, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2x(psrad, Psrad, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psrad, Psrad, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psrad, Psrad, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psrad, Psrad, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psrad, Psrad, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psrad, Psrad, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2x(psraw, Psraw, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psraw, Psraw, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psraw, Psraw, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psraw, Psraw, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psraw, Psraw, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psraw, Psraw, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2x(pshufb, Pshufb, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(pshufb, Pshufb, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(pshufb, Pshufb, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(pshufb, Pshufb, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_3i(pshufd, Pshufd, X86Xmm, X86Xmm, Imm)                         // SSE2
  ASMJIT_INST_3i(pshufd, Pshufd, X86Xmm, X86Mem, Imm)                         // SSE2
  ASMJIT_INST_3i(pshufhw, Pshufhw, X86Xmm, X86Xmm, Imm)                       // SSE2
  ASMJIT_INST_3i(pshufhw, Pshufhw, X86Xmm, X86Mem, Imm)                       // SSE2
  ASMJIT_INST_3i(pshuflw, Pshuflw, X86Xmm, X86Xmm, Imm)                       // SSE2
  ASMJIT_INST_3i(pshuflw, Pshuflw, X86Xmm, X86Mem, Imm)                       // SSE2
  ASMJIT_INST_3i(pshufw, Pshufw, X86Mm, X86Mm, Imm)                           // SSE
  ASMJIT_INST_3i(pshufw, Pshufw, X86Mm, X86Mem, Imm)                          // SSE
  ASMJIT_INST_2x(psignb, Psignb, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(psignb, Psignb, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(psignb, Psignb, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(psignb, Psignb, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_2x(psignd, Psignd, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(psignd, Psignd, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(psignd, Psignd, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(psignd, Psignd, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_2x(psignw, Psignw, X86Mm, X86Mm)                                // SSSE3
  ASMJIT_INST_2x(psignw, Psignw, X86Mm, X86Mem)                               // SSSE3
  ASMJIT_INST_2x(psignw, Psignw, X86Xmm, X86Xmm)                              // SSSE3
  ASMJIT_INST_2x(psignw, Psignw, X86Xmm, X86Mem)                              // SSSE3
  ASMJIT_INST_2x(psrld, Psrld, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psrld, Psrld, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psrld, Psrld, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psrld, Psrld, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psrld, Psrld, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psrld, Psrld, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2i(psrldq, Psrldq, X86Xmm, Imm)                                 // SSE2
  ASMJIT_INST_2x(psrlq, Psrlq, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psrlq, Psrlq, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psrlq, Psrlq, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psrlq, Psrlq, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psrlq, Psrlq, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psrlq, Psrlq, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2x(psrlw, Psrlw, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psrlw, Psrlw, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2i(psrlw, Psrlw, X86Mm, Imm)                                    // MMX
  ASMJIT_INST_2x(psrlw, Psrlw, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psrlw, Psrlw, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2i(psrlw, Psrlw, X86Xmm, Imm)                                   // SSE2
  ASMJIT_INST_2x(psubb, Psubb, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psubb, Psubb, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(psubb, Psubb, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psubb, Psubb, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(psubd, Psubd, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psubd, Psubd, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(psubd, Psubd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psubd, Psubd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(psubq, Psubq, X86Mm, X86Mm)                                  // SSE2
  ASMJIT_INST_2x(psubq, Psubq, X86Mm, X86Mem)                                 // SSE2
  ASMJIT_INST_2x(psubq, Psubq, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psubq, Psubq, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(psubsb, Psubsb, X86Mm, X86Mm)                                // MMX
  ASMJIT_INST_2x(psubsb, Psubsb, X86Mm, X86Mem)                               // MMX
  ASMJIT_INST_2x(psubsb, Psubsb, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(psubsb, Psubsb, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(psubsw, Psubsw, X86Mm, X86Mm)                                // MMX
  ASMJIT_INST_2x(psubsw, Psubsw, X86Mm, X86Mem)                               // MMX
  ASMJIT_INST_2x(psubsw, Psubsw, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(psubsw, Psubsw, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(psubusb, Psubusb, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(psubusb, Psubusb, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(psubusb, Psubusb, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(psubusb, Psubusb, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(psubusw, Psubusw, X86Mm, X86Mm)                              // MMX
  ASMJIT_INST_2x(psubusw, Psubusw, X86Mm, X86Mem)                             // MMX
  ASMJIT_INST_2x(psubusw, Psubusw, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(psubusw, Psubusw, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(psubw, Psubw, X86Mm, X86Mm)                                  // MMX
  ASMJIT_INST_2x(psubw, Psubw, X86Mm, X86Mem)                                 // MMX
  ASMJIT_INST_2x(psubw, Psubw, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(psubw, Psubw, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(ptest, Ptest, X86Xmm, X86Xmm)                                // SSE4_1
  ASMJIT_INST_2x(ptest, Ptest, X86Xmm, X86Mem)                                // SSE4_1
  ASMJIT_INST_2x(punpckhbw, Punpckhbw, X86Mm, X86Mm)                          // MMX
  ASMJIT_INST_2x(punpckhbw, Punpckhbw, X86Mm, X86Mem)                         // MMX
  ASMJIT_INST_2x(punpckhbw, Punpckhbw, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(punpckhbw, Punpckhbw, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(punpckhdq, Punpckhdq, X86Mm, X86Mm)                          // MMX
  ASMJIT_INST_2x(punpckhdq, Punpckhdq, X86Mm, X86Mem)                         // MMX
  ASMJIT_INST_2x(punpckhdq, Punpckhdq, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(punpckhdq, Punpckhdq, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(punpckhqdq, Punpckhqdq, X86Xmm, X86Xmm)                      // SSE2
  ASMJIT_INST_2x(punpckhqdq, Punpckhqdq, X86Xmm, X86Mem)                      // SSE2
  ASMJIT_INST_2x(punpckhwd, Punpckhwd, X86Mm, X86Mm)                          // MMX
  ASMJIT_INST_2x(punpckhwd, Punpckhwd, X86Mm, X86Mem)                         // MMX
  ASMJIT_INST_2x(punpckhwd, Punpckhwd, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(punpckhwd, Punpckhwd, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(punpcklbw, Punpcklbw, X86Mm, X86Mm)                          // MMX
  ASMJIT_INST_2x(punpcklbw, Punpcklbw, X86Mm, X86Mem)                         // MMX
  ASMJIT_INST_2x(punpcklbw, Punpcklbw, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(punpcklbw, Punpcklbw, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(punpckldq, Punpckldq, X86Mm, X86Mm)                          // MMX
  ASMJIT_INST_2x(punpckldq, Punpckldq, X86Mm, X86Mem)                         // MMX
  ASMJIT_INST_2x(punpckldq, Punpckldq, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(punpckldq, Punpckldq, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(punpcklqdq, Punpcklqdq, X86Xmm, X86Xmm)                      // SSE2
  ASMJIT_INST_2x(punpcklqdq, Punpcklqdq, X86Xmm, X86Mem)                      // SSE2
  ASMJIT_INST_2x(punpcklwd, Punpcklwd, X86Mm, X86Mm)                          // MMX
  ASMJIT_INST_2x(punpcklwd, Punpcklwd, X86Mm, X86Mem)                         // MMX
  ASMJIT_INST_2x(punpcklwd, Punpcklwd, X86Xmm, X86Xmm)                        // SSE2
  ASMJIT_INST_2x(punpcklwd, Punpcklwd, X86Xmm, X86Mem)                        // SSE2
  ASMJIT_INST_2x(pxor, Pxor, X86Mm, X86Mm)                                    // MMX
  ASMJIT_INST_2x(pxor, Pxor, X86Mm, X86Mem)                                   // MMX
  ASMJIT_INST_2x(pxor, Pxor, X86Xmm, X86Xmm)                                  // SSE2
  ASMJIT_INST_2x(pxor, Pxor, X86Xmm, X86Mem)                                  // SSE2
  ASMJIT_INST_2x(rcpps, Rcpps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(rcpps, Rcpps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(rcpss, Rcpss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(rcpss, Rcpss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_3i(roundpd, Roundpd, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundpd, Roundpd, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundps, Roundps, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundps, Roundps, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundsd, Roundsd, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundsd, Roundsd, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundss, Roundss, X86Xmm, X86Xmm, Imm)                       // SSE4_1
  ASMJIT_INST_3i(roundss, Roundss, X86Xmm, X86Mem, Imm)                       // SSE4_1
  ASMJIT_INST_2x(rsqrtps, Rsqrtps, X86Xmm, X86Xmm)                            // SSE
  ASMJIT_INST_2x(rsqrtps, Rsqrtps, X86Xmm, X86Mem)                            // SSE
  ASMJIT_INST_2x(rsqrtss, Rsqrtss, X86Xmm, X86Xmm)                            // SSE
  ASMJIT_INST_2x(rsqrtss, Rsqrtss, X86Xmm, X86Mem)                            // SSE
  ASMJIT_INST_3i(shufpd, Shufpd, X86Xmm, X86Xmm, Imm)                         // SSE2
  ASMJIT_INST_3i(shufpd, Shufpd, X86Xmm, X86Mem, Imm)                         // SSE2
  ASMJIT_INST_3i(shufps, Shufps, X86Xmm, X86Xmm, Imm)                         // SSE
  ASMJIT_INST_3i(shufps, Shufps, X86Xmm, X86Mem, Imm)                         // SSE
  ASMJIT_INST_2x(sqrtpd, Sqrtpd, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(sqrtpd, Sqrtpd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(sqrtps, Sqrtps, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(sqrtps, Sqrtps, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(sqrtsd, Sqrtsd, X86Xmm, X86Xmm)                              // SSE2
  ASMJIT_INST_2x(sqrtsd, Sqrtsd, X86Xmm, X86Mem)                              // SSE2
  ASMJIT_INST_2x(sqrtss, Sqrtss, X86Xmm, X86Xmm)                              // SSE
  ASMJIT_INST_2x(sqrtss, Sqrtss, X86Xmm, X86Mem)                              // SSE
  ASMJIT_INST_2x(subpd, Subpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(subpd, Subpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(subps, Subps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(subps, Subps, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(subsd, Subsd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(subsd, Subsd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(subss, Subss, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(subss, Subss, X86Xmm, X86Mem)                                // SSE
  ASMJIT_INST_2x(ucomisd, Ucomisd, X86Xmm, X86Xmm)                            // SSE2
  ASMJIT_INST_2x(ucomisd, Ucomisd, X86Xmm, X86Mem)                            // SSE2
  ASMJIT_INST_2x(ucomiss, Ucomiss, X86Xmm, X86Xmm)                            // SSE
  ASMJIT_INST_2x(ucomiss, Ucomiss, X86Xmm, X86Mem)                            // SSE
  ASMJIT_INST_2x(unpckhpd, Unpckhpd, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(unpckhpd, Unpckhpd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(unpckhps, Unpckhps, X86Xmm, X86Xmm)                          // SSE
  ASMJIT_INST_2x(unpckhps, Unpckhps, X86Xmm, X86Mem)                          // SSE
  ASMJIT_INST_2x(unpcklpd, Unpcklpd, X86Xmm, X86Xmm)                          // SSE2
  ASMJIT_INST_2x(unpcklpd, Unpcklpd, X86Xmm, X86Mem)                          // SSE2
  ASMJIT_INST_2x(unpcklps, Unpcklps, X86Xmm, X86Xmm)                          // SSE
  ASMJIT_INST_2x(unpcklps, Unpcklps, X86Xmm, X86Mem)                          // SSE
  ASMJIT_INST_2x(xorpd, Xorpd, X86Xmm, X86Xmm)                                // SSE2
  ASMJIT_INST_2x(xorpd, Xorpd, X86Xmm, X86Mem)                                // SSE2
  ASMJIT_INST_2x(xorps, Xorps, X86Xmm, X86Xmm)                                // SSE
  ASMJIT_INST_2x(xorps, Xorps, X86Xmm, X86Mem)                                // SSE

  // -------------------------------------------------------------------------
  // [3DNOW & GEODE]
  // -------------------------------------------------------------------------

  ASMJIT_INST_2x(pavgusb, Pavgusb, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pavgusb, Pavgusb, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pf2id, Pf2id, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pf2id, Pf2id, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pf2iw, Pf2iw, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pf2iw, Pf2iw, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfacc, Pfacc, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfacc, Pfacc, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfadd, Pfadd, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfadd, Pfadd, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfcmpeq, Pfcmpeq, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pfcmpeq, Pfcmpeq, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pfcmpge, Pfcmpge, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pfcmpge, Pfcmpge, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pfcmpgt, Pfcmpgt, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pfcmpgt, Pfcmpgt, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pfmax, Pfmax, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfmax, Pfmax, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfmin, Pfmin, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfmin, Pfmin, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfmul, Pfmul, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfmul, Pfmul, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfnacc, Pfnacc, X86Mm, X86Mm)                                // 3DNOW
  ASMJIT_INST_2x(pfnacc, Pfnacc, X86Mm, X86Mem)                               // 3DNOW
  ASMJIT_INST_2x(pfpnacc, Pfpnacc, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pfpnacc, Pfpnacc, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pfrcp, Pfrcp, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfrcp, Pfrcp, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfrcpit1, Pfrcpit1, X86Mm, X86Mm)                            // 3DNOW
  ASMJIT_INST_2x(pfrcpit1, Pfrcpit1, X86Mm, X86Mem)                           // 3DNOW
  ASMJIT_INST_2x(pfrcpit2, Pfrcpit2, X86Mm, X86Mm)                            // 3DNOW
  ASMJIT_INST_2x(pfrcpit2, Pfrcpit2, X86Mm, X86Mem)                           // 3DNOW
  ASMJIT_INST_2x(pfrcpv, Pfrcpv, X86Mm, X86Mm)                                // GEODE
  ASMJIT_INST_2x(pfrcpv, Pfrcpv, X86Mm, X86Mem)                               // GEODE
  ASMJIT_INST_2x(pfrsqit1, Pfrsqit1, X86Mm, X86Mm)                            // 3DNOW
  ASMJIT_INST_2x(pfrsqit1, Pfrsqit1, X86Mm, X86Mem)                           // 3DNOW
  ASMJIT_INST_2x(pfrsqrt, Pfrsqrt, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pfrsqrt, Pfrsqrt, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pfrsqrtv, Pfrsqrtv, X86Mm, X86Mm)                            // GEODE
  ASMJIT_INST_2x(pfrsqrtv, Pfrsqrtv, X86Mm, X86Mem)                           // GEODE
  ASMJIT_INST_2x(pfsub, Pfsub, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pfsub, Pfsub, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pfsubr, Pfsubr, X86Mm, X86Mm)                                // 3DNOW
  ASMJIT_INST_2x(pfsubr, Pfsubr, X86Mm, X86Mem)                               // 3DNOW
  ASMJIT_INST_2x(pi2fd, Pi2fd, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pi2fd, Pi2fd, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pi2fw, Pi2fw, X86Mm, X86Mm)                                  // 3DNOW
  ASMJIT_INST_2x(pi2fw, Pi2fw, X86Mm, X86Mem)                                 // 3DNOW
  ASMJIT_INST_2x(pmulhrw, Pmulhrw, X86Mm, X86Mm)                              // 3DNOW
  ASMJIT_INST_2x(pmulhrw, Pmulhrw, X86Mm, X86Mem)                             // 3DNOW
  ASMJIT_INST_2x(pswapd, Pswapd, X86Mm, X86Mm)                                // 3DNOW
  ASMJIT_INST_2x(pswapd, Pswapd, X86Mm, X86Mem)                               // 3DNOW
  ASMJIT_INST_0x(femms, Femms)                                                // 3DNOW

  // --------------------------------------------------------------------------
  // [AESNI]
  // --------------------------------------------------------------------------

  ASMJIT_INST_2x(aesdec, Aesdec, X86Xmm, X86Xmm)                              // AESNI
  ASMJIT_INST_2x(aesdec, Aesdec, X86Xmm, X86Mem)                              // AESNI
  ASMJIT_INST_2x(aesdeclast, Aesdeclast, X86Xmm, X86Xmm)                      // AESNI
  ASMJIT_INST_2x(aesdeclast, Aesdeclast, X86Xmm, X86Mem)                      // AESNI
  ASMJIT_INST_2x(aesenc, Aesenc, X86Xmm, X86Xmm)                              // AESNI
  ASMJIT_INST_2x(aesenc, Aesenc, X86Xmm, X86Mem)                              // AESNI
  ASMJIT_INST_2x(aesenclast, Aesenclast, X86Xmm, X86Xmm)                      // AESNI
  ASMJIT_INST_2x(aesenclast, Aesenclast, X86Xmm, X86Mem)                      // AESNI
  ASMJIT_INST_2x(aesimc, Aesimc, X86Xmm, X86Xmm)                              // AESNI
  ASMJIT_INST_2x(aesimc, Aesimc, X86Xmm, X86Mem)                              // AESNI
  ASMJIT_INST_3i(aeskeygenassist, Aeskeygenassist, X86Xmm, X86Xmm, Imm)       // AESNI
  ASMJIT_INST_3i(aeskeygenassist, Aeskeygenassist, X86Xmm, X86Mem, Imm)       // AESNI

  // --------------------------------------------------------------------------
  // [SHA]
  // --------------------------------------------------------------------------

  ASMJIT_INST_2x(sha1msg1, Sha1msg1, X86Xmm, X86Xmm)                          // SHA
  ASMJIT_INST_2x(sha1msg1, Sha1msg1, X86Xmm, X86Mem)                          // SHA
  ASMJIT_INST_2x(sha1msg2, Sha1msg2, X86Xmm, X86Xmm)                          // SHA
  ASMJIT_INST_2x(sha1msg2, Sha1msg2, X86Xmm, X86Mem)                          // SHA
  ASMJIT_INST_2x(sha1nexte, Sha1nexte, X86Xmm, X86Xmm)                        // SHA
  ASMJIT_INST_2x(sha1nexte, Sha1nexte, X86Xmm, X86Mem)                        // SHA
  ASMJIT_INST_3i(sha1rnds4, Sha1rnds4, X86Xmm, X86Xmm, Imm)                   // SHA
  ASMJIT_INST_3i(sha1rnds4, Sha1rnds4, X86Xmm, X86Mem, Imm)                   // SHA
  ASMJIT_INST_2x(sha256msg1, Sha256msg1, X86Xmm, X86Xmm)                      // SHA
  ASMJIT_INST_2x(sha256msg1, Sha256msg1, X86Xmm, X86Mem)                      // SHA
  ASMJIT_INST_2x(sha256msg2, Sha256msg2, X86Xmm, X86Xmm)                      // SHA
  ASMJIT_INST_2x(sha256msg2, Sha256msg2, X86Xmm, X86Mem)                      // SHA
  ASMJIT_INST_3x(sha256rnds2, Sha256rnds2, X86Xmm, X86Xmm, XMM0)              // SHA [EXPLICIT]
  ASMJIT_INST_3x(sha256rnds2, Sha256rnds2, X86Xmm, X86Mem, XMM0)              // SHA [EXPLICIT]

  // --------------------------------------------------------------------------
  // [AVX...AVX512]
  // --------------------------------------------------------------------------

  ASMJIT_INST_3x(kaddb, Kaddb, X86KReg, X86KReg, X86KReg)                     // AVX512_DQ
  ASMJIT_INST_3x(kaddd, Kaddd, X86KReg, X86KReg, X86KReg)                     // AVX512_BW
  ASMJIT_INST_3x(kaddq, Kaddq, X86KReg, X86KReg, X86KReg)                     // AVX512_BW
  ASMJIT_INST_3x(kaddw, Kaddw, X86KReg, X86KReg, X86KReg)                     // AVX512_DQ
  ASMJIT_INST_3x(kandb, Kandb, X86KReg, X86KReg, X86KReg)                     // AVX512_DQ
  ASMJIT_INST_3x(kandd, Kandd, X86KReg, X86KReg, X86KReg)                     // AVX512_BW
  ASMJIT_INST_3x(kandnb, Kandnb, X86KReg, X86KReg, X86KReg)                   // AVX512_DQ
  ASMJIT_INST_3x(kandnd, Kandnd, X86KReg, X86KReg, X86KReg)                   // AVX512_BW
  ASMJIT_INST_3x(kandnq, Kandnq, X86KReg, X86KReg, X86KReg)                   // AVX512_BW
  ASMJIT_INST_3x(kandnw, Kandnw, X86KReg, X86KReg, X86KReg)                   // AVX512_F
  ASMJIT_INST_3x(kandq, Kandq, X86KReg, X86KReg, X86KReg)                     // AVX512_BW
  ASMJIT_INST_3x(kandw, Kandw, X86KReg, X86KReg, X86KReg)                     // AVX512_F
  ASMJIT_INST_2x(kmovb, Kmovb, X86KReg, X86KReg)                              // AVX512_DQ
  ASMJIT_INST_2x(kmovb, Kmovb, X86KReg, X86Mem)                               // AVX512_DQ
  ASMJIT_INST_2x(kmovb, Kmovb, X86KReg, X86Gp)                                // AVX512_DQ
  ASMJIT_INST_2x(kmovb, Kmovb, X86Mem, X86KReg)                               // AVX512_DQ
  ASMJIT_INST_2x(kmovb, Kmovb, X86Gp, X86KReg)                                // AVX512_DQ
  ASMJIT_INST_2x(kmovd, Kmovd, X86KReg, X86KReg)                              // AVX512_BW
  ASMJIT_INST_2x(kmovd, Kmovd, X86KReg, X86Mem)                               // AVX512_BW
  ASMJIT_INST_2x(kmovd, Kmovd, X86KReg, X86Gp)                                // AVX512_BW
  ASMJIT_INST_2x(kmovd, Kmovd, X86Mem, X86KReg)                               // AVX512_BW
  ASMJIT_INST_2x(kmovd, Kmovd, X86Gp, X86KReg)                                // AVX512_BW
  ASMJIT_INST_2x(kmovq, Kmovq, X86KReg, X86KReg)                              // AVX512_BW
  ASMJIT_INST_2x(kmovq, Kmovq, X86KReg, X86Mem)                               // AVX512_BW
  ASMJIT_INST_2x(kmovq, Kmovq, X86KReg, X86Gp)                                // AVX512_BW
  ASMJIT_INST_2x(kmovq, Kmovq, X86Mem, X86KReg)                               // AVX512_BW
  ASMJIT_INST_2x(kmovq, Kmovq, X86Gp, X86KReg)                                // AVX512_BW
  ASMJIT_INST_2x(kmovw, Kmovw, X86KReg, X86KReg)                              // AVX512_F
  ASMJIT_INST_2x(kmovw, Kmovw, X86KReg, X86Mem)                               // AVX512_F
  ASMJIT_INST_2x(kmovw, Kmovw, X86KReg, X86Gp)                                // AVX512_F
  ASMJIT_INST_2x(kmovw, Kmovw, X86Mem, X86KReg)                               // AVX512_F
  ASMJIT_INST_2x(kmovw, Kmovw, X86Gp, X86KReg)                                // AVX512_F
  ASMJIT_INST_2x(knotb, Knotb, X86KReg, X86KReg)                              // AVX512_DQ
  ASMJIT_INST_2x(knotd, Knotd, X86KReg, X86KReg)                              // AVX512_BW
  ASMJIT_INST_2x(knotq, Knotq, X86KReg, X86KReg)                              // AVX512_BW
  ASMJIT_INST_2x(knotw, Knotw, X86KReg, X86KReg)                              // AVX512_F
  ASMJIT_INST_3x(korb, Korb, X86KReg, X86KReg, X86KReg)                       // AVX512_DQ
  ASMJIT_INST_3x(kord, Kord, X86KReg, X86KReg, X86KReg)                       // AVX512_BW
  ASMJIT_INST_3x(korq, Korq, X86KReg, X86KReg, X86KReg)                       // AVX512_BW
  ASMJIT_INST_2x(kortestb, Kortestb, X86KReg, X86KReg)                        // AVX512_DQ
  ASMJIT_INST_2x(kortestd, Kortestd, X86KReg, X86KReg)                        // AVX512_BW
  ASMJIT_INST_2x(kortestq, Kortestq, X86KReg, X86KReg)                        // AVX512_BW
  ASMJIT_INST_2x(kortestw, Kortestw, X86KReg, X86KReg)                        // AVX512_F
  ASMJIT_INST_3x(korw, Korw, X86KReg, X86KReg, X86KReg)                       // AVX512_F
  ASMJIT_INST_3i(kshiftlb, Kshiftlb, X86KReg, X86KReg, Imm)                   // AVX512_DQ
  ASMJIT_INST_3i(kshiftld, Kshiftld, X86KReg, X86KReg, Imm)                   // AVX512_BW
  ASMJIT_INST_3i(kshiftlq, Kshiftlq, X86KReg, X86KReg, Imm)                   // AVX512_BW
  ASMJIT_INST_3i(kshiftlw, Kshiftlw, X86KReg, X86KReg, Imm)                   // AVX512_F
  ASMJIT_INST_3i(kshiftrb, Kshiftrb, X86KReg, X86KReg, Imm)                   // AVX512_DQ
  ASMJIT_INST_3i(kshiftrd, Kshiftrd, X86KReg, X86KReg, Imm)                   // AVX512_BW
  ASMJIT_INST_3i(kshiftrq, Kshiftrq, X86KReg, X86KReg, Imm)                   // AVX512_BW
  ASMJIT_INST_3i(kshiftrw, Kshiftrw, X86KReg, X86KReg, Imm)                   // AVX512_F
  ASMJIT_INST_2x(ktestb, Ktestb, X86KReg, X86KReg)                            // AVX512_DQ
  ASMJIT_INST_2x(ktestd, Ktestd, X86KReg, X86KReg)                            // AVX512_BW
  ASMJIT_INST_2x(ktestq, Ktestq, X86KReg, X86KReg)                            // AVX512_BW
  ASMJIT_INST_2x(ktestw, Ktestw, X86KReg, X86KReg)                            // AVX512_DQ
  ASMJIT_INST_3x(kunpckbw, Kunpckbw, X86KReg, X86KReg, X86KReg)               // AVX512_F
  ASMJIT_INST_3x(kunpckdq, Kunpckdq, X86KReg, X86KReg, X86KReg)               // AVX512_BW
  ASMJIT_INST_3x(kunpckwd, Kunpckwd, X86KReg, X86KReg, X86KReg)               // AVX512_BW
  ASMJIT_INST_3x(kxnorb, Kxnorb, X86KReg, X86KReg, X86KReg)                   // AVX512_DQ
  ASMJIT_INST_3x(kxnord, Kxnord, X86KReg, X86KReg, X86KReg)                   // AVX512_BW
  ASMJIT_INST_3x(kxnorq, Kxnorq, X86KReg, X86KReg, X86KReg)                   // AVX512_BW
  ASMJIT_INST_3x(kxnorw, Kxnorw, X86KReg, X86KReg, X86KReg)                   // AVX512_F
  ASMJIT_INST_3x(kxorb, Kxorb, X86KReg, X86KReg, X86KReg)                     // AVX512_DQ
  ASMJIT_INST_3x(kxord, Kxord, X86KReg, X86KReg, X86KReg)                     // AVX512_BW
  ASMJIT_INST_3x(kxorq, Kxorq, X86KReg, X86KReg, X86KReg)                     // AVX512_BW
  ASMJIT_INST_3x(kxorw, Kxorw, X86KReg, X86KReg, X86KReg)                     // AVX512_F
  ASMJIT_INST_6x(v4fmaddps, V4fmaddps, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Mem)   // AVX512_4FMAPS{kz}
  ASMJIT_INST_6x(v4fnmaddps, V4fnmaddps, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Mem) // AVX512_4FMAPS{kz}
  ASMJIT_INST_3x(vaddpd, Vaddpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vaddpd, Vaddpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vaddpd, Vaddpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vaddpd, Vaddpd, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vaddpd, Vaddpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vaddpd, Vaddpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vaddps, Vaddps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vaddps, Vaddps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vaddps, Vaddps, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vaddps, Vaddps, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vaddps, Vaddps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vaddps, Vaddps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vaddsd, Vaddsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vaddsd, Vaddsd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vaddss, Vaddss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vaddss, Vaddss, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vaddsubpd, Vaddsubpd, X86Xmm, X86Xmm, X86Xmm)                // AVX
  ASMJIT_INST_3x(vaddsubpd, Vaddsubpd, X86Xmm, X86Xmm, X86Mem)                // AVX
  ASMJIT_INST_3x(vaddsubpd, Vaddsubpd, X86Ymm, X86Ymm, X86Ymm)                // AVX
  ASMJIT_INST_3x(vaddsubpd, Vaddsubpd, X86Ymm, X86Ymm, X86Mem)                // AVX
  ASMJIT_INST_3x(vaddsubps, Vaddsubps, X86Xmm, X86Xmm, X86Xmm)                // AVX
  ASMJIT_INST_3x(vaddsubps, Vaddsubps, X86Xmm, X86Xmm, X86Mem)                // AVX
  ASMJIT_INST_3x(vaddsubps, Vaddsubps, X86Ymm, X86Ymm, X86Ymm)                // AVX
  ASMJIT_INST_3x(vaddsubps, Vaddsubps, X86Ymm, X86Ymm, X86Mem)                // AVX
  ASMJIT_INST_3x(vaesdec, Vaesdec, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vaesdec, Vaesdec, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vaesdeclast, Vaesdeclast, X86Xmm, X86Xmm, X86Xmm)            // AVX
  ASMJIT_INST_3x(vaesdeclast, Vaesdeclast, X86Xmm, X86Xmm, X86Mem)            // AVX
  ASMJIT_INST_3x(vaesenc, Vaesenc, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vaesenc, Vaesenc, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vaesenclast, Vaesenclast, X86Xmm, X86Xmm, X86Xmm)            // AVX
  ASMJIT_INST_3x(vaesenclast, Vaesenclast, X86Xmm, X86Xmm, X86Mem)            // AVX
  ASMJIT_INST_2x(vaesimc, Vaesimc, X86Xmm, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vaesimc, Vaesimc, X86Xmm, X86Mem)                            // AVX
  ASMJIT_INST_3i(vaeskeygenassist, Vaeskeygenassist, X86Xmm, X86Xmm, Imm)     // AVX
  ASMJIT_INST_3i(vaeskeygenassist, Vaeskeygenassist, X86Xmm, X86Mem, Imm)     // AVX
  ASMJIT_INST_4i(valignd, Valignd, X86Xmm, X86Xmm, X86Xmm, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(valignd, Valignd, X86Xmm, X86Xmm, X86Mem, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(valignd, Valignd, X86Ymm, X86Ymm, X86Ymm, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(valignd, Valignd, X86Ymm, X86Ymm, X86Mem, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(valignd, Valignd, X86Zmm, X86Zmm, X86Zmm, Imm)               //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(valignd, Valignd, X86Zmm, X86Zmm, X86Mem, Imm)               //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(valignq, Valignq, X86Xmm, X86Xmm, X86Xmm, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(valignq, Valignq, X86Xmm, X86Xmm, X86Mem, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(valignq, Valignq, X86Ymm, X86Ymm, X86Ymm, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(valignq, Valignq, X86Ymm, X86Ymm, X86Mem, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(valignq, Valignq, X86Zmm, X86Zmm, X86Zmm, Imm)               //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(valignq, Valignq, X86Zmm, X86Zmm, X86Mem, Imm)               //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vandnpd, Vandnpd, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandnpd, Vandnpd, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandnpd, Vandnpd, X86Ymm, X86Ymm, X86Ymm)                    // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandnpd, Vandnpd, X86Ymm, X86Ymm, X86Mem)                    // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandnpd, Vandnpd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vandnpd, Vandnpd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vandnps, Vandnps, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandnps, Vandnps, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandnps, Vandnps, X86Ymm, X86Ymm, X86Ymm)                    // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandnps, Vandnps, X86Ymm, X86Ymm, X86Mem)                    // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandnps, Vandnps, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_DQ{kz|b32}
  ASMJIT_INST_3x(vandnps, Vandnps, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_DQ{kz|b32}
  ASMJIT_INST_3x(vandpd, Vandpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandpd, Vandpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandpd, Vandpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandpd, Vandpd, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vandpd, Vandpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vandpd, Vandpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vandps, Vandps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandps, Vandps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandps, Vandps, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandps, Vandps, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vandps, Vandps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_DQ{kz|b32}
  ASMJIT_INST_3x(vandps, Vandps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|b32}
  ASMJIT_INST_3x(vblendmb, Vblendmb, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmb, Vblendmb, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmb, Vblendmb, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmb, Vblendmb, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmb, Vblendmb, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vblendmb, Vblendmb, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vblendmd, Vblendmd, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmd, Vblendmd, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmd, Vblendmd, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmd, Vblendmd, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmd, Vblendmd, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vblendmd, Vblendmd, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vblendmpd, Vblendmpd, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmpd, Vblendmpd, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmpd, Vblendmpd, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmpd, Vblendmpd, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmpd, Vblendmpd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vblendmpd, Vblendmpd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vblendmps, Vblendmps, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmps, Vblendmps, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmps, Vblendmps, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmps, Vblendmps, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vblendmps, Vblendmps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vblendmps, Vblendmps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vblendmq, Vblendmq, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmq, Vblendmq, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmq, Vblendmq, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmq, Vblendmq, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vblendmq, Vblendmq, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vblendmq, Vblendmq, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vblendmw, Vblendmw, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmw, Vblendmw, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmw, Vblendmw, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmw, Vblendmw, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vblendmw, Vblendmw, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vblendmw, Vblendmw, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_4i(vblendpd, Vblendpd, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX
  ASMJIT_INST_4i(vblendpd, Vblendpd, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4i(vblendpd, Vblendpd, X86Ymm, X86Ymm, X86Ymm, Imm)             // AVX
  ASMJIT_INST_4i(vblendpd, Vblendpd, X86Ymm, X86Ymm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4i(vblendps, Vblendps, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX
  ASMJIT_INST_4i(vblendps, Vblendps, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4i(vblendps, Vblendps, X86Ymm, X86Ymm, X86Ymm, Imm)             // AVX
  ASMJIT_INST_4i(vblendps, Vblendps, X86Ymm, X86Ymm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4x(vblendvpd, Vblendvpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // AVX
  ASMJIT_INST_4x(vblendvpd, Vblendvpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // AVX
  ASMJIT_INST_4x(vblendvpd, Vblendvpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // AVX
  ASMJIT_INST_4x(vblendvpd, Vblendvpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // AVX
  ASMJIT_INST_4x(vblendvps, Vblendvps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // AVX
  ASMJIT_INST_4x(vblendvps, Vblendvps, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // AVX
  ASMJIT_INST_4x(vblendvps, Vblendvps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // AVX
  ASMJIT_INST_4x(vblendvps, Vblendvps, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // AVX
  ASMJIT_INST_2x(vbroadcastf128, Vbroadcastf128, X86Ymm, X86Mem)              // AVX
  ASMJIT_INST_2x(vbroadcastf32x2, Vbroadcastf32x2, X86Ymm, X86Xmm)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcastf32x2, Vbroadcastf32x2, X86Ymm, X86Mem)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcastf32x2, Vbroadcastf32x2, X86Zmm, X86Xmm)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcastf32x2, Vbroadcastf32x2, X86Zmm, X86Mem)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcastf32x4, Vbroadcastf32x4, X86Ymm, X86Mem)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastf32x4, Vbroadcastf32x4, X86Zmm, X86Mem)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastf32x8, Vbroadcastf32x8, X86Zmm, X86Mem)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcastf64x2, Vbroadcastf64x2, X86Ymm, X86Mem)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcastf64x2, Vbroadcastf64x2, X86Zmm, X86Mem)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcastf64x4, Vbroadcastf64x4, X86Zmm, X86Mem)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcasti128, Vbroadcasti128, X86Ymm, X86Mem)              // AVX2
  ASMJIT_INST_2x(vbroadcasti32x2, Vbroadcasti32x2, X86Xmm, X86Xmm)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcasti32x2, Vbroadcasti32x2, X86Xmm, X86Mem)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcasti32x2, Vbroadcasti32x2, X86Ymm, X86Xmm)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcasti32x2, Vbroadcasti32x2, X86Ymm, X86Mem)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcasti32x2, Vbroadcasti32x2, X86Zmm, X86Xmm)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcasti32x2, Vbroadcasti32x2, X86Zmm, X86Mem)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcasti32x4, Vbroadcasti32x4, X86Ymm, X86Xmm)            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcasti32x4, Vbroadcasti32x4, X86Ymm, X86Mem)            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcasti32x4, Vbroadcasti32x4, X86Zmm, X86Xmm)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcasti32x4, Vbroadcasti32x4, X86Zmm, X86Mem)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcasti32x8, Vbroadcasti32x8, X86Zmm, X86Xmm)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcasti32x8, Vbroadcasti32x8, X86Zmm, X86Mem)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcasti64x2, Vbroadcasti64x2, X86Ymm, X86Xmm)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcasti64x2, Vbroadcasti64x2, X86Ymm, X86Mem)            //      AVX512_DQ{kz}-VL
  ASMJIT_INST_2x(vbroadcasti64x2, Vbroadcasti64x2, X86Zmm, X86Xmm)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcasti64x2, Vbroadcasti64x2, X86Zmm, X86Mem)            //      AVX512_DQ{kz}
  ASMJIT_INST_2x(vbroadcasti64x4, Vbroadcasti64x4, X86Zmm, X86Xmm)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcasti64x4, Vbroadcasti64x4, X86Zmm, X86Mem)            //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastsd, Vbroadcastsd, X86Ymm, X86Mem)                  // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcastsd, Vbroadcastsd, X86Ymm, X86Xmm)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcastsd, Vbroadcastsd, X86Zmm, X86Xmm)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastsd, Vbroadcastsd, X86Zmm, X86Mem)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastss, Vbroadcastss, X86Xmm, X86Mem)                  // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcastss, Vbroadcastss, X86Xmm, X86Xmm)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcastss, Vbroadcastss, X86Ymm, X86Mem)                  // AVX  AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastss, Vbroadcastss, X86Ymm, X86Xmm)                  // AVX2 AVX512_F{kz}
  ASMJIT_INST_2x(vbroadcastss, Vbroadcastss, X86Zmm, X86Xmm)                  //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vbroadcastss, Vbroadcastss, X86Zmm, X86Mem)                  //      AVX512_F{kz}-VL
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86Xmm, X86Xmm, X86Xmm, Imm)                 // AVX
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86Xmm, X86Xmm, X86Mem, Imm)                 // AVX
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86Ymm, X86Ymm, X86Ymm, Imm)                 // AVX
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86Ymm, X86Ymm, X86Mem, Imm)                 // AVX
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86KReg, X86Ymm, X86Ymm, Imm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86KReg, X86Ymm, X86Mem, Imm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86KReg, X86Zmm, X86Zmm, Imm)                //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_4i(vcmppd, Vcmppd, X86KReg, X86Zmm, X86Mem, Imm)                //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86Xmm, X86Xmm, X86Xmm, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86Xmm, X86Xmm, X86Mem, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86Ymm, X86Ymm, X86Ymm, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86Ymm, X86Ymm, X86Mem, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86KReg, X86Ymm, X86Ymm, Imm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86KReg, X86Ymm, X86Mem, Imm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86KReg, X86Zmm, X86Zmm, Imm)                //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_4i(vcmpps, Vcmpps, X86KReg, X86Zmm, X86Mem, Imm)                //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_4i(vcmpsd, Vcmpsd, X86Xmm, X86Xmm, X86Xmm, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpsd, Vcmpsd, X86Xmm, X86Xmm, X86Mem, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpsd, Vcmpsd, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vcmpsd, Vcmpsd, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vcmpss, Vcmpss, X86Xmm, X86Xmm, X86Xmm, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpss, Vcmpss, X86Xmm, X86Xmm, X86Mem, Imm)                 // AVX
  ASMJIT_INST_4i(vcmpss, Vcmpss, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vcmpss, Vcmpss, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vcomisd, Vcomisd, X86Xmm, X86Xmm)                            // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcomisd, Vcomisd, X86Xmm, X86Mem)                            // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcomiss, Vcomiss, X86Xmm, X86Xmm)                            // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcomiss, Vcomiss, X86Xmm, X86Mem)                            // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcompresspd, Vcompresspd, X86Xmm, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompresspd, Vcompresspd, X86Mem, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompresspd, Vcompresspd, X86Ymm, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompresspd, Vcompresspd, X86Mem, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompresspd, Vcompresspd, X86Zmm, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vcompresspd, Vcompresspd, X86Mem, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vcompressps, Vcompressps, X86Xmm, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompressps, Vcompressps, X86Mem, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompressps, Vcompressps, X86Ymm, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompressps, Vcompressps, X86Mem, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcompressps, Vcompressps, X86Zmm, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vcompressps, Vcompressps, X86Mem, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vcvtdq2pd, Vcvtdq2pd, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2pd, Vcvtdq2pd, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2pd, Vcvtdq2pd, X86Ymm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2pd, Vcvtdq2pd, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2pd, Vcvtdq2pd, X86Zmm, X86Ymm)                        //      AVX512_F{kz|b32}
  ASMJIT_INST_2x(vcvtdq2pd, Vcvtdq2pd, X86Zmm, X86Mem)                        //      AVX512_F{kz|b32}
  ASMJIT_INST_2x(vcvtdq2ps, Vcvtdq2ps, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2ps, Vcvtdq2ps, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2ps, Vcvtdq2ps, X86Ymm, X86Ymm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2ps, Vcvtdq2ps, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtdq2ps, Vcvtdq2ps, X86Zmm, X86Zmm)                        //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtdq2ps, Vcvtdq2ps, X86Zmm, X86Mem)                        //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtpd2dq, Vcvtpd2dq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2dq, Vcvtpd2dq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2dq, Vcvtpd2dq, X86Xmm, X86Ymm)                        // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2dq, Vcvtpd2dq, X86Ymm, X86Zmm)                        //      AVX512_F{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2dq, Vcvtpd2dq, X86Ymm, X86Mem)                        //      AVX512_F{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2ps, Vcvtpd2ps, X86Xmm, X86Xmm)                        // AVX
  ASMJIT_INST_2x(vcvtpd2ps, Vcvtpd2ps, X86Xmm, X86Mem)                        // AVX
  ASMJIT_INST_2x(vcvtpd2ps, Vcvtpd2ps, X86Xmm, X86Ymm)                        // AVX
  ASMJIT_INST_2x(vcvtpd2qq, Vcvtpd2qq, X86Xmm, X86Xmm)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2qq, Vcvtpd2qq, X86Xmm, X86Mem)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2qq, Vcvtpd2qq, X86Ymm, X86Ymm)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2qq, Vcvtpd2qq, X86Ymm, X86Mem)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2qq, Vcvtpd2qq, X86Zmm, X86Zmm)                        //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2qq, Vcvtpd2qq, X86Zmm, X86Mem)                        //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2udq, Vcvtpd2udq, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2udq, Vcvtpd2udq, X86Xmm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2udq, Vcvtpd2udq, X86Xmm, X86Ymm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2udq, Vcvtpd2udq, X86Ymm, X86Zmm)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2udq, Vcvtpd2udq, X86Ymm, X86Mem)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2uqq, Vcvtpd2uqq, X86Xmm, X86Xmm)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2uqq, Vcvtpd2uqq, X86Xmm, X86Mem)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2uqq, Vcvtpd2uqq, X86Ymm, X86Ymm)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2uqq, Vcvtpd2uqq, X86Ymm, X86Mem)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtpd2uqq, Vcvtpd2uqq, X86Zmm, X86Zmm)                      //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtpd2uqq, Vcvtpd2uqq, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtph2ps, Vcvtph2ps, X86Xmm, X86Xmm)                        // F16C AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcvtph2ps, Vcvtph2ps, X86Xmm, X86Mem)                        // F16C AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcvtph2ps, Vcvtph2ps, X86Ymm, X86Xmm)                        // F16C AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcvtph2ps, Vcvtph2ps, X86Ymm, X86Mem)                        // F16C AVX512_F{kz}-VL
  ASMJIT_INST_2x(vcvtph2ps, Vcvtph2ps, X86Zmm, X86Ymm)                        //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vcvtph2ps, Vcvtph2ps, X86Zmm, X86Mem)                        //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vcvtps2dq, Vcvtps2dq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2dq, Vcvtps2dq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2dq, Vcvtps2dq, X86Ymm, X86Ymm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2dq, Vcvtps2dq, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2dq, Vcvtps2dq, X86Zmm, X86Zmm)                        //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2dq, Vcvtps2dq, X86Zmm, X86Mem)                        //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2pd, Vcvtps2pd, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2pd, Vcvtps2pd, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2pd, Vcvtps2pd, X86Ymm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2pd, Vcvtps2pd, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2pd, Vcvtps2pd, X86Zmm, X86Ymm)                        //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2pd, Vcvtps2pd, X86Zmm, X86Mem)                        //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3i(vcvtps2ph, Vcvtps2ph, X86Xmm, X86Xmm, Imm)                   // F16C AVX512_F{kz}-VL
  ASMJIT_INST_3i(vcvtps2ph, Vcvtps2ph, X86Mem, X86Xmm, Imm)                   // F16C AVX512_F{kz}-VL
  ASMJIT_INST_3i(vcvtps2ph, Vcvtps2ph, X86Xmm, X86Ymm, Imm)                   // F16C AVX512_F{kz}-VL
  ASMJIT_INST_3i(vcvtps2ph, Vcvtps2ph, X86Mem, X86Ymm, Imm)                   // F16C AVX512_F{kz}-VL
  ASMJIT_INST_3i(vcvtps2ph, Vcvtps2ph, X86Ymm, X86Zmm, Imm)                   //      AVX512_F{kz|sae}
  ASMJIT_INST_3i(vcvtps2ph, Vcvtps2ph, X86Mem, X86Zmm, Imm)                   //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vcvtps2qq, Vcvtps2qq, X86Xmm, X86Xmm)                        //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2qq, Vcvtps2qq, X86Xmm, X86Mem)                        //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2qq, Vcvtps2qq, X86Ymm, X86Xmm)                        //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2qq, Vcvtps2qq, X86Ymm, X86Mem)                        //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2qq, Vcvtps2qq, X86Zmm, X86Ymm)                        //      AVX512_DQ{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2qq, Vcvtps2qq, X86Zmm, X86Mem)                        //      AVX512_DQ{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2udq, Vcvtps2udq, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2udq, Vcvtps2udq, X86Xmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2udq, Vcvtps2udq, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2udq, Vcvtps2udq, X86Ymm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2udq, Vcvtps2udq, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2udq, Vcvtps2udq, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2uqq, Vcvtps2uqq, X86Xmm, X86Xmm)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2uqq, Vcvtps2uqq, X86Xmm, X86Mem)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2uqq, Vcvtps2uqq, X86Ymm, X86Xmm)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2uqq, Vcvtps2uqq, X86Ymm, X86Mem)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvtps2uqq, Vcvtps2uqq, X86Zmm, X86Ymm)                      //      AVX512_DQ{kz|er|b32}
  ASMJIT_INST_2x(vcvtps2uqq, Vcvtps2uqq, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|er|b32}
  ASMJIT_INST_2x(vcvtqq2pd, Vcvtqq2pd, X86Xmm, X86Xmm)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2pd, Vcvtqq2pd, X86Xmm, X86Mem)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2pd, Vcvtqq2pd, X86Ymm, X86Ymm)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2pd, Vcvtqq2pd, X86Ymm, X86Mem)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2pd, Vcvtqq2pd, X86Zmm, X86Zmm)                        //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtqq2pd, Vcvtqq2pd, X86Zmm, X86Mem)                        //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtqq2ps, Vcvtqq2ps, X86Xmm, X86Xmm)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2ps, Vcvtqq2ps, X86Xmm, X86Mem)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2ps, Vcvtqq2ps, X86Xmm, X86Ymm)                        //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtqq2ps, Vcvtqq2ps, X86Ymm, X86Zmm)                        //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtqq2ps, Vcvtqq2ps, X86Ymm, X86Mem)                        //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtsd2si, Vcvtsd2si, X86Gp, X86Xmm)                         // AVX  AVX512_F{er}
  ASMJIT_INST_2x(vcvtsd2si, Vcvtsd2si, X86Gp, X86Mem)                         // AVX  AVX512_F{er}
  ASMJIT_INST_3x(vcvtsd2ss, Vcvtsd2ss, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vcvtsd2ss, Vcvtsd2ss, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|er}
  ASMJIT_INST_2x(vcvtsd2usi, Vcvtsd2usi, X86Gp, X86Xmm)                       //      AVX512_F{er}
  ASMJIT_INST_2x(vcvtsd2usi, Vcvtsd2usi, X86Gp, X86Mem)                       //      AVX512_F{er}
  ASMJIT_INST_3x(vcvtsi2sd, Vcvtsi2sd, X86Xmm, X86Xmm, X86Gp)                 // AVX  AVX512_F{er}
  ASMJIT_INST_3x(vcvtsi2sd, Vcvtsi2sd, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{er}
  ASMJIT_INST_3x(vcvtsi2ss, Vcvtsi2ss, X86Xmm, X86Xmm, X86Gp)                 // AVX  AVX512_F{er}
  ASMJIT_INST_3x(vcvtsi2ss, Vcvtsi2ss, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{er}
  ASMJIT_INST_3x(vcvtss2sd, Vcvtss2sd, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|sae}
  ASMJIT_INST_3x(vcvtss2sd, Vcvtss2sd, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|sae}
  ASMJIT_INST_2x(vcvtss2si, Vcvtss2si, X86Gp, X86Xmm)                         // AVX  AVX512_F{er}
  ASMJIT_INST_2x(vcvtss2si, Vcvtss2si, X86Gp, X86Mem)                         // AVX  AVX512_F{er}
  ASMJIT_INST_2x(vcvtss2usi, Vcvtss2usi, X86Gp, X86Xmm)                       //      AVX512_F{er}
  ASMJIT_INST_2x(vcvtss2usi, Vcvtss2usi, X86Gp, X86Mem)                       //      AVX512_F{er}
  ASMJIT_INST_2x(vcvttpd2dq, Vcvttpd2dq, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2dq, Vcvttpd2dq, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2dq, Vcvttpd2dq, X86Xmm, X86Ymm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2dq, Vcvttpd2dq, X86Ymm, X86Zmm)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2dq, Vcvttpd2dq, X86Ymm, X86Mem)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2qq, Vcvttpd2qq, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2qq, Vcvttpd2qq, X86Xmm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2qq, Vcvttpd2qq, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2qq, Vcvttpd2qq, X86Ymm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2qq, Vcvttpd2qq, X86Zmm, X86Zmm)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2qq, Vcvttpd2qq, X86Zmm, X86Mem)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2udq, Vcvttpd2udq, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2udq, Vcvttpd2udq, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2udq, Vcvttpd2udq, X86Xmm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2udq, Vcvttpd2udq, X86Ymm, X86Zmm)                    //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2udq, Vcvttpd2udq, X86Ymm, X86Mem)                    //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2uqq, Vcvttpd2uqq, X86Xmm, X86Xmm)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2uqq, Vcvttpd2uqq, X86Xmm, X86Mem)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2uqq, Vcvttpd2uqq, X86Ymm, X86Ymm)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2uqq, Vcvttpd2uqq, X86Ymm, X86Mem)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvttpd2uqq, Vcvttpd2uqq, X86Zmm, X86Zmm)                    //      AVX512_DQ{kz|sae|b64}
  ASMJIT_INST_2x(vcvttpd2uqq, Vcvttpd2uqq, X86Zmm, X86Mem)                    //      AVX512_DQ{kz|sae|b64}
  ASMJIT_INST_2x(vcvttps2dq, Vcvttps2dq, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2dq, Vcvttps2dq, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2dq, Vcvttps2dq, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2dq, Vcvttps2dq, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2dq, Vcvttps2dq, X86Zmm, X86Zmm)                      //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2dq, Vcvttps2dq, X86Zmm, X86Mem)                      //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2qq, Vcvttps2qq, X86Xmm, X86Xmm)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2qq, Vcvttps2qq, X86Xmm, X86Mem)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2qq, Vcvttps2qq, X86Ymm, X86Xmm)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2qq, Vcvttps2qq, X86Ymm, X86Mem)                      //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2qq, Vcvttps2qq, X86Zmm, X86Ymm)                      //      AVX512_DQ{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2qq, Vcvttps2qq, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2udq, Vcvttps2udq, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2udq, Vcvttps2udq, X86Xmm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2udq, Vcvttps2udq, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2udq, Vcvttps2udq, X86Ymm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2udq, Vcvttps2udq, X86Zmm, X86Zmm)                    //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2udq, Vcvttps2udq, X86Zmm, X86Mem)                    //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2uqq, Vcvttps2uqq, X86Xmm, X86Xmm)                    //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2uqq, Vcvttps2uqq, X86Xmm, X86Mem)                    //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2uqq, Vcvttps2uqq, X86Ymm, X86Xmm)                    //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2uqq, Vcvttps2uqq, X86Ymm, X86Mem)                    //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_2x(vcvttps2uqq, Vcvttps2uqq, X86Zmm, X86Ymm)                    //      AVX512_DQ{kz|sae|b32}
  ASMJIT_INST_2x(vcvttps2uqq, Vcvttps2uqq, X86Zmm, X86Mem)                    //      AVX512_DQ{kz|sae|b32}
  ASMJIT_INST_2x(vcvttsd2si, Vcvttsd2si, X86Gp, X86Xmm)                       // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcvttsd2si, Vcvttsd2si, X86Gp, X86Mem)                       // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcvttsd2usi, Vcvttsd2usi, X86Gp, X86Xmm)                     //      AVX512_F{sae}
  ASMJIT_INST_2x(vcvttsd2usi, Vcvttsd2usi, X86Gp, X86Mem)                     //      AVX512_F{sae}
  ASMJIT_INST_2x(vcvttss2si, Vcvttss2si, X86Gp, X86Xmm)                       // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcvttss2si, Vcvttss2si, X86Gp, X86Mem)                       // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vcvttss2usi, Vcvttss2usi, X86Gp, X86Xmm)                     //      AVX512_F{sae}
  ASMJIT_INST_2x(vcvttss2usi, Vcvttss2usi, X86Gp, X86Mem)                     //      AVX512_F{sae}
  ASMJIT_INST_2x(vcvtudq2pd, Vcvtudq2pd, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2pd, Vcvtudq2pd, X86Xmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2pd, Vcvtudq2pd, X86Ymm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2pd, Vcvtudq2pd, X86Ymm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2pd, Vcvtudq2pd, X86Zmm, X86Ymm)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_2x(vcvtudq2pd, Vcvtudq2pd, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_2x(vcvtudq2ps, Vcvtudq2ps, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2ps, Vcvtudq2ps, X86Xmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2ps, Vcvtudq2ps, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2ps, Vcvtudq2ps, X86Ymm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vcvtudq2ps, Vcvtudq2ps, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtudq2ps, Vcvtudq2ps, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vcvtuqq2pd, Vcvtuqq2pd, X86Xmm, X86Xmm)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2pd, Vcvtuqq2pd, X86Xmm, X86Mem)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2pd, Vcvtuqq2pd, X86Ymm, X86Ymm)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2pd, Vcvtuqq2pd, X86Ymm, X86Mem)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2pd, Vcvtuqq2pd, X86Zmm, X86Zmm)                      //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtuqq2pd, Vcvtuqq2pd, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtuqq2ps, Vcvtuqq2ps, X86Xmm, X86Xmm)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2ps, Vcvtuqq2ps, X86Xmm, X86Mem)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2ps, Vcvtuqq2ps, X86Xmm, X86Ymm)                      //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_2x(vcvtuqq2ps, Vcvtuqq2ps, X86Ymm, X86Zmm)                      //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_2x(vcvtuqq2ps, Vcvtuqq2ps, X86Ymm, X86Mem)                      //      AVX512_DQ{kz|er|b64}
  ASMJIT_INST_3x(vcvtusi2sd, Vcvtusi2sd, X86Xmm, X86Xmm, X86Gp)               //      AVX512_F{er}
  ASMJIT_INST_3x(vcvtusi2sd, Vcvtusi2sd, X86Xmm, X86Xmm, X86Mem)              //      AVX512_F{er}
  ASMJIT_INST_3x(vcvtusi2ss, Vcvtusi2ss, X86Xmm, X86Xmm, X86Gp)               //      AVX512_F{er}
  ASMJIT_INST_3x(vcvtusi2ss, Vcvtusi2ss, X86Xmm, X86Xmm, X86Mem)              //      AVX512_F{er}
  ASMJIT_INST_4i(vdbpsadbw, Vdbpsadbw, X86Xmm, X86Xmm, X86Xmm, Imm)           //      AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vdbpsadbw, Vdbpsadbw, X86Xmm, X86Xmm, X86Mem, Imm)           //      AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vdbpsadbw, Vdbpsadbw, X86Ymm, X86Ymm, X86Ymm, Imm)           //      AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vdbpsadbw, Vdbpsadbw, X86Ymm, X86Ymm, X86Mem, Imm)           //      AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vdbpsadbw, Vdbpsadbw, X86Zmm, X86Zmm, X86Zmm, Imm)           //      AVX512_BW{kz}
  ASMJIT_INST_4i(vdbpsadbw, Vdbpsadbw, X86Zmm, X86Zmm, X86Mem, Imm)           //      AVX512_BW{kz}
  ASMJIT_INST_3x(vdivpd, Vdivpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vdivpd, Vdivpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vdivpd, Vdivpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vdivpd, Vdivpd, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vdivpd, Vdivpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vdivpd, Vdivpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vdivps, Vdivps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vdivps, Vdivps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vdivps, Vdivps, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vdivps, Vdivps, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vdivps, Vdivps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vdivps, Vdivps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vdivsd, Vdivsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vdivsd, Vdivsd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vdivss, Vdivss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vdivss, Vdivss, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_4i(vdppd, Vdppd, X86Xmm, X86Xmm, X86Xmm, Imm)                   // AVX
  ASMJIT_INST_4i(vdppd, Vdppd, X86Xmm, X86Xmm, X86Mem, Imm)                   // AVX
  ASMJIT_INST_4i(vdpps, Vdpps, X86Xmm, X86Xmm, X86Xmm, Imm)                   // AVX
  ASMJIT_INST_4i(vdpps, Vdpps, X86Xmm, X86Xmm, X86Mem, Imm)                   // AVX
  ASMJIT_INST_4i(vdpps, Vdpps, X86Ymm, X86Ymm, X86Ymm, Imm)                   // AVX
  ASMJIT_INST_4i(vdpps, Vdpps, X86Ymm, X86Ymm, X86Mem, Imm)                   // AVX
  ASMJIT_INST_2x(vexp2pd, Vexp2pd, X86Zmm, X86Zmm)                            //      AVX512_ER{kz|sae|b64}
  ASMJIT_INST_2x(vexp2pd, Vexp2pd, X86Zmm, X86Mem)                            //      AVX512_ER{kz|sae|b64}
  ASMJIT_INST_2x(vexp2ps, Vexp2ps, X86Zmm, X86Zmm)                            //      AVX512_ER{kz|sae|b32}
  ASMJIT_INST_2x(vexp2ps, Vexp2ps, X86Zmm, X86Mem)                            //      AVX512_ER{kz|sae|b32}
  ASMJIT_INST_2x(vexpandpd, Vexpandpd, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandpd, Vexpandpd, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandpd, Vexpandpd, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandpd, Vexpandpd, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandpd, Vexpandpd, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vexpandpd, Vexpandpd, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vexpandps, Vexpandps, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandps, Vexpandps, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandps, Vexpandps, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandps, Vexpandps, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vexpandps, Vexpandps, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vexpandps, Vexpandps, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_3i(vextractf128, Vextractf128, X86Xmm, X86Ymm, Imm)             // AVX
  ASMJIT_INST_3i(vextractf128, Vextractf128, X86Mem, X86Ymm, Imm)             // AVX
  ASMJIT_INST_3i(vextractf32x4, Vextractf32x4, X86Xmm, X86Ymm, Imm)           //      AVX512_F{kz}-VL
  ASMJIT_INST_3i(vextractf32x4, Vextractf32x4, X86Mem, X86Ymm, Imm)           //      AVX512_F{kz}-VL
  ASMJIT_INST_3i(vextractf32x4, Vextractf32x4, X86Xmm, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextractf32x4, Vextractf32x4, X86Mem, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextractf32x8, Vextractf32x8, X86Ymm, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextractf32x8, Vextractf32x8, X86Mem, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextractf64x2, Vextractf64x2, X86Xmm, X86Ymm, Imm)           //      AVX512_DQ{kz}-VL
  ASMJIT_INST_3i(vextractf64x2, Vextractf64x2, X86Mem, X86Ymm, Imm)           //      AVX512_DQ{kz}-VL
  ASMJIT_INST_3i(vextractf64x2, Vextractf64x2, X86Xmm, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextractf64x2, Vextractf64x2, X86Mem, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextractf64x4, Vextractf64x4, X86Ymm, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextractf64x4, Vextractf64x4, X86Mem, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextracti128, Vextracti128, X86Xmm, X86Ymm, Imm)             // AVX2
  ASMJIT_INST_3i(vextracti128, Vextracti128, X86Mem, X86Ymm, Imm)             // AVX2
  ASMJIT_INST_3i(vextracti32x4, Vextracti32x4, X86Xmm, X86Ymm, Imm)           //      AVX512_F{kz}-VL
  ASMJIT_INST_3i(vextracti32x4, Vextracti32x4, X86Mem, X86Ymm, Imm)           //      AVX512_F{kz}-VL
  ASMJIT_INST_3i(vextracti32x4, Vextracti32x4, X86Xmm, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextracti32x4, Vextracti32x4, X86Mem, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextracti32x8, Vextracti32x8, X86Ymm, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextracti32x8, Vextracti32x8, X86Mem, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextracti64x2, Vextracti64x2, X86Xmm, X86Ymm, Imm)           //      AVX512_DQ{kz}-VL
  ASMJIT_INST_3i(vextracti64x2, Vextracti64x2, X86Mem, X86Ymm, Imm)           //      AVX512_DQ{kz}-VL
  ASMJIT_INST_3i(vextracti64x2, Vextracti64x2, X86Xmm, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextracti64x2, Vextracti64x2, X86Mem, X86Zmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vextracti64x4, Vextracti64x4, X86Ymm, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextracti64x4, Vextracti64x4, X86Mem, X86Zmm, Imm)           //      AVX512_F{kz}
  ASMJIT_INST_3i(vextractps, Vextractps, X86Gp, X86Xmm, Imm)                  // AVX  AVX512_F
  ASMJIT_INST_3i(vextractps, Vextractps, X86Mem, X86Xmm, Imm)                 // AVX  AVX512_F
  ASMJIT_INST_4i(vfixupimmpd, Vfixupimmpd, X86Xmm, X86Xmm, X86Xmm, Imm)       //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vfixupimmpd, Vfixupimmpd, X86Xmm, X86Xmm, X86Mem, Imm)       //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vfixupimmpd, Vfixupimmpd, X86Ymm, X86Ymm, X86Ymm, Imm)       //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vfixupimmpd, Vfixupimmpd, X86Ymm, X86Ymm, X86Mem, Imm)       //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vfixupimmpd, Vfixupimmpd, X86Zmm, X86Zmm, X86Zmm, Imm)       //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_4i(vfixupimmpd, Vfixupimmpd, X86Zmm, X86Zmm, X86Mem, Imm)       //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_4i(vfixupimmps, Vfixupimmps, X86Xmm, X86Xmm, X86Xmm, Imm)       //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vfixupimmps, Vfixupimmps, X86Xmm, X86Xmm, X86Mem, Imm)       //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vfixupimmps, Vfixupimmps, X86Ymm, X86Ymm, X86Ymm, Imm)       //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vfixupimmps, Vfixupimmps, X86Ymm, X86Ymm, X86Mem, Imm)       //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vfixupimmps, Vfixupimmps, X86Zmm, X86Zmm, X86Zmm, Imm)       //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_4i(vfixupimmps, Vfixupimmps, X86Zmm, X86Zmm, X86Mem, Imm)       //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_4i(vfixupimmsd, Vfixupimmsd, X86Xmm, X86Xmm, X86Xmm, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vfixupimmsd, Vfixupimmsd, X86Xmm, X86Xmm, X86Mem, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vfixupimmss, Vfixupimmss, X86Xmm, X86Xmm, X86Xmm, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vfixupimmss, Vfixupimmss, X86Xmm, X86Xmm, X86Mem, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_3x(vfmadd132pd, Vfmadd132pd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd132pd, Vfmadd132pd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd132pd, Vfmadd132pd, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd132pd, Vfmadd132pd, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd132pd, Vfmadd132pd, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmadd132pd, Vfmadd132pd, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmadd132ps, Vfmadd132ps, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd132ps, Vfmadd132ps, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd132ps, Vfmadd132ps, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd132ps, Vfmadd132ps, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd132ps, Vfmadd132ps, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmadd132ps, Vfmadd132ps, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmadd132sd, Vfmadd132sd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd132sd, Vfmadd132sd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd132ss, Vfmadd132ss, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd132ss, Vfmadd132ss, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd213pd, Vfmadd213pd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd213pd, Vfmadd213pd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd213pd, Vfmadd213pd, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd213pd, Vfmadd213pd, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd213pd, Vfmadd213pd, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmadd213pd, Vfmadd213pd, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmadd213ps, Vfmadd213ps, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd213ps, Vfmadd213ps, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd213ps, Vfmadd213ps, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd213ps, Vfmadd213ps, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd213ps, Vfmadd213ps, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmadd213ps, Vfmadd213ps, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmadd213sd, Vfmadd213sd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd213sd, Vfmadd213sd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd213ss, Vfmadd213ss, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd213ss, Vfmadd213ss, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd231pd, Vfmadd231pd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd231pd, Vfmadd231pd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd231pd, Vfmadd231pd, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd231pd, Vfmadd231pd, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmadd231pd, Vfmadd231pd, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmadd231pd, Vfmadd231pd, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmadd231ps, Vfmadd231ps, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd231ps, Vfmadd231ps, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd231ps, Vfmadd231ps, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd231ps, Vfmadd231ps, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmadd231ps, Vfmadd231ps, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmadd231ps, Vfmadd231ps, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmadd231sd, Vfmadd231sd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd231sd, Vfmadd231sd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd231ss, Vfmadd231ss, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmadd231ss, Vfmadd231ss, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmaddsub132pd, Vfmaddsub132pd, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub132pd, Vfmaddsub132pd, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub132pd, Vfmaddsub132pd, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub132pd, Vfmaddsub132pd, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub132pd, Vfmaddsub132pd, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmaddsub132pd, Vfmaddsub132pd, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmaddsub132ps, Vfmaddsub132ps, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub132ps, Vfmaddsub132ps, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub132ps, Vfmaddsub132ps, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub132ps, Vfmaddsub132ps, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub132ps, Vfmaddsub132ps, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmaddsub132ps, Vfmaddsub132ps, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmaddsub213pd, Vfmaddsub213pd, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub213pd, Vfmaddsub213pd, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub213pd, Vfmaddsub213pd, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub213pd, Vfmaddsub213pd, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub213pd, Vfmaddsub213pd, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmaddsub213pd, Vfmaddsub213pd, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmaddsub213ps, Vfmaddsub213ps, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub213ps, Vfmaddsub213ps, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub213ps, Vfmaddsub213ps, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub213ps, Vfmaddsub213ps, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub213ps, Vfmaddsub213ps, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmaddsub213ps, Vfmaddsub213ps, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmaddsub231pd, Vfmaddsub231pd, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub231pd, Vfmaddsub231pd, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub231pd, Vfmaddsub231pd, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub231pd, Vfmaddsub231pd, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmaddsub231pd, Vfmaddsub231pd, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmaddsub231pd, Vfmaddsub231pd, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmaddsub231ps, Vfmaddsub231ps, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub231ps, Vfmaddsub231ps, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub231ps, Vfmaddsub231ps, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub231ps, Vfmaddsub231ps, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmaddsub231ps, Vfmaddsub231ps, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmaddsub231ps, Vfmaddsub231ps, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub132pd, Vfmsub132pd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub132pd, Vfmsub132pd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub132pd, Vfmsub132pd, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub132pd, Vfmsub132pd, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub132pd, Vfmsub132pd, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsub132pd, Vfmsub132pd, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsub132ps, Vfmsub132ps, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub132ps, Vfmsub132ps, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub132ps, Vfmsub132ps, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub132ps, Vfmsub132ps, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub132ps, Vfmsub132ps, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub132ps, Vfmsub132ps, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub132sd, Vfmsub132sd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub132sd, Vfmsub132sd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub132ss, Vfmsub132ss, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub132ss, Vfmsub132ss, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub213pd, Vfmsub213pd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub213pd, Vfmsub213pd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub213pd, Vfmsub213pd, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub213pd, Vfmsub213pd, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub213pd, Vfmsub213pd, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsub213pd, Vfmsub213pd, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsub213ps, Vfmsub213ps, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub213ps, Vfmsub213ps, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub213ps, Vfmsub213ps, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub213ps, Vfmsub213ps, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub213ps, Vfmsub213ps, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub213ps, Vfmsub213ps, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub213sd, Vfmsub213sd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub213sd, Vfmsub213sd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub213ss, Vfmsub213ss, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub213ss, Vfmsub213ss, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub231pd, Vfmsub231pd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub231pd, Vfmsub231pd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub231pd, Vfmsub231pd, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub231pd, Vfmsub231pd, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsub231pd, Vfmsub231pd, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsub231pd, Vfmsub231pd, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsub231ps, Vfmsub231ps, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub231ps, Vfmsub231ps, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub231ps, Vfmsub231ps, X86Ymm, X86Ymm, X86Ymm)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub231ps, Vfmsub231ps, X86Ymm, X86Ymm, X86Mem)            // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsub231ps, Vfmsub231ps, X86Zmm, X86Zmm, X86Zmm)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub231ps, Vfmsub231ps, X86Zmm, X86Zmm, X86Mem)            // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsub231sd, Vfmsub231sd, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub231sd, Vfmsub231sd, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub231ss, Vfmsub231ss, X86Xmm, X86Xmm, X86Xmm)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsub231ss, Vfmsub231ss, X86Xmm, X86Xmm, X86Mem)            // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfmsubadd132pd, Vfmsubadd132pd, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd132pd, Vfmsubadd132pd, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd132pd, Vfmsubadd132pd, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd132pd, Vfmsubadd132pd, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd132pd, Vfmsubadd132pd, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsubadd132pd, Vfmsubadd132pd, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsubadd132ps, Vfmsubadd132ps, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd132ps, Vfmsubadd132ps, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd132ps, Vfmsubadd132ps, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd132ps, Vfmsubadd132ps, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd132ps, Vfmsubadd132ps, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsubadd132ps, Vfmsubadd132ps, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsubadd213pd, Vfmsubadd213pd, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd213pd, Vfmsubadd213pd, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd213pd, Vfmsubadd213pd, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd213pd, Vfmsubadd213pd, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd213pd, Vfmsubadd213pd, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsubadd213pd, Vfmsubadd213pd, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsubadd213ps, Vfmsubadd213ps, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd213ps, Vfmsubadd213ps, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd213ps, Vfmsubadd213ps, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd213ps, Vfmsubadd213ps, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd213ps, Vfmsubadd213ps, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsubadd213ps, Vfmsubadd213ps, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsubadd231pd, Vfmsubadd231pd, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd231pd, Vfmsubadd231pd, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd231pd, Vfmsubadd231pd, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd231pd, Vfmsubadd231pd, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfmsubadd231pd, Vfmsubadd231pd, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsubadd231pd, Vfmsubadd231pd, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfmsubadd231ps, Vfmsubadd231ps, X86Xmm, X86Xmm, X86Xmm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd231ps, Vfmsubadd231ps, X86Xmm, X86Xmm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd231ps, Vfmsubadd231ps, X86Ymm, X86Ymm, X86Ymm)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd231ps, Vfmsubadd231ps, X86Ymm, X86Ymm, X86Mem)      // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfmsubadd231ps, Vfmsubadd231ps, X86Zmm, X86Zmm, X86Zmm)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfmsubadd231ps, Vfmsubadd231ps, X86Zmm, X86Zmm, X86Mem)      // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd132pd, Vfnmadd132pd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd132pd, Vfnmadd132pd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd132pd, Vfnmadd132pd, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd132pd, Vfnmadd132pd, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd132pd, Vfnmadd132pd, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmadd132pd, Vfnmadd132pd, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmadd132ps, Vfnmadd132ps, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd132ps, Vfnmadd132ps, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd132ps, Vfnmadd132ps, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd132ps, Vfnmadd132ps, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd132ps, Vfnmadd132ps, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd132ps, Vfnmadd132ps, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd132sd, Vfnmadd132sd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd132sd, Vfnmadd132sd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd132ss, Vfnmadd132ss, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd132ss, Vfnmadd132ss, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd213pd, Vfnmadd213pd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd213pd, Vfnmadd213pd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd213pd, Vfnmadd213pd, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd213pd, Vfnmadd213pd, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd213pd, Vfnmadd213pd, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmadd213pd, Vfnmadd213pd, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmadd213ps, Vfnmadd213ps, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd213ps, Vfnmadd213ps, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd213ps, Vfnmadd213ps, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd213ps, Vfnmadd213ps, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd213ps, Vfnmadd213ps, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd213ps, Vfnmadd213ps, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd213sd, Vfnmadd213sd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd213sd, Vfnmadd213sd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd213ss, Vfnmadd213ss, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd213ss, Vfnmadd213ss, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd231pd, Vfnmadd231pd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd231pd, Vfnmadd231pd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd231pd, Vfnmadd231pd, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd231pd, Vfnmadd231pd, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmadd231pd, Vfnmadd231pd, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmadd231pd, Vfnmadd231pd, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmadd231ps, Vfnmadd231ps, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd231ps, Vfnmadd231ps, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd231ps, Vfnmadd231ps, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd231ps, Vfnmadd231ps, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmadd231ps, Vfnmadd231ps, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd231ps, Vfnmadd231ps, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmadd231sd, Vfnmadd231sd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd231sd, Vfnmadd231sd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd231ss, Vfnmadd231ss, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmadd231ss, Vfnmadd231ss, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub132pd, Vfnmsub132pd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub132pd, Vfnmsub132pd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub132pd, Vfnmsub132pd, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub132pd, Vfnmsub132pd, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub132pd, Vfnmsub132pd, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmsub132pd, Vfnmsub132pd, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmsub132ps, Vfnmsub132ps, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub132ps, Vfnmsub132ps, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub132ps, Vfnmsub132ps, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub132ps, Vfnmsub132ps, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub132ps, Vfnmsub132ps, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmsub132ps, Vfnmsub132ps, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmsub132sd, Vfnmsub132sd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub132sd, Vfnmsub132sd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub132ss, Vfnmsub132ss, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub132ss, Vfnmsub132ss, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub213pd, Vfnmsub213pd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub213pd, Vfnmsub213pd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub213pd, Vfnmsub213pd, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub213pd, Vfnmsub213pd, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub213pd, Vfnmsub213pd, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmsub213pd, Vfnmsub213pd, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmsub213ps, Vfnmsub213ps, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub213ps, Vfnmsub213ps, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub213ps, Vfnmsub213ps, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub213ps, Vfnmsub213ps, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub213ps, Vfnmsub213ps, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmsub213ps, Vfnmsub213ps, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmsub213sd, Vfnmsub213sd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub213sd, Vfnmsub213sd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub213ss, Vfnmsub213ss, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub213ss, Vfnmsub213ss, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub231pd, Vfnmsub231pd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub231pd, Vfnmsub231pd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub231pd, Vfnmsub231pd, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub231pd, Vfnmsub231pd, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vfnmsub231pd, Vfnmsub231pd, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmsub231pd, Vfnmsub231pd, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vfnmsub231ps, Vfnmsub231ps, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub231ps, Vfnmsub231ps, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub231ps, Vfnmsub231ps, X86Ymm, X86Ymm, X86Ymm)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub231ps, Vfnmsub231ps, X86Ymm, X86Ymm, X86Mem)          // FMA  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vfnmsub231ps, Vfnmsub231ps, X86Zmm, X86Zmm, X86Zmm)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmsub231ps, Vfnmsub231ps, X86Zmm, X86Zmm, X86Mem)          // FMA  AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vfnmsub231sd, Vfnmsub231sd, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub231sd, Vfnmsub231sd, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub231ss, Vfnmsub231ss, X86Xmm, X86Xmm, X86Xmm)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3x(vfnmsub231ss, Vfnmsub231ss, X86Xmm, X86Xmm, X86Mem)          // FMA  AVX512_F{kz|er}
  ASMJIT_INST_3i(vfpclasspd, Vfpclasspd, X86KReg, X86Xmm, Imm)                //      AVX512_DQ{k|b64}-VL
  ASMJIT_INST_3i(vfpclasspd, Vfpclasspd, X86KReg, X86Mem, Imm)                //      AVX512_DQ{k|b64} AVX512_DQ{k|b64}-VL
  ASMJIT_INST_3i(vfpclasspd, Vfpclasspd, X86KReg, X86Ymm, Imm)                //      AVX512_DQ{k|b64}-VL
  ASMJIT_INST_3i(vfpclasspd, Vfpclasspd, X86KReg, X86Zmm, Imm)                //      AVX512_DQ{k|b64}
  ASMJIT_INST_3i(vfpclassps, Vfpclassps, X86KReg, X86Xmm, Imm)                //      AVX512_DQ{k|b32}-VL
  ASMJIT_INST_3i(vfpclassps, Vfpclassps, X86KReg, X86Mem, Imm)                //      AVX512_DQ{k|b32} AVX512_DQ{k|b32}-VL
  ASMJIT_INST_3i(vfpclassps, Vfpclassps, X86KReg, X86Ymm, Imm)                //      AVX512_DQ{k|b32}-VL
  ASMJIT_INST_3i(vfpclassps, Vfpclassps, X86KReg, X86Zmm, Imm)                //      AVX512_DQ{k|b32}
  ASMJIT_INST_3i(vfpclasssd, Vfpclasssd, X86KReg, X86Xmm, Imm)                //      AVX512_DQ{k}
  ASMJIT_INST_3i(vfpclasssd, Vfpclasssd, X86KReg, X86Mem, Imm)                //      AVX512_DQ{k}
  ASMJIT_INST_3i(vfpclassss, Vfpclassss, X86KReg, X86Xmm, Imm)                //      AVX512_DQ{k}
  ASMJIT_INST_3i(vfpclassss, Vfpclassss, X86KReg, X86Mem, Imm)                //      AVX512_DQ{k}
  ASMJIT_INST_3x(vgatherdpd, Vgatherdpd, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vgatherdpd, Vgatherdpd, X86Ymm, X86Mem, X86Ymm)              // AVX2
  ASMJIT_INST_2x(vgatherdpd, Vgatherdpd, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherdpd, Vgatherdpd, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherdpd, Vgatherdpd, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_3x(vgatherdps, Vgatherdps, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vgatherdps, Vgatherdps, X86Ymm, X86Mem, X86Ymm)              // AVX2
  ASMJIT_INST_2x(vgatherdps, Vgatherdps, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherdps, Vgatherdps, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherdps, Vgatherdps, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_1x(vgatherpf0dpd, Vgatherpf0dpd, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf0dps, Vgatherpf0dps, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf0qpd, Vgatherpf0qpd, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf0qps, Vgatherpf0qps, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf1dpd, Vgatherpf1dpd, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf1dps, Vgatherpf1dps, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf1qpd, Vgatherpf1qpd, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_1x(vgatherpf1qps, Vgatherpf1qps, X86Mem)                        //      AVX512_PF{k}
  ASMJIT_INST_3x(vgatherqpd, Vgatherqpd, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vgatherqpd, Vgatherqpd, X86Ymm, X86Mem, X86Ymm)              // AVX2
  ASMJIT_INST_2x(vgatherqpd, Vgatherqpd, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherqpd, Vgatherqpd, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherqpd, Vgatherqpd, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_3x(vgatherqps, Vgatherqps, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_2x(vgatherqps, Vgatherqps, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherqps, Vgatherqps, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vgatherqps, Vgatherqps, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_2x(vgetexppd, Vgetexppd, X86Xmm, X86Xmm)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vgetexppd, Vgetexppd, X86Xmm, X86Mem)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vgetexppd, Vgetexppd, X86Ymm, X86Ymm)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vgetexppd, Vgetexppd, X86Ymm, X86Mem)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vgetexppd, Vgetexppd, X86Zmm, X86Zmm)                        //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vgetexppd, Vgetexppd, X86Zmm, X86Mem)                        //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_2x(vgetexpps, Vgetexpps, X86Xmm, X86Xmm)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vgetexpps, Vgetexpps, X86Xmm, X86Mem)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vgetexpps, Vgetexpps, X86Ymm, X86Ymm)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vgetexpps, Vgetexpps, X86Ymm, X86Mem)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vgetexpps, Vgetexpps, X86Zmm, X86Zmm)                        //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_2x(vgetexpps, Vgetexpps, X86Zmm, X86Mem)                        //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_2x(vgetexpsd, Vgetexpsd, X86Xmm, X86Xmm)                        //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vgetexpsd, Vgetexpsd, X86Xmm, X86Mem)                        //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vgetexpss, Vgetexpss, X86Xmm, X86Xmm)                        //      AVX512_F{kz|sae}
  ASMJIT_INST_2x(vgetexpss, Vgetexpss, X86Xmm, X86Mem)                        //      AVX512_F{kz|sae}
  ASMJIT_INST_3i(vgetmantpd, Vgetmantpd, X86Xmm, X86Xmm, Imm)                 //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vgetmantpd, Vgetmantpd, X86Xmm, X86Mem, Imm)                 //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vgetmantpd, Vgetmantpd, X86Ymm, X86Ymm, Imm)                 //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vgetmantpd, Vgetmantpd, X86Ymm, X86Mem, Imm)                 //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vgetmantpd, Vgetmantpd, X86Zmm, X86Zmm, Imm)                 //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3i(vgetmantpd, Vgetmantpd, X86Zmm, X86Mem, Imm)                 //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3i(vgetmantps, Vgetmantps, X86Xmm, X86Xmm, Imm)                 //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vgetmantps, Vgetmantps, X86Xmm, X86Mem, Imm)                 //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vgetmantps, Vgetmantps, X86Ymm, X86Ymm, Imm)                 //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vgetmantps, Vgetmantps, X86Ymm, X86Mem, Imm)                 //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vgetmantps, Vgetmantps, X86Zmm, X86Zmm, Imm)                 //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3i(vgetmantps, Vgetmantps, X86Zmm, X86Mem, Imm)                 //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3i(vgetmantsd, Vgetmantsd, X86Xmm, X86Xmm, Imm)                 //      AVX512_F{kz|sae}
  ASMJIT_INST_3i(vgetmantsd, Vgetmantsd, X86Xmm, X86Mem, Imm)                 //      AVX512_F{kz|sae}
  ASMJIT_INST_3i(vgetmantss, Vgetmantss, X86Xmm, X86Xmm, Imm)                 //      AVX512_F{kz|sae}
  ASMJIT_INST_3i(vgetmantss, Vgetmantss, X86Xmm, X86Mem, Imm)                 //      AVX512_F{kz|sae}
  ASMJIT_INST_3x(vhaddpd, Vhaddpd, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vhaddpd, Vhaddpd, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhaddpd, Vhaddpd, X86Ymm, X86Ymm, X86Ymm)                    // AVX
  ASMJIT_INST_3x(vhaddpd, Vhaddpd, X86Ymm, X86Ymm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhaddps, Vhaddps, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vhaddps, Vhaddps, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhaddps, Vhaddps, X86Ymm, X86Ymm, X86Ymm)                    // AVX
  ASMJIT_INST_3x(vhaddps, Vhaddps, X86Ymm, X86Ymm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhsubpd, Vhsubpd, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vhsubpd, Vhsubpd, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhsubpd, Vhsubpd, X86Ymm, X86Ymm, X86Ymm)                    // AVX
  ASMJIT_INST_3x(vhsubpd, Vhsubpd, X86Ymm, X86Ymm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhsubps, Vhsubps, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vhsubps, Vhsubps, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vhsubps, Vhsubps, X86Ymm, X86Ymm, X86Ymm)                    // AVX
  ASMJIT_INST_3x(vhsubps, Vhsubps, X86Ymm, X86Ymm, X86Mem)                    // AVX
  ASMJIT_INST_4i(vinsertf128, Vinsertf128, X86Ymm, X86Ymm, X86Xmm, Imm)       // AVX
  ASMJIT_INST_4i(vinsertf128, Vinsertf128, X86Ymm, X86Ymm, X86Mem, Imm)       // AVX
  ASMJIT_INST_4i(vinsertf32x4, Vinsertf32x4, X86Ymm, X86Ymm, X86Xmm, Imm)     //      AVX512_F{kz}-VL
  ASMJIT_INST_4i(vinsertf32x4, Vinsertf32x4, X86Ymm, X86Ymm, X86Mem, Imm)     //      AVX512_F{kz}-VL
  ASMJIT_INST_4i(vinsertf32x4, Vinsertf32x4, X86Zmm, X86Zmm, X86Xmm, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinsertf32x4, Vinsertf32x4, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinsertf32x8, Vinsertf32x8, X86Zmm, X86Zmm, X86Ymm, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinsertf32x8, Vinsertf32x8, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinsertf64x2, Vinsertf64x2, X86Ymm, X86Ymm, X86Xmm, Imm)     //      AVX512_DQ{kz}-VL
  ASMJIT_INST_4i(vinsertf64x2, Vinsertf64x2, X86Ymm, X86Ymm, X86Mem, Imm)     //      AVX512_DQ{kz}-VL
  ASMJIT_INST_4i(vinsertf64x2, Vinsertf64x2, X86Zmm, X86Zmm, X86Xmm, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinsertf64x2, Vinsertf64x2, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinsertf64x4, Vinsertf64x4, X86Zmm, X86Zmm, X86Ymm, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinsertf64x4, Vinsertf64x4, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinserti128, Vinserti128, X86Ymm, X86Ymm, X86Xmm, Imm)       // AVX2
  ASMJIT_INST_4i(vinserti128, Vinserti128, X86Ymm, X86Ymm, X86Mem, Imm)       // AVX2
  ASMJIT_INST_4i(vinserti32x4, Vinserti32x4, X86Ymm, X86Ymm, X86Xmm, Imm)     //      AVX512_F{kz}-VL
  ASMJIT_INST_4i(vinserti32x4, Vinserti32x4, X86Ymm, X86Ymm, X86Mem, Imm)     //      AVX512_F{kz}-VL
  ASMJIT_INST_4i(vinserti32x4, Vinserti32x4, X86Zmm, X86Zmm, X86Xmm, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinserti32x4, Vinserti32x4, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinserti32x8, Vinserti32x8, X86Zmm, X86Zmm, X86Ymm, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinserti32x8, Vinserti32x8, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinserti64x2, Vinserti64x2, X86Ymm, X86Ymm, X86Xmm, Imm)     //      AVX512_DQ{kz}-VL
  ASMJIT_INST_4i(vinserti64x2, Vinserti64x2, X86Ymm, X86Ymm, X86Mem, Imm)     //      AVX512_DQ{kz}-VL
  ASMJIT_INST_4i(vinserti64x2, Vinserti64x2, X86Zmm, X86Zmm, X86Xmm, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinserti64x2, Vinserti64x2, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vinserti64x4, Vinserti64x4, X86Zmm, X86Zmm, X86Ymm, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinserti64x4, Vinserti64x4, X86Zmm, X86Zmm, X86Mem, Imm)     //      AVX512_F{kz}
  ASMJIT_INST_4i(vinsertps, Vinsertps, X86Xmm, X86Xmm, X86Xmm, Imm)           // AVX  AVX512_F
  ASMJIT_INST_4i(vinsertps, Vinsertps, X86Xmm, X86Xmm, X86Mem, Imm)           // AVX  AVX512_F
  ASMJIT_INST_2x(vlddqu, Vlddqu, X86Xmm, X86Mem)                              // AVX
  ASMJIT_INST_2x(vlddqu, Vlddqu, X86Ymm, X86Mem)                              // AVX
  ASMJIT_INST_1x(vldmxcsr, Vldmxcsr, X86Mem)                                  // AVX
  ASMJIT_INST_3x(vmaskmovdqu, Vmaskmovdqu, X86Xmm, X86Xmm, DS_ZDI)            // AVX  [EXPLICIT]
  ASMJIT_INST_3x(vmaskmovpd, Vmaskmovpd, X86Mem, X86Xmm, X86Xmm)              // AVX
  ASMJIT_INST_3x(vmaskmovpd, Vmaskmovpd, X86Mem, X86Ymm, X86Ymm)              // AVX
  ASMJIT_INST_3x(vmaskmovpd, Vmaskmovpd, X86Xmm, X86Xmm, X86Mem)              // AVX
  ASMJIT_INST_3x(vmaskmovpd, Vmaskmovpd, X86Ymm, X86Ymm, X86Mem)              // AVX
  ASMJIT_INST_3x(vmaskmovps, Vmaskmovps, X86Mem, X86Xmm, X86Xmm)              // AVX
  ASMJIT_INST_3x(vmaskmovps, Vmaskmovps, X86Mem, X86Ymm, X86Ymm)              // AVX
  ASMJIT_INST_3x(vmaskmovps, Vmaskmovps, X86Xmm, X86Xmm, X86Mem)              // AVX
  ASMJIT_INST_3x(vmaskmovps, Vmaskmovps, X86Ymm, X86Ymm, X86Mem)              // AVX
  ASMJIT_INST_3x(vmaxpd, Vmaxpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmaxpd, Vmaxpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmaxpd, Vmaxpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmaxpd, Vmaxpd, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmaxpd, Vmaxpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3x(vmaxpd, Vmaxpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3x(vmaxps, Vmaxps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmaxps, Vmaxps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmaxps, Vmaxps, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmaxps, Vmaxps, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmaxps, Vmaxps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3x(vmaxps, Vmaxps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3x(vmaxsd, Vmaxsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vmaxsd, Vmaxsd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vmaxss, Vmaxss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vmaxss, Vmaxss, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vminpd, Vminpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vminpd, Vminpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vminpd, Vminpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vminpd, Vminpd, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vminpd, Vminpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3x(vminpd, Vminpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3x(vminps, Vminps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vminps, Vminps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vminps, Vminps, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vminps, Vminps, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vminps, Vminps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3x(vminps, Vminps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3x(vminsd, Vminsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vminsd, Vminsd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vminss, Vminss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_3x(vminss, Vminss, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|sae}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Xmm, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Xmm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Mem, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Ymm, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Ymm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Mem, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Zmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Zmm, X86Mem)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovapd, Vmovapd, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Xmm, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Xmm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Mem, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Ymm, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Ymm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Mem, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Zmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Zmm, X86Mem)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovaps, Vmovaps, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovd, Vmovd, X86Gp, X86Xmm)                                 // AVX  AVX512_F
  ASMJIT_INST_2x(vmovd, Vmovd, X86Mem, X86Xmm)                                // AVX  AVX512_F
  ASMJIT_INST_2x(vmovd, Vmovd, X86Xmm, X86Gp)                                 // AVX  AVX512_F
  ASMJIT_INST_2x(vmovd, Vmovd, X86Xmm, X86Mem)                                // AVX  AVX512_F
  ASMJIT_INST_2x(vmovddup, Vmovddup, X86Xmm, X86Xmm)                          // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovddup, Vmovddup, X86Xmm, X86Mem)                          // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovddup, Vmovddup, X86Ymm, X86Ymm)                          // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovddup, Vmovddup, X86Ymm, X86Mem)                          // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovddup, Vmovddup, X86Zmm, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovddup, Vmovddup, X86Zmm, X86Mem)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqa, Vmovdqa, X86Xmm, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vmovdqa, Vmovdqa, X86Xmm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vmovdqa, Vmovdqa, X86Mem, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vmovdqa, Vmovdqa, X86Ymm, X86Ymm)                            // AVX
  ASMJIT_INST_2x(vmovdqa, Vmovdqa, X86Ymm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vmovdqa, Vmovdqa, X86Mem, X86Ymm)                            // AVX
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqa32, Vmovdqa32, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqa64, Vmovdqa64, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu, Vmovdqu, X86Xmm, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vmovdqu, Vmovdqu, X86Xmm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vmovdqu, Vmovdqu, X86Mem, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vmovdqu, Vmovdqu, X86Ymm, X86Ymm)                            // AVX
  ASMJIT_INST_2x(vmovdqu, Vmovdqu, X86Ymm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vmovdqu, Vmovdqu, X86Mem, X86Ymm)                            // AVX
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Xmm, X86Xmm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Xmm, X86Mem)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Mem, X86Xmm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Ymm, X86Ymm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Ymm, X86Mem)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Mem, X86Ymm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Zmm, X86Zmm)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Zmm, X86Mem)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vmovdqu16, Vmovdqu16, X86Mem, X86Zmm)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu32, Vmovdqu32, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu64, Vmovdqu64, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Xmm, X86Xmm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Xmm, X86Mem)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Mem, X86Xmm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Ymm, X86Ymm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Ymm, X86Mem)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Mem, X86Ymm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Zmm, X86Zmm)                          //      AVX512_BW{kz}
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Zmm, X86Mem)                          //      AVX512_BW{kz}
  ASMJIT_INST_2x(vmovdqu8, Vmovdqu8, X86Mem, X86Zmm)                          //      AVX512_BW{kz}
  ASMJIT_INST_3x(vmovhlps, Vmovhlps, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_F
  ASMJIT_INST_2x(vmovhpd, Vmovhpd, X86Mem, X86Xmm)                            // AVX  AVX512_F
  ASMJIT_INST_3x(vmovhpd, Vmovhpd, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F
  ASMJIT_INST_2x(vmovhps, Vmovhps, X86Mem, X86Xmm)                            // AVX  AVX512_F
  ASMJIT_INST_3x(vmovhps, Vmovhps, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F
  ASMJIT_INST_3x(vmovlhps, Vmovlhps, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_F
  ASMJIT_INST_2x(vmovlpd, Vmovlpd, X86Mem, X86Xmm)                            // AVX  AVX512_F
  ASMJIT_INST_3x(vmovlpd, Vmovlpd, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F
  ASMJIT_INST_2x(vmovlps, Vmovlps, X86Mem, X86Xmm)                            // AVX  AVX512_F
  ASMJIT_INST_3x(vmovlps, Vmovlps, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F
  ASMJIT_INST_2x(vmovmskpd, Vmovmskpd, X86Gp, X86Xmm)                         // AVX
  ASMJIT_INST_2x(vmovmskpd, Vmovmskpd, X86Gp, X86Ymm)                         // AVX
  ASMJIT_INST_2x(vmovmskps, Vmovmskps, X86Gp, X86Xmm)                         // AVX
  ASMJIT_INST_2x(vmovmskps, Vmovmskps, X86Gp, X86Ymm)                         // AVX
  ASMJIT_INST_2x(vmovntdq, Vmovntdq, X86Mem, X86Xmm)                          // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntdq, Vmovntdq, X86Mem, X86Ymm)                          // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntdq, Vmovntdq, X86Mem, X86Zmm)                          //      AVX512_F
  ASMJIT_INST_2x(vmovntdqa, Vmovntdqa, X86Xmm, X86Mem)                        // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntdqa, Vmovntdqa, X86Ymm, X86Mem)                        // AVX2 AVX512_F-VL
  ASMJIT_INST_2x(vmovntdqa, Vmovntdqa, X86Zmm, X86Mem)                        //      AVX512_F
  ASMJIT_INST_2x(vmovntpd, Vmovntpd, X86Mem, X86Xmm)                          // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntpd, Vmovntpd, X86Mem, X86Ymm)                          // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntpd, Vmovntpd, X86Mem, X86Zmm)                          //      AVX512_F
  ASMJIT_INST_2x(vmovntps, Vmovntps, X86Mem, X86Xmm)                          // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntps, Vmovntps, X86Mem, X86Ymm)                          // AVX  AVX512_F-VL
  ASMJIT_INST_2x(vmovntps, Vmovntps, X86Mem, X86Zmm)                          //      AVX512_F
  ASMJIT_INST_2x(vmovq, Vmovq, X86Gp, X86Xmm)                                 // AVX  AVX512_F
  ASMJIT_INST_2x(vmovq, Vmovq, X86Mem, X86Xmm)                                // AVX  AVX512_F
  ASMJIT_INST_2x(vmovq, Vmovq, X86Xmm, X86Mem)                                // AVX  AVX512_F
  ASMJIT_INST_2x(vmovq, Vmovq, X86Xmm, X86Gp)                                 // AVX  AVX512_F
  ASMJIT_INST_2x(vmovq, Vmovq, X86Xmm, X86Xmm)                                // AVX  AVX512_F
  ASMJIT_INST_2x(vmovsd, Vmovsd, X86Mem, X86Xmm)                              // AVX  AVX512_F
  ASMJIT_INST_2x(vmovsd, Vmovsd, X86Xmm, X86Mem)                              // AVX  AVX512_F{kz}
  ASMJIT_INST_3x(vmovsd, Vmovsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}
  ASMJIT_INST_2x(vmovshdup, Vmovshdup, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovshdup, Vmovshdup, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovshdup, Vmovshdup, X86Ymm, X86Ymm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovshdup, Vmovshdup, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovshdup, Vmovshdup, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovshdup, Vmovshdup, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovsldup, Vmovsldup, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovsldup, Vmovsldup, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovsldup, Vmovsldup, X86Ymm, X86Ymm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovsldup, Vmovsldup, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovsldup, Vmovsldup, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovsldup, Vmovsldup, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovss, Vmovss, X86Mem, X86Xmm)                              // AVX  AVX512_F
  ASMJIT_INST_2x(vmovss, Vmovss, X86Xmm, X86Mem)                              // AVX  AVX512_F{kz}
  ASMJIT_INST_3x(vmovss, Vmovss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Xmm, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Xmm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Mem, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Ymm, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Ymm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Mem, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Zmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Zmm, X86Mem)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovupd, Vmovupd, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovups, Vmovups, X86Xmm, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovups, Vmovups, X86Xmm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovups, Vmovups, X86Mem, X86Xmm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovups, Vmovups, X86Ymm, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovups, Vmovups, X86Ymm, X86Mem)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovups, Vmovups, X86Mem, X86Ymm)                            // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vmovups, Vmovups, X86Zmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovups, Vmovups, X86Zmm, X86Mem)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vmovups, Vmovups, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_4i(vmpsadbw, Vmpsadbw, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX
  ASMJIT_INST_4i(vmpsadbw, Vmpsadbw, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4i(vmpsadbw, Vmpsadbw, X86Ymm, X86Ymm, X86Ymm, Imm)             // AVX2
  ASMJIT_INST_4i(vmpsadbw, Vmpsadbw, X86Ymm, X86Ymm, X86Mem, Imm)             // AVX2
  ASMJIT_INST_3x(vmulpd, Vmulpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmulpd, Vmulpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmulpd, Vmulpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmulpd, Vmulpd, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vmulpd, Vmulpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vmulpd, Vmulpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vmulps, Vmulps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmulps, Vmulps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmulps, Vmulps, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmulps, Vmulps, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vmulps, Vmulps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vmulps, Vmulps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vmulsd, Vmulsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vmulsd, Vmulsd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vmulss, Vmulss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vmulss, Vmulss, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vorpd, Vorpd, X86Xmm, X86Xmm, X86Xmm)                        // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vorpd, Vorpd, X86Xmm, X86Xmm, X86Mem)                        // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vorpd, Vorpd, X86Ymm, X86Ymm, X86Ymm)                        // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vorpd, Vorpd, X86Ymm, X86Ymm, X86Mem)                        // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vorpd, Vorpd, X86Zmm, X86Zmm, X86Zmm)                        //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vorpd, Vorpd, X86Zmm, X86Zmm, X86Mem)                        //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vorps, Vorps, X86Xmm, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vorps, Vorps, X86Xmm, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vorps, Vorps, X86Ymm, X86Ymm, X86Ymm)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vorps, Vorps, X86Ymm, X86Ymm, X86Mem)                        // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vorps, Vorps, X86Zmm, X86Zmm, X86Zmm)                        //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vorps, Vorps, X86Zmm, X86Zmm, X86Mem)                        //      AVX512_F{kz|b32}
  ASMJIT_INST_6x(vp4dpwssd, Vp4dpwssd, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Mem)   // AVX512_4FMAPS{kz}
  ASMJIT_INST_6x(vp4dpwssds, Vp4dpwssds, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Zmm, X86Mem) // AVX512_4FMAPS{kz}
  ASMJIT_INST_2x(vpabsb, Vpabsb, X86Xmm, X86Xmm)                              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsb, Vpabsb, X86Xmm, X86Mem)                              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsb, Vpabsb, X86Ymm, X86Ymm)                              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsb, Vpabsb, X86Ymm, X86Mem)                              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsb, Vpabsb, X86Zmm, X86Zmm)                              //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpabsb, Vpabsb, X86Zmm, X86Mem)                              //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpabsd, Vpabsd, X86Xmm, X86Xmm)                              // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsd, Vpabsd, X86Xmm, X86Mem)                              // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsd, Vpabsd, X86Ymm, X86Ymm)                              // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsd, Vpabsd, X86Ymm, X86Mem)                              // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsd, Vpabsd, X86Zmm, X86Zmm)                              //      AVX512_F{kz}
  ASMJIT_INST_2x(vpabsd, Vpabsd, X86Zmm, X86Mem)                              //      AVX512_F{kz}
  ASMJIT_INST_2x(vpabsq, Vpabsq, X86Xmm, X86Xmm)                              //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsq, Vpabsq, X86Xmm, X86Mem)                              //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsq, Vpabsq, X86Ymm, X86Ymm)                              //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsq, Vpabsq, X86Ymm, X86Mem)                              //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpabsq, Vpabsq, X86Zmm, X86Zmm)                              //      AVX512_F{kz}
  ASMJIT_INST_2x(vpabsq, Vpabsq, X86Zmm, X86Mem)                              //      AVX512_F{kz}
  ASMJIT_INST_2x(vpabsw, Vpabsw, X86Xmm, X86Xmm)                              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsw, Vpabsw, X86Xmm, X86Mem)                              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsw, Vpabsw, X86Ymm, X86Ymm)                              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsw, Vpabsw, X86Ymm, X86Mem)                              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpabsw, Vpabsw, X86Zmm, X86Zmm)                              //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpabsw, Vpabsw, X86Zmm, X86Mem)                              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpackssdw, Vpackssdw, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackssdw, Vpackssdw, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackssdw, Vpackssdw, X86Ymm, X86Ymm, X86Ymm)                // AVX2 AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackssdw, Vpackssdw, X86Ymm, X86Ymm, X86Mem)                // AVX2 AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackssdw, Vpackssdw, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_BW{kz|b32}
  ASMJIT_INST_3x(vpackssdw, Vpackssdw, X86Zmm, X86Zmm, X86Mem)                //      AVX512_BW{kz|b32}
  ASMJIT_INST_3x(vpacksswb, Vpacksswb, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpacksswb, Vpacksswb, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpacksswb, Vpacksswb, X86Ymm, X86Ymm, X86Ymm)                // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpacksswb, Vpacksswb, X86Ymm, X86Ymm, X86Mem)                // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpacksswb, Vpacksswb, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpacksswb, Vpacksswb, X86Zmm, X86Zmm, X86Mem)                //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpackusdw, Vpackusdw, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackusdw, Vpackusdw, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackusdw, Vpackusdw, X86Ymm, X86Ymm, X86Ymm)                // AVX2 AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackusdw, Vpackusdw, X86Ymm, X86Ymm, X86Mem)                // AVX2 AVX512_BW{kz|b32}-VL
  ASMJIT_INST_3x(vpackusdw, Vpackusdw, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_BW{kz|b32}
  ASMJIT_INST_3x(vpackusdw, Vpackusdw, X86Zmm, X86Zmm, X86Mem)                //      AVX512_BW{kz|b32}
  ASMJIT_INST_3x(vpackuswb, Vpackuswb, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpackuswb, Vpackuswb, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpackuswb, Vpackuswb, X86Ymm, X86Ymm, X86Ymm)                // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpackuswb, Vpackuswb, X86Ymm, X86Ymm, X86Mem)                // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpackuswb, Vpackuswb, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpackuswb, Vpackuswb, X86Zmm, X86Zmm, X86Mem)                //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddb, Vpaddb, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddb, Vpaddb, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddb, Vpaddb, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddb, Vpaddb, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddb, Vpaddb, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddb, Vpaddb, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddd, Vpaddd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpaddd, Vpaddd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpaddd, Vpaddd, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpaddd, Vpaddd, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpaddd, Vpaddd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpaddd, Vpaddd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpaddq, Vpaddq, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpaddq, Vpaddq, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpaddq, Vpaddq, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpaddq, Vpaddq, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpaddq, Vpaddq, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpaddq, Vpaddq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpaddsb, Vpaddsb, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsb, Vpaddsb, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsb, Vpaddsb, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsb, Vpaddsb, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsb, Vpaddsb, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddsb, Vpaddsb, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddsw, Vpaddsw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsw, Vpaddsw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsw, Vpaddsw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsw, Vpaddsw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddsw, Vpaddsw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddsw, Vpaddsw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddusb, Vpaddusb, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusb, Vpaddusb, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusb, Vpaddusb, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusb, Vpaddusb, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusb, Vpaddusb, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddusb, Vpaddusb, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddusw, Vpaddusw, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusw, Vpaddusw, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusw, Vpaddusw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusw, Vpaddusw, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddusw, Vpaddusw, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddusw, Vpaddusw, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddw, Vpaddw, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddw, Vpaddw, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddw, Vpaddw, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddw, Vpaddw, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpaddw, Vpaddw, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpaddw, Vpaddw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_4i(vpalignr, Vpalignr, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vpalignr, Vpalignr, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vpalignr, Vpalignr, X86Ymm, X86Ymm, X86Ymm, Imm)             // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vpalignr, Vpalignr, X86Ymm, X86Ymm, X86Mem, Imm)             // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_4i(vpalignr, Vpalignr, X86Zmm, X86Zmm, X86Zmm, Imm)             //      AVX512_BW{kz}
  ASMJIT_INST_4i(vpalignr, Vpalignr, X86Zmm, X86Zmm, X86Mem, Imm)             //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpand, Vpand, X86Xmm, X86Xmm, X86Xmm)                        // AVX
  ASMJIT_INST_3x(vpand, Vpand, X86Xmm, X86Xmm, X86Mem)                        // AVX
  ASMJIT_INST_3x(vpand, Vpand, X86Ymm, X86Ymm, X86Ymm)                        // AVX2
  ASMJIT_INST_3x(vpand, Vpand, X86Ymm, X86Ymm, X86Mem)                        // AVX2
  ASMJIT_INST_3x(vpandd, Vpandd, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandd, Vpandd, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandd, Vpandd, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandd, Vpandd, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandd, Vpandd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpandd, Vpandd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpandn, Vpandn, X86Xmm, X86Xmm, X86Xmm)                      // AVX
  ASMJIT_INST_3x(vpandn, Vpandn, X86Xmm, X86Xmm, X86Mem)                      // AVX
  ASMJIT_INST_3x(vpandn, Vpandn, X86Ymm, X86Ymm, X86Ymm)                      // AVX2
  ASMJIT_INST_3x(vpandn, Vpandn, X86Ymm, X86Ymm, X86Mem)                      // AVX2
  ASMJIT_INST_3x(vpandnd, Vpandnd, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandnd, Vpandnd, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandnd, Vpandnd, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandnd, Vpandnd, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpandnd, Vpandnd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpandnd, Vpandnd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpandnq, Vpandnq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandnq, Vpandnq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandnq, Vpandnq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandnq, Vpandnq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandnq, Vpandnq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpandnq, Vpandnq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpandq, Vpandq, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandq, Vpandq, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandq, Vpandq, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandq, Vpandq, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpandq, Vpandq, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpandq, Vpandq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpavgb, Vpavgb, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgb, Vpavgb, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgb, Vpavgb, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgb, Vpavgb, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgb, Vpavgb, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpavgb, Vpavgb, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpavgw, Vpavgw, X86Xmm, X86Xmm, X86Xmm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgw, Vpavgw, X86Xmm, X86Xmm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgw, Vpavgw, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgw, Vpavgw, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpavgw, Vpavgw, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpavgw, Vpavgw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_4i(vpblendd, Vpblendd, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX2
  ASMJIT_INST_4i(vpblendd, Vpblendd, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX2
  ASMJIT_INST_4i(vpblendd, Vpblendd, X86Ymm, X86Ymm, X86Ymm, Imm)             // AVX2
  ASMJIT_INST_4i(vpblendd, Vpblendd, X86Ymm, X86Ymm, X86Mem, Imm)             // AVX2
  ASMJIT_INST_4x(vpblendvb, Vpblendvb, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // AVX
  ASMJIT_INST_4x(vpblendvb, Vpblendvb, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // AVX
  ASMJIT_INST_4x(vpblendvb, Vpblendvb, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // AVX2
  ASMJIT_INST_4x(vpblendvb, Vpblendvb, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // AVX2
  ASMJIT_INST_4i(vpblendw, Vpblendw, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX
  ASMJIT_INST_4i(vpblendw, Vpblendw, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4i(vpblendw, Vpblendw, X86Ymm, X86Ymm, X86Ymm, Imm)             // AVX2
  ASMJIT_INST_4i(vpblendw, Vpblendw, X86Ymm, X86Ymm, X86Mem, Imm)             // AVX2
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Xmm, X86Xmm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Xmm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Ymm, X86Xmm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Xmm, X86Gp)                   //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Ymm, X86Gp)                   //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Zmm, X86Gp)                   //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Zmm, X86Xmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpbroadcastb, Vpbroadcastb, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Xmm, X86Xmm)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Xmm, X86Mem)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Ymm, X86Xmm)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Ymm, X86Mem)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Xmm, X86Gp)                   //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Ymm, X86Gp)                   //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Zmm, X86Gp)                   //      AVX512_F{kz}
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Zmm, X86Xmm)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vpbroadcastd, Vpbroadcastd, X86Zmm, X86Mem)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vpbroadcastmb2d, Vpbroadcastmb2d, X86Xmm, X86KReg)           //      AVX512_CD-VL
  ASMJIT_INST_2x(vpbroadcastmb2d, Vpbroadcastmb2d, X86Ymm, X86KReg)           //      AVX512_CD-VL
  ASMJIT_INST_2x(vpbroadcastmb2d, Vpbroadcastmb2d, X86Zmm, X86KReg)           //      AVX512_CD
  ASMJIT_INST_2x(vpbroadcastmb2q, Vpbroadcastmb2q, X86Xmm, X86KReg)           //      AVX512_CD-VL
  ASMJIT_INST_2x(vpbroadcastmb2q, Vpbroadcastmb2q, X86Ymm, X86KReg)           //      AVX512_CD-VL
  ASMJIT_INST_2x(vpbroadcastmb2q, Vpbroadcastmb2q, X86Zmm, X86KReg)           //      AVX512_CD
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Xmm, X86Xmm)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Xmm, X86Mem)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Ymm, X86Xmm)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Ymm, X86Mem)                  // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Xmm, X86Gp)                   //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Ymm, X86Gp)                   //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Zmm, X86Gp)                   //      AVX512_F{kz}
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Zmm, X86Xmm)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vpbroadcastq, Vpbroadcastq, X86Zmm, X86Mem)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Xmm, X86Xmm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Xmm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Ymm, X86Xmm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Xmm, X86Gp)                   //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Ymm, X86Gp)                   //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Zmm, X86Gp)                   //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Zmm, X86Xmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpbroadcastw, Vpbroadcastw, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_4i(vpclmulqdq, Vpclmulqdq, X86Xmm, X86Xmm, X86Xmm, Imm)         // AVX
  ASMJIT_INST_4i(vpclmulqdq, Vpclmulqdq, X86Xmm, X86Xmm, X86Mem, Imm)         // AVX
  ASMJIT_INST_4i(vpcmpb, Vpcmpb, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpb, Vpcmpb, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpb, Vpcmpb, X86KReg, X86Ymm, X86Ymm, Imm)                //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpb, Vpcmpb, X86KReg, X86Ymm, X86Mem, Imm)                //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpb, Vpcmpb, X86KReg, X86Zmm, X86Zmm, Imm)                //      AVX512_BW{k}
  ASMJIT_INST_4i(vpcmpb, Vpcmpb, X86KReg, X86Zmm, X86Mem, Imm)                //      AVX512_BW{k}
  ASMJIT_INST_4i(vpcmpd, Vpcmpd, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpd, Vpcmpd, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpd, Vpcmpd, X86KReg, X86Ymm, X86Ymm, Imm)                //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpd, Vpcmpd, X86KReg, X86Ymm, X86Mem, Imm)                //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpd, Vpcmpd, X86KReg, X86Zmm, X86Zmm, Imm)                //      AVX512_F{k|b32}
  ASMJIT_INST_4i(vpcmpd, Vpcmpd, X86KReg, X86Zmm, X86Mem, Imm)                //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86KReg, X86Xmm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86KReg, X86Ymm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vpcmpeqb, Vpcmpeqb, X86KReg, X86Zmm, X86Mem)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86KReg, X86Xmm, X86Mem)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86KReg, X86Ymm, X86Mem)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vpcmpeqd, Vpcmpeqd, X86KReg, X86Zmm, X86Mem)                 //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86KReg, X86Xmm, X86Mem)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86KReg, X86Ymm, X86Mem)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vpcmpeqq, Vpcmpeqq, X86KReg, X86Zmm, X86Mem)                 //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86KReg, X86Xmm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86KReg, X86Ymm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vpcmpeqw, Vpcmpeqw, X86KReg, X86Zmm, X86Mem)                 //      AVX512_BW{k}
  ASMJIT_INST_6x(vpcmpestri, Vpcmpestri, X86Xmm, X86Xmm, Imm, ECX, EAX, EDX)  // AVX  [EXPLICIT]
  ASMJIT_INST_6x(vpcmpestri, Vpcmpestri, X86Xmm, X86Mem, Imm, ECX, EAX, EDX)  // AVX  [EXPLICIT]
  ASMJIT_INST_6x(vpcmpestrm, Vpcmpestrm, X86Xmm, X86Xmm, Imm, XMM0, EAX, EDX) // AVX  [EXPLICIT]
  ASMJIT_INST_6x(vpcmpestrm, Vpcmpestrm, X86Xmm, X86Mem, Imm, XMM0, EAX, EDX) // AVX  [EXPLICIT]
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86KReg, X86Xmm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86KReg, X86Ymm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vpcmpgtb, Vpcmpgtb, X86KReg, X86Zmm, X86Mem)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86KReg, X86Xmm, X86Mem)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86KReg, X86Ymm, X86Mem)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vpcmpgtd, Vpcmpgtd, X86KReg, X86Zmm, X86Mem)                 //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86KReg, X86Xmm, X86Mem)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86KReg, X86Ymm, X86Mem)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vpcmpgtq, Vpcmpgtq, X86KReg, X86Zmm, X86Mem)                 //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86KReg, X86Xmm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86KReg, X86Ymm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vpcmpgtw, Vpcmpgtw, X86KReg, X86Zmm, X86Mem)                 //      AVX512_BW{k}
  ASMJIT_INST_4x(vpcmpistri, Vpcmpistri, X86Xmm, X86Xmm, Imm, ECX)            // AVX  [EXPLICIT]
  ASMJIT_INST_4x(vpcmpistri, Vpcmpistri, X86Xmm, X86Mem, Imm, ECX)            // AVX  [EXPLICIT]
  ASMJIT_INST_4x(vpcmpistrm, Vpcmpistrm, X86Xmm, X86Xmm, Imm, XMM0)           // AVX  [EXPLICIT]
  ASMJIT_INST_4x(vpcmpistrm, Vpcmpistrm, X86Xmm, X86Mem, Imm, XMM0)           // AVX  [EXPLICIT]
  ASMJIT_INST_4i(vpcmpq, Vpcmpq, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpq, Vpcmpq, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpq, Vpcmpq, X86KReg, X86Ymm, X86Ymm, Imm)                //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpq, Vpcmpq, X86KReg, X86Ymm, X86Mem, Imm)                //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpq, Vpcmpq, X86KReg, X86Zmm, X86Zmm, Imm)                //      AVX512_F{k|b64}
  ASMJIT_INST_4i(vpcmpq, Vpcmpq, X86KReg, X86Zmm, X86Mem, Imm)                //      AVX512_F{k|b64}
  ASMJIT_INST_4i(vpcmpub, Vpcmpub, X86KReg, X86Xmm, X86Xmm, Imm)              //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpub, Vpcmpub, X86KReg, X86Xmm, X86Mem, Imm)              //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpub, Vpcmpub, X86KReg, X86Ymm, X86Ymm, Imm)              //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpub, Vpcmpub, X86KReg, X86Ymm, X86Mem, Imm)              //      AVX512_BW{k}-VL
  ASMJIT_INST_4i(vpcmpub, Vpcmpub, X86KReg, X86Zmm, X86Zmm, Imm)              //      AVX512_BW{k}
  ASMJIT_INST_4i(vpcmpub, Vpcmpub, X86KReg, X86Zmm, X86Mem, Imm)              //      AVX512_BW{k}
  ASMJIT_INST_4i(vpcmpud, Vpcmpud, X86KReg, X86Xmm, X86Xmm, Imm)              //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpud, Vpcmpud, X86KReg, X86Xmm, X86Mem, Imm)              //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpud, Vpcmpud, X86KReg, X86Ymm, X86Ymm, Imm)              //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpud, Vpcmpud, X86KReg, X86Ymm, X86Mem, Imm)              //      AVX512_F{k|b32}-VL
  ASMJIT_INST_4i(vpcmpud, Vpcmpud, X86KReg, X86Zmm, X86Zmm, Imm)              //      AVX512_F{k|b32}
  ASMJIT_INST_4i(vpcmpud, Vpcmpud, X86KReg, X86Zmm, X86Mem, Imm)              //      AVX512_F{k|b32}
  ASMJIT_INST_4i(vpcmpuq, Vpcmpuq, X86KReg, X86Xmm, X86Xmm, Imm)              //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuq, Vpcmpuq, X86KReg, X86Xmm, X86Mem, Imm)              //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuq, Vpcmpuq, X86KReg, X86Ymm, X86Ymm, Imm)              //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuq, Vpcmpuq, X86KReg, X86Ymm, X86Mem, Imm)              //      AVX512_F{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuq, Vpcmpuq, X86KReg, X86Zmm, X86Zmm, Imm)              //      AVX512_F{k|b64}
  ASMJIT_INST_4i(vpcmpuq, Vpcmpuq, X86KReg, X86Zmm, X86Mem, Imm)              //      AVX512_F{k|b64}
  ASMJIT_INST_4i(vpcmpuw, Vpcmpuw, X86KReg, X86Xmm, X86Xmm, Imm)              //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuw, Vpcmpuw, X86KReg, X86Xmm, X86Mem, Imm)              //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuw, Vpcmpuw, X86KReg, X86Ymm, X86Ymm, Imm)              //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuw, Vpcmpuw, X86KReg, X86Ymm, X86Mem, Imm)              //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpuw, Vpcmpuw, X86KReg, X86Zmm, X86Zmm, Imm)              //      AVX512_BW{k|b64}
  ASMJIT_INST_4i(vpcmpuw, Vpcmpuw, X86KReg, X86Zmm, X86Mem, Imm)              //      AVX512_BW{k|b64}
  ASMJIT_INST_4i(vpcmpw, Vpcmpw, X86KReg, X86Xmm, X86Xmm, Imm)                //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpw, Vpcmpw, X86KReg, X86Xmm, X86Mem, Imm)                //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpw, Vpcmpw, X86KReg, X86Ymm, X86Ymm, Imm)                //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpw, Vpcmpw, X86KReg, X86Ymm, X86Mem, Imm)                //      AVX512_BW{k|b64}-VL
  ASMJIT_INST_4i(vpcmpw, Vpcmpw, X86KReg, X86Zmm, X86Zmm, Imm)                //      AVX512_BW{k|b64}
  ASMJIT_INST_4i(vpcmpw, Vpcmpw, X86KReg, X86Zmm, X86Mem, Imm)                //      AVX512_BW{k|b64}
  ASMJIT_INST_2x(vpcompressd, Vpcompressd, X86Xmm, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressd, Vpcompressd, X86Mem, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressd, Vpcompressd, X86Ymm, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressd, Vpcompressd, X86Mem, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressd, Vpcompressd, X86Zmm, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vpcompressd, Vpcompressd, X86Mem, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vpcompressq, Vpcompressq, X86Xmm, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressq, Vpcompressq, X86Mem, X86Xmm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressq, Vpcompressq, X86Ymm, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressq, Vpcompressq, X86Mem, X86Ymm)                    //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpcompressq, Vpcompressq, X86Zmm, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vpcompressq, Vpcompressq, X86Mem, X86Zmm)                    //      AVX512_F{kz}
  ASMJIT_INST_2x(vpconflictd, Vpconflictd, X86Xmm, X86Xmm)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictd, Vpconflictd, X86Xmm, X86Mem)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictd, Vpconflictd, X86Ymm, X86Ymm)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictd, Vpconflictd, X86Ymm, X86Mem)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictd, Vpconflictd, X86Zmm, X86Zmm)                    //      AVX512_CD{kz|b32}
  ASMJIT_INST_2x(vpconflictd, Vpconflictd, X86Zmm, X86Mem)                    //      AVX512_CD{kz|b32}
  ASMJIT_INST_2x(vpconflictq, Vpconflictq, X86Xmm, X86Xmm)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictq, Vpconflictq, X86Xmm, X86Mem)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictq, Vpconflictq, X86Ymm, X86Ymm)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictq, Vpconflictq, X86Ymm, X86Mem)                    //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vpconflictq, Vpconflictq, X86Zmm, X86Zmm)                    //      AVX512_CD{kz|b32}
  ASMJIT_INST_2x(vpconflictq, Vpconflictq, X86Zmm, X86Mem)                    //      AVX512_CD{kz|b32}
  ASMJIT_INST_4i(vperm2f128, Vperm2f128, X86Ymm, X86Ymm, X86Ymm, Imm)         // AVX
  ASMJIT_INST_4i(vperm2f128, Vperm2f128, X86Ymm, X86Ymm, X86Mem, Imm)         // AVX
  ASMJIT_INST_4i(vperm2i128, Vperm2i128, X86Ymm, X86Ymm, X86Ymm, Imm)         // AVX2
  ASMJIT_INST_4i(vperm2i128, Vperm2i128, X86Ymm, X86Ymm, X86Mem, Imm)         // AVX2
  ASMJIT_INST_3x(vpermb, Vpermb, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermb, Vpermb, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermb, Vpermb, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermb, Vpermb, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermb, Vpermb, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_VBMI{kz}
  ASMJIT_INST_3x(vpermb, Vpermb, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_VBMI{kz}
  ASMJIT_INST_3x(vpermd, Vpermd, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermd, Vpermd, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermd, Vpermd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermd, Vpermd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2b, Vpermi2b, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermi2b, Vpermi2b, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermi2b, Vpermi2b, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermi2b, Vpermi2b, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermi2b, Vpermi2b, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_VBMI{kz}
  ASMJIT_INST_3x(vpermi2b, Vpermi2b, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_VBMI{kz}
  ASMJIT_INST_3x(vpermi2d, Vpermi2d, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2d, Vpermi2d, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2d, Vpermi2d, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2d, Vpermi2d, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2d, Vpermi2d, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermi2d, Vpermi2d, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermi2pd, Vpermi2pd, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2pd, Vpermi2pd, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2pd, Vpermi2pd, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2pd, Vpermi2pd, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2pd, Vpermi2pd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermi2pd, Vpermi2pd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermi2ps, Vpermi2ps, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2ps, Vpermi2ps, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2ps, Vpermi2ps, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2ps, Vpermi2ps, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermi2ps, Vpermi2ps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermi2ps, Vpermi2ps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermi2q, Vpermi2q, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2q, Vpermi2q, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2q, Vpermi2q, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2q, Vpermi2q, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermi2q, Vpermi2q, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermi2q, Vpermi2q, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermi2w, Vpermi2w, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermi2w, Vpermi2w, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermi2w, Vpermi2w, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermi2w, Vpermi2w, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermi2w, Vpermi2w, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpermi2w, Vpermi2w, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpermilpd, Vpermilpd, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilpd, Vpermilpd, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilpd, Vpermilpd, X86Xmm, X86Xmm, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilpd, Vpermilpd, X86Xmm, X86Mem, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilpd, Vpermilpd, X86Ymm, X86Ymm, X86Ymm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilpd, Vpermilpd, X86Ymm, X86Ymm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilpd, Vpermilpd, X86Ymm, X86Ymm, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilpd, Vpermilpd, X86Ymm, X86Mem, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilpd, Vpermilpd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermilpd, Vpermilpd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpermilpd, Vpermilpd, X86Zmm, X86Zmm, Imm)                   //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpermilpd, Vpermilpd, X86Zmm, X86Mem, Imm)                   //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermilps, Vpermilps, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilps, Vpermilps, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilps, Vpermilps, X86Xmm, X86Xmm, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilps, Vpermilps, X86Xmm, X86Mem, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilps, Vpermilps, X86Ymm, X86Ymm, X86Ymm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilps, Vpermilps, X86Ymm, X86Ymm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilps, Vpermilps, X86Ymm, X86Ymm, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermilps, Vpermilps, X86Ymm, X86Mem, Imm)                   // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermilps, Vpermilps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermilps, Vpermilps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpermilps, Vpermilps, X86Zmm, X86Zmm, Imm)                   //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpermilps, Vpermilps, X86Zmm, X86Mem, Imm)                   //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpermpd, Vpermpd, X86Ymm, X86Ymm, Imm)                       // AVX2
  ASMJIT_INST_3i(vpermpd, Vpermpd, X86Ymm, X86Mem, Imm)                       // AVX2
  ASMJIT_INST_3x(vpermps, Vpermps, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vpermps, Vpermps, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3i(vpermq, Vpermq, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermq, Vpermq, X86Ymm, X86Mem, Imm)                         // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermq, Vpermq, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermq, Vpermq, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermq, Vpermq, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermq, Vpermq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermq, Vpermq, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpermq, Vpermq, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2b, Vpermt2b, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermt2b, Vpermt2b, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermt2b, Vpermt2b, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermt2b, Vpermt2b, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_VBMI{kz}-VL
  ASMJIT_INST_3x(vpermt2b, Vpermt2b, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_VBMI{kz}
  ASMJIT_INST_3x(vpermt2b, Vpermt2b, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_VBMI{kz}
  ASMJIT_INST_3x(vpermt2d, Vpermt2d, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2d, Vpermt2d, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2d, Vpermt2d, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2d, Vpermt2d, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2d, Vpermt2d, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermt2d, Vpermt2d, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermt2pd, Vpermt2pd, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2pd, Vpermt2pd, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2pd, Vpermt2pd, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2pd, Vpermt2pd, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2pd, Vpermt2pd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermt2pd, Vpermt2pd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermt2ps, Vpermt2ps, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2ps, Vpermt2ps, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2ps, Vpermt2ps, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2ps, Vpermt2ps, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpermt2ps, Vpermt2ps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermt2ps, Vpermt2ps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpermt2q, Vpermt2q, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2q, Vpermt2q, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2q, Vpermt2q, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2q, Vpermt2q, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpermt2q, Vpermt2q, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermt2q, Vpermt2q, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpermt2w, Vpermt2w, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermt2w, Vpermt2w, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermt2w, Vpermt2w, X86Ymm, X86Ymm, X86Ymm)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermt2w, Vpermt2w, X86Ymm, X86Ymm, X86Mem)                  //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermt2w, Vpermt2w, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpermt2w, Vpermt2w, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpermw, Vpermw, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermw, Vpermw, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermw, Vpermw, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermw, Vpermw, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpermw, Vpermw, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpermw, Vpermw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpexpandd, Vpexpandd, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandd, Vpexpandd, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandd, Vpexpandd, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandd, Vpexpandd, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandd, Vpexpandd, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpexpandd, Vpexpandd, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpexpandq, Vpexpandq, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandq, Vpexpandq, X86Xmm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandq, Vpexpandq, X86Ymm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandq, Vpexpandq, X86Ymm, X86Mem)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpexpandq, Vpexpandq, X86Zmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpexpandq, Vpexpandq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_3i(vpextrb, Vpextrb, X86Gp, X86Xmm, Imm)                        // AVX  AVX512_BW
  ASMJIT_INST_3i(vpextrb, Vpextrb, X86Mem, X86Xmm, Imm)                       // AVX  AVX512_BW
  ASMJIT_INST_3i(vpextrd, Vpextrd, X86Gp, X86Xmm, Imm)                        // AVX  AVX512_DQ
  ASMJIT_INST_3i(vpextrd, Vpextrd, X86Mem, X86Xmm, Imm)                       // AVX  AVX512_DQ
  ASMJIT_INST_3i(vpextrq, Vpextrq, X86Gp, X86Xmm, Imm)                        // AVX  AVX512_DQ
  ASMJIT_INST_3i(vpextrq, Vpextrq, X86Mem, X86Xmm, Imm)                       // AVX  AVX512_DQ
  ASMJIT_INST_3i(vpextrw, Vpextrw, X86Gp, X86Xmm, Imm)                        // AVX  AVX512_BW
  ASMJIT_INST_3i(vpextrw, Vpextrw, X86Mem, X86Xmm, Imm)                       // AVX  AVX512_BW
  ASMJIT_INST_3x(vpgatherdd, Vpgatherdd, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vpgatherdd, Vpgatherdd, X86Ymm, X86Mem, X86Ymm)              // AVX2
  ASMJIT_INST_2x(vpgatherdd, Vpgatherdd, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherdd, Vpgatherdd, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherdd, Vpgatherdd, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_3x(vpgatherdq, Vpgatherdq, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vpgatherdq, Vpgatherdq, X86Ymm, X86Mem, X86Ymm)              // AVX2
  ASMJIT_INST_2x(vpgatherdq, Vpgatherdq, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherdq, Vpgatherdq, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherdq, Vpgatherdq, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_3x(vpgatherqd, Vpgatherqd, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_2x(vpgatherqd, Vpgatherqd, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherqd, Vpgatherqd, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherqd, Vpgatherqd, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_3x(vpgatherqq, Vpgatherqq, X86Xmm, X86Mem, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vpgatherqq, Vpgatherqq, X86Ymm, X86Mem, X86Ymm)              // AVX2
  ASMJIT_INST_2x(vpgatherqq, Vpgatherqq, X86Xmm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherqq, Vpgatherqq, X86Ymm, X86Mem)                      //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpgatherqq, Vpgatherqq, X86Zmm, X86Mem)                      //      AVX512_F{k}
  ASMJIT_INST_3x(vphaddd, Vphaddd, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vphaddd, Vphaddd, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vphaddd, Vphaddd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vphaddd, Vphaddd, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3x(vphaddsw, Vphaddsw, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vphaddsw, Vphaddsw, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vphaddsw, Vphaddsw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vphaddsw, Vphaddsw, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vphaddw, Vphaddw, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vphaddw, Vphaddw, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vphaddw, Vphaddw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vphaddw, Vphaddw, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_2x(vphminposuw, Vphminposuw, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_2x(vphminposuw, Vphminposuw, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vphsubd, Vphsubd, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vphsubd, Vphsubd, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vphsubd, Vphsubd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vphsubd, Vphsubd, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3x(vphsubsw, Vphsubsw, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vphsubsw, Vphsubsw, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vphsubsw, Vphsubsw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2
  ASMJIT_INST_3x(vphsubsw, Vphsubsw, X86Ymm, X86Ymm, X86Mem)                  // AVX2
  ASMJIT_INST_3x(vphsubw, Vphsubw, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vphsubw, Vphsubw, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vphsubw, Vphsubw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vphsubw, Vphsubw, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3i(vpinsrb, Vpinsrb, X86Xmm, X86Gp, Imm)                        // AVX
  ASMJIT_INST_3i(vpinsrb, Vpinsrb, X86Xmm, X86Mem, Imm)                       // AVX
  ASMJIT_INST_4i(vpinsrb, Vpinsrb, X86Xmm, X86Xmm, X86Gp, Imm)                //      AVX512_BW{kz}
  ASMJIT_INST_4i(vpinsrb, Vpinsrb, X86Xmm, X86Xmm, X86Mem, Imm)               //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpinsrd, Vpinsrd, X86Xmm, X86Gp, Imm)                        // AVX
  ASMJIT_INST_3i(vpinsrd, Vpinsrd, X86Xmm, X86Mem, Imm)                       // AVX
  ASMJIT_INST_4i(vpinsrd, Vpinsrd, X86Xmm, X86Xmm, X86Gp, Imm)                //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vpinsrd, Vpinsrd, X86Xmm, X86Xmm, X86Mem, Imm)               //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vpinsrq, Vpinsrq, X86Xmm, X86Gp, Imm)                        // AVX
  ASMJIT_INST_3i(vpinsrq, Vpinsrq, X86Xmm, X86Mem, Imm)                       // AVX
  ASMJIT_INST_4i(vpinsrq, Vpinsrq, X86Xmm, X86Xmm, X86Gp, Imm)                //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vpinsrq, Vpinsrq, X86Xmm, X86Xmm, X86Mem, Imm)               //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vpinsrw, Vpinsrw, X86Xmm, X86Xmm, X86Gp, Imm)                // AVX  AVX512_BW{kz}
  ASMJIT_INST_4i(vpinsrw, Vpinsrw, X86Xmm, X86Xmm, X86Mem, Imm)               // AVX  AVX512_BW{kz}
  ASMJIT_INST_2x(vplzcntd, Vplzcntd, X86Xmm, X86Xmm)                          //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vplzcntd, Vplzcntd, X86Xmm, X86Mem)                          //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vplzcntd, Vplzcntd, X86Ymm, X86Ymm)                          //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vplzcntd, Vplzcntd, X86Ymm, X86Mem)                          //      AVX512_CD{kz|b32}-VL
  ASMJIT_INST_2x(vplzcntd, Vplzcntd, X86Zmm, X86Zmm)                          //      AVX512_CD{kz|b32}
  ASMJIT_INST_2x(vplzcntd, Vplzcntd, X86Zmm, X86Mem)                          //      AVX512_CD{kz|b32}
  ASMJIT_INST_2x(vplzcntq, Vplzcntq, X86Xmm, X86Xmm)                          //      AVX512_CD{kz|b64}-VL
  ASMJIT_INST_2x(vplzcntq, Vplzcntq, X86Xmm, X86Mem)                          //      AVX512_CD{kz|b64}-VL
  ASMJIT_INST_2x(vplzcntq, Vplzcntq, X86Ymm, X86Ymm)                          //      AVX512_CD{kz|b64}-VL
  ASMJIT_INST_2x(vplzcntq, Vplzcntq, X86Ymm, X86Mem)                          //      AVX512_CD{kz|b64}-VL
  ASMJIT_INST_2x(vplzcntq, Vplzcntq, X86Zmm, X86Zmm)                          //      AVX512_CD{kz|b64}
  ASMJIT_INST_2x(vplzcntq, Vplzcntq, X86Zmm, X86Mem)                          //      AVX512_CD{kz|b64}
  ASMJIT_INST_3x(vpmadd52huq, Vpmadd52huq, X86Xmm, X86Xmm, X86Xmm)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52huq, Vpmadd52huq, X86Xmm, X86Xmm, X86Mem)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52huq, Vpmadd52huq, X86Ymm, X86Ymm, X86Ymm)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52huq, Vpmadd52huq, X86Ymm, X86Ymm, X86Mem)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52huq, Vpmadd52huq, X86Zmm, X86Zmm, X86Zmm)            //      AVX512_IFMA{kz|b64}
  ASMJIT_INST_3x(vpmadd52huq, Vpmadd52huq, X86Zmm, X86Zmm, X86Mem)            //      AVX512_IFMA{kz|b64}
  ASMJIT_INST_3x(vpmadd52luq, Vpmadd52luq, X86Xmm, X86Xmm, X86Xmm)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52luq, Vpmadd52luq, X86Xmm, X86Xmm, X86Mem)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52luq, Vpmadd52luq, X86Ymm, X86Ymm, X86Ymm)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52luq, Vpmadd52luq, X86Ymm, X86Ymm, X86Mem)            //      AVX512_IFMA{kz|b64}-VL
  ASMJIT_INST_3x(vpmadd52luq, Vpmadd52luq, X86Zmm, X86Zmm, X86Zmm)            //      AVX512_IFMA{kz|b64}
  ASMJIT_INST_3x(vpmadd52luq, Vpmadd52luq, X86Zmm, X86Zmm, X86Mem)            //      AVX512_IFMA{kz|b64}
  ASMJIT_INST_3x(vpmaddubsw, Vpmaddubsw, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddubsw, Vpmaddubsw, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddubsw, Vpmaddubsw, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddubsw, Vpmaddubsw, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddubsw, Vpmaddubsw, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaddubsw, Vpmaddubsw, X86Zmm, X86Zmm, X86Mem)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaddwd, Vpmaddwd, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddwd, Vpmaddwd, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddwd, Vpmaddwd, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddwd, Vpmaddwd, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaddwd, Vpmaddwd, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaddwd, Vpmaddwd, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaskmovd, Vpmaskmovd, X86Mem, X86Xmm, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vpmaskmovd, Vpmaskmovd, X86Mem, X86Ymm, X86Ymm)              // AVX2
  ASMJIT_INST_3x(vpmaskmovd, Vpmaskmovd, X86Xmm, X86Xmm, X86Mem)              // AVX2
  ASMJIT_INST_3x(vpmaskmovd, Vpmaskmovd, X86Ymm, X86Ymm, X86Mem)              // AVX2
  ASMJIT_INST_3x(vpmaskmovq, Vpmaskmovq, X86Mem, X86Xmm, X86Xmm)              // AVX2
  ASMJIT_INST_3x(vpmaskmovq, Vpmaskmovq, X86Mem, X86Ymm, X86Ymm)              // AVX2
  ASMJIT_INST_3x(vpmaskmovq, Vpmaskmovq, X86Xmm, X86Xmm, X86Mem)              // AVX2
  ASMJIT_INST_3x(vpmaskmovq, Vpmaskmovq, X86Ymm, X86Ymm, X86Mem)              // AVX2
  ASMJIT_INST_3x(vpmaxsb, Vpmaxsb, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsb, Vpmaxsb, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsb, Vpmaxsb, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsb, Vpmaxsb, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsb, Vpmaxsb, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxsb, Vpmaxsb, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxsd, Vpmaxsd, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxsd, Vpmaxsd, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxsd, Vpmaxsd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxsd, Vpmaxsd, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxsd, Vpmaxsd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpmaxsd, Vpmaxsd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpmaxsq, Vpmaxsq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxsq, Vpmaxsq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxsq, Vpmaxsq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxsq, Vpmaxsq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxsq, Vpmaxsq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmaxsq, Vpmaxsq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmaxsw, Vpmaxsw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsw, Vpmaxsw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsw, Vpmaxsw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsw, Vpmaxsw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxsw, Vpmaxsw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxsw, Vpmaxsw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxub, Vpmaxub, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxub, Vpmaxub, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxub, Vpmaxub, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxub, Vpmaxub, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxub, Vpmaxub, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxub, Vpmaxub, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxud, Vpmaxud, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxud, Vpmaxud, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxud, Vpmaxud, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxud, Vpmaxud, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmaxud, Vpmaxud, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpmaxud, Vpmaxud, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpmaxuq, Vpmaxuq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxuq, Vpmaxuq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxuq, Vpmaxuq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxuq, Vpmaxuq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmaxuq, Vpmaxuq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmaxuq, Vpmaxuq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmaxuw, Vpmaxuw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxuw, Vpmaxuw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxuw, Vpmaxuw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxuw, Vpmaxuw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmaxuw, Vpmaxuw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmaxuw, Vpmaxuw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminsb, Vpminsb, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsb, Vpminsb, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsb, Vpminsb, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsb, Vpminsb, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsb, Vpminsb, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminsb, Vpminsb, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminsd, Vpminsd, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminsd, Vpminsd, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminsd, Vpminsd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminsd, Vpminsd, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminsd, Vpminsd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpminsd, Vpminsd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpminsq, Vpminsq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminsq, Vpminsq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminsq, Vpminsq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminsq, Vpminsq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminsq, Vpminsq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpminsq, Vpminsq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpminsw, Vpminsw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsw, Vpminsw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsw, Vpminsw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsw, Vpminsw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminsw, Vpminsw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminsw, Vpminsw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminub, Vpminub, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminub, Vpminub, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminub, Vpminub, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminub, Vpminub, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminub, Vpminub, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminub, Vpminub, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminud, Vpminud, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminud, Vpminud, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminud, Vpminud, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminud, Vpminud, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpminud, Vpminud, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpminud, Vpminud, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpminuq, Vpminuq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminuq, Vpminuq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminuq, Vpminuq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminuq, Vpminuq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpminuq, Vpminuq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpminuq, Vpminuq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpminuw, Vpminuw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminuw, Vpminuw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminuw, Vpminuw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminuw, Vpminuw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpminuw, Vpminuw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpminuw, Vpminuw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovb2m, Vpmovb2m, X86KReg, X86Xmm)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovb2m, Vpmovb2m, X86KReg, X86Ymm)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovb2m, Vpmovb2m, X86KReg, X86Zmm)                         //      AVX512_BW
  ASMJIT_INST_2x(vpmovd2m, Vpmovd2m, X86KReg, X86Xmm)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovd2m, Vpmovd2m, X86KReg, X86Ymm)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovd2m, Vpmovd2m, X86KReg, X86Zmm)                         //      AVX512_DQ
  ASMJIT_INST_2x(vpmovdb, Vpmovdb, X86Xmm, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdb, Vpmovdb, X86Mem, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdb, Vpmovdb, X86Xmm, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdb, Vpmovdb, X86Mem, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdb, Vpmovdb, X86Xmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovdb, Vpmovdb, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovdw, Vpmovdw, X86Xmm, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdw, Vpmovdw, X86Mem, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdw, Vpmovdw, X86Xmm, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdw, Vpmovdw, X86Mem, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovdw, Vpmovdw, X86Ymm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovdw, Vpmovdw, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovm2b, Vpmovm2b, X86Xmm, X86KReg)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovm2b, Vpmovm2b, X86Ymm, X86KReg)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovm2b, Vpmovm2b, X86Zmm, X86KReg)                         //      AVX512_BW
  ASMJIT_INST_2x(vpmovm2d, Vpmovm2d, X86Xmm, X86KReg)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovm2d, Vpmovm2d, X86Ymm, X86KReg)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovm2d, Vpmovm2d, X86Zmm, X86KReg)                         //      AVX512_DQ
  ASMJIT_INST_2x(vpmovm2q, Vpmovm2q, X86Xmm, X86KReg)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovm2q, Vpmovm2q, X86Ymm, X86KReg)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovm2q, Vpmovm2q, X86Zmm, X86KReg)                         //      AVX512_DQ
  ASMJIT_INST_2x(vpmovm2w, Vpmovm2w, X86Xmm, X86KReg)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovm2w, Vpmovm2w, X86Ymm, X86KReg)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovm2w, Vpmovm2w, X86Zmm, X86KReg)                         //      AVX512_BW
  ASMJIT_INST_2x(vpmovmskb, Vpmovmskb, X86Gp, X86Xmm)                         // AVX
  ASMJIT_INST_2x(vpmovmskb, Vpmovmskb, X86Gp, X86Ymm)                         // AVX2
  ASMJIT_INST_2x(vpmovq2m, Vpmovq2m, X86KReg, X86Xmm)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovq2m, Vpmovq2m, X86KReg, X86Ymm)                         //      AVX512_DQ-VL
  ASMJIT_INST_2x(vpmovq2m, Vpmovq2m, X86KReg, X86Zmm)                         //      AVX512_DQ
  ASMJIT_INST_2x(vpmovqb, Vpmovqb, X86Xmm, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqb, Vpmovqb, X86Mem, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqb, Vpmovqb, X86Xmm, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqb, Vpmovqb, X86Mem, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqb, Vpmovqb, X86Xmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovqb, Vpmovqb, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovqd, Vpmovqd, X86Xmm, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqd, Vpmovqd, X86Mem, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqd, Vpmovqd, X86Xmm, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqd, Vpmovqd, X86Mem, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqd, Vpmovqd, X86Ymm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovqd, Vpmovqd, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovqw, Vpmovqw, X86Xmm, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqw, Vpmovqw, X86Mem, X86Xmm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqw, Vpmovqw, X86Xmm, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqw, Vpmovqw, X86Mem, X86Ymm)                            //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovqw, Vpmovqw, X86Xmm, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovqw, Vpmovqw, X86Mem, X86Zmm)                            //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsdb, Vpmovsdb, X86Xmm, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdb, Vpmovsdb, X86Mem, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdb, Vpmovsdb, X86Xmm, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdb, Vpmovsdb, X86Mem, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdb, Vpmovsdb, X86Xmm, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsdb, Vpmovsdb, X86Mem, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsdw, Vpmovsdw, X86Xmm, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdw, Vpmovsdw, X86Mem, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdw, Vpmovsdw, X86Xmm, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdw, Vpmovsdw, X86Mem, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsdw, Vpmovsdw, X86Ymm, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsdw, Vpmovsdw, X86Mem, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsqb, Vpmovsqb, X86Xmm, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqb, Vpmovsqb, X86Mem, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqb, Vpmovsqb, X86Xmm, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqb, Vpmovsqb, X86Mem, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqb, Vpmovsqb, X86Xmm, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsqb, Vpmovsqb, X86Mem, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsqd, Vpmovsqd, X86Xmm, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqd, Vpmovsqd, X86Mem, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqd, Vpmovsqd, X86Xmm, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqd, Vpmovsqd, X86Mem, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqd, Vpmovsqd, X86Ymm, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsqd, Vpmovsqd, X86Mem, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsqw, Vpmovsqw, X86Xmm, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqw, Vpmovsqw, X86Mem, X86Xmm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqw, Vpmovsqw, X86Xmm, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqw, Vpmovsqw, X86Mem, X86Ymm)                          //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsqw, Vpmovsqw, X86Xmm, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsqw, Vpmovsqw, X86Mem, X86Zmm)                          //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovswb, Vpmovswb, X86Xmm, X86Xmm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovswb, Vpmovswb, X86Mem, X86Xmm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovswb, Vpmovswb, X86Xmm, X86Ymm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovswb, Vpmovswb, X86Mem, X86Ymm)                          //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovswb, Vpmovswb, X86Ymm, X86Zmm)                          //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovswb, Vpmovswb, X86Mem, X86Zmm)                          //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovsxbd, Vpmovsxbd, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbd, Vpmovsxbd, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbd, Vpmovsxbd, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbd, Vpmovsxbd, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbd, Vpmovsxbd, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxbd, Vpmovsxbd, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxbq, Vpmovsxbq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbq, Vpmovsxbq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbq, Vpmovsxbq, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbq, Vpmovsxbq, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxbq, Vpmovsxbq, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxbq, Vpmovsxbq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxbw, Vpmovsxbw, X86Xmm, X86Xmm)                        // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovsxbw, Vpmovsxbw, X86Xmm, X86Mem)                        // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovsxbw, Vpmovsxbw, X86Ymm, X86Xmm)                        // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovsxbw, Vpmovsxbw, X86Ymm, X86Mem)                        // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovsxbw, Vpmovsxbw, X86Zmm, X86Ymm)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovsxbw, Vpmovsxbw, X86Zmm, X86Mem)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovsxdq, Vpmovsxdq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxdq, Vpmovsxdq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxdq, Vpmovsxdq, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxdq, Vpmovsxdq, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxdq, Vpmovsxdq, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxdq, Vpmovsxdq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxwd, Vpmovsxwd, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwd, Vpmovsxwd, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwd, Vpmovsxwd, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwd, Vpmovsxwd, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwd, Vpmovsxwd, X86Zmm, X86Ymm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxwd, Vpmovsxwd, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxwq, Vpmovsxwq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwq, Vpmovsxwq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwq, Vpmovsxwq, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwq, Vpmovsxwq, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovsxwq, Vpmovsxwq, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovsxwq, Vpmovsxwq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusdb, Vpmovusdb, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdb, Vpmovusdb, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdb, Vpmovusdb, X86Xmm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdb, Vpmovusdb, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdb, Vpmovusdb, X86Xmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusdb, Vpmovusdb, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusdw, Vpmovusdw, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdw, Vpmovusdw, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdw, Vpmovusdw, X86Xmm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdw, Vpmovusdw, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusdw, Vpmovusdw, X86Ymm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusdw, Vpmovusdw, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusqb, Vpmovusqb, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqb, Vpmovusqb, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqb, Vpmovusqb, X86Xmm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqb, Vpmovusqb, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqb, Vpmovusqb, X86Xmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusqb, Vpmovusqb, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusqd, Vpmovusqd, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqd, Vpmovusqd, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqd, Vpmovusqd, X86Xmm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqd, Vpmovusqd, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqd, Vpmovusqd, X86Ymm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusqd, Vpmovusqd, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusqw, Vpmovusqw, X86Xmm, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqw, Vpmovusqw, X86Mem, X86Xmm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqw, Vpmovusqw, X86Xmm, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqw, Vpmovusqw, X86Mem, X86Ymm)                        //      AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovusqw, Vpmovusqw, X86Xmm, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovusqw, Vpmovusqw, X86Mem, X86Zmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovuswb, Vpmovuswb, X86Xmm, X86Xmm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovuswb, Vpmovuswb, X86Mem, X86Xmm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovuswb, Vpmovuswb, X86Xmm, X86Ymm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovuswb, Vpmovuswb, X86Mem, X86Ymm)                        //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovuswb, Vpmovuswb, X86Ymm, X86Zmm)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovuswb, Vpmovuswb, X86Mem, X86Zmm)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovw2m, Vpmovw2m, X86KReg, X86Xmm)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovw2m, Vpmovw2m, X86KReg, X86Ymm)                         //      AVX512_BW-VL
  ASMJIT_INST_2x(vpmovw2m, Vpmovw2m, X86KReg, X86Zmm)                         //      AVX512_BW
  ASMJIT_INST_2x(vpmovwb, Vpmovwb, X86Xmm, X86Xmm)                            //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovwb, Vpmovwb, X86Mem, X86Xmm)                            //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovwb, Vpmovwb, X86Xmm, X86Ymm)                            //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovwb, Vpmovwb, X86Mem, X86Ymm)                            //      AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovwb, Vpmovwb, X86Ymm, X86Zmm)                            //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovwb, Vpmovwb, X86Mem, X86Zmm)                            //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovzxbd, Vpmovzxbd, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbd, Vpmovzxbd, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbd, Vpmovzxbd, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbd, Vpmovzxbd, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbd, Vpmovzxbd, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxbd, Vpmovzxbd, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxbq, Vpmovzxbq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbq, Vpmovzxbq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbq, Vpmovzxbq, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbq, Vpmovzxbq, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxbq, Vpmovzxbq, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxbq, Vpmovzxbq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxbw, Vpmovzxbw, X86Xmm, X86Xmm)                        // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovzxbw, Vpmovzxbw, X86Xmm, X86Mem)                        // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovzxbw, Vpmovzxbw, X86Ymm, X86Xmm)                        // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovzxbw, Vpmovzxbw, X86Ymm, X86Mem)                        // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_2x(vpmovzxbw, Vpmovzxbw, X86Zmm, X86Ymm)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovzxbw, Vpmovzxbw, X86Zmm, X86Mem)                        //      AVX512_BW{kz}
  ASMJIT_INST_2x(vpmovzxdq, Vpmovzxdq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxdq, Vpmovzxdq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxdq, Vpmovzxdq, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxdq, Vpmovzxdq, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxdq, Vpmovzxdq, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxdq, Vpmovzxdq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxwd, Vpmovzxwd, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwd, Vpmovzxwd, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwd, Vpmovzxwd, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwd, Vpmovzxwd, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwd, Vpmovzxwd, X86Zmm, X86Ymm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxwd, Vpmovzxwd, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxwq, Vpmovzxwq, X86Xmm, X86Xmm)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwq, Vpmovzxwq, X86Xmm, X86Mem)                        // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwq, Vpmovzxwq, X86Ymm, X86Xmm)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwq, Vpmovzxwq, X86Ymm, X86Mem)                        // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_2x(vpmovzxwq, Vpmovzxwq, X86Zmm, X86Xmm)                        //      AVX512_F{kz}
  ASMJIT_INST_2x(vpmovzxwq, Vpmovzxwq, X86Zmm, X86Mem)                        //      AVX512_F{kz}
  ASMJIT_INST_3x(vpmuldq, Vpmuldq, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuldq, Vpmuldq, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuldq, Vpmuldq, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuldq, Vpmuldq, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuldq, Vpmuldq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmuldq, Vpmuldq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmulhrsw, Vpmulhrsw, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhrsw, Vpmulhrsw, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhrsw, Vpmulhrsw, X86Ymm, X86Ymm, X86Ymm)                // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhrsw, Vpmulhrsw, X86Ymm, X86Ymm, X86Mem)                // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhrsw, Vpmulhrsw, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmulhrsw, Vpmulhrsw, X86Zmm, X86Zmm, X86Mem)                //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmulhuw, Vpmulhuw, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhuw, Vpmulhuw, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhuw, Vpmulhuw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhuw, Vpmulhuw, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhuw, Vpmulhuw, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmulhuw, Vpmulhuw, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmulhw, Vpmulhw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhw, Vpmulhw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhw, Vpmulhw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhw, Vpmulhw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmulhw, Vpmulhw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmulhw, Vpmulhw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmulld, Vpmulld, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmulld, Vpmulld, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmulld, Vpmulld, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmulld, Vpmulld, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpmulld, Vpmulld, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpmulld, Vpmulld, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpmullq, Vpmullq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vpmullq, Vpmullq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vpmullq, Vpmullq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vpmullq, Vpmullq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vpmullq, Vpmullq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vpmullq, Vpmullq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vpmullw, Vpmullw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmullw, Vpmullw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmullw, Vpmullw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmullw, Vpmullw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpmullw, Vpmullw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmullw, Vpmullw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpmultishiftqb, Vpmultishiftqb, X86Xmm, X86Xmm, X86Xmm)      //      AVX512_VBMI{kz|b64}-VL
  ASMJIT_INST_3x(vpmultishiftqb, Vpmultishiftqb, X86Xmm, X86Xmm, X86Mem)      //      AVX512_VBMI{kz|b64}-VL
  ASMJIT_INST_3x(vpmultishiftqb, Vpmultishiftqb, X86Ymm, X86Ymm, X86Ymm)      //      AVX512_VBMI{kz|b64}-VL
  ASMJIT_INST_3x(vpmultishiftqb, Vpmultishiftqb, X86Ymm, X86Ymm, X86Mem)      //      AVX512_VBMI{kz|b64}-VL
  ASMJIT_INST_3x(vpmultishiftqb, Vpmultishiftqb, X86Zmm, X86Zmm, X86Zmm)      //      AVX512_VBMI{kz|b64}
  ASMJIT_INST_3x(vpmultishiftqb, Vpmultishiftqb, X86Zmm, X86Zmm, X86Mem)      //      AVX512_VBMI{kz|b64}
  ASMJIT_INST_3x(vpmuludq, Vpmuludq, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuludq, Vpmuludq, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuludq, Vpmuludq, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuludq, Vpmuludq, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpmuludq, Vpmuludq, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpmuludq, Vpmuludq, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vpopcntd, Vpopcntd, X86Zmm, X86Zmm)                          //      AVX512_VPOPCNTDQ{kz|b32}
  ASMJIT_INST_2x(vpopcntd, Vpopcntd, X86Zmm, X86Mem)                          //      AVX512_VPOPCNTDQ{kz|b32}
  ASMJIT_INST_2x(vpopcntq, Vpopcntq, X86Zmm, X86Zmm)                          //      AVX512_VPOPCNTDQ{kz|b64}
  ASMJIT_INST_2x(vpopcntq, Vpopcntq, X86Zmm, X86Mem)                          //      AVX512_VPOPCNTDQ{kz|b64}
  ASMJIT_INST_3x(vpor, Vpor, X86Xmm, X86Xmm, X86Xmm)                          // AVX
  ASMJIT_INST_3x(vpor, Vpor, X86Xmm, X86Xmm, X86Mem)                          // AVX
  ASMJIT_INST_3x(vpor, Vpor, X86Ymm, X86Ymm, X86Ymm)                          // AVX2
  ASMJIT_INST_3x(vpor, Vpor, X86Ymm, X86Ymm, X86Mem)                          // AVX2
  ASMJIT_INST_3x(vpord, Vpord, X86Xmm, X86Xmm, X86Xmm)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpord, Vpord, X86Xmm, X86Xmm, X86Mem)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpord, Vpord, X86Ymm, X86Ymm, X86Ymm)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpord, Vpord, X86Ymm, X86Ymm, X86Mem)                        //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpord, Vpord, X86Zmm, X86Zmm, X86Zmm)                        //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpord, Vpord, X86Zmm, X86Zmm, X86Mem)                        //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vporq, Vporq, X86Xmm, X86Xmm, X86Xmm)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vporq, Vporq, X86Xmm, X86Xmm, X86Mem)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vporq, Vporq, X86Ymm, X86Ymm, X86Ymm)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vporq, Vporq, X86Ymm, X86Ymm, X86Mem)                        //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vporq, Vporq, X86Zmm, X86Zmm, X86Zmm)                        //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vporq, Vporq, X86Zmm, X86Zmm, X86Mem)                        //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vprold, Vprold, X86Xmm, X86Xmm, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprold, Vprold, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprold, Vprold, X86Ymm, X86Ymm, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprold, Vprold, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprold, Vprold, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vprold, Vprold, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vprolq, Vprolq, X86Xmm, X86Xmm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprolq, Vprolq, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprolq, Vprolq, X86Ymm, X86Ymm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprolq, Vprolq, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprolq, Vprolq, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vprolq, Vprolq, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vprolvd, Vprolvd, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprolvd, Vprolvd, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprolvd, Vprolvd, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprolvd, Vprolvd, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprolvd, Vprolvd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vprolvd, Vprolvd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vprolvq, Vprolvq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprolvq, Vprolvq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprolvq, Vprolvq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprolvq, Vprolvq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprolvq, Vprolvq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vprolvq, Vprolvq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vprord, Vprord, X86Xmm, X86Xmm, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprord, Vprord, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprord, Vprord, X86Ymm, X86Ymm, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprord, Vprord, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vprord, Vprord, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vprord, Vprord, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vprorq, Vprorq, X86Xmm, X86Xmm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprorq, Vprorq, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprorq, Vprorq, X86Ymm, X86Ymm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprorq, Vprorq, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vprorq, Vprorq, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vprorq, Vprorq, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vprorvd, Vprorvd, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprorvd, Vprorvd, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprorvd, Vprorvd, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprorvd, Vprorvd, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vprorvd, Vprorvd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vprorvd, Vprorvd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vprorvq, Vprorvq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprorvq, Vprorvq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprorvq, Vprorvq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprorvq, Vprorvq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vprorvq, Vprorvq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vprorvq, Vprorvq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsadbw, Vpsadbw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW-VL
  ASMJIT_INST_3x(vpsadbw, Vpsadbw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW-VL
  ASMJIT_INST_3x(vpsadbw, Vpsadbw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW-VL
  ASMJIT_INST_3x(vpsadbw, Vpsadbw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW-VL
  ASMJIT_INST_3x(vpsadbw, Vpsadbw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW
  ASMJIT_INST_3x(vpsadbw, Vpsadbw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW
  ASMJIT_INST_2x(vpscatterdd, Vpscatterdd, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterdd, Vpscatterdd, X86Mem, X86Ymm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterdd, Vpscatterdd, X86Mem, X86Zmm)                    //      AVX512_F{k}
  ASMJIT_INST_2x(vpscatterdq, Vpscatterdq, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterdq, Vpscatterdq, X86Mem, X86Ymm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterdq, Vpscatterdq, X86Mem, X86Zmm)                    //      AVX512_F{k}
  ASMJIT_INST_2x(vpscatterqd, Vpscatterqd, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterqd, Vpscatterqd, X86Mem, X86Ymm)                    //      AVX512_F{k}
  ASMJIT_INST_2x(vpscatterqq, Vpscatterqq, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterqq, Vpscatterqq, X86Mem, X86Ymm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vpscatterqq, Vpscatterqq, X86Mem, X86Zmm)                    //      AVX512_F{k}
  ASMJIT_INST_3x(vpshufb, Vpshufb, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpshufb, Vpshufb, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpshufb, Vpshufb, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpshufb, Vpshufb, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpshufb, Vpshufb, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpshufb, Vpshufb, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpshufd, Vpshufd, X86Xmm, X86Xmm, Imm)                       // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpshufd, Vpshufd, X86Xmm, X86Mem, Imm)                       // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpshufd, Vpshufd, X86Ymm, X86Ymm, Imm)                       // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpshufd, Vpshufd, X86Ymm, X86Mem, Imm)                       // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpshufd, Vpshufd, X86Zmm, X86Zmm, Imm)                       //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpshufd, Vpshufd, X86Zmm, X86Mem, Imm)                       //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpshufhw, Vpshufhw, X86Xmm, X86Xmm, Imm)                     // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshufhw, Vpshufhw, X86Xmm, X86Mem, Imm)                     // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshufhw, Vpshufhw, X86Ymm, X86Ymm, Imm)                     // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshufhw, Vpshufhw, X86Ymm, X86Mem, Imm)                     // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshufhw, Vpshufhw, X86Zmm, X86Zmm, Imm)                     //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpshufhw, Vpshufhw, X86Zmm, X86Mem, Imm)                     //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpshuflw, Vpshuflw, X86Xmm, X86Xmm, Imm)                     // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshuflw, Vpshuflw, X86Xmm, X86Mem, Imm)                     // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshuflw, Vpshuflw, X86Ymm, X86Ymm, Imm)                     // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshuflw, Vpshuflw, X86Ymm, X86Mem, Imm)                     // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpshuflw, Vpshuflw, X86Zmm, X86Zmm, Imm)                     //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpshuflw, Vpshuflw, X86Zmm, X86Mem, Imm)                     //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsignb, Vpsignb, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vpsignb, Vpsignb, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vpsignb, Vpsignb, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vpsignb, Vpsignb, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3x(vpsignd, Vpsignd, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vpsignd, Vpsignd, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vpsignd, Vpsignd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vpsignd, Vpsignd, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3x(vpsignw, Vpsignw, X86Xmm, X86Xmm, X86Xmm)                    // AVX
  ASMJIT_INST_3x(vpsignw, Vpsignw, X86Xmm, X86Xmm, X86Mem)                    // AVX
  ASMJIT_INST_3x(vpsignw, Vpsignw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2
  ASMJIT_INST_3x(vpsignw, Vpsignw, X86Ymm, X86Ymm, X86Mem)                    // AVX2
  ASMJIT_INST_3i(vpslld, Vpslld, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpslld, Vpslld, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpslld, Vpslld, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpslld, Vpslld, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpslld, Vpslld, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpslld, Vpslld, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpslld, Vpslld, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpslld, Vpslld, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpslld, Vpslld, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_F{kz}
  ASMJIT_INST_3x(vpslld, Vpslld, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz}
  ASMJIT_INST_3i(vpslld, Vpslld, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpslld, Vpslld, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpslldq, Vpslldq, X86Xmm, X86Xmm, Imm)                       // AVX  AVX512_BW-VL
  ASMJIT_INST_3i(vpslldq, Vpslldq, X86Ymm, X86Ymm, Imm)                       // AVX2 AVX512_BW-VL
  ASMJIT_INST_3i(vpslldq, Vpslldq, X86Xmm, X86Mem, Imm)                       //      AVX512_BW-VL
  ASMJIT_INST_3i(vpslldq, Vpslldq, X86Ymm, X86Mem, Imm)                       //      AVX512_BW-VL
  ASMJIT_INST_3i(vpslldq, Vpslldq, X86Zmm, X86Zmm, Imm)                       //      AVX512_BW
  ASMJIT_INST_3i(vpslldq, Vpslldq, X86Zmm, X86Mem, Imm)                       //      AVX512_BW
  ASMJIT_INST_3i(vpsllq, Vpsllq, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllq, Vpsllq, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsllq, Vpsllq, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsllq, Vpsllq, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllq, Vpsllq, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsllq, Vpsllq, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsllq, Vpsllq, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpsllq, Vpsllq, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllq, Vpsllq, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_F{kz}
  ASMJIT_INST_3x(vpsllq, Vpsllq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz}
  ASMJIT_INST_3i(vpsllq, Vpsllq, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpsllq, Vpsllq, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsllvd, Vpsllvd, X86Xmm, X86Xmm, X86Xmm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsllvd, Vpsllvd, X86Xmm, X86Xmm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsllvd, Vpsllvd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsllvd, Vpsllvd, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsllvd, Vpsllvd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsllvd, Vpsllvd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsllvq, Vpsllvq, X86Xmm, X86Xmm, X86Xmm)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllvq, Vpsllvq, X86Xmm, X86Xmm, X86Mem)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllvq, Vpsllvq, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllvq, Vpsllvq, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsllvq, Vpsllvq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsllvq, Vpsllvq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsllvw, Vpsllvw, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllvw, Vpsllvw, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllvw, Vpsllvw, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllvw, Vpsllvw, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllvw, Vpsllvw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsllvw, Vpsllvw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsllw, Vpsllw, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllw, Vpsllw, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllw, Vpsllw, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsllw, Vpsllw, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllw, Vpsllw, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllw, Vpsllw, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsllw, Vpsllw, X86Xmm, X86Mem, Imm)                         //      AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsllw, Vpsllw, X86Ymm, X86Mem, Imm)                         //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsllw, Vpsllw, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsllw, Vpsllw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsllw, Vpsllw, X86Zmm, X86Zmm, Imm)                         //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsllw, Vpsllw, X86Zmm, X86Mem, Imm)                         //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsrad, Vpsrad, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrad, Vpsrad, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsrad, Vpsrad, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsrad, Vpsrad, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrad, Vpsrad, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsrad, Vpsrad, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsrad, Vpsrad, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpsrad, Vpsrad, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrad, Vpsrad, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_F{kz}
  ASMJIT_INST_3x(vpsrad, Vpsrad, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz}
  ASMJIT_INST_3i(vpsrad, Vpsrad, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpsrad, Vpsrad, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsraq, Vpsraq, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsraq, Vpsraq, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsraq, Vpsraq, X86Xmm, X86Xmm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpsraq, Vpsraq, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsraq, Vpsraq, X86Ymm, X86Ymm, X86Xmm)                      //      AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsraq, Vpsraq, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsraq, Vpsraq, X86Ymm, X86Ymm, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpsraq, Vpsraq, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsraq, Vpsraq, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_F{kz}
  ASMJIT_INST_3x(vpsraq, Vpsraq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz}
  ASMJIT_INST_3i(vpsraq, Vpsraq, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpsraq, Vpsraq, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsravd, Vpsravd, X86Xmm, X86Xmm, X86Xmm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsravd, Vpsravd, X86Xmm, X86Xmm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsravd, Vpsravd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsravd, Vpsravd, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsravd, Vpsravd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsravd, Vpsravd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsravq, Vpsravq, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsravq, Vpsravq, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsravq, Vpsravq, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsravq, Vpsravq, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsravq, Vpsravq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsravq, Vpsravq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsravw, Vpsravw, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsravw, Vpsravw, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsravw, Vpsravw, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsravw, Vpsravw, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsravw, Vpsravw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsravw, Vpsravw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsraw, Vpsraw, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsraw, Vpsraw, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsraw, Vpsraw, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsraw, Vpsraw, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsraw, Vpsraw, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsraw, Vpsraw, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsraw, Vpsraw, X86Xmm, X86Mem, Imm)                         //      AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsraw, Vpsraw, X86Ymm, X86Mem, Imm)                         //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsraw, Vpsraw, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsraw, Vpsraw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsraw, Vpsraw, X86Zmm, X86Zmm, Imm)                         //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsraw, Vpsraw, X86Zmm, X86Mem, Imm)                         //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsrld, Vpsrld, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrld, Vpsrld, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsrld, Vpsrld, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsrld, Vpsrld, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrld, Vpsrld, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsrld, Vpsrld, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsrld, Vpsrld, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vpsrld, Vpsrld, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrld, Vpsrld, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_F{kz}
  ASMJIT_INST_3x(vpsrld, Vpsrld, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz}
  ASMJIT_INST_3i(vpsrld, Vpsrld, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpsrld, Vpsrld, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b32}
  ASMJIT_INST_3i(vpsrldq, Vpsrldq, X86Xmm, X86Xmm, Imm)                       // AVX  AVX512_BW-VL
  ASMJIT_INST_3i(vpsrldq, Vpsrldq, X86Ymm, X86Ymm, Imm)                       // AVX2 AVX512_BW-VL
  ASMJIT_INST_3i(vpsrldq, Vpsrldq, X86Xmm, X86Mem, Imm)                       //      AVX512_BW-VL
  ASMJIT_INST_3i(vpsrldq, Vpsrldq, X86Ymm, X86Mem, Imm)                       //      AVX512_BW-VL
  ASMJIT_INST_3i(vpsrldq, Vpsrldq, X86Zmm, X86Zmm, Imm)                       //      AVX512_BW
  ASMJIT_INST_3i(vpsrldq, Vpsrldq, X86Zmm, X86Mem, Imm)                       //      AVX512_BW
  ASMJIT_INST_3i(vpsrlq, Vpsrlq, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlq, Vpsrlq, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsrlq, Vpsrlq, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsrlq, Vpsrlq, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlq, Vpsrlq, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3x(vpsrlq, Vpsrlq, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz}-VL
  ASMJIT_INST_3i(vpsrlq, Vpsrlq, X86Xmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vpsrlq, Vpsrlq, X86Ymm, X86Mem, Imm)                         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlq, Vpsrlq, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_F{kz}
  ASMJIT_INST_3x(vpsrlq, Vpsrlq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz}
  ASMJIT_INST_3i(vpsrlq, Vpsrlq, X86Zmm, X86Zmm, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3i(vpsrlq, Vpsrlq, X86Zmm, X86Mem, Imm)                         //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsrlvd, Vpsrlvd, X86Xmm, X86Xmm, X86Xmm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrlvd, Vpsrlvd, X86Xmm, X86Xmm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrlvd, Vpsrlvd, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrlvd, Vpsrlvd, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsrlvd, Vpsrlvd, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsrlvd, Vpsrlvd, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsrlvq, Vpsrlvq, X86Xmm, X86Xmm, X86Xmm)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlvq, Vpsrlvq, X86Xmm, X86Xmm, X86Mem)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlvq, Vpsrlvq, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlvq, Vpsrlvq, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsrlvq, Vpsrlvq, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsrlvq, Vpsrlvq, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsrlvw, Vpsrlvw, X86Xmm, X86Xmm, X86Xmm)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlvw, Vpsrlvw, X86Xmm, X86Xmm, X86Mem)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlvw, Vpsrlvw, X86Ymm, X86Ymm, X86Ymm)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlvw, Vpsrlvw, X86Ymm, X86Ymm, X86Mem)                    //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlvw, Vpsrlvw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsrlvw, Vpsrlvw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsrlw, Vpsrlw, X86Xmm, X86Xmm, Imm)                         // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlw, Vpsrlw, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlw, Vpsrlw, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsrlw, Vpsrlw, X86Ymm, X86Ymm, Imm)                         // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlw, Vpsrlw, X86Ymm, X86Ymm, X86Xmm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlw, Vpsrlw, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsrlw, Vpsrlw, X86Xmm, X86Mem, Imm)                         //      AVX512_BW{kz}-VL
  ASMJIT_INST_3i(vpsrlw, Vpsrlw, X86Ymm, X86Mem, Imm)                         //      AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsrlw, Vpsrlw, X86Zmm, X86Zmm, X86Xmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsrlw, Vpsrlw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsrlw, Vpsrlw, X86Zmm, X86Zmm, Imm)                         //      AVX512_BW{kz}
  ASMJIT_INST_3i(vpsrlw, Vpsrlw, X86Zmm, X86Mem, Imm)                         //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubb, Vpsubb, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubb, Vpsubb, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubb, Vpsubb, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubb, Vpsubb, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubb, Vpsubb, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubb, Vpsubb, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubd, Vpsubd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsubd, Vpsubd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsubd, Vpsubd, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsubd, Vpsubd, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpsubd, Vpsubd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsubd, Vpsubd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpsubq, Vpsubq, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsubq, Vpsubq, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsubq, Vpsubq, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsubq, Vpsubq, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpsubq, Vpsubq, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsubq, Vpsubq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpsubsb, Vpsubsb, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsb, Vpsubsb, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsb, Vpsubsb, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsb, Vpsubsb, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsb, Vpsubsb, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubsb, Vpsubsb, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubsw, Vpsubsw, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsw, Vpsubsw, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsw, Vpsubsw, X86Ymm, X86Ymm, X86Ymm)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsw, Vpsubsw, X86Ymm, X86Ymm, X86Mem)                    // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubsw, Vpsubsw, X86Zmm, X86Zmm, X86Zmm)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubsw, Vpsubsw, X86Zmm, X86Zmm, X86Mem)                    //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubusb, Vpsubusb, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusb, Vpsubusb, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusb, Vpsubusb, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusb, Vpsubusb, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusb, Vpsubusb, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubusb, Vpsubusb, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubusw, Vpsubusw, X86Xmm, X86Xmm, X86Xmm)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusw, Vpsubusw, X86Xmm, X86Xmm, X86Mem)                  // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusw, Vpsubusw, X86Ymm, X86Ymm, X86Ymm)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusw, Vpsubusw, X86Ymm, X86Ymm, X86Mem)                  // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubusw, Vpsubusw, X86Zmm, X86Zmm, X86Zmm)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubusw, Vpsubusw, X86Zmm, X86Zmm, X86Mem)                  //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubw, Vpsubw, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubw, Vpsubw, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubw, Vpsubw, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubw, Vpsubw, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpsubw, Vpsubw, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpsubw, Vpsubw, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_BW{kz}
  ASMJIT_INST_4i(vpternlogd, Vpternlogd, X86Xmm, X86Xmm, X86Xmm, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vpternlogd, Vpternlogd, X86Xmm, X86Xmm, X86Mem, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vpternlogd, Vpternlogd, X86Ymm, X86Ymm, X86Ymm, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vpternlogd, Vpternlogd, X86Ymm, X86Ymm, X86Mem, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vpternlogd, Vpternlogd, X86Zmm, X86Zmm, X86Zmm, Imm)         //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vpternlogd, Vpternlogd, X86Zmm, X86Zmm, X86Mem, Imm)         //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vpternlogq, Vpternlogq, X86Xmm, X86Xmm, X86Xmm, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vpternlogq, Vpternlogq, X86Xmm, X86Xmm, X86Mem, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vpternlogq, Vpternlogq, X86Ymm, X86Ymm, X86Ymm, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vpternlogq, Vpternlogq, X86Ymm, X86Ymm, X86Mem, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vpternlogq, Vpternlogq, X86Zmm, X86Zmm, X86Zmm, Imm)         //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vpternlogq, Vpternlogq, X86Zmm, X86Zmm, X86Mem, Imm)         //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vptest, Vptest, X86Xmm, X86Xmm)                              // AVX
  ASMJIT_INST_2x(vptest, Vptest, X86Xmm, X86Mem)                              // AVX
  ASMJIT_INST_2x(vptest, Vptest, X86Ymm, X86Ymm)                              // AVX
  ASMJIT_INST_2x(vptest, Vptest, X86Ymm, X86Mem)                              // AVX
  ASMJIT_INST_3x(vptestmb, Vptestmb, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmb, Vptestmb, X86KReg, X86Xmm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmb, Vptestmb, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmb, Vptestmb, X86KReg, X86Ymm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmb, Vptestmb, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestmb, Vptestmb, X86KReg, X86Zmm, X86Mem)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestmd, Vptestmd, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestmd, Vptestmd, X86KReg, X86Xmm, X86Mem)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestmd, Vptestmd, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestmd, Vptestmd, X86KReg, X86Ymm, X86Mem)                 //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestmd, Vptestmd, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vptestmd, Vptestmd, X86KReg, X86Zmm, X86Mem)                 //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vptestmq, Vptestmq, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestmq, Vptestmq, X86KReg, X86Xmm, X86Mem)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestmq, Vptestmq, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestmq, Vptestmq, X86KReg, X86Ymm, X86Mem)                 //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestmq, Vptestmq, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vptestmq, Vptestmq, X86KReg, X86Zmm, X86Mem)                 //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vptestmw, Vptestmw, X86KReg, X86Xmm, X86Xmm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmw, Vptestmw, X86KReg, X86Xmm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmw, Vptestmw, X86KReg, X86Ymm, X86Ymm)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmw, Vptestmw, X86KReg, X86Ymm, X86Mem)                 //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestmw, Vptestmw, X86KReg, X86Zmm, X86Zmm)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestmw, Vptestmw, X86KReg, X86Zmm, X86Mem)                 //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestnmb, Vptestnmb, X86KReg, X86Xmm, X86Xmm)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmb, Vptestnmb, X86KReg, X86Xmm, X86Mem)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmb, Vptestnmb, X86KReg, X86Ymm, X86Ymm)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmb, Vptestnmb, X86KReg, X86Ymm, X86Mem)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmb, Vptestnmb, X86KReg, X86Zmm, X86Zmm)               //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestnmb, Vptestnmb, X86KReg, X86Zmm, X86Mem)               //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestnmd, Vptestnmd, X86KReg, X86Xmm, X86Xmm)               //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestnmd, Vptestnmd, X86KReg, X86Xmm, X86Mem)               //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestnmd, Vptestnmd, X86KReg, X86Ymm, X86Ymm)               //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestnmd, Vptestnmd, X86KReg, X86Ymm, X86Mem)               //      AVX512_F{k|b32}-VL
  ASMJIT_INST_3x(vptestnmd, Vptestnmd, X86KReg, X86Zmm, X86Zmm)               //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vptestnmd, Vptestnmd, X86KReg, X86Zmm, X86Mem)               //      AVX512_F{k|b32}
  ASMJIT_INST_3x(vptestnmq, Vptestnmq, X86KReg, X86Xmm, X86Xmm)               //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestnmq, Vptestnmq, X86KReg, X86Xmm, X86Mem)               //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestnmq, Vptestnmq, X86KReg, X86Ymm, X86Ymm)               //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestnmq, Vptestnmq, X86KReg, X86Ymm, X86Mem)               //      AVX512_F{k|b64}-VL
  ASMJIT_INST_3x(vptestnmq, Vptestnmq, X86KReg, X86Zmm, X86Zmm)               //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vptestnmq, Vptestnmq, X86KReg, X86Zmm, X86Mem)               //      AVX512_F{k|b64}
  ASMJIT_INST_3x(vptestnmw, Vptestnmw, X86KReg, X86Xmm, X86Xmm)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmw, Vptestnmw, X86KReg, X86Xmm, X86Mem)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmw, Vptestnmw, X86KReg, X86Ymm, X86Ymm)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmw, Vptestnmw, X86KReg, X86Ymm, X86Mem)               //      AVX512_BW{k}-VL
  ASMJIT_INST_3x(vptestnmw, Vptestnmw, X86KReg, X86Zmm, X86Zmm)               //      AVX512_BW{k}
  ASMJIT_INST_3x(vptestnmw, Vptestnmw, X86KReg, X86Zmm, X86Mem)               //      AVX512_BW{k}
  ASMJIT_INST_3x(vpunpckhbw, Vpunpckhbw, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhbw, Vpunpckhbw, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhbw, Vpunpckhbw, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhbw, Vpunpckhbw, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhbw, Vpunpckhbw, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpckhbw, Vpunpckhbw, X86Zmm, X86Zmm, X86Mem)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpckhdq, Vpunpckhdq, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckhdq, Vpunpckhdq, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckhdq, Vpunpckhdq, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckhdq, Vpunpckhdq, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckhdq, Vpunpckhdq, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpunpckhdq, Vpunpckhdq, X86Zmm, X86Zmm, X86Mem)              //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpunpckhqdq, Vpunpckhqdq, X86Xmm, X86Xmm, X86Xmm)            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpckhqdq, Vpunpckhqdq, X86Xmm, X86Xmm, X86Mem)            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpckhqdq, Vpunpckhqdq, X86Ymm, X86Ymm, X86Ymm)            // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpckhqdq, Vpunpckhqdq, X86Ymm, X86Ymm, X86Mem)            // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpckhqdq, Vpunpckhqdq, X86Zmm, X86Zmm, X86Zmm)            //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpunpckhqdq, Vpunpckhqdq, X86Zmm, X86Zmm, X86Mem)            //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpunpckhwd, Vpunpckhwd, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhwd, Vpunpckhwd, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhwd, Vpunpckhwd, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhwd, Vpunpckhwd, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpckhwd, Vpunpckhwd, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpckhwd, Vpunpckhwd, X86Zmm, X86Zmm, X86Mem)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpcklbw, Vpunpcklbw, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklbw, Vpunpcklbw, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklbw, Vpunpcklbw, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklbw, Vpunpcklbw, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklbw, Vpunpcklbw, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpcklbw, Vpunpcklbw, X86Zmm, X86Zmm, X86Mem)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpckldq, Vpunpckldq, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckldq, Vpunpckldq, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckldq, Vpunpckldq, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckldq, Vpunpckldq, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpunpckldq, Vpunpckldq, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpunpckldq, Vpunpckldq, X86Zmm, X86Zmm, X86Mem)              //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpunpcklqdq, Vpunpcklqdq, X86Xmm, X86Xmm, X86Xmm)            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpcklqdq, Vpunpcklqdq, X86Xmm, X86Xmm, X86Mem)            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpcklqdq, Vpunpcklqdq, X86Ymm, X86Ymm, X86Ymm)            // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpcklqdq, Vpunpcklqdq, X86Ymm, X86Ymm, X86Mem)            // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpunpcklqdq, Vpunpcklqdq, X86Zmm, X86Zmm, X86Zmm)            //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpunpcklqdq, Vpunpcklqdq, X86Zmm, X86Zmm, X86Mem)            //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpunpcklwd, Vpunpcklwd, X86Xmm, X86Xmm, X86Xmm)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklwd, Vpunpcklwd, X86Xmm, X86Xmm, X86Mem)              // AVX  AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklwd, Vpunpcklwd, X86Ymm, X86Ymm, X86Ymm)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklwd, Vpunpcklwd, X86Ymm, X86Ymm, X86Mem)              // AVX2 AVX512_BW{kz}-VL
  ASMJIT_INST_3x(vpunpcklwd, Vpunpcklwd, X86Zmm, X86Zmm, X86Zmm)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpunpcklwd, Vpunpcklwd, X86Zmm, X86Zmm, X86Mem)              //      AVX512_BW{kz}
  ASMJIT_INST_3x(vpxor, Vpxor, X86Xmm, X86Xmm, X86Xmm)                        // AVX
  ASMJIT_INST_3x(vpxor, Vpxor, X86Xmm, X86Xmm, X86Mem)                        // AVX
  ASMJIT_INST_3x(vpxor, Vpxor, X86Ymm, X86Ymm, X86Ymm)                        // AVX2
  ASMJIT_INST_3x(vpxor, Vpxor, X86Ymm, X86Ymm, X86Mem)                        // AVX2
  ASMJIT_INST_3x(vpxord, Vpxord, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpxord, Vpxord, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpxord, Vpxord, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpxord, Vpxord, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vpxord, Vpxord, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpxord, Vpxord, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vpxorq, Vpxorq, X86Xmm, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpxorq, Vpxorq, X86Xmm, X86Xmm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpxorq, Vpxorq, X86Ymm, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpxorq, Vpxorq, X86Ymm, X86Ymm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vpxorq, Vpxorq, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vpxorq, Vpxorq, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vrangepd, Vrangepd, X86Xmm, X86Xmm, X86Xmm, Imm)             //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_4i(vrangepd, Vrangepd, X86Xmm, X86Xmm, X86Mem, Imm)             //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_4i(vrangepd, Vrangepd, X86Ymm, X86Ymm, X86Ymm, Imm)             //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_4i(vrangepd, Vrangepd, X86Ymm, X86Ymm, X86Mem, Imm)             //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_4i(vrangepd, Vrangepd, X86Zmm, X86Zmm, X86Zmm, Imm)             //      AVX512_DQ{kz|sae|b64}
  ASMJIT_INST_4i(vrangepd, Vrangepd, X86Zmm, X86Zmm, X86Mem, Imm)             //      AVX512_DQ{kz|sae|b64}
  ASMJIT_INST_4i(vrangeps, Vrangeps, X86Xmm, X86Xmm, X86Xmm, Imm)             //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_4i(vrangeps, Vrangeps, X86Xmm, X86Xmm, X86Mem, Imm)             //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_4i(vrangeps, Vrangeps, X86Ymm, X86Ymm, X86Ymm, Imm)             //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_4i(vrangeps, Vrangeps, X86Ymm, X86Ymm, X86Mem, Imm)             //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_4i(vrangeps, Vrangeps, X86Zmm, X86Zmm, X86Zmm, Imm)             //      AVX512_DQ{kz|sae|b32}
  ASMJIT_INST_4i(vrangeps, Vrangeps, X86Zmm, X86Zmm, X86Mem, Imm)             //      AVX512_DQ{kz|sae|b32}
  ASMJIT_INST_4i(vrangesd, Vrangesd, X86Xmm, X86Xmm, X86Xmm, Imm)             //      AVX512_DQ{kz|sae}
  ASMJIT_INST_4i(vrangesd, Vrangesd, X86Xmm, X86Xmm, X86Mem, Imm)             //      AVX512_DQ{kz|sae}
  ASMJIT_INST_4i(vrangess, Vrangess, X86Xmm, X86Xmm, X86Xmm, Imm)             //      AVX512_DQ{kz|sae}
  ASMJIT_INST_4i(vrangess, Vrangess, X86Xmm, X86Xmm, X86Mem, Imm)             //      AVX512_DQ{kz|sae}
  ASMJIT_INST_2x(vrcp14pd, Vrcp14pd, X86Xmm, X86Xmm)                          //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrcp14pd, Vrcp14pd, X86Xmm, X86Mem)                          //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrcp14pd, Vrcp14pd, X86Ymm, X86Ymm)                          //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrcp14pd, Vrcp14pd, X86Ymm, X86Mem)                          //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrcp14pd, Vrcp14pd, X86Zmm, X86Zmm)                          //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vrcp14pd, Vrcp14pd, X86Zmm, X86Mem)                          //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vrcp14ps, Vrcp14ps, X86Xmm, X86Xmm)                          //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrcp14ps, Vrcp14ps, X86Xmm, X86Mem)                          //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrcp14ps, Vrcp14ps, X86Ymm, X86Ymm)                          //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrcp14ps, Vrcp14ps, X86Ymm, X86Mem)                          //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrcp14ps, Vrcp14ps, X86Zmm, X86Zmm)                          //      AVX512_F{kz|b32}
  ASMJIT_INST_2x(vrcp14ps, Vrcp14ps, X86Zmm, X86Mem)                          //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vrcp14sd, Vrcp14sd, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz}
  ASMJIT_INST_3x(vrcp14sd, Vrcp14sd, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz}
  ASMJIT_INST_3x(vrcp14ss, Vrcp14ss, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_F{kz}
  ASMJIT_INST_3x(vrcp14ss, Vrcp14ss, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_F{kz}
  ASMJIT_INST_2x(vrcp28pd, Vrcp28pd, X86Zmm, X86Zmm)                          //      AVX512_ER{kz|sae|b64}
  ASMJIT_INST_2x(vrcp28pd, Vrcp28pd, X86Zmm, X86Mem)                          //      AVX512_ER{kz|sae|b64}
  ASMJIT_INST_2x(vrcp28ps, Vrcp28ps, X86Zmm, X86Zmm)                          //      AVX512_ER{kz|sae|b32}
  ASMJIT_INST_2x(vrcp28ps, Vrcp28ps, X86Zmm, X86Mem)                          //      AVX512_ER{kz|sae|b32}
  ASMJIT_INST_3x(vrcp28sd, Vrcp28sd, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_ER{kz|sae}
  ASMJIT_INST_3x(vrcp28sd, Vrcp28sd, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_ER{kz|sae}
  ASMJIT_INST_3x(vrcp28ss, Vrcp28ss, X86Xmm, X86Xmm, X86Xmm)                  //      AVX512_ER{kz|sae}
  ASMJIT_INST_3x(vrcp28ss, Vrcp28ss, X86Xmm, X86Xmm, X86Mem)                  //      AVX512_ER{kz|sae}
  ASMJIT_INST_2x(vrcpps, Vrcpps, X86Xmm, X86Xmm)                              // AVX
  ASMJIT_INST_2x(vrcpps, Vrcpps, X86Xmm, X86Mem)                              // AVX
  ASMJIT_INST_2x(vrcpps, Vrcpps, X86Ymm, X86Ymm)                              // AVX
  ASMJIT_INST_2x(vrcpps, Vrcpps, X86Ymm, X86Mem)                              // AVX
  ASMJIT_INST_3x(vrcpss, Vrcpss, X86Xmm, X86Xmm, X86Xmm)                      // AVX
  ASMJIT_INST_3x(vrcpss, Vrcpss, X86Xmm, X86Xmm, X86Mem)                      // AVX
  ASMJIT_INST_3i(vreducepd, Vreducepd, X86Xmm, X86Xmm, Imm)                   //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3i(vreducepd, Vreducepd, X86Xmm, X86Mem, Imm)                   //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3i(vreducepd, Vreducepd, X86Ymm, X86Ymm, Imm)                   //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3i(vreducepd, Vreducepd, X86Ymm, X86Mem, Imm)                   //      AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3i(vreducepd, Vreducepd, X86Zmm, X86Zmm, Imm)                   //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3i(vreducepd, Vreducepd, X86Zmm, X86Mem, Imm)                   //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3i(vreduceps, Vreduceps, X86Xmm, X86Xmm, Imm)                   //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3i(vreduceps, Vreduceps, X86Xmm, X86Mem, Imm)                   //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3i(vreduceps, Vreduceps, X86Ymm, X86Ymm, Imm)                   //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3i(vreduceps, Vreduceps, X86Ymm, X86Mem, Imm)                   //      AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3i(vreduceps, Vreduceps, X86Zmm, X86Zmm, Imm)                   //      AVX512_DQ{kz|b32}
  ASMJIT_INST_3i(vreduceps, Vreduceps, X86Zmm, X86Mem, Imm)                   //      AVX512_DQ{kz|b32}
  ASMJIT_INST_4i(vreducesd, Vreducesd, X86Xmm, X86Xmm, X86Xmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vreducesd, Vreducesd, X86Xmm, X86Xmm, X86Mem, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vreducess, Vreducess, X86Xmm, X86Xmm, X86Xmm, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_4i(vreducess, Vreducess, X86Xmm, X86Xmm, X86Mem, Imm)           //      AVX512_DQ{kz}
  ASMJIT_INST_3i(vrndscalepd, Vrndscalepd, X86Xmm, X86Xmm, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vrndscalepd, Vrndscalepd, X86Xmm, X86Mem, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vrndscalepd, Vrndscalepd, X86Ymm, X86Ymm, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vrndscalepd, Vrndscalepd, X86Ymm, X86Mem, Imm)               //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3i(vrndscalepd, Vrndscalepd, X86Zmm, X86Zmm, Imm)               //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3i(vrndscalepd, Vrndscalepd, X86Zmm, X86Mem, Imm)               //      AVX512_F{kz|sae|b64}
  ASMJIT_INST_3i(vrndscaleps, Vrndscaleps, X86Xmm, X86Xmm, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vrndscaleps, Vrndscaleps, X86Xmm, X86Mem, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vrndscaleps, Vrndscaleps, X86Ymm, X86Ymm, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vrndscaleps, Vrndscaleps, X86Ymm, X86Mem, Imm)               //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3i(vrndscaleps, Vrndscaleps, X86Zmm, X86Zmm, Imm)               //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_3i(vrndscaleps, Vrndscaleps, X86Zmm, X86Mem, Imm)               //      AVX512_F{kz|sae|b32}
  ASMJIT_INST_4i(vrndscalesd, Vrndscalesd, X86Xmm, X86Xmm, X86Xmm, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vrndscalesd, Vrndscalesd, X86Xmm, X86Xmm, X86Mem, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vrndscaless, Vrndscaless, X86Xmm, X86Xmm, X86Xmm, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_4i(vrndscaless, Vrndscaless, X86Xmm, X86Xmm, X86Mem, Imm)       //      AVX512_F{kz|sae}
  ASMJIT_INST_3i(vroundpd, Vroundpd, X86Xmm, X86Xmm, Imm)                     // AVX
  ASMJIT_INST_3i(vroundpd, Vroundpd, X86Xmm, X86Mem, Imm)                     // AVX
  ASMJIT_INST_3i(vroundpd, Vroundpd, X86Ymm, X86Ymm, Imm)                     // AVX
  ASMJIT_INST_3i(vroundpd, Vroundpd, X86Ymm, X86Mem, Imm)                     // AVX
  ASMJIT_INST_3i(vroundps, Vroundps, X86Xmm, X86Xmm, Imm)                     // AVX
  ASMJIT_INST_3i(vroundps, Vroundps, X86Xmm, X86Mem, Imm)                     // AVX
  ASMJIT_INST_3i(vroundps, Vroundps, X86Ymm, X86Ymm, Imm)                     // AVX
  ASMJIT_INST_3i(vroundps, Vroundps, X86Ymm, X86Mem, Imm)                     // AVX
  ASMJIT_INST_4i(vroundsd, Vroundsd, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX
  ASMJIT_INST_4i(vroundsd, Vroundsd, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX
  ASMJIT_INST_4i(vroundss, Vroundss, X86Xmm, X86Xmm, X86Xmm, Imm)             // AVX
  ASMJIT_INST_4i(vroundss, Vroundss, X86Xmm, X86Xmm, X86Mem, Imm)             // AVX
  ASMJIT_INST_2x(vrsqrt14pd, Vrsqrt14pd, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrsqrt14pd, Vrsqrt14pd, X86Xmm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrsqrt14pd, Vrsqrt14pd, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrsqrt14pd, Vrsqrt14pd, X86Ymm, X86Mem)                      //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vrsqrt14pd, Vrsqrt14pd, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vrsqrt14pd, Vrsqrt14pd, X86Zmm, X86Mem)                      //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vrsqrt14ps, Vrsqrt14ps, X86Xmm, X86Xmm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrsqrt14ps, Vrsqrt14ps, X86Xmm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrsqrt14ps, Vrsqrt14ps, X86Ymm, X86Ymm)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrsqrt14ps, Vrsqrt14ps, X86Ymm, X86Mem)                      //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vrsqrt14ps, Vrsqrt14ps, X86Zmm, X86Zmm)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_2x(vrsqrt14ps, Vrsqrt14ps, X86Zmm, X86Mem)                      //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vrsqrt14sd, Vrsqrt14sd, X86Xmm, X86Xmm, X86Xmm)              //      AVX512_F{kz}
  ASMJIT_INST_3x(vrsqrt14sd, Vrsqrt14sd, X86Xmm, X86Xmm, X86Mem)              //      AVX512_F{kz}
  ASMJIT_INST_3x(vrsqrt14ss, Vrsqrt14ss, X86Xmm, X86Xmm, X86Xmm)              //      AVX512_F{kz}
  ASMJIT_INST_3x(vrsqrt14ss, Vrsqrt14ss, X86Xmm, X86Xmm, X86Mem)              //      AVX512_F{kz}
  ASMJIT_INST_2x(vrsqrt28pd, Vrsqrt28pd, X86Zmm, X86Zmm)                      //      AVX512_ER{kz|sae|b64}
  ASMJIT_INST_2x(vrsqrt28pd, Vrsqrt28pd, X86Zmm, X86Mem)                      //      AVX512_ER{kz|sae|b64}
  ASMJIT_INST_2x(vrsqrt28ps, Vrsqrt28ps, X86Zmm, X86Zmm)                      //      AVX512_ER{kz|sae|b32}
  ASMJIT_INST_2x(vrsqrt28ps, Vrsqrt28ps, X86Zmm, X86Mem)                      //      AVX512_ER{kz|sae|b32}
  ASMJIT_INST_3x(vrsqrt28sd, Vrsqrt28sd, X86Xmm, X86Xmm, X86Xmm)              //      AVX512_ER{kz|sae}
  ASMJIT_INST_3x(vrsqrt28sd, Vrsqrt28sd, X86Xmm, X86Xmm, X86Mem)              //      AVX512_ER{kz|sae}
  ASMJIT_INST_3x(vrsqrt28ss, Vrsqrt28ss, X86Xmm, X86Xmm, X86Xmm)              //      AVX512_ER{kz|sae}
  ASMJIT_INST_3x(vrsqrt28ss, Vrsqrt28ss, X86Xmm, X86Xmm, X86Mem)              //      AVX512_ER{kz|sae}
  ASMJIT_INST_2x(vrsqrtps, Vrsqrtps, X86Xmm, X86Xmm)                          // AVX
  ASMJIT_INST_2x(vrsqrtps, Vrsqrtps, X86Xmm, X86Mem)                          // AVX
  ASMJIT_INST_2x(vrsqrtps, Vrsqrtps, X86Ymm, X86Ymm)                          // AVX
  ASMJIT_INST_2x(vrsqrtps, Vrsqrtps, X86Ymm, X86Mem)                          // AVX
  ASMJIT_INST_3x(vrsqrtss, Vrsqrtss, X86Xmm, X86Xmm, X86Xmm)                  // AVX
  ASMJIT_INST_3x(vrsqrtss, Vrsqrtss, X86Xmm, X86Xmm, X86Mem)                  // AVX
  ASMJIT_INST_3x(vscalefpd, Vscalefpd, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vscalefpd, Vscalefpd, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vscalefpd, Vscalefpd, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vscalefpd, Vscalefpd, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vscalefpd, Vscalefpd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vscalefpd, Vscalefpd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vscalefps, Vscalefps, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vscalefps, Vscalefps, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vscalefps, Vscalefps, X86Ymm, X86Ymm, X86Ymm)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vscalefps, Vscalefps, X86Ymm, X86Ymm, X86Mem)                //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vscalefps, Vscalefps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vscalefps, Vscalefps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vscalefsd, Vscalefsd, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|er}
  ASMJIT_INST_3x(vscalefsd, Vscalefsd, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|er}
  ASMJIT_INST_3x(vscalefss, Vscalefss, X86Xmm, X86Xmm, X86Xmm)                //      AVX512_F{kz|er}
  ASMJIT_INST_3x(vscalefss, Vscalefss, X86Xmm, X86Xmm, X86Mem)                //      AVX512_F{kz|er}
  ASMJIT_INST_2x(vscatterdpd, Vscatterdpd, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterdpd, Vscatterdpd, X86Mem, X86Ymm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterdpd, Vscatterdpd, X86Mem, X86Zmm)                    //      AVX512_F{k}
  ASMJIT_INST_2x(vscatterdps, Vscatterdps, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterdps, Vscatterdps, X86Mem, X86Ymm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterdps, Vscatterdps, X86Mem, X86Zmm)                    //      AVX512_F{k}
  ASMJIT_INST_1x(vscatterpf0dpd, Vscatterpf0dpd, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf0dps, Vscatterpf0dps, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf0qpd, Vscatterpf0qpd, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf0qps, Vscatterpf0qps, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf1dpd, Vscatterpf1dpd, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf1dps, Vscatterpf1dps, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf1qpd, Vscatterpf1qpd, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_1x(vscatterpf1qps, Vscatterpf1qps, X86Mem)                      //      AVX512_PF{k}
  ASMJIT_INST_2x(vscatterqpd, Vscatterqpd, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterqpd, Vscatterqpd, X86Mem, X86Ymm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterqpd, Vscatterqpd, X86Mem, X86Zmm)                    //      AVX512_F{k}
  ASMJIT_INST_2x(vscatterqps, Vscatterqps, X86Mem, X86Xmm)                    //      AVX512_F{k}-VL
  ASMJIT_INST_2x(vscatterqps, Vscatterqps, X86Mem, X86Ymm)                    //      AVX512_F{k}
  ASMJIT_INST_4i(vshuff32x4, Vshuff32x4, X86Ymm, X86Ymm, X86Ymm, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshuff32x4, Vshuff32x4, X86Ymm, X86Ymm, X86Mem, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshuff32x4, Vshuff32x4, X86Zmm, X86Zmm, X86Zmm, Imm)         //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vshuff32x4, Vshuff32x4, X86Zmm, X86Zmm, X86Mem, Imm)         //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vshuff64x2, Vshuff64x2, X86Ymm, X86Ymm, X86Ymm, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshuff64x2, Vshuff64x2, X86Ymm, X86Ymm, X86Mem, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshuff64x2, Vshuff64x2, X86Zmm, X86Zmm, X86Zmm, Imm)         //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vshuff64x2, Vshuff64x2, X86Zmm, X86Zmm, X86Mem, Imm)         //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vshufi32x4, Vshufi32x4, X86Ymm, X86Ymm, X86Ymm, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshufi32x4, Vshufi32x4, X86Ymm, X86Ymm, X86Mem, Imm)         //      AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshufi32x4, Vshufi32x4, X86Zmm, X86Zmm, X86Zmm, Imm)         //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vshufi32x4, Vshufi32x4, X86Zmm, X86Zmm, X86Mem, Imm)         //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vshufi64x2, Vshufi64x2, X86Ymm, X86Ymm, X86Ymm, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshufi64x2, Vshufi64x2, X86Ymm, X86Ymm, X86Mem, Imm)         //      AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshufi64x2, Vshufi64x2, X86Zmm, X86Zmm, X86Zmm, Imm)         //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vshufi64x2, Vshufi64x2, X86Zmm, X86Zmm, X86Mem, Imm)         //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vshufpd, Vshufpd, X86Xmm, X86Xmm, X86Xmm, Imm)               // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshufpd, Vshufpd, X86Xmm, X86Xmm, X86Mem, Imm)               // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshufpd, Vshufpd, X86Ymm, X86Ymm, X86Ymm, Imm)               // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshufpd, Vshufpd, X86Ymm, X86Ymm, X86Mem, Imm)               // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_4i(vshufpd, Vshufpd, X86Zmm, X86Zmm, X86Zmm, Imm)               //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vshufpd, Vshufpd, X86Zmm, X86Zmm, X86Mem, Imm)               //      AVX512_F{kz|b32}
  ASMJIT_INST_4i(vshufps, Vshufps, X86Xmm, X86Xmm, X86Xmm, Imm)               // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshufps, Vshufps, X86Xmm, X86Xmm, X86Mem, Imm)               // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshufps, Vshufps, X86Ymm, X86Ymm, X86Ymm, Imm)               // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshufps, Vshufps, X86Ymm, X86Ymm, X86Mem, Imm)               // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_4i(vshufps, Vshufps, X86Zmm, X86Zmm, X86Zmm, Imm)               //      AVX512_F{kz|b64}
  ASMJIT_INST_4i(vshufps, Vshufps, X86Zmm, X86Zmm, X86Mem, Imm)               //      AVX512_F{kz|b64}
  ASMJIT_INST_2x(vsqrtpd, Vsqrtpd, X86Xmm, X86Xmm)                            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vsqrtpd, Vsqrtpd, X86Xmm, X86Mem)                            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vsqrtpd, Vsqrtpd, X86Ymm, X86Ymm)                            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vsqrtpd, Vsqrtpd, X86Ymm, X86Mem)                            // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_2x(vsqrtpd, Vsqrtpd, X86Zmm, X86Zmm)                            //      AVX512_F{kz|er|b64}
  ASMJIT_INST_2x(vsqrtpd, Vsqrtpd, X86Zmm, X86Mem)                            //      AVX512_F{kz|er|b64}
  ASMJIT_INST_2x(vsqrtps, Vsqrtps, X86Xmm, X86Xmm)                            // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vsqrtps, Vsqrtps, X86Xmm, X86Mem)                            // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vsqrtps, Vsqrtps, X86Ymm, X86Ymm)                            // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vsqrtps, Vsqrtps, X86Ymm, X86Mem)                            // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_2x(vsqrtps, Vsqrtps, X86Zmm, X86Zmm)                            //      AVX512_F{kz|er|b32}
  ASMJIT_INST_2x(vsqrtps, Vsqrtps, X86Zmm, X86Mem)                            //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vsqrtsd, Vsqrtsd, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vsqrtsd, Vsqrtsd, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vsqrtss, Vsqrtss, X86Xmm, X86Xmm, X86Xmm)                    // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vsqrtss, Vsqrtss, X86Xmm, X86Xmm, X86Mem)                    // AVX  AVX512_F{kz|er}
  ASMJIT_INST_1x(vstmxcsr, Vstmxcsr, X86Mem)                                  // AVX
  ASMJIT_INST_3x(vsubpd, Vsubpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vsubpd, Vsubpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vsubpd, Vsubpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vsubpd, Vsubpd, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vsubpd, Vsubpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vsubpd, Vsubpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b64}
  ASMJIT_INST_3x(vsubps, Vsubps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vsubps, Vsubps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vsubps, Vsubps, X86Ymm, X86Ymm, X86Ymm)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vsubps, Vsubps, X86Ymm, X86Ymm, X86Mem)                      // AVX2 AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vsubps, Vsubps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vsubps, Vsubps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_F{kz|er|b32}
  ASMJIT_INST_3x(vsubsd, Vsubsd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vsubsd, Vsubsd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vsubss, Vsubss, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_3x(vsubss, Vsubss, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_F{kz|er}
  ASMJIT_INST_2x(vtestpd, Vtestpd, X86Xmm, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vtestpd, Vtestpd, X86Xmm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vtestpd, Vtestpd, X86Ymm, X86Ymm)                            // AVX
  ASMJIT_INST_2x(vtestpd, Vtestpd, X86Ymm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vtestps, Vtestps, X86Xmm, X86Xmm)                            // AVX
  ASMJIT_INST_2x(vtestps, Vtestps, X86Xmm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vtestps, Vtestps, X86Ymm, X86Ymm)                            // AVX
  ASMJIT_INST_2x(vtestps, Vtestps, X86Ymm, X86Mem)                            // AVX
  ASMJIT_INST_2x(vucomisd, Vucomisd, X86Xmm, X86Xmm)                          // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vucomisd, Vucomisd, X86Xmm, X86Mem)                          // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vucomiss, Vucomiss, X86Xmm, X86Xmm)                          // AVX  AVX512_F{sae}
  ASMJIT_INST_2x(vucomiss, Vucomiss, X86Xmm, X86Mem)                          // AVX  AVX512_F{sae}
  ASMJIT_INST_3x(vunpckhpd, Vunpckhpd, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpckhpd, Vunpckhpd, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpckhpd, Vunpckhpd, X86Ymm, X86Ymm, X86Ymm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpckhpd, Vunpckhpd, X86Ymm, X86Ymm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpckhpd, Vunpckhpd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vunpckhpd, Vunpckhpd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vunpckhps, Vunpckhps, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpckhps, Vunpckhps, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpckhps, Vunpckhps, X86Ymm, X86Ymm, X86Ymm)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpckhps, Vunpckhps, X86Ymm, X86Ymm, X86Mem)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpckhps, Vunpckhps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vunpckhps, Vunpckhps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vunpcklpd, Vunpcklpd, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpcklpd, Vunpcklpd, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpcklpd, Vunpcklpd, X86Ymm, X86Ymm, X86Ymm)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpcklpd, Vunpcklpd, X86Ymm, X86Ymm, X86Mem)                // AVX  AVX512_F{kz|b64}-VL
  ASMJIT_INST_3x(vunpcklpd, Vunpcklpd, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vunpcklpd, Vunpcklpd, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b64}
  ASMJIT_INST_3x(vunpcklps, Vunpcklps, X86Xmm, X86Xmm, X86Xmm)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpcklps, Vunpcklps, X86Xmm, X86Xmm, X86Mem)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpcklps, Vunpcklps, X86Ymm, X86Ymm, X86Ymm)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpcklps, Vunpcklps, X86Ymm, X86Ymm, X86Mem)                // AVX  AVX512_F{kz|b32}-VL
  ASMJIT_INST_3x(vunpcklps, Vunpcklps, X86Zmm, X86Zmm, X86Zmm)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vunpcklps, Vunpcklps, X86Zmm, X86Zmm, X86Mem)                //      AVX512_F{kz|b32}
  ASMJIT_INST_3x(vxorpd, Vxorpd, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vxorpd, Vxorpd, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vxorpd, Vxorpd, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vxorpd, Vxorpd, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_DQ{kz|b64}-VL
  ASMJIT_INST_3x(vxorpd, Vxorpd, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vxorpd, Vxorpd, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|b64}
  ASMJIT_INST_3x(vxorps, Vxorps, X86Xmm, X86Xmm, X86Xmm)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vxorps, Vxorps, X86Xmm, X86Xmm, X86Mem)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vxorps, Vxorps, X86Ymm, X86Ymm, X86Ymm)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vxorps, Vxorps, X86Ymm, X86Ymm, X86Mem)                      // AVX  AVX512_DQ{kz|b32}-VL
  ASMJIT_INST_3x(vxorps, Vxorps, X86Zmm, X86Zmm, X86Zmm)                      //      AVX512_DQ{kz|b32}
  ASMJIT_INST_3x(vxorps, Vxorps, X86Zmm, X86Zmm, X86Mem)                      //      AVX512_DQ{kz|b32}
  ASMJIT_INST_0x(vzeroall, Vzeroall)                                          // AVX
  ASMJIT_INST_0x(vzeroupper, Vzeroupper)                                      // AVX

  // --------------------------------------------------------------------------
  // [FMA4]
  // --------------------------------------------------------------------------

  ASMJIT_INST_4x(vfmaddpd, Vfmaddpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddpd, Vfmaddpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddpd, Vfmaddpd, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmaddpd, Vfmaddpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmaddpd, Vfmaddpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmaddpd, Vfmaddpd, X86Ymm, X86Ymm, X86Ymm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmaddps, Vfmaddps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddps, Vfmaddps, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddps, Vfmaddps, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmaddps, Vfmaddps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmaddps, Vfmaddps, X86Ymm, X86Ymm, X86Mem, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmaddps, Vfmaddps, X86Ymm, X86Ymm, X86Ymm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmaddsd, Vfmaddsd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddsd, Vfmaddsd, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddsd, Vfmaddsd, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmaddss, Vfmaddss, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddss, Vfmaddss, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmaddss, Vfmaddss, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmaddsubpd, Vfmaddsubpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubpd, Vfmaddsubpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubpd, Vfmaddsubpd, X86Xmm, X86Xmm, X86Xmm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmaddsubpd, Vfmaddsubpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubpd, Vfmaddsubpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubpd, Vfmaddsubpd, X86Ymm, X86Ymm, X86Ymm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmaddsubps, Vfmaddsubps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubps, Vfmaddsubps, X86Xmm, X86Xmm, X86Mem, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubps, Vfmaddsubps, X86Xmm, X86Xmm, X86Xmm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmaddsubps, Vfmaddsubps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubps, Vfmaddsubps, X86Ymm, X86Ymm, X86Mem, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmaddsubps, Vfmaddsubps, X86Ymm, X86Ymm, X86Ymm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmsubaddpd, Vfmsubaddpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddpd, Vfmsubaddpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddpd, Vfmsubaddpd, X86Xmm, X86Xmm, X86Xmm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmsubaddpd, Vfmsubaddpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddpd, Vfmsubaddpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddpd, Vfmsubaddpd, X86Ymm, X86Ymm, X86Ymm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmsubaddps, Vfmsubaddps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddps, Vfmsubaddps, X86Xmm, X86Xmm, X86Mem, X86Xmm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddps, Vfmsubaddps, X86Xmm, X86Xmm, X86Xmm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmsubaddps, Vfmsubaddps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddps, Vfmsubaddps, X86Ymm, X86Ymm, X86Mem, X86Ymm)    // FMA4
  ASMJIT_INST_4x(vfmsubaddps, Vfmsubaddps, X86Ymm, X86Ymm, X86Ymm, X86Mem)    // FMA4
  ASMJIT_INST_4x(vfmsubpd, Vfmsubpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubpd, Vfmsubpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubpd, Vfmsubpd, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmsubpd, Vfmsubpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmsubpd, Vfmsubpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmsubpd, Vfmsubpd, X86Ymm, X86Ymm, X86Ymm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmsubps, Vfmsubps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubps, Vfmsubps, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubps, Vfmsubps, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmsubps, Vfmsubps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmsubps, Vfmsubps, X86Ymm, X86Ymm, X86Mem, X86Ymm)          // FMA4
  ASMJIT_INST_4x(vfmsubps, Vfmsubps, X86Ymm, X86Ymm, X86Ymm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmsubsd, Vfmsubsd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubsd, Vfmsubsd, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubsd, Vfmsubsd, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfmsubss, Vfmsubss, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubss, Vfmsubss, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // FMA4
  ASMJIT_INST_4x(vfmsubss, Vfmsubss, X86Xmm, X86Xmm, X86Xmm, X86Mem)          // FMA4
  ASMJIT_INST_4x(vfnmaddpd, Vfnmaddpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddpd, Vfnmaddpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddpd, Vfnmaddpd, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmaddpd, Vfnmaddpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmaddpd, Vfnmaddpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmaddpd, Vfnmaddpd, X86Ymm, X86Ymm, X86Ymm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmaddps, Vfnmaddps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddps, Vfnmaddps, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddps, Vfnmaddps, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmaddps, Vfnmaddps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmaddps, Vfnmaddps, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmaddps, Vfnmaddps, X86Ymm, X86Ymm, X86Ymm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmaddsd, Vfnmaddsd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddsd, Vfnmaddsd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddsd, Vfnmaddsd, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmaddss, Vfnmaddss, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddss, Vfnmaddss, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmaddss, Vfnmaddss, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmsubpd, Vfnmsubpd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubpd, Vfnmsubpd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubpd, Vfnmsubpd, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmsubpd, Vfnmsubpd, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmsubpd, Vfnmsubpd, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmsubpd, Vfnmsubpd, X86Ymm, X86Ymm, X86Ymm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmsubps, Vfnmsubps, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubps, Vfnmsubps, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubps, Vfnmsubps, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmsubps, Vfnmsubps, X86Ymm, X86Ymm, X86Ymm, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmsubps, Vfnmsubps, X86Ymm, X86Ymm, X86Mem, X86Ymm)        // FMA4
  ASMJIT_INST_4x(vfnmsubps, Vfnmsubps, X86Ymm, X86Ymm, X86Ymm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmsubsd, Vfnmsubsd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubsd, Vfnmsubsd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubsd, Vfnmsubsd, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4
  ASMJIT_INST_4x(vfnmsubss, Vfnmsubss, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubss, Vfnmsubss, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // FMA4
  ASMJIT_INST_4x(vfnmsubss, Vfnmsubss, X86Xmm, X86Xmm, X86Xmm, X86Mem)        // FMA4

  // --------------------------------------------------------------------------
  // [XOP]
  // --------------------------------------------------------------------------

  ASMJIT_INST_2x(vfrczpd, Vfrczpd, X86Xmm, X86Xmm)                            // XOP
  ASMJIT_INST_2x(vfrczpd, Vfrczpd, X86Xmm, X86Mem)                            // XOP
  ASMJIT_INST_2x(vfrczpd, Vfrczpd, X86Ymm, X86Ymm)                            // XOP
  ASMJIT_INST_2x(vfrczpd, Vfrczpd, X86Ymm, X86Mem)                            // XOP
  ASMJIT_INST_2x(vfrczps, Vfrczps, X86Xmm, X86Xmm)                            // XOP
  ASMJIT_INST_2x(vfrczps, Vfrczps, X86Xmm, X86Mem)                            // XOP
  ASMJIT_INST_2x(vfrczps, Vfrczps, X86Ymm, X86Ymm)                            // XOP
  ASMJIT_INST_2x(vfrczps, Vfrczps, X86Ymm, X86Mem)                            // XOP
  ASMJIT_INST_2x(vfrczsd, Vfrczsd, X86Xmm, X86Xmm)                            // XOP
  ASMJIT_INST_2x(vfrczsd, Vfrczsd, X86Xmm, X86Mem)                            // XOP
  ASMJIT_INST_2x(vfrczss, Vfrczss, X86Xmm, X86Xmm)                            // XOP
  ASMJIT_INST_2x(vfrczss, Vfrczss, X86Xmm, X86Mem)                            // XOP
  ASMJIT_INST_4x(vpcmov, Vpcmov, X86Xmm, X86Xmm, X86Xmm, X86Xmm)              // XOP
  ASMJIT_INST_4x(vpcmov, Vpcmov, X86Xmm, X86Xmm, X86Mem, X86Xmm)              // XOP
  ASMJIT_INST_4x(vpcmov, Vpcmov, X86Xmm, X86Xmm, X86Xmm, X86Mem)              // XOP
  ASMJIT_INST_4x(vpcmov, Vpcmov, X86Ymm, X86Ymm, X86Ymm, X86Ymm)              // XOP
  ASMJIT_INST_4x(vpcmov, Vpcmov, X86Ymm, X86Ymm, X86Mem, X86Ymm)              // XOP
  ASMJIT_INST_4x(vpcmov, Vpcmov, X86Ymm, X86Ymm, X86Ymm, X86Mem)              // XOP
  ASMJIT_INST_4i(vpcomb, Vpcomb, X86Xmm, X86Xmm, X86Xmm, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomb, Vpcomb, X86Xmm, X86Xmm, X86Mem, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomd, Vpcomd, X86Xmm, X86Xmm, X86Xmm, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomd, Vpcomd, X86Xmm, X86Xmm, X86Mem, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomq, Vpcomq, X86Xmm, X86Xmm, X86Xmm, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomq, Vpcomq, X86Xmm, X86Xmm, X86Mem, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomw, Vpcomw, X86Xmm, X86Xmm, X86Xmm, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomw, Vpcomw, X86Xmm, X86Xmm, X86Mem, Imm)                 // XOP
  ASMJIT_INST_4i(vpcomub, Vpcomub, X86Xmm, X86Xmm, X86Xmm, Imm)               // XOP
  ASMJIT_INST_4i(vpcomub, Vpcomub, X86Xmm, X86Xmm, X86Mem, Imm)               // XOP
  ASMJIT_INST_4i(vpcomud, Vpcomud, X86Xmm, X86Xmm, X86Xmm, Imm)               // XOP
  ASMJIT_INST_4i(vpcomud, Vpcomud, X86Xmm, X86Xmm, X86Mem, Imm)               // XOP
  ASMJIT_INST_4i(vpcomuq, Vpcomuq, X86Xmm, X86Xmm, X86Xmm, Imm)               // XOP
  ASMJIT_INST_4i(vpcomuq, Vpcomuq, X86Xmm, X86Xmm, X86Mem, Imm)               // XOP
  ASMJIT_INST_4i(vpcomuw, Vpcomuw, X86Xmm, X86Xmm, X86Xmm, Imm)               // XOP
  ASMJIT_INST_4i(vpcomuw, Vpcomuw, X86Xmm, X86Xmm, X86Mem, Imm)               // XOP
  ASMJIT_INST_5i(vpermil2pd, Vpermil2pd, X86Xmm, X86Xmm, X86Xmm, X86Xmm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2pd, Vpermil2pd, X86Xmm, X86Xmm, X86Mem, X86Xmm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2pd, Vpermil2pd, X86Xmm, X86Xmm, X86Xmm, X86Mem, Imm) // XOP
  ASMJIT_INST_5i(vpermil2pd, Vpermil2pd, X86Ymm, X86Ymm, X86Ymm, X86Ymm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2pd, Vpermil2pd, X86Ymm, X86Ymm, X86Mem, X86Ymm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2pd, Vpermil2pd, X86Ymm, X86Ymm, X86Ymm, X86Mem, Imm) // XOP
  ASMJIT_INST_5i(vpermil2ps, Vpermil2ps, X86Xmm, X86Xmm, X86Xmm, X86Xmm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2ps, Vpermil2ps, X86Xmm, X86Xmm, X86Mem, X86Xmm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2ps, Vpermil2ps, X86Xmm, X86Xmm, X86Xmm, X86Mem, Imm) // XOP
  ASMJIT_INST_5i(vpermil2ps, Vpermil2ps, X86Ymm, X86Ymm, X86Ymm, X86Ymm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2ps, Vpermil2ps, X86Ymm, X86Ymm, X86Mem, X86Ymm, Imm) // XOP
  ASMJIT_INST_5i(vpermil2ps, Vpermil2ps, X86Ymm, X86Ymm, X86Ymm, X86Mem, Imm) // XOP
  ASMJIT_INST_2x(vphaddbd, Vphaddbd, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphaddbd, Vphaddbd, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphaddbq, Vphaddbq, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphaddbq, Vphaddbq, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphaddbw, Vphaddbw, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphaddbw, Vphaddbw, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphadddq, Vphadddq, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphadddq, Vphadddq, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphaddwd, Vphaddwd, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphaddwd, Vphaddwd, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphaddwq, Vphaddwq, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphaddwq, Vphaddwq, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphaddubd, Vphaddubd, X86Xmm, X86Xmm)                        // XOP
  ASMJIT_INST_2x(vphaddubd, Vphaddubd, X86Xmm, X86Mem)                        // XOP
  ASMJIT_INST_2x(vphaddubq, Vphaddubq, X86Xmm, X86Xmm)                        // XOP
  ASMJIT_INST_2x(vphaddubq, Vphaddubq, X86Xmm, X86Mem)                        // XOP
  ASMJIT_INST_2x(vphaddubw, Vphaddubw, X86Xmm, X86Xmm)                        // XOP
  ASMJIT_INST_2x(vphaddubw, Vphaddubw, X86Xmm, X86Mem)                        // XOP
  ASMJIT_INST_2x(vphaddudq, Vphaddudq, X86Xmm, X86Xmm)                        // XOP
  ASMJIT_INST_2x(vphaddudq, Vphaddudq, X86Xmm, X86Mem)                        // XOP
  ASMJIT_INST_2x(vphadduwd, Vphadduwd, X86Xmm, X86Xmm)                        // XOP
  ASMJIT_INST_2x(vphadduwd, Vphadduwd, X86Xmm, X86Mem)                        // XOP
  ASMJIT_INST_2x(vphadduwq, Vphadduwq, X86Xmm, X86Xmm)                        // XOP
  ASMJIT_INST_2x(vphadduwq, Vphadduwq, X86Xmm, X86Mem)                        // XOP
  ASMJIT_INST_2x(vphsubbw, Vphsubbw, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphsubbw, Vphsubbw, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphsubdq, Vphsubdq, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphsubdq, Vphsubdq, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_2x(vphsubwd, Vphsubwd, X86Xmm, X86Xmm)                          // XOP
  ASMJIT_INST_2x(vphsubwd, Vphsubwd, X86Xmm, X86Mem)                          // XOP
  ASMJIT_INST_4x(vpmacsdd, Vpmacsdd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // XOP
  ASMJIT_INST_4x(vpmacsdd, Vpmacsdd, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // XOP
  ASMJIT_INST_4x(vpmacsdqh, Vpmacsdqh, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacsdqh, Vpmacsdqh, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacsdql, Vpmacsdql, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacsdql, Vpmacsdql, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacswd, Vpmacswd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // XOP
  ASMJIT_INST_4x(vpmacswd, Vpmacswd, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // XOP
  ASMJIT_INST_4x(vpmacsww, Vpmacsww, X86Xmm, X86Xmm, X86Xmm, X86Xmm)          // XOP
  ASMJIT_INST_4x(vpmacsww, Vpmacsww, X86Xmm, X86Xmm, X86Mem, X86Xmm)          // XOP
  ASMJIT_INST_4x(vpmacssdd, Vpmacssdd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacssdd, Vpmacssdd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacssdqh, Vpmacssdqh, X86Xmm, X86Xmm, X86Xmm, X86Xmm)      // XOP
  ASMJIT_INST_4x(vpmacssdqh, Vpmacssdqh, X86Xmm, X86Xmm, X86Mem, X86Xmm)      // XOP
  ASMJIT_INST_4x(vpmacssdql, Vpmacssdql, X86Xmm, X86Xmm, X86Xmm, X86Xmm)      // XOP
  ASMJIT_INST_4x(vpmacssdql, Vpmacssdql, X86Xmm, X86Xmm, X86Mem, X86Xmm)      // XOP
  ASMJIT_INST_4x(vpmacsswd, Vpmacsswd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacsswd, Vpmacsswd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacssww, Vpmacssww, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmacssww, Vpmacssww, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmadcsswd, Vpmadcsswd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)      // XOP
  ASMJIT_INST_4x(vpmadcsswd, Vpmadcsswd, X86Xmm, X86Xmm, X86Mem, X86Xmm)      // XOP
  ASMJIT_INST_4x(vpmadcswd, Vpmadcswd, X86Xmm, X86Xmm, X86Xmm, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpmadcswd, Vpmadcswd, X86Xmm, X86Xmm, X86Mem, X86Xmm)        // XOP
  ASMJIT_INST_4x(vpperm, Vpperm, X86Xmm, X86Xmm, X86Xmm, X86Xmm)              // XOP
  ASMJIT_INST_4x(vpperm, Vpperm, X86Xmm, X86Xmm, X86Mem, X86Xmm)              // XOP
  ASMJIT_INST_4x(vpperm, Vpperm, X86Xmm, X86Xmm, X86Xmm, X86Mem)              // XOP
  ASMJIT_INST_3x(vprotb, Vprotb, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotb, Vprotb, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotb, Vprotb, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3i(vprotb, Vprotb, X86Xmm, X86Xmm, Imm)                         // XOP
  ASMJIT_INST_3i(vprotb, Vprotb, X86Xmm, X86Mem, Imm)                         // XOP
  ASMJIT_INST_3x(vprotd, Vprotd, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotd, Vprotd, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotd, Vprotd, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3i(vprotd, Vprotd, X86Xmm, X86Xmm, Imm)                         // XOP
  ASMJIT_INST_3i(vprotd, Vprotd, X86Xmm, X86Mem, Imm)                         // XOP
  ASMJIT_INST_3x(vprotq, Vprotq, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotq, Vprotq, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotq, Vprotq, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3i(vprotq, Vprotq, X86Xmm, X86Xmm, Imm)                         // XOP
  ASMJIT_INST_3i(vprotq, Vprotq, X86Xmm, X86Mem, Imm)                         // XOP
  ASMJIT_INST_3x(vprotw, Vprotw, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotw, Vprotw, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vprotw, Vprotw, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3i(vprotw, Vprotw, X86Xmm, X86Xmm, Imm)                         // XOP
  ASMJIT_INST_3i(vprotw, Vprotw, X86Xmm, X86Mem, Imm)                         // XOP
  ASMJIT_INST_3x(vpshab, Vpshab, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshab, Vpshab, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshab, Vpshab, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshad, Vpshad, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshad, Vpshad, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshad, Vpshad, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshaq, Vpshaq, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshaq, Vpshaq, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshaq, Vpshaq, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshaw, Vpshaw, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshaw, Vpshaw, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshaw, Vpshaw, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshlb, Vpshlb, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshlb, Vpshlb, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshlb, Vpshlb, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshld, Vpshld, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshld, Vpshld, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshld, Vpshld, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshlq, Vpshlq, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshlq, Vpshlq, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshlq, Vpshlq, X86Xmm, X86Xmm, X86Mem)                      // XOP
  ASMJIT_INST_3x(vpshlw, Vpshlw, X86Xmm, X86Xmm, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshlw, Vpshlw, X86Xmm, X86Mem, X86Xmm)                      // XOP
  ASMJIT_INST_3x(vpshlw, Vpshlw, X86Xmm, X86Xmm, X86Mem)                      // XOP
};

// ============================================================================
// [asmjit::X86EmitterImplicitT]
// ============================================================================

template<typename This>
struct X86EmitterImplicitT : public X86EmitterExplicitT<This> {
  // --------------------------------------------------------------------------
  // [Options]
  // --------------------------------------------------------------------------

  //! Use REP/REPZ prefix.
  ASMJIT_INLINE This& rep() noexcept { return X86EmitterExplicitT<This>::_addOptions(X86Inst::kOptionRep); }
  //! Use REPNZ prefix.
  ASMJIT_INLINE This& repnz() noexcept { return X86EmitterExplicitT<This>::_addOptions(X86Inst::kOptionRepnz); }

  //! Use REP/REPZ prefix.
  ASMJIT_INLINE This& repe() noexcept { return rep(); }
  //! Use REP/REPZ prefix.
  ASMJIT_INLINE This& repz() noexcept { return rep(); }
  //! Use REPNZ prefix.
  ASMJIT_INLINE This& repne() noexcept { return repnz(); }

  // --------------------------------------------------------------------------
  // [General Purpose and Non-SIMD Instructions]
  // --------------------------------------------------------------------------

  // TODO: xrstor and xsave don't have explicit variants yet.

  using X86EmitterExplicitT<This>::cbw;
  using X86EmitterExplicitT<This>::cdq;
  using X86EmitterExplicitT<This>::cdqe;
  using X86EmitterExplicitT<This>::clzero;
  using X86EmitterExplicitT<This>::cqo;
  using X86EmitterExplicitT<This>::cwd;
  using X86EmitterExplicitT<This>::cwde;
  using X86EmitterExplicitT<This>::cmpsd;
  using X86EmitterExplicitT<This>::cmpxchg;
  using X86EmitterExplicitT<This>::cmpxchg8b;
  using X86EmitterExplicitT<This>::cmpxchg16b;
  using X86EmitterExplicitT<This>::cpuid;
  using X86EmitterExplicitT<This>::div;
  using X86EmitterExplicitT<This>::idiv;
  using X86EmitterExplicitT<This>::imul;
  using X86EmitterExplicitT<This>::jecxz;
  using X86EmitterExplicitT<This>::lahf;
  using X86EmitterExplicitT<This>::mulx;
  using X86EmitterExplicitT<This>::movsd;
  using X86EmitterExplicitT<This>::mul;
  using X86EmitterExplicitT<This>::rdmsr;
  using X86EmitterExplicitT<This>::rdpmc;
  using X86EmitterExplicitT<This>::rdtsc;
  using X86EmitterExplicitT<This>::rdtscp;
  using X86EmitterExplicitT<This>::sahf;
  using X86EmitterExplicitT<This>::wrmsr;
  using X86EmitterExplicitT<This>::xgetbv;
  using X86EmitterExplicitT<This>::xsetbv;

  ASMJIT_INST_0x(cbw, Cbw)                                                    // ANY       [IMPLICIT] AX      <- Sign Extend AL
  ASMJIT_INST_0x(cdq, Cdq)                                                    // ANY       [IMPLICIT] EDX:EAX <- Sign Extend EAX
  ASMJIT_INST_0x(cdqe, Cdqe)                                                  // X64       [IMPLICIT] RAX     <- Sign Extend EAX
  ASMJIT_INST_0x(clzero, Clzero)                                              // CLZERO    [IMPLICIT]
  ASMJIT_INST_2x(cmpxchg, Cmpxchg, X86Gp, X86Gp)                              // I486      [IMPLICIT]
  ASMJIT_INST_2x(cmpxchg, Cmpxchg, X86Mem, X86Gp)                             // I486      [IMPLICIT]
  ASMJIT_INST_1x(cmpxchg16b, Cmpxchg16b, X86Mem)                              // CMPXCHG8B [IMPLICIT] m == RDX:RAX ? m <- RCX:RBX
  ASMJIT_INST_1x(cmpxchg8b, Cmpxchg8b, X86Mem)                                // CMPXCHG16B[IMPLICIT] m == EDX:EAX ? m <- ECX:EBX
  ASMJIT_INST_0x(cpuid, Cpuid)                                                // I486      [IMPLICIT] EAX:EBX:ECX:EDX  <- CPUID[EAX:ECX]
  ASMJIT_INST_0x(cqo, Cqo)                                                    // X64       [IMPLICIT] RDX:RAX <- Sign Extend RAX
  ASMJIT_INST_0x(cwd, Cwd)                                                    // ANY       [IMPLICIT] DX:AX   <- Sign Extend AX
  ASMJIT_INST_0x(cwde, Cwde)                                                  // ANY       [IMPLICIT] EAX     <- Sign Extend AX
  ASMJIT_INST_0x(daa, Daa)
  ASMJIT_INST_0x(das, Das)
  ASMJIT_INST_1x(div, Div, X86Gp)                                             // ANY       [IMPLICIT] {AH[Rem]: AL[Quot] <- AX / r8} {xDX[Rem]:xAX[Quot] <- DX:AX / r16|r32|r64}
  ASMJIT_INST_1x(div, Div, X86Mem)                                            // ANY       [IMPLICIT] {AH[Rem]: AL[Quot] <- AX / m8} {xDX[Rem]:xAX[Quot] <- DX:AX / m16|m32|m64}
  ASMJIT_INST_1x(idiv, Idiv, X86Gp)                                           // ANY       [IMPLICIT] {AH[Rem]: AL[Quot] <- AX / r8} {xDX[Rem]:xAX[Quot] <- DX:AX / r16|r32|r64}
  ASMJIT_INST_1x(idiv, Idiv, X86Mem)                                          // ANY       [IMPLICIT] {AH[Rem]: AL[Quot] <- AX / m8} {xDX[Rem]:xAX[Quot] <- DX:AX / m16|m32|m64}
  ASMJIT_INST_1x(imul, Imul, X86Gp)                                           // ANY       [IMPLICIT] {AX <- AL * r8} {xAX:xDX <- xAX * r16|r32|r64}
  ASMJIT_INST_1x(imul, Imul, X86Mem)                                          // ANY       [IMPLICIT] {AX <- AL * m8} {xAX:xDX <- xAX * m16|m32|m64}
  ASMJIT_INST_0x(iret, Iret)                                                  // ANY       [IMPLICIT]
  ASMJIT_INST_0x(iretd, Iretd)                                                // ANY       [IMPLICIT]
  ASMJIT_INST_0x(iretq, Iretq)                                                // X64       [IMPLICIT]
  ASMJIT_INST_0x(iretw, Iretw)                                                // ANY       [IMPLICIT]
  ASMJIT_INST_1x(jecxz, Jecxz, Label)                                         // ANY       [IMPLICIT] Short jump if CX/ECX/RCX is zero.
  ASMJIT_INST_1x(jecxz, Jecxz, Imm)                                           // ANY       [IMPLICIT] Short jump if CX/ECX/RCX is zero.
  ASMJIT_INST_1x(jecxz, Jecxz, uint64_t)                                      // ANY       [IMPLICIT] Short jump if CX/ECX/RCX is zero.
  ASMJIT_INST_0x(lahf, Lahf)                                                  // LAHFSAHF  [IMPLICIT] AH <- EFL
  ASMJIT_INST_1x(loop, Loop, Label)                                           // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0.
  ASMJIT_INST_1x(loop, Loop, Imm)                                             // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0.
  ASMJIT_INST_1x(loop, Loop, uint64_t)                                        // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0.
  ASMJIT_INST_1x(loope, Loope, Label)                                         // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 1.
  ASMJIT_INST_1x(loope, Loope, Imm)                                           // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 1.
  ASMJIT_INST_1x(loope, Loope, uint64_t)                                      // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 1.
  ASMJIT_INST_1x(loopne, Loopne, Label)                                       // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 0.
  ASMJIT_INST_1x(loopne, Loopne, Imm)                                         // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 0.
  ASMJIT_INST_1x(loopne, Loopne, uint64_t)                                    // ANY       [IMPLICIT] Decrement xCX; short jump if xCX != 0 && ZF == 0.
  ASMJIT_INST_1x(mul, Mul, X86Gp)                                             // ANY       [IMPLICIT] {AX <- AL * r8} {xDX:xAX <- xAX * r16|r32|r64}
  ASMJIT_INST_1x(mul, Mul, X86Mem)                                            // ANY       [IMPLICIT] {AX <- AL * m8} {xDX:xAX <- xAX * m16|m32|m64}
  ASMJIT_INST_3x(mulx, Mulx, X86Gp, X86Gp, X86Gp)                             // BMI2      [IMPLICIT]
  ASMJIT_INST_3x(mulx, Mulx, X86Gp, X86Gp, X86Mem)                            // BMI2      [IMPLICIT]
  ASMJIT_INST_0x(rdmsr, Rdmsr)                                                // ANY       [IMPLICIT]
  ASMJIT_INST_0x(rdpmc, Rdpmc)                                                // ANY       [IMPLICIT]
  ASMJIT_INST_0x(rdtsc, Rdtsc)                                                // RDTSC     [IMPLICIT] EDX:EAX <- CNT
  ASMJIT_INST_0x(rdtscp, Rdtscp)                                              // RDTSCP    [IMPLICIT] EDX:EAX:EXC <- CNT
  ASMJIT_INST_0x(ret, Ret)
  ASMJIT_INST_1i(ret, Ret, Imm)
  ASMJIT_INST_0x(sahf, Sahf)                                                  // LAHFSAHF  [IMPLICIT] EFL <- AH
  ASMJIT_INST_0x(syscall, Syscall)                                            // X64       [IMPLICIT]
  ASMJIT_INST_0x(sysenter, Sysenter)                                          // X64       [IMPLICIT]
  ASMJIT_INST_0x(sysexit, Sysexit)                                            // X64       [IMPLICIT]
  ASMJIT_INST_0x(sysexit64, Sysexit64)                                        // X64       [IMPLICIT]
  ASMJIT_INST_0x(sysret, Sysret)                                              // X64       [IMPLICIT]
  ASMJIT_INST_0x(sysret64, Sysret64)                                          // X64       [IMPLICIT]
  ASMJIT_INST_0x(wrmsr, Wrmsr)                                                // ANY       [IMPLICIT]
  ASMJIT_INST_0x(xgetbv, Xgetbv)                                              // XSAVE     [IMPLICIT] EDX:EAX <- XCR[ECX]
  ASMJIT_INST_0x(xlatb, Xlatb)                                                // ANY       [IMPLICIT]
  ASMJIT_INST_1x(xrstor, Xrstor, X86Mem)                                      // XSAVE     [IMPLICIT]
  ASMJIT_INST_1x(xrstor64, Xrstor64, X86Mem)                                  // XSAVE+X64 [IMPLICIT]
  ASMJIT_INST_1x(xrstors, Xrstors, X86Mem)                                    // XSAVE     [IMPLICIT]
  ASMJIT_INST_1x(xrstors64, Xrstors64, X86Mem)                                // XSAVE+X64 [IMPLICIT]
  ASMJIT_INST_1x(xsave, Xsave, X86Mem)                                        // XSAVE     [IMPLICIT]
  ASMJIT_INST_1x(xsave64, Xsave64, X86Mem)                                    // XSAVE+X64 [IMPLICIT]
  ASMJIT_INST_1x(xsavec, Xsavec, X86Mem)                                      // XSAVE     [IMPLICIT]
  ASMJIT_INST_1x(xsavec64, Xsavec64, X86Mem)                                  // XSAVE+X64 [IMPLICIT]
  ASMJIT_INST_1x(xsaveopt, Xsaveopt, X86Mem)                                  // XSAVE     [IMPLICIT]
  ASMJIT_INST_1x(xsaveopt64, Xsaveopt64, X86Mem)                              // XSAVE+X64 [IMPLICIT]
  ASMJIT_INST_1x(xsaves, Xsaves, X86Mem)                                      // XSAVE     [IMPLICIT]
  ASMJIT_INST_1x(xsaves64, Xsaves64, X86Mem)                                  // XSAVE+X64 [IMPLICIT]
  ASMJIT_INST_0x(xsetbv, Xsetbv)                                              // XSAVE     [IMPLICIT] XCR[ECX] <- EDX:EAX

  // String instructions aliases.
  ASMJIT_INLINE Error cmpsb() { return ASMJIT_EMIT(X86Inst::kIdCmps, X86EmitterExplicitT<This>::ptr_zsi(0, 1), X86EmitterExplicitT<This>::ptr_zdi(0, 1)); }
  ASMJIT_INLINE Error cmpsd() { return ASMJIT_EMIT(X86Inst::kIdCmps, X86EmitterExplicitT<This>::ptr_zsi(0, 4), X86EmitterExplicitT<This>::ptr_zdi(0, 4)); }
  ASMJIT_INLINE Error cmpsq() { return ASMJIT_EMIT(X86Inst::kIdCmps, X86EmitterExplicitT<This>::ptr_zsi(0, 8), X86EmitterExplicitT<This>::ptr_zdi(0, 8)); }
  ASMJIT_INLINE Error cmpsw() { return ASMJIT_EMIT(X86Inst::kIdCmps, X86EmitterExplicitT<This>::ptr_zsi(0, 2), X86EmitterExplicitT<This>::ptr_zdi(0, 2)); }

  ASMJIT_INLINE Error lodsb() { return ASMJIT_EMIT(X86Inst::kIdLods, x86::al , X86EmitterExplicitT<This>::ptr_zdi(0, 1)); }
  ASMJIT_INLINE Error lodsd() { return ASMJIT_EMIT(X86Inst::kIdLods, x86::eax, X86EmitterExplicitT<This>::ptr_zdi(0, 4)); }
  ASMJIT_INLINE Error lodsq() { return ASMJIT_EMIT(X86Inst::kIdLods, x86::rax, X86EmitterExplicitT<This>::ptr_zdi(0, 8)); }
  ASMJIT_INLINE Error lodsw() { return ASMJIT_EMIT(X86Inst::kIdLods, x86::ax , X86EmitterExplicitT<This>::ptr_zdi(0, 2)); }

  ASMJIT_INLINE Error movsb() { return ASMJIT_EMIT(X86Inst::kIdMovs, X86EmitterExplicitT<This>::ptr_zdi(0, 1), X86EmitterExplicitT<This>::ptr_zsi(0, 1)); }
  ASMJIT_INLINE Error movsd() { return ASMJIT_EMIT(X86Inst::kIdMovs, X86EmitterExplicitT<This>::ptr_zdi(0, 4), X86EmitterExplicitT<This>::ptr_zsi(0, 4)); }
  ASMJIT_INLINE Error movsq() { return ASMJIT_EMIT(X86Inst::kIdMovs, X86EmitterExplicitT<This>::ptr_zdi(0, 8), X86EmitterExplicitT<This>::ptr_zsi(0, 8)); }
  ASMJIT_INLINE Error movsw() { return ASMJIT_EMIT(X86Inst::kIdMovs, X86EmitterExplicitT<This>::ptr_zdi(0, 2), X86EmitterExplicitT<This>::ptr_zsi(0, 2)); }

  ASMJIT_INLINE Error scasb() { return ASMJIT_EMIT(X86Inst::kIdScas, x86::al , X86EmitterExplicitT<This>::ptr_zdi(0, 1)); }
  ASMJIT_INLINE Error scasd() { return ASMJIT_EMIT(X86Inst::kIdScas, x86::eax, X86EmitterExplicitT<This>::ptr_zdi(0, 4)); }
  ASMJIT_INLINE Error scasq() { return ASMJIT_EMIT(X86Inst::kIdScas, x86::rax, X86EmitterExplicitT<This>::ptr_zdi(0, 8)); }
  ASMJIT_INLINE Error scasw() { return ASMJIT_EMIT(X86Inst::kIdScas, x86::ax , X86EmitterExplicitT<This>::ptr_zdi(0, 2)); }

  ASMJIT_INLINE Error stosb() { return ASMJIT_EMIT(X86Inst::kIdStos, X86EmitterExplicitT<This>::ptr_zdi(0, 1), x86::al ); }
  ASMJIT_INLINE Error stosd() { return ASMJIT_EMIT(X86Inst::kIdStos, X86EmitterExplicitT<This>::ptr_zdi(0, 4), x86::eax); }
  ASMJIT_INLINE Error stosq() { return ASMJIT_EMIT(X86Inst::kIdStos, X86EmitterExplicitT<This>::ptr_zdi(0, 8), x86::rax); }
  ASMJIT_INLINE Error stosw() { return ASMJIT_EMIT(X86Inst::kIdStos, X86EmitterExplicitT<This>::ptr_zdi(0, 2), x86::ax ); }

  // --------------------------------------------------------------------------
  // [MONITOR|MWAIT]
  // --------------------------------------------------------------------------

  ASMJIT_INST_0x(monitor, Monitor)
  ASMJIT_INST_0x(mwait, Mwait)

  // --------------------------------------------------------------------------
  // [MMX & SSE Instructions]
  // --------------------------------------------------------------------------

  using X86EmitterExplicitT<This>::blendvpd;
  using X86EmitterExplicitT<This>::blendvps;
  using X86EmitterExplicitT<This>::maskmovq;
  using X86EmitterExplicitT<This>::maskmovdqu;
  using X86EmitterExplicitT<This>::pblendvb;
  using X86EmitterExplicitT<This>::pcmpestri;
  using X86EmitterExplicitT<This>::pcmpestrm;
  using X86EmitterExplicitT<This>::pcmpistri;
  using X86EmitterExplicitT<This>::pcmpistrm;

  ASMJIT_INST_2x(blendvpd, Blendvpd, X86Xmm, X86Xmm)                          // SSE4_1 [IMPLICIT]
  ASMJIT_INST_2x(blendvpd, Blendvpd, X86Xmm, X86Mem)                          // SSE4_1 [IMPLICIT]
  ASMJIT_INST_2x(blendvps, Blendvps, X86Xmm, X86Xmm)                          // SSE4_1 [IMPLICIT]
  ASMJIT_INST_2x(blendvps, Blendvps, X86Xmm, X86Mem)                          // SSE4_1 [IMPLICIT]
  ASMJIT_INST_2x(pblendvb, Pblendvb, X86Xmm, X86Xmm)                          // SSE4_1 [IMPLICIT]
  ASMJIT_INST_2x(pblendvb, Pblendvb, X86Xmm, X86Mem)                          // SSE4_1 [IMPLICIT]
  ASMJIT_INST_2x(maskmovq, Maskmovq, X86Mm, X86Mm)                            // SSE    [IMPLICIT]
  ASMJIT_INST_2x(maskmovdqu, Maskmovdqu, X86Xmm, X86Xmm)                      // SSE2   [IMPLICIT]
  ASMJIT_INST_3i(pcmpestri, Pcmpestri, X86Xmm, X86Xmm, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpestri, Pcmpestri, X86Xmm, X86Mem, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpestrm, Pcmpestrm, X86Xmm, X86Xmm, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpestrm, Pcmpestrm, X86Xmm, X86Mem, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpistri, Pcmpistri, X86Xmm, X86Xmm, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpistri, Pcmpistri, X86Xmm, X86Mem, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpistrm, Pcmpistrm, X86Xmm, X86Xmm, Imm)                   // SSE4_1 [IMPLICIT]
  ASMJIT_INST_3i(pcmpistrm, Pcmpistrm, X86Xmm, X86Mem, Imm)                   // SSE4_1 [IMPLICIT]

  // --------------------------------------------------------------------------
  // [SHA]
  // --------------------------------------------------------------------------

  using X86EmitterExplicitT<This>::sha256rnds2;

  ASMJIT_INST_2x(sha256rnds2, Sha256rnds2, X86Xmm, X86Xmm)                    // SHA [IMPLICIT]
  ASMJIT_INST_2x(sha256rnds2, Sha256rnds2, X86Xmm, X86Mem)                    // SHA [IMPLICIT]

  // --------------------------------------------------------------------------
  // [AVX...AVX512]
  // --------------------------------------------------------------------------

  using X86EmitterExplicitT<This>::vmaskmovdqu;
  using X86EmitterExplicitT<This>::vpcmpestri;
  using X86EmitterExplicitT<This>::vpcmpestrm;
  using X86EmitterExplicitT<This>::vpcmpistri;
  using X86EmitterExplicitT<This>::vpcmpistrm;

  ASMJIT_INST_2x(vmaskmovdqu, Vmaskmovdqu, X86Xmm, X86Xmm)                    // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpestri, Vpcmpestri, X86Xmm, X86Xmm, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpestri, Vpcmpestri, X86Xmm, X86Mem, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpestrm, Vpcmpestrm, X86Xmm, X86Xmm, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpestrm, Vpcmpestrm, X86Xmm, X86Mem, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpistri, Vpcmpistri, X86Xmm, X86Xmm, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpistri, Vpcmpistri, X86Xmm, X86Mem, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpistrm, Vpcmpistrm, X86Xmm, X86Xmm, Imm)                 // AVX  [IMPLICIT]
  ASMJIT_INST_3i(vpcmpistrm, Vpcmpistrm, X86Xmm, X86Mem, Imm)                 // AVX  [IMPLICIT]
};

#undef ASMJIT_INST_0x
#undef ASMJIT_INST_1x
#undef ASMJIT_INST_1i
#undef ASMJIT_INST_1c
#undef ASMJIT_INST_2x
#undef ASMJIT_INST_2i
#undef ASMJIT_INST_2c
#undef ASMJIT_INST_3x
#undef ASMJIT_INST_3i
#undef ASMJIT_INST_3ii
#undef ASMJIT_INST_4x
#undef ASMJIT_INST_4i
#undef ASMJIT_INST_4ii
#undef ASMJIT_INST_5x
#undef ASMJIT_INST_5i
#undef ASMJIT_INST_6x
#undef ASMJIT_EMIT

// ============================================================================
// [asmjit::X86Emitter]
// ============================================================================

//! X86/X64 emitter.
//!
//! NOTE: This class cannot be created, you can only cast to it and use it as
//! emitter that emits to either X86Assembler, X86Builder, or X86Compiler (use
//! with caution with X86Compiler as it expects virtual registers to be used).
class X86Emitter : public CodeEmitter, public X86EmitterImplicitT<X86Emitter> {
  ASMJIT_NONCONSTRUCTIBLE(X86Emitter)
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86EMITTER_H
