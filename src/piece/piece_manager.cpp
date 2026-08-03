#include "bittorrent/piece/piece_manager.hpp"
#include "bittorrent/crypto/sha1.hpp"
#include "bittorrent/utils/logger.hpp"
#include <algorithm>

namespace bittorrent::piece {

PieceManager::PieceManager(const torrent::TorrentMetadata& metadata, std::shared_ptr<StorageManager> storage)
    : metadata_(metadata), storage_(storage) {
    size_t num_pieces = metadata_.numPieces();
    pieces_.resize(num_pieces);

    for (size_t i = 0; i < num_pieces; ++i) {
        pieces_[i].index = static_cast<uint32_t>(i);
        pieces_[i].length = metadata_.getPieceSize(i);
        pieces_[i].expected_hash = metadata_.piece_hashes[i];
        pieces_[i].is_complete = false;
        pieces_[i].is_downloading = false;

        size_t num_blocks = (pieces_[i].length + DEFAULT_BLOCK_SIZE - 1) / DEFAULT_BLOCK_SIZE;
        pieces_[i].num_blocks = num_blocks;
        pieces_[i].blocks_received = 0;
        pieces_[i].block_states.assign(num_blocks, BlockState::MISSING);
        pieces_[i].data_buffer.resize(pieces_[i].length);
    }
}

void PieceManager::initializeBitfield(const std::vector<bool>& verified_bitfield) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t len = std::min(pieces_.size(), verified_bitfield.size());

    for (size_t i = 0; i < len; ++i) {
        if (verified_bitfield[i]) {
            pieces_[i].is_complete = true;
            pieces_[i].is_downloading = false;
            pieces_[i].blocks_received = pieces_[i].num_blocks;
            std::fill(pieces_[i].block_states.begin(), pieces_[i].block_states.end(), BlockState::RECEIVED);
            downloaded_bytes_ += pieces_[i].length;
        }
    }
}

std::vector<bool> PieceManager::getBitfield() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<bool> bitfield(pieces_.size(), false);
    for (size_t i = 0; i < pieces_.size(); ++i) {
        bitfield[i] = pieces_[i].is_complete;
    }
    return bitfield;
}

bool PieceManager::isComplete() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& p : pieces_) {
        if (!p.is_complete) return false;
    }
    return true;
}

std::optional<Block> PieceManager::getNextBlockRequest(uint32_t piece_index) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (piece_index >= pieces_.size()) return std::nullopt;

    auto& p = pieces_[piece_index];
    if (p.is_complete) return std::nullopt;

    p.is_downloading = true;

    for (size_t b = 0; b < p.num_blocks; ++b) {
        if (p.block_states[b] == BlockState::MISSING) {
            p.block_states[b] = BlockState::REQUESTED;

            Block block;
            block.piece_index = piece_index;
            block.offset = static_cast<uint32_t>(b * DEFAULT_BLOCK_SIZE);

            if (b == p.num_blocks - 1) {
                uint32_t rem = p.length % DEFAULT_BLOCK_SIZE;
                block.length = (rem == 0) ? DEFAULT_BLOCK_SIZE : rem;
            } else {
                block.length = DEFAULT_BLOCK_SIZE;
            }

            return block;
        }
    }

    return std::nullopt;
}

bool PieceManager::saveBlock(uint32_t piece_index, uint32_t offset, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (piece_index >= pieces_.size()) return false;

    auto& p = pieces_[piece_index];
    if (p.is_complete) return false;

    size_t block_idx = offset / DEFAULT_BLOCK_SIZE;
    if (block_idx >= p.num_blocks) return false;

    if (p.block_states[block_idx] != BlockState::RECEIVED) {
        p.block_states[block_idx] = BlockState::RECEIVED;
        p.blocks_received++;

        // Copy block into memory buffer
        if (offset + data.size() <= p.data_buffer.size()) {
            std::copy(data.begin(), data.end(), p.data_buffer.begin() + offset);
        }
    }

    // Check if piece is fully assembled
    if (p.blocks_received >= p.num_blocks) {
        // Compute SHA-1 hash of piece buffer
        std::string computed_hash = crypto::SHA1::hash(
            std::span<const uint8_t>(p.data_buffer.data(), p.data_buffer.size()));

        if (computed_hash == p.expected_hash) {
            // Write piece to disk via StorageManager
            storage_->writeBlock(piece_index, 0, p.data_buffer);
            p.is_complete = true;
            p.is_downloading = false;
            downloaded_bytes_ += p.length;
            utils::Logger::getInstance().info("Piece #" + std::to_string(piece_index) + " verified SHA-1 OK");
            return true;
        } else {
            utils::Logger::getInstance().warn("Piece #" + std::to_string(piece_index) + " SHA-1 hash mismatch! Retrying piece.");
            // Reset piece state
            p.is_complete = false;
            p.is_downloading = false;
            p.blocks_received = 0;
            std::fill(p.block_states.begin(), p.block_states.end(), BlockState::MISSING);
            return false;
        }
    }

    return false;
}

void PieceManager::resetPiece(uint32_t piece_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (piece_index < pieces_.size()) {
        auto& p = pieces_[piece_index];
        if (!p.is_complete) {
            p.is_downloading = false;
            p.blocks_received = 0;
            std::fill(p.block_states.begin(), p.block_states.end(), BlockState::MISSING);
        }
    }
}

bool PieceManager::verifyPieceHash(uint32_t piece_index) {
    try {
        std::vector<uint8_t> piece_data = storage_->readPiece(piece_index);
        std::string computed_hash = crypto::SHA1::hash(
            std::span<const uint8_t>(piece_data.data(), piece_data.size()));
        return computed_hash == metadata_.piece_hashes[piece_index];
    } catch (...) {
        return false;
    }
}

uint64_t PieceManager::getDownloadedBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return downloaded_bytes_;
}

size_t PieceManager::getCompletedPieceCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& p : pieces_) {
        if (p.is_complete) count++;
    }
    return count;
}

} // namespace bittorrent::piece
