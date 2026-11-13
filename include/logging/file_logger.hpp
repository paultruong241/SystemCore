/*****************************************************************************
 * @file file_logger.hpp
 * @brief Declares the FileLogger class.
 *****************************************************************************/
#pragma once

#include "logger.hpp"

#include <fstream>
#include <string>
#include <string_view>

/**
 * @brief Writes formatted log messages to a timestamp-named file.
 *
 * The logger creates its file in append mode and writes messages whose
 * severity meets or exceeds the configured threshold. Warning and more severe
 * messages are flushed immediately.
 */
class FileLogger : public Logger
{
public:
    /**
     * @brief Constructs a logger and attempts to open its timestamp-named file.
     * @param level Minimum severity of messages to write.
     *
     * If the file cannot be opened, the failure is reported to standard error.
     */
    explicit FileLogger(const LogLevel level = LogLevel::kInfo);

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
    /// The name of the log file, generated at construction.
    const std::string filename_;

    /// Output stream for the generated log file.
    std::ofstream log_file_;

    /**
     * @brief Creates a log filename using the current date and time.
     * @return The generated filename.
     */
    static std::string makeDefaultFilename();

    /**
     * @brief Opens the generated log file in append mode.
     * @return True if the file is open; otherwise, false.
     */
    bool openLogFile();
};
