#ifndef BITTORRENT_NETWORK_IO_CONTEXT_POOL_HPP
#define BITTORRENT_NETWORK_IO_CONTEXT_POOL_HPP

#include <boost/asio.hpp>
#include <vector>
#include <thread>
#include <memory>

namespace bittorrent::network {

/**
 * @brief Thread Pool wrapper around Boost.Asio io_context event loop.
 *
 * WHY: High-throughput BitTorrent network downloads require parallel processing of socket I/O events.
 * Running io_context across a configurable pool of worker threads ensures async handlers execute concurrently.
 */
class IOContextPool {
public:
    /**
     * @brief Constructor.
     * @param num_threads Number of worker threads (default hardware concurrency).
     */
    explicit IOContextPool(size_t num_threads = std::thread::hardware_concurrency());
    ~IOContextPool();

    /**
     * @brief Start executing io_context event loop across worker threads.
     */
    void start();

    /**
     * @brief Stop io_context and join worker threads.
     */
    void stop();

    /**
     * @brief Access the underlying Boost.Asio io_context.
     */
    boost::asio::io_context& getIOContext() { return io_context_; }

private:
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::vector<std::jthread> threads_;
};

} // namespace bittorrent::network

#endif // BITTORRENT_NETWORK_IO_CONTEXT_POOL_HPP
