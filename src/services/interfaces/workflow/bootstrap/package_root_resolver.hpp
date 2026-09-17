#pragma once

#include <filesystem>

namespace sdl3cpp::services::impl {

/**
 * @brief Finds the packages directory nearest to `projectRoot`, trying
 * `<root>/gameengine/packages`, `<root>/packages`, then the same two
 * under the current working directory. Falls back to
 * `<root>/packages` (even if it doesn't exist) if none are found.
 */
std::filesystem::path ResolvePackageRoot(
    const std::filesystem::path& projectRoot);

}  // namespace sdl3cpp::services::impl
