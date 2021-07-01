cmake_minimum_required (VERSION 3.13)
project (BlackBone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(APPEND CMAKE_CXX_FLAGS " -Wno-multichar -Wno-nonportable-include-path")
    string(APPEND CMAKE_CXX_STANDARD_LIBRARIES " -lshlwapi")
elseif(MINGW)
    string(APPEND CMAKE_CXX_FLAGS " -Wno-multichar")
    string(APPEND CMAKE_CXX_STANDARD_LIBRARIES " -lshlwapi")
endif()

add_subdirectory(BlackBone)
add_subdirectory(Samples)