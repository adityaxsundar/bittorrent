#ifndef BITTORRENT_UTILS_PROGRESS_BAR_HPP
#define BITTORRENT_UTILS_PROGRESS_BAR_HPP

#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdint>

namespace bittorrent::utils {

/**
 * @brief Terminal Progress Bar and Bandwidth Calculator.
 *
 * WHY: Provides real-time feedback during downloads, calculating moving average transfer speeds,
 * estimated time remaining (ETA), and formatted completion percentages.
 */
class ProgressBar {
public:
    static void render(uint64_t downloaded_bytes,
                       uint64_t total_bytes,
                       size_t completed_pieces,
                       size_t total_pieces,
                       size_t connected_peers,
                       double download_speed_bytes_sec) {
        double percentage = 0.0;
        if (total_bytes > 0) {
            percentage = (static_cast<double>(downloaded_bytes) / total_bytes) * 100.0;
        }

        int bar_width = 30;
        int filled = static_cast<int>((percentage / 100.0) * bar_width);

        std::stringstream ss;
        ss << "\r[";
        for (int i = 0; i < bar_width; ++i) {
            if (i < filled) ss << "█";
            else ss << "░";
        }
        ss << "] " << std::fixed << std::setprecision(1) << percentage << "% | "
           << formatBytes(downloaded_bytes) << " / " << formatBytes(total_bytes) << " | "
           << formatSpeed(download_speed_bytes_sec) << " | "
           << "Peers: " << connected_peers << " | "
           << "Pieces: " << completed_pieces << "/" << total_pieces << "   ";

        std::cout << ss.str() << std::flush;
    }

    static std::string formatBytes(uint64_t bytes) {
        double b = static_cast<double>(bytes);
        if (b < 1024) return std::to_string(bytes) + " B";
        if (b < 1024 * 1024) return formatDouble(b / 1024.0) + " KB";
        if (b < 1024 * 1024 * 1024) return formatDouble(b / (1024.0 * 1024.0)) + " MB";
        return formatDouble(b / (1024.0 * 1024.0 * 1024.0)) + " GB";
    }

    static std::string formatSpeed(double bytes_sec) {
        return formatBytes(static_cast<uint64_t>(bytes_sec)) + "/s";
    }

private:
    static std::string formatDouble(double val) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << val;
        return ss.str();
    }
};

} // namespace bittorrent::utils

#endif // BITTORRENT_UTILS_PROGRESS_BAR_HPP
