#ifndef BITTORRENT_TRACKER_HTTP_TRACKER_HPP
#define BITTORRENT_TRACKER_HTTP_TRACKER_HPP

#include "bittorrent/tracker/tracker_response.hpp"
#include "bittorrent/torrent/torrent_info.hpp"
#include <boost/asio.hpp>
#include <string>
#include <memory>

namespace bittorrent::tracker {

/**
 * @brief Event state sent to tracker during announce requests.
 */
enum class TrackerEvent {
    STARTED,
    STOPPED,
    COMPLETED,
    NONE
};

/**
 * @brief HTTP Tracker Client using Boost.Asio asynchronous networking.
 *
 * WHY: Communicates with BitTorrent HTTP trackers to announce client state
 * (uploaded, downloaded, left bytes) and discover active swarm peers.
 */
class HTTPTracker {
public:
    /**
     * @brief Constructor.
     * @param io_context Shared Boost.Asio io_context.
     */
    explicit HTTPTracker(boost::asio::io_context& io_context);

    /**
     * @brief Announce client state to tracker and receive peer endpoints.
     * @param announce_url Main tracker HTTP URL (e.g. http://tracker.example.com:6969/announce).
     * @param info_hash Raw 20-byte binary info hash.
     * @param peer_id Raw 20-byte binary client peer ID.
     * @param port Client listening TCP port (default 6881).
     * @param uploaded Bytes uploaded.
     * @param downloaded Bytes downloaded.
     * @param left Bytes remaining to download.
     * @param event Announce event (STARTED, STOPPED, COMPLETED, NONE).
     * @return Parsed TrackerResponse struct.
     */
    TrackerResponse announce(const std::string& announce_url,
                             const std::string& info_hash,
                             const std::string& peer_id,
                             uint16_t port,
                             uint64_t uploaded,
                             uint64_t downloaded,
                             uint64_t left,
                             TrackerEvent event = TrackerEvent::NONE);

    /**
     * @brief Parse raw HTTP response body containing bencoded tracker payload.
     * @param response_body Binary body content of HTTP response.
     * @return Parsed TrackerResponse struct.
     */
    static TrackerResponse parseTrackerResponseBody(const std::string& response_body);

private:
    boost::asio::io_context& io_context_;

    struct ParsedURL {
        std::string host;
        std::string port;
        std::string path;
    };

    ParsedURL parseURL(const std::string& url);
    std::string eventToString(TrackerEvent event) const;
};

} // namespace bittorrent::tracker

#endif // BITTORRENT_TRACKER_HTTP_TRACKER_HPP
