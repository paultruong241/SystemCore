/*****************************************************************************
 * @file logger_test.cpp
 * @brief Tests logger filtering and sink dispatch.
 *****************************************************************************/

#include "logging/logger.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace systemcore::logging
{

namespace
{

class RecordingSink final : public ILogSink
{
public:
    void write(const LogRecord &record) override
    {
        records.push_back(record);
        if (throw_on_write)
        {
            throw std::runtime_error("sink write failed");
        }
    }

    void flush() override
    {
        ++flush_count;
        if (throw_on_flush)
        {
            throw std::runtime_error("sink flush failed");
        }
    }

    std::vector<LogRecord> records;
    std::size_t flush_count{0};
    bool throw_on_write{false};
    bool throw_on_flush{false};
};

} // namespace

TEST(LoggerTest, RejectsEmptyOrNullSinkLists)
{
    std::vector<std::shared_ptr<ILogSink>> no_sinks;
    EXPECT_THROW(Logger(std::move(no_sinks)), std::invalid_argument);

    std::vector<std::shared_ptr<ILogSink>> null_sink{nullptr};
    EXPECT_THROW(Logger(std::move(null_sink)), std::invalid_argument);
}

TEST(LoggerTest, FiltersMessagesAndDispatchesRecordsToEverySink)
{
    auto first_sink = std::make_shared<RecordingSink>();
    auto second_sink = std::make_shared<RecordingSink>();
    Logger logger({first_sink, second_sink}, LogLevel::kWarning);

    logger.info("filtered");
    EXPECT_TRUE(first_sink->records.empty());
    EXPECT_TRUE(second_sink->records.empty());

    logger.warning("accepted");

    ASSERT_EQ(first_sink->records.size(), 1);
    ASSERT_EQ(second_sink->records.size(), 1);
    EXPECT_EQ(first_sink->records.front().level, LogLevel::kWarning);
    EXPECT_EQ(first_sink->records.front().message, "accepted");
    EXPECT_EQ(first_sink->records.front().thread_id, std::this_thread::get_id());
    EXPECT_GT(
        first_sink->records.front().timestamp.time_since_epoch().count(), 0);
    EXPECT_EQ(second_sink->records.front().message, "accepted");
}

TEST(LoggerTest, ConvenienceMethodsUseTheirMatchingSeverity)
{
    auto sink = std::make_shared<RecordingSink>();
    Logger logger({sink}, LogLevel::kTrace);

    logger.trace("trace");
    logger.debug("debug");
    logger.info("info");
    logger.warning("warning");
    logger.error("error");
    logger.fatal("fatal");

    const std::array expected_levels{
        LogLevel::kTrace,
        LogLevel::kDebug,
        LogLevel::kInfo,
        LogLevel::kWarning,
        LogLevel::kError,
        LogLevel::kFatal};
    ASSERT_EQ(sink->records.size(), expected_levels.size());
    for (std::size_t i = 0; i < expected_levels.size(); ++i)
    {
        EXPECT_EQ(sink->records[i].level, expected_levels[i]);
    }
}

TEST(LoggerTest, AppliesUpdatedSeverityThreshold)
{
    auto sink = std::make_shared<RecordingSink>();
    Logger logger({sink}, LogLevel::kError);

    logger.warning("filtered");
    logger.setLevel(LogLevel::kWarning);
    logger.warning("accepted");

    ASSERT_EQ(sink->records.size(), 1);
    EXPECT_EQ(sink->records.front().message, "accepted");
    EXPECT_EQ(logger.level(), LogLevel::kWarning);
}

TEST(LoggerTest, FlushesEverySink)
{
    auto first_sink = std::make_shared<RecordingSink>();
    auto second_sink = std::make_shared<RecordingSink>();
    Logger logger({first_sink, second_sink});

    logger.flush();

    EXPECT_EQ(first_sink->flush_count, 1);
    EXPECT_EQ(second_sink->flush_count, 1);
}

TEST(LoggerTest, PropagatesSinkExceptions)
{
    auto failing_sink = std::make_shared<RecordingSink>();
    auto later_sink = std::make_shared<RecordingSink>();
    failing_sink->throw_on_write = true;
    Logger logger({failing_sink, later_sink});

    EXPECT_THROW(logger.info("failure"), std::runtime_error);
    EXPECT_TRUE(later_sink->records.empty());

    failing_sink->throw_on_write = false;
    failing_sink->throw_on_flush = true;
    EXPECT_THROW(logger.flush(), std::runtime_error);
    EXPECT_EQ(later_sink->flush_count, 0);
}

} // namespace systemcore::logging
