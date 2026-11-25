/*****************************************************************************
 * @file log_formatter.hpp
 * @brief Declares the interface for formatting log records.
 *****************************************************************************/
#pragma once

#include "log_record.hpp"

#include <string>

namespace systemcore::logging
{

    /**
     * @brief Formats log records as strings.
     */
    class ILogFormatter
    {
    public:
        /**
         * @brief Destroys the log formatter.
         */
        virtual ~ILogFormatter() = default;

        /**
         * @brief Formats a log record.
         * @param record Record to format.
         * @return The formatted log record.
         *
         * A formatter may be shared and called concurrently. Implementations must
         * support concurrent calls or be externally synchronized.
         */
        virtual std::string format(const LogRecord &record) const = 0;
    };

} // namespace systemcore::logging
