/*****************************************************************************
 * @file concurrent_circular_buffer.hpp
 * @brief Declares the thread-safe ConcurrentCircularBuffer class.
 *****************************************************************************/
#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief A fixed-capacity, thread-safe circular buffer.
 *
 * Elements are removed in FIFO order. @c head_index_ identifies the next slot
 * to write, while @c tail_index_ identifies the oldest element to remove.
 * Both indexes wrap to zero at the end of the storage. Inserting into a full
 * buffer overwrites and discards its oldest element.
 *
 * @tparam T Type of elements stored in the buffer.
 */
template <typename T>
class ConcurrentCircularBuffer
{
public:
    /// Default capacity used when no capacity is specified.
    static constexpr std::size_t kDefaultCapacity{100};

    /**
     * @brief Constructs a buffer with the specified capacity.
     * @param capacity Maximum number of elements the buffer can store.
     * @throws std::invalid_argument If @p capacity is zero.
     */
    explicit ConcurrentCircularBuffer(
        const std::size_t capacity = kDefaultCapacity)
        : capacity_(validateCapacity(capacity)),
          buffer_(capacity_)
    {
    }

    /**
     * @brief Copies an element into the buffer, overwriting the oldest if full.
     * @param item Element to copy into the buffer.
     */
    void pushOverwrite(const T &item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        overwritePush(item);
    }

    /**
     * @brief Moves an element into the buffer, overwriting the oldest if full.
     * @param item Element to move into the buffer.
     */
    void pushOverwrite(T &&item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        overwritePush(std::move(item));
    }

    /**
     * @brief Removes and returns the oldest element if one is available.
     * @return The oldest element, or @c std::nullopt if the buffer is empty.
     */
    std::optional<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ == 0)
        {
            return std::nullopt;
        }

        std::optional<T> item(std::move(buffer_[tail_index_]));
        buffer_[tail_index_].reset();
        tail_index_ = (tail_index_ + 1) % capacity_;
        --count_;
        return item;
    }

    /**
     * @brief Removes all elements and resets both circular indexes.
     */
    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &item : buffer_)
        {
            item.reset();
        }

        head_index_ = 0;
        tail_index_ = 0;
        count_ = 0;
    }

    /**
     * @brief Checks whether the buffer contains no elements.
     * @return @c true if empty; otherwise @c false.
     *
     * The result is a snapshot and may change immediately if another thread
     * modifies the buffer.
     */
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ == 0;
    }

    /**
     * @brief Checks whether the buffer is at capacity.
     * @return @c true if full; otherwise @c false.
     *
     * The result is a snapshot and may change immediately if another thread
     * modifies the buffer.
     */
    bool isFull() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ == capacity_;
    }

    /**
     * @brief Returns the maximum number of elements the buffer can store.
     * @return The buffer capacity.
     */
    std::size_t capacity() const noexcept
    {
        return capacity_;
    }

    /**
     * @brief Returns the current number of stored elements.
     * @return A snapshot of the number of elements in the buffer.
     */
    std::size_t count() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    /**
     * @brief Validates the requested buffer capacity.
     * @param capacity Requested maximum capacity.
     * @return The validated capacity.
     * @throws std::invalid_argument If @p capacity is zero.
     */
    static std::size_t validateCapacity(const std::size_t capacity)
    {
        if (capacity == 0)
        {
            throw std::invalid_argument(
                "ConcurrentCircularBuffer capacity must be greater than zero");
        }

        return capacity;
    }

    /**
     * @brief Inserts an element, discarding the oldest if the buffer is full.
     * @param item Element to copy or move into the buffer.
     * @tparam U Deduced type of @p item, preserving whether it is an lvalue
     *            or rvalue.
     *
     * The caller must hold @c mutex_. The head advances after every insertion.
     * If full, the tail advances as well because the oldest slot was replaced;
     * otherwise, the stored element count increases.
     */
    template <typename U>
    void overwritePush(U &&item)
    {
        // Forward the value, copying or moving it into the current buffer slot.
        // The optional constructs a value if the slot is empty or assigns to
        // the existing value if the slot is occupied.
        buffer_[head_index_] = std::forward<U>(item);
        head_index_ = (head_index_ + 1) % capacity_;

        if (count_ == capacity_)
        {
            tail_index_ = (tail_index_ + 1) % capacity_;
        }
        else
        {
            ++count_;
        }
    }

    /// Maximum number of elements the buffer can store.
    const std::size_t capacity_;

    /// Optional slots; disengaged slots contain no element.
    std::vector<std::optional<T>> buffer_;

    /// Index of the next slot to write.
    std::size_t head_index_{0};

    /// Index of the oldest element to remove.
    std::size_t tail_index_{0};

    /// Number of elements currently stored.
    std::size_t count_{0};

    /// Protects the buffer storage and indexes.
    mutable std::mutex mutex_;
};
