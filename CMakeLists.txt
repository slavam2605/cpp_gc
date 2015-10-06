cmake_minimum_required(VERSION 3.2)
project(GC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

set(SOURCE_FILES main.cpp)
add_executable(GC ${SOURCE_FILES})