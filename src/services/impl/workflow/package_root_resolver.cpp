#include "services/interfaces/workflow/package_root_resolver.hpp"

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

}  // namespace sdl3cpp::services::impl
