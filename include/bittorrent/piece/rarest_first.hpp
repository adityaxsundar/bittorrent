#ifndef BITTORRENT_PIECE_RAREST_FIRST_HPP
#define BITTORRENT_PIECE_RAREST_FIRST_HPP

#include <vector>
#include <mutex>
#include <cstdint>
#include <algorithm>
#include <random>

namespace bittorrent::piece {

/**
 * @brief Rarest First Piece Selection Algorithm.
 *
 * WHY: Downloading pieces in order of lowest availability (rarity) across connected peers
 * ensures rarest pieces are replicated in the swarm first, preventing piece bottlenecks
 * and improving swarm health.
 */
class RarestFirstSelector {
public:
    /**
     * @brief Initialize selector with total piece count.
     * @param num_pieces Total pieces in torrent.
     */
    explicit RarestFirstSelector(size_t num_pieces);

    /**
     * @brief Update piece counts when a peer sends a bitfield or HAVES message.
     * @param bitfield Peer piece availability vector.
     */
    void addPeerBitfield(const std::vector<bool>& bitfield);

    /**
     * @brief Decrement piece availability counts when a peer disconnects.
     * @param bitfield Peer bitfield vector.
     */
    void removePeerBitfield(const std::vector<bool>& bitfield);

    /**
     * @brief Increment piece availability when a peer announces a HAVE message.
     * @param piece_index Index of piece peer acquired.
     */
    void incrementPieceCount(size_t piece_index);

    /**
     * @brief Select next piece index to download based on rarity.
     * @param peer_bitfield Piece bitfield of candidate peer.
     * @param client_bitfield Bitfield of pieces client already owns or is downloading.
     * @return 0-based piece index if available, or -1 if no suitable piece found.
     */
    int selectNextPiece(const std::vector<bool>& peer_bitfield, const std::vector<bool>& client_bitfield);

private:
    size_t num_pieces_;
    std::vector<uint32_t> piece_counts_;
    std::mutex mutex_;
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace bittorrent::piece

#endif // BITTORRENT_PIECE_RAREST_FIRST_HPP
