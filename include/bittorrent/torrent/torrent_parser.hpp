#ifndef BITTORRENT_TORRENT_TORRENT_PARSER_HPP
#define BITTORRENT_TORRENT_TORRENT_PARSER_HPP

#include "bittorrent/torrent/torrent_info.hpp"
#include <string>
#include <filesystem>

namespace bittorrent::torrent {

/**
 * @brief Torrent File Parser.
 *
 * Reads `.torrent` metainfo files, parses bencoded data, isolates the exact raw `info` dictionary
 * to compute the cryptographic `info_hash`, and builds a validated `TorrentMetadata` struct.
 */
class TorrentParser {
public:
    /**
     * @brief Parse a `.torrent` file from disk.
     * @param filepath Path to file.
     * @return Fully populated TorrentMetadata struct.
     */
    static TorrentMetadata parseFile(const std::filesystem::path& filepath);

    /**
     * @brief Parse raw bencoded string data of a torrent file.
     * @param content Raw binary string content.
     * @return Fully populated TorrentMetadata struct.
     */
    static TorrentMetadata parseContent(const std::string& content);
};

} // namespace bittorrent::torrent

#endif // BITTORRENT_TORRENT_TORRENT_PARSER_HPP
