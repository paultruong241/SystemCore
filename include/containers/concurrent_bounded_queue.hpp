/*****************************************************************************
 * @file concurrent_bounded_queue.hpp
 * @brief Declares the ConcurrentBoundedQueue class.
 *****************************************************************************/
#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <utility>

/**
 * @brief A fixed-capacity, thread-safe FIFO queue.
 *
 * Elements are inserted at the back and removed from the front, preserving
 * their insertion order. The queue supports concurrent access from multiple
 * threads and protects its internal state using a mutex.
 *
 * Non-blocking insertion fails when the queue reaches its maximum capacity.
 * Stop-aware insertion and retrieval can wait for available capacity or
 * elements and can be cancelled through @c std::stop_token.
 *
 * Existing elements are never overwritten.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class ConcurrentBoundedQueue
{
public:
    /// Default capacity used when no capacity is specified.
    static constexpr std::size_t kDefaultCapacity{100};

    /**
     * @brief Constructs a concurrent bounded queue with the specified capacity.
     * @param capacity Maximum number of elements the queue can store.
     * @throws std::invalid_argument If @p capacity is zero.
     */
    explicit ConcurrentBoundedQueue(const std::size_t capacity = kDefaultCapacity)
        : capacity_(validateCapacity(capacity))
    {
    }

    /**
     * @brief Attempts to copy an element into the queue without blocking.
     * @param item Element to copy into the queue.
     * @return @c true if inserted; @c false if the queue is full.
     */
    bool tryPush(const T &item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (queue_.size() == capacity_)
            {
                return false;
            }

            queue_.push(item);
        }

        not_empty_cv_.notify_one();

        return true;
    }

    /**
     * @brief Attempts to move an element into the queue without blocking.
     * @param item Element to move into the queue.
     * @return @c true if inserted; @c false if the queue is full.
     */
    bool tryPush(T &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (queue_.size() == capacity_)
            {
                return false;
            }

            queue_.push(std::move(item));
        }

        not_empty_cv_.notify_one();

        return true;
    }

    /**
     * @brief Copies an element, waiting for capacity or cancellation.
     *
     * This overload blocks while the queue is full. If the wait is cancelled
     * before capacity becomes available, the queue is unchanged.
     * @param item Element to copy into the queue.
     * @param stop Token used to cancel the wait.
     * @return @c true if inserted; @c false if stopped before capacity was available.
     */
    bool waitPush(const T &item, std::stop_token stop)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_full_cv_.wait(lock, stop, [this]
                               { return queue_.size() < capacity_; }))
        {
            return false;
        }

        queue_.push(item);

        lock.unlock();
        not_empty_cv_.notify_one();

        return true;
    }

    /**
     * @brief Moves an element, waiting for capacity or cancellation.
     *
     * This overload blocks while the queue is full. If the wait is cancelled
     * before capacity becomes available, the queue is unchanged.
     * @param item Element to move into the queue.
     * @param stop Token used to cancel the wait.
     * @return @c true if inserted; @c false if stopped before capacity was available.
     */
    bool waitPush(T &&item, std::stop_token stop)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_full_cv_.wait(lock, stop, [this]
                               { return queue_.size() < capacity_; }))
        {
            return false;
        }

        queue_.push(std::move(item));

        lock.unlock();
        not_empty_cv_.notify_one();

        return true;
    }

    /**
     * @brief Attempts to remove the oldest element without blocking.
     * @return The oldest element, or std::nullopt if the queue is empty.
     */
    std::optional<T> tryPop()
    {
        std::optional<T> item;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (queue_.empty())
            {
                return std::nullopt;
            }

            item.emplace(std::move(queue_.front()));
            queue_.pop();
        }

        not_full_cv_.notify_one();

        return item;
    }

    /**
     * @brief Waits for an element and removes it, or returns when stopped.
     * @param stop Token used to cancel the wait.
     * @return The oldest element, or std::nullopt if stopped before
     *         an element becomes available.
     */
    std::optional<T> waitPop(std::stop_token stop)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_empty_cv_.wait(lock, stop, [this]
                                { return !queue_.empty(); }))
        {
            return std::nullopt;
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

        lock.unlock();
        not_full_cv_.notify_one();

        return item;
    }

    /**
     * @brief Checks whether the queue has reached its maximum capacity.
     * @return @c true if the queue is full; otherwise @c false.
     */
    bool isFull() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() == capacity_;
    }

    /**
     * @brief Checks whether the queue contains any elements.
     * @return @c true if the queue is empty; otherwise @c false.
     */
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Returns the number of elements currently stored.
     * @return A snapshot of the number of elements currently in the queue.
     */
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Returns the maximum number of elements the queue can store.
     * @return Maximum queue capacity.
     */
    std::size_t capacity() const noexcept
    {
        return capacity_;
    }

private:
    /**
     * @brief Validates the requested queue capacity.
     * @param capacity Requested maximum queue capacity.
     * @return Validated queue capacity.
     * @throws std::invalid_argument If @p capacity is zero.
     */
    static std::size_t validateCapacity(const std::size_t capacity)
    {
        if (capacity == 0)
        {
            throw std::invalid_argument(
                "ConcurrentBoundedQueue capacity must be greater than zero");
        }

        return capacity;
    }

    /// FIFO storage for queued elements.
    std::queue<T> queue_;

    /// Maximum number of elements the queue can store.
    const std::size_t capacity_;

    /// Protects access to the queue.
    mutable std::mutex mutex_;

    /// Wakes waiting consumers when an element is inserted.
    std::condition_variable_any not_empty_cv_;

    /// Wakes waiting producers when an element is removed.
    std::condition_variable_any not_full_cv_;
};
