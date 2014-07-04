// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_ASMJIT_H
#define _ASMJIT_ASMJIT_H

// ============================================================================
// [asmjit_mainpage]
// ============================================================================

//! @mainpage
//!
//! AsmJit - Complete x86/x64 JIT and Remote Assembler for C++.
//!
//! AsmJit is a complete JIT and remote assembler for C++ language. It can
//! generate native code for x86 and x64 architectures having support for
//! a full instruction set, from legacy MMX to the newest AVX2. It has a
//! type-safe API that allows C++ compiler to do a semantic checks at
//! compile-time even before the assembled code is generated or run.
//!
//! AsmJit is not a virtual machine (VM). It doesn't have functionality to
//! implement VM out of the box; however, it can be be used as a JIT backend
//! for your own VM. The usage of AsmJit is not limited at all; it's suitable
//! for multimedia, VM backends or remote code generation.
//!
//! @section AsmJit_Concepts Code Generation Concepts
//!
//! AsmJit has two completely different code generation concepts. The difference
//! is in how the code is generated. The first concept, also referred as the low
//! level concept, is called 'Assembler' and it's the same as writing RAW
//! assembly by using physical registers directly. In this case AsmJit does only
//! instruction encoding, verification and relocation.
//!
//! The second concept, also referred as the high level concept, is called
//! 'Compiler'. Compiler lets you use virtually unlimited number of registers
//! (called variables) significantly simplifying the code generation process.
//! Compiler allocates these virtual registers to physical registers after the
//! code generation is done. This requires some extra effort - Compiler has to
//! generate information for each node (instruction, function declaration,
//! function call) in the code, perform a variable liveness analysis and
//! translate the code having variables into code having only registers.
//!
//! In addition, Compiler understands functions and function calling conventions.
//! It has been designed in a way that the code generated is always a function
//! having prototype like in a programming language. By having a function
//! prototype the Compiler is able to insert prolog and epilog to a function
//! being generated and it is able to call a function inside a generated one.
//!
//! There is no conclusion on which concept is better. Assembler brings full
//! control on how the code is generated, while Compiler makes the generation
//! more portable.
//!
//! @section AsmJit_Main_CodeGeneration Code Generation
//!
//! - \ref asmjit_base_general "Assembler core" - Operands, intrinsics and low-level assembler.
//! - \ref asmjit_compiler "Compiler" - High level code generation.
//! - \ref asmjit_cpuinfo "Cpu Information" - Get information about host processor.
//! - \ref asmjit_logging "Logging" - Logging and error handling.
//! - \ref AsmJit_MemoryManagement "Memory Management" - Virtual memory management.
//!
//! @section AsmJit_Main_HomePage AsmJit Homepage
//!
//! - https://github.com/kobalicekp/asmjit

// ============================================================================
// [asmjit_base]
// ============================================================================

//! \defgroup asmjit_base AsmJit
//!
//! \brief AsmJit.

// ============================================================================
// [asmjit_base_general]
// ============================================================================

//! \defgroup asmjit_base_general AsmJit General API
//! \ingroup asmjit_base
//!
//! \brief AsmJit general API.
//!
//! Contains all `asmjit` classes and helper functions that are architecture
//! independent or abstract. Abstract classes are implemented by the backend,
//! for example `BaseAssembler` is implemented by `x86x64::X86X64Assembler`.
//!
//! - See `BaseAssembler` for low level code generation documentation.
//! - See `BaseCompiler` for high level code generation documentation.
//! - See `Operand` for operand's overview.
//!
//! Logging and Error Handling
//! --------------------------
//!
//! AsmJit contains robust interface that can be used to log the generated code
//! and to handle possible errors. Base logging interface is defined in `Logger`
//! class that is abstract and can be overridden. AsmJit contains two loggers
//! that can be used out of the box - `FileLogger` that logs into a pure C
//! `FILE*` stream and `StringLogger` that just concatenates all log messages
//! by using a `StringBuilder` class.
//!
//! The following snippet shows how to setup a logger that logs to `stderr`:
//!
//! ~~~
//! // `FileLogger` instance.
//! FileLogger logger(stderr);
//!
//! // `Compiler` or any other `CodeGen` interface.
//! host::Compiler c;
//!
//! // use `setLogger` to replace the `CodeGen` logger.
//! c.setLogger(&logger);
//! ~~~
//!
//! \sa \ref Logger, \ref FileLogger, \ref StringLogger.

// ============================================================================
// [asmjit_base_tree]
// ============================================================================

