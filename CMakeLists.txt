cmake_minimum_required(VERSION 2.8)
project(tfp_generator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# STXXL
find_package(STXXL REQUIRED)
if(STXXL_FOUND)
    # print some info (this can be removed)
    message(STATUS "STXXL_CXX_FLAGS: ${STXXL_CXX_FLAGS}")
    message(STATUS "STXXL_INCLUDE_DIRS: ${STXXL_INCLUDE_DIRS}")
    message(STATUS "STXXL_LIBRARIES: ${STXXL_LIBRARIES}")

    # apply CXXFLAGS to our configuration
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS} -ftemplate-depth=10000")
    include_directories(${STXXL_INCLUDE_DIRS})

    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -DSTXXL_VERBOSE_LEVEL=1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DSTXXL_VERBOSE_LEVEL=0")
endif(STXXL_FOUND)

include_directories(include)

add_executable(tfp_ba main_ba.cpp)
target_link_libraries(tfp_ba ${STXXL_LIBRARIES})

add_executable(tfp_bbcr main_bbcr.cpp)
target_link_libraries(tfp_bbcr ${STXXL_LIBRARIES})

add_subdirectory(tests)
