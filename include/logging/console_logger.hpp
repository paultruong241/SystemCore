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
     * @brief Constructs a console logger with a severity threshold.
     * @param level Minimum severity of messages to write.
     */
    explicit ConsoleLogger(const LogLevel level = LogLevel::kInfo);

    /**
     * @brief Writes a message if its severity meets the configured threshold.
     * @param level Severity of the message.
     * @param msg Message text.
     */
    void log(const LogLevel level, const std::string_view msg) override;
};
