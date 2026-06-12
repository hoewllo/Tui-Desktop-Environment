#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace tdesktop {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    static Logger& instance();

    void init(const std::string& path = "/tmp/tdesktop.log");
    void setLevel(LogLevel level);
    void setDebug(bool debug);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& msg);
    std::string levelStr(LogLevel level);
    std::string timestamp();

    std::ofstream file_;
    LogLevel level_ = LogLevel::Info;
    bool debug_ = false;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) tdesktop::Logger::instance().debug(msg)
#define LOG_INFO(msg) tdesktop::Logger::instance().info(msg)
#define LOG_WARN(msg) tdesktop::Logger::instance().warn(msg)
#define LOG_ERROR(msg) tdesktop::Logger::instance().error(msg)

} // namespace tdesktop
