#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/package_root_resolver.hpp"

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
 * @brief Reads `projectRoot/gamePackage/package.json` and pairs it with
 * `shaderBackend` for the shader compile step. Never throws — failures
 * are reported through the returned status/errorMessage instead.
 */
ShaderPackageLoadResult LoadShaderPackageMetadata(
    const std::filesystem::path& projectRoot, const std::string& gamePackage,
    const std::string& shaderBackend, const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
