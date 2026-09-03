#pragma once

#include <cstddef>
#include <string>

namespace logengine {

enum class LogLevel { trace, debug, info, warn, error, fatal, unknown };

struct LogEntry {
    std::size_t line_number{};
    std::string raw;
    std::string timestamp;
    LogLevel level{LogLevel::unknown};
    std::string message;
};

const char* to_string(LogLevel level) noexcept;
LogLevel level_from_string(std::string value);

} // namespace logengine

