#include "bittorrent/parser/bencode_encoder.hpp"
#include <sstream>

namespace bittorrent::parser {

std::string BencodeEncoder::encode(const BencodeValue& val) {
    std::string result;
    encodeImpl(val, result);
    return result;
}

void BencodeEncoder::encodeImpl(const BencodeValue& val, std::string& out) {
    if (val.isInt()) {
        out += 'i';
        out += std::to_string(val.asInt());
        out += 'e';
    } else if (val.isString()) {
        const auto& str = val.asString();
        out += std::to_string(str.size());
        out += ':';
        out += str;
    } else if (val.isList()) {
        out += 'l';
        for (const auto& elem : val.asList()) {
            encodeImpl(elem, out);
        }
        out += 'e';
    } else if (val.isDict()) {
        out += 'd';
        // std::map keys are automatically sorted lexicographically
        for (const auto& [key, value] : val.asDict()) {
            out += std::to_string(key.size());
            out += ':';
            out += key;
            encodeImpl(value, out);
        }
        out += 'e';
    }
}

} // namespace bittorrent::parser
