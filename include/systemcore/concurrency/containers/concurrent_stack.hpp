/*****************************************************************************
 * @file concurrent_stack.hpp
 * @brief Declares an unbounded, thread-safe LIFO stack.
 *****************************************************************************/
#pragma once

#include "systemcore/concurrency/containers/pop_status.hpp"

#include <condition_variable>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <optional>
#include <stack>
#include <stop_token>
#include <utility>

namespace systemcore::concurrency::containers
{

/**
 * @brief Provides an unbounded, thread-safe LIFO stack.
 *
 * Elements are inserted and removed from the top of the stack. Retrieval can
 * return immediately or wait until an element is available. Closing the stack
 * rejects new elements and lets consumers drain the remaining elements.
 *
 * @tparam T Type of elements stored in the stack.
 */
template <typename T>
class ConcurrentStack
{
public:
    /**
     * @brief Copies an element into the stack unless it is closed.
     * @param item Element to copy into the stack.
     * @return @c ContainerStatus::kSuccess if inserted, or
     *         @c ContainerStatus::kClosed if the stack is closed.
     */
    ContainerStatus push(const T &item)
        requires std::copy_constructible<T>
    {
        return pushImpl(item);
    }

    /**
     * @brief Moves an element into the stack unless it is closed.
     * @param item Element to move into the stack.
     * @return @c ContainerStatus::kSuccess if inserted, or
     *         @c ContainerStatus::kClosed if the stack is closed.
     */
    ContainerStatus push(T &&item)
        requires std::move_constructible<T>
    {
        return pushImpl(std::move(item));
    }

    /**
     * @brief Attempts to remove the newest element without blocking.
     * @return The newest value with @c ContainerStatus::kSuccess,
     *         @c ContainerStatus::kEmpty if the open stack is empty, or
     *         @c ContainerStatus::kClosed if the closed stack is empty.
     */
    PopResult<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stack_.empty())
        {
            return {
                closed_ ? ContainerStatus::kClosed : ContainerStatus::kEmpty, {}};
        }

        std::optional<T> item(std::in_place, std::move(stack_.top()));
        stack_.pop();

        return {ContainerStatus::kSuccess, std::move(item)};
    }

    /**
     * @brief Waits for and removes an element, or returns when stopped or closed.
     * @param stop Token used to cancel the wait.
     * @return The newest value with @c ContainerStatus::kSuccess,
     *         @c ContainerStatus::kStopped if cancelled, or
     *         @c ContainerStatus::kClosed if the closed stack is empty.
     */
    PopResult<T> waitPop(std::stop_token stop = {})
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!not_empty_cv_.wait(
                lock, stop, [this] { return closed_ || !stack_.empty(); }))
        {
            return {ContainerStatus::kStopped, {}};
        }

        if (stack_.empty())
        {
            return {ContainerStatus::kClosed, {}};
        }

        std::optional<T> item(std::in_place, std::move(stack_.top()));
        stack_.pop();

        return {ContainerStatus::kSuccess, std::move(item)};
    }

    /**
     * @brief Closes the stack and wakes all waiting consumers.
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
     * @brief Checks whether the stack is closed.
     * @return @c true if closed; otherwise @c false.
     */
    bool isClosed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    /**
     * @brief Checks whether the stack contains no elements.
     * @return @c true if the stack is empty; otherwise @c false.
     */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stack_.empty();
    }

    /**
     * @brief Returns the current number of elements in the stack.
     * @return A snapshot of the stack size.
     */
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stack_.size();
    }

private:
    /**
     * @brief Pushes an element into the stack by copying or moving it.
     * @tparam U Type deduced from the element being pushed.
     * @param item Element to copy or move into the stack.
     * @return @c ContainerStatus::kSuccess if inserted, or
     *         @c ContainerStatus::kClosed if the stack is closed.
     */
    template <typename U>
    ContainerStatus pushImpl(U &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (closed_)
            {
                return ContainerStatus::kClosed;
            }

            stack_.emplace(std::forward<U>(item));
        }

        not_empty_cv_.notify_one();
        return ContainerStatus::kSuccess;
    }

    /// LIFO storage for stack elements.
    std::stack<T> stack_;

    /// Protects access to the stack.
    mutable std::mutex mutex_;

    /// Rejects new elements and releases waiting consumers when set.
    bool closed_{false};

    /// Wakes waiting consumers when an element is added.
    std::condition_variable_any not_empty_cv_;
};

} // namespace systemcore::concurrency::containers
