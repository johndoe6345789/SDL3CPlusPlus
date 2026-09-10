#include "services/interfaces/app/app_bootstrap.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <vector>

namespace sdl3cpp::services::app {

bool RequirePackage(const std::filesystem::path& projectRoot,
                    const std::string& kind, const std::string& expectedType,
                    const std::string& name) {
    const std::filesystem::path manifest =
        projectRoot / "packages" / name / "package.json";
    if (std::filesystem::exists(manifest)) {
        return true;
    }
    std::cerr << "Unknown " << kind << " package: " << name << std::endl;
    std::cerr << "Looked for: " << manifest.string() << std::endl;

    const std::filesystem::path packagesDir = projectRoot / "packages";
    if (!std::filesystem::is_directory(packagesDir)) {
        return false;
    }
    std::vector<std::string> candidates;
    for (const auto& entry : std::filesystem::directory_iterator(packagesDir)) {
        const std::filesystem::path candidateManifest =
            entry.path() / "package.json";
        if (!std::filesystem::exists(candidateManifest)) {
            continue;
        }
        std::string type;
        std::ifstream candidateFile(candidateManifest);
        if (candidateFile.is_open()) {
            try {
                nlohmann::json candidateJson;
                candidateFile >> candidateJson;
                if (candidateJson.contains("type")) {
                    type = candidateJson["type"].get<std::string>();
                }
            } catch (const std::exception&) {
                // Unreadable manifest: fall through and skip it.
            }
        }
        if (type == expectedType) {
            candidates.push_back(entry.path().filename().string());
        }
    }
    if (candidates.empty()) {
        return false;
    }
    std::cerr << "Available " << kind << " packages:" << std::endl;
    for (const auto& candidate : candidates) {
        std::cerr << "  " << candidate << std::endl;
    }
    return false;
}

}  // namespace sdl3cpp::services::app
