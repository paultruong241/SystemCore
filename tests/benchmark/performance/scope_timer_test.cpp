/*****************************************************************************
 * @file scope_timer_test.cpp
 * @brief Tests elapsed-time reporting by ScopeTimer.
 *****************************************************************************/

#include "systemcore/benchmark/performance/scope_timer.hpp"
#include "utils/cout_capture.hpp"

#include <gtest/gtest.h>

#include <string>

namespace systemcore::benchmark::performance
{

/**
 * @brief Verifies that the timer reports its label and elapsed milliseconds.
 */
TEST(ScopeTimerTest, ReportsElapsedTimeWhenDestroyed)
{
    test_utils::CoutCapture output;
    {
        ScopeTimer timer("sample operation");
    }

    const std::string text = output.str();
    EXPECT_TRUE(text.starts_with("[Timer] sample operation: "));
    EXPECT_TRUE(text.ends_with(" ms\n"));
}

} // namespace systemcore::benchmark::performance
