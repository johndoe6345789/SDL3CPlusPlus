#include "materialx_path_resolver.hpp"

#include <system_error>

namespace sdl3cpp::services::impl {

std::filesystem::path MaterialXPathResolver::Resolve(const std::filesystem::path& path,
                                                     const std::filesystem::path& scriptDirectory) const {
    if (path.empty()) {
        return {};
    }
    if (path.is_absolute()) {
        return path;
    }
    if (!scriptDirectory.empty()) {
        auto projectRoot = scriptDirectory.parent_path();
        if (!projectRoot.empty()) {
            std::error_code ec;
            auto resolved = std::filesystem::weakly_canonical(projectRoot / path, ec);
            if (!ec) {
                return resolved;
            }
        }
    }
    std::error_code ec;
    auto resolved = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return {};
    }
    return resolved;
}

}  // namespace sdl3cpp::services::impl
