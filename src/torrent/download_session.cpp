#include "bittorrent/torrent/download_session.hpp"
#include "bittorrent/utils/logger.hpp"
#include "bittorrent/utils/progress_bar.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace bittorrent::torrent {

DownloadSession::DownloadSession(const std::filesystem::path& torrent_path,
                                 const std::filesystem::path& output_dir,
                                 const std::string& peer_id,
                                 uint16_t port)
    : torrent_path_(torrent_path),
      output_dir_(output_dir),
      peer_id_(peer_id),
      port_(port) {
    metadata_ = TorrentParser::parseFile(torrent_path_);
    resume_file_path_ = output_dir_ / (metadata_.name + ".fastresume");

    storage_ = std::make_shared<piece::StorageManager>(output_dir_, metadata_);
    storage_->prepareFiles();

    piece_manager_ = std::make_shared<piece::PieceManager>(metadata_, storage_);
    rarest_first_ = std::make_shared<piece::RarestFirstSelector>(metadata_.numPieces());

    // Check fast-resume state
    std::vector<bool> resume_bitfield;
    uint64_t downloaded = 0, uploaded = 0;
    if (ResumeManager::loadResumeState(resume_file_path_, metadata_.info_hash_hex, resume_bitfield, downloaded, uploaded)) {
        piece_manager_->initializeBitfield(resume_bitfield);
        utils::Logger::getInstance().info("Loaded fast-resume state for " + metadata_.name);
    }
}

DownloadSession::~DownloadSession() {
    saveCheckpoint();
}

void DownloadSession::start() {
    is_running_ = true;
    is_paused_ = false;

    utils::Logger::getInstance().info("Starting download session for: " + metadata_.name);
    utils::Logger::getInstance().info("Info Hash: " + metadata_.info_hash_hex);
    utils::Logger::getInstance().info("Total Length: " + utils::ProgressBar::formatBytes(metadata_.total_length));

    io_pool_.start();

    // Tracker announcement
    if (!metadata_.announce.empty()) {
        try {
            tracker::HTTPTracker tracker(io_pool_.getIOContext());
            uint64_t downloaded = piece_manager_->getDownloadedBytes();
            uint64_t left = metadata_.total_length > downloaded ? metadata_.total_length - downloaded : 0;

            tracker::TrackerResponse resp = tracker.announce(
                metadata_.announce, metadata_.info_hash, peer_id_, port_, 0, downloaded, left, tracker::TrackerEvent::STARTED);

            utils::Logger::getInstance().info("Discovered " + std::to_string(resp.peers.size()) + " peers from tracker");
            connectToPeers(resp.peers);
        } catch (const std::exception& e) {
            utils::Logger::getInstance().warn("Tracker announce warning: " + std::string(e.what()));
        }
    }

    // Monitoring loop
    uint64_t prev_downloaded = piece_manager_->getDownloadedBytes();
    auto prev_time = std::chrono::steady_clock::now();

    while (is_running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (is_paused_) continue;

        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - prev_time).count();
        uint64_t current_downloaded = piece_manager_->getDownloadedBytes();

        double speed = 0.0;
        if (elapsed_sec > 0 && current_downloaded >= prev_downloaded) {
            speed = (current_downloaded - prev_downloaded) / elapsed_sec;
        }

        prev_downloaded = current_downloaded;
        prev_time = now;

        utils::ProgressBar::render(current_downloaded,
                                   metadata_.total_length,
                                   piece_manager_->getCompletedPieceCount(),
                                   metadata_.numPieces(),
                                   getConnectedPeerCount(),
                                   speed);

        if (piece_manager_->isComplete()) {
            std::cout << "\n[SUCCESS] Download finished and verified for: " << metadata_.name << std::endl;
            break;
        }

        saveCheckpoint();
    }

    io_pool_.stop();
}

void DownloadSession::pause() {
    is_paused_ = true;
    utils::Logger::getInstance().info("Download session paused");
}

void DownloadSession::resume() {
    is_paused_ = false;
    utils::Logger::getInstance().info("Download session resumed");
}

bool DownloadSession::verifyHashes() {
    utils::Logger::getInstance().info("Verifying all pieces against SHA-1 hashes...");
    size_t total = metadata_.numPieces();
    size_t valid_count = 0;

    for (size_t i = 0; i < total; ++i) {
        if (piece_manager_->verifyPieceHash(i)) {
            valid_count++;
        }
    }

    utils::Logger::getInstance().info("SHA-1 Verification Result: " + std::to_string(valid_count) + "/" + std::to_string(total) + " pieces verified valid.");
    return valid_count == total;
}

size_t DownloadSession::getConnectedPeerCount() const {
    size_t count = 0;
    for (const auto& peer : peers_) {
        if (peer->isConnected()) count++;
    }
    return count;
}

void DownloadSession::connectToPeers(const std::vector<tracker::PeerEndpoint>& endpoints) {
    for (const auto& ep : endpoints) {
        auto peer_conn = std::make_shared<peer::PeerConnection>(
            io_pool_.getIOContext(), ep.ip, ep.port, metadata_.info_hash, peer_id_,
            piece_manager_, storage_, rarest_first_);

        peers_.push_back(peer_conn);
        peer_conn->start();
    }
}

void DownloadSession::saveCheckpoint() {
    if (piece_manager_) {
        ResumeManager::saveResumeState(resume_file_path_,
                                       metadata_.info_hash_hex,
                                       piece_manager_->getBitfield(),
                                       piece_manager_->getDownloadedBytes(),
                                       0);
    }
}

} // namespace bittorrent::torrent
