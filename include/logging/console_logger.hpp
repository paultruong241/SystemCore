/*****************************************************************************
 * @file console_logger.hpp
 * @brief Declares the ConsoleLogger class.
 *****************************************************************************/
#pragma once

#include "logger.hpp"

#include <string_view>

/**
 * @brief Writes formatted log messages to the console.
 *
 * The logger writes messages whose severity meets or exceeds the configured
 * threshold. Warning and more severe messages are flushed immediately.
 */
class ConsoleLogger : public Logger
{
public:
    /**
     * @brief Returns the process-wide console logger.
     * @param level Threshold used when the singleton is first created. Later calls
     * return the same logger and ignore this argument.
     * @return Reference to the console logger.
     */
    static ConsoleLogger &instance(LogLevel level = LogLevel::kInfo);

    /**
     * @brief Writes a message if its severity meets the configured threshold.
     * @param level Severity of the message.
     * @param msg Message text.
     */
    void log(const LogLevel level, const std::string_view msg) override;

private:
    /**
     * @brief Constructs the console logger with a severity threshold.
     * @param level Minimum severity of messages to write.
     */
    explicit ConsoleLogger(const LogLevel level);
};
