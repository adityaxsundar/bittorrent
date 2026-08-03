# Modern C++20 BitTorrent Client

A production-quality, high-performance, educational BitTorrent client implemented in modern **C++20** using **Boost.Asio**, **OpenSSL**, **CMake**, and standard C++ STL components.

---

## Technical Features
- **C++20 Architecture**: Modern concepts, `std::span`, smart pointers, and `std::jthread`.
- **Bencoding Parser & Serializer**: High-performance recursive descent parser supporting integers, byte strings, lists, and lexicographically sorted dictionaries.
- **Torrent Metadata Parser**: Parses single-file and multi-file `.torrent` files, extracting piece hashes, file layouts, and computing raw `info_hash` digests.
- **SHA-1 Cryptographic Integrity**: OpenSSL `EVP` API hashing for piece validation.
- **HTTP Tracker Communication**: Asynchronous announce protocol with compact binary (6-byte IPv4+Port) and dictionary peer discovery.
- **Wire Protocol & State Machine**: Full peer handshake exchange, choke/unchoke/interest state machines, keep-alive timers, and pipelined block requests (5 in-flight blocks).
- **Rarest-First Piece Selection**: Swarm availability tracking prioritizing rarest pieces first.
- **File System Storage Manager**: Global byte offset translation mapping pieces across single or multi-file disk layouts.
- **Fast-Resume Persistence**: `.fastresume` state serialization saving piece bitfields and download progress across restarts.
- **CLI Interface**: Commands (`download`, `status`, `verify`) with real-time terminal progress indicators.
- **Comprehensive Unit Tests**: Integrated CTest test runner validating bencode parsing, SHA-1, torrent metainfo, piece assembly, and wire message framing.

---

## Directory Structure

```text
bittorrent/
├── CMakeLists.txt
├── README.md
├── LEARNING_GUIDE.md
├── include/bittorrent/
│   ├── cli/
│   ├── crypto/
│   ├── network/
│   ├── parser/
│   ├── peer/
│   ├── piece/
│   ├── storage/
│   ├── torrent/
│   ├── tracker/
│   └── utils/
├── src/
│   ├── main.cpp
│   ├── cli/
│   ├── crypto/
│   ├── network/
│   ├── parser/
│   ├── peer/
│   ├── piece/
│   ├── torrent/
│   ├── tracker/
│   └── utils/
├── tests/
├── examples/
└── docs/
```

---

## Prerequisites & Dependencies
- **Compiler**: GCC 11+ or Clang 13+ with C++20 support.
- **Build System**: CMake 3.20+ and Ninja / Make.
- **Libraries**:
  - OpenSSL 3.x (`libcrypto`, `libssl`)
  - Boost 1.75+ (`Boost.Asio`, `Boost.System`)

---

## Build Instructions

```bash
# 1. Clone or navigate to workspace
cd bittorrent

# 2. Configure build directory using CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. Build client and unit test executables
cmake --build build

# 4. Run full unit test suite
./build/bittorrent_tests
# Or via CTest
ctest --test-dir build --output-on-failure
```

---

## Usage Examples

### 1. View Torrent Metadata Status
```bash
./build/bittorrent_client status sample.torrent
```

### 2. Download Torrent File
```bash
./build/bittorrent_client download sample.torrent -o ./downloads
```

### 3. Verify Downloaded Files against SHA-1 Hashes
```bash
./build/bittorrent_client verify sample.torrent -o ./downloads
```

---

## Limitations
- **UDP Tracker Protocol (BEP 15)**: Currently supports HTTP/HTTPS trackers. UDP tracker extension is planned for future releases.
- **DHT (Distributed Hash Table - BEP 5)**: Peer discovery currently relies on tracker announcements and peer exchange; trackerless DHT bootstrapping is not yet enabled.
- **IPv6 Support**: Wire protocol socket handlers are currently optimized for IPv4 addresses.

---

## Future Enhancements
- [ ] Implement UDP Tracker Protocol (BEP 15).
- [ ] Add Distributed Hash Table (DHT / BEP 5) for trackerless swarms.
- [ ] Add Peer Exchange (PEX / BEP 11) and Extension Protocol (BEP 10).
- [ ] Add web UI dashboard using Boost.Beast HTTP/WebSocket.
