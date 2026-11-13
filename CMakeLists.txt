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
    src/logging/console_logger.cpp
    src/logging/file_logger.cpp
    src/logging/logger.cpp
)

target_include_directories(system_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/logging
)

# Enable unit testing
include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()