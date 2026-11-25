/*****************************************************************************
 * @file bounded_queue_test.cpp
 * @brief Tests bounded queue capacity, blocking operations, and cancellation.
 *****************************************************************************/

#include "systemcore/concurrency/bounded_queue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
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

    EXPECT_TRUE(queue.waitPush(42, std::stop_token{}));

    const auto value = queue.waitPop(std::stop_token{});
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
}

TEST(BoundedQueueTest, WaitPushReturnsFalseWhenCancelledAndFull)
{
    BoundedQueue<int> queue(1);
    ASSERT_TRUE(queue.tryPush(42));

    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_FALSE(queue.waitPush(7, stop_source.get_token()));

    const auto value = queue.waitPop(std::stop_token{});
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
}

TEST(BoundedQueueTest, TryPushAndTryPopPreserveFifoAndCapacity)
{
    BoundedQueue<std::string> queue(2);

    EXPECT_EQ(queue.capacity(), 2);
    EXPECT_TRUE(queue.isEmpty());
    ASSERT_TRUE(queue.tryPush("first"));
    ASSERT_TRUE(queue.tryPush("second"));
    EXPECT_TRUE(queue.isFull());
    EXPECT_FALSE(queue.tryPush("third"));

    auto first = queue.tryPop();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(*first, "first");
    EXPECT_FALSE(queue.isFull());

    ASSERT_TRUE(queue.tryPush("third"));
    auto second = queue.tryPop();
    auto third = queue.tryPop();
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(*second, "second");
    EXPECT_EQ(*third, "third");
    EXPECT_TRUE(queue.isEmpty());
}

TEST(BoundedQueueTest, WaitPopReturnsEmptyWhenCancelled)
{
    BoundedQueue<int> queue(1);
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_FALSE(queue.waitPop(stop_source.get_token()).has_value());
}

TEST(BoundedQueueTest, HandlesConcurrentProducersAndConsumers)
{
    constexpr std::size_t producer_count = 2;
    constexpr std::size_t consumer_count = 2;
    constexpr std::size_t items_per_producer = 100;
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
                const auto value = queue.waitPop(std::stop_token{});
                if (value)
                {
                    values.push_back(*value);
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
