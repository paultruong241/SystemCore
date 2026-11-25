/*****************************************************************************
 * @file log_sink.hpp
 * @brief Declares the interface for log record sinks.
 *****************************************************************************/
#pragma once

#include "log_record.hpp"

namespace systemcore::logging
{

    /**
     * @brief Interface for writing and flushing log records.
     *
     * Derived sinks provide the destination-specific record-writing and
     * flushing behavior.
     */
    class ILogSink
    {
    public:
        /**
         * @brief Constructs a log sink.
         */
        ILogSink() = default;

        /**
         * @brief Destroys the log sink.
         */
        virtual ~ILogSink() = default;

        /// Copy construction is disabled.
        ILogSink(const ILogSink &) = delete;

        /// Copy assignment is disabled.
        ILogSink &operator=(const ILogSink &) = delete;

        /// Move construction is disabled.
        ILogSink(ILogSink &&) = delete;

        /// Move assignment is disabled.
        ILogSink &operator=(ILogSink &&) = delete;

        /**
         * @brief Writes a log record to the sink.
         * @param record Record to write.
         *
         * A logger may call this function concurrently. Implementations must
         * synchronize access to mutable state when shared across threads.
         */
        virtual void write(const LogRecord &record) = 0;

        /**
         * @brief Flushes any buffered data to the sink's destination.
         */
        virtual void flush() = 0;
    };

} // namespace systemcore::logging