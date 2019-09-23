cmake_minimum_required (VERSION 3.13)
project (BlackBoneLib)

if(CMAKE_CL_64)
enable_language(ASM_MASM)
endif()

include_directories(..)

##########################################################
set(SOURCE_ASMJIT   ../3rd_party/asmjit/base/assembler.cpp
                    ../3rd_party/Asmjit/base/codegen.cpp
                    ../3rd_party/Asmjit/base/constpool.cpp
                    ../3rd_party/Asmjit/base/containers.cpp
                    ../3rd_party/Asmjit/base/cpuinfo.cpp
                    ../3rd_party/Asmjit/base/cputicks.cpp
                    ../3rd_party/Asmjit/base/error.cpp
                    ../3rd_party/Asmjit/base/globals.cpp
                    ../3rd_party/Asmjit/base/operand.cpp
                    ../3rd_party/Asmjit/base/runtime.cpp
                    ../3rd_party/Asmjit/base/string.cpp
                    ../3rd_party/Asmjit/base/vmem.cpp
                    ../3rd_party/Asmjit/base/zone.cpp
                    ../3rd_party/Asmjit/x86/x86assembler.cpp
                    ../3rd_party/Asmjit/x86/x86cpuinfo.cpp
                    ../3rd_party/Asmjit/x86/x86inst.cpp
                    ../3rd_party/Asmjit/x86/x86operand.cpp
                    ../3rd_party/Asmjit/x86/x86operand_regs.cpp)
                    
set (HEADER_ASMJIT  ../3rd_party/Asmjit/apibegin.h
                    ../3rd_party/Asmjit/apiend.h
                    ../3rd_party/Asmjit/asmjit.h
                    ../3rd_party/Asmjit/base.h
                    ../3rd_party/Asmjit/build.h
                    ../3rd_party/Asmjit/config.h
                    ../3rd_party/Asmjit/host.h
                    ../3rd_party/Asmjit/x86.h
                    ../3rd_party/Asmjit/base/assembler.h
                    ../3rd_party/Asmjit/base/codegen.h
                    ../3rd_party/Asmjit/base/compiler.h
                    ../3rd_party/Asmjit/base/constpool.h
                    ../3rd_party/Asmjit/base/containers.h
                    ../3rd_party/Asmjit/base/context_p.h
                    ../3rd_party/Asmjit/base/cpuinfo.h
                    ../3rd_party/Asmjit/base/cputicks.h
                    ../3rd_party/Asmjit/base/error.h
                    ../3rd_party/Asmjit/base/globals.h
                    ../3rd_party/Asmjit/base/intutil.h
                    ../3rd_party/Asmjit/base/lock.h
                    ../3rd_party/Asmjit/base/logger.h
                    ../3rd_party/Asmjit/base/operand.h
                    ../3rd_party/Asmjit/base/runtime.h
                    ../3rd_party/Asmjit/base/string.h
                    ../3rd_party/Asmjit/base/vectypes.h
                    ../3rd_party/Asmjit/base/vmem.h
                    ../3rd_party/Asmjit/base/zone.h
                    ../3rd_party/Asmjit/x86/x86assembler.h
                    ../3rd_party/Asmjit/x86/x86compiler.h
                    ../3rd_party/Asmjit/x86/x86context_p.h
                    ../3rd_party/Asmjit/x86/x86cpuinfo.h
                    ../3rd_party/Asmjit/x86/x86inst.h
                    ../3rd_party/Asmjit/x86/x86operand.h
                    ../3rd_party/Asmjit/x86/x86scheduler_p.h)
                    
FILE(GLOB AsmJit ${SOURCE_ASMJIT} ${HEADER_ASMJIT})
source_group(AsmJit\\Core FILES ${AsmJit})

##########################################################
set(SOURCE_HELPERS  Asm/AsmHelper32.cpp
                    Asm/AsmHelper64.cpp
                    Asm/LDasm.c)
                    
set(HEADER_HELPERS  Asm/AsmFactory.h
                    Asm/AsmHelper32.h
                    Asm/AsmHelper64.h
                    Asm/IAsmHelper.h
                    Asm/AsmStack.hpp
                    Asm/AsmVariant.hpp
                    Asm/LDasm.h)
                    
