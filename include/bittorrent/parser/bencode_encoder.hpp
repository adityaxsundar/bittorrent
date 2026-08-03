#ifndef BITTORRENT_PARSER_BENCODE_ENCODER_HPP
#define BITTORRENT_PARSER_BENCODE_ENCODER_HPP

#include "bittorrent/parser/bencode_value.hpp"
#include <string>
#include <vector>

namespace bittorrent::parser {

/**
 * @brief Serializes BencodeValue data structures back into binary bencode byte strings.
 *
 * WHY: Computing the BitTorrent `info_hash` requires re-encoding the raw `info` dictionary
 * extracted from a `.torrent` file or tracker response. Strict adherence to bencode serialization rules
 * ensures byte-for-byte fidelity with standard BitTorrent clients.
 */
class BencodeEncoder {
public:
    /**
     * @brief Encode a BencodeValue into its binary string representation.
     * @param val Input BencodeValue object.
     * @return Binary bencoded std::string.
     */
    static std::string encode(const BencodeValue& val);

private:
    static void encodeImpl(const BencodeValue& val, std::string& out);
};

} // namespace bittorrent::parser

#endif // BITTORRENT_PARSER_BENCODE_ENCODER_HPP
