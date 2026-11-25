/*****************************************************************************
 * @file console_logger_test.cpp
 * @brief Tests ConsoleLogger filtering and output.
 *****************************************************************************/

#include "logging/console_logger.hpp"
#include "utils/cout_capture.hpp"

#include <gtest/gtest.h>

#include <string>

namespace logging
{

/**
 * @brief Verifies that messages below the configured level are suppressed.
 */
TEST(ConsoleLoggerTest, FiltersMessagesBelowThreshold)
{
    ConsoleLogger &logger = ConsoleLogger::instance(LogLevel::kWarning);
    test_utils::CoutCapture output;

    logger.log(LogLevel::kInfo, "filtered message");

    EXPECT_TRUE(output.str().empty());
}

/**
 * @brief Verifies that accepted messages contain their level, text, and newline.
 */
TEST(ConsoleLoggerTest, WritesAcceptedMessagesWithLevelAndNewline)
{
    ConsoleLogger &logger = ConsoleLogger::instance(LogLevel::kWarning);
    ConsoleLogger &same_logger = ConsoleLogger::instance(LogLevel::kError);
    test_utils::CoutCapture output;

    EXPECT_EQ(&same_logger, &logger);
    EXPECT_EQ(same_logger.getLogLevel(), LogLevel::kWarning);

    logger.log(LogLevel::kWarning, "warning message");
    logger.log(LogLevel::kError, "error message");

    const std::string text = output.str();
    EXPECT_NE(text.find("[WARN]"), std::string::npos);
    EXPECT_NE(text.find("warning message"), std::string::npos);
    EXPECT_NE(text.find("[ERROR]"), std::string::npos);
    EXPECT_NE(text.find("error message"), std::string::npos);
    ASSERT_FALSE(text.empty());
    EXPECT_EQ(text.back(), '\n');
}

} // namespace logging
