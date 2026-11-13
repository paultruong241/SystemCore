/*****************************************************************************
 * @file file_logger.cpp
 * @brief Implements the FileLogger class.
 *****************************************************************************/

#include "file_logger.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>

FileLogger::FileLogger(const LogLevel level) :
    Logger(level), filename_(makeDefaultFilename())
{
    if (!openLogFile())
    {
        std::cerr << formatLogMessage(LogLevel::kError, "Failed to open log file: " + filename_) << "\n";
    }
}

void FileLogger::log(const LogLevel level, const std::string_view msg)
{
    if (level < getLogLevel())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(getMutex());

    if (!log_file_.is_open())
    {
        std::cerr << formatLogMessage(LogLevel::kError, "Log file is not open; message was not logged.") << "\n";
        return;
    }

    log_file_ << formatLogMessage(level, msg) << '\n';

    if (log_file_ && level >= LogLevel::kWarning)
    {
        log_file_.flush();
    }

    if (!log_file_)
    {
        std::cerr << formatLogMessage(LogLevel::kError, "Failed to write to log file: " + filename_) << "\n";
    }
}

std::string FileLogger::makeDefaultFilename()
{
    const auto now = std::chrono::system_clock::now();
    return std::format("log_{:%Y%m%d_%H%M%S}.txt", now);
}

bool FileLogger::openLogFile()
{
    std::lock_guard<std::mutex> lock(getMutex());

    if (log_file_.is_open())
    {
        return true;
    }

    log_file_.clear(); // Permit retrying after a previous open failure.
    log_file_.open(filename_, std::ios::out | std::ios::app);
    return log_file_.is_open();
}