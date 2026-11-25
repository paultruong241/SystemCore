/*****************************************************************************
 * @file file_sink.hpp
 * @brief Declares a sink for formatted file output and its flush policies.
 *****************************************************************************/
#pragma once

#include "systemcore/logging/log_formatter.hpp"
#include "systemcore/logging/log_sink.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>

namespace systemcore::logging
{

    /**
     * @brief Defines when a file sink flushes its output.
     */
    enum class FlushPolicy
    {
        /// Flush only when flush() is called.
        kManual,

        /// Flush records at warning severity or higher.
        kOnWarningOrHigher,

        /// Flush after every record.
        kEveryRecord
    };

    /**
     * @brief Writes formatted log records to a file.
     */
    class FileSink final : public ILogSink
    {
    public:
        /**
         * @brief Opens a log file in append mode and configures its formatter
         *        and flush policy.
         * @param path Path to the log file.
         * @param formatter Formatter used to format log records.
         * @param flush_policy Policy that controls when output is flushed.
         * @throws std::invalid_argument If @p path is empty or @p formatter is
         *         null.
         * @throws std::ios_base::failure If the log file cannot be opened.
         */
        explicit FileSink(
            std::filesystem::path path,
            std::shared_ptr<const ILogFormatter> formatter,
            FlushPolicy flush_policy = FlushPolicy::kOnWarningOrHigher);

        /**
         * @brief Writes a formatted record to the log file.
         * @param record Record to write.
         * @throws std::ios_base::failure If writing or flushing fails.
         */
        void write(const LogRecord &record) override;

        /**
         * @brief Flushes the log file.
         * @throws std::ios_base::failure If the log file cannot be flushed.
         */
        void flush() override;

        /**
         * @brief Returns the path of the log file.
         * @return A reference to the configured log file path.
         */
        const std::filesystem::path &path() const noexcept;

    private:
        /// Path of the log file.
        std::filesystem::path path_;

        /// Formatter used to convert records to text.
        std::shared_ptr<const ILogFormatter> formatter_;

        /// Policy that controls when the stream is flushed.
        FlushPolicy flush_policy_;

        /// Output stream for the log file.
        std::ofstream stream_;

        /// Protects access to the file stream.
        std::mutex mutex_;
    };

} // namespace systemcore::logging
