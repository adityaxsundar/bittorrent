#include "bittorrent/peer/peer_message.hpp"
#include "bittorrent/utils/bytes.hpp"
#include <stdexcept>

namespace bittorrent::peer {

std::vector<uint8_t> Handshake::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(HANDSHAKE_SIZE);

    buffer.push_back(static_cast<uint8_t>(pstr.size()));
    buffer.insert(buffer.end(), pstr.begin(), pstr.end());
    buffer.insert(buffer.end(), reserved.begin(), reserved.end());
    buffer.insert(buffer.end(), info_hash.begin(), info_hash.end());
    buffer.insert(buffer.end(), peer_id.begin(), peer_id.end());

    return buffer;
}

Handshake Handshake::deserialize(std::span<const uint8_t> data) {
    if (data.size() < HANDSHAKE_SIZE) {
        throw std::runtime_error("Buffer too small for Handshake");
    }

    uint8_t pstrlen = data[0];
    if (pstrlen != 19) {
        throw std::runtime_error("Invalid protocol string length in handshake");
    }

    Handshake hs;
    hs.pstr = std::string(reinterpret_cast<const char*>(data.data() + 1), pstrlen);
    if (hs.pstr != PROTOCOL_STRING) {
        throw std::runtime_error("Invalid protocol string: " + hs.pstr);
    }

    hs.reserved = std::string(reinterpret_cast<const char*>(data.data() + 20), 8);
    hs.info_hash = std::string(reinterpret_cast<const char*>(data.data() + 28), 20);
    hs.peer_id = std::string(reinterpret_cast<const char*>(data.data() + 48), 20);

    return hs;
}

std::vector<uint8_t> PeerMessage::serialize() const {
    std::vector<uint8_t> buffer;

    if (type == MessageType::KEEP_ALIVE) {
        buffer.resize(4, 0); // 4 zero bytes
        return buffer;
    }

    uint32_t payload_len = static_cast<uint32_t>(1 + payload.size());
    buffer.resize(4 + payload_len);

    utils::Bytes::writeBigEndian32(payload_len, buffer.data());
    buffer[4] = static_cast<uint8_t>(type);

    if (!payload.empty()) {
        std::copy(payload.begin(), payload.end(), buffer.begin() + 5);
    }

    return buffer;
}

PeerMessage PeerMessage::createKeepAlive() {
    PeerMessage msg;
    msg.type = MessageType::KEEP_ALIVE;
    return msg;
}

PeerMessage PeerMessage::createChoke() {
    PeerMessage msg;
    msg.type = MessageType::CHOKE;
    return msg;
}

PeerMessage PeerMessage::createUnchoke() {
    PeerMessage msg;
    msg.type = MessageType::UNCHOKE;
    return msg;
}

PeerMessage PeerMessage::createInterested() {
    PeerMessage msg;
    msg.type = MessageType::INTERESTED;
    return msg;
}

PeerMessage PeerMessage::createNotInterested() {
    PeerMessage msg;
    msg.type = MessageType::NOT_INTERESTED;
    return msg;
}

PeerMessage PeerMessage::createHave(uint32_t piece_index) {
    PeerMessage msg;
    msg.type = MessageType::HAVE;
    msg.payload.resize(4);
    utils::Bytes::writeBigEndian32(piece_index, msg.payload.data());
    return msg;
}

PeerMessage PeerMessage::createBitfield(const std::vector<bool>& bitfield) {
    PeerMessage msg;
    msg.type = MessageType::BITFIELD;

    size_t byte_count = (bitfield.size() + 7) / 8;
    msg.payload.assign(byte_count, 0);

    for (size_t i = 0; i < bitfield.size(); ++i) {
        if (bitfield[i]) {
            msg.payload[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    return msg;
}

PeerMessage PeerMessage::createRequest(uint32_t piece_index, uint32_t offset, uint32_t length) {
    PeerMessage msg;
    msg.type = MessageType::REQUEST;
    msg.payload.resize(12);
    utils::Bytes::writeBigEndian32(piece_index, msg.payload.data());
    utils::Bytes::writeBigEndian32(offset, msg.payload.data() + 4);
    utils::Bytes::writeBigEndian32(length, msg.payload.data() + 8);
    return msg;
}

PeerMessage PeerMessage::createPiece(uint32_t piece_index, uint32_t offset, const std::vector<uint8_t>& data) {
    PeerMessage msg;
    msg.type = MessageType::PIECE;
    msg.payload.resize(8 + data.size());
    utils::Bytes::writeBigEndian32(piece_index, msg.payload.data());
    utils::Bytes::writeBigEndian32(offset, msg.payload.data() + 4);
    std::copy(data.begin(), data.end(), msg.payload.begin() + 8);
    return msg;
}

PeerMessage PeerMessage::createCancel(uint32_t piece_index, uint32_t offset, uint32_t length) {
    PeerMessage msg;
    msg.type = MessageType::CANCEL;
    msg.payload.resize(12);
    utils::Bytes::writeBigEndian32(piece_index, msg.payload.data());
    utils::Bytes::writeBigEndian32(offset, msg.payload.data() + 4);
    utils::Bytes::writeBigEndian32(length, msg.payload.data() + 8);
    return msg;
}

} // namespace bittorrent::peer
