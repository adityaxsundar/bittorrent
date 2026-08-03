#ifndef BITTORRENT_PARSER_BENCODE_VALUE_HPP
#define BITTORRENT_PARSER_BENCODE_VALUE_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <cstdint>
#include <stdexcept>
#include <optional>

namespace bittorrent::parser {

class BencodeValue;

using BencodeInt = int64_t;
using BencodeString = std::string;
using BencodeList = std::vector<BencodeValue>;
using BencodeDict = std::map<std::string, BencodeValue>;

/**
 * @brief Representation of a Bencoded value using std::variant.
 *
 * WHY: Bencoding supports four primitive types: integers, byte strings, lists, and dictionaries.
 * std::variant provides type-safe polymorphism without allocating polymorphic objects on the heap.
 * Dictionaries use std::map to ensure keys remain lexicographically sorted as required by the Bencode spec.
 */
class BencodeValue {
public:
    using VariantType = std::variant<BencodeInt, BencodeString, BencodeList, BencodeDict>;

    BencodeValue() : data_(BencodeString("")) {}
    BencodeValue(BencodeInt val) : data_(val) {}
    BencodeValue(int val) : data_(static_cast<BencodeInt>(val)) {}
    BencodeValue(BencodeString val) : data_(val) {}
    BencodeValue(const char* val) : data_(std::string(val)) {}
    BencodeValue(BencodeList val) : data_(val) {}
    BencodeValue(BencodeDict val) : data_(val) {}

    bool isInt() const { return std::holds_alternative<BencodeInt>(data_); }
    bool isString() const { return std::holds_alternative<BencodeString>(data_); }
    bool isList() const { return std::holds_alternative<BencodeList>(data_); }
    bool isDict() const { return std::holds_alternative<BencodeDict>(data_); }

    BencodeInt asInt() const {
        if (!isInt()) throw std::runtime_error("BencodeValue is not an integer");
        return std::get<BencodeInt>(data_);
    }

    const BencodeString& asString() const {
        if (!isString()) throw std::runtime_error("BencodeValue is not a string");
        return std::get<BencodeString>(data_);
    }

    const BencodeList& asList() const {
        if (!isList()) throw std::runtime_error("BencodeValue is not a list");
        return std::get<BencodeList>(data_);
    }

    const BencodeDict& asDict() const {
        if (!isDict()) throw std::runtime_error("BencodeValue is not a dictionary");
        return std::get<BencodeDict>(data_);
    }

    BencodeDict& asDict() {
        if (!isDict()) throw std::runtime_error("BencodeValue is not a dictionary");
        return std::get<BencodeDict>(data_);
    }

    /**
     * @brief Lookup key in dictionary, returning nullopt if key does not exist.
     * @param key Key name.
     * @return Optional reference to BencodeValue.
     */
    std::optional<BencodeValue> get(const std::string& key) const {
        if (!isDict()) return std::nullopt;
        const auto& dict = asDict();
        auto it = dict.find(key);
        if (it != dict.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    const VariantType& data() const { return data_; }

private:
    VariantType data_;
};

} // namespace bittorrent::parser

#endif // BITTORRENT_PARSER_BENCODE_VALUE_HPP
