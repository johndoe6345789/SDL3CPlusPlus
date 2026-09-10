#include "services/interfaces/app/app_bootstrap.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace sdl3cpp::services::app {

std::string LoadDefaultWorkflowPath(const std::filesystem::path& projectRoot,
                                     const std::string& gamePackage,
                                     const std::shared_ptr<ILogger>& logger) {
    std::filesystem::path packageJsonPath =
        projectRoot / "packages" / gamePackage / "package.json";
    std::string defaultWorkflow = "workflows/main.json";  // fallback

    if (std::filesystem::exists(packageJsonPath)) {
        std::ifstream packageFile(packageJsonPath);
        if (packageFile.is_open()) {
            try {
                nlohmann::json packageJson;
                packageFile >> packageJson;
                if (packageJson.contains("defaultWorkflow")) {
                    defaultWorkflow =
                        packageJson["defaultWorkflow"].get<std::string>();
                    logger->Info("Loaded package.json, defaultWorkflow: " +
                                 defaultWorkflow);
                }
            } catch (const std::exception& e) {
                logger->Warn("Failed to parse package.json: " +
                             std::string(e.what()));
            }
        }
    }
    return defaultWorkflow;
}

std::string DetermineShaderBackend(const std::filesystem::path& projectRoot,
                                    const std::string& bootstrapPackage) {
    std::string shaderDir = "msl";  // default (Mac)
    std::filesystem::path bootPkgPath =
        projectRoot / "packages" / bootstrapPackage / "package.json";
    if (std::filesystem::exists(bootPkgPath)) {
        std::ifstream bootFile(bootPkgPath);
        nlohmann::json bootJson;
        bootFile >> bootJson;
        if (bootJson.contains("config") &&
            bootJson["config"].contains("renderer")) {
            std::string renderer =
                bootJson["config"]["renderer"].get<std::string>();
            if (renderer != "metal") shaderDir = "spirv";
        }
    }
    return shaderDir;
}

}  // namespace sdl3cpp::services::app
