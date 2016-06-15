cmake_minimum_required(VERSION 2.8)
project(tfp_generator)

#include STXXL
add_subdirectory(libs/stxxl/)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=10000")
include_directories(${STXXL_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -DSTXXL_VERBOSE_LEVEL=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DSTXXL_VERBOSE_LEVEL=0")

#Google Test
add_subdirectory(libs/googletest/googletest)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFILE_DATA_WIDTH=64")


include_directories(include)

add_executable(tfp_ba main_ba.cpp)
target_link_libraries(tfp_ba ${STXXL_LIBRARIES})

add_executable(tfp_bbcr main_bbcr.cpp)
target_link_libraries(tfp_bbcr ${STXXL_LIBRARIES})

add_executable(distribution_count main_distribution.cpp)
target_link_libraries(distribution_count ${STXXL_LIBRARIES})

add_subdirectory(tests)
