/*****************************************************************************
 * @file cout_capture.hpp
 * @brief Provides a utility for capturing standard output in tests.
 *****************************************************************************/
#pragma once

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

namespace test_utils
{

/**
 * @brief Captures standard output until the object is destroyed.
 */
class CoutCapture
{
public:
    CoutCapture() : previous_(std::cout.rdbuf(output_.rdbuf())) {}

    ~CoutCapture()
    {
        std::cout.rdbuf(previous_);
    }

    /**
     * @brief Returns the output captured so far.
     * @return Captured text.
     */
    std::string str() const
    {
        return output_.str();
    }

private:
    std::ostringstream output_;
    std::streambuf *previous_;
};

} // namespace test_utils
