/*****************************************************************************
 * @file concurrent_circular_buffer_test.cpp
 * @brief Tests ConcurrentCircularBuffer ordering and overwrite behavior.
 *****************************************************************************/

#include "containers/concurrent_circular_buffer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace containers
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

} // namespace

TEST(ConcurrentCircularBufferTest, OverwritesOldestElementWhenFull)
{
    ConcurrentCircularBuffer<int> buffer(3);

    buffer.pushOverwrite(1);
    buffer.pushOverwrite(2);
    buffer.pushOverwrite(3);
    buffer.pushOverwrite(4);

    EXPECT_EQ(buffer.count(), 3);
    EXPECT_TRUE(buffer.isFull());

    EXPECT_EQ(buffer.tryPop(), 2);
    EXPECT_EQ(buffer.tryPop(), 3);
    EXPECT_EQ(buffer.tryPop(), 4);
    EXPECT_FALSE(buffer.tryPop().has_value());
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(ConcurrentCircularBufferTest, WrapsCorrectlyAfterTryPop)
{
    ConcurrentCircularBuffer<int> buffer(3);
    buffer.pushOverwrite(1);
    buffer.pushOverwrite(2);
    buffer.pushOverwrite(3);

    EXPECT_EQ(buffer.tryPop(), 1);
    buffer.pushOverwrite(4);

    EXPECT_EQ(buffer.tryPop(), 2);
    EXPECT_EQ(buffer.tryPop(), 3);
    EXPECT_EQ(buffer.tryPop(), 4);
}

TEST(ConcurrentCircularBufferTest, SupportsMoveOnlyElements)
{
    ConcurrentCircularBuffer<std::unique_ptr<int>> buffer(1);
    auto first = std::make_unique<int>(1);
    buffer.pushOverwrite(std::move(first));
    ASSERT_FALSE(first);

    auto second = std::make_unique<int>(2);
    buffer.pushOverwrite(std::move(second));
    ASSERT_FALSE(second);

    auto value = buffer.tryPop();
    ASSERT_TRUE(value.has_value());
    ASSERT_NE(*value, nullptr);
    EXPECT_EQ(**value, 2);
}

TEST(ConcurrentCircularBufferTest, CopiesLvalueElements)
{
    ConcurrentCircularBuffer<int> buffer(1);
    int item = 42;

    buffer.pushOverwrite(item);

    EXPECT_EQ(item, 42);
    EXPECT_EQ(buffer.tryPop(), 42);
}

TEST(ConcurrentCircularBufferTest, ResetEmptiesBufferAndAllowsReuse)
{
    ConcurrentCircularBuffer<int> buffer(2);
    buffer.pushOverwrite(1);
    buffer.pushOverwrite(2);

    buffer.reset();

    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_EQ(buffer.count(), 0);
    buffer.pushOverwrite(3);
    EXPECT_EQ(buffer.tryPop(), 3);
}

TEST(ConcurrentCircularBufferTest, RejectsZeroCapacity)
{
    EXPECT_THROW(ConcurrentCircularBuffer<int> buffer(0), std::invalid_argument);
}

TEST(ConcurrentCircularBufferTest, UsesDefaultCapacity)
{
    ConcurrentCircularBuffer<int> buffer;

    EXPECT_EQ(buffer.capacity(), ConcurrentCircularBuffer<int>::kDefaultCapacity);
}

TEST(ConcurrentCircularBufferTest, SupportsConcurrentWriters)
{
    constexpr std::size_t writer_count = 4;
    constexpr std::size_t items_per_writer = 100;
    constexpr std::size_t capacity = 32;
    ConcurrentCircularBuffer<int> buffer(capacity);
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
        ASSERT_TRUE(value.has_value());
        values.push_back(*value);
    }

    std::sort(values.begin(), values.end());
    EXPECT_TRUE(std::adjacent_find(values.begin(), values.end()) == values.end());
    EXPECT_TRUE(std::all_of(values.begin(), values.end(), [](const int value) {
        return value >= 0 &&
               value < static_cast<int>(writer_count * items_per_writer);
    }));
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(ConcurrentCircularBufferTest, PreservesStateWhenElementAssignmentThrows)
{
    ConcurrentCircularBuffer<ThrowingValue> buffer(1);
    buffer.pushOverwrite(ThrowingValue{1});
    const ThrowingValue replacement{2};

    ThrowingValue::throw_on_copy_assignment = true;
    EXPECT_THROW(buffer.pushOverwrite(replacement), std::runtime_error);
    ThrowingValue::throw_on_copy_assignment = false;

    ASSERT_EQ(buffer.count(), 1);
    auto value = buffer.tryPop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->value, 1);
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(ConcurrentCircularBufferTest, PreservesStateWhenElementMoveThrows)
{
    ConcurrentCircularBuffer<ThrowingValue> buffer(1);
    buffer.pushOverwrite(ThrowingValue{7});

    ThrowingValue::throw_on_move = true;
    EXPECT_THROW(buffer.tryPop(), std::runtime_error);
    ThrowingValue::throw_on_move = false;

    ASSERT_EQ(buffer.count(), 1);
    auto value = buffer.tryPop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->value, 7);
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(ConcurrentCircularBufferTest, SupportsConcurrentProducersAndConsumers)
{
    constexpr std::size_t producer_count = 2;
    constexpr std::size_t consumer_count = 2;
    constexpr std::size_t items_per_producer = 500;
    constexpr std::size_t capacity = 64;
    constexpr std::size_t total_items = producer_count * items_per_producer;

    ConcurrentCircularBuffer<int> buffer(capacity);
    std::atomic<std::size_t> completed_producers{0};
    std::vector<std::vector<int>> consumed(consumer_count);
    std::vector<std::thread> consumers;

    for (std::size_t consumer = 0; consumer < consumer_count; ++consumer)
    {
        consumers.emplace_back([&, consumer] {
            while (true)
            {
                if (auto value = buffer.tryPop())
                {
                    consumed[consumer].push_back(*value);
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

} // namespace containers
