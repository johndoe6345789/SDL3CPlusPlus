#pragma once

#include "services/interfaces/i_logger.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Outcome of loading a package's shader metadata: "success",
/// "not_found" (no package.json at the resolved path), or "error"
/// (exception while reading it, detailed in `errorMessage`).
struct ShaderPackageLoadResult {
    std::string status;
    std::string backend;
    std::string packageJsonPath;
    std::string errorMessage;
};

/**
 * @brief Finds the packages directory nearest to `projectRoot`, trying
 * `<root>/gameengine/packages`, `<root>/packages`, then the same two
 * under the current working directory. Falls back to
 * `<root>/packages` (even if it doesn't exist) if none are found.
 */
std::filesystem::path ResolvePackageRoot(
    const std::filesystem::path& projectRoot);

/**
 * @brief Reads `projectRoot/gamePackage/package.json` and pairs it with
 * `shaderBackend` for the shader compile step. Never throws — failures
 * are reported through the returned status/errorMessage instead.
 */
ShaderPackageLoadResult LoadShaderPackageMetadata(
    const std::filesystem::path& projectRoot, const std::string& gamePackage,
    const std::string& shaderBackend, const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
