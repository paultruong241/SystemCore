/*****************************************************************************
 * @file bounded_queue.hpp
 * @brief Declares a fixed-capacity, thread-safe FIFO queue.
 *****************************************************************************/
#pragma once

#include "systemcore/concurrency/queue_status.hpp"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace systemcore::concurrency
{

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
 * Closing the queue rejects new elements and lets consumers drain existing
 * elements. Existing elements are never overwritten.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class BoundedQueue
{
public:
    /// Default capacity used when no capacity is specified.
    static constexpr std::size_t kDefaultCapacity{100};

    /**
     * @brief Constructs a concurrent bounded queue with the specified capacity.
     * @param capacity Maximum number of elements the queue can store.
     * @throws std::invalid_argument If @p capacity is zero.
     */
    explicit BoundedQueue(const std::size_t capacity = kDefaultCapacity)
        : capacity_(validateCapacity(capacity))
    {
    }

    /**
     * @brief Attempts to copy an element into the queue without blocking.
     * @param item Element to copy into the queue.
     * @return @c kSuccess if inserted, @c kFull if full, or @c kClosed if closed.
     */
    QueueStatus tryPush(const T &item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (closed_)
            {
                return QueueStatus::kClosed;
            }
            if (queue_.size() == capacity_)
            {
                return QueueStatus::kFull;
            }

            queue_.push(item);
        }

        not_empty_cv_.notify_one();

        return QueueStatus::kSuccess;
    }

    /**
     * @brief Attempts to move an element into the queue without blocking.
     * @param item Element to move into the queue.
     * @return @c kSuccess if inserted, @c kFull if full, or @c kClosed if closed.
     */
    QueueStatus tryPush(T &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (closed_)
            {
                return QueueStatus::kClosed;
            }
            if (queue_.size() == capacity_)
            {
                return QueueStatus::kFull;
            }

            queue_.push(std::move(item));
        }

        not_empty_cv_.notify_one();

        return QueueStatus::kSuccess;
    }

    /**
     * @brief Copies an element, waiting for capacity, cancellation, or closure.
     *
     * This overload blocks while the queue is full. If the wait is cancelled
     * before capacity becomes available, or if the queue is closed, the queue
     * is unchanged.
     * @param item Element to copy into the queue.
     * @param stop Token used to cancel the wait.
     * @return @c kSuccess if inserted, @c kStopped if cancelled, or
     *         @c kClosed if the queue is closed.
     */
    QueueStatus waitPush(const T &item, std::stop_token stop)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_full_cv_.wait(lock, stop, [this]
                               { return closed_ || queue_.size() < capacity_; }))
        {
            return QueueStatus::kStopped;
        }
        if (closed_)
        {
            return QueueStatus::kClosed;
        }

        queue_.push(item);

        lock.unlock();
        not_empty_cv_.notify_one();

        return QueueStatus::kSuccess;
    }

    /**
     * @brief Moves an element, waiting for capacity, cancellation, or closure.
     *
     * This overload blocks while the queue is full. If the wait is cancelled
     * before capacity becomes available, or if the queue is closed, the queue
     * is unchanged.
     * @param item Element to move into the queue.
     * @param stop Token used to cancel the wait.
     * @return @c kSuccess if inserted, @c kStopped if cancelled, or
     *         @c kClosed if the queue is closed.
     */
    QueueStatus waitPush(T &&item, std::stop_token stop)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_full_cv_.wait(lock, stop, [this]
                               { return closed_ || queue_.size() < capacity_; }))
        {
            return QueueStatus::kStopped;
        }
        if (closed_)
        {
            return QueueStatus::kClosed;
        }

        queue_.push(std::move(item));

        lock.unlock();
        not_empty_cv_.notify_one();

        return QueueStatus::kSuccess;
    }

    /**
     * @brief Attempts to remove the oldest element without blocking.
     * @return The oldest value and @c kSuccess, @c kEmpty if open and empty,
     *         or @c kClosed if closed and empty.
     */
    QueueResult<T> tryPop()
    {
        std::optional<T> item;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (queue_.empty())
            {
                return {closed_ ? QueueStatus::kClosed : QueueStatus::kEmpty, {}};
            }

            item.emplace(std::move(queue_.front()));
            queue_.pop();
        }

        not_full_cv_.notify_one();

        return {QueueStatus::kSuccess, std::move(item)};
    }

    /**
     * @brief Waits for an element and removes it, or returns when stopped or closed.
     * @param stop Token used to cancel the wait.
     * @return The oldest value and @c kSuccess, @c kStopped if cancelled, or
     *         @c kClosed if closed after all queued elements are removed.
     */
    QueueResult<T> waitPop(std::stop_token stop)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_empty_cv_.wait(lock, stop, [this]
                                { return closed_ || !queue_.empty(); }))
        {
            return {QueueStatus::kStopped, {}};
        }

        if (queue_.empty())
        {
            return {QueueStatus::kClosed, {}};
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

        lock.unlock();
        not_full_cv_.notify_one();

        return {QueueStatus::kSuccess, std::move(item)};
    }

    /**
     * @brief Closes the queue and wakes all waiting producers and consumers.
     *
     * Existing elements remain available for retrieval. Repeated calls have
     * no additional effect.
     */
    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    /**
     * @brief Checks whether the queue has been closed.
     * @return @c true if closed; otherwise @c false.
     */
    bool isClosed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    /**
     * @brief Checks whether the queue has reached its maximum capacity.
     * @return @c true if the queue is full; otherwise @c false.
     */
    bool full() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() == capacity_;
    }

    /**
     * @brief Checks whether the queue contains any elements.
     * @return @c true if the queue is empty; otherwise @c false.
     */
    bool empty() const
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
                "BoundedQueue capacity must be greater than zero");
        }

        return capacity;
    }

    /// FIFO storage for queued elements.
    std::queue<T> queue_;

    /// Maximum number of elements the queue can store.
    const std::size_t capacity_;

    /// Protects access to the queue.
    mutable std::mutex mutex_;

    /// Rejects new elements and releases waiting producers and consumers.
    bool closed_{false};

    /// Wakes waiting consumers when an element is inserted.
    std::condition_variable_any not_empty_cv_;

    /// Wakes waiting producers when an element is removed.
    std::condition_variable_any not_full_cv_;
};

} // namespace systemcore::concurrency
