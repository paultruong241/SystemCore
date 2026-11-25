/*****************************************************************************
 * @file json_formatter.hpp
 * @brief Declares a formatter for JSON log records.
 *****************************************************************************/
#pragma once

#include "systemcore/logging/log_formatter.hpp"

namespace systemcore::logging
{

    /**
     * @brief Formats log records as JSON strings.
     */
    class JsonFormatter final : public ILogFormatter
    {
    public:
        /**
         * @brief Formats a log record as JSON.
         * @param record Record to format.
         * @return The log record formatted as a JSON string.
         */
        std::string format(const LogRecord &record) const override;
    };

} // namespace systemcore::logging