/*****************************************************************************
 * @file container_status.hpp
 * @brief Defines the statuses returned by concurrent container operations.
 *****************************************************************************/
#pragma once

namespace systemcore::concurrency::containers
{

/**
 * @brief Describes the outcome of a concurrent container operation.
 */
enum class ContainerStatus
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

} // namespace systemcore::concurrency::containers
