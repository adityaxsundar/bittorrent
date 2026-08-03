#include "bittorrent/parser/bencode_parser.hpp"
#include "bittorrent/parser/bencode_encoder.hpp"
#include <iostream>
#include <cassert>

void testBencodeInteger() {
    std::string encoded = "i42e";
    auto val = bittorrent::parser::BencodeParser::decode(encoded);
    assert(val.isInt());
    assert(val.asInt() == 42);
    assert(bittorrent::parser::BencodeEncoder::encode(val) == encoded);

    std::string neg = "i-100e";
    auto val_neg = bittorrent::parser::BencodeParser::decode(neg);
    assert(val_neg.asInt() == -100);
}

void testBencodeString() {
    std::string encoded = "4:spam";
    auto val = bittorrent::parser::BencodeParser::decode(encoded);
    assert(val.isString());
    assert(val.asString() == "spam");
    assert(bittorrent::parser::BencodeEncoder::encode(val) == encoded);
}

void testBencodeList() {
    std::string encoded = "l4:spami42ee";
    auto val = bittorrent::parser::BencodeParser::decode(encoded);
    assert(val.isList());
    const auto& list = val.asList();
    assert(list.size() == 2);
    assert(list[0].asString() == "spam");
    assert(list[1].asInt() == 42);
    assert(bittorrent::parser::BencodeEncoder::encode(val) == encoded);
}

void testBencodeDictionary() {
    std::string encoded = "d3:bar4:spam3:fooi42ee";
    auto val = bittorrent::parser::BencodeParser::decode(encoded);
    assert(val.isDict());
    assert(val.get("bar")->asString() == "spam");
    assert(val.get("foo")->asInt() == 42);
    assert(bittorrent::parser::BencodeEncoder::encode(val) == encoded);
}

void runBencodeTests() {
    std::cout << "[TEST] Running Bencode Parser & Encoder tests..." << std::endl;
    testBencodeInteger();
    testBencodeString();
    testBencodeList();
    testBencodeDictionary();
    std::cout << "[PASS] All Bencode tests passed." << std::endl;
}
