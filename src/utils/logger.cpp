#include "logger.h"
#include <chrono>
#include <iomanip>
#include <ctime>
#include <unistd.h>

namespace tdesktop {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

void Logger::init(const std::string& path) {
    file_.open(path, std::ios::out | std::ios::app);
    if (file_.is_open()) {
        info("Logger initialized to " + path);
    }
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::setDebug(bool debug) {
    debug_ = debug;
    if (debug) level_ = LogLevel::Debug;
}

void Logger::debug(const std::string& msg) {
    if (level_ <= LogLevel::Debug) log(LogLevel::Debug, msg);
}

void Logger::info(const std::string& msg) {
    if (level_ <= LogLevel::Info) log(LogLevel::Info, msg);
}

void Logger::warn(const std::string& msg) {
    if (level_ <= LogLevel::Warn) log(LogLevel::Warn, msg);
}

void Logger::error(const std::string& msg) {
    if (level_ <= LogLevel::Error) log(LogLevel::Error, msg);
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string line = "[" + timestamp() + "] [" + levelStr(level) + "] " + msg + "\n";
    if (file_.is_open()) {
        file_ << line;
        file_.flush();
    }
    // Also write to stderr in debug mode
    if (debug_) {
        ::write(STDERR_FILENO, line.data(), line.size());
    }
}

std::string Logger::levelStr(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf) + "." + std::to_string(ms.count());
}

} // namespace tdesktop
