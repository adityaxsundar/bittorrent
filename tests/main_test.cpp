#include <iostream>

void runBencodeTests();
void runSHA1Tests();
void runTorrentTests();
void runPieceManagerTests();
void runWireProtocolTests();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " Starting BitTorrent Client Unit Tests " << std::endl;
    std::cout << "========================================" << std::endl;

    runBencodeTests();
    runSHA1Tests();
    runTorrentTests();
    runPieceManagerTests();
    runWireProtocolTests();

    std::cout << "========================================" << std::endl;
    std::cout << " ALL UNIT TESTS COMPLETED SUCCESSFULLY! " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
