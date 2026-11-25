/*****************************************************************************
 * @file bounded_queue_test.cpp
 * @brief Tests capacity, blocking lifecycle operations, and concurrent access.
 *****************************************************************************/

#include "systemcore/concurrency/bounded_queue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <future>
#include <numeric>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace systemcore::concurrency
{

TEST(BoundedQueueTest, WaitPushInsertsWhenCapacityIsAvailable)
{
    BoundedQueue<int> queue(1);

    EXPECT_EQ(
        queue.waitPush(42, std::stop_token{}), QueueStatus::kSuccess);

    const auto value = queue.waitPop(std::stop_token{});
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(*value.value, 42);
}

TEST(BoundedQueueTest, WaitPushReturnsStoppedWhenCancelledAndFull)
{
    BoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(42), QueueStatus::kSuccess);

    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        queue.waitPush(7, stop_source.get_token()), QueueStatus::kStopped);
    EXPECT_FALSE(queue.isClosed());

    const auto value = queue.waitPop(std::stop_token{});
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(*value.value, 42);
}

TEST(BoundedQueueTest, WaitPushReturnsStoppedWhenCancelledWhileWaiting)
{
    BoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(42), QueueStatus::kSuccess);

    std::stop_source stop_source;
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<QueueStatus> result;
    auto result_future = result.get_future();

    std::thread producer([&] {
        started.set_value();
        result.set_value(queue.waitPush(7, stop_source.get_token()));
    });

    started_future.wait();
    EXPECT_EQ(
        result_future.wait_for(std::chrono::milliseconds(20)),
        std::future_status::timeout);
    stop_source.request_stop();

    if (result_future.wait_for(std::chrono::seconds(1)) !=
        std::future_status::ready)
    {
        queue.close();
    }

    producer.join();
    EXPECT_EQ(result_future.get(), QueueStatus::kStopped);
}

TEST(BoundedQueueTest, TryPushAndTryPopPreserveFifoAndCapacity)
{
    BoundedQueue<std::string> queue(2);

    EXPECT_EQ(queue.capacity(), 2);
    EXPECT_TRUE(queue.empty());
    ASSERT_EQ(queue.tryPush("first"), QueueStatus::kSuccess);
    ASSERT_EQ(queue.tryPush("second"), QueueStatus::kSuccess);
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.tryPush("third"), QueueStatus::kFull);

    auto first = queue.tryPop();
    ASSERT_EQ(first.status, QueueStatus::kSuccess);
    ASSERT_TRUE(first.value.has_value());
    EXPECT_EQ(*first.value, "first");
    EXPECT_FALSE(queue.full());

    ASSERT_EQ(queue.tryPush("third"), QueueStatus::kSuccess);
    auto second = queue.tryPop();
    auto third = queue.tryPop();
    ASSERT_EQ(second.status, QueueStatus::kSuccess);
    ASSERT_EQ(third.status, QueueStatus::kSuccess);
    ASSERT_TRUE(second.value.has_value());
    ASSERT_TRUE(third.value.has_value());
    EXPECT_EQ(*second.value, "second");
    EXPECT_EQ(*third.value, "third");
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.tryPop().status, QueueStatus::kEmpty);
}

TEST(BoundedQueueTest, WaitPopReturnsStoppedWhenCancelled)
{
    BoundedQueue<int> queue(1);
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        queue.waitPop(stop_source.get_token()).status, QueueStatus::kStopped);
    EXPECT_FALSE(queue.isClosed());
}

TEST(BoundedQueueTest, WaitPopReturnsStoppedWhenCancelledWhileWaiting)
{
    BoundedQueue<int> queue(1);
    std::stop_source stop_source;
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<QueueResult<int>> result;
    auto result_future = result.get_future();

    std::thread consumer([&] {
        started.set_value();
        result.set_value(queue.waitPop(stop_source.get_token()));
    });

    started_future.wait();
    EXPECT_EQ(
        result_future.wait_for(std::chrono::milliseconds(20)),
        std::future_status::timeout);
    stop_source.request_stop();

    if (result_future.wait_for(std::chrono::seconds(1)) !=
        std::future_status::ready)
    {
        queue.close();
    }

    consumer.join();
    EXPECT_EQ(result_future.get().status, QueueStatus::kStopped);
}

