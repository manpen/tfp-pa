############################################################################
# Authors:
#  2015 -       Manuel Penschuck
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################

enable_testing()
find_package(GTest)

#if (GTest_FOUND)
message(STATUS "GTEST_INCLUDE_DIRS: ${GTEST_INCLUDE_DIRS}")
include_directories(${GTEST_INCLUDE_DIRS} /home/manpen/documents/code/googletest/googletest/include)

file(GLOB SRCS *.cpp)

add_executable(tests ${SRCS})

target_link_libraries(tests
        /home/manpen/documents/code/googletest/googletest/build/libgtest.a
        /home/manpen/documents/code/googletest/googletest/build/libgtest_main.a
        ${GTEST_BOTH_LIBRARIES}
        ${STXXL_LIBRARIES}
)

add_test(tests tests)
