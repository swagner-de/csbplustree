cmake_minimum_required(VERSION 3.8)
project(csbplustree)

set(CMAKE_CXX_STANDARD 17)


set(SOURCE_FILES test_ChunkrefMemoryHandler.cpp)
add_executable(csbplustree ${SOURCE_FILES})