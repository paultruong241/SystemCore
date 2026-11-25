/*****************************************************************************
 * @file text_formatter.cpp
 * @brief Implements the text log formatter.
 *****************************************************************************/
#include "logging/formatters/text_formatter.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace systemcore::logging
{
    namespace
    {

        std::string formatTimestamp(std::chrono::system_clock::time_point value)
        {
            const auto milliseconds =
                std::chrono::floor<std::chrono::milliseconds>(value);
            const auto seconds =
                std::chrono::floor<std::chrono::seconds>(milliseconds);
            const auto fraction =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    milliseconds - seconds)
                    .count();

            const std::time_t time = std::chrono::system_clock::to_time_t(seconds);
            std::tm calendar_time{};
#if defined(_WIN32)
            gmtime_s(&calendar_time, &time);
#else
            gmtime_r(&time, &calendar_time);
#endif

            std::ostringstream output;
            output << std::put_time(&calendar_time, "%Y-%m-%dT%H:%M:%S")
                   << '.' << std::setfill('0') << std::setw(3) << fraction << 'Z';
            return output.str();
        }

    } // namespace

    std::string TextFormatter::format(const LogRecord &record) const
    {
        std::ostringstream output;
        output << '[' << formatTimestamp(record.timestamp) << ']'
               << '[' << toString(record.level) << ']'
               << "[thread:" << record.thread_id << "] "
               << record.message;
        return output.str();
    }

} // namespace systemcore::logging