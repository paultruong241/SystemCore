/*****************************************************************************
 * @file console_sink.cpp
 * @brief Implements console output for log records.
 *****************************************************************************/
#include "systemcore/logging/sinks/console_sink.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace systemcore::logging
{
    namespace
    {
        // A shared mutex prevents interleaving between console sink instances.
        std::mutex &consoleMutex()
        {
            static std::mutex mutex;
            return mutex;
        }
    } // namespace

    ConsoleSink::ConsoleSink(
        std::shared_ptr<const ILogFormatter> formatter)
        : formatter_(std::move(formatter))
    {
        if (!formatter_)
        {
            throw std::invalid_argument("ConsoleSink requires a formatter");
        }
    }

    void ConsoleSink::write(const LogRecord &record)
    {
        const std::string line = formatter_->format(record);
        std::lock_guard lock(consoleMutex());

        std::ostream &output =
            record.level >= LogLevel::kWarning ? std::cerr : std::cout;
        output << line << '\n';

        if (record.level >= LogLevel::kWarning)
        {
            output.flush();
        }

        if (!output)
        {
            throw std::ios_base::failure("Failed to write log record to console");
        }
    }

    void ConsoleSink::flush()
    {
        std::lock_guard lock(consoleMutex());
        std::cout.flush();
        std::cerr.flush();
        if (!std::cout || !std::cerr)
        {
            throw std::ios_base::failure("Failed to flush console log output");
        }
    }

} // namespace systemcore::logging