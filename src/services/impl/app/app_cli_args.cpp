#include "services/interfaces/app/app_bootstrap.hpp"

#include "services/interfaces/app/cli_env_override.hpp"

#include <iostream>

namespace sdl3cpp::services::app {

bool ParseCliArgs(int argc, char** argv, CliOptions& outOptions) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--game" && i + 1 < argc) {
            outOptions.gamePackage = argv[++i];
        } else if (arg == "--bootstrap" && i + 1 < argc) {
            outOptions.bootstrapPackage = argv[++i];
        } else if (arg == "--project-root" && i + 1 < argc) {
            outOptions.projectRoot = argv[++i];
        } else if (arg == "--env" && i + 1 < argc) {
            // Workflow JSON reads paths such as the Quake 3 pk3 through
            // ${env:NAME}; this supplies one for the run without editing
            // the workflow or exporting anything in the shell.
            const std::string assignment = argv[++i];
            if (!ApplyEnvOverride(assignment)) {
                std::cerr << "Invalid --env argument (expected NAME=VALUE): "
                          << assignment << std::endl;
                return false;
            }
        } else if (arg == "--trace") {
            outOptions.traceEnabled = true;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl
                      << "Usage: sdl3_app [--bootstrap NAME] [--game NAME] "
                         "[--project-root PATH] [--env NAME=VALUE] [--trace]"
                      << std::endl;
            return false;
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::app
