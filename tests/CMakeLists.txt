find_package(GTest CONFIG REQUIRED)

add_executable(system_core_tests
    benchmark/performance/scope_timer_test.cpp
    concurrency/bounded_queue_test.cpp
    concurrency/circular_buffer_test.cpp
    concurrency/queue_test.cpp
    logging/console_sink_test.cpp
    logging/file_sink_test.cpp
    logging/formatter_test.cpp
    logging/logger_test.cpp
    io/files/binary_file_reader_test.cpp
)

target_include_directories(system_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(system_core_tests
    PRIVATE
        SystemCore::systemcore
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(system_core_tests)
