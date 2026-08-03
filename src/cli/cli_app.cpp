#include "bittorrent/cli/cli_app.hpp"
#include "bittorrent/torrent/download_session.hpp"
#include "bittorrent/torrent/torrent_parser.hpp"
#include "bittorrent/utils/logger.hpp"
#include "bittorrent/utils/progress_bar.hpp"
#include <iostream>
#include <filesystem>

namespace bittorrent::cli {

int CLIApp::run(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        if (command == "download") {
            return handleDownload(args);
        } else if (command == "verify") {
            return handleVerify(args);
        } else if (command == "status") {
            return handleStatus(args);
        } else if (command == "pause" || command == "resume") {
            std::cout << "[INFO] Command '" << command << "' applies to interactive session control.\n";
            return 0;
        } else {
            std::cerr << "Unknown command: " << command << "\n";
            printUsage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception caught: " << e.what() << "\n";
        return 1;
    }
}

void CLIApp::printUsage() {
    std::cout << "BitTorrent Client CLI - Modern C++20 Educational BitTorrent Client\n\n"
              << "Usage:\n"
              << "  bittorrent_client <command> [options]\n\n"
              << "Commands:\n"
              << "  download <torrent_file> [-o output_dir]   Download torrent file\n"
              << "  verify   <torrent_file> [-o output_dir]   Verify integrity of downloaded files\n"
              << "  status   <torrent_file>                   Display torrent metadata status\n"
              << "  pause / resume                             Session state indicators\n\n"
              << "Examples:\n"
              << "  bittorrent_client download sample.torrent -o ./downloads\n"
              << "  bittorrent_client verify sample.torrent -o ./downloads\n"
              << "  bittorrent_client status sample.torrent\n";
}

int CLIApp::handleDownload(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: 'download' command requires a .torrent file path.\n";
        return 1;
    }

    std::filesystem::path torrent_path = args[0];
    std::filesystem::path output_dir = "./downloads";

    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-o" && i + 1 < args.size()) {
            output_dir = args[i + 1];
            break;
        }
    }

    torrent::DownloadSession session(torrent_path, output_dir);
    session.start();
    return 0;
}

int CLIApp::handleVerify(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: 'verify' command requires a .torrent file path.\n";
        return 1;
    }

    std::filesystem::path torrent_path = args[0];
    std::filesystem::path output_dir = "./downloads";

    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-o" && i + 1 < args.size()) {
            output_dir = args[i + 1];
            break;
        }
    }

    torrent::DownloadSession session(torrent_path, output_dir);
    bool valid = session.verifyHashes();
    return valid ? 0 : 1;
}

int CLIApp::handleStatus(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: 'status' command requires a .torrent file path.\n";
        return 1;
    }

    std::filesystem::path torrent_path = args[0];
    torrent::TorrentMetadata meta = torrent::TorrentParser::parseFile(torrent_path);

    std::cout << "=== Torrent Metadata Status ===\n"
              << "Name:           " << meta.name << "\n"
              << "Info Hash:      " << meta.info_hash_hex << "\n"
              << "Tracker:        " << meta.announce << "\n"
              << "Piece Length:   " << utils::ProgressBar::formatBytes(meta.piece_length) << "\n"
              << "Piece Count:    " << meta.numPieces() << "\n"
              << "Total Size:     " << utils::ProgressBar::formatBytes(meta.total_length) << "\n"
              << "Layout:         " << (meta.is_multi_file ? "Multi-File" : "Single-File") << "\n"
              << "Files:\n";

    for (const auto& file : meta.files) {
        std::cout << "  - " << file.path.string() << " (" << utils::ProgressBar::formatBytes(file.length) << ")\n";
    }

    return 0;
}

} // namespace bittorrent::cli
