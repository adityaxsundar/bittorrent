#ifndef BITTORRENT_PIECE_STORAGE_MANAGER_HPP
#define BITTORRENT_PIECE_STORAGE_MANAGER_HPP

#include "bittorrent/torrent/torrent_info.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <cstdint>

namespace bittorrent::piece {

/**
 * @brief Thread-safe file system I/O manager for reading and writing piece blocks.
 *
 * WHY: A piece may span across file boundaries in multi-file torrents.
 * StorageManager translates global piece index and block offsets into specific file paths and offsets on disk.
 */
class StorageManager {
public:
    /**
     * @brief Constructor.
     * @param download_dir Base download directory.
     * @param metadata TorrentMetadata containing file list and piece sizes.
     */
    StorageManager(const std::filesystem::path& download_dir, const torrent::TorrentMetadata& metadata);
    ~StorageManager();

    /**
     * @brief Create target files and directories on disk, pre-allocating file size.
     */
    void prepareFiles();

    /**
     * @brief Write block data at calculated global torrent offset.
     * @param piece_index 0-based piece index.
     * @param offset Byte offset within piece.
     * @param data Raw block byte buffer.
     */
    void writeBlock(uint32_t piece_index, uint32_t offset, const std::vector<uint8_t>& data);

    /**
     * @brief Read a complete piece into memory.
     * @param piece_index Piece index to read.
     * @return Raw byte vector containing full piece data.
     */
    std::vector<uint8_t> readPiece(uint32_t piece_index);

    /**
     * @brief Read a specific block for peer upload requests.
     * @param piece_index Piece index.
     * @param offset Offset within piece.
     * @param length Byte length to read.
     * @return Block byte vector.
     */
    std::vector<uint8_t> readBlock(uint32_t piece_index, uint32_t offset, uint32_t length);

private:
    std::filesystem::path download_dir_;
    torrent::TorrentMetadata metadata_;
    std::mutex io_mutex_;

    void writeBytes(uint64_t global_offset, const uint8_t* buffer, size_t length);
    void readBytes(uint64_t global_offset, uint8_t* buffer, size_t length);
};

} // namespace bittorrent::piece

#endif // BITTORRENT_PIECE_STORAGE_MANAGER_HPP
