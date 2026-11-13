/*****************************************************************************
 * @file log_levels.hpp
 * @brief Defines the standard logging levels used by the application.
 *****************************************************************************/
#pragma once

#include <string_view>

/**
 * @brief Represents the severity of a log message.
 *
 * Levels range from kTrace, for the most detailed diagnostic output, to
 * kFatal, for critical failures that prevent the application from continuing
 * safely.
 */
enum class LogLevel
{
    /// Highly detailed diagnostic events for tracing program execution.
    kTrace,

    /// Development-oriented details useful for diagnosing behavior.
    kDebug,

    /// Routine events that confirm normal application operation.
    kInfo,

    /// Unexpected conditions that may need attention but do not prevent
    /// continued operation.
    kWarning,

    /// A failure that prevents an operation from succeeding, but does not
    /// necessarily stop the application.
    kError,

    /// A critical failure after which the application cannot continue safely.
    kFatal
};

/**
 * @brief Converts a LogLevel enum value to its string representation.
 * @param level The LogLevel value to convert.
 * @return A string_view representing the log level.
 */
constexpr std::string_view logLevelToString(const LogLevel level) noexcept
{
    switch (level)
    {
        case LogLevel::kTrace: return "TRACE";
        case LogLevel::kDebug: return "DEBUG";
        case LogLevel::kInfo:  return "INFO";
        case LogLevel::kWarning: return "WARN";
        case LogLevel::kError: return "ERROR";
        case LogLevel::kFatal: return "FATAL";
    }

    return "UNKNOWN";
}
