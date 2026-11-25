/*****************************************************************************
 * @file file_logger.hpp
 * @brief Declares the FileLogger class.
 *****************************************************************************/
#pragma once

#include "logger.hpp"

#include <fstream>
#include <string>
#include <string_view>

namespace logging
{

/**
 * @brief Writes formatted log messages to a file.
 *
 * The logger uses a timestamp-named file by default or a caller-supplied
 * filename. It writes accepted messages in append mode and flushes warnings
 * and more severe messages immediately.
 */
class FileLogger : public Logger
{
public:
    /**
     * @brief Constructs a logger and opens its log file.
     * @param level Minimum severity of messages to write.
     * @param file_name Log filename; defaults to a timestamp-based name.
     *
     * If the file cannot be opened, the failure is reported to standard error.
     */
    explicit FileLogger(
        const LogLevel level = LogLevel::kInfo,
        std::string_view file_name = makeDefaultFilename());

    /**
     * @brief Writes a message if its severity meets the configured threshold.
     * @param level Severity of the message.
     * @param msg Message text.
     *
     * Warning, error, and fatal messages are flushed immediately. If the file
     * is unavailable or a write fails, the problem is reported to standard
     * error.
     */
    void log(const LogLevel level, const std::string_view msg) override;

private:
    /// Name of the log file used by this logger.
    const std::string filename_;

    /// Output stream for the generated log file.
    std::ofstream log_file_;

    /**
     * @brief Generates a log filename using the current date and time.
     * @return The generated filename.
     */
    static std::string makeDefaultFilename();

    /**
     * @brief Opens the configured log file in append mode.
     * @return True if the file is open; otherwise, false.
     */
    bool openLogFile();
};

} // namespace logging
