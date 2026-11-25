/*****************************************************************************
 * @file binary_file_reader.cpp
 * @brief Implements binary file reading.
 *****************************************************************************/

#include "systemcore/io/files/binary_file_reader.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

namespace systemcore::io::files
{

BinaryFileReader::BinaryFileReader(const std::string &file_path) :
    FileReader(file_path, std::ios::in | std::ios::binary)
{
}

bool BinaryFileReader::readAll(std::vector<std::byte> &buffer)
{
    buffer.clear();
    if (!isStreamUsable())
    {
        std::cerr << "BinaryFileReader::readAll(): File stream is not open or "
                     "is in a bad state."
                  << std::endl;
        return false;
    }

    file_stream_.clear();

    file_stream_.seekg(0, std::ios::beg);
    if (!file_stream_)
    {
        std::cerr << "BinaryFileReader::readAll(): Failed to seek to the start of the file."
                  << std::endl;
        return false;
    }

    return readBytes(buffer, file_size_bytes_);
}

bool BinaryFileReader::readChunk(std::vector<std::byte> &buffer,
                                 const std::uintmax_t max_bytes)
{
    buffer.clear();
    if (!isStreamUsable())
    {
        std::cerr << "BinaryFileReader::readChunk(): File stream is not open or "
                     "is in a bad state."
                  << std::endl;
        return false;
    }

    // Clear EOF/failure flags so a retry can continue at the current position.
    file_stream_.clear();
    const auto position = file_stream_.tellg();
    if (position == std::ifstream::pos_type(-1))
    {
        std::cerr << "BinaryFileReader::readChunk(): Failed to get the file position."
                  << std::endl;
        return false;
    }

    const auto offset = static_cast<std::streamoff>(position);
    if (offset < 0)
    {
        std::cerr << "BinaryFileReader::readChunk(): Invalid file position." << std::endl;
        return false;
    }

    // Convert the current position to bytes from the start of the file.
    const auto offset_bytes = static_cast<std::uintmax_t>(offset);
    // Avoid underflow if the position is beyond the file size recorded at open.
    const auto remaining_bytes =
        offset_bytes < file_size_bytes_ ? file_size_bytes_ - offset_bytes : 0;
    // Read no more than either the caller's limit or the bytes remaining.
    const auto byte_count = std::min(max_bytes, remaining_bytes);

    return readBytes(buffer, byte_count);
}

bool BinaryFileReader::readBytes(std::vector<std::byte> &buffer,
                                 const std::uintmax_t byte_count)
{
    if (byte_count > static_cast<std::uintmax_t>(buffer.max_size()) ||
        byte_count > static_cast<std::uintmax_t>(FileReader::kMaxStreamReadSize))
    {
        std::cerr << "BinaryFileReader: Requested read is too large." << std::endl;
        return false;
    }

    buffer.resize(static_cast<std::size_t>(byte_count));
    if (byte_count == 0)
    {
        return true;
    }

    if (!file_stream_.read(reinterpret_cast<char *>(buffer.data()),
                           static_cast<std::streamsize>(byte_count)))
    {
        buffer.clear();
        std::cerr << "BinaryFileReader: Failed to read the requested bytes." << std::endl;
        return false;
    }

    return true;
}

} // namespace systemcore::io::files
