/*****************************************************************************
 * @file file_sink_test.cpp
 * @brief Tests file sink output and flush behavior.
 *****************************************************************************/

#include "logging/formatters/text_formatter.hpp"
#include "logging/sinks/file_sink.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace systemcore::logging
{

namespace
{

class TemporaryLogFile
{
public:
    TemporaryLogFile()
    {
        const auto token =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("systemcore_sink_test_" + std::to_string(token) + ".log");
    }

    ~TemporaryLogFile()
    {
        std::error_code error;
        std::filesystem::remove(path_, error);
        EXPECT_FALSE(error) << error.message();
    }

    const std::filesystem::path &path() const noexcept
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

LogRecord makeTestRecord(const LogLevel level, std::string message)
{
    return LogRecord{
        .level = level,
        .timestamp = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{123}},
        .thread_id = {},
        .message = std::move(message)};
}

} // namespace

TEST(FileSinkTest, AppendsRecordsAndFlushesOnRequest)
{
    TemporaryLogFile file;
    {
        std::ofstream initial_contents(file.path());
        ASSERT_TRUE(initial_contents.is_open());
        initial_contents << "existing record\n";
    }

    FileSink sink(
        file.path(),
        std::make_shared<TextFormatter>(),
        FlushPolicy::kManual);
    sink.write(makeTestRecord(LogLevel::kInfo, "new record"));
    sink.flush();

    const auto contents = readFile(file.path());
    EXPECT_EQ(sink.path(), file.path());
    EXPECT_TRUE(contents.starts_with("existing record\n"));
    EXPECT_NE(contents.find("new record"), std::string::npos);
    EXPECT_TRUE(contents.ends_with('\n'));
}

TEST(FileSinkTest, FlushesEveryRecordWhenConfigured)
{
    TemporaryLogFile file;
    FileSink sink(
        file.path(),
        std::make_shared<TextFormatter>(),
        FlushPolicy::kEveryRecord);

    sink.write(makeTestRecord(LogLevel::kInfo, "flushed record"));

    EXPECT_NE(readFile(file.path()).find("flushed record"), std::string::npos);
}

TEST(FileSinkTest, FlushesWarningRecordsWhenConfigured)
{
    TemporaryLogFile file;
    FileSink sink(
        file.path(),
        std::make_shared<TextFormatter>(),
        FlushPolicy::kOnWarningOrHigher);

    sink.write(makeTestRecord(LogLevel::kInfo, "info record"));
    sink.write(makeTestRecord(LogLevel::kWarning, "warning record"));

    const auto contents = readFile(file.path());
    EXPECT_NE(contents.find("info record"), std::string::npos);
    EXPECT_NE(contents.find("warning record"), std::string::npos);
}

TEST(FileSinkTest, RejectsInvalidConfiguration)
{
    auto formatter = std::make_shared<TextFormatter>();
    EXPECT_THROW(
        (void)FileSink({}, formatter),
        std::invalid_argument);

    TemporaryLogFile file;
    EXPECT_THROW(
        (void)FileSink(
            file.path(), std::shared_ptr<const ILogFormatter>{}),
        std::invalid_argument);
}

} // namespace systemcore::logging
