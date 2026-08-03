#ifndef BITTORRENT_PIECE_PIECE_MANAGER_HPP
#define BITTORRENT_PIECE_PIECE_MANAGER_HPP

#include "bittorrent/piece/block.hpp"
#include "bittorrent/piece/storage_manager.hpp"
#include "bittorrent/torrent/torrent_info.hpp"
#include <vector>
#include <mutex>
#include <optional>
#include <cstdint>
#include <memory>

namespace bittorrent::piece {

/**
 * @brief Complete piece state tracking structure.
 */
struct PieceState {
    uint32_t index{0};
    uint64_t length{0};
    std::string expected_hash;
    bool is_complete{false};
    bool is_downloading{false};

    size_t num_blocks{0};
    size_t blocks_received{0};
    std::vector<BlockState> block_states;
    std::vector<uint8_t> data_buffer;
};

/**
 * @brief Central Piece and Block Manager.
 *
 * WHY: Manages life cycle of torrent piece blocks, ensures SHA-1 cryptographic integrity,
 * interface with StorageManager, and supports partial downloads by excluding unwanted piece indices.
 */
class PieceManager {
public:
    /**
     * @brief Constructor.
     * @param metadata Torrent metadata containing hashes and piece sizes.
     * @param storage Shared StorageManager instance.
     */
    PieceManager(const torrent::TorrentMetadata& metadata, std::shared_ptr<StorageManager> storage);

    /**
     * @brief Mark pieces as completed (used during fast-resume initialization).
     * @param verified_bitfield Vector of completed piece flags.
     */
    void initializeBitfield(const std::vector<bool>& verified_bitfield);

    /**
     * @brief Get client's current piece completion bitfield.
     */
    std::vector<bool> getBitfield() const;

    /**
     * @brief Check if all selected pieces are downloaded and verified.
     */
    bool isComplete() const;

    /**
     * @brief Next block request generator for a specific piece.
     * @param piece_index 0-based piece index.
     * @return Optional Block request parameter (piece_index, offset, length).
     */
    std::optional<Block> getNextBlockRequest(uint32_t piece_index);

    /**
     * @brief Process an incoming block payload received from a peer.
     * @param piece_index Piece index.
     * @param offset Block offset within piece.
     * @param data Payload byte vector.
     * @return True if receiving this block completed and successfully verified the entire piece.
     */
    bool saveBlock(uint32_t piece_index, uint32_t offset, const std::vector<uint8_t>& data);

    /**
     * @brief Reset piece state on hash failure so blocks can be re-requested.
     * @param piece_index Piece index to reset.
     */
    void resetPiece(uint32_t piece_index);

    /**
     * @brief Verify piece hash on disk.
     * @param piece_index Piece index.
     * @return True if SHA-1 digest matches expected piece hash.
     */
    bool verifyPieceHash(uint32_t piece_index);

    /**
     * @brief Total downloaded bytes count.
     */
    uint64_t getDownloadedBytes() const;

    /**
     * @brief Total completed piece count.
     */
    size_t getCompletedPieceCount() const;

    /**
     * @brief Total piece count.
     */
    size_t getTotalPieces() const { return metadata_.numPieces(); }

private:
    torrent::TorrentMetadata metadata_;
    std::shared_ptr<StorageManager> storage_;
    std::vector<PieceState> pieces_;
    mutable std::mutex mutex_;
    uint64_t downloaded_bytes_{0};
};

} // namespace bittorrent::piece

#endif // BITTORRENT_PIECE_PIECE_MANAGER_HPP
