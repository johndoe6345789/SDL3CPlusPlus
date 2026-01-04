#pragma once

#include "config_types.hpp"
#include <filesystem>
#include <optional>

namespace sdl3cpp::services {

/**
 * @brief Parsed command-line options.
 */
struct CommandLineOptions {
    RuntimeConfig runtimeConfig;
    std::optional<std::filesystem::path> seedOutput;
    bool saveDefaultJson = false;
    bool dumpRuntimeJson = false;
    bool traceEnabled = false;
};

/**
 * @brief Command line parsing service interface.
 */
class ICommandLineService {
public:
    virtual ~ICommandLineService() = default;
    virtual CommandLineOptions Parse(int argc, char** argv) = 0;
};

}  // namespace sdl3cpp::services
