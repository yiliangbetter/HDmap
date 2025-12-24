cmake_minimum_required(VERSION 3.14)
project(HDMapServer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++ clang-tidy
set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler options for embedded/safety-critical development
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Option for ARM cross-compilation
option(BUILD_FOR_ARM "Cross-compile for ARM architecture" OFF)
if(BUILD_FOR_ARM)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
endif()

# Library target
add_library(hdmap_lib
    src/types.cpp
    src/rtree.cpp
    src/map_server.cpp
    src/lanelet2_parser.cpp
)

target_include_directories(hdmap_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Main executable
add_executable(hdmap_server
    src/main.cpp
)

target_link_libraries(hdmap_server PRIVATE hdmap_lib)

# Install
install(TARGETS hdmap_server DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/hdmap)

# Testing
enable_testing()

# Find Google Test (try system install first, fallback to FetchContent)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # Download Google Test if not found
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test executable
add_executable(hdmap_tests
    tests/test_types.cpp
    tests/test_rtree.cpp
    tests/test_map_server.cpp
)

target_link_libraries(hdmap_tests PRIVATE
    hdmap_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hdmap_tests)

# Memory profiling target (optional)
option(ENABLE_MEMORY_PROFILING "Enable memory profiling with valgrind" OFF)
if(ENABLE_MEMORY_PROFILING)
    find_program(VALGRIND valgrind)
    if(VALGRIND)
        add_custom_target(memcheck
            COMMAND ${VALGRIND} --leak-check=full --show-leak-kinds=all 
                    --track-origins=yes --verbose 
                    $<TARGET_FILE:hdmap_tests>
            DEPENDS hdmap_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endif()

# Performance benchmarking (optional)
option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)
if(BUILD_BENCHMARKS)
    add_executable(hdmap_benchmark
        tests/benchmark_queries.cpp
    )
    target_link_libraries(hdmap_benchmark PRIVATE hdmap_lib)
endif()
