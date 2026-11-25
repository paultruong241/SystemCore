/*****************************************************************************
 * @file scope_timer.hpp
 * @brief Declares the scope-based timer.
 *****************************************************************************/
#pragma once

#include <chrono>
#include <string_view>

namespace systemcore::benchmark::performance
{

/**
 * @brief Measures a scope's elapsed time and reports it to standard output.
 *
 * The elapsed duration is measured with a steady clock and printed in
 * milliseconds when the timer is destroyed.
 */
class ScopeTimer
{
public:
    /**
     * @brief Starts timing the scope.
     * @param name Label printed with the elapsed time. The referenced
     *             characters must remain valid until this timer is destroyed.
     */
    explicit ScopeTimer(std::string_view name);

    /**
     * @brief Stops timing and prints the elapsed duration in milliseconds.
     */
    ~ScopeTimer();

    /// Copying a scoped timer is disabled.
    ScopeTimer(const ScopeTimer &) = delete;

    /// Copy assignment for a scoped timer is disabled.
    ScopeTimer& operator=(const ScopeTimer &) = delete;

private:
    /// Label printed with the elapsed duration.
    std::string_view m_name;

    /// Time at which measurement began.
    std::chrono::time_point<std::chrono::steady_clock> m_start;
};

} // namespace systemcore::benchmark::performance
