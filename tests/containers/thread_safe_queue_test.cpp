/*****************************************************************************
 * @file thread_safe_queue_test.cpp
 * @brief Tests ThreadSafeQueue value handling and concurrent access.
 *****************************************************************************/

#include "containers/thread_safe_queue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/**
 * @brief Verifies that the queue supports copyable and move-only value types.
 */
TEST(ThreadSafeQueueTest, SupportsDifferentValueTypes)
{
    ThreadSafeQueue<int> integer_queue;
    integer_queue.push(42);
    EXPECT_EQ(integer_queue.wait_pop(), 42);

    ThreadSafeQueue<std::string> string_queue;
    const std::string copied_value = "copied value";
    string_queue.push(copied_value);
    EXPECT_EQ(string_queue.wait_pop(), copied_value);

    std::string moved_value = "moved value";
    string_queue.push(std::move(moved_value));
    EXPECT_EQ(string_queue.wait_pop(), "moved value");

    ThreadSafeQueue<std::unique_ptr<int>> pointer_queue;
    auto input = std::make_unique<int>(7);
    pointer_queue.push(std::move(input));
    EXPECT_FALSE(input);

    auto output = pointer_queue.try_pop();
    ASSERT_TRUE(output.has_value());
    ASSERT_NE(*output, nullptr);
    EXPECT_EQ(**output, 7);
}

/**
 * @brief Verifies that concurrent producers and consumers preserve every item.
 */
TEST(ThreadSafeQueueTest, HandlesMultipleProducersAndConsumers)
{
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t consumer_count = 4;
    constexpr std::size_t items_per_producer = 250;
    constexpr std::size_t items_per_consumer =
        producer_count * items_per_producer / consumer_count;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    ThreadSafeQueue<int> queue;
    std::vector<std::vector<int>> consumed(consumer_count);
    std::vector<std::thread> consumers;
    consumers.reserve(consumer_count);

    for (std::size_t consumer_id = 0; consumer_id < consumer_count; ++consumer_id)
    {
        consumers.emplace_back([&, consumer_id] {
            auto &values = consumed[consumer_id];
            values.reserve(items_per_consumer);
            for (std::size_t i = 0; i < items_per_consumer; ++i)
            {
                values.push_back(queue.wait_pop());
            }
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(producer_count);

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