//! \defgroup asmjit_base_tree AsmJit Code-Tree
//! \ingroup asmjit_base
//!
//! \brief AsmJit code-tree used by Compiler.
//!
//! AsmJit intermediate code-tree is a double-linked list that is made of nodes
//! that represent assembler instructions, directives, labels and high-level
//! constructs compiler is using to represent functions and function calls. The
//! node list can only be used together with \ref BaseCompiler.
//!
//! TODO

// ============================================================================
// [asmjit_base_util]
// ============================================================================

//! \defgroup asmjit_base_util AsmJit Utilities
//! \ingroup asmjit_base
//!
//! \brief AsmJit utility classes.
//!
//! AsmJit contains numerous utility classes that are needed by the library
//! itself. The most useful ones have been made public and are now exported.
//!
//! POD Containers
//! --------------
//!
//! POD containers are used by AsmJit to manage its own data structures. The
//! following classes can be used by AsmJit consumers:
//!
//!   - \ref PodVector - Simple growing array-like container for POD data.
//!   - \ref StringBuilder - Simple string builder that can append string
//!     and integers.
//!
//! Zone Memory Allocator
//! ---------------------
//!
//! Zone memory allocator is an incremental memory allocator that can be used
//! to allocate data of short life-time. It has much better performance
//! characteristics than all other allocators, because the only thing it can do
//! is to increment a pointer and return its previous address. See \ref Zone
//! for more details.
//!
//! CPU Ticks
//! ---------
//!
//! CPU Ticks is a simple helper that can be used to do basic benchmarks. See
//! \ref CpuTicks class for more details.
//!
//! Integer Utilities
//! -----------------
//!
//! Integer utilities are all implemented by a static class \ref IntUtil.
//! There are utilities for bit manipulation and bit counting, utilities to get
//! an integer minimum / maximum and various other helpers required to perform
//! alignment checks and binary casting from float to integer and vica versa->
//!
//! Vector Utilities
//! ----------------
//!
//! SIMD code generation often requires to embed constants after each function
//! or a block of functions generated. AsmJit contains classes `Vec64Data`,
//! `Vec128Data` and `Vec256Data` that can be used to prepare data useful when
//! generating SIMD code.
//!
//! X86/X64 code generator contains member functions `dmm`, `dxmm` and `dymm`
//! which can be used to embed 64-bit, 128-bit and 256-bit data structures into
//! machine code (both assembler and compiler are supported).
//!
//! \note Compiler contains a constant pool, which should be used instead of
//! embedding constants manually after the function body.

// ============================================================================
// [asmjit_x86x64]
// ============================================================================

//! \defgroup asmjit_x86x64 X86/X64
//!
//! \brief X86/X64 module

// ============================================================================
// [asmjit_x86x64_general]
// ============================================================================

