/*****************************************************************************
 * @file json_formatter.cpp
 * @brief Implements the JSON log formatter.
 *****************************************************************************/
#include "logging/formatters/json_formatter.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

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

        std::string escapeJson(std::string_view input)
        {
            constexpr char hex[] = "0123456789abcdef";
            std::string output;
            output.reserve(input.size() + 2);
            for (const unsigned char ch : input)
            {
                switch (ch)
                {
                case '"':
                    output += "\\\"";
                    break;
                case '\\':
                    output += "\\\\";
                    break;
                case '\b':
                    output += "\\b";
                    break;
                case '\f':
                    output += "\\f";
                    break;
                case '\n':
                    output += "\\n";
                    break;
                case '\r':
                    output += "\\r";
                    break;
                case '\t':
                    output += "\\t";
                    break;
                default:
                    if (ch < 0x20)
                    {
                        output += "\\u00";
                        output += hex[(ch >> 4) & 0x0f];
                        output += hex[ch & 0x0f];
                    }
                    else
                    {
                        output += static_cast<char>(ch);
                    }
                }
            }
            return output;
        }

    } // namespace

    std::string JsonFormatter::format(const LogRecord &record) const
    {
        std::ostringstream thread_id;
        thread_id << record.thread_id;

        std::ostringstream output;
        output << "{\"timestamp\":\"" << formatTimestamp(record.timestamp)
               << "\",\"level\":\"" << toString(record.level)
               << "\",\"thread_id\":\"" << escapeJson(thread_id.str())
               << "\",\"message\":\"" << escapeJson(record.message) << "\"}";
        return output.str();
    }

} // namespace systemcore::logging