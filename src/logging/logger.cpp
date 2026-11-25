/*****************************************************************************
 * @file logger.cpp
 * @brief Implements log filtering and sink dispatch.
 *****************************************************************************/
#include "systemcore/logging/logger.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace systemcore::logging
{

    Logger::Logger(
        std::vector<std::shared_ptr<ILogSink>> sinks,
        const LogLevel level)
        : level_(level),
          sinks_(std::move(sinks))
    {
        if (sinks_.empty())
        {
            throw std::invalid_argument("Logger requires at least one sink");
        }
        for (const auto &sink : sinks_)
        {
            if (!sink)
            {
                throw std::invalid_argument("Logger sink list contains nullptr");
            }
        }
    }

    void Logger::setLevel(LogLevel level)
    {
        level_.store(level, std::memory_order_relaxed);
    }

    LogLevel Logger::level() const noexcept
    {
        return level_.load(std::memory_order_relaxed);
    }

    bool Logger::shouldLog(LogLevel message_level) const noexcept
    {
        const auto threshold = level();
        return message_level >= threshold;
    }

    void Logger::log(LogLevel message_level, std::string_view message)
    {
        if (!shouldLog(message_level))
        {
            return;
        }

        const LogRecord record{
            .level = message_level,
            .timestamp = std::chrono::system_clock::now(),
            .thread_id = std::this_thread::get_id(),
            .message = std::string(message)};

        for (const auto &sink : sinks_)
        {
            sink->write(record);
        }
    }

    void Logger::trace(std::string_view message)
    {
        log(LogLevel::kTrace, message);
    }

    void Logger::debug(std::string_view message)
    {
        log(LogLevel::kDebug, message);
    }

    void Logger::info(std::string_view message)
    {
        log(LogLevel::kInfo, message);
    }

    void Logger::warning(std::string_view message)
    {
        log(LogLevel::kWarning, message);
    }

    void Logger::error(std::string_view message)
    {
        log(LogLevel::kError, message);
    }

    void Logger::fatal(std::string_view message)
    {
        log(LogLevel::kFatal, message);
    }

    void Logger::flush()
    {
        for (const auto &sink : sinks_)
        {
            sink->flush();
        }
    }

} // namespace systemcore::logging