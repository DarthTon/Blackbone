cmake_minimum_required (VERSION 2.8)
project (BlackBone)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest" )

add_subdirectory(BlackBone)
add_subdirectory(TestApp)