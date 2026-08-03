#include "bittorrent/torrent/torrent_parser.hpp"
#include "bittorrent/parser/bencode_parser.hpp"
#include "bittorrent/parser/bencode_encoder.hpp"
#include "bittorrent/crypto/sha1.hpp"
#include "bittorrent/utils/bytes.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace bittorrent::torrent {

TorrentMetadata TorrentParser::parseFile(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open torrent file: " + filepath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseContent(buffer.str());
}

TorrentMetadata TorrentParser::parseContent(const std::string& content) {
    parser::BencodeValue root = parser::BencodeParser::decode(content);
    if (!root.isDict()) {
        throw std::runtime_error("Invalid torrent file: Root is not a bencoded dictionary");
    }

    const auto& dict = root.asDict();
    TorrentMetadata metadata;

    // Extract main tracker URL
    auto announce_opt = root.get("announce");
    if (announce_opt && announce_opt->isString()) {
        metadata.announce = announce_opt->asString();
    }

    // Extract announce-list if present
    auto announce_list_opt = root.get("announce-list");
    if (announce_list_opt && announce_list_opt->isList()) {
        for (const auto& tier : announce_list_opt->asList()) {
            if (tier.isList()) {
                for (const auto& tracker : tier.asList()) {
                    if (tracker.isString()) {
                        metadata.announce_list.push_back(tracker.asString());
                    }
                }
            }
        }
    }

    // Optional metadata fields
    auto comment_opt = root.get("comment");
    if (comment_opt && comment_opt->isString()) metadata.comment = comment_opt->asString();

    auto created_by_opt = root.get("created by");
    if (created_by_opt && created_by_opt->isString()) metadata.created_by = created_by_opt->asString();

    auto creation_date_opt = root.get("creation date");
    if (creation_date_opt && creation_date_opt->isInt()) metadata.creation_date = creation_date_opt->asInt();

    // Required "info" dictionary
    auto info_it = dict.find("info");
    if (info_it == dict.end() || !info_it->second.isDict()) {
        throw std::runtime_error("Invalid torrent file: Missing 'info' dictionary");
    }

    const auto& info_dict_val = info_it->second;
    const auto& info = info_dict_val.asDict();

    // Calculate info_hash using BencodeEncoder
    std::string bencoded_info = parser::BencodeEncoder::encode(info_dict_val);
    metadata.info_hash = crypto::SHA1::hash(bencoded_info);
    metadata.info_hash_hex = crypto::SHA1::hashHex(bencoded_info);

    // Extract piece length
    auto piece_length_it = info.find("piece length");
    if (piece_length_it == info.end() || !piece_length_it->second.isInt()) {
        throw std::runtime_error("Invalid torrent file: Missing 'piece length'");
    }
    metadata.piece_length = piece_length_it->second.asInt();

    // Extract raw pieces string (concatenated 20-byte SHA-1 hashes)
    auto pieces_it = info.find("pieces");
    if (pieces_it == info.end() || !pieces_it->second.isString()) {
        throw std::runtime_error("Invalid torrent file: Missing 'pieces'");
    }
    const std::string& pieces_raw = pieces_it->second.asString();
    if (pieces_raw.size() % 20 != 0) {
        throw std::runtime_error("Invalid torrent file: 'pieces' string length is not a multiple of 20");
    }

    for (size_t i = 0; i < pieces_raw.size(); i += 20) {
        metadata.piece_hashes.push_back(pieces_raw.substr(i, 20));
    }

    // Extract name
    auto name_it = info.find("name");
    if (name_it != info.end() && name_it->second.isString()) {
        metadata.name = name_it->second.asString();
    } else {
        metadata.name = "unnamed_torrent";
    }

    // Single-file vs Multi-file handling
    auto files_it = info.find("files");
    if (files_it != info.end() && files_it->second.isList()) {
        // Multi-file torrent
        metadata.is_multi_file = true;
        uint64_t accumulated_length = 0;

        for (const auto& file_val : files_it->second.asList()) {
            if (!file_val.isDict()) continue;
            const auto& file_dict = file_val.asDict();

            auto length_it = file_dict.find("length");
            auto path_it = file_dict.find("path");

            if (length_it == file_dict.end() || !length_it->second.isInt() ||
                path_it == file_dict.end() || !path_it->second.isList()) {
                continue;
            }

            TorrentFile file_item;
            file_item.length = length_it->second.asInt();

            std::filesystem::path rel_path = metadata.name;
            for (const auto& p_element : path_it->second.asList()) {
                if (p_element.isString()) {
                    rel_path /= p_element.asString();
                }
            }
            file_item.path = rel_path;

            metadata.files.push_back(file_item);
            accumulated_length += file_item.length;
        }

        metadata.total_length = accumulated_length;
    } else {
        // Single-file torrent
        metadata.is_multi_file = false;
        auto length_it = info.find("length");
        if (length_it == info.end() || !length_it->second.isInt()) {
            throw std::runtime_error("Invalid single-file torrent: Missing 'length'");
        }

        TorrentFile single_file;
        single_file.path = metadata.name;
        single_file.length = length_it->second.asInt();

        metadata.files.push_back(single_file);
        metadata.total_length = single_file.length;
    }

    return metadata;
}

} // namespace bittorrent::torrent
