#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <fstream>

namespace nexus::core {

enum class LogLevel { Trace, Info, Warn, Error, Fatal };

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string tag;
    double timestamp;
};

class Logger {
public:
    static Logger& get() {
        static Logger instance;
        return instance;
    }

    void log(LogLevel lvl, const char* tag, const char* fmt, ...) {
        std::va_list args;
        va_start(args, fmt);
        char buf[2048];
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        std::lock_guard<std::mutex> lock(m_mutex);
        LogEntry entry{lvl, buf, tag ? tag : "", now()};
        m_entries.push_back(entry);
        if (m_entries.size() > 4096) m_entries.erase(m_entries.begin());

        const char* prefix = "TRACE";
        switch (lvl) {
            case LogLevel::Trace: prefix = "TRACE"; break;
            case LogLevel::Info:  prefix = "INFO";  break;
            case LogLevel::Warn:  prefix = "WARN";  break;
            case LogLevel::Error: prefix = "ERROR"; break;
            case LogLevel::Fatal: prefix = "FATAL"; break;
        }
        std::printf("[%s][%s] %s\n", prefix, entry.tag.c_str(), entry.message.c_str());
    }

    const std::vector<LogEntry>& entries() const { return m_entries; }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.clear();
    }

private:
    Logger() = default;
    double now() const {
        using namespace std::chrono;
        return duration<double>(steady_clock::now().time_since_epoch()).count();
    }
    std::vector<LogEntry> m_entries;
    std::mutex m_mutex;
};

#define NX_LOG(lvl, tag, ...) ::nexus::core::Logger::get().log(lvl, tag, __VA_ARGS__)
#define NX_TRACE(tag, ...) NX_LOG(::nexus::core::LogLevel::Trace, tag, __VA_ARGS__)
#define NX_INFO(tag, ...)  NX_LOG(::nexus::core::LogLevel::Info,  tag, __VA_ARGS__)
#define NX_WARN(tag, ...)  NX_LOG(::nexus::core::LogLevel::Warn,  tag, __VA_ARGS__)
#define NX_ERROR(tag, ...) NX_LOG(::nexus::core::LogLevel::Error, tag, __VA_ARGS__)
#define NX_FATAL(tag, ...) NX_LOG(::nexus::core::LogLevel::Fatal, tag, __VA_ARGS__)

} // namespace nexus::core
