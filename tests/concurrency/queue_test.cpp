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

    queue.push(first);
    queue.push(std::move(second));

    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.isEmpty());

    auto first_value = queue.tryPop();
    auto second_value = queue.waitPop(std::stop_token{});
    ASSERT_TRUE(first_value.has_value());
    ASSERT_TRUE(second_value.has_value());
    EXPECT_EQ(*first_value, "first");
    EXPECT_EQ(*second_value, "second");
    EXPECT_TRUE(queue.isEmpty());
}

TEST(QueueTest, SupportsMoveOnlyValues)
{
    Queue<std::unique_ptr<int>> queue;
    queue.push(std::make_unique<int>(42));

    auto value = queue.tryPop();
    ASSERT_TRUE(value.has_value());
    ASSERT_NE(*value, nullptr);
    EXPECT_EQ(**value, 42);
}

TEST(QueueTest, WaitPopReturnsEmptyWhenCancelled)
{
    Queue<int> queue;
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_FALSE(queue.waitPop(stop_source.get_token()).has_value());
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
