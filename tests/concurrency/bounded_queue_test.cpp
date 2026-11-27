/*****************************************************************************
 * @file bounded_queue_test.cpp
 * @brief Tests capacity, blocking lifecycle operations, and concurrent access.
 *****************************************************************************/

#include "systemcore/concurrency/containers/concurrent_bounded_queue.hpp"

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

namespace systemcore::concurrency::containers
{

TEST(ConcurrentBoundedQueueTest, WaitPushInsertsWhenCapacityIsAvailable)
{
    ConcurrentBoundedQueue<int> queue(1);

    EXPECT_EQ(
        queue.waitPush(42, std::stop_token{}), ContainerStatus::kSuccess);

    const auto value = queue.waitPop(std::stop_token{});
    ASSERT_EQ(value.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(*value.value, 42);
}

TEST(ConcurrentBoundedQueueTest, WaitPushReturnsStoppedWhenCancelledAndFull)
{
    ConcurrentBoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(42), ContainerStatus::kSuccess);

    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        queue.waitPush(7, stop_source.get_token()), ContainerStatus::kStopped);
    EXPECT_FALSE(queue.isClosed());

    const auto value = queue.waitPop(std::stop_token{});
    ASSERT_EQ(value.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(*value.value, 42);
}

TEST(ConcurrentBoundedQueueTest, WaitPushReturnsStoppedWhenCancelledWhileWaiting)
{
    ConcurrentBoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(42), ContainerStatus::kSuccess);

    std::stop_source stop_source;
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<ContainerStatus> result;
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
    EXPECT_EQ(result_future.get(), ContainerStatus::kStopped);
}

TEST(ConcurrentBoundedQueueTest, TryPushAndTryPopPreserveFifoAndCapacity)
{
    ConcurrentBoundedQueue<std::string> queue(2);

    EXPECT_EQ(queue.capacity(), 2);
    EXPECT_TRUE(queue.empty());
    ASSERT_EQ(queue.tryPush("first"), ContainerStatus::kSuccess);
    ASSERT_EQ(queue.tryPush("second"), ContainerStatus::kSuccess);
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.tryPush("third"), ContainerStatus::kFull);

    auto first = queue.tryPop();
    ASSERT_EQ(first.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(first.value.has_value());
    EXPECT_EQ(*first.value, "first");
    EXPECT_FALSE(queue.full());

    ASSERT_EQ(queue.tryPush("third"), ContainerStatus::kSuccess);
    auto second = queue.tryPop();
    auto third = queue.tryPop();
    ASSERT_EQ(second.status, ContainerStatus::kSuccess);
    ASSERT_EQ(third.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(second.value.has_value());
    ASSERT_TRUE(third.value.has_value());
    EXPECT_EQ(*second.value, "second");
    EXPECT_EQ(*third.value, "third");
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.tryPop().status, ContainerStatus::kEmpty);
}

TEST(ConcurrentBoundedQueueTest, WaitPopReturnsStoppedWhenCancelled)
{
    ConcurrentBoundedQueue<int> queue(1);
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        queue.waitPop(stop_source.get_token()).status, ContainerStatus::kStopped);
    EXPECT_FALSE(queue.isClosed());
}

TEST(ConcurrentBoundedQueueTest, WaitPopReturnsStoppedWhenCancelledWhileWaiting)
{
    ConcurrentBoundedQueue<int> queue(1);
    std::stop_source stop_source;
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<PopResult<int>> result;
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
    EXPECT_EQ(result_future.get().status, ContainerStatus::kStopped);
}

TEST(ConcurrentBoundedQueueTest, WaitPopUnblocksAfterItemIsInserted)
{
    ConcurrentBoundedQueue<int> queue(1);
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<PopResult<int>> result;
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
    EXPECT_EQ(push_status, ContainerStatus::kSuccess);
    if (push_status != ContainerStatus::kSuccess)
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
    ASSERT_EQ(value.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(*value.value, 42);
}

TEST(ConcurrentBoundedQueueTest, WaitPushUnblocksAfterCapacityBecomesAvailable)
{
    ConcurrentBoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(1), ContainerStatus::kSuccess);
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<ContainerStatus> result;
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
    EXPECT_EQ(first.status, ContainerStatus::kSuccess);
    if (first.status != ContainerStatus::kSuccess || !first.value)
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
    EXPECT_EQ(result_future.get(), ContainerStatus::kSuccess);
    ASSERT_EQ(first.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(first.value.has_value());
    EXPECT_EQ(*first.value, 1);
    const auto second = queue.tryPop();
    ASSERT_EQ(second.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(second.value.has_value());
    EXPECT_EQ(*second.value, 2);
}

TEST(ConcurrentBoundedQueueTest, CloseRejectsPushesAndDrainsExistingValues)
{
    ConcurrentBoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(1), ContainerStatus::kSuccess);

    queue.close();

    EXPECT_TRUE(queue.isClosed());
    EXPECT_EQ(queue.tryPush(2), ContainerStatus::kClosed);
    EXPECT_EQ(
        queue.waitPush(2, std::stop_token{}), ContainerStatus::kClosed);

    const auto existing = queue.waitPop(std::stop_token{});
    ASSERT_EQ(existing.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(existing.value.has_value());
    EXPECT_EQ(*existing.value, 1);
    EXPECT_EQ(queue.waitPop(std::stop_token{}).status, ContainerStatus::kClosed);
    EXPECT_EQ(queue.tryPop().status, ContainerStatus::kClosed);
}

TEST(ConcurrentBoundedQueueTest, CloseReleasesWaitingProducer)
{
    ConcurrentBoundedQueue<int> queue(1);
    ASSERT_EQ(queue.tryPush(1), ContainerStatus::kSuccess);
    ContainerStatus result = ContainerStatus::kSuccess;

    std::thread producer([&] {
        result = queue.waitPush(2, std::stop_token{});
    });

    queue.close();
    producer.join();

    EXPECT_EQ(result, ContainerStatus::kClosed);
    EXPECT_TRUE(queue.isClosed());
}

TEST(ConcurrentBoundedQueueTest, CloseReleasesWaitingConsumer)
{
    ConcurrentBoundedQueue<int> queue(1);
    ContainerStatus result = ContainerStatus::kSuccess;

    std::thread consumer([&] {
        result = queue.waitPop(std::stop_token{}).status;
    });

    queue.close();
    consumer.join();

    EXPECT_EQ(result, ContainerStatus::kClosed);
}

TEST(ConcurrentBoundedQueueTest, HandlesConcurrentProducersAndConsumers)
{
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t consumer_count = 4;
    constexpr std::size_t items_per_producer = 1'000;
    constexpr std::size_t items_per_consumer =
        producer_count * items_per_producer / consumer_count;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    ConcurrentBoundedQueue<int> queue(8);
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
                if (result.status == ContainerStatus::kSuccess && result.value)
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

} // namespace systemcore::concurrency::containers
