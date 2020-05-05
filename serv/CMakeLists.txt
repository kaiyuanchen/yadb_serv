cmake_minimum_required(VERSION 2.8)
project(serv)
find_package(GTest REQUIRED)

if(test)
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -pthread ${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
else()
set(CMAKE_CXX_FLAGS "-std=c++0x -O2 -pthread ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

add_subdirectory(src)
include_directories(src)

add_executable(${PROJECT_NAME} main-serv.cpp)
target_link_libraries(${PROJECT_NAME} serv_lib) 

add_executable(${PROJECT_NAME}-cli main-cli.cpp)
target_link_libraries(${PROJECT_NAME}-cli serv_lib) 

include_directories(${GTEST_INCLUDE_DIRS})
add_subdirectory(test)
enable_testing()