#include "bittorrent/piece/rarest_first.hpp"

namespace bittorrent::piece {

RarestFirstSelector::RarestFirstSelector(size_t num_pieces)
    : num_pieces_(num_pieces), piece_counts_(num_pieces, 0) {}

void RarestFirstSelector::addPeerBitfield(const std::vector<bool>& bitfield) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t len = std::min(num_pieces_, bitfield.size());
    for (size_t i = 0; i < len; ++i) {
        if (bitfield[i]) {
            piece_counts_[i]++;
        }
    }
}

void RarestFirstSelector::removePeerBitfield(const std::vector<bool>& bitfield) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t len = std::min(num_pieces_, bitfield.size());
    for (size_t i = 0; i < len; ++i) {
        if (bitfield[i] && piece_counts_[i] > 0) {
            piece_counts_[i]--;
        }
    }
}

void RarestFirstSelector::incrementPieceCount(size_t piece_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (piece_index < num_pieces_) {
        piece_counts_[piece_index]++;
    }
}

int RarestFirstSelector::selectNextPiece(const std::vector<bool>& peer_bitfield, const std::vector<bool>& client_bitfield) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<size_t> candidate_pieces;
    uint32_t min_count = UINT32_MAX;

    size_t len = std::min({num_pieces_, peer_bitfield.size(), client_bitfield.size()});

    for (size_t i = 0; i < len; ++i) {
        // Must be available on peer AND missing on client
        if (peer_bitfield[i] && !client_bitfield[i]) {
            uint32_t count = piece_counts_[i];
            if (count < min_count) {
                min_count = count;
                candidate_pieces.clear();
                candidate_pieces.push_back(i);
            } else if (count == min_count) {
                candidate_pieces.push_back(i);
            }
        }
    }

    if (candidate_pieces.empty()) {
        return -1;
    }

    // Pick randomly among tied rarest pieces to avoid duplicate simultaneous requests across peers
    std::uniform_int_distribution<size_t> dist(0, candidate_pieces.size() - 1);
    return static_cast<int>(candidate_pieces[dist(rng_)]);
}

} // namespace bittorrent::piece
