cmake_minimum_required(VERSION 3.20)
project(bittorrent_client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Required Dependencies
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Include Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# Core Library Sources
set(CORE_SOURCES
    src/utils/logger.cpp
    src/crypto/sha1.cpp
    src/parser/bencode_parser.cpp
    src/parser/bencode_encoder.cpp
    src/torrent/torrent_parser.cpp
    src/torrent/resume_manager.cpp
    src/torrent/download_session.cpp
    src/piece/storage_manager.cpp
    src/piece/piece_manager.cpp
    src/piece/rarest_first.cpp
    src/tracker/http_tracker.cpp
    src/peer/peer_message.cpp
    src/peer/peer_connection.cpp
    src/network/io_context_pool.cpp
    src/cli/cli_app.cpp
)

# Main BitTorrent Executable
add_executable(bittorrent_client src/main.cpp ${CORE_SOURCES})
target_link_libraries(bittorrent_client PRIVATE
    OpenSSL::Crypto
    OpenSSL::SSL
    Boost::system
    Threads::Threads
    ws2_32
    mswsock
)

# Unit Tests Executable
enable_testing()
add_executable(bittorrent_tests
    tests/main_test.cpp
    tests/test_bencode.cpp
    tests/test_sha1.cpp
    tests/test_torrent.cpp
    tests/test_piece_manager.cpp
    tests/test_wire_protocol.cpp
    ${CORE_SOURCES}
)

target_link_libraries(bittorrent_tests PRIVATE
    OpenSSL::Crypto
    OpenSSL::SSL
    Boost::system
    Threads::Threads
    ws2_32
    mswsock
)

add_test(NAME AllUnitTests COMMAND bittorrent_tests)
