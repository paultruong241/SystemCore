/*****************************************************************************
 * @file log_record.hpp
 * @brief Defines the data structure for a log record.
 *****************************************************************************/
#pragma once

#include "log_level.hpp"

#include <chrono>
#include <string>
#include <thread>

namespace systemcore::logging
{

    /**
     * @brief Stores the metadata and message text for a log event.
     */
    struct LogRecord
    {
        /// Severity level of the log event.
        LogLevel level{LogLevel::kInfo};

        /// Time at which the log event was recorded.
        std::chrono::system_clock::time_point timestamp{};

        /// ID of the thread that recorded the log event.
        std::thread::id thread_id{};

        /// Text of the log message.
        std::string message{};
    };

} // namespace systemcore::logging