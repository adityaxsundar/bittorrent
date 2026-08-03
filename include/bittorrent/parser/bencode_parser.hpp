#ifndef BITTORRENT_PARSER_BENCODE_PARSER_HPP
#define BITTORRENT_PARSER_BENCODE_PARSER_HPP

#include "bittorrent/parser/bencode_value.hpp"
#include <string>
#include <span>
#include <cstddef>

namespace bittorrent::parser {

/**
 * @brief Recursive descent Bencoding Parser.
 *
 * Specification Rules:
 * 1. Integers: `i<integer>e` (e.g. `i42e`, `i-3e`). No leading zeros (e.g. `i03e` is invalid). `i-0e` is invalid.
 * 2. Strings: `<length>:<contents>` (e.g. `4:spam`). Length must be non-negative integer.
 * 3. Lists: `l<item1><item2>...e` (e.g. `l4:spami42ee`).
 * 4. Dictionaries: `d<key1><value1>...e` (e.g. `d3:cow3:moo4:spam4:eggse`). Keys MUST be bencoded strings sorted lexicographically.
 */
class BencodeParser {
public:
    /**
     * @brief Parse a full bencoded string or byte stream.
     * @param data Binary stream data.
     * @return Root BencodeValue object.
     */
    static BencodeValue decode(std::span<const uint8_t> data);

    /**
     * @brief Parse a std::string containing bencoded data.
     * @param data Binary string data.
     * @return Root BencodeValue object.
     */
    static BencodeValue decode(const std::string& data);

private:
    explicit BencodeParser(std::span<const uint8_t> data);

    BencodeValue parseNext();
    BencodeInt parseInt();
    BencodeString parseString();
    BencodeList parseList();
    BencodeDict parseDict();

    std::span<const uint8_t> data_;
    size_t offset_{0};
};

} // namespace bittorrent::parser

#endif // BITTORRENT_PARSER_BENCODE_PARSER_HPP
