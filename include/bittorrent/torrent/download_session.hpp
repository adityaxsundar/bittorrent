#ifndef BITTORRENT_TORRENT_DOWNLOAD_SESSION_HPP
#define BITTORRENT_TORRENT_DOWNLOAD_SESSION_HPP

#include "bittorrent/torrent/torrent_info.hpp"
#include "bittorrent/torrent/torrent_parser.hpp"
#include "bittorrent/torrent/resume_manager.hpp"
#include "bittorrent/piece/piece_manager.hpp"
#include "bittorrent/piece/storage_manager.hpp"
#include "bittorrent/piece/rarest_first.hpp"
#include "bittorrent/peer/peer_connection.hpp"
#include "bittorrent/tracker/http_tracker.hpp"
#include "bittorrent/network/io_context_pool.hpp"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <atomic>
#include <chrono>

namespace bittorrent::torrent {

/**
 * @brief Main BitTorrent Download Session Controller.
 *
 * WHY: Ties together metadata parsing, storage pre-allocation, fast-resume, tracker communication,
 * peer connection pools, piece managers, and UI progress reporting into a unified execution loop.
 */
class DownloadSession {
public:
    /**
     * @brief Constructor.
     * @param torrent_path Path to input .torrent file.
     * @param output_dir Destination output directory.
     * @param peer_id Client peer ID (20 bytes).
     * @param port Client listening port.
     */
    DownloadSession(const std::filesystem::path& torrent_path,
                    const std::filesystem::path& output_dir,
                    const std::string& peer_id = "-BT2000-123456789012",
                    uint16_t port = 6881);

    ~DownloadSession();

    /**
     * @brief Start torrent download session and block until complete or paused.
     */
    void start();

    /**
     * @brief Pause downloading session.
     */
    void pause();

    /**
     * @brief Resume downloading session.
     */
    void resume();

    /**
     * @brief Force full SHA-1 hash re-verification of target files on disk.
     */
    bool verifyHashes();

    /**
     * @brief Access torrent metadata.
     */
    const TorrentMetadata& getMetadata() const { return metadata_; }

    /**
     * @brief Get active connected peer count.
     */
    size_t getConnectedPeerCount() const;

private:
    std::filesystem::path torrent_path_;
    std::filesystem::path output_dir_;
    std::string peer_id_;
    uint16_t port_;

    TorrentMetadata metadata_;
    std::filesystem::path resume_file_path_;

    std::shared_ptr<piece::StorageManager> storage_;
    std::shared_ptr<piece::PieceManager> piece_manager_;
    std::shared_ptr<piece::RarestFirstSelector> rarest_first_;

    network::IOContextPool io_pool_;
    std::vector<std::shared_ptr<peer::PeerConnection>> peers_;

    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_paused_{false};

    void connectToPeers(const std::vector<tracker::PeerEndpoint>& endpoints);
    void saveCheckpoint();
};

} // namespace bittorrent::torrent

#endif // BITTORRENT_TORRENT_DOWNLOAD_SESSION_HPP
