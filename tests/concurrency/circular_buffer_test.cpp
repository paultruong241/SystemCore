/*****************************************************************************
 * @file circular_buffer_test.cpp
 * @brief Tests circular buffer ordering, overwriting, and concurrent access.
 *****************************************************************************/

#include "systemcore/concurrency/circular_buffer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace systemcore::concurrency
{

namespace
{

struct ThrowingValue
{
    explicit ThrowingValue(const int value)
        : value(value)
    {
    }

    ThrowingValue(const ThrowingValue &other)
        : value(other.value)
    {
    }

    ThrowingValue(ThrowingValue &&other)
    {
        if (throw_on_move)
        {
            throw std::runtime_error("move failed");
        }

        value = other.value;
        other.value = -1;
    }

    ThrowingValue &operator=(const ThrowingValue &other)
    {
        if (throw_on_copy_assignment)
        {
            throw std::runtime_error("copy assignment failed");
        }

        value = other.value;
        return *this;
    }

    ThrowingValue &operator=(ThrowingValue &&other)
    {
        if (throw_on_move)
        {
            throw std::runtime_error("move assignment failed");
        }

        value = other.value;
        other.value = -1;
        return *this;
    }

    int value{0};
    static bool throw_on_copy_assignment;
    static bool throw_on_move;
};

bool ThrowingValue::throw_on_copy_assignment{false};
bool ThrowingValue::throw_on_move{false};

template <typename T>
void expectPopValue(CircularBuffer<T> &buffer, const T &expected)
{
    const auto result = buffer.tryPop();
    ASSERT_EQ(result.status, QueueStatus::kSuccess);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(*result.value, expected);
}

} // namespace

TEST(CircularBufferTest, OverwritesOldestElementWhenFull)
{
    CircularBuffer<int> buffer(3);

    buffer.pushOverwrite(1);
    buffer.pushOverwrite(2);
    buffer.pushOverwrite(3);
    buffer.pushOverwrite(4);

    EXPECT_EQ(buffer.count(), 3);
    EXPECT_TRUE(buffer.isFull());

    expectPopValue(buffer, 2);
    expectPopValue(buffer, 3);
    expectPopValue(buffer, 4);
    EXPECT_EQ(buffer.tryPop().status, QueueStatus::kEmpty);
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(CircularBufferTest, WrapsCorrectlyAfterTryPop)
{
    CircularBuffer<int> buffer(3);
    buffer.pushOverwrite(1);
    buffer.pushOverwrite(2);
    buffer.pushOverwrite(3);

    expectPopValue(buffer, 1);
    buffer.pushOverwrite(4);

    expectPopValue(buffer, 2);
    expectPopValue(buffer, 3);
    expectPopValue(buffer, 4);
}

TEST(CircularBufferTest, SupportsMoveOnlyElements)
{
    CircularBuffer<std::unique_ptr<int>> buffer(1);
    auto first = std::make_unique<int>(1);
    buffer.pushOverwrite(std::move(first));
    ASSERT_FALSE(first);

    auto second = std::make_unique<int>(2);
    buffer.pushOverwrite(std::move(second));
    ASSERT_FALSE(second);

    auto value = buffer.tryPop();
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    ASSERT_NE(*value.value, nullptr);
    EXPECT_EQ(**value.value, 2);
}

TEST(CircularBufferTest, CopiesLvalueElements)
{
    CircularBuffer<int> buffer(1);
    int item = 42;

    buffer.pushOverwrite(item);

    EXPECT_EQ(item, 42);
    expectPopValue(buffer, 42);
}

TEST(CircularBufferTest, ResetEmptiesBufferAndAllowsReuse)
{
    CircularBuffer<int> buffer(2);
    buffer.pushOverwrite(1);
    buffer.pushOverwrite(2);

    buffer.reset();

    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_EQ(buffer.count(), 0);
    buffer.pushOverwrite(3);
    expectPopValue(buffer, 3);
}

TEST(CircularBufferTest, CloseRejectsWritesAndPreservesBufferedValues)
{
    CircularBuffer<int> buffer(2);
    ASSERT_EQ(buffer.pushOverwrite(1), QueueStatus::kSuccess);
    ASSERT_EQ(buffer.pushOverwrite(2), QueueStatus::kSuccess);

    buffer.close();

    EXPECT_TRUE(buffer.isClosed());
    EXPECT_EQ(buffer.pushOverwrite(3), QueueStatus::kClosed);
    expectPopValue(buffer, 1);
    expectPopValue(buffer, 2);
    EXPECT_EQ(buffer.tryPop().status, QueueStatus::kClosed);

    buffer.reset();
    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_EQ(buffer.pushOverwrite(4), QueueStatus::kClosed);
}

TEST(CircularBufferTest, RejectsZeroCapacity)
{
    EXPECT_THROW(CircularBuffer<int> buffer(0), std::invalid_argument);
}

TEST(CircularBufferTest, UsesDefaultCapacity)
{
    CircularBuffer<int> buffer;

    EXPECT_EQ(buffer.capacity(), CircularBuffer<int>::kDefaultCapacity);
}

TEST(CircularBufferTest, SupportsConcurrentWriters)
{
    constexpr std::size_t writer_count = 4;
    constexpr std::size_t items_per_writer = 100;
    constexpr std::size_t capacity = 32;
    CircularBuffer<int> buffer(capacity);
    std::vector<std::thread> writers;

    for (std::size_t writer = 0; writer < writer_count; ++writer)
    {
        writers.emplace_back([&, writer] {
            const auto first_item = writer * items_per_writer;
            for (std::size_t i = 0; i < items_per_writer; ++i)
            {
                buffer.pushOverwrite(static_cast<int>(first_item + i));
            }
        });
    }

    for (auto &writer : writers)
    {
        writer.join();
    }

    ASSERT_TRUE(buffer.isFull());
    ASSERT_EQ(buffer.count(), capacity);

    std::vector<int> values;
    values.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i)
    {
        const auto value = buffer.tryPop();
        ASSERT_EQ(value.status, QueueStatus::kSuccess);
        ASSERT_TRUE(value.value.has_value());
        values.push_back(*value.value);
    }

    std::sort(values.begin(), values.end());
    EXPECT_TRUE(std::adjacent_find(values.begin(), values.end()) == values.end());
    EXPECT_TRUE(std::all_of(values.begin(), values.end(), [](const int value) {
        return value >= 0 &&
               value < static_cast<int>(writer_count * items_per_writer);
    }));
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(CircularBufferTest, PreservesStateWhenElementAssignmentThrows)
{
    CircularBuffer<ThrowingValue> buffer(1);
    buffer.pushOverwrite(ThrowingValue{1});
    const ThrowingValue replacement{2};

    ThrowingValue::throw_on_copy_assignment = true;
    EXPECT_THROW(buffer.pushOverwrite(replacement), std::runtime_error);
    ThrowingValue::throw_on_copy_assignment = false;

    ASSERT_EQ(buffer.count(), 1);
    auto value = buffer.tryPop();
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(value.value->value, 1);
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(CircularBufferTest, PreservesStateWhenElementMoveThrows)
{
    CircularBuffer<ThrowingValue> buffer(1);
    buffer.pushOverwrite(ThrowingValue{7});

    ThrowingValue::throw_on_move = true;
    EXPECT_THROW(buffer.tryPop(), std::runtime_error);
    ThrowingValue::throw_on_move = false;

    ASSERT_EQ(buffer.count(), 1);
    auto value = buffer.tryPop();
    ASSERT_EQ(value.status, QueueStatus::kSuccess);
    ASSERT_TRUE(value.value.has_value());
    EXPECT_EQ(value.value->value, 7);
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(CircularBufferTest, SupportsConcurrentProducersAndConsumers)
{
    constexpr std::size_t producer_count = 2;
    constexpr std::size_t consumer_count = 2;
    constexpr std::size_t items_per_producer = 500;
    constexpr std::size_t capacity = 64;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    CircularBuffer<int> buffer(capacity);
    std::atomic<std::size_t> completed_producers{0};
    std::vector<std::vector<int>> consumed(consumer_count);
    std::vector<std::thread> consumers;

    for (std::size_t consumer = 0; consumer < consumer_count; ++consumer)
    {
        consumers.emplace_back([&, consumer] {
            while (true)
            {
                const auto result = buffer.tryPop();
                if (result.status == QueueStatus::kSuccess)
                {
                    consumed[consumer].push_back(*result.value);
                }
                else if (completed_producers.load(std::memory_order_acquire) ==
                         producer_count)
                {
                    break;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::vector<std::thread> producers;
    for (std::size_t producer = 0; producer < producer_count; ++producer)
    {
        producers.emplace_back([&, producer] {
            const auto first_item = producer * items_per_producer;
            for (std::size_t i = 0; i < items_per_producer; ++i)
            {
                buffer.pushOverwrite(static_cast<int>(first_item + i));
            }
            completed_producers.fetch_add(1, std::memory_order_release);
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

    std::vector<int> values;
    for (const auto &consumer_values : consumed)
    {
        values.insert(values.end(), consumer_values.begin(), consumer_values.end());
    }
    std::sort(values.begin(), values.end());

    EXPECT_FALSE(values.empty());
    EXPECT_TRUE(std::adjacent_find(values.begin(), values.end()) == values.end());
    EXPECT_TRUE(std::all_of(values.begin(), values.end(), [](const int value) {
        return value >= 0 && value < static_cast<int>(total_items);
    }));
    EXPECT_TRUE(buffer.isEmpty());
}

} // namespace systemcore::concurrency