//! \defgroup asmjit_x86x64_general X86/X64 General API
//! \ingroup asmjit_x86x64
//!
//! \brief X86/X64 general API.
//!
//! X86/X64 Registers
//! -----------------
//!
//! There are static objects that represents X86 and X64 registers. They can
//! be used directly (like `eax`, `mm`, `xmm`, ...) or created through
//! these functions:
//!
//! - `asmjit::gpb_lo()` - Get Gpb-lo register.
//! - `asmjit::gpb_hi()` - Get Gpb-hi register.
//! - `asmjit::gpw()` - Get Gpw register.
//! - `asmjit::gpd()` - Get Gpd register.
//! - `asmjit::gpq()` - Get Gpq Gp register.
//! - `asmjit::gpz()` - Get Gpd/Gpq register.
//! - `asmjit::fp()`  - Get Fp register.
//! - `asmjit::mm()`  - Get Mm register.
//! - `asmjit::xmm()` - Get Xmm register.
//! - `asmjit::ymm()` - Get Ymm register.
//!
//! X86/X64 Addressing
//! ------------------
//!
//! X86 and x64 architectures contains several addressing modes and most ones
//! are possible with AsmJit library. Memory represents are represented by
//! `BaseMem` class. These functions are used to make operands that represents
//! memory addresses:
//!
//! - `asmjit::ptr()`
//! - `asmjit::byte_ptr()`
//! - `asmjit::host::word_ptr()`
//! - `asmjit::host::dword_ptr()`
//! - `asmjit::host::qword_ptr()`
//! - `asmjit::tword_ptr()`
//! - `asmjit::oword_ptr()`
//! - `asmjit::yword_ptr()`
//! - `asmjit::intptr_ptr()`
//!
//! Most useful function to make pointer should be `asmjit::ptr()`. It creates
//! pointer to the target with uzspecified size. Uzspecified size works in all
//! intrinsics where are used registers (this means that size is specified by
//! register operand or by instruction itself). For example `asmjit::ptr()`
//! can't be used with @c asmjit::host::Assembler::inc() instruction. In this case
//! size must be specified and it's also reason to make difference between
//! pointer sizes.
//!
//! Supported are simple address forms `[base + displacement]` and complex
//! address forms `[base + index * scale + displacement]`.
//!
//! X86/X64 Immediates
//! ------------------
//!
//! Immediate values are constants thats passed directly after instruction
//! opcode. To create such value use @c asmjit::imm() or @c asmjit::imm_u()
//! methods to create signed or unsigned immediate value.
//!
//! X86/X64 CPU Information
//! -----------------------
//!
//! The CPUID instruction can be used to get an exhaustive information about
//! the host X86/X64 processor. AsmJit contains utilities that can get the most
//! important information related to the features supported by the CPU and the
//! host operating system, in addition to host processor name and number of
//! cores. Class `CpuInfo` extends `BaseCpuInfo` and provides functionality
//! specific to X86 and X64.
//!
//! By default AsmJit queries the CPU information after the library is loaded
//! and the queried information is reused by all instances of `JitRuntime`.
//! The global instance of `CpuInfo` can't be changed, because it will affect
//! the code generation of all `Runtime`s. If there is a need to have a
//! specific CPU information which contains modified features or processor
//! vendor it's possible by creating a new instance of `CpuInfo` and setting
//! up its members. `CpuUtil::detect` can be used to detect CPU features into
//! an existing `CpuInfo` instance - it may become handly if only one property
//! has to be turned on/off.
//!
//! If the high-level interface `CpuInfo` offers is not enough there is also
//! `CpuUtil::callCpuId` helper that can be used to call CPUID instruction with
//! a given parameters and to consume the output.
//!
//! Cpu detection is important when generating a JIT code that may or may not
//! use certain CPU features. For example there used to be a SSE/SSE2 detection
//! in the past and today there is often AVX/AVX2 detection.
//!
//! The example below shows how to detect SSE2:
//!
//! ~~~
//! using namespace asmjit;
//! using namespace asmjit::host;
//!
//! // Get `CpuInfo` global instance.
//! const CpuInfo* cpuInfo = CpuInfo::getHost();
//!
//! if (cpuInfo->hasFeature(kCpuFeatureSse2)) {
//!   // Processor has SSE2.
//! }
//! else if (cpuInfo->hasFeature(kCpuFeatureMmx)) {
//!   // Processor doesn't have SSE2, but has MMX.
//! }
//! else {
//!   // Processor is archaic; it's a wonder AsmJit works here!
//! }
//! ~~~
//!
//! The next example shows how to call `CPUID` directly:
//!
//! ~~~
//! using namespace asmjit;
//!
//! // Call cpuid, first two arguments are passed in Eax/Ecx.
//! CpuId out;
//! CpuUtil::callCpuId(0, 0, &out);
//!
//! // If Eax argument is 0, Ebx, Ecx and Edx registers are filled with a cpu vendor.
//! char cpuVendor[13];
//! ::memcpy(cpuVendor, &out.ebx, 4);
//! ::memcpy(cpuVendor + 4, &out.edx, 4);
//! ::memcpy(cpuVendor + 8, &out.ecx, 4);
//! vendor[12] = '\0';
//!
//! // Print a CPU vendor retrieved from CPUID.
//! ::printf("%s", cpuVendor);
//! ~~~

// ============================================================================
// [asmjit_x86x64_tree]
// ============================================================================

//! \defgroup asmjit_x86x64_tree X86/X64 Code-Tree
//! \ingroup asmjit_x86x64
//!
//! \brief X86/X64 code-tree and helpers.

// ============================================================================
// [asmjit_x86x64_inst]
// ============================================================================

//! \defgroup asmjit_x86x64_inst X86/X64 Instructions
//! \ingroup asmjit_x86x64
//!
//! \brief X86/X64 low-level instruction definitions.

// ============================================================================
// [asmjit_x86x64_util]
// ============================================================================

//! \defgroup asmjit_x86x64_util X86/X64 Utilities
//! \ingroup asmjit_x86x64
//!
//! \brief X86/X64 utility classes.

// ============================================================================
// [asmjit_contrib]
// ============================================================================

//! \defgroup asmjit_contrib Contributions
//!
//! \brief Contributions.

// [Dependencies - Base]
#include "base.h"

// [Dependencies - X86/X64]
#if defined(ASMJIT_BUILD_X86) || defined(ASMJIT_BUILD_X64)
#include "x86.h"
#endif // ASMJIT_BUILD_X86 || ASMJIT_BUILD_X64

// [Dependencies - Host]
#include "host.h"

// [Guard]
#endif // _ASMJIT_ASMJIT_H
