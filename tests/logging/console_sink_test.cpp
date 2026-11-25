/*****************************************************************************
 * @file console_sink_test.cpp
 * @brief Tests console sink routing and output formatting.
 *****************************************************************************/

#include "systemcore/logging/formatters/text_formatter.hpp"
#include "systemcore/logging/sinks/console_sink.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace systemcore::logging
{

namespace
{

class StreamCapture
{
public:
    explicit StreamCapture(std::ostream &stream)
        : stream_(stream), previous_(stream_.rdbuf(output_.rdbuf()))
    {
    }

    ~StreamCapture()
    {
        stream_.rdbuf(previous_);
    }

    std::string str() const
    {
        return output_.str();
    }

private:
    std::ostream &stream_;
    std::ostringstream output_;
    std::streambuf *previous_;
};

} // namespace

TEST(ConsoleSinkTest, RoutesWarningAndHigherSeverityToStandardError)
{
    StreamCapture standard_output(std::cout);
    StreamCapture standard_error(std::cerr);
    ConsoleSink sink(std::make_shared<TextFormatter>());

    LogRecord info_record;
    info_record.level = LogLevel::kInfo;
    info_record.message = "info message";
    sink.write(info_record);

    LogRecord warning_record;
    warning_record.level = LogLevel::kWarning;
    warning_record.message = "warning message";
    sink.write(warning_record);
    sink.flush();

    EXPECT_NE(standard_output.str().find("info message"), std::string::npos);
    EXPECT_NE(standard_output.str().find('\n'), std::string::npos);
    EXPECT_NE(standard_error.str().find("warning message"), std::string::npos);
    EXPECT_NE(standard_error.str().find('\n'), std::string::npos);
}

TEST(ConsoleSinkTest, RejectsNullFormatter)
{
    EXPECT_THROW(
        (void)ConsoleSink(std::shared_ptr<const ILogFormatter>{}),
        std::invalid_argument);
}

} // namespace systemcore::logging
