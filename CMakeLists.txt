cmake_minimum_required(VERSION 3.5)
project(csbplustree)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)

ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_executable(test_ChunkrefMemoryHandler tests/ChunkrefMemoryHandler.cpp)
target_link_libraries(test_ChunkrefMemoryHandler
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        )

add_executable(csbtree_write_read tests/csbtree_write_read.cpp)