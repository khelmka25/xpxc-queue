cmake_minimum_required(VERSION 3.20)
project(MCPC_Queue)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# internal library
add_subdirectory(${CMAKE_SOURCE_DIR}/src)

# Catch2
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/Catch2)

enable_testing()
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)