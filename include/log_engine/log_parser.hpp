#pragma once

#include "log_engine/log_entry.hpp"

#include <string>

namespace logengine {

class LogParser {
public:
    // Expected format: 2026-01-15 10:23:45 [INFO] message
    // Timestamp is optional; supported levels may also appear without brackets.
    LogEntry parse(std::size_t line_number, std::string line) const;
};

} // namespace logengine

