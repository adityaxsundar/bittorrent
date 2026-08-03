#ifndef BITTORRENT_TORRENT_TORRENT_INFO_HPP
#define BITTORRENT_TORRENT_TORRENT_INFO_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace bittorrent::torrent {

/**
 * @brief Represents a single file within a torrent dataset.
 */
struct TorrentFile {
    std::filesystem::path path; /**< Relative file path */
    uint64_t length{0};         /**< File size in bytes */
};

/**
 * @brief Complete metadata structure extracted from a `.torrent` file.
 *
 * WHY: High-level BitTorrent logic requires structured metadata (piece length, info_hash, file list)
 * decoupled from low-level bencoding representation.
 */
struct TorrentMetadata {
    std::string announce;                    /**< Main Tracker URL */
    std::vector<std::string> announce_list;  /**< Backup Tracker Tier URLs */
    std::string comment;                     /**< Optional creator comment */
    std::string created_by;                  /**< Optional application identifier */
    uint64_t creation_date{0};               /**< Creation timestamp (POSIX time) */

    // Info dictionary properties
    std::string name;                        /**< Target file or directory name */
    uint64_t piece_length{0};                /**< Byte size per piece (e.g. 262144, 524288) */
    std::vector<std::string> piece_hashes;   /**< 20-byte raw SHA-1 binary hash string per piece */
    uint64_t total_length{0};                /**< Combined size of all files in torrent */

    std::vector<TorrentFile> files;          /**< File layout (single-file or multi-file) */
    bool is_multi_file{false};               /**< True if multi-file mode */

    std::string info_hash;                   /**< 20-byte raw binary SHA-1 digest of raw info dict */
    std::string info_hash_hex;               /**< 40-character hex string representation */

    /**
     * @brief Compute total piece count.
     */
    size_t numPieces() const {
        return piece_hashes.size();
    }

    /**
     * @brief Calculate exact byte length for a given piece index.
     * @param piece_index 0-based piece index.
     * @return Byte length of piece (last piece may be smaller than piece_length).
     */
    uint64_t getPieceSize(size_t piece_index) const {
        if (piece_index >= numPieces()) return 0;
        if (piece_index == numPieces() - 1) {
            uint64_t remainder = total_length % piece_length;
            return (remainder == 0) ? piece_length : remainder;
        }
        return piece_length;
    }
};

} // namespace bittorrent::torrent

#endif // BITTORRENT_TORRENT_TORRENT_INFO_HPP
