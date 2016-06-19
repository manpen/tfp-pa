cmake_minimum_required(VERSION 2.8)
project(tfp_generator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTXXL_PARALLEL=1")
#include STXXL

SET(TMP_CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
add_subdirectory(libs/stxxl/)
SET(CMAKE_BUILD_TYPE "${TMP_CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=10000")
include_directories(${STXXL_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -DSTXXL_VERBOSE_LEVEL=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DSTXXL_VERBOSE_LEVEL=0")

#Google Test
add_subdirectory(libs/googletest/googletest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -Og")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFILE_DATA_WIDTH=64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=native -msse4")


check_cxx_compiler_flag( -flto GXX_HAS_LTO_FLAG )
if( CMAKE_BUILD_TYPE MATCHES Release AND GXX_HAS_LTO_FLAG )
    find_program(CMAKE_GCC_AR NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-ar${_CMAKE_TOOLCHAIN_SUFFIX} HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
    find_program(CMAKE_GCC_NM NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-nm HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
    find_program(CMAKE_GCC_RANLIB NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-ranlib HINTS ${_CMAKE_TOOLCHAIN_LOCATION})

    if( CMAKE_GCC_AR AND CMAKE_GCC_NM AND CMAKE_GCC_RANLIB )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -fno-fat-lto-objects" )
        set( CMAKE_AR "${CMAKE_GCC_AR}" )
        set( CMAKE_NM "${CMAKE_GCC_NM}" )
        set( CMAKE_RANLIB "${CMAKE_GCC_RANLIB}" )
        message ("Enable LTO")
    else()
        message( WARNING "GCC indicates LTO support, but binutils wrappers could not be found. Disabling LTO." )
    endif()
endif()

execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
if ("${LD_VERSION}" MATCHES "GNU gold")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
    message("Enable GOLD Linker")
else ()
    message(WARNING "GNU gold linker isn't available, using the default system linker.")
    set(USE_LD_GOLD OFF)
endif ()



include_directories(include)

add_executable(tfp_ba main_ba.cpp)
target_link_libraries(tfp_ba ${STXXL_LIBRARIES})

add_executable(ptfp_ba main_pba.cpp)
target_link_libraries(ptfp_ba ${STXXL_LIBRARIES})

add_executable(tfp_bbcr main_bbcr.cpp)
target_link_libraries(tfp_bbcr ${STXXL_LIBRARIES})

add_executable(distribution_count main_distribution.cpp)
target_link_libraries(distribution_count ${STXXL_LIBRARIES})

add_subdirectory(tests)
