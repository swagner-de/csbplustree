cmake_minimum_required(VERSION 3.5)
project(csbplustree)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Weffc++")


add_executable(csbtree_write_read tests/csbtree_write_read.cpp)

add_executable(perf_test tests/perf.cpp)
add_library(csvwriter src/CsvWriter_t.cxx)
add_library(art art_src/art.c)
target_link_libraries(perf_test csvwriter art)


add_executable(test_FatStack tests/FatStack.cpp)

find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_executable(test_ChunkrefMemoryHandler tests/ChunkrefMemoryHandler.cpp)
target_link_libraries(test_ChunkrefMemoryHandler
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        )
