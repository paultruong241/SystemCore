/*****************************************************************************
 * @file file_logger_test.cpp
 * @brief Tests FileLogger filtering and file output.
 *****************************************************************************/

#include "logging/file_logger.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace
{

/**
 * @brief Provides a temporary working directory for file logger tests.
 */
class FileLoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        previous_directory_ = std::filesystem::current_path();
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        test_directory_ = std::filesystem::temp_directory_path() /
                          ("systemcore_file_logger_tests_" + std::to_string(timestamp));

        ASSERT_TRUE(std::filesystem::create_directory(test_directory_));
        std::filesystem::current_path(test_directory_);
        working_directory_changed_ = true;
    }

    void TearDown() override
    {
        if (working_directory_changed_)
        {
            std::error_code error;
            std::filesystem::current_path(previous_directory_, error);
            EXPECT_FALSE(error) << error.message();
        }

        if (!test_directory_.empty())
        {
            std::error_code error;
            std::filesystem::remove_all(test_directory_, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    /**
     * @brief Finds the log file created in the temporary working directory.
     * @return The matching path, or an empty path if no log file exists.
     */
    std::filesystem::path findLogFile() const
    {
        for (const auto &entry : std::filesystem::directory_iterator(test_directory_))
        {
            if (entry.path().filename().string().starts_with("log_") &&
                entry.path().extension() == ".txt")
            {
                return entry.path();
            }
        }

        return {};
    }

private:
    std::filesystem::path previous_directory_;
    std::filesystem::path test_directory_;
    bool working_directory_changed_ = false;
};

} // namespace

/**
 * @brief Verifies that below-threshold messages are omitted from the log file.
 */
TEST_F(FileLoggerTest, FiltersMessagesBelowThreshold)
{
    {
        FileLogger logger(LogLevel::kInfo);
        logger.log(LogLevel::kDebug, "filtered file message");
        logger.log(LogLevel::kInfo, "accepted file message");
    }

    const auto log_path = findLogFile();
    ASSERT_FALSE(log_path.empty()) << "FileLogger did not create a log file";

    std::ifstream log_file(log_path);
    ASSERT_TRUE(log_file.is_open()) << "Could not read " << log_path;

    const std::string contents((std::istreambuf_iterator<char>(log_file)),
                               std::istreambuf_iterator<char>());
    EXPECT_EQ(contents.find("filtered file message"), std::string::npos);
    EXPECT_NE(contents.find("[INFO]"), std::string::npos);
    EXPECT_NE(contents.find("accepted file message"), std::string::npos);
}

/**
 * @brief Verifies that warning-level messages are written when accepted.
 */
TEST_F(FileLoggerTest, WritesAcceptedWarningMessages)
{
    {
        FileLogger logger(LogLevel::kWarning);
        logger.log(LogLevel::kInfo, "filtered info message");
        logger.log(LogLevel::kWarning, "accepted warning message");
    }

    const auto log_path = findLogFile();
    ASSERT_FALSE(log_path.empty()) << "FileLogger did not create a log file";

    std::ifstream log_file(log_path);
    ASSERT_TRUE(log_file.is_open()) << "Could not read " << log_path;

    const std::string contents((std::istreambuf_iterator<char>(log_file)),
                               std::istreambuf_iterator<char>());
    EXPECT_EQ(contents.find("filtered info message"), std::string::npos);
    EXPECT_NE(contents.find("[WARN]"), std::string::npos);
    EXPECT_NE(contents.find("accepted warning message"), std::string::npos);
}
