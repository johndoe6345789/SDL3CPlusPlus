#include "services/interfaces/app/app_bootstrap.hpp"

#include "services/interfaces/app/cli_env_override.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>

namespace sdl3cpp::services::app {

namespace {

bool IsUnset(const std::string& name) {
    const char* value = std::getenv(name.c_str());
    return value == nullptr || *value == '\0';
}

}  // namespace

void ApplyLaunchOptionDefaults(const std::filesystem::path& projectRoot,
                               const std::string& gamePackage,
                               const std::shared_ptr<ILogger>& logger) {
    std::ifstream file(projectRoot / "packages" / gamePackage /
                       "package.json");
    if (!file.is_open()) {
        return;
    }
    nlohmann::json package;
    try {
        file >> package;
    } catch (const std::exception& e) {
        logger->Warn("launch_options: unreadable package.json: " +
                     std::string(e.what()));
        return;
    }
    const auto options = package.find("launch_options");
    if (options == package.end() || !options->is_array()) {
        return;
    }
    for (const auto& option : *options) {
        const std::string name = option.value("env", "");
        const std::string fallback = option.value("default", "");
        if (name.empty() || fallback.empty() || !IsUnset(name)) {
            continue;
        }
        if (ApplyEnvOverride(name + "=" + fallback)) {
            logger->Info("launch_options: " + name + " defaulted to " +
                         fallback);
        }
    }
}

}  // namespace sdl3cpp::services::app
