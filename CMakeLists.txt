###############################################################################
# @file CMakeLists.txt
# @brief Defines the SystemCore build targets and test integration.
###############################################################################

cmake_minimum_required(VERSION 3.20)

project(SystemCore
    VERSION 1.0.0
    LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the production library
add_library(system_core
    src/benchmark/performance/scope_timer.cpp
    src/io/files/binary_file_reader.cpp
    src/io/files/file_reader.cpp
    src/logging/formatters/json_formatter.cpp
    src/logging/formatters/text_formatter.cpp
    src/logging/logger.cpp
    src/logging/sinks/console_sink.cpp
    src/logging/sinks/file_sink.cpp
)

target_include_directories(system_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/logging
        ${PROJECT_SOURCE_DIR}/include/containers
)

# Build CMake Alias
add_library(SystemCore::SystemCore ALIAS system_core)

# Enable unit testing
include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()