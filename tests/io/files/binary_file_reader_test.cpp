/*****************************************************************************
 * @file binary_file_reader_test.cpp
 * @brief Tests reading binary files with BinaryFileReader.
 *****************************************************************************/

#include "io/files/binary_file_reader.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace io::files
{

/**
 * @brief Verifies that BinaryFileReader reads every byte, including null bytes.
 */
TEST(BinaryFileReaderTest, ReadsBinaryFileContents)
{
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto file_path = std::filesystem::temp_directory_path() /
                           ("systemcore_binary_reader_test_" +
                            std::to_string(timestamp) + ".bin");
    const std::array<std::byte, 3> expected = {
        std::byte{0x41},
        std::byte{0x00},
        std::byte{0x42},
    };

    {
        std::ofstream file(file_path, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file.write(reinterpret_cast<const char *>(expected.data()),
                   static_cast<std::streamsize>(expected.size()));
        ASSERT_TRUE(file.good());
    }

    {
        BinaryFileReader reader(file_path.string());
        std::vector<std::byte> contents;

        EXPECT_TRUE(reader.readChunk(contents, 2));
        EXPECT_EQ(contents, (std::vector<std::byte>{std::byte{0x41}, std::byte{0x00}}));

        EXPECT_TRUE(reader.readChunk(contents, 2));
        EXPECT_EQ(contents, (std::vector<std::byte>{std::byte{0x42}}));

        EXPECT_TRUE(reader.readChunk(contents, 2));
        EXPECT_TRUE(contents.empty());

        EXPECT_TRUE(reader.readAll(contents));
        EXPECT_EQ(contents, std::vector<std::byte>(expected.begin(), expected.end()));
    }

    std::error_code error;
    std::filesystem::remove(file_path, error);
    EXPECT_FALSE(error) << error.message();
}

/**
 * @brief Verifies failed reads clear the buffer and can be retried.
 */
TEST(BinaryFileReaderTest, FailedReadCanBeRetried)
{
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto file_path = std::filesystem::temp_directory_path() /
                           ("systemcore_binary_reader_retry_test_" +
                            std::to_string(timestamp) + ".bin");
    const std::array<std::byte, 4> expected = {
        std::byte{0x41},
        std::byte{0x42},
        std::byte{0x43},
        std::byte{0x44},
    };

    {
        std::ofstream file(file_path, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file.write(reinterpret_cast<const char *>(expected.data()),
                   static_cast<std::streamsize>(expected.size()));
        ASSERT_TRUE(file.good());
    }

    {
        BinaryFileReader reader(file_path.string());
        std::vector<std::byte> contents{std::byte{0xFF}};

        {
            std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(file.is_open());
            file.write(reinterpret_cast<const char *>(expected.data()), 2);
            ASSERT_TRUE(file.good());
        }

        EXPECT_FALSE(reader.readChunk(contents, 4));
        EXPECT_TRUE(contents.empty());
        EXPECT_FALSE(reader.readAll(contents));
        EXPECT_TRUE(contents.empty());

        {
            std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(file.is_open());
            file.write(reinterpret_cast<const char *>(expected.data()),
                       static_cast<std::streamsize>(expected.size()));
            ASSERT_TRUE(file.good());
        }

        EXPECT_TRUE(reader.readChunk(contents, 4));
        EXPECT_EQ(contents, (std::vector<std::byte>{expected[2], expected[3]}));

        EXPECT_TRUE(reader.readAll(contents));
        EXPECT_EQ(contents, std::vector<std::byte>(expected.begin(), expected.end()));
    }

    std::error_code error;
    std::filesystem::remove(file_path, error);
    EXPECT_FALSE(error) << error.message();
}

} // namespace io::files
