/*****************************************************************************
 * @file binary_file_reader.hpp
 * @brief BinaryFileReader class declaration.
 *****************************************************************************/
#pragma once

#include "io/files/file_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace io::files
{

/**
 * @brief Provides binary file reading in full or bounded chunks.
 *
 * Reads are bounded by the file size recorded when the reader is constructed.
 * Appended data is ignored, and truncating the file can cause a read to fail.
 */
class BinaryFileReader : public FileReader
{
public:
    /**
     * @brief Constructs a binary reader for the specified file.
     * @param file_path Path to the file to read.
     */
    BinaryFileReader(const std::string &file_path);

    /**
     * @brief Reads the file size recorded at construction from the beginning.
     *
     * Appended data is not included; truncating the file can cause the read to fail.
     * The buffer is empty on failure. A retry clears EOF/failure flags and starts
     * from the beginning; a bad stream state requires constructing a new reader.
     * @param buffer Receives the file bytes; it is empty on failure.
     * @return @c true if the full read succeeds; otherwise, @c false.
     */
    bool readAll(std::vector<std::byte> &buffer);

    /**
     * @brief Reads up to the requested number of bytes from the current position.
     *
     * The available byte count is based on the file size recorded at construction.
     * Appended data is ignored, and truncating the file can cause a read to fail.
     * The buffer is empty on failure. A retry clears EOF/failure flags and
     * continues at the current position; a bad stream state requires a new reader.
     * @param buffer Receives the bytes read; it is empty on failure.
     * @param max_bytes Maximum number of bytes to read.
     * @return @c true if the read succeeds, including when no bytes remain;
     *         otherwise, @c false.
     */
    bool readChunk(std::vector<std::byte> &buffer, std::uintmax_t max_bytes);

private:
    /**
     * @brief Reads an exact byte count from the stream into the buffer.
     * @param buffer Receives the bytes read; it is empty on failure.
     * @param byte_count Number of bytes to read.
     * @return @c true if the requested bytes are read; otherwise, @c false.
     */
    bool readBytes(std::vector<std::byte> &buffer, std::uintmax_t byte_count);
};

} // namespace io::files