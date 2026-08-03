#include "bittorrent/piece/storage_manager.hpp"
#include <system_error>
#include <stdexcept>
#include <algorithm>

namespace bittorrent::piece {

StorageManager::StorageManager(const std::filesystem::path& download_dir, const torrent::TorrentMetadata& metadata)
    : download_dir_(download_dir), metadata_(metadata) {}

StorageManager::~StorageManager() {}

void StorageManager::prepareFiles() {
    std::lock_guard<std::mutex> lock(io_mutex_);

    for (const auto& file_info : metadata_.files) {
        std::filesystem::path full_path = download_dir_ / file_info.path;
        std::filesystem::create_directories(full_path.parent_path());

        if (!std::filesystem::exists(full_path)) {
            std::ofstream file(full_path, std::ios::binary | std::ios::out);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to create file: " + full_path.string());
            }
            if (file_info.length > 0) {
                file.seekp(file_info.length - 1);
                file.put(0);
            }
            file.close();
        }
    }
}

void StorageManager::writeBlock(uint32_t piece_index, uint32_t offset, const std::vector<uint8_t>& data) {
    uint64_t global_offset = static_cast<uint64_t>(piece_index) * metadata_.piece_length + offset;
    writeBytes(global_offset, data.data(), data.size());
}

std::vector<uint8_t> StorageManager::readPiece(uint32_t piece_index) {
    uint64_t piece_size = metadata_.getPieceSize(piece_index);
    std::vector<uint8_t> buffer(piece_size);
    uint64_t global_offset = static_cast<uint64_t>(piece_index) * metadata_.piece_length;
    readBytes(global_offset, buffer.data(), piece_size);
    return buffer;
}

std::vector<uint8_t> StorageManager::readBlock(uint32_t piece_index, uint32_t offset, uint32_t length) {
    std::vector<uint8_t> buffer(length);
    uint64_t global_offset = static_cast<uint64_t>(piece_index) * metadata_.piece_length + offset;
    readBytes(global_offset, buffer.data(), length);
    return buffer;
}

void StorageManager::writeBytes(uint64_t global_offset, const uint8_t* buffer, size_t length) {
    std::lock_guard<std::mutex> lock(io_mutex_);

    uint64_t current_file_start = 0;
    size_t bytes_written = 0;

    for (const auto& file_info : metadata_.files) {
        uint64_t current_file_end = current_file_start + file_info.length;

        if (global_offset + bytes_written >= current_file_start && global_offset + bytes_written < current_file_end) {
            uint64_t offset_in_file = (global_offset + bytes_written) - current_file_start;
            size_t bytes_to_write = std::min(length - bytes_written, static_cast<size_t>(file_info.length - offset_in_file));

            std::filesystem::path full_path = download_dir_ / file_info.path;
            std::fstream file(full_path, std::ios::binary | std::ios::in | std::ios::out);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file for writing: " + full_path.string());
            }

            file.seekp(offset_in_file);
            file.write(reinterpret_cast<const char*>(buffer + bytes_written), bytes_to_write);
            file.close();

            bytes_written += bytes_to_write;
            if (bytes_written >= length) {
                break;
            }
        }
        current_file_start = current_file_end;
    }
}

void StorageManager::readBytes(uint64_t global_offset, uint8_t* buffer, size_t length) {
    std::lock_guard<std::mutex> lock(io_mutex_);

    uint64_t current_file_start = 0;
    size_t bytes_read = 0;

    for (const auto& file_info : metadata_.files) {
        uint64_t current_file_end = current_file_start + file_info.length;

        if (global_offset + bytes_read >= current_file_start && global_offset + bytes_read < current_file_end) {
            uint64_t offset_in_file = (global_offset + bytes_read) - current_file_start;
            size_t bytes_to_read = std::min(length - bytes_read, static_cast<size_t>(file_info.length - offset_in_file));

            std::filesystem::path full_path = download_dir_ / file_info.path;
            std::ifstream file(full_path, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file for reading: " + full_path.string());
            }

            file.seekg(offset_in_file);
            file.read(reinterpret_cast<char*>(buffer + bytes_read), bytes_to_read);
            file.close();

            bytes_read += bytes_to_read;
            if (bytes_read >= length) {
                break;
            }
        }
        current_file_start = current_file_end;
    }
}

} // namespace bittorrent::piece
