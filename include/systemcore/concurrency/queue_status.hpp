/*****************************************************************************
 * @file queue_status.hpp
 * @brief Defines operation statuses and pop results for concurrency classes.
 *****************************************************************************/
#pragma once

#include <optional>

namespace systemcore::concurrency
{

/**
 * @brief Describes the outcome of a queue or circular-buffer operation.
 */
enum class QueueStatus
{
    /// The operation succeeded.
    kSuccess,

    /// A non-blocking read found no item in an open container.
    kEmpty,

    /// A closed container rejected the operation or has no remaining items.
    kClosed,

    /// A wait ended because its stop token was requested.
    kStopped,

    /// A non-blocking insertion found a full bounded queue.
    kFull
};

/**
 * @brief Holds a pop status and its value when an item was retrieved.
 * @tparam T Type of value returned by a successful pop.
 *
 * The value is engaged only when @p status is @c QueueStatus::kSuccess.
 */
template <typename T>
struct QueueResult
{
    /// Outcome of the pop operation.
    QueueStatus status{QueueStatus::kEmpty};

    /// Retrieved value, present only on success.
    std::optional<T> value{};
};

} // namespace systemcore::concurrency
