/*****************************************************************************
 * @file file_reader.cpp
 * @brief Implements the FileReader constructor.
 *****************************************************************************/

#include "io/files/file_reader.hpp"

FileReader::FileReader(const std::string &file_path,
                       const std::ios_base::openmode open_mode) :
    file_path_(file_path),
    file_size_bytes_(std::filesystem::file_size(file_path)),
    file_stream_(file_path, open_mode)
{
}

bool FileReader::isStreamUsable() const
{
    return file_stream_.is_open() && !file_stream_.bad();
}
