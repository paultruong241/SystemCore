###############################################################################
# @file CMakeLists.txt
# @brief Configures the SystemCore library and test targets.
###############################################################################

cmake_minimum_required(VERSION 3.25) # Require CMake 3.25 or newer.

project(SystemCore
    VERSION 1.0.0 # Set the project version used by targets and installation.
    DESCRIPTION "A modern, modular C++ systems utility library" # Describe the project in generated metadata.
    LANGUAGES CXX # Enable C++ as the project's language.
)

include(GNUInstallDirs) # Load platform-specific installation directory variables.

option(SYSTEMCORE_BUILD_TESTS
    "Build SystemCore unit tests" # Explain this option in CMake GUIs.
    ${PROJECT_IS_TOP_LEVEL} # Enable tests by default only for top-level builds.
)

option(SYSTEMCORE_BUILD_EXAMPLES
    "Build SystemCore examples" # Explain this option in CMake GUIs.
    OFF # Keep examples disabled unless explicitly requested.
)

option(SYSTEMCORE_BUILD_BENCHMARKS
    "Build SystemCore benchmarks" # Explain this option in CMake GUIs.
    OFF # Keep benchmarks disabled unless explicitly requested.
)

add_library(systemcore
    src/benchmark/performance/scope_timer.cpp # Implement the scope timer.
    src/io/files/binary_file_reader.cpp # Implement binary file reading.
    src/io/files/file_reader.cpp # Implement shared file-reader behavior.
    src/logging/formatters/json_formatter.cpp # Implement JSON log formatting.
    src/logging/formatters/text_formatter.cpp # Implement text log formatting.
    src/logging/logger.cpp # Implement logger filtering and dispatch.
    src/logging/sinks/console_sink.cpp # Implement console output.
    src/logging/sinks/file_sink.cpp # Implement file output.
)

add_library(SystemCore::systemcore ALIAS systemcore) # Provide a namespaced target for consumers.

target_compile_features(systemcore
    PUBLIC cxx_std_20 # Require C++20 for this library and its consumers.
)

target_include_directories(systemcore
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> # Use source headers while building.
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> # Use installed headers for consumers.
)

set_target_properties(systemcore PROPERTIES
    CXX_EXTENSIONS OFF # Use standard C++20 without compiler-specific extensions.
    VERSION ${PROJECT_VERSION} # Set the library's full version.
    SOVERSION ${PROJECT_VERSION_MAJOR} # Set the shared-library ABI version.
)

if(SYSTEMCORE_BUILD_TESTS) # Add test targets only when tests are enabled.
    enable_testing() # Enable CTest test discovery and execution.
    add_subdirectory(tests) # Configure the unit-test targets.
endif()

if(SYSTEMCORE_BUILD_EXAMPLES) # Add examples only when explicitly enabled.
    add_subdirectory(examples) # Configure the examples subproject.
endif()

if(SYSTEMCORE_BUILD_BENCHMARKS) # Add benchmarks only when explicitly enabled.
    add_subdirectory(benchmarks) # Configure the benchmarks subproject.
endif()

install(
    TARGETS systemcore # Install the compiled library target.
    EXPORT SystemCoreTargets # Add it to the exported CMake target set.
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # Install static libraries.
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # Install shared libraries.
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # Install runtime binaries.
)

install(
    DIRECTORY include/ # Install the public header tree.
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # Preserve it under the include directory.
)

install(
    EXPORT SystemCoreTargets # Install the exported target definitions.
    FILE SystemCoreTargets.cmake # Name the exported CMake file.
    NAMESPACE SystemCore:: # Prefix imported targets with SystemCore::.
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SystemCore # Store CMake metadata with the library.
)
