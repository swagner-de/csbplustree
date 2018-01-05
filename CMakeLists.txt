cmake_minimum_required(VERSION 3.8)
project(csbplustree)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp)
add_executable(csbplustree ${SOURCE_FILES})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")