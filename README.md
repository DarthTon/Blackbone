# Blackbone

### Windows memory hacking library

## Features

- **x86 and x64 support**

**Process interaction**
 - Manage PEB32/PEB64
 - Manage process through WOW64 barrier

**Process Memory**
 - Allocate and free virtual memory
 - Change memory protection
 - Read/Write virtual memory 

**Process modules**
 - Enumerate all (32/64 bit) modules loaded. Enumerate modules using Loader list/Section objects/PE headers methods.
 - Get exported function address
 - Get the main module
 - Unlink module from loader lists
 - Inject and eject modules (including pure IL images)
 - Inject 64bit modules into WOW64 processes
 - Manually map native PE images

**Threads**
 - Enumerate threads
 - Create and terminate threads. Support for cross-session thread creation.
 - Get thread exit code
 - Get main thread
 - Manage TEB32/TEB64
 - Join threads
 - Suspend and resume threads
 - Set/Remove hardware breakpoints

**Pattern search**
 - Search for arbitrary pattern in local or remote process
 
**Remote code execution**
 - Execute functions in remote process
 - Assemble own code and execute it remotely
 - Support for cdecl/stdcall/thiscall/fastcall conventions
 - Support for arguments passed by value, pointer or reference, including structures
 - FPU types are supported
 - Execute code in new thread or any existing one
 
**Remote hooking**
 - Hook functions in remote process using int3 or hardware breakpoints
 - Hook functions upon return
 
**Manual map features**
 - x86 and x64 image support
 - Mapping into any arbitrary unprotected process
 - Section mapping with proper memory protection flags
 - Image relocations (only 2 types supported. I haven't seen a single PE image with some other relocation types)
 - Imports and Delayed imports are resolved
 - Bound import is resolved as a side effect, I think
 - Module exports
 - Loading of forwarded export images
 - Api schema name redirection
 - SxS redirection and isolation
 - Activation context support
 - Dll path resolving similar to native load order
 - TLS callbacks. Only for one thread and only with PROCESS_ATTACH/PROCESS_DETACH reasons.
 - Static TLS
 - Exception handling support (SEH and C++)
 - Adding module to some native loader structures(for basic module api support: GetModuleHandle, GetProcAdress, etc.)
 - Security cookie initialization
 - C++/CLI images are supported
 - Image unloading 
 - Increase reference counter for import libraries in case of manual import mapping
 - Cyclic dependencies are handled properly
 
**Driver features**
- Allocate/free/protect user memory
- Read/write user and kernel memory
- Disable permanent DEP for WOW64 processes
- Change process protection flag
- Change handle access rights
- Remap process memory
- Hiding allocated user-mode memory
- User-mode dll injection and manual mapping
- Manual mapping of drivers

## Requirements

- Visual Studio 2017 15.7 or higher
- Windows SDK 10.0.17134 or higher
- WDK 10.0.17134 or higher (driver only)
- VC++ 2017 Libs for Spectre (x86 and x64)
- Visual C++ ATL (x86/x64) with Spectre Mitigations

## License
Blackbone is licensed under the MIT License. Dependencies are under their respective licenses.

![Library](https://github.com/DarthTon/Blackbone/workflows/Library/badge.svg?branch=master) ![Driver](https://github.com/DarthTon/Blackbone/workflows/Driver/badge.svg?branch=master)
