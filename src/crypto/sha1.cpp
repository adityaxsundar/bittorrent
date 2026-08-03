#include "bittorrent/crypto/sha1.hpp"
#include "bittorrent/utils/bytes.hpp"
#include <openssl/evp.h>
#include <stdexcept>
#include <array>

namespace bittorrent::crypto {

std::string SHA1::hash(std::span<const uint8_t> data) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    if (!data.empty()) {
        if (EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestUpdate failed");
        }
    }

    std::array<unsigned char, HASH_SIZE> md;
    unsigned int md_len = 0;
    if (EVP_DigestFinal_ex(ctx, md.data(), &md_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);
    return std::string(reinterpret_cast<const char*>(md.data()), md_len);
}

std::string SHA1::hash(const std::string& data) {
    return hash(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

std::string SHA1::hashHex(std::span<const uint8_t> data) {
    std::string raw_hash = hash(data);
    return utils::Bytes::toHex(raw_hash);
}

std::string SHA1::hashHex(const std::string& data) {
    std::string raw_hash = hash(data);
    return utils::Bytes::toHex(raw_hash);
}

} // namespace bittorrent::crypto
