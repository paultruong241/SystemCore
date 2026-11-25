/*****************************************************************************
 * @file formatter_test.cpp
 * @brief Tests text and JSON log formatting.
 *****************************************************************************/

#include "logging/formatters/json_formatter.hpp"
#include "logging/formatters/text_formatter.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>

namespace systemcore::logging
{

namespace
{

LogRecord makeTestRecord()
{
    return LogRecord{
        .level = LogLevel::kWarning,
        .timestamp = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{123}},
        .thread_id = std::this_thread::get_id(),
        .message = "test message"};
}

std::string currentThreadIdText()
{
    std::ostringstream output;
    output << std::this_thread::get_id();
    return output.str();
}

} // namespace

TEST(FormatterTest, TextFormatterIncludesTimestampLevelThreadAndMessage)
{
    const auto record = makeTestRecord();
    const TextFormatter formatter;

    const auto formatted = formatter.format(record);
    const auto expected =
        "[1970-01-01T00:00:00.123Z][WARN][thread:" +
        currentThreadIdText() + "] test message";

    EXPECT_EQ(formatted, expected);
}

TEST(FormatterTest, JsonFormatterIncludesFieldsAndEscapesMessage)
{
    auto record = makeTestRecord();
    record.message = "quoted \" text\nline";
    const JsonFormatter formatter;

    const auto formatted = formatter.format(record);
    const auto expected_prefix =
        "{\"timestamp\":\"1970-01-01T00:00:00.123Z\",\"level\":\"WARN\","
        "\"thread_id\":\"" + currentThreadIdText() + "\",";

    EXPECT_TRUE(formatted.starts_with(expected_prefix));
    EXPECT_TRUE(formatted.ends_with("\"message\":\"quoted \\\" text\\nline\"}"));
}

} // namespace systemcore::logging
