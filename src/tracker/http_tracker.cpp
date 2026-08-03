#include "bittorrent/tracker/http_tracker.hpp"
#include "bittorrent/parser/bencode_parser.hpp"
#include "bittorrent/utils/bytes.hpp"
#include "bittorrent/utils/logger.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace bittorrent::tracker {

HTTPTracker::HTTPTracker(boost::asio::io_context& io_context)
    : io_context_(io_context) {}

TrackerResponse HTTPTracker::announce(const std::string& announce_url,
                                      const std::string& info_hash,
                                      const std::string& peer_id,
                                      uint16_t port,
                                      uint64_t uploaded,
                                      uint64_t downloaded,
                                      uint64_t left,
                                      TrackerEvent event) {
    ParsedURL parsed = parseURL(announce_url);

    // Build HTTP GET query string
    std::stringstream request_path;
    request_path << parsed.path
                 << "?info_hash=" << utils::Bytes::urlEncode(info_hash)
                 << "&peer_id=" << utils::Bytes::urlEncode(peer_id)
                 << "&port=" << port
                 << "&uploaded=" << uploaded
                 << "&downloaded=" << downloaded
                 << "&left=" << left
                 << "&compact=1";

    std::string evt_str = eventToString(event);
    if (!evt_str.empty()) {
        request_path << "&event=" << evt_str;
    }

    // Resolve DNS host
    boost::asio::ip::tcp::resolver resolver(io_context_);
    boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(parsed.host, parsed.port);

    // Connect socket
    boost::asio::ip::tcp::socket socket(io_context_);
    boost::asio::connect(socket, endpoints);

    // Build HTTP request header
    std::stringstream http_req;
    http_req << "GET " << request_path.str() << " HTTP/1.1\r\n"
             << "Host: " << parsed.host << "\r\n"
             << "User-Agent: BitTorrentClient/1.0\r\n"
             << "Accept: */*\r\n"
             << "Connection: close\r\n\r\n";

    boost::asio::write(socket, boost::asio::buffer(http_req.str()));

    // Read HTTP response
    boost::asio::streambuf response_buf;
    boost::system::error_code ec;
    boost::asio::read(socket, response_buf, ec);
    if (ec && ec != boost::asio::error::eof) {
        throw std::runtime_error("Tracker HTTP read failed: " + ec.message());
    }

    std::string raw_response((std::istreambuf_iterator<char>(&response_buf)),
                             std::istreambuf_iterator<char>());

    // Isolate body after \r\n\r\n
    size_t header_end = raw_response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        throw std::runtime_error("Invalid HTTP response from tracker: missing header delimiter");
    }

    std::string response_body = raw_response.substr(header_end + 4);
    return parseTrackerResponseBody(response_body);
}

TrackerResponse HTTPTracker::parseTrackerResponseBody(const std::string& response_body) {
    parser::BencodeValue root = parser::BencodeParser::decode(response_body);
    if (!root.isDict()) {
        throw std::runtime_error("Invalid tracker response: root is not a bencoded dictionary");
    }

    TrackerResponse resp;

    auto failure_opt = root.get("failure reason");
    if (failure_opt && failure_opt->isString()) {
        resp.failure_reason = failure_opt->asString();
        return resp;
    }

    auto interval_opt = root.get("interval");
    if (interval_opt && interval_opt->isInt()) {
        resp.interval = static_cast<uint32_t>(interval_opt->asInt());
    }

    auto complete_opt = root.get("complete");
    if (complete_opt && complete_opt->isInt()) {
        resp.complete = static_cast<uint32_t>(complete_opt->asInt());
    }

    auto incomplete_opt = root.get("incomplete");
    if (incomplete_opt && incomplete_opt->isInt()) {
        resp.incomplete = static_cast<uint32_t>(incomplete_opt->asInt());
    }

    // Parse peers
    auto peers_opt = root.get("peers");
    if (peers_opt) {
        if (peers_opt->isString()) {
            // Compact peers string (6 bytes per peer: 4 bytes IPv4 + 2 bytes port)
            const std::string& peer_str = peers_opt->asString();
            for (size_t i = 0; i + 6 <= peer_str.size(); i += 6) {
                PeerEndpoint peer;
                const uint8_t* p = reinterpret_cast<const uint8_t*>(peer_str.data() + i);

                peer.ip = std::to_string(p[0]) + "." +
                          std::to_string(p[1]) + "." +
                          std::to_string(p[2]) + "." +
                          std::to_string(p[3]);
                peer.port = utils::Bytes::readBigEndian16(p + 4);
                resp.peers.push_back(peer);
            }
        } else if (peers_opt->isList()) {
            // Dictionary peers list
            for (const auto& p_val : peers_opt->asList()) {
                if (p_val.isDict()) {
                    PeerEndpoint peer;
                    auto ip_opt = p_val.get("ip");
                    auto port_opt = p_val.get("port");
                    auto id_opt = p_val.get("peer id");

                    if (ip_opt && ip_opt->isString()) peer.ip = ip_opt->asString();
                    if (port_opt && port_opt->isInt()) peer.port = static_cast<uint16_t>(port_opt->asInt());
                    if (id_opt && id_opt->isString()) peer.peer_id = id_opt->asString();

                    if (!peer.ip.empty() && peer.port != 0) {
                        resp.peers.push_back(peer);
                    }
                }
            }
        }
    }

    return resp;
}

HTTPTracker::ParsedURL HTTPTracker::parseURL(const std::string& url) {
    ParsedURL parsed;
    std::string u = url;

    // Strip http://
    if (u.rfind("http://", 0) == 0) {
        u = u.substr(7);
    }

    size_t path_pos = u.find('/');
    std::string host_port;
    if (path_pos != std::string::npos) {
        host_port = u.substr(0, path_pos);
        parsed.path = u.substr(path_pos);
    } else {
        host_port = u;
        parsed.path = "/";
    }

    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos) {
        parsed.host = host_port.substr(0, colon_pos);
        parsed.port = host_port.substr(colon_pos + 1);
    } else {
        parsed.host = host_port;
        parsed.port = "80";
    }

    return parsed;
}

std::string HTTPTracker::eventToString(TrackerEvent event) const {
    switch (event) {
        case TrackerEvent::STARTED:   return "started";
        case TrackerEvent::STOPPED:   return "stopped";
        case TrackerEvent::COMPLETED: return "completed";
        case TrackerEvent::NONE:      return "";
    }
    return "";
}

} // namespace bittorrent::tracker
