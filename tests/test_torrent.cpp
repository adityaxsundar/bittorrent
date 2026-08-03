#include "bittorrent/torrent/torrent_parser.hpp"
#include <iostream>
#include <cassert>

void testTorrentParserSynthetic() {
    // Construct synthetic bencoded torrent file string
    // Dict containing: announce, info dict (name, piece length, pieces, length)
    std::string pieces = std::string(20, 'a') + std::string(20, 'b'); // 2 pieces
    std::string raw_torrent = "d"
        "8:announce35:http://tracker.example.com/announce"
        "4:infod"
            "6:lengthi102400e"
            "4:name12:testfile.dat"
            "12:piece lengthi51200e"
            "6:pieces40:" + pieces +
        "e"
    "e";

    auto meta = bittorrent::torrent::TorrentParser::parseContent(raw_torrent);
    assert(meta.announce == "http://tracker.example.com/announce");
    assert(meta.name == "testfile.dat");
    assert(meta.piece_length == 51200);
    assert(meta.total_length == 102400);
    assert(meta.numPieces() == 2);
    assert(!meta.info_hash.empty());
    assert(meta.info_hash_hex.size() == 40);
}

void runTorrentTests() {
    std::cout << "[TEST] Running Torrent Metainfo Parser tests..." << std::endl;
    testTorrentParserSynthetic();
    std::cout << "[PASS] All Torrent Parser tests passed." << std::endl;
}
