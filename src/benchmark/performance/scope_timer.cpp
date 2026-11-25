/*****************************************************************************
 * @file scope_timer.cpp
 * @brief Implements the ScopeTimer class.
 *****************************************************************************/

#include "benchmark/performance/scope_timer.hpp"

#include <chrono>
#include <iostream>

namespace benchmark::performance
{

ScopeTimer::ScopeTimer(const std::string_view name)
    : m_name(name), m_start(std::chrono::steady_clock::now())
{
}

ScopeTimer::~ScopeTimer()
{
    const auto end = std::chrono::steady_clock::now();
    const auto duration =
        std::chrono::duration<double, std::milli>(end - m_start).count();
    std::cout << "[Timer] " << m_name << ": " << duration << " ms\n";
}

} // namespace benchmark::performance
