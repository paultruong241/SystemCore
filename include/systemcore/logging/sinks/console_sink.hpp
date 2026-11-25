/*****************************************************************************
 * @file console_sink.hpp
 * @brief Declares a sink for formatted console output.
 *****************************************************************************/
#pragma once

#include "systemcore/logging/log_formatter.hpp"
#include "systemcore/logging/log_sink.hpp"

#include <memory>

namespace systemcore::logging
{

    /**
     * @brief Writes formatted log records to the console.
     */
    class ConsoleSink final : public ILogSink
    {
    public:
        /**
         * @brief Constructs a console sink with a record formatter.
         * @param formatter Formatter used to format log records.
         * @throws std::invalid_argument If @p formatter is null.
         */
        explicit ConsoleSink(std::shared_ptr<const ILogFormatter> formatter);

        /**
         * @brief Writes warning and higher severity records to standard error;
         *        other records go to standard output.
         * @param record Record to write.
         * @throws std::ios_base::failure If the record cannot be written.
         */
        void write(const LogRecord &record) override;

        /**
         * @brief Flushes standard output and standard error.
         * @throws std::ios_base::failure If either stream cannot be flushed.
         */
        void flush() override;

    private:
        /// Formatter used to convert records to text.
        std::shared_ptr<const ILogFormatter> formatter_;
    };

} // namespace systemcore::logging
