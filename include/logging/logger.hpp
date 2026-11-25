/*****************************************************************************
 * @file logger.hpp
 * @brief Declares the Logger class.
 *****************************************************************************/
#pragma once

#include "logging/log_level.hpp"
#include "logging/log_sink.hpp"

#include <atomic>
#include <memory>
#include <string_view>
#include <vector>

namespace systemcore::logging
{

    /**
     * @brief Routes log messages to configured sinks after filtering by
     *        severity.
     *
     * Each accepted message is recorded with the current time and thread ID.
     */
    class Logger final
    {
    public:
        /**
         * @brief Constructs a logger with the specified sinks and severity
         *        level.
         * @param sinks Sinks that receive accepted log records.
         * @param level Minimum severity level to write.
         * @throws std::invalid_argument If @p sinks is empty or contains null.
         */
        explicit Logger(
            std::vector<std::shared_ptr<ILogSink>> sinks,
            const LogLevel level = LogLevel::kInfo);

        /// Copy construction is disabled.
        Logger(const Logger &) = delete;

        /// Copy assignment is disabled.
        Logger &operator=(const Logger &) = delete;

        /// Move construction is disabled.
        Logger(Logger &&) = delete;

        /// Move assignment is disabled.
        Logger &operator=(Logger &&) = delete;

        /**
         * @brief Sets the minimum severity level written by the logger.
         * @param level New minimum severity level.
         */
        void setLevel(LogLevel level);

        /**
         * @brief Returns the current minimum severity level.
         * @return The current severity threshold.
         */
        LogLevel level() const noexcept;

        /**
         * @brief Writes a message if its severity meets the current threshold.
         * @param level Severity of the message.
         * @param message Message text.
         *
         * Exceptions thrown by a sink are propagated to the caller. If a sink
         * throws, later sinks do not receive the record.
         */
        void log(LogLevel level, std::string_view message);

        /**
         * @brief Writes a trace-level message.
         * @param message Message text.
         */
        void trace(std::string_view message);

        /**
         * @brief Writes a debug-level message.
         * @param message Message text.
         */
        void debug(std::string_view message);

        /**
         * @brief Writes an informational message.
         * @param message Message text.
         */
        void info(std::string_view message);

        /**
         * @brief Writes a warning-level message.
         * @param message Message text.
         */
        void warning(std::string_view message);

        /**
         * @brief Writes an error-level message.
         * @param message Message text.
         */
        void error(std::string_view message);

        /**
         * @brief Writes a fatal-level message.
         * @param message Message text.
         */
        void fatal(std::string_view message);

        /**
         * @brief Flushes each configured sink.
         *
         * Exceptions thrown by a sink are propagated to the caller.
         */
        void flush();

    private:
        /// Returns whether a message meets the current severity threshold.
        bool shouldLog(LogLevel message_level) const noexcept;

        /// Minimum severity level, which may be changed concurrently.
        std::atomic<LogLevel> level_;

        /// Sinks registered at construction; this list is not modified
        /// afterward.
        std::vector<std::shared_ptr<ILogSink>> sinks_;
    };

} // namespace systemcore::logging