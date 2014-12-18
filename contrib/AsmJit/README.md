AsmJit
------

Complete x86/x64 JIT and Remote Assembler for C++.

  * [Official Repository (kobalicek/asmjit)](https://github.com/kobalicek/asmjit)
  * [Zlib Licensed] (http://www.opensource.org/licenses/zlib-license.php)

Introduction
------------

AsmJit is a complete JIT and remote assembler for C++ language. It can generate native code for x86 and x64 architectures and supports the whole x86/x64 instruction set - from legacy MMX to the newest AVX2. It has a type-safe API that allows C++ compiler to do a semantic checks at compile-time even before the assembled code is generated or run.

AsmJit is not a virtual machine nor tries to be. It's a tool that can be used to encode instructions into their machine code representations and tries to make such process easy and fun. AsmJit has been used so far in software encryption, image/sound processing, emulators and as a JIT backend in virtual machines.

Features
--------

  * Complete x86/x64 instruction set - MMX, SSE, AVX, BMI, XOP, FMA...,
  * Low-level and high-level code generation,
  * Built-in CPU detection,
  * Virtual Memory management,
  * Pretty logging and error handling,
  * Small and embeddable, around 150-200kB compiled,
  * Zero dependencies, not even STL or RTTI.

Supported Environments
----------------------

### Operating Systems

  * BSDs
  * Linux
  * Mac
  * Windows

### C++ Compilers

  * BorlandC++
  * Clang (Travis-CI)
  * Gcc (Travis-CI)
  * MinGW
  * MSVC
  * Other compilers require testing and support in `asmjit/build.h` header

### Backends

  * X86
  * X64

Project Organization
--------------------

  * `/`             - Project root
    * `src`         - Source code
      * `asmjit`    - Public header files (always include from here)
        * `base`    - Base files, used by the AsmJit and all backends
        * `contrib` - Contributions that extend the base functionality
        * `test`    - Unit testing support (don't include in your project)
        * `x86`     - X86/X64 specific files, used only by X86/X64 backend
    * `tools`       - Tools used for configuring, documenting and generating files

Code Generation Concepts
------------------------

AsmJit has two completely different code generation concepts. The difference is in how the code is generated. The first concept, also referred as the low level concept, is called `Assembler` and it's the same as writing RAW assembly by using physical registers directly. In this case AsmJit does only instruction encoding, verification and optionally code-relocation.

The second concept, also referred as the high level concept, is called `Compiler`. Compiler lets you use virtually unlimited number of registers (called variables) significantly simplifying the code generation process. Compiler allocates these virtual registers to physical registers after the code generation is done. This requires some extra effort - Compiler has to generate information for each node (instruction, function declaration, function call) in the code, perform a variable liveness analysis and translate the code having variables into code having only registers.

In addition, Compiler understands functions and function calling conventions. It has been designed in a way that the code generated is always a function having a prototype like in a programming language. By having a function prototype the Compiler is able to insert prolog and epilog to a function being generated and it is able to call a function inside a generated one.

There is no conclusion on which concept is better. Assembler brings full control on how the code is generated, while Compiler makes the generation easier and more portable. However, Compiler does sometimes relatively bad job when it comes to register allocation, so for projects where there is already an analysis performed, pure Assembler code generator is the preferred way.

Configuring & Building
----------------------

AsmJit is designed to be easy embeddable in any kind project. However, it has some compile-time flags that can be used to build a specific version of AsmJit including or omitting certain features. A typical way to build AsmJit is to use [cmake](http://www.cmake.org), but it's also possible to just include AsmJit source code in our project and build it with it optionally editing its `asmjit/config.h` file to turn on/off some specific features. The most easies way to include AsmJit in your project is to just copy AsmJit source somewhere into it and to define globally `ASMJIT_STATIC` macro. This way AsmJit can be just updated from time to time without any changes to it. Please do not include / compile AsmJit test files (`asmjit/test` directory) when embedding.

### Build Type

  * `ASMJIT_EMBED` - Parameter that can be set to cmake to turn off building library, useful if you want to include asmjit in your project without building the library. `ASMJIT_EMBED` behaves identically as `ASMJIT_STATIC`.
  * `ASMJIT_STATIC` - Define when building AsmJit as a static library. No symbols will be exported by AsmJit by default.

  * By default AsmJit build is configured as a shared library so none of `ASMJIT_EMBED` and `ASMJIT_STATIC` have to be defined explicitly.

### Build Mode

  * `ASMJIT_DEBUG` - Define to always turn debugging on (regardless of build-mode).
  * `ASMJIT_RELEASE` - Define to always turn debugging off (regardless of build-mode).
  * `ASMJIT_TRACE` - Define to enable AsmJit tracing. Tracing is used to catch bugs in AsmJit and it has to be enabled explicitly. When AsmJit is compiled with `ASMJIT_TRACE` it uses `stdout` to log information related to AsmJit execution. This log can be helpful when examining liveness analysis, register allocation or any other part of AsmJit.

  * By default none of these is defined, AsmJit detects mode based on compile-time macros (useful when using IDE that has switches for Debug/Release/etc...).

### Architectures

  * `ASMJIT_BUILD_X86` - Always build x86 backend regardless of host architecture.
  * `ASMJIT_BUILD_X64` - Always build x64 backend regardless of host architecture.
  * `ASMJIT_BUILD_HOST` - Always build host backend, if only `ASMJIT_BUILD_HOST` is used only the host architecture detected at compile-time will be included.

  * By default only `ASMJIT_BUILD_HOST` is defined.

### Features

  * `ASMJIT_DISABLE_COMPILER` - Disable `Compiler` completely. Use this flag if you don't use Compiler and want slimmer binary.

  * `ASMJIT_DISABLE_LOGGER` - Disable `Logger` completely. Use this flag if you don't need `Logger` functionality and want slimmer binary. AsmJit compiled with or without `Logger` support is binary compatible (all classes that use Logger pointer will simply use `void*`), but the Logger interface and in general instruction dumps are not available.

  * `ASMJIT_DISABLE_NAMES` - Disable everything that uses strings and that causes certain strings to be stored in the resulting binary. For example when this flag is enabled instruction or error names (and related APIs) will not be available. This flag has to be disabled together with `ASMJIT_DISABLE_LOGGER`.

Using AsmJit
------------

AsmJit library uses one global namespace called `asmjit`, which contains the basics. Architecture specific code is prefixed by the architecture and architecture registers and operand builders are in its own namespace. For example classes for both x86 and x64 code generation are prefixed by `X86`, enums by `kX86`, registers and operand builders are accessible through `x86` namespace. This design is very different from the initial version of AsmJit and it seems now as the most convenient one.

### Runtime & Code-Generators

AsmJit contains two classes that are required to generate a machine code. `Runtime` specifies where the code is generated and acts as a storage, while `CodeGen` specifies how the code is generated and acts as a machine code stream. All the examples here use `Compiler` code-generator to generate the code and `JitRuntime` to store and run it.

### Instruction Operands

Operand is a part of CPU instruction which specifies the data the instruction will operate on. There are five types of operands in AsmJit:

  * `Reg` - Physical register, used only by `Assembler`
  * `Var` - Virtual register, used only by `Compiler`
  * `Mem` - Used to reference memory location
  * `Label` - Used to reference a location in code
  * `Imm` - Immediate value that is encoded with the instruction itself

Base class for all operands is `Operand`. It contains interface that can be used by all types of operands only and it is typically passed by value, not as a pointer. The classes `Reg`, `Var`, `BaseMem`, `Label` and `Imm` all inherit `Operand` and provide an operand specific functionality. Architecture specific operands are prefixed by the architecture like `X86Reg` or `X86Mem`. Most of the architectures provide several types of registers, for example x86/x64 architecture has `X86GpReg`, `X86MmReg`, `X86FpReg`, `X86XmmReg` and `X86YmmReg` registers plus some extras including segment registers and `rip` (relative instruction pointer).

When using a code-generator some operands have to be created explicitly by using its interface. For example labels are created by using `newLabel()` method of the code-generator and variables are created by using architecture specific methods like `newGpVar()`, `newMmVar()` or `newXmmVar()`.

### Function Prototypes

AsmJit needs to know the prototype of the function it will generate or call. AsmJit contains a mapping between a type and the register that will be used to represent it. To make life easier there is a function builder that does the mapping on the fly. Function builder is a template class that helps with creating a function prototype by using native C/C++ types that describe function arguments and return value. It translates C/C++ native types into AsmJit specific IDs and makes these IDs accessible to Compiler.

### Putting It All Together

Let's put all together and generate a first function that sums its two arguments and returns the result. At the end the generated function is called from a C++ code.

```C++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  // Create JitRuntime and X86 Compiler.
  JitRuntime runtime;
  X86Compiler c(&runtime);

  // Build function having two arguments and a return value of type 'int'.
  // First type in function builder describes the return value. kFuncConvHost
  // tells compiler to use a host calling convention.
  c.addFunc(kFuncConvHost, FuncBuilder2<int, int, int>());

  // Create 32-bit variables (virtual registers) and assign some names to
  // them. Using names is purely optional and only greatly helps while
  // debugging.
  X86GpVar a(c, kVarTypeInt32, "a");
  X86GpVar b(c, kVarTypeInt32, "b");

  // Tell asmjit to use these variables as function arguments.
  c.setArg(0, a);
  c.setArg(1, b);

  // a = a + b;
  c.add(a, b);

  // Tell asmjit to return 'a'.
  c.ret(a);

  // Finalize the current function.
  c.endFunc();

  // Now the Compiler contains the whole function, but the code is not yet
  // generated. To tell compiler to generate the function make() has to be
  // called.

  // Make uses the JitRuntime passed to Compiler constructor to allocate a
  // buffer for the function and make it executable.
  void* funcPtr = c.make();

  // In order to run 'funcPtr' it has to be casted to the desired type.
  // Typedef is a recommended and safe way to create a function-type.
  typedef int (*FuncType)(int, int);

  // Using asmjit_cast is purely optional, it's basically a C-style cast
  // that tries to make it visible that a function-type is returned.
  FuncType func = asmjit_cast<FuncType>(funcPtr);

  // Finally, run it and do something with the result...
  int x = func(1, 2);
  printf("x=%d\n", x); // Outputs "x=3".

  // The function will remain in memory after Compiler is destroyed, but
  // will be destroyed together with Runtime. This is just simple example
  // where we can just destroy both at the end of the scope and that's it.
  // However, it's a good practice to clean-up resources after they are
  // not needed and using runtime.release() is the preferred way to free
  // a function added to JitRuntime.
  runtime.release((void*)func);

  // Runtime and Compiler will be destroyed at the end of the scope.
  return 0;
}
```

The code should be self explanatory, however there are some details to be clarified.

The code above generates and calls a function of `kFuncConvHost` calling convention. 32-bit architecture contains a wide range of function calling conventions that can be all used by a single program, so it's important to know which calling convention is used by your C/C++ compiler so you can call the function. However, most compilers should generate CDecl by default. In 64-bit mode there are only two calling conventions, one is specific for Windows (Win64 calling convention) and the other for Unix (AMD64 calling convention). The `kFuncConvHost` is defined to be one of CDecl, Win64 or AMD64 depending on your architecture and operating system.

Default integer size is platform specific, virtual types `kVarTypeIntPtr` and `kVarTypeUIntPtr` can be used to make the code more portable and they should be always used when a pointer type is needed. When no type is specified AsmJit always defaults to `kVarTypeIntPtr`. The code above works with integers where the default behavior has been overidden to 32-bits. Note it's always a good practice to specify the type of the variable used. Alternative form of creating a variable is `c.newGpVar(...)`, `c.newMmVar(...)`, `c.newXmmVar` and so on...

The function starts with `c.addFunc()` and ends with `c.endFunc()`. It's not allowed to put code outside of the function; however, embedding data outside of the function body is allowed.

### Using Labels

Labels are essential for making jumps, function calls or to refer to a data that is embedded in the code section. Label has to be explicitly created by using `newLabel()` method of your code generator in order to be used. The following example executes a code that depends on the condition by using a `Label` and conditional jump instruction. If the first parameter is zero it returns `a + b`, otherwise `a - b`.

```C++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  JitRuntime runtime;
  X86Compiler c(&runtime);

  // This function uses 3 arguments.
  c.addFunc(kFuncConvHost, FuncBuilder3<int, int, int, int>());

  // New variable 'op' added.
  X86GpVar op(c, kVarTypeInt32, "op");
  X86GpVar a(c, kVarTypeInt32, "a");
  X86GpVar b(c, kVarTypeInt32, "b");

  c.setArg(0, op);
  c.setArg(1, a);
  c.setArg(2, b);

  // Create labels.
  Label L_Subtract(c);
  Label L_Skip(c);

  // If (op != 0)
  //   goto L_Subtract;
  c.test(op, op);
  c.jne(L_Subtract);

  // a = a + b;
  // goto L_Skip;
  c.add(a, b);
  c.jmp(L_Skip);

  // L_Subtract:
  // a = a - b;
  c.bind(L_Subtract);
  c.sub(a, b);

  // L_Skip:
  c.bind(L_Skip);

  c.ret(a);
  c.endFunc();

  // The prototype of the generated function changed also here.
  typedef int (*FuncType)(int, int, int);
  FuncType func = asmjit_cast<FuncType>(c.make());

  int x = func(0, 1, 2);
  int y = func(1, 1, 2);

  printf("x=%d\n", x); // Outputs "x=3".
  printf("y=%d\n", y); // Outputs "y=-1".

  runtime.release((void*)func);
  return 0;
}
```

In this example conditional and unconditional jumps were used with labels together. Labels are created explicitly by the `Compiler` by passing a `Compiler` instance to a `Label` constructor or by using a `Label l = c.newLabel()` form. Each label as an unique ID that identifies it, however it's not a string and there is no way to query for a `Label` instance that already exists. Label is like any other operand moved by value, so the copy of the label will still reference the same label and changing a copied label will not change the original label.

Each label has to be bound to the location in the code by using `c.bind()`; however, it can be only bound once! Trying to bind the same label multiple times has undefined behavior - it will trigger an assertion failure in the best case.

### Memory Addressing

X86/X64 architectures have several memory addressing modes which can be used to combine base register, index register and a displacement. In addition, index register can be shifted by a constant from 1 to 3 that can help with addressing elements up to 8-byte long in an array. AsmJit supports all forms of memory addressing. Memory operand can be created by using `asmjit::X86Mem` or by using related non-member functions like `asmjit::x86::ptr` or `asmjit::x86::ptr_abs`. Use `ptr` to create a memory operand having a base register with optional index register and a displacement; use and `ptr_abs` to create a memory operand referring to an absolute address in memory (32-bit) and optionally having an index register.

In the following example various memory addressing modes are used to demonstrate how to construct and use them. It creates a function that accepts an array and two indexes which specify which elements to sum and return.

```C++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  JitRuntime runtime;
  X86Compiler c(&runtime);

  // Function returning 'int' accepting pointer and two indexes.
  c.addFunc(kFuncConvHost, FuncBuilder3<int, const int*, intptr_t, intptr_t>());

  X86GpVar p(c, kVarTypeIntPtr, "p");
  X86GpVar aIndex(c, kVarTypeIntPtr, "aIndex");
  X86GpVar bIndex(c, kVarTypeIntPtr, "bIndex");

  c.setArg(0, p);
  c.setArg(1, aIndex);
  c.setArg(2, bIndex);

  X86GpVar a(c, kVarTypeInt32, "a");
  X86GpVar b(c, kVarTypeInt32, "b");

  // Read 'a' by using a memory operand having base register, index register
  // and scale. Translates to 'mov a, dword ptr [p + aIndex << 2]'.
  c.mov(a, ptr(p, aIndex, 2));

  // Read 'b' by using a memory operand having base register only. Variables
  // 'p' and 'bIndex' are both modified.

  // Shift bIndex by 2 (exactly the same as multiplying by 4).
  // And add scaled 'bIndex' to 'p' resulting in 'p = p + bIndex * 4'.
  c.shl(bIndex, 2);
  c.add(p, bIndex);

  // Read 'b'.
  c.mov(b, ptr(p));

  // a = a + b;
  c.add(a, b);

  c.ret(a);
  c.endFunc();

  // The prototype of the generated function changed also here.
  typedef int (*FuncType)(const int*, intptr_t, intptr_t);
  FuncType func = asmjit_cast<FuncType>(c.make());

  // Array passed to 'func'
  const int array[] = { 1, 2, 3, 5, 8, 13 };

  int x = func(array, 1, 2);
  int y = func(array, 3, 5);

  printf("x=%d\n", x); // Outputs "x=5".
  printf("y=%d\n", y); // Outputs "y=18".

  runtime.release((void*)func);
  return 0;
}
```

### Using Stack

AsmJit uses stack automatically to spill variables if there is not enough registers to keep them all allocated. The stack frame is managed by `Compiler` that provides also an interface to allocate chunks of memory of user specified size and alignment.

In the following example a stack of 256 bytes size is allocated, filled by bytes starting from 0 to 255 and then iterated again to sum all the values.

```C++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  JitRuntime runtime;
  X86Compiler c(&runtime);

  // Function returning 'int' without any arguments.
  c.addFunc(kFuncConvHost, FuncBuilder0<int>());

  // Allocate a function stack of size 256 aligned to 4 bytes.
  X86Mem stack = c.newStack(256, 4);

  X86GpVar p(c, kVarTypeIntPtr, "p");
  X86GpVar i(c, kVarTypeIntPtr, "i");

  // Load a stack address to 'p'. This step is purely optional and shows
  // that 'lea' is useful to load a memory operands address (even absolute)
  // to a general purpose register.
  c.lea(p, stack);

  // Clear 'i'. Notice that xor_() is used instead of xor(), because xor is
  // unfortunately a keyword in C++.
  c.xor_(i, i);

  // First loop, fill the stack allocated by a sequence of bytes from 0 to 255.
  Label L1(c);
  c.bind(L1);

  // Mov [p + i], i.
  //
  // Any operand can be cloned and modified. By cloning 'stack' and calling
  // 'setIndex' we created a new memory operand based on stack having an
  // index register set.
  c.mov(stack.clone().setIndex(i), i.r8());

  // if (++i < 256)
  //   goto L1;
  c.inc(i);
  c.cmp(i, 256);
  c.jb(L1);

  // Second loop, sum all bytes stored in 'stack'.
  X86GpVar a(c, kVarTypeInt32, "a");
  X86GpVar t(c, kVarTypeInt32, "t");

  c.xor_(i, i);
  c.xor_(a, a);

  Label L2(c);
  c.bind(L2);

  // Movzx t, byte ptr [stack + i]
  c.movzx(t, stack.clone().setIndex(i).setSize(1));
  // a += t;
  c.add(a, t);

  // if (++i < 256)
  //   goto L2;
  c.inc(i);
  c.cmp(i, 256);
  c.jb(L2);

  c.ret(a);
  c.endFunc();

  typedef int (*FuncType)(void);

  FuncType func = asmjit_cast<FuncType>(c.make());
  printf("a=%d\n", func()); // Outputs "a=32640".

  runtime.release((void*)func);
  return 0;
}
```

### Using Constant Pool

To be documented.

Advanced Features
-----------------

AsmJit offers much more, but not everything can fit into the introduction. The following sections don't have complete examples, but contain hints that can be useful and can change a bit the way AsmJit is used.

### Logging and Error Handling

Failures are common when working at machine level. AsmJit does already a good job with function overloading to prevent from emitting semantically incorrect instructions; however, AsmJit can't prevent from emitting code that is semantically correct, but contains bug(s). Logging has always been an important part of AsmJit's infrastructure and the output can be very valuable after something went wrong.

AsmJit contains extensible logging interface defined by `Logger` class and implemented by `FileLogger` and `StringLogger`. `FileLogger` can log into a standard C-based `FILE*` stream while `StringLogger` logs to an internal buffer that can be used after the code generation is done.

Loggers can be assigned to any code generator and there is no restriction of assigning a single logger to multiple code generators, but this is not practical when running these in multiple threads. `FileLogger` is thread-safe since it uses plain C `FILE*` stream, but `StringLogger` is not!

The following snippet describes how to log into `FILE*`:

```C++
// Create logger logging to `stdout`. Logger life-time should always be
// greater than lifetime of the code generator.
FileLogger logger(stdout);

// Create a code generator and assign our logger into it.
X86Compiler c(...);
c.setLogger(&logger);

// ... Generate the code ...
```

The following snippet describes how to log into a string:

```C++
StringLogger logger;

// Create a code generator and assign our logger into it.
X86Compiler c(...);
c.setLogger(&logger);

// ... Generate the code ...

printf("Logger Content:\n%s", logger.getString());

// You can also use `logger.clearString()` if the logger
// instance will be reused.
```

Logger can be configured to show more information by using `logger.setOption()` method. The following options are available:

  * `kLoggerOptionBinaryForm` - Log also binary sequence for each instruction generated.
  * `kLoggerOptionHexImmediate` - Format immediate values to base16 (hex) form.
  * `kLoggerOptionHexDisplacement` - Format memory displacements to base16 (hex) form.

TODO: Liveness analysis and instruction scheduling options.

### Code Injection

Code injection was one of key concepts of Compiler from the beginning. Compiler records all emitted instructions in a double-linked list which can be manipulated before `make()` is called. Any call to Compiler that adds instruction, function or anything else in fact manipulates this list by inserting nodes into it.

To manipulate the current cursor use Compiler's `getCursor()` and `setCursor()` methods. The following snippet demonstrates the proper way of code injection.

```C++
X86Compiler c(...);

X86GpVar a(c, kVarTypeInt32, "a");
X86GpVar b(c, kVarTypeInt32, "b");

Node* here = c.getCursor();
c.mov(b, 2);

// Now, 'here' can be used to inject something before 'mov b, 2'. To inject
// anything it's good to remember the current cursor so it can be set back
// after the injecting is done. When setCursor() is called it returns the old
// cursor.
Node* oldCursor = c.setCursor(here);
c.mov(a, 1);
c.setCursor(oldCursor);
```

The resulting code would look like:

```
c.mov(a, 1);
c.mov(b, 2);
```

Support
-------

Please consider a donation if you use the project and would like to keep it active in the future.

  * [Donate by PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=QDRM6SRNG7378&lc=EN;&item_name=asmjit&currency_code=EUR)

Received From:

  * [PeLock - Software copy protection and license key system](http://www.pelock.com)

Authors & Maintainers
---------------------

  * Petr Kobalicek <kobalicek.petr@gmail.com>
