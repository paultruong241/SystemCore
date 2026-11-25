/*****************************************************************************
 * @file queue.hpp
 * @brief Declares an unbounded, thread-safe FIFO queue.
 *****************************************************************************/
#pragma once

#include "systemcore/concurrency/queue_status.hpp"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <utility>

namespace systemcore::concurrency
{

/**
 * @brief A thread-safe, unbounded FIFO queue.
 *
 * Elements are inserted at the back and removed from the front. Retrieval can
 * either return immediately or wait until an element is available. Closing the
 * queue rejects new elements and lets consumers drain the remaining elements.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class Queue
{
public:
    /**
     * @brief Copies an element into the queue unless it is closed.
     * @param item Element to copy into the queue.
     * @return @c kSuccess if inserted; @c kClosed if the queue is closed.
     */
    QueueStatus push(const T &item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_)
            {
                return QueueStatus::kClosed;
            }
            queue_.push(item);
        }

        not_empty_cv_.notify_one();
        return QueueStatus::kSuccess;
    }

    /**
     * @brief Moves an element into the queue unless it is closed.
     * @param item Element to move into the queue.
     * @return @c kSuccess if inserted; @c kClosed if the queue is closed.
     */
    QueueStatus push(T &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_)
            {
                return QueueStatus::kClosed;
            }
            queue_.push(std::move(item));
        }

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
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
        {
            return {closed_ ? QueueStatus::kClosed : QueueStatus::kEmpty, {}};
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

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

        if (!not_empty_cv_.wait(
                lock, stop, [this] { return closed_ || !queue_.empty(); }))
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

        return {QueueStatus::kSuccess, std::move(item)};
    }

    /**
     * @brief Closes the queue and wakes all waiting consumers.
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
     * @brief Checks whether the queue is empty.
     * @return @c true if the queue is empty; otherwise @c false.
     */
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Returns the number of elements currently in the queue.
     * @return A snapshot of the number of queued elements.
     */
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    /// FIFO storage for queued elements.
    std::queue<T> queue_;

    /// Protects access to the queue.
    mutable std::mutex mutex_;

    /// Rejects new elements and releases waiting consumers when set.
    bool closed_{false};

    /// Wakes waiting consumers when an element is added.
    std::condition_variable_any not_empty_cv_;
};

} // namespace systemcore::concurrency
