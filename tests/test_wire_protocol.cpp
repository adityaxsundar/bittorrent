#include "bittorrent/peer/peer_message.hpp"
#include "bittorrent/utils/bytes.hpp"
#include <iostream>
#include <cassert>

void testHandshakeSerialization() {
    bittorrent::peer::Handshake hs;
    hs.info_hash = std::string(20, 'X');
    hs.peer_id = std::string(20, 'Y');

    std::vector<uint8_t> serialized = hs.serialize();
    assert(serialized.size() == 68);

    bittorrent::peer::Handshake res = bittorrent::peer::Handshake::deserialize(serialized);
    assert(res.pstr == "BitTorrent protocol");
    assert(res.info_hash == std::string(20, 'X'));
    assert(res.peer_id == std::string(20, 'Y'));
}

void testWireMessageSerialization() {
    auto interested = bittorrent::peer::PeerMessage::createInterested();
    std::vector<uint8_t> int_bytes = interested.serialize();
    assert(int_bytes.size() == 5);
    assert(int_bytes[4] == static_cast<uint8_t>(bittorrent::peer::MessageType::INTERESTED));

    auto request = bittorrent::peer::PeerMessage::createRequest(10, 16384, 16384);
    std::vector<uint8_t> req_bytes = request.serialize();
    assert(req_bytes.size() == 17); // 4 len + 1 id + 12 payload
    assert(req_bytes[4] == static_cast<uint8_t>(bittorrent::peer::MessageType::REQUEST));

    uint32_t piece_idx = bittorrent::utils::Bytes::readBigEndian32(req_bytes.data() + 5);
    assert(piece_idx == 10);
}

void runWireProtocolTests() {
    std::cout << "[TEST] Running Peer Wire Protocol serialization tests..." << std::endl;
    testHandshakeSerialization();
    testWireMessageSerialization();
    std::cout << "[PASS] All Wire Protocol tests passed." << std::endl;
}
