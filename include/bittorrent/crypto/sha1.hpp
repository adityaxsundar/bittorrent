#ifndef BITTORRENT_CRYPTO_SHA1_HPP
#define BITTORRENT_CRYPTO_SHA1_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <span>

namespace bittorrent::crypto {

/**
 * @brief SHA-1 Cryptographic Hash Utility wrapping OpenSSL's EVP API.
 *
 * WHY: SHA-1 validation is fundamental to BitTorrent integrity verification.
 * Each torrent contains a list of 20-byte SHA-1 piece hashes. When a piece is fully downloaded from peers,
 * its raw bytes must be hashed using SHA-1 and compared to the expected piece hash.
 * OpenSSL 3.x deprecates legacy low-level SHA1_* functions in favor of the EVP interface.
 */
class SHA1 {
public:
    /**
     * @brief Digest size for SHA-1 in bytes (20 bytes / 160 bits).
     */
    static constexpr size_t HASH_SIZE = 20;

    /**
     * @brief Calculate SHA-1 hash of a raw byte buffer.
     * @param data Input byte buffer span.
     * @return 20-byte raw binary hash string.
     */
    static std::string hash(std::span<const uint8_t> data);

    /**
     * @brief Calculate SHA-1 hash of a std::string.
     * @param data Input string.
     * @return 20-byte raw binary hash string.
     */
    static std::string hash(const std::string& data);

    /**
     * @brief Calculate SHA-1 hash of a byte buffer and return as hex string.
     * @param data Input byte buffer span.
     * @return 40-character hex string.
     */
    static std::string hashHex(std::span<const uint8_t> data);

    /**
     * @brief Calculate SHA-1 hash of a string and return as hex string.
     * @param data Input string.
     * @return 40-character hex string.
     */
    static std::string hashHex(const std::string& data);
};

} // namespace bittorrent::crypto

#endif // BITTORRENT_CRYPTO_SHA1_HPP
