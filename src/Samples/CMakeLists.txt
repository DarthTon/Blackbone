cmake_minimum_required (VERSION 3.13)
project (Samples)

include_directories(..)

cmake_policy(SET CMP0015 NEW)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(../3rd_party/DIA/lib/amd64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    link_directories(../3rd_party/DIA/lib)
endif()

add_executable(Samples Main.cpp ManualMap.cpp)
                        
target_link_libraries(Samples BlackBone diaguids.lib)