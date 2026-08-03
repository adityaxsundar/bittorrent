#ifndef BITTORRENT_TRACKER_TRACKER_RESPONSE_HPP
#define BITTORRENT_TRACKER_TRACKER_RESPONSE_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace bittorrent::tracker {

/**
 * @brief Represents a peer endpoint discovered from tracker.
 */
struct PeerEndpoint {
    std::string ip;
    uint16_t port{0};
    std::string peer_id;
};

/**
 * @brief Parsed tracker response metadata.
 *
 * WHY: Decouples raw bencoded HTTP tracker responses into structured peer endpoints
 * and re-announce interval timings.
 */
struct TrackerResponse {
    uint32_t interval{1800};             /**< Re-announce interval in seconds */
    std::string tracker_id;               /**< Optional tracker string ID */
    uint32_t complete{0};                 /**< Seeder count */
    uint32_t incomplete{0};               /**< Leecher count */
    std::vector<PeerEndpoint> peers;     /**< Discovered peer list */
    std::string failure_reason;          /**< Non-empty if tracker reported failure */
};

} // namespace bittorrent::tracker

#endif // BITTORRENT_TRACKER_TRACKER_RESPONSE_HPP
