#include "log_engine/log_processing_engine.hpp"

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void usage(const char* program) {
    std::cout << "Multithreaded Log Processing Engine\n\n"
              << "Usage: " << program << " <input-file> [options]\n\n"
              << "Options:\n"
              << "  -t, --threads N       Worker count (default: CPU count)\n"
              << "  -q, --queue-size N    Bounded queue size (default: 4096)\n"
              << "  -l, --level LEVEL     Minimum level: TRACE..FATAL (default: TRACE)\n"
              << "  -c, --contains TEXT   Only match lines containing TEXT\n"
              << "  -o, --output FILE     Write matching lines, preserving input order\n"
              << "  -h, --help            Show this help\n";
}

std::size_t positive_number(const std::string& value, const char* option) {
    std::size_t used = 0;
    const auto number = std::stoull(value, &used);
    if (used != value.size() || number == 0) throw std::invalid_argument(std::string(option) + " requires a positive integer");
    return static_cast<std::size_t>(number);
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) { usage(argv[0]); return 2; }
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") { usage(argv[0]); return 0; }
        const std::filesystem::path input = argv[1];
        logengine::EngineConfig config;
        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            auto value = [&](const char* option) -> std::string {
                if (++i >= argc) throw std::invalid_argument(std::string("Missing value for ") + option);
                return argv[i];
            };
            if (arg == "-t" || arg == "--threads") config.worker_count = positive_number(value(arg.c_str()), arg.c_str());
            else if (arg == "-q" || arg == "--queue-size") config.queue_capacity = positive_number(value(arg.c_str()), arg.c_str());
            else if (arg == "-l" || arg == "--level") {
                config.minimum_level = logengine::level_from_string(value(arg.c_str()));
                if (config.minimum_level == logengine::LogLevel::unknown) throw std::invalid_argument("Unsupported log level");
            } else if (arg == "-c" || arg == "--contains") config.contains = value(arg.c_str());
            else if (arg == "-o" || arg == "--output") config.output_path = value(arg.c_str());
            else if (arg == "-h" || arg == "--help") { usage(argv[0]); return 0; }
            else throw std::invalid_argument("Unknown option: " + arg);
        }

        const logengine::LogProcessingEngine engine(config);
        const auto stats = engine.process(input);
        std::cout << "Processed " << stats.total_lines << " lines; matched " << stats.matched_lines << "\n";
        for (std::size_t i = 0; i < stats.by_level.size(); ++i)
            std::cout << std::setw(7) << logengine::to_string(static_cast<logengine::LogLevel>(i)) << ": " << stats.by_level[i] << '\n';
        std::cout << "Elapsed: " << std::fixed << std::setprecision(3) << stats.elapsed_seconds << "s\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

