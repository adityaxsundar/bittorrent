#ifndef BITTORRENT_PEER_PEER_MESSAGE_HPP
#define BITTORRENT_PEER_PEER_MESSAGE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <span>

namespace bittorrent::peer {

/**
 * @brief Wire protocol message types according to BitTorrent specification.
 */
enum class MessageType : uint8_t {
    KEEP_ALIVE   = 255, /**< Length 0 message */
    CHOKE        = 0,   /**< Length 1, ID 0 */
    UNCHOKE      = 1,   /**< Length 1, ID 1 */
    INTERESTED   = 2,   /**< Length 1, ID 2 */
    NOT_INTERESTED = 3, /**< Length 1, ID 3 */
    HAVE         = 4,   /**< Length 5, ID 4, payload: 4-byte piece_index */
    BITFIELD     = 5,   /**< Length 1+N, ID 5, payload: bitfield bytes */
    REQUEST      = 6,   /**< Length 13, ID 6, payload: 4-byte index, 4-byte offset, 4-byte length */
    PIECE        = 7,   /**< Length 9+N, ID 7, payload: 4-byte index, 4-byte offset, data bytes */
    CANCEL       = 8    /**< Length 13, ID 8, payload: 4-byte index, 4-byte offset, 4-byte length */
};

/**
 * @brief BitTorrent Peer Wire Protocol Handshake structure (68 bytes).
 */
struct Handshake {
    static constexpr size_t HANDSHAKE_SIZE = 68;
    static constexpr const char* PROTOCOL_STRING = "BitTorrent protocol";

    std::string pstr{"BitTorrent protocol"};
    std::string reserved{std::string(8, '\0')};
    std::string info_hash; // 20 bytes
    std::string peer_id;   // 20 bytes

    /**
     * @brief Serialize Handshake into 68-byte binary buffer.
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Parse 68-byte binary buffer into Handshake struct.
     */
    static Handshake deserialize(std::span<const uint8_t> data);
};

/**
 * @brief BitTorrent Wire Protocol Message.
 *
 * Encapsulates length prefix, message ID, and payload bytes.
 */
class PeerMessage {
public:
    MessageType type{MessageType::KEEP_ALIVE};
    std::vector<uint8_t> payload;

    /**
     * @brief Serialize message into binary wire format [4-byte len][1-byte id][payload].
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Factory methods for creating standard protocol messages.
     */
    static PeerMessage createKeepAlive();
    static PeerMessage createChoke();
    static PeerMessage createUnchoke();
    static PeerMessage createInterested();
    static PeerMessage createNotInterested();
    static PeerMessage createHave(uint32_t piece_index);
    static PeerMessage createBitfield(const std::vector<bool>& bitfield);
    static PeerMessage createRequest(uint32_t piece_index, uint32_t offset, uint32_t length);
    static PeerMessage createPiece(uint32_t piece_index, uint32_t offset, const std::vector<uint8_t>& data);
    static PeerMessage createCancel(uint32_t piece_index, uint32_t offset, uint32_t length);
};

} // namespace bittorrent::peer

#endif // BITTORRENT_PEER_PEER_MESSAGE_HPP
