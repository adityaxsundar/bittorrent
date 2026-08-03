#ifndef BITTORRENT_TORRENT_RESUME_MANAGER_HPP
#define BITTORRENT_TORRENT_RESUME_MANAGER_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace bittorrent::torrent {

/**
 * @brief Manages fast resume state persistence.
 *
 * WHY: Re-verifying multi-gigabyte torrent files on client startup is CPU-intensive.
 * Storing a fast-resume state file containing piece bitfield maps and byte completion stats
 * allows immediate download resumption across restarts.
 */
class ResumeManager {
public:
    /**
     * @brief Save fast resume state to disk.
     * @param filepath Target resume file path (e.g. state.fastresume).
     * @param info_hash_hex 40-char hex info hash of torrent.
     * @param bitfield Vector of bools indicating piece completion state.
     * @param downloaded Bytes downloaded.
     * @param uploaded Bytes uploaded.
     */
    static void saveResumeState(const std::filesystem::path& filepath,
                                const std::string& info_hash_hex,
                                const std::vector<bool>& bitfield,
                                uint64_t downloaded,
                                uint64_t uploaded);

    /**
     * @brief Load fast resume state from disk.
     * @param filepath Path to fast resume file.
     * @param info_hash_hex Expected info hash hex to verify match.
     * @param out_bitfield Output bitfield array.
     * @param out_downloaded Output downloaded byte count.
     * @param out_uploaded Output uploaded byte count.
     * @return True if state was loaded successfully, false if missing/corrupt.
     */
    static bool loadResumeState(const std::filesystem::path& filepath,
                                const std::string& info_hash_hex,
                                std::vector<bool>& out_bitfield,
                                uint64_t& out_downloaded,
                                uint64_t& out_uploaded);
};

} // namespace bittorrent::torrent

#endif // BITTORRENT_TORRENT_RESUME_MANAGER_HPP
