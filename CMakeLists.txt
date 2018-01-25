cmake_minimum_required(VERSION 3.5)
project(csbplustree)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_ChunkrefMemoryHandler tests/test_ChunkrefMemoryHandler.cpp tests/csbtree_write_read.cpp)
add_executable(csbtree_write_read tests/csbtree_write_read.cpp)