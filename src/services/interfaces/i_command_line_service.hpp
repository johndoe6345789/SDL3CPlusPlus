#pragma once

#include "services/interfaces/config_types.hpp"
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
    std::string bootstrapPackage;  // Bootstrap package name (e.g., "bootstrap_mac")
    std::string gamePackage;       // Game package name (e.g., "seed")
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
