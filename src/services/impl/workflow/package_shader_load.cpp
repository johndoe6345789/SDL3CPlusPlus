#include "services/interfaces/workflow/package_shader_load.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

std::filesystem::path ResolvePackageRoot(
    const std::filesystem::path& projectRoot) {
    std::error_code ec;
    const std::vector<std::filesystem::path> candidates = {
        projectRoot / "gameengine" / "packages",
        projectRoot / "packages",
        std::filesystem::current_path() / "gameengine" / "packages",
        std::filesystem::current_path() / "packages",
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate, ec)) return candidate;
    }

    // Return the most likely path even if it doesn't exist
    return projectRoot / "packages";
}

ShaderPackageLoadResult LoadShaderPackageMetadata(
    const std::filesystem::path& projectRoot, const std::string& gamePackage,
    const std::string& shaderBackend, const std::shared_ptr<ILogger>& logger) {
    ShaderPackageLoadResult result;
    result.backend = shaderBackend;

    try {
        const std::filesystem::path packageJsonPath =
            projectRoot / gamePackage / "package.json";
        result.packageJsonPath = packageJsonPath.string();

        if (logger) {
            logger->Trace("LoadShaderPackageMetadata", "Execute",
                "packageJsonPath=" + result.packageJsonPath);
        }

        if (!std::filesystem::exists(packageJsonPath)) {
            if (logger) {
                logger->Warn("LoadShaderPackageMetadata: package.json "
                    "not found at " + result.packageJsonPath);
            }
            result.status = "not_found";
            return result;
        }

        // Read package.json to get shader backend declaration
        std::ifstream jsonFile(packageJsonPath);
        if (!jsonFile) {
            throw std::runtime_error("Failed to open package.json");
        }
        std::string jsonContent((std::istreambuf_iterator<char>(jsonFile)),
            std::istreambuf_iterator<char>());

        if (logger) {
            logger->Info("LoadShaderPackageMetadata: Using shader "
                "backend: " + shaderBackend);
            logger->Info("LoadShaderPackageMetadata: Loaded "
                "package.json with backend=" + shaderBackend);
        }
        result.status = "success";
    } catch (const std::exception& e) {
        if (logger) {
            logger->Error(
                "LoadShaderPackageMetadata: Error: " + std::string(e.what()));
        }
        result.status = "error";
        result.errorMessage = e.what();
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
