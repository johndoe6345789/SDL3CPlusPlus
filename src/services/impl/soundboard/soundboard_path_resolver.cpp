#include "soundboard_path_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::filesystem::path FindPackageRoot(const std::filesystem::path& seed) {
    std::filesystem::path current = seed;
    for (int depth = 0; depth < 6; ++depth) {
        const auto candidate = current / "packages" / "soundboard";
        if (std::filesystem::exists(candidate)) {
            return current;
        }
        if (!current.has_parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    return seed;
}

}  // namespace

std::filesystem::path ResolveSoundboardPackageRoot(const std::shared_ptr<IConfigService>& configService) {
    std::filesystem::path seed = std::filesystem::current_path();
    if (configService) {
        const auto projectRoot = configService->GetProjectRoot();
        if (!projectRoot.empty()) {
            seed = projectRoot;
        }
    }
    return FindPackageRoot(seed) / "packages" / "soundboard";
}

}  // namespace sdl3cpp::services::impl
