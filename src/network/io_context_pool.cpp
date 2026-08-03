#include "bittorrent/network/io_context_pool.hpp"

namespace bittorrent::network {

IOContextPool::IOContextPool(size_t num_threads)
    : work_guard_(boost::asio::make_work_guard(io_context_)) {
    if (num_threads == 0) num_threads = 1;
    threads_.reserve(num_threads);
}

IOContextPool::~IOContextPool() {
    stop();
}

void IOContextPool::start() {
    size_t num_threads = threads_.capacity();
    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
}

void IOContextPool::stop() {
    work_guard_.reset();
    if (!io_context_.stopped()) {
        io_context_.stop();
    }
    threads_.clear(); // std::jthread automatically joins on destruction
}

} // namespace bittorrent::network
