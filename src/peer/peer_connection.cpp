#include "bittorrent/peer/peer_connection.hpp"
#include "bittorrent/utils/bytes.hpp"
#include "bittorrent/utils/logger.hpp"
#include <iostream>

namespace bittorrent::peer {

PeerConnection::PeerConnection(boost::asio::io_context& io_context,
                               const std::string& ip,
                               uint16_t port,
                               const std::string& info_hash,
                               const std::string& peer_id,
                               std::shared_ptr<piece::PieceManager> piece_manager,
                               std::shared_ptr<piece::StorageManager> storage,
                               std::shared_ptr<piece::RarestFirstSelector> rarest_first)
    : io_context_(io_context),
      socket_(io_context),
      timeout_timer_(io_context),
      ip_(ip),
      port_(port),
      info_hash_(info_hash),
      peer_id_(peer_id),
      piece_manager_(piece_manager),
      storage_(storage),
      rarest_first_(rarest_first) {
    peer_bitfield_.assign(piece_manager_->getTotalPieces(), false);
}

PeerConnection::~PeerConnection() {
    disconnect();
}

void PeerConnection::start() {
    doConnect();
}

void PeerConnection::disconnect() {
    if (is_connected_) {
        is_connected_ = false;
        boost::system::error_code ec;
        socket_.close(ec);
        timeout_timer_.cancel();
        if (rarest_first_) {
            rarest_first_->removePeerBitfield(peer_bitfield_);
        }
        if (current_downloading_piece_ >= 0) {
            piece_manager_->resetPiece(current_downloading_piece_);
            current_downloading_piece_ = -1;
        }
        utils::Logger::getInstance().info("Disconnected peer " + ip_ + ":" + std::to_string(port_));
    }
}

void PeerConnection::doConnect() {
    auto self = shared_from_this();
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(ip_), port_);

    timeout_timer_.expires_after(std::chrono::seconds(10));
    timeout_timer_.async_wait([self](const boost::system::error_code& ec) {
        if (!ec && !self->is_connected_) {
            utils::Logger::getInstance().warn("Connection timeout to peer " + self->ip_);
            self->disconnect();
        }
    });

    socket_.async_connect(endpoint, [self](const boost::system::error_code& ec) {
        self->timeout_timer_.cancel();
        if (!ec) {
            self->is_connected_ = true;
            utils::Logger::getInstance().info("Connected to peer " + self->ip_ + ":" + std::to_string(self->port_));
            self->doHandshake();
        } else {
            utils::Logger::getInstance().warn("Failed to connect to peer " + self->ip_ + ": " + ec.message());
            self->disconnect();
        }
    });
}

void PeerConnection::doHandshake() {
    auto self = shared_from_this();

    Handshake req;
    req.info_hash = info_hash_;
    req.peer_id = peer_id_;
    std::vector<uint8_t> handshake_bytes = req.serialize();

    sendMessage(PeerMessage{MessageType::KEEP_ALIVE, {}}); // placeholder write queue fill
    write_queue_.clear();

    boost::asio::async_write(socket_, boost::asio::buffer(handshake_bytes),
        [self](const boost::system::error_code& ec, size_t /*bytes*/) {
            if (!ec) {
                // Read 68-byte handshake response
                auto response_buf = std::make_shared<std::vector<uint8_t>>(Handshake::HANDSHAKE_SIZE);
                boost::asio::async_read(self->socket_, boost::asio::buffer(*response_buf),
                    [self, response_buf](const boost::system::error_code& ec2, size_t /*bytes2*/) {
                        if (!ec2) {
                            try {
                                Handshake res = Handshake::deserialize(*response_buf);
                                if (res.info_hash != self->info_hash_) {
                                    utils::Logger::getInstance().warn("Peer " + self->ip_ + " info_hash mismatch");
                                    self->disconnect();
                                    return;
                                }
                                self->remote_peer_id_ = res.peer_id;
                                utils::Logger::getInstance().info("Handshake successful with peer " + self->ip_);

                                // Send client Bitfield if available
                                auto bitfield = self->piece_manager_->getBitfield();
                                bool has_any = false;
                                for (bool b : bitfield) { if (b) { has_any = true; break; } }
                                if (has_any) {
                                    self->sendMessage(PeerMessage::createBitfield(bitfield));
                                }

                                // Send INTERESTED message
                                self->am_interested_ = true;
                                self->sendMessage(PeerMessage::createInterested());

                                self->doReadHeader();
                            } catch (const std::exception& e) {
                                utils::Logger::getInstance().warn("Invalid handshake from " + self->ip_ + ": " + e.what());
                                self->disconnect();
                            }
                        } else {
                            self->disconnect();
                        }
                    });
            } else {
                self->disconnect();
            }
        });
}

void PeerConnection::doReadHeader() {
    auto self = shared_from_this();
    read_buffer_.resize(4);

    boost::asio::async_read(socket_, boost::asio::buffer(read_buffer_),
        [self](const boost::system::error_code& ec, size_t /*bytes*/) {
            if (!ec) {
                uint32_t message_length = utils::Bytes::readBigEndian32(self->read_buffer_.data());
                if (message_length == 0) {
                    // Keep-Alive
                    self->handleMessage(MessageType::KEEP_ALIVE, {});
                    self->doReadHeader();
                } else if (message_length > 100000) {
                    utils::Logger::getInstance().warn("Peer " + self->ip_ + " sent invalid large message length: " + std::to_string(message_length));
                    self->disconnect();
                } else {
                    self->doReadBody(message_length);
                }
            } else {
                self->disconnect();
            }
        });
}

