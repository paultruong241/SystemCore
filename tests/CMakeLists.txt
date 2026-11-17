find_package(GTest CONFIG REQUIRED)

add_executable(system_core_tests
    benchmark/performance/scope_timer_test.cpp
    containers/thread_safe_queue_test.cpp
    io/files/binary_file_reader_test.cpp
    logging/console_logger_test.cpp
    logging/file_logger_test.cpp
)

target_include_directories(system_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(system_core_tests
    PRIVATE
        system_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(system_core_tests)
