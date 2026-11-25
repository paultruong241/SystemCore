/*****************************************************************************
 * @file queue_test.cpp
 * @brief Tests queue insertion, retrieval, cancellation, and concurrent access.
 *****************************************************************************/

#include "systemcore/concurrency/queue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace systemcore::concurrency
{

TEST(QueueTest, PushSupportsCopiesMovesAndFifoRetrieval)
{
    Queue<std::string> queue;
    const std::string first = "first";
    std::string second = "second";

    EXPECT_EQ(queue.push(first), QueueStatus::kSuccess);
    EXPECT_EQ(queue.push(std::move(second)), QueueStatus::kSuccess);

    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.isEmpty());

    auto first_value = queue.tryPop();
    auto second_value = queue.waitPop(std::stop_token{});
    ASSERT_EQ(first_value.status, QueueStatus::kSuccess);
    ASSERT_EQ(second_value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(first_value.value.has_value());
    ASSERT_TRUE(second_value.value.has_value());
    EXPECT_EQ(*first_value.value, "first");
    EXPECT_EQ(*second_value.value, "second");
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.tryPop().status, QueueStatus::kEmpty);
}

TEST(QueueTest, SupportsMoveOnlyValues)
{
    Queue<std::unique_ptr<int>> queue;
    ASSERT_EQ(queue.push(std::make_unique<int>(42)), QueueStatus::kSuccess);

    auto value = queue.tryPop();
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    ASSERT_NE(*value.value, nullptr);
    EXPECT_EQ(**value.value, 42);
}

TEST(QueueTest, WaitPopReturnsStoppedWhenCancelled)
{
    Queue<int> queue;
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        queue.waitPop(stop_source.get_token()).status, QueueStatus::kStopped);
    EXPECT_FALSE(queue.isClosed());
    EXPECT_EQ(queue.push(42), QueueStatus::kSuccess);
}

TEST(QueueTest, CloseRejectsPushesAndDrainsExistingValues)
{
    Queue<int> queue;
    ASSERT_EQ(queue.push(1), QueueStatus::kSuccess);
    ASSERT_EQ(queue.push(2), QueueStatus::kSuccess);

    queue.close();
    queue.close();

    EXPECT_TRUE(queue.isClosed());
    EXPECT_EQ(queue.push(3), QueueStatus::kClosed);
    EXPECT_EQ(queue.push(4), QueueStatus::kClosed);

    const auto first = queue.waitPop(std::stop_token{});
    const auto second = queue.waitPop(std::stop_token{});
    ASSERT_EQ(first.status, QueueStatus::kSuccess);
    ASSERT_EQ(second.status, QueueStatus::kSuccess);
    ASSERT_TRUE(first.value.has_value());
    ASSERT_TRUE(second.value.has_value());
    EXPECT_EQ(*first.value, 1);
    EXPECT_EQ(*second.value, 2);
    EXPECT_EQ(queue.waitPop(std::stop_token{}).status, QueueStatus::kClosed);
    EXPECT_EQ(queue.tryPop().status, QueueStatus::kClosed);
}

TEST(QueueTest, CloseReleasesWaitingConsumer)
{
    Queue<int> queue;
    QueueStatus result = QueueStatus::kSuccess;

    std::thread consumer([&] {
        result = queue.waitPop(std::stop_token{}).status;
    });

    queue.close();
    consumer.join();

    EXPECT_EQ(result, QueueStatus::kClosed);
}

TEST(QueueTest, HandlesConcurrentProducersAndConsumers)
{
    constexpr std::size_t producer_count = 2;
    constexpr std::size_t consumer_count = 2;
    constexpr std::size_t items_per_producer = 100;
    constexpr std::size_t items_per_consumer =
        producer_count * items_per_producer / consumer_count;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    Queue<int> queue;
    std::vector<std::vector<int>> consumed(consumer_count);
    std::vector<std::thread> consumers;

    for (std::size_t consumer_id = 0; consumer_id < consumer_count; ++consumer_id)
    {
        consumers.emplace_back([&, consumer_id] {
            auto &values = consumed[consumer_id];
            values.reserve(items_per_consumer);
            for (std::size_t i = 0; i < items_per_consumer; ++i)
            {
                const auto result = queue.waitPop(std::stop_token{});
                if (result.status == QueueStatus::kSuccess && result.value)
                {
                    values.push_back(*result.value);
                }
            }
        });
    }

    std::vector<std::thread> producers;
    for (std::size_t producer_id = 0; producer_id < producer_count; ++producer_id)
    {
        producers.emplace_back([&, producer_id] {
            const auto first_value = producer_id * items_per_producer;
            for (std::size_t i = 0; i < items_per_producer; ++i)
            {
                queue.push(static_cast<int>(first_value + i));
            }
        });
    }

    for (auto &producer : producers)
    {
        producer.join();
    }
    for (auto &consumer : consumers)
    {
        consumer.join();
    }

    std::vector<int> actual_values;
    actual_values.reserve(total_items);
    for (const auto &values : consumed)
    {
        actual_values.insert(actual_values.end(), values.begin(), values.end());
    }
    std::sort(actual_values.begin(), actual_values.end());

    std::vector<int> expected_values(total_items);
    std::iota(expected_values.begin(), expected_values.end(), 0);
    EXPECT_EQ(actual_values, expected_values);
}

} // namespace systemcore::concurrency
