#include "bittorrent/piece/piece_manager.hpp"
#include "bittorrent/piece/storage_manager.hpp"
#include "bittorrent/crypto/sha1.hpp"
#include <iostream>
#include <filesystem>
#include <cassert>

void testPieceManagerAssembly() {
    std::filesystem::path temp_dir = "./temp_test_dir";
    std::filesystem::create_directories(temp_dir);

    std::string data = "Hello BitTorrent Educational World!";
    std::string piece_hash = bittorrent::crypto::SHA1::hash(data);

    bittorrent::torrent::TorrentMetadata meta;
    meta.name = "test_piece.txt";
    meta.piece_length = data.size();
    meta.total_length = data.size();
    meta.piece_hashes.push_back(piece_hash);

    bittorrent::torrent::TorrentFile file_item;
    file_item.path = "test_piece.txt";
    file_item.length = data.size();
    meta.files.push_back(file_item);

    auto storage = std::make_shared<bittorrent::piece::StorageManager>(temp_dir, meta);
    storage->prepareFiles();

    bittorrent::piece::PieceManager manager(meta, storage);

    assert(manager.getCompletedPieceCount() == 0);

    std::vector<uint8_t> block_bytes(data.begin(), data.end());
    bool verified = manager.saveBlock(0, 0, block_bytes);

    assert(verified == true);
    assert(manager.getCompletedPieceCount() == 1);
    assert(manager.isComplete() == true);

    std::filesystem::remove_all(temp_dir);
}

void runPieceManagerTests() {
    std::cout << "[TEST] Running PieceManager Assembly & SHA-1 verification tests..." << std::endl;
    testPieceManagerAssembly();
    std::cout << "[PASS] All PieceManager tests passed." << std::endl;
}
