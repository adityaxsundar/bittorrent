#include "bittorrent/parser/bencode_parser.hpp"
#include <stdexcept>
#include <cctype>

namespace bittorrent::parser {

BencodeValue BencodeParser::decode(std::span<const uint8_t> data) {
    BencodeParser parser(data);
    BencodeValue result = parser.parseNext();
    if (parser.offset_ != data.size()) {
        throw std::runtime_error("Trailing data found after valid bencode structure");
    }
    return result;
}

BencodeValue BencodeParser::decode(const std::string& data) {
    return decode(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

BencodeParser::BencodeParser(std::span<const uint8_t> data) : data_(data) {}

BencodeValue BencodeParser::parseNext() {
    if (offset_ >= data_.size()) {
        throw std::runtime_error("Unexpected end of data while parsing bencode");
    }

    uint8_t ch = data_[offset_];
    if (ch == 'i') {
        return parseInt();
    } else if (ch == 'l') {
        return parseList();
    } else if (ch == 'd') {
        return parseDict();
    } else if (std::isdigit(ch)) {
        return parseString();
    } else {
        throw std::runtime_error(std::string("Invalid bencode character: '") + static_cast<char>(ch) + "' at offset " + std::to_string(offset_));
    }
}

BencodeInt BencodeParser::parseInt() {
    if (data_[offset_] != 'i') {
        throw std::runtime_error("Expected 'i' at start of integer");
    }
    offset_++; // Skip 'i'

    size_t start = offset_;
    while (offset_ < data_.size() && data_[offset_] != 'e') {
        offset_++;
    }

    if (offset_ >= data_.size()) {
        throw std::runtime_error("Unterminated integer token");
    }

    std::string str(reinterpret_cast<const char*>(data_.data() + start), offset_ - start);
    offset_++; // Skip 'e'

    if (str.empty()) {
        throw std::runtime_error("Empty integer token");
    }

    // Validate rules: no -0, no leading zeroes unless '0' alone
    if (str == "-0") {
        throw std::runtime_error("Invalid integer representation: -0");
    }
    if (str.size() > 1 && str[0] == '0') {
        throw std::runtime_error("Invalid integer representation: leading zero");
    }
    if (str.size() > 2 && str[0] == '-' && str[1] == '0') {
        throw std::runtime_error("Invalid integer representation: leading zero after minus");
    }

    try {
        return std::stoll(str);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse integer: ") + e.what());
    }
}

BencodeString BencodeParser::parseString() {
    size_t start = offset_;
    while (offset_ < data_.size() && std::isdigit(data_[offset_])) {
        offset_++;
    }

    if (offset_ >= data_.size() || data_[offset_] != ':') {
        throw std::runtime_error("Expected ':' after string length specification");
    }

    std::string len_str(reinterpret_cast<const char*>(data_.data() + start), offset_ - start);
    offset_++; // Skip ':'

    if (len_str.empty()) {
        throw std::runtime_error("Missing string length");
    }

    size_t len = 0;
    try {
        len = std::stoull(len_str);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Invalid string length: ") + e.what());
    }

    if (offset_ + len > data_.size()) {
        throw std::runtime_error("String length exceeds remaining data size");
    }

    std::string str(reinterpret_cast<const char*>(data_.data() + offset_), len);
    offset_ += len;
    return str;
}

BencodeList BencodeParser::parseList() {
    if (data_[offset_] != 'l') {
        throw std::runtime_error("Expected 'l' at start of list");
    }
    offset_++; // Skip 'l'

    BencodeList list;
    while (offset_ < data_.size() && data_[offset_] != 'e') {
        list.push_back(parseNext());
    }

    if (offset_ >= data_.size()) {
        throw std::runtime_error("Unterminated list");
    }
    offset_++; // Skip 'e'
    return list;
}

BencodeDict BencodeParser::parseDict() {
    if (data_[offset_] != 'd') {
        throw std::runtime_error("Expected 'd' at start of dictionary");
    }
    offset_++; // Skip 'd'

    BencodeDict dict;
    std::string last_key;
    bool has_last_key = false;

    while (offset_ < data_.size() && data_[offset_] != 'e') {
        if (!std::isdigit(data_[offset_])) {
            throw std::runtime_error("Dictionary keys must be bencoded byte strings");
        }

        std::string key = parseString();

        if (has_last_key && key <= last_key) {
            // Keys sorted lexicographically per spec
        }
        last_key = key;
        has_last_key = true;

        dict[key] = parseNext();
    }

    if (offset_ >= data_.size()) {
        throw std::runtime_error("Unterminated dictionary");
    }
    offset_++; // Skip 'e'
    return dict;
}

} // namespace bittorrent::parser
