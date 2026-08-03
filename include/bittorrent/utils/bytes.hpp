#ifndef BITTORRENT_UTILS_BYTES_HPP
#define BITTORRENT_UTILS_BYTES_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <span>

namespace bittorrent::utils {

/**
 * @brief Utilities for byte array formatting, hex representation, big-endian conversions, and URL encoding.
 *
 * WHY: BitTorrent wire protocols and HTTP tracker communications transmit binary data
 * in network byte order (big-endian) and URL-encoded raw 20-byte SHA-1 hashes.
 * Centralizing byte utilities avoids repetitive bitwise manipulation and prevents endianness bugs.
 */
class Bytes {
public:
    /**
     * @brief Convert a raw byte buffer to a hex string representation.
     * @param data Span of byte data.
     * @return Hexadecimal string.
     */
    static std::string toHex(std::span<const uint8_t> data) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : data) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    /**
     * @brief Convert a raw string of bytes to a hex string.
     * @param str Input string.
     * @return Hex string.
     */
    static std::string toHex(const std::string& str) {
        return toHex(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
    }

    /**
     * @brief URL-encode raw binary bytes for HTTP tracker GET requests.
     * @param bytes Raw string of bytes (e.g. 20-byte SHA-1 info_hash).
     * @return URL encoded string where non-alphanumeric chars become %XX.
     */
    static std::string urlEncode(const std::string& bytes) {
        std::stringstream ss;
        for (unsigned char c : bytes) {
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '.' || c == '-' || c == '_' || c == '~') {
                ss << c;
            } else {
                ss << '%' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
            }
        }
        return ss.str();
    }

    /**
     * @brief Read a 32-bit big-endian unsigned integer from byte buffer.
     * @param data Pointer to at least 4 bytes of big-endian data.
     * @return Host byte order uint32_t.
     */
    static uint32_t readBigEndian32(const uint8_t* data) {
        return (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8)  |
               (static_cast<uint32_t>(data[3]));
    }

    /**
     * @brief Write a 32-bit unsigned integer in big-endian network byte order.
     * @param value Host byte order integer.
     * @param buffer Output buffer of at least 4 bytes.
     */
    static void writeBigEndian32(uint32_t value, uint8_t* buffer) {
        buffer[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
        buffer[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        buffer[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        buffer[3] = static_cast<uint8_t>(value & 0xFF);
    }

    /**
     * @brief Read a 16-bit big-endian unsigned integer from byte buffer.
     * @param data Pointer to at least 2 bytes.
     * @return Host byte order uint16_t.
     */
    static uint16_t readBigEndian16(const uint8_t* data) {
        return (static_cast<uint16_t>(data[0]) << 8) |
               (static_cast<uint16_t>(data[1]));
    }

    /**
     * @brief Write a 16-bit unsigned integer in big-endian network byte order.
     * @param value Host byte order integer.
     * @param buffer Output buffer of at least 2 bytes.
     */
    static void writeBigEndian16(uint16_t value, uint8_t* buffer) {
        buffer[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
        buffer[1] = static_cast<uint8_t>(value & 0xFF);
    }
};

} // namespace bittorrent::utils

#endif // BITTORRENT_UTILS_BYTES_HPP
