#include "bittorrent/cli/cli_app.hpp"

/**
 * @brief Application Main Entry Point.
 */
int main(int argc, char* argv[]) {
    bittorrent::cli::CLIApp app;
    return app.run(argc, argv);
}
