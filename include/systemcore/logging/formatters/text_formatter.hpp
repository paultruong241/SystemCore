/*****************************************************************************
 * @file text_formatter.hpp
 * @brief Defines the text formatter for log records.
 *****************************************************************************/
#pragma once

#include "logging/log_formatter.hpp"

namespace systemcore::logging
{

    /**
     * @brief Formats log records as text strings.
     */
    class TextFormatter final : public ILogFormatter
    {
    public:
        /**
         * @brief Formats a log record as text.
         * @param record Record to format.
         * @return The log record formatted as a text string.
         */
        std::string format(const LogRecord &record) const override;
    };

} // namespace systemcore::logging