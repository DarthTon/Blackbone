cmake_minimum_required (VERSION 2.8)
project (TestApp)

include_directories(../../Contrib)

add_executable(TestApp  TestApp.cpp 
                        DriverTest.cpp 
                        LocalHookTest.cpp 
                        MMapTest.cpp
                        MultiPtrTest.cpp
                        PatternTest.cpp 
                        RemoteCallTest.cpp 
                        RemoteHookTest.cpp 
                        RemoteMemTest.cpp
                        Tests.h)
                        
target_link_libraries(TestApp BlackBone)