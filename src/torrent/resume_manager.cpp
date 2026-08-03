#include "bittorrent/torrent/resume_manager.hpp"
#include "bittorrent/parser/bencode_value.hpp"
#include "bittorrent/parser/bencode_parser.hpp"
#include "bittorrent/parser/bencode_encoder.hpp"
#include <fstream>
#include <sstream>

namespace bittorrent::torrent {

void ResumeManager::saveResumeState(const std::filesystem::path& filepath,
                                     const std::string& info_hash_hex,
                                     const std::vector<bool>& bitfield,
                                     uint64_t downloaded,
                                     uint64_t uploaded) {
    // Encode bitfield as binary byte string
    std::string bit_str;
    bit_str.reserve((bitfield.size() + 7) / 8);

    uint8_t current_byte = 0;
    int bit_count = 0;

    for (bool bit : bitfield) {
        if (bit) {
            current_byte |= (1 << (7 - bit_count));
        }
        bit_count++;
        if (bit_count == 8) {
            bit_str.push_back(static_cast<char>(current_byte));
            current_byte = 0;
            bit_count = 0;
        }
    }
    if (bit_count > 0) {
        bit_str.push_back(static_cast<char>(current_byte));
    }

    parser::BencodeDict dict;
    dict["info_hash"] = info_hash_hex;
    dict["bitfield"] = bit_str;
    dict["num_pieces"] = static_cast<int64_t>(bitfield.size());
    dict["downloaded"] = static_cast<int64_t>(downloaded);
    dict["uploaded"] = static_cast<int64_t>(uploaded);

    std::string encoded = parser::BencodeEncoder::encode(parser::BencodeValue(dict));

    std::ofstream out(filepath, std::ios::binary);
    if (out.is_open()) {
        out.write(encoded.data(), encoded.size());
    }
}

bool ResumeManager::loadResumeState(const std::filesystem::path& filepath,
                                     const std::string& info_hash_hex,
                                     std::vector<bool>& out_bitfield,
                                     uint64_t& out_downloaded,
                                     uint64_t& out_uploaded) {
    if (!std::filesystem::exists(filepath)) {
        return false;
    }

    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) return false;

    std::stringstream buffer;
    buffer << in.rdbuf();

    try {
        parser::BencodeValue root = parser::BencodeParser::decode(buffer.str());
        if (!root.isDict()) return false;

        auto hash_opt = root.get("info_hash");
        if (!hash_opt || hash_opt->asString() != info_hash_hex) {
            return false;
        }

        auto bitfield_opt = root.get("bitfield");
        auto num_pieces_opt = root.get("num_pieces");
        auto downloaded_opt = root.get("downloaded");
        auto uploaded_opt = root.get("uploaded");

        if (!bitfield_opt || !num_pieces_opt || !downloaded_opt || !uploaded_opt) {
            return false;
        }

        const std::string& bit_str = bitfield_opt->asString();
        size_t total_pieces = num_pieces_opt->asInt();

        out_bitfield.assign(total_pieces, false);
        size_t piece_idx = 0;

        for (uint8_t byte_val : bit_str) {
            for (int b = 7; b >= 0; --b) {
                if (piece_idx < total_pieces) {
                    out_bitfield[piece_idx] = (byte_val >> b) & 1;
                    piece_idx++;
                }
            }
        }

        out_downloaded = downloaded_opt->asInt();
        out_uploaded = uploaded_opt->asInt();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace bittorrent::torrent
