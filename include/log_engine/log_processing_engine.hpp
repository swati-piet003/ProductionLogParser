#pragma once

#include "log_engine/log_entry.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace logengine {

struct EngineConfig {
    std::size_t worker_count{0};
    std::size_t queue_capacity{4096};
    LogLevel minimum_level{LogLevel::trace};
    std::string contains;
    std::optional<std::filesystem::path> output_path;
};

struct ProcessingStats {
    std::size_t total_lines{};
    std::size_t matched_lines{};
    std::array<std::size_t, 7> by_level{};
    double elapsed_seconds{};
};

class LogProcessingEngine {
public:
    explicit LogProcessingEngine(EngineConfig config = {});
    ProcessingStats process(const std::filesystem::path& input_path) const;

private:
    EngineConfig config_;
};

} // namespace logengine

