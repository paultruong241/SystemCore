/*****************************************************************************
 * @file console_logger.cpp
 * @brief Implements the ConsoleLogger class.
 *****************************************************************************/

#include "console_logger.hpp"

#include <iostream>

ConsoleLogger::ConsoleLogger(const LogLevel level) : Logger(level)
{
    // Do nothing
}

void ConsoleLogger::log(const LogLevel level, const std::string_view msg)
{
    if (level < getLogLevel())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(getMutex());

    std::cout << formatLogMessage(level, msg) << '\n';

    if (level >= LogLevel::kWarning)
    {
        std::cout.flush();
    }
}