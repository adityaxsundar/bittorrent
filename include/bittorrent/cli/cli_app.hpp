#ifndef BITTORRENT_CLI_CLI_APP_HPP
#define BITTORRENT_CLI_CLI_APP_HPP

#include <string>
#include <vector>

namespace bittorrent::cli {

/**
 * @brief Command-Line Interface Application Manager.
 *
 * WHY: Provides user-facing subcommands ('download', 'pause', 'resume', 'status', 'verify')
 * for interacting with BitTorrent torrent files.
 */
class CLIApp {
public:
    /**
     * @brief Main entry point for command-line parsing.
     * @param argc Argument count.
     * @param argv Argument array.
     * @return Exit code (0 on success).
     */
    int run(int argc, char* argv[]);

private:
    void printUsage();
    int handleDownload(const std::vector<std::string>& args);
    int handleVerify(const std::vector<std::string>& args);
    int handleStatus(const std::vector<std::string>& args);
};

} // namespace bittorrent::cli

#endif // BITTORRENT_CLI_CLI_APP_HPP
