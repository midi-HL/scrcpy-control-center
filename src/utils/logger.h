#pragma once
#include <string>
#include <deque>
#include <mutex>
#include <cstdint>

// Thread-safe logger with ImGui rendering support
class Logger {
public:
    enum class Level : uint8_t { Info, Warning, Error, Debug };

    static Logger& instance();

    void log(Level level, const char* fmt, ...);
    void clear();

    // ImGui rendering
    void renderPanel();

    // Accessors for log data (used by content_panel)
    const std::deque<std::string>& getLines() const { return m_lines; }
    size_t getMaxLines() const { return MAX_LINES; }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static constexpr size_t MAX_LINES = 1000;
    std::mutex m_mutex;
    std::deque<std::string> m_lines;
    char m_filterBuf[256] = "";
    bool m_autoScroll = true;

    const char* levelTag(Level level) const;
};

// Convenience macros
#define LOG_INFO(fmt, ...)  Logger::instance().log(Logger::Level::Info, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Logger::instance().log(Logger::Level::Warning, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::instance().log(Logger::Level::Error, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) Logger::instance().log(Logger::Level::Debug, fmt, ##__VA_ARGS__)
