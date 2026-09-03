#include "log_engine/log_processing_engine.hpp"

#include "log_engine/bounded_queue.hpp"
#include "log_engine/log_parser.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace logengine {
namespace {
struct WorkItem { std::size_t number; std::string text; };
struct OutputItem { std::size_t number; std::string text; };

std::size_t index_of(LogLevel level) { return static_cast<std::size_t>(level); }

bool passes(LogLevel actual, LogLevel minimum) {
    if (actual == LogLevel::unknown) return minimum == LogLevel::unknown || minimum == LogLevel::trace;
    if (minimum == LogLevel::unknown) return true;
    return static_cast<int>(actual) >= static_cast<int>(minimum);
}
}

LogProcessingEngine::LogProcessingEngine(EngineConfig config) : config_(std::move(config)) {
    if (config_.worker_count == 0) config_.worker_count = std::max(1u, std::thread::hardware_concurrency());
    if (config_.queue_capacity == 0) config_.queue_capacity = 1;
}

ProcessingStats LogProcessingEngine::process(const std::filesystem::path& input_path) const {
    if (config_.output_path &&
        std::filesystem::absolute(input_path).lexically_normal() ==
            std::filesystem::absolute(*config_.output_path).lexically_normal()) {
        throw std::runtime_error("Input and output paths must be different");
    }
    std::ifstream input(input_path);
    if (!input) throw std::runtime_error("Could not open input file: " + input_path.string());
    std::ofstream output;
    if (config_.output_path) {
        output.open(*config_.output_path, std::ios::trunc);
        if (!output) throw std::runtime_error("Could not open output file: " + config_.output_path->string());
    }

    const auto started = std::chrono::steady_clock::now();
    BoundedQueue<WorkItem> work(config_.queue_capacity);
    BoundedQueue<OutputItem> results(config_.queue_capacity);
    std::array<std::atomic<std::size_t>, 7> counts{};
    std::atomic<std::size_t> matched{0};
    std::size_t total = 0;

    std::thread writer;
    if (config_.output_path) {
        writer = std::thread([&] {
            std::map<std::size_t, std::string> pending;
            std::size_t next = 1;
            while (auto item = results.pop()) {
                pending.emplace(item->number, std::move(item->text));
                while (true) {
                    auto it = pending.find(next);
                    if (it == pending.end()) break;
                    if (!it->second.empty()) output << it->second << '\n';
                    pending.erase(it);
                    ++next;
                }
            }
        });
    }

    std::vector<std::thread> workers;
    workers.reserve(config_.worker_count);
    for (std::size_t i = 0; i < config_.worker_count; ++i) {
        workers.emplace_back([&] {
            LogParser parser;
            while (auto item = work.pop()) {
                auto entry = parser.parse(item->number, std::move(item->text));
                counts[index_of(entry.level)].fetch_add(1, std::memory_order_relaxed);
                const bool is_match = passes(entry.level, config_.minimum_level) &&
                    (config_.contains.empty() || entry.raw.find(config_.contains) != std::string::npos);
                if (is_match) matched.fetch_add(1, std::memory_order_relaxed);
                if (config_.output_path) results.push({entry.line_number, is_match ? std::move(entry.raw) : std::string{}});
            }
        });
    }

    std::string line;
    while (std::getline(input, line)) work.push({++total, std::move(line)});
    work.close();
    for (auto& worker : workers) worker.join();
    if (config_.output_path) {
        results.close();
        writer.join();
        if (!output) throw std::runtime_error("Failed while writing output file");
    }

    ProcessingStats stats;
    stats.total_lines = total;
    stats.matched_lines = matched.load();
    for (std::size_t i = 0; i < stats.by_level.size(); ++i) stats.by_level[i] = counts[i].load();
    stats.elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return stats;
}

} // namespace logengine
