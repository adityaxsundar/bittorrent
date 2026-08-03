#include "bittorrent/utils/logger.hpp"

namespace bittorrent::utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    if (!filename.empty()) {
        file_stream_.open(filename, std::ios::out | std::ios::app);
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string timestamp = currentTimestamp();
    std::string level_str = levelToString(level);

    std::ostringstream formatted;
    formatted << "[" << timestamp << "] [" << level_str << "] " << message;

    if (level == LogLevel::LOG_ERROR) {
        std::cerr << formatted.str() << std::endl;
    } else {
        std::cout << formatted.str() << std::endl;
    }

    if (file_stream_.is_open()) {
        file_stream_ << formatted.str() << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::LOG_DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::LOG_INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::LOG_WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::LOG_ERROR, message);
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::LOG_DEBUG: return "DEBUG";
        case LogLevel::LOG_INFO:  return "INFO ";
        case LogLevel::LOG_WARN:  return "WARN ";
        case LogLevel::LOG_ERROR: return "ERROR";
        default:                  return "UNKNOWN";
    }
}

std::string Logger::currentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::stringstream ss;
    struct tm tm_buf;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

} // namespace bittorrent::utils
