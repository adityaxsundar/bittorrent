#ifndef BITTORRENT_UTILS_LOGGER_HPP
#define BITTORRENT_UTILS_LOGGER_HPP

#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Prevent Windows.h macro interference
#ifdef ERROR
#undef ERROR
#endif

namespace bittorrent::utils {

/**
 * @brief Log levels indicating message severity.
 */
enum class LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

/**
 * @brief Thread-safe logging utility for the BitTorrent client.
 *
 * WHY: Async networking with Boost.Asio uses multiple worker threads concurrently.
 * Concurrent std::cout calls without synchronization lead to corrupted, interleaved text output.
 * Logger provides a mutex-guarded logging pipeline supporting console output and file sinking.
 */
class Logger {
public:
    static Logger& getInstance();

    void setLevel(LogLevel level);
    void setLogFile(const std::string& filename);

    void log(LogLevel level, const std::string& message);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string levelToString(LogLevel level) const;
    std::string currentTimestamp() const;

    LogLevel min_level_{LogLevel::LOG_INFO};
    std::mutex mutex_;
    std::ofstream file_stream_;
};

} // namespace bittorrent::utils

#endif // BITTORRENT_UTILS_LOGGER_HPP