FILE(GLOB AsmJitHelpers ${SOURCE_HELPERS} ${HEADER_HELPERS})
source_group(AsmJit\\Helpers FILES ${AsmJitHelpers})

##########################################################
set(SOURCE_DRV      DriverControl/DriverControl.cpp)                  
set(HEADER_DRV      DriverControl/DriverControl.h)
                    
FILE(GLOB DriverControl ${SOURCE_DRV} ${HEADER_DRV})
source_group(DriverControl FILES ${DriverControl})

##########################################################                    
set(HEADER_INCLUDE  Include/ApiSet.h
                    Include/CallResult.h
                    Include/FunctionTypes.h
                    Include/HandleGuard.h
                    Include/Macro.h
                    Include/NativeStructures.h
                    Include/Types.h
                    Include/Win7Specific.h
                    Include/Win8Specific.h
                    Include/Winheaders.h
                    Include/WinXPSpecific.h)
                    
FILE(GLOB Include ${HEADER_INCLUDE})
source_group(Include FILES ${Include})

##########################################################
set(SOURCE_LOCALHK  LocalHook/LocalHookBase.cpp
                    LocalHook/TraceHook.cpp)
                    
set(HEADER_LOCALHK  LocalHook/HookHandlerCdecl.h
                    LocalHook/HookHandlerFastcall.h
                    LocalHook/HookHandlers.h
                    LocalHook/HookHandlerStdcall.h
                    LocalHook/HookHandlerThiscall.h
                    LocalHook/LocalHook.hpp
                    LocalHook/LocalHookBase.h
                    LocalHook/TraceHook.h
                    LocalHook/VTableHook.hpp)
                    
FILE(GLOB LocalHook ${SOURCE_LOCALHK} ${HEADER_LOCALHK})
source_group(LocalHook FILES ${LocalHook})

##########################################################
set(SOURCE_MMAP     ManualMap/MExcept.cpp
                    ManualMap/MMap.cpp
                    ManualMap/Native/NtLoader.cpp)
                    
set(HEADER_MMAP     ManualMap/MExcept.h
                    ManualMap/MMap.h
                    ManualMap/Native/NtLoader.h)
                    
FILE(GLOB ManualMap ${SOURCE_MMAP} ${HEADER_MMAP})
source_group(ManualMap FILES ${ManualMap})

##########################################################
set(SOURCE_MISC     Misc/InitOnce.cpp
                    Misc/NameResolve.cpp
                    Misc/Utils.cpp)
                    
set(HEADER_MISC     Misc/DynImport.h
                    Misc/InitOnce.h
                    Misc/NameResolve.h
                    Misc/Thunk.hpp
                    Misc/Trace.hpp
                    Misc/Utils.h)
                    
FILE(GLOB Misc ${SOURCE_MISC} ${HEADER_MISC})
source_group(Misc FILES ${Misc})

##########################################################
set(SOURCE_PATTERN  Patterns/PatternSearch.cpp)                  
set(HEADER_PATTERN  Patterns/PatternSearch.h)
                    
FILE(GLOB Patterns ${SOURCE_PATTERN} ${HEADER_PATTERN})
source_group(Patterns FILES ${Patterns})

##########################################################
set(SOURCE_PE       PE/ImageNET.cpp PE/PEImage.cpp)                  
set(HEADER_PE       PE/ImageNET.h   PE/PEImage.h)
                    
FILE(GLOB PE ${SOURCE_PE} ${HEADER_PE})
source_group(PE FILES ${PE})

##########################################################
set(SOURCE_PROCESS  Process/MemBlock.cpp
                    Process/Process.cpp
                    Process/ProcessCore.cpp
                    Process/ProcessMemory.cpp
                    Process/ProcessModules.cpp)
                    
set(HEADER_PROCESS  Process/MemBlock.h
                    Process/Process.h
                    Process/ProcessCore.h
                    Process/ProcessMemory.h
                    Process/ProcessModules.h)
                    
FILE(GLOB Process ${SOURCE_PROCESS} ${HEADER_PROCESS})
source_group(Process FILES ${Process})

