#include "log_engine/log_parser.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <unordered_map>

namespace logengine {
namespace {
std::string uppercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}
}

const char* to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::trace: return "TRACE";
        case LogLevel::debug: return "DEBUG";
        case LogLevel::info: return "INFO";
        case LogLevel::warn: return "WARN";
        case LogLevel::error: return "ERROR";
        case LogLevel::fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}

LogLevel level_from_string(std::string value) {
    value = uppercase(std::move(value));
    if (value == "TRACE") return LogLevel::trace;
    if (value == "DEBUG") return LogLevel::debug;
    if (value == "INFO") return LogLevel::info;
    if (value == "WARN" || value == "WARNING") return LogLevel::warn;
    if (value == "ERROR") return LogLevel::error;
    if (value == "FATAL" || value == "CRITICAL") return LogLevel::fatal;
    return LogLevel::unknown;
}

LogEntry LogParser::parse(std::size_t line_number, std::string line) const {
    static const std::regex pattern(
        R"(^\s*(?:(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?)\s+)?\[?(TRACE|DEBUG|INFO|WARN(?:ING)?|ERROR|FATAL|CRITICAL)\]?\s*[-:]?\s*(.*)$)",
        std::regex::icase);
    std::smatch match;
    LogEntry entry;
    entry.line_number = line_number;
    entry.raw = std::move(line);
    if (std::regex_match(entry.raw, match, pattern)) {
        entry.timestamp = match[1].str();
        entry.level = level_from_string(match[2].str());
        entry.message = match[3].str();
    } else {
        entry.message = entry.raw;
    }
    return entry;
}

} // namespace logengine

