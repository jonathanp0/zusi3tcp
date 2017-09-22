cmake_minimum_required (VERSION 3.2)
project (zusi3tcptest)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# BEGIN gtest

find_package(Threads REQUIRED)
include(ExternalProject)

ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(gtest source_dir binary_dir)

add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

set(GTEST_LIBRARY "${binary_dir}/googlemock/gtest/libgtest.a")
set(GTEST_MAIN_LIBRARY "${binary_dir}/googlemock/gtest/libgtest_main.a")
set(GTEST_INCLUDE_DIR "${source_dir}/googletest/include")

# END gtest

enable_testing()

file(GLOB SRCS *.cpp)

find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIR} "${source_dir}/googlemock/include")

ADD_EXECUTABLE(testzusi3tcp ${SRCS} ../src/Zusi3TCP.cpp)

TARGET_LINK_LIBRARIES(testzusi3tcp zusi3tcp libgtest libgmock)

add_test(zusi3tcp testzusi3tcp)
# This needs to re-run cmake for each new test case, the beow command
# should fix this, but is not suppported on my system, or something
#gtest_add_tests(testzusi3tcp "" ${SRCS})
#add_test_suite(testzusi3tcp
#  LIST_TEST_FLAGS   "--gtest_list_tests"
#  SELECT_TEST_FLAGS "--gtest_filter=<SECTION><TEST>"
#  SECTION_REGEX     "^([A-Za-z_/0-9]+)\\$"
#  TEST_REGEX        "  ([A-Za-z_/0-9])"
#  )