void PeerConnection::doReadBody(uint32_t length) {
    auto self = shared_from_this();
    read_buffer_.resize(length);

    boost::asio::async_read(socket_, boost::asio::buffer(read_buffer_),
        [self, length](const boost::system::error_code& ec, size_t /*bytes*/) {
            if (!ec) {
                MessageType type = static_cast<MessageType>(self->read_buffer_[0]);
                std::vector<uint8_t> payload(self->read_buffer_.begin() + 1, self->read_buffer_.end());
                self->handleMessage(type, payload);
                self->doReadHeader();
            } else {
                self->disconnect();
            }
        });
}

void PeerConnection::sendMessage(const PeerMessage& msg) {
    std::vector<uint8_t> bytes = msg.serialize();
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(bytes);
    if (!write_in_progress) {
        doWrite();
    }
}

void PeerConnection::doWrite() {
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(write_queue_.front()),
        [self](const boost::system::error_code& ec, size_t /*bytes*/) {
            if (!ec) {
                self->write_queue_.pop_front();
                if (!self->write_queue_.empty()) {
                    self->doWrite();
                }
            } else {
                self->disconnect();
            }
        });
}

void PeerConnection::handleMessage(MessageType type, const std::vector<uint8_t>& payload) {
    switch (type) {
        case MessageType::KEEP_ALIVE:
            break;
        case MessageType::CHOKE:
            peer_choking_ = true;
            utils::Logger::getInstance().debug("Peer " + ip_ + " CHOKED client");
            break;
        case MessageType::UNCHOKE:
            peer_choking_ = false;
            utils::Logger::getInstance().debug("Peer " + ip_ + " UNCHOKED client");
            requestBlocksPipeline();
            break;
        case MessageType::INTERESTED:
            peer_interested_ = true;
            // Automatically unchoke peer if we can upload
            am_choking_ = false;
            sendMessage(PeerMessage::createUnchoke());
            break;
        case MessageType::NOT_INTERESTED:
            peer_interested_ = false;
            break;
        case MessageType::HAVE:
            if (payload.size() >= 4) {
                uint32_t piece_idx = utils::Bytes::readBigEndian32(payload.data());
                if (piece_idx < peer_bitfield_.size()) {
                    peer_bitfield_[piece_idx] = true;
                    if (rarest_first_) rarest_first_->incrementPieceCount(piece_idx);
                }
            }
            break;
        case MessageType::BITFIELD: {
            size_t bit_idx = 0;
            for (uint8_t byte_val : payload) {
                for (int b = 7; b >= 0; --b) {
                    if (bit_idx < peer_bitfield_.size()) {
                        peer_bitfield_[bit_idx] = (byte_val >> b) & 1;
                        bit_idx++;
                    }
                }
            }
            if (rarest_first_) rarest_first_->addPeerBitfield(peer_bitfield_);
            break;
        }
        case MessageType::REQUEST:
            if (payload.size() >= 12 && !am_choking_) {
                uint32_t index = utils::Bytes::readBigEndian32(payload.data());
                uint32_t offset = utils::Bytes::readBigEndian32(payload.data() + 4);
                uint32_t length = utils::Bytes::readBigEndian32(payload.data() + 8);

                try {
                    std::vector<uint8_t> block_data = storage_->readBlock(index, offset, length);
                    sendMessage(PeerMessage::createPiece(index, offset, block_data));
                } catch (const std::exception& e) {
                    utils::Logger::getInstance().warn("Upload block read failed: " + std::string(e.what()));
                }
            }
            break;
        case MessageType::PIECE:
            if (payload.size() >= 8) {
                uint32_t index = utils::Bytes::readBigEndian32(payload.data());
                uint32_t offset = utils::Bytes::readBigEndian32(payload.data() + 4);
                std::vector<uint8_t> block_data(payload.begin() + 8, payload.end());

                in_flight_requests_--;
                bool piece_completed = piece_manager_->saveBlock(index, offset, block_data);
                if (piece_completed) {
                    current_downloading_piece_ = -1;
                }

                requestBlocksPipeline();
            }
            break;
        case MessageType::CANCEL:
            break;
        default:
            break;
    }
}

void PeerConnection::requestBlocksPipeline() {
    if (peer_choking_ || !is_connected_) return;

    while (in_flight_requests_ < MAX_IN_FLIGHT) {
        if (current_downloading_piece_ < 0) {
            auto client_bitfield = piece_manager_->getBitfield();
            int next_piece = rarest_first_->selectNextPiece(peer_bitfield_, client_bitfield);
            if (next_piece < 0) {
                return; // No piece available to request right now
            }
            current_downloading_piece_ = next_piece;
        }

        auto block_opt = piece_manager_->getNextBlockRequest(static_cast<uint32_t>(current_downloading_piece_));
        if (!block_opt) {
            current_downloading_piece_ = -1;
            continue;
        }

        sendMessage(PeerMessage::createRequest(block_opt->piece_index, block_opt->offset, block_opt->length));
        in_flight_requests_++;
    }
}

} // namespace bittorrent::peer