##########################################################
set(SOURCE_RPC      Process/RPC/RemoteExec.cpp
                    Process/RPC/RemoteHook.cpp
                    Process/RPC/RemoteLocalHook.cpp
                    Process/RPC/RemoteMemory.cpp)
                    
set(HEADER_RPC      Process/RPC/RemoteContext.hpp
                    Process/RPC/RemoteExec.h
                    Process/RPC/RemoteFunction.hpp
                    Process/RPC/RemoteHook.h
                    Process/RPC/RemoteLocalHook.h
                    Process/RPC/RemoteMemory.h)
                    
FILE(GLOB RPC ${SOURCE_RPC} ${HEADER_RPC})
source_group(Process\\Remote FILES ${RPC})

##########################################################
set(SOURCE_THREADS  Process/Threads/Thread.cpp Process/Threads/Threads.cpp)                 
set(HEADER_THREADS  Process/Threads/Thread.h   Process/Threads/Threads.h)
                    
FILE(GLOB Threads ${SOURCE_THREADS} ${HEADER_THREADS})
source_group(Process\\Threads FILES ${Threads})

##########################################################
set(SOURCE_SUB      Subsystem/NativeSubsystem.cpp
                    Subsystem/Wow64Subsystem.cpp
                    Subsystem/x86Subsystem.cpp
                    ../3rd_party/rewolf-wow64ext/src/wow64ext.cpp)
                    
set(HEADER_SUB      Subsystem/NativeSubsystem.h
                    Subsystem/Wow64Subsystem.h
                    Subsystem/x86Subsystem.h
                    ../3rd_party/rewolf-wow64ext/src/wow64ext.h)
                    
FILE(GLOB Subsystem ${SOURCE_SUB} ${HEADER_SUB})
source_group(Subsystem FILES ${Subsystem})
                    
##########################################################
set(SOURCE_SYMBOLS  Symbols/PatternLoader.cpp
                    Symbols/PDBHelper.cpp
                    Symbols/SymbolData.cpp
                    Symbols/SymbolLoader.cpp)
                    
set(HEADER_SYMBOLS  Symbols/PatternLoader.h
                    Symbols/PDBHelper.h
                    Symbols/SymbolData.h
                    Symbols/SymbolLoader.h)
                    
FILE(GLOB Symbols ${SOURCE_SYMBOLS} ${HEADER_SYMBOLS})
source_group(Symbols FILES ${Symbols})

##########################################################
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
set(SOURCE_SYSCALL  Syscalls/Syscall64.asm)              
else()
set(SOURCE_SYSCALL  Syscalls/Syscall32.asm)              
endif()

set(HEADER_SYSCALL  Syscalls/Syscall.h)
                    
FILE(GLOB Syscalls ${SOURCE_SYSCALL} ${HEADER_SYSCALL})
source_group(Syscalls FILES ${Syscalls})
                
##########################################################
set (HEADER_MAIN    Config.h )

##########################################################                
set(SOURCE_LIB      ${SOURCE_ASMJIT} 
                    ${SOURCE_HELPERS} 
                    ${SOURCE_DRV}
                    ${SOURCE_LOCALHK}
                    ${SOURCE_MMAP}
                    ${SOURCE_MISC}
                    ${SOURCE_PATTERN}
                    ${SOURCE_PE}
                    ${SOURCE_PROCESS}
                    ${SOURCE_RPC}
                    ${SOURCE_THREADS}
                    ${SOURCE_SUB}
                    ${SOURCE_SYMBOLS}
                    ${SOURCE_SYSCALL})
                    
set(HEADER_LIB      ${HEADER_ASMJIT} 
                    ${HEADER_HELPERS} 
                    ${HEADER_DRV}
                    ${HEADER_INCLUDE}
                    ${HEADER_LOCALHK}
                    ${HEADER_MMAP}
                    ${HEADER_MISC}
                    ${HEADER_PATTERN}
                    ${HEADER_PE}
                    ${HEADER_PROCESS}
                    ${HEADER_RPC}
                    ${HEADER_THREADS}
                    ${HEADER_SUB}
                    ${HEADER_SYMBOLS}
                    ${HEADER_SYSCALL}
                    ${HEADER_MAIN})

add_library(BlackBone STATIC ${SOURCE_LIB} ${HEADER_LIB})