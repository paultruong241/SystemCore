/*****************************************************************************
 * @file file_reader.hpp
 * @brief Declares the base FileReader class.
 *****************************************************************************/
#pragma once

#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

/**
 * @brief Provides input-file access and records its path and size.
 */
class FileReader
{
public:
    /// Maximum byte count accepted by a stream read operation.
    static constexpr std::streamsize kMaxStreamReadSize =
        std::numeric_limits<std::streamsize>::max();

    /**
     * @brief Opens the specified file and records its size.
     * @param file_path Path to the file to open.
     * @param open_mode Flags for opening the input stream; defaults to @c std::ios::in.
     * @throws std::filesystem::filesystem_error If the file size cannot be queried.
     */
    FileReader(const std::string &file_path,
               const std::ios_base::openmode open_mode = std::ios::in);

    /// Supports safe destruction through a FileReader pointer.
    virtual ~FileReader() = default;

    /// Copy construction is disabled.
    FileReader(const FileReader &) = delete;

    /// Copy assignment is disabled.
    FileReader &operator=(const FileReader &) = delete;

protected:
    /// Path of the opened file.
    const std::filesystem::path file_path_;

    /// File size in bytes captured at construction; not updated if the file changes.
    const std::uintmax_t file_size_bytes_;

    /// Input stream for the file.
    std::ifstream file_stream_;
};