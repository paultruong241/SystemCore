/*****************************************************************************
 * @file scope_timer.cpp
 * @brief Implements scope timing and reports elapsed time on destruction.
 *****************************************************************************/

#include "systemcore/benchmark/performance/scope_timer.hpp"

#include <chrono>
#include <iostream>

namespace systemcore::benchmark::performance
{

ScopeTimer::ScopeTimer(const std::string_view name)
    : name_(name), start_(std::chrono::steady_clock::now())
{
}

ScopeTimer::~ScopeTimer()
{
    const auto end = std::chrono::steady_clock::now();
    const auto duration =
        std::chrono::duration<double, std::milli>(end - start_).count();
    std::cout << "[Timer] " << name_ << ": " << duration << " ms\n";
}

} // namespace systemcore::benchmark::performance
