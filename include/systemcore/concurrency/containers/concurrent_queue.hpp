/*****************************************************************************
 * @file concurrent_queue.hpp
 * @brief Declares an unbounded, thread-safe FIFO queue.
 *****************************************************************************/
#pragma once

#include "systemcore/concurrency/containers/pop_status.hpp"

#include <condition_variable>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <utility>

namespace systemcore::concurrency::containers
{

/**
 * @brief Provides an unbounded, thread-safe FIFO queue.
 *
 * Elements are inserted at the back and removed from the front. Retrieval can
 * either return immediately or wait until an element is available. Closing the
 * queue rejects new elements and lets consumers drain the remaining elements.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class ConcurrentQueue
{
public:
    /**
     * @brief Copies an element into the queue unless it is closed.
     * @param item Element to copy into the queue.
     * @return @c ContainerStatus::kSuccess if inserted, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    ContainerStatus push(const T &item)
        requires std::copy_constructible<T>
    {
        return pushImpl(item);
    }

    /**
     * @brief Moves an element into the queue unless it is closed.
     * @param item Element to move into the queue.
     * @return @c ContainerStatus::kSuccess if inserted, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    ContainerStatus push(T &&item)
        requires std::move_constructible<T>
    {
        return pushImpl(std::move(item));
    }

    /**
     * @brief Attempts to remove the oldest element without blocking.
     * @return The oldest value with @c ContainerStatus::kSuccess,
     *         @c ContainerStatus::kEmpty if the open queue is empty, or
     *         @c ContainerStatus::kClosed if the closed queue is empty.
     */
    PopResult<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
        {
            return {
                closed_ ? ContainerStatus::kClosed : ContainerStatus::kEmpty, {}};
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

        return {ContainerStatus::kSuccess, std::move(item)};
    }

    /**
     * @brief Waits for and removes an element, or returns when stopped or closed.
     * @param stop Token used to cancel the wait.
     * @return The oldest value with @c ContainerStatus::kSuccess,
     *         @c ContainerStatus::kStopped if cancelled, or
     *         @c ContainerStatus::kClosed if the closed queue is empty.
     */
    PopResult<T> waitPop(std::stop_token stop = {})
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_empty_cv_.wait(
                lock, stop, [this] { return closed_ || !queue_.empty(); }))
        {
            return {ContainerStatus::kStopped, {}};
        }

        if (queue_.empty())
        {
            return {ContainerStatus::kClosed, {}};
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

        return {ContainerStatus::kSuccess, std::move(item)};
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
     * @brief Checks whether the queue is closed.
     * @return @c true if closed; otherwise @c false.
     */
    bool isClosed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    /**
     * @brief Checks whether the queue contains no elements.
     * @return @c true if the queue is empty; otherwise @c false.
     */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Returns the current number of elements in the queue.
     * @return A snapshot of the queue size.
     */
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    /**
     * @brief Pushes an element into the queue by copying or moving it.
     * @tparam U Type deduced from the element being pushed.
     * @param item Element to copy or move into the queue.
     * @return @c ContainerStatus::kSuccess if inserted, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    template <typename U>
    ContainerStatus pushImpl(U &&item)
    {
        {
            std::lock_guard lock(mutex_);

            if (closed_)
            {
                return ContainerStatus::kClosed;
            }

            queue_.emplace(std::forward<U>(item));
        }

        not_empty_cv_.notify_one();
        return ContainerStatus::kSuccess;
    }

    /// FIFO storage for queued elements.
    std::queue<T> queue_;

    /// Protects access to the queue.
    mutable std::mutex mutex_;

    /// Rejects new elements and releases waiting consumers when set.
    bool closed_{false};

    /// Wakes waiting consumers when an element is added.
    std::condition_variable_any not_empty_cv_;
};

} // namespace systemcore::concurrency::containers
