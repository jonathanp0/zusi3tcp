cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(zusi3tcp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