TEST(BoundedQueueTest, WaitPopUnblocksAfterItemIsInserted)
{
    BoundedQueue<int> queue(1);
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<QueueResult<int>> result;
    auto result_future = result.get_future();

    std::thread consumer([&] {
        started.set_value();
        result.set_value(queue.waitPop(std::stop_token{}));
    });

    started_future.wait();
    EXPECT_EQ(
        result_future.wait_for(std::chrono::milliseconds(20)),
        std::future_status::timeout);
    const auto push_status = queue.tryPush(42);
    EXPECT_EQ(push_status, QueueStatus::kSuccess);
    if (push_status != QueueStatus::kSuccess)
    {
        queue.close();
    }

    const bool completed_after_insert =
        result_future.wait_for(std::chrono::seconds(1)) ==
        std::future_status::ready;
    if (!completed_after_insert)
    {
        queue.close();
    }

    consumer.join();
    const auto value = result_future.get();
    EXPECT_TRUE(completed_after_insert);
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(*value.value, 42);
}

TEST(BoundedQueueTest, WaitPushUnblocksAfterCapacityBecomesAvailable)
{
    BoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(1), QueueStatus::kSuccess);
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<QueueStatus> result;
    auto result_future = result.get_future();

    std::thread producer([&] {
        started.set_value();
        result.set_value(queue.waitPush(2, std::stop_token{}));
    });

    started_future.wait();
    EXPECT_EQ(
        result_future.wait_for(std::chrono::milliseconds(20)),
        std::future_status::timeout);
    const auto first = queue.tryPop();
    EXPECT_EQ(first.status, QueueStatus::kSuccess);
    if (first.status != QueueStatus::kSuccess || !first.value)
    {
        queue.close();
    }

    const bool completed_after_pop =
        result_future.wait_for(std::chrono::seconds(1)) ==
        std::future_status::ready;
    if (!completed_after_pop)
    {
        queue.close();
    }

    producer.join();
    EXPECT_TRUE(completed_after_pop);
    EXPECT_EQ(result_future.get(), QueueStatus::kSuccess);
    ASSERT_EQ(first.status, QueueStatus::kSuccess);
    ASSERT_TRUE(first.value.has_value());
    EXPECT_EQ(*first.value, 1);
    const auto second = queue.tryPop();
    ASSERT_EQ(second.status, QueueStatus::kSuccess);
    ASSERT_TRUE(second.value.has_value());
    EXPECT_EQ(*second.value, 2);
}

TEST(BoundedQueueTest, CloseRejectsPushesAndDrainsExistingValues)
{
    BoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(1), QueueStatus::kSuccess);

    queue.close();

    EXPECT_TRUE(queue.isClosed());
    EXPECT_EQ(queue.tryPush(2), QueueStatus::kClosed);
    EXPECT_EQ(
        queue.waitPush(2, std::stop_token{}), QueueStatus::kClosed);

    const auto existing = queue.waitPop(std::stop_token{});
    ASSERT_EQ(existing.status, QueueStatus::kSuccess);
    ASSERT_TRUE(existing.value.has_value());
    EXPECT_EQ(*existing.value, 1);
    EXPECT_EQ(queue.waitPop(std::stop_token{}).status, QueueStatus::kClosed);
    EXPECT_EQ(queue.tryPop().status, QueueStatus::kClosed);
}

TEST(BoundedQueueTest, CloseReleasesWaitingProducer)
{
    BoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(1), QueueStatus::kSuccess);
    QueueStatus result = QueueStatus::kSuccess;

    std::thread producer([&] {
        result = queue.waitPush(2, std::stop_token{});
    });

    queue.close();
    producer.join();

    EXPECT_EQ(result, QueueStatus::kClosed);
    EXPECT_TRUE(queue.isClosed());
}

TEST(BoundedQueueTest, CloseReleasesWaitingConsumer)
{
    BoundedQueue<int> queue(1);
    QueueStatus result = QueueStatus::kSuccess;

    std::thread consumer([&] {
        result = queue.waitPop(std::stop_token{}).status;
    });

    queue.close();
    consumer.join();

    EXPECT_EQ(result, QueueStatus::kClosed);
}

TEST(BoundedQueueTest, HandlesConcurrentProducersAndConsumers)
{
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t consumer_count = 4;
    constexpr std::size_t items_per_producer = 1'000;
    constexpr std::size_t items_per_consumer =
        producer_count * items_per_producer / consumer_count;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    BoundedQueue<int> queue(8);
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
                (void)queue.waitPush(
                    static_cast<int>(first_value + i), std::stop_token{});
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
