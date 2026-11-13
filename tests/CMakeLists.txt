find_package(GTest CONFIG REQUIRED)

add_executable(system_core_tests
    containers/thread_safe_queue_test.cpp
    logging/console_logger_test.cpp
    logging/file_logger_test.cpp
)

target_link_libraries(system_core_tests
    PRIVATE
        system_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(system_core_tests)
