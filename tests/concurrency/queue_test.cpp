/*****************************************************************************
 * @file queue_test.cpp
 * @brief Tests queue operations, lifecycle behavior, and concurrent access.
 *****************************************************************************/

#include "systemcore/concurrency/containers/concurrent_queue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <numeric>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace systemcore::concurrency::containers
{

namespace
{

void expectEveryValueOnce(std::vector<int> values, const std::size_t count)
{
    std::sort(values.begin(), values.end());

    std::vector<int> expected(count);
    std::iota(expected.begin(), expected.end(), 0);
    EXPECT_EQ(values, expected);
}

} // namespace

TEST(ConcurrentQueueTest, PushSupportsCopiesMovesAndFifoRetrieval)
{
    ConcurrentQueue<std::string> queue;
    const std::string first = "first";
    std::string second = "second";

    EXPECT_EQ(queue.push(first), ContainerStatus::kSuccess);
    EXPECT_EQ(queue.push(std::move(second)), ContainerStatus::kSuccess);

    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.empty());

    auto first_value = queue.tryPop();
    auto second_value = queue.waitPop(std::stop_token{});
    ASSERT_EQ(first_value.status, ContainerStatus::kSuccess);
    ASSERT_EQ(second_value.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(first_value.value.has_value());
    ASSERT_TRUE(second_value.value.has_value());
    EXPECT_EQ(*first_value.value, "first");
    EXPECT_EQ(*second_value.value, "second");
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.tryPop().status, ContainerStatus::kEmpty);
}

