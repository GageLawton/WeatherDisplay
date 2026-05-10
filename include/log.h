#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>
#include <sstream>
#include <string>

// Simple severity-tagged logging. Output format:
//   [HH:MM:SS] [LEVEL] message
//
// Goes to stdout for INFO, stderr for WARN/ERROR. Systemd's journal
// captures both streams and adds its own absolute timestamp, so we
// only need wall-clock-of-day here for terminal use.

namespace logging {

enum class Level { INFO, WARN, ERROR };

inline const char* levelName(Level l) {
    switch (l) {
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
    }
    return "?";
}

inline std::string timeStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return std::string(buf);
}

inline void emit(Level lvl, const std::string& msg) {
    std::ostream& out = (lvl == Level::INFO) ? std::cout : std::cerr;
    out << "[" << timeStamp() << "] [" << levelName(lvl) << "] " << msg << std::endl;
}

} // namespace logging

// Convenience macros. Usage:
//   LOG_INFO("Weather: " << temp << "F");
//   LOG_WARN("OLED show failed");
//   LOG_ERROR("Failed to open I2C bus");
//
// The streaming syntax lets us mix variables and strings cleanly.
#define LOG_INFO(expr)  do { std::ostringstream _oss; _oss << expr; \
                             logging::emit(logging::Level::INFO,  _oss.str()); } while(0)
#define LOG_WARN(expr)  do { std::ostringstream _oss; _oss << expr; \
                             logging::emit(logging::Level::WARN,  _oss.str()); } while(0)
#define LOG_ERROR(expr) do { std::ostringstream _oss; _oss << expr; \
                             logging::emit(logging::Level::ERROR, _oss.str()); } while(0)

#endif