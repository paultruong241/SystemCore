/*****************************************************************************
 * @file logger.hpp
 * @brief Declares the Logger class.
 *****************************************************************************/
#pragma once

#include "log_levels.hpp"

#include <mutex>
#include <string>
#include <string_view>

/**
 * @brief Base class for loggers.
 *
 * Stores the severity threshold and synchronization mutex shared by concrete
 * logger implementations, and provides helpers for formatting log entries.
 */
class Logger
{
public:
    /**
     * @brief Constructs a logger with a severity threshold.
     * @param level Minimum severity accepted by the logger.
     */
    explicit Logger(const LogLevel level = LogLevel::kInfo);

    /**
     * @brief Destroys the logger.
     */
    virtual ~Logger() = default;

    /// Copying a logger is disabled.
    Logger(const Logger &) = delete;

    /// Copy assignment for a logger is disabled.
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief Returns the logger's severity threshold.
     * @return The configured severity level.
     */
    LogLevel getLogLevel() const;

    /**
     * @brief Writes a message according to the logger's severity threshold.
     * @param level Severity of the message to write.
     * @param msg Text of the message to write.
     *
     * Concrete implementations define the output destination and should omit
     * messages less severe than the configured threshold.
     */
    virtual void log(const LogLevel level, const std::string_view msg) = 0;

protected:
    /**
     * @brief Returns the mutex used to synchronize log output.
     * @return A reference to the logger's mutex.
     */
    std::mutex &getMutex();

    /**
     * @brief Formats the current time to whole-second precision.
     * @return The timestamp formatted as @c YYYY-MM-DD HH:MM:SS.
     */
    std::string formatTimestamp() const;

    /**
     * @brief Formats the calling thread's ID.
     * @return The thread ID as a string.
     */
    std::string formatThreadId() const;

    /**
     * @brief Formats a log entry with its timestamp, severity, thread ID, and
     * message.
     * @param level Severity of the log entry.
     * @param msg Message text.
     * @return The formatted entry in the form @c [timestamp][level][thread-id] message.
     */
    std::string formatLogMessage(const LogLevel level, const std::string_view msg) const;

private:
    /// Minimum severity level accepted by the logger.
    const LogLevel log_level_{LogLevel::kInfo};

    /// Synchronizes access to log output.
    std::mutex log_mutex_{};
};
