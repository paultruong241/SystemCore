/*****************************************************************************
 * @file logger.cpp
 * @brief Implements the Logger class.
 *****************************************************************************/

#include "logger.hpp"

#include <chrono>
#include <format>
#include <sstream>
#include <thread>

Logger::Logger(const LogLevel level)
    : log_level_(level)
{
}

LogLevel Logger::getLogLevel() const
{
    return log_level_;
}

std::mutex &Logger::getMutex()
{
    return log_mutex_;
}

std::string Logger::formatTimestamp() const
{
    const auto now = std::chrono::floor<std::chrono::seconds>(
        std::chrono::system_clock::now());

    return std::format("{:%Y-%m-%d %H:%M:%S}", now);
}

std::string Logger::formatThreadId() const
{
    std::ostringstream thread_id;
    thread_id << std::this_thread::get_id();
    return thread_id.str();
}

std::string Logger::formatLogMessage(const LogLevel level, const std::string_view msg) const
{
    return std::format("[{}][{}][{}] {}", formatTimestamp(),
                       logLevelToString(level), formatThreadId(), msg);
}