TEST(ConcurrentQueueTest, SupportsMoveOnlyValues)
{
    ConcurrentQueue<std::unique_ptr<int>> queue;
    ASSERT_EQ(queue.push(std::make_unique<int>(42)), ContainerStatus::kSuccess);

    auto value = queue.tryPop();
    ASSERT_EQ(value.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    ASSERT_NE(*value.value, nullptr);
    EXPECT_EQ(**value.value, 42);
}

TEST(ConcurrentQueueTest, WaitPopReturnsStoppedWhenCancelled)
{
    ConcurrentQueue<int> queue;
    std::stop_source stop_source;
    stop_source.request_stop();

    EXPECT_EQ(
        queue.waitPop(stop_source.get_token()).status, ContainerStatus::kStopped);
    EXPECT_FALSE(queue.isClosed());
    EXPECT_EQ(queue.push(42), ContainerStatus::kSuccess);
}

TEST(ConcurrentQueueTest, WaitPopReturnsStoppedWhenCancelledWhileWaiting)
{
    ConcurrentQueue<int> queue;
    std::stop_source stop_source;
    std::promise<void> started;
    auto started_future = started.get_future();
    std::promise<ContainerStatus> result;
    auto result_future = result.get_future();

    std::thread consumer([&] {
        started.set_value();
        result.set_value(queue.waitPop(stop_source.get_token()).status);
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
    EXPECT_EQ(result_future.get(), ContainerStatus::kStopped);
}

TEST(ConcurrentQueueTest, CloseRejectsPushesAndDrainsExistingValues)
{
    ConcurrentQueue<int> queue;
    ASSERT_EQ(queue.push(1), ContainerStatus::kSuccess);
    ASSERT_EQ(queue.push(2), ContainerStatus::kSuccess);

    queue.close();
    queue.close();

    EXPECT_TRUE(queue.isClosed());
    EXPECT_EQ(queue.push(3), ContainerStatus::kClosed);
    EXPECT_EQ(queue.push(4), ContainerStatus::kClosed);

    const auto first = queue.waitPop(std::stop_token{});
    const auto second = queue.waitPop(std::stop_token{});
    ASSERT_EQ(first.status, ContainerStatus::kSuccess);
    ASSERT_EQ(second.status, ContainerStatus::kSuccess);
    ASSERT_TRUE(first.value.has_value());
    ASSERT_TRUE(second.value.has_value());
    EXPECT_EQ(*first.value, 1);
    EXPECT_EQ(*second.value, 2);
    EXPECT_EQ(queue.waitPop(std::stop_token{}).status, ContainerStatus::kClosed);
    EXPECT_EQ(queue.tryPop().status, ContainerStatus::kClosed);
}

TEST(ConcurrentQueueTest, CloseReleasesWaitingConsumer)
{
    ConcurrentQueue<int> queue;
    ContainerStatus result = ContainerStatus::kSuccess;

    std::thread consumer([&] {
        result = queue.waitPop(std::stop_token{}).status;
    });

    queue.close();
    consumer.join();

    EXPECT_EQ(result, ContainerStatus::kClosed);
}

TEST(ConcurrentQueueTest, SingleProducerSingleConsumerPreservesOrder)
{
    constexpr std::size_t item_count = 500;
    ConcurrentQueue<int> queue;
    std::vector<int> consumed;
    bool pushes_succeeded = true;
    bool consumer_succeeded = true;

    std::thread consumer([&] {
        while (true)
        {
            const auto result = queue.waitPop(std::stop_token{});
            if (result.status == ContainerStatus::kClosed)
            {
                break;
            }
            if (result.status != ContainerStatus::kSuccess || !result.value)
            {
                consumer_succeeded = false;
                break;
            }
            consumed.push_back(*result.value);
        }
    });

    std::thread producer([&] {
        for (std::size_t value = 0; value < item_count; ++value)
        {
            if (queue.push(static_cast<int>(value)) != ContainerStatus::kSuccess)
            {
                pushes_succeeded = false;
            }
        }
        queue.close();
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(pushes_succeeded);
    EXPECT_TRUE(consumer_succeeded);
    std::vector<int> expected(item_count);
    std::iota(expected.begin(), expected.end(), 0);
    EXPECT_EQ(consumed, expected);
}

TEST(ConcurrentQueueTest, MultipleProducersSingleConsumerReceivesEveryValue)
{
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t items_per_producer = 250;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    ConcurrentQueue<int> queue;
    std::vector<int> consumed;
    std::atomic<bool> pushes_succeeded{true};
    bool consumer_succeeded = true;

    std::thread consumer([&] {
        while (true)
        {
            const auto result = queue.waitPop(std::stop_token{});
            if (result.status == ContainerStatus::kClosed)
            {
                break;
            }
            if (result.status != ContainerStatus::kSuccess || !result.value)
            {
                consumer_succeeded = false;
                break;
            }
            consumed.push_back(*result.value);
        }
    });

    std::vector<std::thread> producers;
    for (std::size_t producer_id = 0; producer_id < producer_count; ++producer_id)
    {
        producers.emplace_back([&, producer_id] {
            const auto first_value = producer_id * items_per_producer;
            for (std::size_t i = 0; i < items_per_producer; ++i)
            {
                if (queue.push(static_cast<int>(first_value + i)) !=
                    ContainerStatus::kSuccess)
                {
                    pushes_succeeded.store(false);
                }
            }
        });
    }

    for (auto &producer : producers)
    {
        producer.join();
    }
    queue.close();
    consumer.join();

    EXPECT_TRUE(pushes_succeeded.load());
    EXPECT_TRUE(consumer_succeeded);
    expectEveryValueOnce(std::move(consumed), total_items);
}

TEST(ConcurrentQueueTest, SingleProducerMultipleConsumersReceiveEveryValue)
{
    constexpr std::size_t consumer_count = 4;
    constexpr std::size_t items_per_consumer = 250;
    constexpr std::size_t total_items = consumer_count * items_per_consumer;

    ConcurrentQueue<int> queue;
    std::vector<std::vector<int>> consumed(consumer_count);
    std::atomic<bool> consumers_succeeded{true};
    bool pushes_succeeded = true;
    std::vector<std::thread> consumers;

    for (std::size_t consumer_id = 0; consumer_id < consumer_count; ++consumer_id)
    {
        consumers.emplace_back([&, consumer_id] {
            auto &values = consumed[consumer_id];
            while (true)
            {
                const auto result = queue.waitPop(std::stop_token{});
                if (result.status == ContainerStatus::kClosed)
                {
                    break;
                }
                if (result.status != ContainerStatus::kSuccess || !result.value)
                {
                    consumers_succeeded.store(false);
                    break;
                }
                values.push_back(*result.value);
            }
        });
    }

    std::thread producer([&] {
        for (std::size_t value = 0; value < total_items; ++value)
        {
            if (queue.push(static_cast<int>(value)) != ContainerStatus::kSuccess)
            {
                pushes_succeeded = false;
            }
        }
        queue.close();
    });

    producer.join();
    for (auto &consumer : consumers)
    {
        consumer.join();
    }

    std::vector<int> all_values;
    all_values.reserve(total_items);
    for (const auto &values : consumed)
    {
        all_values.insert(all_values.end(), values.begin(), values.end());
    }
    EXPECT_TRUE(pushes_succeeded);
    EXPECT_TRUE(consumers_succeeded.load());
    expectEveryValueOnce(std::move(all_values), total_items);
}

TEST(ConcurrentQueueTest, StressTestHandlesMultipleProducersAndConsumers)
{
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t consumer_count = 4;
    constexpr std::size_t items_per_producer = 2'000;
    constexpr std::size_t items_per_consumer =
        producer_count * items_per_producer / consumer_count;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    ConcurrentQueue<int> queue;
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

} // namespace systemcore::concurrency::containers
