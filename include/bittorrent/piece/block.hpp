#ifndef BITTORRENT_PIECE_BLOCK_HPP
#define BITTORRENT_PIECE_BLOCK_HPP

#include <cstdint>
#include <vector>

namespace bittorrent::piece {

/**
 * @brief Standard block length requested over BitTorrent wire protocol (16 KB / 16384 bytes).
 */
constexpr uint32_t DEFAULT_BLOCK_SIZE = 16384;

/**
 * @brief Represents a single block request or payload within a piece.
 */
struct Block {
    uint32_t piece_index{0};           /**< Index of piece */
    uint32_t offset{0};                /**< Byte offset within piece */
    uint32_t length{DEFAULT_BLOCK_SIZE}; /**< Byte size of block */
    std::vector<uint8_t> data;         /**< Payload bytes (empty for request messages) */
};

/**
 * @brief State tracking for individual piece blocks.
 */
enum class BlockState {
    MISSING,
    REQUESTED,
    RECEIVED
};

} // namespace bittorrent::piece

#endif // BITTORRENT_PIECE_BLOCK_HPP
