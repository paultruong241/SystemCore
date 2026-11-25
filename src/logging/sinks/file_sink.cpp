/*****************************************************************************
 * @file file_sink.cpp
 * @brief Implements file output for log records.
 *****************************************************************************/
#include "systemcore/logging/sinks/file_sink.hpp"

#include <stdexcept>
#include <utility>

namespace systemcore::logging
{
    FileSink::FileSink(
        std::filesystem::path path,
        std::shared_ptr<const ILogFormatter> formatter,
        FlushPolicy flush_policy)
        : path_(std::move(path)),
          formatter_(std::move(formatter)),
          flush_policy_(flush_policy)
    {
        if (path_.empty())
        {
            throw std::invalid_argument("FileSink path must not be empty");
        }
        if (!formatter_)
        {
            throw std::invalid_argument("FileSink requires a formatter");
        }

        stream_.open(path_, std::ios::out | std::ios::app);
        if (!stream_.is_open())
        {
            throw std::ios_base::failure(
                "Unable to open log file: " + path_.string());
        }
    }

    void FileSink::write(const LogRecord &record)
    {
        const std::string line = formatter_->format(record);

        std::lock_guard lock(mutex_);
        stream_ << line << '\n';
        if (!stream_)
        {
            throw std::ios_base::failure(
                "Failed to write log file: " + path_.string());
        }

        const bool flush_now =
            flush_policy_ == FlushPolicy::kEveryRecord ||
            (flush_policy_ == FlushPolicy::kOnWarningOrHigher &&
             record.level >= LogLevel::kWarning);

        if (flush_now)
        {
            stream_.flush();
            if (!stream_)
            {
                throw std::ios_base::failure(
                    "Failed to flush log file: " + path_.string());
            }
        }
    }

    void FileSink::flush()
    {
        std::lock_guard lock(mutex_);
        stream_.flush();
        if (!stream_)
        {
            throw std::ios_base::failure(
                "Failed to flush log file: " + path_.string());
        }
    }

    const std::filesystem::path &FileSink::path() const noexcept
    {
        return path_;
    }

} // namespace systemcore::logging