#include "services/interfaces/workflow/graphics/graphics_screenshot_request_helpers.hpp"

#include <cstdlib>
#include <filesystem>

namespace sdl3cpp::services::impl {

std::string ResolveScreenshotOutputPath(const std::string& rawPath) {
    std::string resolved = rawPath;
    if (!resolved.empty() && resolved[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            resolved = std::string(home) + resolved.substr(1);
        }
    }

    std::filesystem::path path(resolved);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    return resolved;
}

std::string ToBmpPath(const std::string& path) {
    if (path.size() > 4 && path.substr(path.size() - 4) == ".png") {
        return path.substr(0, path.size() - 4) + ".bmp";
    }
    return path;
}

}  // namespace sdl3cpp::services::impl
