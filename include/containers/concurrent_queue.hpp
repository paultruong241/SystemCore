/*****************************************************************************
 * @file concurrent_queue.hpp
 * @brief Declares the unbounded ConcurrentQueue class.
 *****************************************************************************/
#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <utility>

/**
 * @brief A thread-safe, unbounded FIFO queue.
 *
 * Elements are inserted at the back and removed from the front. Retrieval can
 * either return immediately or wait until an element is available.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class ConcurrentQueue
{
public:
    /**
     * @brief Copies an element into the queue.
     * @param item Element to copy into the queue.
     */
    void push(const T &item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item);
        }

        not_empty_cv_.notify_one();
    }

    /**
     * @brief Moves an element into the queue.
     * @param item Element to move into the queue.
     */
    void push(T &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }

        not_empty_cv_.notify_one();
    }

    /**
     * @brief Attempts to remove the oldest element without blocking.
     * @return The oldest element, or std::nullopt if the queue is empty.
     */
    std::optional<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
        {
            return std::nullopt;
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

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

        if (!not_empty_cv_.wait(
                lock, stop, [this]{ return !queue_.empty(); }))
        {
            return std::nullopt;
        }

        std::optional<T> item(
            std::in_place, std::move(queue_.front()));

        queue_.pop();

        return item;
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

    /// Wakes waiting consumers when an element is added.
    std::condition_variable_any not_empty_cv_;
};
