#ifndef BITTORRENT_PEER_PEER_CONNECTION_HPP
#define BITTORRENT_PEER_PEER_CONNECTION_HPP

#include "bittorrent/peer/peer_message.hpp"
#include "bittorrent/piece/piece_manager.hpp"
#include "bittorrent/piece/rarest_first.hpp"
#include "bittorrent/piece/storage_manager.hpp"
#include <boost/asio.hpp>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <chrono>

namespace bittorrent::peer {

/**
 * @brief Asynchronous Peer Connection and Protocol State Machine.
 *
 * WHY: Each peer connection operates asynchronously using Boost.Asio sockets and timers.
 * Manages peer handshakes, choking/interested state machines, block pipelining,
 * and block upload responses.
 */
class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
public:
    /**
     * @brief Constructor.
     * @param io_context Shared Boost.Asio io_context.
     * @param ip Peer IPv4 address.
     * @param port Peer TCP port.
     * @param info_hash 20-byte torrent info hash.
     * @param peer_id 20-byte client peer ID.
     * @param piece_manager Shared PieceManager.
     * @param storage Shared StorageManager.
     * @param rarest_first Shared RarestFirstSelector.
     */
    PeerConnection(boost::asio::io_context& io_context,
                   const std::string& ip,
                   uint16_t port,
                   const std::string& info_hash,
                   const std::string& peer_id,
                   std::shared_ptr<piece::PieceManager> piece_manager,
                   std::shared_ptr<piece::StorageManager> storage,
                   std::shared_ptr<piece::RarestFirstSelector> rarest_first);

    ~PeerConnection();

    /**
     * @brief Start async connection and handshake sequence.
     */
    void start();

    /**
     * @brief Gracefully disconnect socket and cancel timers.
     */
    void disconnect();

    bool isConnected() const { return is_connected_; }
    bool isPeerChoking() const { return peer_choking_; }
    bool isAmInterested() const { return am_interested_; }
    const std::vector<bool>& getPeerBitfield() const { return peer_bitfield_; }

private:
    void doConnect();
    void doHandshake();
    void doReadHeader();
    void doReadBody(uint32_t length);
    void sendMessage(const PeerMessage& msg);
    void doWrite();

    void handleMessage(MessageType type, const std::vector<uint8_t>& payload);
    void requestBlocksPipeline();

    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::steady_timer timeout_timer_;

    std::string ip_;
    uint16_t port_;
    std::string info_hash_;
    std::string peer_id_;
    std::string remote_peer_id_;

    std::shared_ptr<piece::PieceManager> piece_manager_;
    std::shared_ptr<piece::StorageManager> storage_;
    std::shared_ptr<piece::RarestFirstSelector> rarest_first_;

    bool is_connected_{false};

    // Protocol State Machine Flags
    bool am_choking_{true};       /**< Client is choking peer */
    bool am_interested_{false};   /**< Client is interested in peer */
    bool peer_choking_{true};     /**< Peer is choking client */
    bool peer_interested_{false}; /**< Peer is interested in client */

    std::vector<bool> peer_bitfield_;
    int current_downloading_piece_{-1};
    int in_flight_requests_{0};
    static constexpr int MAX_IN_FLIGHT = 5;

    // Buffer queues
    std::vector<uint8_t> read_buffer_;
    std::deque<std::vector<uint8_t>> write_queue_;
};

} // namespace bittorrent::peer

#endif // BITTORRENT_PEER_PEER_CONNECTION_HPP
