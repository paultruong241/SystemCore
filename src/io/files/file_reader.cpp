/*****************************************************************************
 * @file file_reader.cpp
 * @brief Implements input-file opening and metadata capture.
 *****************************************************************************/

#include "systemcore/io/files/file_reader.hpp"

namespace systemcore::io::files
{

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

} // namespace systemcore::io::files
