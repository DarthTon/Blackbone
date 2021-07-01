# Support for MinGW/Clang

There is limited, experimental support for building BlackBone with MinGW (GCC) and Clang on Windows. Read the following hints carefully before attempting to use it.


## General

* You must **build with CMake**, by using the directory with the top-level CMakeLists.txt as source directory.
* **Only the library and Samples** work with MinGW/Clang. I have not attempted to make the driver or the unit tests  compile. I have not even tried if the library works with an MSVC-compiled driver.
* **Only 64-bit builds** are supported. Even if it compiles under 32-bit, it may crash at runtime. This is because BlackBone uses the Rewolf-WOW64Ext library when running under Wow64, and that contains inline ASM code that only works in MSVC right now. No idea about 32-bit builds on 32-bit Windows.
* **Syscall support is disabled.** This means anything in the *Blackbone/Syscalls* directory won't work. That's because syscalls in BlackBone require ASM code that was written in MASM syntax, which neither MinGW nor Clang support.


## MinGW (GCC) Specifics

* It was tested with **MinGW GCC 10.0.3 from MSYS2**. GCC 8.1.0 did also work at some point, but YMMV.


## Clang Specifics

* It was tested with **Clang 12.0.0 from MSYS2**. Earlier versions may or may not work.
