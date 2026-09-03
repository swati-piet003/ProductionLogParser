#include "log_engine/bounded_queue.hpp"
#include "log_engine/log_parser.hpp"
#include "log_engine/log_processing_engine.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void parser_test() {
    logengine::LogParser parser;
    auto entry = parser.parse(7, "2026-01-15 10:23:45 [ERROR] database unavailable");
    require(entry.line_number == 7, "line number");
    require(entry.level == logengine::LogLevel::error, "level parsing");
    require(entry.timestamp == "2026-01-15 10:23:45", "timestamp parsing");
    require(entry.message == "database unavailable", "message parsing");
    require(parser.parse(8, "unstructured data").level == logengine::LogLevel::unknown, "unknown parsing");
}

void queue_test() {
    logengine::BoundedQueue<int> queue(2);
    int sum = 0;
    std::thread consumer([&] { while (auto value = queue.pop()) sum += *value; });
    queue.push(2); queue.push(3); queue.push(5); queue.close(); consumer.join();
    require(sum == 10, "bounded queue");
}

void engine_test() {
    const auto base = std::filesystem::temp_directory_path() / "log_engine_test";
    const auto input = base.string() + ".log";
    const auto output = base.string() + ".out";
    {
        std::ofstream file(input);
        file << "2026-01-01 00:00:00 [INFO] startup\n"
             << "2026-01-01 00:00:01 [ERROR] disk failure\n"
             << "WARN disk almost full\n"
             << "random line\n";
    }
    logengine::EngineConfig config;
    config.worker_count = 3;
    config.queue_capacity = 2;
    config.minimum_level = logengine::LogLevel::warn;
    config.output_path = output;
    const auto stats = logengine::LogProcessingEngine(config).process(input);
    require(stats.total_lines == 4 && stats.matched_lines == 2, "engine totals");
    require(stats.by_level[static_cast<std::size_t>(logengine::LogLevel::error)] == 1, "error count");
    std::ifstream result(output);
    std::string all((std::istreambuf_iterator<char>(result)), {});
    require(all == "2026-01-01 00:00:01 [ERROR] disk failure\nWARN disk almost full\n", "ordered output");
    result.close();
    std::filesystem::remove(input);
    std::filesystem::remove(output);
}
}

int main() {
    try {
        parser_test(); queue_test(); engine_test();
        std::cout << "All tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
