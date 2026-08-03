#include "bittorrent/crypto/sha1.hpp"
#include <iostream>
#include <cassert>

void testSHA1StandardVector() {
    std::string input = "abc";
    std::string hex_hash = bittorrent::crypto::SHA1::hashHex(input);
    assert(hex_hash == "a9993e364706816aba3e25717850c26c9cd0d89d");
}

void testSHA1EmptyString() {
    std::string input = "";
    std::string hex_hash = bittorrent::crypto::SHA1::hashHex(input);
    assert(hex_hash == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

void runSHA1Tests() {
    std::cout << "[TEST] Running SHA-1 Hash tests..." << std::endl;
    testSHA1StandardVector();
    testSHA1EmptyString();
    std::cout << "[PASS] All SHA-1 tests passed." << std::endl;
}
