/*****************************************************************************
 * @file concurrent_bounded_queue.hpp
 * @brief Declares a fixed-capacity, thread-safe FIFO queue.
 *****************************************************************************/
#pragma once

#include "systemcore/concurrency/containers/pop_status.hpp"

#include <condition_variable>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace systemcore::concurrency::containers
{

/**
 * @brief Provides a fixed-capacity, thread-safe FIFO queue.
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
    explicit ConcurrentBoundedQueue(
        const std::size_t capacity = kDefaultCapacity)
        : capacity_(validateCapacity(capacity))
    {
    }

    /**
     * @brief Attempts to copy an element into the queue without blocking.
     * @param item Element to copy into the queue.
     * @return @c ContainerStatus::kSuccess if inserted,
     *         @c ContainerStatus::kFull if full, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    ContainerStatus tryPush(const T &item)
        requires std::copy_constructible<T>
    {
        return tryPushImpl(item);
    }

    /**
     * @brief Attempts to move an element into the queue without blocking.
     * @param item Element to move into the queue.
     * @return @c ContainerStatus::kSuccess if inserted,
     *         @c ContainerStatus::kFull if full, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    ContainerStatus tryPush(T &&item)
        requires std::move_constructible<T>
    {
        return tryPushImpl(std::move(item));
    }

    /**
     * @brief Copies an element, waiting for capacity, cancellation, or closure.
     *
     * This overload blocks while the queue is full. If the wait is cancelled
     * before capacity becomes available or if the queue is closed, the queue
     * remains unchanged.
     * @param item Element to copy into the queue.
     * @param stop Token used to cancel the wait. Defaults to a non-stoppable token.
     * @return @c ContainerStatus::kSuccess if inserted,
     *         @c ContainerStatus::kStopped if cancelled, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    ContainerStatus waitPush(const T &item, std::stop_token stop = {})
        requires std::copy_constructible<T>
    {
        return waitPushImpl(item, stop);
    }

    /**
     * @brief Moves an element, waiting for capacity, cancellation, or closure.
     *
     * This overload blocks while the queue is full. If the wait is cancelled
     * before capacity becomes available, or if the queue is closed, the queue
     * remains unchanged.
     * @param item Element to move into the queue.
     * @param stop Token used to cancel the wait. Defaults to a non-stoppable token.
     * @return @c ContainerStatus::kSuccess if inserted,
     *         @c ContainerStatus::kStopped if cancelled, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    ContainerStatus waitPush(T &&item, std::stop_token stop = {})
        requires std::move_constructible<T>
    {
        return waitPushImpl(std::move(item), stop);
    }

    /**
     * @brief Attempts to remove the oldest element without blocking.
     * @return The oldest value with @c ContainerStatus::kSuccess,
     *         @c ContainerStatus::kEmpty if the open queue is empty, or
     *         @c ContainerStatus::kClosed if the closed queue is empty.
     */
    PopResult<T> tryPop()
    {
        std::optional<T> item;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (queue_.empty())
            {
                return {
                    closed_ ? ContainerStatus::kClosed : ContainerStatus::kEmpty,
                    {}};
            }

            item.emplace(std::move(queue_.front()));
            queue_.pop();
        }

        not_full_cv_.notify_one();

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

        lock.unlock();
        not_full_cv_.notify_one();

        return {ContainerStatus::kSuccess, std::move(item)};
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
     * @brief Checks whether the queue is closed.
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
     * @brief Returns the current number of elements in the queue.
     * @return A snapshot of the queue size.
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

    /**
     * @brief Attempts to push an element into the queue without waiting.
     * @tparam U Type deduced from the element being pushed.
     * @param item Element to copy or move into the queue.
     * @return @c ContainerStatus::kSuccess if inserted,
     *         @c ContainerStatus::kFull if full, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    template <typename U>
    ContainerStatus tryPushImpl(U &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (closed_)
            {
                return ContainerStatus::kClosed;
            }
            if (queue_.size() == capacity_)
            {
                return ContainerStatus::kFull;
            }

            queue_.emplace(std::forward<U>(item));
        }

        not_empty_cv_.notify_one();

        return ContainerStatus::kSuccess;
    }

    /**
     * @brief Waits for space to enqueue an element, cancellation, or closure.
     * @tparam U Type deduced from the element being pushed.
     * @param item Element to copy or move into the queue.
     * @param stop Token used to cancel the wait.
     * @return @c ContainerStatus::kSuccess if inserted,
     *         @c ContainerStatus::kStopped if cancelled, or
     *         @c ContainerStatus::kClosed if the queue is closed.
     */
    template <typename U>
    ContainerStatus waitPushImpl(U &&item, std::stop_token stop = {})
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!not_full_cv_.wait(
                    lock, stop, [this] { return closed_ || queue_.size() < capacity_; }))
            {
                return ContainerStatus::kStopped;
            }
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

} // namespace systemcore::concurrency::containers
