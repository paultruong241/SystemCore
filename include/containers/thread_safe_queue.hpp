/*****************************************************************************
 * @file thread_safe_queue.hpp
 * @brief Declares the ThreadSafeQueue class.
 *****************************************************************************/
#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

/**
 * @brief A thread-safe FIFO queue with blocking and non-blocking retrieval.
 * @tparam T Stored value type; retrieval requires construction from an rvalue.
 */
template <typename T>
class ThreadSafeQueue
{
public:
    /**
     * @brief Copies a value into the queue.
     * @param value The value to copy into the queue.
     */
    void push(const T &value)
    {
        {
            std::lock_guard<std::mutex> queue_lock(mutex_);
            queue_.push(value);
        }
        not_empty_cv_.notify_one();
    }

    /**
     * @brief Moves a value into the queue.
     * @param value The value to move into the queue.
     */
    void push(T &&value)
    {
        {
            std::lock_guard<std::mutex> queue_lock(mutex_);
            queue_.push(std::move(value));
        }
        not_empty_cv_.notify_one();
    }

    /**
     * @brief Removes and returns the next value, blocking until one is
     *        available.
     * @return The value moved from the front of the queue.
     */
    T pop()
    {
        std::unique_lock<std::mutex> queue_lock(mutex_);
        not_empty_cv_.wait(queue_lock, [this] { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * @brief Moves and returns the next value if one is available.
     * @return The value moved from the front, or @c std::nullopt if the queue
     *         is empty.
     */
    std::optional<T> try_pop()
    {
        std::lock_guard<std::mutex> queue_lock(mutex_);
        if (queue_.empty())
        {
            return std::nullopt;
        }
        std::optional<T> value(std::in_place, std::move(queue_.front()));
        queue_.pop();
        return value;
    }

    /**
     * @brief Waits up to the specified duration for a value, then removes and
     *        returns it.
     * @param timeout Maximum duration to wait for a value, in milliseconds.
     * @return The value moved from the front of the queue, or @c std::nullopt
     *         if the wait times out.
     */
    std::optional<T> wait_pop_for(const std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> queue_lock(mutex_);
        if (!not_empty_cv_.wait_for(queue_lock, timeout, [this] { return !queue_.empty(); }))
        {
            return std::nullopt;
        }
        std::optional<T> value(std::in_place, std::move(queue_.front()));
        queue_.pop();
        return value;
    }

private:
    /// FIFO storage for queued values.
    std::queue<T> queue_;

    /// Protects access to the queue.
    std::mutex mutex_;

    /// Wakes a waiting consumer when a value is added.
    std::condition_variable not_empty_cv_;
};