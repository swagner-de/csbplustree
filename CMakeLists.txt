cmake_minimum_required(VERSION 3.5)
project(csbplustree)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Weffc++ -march=native")


add_executable(test_csbtree_read_write tests/test_csbtree_read_write.cpp)

add_executable(perf_all tests/perf_all_idx_struc.cpp)
add_library(csvwriter src/CsvWriter_t.cxx)
add_library(art art_src/art.c)
target_link_libraries(perf_all csvwriter art)


add_executable(perf_FatStack tests/perf_FatStack.cpp)

find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_executable(test_ChunkRefMemoryHandler tests/test_ChunkRefMemoryHandler.cpp)
target_link_libraries(test_ChunkRefMemoryHandler
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        )


add_executable(test_BitSetMemoryHandler tests/test_BitSetMemoryHandler.cpp)
target_link_libraries(test_BitSetMemoryHandler
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        )

add_executable(test_perf_simd tests/perf_simd.cxx)
add_executable(test_perf_move tests/perf_move.cpp)
add_executable(pef_mem_fill tests/perf_mem_fill.cpp)