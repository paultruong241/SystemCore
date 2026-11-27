/*****************************************************************************
 * @file pop_status.hpp
 * @brief Defines the results returned by concurrent container pop operations.
 *****************************************************************************/
#pragma once

#include "systemcore/concurrency/containers/container_status.hpp"

#include <optional>

namespace systemcore::concurrency::containers
{

/**
 * @brief Holds the status and value returned by a pop operation.
 * @tparam T Type of value returned by a successful pop.
 *
 * The value is present only when @p status is
 * @c ContainerStatus::kSuccess.
 */
template <typename T>
struct PopResult
{
    /// Outcome of the pop operation.
    ContainerStatus status{ContainerStatus::kEmpty};

    /// Retrieved value, present only on success.
    std::optional<T> value{};

    /**
     * @brief Checks whether the pop operation retrieved a value.
     * @return @c true if the result contains a value; otherwise @c false.
     */
    [[nodiscard]]
    bool hasValue() const noexcept
    {
        return status == ContainerStatus::kSuccess && value.has_value();
    }

    /**
     * @brief Checks whether the pop operation retrieved a value.
     * @return @c true if the result contains a value; otherwise @c false.
     */
    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return hasValue();
    }
};

} // namespace systemcore::concurrency::containers